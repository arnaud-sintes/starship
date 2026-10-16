#include "Game.h"

#include "core/Macros.h"
#include "version.h"


namespace
{
    constexpr Color_d accentColor{ 0.35, 0.65, 1 };
    constexpr Color_d dangerColor{ 1, 0.32, 0.38 };
}


Game::Game( const Win32::Windows & _windows, const View & _view, const Packer::Resources & _resources, const int _frameRate, const Options & _options )
    : m_windows{ _windows }
    , m_view{ _view }
    , m_frameRate{ _frameRate }
    , m_audio{ _view.logical, _resources }
    , m_updateChecker{ __ToString( VERSION ) }
    , m_world{ std::make_unique< World >( m_audio, _view.logical, _frameRate, ( _options.minesLayout || _options.singularity ) ? WorldCheckpoint{ .progress = 99 } : ( _options.warden ? WorldCheckpoint{ .progress = 11 } : ( _options.foreman ? WorldCheckpoint{ .progress = 5 } : WorldCheckpoint{} ) ) ) } // 99: past the last mission, free play; 5, 11: the bosses
    , m_scene{ _view.logical }
    , m_hud{ _view.logical }
{
    if( _options.minesLayout || _options.singularity ) {
        if( _options.singularity )
            m_world->DebugSingularity();
        else
            m_world->DebugMinesLayout();
        m_step = eStep::stage11; // no prologue, no tutorial, no epilogue
        m_tutorialEnabled = false;
        m_epilogueShown = true;
    }
    else if( _options.warden || _options.foreman ) {
        m_world->DebugToggleInvincible();
        m_world->DebugBossInView();
        m_step = eStep::stage11; // no prologue, no tutorial, no scenario screen
        m_tutorialEnabled = false;
        m_storyStage = _options.warden ? 2 : 1;
    }
}


void Game::_Restart()
{
    // restarts the mission being played, with the gear it started with:
    const WorldCheckpoint checkpoint{ m_world->Checkpoint() };
    m_world = std::make_unique< World >( m_audio, m_view.logical, m_frameRate, checkpoint );
    m_step = eStep::stage11;
    if( m_world->Lives() == 1 && m_world->Stage().Phase() != StageDirector::ePhase::freePlay )
        m_audio.Play( eSound::lastShip );
}


void Game::_NewGame()
{
    m_world = std::make_unique< World >( m_audio, m_view.logical, m_frameRate );
    m_step = eStep::stage11_prologue;
    m_storyStage = 1;
    m_epilogueShown = false;
    m_clickArmed = false;
}


