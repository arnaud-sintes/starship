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
        enum class eColorScheme {
            red,
            green,
            blue,
        };
        eColorScheme colorScheme;
    };
    static std::unordered_map< eType, TypeInfo > infos{
        { eType::laserUp, { "L", { 5, 8 }, TypeInfo::eColorScheme::red } },
        { eType::homingMissiles, { "H", { 7, 8 }, TypeInfo::eColorScheme::green } },
        { eType::magneticMines, { "M", { 9, 8 }, TypeInfo::eColorScheme::green } },
        { eType::plasmaShield, { "S", { 5, 8 }, TypeInfo::eColorScheme::green } },
        { eType::shieldAdd, { "S", { 5, 8 }, TypeInfo::eColorScheme::blue } },
        { eType::propellantAdd, { "P", { 5, 8 }, TypeInfo::eColorScheme::blue } },
    };
    const auto & info{ infos.find( type )->second };

    Color_d color;
    switch( info.colorScheme ) {
        case TypeInfo::eColorScheme::red: color = Color_d{ 1, 0.5, ( sinGrow + 1 ) * 0.5 }; break;
        case TypeInfo::eColorScheme::green: color = Color_d{ 0.25, 1, ( sinGrow + 1 ) * 0.5 }; break;
        case TypeInfo::eColorScheme::blue: color = Color_d{ 0.25, ( sinGrow + 1 ) * 0.5, 1 }; break;
    }
    _frame.StrokeCircle( position + _translation, dynamic.radius, color, 4 );
    _frame.Text( position + _translation - info.adjust, "openSansBold", 20, info.letter, colorWhite );
    _frame.Reflect( position + _translation, dynamic.radius, color, -0.4, dynamic.reflectAnimation += 0.3 );
}