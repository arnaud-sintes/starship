#pragma once

#include "Rocket.h"
#include "Laser.h"
#include "Enemy.h"
#include "Missile.h"
#include "StarField.h"
#include "Particule.h"
#include "Goody.h"
#include "MiniAudio.h"
#include "core/Packer.h"


// --------------
class Renderer
{
public:
    Renderer( const Win32::Windows & _windows, const Packer::Resources & _resources );
    ~Renderer() = default;

private:
    void _AddEnemy();

public:
    void Loop( const NanoVGRenderer::Frame & _frame );

private:
    inline static const int bigExplosion{ 200 };
    inline static const int mediumExplosion{ 200 };
    inline static const int smallExplosion{ 20 };

private:
    Rocket * _ClosestEnemy( const Vector & _position );
    void _Keys();
    void _Reset();
    void _AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size );
    void _AddExplosion( const Vector & _position, const Vector & _direction, const int _count = mediumExplosion );
    void _AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter );
    void _AddEnginesParticules( const Rocket & _rocket );
    bool _RocketCollision( const Rocket & _a, const Rocket & _b );
    void _TransmitMomentum( const Vector & _position, const Vector & _momentum, const double _impact, Rocket & _b );
    void _TransmitMomentum( Rocket & _a, Rocket & _b );
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
        plasmaShield,
        plasmaShieldOff,
        shieldRepair,
        propellantRefuel,
    };
    MiniAudio::Sound & _SetupSound( MiniAudio::Sound & _sound, const Rocket & _rocket, const double _pitch = 0, const bool _loop = false, const std::optional< double > & _volume = {} );
    MiniAudio::Sound & _SetupSound( const eSound _sound, const Rocket & _rocket, const double _pitch = 0, const bool _loop = false, const std::optional< double > & _volume = {} );
    void _QueueSoundPlay( MiniAudio::Sound & _sound );
    void _PurgeSoundQueue();
    void _DisplayInfos( const NanoVGRenderer::Frame & _frame );

private:
    const Win32::Windows & m_windows;
    StarField m_starField;
    Rocket m_ship;
    std::list< std::unique_ptr< Enemy > > m_enemies;
    std::list< std::unique_ptr< Laser > > m_lasers;
    std::list< std::unique_ptr< Missile > > m_missiles;
    std::list< std::unique_ptr< Particule > > m_particules;
    std::list< std::unique_ptr< Goody > > m_goodies;    

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
};