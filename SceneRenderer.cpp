#include "SceneRenderer.h"

#include "Mesh3D.h"


namespace
{
    constexpr double haloRadiusFactor{ 7 }; // attractor halo extent, in body radii

    // moves a mesh built along the x axis to _origin, its x axis along _axis, turning around it
    std::function< Vector3( const Vector3 & ) > Placement( const Vector3 & _origin, const Vector3 & _axis, const double _turn )
    {
        const auto reference{ std::abs( _axis.z ) < 0.9 ? Vector3{ 0, 0, 1 } : Vector3{ 1, 0, 0 } };
        const auto side{ _axis.CrossProd( reference ).Normalized() };
        const auto up{ _axis.CrossProd( side ) };
        const auto e2{ side * std::cos( _turn ) + up * std::sin( _turn ) };
        const auto e3{ up * std::cos( _turn ) - side * std::sin( _turn ) };
        return [ = ]( const Vector3 & _v ) { return _origin + _axis * _v.x + e2 * _v.y + e3 * _v.z; };
    }
}


SceneRenderer::SceneRenderer( const Dimension_ui & _screenDimension )
    : m_screenDimension{ _screenDimension }
    , m_screenCenter{ static_cast< double >( _screenDimension.width ) * 0.5, static_cast< double >( _screenDimension.height ) * 0.5 }
    , m_starField{ _screenDimension }
{}


bool SceneRenderer::_Visible( const World & _world, const Vector & _worldPosition, const double _radius ) const
{
    const auto relative{ _worldPosition - _world.Ship().position };
    return std::abs( relative.u ) <= m_screenCenter.u + _radius
        && std::abs( relative.v ) <= m_screenCenter.v + _radius;
}


void SceneRenderer::DrawBackdrop( const World & _world, const NanoVGRenderer::Frame & _frame )
{
    m_starField.SetDensity( _world.Stage().StarDensity() );
    m_starField.Draw( _frame, _world.Ship().momentum * 2 );
}


