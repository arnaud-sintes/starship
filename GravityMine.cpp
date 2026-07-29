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

    // pull field halo:
    _frame.GradientCircle( translated, pullRange * 0.6,
        Color_d{ 0.22, 0.03, 0.08, 0.3 + pulse * 0.15 }, Color_d{ 0.08, 0, 0.04, 0 } );

    // rotating spikes:
    constexpr int spikes{ 8 };
    for( int i{ 0 }; i < spikes; i++ ) {
        const auto angle{ spin + Maths::Pi2 * i / spikes };
        _frame.Line( translated + Vector::From( angle, dynamic.radius + 2 ),
            translated + Vector::From( angle, dynamic.radius + 8 + pulse * 3 ),
            Color_d{ 1, 0.3, 0.35, 0.8 }, 2 );
    }

    // dark core with a pulsing red ring and a hot center:
    _frame.FillCircle( translated, dynamic.radius * 0.6, Color_d{ 0.12, 0.02, 0.06 } );
    _frame.StrokeCircle( translated, dynamic.radius, Color_d{ 1, 0.25 + pulse * 0.2, 0.35 }, 3 );
    _frame.FillCircle( translated, 3, Color_d{ 1, 0.4, 0.45, 0.7 + pulse * 0.3 } );
}
