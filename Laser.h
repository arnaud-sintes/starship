#pragma once

#include "Graphics.h"


// --------------
// TODO class
struct Laser
{
    Vector position;
    Vector momentum;
    double damage;
    int lifeSpan{ 0 };
    int maxLifeSpan{ 20 };

    struct Dynamic
    {
        Vector positionA;
        Vector positionB;
    };
    Dynamic dynamic;

    void Update();
    void Draw( cairo_t & _cairo, const Vector & _translation );
};
