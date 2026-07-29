#include "Game.h"


namespace
{
    constexpr Color_d accentColor{ 0.35, 0.65, 1 };
    constexpr Color_d dangerColor{ 1, 0.32, 0.38 };
}


Game::Game( const Win32::Windows & _windows, const View & _view, const Packer::Resources & _resources, const int _frameRate )
    : m_windows{ _windows }
    , m_view{ _view }
    , m_frameRate{ _frameRate }
    , m_audio{ _view.logical, _resources }
    , m_world{ std::make_unique< World >( m_audio, _view.logical, _frameRate ) }
    , m_scene{ _view.logical }
    , m_hud{ _view.logical }
{}


void Game::_Restart()
{
    m_world = std::make_unique< World >( m_audio, m_view.logical, m_frameRate );
    m_step = eStep::stage11;
}


void Game::Tick()
{
    if( m_step == eStep::stage11_prologue ) {
        m_world->UpdateAmbientSound();
        m_audio.Update();
        if( m_windows.LeftMouseButtonPressed() )
            m_step = eStep::stage11;
        return;
    }

    if( m_step == eStep::stage11 ) {
        const PlayerInput input{
            m_view.ToLogical( m_windows.CursorPosition() ),
            m_windows.LeftMouseButtonPressed(),
            m_windows.RightMouseButtonPressed(),
        };
        m_world->Update( input );
        m_audio.Update();
        // let the explosion play out before the overlay:
        if( m_world->ShipDestroyed() && m_world->ShipDestroyedTicks() > m_frameRate * 2 ) {
            m_step = eStep::gameOver;
            m_clickArmed = false;
        }
        return;
    }

    // game over: the world keeps living behind the overlay
    m_world->Update( {} );
    m_audio.Update();
    if( !m_windows.LeftMouseButtonPressed() )
        m_clickArmed = true;
    else if( m_clickArmed )
        _Restart();
}


void Game::Draw( const NanoVGRenderer::Frame & _frame )
{
    if( m_step == eStep::stage11_prologue ) {
        m_scene.DrawBackdrop( *m_world, _frame );
        _DrawPrologue( _frame );
        _DrawCursor( _frame );
        return;
    }

    m_scene.Draw( *m_world, _frame );
    if( m_step == eStep::gameOver )
        _DrawGameOver( _frame );
    else
        m_hud.Draw( *m_world, _frame );
    _DrawCursor( _frame );
}


