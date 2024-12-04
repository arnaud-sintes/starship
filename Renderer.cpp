#include "Renderer.h"

// *** TODO list:
// - technical:
//      - properly condition timings in second to current fps
//      - choose a distance unit (not pixel), e.g.: closest enemy display
//      - move code logic inside classes
//      - Renderer is way too big, move things at the right places (classes)
//      - Rocket must be split
//      - optimize what can be optimize (perf)
//      - general code refactoring (no struct...)
// 
// - engine:
//      - add solar wind (waves) -> how to maintain a grid (with resolution) dealing with this kind of stuff
//          and quickly interpolate a value for a given object position (any rocket/laser?/goodie?/particule?/...?)
//      - add planets & rocks & mines with gravity/attraction
//      -> the attraction formula may be reused to deal with goodies attraction (quite naïve today)
//      -> rockets are subjected to all attraction forces
//      -> goodies are subjected to ship's attraction force
// 
// - functional:
//      - full screen option, "best fit" regarding current resolution option
//
//      - deal with shield / potential explosion/removal (game-over)
//      - add stages, with different ennemies (more variety) and specific choregraphies
//      - add special stages (asteroid field, mine field, rescue broken ship etc.)
//      - end of stage big boss
// 
//      - more special bonues (turel, plasma, external module ala r-type, allied ship etc.)
//      - non-guided missiles rotator goodie ?


// debugging purpose
//#define _NO_ENEMY
#define _ENEMY_COUNT 1 // 10
//#define _TESTING_GOODIES
#define _TESTING_ATTRACTORS

// TODO collision with attractors -> magnetic mines <- new goody
// TODO different attraction formula between "magnetic" and "gravity attraction"


Renderer::Renderer( const Win32::Windows & _windows, const Packer::Resources & _resources )
    : m_windows{ _windows }
    , m_starField{ m_windows.GetDimension() }
    , m_ship{ { 0.5, 0.75, 1 },  {}, Maths::PiHalf, { 0, -5 }, {}, 0, // initial momentum is debatable
        5, // damage
        { 5, 5, 0.01, 0.5 }, // shield
        { 20, 20, 0.01, 0.75 }, // propellant
        { 0, 0.5, false, 0.005, 0.01, 0.75, 0 }, // engine
        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } } // rotators
    , m_sounds{
        { eSound::lowFuelAlert, m_audioEngine.LoadSound( _resources.find( "lowFuelAlert.wav" )->second ) },
        { eSound::lowShieldAlert, m_audioEngine.LoadSound( _resources.find( "lowShieldAlert.wav" )->second ) },
        { eSound::spaceWind, m_audioEngine.LoadSound( _resources.find( "spaceWind.wav" )->second ) },
        { eSound::laserShot, m_audioEngine.LoadSound( _resources.find( "laserShot.wav" )->second ) },
        { eSound::laserCollision, m_audioEngine.LoadSound( _resources.find( "laserCollision.wav" )->second ) },
        { eSound::missileShot, m_audioEngine.LoadSound( _resources.find( "missileShot.wav" )->second ) },
        { eSound::missileRun, m_audioEngine.LoadSound( _resources.find( "missileRun.wav" )->second ) },
        { eSound::missileExplosion, m_audioEngine.LoadSound( _resources.find( "missileExplosion.wav" )->second ) },
        { eSound::shipCollision, m_audioEngine.LoadSound( _resources.find( "shipCollision.wav" )->second ) },
        { eSound::shipExplosion, m_audioEngine.LoadSound( _resources.find( "shipExplosion.wav" )->second ) },
        { eSound::shipRotationEngine, m_audioEngine.LoadSound( _resources.find( "shipRotationEngine.wav" )->second ) },
        { eSound::shipMainEngine, m_audioEngine.LoadSound( _resources.find( "shipMainEngine.wav" )->second ) },
        { eSound::laserPowerUp, m_audioEngine.LoadSound( _resources.find( "laserPowerUp.wav" )->second ) },
        { eSound::homingMissiles, m_audioEngine.LoadSound( _resources.find( "homingMissiles.wav" )->second ) },
        { eSound::homingMissilesOff, m_audioEngine.LoadSound( _resources.find( "homingMissilesOff.wav" )->second ) },
        { eSound::plasmaShield, m_audioEngine.LoadSound( _resources.find( "plasmaShield.wav" )->second ) },
        { eSound::plasmaShieldOff, m_audioEngine.LoadSound( _resources.find( "plasmaShieldOff.wav" )->second ) },
        { eSound::shieldRepair, m_audioEngine.LoadSound( _resources.find( "shieldRepair.wav" )->second ) },
        { eSound::propellantRefuel, m_audioEngine.LoadSound( _resources.find( "propellantRefuel.wav" )->second ) },
    }
    , m_plasmaShield{ 60 * 10 } // 10 seconds immunity at startup
{
    _SetupSound( eSound::spaceWind, m_ship, 0.2, true ).Play();
    _SetupSound( eSound::shipMainEngine, m_ship, 1.0, true, 0 ).Play();
    _SetupSound( eSound::shipRotationEngine, m_ship, 1.0, true, 0 ).Play();

    #ifndef _NO_ENEMY
    for( int i{ 0 }; i < _ENEMY_COUNT; i++ )
        _AddEnemy();
    #endif

    #ifdef _TESTING_GOODIES
    for( int i{ 0 }; i < 20; i++ )
        m_goodies.emplace_back( new Goody{ { 100.0 + i * 30, 300 }, Goody::eType::laserUp } );
    m_goodies.emplace_back( new Goody{ { 300, 200 }, Goody::eType::homingMissiles } );
    m_goodies.emplace_back( new Goody{ { 400, 200 }, Goody::eType::plasmaShield } );
    m_goodies.emplace_back( new Goody{ { 500, 200 }, Goody::eType::shieldAdd } );
    m_goodies.emplace_back( new Goody{ { 600, 200 }, Goody::eType::propellantAdd } );
    #endif

    #ifdef _TESTING_ATTRACTORS
    for( int i{ 0 }; i < 1000; i++ ) {
        const auto radius{ Maths::Random( 1, 2 ) };
        const double range{ 10000 };
        const double securityDistance{ 400 };
        L_generate:
        double x{ 0 }, y{ 0 };
        while( x > -securityDistance && x < securityDistance ) x = Maths::Random( -range, range );
        while( y > -securityDistance && y < securityDistance ) y = Maths::Random( -range, range );
        for( const auto & attractor : m_attractors )
            if( Maths::Collision( { x, y }, radius * m_attractorMassSizeRatio, attractor.position, attractor.mass * m_attractorMassSizeRatio ) )
                goto L_generate;
        m_attractors.emplace_back( Attractor{ { x, y }, radius } );
    }
    #endif
}


