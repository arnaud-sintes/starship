#pragma once

#include "Rocket.h"
#include "MiniAudio.h"


// --------------
struct Missile
{
    Rocket rocket;
    bool targetShip;
    const Rocket & origin;
    MiniAudio::Sound sound_shot;
    MiniAudio::Sound sound_run;
    MiniAudio::Sound sound_explosion;
    bool bypassCollision{ false };
    int lifeSpan{ 0 };
};
