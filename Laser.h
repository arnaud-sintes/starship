#pragma once

#include "NanoVGRenderer.h"


// --------------
struct Laser
{
    Vector position;
    const Vector momentum;
    const double damage;
    int lifeSpan{ 0 };
    const int maxLifeSpan{ 20 };

    struct Dynamic
    {
        Vector positionA;
        Vector positionB;
    };
    Dynamic dynamic;

    void Update();
    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation );
};
