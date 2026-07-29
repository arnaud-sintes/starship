#pragma once

#include "World.h"
#include "SceneRenderer.h"
#include "Hud.h"
#include "AudioDirector.h"
#include "core/Packer.h"


// --------------
// Composition root: decodes player input, drives the prologue/stage flow, then
// updates the simulation and hands it to the renderers.
class Game
{
public:
    Game( const Win32::Windows & _windows, const Packer::Resources & _resources, const int _frameRate );

public:
    void RunFrame( const NanoVGRenderer::Frame & _frame );

private:
    void _DrawPrologue( const NanoVGRenderer::Frame & _frame );
    void _DrawCursor( const NanoVGRenderer::Frame & _frame );

private:
    const Win32::Windows & m_windows;
    AudioDirector m_audio;
    World m_world;
    SceneRenderer m_scene;
    Hud m_hud;

    enum class eStep
    {
        stage11_prologue,
        stage11,
    };
    eStep m_step{ eStep::stage11_prologue };

    double m_cursorFlash{ 0 };
};