void Renderer::_AddEnemy()
{
    // enemy shield is correlated to current laser pass:
    const double shield{ static_cast< double >( m_laserPass ) };
    const auto minDistance{ Maths::Random( 0.5, 0.75 ) * static_cast< double >( std::max( m_windows.GetDimension().width, m_windows.GetDimension().height ) ) };
    m_enemies.emplace_back( new Enemy{ Rocket{ { 1, 0.5, 0.75 }, m_ship.position + Vector::From( Maths::Random( 0, Maths::Pi2 ), minDistance ), Maths::Random( 0, Maths::Pi2 ), {}, {}, 0,
            5, // damage
            { shield, shield, 0.001, 0.2 }, // shield
            { 10, 10, 0.05, Maths::Random( 0.1, 0.75 ) }, // propellant
            { 0, Maths::Random( 0.1, 0.5 ), false, 0.005, 0.01, Maths::Random( 0.2, 0.75 ), 0 }, // engine
            { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } }, // rotators
            static_cast< int >( Maths::Random( 0, 60 * 5 ) ),
            { m_sounds.find( eSound::laserCollision )->second, nullptr },
            { m_sounds.find( eSound::shipCollision )->second, nullptr },
            { m_sounds.find( eSound::shipExplosion )->second, nullptr },
            { m_sounds.find( eSound::shipMainEngine )->second, nullptr },
            { m_sounds.find( eSound::shipRotationEngine )->second, nullptr }
        } );
    _SetupSound( m_enemies.back()->sound_shipMainEngine, m_ship, 1.0, true, 0 ).Play();
    _SetupSound( m_enemies.back()->sound_shipRotationEngine, m_ship, 1.0, true, 0 ).Play();
}


void Renderer::Loop( const NanoVGRenderer::Frame & _frame )
{
    // reset:
    _Reset();

    // key mapping:
    _Keys();

    // update:
    _Update(); // TODO bypass when displaying stage information

    // draw:
    _Draw( _frame );
}


Rocket * Renderer::_ClosestEnemy( const Vector & _position )
{
    Rocket * pTarget{ nullptr };
    double minDistance{ std::numeric_limits< double >::infinity() };
    for( auto & enemy : m_enemies ) {
        const auto distance{ ( enemy->rocket.position - _position ).Distance() };
        if( distance >= minDistance )
            continue;
        minDistance = distance;
        pTarget = &enemy->rocket;
    }
    return pTarget;
}


