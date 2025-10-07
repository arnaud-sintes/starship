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
// 
// - functional:
//      - "best fit" regarding current resolution option
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
//#define _NO_ATTRACTORS
//#define _TESTING_GOODIES
#define _ENEMY_COUNT 2
#define _ATTRACTORS_COUNT 1000


Renderer::Renderer( const Win32::Windows & _windows, const Packer::Resources & _resources, const int _frameRate )
    : m_windows{ _windows }
    , m_frameRate{ _frameRate }
    , m_starField{ m_windows.GetDimension() }
    , m_ship{ { 0.5, 0.75, 1 },  {}, Maths::PiHalf, { 0, -5 }, {}, 0,
        5, // damage
        { 5, 5, 0.01, 0.5 }, // shield
        { 20, 20, 0.01, 0.75 }, // propellant
        { 0, 0.5, false, 0.005, 0.01, 0.75, 0 }, // engine
        { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } } // rotators
    , m_plasmaShield{ m_frameRate * 10 } // 10 seconds immunity at startup
{
    m_sound.Init( _windows );
    m_sound.Load( {
            { static_cast< size_t >( eSound::lowFuelAlert ), _resources.find( "lowFuelAlert.wav" )->second },
            { static_cast< size_t >( eSound::lowShieldAlert ), _resources.find( "lowShieldAlert.wav" )->second },
            { static_cast< size_t >( eSound::spaceWind ), _resources.find( "spaceWind.wav" )->second },
            { static_cast< size_t >( eSound::laserShot ), _resources.find( "laserShot.wav" )->second },
            { static_cast< size_t >( eSound::laserCollision ), _resources.find( "laserCollision.wav" )->second },
            { static_cast< size_t >( eSound::missileShot ), _resources.find( "missileShot.wav" )->second },
            { static_cast< size_t >( eSound::missileRun ), _resources.find( "missileRun.wav" )->second },
            { static_cast< size_t >( eSound::missileExplosion ), _resources.find( "missileExplosion.wav" )->second },
            { static_cast< size_t >( eSound::shipCollision ), _resources.find( "shipCollision.wav" )->second },
            { static_cast< size_t >( eSound::shipExplosion ), _resources.find( "shipExplosion.wav" )->second },
            { static_cast< size_t >( eSound::shipRotationEngine ), _resources.find( "shipRotationEngine.wav" )->second },
            { static_cast< size_t >( eSound::shipMainEngine ), _resources.find( "shipMainEngine.wav" )->second },
            { static_cast< size_t >( eSound::laserPowerUp ), _resources.find( "laserPowerUp.wav" )->second },
            { static_cast< size_t >( eSound::homingMissiles ), _resources.find( "homingMissiles.wav" )->second },
            { static_cast< size_t >( eSound::homingMissilesOff ), _resources.find( "homingMissilesOff.wav" )->second },
            { static_cast< size_t >( eSound::magneticMines ), _resources.find( "magneticMines.wav" )->second },
            { static_cast< size_t >( eSound::magneticMinesOff ), _resources.find( "magneticMinesOff.wav" )->second },
            { static_cast< size_t >( eSound::magneticMinesDrop ), _resources.find( "magneticMinesDrop.wav" )->second },
            { static_cast< size_t >( eSound::mineExplosion ), _resources.find( "mineExplosion.wav" )->second },
            { static_cast< size_t >( eSound::plasmaShield ), _resources.find( "plasmaShield.wav" )->second },
            { static_cast< size_t >( eSound::plasmaShieldOff ), _resources.find( "plasmaShieldOff.wav" )->second },
            { static_cast< size_t >( eSound::shieldRepair ), _resources.find( "shieldRepair.wav" )->second },
            { static_cast< size_t >( eSound::propellantRefuel ), _resources.find( "propellantRefuel.wav" )->second },
            { static_cast< size_t >( eSound::attractorLaserCollision ), _resources.find( "attractorLaserCollision.wav" )->second },
            { static_cast< size_t >( eSound::attractorExplosion ), _resources.find( "attractorExplosion.wav" )->second },
            { static_cast< size_t >( eSound::attractorShipCollision ), _resources.find( "attractorShipCollision.wav" )->second },
        } );

    m_ship.momentum -= m_solarWind;

    m_sound_spaceWind = m_sound.Play( static_cast< size_t >( eSound::spaceWind ), { 0.2 }, true );
    m_sound_shipMainEngine = m_sound.Play( static_cast< size_t >( eSound::shipMainEngine ), { 0, {}, 0 }, true );
    m_sound_shipRotationEngine = m_sound.Play( static_cast< size_t >( eSound::shipRotationEngine ), { 0, {}, 0 }, true );

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
    m_goodies.emplace_back( new Goody{ { 700, 200 }, Goody::eType::magneticMines } );
    #endif

    #ifndef _NO_ATTRACTORS
    for( int i{ 0 }; i < _ATTRACTORS_COUNT; i++ ) {
        const auto mass{ Maths::Random( 1, 2 ) };
        const double range{ 10000 };
        const double securityDistance{ 200 };
        L_generate:
        double x{ 0 }, y{ 0 };
        while( x > -securityDistance && x < securityDistance ) x = Maths::Random( -range, range );
        while( y > -securityDistance && y < securityDistance ) y = Maths::Random( -range, range );
        for( const auto & attractor : m_attractors )
            if( Maths::Collision( { x, y }, mass * m_attractorMassSizeRatio, attractor.position, attractor.mass * m_attractorMassSizeRatio ) )
                goto L_generate;
        m_attractors.emplace_back( Attractor{ { x, y }, mass, 10.0 * mass } );
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
            static_cast< int >( Maths::Random( 0, m_frameRate * 5 ) ),
            m_sound.Play( static_cast< size_t >( eSound::shipMainEngine ), { 0, {}, 0 }, true )->get(),
            m_sound.Play( static_cast< size_t >( eSound::shipRotationEngine ), { 0, {}, 0 }, true )->get()
        } );
}


