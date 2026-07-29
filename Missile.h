#pragma once

#include "Rocket.h"
#include "AudioDirector.h"

// --------------
struct Missile
{
    Rocket rocket;
    bool targetShip{ false };
    unsigned long long originId{ 0 }; // rocket id of the launcher (safe cross-reference)
    bool fromShip{ false };
    AudioDirector::Loop sound_run;
    bool bypassCollision{ false };
    int lifeSpan{ 0 };
    bool dead{ false }; // marked during collision passes, compacted at end of update
};
