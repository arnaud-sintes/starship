#include "Laser.h"


void Laser::Update()
{
    position += momentum;
}


void Laser::Draw( cairo_t & _cairo, const Vector & _translation )
{
    dynamic.positionA = position;
    dynamic.positionB = dynamic.positionA + momentum;
    const auto lifeRatio{ static_cast< double >( lifeSpan ) / static_cast< double >( maxLifeSpan ) };
    Graphics::Line( _cairo, dynamic.positionA + _translation, dynamic.positionB + _translation, Graphics::FireColor( lifeRatio ), 3, false );
}