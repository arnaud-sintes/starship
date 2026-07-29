#include "Hud.h"


Hud::Hud( const Dimension_ui & _screenDimension )
    : m_screenDimension{ _screenDimension }
{}


void Hud::Draw( const World & _world, const NanoVGRenderer::Frame & _frame ) const
{
    const auto info{ _world.GetHudInfo() };

    constexpr double margin{ 8 };
    constexpr double spacing{ 6 };
    constexpr double borderRadius{ 2 };
    constexpr double strokeWidth{ 1 };
    constexpr double xText{ margin };
    constexpr double yText{ 2 };
    constexpr double textHeight{ 18 };
    constexpr double xMenu{ xText + 110 };
    constexpr double barWidth{ 150 };
    constexpr double barHeight{ 18 };
    double yMenu{ 0 };

    // shield state:
    yMenu = margin;
    _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Shield:", colorWhite );
    const auto shieldRate{ std::round( info.shieldValue * barWidth ) / info.shieldCapacity };
    _frame.FillRectangle( { xMenu, yMenu }, { xMenu + shieldRate, yMenu + barHeight }, Color_d::FadeOrange( 1 - ( shieldRate / barWidth ) ) );
    _frame.StrokeRectangle( { xMenu, yMenu }, { xMenu + barWidth, yMenu + barHeight }, colorWhite, strokeWidth, borderRadius );

    // propellant state:
    yMenu = margin + barHeight + spacing;
    _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Propellant:", colorWhite );
    const auto propellantRate{ std::round( info.propellantValue * barWidth ) / info.propellantCapacity };
    _frame.FillRectangle( { xMenu, yMenu }, { xMenu + propellantRate, yMenu + barHeight }, Color_d::FadeViolet( 1 - ( propellantRate / barWidth ) ) );
    _frame.StrokeRectangle( { xMenu, yMenu }, { xMenu + barWidth, yMenu + barHeight }, colorWhite, strokeWidth, borderRadius );

    // laser power state:
    yMenu = margin + ( barHeight + spacing ) * 2;
    _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Laser power: " + std::to_string( info.laserPowerPercent ) + "%", { 1, 0.5, 0.5 } );

    int optionalInfos{ 2 };
    constexpr Color_d greenColor{ 0.5, 1, 0.5 };

    // homing missiles count (temporary):
    if( info.homingMissiles != 0 ) {
        yMenu = margin + ( barHeight + spacing ) * ++optionalInfos;
        _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Homing-missiles x" + std::to_string( info.homingMissiles ), greenColor );
    }

    // magnetic mines count (temporary):
    if( info.magneticMines != 0 ) {
        yMenu = margin + ( barHeight + spacing ) * ++optionalInfos;
        _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Magnetic-mines x" + std::to_string( info.magneticMines ), greenColor );
    }

    // plasma shield remaining time (temporary):
    if( info.plasmaShieldSeconds != 0 ) {
        yMenu = margin + ( barHeight + spacing ) * ++optionalInfos;
        _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Plasma-shield: " + std::to_string( info.plasmaShieldSeconds ) + "s", greenColor );
    }

    // score:
    _frame.Text( { static_cast< double >( m_screenDimension.width ) - margin, margin }, "openSansBold", 24, std::format( "{:010}", info.score ), colorWhite, NanoVGRenderer::Frame::eTextAlign::topRight );
}
