#pragma once

#include "Rocket.h"
#include "Laser.h"
#include "Enemy.h"
#include "Missile.h"
#include "StarField.h"
#include "Particule.h"
#include "Goody.h"
#include "Mine.h"
#include "MiniAudio.h"
#include "core/Packer.h"


// --------------
class Renderer
{
public:
    Renderer( const Win32::Windows & _windows, const Packer::Resources & _resources, const int _frameRate );
    ~Renderer() = default;

private:
    void _AddEnemy();

public:
    void Loop( const NanoVGRenderer::Frame & _frame );

private:
    enum class eExplosion : int {
        small = 20,
        medium = 100,
        big = 200,
    };

private:
    Rocket * _ClosestEnemy( const Vector & _position );
    void _Keys();
    void _Reset();
    void _AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size, const eFadeColor _color = eFadeColor::orange );
    void _AddExplosion( const Vector & _position, const Vector & _direction, const eExplosion _explosion = eExplosion::medium, const eFadeColor _color = eFadeColor::orange );
    void _AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter );
    void _AddEnginesParticules( const Rocket & _rocket );
    bool _RocketCollision( const Rocket & _a, const Rocket & _b );
    void _TransmitMomentum( const Vector & _position, const Vector & _momentum, const double _impact, Rocket & _b );
    void _TransmitMomentum( Rocket & _a, Rocket & _b );
    void _LaserImpact( const Laser & _a, const Vector & _b, const eFadeColor _color = eFadeColor::orange );
    void _LaserImpact( Laser & _a, Rocket & _b );
    void _RocketImpact( Rocket & _a, Rocket & _b );
    bool _LaserRocketCollision( Laser & _laser, Rocket & _other );
    bool _MissileRocketCollision( Missile & _missile, Rocket & _other );
    void _Goody( const Goody::eType _type );
    void _Update();
    void _Draw( const NanoVGRenderer::Frame & _frame );

    enum class eSound {
        lowFuelAlert,
        lowShieldAlert,
        spaceWind,
        laserShot,
        laserCollision,
        missileShot,
        missileRun,
        missileExplosion,
        shipCollision,
        shipExplosion,
        shipRotationEngine,
        shipMainEngine,
        laserPowerUp,
        homingMissiles,
        homingMissilesOff,
        magneticMines,
        magneticMinesOff,
        magneticMinesDrop,
        mineExplosion,
        plasmaShield,
        plasmaShieldOff,
        shieldRepair,
        propellantRefuel,
        attractorLaserCollision,
        attractorExplosion,
        attractorShipCollision,
    };
    const MiniAudio::Sound & _SetupSound( const MiniAudio::Sound & _sound, const Vector & _relativePosition, const double _pitch = 0, const bool _loop = false, const std::optional< double > & _volume = {} );
    const MiniAudio::Sound & _SetupSound( const MiniAudio::Sound & _sound, const Rocket & _rocket, const double _pitch = 0, const bool _loop = false, const std::optional< double > & _volume = {} );
    const MiniAudio::Sound & _SetupSound( const eSound _sound, const Rocket & _rocket, const double _pitch = 0, const bool _loop = false, const std::optional< double > & _volume = {} );
    void _QueueSoundPlay( const MiniAudio::Sound & _sound );
    void _PurgeSoundQueue();
    void _DisplayInfos( const NanoVGRenderer::Frame & _frame );

    bool _IsPrologue();
    void _DisplayPrologue( const NanoVGRenderer::Frame & _frame, const Vector & _screenCenter );
    void _DrawCursor( const NanoVGRenderer::Frame & _frame );

private:
    const Win32::Windows & m_windows;
    const int m_frameRate;
    StarField m_starField;
    Rocket m_ship;
    std::list< std::unique_ptr< Enemy > > m_enemies;
    std::list< std::unique_ptr< Laser > > m_lasers;
    std::list< std::unique_ptr< Missile > > m_missiles;
    std::list< std::unique_ptr< Particule > > m_particules;
    std::list< std::unique_ptr< Goody > > m_goodies;
    std::list< std::unique_ptr< Mine > > m_mines;

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

private:
    int m_plasmaShield{ 0 };
    double m_plasmaShieldIncrement{ 0 };
    double m_plasmaShieldRamp{ 0 };
    double m_plasmaShieldRadius{ 0 };
    double m_plasmaShieldReflectAnimation{ 0 };

private:
    MiniAudio m_audioEngine;
    std::unordered_map< eSound, MiniAudio::Sound > m_sounds;
    struct Sound
    {
        MiniAudio::Sound sound;
        int lifeSpan;
    };
    std::list< Sound > m_soundQueue;
    bool m_shieldAlert{ false };
    bool m_fuelAlert{ false };

    struct Attractor
    {
        Vector position;
        double mass;
        double shield;
        MiniAudio::Sound sound_laserCollision;
        MiniAudio::Sound sound_shipCollision;
        MiniAudio::Sound sound_explosion;
    };
    std::list< Attractor > m_attractors;
    inline static const double m_attractorMassSizeRatio{ 50 };
    inline static const double m_attractorDistanceThreshold{ 40 };

    int m_mineDrop{ 0 };

    Vector m_solarWind{ 0.05, 0.2 };
    Vector m_solarWindCurrent, m_solarWindTarget;
    int m_solarWindIndex{ 0 }, m_solarWindCount{ 0 };

private:
    enum class eStep
    {
        stage11_prologue,
        stage11,
    };

    eStep m_step = { eStep::stage11_prologue };
};