void Renderer::_Keys()
{
    // activate ship burst:
    if( m_windows.RightMouseButtonPressed() )
        m_ship.ActivateThrust();

    // follow the mouse cursor:
    const auto screenCenter{ m_windows.GetDimension().As< Vector, double >() * 0.5 };
    m_ship.PointTo( Vector::From( m_windows.CursorPosition().ToType< double >() ) - screenCenter + m_ship.position, 0.5 );

    // laser:
    static int laserShotRate{ 0 };
    if( laserShotRate++ % static_cast< int >( m_laserSpeed ) == 0  && m_windows.LeftMouseButtonPressed() ) {
        m_sounds.find( eSound::laserShot )->second.Play();
        for( int i{ 0 }; i < static_cast< int >( m_laserPass ); i++ ) {
            static int alternateLazerPosition{ 0 };
            const auto rightSide{ ( alternateLazerPosition % 2 ) == 0 };
            const auto position{ Vector::From( m_ship.orientation + Maths::PiHalf * ( rightSide ? 1 : -1 ), m_ship.dynamic.boundingBoxRadius ) };
            const auto wave{ std::sin( static_cast< double >( alternateLazerPosition ) * 0.5 ) }; // wave speed
            const auto wideAngle{ ( rightSide ? 1 : -1 ) * ( Maths::PiHalf * wave * 0.05 ) }; // wave amplitude
            const auto momentum{ Vector::From( m_ship.orientation + Maths::Pi - wideAngle, 50 ) }; // 50px length
            m_lasers.emplace_back( new Laser{ m_ship.position - momentum + m_ship.dynamic.headPosition + position, momentum,
                0.2 } ); // damage
            alternateLazerPosition++;
        }
    }
    
    // shoot missile:
    static int missileShotRate{ 0 };
    if( m_homingMissiles > 0 && missileShotRate++ > 20 && m_windows.LeftMouseButtonPressed() ) {
        m_homingMissiles--;
        if( m_homingMissiles == 0 )
            m_sounds.find( eSound::homingMissilesOff )->second.Play();
        missileShotRate = 0;
        Vector motion{};
        if( _ClosestEnemy( m_ship.position ) == nullptr )
            motion = Vector::From( m_ship.orientation, -5 );
        const auto & missile{ m_missiles.emplace_back( new Missile{ Rocket{ { 0.5, 0.75, 1 }, m_ship.position, m_ship.orientation, motion, m_ship.momentum, 0,
            3, // damage
            { 1, 1, 0.01, 0.5 }, // shield
            { 10, 10, 0.005, 0.9 }, // propellant
            { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
            //{ { 0, 0 }, 0, { false, false }, 0, 0, 0, { 0, 0 } } }, // non-guided missiles rotator
            { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0.5, { 0, 0 } } }, // guided missiles rotator
            false, m_ship,
            { m_sounds.find( eSound::missileShot )->second, nullptr },
            { m_sounds.find( eSound::missileRun )->second, nullptr },
            { m_sounds.find( eSound::missileExplosion )->second, nullptr }
            } ) };
        missile->sound_shot.Play();
        _SetupSound( missile->sound_run, m_ship, 0.0, true ).Play();
    }
}


void Renderer::_Reset()
{
    // reset enemies data:
    for( auto & enemy : m_enemies )
        enemy->rocket.Reset();

    // reset missiles data:
    for( auto & missile : m_missiles )
        missile->rocket.Reset();

    // reset ship data:
    m_ship.Reset();
}


void Renderer::_AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size )
{
    const auto momentum{ Vector::From( _orientation, _speed ) + _direction };
    m_particules.emplace_back( new Particule{ _position, momentum, Maths::Random( 1.5, 3 ), _size } );
}


void Renderer::_AddExplosion( const Vector & _position, const Vector & _direction, const int _count )
{
    for( int i{ 0 }; i < _count; i++ )
        _AddParticule( _position, _direction, Maths::Random( 0, Maths::Pi2 ), Maths::Random( 0, 4 ), Maths::Random( 0.5, 3 ) );
}


void Renderer::_AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter )
{
    const int passCount{ _rate < 0.5 ? 1 : _maxPass };
    const double maxSize{ 0.5 + _rate * _limiter }; // [0.5, 0.5 to 1.5]
    const double maxSpeed{ 1 + _rate }; // [0.5, 1 to 2]
    const double maxWideness{ 0.25 + _rate * 0.75 * _limiter }; // [-0.25, +0.25] to [-1, +1]
    for( int i{ 0 }; i < passCount; i++ )
        _AddParticule( _position + _burst.position, {}, _burst.orientation + Maths::Random( -maxWideness, maxWideness ), Maths::Random( 0.5, maxSpeed ), Maths::Random( 0.5, maxSize ) );
}

    
void Renderer::_AddEnginesParticules( const Rocket & _rocket )
{
    if( _rocket.engine.thrust != 0 )
        _AddEngineParticules( _rocket.position, _rocket.dynamic.engine, _rocket.engine.thrust / _rocket.engine.power, 2, 1 );
    const auto & leftThrust{ _rocket.rotator.thrust.at( Rocket::Rotator::left ) };
    if( leftThrust != 0 ) {
        const auto & rotator{ _rocket.dynamic.rotators.at( Rocket::Rotator::left ) };
        _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::left ), leftThrust / _rocket.rotator.power, 1, 0.5 );
        _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::right ), leftThrust / _rocket.rotator.power, 1, 0.5 );
    }
    const auto & rightThrust{ _rocket.rotator.thrust.at( Rocket::Rotator::right ) };
    if( rightThrust != 0 ) {
        const auto & rotator{ _rocket.dynamic.rotators.at( Rocket::Rotator::right ) };
        _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::left ), rightThrust / _rocket.rotator.power, 1, 0.5 );
        _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::right ), rightThrust / _rocket.rotator.power, 1, 0.5 );
    }
}


bool Renderer::_RocketCollision( const Rocket & _a, const Rocket & _b )
{
    return Maths::Collision( _a.position, _a.dynamic.boundingBoxRadius, _b.position, _b.dynamic.boundingBoxRadius );
}


void Renderer::_TransmitMomentum( const Vector & _position, const Vector & _momentum, const double _impact, Rocket & _b )
{
    _b.thrustMotion += _momentum;
    const auto normalizedEnemyOrientation{ Vector::From( _b.orientation, 1 ).Orientation() };
    const auto impactOrientation{ ( _position - _b.position ).Orientation() };
    _b.rotationMomentum += _impact * Maths::NormalizeAngle( normalizedEnemyOrientation - impactOrientation );
}


void Renderer::_TransmitMomentum( Rocket & _a, Rocket & _b )
{
    _TransmitMomentum( _a.position, _a.momentum, 0.05 * ( _a.dynamic.totalMass / 15 ), _b );
}


void Renderer::_LaserImpact( Laser & _a, Rocket & _b )
{   
    const auto momentum{ _a.momentum * 0.01 };
    // transmit momentum:
    _TransmitMomentum( _a.position, momentum, 0.05, _b );
    // add small impact explosion on target:
    _AddExplosion( _b.position, Vector{} - momentum, smallExplosion );
    // shield impact:
    _b.shield.value -= _a.damage;
}


