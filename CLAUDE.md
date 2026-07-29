# Starship

Personal 2D space shooter: C++ (latest std), Win32 + OpenGL + NanoVG (vector graphics), miniaudio (sound), MSVC solution, x64 only.

## Build

```bash
"/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" starship.sln \
  -p:Configuration=Release -p:Platform=x64 -p:PostBuildEventUseInBuild=false -m -v:m -nologo
```

- Always build BOTH `Release` and `Debug` before declaring work done.
- `-p:PostBuildEventUseInBuild=false` skips `package/package.bat` (zip packaging), which fails outside real release flows; the pre-build step (packer.exe -> `resource.dat`) still runs.
- Output: `.runtime/<Config>/starship.exe`, run it from that directory (loads `./resource.dat`).
- The exe links as SUBSYSTEM:WINDOWS with `/ENTRY:mainCRTStartup` (plain `main`, no console — do not "fix" this back to Console).

## Architecture (one class = one job; keep it that way)

- `main.cpp` — window/GL/NanoVG setup, fixed-timestep loop: simulation ticks at exactly 60Hz via a wall-clock accumulator (late frames trigger catch-up ticks, capped at 5), rendering paced separately to 60fps by `core/Timer` (vsync off).
- `Game` — composition root: input decoding, prologue / stage / game-over state machine, click-to-retry (rebuilds `World` through `unique_ptr`).
- `World` — the simulation only: entities in `std::vector` (mark-dead + `erase_if` compaction), collisions, score, plasma shield, alerts, audio *events*. Knows nothing about rendering or Win32.
- `SceneRenderer` / `Hud` — drawing only, take `const World &`; everything is culled against the ship-centered screen rect before touching NanoVG.
- `AttractorField` — static strange-attractor field over a uniform spatial grid (physics + visibility queries visit only nearby cells).
- `AudioDirector` — game events -> sounds: positional volume/pan, per-sound cooldown ticks (prevents same-sample spam), looped sounds via `Loop` handles.
- `Audio` — thin facade over miniaudio's `ma_engine` (the ONLY backend-aware file); slot+generation handles, stale handles no-op.
- Entities (`Rocket`, `Laser`, `Missile`, `Enemy`, `Mine`, `Goody`, `Particule`) — plain aggregates; ALL derived/physics state (bounding radius, burst anchors) is computed in `Update()`/`RefreshGeometry()`, never in `Draw()` (Draw is `const`; only visual flicker counters are `mutable`).
- `View` — logical<->physical mapping (see below).
- `core/` — Win32 window/input, Timer (NT high-resolution), Maths/Vector, resource Packer.

## Rules & conventions

- Match the existing style: `_parameter` prefix, `m_member`, `_PrivateMethod()`, braced init everywhere (`const auto x{ ... }`), 4 spaces, opening brace on the same line.
- **Logical coordinates**: the game is designed at a fixed logical HEIGHT of 900; logical width follows the monitor aspect ratio (no letterbox bars). All gameplay/HUD code speaks logical units; conversion happens only in `View` (input via `View::ToLogical`, output via viewport + `nvgBeginFrame` devicePixelRatio). Never use physical pixels inside `Game`/`World`/renderers.
- **Durations are ticks**: the simulation is frame-count based at `m_frameRate` (60); express times as `m_frameRate * seconds`.
- Cross-entity references use rocket `id`s (or index+generation handles), never raw pointers/references that outlive a tick.
- Entity vectors: mark dead during passes (`dead`/`alive` flags, guard iterations), compact once with `std::erase_if`.
- Looped sounds must be stopped explicitly when their owner dies (see `World::~World`); no destructor side effects in entities.
- The process is Per-Monitor-V2 DPI aware: every Win32 metric is physical pixels.
- Windows headers: define `NOMINMAX` before including `windows.h`/miniaudio to keep `std::min/max`.

## Testing / verification

- No unit tests; verify by building both configs + launching briefly.
- The developer is usually AT the machine: launched windows steal focus — keep test runs short (a few seconds), and prefer asking them to play for gameplay-feel changes.
- Screenshot automation gotcha: the desktop is 2560x1440 @ 125%; PowerShell capture/click tooling is DPI-unaware (virtual 2048x1152 coords, captures may be physical crops) while the game is DPI-aware — account for the 1.25 factor before concluding anything is misaligned.
- Sound issues can't be heard from here: reason from code + ask the developer to listen.

## Git

- Commit ONLY when explicitly asked; never push. Message style: short, lowercase, descriptive (see history).
- Bump `version.h` (and the readme title) when a release is cut; `package.bat` extracts the version for the zip name.
- Keep `readme.md` (player-facing) in sync with gameplay/technical changes, and prune the `World.cpp` TODO list when items land.
- Pending work lives in two places: the gameplay TODO list at the top of `World.cpp` and `TODO.md` at the repo root (currently: missing dedicated sounds).
