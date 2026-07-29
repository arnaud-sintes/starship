#pragma once

#include "Rocket.h"
#include "Laser.h"
#include "Enemy.h"
#include "Missile.h"
#include "Particule.h"
#include "Goody.h"
#include "Mine.h"
#include "GravityMine.h"
#include "AttractorField.h"
#include "WindField.h"
#include "AudioDirector.h"


// --------------
// player intents for one tick, decoded from the OS by the Game layer
struct PlayerInput
{
    Vector cursorPosition; // window coordinates
    bool fire{ false };
    bool thrust{ false };
};


// --------------
// The simulation: owns every entity and the game state, updates one fixed tick at a
// time and emits audio events; knows nothing about windows, OpenGL or NanoVG frames.
class World
{
public:
    World( AudioDirector & _audio, const Dimension_ui & _screenDimension, const int _frameRate );
    ~World(); // stops the looped sounds so a world can be thrown away and rebuilt (retry)

public:
    void Update( const PlayerInput & _input );
    void UpdateAmbientSound(); // space wind, also audible during the prologue

    // read access for the renderers:
    const Rocket & Ship() const { return m_ship; }
    const std::vector< Enemy > & Enemies() const { return m_enemies; }
    const std::vector< Laser > & Lasers() const { return m_lasers; }
    const std::vector< Missile > & Missiles() const { return m_missiles; }
    const std::vector< Particule > & Particules() const { return m_particules; }
    const std::vector< Goody > & Goodies() const { return m_goodies; }
    const std::vector< Mine > & Mines() const { return m_mines; }
    const std::vector< GravityMine > & GravityMines() const { return m_gravityMines; }
    const AttractorField & Attractors() const { return m_attractors; }
    const Rocket * ClosestEnemy( const Vector & _position ) const;

    bool PlasmaShieldActive() const { return m_plasmaShield > 0; }
    double PlasmaShieldRadius() const { return m_plasmaShieldRadius; }
    double PlasmaShieldRamp() const { return m_plasmaShieldRamp; }

    bool TurretActive() const { return m_turretTicks > 0; }
    const Vector & TurretPosition() const { return m_turretPosition; }
    double TurretOrientation() const { return m_turretOrientation; }

    bool DecoyActive() const { return m_decoyTicks > 0; }
    const Vector & DecoyPosition() const { return m_decoyPosition; }

    bool SingularityActive() const { return m_singularityTicks > 0; }
    const Vector & SingularityPosition() const { return m_singularityPosition; }
    double SingularityProgress() const { return m_singularityTotalTicks > 0 ? 1.0 - static_cast< double >( m_singularityTicks ) / m_singularityTotalTicks : 0; }

    bool ShipDestroyed() const { return m_shipDestroyed; }
    int ShipDestroyedTicks() const { return m_shipDestroyedTicks; }

    // an explosion with a range: damages, pushes and chain-triggers whatever stands
    // in its impact zone, then lives on a few ticks as an expanding shockwave ring
    struct Blast
    {
        Vector position;
        double radius;
        double damage;  // at the blast center, quadratic falloff to the edge
        double impulse; // knockback momentum at the blast center, same falloff
        Color_d ringColor;
        bool ignoreShip{ false }; // friendly blasts (repulsor) spare the ship
        int age{ 0 };
        static constexpr int maxAge{ 22 }; // shockwave ring lifetime, in ticks
    };
    const std::vector< Blast > & Blasts() const { return m_blasts; }

    // dumb-fire sniper projectile: fast, unguided, dodgeable
    struct Slug
    {
        Vector position;
        Vector momentum;
        int lifeSpan{ 0 };
        bool dead{ false };
        static constexpr int maxLifeSpan{ 130 };
    };
    const std::vector< Slug > & Slugs() const { return m_slugs; }

