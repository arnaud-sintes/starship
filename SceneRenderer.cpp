#include "SceneRenderer.h"


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
    // the sentinel of the belt: an armillary of rotating rings around a dark eye, shielded
    // by a barrier as long as its three pylons (gravity wells, tethered to the core) stand.
    // Once they fall, the barrier drops and the eye burns open.
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

    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, R * 3, { accent.r, accent.g, accent.b, 0.18 }, { accent.r, accent.g, accent.b, 0 } );
    }

    // the pylons: contracting rings show the pull of their wells, tethers feed the core
    for( int i{ 0 }; i < Enemy::pylonCount; i++ ) {
        const auto position{ _enemy.PylonPosition( i ) + _translation };
        if( !_enemy.PylonAlive( i ) )
            continue;
        for( int ring{ 0 }; ring < 3; ring++ ) {
            const auto phase{ std::fmod( anim * 0.6 + ring / 3.0, 1.0 ) };
            _frame.StrokeCircle( position, 190 * ( 1 - phase ) + 30, { violet.r, violet.g, violet.b, 0.22 * phase }, 2 );
        }
        {
            const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
            _frame.Line( center, position, { violet.r, violet.g, violet.b, 0.35 }, 7 );
        }
        _frame.Line( center, position, { ice.r, ice.g, ice.b, 0.7 }, 2 );
        for( int bead{ 0 }; bead < 3; bead++ ) { // energy flowing to the core
            const auto t{ std::fmod( anim * 0.7 + bead / 3.0 + i * 0.3, 1.0 ) };
            _frame.FillCircle( position + ( center - position ) * t, 3.5, { 1, 1, 1, 0.9 } );
        }
    }

    // the body: fins, a dark disc, counter-rotating arcs
    for( int i{ 0 }; i < 8; i++ ) {
        const auto angle{ anim * 0.5 + Maths::Pi2 * i / 8 };
        const std::vector< Position_d > fin{ center + Vector::From( angle - 0.14, R * 0.95 ), center + Vector::From( angle, R * 1.4 ), center + Vector::From( angle + 0.14, R * 0.95 ) };
        _frame.FillPolygon( fin, { 0.14, 0.1, 0.22 } );
        _frame.StrokePolygon( fin, { violet.r, violet.g, violet.b, 0.8 }, 2.5 );
    }
    _frame.FillCircle( center, R, { 0.07, 0.05, 0.12 } );
    _frame.StrokeCircle( center, R, violet, 7 );
    _frame.StrokeCircle( center, R * 0.7, { ice.r, ice.g, ice.b, 0.3 }, 2 );
    for( int i{ 0 }; i < 3; i++ ) {
        const auto angle{ -anim * 1.2 + Maths::Pi2 * i / 3 };
        _frame.StrokeArc( center, R * 1.12, angle, angle + 1.1, { ice.r, ice.g, ice.b, 0.85 }, 4 );
    }
    for( int i{ 0 }; i < 4; i++ ) {
        const auto angle{ anim * 1.8 + Maths::Pi2 * i / 4 };
        _frame.StrokeArc( center, R * 0.82, angle, angle + 0.7, { violet.r, violet.g, violet.b, 0.9 }, 3 );
    }

    // the eye: dim and shuttered under the barrier, burning once exposed
    const auto pulse{ ( std::sin( anim * ( exposed ? 9 : 3 ) ) + 1 ) * 0.5 };
    const Color_d eye{ hit && exposed ? Color_d{ 1, 1, 1 } : ( exposed ? accent : ice ) };
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, R * ( exposed ? 1.8 + pulse * 0.5 : 0.9 ), { eye.r, eye.g, eye.b, exposed ? 0.55 : 0.25 }, { eye.r, eye.g, eye.b, 0 } );
    }
    _frame.FillCircle( center, R * ( exposed ? 0.42 + pulse * 0.1 : 0.28 ), eye );
    _frame.FillCircle( center, R * 0.12, { 0.05, 0.03, 0.08 } );
    if( exposed )
        for( int i{ 0 }; i < 10; i++ ) { // crown of flames
            const auto angle{ anim * 2 + Maths::Pi2 * i / 10 };
            _frame.Line( center + Vector::From( angle, R * 0.5 ), center + Vector::From( angle, R * ( 0.75 + 0.2 * std::sin( anim * 11 + i ) ) ), { accent.r, accent.g, accent.b, 0.9 }, 3 );
        }

    // the barrier, up while a pylon stands; flashing white when shot
    if( !exposed ) {
        std::vector< Position_d > shape;
        for( int i{ 0 }; i < 6; i++ )
            shape.emplace_back( center + Vector::From( -anim * 0.25 + Maths::Pi2 * i / 6, R * 1.65 ) );
        _frame.FillPolygon( shape, { violet.r, violet.g, violet.b, hit ? 0.28 : 0.07 } );
        _frame.StrokePolygon( shape, hit ? Color_d{ 1, 1, 1, 0.95 } : Color_d{ violet.r, violet.g, violet.b, 0.45 }, hit ? 5 : 2 );
    }

    // the pylons themselves: crystal obelisks, wearing a ring of remaining hull
    for( int i{ 0 }; i < Enemy::pylonCount; i++ ) {
        const auto position{ _enemy.PylonPosition( i ) + _translation };
        const auto angle{ boss.pylonAngle + Maths::Pi2 * i / Enemy::pylonCount };
        const auto index{ static_cast< size_t >( i ) };
        if( !_enemy.PylonAlive( i ) ) { // a smoldering stump
            const auto flicker{ 0.5 + 0.5 * std::sin( anim * 8 + i * 2 ) };
            _frame.FillCircle( position, 9, { 0.12, 0.08, 0.15 } );
            _frame.FillCircle( position, 4, { 1, 0.4, 0.2, 0.4 + 0.4 * flicker } );
            continue;
        }
        const auto ratio{ std::clamp( boss.pylonShield.at( index ) / boss.pylonMax, 0.0, 1.0 ) };
        const bool flash{ boss.pylonFlash.at( index ) > 0 };
        const Color_d body{ flash ? Color_d{ 1, 1, 1 } : ice };
        const std::vector< Position_d > crystal{ position + Vector::From( angle, 44 ), position + Vector::From( angle + Maths::Pi * 0.5, 20 ),
            position + Vector::From( angle + Maths::Pi, 44 ), position + Vector::From( angle - Maths::Pi * 0.5, 20 ) };
        _frame.FillPolygon( crystal, { 0.1, 0.1, 0.2 } );
        _frame.StrokePolygon( crystal, body, 4 );
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( position, 36 + 6 * std::sin( anim * 5 + i ), { body.r, body.g, body.b, flash ? 0.8 : 0.4 }, { body.r, body.g, body.b, 0 } );
        _frame.StrokeArc( position, 42, -Maths::Pi * 0.5, -Maths::Pi * 0.5 + Maths::Pi2 * ratio, { 1, 0.75 + 0.2 * ratio, 1, 0.8 }, 3 );
    }
}


