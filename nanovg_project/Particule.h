#pragma once

#include "Maths.h"


// --------------
struct Particule
{
    Vector position;
    const Vector momentum;
    double lifeSpan;
    const double width;
};