void SceneRenderer::_DrawWarden( const Enemy & _enemy, const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    // the sentinel of the belt: a faceted crystal sphere turning slowly, its eye following you, wrapped
    // in a gyroscope of three tumbling rings and a crown of spikes, shielded by a barrier as long as its
    // three pylons (gravity wells, tethered to the core) stand. Once they fall, the barrier drops and the
    // eye burns open.
    // It is drawn as solids (see Mesh3D), in pixels: the pylons orbit on a tilted, precessing plane, in
    // front of and behind the core, and are hit and pull exactly where they are projected.
    const auto & rocket{ _enemy.rocket };
    const auto & boss{ _enemy.boss };
    const auto center{ rocket.position + _translation };
    const auto R{ rocket.dynamic.bodyRadius };
    const auto alive{ _enemy.PylonsAlive() };
    const bool exposed{ alive == 0 };
    const auto health{ exposed ? std::clamp( rocket.shield.value / boss.maxShield, 0.0, 1.0 ) : 1.0 };
    m_wardenAnim += 0.02 + ( Enemy::pylonCount - alive ) * 0.008;
    const auto anim{ m_wardenAnim };
    const Color_d ice{ 0.55, 0.8, 1 };
    const Color_d violet{ 0.72, 0.48, 1 };
    const Color_d accent{ exposed ? Color_d{ 1, 0.35 + 0.35 * health, 0.55 + 0.3 * health } : violet };
    const bool hit{ boss.coreFlash > 0 };

    Scene3D scene{ center, 0, 1 };
    const auto r{ R * 0.85 }; // the sphere
    const auto viewer{ Vector3::Viewer() };
    const auto project{ [ & ]( const Vector3 & _point ) { return scene.ProjectModel( _point ); } };

    // where the eye looks: toward the ship, and a bit toward the viewer
    const auto toShip{ m_screenCenter - center };
    const auto gaze{ ( Vector3{ toShip.u, toShip.v, 0 }.Normalized() * 0.55 + Vector3{ 0, 0, 0.8 } ).Normalized() };

    scene.Add( { 0, 0, -400 }, [ & ]() {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, R * 3.2, { accent.r, accent.g, accent.b, 0.2 }, { accent.r, accent.g, accent.b, 0 } );
        for( int i{ 0 }; i < Enemy::pylonCount; i++ ) { // the wells: contracting rings show the pull
            if( !_enemy.PylonAlive( i ) )
                continue;
            const auto position{ _enemy.PylonPosition( i ) + _translation };
            for( int ring{ 0 }; ring < 3; ring++ ) {
                const auto phase{ std::fmod( anim * 0.6 + ring / 3.0, 1.0 ) };
                _frame.StrokeCircle( position, 190 * ( 1 - phase ) + 30, { violet.r, violet.g, violet.b, 0.22 * phase }, 2 );
            }
        }
    } );

    // the track of the pylons, a faint tilted ellipse: its far side below the core, its near side above
    const auto drawTrack{ [ & ]( const bool _front ) {
            std::vector< Position_d > run;
            const auto flush{ [ & ]() {
                    if( run.size() >= 2 )
                        _frame.StrokePolyline( run, { violet.r, violet.g, violet.b, _front ? 0.3 : 0.15 }, 1.5 );
                    run.clear();
                } };
            constexpr int steps{ 72 };
            for( int i{ 0 }; i <= steps; i++ ) {
                const auto point{ _enemy.OrbitPoint( Maths::Pi2 * i / steps ) };
                if( ( point.DotProd( viewer ) >= 0 ) == _front )
                    run.emplace_back( project( point ) );
                else
                    flush();
            }
            flush();
        } };
    scene.Add( { 0, 0, -390 }, [ & ]() { drawTrack( false ); } );
    scene.Add( { 0, 0, 150 }, [ & ]() { drawTrack( true ); } );

    // the core: a faceted crystal sphere, plated, turning slowly; its facets burn once the barrier is down
    const auto spinZ{ anim * 0.12 }, spinX{ 0.35 + 0.2 * std::sin( anim * 0.2 ) };
    scene.Add( Mesh3D::Icosphere( r, [ & ]( const int _face, const Vector3 &, Mesh3D::Face & _out ) {
            const auto variation{ 1.0 + 0.5 * std::fmod( _face * 0.6180339, 1.0 ) };
            const bool plate{ _face % 5 == 0 };
            _out.color = hit ? Color_d{ 0.9, 0.85, 1 } : ( plate ? Color_d{ 0.3, 0.22, 0.5 } : Color_d{ 0.17 * variation, 0.11 * variation, 0.3 * variation } );
            _out.glow = hit ? 0.5 : ( exposed && plate ? 0.45 + 0.25 * std::sin( anim * 6 + _face ) : 0 );
            if( exposed && plate && !hit )
                _out.color = accent;
            _out.edge = { ice.r, ice.g, ice.b, 0.55 };
            _out.edgeWidth = 1.5;
        } ).Transformed( [ & ]( const Vector3 & _v ) { return _v.RotatedZ( spinZ ).RotatedX( spinX ); } ) );

    // the eye, on the sphere where it looks: dim and shuttered under the barrier, burning once exposed
    scene.Add( { 0, 0, 120 }, [ & ]() {
        const auto side{ gaze.CrossProd( Vector3{ 0, 0, 1 } ).Normalized() };
        const auto up{ gaze.CrossProd( side ) };
        const auto eyeRadius{ r * 0.42 };
        const auto circle{ [ & ]( const double _radius, const double _lift ) { // a circle on the sphere around the gaze
                std::vector< Position_d > points;
                for( int i{ 0 }; i < 28; i++ ) {
                    const auto angle{ Maths::Pi2 * i / 28 };
                    points.emplace_back( project( ( gaze * r + ( side * std::cos( angle ) + up * std::sin( angle ) ) * _radius ).Normalized() * ( r + _lift ) ) );
                }
                return points;
            } };
        const auto pulse{ ( std::sin( anim * ( exposed ? 9 : 3 ) ) + 1 ) * 0.5 };
        const Color_d eye{ hit && exposed ? Color_d{ 1, 1, 1 } : ( exposed ? accent : ice ) };
        const auto eyeCenter{ project( gaze * ( r + 1 ) ) };
        {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            _frame.GradientCircle( eyeCenter, r * ( exposed ? 1.6 + pulse * 0.5 : 0.8 ), { eye.r, eye.g, eye.b, exposed ? 0.55 : 0.25 }, { eye.r, eye.g, eye.b, 0 } );
        }
        _frame.FillPolygon( circle( eyeRadius * 1.25, 1 ), { 0.04, 0.02, 0.08 } );
        _frame.StrokePolygon( circle( eyeRadius * 1.25, 1 ), violet, 3 );
        _frame.FillPolygon( circle( eyeRadius * ( exposed ? 0.8 + pulse * 0.15 : 0.55 ), 2 ), eye );
        _frame.FillPolygon( circle( eyeRadius * 0.28, 3 ), { 0.05, 0.03, 0.08 } ); // the pupil
        if( exposed )
            for( int i{ 0 }; i < 12; i++ ) { // crown of flames
                const auto angle{ anim * 2 + Maths::Pi2 * i / 12 };
                const auto direction{ side * std::cos( angle ) + up * std::sin( angle ) };
                const auto from{ ( gaze * r + direction * eyeRadius * 1.25 ).Normalized() * ( r + 2 ) };
                const auto to{ ( gaze * r + direction * eyeRadius * ( 1.7 + 0.4 * std::sin( anim * 11 + i ) ) ).Normalized() * ( r + 2 ) };
                _frame.Line( project( from ), project( to ), { accent.r, accent.g, accent.b, 0.9 }, 3 );
            }
    } );

    // the crown: eight spikes around a tilted equator
    const auto crownTilt{ 1.15 + 0.2 * std::sin( anim * 0.3 ) }, crownTurn{ anim * 0.2 };
    for( int i{ 0 }; i < 8; i++ ) {
        const auto direction{ Vector3{ std::cos( anim * 0.5 + Maths::Pi2 * i / 8 ), std::sin( anim * 0.5 + Maths::Pi2 * i / 8 ), 0 }.RotatedX( crownTilt ).RotatedY( crownTurn ) };
        scene.Add( Mesh3D::Lathe( { { 0, 13 }, { 92, 0 } }, 4, 0, 0,
                [ & ]( const int, const int _segment ) { return _segment % 2 == 0 ? Color_d{ 0.5, 0.36, 0.8 } : Color_d{ 0.3, 0.2, 0.55 }; }, { ice.r, ice.g, ice.b, 0.85 } )
            .Transformed( Placement( direction * ( r * 0.92 ), direction, 0 ) ) );
    }

    // the gyroscope: three tumbling rings, banded, turning on themselves
    const auto ring{ [ & ]( const double _radius, const double _tube, const double _tiltX, const double _tiltY, const double _spin ) {
            scene.Add( Mesh3D::Torus( _radius, _tube, 24, 5, [ & ]( const int _segment, const int, Mesh3D::Face & _out ) {
                    const bool lit{ _segment % 6 < 3 };
                    _out.color = lit ? Color_d{ 0.45, 0.7, 0.95 } : Color_d{ 0.3, 0.2, 0.55 };
                    _out.glow = exposed && lit ? 0.35 : 0.1;
                    _out.edge = { ice.r, ice.g, ice.b, 0.5 };
                    _out.edgeWidth = 1;
                } ).Transformed( [ & ]( const Vector3 & _v ) { return _v.RotatedZ( _spin ).RotatedX( _tiltX ).RotatedY( _tiltY ); } ) );
        } };
    ring( r * 1.35, 5, 0.9 + 0.3 * std::sin( anim * 0.4 ), anim * 0.35, -anim * 1.2 );
    ring( r * 1.6, 4, -0.7, 0.6 + anim * 0.5, anim * 1.8 );
    ring( r * 1.9, 3, 1.4 + 0.2 * std::sin( anim * 0.27 ), -anim * 0.25, anim * 0.9 );

    // the barrier, up while a pylon stands; flashing white when shot: a translucent sphere with a turning lattice
    if( !exposed )
        scene.Add( { 0, 0, 230 }, [ & ]() {
            const auto radius{ R * 1.65 };
            const auto first{ viewer.CrossProd( Vector3{ 1, 0, 0 } ).Normalized() };
            const auto second{ viewer.CrossProd( first ).Normalized() };
            std::vector< Position_d > silhouette; // what the sphere looks like along the view
            for( int i{ 0 }; i < 48; i++ )
                silhouette.emplace_back( project( ( first * std::cos( Maths::Pi2 * i / 48 ) + second * std::sin( Maths::Pi2 * i / 48 ) ) * radius ) );
            const auto barrier{ hit ? Color_d{ 1, 1, 1, 0.95 } : Color_d{ violet.r, violet.g, violet.b, 0.45 } };
            _frame.FillPolygon( silhouette, { violet.r, violet.g, violet.b, hit ? 0.28 : 0.07 } );
            _frame.StrokePolygon( silhouette, barrier, hit ? 5 : 2 );
            for( int great{ 0 }; great < 3; great++ ) // the lattice: great circles, the near half brighter
                for( const bool front : { false, true } ) {
                    std::vector< Position_d > run;
                    const auto flush{ [ & ]() {
                            if( run.size() >= 2 )
                                _frame.StrokePolyline( run, { barrier.r, barrier.g, barrier.b, barrier.a * ( front ? 0.6 : 0.2 ) }, hit ? 3 : 1.5 );
                            run.clear();
                        } };
                    for( int i{ 0 }; i <= 48; i++ ) {
                        const auto angle{ Maths::Pi2 * i / 48 };
                        const auto point{ Vector3{ std::cos( angle ), std::sin( angle ), 0 }.RotatedX( 0.5 + great * 1.0 ).RotatedY( anim * 0.3 + great * 0.8 ) * radius };
                        if( ( point.DotProd( viewer ) >= 0 ) == front )
                            run.emplace_back( project( point ) );
                        else
                            flush();
                    }
                    flush();
                }
        } );

    // the pylons: crystal obelisks tumbling on their long axis, tethered to the core, wearing a ring of remaining hull
    for( int i{ 0 }; i < Enemy::pylonCount; i++ ) {
        const auto space{ _enemy.PylonSpace( i ) };
        const auto index{ static_cast< size_t >( i ) };
        const auto position{ project( space ) };
        const auto depth{ space.DotProd( viewer ) / Enemy::pylonOrbit }; // -1 far .. 1 near: the size, the brightness
        const auto scale{ 1 + 0.15 * depth };
        if( !_enemy.PylonAlive( i ) ) { // a smoldering stump
            scene.Add( space, [ &, position, scale, i ]() {
                const auto flicker{ 0.5 + 0.5 * std::sin( anim * 8 + i * 2 ) };
                _frame.FillCircle( position, 9 * scale, { 0.12, 0.08, 0.15 } );
                _frame.FillCircle( position, 4 * scale, { 1, 0.4, 0.2, 0.4 + 0.4 * flicker } );
            } );
            continue;
        }
        const auto radial{ space.Normalized() };
        const auto surface{ radial * r };
        scene.Add( ( space + surface ) * 0.5, [ &, space, surface, i ]() {
            {
                const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                _frame.Line( project( space ), project( surface ), { violet.r, violet.g, violet.b, 0.35 }, 7 );
            }
            _frame.Line( project( space ), project( surface ), { ice.r, ice.g, ice.b, 0.7 }, 2 );
            for( int bead{ 0 }; bead < 3; bead++ ) { // energy flowing to the core
                const auto t{ std::fmod( anim * 0.7 + bead / 3.0 + i * 0.3, 1.0 ) };
                _frame.FillCircle( project( space + ( surface - space ) * t ), 3.5, { 1, 1, 1, 0.9 } );
            }
        } );
        const bool flash{ boss.pylonFlash.at( index ) > 0 };
        scene.Add( Mesh3D::Lathe( { { -46 * scale, 0 }, { -30 * scale, 18 * scale }, { 28 * scale, 18 * scale }, { 46 * scale, 0 } }, 6, 0, 0,
                [ & ]( const int _band, const int _segment ) {
                    return flash ? Color_d{ 1, 1, 1 } : ( _segment % 2 == 0 ? Color_d{ 0.45, 0.7, 0.95 } : ( _band == 1 ? Color_d{ 0.3, 0.2, 0.55 } : Color_d{ 0.36, 0.3, 0.7 } ) );
                }, flash ? Color_d{ 1, 1, 1, 0.9 } : Color_d{ ice.r, ice.g, ice.b, 0.8 } ).Glowing( flash ? 0.6 : 0.2 )
            .Transformed( Placement( space, radial, anim * 1.5 + i * 2.1 ) ) );
        const auto ratio{ std::clamp( boss.pylonShield.at( index ) / boss.pylonMax, 0.0, 1.0 ) };
        scene.Add( space + Vector3{ 0, 0, 40 }, [ &, position, scale, flash, ratio, i ]() {
            const Color_d body{ flash ? Color_d{ 1, 1, 1 } : ice };
            {
                const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                _frame.GradientCircle( position, ( 36 + 6 * std::sin( anim * 5 + i ) ) * scale, { body.r, body.g, body.b, flash ? 0.8 : 0.4 }, { body.r, body.g, body.b, 0 } );
            }
            _frame.StrokeArc( position, 42 * scale, -Maths::Pi * 0.5, -Maths::Pi * 0.5 + Maths::Pi2 * ratio, { 1, 0.75 + 0.2 * ratio, 1, 0.8 }, 3 );
        } );
    }

    scene.Draw( _frame );
}


