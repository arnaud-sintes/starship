#include "World.h"

// *** TODO list:
// - functional:
//      - more stages (StageDirector.cpp), with different ennemies (more variety) and specific choregraphies
//      - add special stages (asteroid field, mine field, rescue broken ship etc.)

namespace
{
    // enemy behaviors:
    constexpr int sniperFirePeriodTicks{ 110 };
    constexpr double sniperStationMargin{ 140 }; // it creeps until parked comfortably INSIDE the visible screen...
    constexpr double sniperFireMargin{ 60 };     // ...and only fires while visible
    constexpr double slugSpeed{ 14 }, slugDamage{ 1.5 }, slugSpread{ 0.03 };

    constexpr int attractorsCount{ 1600 }; // generated once; the missions reveal a growing share of them
    constexpr double attractorsRange{ 10000 };
    constexpr double attractorsSecurityDistance{ 200 };

    // blast presets: { radius, damage at center, knockback at center }
    constexpr double missileBlastRadius{ 55 },  missileBlastDamage{ 1 },   missileBlastImpulse{ 8 };
    constexpr double mineBlastRadius{ 110 },    mineBlastDamage{ 2.5 },    mineBlastImpulse{ 14 };
    constexpr double enemyBlastRadius{ 120 },   enemyBlastDamage{ 1.5 },   enemyBlastImpulse{ 12 };
    constexpr double shipBlastRadius{ 150 },    shipBlastDamage{ 3 },      shipBlastImpulse{ 16 };
    constexpr double attractorBlastRadiusFactor{ 2.2 }, attractorBlastDamage{ 3 }, attractorBlastImpulse{ 18 };
    constexpr double gravityMineBlastRadius{ 130 }, gravityMineBlastDamage{ 2.5 }, gravityMineBlastImpulse{ 16 };
    constexpr double waspBlastRadius{ 70 },     waspBlastDamage{ 1 },      waspBlastImpulse{ 8 };

    // attractor pull on free objects (mines, decoy...): the mass drives the
    // interaction range (~480px average, comparable to the ship's), the factor the
    // sinking speed (up to ~2.4 px/tick near the surface, accelerating inward):
    constexpr double attractorPullObjectMass{ 8 };
    constexpr double attractorPullDriftFactor{ 12 };

    // goody bonuses:
    constexpr double repulsorBlastRadius{ 400 }, repulsorBlastImpulse{ 30 };
    constexpr int decoyDurationSeconds{ 8 };
    constexpr int decoyHitPoints{ 3 };
    constexpr double decoyRadius{ 14 };
    constexpr int empDurationSeconds{ 4 };
    constexpr int overdriveDurationSeconds{ 8 };
    constexpr double overdrivePowerBoost{ 1.5 };
    constexpr int singularityDurationSeconds{ 3 };
    constexpr double singularityPullRange{ 800 };
    constexpr double singularityPullStrength{ 0.3 };
    constexpr double singularityCoreRadius{ 30 };
    constexpr double singularityBlastRadius{ 320 }, singularityBlastDamage{ 4 }, singularityBlastImpulse{ 30 };
    constexpr int blossomDurationSeconds{ 4 };
    constexpr int blossomArms{ 2 }; // opposite spiral arms
    constexpr double blossomSpin{ 0.13 }; // per tick, one revolution every ~0.8s
    constexpr int hellstormMissiles{ 16 };

    // goody drop weights:
    constexpr double goodyWeightUnlocked{ 1 };  // the regulars
    constexpr double goodyWeightFresh{ 0.6 };   // freshly unlocked, during their first mission

    // the foreman, stage 1 boss:
    constexpr double shipLaserDamage{ 0.2 };
    constexpr double foremanFightSeconds{ 55 };       // of sustained laser fire at the current fire power
    constexpr double foremanOtherDamageScale{ 0.35 }; // missiles, blasts and rams hurt it less than lasers
    constexpr double foremanCoreMultiplier{ 4 };      // laser damage on its rear core
    constexpr double foremanHoldDistance{ 420 };
    constexpr double foremanSpeed{ 1.6 };             // px/tick
    constexpr double foremanMaxTurn{ 0.0025 };        // rad/tick: slow enough to be outflanked
    constexpr double foremanPullRange{ 650 }, foremanPullStrength{ 0.05 }; // enraged gravity well on the ship
    constexpr int foremanVentFirstSeconds{ 7 };       // before the first vent
    constexpr int foremanVentClosedSeconds{ 12 };     // armored period, shortened by 2 s per phase
    constexpr double foremanVentWarningSeconds{ 1.5 };
    constexpr int foremanVentOpenSeconds{ 4 };        // exposed window
    constexpr double foremanVentTurn{ 0.0004 };       // rad/tick: nearly frozen while venting
    constexpr double foremanVentSpeedScale{ 0.4 };
    constexpr double foremanAttractorDamage{ 0.04 };  // of its max hull per attractor rammed (reduced like any non-laser damage)
    constexpr int foremanMaxMines{ 14 };
    constexpr size_t foremanMaxWasps{ 5 };

    // the warden, stage 2 boss: three pylons shield the core
    constexpr double wardenPylonSeconds{ 15 };        // of sustained laser fire per pylon
    constexpr double wardenCoreSeconds{ 45 };         // of sustained laser fire on the exposed core
    constexpr double wardenHoldDistance{ 560 };
    constexpr double wardenSpeed{ 1.0 };              // px/tick
    constexpr double wardenMaxTurn{ 0.006 };          // rad/tick
    constexpr double wardenOrbitTilt{ 0.5 }, wardenOrbitWobble{ 0.1 }; // rad: the pylons' orbit seen at an angle, wobbling
    constexpr double wardenOrbitPrecession{ 0.003 };  // rad/tick: the orbital plane turns slowly
    constexpr double wardenPylonSpin{ 0.009 };        // rad/tick, accelerating as the pylons fall
    constexpr double wardenPylonPullRange{ 650 }, wardenPylonPullStrength{ 0.03 }; // each live pylon is a gravity well
    constexpr double wardenCorePullRange{ 750 }, wardenCorePullStrength{ 0.05 };   // the exposed core drags the ship in
    constexpr double wardenOtherDamageScale{ 0.35 };  // missiles, blasts and rams vs the exposed core
    constexpr int wardenMaxMines{ 16 };
    constexpr size_t wardenMaxWasps{ 6 };

    constexpr size_t maxParticules{ 2500 }; // global cap, keeps sustained blast chains from melting the frame

    // turret bonus:
    constexpr double turretOrbitRadius{ 75 };
    constexpr double turretOrbitSpeed{ 0.052 }; // one revolution every ~2 seconds
    constexpr double turretRange{ 900 };
    constexpr int turretFirePeriod{ 4 }; // ticks between shots (15/s)
    constexpr int turretDurationSeconds{ 10 };
    constexpr double turretExpireBlastRadius{ 90 }, turretExpireBlastDamage{ 1 }, turretExpireBlastImpulse{ 10 };

    // enemy gravity mines population, maintained in a band around the ship:
    constexpr double gravityMineSpawnMin{ 0.7 }, gravityMineSpawnMax{ 1.5 }; // in max screen dimensions
    constexpr double gravityMineDespawn{ 2.2 };
    constexpr double gravityMineSeparation{ 300 };

    // pull of a gravity well on an object at _from:
    Vector WellPull( const Vector & _from, const Vector & _source, const double _range, const double _strength )
    {
        const auto delta{ _source - _from };
        const auto distance{ delta.Distance() };
        if( distance <= 1 || distance >= _range )
            return {};
        const auto falloff{ 1 - distance / _range };
        return delta * ( 1.0 / distance ) * ( _strength * falloff * falloff );
    }

    Color_d _RingColor( const eFadeColor _color )
    {
        switch( _color ) {
            case eFadeColor::azure: return { 0.45, 0.75, 1 };
            case eFadeColor::rose: return { 1, 0.4, 0.55 };
            case eFadeColor::green: return { 0.5, 1, 0.7 };
            case eFadeColor::violet: return { 0.75, 0.5, 1 };
            default: return { 1, 0.75, 0.4 };
        }
    }
}


World::World( AudioDirector & _audio, const Dimension_ui & _screenDimension, const int _frameRate, const WorldCheckpoint & _checkpoint )
    : m_audio{ _audio }
    , m_screenDimension{ _screenDimension }
    , m_screenCenter{ static_cast< double >( _screenDimension.width ) * 0.5, static_cast< double >( _screenDimension.height ) * 0.5 }
    , m_frameRate{ _frameRate }
    , m_ship{ { 0.5, 0.75, 1 }, {}, Maths::PiHalf, { 0, -5 }, {}, 0,
        5, // damage
        { 7, 7, 0.015, 0.65 }, // shield (quality keeps the mass close to the historical 5/0.5 setup)
        { 20, 20, 0.015, 0.75 }, // propellant
        { 0, 0.5, false, 0.005, 0.01, 0.75, 0 }, // engine
        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } } // rotators
    , m_stage{ _frameRate, _checkpoint.progress }
    , m_checkpoint{ _checkpoint }
    , m_score{ _checkpoint.score }
    , m_lives{ _checkpoint.lives }
    , m_laserSpeed{ static_cast< eLaserSpeed >( _checkpoint.laserSpeed ) }
    , m_laserPass{ static_cast< eLaserPass >( _checkpoint.laserPass ) }
    , m_homingMissiles{ _checkpoint.homingMissiles }
    , m_magneticMines{ _checkpoint.magneticMines }
    , m_upgrades{ _checkpoint.upgrades }
    , m_plasmaShield{ _frameRate * 10 } // 10 seconds immunity at startup
    , m_wind{ _frameRate }
{
    m_ship.id = m_nextRocketId++;
    m_upgrades.ApplyAll( m_ship );
    m_ship.RefreshGeometry();
    m_ship.momentum -= m_wind.Base();
    m_shipBaseEnginePower = m_ship.engine.power;

    m_sound_spaceWind = m_audio.CreateLoop( eSound::spaceWind, { 0.0, {}, {} } );
    m_sound_shipMainEngine = m_audio.CreateLoop( eSound::shipMainEngine, { 0.0, {}, {} } );
    m_sound_shipRotationEngine = m_audio.CreateLoop( eSound::shipRotationEngine, { 0.0, {}, {} } );

    for( const auto type : m_stage.Missing( m_enemies ) )
        _AddEnemy( type );

    m_attractors.Generate( attractorsCount, attractorsRange, attractorsSecurityDistance, 1, 2 );
    m_attractors.SetDensity( m_stage.Mission().attractorDensity );
    m_attractors.ApplyDensity( m_ship.position, 0 ); // nobody is watching yet
}


World::~World()
{
    m_audio.StopLoop( m_sound_spaceWind );
    m_audio.StopLoop( m_sound_shipMainEngine );
    m_audio.StopLoop( m_sound_shipRotationEngine );
    for( auto & enemy : m_enemies ) {
        m_audio.StopLoop( enemy.sound_mainEngine );
        m_audio.StopLoop( enemy.sound_rotationEngine );
    }
    for( auto & missile : m_missiles )
        m_audio.StopLoop( missile.sound_run );
}


void World::_AddEnemy( const Enemy::eType _type )
{
    // shields are correlated to the current laser pass to remain fair:
    const double laserPassShield{ static_cast< double >( m_laserPass ) };
    const auto orientation{ Maths::Random( 0, Maths::Pi2 ) };
    const auto buildRocket{ [ & ]() -> Rocket {
            switch( _type ) {
                case Enemy::eType::wasp:
                    return Rocket{ { 1, 0.8, 0.3 }, {}, orientation, {}, {}, 0,
                        1.5, // damage: ram chip
                        { 0.4, 0.4, 0.001, 0.2 }, // shield: chaff, dies to anything
                        { 4, 4, 0.08, 0.75 }, // propellant: tiny
                        { 0, 0.7, false, 0.01, 0.02, 0.8, 0 }, // engine: fast
                        { { 0, 0 }, 0.02, { false, false }, 0.002, 0.008, 0.5, { 0, 0 } } }; // agile
                case Enemy::eType::sniper:
                    return Rocket{ { 1, 0.65, 0.6 }, {}, orientation, {}, {}, 0,
                        2, // damage
                        { 1 + laserPassShield * 0.75, 1 + laserPassShield * 0.75, 0.001, 0.2 }, // shield
                        { 8, 8, 0.05, 0.5 }, // propellant
                        { 0, 0.2, false, 0.005, 0.01, 0.5, 0 }, // engine: slow creep into view
                        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } };
                case Enemy::eType::foreman: {
                    // hit points follow the player's fire power, whatever the laser level:
                    const double fireRate{ static_cast< double >( m_laserPass ) * m_frameRate / static_cast< double >( m_laserSpeed ) };
                    const double hitPoints{ shipLaserDamage * fireRate * foremanFightSeconds };
                    constexpr double hullStroke{ 14 };
                    return Rocket{ { 1, 0.62, 0.25 }, {}, orientation, {}, {}, 0,
                        3, // damage: a ram hurts
                        { hitPoints, hitPoints, 0, 1 - hullStroke / hitPoints }, // shield: the hull stays 14 px thick at any laser level
                        { 70, 70, 5, 0 }, // propellant: gives the 70 px body
                        { 0, 0.001, false, 0.0001, 0.0001, 0.9, 0 }, // engine: inert, the steering drives the motion
                        { { 0, 0 }, 0.0015, { false, false }, 0.0004, 0.002, 0.5, { 0, 0 } } }; // slow turning
                }
                case Enemy::eType::warden: {
                    const double fireRate{ static_cast< double >( m_laserPass ) * m_frameRate / static_cast< double >( m_laserSpeed ) };
                    const double hitPoints{ shipLaserDamage * fireRate * wardenCoreSeconds };
                    constexpr double hullStroke{ 14 };
                    return Rocket{ { 0.7, 0.5, 1 }, {}, orientation, {}, {}, 0,
                        3, // damage: a ram hurts
                        { hitPoints, hitPoints, 0, 1 - hullStroke / hitPoints }, // shield: the exposed core
                        { 95, 95, 5, 0 }, // propellant: gives the 95 px body
                        { 0, 0.001, false, 0.0001, 0.0001, 0.9, 0 }, // engine: inert, the steering drives the motion
                        { { 0, 0 }, 0.0015, { false, false }, 0.0004, 0.002, 0.5, { 0, 0 } } };
                }
                case Enemy::eType::chaser:
                default:
                    return Rocket{ { 1, 0.5, 0.75 }, {}, orientation, {}, {}, 0,
                        2, // damage (contact damage applies per tick of overlap, keep it survivable)
                        { laserPassShield, laserPassShield, 0.001, 0.2 }, // shield
                        { 10, 10, 0.05, Maths::Random( 0.1, 0.75 ) }, // propellant
                        { 0, Maths::Random( 0.1, 0.5 ), false, 0.005, 0.01, Maths::Random( 0.2, 0.75 ), 0 }, // engine
                        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } };
            }
        } };

    // spawn band, in max screen dimensions - the sniper appears farther out:
    double spawnMin{ 0.5 }, spawnMax{ 0.75 };
    if( _type == Enemy::eType::sniper ) {
        spawnMin = 0.8;
        spawnMax = 1.1;
    }
    else if( _type == Enemy::eType::wasp ) {
        spawnMin = 0.6;
        spawnMax = 0.9;
    }
    else if( _type == Enemy::eType::foreman || _type == Enemy::eType::warden ) {
        spawnMin = 0.9;
        spawnMax = 1.0;
    }

    auto & enemy{ m_enemies.emplace_back( Enemy{ buildRocket(), _type,
            static_cast< int >( Maths::Random( 0, m_frameRate * 5 ) ),
            m_audio.CreateLoop( eSound::shipMainEngine, { 0.0, {}, {} } ),
            m_audio.CreateLoop( eSound::shipRotationEngine, { 0.0, {}, {} } )
        } ) };
    enemy.rocket.position = m_ship.position + Vector::From( Maths::Random( 0, Maths::Pi2 ),
        Maths::Random( spawnMin, spawnMax ) * static_cast< double >( std::max( m_screenDimension.width, m_screenDimension.height ) ) );
    enemy.rocket.id = m_nextRocketId++;
    enemy.rocket.RefreshGeometry();
    enemy.boss.maxShield = enemy.rocket.shield.capacity;
    enemy.boss.lastShield = enemy.rocket.shield.value;
    enemy.boss.orbit = Maths::Random( 0, 1 ) < 0.5 ? -1 : 1;
    enemy.boss.ventTimer = m_frameRate * foremanVentFirstSeconds;
    if( _type == Enemy::eType::warden ) {
        const double fireRate{ static_cast< double >( m_laserPass ) * m_frameRate / static_cast< double >( m_laserSpeed ) };
        enemy.boss.pylonMax = shipLaserDamage * fireRate * wardenPylonSeconds;
        enemy.boss.pylonShield.fill( enemy.boss.pylonMax );
    }
}


