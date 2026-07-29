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
    m_starField.Draw( _frame, _world.Ship().momentum * 2 );
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
        if( _Visible( _world, enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius + 60 ) )
            enemy.rocket.Draw( _frame, translation );
    }

    // draw lasers:
    for( const auto & laser : _world.Lasers() )
        if( _Visible( _world, laser.position, 55 ) )
            laser.Draw( _frame, translation );

    // draw missiles:
    for( const auto & missile : _world.Missiles() )
        if( _Visible( _world, missile.rocket.position, missile.rocket.dynamic.boundingBoxRadius + 60 ) )
            missile.rocket.Draw( _frame, translation );

    // draw attractors:
    _DrawAttractors( _world, _frame, translation );

    // blast shockwave rings, expanding fast then fading out:
    for( const auto & blast : _world.Blasts() ) {
        if( !_Visible( _world, blast.position, blast.radius ) )
            continue;
        const auto age{ static_cast< double >( blast.age ) / World::Blast::maxAge };
        const auto ringRadius{ blast.radius * ( 1 - ( 1 - age ) * ( 1 - age ) ) };
        const auto fade{ std::pow( 1 - age, 1.5 ) };
        _frame.StrokeCircle( blast.position + translation, ringRadius,
            { blast.ringColor.r, blast.ringColor.g, blast.ringColor.b, fade * 0.55 }, 1.5 + fade * 2 );
    }

    // plasma shield:
    const auto plasmaShieldSin{ std::sin( _world.PlasmaShieldRamp() * Maths::Pi ) };
    const auto plasmaShieldColor{ Color_d{ 0.5, 1, 0.75 } * plasmaShieldSin };
    if( _world.PlasmaShieldActive() )
        _frame.StrokeCircle( m_screenCenter, _world.PlasmaShieldRadius(), plasmaShieldColor, 2 * plasmaShieldSin );

    // draw ship:
    if( !_world.ShipDestroyed() )
        ship.Draw( _frame, translation );

    // turret bonus, a mini-ship orbiting the main one:
    if( !_world.ShipDestroyed() && _world.TurretActive() ) {
        const auto position{ _world.TurretPosition() + translation };
        const auto orientation{ _world.TurretOrientation() };
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
