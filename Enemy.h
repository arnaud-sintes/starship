#pragma once

#include "Rocket.h"
#include "AudioDirector.h"


// --------------
struct Enemy
{
    Rocket rocket;
    int shotRate{ 0 };
    AudioDirector::Loop sound_mainEngine;
    AudioDirector::Loop sound_rotationEngine;
    bool dead{ false }; // marked during collision passes, compacted at end of update
};