void World::_SpawnMissile( const Rocket & _launcher, const bool _targetShip, const double _orientationOffset, const double _spawnDistance, const double _kick )
{
    const auto orientation{ _launcher.orientation + _orientationOffset };
    const auto position{ _launcher.position + ( _spawnDistance > 0 ? Vector::From( orientation + Maths::Pi, _spawnDistance ) : Vector{} ) };
    Vector motion{};
    if( !_targetShip && _ClosestEnemy( _launcher.position ) == nullptr )
        motion = Vector::From( orientation, -5 );
    motion += Vector::From( orientation + Maths::Pi, _kick ); // fan-out launch kick
    auto & missile{ m_missiles.emplace_back( Missile{ Rocket{ _targetShip ? Color_d{ 1, 0.5, 0.75 } : Color_d{ 0.5, 0.75, 1 }, position, orientation, motion, _launcher.momentum, 0,
        2, // damage (a direct hit also eats the point-blank blast on top)
        { 1, 1, 0.01, 0.5 }, // shield
        { 10, 10, _targetShip ? 0.01 : 0.005, 0.9 }, // propellant
        { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0.5, { 0, 0 } } }, // guided missiles rotator
        _targetShip, _launcher.id, &_launcher == &m_ship,
        m_audio.CreateLoop( eSound::missileRun, { 0.0, {}, 0.0 } )
        } ) };
    missile.rocket.id = m_nextRocketId++;
    missile.rocket.RefreshGeometry();
}


void World::Update( const PlayerInput & _input )
{
    // reset burst states:
    _ResetBursts();

    // player commands (none once the ship is gone):
    if( !m_shipDestroyed ) {
        _HandleControls( _input );
        _UpdateTurret();
        _UpdateBlossom();
    }

    // simulation, same ordering as the historical game loop:
    _UpdateAttractions();
    _UpdatePlasmaShield();
    _UpdateBonuses();
    _UpdateSingularity();
    _UpdateBlasts();
    _UpdateEnemyCollisions();
    _UpdateLaserCollisions();
    _UpdateMissileCollisions();
    _UpdateShipAttractorCollisions();
    _UpdateEnemies();
    _UpdateMission();
    _UpdateMines();
    _UpdateGravityMines();
    _UpdateAttractorsDeletion();
    _UpdateGoodies();
    _UpdateLasers();
    _UpdateMissiles();
    _UpdateSlugs();
    _UpdateShip();
    _UpdateWind();
    _UpdateAlerts();
    _UpdateEngineSounds();
    UpdateAmbientSound();

    // particules:
    for( const auto & enemy : m_enemies )
        _AddEnginesParticules( enemy.rocket );
    for( const auto & missile : m_missiles )
        _AddEnginesParticules( missile.rocket );
    if( !m_shipDestroyed )
        _AddEnginesParticules( m_ship );
    _UpdateParticules();
}


const Rocket * World::ClosestEnemy( const Vector & _position ) const
{
    return const_cast< World * >( this )->_ClosestEnemy( _position );
}


Rocket * World::_ClosestEnemy( const Vector & _position )
{
    Rocket * pTarget{ nullptr };
    double minDistance{ std::numeric_limits< double >::infinity() };
    for( auto & enemy : m_enemies ) {
        if( enemy.dead )
            continue;
        const auto distance{ ( enemy.rocket.position - _position ).DistanceSquared() };
        if( distance >= minDistance )
            continue;
        minDistance = distance;
        pTarget = &enemy.rocket;
    }
    return pTarget;
}


void World::_ResetBursts()
{
    for( auto & enemy : m_enemies )
        enemy.rocket.Reset();
    for( auto & missile : m_missiles )
        missile.rocket.Reset();
    m_ship.Reset();
}


void World::_HandleControls( const PlayerInput & _input )
{
    // activate ship burst:
    if( _input.thrust )
        m_ship.ActivateThrust();

    // follow the mouse cursor:
    m_ship.PointTo( _input.cursorPosition - m_screenCenter + m_ship.position, 0.5 );

    // laser:
    if( m_laserCadence++ % static_cast< int >( m_laserSpeed ) == 0 && _input.fire ) {
        m_audio.Play( eSound::laserShot );
        for( int i{ 0 }; i < static_cast< int >( m_laserPass ); i++ ) {
            const auto rightSide{ ( m_laserAlternate % 2 ) == 0 };
            const auto position{ Vector::From( m_ship.orientation + Maths::PiHalf * ( rightSide ? 1 : -1 ), m_ship.dynamic.boundingBoxRadius ) };
            const auto wave{ std::sin( static_cast< double >( m_laserAlternate ) * 0.5 ) }; // wave speed
            const auto wideAngle{ ( rightSide ? 1 : -1 ) * ( Maths::PiHalf * wave * 0.05 ) }; // wave amplitude
            const auto momentum{ Vector::From( m_ship.orientation + Maths::Pi - wideAngle, 50 ) }; // 50px length
            auto & laser{ m_lasers.emplace_back( Laser{ m_ship.position - momentum + m_ship.dynamic.headPosition + position, momentum,
                shipLaserDamage } ) }; // damage
            laser.Refresh();
            m_laserAlternate++;
        }
    }

    // shoot missile:
    if( m_homingMissiles > 0 && m_missileCadence++ > 25 && _input.fire ) {
        m_homingMissiles--;
        if( m_homingMissiles == 0 )
            m_audio.Play( eSound::homingMissilesOff, 0.9 );
        m_missileCadence = 0;
        _SpawnMissile( m_ship, false );
        m_audio.Play( eSound::missileShot );
    }

    // drop mines:
    if( m_magneticMines > 0 && m_mineCadence++ > 60 ) {
        m_mineCadence = 0;
        double shortestDistanceSquared{ 1000 * 1000 };
        for( const auto & mine : m_mines )
            shortestDistanceSquared = std::min( shortestDistanceSquared, ( m_ship.position - mine.position ).DistanceSquared() );
        // must be far enough of other mines:
        if( shortestDistanceSquared > 50 * 50 ) {
            m_mines.emplace_back( Mine{ m_ship.position, 0.5 } );
            m_magneticMines--;
            m_audio.Play( eSound::magneticMinesDrop );
            if( m_magneticMines == 0 )
                m_audio.Play( eSound::magneticMinesOff, 0.9 );
        }
    }
}


void World::_AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size, const eFadeColor _color )
{
    if( m_particules.size() >= maxParticules ) // explosions degrade gracefully under heavy load
        return;
    const auto momentum{ Vector::From( _orientation, _speed ) + _direction };
    m_particules.emplace_back( Particule{ _position, momentum, Maths::Random( 1.5, 3 ), _size, _color } );
}


void World::_AddExplosion( const Vector & _position, const Vector & _direction, const eExplosion _explosion, const eFadeColor _color )
{
    const auto rangeMax{ _explosion == eExplosion::small ? 1.0 : ( _explosion == eExplosion::medium ? 2.0 : 3.0 ) };
    for( int i{ 0 }; i < static_cast< int >( _explosion ); i++ )
        _AddParticule( _position, _direction, Maths::Random( 0, Maths::Pi2 ), Maths::Random( 0, 4 ), Maths::Random( 0.5, rangeMax ), _color );
}


void World::_AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter )
{
    const int passCount{ _rate < 0.5 ? 1 : _maxPass };
    const double maxSize{ 0.5 + _rate * _limiter }; // [0.5, 0.5 to 1.5]
    const double maxSpeed{ 1 + _rate }; // [0.5, 1 to 2]
    const double maxWideness{ 0.25 + _rate * 0.75 * _limiter }; // [-0.25, +0.25] to [-1, +1]
    for( int i{ 0 }; i < passCount; i++ )
        _AddParticule( _position + _burst.position, {}, _burst.orientation + Maths::Random( -maxWideness, maxWideness ), Maths::Random( 0.5, maxSpeed ), Maths::Random( 0.5, maxSize ), eFadeColor::violet );
}


void World::_AddEnginesParticules( const Rocket & _rocket )
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


bool World::_RocketCollision( const Rocket & _a, const Rocket & _b )
{
    return Maths::Collision( _a.position, _a.dynamic.boundingBoxRadius, _b.position, _b.dynamic.boundingBoxRadius );
}


void World::_LaserImpact( const Laser & _a, const Vector & _b, const eFadeColor _color )
{
    const auto momentum{ _a.momentum * 0.01 };
    _AddExplosion( _b, Vector{} - momentum, eExplosion::small, _color );
}


void World::_LaserImpact( Laser & _a, Rocket & _b )
{
    const auto momentum{ _a.momentum * 0.01 };
    // transmit momentum:
    _b.ReceiveImpact( _a.position, momentum, 0.05 );
    // add small impact explosion on target:
    _AddExplosion( _b.position, Vector{} - momentum, eExplosion::small );
    // shield impact:
    _b.shield.value -= _a.damage;
}


void World::_RocketImpact( Rocket & _a, Rocket & _b )
{
    // transmit momentum:
    _b.ReceiveImpact( _a.position, _a.momentum, 0.05 * ( _a.dynamic.totalMass / 15 ) );
    // add small impact explosion on target:
    _AddExplosion( _b.position, Vector{} - _a.momentum, eExplosion::medium );
    // shield impact:
    _b.shield.value -= _a.damage;
}


bool World::_LaserRocketCollision( Laser & _laser, Rocket & _other )
{
    if( !Maths::Collision( _other.position, _other.dynamic.boundingBoxRadius, _laser.dynamic.positionA, _laser.dynamic.positionB ) )
        return false;
    _LaserImpact( _laser, _other );
    return true;
}


bool World::_LaserCoreCollision( Laser & _laser, Enemy & _enemy )
{
    const auto hit{ Maths::Collision( _enemy.CorePosition(), Enemy::coreRadius, _laser.dynamic.positionA, _laser.dynamic.positionB ) };
    if( !hit )
        return false;
    if( !_enemy.CoreOpen() ) {
        // armored: the shot is absorbed with a dull spark
        _AddExplosion( *hit, {}, eExplosion::small, eFadeColor::orange );
        m_audio.PlayAt( eSound::laserCollision, _RelativeToShip( *hit ) );
        return true;
    }
    _LaserImpact( _laser, _enemy.rocket ); // a regular hit...
    _enemy.rocket.shield.value -= _laser.damage * ( foremanCoreMultiplier - 1 ); // ...hurting much more
    _enemy.boss.laserDamage += _laser.damage * foremanCoreMultiplier;
    _enemy.boss.coreFlash = 10;
    _AddExplosion( *hit, {}, eExplosion::small, eFadeColor::rose );
    m_audio.PlayAt( eSound::laserCollision, _RelativeToShip( *hit ) );
    _AddScore( 5 );
    return true;
}


bool World::_LaserPylonCollision( Laser & _laser, Enemy & _enemy )
{
    for( int i{ 0 }; i < Enemy::pylonCount; i++ ) {
        if( !_enemy.PylonAlive( i ) )
            continue;
        const auto position{ _enemy.PylonPosition( i ) };
        const auto hit{ Maths::Collision( position, Enemy::pylonRadius, _laser.dynamic.positionA, _laser.dynamic.positionB ) };
        if( !hit )
            continue;
        const auto index{ static_cast< size_t >( i ) };
        _enemy.boss.pylonShield.at( index ) -= _laser.damage;
        _enemy.boss.pylonFlash.at( index ) = 8;
        _AddExplosion( *hit, {}, eExplosion::small, eFadeColor::violet );
        m_audio.PlayAt( eSound::laserCollision, _RelativeToShip( *hit ) );
        _AddScore( 3 );
        if( !_enemy.PylonAlive( i ) ) {
            // the pylon bursts: a friendly shockwave and its well is gone
            _Detonate( position, {}, eExplosion::big, eFadeColor::violet, 240, 0, 10, true );
            m_audio.PlayAt( eSound::attractorExplosion, _RelativeToShip( position ) );
            m_audio.PlayAt( eSound::shipExplosion, _RelativeToShip( position ) );
            _AddScore( 250 );
        }
        return true;
    }
    return false;
}


