#include "Hud.h"


namespace
{
    // shared palette:
    constexpr Color_d accentColor{ 0.35, 0.65, 1 };
    constexpr Color_d labelColor{ 0.62, 0.72, 0.88, 0.9 };
    constexpr Color_d valueColor{ 0.85, 0.92, 1 };
    constexpr Color_d panelTopColor{ 0.07, 0.12, 0.2, 0.6 };
    constexpr Color_d panelBottomColor{ 0.02, 0.045, 0.09, 0.6 };
    constexpr Color_d panelBorderColor{ 0.35, 0.6, 1, 0.28 };
    constexpr Color_d trackColor{ 1, 1, 1, 0.09 };
    constexpr Color_d shieldColor{ 0.35, 0.8, 1 };
    constexpr Color_d propellantColor{ 0.68, 0.52, 1 };
    constexpr Color_d laserColor{ 1, 0.45, 0.55 };
    constexpr Color_d warningColor{ 1, 0.72, 0.25 };
    constexpr Color_d criticalColor{ 1, 0.28, 0.32 };
    constexpr Color_d bonusColor{ 0.4, 1, 0.6 };

    // layout:
    constexpr double margin{ 14 };
    constexpr double panelWidth{ 322 };
    constexpr double panelRadius{ 8 };
    constexpr double rowHeight{ 26 };
    constexpr double panelPadding{ 12 };
    constexpr double labelWidth{ 104 };
    constexpr double barWidth{ 138 };
    constexpr double barHeight{ 8 };
}


Hud::Hud( const Dimension_ui & _screenDimension )
    : m_screenDimension{ _screenDimension }
{}


Color_d Hud::_StateColor( const double _rate, const Color_d & _healthy ) const
{
    if( _rate > 0.5 )
        return _healthy;
    if( _rate > 0.25 )
        return warningColor;
    // critical: blink by modulating alpha
    return { criticalColor.r, criticalColor.g, criticalColor.b, 0.45 + ( std::sin( m_pulse ) + 1 ) * 0.275 };
}


void Hud::_Bar( const NanoVGRenderer::Frame & _frame, const Vector & _position, const double _width, const double _height, const double _rate, const Color_d & _color ) const
{
    const double radius{ _height * 0.5 };
    _frame.FillRectangle( _position, _position + Vector{ _width, _height }, trackColor, radius );
    const auto rate{ std::clamp( _rate, 0.0, 1.0 ) };
    if( rate <= 0 )
        return;
    const Color_d brighter{ std::min( _color.r * 1.25, 1.0 ), std::min( _color.g * 1.25, 1.0 ), std::min( _color.b * 1.25, 1.0 ), _color.a };
    _frame.GradientRectangle( _position, _position + Vector{ _width * rate, _height }, brighter, _color, radius );
}


void Hud::Draw( const World & _world, const NanoVGRenderer::Frame & _frame )
{
    m_pulse += 0.16;

    const auto info{ _world.GetHudInfo() };

    // status panel:
    const Vector panelPosition{ margin, margin };
    const double panelHeight{ panelPadding * 2 + rowHeight * 3 };
    _frame.GradientRectangle( panelPosition, panelPosition + Vector{ panelWidth, panelHeight }, panelTopColor, panelBottomColor, panelRadius );
    _frame.StrokeRectangle( panelPosition, panelPosition + Vector{ panelWidth, panelHeight }, panelBorderColor, 1, panelRadius );

    struct Row
    {
        const char * label;
        double rate;
        Color_d color;
        bool stateColored;
    };
    const std::array< Row, 3 > rows{ {
        { "SHIELD", info.shieldValue / info.shieldCapacity, shieldColor, true },
        { "PROPELLANT", info.propellantValue / info.propellantCapacity, propellantColor, true },
        { "LASER", info.laserPowerPercent / 100.0, laserColor, false },
    } };
    for( size_t i{ 0 }; i < rows.size(); i++ ) {
        const auto & row{ rows.at( i ) };
        const double rowY{ panelPosition.v + panelPadding + rowHeight * i };
        _frame.Text( { panelPosition.u + panelPadding, rowY + 4 }, "openSansBold", 12, row.label, labelColor, NanoVGRenderer::Frame::eTextAlign::topLeft, 1.4 );
        const auto color{ row.stateColored ? _StateColor( row.rate, row.color ) : row.color };
        _Bar( _frame, { panelPosition.u + panelPadding + labelWidth, rowY + 7 }, barWidth, barHeight, row.rate, color );
        _frame.Text( { panelPosition.u + panelWidth - panelPadding, rowY + 4 }, "sourceCodePro", 13, std::format( "{:3}%", static_cast< int >( std::round( std::clamp( row.rate, 0.0, 1.0 ) * 100 ) ) ), valueColor, NanoVGRenderer::Frame::eTextAlign::topRight );
    }

    // temporary bonuses as pills below the panel:
    struct Pill
    {
        std::string text;
        bool active;
    };
    const std::array< Pill, 4 > pills{ {
        { std::format( "H x{}", info.homingMissiles ), info.homingMissiles != 0 },
        { std::format( "M x{}", info.magneticMines ), info.magneticMines != 0 },
        { std::format( "S {}s", info.plasmaShieldSeconds ), info.plasmaShieldSeconds != 0 },
        { std::format( "T {}s", info.turretSeconds ), info.turretSeconds != 0 },
    } };
    constexpr double pillHeight{ 24 };
    constexpr double pillPaddingX{ 12 };
    constexpr double pillSpacing{ 8 };
    constexpr double pillCharWidth{ 8 }; // sourceCodePro is monospaced, close enough for sizing
    double pillX{ margin };
    const double pillY{ panelPosition.v + panelHeight + 10 };
    for( const auto & pill : pills ) {
        if( !pill.active )
            continue;
        const double pillWidth{ pillPaddingX * 2 + pillCharWidth * static_cast< double >( pill.text.size() ) };
        _frame.FillRectangle( { pillX, pillY }, { pillX + pillWidth, pillY + pillHeight }, { bonusColor.r, bonusColor.g, bonusColor.b, 0.12 }, pillHeight * 0.5 );
        _frame.StrokeRectangle( { pillX, pillY }, { pillX + pillWidth, pillY + pillHeight }, { bonusColor.r, bonusColor.g, bonusColor.b, 0.45 }, 1, pillHeight * 0.5 );
        _frame.Text( { pillX + pillWidth * 0.5, pillY + pillHeight * 0.5 }, "sourceCodePro", 13, pill.text, bonusColor, NanoVGRenderer::Frame::eTextAlign::center );
        pillX += pillWidth + pillSpacing;
    }

    // score, leading zeros dimmed:
    const Vector scoreCorner{ static_cast< double >( m_screenDimension.width ) - margin, margin };
    _frame.Text( { scoreCorner.u, scoreCorner.v }, "openSansBold", 12, "SCORE", labelColor, NanoVGRenderer::Frame::eTextAlign::topRight, 2.2 );
    const auto score{ std::format( "{:010}", info.score ) };
    _frame.Text( { scoreCorner.u, scoreCorner.v + 16 }, "sourceCodePro", 26, score, { 1, 1, 1, 0.18 }, NanoVGRenderer::Frame::eTextAlign::topRight );
    const auto significant{ score.find_first_not_of( '0' ) };
    if( significant != std::string::npos )
        _frame.Text( { scoreCorner.u, scoreCorner.v + 16 }, "sourceCodePro", 26, score.substr( significant ), valueColor, NanoVGRenderer::Frame::eTextAlign::topRight );
}