void SceneRenderer::_DrawForeman( const Enemy & _enemy, const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    // a guild mining barge, a real piece of heavy machinery: a steel hull with ore hoppers and a bridge,
    // two outrigger pods, and a bore head at its bow (a fluted drill spinning in a gyroscope of three
    // cutter rings). Its thrusters are small and where the real exhaust leaves: a main engine bell at the
    // stern, its glowing heart being the rear core (the weak spot), and four attitude nozzles on the pods.
    // The core's armor petals close over it, tremble while the vent warms up, and open when it is exposed.
    // Damage shows: cracks, fires, lost hoppers, a broken ring, a hotter hull.
    // It is drawn as solids (see Mesh3D), in body radii: the ground plane is the game's, so the core and
    // the nozzles are where the core is hit and where the exhaust comes out.
    const auto & rocket{ _enemy.rocket };
    const auto & boss{ _enemy.boss };
    const auto center{ rocket.position + _translation };
    const auto R{ rocket.dynamic.bodyRadius };
    const auto health{ std::clamp( rocket.shield.value / boss.maxShield, 0.0, 1.0 ) };
    const auto head{ rocket.orientation + Maths::Pi }; // the head faces the opposite way of the stern
    m_foremanAnim += 0.03 + ( 1 - health ) * 0.03;
    const auto anim{ m_foremanAnim };
    const auto core{ _enemy.CorePosition() + _translation };
    Scene3D scene{ center, head, R, 0.04 * std::sin( anim * 0.4 ) };
    const auto project{ [ & ]( const Vector3 & _point ) { return scene.ProjectModel( _point ); } };

    // the real exhaust anchors (see Rocket::RefreshGeometry), in the model space: x toward the head, in body radii
    const auto forward{ Vector::From( head, 1 ) };
    const auto across{ Vector::From( head + Maths::PiHalf, 1 ) };
    const auto toModel{ [ & ]( const Vector & _offset ) {
            return Vector3{ ( _offset.u * forward.u + _offset.v * forward.v ) / R, ( _offset.u * across.u + _offset.v * across.v ) / R, 0 };
        } };
    const auto directionToModel{ [ & ]( const double _angle ) { return Vector3{ std::cos( _angle - head ), std::sin( _angle - head ), 0 }; } };

    // guild amber and steel, the hull heating up as it fails:
    const auto heat{ 1 - health };
    const Color_d amber{ 1, 0.3 + health * 0.4, 0.15 + health * 0.1 };
    const Color_d edge{ amber.r, amber.g, amber.b, 0.85 };
    const Color_d edgeSoft{ amber.r, amber.g, amber.b, 0.4 };
    const Color_d hullDark{ 0.08 + heat * 0.08, 0.07, 0.07 };
    const Color_d deck{ 0.24 + heat * 0.16, 0.26 - heat * 0.08, 0.3 - heat * 0.14 };
    const Color_d hullSide{ 0.13 + heat * 0.1, 0.14 - heat * 0.04, 0.17 - heat * 0.07 };
    const Color_d steel{ 0.4, 0.43, 0.48 };
    const Color_d amberPlate{ 0.62, 0.38, 0.12 };
    const Color_d hazard{ 0.95, 0.62, 0.12 };
    const Color_d lit{ 1, 0.85, 0.55 };
    const Color_d none{ 0, 0, 0, 0 };

    scene.Add( { 0, 0, -3 }, [ & ]() {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, R * 2.1, { amber.r, amber.g, amber.b, 0.14 + heat * 0.08 }, { amber.r, amber.g, amber.b, 0 } );
    } );

    // the hull: keel, armored hull, an amber armor plate at the bow, the spine
    scene.Add( Mesh3D::Extrusion( { { 0.9, 0 }, { 0.6, 0.4 }, { 0.15, 0.52 }, { -0.5, 0.52 }, { -0.7, 0.34 }, { -0.7, -0.34 }, { -0.5, -0.52 }, { 0.15, -0.52 }, { 0.6, -0.4 } },
        -0.16, 0.04, hullDark, hullDark, edgeSoft, 1.5 ) );
    scene.Add( Mesh3D::Extrusion( { { 1.0, 0 }, { 0.7, 0.46 }, { 0.2, 0.6 }, { -0.5, 0.6 }, { -0.76, 0.4 }, { -0.76, -0.4 }, { -0.5, -0.6 }, { 0.2, -0.6 }, { 0.7, -0.46 } },
        0.04, 0.26, deck, hullSide, edge, 2.5 ) );
    scene.Add( Mesh3D::Extrusion( { { 0.97, 0 }, { 0.7, 0.42 }, { 0.54, 0.44 }, { 0.54, -0.44 }, { 0.7, -0.42 } }, 0.26, 0.32, amberPlate, hullSide, edge, 2 ) );
    scene.Add( Mesh3D::Extrusion( { { 0.5, 0 }, { 0.38, 0.16 }, { -0.36, 0.2 }, { -0.56, 0.14 }, { -0.56, -0.14 }, { -0.36, -0.2 }, { 0.38, -0.16 } },
        0.26, 0.42, steel, hullSide, edge, 2 ) );

    // deck lights along the hull edge, battle damage on the deck
    scene.Add( { 0, 0, 0.28 }, [ & ]() {
        for( int i{ 0 }; i < 6; i++ ) {
            const auto x{ 0.3 - i * 0.14 };
            for( const auto side : { -1.0, 1.0 } )
                _frame.FillCircle( project( { x, side * 0.57, 0.26 } ), 2, { lit.r, lit.g, lit.b, 0.4 + 0.5 * ( std::fmod( anim * 0.8 - i * 0.15, 1.0 ) < 0.2 ? 1 : 0 ) } );
        }
        for( const auto side : { -1.0, 1.0 } ) // the engine room vents, glowing hotter as the hull fails
            for( int i{ 0 }; i < 4; i++ ) {
                const auto x{ -0.62 + i * 0.07 };
                _frame.Line( project( { x, side * 0.3, 0.26 } ), project( { x + 0.04, side * 0.5, 0.26 } ), { 0.05, 0.04, 0.04, 0.9 }, 3 );
                _frame.Line( project( { x, side * 0.3, 0.26 } ), project( { x + 0.04, side * 0.5, 0.26 } ), { 1, 0.4 + 0.3 * health, 0.1, 0.25 + 0.6 * heat }, 1.5 );
            }
        if( boss.phase >= 1 ) {
            const Color_d crack{ 1, 0.35, 0.1, 0.9 };
            const std::vector< std::vector< Vector3 > > cracks{ { { 0.86, 0.1, 0.32 }, { 0.74, 0.2, 0.32 }, { 0.68, 0.13, 0.32 }, { 0.56, 0.26, 0.32 } },
                { { -0.48, -0.55, 0.26 }, { -0.38, -0.44, 0.26 }, { -0.44, -0.3, 0.26 } } };
            for( const auto & line : cracks )
                for( size_t i{ 0 }; i + 1 < line.size(); i++ )
                    _frame.Line( project( line.at( i ) ), project( line.at( i + 1 ) ), crack, 2 );
        }
    } );

    // the ore hoppers on the deck, two of them open on glowing ore; the damage costs some, and sets the places on fire
    for( int i{ 0 }; i < 4; i++ ) {
        const auto x{ i / 2 == 0 ? 0.24 : -0.14 };
        const auto y{ i % 2 == 0 ? -0.38 : 0.38 };
        const bool lost{ ( i == 1 && boss.phase >= 1 ) || ( i == 2 && boss.phase >= 2 ) };
        if( lost ) {
            scene.Add( { x, y, 0.32 }, [ &, x, y, i ]() {
                const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                const auto flicker{ 0.5 + 0.5 * std::sin( anim * 9 + i * 2.1 ) };
                _frame.GradientCircle( project( { x, y, 0.32 } ), R * ( 0.22 + 0.08 * flicker ), { 1, 0.55, 0.15, 0.65 }, { 1, 0.3, 0.1, 0 } );
                _frame.FillCircle( project( { x, y, 0.27 } ), R * 0.1, { 0.08, 0.05, 0.04 } ); // the scorched socket
                for( int k{ 0 }; k < 3; k++ ) { // flames licking up
                    const auto height{ 0.18 + 0.12 * ( 0.5 + 0.5 * std::sin( anim * 11 + i + k * 2.3 ) ) };
                    const Vector3 base{ x + ( k - 1 ) * 0.05, y, 0.28 };
                    _frame.Line( project( base ), project( { base.x, base.y, base.z + height } ), { 1, 0.5 + 0.2 * k, 0.15, 0.8 }, 3 - k * 0.6 );
                }
            } );
            continue;
        }
        const bool ore{ i == 0 || i == 3 };
        scene.Add( Mesh3D::Box( x - 0.15, x + 0.15, y - 0.12, y + 0.12, 0.26, 0.4, ore ? hullSide : steel, hullSide, edge, 1.5 ) );
        if( ore )
            scene.Add( Mesh3D::Box( x - 0.11, x + 0.11, y - 0.08, y + 0.08, 0.4, 0.405, amber, amber, none ).Glowing( 0.7 + 0.2 * std::sin( anim * 3 + i ) ) );
    }

    // the bridge at the stern of the spine, its windows lit, a mast with a beacon
    scene.Add( Mesh3D::Box( -0.54, -0.3, -0.17, 0.17, 0.42, 0.6, steel, hullSide, edge, 2 ) );
    scene.Add( Mesh3D::Box( -0.3, -0.29, -0.14, 0.14, 0.5, 0.56, lit, lit, none ).Glowing( 1 ) );
    scene.Add( Mesh3D::Beam( { -0.44, 0, 0.6 }, { -0.44, 0, 0.88 }, 0.015, steel, edgeSoft, 1 ) );
    scene.Add( { -0.44, 0, 0.88 }, [ & ]() {
        if( std::fmod( anim, 2 ) < 1 ) {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            _frame.GradientCircle( project( { -0.44, 0, 0.88 } ), 12, { 1, 0.2, 0.15, 0.9 }, { 1, 0.1, 0.1, 0 } );
        }
    } );

    // the outrigger pods along the flanks, a hazard band around their middle, running lights at their bows
    for( const auto side : { -1.0, 1.0 } ) {
        for( const auto x : { 0.08, -0.48 } )
            scene.Add( Mesh3D::Box( x - 0.06, x + 0.06, std::min( side * 0.56, side * 0.8 ), std::max( side * 0.56, side * 0.8 ), 0.0, 0.14, steel, hullSide, edgeSoft, 1.5 ) );
        scene.Add( Mesh3D::Lathe( { { 0.66, 0.03 }, { 0.54, 0.12 }, { 0.12, 0.12 }, { -0.12, 0.12 }, { -0.56, 0.12 }, { -0.64, 0.05 } }, 8, side * 0.86, 0.04,
            [ & ]( const int _band, const int _segment ) { return _band == 2 ? ( _segment % 2 == 0 ? hazard : hullDark ) : ( _band == 0 || _band == 4 ? steel : hullSide ); }, edgeSoft ) );
        scene.Add( { 0.66, side * 0.86, 0.04 }, [ &, side ]() {
            if( std::fmod( anim + ( side > 0 ? 0.5 : 0 ), 2 ) < 1 ) {
                const auto light{ project( { 0.67, side * 0.86, 0.04 } ) };
                {
                    const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                    _frame.GradientCircle( light, 14, { 1, 0.15, 0.1, 0.8 }, { 1, 0.1, 0.1, 0 } );
                }
                _frame.FillCircle( light, 3.5, { 1, 0.2, 0.15 } );
            }
        } );
        if( boss.phase >= 2 ) // a pod smoldering
            scene.Add( { -0.2, side * 0.86, 0.16 }, [ &, side ]() {
                const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                const auto flicker{ 0.5 + 0.5 * std::sin( anim * 7 + side * 2 ) };
                _frame.GradientCircle( project( { -0.2, side * 0.86, 0.16 } ), R * 0.2, { 1, 0.4, 0.1, 0.3 + 0.3 * flicker }, { 1, 0.3, 0.1, 0 } );
            } );
    }

    // the four attitude nozzles, exactly at the rotators' exhaust anchors and along their thrust: a small bell
    // each, its mouth warming up with the thrust (the real exhaust particles come out of it)
    for( const auto pair : { Rocket::Rotator::left, Rocket::Rotator::right } )
        for( const auto side : { Rocket::Rotator::left, Rocket::Rotator::right } ) {
            const auto & anchor{ rocket.dynamic.rotators.at( pair ).at( side ) };
            const auto mouth{ toModel( anchor.position ) };
            const auto direction{ directionToModel( anchor.orientation ) };
            const auto thrust{ std::clamp( std::abs( rocket.rotator.thrust.at( pair ) ) / std::max( rocket.rotator.power, 1e-9 ), 0.0, 1.0 ) };
            constexpr double length{ 0.17 };
            scene.Add( Mesh3D::Lathe( { { 0, 0.05 }, { 0.06, 0.06 }, { length, 0.1 } }, 8, 0, 0,
                [ & ]( const int, const int _segment ) { return _segment % 2 == 0 ? steel : hullSide; }, edge )
                .Transformed( Placement( mouth - direction * length, direction, 0 ) ) );
            scene.Add( mouth, [ &, mouth, thrust ]() {
                const auto position{ project( mouth ) };
                _frame.FillCircle( position, R * 0.07, { 0.1 + 0.9 * thrust, 0.06 + 0.5 * thrust, 0.05 + 0.1 * thrust } );
                if( thrust > 0 ) {
                    const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                    _frame.GradientCircle( position, R * ( 0.1 + 0.12 * thrust ), { 1, 0.55, 0.2, 0.25 + 0.35 * thrust }, { 1, 0.3, 0.1, 0 } );
                }
            } );
        }

    // the main engine at the stern: a housing, and a bell opening exactly where the main exhaust leaves; the core is its heart
    const auto engineMouth{ toModel( rocket.dynamic.engine.position ) };
    scene.Add( Mesh3D::Extrusion( { { -0.6, 0.42 }, { -0.82, 0.32 }, { -0.82, -0.32 }, { -0.6, -0.42 } }, -0.12, 0.22, deck, hullSide, edge, 2 ) );
    scene.Add( Mesh3D::Lathe( { { -0.78, 0.26 }, { engineMouth.x + 0.07, 0.3 }, { engineMouth.x, 0.4 } }, 16, 0, 0,
        [ & ]( const int _band, const int _segment ) { return _band == 1 ? ( _segment % 2 == 0 ? hazard : hullDark ) : steel; }, edge ) );
    const auto engineThrust{ std::clamp( std::abs( rocket.engine.thrust ) / std::max( rocket.engine.power, 1e-9 ), 0.0, 1.0 ) };

    // the bore head: a collar with hazard bands, the fluted drill spinning in it
    constexpr double boreX{ 1.1 }, boreZ{ 0.17 };
    scene.Add( Mesh3D::Lathe( { { 0.88, 0.2 }, { 0.96, 0.25 }, { 1.06, 0.25 }, { 1.12, 0.22 } }, 12, 0, boreZ,
        [ & ]( const int _band, const int _segment ) { return _band == 1 ? ( _segment % 2 == 0 ? hazard : hullDark ) : steel; }, edge ) );
    scene.Add( Mesh3D::Lathe( { { 1.12, 0.21 }, { 1.24, 0.17 }, { 1.36, 0.11 }, { 1.47, 0.04 }, { 1.52, 0 } }, 10, 0, boreZ,
        []( const int, const int _segment ) { return _segment % 2 == 0 ? Color_d{ 0.95, 0.62, 0.2 } : Color_d{ 0.3, 0.2, 0.14 }; },
        { amber.r, amber.g, amber.b, 0.35 }, 0.55, 5, anim * 3 ) );
    // the cutter rings: a gyroscope around the bore head, banded steel and amber, turning on themselves and
    // tumbling around the drill axis; the outer one breaks in the last phase
    const auto ring{ [ & ]( const double _radius, const double _tube, const double _tilt, const double _precession, const double _spin, const bool _broken ) {
            scene.Add( Mesh3D::Torus( _radius, _tube, 24, 5, [ & ]( const int _segment, const int, Mesh3D::Face & _out ) {
                    if( _broken && _segment >= 8 && _segment < 12 ) {
                        _out.color = none;
                        return;
                    }
                    const bool tooth{ _segment % 4 == 0 };
                    const bool band{ _segment % 8 < 4 };
                    _out.color = tooth ? lit : ( band ? Color_d{ amber.r * 0.85, amber.g * 0.85, amber.b * 0.85 } : steel );
                    _out.glow = tooth ? 0.6 : 0.05;
                    _out.edge = edge;
                    _out.edgeWidth = 1;
                } ).Transformed( [ & ]( const Vector3 & _v ) {
                    return _v.RotatedZ( _spin ).RotatedY( Maths::PiHalf + _tilt ).RotatedX( _precession ) + Vector3{ boreX, 0, boreZ };
                } ) );
        } };
    const auto wobble{ 1 + heat };
    ring( 0.36, 0.05, 0.1 * std::sin( anim * 0.7 ) * wobble, 0, anim * 2.2, false );
    ring( 0.48, 0.045, 0.9 + 0.12 * std::sin( anim * 0.5 ) * wobble, anim * 0.35, -anim * 1.4, false );
    ring( 0.6, 0.04, -0.9 + 0.15 * std::sin( anim * 0.33 ) * wobble, -anim * 0.25 + 1.5, anim * 0.9, boss.phase >= 2 );

    scene.AddLast( [ & ]() {
        // the rear core, the heart of the main engine: armored behind six petals, glowing amber while the
        // vent warms up, then exposed (cyan, flashing white when hit) as the petals open
        const bool armored{ boss.vent == 0 };
        const bool warning{ boss.vent == 1 };
        const bool flashing{ boss.coreFlash > 0 };
        const auto target{ boss.vent == 2 ? 1.0 : ( warning ? 0.18 : 0.0 ) };
        m_foremanOpen += ( target - m_foremanOpen ) * 0.15;
        const auto open{ m_foremanOpen };
        const auto pulse{ ( std::sin( m_foremanAnim * ( warning ? 14 : 5 ) ) + 1 ) * 0.5 };
        const auto r{ Enemy::coreRadius };
        const Color_d coreColor{ flashing ? Color_d{ 1, 1, 1 } : ( armored ? Color_d{ 0.45, 0.5, 0.55 } : ( warning ? Color_d{ 1, 0.6, 0.15 } : Color_d{ 0.35, 0.9, 1 } ) ) };
        {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            if( !armored )
                _frame.GradientCircle( core, r * ( flashing ? 4.5 : ( warning ? 2.5 + pulse * 1.5 : 3 + pulse ) ), { coreColor.r, coreColor.g, coreColor.b, flashing ? 0.7 : 0.4 }, { coreColor.r, coreColor.g, coreColor.b, 0 } );
            if( engineThrust > 0 )
                _frame.GradientCircle( project( engineMouth ), r * ( 1 + engineThrust ), { 1, 0.55, 0.2, 0.3 * engineThrust }, { 1, 0.3, 0.1, 0 } );
        }
        // the lip of the engine bell around it, hazard-striped
        _frame.FillCircle( core, r * 1.2, hullDark );
        for( int k{ 0 }; k < 16; k++ ) {
            const auto angle{ Maths::Pi2 * k / 16 };
            _frame.StrokeArc( core, r * 1.1, angle, angle + Maths::Pi2 / 16, k % 2 == 0 ? Color_d{ hazard.r * 0.8, hazard.g * 0.8, hazard.b * 0.8 } : Color_d{ 0.12, 0.1, 0.1 }, r * 0.16 );
        }
        _frame.StrokeCircle( core, r * 1.2, edge, 2.5 );
        _frame.FillCircle( core, r, { 0.04, 0.08, 0.12 } );
        _frame.FillCircle( core, r * ( flashing ? 0.7 : 0.1 + open * ( 0.35 + pulse * 0.15 ) + ( warning ? 0.15 : 0 ) ), coreColor );
        // the thin technical rings around the core, counter-turning: visible while armored, faster as the vent warms up,
        // spinning wildly (and breathing, with energy rings collapsing into it) once the core is open
        m_foremanCoreSpin += 0.025 + open * 0.3 + ( warning ? 0.07 : 0.0 );
        {
            const auto spin{ m_foremanCoreSpin };
            const auto strength{ 0.7 + 0.3 * open + ( warning ? 0.25 : 0.0 ) + ( flashing ? 0.4 : 0.0 ) };
            const auto breath{ 1 + open * ( 0.08 + 0.05 * pulse ) };
            const auto tint{ [ & ]( const double _alpha ) { return Color_d{ coreColor.r, coreColor.g, coreColor.b, std::min( _alpha * strength, 1.0 ) }; } };
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            for( int i{ 0 }; i < 3 && open > 0.05; i++ ) { // energy collapsing into the core
                const auto phase{ std::fmod( m_foremanAnim * 1.4 + i / 3.0, 1.0 ) };
                _frame.StrokeCircle( core, r * ( 3.3 - 2.0 * phase ), tint( 0.7 * open * std::sin( phase * Maths::Pi ) ), 2 );
            }
            const auto first{ r * 1.42 * breath }, second{ r * 1.75 * breath }, third{ r * 2.15 * breath };
            _frame.StrokeCircle( core, first, tint( 0.55 ), 1.5 ); // a fine full circle carrying four bright arcs
            for( int i{ 0 }; i < 4; i++ ) {
                const auto angle{ spin * 1.5 + Maths::Pi2 * i / 4 };
                _frame.StrokeArc( core, first, angle, angle + 0.7 + 0.6 * open, tint( 1 ), 3.5 + open );
            }
            for( int i{ 0 }; i < 28; i++ ) { // a dashed one turning the other way
                const auto angle{ -spin * 1.1 + Maths::Pi2 * i / 28 };
                _frame.StrokeArc( core, second, angle, angle + Maths::Pi2 / 56, tint( 1 ), 2 + open );
            }
            _frame.StrokeCircle( core, third, tint( 0.4 ), 1.5 ); // the outer one: ticks, and two satellites
            for( int i{ 0 }; i < 24; i++ ) {
                const auto angle{ spin * 0.6 + Maths::Pi2 * i / 24 };
                const auto length{ i % 6 == 0 ? 0.24 : 0.11 };
                _frame.Line( core + Vector::From( angle, third ), core + Vector::From( angle, third + r * length ), tint( i % 6 == 0 ? 1 : 0.65 ), 1.6 );
            }
            for( int i{ 0 }; i < 2; i++ ) {
                const auto satellite{ core + Vector::From( -spin * 2.2 + Maths::Pi * i, third ) };
                _frame.GradientCircle( satellite, 9, tint( 0.8 ), { coreColor.r, coreColor.g, coreColor.b, 0 } );
                _frame.FillCircle( satellite, 3.2, tint( 1 ) );
            }
        }
        // the armor petals, hinged on the rim: their tips come in to the middle when closed, and sweep outward
        // when open; each is creased along its middle, one half lit and the other in shade
        const auto lightAngle{ std::atan2( -0.35, -0.5 ) };
        for( int k{ 0 }; k < 6; k++ ) {
            const auto angle{ Maths::Pi2 * k / 6 + 0.3 };
            const auto tremble{ warning ? std::sin( m_foremanAnim * 40 + k * 1.7 ) * 1.5 : 0.0 };
            const auto tip{ core + Vector::From( angle, r * ( 0.05 + open * 1.5 ) + tremble ) };
            const auto hingeA{ core + Vector::From( angle - 0.52, r ) }, hingeB{ core + Vector::From( angle + 0.52, r ) };
            const auto hinge{ core + Vector::From( angle, r * 1.02 ) };
            const auto shade{ 0.55 + 0.45 * std::cos( angle - lightAngle ) };
            const auto tone{ [ & ]( const double _k ) { return Color_d{ std::min( steel.r * _k, 1.0 ), std::min( steel.g * _k, 1.0 ), std::min( steel.b * _k, 1.0 ) }; } };
            _frame.FillPolygon( { hingeA, hinge, tip }, tone( shade * 1.35 ) );
            _frame.FillPolygon( { hinge, hingeB, tip }, tone( shade * 0.85 ) );
            _frame.Line( hinge, tip, { 0, 0, 0, 0.35 }, 1 );
            const Color_d outline{ flashing ? Color_d{ 1, 1, 1, 0.9 } : ( warning ? Color_d{ 1, 0.6, 0.15, 0.6 + 0.4 * pulse } : edge ) };
            _frame.StrokePolygon( { hingeA, hingeB, tip }, outline, 2 );
            _frame.FillCircle( core + Vector::From( angle, r * 0.82 * ( 1 - open ) + r * open * 1.1 ), 2.2, { outline.r, outline.g, outline.b, 0.9 } );
        }
        _frame.StrokeCircle( core, r, { coreColor.r, coreColor.g, coreColor.b, 0.9 }, armored ? 5 : 3 );
    } );

    scene.Draw( _frame );
}