void Game::_DrawPrologue( const NanoVGRenderer::Frame & _frame )
{
    m_overlayAnim += 0.05;
    const auto & dimension{ m_view.logical };
    const Vector screenCenter{ static_cast< double >( dimension.width ) * 0.5, static_cast< double >( dimension.height ) * 0.5 };
    const Vector titleCenter{ screenCenter - Vector{ 0, 60 } };

    // chapter tag + title with a subtle accent glow:
    _frame.Text( titleCenter - Vector{ 0, 70 }, "openSansBold", 15, "M I S S I O N", { accentColor.r, accentColor.g, accentColor.b, 0.85 }, NanoVGRenderer::Frame::eTextAlign::center, 6 );
    _frame.Text( titleCenter + Vector{ 0, 3 }, "openSansBold", 84, "STAGE 1-1", { accentColor.r, accentColor.g, accentColor.b, 0.3 }, NanoVGRenderer::Frame::eTextAlign::center, 8 );
    _frame.Text( titleCenter, "openSansBold", 84, "STAGE 1-1", colorWhite, NanoVGRenderer::Frame::eTextAlign::center, 8 );

    // divider:
    const double dividerY{ titleCenter.v + 70 };
    _frame.Line( { screenCenter.u - 320, dividerY }, { screenCenter.u + 320, dividerY }, { accentColor.r, accentColor.g, accentColor.b, 0.35 }, 1 );
    _frame.FillCircle( { screenCenter.u, dividerY }, 2.5, { accentColor.r, accentColor.g, accentColor.b, 0.8 } );

    // story:
    static const std::array< const char *, 7 > lines{
        "By jumping out of hyperdrive to reach the Rexxus-3b system, you encounter hostile resistance from the",
        "space mining guild who are illegally exploiting the strange attractors energy around the planet Stellis Secura.",
        "",
        "As a faithful member of the Universal Alliance for Peace, you cannot tolerate such a defiance to our glorious",
        "open-despocracy and the uncontestable authority of our galactic emperator Muhammad Silmane XIV!",
        "",
        "Let's teach this gang of small-time smugglers a lesson to remember..." };
    Vector verticalText{ 0, dividerY - screenCenter.v + 50 };
    for( const auto * text : lines ) {
        _frame.Text( screenCenter + verticalText, "openSans", 22, text, { 0.72, 0.85, 1, 0.92 }, NanoVGRenderer::Frame::eTextAlign::center );
        verticalText.v += 34;
    }

    // pulsing call to action:
    const double pulse{ ( std::sin( m_overlayAnim * 2 ) + 1 ) * 0.5 };
    _frame.Text( screenCenter + Vector{ 0, verticalText.v + 60 }, "openSansBold", 16, "CLICK TO ENGAGE", { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 5 );
}


void Game::_DrawGameOver( const NanoVGRenderer::Frame & _frame )
{
    m_overlayAnim += 0.05;
    const auto & dimension{ m_view.logical };
    const Vector screen{ static_cast< double >( dimension.width ), static_cast< double >( dimension.height ) };
    const Vector screenCenter{ screen * 0.5 };

    // dark veil over the still-living world:
    _frame.FillRectangle( {}, screen, { 0, 0.01, 0.03, 0.6 } );

    // title with a red glow:
    const Vector titleCenter{ screenCenter - Vector{ 0, 80 } };
    _frame.Text( titleCenter + Vector{ 0, 3 }, "openSansBold", 84, "SHIP DESTROYED", { dangerColor.r, dangerColor.g, dangerColor.b, 0.35 }, NanoVGRenderer::Frame::eTextAlign::center, 8 );
    _frame.Text( titleCenter, "openSansBold", 84, "SHIP DESTROYED", colorWhite, NanoVGRenderer::Frame::eTextAlign::center, 8 );

    // divider:
    const double dividerY{ titleCenter.v + 70 };
    _frame.Line( { screenCenter.u - 320, dividerY }, { screenCenter.u + 320, dividerY }, { dangerColor.r, dangerColor.g, dangerColor.b, 0.4 }, 1 );
    _frame.FillCircle( { screenCenter.u, dividerY }, 2.5, { dangerColor.r, dangerColor.g, dangerColor.b, 0.85 } );

    // final score, leading zeros dimmed:
    _frame.Text( { screenCenter.u, dividerY + 45 }, "openSansBold", 15, "F I N A L   S C O R E", { 0.62, 0.72, 0.88, 0.9 }, NanoVGRenderer::Frame::eTextAlign::center, 2 );
    const auto score{ std::format( "{:010}", m_world->GetHudInfo().score ) };
    const Vector scoreCenter{ screenCenter.u, dividerY + 90 };
    _frame.Text( scoreCenter, "sourceCodePro", 42, score, { 1, 1, 1, 0.18 }, NanoVGRenderer::Frame::eTextAlign::center );
    const auto significant{ score.find_first_not_of( '0' ) };
    if( significant != std::string::npos ) {
        // right-align the bright suffix over the dim full number (monospace font):
        constexpr double charWidth{ 42 * 0.6 }; // SourceCodePro advance ~0.6em
        const double offset{ static_cast< double >( significant ) * charWidth * 0.5 };
        _frame.Text( { scoreCenter.u + offset, scoreCenter.v }, "sourceCodePro", 42, score.substr( significant ), { 0.85, 0.92, 1 }, NanoVGRenderer::Frame::eTextAlign::center );
    }

    // pulsing call to action:
    const double pulse{ ( std::sin( m_overlayAnim * 2 ) + 1 ) * 0.5 };
    _frame.Text( { screenCenter.u, dividerY + 170 }, "openSansBold", 16, "CLICK TO RETRY", { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 5 );
}


void Game::_DrawCursor( const NanoVGRenderer::Frame & _frame )
{
    const auto position{ m_view.ToLogical( m_windows.CursorPosition() ) };
    const double pulse{ std::sin( m_cursorFlash += 0.15 ) };
    const Color_d color{ 0.35, 0.62 + pulse * 0.18, 1, 0.9 };
    // thin reticle: ring, center dot and four ticks
    _frame.StrokeCircle( position, 13, color, 1.5 );
    _frame.FillCircle( position, 1.8, color );
    constexpr double tickInner{ 16 }, tickOuter{ 22 };
    _frame.Line( position + Vector{ 0, -tickInner }, position + Vector{ 0, -tickOuter }, color, 1.5 );
    _frame.Line( position + Vector{ 0, tickInner }, position + Vector{ 0, tickOuter }, color, 1.5 );
    _frame.Line( position + Vector{ -tickInner, 0 }, position + Vector{ -tickOuter, 0 }, color, 1.5 );
    _frame.Line( position + Vector{ tickInner, 0 }, position + Vector{ tickOuter, 0 }, color, 1.5 );
}
