# Rendering

<!-- contents -->
Contents:
- Framebuffer and palette (M10-01/03)
- Rasteriser (`core/raster.*`)
- Bank assignment per scene
- Draw order
- HUD (M10-04)
- M5 additions (2026-09-26)
- Presentation options (M6-01)
- Map cache (M7-02)
<!-- /contents -->

## Framebuffer and palette (M10-01/03)

* Logical size 320x200 (`UW`, `UH`) times a runtime scale `FB_SCALE` 1..4 (`FBW`, `FBH`, set by `setFramebufferScale`; the settings item "render scale", default 4 = 1280x800 since B-310 on 2026-09-27, one framebuffer pixel per pixel of the default window; it was 2 = 640x400 before). Every `Framebuffer` is re-created after a scale change (`Game::applySettings`). `Framebuffer::idx` holds 16-bit pixels (`Pix`); `invz` holds 1/z per pixel (0 = infinitely far, larger = closer).
* Pixel format: **32 banks x 11-bit intensity** (`BANKS`, `BANK_SHIFT`, `INTEN_MASK`): bank = `p >> 11`, intensity = `p & 2047`;
  32 intensity units per classic shade (`INTEN_PER_SHADE`), so writers still think in shades 0..63 (`pix(bank, shade)` keeps fractions; `pixI(bank, inten)`, `bankOf`, `intenOf`, `shadeOf`, `toInten`). Bank arguments everywhere are plain indices (0..31), never pre-shifted.
* Palette: 64 colour stops per bank (`pal[BANKS*64*3]`); `setRamp(pal, bank, {{pos, RGB}, ...})` defines a bank as a piecewise linear ramp through stops at positions 0..63; `scalePalette` fades all banks. `toRGB(out, gain)` interpolates the 64 stops to 256 steps per bank and looks pixels up by `intensity >> 3`, so smooth gradients (sky, glow) no longer show 64-step banding.
* `Framebuffer::mush(passes)` is Noctis' `psmooth_64` generalised: for each pixel, keep the bank of the pixel one row below and replace the intensity by the average of the k x k block starting one row below. `g_mushKernel` = scale + 1 in the "classic" setting (2x2 at 1x, 3x3 at 2x, 4x4 at 3x, 5x5 at 4x; "soft" = scale + 2, "sharp" = max(2, scale), and since B-310 "fine" = max(2, scale - 1), the default: 3x3 at 4x, so two passes span five pixels, 1.25 logical pixels, and leaves, trunks and ridges resolve while the edges still melt); two passes everywhere. k = 2 keeps the original inner loop; larger boxes use separable running sums (windows clamped at the right edge). F-07: a pass reads the frame and writes `back`, the two swapped after it (no copy); its rows go to the pool, k = 2's too (the last two rows copied as they were); a larger box slides the column sums of its k rows down a band of rows one row at a time and a k-wide window along them, the row under the pixel giving the bank and leaving the sums in one read, and the quotient is a multiply and a shift (`(sum x m) >> 32`, `m = 2^32 / divisor + 1`: exact while sum x divisor < 2^32, every k to 38), the same pixels as the two passes over a 4 MB buffer it replaced. Consequences: colours never bleed across banks, edges melt, single-pixel points soften (stars and far bodies are therefore drawn as 2x2 logical blocks with a halo), the image shifts up by about one logical row per pass.
* `glowDisc(cx, cy, radius, coreRadius, maxAddShades, bankIfEmpty)` adds intensity inside a disc (sun glow, flares); `bankIfEmpty` -1 keeps the existing bank. F-05: nothing at all when `maxAddShades` is 0 or less (every add would be 0: a dim veil walked its 0.9 M pixels at 4x), and the rows on the pool (a disc under 64 rows stays on the caller); every caller is on the main thread outside a pool job.
* Sizes in scale units (M10-15): line thickness, point size, star dots, the far-body point, the sun's minimum corona and the Vimana streak length are multiplied by `FB_SCALE`; the terrain grain scale too, so the grain keeps its on-screen size; the sky cloud grid cell is `4 * FB_SCALE` pixels; at 3x and 4x the sky is evaluated at half resolution.

## Rasteriser (`core/raster.*`)