void Renderer::_RocketImpact( Rocket & _a, Rocket & _b )
{   
    // transmit momentum:
    _TransmitMomentum( _a, _b );
    // add small impact explosion on target:
    _AddExplosion( _b.position, Vector{} - _a.momentum, smallExplosion );
    // shield impact:
    _b.shield.value -= _a.damage;
}


bool Renderer::_LaserRocketCollision( Laser & _laser, Rocket & _other )
{
    if( !Maths::Collision( _other.position, _other.dynamic.boundingBoxRadius, _laser.dynamic.positionA, _laser.dynamic.positionB ) )
        return false;
    _LaserImpact( _laser, _other );
    return true;
}


bool Renderer::_MissileRocketCollision( Missile & _missile, Rocket & _other )
{
    // prevent collision when launched:
    if( &_missile.origin == &_other && !_missile.bypassCollision ) {
        if( !Maths::Collision( _missile.rocket.position, _missile.rocket.dynamic.boundingBoxRadius, _other.position, _other.dynamic.boundingBoxRadius ) )
            _missile.bypassCollision = true;
        return false;
    }
    if( !_RocketCollision( _missile.rocket, _other ) )
        return false;
    _RocketImpact( _missile.rocket, _other );
    return true;
}


MiniAudio::Sound & Renderer::_SetupSound( MiniAudio::Sound & _sound, const Rocket & _rocket, const double _pitch, const bool _loop, const std::optional< double > & _volume )
{
    const auto relativePosition{ _rocket.position - m_ship.position };
    const double maxDistance{ 2000 };
    const auto volume{ _volume ? *_volume : 1.0 - std::min( relativePosition.Distance() / maxDistance, 0.9 ) };
    const auto pan{ relativePosition.u / 500 };
    _sound.Setup( volume, std::clamp( pan, -1.0, 1.0 ), _pitch, _loop );
    return _sound;
}


MiniAudio::Sound & Renderer::_SetupSound( eSound _sound, const Rocket & _rocket, const double _pitch, const bool _loop, const std::optional< double > & _volume )
{
    return _SetupSound( m_sounds.find( _sound )->second, _rocket, _pitch, _loop, _volume );
}


void Renderer::_QueueSoundPlay( MiniAudio::Sound & _sound )
{
    m_soundQueue.emplace_back( Sound{ _sound, 60 } );
    m_soundQueue.back().sound.Play();
}


void Renderer::_PurgeSoundQueue()
{
    for( auto it{ m_soundQueue.begin() }; it != m_soundQueue.cend(); )
    {
        if( --it->lifeSpan == 0 ) {
            it = m_soundQueue.erase( it );
            continue;
        }
        ++it;
    }
}


void Renderer::_Goody( const Goody::eType _type )
{
    m_score++;

    if( _type == Goody::eType::laserUp ) {
        const auto currentLaserSpeed{ m_laserSpeed };
        const auto currentLaserPass{ m_laserPass };
        if( m_laserSpeed == eLaserSpeed::slow )         m_laserSpeed = eLaserSpeed::medium;
        else if( m_laserSpeed == eLaserSpeed::medium )  m_laserSpeed = eLaserSpeed::fast;
        else
        if( m_laserSpeed == eLaserSpeed::fast ) {
            m_laserSpeed = eLaserSpeed::slow;
            if( m_laserPass == eLaserPass::one )            m_laserPass = eLaserPass::two;
            else if( m_laserPass == eLaserPass::two )       m_laserPass = eLaserPass::four;
            else if( m_laserPass == eLaserPass::four )      m_laserPass = eLaserPass::six;
            else if( m_laserPass == eLaserPass::six )       m_laserPass = eLaserPass::height;
            else if( m_laserPass == eLaserPass::height )    m_laserSpeed = eLaserSpeed::fast;
        }
        if( currentLaserSpeed != m_laserSpeed || currentLaserPass != m_laserPass )
            m_sounds.find( eSound::laserPowerUp )->second.Play();
        return;
    }
    if( _type == Goody::eType::homingMissiles ) {
        m_homingMissiles += 20; // 20x missiles pack
        m_sounds.find( eSound::homingMissiles )->second.Play();
        return;
    }
    if( _type == Goody::eType::plasmaShield ) {
        m_plasmaShield += 60 * 5; // 5 seconds plasma shield
        m_sounds.find( eSound::plasmaShield )->second.Play();
        return;
    }
    if( _type == Goody::eType::shieldAdd ) {
        if( m_ship.shield.value >= m_ship.shield.capacity )
            return;
        const auto capacityBoost{ m_ship.shield.capacity * 0.5 }; // 50% capacity boost
        m_ship.shield.value += capacityBoost;
        if( m_ship.shield.value > m_ship.shield.capacity )
            m_ship.shield.value = m_ship.shield.capacity;
        m_sounds.find( eSound::shieldRepair )->second.Play();
        return;
    }
    if( _type == Goody::eType::propellantAdd ) {
        if( m_ship.propellant.value >= m_ship.propellant.capacity )
            return;
        const auto capacityBoost{ m_ship.propellant.capacity * 0.5 }; // 50% capacity boost
        m_ship.propellant.value += capacityBoost;
        if( m_ship.propellant.value > m_ship.propellant.capacity )
            m_ship.propellant.value = m_ship.propellant.capacity;
        m_sounds.find( eSound::propellantRefuel )->second.Play();
        return;
    }
}


