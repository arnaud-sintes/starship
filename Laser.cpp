#include "Laser.h"


void Laser::Refresh()
{
    dynamic.positionA = position;
    dynamic.positionB = dynamic.positionA + momentum;
}


void Laser::Update()
{
    position += momentum;
    Refresh();
}


void Laser::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const
{
    const auto lifeRatio{ static_cast< double >( lifeSpan ) / static_cast< double >( maxLifeSpan ) };
    const auto color{ Color_d::FadeRadium( lifeRatio ) };
    // beam glow around the segment center:
    _frame.GradientCircle( ( dynamic.positionA + ( dynamic.positionB - dynamic.positionA ) * 0.5 ) + _translation, 26,
        { color.r, color.g, color.b, 0.3 }, { color.r, color.g, color.b, 0 } );
    _frame.Line( dynamic.positionA + _translation, dynamic.positionB + _translation, color, 3 );
}
