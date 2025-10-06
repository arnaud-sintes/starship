# Starship beta 0.0.6

## Goal

None currently, just enjoy the game engine and destroy everything for maximum fun!

<iframe style='width:100%;height:480px' src="https://www.youtube.com/embed/ECsGg_MohWk" allowfullscreen></iframe>

## Controls

*Mouse* only:

- ***aim*** a direction (will automatically deal with left and right engines to reach the targeted direction as fast as physically possible)

- ***left button*** to *shoot* (both *main* laser weapon or temporary *secondary* weapon)
- ***right button*** to activate the *main thrust engine*

## Goodies

When destroying an enemy, goodies may appears (50% chance):

- **laser power up** (*red* "**L**") that will increase the fire power of the main laser weapon: it will increase first the laser **speed**, from *slow* to *medium* and from *medium* to *fast*, then will increase the laser **pass count** (while resetting the speed to *slow*), from *one pass* to *two passes*, then *four passes*, *six passes* and finally to a maximum of *height passes*
- **homing missiles pack** (*green* "**H**") that will add **20 missiles** reaching automatically the closest enemy
- **magnetic mines pack** (*green* "**M**") that will add **10 magnetic** mines, attracted by the closest enemies
- **plasma shield** (*green* "**S**") that will activate a **shield** during **5 seconds** (dynamic radius from *30* to *100* units)
- **shield addition** (*blue* "**S**") that will boost the shield up to 50% of its current capacity
- **propellant addition** (*blue* "**P**") that will boost the propellant value up to 50% of its current capacity

> [!NOTE]
>
> Goodies are *attracted* by the ship (the closest, the fastest) to ease their acquisition

## Notes

- Games start with a *10 seconds* plasma shield activated, so you can safely run away
- All engines (left and right / upper and lower, and main engine) will consume propellant
- Propellant tank of all rockets got a *capacity*, a natural *production rate* (refill when no thrust) and a *quality factor* that will reduce the overall *ship form factor* (therefore the ship *mass*) regarding its capacity
- Shield of all rockets got a *capacity*, a natural *repair rate* and a *quality factor* that will reduce the *shield form factor* (therefore the ship *mass*) regarding its capacity
- Engines of all rockets (both *main engine* and lateral *rotator engines*) got a *power*, an *acceleration* and *deceleration rates* and a *quality factor* that will reduce the *engine form factor* (therefore the ship *mass*) regarding its power
- The cumulated *form factor* (so called "*mass*") influences the *drag force* power that will directly influence the *thrust motion*, the ship's *momentum* and the *attraction effects*
- Rotation occurs using a proper rotation momentum, created thanks to the four lateral engines, as it's hard to manually manage, a change of mouse position will automatically deal with these engine to properly rotate the ship as expected (front of the ship pointing in the mouse cursor current position)
- A slight *solar density* factor will slowly reduce the thrust motion
- *Strange attractors* with variable masses attracts all rockets at proximity, can be used as natural shield with some training
- A shield alert will occur when being under 25% of the shield capacity
- A low propellant alert will occur when being under 25% of the propellant tank capacity
- Collision engine takes properly in account *enemies* (including enemy/enemy collisions), *ship*, *plasma shield*, *laser beam* and *missiles* (including missile/missile collisions)
- Enemy ships can launch a homing-missile every 5 seconds
- There are always 10 enemies, regenerated off-screen when destroyed, all with random capacities (shield will increase as your current laser fire power increases to remain fair)
- Currently, you cannot die!

## Technical

Starship currently relies over the [NavoVG](https://github.com/memononen/nanovg) vector graphics library on top of *OpenGL* for *hardware accelerated* displays.

Resources (fonts and sound effects) are all packed in a unique *resource.dat* file using a custom packer.

Window resolution is 1500x900, an fps counter is displayed, fixed rendering loop being made to always reach 60fps: a consumption percent indicator shows how must we currently consume regarding the targeted 16.7ms timespan, a frame drop alert being display when exceeded loop limit.