void SceneRenderer::_DrawSingularity( const World & _world, const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    // a micro black hole in fake 3D: a pure-black event horizon, a tilted accretion disk of Keplerian rings (hot spots
    // orbiting faster inward, the near half passing in front of the horizon, the far half behind it) with the far side's
    // light lensed over and under the hole, and two precessing polar jets; all intensifying toward the collapse
    const auto position{ _world.SingularityPosition() + _translation };
    m_singularityAnim += 0.12;
    const auto anim{ m_singularityAnim };
    const auto progress{ _world.SingularityProgress() };
    m_singularityDust += 0.011 * ( 1 + progress * 0.8 ); // the dust cycles quicker toward the collapse
    const auto intensity{ ( 0.7 + progress * 0.5 ) * 1.3 };
    const auto viewer{ Vector3::Viewer() };
    Scene3D scene{ position, 0, 1 };
    const auto project{ [ & ]( const Vector3 & _point ) { return scene.ProjectModel( _point ); } };

    const auto horizon{ 11 * intensity };
    const auto tilt{ 0.7 + 0.1 * std::sin( anim * 0.15 ) }, turn{ anim * 0.05 };
    const auto axis{ Vector3{ 0, 0, 1 }.RotatedX( tilt ).RotatedZ( turn ) }; // the normal of the disk, the jets' direction
    constexpr int ringCount{ 8 }, ringSteps{ 56 };
    const auto heat{ [ & ]( const double _layer ) { return Color_d{ 1 - _layer * 0.45, 0.7 - _layer * 0.45, 1, 1 }; } }; // white-violet inward, deep violet outward

    scene.Add( { 0, 0, -400 }, [ & ]() {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( position, 180 * intensity * 0.8, { 0.3, 0.12, 0.6, 0.3 + progress * 0.3 }, { 0.08, 0, 0.18, 0 } );
    } );

    // the accretion disk, one half at a time: the far half goes behind the horizon, the near half over it
    const auto drawDisk{ [ & ]( const bool _near ) {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            for( int k{ 0 }; k < ringCount; k++ ) {
                const auto layer{ k / ( ringCount - 1.0 ) };
                const auto radius{ horizon * ( 1.9 + k * 0.62 ) };
                const auto speed{ 2.6 * std::pow( 1.9 / ( 1.9 + k * 0.62 ), 1.5 ) * ( 1 + progress ) }; // Keplerian: faster inward, faster toward the collapse
                const auto color{ heat( layer ) };
                const auto point{ [ & ]( const double _angle ) { return Vector3{ std::cos( _angle ), std::sin( _angle ), 0 }.RotatedX( tilt ).RotatedZ( turn ) * radius; } };
                for( int i{ 0 }; i < ringSteps; i++ ) {
                    const auto a0{ Maths::Pi2 * i / ringSteps }, a1{ Maths::Pi2 * ( i + 1 ) / ringSteps };
                    if( ( point( ( a0 + a1 ) * 0.5 ).DotProd( viewer ) >= 0 ) != _near )
                        continue;
                    const auto spot{ 0.5 + 0.5 * std::cos( a0 - anim * speed + k * 1.3 ) }; // the hot spots orbiting
                    const auto brightness{ 0.18 + 0.82 * spot * spot };
                    _frame.Line( project( point( a0 ) ), project( point( a1 ) ), { color.r, color.g, color.b, brightness * ( 0.85 - layer * 0.35 ) }, 3.2 - layer * 1.6 );
                }
            }
        } };
    scene.Add( { 0, 0, -300 }, [ & ]() { drawDisk( false ); } );

    // the star dust: grains falling in from afar, more and more of them as the collapse nears, along spiraling orbits (mostly in the plane of the disk, some inclined), the fall
    // accelerating (the radius shrinks as 1 - t^2.2, the angle grows as t^2), leaving streaks that lengthen as they speed up
    const auto drawDust{ [ & ]( const bool _near ) {
            constexpr int grains{ 260 };
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            const auto hash{ []( const double _a, const double _b ) { const auto v{ std::sin( _a * 12.9898 + _b * 78.233 ) * 43758.5453 }; return v - std::floor( v ); } };
            const auto crowd{ progress * 1.5 }; // the cloud thickens progressively: every grain has its moment to join, all of them by two thirds of the life
            for( int i{ 0 }; i < grains; i++ ) {
                const auto presence{ std::clamp( ( crowd - hash( i, 9 ) ) * 6, 0.0, 1.0 ) };
                if( presence <= 0 )
                    continue;
                const auto phase{ hash( i, 1 ) + m_singularityDust };
                const auto cycle{ std::floor( phase ) };
                const auto t{ phase - cycle };
                const auto h{ [ & ]( const double _salt ) { return hash( i + cycle * 37, _salt ); } };
                const auto flat{ h( 2 ) < 0.75 }; // in the disk plane, or inclined
                const auto grainTilt{ flat ? tilt + ( h( 3 ) - 0.5 ) * 0.3 : h( 3 ) * Maths::Pi };
                const auto grainTurn{ flat ? turn + ( h( 4 ) - 0.5 ) * 0.3 : h( 4 ) * Maths::Pi2 };
                const auto start{ ( 100 + 150 * h( 5 ) ) * intensity / 1.1 }, end{ horizon * 1.2 };
                const auto angle0{ h( 6 ) * Maths::Pi2 };
                const auto position3{ [ & ]( const double _t ) {
                        const auto radius{ end + ( start - end ) * ( 1 - std::pow( _t, 2.2 ) ) };
                        const auto angle{ angle0 + 5 * _t * _t };
                        return Vector3{ std::cos( angle ), std::sin( angle ), 0 }.RotatedX( grainTilt ).RotatedZ( grainTurn ) * radius;
                    } };
                const auto head{ position3( t ) };
                if( ( head.DotProd( viewer ) >= 0 ) != _near )
                    continue;
                const auto tail{ position3( std::max( t - 0.018, 0.0 ) ) };
                const auto alpha{ std::min( t * 6, 1.0 ) * ( 0.12 + 0.88 * t * t ) * ( _near ? 1 : 0.65 ) * presence };
                const auto color{ heat( 1 - t ) }; // violet from afar, white-hot near the hole
                _frame.Line( project( tail ), project( head ), { color.r, color.g, color.b, alpha }, 0.8 + 1.4 * t );
                _frame.FillCircle( project( head ), 0.7 + 1.1 * t, { 1, 0.95, 1, alpha } );
            }
        } };
    scene.Add( { 0, 0, -250 }, [ & ]() { drawDust( false ); } );

    // the polar jets: beads flowing out along the axis, one toward the viewer and one away
    for( const double side : { -1.0, 1.0 } ) {
        const auto direction{ axis * side };
        scene.Add( direction * 100, [ &, direction ]() {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            for( int segment{ 0 }; segment < 4; segment++ ) {
                const auto from{ project( direction * ( horizon * 1.2 + segment * horizon * 1.9 ) ) };
                const auto to{ project( direction * ( horizon * 1.2 + ( segment + 1 ) * horizon * 1.9 ) ) };
                _frame.Line( from, to, { 0.55, 0.35, 1, 0.3 - segment * 0.06 }, 9 - segment * 2 );
                _frame.Line( from, to, { 0.85, 0.7, 1, 0.65 - segment * 0.14 }, 3.5 - segment * 0.7 );
            }
            for( int bead{ 0 }; bead < 3; bead++ ) {
                const auto t{ std::fmod( anim * 0.35 + bead / 3.0, 1.0 ) };
                _frame.FillCircle( project( direction * ( horizon * 1.2 + t * horizon * 7.6 ) ), 2.6 * ( 1 - t * 0.6 ), { 1, 0.9, 1, 0.9 * ( 1 - t ) } );
            }
        } );
    }

    // the event horizon: it must stay BLACK (normal blending), its rim flickering
    scene.Add( { 0, 0, 0 }, [ & ]() {
        _frame.FillCircle( position, horizon + std::sin( anim * 5 ) * 0.8, { 0.01, 0, 0.03 } );
        _frame.StrokeCircle( position, horizon * 1.08, { 0.85, 0.55, 1, 0.75 + std::sin( anim * 3 ) * 0.2 }, 2 );
    } );

    // the far side of the disk, lensed over and under the horizon: thin bright arcs turning with the disk
    scene.Add( { 0, 0, 40 }, [ & ]() {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        const auto shimmer{ 0.5 + 0.25 * std::sin( anim * 4 ) };
        _frame.StrokeCircle( position, horizon * 1.5, { 0.9, 0.7, 1, 0.3 + 0.2 * shimmer }, 1.2 );
        _frame.StrokeArc( position, horizon * 1.5, -Maths::PiHalf - 1.2, -Maths::PiHalf + 1.2, { 1, 0.85, 1, 0.55 + 0.3 * shimmer }, 3 );
        _frame.StrokeArc( position, horizon * 1.5, Maths::PiHalf - 0.8, Maths::PiHalf + 0.8, { 0.9, 0.6, 1, 0.3 + 0.2 * shimmer }, 2 );
    } );

    scene.Add( { 0, 0, 100 }, [ & ]() { drawDisk( true ); } );
    scene.Add( { 0, 0, 110 }, [ & ]() { drawDust( true ); } );

    // pre-collapse white flare:
    if( progress > 0.75 )
        scene.AddLast( [ & ]() {
            const auto flare{ ( progress - 0.75 ) * 4 };
            _frame.FillCircle( position, flare * 9, { 1, 0.95, 1, flare * 0.85 } );
        } );

    scene.Draw( _frame );
}


