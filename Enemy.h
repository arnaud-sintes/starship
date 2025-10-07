#pragma once

#include "Rocket.h"
#include "CuteSound.h"


// --------------
struct Enemy
{
    Rocket rocket;
    int shotRate;
    CuteSound::Instance & sound_shipMainEngine;
    CuteSound::Instance & sound_shipRotationEngine;

    ~Enemy()
    {
        sound_shipMainEngine.Stop();
        sound_shipRotationEngine.Stop();
    }
};
