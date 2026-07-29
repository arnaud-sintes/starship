#pragma once

#include "NanoVGRenderer.h"

// --------------
struct Mine
{
    Vector position;
    double damage{ 0 };

    struct Dynamic
    {
        double radius{ 0 };
    };
    Dynamic dynamic;

    double grow{ 0 };
    bool alive{ true };

    void Update(); // grow animation drives the collision radius
    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const;
};