bool World::_MissileRocketCollision( Missile & _missile, Rocket & _other )
{
    // prevent collision when launched:
    if( _missile.originId == _other.id && !_missile.bypassCollision ) {
        if( !Maths::Collision( _missile.rocket.position, _missile.rocket.dynamic.boundingBoxRadius * 2, _other.position, _other.dynamic.boundingBoxRadius * 4 ) )
            _missile.bypassCollision = true;
        return false;
    }
    if( !_RocketCollision( _missile.rocket, _other ) )
        return false;
    _RocketImpact( _missile.rocket, _other );
    return true;
}


void World::_AddScore( const int _points )
{
    // whatever keeps exploding in the background once the ship is gone is not the
    // player's merit anymore:
    if( !m_shipDestroyed )
        m_score += _points;
}


void World::_CollectGoody( const Goody::eType _type )
{
    _AddScore( 5 );

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
            m_audio.Play( eSound::laserPowerUp, 0.9 );
        return;
    }
    if( _type == Goody::eType::homingMissiles ) {
        m_homingMissiles += 30; // 30x missiles pack
        m_audio.Play( eSound::homingMissiles, 0.9 );
        return;
    }
    if( _type == Goody::eType::magneticMines ) {
        m_magneticMines += 10; // 10x mines pack
        m_audio.Play( eSound::magneticMines, 0.9 );
        return;
    }
    if( _type == Goody::eType::plasmaShield ) {
        m_plasmaShield += m_frameRate * 5; // 5 seconds plasma shield
        m_audio.Play( eSound::plasmaShield, 0.9 );
        return;
    }
    if( _type == Goody::eType::shieldAdd ) {
        if( m_ship.shield.value >= m_ship.shield.capacity )
            return;
        m_ship.shield.value = std::min( m_ship.shield.value + m_ship.shield.capacity * 0.5, m_ship.shield.capacity ); // 50% capacity boost
        m_audio.Play( eSound::shieldRepair, 0.9 );
        return;
    }
    if( _type == Goody::eType::propellantAdd ) {
        if( m_ship.propellant.value >= m_ship.propellant.capacity )
            return;
        m_ship.propellant.value = std::min( m_ship.propellant.value + m_ship.propellant.capacity * 0.5, m_ship.propellant.capacity ); // 50% capacity boost
        m_audio.Play( eSound::propellantRefuel, 0.9 );
        return;
    }
    if( _type == Goody::eType::turret ) {
        if( m_turretTicks == 0 )
            m_turretOrbitAngle = m_ship.orientation; // deploy behind the ship
        m_turretTicks += m_frameRate * turretDurationSeconds; // durations stack
        m_audio.Play( eSound::turret, 0.9 );
        return;
    }
    if( _type == Goody::eType::repulsor ) {
        // instant: a friendly shockwave - zero damage, huge knockback, chain-triggering
        m_audio.Play( eSound::repulsor, 0.9 );
        m_audio.Play( eSound::attractorExplosion );
        _Detonate( m_ship.position, {}, eExplosion::big, eFadeColor::green, repulsorBlastRadius, 0, repulsorBlastImpulse, true );
        return;
    }
    if( _type == Goody::eType::decoy ) {
        m_decoyPosition = m_ship.position; // dropped where it was picked
        m_decoyTicks = m_frameRate * decoyDurationSeconds; // redeployed, not stacked
        m_decoyHp = decoyHitPoints;
        m_audio.Play( eSound::decoy, 0.9 );
        return;
    }
    if( _type == Goody::eType::emp ) {
        m_empTicks += m_frameRate * empDurationSeconds; // durations stack
        m_audio.Play( eSound::emp, 0.9 );
        return;
    }
    if( _type == Goody::eType::overdrive ) {
        m_overdriveTicks += m_frameRate * overdriveDurationSeconds; // durations stack
        m_audio.Play( eSound::overdrive, 0.9 );
        return;
    }
    if( _type == Goody::eType::singularity ) {
        m_singularityPosition = m_ship.position; // deployed where it was picked
        m_singularityTotalTicks = m_frameRate * singularityDurationSeconds;
        m_singularityTicks = m_singularityTotalTicks;
        m_audio.Play( eSound::singularity );
        return;
    }
    if( _type == Goody::eType::blossom ) {
        m_blossomTicks += m_frameRate * blossomDurationSeconds; // durations stack
        m_audio.Play( eSound::blossom );
        return;
    }
    if( _type == Goody::eType::hellstorm ) {
        // instant: a spiral fan of homing missiles
        m_audio.Play( eSound::hellstorm, 0.9 );
        m_audio.Play( eSound::missileShot );
        for( int i{ 0 }; i < hellstormMissiles; i++ )
            _SpawnMissile( m_ship, false, Maths::Pi2 * i / hellstormMissiles, 65, 4 );
        return;
    }
}


void World::_MaybeDropGoody( const Vector & _position )
{
    // 50% chance of goody addition, only if far enough (avoid volontary collision bonuses...)
    if( ( _position - m_ship.position ).Distance() <= 100 || Maths::Random( 0, 1 ) >= 0.5 )
        return;

    // the missions unlock the goodies progressively (Goody::FirstMission), the
    // freshly unlocked ones being rarer for their first mission:
    using eType = Goody::eType;
    constexpr std::array< eType, 14 > all{ eType::laserUp, eType::homingMissiles, eType::magneticMines, eType::plasmaShield, eType::shieldAdd,
        eType::propellantAdd, eType::turret, eType::repulsor, eType::decoy, eType::emp, eType::overdrive, eType::singularity, eType::blossom,
        eType::hellstorm };
    std::array< std::pair< eType, double >, 14 > candidates;
    size_t candidateCount{ 0 };
    double totalWeight{ 0 };
    for( const auto type : all ) {
        if( type == eType::shieldAdd && m_ship.shield.value >= m_ship.shield.capacity )
            continue;
        if( type == eType::propellantAdd && m_ship.propellant.value >= m_ship.propellant.capacity )
            continue;
        if( type == eType::laserUp && m_laserSpeed == eLaserSpeed::fast && m_laserPass == eLaserPass::height )
            continue;
        const auto first{ Goody::FirstMission( type ) };
        if( !m_stage.Reached( first.stage, first.mission ) )
            continue;
        const auto weight{ m_stage.IsAt( first.stage, first.mission ) ? goodyWeightFresh : goodyWeightUnlocked };
        candidates.at( candidateCount++ ) = { type, weight };
        totalWeight += weight;
    }
    auto roll{ Maths::Random( 0, totalWeight ) };
    for( size_t i{ 0 }; i < candidateCount; i++ ) {
        roll -= candidates.at( i ).second;
        if( roll <= 0 || i + 1 == candidateCount ) {
            m_goodies.emplace_back( Goody{ _position, candidates.at( i ).first } );
            return;
        }
    }
}


double World::_AttractionQueryRange( const Rocket & _rocket ) const
{
    return AttractorField::distanceThreshold * m_attractors.MaxMass() * _rocket.dynamic.totalMass;
}


Vector World::_AttractorPull( const Vector & _position, const double _mass )
{
    Vector pull;
    m_attractors.ForEachInRange( _position, AttractorField::distanceThreshold * m_attractors.MaxMass() * _mass,
        [ & ]( const int, const Attractor & _attractor ){
            pull += _attractor.position.ProximityAttraction( _position, _attractor.mass * _mass, AttractorField::distanceThreshold );
        } );
    return pull;
}


bool World::_TouchesAttractor( const Vector & _position, const double _radius )
{
    bool touches{ false };
    m_attractors.ForEachInRange( _position, _radius,
        [ & ]( const int, const Attractor & _attractor ){
            if( Maths::Collision( _position, _radius, _attractor.position, _attractor.radius ) )
                touches = true;
        } );
    return touches;
}


void World::_UpdateAttractions()
{
    m_attractedRockets.clear();
    if( !m_shipDestroyed )
        m_attractedRockets.emplace_back( &m_ship );
    for( auto & enemy : m_enemies )
        if( !enemy.IsBoss() ) // too heavy to be moved by the wells
            m_attractedRockets.emplace_back( &enemy.rocket );
    for( auto & missile : m_missiles )
        m_attractedRockets.emplace_back( &missile.rocket );
    constexpr double windForceFactor{ 0.05 }; // terminal wind drift of ~1-2 px/tick for the ship
    for( auto * pRocket : m_attractedRockets ) {
        Vector attraction;
        m_attractors.ForEachInRange( pRocket->position, _AttractionQueryRange( *pRocket ),
            [ & ]( const int, const Attractor & _attractor ){
                attraction += _attractor.position.ProximityAttraction( pRocket->position, _attractor.mass * pRocket->dynamic.totalMass, AttractorField::distanceThreshold );
            } );
        // gravity mines pull:
        for( const auto & gravityMine : m_gravityMines )
            attraction += gravityMine.Attraction( pRocket->position );
        // singularity pull (the ship resists it better - it must stay escapable):
        if( m_singularityTicks > 0 )
            attraction += _SingularityPull( pRocket->position ) * ( pRocket == &m_ship ? 0.5 : 1.0 );
        // the enraged foreman drags the ship toward itself:
        if( pRocket == &m_ship )
            for( const auto & enemy : m_enemies ) {
                // the warden: each live pylon is a gravity well, and its exposed core drags the ship in
                if( enemy.type == Enemy::eType::warden ) {
                    for( int i{ 0 }; i < Enemy::pylonCount; i++ )
                        if( enemy.PylonAlive( i ) )
                            attraction += WellPull( m_ship.position, enemy.PylonPosition( i ), wardenPylonPullRange, wardenPylonPullStrength );
                    if( enemy.PylonsAlive() == 0 )
                        attraction += WellPull( m_ship.position, enemy.rocket.position, wardenCorePullRange, wardenCorePullStrength );
                }
                if( enemy.type == Enemy::eType::foreman && enemy.boss.phase == 2 )
                    attraction += WellPull( m_ship.position, enemy.rocket.position, foremanPullRange, foremanPullStrength );
            }
        // the local solar wind pushes every rocket through the same force pathway:
        pRocket->dynamic.attraction = attraction + m_wind.At( pRocket->position ) * windForceFactor;
    }
}


void World::_Detonate( const Vector & _position, const Vector & _direction, const eExplosion _explosion, const eFadeColor _color, const double _radius, const double _damage, const double _impulse, const bool _ignoreShip )
{
    _AddExplosion( _position, _direction, _explosion, _color );
    m_pendingBlasts.emplace_back( Blast{ _position, _radius, _damage, _impulse, _RingColor( _color ), _ignoreShip } );
}


void World::_ExplodeMissile( Missile & _missile )
{
    _missile.dead = true;
    m_audio.StopLoop( _missile.sound_run );
    m_audio.PlayAt( eSound::missileExplosion, _RelativeToShip( _missile.rocket.position ) );
    _Detonate( _missile.rocket.position, _missile.rocket.momentum, eExplosion::medium, eFadeColor::orange, missileBlastRadius, missileBlastDamage, missileBlastImpulse );
}


void World::_UpdateBlasts()
{
    // age and expire the shockwave rings:
    for( auto & blast : m_blasts )
        blast.age++;
    std::erase_if( m_blasts, []( const Blast & _blast ){ return _blast.age > Blast::maxAge; } );

    // apply the impact zone of the blasts enqueued since the last tick; chains
    // triggered here enqueue new blasts for the NEXT tick, so reactions ripple:
    auto pending{ std::move( m_pendingBlasts ) };
    m_pendingBlasts.clear();
    for( const auto & blast : pending ) {
        _ApplyBlast( blast );
        m_blasts.emplace_back( blast );
    }
}


void World::_ApplyBlast( const Blast & _blast )
{
    const auto factorAt{ [ &_blast ]( const Vector & _position ){
            const auto distanceSquared{ ( _position - _blast.position ).DistanceSquared() };
            const auto radiusSquared{ _blast.radius * _blast.radius };
            return distanceSquared >= radiusSquared ? 0.0 : 1.0 - distanceSquared / radiusSquared;
        } };
    const auto push{ [ &_blast ]( Rocket & _rocket, const double _factor ){
            auto direction{ _rocket.position - _blast.position };
            const auto distance{ direction.Distance() };
            direction = distance > 0.001 ? direction * ( 1.0 / distance ) : Vector::From( Maths::Random( 0, Maths::Pi2 ), 1 );
            _rocket.ReceiveImpact( _blast.position, direction * ( _blast.impulse * _factor ), 0.02 * _factor );
        } };

    // ship (the plasma shield blocks the damage, not the shove; friendly blasts spare it entirely):
    if( !m_shipDestroyed && !_blast.ignoreShip ) {
        const auto factor{ factorAt( m_ship.position ) };
        if( factor > 0 ) {
            push( m_ship, factor );
            if( m_plasmaShield <= 0 )
                m_ship.shield.value -= _blast.damage * factor;
        }
    }

    // enemies:
    for( auto & enemy : m_enemies ) {
        const auto factor{ factorAt( enemy.rocket.position ) };
        if( factor > 0 ) {
            push( enemy.rocket, factor );
            enemy.rocket.shield.value -= _blast.damage * factor;
        }
    }

    // missiles caught in the zone explode (and blast in turn - chain):
    for( auto & missile : m_missiles )
        if( !missile.dead && factorAt( missile.rocket.position ) > 0 )
            _ExplodeMissile( missile );

    // mines and gravity mines caught in the zone are triggered (they detonate in
    // their own pass - chain):
    for( auto & mine : m_mines )
        if( mine.alive && factorAt( mine.position ) > 0 )
            mine.alive = false;
    for( auto & gravityMine : m_gravityMines )
        if( gravityMine.alive && factorAt( gravityMine.position ) > 0 )
            gravityMine.alive = false;

    // attractors get chipped:
    m_attractors.ForEachInRange( _blast.position, _blast.radius,
        [ & ]( const int, Attractor & _attractor ){
            _attractor.shield -= _blast.damage * factorAt( _attractor.position );
        } );

    // and the dust is blown away:
    for( auto & particule : m_particules ) {
        const auto factor{ factorAt( particule.position ) };
        if( factor <= 0 )
            continue;
        const auto direction{ particule.position - _blast.position };
        const auto distance{ direction.Distance() };
        if( distance > 0.001 )
            particule.momentum += direction * ( 1.0 / distance ) * ( factor * _blast.impulse * 0.1 );
    }
}