void Game::Tick()
{
    // the mouse or the gamepad, whichever is in use (the gamepad's aim assist sticks to the closest enemy):
    std::optional< PointerInput::AimTarget > aimTarget;
    if( m_step == eStep::stage11 && !m_world->ShipDestroyed() )
        if( const auto * pEnemy{ m_world->ClosestEnemy( m_world->Ship().position ) } )
            aimTarget = PointerInput::AimTarget{ pEnemy->id, pEnemy->position - m_world->Ship().position, pEnemy->dynamic.boundingBoxRadius };
    m_pointer.Update( m_windows, m_view, m_step == eStep::stage11, 1.0 / m_frameRate, aimTarget );

    // a click on the update notice opens the download page, and is not meant for anything else:
    if( _UpdateNoticeVisible() ) {
        const auto [ a, b ]{ _UpdateNoticeRect() };
        const auto cursor{ m_pointer.Cursor() };
        if( m_pointer.Left() && cursor.u >= a.u && cursor.u <= b.u && cursor.v >= a.v && cursor.v <= b.v ) {
            if( !m_noticePressed )
                Win32::OpenUrl( m_updateChecker.Url() );
            m_clickArmed = false;
        }
        m_noticePressed = m_pointer.Left();
    }
    if( m_step == eStep::stage11_prologue ) {
        m_world->UpdateAmbientSound();
        m_audio.Update();
        // engage by answering the tutorial question (YES / NO buttons):
        if( !m_pointer.Left() )
            m_clickArmed = true;
        else if( m_clickArmed ) {
            const auto cursor{ m_pointer.Cursor() };
            const auto inside{ [ & ]( const int _index ){
                    const auto [ a, b ]{ _PrologueButtonRect( _index ) };
                    return cursor.u >= a.u && cursor.u <= b.u && cursor.v >= a.v && cursor.v <= b.v;
                } };
            if( inside( 0 ) || inside( 1 ) ) {
                m_tutorialEnabled = inside( 0 );
                m_step = eStep::stage11;
                m_clickArmed = false;
            }
        }
        return;
    }

    if( m_step == eStep::upgrade ) {
        m_audio.Update();
        if( !m_pointer.Left() )
            m_clickArmed = true;
        else if( m_clickArmed ) {
            const auto cursor{ m_pointer.Cursor() };
            for( size_t i{ 0 }; i < static_cast< size_t >( ShipUpgrades::eSystem::count ); i++ ) {
                const auto [ a, b ]{ _UpgradeCardRect( i ) };
                if( cursor.u >= a.u && cursor.u <= b.u && cursor.v >= a.v && cursor.v <= b.v ) {
                    m_world->Upgrade( static_cast< ShipUpgrades::eSystem >( i ) );
                    m_clickArmed = false;
                }
            }
            if( !m_world->Upgrades().AnyUpgradable() )
                m_step = eStep::stage11;
        }
        return;
    }

    if( m_step == eStep::tutorial ) {
        // world frozen behind the panel; the audio loops keep breathing
        m_audio.Update();
        if( !m_pointer.Left() )
            m_clickArmed = true;
        else if( m_clickArmed ) {
            m_tutorialSeen.at( static_cast< size_t >( m_tutorialTopic ) ) = true;
            m_step = eStep::stage11;
        }
        return;
    }

    if( m_step == eStep::stageStory || m_step == eStep::epilogue ) {
        // a real pause: the world is frozen, click to engage the new stage (or free play)
        m_audio.Update();
        if( !m_pointer.Left() )
            m_clickArmed = true;
        else if( m_clickArmed ) {
            m_step = eStep::stage11;
            m_clickArmed = false;
        }
        return;
    }

    if( m_step == eStep::stage11 ) {
        const PlayerInput input{
            m_pointer.Cursor(),
            m_pointer.Left(),
            m_pointer.Right(),
        };
        // TEMPORARY test shortcut: F9 skips to just before the next phase (see World::DebugSkip)
        const auto f9{ m_windows.KeyPressed( Win32::Windows::eKey::f9 ) };
        if( f9 && !m_debugKeyDown )
            m_world->DebugSkip();
        m_debugKeyDown = f9;
        // TEMPORARY test shortcut: F10 toggles the invincibility
        const auto f10{ m_windows.KeyPressed( Win32::Windows::eKey::f10 ) };
        if( f10 && !m_debugInvincibleKeyDown )
            m_world->DebugToggleInvincible();
        m_debugInvincibleKeyDown = f10;
        m_world->Update( input );
        m_audio.Update();
        // let the explosion play out before the overlay:
        if( m_world->ShipDestroyed() && m_world->ShipDestroyedTicks() > m_frameRate * 2 ) {
            m_step = eStep::gameOver;
            m_clickArmed = false;
            if( m_world->OutOfLives() )
                m_audio.Play( eSound::gameOver );
            return;
        }
        // all the missions are cleared: the epilogue, then free play
        if( !m_world->ShipDestroyed() && !m_epilogueShown && m_world->Stage().Phase() == StageDirector::ePhase::freePlay ) {
            m_epilogueShown = true;
            m_step = eStep::epilogue;
            m_clickArmed = false;
            return;
        }
        // entering a new stage: pause on its scenario screen
        if( !m_world->ShipDestroyed() && m_world->Stage().Phase() == StageDirector::ePhase::intro && m_world->Stage().StageNumber() > m_storyStage ) {
            m_storyStage = m_world->Stage().StageNumber();
            m_step = eStep::stageStory;
            m_clickArmed = false;
            return;
        }
        // skill points to spend (earned on the debriefing, or kept over a retry):
        const auto phase{ m_world->Stage().Phase() };
        if( !m_world->ShipDestroyed() && ( phase == StageDirector::ePhase::cleared || phase == StageDirector::ePhase::intro ) && m_world->Upgrades().AnyUpgradable() ) {
            m_step = eStep::upgrade;
            m_clickArmed = false;
            return;
        }
        // first close encounter with something new: pause and explain it
        if( !m_world->ShipDestroyed() && _DetectTutorial() ) {
            m_step = eStep::tutorial;
            m_clickArmed = false;
        }
        return;
    }

    // game over: the world keeps living behind the overlay
    m_world->Update( {} );
    m_audio.Update();
    if( !m_pointer.Left() )
        m_clickArmed = true;
    else if( m_clickArmed ) {
        if( m_world->OutOfLives() )
            _NewGame();
        else
            _Restart();
    }
}


void Game::Draw( const NanoVGRenderer::Frame & _frame )
{
    if( m_step == eStep::stage11_prologue ) {
        m_scene.DrawBackdrop( *m_world, _frame );
        _DrawPrologue( _frame );
        _DrawUpdateNotice( _frame );
        _DrawCursor( _frame );
        return;
    }

    if( m_step == eStep::stageStory || m_step == eStep::epilogue ) {
        m_scene.DrawBackdrop( *m_world, _frame );
        _DrawStageStory( _frame );
        _DrawUpdateNotice( _frame );
        _DrawCursor( _frame );
        return;
    }

    m_scene.Draw( *m_world, _frame );
    if( m_step == eStep::gameOver ) {
        _DrawGameOver( _frame );
        _DrawUpdateNotice( _frame );
    }
    else {
        m_hud.Draw( *m_world, _frame );
        if( m_step == eStep::tutorial )
            _DrawTutorial( _frame );
        else if( m_step == eStep::upgrade )
            _DrawUpgrade( _frame );
    }
    _DrawCursor( _frame );
}