void SceneRenderer::Draw( const World & _world, const NanoVGRenderer::Frame & _frame )
{
    const auto & ship{ _world.Ship() };
    const Vector translation{ m_screenCenter - ship.position };

    // draw starfield, related to ship motion & solar wind:
    DrawBackdrop( _world, _frame );

    // draw particules:
    for( const auto & particule : _world.Particules() )
        if( _Visible( _world, particule.position, particule.width ) )
            _frame.FillCircle( particule.position + translation, particule.width, particule.GetColor() );

    // draw goodies:
    for( const auto & goody : _world.Goodies() )
        if( _Visible( _world, goody.position, goody.dynamic.radius + 10 ) )
            goody.Draw( _frame, translation );

    // draw mines:
    for( const auto & mine : _world.Mines() )
        if( _Visible( _world, mine.position, mine.dynamic.radius + 10 ) )
            mine.Draw( _frame, translation );

    // draw gravity mines (halo included in the culling extent):
    for( const auto & gravityMine : _world.GravityMines() )
        if( _Visible( _world, gravityMine.position, GravityMine::pullRange * 0.6 ) )
            gravityMine.Draw( _frame, translation );

    // singularity:
    if( _world.SingularityActive() && _Visible( _world, _world.SingularityPosition(), 280 ) )
        _DrawSingularity( _world, _frame, translation );

    // decoy beacon: shiny pulsing white-hot core, orbiting sparkles and double sonar pings:
    if( _world.DecoyActive() && _Visible( _world, _world.DecoyPosition(), 60 ) ) {
        const auto position{ _world.DecoyPosition() + translation };
        m_decoyPing += 0.035;
        const auto ping{ m_decoyPing - std::floor( m_decoyPing ) };
        const auto ping2{ m_decoyPing + 0.5 - std::floor( m_decoyPing + 0.5 ) };
        const auto pulse{ ( std::sin( m_decoyPing * Maths::Pi2 ) + 1 ) * 0.5 };
        constexpr Color_d beaconColor{ 1, 0.72, 0.28 };
        _frame.GradientCircle( position, 46 + pulse * 10,
            { beaconColor.r, beaconColor.g, beaconColor.b, 0.4 + pulse * 0.2 }, { 1, 0.4, 0.1, 0 } );
        _frame.FillCircle( position, 6.5 + pulse * 2, { 1, 0.95, 0.8 } );
        _frame.StrokeCircle( position, 10 + pulse * 2, { beaconColor.r, beaconColor.g, beaconColor.b, 0.95 }, 2 );
        for( int i{ 0 }; i < 4; i++ )
            _frame.FillCircle( position + Vector::From( m_decoyPing * 4 + i * Maths::PiHalf, 16 ), 1.8, { 1, 0.9, 0.6, 0.85 } );
        _frame.StrokeCircle( position, 12 + ping * 34, { beaconColor.r, beaconColor.g, beaconColor.b, ( 1 - ping ) * 0.7 }, 2.5 );
        _frame.StrokeCircle( position, 12 + ping2 * 34, { beaconColor.r, beaconColor.g, beaconColor.b, ( 1 - ping2 ) * 0.45 }, 2 );
    }

    // draw enemies (no target designation once the ship is gone):
    const Rocket * pTarget{ _world.ShipDestroyed() ? nullptr : _world.ClosestEnemy( ship.position ) };
    for( const auto & enemy : _world.Enemies() ) {
        if( pTarget == &enemy.rocket ) {
            // faint designation line toward the closest enemy:
            _frame.Line( m_screenCenter, enemy.rocket.position + translation, { 0.1, 0.5, 1, 0.4 }, 0.75 );
            const auto vector{ enemy.rocket.position - ship.position };
            const auto shipLengths{ static_cast< int >( vector.Distance() / ( ship.dynamic.boundingBoxRadius * 2 ) ) };
            if( shipLengths >= 15 ) { // below that the target is close enough, no pointer needed
                // chevron pointer orbiting the ship, with the distance in ship lengths:
                constexpr Color_d accent{ 0.35, 0.65, 1 };
                const auto angle{ vector.Orientation() };
                const auto tip{ m_screenCenter + Vector::From( angle, ship.dynamic.boundingBoxRadius + 46 ) };
                const auto wingA{ m_screenCenter + Vector::From( angle + 0.35, ship.dynamic.boundingBoxRadius + 32 ) };
                const auto wingB{ m_screenCenter + Vector::From( angle - 0.35, ship.dynamic.boundingBoxRadius + 32 ) };
                _frame.Line( wingA, tip, { accent.r, accent.g, accent.b, 0.9 }, 2 );
                _frame.Line( wingB, tip, { accent.r, accent.g, accent.b, 0.9 }, 2 );
                _frame.Text( m_screenCenter + Vector::From( angle, ship.dynamic.boundingBoxRadius + 66 ), "sourceCodePro", 13,
                    std::to_string( shipLengths ), { 0.5, 0.8, 1, 0.85 }, NanoVGRenderer::Frame::eTextAlign::center );
            }
        }
        // nozzle and flames extend beyond the bounding box, keep a comfortable margin (the foreman: its bore head, and the glow of its flashing core):
        if( _Visible( _world, enemy.rocket.position, enemy.type == Enemy::eType::warden ? Enemy::pylonOrbit * 1.5 : ( enemy.type == Enemy::eType::foreman ? enemy.rocket.dynamic.bodyRadius * 3 : enemy.rocket.dynamic.boundingBoxRadius + Enemy::coreRadius * 4 ) ) ) {
            if( enemy.type == Enemy::eType::foreman )
                _DrawForeman( enemy, _frame, translation );
            else if( enemy.type == Enemy::eType::warden )
                _DrawWarden( enemy, _frame, translation );
            else
                enemy.rocket.Draw( _frame, translation );
        }
    }

    // draw lasers:
    for( const auto & laser : _world.Lasers() )
        if( _Visible( _world, laser.position, 55 ) )
            laser.Draw( _frame, translation );

    // draw missiles:
    for( const auto & missile : _world.Missiles() )
        if( _Visible( _world, missile.rocket.position, missile.rocket.dynamic.boundingBoxRadius + 60 ) )
            missile.rocket.Draw( _frame, translation );

    // draw sniper slugs: long hot tracers with a white-hot head, unmissable:
    for( const auto & slug : _world.Slugs() )
        if( _Visible( _world, slug.position, 50 ) ) {
            const auto head{ slug.position + translation };
            _frame.GradientCircle( head, 24, { 1, 0.55, 0.25, 0.45 }, { 1, 0.3, 0.1, 0 } );
            _frame.Line( head - slug.momentum * 2.5, head, { 1, 0.45, 0.2, 0.9 }, 5 ); // hot trail
            _frame.Line( head - slug.momentum, head, { 1, 0.9, 0.7 }, 2.5 ); // white-hot core
            _frame.FillCircle( head, 3, { 1, 0.85, 0.6 } );
        }

    // draw attractors:
    _DrawAttractors( _world, _frame, translation );

    // blast shockwaves: an initial flash, a thick expanding leading ring and a
    // fainter trailing echo:
    for( const auto & blast : _world.Blasts() ) {
        if( !_Visible( _world, blast.position, blast.radius ) )
            continue;
        const auto position{ blast.position + translation };
        const auto age{ static_cast< double >( blast.age ) / World::Blast::maxAge };
        const auto expansion{ 1 - ( 1 - age ) * ( 1 - age ) };
        const auto ringRadius{ blast.radius * expansion };
        const auto fade{ std::pow( 1 - age, 1.2 ) };
        if( age < 0.35 ) // detonation flash filling the zone:
            _frame.GradientCircle( position, ringRadius,
                { blast.ringColor.r, blast.ringColor.g, blast.ringColor.b, ( 0.35 - age ) * 2.2 },
                { blast.ringColor.r, blast.ringColor.g, blast.ringColor.b, 0 } );
        _frame.StrokeCircle( position, ringRadius,
            { blast.ringColor.r, blast.ringColor.g, blast.ringColor.b, fade * 0.9 }, 2.5 + fade * 4 );
        _frame.StrokeCircle( position, ringRadius * 0.7,
            { blast.ringColor.r, blast.ringColor.g, blast.ringColor.b, fade * 0.4 }, 1.5 + fade * 2 );
    }

    // plasma shield, with an energy glow:
    const auto plasmaShieldSin{ std::sin( _world.PlasmaShieldRamp() * Maths::Pi ) };
    const auto plasmaShieldColor{ Color_d{ 0.5, 1, 0.75 } * plasmaShieldSin };
    if( _world.PlasmaShieldActive() ) {
        _frame.GradientCircle( m_screenCenter, _world.PlasmaShieldRadius() * 1.5,
            { 0.3, 1, 0.6, 0.16 * plasmaShieldSin }, { 0.1, 0.6, 0.3, 0 } );
        _frame.StrokeCircle( m_screenCenter, _world.PlasmaShieldRadius(), plasmaShieldColor, 2 * plasmaShieldSin );
    }

    // draw ship:
    if( !_world.ShipDestroyed() )
        ship.Draw( _frame, translation );

    // turret bonus, a mini-ship orbiting the main one:
    if( !_world.ShipDestroyed() && _world.TurretActive() ) {
        const auto position{ _world.TurretPosition() + translation };
        const auto orientation{ _world.TurretOrientation() };
        _frame.GradientCircle( position, 22, { 0.4, 0.7, 1, 0.35 }, { 0.2, 0.4, 1, 0 } ); // glow
        _frame.FillCircle( position, 6, { 0.05, 0.12, 0.2 } );
        _frame.StrokeCircle( position, 6, { 0.5, 0.75, 1 }, 2 );
        _frame.Line( position + Vector::From( orientation, 5 ), position + Vector::From( orientation, 13 ), { 1, 0.45, 0.55 }, 2.5 ); // barrel toward the target
        _frame.FillCircle( position, 2, { 0.7, 0.9, 1 } );
    }

    // plasma shield reflection:
    if( _world.PlasmaShieldActive() )
        _frame.Reflect( m_screenCenter, _world.PlasmaShieldRadius(), plasmaShieldColor, 0.4, m_plasmaShieldReflectAnimation += 0.3 );
}


