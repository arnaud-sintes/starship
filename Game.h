#pragma once

#include "World.h"
#include "SceneRenderer.h"
#include "Hud.h"
#include "AudioDirector.h"
#include "View.h"
#include "PointerInput.h"
#include "UpdateChecker.h"
#include "SplashScreen.h"
#include "core/Packer.h"


// --------------
// Composition root: decodes player input, drives the prologue/stage/game-over flow,
// then updates the simulation and hands it to the renderers.
class Game
{
public:
    struct Options
    {
        bool minesLayout{ false }; // TEMPORARY test option (--mines): straight into a sandbox with a layout of mines
        bool foreman{ false };     // TEMPORARY test option (--foreman): straight into the stage 1 boss, invincible (F10 toggles)
        bool cheats{ false };      // --cheats: the F9 (skip) and F10 (invincibility) keys are active
        bool stocks{ false };      // TEMPORARY test option (--stocks): free play with big stocks of missiles and mines to hold and release, invincible
        bool upgrades{ false };    // TEMPORARY test option (--upgrades): straight to the ship upgrade screen, with skill points to spend
        bool singularity{ false }; // TEMPORARY test option (--singularity): a free-play sandbox where a singularity is deployed again and again
        bool warden{ false };      // TEMPORARY test option (--warden): same with the stage 2 boss
    };

    Game( const Win32::Windows & _windows, const View & _view, const Packer::Resources & _resources, const int _frameRate, const Options & _options = {} );

public:
    void Tick(); // one fixed-rate simulation step, called 0..N times per rendered frame
    void Draw( const NanoVGRenderer::Frame & _frame );

private:
    void _Restart();
    std::string _Click() const; // "CLICK", or the gamepad button doing it
    std::pair< Vector, Vector > _MenuRowRect( const int _index ) const; // the title screen menu: 0 = start new game, 1 = the tutorial checkbox
    bool _DetectTutorial();
    void _DrawStory( const NanoVGRenderer::Frame & _frame, const std::string & _tag, const std::string & _title, const std::vector< const char * > & _lines ); // chapter tag, big title, scenario lines
    void _DrawPrologue( const NanoVGRenderer::Frame & _frame );
    void _DrawStartMenu( const NanoVGRenderer::Frame & _frame, const double _alpha ); // on the title screen
    void _DrawStageStory( const NanoVGRenderer::Frame & _frame );
    void _DrawTutorial( const NanoVGRenderer::Frame & _frame );
    std::pair< Vector, Vector > _UpgradeCardRect( const size_t _index ) const; // one per ShipUpgrades::eSystem
    void _DrawUpgrade( const NanoVGRenderer::Frame & _frame );
    void _DrawGameOver( const NanoVGRenderer::Frame & _frame );
    void _NewGame(); // out of lives: back to the very beginning
    void _DrawCursor( const NanoVGRenderer::Frame & _frame );
    bool _UpdateNoticeVisible() const; // a newer release exists, and the screen is a menu
    std::pair< Vector, Vector > _UpdateNoticeRect() const;
    void _DrawUpdateNotice( const NanoVGRenderer::Frame & _frame );

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
        splash,     // the title screen, before anything
        stage11_prologue,
        stage11,
        stageStory, // full-screen scenario of a new stage, world frozen behind
        epilogue,   // full-screen ending once every mission is cleared, then free play
        tutorial, // world frozen behind an explanation panel
        upgrade,  // world frozen behind the ship upgrade cards, until the skill points are spent
        gameOver,
    };
    eStep m_step{ eStep::splash };
    SplashScreen m_splash;
    bool m_choiceFromKeys{ false }; // the arrow keys picked a line, until the mouse moves again
    Vector m_choiceCursor;
    int m_upgradeSelection{ 0 }; // the card of the ship upgrade screen picked with the pad
    int m_upgradeDirectionLatch{ 0 }; // one step per push of the stick or the d-pad
    int m_menuRow{ 0 }; // the line of the title screen menu picked with the keys or the pad
    PointerInput m_pointer; // the mouse or the gamepad
    UpdateChecker m_updateChecker; // asks GitHub for a newer release, in the background
    bool m_noticePressed{ false }; // the button was down over the update notice
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
    bool m_tutorialEnabled{ false }; // the checkbox of the title screen
    std::array< bool, static_cast< size_t >( eTutorial::count ) > m_tutorialSeen{};
    eTutorial m_tutorialTopic{ eTutorial::chaser };
    Vector m_tutorialTarget; // world position of the concerned item

    int m_storyStage{ 1 }; // highest stage whose scenario screen was shown (stage 1 has the prologue)
    bool m_epilogueShown{ false };
    bool m_cheats{ false }; // --cheats: F9 and F10 are active
    bool m_debugKeyDown{ false }; // TEMPORARY F9 test shortcut
    bool m_debugInvincibleKeyDown{ false }; // TEMPORARY F10 test shortcut
    double m_cursorFlash{ 0 };
    double m_overlayAnim{ 0 };
};
