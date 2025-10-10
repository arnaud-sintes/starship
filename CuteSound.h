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
    bool Init( const Win32::Windows & _windows );
    bool Load( const std::unordered_map< size_t, std::string > & _sounds );

    struct Param
    {
        std::optional< double > volume;
        std::optional< double > pan;
        std::optional< double > pitch;
    };
    
    class Instance;
    Instance & Play( const size_t _sound, const Param & _param = {}, const bool _looped = false );

private:
    void _Queue( std::function< void() > && _fn, const bool _lock = true );
    void _UpdateLoop();

private:
    bool m_stopPlaybackThread{ false };
    std::unique_ptr< std::jthread > m_playbackThread;
    class Sound;
    std::unordered_map< size_t, std::unique_ptr< Sound > > m_sounds;
    std::mutex m_mtx;
    std::list< std::unique_ptr< Instance > > m_instances;
    std::list< std::function< void() > > m_queue;
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
    friend CuteSound;

public:
    Instance( CuteSound & _cuteSound );
    ~Instance() = default;

private:
    void UnqueuedPlay( const Sound & _sound, const Param _param = {}, const bool _looped = false );
    void UnqueuedPause( const bool _paused );
    bool UnqueuedTerminated() const;
    void UnqueuedSetParam( const Param _param );
    void UnqueuedStop();

private:
    void Play( const Sound & _sound, const Param & _param = {}, const bool _looped = false );

public:
    void Pause( const bool _paused = true );
    bool Paused() const { return m_paused; }
    void SetParam( const Param & _param );
    void Stop();

private:
    CuteSound & m_cuteSound;
    unsigned long long m_instance{ 0 };
    bool m_played{ false };
    bool m_paused{ false };
};