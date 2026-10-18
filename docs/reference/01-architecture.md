# Architecture

About 5,700 lines of C++17. No engine: everything is software-rendered into a 320x200 8-bit buffer; raylib only opens the window, reads input, plays an audio stream and blits the buffer.

## Modules and dependency rules

```
src/core      <- nothing (math, hashing, noise, framebuffer, raster, font, png, input)
src/galaxy    <- core      (starfield, system, planetmap)
src/space     <- galaxy    (space_view: stars, suns, globes, rings, sky bodies)
src/surface   <- space     (site, surface_view)
src/game      <- surface   (game.cpp core loop; autopilot.cpp ship; hud.cpp; landing_map.cpp; game_states.cpp
                            descent/surface/overlays; persistence.cpp saves; guide.* + guide_screens.cpp the GUIDE (M3);
                            ui.* text helpers; settings.*; audio.*; autoexplore.cpp auto-explore, R-419)
src/platform.h + platform_raylib.cpp  <- game/audio only (never included by game code)
src/main_raylib.cpp        entry point for the window build
src/main_headless.cpp      tests and screenshot generation (no raylib)
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
| `Body`, `StarSystem`, `PLANET_TYPES` | galaxy/system.h | system generation, `bodyPos(i,t)`, `bodyFrame(i,t)` |
| `BodyGen`, `sampleSurface`, `PlanetMap` | galaxy/planetmap.h | the planet function and its coarse map |
| `SpaceRenderer`, `SpaceContext`, `BodyScreenInfo` | space/space_view.h | space scene; also draws bodies into planetary skies |
| `SurfaceSite`, `TerrainCache`, `SunInfo` | surface/site.h | landing site geometry, terrain caches, astronomy |
| `SurfaceView`, `Player`, `SurfaceEnvironment`, `Flock`, `Critter` | surface/surface_view.h | first-person exploration |
| `Game`, `GameState`, `ShipState` | game/game.h | orchestration, HUD, menus, save/load |
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
4. Platform pumps the audio stream from `AudioSynth::render` using `Game::audio` levels.

F-01 (2026-10-12): with `VESPERIS_TRACE=1` the window build times each frame's parts (the poll, `Game::frame`, the audio pump with
its synth blocks, the screenshot and recorder writes, the texture's upload, the drawing, the swap with its vsync wait: the platform
layer reports the last present's and pump's parts through `platformPresentTimes`/`platformAudioTimes`) and prints their mean and worst
once a second on stderr and, at the quit, the percentiles of the frames' intervals and of their work without the swap.

F-08 (2026-10-14): the platform paces the frames itself (raylib's `SetTargetFPS(0)` on the desktop; the browser paces its own): a
frame is meant for the refresh a whole number of the monitor's refreshes after the last one's (two at 120 Hz: 60 frames a second; one
at 60 Hz), its swap waits until three quarters of a refresh before that one, and the schedule follows the refreshes the swaps met
(`platform_raylib.cpp`). The trace's `hold` is that wait, its work the frame without the hold and the swap. The synth stays on the
main thread (its block is about 2 ms at most on a loaded Mac, one frame in 5.4).

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