void World::_UpdatePlasmaShield()
{
    if( m_plasmaShield > 0 ) {
        m_plasmaShield--;
        if( m_plasmaShield == 0 )
            m_audio.Play( eSound::plasmaShieldOff, 0.9 );
    }
    m_plasmaShieldIncrement += 4;
    if( m_plasmaShieldIncrement > 100 )
        m_plasmaShieldIncrement = 0;
    m_plasmaShieldRamp = ( m_plasmaShieldIncrement * m_plasmaShieldIncrement ) / 10000;
    m_plasmaShieldRadius = 30 + ( m_plasmaShieldRamp * 70 ); // maxium radius of 100
}


void World::_UpdateEnemyCollisions()
{
    for( auto & enemy : m_enemies ) {
        // enemies-enemies collisions:
        for( auto & enemyOther : m_enemies )
            if( &enemy != &enemyOther )
                if( _RocketCollision( enemy.rocket, enemyOther.rocket ) ) {
                    m_audio.PlayAt( eSound::shipCollision, _RelativeToShip( enemy.rocket.position ) );
                    _RocketImpact( enemy.rocket, enemyOther.rocket );
                }
        // enemy-ship collision:
        if( !m_shipDestroyed && _RocketCollision( enemy.rocket, m_ship ) ) {
            m_audio.PlayAt( eSound::shipCollision, _RelativeToShip( enemy.rocket.position ) );
            _RocketImpact( enemy.rocket, m_ship );
            _RocketImpact( m_ship, enemy.rocket );
        }
        // enemy-plasma shield collision (no score: the kill will pay, grinding won't):
        if( m_plasmaShield > 0 && Maths::Collision( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius, m_ship.position, m_plasmaShieldRadius ) ) {
            m_audio.PlayAt( eSound::shipCollision, _RelativeToShip( enemy.rocket.position ) );
            _RocketImpact( m_ship, enemy.rocket );
        }
        // enemy-mines collision:
        for( auto & mine : m_mines ) {
            if( mine.alive && Maths::Collision( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius, mine.position, mine.dynamic.radius ) ) {
                mine.alive = false;
                enemy.rocket.shield.value -= mine.damage;
                _AddScore( 5 );
            }
        }
        // enemy-attractors collision (the foreman plows through them, shattering them
        // at the cost of some hull; the others die on contact):
        if( enemy.IsBoss() )
            m_attractors.ForEachInRange( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius,
                [ & ]( const int, Attractor & _attractor ){
                    if( Maths::Collision( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius, _attractor.position, _attractor.radius ) ) {
                        enemy.rocket.shield.value -= enemy.boss.maxShield * foremanAttractorDamage;
                        _attractor.shield -= 1000;
                        _AddExplosion( _attractor.position, enemy.rocket.momentum, eExplosion::medium, eFadeColor::rose );
                    }
                } );
        else
        m_attractors.ForEachInRange( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius,
            [ & ]( const int, Attractor & _attractor ){
                if( Maths::Collision( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius, _attractor.position, _attractor.radius ) ) {
                    enemy.rocket.shield.value = -1;
                    _attractor.shield -= enemy.rocket.damage;
                }
            } );
    }
}


void World::_UpdateLaserCollisions()
{
    for( auto & laser : m_lasers ) {
        Rocket * pCollision{ nullptr };
        // laser-ships collisions:
        for( auto & enemy : m_enemies ) {
            if( pCollision != nullptr )
                break;
            if( enemy.type == Enemy::eType::foreman && _LaserCoreCollision( laser, enemy ) ) {
                pCollision = &enemy.rocket;
                continue;
            }
            if( enemy.type == Enemy::eType::warden && _LaserPylonCollision( laser, enemy ) ) {
                pCollision = &enemy.rocket;
                continue;
            }
            if( _LaserRocketCollision( laser, enemy.rocket ) ) {
                pCollision = &enemy.rocket;
                if( enemy.type == Enemy::eType::foreman )
                    enemy.boss.laserDamage += laser.damage;
                else if( enemy.type == Enemy::eType::warden ) {
                    enemy.boss.laserDamage += laser.damage;
                    enemy.boss.coreFlash = 6; // the barrier (or the exposed core) flashes
                }
                m_audio.PlayAt( eSound::laserCollision, _RelativeToShip( pCollision->position ) );
                _AddScore( 2 ); // hits pay little, kills pay big
            }
        }
        // laser-missiles collisions:
        for( auto & missile : m_missiles ) {
            if( pCollision != nullptr )
                break;
            if( missile.dead || missile.fromShip ) // laser don't destroy ship's missiles
                continue;
            if( _LaserRocketCollision( laser, missile.rocket ) ) {
                pCollision = &missile.rocket;
                _ExplodeMissile( missile );
                _AddScore( 10 );
            }
        }
        if( pCollision != nullptr )
            laser.lifeSpan = Laser::maxLifeSpan;
        // laser-mines collision:
        for( auto & mine : m_mines ) {
            if( !mine.alive )
                continue;
            // too close:
            if( ( mine.position - m_ship.position ).DistanceSquared() < 100 * 100 )
                continue;
            if( !Maths::Collision( mine.position, mine.dynamic.radius, laser.dynamic.positionA, laser.dynamic.positionB ) )
                continue;
            laser.lifeSpan = Laser::maxLifeSpan;
            mine.alive = false;
        }
        // laser-gravity mines collision:
        for( auto & gravityMine : m_gravityMines ) {
            if( !gravityMine.alive )
                continue;
            if( !Maths::Collision( gravityMine.position, gravityMine.dynamic.radius, laser.dynamic.positionA, laser.dynamic.positionB ) )
                continue;
            laser.lifeSpan = Laser::maxLifeSpan;
            gravityMine.alive = false;
            _AddScore( 15 );
        }
        // laser-attractors collision:
        const double laserExtent{ laser.momentum.Distance() };
        m_attractors.ForEachInRange( laser.position, laserExtent,
            [ & ]( const int, Attractor & _attractor ){
                const auto collision{ Maths::Collision( _attractor.position, _attractor.radius, laser.dynamic.positionA, laser.dynamic.positionB ) };
                if( !collision )
                    return;
                laser.lifeSpan = Laser::maxLifeSpan;
                m_audio.PlayAt( eSound::attractorLaserCollision, _RelativeToShip( _attractor.position ) );
                _LaserImpact( laser, *collision, eFadeColor::azure );
                _attractor.shield -= laser.damage;
                _AddScore( 1 );
            } );
    }
}


void World::_UpdateMissileCollisions()
{
    for( auto & missile : m_missiles ) {
        if( missile.dead )
            continue;
        auto & rocket{ missile.rocket };
        bool collision{ false };
        // missiles-missiles collisions:
        for( auto & missileOther : m_missiles ) {
            if( collision )
                break;
            if( &missile == &missileOther || missileOther.dead )
                continue;
            if( _RocketCollision( rocket, missileOther.rocket ) ) {
                collision = true;
                _ExplodeMissile( missileOther );
                _AddScore( 5 ); // making missiles take each other out is a play
            }
        }
        // missiles-enemies collisions:
        for( auto & enemy : m_enemies ) {
            if( collision )
                break;
            collision = _MissileRocketCollision( missile, enemy.rocket );
        }
        // missiles-ship collision:
        if( !collision && !m_shipDestroyed )
            collision = _MissileRocketCollision( missile, m_ship );
        // missiles-decoy collision (the bait takes the hit):
        if( !collision && !missile.fromShip && m_decoyTicks > 0 )
            if( Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, m_decoyPosition, decoyRadius ) ) {
                collision = true;
                m_decoyHp--;
            }
        // missiles-plasma shield collision:
        if( !collision && !missile.fromShip && m_plasmaShield > 0 )
            collision = Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, m_ship.position, m_plasmaShieldRadius );
        // missiles-mines collision:
        if( !missile.fromShip ) // avoid collisions with our own missiles
            for( auto & mine : m_mines )
                if( mine.alive && Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, mine.position, mine.dynamic.radius ) ) {
                    collision = true;
                    mine.alive = false;
                }
        // missiles-attractors collision:
        m_attractors.ForEachInRange( rocket.position, rocket.dynamic.boundingBoxRadius,
            [ & ]( const int, Attractor & _attractor ){
                if( Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, _attractor.position, _attractor.radius ) ) {
                    collision = true;
                    _attractor.shield -= rocket.damage;
                }
            } );
        // explode and remove (no score: this includes enemy missiles hitting the ship):
        if( collision )
            _ExplodeMissile( missile );
    }
    std::erase_if( m_missiles, []( const Missile & _missile ){ return _missile.dead; } );
}


void World::_UpdateShipAttractorCollisions()
{
    if( m_shipDestroyed )
        return;
    m_attractors.ForEachInRange( m_ship.position, m_ship.dynamic.boundingBoxRadius,
        [ & ]( const int, Attractor & _attractor ){
            if( !Maths::Collision( m_ship.position, m_ship.dynamic.boundingBoxRadius, _attractor.position, _attractor.radius ) )
                return;
            m_ship.shield.value -= _attractor.mass * 0.5; // per tick of contact, keep it a graze not a melt
            _attractor.shield -= m_ship.damage;
            m_audio.Play( eSound::attractorShipCollision );
            const auto collisionPoint{ Maths::Collision( _attractor.position, _attractor.radius, _attractor.position, m_ship.position ) };
            if( !collisionPoint )
                return;
            const auto counterMomentum{ Vector{ m_ship.position - _attractor.position } * m_ship.momentum.Distance() * 0.01 };
            _AddExplosion( *collisionPoint, counterMomentum, eExplosion::medium, eFadeColor::azure );
            m_ship.ReceiveImpact( *collisionPoint, counterMomentum, 0.05 * ( _attractor.mass / 15 ) );
        } );
}


void World::_UpdateEnemies()
{
    for( auto & enemy : m_enemies ) {
        const auto soundPosition{ m_audio.Locate( _RelativeToShip( enemy.rocket.position ) ) };

        const auto thrustRatio{ std::min( enemy.rocket.engine.thrust / enemy.rocket.engine.power, 1.0 ) };
        const auto thrustVolume{ thrustRatio * 0.2 }; // [ 0.0, 0.2 ]
        const auto thrustPitch{ std::max( thrustRatio, 0.5 ) }; // [ 0.5, 1.0 ]
        m_audio.SetLoop( enemy.sound_mainEngine, { thrustVolume * soundPosition.volume, soundPosition.pan, thrustPitch } );

        const auto rotatorRatio{ std::min( ( enemy.rocket.rotator.thrust.at( 0 ) + enemy.rocket.rotator.thrust.at( 1 ) ) / ( 2 * enemy.rocket.rotator.power ), 1.0 ) };
        const auto rotatorVolumeThreshold{ 0.25 };
        const auto rotatorVolume{ rotatorRatio < rotatorVolumeThreshold ? 0 : ( rotatorRatio - rotatorVolumeThreshold ) * 0.1 }; // [ 0.0, 0.1 ]
        const auto rotatorPitch{ std::max( rotatorRatio, 0.5 ) }; // [ 0.5, 1.0 ]
        m_audio.SetLoop( enemy.sound_rotationEngine, { rotatorVolume * soundPosition.volume, soundPosition.pan, rotatorPitch } );

        if( m_empTicks > 0 ) {
            // EMP: engines and launchers are down, the enemy drifts ballistically:
            if( m_empTicks % 5 == 0 )
                _AddParticule( enemy.rocket.position + Vector::From( Maths::Random( 0, Maths::Pi2 ), Maths::Random( 0, enemy.rocket.dynamic.boundingBoxRadius ) ),
                    {}, Maths::Random( 0, Maths::Pi2 ), Maths::Random( 0.5, 1.5 ), Maths::Random( 0.5, 1.2 ), eFadeColor::azure );
        }
        else
            _SteerEnemy( enemy );
        if( enemy.type == Enemy::eType::foreman )
            _UpdateForemanDamage( enemy );
        else if( enemy.type == Enemy::eType::warden )
            _UpdateWardenDamage( enemy );
        // the bosses are too heavy to be spun by what hits them: the turn limit applies
        // before the physics step (a hit's rotation impulse would otherwise be integrated
        // for a tick) and after it (the steering adds its own)
        const auto maxTurn{ enemy.type == Enemy::eType::warden ? wardenMaxTurn : ( enemy.type == Enemy::eType::foreman ? ( enemy.CoreOpen() ? foremanVentTurn : foremanMaxTurn ) : 0.0 ) };
        if( enemy.IsBoss() )
            enemy.rocket.rotationMomentum = std::clamp( enemy.rocket.rotationMomentum, -maxTurn, maxTurn );
        enemy.rocket.Update();
        if( enemy.IsBoss() )
            enemy.rocket.rotationMomentum = std::clamp( enemy.rocket.rotationMomentum, -maxTurn, maxTurn );

        // shield:
        if( enemy.rocket.shield.value <= 0 ) {
            m_audio.PlayAt( eSound::shipExplosion, _RelativeToShip( enemy.rocket.position ) );
            switch( enemy.type ) {
                case Enemy::eType::wasp: // chaff: small blast, no goody
                    _Detonate( enemy.rocket.position, enemy.rocket.momentum, eExplosion::medium, eFadeColor::orange, waspBlastRadius, waspBlastDamage, waspBlastImpulse );
                    _AddScore( 40 );
                    break;
                case Enemy::eType::sniper:
                    _MaybeDropGoody( enemy.rocket.position );
                    _Detonate( enemy.rocket.position, enemy.rocket.momentum, eExplosion::big, eFadeColor::orange, enemyBlastRadius, enemyBlastDamage, enemyBlastImpulse );
                    _AddScore( 150 );
                    break;
                case Enemy::eType::foreman: {
                    // a friendly blast wipes the surroundings, the hull bursts in a ring, and a laser upgrade is left behind:
                    _Detonate( enemy.rocket.position, enemy.rocket.momentum, eExplosion::big, eFadeColor::orange, 320, 2, 14, true );
                    m_audio.PlayAt( eSound::attractorExplosion, _RelativeToShip( enemy.rocket.position ) );
                    m_audio.PlayAt( eSound::mineExplosion, _RelativeToShip( enemy.rocket.position ) );
                    for( int i{ 0 }; i < 14; i++ ) {
                        const auto angle{ Maths::Pi2 * i / 14 };
                        _AddExplosion( enemy.rocket.position + Vector::From( angle, enemy.rocket.dynamic.bodyRadius * 0.8 ), Vector::From( angle, 1.5 ), eExplosion::big,
                            i % 2 == 0 ? eFadeColor::orange : eFadeColor::rose );
                    }
                    if( m_laserSpeed != eLaserSpeed::fast || m_laserPass != eLaserPass::height )
                        m_goodies.emplace_back( Goody{ enemy.rocket.position, Goody::eType::laserUp } );
                    _AddScore( 1000 );
                    m_stage.OnBossDefeated();
                    break;
                }
                case Enemy::eType::warden: {
                    // the core collapses: a huge friendly blast, rings of fire, a laser upgrade
                    _Detonate( enemy.rocket.position, enemy.rocket.momentum, eExplosion::big, eFadeColor::violet, 420, 2, 16, true );
                    m_audio.PlayAt( eSound::attractorExplosion, _RelativeToShip( enemy.rocket.position ) );
                    m_audio.PlayAt( eSound::mineExplosion, _RelativeToShip( enemy.rocket.position ) );
                    for( int ring{ 0 }; ring < 2; ring++ )
                        for( int i{ 0 }; i < 16; i++ ) {
                            const auto angle{ Maths::Pi2 * i / 16 + ring * 0.2 };
                            _AddExplosion( enemy.rocket.position + Vector::From( angle, enemy.rocket.dynamic.bodyRadius * ( 0.5 + ring * 0.7 ) ), Vector::From( angle, 1.5 + ring ), eExplosion::big,
                                i % 2 == 0 ? eFadeColor::violet : eFadeColor::azure );
                        }
                    if( m_laserSpeed != eLaserSpeed::fast || m_laserPass != eLaserPass::height )
                        m_goodies.emplace_back( Goody{ enemy.rocket.position, Goody::eType::laserUp } );
                    _AddScore( 2000 );
                    m_stage.OnBossDefeated();
                    break;
                }
                case Enemy::eType::chaser:
                default:
                    _MaybeDropGoody( enemy.rocket.position );
                    _Detonate( enemy.rocket.position, enemy.rocket.momentum, eExplosion::big, eFadeColor::orange, enemyBlastRadius, enemyBlastDamage, enemyBlastImpulse );
                    _AddScore( 100 );
                    break;
            }
            enemy.dead = true;
            m_audio.StopLoop( enemy.sound_mainEngine );
            m_audio.StopLoop( enemy.sound_rotationEngine );
            if( !m_shipDestroyed )
                m_stage.OnKill();
            if( !enemy.IsBoss() )
                m_pendingSpawns.emplace_back( PendingSpawn{ enemy.type, static_cast< int >( m_stage.Mission().respawnSeconds * m_frameRate ) } );
            continue;
        }

        // cadence actions - missiles, mines, slugs (all down during an EMP):
        if( m_empTicks == 0 )
            _EnemyAction( enemy );
    }
    std::erase_if( m_enemies, []( const Enemy & _enemy ){ return _enemy.dead; } );

    // the wasps released by the foreman (requested during the pass: spawning
    // here keeps the references of the loop valid):
    for( ; m_bossWasps > 0; m_bossWasps-- ) {
        _AddEnemy( Enemy::eType::wasp );
        m_enemies.back().rocket.position = m_bossSpawn + Vector::From( Maths::Random( 0, Maths::Pi2 ), 120 );
    }
    _RefillEnemies();
    if( m_debugPlaceBoss )
        for( auto & enemy : m_enemies )
            if( enemy.IsBoss() ) {
                enemy.rocket.position = m_ship.position + Vector{ 520, -100 };
                m_debugPlaceBoss = false;
            }
}