void SceneRenderer::_DrawAttractors( const World & _world, const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    const auto & ship{ _world.Ship() };
    const auto & attractors{ _world.Attractors() };

    // collect the attractors whose body or halo intersects the screen; the grid query
    // only visits the cells around the view instead of the whole field:
    m_visibleAttractors.clear();
    const Vector viewMin{ ship.position - m_screenCenter };
    const Vector viewMax{ ship.position + m_screenCenter };
    attractors.ForEachInRect( viewMin, viewMax, attractors.MaxRadius() * haloRadiusFactor,
        [ & ]( const int, const Attractor & _attractor ){
            const auto haloRadius{ _attractor.radius * haloRadiusFactor };
            const bool haloVisible{ _Visible( _world, _attractor.position, haloRadius ) };
            const bool bodyVisible{ _Visible( _world, _attractor.position, _attractor.radius + 6 ) };
            if( !haloVisible && !bodyVisible )
                return;
            // color depends of distance with ship:
            const auto distance{ ( _attractor.position - ship.position ).Distance() };
            const auto mass{ _attractor.mass * ship.dynamic.totalMass };
            const double maxDistance{ AttractorField::distanceThreshold * mass };
            std::array< double, 3 > colors{};
            for( int i{ 0 }; i < 3; i++ ) {
                const auto currMaxDistance{ maxDistance / ( i + 1 ) };
                colors.at( i ) = distance < currMaxDistance ? std::pow( 1 - ( distance / currMaxDistance ), 2 ) : 0;
            }
            const auto intensity{ distance < maxDistance ? ( 1 - std::pow( distance / maxDistance, 2 ) ) : 0 };
            m_visibleAttractors.emplace_back( VisibleAttractor{
                _attractor.position + _translation,
                _attractor.radius,
                Color_d{ colors.at( 2 ), colors.at( 1 ), colors.at( 0 ) },
                intensity,
                bodyVisible,
                haloVisible } );
        } );

    // halos first (single additive composition scope), bodies on top:
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        for( const auto & attractor : m_visibleAttractors )
            if( attractor.haloVisible )
                _frame.GradientCircle( attractor.position, attractor.radius * haloRadiusFactor,
                    Color_d{ 0.01, 0.175, 0.2, attractor.intensity * 0.5 + 0.5 }, Color_d{ 0, 0.05, 0.1, 0 } );
    }
    for( const auto & attractor : m_visibleAttractors )
        if( attractor.bodyVisible ) {
            _frame.FillCircle( attractor.position, attractor.radius, attractor.bodyColor, false );
            _frame.StrokeCircle( attractor.position, attractor.radius, Color_d{ 0.25, 0.5, 1 }, attractor.intensity * 5.5 + 0.5 );
        }
}