* View space: x right, y up, z forward. `Proj::fromHFov(70)` gives focal `f = 160 / tan(35 deg) ~ 228.5` px; screen `sx = cx + f*x/z`, `sy = cy - f*y/z`. W-06 (2026-10-06): the telescope multiplies `f` by its power (`Game::telescopeProj`, x2..x1024, set on `SpaceRenderer::proj` every space frame), so the space renderer draws a field of 0.08 degrees as it draws 70: every body by angle from its direction and radius (`drawGlobe`, `drawSun`), the stars by position, the globe's box by the exact projection (B-312); `bodyInfo`'s radii scale exactly with the power (the harness checks it).
* Near plane `NEAR_Z = 0.05`; polygons are clipped with Sutherland-Hodgman before projection (`rasterPolygon`), triangles fully in front skip clipping.
* Scanline fill with linear interpolation of 1/z, shade, and grain coordinates u,v (stored as u/z, v/z so they are perspective correct; a per-pixel divide recovers them). B-311: a texel of a world-mapped tile that spans more than about two and a half pixels on screen is read bilinearly (`GrainTexture::atLerp`, integer arithmetic), nearest beyond; the ground within a few metres used to be a mosaic of texel squares the mush could not melt. F-07: a span at one depth (`diz == 0`: a leaf cluster, a lily pad, a blob shadow) divides once; a polygon or triangle with no row in the thread's band returns before its fan, and one wholly in front projects its first 15 vertices in place (the clip keeps 15); the opaque replace is written inline and the other blends and the quantized shade out of line (`emitPixelRare`), the textured loop not testing a pixel's depth twice (its float test passing, the double one would).
* Two-material cells (B-311): `RVert::w` is a per-corner weight (1 at the corners of the cell's majority material, 0 at the others), interpolated like the grain coordinates; where `RasterParams::bank2` is set, each pixel's weight is wobbled by the world-mapped `edge` noise (`edgeScale` texels per unit, `edgeAmp` weight units per full amplitude) and the pixel goes to `bank`/`grain2`/the shade or to `bank2`/`grain2b`/the shade plus `shade2` on either side of the half-way contour: the boundary is an irregular line through the cell instead of its square edge.
* Band-parallel drawing (B-310): `setRasterBand(y0, y1)` clips every fill of the calling thread (triangles, lines, points) to
  those rows; the surface draws the terrain rings and the flora in bands of rows on the worker threads (`core/parallel.h`:
  `parallelFor` on a persistent pool, since a frame dispatches a dozen regions and threads created per region cost more than a
  millisecond; one band a thread until F-06 dealt them in chunks, below). A line walks only the steps whose rows fall into the
  band. B-423 (2026-10-12): a pass whose picture depends on where its bands begin runs `parallelBands(n, bands, fn)` instead,
  exactly `bands` bands whatever the threads, shared out among them and run one after another by a single thread: the flora (each
  band's coverage grid hides the trees its own canopies cover), six bands at 1x and eight at 2x-4x (`floraBands`). The terrain and
  the forms draw the same picture in any band layout (each pixel's writes are its row's walk); `unit`'s both-ways check and
  `bothways` hold all three to it. F-02 (2026-10-13): the sky's direction table (`buildDirLUT`, a unit vector a pixel) is made in
  `parallelFor` bands too (a pure function of the pixel; the sprint's FOV kick makes it again on six frames at its start and its
  stop). F-05 (2026-10-14): the cabin is drawn in bands the same way (below), and so are the floor and the reflections inside the
  terrain's bands; the passes that write each pixel once go to the pool by rows: the camera feeds, the screens' dimming
  (`blendRectRGB` from 100 rows: the status bar and the small blends stay direct calls), the landing map's and the sector map's
  blits, `glowDisc`, the sky's cloud and zodiacal grids. The feeds' noise is a xorshift32 walk along the rows: linear over GF(2),
  so a row's steps are a 32x32 bit matrix and each band starts from the state the serial walk reaches at its first row
  (`xorshiftRowStarts`, `core/rng.h`). A blend or a disc that dispatches runs alone, without the pool, when it is called inside a pool job
  (`pool::inside`): every RGB pass runs on the main thread after the mush. F-06 (2026-10-14): the pool deals its rows. `parallelFor`
  cuts its range into chunks (a quarter of a thread's share, at least half its `minPerThread`; `chunk` sets it) that the caller
  and the workers take from a counter until none is left (`core/parallel.cpp`): the caller starts at once and a worker joins as it
  wakes, a fast core takes more chunks than an efficiency core, and a dispatch is done when its work is (the caller waits only for
  the workers that entered: one that wakes late finds nothing). `parallelBands` deals its exact bands the same way. The terrain's
  band pass and the cabin's take the default chunks (thirty-odd at 4x: the horizon's chunk is one of them), the forms chunks of
  eight rows or a sixty-fourth of the frame, the plate's blocks 64 at a time, the probe's panoramas one column at a time; the
  flora keeps its pinned bands. A dispatch from inside a dispatch, or from a second thread while the pool is held, runs alone in
  one call (`pool::inside`, `pool::contended`). The threads sleep between jobs (kept awake 50 µs they won 0.15 ms on the surface
  at 1x-3x and lost 0.1-0.4 on the space frames) and stay at eight (ten on the M4's ten cores starved the recorder's PNG writer).
  Every pass is row-pure or band-pinned, so the picture is the same whatever the thread count: the unit draws six scenes at 4x
  with one, two, four, six and eight threads.
* B-313/B-315 (2026-09-28): `RasterParams::zbias` multiplies the depth tested and written (a coarser terrain ring is pushed back 0.1% so the finer ring wins where the two coincide at a morphed edge); `RasterParams::cutout` drops every pixel whose wobbled weight falls under a half instead of sending it to a second material (the ragged rim and the holes of a leaf cluster, the serrations of a frond: the fan's centre vertex weighs 1.1 and the rim 0.3); the depth test comes before the texel reads, so a hidden pixel costs nothing more; the polygon clipper takes 16 vertices (a leaf cluster is a fan of up to 12).
* `RasterParams`: `bank` (already shifted, e.g. `2 << 6`), optional `grain` (64x64 signed texture added to the shade, `grainScale` texels per unit), `ztest`, `zwrite`, blend mode (`BLEND_REPLACE`, `BLEND_ADD`, `BLEND_MAX`), shade clamp.
* A-02: `RasterParams::pattern` (`FacePattern`), windows on a face by its own u and v in metres (along it, up it): a cell a bay wide and a storey high, the window within it (a rectangle, an arched head, a slit, round, a band), `dark` shades under the wall, from `fromV` up. Where a cell spans under two 1x pixels (`proj.f * min(bay, storey) / z / FB_SCALE`) the pattern is its mean (`meanShare * dark`), from four whole; a lit window (`litBank`) is drawn in that bank at `litShade` where the pattern is resolved. A-04: two shapes more (5 a triangle, 6 a keyhole), `group` lights to a window split by mullions (a pair, three or four: the people's rhythm), `scatter` the share of cells left blank by a hash of the cell; the far mean counts them.
* Lines (`rasterLine3`, with thickness) and points (`rasterPoint3`, size) are depth tested. 2D helpers: `fillRect`, `fillDisc`, `drawLine2D`.

## Bank assignment per scene

Space (`SpaceRenderer::setupPalette`):

| Bank | Content |
| --- | --- |
| 0 | far stars, white ramp (black -> bluish grey -> white) |
| 1 | the local sun: black -> star colour -> near white; also used for sun glow |
| 2 | body A: the body with the largest apparent size (`Game::choosePaletteBodies`) |
| 3 | body B: second largest |
| 4 | blue stars (always defined, so stars stay coloured whatever the bodies use; closes KI-008) |
| 5 | red stars |
| 6 | body A's water (any body's water texels, M1-06/N1-02) |
| 7 | body B's water |
| 8 | the cabin hull (M2), lit by `cabin.lightLevel` |
| 9 | the flight computer's screen (green phosphor); R-404 (2026-10-06): the other computers' screens are banks 21-27, each in its own colour |
| 10 | the cabin light, the fuel orb and the capsule |
| 11 | body A's forest (N1-02) |
| 12 | the companion star (M5-01); D-02: in a system with no sun (no second sun to hold it), a giant's or a brown dwarf drifter's aurorae (hydrogen's pink and violet, `auroraBank12`) |
| 13 | body A's sand family: sand, dust, sulphur, quartz |
| 14 | body A's snow family: snow, ice, and thick cloud (`cloud > 0.6`) so clouds read white |
| 15 | body A's grass |
| 16 / 17 / 18 / 19 | body B's forest / sand / snow / grass (O0-02, B-305: the family banks used to be shared between A and B, tinted by whichever body had the family first, so the smaller world borrowed the bigger one's sand and snow and changed colour as you approached its neighbour) |
| 20 | belt rocks (O3): a warm dark grey under the star's light |
| 21-27 | the cabin's other computers (R-404): the GOES console amber, the star map screen blue, the landing map screen pink, the target data screen cyan, the shard decoder violet, the signal radar red, the telescope's set ice-white (`cabinPalette`, the screens' ramp shape) |
| 28 | in space, the cabin's instruments' case (C-11, R-404's scheme: its shelves' glass in lime); X-01 had drawn the probe's sun here before X-02 composited the probe's picture in RGB |
| 29 | in space, the cabin's drone bay (C-18, R-404's scheme: its screen's cells in a pale yellow; the drones' stripe and arms, their number lamps) |
| 30 | in space, the bay's drones' paint (C-18: a light warm grey under the cabin's light, `cabinPalette`) |
| 31 | body A's water trait colour (Q-02, 2026-10-22: a bloom's, a reef's lagoon's, else a pink lake's, from its map's `ltint`/`wtint`; body B's water stays its own: the cabin holds the other banks). X-01 drew the probe's rain and entry plasma in 30 and 31; since X-02 the probe's picture is composited in RGB from `ShadeRamp`s and uses no bank of its own. A milky sea's glow on body A's night side is drawn in its water's own ramp (its pale upper stops) |

The pixel (O0-02, 2026-09-27): `Pix` is still 16 bits, but the top 5 bits are the bank (32 banks, `BANK_MASK 0xF800`) and the low 11 bits the intensity (`INTEN_PER_SHADE` 32 units per classic shade, 0..2047). The mush, the glow discs, the rasteriser's blends and the RGB conversion work in those units through the constants; the ordered dither nudges by 12 units (1.5/16 of a stop, as before). The regression frames were re-blessed.

Body ramps (N1-02): banks 2/3 carry the rock family's representative material colour (`familyRep(type, FAM_ROCK)`: basalt on molten, venusian, volcanic and cratered worlds, metal, graphite, gas, else rock) from `BodyGen::matColor` times `lerp(starColour, white, 0.5)`, through the shared `materialRamp` (stops {0 black, 14 dark, 34 mid, 47 c, 56 c->white 50%, 63 white}; `dark` = 0.25c, or the body's noon sky light blended in on atmosphere worlds, `mid` = lerp(dark, c, 0.55)); molten and volcanic bodies keep the lava ramp (basalt in its dark part, lava at 50+), substellar bodies their glow ramp, venusian bodies the cloud colour. `SpaceRenderer::globeBank(bodyBank, material)` picks the pixel's bank by `matFamily` (A: 6, 11, 13, 14, 15; B: 7, 16, 17, 18, 19); gas giants, venusian and substellar bodies stay in one bank. The globe's value is `exposureStop(albedo, shade, atmosphere)` (`05-planet-function.md`), the same rule as the ground; the old `63 albedo^0.6 shade 1.1` is gone.

A system with no sun (D-02, `StarSystem::sunless()`): no side of a globe faces a sun (the shadows, the relief's slope term and the limb's
light are off), and the floor of 0.05-0.10 gives way to the galactic band's own light. `galaxyBandLight` takes the parked band (its tone's
1/0.7 power, the column's scale, each cell by its solid angle) to spherical harmonics of the second order, once a build of the band
(`bandSHGen`); `galaxyBandIrradiance` is the light a normal receives over pi (Ramamoorthi and Hanrahan's nine terms): at the drifters near
home 0.105 facing the core, 0.012-0.034 facing away, the galactic poles' normals the darkest. A globe's value there is the band's own
exposure: `DARK_BAND_SHADES` (three times the band's 24 shades) times the albedo times the irradiance, back to the tone (the 0.7 power),
taken over what else it has: a mid albedo's coreward side 9, its far side 3-4, a dark rock's 2-4, so the disc is darker than the band behind
it (the plane 8-10) and the stars stop at its edge. Its own glows stay: a brown dwarf's (darkened toward its limb, `0.55 + 0.45 sqrt(cos)`),
lava, a giant's lightning (its whole disc a night), the moons of a glowing drifter on the side facing it (W-04's earthshine with no night
to gate it), and a giant's or a brown dwarf drifter's aurorae: an oval round each pole at 66-76 degrees, 1.6-3 wide, its curtains running
along it, in bank 12 at 18-30 by its power where it outshines the disc. A dark world too small for a disc is no point of light (a molten
one a faint ember at 12); the rings, lit by nothing, darken the band through them.

Surface (`SurfaceView::setupPalette`, colours from `lookFor(gen, star)`):

| Bank | Content |
| --- | --- |
| 0 | rock family (rock, basalt, metal, graphite): the shared `materialRamp` of `matColor[familyRep(type, FAM_ROCK)]` with the surface's extras: moonlight in the dark end at night, sunset tint and overcast flattening in the lit end, stop 63 = fog colour (horizon with atmosphere, else lightened ground) |
| 1 | sky: {0 zenith, 40 horizon, 50 glow, 56, 63 sun}. Night variant lifts the top toward star-white so stars remain visible. Airless variant: black -> grey -> star colour |
| 2 | water (felisian, ocean) or lava (molten, volcanic); elsewhere the bright crystal/ejecta tint for objects |
| 3 | forest on living worlds (trees and bushes too); elsewhere boulders a shade lighter than the ground |
| 4 | stars: black -> grey-blue -> star white (before M10-03 stars borrowed the ground ramp and came out ground-coloured) |
| 5 | the second flora family's colour (`BodyGen::vegColor2`), and the autumn colour of deciduous trees in the cold shoulder seasons; berries and half the flowers (N2-05) |
| 6 | the buggy's and the drone's hulls, the ship's grey-blue `0.52, 0.55, 0.6` through the material ramp (N4-01, R-403) |
| 7 | chrome: cage tubes, rims, spokes, dial rings {0 black, 20 grey, 44 light grey, 63 white} (N4-01) |
| 8 | snow family on every type: snow, ice (bank 2 stays water only, which the reflection pass relies on) (N1-03) |
| 9 | sand family: sand, dust, sulphur, quartz (N1-03); also mushrooms and lichen points (N2); C-08's drifts against a settlement's walls; A-03's earth (a mound, a bank, a spoil heap, a wreck's furrow) on a desert world |
| 10 | grass (N1-03); tall grass, reeds and the straw variant of 15% of the trees (N2); A-03's earth on a world that is not a desert |
| 11 | aurora (M4-06; B-403: curtains on the cloud grid, drawn after the stars on sky pixels only; its stop 0 is the night sky's zenith colour so the curtains fade into the sky; R-402: the lower curtain's colour per world, bank 21 the tops'); fireflies reuse its green (N2-05) |
| 12 | the companion star's disc and glow in its own colour (M5-01) |
| 13 | a glowing parent in the sky, its colour toward amber (M5-02) |
| 14 | warning red: tail lights, the antenna light, the dial needles {0 black, 14 dark red, 40 red, 63 salmon} (N4-01) |
| 15 | on comets only (O4): the ion tail's blue {0 black, 18 deep blue, 40 blue, 63 pale blue}; P-03 the chemotroph's spores; Q-02 a milky sea's glow at night (its dark end the night sea's, its top the bacteria's pale blue) |
| 16 | bark (B-315): a brown of the planet's own, toward the second flora colour, through `matRamp` (trunks, branches, logs, stubs, roots; on the ground bank the dark stops took the sky's colour and trunks came out black) |
| 17 | cut wood (B-315): pale tan through `matRamp` (the ends of logs) |
| 18 / 19 / 20 / 24 | A-02: the first people's palette (`FORM_BANKS[0]`): the structure, the accent, metal and glass through `matRamp` (a slot of the ground's own material takes that bank's colour, so C-01's walls of rock or adobe look as they did), and light, a ramp that glows whatever the sun (`{0 black, 24 0.3c, 46 c, 56 c->white 45%, 63 fog}`); A-03: the age's materials in them (`agePalette`: the rock, timber and thatch and a post hole's dark; the rock, the roofs, timber; brick, slate, rust; concrete, an accent, steel; composite, an accent, diamond glass; the far future's one material, its accent, black); A-04: the people's genome through its age (`culturePalette`): the structure (the ground's rock tinted a quarter toward the people's base colour, `FormPalette::tint`, or painted in it; its brick, concrete, composite), the accent (where the people put it), the roofing or the glass in the people's tint, and timber and rust in bank 24 where the age has no light (`FormPalette::glow` false: a material ramp lit as the rest), the light's ramp only in the futuristic and the far future's; a pool's water draws in bank 2, a great tree's trunk in bank 16 and its crown in bank 3 (`Form::part` 9, 10, 11) |
| 25-28 | A-02: the second people's palette (`FORM_BANKS[1]`), as 18-20 and 24. Q-02 (2026-10-22): on a world without a people (its traits that take them never come to a people's) 25 is a pink lake's water or the hot springs' mats, 26 a bloom's or a reef lagoon's water or the petrified trunks' jasper, 27 the rainbow strata's accent (`setupPalette`; where the harness stages a people's forms they keep theirs) |
| 21 | the aurora's tops (R-402): the high part of the curtains' profile in the world's second colour, the same ramp shape as bank 11 |
| 22 | the galactic band and the nebula patches (G-03): from the night sky's colour (black on an airless world) to a pale star-grey at 40, toward a protostar's cloud tone under one (S-04). B-408: an airless sky lies in it all over and its sun is drawn in it, so the ramp keeps the star-grey under 20, turns to the sun's colour above it at the grey's light and takes the sky bank's sun stops from 55 (a protostar's sky keeps the cloud's ramp). Q-02: the noctilucent clouds and the moon's ice halo drawn in it (the sky's colour to a pale silver, where the night sky's own ramp is darker in its middle than at its ends: they had dark outlines in bank 1) |

`SurfaceView::materialLook` maps `matFamily(material)` to these banks; a terrain quad takes the majority bank of its four corners (vote over 16 banks); the far floor ring uses the type's dominant family (water, ice on icy worlds, sand on cratered/thin-atmosphere/quartz/volcanic worlds, else rock). Objects: rocks bank 3 (bank 0 on living worlds, so they are rock-coloured and not forest-coloured), quartz crystals bank 2, trees/bushes/grass bank 3, birds bank 1 (dark silhouettes), grazers bank 3, capsule bank 1, rain bank 1. Lightning adds a flash to the whole palette. All colours come from `BodyGen::matColor` (N1); `lookFor` only chooses sky, fog, night and cloud parameters per type.

Distance fog is expressed inside the ramp: shade moves toward 63 (the fog colour) with `1 - exp(-dist / fogDistance)`.

## Draw order

Space: clear (or fade the previous frame by 9 shades during Vimana flight, which makes streaks) -> galactic band (M1-09, the map
at half resolution; G-03: `buildGalaxyBand`, the starlight behind the dust on a fixed exposure in a 128 x 64 map, rebuilt after a
hop of ten sectors, 24 shades of bank 0 at full brightness: the bulge 21, the plane 8-10; F-07: outside a flight and with no
nebula patch in the sky the band's layer is a function of `SpaceRenderer::BackdropKey`, the camera, the projection's focal length
and centre, the frame's size and the band map's generation `bandGen` (moved on by both of its writers, the parked build and a
flight's handover): a frame whose key matches the last one's keeps its layer (2 MB at 4x), and while the key holds the layer is
copied instead of cleared and drawn; any new input of that pass belongs in the key, or a still camera shows a stale band that no
regress catches) -> star field (2x2-logical dots with halo, brightness from luminosity/distance, optional streaks blue-shifted
near the centre and red-shifted outside) -> sun (B-308: `drawSun` draws by angle per pixel from the unit direction to the star and
its angular radius: the disc where a pixel's ray is within the radius, two glow falloffs outside it, the box the projection of the
cone or the whole frame when the cone reaches the camera plane, rows in parallel; F-05: the sun beside or behind the camera skips
the pass when no pixel can lie in its cone (`coneMissesFrame`: its angle off the view's axis less the farthest pixel centre's
exceeds the cone's half-angle, with 1e-5 rad to spare), and so do the protostar's strip and the black hole's lens (`suncone`
proves it); the loop reads its values from its band's locals (KI-409); the rays of compact stars and blue giants and the point of
a tiny sun stay screen-space while the centre is in front; the disc is never dimmer than the glow-lit sky round it; S-04: a
protostar's dust disc is a second pass over the frame, cheap outside the strip along the orbital plane (`discUp`), see
`04-galaxy-and-systems.md`; S-05: a Wolf-Rayet star's shell is a ring at five radii added in the glow's pass, whose cone is
widened to reach it; S-06: a black hole is handed to `drawBlackHole` instead, which paints the shadow, bends the frame drawn so
far (the band, the stars, the belts) round it through a lens map on a grid of one logical pixel, draws the accretion disc with its
far half along the bent rays and a jet on some, see `04-galaxy-and-systems.md`; the lens flare is skipped for it; the old
screen-space circle's radius `f tan(a) d/z` exploded as the centre neared the camera plane and flooded the frame with a companion
star's colour) and lens flare (the sun radius clamped to a fifth of the frame) -> bodies far to near, each ray-traced per pixel (a
globe is skipped only when its whole sphere lies behind the camera plane, O0-03/B-304: with its centre behind or beside the camera
the rays are tested over the whole frame, since a huge parent's limb can fill the top of a moon's sky while its centre is behind
you; B-312: the box of a globe in front is the exact screen extent of its sphere, `sphereBox`, the tangent planes through the
camera's axes, united with the projection of the ring's outer circle, `ringBox`, so a parent whose centre has left the frame to
the left or the right keeps its limb and its rings in view: the old box of radius `f tan(a) d/z` round the projected centre was
empty with the centre ten degrees outside the frame, and the rings' box was never reached; sphere hit, normal -> body frame
lat/lon -> `PlanetMap` albedo/material/cloud (F-10: the clouds' drift wrapped within a turn once a frame, `cloudDriftOf`), relief
shading from the height map's slope toward the star (M9-15; B-407, 2026-10-06: the slope's term is taken off the star's cosine,
since ground rising toward the star faces away from it; M9-15 had added it and ridges were lit from the wrong side); W-06 (the
user's review, 2026-10-06): for `SpaceContext::detailBody`, the telescope's body, the albedo, the material, the storms, the cloud
and the slopes come from the renderer's plate wherever it has a cell (`DetailPlate`, below), the map standing in elsewhere,
lighting, shadows of moons, the parent and the rings (M1-08; F-07: a body whose shadow cannot fall on a lit point that frame, its
shadow's cylinder wide of the globe or the body behind it from the star, is left out of the pixels' loop; the map's lookups are
inline in `planetmap.h`), W-04's earthshine (a moon's night side lit by its parent: `bodyShineAt`, the ground's planetshine
formula, times the cosine toward the parent, fading in over the first tenth past the terminator; in space added to the shade, in a
sky drawn at 30-42 of the sky bank where it beats the sky's own pixel), limb glow, rings with planet shadow and shepherd gaps; gas
giants get zonal jets and night lightning; a globe whose map is still generating is drawn flat; far bodies are single logical
pixels). A gas giant or a substellar object of a Wolf-Rayet star gets, after its globe, its envelope streaming away from the star
(S-05, `drawStrippedTail`: a tube from the limb to four radii out, widening and thinning, curling back along the orbit and
wavering slowly, in filaments along the flow; B-427: about ten splats along the axis, each with the light of its stretch, 24
shades side-on at the root and a soft ceiling of 36 down its length, each hidden where the frame holds something nearer (the globe
hides the root from the day side, the tail veils the night face), in the giant's bank where it outshines the pixel and dithered
with the pixel's bank where neither does, sampled once a 2 x 2 block at 3x and 4x; in a sky the sky bank's luminance is added, at
most its shade 40's). A black hole's companion gets, before its own disc, its gas streaming to the hole (S-06,
`drawAccretionStream`: forty glow discs of the companion's bank 12 from the limb facing the hole to the disc's edge at nine radii,
thin at the companion and flaring into the disc, bent back against its motion; in space only, since a sky draws its own companion
sun). Asteroid belts are drawn as hashed points between the star field and the sun; after the globes, when the ship is inside a
belt, `drawBeltRocks` (O3) draws the rocks within four cells: points under 1.6 px (bank 3, lit by phase), else lumpy meshes (a 6 x
3 or 12 x 6 sphere with fbm radii, tumbling, back faces culled, `0.06 + 0.94 lambert` shading, depth-tested), far to near. In
cabin mode the cabin geometry is drawn after the space scene with depth (`Game::drawCabin`; F-05: in bands of rows on the pool as
the ground's rings are, the frame's values taken once in a `CabinFrame` (the eye, the light, the screens' flickers, the decoder's
wait, the rack's drones with their models: `droneRackFrame`), every band walking every piece in order (`drawCabinBand`), then the
bulb's and the orb's points and glows, a glow not being clipped to a band): floor, ceiling ring, wall bands and pillars, mullions,
the cage bars and the capsule dome, the console and desk boxes with their screens, the bulb and the orb, and C-18's drone rack
(its panels and shelves in the hull's bank, its screen's cells in bank 29, the drones from `droneModel`: faces in bank 30, 8, 29
and 0, the arms and struts as lines, the lamps as points; drawn after the case, so the rack's back panel hides a drone backing
through its port); before the cabin, C-18's drones outside the hull (`drawDroneLights`: within 2.6 s of the hull the model lit by
the sun, depth-written so the walls drawn after cover it outside the window band, farther a strobe in bank 10 without depth); a
depolarised hull (B-411) draws only the hull (the floor, the ceiling ring, the walls, the pillars and the mullions, the deck's rim
from the roof) with `BLEND_DARKEN` and its `sheen` (the pixel behind at 45% of its intensity, or the glass's own shade in bank 8
where that is the brighter: the cabin light's level times 16, `GLASS_SHEEN`, times the face's share of the hull's shade, so the
outside shows through tinted and the lit cabin's glass catches a faint sheen), and what stands in the room (the consoles, the back
wall's sets, the case, the front desk) and the deck's floor with the solid hull's parameters; M2's `BLEND_DARKEN` on every face
had turned the furniture to glass with the walls and left the light nothing to light.

The telescope's plate (W-06 reworked on the user's review, 2026-10-06, `DetailPlate` in `space/space_view.h`, `updatePlate` in
`space_globe.cpp`): the world map is the planet function sampled at 11-44 km, so at the eyepiece's powers (a disc over about 80
px at 1x) a texel spans pixels and the globe blurs. The plate is the same function sampled again at the eyepiece's own footprint:
a body-fixed orthographic grid round the aim point (`u0` where the central ray meets the sphere, else the point of the sphere
nearest the ray; `e1` along the screen's x projected onto the tangent plane, `e2 = u0 x e1`; a point's cell at `a = dot(p, e1)`,
`b = dot(p, e2)` over the step `texel` in radii, the cell's own point `sqrt(1 - a^2 - b^2) u0 + a e1 + b e2`), the step a pixel's
footprint at the aim point (`dHit / f`, at 3x and 4x the 2x step: the mush melts the rest), the extent from the field's nine
points on the sphere (the corners, the edges' middles, the centre: where a pixel's ray meets it, else the point of the sphere
nearest the ray, which counts up to two and a half fields) in the grid's own axes, a quarter more and four cells (so an oblique
view, whose ground stretches along one axis, is held, and the whole near hemisphere when the disc is smaller than the field),
capped at 900,000 cells (a wider plate takes a coarser step), each cell `sampleSurface` at `texelM` (the height, the material, the
albedo with the giants' polar vortex, the storms' veg) and `sampleCloudPattern` at the drift of the build. Built progressively
(`plateFill`: blocks of 8, 4, 2 and 1 texels, a block's origin sample standing for the block until the finer pass reaches it, the
blocks of a pass in rows across the worker threads) within `detailBudget` samples a frame (the game's 8000, about 2-4 ms on eight
workers at 1-3.5 us a sample; 0 reads the whole plate out: a photo, the harness). The globe reads it per pixel (`plateFind`: the
point turned about the spin axis by a giant's jet, the cloud by the drift since the build, so the plate never moves; the albedo
and the cloud bilinear between the samples of the cell's own block step, the material and the storms nearest, the slopes over the
block step along `e1`/`e2` times the rim's cosine, weighted `2.5 + 3.5 smoothstep(200 m, 1.5 km)` into `lit` as the map's 5 is:
the function's slopes are a few thousandths at kilometre steps and a tenth at a few hundred metres), the current plate first, the
previous as the fallback (kept while a new one builds after a zoom step, the field's points leaving the plate or the aim leaving
its forty degrees, the season's bucket, or the rivers' tiles arriving: `drainagePrefetch` in the background for steps under 2048 m
over at most 200 km of ground, the plate sampled with `DrainageOff` meanwhile and begun again when `drainageReady`), the map where
neither has a cell. No plate when the step would be coarser than three quarters of the map's texel (the map serves) or the disc is
under eight pixels; `plateInfo` for the harness. W-03 (2026-10-06): the fronts' bands on a globe come from
`SpaceRenderer::frontsFor` (`buildFrontMap` in `galaxy/fronts.*`: the fronts' cloud at the clock's time on the world map's 512 x
256 grid, four entries by seed, built again when the clock has moved four minutes), read bilinearly per pixel at the body-frame
point (body-fixed moving features: no drift) where the cache's mask of 16 x 16 texel blocks says a band can be and taken as the
greater of it and the pattern's or the plate's cloud; on desert and thin-atmosphere worlds a band is a haze in the ground's own
bank at 70% instead of the snow bank's white. The same overlay whitens the landing map and its zoom window.

The probe's camera (X-01, 2026-10-07; X-02 the same day, `Game::renderProbeFeed` in `game/probe_feed.cpp`, `renderProbeView` in
`space/probe_view.*`): through the fall and the entry the picture is the space renderer from the probe's place (`detailBody` -1);
over the entry's 62-92% it hands over to the probe's own picture, crossfaded in RGB (two renders for four seconds), and the
entry's plasma goes over both in RGB (brightest round the fall's direction, streaked by a 1024-entry table of the angle round its
vanishing point, the scene showing through in the plasma's colour). The probe's own picture is drawn from the decks' fields
(`GiantDecks` in `galaxy/probe.*`: the giant's decks round the aim point (X-02's ammonia and water decks, X-03's one to three by
condensation), km east and north, heights over the one-bar level: a top is the globe's bands lifting the zones and dropping the
belts (`GiantBands`, the planet function sampled at the launch on a 160 x 160 grid of the tangent plane out to 4800 km, at the
jets' drift of the launch's time, so the band and the storm aimed at are the ones fallen into), a long swell, cloud streets along
the bands, the convection's cells (broad domes, narrow troughs), five octaves of billows (folds of |noise|, steeper the finer) and
the thunderheads (one at most in a cell of a 52 km grid, more in the belts and the storms: a dome whose sides are lumped into
cauliflower heads and whose top flattens into an anvil with an overshooting dome); a base hangs in pouches; inside, the density
follows the top and the base with soft edges and a structure of flattened cells with caverns a few km across; everything scales
with the scale height (`sh`, the plain giant's 25 km = 1, 0.2-2.5); the noise is `cloudNoise2/3`, a 32-bit gradient noise ten
times as fast as the planet function's, which buys nothing at these scales). Three passes, all on the workers and at the logical
resolution whatever the scale: (1) the panoramas: the deck under the probe and the deck over it are marched outward one azimuth at
a time (one column a logical pixel, the columns and the rows on fixed angular grids and the steps on a fixed grid of distances,
2.2% each, so neither a turn nor the fall resamples the clouds), each sample filling the rows it rises over (a top seen from
above) or hangs under (an underside from below), the rows between two samples seen one after the other interpolated, the giant's
curve dropping the far deck to a true horizon; a top's shade is the globe's exposure of the band's albedo (brighter on the high
tops, darker in the troughs) under the sun wrapped round the billows (the slopes it sees held soft), the towers' shadows (the
cells along the sun's way, each tower's dome against the sun's line at its closest approach), the rings' shadow and the sky's
light darker in the folds, the terminator crossing the decks (the sun over a place raised by the giant's curve toward it, a crest
seeing past the dip), a highlight shoulder; an underside's is the light come through the deck, the pouches' bottoms brighter than
the creases; (2) the fog, every fourth logical pixel: 18 steps along the ray out to 7 sh km through the deck the probe is in (its
top and base cached round the probe), the light coming down through the deck from its top's to its base's, the sun's own under the
top toward its side, each sample shaded by the cloud over it (two taps up, against the deck's mean: a cavern's walls lit from its
open roof); the lamp a little under the lens marched on its own twelve short steps (its cone in the fog, the rain's drops and the
deep's motes), the lightning's light in the fog, the deep's heat glow; (3) the logical nodes: the nearer of the floor and the
ceiling under the clear air's haze (the optical depth of the path from the mean extinction over the altitudes it spans,
`giantHazeTau` by altitude, the haze's colour the horizon's), or the sky: as much of the horizon's light as the ray's optical
depth gathers over the zenith's (so high over the clouds the sky is black down to a thin bright limb at the deck's own horizon, a
grazing ray's path through the layers the grazing factor longer; in the haze bright all over), the sun's aureole, the giant's
shadow rising in the air toward the night side at dusk, the rings' arc (their plane's density, the giant's shadow on them) and the
cirrus (streaks along the wind racing on the jet in the band's colour), then the fog in front; each node keeps the light it adds
and how much of the space beyond shows through. The frame is the nodes read bilinearly plus that share of the space renderer's
frame without the giant (`SpaceContext::excludeBody`, rendered only when the frame's sky can show it: its stars dimmed by day, the
sun's disc, the moons in their textured banks), then the lightning's channel (a midpoint walk with branches, glowing, behind the
haze), the rain rising past the lens and the deep's motes (lit by the lamp's beam and the flashes), the link's static. Every
colour is a `ShadeRamp` (a 64-shade ramp read as RGB): the decks' the globe's own material ramp of the giant's colour under the
star's light, reddened by the light come down through the decks, the water deck paler; the air's from looks blended by the
pressure's logarithm (the dark sky high over the clouds, the amber haze, each deck's top and base, the clear band's red, the
deep's grey). The camera's exposure is set over the clouds for the deck under it (and takes back four tenths of a dark belt's
dark), in a deck for the fog round it, in the clear band for the ceiling (the light come through it), up to a gain of 12; what it
cannot take back dims the colours. Costs at 2x: 4-8 ms a frame in the descent (the panoramas 2-4.5 ms, the fog 2-3, the nodes
1-2), 15-20 ms in the four seconds of the handover; `probe stability` shows no flicker (under 0.03% of the pixels jump by 40 a
frame, the readouts' digits). After the picture the link's static goes over the RGB (a noise of 4 levels at the top growing to 38
and snow toward the hull's limit), and the last frame is held with a rolling band and the hiss's grain.

X-03 (2026-10-07) gives each giant its own picture. The decks are one to three, each its cloud's (`CLOUD_SPECIES`: the first deck
the globe's colour as before, a deck under it its cloud's own mixed half with the band's), and `layersOf` finds the floor, the
ceiling and the fog among them. A clear-air hole (`GiantDecks::holeAt`: the hashed holes and the descent's own) shows in its
deck's panorama: its floor is the next whole deck's top (a panorama cell carries the deck it shows, `Pano::dk`, read from the
nearest of the four cells), its wall is the deck's own cloud over the outer two fifths of the radius, its slope toward the sun
taken on the holed surface (`holedTop`; the deck's unholed top had turned every wall from the sun), and the rows between a hole's
floor and its wall are the wall's (switching colours row by row had striped the walls); an underside is open through a hole and
the fog has none there (its cached top under its base). The deep's glow (`giantGlowAt`) is a second channel of every surface and
of the fog (`Pano::gl`, `FogNode::glow`) in the glow's colour: come up through a top (brighter in a trough than through a tower's
thickness), onto an underside (its pouches' bottoms most), up from a deck's base in its fog (B-409: six tenths of it back to the
lens, darkened where the cloud under the point is thick, two taps down the glow's way against the deck's mean structure as the
sun's two taps up are, only where the glow counts over 0.02), and scattered by the clear air (most toward the depth). The polar
aurora (`auroraAt`: B-403's curtains at the giant's scale, three sheets across the way to the nearer magnetic pole at the oval's
distance, a lower border 300-600 km up, folded and rayed, the profile's integral between the heights where a ray enters and leaves
each slab) is computed on the fog's grid and added where the sky is open, over the air and in front of the rings and the stars.
Diamond hail glints in the lamp (specks, a cross of light when a facet catches the beam); the rare giant's sight is a density in
the lamp's march alone. The great storm is the deck's own relief (a dome, a wall, streets wound round its centre). The colours: an
ice giant's light is blued by its methane from high in the air down and its colour gain falls slower with the depth; a brown
dwarf's ramps are its globe's red, its deep orange. The exposure counts the glow with the sunlight (a gain down to a quarter); the
camera's default turns to a great storm's wall within 2,500 km of the aim, else to the aurora's curtains within 2,800 km of the
oval, and looks down into a clear-air hole the descent falls through (further down in its shaft); the cirrus sits over the first
deck whatever its height (at 0.3 bar it had shown inside the holes). Costs at 2x as X-02's (`bench check`'s `probe 2x` 7.5-7.7 ms
of 16, 3.8 between the decks; the frames of `probe giants` 4-15 ms, the aurora's grid under a millisecond, a hole's floor and wall
a second top read where a hole is).

X-04 (2026-10-07) keeps what the probe saw by drawing it again. The descent is a function of the probe's numbers, so a recording is the record's numbers put into a `ProbeState` and drawn by `renderProbeFeed` at the recording's clock and the game time the record's clock track gives, the camera the record's track plus the viewer's look (`Game::renderRecording`): the record's probe, and for a giant of another star its `StarSystem` and its `StarNeighborhood` (the sky's stars round that star), are swapped into the game's for the call (`Game::asRecording`; the space renderer's caches are keyed by the bodies' seeds, so nothing it holds is mixed up). A frame of the recording equals the live frame of that clock (the unit compares them: under 2 levels a pixel on the mean). The final image is the end's frame drawn at the end (`probeKeep`), written beside the photographs and held on the screen; a lent recording's is drawn when the gallery shows it. The recording's depth bar and the data sheet's profile chart are drawn with the HUD's lines and fills at the logical resolution (the chart's grid dotted, the layers' strip in their colours: a deck its cloud's, the deep dark red, a hole cyan).

P-07 (2026-10-22) gives the made giants their class's looks (`KindDef::giantClass`). On the globe (`drawGlobe`, its ramp in
`setupPalette`'s `bodyRamp`): the water-cloud and the azure giants through the material ramp of their white and their deep blue;
the alkali giant's ramp a night's dull red low (6-13), a dark slate over it (32-46) to a pale grey-slate at 63 (its lit air the
little light it gives back); the silicate giant's red to orange low (7-21: its night and its terminator) under its clouds of rock
(44, a grey-tan, to a pale tan at 63). The hot giants' night side glows by the ramp's low end (`v` at least the glow's shade past
the terminator, fading in from `lit` 0.12 to -0.3): the alkali's 12 shades by the bands; the silicate's 11-16 by its longitude
past the evening terminator (east of the substellar point along the turn: the jet carries the day's heat over it), dimmed by up to
three fifths toward the morning in streaks along the bands where its iron condenses and rains out (Ehrenreich et al. 2020); seen
as a point the silicate giant glows (30 at least). Lightning on the night side only on a plain giant and a water-cloud giant. A
hazy giant's air reaches far over its clouds (the bubble 1.07 radii for the gas dwarf, 1.22 for the super-puff, the old 1.045 for
the rest), and its disc's limb is pulled toward the haze's pale (half of the way at the edge for the gas dwarf, four fifths for
the super-puff, by the square of the edge's nearness). From a moon's ground a hot giant's night side stays dark as every giant's
(no bank of its own there). In the probe (`renderProbeFeed`, `galaxy/probe.cpp`): each class's light down the decks, the deep's
colours and the haze's shift (`MadeLight`: the water cloud's white, an azure giant's blue from high in its clear air, an alkali
giant's blue and dim with a deep dull red, a silicate giant's orange from the glow under it, a hazy giant's pale amber); an azure
and an alkali giant expose for the air round the probe (their globe is their air's, no deck seen from orbit), their first deck its
cloud's own colour.

Surface: `setupPalette` -> `clearDepth` -> sky per pixel (direction LUT rotated by the camera; two cloud layers on a `4 x scale`
grid, halo, twilight wedge) -> system bodies in sky mode (lit side mapped to the top of the sky ramp, dark side only writes depth;
B-317: the depth in metres, the terrain's unit, since a moon 20,000 km away written in kilometres read as 20 km and stood in front
of every hill beyond) -> stars masked where the sky is bright, twinkling through atmospheres -> the galactic band and the nebula
patches (G-03: bank 22, from the night sky's colour to a pale star-grey, over the sky pixels whose light it beats, both read from
the palette; hidden by the clouds; 20 shades at full brightness, the wings under two left to the sky; it used to be added to the
sky's value, which the night ramp cannot show; B-408: down to the horizon of the drawn ground, the floor's on a planet and the
frame's bottom on a small body, not to the horizontal, and behind the bodies' night sides; on an airless world every sky pixel
goes to bank 22, the band added to the sky's own light carried over by its light through a table of equal luminance, the pixels
under a sun's disc excepted, whose shade the disc reads, so the suns' glows drawn after it land on one bank: over the band's patch
they had lit the band's ramp and beside it the sky's, black under 30, a pale fan with the patch's straight edges; W-04: the
zodiacal light joins the band's light here, `zodiacalLight` times the world's `zodiacalDust` times the band's gain, read on the
sky's cell grid and interpolated) -> the aurora (B-403: the curtain grid's shade over the sky-bank pixels darker than it, faded
under the twilight glow, in bank 11 or, where the tops' share is the greater, 21 (R-402), and over the band's pixels (G-03); the
stars and the lit discs of sky bodies stay, a body's night side shows it in front) -> meteors (M10-14's sporadic ones; W-04: a
shower's streaks from `showerMeteorsAt` while its stream is active, running away from its radiant, in the stars' bank as the
sporadic ones) -> sun (depth-tested, so a body in front carves a partial eclipse; corona grows with the coverage; B-408: on an
airless sky in bank 22 with its glow, eclipse corona and flare, a black hole's and a protostar's in bank 1) and lens flare -> the
floor fan beyond the last ring at the minimum far height or sea level (curved; none on small bodies; F-05: drawn first in each of
the terrain's bands) -> the terrain rings, the near ring at 4 m (radius 30, O6-02), LOD0 (16 m cells, radius 40, inner 8 skipped
under the near ring), LOD1 (64 m, radius 36, inner 10 skipped), LOD2 (512 m, radius 26, inner 5 skipped), LOD3 (2048 m, radius 26,
inner 7 skipped; O1; B-313: a coarser ring skips the cells the finer covers up to a cell of overlap, `skipOf`, and each ring's
outer band is morphed onto the coarser ring's surface, see 06 "Geomorphing"), with skirts and water quads at sea level (waves,
foam); B-310: the vertex caches and the material tiles of every ring are filled first (`warmTerrainLOD`: the cells' misses on the
pool since F-03, the vertices in parallel by rows of vertices since F-04, a water corner with its vertex; B-313: coarse to fine,
since a ring's morph reads the coarser ring's vertices, three cells into its skip disc too), then the cells are binned once on the
pool (F-04, `binTerrainLOD`: the culls, and the rows each cell's pieces reach as the rasteriser projects and clips them) and every
worker thread draws, near to far in the walk's order, the cells that reach its band of rows (`drawTerrainLOD`), reading only, so
the pixels come out as the serial loop's -> reflections of the land into water pixels (F-05: their polygons gathered once after
the bin with the rows each can fill, serially since the walk fills the caches on a miss, `gatherReflections`, and drawn by every
terrain band after its rings, `drawReflectionBand`) -> footprints and the explorer's shadow -> objects (with blob shadows) ->
flora (band-parallel too: the trees are listed once per frame with the rows each can touch, a band thread places only those and
skips a canopy blob wholly outside its rows before its points are placed) -> the ruins (A-02: every settlement's and work's forms
gathered building by building at its own level of detail, built, lit and fogged in chunks on the workers, then drawn band-parallel
like the terrain, every band walking the same polygons in order; `surface/forms_draw.cpp`; B-417: the old ones' monoliths and
their own drawing gone) -> the shards (C-03) -> the buggy and the drone -> life -> capsule -> weather -> the veiling glare round
the sun (B-309, `drawVeil`) -> `mush(2)` (3 on venusian worlds) -> `toRGB` -> HUD in RGB.

Terrain quads are shaded by the smooth gradient of their corner shades since B-311 (`shadingMode` 2, the default; the 4 m near ring had made the flat cells a mosaic); the flat cells of the original's `polymap` (average of the four corner shades, mode 0) and the 12-step gradient (mode 1, M10-12) remain in the settings. The grain texture (amplitude 9, 3 texels per metre at LOD0), the material meso tiles, the broad tone tile of the vertex shade and the mush provide the texture.

## HUD (M10-04)

Drawn after the mush in the RGB buffer with the 5x7 font (`core/font.*`): glyphs for ASCII 32..95 plus `CH_DEGREE`, arrows, block, dot. Lowercase maps to uppercase. Colours are constants in `game/ui.h` (`HUD_GREEN`, `HUD_AMBER`, ...). The HUD code lays everything out in logical 320x200 units (`UW`, `UH`); `RGBCanvas::scale` multiplies coordinates, glyphs, lines and rectangles by the resolution scale, so the HUD looks identical at every scale. Screen positions that come from the projection (`BodyScreenInfo`, `proj.f`) are framebuffer pixels and are divided by `FB_SCALE` before use in the HUD; pick radii (14 px stars, 18 px bodies) are multiplied by it. The landing map fits the 512x256 map into a 256x128 logical frame: one texel per pixel at 2x, every second texel at 1x.

## M5 additions (2026-09-26)

* Bank 12 is the companion star's ramp (space `setupPalette` when `sys.companion >= 0`; the surface when `env.hasSun2`), bank 13 the substellar glow ramp in a moon's sky. Space `bodyRamp` gives substellar bodies their own red ramp instead of the star-tinted one.
* `drawGlobe` draws a `PT_COMPANION` body with `drawSun` (depth-tested, bank 12) and returns; in sky mode the surface draws it itself. Every other globe is lit by `sys.lightAt(pos, t)`: the brighter of the two suns. Substellar globes take `shade = max(shade, selfGlow (0.3 + 0.7 albedo))`.
* N5-01 nebula patches (G-03: of the star-forming complexes within 42 ly): the backdrop loop of `SpaceRenderer::render` asks `nebulaPatches(obs)` for the patches and paints `nebulaGlow` (0..1) as bank 4 (blue), 5 (red) or 0 (white) at up to 17 shades where it beats the band. N5-03: gas giant globes read the storm term from the map's veg channel and replace the baked storm with `storm x (0.55 + 0.45 sin(t / 86400 x 0.9 + 4 lat + seed))`, so storms grow and fade over about a day, band by band.
* `drawCometTail` (called before the nucleus point or globe): activity `clamp((1.6 ref / d_star)^2)` with `ref` the first planet's orbit; a coma of 60,000 km and 36 tail discs widening to 960,000 km along the anti-star direction, `glowDisc` adds 30 and `18 (1 - f)^2` scaled by activity and `skyDark`. N0-01 (B-201) animates it without touching the orbit: the coma breathes (`1 + 0.1 sin(1.7t) sin(0.61t + 1.3) + 0.05 sin(3.1t)`), five knots (`cometKnot(k, t)`: `k/5 + jitter + t/40`, wrapped, Gaussian width 0.06 along the tail) modulate the ion tail by 0.82-1.18, and a dust tail of 24 discs (`0.55 lenKm g` along the anti-star direction plus `0.28 lenKm g^2` along `-velocity` projected off it, 100,000-1,200,000 km wide, adds `9 (1 - g)^2`) curves back along the orbit. When the nucleus is at least 3 px across, `2 + seed % 3` jets vent from the sunlit hemisphere (directions hashed from the seed, pushed toward the star when they face away), each 14 discs from 1.02 to 4 radii bending `2.2 R s^2` anti-starward, radius `0.1-0.45 R`, three knots per jet streaming out every 8 s; they turn with the nucleus frame.

## Presentation options (M6-01)

`Framebuffer::toRGB(out, gain, dither)`: with `dither` the 11-bit intensity is nudged by a 2x2 Bayer pattern (-12, +4, +12, -4 in intensity units, 32 a stop, one cell per logical pixel) and snapped to whole palette stops, so ramps show the original's chequered steps instead of the 256-entry interpolation. `platformPresent(..., aspect)`: aspect 1 scales the picture 1.2x vertically inside the window (letterboxed), the way 320x200 filled a 4:3 monitor; scanlines and the CRT shader follow the drawn rectangle.

## Map cache (M7-02)

`SpaceRenderer` keeps at most `mapCacheCap` (24) planet maps, about 19 MB: `mapIfReady` stamps every use, `startMap` evicts the least recently used map that is not being generated before adding one. `mapStats` counts generated maps, hits and evictions; `genMicros` sums the worker generation time (`bench` prints the average: about 113 ms per 512x256 map on an M4).

T-01 (2026-10-09): the world's plan on the maps (`peoples/plan.h`). The landing zoom (`Game::buildLandingZoom`) draws the plan's ways
through the window as texel lines by their kind (a path fainter, a railway darker, a highway and a maglev pale, a canal blue-dark; the
waypoints: a leg's walk wanders less than a texel) beside C-09's lanes, and the places' cores as discs of built ground over the map (the
rock's colour and the people's structure colour, the disc's share of the texel, times 0.75 - 0.45 x the people's decay). The telescope's
plate (`SpaceRenderer::plateFill`) reads the same once the plan is built (`worldPlanIfReady`: never waited for in a frame): over each cell
a core's share pulls the albedo toward the structure's lightness (the rock's material over 0.4 of it) and, where a cell is under 300 m,
a way's share darkens it (a railway's ballast) or pales it (a highway's slabs). The whole-world map draws nothing of it: a core is a
thousandth of a texel there.
