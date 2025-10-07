#pragma once

#include "NanoVGRenderer.h"

// --------------
struct Mine
{
    Vector position;
    const double damage;

    struct Dynamic
    {
        double radius{ 0 };
    };
    Dynamic dynamic;

    double grow{ 0 };
    bool alive{ true };

    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation );
};
