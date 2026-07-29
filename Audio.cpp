#include "Audio.h"

#define NOMINMAX // miniaudio pulls windows.h, keep std::min/max usable
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_ENCODING
#include "external/miniaudio/miniaudio.h"


namespace
{
    // the game expresses pan in [0,1] with 0.5 centered, miniaudio expects [-1,+1]:
    float _Pan( const double _pan ) { return static_cast< float >( std::clamp( _pan, 0.0, 1.0 ) * 2 - 1 ); }
    // a zero pitch means "inaudible" game-side (always paired with volume 0), keep the resampler in a sane range:
    float _Pitch( const double _pitch ) { return static_cast< float >( std::max( _pitch, 0.05 ) ); }
}


Audio::~Audio()
{
    for( auto & instance : m_instances )
        if( instance.active )
            _Release( instance );
    for( auto * pSound : m_pinnedSounds ) {
        ::ma_sound_uninit( static_cast< ::ma_sound * >( pSound ) );
        delete static_cast< ::ma_sound * >( pSound );
    }
    if( m_engine != nullptr ) {
        ::ma_engine_uninit( static_cast< ::ma_engine * >( m_engine ) );
        delete static_cast< ::ma_engine * >( m_engine );
    }
}


bool Audio::Init()
{
    auto pEngine{ std::make_unique< ::ma_engine >() };
    if( ::ma_engine_init( nullptr, pEngine.get() ) != MA_SUCCESS )
        return false;
    m_engine = pEngine.release();
    return true;
}


bool Audio::Load( const size_t _id, const std::string & _filePath )
{
    if( m_engine == nullptr )
        return false;
    // decode once now and keep an idle instance around so the resource manager
    // never drops nor re-decodes the data:
    auto pSound{ std::make_unique< ::ma_sound >() };
    if( ::ma_sound_init_from_file( static_cast< ::ma_engine * >( m_engine ), _filePath.c_str(),
            MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, pSound.get() ) != MA_SUCCESS )
        return false;
    m_pinnedSounds.emplace_back( pSound.release() );
    m_sounds.emplace( _id, _filePath );
    return true;
}


void Audio::Update()
{
    // recycle finished one-shots:
    for( auto & instance : m_instances )
        if( instance.active && !instance.looped && ::ma_sound_at_end( static_cast< ::ma_sound * >( instance.pSound ) ) )
            _Release( instance );
}


Audio::Handle Audio::Play( const size_t _sound, const Param & _param, const bool _looped )
{
    const auto itSound{ m_sounds.find( _sound ) };
    if( m_engine == nullptr || itSound == m_sounds.end() )
        return {};

    // reuse a free slot or add one:
    size_t slot{ 0 };
    while( slot < m_instances.size() && m_instances.at( slot ).active )
        slot++;
    if( slot == m_instances.size() )
        m_instances.emplace_back();
    auto & instance{ m_instances.at( slot ) };

    auto pSound{ std::make_unique< ::ma_sound >() };
    if( ::ma_sound_init_from_file( static_cast< ::ma_engine * >( m_engine ), itSound->second.c_str(),
            MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, pSound.get() ) != MA_SUCCESS )
        return {};
    ::ma_sound_set_looping( pSound.get(), _looped );
    ::ma_sound_set_volume( pSound.get(), static_cast< float >( _param.volume ? *_param.volume : 1.0 ) );
    ::ma_sound_set_pan( pSound.get(), _Pan( _param.pan ? *_param.pan : 0.5 ) );
    ::ma_sound_set_pitch( pSound.get(), _Pitch( _param.pitch ? *_param.pitch : 1.0 ) );
    if( ::ma_sound_start( pSound.get() ) != MA_SUCCESS ) {
        ::ma_sound_uninit( pSound.get() );
        return {};
    }

    instance.pSound = pSound.release();
    instance.looped = _looped;
    instance.active = true;
    return { ( static_cast< unsigned long long >( slot + 1 ) << 32 ) | instance.generation };
}


Audio::Instance * Audio::_Resolve( const Handle _handle ) const
{
    if( _handle.id == 0 )
        return nullptr;
    const auto slot{ static_cast< size_t >( _handle.id >> 32 ) - 1 };
    const auto generation{ static_cast< unsigned int >( _handle.id & 0xffffffff ) };
    if( slot >= m_instances.size() )
        return nullptr;
    auto & instance{ m_instances.at( slot ) };
    if( !instance.active || instance.generation != generation )
        return nullptr;
    return &instance;
}


void Audio::_Release( Instance & _instance )
{
    ::ma_sound_uninit( static_cast< ::ma_sound * >( _instance.pSound ) );
    delete static_cast< ::ma_sound * >( _instance.pSound );
    _instance.pSound = nullptr;
    _instance.active = false;
    _instance.generation++; // stale handles now resolve to nothing
}


void Audio::SetParam( const Handle _handle, const Param & _param ) const
{
    auto * pInstance{ _Resolve( _handle ) };
    if( pInstance == nullptr )
        return;
    auto * pSound{ static_cast< ::ma_sound * >( pInstance->pSound ) };
    if( _param.volume )
        ::ma_sound_set_volume( pSound, static_cast< float >( *_param.volume ) );
    if( _param.pan )
        ::ma_sound_set_pan( pSound, _Pan( *_param.pan ) );
    if( _param.pitch )
        ::ma_sound_set_pitch( pSound, _Pitch( *_param.pitch ) );
}


void Audio::SetPaused( const Handle _handle, const bool _paused ) const
{
    auto * pInstance{ _Resolve( _handle ) };
    if( pInstance == nullptr )
        return;
    auto * pSound{ static_cast< ::ma_sound * >( pInstance->pSound ) };
    if( _paused )
        ::ma_sound_stop( pSound ); // pauses, keeps the cursor
    else
        ::ma_sound_start( pSound );
}


void Audio::Stop( const Handle _handle )
{
    auto * pInstance{ _Resolve( _handle ) };
    if( pInstance == nullptr )
        return;
    _Release( *pInstance );
}
