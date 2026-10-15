#include "SceneRenderer.h"

#include "Mesh3D.h"


namespace
{
    constexpr double haloRadiusFactor{ 7 }; // attractor halo extent, in body radii
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

    // moves a mesh built along the x axis to _origin, its x axis along _axis, turning around it
    const auto place{ [ & ]( const Vector3 & _origin, const Vector3 & _axis, const double _turn ) {
            const auto reference{ std::abs( _axis.z ) < 0.9 ? Vector3{ 0, 0, 1 } : Vector3{ 1, 0, 0 } };
            const auto side{ _axis.CrossProd( reference ).Normalized() };
            const auto up{ _axis.CrossProd( side ) };
            const auto e2{ side * std::cos( _turn ) + up * std::sin( _turn ) };
            const auto e3{ up * std::cos( _turn ) - side * std::sin( _turn ) };
            return [ = ]( const Vector3 & _v ) { return _origin + _axis * _v.x + e2 * _v.y + e3 * _v.z; };
        } };

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
            .Transformed( place( direction * ( r * 0.92 ), direction, 0 ) ) );
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
            .Transformed( place( space, radial, anim * 1.5 + i * 2.1 ) ) );
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
    // a guild mining barge, a real piece of heavy machinery: a three-tier armored hull carrying cargo
    // containers, a bridge tower and a swinging crane, two hazard-striped engine nacelles on struts, and a
    // giant fluted drill turning at its head. A pylon holds the rear core (the weak spot) behind the stern:
    // its armor petals close over it, tremble while the vent warms up, and open when it is exposed.
    // Damage shows: cracks, fires, lost containers, a hotter hull.
    // It is drawn as solids (see Mesh3D): the ground plane is the game's, so the core is where it is hit.
    const auto & rocket{ _enemy.rocket };
    const auto & boss{ _enemy.boss };
    const auto center{ rocket.position + _translation };
    const auto R{ rocket.dynamic.bodyRadius };
    const auto health{ std::clamp( rocket.shield.value / boss.maxShield, 0.0, 1.0 ) };
    const auto head{ rocket.orientation + Maths::Pi }; // the head faces the opposite way of the stern
    m_foremanAnim += 0.03 + ( 1 - health ) * 0.03;
    const auto anim{ m_foremanAnim };
    const auto core{ _enemy.CorePosition() + _translation };
    // drawn bigger than its hit circle, around the core: the core stays where it is hit, the barge extends toward its head
    constexpr double scale{ 1.25 }, coreX{ -1.12 }; // coreX: the core in model units, on the axis behind the stern
    const auto unit{ R * scale };
    const auto origin{ center + Vector::From( head, 1 ) * ( -( R + 6 ) - coreX * unit ) };
    Scene3D scene{ origin, head, unit, 0.1 * std::sin( anim * 0.4 ) };

    // guild amber, turning angry red as the hull fails:
    const Color_d amber{ 1, 0.3 + health * 0.4, 0.15 + health * 0.1 };
    const Color_d edge{ amber.r, amber.g, amber.b, 0.9 };
    const Color_d edgeSoft{ amber.r, amber.g, amber.b, 0.4 };
    const Color_d hullDark{ 0.1, 0.06, 0.05 };
    const Color_d plate{ 0.27, 0.16, 0.11 };
    const Color_d deckPlate{ 0.36, 0.22, 0.14 };
    const Color_d steel{ 0.3, 0.33, 0.38 };
    const Color_d lit{ 1, 0.85, 0.55 };
    const auto outlineOf{ []( const Color_d & _color ) { return Color_d{ std::min( _color.r * 1.5 + 0.1, 1.0 ), std::min( _color.g * 1.5 + 0.1, 1.0 ), std::min( _color.b * 1.5 + 0.1, 1.0 ), 0.85 }; } };

    scene.Add( { 0, 0, -3 }, [ & ]() {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, unit * 2.6, { amber.r, amber.g, amber.b, 0.16 }, { amber.r, amber.g, amber.b, 0 } );
    } );

    // the hull: keel, armored mid-hull, upper deck
    scene.Add( Mesh3D::Extrusion( { { 1.0, 0 }, { 0.8, 0.35 }, { 0.3, 0.5 }, { -0.7, 0.52 }, { -0.95, 0.28 }, { -0.95, -0.28 }, { -0.7, -0.52 }, { 0.3, -0.5 }, { 0.8, -0.35 } },
        -0.14, 0.04, hullDark, hullDark, edgeSoft, 1.5 ) );
    scene.Add( Mesh3D::Extrusion( { { 1.15, 0 }, { 0.85, 0.42 }, { 0.35, 0.58 }, { -0.7, 0.6 }, { -1.0, 0.3 }, { -1.0, -0.3 }, { -0.7, -0.6 }, { 0.35, -0.58 }, { 0.85, -0.42 } },
        0.04, 0.22, deckPlate, plate, edge, 3 ) );
    scene.Add( Mesh3D::Extrusion( { { 0.7, 0 }, { 0.55, 0.3 }, { 0.2, 0.4 }, { -0.55, 0.42 }, { -0.75, 0.2 }, { -0.75, -0.2 }, { -0.55, -0.42 }, { 0.2, -0.4 }, { 0.55, -0.3 } },
        0.22, 0.32, deckPlate, plate, edge, 2.5 ) );

    // rivet lights along the mid-hull edge, battle damage on it
    scene.Add( { 0, 0, 0.22 }, [ & ]() {
        for( int i{ 0 }; i < 7; i++ ) {
            const auto x{ 0.6 - i * 0.22 };
            _frame.FillCircle( scene.ProjectModel( { x, 0.51, 0.22 } ), 2.2, { lit.r, lit.g, lit.b, 0.6 } );
            _frame.FillCircle( scene.ProjectModel( { x, -0.51, 0.22 } ), 2.2, { lit.r, lit.g, lit.b, 0.6 } );
        }
        if( boss.phase >= 1 ) {
            const Color_d crack{ 1, 0.35, 0.1, 0.9 };
            const std::vector< std::vector< Vector3 > > cracks{ { { 0.4, 0.55, 0.22 }, { 0.2, 0.47, 0.22 }, { 0.02, 0.52, 0.22 }, { -0.1, 0.45, 0.22 } },
                { { -0.3, -0.56, 0.22 }, { -0.45, -0.48, 0.22 }, { -0.55, -0.54, 0.22 } } };
            for( const auto & line : cracks )
                for( size_t i{ 0 }; i + 1 < line.size(); i++ )
                    _frame.Line( scene.ProjectModel( line.at( i ) ), scene.ProjectModel( line.at( i + 1 ) ), crack, 2 );
        }
        if( boss.phase >= 2 ) {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            const auto flicker{ 0.5 + 0.5 * std::sin( anim * 7 ) };
            _frame.GradientCircle( scene.ProjectModel( { 0.25, 0.5, 0.22 } ), unit * 0.35, { 1, 0.4, 0.1, 0.35 * flicker }, { 1, 0.3, 0.1, 0 } );
            _frame.GradientCircle( scene.ProjectModel( { -0.45, -0.5, 0.22 } ), unit * 0.3, { 1, 0.4, 0.1, 0.35 * ( 1 - flicker ) }, { 1, 0.3, 0.1, 0 } );
        }
    } );

    // the cargo containers on the deck; the damage costs some, and sets the places on fire
    const std::array< Color_d, 6 > containerColors{ Color_d{ 0.62, 0.3, 0.12 }, Color_d{ 0.15, 0.4, 0.42 }, Color_d{ 0.7, 0.52, 0.15 },
        Color_d{ 0.35, 0.37, 0.4 }, Color_d{ 0.55, 0.22, 0.12 }, Color_d{ 0.2, 0.34, 0.5 } };
    const std::array< double, 3 > containerX{ -0.36, -0.06, 0.24 };
    for( int i{ 0 }; i < 6; i++ ) {
        const auto x{ containerX.at( static_cast< size_t >( i / 2 ) ) };
        const auto y{ i % 2 == 0 ? -0.22 : 0.22 };
        const bool lost{ ( i == 3 && boss.phase >= 1 ) || ( i == 0 && boss.phase >= 2 ) };
        if( lost ) {
            scene.Add( { x, y, 0.34 }, [ &, x, y, i ]() {
                const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                const auto flicker{ 0.5 + 0.5 * std::sin( anim * 9 + i * 2.1 ) };
                _frame.GradientCircle( scene.ProjectModel( { x, y, 0.4 } ), unit * ( 0.22 + 0.1 * flicker ), { 1, 0.55, 0.15, 0.6 }, { 1, 0.3, 0.1, 0 } );
            } );
            continue;
        }
        const auto & color{ containerColors.at( static_cast< size_t >( i ) ) };
        scene.Add( Mesh3D::Box( x - 0.13, x + 0.13, y - 0.085, y + 0.085, 0.32, 0.48 + 0.04 * ( i % 2 ), color, color, outlineOf( color ), 1.5 ) );
    }

    // the bridge tower at the stern, its windows lit, an antenna with a beacon
    scene.Add( Mesh3D::Box( -0.74, -0.5, -0.16, 0.16, 0.32, 0.6, plate, steel, edge, 2 ) );
    scene.Add( Mesh3D::Box( -0.7, -0.54, -0.11, 0.11, 0.6, 0.72, steel, plate, edge, 2 ) );
    scene.Add( Mesh3D::Box( -0.502, -0.5, -0.13, 0.13, 0.4, 0.55, lit, lit, { 0, 0, 0, 0 } ).Glowing( 1 ) );
    scene.Add( Mesh3D::Box( -0.542, -0.54, -0.09, 0.09, 0.63, 0.69, lit, lit, { 0, 0, 0, 0 } ).Glowing( 1 ) );
    scene.Add( Mesh3D::Beam( { -0.62, 0, 0.72 }, { -0.62, 0, 1.05 }, 0.012, steel, edgeSoft, 1 ) );
    scene.Add( { -0.62, 0, 1.05 }, [ & ]() {
        if( std::fmod( anim, 2 ) < 1 ) {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            _frame.GradientCircle( scene.ProjectModel( { -0.62, 0, 1.05 } ), 12, { 1, 0.2, 0.15, 0.9 }, { 1, 0.1, 0.1, 0 } );
        }
    } );

    // the crane: a post, a boom swinging over the deck with a counterweight, a hanging hook
    const auto swing{ 0.9 * std::sin( anim * 0.45 ) + 0.5 };
    const Vector3 boomEnd{ 0.38 + 0.6 * std::cos( swing ), 0.6 * std::sin( swing ), 0.68 };
    scene.Add( Mesh3D::Box( 0.35, 0.41, -0.03, 0.03, 0.32, 0.66, steel, plate, edgeSoft, 1.5 ) );
    scene.Add( Mesh3D::Beam( { 0.38, 0, 0.68 }, boomEnd, 0.03, amber, edge, 1.5 ) );
    scene.Add( Mesh3D::Beam( { 0.38, 0, 0.68 }, { 0.38 - 0.2 * std::cos( swing ), -0.2 * std::sin( swing ), 0.68 }, 0.06, hullDark, edgeSoft, 1.5 ) );
    scene.Add( boomEnd, [ &, boomEnd ]() {
        _frame.Line( scene.ProjectModel( boomEnd ), scene.ProjectModel( { boomEnd.x, boomEnd.y, 0.5 } ), steel, 1.5 );
    } );
    scene.Add( Mesh3D::Box( boomEnd.x - 0.04, boomEnd.x + 0.04, boomEnd.y - 0.04, boomEnd.y + 0.04, 0.44, 0.5, amber, plate, edge, 1.5 ) );

    // the engine nacelles on struts: hazard bands, a bell nozzle, a flickering exhaust
    for( const auto side : { -1.0, 1.0 } ) {
        const auto y{ side * 0.88 };
        for( const auto x : { 0.25, -0.45 } )
            scene.Add( Mesh3D::Beam( { x, side * 0.56, 0.1 }, { x, side * 0.8, 0.08 }, 0.05, steel, edgeSoft, 1.5 ) );
        scene.Add( Mesh3D::Lathe( { { 0.5, 0.1 }, { 0.38, 0.17 }, { 0.12, 0.17 }, { -0.14, 0.17 }, { -0.4, 0.17 }, { -0.66, 0.17 }, { -0.92, 0.17 } }, 8, y, 0.08,
            [ & ]( const int _band, const int ) { return _band == 0 ? steel : ( _band % 2 == 1 ? Color_d{ 0.85, 0.48, 0.12 } : Color_d{ 0.14, 0.09, 0.07 } ); }, edgeSoft ) );
        scene.Add( Mesh3D::Lathe( { { -0.92, 0.17 }, { -1.08, 0.25 } }, 8, y, 0.08, [ & ]( const int, const int ) { return steel; }, edge ) );
        scene.Add( { -1.1, y, 0.08 }, [ &, y, side ]() {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            const auto flicker{ 0.5 + 0.5 * std::sin( anim * 9 + side ) };
            const auto nozzle{ scene.ProjectModel( { -1.1, y, 0.08 } ) };
            const auto tip{ scene.ProjectModel( { -1.1 - 0.35 - 0.12 * flicker, y, 0.08 } ) };
            const auto direction{ tip - nozzle };
            const auto length{ std::max( direction.Distance(), 1.0 ) };
            const Vector perpendicular{ -direction.v / length * unit * 0.17, direction.u / length * unit * 0.17 };
            _frame.FillPolygon( { nozzle + perpendicular, tip, nozzle - perpendicular }, { 1, 0.5, 0.15, 0.55 } );
            _frame.FillPolygon( { nozzle + perpendicular * 0.5, nozzle + ( tip - nozzle ) * 0.6, nozzle - perpendicular * 0.5 }, { 1, 0.9, 0.5, 0.7 } );
            _frame.GradientCircle( nozzle, unit * ( 0.34 + 0.06 * flicker ), { 1, 0.6, 0.25, 0.55 }, { 1, 0.3, 0.1, 0 } );
        } );
        scene.Add( { 0.45, y, 0.27 }, [ &, y, side ]() { // blinking beacon
            if( std::fmod( anim, 2 ) < 1 )
                _frame.FillCircle( scene.ProjectModel( { 0.45, y, 0.27 } ), 4, side > 0 ? Color_d{ 1, 0.1, 0.1 } : Color_d{ 0.1, 1, 0.3 } );
        } );
    }

    // the head: a collar with hydraulic clamps, and the giant fluted drill turning in it
    scene.Add( Mesh3D::Lathe( { { 0.9, 0.46 }, { 1.0, 0.5 }, { 1.15, 0.46 } }, 12, 0, 0.12,
        [ & ]( const int, const int _segment ) { return _segment % 2 == 0 ? steel : plate; }, edge ) );
    for( const auto side : { -1.0, 1.0 } )
        scene.Add( Mesh3D::Beam( { 0.6, side * 0.3, 0.2 }, { 1.05, side * 0.42, 0.2 + 0.02 * std::sin( anim * 3 + side ) }, 0.04, steel, edge, 1.5 ) );
    scene.Add( Mesh3D::Lathe( { { 1.15, 0.44 }, { 1.4, 0.4 }, { 1.65, 0.32 }, { 1.85, 0.22 }, { 2.0, 0.12 }, { 2.12, 0.04 }, { 2.18, 0 } }, 10, 0, 0.12,
        []( const int, const int _segment ) { return _segment % 2 == 0 ? Color_d{ 0.95, 0.6, 0.2 } : Color_d{ 0.3, 0.17, 0.1 }; },
        { amber.r, amber.g, amber.b, 0.3 }, 0.55, 2.2, anim * 2.4 ) );
    scene.Add( { 2.18, 0, 0.12 }, [ & ]() { // the bit biting: a glow and a few sparks
        const auto tip{ scene.ProjectModel( { 2.18, 0, 0.12 } ) };
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( tip, 22, { 1, 0.9, 0.6, 0.7 }, { 1, 0.6, 0.2, 0 } );
        for( int i{ 0 }; i < 6; i++ ) {
            const auto angle{ anim * 9 + i * 1.7 };
            const auto length{ 8 + 12 * ( 0.5 + 0.5 * std::sin( anim * 13 + i * 2.3 ) ) };
            _frame.Line( tip, tip + Vector::From( angle, length ), { 1, 0.85, 0.5, 0.8 }, 2 );
        }
    } );

    // the stern struts to the core
    for( const auto side : { -1.0, 1.0 } )
        scene.Add( Mesh3D::Beam( { -0.92, side * 0.26, 0.12 }, { coreX * 0.95, side * 0.1, 0 }, 0.05, steel, edge, 1.5 ) );
    scene.Add( Mesh3D::Beam( { -0.98, 0, 0.1 }, { coreX * 0.9, 0, 0 }, 0.08, hullDark, edge, 1.5 ) );

    scene.AddLast( [ & ]() {
        // the rear core: armored behind six petals, glowing amber while the vent warms up, then
        // exposed (cyan, flashing white when hit) as the petals open
        const bool armored{ boss.vent == 0 };
        const bool warning{ boss.vent == 1 };
        const bool flashing{ boss.coreFlash > 0 };
        const auto target{ boss.vent == 2 ? 1.0 : ( warning ? 0.18 : 0.0 ) };
        m_foremanOpen += ( target - m_foremanOpen ) * 0.15;
        const auto open{ m_foremanOpen };
        const auto pulse{ ( std::sin( m_foremanAnim * ( warning ? 14 : 5 ) ) + 1 ) * 0.5 };
        const auto r{ Enemy::coreRadius };
        const Color_d coreColor{ flashing ? Color_d{ 1, 1, 1 } : ( armored ? Color_d{ 0.45, 0.5, 0.55 } : ( warning ? Color_d{ 1, 0.6, 0.15 } : Color_d{ 0.35, 0.9, 1 } ) ) };
        if( !armored ) {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            _frame.GradientCircle( core, r * ( flashing ? 4.5 : ( warning ? 2.5 + pulse * 1.5 : 3 + pulse ) ), { coreColor.r, coreColor.g, coreColor.b, flashing ? 0.7 : 0.4 }, { coreColor.r, coreColor.g, coreColor.b, 0 } );
        }
        _frame.FillCircle( core, r * 1.12, hullDark );
        _frame.StrokeCircle( core, r * 1.12, edge, 4 );
        _frame.FillCircle( core, r * 0.75, { 0.05, 0.12, 0.18 } );
        _frame.FillCircle( core, r * ( flashing ? 0.7 : 0.1 + open * ( 0.35 + pulse * 0.15 ) + ( warning ? 0.15 : 0 ) ), coreColor );
        if( open > 0.5 )
            for( int i{ 0 }; i < 3; i++ ) {
                const auto angle{ -m_foremanAnim * 1.5 + Maths::Pi2 * i / 3 };
                _frame.StrokeArc( core, r + 9, angle, angle + 0.9, { coreColor.r, coreColor.g, coreColor.b, 0.8 }, 2.5 );
            }
        // the armor petals, hinged on the rim: their tips come in to the middle when closed, and sweep outward when open
        const auto lightAngle{ std::atan2( -0.35, -0.5 ) };
        for( int k{ 0 }; k < 6; k++ ) {
            const auto angle{ Maths::Pi2 * k / 6 + 0.3 };
            const auto tremble{ warning ? std::sin( m_foremanAnim * 40 + k * 1.7 ) * 1.5 : 0.0 };
            const auto tip{ core + Vector::From( angle, r * ( 0.05 + open * 1.5 ) + tremble ) };
            const std::vector< Position_d > petal{ core + Vector::From( angle - 0.5, r ), tip, core + Vector::From( angle + 0.5, r ) };
            const auto shade{ 0.55 + 0.45 * std::cos( angle - lightAngle ) };
            _frame.FillPolygon( petal, { steel.r * shade * 1.3, steel.g * shade * 1.3, steel.b * shade * 1.3 } );
            _frame.StrokePolygon( petal, flashing ? Color_d{ 1, 1, 1, 0.9 } : ( warning ? Color_d{ 1, 0.6, 0.15, 0.6 + 0.4 * pulse } : edge ), 2 );
        }
        _frame.StrokeCircle( core, r, { coreColor.r, coreColor.g, coreColor.b, 0.9 }, armored ? 5 : 3 );
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

    // singularity: layered accretion disk, precessing polar jets, gravitational
    // lensing ring and a pure-black event horizon, all intensifying toward collapse:
    if( _world.SingularityActive() && _Visible( _world, _world.SingularityPosition(), 280 ) ) {
        const auto position{ _world.SingularityPosition() + translation };
        m_singularityAnim += 0.12;
        const auto progress{ _world.SingularityProgress() };
        const auto intensity{ 0.7 + progress * 0.5 };
        {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            // deep halo:
            _frame.GradientCircle( position, 180 * intensity,
                { 0.3, 0.12, 0.6, 0.3 + progress * 0.3 }, { 0.08, 0, 0.18, 0 } );
            // accretion disk: six layered arcs, faster and hotter inward:
            for( int k{ 0 }; k < 6; k++ ) {
                const auto layer{ k / 5.0 };
                const auto arcRadius{ ( 20 + k * 9 ) * intensity };
                const auto arcAngle{ m_singularityAnim * ( 2.4 - layer * 1.6 ) * ( k % 2 == 0 ? -1 : 1 ) + k * 1.3 };
                const Color_d arcColor{ 1 - layer * 0.4, 0.6 - layer * 0.35, 1, 0.75 - layer * 0.4 };
                _frame.StrokeArc( position, arcRadius, arcAngle, arcAngle + 1.4 + layer, arcColor, 2.8 - layer * 1.4 );
            }
            // polar jets, slowly precessing:
            const auto jetAngle{ m_singularityAnim * 0.18 };
            for( int side{ -1 }; side <= 1; side += 2 )
                for( int segment{ 0 }; segment < 3; segment++ ) {
                    const auto from{ position + Vector::From( jetAngle, side * ( 16.0 + segment * 24 ) * intensity ) };
                    const auto to{ position + Vector::From( jetAngle, side * ( 16.0 + ( segment + 1 ) * 24 ) * intensity ) };
                    _frame.Line( from, to, { 0.8, 0.6, 1, 0.55 - segment * 0.17 }, 4.0 - segment * 1.2 );
                }
            // gravitational lensing ring, shimmering:
            _frame.StrokeCircle( position, 15 * intensity, { 0.9, 0.7, 1, 0.45 + 0.25 * std::sin( m_singularityAnim * 4 ) }, 1.3 );
        }
        // event horizon (normal blending: it must stay BLACK), rim flickering:
        _frame.FillCircle( position, ( 11 + std::sin( m_singularityAnim * 5 ) * 0.8 ) * intensity, { 0.01, 0, 0.03 } );
        _frame.StrokeCircle( position, 12 * intensity, { 0.85, 0.55, 1, 0.75 + std::sin( m_singularityAnim * 3 ) * 0.2 }, 2 );
        // pre-collapse white flare:
        if( progress > 0.75 ) {
            const auto flare{ ( progress - 0.75 ) * 4 };
            _frame.FillCircle( position, flare * 9, { 1, 0.95, 1, flare * 0.85 } );
        }
    }

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
        // nozzle and flames extend beyond the bounding box, keep a comfortable margin:
        if( _Visible( _world, enemy.rocket.position, enemy.type == Enemy::eType::warden ? Enemy::pylonOrbit * 1.5 : ( enemy.type == Enemy::eType::foreman ? enemy.rocket.dynamic.bodyRadius * 3.8 : enemy.rocket.dynamic.boundingBoxRadius + Enemy::coreRadius * 4 ) ) ) {
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
