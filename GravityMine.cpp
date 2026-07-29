#include "GravityMine.h"


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

    // naval-mine body: a dark sphere - no rings, no bullseye:
    const auto bodyRadius{ dynamic.radius - 3 };
    _frame.FillCircle( translated, bodyRadius, Color_d{ 0.28, 0.05, 0.14 } );

    // seven horns slowly rotating, tips glowing (odd count avoids any crosshair symmetry):
    constexpr int horns{ 7 };
    for( int i{ 0 }; i < horns; i++ ) {
        const auto angle{ spin + Maths::Pi2 * i / horns };
        const auto tipDistance{ bodyRadius + 6 + pulse * 2 };
        _frame.Line( translated + Vector::From( angle, bodyRadius - 2 ),
            translated + Vector::From( angle, tipDistance ),
            Color_d{ 1, 0.3, 0.42 }, 3.5 );
        _frame.FillCircle( translated + Vector::From( angle, tipDistance ), 2.2,
            Color_d{ 1, 0.55, 0.5, 0.8 + pulse * 0.2 } );
    }

    // partial rim light for volume, and an off-center molten ember slowly orbiting:
    _frame.StrokeArc( translated, bodyRadius - 1.5, -spin * 0.3, -spin * 0.3 + 1.9, Color_d{ 1, 0.45, 0.6, 0.55 }, 2 );
    _frame.FillCircle( translated + Vector::From( -spin * 0.7, bodyRadius * 0.35 ), 2.5 + pulse,
        Color_d{ 1, 0.5, 0.35, 0.85 } );
}
