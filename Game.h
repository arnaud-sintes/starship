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
    std::pair< Vector, Vector > _PrologueButtonRect( const int _index ) const; // 0 = yes, 1 = no
    bool _DetectTutorial();
    void _DrawPrologue( const NanoVGRenderer::Frame & _frame );
    void _DrawTutorial( const NanoVGRenderer::Frame & _frame );
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
        tutorial, // world frozen behind an explanation panel
        gameOver,
    };
    eStep m_step{ eStep::stage11_prologue };
    bool m_clickArmed{ false }; // require a button release before accepting the retry click

    // in-game tutorial (opt-in at the prologue): the first time each object type comes
    // close, the game pauses and explains it (topics survive retries):
    enum class eTutorial : size_t
    {
        chaser,
        wasp,
        sniper,
        gravityMine,
        attractor,
        missile,
        // one per goody, MUST follow the Goody::eType order (mapped by offset):
        goodyLaserUp,
        goodyHomingMissiles,
        goodyMagneticMines,
        goodyPlasmaShield,
        goodyShieldAdd,
        goodyPropellantAdd,
        goodyTurret,
        goodyRepulsor,
        goodyDecoy,
        goodyEmp,
        goodyOverdrive,
        goodySingularity,
        goodyBlossom,
        goodyHellstorm,
        count
    };
    bool m_tutorialEnabled{ true };
    std::array< bool, static_cast< size_t >( eTutorial::count ) > m_tutorialSeen{};
    eTutorial m_tutorialTopic{ eTutorial::chaser };
    Vector m_tutorialTarget; // world position of the concerned item

    double m_cursorFlash{ 0 };
    double m_overlayAnim{ 0 };
};
