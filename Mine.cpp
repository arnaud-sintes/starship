#include "Mine.h"


void Mine::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    grow += 0.1;
    const auto sinGrow{ std::sin( grow ) };
    dynamic.radius = 19 + sinGrow * 2;

    _frame.StrokeCircle( position + _translation, dynamic.radius, Color_d{ 0.25, 1, 0.5 + sinGrow * 0.25, 0.5 }, 8 );
    _frame.StrokeCircle( position + _translation, dynamic.radius - 5, Color_d{ 1, 1, 1 }, 2 );
}