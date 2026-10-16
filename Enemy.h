#pragma once

#include <array>

#include "Rocket.h"
#include "AudioDirector.h"


// --------------
struct Enemy
{
    enum class eType
    {
        chaser, // hunts the ship, fires homing missiles
        wasp,   // small, fast, fragile, rams the ship
        sniper, // holds far away, fires lead-compensated slugs
        foreman, // stage 1 boss: a guild mining barge, weak spot on its rear core
        warden,  // stage 2 boss: a sentinel of the belt, its core shielded by orbiting pylons
    };

    Rocket rocket;
    eType type{ eType::chaser };
    int shotRate{ 0 }; // generic action cadence (missiles, mines, slugs)
    AudioDirector::Loop sound_mainEngine;
    AudioDirector::Loop sound_rotationEngine;
    bool dead{ false }; // marked during collision passes, compacted at end of update

    // boss state (foreman only):
    struct Boss
    {
        double maxShield{ 0 };
        double lastShield{ 0 };  // shield at the previous tick: what is not a laser hit is reduced
        double laserDamage{ 0 }; // laser damage taken since the previous tick
        int phase{ 0 };          // 0, 1, 2: grows as the shield drops
        int coreFlash{ 0 };      // ticks left of the "rear core was hit" flash
        int slugCadence{ 0 }, missileCadence{ 0 }, waspCadence{ 0 }, mineCadence{ 0 };
        int cycle{ 0 };          // enraged phase: spiral / fans alternation
        int spiralTicks{ 0 };
        double spiral{ 0 };
        double orbit{ 1 };       // orbiting direction around the ship
        // warden: three pylons orbit the core, each with its own hull; the core is untouchable until they are all down
        std::array< double, 3 > pylonShield{};
        std::array< int, 3 > pylonFlash{};  // ticks left of the "pylon was hit" flash
        double pylonMax{ 0 };
        double pylonAngle{ 0 };
        double orbitTilt{ 0.8 }, orbitAzimuth{ 0 }; // the orbital plane of the pylons, seen at an angle: a wobbling ellipse
        int vent{ 0 };           // rear core: 0 armored, 1 warning (glowing), 2 vented (exposed)
        int ventTimer{ 0 };      // ticks left in the current vent state
        int ventCadence{ 0 };    // rear slug ring cadence while vented
    };
    Boss boss;

    // the weak spot of the foreman, a glowing core behind its stern (the heart of its main engine):
    static constexpr double coreRadius{ 34 };
    bool CoreOpen() const { return boss.vent == 2; }
    bool IsBoss() const { return type == eType::foreman || type == eType::warden; }

    // the pylons of the warden, orbiting its core:
    static constexpr int pylonCount{ 3 };
    static constexpr double pylonOrbit{ 250 }, pylonRadius{ 34 };
    // the orbit of the pylons, a tilted and precessing circle around the core (z is height: the pylons pass in front of and behind it)
    Vector3 OrbitPoint( const double _angle ) const { return Vector3{ std::cos( _angle ), std::sin( _angle ), 0 }.RotatedX( boss.orbitTilt ).RotatedZ( boss.orbitAzimuth ) * pylonOrbit; }
    Vector3 PylonSpace( const int _index ) const { return OrbitPoint( boss.pylonAngle + Maths::Pi2 * _index / pylonCount ); }
    Vector3 PylonOrbitNormal() const { return Vector3{ 0, 0, 1 }.RotatedX( boss.orbitTilt ).RotatedZ( boss.orbitAzimuth ); }
    Vector PylonPosition( const int _index ) const // what the pylon looks like on screen is where it is hit and where it pulls from
    {
        return rocket.position + PylonSpace( _index ).Oblique();
    }
    bool PylonAlive( const int _index ) const { return boss.pylonShield.at( static_cast< size_t >( _index ) ) > 0; }
    int PylonsAlive() const
    {
        int alive{ 0 };
        for( int i{ 0 }; i < pylonCount; i++ )
            alive += PylonAlive( i ) ? 1 : 0;
        return alive;
    }

    // boss health bar: the warden shows its pylons first, then its core
    double BossRatio() const
    {
        if( type == eType::warden && PylonsAlive() > 0 ) {
            double sum{ 0 };
            for( const auto shield : boss.pylonShield )
                sum += std::max( shield, 0.0 );
            return sum / ( pylonCount * boss.pylonMax );
        }
        return rocket.shield.value / boss.maxShield;
    }
    Vector CorePosition() const { return rocket.position + Vector::From( rocket.orientation, rocket.dynamic.boundingBoxRadius + 6 ); }
};
