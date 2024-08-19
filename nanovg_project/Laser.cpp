#include "Laser.h"


void Laser::Update()
{
    position += momentum;
}


void Laser::Draw( NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    dynamic.positionA = position;
    dynamic.positionB = dynamic.positionA + momentum;
    const auto lifeRatio{ static_cast< double >( lifeSpan ) / static_cast< double >( maxLifeSpan ) };
    _frame.Line( dynamic.positionA + _translation, dynamic.positionB + _translation, Color_d::FireColor( lifeRatio ), 3 );
}