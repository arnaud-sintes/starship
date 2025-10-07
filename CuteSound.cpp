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

bool CuteSound::Init( const Win32::Windows & _windows, const bool _playbackThread )
{
    if( ::cs_init( _windows.GetWindowHandle().As< HWND >(), 44100, 1024, nullptr ) != CUTE_SOUND_ERROR_NONE )
        return false;
    if( _playbackThread )
        m_playbackThread = std::make_unique< std::jthread >( [ this ]{
            auto & timer{ Timer::GetInstance() };
            auto last{ timer.Get() };
            while( !m_stopPlaybackThread ) {
                const auto current{ timer.Get() };
                const auto lapse{ current - last };
                last = current;
                Update( static_cast< double >( lapse ) / double{ 1'000'000'000 } );
                std::this_thread::sleep_for( std::chrono::microseconds{ 16666 } );
            }
        } );
    // TODO sound seems to impact main thread performance
    //::cs_spawn_mix_thread();
    return true;
}

void CuteSound::Update( const double _frameDurationSec ) const
{
    ::cs_update( static_cast< float >( _frameDurationSec ) );
}

bool CuteSound::Load( const std::unordered_map< size_t, std::string > & _sounds )
{
    m_sounds.clear();
    for( const auto & pair : _sounds ) {
        auto sound{ std::make_unique< Sound >() };
        if( !sound->Load( pair.second ) )
            return false;
        m_sounds.emplace( pair.first, std::move( sound ) );
    }
    return true;
}

CuteSound::InstanceWeakPtr CuteSound::Play( const size_t _sound, const Param & _param, const bool _looped )
{
    std::erase_if( m_instances, []( const auto & _instance ){ return !_instance->Active(); } );
    const auto itSound{ m_sounds.find( _sound ) };
    if( itSound == m_sounds.cend() )
        return {};
    auto & instance{ m_instances.emplace_back( std::make_unique< Instance >() ) };
    instance->Play( *itSound->second, _param, _looped );
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

CuteSound::Instance::Instance()
    : m_instance{ new cs_playing_sound_t{} }
{}

CuteSound::Instance::~Instance()
{
    auto * pInstance{ static_cast< ::cs_playing_sound_t * >( m_instance ) };
    Stop();
    delete pInstance;
}

void CuteSound::Instance::Play( const Sound & _sound, const Param & _param, const bool _looped )
{
    auto * pAudioSource{ static_cast< ::cs_audio_source_t * >( _sound.m_audioSource ) };
    auto * pInstance{ static_cast< ::cs_playing_sound_t * >( m_instance ) };
    *pInstance = ::cs_play_sound( pAudioSource, ::cs_sound_params_t{
        false, _looped,
        _param.volume ? static_cast< float >( *_param.volume ) : float{ 1 },
        _param.pan ? static_cast< float >( *_param.pan ) : float{ 0.5 },
        _param.pitch ? static_cast< float >( *_param.pitch ) : float{ 1 },
        0
    } );
}

void CuteSound::Instance::SetParam( const Param & _param )
{
    auto * pInstance{ static_cast< ::cs_playing_sound_t * >( m_instance ) };
    if( _param.volume )
        ::cs_sound_set_volume( *pInstance, static_cast< float >( *_param.volume ) );
    if( _param.pan )
        ::cs_sound_set_pan( *pInstance, static_cast< float >( *_param.pan ) );
    if( _param.pitch )
        ::cs_sound_set_pitch( *pInstance, static_cast< float >( *_param.pitch ) );
}

void CuteSound::Instance::Stop()
{
    auto * pInstance{ static_cast< ::cs_playing_sound_t * >( m_instance ) };
    ::cs_sound_stop( *pInstance );
}

bool CuteSound::Instance::Active() const
{
    auto * pInstance{ static_cast< ::cs_playing_sound_t * >( m_instance ) };
    return ::cs_sound_is_active( *pInstance );
}