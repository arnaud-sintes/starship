#include "World.h"

// *** TODO list:
// - engine:
//      - add solar wind (waves) -> how to maintain a grid (with resolution) dealing with this kind of stuff
//          and quickly interpolate a value for a given object position (any rocket/laser?/goodie?/particule?/...?)
//      - add planets & rocks & mines with gravity/attraction
//
// - functional:
//      - add stages, with different ennemies (more variety) and specific choregraphies
//      - add special stages (asteroid field, mine field, rescue broken ship etc.)
//      - end of stage big boss
//
//      - more special bonues (turel, plasma, external module ala r-type, allied ship etc.)
//      - non-guided missiles rotator goodie ?
//
// TODO explosions got a range -> explosion chaining / impact zone / affect momentum


namespace
{
    constexpr int enemyCount{ 2 };
    constexpr int attractorsCount{ 1000 };
    constexpr double attractorsRange{ 10000 };
    constexpr double attractorsSecurityDistance{ 200 };
}


World::World( AudioDirector & _audio, const Dimension_ui & _screenDimension, const int _frameRate )
    : m_audio{ _audio }
    , m_screenDimension{ _screenDimension }
    , m_screenCenter{ static_cast< double >( _screenDimension.width ) * 0.5, static_cast< double >( _screenDimension.height ) * 0.5 }
    , m_frameRate{ _frameRate }
    , m_ship{ { 0.5, 0.75, 1 }, {}, Maths::PiHalf, { 0, -5 }, {}, 0,
        5, // damage
        { 5, 5, 0.01, 0.5 }, // shield
        { 20, 20, 0.01, 0.75 }, // propellant
        { 0, 0.5, false, 0.005, 0.01, 0.75, 0 }, // engine
        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } } // rotators
    , m_plasmaShield{ _frameRate * 10 } // 10 seconds immunity at startup
{
    m_ship.id = m_nextRocketId++;
    m_ship.RefreshGeometry();
    m_ship.momentum -= m_solarWind;

    m_sound_spaceWind = m_audio.CreateLoop( eSound::spaceWind, { 0.0, {}, {} } );
    m_sound_shipMainEngine = m_audio.CreateLoop( eSound::shipMainEngine, { 0.0, {}, {} } );
    m_sound_shipRotationEngine = m_audio.CreateLoop( eSound::shipRotationEngine, { 0.0, {}, {} } );

    for( int i{ 0 }; i < enemyCount; i++ )
        _AddEnemy();

    m_attractors.Generate( attractorsCount, attractorsRange, attractorsSecurityDistance, 1, 2 );
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


void World::_AddEnemy()
{
    // enemy shield is correlated to current laser pass:
    const double shield{ static_cast< double >( m_laserPass ) };
    const auto minDistance{ Maths::Random( 0.5, 0.75 ) * static_cast< double >( std::max( m_screenDimension.width, m_screenDimension.height ) ) };
    auto & enemy{ m_enemies.emplace_back( Enemy{ Rocket{ { 1, 0.5, 0.75 }, m_ship.position + Vector::From( Maths::Random( 0, Maths::Pi2 ), minDistance ), Maths::Random( 0, Maths::Pi2 ), {}, {}, 0,
            5, // damage
            { shield, shield, 0.001, 0.2 }, // shield
            { 10, 10, 0.05, Maths::Random( 0.1, 0.75 ) }, // propellant
            { 0, Maths::Random( 0.1, 0.5 ), false, 0.005, 0.01, Maths::Random( 0.2, 0.75 ), 0 }, // engine
            { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } }, // rotators
            static_cast< int >( Maths::Random( 0, m_frameRate * 5 ) ),
            m_audio.CreateLoop( eSound::shipMainEngine, { 0.0, {}, {} } ),
            m_audio.CreateLoop( eSound::shipRotationEngine, { 0.0, {}, {} } )
        } ) };
    enemy.rocket.id = m_nextRocketId++;
    enemy.rocket.RefreshGeometry();
}


void World::_SpawnMissile( const Rocket & _launcher, const bool _targetShip )
{
    Vector motion{};
    if( !_targetShip && _ClosestEnemy( _launcher.position ) == nullptr )
        motion = Vector::From( _launcher.orientation, -5 );
    auto & missile{ m_missiles.emplace_back( Missile{ Rocket{ _targetShip ? Color_d{ 1, 0.5, 0.75 } : Color_d{ 0.5, 0.75, 1 }, _launcher.position, _launcher.orientation, motion, _launcher.momentum, 0,
        3, // damage
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
    if( !m_shipDestroyed )
        _HandleControls( _input );

    // simulation, same ordering as the historical game loop:
    _UpdateAttractions();
    _UpdatePlasmaShield();
    _UpdateEnemyCollisions();
    _UpdateLaserCollisions();
    _UpdateMissileCollisions();
    _UpdateShipAttractorCollisions();
    _UpdateEnemies();
    _UpdateMines();
    _UpdateAttractorsDeletion();
    _UpdateGoodies();
    _UpdateLasers();
    _UpdateMissiles();
    _UpdateShip();
    _UpdateSolarWind();
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
                0.2 } ) }; // damage
            laser.Refresh();
            m_laserAlternate++;
        }
    }

    // shoot missile:
    if( m_homingMissiles > 0 && m_missileCadence++ > 25 && _input.fire ) {
        m_homingMissiles--;
        if( m_homingMissiles == 0 )
            m_audio.Play( eSound::homingMissilesOff, 0.75 );
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
                m_audio.Play( eSound::magneticMinesOff, 0.75 );
        }
    }
}


