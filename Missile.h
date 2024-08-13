#pragma once

#include "Rocket.h"


// --------------
// TODO class
struct Missile
{
    Rocket rocket;
    bool targetShip;
    Rocket * pOrigin;
    bool bypassCollision{ false };
    int lifeSpan{ 0 };
};
