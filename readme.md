# Starship beta 0.0.8

## Goal

None currently, just enjoy the game engine and destroy everything for maximum fun!

<iframe style='width:100%;height:480px' src="https://www.youtube.com/embed/zJTK6VkaCGc" allowfullscreen></iframe>

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
- **turret** (*green* "**T**") that will deploy a **mini-turret** orbiting your ship during **10 seconds** (durations stack), aiming at the closest enemy, gravity mine or incoming missile - with lead compensation - and firing at high rate
- **repulsor** (*red* "**R**"), instant: a massive **friendly shockwave** erupts from your ship - zero damage to you, huge knockback, and it chain-triggers every missile and mine in range
- **decoy** (*green* "**D**") that will drop a pinging **beacon** where you grabbed it: enemy missiles chase it instead of you during **8 seconds** (or until it takes 3 hits)
- **EMP** (*green* "**E**") that will shut down **enemy engines and launchers** during **4 seconds** (durations stack): they drift ballistically, at the mercy of attractors, gravity mines... and you
- **overdrive** (*blue* "**O**") that will grant **free propellant** and a **boosted main engine** during **8 seconds** (durations stack)
- **singularity** (*violet* "**X**") that will deploy a **micro black hole** where you grabbed it: for **3 seconds** it drags everything - enemies, missiles, mines, goodies, dust - into an accelerating spiral (you resist it better, but don't linger), shreds whatever reaches its core, then **collapses** into a massive chain-triggering detonation
- **death blossom** (*red* "**B**") that will erupt a rotating **360° laser storm** around your ship during **2 seconds** (durations stack)
- **hellstorm** (*green* "**W**"), instant: launches a **spiral fan of 16 homing missiles**
- **shield addition** (*blue* "**S**") that will boost the shield up to 50% of its current capacity
- **propellant addition** (*blue* "**P**") that will boost the propellant value up to 50% of its current capacity

> [!NOTE]
>
> Goodies are *attracted* by the ship (the closest, the fastest) to ease their acquisition

## Notes

- Games start with a *10 seconds* plasma shield activated, so you can safely run away
- The closest enemy is designated by a faint line and, when 15+ ship lengths away, a **chevron** orbiting your ship with the distance in **ship lengths**
- Scoring rewards *outcomes* over grinding: laser hit **2**, missile-on-missile trick **5**, goody pickup **5**, mine hit **5**, missile shot down **10**, gravity mine sniped **15**, attractor destroyed **25**, enemy destroyed **100** (and nothing for events you merely suffered)
- All engines (left and right / upper and lower, and main engine) will consume propellant
- Propellant tank of all rockets got a *capacity*, a natural *production rate* (refill when no thrust) and a *quality factor* that will reduce the overall *ship form factor* (therefore the ship *mass*) regarding its capacity
- Shield of all rockets got a *capacity*, a natural *repair rate* and a *quality factor* that will reduce the *shield form factor* (therefore the ship *mass*) regarding its capacity
- Engines of all rockets (both *main engine* and lateral *rotator engines*) got a *power*, an *acceleration* and *deceleration rates* and a *quality factor* that will reduce the *engine form factor* (therefore the ship *mass*) regarding its power
- The cumulated *form factor* (so called "*mass*") influences the *drag force* power that will directly influence the *thrust motion*, the ship's *momentum* and the *attraction effects*
- Rotation occurs using a proper rotation momentum, created thanks to the four lateral engines, as it's hard to manually manage, a change of mouse position will automatically deal with these engine to properly rotate the ship as expected (front of the ship pointing in the mouse cursor current position)
- A slight *solar density* factor will slowly reduce the thrust motion
- The **solar wind** blows in waves: a global drift plus local gusts varying across space, gently pushing every rocket, mine, goody and dust particle (listen to the wind ambience swelling and panning with the local flow)
- *Strange attractors* with variable masses attracts all rockets at proximity, can be used as natural shield with some training
- **Gravity mines** (dark red, spiked, don't confuse them with your green magnetic mines) roam the void around you: they pull everything nearby - ship, enemies, missiles, even your own mines and the dust - into themselves and detonate on contact with a heavy blast; shoot them from a distance (**+15 pts**) or lure your pursuers through their pull
- The attractor fields drag free-floating objects too: mines (both kinds) and the decoy beacon slowly sink toward nearby attractors and detonate on contact
- A shield alert will occur when being under 25% of the shield capacity
- A low propellant alert will occur when being under 25% of the propellant tank capacity
- Collision engine takes properly in account *enemies* (including enemy/enemy collisions), *ship*, *plasma shield*, *laser beam* and *missiles* (including missile/missile collisions)
- **Explosions have a blast range**: anything caught in the impact zone takes distance-falloff damage and is knocked back; missiles and mines caught in a blast detonate in turn, so tight formations and mine fields go up in **chain reactions** (the plasma shield blocks blast damage, not the shove - and yes, exploding an enemy point-blank hurts you too)
- The enemy roster is constant (each death respawns the same type off-screen; most shields increase with your laser fire power to remain fair):
  - **3 chasers** (*pink*) hunting you and launching a homing-missile every 5 seconds
  - **1 sniper** (*pale red*) holding far away and firing fast **lead-compensated slugs** at your predicted position - punishes straight-line flying, helpless up close (**+150 pts**)
  - **2 wasps** (*yellow*, small and fast) ramming you kamikaze-style: fragile, but their death blasts chain (**+40 pts** each)
- When your shield is fully depleted, the next hit destroys the ship: after the explosion, click to retry (the score resets, enemies keep patrolling the wreck in the background...)

## Technical

Starship relies on the [NanoVG](https://github.com/memononen/nanovg) vector graphics library on top of *OpenGL* for *hardware accelerated* displays.
Everything drawn is culled against the screen, and the strange-attractor field is indexed by a uniform spatial grid so both rendering and physics only visit what is nearby.

Sound effects are played through [miniaudio](https://miniaud.io/)'s engine (WASAPI): positional volume/pan and continuous pitch modulation of the engine loops are mixed on the OS audio callback with proper resampling.

The simulation runs at a **fixed 60Hz timestep, decoupled from rendering**: wall-clock time is accumulated and late frames trigger catch-up ticks, so the game always runs at real-time speed (under sustained overload, catch-up is capped and the game slows down rather than spiraling).

The code is organized around a few focused classes: `World` (simulation only), `SceneRenderer`/`Hud` (drawing only), `Game` (input, prologue and composition), `AudioDirector`/`Audio` (game events to positional sounds, backend behind a facade).

Resources (fonts and sound effects) are all packed in a unique *resource.dat* file using a custom packer.

The game is designed at a fixed **logical height of 900** (DPI aware): the gameplay scale is identical on every monitor while the logical width follows the screen aspect ratio, so the frame always fills the display edge to edge - no letterbox bars, no distortion. Windowed mode covers the whole primary monitor work area; being vector-drawn, everything stays crisp at any scale.

Vsync is disabled and the render loop is paced to 60fps by a high-resolution timer: an fps counter is displayed with a consumption percent indicator showing how much of the targeted 16.7ms timespan is currently used, a frame drop alert being displayed when the loop limit is exceeded.