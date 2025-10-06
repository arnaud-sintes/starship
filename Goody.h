#pragma once

#include "NanoVGRenderer.h"


// --------------
struct Goody
{
    Vector position;

    enum class eType {
        laserUp,        // increase laser power
        homingMissiles, // homing missiles pack
        magneticMines,  // magnetic mines pack
        plasmaShield,   // temporary plasma shield
        shieldAdd,      // shield addition
        propellantAdd,  // propellant addition
    };
    eType type;

    struct Dynamic
    {
        double radius{ 0 };
        double reflectAnimation{ 0 };
    };
    Dynamic dynamic;

    double grow{ 0 };

    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation );
};
