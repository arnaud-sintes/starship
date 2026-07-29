#pragma once

#include "core/std.h"


// ----------------
// Thin wrapper over miniaudio's engine.
// Mixing runs on the OS audio callback (WASAPI) with proper fractional resampling,
// so per-frame volume/pan/pitch modulation stays artifact-free; every method here is
// meant for the game thread.
class Audio
{
public:
    Audio() = default;
    ~Audio();
    Audio( const Audio & ) = delete;
    Audio & operator =( const Audio & ) = delete;

public:
    bool Init();
    bool Load( const size_t _id, const std::string & _filePath );
    void Update(); // once per game tick: recycles finished one-shot instances

    struct Param
    {
        std::optional< double > volume; // [0,1]
        std::optional< double > pan;    // [0,1], 0.5 centered
        std::optional< double > pitch;
    };

    // handle to a playing instance; operations on terminated instances are no-ops,
    // so handles can safely outlive the sounds they refer to
    struct Handle
    {
        unsigned long long id{ 0 }; // slot+1 in the high 32 bits, generation in the low 32 bits
    };

    Handle Play( const size_t _sound, const Param & _param = {}, const bool _looped = false );
    void SetParam( const Handle _handle, const Param & _param ) const;
    void SetPaused( const Handle _handle, const bool _paused ) const;
    void Stop( const Handle _handle );

private:
    struct Instance
    {
        void * pSound{ nullptr }; // ma_sound
        unsigned int generation{ 0 };
        bool looped{ false };
        bool active{ false };
    };
    Instance * _Resolve( const Handle _handle ) const;
    void _Release( Instance & _instance );

private:
    void * m_engine{ nullptr }; // ma_engine
    std::unordered_map< size_t, std::string > m_sounds; // id -> file path (decoded data is cached by the engine's resource manager)
    std::vector< void * > m_pinnedSounds; // one idle ma_sound per source, keeps decoded data cached
    mutable std::vector< Instance > m_instances; // slots, reused
};
