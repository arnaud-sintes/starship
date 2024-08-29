#include "Renderer.h"


Renderer::Renderer( const Win32::Windows & _windows )
    : m_windows{ _windows }
    , m_starField{ m_windows.GetDimension() }
    , m_ship{ { 0.5, 0.75, 1 },  {}, Maths::PiHalf, {}, {}, 0,
        5, // damage
        { 20, 20, 0.01, 0.9 }, // shield
        { 20, 20, 0.01, 0.75 }, // propellant
        { 0, 0.5, false, 0.005, 0.01, 0.75, 0 }, // engine
        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } } // rotators
    , m_sounds{
        { eSound::spaceWind, m_audioEngine.LoadSound( "./spaceWind.wav" ) },
        { eSound::laserShot, m_audioEngine.LoadSound( "./laserShot.wav" ) },
        { eSound::laserCollision, m_audioEngine.LoadSound( "./laserCollision.wav" ) },
        { eSound::missileShot, m_audioEngine.LoadSound( "./missileShot.wav" ) },
        { eSound::missileRun, m_audioEngine.LoadSound( "./missileRun.wav" ) },
        { eSound::missileExplosion, m_audioEngine.LoadSound( "./missileExplosion.wav" ) },
        { eSound::shipCollision, m_audioEngine.LoadSound( "./shipCollision.wav" ) },
        { eSound::shipExplosion, m_audioEngine.LoadSound( "./shipExplosion.wav" ) },
        { eSound::shipRotationEngine, m_audioEngine.LoadSound( "./shipRotationEngine.wav" ) },
    }
{
    // [ ][ ][ ][ ] proximity alert         [    ]
    // [ ][ ][ ][ ] low-fuel                [    ]
    // [ ][ ][ ][ ] low-shield              [    ]
    // [ ][ ][ ][P] space wind              [DONE]
    // [ ][ ][ ][ ] laser shot              [DONE]
    // [S][V][ ][ ] laser collision         [DONE]
    // [S][V][ ][ ] missile shot            [DONE]
    // [S][V][L][P] missile run             [DONE]
    // [S][V][ ][ ] missile explosion       [DONE]
    // [S][V][ ][ ] ship collision          [DONE]
    // [S][V][ ][ ] ship explosion          [DONE]
    // [S][V][L][P] ship rotation engine    [    ]
    // [S][V][L][P] ship main engine        [    ]

    // TODO embedded resource management

    for( int i{ 0 }; i < 3; i++ )
        _AddEnemy();

    _SetupSound( eSound::spaceWind, m_ship, 0.2, true ).Play();
}


void Renderer::_AddEnemy()
{
    const auto minDistance{ Maths::Random( 0.25, 0.75 ) * static_cast< double >( std::max( m_windows.GetDimension().width, m_windows.GetDimension().height ) ) };
    m_enemies.emplace_back( new Enemy{ Rocket{ { 1, 0.5, 0.75 }, m_ship.position + Vector::From( Maths::Random( 0, Maths::Pi2 ), minDistance ), Maths::Random( 0, Maths::Pi2 ), {}, {}, 0,
            5, // damage
            { 5, 5, 0.001, 0.2 }, // shield
            { 10, 10, 0.05, Maths::Random( 0.1, 0.75 ) }, // propellant
            { 0, Maths::Random( 0.1, 0.5 ), false, 0.005, 0.01, Maths::Random( 0.2, 0.75 ), 0 }, // engine
            { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } }, // rotators
            static_cast< int >( Maths::Random( 0, 60 * 5 ) ),
            { m_sounds.find( eSound::laserCollision )->second, nullptr },
            { m_sounds.find( eSound::shipCollision )->second, nullptr },
            { m_sounds.find( eSound::shipExplosion )->second, nullptr }
        } );
}


