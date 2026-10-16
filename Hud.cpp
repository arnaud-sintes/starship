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


void Hud::_DrawMission( const World & _world, const NanoVGRenderer::Frame & _frame ) const
{
    using ePhase = StageDirector::ePhase;
    const auto & _stage{ _world.Stage() };
    const Vector screenCenter{ static_cast< double >( m_screenDimension.width ) * 0.5, static_cast< double >( m_screenDimension.height ) * 0.5 };
    const auto phase{ _stage.Phase() };

    // boss health bar, top center: the phases are marked, the rear core is the hint
    if( phase == ePhase::active && _stage.IsBoss() ) {
        if( const auto * pBoss{ _world.Boss() } ) {
            constexpr double barPanelWidth{ 560 }, barPanelHeight{ 66 };
            const Vector a{ screenCenter.u - barPanelWidth * 0.5, margin };
            const Vector b{ a.u + barPanelWidth, a.v + barPanelHeight };
            _frame.GradientRectangle( a, b, panelTopColor, panelBottomColor, panelRadius );
            _frame.StrokeRectangle( a, b, { warningColor.r, warningColor.g, warningColor.b, 0.5 }, 1, panelRadius );
            _frame.Text( { screenCenter.u, a.v + 16 }, "openSansBold", 13, ( pBoss->type == Enemy::eType::warden ? "THE WARDEN" : "THE FOREMAN" ), warningColor, NanoVGRenderer::Frame::eTextAlign::center, 3 );
            const auto ratio{ pBoss->BossRatio() };
            const bool warden{ pBoss->type == Enemy::eType::warden };
            const Vector barA{ a.u + panelPadding, a.v + 32 };
            const double barW{ barPanelWidth - panelPadding * 2 };
            _Bar( _frame, barA, barW, 12, ratio, warden ? Color_d{ 0.65, 0.45 + 0.2 * ratio, 1 } : Color_d{ 1, 0.5 + 0.2 * ratio, 0.2 } );
            if( !warden || pBoss->PylonsAlive() > 0 ) // the phase thresholds (one per pylon for the warden)
            for( const auto mark : { 1.0 / 3, 2.0 / 3 } )
                _frame.Line( { barA.u + barW * mark, barA.v - 2 }, { barA.u + barW * mark, barA.v + 14 }, { 1, 1, 1, 0.55 }, 1.5 );
            _frame.Text( { screenCenter.u, a.v + 55 }, "openSansBold", 11, ( warden ? ( pBoss->PylonsAlive() > 0 ? std::format( "SHIELDED CORE  -  DESTROY THE PYLONS ({} / 3)", Enemy::pylonCount - pBoss->PylonsAlive() ) : std::string{ "CORE EXPOSED  -  FINISH IT!" } ) : pBoss->boss.vent == 2 ? "CORE EXPOSED  -  STRIKE IT FROM BEHIND!" : ( pBoss->boss.vent == 1 ? "THE CORE IS VENTING..." : "REAR CORE ARMORED  -  WAIT FOR THE VENT" ) ), labelColor, NanoVGRenderer::Frame::eTextAlign::center, 1.6 );
        }
    }

    // objective tracker, top center:
    else if( phase == ePhase::active || phase == ePhase::freePlay ) {
        constexpr double trackerWidth{ 240 }, trackerHeight{ 62 };
        const Vector a{ screenCenter.u - trackerWidth * 0.5, margin };
        const Vector b{ a.u + trackerWidth, a.v + trackerHeight };
        _frame.GradientRectangle( a, b, panelTopColor, panelBottomColor, panelRadius );
        _frame.StrokeRectangle( a, b, panelBorderColor, 1, panelRadius );
        const auto title{ phase == ePhase::freePlay ? _stage.TrackerLabel() : std::format( "{}  -  {}", _stage.Label(), _stage.TrackerLabel() ) };
        _frame.Text( { screenCenter.u, a.v + 15 }, "openSansBold", 12, title, labelColor, NanoVGRenderer::Frame::eTextAlign::center, 1.8 );
        _frame.Text( { screenCenter.u, a.v + 34 }, "sourceCodePro", 22, _stage.TrackerValue(), valueColor, NanoVGRenderer::Frame::eTextAlign::center );
        if( phase == ePhase::active )
            _Bar( _frame, { a.u + panelPadding, b.v - 12 }, trackerWidth - panelPadding * 2, 5, _stage.ObjectiveRatio(), accentColor );
    }

    // banners:
    if( phase != ePhase::intro && phase != ePhase::cleared )
        return;
    const auto ratio{ _stage.PhaseRatio() };
    auto fade{ std::clamp( std::min( { ratio / 0.12, ( 1 - ratio ) / 0.2, 1.0 } ), 0.0, 1.0 ) };
    const bool bossAlert{ phase == ePhase::intro && _stage.IsBoss() };
    if( bossAlert ) {
        // flashing red alert: pulsing text and a blinking vignette
        const auto flash{ std::abs( std::sin( m_pulse * 1.3 ) ) };
        _frame.FillRectangle( {}, { static_cast< double >( m_screenDimension.width ), static_cast< double >( m_screenDimension.height ) }, { 1, 0.08, 0.12, 0.16 * flash * fade } );
        fade *= 0.35 + 0.65 * flash;
    }
    const Vector titleCenter{ screenCenter.u, screenCenter.v * 0.5 };
    const auto drawBanner{ [ & ]( const char * _tag, const std::string & _title, const std::string & _subtitle, const Color_d & _color ){
            _frame.Text( titleCenter - Vector{ 0, 70 }, "openSansBold", 15, _tag, { _color.r, _color.g, _color.b, 0.85 * fade }, NanoVGRenderer::Frame::eTextAlign::center, 6 );
            _frame.Text( titleCenter + Vector{ 0, 3 }, "openSansBold", 84, _title, { _color.r, _color.g, _color.b, 0.3 * fade }, NanoVGRenderer::Frame::eTextAlign::center, 8 );
            _frame.Text( titleCenter, "openSansBold", 84, _title, { 1, 1, 1, fade }, NanoVGRenderer::Frame::eTextAlign::center, 8 );
            const double dividerY{ titleCenter.v + 70 };
            _frame.Line( { screenCenter.u - 320, dividerY }, { screenCenter.u + 320, dividerY }, { _color.r, _color.g, _color.b, 0.35 * fade }, 1 );
            _frame.FillCircle( { screenCenter.u, dividerY }, 2.5, { _color.r, _color.g, _color.b, 0.8 * fade } );
            _frame.Text( { screenCenter.u, dividerY + 38 }, "openSansBold", 26, _subtitle, { 0.85, 0.92, 1, 0.95 * fade }, NanoVGRenderer::Frame::eTextAlign::center, 4 );
        } };
    if( bossAlert )
        drawBanner( "UNIDENTIFIED SIGNATURE", "WARNING", _stage.ObjectiveText(), criticalColor );
    else if( phase == ePhase::intro )
        drawBanner( _stage.StageTitle(), _stage.Label(), _stage.ObjectiveText(), accentColor );
    else
        drawBanner( "OBJECTIVE ACCOMPLISHED", _stage.LastOfStage() ? std::format( "STAGE {} CLEARED", _stage.StageNumber() ) : "MISSION COMPLETE",
            "SHIELD AND PROPELLANT RESTORED", bonusColor );
}


