#include "Game.h"


Game::Game( const Win32::Windows & _windows, const Packer::Resources & _resources, const int _frameRate )
    : m_windows{ _windows }
    , m_audio{ _windows, _resources }
    , m_world{ m_audio, _windows.GetDimension(), _frameRate }
    , m_scene{ _windows.GetDimension() }
    , m_hud{ _windows.GetDimension() }
{}


void Game::RunFrame( const NanoVGRenderer::Frame & _frame )
{
    if( m_step == eStep::stage11_prologue ) {
        m_scene.DrawBackdrop( m_world, _frame );
        m_world.UpdateAmbientSound();
        _DrawPrologue( _frame );
        _DrawCursor( _frame );
        m_audio.Update();
        if( m_windows.LeftMouseButtonPressed() )
            m_step = eStep::stage11;
        return;
    }

    const PlayerInput input{
        Vector::From( m_windows.CursorPosition().ToType< double >() ),
        m_windows.LeftMouseButtonPressed(),
        m_windows.RightMouseButtonPressed(),
    };
    m_world.Update( input );
    m_scene.Draw( m_world, _frame );
    m_hud.Draw( m_world, _frame );
    _DrawCursor( _frame );
    m_audio.Update();
}


void Game::_DrawPrologue( const NanoVGRenderer::Frame & _frame )
{
    const auto & dimension{ m_windows.GetDimension() };
    const Vector screenCenter{ static_cast< double >( dimension.width ) * 0.5, static_cast< double >( dimension.height ) * 0.5 };
    _frame.Text( screenCenter, "openSansBold", 80, "STAGE 1-1", colorWhite, NanoVGRenderer::Frame::eTextAlign::center );
    static const std::array< const char *, 7 > lines{
        "By jumping out of hyperdrive to reach the Rexxus-3b system, you encounter hostile resistance from the",
        "space mining guild who are illegally exploiting the strange attractors energy around the planet Stellis Secura.",
        "",
        "As a faithful member of the Universal Alliance for Peace, you cannot tolerate such a defiance to our glorious",
        "open-despocracy and the uncontestable authority of our galactic emperator Muhammad Silmane XIV!",
        "",
        "Let's teach this gang of small-time smugglers a lesson to remember..." };
    Vector verticalText{ 0, 100 };
    for( const auto * text : lines ) {
        _frame.Text( screenCenter + verticalText, "openSans", 24, text, { 0.6, 0.85, 1 }, NanoVGRenderer::Frame::eTextAlign::center );
        verticalText.v += 32;
    }
}


void Game::_DrawCursor( const NanoVGRenderer::Frame & _frame )
{
    _frame.StrokeCircle( m_windows.CursorPosition().ToType< double >(), 15, { 0.25, 0.5 + std::sin( m_cursorFlash += 0.15 ) * 0.25, 1 }, 4 );
}