void World::_RefillEnemies()
{
    // the stage decides what is missing; a destroyed enemy is replaced after its
    // delay, anything else missing (mission start...) shows up right away:
    if( m_debugSandbox )
        return;
    for( auto & pending : m_pendingSpawns )
        pending.ticks--;
    for( const auto type : m_stage.Missing( m_enemies ) ) {
        const auto pending{ std::ranges::find_if( m_pendingSpawns, [ & ]( const PendingSpawn & _pending ){ return _pending.type == type; } ) };
        if( pending == m_pendingSpawns.end() )
            _AddEnemy( type );
        else if( pending->ticks <= 0 ) {
            _AddEnemy( type );
            m_pendingSpawns.erase( pending );
        }
    }
}


void World::_UpdateMission()
{
    if( m_shipDestroyed ) // the wreck does not progress
        return;
    if( m_stage.Update() == StageDirector::eEvent::cleared )
        _OnMissionCleared();
    // boss alert: the announcer goes with the flashing banner
    if( m_stage.Phase() == StageDirector::ePhase::intro && m_stage.IsBoss() && m_stage.PhaseTicks() == 1 )
        m_audio.Play( m_stage.Mission().roster.front().type == Enemy::eType::warden ? eSound::bossWarningWarden : eSound::bossWarningForeman );

    // the attractor field fills in with the missions, out of the player's sight:
    m_attractors.SetDensity( m_debugSandbox ? 0 : m_stage.Mission().attractorDensity );
    m_attractors.ApplyDensity( m_ship.position, static_cast< double >( std::max( m_screenDimension.width, m_screenDimension.height ) ) + m_attractors.MaxRadius() );
}


void World::DebugBossInView()
{
    m_debugPlaceBoss = true;
    DebugSkip(); // the briefing banner, the boss comes with the fight
}


void World::DebugSingularity()
{
    // the mines sandbox, without the mines: _UpdateSingularity deploys the singularities
    DebugMinesLayout();
    m_mines.clear();
    m_gravityMines.clear();
    m_debugSingularity = true;
    m_debugSingularityWait = m_frameRate;
}


void World::DebugMinesLayout()
{
    // a sandbox to look at the mines: no enemy, no attractor, the ship is invincible; the player's
    // mines in a row above the ship, the gravity mines (which pull the others) in a row below
    m_debugSandbox = true;
    m_debugInvincible = true;
    for( auto & enemy : m_enemies ) {
        m_audio.StopLoop( enemy.sound_mainEngine );
        m_audio.StopLoop( enemy.sound_rotationEngine );
    }
    m_enemies.clear();
    m_pendingSpawns.clear();
    m_attractors.SetDensity( 0 );
    m_attractors.ApplyDensity( m_ship.position, 0 );
    constexpr int count{ 4 };
    for( int i{ 0 }; i < count; i++ ) {
        const auto offset{ i - ( count - 1 ) * 0.5 };
        m_mines.emplace_back( Mine{ m_ship.position + Vector{ offset * 180, -250 }, 0.5 } );
        m_gravityMines.emplace_back( GravityMine{ m_ship.position + Vector{ offset * 330, 250 }, Maths::Random( 0, Maths::Pi2 ) } );
    }
}


void World::DebugSkip()
{
    // banners are skipped; an objective is brought to its last kill (or last seconds);
    // a boss is brought just above its next phase threshold
    if( m_stage.Phase() == StageDirector::ePhase::freePlay )
        return;
    if( m_stage.Phase() != StageDirector::ePhase::active ) {
        m_stage.DebugSkipBanner();
        return;
    }
    if( !m_stage.IsBoss() ) {
        m_stage.DebugJumpAlmostDone( m_stage.Progress() );
        return;
    }
    for( auto & enemy : m_enemies ) {
        if( !enemy.IsBoss() )
            continue;
        auto & boss{ enemy.boss };
        auto & shield{ enemy.rocket.shield.value };
        if( enemy.type == Enemy::eType::warden && enemy.PylonsAlive() > 0 ) {
            for( int i{ 0 }; i < Enemy::pylonCount; i++ )
                if( enemy.PylonAlive( i ) ) {
                    boss.pylonShield.at( static_cast< size_t >( i ) ) = boss.pylonMax * 0.03;
                    break;
                }
            continue;
        }
        const auto ratio{ enemy.type == Enemy::eType::warden || boss.phase == 2 ? 0.04 : ( boss.phase == 0 ? 0.67 : 0.34 ) };
        shield = std::min( shield, boss.maxShield * ratio );
        boss.lastShield = shield;
    }
}


void World::Upgrade( const ShipUpgrades::eSystem _system )
{
    if( !m_upgrades.CanUpgrade( _system ) )
        return;
    const double enginePower{ m_ship.engine.power };
    ShipUpgrades::ApplyLevel( m_ship, _system );
    m_shipBaseEnginePower += m_ship.engine.power - enginePower; // overdrive restores the upgraded power
    m_ship.RefreshGeometry();
    m_upgrades.levels.at( static_cast< size_t >( _system ) )++;
    m_upgrades.points--;
    m_checkpoint.upgrades = m_upgrades; // a retry keeps what was spent
    m_audio.Play( eSound::laserPowerUp ); // TODO dedicated sound (see TODO.md)
}


void World::_OnMissionCleared()
{
    // a skill point per cleared mission (spent on the upgrade screen, see Game):
    m_upgrades.points++;

    // retry restarts the NEXT mission, with the gear as it stands now:
    m_checkpoint = { m_stage.NextProgress(), m_score, static_cast< int >( m_laserSpeed ), static_cast< int >( m_laserPass ), m_homingMissiles, m_magneticMines, m_lives, m_upgrades };

    // debriefing: repairs, and the surviving hostiles are wiped out harmlessly (no
    // score, no goody) so the banner is a real breather:
    m_ship.shield.value = m_ship.shield.capacity;
    m_ship.propellant.value = m_ship.propellant.capacity;
    m_audio.Play( m_stage.IsBoss() ? eSound::stageCleared : eSound::missionCleared );
    for( auto & enemy : m_enemies ) {
        _AddExplosion( enemy.rocket.position, enemy.rocket.momentum, eExplosion::big );
        m_audio.PlayAt( eSound::shipExplosion, _RelativeToShip( enemy.rocket.position ) );
        m_audio.StopLoop( enemy.sound_mainEngine );
        m_audio.StopLoop( enemy.sound_rotationEngine );
        enemy.dead = true;
    }
    std::erase_if( m_enemies, []( const Enemy & _enemy ){ return _enemy.dead; } );
    m_pendingSpawns.clear();
    for( auto & missile : m_missiles )
        if( !missile.fromShip && !missile.dead ) {
            _AddExplosion( missile.rocket.position, missile.rocket.momentum, eExplosion::small );
            m_audio.StopLoop( missile.sound_run );
            missile.dead = true;
        }
    for( auto & slug : m_slugs )
        slug.dead = true;
}


void World::_UpdateForemanDamage( Enemy & _enemy )
{
    auto & boss{ _enemy.boss };
    auto & shield{ _enemy.rocket.shield.value };

    // lasers (and especially the rear core hits) count full, anything else is reduced:
    const auto taken{ std::max( boss.lastShield - shield, 0.0 ) };
    const auto other{ std::max( taken - boss.laserDamage, 0.0 ) };
    shield = boss.lastShield - std::min( taken, boss.laserDamage ) - other * foremanOtherDamageScale;
    boss.lastShield = shield;
    boss.laserDamage = 0;
    if( boss.coreFlash > 0 )
        boss.coreFlash--;

    // the rear core vents periodically: armored, then a telegraphed warning, then exposed
    // for a few seconds while the barge freezes (the windows come faster in later phases):
    if( boss.ventTimer > 0 )
        boss.ventTimer--;
    else if( boss.vent == 0 ) {
        boss.vent = 1;
        boss.ventTimer = static_cast< int >( m_frameRate * foremanVentWarningSeconds );
        m_audio.Play( eSound::foremanVent );
    }
    else if( boss.vent == 1 ) {
        boss.vent = 2;
        boss.ventTimer = m_frameRate * foremanVentOpenSeconds;
        boss.ventCadence = 0;
        m_audio.PlayAt( eSound::attractorExplosion, _RelativeToShip( _enemy.rocket.position ) );
    }
    else {
        boss.vent = 0;
        boss.ventTimer = m_frameRate * ( foremanVentClosedSeconds - 2 * boss.phase );
    }

    // phases follow the shield; each new one bursts the hull with a friendly shockwave:
    const auto ratio{ shield / boss.maxShield };
    const auto phase{ ratio > 2.0 / 3 ? 0 : ( ratio > 1.0 / 3 ? 1 : 2 ) };
    if( phase > boss.phase ) {
        boss.phase = phase;
        m_audio.Play( phase == 1 ? eSound::foremanPhase2 : eSound::foremanPhase3 );
        m_audio.PlayAt( eSound::attractorExplosion, _RelativeToShip( _enemy.rocket.position ) );
        _Detonate( _enemy.rocket.position, {}, eExplosion::big, eFadeColor::rose, 280, 0, 12, true );
        for( int i{ 0 }; i < 8; i++ ) {
            const auto angle{ Maths::Pi2 * i / 8 };
            _AddExplosion( _enemy.rocket.position + Vector::From( angle, _enemy.rocket.dynamic.bodyRadius ), Vector::From( angle, 1.2 ), eExplosion::medium, eFadeColor::rose );
        }
    }
}


void World::_SteerForeman( Enemy & _enemy )
{
    // a slow, heavy barge: closes in to its holding distance, then circles the ship
    // while its head keeps facing it (turning slowly: the stern can be outflanked)
    auto & rocket{ _enemy.rocket };
    const auto delta{ m_ship.position - rocket.position };
    const auto distance{ std::max( delta.Distance(), 1.0 ) };
    const auto toShip{ delta * ( 1.0 / distance ) };
    const Vector tangent{ -toShip.v * _enemy.boss.orbit, toShip.u * _enemy.boss.orbit };
    const auto speed{ foremanSpeed * ( _enemy.boss.phase == 2 ? 1.5 : 1.0 ) * ( _enemy.CoreOpen() ? foremanVentSpeedScale : 1.0 ) };
    const auto radial{ std::clamp( ( distance - foremanHoldDistance ) * 0.01, -speed, speed ) };
    const auto desired{ toShip * radial + tangent * ( speed * 0.6 ) };
    const auto mass{ std::max( rocket.dynamic.totalMass, 1.0 ) };
    rocket.thrustMotion += ( desired * mass - rocket.thrustMotion ) * 0.03; // velocity control, heavy inertia
    rocket.PointTo( m_ship.position, 0.3 );
}