bool Game::_DetectTutorial()
{
    if( !m_tutorialEnabled )
        return false;
    const auto & ship{ m_world->Ship() };
    // the item must be comfortably INSIDE the visible screen - a tutorial pointing at
    // something the player cannot see teaches nothing:
    const Vector halfView{ m_view.logical.width * 0.5, m_view.logical.height * 0.5 };
    constexpr double margin{ 130 };
    const auto trigger{ [ & ]( const eTutorial _topic, const Vector & _position ){
            if( m_tutorialSeen.at( static_cast< size_t >( _topic ) ) )
                return false;
            const auto delta{ _position - ship.position };
            if( std::abs( delta.u ) > halfView.u - margin || std::abs( delta.v ) > halfView.v - margin )
                return false;
            m_tutorialTopic = _topic;
            m_tutorialTarget = _position;
            return true;
        } };
    for( const auto & enemy : m_world->Enemies() ) {
        if( enemy.IsBoss() ) // the alert already says it all
            continue;
        const auto topic{ enemy.type == Enemy::eType::wasp ? eTutorial::wasp
            : ( enemy.type == Enemy::eType::sniper ? eTutorial::sniper : eTutorial::chaser ) };
        if( trigger( topic, enemy.rocket.position ) )
            return true;
    }
    for( const auto & gravityMine : m_world->GravityMines() )
        if( gravityMine.alive && trigger( eTutorial::gravityMine, gravityMine.position ) )
            return true;
    for( const auto & attractor : m_world->Attractors().All() )
        if( attractor.Present() && trigger( eTutorial::attractor, attractor.position ) )
            return true;
    for( const auto & goody : m_world->Goodies() ) // one lesson per goody type
        if( trigger( static_cast< eTutorial >( static_cast< size_t >( eTutorial::goodyLaserUp ) + static_cast< size_t >( goody.type ) ), goody.position ) )
            return true;
    for( const auto & missile : m_world->Missiles() )
        if( !missile.fromShip && !missile.dead && trigger( eTutorial::missile, missile.rocket.position ) )
            return true;
    return false;
}


