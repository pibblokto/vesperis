# Architecture

About 5,700 lines of C++17. No engine: everything is software-rendered into a 320x200 8-bit buffer; raylib only opens the window, reads input, plays an audio stream (and auto-explore's music files beside it, R-420) and blits the buffer.

<!-- contents -->
Contents:
- Modules and dependency rules
- Key types
- Per-frame data flow
- Time
- Persistence
- Build
- M6 additions (2026-09-26)
- Threads (M7, 2026-09-26)
<!-- /contents -->

## Modules and dependency rules

```
src/core      <- nothing (math, hashing, noise, framebuffer, raster, font, png, input)
src/galaxy    <- core      (the worlds: starfield, system, planetmap; landmarks and planet_bodygen alone read src/peoples)
src/peoples   <- galaxy    (the peoples: history, plan, towns, works, buildings, ruins, records and the grammar, music, signals)
src/space     <- peoples   (space_view: stars, the band, render; space_sun, space_globe, space_bodies)
src/surface   <- space     (site, surface_view: the frame; surface_env, _places, _walk, _palette, _terrain, _sky, _effects)
src/game      <- surface   (game.cpp the frame; a file per state, screen or device: autopilot.cpp the ship, scene_draw.cpp
                            the scenes, hud.cpp the HUDs, surface_play.cpp the ground, menus.cpp, landing_map*.cpp, the
                            GUIDE's names.cpp, guide_screens.cpp and star_map.cpp (M3), probe*.cpp, radar*.cpp, autoexplore*.cpp
                            (R-419); persistence.cpp saves; ui.*, settings.*; audio*.cpp the synth; src/game/CLAUDE.md the map)
src/platform.h + platform_raylib.cpp  <- game/audio only (never included by game code)
src/main_raylib.cpp        entry point for the window build
src/harness/               vesperis_test: the modes, the unit, the proofs (no raylib; src/harness/CLAUDE.md)
```

Rule: nothing below `platform_raylib.cpp` may include `raylib.h`. raylib defines `PI` as a macro and `KEY_*` enums that collide with ours; `src/core/input.h` mirrors the GLFW key codes so the platform layer can copy them by number.

## Key types

| Type | File | Role |
| --- | --- | --- |
| `Vec3`, `Mat3`, `RGB` | core/types.h | double-precision math; `cameraBasis(yaw, pitch)` |
| `Rng`, `hash3i`, `mix64` | core/rng.h | all randomness is hashing of integer coordinates and seeds |
| `gnoise3/2`, `fbm3/2`, `ridged3`, `worley3`, `GrainTexture` | core/noise.h | gradient noise in doubles |
| `Framebuffer` | core/framebuffer.h | `idx` (uint8 colour index), `invz` (float 1/z), `pal` (768 bytes), `mush()`, `toRGB()` |
| `RVert`, `RasterParams`, `Proj` | core/raster.h | scanline triangles, lines, points in view space |
| `RGBCanvas`, `drawText` | core/font.h | HUD drawing on the RGB output |
| `Input`, `Key` | core/input.h | per-frame input snapshot |
| `Star`, `StarNeighborhood`, `STAR_CLASSES` | galaxy/starfield.h | sector hashing, class table, names |
| `Body`, `StarSystem` | galaxy/system.h | system generation, `bodyPos(i,t)`, `bodyFrame(i,t)` |
| `placeInSector`, `driftersNear`, `DARK_KINDS` | galaxy/drifters.h | D-01: the drifters, worlds with no sun in sectors without a star; the one resolver from a key to its place (a star, else a drifter) |
| `KINDS`, `KindDef`, `TRAITS` | galaxy/kinds.h | P-00: the kinds of body and the traits as rows; the tags the code paths read |
| `BodyGen`, `sampleSurface`, `PlanetMap` | galaxy/planetmap.h | the planet function and its coarse map |
| `GeyserVent`, `landformFeatures`, the finders | galaxy/planet_features.h | E-07: what stands near a point, from the features the ground builds |
| `SpaceRenderer`, `SpaceContext`, `BodyScreenInfo` | space/space_view.h | space scene; also draws bodies into planetary skies |
| `SurfaceSite`, `TerrainCache`, `SunInfo` | surface/site.h | landing site geometry, terrain caches, astronomy |
| `SurfaceView` | surface/surface_view.h | first-person exploration: its data, then its functions by the file that defines them (E-08) |
| `Player`, `Buggy`, `Drone`, `SurfaceEnvironment`, `LocalFront`, `Critter`, `TreeInst`, `Ruin` | surface/surface_types.h | the types beside `SurfaceView` (E-08) |
| `Game` | game/game.h | orchestration, HUD, menus, save/load; an index of its sections at the top |
| `GameState` + `STATE_ROWS`, `ShipState`, `Cabin`, `HarnessKnobs` | game/game_types.h | the types beside `Game`; a state's row (E-06) |
| `RadarState`, `ProbeState`, `DroneBay`, `AutoState` ... | game/*_state.h | each subsystem's state, a `Game` member (E-05) |
| `TestGame` | harness/test_game.h | the harness's `Game`, with the hooks the tests read and set it through (E-05) |
| `AudioSynth`, `AudioState` | game/audio.h | procedural audio |
| `Settings` | game/settings.h | persisted player settings |
| `Guide`, `LogEntry` | game/guide.h | names, log, statistics, history, home, inbox (M3) |

## Per-frame data flow

1. Platform polls input into `Input` and calls `Game::frame(in, realDt)`, which makes its own copy: the controller's buttons become its keys (`padFrame`, R-418), and while auto-explore plays the explorer's input replaces it (`autoInput`, R-419; any key, click, mouse motion or button ends that first).
2. `Game::frame` advances game time `t` by `realDt * timeWarp` while in a simulating state (space, surface, descent/ascent, landing map), runs the state's update, then renders:
   * space: `SpaceRenderer::setupPalette` + `render` into `fb`, `fb.mush(2)`, `fb.toRGB(rgbBuf)`, then the HUD is drawn in RGB.
   * surface: `SurfaceView::render` (palette, sky, terrain, objects, life, capsule, weather), `mush(2)`, `toRGB`, HUD.
   * overlays (help, menu, list, data) render the underlying scene first, then blend a dark rectangle and text.
3. Platform uploads `rgbBuf` (320x200 RGBA) to a texture and draws it scaled with point filtering; optional scanlines.
4. Platform pumps the audio stream from `AudioSynth::render` using `Game::audio` levels. R-420: the same pump hands auto-explore's tape (`AudioState::music`, `musicGo`, `musicGain`) to a thread of the platform's, which streams the file with raylib's music stream, and takes its place, length and end back into `Game::audio`.

F-01 (2026-10-12): with `VESPERIS_TRACE=1` the window build times each frame's parts (the poll, `Game::frame`, the audio pump with
its synth blocks, the screenshot and recorder writes, the texture's upload, the drawing, the swap with its vsync wait: the platform
layer reports the last present's and pump's parts through `platformPresentTimes`/`platformAudioTimes`) and prints their mean and worst
once a second on stderr and, at the quit, the percentiles of the frames' intervals and of their work without the swap.

F-08 (2026-10-14): the platform paces the frames itself (raylib's `SetTargetFPS(0)` on the desktop; the browser paces its own): a
frame is meant for the refresh a whole number of the monitor's refreshes after the last one's (two at 120 Hz: 60 frames a second; one
at 60 Hz), its swap waits until three quarters of a refresh before that one, and the schedule follows the refreshes the swaps met
(`platform_raylib.cpp`). The trace's `hold` is that wait, its work the frame without the hold and the swap. The synth stays on the
main thread (its block is about 2 ms at most on a loaded Mac, one frame in 5.4).

R-420 (2026-10-19): auto-explore's tape is a second stream beside the synth's, raylib's music stream (the file decoded as it plays, at its
own rate and in stereo; the synth is mono at 22,050 Hz). A thread of the platform's feeds it every 5 ms: the game's frames can stall
longer than a stream's buffers last (a landing's first ground, a star's system), and raylib, told not to loop, stops a track as it reads
its last block, that block and the one playing still unheard (up to 0.37 s with the 8,192-frame halves used); so the stream loops in
raylib and the thread stops it 30 ms short of its end. The pump passes the requests and the reports under a mutex; the browser has no
thread there, and its pump does the thread's work. The cassette deck's sounds are the synth's (`deckRender`, a sequence rendered whole
as it begins), mixed over the rest after the duck (`AudioState::duck`). The game's own tape lies beside its executable, whose folder the
platform gives (`platformAppDir`, raylib's `GetApplicationDirectory`; `Game::tapeGameDirs` makes the places from it).

## Time

Two clocks: `realTime` (seconds of wall clock) drives transitions, UI, flight and approach durations; `t` (game seconds) drives orbits, rotations, clouds and the sky. Time warp multiplies `t` only. A new game starts at `t = 3.6e6` s.

## Persistence

Text saves in three slots plus an autosave (`vesperis_save_1..3.txt`, `vesperis_save_auto.txt`, key/value lines, header `vesperis-save 2`). `Game::loadFromDisk()` (called by the window build, never by the constructor, so tests touch no user files) loads the settings and the newest save; the title screen offers Continue. Saves happen on Ctrl+S/F5, from the menu's slot picker, and automatically every 5 minutes, on landing, on launch and on quit. `vesperis_settings.txt` holds the settings (`game/settings.h`).

## Build

`Makefile` with automatic dependency tracking (`-MMD`). Targets: `noctis`, `vesperis_test`, `clean`. Compiler: Apple clang via `/usr/bin/clang++` by default (`CXX=` to override). raylib flags via `pkg-config`.

`CMakeLists.txt` (M0-05): static library `vesperis_core` (core, galaxy, space, surface, game), `vesperis_test`, and `noctis` when raylib is found through `find_package(raylib)` or pkg-config (`-DVESPERIS_BUILD_GAME=OFF` skips it); `ctest` runs the headless modes from the source directory. `.github/workflows/ci.yml` builds with make on macOS (brew raylib) and Linux (headless only, g++), runs the checks, builds the harness with CMake and uploads `shots/*.png`. The tree is not a git repository yet, so the workflow has not run; it was written to be pushed as is.

## M6 additions (2026-09-26)

* `core/input.cpp`: key names, `KeyMap` (physical-to-logical remap from `vesperis_keys.txt`) and, since R-418, the controller's controls and bindings (`PadState`, `PadMap`, `padCapture`; `vesperis_pad.txt`), which `game/pad.cpp` turns into the frame's keys (`Game::padFrame`: the layouts by context, the command menu, the on-screen keyboard, the CONTROLS screen's controller page); `core/fs.h`: `makeDir`/`fileExists` for the Windows build.
* `platform.h` stays free of the core key enum (raylib defines its own `KEY_*`): the pad is passed as `PlatformPad` and copied into `Input::pad` in `main_raylib.cpp`, which also loads the key map and drives the photo tools and the recorder. R-418: the platform reads the controller through GLFW's joystick calls (declared in `platform_raylib.cpp`; raylib links statically, the dev Makefile too), raw and in its standard layout. The desktop loop body is a `step` lambda so the Emscripten build can hand it to the browser's animation loop.
* `packaging/` (app bundle and zip scripts; since F-09 the release's six builds: `toolchains.sh`, `build-mac.sh`, `build-win.sh`, `build-linux.sh` in Docker, `build-release.sh`), `web/` (shell page, build script), `.clang-format`, CI `package` job.

## Threads (M7, 2026-09-26)

Three kinds of worker: `parallelFor` row bands inside a frame (renderers, `toRGB`), one thread per planet map generation (`SpaceRenderer::startMap`, joined when the map is read or evicted), and the terrain prefetch worker (`SurfaceView::prefetchAhead`, one job at a time, joined before its buffer is read). All of them call pure functions; the caches and the framebuffer are written only by the main thread.

F-02 (2026-10-13) adds a fourth, the PNG writer (`core/png.cpp`'s `PngWriter`, one thread made at the first `writePNGAsync`): the
photographs, the recording's frames and the probe's final image are copied and queued (three at most: the caller waits beyond them, so
none is dropped) and written in their order by `writePNG`'s encoder. `pngFlush` returns once the queue is on disk: before a picture is
read back (`readPNG`), listed (the gallery) or must exist (a new take, the window's exit); the queue's destructor writes what is left at
any exit. A thread for a result no frame waits for, like the maps and the drainage tiles; the panorama, the harness's own pictures and
the browser build write synchronously. The screens that show a world's map (the landing map, the drone bay) read it through
`mapIfReady` and say they are reading the world until its thread has made it: `mapFor`, which waits, is the harness's.