void World::_ForemanAction( Enemy & _enemy )
{
    if( m_shipDestroyed )
        return;
    auto & rocket{ _enemy.rocket };
    auto & boss{ _enemy.boss };
    const auto delta{ m_ship.position - rocket.position };
    const auto distance{ delta.Distance() };
    const auto fireSlug{ [ & ]( const double _angle ){
            m_slugs.emplace_back( Slug{ rocket.position + Vector::From( _angle, rocket.dynamic.boundingBoxRadius + 12 ), Vector::From( _angle, slugSpeed ) } );
            m_audio.PlayAt( eSound::laserShot, _RelativeToShip( rocket.position ) );
        } };

    // vented: the aimed barrage stops, the barge lashes out through its rear instead
    if( boss.vent == 2 ) {
        if( boss.ventCadence++ % 40 == 0 ) {
            const auto count{ boss.phase == 2 ? 9 : 7 };
            for( int i{ 0 }; i < count; i++ )
                fireSlug( rocket.orientation + ( i - ( count - 1 ) * 0.5 ) * 0.3 );
        }
        return;
    }

    // slugs: aimed fans (3 then 5 slugs), and in the enraged phase a rotating spiral
    // alternating with the fans:
    const bool spiralling{ boss.phase == 2 && ( boss.cycle++ % ( m_frameRate * 7 ) ) < m_frameRate * 4 };
    if( spiralling ) {
        if( boss.spiralTicks++ % 6 == 0 ) {
            fireSlug( boss.spiral );
            boss.spiral += 0.42;
        }
    }
    else if( distance < 800 && boss.slugCadence++ > ( boss.phase == 0 ? 110 : 85 ) ) {
        boss.slugCadence = 0;
        const auto predicted{ m_ship.position + m_ship.momentum * ( distance / slugSpeed ) };
        const auto aim{ ( predicted - rocket.position ).Orientation() };
        const auto fan{ boss.phase == 0 ? 1 : 2 }; // slugs each side of the aim
        for( int i{ -fan }; i <= fan; i++ )
            fireSlug( aim + i * 0.16 );
    }

    // homing missiles, in pairs:
    const auto missilePeriod{ m_frameRate * ( boss.phase == 0 ? 6 : ( boss.phase == 1 ? 5 : 4 ) ) };
    if( distance < 900 && boss.missileCadence++ > missilePeriod ) {
        boss.missileCadence = 0;
        for( const auto offset : { -0.6, 0.6 } )
            _SpawnMissile( rocket, true, offset, rocket.dynamic.boundingBoxRadius + 24, 3 );
        m_audio.PlayAt( eSound::missileShot, _RelativeToShip( rocket.position ) );
    }

    if( boss.phase == 0 )
        return;

    // second phase: wasps released in pairs...
    if( boss.waspCadence++ > m_frameRate * 5 ) {
        boss.waspCadence = 0;
        const auto wasps{ static_cast< size_t >( std::ranges::count_if( m_enemies, []( const Enemy & _other ){ return !_other.dead && _other.type == Enemy::eType::wasp; } ) ) };
        if( wasps < foremanMaxWasps ) {
            m_bossWasps = 2;
            m_bossSpawn = rocket.position;
        }
    }
    // ...and gravity mines laid in its wake, behind the rear core:
    if( boss.mineCadence++ > m_frameRate * 7 / 2 ) {
        boss.mineCadence = 0;
        if( m_gravityMines.size() < static_cast< size_t >( foremanMaxMines ) )
            m_gravityMines.emplace_back( GravityMine{ rocket.position + Vector::From( rocket.orientation, rocket.dynamic.boundingBoxRadius + 90 ), Maths::Random( 0, Maths::Pi2 ) } );
    }
}


void World::_UpdateWardenDamage( Enemy & _enemy )
{
    auto & boss{ _enemy.boss };
    auto & shield{ _enemy.rocket.shield.value };

    // the pylons shield the core completely; once they are down, lasers count full and
    // anything else is reduced:
    const auto alive{ _enemy.PylonsAlive() };
    if( alive > 0 )
        shield = boss.lastShield;
    else {
        const auto taken{ std::max( boss.lastShield - shield, 0.0 ) };
        const auto other{ std::max( taken - boss.laserDamage, 0.0 ) };
        shield = boss.lastShield - std::min( taken, boss.laserDamage ) - other * wardenOtherDamageScale;
    }
    boss.lastShield = shield;
    boss.laserDamage = 0;
    if( boss.coreFlash > 0 )
        boss.coreFlash--;
    for( auto & flash : boss.pylonFlash )
        flash = std::max( flash - 1, 0 );

    // the phases are the fallen pylons; each one bursts the central hull with a shockwave:
    const auto phase{ Enemy::pylonCount - alive };
    if( phase > boss.phase ) {
        boss.phase = phase;
        m_audio.Play( phase == Enemy::pylonCount ? eSound::wardenCoreExposed : eSound::wardenPylon );
        _Detonate( _enemy.rocket.position, {}, eExplosion::big, eFadeColor::violet, 300, 0, 12, true );
        for( int i{ 0 }; i < 8; i++ ) {
            const auto angle{ Maths::Pi2 * i / 8 };
            _AddExplosion( _enemy.rocket.position + Vector::From( angle, _enemy.rocket.dynamic.bodyRadius ), Vector::From( angle, 1.2 ), eExplosion::medium, eFadeColor::violet );
        }
        if( phase == Enemy::pylonCount )
            boss.cycle = 0; // the enraged cycle starts clean
    }
}


void World::_SteerWarden( Enemy & _enemy )
{
    // a slow sentinel: holds its distance from the ship, drifting sideways, its pylons
    // turning faster as they fall
    auto & rocket{ _enemy.rocket };
    auto & boss{ _enemy.boss };
    boss.pylonAngle += wardenPylonSpin * ( 1 + boss.phase * 0.35 );
    boss.orbitAzimuth += wardenOrbitPrecession;
    boss.orbitTilt = wardenOrbitTilt + wardenOrbitWobble * std::sin( boss.orbitAzimuth * 2.3 ); // keeps the pylons clear of the core
    const auto delta{ m_ship.position - rocket.position };
    const auto distance{ std::max( delta.Distance(), 1.0 ) };
    const auto toShip{ delta * ( 1.0 / distance ) };
    const Vector tangent{ -toShip.v * boss.orbit, toShip.u * boss.orbit };
    const auto speed{ wardenSpeed * ( boss.phase == Enemy::pylonCount ? 1.6 : 1.0 ) };
    const auto radial{ std::clamp( ( distance - wardenHoldDistance ) * 0.01, -speed, speed ) };
    const auto desired{ toShip * radial + tangent * ( speed * 0.35 ) };
    const auto mass{ std::max( rocket.dynamic.totalMass, 1.0 ) };
    rocket.thrustMotion += ( desired * mass - rocket.thrustMotion ) * 0.03;
    rocket.PointTo( m_ship.position, 0.3 );
}


void World::_WardenAction( Enemy & _enemy )
{
    if( m_shipDestroyed )
        return;
    auto & rocket{ _enemy.rocket };
    auto & boss{ _enemy.boss };
    const auto delta{ m_ship.position - rocket.position };
    const auto distance{ delta.Distance() };
    const auto phase{ boss.phase };
    const auto fire{ [ & ]( const Vector & _from, const double _angle ){
            m_slugs.emplace_back( Slug{ _from + Vector::From( _angle, 14 ), Vector::From( _angle, slugSpeed ) } );
            m_audio.PlayAt( eSound::laserShot, _RelativeToShip( _from ) );
        } };
    const auto aimFrom{ [ & ]( const Vector & _from ){
            const auto predicted{ m_ship.position + m_ship.momentum * ( ( m_ship.position - _from ).Distance() / slugSpeed ) };
            return ( predicted - _from ).Orientation();
        } };

    // the live pylons take turns firing aimed slugs, in bigger fans as their number drops:
    const auto alive{ _enemy.PylonsAlive() };
    if( alive > 0 && distance < 1000 && boss.slugCadence++ > 48 - phase * 8 ) {
        boss.slugCadence = 0;
        for( int tries{ 0 }; tries < Enemy::pylonCount; tries++ ) {
            boss.cycle = ( boss.cycle + 1 ) % Enemy::pylonCount;
            if( !_enemy.PylonAlive( boss.cycle ) )
                continue;
            const auto from{ _enemy.PylonPosition( boss.cycle ) };
            const auto aim{ aimFrom( from ) };
            const auto fan{ phase == 0 ? 0 : phase }; // slugs each side of the aim
            for( int i{ -fan }; i <= fan; i++ )
                fire( from, aim + i * 0.18 );
            break;
        }
    }

    // the core's own homing missiles, in salvos:
    const auto missilePeriod{ m_frameRate * ( phase == 0 ? 8 : ( phase == 1 ? 6 : ( phase == 2 ? 5 : 4 ) ) ) };
    if( distance < 1000 && boss.missileCadence++ > missilePeriod ) {
        boss.missileCadence = 0;
        const auto salvo{ phase == 0 ? 2 : ( phase == 3 ? 4 : 3 ) };
        for( int i{ 0 }; i < salvo; i++ )
            _SpawnMissile( rocket, true, ( i - ( salvo - 1 ) * 0.5 ) * 0.5, rocket.dynamic.boundingBoxRadius + 24, 3 );
        m_audio.PlayAt( eSound::missileShot, _RelativeToShip( rocket.position ) );
    }

    // the belt as a weapon: gravity mines laid on a ring around the core, closing the arena
    if( phase >= 1 && boss.mineCadence++ > m_frameRate * ( phase == 1 ? 5 : 3 ) ) {
        boss.mineCadence = 0;
        if( m_gravityMines.size() < static_cast< size_t >( wardenMaxMines ) )
            m_gravityMines.emplace_back( GravityMine{ rocket.position + Vector::From( Maths::Random( 0, Maths::Pi2 ), Maths::Random( 380, 560 ) ), Maths::Random( 0, Maths::Pi2 ) } );
    }

    // from the second fallen pylon, wasps pour out of the core:
    if( phase >= 2 && boss.waspCadence++ > m_frameRate * ( phase == 2 ? 7 : 5 ) ) {
        boss.waspCadence = 0;
        const auto wasps{ static_cast< size_t >( std::ranges::count_if( m_enemies, []( const Enemy & _other ){ return !_other.dead && _other.type == Enemy::eType::wasp; } ) ) };
        if( wasps < wardenMaxWasps ) {
            m_bossWasps = 2;
            m_bossSpawn = rocket.position;
        }
    }

    // the last pylon and the exposed core pulse rings of slugs (a rotating ring, then a gap)
    if( phase >= 2 ) {
        const auto period{ m_frameRate * ( phase == 2 ? 5 : 3 ) };
        if( boss.ventCadence++ > period ) {
            boss.ventCadence = 0;
            const auto count{ phase == 2 ? 12 : 18 };
            boss.spiral += 0.21;
            for( int i{ 0 }; i < count; i++ )
                fire( rocket.position + Vector::From( boss.spiral + Maths::Pi2 * i / count, rocket.dynamic.bodyRadius ), boss.spiral + Maths::Pi2 * i / count );
        }
    }

    // enraged core: a slow slug spiral on top of the rings
    if( phase == Enemy::pylonCount && boss.spiralTicks++ % 7 == 0 ) {
        fire( rocket.position, boss.spiralTicks * 0.19 );
    }
}


void World::_SteerEnemy( Enemy & _enemy )
{
    auto & rocket{ _enemy.rocket };
    switch( _enemy.type ) {
        case Enemy::eType::foreman:
            _SteerForeman( _enemy );
            break;
        case Enemy::eType::warden:
            _SteerWarden( _enemy );
            break;
        case Enemy::eType::wasp:
            // kamikaze: aims straight at the ship
            rocket.Acquire( m_ship, 0.6 );
            rocket.ActivateThrust();
            break;
        case Enemy::eType::sniper: {
            // static gun platform: creeps closer only until parked inside the player's
            // screen, then BRAKES HARD (otherwise the accumulated motion would coast it
            // straight across and out of view), holds position and tracks; it never
            // flees, so closing in to kill it always works
            const auto delta{ rocket.position - m_ship.position };
            if( std::abs( delta.u ) > m_screenCenter.u - sniperStationMargin
                || std::abs( delta.v ) > m_screenCenter.v - sniperStationMargin ) {
                rocket.Acquire( m_ship, 0.3 );
                rocket.ActivateThrust();
            }
            else {
                rocket.thrustMotion *= 0.9; // station-keeping thrusters
                rocket.PointTo( m_ship.position, 0.4 );
            }
            break;
        }
        case Enemy::eType::chaser:
        default:
            // better NOT aim directly the ship to avoid collisions:
            rocket.Acquire( m_ship, 0.5, Vector::From( m_ship.orientation + Maths::Pi, 100 ) );
            rocket.ActivateThrust();
            break;
    }
}


void World::_EnemyAction( Enemy & _enemy )
{
    switch( _enemy.type ) {
        case Enemy::eType::foreman:
            _ForemanAction( _enemy );
            break;
        case Enemy::eType::warden:
            _WardenAction( _enemy );
            break;
        case Enemy::eType::chaser:
            // launches a homing missile aiming the ship every 5 seconds:
            if( _enemy.shotRate++ > m_frameRate * 5 ) {
                _enemy.shotRate = 0;
                _SpawnMissile( _enemy.rocket, true );
                m_audio.PlayAt( eSound::missileShot, _RelativeToShip( _enemy.rocket.position ) );
            }
            break;
        case Enemy::eType::sniper:
            // fires a fast dumb slug at the ship's PREDICTED position, but only while
            // actually visible on the player's screen:
            if( _enemy.shotRate++ > sniperFirePeriodTicks ) {
                const auto delta{ _enemy.rocket.position - m_ship.position };
                if( m_shipDestroyed
                    || std::abs( delta.u ) > m_screenCenter.u - sniperFireMargin
                    || std::abs( delta.v ) > m_screenCenter.v - sniperFireMargin )
                    return; // hold fire, the cadence keeps accumulating
                _enemy.shotRate = 0;
                const auto distance{ delta.Distance() };
                const auto predicted{ m_ship.position + m_ship.momentum * ( distance / slugSpeed ) };
                const auto aim{ ( predicted - _enemy.rocket.position ).Orientation() + Maths::Random( -slugSpread, slugSpread ) };
                m_slugs.emplace_back( Slug{ _enemy.rocket.position + Vector::From( aim, _enemy.rocket.dynamic.boundingBoxRadius + 12 ),
                    Vector::From( aim, slugSpeed ) } );
                m_audio.PlayAt( eSound::laserShot, _RelativeToShip( _enemy.rocket.position ) );
            }
            break;
        case Enemy::eType::wasp:
        default:
            break; // ramming IS the action
    }
}