void Renderer::Loop( const NanoVGRenderer::Frame & _frame )
{
    // prologues:
    if( _IsPrologue() ) {
        _Draw( _frame );
        return;
    }

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
    // activate ship burst:
    if( m_windows.RightMouseButtonPressed() )
        m_ship.ActivateThrust();

    // follow the mouse cursor:
    const auto screenCenter{ m_windows.GetDimension().As< Vector, double >() * 0.5 };
    m_ship.PointTo( Vector::From( m_windows.CursorPosition().ToType< double >() ) - screenCenter + m_ship.position, 0.5 );

    // laser:
    static int laserShotRate{ 0 };
    if( laserShotRate++ % static_cast< int >( m_laserSpeed ) == 0  && m_windows.LeftMouseButtonPressed() ) {
        m_sound.Play( static_cast< size_t >( eSound::laserShot ) );
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
    if( m_homingMissiles > 0 && missileShotRate++ > 25 && m_windows.LeftMouseButtonPressed() ) {
        m_homingMissiles--;
        if( m_homingMissiles == 0 )
            m_sound.Play( static_cast< size_t >( eSound::homingMissilesOff ) );
        missileShotRate = 0;
        Vector motion{};
        if( _ClosestEnemy( m_ship.position ) == nullptr )
            motion = Vector::From( m_ship.orientation, -5 );
        m_missiles.emplace_back( new Missile{ Rocket{ { 0.5, 0.75, 1 }, m_ship.position, m_ship.orientation, motion, m_ship.momentum, 0,
            3, // damage
            { 1, 1, 0.01, 0.5 }, // shield
            { 10, 10, 0.005, 0.9 }, // propellant
            { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
            //{ { 0, 0 }, 0, { false, false }, 0, 0, 0, { 0, 0 } } }, // non-guided missiles rotator
            { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0.5, { 0, 0 } } }, // guided missiles rotator
            false, m_ship,
            m_sound.Play( static_cast< size_t >( eSound::missileRun ), { 0, {}, 0 }, true )->get()
            } );
        m_sound.Play( static_cast< size_t >( eSound::missileShot ) );
    }
    
    // drop mines:
    static int mineDropRate{ 0 };
    if( m_magneticMines > 0 && mineDropRate++ > 60 ) {
        mineDropRate = 0;
        double shortestDistance{ 1000 };
        for( const auto & mine : m_mines ) {
            const auto distance{ ( m_ship.position - mine->position ).Distance() };
            if( distance < shortestDistance )
                shortestDistance = distance;
        }
        // must be far enough of other mines:
        if( shortestDistance > 50 ) {
            m_mineDrop = 0;
            m_mines.emplace_back( new Mine{ m_ship.position, 0.5 } );
                        
            m_magneticMines--;
            m_sound.Play( static_cast< size_t >( eSound::magneticMinesDrop ) );
            if( m_magneticMines == 0 )
                m_sound.Play( static_cast< size_t >( eSound::magneticMinesOff ) );
        }
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


void Renderer::_AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size, const eFadeColor _color )
{
    const auto momentum{ Vector::From( _orientation, _speed ) + _direction };
    m_particules.emplace_back( new Particule{ _position, momentum, Maths::Random( 1.5, 3 ), _size, _color } );
}


void Renderer::_AddExplosion( const Vector & _position, const Vector & _direction, const eExplosion _explosion, const eFadeColor _color )
{
    static const std::unordered_map< eExplosion, double > ranges{
        { eExplosion::small, 1 },
        { eExplosion::medium, 2 },
        { eExplosion::big, 3 },
    };
    const auto rangeMax{ ranges.find( _explosion )->second };
    for( int i{ 0 }; i < static_cast< int >( _explosion ); i++ )
        _AddParticule( _position, _direction, Maths::Random( 0, Maths::Pi2 ), Maths::Random( 0, 4 ), Maths::Random( 0.5, rangeMax ), _color );
}


void Renderer::_AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter )
{
    const int passCount{ _rate < 0.5 ? 1 : _maxPass };
    const double maxSize{ 0.5 + _rate * _limiter }; // [0.5, 0.5 to 1.5]
    const double maxSpeed{ 1 + _rate }; // [0.5, 1 to 2]
    const double maxWideness{ 0.25 + _rate * 0.75 * _limiter }; // [-0.25, +0.25] to [-1, +1]
    for( int i{ 0 }; i < passCount; i++ )
        _AddParticule( _position + _burst.position, {}, _burst.orientation + Maths::Random( -maxWideness, maxWideness ), Maths::Random( 0.5, maxSpeed ), Maths::Random( 0.5, maxSize ), eFadeColor::violet );
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


void Renderer::_LaserImpact( const Laser & _a, const Vector & _b, const eFadeColor _color )
{   
    const auto momentum{ _a.momentum * 0.01 };
    _AddExplosion( _b, Vector{} - momentum, eExplosion::small, _color );
}


void Renderer::_LaserImpact( Laser & _a, Rocket & _b )
{   
    const auto momentum{ _a.momentum * 0.01 };
    // transmit momentum:
    _TransmitMomentum( _a.position, momentum, 0.05, _b );
    // add small impact explosion on target:
    _AddExplosion( _b.position, Vector{} - momentum, eExplosion::small );
    // shield impact:
    _b.shield.value -= _a.damage;
}


void Renderer::_RocketImpact( Rocket & _a, Rocket & _b )
{   
    // transmit momentum:
    _TransmitMomentum( _a, _b );
    // add small impact explosion on target:
    _AddExplosion( _b.position, Vector{} - _a.momentum, eExplosion::medium );
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
        if( !Maths::Collision( _missile.rocket.position, _missile.rocket.dynamic.boundingBoxRadius * 2, _other.position, _other.dynamic.boundingBoxRadius * 4 ) )
            _missile.bypassCollision = true;
        return false;
    }
    if( !_RocketCollision( _missile.rocket, _other ) )
        return false;
    _RocketImpact( _missile.rocket, _other );
    return true;
}

// TODO better sound param depending if it's static or dynamic?
CuteSound::Param Renderer::_SoundParam( const Vector & _relativePosition, const std::optional< double > & _volume ) const
{
    const double maxDistance{ 2000 };
    const auto volume{ 1.0 - std::min( _relativePosition.Distance() / maxDistance, 0.9 ) };
    const auto pan{ _relativePosition.u / 1000 };
    return { volume * ( _volume ? *_volume : 1 ), std::clamp( pan + 0.5, 0.0, 1.0 ), {} };
}

CuteSound::Param Renderer::_SoundParam( const Rocket & _rocket, const std::optional< double > & _volume ) const
{
    return _SoundParam( _rocket.position - m_ship.position,  _volume );
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
            m_sound.Play( static_cast< size_t >( eSound::laserPowerUp ) );
        return;
    }
    if( _type == Goody::eType::homingMissiles ) {
        m_homingMissiles += 30; // 30x missiles pack
        m_sound.Play( static_cast< size_t >( eSound::homingMissiles ) );
        return;
    }
    if( _type == Goody::eType::magneticMines ) {
        m_magneticMines += 10; // 10x mines pack
        m_sound.Play( static_cast< size_t >( eSound::magneticMines ) );
        return;
    }
    if( _type == Goody::eType::plasmaShield ) {
        m_plasmaShield += m_frameRate * 5; // 5 seconds plasma shield
        m_sound.Play( static_cast< size_t >( eSound::plasmaShield ) );
        return;
    }
    if( _type == Goody::eType::shieldAdd ) {
        if( m_ship.shield.value >= m_ship.shield.capacity )
            return;
        const auto capacityBoost{ m_ship.shield.capacity * 0.5 }; // 50% capacity boost
        m_ship.shield.value += capacityBoost;
        if( m_ship.shield.value > m_ship.shield.capacity )
            m_ship.shield.value = m_ship.shield.capacity;
        m_sound.Play( static_cast< size_t >( eSound::shieldRepair ) );
        return;
    }
    if( _type == Goody::eType::propellantAdd ) {
        if( m_ship.propellant.value >= m_ship.propellant.capacity )
            return;
        const auto capacityBoost{ m_ship.propellant.capacity * 0.5 }; // 50% capacity boost
        m_ship.propellant.value += capacityBoost;
        if( m_ship.propellant.value > m_ship.propellant.capacity )
            m_ship.propellant.value = m_ship.propellant.capacity;
        m_sound.Play( static_cast< size_t >( eSound::propellantRefuel ) );
        return;
    }
}


void Renderer::_Update()
{
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
        const double precisionFactor{ 10000 }; // because of cumulative small additions
        for( const auto & attractor : m_attractors )
            attraction += ( attractor.position.ProximityAttraction( rocket.position, attractor.mass * rocket.dynamic.totalMass, m_attractorDistanceThreshold ) * precisionFactor );
        attraction *= ( 1 / precisionFactor );
        rocket.dynamic.attraction = attraction;
    }

    // plasma shield:
    if( m_plasmaShield > 0 ) {
        m_plasmaShield--;
        if( m_plasmaShield == 0 )
            m_sound.Play( static_cast< size_t >( eSound::plasmaShieldOff ) );
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
                    m_sound.Play( static_cast< size_t >( eSound::shipCollision ), _SoundParam( enemy->rocket ) );
                    _RocketImpact( enemy->rocket, enemyOther->rocket );
                }
        // enemy-ship collision:
        if( _RocketCollision( enemy->rocket, m_ship ) ) {
            m_sound.Play( static_cast< size_t >( eSound::shipCollision ), _SoundParam( enemy->rocket ) );
            _RocketImpact( enemy->rocket, m_ship );
            _RocketImpact( m_ship, enemy->rocket );
        }
        // enemy-plasma shield collision:
        if( m_plasmaShield > 0 && Maths::Collision( enemy->rocket.position, enemy->rocket.dynamic.boundingBoxRadius, m_ship.position, m_plasmaShieldRadius ) ) {
            m_sound.Play( static_cast< size_t >( eSound::shipCollision ), _SoundParam( enemy->rocket ) );
            _RocketImpact( m_ship, enemy->rocket );
            m_score++;
        }
        // enemy-mines collision:
        for( auto & mine : m_mines ) {
            if( Maths::Collision( enemy->rocket.position, enemy->rocket.dynamic.boundingBoxRadius, mine->position, mine->dynamic.radius ) ) {
                mine->alive = false;
                enemy->rocket.shield.value -= mine->damage;
                m_score += 5;
            }
        }
        // enemy-attractors collision:
        for( auto & attractor : m_attractors )
            if( Maths::Collision( enemy->rocket.position, enemy->rocket.dynamic.boundingBoxRadius, attractor.position, attractor.mass * m_attractorMassSizeRatio ) ) {
                enemy->rocket.shield.value = -1;
                attractor.shield -= enemy->rocket.damage;
            }
    }

    // laser collision:
    for( auto & laser : m_lasers ) {
        Rocket * pCollision{ nullptr };
        // laser-ships collisions:
        for( auto itEnemy{ m_enemies.begin() }, itEnemyEnd{ m_enemies.end() }; pCollision == nullptr && itEnemy != itEnemyEnd; ++itEnemy ) {
            if( _LaserRocketCollision( *laser, ( **itEnemy ).rocket ) ) {
                pCollision = &( **itEnemy ).rocket;
                m_sound.Play( static_cast< size_t >( eSound::laserCollision ), _SoundParam( *pCollision ) );
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
        // laser-mines collision:
        for( auto & mine : m_mines ) {
            // too close:
            if( ( mine->position - m_ship.position ).Distance() < 100 )
                continue;
            const auto collision{ Maths::Collision( mine->position, mine->dynamic.radius, laser->dynamic.positionA, laser->dynamic.positionB ) };
            if( !collision )
                continue;
            laser->lifeSpan = laser->maxLifeSpan;
            mine->alive = false;
        }
        // laser-attractors collision:
        for( auto & attractor : m_attractors ) {
            const auto collision{ Maths::Collision( attractor.position, attractor.mass * m_attractorMassSizeRatio, laser->dynamic.positionA, laser->dynamic.positionB ) };
            if( !collision )
                continue;
            laser->lifeSpan = laser->maxLifeSpan;
            m_sound.Play( static_cast< size_t >( eSound::attractorLaserCollision ), _SoundParam( attractor.position - m_ship.position ) );
            _LaserImpact( *laser, *collision, eFadeColor::azure );
            attractor.shield -= laser->damage;
            m_score++;
        }
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
        // missiles-mines collision:
        if( &( ( **it ).origin ) != &m_ship ) // avoid collisions with our own missiles
            for( auto & mine : m_mines )
                if( Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, mine->position, mine->dynamic.radius ) ) {
                    collision = true;
                    mine->alive = false;
                }
        // missiles-attractors collision:
        for( auto & attractor : m_attractors )
            if( Maths::Collision( rocket.position, rocket.dynamic.boundingBoxRadius, attractor.position, attractor.mass * m_attractorMassSizeRatio ) ) {
                collision = true;
                attractor.shield -= rocket.damage;
            }
        // explode and remove:
        if( collision ) {
            m_sound.Play( static_cast< size_t >( eSound::missileExplosion ), _SoundParam( rocket ) );
            _AddExplosion( rocket.position, rocket.momentum );
            it = m_missiles.erase( it );
            m_score++;
            continue;
        }
        ++it;
    }

    // ship-attractors collision:
    for( auto & attractor : m_attractors )
        if( Maths::Collision( m_ship.position, m_ship.dynamic.boundingBoxRadius, attractor.position, attractor.mass * m_attractorMassSizeRatio ) ) {
            m_ship.shield.value -= attractor.mass;
            attractor.shield -= m_ship.damage;
            m_sound.Play( static_cast< size_t >( eSound::attractorShipCollision ) );
            const auto collisionPoint{ Maths::Collision( attractor.position, attractor.mass * m_attractorMassSizeRatio, attractor.position, m_ship.position ) };
            const auto counterMomentum{ Vector{ m_ship.position - attractor.position } * m_ship.momentum.Distance() * 0.01 };
            _AddExplosion( *collisionPoint, counterMomentum, eExplosion::medium, eFadeColor::azure );
            _TransmitMomentum( *collisionPoint, counterMomentum, 0.05 * ( attractor.mass / 15 ), m_ship );
        }

    const auto AddGoody{ [ & ]( const Vector & _position ){
        // 50% chance of goody addition, only if far enough (avoid volontary collision bonuses...)
        if( ( _position - m_ship.position ).Distance() <= 100 || Maths::Random( 0, 1 ) >= 0.5 )
            return;
        std::vector< Goody::eType > types{ Goody::eType::homingMissiles, Goody::eType::magneticMines, Goody::eType::plasmaShield };
        if( m_ship.shield.value < m_ship.shield.capacity ) types.emplace_back( Goody::eType::shieldAdd );
        if( m_ship.propellant.value < m_ship.propellant.capacity ) types.emplace_back( Goody::eType::propellantAdd );
        if( m_laserSpeed != eLaserSpeed::fast || m_laserPass != eLaserPass::height ) types.emplace_back( Goody::eType::laserUp );
        const auto typeRandom{ Maths::Random( 0, static_cast< double >( types.size() ) - 0.01 ) };
        m_goodies.emplace_back( new Goody{ _position, types.at( static_cast< int >( typeRandom ) ) } );
    } };
        
    // update enemies data:
    int newEnemiesToGenerate{ 0 };
    for( auto it{ m_enemies.begin() }; it != m_enemies.end(); ) {
        auto & enemy{ **it };

        const auto thrustVolume{ enemy.rocket.engine.thrust / enemy.rocket.engine.power };
        const auto param{ _SoundParam( enemy.rocket, thrustVolume ) };
        enemy.sound_shipMainEngine.SetParam( { *param.volume * 0.75, param.pan, thrustVolume } );
        const auto rotatorVolume{ ( enemy.rocket.rotator.thrust.at( 0 ) + enemy.rocket.rotator.thrust.at( 1 ) ) / ( 2 * enemy.rocket.rotator.power ) };
        const auto rotatorVolume2{ rotatorVolume > 0.25 ? ( rotatorVolume - 0.25 ) * 1.33 : 0 };
        const auto rotatorParam{ _SoundParam( enemy.rocket, rotatorVolume2 ) };
        enemy.sound_shipRotationEngine.SetParam( { *rotatorParam.volume * 0.75, rotatorParam.pan, rotatorVolume2 } );

        // better NOT aim directly the ship to avoid collisions:
        enemy.rocket.Acquire( m_ship, 0.5, Vector::From( m_ship.orientation + Maths::Pi, 100 ) );
        enemy.rocket.ActivateThrust();
        enemy.rocket.Update();

        // shield:
        if( enemy.rocket.shield.value <= 0 ) {
            AddGoody( enemy.rocket.position );
            m_sound.Play( static_cast< size_t >( eSound::shipExplosion ), _SoundParam( enemy.rocket ) );

            _AddExplosion( enemy.rocket.position, enemy.rocket.momentum, eExplosion::big );
            newEnemiesToGenerate++;
            it = m_enemies.erase( it );
            m_score += 50;
            continue;
        }

        // enemies launch rockets aiming the ship:
        if( enemy.shotRate++ > m_frameRate * 5 ) { // every 5 seconds
            enemy.shotRate = 0;
            m_missiles.emplace_back( new Missile{ Rocket{ { 1, 0.5, 0.75 }, enemy.rocket.position, enemy.rocket.orientation, {}, enemy.rocket.momentum, 0,
                3, // damage
                { 1, 1, 0.01, 0.5 }, // shield
                { 10, 10, 0.01, 0.9 }, // propellant
                { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
                { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0.5, { 0, 0 } } },
                true, enemy.rocket,
                m_sound.Play( static_cast< size_t >( eSound::missileRun ), { 0, {}, 0 }, true )->get()
                } );
            m_sound.Play( static_cast< size_t >( eSound::missileShot ), _SoundParam( enemy.rocket ) );
        }
        ++it;
    }
    for( int i{ 0 }; i < newEnemiesToGenerate; i++ )
        _AddEnemy();

    // mines deletion:
    for( auto it{ m_mines.begin() }; it != m_mines.end(); ) {
        auto & mine{ *it };
        if( !mine->alive ) {
            m_sound.Play( static_cast< size_t >( eSound::mineExplosion ), _SoundParam( mine->position - m_ship.position ) );
            _AddExplosion( mine->position, {}, eExplosion::big, eFadeColor::orange );
            it = m_mines.erase( it );
            continue;
        }
        ++it;
    }

    // attractors deletion:
    for( auto it{ m_attractors.begin() }; it != m_attractors.end(); ) {
        auto & attractor{ *it };
        if( attractor.shield < 0 ) {
            AddGoody( attractor.position );
            m_sound.Play( static_cast< size_t >( eSound::attractorExplosion ), _SoundParam( attractor.position - m_ship.position ) );
            const auto radius{ attractor.mass * m_attractorMassSizeRatio };
            for( int j{ 0 }; j < 2; j++ ) {
                const int divisions{ 8 };
                for( int i{ 0 }; i < divisions; i++ ) {
                    const double angle{ Maths::Pi2 * i / divisions };
                    const double currentRadius{ radius * ( 1.0 / static_cast< double >( j + 1 ) ) };
                    const auto explosionPosition{ attractor.position + Vector::From( angle, currentRadius ) };
                    _AddExplosion( explosionPosition, ( explosionPosition - attractor.position ) * 0.03, eExplosion::medium, eFadeColor::azure );
                }
            }
            it = m_attractors.erase( it );
            continue;
        }
        ++it;
    }

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
            const auto thrustVolume{ missile.rocket.engine.thrust / missile.rocket.engine.power };
            const auto param{ _SoundParam( missile.rocket, thrustVolume ) };
            missile.sound_run.SetParam( { *param.volume * 0.5, param.pan, thrustVolume } );
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
            if( missile.lifeSpan == m_frameRate ) {
                m_sound.Play( static_cast< size_t >( eSound::missileExplosion ), _SoundParam( missile.rocket ) );
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

    // mines attraction:
    for( const auto & mine : m_mines ) {
        Rocket * pTarget{ _ClosestEnemy( mine->position ) };
        if( pTarget != nullptr )
            mine->position += pTarget->position.InfiniteAttraction( mine->position, pTarget->dynamic.totalMass );
    }

    // update ship data:
    m_ship.Update();

    // solar wind:
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

    // ship shield management:
    if( m_ship.shield.value < 0 )
        m_ship.shield.value = 0; // TODO the ship exploded (you lose)
    if( m_ship.shield.value < ( m_ship.shield.capacity * 0.25 ) ) {
        if( !m_shieldAlert ) {
            m_shieldAlert = true;
            m_sound.Play( static_cast< size_t >( eSound::lowShieldAlert ) );
        }
    }
    else
        m_shieldAlert = false;

    // ship propellant management:
    if( m_ship.propellant.value < ( m_ship.propellant.capacity * 0.25 ) ) {
        if( !m_fuelAlert ) {
            m_fuelAlert = true;
            m_sound.Play( static_cast< size_t >( eSound::lowFuelAlert ) );
        }
    }
    else
        m_fuelAlert = false;

    const auto thrustVolume{ m_ship.engine.thrust / m_ship.engine.power };
    m_sound_shipMainEngine->get().SetParam( { thrustVolume * 0.75, {}, thrustVolume } );
    const auto rotatorVolume{ ( m_ship.rotator.thrust.at( 0 ) + m_ship.rotator.thrust.at( 1 ) ) / ( 2 * m_ship.rotator.power ) };
    const auto rotatorVolume2{ rotatorVolume > 0.25 ? ( rotatorVolume - 0.25 ) * 1.33 : 0 };
    m_sound_shipRotationEngine->get().SetParam( { rotatorVolume2 * 0.75, {}, rotatorVolume2 } );

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
}


bool Renderer::_IsPrologue()
{
    return m_step == eStep::stage11_prologue;
}


void Renderer::_DisplayPrologue( const NanoVGRenderer::Frame & _frame, const Vector & _screenCenter )
{
    if( m_step == eStep::stage11_prologue ) {
        _frame.Text( _screenCenter, "openSansBold", 80, "STAGE 1-1", colorWhite, NanoVGRenderer::Frame::eTextAlign::center );
        Vector verticalText{ 0, 100 };
        for( const auto & text : std::list< std::string >{
            "By jumping out of hyperdrive to reach the Rexxus-3b system, you encounter hostile resistance from the",
            "space mining guild who are illegally exploiting the strange attractors energy around the planet Stellis Secura.",
            "",
            "As a faithful member of the Universal Alliance for Peace, you cannot tolerate such a defiance to our glorious",
            "open-despocracy and the uncontestable authority of our galactic emperator Muhammad Silmane XIV!",
            "",
            "Let's teach this gang of small-time smugglers a lesson to remember..." } ) {
            _frame.Text( _screenCenter + verticalText, "openSans", 24, text, { 0.6, 0.85, 1 }, NanoVGRenderer::Frame::eTextAlign::center );
            verticalText.v += 32;
        }
        if( m_windows.LeftMouseButtonPressed() )
            m_step = eStep::stage11;
        return;
    }
}


void Renderer::_DrawCursor( const NanoVGRenderer::Frame & _frame )
{
    static double cursorFlash{ 0 };
    _frame.StrokeCircle( m_windows.CursorPosition().ToType< double >(), 15, { 0.25, 0.5 + std::sin( cursorFlash += 0.15 ) * 0.25, 1 }, 4 );
}


void Renderer::_Draw( const NanoVGRenderer::Frame & _frame )
{
    const Vector screen{ static_cast< double >( m_windows.GetDimension().width ), static_cast< double >( m_windows.GetDimension().height ) };
    const Vector screenCenter{ screen * 0.5 };
    const Vector shipPosition{ screenCenter - m_ship.position };

    // draw starfield, related to ship motion & solar wind:
    m_starField.Draw( _frame, m_ship.momentum * 2 );
    m_sound_spaceWind->get().SetParam( { {}, {}, std::max( std::min( m_ship.momentum.Distance() / 20, 1.0 ), 0.3 ) } );

    // prologues:
    if( _IsPrologue() ) {
        _DisplayPrologue( _frame, screenCenter );
        _DrawCursor( _frame );
        return;
    }

    // draw particules:
    for( auto & particule : m_particules )
        _frame.FillCircle( particule->position + shipPosition, particule->width, particule->GetColor() );

    // draw goodies:
    for( auto & goody : m_goodies )
        goody->Draw( _frame, shipPosition );

    // draw mines:
    for( auto & mines : m_mines )
        mines->Draw( _frame, shipPosition );

    // draw enemies:
    Rocket * pTarget{ _ClosestEnemy( m_ship.position ) };
    for( auto & enemy : m_enemies ) {
        if( pTarget == &enemy->rocket ) {
            _frame.Line( screenCenter, enemy->rocket.position + shipPosition, { 0.1, 0.5, 1 }, 0.75 );
            const auto vector{ enemy->rocket.position - m_ship.position };
            const auto distance{ static_cast< int >( vector.Distance() ) };
            if( distance > 500 ) { // 500 is "close"
                const auto position{ Vector::From( vector.Orientation(), m_ship.dynamic.boundingBoxRadius + 50 ) };
                _frame.Text( position + screenCenter, "openSans", 14, std::to_string( distance / 10 ), { 0.5, 0.75, 1 } );
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
    std::vector< std::function< void( const NanoVGRenderer::Frame & ) > > attractors;
    for( const auto & attractor : m_attractors ) {
        const auto position{ attractor.position + shipPosition };
        const auto radius{ attractor.mass * m_attractorMassSizeRatio };
        // color depends of distance with ship:
        const auto distance{ ( attractor.position - m_ship.position ).Distance() };
        const auto mass{ attractor.mass * m_ship.dynamic.totalMass };
        const double maxDistance{ m_attractorDistanceThreshold * mass };
        std::vector< double > colors;
        for( int i{ 0 }; i < 3; i++ ) {
            const auto currMaxDistance{ maxDistance / ( i + 1 ) };
            colors.emplace_back( distance < currMaxDistance ? ( std::pow( 1 - ( distance / currMaxDistance ), 2 ) ) : 0 );
        }
        const auto intensity{ ( distance < maxDistance ? ( 1 - std::pow( ( distance / maxDistance ), 2 ) ) : 0 ) };
        attractors.emplace_back( [ = ]( const NanoVGRenderer::Frame & _frame ){
            _frame.FillCircle( position, radius, Color_d{ colors.at( 2 ), colors.at( 1 ), colors.at( 0 ) }, false );
            _frame.StrokeCircle( position, radius, Color_d{ 0.25, 0.5, 1 }, intensity * 5.5 + 0.5 );
        } );
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( position, radius * 7, Color_d{ 0.01, 0.2, 0.2, intensity * 0.5 + 0.5 }, Color_d{ 0, 0.05, 0.1, 0 } );
    }
    for( const auto & attractor : attractors )
        attractor( _frame );

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
    _DrawCursor( _frame );
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
    _frame.FillRectangle( { xMenu, yMenu }, { xMenu + shieldRate, yMenu + barHeight }, Color_d::FadeOrange( 1 - ( shieldRate / barWidth ) ) );
    _frame.StrokeRectangle( { xMenu, yMenu }, { xMenu + barWidth, yMenu + barHeight }, colorWhite, strokeWidth, borderRadius );

    // propellant state:
    yMenu = margin + barHeight + spacing;
    _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Propellant:", colorWhite );
    const auto propellantdRate{ std::round( m_ship.propellant.value * barWidth ) / m_ship.propellant.capacity };
    _frame.FillRectangle( { xMenu, yMenu }, { xMenu + propellantdRate, yMenu + barHeight }, Color_d::FadeViolet( 1 - ( propellantdRate / barWidth ) ) );
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

    // magnetic mines count (temporary):
    if( m_magneticMines != 0 ) {
        yMenu = margin + ( barHeight + spacing ) * ++optionalInfos;
        _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Magnetic-mines x" + std::to_string( m_magneticMines ), greenColor );
    }
    
    // plasma shield remaining time (temporary):
    if( m_plasmaShield != 0 ) {
        yMenu = margin + ( barHeight + spacing ) * ++optionalInfos;
        _frame.Text( { xText, yMenu + yText }, "openSansBold", textHeight, "Plasma-shield: " + std::to_string( static_cast< int >( std::ceil( static_cast< double >( m_plasmaShield ) / m_frameRate ) ) ) + "s", greenColor );
    }
}