void SceneRenderer::_DrawForeman( const Enemy & _enemy, const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    // a guild mining barge, seen from above: an armored wedge hull with a drilling drum
    // amidships, two engine nacelles, a head drill-cannon, and a pylon holding the
    // glowing rear core (the weak spot). Damage shows: cracks, embers, a hotter hull.
    const auto & rocket{ _enemy.rocket };
    const auto & boss{ _enemy.boss };
    const auto center{ rocket.position + _translation };
    const auto R{ rocket.dynamic.bodyRadius };
    const auto health{ std::clamp( rocket.shield.value / boss.maxShield, 0.0, 1.0 ) };
    const auto head{ rocket.orientation + Maths::Pi }; // the head faces the opposite way of the stern
    m_foremanAnim += 0.03 + ( 1 - health ) * 0.03;
    const auto anim{ m_foremanAnim };

    // hull space: f along the barge (+ toward the head), s across it, in body radii
    const auto at{ [ & ]( const double _f, const double _s ) -> Vector {
            return center + Vector::From( head, _f * R ) + Vector::From( head + Maths::Pi * 0.5, _s * R );
        } };
    const auto poly{ [ & ]( std::initializer_list< std::pair< double, double > > _points ) {
            std::vector< Position_d > points;
            for( const auto & [ f, s ] : _points )
                points.emplace_back( at( f, s ) );
            return points;
        } };

    // guild amber, turning angry red as the hull fails:
    const Color_d amber{ 1, 0.3 + health * 0.4, 0.15 + health * 0.1 };
    const Color_d dark{ 0.09, 0.055, 0.045 };
    const Color_d plate{ 0.2, 0.12, 0.09 };
    const Color_d lit{ 1, 0.85, 0.55 };
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, R * 2.4, { amber.r, amber.g, amber.b, 0.16 }, { amber.r, amber.g, amber.b, 0 } );
    }

    // engine nacelles on each flank, hazard-striped, with a glowing exhaust:
    for( const auto side : { -1.0, 1.0 } ) {
        const auto nacelle{ poly( { { 0.55, side * 0.62 }, { 0.35, side * 1.0 }, { -0.85, side * 1.0 }, { -1.05, side * 0.78 }, { -0.8, side * 0.55 }, { 0.3, side * 0.55 } } ) };
        _frame.FillPolygon( nacelle, plate );
        _frame.StrokePolygon( nacelle, amber, 3 );
        for( int i{ 0 }; i < 5; i++ ) { // hazard stripes
            const auto f{ 0.2 - i * 0.22 };
            _frame.Line( at( f, side * 0.62 ), at( f - 0.12, side * 0.96 ), { amber.r, amber.g, amber.b, 0.75 }, 5 );
        }
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( at( -1.05, side * 0.78 ), R * ( 0.32 + 0.05 * std::sin( anim * 9 + side ) ), { 1, 0.6, 0.25, 0.55 }, { 1, 0.3, 0.1, 0 } );
    }

    // main wedge hull:
    const auto hullShape{ poly( { { 1.2, 0 }, { 0.85, 0.42 }, { 0.35, 0.58 }, { -0.7, 0.6 }, { -1.0, 0.3 }, { -1.0, -0.3 }, { -0.7, -0.6 }, { 0.35, -0.58 }, { 0.85, -0.42 } } ) };
    _frame.FillPolygon( hullShape, dark );
    _frame.StrokePolygon( hullShape, amber, 6 );
    _frame.StrokePolygon( poly( { { 0.95, 0 }, { 0.7, 0.3 }, { 0.3, 0.42 }, { -0.6, 0.44 }, { -0.8, 0.22 }, { -0.8, -0.22 }, { -0.6, -0.44 }, { 0.3, -0.42 }, { 0.7, -0.3 } } ), { amber.r, amber.g, amber.b, 0.35 }, 2 );

    // rivets along the hull edge:
    for( int i{ 0 }; i < 7; i++ ) {
        const auto f{ 0.6 - i * 0.22 };
        _frame.FillCircle( at( f, 0.5 ), 2.5, { lit.r, lit.g, lit.b, 0.6 } );
        _frame.FillCircle( at( f, -0.5 ), 2.5, { lit.r, lit.g, lit.b, 0.6 } );
    }

    // the drilling drum amidships: counter-turning teeth around a dark bore:
    const auto drum{ at( 0.05, 0 ) };
    _frame.FillCircle( drum, R * 0.42, { 0.03, 0.02, 0.02 } );
    _frame.StrokeCircle( drum, R * 0.42, amber, 4 );
    for( int i{ 0 }; i < 12; i++ ) {
        const auto angle{ anim + Maths::Pi2 * i / 12 };
        _frame.Line( drum + Vector::From( angle, R * 0.22 ), drum + Vector::From( angle + 0.25, R * 0.4 ), { amber.r, amber.g, amber.b, 0.9 }, 3 );
    }
    for( int i{ 0 }; i < 6; i++ ) {
        const auto angle{ -anim * 1.6 + Maths::Pi2 * i / 6 };
        _frame.FillCircle( drum + Vector::From( angle, R * 0.14 ), 3, lit );
    }

    // the head: a drill-cannon barrel with a glowing muzzle and flanking pods:
    for( const auto side : { -1.0, 1.0 } ) {
        _frame.FillCircle( at( 0.95, side * 0.3 ), 9, amber );
        _frame.FillCircle( at( 0.95, side * 0.3 ), 4, dark );
    }
    _frame.Line( at( 0.9, 0 ), at( 1.55, 0 ), amber, 11 );
    _frame.Line( at( 0.9, 0 ), at( 1.55, 0 ), dark, 5 );
    for( int i{ 0 }; i < 3; i++ ) // barrel rings
        _frame.Line( at( 1.05 + i * 0.14, -0.1 ), at( 1.05 + i * 0.14, 0.1 ), lit, 3 );
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( at( 1.58, 0 ), 20, { 1, 0.9, 0.6, 0.7 }, { 1, 0.6, 0.2, 0 } );
    }

    // battle damage: cracks and ember glow grow with the phases
    if( boss.phase >= 1 ) {
        const Color_d crack{ 1, 0.35, 0.1, 0.9 };
        _frame.Line( at( 0.4, 0.45 ), at( 0.15, 0.3 ), crack, 2 );
        _frame.Line( at( 0.15, 0.3 ), at( 0.2, 0.12 ), crack, 2 );
        _frame.Line( at( -0.3, -0.5 ), at( -0.45, -0.3 ), crack, 2 );
        _frame.Line( at( -0.45, -0.3 ), at( -0.4, -0.12 ), crack, 2 );
    }
    if( boss.phase >= 2 ) {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        const auto flicker{ 0.5 + 0.5 * std::sin( anim * 7 ) };
        _frame.GradientCircle( at( 0.2, 0.25 ), R * 0.35, { 1, 0.4, 0.1, 0.35 * flicker }, { 1, 0.3, 0.1, 0 } );
        _frame.GradientCircle( at( -0.45, -0.3 ), R * 0.3, { 1, 0.4, 0.1, 0.35 * ( 1 - flicker ) }, { 1, 0.3, 0.1, 0 } );
    }

    // blinking beacons on the nacelles:
    if( std::fmod( anim, 2 ) < 1 ) {
        _frame.FillCircle( at( 0.35, 1.0 ), 4, { 1, 0.1, 0.1 } );
        _frame.FillCircle( at( 0.35, -1.0 ), 4, { 0.1, 1, 0.3 } );
    }

    // the stern pylon holding the core, braced to the hull corners:
    const auto core{ _enemy.CorePosition() + _translation };
    for( const auto side : { -1.0, 1.0 } )
        _frame.Line( at( -0.9, side * 0.28 ), core, { amber.r, amber.g, amber.b, 0.8 }, 4 );
    _frame.Line( at( -0.95, 0 ), core, amber, 8 );

    // the rear core: armored behind shutters, glowing amber while the vent warms up, then
    // exposed (cyan, flashing white when hit):
    const bool armored{ boss.vent == 0 };
    const bool warning{ boss.vent == 1 };
    const bool flashing{ boss.coreFlash > 0 };
    const auto pulse{ ( std::sin( m_foremanAnim * ( warning ? 14 : 5 ) ) + 1 ) * 0.5 };
    const Color_d coreColor{ flashing ? Color_d{ 1, 1, 1 } : ( armored ? Color_d{ 0.45, 0.5, 0.55 } : ( warning ? Color_d{ 1, 0.6, 0.15 } : Color_d{ 0.35, 0.9, 1 } ) ) };
    if( !armored ) {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( core, Enemy::coreRadius * ( flashing ? 4.5 : ( warning ? 2.5 + pulse * 1.5 : 3 + pulse ) ), { coreColor.r, coreColor.g, coreColor.b, flashing ? 0.7 : 0.4 }, { coreColor.r, coreColor.g, coreColor.b, 0 } );
    }
    _frame.FillCircle( core, Enemy::coreRadius * 0.75, { 0.05, 0.12, 0.18 } );
    if( armored ) {
        // closed shutters: two steel plates over a faint ember
        _frame.FillCircle( core, Enemy::coreRadius * 0.4, { 0.35, 0.18, 0.1 } );
        _frame.StrokeCircle( core, Enemy::coreRadius * 0.55, coreColor, Enemy::coreRadius * 0.5 );
        _frame.StrokeCircle( core, Enemy::coreRadius, { coreColor.r, coreColor.g, coreColor.b, 0.9 }, 5 );
    }
    else {
        _frame.FillCircle( core, Enemy::coreRadius * ( flashing ? 0.7 : 0.45 + pulse * 0.15 ), coreColor );
        _frame.StrokeCircle( core, Enemy::coreRadius, { coreColor.r, coreColor.g, coreColor.b, 0.9 }, 3 );
        for( int i{ 0 }; i < 3; i++ ) {
            const auto angle{ -m_foremanAnim * 1.5 + Maths::Pi2 * i / 3 };
            _frame.StrokeArc( core, Enemy::coreRadius + 9, angle, angle + 0.9, { coreColor.r, coreColor.g, coreColor.b, 0.8 }, 2.5 );
        }
    }
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
        if( _Visible( _world, enemy.rocket.position, enemy.type == Enemy::eType::warden ? Enemy::pylonOrbit * 1.5 : enemy.rocket.dynamic.boundingBoxRadius + Enemy::coreRadius * 4 ) ) {
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
