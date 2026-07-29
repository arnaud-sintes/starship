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
    _frame.Line( dynamic.positionA + _translation, dynamic.positionB + _translation, Color_d::FadeRadium( lifeRatio ), 3 );
}
