#pragma once

#include "NanoVGRenderer.h"


// --------------
struct Laser
{
    Vector position;
    Vector momentum;
    double damage{ 0 };
    int lifeSpan{ 0 };
    static constexpr int maxLifeSpan{ 20 };

    struct Dynamic
    {
        Vector positionA;
        Vector positionB;
    };
    Dynamic dynamic;

    void Refresh(); // recompute the collision segment, call after any position change
    void Update();
    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const;
};
