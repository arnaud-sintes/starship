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
        // engage by answering the tutorial question (YES / NO buttons):
        if( !m_windows.LeftMouseButtonPressed() )
            m_clickArmed = true;
        else if( m_clickArmed ) {
            const auto cursor{ m_view.ToLogical( m_windows.CursorPosition() ) };
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

    if( m_step == eStep::tutorial ) {
        // world frozen behind the panel; the audio loops keep breathing
        m_audio.Update();
        if( !m_windows.LeftMouseButtonPressed() )
            m_clickArmed = true;
        else if( m_clickArmed ) {
            m_tutorialSeen.at( static_cast< size_t >( m_tutorialTopic ) ) = true;
            m_step = eStep::stage11;
        }
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
    else {
        m_hud.Draw( *m_world, _frame );
        if( m_step == eStep::tutorial )
            _DrawTutorial( _frame );
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
        const auto topic{ enemy.type == Enemy::eType::wasp ? eTutorial::wasp
            : ( enemy.type == Enemy::eType::sniper ? eTutorial::sniper : eTutorial::chaser ) };
        if( trigger( topic, enemy.rocket.position ) )
            return true;
    }
    for( const auto & gravityMine : m_world->GravityMines() )
        if( gravityMine.alive && trigger( eTutorial::gravityMine, gravityMine.position ) )
            return true;
    for( const auto & attractor : m_world->Attractors().All() )
        if( attractor.alive && trigger( eTutorial::attractor, attractor.position ) )
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
    _frame.Text( { screenCenter.u, panelTop + 122 }, "openSansBold", 13, "CLICK TO CONTINUE", { 1, 1, 1, 0.35 + pulse * 0.5 }, NanoVGRenderer::Frame::eTextAlign::center, 4 );

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


void Game::_DrawPrologue( const NanoVGRenderer::Frame & _frame )
{
    m_overlayAnim += 0.05;
    const auto & dimension{ m_view.logical };
    const Vector screenCenter{ static_cast< double >( dimension.width ) * 0.5, static_cast< double >( dimension.height ) * 0.5 };
    // layout: title block in the upper third, story in the middle, tutorial choice at
    // the bottom - visually balanced over the full screen
    const Vector titleCenter{ screenCenter - Vector{ 0, 240 } };

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

    // tutorial question with YES / NO buttons - answering engages the stage:
    _frame.Text( screenCenter + Vector{ 0, 250 }, "openSansBold", 16, "ENABLE THE IN-GAME TUTORIAL?", { 0.85, 0.92, 1, 0.9 }, NanoVGRenderer::Frame::eTextAlign::center, 3 );
    const auto cursor{ m_view.ToLogical( m_windows.CursorPosition() ) };
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