void World::_AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size, const eFadeColor _color )
{
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
    _AddScore( 1 );

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
            m_audio.Play( eSound::laserPowerUp, 0.75 );
        return;
    }
    if( _type == Goody::eType::homingMissiles ) {
        m_homingMissiles += 30; // 30x missiles pack
        m_audio.Play( eSound::homingMissiles, 0.75 );
        return;
    }
    if( _type == Goody::eType::magneticMines ) {
        m_magneticMines += 10; // 10x mines pack
        m_audio.Play( eSound::magneticMines, 0.75 );
        return;
    }
    if( _type == Goody::eType::plasmaShield ) {
        m_plasmaShield += m_frameRate * 5; // 5 seconds plasma shield
        m_audio.Play( eSound::plasmaShield, 0.75 );
        return;
    }
    if( _type == Goody::eType::shieldAdd ) {
        if( m_ship.shield.value >= m_ship.shield.capacity )
            return;
        m_ship.shield.value = std::min( m_ship.shield.value + m_ship.shield.capacity * 0.5, m_ship.shield.capacity ); // 50% capacity boost
        m_audio.Play( eSound::shieldRepair, 0.75 );
        return;
    }
    if( _type == Goody::eType::propellantAdd ) {
        if( m_ship.propellant.value >= m_ship.propellant.capacity )
            return;
        m_ship.propellant.value = std::min( m_ship.propellant.value + m_ship.propellant.capacity * 0.5, m_ship.propellant.capacity ); // 50% capacity boost
        m_audio.Play( eSound::propellantRefuel, 0.75 );
        return;
    }
}


void World::_MaybeDropGoody( const Vector & _position )
{
    // 50% chance of goody addition, only if far enough (avoid volontary collision bonuses...)
    if( ( _position - m_ship.position ).Distance() <= 100 || Maths::Random( 0, 1 ) >= 0.5 )
        return;
    std::array< Goody::eType, 6 > types{ Goody::eType::homingMissiles, Goody::eType::magneticMines, Goody::eType::plasmaShield };
    size_t typeCount{ 3 };
    if( m_ship.shield.value < m_ship.shield.capacity ) types.at( typeCount++ ) = Goody::eType::shieldAdd;
    if( m_ship.propellant.value < m_ship.propellant.capacity ) types.at( typeCount++ ) = Goody::eType::propellantAdd;
    if( m_laserSpeed != eLaserSpeed::fast || m_laserPass != eLaserPass::height ) types.at( typeCount++ ) = Goody::eType::laserUp;
    const auto typeRandom{ Maths::Random( 0, static_cast< double >( typeCount ) - 0.01 ) };
    m_goodies.emplace_back( Goody{ _position, types.at( static_cast< size_t >( typeRandom ) ) } );
}


double World::_AttractionQueryRange( const Rocket & _rocket ) const
{
    return AttractorField::distanceThreshold * m_attractors.MaxMass() * _rocket.dynamic.totalMass;
}