void Renderer::_Update()
{
    // plasma shield:
    if( m_plasmaShield > 0 ) {
        m_plasmaShield--;
        if( m_plasmaShield == 0 )
            m_sounds.find( eSound::plasmaShieldOff )->second.Play();
    }
    m_plasmaShieldIncrement += 4;
    if( m_plasmaShieldIncrement > 100 )
        m_plasmaShieldIncrement = 0;
    m_plasmaShieldRamp = ( m_plasmaShieldIncrement * m_plasmaShieldIncrement ) / 10000;
    m_plasmaShieldRadius = 30 + ( m_plasmaShieldRamp * 70 ); // maxium radius of 100

    // enemies collision:
    for( auto & enemy : m_enemies ) {
        // enemies-enemies collisions:
        for( auto & enemyOther : m_enemies )
            if( &enemy != &enemyOther )
                if( _RocketCollision( enemy->rocket, enemyOther->rocket ) ) {
                    _SetupSound( enemy->sound_collision, enemy->rocket ).Play();
                    _RocketImpact( enemy->rocket, enemyOther->rocket );
                }
        // enemy-ship collision:
        if( _RocketCollision( enemy->rocket, m_ship ) ) {
            _SetupSound( enemy->sound_collision, enemy->rocket ).Play();
            _RocketImpact( enemy->rocket, m_ship );
            _RocketImpact( m_ship, enemy->rocket );
        }
        // enemy-plasma shield collision:
        if( m_plasmaShield > 0 && Maths::Collision( enemy->rocket.position, enemy->rocket.dynamic.boundingBoxRadius, m_ship.position, m_plasmaShieldRadius ) ) {
            _SetupSound( enemy->sound_collision, enemy->rocket ).Play();
            _RocketImpact( m_ship, enemy->rocket );
            m_score++;
        }
        // enemy-attractors collision:
        for( const auto & attractor : m_attractors )
            if( Maths::Collision( enemy->rocket.position, enemy->rocket.dynamic.boundingBoxRadius, attractor.position, attractor.mass * m_attractorMassSizeRatio ) )
                enemy->rocket.shield.value = -1;
    }

    // laser collision:
    for( auto & laser : m_lasers ) {
        Rocket * pCollision{ nullptr };
        // laser-ships collisions:
        for( auto itEnemy{ m_enemies.begin() }, itEnemyEnd{ m_enemies.end() }; pCollision == nullptr && itEnemy != itEnemyEnd; ++itEnemy ) {
            if( _LaserRocketCollision( *laser, ( **itEnemy ).rocket ) ) {
                pCollision = &( **itEnemy ).rocket;
                _SetupSound( ( **itEnemy ).sound_laserCollision, *pCollision ).Play();
                m_score += 5;
            }
        }
        // laser-missiles collisions:
        for( auto itMissile{ m_missiles.begin() }; pCollision == nullptr && itMissile != m_missiles.end(); ) {
            auto & missile{ **itMissile };
            if( &missile.origin != &m_ship ) // laser don't destroy ship's missiles
                if( _LaserRocketCollision( *laser, missile.rocket ) ) {
                    pCollision = &missile.rocket;
                    _AddExplosion( laser->position, missile.rocket.momentum );
                    itMissile = m_missiles.erase( itMissile );
                    m_score += 10;
                    continue;
                }
            ++itMissile;
        }
        if( pCollision != nullptr )
            laser->lifeSpan = laser->maxLifeSpan;
        // laser-attractors collision:
        for( const auto & attractor : m_attractors )
            if( Maths::Collision( attractor.position, attractor.mass * m_attractorMassSizeRatio, laser->dynamic.positionA, laser->dynamic.positionB ) )
                laser->lifeSpan = laser->maxLifeSpan;
                // TODO sound & particles?
    }
        
    // missiles collision:
    for( auto it{ m_missiles.begin() }; it != m_missiles.end(); ) {
        auto & rocket{ ( **it ).rocket };
        bool collision{ false };
        // missiles-missiles collisions:
        for( auto itMissile{ m_missiles.begin() }; !collision && itMissile != m_missiles.end(); ) {
            auto & otherRocket{ ( **itMissile ).rocket };
            if( it != itMissile )
                if( _RocketCollision( rocket, otherRocket ) ) {
                    collision = true;
                    _AddExplosion( otherRocket.position, otherRocket.momentum );
                    itMissile = m_missiles.erase( itMissile );
                    m_score++;
                    continue;
                }
            ++itMissile;
        }
        // missiles-enemies collisions:
        for( auto itEnemy{ m_enemies.begin() }, itEnemyEnd{ m_enemies.end() }; !collision && itEnemy != itEnemyEnd; ++itEnemy ) {
            collision = _MissileRocketCollision( **it, ( **itEnemy ).rocket );
        }
        // missiles-ship collision:
        if( !collision )
            collision = _MissileRocketCollision( **it, m_ship );
        // missiles-plasma shield collision:
        if( !collision && &it->get()->origin != &m_ship && m_plasmaShield > 0 )
            collision = Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, m_ship.position, m_plasmaShieldRadius );
        // missiles-attractors collision:
        for( const auto & attractor : m_attractors )
            if( Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, attractor.position, attractor.mass * m_attractorMassSizeRatio ) )
                collision = true;
        // explode and remove:
        if( collision ) {
            _SetupSound( ( **it ).sound_run, rocket ).Stop();
            _QueueSoundPlay( _SetupSound( ( **it ).sound_explosion, rocket ) );
            _AddExplosion( rocket.position, rocket.momentum );
            it = m_missiles.erase( it );
            m_score++;
            continue;
        }
        ++it;
    }        
        
    // update enemies data:
    int newEnemiesToGenerate{ 0 };
    for( auto it{ m_enemies.begin() }; it != m_enemies.end(); ) {
        auto & enemy{ **it };
        _SetupSound( enemy.sound_shipMainEngine, enemy.rocket, 1.0, true, enemy.rocket.engine.thrust / enemy.rocket.engine.power );
        const auto rotatorVolume{ ( enemy.rocket.rotator.thrust.at( 0 ) + enemy.rocket.rotator.thrust.at( 1 ) ) / ( 2 * enemy.rocket.rotator.power ) };
        _SetupSound( enemy.sound_shipRotationEngine, enemy.rocket, 1.0, true, rotatorVolume > 0.25 ? ( rotatorVolume - 0.25 ) * 1.33 : 0 );

        // better NOT aim directly the ship to avoid collisions:
        enemy.rocket.Acquire( m_ship, 0.5, Vector::From( m_ship.orientation + Maths::Pi, 100 ) );
        enemy.rocket.ActivateThrust();
        enemy.rocket.Update();

        // shield:
        if( enemy.rocket.shield.value <= 0 ) {
            // 50% chance of goody addition, only if far enough (avoid volontary collision bonuses...)
            if( ( enemy.rocket.position - m_ship.position ).Distance() > 100 && Maths::Random( 0, 1 ) < 0.5 ) { 
                std::vector< Goody::eType > types{ Goody::eType::homingMissiles, Goody::eType::plasmaShield };
                if( m_ship.shield.value < m_ship.shield.capacity ) types.emplace_back( Goody::eType::shieldAdd );
                if( m_ship.propellant.value < m_ship.propellant.capacity ) types.emplace_back( Goody::eType::propellantAdd );
                if( m_laserSpeed != eLaserSpeed::fast || m_laserPass != eLaserPass::height ) types.emplace_back( Goody::eType::laserUp );
                const auto typeRandom{ Maths::Random( 0, static_cast< double >( types.size() ) - 0.01 ) };
                m_goodies.emplace_back( new Goody{ enemy.rocket.position, types.at( static_cast< int >( typeRandom ) ) } );
            }
            _QueueSoundPlay( _SetupSound( enemy.sound_explosion, enemy.rocket ) );
            _AddExplosion( enemy.rocket.position, enemy.rocket.momentum, bigExplosion );
            newEnemiesToGenerate++;
            it = m_enemies.erase( it );
            m_score += 50;
            continue;
        }

        // enemies launch rockets aiming the ship:
        if( enemy.shotRate++ > 60 * 5 ) { // every 5 seconds
            enemy.shotRate = 0;
            const auto & missile{ m_missiles.emplace_back( new Missile{ Rocket{ { 1, 0.5, 0.75 }, enemy.rocket.position, enemy.rocket.orientation, {}, enemy.rocket.momentum, 0,
                3, // damage
                { 1, 1, 0.01, 0.5 }, // shield
                { 10, 10, 0.01, 0.9 }, // propellant
                { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
                { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0.5, { 0, 0 } } },
                true, enemy.rocket,
                { m_sounds.find( eSound::missileShot )->second, nullptr },
                { m_sounds.find( eSound::missileRun )->second, nullptr },
                { m_sounds.find( eSound::missileExplosion )->second, nullptr }
                } ) };
            missile->sound_shot.Play();
            _SetupSound( missile->sound_run, enemy.rocket, 0.0, true ).Play();
        }
        ++it;
    }
    for( int i{ 0 }; i < newEnemiesToGenerate; i++ )
        _AddEnemy();

    // goodies collision:
    for( auto it{ m_goodies.begin() }; it != m_goodies.end(); ) {
        auto & goody{ **it };
        if( Maths::Collision( m_ship.position, m_ship.dynamic.boundingBoxRadius, goody.position, goody.dynamic.radius ) ) {
            _Goody( goody.type );
            it = m_goodies.erase( it );
            continue;
        }
        ++it;
    }

    // update laser data:
    for( auto it{ m_lasers.begin() }; it != m_lasers.end(); ) {
        auto & laser{ **it };
        if( laser.lifeSpan++ >= laser.maxLifeSpan ) {
            it = m_lasers.erase( it );
            continue;
        }
        // regular update:
        laser.Update();
        ++it;
    }

    // update missiles data:
    for( auto it{ m_missiles.begin() }; it != m_missiles.end(); ) {
        auto & missile{ **it };
        // running:
        if( missile.lifeSpan == 0 ) {
            _SetupSound( missile.sound_run, missile.rocket, missile.rocket.engine.thrust / missile.rocket.engine.power, true );
            // acquiring proper target:
            const auto pTarget{ missile.targetShip ? &m_ship : _ClosestEnemy( missile.rocket.position ) };
            if( pTarget != nullptr ) {
                missile.rocket.Acquire( *pTarget, 0.1 );
                missile.rocket.ActivateThrust();
            }
            // stopping when out of propellant:
            if( missile.rocket.propellant.value <= missile.rocket.propellant.production_rate ) {
                missile.lifeSpan = 1;
                missile.sound_run.Stop();
            }
        }
        // stopped:
        else {
            missile.lifeSpan++;
            // explode and remove after one second of drift:
            if( missile.lifeSpan == 60 ) {
                _QueueSoundPlay( _SetupSound( missile.sound_explosion, missile.rocket ) );
                _AddExplosion( missile.rocket.position, missile.rocket.momentum );
                it = m_missiles.erase( it );
                continue;
            }
        }
        // regular update:
        missile.rocket.Update();
        ++it;
    }

    // goodies attraction:
    for( const auto & goody : m_goodies )
        goody->position += m_ship.position.InfiniteAttraction( goody->position, m_ship.dynamic.totalMass );

    // update ship data:
    m_ship.Update();

    // attractions
    std::list< std::reference_wrapper< Rocket > > rockets;
    rockets.emplace_back( m_ship );
    for( auto & enemy : m_enemies )
        rockets.emplace_back( enemy->rocket );
    for( auto & missile : m_missiles )
        rockets.emplace_back( missile->rocket );
    for( auto & rrocket : rockets ) {
        Rocket & rocket{ rrocket.get() };
        Vector attraction;
        const double precisionFactor{ 1000 }; // because of cumulative small additions
        for( const auto & attractor : m_attractors )
            attraction += ( attractor.position.ProximityAttraction( rocket.position, attractor.mass * rocket.dynamic.totalMass ) * precisionFactor );
        attraction *= ( 1 / precisionFactor );
        rocket.momentum += attraction;
        rocket.position += attraction;
    }

    if( m_ship.shield.value < 0 )
        m_ship.shield.value = 0;
    if( m_ship.shield.value < ( m_ship.shield.capacity * 0.25 ) ) {
        if( !m_shieldAlert ) {
            m_shieldAlert = true;
            m_sounds.find( eSound::lowShieldAlert )->second.Play();
        }
    }
    else
        m_shieldAlert = false;

    if( m_ship.propellant.value < ( m_ship.propellant.capacity * 0.25 ) ) {
        if( !m_fuelAlert ) {
            m_fuelAlert = true;
            m_sounds.find( eSound::lowFuelAlert )->second.Play();
        }
    }
    else
        m_fuelAlert = false;

    _SetupSound( eSound::shipMainEngine, m_ship, 1.0, true, m_ship.engine.thrust / m_ship.engine.power );
    const auto rotatorVolume{ ( m_ship.rotator.thrust.at( 0 ) + m_ship.rotator.thrust.at( 1 ) ) / ( 2 * m_ship.rotator.power ) };
    _SetupSound( eSound::shipRotationEngine, m_ship, 1.0, true, rotatorVolume > 0.25 ? ( rotatorVolume - 0.25 ) * 1.33 : 0 );

    // add particules:
    for( auto & enemy : m_enemies )
        _AddEnginesParticules( enemy->rocket );
    for( auto & missile : m_missiles )
        _AddEnginesParticules( missile->rocket );
    _AddEnginesParticules( m_ship );

    // update particules data:
    for( auto it{ m_particules.begin() }; it != m_particules.end(); ) {
        auto & particule{ **it };
        particule.lifeSpan -= 0.09; // quickly reduce particules lifespan
        if( particule.lifeSpan <= 0 ) {
            it = m_particules.erase( it );
            continue;
        }
        particule.position += particule.momentum;
        ++it;
    }

    _PurgeSoundQueue();
}


