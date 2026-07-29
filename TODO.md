# TODO

## Missing dedicated sounds

Everything plays *something* today, but every recent feature borrows from the original 26-sample bank; several reuses are ambiguous or misleading.

| # | Event | Currently reusing | Problem | Suggested file |
|---|---|---|---|---|
| 1 | Gravity mine explosion | `mineExplosion` | identical to the player's own mines - no audio friend/foe distinction | `gravityMineExplosion.wav` (deeper, darker) |
| 2 | Gravity mine pull | *nothing* | the pull field is silent, you only hear it when it's too late | `gravityMineHum.wav` (loop, volume by proximity) |
| 3 | Singularity active | *nothing* | a black hole devouring the screen makes no noise at all | `singularityLoop.wav` (suction/vortex loop, pitch rising toward collapse) |
| 4 | Singularity deploy | `plasmaShield` | misleading - sounds like gaining a shield | `singularityDeploy.wav` |
| 5 | Singularity collapse | `attractorExplosion` | shared with attractor deaths, which happen constantly | `singularityCollapse.wav` (implosion then boom) |
| 6 | Repulsor burst | `attractorExplosion` | same overload; deserves a bass *whump* | `repulsorBurst.wav` |
| 7 | EMP activation | `plasmaShieldOff` | sounds like something powering *down* on the player's side | `empBurst.wav` (electric discharge/crackle) |
| 8 | Decoy pinging | *nothing* (visual ping only) | the sonar ring begs for an audible ping | `decoyPing.wav` (short ping, repeated with the visual) |
| 9 | Decoy drop | `magneticMinesDrop` | confusable with actually dropping a mine | `decoyDeploy.wav` |
| 10 | Hellstorm launch | `missileShot` + `homingMissiles` | one shot sound for 16 missiles - undersells it | `hellstormLaunch.wav` (ripple launch) |
| 11 | Turret fire | `laserShot` at 0.35 | indistinguishable from the main gun; cooldowns also mask one another | `turretShot.wav` (lighter blip) |
| 12 | Turret deploy | `laserPowerUp` | shared with the laser-up goody AND blossom pickup - three meanings, one jingle | `turretDeploy.wav` |
| 13 | Blossom pickup | `laserPowerUp` | see above | `blossomIgnite.wav` |
| 14 | Overdrive ignite | `propellantRefuel` | sounds like the blue refill goody | `overdriveIgnite.wav` (turbo spool-up) |
| 15 | Generic bonus expiry | `homingMissilesOff` | quadruple-duty (missiles off, turret, overdrive...) | `bonusExpired.wav` - one shared but *dedicated* "buff ended" cue |

Bonus items:

- game-over screen: no UI confirmation sound on the retry click
- blast shockwaves have no dedicated boom (they ride each source's explosion sound - acceptable)

### Priority order

1. **#1, #7, #6** - friend/foe ambiguity and misleading cues actively harm gameplay reading
2. **#3, #2** - the loops; biggest atmosphere payoff (positional hums are what make the engine audio feel good)
3. **#4, #9, #12, #13, #14** - pickup/deploy jingle clarity
4. **#8, #10, #11, #15** - polish

### How to add a sound

1. Drop the `.wav` (44.1 kHz 16-bit preferred, miniaudio resamples anything) into `resource/` - the packer picks the folder up automatically at build.
2. Add the enum entry in `AudioDirector.h` (`eSound`, before `count`).
3. Add the filename at the same position in the table in `AudioDirector.cpp` (`_Init`).
4. Swap the call site in `World.cpp`.
5. Loops (#2, #3, #8) additionally want an `AudioDirector::Loop` handle + `SetLoop` volume/pan logic - follow the `spaceWind` / enemy engine pattern (~10 lines each).