    struct HudInfo
    {
        double shieldValue, shieldCapacity;
        double propellantValue, propellantCapacity;
        int laserPowerPercent;
        int homingMissiles;
        int magneticMines;
        int plasmaShieldSeconds;
        int turretSeconds;
        int decoySeconds;
        int empSeconds;
        int overdriveSeconds;
        int blossomSeconds;
        int singularitySeconds;
        int score;
    };
    HudInfo GetHudInfo() const;

private:
    enum class eExplosion : int
    {
        small = 20,
        medium = 100,
        big = 200,
    };

private:
    void _AddEnemy( const Enemy::eType _type );
    void _SteerEnemy( Enemy & _enemy );
    void _EnemyAction( Enemy & _enemy );
    Rocket * _ClosestEnemy( const Vector & _position );
    void _HandleControls( const PlayerInput & _input );
    void _ResetBursts();
    void _AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size, const eFadeColor _color = eFadeColor::orange );
    void _AddExplosion( const Vector & _position, const Vector & _direction, const eExplosion _explosion = eExplosion::medium, const eFadeColor _color = eFadeColor::orange );
    void _AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter );
    void _AddEnginesParticules( const Rocket & _rocket );
    static bool _RocketCollision( const Rocket & _a, const Rocket & _b );
    void _LaserImpact( const Laser & _a, const Vector & _b, const eFadeColor _color = eFadeColor::orange );
    void _LaserImpact( Laser & _a, Rocket & _b );
    void _RocketImpact( Rocket & _a, Rocket & _b );
    bool _LaserRocketCollision( Laser & _laser, Rocket & _other );
    bool _MissileRocketCollision( Missile & _missile, Rocket & _other );
    void _CollectGoody( const Goody::eType _type );
    void _MaybeDropGoody( const Vector & _position );
    void _UpdateAttractions();
    void _UpdatePlasmaShield();
    void _UpdateBlasts();
    void _ApplyBlast( const Blast & _blast );
    void _Detonate( const Vector & _position, const Vector & _direction, const eExplosion _explosion, const eFadeColor _color, const double _radius, const double _damage, const double _impulse, const bool _ignoreShip = false );
    void _ExplodeMissile( Missile & _missile );
    void _UpdateEnemyCollisions();
    void _UpdateLaserCollisions();
    void _UpdateMissileCollisions();
    void _UpdateShipAttractorCollisions();
    void _UpdateEnemies();
    void _UpdateMines();
    void _UpdateGravityMines();
    void _UpdateAttractorsDeletion();
    void _UpdateGoodies();
    void _UpdateBonuses();
    void _UpdateTurret();
    void _UpdateBlossom();
    void _UpdateSingularity();
    Vector _SingularityPull( const Vector & _position ) const;
    void _UpdateLasers();
    void _UpdateMissiles();
    void _UpdateSlugs();
    void _UpdateShip();
    void _UpdateWind();
    void _UpdateAlerts();
    void _UpdateEngineSounds();
    void _UpdateParticules();
    void _SpawnMissile( const Rocket & _launcher, const bool _targetShip, const double _orientationOffset = 0, const double _spawnDistance = 0, const double _kick = 0 );
    void _DestroyShip();
    void _AddScore( const int _points );

    Vector _RelativeToShip( const Vector & _position ) const { return _position - m_ship.position; }
    double _AttractionQueryRange( const Rocket & _rocket ) const;
    Vector _AttractorPull( const Vector & _position, const double _mass ); // pull of the attractor fields on a free object
    bool _TouchesAttractor( const Vector & _position, const double _radius );

private:
    AudioDirector & m_audio;
    const Dimension_ui m_screenDimension;
    const Vector m_screenCenter;
    const int m_frameRate;

    unsigned long long m_nextRocketId{ 1 };
    Rocket m_ship;
    std::vector< Enemy > m_enemies;
    std::vector< Laser > m_lasers;
    std::vector< Missile > m_missiles;
    std::vector< Particule > m_particules;
    std::vector< Goody > m_goodies;
    std::vector< Mine > m_mines;
    std::vector< GravityMine > m_gravityMines;
    std::vector< Slug > m_slugs;
    AttractorField m_attractors;

private:
    int m_score{ 0 };

    enum class eLaserSpeed : int
    {
        slow = 10,
        medium = 8,
        fast = 6,
    };
    eLaserSpeed m_laserSpeed{ eLaserSpeed::slow };

    enum class eLaserPass : int
    {
        one = 1,
        two = 2,
        four = 4,
        six = 6,
        height = 8
    };
    eLaserPass m_laserPass{ eLaserPass::one };

    int m_homingMissiles{ 0 };
    int m_magneticMines{ 0 };

    // firing cadences (previously function-local statics):
    int m_laserCadence{ 0 };
    int m_laserAlternate{ 0 };
    int m_missileCadence{ 0 };
    int m_mineCadence{ 0 };

private:
    int m_plasmaShield{ 0 };
    double m_plasmaShieldIncrement{ 0 };
    double m_plasmaShieldRamp{ 0 };
    double m_plasmaShieldRadius{ 0 };

private: // turret bonus, a temporary mini-ship orbiting the main one:
    int m_turretTicks{ 0 };
    double m_turretOrbitAngle{ 0 };
    double m_turretOrientation{ 0 };
    int m_turretCadence{ 0 };
    Vector m_turretPosition;

private: // other temporary bonuses:
    int m_decoyTicks{ 0 };
    Vector m_decoyPosition;
    int m_decoyHp{ 0 };
    int m_empTicks{ 0 };
    int m_overdriveTicks{ 0 };
    double m_shipBaseEnginePower{ 0 };
    int m_blossomTicks{ 0 };
    double m_blossomPhase{ 0 };
    int m_singularityTicks{ 0 };
    int m_singularityTotalTicks{ 0 };
    Vector m_singularityPosition;

private:
    AudioDirector::Loop m_sound_spaceWind;
    AudioDirector::Loop m_sound_shipMainEngine;
    AudioDirector::Loop m_sound_shipRotationEngine;

    bool m_shieldAlert{ false };
    bool m_fuelAlert{ false };

    bool m_shipDestroyed{ false };
    int m_shipDestroyedTicks{ 0 };

    WindField m_wind;

    std::vector< Blast > m_pendingBlasts; // enqueued during a tick, applied the next one (chains ripple)
    std::vector< Blast > m_blasts;        // applied, still ringing

    // reused each tick to avoid per-frame allocations:
    std::vector< Rocket * > m_attractedRockets;
};
