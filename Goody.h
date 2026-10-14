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

    // unlock schedule: the first mission (stage, mission) where each goody may drop,
    // from the humblest to the fanciest - the later stages are forward-looking
    struct MissionId
    {
        int stage;
        int mission;
    };
    static constexpr MissionId FirstMission( const eType _type )
    {
        switch( _type ) {
            case eType::laserUp:
            case eType::shieldAdd:
            case eType::propellantAdd:
                return { 1, 1 }; // the essentials
            case eType::homingMissiles:
                return { 1, 2 }; // first secondary weapon
            case eType::plasmaShield:
                return { 1, 3 }; // first defensive trick
            case eType::magneticMines:
            case eType::repulsor: // (chain-triggers the gravity mines met in 1-4)
                return { 1, 4 };
            case eType::turret:
            case eType::decoy:
                return { 1, 5 };
            case eType::emp:
            case eType::overdrive:
                return { 2, 1 };
            case eType::hellstorm:
                return { 2, 2 };
            case eType::blossom:
                return { 2, 3 };
            case eType::singularity:
            default:
                return { 2, 4 }; // the most spectacular
        }
    }

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
