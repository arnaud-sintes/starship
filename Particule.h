#pragma once

#include "core/Maths.h"


// --------------
struct Particule
{
    Vector position;
    Vector momentum;
    double lifeSpan{ 0 };
    double width{ 0 };
    eFadeColor fadeColor{ eFadeColor::orange };

    Color_d GetColor() const { return Color< double >::FadeColor( fadeColor, ( 3 - lifeSpan ) / 3 ); }
};