void Game::_DrawTutorial( const NanoVGRenderer::Frame & _frame )
{
    m_overlayAnim += 0.05;
    const auto & dimension{ m_view.logical };
    const Vector screen{ static_cast< double >( dimension.width ), static_cast< double >( dimension.height ) };
    const Vector screenCenter{ screen * 0.5 };

    // light veil, the frozen world stays visible behind:
    _frame.FillRectangle( {}, screen, { 0, 0.01, 0.03, 0.35 } );

    struct Topic
    {
        const char * title;
        const char * line1;
        const char * line2;
    };
    static const std::array< Topic, static_cast< size_t >( eTutorial::count ) > topics{ {
        { "HOSTILE CHASER", "It hunts you relentlessly and fires a homing missile every few seconds.", "Keep moving and shoot it down (+100)." },
        { "WASP", "Fast kamikaze chaff going straight for the ram.", "Fragile: one good hit pops it, and its blast can chain (+40)." },
        { "SNIPER", "It holds its distance and fires slugs at your PREDICTED position.", "Fly erratically to dodge - or close in, it is helpless up close (+150)." },
        { "GRAVITY MINE", "A hostile trap pulling everything nearby into its detonation.", "Shoot it from outside its pull (+15), or bait your pursuers through it." },
        { "STRANGE ATTRACTOR", "A gravity well: its pull is strong and touching it hurts.", "Crack it open (+25) or use its pull as a natural shield." },
        { "INCOMING MISSILE", "A homing missile is locked on you.", "Outrun it until it starves, shoot it down (+10), or drag it into a trap." },
        // goodies, in Goody::eType order:
        { "LASER POWER-UP (L)", "Boosts your main laser: first the fire rate, then the number of beams.", "Stack them up to the full 8-beam volley." },
        { "HOMING MISSILES (H)", "A pack of 30 missiles hunting the closest enemy.", "Fired automatically while you hold the trigger." },
        { "MAGNETIC MINES (M)", "A pack of 10 mines, dropped behind you and attracted by enemies.", "Lay a trap and lure your pursuers through it." },
        { "PLASMA SHIELD (S)", "5 seconds behind an energy dome that blocks hits and blasts.", "Enemies bounce off it - ram them while it lasts." },
        { "SHIELD REPAIR (blue S)", "Instantly restores half of your shield capacity.", "It only spawns when your shield is damaged." },
        { "PROPELLANT (P)", "Instantly refills half of your propellant tank.", "It only spawns when your tank is not full." },
        { "TURRET (T)", "A mini-turret orbits you for 10 seconds, sniping enemies, mines and missiles.", "High fire rate, lead compensation - durations stack." },
        { "REPULSOR (R)", "Instant shockwave: huge knockback, zero damage to you.", "It chain-triggers every missile and mine around - a panic button." },
        { "DECOY (D)", "Drops a beacon that enemy missiles chase instead of you.", "It survives 8 seconds or 3 hits." },
        { "EMP (E)", "Shuts down enemy engines and launchers for 4 seconds.", "They drift helplessly - into attractors, mines... or your lasers." },
        { "OVERDRIVE (O)", "8 seconds of free propellant and a boosted engine.", "Escape a gravity well or reposition across the field." },
        { "SINGULARITY (X)", "Deploys a micro black hole: everything spirals in for 3 seconds.", "It shreds what it catches, then collapses in a massive blast. Do not linger." },
        { "DEATH BLOSSOM (B)", "2 seconds of rotating 360-degree laser storm.", "Grab it when surrounded." },
        { "HELLSTORM (W)", "Instantly launches a spiral fan of 16 homing missiles.", "Watch the chain reactions." },
    } };
    const auto & topic{ topics.at( static_cast< size_t >( m_tutorialTopic ) ) };

    // target position first - it decides the panel placement:
    const auto & worldShip{ m_world->Ship() };
    Vector targetProbe{ m_tutorialTarget - worldShip.position + screenCenter };
    targetProbe.u = std::clamp( targetProbe.u, 50.0, screen.u - 50 );
    targetProbe.v = std::clamp( targetProbe.v, 50.0, screen.v - 50 );

    // the panel sits in the upper third, unless the item would hide behind it -
    // then it moves to the bottom so the item is always visible:
    constexpr double panelWidth{ 800 }, panelHeight{ 150 }, panelMargin{ 100 };
    const bool bottomPlacement{ targetProbe.u > screenCenter.u - panelWidth * 0.5 - 70 && targetProbe.u < screenCenter.u + panelWidth * 0.5 + 70
        && targetProbe.v < panelMargin + panelHeight + 90 };
    const double panelTop{ bottomPlacement ? screen.v - panelMargin - panelHeight : panelMargin };
    const Vector panelA{ screenCenter.u - panelWidth * 0.5, panelTop };
    const Vector panelB{ screenCenter.u + panelWidth * 0.5, panelTop + panelHeight };
    _frame.GradientRectangle( panelA, panelB, { 0.07, 0.12, 0.2, 0.85 }, { 0.02, 0.045, 0.09, 0.85 }, 10 );
    _frame.StrokeRectangle( panelA, panelB, { 0.35, 0.6, 1, 0.4 }, 1, 10 );
    _frame.Text( { screenCenter.u, panelTop + 30 }, "openSansBold", 24, topic.title, { 0.45, 0.72, 1 }, NanoVGRenderer::Frame::eTextAlign::center, 4 );
    _frame.Text( { screenCenter.u, panelTop + 64 }, "openSans", 17, topic.line1, { 0.85, 0.92, 1 }, NanoVGRenderer::Frame::eTextAlign::center );
    _frame.Text( { screenCenter.u, panelTop + 88 }, "openSans", 17, topic.line2, { 0.85, 0.92, 1 }, NanoVGRenderer::Frame::eTextAlign::center );
    const auto pulse{ ( std::sin( m_overlayAnim * 2 ) + 1 ) * 0.5 };
    _frame.Text( { screenCenter.u, panelTop + 122 }, "openSansBold", 13, std::format( "{} TO CONTINUE", _Click() ), { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 4 );

    // callout toward the concerned item, technical-drawing style: orthogonal thin
    // segments with joint dots, ending on a pulsing marker ring; the callout leaves
    // the panel from the edge facing the play area:
    const auto & target{ targetProbe };
    constexpr Color_d accent{ 0.45, 0.72, 1 };
    _frame.StrokeCircle( target, 34 + std::sin( m_overlayAnim * 3 ) * 5, { accent.r, accent.g, accent.b, 0.8 }, 2 );
    if( target.u > panelA.u - 40 && target.u < panelB.u + 40 && target.v > panelA.v - 70 && target.v < panelB.v + 70 )
        return; // the item still sits near the panel, the marker ring alone is enough

    const Color_d lineColor{ accent.r, accent.g, accent.b, 0.9 };
    constexpr double lineWidth{ 1.5 };
    const auto outward{ bottomPlacement ? -1.0 : 1.0 };
    const Vector p0{ screenCenter.u, bottomPlacement ? panelA.v : panelB.v };
    const Vector p1{ p0.u, p0.v + outward * 26 };
    _frame.Line( p0, p1, lineColor, lineWidth );
    _frame.FillCircle( p1, 2.2, lineColor );
    if( std::abs( target.v - p1.v ) < 60 ) {
        // item at elbow height: side approach, ending horizontally into the marker
        const auto side{ target.u >= p1.u ? -1.0 : 1.0 };
        const Vector end{ target.u + side * 46, p1.v };
        _frame.Line( p1, end, lineColor, lineWidth );
        _frame.Line( end, end + Vector{ side * 9, -6.0 }, lineColor, lineWidth );
        _frame.Line( end, end + Vector{ side * 9, 6.0 }, lineColor, lineWidth );
    }
    else {
        // horizontal run, then vertical approach into the marker:
        const Vector p2{ target.u, p1.v };
        const auto approach{ target.v >= p1.v ? 1.0 : -1.0 };
        const Vector end{ target.u, target.v - approach * 46 };
        _frame.Line( p1, p2, lineColor, lineWidth );
        _frame.FillCircle( p2, 2.2, lineColor );
        _frame.Line( p2, end, lineColor, lineWidth );
        _frame.Line( end, end + Vector{ -6.0, -approach * 9 }, lineColor, lineWidth );
        _frame.Line( end, end + Vector{ 6.0, -approach * 9 }, lineColor, lineWidth );
    }
}


std::pair< Vector, Vector > Game::_UpgradeCardRect( const size_t _index ) const
{
    // a row of cards in the lower part of the screen, the ship stays visible at the center:
    constexpr double cardWidth{ 250 }, cardHeight{ 150 }, gap{ 20 };
    constexpr double count{ static_cast< double >( ShipUpgrades::eSystem::count ) };
    const Vector screenCenter{ static_cast< double >( m_view.logical.width ) * 0.5, static_cast< double >( m_view.logical.height ) * 0.5 };
    const double left{ screenCenter.u - ( count * cardWidth + ( count - 1 ) * gap ) * 0.5 + static_cast< double >( _index ) * ( cardWidth + gap ) };
    const double top{ screenCenter.v + 140 };
    return { { left, top }, { left + cardWidth, top + cardHeight } };
}


void Game::_DrawUpgrade( const NanoVGRenderer::Frame & _frame )
{
    m_overlayAnim += 0.05;
    const auto & dimension{ m_view.logical };
    const Vector screen{ static_cast< double >( dimension.width ), static_cast< double >( dimension.height ) };
    const Vector screenCenter{ screen * 0.5 };
    const double pulse{ ( std::sin( m_overlayAnim * 2 ) + 1 ) * 0.5 };
    const auto & upgrades{ m_world->Upgrades() };

    // light veil, the frozen world stays visible behind:
    _frame.FillRectangle( {}, screen, { 0, 0.01, 0.03, 0.45 } );

    // title block in the upper part:
    const Vector titleCenter{ screenCenter - Vector{ 0, 250 } };
    _frame.Text( titleCenter - Vector{ 0, 55 }, "openSansBold", 15, "H A N G A R", { accentColor.r, accentColor.g, accentColor.b, 0.85 }, NanoVGRenderer::Frame::eTextAlign::center, 6 );
    _frame.Text( titleCenter + Vector{ 0, 3 }, "openSansBold", 56, "SHIP UPGRADE", { accentColor.r, accentColor.g, accentColor.b, 0.3 }, NanoVGRenderer::Frame::eTextAlign::center, 6 );
    _frame.Text( titleCenter, "openSansBold", 56, "SHIP UPGRADE", colorWhite, NanoVGRenderer::Frame::eTextAlign::center, 6 );
    _frame.Text( titleCenter + Vector{ 0, 55 }, "openSansBold", 16, std::format( "{} SKILL POINT{} TO ASSIGN", upgrades.points, upgrades.points > 1 ? "S" : "" ),
        { 0.85, 0.92, 1, 0.9 }, NanoVGRenderer::Frame::eTextAlign::center, 3 );

    // the ship itself (screen center) is the live preview: a pulsing ring frames it
    const auto & ship{ m_world->Ship() };
    _frame.StrokeCircle( screenCenter, ship.dynamic.boundingBoxRadius + 26 + pulse * 6, { accentColor.r, accentColor.g, accentColor.b, 0.6 }, 1.5 );

    struct Card
    {
        const char * title;
        const char * line1;
        const char * line2;
        bool heavier; // the drawback shows in red
    };
    static const std::array< Card, static_cast< size_t >( ShipUpgrades::eSystem::count ) > cards{ {
        { "SHIELD", "+20% capacity, faster repair", "thicker hull - heavier", true },
        { "PROPELLANT", "+15% tank, faster refuel", "bigger tank - heavier", true },
        { "ENGINE", "+15% thrust, quicker spool-up", "burns more propellant", false },
        { "ROTATORS", "+20% turn power", "sharper aiming", false },
    } };
    const auto cursor{ m_pointer.Cursor() };
    for( size_t i{ 0 }; i < cards.size(); i++ ) {
        const auto & card{ cards.at( i ) };
        const auto system{ static_cast< ShipUpgrades::eSystem >( i ) };
        const auto [ a, b ]{ _UpgradeCardRect( i ) };
        const bool enabled{ upgrades.CanUpgrade( system ) };
        const bool hover{ enabled && cursor.u >= a.u && cursor.u <= b.u && cursor.v >= a.v && cursor.v <= b.v };
        const double alpha{ enabled ? 1.0 : 0.45 };
        _frame.GradientRectangle( a, b, { 0.07, 0.12, 0.2, hover ? 0.95 : 0.75 }, { 0.02, 0.045, 0.09, hover ? 0.95 : 0.75 }, 10 );
        _frame.StrokeRectangle( a, b, { accentColor.r, accentColor.g, accentColor.b, hover ? 0.95 : 0.4 * alpha }, 1.5, 10 );
        const double centerU{ ( a.u + b.u ) * 0.5 };
        _frame.Text( { centerU, a.v + 28 }, "openSansBold", 20, card.title, hover ? colorWhite : Color_d{ 0.45, 0.72, 1, alpha }, NanoVGRenderer::Frame::eTextAlign::center, 3 );
        _frame.Text( { centerU, a.v + 60 }, "openSans", 15, card.line1, { 0.85, 0.92, 1, 0.9 * alpha }, NanoVGRenderer::Frame::eTextAlign::center );
        _frame.Text( { centerU, a.v + 82 }, "openSans", 15, card.line2,
            card.heavier ? Color_d{ dangerColor.r, dangerColor.g, dangerColor.b, 0.85 * alpha } : Color_d{ 0.62, 0.72, 0.88, 0.85 * alpha }, NanoVGRenderer::Frame::eTextAlign::center );

        // level pips:
        constexpr double pipGap{ 22 };
        const double pipLeft{ centerU - ( ShipUpgrades::maxLevel - 1 ) * pipGap * 0.5 };
        const int level{ upgrades.Level( system ) };
        for( int pip{ 0 }; pip < ShipUpgrades::maxLevel; pip++ ) {
            const Vector position{ pipLeft + pip * pipGap, b.v - 28 };
            if( pip < level )
                _frame.FillCircle( position, 6, { accentColor.r, accentColor.g, accentColor.b, 0.95 } );
            else if( pip == level && hover )
                _frame.FillCircle( position, 6, { 1, 1, 1, 0.35 + pulse * 0.5 } ); // the level about to be bought
            _frame.StrokeCircle( position, 6, { accentColor.r, accentColor.g, accentColor.b, 0.6 * alpha }, 1.2 );
        }
    }

    _frame.Text( screenCenter + Vector{ 0, 340 }, "openSansBold", 13, m_pointer.UsingGamepad() ? "AIM AT A SYSTEM AND PRESS A TO UPGRADE IT" : "CLICK A SYSTEM TO UPGRADE IT", { 1, 1, 1, 0.3 + pulse * 0.45 }, NanoVGRenderer::Frame::eTextAlign::center, 4 );
}


void Game::_DrawStory( const NanoVGRenderer::Frame & _frame, const std::string & _tag, const std::string & _title, const std::vector< const char * > & _lines )
{
    const auto & dimension{ m_view.logical };
    const Vector screenCenter{ static_cast< double >( dimension.width ) * 0.5, static_cast< double >( dimension.height ) * 0.5 };
    // layout: title block in the upper third, story in the middle, the choice or
    // call to action at the bottom - visually balanced over the full screen
    const Vector titleCenter{ screenCenter - Vector{ 0, 240 } };

    // chapter tag + title with a subtle accent glow:
    _frame.Text( titleCenter - Vector{ 0, 70 }, "openSansBold", 15, _tag, { accentColor.r, accentColor.g, accentColor.b, 0.85 }, NanoVGRenderer::Frame::eTextAlign::center, 6 );
    _frame.Text( titleCenter + Vector{ 0, 3 }, "openSansBold", 84, _title, { accentColor.r, accentColor.g, accentColor.b, 0.3 }, NanoVGRenderer::Frame::eTextAlign::center, 8 );
    _frame.Text( titleCenter, "openSansBold", 84, _title, colorWhite, NanoVGRenderer::Frame::eTextAlign::center, 8 );

    // divider:
    const double dividerY{ titleCenter.v + 70 };
    _frame.Line( { screenCenter.u - 320, dividerY }, { screenCenter.u + 320, dividerY }, { accentColor.r, accentColor.g, accentColor.b, 0.35 }, 1 );
    _frame.FillCircle( { screenCenter.u, dividerY }, 2.5, { accentColor.r, accentColor.g, accentColor.b, 0.8 } );

    // story:
    Vector verticalText{ 0, dividerY - screenCenter.v + 50 };
    for( const auto * text : _lines ) {
        _frame.Text( screenCenter + verticalText, "openSans", 22, text, { 0.72, 0.85, 1, 0.92 }, NanoVGRenderer::Frame::eTextAlign::center );
        verticalText.v += 34;
    }
}


void Game::_DrawStageStory( const NanoVGRenderer::Frame & _frame )
{
    m_overlayAnim += 0.05;
    const auto & stage{ m_world->Stage() };
    const Vector screenCenter{ static_cast< double >( m_view.logical.width ) * 0.5, static_cast< double >( m_view.logical.height ) * 0.5 };
    const double pulse{ ( std::sin( m_overlayAnim * 2 ) + 1 ) * 0.5 };
    if( m_step == eStep::epilogue ) {
        _DrawStory( _frame, "E P I L O G U E", "BELT SECURED", StageDirector::Epilogue() );
        const auto score{ std::format( "{:010}", m_world->GetHudInfo().score ) };
        _frame.Text( { screenCenter.u, screenCenter.v + 140 }, "openSansBold", 15, "F I N A L   S C O R E", { 0.62, 0.72, 0.88, 0.9 }, NanoVGRenderer::Frame::eTextAlign::center, 2 );
        _frame.Text( { screenCenter.u, screenCenter.v + 185 }, "sourceCodePro", 42, score, { 0.85, 0.92, 1 }, NanoVGRenderer::Frame::eTextAlign::center );
        _frame.Text( screenCenter + Vector{ 0, 330 }, "openSansBold", 16, std::format( "{} TO KEEP PLAYING  -  FREE PLAY", _Click() ), { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 5 );
        return;
    }
    _DrawStory( _frame, stage.StageTitle(), stage.Label(), stage.Story() );
    _frame.Text( screenCenter + Vector{ 0, 330 }, "openSansBold", 16, std::format( "{} TO ENGAGE", _Click() ), { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 5 );
}


void Game::_DrawPrologue( const NanoVGRenderer::Frame & _frame )
{
    m_overlayAnim += 0.05;
    const auto & dimension{ m_view.logical };
    const Vector screenCenter{ static_cast< double >( dimension.width ) * 0.5, static_cast< double >( dimension.height ) * 0.5 };
    _DrawStory( _frame, "M I S S I O N", m_world->Stage().Label(), m_world->Stage().Story() );

    // tutorial question with YES / NO buttons - answering engages the stage:
    _frame.Text( screenCenter + Vector{ 0, 250 }, "openSansBold", 16, "ENABLE THE IN-GAME TUTORIAL?", { 0.85, 0.92, 1, 0.9 }, NanoVGRenderer::Frame::eTextAlign::center, 3 );
    const auto cursor{ m_pointer.Cursor() };
    for( int i{ 0 }; i < 2; i++ ) {
        const auto [ a, b ]{ _PrologueButtonRect( i ) };
        const bool hover{ cursor.u >= a.u && cursor.u <= b.u && cursor.v >= a.v && cursor.v <= b.v };
        _frame.GradientRectangle( a, b, { 0.07, 0.12, 0.2, hover ? 0.95 : 0.7 }, { 0.02, 0.045, 0.09, hover ? 0.95 : 0.7 }, 10 );
        _frame.StrokeRectangle( a, b, { accentColor.r, accentColor.g, accentColor.b, hover ? 0.95 : 0.4 }, 1.5, 10 );
        _frame.Text( ( a + b ) * 0.5, "openSansBold", 18, i == 0 ? "YES" : "NO",
            hover ? colorWhite : Color_d{ 0.85, 0.92, 1, 0.85 }, NanoVGRenderer::Frame::eTextAlign::center, 2 );
    }
    const double pulse{ ( std::sin( m_overlayAnim * 2 ) + 1 ) * 0.5 };
    _frame.Text( screenCenter + Vector{ 0, 350 }, "openSansBold", 13, "CHOOSE TO ENGAGE", { 1, 1, 1, 0.3 + pulse * 0.45 }, NanoVGRenderer::Frame::eTextAlign::center, 4 );
}


std::pair< Vector, Vector > Game::_PrologueButtonRect( const int _index ) const
{
    const Vector screenCenter{ static_cast< double >( m_view.logical.width ) * 0.5, static_cast< double >( m_view.logical.height ) * 0.5 };
    const Vector center{ screenCenter.u + ( _index == 0 ? -95.0 : 95.0 ), screenCenter.v + 305 };
    return { center - Vector{ 65, 23 }, center + Vector{ 65, 23 } };
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
    const bool outOfLives{ m_world->OutOfLives() };
    const char * title{ outOfLives ? "GAME OVER" : "SHIP DESTROYED" };
    const Vector titleCenter{ screenCenter - Vector{ 0, 80 } };
    _frame.Text( titleCenter + Vector{ 0, 3 }, "openSansBold", 84, title, { dangerColor.r, dangerColor.g, dangerColor.b, 0.35 }, NanoVGRenderer::Frame::eTextAlign::center, 8 );
    _frame.Text( titleCenter, "openSansBold", 84, title, colorWhite, NanoVGRenderer::Frame::eTextAlign::center, 8 );

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
    if( outOfLives ) {
        _frame.Text( { screenCenter.u, dividerY + 160 }, "openSansBold", 14, "NO SHIPS LEFT  -  THE MISSION IS LOST", { dangerColor.r, dangerColor.g, dangerColor.b, 0.9 }, NanoVGRenderer::Frame::eTextAlign::center, 3 );
        _frame.Text( { screenCenter.u, dividerY + 195 }, "openSansBold", 16, std::format( "{} TO START A NEW GAME", _Click() ), { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 5 );
        return;
    }
    if( m_world->Stage().Phase() != StageDirector::ePhase::freePlay )
        _frame.Text( { screenCenter.u, dividerY + 160 }, "openSansBold", 14, std::format( "{} SHIP{} LEFT", m_world->Lives(), m_world->Lives() > 1 ? "S" : "" ), { 0.62, 0.72, 0.88, 0.9 }, NanoVGRenderer::Frame::eTextAlign::center, 3 );
    _frame.Text( { screenCenter.u, dividerY + 195 }, "openSansBold", 16, std::format( "{} TO RETRY  -  {}", _Click(), StageDirector::Label( m_world->Checkpoint().progress ) ), { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 5 );
}


bool Game::_UpdateNoticeVisible() const
{
    const bool menu{ m_step == eStep::stage11_prologue || m_step == eStep::stageStory || m_step == eStep::epilogue || m_step == eStep::gameOver };
    return menu && m_updateChecker.UpdateAvailable();
}


std::pair< Vector, Vector > Game::_UpdateNoticeRect() const
{
    const Vector center{ m_view.logical.width * 0.5, 52.0 };
    return { center - Vector{ 330, 30 }, center + Vector{ 330, 30 } };
}


void Game::_DrawUpdateNotice( const NanoVGRenderer::Frame & _frame )
{
    if( !_UpdateNoticeVisible() )
        return;
    constexpr Color_d gold{ 1, 0.78, 0.3 };
    const auto [ a, b ]{ _UpdateNoticeRect() };
    const auto cursor{ m_pointer.Cursor() };
    const bool hover{ cursor.u >= a.u && cursor.u <= b.u && cursor.v >= a.v && cursor.v <= b.v };
    const double pulse{ ( std::sin( m_overlayAnim * 2 ) + 1 ) * 0.5 };
    _frame.GradientRectangle( a, b, { 0.12, 0.09, 0.03, hover ? 0.95 : 0.8 }, { 0.05, 0.035, 0.01, hover ? 0.95 : 0.8 }, 10 );
    _frame.StrokeRectangle( a, b, { gold.r, gold.g, gold.b, hover ? 1.0 : 0.45 + pulse * 0.25 }, 1.5, 10 );
    const auto center{ ( a + b ) * 0.5 };
    _frame.Text( center - Vector{ 0, 11 }, "openSansBold", 15, std::format( "A NEW VERSION IS AVAILABLE: {}  (YOU PLAY {})", m_updateChecker.LatestVersion(), m_updateChecker.CurrentVersion() ),
        hover ? colorWhite : Color_d{ gold.r, gold.g, gold.b, 0.95 }, NanoVGRenderer::Frame::eTextAlign::center, 2 );
    _frame.Text( center + Vector{ 0, 12 }, "openSansBold", 12, std::format( "{} TO DOWNLOAD  -  github.com/arnaud-sintes/starship/releases/latest", _Click() ),
        hover ? colorWhite : Color_d{ 0.85, 0.92, 1, 0.75 }, NanoVGRenderer::Frame::eTextAlign::center, 1 );
}


std::string Game::_Click() const
{
    return m_pointer.UsingGamepad() ? "PRESS A" : "CLICK";
}


void Game::_DrawCursor( const NanoVGRenderer::Frame & _frame )
{
    const auto position{ m_pointer.Cursor() };
    const double pulse{ std::sin( m_cursorFlash += 0.15 ) };
    const Color_d color{ 0.45, 0.72 + pulse * 0.15, 1 };
    constexpr Color_d outline{ 0, 0.02, 0.08, 0.85 }; // dark under-stroke, keeps contrast over bright effects
    constexpr double tickInner{ 16 }, tickOuter{ 23 };

    // soft glow so the reticle pops over any backdrop:
    _frame.GradientCircle( position, 34, { 0.25, 0.55, 1, 0.3 + pulse * 0.08 }, { 0.1, 0.25, 1, 0 } );

    // reticle: ring, center dot and four ticks, dark-outlined then bright:
    const auto reticle{ [ & ]( const Color_d & _color, const double _strokeWidth ){
            _frame.StrokeCircle( position, 13, _color, _strokeWidth );
            _frame.Line( position + Vector{ 0, -tickInner }, position + Vector{ 0, -tickOuter }, _color, _strokeWidth );
            _frame.Line( position + Vector{ 0, tickInner }, position + Vector{ 0, tickOuter }, _color, _strokeWidth );
            _frame.Line( position + Vector{ -tickInner, 0 }, position + Vector{ -tickOuter, 0 }, _color, _strokeWidth );
            _frame.Line( position + Vector{ tickInner, 0 }, position + Vector{ tickOuter, 0 }, _color, _strokeWidth );
        } };
    reticle( outline, 4.5 );
    reticle( color, 2.2 );
    _frame.FillCircle( position, 3.4, outline );
    _frame.FillCircle( position, 2.2, color );
}
