#include "Mine.h"


void Mine::Update()
{
    grow += 0.1;
    dynamic.radius = 19 + std::sin( grow ) * 2;
}


void Mine::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const
{
    const auto sinGrow{ std::sin( grow ) };
    _frame.StrokeCircle( position + _translation, dynamic.radius, Color_d{ 0.25, 1, 0.5 + sinGrow * 0.25, 0.5 }, 8 );
    _frame.StrokeCircle( position + _translation, dynamic.radius - 5, Color_d{ 1, 1, 1 }, 2 );
}