void Hud::Draw( const World & _world, const NanoVGRenderer::Frame & _frame )
{
    m_pulse += 0.16;
    _DrawMission( _world, _frame );

    const auto info{ _world.GetHudInfo() };

    // status gauges, top left, no frame: a soft shade behind keeps them readable, then three stacked gauges (an accent tick, the
    // label and the value on one line, a thin segmented bar with a glowing tip underneath)
    const Vector origin{ margin + 2, margin };
    constexpr double gaugeWidth{ 236 }, gaugePitch{ 38 }, gaugeBarHeight{ 5 };
    {
        const Vector shadeCenter{ origin.u + gaugeWidth * 0.5, origin.v + gaugePitch * 1.5 };
        _frame.GradientCircle( shadeCenter, 250, { 0, 0.015, 0.04, 0.5 }, { 0, 0.015, 0.04, 0 } );
    }

    // the pips of an upgradable system: one per level, filled as far as it is bought
    const auto pips{ [ & ]( const Vector & _position, const int _count, const int _level, const Color_d & _color ) {
            for( int pip{ 0 }; pip < _count; pip++ ) {
                const Vector a{ _position.u + pip * 8, _position.v };
                if( pip < _level )
                    _frame.FillRectangle( a, a + Vector{ 5, 5 }, { _color.r, _color.g, _color.b, 0.95 }, 1.5 );
                else
                    _frame.FillRectangle( a, a + Vector{ 5, 5 }, { _color.r, _color.g, _color.b, 0.2 }, 1.5 );
            }
        } };

    struct Row
    {
        const char * label;
        double rate;
        Color_d color;
        bool stateColored;
        int level; // upgrade level shown as pips (-1: not upgradable)
    };
    const std::array< Row, 3 > rows{ {
        { "SHIELD", info.shieldValue / info.shieldCapacity, shieldColor, true, info.shieldLevel },
        { "PROPELLANT", info.propellantValue / info.propellantCapacity, propellantColor, true, info.propellantLevel },
        { "LASER", 0, laserColor, false, -1 },
    } };
    for( size_t i{ 0 }; i < rows.size(); i++ ) {
        const auto & row{ rows.at( i ) };
        const auto rate{ std::clamp( row.rate, 0.0, 1.0 ) };
        const Vector top{ origin.u, origin.v + gaugePitch * static_cast< double >( i ) };
        const auto color{ row.stateColored ? _StateColor( row.rate, row.color ) : row.color };
        _frame.FillRectangle( top + Vector{ 0, 2 }, top + Vector{ 3, 30 }, { color.r, color.g, color.b, std::max( color.a, 0.6 ) }, 1.5 ); // the accent tick
        _frame.Text( top + Vector{ 14, 3 }, "openSansBold", 11, row.label, { color.r, color.g, color.b, 0.9 }, NanoVGRenderer::Frame::eTextAlign::topLeft, 2.6 );
        const auto labelEnd{ top.u + 14 + _frame.TextWidth( "openSansBold", 11, row.label, 2.6 ) + 10 };
        const Vector trackA{ top.u + 14, top.v + 22 };
        const double trackWidth{ gaugeWidth - 14 };
        if( row.level >= 0 ) { // shield, propellant: the upgrade level as pips, the value, the bar
            pips( { labelEnd, top.v + 6 }, ShipUpgrades::maxLevel, row.level, row.color );
            _frame.Text( top + Vector{ gaugeWidth, 0 }, "sourceCodePro", 17, std::format( "{:3}", static_cast< int >( std::round( rate * 100 ) ) ), valueColor, NanoVGRenderer::Frame::eTextAlign::topRight );
            // the bar: a track, the level, ticks every quarter, a glowing tip
            _frame.FillRectangle( trackA, trackA + Vector{ trackWidth, gaugeBarHeight }, trackColor, gaugeBarHeight * 0.5 );
            if( rate > 0 ) {
                const Color_d brighter{ std::min( color.r * 1.25, 1.0 ), std::min( color.g * 1.25, 1.0 ), std::min( color.b * 1.25, 1.0 ), color.a };
                _frame.GradientRectangle( trackA, trackA + Vector{ trackWidth * rate, gaugeBarHeight }, brighter, color, gaugeBarHeight * 0.5 );
                const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                _frame.GradientCircle( trackA + Vector{ trackWidth * rate, gaugeBarHeight * 0.5 }, 11, { color.r, color.g, color.b, 0.5 * color.a }, { color.r, color.g, color.b, 0 } );
            }
            for( const auto mark : { 0.25, 0.5, 0.75 } )
                _frame.Line( trackA + Vector{ trackWidth * mark, -1 }, trackA + Vector{ trackWidth * mark, gaugeBarHeight + 1 }, { 0, 0.02, 0.05, 0.75 }, 1.5 );
            continue;
        }
        // the laser is a weapon, not a quantity: the whole of its progression, from the base gun to the full volley, as a row of
        // vertical bars lit as far as it is upgraded
        const auto steps{ info.laserLevelMax + 1 };
        for( int step{ 0 }; step < steps; step++ ) {
            const double x{ trackA.u + ( trackWidth - 3 ) * step / ( steps - 1 ) };
            const bool lit{ step <= info.laserLevel };
            if( lit ) {
                const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
                _frame.GradientCircle( { x + 1.5, trackA.v + 2.5 }, step == info.laserLevel ? 11 : 7, { laserColor.r, laserColor.g, laserColor.b, step == info.laserLevel ? 0.5 : 0.25 }, { laserColor.r, laserColor.g, laserColor.b, 0 } );
            }
            _frame.FillRectangle( { x, trackA.v - 2 }, { x + 3, trackA.v + 9 }, lit ? Color_d{ laserColor.r, laserColor.g, laserColor.b, 0.95 } : trackColor, 1.5 );
        }
    }

    // the other two systems of the ship, upgraded on the hangar screen: small, under the gauges
    {
        const double lineY{ origin.v + gaugePitch * 3 + 1 };
        _frame.Text( { origin.u + 14, lineY }, "openSansBold", 10, "ENGINE", { labelColor.r, labelColor.g, labelColor.b, 0.7 }, NanoVGRenderer::Frame::eTextAlign::topLeft, 2.2 );
        pips( { origin.u + 14 + _frame.TextWidth( "openSansBold", 10, "ENGINE", 2.2 ) + 8, lineY + 3 }, ShipUpgrades::maxLevel, info.engineLevel, accentColor );
        _frame.Text( { origin.u + 130, lineY }, "openSansBold", 10, "ROTATORS", { labelColor.r, labelColor.g, labelColor.b, 0.7 }, NanoVGRenderer::Frame::eTextAlign::topLeft, 2.2 );
        pips( { origin.u + 130 + _frame.TextWidth( "openSansBold", 10, "ROTATORS", 2.2 ) + 8, lineY + 3 }, ShipUpgrades::maxLevel, info.rotatorLevel, accentColor );
    }

    // temporary bonuses as chips below the gauges (no outline either):
    struct Pill
    {
        std::string text;
        bool active;
        bool held{ false }; // paused: the stocks are kept for later
    };
    const std::array< Pill, 10 > pills{ {
        { "HOLD", info.stocksHeld, true },
        { std::format( "H x{}", info.homingMissiles ), info.homingMissiles != 0, info.stocksHeld },
        { std::format( "M x{}", info.magneticMines ), info.magneticMines != 0, info.stocksHeld },
        { std::format( "S {}s", info.plasmaShieldSeconds ), info.plasmaShieldSeconds != 0 },
        { std::format( "T {}s", info.turretSeconds ), info.turretSeconds != 0 },
        { std::format( "D {}s", info.decoySeconds ), info.decoySeconds != 0 },
        { std::format( "E {}s", info.empSeconds ), info.empSeconds != 0 },
        { std::format( "O {}s", info.overdriveSeconds ), info.overdriveSeconds != 0 },
        { std::format( "B {}s", info.blossomSeconds ), info.blossomSeconds != 0 },
        { std::format( "X {}s", info.singularitySeconds ), info.singularitySeconds != 0 },
    } };
    constexpr double pillHeight{ 22 };
    constexpr double pillPaddingX{ 11 };
    constexpr double pillSpacing{ 6 };
    constexpr double pillCharWidth{ 8 }; // sourceCodePro is monospaced, close enough for sizing
    double pillX{ origin.u };
    const double pillY{ origin.v + gaugePitch * 3 + 24 };
    for( const auto & pill : pills ) {
        if( !pill.active )
            continue;
        const double pillWidth{ pillPaddingX * 2 + pillCharWidth * static_cast< double >( pill.text.size() ) };
        const Color_d color{ pill.held ? Color_d{ 1, 0.78, 0.3 } : bonusColor }; // gold while held
        _frame.FillRectangle( { pillX, pillY }, { pillX + pillWidth, pillY + pillHeight }, { color.r * 0.25, color.g * 0.25, color.b * 0.25, 0.55 }, pillHeight * 0.5 );
        _frame.Text( { pillX + pillWidth * 0.5, pillY + pillHeight * 0.5 }, "sourceCodePro", 13, pill.text, color, NanoVGRenderer::Frame::eTextAlign::center );
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

    if( _world.DebugInvincible() )
        _frame.Text( { scoreCorner.u, scoreCorner.v + 80 }, "openSansBold", 12, "INVINCIBLE", bonusColor, NanoVGRenderer::Frame::eTextAlign::topRight, 2.2 );

    // remaining ships, as small hulls (the lost ones stay as dim outlines); free play has none:
    if( _world.Stage().Phase() != StageDirector::ePhase::freePlay ) {
        constexpr int maxLives{ 5 };
        constexpr double shipWidth{ 16 }, shipHeight{ 18 }, shipSpacing{ 8 };
        const double y{ scoreCorner.v + 54 };
        const bool lastShip{ _world.Lives() == 1 };
        for( int i{ 0 }; i < maxLives; i++ ) {
            const double right{ scoreCorner.u - i * ( shipWidth + shipSpacing ) };
            const std::vector< Position_d > hull{ { right - shipWidth * 0.5, y }, { right, y + shipHeight }, { right - shipWidth * 0.5, y + shipHeight * 0.72 }, { right - shipWidth, y + shipHeight } };
            const bool alive{ i < _world.Lives() };
            if( alive ) {
                const Color_d color{ lastShip ? Color_d{ criticalColor.r, criticalColor.g, criticalColor.b, 0.45 + ( std::sin( m_pulse ) + 1 ) * 0.275 } : shieldColor };
                _frame.FillPolygon( hull, { color.r, color.g, color.b, color.a * 0.6 } );
                _frame.StrokePolygon( hull, color, 1.5 );
            }
            else
                _frame.StrokePolygon( hull, { 1, 1, 1, 0.15 }, 1 );
        }
    }
}
