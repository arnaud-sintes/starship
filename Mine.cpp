#include "Mine.h"

#include "SpikedBall.h"


void Mine::Update()
{
    grow += 0.1;
    dynamic.radius = 19 + std::sin( grow ) * 2;
}


void Mine::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const
{
    const auto translated{ position + _translation };
    const auto sinGrow{ std::sin( grow ) };
    const auto pulse{ ( sinGrow + 1 ) * 0.5 };
    const auto spin{ grow * 0.22 }; // slow rotation

    // same design language as the gravity mines - dark sphere plus glowing horns -
    // but in the friendly green/white scheme:
    // bright core glow so the mine pops against the void:
    _frame.GradientCircle( translated, dynamic.radius * 2.6,
        Color_d{ 0.25, 1, 0.5, 0.35 + pulse * 0.15 }, Color_d{ 0.05, 0.4, 0.2, 0 } );

    // the horns and body are a fake-3D ball tumbling on two axes (each mine with its own phase):
    const auto phase{ position.u * 0.017 + position.v * 0.031 };
    SpikedBall::Draw( _frame, translated, {
        .horns = 8,
        .bodyRadius = dynamic.radius - 7,
        .tipDistance = dynamic.radius - 1 + pulse * 2,
        .hornWidth = 3,
        .tipRadius = 2,
        .yaw = spin + phase,
        .pitch = 0.5 + grow * 0.14 + phase * 0.7,
        .body = Color_d{ 0.06, 0.24, 0.13 },
        .horn = Color_d{ 0.35, 1, 0.55 + sinGrow * 0.2 },
        .tip = Color_d{ 1, 1, 1, 0.8 + pulse * 0.2 },
        .highlight = Color_d{ 0.8, 1, 0.9, 0.4 } } );
}
