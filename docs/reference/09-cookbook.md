# Cookbook

## Add a planet type

1. `galaxy/system.h`: add the enum value before `PT_COUNT`; `system.cpp`: add a `PLANET_TYPES` row (name, description, radius range, landable, atmosphere, max moons, albedo), a `typeColor` case, and weights in `pickPlanetType` (every zone that may host it, plus star-class and moon modifiers).
2. `galaxy/planetmap.cpp`: `BodyGen::make` case (amplitudes, densities, colours) and a `sampleSurface` case built from the `fb/rg/crater/domeField/volcanoField` helpers; assign materials, albedo, relief, veg, glow.
3. `surface/surface_view.cpp`: `lookFor` case (ground/secondary/tertiary colours, sky colours, fog distance, sky density, clouds), `materialLook` bank choices if the type uses new materials, rock density in `drawObjects`, pressure in `computeEnvironment`, any temperature offset.
4. `game/game.cpp`: `shortType` name table (10 chars max); landing map `matCol` if a new material was added (also in `main_headless.cpp`'s map renderer and `MATERIAL_NAMES`).
5. Run `./vesperis_test maps`, `space`, `surface <id>` and look at the PNGs.

## Add a star class

1. `galaxy/starfield.h/.cpp`: enum value and `STAR_CLASSES` row (code, name, description, colour, radius, variance, luminosity, mass, max planets, rarity, first orbit multiplier, minimum first orbit).
2. `space/space_view.cpp` `drawSun`: glow multiplier and disc appearance branch.
3. `galaxy/system.cpp` `pickPlanetType`: class modifiers if the class should shape its worlds.
4. `game/game.cpp`: radiation warning distances if hazardous; help text mentions the classes.
5. `./vesperis_test space` renders every class from the first orbit.

## Add a HUD readout

Draw in `renderSpaceHUD` or `renderSurfaceHUD` with `drawTextShadow(canvas, x, y, text, colour, HUD_SHADOW)`. Keep lines under ~52 characters (320 px / 6 px per glyph); use `trunc(name, n)` for names. Text is uppercase only.

## Add a key

Keys are GLFW codes in `core/input.h`; the platform copies raylib's `IsKeyDown/IsKeyPressed` by number. Read `in.wasPressed(KEY_X)` for edges, `in.isDown` for holds. Document it in the help pages (`renderHelp`) and in `README.md`.

## Add a game state

Add to `GameState`, handle update in `Game::frame`'s first switch and rendering in the second; overlays should render the underlying scene first and set `returnState`. Update `mouseCaptureWanted()` if the cursor should be released.

## Add a save field

Append a `key value...` line in `save()` and parse it in `load()`; unknown keys are ignored by older builds, so this is backward compatible. Bump the header only for incompatible layouts.

## Add a palette effect

Palettes are rebuilt every frame (`setupPalette`), so time-varying effects are cheap: modify the ramps or scale `fb.pal` after `setupPalette`.

## Change terrain detail or draw distance

`SurfaceSite::init` sets cache cell sizes and torus sizes (the torus must be larger than twice the draw radius in cells); `SurfaceView::render` sets radii `r0, r1, r2` and the inner skips (`(r0*16)/64 - 1` etc.). Keep LOD1's inner skip just inside LOD0's radius to avoid gaps.
