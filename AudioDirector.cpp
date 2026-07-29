#include "AudioDirector.h"


AudioDirector::AudioDirector( const Win32::Windows & _windows, const Packer::Resources & _resources )
    : m_maxDistanceVolume{ Vector{ static_cast< double >( _windows.GetDimension().width ), static_cast< double >( _windows.GetDimension().height ) }.Distance() * 0.5 * 1.25 }
    , m_maxDistancePan{ Vector{ static_cast< double >( _windows.GetDimension().width ), static_cast< double >( _windows.GetDimension().height ) }.Distance() * 0.5 * 0.8 }
{
    m_initialized = _Init( _windows, _resources );
}


bool AudioDirector::_Init( const Win32::Windows &, const Packer::Resources & _resources )
{
    if( !m_audio.Init() )
        return false;
    static const std::array< const char *, static_cast< size_t >( eSound::count ) > files{
        "lowFuelAlert.wav",
        "lowShieldAlert.wav",
        "spaceWind.wav",
        "laserShot.wav",
        "laserCollision.wav",
        "missileShot.wav",
        "missileRun.wav",
        "missileExplosion.wav",
        "shipCollision.wav",
        "shipExplosion.wav",
        "shipRotationEngine.wav",
        "shipMainEngine.wav",
        "laserPowerUp.wav",
        "homingMissiles.wav",
        "homingMissilesOff.wav",
        "magneticMines.wav",
        "magneticMinesOff.wav",
        "magneticMinesDrop.wav",
        "mineExplosion.wav",
        "plasmaShield.wav",
        "plasmaShieldOff.wav",
        "shieldRepair.wav",
        "propellantRefuel.wav",
        "attractorLaserCollision.wav",
        "attractorExplosion.wav",
        "attractorShipCollision.wav",
    };
    for( size_t id{ 0 }; id < files.size(); id++ ) {
        const auto itResource{ _resources.find( files.at( id ) ) };
        if( itResource == _resources.end() || !m_audio.Load( id, itResource->second ) )
            return false;
    }
    return true;
}


void AudioDirector::Update()
{
    m_tick++;
    m_audio.Update();
}


AudioDirector::Position AudioDirector::Locate( const Vector & _listenerRelativePosition ) const
{
    return {
        1.0 - std::min( _listenerRelativePosition.Distance() / m_maxDistanceVolume, 1.0 ),
        std::clamp( ( _listenerRelativePosition.u / m_maxDistancePan ) + 0.5, 0.0, 1.0 )
    };
}


unsigned long long AudioDirector::_CooldownTicks( const eSound _sound )
{
    switch( _sound ) {
        // contact sounds retrigger as long as objects overlap - keep them sparse:
        case eSound::shipCollision:
        case eSound::laserCollision:
        case eSound::attractorLaserCollision:
        case eSound::attractorShipCollision:
            return 6;
        // explosion chains (mines, missile packs...) easily stack in a single tick:
        case eSound::missileExplosion:
        case eSound::mineExplosion:
        case eSound::shipExplosion:
        case eSound::attractorExplosion:
            return 3;
        default:
            return 2;
    }
}


bool AudioDirector::_CoolingDown( const eSound _sound )
{
    auto & lastPlayed{ m_lastPlayed.at( static_cast< size_t >( _sound ) ) };
    if( lastPlayed != 0 && m_tick - lastPlayed < _CooldownTicks( _sound ) )
        return true;
    lastPlayed = m_tick;
    return false;
}


void AudioDirector::Play( const eSound _sound, const double _volume )
{
    if( _volume <= 0 || _CoolingDown( _sound ) )
        return;
    m_audio.Play( static_cast< size_t >( _sound ), { _volume, {}, {} } );
}


void AudioDirector::PlayAt( const eSound _sound, const Vector & _listenerRelativePosition )
{
    const auto position{ Locate( _listenerRelativePosition ) };
    if( position.volume <= 0 || _CoolingDown( _sound ) )
        return;
    m_audio.Play( static_cast< size_t >( _sound ), { position.volume, position.pan, {} } );
}


AudioDirector::Loop AudioDirector::CreateLoop( const eSound _sound, const Audio::Param & _param )
{
    return { m_audio.Play( static_cast< size_t >( _sound ), _param, true ) };
}


void AudioDirector::SetLoop( Loop & _loop, const Audio::Param & _param ) const
{
    const bool audible{ _param.volume && *_param.volume > 0 };
    if( _loop.paused && audible ) {
        m_audio.SetPaused( _loop.handle, false );
        _loop.paused = false;
    }
    if( _loop.paused )
        return;
    if( audible )
        m_audio.SetParam( _loop.handle, _param );
    else {
        m_audio.SetPaused( _loop.handle, true );
        _loop.paused = true;
    }
}


void AudioDirector::StopLoop( Loop & _loop )
{
    m_audio.Stop( _loop.handle );
    _loop.handle = {};
    _loop.paused = false;
}
