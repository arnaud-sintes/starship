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
        turret,         // temporary orbiting mini-turret
        repulsor,       // instant friendly shockwave
        decoy,          // beacon luring enemy missiles
        emp,            // temporary enemy engines/launchers shutdown
        overdrive,      // temporary free propellant + engine boost
        singularity,    // deployed micro black hole, pulls everything then collapses
        blossom,        // temporary rotating radial laser storm
        hellstorm,      // instant homing missiles spiral fan
    };
    eType type{ eType::laserUp };

    struct Dynamic
    {
        double radius{ 0 };
        double reflectAnimation{ 0 };
    };
    Dynamic dynamic;

    double grow{ 0 };

    void Update(); // grow animation drives the collision radius
    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const;
};
