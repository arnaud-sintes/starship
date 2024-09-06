#pragma once

#include "../core/std.h"


// ----------------
class MiniAudio
{
public:
    MiniAudio();
    ~MiniAudio();

public:
    class Sound
    {
    public:
        Sound() = default;
        Sound( void * _pEngine, const std::string & _path );
        Sound( const Sound & _sound, void * ); // duplication ctor
        Sound( const Sound & _other );
        ~Sound();

    public:
        void Setup( const double _volume = 1, const double _pan = 0, const double _pitch = 0, const bool _loop = false ) const;
        void Play() const;
        void Stop() const;

    private:
        void * m_pEngine;
        void * m_pSound{ nullptr };
    };

    Sound LoadSound( const std::string & _path );

private:
    void * m_pEngine;
};