void World::_UpdateAttractions()
{
    m_attractedRockets.clear();
    if( !m_shipDestroyed )
        m_attractedRockets.emplace_back( &m_ship );
    for( auto & enemy : m_enemies )
        m_attractedRockets.emplace_back( &enemy.rocket );
    for( auto & missile : m_missiles )
        m_attractedRockets.emplace_back( &missile.rocket );
    for( auto * pRocket : m_attractedRockets ) {
        Vector attraction;
        m_attractors.ForEachInRange( pRocket->position, _AttractionQueryRange( *pRocket ),
            [ & ]( const int, const Attractor & _attractor ){
                attraction += _attractor.position.ProximityAttraction( pRocket->position, _attractor.mass * pRocket->dynamic.totalMass, AttractorField::distanceThreshold );
            } );
        pRocket->dynamic.attraction = attraction;
    }
}


void World::_UpdatePlasmaShield()
{
    if( m_plasmaShield > 0 ) {
        m_plasmaShield--;
        if( m_plasmaShield == 0 )
            m_audio.Play( eSound::plasmaShieldOff, 0.75 );
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
        // enemy-plasma shield collision:
        if( m_plasmaShield > 0 && Maths::Collision( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius, m_ship.position, m_plasmaShieldRadius ) ) {
            m_audio.PlayAt( eSound::shipCollision, _RelativeToShip( enemy.rocket.position ) );
            _RocketImpact( m_ship, enemy.rocket );
            _AddScore( 1 );
        }
        // enemy-mines collision:
        for( auto & mine : m_mines ) {
            if( mine.alive && Maths::Collision( enemy.rocket.position, enemy.rocket.dynamic.boundingBoxRadius, mine.position, mine.dynamic.radius ) ) {
                mine.alive = false;
                enemy.rocket.shield.value -= mine.damage;
                _AddScore( 5 );
            }
        }
        // enemy-attractors collision:
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
            if( _LaserRocketCollision( laser, enemy.rocket ) ) {
                pCollision = &enemy.rocket;
                m_audio.PlayAt( eSound::laserCollision, _RelativeToShip( pCollision->position ) );
                _AddScore( 5 );
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
                _AddExplosion( laser.position, missile.rocket.momentum );
                missile.dead = true;
                m_audio.StopLoop( missile.sound_run );
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
                _AddExplosion( missileOther.rocket.position, missileOther.rocket.momentum );
                missileOther.dead = true;
                m_audio.StopLoop( missileOther.sound_run );
                _AddScore( 1 );
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
        // explode and remove:
        if( collision ) {
            m_audio.PlayAt( eSound::missileExplosion, _RelativeToShip( rocket.position ) );
            _AddExplosion( rocket.position, rocket.momentum );
            missile.dead = true;
            m_audio.StopLoop( missile.sound_run );
            _AddScore( 1 );
        }
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
            m_ship.shield.value -= _attractor.mass;
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
    int newEnemiesToGenerate{ 0 };
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

        // better NOT aim directly the ship to avoid collisions:
        enemy.rocket.Acquire( m_ship, 0.5, Vector::From( m_ship.orientation + Maths::Pi, 100 ) );
        enemy.rocket.ActivateThrust();
        enemy.rocket.Update();

        // shield:
        if( enemy.rocket.shield.value <= 0 ) {
            _MaybeDropGoody( enemy.rocket.position );
            m_audio.PlayAt( eSound::shipExplosion, _RelativeToShip( enemy.rocket.position ) );
            _AddExplosion( enemy.rocket.position, enemy.rocket.momentum, eExplosion::big );
            newEnemiesToGenerate++;
            enemy.dead = true;
            m_audio.StopLoop( enemy.sound_mainEngine );
            m_audio.StopLoop( enemy.sound_rotationEngine );
            _AddScore( 50 );
            continue;
        }

        // enemies launch rockets aiming the ship:
        if( enemy.shotRate++ > m_frameRate * 5 ) { // every 5 seconds
            enemy.shotRate = 0;
            _SpawnMissile( enemy.rocket, true );
            m_audio.PlayAt( eSound::missileShot, _RelativeToShip( enemy.rocket.position ) );
        }
    }
    std::erase_if( m_enemies, []( const Enemy & _enemy ){ return _enemy.dead; } );
    for( int i{ 0 }; i < newEnemiesToGenerate; i++ )
        _AddEnemy();
}


void World::_UpdateMines()
{
    for( auto & mine : m_mines )
        if( !mine.alive ) {
            m_audio.PlayAt( eSound::mineExplosion, _RelativeToShip( mine.position ) );
            _AddExplosion( mine.position, {}, eExplosion::big, eFadeColor::orange );
        }
    std::erase_if( m_mines, []( const Mine & _mine ){ return !_mine.alive; } );

    // grow animation (drives the collision radius):
    for( auto & mine : m_mines )
        mine.Update();

    // mines attraction:
    for( auto & mine : m_mines ) {
        const Rocket * pTarget{ _ClosestEnemy( mine.position ) };
        if( pTarget != nullptr )
            mine.position += pTarget->position.InfiniteAttraction( mine.position, pTarget->dynamic.totalMass );
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
        m_audio.PlayAt( eSound::attractorExplosion, _RelativeToShip( attractor.position ) );
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

    // grow animation and attraction toward the ship:
    for( auto & goody : m_goodies ) {
        goody.Update();
        goody.position += m_ship.position.InfiniteAttraction( goody.position, m_ship.dynamic.totalMass );
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

            // acquiring proper target:
            const auto pTarget{ missile.targetShip ? &m_ship : _ClosestEnemy( missile.rocket.position ) };
            if( pTarget != nullptr ) {
                missile.rocket.Acquire( *pTarget, 0.1 );
                missile.rocket.ActivateThrust();
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
                m_audio.PlayAt( eSound::missileExplosion, _RelativeToShip( missile.rocket.position ) );
                _AddExplosion( missile.rocket.position, missile.rocket.momentum );
                missile.dead = true;
                continue;
            }
        }
        // regular update:
        missile.rocket.Update();
    }
    std::erase_if( m_missiles, []( const Missile & _missile ){ return _missile.dead; } );
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


void World::_UpdateSolarWind()
{
    if( m_shipDestroyed )
        return;
    if( m_solarWindIndex == 0 ) {
        m_solarWindCurrent = m_solarWind;
        m_solarWindTarget = Vector::From( Maths::Random( 0, Maths::Pi2 ), Maths::Random( 0.1, 0.3 ) );
        m_solarWindCount = m_frameRate * static_cast< int >( Maths::Random( 3, 10 ) ); // between 3 and 10 secs transitions
    }
    const double solarWindRate{ static_cast< double >( m_solarWindIndex++ ) / static_cast< double >( m_solarWindCount ) };
    if( m_solarWindIndex == m_solarWindCount )
        m_solarWindIndex = 0;
    m_solarWind = m_solarWindCurrent * ( 1.0 - solarWindRate ) + ( m_solarWindTarget * solarWindRate );
    m_ship.momentum += m_solarWind;
}


void World::_UpdateAlerts()
{
    if( m_shipDestroyed )
        return;

    // ship shield management: once the shield is fully depleted, the next hit is fatal
    if( m_ship.shield.value < 0 ) {
        _DestroyShip();
        return;
    }
    if( m_ship.shield.value < ( m_ship.shield.capacity * 0.25 ) ) {
        if( !m_shieldAlert ) {
            m_shieldAlert = true;
            m_audio.Play( eSound::lowShieldAlert, 0.75 );
        }
    }
    else
        m_shieldAlert = false;

    // ship propellant management:
    if( m_ship.propellant.value < ( m_ship.propellant.capacity * 0.25 ) ) {
        if( !m_fuelAlert ) {
            m_fuelAlert = true;
            m_audio.Play( eSound::lowFuelAlert, 0.75 );
        }
    }
    else
        m_fuelAlert = false;
}


void World::_DestroyShip()
{
    m_shipDestroyed = true;
    m_shipDestroyedTicks = 0;
    m_ship.shield.value = 0;
    m_plasmaShield = 0;
    m_audio.Play( eSound::shipExplosion );
    m_audio.StopLoop( m_sound_shipMainEngine );
    m_audio.StopLoop( m_sound_shipRotationEngine );

    // main blast plus two shockwave rings (same recipe as the attractor death):
    _AddExplosion( m_ship.position, m_ship.momentum, eExplosion::big );
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
    // always faintly audible ambience, swelling with the ship speed:
    const auto spaceWindRatio{ std::min( m_ship.momentum.Distance() / 12, 1.0 ) };
    const auto spaceWindVolume{ 0.06 + spaceWindRatio * 0.38 }; // [ 0.06, 0.44 ]
    const auto spaceWindPitch{ 0.3 + spaceWindRatio * 0.7 }; // [ 0.3, 1.0 ]
    m_audio.SetLoop( m_sound_spaceWind, { spaceWindVolume, {}, spaceWindPitch } );
}


void World::_UpdateParticules()
{
    std::erase_if( m_particules, []( Particule & _particule ){
            _particule.lifeSpan -= 0.09; // quickly reduce particules lifespan
            if( _particule.lifeSpan <= 0 )
                return true;
            _particule.position += _particule.momentum;
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
        m_score,
    };
}
