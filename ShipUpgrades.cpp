#include "ShipUpgrades.h"


bool ShipUpgrades::AnyUpgradable() const
{
    for( size_t i{ 0 }; i < levels.size(); i++ )
        if( CanUpgrade( static_cast< eSystem >( i ) ) )
            return true;
    return false;
}


void ShipUpgrades::ApplyAll( Rocket & _ship ) const
{
    for( size_t i{ 0 }; i < levels.size(); i++ )
        for( int level{ 0 }; level < levels.at( i ); level++ )
            ApplyLevel( _ship, static_cast< eSystem >( i ) );
}


void ShipUpgrades::ApplyLevel( Rocket & _ship, const eSystem _system )
{
    // steps are ~15-20% of the base setup (see World::World); the mass penalty comes
    // for free from Rocket::Update (capacity * ( 1 - quality ) terms):
    switch( _system ) {
    case eSystem::shield: {
        constexpr double capacityStep{ 1.4 }; // +20%, thicker hull ring
        _ship.shield.capacity += capacityStep;
        _ship.shield.value += capacityStep;
        _ship.shield.repair_rate += 0.003;
        break;
    }
    case eSystem::propellant: {
        constexpr double capacityStep{ 3 }; // +15%, bigger tank
        _ship.propellant.capacity += capacityStep;
        _ship.propellant.value += capacityStep;
        _ship.propellant.production_rate += 0.003;
        _ship.propellant.quality += 0.01; // keeps the tank growth (and its mass) reasonable
        break;
    }
    case eSystem::engine:
        _ship.engine.power += 0.075; // +15% top thrust, wider nozzle, more propellant burnt
        _ship.engine.acceleration_rate += 0.001;
        break;
    case eSystem::rotator:
        _ship.rotator.power += 0.002; // +20% turn power, sharper aiming
        _ship.rotator.acceleration_rate += 0.0002;
        break;
    case eSystem::count:
        break;
    }
}
