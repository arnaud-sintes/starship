#pragma once

#include "Rocket.h"
#include "AudioDirector.h"


// --------------
struct Enemy
{
    enum class eType
    {
        chaser, // hunts the ship, fires homing missiles
        wasp,   // small, fast, fragile, rams the ship
        sniper, // holds far away, fires lead-compensated slugs
    };

    Rocket rocket;
    eType type{ eType::chaser };
    int shotRate{ 0 }; // generic action cadence (missiles, mines, slugs)
    AudioDirector::Loop sound_mainEngine;
    AudioDirector::Loop sound_rotationEngine;
    bool dead{ false }; // marked during collision passes, compacted at end of update
};
