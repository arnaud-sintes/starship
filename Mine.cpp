#include "Mine.h"


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
    const auto spin{ grow * 0.2 }; // slow rotation

    // same design language as the gravity mines - dark sphere plus glowing horns -
    // but in the friendly green/white scheme:
    // bright core glow so the mine pops against the void:
    _frame.GradientCircle( translated, dynamic.radius * 2.6,
        Color_d{ 0.25, 1, 0.5, 0.35 + pulse * 0.15 }, Color_d{ 0.05, 0.4, 0.2, 0 } );

    const auto bodyRadius{ dynamic.radius - 7 };
    _frame.FillCircle( translated, bodyRadius, Color_d{ 0.06, 0.24, 0.13 } );

    constexpr int horns{ 6 };
    for( int i{ 0 }; i < horns; i++ ) {
        const auto angle{ spin + Maths::Pi2 * i / horns };
        const auto tipDistance{ dynamic.radius - 1 + pulse * 2 };
        _frame.Line( translated + Vector::From( angle, bodyRadius - 2 ),
            translated + Vector::From( angle, tipDistance ),
            Color_d{ 0.35, 1, 0.55 + sinGrow * 0.2 }, 3 );
        _frame.FillCircle( translated + Vector::From( angle, tipDistance ), 2,
            Color_d{ 1, 1, 1, 0.8 + pulse * 0.2 } );
    }

    // partial white rim light for volume:
    _frame.StrokeArc( translated, bodyRadius - 1.5, -spin * 0.4, -spin * 0.4 + 1.9, Color_d{ 1, 1, 1, 0.55 }, 1.5 );
}
