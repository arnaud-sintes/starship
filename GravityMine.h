#pragma once

#include "NanoVGRenderer.h"


// --------------
// Enemy gravity mine: roams the void and pulls everything nearby - ship, enemies,
// missiles, even the player's own mines and the dust - toward itself, detonating on
// contact. Visually distinct from the player's green magnetic mines: a dark core,
// pulsing red ring, rotating spikes and a faint halo telegraphing the pull field.
struct GravityMine
{
    Vector position;
    double spin{ 0 }; // visual spike rotation, random phase at spawn

    static constexpr double pullRange{ 320 };
    static constexpr double pullStrength{ 0.06 }; // force at the mine, quadratic falloff to pullRange

    struct Dynamic
    {
        double radius{ 13 }; // contact/collision radius
    };
    Dynamic dynamic;

    double grow{ 0 };
    bool alive{ true };

    void Update(); // pulse/spin animation drives the collision radius
    Vector Attraction( const Vector & _attracted ) const; // pull force on an object at _attracted
    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const;
};