void World::_UpdateMines()
{
    for( auto & mine : m_mines )
        if( !mine.alive ) {
            m_audio.PlayAt( eSound::mineExplosion, _RelativeToShip( mine.position ) );
            _Detonate( mine.position, {}, eExplosion::big, eFadeColor::orange, mineBlastRadius, mineBlastDamage, mineBlastImpulse );
        }
    std::erase_if( m_mines, []( const Mine & _mine ){ return !_mine.alive; } );

    // grow animation (drives the collision radius):
    for( auto & mine : m_mines )
        mine.Update();

    // mines wind drift, gravity mines drag, attractor pull and attraction:
    for( auto & mine : m_mines ) {
        mine.position += m_wind.At( mine.position ) * 0.5;
        for( const auto & gravityMine : m_gravityMines )
            mine.position += gravityMine.Attraction( mine.position ) * 15;
        mine.position += _AttractorPull( mine.position, attractorPullObjectMass ) * attractorPullDriftFactor;
        if( _TouchesAttractor( mine.position, mine.dynamic.radius ) ) { // sank into an attractor
            mine.alive = false;
            continue;
        }
        const Rocket * pTarget{ _ClosestEnemy( mine.position ) };
        if( pTarget != nullptr )
            mine.position += pTarget->position.InfiniteAttraction( mine.position, pTarget->dynamic.totalMass );
    }
}


void World::_UpdateGravityMines()
{
    // animation and contact detonation (the pull drags victims into it):
    for( auto & gravityMine : m_gravityMines ) {
        if( !gravityMine.alive )
            continue;
        gravityMine.Update();
        // the attractor fields pull the gravity mines themselves:
        gravityMine.position += _AttractorPull( gravityMine.position, attractorPullObjectMass ) * attractorPullDriftFactor;
        if( _TouchesAttractor( gravityMine.position, gravityMine.dynamic.radius ) ) {
            gravityMine.alive = false;
            continue;
        }
        // ship (an active plasma shield triggers the mine at its edge):
        if( !m_shipDestroyed ) {
            const auto shipRadius{ m_plasmaShield > 0 ? m_plasmaShieldRadius : m_ship.dynamic.boundingBoxRadius };
            if( Maths::Collision( gravityMine.position, gravityMine.dynamic.radius, m_ship.position, shipRadius ) )
                gravityMine.alive = false;
        }
        for( const auto & enemy : m_enemies )
            if( Maths::Collision( gravityMine.position, gravityMine.dynamic.radius, enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius ) )
                gravityMine.alive = false;
        for( const auto & missile : m_missiles )
            if( !missile.dead && Maths::Collision( gravityMine.position, gravityMine.dynamic.radius, missile.rocket.position, missile.rocket.dynamic.boundingBoxRadius ) )
                gravityMine.alive = false;
    }

    // detonate the triggered ones (contact, laser, blast chain):
    for( const auto & gravityMine : m_gravityMines )
        if( !gravityMine.alive ) {
            m_audio.PlayAt( eSound::mineExplosion, _RelativeToShip( gravityMine.position ) );
            _Detonate( gravityMine.position, {}, eExplosion::big, eFadeColor::rose, gravityMineBlastRadius, gravityMineBlastDamage, gravityMineBlastImpulse );
        }
    std::erase_if( m_gravityMines, []( const GravityMine & _gravityMine ){ return !_gravityMine.alive; } );

    // population maintenance around the ship:
    const auto maxDimension{ static_cast< double >( std::max( m_screenDimension.width, m_screenDimension.height ) ) };
    if( !m_debugSandbox )
        std::erase_if( m_gravityMines, [ & ]( const GravityMine & _gravityMine ){
                const auto limit{ gravityMineDespawn * maxDimension };
                return ( _gravityMine.position - m_ship.position ).DistanceSquared() > limit * limit; // left far behind, silently recycled
            } );
    if( !m_debugSandbox && m_gravityMines.size() < m_stage.Mission().gravityMines ) { // at most one spawn attempt per tick
        const auto position{ m_ship.position + Vector::From( Maths::Random( 0, Maths::Pi2 ), Maths::Random( gravityMineSpawnMin, gravityMineSpawnMax ) * maxDimension ) };
        bool valid{ true };
        for( const auto & other : m_gravityMines )
            if( ( other.position - position ).DistanceSquared() < gravityMineSeparation * gravityMineSeparation ) {
                valid = false;
                break;
            }
        if( valid ) // keep clear of the attractors:
            m_attractors.ForEachInRange( position, 100, [ & ]( const int, const Attractor & ){ valid = false; } );
        if( valid )
            m_gravityMines.emplace_back( GravityMine{ position, Maths::Random( 0, Maths::Pi2 ) } );
    }
}


void World::_UpdateAttractorsDeletion()
{
    auto & attractors{ m_attractors.All() };
    for( size_t i{ 0 }; i < attractors.size(); i++ ) {
        auto & attractor{ attractors.at( i ) };
        if( !attractor.alive || attractor.shield >= 0 )
            continue;
        _MaybeDropGoody( attractor.position );
        _AddScore( 25 ); // cracking an attractor open is an investment
        m_audio.PlayAt( eSound::attractorExplosion, _RelativeToShip( attractor.position ) );
        _Detonate( attractor.position, {}, eExplosion::medium, eFadeColor::azure, attractor.radius * attractorBlastRadiusFactor, attractorBlastDamage, attractorBlastImpulse );
        for( int j{ 0 }; j < 2; j++ ) {
            const int divisions{ 8 };
            for( int k{ 0 }; k < divisions; k++ ) {
                const double angle{ Maths::Pi2 * k / divisions };
                const double currentRadius{ attractor.radius * ( 1.0 / static_cast< double >( j + 1 ) ) };
                const auto explosionPosition{ attractor.position + Vector::From( angle, currentRadius ) };
                _AddExplosion( explosionPosition, ( explosionPosition - attractor.position ) * 0.03, eExplosion::medium, eFadeColor::azure );
            }
        }
        m_attractors.Remove( i );
    }
}


void World::_UpdateGoodies()
{
    // pickup:
    if( !m_shipDestroyed )
        std::erase_if( m_goodies, [ this ]( const Goody & _goody ){
                if( !Maths::Collision( m_ship.position, m_ship.dynamic.boundingBoxRadius, _goody.position, _goody.dynamic.radius ) )
                    return false;
                _CollectGoody( _goody.type );
                return true;
            } );

    // grow animation, wind drift and attraction toward the ship:
    for( auto & goody : m_goodies ) {
        goody.Update();
        goody.position += m_wind.At( goody.position ) * 0.5;
        goody.position += m_ship.position.InfiniteAttraction( goody.position, m_ship.dynamic.totalMass );
    }
}


void World::_UpdateBonuses()
{
    // EMP shutdown of the enemies:
    if( m_empTicks > 0 )
        m_empTicks--;

    // overdrive: free propellant and boosted main engine while running:
    if( m_overdriveTicks > 0 ) {
        m_overdriveTicks--;
        m_ship.consumptionFactor = 0;
        m_ship.engine.power = m_shipBaseEnginePower * overdrivePowerBoost;
        if( m_overdriveTicks == 0 ) {
            m_ship.consumptionFactor = 1;
            m_ship.engine.power = m_shipBaseEnginePower;
            m_ship.engine.thrust = std::min( m_ship.engine.thrust, m_ship.engine.power );
            m_audio.Play( eSound::homingMissilesOff, 0.9 );
        }
    }

    // decoy beacon lifetime, dragged by the attractor fields:
    if( m_decoyTicks > 0 ) {
        m_decoyPosition += _AttractorPull( m_decoyPosition, attractorPullObjectMass ) * attractorPullDriftFactor;
        if( _TouchesAttractor( m_decoyPosition, decoyRadius ) )
            m_decoyHp = 0;
        m_decoyTicks--;
        if( m_decoyTicks == 0 || m_decoyHp <= 0 ) {
            m_decoyTicks = 0;
            m_audio.PlayAt( eSound::missileExplosion, _RelativeToShip( m_decoyPosition ) );
            _AddExplosion( m_decoyPosition, {}, eExplosion::medium, eFadeColor::green );
        }
    }
}


void World::_UpdateTurret()
{
    if( m_turretTicks <= 0 )
        return;
    m_turretTicks--;
    if( m_turretTicks == 0 ) {
        // goes out with a bang - a small friendly farewell blast:
        m_audio.Play( eSound::turretOff, 0.9 );
        _Detonate( m_turretPosition, {}, eExplosion::medium, eFadeColor::azure, turretExpireBlastRadius, turretExpireBlastDamage, turretExpireBlastImpulse, true );
        return;
    }

    // orbit around the ship:
    m_turretOrbitAngle += turretOrbitSpeed;
    m_turretPosition = m_ship.position + Vector::From( m_turretOrbitAngle, turretOrbitRadius );

    // acquire the closest threat - enemy, gravity mine or incoming missile:
    double bestDistanceSquared{ turretRange * turretRange };
    Vector targetPosition, targetMomentum;
    bool acquired{ false };
    const auto consider{ [ & ]( const Vector & _position, const Vector & _momentum ){
            const auto distanceSquared{ ( _position - m_turretPosition ).DistanceSquared() };
            if( distanceSquared >= bestDistanceSquared )
                return;
            bestDistanceSquared = distanceSquared;
            targetPosition = _position;
            targetMomentum = _momentum;
            acquired = true;
        } };
    for( const auto & enemy : m_enemies )
        if( !enemy.dead )
            consider( enemy.rocket.position, enemy.rocket.momentum );
    for( const auto & gravityMine : m_gravityMines )
        if( gravityMine.alive )
            consider( gravityMine.position, {} );
    for( const auto & missile : m_missiles )
        if( !missile.dead && !missile.fromShip )
            consider( missile.rocket.position, missile.rocket.momentum );
    if( !acquired ) {
        m_turretOrientation = m_turretOrbitAngle + Maths::PiHalf; // idle, aligned with the orbit
        return;
    }

    // aim with lead compensation and fire at high rate:
    constexpr double laserSpeed{ 50 }; // laser momentum per tick
    const auto lead{ targetMomentum * ( std::sqrt( bestDistanceSquared ) / laserSpeed ) };
    m_turretOrientation = ( targetPosition + lead - m_turretPosition ).Orientation();
    if( m_turretCadence++ % turretFirePeriod == 0 ) {
        m_audio.Play( eSound::laserShot, 0.35 );
        auto & laser{ m_lasers.emplace_back( Laser{ m_turretPosition, Vector::From( m_turretOrientation, laserSpeed ),
            0.2 } ) }; // same damage as one main-gun laser
        laser.Refresh();
    }
}


void World::_UpdateBlossom()
{
    if( m_blossomTicks <= 0 )
        return;
    m_blossomTicks--;
    // twin-arm spiral: one beam per arm every tick, the phase sweeping around, so
    // consecutive beams draw a visibly rotating spiral instead of a static starburst:
    m_blossomPhase += blossomSpin;
    if( m_blossomTicks % 4 == 0 )
        m_audio.Play( eSound::laserShot, 0.4 );
    for( int arm{ 0 }; arm < blossomArms; arm++ ) {
        const auto angle{ m_blossomPhase + Maths::Pi2 * arm / blossomArms };
        auto & laser{ m_lasers.emplace_back( Laser{ m_ship.position + Vector::From( angle, m_ship.dynamic.boundingBoxRadius + 6 ),
            Vector::From( angle, 50 ), 0.2 } ) };
        laser.Refresh();
    }
}


Vector World::_SingularityPull( const Vector & _position ) const
{
    const auto direction{ m_singularityPosition - _position };
    const auto distanceSquared{ direction.DistanceSquared() };
    if( m_singularityTicks <= 0 || distanceSquared >= singularityPullRange * singularityPullRange || distanceSquared < 1 )
        return {};
    const auto distance{ std::sqrt( distanceSquared ) };
    const auto falloff{ 1.0 - distanceSquared / ( singularityPullRange * singularityPullRange ) };
    const auto inward{ direction * ( 1.0 / distance ) };
    const Vector tangent{ -inward.v, inward.u }; // swirl: everything spirals, not just falls
    return ( inward + tangent * 0.45 ) * ( singularityPullStrength * falloff );
}