void Renderer::_Draw( const NanoVGRenderer::Frame & _frame )
{
    const Vector solarWind{ 0.05, 0.2 }; // TODO create a very subtile movement, even when not moving (avoid fixed starfield positions)
    // draw starfield, related to ship motion & solar wind:
    m_starField.Draw( _frame, m_ship.momentum * 2 + solarWind );
    _SetupSound( eSound::spaceWind, m_ship, std::max( std::min( m_ship.momentum.Distance() / 20, 1.0 ), 0.2 ), true );

    const Vector screen{ static_cast< double >( m_windows.GetDimension().width ), static_cast< double >( m_windows.GetDimension().height ) };
    const Vector screenCenter{ screen * 0.5 };
    
    // stage information:
    // TODO
    //_frame.Text( screenCenter, "openSansBold", 80, "STAGE 1-1", colorWhite, NanoVGRenderer::Frame::eTextAlign::center );
    // TODO details
    //return; // TODO bypass when displaying stage information

    const Vector shipPosition{ screenCenter - m_ship.position };

    // draw particules:
    for( auto & particule : m_particules )
        _frame.FillCircle( particule->position + shipPosition, particule->width, Color_d::FireColor( ( 3 - particule->lifeSpan ) / 3 ) );

    // draw goodies:
    for( auto & goody : m_goodies )
        goody->Draw( _frame, shipPosition );

    // draw enemies:
    Rocket * pTarget{ _ClosestEnemy( m_ship.position ) };
    for( auto & enemy : m_enemies ) {
        if( pTarget == &enemy->rocket ) {
            _frame.Line( screenCenter, enemy->rocket.position + shipPosition, { 1, 0.5, 0.1 }, 0.75 );
            const auto vector{ enemy->rocket.position - m_ship.position };
            const auto distance{ static_cast< int >( vector.Distance() ) };
            if( distance > 500 ) { // 500 is "close"
                const auto position{ Vector::From( vector.Orientation(), m_ship.dynamic.boundingBoxRadius + 50 ) };
                _frame.Text( position + screenCenter, "openSans", 14, std::to_string( distance / 10 ), { 1, 0.75, 0.5 } );
            }
        }
        enemy->rocket.Draw( _frame, shipPosition );
    }

    // draw lasers:
    for( auto & laser : m_lasers )
        laser->Draw( _frame, shipPosition );

    // draw missiles:
    for( auto & missile : m_missiles )
        missile->rocket.Draw( _frame, shipPosition );

    // draw attractors:
    for( const auto & attractor : m_attractors ) {
        const auto position{ attractor.position + shipPosition };
        const auto radius{ attractor.mass * m_attractorMassSizeRatio };
        // color depends of distance with ship:
        const auto distance{ ( attractor.position - m_ship.position ).Distance() };
        const double maxDistance{ 800 };
        std::vector< double > colors;
        for( int i{ 0 }; i < 3; i++ ) {
            const auto currMaxDistance{ maxDistance / ( i + 1 ) };
            colors.emplace_back( distance < currMaxDistance ? ( std::pow( 1 - ( distance / currMaxDistance ), 2 ) ) : 0 );
        }
        _frame.FillCircle( position, radius, Color_d{ colors.at( 2 ), colors.at( 1 ), colors.at( 0 ) } );
        _frame.StrokeCircle( position, radius, Color_d{ 0.25, 0.5, 1 }, 2 );
    }

    // plasma  shield:
    const auto plasmaShieldSin{ std::sin( m_plasmaShieldRamp * Maths::Pi ) };
    const auto plasmaShieldColor{ Color_d{ 0.5, 1, 0.75 } * plasmaShieldSin };
    if( m_plasmaShield > 0 )
        _frame.StrokeCircle( screenCenter, m_plasmaShieldRadius, plasmaShieldColor, 2 * plasmaShieldSin );

    // draw ship:
    m_ship.Draw( _frame, shipPosition );

    // plasma shield reflection:
    if( m_plasmaShield > 0 )
        _frame.Reflect( screenCenter, m_plasmaShieldRadius, plasmaShieldColor, 0.4, m_plasmaShieldReflectAnimation += 0.3 );

    // display informations:
    _DisplayInfos( _frame );

    // display score:
    constexpr double margin{ 8 };
    _frame.Text( { static_cast< double >( m_windows.GetDimension().width ) - margin, margin }, "openSansBold", 24, std::format( "{:010}", m_score ), colorWhite, NanoVGRenderer::Frame::eTextAlign::topRight );

    // cursor:
    static double cursorFlash{ 0 };
    _frame.StrokeCircle( m_windows.CursorPosition().ToType< double >(), 15, { 0.25, 0.5 + std::sin( cursorFlash += 0.15 ) * 0.25, 1 }, 4 );
}


