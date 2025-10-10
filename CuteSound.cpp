#include "CuteSound.h"

#include "core/Timer.h"

#define CUTE_SOUND_IMPLEMENTATION
#include "external/cute_sound/cute_sound.h"


CuteSound::~CuteSound()
{
    m_stopPlaybackThread = true;
    m_playbackThread.reset();
    m_instances.clear();
    m_sounds.clear();
    ::cs_shutdown();
}

bool CuteSound::Init( const Win32::Windows & _windows )
{
    if( ::cs_init( _windows.GetWindowHandle().As< HWND >(), 44100, 1024, nullptr ) != CUTE_SOUND_ERROR_NONE )
        return false;
    m_playbackThread = std::make_unique< std::jthread >( [ this ]{ _UpdateLoop(); } );
    return true;
}

void CuteSound::_Queue( std::function< void() > && _fn, const bool _lock )
{
    std::unique_ptr< std::lock_guard< std::mutex > > lock;
    if( _lock )
        lock.reset( new std::lock_guard{ m_mtx } );
    m_queue.emplace_back( std::move( _fn ) );
}

void CuteSound::_UpdateLoop()
{
    //Win32::SetThreadRealtimePriority();
    while( !m_stopPlaybackThread ) {
        std::list< std::function< void() > > queue;
        {
            std::lock_guard lock{ m_mtx };
            std::erase_if( m_instances, []( const auto & _pInstance ){ return _pInstance->UnqueuedTerminated(); } );
            queue = std::move( m_queue );
        }
        for( auto && fn : queue )
            fn();
        ::cs_update( 0 );
    }
}

bool CuteSound::Load( const std::unordered_map< size_t, std::string > & _sounds )
{
    for( const auto & pair : _sounds ) {
        auto sound{ std::make_unique< Sound >() };
        if( !sound->Load( pair.second ) )
            return false;
        m_sounds.emplace( pair.first, std::move( sound ) );
    }
    return true;
}

CuteSound::Instance & CuteSound::Play( const size_t _sound, const Param & _param, const bool _looped )
{
    std::lock_guard lock{ m_mtx };
    auto & instance{ m_instances.emplace_back( std::make_unique< Instance >( *this ) ) };
    instance->Play( *m_sounds.find( _sound )->second, _param, _looped );
    return *instance;
}

CuteSound::Sound::~Sound()
{
    auto * pAudioSource{ static_cast< ::cs_audio_source_t * >( m_audioSource ) };
    ::cs_free_audio_source( pAudioSource );
}

bool CuteSound::Sound::Load( const std::string & _filePath )
{
    m_audioSource = ::cs_load_wav( _filePath.c_str(), nullptr );
    return m_audioSource != nullptr;
}

CuteSound::Instance::Instance( CuteSound & _cuteSound )
    : m_cuteSound{ _cuteSound }
{}

void CuteSound::Instance::UnqueuedPlay( const Sound & _sound, const Param _param, const bool _looped )
{
    auto * pAudioSource{ static_cast< ::cs_audio_source_t * >( _sound.m_audioSource ) };
    m_instance = ::cs_play_sound( pAudioSource, ::cs_sound_params_t{
        false, _looped,
        _param.volume ? static_cast< float >( *_param.volume ) : float{ 1 },
        _param.pan ? static_cast< float >( *_param.pan ) : float{ 0.5 },
        _param.pitch ? static_cast< float >( *_param.pitch ) : float{ 1 },
        0
    } ).id;
    m_played = true;
}

void CuteSound::Instance::UnqueuedPause( const bool _paused )
{
    ::cs_sound_set_is_paused( ::cs_playing_sound_t{ m_instance }, _paused );
}

bool CuteSound::Instance::UnqueuedTerminated() const
{
    return m_played && !::cs_sound_is_active( ::cs_playing_sound_t{ m_instance } );
}

void CuteSound::Instance::UnqueuedSetParam( const Param _param )
{
    auto instance{ ::cs_playing_sound_t{ m_instance } };
    if( _param.volume )
        ::cs_sound_set_volume( instance, static_cast< float >( *_param.volume ) );
    if( _param.pan )
        ::cs_sound_set_pan( instance, static_cast< float >( *_param.pan ) );
    if( _param.pitch )
        ::cs_sound_set_pitch( instance, static_cast< float >( *_param.pitch ) );
}

void CuteSound::Instance::UnqueuedStop()
{
    ::cs_sound_stop( ::cs_playing_sound_t{ m_instance } );
}

void CuteSound::Instance::Play( const Sound & _sound, const Param & _param, const bool _looped )
{
    m_cuteSound._Queue( [ this, &_sound, _param, _looped ]{ UnqueuedPlay( _sound, _param, _looped ); }, false );
}

void CuteSound::Instance::Pause( const bool _paused )
{
    m_paused = _paused;
    m_cuteSound._Queue( [ this, _paused ]{ UnqueuedPause( _paused ); } );
}

void CuteSound::Instance::SetParam( const Param & _param )
{
    m_cuteSound._Queue( [ this, _param ]{ UnqueuedSetParam( _param ); } );
}

void CuteSound::Instance::Stop()
{
    m_cuteSound._Queue( [ this ]{ UnqueuedStop(); } );
}