void Renderer::Loop( const NanoVGRenderer::Frame & _frame )
{
    // reset:
    _Reset();

    // key mapping:
    _Keys();

    // update:
    _Update();

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
    // stabilize rotation:
    if( m_windows.KeyPressed( Win32::Windows::eKey::down ) )
        m_ship.StabilizeRotation();
    else
    // invert momentum:
    if( m_windows.KeyPressed( Win32::Windows::eKey::lControl ) || m_windows.KeyPressed( Win32::Windows::eKey::rControl ) )
        m_ship.InvertMomentum( 0.2 );
    else
    // change ship orientation:
    if( m_windows.KeyPressed( Win32::Windows::eKey::right ) )
        m_ship.Rotate( Rocket::Rotator::right );
    else
    if( m_windows.KeyPressed( Win32::Windows::eKey::left ) )
        m_ship.Rotate( Rocket::Rotator::left );

    // activate ship burst:
    if( m_windows.KeyPressed( Win32::Windows::eKey::up ) )
        m_ship.ActivateThrust();

    // acquire closest target:
    m_pTarget = nullptr;
    if( m_windows.KeyPressed( Win32::Windows::eKey::lShift ) || m_windows.KeyPressed( Win32::Windows::eKey::rShift ) ) {
        m_pTarget = _ClosestEnemy( m_ship.position );
        if( m_pTarget != nullptr )
            m_ship.Acquire( *m_pTarget, 0.5 );
    }

    // laser:
    static bool alternateLazerPosition{ true };
    static int laserShotRate{ 0 };
    if( laserShotRate++ > 5 && m_windows.KeyPressed( Win32::Windows::eKey::space ) )
    {
        m_sounds.find( eSound::laserShot )->second.Play();
        alternateLazerPosition = !alternateLazerPosition;
        const auto momentum{ Vector::From( m_ship.orientation + Maths::Pi, 50 ) }; // 50px length
        const auto position{ Vector::From( m_ship.orientation + Maths::PiHalf * ( alternateLazerPosition ? 1 : -1 ), m_ship.dynamic.boundingBoxRadius ) };
        m_lasers.emplace_back( new Laser{ m_ship.position - momentum + m_ship.dynamic.headPosition + position, momentum,
            0.2 } ); // damage
        laserShotRate = 0;
    }
        
    // TODO non-moving celestial object (mines)

    // shoot missile:
    static int missileShotRate{ 0 };
    if( missileShotRate++ > 35 && m_windows.KeyPressed( Win32::Windows::eKey::space ) ) {
        missileShotRate = 0;
        const auto & missile{ m_missiles.emplace_back( new Missile{ Rocket{ { 0.5, 0.75, 1 }, m_ship.position, m_ship.orientation, {}, m_ship.momentum, 0,
            3, // damage
            { 1, 1, 0.01, 0.5 }, // shield
            { 10, 10, 0.005, 0.9 }, // propellant
            { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
            //{ { 0, 0 }, 0, { false, false }, 0, 0, 0, { 0, 0 } } } ); // TODO non-guided missiles rotator?
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


MiniAudio::Sound & Renderer::_SetupSound( MiniAudio::Sound & _sound, const Rocket & _rocket, const double _pitch, const bool _loop )
{
    const auto relativePosition{ _rocket.position - m_ship.position };
    const double maxDistance{ 2000 };
    const auto volume{ 1.0 - std::min( relativePosition.Distance() / maxDistance, 0.9 ) };
    const auto pan{ relativePosition.u / 500 };
    _sound.Setup( volume, std::clamp( pan, -1.0, 1.0 ), _pitch, _loop );
    return _sound;
}


MiniAudio::Sound & Renderer::_SetupSound( eSound _sound, const Rocket & _rocket, const double _pitch, const bool _loop )
{
    return _SetupSound( m_sounds.find( _sound )->second, _rocket, _pitch, _loop );
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


void Renderer::_Update()
{
    // TODO use mouse to move?
    // TODO move code logic to proper classes
        
    // TODO solar wind (waves)
    // TODO planets and gravity attraction

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
    }

    // laser collision:
    for( auto & laser : m_lasers ) {
        Rocket * pCollision{ nullptr };
        // laser-ships collisions:
        for( auto itEnemy{ m_enemies.begin() }, itEnemyEnd{ m_enemies.end() }; pCollision == nullptr && itEnemy != itEnemyEnd; ++itEnemy ) {
            if( _LaserRocketCollision( *laser, ( **itEnemy ).rocket ) ) {
                pCollision = &( **itEnemy ).rocket;
                _SetupSound( ( **itEnemy ).sound_laserCollision, *pCollision ).Play();
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
                    continue;
                }
            ++itMissile;
        }
        if( pCollision != nullptr )
            laser->lifeSpan = laser->maxLifeSpan;
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
                    continue;
                }
            ++itMissile;
        }
        // missiles-enemies collisions:
        for( auto itEnemy{ m_enemies.begin() }, itEnemyEnd{ m_enemies.end() }; !collision && itEnemy != itEnemyEnd; ++itEnemy )
            collision = _MissileRocketCollision( **it, ( **itEnemy ).rocket );
        // missiles-ship collision:
        if( !collision )
            collision = _MissileRocketCollision( **it, m_ship );
        // explode and remove:
        if( collision ) {
            _SetupSound( ( **it ).sound_run, rocket ).Stop();
            _QueueSoundPlay( _SetupSound( ( **it ).sound_explosion, rocket ) );
            _AddExplosion( rocket.position, rocket.momentum );
            it = m_missiles.erase( it );
            continue;
        }
        ++it;
    }        
        
    // update enemies data:
    int newEnemiesToGenerate{ 0 };
    for( auto it{ m_enemies.begin() }; it != m_enemies.end(); ) {
        auto & enemy{ **it };
        // better NOT aim directly the ship to avoid collisions:
        enemy.rocket.Acquire( m_ship, 0.5, Vector::From( m_ship.orientation + Maths::Pi, 100 ) );
        enemy.rocket.ActivateThrust();
        enemy.rocket.Update();

        // shield:
        if( enemy.rocket.shield.value <= 0 ) {
            _QueueSoundPlay( _SetupSound( enemy.sound_explosion, enemy.rocket ) );
            _AddExplosion( enemy.rocket.position, enemy.rocket.momentum, bigExplosion );
            newEnemiesToGenerate++;
            it = m_enemies.erase( it );
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
            _SetupSound( missile->sound_run, m_ship, 0.0, true ).Play();
        }
        ++it;
    }
    for( int i{ 0 }; i < newEnemiesToGenerate; i++ )
        _AddEnemy();

    // update laser data:
    for( auto it{ m_lasers.begin() }; it != m_lasers.end(); ) {
        auto & laser{ **it };
        if( laser.lifeSpan++ > laser.maxLifeSpan ) {
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
            // acquiring proper target:
            const auto pTarget{ missile.targetShip ? &m_ship : _ClosestEnemy( missile.rocket.position ) };
            if( pTarget != nullptr ) {
                _SetupSound( missile.sound_run, missile.rocket, missile.rocket.engine.thrust / missile.rocket.engine.power, true );
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

    // TODO deal with shield / potential explosion/removal (game-over)

    // update ship data:
    m_ship.Update();

    // TODO
    static bool rotatorSound{ false };
    if( m_ship.rotator.thrust.at( 0 ) == 0 ) {
        if( rotatorSound ) {
            m_sounds.find( eSound::shipRotationEngine )->second.Stop();
            rotatorSound = false;
        }
    }
    else {
        auto & sound{ _SetupSound( eSound::shipRotationEngine, m_ship, m_ship.rotator.thrust.at( 0 ) / m_ship.rotator.power, true ) };
        if( !rotatorSound )
            sound.Play();
        rotatorSound = true;
    }

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
    // TODO information display (what?)
    // - current ship information (shield, propeller & engines classes)
    // - current target information
    // - fuel & shield alerts
        
    // draw starfield, related to ship motion:
    m_starField.Draw( _frame, m_ship.momentum * 2 );
    _SetupSound( eSound::spaceWind, m_ship, std::max( std::min( m_ship.momentum.Distance() / 20, 1.0 ), 0.2 ), true );

    const Vector screenCenter{ static_cast< double >( m_windows.GetDimension().width ) / 2, static_cast< double >( m_windows.GetDimension().height ) / 2 };
    const Vector shipPosition{ screenCenter - m_ship.position };

    // draw particules:
    for( auto & particule : m_particules )
        _frame.FillCircle( particule->position + shipPosition, particule->width, Color_d::FireColor( ( 3 - particule->lifeSpan ) / 3 ) );

    // draw enemies:
    for( auto & enemy : m_enemies ) {
        if( m_pTarget == &enemy->rocket ) {
            _frame.Line( screenCenter, enemy->rocket.position + shipPosition, { 1, 0.5, 0.1 }, 0.75 );
            const auto vector{ enemy->rocket.position - m_ship.position };
            const auto distance{ static_cast< int >( vector.Distance() ) };
            if( distance > 500 ) { // 500 is "close"
                const auto position{ Vector::From( vector.Orientation(), m_ship.dynamic.boundingBoxRadius + 50 ) };
                // TODO distance unit?
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

    // draw ship:
    m_ship.Draw( _frame, shipPosition );
}