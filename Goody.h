#pragma once

#include "NanoVGRenderer.h"


// --------------
struct Goody
{
    Vector position;

    enum class eType {
        laserUp,    // increase laser power
    };
    eType type;

    struct Dynamic
    {
        double radius{ 0 };
    };
    Dynamic dynamic;

    double grow{ 0 };

    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation );
};
