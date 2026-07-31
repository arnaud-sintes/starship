# TODO

## Missing dedicated sounds

Most events now have a dedicated cue, but a few still borrow from the shared bank; the remaining reuses are ambiguous or misleading.

Landed: dedicated voice callouts for the goody pickups/deploys and the turret farewell (`turret`, `turretOff`, `decoy`, `emp`, `overdrive`, `singularity`, `blossom`) - these replaced the misleading `laserPowerUp` / `plasmaShield` / `propellantRefuel` / `magneticMinesDrop` / `homingMissilesOff` reuses.
Also `hellstorm` ("Hellstorm!") and `repulsor` ("Shockwave!") voice callouts, layered *over* their kept SFX (`missileShot` and `attractorExplosion` respectively) - so the event now reads distinctly even though the effect sound is intentionally shared.

| # | Event | Currently reusing | Problem | Suggested file |
|---|---|---|---|---|
| 1 | Gravity mine explosion | `mineExplosion` | identical to the player's own mines - no audio friend/foe distinction | `gravityMineExplosion.wav` (deeper, darker) |
| 2 | Gravity mine pull | *nothing* | the pull field is silent, you only hear it when it's too late | `gravityMineHum.wav` (loop, volume by proximity) |
| 3 | Singularity active | *nothing* | a black hole devouring the screen makes no noise at all | `singularityLoop.wav` (suction/vortex loop, pitch rising toward collapse) |
| 5 | Singularity collapse | `attractorExplosion` | shared with attractor deaths, which happen constantly | `singularityCollapse.wav` (implosion then boom) |
| 8 | Decoy pinging | *nothing* (visual ping only) | the sonar ring begs for an audible ping | `decoyPing.wav` (short ping, repeated with the visual) |
| 11 | Turret fire | `laserShot` at 0.35 | indistinguishable from the main gun; cooldowns also mask one another | `turretShot.wav` (lighter blip) |

(Numbering kept from the original list for traceability; resolved rows #4, #6, #7, #9, #10, #12, #13, #14, #15 removed.)

Bonus items:

- game-over screen: no UI confirmation sound on the retry click
- blast shockwaves have no dedicated boom (they ride each source's explosion sound - acceptable)

### Priority order

1. **#1** - friend/foe ambiguity actively harms gameplay reading
2. **#3, #2** - the loops; biggest atmosphere payoff (positional hums are what make the engine audio feel good)
3. **#5** - deploy clarity mostly done; the collapse still shares the attractor boom
4. **#8, #11** - polish

### How to add a sound

1. Drop the `.wav` (44.1 kHz 16-bit preferred, miniaudio resamples anything) into `resource/` - the packer picks the folder up automatically at build.
2. Add the enum entry in `AudioDirector.h` (`eSound`, before `count`).
3. Add the filename at the same position in the table in `AudioDirector.cpp` (`_Init`).
4. Swap the call site in `World.cpp`.
5. Loops (#2, #3, #8) additionally want an `AudioDirector::Loop` handle + `SetLoop` volume/pan logic - follow the `spaceWind` / enemy engine pattern (~10 lines each).
