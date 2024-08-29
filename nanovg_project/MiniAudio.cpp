#include "MiniAudio.h"

#define MINIAUDIO_IMPLEMENTATION
#include "external/miniaudio/miniaudio.h"


MiniAudio::MiniAudio()
    : m_pEngine{ new ::ma_engine{} }
{
    auto pEngine{ static_cast< ::ma_engine * >( m_pEngine ) };
    ::ma_engine_init( nullptr, pEngine );
    ::ma_engine_start( pEngine );
}


MiniAudio::~MiniAudio()
{
    auto pEngine{ static_cast< ::ma_engine * >( m_pEngine ) };
    ::ma_engine_stop( pEngine );
    ::ma_engine_uninit( pEngine );
    delete pEngine;
}


MiniAudio::Sound MiniAudio::LoadSound( const std::string & _path )
{
    return { m_pEngine, _path };
}


// ----------------

MiniAudio::Sound::Sound( void * _pEngine, const std::string & _path )
    : m_pEngine{ _pEngine }
    , m_pSound{ new ::ma_sound{} }
{
    auto pEngine{ static_cast< ::ma_engine * >( m_pEngine ) };
    auto pSound{ static_cast< ::ma_sound * >( m_pSound ) };
    ::ma_sound_init_from_file( pEngine, _path.c_str(), 0, nullptr, nullptr, pSound );
}


MiniAudio::Sound::Sound( const Sound & _sound, void * )
    : m_pEngine{ _sound.m_pEngine }
    , m_pSound{ new ::ma_sound{} }
{
    auto pEngine{ static_cast< ::ma_engine * >( m_pEngine ) };
    auto pSourceSound{ static_cast< ::ma_sound * >( _sound.m_pSound ) };
    auto pSound{ static_cast< ::ma_sound * >( m_pSound ) };
    ::ma_sound_init_copy( pEngine, pSourceSound, 0, nullptr, pSound );
}


MiniAudio::Sound::Sound( const Sound & _other )
    : m_pEngine{ _other.m_pEngine }
    , m_pSound{ _other.m_pSound }
{
    const_cast< Sound & >( _other ).m_pSound = nullptr;
}


MiniAudio::Sound::~Sound()
{
    auto pSound{ static_cast< ::ma_sound * >( m_pSound ) };
    if( pSound == nullptr )
        return;
    Stop();
    ::ma_sound_uninit( pSound );
    delete pSound;
}


void MiniAudio::Sound::Setup( const double _volume, const double _pan, const double _pitch, const bool _loop ) const
{
    auto pSound{ static_cast< ::ma_sound * >( m_pSound ) };
    ::ma_sound_set_volume( pSound, static_cast< float >( _volume ) );
    ::ma_sound_set_pan( pSound, static_cast< float >( _pan ) );
    ::ma_sound_set_pitch( pSound, static_cast< float >( _pitch ) );
    ::ma_sound_set_looping( pSound, _loop );
}


void MiniAudio::Sound::Play() const
{
    auto pSound{ static_cast< ::ma_sound * >( m_pSound ) };
    ::ma_sound_seek_to_pcm_frame( pSound, 0 );
    ::ma_sound_start( pSound );
}


void MiniAudio::Sound::Stop() const
{
    auto pSound{ static_cast< ::ma_sound * >( m_pSound ) };
    ::ma_sound_stop( pSound );
}