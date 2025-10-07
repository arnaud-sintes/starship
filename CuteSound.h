#pragma once

#include "core/std.h"
#include "core/win32.h"


// ----------------
class CuteSound
{
public:
    CuteSound() = default;
    ~CuteSound();

public:
    bool Init( const Win32::Windows & _windows, const bool _playbackThread = true );
    void Update( const double _frameDurationSec ) const;
    bool Load( const std::unordered_map< size_t, std::string > & _sounds );

    struct Param
    {
        std::optional< double > volume;
        std::optional< double > pan;
        std::optional< double > pitch;
    };
    
    class Instance;
    using InstanceWeakPtr = std::optional< std::reference_wrapper< Instance > >;
    InstanceWeakPtr Play( const size_t _sound, const Param & _param = {}, const bool _looped = false );

private:
    bool m_stopPlaybackThread{ false };
    std::unique_ptr< std::jthread > m_playbackThread;
    class Sound;
    std::unordered_map< size_t, std::unique_ptr< Sound > > m_sounds;
    std::list< std::unique_ptr< Instance > > m_instances;
};


// ----------------
class CuteSound::Sound
{
    friend Instance;

public:
    Sound() = default;
    ~Sound();

public:
    bool Load( const std::string & _filePath );

private:
    void * m_audioSource{ nullptr };
};


// ----------------
class CuteSound::Instance
{
public:
    Instance();
    ~Instance();

public:
    void Play( const Sound & _sound, const Param & _param = {}, const bool _looped = false );
    void SetParam( const Param & _param );
    void Stop();
    bool Active() const;

private:
    void * m_instance{ nullptr };
};