void World::_UpdateSingularity()
{
    if( m_debugSingularity && m_singularityTicks <= 0 && ++m_debugSingularityWait >= m_frameRate ) { // TEMPORARY (--singularity)
        m_debugSingularityWait = 0;
        m_singularityPosition = m_ship.position + Vector{ 320, -140 };
        m_singularityTotalTicks = m_frameRate * singularityDurationSeconds;
        m_singularityTicks = m_singularityTotalTicks;
        m_audio.Play( eSound::singularity );
    }
    if( m_singularityTicks <= 0 )
        return;
    m_singularityTicks--;

    // collapse:
    if( m_singularityTicks == 0 ) {
        m_audio.Play( eSound::attractorExplosion );
        _Detonate( m_singularityPosition, {}, eExplosion::big, eFadeColor::violet, singularityBlastRadius, singularityBlastDamage, singularityBlastImpulse, true );
        for( int i{ 0 }; i < 8; i++ ) { // extra debris ring
            const auto explosionPosition{ m_singularityPosition + Vector::From( Maths::Pi2 * i / 8, 60 ) };
            _AddExplosion( explosionPosition, ( explosionPosition - m_singularityPosition ) * 0.05, eExplosion::medium, eFadeColor::violet );
        }
        return;
    }

    // whatever reaches the core is SWALLOWED, not detonated - no shockwave escapes a
    // black hole, and detonating there would chain-feed an explosion factory:
    const auto inCore{ [ this ]( const Vector & _position ){
            return ( _position - m_singularityPosition ).DistanceSquared() < singularityCoreRadius * singularityCoreRadius;
        } };
    const auto swallowFlash{ [ this ]( const Vector & _position ){
            _AddExplosion( _position, {}, eExplosion::small, eFadeColor::violet );
        } };

    // drag the free objects in (rockets go through the attraction pathway):
    constexpr double objectDrift{ 10 };
    for( auto & mine : m_mines )
        mine.position += _SingularityPull( mine.position ) * objectDrift;
    std::erase_if( m_mines, [ & ]( const Mine & _mine ){
            if( !inCore( _mine.position ) )
                return false;
            swallowFlash( _mine.position );
            return true;
        } );
    for( auto & gravityMine : m_gravityMines )
        gravityMine.position += _SingularityPull( gravityMine.position ) * objectDrift;
    std::erase_if( m_gravityMines, [ & ]( const GravityMine & _gravityMine ){
            if( !inCore( _gravityMine.position ) )
                return false;
            swallowFlash( _gravityMine.position );
            return true;
        } );
    for( auto & goody : m_goodies )
        goody.position += _SingularityPull( goody.position ) * objectDrift;
    std::erase_if( m_goodies, [ & ]( const Goody & _goody ){ return inCore( _goody.position ); } ); // swallowed whole
    if( m_decoyTicks > 0 ) {
        m_decoyPosition += _SingularityPull( m_decoyPosition ) * objectDrift;
        if( inCore( m_decoyPosition ) )
            m_decoyHp = 0;
    }
    for( auto & particule : m_particules )
        particule.position += _SingularityPull( particule.position ) * 18;

    // missiles are swallowed too (silenced, no blast):
    for( auto & missile : m_missiles )
        if( !missile.dead && inCore( missile.rocket.position ) ) {
            missile.dead = true;
            m_audio.StopLoop( missile.sound_run );
            swallowFlash( missile.rocket.position );
        }

    // the core shreds the rockets that linger in it:
    constexpr double shredDamage{ 0.08 };
    for( auto & enemy : m_enemies )
        if( inCore( enemy.rocket.position ) )
            enemy.rocket.shield.value -= shredDamage;
    if( !m_shipDestroyed && inCore( m_ship.position ) )
        m_ship.shield.value -= shredDamage;

    // accretion disk sparkle:
    for( int i{ 0 }; i < 5; i++ ) {
        const auto angle{ Maths::Random( 0, Maths::Pi2 ) };
        const auto radius{ Maths::Random( 40, 180 ) };
        _AddParticule( m_singularityPosition + Vector::From( angle, radius ),
            Vector::From( angle + Maths::PiHalf, 2.2 ), angle + Maths::Pi, 0.6, Maths::Random( 0.5, 1.5 ), eFadeColor::violet );
    }
}


void World::_UpdateLasers()
{
    std::erase_if( m_lasers, []( Laser & _laser ){ return _laser.lifeSpan++ >= Laser::maxLifeSpan; } );
    for( auto & laser : m_lasers )
        laser.Update();
}


void World::_UpdateMissiles()
{
    for( auto & missile : m_missiles ) {
        // running:
        if( missile.lifeSpan == 0 ) {
            const auto soundPosition{ m_audio.Locate( _RelativeToShip( missile.rocket.position ) ) };

            const auto thrustRatio{ std::min( missile.rocket.engine.thrust / missile.rocket.engine.power, 1.0 ) };
            const auto thrustVolume{ thrustRatio * 0.4 }; // [ 0.0, 0.4 ]
            const auto thrustPitch{ std::max( thrustRatio, 0.1 ) }; // [ 0.1, 1.0 ]
            m_audio.SetLoop( missile.sound_run, { thrustVolume * soundPosition.volume, soundPosition.pan, thrustPitch } );

            // acquiring proper target (a live decoy fools the ship-seeking missiles):
            if( missile.targetShip && m_decoyTicks > 0 ) {
                missile.rocket.PointTo( m_decoyPosition, 0.1 );
                missile.rocket.ActivateThrust();
            }
            else {
                const auto pTarget{ missile.targetShip ? &m_ship : _ClosestEnemy( missile.rocket.position ) };
                if( pTarget != nullptr ) {
                    missile.rocket.Acquire( *pTarget, 0.1 );
                    missile.rocket.ActivateThrust();
                }
            }
            // stopping when out of propellant:
            if( missile.rocket.propellant.value <= missile.rocket.propellant.production_rate ) {
                missile.lifeSpan = 1;
                m_audio.StopLoop( missile.sound_run );
            }
        }
        // stopped:
        else {
            missile.lifeSpan++;
            // explode and remove after one second of drift:
            if( missile.lifeSpan == m_frameRate ) {
                _ExplodeMissile( missile );
                continue;
            }
        }
        // regular update:
        missile.rocket.Update();
    }
    std::erase_if( m_missiles, []( const Missile & _missile ){ return _missile.dead; } );
}


void World::_UpdateSlugs()
{
    for( auto & slug : m_slugs ) {
        const auto tail{ slug.position };
        slug.position += slug.momentum;
        if( ++slug.lifeSpan >= Slug::maxLifeSpan ) {
            slug.dead = true;
            continue;
        }
        // the plasma shield blocks slugs:
        if( !m_shipDestroyed && m_plasmaShield > 0 && Maths::Collision( m_ship.position, m_plasmaShieldRadius, tail, slug.position ) ) {
            _AddExplosion( slug.position, {}, eExplosion::small, eFadeColor::azure );
            slug.dead = true;
            continue;
        }
        // ship hit:
        if( !m_shipDestroyed && Maths::Collision( m_ship.position, m_ship.dynamic.boundingBoxRadius, tail, slug.position ) ) {
            m_audio.Play( eSound::laserCollision );
            m_ship.shield.value -= slugDamage;
            m_ship.ReceiveImpact( slug.position, slug.momentum * 0.05, 0.03 );
            _AddExplosion( slug.position, slug.momentum * 0.02, eExplosion::small );
            slug.dead = true;
            continue;
        }
        // stray slugs trigger gravity mines (friendly fire is fair game):
        for( auto & gravityMine : m_gravityMines )
            if( gravityMine.alive && Maths::Collision( gravityMine.position, gravityMine.dynamic.radius, tail, slug.position ) ) {
                gravityMine.alive = false;
                slug.dead = true;
                break;
            }
        if( slug.dead )
            continue;
        // absorbed by the attractors:
        m_attractors.ForEachInRange( slug.position, slugSpeed,
            [ & ]( const int, const Attractor & _attractor ){
                if( Maths::Collision( _attractor.position, _attractor.radius, tail, slug.position ) )
                    slug.dead = true;
            } );
    }
    std::erase_if( m_slugs, []( const Slug & _slug ){ return _slug.dead; } );
}


void World::_UpdateShip()
{
    if( m_shipDestroyed ) {
        // drifting wreck: the camera glides to a stop while the debris burns out
        m_ship.momentum *= 0.97;
        if( m_shipDestroyedTicks < m_frameRate && m_shipDestroyedTicks % 6 == 0 ) {
            const auto debrisPosition{ m_ship.position + Vector::From( Maths::Random( 0, Maths::Pi2 ), Maths::Random( 0, m_ship.dynamic.boundingBoxRadius * 1.5 ) ) };
            _AddExplosion( debrisPosition, m_ship.momentum, eExplosion::medium, m_shipDestroyedTicks % 12 == 0 ? eFadeColor::orange : eFadeColor::azure );
        }
        m_shipDestroyedTicks++;
        return;
    }
    m_ship.Update();
}


void World::_UpdateWind()
{
    // the field keeps waving even once the ship is gone (the world lives on):
    m_wind.Update();
    if( m_shipDestroyed )
        return;
    m_ship.momentum += m_wind.At( m_ship.position );
}


void World::_UpdateAlerts()
{
    if( m_shipDestroyed )
        return;

    if( m_debugInvincible )
        m_ship.shield.value = m_ship.shield.capacity;

    // ship shield management: once the shield is fully depleted, the next hit is fatal
    if( m_ship.shield.value < 0 ) {
        _DestroyShip();
        return;
    }
    if( m_ship.shield.value < ( m_ship.shield.capacity * 0.25 ) ) {
        if( !m_shieldAlert ) {
            m_shieldAlert = true;
            m_audio.Play( eSound::lowShieldAlert, 0.9 );
        }
    }
    else
        m_shieldAlert = false;

    // ship propellant management:
    if( m_ship.propellant.value < ( m_ship.propellant.capacity * 0.25 ) ) {
        if( !m_fuelAlert ) {
            m_fuelAlert = true;
            m_audio.Play( eSound::lowFuelAlert, 0.9 );
        }
    }
    else
        m_fuelAlert = false;
}


void World::_DestroyShip()
{
    m_shipDestroyed = true;
    m_shipDestroyedTicks = 0;
    // a ship is lost (in free play the game is won already: nothing is at stake), and the
    // retry keeps the count:
    if( m_stage.Phase() != StageDirector::ePhase::freePlay ) {
        m_lives = std::max( m_lives - 1, 0 );
        m_checkpoint.lives = m_lives;
    }
    m_ship.shield.value = 0;
    m_plasmaShield = 0;
    m_audio.Play( eSound::shipExplosion );
    m_audio.StopLoop( m_sound_shipMainEngine );
    m_audio.StopLoop( m_sound_shipRotationEngine );

    // main blast plus two shockwave rings (same recipe as the attractor death):
    _Detonate( m_ship.position, m_ship.momentum, eExplosion::big, eFadeColor::orange, shipBlastRadius, shipBlastDamage, shipBlastImpulse );
    const auto radius{ m_ship.dynamic.boundingBoxRadius };
    for( int j{ 0 }; j < 2; j++ ) {
        const int divisions{ 8 };
        for( int i{ 0 }; i < divisions; i++ ) {
            const double angle{ Maths::Pi2 * i / divisions };
            const double currentRadius{ radius * ( 2.0 / static_cast< double >( j + 1 ) ) };
            const auto explosionPosition{ m_ship.position + Vector::From( angle, currentRadius ) };
            _AddExplosion( explosionPosition, ( explosionPosition - m_ship.position ) * 0.05, eExplosion::medium, eFadeColor::orange );
        }
    }
}


void World::_UpdateEngineSounds()
{
    if( m_shipDestroyed )
        return;

    const auto thrustRatio{ std::min( m_ship.engine.thrust / m_ship.engine.power, 1.0 ) };
    const auto thrustVolume{ thrustRatio * 0.2 }; // [ 0.0, 0.2 ]
    const auto thrustPitch{ std::max( thrustRatio, 0.5 ) }; // [ 0.5, 1.0 ]
    m_audio.SetLoop( m_sound_shipMainEngine, { thrustVolume, {}, thrustPitch } );

    const auto rotatorRatio{ std::min( ( m_ship.rotator.thrust.at( 0 ) + m_ship.rotator.thrust.at( 1 ) ) / ( 2 * m_ship.rotator.power ), 1.0 ) };
    const auto rotatorVolumeThreshold{ 0.25 };
    const auto rotatorVolume{ rotatorRatio < rotatorVolumeThreshold ? 0 : ( rotatorRatio - rotatorVolumeThreshold ) * 0.1 }; // [ 0.0, 0.1 ]
    const auto rotatorPitch{ std::max( rotatorRatio, 0.5 ) }; // [ 0.5, 1.0 ]
    m_audio.SetLoop( m_sound_shipRotationEngine, { rotatorVolume, {}, rotatorPitch } );
}


void World::UpdateAmbientSound()
{
    // always faintly audible ambience, swelling with the ship speed and the local
    // wind strength, panned toward where the wind blows:
    const auto wind{ m_wind.At( m_ship.position ) };
    const auto intensity{ std::min( ( m_ship.momentum.Distance() + wind.Distance() * 10 ) / 15, 1.0 ) };
    const auto spaceWindVolume{ 0.06 + intensity * 0.38 }; // [ 0.06, 0.44 ]
    const auto spaceWindPitch{ 0.3 + intensity * 0.7 }; // [ 0.3, 1.0 ]
    const auto spaceWindPan{ std::clamp( 0.5 + wind.u * 0.35, 0.15, 0.85 ) };
    m_audio.SetLoop( m_sound_spaceWind, { spaceWindVolume, spaceWindPan, spaceWindPitch } );
}


void World::_UpdateParticules()
{
    std::erase_if( m_particules, [ this ]( Particule & _particule ){
            _particule.lifeSpan -= 0.09; // quickly reduce particules lifespan
            if( _particule.lifeSpan <= 0 )
                return true;
            // particules make the wind waves visible, drifting with the local flow,
            // and spiral into the gravity mines' pull:
            _particule.position += _particule.momentum + m_wind.At( _particule.position ) * 1.5;
            for( const auto & gravityMine : m_gravityMines )
                _particule.position += gravityMine.Attraction( _particule.position ) * 25;
            return false;
        } );
}


World::HudInfo World::GetHudInfo() const
{
    const int laserSpeed{ ( m_laserSpeed == eLaserSpeed::slow ) ? 0 : ( ( m_laserSpeed == eLaserSpeed::medium ) ? 1 : 2 ) };
    const int laserPass{ ( m_laserPass == eLaserPass::one ) ? 0 : ( m_laserPass == eLaserPass::two ? 1 : ( m_laserPass == eLaserPass::four ? 2 : ( m_laserPass == eLaserPass::six ? 3 : 4 ) ) ) };
    const int maxLaserPower{ 14 }; // 4 * 3 + 2
    return {
        m_ship.shield.value, m_ship.shield.capacity,
        m_ship.propellant.value, m_ship.propellant.capacity,
        ( laserPass * 3 + laserSpeed ) * 100 / maxLaserPower,
        m_homingMissiles,
        m_magneticMines,
        static_cast< int >( std::ceil( static_cast< double >( m_plasmaShield ) / m_frameRate ) ),
        static_cast< int >( std::ceil( static_cast< double >( m_turretTicks ) / m_frameRate ) ),
        static_cast< int >( std::ceil( static_cast< double >( m_decoyTicks ) / m_frameRate ) ),
        static_cast< int >( std::ceil( static_cast< double >( m_empTicks ) / m_frameRate ) ),
        static_cast< int >( std::ceil( static_cast< double >( m_overdriveTicks ) / m_frameRate ) ),
        static_cast< int >( std::ceil( static_cast< double >( m_blossomTicks ) / m_frameRate ) ),
        static_cast< int >( std::ceil( static_cast< double >( m_singularityTicks ) / m_frameRate ) ),
        m_score,
    };
}
