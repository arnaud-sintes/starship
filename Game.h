#pragma once

#include "World.h"
#include "SceneRenderer.h"
#include "Hud.h"
#include "AudioDirector.h"
#include "View.h"
#include "core/Packer.h"


// --------------
// Composition root: decodes player input, drives the prologue/stage/game-over flow,
// then updates the simulation and hands it to the renderers.
class Game
{
public:
    Game( const Win32::Windows & _windows, const View & _view, const Packer::Resources & _resources, const int _frameRate );

public:
    void Tick(); // one fixed-rate simulation step, called 0..N times per rendered frame
    void Draw( const NanoVGRenderer::Frame & _frame );

private:
    void _Restart();
    void _DrawPrologue( const NanoVGRenderer::Frame & _frame );
    void _DrawGameOver( const NanoVGRenderer::Frame & _frame );
    void _DrawCursor( const NanoVGRenderer::Frame & _frame );

private:
    const Win32::Windows & m_windows;
    const View & m_view;
    const int m_frameRate;
    AudioDirector m_audio;
    std::unique_ptr< World > m_world; // rebuilt on retry
    SceneRenderer m_scene;
    Hud m_hud;

    enum class eStep
    {
        stage11_prologue,
        stage11,
        gameOver,
    };
    eStep m_step{ eStep::stage11_prologue };
    bool m_clickArmed{ false }; // require a button release before accepting the retry click

    double m_cursorFlash{ 0 };
    double m_overlayAnim{ 0 };
};
