#pragma once

#include "core/Maths.h"


// --------------
struct Particule
{
    Vector position;
    const Vector momentum;
    double lifeSpan;
    const double width;
    const eFadeColor fadeColor = eFadeColor::orange;
    Color_d GetColor() { return Color< double >::FadeColor( fadeColor, ( 3 - lifeSpan ) / 3 ); }
};
