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

    ~Missile()
    {
        sound_run.Stop();
    }
};
