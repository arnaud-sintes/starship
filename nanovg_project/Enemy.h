#pragma once

#include "Rocket.h"
#include "MiniAudio.h"


// --------------
struct Enemy
{
    Rocket rocket;
    int shotRate;
    MiniAudio::Sound sound_laserCollision;
    MiniAudio::Sound sound_collision;
    MiniAudio::Sound sound_explosion;
};
