#pragma once

#include "Rocket.h"


// --------------
// Ship customization: every cleared mission earns a skill point, spent one level at a
// time on a ship subsystem. A level grows the subsystem - and its mass, hence the
// drag - so the hull shape follows the build (thicker ring, bigger tank, wider
// nozzle). Plain data + rules, simulation-only.
struct ShipUpgrades
{
    enum class eSystem : size_t
    {
        shield,
        propellant,
        engine,
        rotator,
        count
    };
    static constexpr int maxLevel{ 5 };

    std::array< int, static_cast< size_t >( eSystem::count ) > levels{};
    int points{ 0 }; // earned, not spent yet

    int Level( const eSystem _system ) const { return levels.at( static_cast< size_t >( _system ) ); }
    bool CanUpgrade( const eSystem _system ) const { return points > 0 && Level( _system ) < maxLevel; }
    bool AnyUpgradable() const;
    void ApplyAll( Rocket & _ship ) const; // every level, on a ship fresh from its base setup
    static void ApplyLevel( Rocket & _ship, const eSystem _system ); // one level more (stored values grow with the capacities)
};