void Renderer::_DisplayInfos( const NanoVGRenderer::Frame & _frame )
{
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
    const auto shieldRate{ std::round( m_ship.shield.value * barWidth ) / m_ship.shield.capacity };
    _frame.FillRectangle( { xMenu, yMenu }, { xMenu + shieldRate, yMenu + barHeight }, Color_d::FireColor( 1 - ( shieldRate / barWidth ) ) );
    _frame.StrokeRectangle( { xMenu, yMenu }, { xMenu + barWidth, yMenu + barHeight }, colorWhite, strokeWidth, borderRadius );

    // propellant state:
    yMenu = margin + barHeight + spacing;
    _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Propellant:", colorWhite );
    const auto propellantdRate{ std::round( m_ship.propellant.value * barWidth ) / m_ship.propellant.capacity };
    _frame.FillRectangle( { xMenu, yMenu }, { xMenu + propellantdRate, yMenu + barHeight }, Color_d::FireColor( 1 - ( propellantdRate / barWidth ) ) );
    _frame.StrokeRectangle( { xMenu, yMenu }, { xMenu + barWidth, yMenu + barHeight }, colorWhite, strokeWidth, borderRadius );

    // laser power state:
    const int laserSpeed{ ( m_laserSpeed == eLaserSpeed::slow ) ? 0 : ( ( m_laserSpeed == eLaserSpeed::medium ) ? 1 : 2 ) };
    const int laserPass{ ( m_laserPass == eLaserPass::one ) ? 0 : ( m_laserPass == eLaserPass::two ? 1 : ( m_laserPass == eLaserPass::four ? 2 : ( m_laserPass == eLaserPass::six ? 3 : 4 ) ) ) };
    const int maxLaserPower{ 14 }; // 4 * 3 + 2
    yMenu = margin + ( barHeight + spacing ) * 2;
    const auto laserPower{ static_cast< int >( laserPass * 3 + laserSpeed ) * 100 / maxLaserPower };
    _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Laser power: " + std::to_string( laserPower ) + "%", { 1, 0.5, 0.5 } );

    int optionalInfos{ 2 };
    constexpr Color_d greenColor{ 0.5, 1, 0.5 };

    // homing missiles count (temporary):
    if( m_homingMissiles != 0 ) {
        yMenu = margin + ( barHeight + spacing ) * ++optionalInfos;
        _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Homing-missiles x" + std::to_string( m_homingMissiles ), greenColor );
    }
    
    // plasma shield remaining time (temporary):
    if( m_plasmaShield != 0 ) {
        yMenu = margin + ( barHeight + spacing ) * ++optionalInfos;
        _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Plasma-shield: " + std::to_string( static_cast< int >( std::ceil( static_cast< double >( m_plasmaShield ) / 60 ) ) ) + "s", greenColor );
    }
}