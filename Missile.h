#pragma once

#include "Rocket.h"
#include "CuteSound.h"

// --------------
struct Missile
{
    Rocket rocket;
    bool targetShip;
    const Rocket & origin;
    CuteSound::Instance & sound_run;
    bool bypassCollision{ false };
    int lifeSpan{ 0 };
    bool sound_runActive{ true };

    void StopSound()
    {
        if( !sound_runActive )
            return;
        sound_run.Stop();
        sound_runActive = false;
    }

    ~Missile()
    {
        StopSound();
    }
};
