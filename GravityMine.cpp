#include "GravityMine.h"

#include "SpikedBall.h"


void GravityMine::Update()
{
    grow += 0.08;
    spin += 0.025;
    dynamic.radius = 13 + std::sin( grow ) * 1.5;
}


Vector GravityMine::Attraction( const Vector & _attracted ) const
{
    const auto direction{ position - _attracted };
    const auto distanceSquared{ direction.DistanceSquared() };
    if( distanceSquared >= pullRange * pullRange || distanceSquared < 1 )
        return {};
    const auto falloff{ 1.0 - distanceSquared / ( pullRange * pullRange ) };
    return direction * ( pullStrength * falloff / std::sqrt( distanceSquared ) );
}


void GravityMine::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const
{
    const auto translated{ position + _translation };
    const auto pulse{ ( std::sin( grow ) + 1 ) * 0.5 };

    // pull field halo, purple-red:
    _frame.GradientCircle( translated, pullRange * 0.6,
        Color_d{ 0.2, 0.02, 0.12, 0.3 + pulse * 0.15 }, Color_d{ 0.08, 0, 0.06, 0 } );

    // bright core glow so the mine pops against the void:
    _frame.GradientCircle( translated, dynamic.radius * 3.2,
        Color_d{ 1, 0.15, 0.3, 0.4 + pulse * 0.2 }, Color_d{ 0.5, 0.05, 0.15, 0 } );

    // naval-mine body: a dark sphere tumbling in fake 3D, nine glowing horns (odd count avoids any crosshair symmetry)
    // and an off-center molten ember orbiting on its surface:
    const auto bodyRadius{ dynamic.radius - 3 };
    SpikedBall::Draw( _frame, translated, {
        .horns = 9,
        .bodyRadius = bodyRadius,
        .tipDistance = bodyRadius + 6 + pulse * 2,
        .hornWidth = 3.5,
        .tipRadius = 2.2,
        .yaw = spin,
        .pitch = 0.5 + spin * 0.63,
        .body = Color_d{ 0.28, 0.05, 0.14 },
        .horn = Color_d{ 1, 0.3, 0.42 },
        .tip = Color_d{ 1, 0.55, 0.5, 0.8 + pulse * 0.2 },
        .highlight = Color_d{ 1, 0.5, 0.6, 0.4 },
        .ember = Color_d{ 1, 0.5, 0.35, 0.85 },
        .emberAngle = -spin * 0.7,
        .emberRadius = 2.5 + pulse } );
}
