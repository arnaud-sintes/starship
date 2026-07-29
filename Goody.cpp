#include "Goody.h"


void Goody::Update()
{
    grow += 0.1;
    dynamic.radius = 14 + std::sin( grow ) * 2;
    dynamic.reflectAnimation += 0.3;
}


void Goody::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const
{
    struct TypeInfo
    {
        const char * letter;
        Vector adjust;
        enum class eColorScheme {
            red,
            green,
            blue,
            violet,
        };
        eColorScheme colorScheme;
    };
    static const std::unordered_map< eType, TypeInfo > infos{
        { eType::laserUp, { "L", { 5, 8 }, TypeInfo::eColorScheme::red } },
        { eType::homingMissiles, { "H", { 7, 8 }, TypeInfo::eColorScheme::green } },
        { eType::magneticMines, { "M", { 9, 8 }, TypeInfo::eColorScheme::green } },
        { eType::plasmaShield, { "S", { 5, 8 }, TypeInfo::eColorScheme::green } },
        { eType::shieldAdd, { "S", { 5, 8 }, TypeInfo::eColorScheme::blue } },
        { eType::propellantAdd, { "P", { 5, 8 }, TypeInfo::eColorScheme::blue } },
        { eType::turret, { "T", { 5, 8 }, TypeInfo::eColorScheme::green } },
        { eType::repulsor, { "R", { 6, 8 }, TypeInfo::eColorScheme::red } },
        { eType::decoy, { "D", { 6, 8 }, TypeInfo::eColorScheme::green } },
        { eType::emp, { "E", { 5, 8 }, TypeInfo::eColorScheme::green } },
        { eType::overdrive, { "O", { 7, 8 }, TypeInfo::eColorScheme::blue } },
        { eType::singularity, { "X", { 6, 8 }, TypeInfo::eColorScheme::violet } },
        { eType::blossom, { "B", { 6, 8 }, TypeInfo::eColorScheme::red } },
        { eType::hellstorm, { "W", { 8, 8 }, TypeInfo::eColorScheme::green } },
    };
    const auto & info{ infos.find( type )->second };

    const auto sinGrow{ std::sin( grow ) };
    Color_d color;
    switch( info.colorScheme ) {
        case TypeInfo::eColorScheme::red: color = Color_d{ 1, 0.5, ( sinGrow + 1 ) * 0.5 }; break;
        case TypeInfo::eColorScheme::green: color = Color_d{ 0.25, 1, ( sinGrow + 1 ) * 0.5 }; break;
        case TypeInfo::eColorScheme::blue: color = Color_d{ 0.25, ( sinGrow + 1 ) * 0.5, 1 }; break;
        case TypeInfo::eColorScheme::violet: color = Color_d{ 0.72, 0.3 + ( sinGrow + 1 ) * 0.2, 1 }; break;
    }
    // colored glow so the pod calls for attention:
    _frame.GradientCircle( position + _translation, dynamic.radius * 2.6,
        { color.r, color.g, color.b, 0.3 + ( sinGrow + 1 ) * 0.06 }, { color.r, color.g, color.b, 0 } );
    _frame.StrokeCircle( position + _translation, dynamic.radius, color, 4 );
    _frame.Text( position + _translation - info.adjust, "openSansBold", 20, info.letter, colorWhite );
    _frame.Reflect( position + _translation, dynamic.radius, color, -0.4, dynamic.reflectAnimation );
}
