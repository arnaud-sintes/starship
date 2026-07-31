#pragma once

#include "Audio.h"
#include "core/Maths.h"
#include "core/Win32.h"
#include "core/Packer.h"


// ----------------
enum class eSound : size_t
{
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
    turret,
    turretOff,
    decoy,
    emp,
    overdrive,
    singularity,
    blossom,
    hellstorm,
    repulsor,
    attractorLaserCollision,
    attractorExplosion,
    attractorShipCollision,
    count
};


// ----------------
// Game-facing audio orchestration: maps game events to sounds, computes positional
// volume/pan relative to the listener (the ship, always at screen center), and
// applies per-sound cooldowns so persistent contacts and explosion chains don't
// retrigger the same sample every tick (audible as clipping/crackles).
class AudioDirector
{
public:
    // a failed device init or a missing resource leaves the game silently soundless
    AudioDirector( const Dimension_ui & _screenDimension, const Packer::Resources & _resources );

public:
    bool Initialized() const { return m_initialized; }
    void Update(); // once per game tick

    struct Position
    {
        double volume;
        double pan;
    };
    Position Locate( const Vector & _listenerRelativePosition ) const;

    // one-shots (cooldown-limited):
    void Play( const eSound _sound, const double _volume = 1 );
    void PlayAt( const eSound _sound, const Vector & _listenerRelativePosition );

    // looped sounds (engines, wind...), paused whenever silent to save mixing work:
    struct Loop
    {
        Audio::Handle handle;
        bool paused{ false };
    };
    Loop CreateLoop( const eSound _sound, const Audio::Param & _param = {} );
    void SetLoop( Loop & _loop, const Audio::Param & _param ) const;
    void StopLoop( Loop & _loop );

private:
    bool _Init( const Packer::Resources & _resources );
    static unsigned long long _CooldownTicks( const eSound _sound );
    static double _BaseGain( const eSound _sound ); // per-sound mix trim, applied to every play
    bool _CoolingDown( const eSound _sound );

private:
    Audio m_audio;
    bool m_initialized{ false };
    const double m_maxDistanceVolume;
    const double m_maxDistancePan;
    unsigned long long m_tick{ 0 };
    std::array< unsigned long long, static_cast< size_t >( eSound::count ) > m_lastPlayed{};
};
