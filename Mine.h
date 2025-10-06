#pragma once

#include "NanoVGRenderer.h"
#include "MiniAudio.h"


// --------------
struct Mine
{
    Vector position;
    const double damage;
    MiniAudio::Sound sound_explosion;

    struct Dynamic
    {
        double radius{ 0 };
    };
    Dynamic dynamic;

    double grow{ 0 };
    bool alive{ true };

    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation );
};
