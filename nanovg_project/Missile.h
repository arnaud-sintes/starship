#pragma once

#include "Rocket.h"


// --------------
struct Missile
{
    Rocket rocket;
    bool targetShip;
    const Rocket & origin;
    bool bypassCollision{ false };
    int lifeSpan{ 0 };
};
