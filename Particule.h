#pragma once

#include "Vector.h"


// --------------
struct Particule
{
    Vector position;
    const Vector momentum;
    double lifeSpan;
    const double width;
};
