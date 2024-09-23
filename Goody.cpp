#include "Goody.h"


void Goody::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    grow += 0.1;
    const auto sinGrow{ std::sin( grow ) };
    dynamic.radius = 14 + sinGrow * 2;

    struct TypeInfo
    {
        std::string letter;
        Vector adjust;
    };
    static std::unordered_map< eType, TypeInfo > infos{
        { eType::laserUp, { "L", { 5, 8 } } },
    };
    const auto & info{ infos.find( type )->second };

    _frame.StrokeCircle( position + _translation, dynamic.radius, { 0.25, 1, ( sinGrow + 1 ) * 0.5 }, 4 );
    _frame.Text( position + _translation - info.adjust, "openSansBold", 20, info.letter, colorWhite );
}