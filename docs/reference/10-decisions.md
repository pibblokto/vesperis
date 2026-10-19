# Decision log

| Decision | Why | Consequence |
| --- | --- | --- |
| Software rendering at 320x200 with 4 banks x 64 shades and the psmooth_64-style filter | It is the source of the Noctis look: smooth gradients that melt at edges without colour bleeding | Only four hues per scene; scenes juggle banks; HUD must be drawn after the filter in RGB |
| Double precision everywhere, including noise | Galactic coordinates (1e13 km) and metre-scale terrain in the same numbers; float would lose sub-metre detail on a 6000 km sphere | Slightly slower noise; deterministic on one machine |
| raylib isolated behind `platform.h` | raylib's `PI` macro and `KEY_*` enums collide with game code; keeps the game testable headless | Two translation units touch raylib; key codes are mirrored by number |
| One planet function for orbit and surface, with LOD gating | Guarantees that what is seen from orbit is what is walked on | Every feature must be expressed as a function of the sphere point; fine features fade with `lodFade` |
| Terrain caches as toroidal tag tables computed on demand | Unbounded terrain with no explicit streaming | First frames after landing compute many samples (hitch); can be prefetched later |
| Azimuthal equidistant local mapping around the site | Exact and pole-safe; local x/z stay metres | Beyond ~100 km the site frame is no longer the local vertical |
| Circular orbits, compressed scales (first orbits ~10-50 star radii, moons 3.5-6.5 planet radii) | Bodies visible as discs from each other, suns visibly large; periods observable with time warp | Not physically scaled; temperatures use a fictional AU |
| Real-time durations for flights, approach, descent; game time only for orbits and rotation | Time warp must not make transitions instant or skip them | Two clocks (`realTime`, `t`) |
| Flat-shaded quads with a world-mapped grain texture | Matches the original's `polymap` look; the mush softens steps | Needs perspective-correct grain coordinates (affine caused stripes) |
| No fuel, no damage, no fail state | The brief: pure exploration with no goal | Radiation is only a warning |
| Sector = one in-game light year (1.5e10 km) | Keeps star systems (up to ~1e10 km) from overlapping neighbours while reading naturally in the HUD | Distances are fiction |
| Vegetation as quads clusters rather than point sprays | Fast first implementation | Blocky (KI-001), scheduled for M1-01 |
| Uppercase-only 5x7 font | Small, legible at 320x200, Noctis-like | Names look shouty; lowercase not available |
| 16-bit pixels, 16 banks x 12-bit intensity, runtime resolution 1x-4x with mush box = scale + 1 (2026-09-26, M10-01/03) | Richer textures and lighting need more ramps than four; 64 intensity steps banded the sky; resolution as a setting instead of a build variant | Every writer uses bank indices and fractional shades; HUD in logical units scaled by the canvas; sheets in `shots/scale_*.png` |
| Generation version 2 (2026-09-26, M9): plates, rivers, biomes, seasons, four new types, tuned distributions; `tests/regress_baseline.txt` re-blessed | Every world changed shape; saves record `gen` and warn once; star positions are integer hashes and did not move | Old saves keep their star and system, the surfaces differ |
| Rivers as a hashed worley network with local water levels instead of a traced flow map (2026-09-26, M9-04) | A pure function of position with no global simulation, consistent at every detail level, cheap | Rivers can cross ridges; banks and lakes look right at walking scale; a flow map can replace the network later |
| Flat cells stay the default terrain shading; the quantised gradient is a setting (2026-09-26, M10-12) | The flat polymap look is part of the character; the gradient reads smoother but more modern | Both selectable; sheets can be made with `surface` at either setting |
| Cast shadows never go below 40% of direct light (2026-09-26, M10-06) | Physically airless shadows are black, but at low sun the frame became unreadable and lost the original's bright sunsets | A softer, more readable picture; a future "hard shadows" toggle is possible |
| Default render scale 2x, 3x3 mush (2026-09-26, M10-02) | The sheets show 2x/3x3 keeps the melt with twice the definition; 3x and 4x go painterly and cost 2-4x more | 1x stays available in the settings; 3x/4x optional |
| HD prototype removed (2026-09-26): the compile-time `FB_SCALE` build, the variable mush box and the `compare`/`sheet` harness are gone; `shots/compare*.png` stay as the record | The user does not want two parallel code paths (HD and non-HD) in the tree while the richer picture is a separate, not yet started milestone (M10) | One resolution, one code path; M10-01 re-adds resolution as a runtime setting; experiments are removed once their decision is recorded |
| Generation version 3 (2026-09-26, M5): companion stars, substellar objects, comets, Kepler orbits, double planets, closer and bigger first moons; `tests/regress_baseline.txt` re-blessed | The astronomy milestone changes what a system is; eccentricities and companions are drawn from separate streams so most existing bodies keep their looks, but moons and the cold band shift | Old saves point at systems that now hold more bodies; the guide records `gen` so a friend's names can be matched |
| Two suns light a surface through one blended `env.sun` rather than a second light in every shading path (2026-09-26, M5-01) | 24 shading sites read `env.sun`; blending direction by strength and summing the factor gives the colour mix and the brightness with no per-quad cost; blob shadows are doubled where it shows most | Terrain cast shadows follow the blended direction, so at two equal suns they point between them (KI-020) |
| R-001 sky targets left partly short (2026-09-26, M5-09): over 1 deg 39% (target 45-50), over 3 deg 33% (35), over 10 deg 18% (12-15); superseded 2026-10-18 by B-428 | The user asked for "more often but not too often"; the >10 deg share already passed the target and going further would make the big-parent view ordinary | Can be revisited by moving the first moon closer still; B-428 went the other way (a system with distance from the ship, the user's choice): over 1 deg 31%, over 3 deg 17%, over 10 deg 3% |
| Key remapping as a text file of physical-to-logical pairs, not an in-game rebinding screen (2026-09-26, M6-02) | The game code names keys directly in 200 places; remapping at the input layer covers layouts (AZERTY, Dvorak) with no refactor and stays testable headlessly | No per-action binding UI; the help text still names the default keys |
| Gamepad mapping fixed in code (2026-09-26, M6-02; superseded 2026-10-17 by R-418: the controller read raw and bound by role on a screen, a layout a context, the command menu) | One sensible layout, documented; the key map file can still move letters | Not rebindable |
| Warnings as errors only in `core/` and `galaxy/` (2026-09-26, M7-06) | Those layers are the deterministic ones and small enough to keep clean; the renderers and the game carry long one-line style that trips `-Wextra` noise | `space/`, `surface/`, `game/` build with warnings visible, not fatal |
| Web build written but not run (2026-09-26, M6-04) | No emsdk on the development machine; the code path is small (`step` callback, IDBFS mount) and mirrors raylib's own web template | KI-021: first run may need fixes |
| Frame hashes must match exactly (2026-09-26, M7-04) | The renderer is deterministic across runs on one machine (integer hashes, every pass row-pure or band-pinned so the picture does not depend on which thread draws a row or how many there are (F-06: the rows are dealt, no longer fixed to threads), maps warmed before hashing); a tolerance would hide real regressions | Another compiler or CPU may hash differently (KI-014); bless per platform if CI ever runs the frame hashes elsewhere |
| Generous performance budgets (2026-09-26, M7-04) | CI runners are 3-5x slower than the development Mac; the budgets catch a pathological regression, not a 20% slowdown | Small slowdowns are read from the printed timings, not enforced |
| One material palette per body and one exposure rule in every view (2026-09-26, N1, B-202): `BodyGen::matColor`, `materialRamp`, `exposureStop`; the ground's curve (`50 clamp(0.45 + 0.75 a) light^0.65`) became the common rule; frame hashes re-blessed | The globe, the map and the ground were tuned separately and disagreed on every world; the ground's curve was the one people look at longest | Globes are a little darker for dim stars than before; icy worlds show dark rock crack zones on the ground where the globe shows them; the venusian globe stays its cloud deck, so the consistency test compares its map (`C`) with the ground only |
| Forest frame budget 14 ms at 2x instead of the 6 ms aimed at (2026-09-26, N2-04) | A tropical forest with giants is ten trees deep; after tiers, point budgets, solid mid-distance blobs and skipping trees behind filled canopy tiles the densest case measures 12 ms (trees 5.8 ms) and the look was not worth cutting further | The worst-case forest frame is 12 ms on an M4 (KI-206); plains stay at 3.4 ms; 1x remains the escape on slow machines |
| Trees hidden behind filled canopy tiles are skipped by a coverage grid, not by a depth prepass (2026-09-26, N2-04) | Cheap (a 40x25 count grid) and good enough with near-to-far order; a prepass would need the sprays drawn twice | A far trunk visible between near canopies can be missing when its canopy is covered |
| Seasonal leaf colour from the season value alone (2026-09-26, N2-05) | The site knows the season phase, not its direction; scanning the orbit per frame is not worth it | Spring and autumn shoulders both show autumn colours (KI-208) |
| Encounter chances per biome rather than a herd at every landing (2026-09-26, N3-04) | The request asked for life on 70% of temperate and tropical landings and rarer elsewhere; ice and open sea get nothing | A landing can be empty of animals, and most random sites on the cold living worlds near the start are ice, so the overall rate over random sites is 11% while living biomes reach 75-100% |
| Creatures as boxes and lines, not point sprays (2026-09-26, N3-02) | Articulated bodies need silhouettes that hold their shape while walking; sprays melt into blobs | Animals look like carved wood next to the sprayed canopies; a herd costs 0.05 ms |
| Small rocks are bumps, not walls (2026-09-26, N4-02) | With rocks as colliders a buggy at 100 km/h stopped every few hundred metres on a felisian plain | Rocks under 0.9 m cost 12% of the speed and a shake; larger rocks, trunks, logs and ruins still stop it |
| The long drive test runs on a desert site with a scripted driver, not in the flow (2026-09-26, N4-02) | The flow's landing is forest for 800 m around and no straight run exists; a desert has no trees and no rocks on sand | `vesperis_test drive` is the acceptance; the flow keeps a short smoke drive (KI-211) |
| Generation version 4 (2026-09-26, N5-03): gas giant maps carry their storm term in the veg channel; `tests/regress_baseline.txt` re-blessed (the shore frame, whose parent is a gas giant) | Storms that evolve need the storm term apart from the bands at render time; the maps' albedo is unchanged | Old saves report the regeneration notice once; only gas giant maps' unused veg channel differs |
| Visor glints as HUD arcs, surface only (2026-09-26, N5-02) | The sun's screen position is known on the surface; drawing into the framebuffer would need a bank and the mush would melt thin arcs | No glints from the ship's cabin (KI-212) |
| The web build and the CI run stay undone (2026-09-26, N5-04) | No emsdk on this machine and no git repository or remote; both need the user | KI-021 and KI-214 |
| The buggy is closed and driven through its nose camera, never a drawn cabin (2026-09-27, N4-06, R-204) | The user asked for a closed futuristic vehicle whose only inside view is the front camera's picture; the drawn cabin had shaken (B-203) and its wheel was wrong (B-204); a pan limit of +-70 deg keeps the driver from looking back, which confused | No cockpit geometry from inside (the hull is skipped when the camera is live); reversing uses the outside view (KI-215); the CCTV pass costs about 0.3 ms at 2x |
| The capsule always answers `K` and `B` always gives a fresh buggy (2026-09-27, B-205) | The 200 m rule and the one-buggy rule stranded the player whenever the buggy state went stale; the user wants the capsule and a new buggy to be always available, the old buggy simply gone | An abandoned buggy stays where it was until replaced (it is on the sector map); the launch log counts the metres driven over every buggy of the landing |
| Cloud drift as an integral of the wind, not `wind x t` (2026-09-27, B-206) | With t at 3.6e6 s a change of 0.1 knots moved the clouds 180 km; the integral is continuous and seeded to the old value on the first frame so the regression frames did not change | The drift is per `SurfaceView` instance and not saved: after a load the clouds start from the formula's value again |
| Prefetch on a worker thread through a staging buffer, the caches written only by the main thread (2026-09-26, M7-01) | No locks in the render path; the sampled function is pure so the worker's results are identical | The job is at most one frame stale; cells the main thread already computed are skipped. Since F-03 (2026-10-13) the pool's workers write them too, inside the warm pass's joined fill, each cell on its own slot |
| The ring arc mapped to the bright part of the sky bank, like the moons (2026-09-27, O0-01, B-301) | The sky bank's ramps are black to shade 30 and dark to 44; the old formula peaked at 39, so the rings were there and invisible | Rings read as a pale band by day and bright at night; the unlit face is half as bright; the shadowed part is not drawn |
| Generation version 5: rings on 12% of solid planets (2026-09-27, O0-01) | The user could not find a ringed world to land on; the same random draw with a higher threshold changes nothing else in a system, so the generation hashes did not move | One solid world in eight wears rings; old saves warn once |
| The curvature lives in `toView`, the far ring reaches 53 km and the haze opened (2026-09-27, O1, B-302); frame hashes re-blessed | Only the terrain mesh curved; objects sat on a flat plane 0.1-25 m too high with distance (a rock at 300 m on a comet 12 m up); the 13 km ring with a 5 km haze hid what you were told to walk to | Every distant frame changed; airless worlds show 45 km of ground; surface 2x costs 3.9 ms (3.4 before) |
| Sectors are the one-degree cells of the grid, named `LON:LAT` everywhere; the landing map zooms into one (2026-09-27, O2, R-301) | A sector had to be the same thing on the landing map, the HUD and the sector map, big enough to be a region (105 km) and drawn large enough to pick a site in; the zoom samples the far ring's detail so the picture is what you will see | 360 x 180 named sectors per world; the old 360 x 120 readout is gone; the zoom costs about 30 ms once per window |
| Belt rocks in rings that turn at their own rate, not one co-rotating cell space (2026-09-27, O3, R-302) | At t = 3.6e6 s rings 400 km apart have sheared by hundreds of cells; hashing cells in one frame lost every rock but the parked one | A ring keeps its formation (its rocks move together); rings shear against each other; the same rock is found at any time |
| Belt rocks share palette bank 3 (2026-09-27, O3) | All sixteen banks are taken in space (stars 0/4/5, sun 1, bodies 2/3/6/7, families 11/13/14/15, companion 12, cabin 8/9/10); bank 3 is free whenever no second world has a visible disc, which is the case inside a belt | With a second world in view the rocks take its rock colour (KI-303) |
| No landing on belt rocks yet (2026-09-27, O3) | The user said landing on nothing is fine for now; the small-body surface code of O4 would carry it later | `C` in a belt is refused with a message |
| Comets keep honest microgravity with caps, not fake gravity (2026-09-27, O4, R-303) | The user asked for the small size and its effects; capping the ground speed and the jet under the escape velocity keeps you on the nucleus, `Ctrl` brings a floating jump down, the buggy would float off so it refuses | Jumps float for minutes (KI-302); the buggy is unavailable on comets |
| The harness writes to `shots/tests/`, the player's screenshots stay in `shots/` (2026-09-27, O7-01) | Three hundred test frames had buried the player's own pictures; the tests regenerate everything they write, so the old files were deleted rather than moved | Paths in older reports and notes omit `tests/`; the CI artefact path changed |
| The buggy's top speed is 50 m/s, 180 km/h (2026-09-27, O1-05) | The user asked for 180; the drag term (`0.0011 v^2`) needed a torque curve that still pulls 3.4 m/s^2 at the top, and the gears moved to 12 and 28 m/s | On the desert run the test reaches 169 km/h; the rangefinder's drive estimate assumes a 32 m/s cruise |
| Six sector-map levels up to 2048 km, offered by the world's radius (2026-09-27, O1-04) | The user wanted to zoom out further; the azimuthal-equidistant frame is exact but distorts past a radius or so, so a level is shown only while its half-width is within 1.2 radii | A comet has two levels, a planet six; the widest ones sample the planet function at 4-16 km |
| Five-bit banks: 32 palette banks of 2048 intensities instead of 16 of 4096 (2026-09-27, O0-02, B-305) | Two worlds with discs in one frame (a moon and its parent) shared the forest/sand/snow/grass banks, so the smaller borrowed the bigger one's colours and a world changed colour as you approached its neighbour; every other fix (blends, ownership rules, one-bank fallbacks) still flipped; 32 units per shade are plenty under the mush | Body B owns 16-19, belt rocks own 20, eleven banks are free; frame hashes re-blessed; the surface bank table is unchanged |
| A globe is culled only when its whole sphere is behind the camera (2026-09-27, O0-03, B-304) | The centre test dropped a parent that fills a moon's sky the moment the camera looked at the horizon; testing every pixel of the frame when the centre is behind costs a full-screen ray test only in that case | No change for small or distant bodies |
| The landing map carries the shadows of moons, parents and rings (2026-09-27, O0-04, B-306) | The globe showed a moon's shadow crossing a planet and the map did not; the same rule per texel, cached and refreshed twice a second | About 2 ms per rebuild; the shadow moves in half-second steps at high time warp |
| Terrain III is planned, not started (2026-09-27, O5) | The user asked for investigation and a plan; the measurements (`vesperis_test terrain`) show felisian ground at a 4% median grade and 76 m of relief over 2 km | `docs/iteration-3/plans/PLAN-terrain-3.md` with targets, techniques, seven items and three open questions |
| Generation version 6: the relief spectrum (2026-09-27, O6-02); `tests/regress_baseline.txt` re-blessed | The living worlds were the flattest (felisian: a 4% median grade, 76 m over 2 km); the fixed-wavelength hill, ridge and detail terms fell off far faster than natural terrain; one self-affine sum with analytic derivatives (slope-damped octaves, hybrid weights, ridged first octaves, two warps, a sediment floor) is the cheapest way to mountains at 25-40% grades with the eroded look, and it stays a pure function gated by `lodFade` like everything else | Every solid surface changes shape (the maps of the airless types do not); old saves warn once; felisian mountains 0.30 median, 576 m over 2 km, 1469 m over 16 km; the cost 1.0-1.8x; the airless maps' hashes did not move |
| The near ring at 4 m fades out with speed instead of following the buggy (2026-09-27, O6-02) | A buggy at 20 m/s would sweep 750 new 4 m cells a frame; on foot and under 12 m/s the ring costs 0.4 ms; blending the near vertices and `groundHeight` by the same weight keeps the feet and the wheels on the drawn ground while the relief melts into the 16 m cells | Above 20 m/s the buggy drives on lod0 as before; the sprint's worst frame is 2.7 ms |
| The relief class is the regional mean grade, cut at 0.05 and 0.2 (2026-09-27, O6-02) | The steepest of eight probes made a flat site with one crack or crater wall "MOUNTAINS"; the mean of directional slopes over 150 and 600 m, scaled by 1 / 0.64 to a gradient, describes the region, which is what the class is for; the cuts moved so the classes match the plan's targets (hills 0.08-0.15 median, mountains 0.25-0.45 at 16 m) | Class shares on felisian land: 60% flat, 27% hills, 17% mountains near the start; `consistency` accepts a boundary flip within 30% |
| Terrain targets are owned by items (2026-09-27, O6-01) | `vesperis_test terrain` judges every target of the plan from the first step, but cliffs, broken plains, mountain floors and crater floors are the landform library's and the drainage's work; counting them would fail every run until then | They print as `FAIL(O6-0N)` and are not counted; the exit code follows the spectrum's targets only |
| The plains stay flat by class (2026-09-27, O6-02) | The user's direction: cinematic realism, the flat classes flat on purpose, broken later by terraces, gullies, dunes, outcrops and playas (O6-04/05); the base amplitude of 100 m per 16 km gives a 1-2% median grade so a plain is not a billiard table | Coastal plains and lake basins are still nearly featureless (the land mask and the basin damping); the "broken" targets wait for O6-04 |
| Caves and overhangs are out of scope for O6 (2026-09-27) | A height field cannot carry them; the user chose to leave them to the next iteration (a second height field or displaced meshes) and asked that nothing in O6 block them | The pit chains and cave mouths of O6-05/O6-06 stay surface features a later milestone can open |
| The surface's light-factor floor is kept (2026-09-27, O6-02, KI-309) | `SurfaceSite::sun` clamps the star's light factor at 0.62 so the ground of a dim star stays readable; the globe and the map use the raw factor (0.35 for Skeatoltdos); on rock the gap stays under the consistency threshold, on snow it is 105 | `consistency` reports 1 of 13 over since the felisian test site moved onto snow; aligning the views is a lighting decision for its own milestone |
| Default render scale 4x with the "fine" 3x3 mush (2026-09-27, B-310; supersedes the 2x / 3x3 default of M10-02) | The user found 2x "super mushy" after the O6-02 relief; the sheets of `compare`/`sheet` (`shots/tests/scale_*.png`, the six configurations as the 1280 x 800 window shows them) put D, 4x with a 3x3 box, ahead: one framebuffer pixel per window pixel, the melt kept at the pixel level, leaves and ridges resolved | 4x costs 2-2.5x the pixels of 2x; the surface and the flora were made band-parallel on a persistent pool (surface 4x 10 ms, the dense forest 14, the buggy 9, space 13); 1x-3x and the other boxes stay in the settings; a version-1 settings file takes the new look once |
| Smooth terrain shading by default (2026-09-27, B-311; supersedes "flat cells stay the default" of M10-12) | With the 4 m near ring every cell had its own flat lambert shade from the fine relief and the ground became a patchwork of plates, one of the user's "ugly squares"; the gradient keeps the character through the grain, the tiles and the mush | Flat cells (0) and the 12-step gradient (1) remain in the settings; the regress frames were re-blessed |
| Texels read bilinearly when they span over two and a half pixels; material boundaries as noisy contours through two-material cells (2026-09-27, B-311) | Nearest texels of the grain and the tiles were 20-45 pixel squares within a few metres at 2x and worse at 4x; the majority bank per cell made sand, rock and grass meet in staircases | About 2 ms more per 4x surface frame for the bilinear reads (the tone tile moved to the vertex shade for that reason); the far rings carry the tiles stretched to two cells |
| The sun is drawn by angle per pixel, with a brightness floor at its glow (2026-09-27, B-308) | The screen-space circle's radius grows without bound as the star's centre nears the camera plane: a companion star flooded the frame when the camera swung past it; and a dim low sun inside a saturated corona read as a ring | The whole frame is tested when the cone reaches the camera plane (rows in parallel); the rays of compact stars stay screen-space; the space regress frame changed |
| The visor glint arcs are gone; a veiling glare in the framebuffer instead (2026-09-27, B-309; supersedes N5-02) | The arcs spun when the sun sat near the screen centre and wobbled with every head bob, and crisp HUD lines did not sit in the melted picture; a broad faint glow drawn before the mush melts in and fades smoothly behind ridges and canopies | KI-212 is moot; `stability` measures frame-to-frame change per region |
| The buggy leaves the ground only above four g of asked-for downward acceleration (2026-09-27, B-307) | The 3 m/s rule launched it horizontally on every steady descent above 10 m/s (a ladder of hops and thumps); the honest limit is one g, but a height field's cell creases are sharper than real ground, so the suspension is allowed to follow four g (at least 40 m/s^2, so light worlds still hop) | Crests at speed still jump (the vertical speed is kept); landings are judged relative to the slope; `descent` guards it |
| Band-parallel drawing with identical output (2026-09-27, B-310) | The rasteriser is serial; splitting the frame into bands of rows per worker keeps every pixel's write order, so the frames are bit-identical to the serial loop (the regress test proved it), where splitting primitives among threads would race on the depth test | The vertex caches and the tiles are filled before the bands (in parallel by rows of cells); the per-cell and per-tree setup is repeated per band, so the gain is 2-3x rather than 8x; the trees are listed once with their rows to cut the repeats |
| The forest bench at 4x is budgeted at 24 ms, the surface 4x rows print their sections (2026-09-27, B-310) | The densest scene is the honest budget of the new default; the sections tell where a regression is | `VESPERIS_THREADS=1` gives the serial baseline for comparison |
| A globe's box is the exact conic extent of its sphere plus the ring's projected circle (2026-09-28, B-312; completes O0-03/B-304) | A box of radius `f tan(a) d/z` round the projected centre is a circle, the projection of a sphere off axis is not: with the centre ten degrees outside the frame the box was empty and a huge parent vanished with its rings | `sphereBox`/`ringBox`; the whole frame when the sphere or a ring point reaches the camera plane; `unit` holds the case |
| Sky bodies write their depth in metres (2026-09-28, B-317) | The space renderer's kilometre read as metres against the terrain: a moon 20,000 km away hid the ground beyond 20 km | The sun and the stars test for any depth, so eclipses and the star mask are unchanged; `unit` bounds the nearest sky depth |
| The first moon at 4.5-6 radii and outside the rings, generation 7 (2026-09-28, B-314; supersedes M5-09's 2.8-4.2; a solid world's since B-428, generation 15, at 5-40) | The user asked for a cap: from 2.8 radii a parent's disc was 42 degrees across and a moon inside the rings saw them fill the sky; the same random draw keeps the rest of every system | Every moon's orbit and period moved (saves get the regeneration notice); 26 degrees across is the biggest parent now (a giant from its first moon) |
| Geomorphing at every terrain ring edge, the band from the finer ring's cover to the edge (2026-09-28, B-313) | Each ring samples the planet function at its own detail, so a coarser ring is a smoother terrain, up to 1015 m off at the 13 km edge: cells popped as the edges swept over the ground; a vertex now slides onto the coarser surface (height and shade) by its continuous distance from the camera; a six-cell band twisted the cells into a sawtooth skyline, so the band spans most of each ring | The far ring's swell rises into its ridges over 10 km as you approach (KI-318); the rings are warmed coarse to fine, overlap by one cell with a depth bias; `unit` requires a zero gap at every edge; the lodFade thresholds themselves are O6-07's question |
| The near ring at every speed; the buggy rides on a suspension (2026-09-28, B-313; supersedes the speed fade of O6-02) | The fade was the visible morph on getting out of the buggy; its reason (hops over 4 m bumps at 180 km/h) is met by averaging the hull's reference height over 16 m along the way within the wheels' travel | Rough ground at full speed still launches the buggy where the averaged ground asks for more than four g; `descent` and `drive` unchanged |
| Leaf clusters are camera-facing cutout polygons; bark and cut wood own banks 16 and 17 (2026-09-28, B-315; supersedes the point sprays of N2) | At 4x the sprays were sparse pixel clouds; a ragged disc cut by a leaf-edge noise, shaded by its place in the canopy and textured, reads as leaves at every scale; on the ground bank the trunks came out black | A canopy from straight above is a stack of discs (KI-316); the depth test before the texel reads and the 16-vertex clipper keep the forest at 12.7 ms at 4x |
| Habitat cells for herds (2026-09-28, B-316; supersedes the landing-only spawn of N3) | Herds lived within 180 m of the capsule and the re-anchor cleared them; hashed 512 m cells of latitude and longitude put them wherever the biome has them, spawned within 900 m and forgotten beyond 1300 m | A forgotten herd respawns at its spot when you return (KI-317); the grassland site holds nine herds |
| Generation version 11: the galaxy of 200 billion stars (2026-10-01, G-01); `tests/regress_baseline.txt` re-blessed | The first milestone of `docs/ideas/MILESTONES.md`: a disc with the Milky Way's proportions, home kept at its sectors by putting the galactic centre 13,050 ly away; every star's existence, class and name can change, so the whole G/S series is this one bump | A save whose star is gone does not load and says so; one whose star survived says `THE GALAXY WAS REBUILT`; the four pinned harness bodies exist at the new density; the flow stops comparing the hop's star name (Skeatoltdos) and approaches unless its comet case parked the ship |
| Generation version 12: each giant's character on its globe (2026-10-07, X-03); the regress unchanged | A descent must fall into what the globe shows (the consistency principle): each giant's share of storms, an ice giant's faint bands and a great storm on one giant in four are the planet function's, so the telescope and the probe see the same | Only the globes of gas giants and brown dwarfs change (no system, star or solid surface moves); a 1.1.0 save loads with `WORLDS WERE REGENERATED SINCE THIS SAVE` |
| Generation version 13: the ages (2026-10-08, A-01); the regress unchanged | The civilisation expansion of 1.3.0 is one generation change from its first milestone: every people's age, eras, end and collapse (`galaxy/peoples.*`) are what every later milestone reads, and only the peoples that had radio transmit | The peoples' worlds' ruins weather by their age's years (Aieliaalas II 0.92 to 0.91, Leileashphail III as before); the transmitters moved (the cells' rate and salt); a chart's heard marks follow; systems, stars and surfaces do not move; a 1.2.0 save loads with `WORLDS WERE REGENERATED SINCE THIS SAVE` (the found shards by their index, T-04) |
| Generation version 14: the companion stars keep clear of the other sun's worlds (2026-10-18, B-425); the regress unchanged | A red dwarf's red giant companion came within its own radius of three felisian worlds (Yaiphuyiabrus, the user's report): M5-01 drew a far companion's orbit from the primary's widest orbit alone, before S-01 brought the red dwarfs and the giant varieties, and the worlds' warmth is the primary's alone | The companion's orbit moved in 56% of the multiples (11.5% of the stars, a median 2.4 times out), 3,383 close companions of 160,778 stars are far now, a companion's worlds new or changed in 3,053 systems (2,520 a close one's gone far); every primary's worlds and the stars do not move, nor any body's index (a far companion's new worlds come after every other body); a 1.3.1 save loads with `WORLDS WERE REGENERATED SINCE THIS SAVE` |

## O10 (2026-09-28): the third review

* **Generation version 8 in one bump.** The climate (B-321), the rivers and lakes (B-320), the traits and landforms (R-304/R-305) and the plains' relief (R-306) all change every solid surface; one version, one re-bless, one note in the saves. Systems and star positions do not move.
* **A greenhouse on living worlds only.** `tempBias` adds 33 K over the equilibrium temperature for felisian worlds (Earth's), the other types keep their bare equilibrium: a thin-air world stays cold. The alternative, warming every type, would have moved every HUD temperature for no gain.
* **Rivers are still hashed networks, not drainage.** O6-03 (flow accumulation on the map worker) stays on the roadmap; the ribbons cut into a smooth ground fix what the user sees (squares, bumpy sheets) at a fraction of the cost and keep the map/ground consistency rule (the same function at every detail). Rivers do not yet run downhill from ridge to sea.
* **Lakes are features with one level, computed at the centre.** A level from any local function tilts across the basin (the 8 km octave alone slopes up to 10%); evaluating the felisian ground at the lake's centre costs one extra evaluation per lake per thread (cached) and gives a flat sheet. The same idea is available for the rift lakes and the calderas later (the caldera lake is still a draped sheet).
* **Traits over new types.** Thirty-one traits on the sixteen types give more variety than new types would, at a fraction of the code, and they combine (a chalk-white world of canyons and salt flats; an icy world with a rift, brine seas and patterned ground). The draw alternates between landform and world lists so felisian hydrology and flora traits are not crowded out by the eighteen landforms. Eligibility is a table (`traitEligible`), not a weight matrix: simpler to read, the distribution printed by `traits`.
* **Landforms are additive terms with their own LOD gates, not a rewrite of the spectrum.** Each one is 15-40 lines in `landformsAt` with a province or climate mask, `dhSmooth`/`dhFine`/`flatten` and a material; the smooth ground `hS` carries the big ones so the water follows them. The price: the fine ones (terraces, polygons, yardangs, spires) exist only in the 16 and 4 m rings (KI-320), to be given coarse stand-ins in O6-07.
* **The far-tree flicker is reduced, not abolished.** A one-pixel trunk line and a texture under parallax cannot hold still without anti-aliasing; the cutout and tile fades remove the sparkle, the walking metric keeps the rest as a number.

## O11 (2026-09-28): the fourth review

* **The water's edge is cut into the mesh, not smoothed into the planet function.** Making every bank a slope gentle enough for the 512 m ring to interpolate would have flattened every river into a ramp hundreds of metres wide; the cut costs a few triangles on shore cells only and gives the exact contour of the outline field at every ring. The price is a second field per sample (`shore`) and the morph having to carry it, with the clamp closing to the cut's lip at the ring edge so the seams stay exact.
* **Sub-cell water fades across the morph band instead of popping at the edge** (KI-323). The finer ring's edge morphs onto the coarser ring's, which cannot see a lake under 1.2 km at 512 m detail; the lake thins toward the edge and is gone at it. Keeping it to the edge would pop it there; a coarse stand-in per lake is O6-07's.
* **One depth bias for a ring's ground and its water.** Coincident layers of one ring must be compared unbiased against each other; the bias exists only to rank rings.
* **Six new types over more traits this time.** R-304 chose traits over types for variety at low cost; the user asked for types with their own events (lava geysers, water geysers), which traits cannot give: a type owns a planet function, a sky, an environment and an event. The type table changes every system (GEN 9) - the saves say so.
* **Two type groups instead of per-type branches.** `hasOpaqueDeck` and `isLavaWorld` replace fourteen `== PT_VENUSIAN` and `== PT_MOLTEN || == PT_VOLCANIC` tests, and `typeSky` replaces three copies of the sky table; a new type touches one list.
* **The flying capsule is not drawn.** During the descent and the ascent the capsule is the camera; drawing it (and its beam) on the ground was a leftover that only showed from above.
* **The consistency rows under dim stars stay over the threshold** (KI-325): the lighting decision of KI-309 is unchanged, the sample bodies moved with GEN 9. The acidic sea's row measures the map's liquid against the ground's Fresnel body colour, which the ocean's row also shows in small.

## O6-03..O6-07 (2026-09-28): drainage, landforms, landmarks

* **The drainage is a precomputed layer of the planet function, per tile, not a global map and not a per-sample search.** A flood needs the whole basin; a tile of one degree with a 0.3 degree margin routes every river that matters to a landing, blends its estimate with the neighbours' over 0.2 degrees, and costs 100-200 ms on a thread once. The price: a river crossing a tile border far from any landing may show a small step in level at the border (KI-328), and a basin cut by the domain edge is breached, never a lake (KI-329).
* **Lakes are kept by rule, not by size alone.** Every closed basin was a lake at first (40-49% of the windows water); breaching them all left none. A basin under 30 km^2 stays a lake two times in five by hash; the rest drain through a breach of `60 + 12 sqrt(area)` metres at most, so a big basin becomes a gorge of a hundred metres, not a 1.6 km trench.
* **Rivers are routed on a noised surface and drawn as curves.** D8 on the raw heights produced dead-straight combs over plains; a metre of noise on the routing surface, jittered node points, Catmull-Rom reaches and a meander swing on flat reaches give rivers that look routed, at the cost of a neighbourhood rebuild per lattice cell (the 512 m and 2048 m rings sample a new cell every time: 3-9 us a sample; the 2048 m ring reads the raw cells with straight reaches instead).
* **A river never narrower than 1.6 samples.** A sheet drawn on cell corners breaks into blobs when the channel is narrower than the cell; the half-width is `max(w/2, 0.8 detail)`, so a river looks wider from the air than on foot (KI-322's line), and streams under `0.05 detail` fade out at that scale.
* **The valley floors are flat and the plains keep their grade by a floodplain relief.** A V to the channel left no floor a buggy could follow; a flat floor of ten widths (30-350 m) either side of every stream and brook, varied by a slow noise, gives the valleys their bottoms. Over a plain the flat floor took the median grade under the class's 1%; bars and levee scars of a metre on 90 and 30 m outside the banks put it back. The mountain rows still read 2% of floors against the plan's 15% (KI-330): brooks 1-2 km apart with 80-160 m floors cannot flatten 15% of a 2 km window's lowest quarter, and a floor wide enough to would flatten the mountains.
* **Cliffs where the spectrum is steep, anywhere.** The cliff bands read the spectrum's own slope (the only gradient the function has cheaply); the crater-field types' mountains are basin walls with a low spectrum slope, so their cliffs come from the crater terraces' scarps instead (risers over a sixth of a terrace). Thin-atmosphere mountains stay at 0.1% of cliffs (KI-330).
* **Landmarks are found, not placed.** A grid cell of 14 km holds the biggest thing the terrain already has there, in a fixed order of kinds; a search costs about a millisecond a cell with the tiles warm and runs on a thread at every landing, so the first frame does not wait for it. Ruins are the one placed kind (5% of a 2 km grid's cells), the "payoff up close" the plan asked for where the terrain has nothing. (R-408, 2026-10-07: the sights are found as before but are no longer markers: each kind is read on its own as the scanner's echoes and the maps show the explorer's own marks; see R-408.)
* **The landing map never computes drainage tiles on the main thread.** The cursor's prefetch runs on a worker, the site probe samples without the drainage until the tiles exist, the zoom is drawn dry and drawn again when they do. A landing a second after the cursor moved still waits for its tiles in `SurfaceView::init` (100-200 ms a tile); the bench measures that wait.
* **Generation version 10, the last before the release.** Heights, water and materials change on every solid surface; systems and star positions do not. Saves and the guide say so (`WORLDS WERE REGENERATED`). Caves and overhangs stay out (no design agreed); release preparation is a separate item.
* **The descent budget is 80 ms.** The first frame fills four rings with 12,000 samples, a fifth of them drainage, at 50-80 ms on the development Mac; it runs under the fade from black. The old 60 ms guarded a frame without drainage.

## Release (2026-09-29): Vesperis

* **The project is Vesperis.** Binaries `vesperis` and `vesperis_test`, files `vesperis_save.txt`, `vesperis_settings.txt`, `vesperis_keys.txt` (headers `vesperis-save 2`, `vesperis-settings 2`, `vesperis-guide 1`; a file written by the noctis-clone tree is read once its name and header line are renamed: no player had one), env `VESPERIS_TRACE`, `VESPERIS_SKIP`, `VESPERIS_NODRAIN`, `VESPERIS_THREADS`, CMake options `VESPERIS_*`. The title screen carries the name and the choices only; the credits page names the inspiration (Noctis IV) and nothing else. The public repository is `pibblokto/vesperis`, without this tree's history or `shots/`.
* **Nothing has a name until the explorer gives one (R-401).** `starNameOf`, `bodyNameOf`, `beltNameOf` and `landmarkName` return the guide's (or the inbox's) name, else `UNKNOWN` (landmarks: an empty name, shown as `UNNAMED PEAK`); the generated names stay inside the generator, where the keys, the landmark ids and the harness (`testStarGenName`) still use them. The derived names of a renamed star's worlds went with it: each world is named on its own. The save file's `name` line carries the display name.
* **The maps show only the sights already found (B-401; superseded by R-408: the maps show the explorer's marks).** A fresh landing put every landmark of the drawn disc on the sector map (glyphs and names at 8-128 km: a mush). The sector map and the landing zoom now draw a landmark once it is in `landmarksSeen` (logged within a kilometre, or named), the sector map the nearest 24, names only when the explorer gave one. Removing them altogether was the alternative; finding them on foot keeps the log and the guide's naming meaningful.
* **The sector grid's step follows the zoom (B-402).** One-degree lines at 2048 km wide were 10 px apart. The step is the first of 1, 2, 5, 10, 15, 30 degrees that keeps the lines 24 px apart; the one-degree cells carry the sectors' names when they are 46 px wide, the coarser lines their latitude and longitude along the map's edges.

## Release 1.1.0 (2026-10-03)

* **1.1.0 is everything since 1.0.0, nothing held back.** The user drew the line after C-14: generation 11 (G, S), the C series but C-11, the aurora's two items. The milestone index (`docs/ideas/MILESTONES.md`) marks every row `1.1.0` or `1.2.0`, so the next release's scope is read from the index, not from the progress log. The build recipe is 1.0.0's (static raylib 5.5 per target, Ubuntu 20.04 images for Linux, llvm-mingw for Windows, the Mac's clang for both macOS architectures), run on the merged tree so the zips carry the tagged commit. Version strings live in `CMakeLists.txt` and `packaging/make_app.sh` only; the game shows no version (the title screen stays the name and the choices).

## Release 1.2.0 (2026-10-07)

* **1.2.0 is the iteration and the requests it drew, nothing held back.** The user called the release after R-408: the eight milestones of `ITERATION-1.2.0.md` with R-404..R-408 and B-407..B-410, generation 12 (the giants' decks and storms; saves load, a giant may look different). The index marks the rows not done `1.3.0`. The recipe is 1.1.0's, run on the merged tree.

## Release 1.3.0 (2026-10-11)

* **1.3.0 is the iteration and everything the user asked after it, nothing held back.** The user called the release after R-417: the twenty-two milestones of `ITERATION-1.3.0.md` (G-05..G-07, H-01..H-06, A-01..A-04, C-15, C-16, T-01..T-04, C-11, C-17, C-18) with R-412..R-417 and B-411..B-422, generation 13 (the ages: saves load, the shards found kept by their place in their worlds' records, their contents new). The index marks the rows not done `1.4.0`. The README's paragraph and keys (the chart, the gain, the decoder's pages, the case and the bay, the landing map's peoples, the readings, the works). The recipe is 1.2.0's, run on the merged tree.

## B-403 (2026-10-01): the aurora

* **The aurora is curtains in the sky's geometry, drawn after the stars.** The M4-06 effect was five lobes of azimuth, opaque, hard-edged, meeting at the zenith: a player under a pulsar reported "four moving morphing green blobs". Sheets of light along an auroral oval at a hashed latitude, with a height profile integrated analytically between a ray's entry and exit of each slab, give the lower border, the fade upward, the brightening along the sheet and the latitude dependence (a low arc at 55 degrees, overhead at the oval) from a few lines; the per-pixel noise of the old code was no cheaper. Drawn as a pass over the sky-bank pixels after the bodies and the stars, with no threshold, so the stars show through it; a moon's night side shows it in front, since the curtains hang 100-250 km up, far under any body in the sky, and its lit disc is brighter and stays (the pass compares shades, it cannot add light across banks: the twilight glow near the sun washes the curtains out smoothly instead of cutting holes in them). The alternative, keeping the per-pixel loop and making the star field treat bank 11 as transparent, kept the hard edge. Rain and dust take it away as the rain takes the stars; the night clouds hide neither (KI-336). The regression frames stand at 12 degrees of latitude and did not change.
* **The magnetic field is a hashed, derived property, not a generated one (R-402).** The user asked for aurorae by field strength, reliable on some worlds and rare on others. Hashing the field from the seed with a bias by spin and size costs no generation version and moves no world; a dynamo model from the core's state would have been a guess dressed up. The nights are decided by the star's storm (a noise over days per star) and the field: a strong field keeps a steady glow, the storm drives the rest. Two colour banks (the lower curtain's and its tops') rather than a per-pixel blend, since the palette cannot mix hues; the tops' share picks the bank, the mush softens the seam.

## G-01 (2026-10-01): the galaxy of 200 billion stars

* **The sector frame is heliocentric: the centre moved, not home.** `PLAN-galaxy-scale.md` had the start move to a comparable place in the new disc; instead the galactic centre sits at sector (12000, 0, -5500) and home keeps its sectors. The sixty harness scans, the pinned sites, the new-game search and every coordinate a player wrote down keep working, and the real galactic frame is Sun-centred too; the cost is one subtraction in `galaxyTerms`.
* **Four logarithmic arms of 12 degrees pitch, not two Archimedean ones.** A squared cosine of the log-spiral phase gives narrow crests 5,000 ly apart at home (the Milky Way's spacing near the Sun) and a face-on map that reads as a galaxy (`galaxy_faceon.png`); the plan's `r/1500` gave arms of one fixed spacing winding five times.
* **Home at 0.15 stars per sector, on an arm crest.** The plan's "one in ten in the arms" and the old 0.22 bracketed it: 0.15 keeps 664 stars within ten light years (940 before) and the nearest star 1.16 ly away on average (0.95), so the Vimana hops and the star map feel as before; the gap beside the arm holds 0.07, the outer disc 0.03, the core 0.97. The count came out at 204 billion with the disc's peak at 0.68.
* **The band integrates the whole disc, with a gamma, as a stopgap for G-03.** In a disc 300 ly thick the old 200 ly integration gave the same value in every direction: a uniform glow, and the companion unit checks, which thresholded the sky's mean shade, failed. Two scales (twenty geometric steps to 55 ly, then 1,000 ly steps to 60,000: 4 ms, serial on purpose since the surface builds it while the terrain prefetch holds the worker pool) and a 0.35 gamma after normalising to the core's direction keep the band visible all round the plane. G-03 judges the look: the core's glow, the tone, the nebula patches, the night sky on the ground (KI-337).
* **The companion flood checks compare frames, not thresholds.** The band's brightness in a direction is no longer a near constant; the frames beside and behind the camera are checked against the one 70 degrees off, where there is no disc either, and the finder skips skies with nebula patches (the arm's star-forming cells would raise the mean by themselves).
* **The budget check waits five seconds on the landing map, not three** (superseded the same day by G-02: the 3.2 s were the tiles of a random site picked by the bench's R press, not the home world's; the wait is three seconds again and the site the default one). The hitch is real for a player who descends within three seconds of such a site (KI-338).
* **Names near home changed style.** Home is in a spiral arm now, and the arm's flowing syllables renamed every surviving star there (Brooshaing is Lathyari, Gezhaugreix Seapheeseeri): the generated names are internal since R-401, and the seeds, classes, systems and surfaces did not move.

## G-02 (2026-10-01): regions, clusters and the home star

* **The home star is pinned; the search is the fallback.** The search's pick moved with every density change (the old home vanished with G-01 and the score's early exit then chose Vyavies, whose first landing happened to be slow), and the plan asked for a fixed start. `vesperis_test home` chooses by what a start needs (a yellow sun, two living worlds, a temperate first world, its default site's tiles under a second, the fewest nebula patches) and the sector goes into `HOME_SX/SY/SZ`; `newGame` runs the old search only when the pinned star is gone or has no living world. Heilya's sector hash (0.142) sits close under the density there (0.152): a retune of the disc by more than a fifteenth needs a new pin, which `galaxy` and `home` both announce.
* **Two kinds of clusters, both knots to the cap.** The 6-sector knots at 1.5% were everywhere (one every 24 ly, a sixth of the home region's sectors) and only 2.5 times the field; a galaxy has ~150 globulars in its halo and open clusters along its arms. The field near home (0.15 stars per sector) is already denser than a real open cluster, so a cluster that is to show at all must reach one star per sector at its core: a Plummer-like profile with the core half the radius, 1.2 R^3 stars. A cluster lies far enough inside its cell that a point asks one cell (one hash per density call instead of 27).
* **The bulge region is the bulge term against 0.6 of the disc's.** The bare comparison named `THE BULGE` only to 1,750 ly in the plane, inside the arms' crests not at all; against 0.6 it reaches 2,600 in the plane and 4,000 above, where the gaussian still shapes the sky.
* **The budget check lands at the default site, not R's.** `R` on the landing map picks a random lit site seeded by the wall clock; the descent's first frame measured 37-240 ms between runs by that alone (the tiles of some sites take 3 s, and the frame then builds the rest itself). The default site is deterministic and is what a player who presses Enter at once gets; the wait went back to 3 s (KI-338 rewritten).
* **Nebula patches at the start stay until G-03.** Every candidate of the search region has three to six in its sky (the arm's 14 ly star-forming cells at 15%); the cells' scale is a look question that belongs with the band.
* **`STAR CLUSTER` reads `GLOBULAR CLUSTER`, and `OPEN CLUSTER` is the eighth region.** The HUD, the console and the log print `REGION_NAMES`; old log lines keep their text.

## G-03 (2026-10-01): the band's look at the galaxy's scale

* **The band is starlight behind dust, on a fixed exposure.** G-01's stopgap integrated the stars alone, normalised to the brightest direction and toned with a 0.35 gamma: a soft blob of 20 degrees at the core, the plane at half of it all round (the arms' tangents a tenth over the gap), a map that depended on where you stood (inside the core every direction would read 1), and in space 1-2 shades of bank 0 along the plane, which is black. `galaxyColumn` integrates the stars behind the dust's extinction (`galaxyDust`: a disc two fifths as thick as the stars', denser in the arms, in clouds of a few hundred light years within 700 ly; `GALAXY_DUST` 1e-3 per light year at home, the real disc's magnitude per kiloparsec, so the core's midplane lies eight optical depths deep and its glow comes through above and below the rift, the anticentre one deep), and `bandToneOf` maps a column to a brightness with a 0.7 power over a fixed exposure (`BAND_EXPOSURE` 2,600: from home the bulge beside the rift reads 0.87, the rift 0.55, along the arm 0.41, the anticentre 0.33, 30 degrees up 0.08, the pole 0.06) and a soft knee at 1: the same place always looks the same, the sky saturates inside the core, from the halo the disc is a faint whirl round a bright centre.
* **Three scales of steps, the far ones geometric.** Twelve geometric steps to 55 ly with the clusters, twenty to 700 ly with the dust's clouds (the steps stay under the clouds' 90 ly octave), then steps growing by 7% to 60,000: 94 samples a direction. Steps of 1,000 ly aliased the arms' crests (1,250 ly wide) into a comb along the band.
* **A 128 x 64 map with its rows stretched toward the plane, built on the pool.** The 64 x 32 rows were 5.6 degrees apart: the band's cusp and the rift (2 degrees either side of the plane at the core) were smeared, and the plane itself was never sampled. The rows follow `|u|^1.5` (`bandRowLat`: 0.4 degrees apart beside the plane, 2.6 at 30 degrees, 4 at the poles) and `sampleGalaxyBand` inverts it. 8,192 directions take 9 ms in parallel; the pool is free where the map is built (the landing's site build and the space view run on the main thread, the drainage prefetch has threads of its own, so G-01's "serial on purpose" was a wrong worry). The space view rebuilds it after a hop of ten sectors (25 before): the near clouds' rifts move by a couple of degrees per hundred.
* **On the ground the band lies over the sky in its own bank, after the stars.** Added to the sky's value it never showed: the night sky bank runs from the zenith's colour (0.01) to the horizon's (0.05) over 36 shades, so the band's 5-14 shades lightened a felisian night by two levels of 255 (and 1.0.0's band, squared and at 7, by less). Bank 22 runs from the night sky's own colour to a pale star-grey (no hue edge where it fades into the sky, as the aurora's banks since B-403) and `drawSky` writes it where its light beats the sky pixel's own, both read from the palette since the ramps differ; the clouds hide it. It comes after the stars because the star mask dims a star by its pixel's shade whatever the bank: drawn first, the band would have dimmed the stars over the bulge by two thirds. A sky body's disc is brighter and stays; the aurora lays over the band's pixels as over the sky's. Twenty shades at full brightness: the bulge 17 on a dark clear night, the plane 6-8, the wings under two shades left to the sky bank; airless worlds show it by day too, as before.
* **Star-forming regions are complexes: rarer, bigger, a place to go.** The 14 ly cells at 15% put three to six patches in three skies of the arm in four (every candidate of the new-game search, home's included), and a patch kept a strength of 0.12 at any distance within the 3 x 3 cells. A complex (`nebulaInCell`) is one 42 ly column of the arms in eight (`NEBULA_RATE` 0.12), its three to six patches within 12 ly of a centre near the plane, in sight to 42 ly (the strength falls from full inside the complex to nothing there); `STAR-FORMING REGION` is the 12 ly round the centre (about a thousand stars, a fifth of them blue giants), not the whole column. Near home: twenty complexes within 300 ly, one per 119 ly of the arm's plane, the nearest 92 ly away at sector 274 0 67; home's sky has none. The former cells' stars changed class with their region (among the 4,559 stars near the start, blue giants 611 to 536, white dwarfs 575 to 634); the regress systems and the pinned bodies kept theirs. KI-338's last sentence closed with it.
* **The space band at 24 shades of bank 0** (the ramp's dim blue-grey stop is 20): the bulge 21, the plane 8-10, the poles 1; a star-forming complex's patches at 17 shades of their blue or red remain the brighter thing in its sky.

## G-04 (2026-10-01): the finders, the pins and the flow after the bump

* **The pinned mountain site was stale before the bump.** `PIN_MOUNTAIN_LAT/LON` (-47.733 / -43.747 on Wyariothmai III, Skeatoltdos III before the renames) read FLAT 0.04 on a flat ice sheet at HEAD exactly as after G-01..G-03 (a worktree of HEAD ran the same modes): the generation bump did not move the scan's first felisian world, the ground under the GEN 10 pin had moved earlier (the descent's steepest line was a 0.15 grade; the doc's numbers came from the pin's first day). Re-pinned at -41.674 / -127.807 (MOUNTAINS 0.46, grass, 283 m of relief in 2 km, floors 46%): of six mountain candidates (`landforms sites search` lists them; `descent lat lon` tries one) the one whose slope the descent test passes with B-307's own numbers (grade 0.45, 7.2 s, 27.7 m/s, nothing airborne). The walking stability at the new pin: 0.24% of the far band jumping, 59 skyline flips a frame (2,990 at the old one).
* **The scan's first bodies: what the bump moved.** The stars at sectors 150 0 38, 41, 42, 72 and 83 (Eyshiobriar, Strozheidiax, Thaibream, Goorwes, Cavris) fell under G-01's density, so the first molten world (Goorwes I, a planet, to Henaliolyen III-a, a moon round a white dwarf), venusian, desert (Phyaalion III), hydrocarbon, tectonic and volcanic bodies moved, as did the regress's fourth and fifth systems; the first cratered, felisian (two), thin-atmosphere, icy, quartz, ocean, metal, carbon, comet, europan, bombarded and acidic bodies are the same worlds under new names, which is why the maps and the pins held.
* **The scene finders get the light they ask for.** `findTime` scans the year (twelve epochs of the body's or its parent's orbit) when one day misses the asked altitude by a degree: the tundra scenes at latitude 55 and the aurora scenes at 58-68 had a polar day (`thinatmo_aurora` was lit, sun 31 degrees). It matches the light the view uses (`effectiveSunAlt`: the two suns of a binary blended by their strength above the horizon as `computeEnvironment` does): `molten_night` under Henaliolyen's companion read 6.8 degrees with the primary at -40. A world locked to its star gets a longitude search (72 longitudes, the sun does not move): the scan's first cratered world, the regress's `cratered_noon`, had kept its night side (-54.5 degrees at "noon") at HEAD too. A latitude without that light in any season is skipped for the next body. Every scene prints its site (the body, its star's sector, latitude, longitude, the light), and the regress notes name the body.
* **The forest scene wants its canopy.** The scan's first dense cell (Wyariothmai III, an ice-age world with a temperate family) covered a fifth of the forest ground, under the check's three fifths, at HEAD too; the finder asks for eight dense neighbours 100 m out and moves to the next world when the canopy stays under the bar: Wyariothmai II-a, a tropical moon, 93%.
* **The flow flew nowhere since G-02 and swam.** `cycleTargetStar` turned the ship toward the next star but the lock reads the view, which in the cabin keeps its own yaw (`viewBasis`): with the pinned home no star stood within 14 px and `V` answered NO REMOTE TARGET; at G-01's home a chance star near the crosshair (7.17 ly, not the nearest) had been locked. The `N` key resets the cabin view as `X` does (a game fix). The landing then went where `J` had left the cursor, under the moon: the sea at Heilya II (SWIMMING, the sprint at 1.9 m/s, the buggy at 0 km/h, numbers nobody read since the flow only reports them). `Game::testLandSite` puts the cursor on a lit grassland site (then any land) drawn from the body's seed, three map texels from water; the arrival's world gives the approach and orbit frames, and when it holds no living world the landing parks at the scan's first felisian world (Siophios, 0.97 ly from Heilya, has none). The herd and the long drive run again (983 m at 180 km/h, a herd of seven that flees).
* **The descent's line is a slope, not a boulder field.** The line choice walks every 2 m from the start (10 m steps let a ledge through, and a rock beside the start held the buggy ten seconds), refuses colliders within 3 m as `findOpenRun` does, and asks for a steady grade (the profile bending under 0.5 m a step): on GEN 10's broken mountain ground a metre's bump at 100 km/h is a hop (one to five landings, 6-20% airborne at five of the six candidates), not the launch-thump-launch ladder B-307 tested; the test keeps B-307's bar.
* **Left as it was, noted in KI-339:** `biome_grassland` stands on a cell the site reads as temperate forest, `felisian_river` on one the tropical moon reads as ice sheet; both so at GEN 10, both frames show what they are named for.

## S-01 (2026-10-01): the four plain star classes

* **Varieties, not new weights.** The six families keep their draw (the same weights at the same place in the sector's stream as at G-01); a variety is a hash of the sector's own (`starVariety`, `mix64(seed ^ 0x5A11E7)`: mixed, because the seed's own uniform is the existence test and sits under the density, so the plain XOR claimed every star) that takes a share of a family by region: red dwarfs 40% of the yellow and 75% of the orange family (50 and 85 in the core, bulge, halo and globulars, 35 and 65 where stars form), blue-white stars 50% of the blue giants (60 in the arms, 65 in open clusters and complexes, 30 in the old populations), orange giants 30% and carbon stars 10% of the red giants (40 and 20 in the bulge and the clusters). A table of ten weights would have re-dealt every star's class (the pick walks the cumulative weights), and every S milestone after this one would have moved every pin and the home star again; a share claims its stars and leaves the rest as they were. The companion's class goes the same way (`hashCombine(seed, 901)`). The cost is local: a claimed star changes its system (the luminosity, the planet count, the types by temperature), which is what happened to Wyariothmai and Feeyeithis below.
* **A third red dwarfs, not the plan's half.** The cube of `stars` near home (4,559 stars): S06 33%, S00 17%, S04 14%, S03 7%, S05 7%, S07 7%, S01 6%, S02 5%, S08 4%, S09 1%; the new-game search's 400 sectors hold 66 stars, 22 of them red dwarfs, 11 yellow, 2 orange. The plan's half would have left the yellow and orange families at a tenth: too few living worlds for the search and the target cycling (the plan's own warning was monotony). Heilya is still a yellow star, so the home pin holds; the search's own pick moved from Vyavies, claimed, to Vioyyas (184 0 44).
* **What each row brings.** The red dwarf (L 0.06, R 2.6e5 km, the first orbit at 3e6 km: its temperate worlds huddle at a few million kilometres) locks its first three planets 90% of the time (never retrograde, their eccentricities x0.3 so M5-08's test holds: 80% of 1,781 near worlds in `unit`) and flares: a cell of 3,000 s flares with probability 0.4 (one every two hours on average, 123 in ten days at Wyariothmai), rising in 6 s and fading over 40 (amplitude 0.6-1.4). In space the flare whitens the disc by 12 shades and widens the corona by half again; on the ground the light's colour moves a third of the way to white, the light factor a quarter past its exposure clamp and `setupPalette` opens the exposure by the same quarter, and the aurora reads it as a storm (x0.8; the red dwarf's activity is 0.8, the blue-white star's 0.7). The near world's picture gains little in its mean (168 to 175 of 255 on Wyariothmai I: those worlds sit at the clamp already); the sun's glow and the whitening are the flare. The blue-white star (L 8, a cross of four rays, glow 1.5) warns of its ultraviolet at `3.5e7/d` and favours bare rock and ice; the orange giant (L 12, R 8e6 km, the red giant's mottled disc in amber) scorches its inner worlds and thaws the ice worlds (x2 ocean, x1.5 europan); the carbon star (L 9, R 3e7 km, a dark mottled disc inside a wide dim glow) favours carbon worlds in every zone (15 of 27 systems against 42 of 396 round yellow stars). `describeStar` has a sentence for each; the data sheet's class line shows the table's description.
* **What the claims moved.** Wyariothmai (150 0 36, the pinned mountain site's star since O6-03) and Feeyeithis (150 0 58) are red dwarfs: Wyariothmai keeps 10 of its 16 bodies and its first world is felisian now (locked, 333 K: the scan's first felisian world; its III, the G-04 pin's world, an ocean world), Feeyeithis 11 of 25 with two felisian worlds. `felisian_shore` moved to Feeyeithis II, `cratered_noon` to Wyariothmai IV, `felisian_forest`, the herd and the buggy scenes to Feeyeithis IV (78% canopy), the biome scenes to Eliameavevas II (150 0 65), `space_S00` to Salyiesesh (150 0 97); `molten_night` kept Henaliolyen III-a under a companion of another class. The three locked felisian worlds that come first in the scan are skipped by the placed scenes whose site's light is wrong (G-04's rule: a placed site keeps its longitude), which is why those stand on the fourth world. The flow lands on Wyariothmai I (Siophios still has no living world): the first run put the buggy on a mountainside (18 m of drive at 1 km/h, a 3.8 m jetpack), so `Game::testLandSite` probes the relief of its candidates without the rivers' tiles and takes the first under a 0.1 grade (1,847 m at 179 km/h, the jetpack at 21 m, a herd of 24). The pins: `landforms sites search` keeps to the day side of a world locked to its star (the sheets and the stability frames want light; of the first six mountain candidates only one was lit and it hopped), the mountain site at -48.030 / 152.547 on Wyariothmai I (MOUNTAINS 0.26, rock, 561 m of relief in 2 km, no floors; the descent's grade 0.45, 7.1 s, 28.1 m/s, nothing airborne: B-307's numbers again, four of the five lit candidates pass; the walking stability 3.2% of the far band jumping and 185 skyline flips a frame, rougher than the G-04 pin's 0.24% and 59, far from the old pin's 2,990), the plain at -55.154 / -122.371 (a sand plain: the lit face of the locked world is dry; the old pin had gone into the sea), the cratered moon's at -13.184 / 175.712 on Wyariothmai II-a (hills of basalt; the old pin read FLAT on the new first cratered moon); `thinatmo_hills` holds (Seapheeseeri's red giant was not claimed).
* **Left to S-02 (KI-340).** The star map's keys `1`-`9` toggle S00-S08 and S09 has no key; the legend lists ten codes; the data sheet's class line truncates the new rows' descriptions at 50 characters; the aurora scenes' "active star" is still a pulsar or a blue giant. "Classes seen x/10" reads the table's count already.

## B-404 (2026-10-01): one material at every scale

* **A material never follows a faded value.** The reference note had the rule (materials depend on nothing that `detailMeters` changes) and the code broke it wherever a threshold read a term multiplied by `lodFade`: the europan stain ended at the near ring's edge, 120 m in front of the explorer, and moved with the camera (the report's "renders as you go"); the lava tongues, the cliffs' rock and scree, the crack lines, every landform's walls and sand and the slope-based rock did the same at one ring or another. The rule is enforced by `matVisible`: a feature decides its material unfaded wherever its patch spans a quarter of a cell of the sampling scale (half a texel on the world map), and the slope rules read a material chain of the relief spectrum (the octaves of 48 m and longer, unfaded at every scale), so the slope that decides a material is one number in every ring. A smooth fade of the *material* is impossible (a bank is a bank), so the only consistent choice is the unfaded patch with a hard width gate; a quarter cell rather than a half, because the half ended the icy worlds' 200 m crack lines and the chaos cracks at 2.3 km where they had reached 13 km (a line narrower than a cell is a chain of cells with gaps, which is what a line looks like at that scale).
* **A shaped feature fades by its own width, not by `lodFade`.** Twelve samples per wavelength is the rule for noise (aliasing); applied to a mesa, a canyon, an inselberg or a cone it hid them from the ring that could draw them, and the note's claim that the 512 m ring carried them was false (`lodFade(2.0)` drew no inselberg beyond the 64 m ring). O6-04's `featFade` (full at four samples) is shared now; the periodic landforms (draa, yardangs, polygons) keep `lodFade` (KI-311's stripes). The far rings' heights change where a landform reaches them; the walked rings keep theirs but for the europan ridges (full in the 16 m ring now), the karst towers and the cirques (full in the 64 m ring) and the cliff zone on the chain's slope.
* **The world map keeps its look.** Half a texel rather than a quarter there: the trap terraces' risers (2 km) on a 7 km texel came out as dots, and the icy worlds' great crack lines (7-20 km) must stay the dotted net the globe always showed (the N1 decision: the ground shows the crack zones where the globe does). The felisian maps' rock and biome texels follow the chain's slope and the desert maps show their ergs as sand (as the ground does); every other map is bit-identical. One of the regression's ten maps changed; re-blessed.
* **No generation bump of its own.** The G/S series of 1.1.0 is one bump (generation 11, before anyone keeps a save) and this rides it: the walked ground's materials move only where a slope rule or a gate flipped a cell, the crystal spires and the salt cracks (gated under every ring before) appear in the 4 m ring, the far rings carry more.
* **What stays (KI-341).** The drainage's lines a cell and a half wide at every scale (a beach or a wadi widens at a ring's edge), the lines under a quarter cell (the 54 m icy cracks, the 90 m lineae bands and the chaos cracks end at 2.3 km, a few pixels wide at the horizon; a comet's 512 m ring is its far side), the height thresholds on the far ring's smoother ground (25 m rms at 64/512, 120 m at 512/2048). A coarse stand-in per landform (KI-320) remains the candidate for the terraces, the polygons and the spires.
* **The measure comes with the fix.** `matlod` samples every type at the five scales and prints the pairs that differ; a bug of this kind is a number before it is a screenshot, and `unit` guards the walked rings on four types.

## S-02 (2026-10-02): the UI seams of many classes

* **A cycling filter, not a key per class.** The star map's keys `1`-`9` toggled S00-S08 and the tenth class had no key; with fourteen on the way a key per class was over. `C` and `X` cycle the filter through ALL and every class of the table (the mask stays an int: a class or all of them), `0` shows all, the legend reads `CLASSES: ALL` or the one class in amber and the title counts the stars shown. Toggling several classes at once is gone with the number keys; what the map is used for (where is the nearest star of a kind) is the solo filter. The footer is 53 characters, the frame's width (the old one was 65 and clipped).
* **The codes on their own lines.** `STAR CLASSES SEEN 7/11` and the codes below it in the dim colour like the world types, wrapped at 48 characters onto two lines past twelve classes (the statistics screen has the room: the home star's line ends at 182 of 200 with two). The analyzer's class description wraps onto a second line when it needs one and the list gives up a row.
* **One activity table.** `starActivity(cls)` in `galaxy/system.cpp` is the table the aurora read inline since R-402; the scene finders' "active star" and the unit check read it now, so a class that drives the nights counts everywhere it should (the aurora scenes take the first active star of the scan: `aurora_moon` moved from Hiolyas V-e under a blue giant at sector 279 0 59 to Shialyioleila III under a blue-white star at 179 0 80, aurora 0.37 on a strong field in a storm of 0.79; the two thin-air aurora scenes kept Phyaalion's blue giant). KI-340 resolved.

## S-03 (2026-10-02): the neutron star

* **A share of the pulsars.** `starVariety` turns 35% of the pulsar family into neutron stars (55% in the core, the bulge, the halo and the globulars), S-01's way: a claimed pulsar changes its system, the rest hold. Near home (`stars`, 4,559): S10 2.2% (99), S05 4.7%; by region (`galaxy`): the core 10% against the pulsars' 7%, the bulge 3%, the arm 2%, the disk 4%, a globular 2%, an open cluster 1%, a star-forming complex 4%. The plan's "trickle in the disc" is a star in fifty.
* **The survivors' system.** L 0.03 (the pulsar's 0.05: dim to the eye, the hazard is the x-rays at `3e7/d` on the HUD), R 1.4e4 km, mass 1.5, three planets at most from 1.5e7 km, a quarter of the stars with none; the dead stars' type rule (no living, cloudy, quartz, oceanic or acidic world, thin airs x0.3) and no gas giant kept its envelope (metal +14: the bare cores; bombarded +8: the captured rocks; rocky x1.3; hydrocarbon none, substellar x0.3, molten x0.3, europan x0.5); belts in half the gaps and beyond the last orbit three times in four, even with no world at all (the debris); a companion one time in five. `unit`: 65 in the scan, no such world among their own (a companion's worlds are its own class's), 70 belts.
* **Glassed worlds as a forced trait.** "One or two worlds in wide orbits with a surface of glass": the generator marks the outermost one or two planets of a solid type (`Body::glassed`) and `BodyGen::make` puts `TR_GLASSED` first, dropping the exotic seas (the glass owns their bank). A trait, so the data sheet, the description (", its plains fused to glass by the blast that made its star") and the galleries know it; forced, because `BodyGen::make` sees a body and not its star. The landform (`05-planet-function.md`): the melt pooled on the flats and lows (a mask on the material chain's slope and the relief, the same in every ring: the sheets' share at 512 m is within a point of the 16 m one on every tested world), the fine relief melted away, ripples frozen in, `MAT_GLASS` at albedo 0.42 on 14-59% of the land (41% over five worlds). The material is family water (bank 2) and the glassed palette colours the water as the glass, so the globe, the landing map and the ground agree without a bank of their own; the ground round the sheets is scorched. Pale green (trinitite), not obsidian: a dark sheet under a star of L 0.03 at the surface's 0.62 light floor was a black screen (`scene neutron_glass`, Yaieliola I at sector 152 0 87: bank 2 on 91% of the lower half at a mean shade of 32).
* **The look of the star.** A point of white light in a halo of 1.3 with no rays and no beams (`sun_S10`): the pulsar's quiet sibling, told from the white dwarf's cross and the pulsar's sweep. Activity 0.85 for the aurorae. The code is S10, the next in the enum: the plan's table is renumbered as the classes arrive (the protostar S11, the Wolf-Rayet star S12, the black hole S13).
* **What moved.** `space_S00` re-blessed: 264 pixels of the dim glow round Salyiesesh by one unit where a pulsar is a neutron star now (the nearest 2.7 ly off); the five systems, the maps, the profiles and the three surface frames held. No generation bump: the series' one (11). A material added at the end of the enum moves no map hash.

## S-04 (2026-10-02): the protostar

* **A share of the stars that stayed yellow, where stars form.** `starVariety` turns half the yellow family of a star-forming complex into protostars, a fifth of an open cluster's and one in fifty along the arms (none elsewhere), by a hash of their own and out of the stars the red dwarfs did not take, so the claims touch the fewest systems: the regress held every hash, Heilya stayed yellow, the new-game search holds no protostar. The plan said "found only in nebula regions"; the trickle in the arms (12 of 4,559 stars near home) is physically right (the arms' clouds are where stars form) and keeps the finders and the unit check honest: `space` renders `sun_S11` from Niewamil, the scene stands on Shelfesh I, "every class near home" passes. By region (`galaxy`): a complex 11%, an open cluster 4%, the arm 1%.
* **The look.** A soft orange disc (L 1.5, R 2e6 km: a cool, swollen young star) strongly limb-darkened and mottled through its dust, inside a glow of 2.0 with wisps, and the dust disc seen edge-on from the plane of its worlds: a band of glow along the orbital plane through the star, narrow at the star and flaring, gone 34 degrees out, with the dark lane of the disc's own shadow along its middle. `drawSun` takes the plane's normal in view space (world Y: the planets orbit within their inclinations of it; the surface passes `localFrame(t) * Y`) and runs the band as a pass of its own over the frame, skipping every pixel outside the strip with two dot products, so a 4x frame costs a few milliseconds only under a protostar. The band is a wedge round the star, not the whole ecliptic: from inside the disc the dust would ring the sky, but the wedge is what reads as "the disc" and the belts' sparkle bands carry the rest.
* **The sky full of the nebula.** `starNebulaPatches` gives a protostar two nebula patches round the direction to the star (1.1 rad at 0.8, the whole sky at 0.3) in a tone by its seed, appended to the field's patches in both views every frame (the direction moves with the planet), and the surface tints bank 22 toward that tone: the night glows toward where the star set and faintly everywhere. The space view already draws nebula patches in their tones (banks 4, 5, 0).
* **Belts rather than worlds.** Two planets at most, a third of the stars with none; the types: molten +25, bombarded +20, tectonic +4, nothing evolved (no air, sea, desert or ice shell had time), gas giants x0.2; the belts are the disc: one inside the first orbit, one in nine gaps in ten, one beyond (95%) even with no world. `unit`: 11 of 12 with a belt, 10 worlds, none evolved. Activity 0.9, no hazard, a companion one time in three (young stars pair).
* **What moved.** Nothing: the regress passed unchanged, the flow, the pinned sites, the maps and the aurora scenes are the S-03 run's. The scene finders' biome branch (`wantMat <= -10`) swallowed every code below -22 that had no branch of its own (-44 found no biome 34): it is bounded to the biome codes now.

## S-05 (2026-10-02): the Wolf-Rayet star

* **A share of the blue giants that stayed, mostly in the arms.** `starVariety` turns a fifth of the blue giants the blue-white stars left into Wolf-Rayet stars along the arms, in open clusters and in star-forming complexes, a tenth in the core, one in twenty in the bulge and the disc and none in the halo or a globular (no massive star is old), by a hash of their own (`mix64(hash ^ 0x3E5F)`), so the blue-white claims and every other star stay as they were. Near home 40 of 4,559 stars (against 175 blue giants); by region (`galaxy`): the core 1%, an open cluster 1%, a complex 1%, the arm under 1%. The plan said "rare, in the arms": the trickle in the core and the disc keeps a few in every population where massive stars live.
* **The look.** The blue giant's saturated disc (L 180, R 3.5e6 km) with the cross of four rays and a glow of 1.6, and the shell it has shed as a ring round it: at five radii (four pixels at least), a seventh of that wide, sharp outside (`exp(-4 ex^2)`) and diffuse inside (`exp(-ex^2 / 2)`), filamentary (`0.6 + 0.4 gnoise2` round the ring) and brighter on the side it drives into (`0.7 + 0.3 cos` of a direction fixed by the seed), 34 shades at most. `drawSun` widens the glow's cone to reach past the ring and adds the ring in the glow's pass, so it costs no pass of its own (the protostar's band needed one because it leaves the cone); the pattern is fixed in the world through `discUp`, which both views pass already. A bubble of five radii is a stylisation: a real shell lies light years out and would be the whole sky from a planet, but the ring is what reads as "the shell" (the Crescent and the Bubble nebulae are how everyone knows these stars), and it shows from a world's surface in the black day sky and after sunset as an arc over the horizon (`06-surface.md`).
* **The wind.** `pickPlanetType` zeroes every type with an atmosphere but the giants (applied after the moon rule, which offers a giant's moons a tectonic sky), halves the ice and the giants (x0.4), and favours bombarded (+25), cratered (x1.5), rocky (x1.3), metal (+8) and molten (+5) worlds; two systems in five are rubble (no world), and the belts come 35% in every gap and 50% beyond the last orbit. The zones within four orbits of a star of L 180 are too hot for a giant to form, so two systems in five get one gas giant far out as the last body (at 2.5-3.5 times the next orbit's key), and a gas giant or a substellar object of a Wolf-Rayet star streams its envelope away from the star (`drawStrippedTail`: forty glow discs of its own bank from the limb to four radii out, widening, thinning and curling back along the orbit, before its globe; in a sky in the sun's bank, like the comet's tail; B-427 redrew it: the discs heaped to a white blob, see there). `unit`: 31 stars, 185 worlds (15 giants), none with an atmosphere, a bombarded world in 21 of 24 systems with planets, 21 with a belt.
* **The radiation.** The space HUD's warning at `5e8/d`: a hull exposure inside 5e8 km (every orbit of the four) and high to 1.25e9, so the whole system is inside the hazard, as the plan asked; activity 1.0 (the aurora table); a companion two times in five (most Wolf-Rayet stars are binaries), a blue one possible as for the blue giants. The hazard is the warning the other hostile stars have: no damage model exists, and none was added here.
* **What moved.** The first molten world of the harness's scan is Shiesaimiori I now (sector 151 0 53: Shiesaimiori was a blue giant whose first world was not molten; under the wind's weights it is), so `scene molten_night` and the `maps` gallery's molten map stand there instead of Henaliolyen III-a and the regress's `scene_molten_night` hash was re-blessed deliberately; the other 21 hashes, the flow, the pinned sites and the aurora scenes are the S-04 run's. Heilya stayed yellow.

## S-06 (2026-10-02): the black hole

* **A share of the pulsars that stayed, most in the core and the bulge.** `starVariety` turns a quarter of the pulsars the neutron stars left into black holes in the core, a fifth in the bulge, one in seven in a globular, one in ten in the halo, one in sixteen in the arms and the disc and one in thirty where stars form, by a hash of their own (`mix64(hash ^ 0x8B1E)`). Near home 8 of 4,559 stars (against 206 pulsars); in the core cube 52 of 3,281 (1.6%); by region (`galaxy`): the core 2%, the bulge 1%, the disc 1%, the rest under half a percent. "Rare, in the core and the bulge, a few in the disc", as the plan said; the few near home are what the finders and the unit check need.
* **The radius is a stylisation, the mass is real.** A black hole of 40 suns has a Schwarzschild radius of 120 km, which from a planet is a point: nothing to see, no lensing to speak of (an Einstein angle of a fifth of a degree). The class's radius of 3e5 km stands for the Schwarzschild radius the renderer draws from, so the shadow is a degree across from the first orbit and the lens reaches seven degrees; the mass (`massFactor` 40 x [0.8, 1.2]) drives the orbits and the data sheet (`describeStar` says "of N suns' mass"). Its light is the disc's (L 0.03, a warm white): its worlds' days are as dim as a white dwarf's.
* **The look.** `drawSun` hands a black hole to `drawBlackHole`: a shadow of 2.6 radii (the photon ring), the frame drawn so far (the band, the stars, the belts) bent round it by a lens in the sky (a pixel at the angle th from the hole shows what lies at `th - thE^2 / th` along the same great circle, the other side when that is negative, with the Einstein angle `thE = sqrt(R / D)`, the weak-field deflection of a source at infinity; the bend fades out toward the edge of the region, 2.6 thE, so there is no seam), the accretion disc in the plane of the worlds (a slab half a radius thick from 3 to 9 radii, 63 shades and hot inside to a dim orange outside, brighter on the side that comes toward the viewer, the spin by the seed; its near half along the straight ray over the shadow, its far half along the bent ray from the lens plane, so it arches over and under the shadow, the picture everyone knows) and, two in five, a jet along the plane's normal, drawn before the lens so the shadow covers its root. The lens is a screen-space warp, as the plan proposed: the bent directions are computed on a grid of one logical pixel and interpolated, the frame before the lens copied once, rows in parallel; a 4x frame with the hole a degree across costs 2 ms with the disc edge-on and 9 with it tilted into view. What is drawn after the sun (the globes) is not bent: a planet passing behind the hole would need the globes before the lens, which the order of the space pass does not allow (the sun is depth-tested against the globes drawn before it only in a sky). No lens flare and no veiling glare from a hole.
* **The companion, drawn out.** Half the black holes have a companion, always close (3-6 radii of the pair) and never a giant: `drawAccretionStream` runs its gas from the limb facing the hole to the disc's edge, bent back against its motion (the Coriolis side), under the companion's own disc, in space only (a sky draws its own companion sun; KI-344). The worlds are the dead stars' and less: no air, no giant, no lava, rocks and ice (`pickPlanetType`), half the systems rubble, the belts as a neutron star's. `unit`: 8 stars, 12 worlds, none with an air, a giant or lava, 4 companions all close, 6 with a belt.
* **The radiation.** The disc's x-rays: the space HUD at `6e7/d` (a hull exposure inside 6e7 km, high to 1.5e8), activity 0.9. Time near a black hole does nothing special, as the plan said.
* **What moved.** One star dot of `space_S00` (four pixels: a pulsar in Salyiesesh's sky is a black hole now, a dimmer and warmer point), re-blessed deliberately; the other 21 hashes, the flow, the pinned sites, the maps and the aurora scenes are the S-05 run's. Heilya stayed yellow. The fourteen rows of `PLAN-star-classes.md` are in; the G and S series stay generation 11 without a bump, since every star the varieties did not claim is the star it was.

## C-01 (2026-10-02): the civilisations' worlds

* **A trait by a hash of its own, one world in eight.** `TR_CIVILISATION` is never drawn by the trait draw: `BodyGen::make` gives it to a felisian or desert world when `mix64(seed ^ 0xC1D1)` falls under 0.125, first among the traits (after the glass), so no other world's traits move and the other seven deserts in eight are deserts and nothing more (the user's rule of 2026-10-02: "not all deserts are post-apocalyptic"). The share is a balance: rarer and the scan's finders and a player's first hundred worlds find none; commoner and the signal radar (C-07) has nothing rare to find. Near home 10.5% of the felisian and 12.9% of the desert worlds have it. GEN_VERSION stays 11 (the generation changes for those worlds only, within the one unreleased bump).
* **The ruins are a module of the galaxy, not of the view.** `galaxy/ruins.*` holds the grid, the draw and the settlements' layouts and pieces (`ruinOfCell`, `cultureOf`, `ruinElements`, `ruinSiteOk`), read by the surface view, the landmark finder and the harness alike: the old M4-07 monoliths kept their draw to the bit (the regress's 22 hashes held), the settlements took the cells the monoliths leave on a civilisation's world (hamlets 16%, villages 9%, towns 5%; a desert's lone monuments in the monoliths' 5%). B-417 removed the monoliths (a world's ruins are its people's): the lone monuments take their twentieth on every people's world.
* **Variety and interiors (the user's addition).** Ten building kinds and five plans, every building its own seed: walls standing to a share of their height, doorways with lintels, windows, inner walls, roofs on sound houses where the culture built stone ones, rubble at fallen walls, columns in the halls, domes on the rotundas, gates and a wall round most towns. The pieces are boxes and one dome shape, drawn at three levels of detail with the far faces culled: a walled town of 44 buildings is 275 pieces and half a millisecond at 1x. The colliders follow the pieces (rows of discs along the standing walls), so the rooms are walked into through their doorways; the unit check proves it for every house of a world and for a town's houses through the view.
* **No ground is flattened.** Each piece stands at its own foot on the drawn ground (sunk a metre), so a settlement follows the terrain and the terrain cache is never touched; settlements keep off water, the dry seabed and slopes steeper than a quarter. Flattening the planet function under a town (a plinth) would be a generation change for every trait world and a second code path in the rings; a later milestone (C-08, the ruins that read the world) can add it if the slopes look wrong.
* **The dead desert world's seas and rivers.** A trait desert's seas stood at `oldSeaM` (-120..100 m), on the coarse ground with its canyons (a canyon under the level was a sound of the sea); the bed is a sediment floor keeping a tenth of the relief, the shore a band of 3 m of the coarse ground (a scarp on a sloping coast, a ramp on a flat one), the land within 40 m over the level pulled to a bench (the coastal flats). The drainage reads the bed as the sea, so the wadis end at the old shore and the flood has its outlets there; the net is denser (`riverDensity` 0.8, moist 0.75). The bed is the rock material at a low albedo (dark basins from orbit), salt only in the deeps (over 420-570 m): a first version of the salt put it on two thirds of the bed and read as snow.
* **Trees.** A felisian town is in forest more often than not; no tree stands in a building, the streets keep one tree in three and the street's line out of the gates is clear (a first picture of the town was the trunk at the gate). Rocks still stand in rooms (KI-345).
* **What moved.** Nothing of the regress (no regress world has the trait), the flow, the pinned sites, the maps; the descent and KI-343 stand.

## B-405 (2026-10-02): colliders with a height

* **A collider is a cylinder, not a disc.** `SurfaceView::Collider` carries `y0..y1` (absolute metres): the explorer, the buggy and `findOpenRun` skip what ends under them or starts over them. A town's wall stub stopped the jetpack at any height and a wall whose foot stands on the slope below stopped a walker on the ground above it (the user's "invisible walls"); the trunks and the boulders had the same fault unseen. The tops are not floors: the ground stays the terrain (KI-345), so an explorer who lands on a wall sinks through it and is pushed out sideways; a plinth under a settlement is C-08's.

## C-02 (2026-10-02): the story grammar

* **Templates, not a model.** A shard's text is a template of a kind drawn by tone, its slots filled from the world's data and the shard's own `Rng` (`galaxy/shards.*`, `12-civilisations.md`): deterministic from the body's seed and the shard's index, offline, the same on every machine, and short by design (31-34 words a shard; a fragment with its gaps). A language model would read better and break all three.
* **The world speaks through the slots.** The sun by the star's class, the sky by the moons, the ring, the aurora, the lock, the companion, the comets and the wanderers; the land by the traits; the trees by the flora families; the water by the type and, on a desert, by the age of the shard (the river and the quay while they ran, then the well and the bed). The texts are translations, so their common nouns are ours.
* **Tone by the world and the age.** A desert's shards darken with their age (the sea going, the river, the heat, the dust, the council that laughs, the leaving, the last one), a felisian world's stay ordinary with elegies and a fading at the end: the agreed register (the world went on, the people did not). Half of a desert's fifty are warnings or the end, a tenth of a felisian's.
* **Recurring names.** Eleven proper nouns per world (`Lore`), one word of nine letters at most in one of the name generator's three styles, and the year of each shard counted from the founding (`year N of <town>`): the reader builds the picture across shards, and C-12's timeline is already there in `Shard::year`.
* **Judged on paper.** `vesperis_test shards` prints a world's fifty; nothing is in the game yet (C-03 places them, C-06 decodes them). The game's font has no lowercase: the cabin screen will show the texts in capitals.
* **What moved.** Nothing: no generation change, the regress and the flow unchanged.

## C-03 (2026-10-02): the shards in the ruins

* **Fifty a world, in copies.** The catalogue is the fifty texts C-02 generates per world (`SHARDS_PER_WORLD`); a settlement holds copies of a few of them (`shardSitesOf`: a hamlet one three times in five, a village one or two, a town two to four, a lone monument one seven times in ten, distinct within the settlement). A people's recordings were made in many copies, and a world's millions of cells could not each hold something unique without an enumeration of them all. Once the explorer has a shard every copy of it on the world is gone (the guide keeps the index, the view hides the copies), so a glint is always a new one, and the fifty are a world's whole collection: there is nothing to find twice.
* **Where they lie.** On the floor of a room (a house's far corner from the door, a hall's far end, a tower, a rotunda's side opposite its doorway) or at the foot of a stela, by candidate spots tested against the building's own pieces and its doorway; a building with no clear spot is skipped. Not on platforms or walls (their tops are not floors, KI-345) and not in the open: a shard is found by going into the houses, which is what the settlements are for.
* **Found by sight, taken by hand.** A dark slab with a point of light over it that pulses, drawn behind the depth test, so a glint through a doorway or over a fallen wall gives one away from across a street and a room has to be entered to be sure; `E` within 2.2 m takes it (the HUD says so), not a walk-over, so nothing is picked up unnoticed. No rangefinder hint and no map mark: the ideas note's scarcity rule.
* **The log and the statistics, not the text.** Taking a shard writes a `SHARD` entry (the place, the settlement, the world, the count of fifty) and counts on the statistics screen (`SHARDS TAKEN  N FROM M WORLDS`, on the line the two distances freed); the text itself is C-06's (the decoding on the ship). The guide file gains `shard <body key>/S<index>` lines; a guide from before C-03 reads as having none.
* **What moved.** No generation change (the cells, the layouts and the colliders are as C-01 left them); the regress unchanged; one frame more in the harness (`civilisation_shard`).

## C-05 (2026-10-02): the voice

* **The spoken words are the written ones.** The ideas note asked that "speech and decoded text come from the same seed, so they agree in length and rhythm"; here they agree word for word: the recording speaks the people's words (`DecodedWord::theirs`, the `Tongue`'s word for each of ours, a name as it is, a numeral digit by digit) and the speech carries each word's span, which is what the screen aligns the text to. The speech does not depend on how much of the language is read (the unit checks the spans at one shard held and at ten), so the alignment holds at any share.
* **The sounds are a table, the voice a world's.** `PHONES` gives every spelling the tongue's pools and `generateName` use a sound (a vowel's formants, a consonant's manner and place); the per-world part is the voice (`voiceOf`: pitch, range, rate, tract, breath, roughness, nasal murmur, tremor, a second tone, the stress rule, the sentence melody, tones, clicks, rolled r's), drawn from the body's seed and leaning with the names' style as the music's timbres do. The inventory a world shows is the sounds its pools spell; a tongue of its own letters would have been a second alphabet to read, and the C-06 tongue is already what the texts show.
* **The recording replaces the wait.** C-06's reading took 0.11 s a word while the words resolved; now a first reading resolves the words as the recording speaks them, 12-25 s for a text of thirty words by the voice's rate, with Enter skipping to the end as before and Esc leaving the shard unread. A read shard is spoken again on opening with the whole text shown and the spoken word underlined, Space pauses, Enter skips or replays. It is longer than the old wait and it is the feature: the shard is heard.
* **Formants in cascade, not samples.** A glottal pulse through four resonators (`speechSample`) is a few hundred bytes of code and sounds like a voice from somewhere else, which is the register wanted; a sample bank would be a voice from here. The pitch in the loudest windows of a rendered shard sits at the voice's base and the envelope's peaks come near its rate (the python check in the scratchpad), which is as far as paper goes: the ear is the user's.
* **The voice stays on the decoder screen**, like the music (`Game::frame` clears `audio.speech` off it); the ship's hum ducks under either and comes back when both are off. KI-347 covers both.
* **What moved.** No generation change (`GEN_VERSION` 11); the regress unchanged (no hash sees a shard). `decodeShard` gives a numeral a spoken form in `theirs` (the digits' words joined by `-`), which the screen never shows (a numeral is read from the start); `decodeT`/`decodeDur` went, `shardFresh` and the speech clock took their place; the flow's `shards_decoding` is a second into the recording, `shards_text` after Enter, `shards_speaking` is new; two unit checks are new.

## C-04 (2026-10-02): the music

* **A piece is a shard of its own.** The ideas note had a shard as "either a spoken monologue or a piece of music"; so twelve to eighteen of a world's fifty are music (`shardIsMusic`: a shuffle of the fifty from the world's seed, the count from the seed too), with no text. The alternative, every shard a text with music under it, was set aside: a world's catalogue reads better with songs among the records, a town's few shards can hold a song, and C-05's voice gets the texts to itself. The count is a shuffle and not a coin per shard because a coin gave Aieliaalas II 23 of 50 (a 1-in-80 draw): the first world a player reaches should not be an outlier. The music shard keeps the tone the text would have had, and the piece's form follows it (an elegy a lament, a warning a march, the end the last song).
* **The tradition is a function of the world, the piece of the shard.** `traditionOf` reads the body's seed and the lore's style (hard names lean the timbres to bells, plucked strings and the drum; flowing ones to pipes and bows), so the language and the music of a people agree in character; `pieceOf` reads the shard's seed. Scales are five to nine notes to a period from an equal division of 5-24 steps or from just ratios, never with neighbours under 75 cents or a hole over 450; the period is the octave at 82%, a stretched or narrow one at 13%, the twelfth at 5%: enough that a tradition sounds like somewhere else, not so much that every world is a noise experiment. The resting degree (nearest a fifth, else a fourth) is where the open phrases end, so the phrases breathe the way ours do even in a 13-step scale.
* **Form over improvisation.** A piece is statement / variation / answer / return (with the answer and the return once more for the quick forms) and a close of the tonic held a cycle: 28-70 s. Rests are a beat at most; a leap is answered by a step back; every timbre of a tradition plays (the answer on the second when it has no part of its own, the variation on the third when there is no drone). The forms' table (tempo, density, register, drum) is what makes a lullaby a lullaby.
* **Heard whole from the first.** The language's learning curve does not apply to music: a piece plays whole on its first opening, and hearing it is recorded as read at the world's full count (`decoded <key> 50`), so it never waits and never shows `*` again. The wait of a text is the listening of a piece.
* **The music stays on the decoder screen.** `Game::frame` stops the piece whenever the state is not SHARDS or its naming entry: the synth reads the pointers the decoder gave it, and the decoder is the one place that owns them. A way to hear the pieces found elsewhere on the ship (a cabin screen, in flight) belongs with C-11, the instruments. (C-11, 2026-10-10: the decoder's own still stop off its screen; the ship's player owns a copy of its own and plays on, below.)
* **Naming.** `N` on a piece names it through the text entry (kind 7, keyed like the shard in the guide's `names`, a note in the log); the entry draws over the decoder and the music goes on. It is the "name a piece" of the milestone and the guide-loop's "see, name, remember" for a sound.
* **What moved.** No generation change (`GEN_VERSION` 11); the regress unchanged (no hash sees a shard). The text shards keep their texts (the music decision draws from its own hash before the text is expanded), but with twelve to eighteen fewer texts a world's language is smaller (Aieliaalas II 345 to 255 words) and the share read with one shard moves a little. A guide holding a shard that is now music lists it as a piece. The unit's shard checks count the text shards; two music checks and the flow's four `shards_music*` frames are new.

## C-06 (2026-10-02): the decoding on the ship

* **The language, not the damage.** C-02's `damage` (10-40% of the words lost as "...") was the stand-in for a decoder; the decoder is the thing itself now and `Shard::fragment` went with it. What a shard withholds is the words the ship's computer has not learnt: a word not learnt shows in the people's own tongue, dim; a name in amber; a word read in white. Gaps of dots read as a broken recording, words in another language as a language being learnt, which is what the ideas note asked for ("a language you learn"), and the player can watch the same word turn up across shards. The whole text is the end state, as C-02 decided: ten shards of a world read all of it.
* **The words most used are learnt first.** The language is the fifty texts' words by their count (`languageOf`); `languageShare` gives 30% of the uses with one shard held, 8% more per shard, all at ten, and `languageKnown` takes words from the top until that share is covered. So the first reading gives the articles, the pronouns, the names and the shape of the sentences; the content comes with the next shards and the rarest words last. Deterministic from the world and the count held, nothing stored but the count. Names are kept from the start (the decoder transliterates what it cannot read), numerals too.
* **The tongue is a function of the world and the word.** `tongueOf` draws a people's syllable parts from pools of the style their names are in; `tongueWord` hashes the world and our word into one to three syllables of about our word's length (never under two letters: one reads as noise in capitals), so "the" is the same short word everywhere on a world and two worlds never share a tongue; a word that would repeat one already given is re-drawn (`languageOf`), so the most used words stay distinct. C-05 gives these words a voice from the same `Tongue`; its pools are where the phoneme inventory starts.
* **A wait, once per reading** (superseded by C-05: the wait is the recording speaking). Reading a shard the computer has not read at the language's current share takes 0.11 s a word (2-6 s) while the words resolve left to right; Enter skips it, Esc stops it. The guide records the count it was read with (`decoded <key> <n>`), so a shard re-read is instant until more of its world's shards are found, when it decodes again with fewer gaps: the re-decoding the ideas note wanted is a second wait the player can see. The list shows an unread shard as the computer last read it, all in the tongue when never, so a pile of new shards looks like one.
* **Where.** A decoder on the cabin's back wall (the one wall that had nothing; `facing` 7), the flight computer's preferences page for the cockpit view, and the guide menu's `THE SHARDS`, which on the surface says they are read on the ship: the ship is the place to read, as the ideas note asked ("a reason to return to the ship other than to leave"). The screen runs game time like the flight computer. The guide menu's box grew with its seventeenth item (its sixteen already ran into the footer).
* **What moved.** No generation change; `Shard::text` is unchanged (the removed `damage` drew after it); the regress unchanged; the statistics line reads `SHARDS TAKEN N FROM M WORLDS  DECODED K`; eight frames more in the flow. A guide from before C-06 has no `decoded` lines: every shard it holds reads with a wait once.

## R-403 (2026-10-02): the drone

* **A second vehicle, the buggy's twin.** The user, playing the C series: the ruins are "super hard to get to" by buggy (kilometres apart over water, forest and slopes). Not a faster buggy: a drone, built from the buggy's hull and seen through the same nose camera and CCTV feed, so the two vehicles are one language; four ducted thrust pods on arms and two skids where the wheels were. The camera rides a gimbal: the hull banks and dips in the outside view, the picture stays level (a tilting picture at 320 km/h sold nothing).
* **Thrust pods, not rotors: no air needed.** The jetpack fires on airless worlds and the drone is the same craft's technology, so it flies wherever there is gravity enough to need it; the one refusal is the buggy's (a comet: at speed it would leave the world).
* **320 km/h, 400 m, no fuel.** The user's speed; a ceiling of 400 m over the ground (above the jetpack's 300, with the far ring drawn to 53 km there is something to see); no fuel, because the buggy has none and a drone out of fuel over a lake would be a trap. The limits are the ceiling and the collisions: B-405's spans make a wall or a trunk met low a hit and flown over high nothing.
* **It lands on anything level enough, including water.** The skids rest on `surfaceHeight`, the water included: a landing on a lake floats and the explorer steps out swimming; on a slope the hull sits level at its centre's height (KI-348). `E` out only on the ground: an explorer dropped from 300 m with a jetpack would be a stunt the game does not sell.
* **The keys.** `F` at the capsule (free on the surface; the KEYS label `FIELD AMP. / DRONE` shares the key with space's field amplificator as `E` shares use and the vehicles); Space and Shift for the vertical, because Space is already the jump, the jet and the buggy's brake and Shift is the sprint a seated explorer has no use for; W/S and A/D as the buggy's.
* **What moved.** No generation change (`GEN_VERSION` 11); the save gains a `drone` line (a save without it has no drone); the guide a `stat flown`; the regress unchanged (no hash sees a vehicle); the flow 5 s longer with three frames; the bench an eleventh budget (`drone 2x` 16 ms; 7.3 measured); `unit` 125 checks.

## B-406 (2026-10-02): the water and the jetpack

* **Swimming is where the body is, not what lies under it.** `updateWalking` decided it from the ground alone (0.4 m or more under the water: swimming, at any height), so the jet over a shore was pulled to the float level and cut out. Deep water is swum once the feet are within 0.3 m of the float level (0.6 m while swimming already); the air over it is the air.
* **The jet lifts out of the water.** Held Space afloat (not diving) fires it: a lake crossed by air and run out of heat over was otherwise a trap, since there is no jump from the water. The dive and the swim stamina are untouched.

## C-07 (2026-10-02): the signal radar

* **The transmitters are sectors of the galaxy, found by the radar, not made by it.** The ideas note asked for determinism ("which systems carried a civilisation must be a property of the galaxy, hashed like everything else; the radar only reveals it"). Hashing every civilised world would mean generating every system within reach (39,000 civilised worlds within 200 ly of home: 12.7% of the systems have one), so the transmitters are hashed sectors instead, a few or none per cube of 32 (`transmitterCell`), and the sector's star transmits when it has a people's world. The scan is then 2,500 cells, 115 candidate sectors, 13 stars and under a millisecond, and the rarity is the hash's: 1.8 peoples' signals within 200 ly on the arm, more in the core where the stars are.
* **The salt is pinned like the home star.** Fifteen of forty salts gave the home sector no people's signal within reach; the one kept gives it three (79, 85 and 179 ly) and the two test worlds three each. A hashed galaxy makes every salt as true as any other; the start should hear something.
* **Far means 200 ly, and the age is the distance.** The reach is what keeps the signals far ("rare and far (many jumps)", before the Vimana reached any star in one flight): the nearest from home is 79 ly, 25 s of flight, and the signal heard left its world 79 years ago. Nothing ties that age to the shards' years (`Lore::spanYears` is the people's own count); C-12's timeline is where that could meet.
* **The radar is a camera you enter** (the user's call on the first build, 2026-10-02: "a camera mode you enter for this signal search minigame, like you are viewing through radar camera"). The first build was a HUD over the ordinary view, the cabin still around you; now `B` leaves the cabin for the ship's own instrument, seen through like the vehicles' nose cameras (`radarCameraFeed`: a cold phosphor, scanlines, a rolling band, static that thins with the clarity), the mouse turns the ship, and `B`/`Esc` come back. The meter is three lobes (forty, ten and three degrees) over a wandering static, the scope keeps four seconds so a rise swept past can be turned back to, within the middle lobe the source is a smudge of static to centre on (nothing outside ten degrees: "a signal should be found, not handed over"), the hold within three degrees locks in 2.5-5.5 s by the strength, and the lock holds within eight.
* **The lock names the system only, and Enter flies** (the user's call: "signal only shows the system from which the signal comes, you don't know if it's felisian, or desert or anything, and when you press enter you lock on the star in that system and fly to system"). The first build's readout said `A PEOPLE'S TRANSMISSION - A VOICE` and `A DESERT WORLD OF A YELLOW STAR`; now it says the star, its sector, the distance and the age, and the log the same; what sent the signal is heard, not told, and found on arrival. Enter sets the remote target and starts the Vimana flight in one (a signal of this system: the fine approach), leaving the camera.
* **The recording is heard before the lock, through the static.** A people's world within the middle lobe puts its recording on the synth through the receiver's band (`AudioState::radio`: 250 Hz to 2.6 kHz, a slow fade, the static thinned as the beam sharpens), so the sweep is heard as much as read, and the voice or the music of a people is known before the ruins are walked. The broadcast walks the fifty in a stride coprime with fifty (a hashed chain first tried fell into a cycle of six), one recording a slot of 75 s; the lock starts the recording from its top (the synth cannot seek). What is heard while locked goes into `Guide::heard`, and the shard taken later that the radar caught is logged as "the recording the radar caught".
* **Honest as an instrument, and the sound tells the nature** (the user's call: "sounds should be more varying depending on nature of the signal with some deep cosmic-horror like feeling"). Pulsars (every one in a transmitter sector, one in 320 of those within 25 ly), a comet outgassing within 1.5 AU of its sun and the magnetospheres of the giants and the strongest-fielded big worlds are locks too, unnamed on the readout, each with a sound of its own: the first build's bright hiss and plain tones became a bed of the void (a rumble that breathes, a drone of two tones a quarter-tone apart, a wisp of resonant static wandering, through an echo a third of a second behind), a pulsar's thump with a growl falling away after it and a thin whine, a comet's song (a warble far down with a second voice a semitone off) between geysers and cracking ice, a magnetosphere's slow whistlers, chorus, breathing hiss and a choir of three voices a quarter-tone apart, and a people's voice ghosted (a ring at 37 Hz under it, dropouts, a carrier drifting). The levels were set from rendered WAVs (`vesperis_test signals wav`): the first bed sat at an rms of 0.25 and clipped. The first filter (every world of `magneticClass` 3) gave the home system four magnetospheres and Aieliaalas six; a field of 0.8 and over on a world of 2,500 km leaves one.
* **Three ways in, one key.** `B` in space (free there; the KEYS label `BUGGY / SIGNAL RADAR` shares it with the surface's buggy as `E` and `F` share theirs), the radar set beside the decoder on the back wall (`facing` 8), the flight computer's devices page; `B` and `Esc` leave the camera. The decoder takes the synth (opening it switches the radar off), a Vimana flight, an arrival, the surface and a load switch it off, the other screens pause the sweep and keep the sound.
* **What moved.** No generation change (`GEN_VERSION` 11); the regress unchanged (no hash sees the radar); the guide gains `signal` and `heard` lines and `SIGNALS HEARD` on the statistics; three unit checks, a `signals` mode (with `wav`), six flow frames; `page0` of the help has the `B` line in place of its blank. KI-349 holds what stays.
* **More sounds, and no two signals alike** (the user's second call, after playing: "the sounds for signal are PERFECT ... echoed voices, buzzing, pulsating were all perfect ... the only thing I didn't like too much were pieces that were similar to music. Please, give MORE sounds for signals, greater variety in that cosmic horror direction"). A people's broadcast became a walk of *programmes* (`programmeOf`, `programmeFor`): the recording spoken, whispered, chanted or looped, a numbers station, or the world's machines (a beacon, data bursts, a bell tolling, a siren, a murmur of phantom voices), with the piece kept but rare (three music slots in ten, at half to three quarters of its speed through a heavy wow: the one sound the user liked least is now the one heard least, without losing the recordings' music from the game) and each world given a character (two machines and two ways of speaking of its own from its seed) so that worlds differ by ear before they are reached; the natural kinds draw their parameters from the signal's seed (`AudioSynth::rvDerive`: interpulses, nulling, a breath before the thump; the comet's interval, glide and moan; sferics, roars, four choirs) so that the same pulsar is the same pulsar and two are not; and the static got its own events (a carrier passing, a boom, a moan, knocking, a murmur, a breath, a scream, every twelve to forty seconds). The generator of the voices that are no one's is one engine (`phantomSample`: a glottal pulse or breath through three formants, babbling vowels at a syllable rate or steered by the caller) used for the murmur, the comet's moan and the static's moans and screams; the levels were set from the renders again (the chant's hum trimmed from 0.3 to 0.2 and its echo from 0.78 to 0.72 after an rms of 0.165). The lock's readout and the log still name the system only: the programme is heard, never told.
* **A repeat runs on the game's clock.** A loop's phrase and a numbers station's groups are spoken again after the gap by `radarRepeatContent` from the game's own content clock (as the decoder's `pieceT` and `speechT`), not from the synth's `speechDone`: the headless harness never runs the synth, and the flow must see the repeats as the game does.

## C-08 (2026-10-02): ruins that read the world

* **The terrain is read once, when the layout is made, and kept with it.** `readSite` samples the planet function at 64 m (the drainage off: no tile) at the centre, sixteen directions at the radius and rings out to 500 m past the edge, about forty samples for an inland settlement and a few more at a coast, under a millisecond; the result lives in `RuinSpec::site` so the view, the harness and the finders read one answer. The landmark finder, which reads cells without their layouts (`withBuildings` false), reads no terrain and moves nothing: a settlement stays where C-01 hashed it, the harbour comes to the shore rather than the village to the harbour (a quay up to half a kilometre from its village is a port a walk away; moving coastal settlements onto their shores would have put the finder's positions a kilometre off or cost it a site read per cell during the descent).
* **The features come after the plan and the wall.** `addFeatures` draws from the layout's stream only after C-01's draws, so every settlement keeps the buildings it had and the regress, the shards' sites (C-03) and the unit's counts hold; the new kinds (`BK_QUAY`, `BK_MOLE`, `BK_BOLLARD`, `BK_TERRACE`, `BK_CISTERN`, `BK_WINDWALL`, `BK_CAIRN`) take no shards and have no doorway.
* **Terraces are walls on the slope, not ground.** KI-345 asked for a plinth; flattening ground means touching the terrain cache or teaching the explorer to stand on slabs, neither of which C-08 does. A retaining wall every wall's height of fall along the contour, in segments that stop at the houses, reads as terracing from the slope's foot without changing what the feet walk on; the walls still step at their joints (KI-345 narrowed, not closed).
* **The harbour works stand in the water.** C-01 skipped any piece whose ground is under water; a mole's point is to be in the water, so pieces of part 6 draw with their foot at the water's level instead. On a dead desert world the same pieces stand on the dry bed, and that is the old shore made visible where KI-346 said it was a ramp.
* **The decay is the age's, not a draw** (`decayOf(ageYears, wet)`: 0.08 + 0.3 log10(age / 100) + 0.2 wet, the age 200-40,000 years log-uniform from the seed). The ideas note's "the ruin tells you how long ago without a number": a two-century village stands, a twenty-millennia town is foundations. The culture's old decay draw is kept in the stream (unused) so the roofs, the plan and the towers do not move; the two test worlds landed at the ends (209 and 22,045 years), which the user will see at once on Aieliaalas II, the first world the flow and the scenes reach: its town is stubs now, which is what 22,000 years of rain do.
* **The wind is a heading by the dune fields' rule, not the dunes' drawn orientation.** The dunes' direction lives in the body's 3D frame inside the planet function and is not a function of the ruins; the same rule (the world's angle, flipped per hemisphere, turned 0.9 rad per 30-degree band) read as a compass heading gives a wind that is consistent across a world and plausible against the latitude, which is what a wind wall needs.
* **Rocks and logs keep out of the rooms per cell, not per rock.** `settlementsNear` is answered once per 16 m cell (as the trees did since C-01) and the rectangle test runs only where a settlement is within reach, so the surface budget does not move.
* **The capsule lands outside the walls at the galaxy level.** `settlementClearance` reads the landing cell and its neighbours without layouts and moves the point radially to the radius plus 30 m; it is a free function of `galaxy/ruins.h` so the unit tests it without a `Game`, and the landing map calls it once at Enter with a status line.
* **Two C-01 defects the age exposed, fixed in passing.** With Aieliaalas II at a decay of 0.92 (its stream's old draw gave 0.4-0.7) the rubble reached nearly every wall and the unit found a doorway blocked: the doorway's midpoint that the rubble and the shards (C-03, `buildingDoor`) keep clear of was mirrored on the back and the left walls (the walls' axes run +x' on the front and the back and -z' on the right and the left; the code had -doorPos on side 2 and +doorPos on side 3), so on half the houses the rubble avoided the wrong spot and a shard could sit in the real doorway; both now read the gap where the jambs are drawn, which moves some of C-03's shard sites on those houses. And on the desert test world a town wall ran through a house at the edge of a small grid town (the ring at 1.04 radii, the blocks to 0.95 radii plus a house's half extent): the ring is now at 1.04 radii or three metres outside the farthest building, whichever is more. The C-08 unit check walks every house's doorway on both test worlds (C-01's check walked the felisian world's only).
* **What moved.** No generation change (`GEN_VERSION` 11); the regress unchanged; two unit checks and the collider check widened (131); three scenes; `ruins` prints the terrain read. KI-345 narrowed, KI-346 noted, KI-350 for what stays.

## C-10 (2026-10-03): star charts as shards

* **The figures are the people's, not the chart's.** The first build grew figures per chart and named them per chart, so the same stars had two names on two charts of one world. The figures are one set over the whole sky (`skyFiguresOf`: ten to fourteen from the world's seed) and a chart shows those that fit whole within its hemisphere, so a name means one figure wherever it is seen and the caption lists exactly what the chart shows.
* **A chart's stars are the cube the game draws.** `chartSkyOf` reads the ten sectors round the people's star, which is `StarNeighborhood`'s radius, so at their star every star of a chart is in the sky (the unit checks it); a brilliant star twenty light years off that their sky would show is left out (KI-351). The brightest hundred by the star field's own measure keep the disc readable at 66 px.
* **A people's world is a choice, not a chance.** Near home one star in eight has one, and the first build had every chart pointing at a people. The first chart marks the people the world knew, heard first (C-07's transmitters, so the charts and the radar tell one story), the second the lore's named star (the brightest of its nights, which C-02's texts already mention), the rest by the world's coin: 43% of the charts near home mark a bright star.
* **The marks live in the lore.** `Lore::marks` is computed in `loreOf` (the cube scanned, sixty systems generated, the radio's reach scanned: about a millisecond a world), so `shardOf` writes the caption without a game and the other people's name is a name of the shard (`Shard::child`); `peopleNameOf` reads the lore's first draw alone, since a full `loreOf` of the other world would scan its sky and the sky of each world it knew, without end.
* **A chart is drawn, not spoken.** No voice for a chart (a map, not a sound): the reading is the chart drawing itself over four seconds, the wait a text's recording is in spirit, Enter skipping it as there.
* **The caption carries the names into the language.** The figures' names are in the caption, so they are words of the fifty and learnt like the rest (a name of three words, "the two sisters", resolves word by word); nothing was added to `Language`.
* **The overlay draws on the sky alone.** A line's pixel is skipped where the frame has depth (`Framebuffer::invz`: the cabin's walls, a globe, the ground), so the figures sit among the stars in the window band and never across the cabin; the HUD's own markers (the remote square, a belt's bracket) keep drawing over everything as they did.
* **Enter sets the target; the flight is the player's.** As the radar's Enter sets the remote star (C-07) but without the flight: the decoder is not a flight instrument, and the target line on the chart says where `V` will go.
* **What moved.** No generation change (`GEN_VERSION` 11); the shards' shuffle unchanged for the music; three to five text shards a world became charts (the C-02 and C-06 unit numbers moved a little); two unit checks; the `charts` mode; four flow frames; KI-351.

## C-09 (2026-10-03): roads that still go somewhere

* **The network is a property of the galaxy, not of a landing.** The roads are a function of the settlements and the terrain (`galaxy/roads.*`), read from the ruin grid like the settlements themselves, so the surface, the landing zoom and the harness see one network and a road found from the air is the road under the wheels. The relative neighbourhood graph was chosen over the nearest-two-settlements rule because it is symmetric by construction (a pair is judged by the union of its two ends' blocks of cells, so no end can see a neighbour the other cannot), connected within a region (every settlement reaches its nearest neighbour, nobody is left without a road: 0 of 103 on the two test regions), planar, and a town gathers several roads as a town should.
* **A road is a walk, not a line.** Five headings a step, the grade squared against the turn and the way off the goal, with the straight step taken without looking further where the ground rises under six per cent: a road over a plain costs one planet sample a step (a fifth of a millisecond for three kilometres), a road through hills bends round them, and every way is dry (the sea, a lake, and a dead desert's old sea, because the roads were used when the sea stood). A pair whose every way steps into water has no road rather than a road through the water; a river is crossed (a ford: the vertex keeps its water, the paving stops at the bank).
* **The road lives in the terrain vertices, not in a drawn object.** The ground's own material, albedo and vegetation carry it (`sampleAt`), so every pass that reads the terrain (the rings, the two-material contour, the flora, the rocks, the buggy's traction, the HUD's material line) sees the road without a branch of its own; the site grows its road set as the explorer moves rather than building the fifty-kilometre reach of a frame at a landing (eleven by eleven cells at a landing, two cells on each time, the worker joined before the list grows because `sampleAt` runs on the render threads). A road's share of a vertex's footprint is a box across the road, so the 64 m ring shows a faint tone and the 512 m ring nothing: the road is seven metres wide and the coarse rings are honest about it.
* **The wear is the culture's, by the age, and it leaves gaps, not a fainter line.** A noise along the way under the culture's wear (`roadLeft`, soft-edged gaps of forty to a hundred metres) is what twenty millennia do to a road: stretches survive and the eye joins them. The noise's spread was set from a measured table (a wear of 0.1 leaves 97 per cent, 0.3 leaves 81, 0.5 leaves 50, 0.6 leaves 34), so the two test worlds sit where the age puts them (94 and 51 per cent).
* **The zoom draws the roads it has not found.** The landing map's other marks are the sights already found, but the roads are terrain, as the rivers the zoom draws are: a network to read from orbit is the idea's own words. They are lines in the ground's own colour darkened under the texel's light, and the whole network of the window is walked on the worker pool at a zoom build (the walks are a fifth of a millisecond each).
* **A beaten track on sand is dust at half the albedo.** The first build darkened the sand a fifth and kept its material, and the track vanished into the ground's own tone patches (B-311's tile is four shades either way; the scene's pixels put the track fifteen per cent under the sides). A road that cannot be seen is no road: the beaten track through sand takes the dust material (the dust tile, no ripples) at half the sand's albedo, through grass or forest the sand family (bare earth), and a paved road through rock darkens its stone a quarter, so every pairing of road and ground has a cue the eye catches.
* **The zoom's roads are hairlines in the frame's own pixels.** `drawLineRGB` draws logical pixels as blocks the scale wide, which is the sector grid's weight; a road is drawn pixel by pixel in the canvas, so at the game's 4x it is a hairline in the ground's own colour darkened under the texel's light, and the window's network reads as a faint web that resolves into roads near the cursor. The whole network (four and a half thousand roads over a big world's window, a settlement every three kilometres) was a honeycomb over the land, so the zoom draws the roads with a village or a town at an end and leaves the lanes between hamlets to the ground: a map's convention, and the hubs show.
* **What moved.** No generation change (`GEN_VERSION` 11); the regress unchanged (none of its scenes is on a civilised world); two unit checks (135); two scenes; the flow's `roads_zoom` frame and the seconds on every flow frame's line; the `roads` and `zoom` harness modes; KI-352 for what stays.

## C-12 (2026-10-03): graves and names, the calendar, the last recording

* **No unit of ours in the people's calendar.** The game's days are minutes of play and its years their orbits, so an hour or a year of ours on a shard would be a lie. The calendar is the world's own ratios (the day its turn, the year its orbit, the month the nearest moon's round in days, three days at least and twice within the year), the dates read "year 212 of Aiyin, moon 69, day 5", and the timeline is in their years alone. A world whose day is a quarter of its year or more counts no days (a locked world's day is its year): moons alone, else years alone.
* **The dates come from a hash of the shard's seed, not its stream.** A month and a day drawn from the `Rng` would have moved every draw after them and rewritten the fifty; the hash leaves C-02's texts as they were (the C-02 unit numbers moved only where the tones did).
* **One end a world.** The ideas note asked for one shard that is different, the final recording; C-02 had given the end's tone to a share of the late shards (five of a desert's fifty), so several said "I am the last". Now the text shard of the greatest age draw is the last (`lastShardOf`: its age set to 1, the span's end, the end's tone) and `pickTone` gives the end to no other (the late draws fall to warnings on a desert, elegies on a felisian world). A generation change of the shards' tones (not of the galaxy: `GEN_VERSION` stays 11, nothing in a save points at a shard's tone); the regress has no civilised world in it.
* **The last is not signposted, and the timeline's end is not told before it is read.** The shard lies in a ruin like any other, the taking says what every taking says, the row shows its words in the people's tongue like any unread shard; the span's end is kept from the timeline until the last recording is read (the rule runs dotted past the latest held). Then the log notes it, the screen names it, the rule closes in amber. The reading is the moment, not the finding: the explorer cannot know what a shard holds before the decoder.
* **The graves are drawn as pieces of the settlement, outside it.** A stone is a `RuinElem` appended to the cell's lod 0 (a slab with glyphs, or the same lying flat), so the drawing, the lighting, the fog and the colliders are the ruins' own and no new object kind exists; the burial ground lies beyond the wall and every building's reach, away from the shore, since a graveyard inside a town's rooms or on its quay would read as rubble. A stone whose place is taken (a fallen stretch of wall, a cairn) is skipped after its draws, so the others stay.
* **The names are read as they are.** A name is kept in every decoding (C-06), so the HUD can read a stone to an explorer who holds no shard of the world: "A GRAVE: THAELU, 140-212". The years are in the people's count, which the explorer learns to read from the shards' dates; a stone before the founding reads "to year 41".
* **A grave is recorded by standing at it, with no key.** Like a road under the feet (C-09) and unlike a shard (E takes it): nothing is taken, the guide keeps the id the first time within 2.2 m (`Guide::graves`, a `grave` line), a status and a `GRAVE` log line say so once, and the statistics count them with the records ended on a dim line under the shards (the page is full: the dim rows now step 9 px).
* **What moved.** `GEN_VERSION` 11; the regress unchanged; the shards' tones on the two test worlds (Aieliaalas II 44/2/3/1, Leileashphail III 28/4/17/1) and the C-02 doc's numbers; four unit checks (139); two scenes; the flow's `shards_last` and `shards_timeline` frames; `vesperis_test graves`; `galaxy/graves.*`; the guide's `grave` and `ended` lines.

## C-13 (2026-10-03): cultures per world: none, one or two

* **"Rarely" is one civilisation world in five.** The ideas note asked for two cultures rarely; a people's world is one star in eight near home, so a world of two is one in forty of the kind: a few in a long game, each a find. "None" stays the trait's absence (seven worlds in eight), not a trait world without a people: a world with settlements and no words would be a question the game cannot answer.
* **The second people is a second seed, not a second module.** `peopleGen` mixes the body's seed for the second people and leaves the first's alone, so the culture, the names, the span, the tongue, the voice, the tradition, the shuffle of the fifty, the charts' figures and the last recording of C-01..C-12 are each people's without one of those draws changing: no one-people world moved (the regress unchanged, the two test worlds' texts as they were). The lore carries the people's seed (`Lore::seed`, read by `loreSeed`) so the functions that take a lore give the lore's people whatever generator the caller holds; the seed-only ones take the people's generator. The generator is never handed to the planet function.
* **A people's fifty, not a split fifty.** A world of two holds a hundred, the second people's at the indices from fifty up, rather than twenty-five each: a people is a whole civilisation with its music, its charts, its last recording and a language learnt in ten shards, and a world of two is richer, not thinner. The guide's keys stay `<body key>/S<index>`; the decoder splits a world index into the people and its place with two inline functions.
* **The divide is a great circle through the least land, not continents.** The planet function labels no landmass and a flood fill over the globe would cost a tile; twelve poles drawn from the seed and forty-eight samples at 2 km along each find the circle that crosses the least land, which runs through the sea between two continents where there are two and across the land where there is one (the test world's crosses land at 23 points of 48: a frontier to walk). Found once a world and kept under a mutex, like the landmark cells.
* **They know of each other through the grammar alone.** The "other" kinds are appended to the kinds table and are candidates only where the lore names an other, so the one-people worlds' picks are untouched; the end-tone kind opens with the fate so the last recording of a world of two can be about the other. The catastrophe is a coin of the world's: shared, the second people's age is the first's (its decay from its own wet); apart, its own. The calendar is the world's for both: the day, the moon and the year are physical.
* **The decoder lists a world once a people and names them.** The people's name is already on the ship (the last recording's and the chart's log lines, a piece's title), so it is what tells two entries of one world apart; the world's system is generated once at the opening so the world is named as of two from the first shard of either. The second people's record ends under its own key (`/P1`) so each timeline closes on its own last.
* **The frontier's roads are the bigger settlement's build, and the radar hears both.** A road between two peoples' settlements takes the width, the paving and the wear of the people of the bigger settlement (the first's when they are of a class): the site keeps each road's paving and wear on the road. On the air a world of two gives the slot's people by a coin of the slot's, so a lock hears two voices over a quarter of an hour.
* **What moved.** `GEN_VERSION` 11; the regress unchanged; the chart captions on worlds whose season carries its article ("in the the rains" read doubled: the template says "in {season}" now, the draws the same); four unit checks (143); one scene; the flow's `shards_peoples` and `shards_second_people` frames; `vesperis_test peoples` and `second`; the third test world Eleinewai I (152 0 92 body 0); KI-354 (a chart of another people names the first people of such a world alone).

## C-14 (2026-10-03): lending

* **What travels is what the friend found and read.** A friend's `shard` keys with a `decoded` line: a shard taken and never read on the decoder is not lent (the friend has nothing to lend of it), and what the friend was lent themselves does not travel on, like the inbox's names (no chains), so every lent shard is credited to the explorer who found it. Nothing else of the friend's guide comes (their log, their landings, their graves stay theirs).
* **A lent shard counts in the language.** The friend's computer read it and the recording is on the ship now, so it is one of the world's held: it raises the share, it is read, spoken or played like an own shard, and its reading goes into `decoded`. The alternative, reading it at the friend's share with their gaps, would have made lent shards second-class and the language a per-explorer secret, which the lending is meant to undo.
* **Cyan everywhere, like the names.** The row, the timeline's tick, `LENT` after the header, a friend's name for a piece and for a world: one colour says "a friend's" across the guide, so no new label is needed.
* **Never taken.** A lent shard stays in the ruins: the world a friend read is still worth flying to, and taking the shard there makes it the explorer's own, the credit passing with it. The lent set keeps the key; "lent" means lent and not taken since.
* **One `Game` in the harness.** The flow's first draft imported into a second `Game` alive beside the first and the first corrupted (a bus error in the radar, then in the star map); the flow now swaps a fresh guide into the one game and puts the guide back. Two games alive at once are not supported by the game's globals, and the harness keeps to one.
* **What moved.** `GEN_VERSION` 11; the regress unchanged; `Guide::lent` (`lent` lines), `importInbox`'s `lentOut`, `Game::importInboxFile` (the menu's case and the harness), one unit check (144), the flow's `shards_lent` and `shards_lent_piece` frames; the decoder's lists byte for byte as they were where nothing is lent.

## W-01 (2026-10-06): the almanac

* **A menu with countdowns, not a toggle.** At x600 or x10000 no toggle catches a moment; `Ctrl+T` opens the list of the next events at the place and Enter runs to one. M5-06's 25 s time-lapse went with it: the list's free row (`RUN FOR 1 H`, a minute to five days) does what it did and says how long. `T` keeps its warp steps and stops a run; the data sheet's `T` and the flight computer's devices page open the same menu, so the almanac is reached from every place the clock matters, without a new key.
* **Every event is the geometry, read where the sky reads it.** A finder is a zero crossing or an extremum of a function of the system's positions (`galaxy/almanac.cpp` repeats `SurfaceSite::sun`'s formulas: the sun from the world's centre, the cover of M1-08, O0-01's ring rule, the landing map's shadow rule, the autopilot's lap for the ship), scanned at a step small against the shortest period the function moves with (the turn, the orbit, the ship's lap: a sixty-fourth, a thirty-second for the ship) and refined by bisection or a golden section to a ten-thousandth of a second, so the time listed is the time the geometry reaches it and the unit re-reads each event with the site's own sun and frame. A fast wobble on a slow motion (the place's turn under a moon's month: the parallax of an eclipse, the ship's lap under a moon's orbit) is found in two levels, the slow dips at the slow step and the fast minima round each at the fast step, so an eclipse a few minutes long is not stepped over and a locked world's list stays a millisecond. A phase is read from the world's centre, not the place: the parallax of a moon at five radii would give the lit share a local maximum every turn near the full, and a full moon is the world's.
* **Sunrise is the limb, dark is the sky model's own line.** The sun's rise is its upper limb at the horizon (no refraction: the game has no model of it) and a body's rise the same; the sky goes dark where `computeEnvironment` ends its glow, the sine of the altitude at -0.12, so `END OF TWILIGHT` is the moment the stars come out and `FIRST LIGHT` the moment the sky lifts, which on a world whose sun is wider than fourteen degrees comes after its limb: the list says so rather than lie. `NOON` is the transit (the hour angle through zero), and on a locked world the sun's events are whatever the libration gives, with the header's `THE SUN STANDS STILL HERE` when it gives nothing.
* **The run slows as a power of what is left.** The warp is `(remaining / 3 s)^1.5` clamped to x1..x10000, set again every frame: a day away runs at x10000, the last hour takes fourteen seconds of ours, the last minute five, the last three seconds run at x1, and the last frame's step is cut to the remainder so the clock lands on the event and not past it. A linear slowdown (`remaining / tau`) approaches the event exponentially and takes forty-six seconds through the cap; a steeper power skips the approach. The run goes on through the other screens the clock runs in (the flight computer, the decoder) and pauses in the ones it does not; a run is not saved (the save's warp reads 1: a loaded game at x4000 with nothing to run to would be a puzzle) and the wall clock refuses it.
* **The windows: five days on the ground, thirty from the ship; the first of each kind per body.** A ground's events recur every turn and every month, so five days holds a cycle of everything but the ring shadow's season; the ship's events (a comet's plunge, the planets meeting) run on the planets' periods of hundreds of hours. A moon's events from the ship are searched two days or three orbits ahead, whichever is longer, since they come round every orbit and the scan at the lap's step over thirty days cost a hundred milliseconds. The list is the next of each kind per body, so a world with four moons reads twenty lines rather than a month of risings.
* **The landing map's almanac is the cursor's.** The place on the landing map is the cursor's site (`landBody`, `landLat`, `landLon`), so a landing can be timed: wait for the dawn at the site and land into it. From the ship the observer is the parking and the lap of the orbit mode is followed, since a transit of a moon across the parked body depends on where the ship stands round it.
* **A parking never sees a transit.** The ship parks at 3.5 radii (outside the rings and 1.3 radii) and every moon orbits farther out, so a moon is always beyond the parked body's centre from the ship: it passes behind the disc (`BEHIND`) and never across it. `AL_TRANSIT` stays in the registry for the telescope of W-06, from which a view across the system sees the inner moons cross.
* **What moved.** `GEN_VERSION` 11; the regress unchanged (nothing it hashes is drawn differently); `GameState::ALMANAC`, `Game::almanac*`, `timeLapse` and `timeLapseUntil` gone, the save's `warp` line 1 during a run; the help's two lines, the keys screen's `WARP / ALMANAC`, the data sheet's footer, the devices page's eighth row; eight unit checks (152); the flow's five frames in place of `timelapse`; `vesperis_test almanac`; KI-355.

## W-06 (2026-10-06): the telescope

* **A magnified attitude, not a camera of its own.** The telescope is the ship's own view with the focal length multiplied (`Game::telescopeProj`, powers of two from x2 to x1024): the same renderer, the same bodies, the same star field and backdrop, nothing drawn twice or differently, so what the eyepiece shows is what the window shows, closer, and every body's screen radius is exactly the power times the window's (the harness checks it). The cabin is left behind as in the radar camera: the eye is at the eyepiece.
* **Powers of two, to x1024.** Steps by two are what the wheel and `+`/`-` want; ten of them from x2. x1024 is a field of 0.08 degrees at the 70 degree setting, where a planet eight AU across the system (0.04 degrees) is a globe of twenty pixels at 1x and a moon of a giant from the giant's parking fills the frame. Deeper would show the planet map's texels (512 round the world, 0.7 degrees each; the albedo and the heights are read bilinearly already, the materials are not) and would cost the same (a globe over the whole frame is the cost whatever the power: the `telescope 2x` budget of `bench check`), so the cap is the map's, not the renderer's.
* **The telescope tracks what it is pointed at.** From a synchronous parking the ship circles its body in ten minutes, so everything but the parked body sweeps the sky at half a degree a second: across the field in a second at x16, in a frame at x1024. Opening the telescope tracks the body under the crosshair at once (`telescopeAim`: the ship re-aimed at it every frame, the mouse moving the view's offset from its centre), Enter lets go or tracks the reticle's target, so the eyepiece never needs the chase mode (`O`) to hold a moon. An approach's own turn is left alone: tracking stops while the autopilot flies.
* **The reticle picks as the crosshair does, in pixels.** A body or the sun within 18 px of its disc, a star within 14: the local target's and the aim's own rules, computed from the attitude every frame rather than read from the last frame's `bodyInfo`, so the pick shrinks in angle as the power grows and a crowded field resolves into single picks.
* **`Z`, since no letter was free and `Z` is the zoom elsewhere.** Every letter has a binding in space; `Z` is the surface's "stand tall" and the landing map's sector zoom and nothing in space, and a zoom key reads as a zoom. The flight computer's devices page carries the telescope too, as it carries the radar and the almanac.
* **A photo is the screenshot, named by its target.** No new path: `P` in the telescope is `P`, the caption says `THROUGH THE TELESCOPE X64: <the reticle's line>` and the gallery shows it; `N` names the target through the text entry the guide uses, a moon, a comet and the second sun with a prompt of their own (and `NAMED A MOON ...` in the log), a star of the neighbourhood with a kind of its own (8), the first naming of a star that is not the current one.
* **Not saved.** Like the radar, the telescope is a thing the explorer is doing, not a state of the ship; a load, a Vimana flight, the surface, the landing map and the radar stow it, and the radar and the telescope exclude each other (one instrument at the window at a time).
* **What moved.** `GEN_VERSION` 11; the regress unchanged (nothing it hashes is drawn differently: the window's projection is the setting's whenever the telescope is stowed); `Game::tele` and `game/telescope.cpp`, text entry kind 8 and kind 2's prompts by the body, `choosePaletteBodies` giving the telescope's target bank 2, `Z` in `updateSpace`, Esc and the put-away rule in `frame`, the devices page's ninth row; harness: `space` renders `telescope_moon/giant/companion/comet.png` (`renderTelescopeFrames`, each at the power of two that fits it), eight unit checks (160), the flow's `telescope`, `telescope_moon`, `telescope_name` and `telescope_named` with the gallery's second picture, `bench check`'s `telescope 2x` (16 ms). KI-356 for what stays.

## W-06 reworked (2026-10-06): the plate, the set, the colours

The user's review of the telescope as first built: "feels like a poor zoom in and that's it, also want separate computer like for signal radar. I expected that you would come up with sophisticated zoom in that would render terrain from stardrifter's camera telescope" and (R-404) "can you make computers at each wall have different unique color so it's easier to find the one you need?". The magnified attitude stays (every decision above holds); what changed:

* **The eyepiece draws the planet function, not the map.** The first build magnified the world map (512 texels round the world, 11-44 km each), so past x8 at a parking the globe was a blur of texels: a zoom and nothing more. The planet function is one function of position down to the metre ("what you walk on is what you saw from orbit"), and at 1-3.5 us a sample a frame of it is a few hundred milliseconds on the workers, so the eyepiece samples it again at its own footprint: the plate (`DetailPlate`, `02-rendering.md`), a body-fixed patch round the aim point built in blocks of 8, 4, 2 and 1 texels within 8000 samples a frame (two to four milliseconds), the map standing in until a cell is filled and the previous plate kept as the fallback. The picture forms at once and sharpens over a second or two (`EXPOSING 46%` on the frame: an exposure, as a camera's), and a photo reads the plate out whole first. Body-fixed rather than screen-fixed so the body's turn, the ship's orbit and the mouse move the lookup, never the plate: the picture never tears, and a step out finds the coarser plate still there. The rivers and the lakes (the drainage's tiles, a second or more each) come in the background for steps under 2 km over at most 200 km of ground, the plate begun again when they are in; a coarser plate shows the valleys without them, which is what the function gives at that scale anyway.
* **Only where it is finer than the map.** No plate when the eyepiece's step is coarser than three quarters of the map's texel (a disc under about 80 px at 1x): the map is the function at that scale already, read bilinearly, and the plate would cost samples for nothing.
* **The relief shading's weight falls with the step.** The map's `5 x slope` is right for slopes over fifty kilometres (a few hundredths); a plate at a hundred metres has slopes of a half, which would saturate. The plate weighs `2.5 + 3.5 smoothstep(200 m, 1.5 km)` (the first try, 2 to 5 over 300 m to 30 km, left kilometre steps flat; 6 everywhere saturated a snowy range at 300 m): cliffs still read, hills do not go black.
* **Sized from the field's rays, body-fixed.** The plate's extent comes from where the frame's corners and edges meet the sphere, in the plate's own axes (the screen's x projected on the tangent plane), not from the footprint times the frame: an oblique view stretches the ground along one axis (at sixty degrees off the vertical, twice), and a plate sized face-on showed as a tilted band with the map's blur round it. A ray past the limb counts for two and a half fields at most, so looking past the limb at a deep power does not ask for the hemisphere at that step; a plate over the cap takes a coarser step rather than a smaller extent. What the plate shows at a step is what the function holds at that step: a moon's ground from its giant's parking (3.7 km a pixel at x1024) is smooth, since the relief's longest wavelength is 16 km and fades under four pixels; the same ground from the moon's own parking (80 m a pixel at x512) is the terrain. The harness's ground frames are taken from the bodies' own parkings for that reason.
* **B-407: the relief's sign.** Writing the plate's shading from the map's showed the map's term added where it should subtract: ground rising toward the star faces away from it. Both take the term off now (the plate's held within +-0.7); at the map's texels the term was a few hundredths and the crater-or-dome illusion hid it, at the plate's steps it would have lit every ridge from the wrong side.
* **A deep power holds the ground, not the centre.** The bench at x1024 on the parked world showed the plate begun again every few frames: from the parking the ship laps its world in ten to twenty minutes, 70 km/s of ground at a few hundred pixels a second through a 13 km field. Tracking the centre is right while the disc is in view; once the tracked disc is twice the frame's height (x8 on a parked world) the hold is of the ground under the reticle, a body-frame point the ship is re-aimed at every frame while it passes over, so a mountain range is held for the four minutes it is above the limb with its perspective changing, and the plate, body-fixed, serves throughout. The mouse pans the held point; past the limb, or back under the power, the hold is let go with the view kept. The alternative, the fixed-point chase (`O`), stays what it is: the ship's parking, not the eyepiece's business. (Superseded the same day by R-405's stabiliser, the section below: the ground hold is gone.)
* **Not displaced.** The plate changes what each ray's point on the sphere looks like, not where the sphere is: a mountain at the limb does not stand out of it (its shading does). The limb at the finest steps is a small part of what the telescope shows; a displaced globe is a renderer of its own, left in KI-356.
* **The telescope's own set, the radar's twin.** On the cabin's back wall at the end the radar is not (x 1.0..2.1, `facing` 9), `E` looking through it as `E` at the radar sweeps; the fuel orb, which stood in that corner, moved to the front-left corner. The frame is the camera's (`CAM 04 TELESCOPE`, the visor, the brackets, the scale bar, the exposure), in place of the space HUD as the radar camera's is: an instrument of its own, which is what the review asked for.
* **R-404: every computer in a colour of its own.** One green phosphor for eight screens made them alike from across the room. Each has a bank (9 and 21-27) and a colour chosen pairwise at least 0.3 apart in RGB (`unit` checks it: the first choice, an orange landing map beside a red radar, was 0.18): the console amber, the flight computer green, the star map blue, the landing map pink, the target data cyan, the decoder violet, the radar red, the telescope ice-white; the `E` prompt in the same colour. Seven banks of the eleven that were free; 28-31 remain.
* **What moved.** `GEN_VERSION` 11 (nothing generated changes: the plate samples the function the maps sample); the regress unchanged (no plate without `detailBody`, which only the telescope sets; the cabin is in no hashed frame); `SpaceContext::detailBody/detailBudget`, `SpaceRenderer::plates`, `updatePlate`, `plateFill`, `plateInfo`; `Game::telescopePlateBody`, `telePlateFinish`, the camera frame in `renderTelescopeHUD`; `cabin.cpp`'s `SCREENS`, the set, `facing` 9; harness: `space` renders `telescope_world`, `telescope_world_land` and `telescope_world_ground` (a living world's disc, its high ground at x64 and x256), `telescope_moon_near` (x256) and `telescope_ground` (x1024) beside the four of the first build, each plate frame with a `_map` twin where the difference shows, `plateInfo` printed with each; the flow's `cabin_back`, `cabin_telescope`, `telescope_set`, `telescope_plate` and `telescope_plate_deep`; eight unit checks more (five of the plate, the ground held, the set, the colours).

## R-405 (2026-10-06): the stabiliser and the orbit keys

The user's review of the reworked telescope: "when you are in telescope planet still moves and it creates shaking. How I suggest you to fix it - when you enter telescope mode planet completely locks on the side you are located on - it should feel like it's not moving at all while in this mode (lore-wise it should be something like comprehensive stardrifter stabilization system). Also ... an ability to rotate stardrifter around the planet. But then you can just enter telescope and you will lock on the side you are facing."

* **The frame is held, not a point.** The ground hold re-aimed the ship at a body-frame point every frame while the parking lapped the world at 70 km/s of ground: the point stayed, everything round it slid and the lower powers (the centre tracked) streamed. The stabiliser holds the whole geometry instead: the parking direction and the view's axis as body-frame vectors (`tele.parkBF`, `tele.viewBF`), the parking rebuilt from the body's frame every frame in `updateShipMotion` and the attitude from the view's axis in `updateTelescope`, so the ship rides the world's turn and nothing moves at any power but the light and the clouds. Lore: the Stardrifter's stabilisation system. The roll is not held (the camera has no roll): a fraction of a degree over minutes, nothing at the reticle.
* **The side faced is the side held.** Opening the telescope no longer brings the parked body's centre under the reticle: the attitude is kept and held (Enter still centres it). Other bodies and the sun are tracked on their centres as before, since they move in their own orbits.
* **Only a parking is stabilised.** In standby or on the approach the ship holds nothing (it is not at a body); the frame is taken when it arrives at a parking with the telescope out. The ground hold went with it (one mechanism: `groundLock`, `telescopeLockGround`, `telescopeAimGround`, `telescopeUpdateLock` removed).
* **The lap goes on when the telescope is stowed.** The orbit mode (`O`) is the parking's business as before; the stabiliser only holds it off while the eye is at the eyepiece, and the lap resumes from where the ship is (the parking co-rotated with the ground meanwhile, no jump).
* **Shift+arrows carry the ship round the world.** Left/Right along the parallel (about the spin axis, so the world turns in the window like a globe on its stand), Up/Down along the meridian within 85 degrees of the poles (past them a yaw-pitch camera swings), twenty degrees a second (a lap in eighteen seconds: fast enough to go round, slow enough to read the ground); the attitude turns with the ship so the world stays where it was in the window. The arrows alone turn the ship as before; the keys are refused in the telescope (its arrows steer the view) and in a belt (no world). The status reads the ground under the ship. The help's flight page says `SHIFT+ARROWS  ROUND THE WORLD`.
* **What moved.** `GEN_VERSION` 11; the regress unchanged (no hashed frame has a parked ship under the stabiliser or the keys); `TelescopeState::frameBody/parkBF/viewBF` in place of `groundLock/groundUnit`; `Game::orbitShip`, `telescopeHoldFrame`, `telescopeHoldView`, `telescopeStabilised`, `telescopeTrack(id, recentre)`; `SpaceRenderer::plateBegun`; the hooks `testTelescopeStabilised`, `testTelescopeGround`, `testGroundScreen`, `testSubShip`, `testParkDir`, `testSpinAxis`, `testPlateBegun`; the unit's telescope checks rewritten round the stabiliser (four more: the lap goes on, Shift+Right, Shift+Up, opened after the turn), the flow's `orbit_turn`; KI-356.

## W-03 (2026-10-06): weather that arrives

* **One function of (body, place, time).** A front is a point, a heading and a speed hashed from the body and the time (`galaxy/fronts.*`: slots of 3-24 by the world's size, a front every two to four days per slot, born on a mid-latitude, running along a great circle east with a poleward lean in the mid-latitudes and west in the tropics at 8-25 m/s for a day or two, 1200-3000 km long, bowed), and its band is a profile by the distance ahead of its line (the deck from 280 km ahead, its solid wall from 90, the veil from 520, the wind's rise from 350, the rain from the line itself to 150 km behind, the cold behind). The globe's overlay (`buildFrontMap` on the world map's grid, built again when the clock has moved four minutes), the landing map's band, the ground's weather (`frontWeatherAt` in `computeEnvironment`), the data sheet's forecast and the almanac's finder (`frontsAhead`: the distance ahead through zero, scanned at ten minutes and bisected) read the same numbers: the plan's consistency principle. A new hash stream beside the planet function: nothing older moves and `GEN_VERSION` stays 11.
* **The line is the arrival.** The forecast's time and `A FRONT FROM THE W` are the moment the front's line reaches the place, which is where the rain profile begins (zero ahead of it, its peak 25 km behind), so the unit reads the ground's own rain twenty seconds either side of the time given; the first drops follow within minutes, the deck and the wind come hours before. `THE FRONT CLEARS` is the rain's end (150 km behind the line, or the front's death). A front dying or ending at the place (its life under a quarter, its lateral taper under a half) is not listed.
* **Rain where it rains, dust where it is dry, cloud and wind on a quartz world.** Felisian, ocean and acidic worlds take the front's rain (snow and hail by the old cold rules), desert and thin-atmosphere worlds a dust front through the dust storm's own code (the visibility, the tinted palette, the howl), quartz worlds the band and the wind alone; the opaque decks (venusian, hydrocarbon), a tectonic world's thin sulphurous sky and the airless get none.
* **The pattern rains by its own rule; a front rains from its line.** The front's cloud raises the cover (`env.cloudCover = max(pattern, front)`) but the pattern's rain, snow and hail rules read the pattern's cloud alone (`env.cloudPattern`): otherwise the deck 150 km ahead of the line would already rain at 100% and there would be no wall.
* **One line in local metres drives the sky, the ground and the fog.** `LocalFront` (the nearest front's motion projected on the site's east and north, the distance ahead at the player) gives every point of the site its distance ahead by a dot product: the sky's deck is thresholded per cell where its ray meets the layer (the band moves with the front, the texture with the wind), the ground's cloud shadow per vertex, the wall of rain is the extinction of the ray through the rain's box (three quarters of the peak, less the rain already in the fog at the camera; a visibility of 1.5 km in rain, 400 m in dust) on the terrain, the water and the trees, and the curtain under the deck is the same box on the sky's rays to the cloud base, drawn over the clouds since the rain hangs in front of their base. The rain at the camera, the lightning and the thunder are the old ones.
* **A storm bank.** The sky bank runs from the zenith's colour to the horizon's, and on a red dwarf's mauve world the two are nearly one: nothing drawn in it can be dark, and the first frames of the deck were invisible (measured: the deck's pixels four levels of 255 from the sky's). Bank 23 shares the sky's stops from the horizon up and goes down to a storm's grey (dimmed with the rain); a pixel whose deck or curtain share passes a half takes it, so the lit tops look as before and the bases and the wall read dark on any sky; the pattern's clouds keep the sky bank: a front's deck is a storm's. The sky's colours also dim by 30% of the front's cloud at the site, as the rain dims them by 35%.
* **The texture is the world's, gated by the core.** A thresholded cloud noise of the world (features of hundreds of kilometres) thins the band outside its core (90 km ahead of the line to 130 behind), so from orbit a band is streaks and patches with a bright core and frayed ends rather than the flat ribbon of the first build, and the ground under the outer deck sees the same patchiness; the core stays solid so the wall is a wall, with an edge of ten kilometres.
* **The scene site is searched, then pinned.** The pinned plain lay under the pattern's own overcast and the deck's edge never showed; `frontSceneSite` scans the first felisian world's hashed land sites for open ground with hills within 16 km, a clear pattern sky and a strong front by day, printed to pin (`PIN_FRONT_*`: -26.555 / 166.809 on Wyariothmai I, a locked world, so four frames hours apart are all lit). The frames: the wall 18 km off (a dark band over the horizon), four minutes before the line (the deck overhead, the light down), the rain's peak, an hour after the rain's end facing where it came from (the sky clearing).
* **The overlay is read where a band can be.** A bilinear read of the overlay for every pixel of a globe that fills the frame cost five milliseconds at 2x (`telescope 2x` read 16.4 ms against its 16), so `frontsFor` keeps a mask of the 16 x 16 texel blocks that hold any band (a texel's neighbours counted) and a pixel off every band costs one byte; the read itself folds the longitude once instead of taking two modulos and returns at once when its four texels are zero (12.4 ms).
* **What moved.** `GEN_VERSION` 11; the regress unchanged (the frames it hashes hold no front; a build changed `scene_felisian_shore` through the veil's ramp and the rain's darkening of the cloud bases, and both were put back to the old formulas where no front is present); `galaxy/fronts.*`, `AL_FRONT`/`AL_FRONT_CLEAR`, `SurfaceEnvironment::cloudPattern/frontCloud/frontRain/frontCold/frontAhead`, `LocalFront`, `SurfaceView::rainWall`, bank 23, `SpaceRenderer::frontsFor`, the landing map's and the zoom's bands, the sector map's line and hatch with `RAIN FRONT  IN 2 H 05 MIN` in the legend, the data sheet's `FORECAST`, `Game::updateForecast/forecastLine`; harness: the scenes `front_far/arriving/through/behind`, `space_fronts.png` and `_x4`, six unit checks (178), the flow's two `fronts:` prints; KI-357.

## B-408 (2026-10-07): one bank under the glow of an airless sky

* **The cause was two banks under one glow.** The suns' glows add their shades in the bank a pixel holds. Over the galactic band's patch that was the band's ramp, which rises from black at once; beside it the airless sky's, black up to shade 30 (the glow's tail, the halo round the sun and a comet's coma lie there unseen). So a glow showed as a broad pale fan over the patch and not beside it, and the patch's own edges, the contour where its light passed the sky's, drew the fan's straight sides. A comet made it plain: its coma lifts the whole sky's shade without lifting its light, so the patch ended in arcs and triangles round the sun, and the mush, averaging shades across an edge, rimmed every edge in light.
* **The glow on the band's bank, not on the sky's.** Drawing the suns before the band, so the glow always lands on the sky's bank, was tried and dropped: the glow's tail is then black everywhere, and where the band runs behind a sun it had to stop short of the glow (a black disc in the Milky Way round the sun, the Wolf-Rayet star's shell gone, S-05's ring having shown only through the band's bank) or meet the glow's visible edge with the mush's rims. The band's bank under the glow was the look of most airless skies already (the band's wings cover much of the sky), light adding to light, so the whole airless sky goes to the band's bank and the glow lands on it everywhere.
* **Carried over by light, not by shade.** The sky's own shades on an airless world (the coma's 6-20, the sun's halo up to 22, a ring arc 36-60) move to the band's bank at the intensity of equal luminance (a table per frame), so what was unseen stays unseen; moved as shades they would have lit the whole comet sky grey and put a broad halo round every airless sun. The stars are drawn before the change and are masked by the sky's shades as before.
* **The sun in the band's bank too.** A disc in the sky bank beside a glow in the band's mixed through the sky bank's grey corona shades (30-44) at its edge, a dark ring round every airless sun. On an airless sky the primary's disc, glow, eclipse corona and flare are drawn in bank 22, whose ramp keeps the band's star-grey under 20, turns to the sun's colour above it (at the grey's light, so the glow is as bright as before) and takes the sky bank's sun stops from 55, so the disc's colours are the sky bank's. The pixels the disc covers keep the sky's shade, which `drawSun` reads to keep the disc as bright as the glow-lit sky round it (on the comet the coma's shade washes the disc white, as it did beside the band).
* **Two stars keep the sky bank.** A black hole has no light: its halo is carried over as nothing, its pixels are not kept (its lens shows what lies behind it, and a kept black spot behind the hole came out as a dark Einstein ring), and its disc keeps the sky bank's colours. A protostar's cloud lies over the whole sky in the band's bank tinted to the cloud's tone (S-04), its glow landed on it everywhere already, so its sky keeps that ramp and its disc the sky bank.
* **Down to the ground's horizon, behind the bodies.** The band stopped at the horizontal, a straight edge across the sky of a small body (the ground of a 2 km comet falls away within a hundred metres); it now reaches the frame's bottom on a small body and the floor's horizon on a planet (`floorHeight`, computed once a frame before the sky), and it skips a sky body's night side as the stars do (a dark moon showed the band through it).
* **What moved.** `GEN_VERSION` 11; the regress re-blessed for `scene_cratered_noon` and `scene_molten_night` (airless: the band's wings now run on where they were cut, a comet's tail in the sky is whole), the other twenty unchanged; worlds with an atmosphere are drawn bit for bit as before (four scenes compared); `SurfaceView::floorHeight/floorH`; KI-358.

## W-04 (2026-10-07): the nights

* **A shower is where two orbits pass.** A comet sheds dust along its whole orbit; `meteorShowersOf` (`galaxy/nights.*`) samples the comet's orbit by true anomaly against the world's (a moon's: its parent's), measures the least distance in the world's plane radially and out of it, refines it (golden section), and keeps a stream when `age x shed x exp(-moid^2 / 2 w^2)` is 0.08 or more: `w` three hundredths of the world's orbit, `shed` the comet's activity at its periapsis (`0.3 + 0.7 cometActivityAt(q)`), `age` 0.5-1 from a hash of the comet's own seed (the only number not from the orbits). The peak is the world at the point of its orbit nearest the comet's, once a year, its width the stream's over the world's speed (5 minutes to a day). One planet in ten near home has a shower (196 of 1899, the `showers` scan), 0.09 ms a world. Nothing generated moves: `GEN_VERSION` 11.
* **The radiant is the stream's velocity relative to the world, reversed**, in the system's axes, so it rises and sets with the stars; the meteors light up 6-70 degrees from it and run away from it along great circles, shorter paths near it, in quarter-second slots hashed with the stream (`showerMeteorsAt`: the sky is the same at a time). The rate is 60 a minute for a full stream at its peak with the radiant overhead, times the sine of its altitude: the sporadic meteors are seven, so the peak is plainly a shower, and an evening of one is "a few a minute" before and after. The streaks go where the sporadic ones go (the stars' bank, airless worlds too: M10-14's rule kept, KI-359).
* **The almanac's `<COMET> METEORS PEAK`** (`AL_SHOWER`, the name the explorer gave the comet or UNKNOWN): W-01's registry takes the finder, the ground's list only.
* **The zodiacal light from the dust a system has**: a young star's (a protostar 1, a Wolf-Rayet star 0.5, a blue giant 0.4, a blue-white star 0.3, others 0.1) and a belt's near the world's orbit (`0.8 exp(-(ln(r / mid) / 0.45)^2)`), varied by a hash of the star; 0.12 and over is drawn (two systems in five near home, mostly by their belts). A cone along the plane of the worlds: brightness `0.25 (0.6 / e)^1.3` of the band's full measure at the elongation `e`, falling with the latitude nearer an exponential than a bell over 16 degrees (the dust near the sun shines all round it, far from it only near the plane, so what shows is broad over the sun's place and narrowing up), and the faint patch opposite the sun. It joins the band's light in the band's pass (the same fade by day and rain, the same clouds), read on the sky's cell grid: per pixel it would have cost milliseconds at 4x.
* **The earthshine and the ground's planetshine are one formula.** `bodyShineAt` (M10-09's: lit share times albedo times apparent size, a substellar body's own glow) gives the night ground its light and a moon's night side its parent's (`drawGlobe`: in space added to the shade past the terminator; in a sky drawn at 30-42 of the sky bank, just over its black and under the lit side's 43, where it beats the sky's own pixel, so a day sky hides it). A thin crescent shows the rest of its disc; the scene's 15-degree moon of an icy world reads as the old moon in the new moon's arms.
* **The shadows of rings and moons stay the site's.** The site's eclipse and ring shadow (M1-08, O0-01) and the landing map's per-place shadows (`buildShadowMap`) already carry them; a shadow edge crossing the ground within a view is not drawn: the game's suns are about four degrees across at a typical world, so a moon's or a ring's penumbra on the ground is thousands of kilometres wide and would change a 100 km view by a few percent. The moonlight's shadows of the ground were built and dropped: on the night ground the light is mostly the ramps' planetshine lift (M10-09), the directional part a few levels, and the shadows moved one pixel of the scene; making the moonlight directional would change every moonlit night (KI-359).
* **What moved.** `GEN_VERSION` 11; the regress re-blessed for `scene_molten_night` (an airless night of a dusty system: the zodiacal glow lifts the sky one to three levels), the other twenty-one unchanged; `galaxy/nights.*` (`MeteorShower`, `meteorShowersOf`, `showerActivity`, `showerPeakAfter`, `orbitDistanceKm`, `orbitPointAt`, `zodiacalDust`, `zodiacalLight`, `bodyShineAt`), `cometActivityAt`/`cometRefKm` in `galaxy/system.*` (the three copies of the comet's activity made one), `AL_SHOWER`, `SurfaceView::showers/zodiDust/zodiGrid/showerMeteorsAt`; harness: `showers`, the scenes `meteor_shower` (with `_20s`, twenty seconds in one picture), `zodiacal_light`, `earthshine`, `moonlit_night`, four unit checks (182); KI-359.

## X-01 (2026-10-07): the probe, launch to end

* **Unlimited probes, one falling at a time** (the user's answer at X-01's start, 2026-10-07: "unlimited probes"). No stock, no restocking: a giant is something to look into as often as wanted, and every descent is still its own (the probe's seed draws its hull's limit and its lightning; the aim is the explorer's). The relay follows one probe: `C` at the giant watches the falling one instead of sending a second.
* **The capsule's key and its places.** At a giant `C` launches a probe where the capsule is refused (`SURFACE NOT CONSISTENT`); the landing screen on the right wall and the capsule cage say `PROBE SCREEN` / `PROBE BAY` there and `PROBE RELAY` while one falls; the flight computer's row reads `DEPLOY THE CAPSULE OR A PROBE`. No key collides (the iteration's rule asked to check first): a giant has no landing map and the capsule never deploys there. The relay's screen is the landing screen (pink, R-404's colour), so the cabin has no new set.
* **The telescope aims it.** With the telescope out and its reticle on the parked giant, the probe goes to the ground under the reticle (`viewGround`, W-06's reticle ray, now shared with the harness's check); without it, to the ground under the ship. The telescope's frame says where (`C SENDS A PROBE TO ...`).
* **The relay's clock is real time.** The fall, like the capsule's descent and every transition (the decision table's "real-time durations for flights, approach, descent"), runs on real seconds: the warp turns the sky over the probe (the sun crossing the deck, the night) and not the fall. It runs whenever the game simulates (the probe's screen, the cabin, the ship's screens) and stops in the menus.
* **The ship holds its parking**, or the relay is cut: an approach, a Vimana flight or another star end the probe (`THE PROBE'S RELAY IS CUT`, a log line). Simpler than a relay with a range, and the parking is where the explorer watches from; Shift+arrows (R-405) still carry the ship round the giant.
* **Closed-form descent.** The fourth root of the pressure grows evenly with the clock (an e-fold of pressure takes `50 s x P^0.25`), a quarter as fast under the chute (opened once after the entry, holding over a 4.5-fold growth unless cut); the end is the hull's limit (18-32 bar by the probe's seed) or 750 K, the shallower. Every number on the screen is a function of the clock, so a save is the clock and the chute's two clocks, the harness skips by setting the clock, and the end is found exactly. The pace is the game's compression (planets turn in hours here): seven or eight minutes free, about four more with the chute, against the idea's "twenty minutes"; X-02 can retune `PROBE_TAU1` once the stages are worth more time.
* **The giant's air is derived**: the temperature from the equilibrium's 1.5 (an adiabat under the tropopause, a warming stratosphere over it), the heights from the scale height, the decks fixed until X-03 (since then by condensation, below), the wind's sense and shape from the globe's zonal jets but its strength the giant's own (80-450 m/s at one bar by a hash): the globe's drift is drawn in the game's compressed spin and would read kilometres a second. No generation bump; brown dwarfs fall like gas giants until X-03 gives them their glow.
* **A falling probe is saved; a held last frame is not.** The `probe` line carries the clock, the aim, the chute's clocks, the seed and whether its screen was open (a save from the screen loads onto it). The last frame waits on the landing screen until it is seen (`PROBE LOST - ITS LAST FRAME IS ON THE SCREEN (C)`), then Enter or Esc let it go; X-04 will keep it as the final image.
* **Placeholder stages** (superseded by X-02 the same day: the probe's own picture from the decks' fields, composited in RGB). Above the clouds the space renderer draws from the probe's place (the giant's globe, its rings and moons: the idea's first stage is the sky bodies of the space renderer); below about 0.03 bar a pass of its own (`space/probe_view.*`) draws the sky by elevation, the deck under the probe as a lit plane of billows hazing into the horizon, the fog inside a deck rising past the lens, the overcast and the floor between the decks with the lightning in it, the rain and the lamp in the water deck, the dark under it; six looks blended by the pressure colour it from the giant's own colour. Two seams were met on the way: a deck's bank meeting the sky's at the horizon (the decks' ramp shares the air's from shade 44 up, B-408's lesson) and the light of the depth darkening shades instead of colours (the shades on both sides of the horizon must stay those that agree; the day and the depth scale the ramps, the frame's light being the larger of the day's, the lamp's and a flash's). X-02 replaces the pass with the real decks.
* **The camera's frame** is the vehicles' (`CAM 05 PROBE`, as the radar's 03 and the telescope's 04) with the readouts in the surface HUD's units (`ATM`, `C`, `KT` toward a compass point) and a `SIGNAL` bar that empties with the depth while static and snow grow over the picture.
* **What moved.** `GEN_VERSION` 11; the regress unchanged (nothing it hashes is drawn differently); `galaxy/probe.*` (`GiantAtmosphere`, `giantAtmosphereOf`, `giantTemperatureK`, `giantAltitudeKm`, `giantWindEast`, `giantStageAt`, `giantSunlightAt`, `ProbeFlight`, `probeCrushBar`, `probeDescentStart`, `probeBarAt`, `probeEndBar`, `probeEndClock`, `probeChuteCloseAt`, `probeStageAt`, `PROBE_STAGE_NAMES`), `space/probe_view.*` (`ProbeView`, `renderProbeView`), `game/probe.cpp` (`Game::ProbeState probe`, `launchProbe`, `watchProbe`, `updateProbe`, `updateProbeScreen`, `renderProbe`, `renderProbeFeed`, `renderProbeHUD`, `probeAudio`, `probeCut`, `probeEnd`, `probeRelease`, `probeCaption`), `GameState::PROBE`, `Game::viewGround` (from `testTelescopeGround`), the `probe` save line, `AudioState::probe*` and `AudioSynth::probeSample`, the help's `C CAPSULE / PROBE`; harness: `vesperis_test probe [<sx> <sy> <sz> <body>] [chute] [night] [wav]`, four unit checks (186), the flow's `probe_*` frames and the load of the probe's save. KI-360 holds what stays.

## X-02 (2026-10-07): the descent's stages

* **The decks are fields, not planes.** `GiantDecks` (galaxy/probe.*) gives each deck a top and a base at any place round the aim point (km east and north, heights over the one-bar level) and a density inside: the ammonia deck's top lifted in the zones and dropped in the belts by the globe's own albedo there, a swell, cloud streets along the bands, the convection's cells, five octaves of billows (the folds of |noise|, dropped under the sampling's spacing) and thunderheads (one at most in a cell of a 52 km grid, more in the belts and the storms, their sides lumped, their tops an anvil with an overshooting dome); the bases hang in pouches; inside, layers and caverns. The water deck has no bands (the same under every band) and taller towers held under the ceiling. Everything scales with the scale height (`sh`, the plain giant's 25 km = 1, 0.2 to 2.5): a brown dwarf's decks and relief are a quarter of a plain giant's (X-03 gives them their own character).
* **The bands are the globe's, sampled once.** `GiantBands` is the planet function (the globe's `sampleSurface`, its polar vortex) on a 160 x 160 grid of the tangent plane out to 4800 km round the aim, at the jets' drift of the launch's time (`giantJetTurn`, now the one definition the globe's lookup calls too), built at the launch and after a load (7 ms on the workers). So the band and the storm the telescope aimed at are the ones fallen into, and the deck's colour is the globe's material ramp of the giant's colour. The storms lift the deck (+4.5 sh km at a storm's heart) and breed towers; a storm's wall seen at its edge waits for X-03's storms of their own (the grid's 60 km cells make the edge a slope).
* **A fast noise of its own.** The decks are sampled a hundred thousand times a frame at kilometre scales where the planet function's double-precision noise buys nothing: `cloudNoise2/3` (a 32-bit hash, a gradient table, single-precision corners) is ten times as fast (4 and 9 ns against 40 and 60) with the same spread. The world's generation does not use it.
* **Three passes at the logical resolution.** The panoramas (a deck's top seen from above or its underside from below, marched outward one azimuth at a time with each sample filling the rows it rises over: the voxel-space idea, exact for any pitch since rows are elevations), the fog (a march along the rays every fourth logical pixel through the deck the probe is in, with the light coming down through it and the lamp on short steps of its own) and the nodes (one a logical pixel: the surfaces under the haze, the sky, the rings, the cirrus, the fog in front), all on the workers, the work taken a piece at a time by whichever thread is free (an even split waited on the efficiency cores). The frame reads the nodes bilinearly: at 4x the picture is as soft as the mush makes everything else, and the cost does not grow with the scale (2x 4-8 ms a frame in the descent, 4x 5-13). A mesh of the decks through the rasteriser was weighed: many more vertices for the same towers, and a tall tower breaks a projected grid.
* **Composited in RGB.** A fog over a deck of another colour, the haze over both, a flash's light: these mix colours, and a bank holds one colour per shade (the mush averages shades across banks, B-408's lesson: X-01's placeholder had to share ramps to hide its seams). The probe's picture is a feed already composited in RGB (X-01's crossfade and static), so its layers are mixed there; every colour still comes from a 64-shade ramp (`ShadeRamp`, the globe's material ramp for the decks), at the logical resolution. The space renderer draws what lies beyond the air (its stars dimmed by day, the sun's disc, the moons in their textured banks; `SpaceContext::excludeBody` keeps the giant out of `render`) and shows through as much as the ray's optical depth lets it; it is skipped once nothing of it can show. Banks 28-31 are free again.
* **Stable by grids.** A panorama's columns and rows lie on fixed angular grids and its steps on a fixed grid of distances (2.2% each past 0.9 sh km), so neither a turn nor the fall resamples the clouds; `probe stability` finds under 0.03% of the pixels jumping by 40 in a frame over the deck's tops, inside the deck and between the decks (the readouts' digits).
* **The light and the camera.** The clear air's haze is an optical depth by pressure shared with the sunlight (`giantHazeTau`: a little in the stratosphere, the upper haze, the clear band's), integrated along each path from the mean extinction over the altitudes it spans; the sky is as much of the horizon's light as a ray's optical depth gathers (black high over the clouds down to a thin bright limb at the deck's own horizon; bright all over in the haze). The camera exposes over the clouds for the deck under it and takes back four tenths of a dark belt's dark (the colours stay the band's), in a deck for the fog round it, in the clear band for the ceiling, up to a gain of 12 (the water deck goes dark and the lamp is what is left). The shading of a cloud is soft by design: the slopes it sees are held to 1.2 and the sun's light is wrapped round its sides, else a steep lump read as a hole.
* **Lightning has a place.** A flash is at a place in the decks (in the water deck's towers under the clear band, where the camera sees the floor; close by in a deck's fog; under the deck as a glow seen from over the clouds; over the deep), lighting its tower from within, the ceiling over it, the air round it and, one flash in three between the decks, a channel up through the clear air. The thunder's loudness falls with the flash's distance.
* **The pace stays.** X-01's seven or eight minutes free were kept: each stage now has enough to watch, and Shift (R-406) is the explorer's own way through what they have seen.
* **What moved.** `GEN_VERSION` 11; the regress unchanged (the globe's jet lookup is the same arithmetic, called through `giantJetTurn`; nothing it hashes is drawn differently); `galaxy/probe.*` (`GiantBands`, `giantBandsOf`, `giantJetTurn`, `GiantDecks`, `cloudNoise2/3`, `giantHazeTau`; `giantSunlightAt` now on it), `space/probe_view.*` rewritten (`ShadeRamp`, `ProbeView`, `renderProbeView`, `probeSpaceShows`, `probeViewStats`), `game/probe.cpp` (`probeEnsureClouds`, `probeDeckDrift`, the feed's handover over the entry's 62-92%, the plasma in RGB, `flashAt` with places), `SpaceRenderer::render` honours `excludeBody`; harness: `vesperis_test probe stages [night]`, `probe stability`, the views' layers and costs under every probe frame, the bench's `probe 2x`, two unit checks and the flow's `probe_between`. KI-360 narrowed; KI-361 holds what stays.

## X-03 (2026-10-07): each giant its own

* **Decks by condensation.** A giant's decks are where its adiabat crosses each cloud's condensation temperature (`CLOUD_SPECIES` in `galaxy/probe.*`: methane 80 K, hydrogen sulphide 125, ammonia 150, ammonium hydrosulphide 200, water 275, the chloride and sulphide salts 700), the base deeper with more of it (the pressure times the abundance's 0.6th power: 0.74-1.73 by a hash, more in an ice giant), the top the base over the cloud's span (1.7-2.4), within the probe's reach (32 bar or the heat's end), one to three from the top down, a clear band of a quarter of the pressure at least between two. Nothing condenses where the tropopause is warmer than the cloud (less 6 K) or within a fifth of it; a giant poor in a species lacks its deck (by a hash: ammonium hydrosulphide one giant in two, the salts one in four, hydrogen sulphide one in five, ammonia one in ten). So the giants near home fall into the kinds a planetary scientist would name: a cold giant's methane over its ammonia, Jupiter's three (Wailie VI: ammonia 0.25-0.61 bar, ammonium hydrosulphide 0.98-1.66, water 2.8-5.1), a warm giant's high water deck without ammonia and its salts far down (Thilawiane III: water 0.35-0.62, salts 8.6-16.4); of the first 600 near home 10% have one deck, 41% two, 49% three. The upper haze's top is the first deck's over 1.6-3.0 (a hash). X-01's fixed decks (0.6-1.6 and 4-9 bar) are gone; a giant with nothing condensing in reach (none of the first 300) gets the salts' haze just over its end, so a picture always has a deck.
* **Three kinds, the heat, a quieter ice giant.** An ice giant is a small blue gas giant (`isIceGiant` in `galaxy/system.*`: under 45,000 km, its globe's blue over its red: one gas giant in seven near home), derived from what the generator made, so its globe already looks the part: richer in what condenses, hydrogen sulphide where a gas giant has ammonia, its storms at 0.55 of a gas giant's, its lightning 0.06-0.18, its billows smoother, the relay's wind at 0.6, its jets the fastest (220-600 m/s), the light blued by its methane from high in the air down, its magnetic pole 30-60 degrees off its axis. A brown dwarf is warmed by its own heat (the temperature at one bar its equilibrium's 1.5 and 90-210 K more by its glow), its decks thin under its gravity (the scale height a third of a plain giant's) and lit from under them. A gas giant's internal heat comes from its star's youth (a protostar 1, a Wolf-Rayet star 0.95, a blue giant 0.9, a blue-white star 0.6, a yellow star 0.3 ... a dead star's remnant 0.03), its mass and a hash; a young one (a young star's, its heat 0.62 and over: one gas giant in six near home) is up to 1.3 times warmer at one bar (its decks higher), stormier, and glows dull red in its deep. The giant's own character is printed by `vesperis_test probe giants` (`giantCharacterText`).
* **The storms are the globe's (generation 12).** Each giant's share of the storms' cells (`BodyGen::stormShare`: a gas giant 0.04-0.28, the plain giant's 0.15; an ice giant 0.02-0.06; a brown dwarf 0.07-0.19) is drawn on the globe and sets the descent's towers (times 0.5-1.6 with the heat: a calm giant half the plain giant's); an ice giant's bands are faint on its globe (0.45-0.65 of the contrast). A great storm on one giant in four (a brown dwarf one in six) is the planet function's: an oval of 10,000-24,000 km at 12-40 degrees of latitude, half to three quarters as tall as long, bright or dark (an ice giant's and a brown dwarf's dark, as a Neptune's spot), its bands swirled round its centre, a dark collar, no small storm inside, in the albedo and not the storm term (it does not fade with the N5-03 storms). It turns whole with its centre's latitude's jet (`giantJetAt`, its surroundings easing into it over 1.3-2.2 of its size; the globe's lookup and the descent's bands both call it), so it keeps its shape a day later while the bands round it shear. In the descent (`GiantBands::greatStormAt`, the oval in the tangent plane) it is a dome six scale heights over the bands with streets wound round its centre (on its circle's cosine and sine, so no seam) and calmer billows, and a wall ten more over its rim: a steep inner face over 25 scale heights' run, a long outer slope over 300, a crowd of tall towers on it; aimed within 2,500 km of its rim, the camera faces the wall. Every gas giant's and brown dwarf's globe changed with the storms' share: generation 12 (systems, stars and solid surfaces stay; the regress saw no giant change).
* **The deep's glow from below.** The giant's light under its lowest deck rises through each deck under a place at a third of its optical depth (`giantGlowAt`: the light from below is diffuse already): an old giant's faint (0.004 of the sunlight over the clouds), a young one's 0.1-0.3 (its deep dull red, the undersides lit), a brown dwarf's calibrated so its cloud tops shine as its globe does from afar (half its `selfGlow`), its deep many times brighter: a red glow through the tops on the night side, an underlit orange cavern between its decks. The view carries it as a second channel of every surface and of the fog in the glow's colour; the camera's exposure counts it with the sunlight (down to a gain of a quarter).
* **Lightning lives in the convective decks** (water, the salts): over the clouds a glow under them, in a convective deck close by, between the decks over a convective floor (a channel up to the deck over it one flash in three), under a quiet floor a glow, at the giant's rate (a gas giant's 0.2-2.0 times the plain giant's, rising with its heat; an ice giant's 0.06-0.18; 0.15 of it without a convective deck within reach).
* **The aurora** (R-402's field and the star's activity as on the worlds, a volcanic moon's plasma feeding it steadily, a brown dwarf's own): an oval round each magnetic pole (a gas giant's pole 2-12 degrees off its axis, an ice giant's 30-60, a brown dwarf's 5-15; the oval 10-23 degrees round it). Over the clouds within sight of it, B-403's curtains at the giant's scale (three sheets across the way to the nearer pole at the oval's distance, folded, rayed, a lower border near 2e-7 bar, 300-600 km up), in hydrogen's colours (magenta under violet; an ice giant's blue under violet; a brown dwarf's crimson under orange), seen by day too (the sky over the clouds is black) and washed out in the haze; aimed within 2,800 km of the oval, the camera faces the curtains. Not drawn on the globe from orbit (KI-362).
* **Rare sights.** A clear-air hole (a hot spot: the air sinking, the clouds dried out down to the next deck) on one giant in four (a brown dwarf's three in five, an ice giant's one in twelve): hashed in cells of 600 scale heights, and the descent's own about half the time on such a giant (its radius 22-52 scale heights, where the probe crosses the deck's top, through every deck over the next that opens), the camera looking down into it and further down the shaft. A hole opens only onto a deck close under it (`GiantDecks::holeable`: a gap under one and a half times the deck's thickness and ten scale heights), since the first build opened a water deck onto salts 290 km down and drew a chasm of wall. Diamond hail (every giant of a carbon star, one in seventeen of the others) glints in the lamp out of the lowest deck. And on one giant in forty (none of the brown dwarfs), once in the deep at a depth of the probe's own (its clock follows the chute), something large and pale passes through the lamp's beam over a few seconds; nothing names it, logs it or sounds it (the source's "What to resist": no life in the clouds, at most one ambiguous sight, never explained).
* **The stage is the layer.** The stages keep their kinds (a deck, the clear band, the deep) and the screen names a deck by its cloud (`THE HYDROSULPHIDE DECK`; `giantStageName`); with three decks the clear band comes twice, so order is the layer's (`giantLayerAt`). The probe's own HUD names the layer its picture shows (a great storm's dome lifts a deck over its pressure: the deck's name in its fog; `A CLEAR-AIR HOLE` in one); the cabin's line and the photograph's caption name the pressure's.
* **What moved.** `GEN_VERSION` 12; the regress unchanged (22 hashes: no frame it hashes shows a giant's changed face); `galaxy/probe.*` (`GiantKind`, `GIANT_KIND_NAMES`, `CloudSpecies`, `CLOUD_SPECIES`, `GiantCharacter`, `giantCharacterOf`, `giantAtmosphereOf(system, body)` with one to three decks (`decks`, `species`, `top`, `base` for X-01's four numbers) and its character, `giantStageName`, `giantDeckAt`, `giantDeckAbove`, `giantLayerAt`, `giantStormDeckFrom`, `giantCharacterText`, `giantGlowAt`, `giantSunlightAt`'s holes, `giantJetAt`, the great storm in `GiantBands`, `GiantDecks` over the decks with `storms`, `calm`, `holes`, `path`, `holeAt`, `holeable`), `galaxy/planetmap.*` (`BodyGen::stormShare`, `bandContrast`, `gsOn`..`gsTone`, `giantGlobe`, the gas giant branch), `isIceGiant` (`galaxy/system.*`), `space/probe_view.*` (the decks, holes, the glow, the aurora, hail, the sight; `ProbeViewStats::inHole`, `aurora`), `game/probe.cpp` (the character's looks, the exposure with the glow, the lightning, the hole and the sight, the camera's turn to a storm's wall or the aurora, `probeStormWall`, `probeAuroraWay`, `probeStageLabel`); harness `probe giants`, the stages mode's frames by layer (`deck_<cloud>`, `between_<n>`), two unit checks; KI-360 narrowed, KI-362.

## X-04 (2026-10-07): what comes back

* **A recording is the descent drawn again.** The fall was made a function of the probe's numbers in X-01 (the clock in closed form, the save a handful of numbers) and the stages fields round the aim in X-02, so the ship keeps the numbers and not the pictures: the launch (the giant's key, the aim, the seed, the ship's place, the headings, the chute's clocks, the hull's limit), the explorer's turns of the camera and the game's clock along the fall (keys where they change: a turn of half a degree, half a second of drift off the pace, the frame before a turn after a still stretch kept too), the end. Played, they go through the same `renderProbeFeed` as the live screen, so a frame of the recording is the live frame of its clock (the unit compares them) and the viewer can still look round. A video was weighed: twelve minutes at the logical resolution is hundreds of megabytes of PNGs (the link's static does not compress), and it could not travel through the guide's export, a text file. The sound is the relay's synth driven by the recording's clock, as the live screen's.
* **Played anywhere on the ship, from the gallery.** A recording of another star's giant draws with that star's system and its sky's stars (a `StarSystem` and a `StarNeighborhood` of the replay's own, swapped into the game's for the call with the record's probe: the probe's functions read the game's members, and the space renderer's caches are keyed by the bodies' seeds). The recording's screen is not a simulating state: the game's clock, and a probe falling meanwhile, wait while it plays. On the ground the gallery says the recordings play on the ship.
* **Scrubbed by depth.** The bar runs by the pressure's logarithm from the clouds' top to the end (the fall and the entry by the clock over its first tenth), so the decks are where their pressures are; the arrows move along it and Up/Down jump to the layers' tops.
* **The profile is kept, not recomputed.** The record stores what the probe measured (a sample every quarter of an e-fold of the pressure, the layers it found with their bounds found by halves): a later generation of the giant would draw its recording differently, but its profile and its final image stay what came back. The layers are named by what the probe found at its place (the decks' fields, the view's own `layersAt`: a deck its storm lifted, a clear-air hole) rather than by the giant's pressures; the light is the sun's at the depth by its height over the aim's horizon and the deep's glow, so a probe at night or under a hole reads differently; the wind is the screen's reading. Two probes into one giant differ by their latitude's winds, the decks where they met them and their light: the data sheet draws the giant's other probes dim under the one shown, on the same scales.
* **The log keeps the crossings, once each, with the depth**: the storm it fell into (named if named), the great storm's wall seen from outside, the aurora's curtains, a clear-air hole, diamond hail; the rare sight is never logged (X-03's rule). Looked for every frame the probe falls, wherever the explorer is; the hole by the layers found over the frame's stretch of the clock (so a jump of the clock does not step over it).
* **A storm is named by its cell.** `N` on the probe's screen or its recording names the storm under the aim: the great storm by the giant (`<body>/TG`), a storm of the globe's storm term by its worley cell (`<body>/T<id>`, `giantStormCell`, which the globe's branch now calls: the same arithmetic, no generation change), so a second probe into the same storm finds its name. The name lives with the explorer's names (exported with them).
* **Lent like the shards.** The export is the guide; the import copies the friend's own recordings (not those lent to them) as lent: cyan in the gallery and on the profile pages, never counted in the statistics. A lent recording has no final image on the disk: the gallery draws its last frame when it is shown.
* **What moved.** No generation bump (`GEN_VERSION` 12); the regress unchanged (22 hashes); `game/guide.*` (`ProbeSample`, `ProbeRecord`, `Guide::probes`, `probesSent`, `deepestProbeKm`, `probeIndexBySeed`, the `probe`/`probecam`/`probetime`/`probeprofile`/`probelayers` lines, `stat probes`, `importInbox`'s recordings), `game/probe.cpp` (`probeTrack`, `probeKeep`, `probeProfileOf`, `probeLightAt`, `probeLayerAt`, `probeStormName`, `nameOfKey`, `probeHiss`, `probeSound`, the replay: `ReplayState`, `asRecording`, `recordingContext`, `openRecording`, `updateRecording`, `renderRecording`, `recordingAudio`, `recordingPoster`, `recordTimeAt`, `recordCamAt`, `recordDepthU`, `recordClockAtU`, `recordTitle`, `recordLine`; `renderProbeHUD` serves both; `dataProbes`, `renderProbeProfile`), `GameState::RECORDING`, the gallery's `GalleryItem`s, the data sheet's `dataPage`, text entry kind 9, `Game::epocOf`, the save's probe extras and `probetrack` line; `galaxy/probe.*` (`GiantDecks::layersAt` from the view's `layersOf`, `giantFoundLayer`, `giantLayerName`, `giantStormAt`), `galaxy/planetmap.*` (`giantStormCell`); harness: four unit checks, the flow's `probe_gallery`, `probe_recording*`, `probe_stats`, `probe_profile`. KI-360 narrowed, KI-363.

## R-406 (2026-10-07): Shift runs the probe's clock eight times as fast

* The user after playing X-01: "an acceleration button like shift so you can speed up probe". Held on the probe's screen, Shift runs the relay's clock eight times as fast (`FAST` in amber after the stage's name): the fall, the stages and the chute go by eight times as fast, the flashes come eight times as often; let go, or away from the screen, it runs at one again. A hold, not a toggle: the descent's moments are worth slowing for, and a toggle left on would rush them. Eight: a free descent of seven or eight minutes passes in under a minute. The warp still turns only the sky. The screen's last line names it (`SHIFT FAST`, in place of `X CENTRES`; `X` still centres).

## B-409 (2026-10-07): a step where a smoothstep's edges meet

* **The cause was a 0/0.** `smoothstep` divided by the span between its edges, and on a giant with one deck the probe's tint asked for the span from the first deck's base to the last's, which is none: at the base itself the tint was a NaN, and the air's and the fog's colours read toward it came out black from the deck's top to the end. One giant in eight has one deck.
* **Fixed in the helper, not at the call.** Equal edges are a step (0 under the edge, 1 from it), so no caller can get a NaN out of it; guarding the one call would have left the trap for the next. The step is what the function already gave on either side of the edge (an infinite slope, clamped), so only the edge itself changed: the regress (22 hashes) is unchanged and nothing generated moves (`GEN_VERSION` 12). A one-deck giant's base takes the deep's tint, as every giant's last base does.
* **A brown dwarf's fog at a deck's base: six tenths of the glow, darkened by the cloud under it.** The fog took the glow under its deck in full, a flat orange clipped over nine tenths of the frame where a brown dwarf's first deck meets the cavern under it. The exposure stays as X-03 tuned it for the cavern and the deep; the fog gives back six tenths of the glow, about what the deck's underside shows from under it (0.55 and its pouches), darkened where the cloud under a point is thick by two taps down against the deck's mean structure (the sun's two taps up, turned over). Never brightened: under the base the taps find clear air and would have lit the fog there twice over. Only where the glow counts (over 0.02), so a gas giant's deck pays nothing.
* **Tests read pictures.** The unit renders five giants (one deck of each kind, two, three) at each deck's middle and base and in the deep, without the HUD and the static, and wants light in every frame (a mean of 8) and no flat clip (under 40% of the pixels at 250); `probe stability` has a fourth place, in a brown dwarf's glow.

## B-410 (2026-10-07): a body's approach leaves the belt

* **One ship, one parking.** A ship parked in a belt kept `parkedBelt` through a body's approach, and the parking's branch for a belt comes first, so at the approach's end it was put back beside its rock while the analyzer named the planet. `startApproach` clears the belt's target and parking, as `startApproachBelt` already cleared the body's; the flight computer's next body clears the belt target, as every other way of choosing a body does. Fixed where the state is set, not by ordering the parking's branches: a parking of both kinds is no state the ship can be in.
* **A save in that state loads at the body.** Its approach had reached the body (the body's parking is set only at an approach's end), so that is where the explorer was going; the rock is forgotten.

## R-407 (2026-10-07): a jump of any length along the aim

* **The jump is the aim's, not a new mode.** The user: "now I jump 400 light years in that direction". The aim (R) already says where the ship looks and turns with the mouse; the digits typed in it are the jump's light years and Enter flies, so the one screen that picks a star also picks a direction (Enter without digits still locks the star under the crosshair). The flight computer's last row opens the aim along the nose; the console's `JUMP` flies along the nose too. One code path: the jump makes a star the remote target and calls the Vimana flight every other target uses.
* **A jump ends at a star.** The Vimana comes out where a star holds it: the star nearest the end of the line (a cube of 21 sectors round it, 1.4 ms at home). A ship with no star under it is no state the game keeps (the save, the radar, the system list all read one), so where the end lies in the void the jump comes back along the line, ten light years at a time by the density (70 ns a question; a cube is searched only where it holds half a star or more), to the first place that holds one, and says how short it fell. 100,000 ly at most: the galaxy is about that across.
* **The flight's length: logarithmic past 100 ly.** `7 + 2 sqrt(ly)` made a crossing of the galaxy ten minutes; it stands to 100 ly (the radar's and the aim's flights did not change) and grows 6 s a doubling beyond (400 ly 39 s, 10,000 67 s, 100,000 87 s). The streaks need nothing more: the band is not drawn in a flight and the neighbourhood's rebuild costs 1.4 ms a sector.
* **No UNKNOWN in the aim.** Since R-401 the six markers read `UNKNOWN 4.2` six times: the user saw "green marker stars as unknown" and could not say what they were. A marker reads its distance alone where the star has no name, the name where it has one, and `VISITED` in amber where the ship has been (the radar's loops led back to systems already named); the crosshair's line, the remote's and the radar's lock say `VISITED` too, and the star map explains its boxes and names an unnamed visited star by a world named there.
* **What moved.** No generation change (`GEN_VERSION` 12); the regress unchanged. `galaxy/starfield.*` (`nearestStarTo`, `jumpTarget`, `galacticHeading`, `vimanaSeconds`, `JUMP_MAX_LY`), `game/autopilot.cpp` (`aimDirection`, `startJump`, the aim's digits), `game/hud.cpp` (`aimStarLabel`, the jump's panel, the key line), `game/cabin.cpp` (the row, `JUMP`), `game/radar.cpp`, `game/guide_screens.cpp` (`worldNamesAt`, the star map's legend).

## R-408 (2026-10-07): the explorer's marks and the scanner

* **The landmarks are no longer markers** (it supersedes O6-06's "landmarks are found, not placed" as markers, and B-401's rule for them; the sights are still found in the terrain). The user: one could name only "the nearest landmark", once, and the next ruin found was nothing to the game; "remove them as markers ... place arbitrary marker on something YOU see as a landmark and name it". The maps show what the explorer marked and nothing else: a mark is a place of a world, a name (or none) and what it marks, placed with `L` at the crosshair's spot or with the sector map's cursor. Nothing is logged or drawn by itself.
* **The sights are the scanner's echoes, each kind on its own.** O6-06 kept the first kind of an order per 14 km cell, so a crater hid its ruins and a hamlet beside a town was never a sight. The cells now give a candidate of every kind (the old order kept apart for the old names), and the ruins of the 2 km grid are listed one by one: "find the next nearest ruins" needs every one.
* **A lake is water.** O6-06's lake was the centroid of every lake cell of the drainage in its cell: a playa's fill, a frozen basin's, or the dry ground between two lakes as often as not (round a test place 5 of 22 stood on water). The scanner's lake is the biggest lake of the cell that holds water at 64 m, at its sample under the water nearest its centroid; O6-06's is kept beside it for the old names.
* **The scanner gives a way, not a place.** Its modes are the kinds the world can hold; it hears every sight of the mode within 40 km (on a thread: 50 ms warm, 2 s at most cold) and names the nearest at the top left with its bearing on the compass. An echo is heard off its place by up to 8% of its distance (3 km at most), in a direction of its own: from afar a way to walk, near it the place. 40 km: a drone's quarter hour, the nearest ruin of a people's world a kilometre or two, a lake or a peak most often within reach where there are any.
* **The marked sights are left out, optionally.** A sight is marked when one of the explorer's own marks stands within its extent (150 m at least) and 250 m more, so a mark anywhere on a town or a lake's shore leaves it out and the next is the nearest; `Shift+R` takes them in again (the user: "make it optional"). A friend's lent marks leave nothing out: the explorer has not been there.
* **The sector map is where marks are made.** It gains a cursor (the arrows moved the zoom: `+`/`-` and the wheel now); Enter marks or renames, Delete twice removes (a named place is not lost to one key), M puts the waypoint at the cursor (the help always said so; it was put at the feet). The landing map shows the marks too, so a landing can be made beside one.
* **The old names became marks.** The explorer's names of O6-06 landmarks (`<world>/L<id>`) are moved to marks where the landmarks stand when the ship is in their system: one pass over the world's grid finds the cells by their ids, and the old order picks the landmark the name was given to, on a thread of its own (the player's dozen names of one system asked a dozen cells and their drainage tiles: 7.5 s cold, which froze the arrival on the main thread; the marks come in when it is done, with a status line). A name whose landmark is not found again stays where it was (and is not asked again that session). The `landmark` lines (the seen set) are not read any more.
* **What moved.** No generation change (`GEN_VERSION` 12): the terrain, the ruins and the sights are as they were. `galaxy/landmarks.*` (`CellSights`, `sightsOfCell`, `legacyLandmarkOf`, `cellsOfLegacyIds`, `sightsNear`, `ruinsNear`, `sightKindsOf`; `landmarksNear`, `landmarkOfCell` and the generated names gone), `surface/surface_view.*` (`ScanEcho`, `startScan`, `collectScan`, `joinScan`; the landmark thread gone), `game/guide.*` (`SurfaceMark`, `mark` lines, the lending), `game/marks.cpp` (new), `game/hud.cpp` (the sector map, the rangefinder's line), `game/landing_map.cpp`, `game/guide_screens.cpp` (the menu, text kind 6, the inbox's status), `game/game_states.cpp` (`R`, `L`, the help).

## G-05 (2026-10-08): the galaxy chart

R-409 (the user: "display whole galaxy map and show a point where you are + density of the stars and the ability to select on that map where to jump next"); the chart is G-05, choosing the jump on it G-06.

* **A page of the star map, not a screen of its own.** `Tab` turns the map between the neighbourhood and the galaxy (Tab had no use there) and the map opens on the page it was left on, so `M`, the right wall's screen and the guide menu all reach it. In a Vimana flight `M` opens the map now, always on the galaxy (the line flown, the ship moving along it), and `Tab` is refused there: the neighbourhood is read at a star, and its `Enter` would retarget a flight in progress. The neighbourhood's title names the page (`THE NEIGHBOURHOOD  1419 STARS IN 10 LY  ZOOM 1.0X`) and its keys say `TAB GALAXY` (`MOUSE/WHEEL TURN/ZOOM` and `0 ALL` gave way: the mouse turns it without being told, `C/X/0 CLASS` keeps the key). The help's page 0 merges the line (`G GUIDE  M MAP, TAB GALAXY  J LOG  C CAPSULE/PROBE`: page 0 holds 21 lines).
* **The column in closed form, not integrated.** The plan measured the harness's map at 61 samples a pixel and planned a worker, coarse images first. The disc's height profile is an exponential and the bulge's a gaussian, so the face-on column has a closed form (`galaxyFaceOnColumn`, `04-galaxy-and-systems.md`; the cap's slab bisected in the core) that agrees with a 2 ly integration to 0.01% at 65-80 ns a column. The face (320 x 128 at the logical resolution, 2 x 2 samples to level 3) and the edge-on strip (40 samples along the view's depth a pixel) build in 2-7 ms on the pool, synchronously when the view changes; no worker, no coarse pass, no stale image. The pixel grid is anchored in the galaxy (the view's centre snapped to a pixel of its level), so a pan moves the picture by whole pixels without a shimmer and a cached image is reused while the view stays on its pixel.
* **Starlight, toned to the core.** Brightness is the log of the column (the harness's tone) normalised at the core's 4,000 stars a square light year, raised to 0.7, at 0.85 of white: the light climbs all the way in (the first tone, normalised at 700, saturated everything inside 8,000 ly and the core stopped standing out). The arms' excess is weighted four times at the crest (`1 + 3 x arm share`): an arm holds about twice its gap's stars and its young stars outshine it, and with counts alone home's own arm did not read at all beside the inner arms (home is on a crest, arm factor 0.99). The hue is the three parts' shares of the column: the bulge's old stars warm, the disc between the arms a pale gold, the arms blue-white. The scale bar and, in G-06, the readout carry the numbers; the picture shows where the stars are dense.
* **The knots from their cells.** The column steps over anything a few tens of light years thick, so the globulars (all 145, found once a session) are faint warm points at every level and in the strip, and from level 6 (13 ly a pixel) the open clusters (100 ly cells through six scale heights) are blue-white knots and the star-forming complexes faint red glows of their 12 ly reach. The first try drew them as bright as stars and the chart read as a star field: they are dimmed under the markers' brightness. Nothing on the chart has a name.
* **Levels.** Nine by powers of two, `+`/`-` and the wheel: level 0 is the whole disc (110,000 ly tall, 860 ly a pixel, the face 2.5 times as wide), always centred on the galactic centre and not panned; from level 1 the view follows the ship until the arrows or the mouse pan it (a nudge of under two pixels of the mouse does not let it go), `C` or `Space` follow again; level 8 is 430 ly tall (3.4 ly a pixel), the neighbourhood page below that. The chart opens on the whole disc: the first question is where the ship is in the galaxy.
* **The strip.** The edge-on view of the same window: the field's mean density along the face's depth, its height 600 to 3,000 ly over and under the plane by the level and larger when the ship is higher (R-407's jump north leaves it 2,300 ly over): the ship is always in it.
* **What the explorer has.** The ship (a white crosshair open in the middle with a dark rim, `YOU`, an arrow at the edge when panned away), home (the star map's amber diamond, rimmed), the stars visited (dots) and the way between them in the order of the arrivals (`Guide::route`, `route` lines, one per arrival, ten thousand kept: an older guide's visited stars are dots without lines until it travels again), the remote target with a dotted line from the ship, the line flown in a flight, the signals the radar locked as rings in the radar's red (KI-349's "on no map or screen of the ship"), the explorer's named stars at levels 6-8. The galaxy's marks and the cursor are G-06's.
* **No scene under it.** The chart is opaque, so the render dispatch draws no space frame under the galaxy page (the neighbourhood keeps the scene under its veil); a frame of the chart without a rebuild costs 0.75 ms at 4x.
* **What moved.** No generation change (`GEN_VERSION` 12); the regress unchanged. `galaxy/starfield.*` (`galaxyFieldDensity`, `FaceOnColumn`, `galaxyFaceOnColumn`), `game/galaxy_chart.cpp` (new: `Game::chartView`, `chartProject`, `updateGalaxyChart`, `renderGalaxyChart`), `game/game.h` (`mapPage`, `GalaxyChart gchart`, `CHART_*`, `chartScale`, the hooks `testMapPage`, `testChartLevel`, `testChartFollow`, `testChartBuildMs`, `testChartYouX/Y`, `testChartLegs`, `testArriveAt`, `testPlaceShip`), `game/guide.*` (`route`, `pushRoute`), `game/guide_screens.cpp` (the pages, `noteVisit`'s route), `game/autopilot.cpp` (`M` in a flight), `game/game.cpp` (no scene under the chart), `game/game_states.cpp` (the help's line), the harness's `chart` mode, three unit checks, the flow's `starmap_galaxy`, `starmap_galaxy_x16`, `starmap_flight`.

## G-06 (2026-10-08): the next jump chosen on the chart

R-409's second half ("the ability to select on that map where to jump next"): a cursor on the galaxy chart, what lies under it, the jump plotted and flown, a look at the stars round its end first, the explorer's things as stops, the explorer's own marks of the galaxy.

* **A free cursor that carries the view.** The R-408 sector map's cursor was the model: white corners round a point, moved by the arrows (80 px a second, Shift three times) and the mouse; the chart's pan (G-05) became the cursor pushing the view within 16 px of the face's sides and 12 of its top and bottom, so one control does both and level 0 (the whole disc, not panned) moves the cursor alone. The zoom keeps the cursor's pixel (the wheel zooms where the explorer looks). The cursor sits on the ship and moves with it until it is moved (a mouse nudge under 2 counts leaves it, as it left the view); `C` or `Space` bring it and the view back, and an arrival puts both on the ship's new place. Its reach is 54,000 ly from the galactic centre (the level-0 face).
* **In the plane unless told.** Let go of the ship, the cursor's point lies at `y` 0, where the stars are (after a jump north the ship is 2,300 ly over the plane and a jump back to the disc is the likely next); `PgUp`/`PgDn` move its height on the strip (two seconds from the plane to the strip's edge, to 30,000 ly), and the strip grows to hold it as it holds the ship.
* **The readout in the rows the chart had.** The top row's region and the second row describe the cursor's place while it is off the ship (white): the stars there as one star in so many sectors (`galaxyDensity` at the point, two figures; `NO STARS` past one in a million) and its height; on the ship the rows stay G-05's (green). The row under the strip gives the jump's line (`5,000 LY COREWARD - 61 S BY VIMANA`), the legend's place: the legend shows while the cursor is on the ship. The zoom went into the scale bar's box (`10,000 LY  X4`) to free the second row. What the explorer has under the cursor is a label beside it, not a row. The last row shows the status while one is up (the map pages drew none: G-05's in-flight refusal of `Tab` was silent) and otherwise the keys of the moment (four sets: on the ship, away, on a mark, plotted).
* **Plot, then fly: R-407's line in two steps.** `Enter` plots: `Game::plotJump` (R-407's `startJump` split) runs `jumpTarget` along the line from the ship to the cursor's point, so the chart's jump is the aim's jump (the void sends it back along the line to the first stars, the readout saying `16,795 LY SHORT, THE VOID`), and makes the star the remote target; `Enter` again flies it (`flyJump`), `Esc` keeps the target for `V`. The `JUMP` log line moved to the flight's start (`toggleVimana`) and is written for any plotted target flown (`Game::jumpPlot`, forgotten by any other kind of target), with `- PLOTTED ON THE CHART` when the chart plotted it; the aim's and the console's jumps log as before. A move of the cursor lets the plot go (its status too) and the target stays until another is set. The plot is not saved: after a load `V` flies the target as a plain flight.
* **The look before the leap is the neighbourhood page moved.** On a plotted jump `Tab` shows the neighbourhood page round the plotted star (`AT THE PLOTTED END`, a `StarNeighborhood` of its own: the page reads `Game::mapStars`/`mapCentre`), not a new screen. The page's pick is the star nearest the screen's middle, which in the look would always be the plotted star itself, so the plotted star is left out of the pick as the ship's own star is on the ship's page; `Enter` takes the picked star as the plotted end (the line kept for the log) and goes back to the chart still plotted, so `Enter` flies.
* **Stops by kind.** `N` (Shift+`N` back) steps through the locked signals first (a signal locked 3,000 ly off is `N`, `Enter`, `Enter` from a jump), then home, the named stars, the marks of the galaxy, the systems visited, nearest the ship first within a kind, a star once (its first kind), the star the ship is at left out. A star's stop is the star's own place, so the plot finds that star.
* **Marks of the galaxy as R-408's marks.** `L` at the cursor names a place (`NAME THE PLACE` over the chart with its region and distance from the core; an empty name places an unnamed mark; on a mark `RENAME THE PLACE`), `Delete` twice removes it, the log has a `MARK` line. Kept as `gmark` lines in the guide (`Guide::galaxyMarks`) and lent by an export like the marks of the worlds (a friend's in cyan, theirs lent to them not passed on, one within a light year of a mark already there the same place; the inbox counts them with the world marks). Drawn at every level as a pennant with the name (the named stars only from level 6): they mark galaxy-scale places.
* **Labels by priority.** The ship's `YOU` is placed first, then what the cursor rests on, then home's label, then the marks' and the named stars' names, each trying the right of its place at three heights and then the left; a name with no room is left out (the first frames drew a mark's name twice over `YOU` when the cursor stood on it).
* **What moved.** No generation change (`GEN_VERSION` 12). `game/galaxy_chart.cpp` (the cursor, `chartCentre`, `chartCursor`, `chartPlotted`, `chartLetGo`, `chartUnplot`, `chartEnter`, `chartStops`, `chartNextStop`, `galaxyMarkNear`, `chartThere`, `galaxyPlace`, `beginGalaxyMark`, `finishGalaxyMark`, `drawMapStatus`; the readout, the strip's target and cursor), `game/autopilot.cpp` (`plotJump`, `flyJump`; the arrival resets the cursor), `game/cabin.cpp` (the `JUMP` line at the flight's start), `game/guide.*` (`GalaxyMark`, `galaxyMarks`, the `gmark` lines, their lending), `game/guide_screens.cpp` (the look, the status on the map pages, the mark's entry, `setRemoteStar` forgets a plot), `game/game.cpp` (the entry over the chart), `game/game.h` (`JumpPlot`, the cursor's state, `CHART_REACH_LY`, `CHART_HEIGHT_LY`, the hooks `testChartCursor`, `testChartCursorAt`, `testChartOnShip`, `testChartPlotted`, `testChartLook`, `testLookStars`, `testChartCurX/Y`, `testJumpShort`, `testMapPick`), the harness's `chart` mode, five unit checks, the flow's `starmap_cursor`, `starmap_plotted`, `starmap_look`, `starmap_jumped`, `starmap_arrived` (not `chart_`: C-10's star charts have those).

## G-07 (2026-10-08): the crossing

R-409's third part and KI-364's "the band is not drawn in a flight": a jump across the galaxy showed the neighbourhood's streaks on black, so nothing of the disc turned round the ship.

* **Keyframes built ahead, blended between.** The plan said a band rebuilt every half second on a worker and crossfaded. The flight's line is closed-form (`vimanaProgress`, now the one curve `updateShipMotion` moves the ship by), so the band is built ahead: keyframes at flight times (`SpaceRenderer::FlightBand::tau`), each a whole map at the ship's place then, and a frame draws the two round its clock blended, a piecewise-linear band in time rather than a chase of where the ship was. Not a worker thread: `WorkerPool` is not reentrant and the frame's passes use it, and a thread finishing whenever it finishes would make a flight's frames depend on the machine. The keyframes are built a share a frame on the pool inside the frame (`buildGalaxyBandCells`), sized so the next one is whole within half its interval at the clock's pace, so a flight's frame is a function of its clock and the frames before it (the harness's frames repeat).
* **Closer where the core turns fast.** Half a second apart, as planned, wherever the galactic centre turns less than two degrees past the ship between two keyframes (its speed over its distance from the centre, 1,000 ly at least); closer than that where it would turn more, to a twentieth of a second (a flight through the core: 520 ly a second at the centre on a 26,000 ly jump). A crossfade of two keyframes 15 degrees apart would show the bulge fading out in one place and in at another.
* **Neither end builds at once.** The first keyframe is the parked map when it was built within ten sectors of the start (what the explorer was looking at), and the last is handed to the parked band as soon as it is whole, so the arrival does not rebuild the band (it did, synchronously, 9 ms). A keyframe the clock needs before its share is done is built whole in that frame: a flight begun with no parked band, a skip of the clock (the harness's), and nothing else (a 10,000 ly flight at 60 frames a second builds none at once). The galaxy page, which draws no scene, keeps the keyframes building (`Game::keepFlightBand`), or closing it in a flight would build one.
* **The star-forming regions' patches fade with the speed.** They lie within 42 ly (N5-01, G-03), so at a crossing's 100-200 ly a second the ship passes a complex in a fifth of a second: a 30-degree disc of red or blue for one frame and gone (`stability flight` counted four to seven such frames on the 10,000 ly flight, the worst moving the whole frame's mean by 15 levels). Building them into the keyframes would spread a disc of the wrong place over half a second; they are faded by the flight's speed instead (`1 / (1 + (v / 15 ly/s)^2)`), seen as the ship sets off and arrives and on a short hop, gone on the way. The band's own near part (the clusters within 55 ly) is in the keyframes and blends.
* **The tone stays G-03's.** The band's exposure is fixed so a place always looks the same; toward the core the bulge widens until it fills the view at the tone's knee (the luminance 48 over the whole view at 3,000 ly from the centre). An exposure that adapted in flight would show the bulge's structure near the core but make the arrival's sky differ from the same place parked; not done.
* **A flight straight out of the disc turns its side to the core.** The ship's attitude is a yaw and a pitch, no roll; a flight within 0.6 degrees of the pole took its yaw from the target's few light years aside, which on the harness's jump north put the galactic centre under the cabin's floor, the one side without a window. Such a flight turns its right side to the centre (the nose stays within 0.6 degrees of the line), so the side windows watch the disc fall away (as a stripe across them: the ship stands on its tail). A steep flight that is not that steep keeps its heading.
* **What moved.** No generation change (`GEN_VERSION` 12). `galaxy/starfield.*` (`buildGalaxyBandCells`, `buildGalaxyBand` on it, `vimanaProgress`), `space/space_view.*` (`SpaceContext::flightFrom/flightTo/flightT/flightDur/nebulae`, `SpaceRenderer::FlightBand`, `fband`, `flightBand`, `flightBandPos`, `flightKeyStep`, `parkedBandBuilds`, `parkedBand`, `parkedBandPos`; the band drawn in a flight, the patches faded), `game/hud.cpp` (the flight's line in the context, `keepFlightBand`), `game/game.cpp` (the galaxy page keeps the band), `game/autopilot.cpp` (`vimanaProgress`), `game/cabin.cpp` (the roll of a flight straight out of the disc), `game/game.h` (the hooks `testFlightT`, `testFlying`, `testFlightSkip`, `testStartJump`, `testCabinTurn`, `testCabinLook`, `testFlightBand`, `testParkedBandBuilds`), the harness's `crossing` and `stability flight` modes, the bench's `vimana 2x` (budget 12 ms), one unit check.

## H-01 (2026-10-08): the records' grammar (research)

R-411's request that the text overhaul be researched first. `docs/ideas/RESEARCH-records.md` is the survey, the choice and the data model; `docs/ideas/RESEARCH-records-samples.md` holds the prototype's fifty records beside C-02's texts of the same two worlds. The prototype was built in the scratchpad against `build/galaxy/*.o` and `build/core/*.o` and is out of the tree.

* **History first, told by a grammar.** A people's records are told from a simulated history (H-02): places, rulers, families year by year, public and private events with their causes, the collapse in stages. A record's plan (a genre's schema, its events picked by salience) decides what it tells. The depth is the history: C-02's hundred templates have none, and keep no person, crop or event from one text to the next.
* **A small attribute grammar over a realiser.** Weighted alternatives with guards (on the record's and the world's facts, on fields of the bound entities and events, on attributes) and `once`. Attributes pass down (the tense, the bindings, the facts) and up (a part's choice, `{=.key}`, so a tail fits its main clause). Dryness works inside a record (an alternative is not reused while others are left) and, weakly, across a world's catalogue. The record's plan is a set of directives (`@news`, `@years`, `@entries`, `@death`, `@deed`). The realiser owns the English: references (the first mention in full, kinship from the writer or the reader's close kin, titles; pronouns only where clear), verbs by tense and subject, numbers with their nouns, plurals, articles, capitals, punctuation and the spaces at joins. A plain weighted grammar (C-02's) was measured, not only argued against.
* **Composition for the frequent parts.** Over 100 chronicles, entries built of a time, a main clause and a consequence cut the share of repeated sentence frames from 77% to 30% with 1.7 times the authored lines, while weighting the draw (catalogue dryness from 0 to 6) moved the repeats by three points at most. Variety is pools of alternatives and their composition, not the draw; whole hand-written sentences stay for the strong moments.
* **The authoring format.** Plain `.vg` files: a rule name at the start of a line, its alternatives indented, options in parentheses first; the marks `{rule}`, `[a|b]`, `{$entity:form}`, `{lemma:$subject}`, `{#number|noun}`, `{=.attribute}`, `{@directive}` and `\n`; the joins are spaces, local attributes start with a dot. Each file is wrapped once as a C++ raw string and `#include`d, so neither build system changes and both track the dependency (files under 16 KB for an MSVC build); the harness reads the same files from disk, so an edit reprints without a rebuild. C++ tables (C-02's) and JSON (Tracery's) were set aside.
* **One catalogue a people.** A record's text depends on what the world's other records told (an event once in a record, at half weight in the next, from other sides elsewhere), so a people's records are built together: deterministic, about 10 ms with the history, on a thread at the approach, cached; a shard's text becomes a lookup.
* **Measured** (the prototype: 167 rules, 579 alternatives). A record's text 28 µs, a world's history 1.5 to 2.9 ms, the grammar parsed in under 2 ms. Repeated frames of six words or more (the genres' formulas left out) at 50, 100 and 200 records a world: 12%, 21–22% and 33–35% (C-02: 11–20%, 34–38%, 50–51%). The harness's checks found nothing in 3,000 records a world but lengths at the genres' edges. The targets for H-03 and H-04: under 15%, 25% and 35%, every check at zero, a record under 50 µs, a history under 10 ms.
* **The data model the peoples' milestones build to** (the note's section 6). A-01's profile: the age, the eras with their years, the collapse's cause, onset and stages, government, faith, economy, values, the genome's meanings. H-02's history: entities with kinship, homes, trades and causes of death; events with roles, season, cause, value, outcome and their own text; the per-year index; causes kept for every event that has one, a death in its cause's season. T-04's records: genre, date, writer, reader, subject, place, medium, the events told, tags, the tokens with their lemmas. The inputs list by group.
* **The language layer.** The tongue maps lemmas, not word forms, with endings of its own, and the language share counts lemmas (H-04 re-tunes it). The voice reads a record's opening. C-16's programmes take whole sentences from the radio ages' records.
* **What moved.** Nothing in the game or the harness; no generation change (`GEN_VERSION` 12). The two notes in `docs/ideas/`, the H-01 rows of `ITERATION-1.3.0.md` and `MILESTONES.md`, `PROGRESS.md`, R-411's status, KI-368 (C-02's measured repetition, for H-03 and H-04).

## A-01 (2026-10-08): the ages

The first milestone of the peoples (R-411, `docs/ideas/IDEAS-civilisation-expansion.md` section 1): a people is found at the age it had reached when it collapsed, with a profile every later milestone reads, built to H-01's data model (`docs/ideas/RESEARCH-records.md` section 6). `docs/reference/12-civilisations.md` ("The ages") has the numbers.

* **The age is the people's, not the world's.** The six ages are drawn from the people's generator by the decided shares (28, 32, 18, 13, 7 and 2 per cent: the early ages commoner, the far future rare, about one people in fifty: 32 of the 1,501 within 25 ly of home, the nearest 8.6 ly away and 21 ly on the harness's plane), whatever the world. A world's type, sun and system weigh on the cause instead, where they can say something (a desert dried with its people, a red dwarf flares). (R-413, 2026-10-11: a world's second people takes its first's age.)
* **Years in the people's own count.** Year 1 is the first towns' founding (the medieval age's start), or a prehistoric people's first village; the ages before it are counted back, negative. So the eras, the radio's year, the collapse's onset and stages are all in one reckoning the history (H-02) and the records (H-04) can date by. C-02's lore keeps its own span (150-600 years from the founding) and the shards their years until H-04 rebuilds them on the history: the profile is in no text yet.
* **The end's years by the age, the weathering by the age's durability.** C-08's draw is kept (its hash, log-uniform), over 200-40,000 years for the first two ages (C-08's span: their years did not move) and longer spans for the late ages, whose works outlast them (to 60,000, 100,000, 250,000 and 2,000,000 years). The ruins' decay reads the years over a durability by the age (1, 1, 1.5, 2.5, 6, 40), so a late people found long after its end is not foundations by that alone; A-03 gives each material its own decay and replaces this.
* **The cause by the age and the world.** Thirty-seven causes (the ideas note's lists, with the world's own disasters weighed in at every age by what the world and its system are), how long the collapse took by the cause and the stages by its speed. Two peoples that ended together (C-13) share the first's years and one cause that can end a world, drawn from the world's seed over both ages; where the other people went on after this one, conquest and war come from them. Prehistoric peoples ended by a nuclear exchange exist: a neighbour's war that ended both.
* **The extent from the land of their time.** The land share is sampled (64 points at 2 km, a dead desert's old seas counted as water: they were seas), so a world of islands holds fewer than a world of continents; the held share and the density are by the age (Earth-like orders: a medieval people some hundreds of millions, a modern one some billions on a world of 5,000-7,600 km).
* **Names, not only numbers.** Every enum has a name table as the grammar will read it (lowercase noun phrases; the ruler's title as a man and as a woman), as the research asked. The genome's meanings (the sacred number, the focal monument, the dead's custom) are A-04's and are not in the profile yet.
* **Two functions.** `peopleAgeOf` (the generator alone: the age, the eras, the end, radio) is what the per-cell readers need (the culture, the radar, the charts); `peopleProfileOf` (with the system and the body: the cause, the extent, the rest) is the whole, 0.25 ms.
* **Only the radio ages on the air, the density kept.** A transmitter sector's people's world transmits only where one of its peoples had radio (an industrial people that got far enough into its age, every later one: 29% of all), so C-07's candidate rate went up 3.4 times and the mean heard within 200 ly on the arm stayed 1.85 (C-07's 1.8). The salt was re-pinned from sixty: home hears three peoples of three ages (modern at 90 ly, futuristic at 184, industrial at 197), each test world four with a far-future people among them; C-15 pins it again with the powers. A world of two broadcasts its peoples that had radio only (`slotPeople`), and a chart's "heard" mark (C-10) needs radio on both ends.
* **What is heard is not yet the age's.** A signal heard at d ly left d years ago; a people that ended E > d years ago was gone when it left, so what reaches the radar is its machines (C-07's "dead machines"); the programmes are C-07's whatever the age. C-16 makes each age's sound on the air and can make the time tell (only machines where d < E, the living voices from farther: a reason to turn the gain up). KI-370.
* **A test world of each age, pinned.** The nearest people of each age on the harness's plane (sector y 0, so every mode's `<sx> <sz> <body>` reaches it; `second` for the modern one), in `AGE_WORLDS`; `peoples ages` says when they move and the unit checks they hold.
* **What moved.** `GEN_VERSION` 13; the regress unchanged (no hashed frame has a people). `galaxy/peoples.*` (new); `Culture::age/weatherYears`, `cultureOf` reads the profile's end; `Lore::otherFate` by the profile's ends; `starSignal`, `localSignals`, `slotPeople` (the radar's coin moved there), `transmitterCell`'s rate and salt; `chartMarksOf`'s heard marks; the harness's `peoples ages`, the profile in `peoples <sx> <sz> <body>`, `AGE_WORLDS`; three unit checks (217). KI-369 (the ages are in the data, not yet in the world), KI-370 (the radio's time).

## C-15 (2026-10-08): deep listening, the radar's gain

The first of the faint (R-410): a gain in the radar camera, x1 to x32 in six steps. At x1 the radar is C-07's under A-01's rule; turned up it hears transmitters beyond 200 ly as far as each one's power carries it, at the price of the static. `docs/reference/12-civilisations.md` (the radar's gain) has the numbers.

* **A power per transmitter, in two populations.** The plan's "most weak, a long tail of strong ones". The sectors of C-07's cells (A-01's salt and rate, kept) are the weak, powers 0.005-0.05 log-uniform, carried at most 319 ly at x32. A sparse second population of strong transmitters is hashed in cells of 256 sectors (`deepCell`: a Poisson count of mean 1.4 a cell, one candidate in 12 million sectors against C-07's one in 85,000) with powers 1-40 log-uniform, carried to 7,045 ly at x32. Their own coarse cells keep the scan cheap: within 7,000 ly there are 45 million of C-07's cells (a quarter of a second to hash before a candidate is read) but 170,000 strong cells, each candidate read by its power before any star is looked for (about 10 ms from home at x32, spread over five frames).
* **x1 is C-07's, by rule.** Within 200 ly every transmitter is heard at every gain, at C-07's strength by the distance, whatever its power; beyond, a transmitter of power p is heard at gain G within 200 sqrt(1 + (G - 1) p) ly, its margin over the floor (reach over distance) squared and read as C-07 reads the distance (a strength of 0.18 at the edge of its reach). A purely physical law (a received power p/d^2 against a floor 1/G) cannot give both: an x1 that hears every transmitter within 200 ly and none beyond needs every power the same, and then the gain cannot single out the strong ones. The rule is the compromise; the player meets a receiver whose near list never changes and whose far list grows with the dial. x1 hears exactly A-01's lists at home and the nine test worlds (compared with the A-01 build).
* **Beyond 200 ly, only what was on the air in the light that reaches the ship.** A signal d ly off left d years ago. A-01's ends run from 200 years, so within the reach the light always left after a people's end (C-07's dead machines still running). Beyond it the light can be older than the radio: with powers alone, the first pin had two of home's eight far peoples heard from a time before they had any (Greyrveideand III at 6,379 ly ended 998 years ago and had radio for 377). So a far people is heard only where d is within the years since its world's first broadcast (`Signal::airYears`: the end's years plus the radio's). That silences about a third of the strong population's peoples within reach (its density went from 0.85 to 1.4 a cell to keep the steps), and what is left is almost all machines (d below the end's years): the window in which a people is heard alive is its radio's years, decades to centuries for most ages, so a living voice from afar is rare (none from home). KI-370 has what C-16 can make of it.
* **The steps even in the gain, not doubling.** Measured on the real galaxy (`scratchpad/c15/sim.cpp`, 300 layouts of the salts): the peoples on the air within D of home grow as D^2 beyond about 800 ly (the disc), and under the reach law the area within a transmitter's reach grows with (G - 1) p, so equal steps of G add equal areas. A doubling dial (x2, x4, x8, x16, x32) adds 0.6, 1.2, 1.8, 2.4 and 4.4 peoples from home on average and no layout gives one to three at every step from the four places; x5, x11, x18, x25, x32 add 2.3, 2.2, 2.2, 2.0 and 1.7 (the last lower: farther, more of the light is older than the radio). The dial says `GAIN X11`; the reach is the transmitter's and is never printed.
* **The salts pinned together.** Of 2,000 pairs (the weak powers' salt and the strong cells'), 33 gave home and the three C-series worlds one to three peoples more at every step and home's farthest at x32 within 4,500-7,000 ly; two kept every step within one to three at the six age worlds as well (50 steps of the ten places), and the one kept (0xC15A0165, 0xC1500165) puts a far-future people among home's far signals. From home: A-01's three at x1, then +3 (202, 216 and 349 ly), +1 (3,085), +1 (3,469), +2 (211 and 4,548) and +2 (4,500 and 5,524: Phiostraisia I's far-future people and Esamiala I's modern one); five pulsars more, the farthest at 3,841 ly. The strongest are heard to about 5,500 ly from home and up to 7,000 elsewhere (the plan's "about 6,000").
* **The pulsars.** A pulsar in a transmitter sector carries a third of its sector's power (no light rule: a pulsar does not end), so about one is added a step against two peoples. The loud near pulsars keep their 25 ly at every gain: scanning them sector by sector farther costs the cube (25 ly is 130,000 sectors, 100 ly eight million).
* **The price: the floor, the overload, the hold, the cone.** The static's floor rises with the gain (0.07 at x1, 0.37 at x32), wanders wider and fills the scope with grass, so a far signal is a small rise over it; above x1 the meter's top is soft (a knee at 0.6), so a full meter is an overload's alone. A near or a local signal reads a little louder on the meter (its strength times G^0.1) but drives the receiver hard (times G^0.4): with the beam on one whose drive is 1 or over (at x32 the peoples within about 165 ly and most of a system's own) the meter pins red, the hold is refused (`OVERLOADED - TURN THE GAIN DOWN`) and the sound is overdriven (`AudioState::radarDrive`, the signal pushed into a hard knee: the renders keep their peaks under 0.5 and their rms within C-07's, a voice's crest factor 7.5 falling to 4.3). The first build read the meter by the drive too: the near signals' side lobes and the floor pinned it at 0.99 on the far signal at x32 from home, so the meter and the overload are two readings now. The hold grows by 1 + 0.25 log2 G (x2.25 at x32: eleven seconds for a faint far signal), the lock cone narrows from 3 to 2 degrees and the keep cone from 8 to 4; turning the gain down under a locked signal loses it (`LOCK LOST - TOO FAINT AT THIS GAIN`).
* **The hold is on what the crosshair has.** C-07 held the strongest signal when it was within the lock cone; at a high gain the near signals are loud in their side lobes and the strongest is seldom the one centred, so the hold is on the loudest within the lock cone (at x1 the same whenever C-07's could hold), and what is heard follows it.
* **The far scan in slices, not on a worker.** The plan put it on a worker; G-07 found that a worker's timing makes the harness's frames machine-dependent, and the whole scan is about 10 ms. It runs 40,000 cells a frame (five frames from home, `LISTENING DEEPER...`), once a sector when the gain first goes over x1, and keeps the whole dial's list (each signal with the least gain that hears it), so a step up or down is a filter. The x1 scan stays C-07's, on the main thread.
* **What moved.** No generation change: `GEN_VERSION` 13 is A-01's and 1.3.0 is unreleased, so the strong transmitters join A-01's change; C-07's cells, their salt and every transmitter within 200 ly of the test places are as A-01 left them. The regress unchanged (no hashed frame has the radar). `transmitterPower`, `deepCell`, `signalReachAtGain`, `signalNeedGain`, `signalStrengthAtGain`, `signalLevel`, `signalDrive`, `SignalScan` and `signalsNear`'s gain; `sectorTransmits` reads both populations; `Signal::power`, `needGain`, `airYears`; `RadarState`'s dial and far list, `radarGainStep`, `radarDeepStep`; `thousands` moved to `ui.cpp`; the harness's `signals` by the step and `x32`, the overdriven renders of `signals wav`; two unit checks (219); four flow frames. KI-370 rewritten, KI-371 for what stays.

## C-16 (2026-10-09): what the gain lets in, and each age on the air

The second of the faint (R-410): "radio noise, creepy mixed sounds when you are trying to get really far signal". C-07's receiver is the instrument and the gain is how much of the void comes through it. The register stays C-07's: the sound tells, the readout never does. `docs/reference/12-civilisations.md` (what the gain lets in) has what is heard where.

* **One receiver, two channels.** C-07's synth heard one signal at a time, its oscillators the synth's own. A signal under another needed every signal's sound in a channel of its own (`AudioSynth::RadarChannel`, `channelSample`): the one centred, and the loudest other within forty degrees (`RadarState::under`). The under one bleeds in with the gain: none at x1, which keeps C-07's receiver exactly, and up to 0.6 of the way at the top. A people under another is heard as its age's machine or as the shape of a voice (a phantom babbling, `underVoiceOf`). Its own programme would need its world built for what is only a bleed. Two peoples' carriers beat into a whistle that slides as the beam turns (350-2,050 Hz, by the two seeds and the angle between them). From about x18 the two ring against each other: the centred signal ring-modulated and multiplied by the other, a metallic intermodulation.
* **The floor rises, the loudness does not.** The bed's hiss grows 2.6 times at x32, a crackle of clicks comes in (up to 55 a second) and the rumble breathes deeper and quicker. A receiver's gain lifts the noise, not the loudness, so the rumble falls by 30% and the whole by 15% at x32. The bed's rms stays C-07's: 0.096 at x1, 0.090 at x11, 0.095 at x32 toward the plane. C-07's lesson held: the levels were set from rendered WAVs first.
* **The galaxy's noise is the band's map.** `radarSkyAt` reads the band's brightness (the map the sky already has, G-03) at the beam's centre and on a ring of six points ten degrees out.
  * The bed gains a scintillating rush by it: 0.15 of it at x1, all of it at x32.
  * The meter's floor rises by up to 0.12 toward the core at x32.
  * A far signal comes through up to 35% less there, so a far signal toward the core is the hardest to pull out. From home the core is at AZ 115 EL 0, and a far-future people at 4,500 ly sits three degrees from it.
  * A real radio sky would not show the dust's rifts, but the visible band is the sky the game has (KI-372).
* **Far voices worn by their distance.** The wear is log(d/200)/log(30): none within 200 ly, 0.47 at 1,000 ly and 0.97 at 5,500. With it the content comes through:
  * in a narrower band (the high-pass up, the low-pass down);
  * with its pitch drifting (the wow's delay wandering by up to 34 ms over tens of seconds);
  * with deep slow fades (to a tenth at the bottom of a cycle of tens of seconds);
  * with dropouts that come oftener and last longer;
  * with three echoes of itself, 0.13, 0.29 and 0.47 s behind (the paths it came by).
  The signals within 200 ly are C-07's.
* **The static's events, oftener and stranger.** C-07's seven come every 12-40 s at x1 and 3.2 times as often at x32 (4-13 s). From x25 five stranger ones join, up to 55% of the draw at x32:
  * speech running backwards (syllables swelling out of their own echo and cut off, the phrase's melody rising);
  * counting whispered in no one's language (a two-syllable word a number, the last drawn out);
  * a breath close to the microphone (the pop, the lips, in, a held moment, out);
  * a knock answering a knock (the answer nearer and deeper);
  * a choir swelling and stopping (four voices in a semitone cluster, cut off at once).
  The near ones are heard dry, outside the echo. The void's echo makes everything sound far, so near means without it. Over two minutes the renders hold 4 events at x1 and 11 at x32.
* **A word heard before comes back.** At x25 and x32, with nothing on the air, every 35-90 s a word of a recording from `Guide::heard` is spoken in its own world's voice. Sometimes two or three words come together. It comes through the receiver as from very far: the wear's band and echoes, at a level of its own. It is never a recording not heard, and it is never logged. When the radar's own world is the one, it is used; otherwise that world is built once (C-07's `buildShardWorld`) and kept.
* **False rises.** At x25 and x32 the noise makes a rise 8-30 degrees off the beam every 12-45 s.
  * A rise lives 10-16 s and lifts the meter by 0.16-0.30 (a far signal's rise), and the source's smudge shows as a real one's would.
  * Where the beam has it, the static gathers into almost a tone.
  * Held, the hold fills a third to two thirds of a faint signal's and empties, `NOTHING THERE` shows, and the rise fades.
  * It never locks. The meter is what the receiver hears, and noise makes ghosts: the radar stays honest as an instrument.
* **Each age on the air** (the expansion note's section 10). It is built here rather than at H-05, so the receiver is designed once. A slot's programme is by its people's age (`programmeOf`'s `age`; `Signal::age` is the first people's, for the carrier before a programme is known).
  * **The industrial age:**
    * A phonograph in six text slots in ten and in one music slot in three: the recording through a horn's hump, the needle's edge and the turntable's wow, over the record's hiss, its crackle and a tick every turn.
    * Otherwise Morse by spark. The text is keyed in a code of the people's own: each letter is one to four dots and dashes by the world's hash, after the station's call twice, as much as forty seconds holds. A music slot keys the call and a group.
    * The spark gap fires at 260-740 Hz, each spark a damped ring and a crack: a quenched gap's sparks are even (a note), a plain one's ragged.
  * **The modern age:** C-07's eleven, so home's nearest, a modern people, keeps the variety C-07's check pins.
  * **The futuristic age:**
    * a synthetic voice: one pitch, no breath, a ring at 38-72 Hz under it, its samples held on a coarse clock of 4.5-7.5 kHz;
    * data and beacons (C-07's machines);
    * the piece in one music slot in five.
  * **The far future:** a beacon of mathematics, and a synthetic voice in one text slot in four. The beacon sends pure pips, a group a term, by the world's seed: the primes, the squares, Fibonacci's numbers, the powers of two, the triangular numbers, the harmonic series as pitches, a count in binary sent high and low, or the digits of pi.
  * **The carrier by the age:**
    * modern: C-07's 1,200 Hz;
    * industrial: none, only the spark's rasp;
    * futuristic: a thin whine that flutters;
    * far future: a pure tone at 1,420 Hz (the hydrogen line's number) that never moves.
  * A Morse keying, a phonograph and a synthetic voice carry the recording, so it counts as heard (`speaks`); the mathematics does not.
* **The picture.** With the gain, in the radar camera:
  * the static (+50 intensity units) and the snow thicken;
  * the band glows through the phosphor, from two shades at its brightest at x1 to twelve at x32, sampled on a grid of every eighth pixel (the radio sky the receiver hears, KI-349's "not a radio sky");
  * the star field ghosts a faint double (three pixels right and one down, 0.45 of it at x32);
  * the rolling band runs faster (6 s at x1, 2.4 s at x32);
  * the picture tears now and then (a band of rows thrown sideways for an eighth of a second, about one tear in three seconds at x32).
  The scope reads `SWEEP` while a false rise's hold empties.
* **What moved.** No generation change (`GEN_VERSION` 13). The regress is unchanged (no hashed frame has the radar).
  * `galaxy/signals.*`: the four programmes, `programmeOf` and `programmeFor` by the age, `Programme::piece`, `code`, `unit` and `age`, `MORSE_OFF`, `morseSeconds`, `Signal::age`.
  * `game/audio.*`: the channels, the floor, the sky, the wear, the events, the ages' sounds, `mathsName`.
  * `game/radar.cpp`: `radarSkyAt`, `radarRiseStep`, `radarWordStep` and `radarWordStop`, the under signal, the picture.
  * `RadarState`: `progAge`, `sky`, `under`, `rollPh`, `rise`, the word.
  * The harness: `RxOpts`, the C-16 renders of `signals wav`, the AZ, EL and age of each signal in `signals`, three unit checks (222).
  * KI-349 and KI-370 narrowed, KI-371 closed, KI-372 new.

## A-02 (2026-10-09): the forms the renderer needs

The first of what they built (R-411, the expansion note's section 4): the shapes the ages' buildings will be made of, their broken states, their colliders, their levels of detail down to a skyline, and a people's colours in banks. Who builds what is A-03's and A-04's; until then the game's settlements are C-01's. `docs/reference/12-civilisations.md` (the forms) has the kinds and where to see them.

* **One form, many kinds.** C-01's `RuinElem` (a box or a dome) became `Form` (`galaxy/forms.*`) with twelve kinds and their variants: the box, the dome (four profiles), turned forms (six profiles, round or a prism), pyramids and stepped masses, vaults, arches and arcades, shells, trusses, cables, rings, sweeps along an arc or a line, grown forms. A form is geometry alone (`formMesh` at a level, `formColliders`, `formProfile` for its silhouette), read by the view, the harness and the unit alike. Most kinds are a few generators underneath (a profile turned, a section swept, a grid bent), so A-03 can ask for shapes not on the list by their numbers.
* **C-01 to the bit.** C-01's box and hemisphere keep their own arithmetic and their faces' order, and the renderer keeps C-01's light and fog, so the nine civilisation scenes are pixel-identical to C-16's (compared PNG by PNG at 2x). The rename (`shape` to `kind`) touched the ruins, the graves, the shards' placement and the harness.
* **Broken is the form's, falling is the building's.** Every kind has its broken state from a share and a seed (a tower snapped and holed, a dome caved, a frame bent, a vault's stretch fallen, a keystone gone...). What it drops (rubble, a fallen top lying on its side by `tilt`) is the building's to place (A-03), and a form resting on another is broken with it by the building (the sheet keeps the supports whole).
* **A level a building.** C-01 drew a settlement at one level by the distance to its edge. A city two kilometres across needs each building's own: whole within 350 m, simpler to 1.3 km, coarse to 4 km, then a silhouette where it stands a pixel high (`gatherSettlement`, the pieces grouped by building in each cell, `RuinCell::groups`).
* **The skyline is the silhouettes.** Beyond 4 km a building's forms are flat outlines across the line of sight with their own profile up their height (`formProfile`, `formHalfWidth`, `formTop`). A tower under a pixel wide is drawn one framebuffer pixel wide at its own shade, and the mush averages it into its neighbours: the first build widened it to a logical pixel and paled it toward the fog colour, which against a dark sky stood bright ghost lines on the horizon. `stability skyline` shows no shimmer (a pixel's light over the city changes 0.3 a frame against 0.26 over the empty horizon beside it; no pixel jumps).
* **The fog is the terrain's.** A tower and the hill beside it fade alike, so the air decides how far a city is seen: on the peoples' worlds (9-12 km) a city is a pale cluster at 15 km and a ghost at 30, while an airless world's 45 km shows it clearly. Lifting the haze with height would let a far-future spike stand clear at 50 km, but the terrain would need it too (V-03, parked). KI-373.
* **The ground under a form.** A big form (over 6 m) stands on the highest ground under its footprint and reaches down to the lowest (an aqueduct's deck stays level, a block stands on its plinth). Near, that is the ground drawn this frame; farther, the coarse ring's surface that draws the ground there (`drawnGround`, `drawnRing`), read once per form while that ring does. C-01's far towns stood on 64 m samples with the rivers off: a mast in a wadi floated 22 m over the drawn floor. Reading every form's ground every frame cost 3 ms at the test city; the ring follows from the distance, so only a change of ring reads.
* **Colliders by the form.** Discs with their span of heights (B-405), C-01's rows along a wall kept; slices up a turned form by its profile, outlines for prisms and pyramids, an arch's piers with the opening free, a vault's flanks and closed ends with the hall free, a mast's legs. A broken form's missing parts have none. The unit's check tracks every frame of a walk: the explorer touches the form and never enters a disc (walking at a thin mast or along a curved wall the explorer slides round it, which is right).
* **Colours in banks.** `FormPalette` (in `Culture`, A-04 draws it): the structure, the accent, metal and glass, light; the first people's banks 18, 19, 20 and 24, the second's 25-28 (`FORM_BANKS`; banks 21-23 are the aurora's tops, the band and the storm). A slot can be the ground's own material, whose ramp is then that bank's (C-01's walls: the same RGB in bank 18 as in bank 0 or 9). The striated rock's grain stays on walls cut from the rock: on the test palette's concrete it buried the windows.
* **Windows are a pattern.** `FacePattern` in `RasterParams` (core): the face's own u and v in metres, a cell a bay wide and a storey high, the window's shape; where a cell spans under two pixels it fades to its mean, from four whole. A lit window switches to the light's bank. Thirty storeys cost what a blank face does.
* **Threads.** The gather is serial (it reads the ground caches); the forms are built, lit and fogged in 64 chunks on the workers, and drawn in bands of rows that every thread walks in chunk order, so the pixels are the serial loop's (B-310's contract). `parallelThreadsOverride()` lets the unit draw the city frame both ways in one run: the same hash, 2.5 ms against 4.8. Lattice members are prisms only where they span a pixel and a half, lines thinner (legs, chords and cables however thin, the rest from a fifth of a pixel).
* **What moved.** No generation change (`GEN_VERSION` 13). The regress is unchanged. C-01's scenes are pixel-identical.
  * `galaxy/forms.*` (new): `Form`, `FormKind`, `FormMesh`, `formMesh`, `formColliders`, `formProfile`, `formHalfWidth`, `formTop`, `formReach`, `WindowSpec`, `FormPalette`, `FORM_BANKS`.
  * `galaxy/ruins.*`, `graves.*`, `shards.cpp`: `RuinElem` is `Form`; `Culture::palette`.
  * `surface/forms_draw.cpp` (new): `gatherSettlement`, `drawFormItems`, `groupPieces`, `testPlaceSettlement`; `surface_view.*`: `drawSettlement` gone, `settlementColliders`, `drawnGround`, `drawnRing`, the peoples' ramps in `setupPalette`, `placedCells`, `testForceLod`, `RuinCell::groups/foot/placed`.
  * `core/raster.*`: `FacePattern`, `RasterParams::pattern`. `core/parallel.h`: `parallelThreadsOverride`.
  * The harness: `forms sheet|city|skyline|check`, `stability skyline`, the bench's `city 2x` (16 ms), four unit checks (226).
  * KI-373 new.

## A-03 (2026-10-09): what each age builds

The second of what they built (R-411, the expansion note's section 2): each age's buildings from A-02's forms, standing in C-01's
plans until T-02 builds towns by age, and their decay by material. `docs/reference/12-civilisations.md` (what each age builds) has the
kinds, the plans, the shares and the materials' table.

* **A kind is an age's way of building.** About forty-five generators in `galaxy/buildings.*` (`buildingForms`), each a kind of one
  age (`buildingAge`): C-01's ten kinds are the medieval age's, roofed; the works every age built its own way (a gate, a wall, a stela,
  a heap, C-08's features) are built in the building's age's material. A building keeps the age whose way built it (`Building::age`),
  and `cultureAs` / `settlementOf` lay out a people's settlement as if it had reached any age, so T-02's old cores and megaliths are
  generators already, not new code.
* **The plans stay C-01's, the places ask the age.** A dwelling, a centre, a hall, a tower, a stela, a monument: each age answers
  with its own kind and size, and the plan's spacing and radius scale with the age (0.85 to 2.4). The futuristic age has a plan of its
  own (`SP_GREAT`: a few great structures, the land between wild), the far future one structure. How many cells hold a settlement
  follows the age (the prehistoric camps days apart, the industrial and modern towns oftener, the futuristic few, the far future one
  cell in about seven hundred); a lone monument draws its kind and size with the class's size, so the finders' radius (read without
  the layout) is its reach.
* **The decay is the material's.** `materialDecay` is a log-logistic in the years for each of ten materials (the years to half gone at a
  middling rain, the decades from a tenth to nine tenths, how much sooner the rain takes it). Masonry is C-08's `decayOf` to the digit
  (half at 10^(0.92/0.3) years), so a medieval town of stone weathers as the C series' did. The buildings read the real years since
  the end: A-01's stopgap (the years over the age's durability) stays only for the roads' wear and the sand's burial, which are not
  the buildings'. A building's luck (C-01's decay draws: a quarter for the building, a sixth for the settlement) moves a material only
  where it is half gone (`2.5 d (1 - d)`), so glass is gone and the far future's material whole in every building, and a concrete tower
  after three thousand years is a stump in every one (the first spread, four times as wide, left a lucky tower whole).
* **Nothing rests on air.** A roof, a dome, a spire, a lintel, a tank, a tower's shaft never outlasts what holds it up: a roof needs
  every wall at 92% of its height, a tower's lobby stands whole while its shaft does (and falls with the rest after), a gasometer's
  girders go before its columns, a maglev's span lies on the ground where a pylon fell. The unit checks it piece by piece (every piece
  over the ground has one of its building under it that reaches it). A tall thing that fell lies across the ground away from the
  town's middle and breaks up with its material; the heap at its foot lies on the side it fell, so the tower's lobby stays a room.
* **Rooms are the buildings' own.** `buildingRoom` replaces C-03's per-kind spots and C-01's `buildingDoor`: the floor, the doorway and
  the spots, from the generator's own draws. A shard's place is its building's kind (`ShardSite::place`, `outside`; the log names
  it with `buildingPlaceName`); a spot is cleared against the building's colliders, not its boxes. A great structure's room is the gate
  hall before it (an open vault); a standing stone's, a ring's and a far-future structure's records lie at their foot.
* **A clearing that grows over.** A settlement's yards keep a twentieth of their trees two centuries after the end, a third after ten
  thousand years (C-01's rule): the prehistoric scene stood in a forest that hid the village.
* **Two A-02 errors found.** A form lying on its side had its row of colliders centred on its foot (half of it behind, `cos` for
  `sin`), which shut a fallen tower's lobby; a `Form&` from a generator dies with the next piece added (a spire's collar floated):
  every generator copies what it needs before adding more.
* **What moved.** No generation change (13 holds: 1.3.0 is one generation). Every civilised world's settlements, its shards' places
  (the indices stay), its trees in the yards; the regress unchanged; C-01's scenes stand on whatever age their worlds' peoples are.
  * `galaxy/buildings.*` (new): `BuiltMaterial`, `materialDecay`, `buildingDecay`, `buildingAge`, `buildingPlaceName`, `buildingForms`,
    `BuildingRoom`, `buildingRoom`, `agePalette`.
  * `galaxy/ruins.*`: the kinds of the ages (`BuildingKind`), `Building::luck/age/style` (`decay` gone), `Culture::roof/temple/hut/
    towerForm/great` and the age's palette, `SP_GREAT`, the plans by age, the shares by age, the monument's pick, `settlementOf`,
    `cultureAs`; `elemsBuilding` moved into the generators.
  * `galaxy/shards.*`: `ShardSite::place` the building's kind, `outside`; `SHARD_PLACE_NAMES` gone. `galaxy/forms.*`: `Form::material`,
    part 8 (earth), the lying collider; colliders skip what is lower than a step. `surface/forms_draw.cpp`: the soil's bank for earth.
    `surface/flora.cpp`: the clearing.
  * The harness: `ruins ages`, `ruins kinds`, `ruins check`, `peoples fresh`, the scenes `civilisation_<age>` (`AGE_SCENE_WORLDS`), four unit
    checks (230); C-01's, C-03's and C-08's checks read every building's room; the dead desert's and the town's scans of the globe run
    without the rivers' tiles (the unit takes two minutes and a quarter, from six and a half).
  * KI-374 new; KI-373 and KI-350 narrowed.

## A-04 (2026-10-09): the architecture genome

The third of what they built (R-411, the expansion note's section 3): every people's architecture from thirty-five deep variables it
keeps through its ages, read through the age it reached. `docs/reference/12-civilisations.md` (the architecture genome) has the table
of the variables, what each does and in which ages.

* **A variable a hash.** `genomeOf(peopleSeed)` draws each variable from its own hash of the people's seed (as C-09's and A-03's
  draws did), so C-01's stream and every draw before it stay, and a variable added later moves none of the others. A variable is a
  number in [0, 1): a category is its share of the range, a measure the number itself, so the harness and the unit set any of them
  (`Genome::set`) and draw again (`cultureWays`).
* **The genome is the culture's ways.** C-01's `tall`, `plan`, `towers`, `style` and `stoneRoofs` and A-03's `roof`, `temple`, `hut`,
  `towerForm`, `great` are the genome read through the age now (`cultureWays`: the proportion, the streets, the appetite for height,
  the structure, the roof, the focus and the centre, the plan and the massing, the technique); the generators read those and the
  genome; the unit's tests that set a way (a wattle village, the spike) still set it after.
* **The accent has its place.** One variable carries the accent's colour and where it goes (bands, roofs, doors, tops): a slot is what
  a piece is (`Gen::roofSlot`, `topSlot`, `doorSlot`, `timberSlot`). The four slots are the structure, the accent, the roofing or the
  glass, and timber and iron where the age has no light: a light's ramp only in the futuristic and the far future's (`glow`), and only
  the far future's structures glow (a dead city is dark). The ground's own stone is tinted a quarter toward the people's colour, so
  the base colour shows on a people of unpainted stone too.
* **Every variable acts at a scale the picture shows.** The plan's forms, the nine roofs in every age's material, the profiles, the
  towers' sections, the windows' shape, rhythm and share, bands and cornices, an expressed frame, tops and motifs on the great
  buildings, the focal accent (a building of its own before the centre), the centre's kind, the monuments, the graves' architecture,
  the plans (spokes, rings, scattered, a street, across the fall, branching), plazas, courts, gardens, compounds, rows, an edge, joined
  towers, arcades, colonnades, tunnels, a high place, an aqueduct, stilts, mounds, pits, cuts, terraces. The unit checks that every
  variable reaches the drawing in every age it says it acts in (each of its values set in three peoples' genomes in turn changes a
  town's pieces or the colours they use); light acts in the far future alone and a people built to last in the ages that weather.
* **The relation to the land is the buildings' until T-01.** Ridges, valleys, cliffs are where a people settled: the cells are hashed
  until T-01 plans a world, so the land acts on each settlement (a high place, an aqueduct, a cut, terraces, stilts, mounds, pits). No
  floor is raised where a record lies: a house on stilts keeps its records on the ground under its floor; a sunk house keeps its
  doorway 2.1 m over the ground outside.
* **The uniqueness is measured, not hoped for.** `genomeDistance` weighs the strongest variables (the base colour, the roof, the
  plan, the massing, the accent, the proportion, the focus, the profile, the height, the openings, the structure, the centre, the
  tops, the streets, the ornament: 13.4 at most); two peoples are alike under a quarter of it (3.4). The hundred peoples nearest home
  are distinct (the nearest pair 4.93 apart; random pairs under 4.0 five in a million); the nearest pair drawn side by side reads as
  two peoples (grey and pitched both, an oval village round a great tree and a walled town round a domed temple).
* **Found on the way.** C-01's rotundas and A-03's dry-stone huts, cisterns and observatories stood their ring segments across the
  ring as fins, and the stone circles their stones and lintels radially: a ring's segment lies along its chord now (`ringWalls`). Tops
  on a roof stood after their roof fell (a stone finial over a gone timber roof): they are the roof's own and stand while it does.
* **What moved.** No generation change (13 holds). Every civilised world's settlements (their plans, headings, sizes, kinds, colours),
  its roads' ends, its shards' places; the regress unchanged; the flow as A-03's but the zoom's roads (3,723 to 3,725).
  * `galaxy/genome.*` (new): the variables, their names and ages, `genomeOf`, `genomeValue`, the colours, `genomeDistance`,
    `techniqueOf`.
  * `galaxy/buildings.*`: `cultureWays`, `culturePalette` (for `agePalette`), the genome's slots, `walls4`'s profiles and dress,
    `ringWalls`, the nine roofs, `towerShaft`/`towerTop`, tops, figures, foundations, the motif; the new kinds' generators and rooms.
  * `galaxy/ruins.*`: `Culture::genome/technique/life`, `Building::form/sides/found/centred`, `BK_FOCUS`, `BK_WINDMILL`, `BK_AQUEDUCT`,
    `BK_TWIN`, `BK_PORTAL`, `Foundation`; the plans by the genome; `orientedHeading`. `galaxy/graves.*`: the dead's architecture.
  * `galaxy/forms.h`: `WindowSpec::group/scatter`, `FormPalette::tint/glow`, parts 9-11. `core/raster.*`: the triangle and the keyhole,
    mullions, scattered cells. `surface/forms_draw.cpp`, `surface/surface_view.cpp`: the tints, the light only where it glows, the
    water's, the bark's and the leaves' banks.
  * The harness: `genome sheet|ages|distinct|pair|print|reach`, `ruins kinds`' page of the genome's works; three unit checks (233);
    A-02's banks check reads an untinted people; C-08's terraces allow a terracing people's gentle slopes; A-03's heights to 800 m (a
    lofty people's towers to 650 m and their masts).
  * KI-375 new; KI-374 narrowed.

## T-01 (2026-10-09): a people's map of its world

The first of the towns and the works (R-411, the expansion note's section 5): a plan of each people's world, read once a world, which
the 2 km cells, the view, the maps, the telescope and the scanner read. `docs/reference/12-civilisations.md` (a people's map of its
world) has the plan's parts and where to see them.

* **A plan of the world, not a hash a cell.** The cells were hashed alone (a third of them settled, the whole world over). Now a coarse
  reading of the world (the planet function on 165,018 cells of half a degree, without the rivers' tiles: the land of the people's
  time, the height, the roughness, the climate, the fertility, the coast, coal and ore, and the rivers of a flow over the grid) is
  scored for the age and the people's relation to the land, a territory is grown from the capital to the profile's held share, and the
  places and ways are laid in it. Where it is built: once a world, on a thread of its own at the approach (the parking, the landing map
  and a load ask too); 0.24-0.67 s on one thread for the six age worlds (the reading 0.2 s); four worlds kept; whatever asks first
  waits for it or builds it (the harness and the unit build at once).
* **Places by rank, the countryside by density.** The plan lays the places a history can name (the capital, the cities, the towns; a
  prehistoric people's gathering place and its bands' seats; a futuristic people's great places; the far future's one to three works),
  hundreds a world, each with its rank in the founding, its population and its core; the countryside (C-01's hamlets, villages and
  market towns, a lone monument) stays the cells' own by a density the plan gives (millions a world: a list of them would be a hundred
  megabytes and seconds to build). Nothing stands in the land no people held, nothing in a core but its city.
* **A city is its core, as the renderer affords it.** The cores hold C-01's density at the age's spacing, so a capital is 1,100-1,400
  buildings (a medieval one 470 m across its radius, a modern one 1.15 km; A-02's test city of 1,348 drew in 8 ms at 2x) and spans 2 km
  cells: the view reaches it from every cell it covers and draws the plan's places to 24 km for the skyline. A modern capital's suburbs
  and districts, its street plan and the density falling from its centre are T-02's; T-01 builds a city's middle up (tenements, blocks,
  towers by the people's appetite for height) so it reads as one.
* **The networks join real places.** A greedy spanner over the places' nearest neighbours (each link kept where the laid ways go round
  by more than the age's detour), the water dear, the trunk the routes between the capital and the cities: a web as dense as the age
  travelled and connected everywhere. The ways are laid on the ground in legs between waypoints every 3 km (made when first asked: a
  ground walk of a 500 km way is never needed whole) and walked by C-09's walk with each kind's weights; C-09's lanes join the
  countryside's settlements as before.
* **What is seen from orbit is what is walked on, at every scale that shows it.** The zoom and the telescope's plate draw the cores and
  the ways as the ground has them; the whole-world map draws nothing at its scale (a core is a thousandth of a 74 km texel), and nothing
  is exaggerated to be seen. The scanner hears the plan's places by their size, so an explorer outside the land a people held is led to
  it.
* **Two peoples meet only where their lands do.** Each people grows its territory on its side of the divide from its own capital; on
  Eleinewai I a prehistoric and a medieval people hold lands far apart, so C-13's frontier roads are checked where territories meet
  (and the rule on two made-up settlements). Contact, trade and war between them are H-02's.
* **Found on the way.** C-09's road blocks stepped by one longitude index in every row; rows have their own steps, so far from the
  meridian the next row's cell of that index lies cells away (a hamlet's neighbour 1.8 km off was outside its block): `ruinCellLonIn`.
* **What moved.** No generation change (13 holds). Every civilised world's settlements, its roads, its shards' and graves' places; the
  civilisation scenes (from a town of the plan, the ages' from the capital); the regress unchanged; the flow as A-04's but the zoom's
  roads (its step now round a town of Aieliaalas II's plan).
  * `galaxy/plan.*` (new): `PlanGrid`/`readPlanGrid`, `planPeople`, `WorldPlan`/`buildWorldPlan`, the cache (`worldPlanOf`,
    `worldPlanIfReady`, `worldPlanPrefetch`), the cells' reading (`planPlaceIn`, `planCountryside`, `planCoreAt`, `planPlacesNear`,
    `planEdgesNear`, `planEdgeWay`, `planSpansIn`), `NetKind`.
  * `galaxy/ruins.*`: `SC_CITY`, `SC_CAPITAL`, `SC_WORKS`, `RuinSpec::rank/wanted`, `settlementRadiusOf`, `townOrGreater`,
    `ruinCellLonIn`; `ruinOfCell` reads the plan (the places, the countryside, the maglev's spans); the cores' layouts.
    `galaxy/peoples.*`: `peopleExtentOf`. `galaxy/roads.*`: `roadLegWalk`, `netHalfWidth`, `Road::kind/trunk`, the blocks' rows.
    `galaxy/landmarks.*`: the plan's places heard by size. `galaxy/shards.*`, `graves.*`: a city's and a capital's counts, none for works.
  * `surface/*`: `SurfaceView::planNear`, `refreshPlanNear`, `forNearbyRuins` and `drawRuins` read it; `SiteRoad::kind`, the ways' legs
    in `ensureRoads`, their beds in `sampleAt`. `space/space_view.*`: the plate's cores and ways. `game/*`: the prefetch, the zoom's
    cores and ways by kind, the HUD's and the log's way kinds, the scanner's `A CITY` and `A GREAT CITY`.
  * The harness: `plan <sx> <sz> <body>`, `plan ages`, `plan view`; the bench's `descent people` (57.4 ms of 80) and `capital 2x` (8.9
    ms of 16); five unit checks (238); C-01's, C-08's, C-09's, C-12's and C-13's checks read the countryside round a town of the plan.
  * KI-376 new; KI-352, KI-373, KI-374, KI-375 narrowed.

## H-02 (2026-10-09): the history

The first of the histories and the records (R-411, the expansion note's section 8): each people's history, simulated over the plan's
places before the towns, the works and the records that will read it. `docs/reference/12-civilisations.md` (the history) has its parts
and where to see them.

* **A history simulated, then told.** H-01's answer: the depth is in what a text tells, so a people gets the history the records will
  draw on (Dwarf Fortress's order). A year at a time from year 1 (the capital's founding: the reckoning's start) to the end, with a few
  round-year legends of the old time before it; deterministic from the profile (A-01), the plan (T-01) and the world; built when asked
  (0.9-6.5 ms on the six age worlds, the world's plan first) and kept (`historyOf`, eight). Not prefetched at the approach: nothing in the
  game reads it yet (T-02's cells will, and then it is built with the plan on its thread).
* **The places are the plan's.** A history names no place the ground does not have, so every place it founds is one of the plan's, by
  its rank, in the era whose ground suited it (the plan's scores for the older ages over the territory); a futuristic or far-future
  people's older cities, which its plan does not draw, are places it cleared before its collapse (nothing of them stands). A place left
  early stays on the plan (T-02 draws it in its older way); every place ends by the end, the last one at it.
* **Ten families for six centuries.** The records need writers and readers with kin: ten households are followed through the last 600
  years (the whole history of a short one), births, marriages, moves and deaths with their causes. Past eight living, only the main line
  marries in: a branch let go keeps its life (its death drawn alone) but its children are not followed, so a family stays a household
  and a history some thousands of persons (1,100-4,800 on the six worlds; the prototype's 873 in 343 years). A family that dies out is
  replaced. Before the families' years the history follows its rulers, their houses and its figures alone, so a record of an older age
  names public persons (KI-377). The families' events are in the one list, marked private (the chronicle is the public ones: 330-3,435).
* **The rule by its form.** Life tenures (chiefs, a monarchy's houses with heirs by the people's rule of succession, elders, high
  priests), terms (consuls and speakers, three to seven years, longer in the later ages: a far-future speaker's twelve times), the
  machine's rule as a reign with no person. The government changes once in an era, to the one A-01 drew for its close.
* **Rates for reading.** The rates of wars, sicknesses, works, deeds and the rest were set on the six age worlds so a long history reads:
  a medieval people at war one year in sixty times what it valued war, a sickness one year in eighty-five, a battle's dead capped by its
  age, the comet told where it returns once in thirty years or rarer, the capital's centuries its festivals, a failed harvest told in a
  collapse only with its famine, a routine term's end and a former consul's death left to the families' record.
* **Two peoples, one shared history.** Where two peoples' years overlap and they could meet (lands touching, or one of them sailing),
  their meeting, treaties and wars are drawn from the world's seed in years before the present, so both histories hold the same; a
  people whose end A-01 drew as a conquest while the other went on is conquered by the other (who meets it first if it had not).
* **What moved.** No generation change (13 holds); nothing drawn or heard changed (the regress, the flow and the bench as T-01's).
  * `galaxy/history.*` (new): `History` (`HistoryEra`, `HistoryPlace`, `HistoryPerson`, `HistoryFamily`, `HistoryReign`, `HistoryWar`,
    `HistorySickness`, `HistoryInstitution`, `HistoryInvention`, `HistoryEvent`: 46 kinds), `buildHistory`, `historyOf`, the queries
    (`eventsIn`, `reignAt`, `eraAt`, `harvestAt`, `populationAt`, `inventionYear`, `eventsOf`, `eventsAt`), the lines (`historyEventLine`,
    `historyPersonName`, `historyPlaceName`, `historyWarName`, `historySicknessName`, `historyYear`, `historySeason`, `historyKinship`,
    `historySeat`).
  * `galaxy/plan.*`: `planScores` (a people's ground scored for an age, its own age's the plan's as before: the plans are the same to the
    bit). `galaxy/peoples.*`: `PeopleAge::fullYears` (each whole age's length as drawn), `WorldRead`/`readWorld` public.
  * The harness: `history <sx> <sz> <body> [second]`, `history ages`, `history check`; five unit checks (243).
  * KI-377 new; KI-376 narrowed (the two peoples' contact, trade and war are H-02's now).

## T-02 (2026-10-09): towns by age

The first of the towns and the works (R-411, the expansion note's section 6): towns laid out as each age laid them, with streets, districts,
an edge, the old cores the history founded them in, and C-08's reading of the site by age. `docs/reference/12-civilisations.md` (towns by
age) has the parts and where to see them.

* **Streets first, buildings on them.** C-01's plans placed buildings, and the streets were the gaps between them. A town now lays its
  streets first (`galaxy/towns.*`: the people's pattern in each age's way) and puts its buildings on the streets' lots, fronting them,
  tested as rectangles against each other (a row's houses side by side) and against every street's corridor. So "no building across a
  street" holds by construction, and the unit checks it over 41 towns. A building's own pieces that reach past its footprint (a mill's
  chimney, a mound's bank, a jetty, a gate's piers) are tested too: such a building is set back by as much once, else not built.
* **The plan's budget sets the blocks.** A city's buildings stay at its class's count (a capital about a thousand: the frame's budget,
  `capital 2x`), so a city whose buildings could not line its streets lays wider blocks (the street network laid again, the blocks
  the root of the ratio wider, up to three times). The density falls from the centre and with the street's rank (the main streets built
  up the most), and a lot left empty is a yard or a gap.
* **The history through the planet function.** A plan place's layout reads its history (its founding age, its fate). The history needs
  the star system, and a cell's layout has only the world's generator, so `Body` and `BodyGen` carry the star's sector and the body's
  index, and `historyOfWorld` regenerates the system (microseconds) when the history is not cached. The approach's plan thread builds the
  peoples' histories after the plan. A plan place's size (what the finders, the roads and the landing read without the layout) stays the
  history-free one: a place left early lays out its older town inside it.
* **Layers by the age's radius.** A city founded in an older age is built in rings, the founding age's inside, each later age's out to
  its own class's typical radius in that age (a capital 470 m medieval, 820 industrial, 1,150 modern), each ring in its age's way (the
  people's genome read through that age: `cultureAtAge`). The edge of an older ring is a boulevard; a medieval ring's wall stands in
  stretches inside a later city, its gates kept where they stood. A modern city builds its towers round its old core.
* **The countryside's past from the ages alone.** The countryside's settlements are not in the history, so a later people's village or
  town takes a founding age from the history's shares by a hash of its cell (its old core that age's), one in twelve of the older ones was
  left early (in its founding age, weathered from then), and three of five of its lone monuments are megaliths of its old time. The finders
  read the same without the layout.
* **One palette, older windows.** A people has four banks for its colours (the second people four more), so an old core is drawn in its
  people's colours; its windows are the age before's (the palette's fourth window slot), so a modern city's old streets show sash
  windows, not bands.
* **The ground marked where the eye is.** A town's streets, squares and fields are drawn into the terrain's vertices on the 4, 16 and 64 m
  rings (`SurfaceSite::addTownMarks`), added as a town comes within two cells of the explorer, the vertices under it sampled again. The
  paving's cut is deep (setts 0.6 of the albedo, asphalt 0.45: C-09's lesson that a fifth vanishes in the tone tile), the fields' tones
  fade with the years. A medieval lane of four metres is two pixels from 100 m up: it reads on the ground, not from the air.
* **Found on the way.** Compounds of wide houses overlapped (A-04's houses round a court at a fixed radius): the houses are sized first and
  the court set as wide as they need. A fallen dolmen slab fell into its chamber half the time: the slabs fall outward and the capstone
  slides off beside it. A leaning wall's doorway side stands upright. C-08's quay ran across the shore as a pier: it runs along the shore
  now. The genome's EDGE acts in the prehistoric and medieval ages (an industrial town is not walled; its medieval core keeps its wall).
* **What moved.** No generation change (13 holds). Every civilised world's villages, towns and cities change (their streets, districts,
  harbours, fields, walls; the countryside's older ages and megaliths), and with them the shards' and graves' places; the regress is
  unchanged.
  * `galaxy/towns.*` (new): `Street`, `Plaza`, `FieldPatch`, `TownLayer` (in ruins.h), `TownSpec`, `townStreets`, `townFields`,
    `StreetGrid`, `streetRectGap`, `buildingStreet`, `streetsJoined`, the names.
  * `galaxy/ruins.*`: `layoutTown` (the layers, the centre, the streets, the districts, the lots, the stations and market halls, the
    stadium and power station, the wall and gates, the works of the land, the features, the old stones, the fields), `placeHist`,
    `countrysideHist`, `cultureAtAge`, `Culture::seed`, `Building::district/before`, `RuinSpec::streets/plazas/fields/layers/buildAge/
    before/founded/harbourKind/reservoir`; the harbour by age, the reservoir; `BuildingGrid`, `Layout::fitsRect/offStreets/intrusion/
    undoTo/dwellingSpec`.
  * `galaxy/buildings.*`: `BK_CRANE` (a jib crane, a gantry), `BK_CONTAINERS`, `BK_RESERVOIR`; the dolmen's fall, the leaning doorway side,
    the older window in the palette's fourth slot. `galaxy/history.*`: `historyOfWorld`. `galaxy/system.*`, `galaxy/planetmap.*`: the
    star's sector on the body and its generator. `galaxy/plan.cpp`: the histories on the approach's thread. `galaxy/genome.cpp`: EDGE's ages.
  * `surface/site.*`: `GroundMark`, `addTownMarks`, `applyMarks`, `markCover`, `TerrainCache::invalidate`. `surface/surface_view.*`:
    `ensureTownMarks` (at a landing, a re-anchor and every 250 m), the flora and the rocks off the streets.
  * The harness: `towns <sx> <sz> <body> [second]`, `towns ages`, `towns view`, `towns check`; five unit checks.
  * KI-378 new; KI-377 narrowed (the towns read the history now).

## T-03 (2026-10-09): the works

The second of the towns and the works (R-411, the expansion note's section 6): everything a people made outside its towns, each placed by
its rule of the terrain or where the history made it. `docs/reference/12-civilisations.md` (the works) has the twenty-one kinds, their rules
and their ages' ways.

* **A layer over the ruin cells, not in them.** A work is not a ruin cell's settlement: `ruinOfCell` (what the finders, the roads, the
  shards, the graves and the landing read) is untouched, so no settlement moved and generation 13 holds. A world's works are one list
  (`worldWorksOf`, cached four worlds, built on the approach's plan thread after the histories: they read both), each laid out on demand as
  a settlement's pieces (`workSpec`), and the view visits them with the cells (`forNearbyRuins`). A work keeps clear of the settlements'
  radii, the cities' cores and the works laid before it.
* **The history first, then the land.** A battlefield is each war's greatest battle (two of a great war), between the battle's place and
  the place the other side came from (the war's head, else the seat, else the nearest place standing), on the line between them; a dam, a
  mine, an aqueduct, an observatory, a lighthouse, a launch site are where the history finished one (`HE_BUILT`, `HistoryWork` moved to
  history.h), in the era's age, weathered from when its place was left; the last shelter is by the history's last place; a people's older
  works of a city it cleared (a futuristic or far-future people's) went with the city. Then the land's own: the quarries by the stone-built
  towns, the mines on the plan's coal and ore, the shrine at the holy place, the necropolises, the plains' lines, the henges, the fortified
  line, the antenna field, the wrecks, a colossus, the terraformed basins, the far future's, the wind farms.
* **A dam fits its valley.** The first dams never found a valley: a fixed height asked of walls within a few hundred metres, and the river
  towns the history builds them for stand in broad plains. Now a dam is sought by its town and then up its river (the plan's coarse rivers,
  into the hills), over a grid of 300 m with the flow gathered down its steepest neighbours; every valley cell's cross-section is read on
  the grid (free) out to the age's reach, the best few again every 25 m; the crest is the lower shoulder (the age's height at most); the
  reservoir is filled from behind the wall over a grid of 50 m to the highest level that neither runs round the wall nor reaches the grid's
  edge, the crest a few metres over it. These worlds' valleys are broad: the dams come out long and low (8 to 26 m over 0.9 to 1.4 km).
* **The drainage's basins kept out.** The view draws the ground with the rivers and the lakes (the drainage fills closed basins with a lake
  or a playa's flat, and melts a valley toward its channel); the dam's search reads the planet function without them (a drainage tile is
  200 to 500 ms). The first desert dam stood 25 m under the drawn ground at its middle: a playa had filled its valley. Checking each dam
  against the drawn ground built a tile per try (2.3 s a world); a priority flood on the search's own grid finds the closed basins the
  drainage fills instead, and a dam's floor in one is passed over (44 to 350 ms a world).
* **Heights over the reference level.** A dam's wall spans its valley: a form standing on its foot's ground (the drawing raises a big form
  to the highest ground under it) would bury it or lift it. `Building::base` and `Form::absolute` stand a dam's wall, an aqueduct's arches
  and the far future's lattice at their own heights; the drawing and the colliders read them so.
* **A reservoir in the ground.** The water is an edit of the site's vertices at all the rings (`GE_WATER`: its level over the cells the
  fill reached, the plane under its banks a cell further, the shore cut at the level as B-322's): no new water renderer. A dam whose wall is
  half gone (its material's decay as its generator reads it, `damBreached`) holds none: its bed is silt; a dead desert's reservoirs are old
  pale beds.
* **The ground's edits** (`GroundEdit`, `SurfaceSite::addWorkEdits`/`applyEdits`/`editCover`): cuts and pits lower the ground (a fresh
  pale rock), ditches and craters silt up with the years (`editLeft`), plains are made flat, lines are tones; each on the rings that can show
  it (a crater under its radius, a trench on the 4 m ring), the great ones (over 600 m) found by their bounds, the rest by 64 m cells.
  Lines on a plain are cleared bands of ten metres and twice the albedo: three or four metres vanished in the 16 m cells from the air (C-09's
  and T-02's lesson again).
* **Heard as what they are.** The scanner's WORKS mode names the kind (`A DAM`, `LINES ON THE GROUND`, `SOMETHING MADE` for the far
  future's): a description, as `A TOWN` is, never a name. `LM_WORK` took 8 and the place mark moved to 9 in memory; the guide's file keeps
  R-408's numbers (8 a place, 9 a work), so older guides read as they were.
* **What moved.** No generation change, no ruin cell moved; the regress is unchanged (its scenes are not on peoples' worlds). Every
  civilised world gains its works (and the scenes near them change); the scanner on a civilised world steps RUINS, WORKS, then the land's.
  * `galaxy/works.*` (new): `WorkKind` (21), `GroundEdit`, `Work`, `WorldWorks`, `worldWorksOf`/`IfReady`/`buildWorldWorks`/`ClearCache`,
    `worksNear`, `workSpec`, `workLine`, `editLeft`, `damBreached`; the rules (`WB`: `quarries`, `mines`, `dams`/`damNear`, `aqueducts`,
    `observatories`, `shrine`, `necropolises`, `geoglyphs`, `henges`, `fortLine`, `battlefields`, `launchSites`, `antennas`, `wrecks`,
    `colossi`, `terraforms`, `shelter`, `strange`, `windfarms`, `lighthouses`).
  * `galaxy/buildings.*`: twenty-two generators of the works' kinds; the aqueduct at heights over the reference level; their ages and places.
    `galaxy/ruins.*`: the kinds, `Building::base`, `RuinSpec::work`, the landing's clearance of a work. `galaxy/forms.h`: `Form::absolute`.
    `galaxy/graves.*`: a work's plots' graves, `graveElements(..., stonesOnly)`. `galaxy/history.h`: `HistoryWork`. `galaxy/landmarks.*`:
    `LM_WORK`, `worksHeardNear`. `galaxy/plan.cpp`: the works on the approach's thread.
  * `surface/site.*`: `SiteEdit`, `addWorkEdits`, `applyEdits`, `editCover`, `clearEdits`. `surface/surface_view.*`: `workNear`,
    `workCell`, `worksHere`, the works in `forNearbyRuins` and their edits in `ensureTownMarks`, the absolute forms' colliders.
    `surface/forms_draw.cpp`: the absolute forms. `surface/flora.cpp`: no tree on a work's ground or across its line.
  * `game/marks.cpp`, `game/hud.cpp`: the WORKS mode. `game/guide.*`: the marks' kinds in the file. `game/game.cpp`: `testWalkToWork`.
  * The harness: `works`, `works ages`, `works gallery`, `works map`, `works view`, `works check`; six unit checks.
  * KI-379 new; KI-377 narrowed (the works read the history's battles, works and last place).

## T-04 (2026-10-09): where the records lie

The last of the towns and the works (R-411, the expansion note's section 7): how many records a people left, when each was made and in
what, and where each lies, fixed before H-04 writes them. `docs/reference/12-civilisations.md` (where the records lie) has the numbers.

* **A people's own count, C-02's fifty its first.** 80 to 200 records a people (`recordCountOf`: a count by the age, moved by its numbers at
  its height against its age's middling numbers, and a draw of its own): the age worlds hold 86, 101, 135, 143, 175 and 178. The fifty C-02
  made are the first fifty and keep their texts, their music, their charts and their last recording, so a shard found before is the same
  shard; past them music and charts come at the people's own shares of the fifty (`recordIsMusic`, `recordChartIndex`: a shuffle of their
  own), and the charts past the fifty are marked too (`chartMarksOf`'s count, C-10's marks first). H-05 sorts the three categories by age.
* **Dated in the history, the order C-02's.** A record's date is its share of the people's years from the founding to the end, weighed by
  what each age wrote a year and what the years left (`recordYear`); the share is C-02's first draw of the shard, so the records keep
  C-02's order and the last recording stays the latest (the others end a year before it). The shards' years, the `{year}` of the texts, the
  decoder's dates and timeline and the graves' years read the history's count (`Lore::spanYears` is the end's year less one, `Lore::
  endYear`): A-01 had left C-02's span of 150 to 600 years for H-04, but a record's medium follows its date, and a disc dated in the lore's
  year 200 of a people whose year 200 was medieval would be wrong. The texts are C-02's until H-04.
* **The medium by the year's age** (`RecordMedium`, fifteen; `mediaOf` a weight by the age, the category and how far into its era): what
  the expansion note listed, carved bone to something that hums; an age's early and late years differ (tablets before codices, reels
  before drives).
* **Every record somewhere: a home.** Laid as C-03 laid them (each settlement drawing a few from the whole catalogue), the futuristic test
  world's records were a quarter nowhere and the far future's nine in ten nowhere: few places, no countryside. Each record has a home now
  (`recordHomesOf`): a place of the plan standing in its year, drawn by its size, which always holds it (its archive, else its rooms); the
  copies come on top, as many as before. Every record of the six age worlds lies somewhere; the far future's two structures hold half each
  in rings. H-04 can say a record was written where it lies.
* **Archives in the middle of the room.** An archive is a row of records across the middle of the room its age kept writings in (a
  temple, a palace's hall, a town hall, a tower's ground floor, a spire's gate hall), the place's own first: rows along the back wall ran
  into the corners of polygonal temples and their ambulatories' columns. The capital's holds 8 to 14, a city's 5 to 8, a third of the towns'
  3 to 5; the far future's structures ring theirs round their feet.
* **The last recording where the history ended.** At the last place (`History::lastPlace`), in the building its age would keep last words in
  (the capital's mast, the temple, the town hall), and in the last shelter T-03 laid by it: two copies, both meaning it, so a world whose
  shelter is a long walk off still gives it at its last place; nowhere else (the general draws leave it out).
* **A place keeps nothing written after it was left.** A town the history ended early holds records up to its end; the old time's stones
  (left before year 1) hold none.
* **A painted wall on a wall.** A painting is on the face of the first wall box a line from its spot away from the doorway meets (the
  wall's own geometry: a march through the colliders' discs hit a polygonal hut's corners, half the painting inside the next wall), centred
  on the face within its length and leaning with it (a hut's walls lean); recorded, it stays painted. Where no wall stands, a painted slab.
* **The objects are forms.** Each object is a few small forms in the records' banks (29 pale, 30 dark, 31 ochre; clay the cut wood's 17,
  metal the chrome's 7; the free banks the surface left) or its people's slots (a carved stone in its stone, a sleeve in its accent, a
  crystal and a sphere in its light), drawn with the settlements' forms within 40 m, no collider; C-03's glint stays the find. The HUD
  names the object a row over the sun's line (`A CODEX - E`; C-03's line ran into the elevation).
* **The found shards across the change: kept by their index.** A people's records are at people x 1000 + index in the world's indices (the
  guide's keys): two hundred no longer fit C-13's 50 + index. A guide without the file's `records 2` line is moved when loaded (`Guide::load`:
  its second people's shards, decoded, heard, lent and pieces' names), so the counts and the language are kept and the contents are the
  records' (the first fifty's texts, music and charts C-02's as before); the game says it once (`RECORDS REGENERATED - N SHARDS KEPT,
  CONTENTS NEW`), logs it and saves the guide. The marker, not the generation: guides saved between A-01 and now carry generation 13 with
  C-13's keys.
* **What moved.** No generation change; the regress unchanged. Every people's shards lie elsewhere (more of them, archives, the last at its
  place), their years are the history's, the second people's keys moved.
  * `galaxy/records.*` (new): `RecordMedium`, `RecordCategory`, `recordCountOf`, `shardsOfWorld`, `recordsOf`/`RecordCatalogue`/
    `RecordEntry`, `recordYear`, `recordIsMusic`, `recordChartIndex`, `recordChartCount`, `recordMediumFits`, `recordHomesOf`, `ShardSite`
    (moved from shards.h, with the medium, the archive, the last, the wall), `recordSitesOf` (C-03's `shardSitesOf` replaced), `recordForms`;
    `SHARD_STRIDE`, `shardPeopleOf`/`shardLocalOf`/`shardWorldIndex` moved here at the new stride.
  * `galaxy/shards.*`: `Lore::records`/`endYear`, the span the history's, `shardOf` dated by the catalogue, music and charts past the fifty.
    `galaxy/charts.*`: `chartMarksOf`'s count, `chartOf` past the fifty. `galaxy/buildings.*`: the works' pieces' feet as rooms
    (`buildingRoom`), their places' names. `galaxy/forms.h`: the records' parts.
  * `surface/surface_view.*`: `RuinCell::recordForms`/`recordFirst`, `recordObjects`, `gatherRecords`, `drawShards` the glint alone, the
    works' cells' records, `NearShard`'s medium and more, banks 29 to 31. `surface/forms_draw.cpp`: the records' parts' banks.
  * `game/guide.*`: `records 2`, the move, `migratedShards`. `game/game.cpp`: the status on the first load, `testWalkToRecord`.
    `game/guide_screens.cpp`: the taking's words. `game/hud.cpp`: the prompt. `game/shards_screen.cpp`: the people's counts.
    `game/radar.cpp`: the people of an index.
  * The harness: `records`, `records ages`, `records scenes`, `records check`; five unit checks; the C-03, C-10, C-13 and C-14 checks read
    the people's counts.
  * KI-380 new; KI-377 narrowed (the records' homes and dates read the history).

## H-03 (2026-10-09): the grammar engine

The engine H-01 chose (`docs/ideas/RESEARCH-records.md`, sections 4 and 5), built, and C-02's kinds rebuilt on it from the same inputs
before H-04 writes new content. `docs/reference/12-civilisations.md` (the grammar engine) documents the format and has the numbers.

* **The format is plain text, compiled in as raw strings.** `src/galaxy/grammar/*.vg`, each file wrapped once in `R"VG(` and `)VG"` and
  `#include`d where a string goes: no build step, no generated header, Make's and CMake's dependency scans see the include. The loader skips
  the wrapper, so a file from disk parses as its compiled-in copy with the same line numbers; the harness reads the tree's files (an edited
  rule reprinted without a rebuild) and the unit checks the tree's files are the text compiled in. Files stay under 16 KB (MSVC's literal).
* **Texts parsed once into nodes, linked against the model's schema.** A reference to a rule, a fact, an entity, a field or a directive
  that does not exist, a guard's unknown key, a rule no `@root` reaches and "a" before a plural list are errors at load with their file and
  line (`vgLink`). The model names what it gives (`VgSchema`); the shard model's facts are indexed, not looked up by string.
* **The realiser owns the English.** Entities, not pronoun slots: a person's first mention in full, a pronoun only where clear, its later
  form else ("the child", "my mother", "the woman"); the case by the mark (`:obj`, `:poss`, `:refl`). Clear is stricter than H-01's rule (the
  last of its gender within a sentence): no person may have been mentioned since whose gender the reader cannot know, because the peoples'
  names do not say it ("Cekrast says they follow him", right by gender when Cekrast is a girl, reads as Cekrast); a kin word, a stated
  gender or a pronoun used shows one. Verbs by tense, person and number (140 irregular verbs), numbers with their nouns, "a" or "an" by
  sound, the seams. C-02's `tidy` and its `{he}`/`{him}`/`{his}` slots are gone.
* **Attributes in reading order.** `{=.x}` sets a record-wide attribute a later part's guard reads (the prototype passed it up to every
  caller, which is the same thing in reading order); `{rule>.x}` keeps a word with its number for a later verb or pronoun ("the nets are
  mine"); a `keep` rule says one thing all through a record (one crop, one herd animal); a person a text brings in (`{+$x:?|the stranger}`)
  is the text's own.
* **A people's texts are one catalogue.** Built in index order with the engine's memory (an alternative's weight over 1 + 1.5 times its
  uses by the people's earlier records); `shardTextsOf` caches the last eight peoples and `shardOf` reads it, so a shard alone is the same
  text as in its catalogue. A `steady` rule keeps its weights across the catalogue: the kinds keep C-02's shares (spread evenly, a desert
  drew "the dust" four times in its first fifty and a one-template elegy as often as the elegy); only the templates within a kind are
  spread. A text the same as an earlier one of the people is drawn again (up to eight times; the memory untouched by the draws thrown
  away), so a world never repeats a text whole.
* **C-02's draws kept in their order.** The herd, the kin and the child's name are drawn from the shard's stream as before the tone, so every
  shard's tone is C-02's and C-04's pieces are the same pieces; the text is drawn from its own seed. The child has its own pronoun (C-02's
  shared the kin's); the kin, the child and the founder are the model's persons, the beloved, the stranger, the woman from the other town
  and the old one who kept the count the texts' own.
* **The lexicon by age, as written.** `@lexicon <age> words` lists an age's words in the forms the texts would use (not stems), so "bore"
  does not match the industrial "bores"; the check reads a record's year's age (T-04's). C-02's texts needed one guard: iron waits for the
  medieval age.
* **The kinds' names** are their rules' (`kind.the-other-people.gone` "the other people"), the order of the root's alternatives their index.
* **What moved.** No generation change; the regress unchanged. Every text shard's text is new (the same kinds and templates, other draws:
  the engine's stream is the text's own), the tones, music, charts and years are as they were; the captions of the charts are written by
  the engine (`shardChartRecord`).
  * `galaxy/grammar.*` (new): `VgGrammar`, `vgParse`, `vgLink`, `VgSchema`, `VgModel`, `VgEntity`, `VgRecord`, `vgRecord`, `VgMemory`,
    `VgRun`, `vgCheck`, `vgMeasure`, `vgFrame`, `vgSentences`, `vgTopFrames`, `vgInflect`, `vgPlural`, `vgNumberWords`, `vgOrdinal`.
  * `galaxy/grammar/` (new): `shards.vg`, `shards_end.vg`, `shards_world.vg`, `lexicon.vg`.
  * `galaxy/shards.*`: C-02's `Grammar`, `tidy`, `kinds()` and `buildGrammar` replaced by the shard model (`schemaOf`, `worldFacts`,
    `recordFacts`, `shardDraw`), `shardGrammar`, `shardSchema`, `shardGrammarFromDisk`, `shardGrammarSource`, `shardTextsOf`/`ShardTexts`,
    `shardChartRecord`; `shardKindCount`/`shardKindName` from the grammar.
  * The harness: `grammar` (the tree's files, every people's texts of the test worlds, the age worlds and the forty nearest home through
    every check, the measures, the rule use, the cost; `shots/tests/grammar_texts.txt`), `grammar check`; four unit checks.
  * KI-381 new (what C-02's content still does and what H-04 takes); KI-368 narrowed.

## H-04 (2026-10-10): the records

The records written from the history on H-03's engine: the genres of each age, the collapse told from many sides, the language by
roots, the decoder's pages. `docs/reference/12-civilisations.md` ("The records (H-04)") has the genres, the model and the numbers.

* **The genre is the grammar's choice.** The root (`record (steady)` in `records.vg`) picks a genre by the age of the record's year and
  T-04's medium, guarded by what the history offers at its date (a pair of a family alive, someone to write, a death, an old one who
  dies soon, an event, a battle, a building, an invention, a railway); its weights hold across a people's records, so what a people
  wrote most stays so. 63 genres in 81 alternatives; a genre's name is its rule's (`genre.letter.far` "letter").
* **The model reads the history; the directives plan the record.** Facts (the world, the people, the record's year: its government,
  era, ruler and reign, harvest, war, sickness, the collapse's stage, the inventions and institutions at the year, the genome's traits),
  entities (the writer, the reader, the subject, the event, the ruler, the home, the seat, kin, bound lazily), paths and fields.
  `@cast` binds a record's persons (a letter's two of a family, weighed by kinship and by living apart, the reader at home five times
  as likely), `@news` tells the most salient event not told, `@entries` a year's events in their seasons' order, `@years` a chronicle's
  years with the gaps between, `@death`, `@deed`, `@life` and `@event` the rest. Salience: the event's weight, its nearness, its place,
  the writer's stake; an event told elsewhere counts half, a great one told in another register 1.3 (the annal, the letter and the song
  of one event); nothing twice in a record.
* **The register kept by the collapse's stage.** Most records are the ordinary life; the warning, the denial, the orders, the inventory,
  the emergency and the whoever-comes come only in the collapse's years (`stage`), the last recording where the history ended.
* **Every name the history's at the record's year.** Places founded by then, persons born by then (by the whole name or the given
  name), the lore's; the unit checks every name written (`recordNameKnown`). A ruler's title is capitalised word by word ("High
  Priestess"); a letter greets by the given name (the second word is a surname unless it is a regnal numeral); places and events are
  never "it". Names are marked through the realiser (`vgName`) so the checks and the decoder know them.
* **Anachronism by invention, not only by age.** `@lexicon inv.railway train trains` (a word whose fact is unset at the year fails);
  "broadcast" waits for the radio, "steam" for the steam engine, trains for the railway; a prehistoric text says "the ship", not "the
  computer"; the history's coal-out signs say "the engines stopped" and "the last works shut" (its text, not its events).
* **Repetition: composition within the sentence.** A slot a people draws a hundred times needs many frames, so the parts compose inside
  their sentences (two or three slots a line) and a tail goes with the part it fits (the hunt's beasts one or many, the instructions'
  things with their reasons, the machine's notes with their reactions, the mission's days in order); the catalogue dryness rose from
  1.5 to 4; a genre's heading and closing are `formula` rules, left out of the measures. Over the 48 peoples: 10.6% of the longer
  sentences on a repeated frame at a people's first 50 texts, 34.6% at 100, 15.8% over all, distinct 2/3-grams 0.74/0.89; C-02's texts of
  the same peoples 20.4%, 36.4%, 27.6%, 0.68/0.83 (the targets were 15%, 25% at 100: met at 50, not at 100, KI-381).
* **An object as its medium.** A bone's hunt is cut, not painted; a stone's weathers, a wall's paint flakes; a diary has no "you"; the
  night sky has no sun (`skynight`).
* **Texts built on a thread.** A people's records with its history take a third of a second; `shardTextsIfReady` returns them when built
  and starts a thread when not (`shardTextsPrefetch`, joined at exit); the radar asks every quarter second, the decoder builds the world
  under its cursor. `shardTextsOf` still blocks for the harness.
* **The language by roots** (C-06 re-tuned): the computer learns a root and reads its forms (`rootOf`, the tongue's endings `endS`,
  `endPast`, `endIng`); the share is 30% with one shard and all at the people's full count (`languageFull`: 12% of its records, 10 to 24;
  C-06's was all at ten, with three times fewer words); the names read from the start are every name the records wrote.
* **Long texts in pages; the voice reads the opening.** The decoder lays a text out from its final reading, a page starting at a paragraph
  or a sentence; the voice speaks the page shown (the opening first), `UP`/`DOWN` turn pages, `ENTER` speaks the page; the radar's voice
  reads a record's opening (sixty words at most). A word followed by a paragraph gets a sentence's pause.
* **What moved.** No generation change (13); the regress unchanged. Every text shard's text is new (the genres replace C-02's kinds);
  C-02's stream draws are kept before the text, so the tones, the music and the star charts are as they were; the charts' captions read
  the record model.
  * `galaxy/genres.*` (new): `RecordModel`, `recordSchema`, `recordWorldFacts`, `writeRecordTexts`, `recordGenreNames`, `recordNameKnown`.
  * `galaxy/grammar/` (28 files): `records.vg`, `world.vg`, `people.vg`, the registers (`annal.vg`, `annal2.vg`, `letter_events.vg`,
    `letter_events2.vg`, `news.vg`, `song.vg`, `log.vg`), `letters.vg`, the ages' genres (`prehistoric.vg` to `farfuture.vg`, two or three
    files an age), `ending.vg`, `ending2.vg`, `charts.vg`, `lexicon.vg`; C-02's `shards.vg`, `shards_end.vg`, `shards_world.vg` gone.
  * `galaxy/grammar.*`: `formula`, `@lexicon` by fact, name marks (`vgName`), `VgModel::lazy`, `VgEntity::named`, `VgRun::has`, the
    measures without formulas and with the distinct sentences, `VG_DRYNESS` 4, the anachronism diagnostic.
  * `galaxy/shards.*`: the texts from `writeRecordTexts`, the async build, `ShardText` (genre, age, year, told, writer, reader, subject),
    `Shard::names`, the language by roots (`rootOf`, `languageShare(held, records)`, `languageFull`), `shardOpening`.
  * `galaxy/history.*`: `historyWorkName`, `historySignPattern` and `historyLeftWhy` exported; the coal-out signs reworded.
  * `game/shards_screen.cpp` (the pages, `speakShardPage`, the prefetch), `game/radar.cpp` (the opening, the build on the thread),
    `galaxy/voice.cpp` (a paragraph's pause).
  * The harness: `grammar ages`, `grammar compare <file>`, the genres' table, one event from several sides, the names checked; the unit's
    records checks rewritten (the count stays 263); the flow's `shards_long` and `shards_long_page2`.
  * KI-381 narrowed (H-04 took the content and the directives); KI-368 closed by H-04; KI-382 new (what the records leave).

## H-04, the second pass (2026-10-10): each people its own

The user read H-04's records and asked for more: the repetition at a people's hundredth text down to 5% (H-04 left 34.6%, H-01's
target was 25%), the words that came back on every world ("jars, wells, chiefs and so on") gone, names that differ from people to
people ("everything sounds like it was one civilization"), and texts that feel unique to each civilisation, the genres kept. Done the
same day; `docs/reference/12-civilisations.md` ("Each people its own") has the parts and the numbers.

* **Measured first.** Two measures beside H-04's: the share of a people's longer sentences (its first hundred texts) on a frame another
  people also wrote (65.9% over the 48 peoples: most of what any civilisation wrote, another wrote word for word), and how well a
  held-out name tells its people from its letters (C-02's one generator in three styles: 53% over 48 peoples). Within one genre over all
  the peoples the frames were 50-95% shared (the letter 56%, the newspaper 71%, the picture 94%). `grammar late N` adds the nearest late
  peoples, so the measure at a hundred texts reads seventeen peoples, not six.
* **A sentence said once (the engine).** A people's records keep the frame of every sentence they wrote (names, persons and numbers
  masked); a rule that writes whole sentences and holds no directive is drawn again, up to six times, while a sentence it finished
  repeats one (its draws, their dryness and their marks undone; a record's budget 160; a repeat an inner rule let stand does not make
  the rules round it draw again). Each attempt opens a token of its own, so the check sees where its sentence began (writer F found
  that a sentence merged into the previous token was never checked; writer A that the first sentence after a formula was skipped).
  On H-04's grammar alone it took the three figures from 10.6/34.6/15.8% to 4.8/21.6/8.1%; its real use is that a rule with N frames
  says N things before it repeats one, instead of repeating at the birthday bound.
* **A word worn out.** A distinctive word (in 24 alternatives or fewer) weighs down every alternative it is in by the people's uses of
  it, so "jars" does not come back from five rules; the culture's pools put each people's own vessels, waters and titles where the
  fixed words were.
* **A people's own few, not a people's own weights.** `people=K` keeps K of a rule's alternatives for a people (its seed ranks those
  whose guards hold), `share=F` a share. A dialect (every alternative weighed by the people's seed, sigma 0.9) was tried and set aside:
  with the dryness a people uses every alternative in the end, so weights changed the order and not the set; the sharing between
  peoples stayed (65.4% against 66.0%) and the repetition within one rose (4.8% to 7.3% at fifty). It is kept on the genres' root only
  (sigma 0.45): one people writes more prayers, another more letters. N2 found the same of `people=K` on a sentence: what lowered the
  sharing was more people-specific words in each sentence and longer lists.
* **Their own words.** `{~key}` writes the people's own word for a thing of theirs with none of ours (their coin, their dish, their
  dance), from their tongue, kept by the decoder (`VgRecord::theirs`).
* **A tongue for every people** (`galaxy/tongue.*`). C-02's names came from one generator in three styles keyed by the galaxy's region;
  a people's tongue is now drawn from its seed after Martin O'Leary's naming languages: twenty-four consonant sets, fourteen vowel sets,
  one to three syllable shapes, the clusters a tongue allows, its spelling, a doubling in some, the morphemes of thirty-three meanings
  before or after the root, joined or hyphened, thirty to fifty name roots that come back, a given name's ending by sex where the
  tongue marks it, a family affix. The lore's names, the history's persons, families and places (by rank), the graves', the decoder's
  words, their own words and the voice are the tongue's; a name that reads as a rude or famous word of ours, or stutters a syllable
  thrice, is drawn again. A held-out name tells its people 74% of the time over 60 peoples (C-02's 51%), 83% over 24 (the unit's).
  C-02's draw of the style stays in the lore (`Lore::style`): the shards' stream reads it, so every shard's tone and every piece of
  music is as it was; the voice leans with the tongue's own hardness now.
* **The ruins in the texts.** The genome read through the record's age (the house's plan, the roofs, the walls, the huts, the ornament,
  the motif, the tops, the accent's colour, the land, the streets, the squares, the edge, the links, the lights, the towers) are the
  model's facts, so a text says "under the domed roofs" where the explorer finds domes. A place has a trade, a landmark and quarters,
  a ruler an epithet, a person a trait; a government's ruler the people's own title (a headman, an archon, an oracle; a word of their
  tongue for nearly half the monarchs, kept as a name); wars, sicknesses and eras are named from larger tables by the people's seed.
* **The everyday in the history.** Fourteen public kinds (fair, fire, storm, pest, wonder, games, law, trial, embassy, accident, price,
  the herds, a show, the ruler's visit) from a stream of their own a year, and seven private kinds of the families followed (an illness
  got over, a hurt, an apprenticeship, the house, a quarrel, a journey, a betrothal) from each person's own stream after the years are
  drawn: neither harms anyone or makes a person, so H-02's history stands as it was (every war, death and ruler where it was). Low in
  salience, they are what quiet years are told with (75,827 of 184,647 events on the test worlds' peoples). A reader is never told
  their own news; `@digest` tells several items in the order they happened (KI-382's defeat above its war).
* **Seven writers.** The grammar was rewritten in parallel by seven writers (prehistoric; medieval; letters and diaries; industrial and
  the news register; modern and the log register; futuristic and far future; annals, songs and the collapse), each owning its files, a
  rule prefix and the registers of the new kinds, on a copy of the tree with a copy of the binary (`grammar dir D out F`), merged by file;
  the harness reads every `.vg` in the directory and the unit holds the tree's list to the build's. 94 files, 2,864 rules, 16,808
  alternatives (H-04: 28, 914, 3,640), every file under 16 KB.
* **The numbers.** Over the 48 peoples H-04 measured: 0.2% of the longer sentences on a repeated frame at a people's first 50 texts
  (99.9% written once), 0.9% at 100, 0.6% over all (H-04: 10.6%, 34.6%, 15.8%); distinct 2/3-grams 0.79/0.94 (0.74/0.89); 17.1% of a
  people's sentences on a frame another people wrote (65.9%); 287,211 words, 83 a text (216,497, 62). With the twenty nearest late peoples
  (68 peoples, 17 at a hundred texts): 0.3%, 1.6%, 0.9%, sharing 22.2%. Every check at zero, every one of 14,868 names the history's.
  A text 67 us (501 at most), a people's texts with its history 0.35 s.
* **What moved.** The generation stays 13 (1.3.0 unreleased: the tongues, the everyday and the names join A-01's change; no save of
  13 has a people's name in it, the guide keeps the explorer's); the regress unchanged (22 hashes). `galaxy/tongue.*` (new: `Tongue`
  moved there from `shards.h`, `tongueOfSeed`, `tongueName`, `tongueTerm`, `tongueReadable`); `galaxy/grammar.*` (the frame memory,
  the fatigue, `people=K`, `share=F`, `{~key}`, the root's lean, the kept rule's tokens, the article-pair check, the agreement check's
  prepositions); `galaxy/genres.*` (the culture facts, `what`, `verdict`, `epithet`, `trait`, `ptrade`, `landmark`, `quarter`, `@digest`,
  `@years` reaching further in a sparse age, the native titles, the private kinds' salience); `galaxy/history.*` (the everyday and the
  families' kinds, `historyEverydayKey`, `historyRulerTitle`, `historyTitleNative`, `historyFamilyName`, the names of wars, sicknesses and
  eras); `galaxy/shards.*` (the lore's names, `personName(Lore, ...)`, the loader reading the directory, the 66 new files compiled in);
  `galaxy/graves.cpp`, `galaxy/voice.cpp`; the harness's `tongues`, `grammar late N`, `out F`, the sharing measures, `VESPERIS_TOPFRAMES`;
  three unit checks more (266). KI-381 and KI-382 narrowed, KI-383 new.

## H-05 (2026-10-10): more shards in three categories, by age

* **The categories by the record's age.** C-02's fifty keep C-04's and C-10's categories (a found shard keeps what it was). Past them a
  record is music or a star chart by the age of its year, not at the fifty's shares (KI-380): an age's share (music 36% prehistoric, 22%
  medieval, 25% industrial, 32% modern, 24% futuristic, 26% far future; charts 9, 8, 7, 6, 12, 15%) leaned by the people's own fifty (its
  pieces against fifteen, its charts against four), the records of each age taken by a hash of their own so the count is exact. The
  record's year and age are the people's generator's (`peopleAgeOf(pg, 0)`: the eras do not depend on the end's years), so the
  generator-only callers (`recordIsMusic`, `recordChartIndex`, the lore's chart count) agree with the catalogue. `Shard::recordAge` and
  `Shard::medium` carry the record's age and medium to the music, the charts and the decoder.
* **The music: the age as an axis of C-04's tradition.** `traditionOf` draws C-04's core first and to the bit (each people keeps its
  scale, tuning, cycle, beat, timbres, drum, drone, second voice and ornament: "after two pieces the listener knows the tradition" holds
  across its ages), then from streams of its own what holds through the ages (a swing, a refrain, choruses in octaves, the singers'
  vowels from its tongue) and an ensemble per age (thirty timbre families by role, leaned by the people's own C-04 timbres). A piece is
  played on its record's age's ensemble in a form of forty by the tone and the age, in the age's texture, its length by the clock (15 to
  120 s, not by cycles). Chords are stacked from the people's own scale (the degree nearest a third, the nearest a fifth), so a people
  without a third plays what it has: alien by design. The player moved to `game/audio_music.cpp`: 28 voices of up to 32 partials on a
  sine table at a control rate of 32 samples, the kits' pieces of sines, resonators and noise, a limiter on the music's bus (stacked
  chords under 0.82). Every age's pieces peak under 0.78 with an rms of 0.061-0.152 (the speech's 0.13-0.15); at most 22 voices sound at
  once; the dearest piece (a modern song) costs 315 ms of a core a minute.
* **The charts: a way per age and medium.** `chartStyleOf(age, medium)` picks one of twelve ways (a cave's ceiling, a carved bone, a
  painted vault, an astrolabe's plate, a codex page, an engraved atlas, a photograph on glass, a survey on glass, printed or on a drive,
  a navigation chart, a chart of far places); each reads the world (the moon's count is the calendar's, the ecliptic the world's orbit,
  the tropics its tilt, the wanderers the other planets' apparent paths, the atlas's grid the spin axis, the survey's rings the voices
  heard). The figures stay one set over the people's sky (C-10's rule); their names are animals for a people whose records began in the
  oldest age, creatures, heroes and things for the others. A navigation chart is a map of the neighbourhood from above whose routes are
  the overlay's lines between real stars; the far future marks a real star thousands of light years off (`ChartMark::how` 3: toward the
  heart, a cluster, out of the disc or along it), so Enter can target it and the galaxy chart's jumps reach it. A chart drawn before its
  people's radio marks no voice (C-10's marks were the people's whatever the chart's year): near home 15 of 341 charts mark a people
  heard (33 before). The chart's page moved to `game/chart_draw.cpp`; 3.1-4.1 ms a frame at 2x.
* **The media.** A medium with a sound of its own (`recordMediumSounds`: a wax cylinder, a reel, a disc, a drive, a crystal, something
  that hums) is the recording, heard on the decoder through it (`AudioState::medium`, `game/audio_medium.cpp`: the band, the horn, the
  saturation, the wow and flutter through a delay line, the surface's hiss, rumble and crackle, a reel's dropouts, a drive's stutters,
  coarse moments and gaps, a crystal's second reading, the hum's drone); the content's level is kept within a tenth (measured in the
  speech's pauses: the cylinder's surface 0.012-0.016, the drive's 0.0005). The air keeps C-16's colours (the receiver has its own; a
  medium on the radio would double them). A record with no sound of its own is the ship's rendering: a text read in the people's voice
  made flatter (`voiceRead`: the melody a third, the pace even and slower, no tremor or second tone, little breath; the vowels' pitch
  spread 30% of the voice's), notation played dry. The decoder names the medium and who is heard under the date and draws the object at
  its right (`game/medium_draw.cpp`); a chart's page draws its object as the chart.
* **Two writers.** The music and the charts were built side by side by two writers, each on a copy of the tree owning its files (the
  music's: `music.*`, `audio_music.cpp`, the roll; the charts': `charts.*`, `charts.vg`, `chart_draw.cpp`, the caption's facts), merged
  three ways against the shared base (`git merge-file`), the lead keeping the media, the decoder's header, the harness's `decoder` mode
  and the docs. A merge must give the files it writes a fresh time: the first merged build kept the writers' older times, so the files
  including the changed headers were not rebuilt and the unit crashed in `StarChart`'s move (a layout of two definitions); a clean
  rebuild passed.
* **What moved.** The generation stays 13 (1.3.0 unreleased: the categories past the fifty, the pieces and the charts join A-01's
  change; the found shards keep their indices, as T-04 decided); the regress unchanged (22 hashes). `galaxy/records.*`
  (`recordAgeAt`, the categories by age, `recordChartRecords`, `recordMusicShare`/`recordChartShare`, `recordMediumSounds`),
  `galaxy/shards.*` (`Shard::recordAge`/`medium`, the chart caption's facts), `galaxy/music.*`, `galaxy/charts.*`, `galaxy/voice.*`
  (`voiceRead`), `galaxy/genres.cpp` (the chart facts in the schema), `src/galaxy/grammar/charts.vg`, `game/audio_music.cpp`,
  `game/audio_medium.cpp`, `game/chart_draw.cpp`, `game/medium_draw.cpp` (new), `game/audio.*`, `game/shards_screen.cpp`; the harness's
  `decoder [ages|counts|wav|check]`, `music ages [wav|frames]`, `music ... age=N tone=N`, `charts ages|check`; four unit checks more
  (270). KI-347, KI-380 and KI-382 narrowed; KI-384, KI-385 and KI-386 new.

## H-06 (2026-10-10): the decoder reads a history

* **Three pages, Tab.** A world's records on the decoder are three pages (the star map's G-05 key): the records (C-06's list with each
  row's kind as a glyph and Left/Right showing one kind; the year column without its word, the line one character short of the screen
  with three-figure counts otherwise), the timeline of the eras (C-12's grown: the records stacked by their years in their kinds'
  colours, the events learnt under the axis, the record under the cursor below) and the history. A record opened from the timeline or the
  history comes back to it; a record's own Left/Right keep to the kind the records page shows.
* **Only what the read records told.** The history page is assembled from the records read (`Guide::decoded`), never from the history
  itself: the events their texts told (`ShardText::told`: the directives' picks, and since H-06 the death an epitaph or an obituary is
  about and the end the last recording tells, kept apart from the salience so no text moved), a person by name only once a read record
  wrote it (else what they were at the event: a woman, a man, a child, a ruler; `historyEventLine` takes a namer and is the legends'
  line unchanged without one), the people's ways their drawn alternatives were guarded by (faith, rite, holy place, living, values, the
  dead, the cause of the end: `ShardText::ways`). Enter on an event opens the record that told it, so the legends are an index into the
  records. The unit checks the page against the read records' own lists (nothing an unread record told, nothing missing).
* **The eras placed by a record of them.** The grammar names an era in three rules of 94 files, so an era named by its name in a text would
  almost never be: an era is placed and named once a record written in it is read (the computer dates its records); its name is read
  through the language like the rest. On the timeline each era's width is its share of the people's records (an era of few or none a
  sliver: the medieval Ta-Zhozo's six thousand years of the old time would otherwise fill the axis), linear within; its name over the
  band, else under it, else (two rows taken) left out, never cut.
* **The age named by three records of it.** A-01's rule (the age shown, never named until the records tell it) is kept by the count:
  three records written in the people's last age read, or its last recording, name it (THE AGE THEY REACHED: INDUSTRIAL). One record would
  name it at the first reading of a late record; the end recording names it because it tells the end.
* **Read through the language.** The history page's lines, the eras' and the people's names are decoded as the records are
  (`decodeLine`): a word the records wrote by its root as the shards held read it, a name kept, a word the records never wrote read only
  once the whole tongue is (the ship learns their words from their records). The page is built when the world's pages are opened and
  again after every reading (`Game::ensureTold`), not each frame.
* **What moved.** No generation change (GEN 13 holds, 1.3.0 unreleased); the regress unchanged (22 hashes); the grammar's 3,564 texts H-05's
  byte for byte (the measure "one event from several sides" 75 events where it was 71: the epitaphs' deaths). New: `galaxy/legends.*`,
  `game/history_screen.cpp`; changed: `galaxy/history.*` (the namer), `galaxy/genres.cpp` (`toldLegend`, the ways), `galaxy/shards.*`
  (`ShardText::ways`, `decodeLine`), `game/shards_screen.cpp` (the pages, the kinds), `game/game.h`; the harness's `decoder pages`, the
  flow's `shards_eras` and `shards_history`, seven unit checks (277). KI-377 and KI-382 narrowed; KI-387 new.

## C-11 (2026-10-10): the ship's player and the instruments of the ages

* **One owner a sound, the player its own copy.** The synth reads one piece and one speech through pointers, and a pointer moved under it
  stops it. The decoder's playback stays the decoder's (C-04, C-05: off its screen it stops; its first readings resolve the words as they
  are spoken); the ship's player holds its own `ShardWorld` (`ShipPlayer::world`) and points the synth into it, so nothing the decoder opens
  moves it, and the decoder's stops touch only its own pointers. One sound at a time: while another owner plays (the decoder's record, the
  radar's receiver, the gramophone) the player lets go and is held; it takes up where it was through `AudioState::pieceFrom` (beats) and
  `speechFrom` (seconds), the notes begun before not struck again. A second music engine for two pieces at once was not built: two
  peoples' pieces together are noise, and the player's piece and an instrument can sound together (the instrument has voices of its own).
* **What the player lists.** The pieces held, the recordings held (a text made on something that sounds: H-05's media; a tablet or a page
  is read on the decoder, by the ship), and the recordings the radar caught (`Guide::heard`, their opening as it was on the air, through
  `MEDIUM_AIR`, the receiver's band): the plan's three. A piece's form and length come from its draws and the catalogue's age
  (`shardDrawOf`), so the page opens without the people's texts written; a world's records are written on a thread when an item of it
  plays (`buildShardWorld` without waiting: `waiting` until they are).
* **Where it plays, where it stops.** Everywhere on the ship, in every state but a landing's (the descent, the surface, the ascent) and the
  title: a landing stops it, as the plan asked; the menu and the maps let it play. It plays the list on from the item chosen and stops at the
  list's end; Enter, Space, Backspace on its page, no key in flight (the HUD line says what plays; the decoder is where it is changed). The
  hum and the drive lower under it (0.3 and 0.55, from 0.6 and 0.9).
* **Instruments are the ensembles' voices.** A found instrument is one of its people's ensemble voices of an age (H-05), so it sounds as
  their pieces do on their scale, and every people's are its own: the kinds by the ensemble (a lute, a lyre or a harp by the people's coin
  among the plucked strings), the timbre the voice's own, the look from its numbers and the scale. What a hand cannot carry stays (the
  singers, the drones, the church organs, the pianos, a band's drum kit); the kits add the drums a prehistoric, medieval and industrial
  people carried and a modern drum machine (pads); the industrial age adds the gramophone, which plays a disc (the plan's example) rather
  than notes: a piece of the age through a cylinder's horn. Names by what it is, never by the age (A-01).
* **Placed beside the music, on a stream of their own.** An instrument lies beside a third of the music records, by the record, so every copy
  of that record has it and taking it once takes every copy (C-03's rule, the guide's key the instrument's: `<body>/I<id>`); it is placed in
  `recordSitesOf` after the records, on its own random stream, so the records lie where they did (the unit compares). Which of the age's
  instruments is resolved where the people's tradition is (the view, `instrumentIdOf`), and a settlement shows one of each. `E` takes the
  nearer of a record and an instrument.
* **Played from a case, by degrees.** The case is a cabin station like the decoder's (`facing` 10, the devices page, lime in bank 28);
  ten keys play ten degrees from the register's first note, past the scale into the next period, so a five-note scale spans two periods
  and a nine-note one a period and a step: the scale is what the keys give, the explorer cannot play a note the people did not have. The
  register is the instrument's own (a bass a period down, a flute, bells, a music box one up), Left and Right a period either way.
* **What moved.** No generation change (GEN 13 holds); the music's 25 WAVs byte-identical to H-06's (the voice set-up split into
  `voiceShape` for the instrument's notes); the records' listings identical. New: `galaxy/instruments.*`, `game/player.cpp`,
  `game/instruments_screen.cpp`, `game/instrument_draw.cpp`; changed: the synth (`audio.*`: the instrument's voices, the resume points,
  `MEDIUM_AIR`), `galaxy/records.*` (`InstrumentSite`, the placement), the view (the sites, objects, glints, the nearest), the cabin (the
  case, the devices row), the decoder (the player page, its stops), the HUD, the guide (`instrument` lines), the statistics. KI-347 closed;
  KI-388 new.

## C-17 (2026-10-10): the instruments' maker

* **Made by their peoples, not chosen by their age.** The user, after C-11: "civilizations are different, numerous, alien - yes, there are
  industrial or prehistoric societies but it doesn't mean everyone was carving bones or building gramophones". C-11's 49 kinds (by the age,
  with a coin among a family's few) are replaced by a constructor: what sounds is the voice's (the class, from the timbre's family), the plan
  is drawn among the class's by the people's leanings, the materials are what its world gave in its age by its loves, the style is its
  architecture genome's. Supersedes C-11's "Instruments are the ensembles' voices" for the kinds and the look; what holds is that an
  instrument is a voice of its people's ensembles and sounds as their pieces do.
* **The sound stays the voice's.** The make does not change the timbre: the music the decoder plays and the instrument the case plays are
  one sound, and the pieces are the same to the byte (the 25 music WAVs identical to C-11's). A make that shaped its own sound (a clay
  vessel's purer tone, a shell's breath) would split the instrument from its people's music; left for later if wanted (KI-389).
* **The genome, not a second set of variables.** The style reads A-04's thirty-five deep variables (plan, profile, roof for the bodies'
  outline; openings for holes; tops for heads; ornament, motif, accent, paint, symmetry, number, rhythm, proportion, light), so a people's
  instruments and buildings are one people's (a keyhole window, a keyhole sound hole), and the genome's measured distinctness carries over.
  The materials read the world (its flora's families, a hashed fauna, its seas, its cold, its volcanoes, its crystal) and three loves a
  people; the plans a log-normal leaning a people pushed by the genome. All on streams of their own: `traditionOf`'s draws stay.
* **One list of parts, two pictures.** The constructor writes parts (tubes, outlined bodies, discs, balls, boxes, rings, strings, marks);
  the ruins draw them as A-02's forms and the case draws them seen from above, so the picture is the object. An outlined body needed a new
  form kind (`FK_PLATE`: a superellipse with a taper, a waist, a lean, a dome, or a polygon), its outline one function both draw with; the
  forms' sheet and check carry it. An instrument's strings are cables a millimetre thick (`cableMesh`'s floor lowered from 1 cm: every
  building's cable is 3 cm or more) drawn in their part's bank (a line's bank was the people's slot's alone). The 3D object's colours are
  the nine banks it can take (the records' five, the people's four); the picture's are the materials' own, the people's where the object takes
  theirs.
* **Measured apart.** As A-04's genome: every age's and class's makes of 48 peoples near home in pairs, their pictures drawn small and
  compared, alike under 6 of 255 on the mean: under 1.5% (0.86% over 100 peoples), the same plan and body under 15%; prehistoric pipes of
  bone under half (13 of 100); the cylinders' machine for a quarter to nine tenths of the peoples (62 of 100). The closest pairs are drawn
  side by side for a look, and the simple plans the measure found too alike (single rings, orbs, cymbals, stars, rain tubes, pad rings,
  reed pipes, blades) were given more of their people's style until the closest pairs differed to the eye.
* **Names from the make, the decoder's too.** "A SHELL PIPE", "A NECKED GOURD", "BRONZE BOWLS", "A TALKING MACHINE": what it is made of and
  how, never the age; a timbre takes its make's name (a kit piece the plural), so a piece's instruments on the decoder are the ones the ruins
  hold. Seventeen letters at the most (the case's list): the material's word is left out where it would not fit, never a word cut.
* **What moved.** No generation change (GEN 13 holds); the music's WAVs and the records' listings identical; what an instrument is at an
  id moved (1.3.0 is unreleased). New: `galaxy/instrument_plans.cpp`, `FK_PLATE`; rewritten: `galaxy/instruments.*`,
  `game/instrument_draw.cpp`; changed: `galaxy/music.*` (`InstrumentMake`, `Timbre::make`, `Tradition::makes` and `machine`), the forms
  (the plate, the cable's floor), the forms' drawing (a line's bank), the view (an instrument's name, its small parts far off), the HUD, the
  guide's log, the case. KI-389 new.

## B-411 (2026-10-10): the glass is the hull's alone

* **The hull is glass, the room is not.** M2 drew a depolarised hull by darkening every face of the cabin, and the consoles, the
  back wall's sets, the case and the desk are faces like the walls: the room went to glass with the hull, and the light, which works
  through the hull's ramp and the faces' shades, had nothing left to light. The glass is the hull's now (the floor, the ceiling, the
  walls, the pillars and the mullions; the deck's rim seen from the roof); what stands in the room is drawn solid, lit as with a
  polarised hull. The screens, the cage and the capsule were solid already.
* **The glass catches the light.** `BLEND_DARKEN` with `RasterParams::sheen` (`core/raster.*`): the pixel behind darkened to 45%, or the
  glass's own shade in the hull's bank where that is the brighter, the light's level times 16 (`GLASS_SHEEN`). Lit, the glass shows the room's shape as a faint
  sheen with whatever outshines it showing through; unlit, it is clear: the light becomes the glass hull's own control (off to see out),
  as a lit room's windows at night. The brighter of the two keeps the outside's bank where it wins (`BLEND_MAX` would repaint a star
  in the hull's grey). A flag on the darkening, not a blend of its own: a fifth case in the blends' chain put a compare before every
  opaque pixel (the compiler tested it first), and in four alternating pairs of `bench check` against the commit the people's descent
  ran 3-10 ms slower each time (the forests and the capital a little); with the flag, two pairs came out even.
* **The state is said where it is missed.** `Y` lies beside `U`, its status lasts three seconds, and the glass is saved with the cabin,
  so a load comes back to it with nothing said: `U` with the hull glass says `Y POLARISES THE GLASS HULL`. The key stays (M2's, the
  original's depolarisation) and so does the saved state (KI-390).
* **What moved.** Nothing generated; the regress unchanged (the cabin is in no hashed frame); a polarised cabin draws as before (the
  flow's `cabin` and `roof` frames byte-identical to the commit's). New: `RasterParams::sheen`, `Game::testCabinLights`; the flow's `cabin_glass_lit`.

## B-412 (2026-10-10): one mark over the view, the explorer's choice

* **None over the view until one is chosen.** R-408 drew the eight nearest marks within 25 km over the view, each a diamond, its
  name and its distance, and nothing kept the names apart (the sector map's do keep apart): two marks in one line of sight wrote
  their names over each other (the user's TOWERS and FALLEN CITY over a ruin), and a world with seven marks within a few kilometres
  filled the view with them. A mark placed was over the view at once. The user: "by default there should be none and you can toggle
  it on or off - it should be optional and no need to show more than one". This supersedes R-408's "over the view, the eight nearest
  within 25 km". The maps keep every mark (they show what the explorer marked), and the rangefinder still names the mark the
  crosshair rests on.
* **Shown from the sector map, one at a time.** `Shift+Enter`, or a click with `Shift`, on a mark shows it in place of any other
  (the user: "select it by clicking in N map, maybe with shift"); on the shown one it hides it; on no mark it says `NO MARK UNDER THE
  CURSOR`. A plain Enter or click still marks and renames. The map boxes the shown mark in its colour and draws its name first, so
  neither the twelve names nor a collision drops it, and the panel says `SHIFT+ENTER SHOWS` or `HIDES` under the mark at the cursor
  (the scanner's lines a row lower). There is no key for it on the ground: the map is where marks are made, named and removed.
* **At any distance, and on the compass.** The shown mark is drawn wherever it stands on the world (the 25 km went with the eight:
  the explorer chose it), and a hollow diamond on the compass strip gives its bearing, an arrow at the strip's end beyond sixty
  degrees (the scanner's echo, filled and cyan, is drawn the same way by `drawCompassMark`). One mark over the view must be found
  when it is behind the explorer, or the choice looks lost.
* **Kept in the guide.** `Guide::shownMark`, an index in the marks written as a `shownmark` line after them (a guide without the line
  shows none; an older version reads past it). A removal before it moves it down and its own removal clears it. The guide holds the
  marks, so the choice goes with them across the save slots. A friend's export carries the line, and the import does not read it.
* **What moved.** Nothing generated; the regress unchanged (no mark is in a hashed frame). `game/marks.cpp` (`shownMarkHere`,
  `showMark`, `drawShownMark`, `drawCompassMark`; `drawMarksInView` gone), `game/hud.cpp` (the map's Shift+Enter, the box, the panel's
  line), `game/guide.*` (`shownMark`, `shownmark`), the help's `N` line (`MAP: ENTER MARKS, SHIFT+ENTER SHOWS`); the harness's
  `testShownMark`, `testMarkOnScreen`, `testPitch` and `testYaw`, two unit checks, the flow's `mark_shown`.

## B-413 (2026-10-10): the ground's sounds stay on the ground

* **Nothing of a world off it.** The surface sets its sounds in `Game::audio` every frame and the synth holds each level where it
  was last set; the space view silenced the wind, the rain and the lava only, so the breath after a sprint, the waves, the birds,
  the leaves, the hooves, the insects and the drone's pods went on in the capsule and aboard at their last level, through a load
  too, until another landing (the user: "that sound persists on stardrifter unless you quit the game and start it again").
  `AudioState::surfaceOff()` sets every level the surface sets to 0, and it stands beside those levels in the struct, so a sound
  added to the ground is added to it. The jetpack has no sound of its own (the wind rises with the height over the ground).
* **The capsule hears none either.** It runs every frame of the descent and of the climb, so the ground's sounds fade as the capsule
  lifts off (they went on, frozen at the last frame's levels, for the climb's seven seconds), as none was heard on the way down;
  the capsule has its drive and the ship's hum. It runs every frame of the space view (in place of the three) and at a load, which
  can come back to the probe's screen or a descent or a climb resumed (a ground loaded sets its own on its first frame).
* **On the ground they go on.** The menu, the maps and the guide opened on the ground keep the ground's sounds under them: the
  explorer is still there.
* **What moved.** `game/audio.h` (`surfaceOff`), `game/game_states.cpp` (`updateDescentAscent`), `game/autopilot.cpp`
  (`updateSpace`), `game/persistence.cpp` (`load`); one unit check. Nothing generated or drawn: the regress unchanged.

## B-414 (2026-10-10): one age a world

* **A world shows only the age its people reached.** The user found a disc in what read as a medieval village and a lattice mast over
  its ruins, and expected "prehistoric [to] have one set of shards, medieval another set, modern its set [...] and also building
  design"; asked whether a late people's world should keep its history in layers made legible or show its last age alone, they chose
  the last age. This supersedes A-01's "a people passed through the ages before its own: a modern people's world keeps its medieval towns
  as the old cores of its cities and its megaliths in the fields" (the expansion note's section 1), T-02's layers (a later people's old
  cores, a place left early in its older way, the old time's stones, the countryside's older villages and megaliths) and T-04's dating of
  the records over the eras. The history (H-02) keeps every era; what is on the ground is the last age's (`ageFromOf`: its first year).
* **What was made before it is gone.** A place the history left before that age began has nothing on the ground (its cell holds nothing:
  the view, the finders and the landing alike), and its works with it; a battle of an earlier age left no field; a work the history
  finished in an earlier era at a place that stood on stands in the last age's way (a modern people's industrial dam a modern dam). A place
  left early within the age keeps its weathering from when it was left.
* **Its records were made in that age's years**, the later the likelier (`recordYear`), so the media, the categories' shares, the
  genres, the music, the charts' ways and the instruments are that age's; a late people's earlier ages are told by its own **histories**
  (`genre.history`, `@past`: an earlier era since year 1, a run of its years with their facts and rulers, its events in the annal's
  register, between an opening and a closing in the last age's voice). The graves' dead died in the age's years while their place stood.
  A guide saved before keeps its found shards by their index, their contents new (`records 3`, said once).
* **The age speaks first on the everyday buildings, the people on its own.** A-04 kept a people's plan, roof and openings through every
  age, and a people of polygon plans, conical roofs and triangular windows built modern suburbs that read as a medieval village. An
  industrial or a modern people's villas, suburban houses and blocks stand on the age's plans (a rectangle, an L, a court), under the
  age's roofs (`ageRoof`: the people's where the age built so, else hipped slate in the industrial age and a flat roof in the modern) with
  the age's windows (sash, round-headed where the people's openings were arched, round, pointed or keyholed; wide panes and bands); the
  people's own plan, roof and openings stay on its temples, palaces and focal accent, and its colours, accents, tops and proportions on
  everything. The window slots are the dwellings' (0), the public and tall buildings' (1) and the people's own (2); the age before's slot
  (3) went with the old cores.
* **What moved.** No generation change (13 holds), as A-03, A-04, T-02 and T-04 changed the ground without one: no star, body or terrain
  moved, and a save's place is where it was. Every civilised world's towns, works, records and graves; the regress unchanged.
  * `galaxy/peoples.h` (`ageFromOf`), `galaxy/records.*` (`recordYear`), `galaxy/shards.*` (`Lore::ageFrom`), `galaxy/graves.*`.
  * `galaxy/ruins.*` (`placeHist`, `countrysideHist`, `TownHist::gone`, the old stones and the older age's settlement gone, `formOf`, the
    window slot's remap gone), `galaxy/towns.h`, `galaxy/works.*` (`placeAge`, `gone`, `ageFrom`; `eraAge` gone).
  * `galaxy/buildings.cpp` (`culturePalette`'s windows, `ageRoof`, the mill's doorway and chimney, the slots), `galaxy/genres.cpp`
    (`F_PAST`, `@past`), `galaxy/grammar/history.vg` (new), `records.vg` (the history in the late ages' roots), `medieval3.vg`
    (`sermon.turn` in the warning stage), `galaxy/shards.cpp` (the file), `game/guide.*` (`records 3`).
  * The harness: T-02's old-core check replaced by the one-age check (`towns check`), and a guide of `records 2` in the T-04 check.

## B-415 (2026-10-11): each age its own vocabulary

* **The age speaks first, the people second.** The user found an industrial town (BUBAS) that read as modern (white stucco, flat roofs,
  ribbon windows) and as medieval (colonnades of stone, white chimneys standing like columns), and asked that every age be clear, "but
  whole genome thing stays". A-04 let a people's genome speak as loudly in every age as in its own; B-414 gave the machine ages' houses
  only their plans, roofs in place of domes and cones, and their windows' shapes. Now every age has its vocabulary (its walls, its
  everyday roofs, its windows, its tops, its arches and domes, its colonnades, its great buildings' forms) and the genome chooses within
  it: `ageExpressed` reads a variable as the age let the people build it, the great buildings (`greatKind`) keeping the people's forms
  where the everyday ones take the age's. This supersedes A-04's "a people that loved domes builds them in dry stone, in masonry, in iron
  and glass, in concrete and in diamond glass" for the everyday buildings (its great buildings still do, in each age's profile) and
  B-414's `ageRoof` (an industrial people's flat roofs and a modern people's pitched, hipped and saddle ones kept).
* **An industrial people builds in brick, slate, sash windows and chimneys**: its walls brick painted or not (`BRICK`, by the base); its
  everyday roofs slate, pitched or hipped (a people of flat roofs behind a parapet), with gables of brick that outlast them and stacks with
  pots; its windows tall sash one to a bay; its frame brick piers, its colonnades cast iron under glass, its platform a bandstand, its
  round temple a brick rotunda under an iron saucer; no crenels, nothing pointed, no onion domes.
* **A modern people builds flat, glazed and in concrete**: its everyday roofs flat; wide panes or bands; its frame a concrete grid; its
  temple of its age (setbacks, shells, needles; no ziggurat, no peristyle, no onion); its focal tower a tower of the air.
* **A medieval people's windows are small** (a third of a bay at the most), whatever its glazing; a futuristic people's arched or
  keyholed windows portholes. Pitched roofs on walls of stone, brick or adobe have gables of the walls' own that stand after them.
* **The genome's reach moves**: the finish no longer acts in the industrial age, the tint no longer in the modern (a modern ruin's glass
  is gone within years), the modules act in the modern through the lift houses. Two industrial peoples' towns are nearer alike than A-04
  drew them (KI-393).
* **What moved.** No generation change (13 holds), as A-04 and B-414 changed the ground without one: no star, body or terrain moved. Every
  civilised world's buildings, and its towns' layouts where a building's pieces met a street; the regress unchanged.
  * `galaxy/buildings.*` (`ageExpressed`, `greatKind`, `archSub`, `ageDome`, `gables`, `stack`, `parapetRoof`, `Walls::parapet`,
    `ageRoof`, `culturePalette`'s walls and windows by age, `BRICK`, the frame, `towerTop`, the industrial and modern generators, the
    temple, the colonnade, the platform, the focal tower, the highrise's tops on its shaft), `galaxy/genome.cpp` (FINISH's and TINT's
    ages), `galaxy/records.cpp` (`Placer::collide`: every piece over a building's ground).
  * The harness: `genome street` (new); `genome sheet ... felisian` a grassland site's.

## B-416 (2026-10-11): nothing round a building

* **A building stands on the ground it covers.** A-04 drew a people of mounds' earth banked against its dwellings and a people of pits'
  rims as round banks of the ground's soil as wide as each building's longest side (a row 40 m long in a ring 43 m across); the user saw
  "round green things around each building [...] they overlap with each other and other buildings a lot". The banks and the rims are gone
  (`foundation`, `genHut`); the pits, the cuts in the rock and the kerbs are the hand-made ages' (`Layout::foundOf`). A plinth of the
  walls' stone in their place read from the drone as a modern slab once a ruin's walls fell, and was left out (KI-394).

## B-417 (2026-10-11): a world's ruins are its people's

* **The old ones' monoliths are gone.** M4-07 put a ring of columns, a cube, a dome, three walls or a 40 m cube in one 2 km cell in
  twenty of every felisian, quartz and ocean world, and C-01 kept them to the bit beside its peoples ("so nothing moved on the worlds
  that had no civilisation"); the user found them on most felisian worlds and asked them gone: "they don't follow the genome, never have
  records on anything". This supersedes M4-07's ruins and C-01's keeping them: `worldHadCivilisation` is the one question (`worldHasRuins`
  is gone), a world without a people has no ruins and no RUINS mode (KI-395), and on a people's world the draw's first twentieth of a cell
  holds the countryside's lone monuments, as on a dead desert world.
* **One code path.** `RuinKind`, `RuinSpec::kind` and `style`, `Ruin::kind` and `style`, the monoliths' drawing (`drawRuins`' own boxes,
  domes and glyphs) and colliders, and the `felisian_ruin` scene are gone; a ruin's echo is its settlement's class (`Landmark::sub`).
* **What moved.** No generation change (13 holds): the people's worlds' first twentieth of their cells (a lone monument where a monolith
  stood), the worlds without a people's ruins (none); no terrain moved. The harness's R-408 check lands 5 km off a town of the people's
  plan (its old landing point lay outside the people's land).

## B-418 (2026-10-11): a people's everyday works are its land's

* **The works wherever a people lived.** T-03 made every world's works once, each rule a handful a people (24 quarries, four
  necropolises, four wrecks, three henges, wind farms one to three) while T-01's plan gave a people hundreds of places over a third of its
  world or more: over 41 people's worlds a tenth of the places and one point of the land in two hundred had one within the scanner's 40
  km, and the user heard "nothing in 40 km" wherever they went. The world's list keeps the history's works and the world's great ones;
  the everyday kinds are the land's (`landWorkOf`): a grid of 8 km cells, a cell holding one by the countryside's density and its age's
  appetite (`LAND_RATE`), the kind drawn among what its ground offers that age, sited by the kind's rule inside the cell (its reach and
  60 m inside: none touch), clear of the settlements, the cores and the world's works. This supersedes T-03's "made once a world" for
  those kinds (the quarries in the rock nearest each town, the necropolises by the capital and the cities, the henges by the old seats,
  the wrecks by the harbour towns, the mines on the territory's richest ground, the wind farms).
* **Made when asked, kept.** A cell's work is made the first time it is asked for (a few milliseconds) and kept for the last two worlds;
  `worksAround` gives the world's and the land's within a reach, `worksAroundIfReady` the view's (never waiting for the world's).
  A lazy grid like the countryside's cells: a world's land holds tens of thousands of works, too many to make at the approach.
* **A fort** (`WO_FORT`) is the land's new kind, so a people whose ground offers little (flat forest, no rock, no coal, no burials, a holy
  place of the heights) still left something: a hill fort, a castle (behind a moat on the flat), a redoubt, pillboxes. A medieval or later
  people's pits on any dry ground and a prehistoric people's flint pits under the forest (it grew since, as over the villages) fill the
  rest; a necropolis serves a village or more, a shrine stands where the cell's ground is its holy place's kind.
* **What moved.** No generation change (13 holds): every people's world's works (the world's list lost the everyday kinds; the land's
  are new), no settlement, record of a place, grave or road; the regress unchanged.
  * `galaxy/works.*` (the land's grid, `LandCell`, `LandSettle`, `landWork` and each kind's rule, `landWorkOf`, `buildLandWork`,
    `worksAround`, `worksAroundIfReady`, `Work::land`, `WO_FORT` and its layout; `WB` writes to a list and keeps clear of the world's;
    `worksNear` gone), `galaxy/landmarks.cpp` (`worksHeardNear`), `galaxy/ruins.cpp` (`settlementClearance`), `galaxy/records.cpp` (a
    fort's record), `surface/surface_view.*` (`WorkNear` a shared `Work`, `workCell` by the work), `game/game.cpp` (`testWalkToWork`).
  * The harness: the land's works in `works <sx> <sz> <body>`, `works view` (a whole name matched whole), the gallery and the map, the
    records' walk round each capital; the works checks' quarry check on the land's, two new (the land's sound, the scanner's coverage).

## B-419 (2026-10-11): what is made on a world is not its season's

* **The season is the terrain's alone.** M9-09 gave the generator a season (the winter cap 11 degrees further, the snow line 900 m
  lower, the sea ice) and the game makes its generators in the season of the moment (the approach's `genFor`, the site's); T-01's plan,
  the people's extent, H-02's histories, the settlements' sites, C-09's roads, T-03's works and B-418's land's read the planet through
  whichever asked first in a session, and a winter's sea ice was land: nearly every place of a plan moved between a winter's and a summer's
  visit. Every reader of the world's made things takes `unseasoned(g)` at its entry (the generator at the year's mean); the terrain, the
  snow and the planet's maps from orbit keep the season. This supersedes nothing written: the plan was meant to be the world's ("built once
  a world"), and the harness had always read it at the mean (`BodyGen::make(b)`); the game's plans now agree with it.
* **What moved.** No generation change (13 holds): the plans the harness reads are the ones it always read; a game session's plan, made
  in its approach's season, is the mean's now (KI-397: a mark put at a town of a solstice's plan). The unit's works checks hold one more:
  the plan, the extent, the settlements and the works the same from a winter's and a summer's generator.

## B-420 (2026-10-11): the walls the explorer meets are the walls drawn

* **One rule for every kind**: a piece's colliders (discs with the heights they span) follow its drawn mesh, measured, not reasoned
  (`walls forms`: a disc is a wall over nothing where its rim at the feet lies outside the drawn solid and off its surface by 0.25 m;
  the unit allows one in 20,000, KI-398's rare forms). A turned form's slices take their narrowest outline there, so the explorer is
  never stopped short of what is drawn (it may stand a hand inside a cone's flank at the head): a wall short of the drawn surface is a
  smaller fault than a wall in the air.
* **Heaps, banks, barrows and tiers are ground**: `formWalkable` pieces carry no discs; the explorer stands on their drawn surface
  (`formSupportAt`) through the view's floor (`floorAt`): a step of 0.5 m, a slope over 1.35 barred, as the ground's. A ring of discs at a
  barrow's foot was a wall where its slope began.
* **One foot, one enumeration**: a big piece is collided where it is drawn (`pieceFootNear`), and the rocks drawn are the rocks
  collided with (`forRocks`), none in a settlement, on a road, by a mark, on an edit or in water. Earth (a walkable piece) stands on the
  lowest ground under it, sunk into the slope above (R-412's run: lifted to the highest like a wall, a hill fort's rampart stood a cliff
  downhill and its inside was out of reach).
* **What moved.** No generation change (13 holds): the colliders, the meshes' undersides and skirts, the rocks on the beds; the regress
  unchanged. The harness: `walls` (forms, view, one, at, place, reach, walk, check) and the unit's four checks.

## B-421 (2026-10-11): a record lies where the explorer can reach it

* **The placer reads what is drawn**: a record's spot is clear of each piece's drawn slice at the floor (0.06 m), of the heaps' slopes and
  of the water, not only of the colliders; a room with a shut doorway (the A-03 check's rule: no metre of its line clear at the knee) holds
  none. The water of the generation is the sea's and the planet function's at 16 m without the rivers (a place is laid out before its
  rivers' tiles exist); the view lays a place's records again with its drawn water within 600 m (`waterRecords`), so a record moves
  only from where it could not be reached.
* **What moved.** No generation change: a few records of a place (those that lay in water, in a piece, under a slope or behind a shut
  doorway) lie elsewhere in it; the works' buildings have their doorways; a holy spring's temple stands 17 m off.

## R-412 (2026-10-11): the genome grows within each age

* **Sixteen more variables, read through the age** (fifty-one; each its own hash, so A-04's did not move): the roofs' pitch, eaves,
  roofline, gables and roofing; the walls' polychromy, trim, projections, entrances and corners; the towns' storeys, frontage, setback,
  boundaries, furniture and a prehistoric people's poles. Each acts where B-415 lets the age speak first: an age's expression, a people's
  choice (a medieval people's cresting pales of its roofing, an industrial people's iron; a modern people's corners glazed, finned or
  rounded, never quoined). `genome reach` holds every value in every age a variable claims.
* **New shapes over new kinds**: one new form (`FK_FENCE`: pales, bars, wattle, a balustrade) serves the yards, the balconies, the
  ridges and the roofs; one new building kind (`BK_FURNITURE`) the streets' furniture, laid last in a town by a stream of its own so the
  plan does not move for it. The rest are the forms there were.
* **Low things are stepped over**: a yard's wall, fence, hedge or railings, a stoop, a hut's ring stand under C-01's 0.9 m and carry no
  collider (KI-398), so no yard traps the explorer.
* **A house faces its street before it is tested**: a town's house turned its door to its street after its pieces were tested against
  the streets; its porch stood in a lane. The door and the yard are set first (`fronting`).
* **What moved.** No generation bump (13 holds: 1.3.0 is unreleased, as for B-414..B-419): every people's dwellings and its works'
  roofs; a town's lots with the setback, the frontage and the storeys (its houses, and the records in their rooms, stand elsewhere in
  it), its plan's places not; the regress unchanged (its scenes hold no settlement).

## C-18 (2026-10-11): the drone bay

* **The per-drone rule over the total.** The request said two to eight a drone "(so minimum 6 and max 32)": three drones at eight are
  twenty-four. The per-drone rule is the one the bay keeps (six to twenty-four a trip); the difference was told to the user, who can move
  `DRONE_MAX_FINDS` or add a drone.
* **The drones bring from where the records are kept.** A record's home (T-04's `recordHomesOf`) is where a drone goes, and copies of its
  people's other records there fill its hands (T-04 lays copies everywhere): a place is the story the screen tells (`IN A CITY`), and the
  per-place counts (one or two of a people's records are homed in most towns) would never fill eight in three stops without the copies.
  The rarer kinds are leaned toward (a star chart 2.2 times a text, an instrument four times, and at one stop in three an instrument of
  the place's people looked for first: a world holds a few instruments to its hundreds of records, nine to 241 on the modern age world),
  so a trip brings two or three instruments and a star chart or two; leaned by weights alone (the place's own records come first, and few
  places keep an instrument) a trip brought one. What is brought is the guide's (the same keys as by hand): the ruins hold it no more.
* **Real time, like the probe.** The trip and the five minutes run on the explorer's time in the simulating states, not on the game's
  clock: the warp (x10,000, the almanac's runs) would otherwise empty the cooldown in a second. A closed game or a menu stops the clock.
  (Reworked the same day, below: two minutes searching, the five minutes from the return.)
* **Called back, not held.** Leaving the parking does not wait for the drones (no refusal of V or an approach): they come aboard with
  what they had found, the rest left down there.
* **Seen in the cabin and outside it.** The rack stands in the cabin's one free stretch of wall (the left wall's front, the GOES console's
  neighbour); the drones back through ports in its back (hidden by the rack's own back panel, so nothing passes through the window band's
  glass), fly past the windows as models lit by the sun and dwindle to strobes over the world. On the screen they are lines (the visible
  faces' edges at the canvas's own resolution), like the ship's other schematics, not a second camera: a surface seen through a drone would
  be a second `SurfaceView`.
* **The case opens on its worlds.** The C-11 screen is one world's now; a case of many worlds' instruments in one column read as a list
  without a place (the user's request); the world under the ship is selected first, as on the decoder.
* **Found on the way: a plan's prefetch at the exit.** A short harness run quit while T-01's prefetch thread was still writing a world's
  histories and works, and the process aborted on a mutex already destroyed (`mutex lock failed`): the static destructors of another file
  ran before plan.cpp's joining one. Both mains now wait for the prefetch threads at the exit (`worldPlanThreadsJoin`, `atexit`), the game
  too (a quit right after an approach could have crashed on the way out).
* **What moved.** No generation change; the regress unchanged (no settlement, no cabin in its scenes); the C-11 unit check opens the case's
  world before playing.
* **Reworked on the user's review (2026-10-11, the same day).** After three trips to their world: "Why on my save Bubas doesn't show shards
  adding up from drones rack? Nothing changes and rack shows there are about 200+ shards and shards decoder there are 130+ or something.
  What I am expecting - when drones arrive everything immediately appears on shard decoder [...] make them look for stuff longer, maybe for
  2 minutes + 5 minutes recharge". Their guide held every find: Bubas 58 shards, 35 of them the drones' (40 finds in three trips, 5 of
  them instruments). The screens counted other things: the rack's `LEFT: 204 RECORDS` what the world still held (its two peoples' 262
  less the 58), the decoder's `OF 129` and `OF 133` each people's whole count, the 32 and 26 held on two rows (C-13: a people a row). And a
  decoder left open did not grow: a world's records were listed on Enter (`shardHeld`) and the worlds on opening, so finds docked while it
  was open showed only after leaving and entering again.
  * The rack says what the ship holds, in the decoder's words: `ABOARD: 58 OF ITS 262 SHARDS` / `AND 7 OF ITS 9 INSTRUMENTS` (the world's
    peoples together); what is left is the difference, not a third number. The finds' list is four rows under the two lines.
  * The decoder lists again when the guide holds more (`listShardWorlds`, `listShardsHeld`, `shardsListed`): the worlds, the open world's
    records (the cursor kept on its record), the player's page (on its row); a record on the screen keeps the count it is read with until
    it is left, then the list is made again. Its worlds' rows ran off the screen with three-figure counts on a world of two peoples (`16
    RE`: H-04's counts in C-13's rows); such a row says `LANGUAGE 100%` for `THE LANGUAGE 100% LEARNT`, the rest as they were.
  * Two minutes down there (`DRONE_SEARCH_S`): a drone's time from its first place to its last, the flights between its places within it,
    the rest shared by the places (a share each and one a find) and its finds spread through each. The trip is about 2:35 from the launch
    to the last dock; the header and the cabin's line count down to the return (`BACK IN 1:52`). The five minutes charge from the return
    (`DroneBay::back`: the last dock or the recall; a `bayback` save line, a save of the first trips reading its end from its docks):
    launch to launch about seven and a half minutes, from five.
* **The case a folder each people (2026-10-11, the same day).** Asked whether a world of two peoples kept two records of instruments, the
  user: "in instruments case split instruments by civilization as a decoder does". The case's folder was a world, both peoples' instruments
  in it by their ids (people x 1000 + age x 100 + slot) with the peoples named together under it (`3 INSTRUMENTS   THE GREIP AND THE
  WIPA`); a world of two is a folder each people now (`openInstruments`: by the key's world and the id's people), the people named on the
  folder's line after the world as the decoder's rows name it (`UNKNOWN (+208 +0 +51 / 5)  THE VASANTHUL`), the open folder headed with
  both (`UNKNOWN - THE VASANTHUL`) and holding that people's alone; the head counts the worlds, not the folders (`12 IN THE CASE FROM 2
  WORLDS`). A world of one people is the folder it was. Two checks: the case's folder its world's and its people's, and a world of two a
  folder each people (an instrument of each of the bay's world's peoples put in the case).

## R-413 (2026-10-11): the two peoples of a world of one age

The user, asking whether a world's two peoples could be of different ages: "It would be nice if they were to differ not more than by 1
stage (because it won't make sense to have futuristic society next to prehistoric). To be honest even having medieval next to industrial
doesn't make sense, it would be cool if both societes were of the same age." Each people's age was A-01's own draw: of 600 worlds of two,
22% of one age, 36% a stage apart, 42% two or more (a prehistoric people beside a far-future one twelve times). Offered the second people
taking the first's age: "yes, I would like planets with two civilizations for both civilizations to be of same age".
`docs/iteration-3/plans/requests/R-413-two-peoples-of-one-age.md`.

* **The second takes the first's age.** Not a draw within a stage of it (the user's first thought, then "the same age") and not a world's
  age both draw from: the first people's own draw is that already, so every world of one people and every first people stand as they
  were. The second people's draw is still taken (`ageOfPeople(pg, age)`: A-01's draws at fixed positions of the stream) and its result set
  aside, so its eras' lengths, its end's years over the age's span and its radio's year are its own draws within the shared age. Of the 600
  worlds of two 466 (78%) have a new second people: its towns, works, records, instruments, history, voice and music; the rest (the user's
  BUBAS, two industrial peoples, and XOHORT, two medieval ones) are as they were. Supersedes A-01's "the age is the people's, not the
  world's" for a world's second people.
* **A people's generator keeps its world's seed.** The charts (the stars a people marked, its figures' names) and the records'
  categories past the fifty read a people's age from its own generator (`peopleAgeOf(pg, 0)`: A-01's "the generator alone"), which a
  second people's no longer says: a second people made prehistoric drew its chart's figures as the medieval people it had been, and the
  records' check caught "sword" (medieval) in a prehistoric record of Eleinewai I. `BodyGen::worldSeed` (set by `make`; C-13's
  `peopleGen` and the copies with a people's seed change `seed` alone) lets `peopleAgeOf` read a second people's generator as that
  people; chosen over passing the age to fourteen callers of the records' and the charts' functions, which take the people's generator.
* **One age, not one time.** Ended apart (half the worlds of two) each people's end is its own draw over the age's span, so two peoples
  of one age can have ended tens of thousands of years apart and never met (H-02 meets them where their years overlap and their lands
  touch or one of them sailed): 13 of the 35 worlds of two nearest home met. Bringing their ends together was not asked; KI-400.
* **The causes and the conquests by the one age.** Two peoples that ended together draw their shared cause by their age (the weights of
  both ages summed before; where the ages agreed the pick is the same: every weight doubled); a people conquered by the other needs no
  later age of it. A prehistoric pair is no longer ended by a neighbour's nuclear exchange (A-01's "prehistoric peoples ended by a
  nuclear exchange exist" was a later people's war).
* **The radar's weak powers pinned again, the rest kept.** A world of two transmits where one of its peoples had radio: 47.8% of the
  worlds of two did, 33.5% do (all civilised worlds 34.2%, now 31.3%), and home's x18 step added none (C-15's rule: one to three peoples
  every step from home and the three C-series worlds). The cells' rate and salt (the near transmitters, the three peoples home hears)
  and the strong transmitters' salt (C-15's far signals: the user's far finds) are kept; the weak powers' salt alone is pinned again
  (`SALT_POWER` 0xC15A0165 to 0xC15A0404): of 8,000 tried on the game's own scan, 33 gave every step one to three from the four places,
  all of them peoples of the four radio ages from home and home's farthest at 5,524 ly; none kept every step at the six age worlds as C-15's
  had, and this one misses one (the far-future world's first step adds four; the other three missed by a step adding none).
* **The test worlds moved with it.** The modern age world was Rieneseirela VI-a's second people (11 ly), medieval now with its first:
  `peoples ages` pins Yeesheleemiola II-b's first (13.9 ly, the modern scene's world, two modern peoples; the drone bay's world, so its
  checks keep a world of two). H-02's worlds of two were Eleinewai I's and Meareafionris III's, whose peoples of one age never meet now:
  the nearest that met (Leanmya V-a, 19.4 ly: 42 shared events) and the nearest with a conquest (Hiameshseine I-a, 37.7 ly: 47). C-13's
  check reads its naming where the peoples met (Eleinewai I's prehistoric peoples never did and name none; Leanmya V-a's name each
  other). T-03's dam holding its water was the Veathiash's on Rieneseirela VI-a: the works' checks read Shioleesein I-a too (196 66
  body 7, 20 ly, the nearest living world whose dams hold their water). One new check: the drones bring of both peoples (185 of 200 hauls
  on the bay's world).
* **What moved.** No generation change (13 holds: 1.3.0 is one generation, unreleased): the second people of 78% of the worlds of two
  and everything its age gives (its towns, works, records, instruments, history, voice, music), the weak transmitters' powers, the
  harness's pinned worlds above. The user's guide touches eleven civilised worlds, two of them of two peoples already of one age: all as
  they were. The regress unchanged (22 hashes); unit 0 failures (313); flow 37.5 s; `bench check` within every budget. KI-400.

## R-414 (2026-10-11): the HUD pared down

The user: "Now let's refactor all the HUD/UI we have since it's overblown at times", item by item: on foot the capsule's, the buggy's
and the drone's distances gone ("there is no need for that if you can always call capsule in 1 button and buggy/drone in 2"), the
readouts "hidden in a separate menu you can open by some key", the epoch, the name with the coordinates, the scanner, the compass and
the stamina kept; in the drone and the buggy the turn bar, the camera's pan, REC, the compass and the scanner kept, "Only keep speed";
the routes travelled removed ("they seem to be a good idea but they confuse you a lot"); the decoder's medium line "just say what kind of
medium this is"; the case's "From <planet>" line gone; the rack's finds "cleared after you took a look at them".
`docs/iteration-3/plans/requests/R-414-the-hud-pared-down.md`.

* **The readings on Tab** (`drawReadings`). Nothing like it existed: the data sheet (`I`) is the world's (its size, its orbit, its
  season, its forecast, its sky, its life) and was already full on a world of many species; the walking readouts are the place's and the
  moment's. A panel over the view rather than a screen of its own: it can stay open while the explorer climbs or drives (the elevation,
  the wind, the range ahead change as they go), and Tab, free on the surface, is the analyzer's key in space (the system's readings).
  At the right (free on foot over the stamina, and in a vehicle, whose speed is at the left), its rows packed to 23 characters so it
  stops short of the crosshair (a first draft at the left, 29 wide, hid the vehicles' crosshair).
  It holds what left the view: the sector (O2's line), the local time and the sun, the elevation, the temperature, the gravity and the
  pressure, the wind and the weather (one row, a second for more), the life sensor (B-321), the old road (C-09), the rangefinder and
  the mark it rests on (O1, R-408), swimming, the jetpack's height and climb, the autowalk and the vision; in a vehicle its heading,
  trip, gear or height and climb, and its state. Supersedes O1's rangefinder, O2's sector line, B-321's sensor and C-09's road on the
  view.
* **What E or Q would do stays** (`CAPSULE HERE - Q`, a record's or an instrument's `- E`, a grave's name), stacked over the stamina
  from the bottom; `BUGGY HERE - E` and `DRONE HERE - E` are new where the vehicles' distances went (E takes the vehicle before a find,
  as `updateSurface` does). The jetpack keeps its heat on the view, the one gauge besides the stamina (its height and climb are in the
  readings).
* **The nose camera, one branch for the buggy and the drone**: the brackets, the blinking REC, the pan gauge, the crosshair, the speed
  and the turn bar; the epoch is not drawn from a vehicle's camera; the scanner's lines sit under the REC as they sit under the name on
  foot. Supersedes R-204's and R-403's channel name, pan and tilt, trip, gear, heading, state, capsule and local time on the view (the
  heading, the trip, the gear, the height, the climb and the state are in the readings; the channel's name, the pan and tilt numbers and
  the capsule nowhere).
* **No ways travelled**: G-05's route (the guide's `route` lines and the chart's green legs) and M4-02's trail (a point every 25 m on the
  N map) are gone, not hidden: kept unread they would be a second code path. The stars visited keep their dots and their VISITED.
* **The medium line names the object**: supersedes C-05's voice on the decoder's line and H-05's "read by the ship" and "played by the
  ship"; the ship still reads a record without a voice of its own in its flat voice (C-05's speech unchanged).
* **The case's instrument line is the instrument**: the people's name moves to the open folder's title on a world of one too (C-18 put
  it there on a world of two), C-17's `FROM <world> (<sector> / <body>)` is gone.
* **The rack's list goes once seen** (`droneFindsSeen`): C-18 kept a trip's finds and places on the rack's screen until the next trip;
  the screen closed after the drones are back clears them (a trip still on keeps them: they are not all back). The log and the status at
  the dock still count them; the guide holds them.
* **The landing map's hints** fit (R-415's V took the room the zoom's line never had).

## R-415 (2026-10-11): the peoples on the landing map

The user: "on the capsule deployment map there are these dots for places were civilization ruins were found - they are too huge and
non-subtle. Please, make them turned of by-default and make it a separate button. Why? It would polute map if you find too many cells
with ruins/works. There should be some map like the one you used when you were adding cities layout, it just should be a button toggle.
Also It would be cool to have something like whole map autocompleted when you find all the shards for a civilization, as if you have
enough information to know where is what, where to find cities, capital, etc". The dots were their own marks of ruins (R-408: BUBAS
holds twelve, seven in one town's few kilometres), a glyph and a name each in the zoom.
`docs/iteration-3/plans/requests/R-415-the-peoples-on-the-landing-map.md`.

* **V, off on every opening.** The marks of ruins and works, the zoom's old roads (C-09) and the plan's cores (T-01) are the layer's;
  the marks of the natural sights stay on the map. Not saved: the map opens on the land each time, as asked.
* **What the explorer knows, from the records held.** A record names the place it was kept at (T-04's home: of the places standing in
  its year, by their size), so each held record shows its home: a point, a city paler and rimmed, the capital white and named. Chosen
  over the places the explorer walked (the guide keeps no trail, R-414) and over the drones' searches (they search where the records are,
  so they would show the same places a trip later). The homes spread over the land (BUBAS-sized peoples keep a record or two in most of
  their places): a few records show scattered towns; the capital shows once a record kept there is held.
* **The whole map once every record is held**, the user's "autocompleted": the people's land tinted to its frontier, every place and
  every way, by kind, as the plan harness draws them (`NET_KIND_COLOURS` is `galaxy/plan.h`'s now, the harness's too). All of a
  people's records, as asked, not a share: a people's 80 to 200 records are an hour or two of the drones' trips (6 to 24 finds a trip,
  a trip every seven minutes or so) or longer on foot.
* **The reading** is the drone bay's (`droneWorldAsk`: the world's records with their homes, the plan's places), asked at the map's
  opening on a civilised world; the plan is the approach's prefetch (`worldPlanIfReady`: the whole map waits for it, the known places
  do not).
* **The names of the marks by the cursor**: the three nearest within ten pixels are listed beside it, not at each mark, so a town of
  marks is a cluster of points and a short list where the explorer looks.

## R-416 (2026-10-11): the decoder's quick scroll

The user: "Can you add a quick scroll in timeline/history with shift pressed so you don't have to click right/left or up/down one event
at a time?" The arrows took one step a press (the platform's keys do not repeat: `IsKeyPressed`); `PGUP`/`PGDN` jumped ten and eight,
but a Mac's keyboard has them only under `FN`.
`docs/iteration-3/plans/requests/R-416-the-decoders-quick-scroll.md`.

* **Shift and an arrow held run the cursor** (`shardRunSteps`, `game/shards_screen.cpp`): a step as the arrow (or Shift) goes down, then
  from a fifth of a second on ever faster, 12 a second doubling every half second to 100 (17 steps in a second's hold, a hundred in about
  two), so a short hold lands near and a long one crosses a people's records. A run stops at the first and the last; the arrow alone
  still goes round on the timeline. A slow frame (the history read again) counts as a tenth of a second at most, so it never leaps.
* **The timeline and the history**, as asked. The records page keeps its steps: its hint line (47 characters of 53, 48 with `* UNREAD`)
  has no room for `SHIFT FAST` beside `LEFT/RIGHT KIND`, and its `PGUP`/`PGDN` turn its pages of fifteen rows (superseded by R-417 on
  the user's asking: the records page runs too).
* **The hints say so**: `ARROWS  SHIFT FAST  ENTER READ  TAB PAGE  ESC BACK` (the timeline: Up and Down move it too, as before),
  `UP/DOWN  SHIFT FAST  ENTER READ  TAB PAGE  ESC BACK` (the history: Enter reads the record that told the event, as `ENTER THE RECORD`
  said; that line had no room for both).

## B-422 (2026-10-11): nothing far off is computed on the main thread for a glance

The user: freezes of five to seven seconds when landing ("looking slightly underground"), at `Tab` or `R` just after, and at an Enter,
with the CPU spiking while the landing map's cursor moved. `docs/iteration-3/bugs/resolved/B-422-freezes-at-the-landing-tab-r-and-enter.md`.

* **A mark far off is read without its rivers**: the marks' ground at the map's 64 m reads the drainage only where its tiles are built,
  as the scanner's sights' heights already did; a mark read without them asks again every two seconds. Reading them built a tile of a
  degree a mark on the main thread: nineteen marks, eight seconds, at the readings', the scanner's or a shown mark's first look. The
  height is the shown mark's diamond's alone, so a far one a few metres off its ground until the explorer's way builds its tiles is the
  price (KI-403).
* **The descent ends at the eye** (1.65 m), not on the ground: its last third of a second stood the camera under the near ground's
  triangles, the frame a freeze held.
* **The landing map asks for a place's tiles once the cursor rests 0.4 s**, not at every step: a step's asking cost a thread of eight per
  tile and Shift held kept every core busy. A landing right after a move builds the site's tiles then (KI-403); a rest of two seconds
  has them.
* **A prefetch's tiles side by side, a superseded one's abandoned**: lanes, the nearest the point first; a worker keeps to every core
  but two, a caller that waits takes every core but one; a tile being computed for a superseded worker is abandoned unless a thread
  waits for it or the newest asked for it too. A tile is computed whole or not at all and the same as before: no generation change.
  Half the cores for a worker was measured and refused: the slow walk over the map cost as much and its landing waited longer.

## R-417 (2026-10-11): the records page runs too

The user: "in shards decoder also add this fast shift scrolling for records tab". `docs/iteration-3/plans/requests/R-417-the-records-quick-scroll.md`.

* **`SHIFT` with `UP` or `DOWN` held runs the records page** as R-416's run does, through the kind shown and stopping at the ends;
  `LEFT`/`RIGHT` stay the kind's keys, with `SHIFT` or without.
* **The hint makes room with the font's arrows**: `ENTER READ  ↔ KIND  SHIFT FAST  TAB PAGE  ESC BACK` (51 characters of 53; 52 with
  `* UNREAD`). Words (`LEFT/RIGHT KIND`) beside `SHIFT FAST` were 59; dropping `ENTER READ` or `TAB PAGE` would have hidden what the
  page is for on the page the decoder opens on; `L/R` reads as the `L` and `R` keys.

## F-01 (2026-10-12): what the bench could not see, and the proof tools

The first milestone of the performance plan (`docs/ideas/PLAN-performance.md`): nothing drawn changes; the tools every later F item is
proved with, and the numbers that decide which of the plan's estimates stand.

* **The proof is the harness's.** `pngdiff` (two PNGs or two directories: the pixels that differ, the largest step of a channel, the
  first, the rows, the eighths, a diff image in red) and `bothways <scene> [scale]` (a scene by the workers twice, by one thread, by the
  workers again) replace the audit's thirty-line Python reader. `unit`'s new check draws fourteen frames at 4x the regress never hashes
  both ways in one run (the shore; the landing map and its zoom; the cabin lit and dark, polarised and glass; the roof; the radar camera;
  the drones out at the rack and outside the window; the ground; the nose camera; the sector map), each frame drawn at one instant
  (`Game::frame(in, 0)`) and twice by the workers first, so a frame that changes by itself fails rather than proving nothing. All
  fourteen are the serial loop's today. The forest is not among them: it is B-423, measured again here (13,921 pixels at 4x, 3,419 at
  2x, 506 at 1x, steps to 130; the shore identical), and joins the check with its fix.
* **The flow is not a reference.** Two runs of the same binary from the same files differ in 44 of the flow's frames (the threads
  deliver drainage tiles, works and scans at different frames; the slots, the log, the gallery and the drone bay's charge read the wall
  clock), and the unit's six scanner and marks frames follow what a previous run left in `shots/tests`. A change is proved by `regress`
  both ways, the `scene` frames at 2x and 4x against the previous binary (deterministic), the unit's check and `bothways`; the flow
  only from the same restored state beside a baseline-against-baseline run (F-01: the baseline and F-01 identical in all 1,113 pictures,
  the baseline against itself 44 apart). Whole copies of `shots/tests` (about 2 GB) filled the disk once: copy what a run writes.
* **The bands' trace stays in the tree.** Two clock reads a band give every frame's terrain and flora bands (rows and ms), which the
  bench prints; `VESPERIS_TRACE` adds the terrain's split (four clock reads a drawn cell). It shows F-04's premise in numbers: at
  surface 4x every band costs 2.5-5.2 ms whatever its rows, and in the forest each of the eight walks all 9,079 cells and sets up the
  water of all 2,982 wet ones.
* **The real game's frame is traced, not benchmarked**: `VESPERIS_TRACE=1 ./vesperis` prints the poll, the game, the audio pump and its
  synth blocks, the shot and recorder writes, the upload, the drawing and the swap once a second, and the percentiles at the quit (the
  platform layer reports its parts through `platformPresentTimes`/`platformAudioTimes`). The window's states were measured with a
  scratch copy of the window loop driven by script (removed): two minutes each at 4x in a 1280x800 window on the dev Mac's built-in
  ProMotion display, muted (the synth renders every sample and scales them by zero). The synth's block costs 0.07 ms quiet and 0.76 ms
  at most (the ship's player, the radar, the decoder's piece; one block in 5.4 frames; the ground walked had no wind): F-08's helper
  thread is under its bar and is dropped. The upload is 0.38-0.52 ms (0.96 at worst): under the bar of a pixel-unpack ring, not planned, as the plan said. The pacing
  is the finding: with `SetTargetFPS(60)` on top of the vsync the frames' intervals sit at 16.8-17.2 ms and 4-28% of them exceed
  17.5 ms while the work is 7-13 ms (driving: 9.0 ms of work, 55 frames a second); parked in the cabin, where the work nears 14 ms,
  3.3% of the frames wait for a second refresh (33 ms). F-08 becomes the platform's pacing experiment (`SetTargetFPS(0)` with the vsync,
  judged by the same histograms; no pixel changes). The decoder at 4x with a piece playing runs at 30 frames a second (18.5 ms of work):
  a frame the bench's row (13.3 ms, no piece) understated, for F-05's screens.
* **Rows without budgets and modes for the stalls**: the bench's new rows (the maps, the radar, a recorded frame and a photo's PNG,
  the telescope, the decoder, the sun in the view, a sprint, the city, the probe and the capital at 4x, an arrival, the guide's save and
  load) and `load landing|map|arrival|drive|capital|probe` (B-422's cores-and-threads measure, paced at 60 frames a second), `soak`
  (memory over a long expedition), `reanchor comet`, `bench noise`, `zoom`'s parts. F-11 sets budgets where they are stable.
* **What the numbers say about the plan's estimates.** The guide's save is 7.9 ms (the player's 731,810 bytes; the bench's guide of
  the same composition 7.6-7.9), not 20-35: F-02's item still turns a dropped frame at space 4x (14 + 8 ms) into none, for a third of
  the gain estimated. A 4x PNG is 16-19 ms and a recorded frame 27.8 ms, as estimated. The landing zoom builds in 24-32 ms (the home
  worlds' samples, the modern capital's roads 14.7 ms of it), a frame or two, not the "seconds" feared: its progressive fill is not
  needed. A world not yet mapped blocks the landing map's opening 150-200 ms a map (two maps, 287-414 ms in every `load` landing),
  which F-02's `mapIfReady` item answers. The landing map at 4x is 7.3 ms (13.5 with the cursor tapped) and the radar camera 6.7 ms
  (it has no cabin): neither over the budget the plan feared, so F-05's blits and feeds are gains, not rescues. The telescope at 4x is
  25 ms (101 serially), and the probe's fall toward its giant holds 26 ms a frame from ten seconds after the launch (the giant filling
  the frame, as at the eyepiece; the entry's handover, KI-361, comes later): the worst steady frames at the default scale, rows the plan
  did not have (KI-406). The trees listed against a
  settlement's buildings cost 1.0 ms in the prehistoric village's forest (2,855 trees) and nothing at the other test capitals, none of
  which stands in a forest: F-02's 20-120 ms stays an estimate. The pool's dispatch is 34-50 µs after a rest and 22-32 µs back to back:
  over F-06's 25 µs bar, so its spin stays. `gnoise3` costs 88 ns a call against `gnoise3d`'s 23.5 with its gradient: F-07's gradient
  table is worth more than estimated. A comet's re-anchor costs its next frame 30-33 ms, F-03's parallel fill's case. The people's
  descent sits on its 80 ms budget with either binary (four of sixteen runs over: F-03's). The sweep: ten threads take space 4x from
  14.05 to 13.41 ms while the surface rows are fastest at four; half the rows-per-thread rule takes space 2x from 4.01 to 3.65 (F-06).
  The site's roads' rebuild on the main thread (every two ruin cells) was not measured: the buggy stops at
  the capital's edge long before 4 km (`load drive`: 0.39 km in 30 s), and `load roads` (the explorer moved 4 km at a time) waits
  for an awake machine. A long expedition's memory (`soak`, 20 minutes at 4x before the Mac's lid closed) settles at 115-185 MB
  from its second stop on (24 maps and 28-36 drainage tiles held, both at their caps; the peak 185 MB): no growth.
* **Left open.** One bench and one serial-against-workers hash on Linux: Docker Desktop's daemon did not answer (before and after the
  machine slept), so the container run (a copy of the tree built in `vesperis-build:arm64`, `regress bless` into the copy, then
  `VESPERIS_THREADS=1 regress` against it, `bothways`, `bench check`) waits for it; no Windows machine. The runs that met the Mac's idle
  sleep (the session's batch from 18:22 on) were taken again awake under `caffeinate -i`.
* **What moved.** No generation change (GEN_VERSION 13); the regress unchanged, and with `VESPERIS_THREADS=1`; the scene frames at 2x
  and 4x, the unit's and the flow's pictures (from the same state) identical to the previous binary's; `bench check`'s rows within 1.3%
  of the previous binary in four alternating pairs (the people's descent in eight: 77.8 ms against 77.2); `unit` 320 checks. New:
  `pngdiff`, `bothways`, `bench noise`, `load`, `soak`, `reanchor comet`, `zoom ... site|capital`, the bench's F-01 rows, the unit's
  both-ways check and pool print, `SurfaceView::BandTrace`/`bandTraceLine`/`lastFloraListMs`/`lastRoadsMs`/`roadsBuilds`,
  `SpaceRenderer::MapStats::blocked`, `Game::testZoomBuildMs`, `benchFeatureField` (planetmap), the window build's trace.

## B-423 (2026-10-12): the forest the same on every machine

`docs/iteration-3/bugs/resolved/B-423-the-forests-picture-depends-on-the-core-count.md`; the performance plan's section 4.

* **The flora's bands are pinned, not its grid made global.** The canopy-coverage grid is per band (B-310), so a far tree whose canopy
  crosses a band edge shows in one band layout and hides in another, and the layout followed the thread count: the forest differed
  between one thread and eight (13,921 pixels at 4x) and between machines. `parallelBands(n, bands, fn)` runs exactly `bands` bands
  whatever the threads (shared out among them, one after another on a single thread); the flora runs `floraBands()` = min(8, FBH / 32):
  six at 1x, eight at 2x-4x, the dev Mac's layout, so nothing moved there. The plan's other way (one grid a frame by the serial rule)
  would have moved the forest on the dev Mac for a serial pass of ~0.2 ms; the plan recommended the pin and the user's "proceed" took it.
* **The price** is the one-thread forest's: it walks the eight bands' trees one after another (the band trace: 4,804 visits at 4x
  against 2,073 in a single band): `VESPERIS_THREADS=1 bench forest` 39.7-40.3 ms against F-01's 37.8-38.7 (the flora 14.6-15.6 against
  12.6-13.3), and in one serial `bench check` each the forest 2x 22.3 against 19.2, 4x 39.5 against 37.2, at the game's 3x3 40.5
  against 36.5; every other serial row the same. With the workers nothing: four alternating `bench check` pairs against F-01's binary
  put every row within the runs' own spread (measured 2026-10-13, with F-02).
* **What moved.** No generation change (GEN_VERSION 13); the regress unchanged (and with one thread); the default-thread scene frames
  at 1x, 2x and 4x identical to F-01's; with one thread the forest, the tropical biome and the buggy's seat moved to the workers'
  picture (B-423 reached every felisian scene with trees). `unit`'s both-ways check holds the forest at 1x, 2x and 4x with one, three and
  eight threads (20 frames). New: `parallelBands` (`core/parallel.h`), `floraBands` (`surface/flora.cpp`).

## F-02 (2026-10-13): the stalls in play

The second milestone of the performance plan (`docs/ideas/PLAN-performance.md`): the hitches a player feels at an event, which the
bench never saw (its guide was fresh, its PNGs written outside its timed loops). Nothing drawn changes; a world not mapped yet no
longer holds the landing map or the drone bay (the user's choice).

* **The guide writes its probes' lines as they were made.** `Guide::save` formatted the probe records' 39,615 doubles again at every
  logged event (566 KB of the player's 731,810-byte guide; a sector crossed, a sighting, a mark, a photo, the five-minute autosave). A
  `ProbeRecord` keeps its four lines (`probecam`, `probetime`, `probeprofile`, `probelayers`) in `lines`, made by `probeLines` (an
  `ostringstream` at the file's precision 12) once the whole file is loaded, or at a new record's first save (`probeKeep`'s); the
  inbox's import copies them with the record. The tracks never change once a record is made (said at the vectors). The same bytes: the
  player's guide loaded and saved by the old code and the new is the file itself, byte for byte, through a load-save-load-save chain
  and an import (but the lending's clock). A save of it 7.0 → 0.9 ms (20 saves, a scratch program); the load pays the lines once, 6.8 →
  13.2 ms at the start. The plan's dirty flag (one save at the frame's end) was not built: two events in one frame now cost 2 ms.
* **The PNG encoder writes the same bytes about three times sooner.** The rows packed through a pointer (three `push_back`s a pixel
  before); the Adler-32 sums reduced once a run of 5,552 bytes (zlib's NMAX: the most bytes after which the sum still fits 32 bits, so
  the result is the per-byte modulo's exactly); the chunks' CRC run over the type, then the data in place (the data was copied behind
  its type); the CRC table a function-local static (made once, safely, from the writer's thread too); the bytewise CRC kept. A 4x
  photo's PNG 15.4 → 7.2 ms (the medians of four alternating `bench check` pairs, as every bench number here). Proof: 17 scenes at 1x,
  2x and 4x byte for byte the previous binary's files (51 pictures), the unit's check, and a Python check of every chunk's CRC and the
  IDAT's zlib stream with its Adler-32 (the scenes' 51, the TSan run's 47).
* **One thread writes the photographs, the recording and the probe's image.** `writePNGAsync` copies the pixels (0.4 ms at 4x) and
  queues them for one thread (`PngWriter`, made at the first picture) that writes them in their order with `writePNG`'s encoder; the
  caller waits only while three are queued, so no picture is dropped and the names stay in order; the pixel buffers are kept for the
  next pictures (a fresh 4 MB a frame cost a millisecond of page faults). `pngFlush` returns once the queue is on disk: `readPNG`
  flushes, the gallery flushes before it lists its directory, a recording flushes the last take before it starts, the window flushes
  before the platform closes, and the queue's destructor writes what is left at any exit. The panorama, the harness's own pictures and
  the browser build stay synchronous. A decision: a thread for a result no frame waits for, like the maps and the tiles
  (`01-architecture.md`). Recording at 4x (the bench's row in space): 24.5 → 9.6 ms a frame, the recorder's part of it 15.8 → 0.1 ms,
  the writer one picture behind at the row's end (its queue empty 7.2 ms after); serially 37.1 → 21.9. Its behaviour changes: at 4x it
  no longer halves the frame rate, so a take holds twice the frames a game second it did (KI-022). ThreadSanitizer is clean on a
  scratch program (a burst of 40 through the queue with flushes and reads back, three left for the exit's drain).
* **A settlement's buildings tested only where they can hold the point.** The trees' listing (`forTrees`), the logs and the rocks
  (`inSettlementBuilding`) turned the point into every building's frame with a sine and a cosine before the rectangle test. A point
  beyond the circle round the rectangle (with the margin) is outside it at any heading: a turn keeps the length, and a slack of 1e-9 (a
  hundred times the turn's rounding) keeps every point the rectangle would hold. The yards' one hash is drawn before the buildings (it
  clears 67-95% of a yard's trees), and `forTrees` calls `inSettlementBuilding` (one rectangle test in the tree, not two). The listing
  at the prehistoric capital in its forest (`load capital 6 4 0`, 2,855 trees): 0.99 → 0.70 ms a frame (the frame's p50 11.9 → 11.6);
  the bench's capital 4x 9.20 → 8.93 and the descent beside the modern capital 78.4 → 75.1 ms (its 80 ms budget, which it touched).
* **The sky's direction table made on the pool.** `buildDirLUT` (1,024,000 entries at 4x, a pure function of the pixel) runs in
  `parallelFor` bands: the sprint's FOV kick makes it again on six frames at its start and its stop (the 1.2% rule kept: a hysteresis
  would change which frames rebuild and so the sky during the ramp). The sprint's second at 4x: its frames over the median + 1.5 ms 2-4
  → 0 (one run of four kept one unrelated 23 ms frame), the worst 12.0 → 10.9-11.2 ms; with one thread the same as before (the pool has
  nothing to share).
* **The star neighbourhood kept across moves.** `StarNeighborhood::update` made its 21^3 cube again whenever the ship's sector changed,
  every frame of a Vimana flight. A star is a function of its sector alone and both cubes are listed x, y, z ascending, so the sectors
  the new cube shares with the last take their stars from the last list by one forward cursor, in the order a fresh walk finds them;
  only the sectors new to the cube are generated (`builtRadius` says what the list holds). Vimana 2x 2.57 → 2.24 ms (serially 4.40 →
  4.07): a third of the estimate, the cube costing 0.5 ms a rebuild along the bench's flight, not 1.4 as at home; the unit's 53 moves
  6.8 ms kept against 25.0 fresh.
* **The landing map's probe of its cursor is a frame, not a site.** `SurfaceSite::initFrame` is `init` without the five terrain caches
  (106,496 vertices, 3.8 MB, filled at every step of the cursor and never read by the probe); the probe, `testReliefClass` and
  `testLandSite` call it. The caches' fill cost less than estimated: the landing map at 4x with Shift held 7.27 → 7.26 ms a frame,
  `load map` (the cursor tapped) p50 13.5 both, p99 15.2-15.3 → 14.6-14.7; the 3.8 MB are no longer held.
* **A world not mapped yet: the screens open at once (the user's choice).** The landing map and the drone bay asked for the world's map
  with `mapFor` and waited 150-200 ms a map. Asked whether they should open at once and show the map when it is made, the user chose to
  open at once (2026-10-13). Both read the map through `mapIfReady`: the landing map draws its empty frame with `READING THE WORLD...`
  until the map's thread has made it (the keys work meanwhile as they did after the wait: `Enter` lands at the cursor, `R` waits for
  the map), the bay its `THE MAP IS BEING DRAWN` and builds its picture when the map is there (`droneBayWorld`). The finished map is
  the same picture. The harness's `testParkAt` waits for the map before it returns, after giving the renderer the system's season as a
  frame would (the map was made for the last system's season, then again: the "two maps" of F-01's `load` landings); `testParkAt(s, b,
  false)` does not; `testWaitMap` for the tests that open the map with `C`. The harness's parking at a world never seen (`load
  capital`) blocked twice for 358-420 ms before and not at all now.
* **Measured and not built.** the plan's bound on the arrival's map threads (two at a time, nearest first): `load arrival` runs its
  first second at 8.6 ms a frame against 8.4-8.8 after, nothing to win on ten cores (a four-core machine is the open question, as
  before); the roads' block in slices: `load roads` (the explorer moved 4 km ten times by the modern capital) grows them in 0.0-0.3 ms;
  the hop's frame of 26-29 ms is the rings' refill after the move (F-03's case); the comet's re-anchor is F-03's (F-01).
* **What moved.** No generation change (GEN_VERSION 13); the regress unchanged, and with `VESPERIS_THREADS=1`; 17 scenes at 1x, 2x and
  4x byte for byte the previous binary's files, with the workers and with one thread; `unit` 324 checks (four new), 0 failures; the
  flow (32.1-32.2 s; the previous binary's 32.8-33.7) from one restored state in eight runs: F-02 drew all 163 pictures of the previous
  binary's in one run and all but one in two (`flow_shipscreen.png`: a world's map arriving a frame later under the dimmed screen,
  which F-01's binary drew so too), the previous binary differed from itself in 45 pictures in one of its four; `bench check` within
  every budget in four alternating pairs, every row not named above within the pairs' own spread. New: `ProbeRecord::lines`,
  `probeLines`, `writePNGAsync`, `pngFlush`, `SurfaceSite::initFrame`, `StarNeighborhood::builtRadius`, `Game::droneBayWorld`,
  `Game::testWaitMap`, `Game::testMapReadyNow`, `Game::testDroneBayMapped`, `testParkAt`'s `waitMap`.

## F-03 (2026-10-13): the first frame of a descent, and the tiles

The third milestone of the performance plan (`docs/ideas/PLAN-performance.md`): the frame under the fade at a landing's Enter, where
the site is made and every cell of its rings is sampled at once, and the drainage tiles a landing waits for. Nothing generated changes
but one cache's order dependence, which the parallel fill exposed (fixed: the user's choice); every tile is the same bytes.

* **The rings' misses sampled on the pool.** `warmTerrainLOD` sampled every missing cell of a ring's disc on the main thread while the
  pool waited: at a landing's first frame 12,084 cells in four rings (13.9 ms of the frame at the home world's default site), at a
  re-anchor all five rings, after a hop the near ones. It now lists the disc's misses first (`fillMisses`, kept) and, over 64 of them
  (`FILL_SERIAL_MAX`), samples them on the pool in equal shares of the list (`parallelFor`), each listed cell by one worker into its
  own slot: a ring's disc is 2r + 3 cells a side, within its torus's `1 << logN` for every ring (83, 75, 55, 55 and 63 against 128,
  128, 64, 64 and 256 on a planet; the unit checks it on a planet and a comet), so distinct cells have distinct slots, and a sample is
  a function of the cell and the site (the drainage read through each thread's own caches; the site's roads, streets and works read,
  never written). The pool's join is the barrier before the vertex pass. Fewer misses (a walk's edge) are sampled by the disc's walk
  as before. A decision, amending M7-01's rule (the caches written by the main thread alone; the row says so): the caches are written by
  the main thread, or by the pool inside a joined region on distinct slots; the ahead worker keeps its staging buffer. The fill at the
  home world's site 13.9 → 3.3 ms; the descent's prefetch (`prefetch(3.0)`, budgeted, done in the descent's first twenty frames) stays
  serial, as the plan's checklist said.
* **A sample was not a function of its point: fixed (the user's choice).** The 67 comparison scenes drawn by the workers and by one
  thread found `civilisation_road` 3 pixels apart (one step). The drainage keeps a thread's last neighbourhood of reaches (`t_neigh`,
  keyed by the world, the 400 m lattice cell, the coarse flag and the brooks), but `buildNeigh` bent the reaches with the east-west
  metres of the first point asked in the cell (its latitude's `cellLonM`) and chose the coarse list's channels by the first point's
  scale: the rivers near a point moved by millimetres to centimetres with the order the cells were asked for. Serially that order is
  fixed, so a run repeated itself; the ahead worker (a fresh thread a job) and the landing map's zoom on the pool were order-dependent
  already, and the fill on the pool would have drawn a picture of its own on every core count (B-310's contract). Asked whether to fix
  the cache (the rivers made from their own cell; five scenes moving once) or to keep the fill serial (every picture byte-identical,
  the descent ~32 ms and the re-anchor ~30), the user chose the fix (2026-10-13). The neighbourhood is made from its cell alone (the
  cell's centre latitude; the coarse list keyed by its scale). What moved, against the previous binary: of the 67 scenes at 2x
  `biome_taiga` 18 pixels, `civilisation_futuristic` 3 (40 at 4x), `civilisation_road` 3, `felisian_river` 8, and
  `civilisation_second_people` 1 at 1x and 45 at 4x, every one a step or two; the regress's 22 hashes, every drainage tile and 815,409
  vertices of three grids round the home world's sites (4, 16 and 64 m, asked row by row) unchanged; no GEN bump (no generation hash,
  map or save moves). The fixed picture is the same with 1, 3, 5 and 8 threads.
* **The material tiles a ring wants, made together.** The disc's walk after the fill names the materials whose meso tiles are not
  made yet and `buildMesoTiles` makes them a material to a worker (each a function of its material and the seed, into its own slot);
  a band thread reads a tile, never makes one, as before. On the modern test world the far ring's first fill held five or six of them,
  about a millisecond each: its fill 8.3 → 2.6 ms.
* **The landing's ground textures on the pool.** `buildMacroTile`, `buildLeafTiles` and `buildBarkTile`, made at every landing (7 ms
  together), run their rows on the pool (a texel is a function of its place and the seed; `SurfaceView::init` already dispatched the
  galaxy band and the direction table, so it never runs inside a pool job). The meso tile's own rows stay serial: a band thread may
  ask for one, and the pool takes no dispatch from inside a dispatch.
* **The drainage tile, the same bytes in about half the time.** The plan named the topological sort (5-13 ms a tile); timing the
  tile's parts found the breach's descent: after a pit is breached, the walk down from the spill's far cell turned back on itself in
  every walk that reached its guard (the tile's cell count), from its third or fourth step: two cells, each the other's lowest
  neighbour, sinking in turn 5 mm a step (520 million steps, 3.1 s, in one tile of lake country). Once the walk has turned back twice
  running it alternates to the guard (the two only sink, two steps lower at each turn, and nothing round them is written), so the two
  cells' last values are the last two of the same float subtractions, computed a binade of floats at a time (`drainageSink`: inside one
  binade every step rounds to the same multiple of the spacing, so k steps are one exact subtraction in double; a step under 1 m, and
  one that might leave its binade, is the subtraction itself). The sort runs on one integer key a cell (the height's bits made
  monotone over the pop index: a strict total order, the comparator's permutation); the breach's queue is emptied at once; each cell's
  routing noise is made beside its sample on the sampling threads. `drain`: the median tile 163 → 96 ms, the worst 3,202 → 177; `drain
  desert` 75 → 50 (440 → 129); `home` 162 → 137 (its candidates' default sites 0.5-1.0 → 0.3-0.8 s); the shore scene's four 196 → 169
  (their sampling). Proof: a hash of every finished tile's arrays in the trace line (`tileHash`): the 159 tiles of `drain`, `drain
  desert`, `drain thin_atmosphere`, `home` and `scene felisian_shore` the same by both binaries; `drainageSink` against the loop in
  43,462 cases (5.1 billion steps) in a scratch sweep and 3,720 in `unit`.
* **A quirk found and left** (a generation question): the walk sinks the two cells about 1.2 km down the routing surface before its
  guard stops it, so every neighbour drains into them and they come last in the topological order: the water gathered there is not
  carried down past the cell they drain to (KI-408; 50-120 such pits in a felisian tile, 2,121 in the lake country's). A GEN bump's
  question, not this plan's.
* **The numbers.** The descent's first frame at the home world's default site (the bench's row, 1x, the medians of four alternating
  pairs) 40.0 → 22.3 ms (in `load landing`'s trace 37-41 → 22.5: the fill 13.9 → 3.5, the textures 7.0 → 1.8; the galaxy band's 7.6 and
  the terrain's 4.4 remain); beside the modern capital 78.9 → 45.5 ms (the plan's estimate was ~60; its town marks, 11 ms, the capital's
  cells built, remain); a comet's re-anchor's next frame at 4x 30.0-32.8 → 14.9-15.8 ms (under a 60 Hz frame, KI-302); the `load
  roads` hop's frame (4 km at once) 26.7-28.8 → 11.6-15.3 ms. The budgets fall with the rows (the method's step 5): the descent's first
  frame 80 → 35 ms, beside a capital 80 → 70. The plan's estimates were 37 → 20-22 and 77 → 55-62: both met.
* **Measured and not built.** The galaxy band at a landing (7.6 ms, on the pool already: its equal bands wait for the efficiency
  cores, F-06's dealing); the town marks beside a capital (11 ms: the capital's cells built, a cache of their own); the prefetch
  during the descent (above). A cold landing builds its tiles in `SurfaceView::init`'s synchronous prefetch (radius + 3 km + 0.2
  degrees + ten lattice cells, about 77 km on a 6,000 km world, against the fill's 58 km disc): the fill never builds one (two traced
  landings entered a second after the landing map opened: no tile built inside the first frame; the resident memory 72-97 MB at 1x).
* **What moved.** No GEN bump (GEN_VERSION 13); the regress unchanged, and with `VESPERIS_THREADS=1`; the 67 scenes at 2x and the 17 at
  1x, 2x and 4x the previous binary's but the five scenes above, every one the same with the workers and with one thread (the five
  also with three and five); `unit` 326 checks (two new), 0 failures; TSan clean on a landing, two comet re-anchors and a landing beside
  the modern capital; `capsule` 0.00% at every height; the flow (32.4-33.3 s; the previous binary's 32.2-32.3) from one restored state:
  the previous binary twice identical, F-03's two runs 1 picture apart from it (`flow_shipscreen.png`, the known one) and 46 in the
  other (the flow's timing set: the almanac, the slots, the log, the gallery, the probe; KI-404), and 45 apart from each other. `bench
  check` in four alternating pairs: every row but the two descents within the pairs' spread, but for the space rows and the telescope,
  which the four F-03 runs and the one-thread pair drew 20-33% slower; the Mac was on battery at 22% and no F-03 code runs there (the
  view's init and fill run at a landing), and the space rows alone alternated eight times came out within 1.5% (4x 14.13 against 14.25
  ms): a clean batch on mains power was left open, and run on 2026-10-14 (four pairs against F-02's binary): the space rows and the
  telescope within 1% (space 4x 14.09 against 13.97 ms), the descents 39.8 → 23.2 and 77.7 → 47.4 ms. New: `SurfaceView::fillMisses`, `FILL_SERIAL_MAX`, `buildMesoTiles`, `lastWarmMisses`, `testRings`,
  `drainageSink`, `tileHash`, the tile's `sampleMs` and `sortMs`, `Neigh::detail`.

## F-04 (2026-10-14): the terrain rings

The fourth milestone of the performance plan (`docs/ideas/PLAN-performance.md`): every band thread walked every cell of every ring
and set up every wet cell's water, so the terrain section cost the same with eight threads as with one. Nothing drawn changes: every
picture is the previous binary's, with the workers and with one thread.

* **The water corners once per vertex.** A wet cell's sheet made each of its four corners (the place in view, the depth, the glint's
  `pow 28`, the waves' two sines, the Fresnel `pow 4`, the fog's `exp`, the rain's wall) in each of the four cells that share it, and
  in every band, since the band cull spared wet cells: on the bench's sea view 1.4-2.5 ms of every band's 2.7-5.8. The corner's code
  moved verbatim into `waterCorner(x, z, wl, hy)`; `vertexOf` keeps the corner with its vertex (`VtxCache::wv`) when the vertex has its
  own level (on a world with water), from the stored floats the cells read; a corner without one takes the cell's mean level and is
  made by the cell, as before. The frame's wind direction (its sine and cosine in one statement, as the cell had them) and the sun's
  rise are taken once in `render`.
* **The warm's vertices by rows of vertices.** The vertex pass warmed the cells' bottom corners on the pool by rows of cells, then
  walked every cell of every ring again on the main thread for their top corners. One `parallelFor` over the 2r + 1 rows of vertices
  now: the thread of row vz warms the corners on it of cells (cx, vz) and (cx, vz - 1) when either passes the culls, the same vertices
  as before, each written by one thread. `vertexOf` looks its cell up after its hit test (a current vertex's cell was sampled when it
  was made).
* **The fill reaches what the vertices read (a race found by TSan).** A corner reads its neighbours for its normal and for its side of
  the water's edge (`vertexShore`, the eight round it when it has no level of its own), so a drawn cell's corners read cells up to 2.83
  cells from its centre, and on a lava world the near rings' glow two cells round a corner (4.24); the warm's disc reached two (O6-06),
  so a rare cell at the disc's edge was sampled inside the parallel vertex pass, where two threads could sample it into one slot. With
  the top corners on the pool it was no longer rare: TSan reported it in each of six scenes. The disc now reaches three cells past the radius
  (four and a half on a lava world's near rings) and every ring's square of 2r + 5 cells still fits its torus (the unit's check
  tightened from 2r + 3); TSan clean on eight scenes after (a lava world and the forest among them).
* **The cells binned once.** After the warm, `binTerrainLOD` runs on the pool by rows of cells, every ring's rows in one dispatch: the
  culls each band thread ran (the distance, the cover, the frustum, the four corners behind the camera), the corners (vertexOf's hit;
  a corner the warm missed is counted, `lastStaleCorners`, and the unit wants none), and the rows each piece of the cell reaches: the
  ground's two triangles or the pieces of its shore cut, the outermost ring's walls, the water's sheet, each projected as the
  rasteriser projects it and clipped at the near plane as `rasterPolygon` clips it, a row of slack each side; a piece wholly beside,
  above or below the frame adds nothing (the rasteriser rejects it), a cell none of whose pieces reaches the frame is not listed, and
  the foam's sparkles put a cell in every band. A ring's list keeps the walk's order (`TerrainBin`, a row of cells to a slice, 12 bytes
  a cell); a band thread draws, in that order, the cells whose rows meet its own, reading the corners from the vertex cache (no band
  thread writes a cache). Each pixel is written by the same cells in the same order as by the serial loop, whatever the band edges
  (B-310's contract), so no band layout has to be shared between the bin and the draw (the plan's "one source of truth" is not
  needed): the bin knows rows, not bands. The pieces' points that are not corners come from helpers the bin and the draw share
  (`cellWaterLevels`, `cellShoreSplit`, `shoreCutView`, `skirtFoot`). Bounding a cell by all its points at once was too loose: over a
  deep sea the bed lies thousands of rows under the sheet (the bench's view: 3,135 of 3,567 cells in the bottom band); by piece the
  same view lists 3,194 cells in 0.11 ms, none in every band.
* **The numbers** (the medians of four alternating `bench check` pairs against F-03's binary, on mains power). The terrain section at
  4x 5.2 → 1.2-1.6 ms (the plan's estimate 2.3-3.7, then ~2 with the chunks), at 2x 4.2 → 0.7-0.8 (~2); surface 4x 8.93 → 4.85 ms, 3x
  6.77 → 3.10, 2x 6.56 → 3.09, 1x 5.28 → 1.83 (section 2's targets for surface 2x, 4 ms, and the buggy, 8, met; surface 4x's 4.5 nearly);
  the landing's first frame 5.30 → 1.75, the descent's 22.95 → 19.85; buggy 2x 8.79 → 5.12, 4x 11.88 → 7.76; drone 2x 7.35 → 3.38;
  forest 2x 10.00 → 8.04, 4x 15.35 → 13.21; city 2x 6.25 → 5.62, 4x 9.89 → 9.30; capital 2x 6.13 → 5.66, 4x 8.98 → 8.21; sunward 4x 12.62
  → 10.64, sunaway 10.38 → 8.00, the sprint 10.19 → 7.64; every row away from the ground within the pairs' spread. The 4-to-8-thread
  slowdown is gone (surface 1x with 1, 2, 4, 6 and 8 threads 5.32 / 4.56 / 3.80 / 5.30 / 5.28 → 3.98 / 2.90 / 1.97 / 1.89 / 1.77 ms), and
  one thread gains too (surface 4x 17.12 → 15.82, 1x 5.28 → 4.00, buggy 4x 23.05 → 19.31). The warm stays at 0.8 ms at 4x: the water
  corners moved into it, the serial walk left it. The flow 34.2-34.8 → 28.1-28.3 s. A scene's peak footprint 70.9 → 76.0 MB (the water
  corner in every vertex of the five caches).
* **Not built.** The adaptive row chunks (the plan's fourth item): they need F-06's dealing primitive, and the bands now show the
  skew they are for: at 4x on the bench's sea the horizon's band (rows 400-500) draws 3,193 of the 3,194 cells in 1.28 ms, the three
  sky bands nothing, the bands under the horizon 215-514 cells in 0.76-0.84 ms (the near water's pixels).
* **Found on the way: the sun's loop and the stack** (KI-409). The one-thread pairs drew the sunward, sunaway, sprint, city 4x and
  forest 2x rows up to 2.4 times slower with F-04's binary (sunward 26.6 → 62.6 ms, all of it in the sky's section), but only when the
  run was a zsh background job whose one added variable was `VESPERIS_THREADS=1`: six such runs of F-04's binary slow, none of F-03's
  four, and the same binary through `env`, with one more variable, under `/usr/bin/time`, under a pty or in the foreground 25.7-26.2 ms.
  A profile of a slow run puts five times the samples in `drawSun`'s per-pixel lambda (the sky's own loop the same): its captures are
  read through the closure from `drawSun`'s frame while it stores pixels, so the main thread's stack alignment decides it. Nothing in
  F-04 runs there; F-05's sun item or F-07 takes it.
* **What moved.** Nothing drawn: the regress's 22 hashes, with the workers and with one thread; the 67 scenes at 2x and 4x the
  previous binary's, and at 4x the same drawn by one thread in fresh runs; `VESPERIS_SKIP=wq` and `cut` the previous binary's frames
  (the bin lists a superset, the draw skips); `unit` 327 checks (one new: four scenes at 1x, 2x and 4x drawn with one, three and five
  threads the same as with eight, no corner unwarmed), 0 failures; the ring seams 0.0000 m; `capsule` 0.00% at every height; TSan
  clean; the flow from one restored state: F-04's two runs identical, F-03's two 45 pictures apart (KI-404), F-04 against F-03 the
  same 45 and `flow_shipscreen.png` (the known race); the 13 bench pictures that hold still between runs (the surface's four, the
  forest, the buggy, the drone, the city's two, the sunward view, the probe, the belt, Vimana) the same by both binaries and by F-04's
  one thread. New: `waterCorner`, `VtxCache::wv`, `binTerrainLOD`, `TerrainBin`, `TerrainCellRef`, `lastTerrainCells`,
  `lastTerrainBinMs`, `lastTerrainAllRows`, `lastStaleCorners`, `shoreCutView`, `skirtFoot`, `cellWaterLevels`, `cellShoreSplit`.

## F-05 (2026-10-14): the passes one thread drew alone

The fifth milestone of the performance plan (`docs/ideas/PLAN-performance.md`): beside the parallel passes, the main thread drew the
cabin, the screens' dimming and blits, the sun's veil, the camera feeds, the sky's cloud and zodiacal grids, the far floor and the
reflections alone, and walked the whole frame for a sun out of view. Nothing drawn changes: every picture is the previous binary's,
with the workers and with one thread.

* **The cabin in bands.** `drawCabin` takes what the frame shares once (`CabinFrame`: the eye, the light, the screens' flickers, the
  decoder's wait, which builds a map and a set a call, and the rack's three drones, their places, poses and models: `droneRackFrame`,
  `drawDronePrims`), then the room's ~150 pieces go to the pool in bands of rows (`drawCabinBand` under `setRasterBand`, B-310's rule:
  every band walks every piece in the serial order, its fills clipped to its rows). The roof's early return leaves `drawCabinBand`,
  not the band; the bulb's and the orb's points and glows come after the bands, last as before (`glowDisc` is not clipped to a band);
  the screens' `rs.bank` is each band's own. The space rows: 4x 14.09 → 10.62 ms, 3x 7.85 → 5.98, 2x 4.14 → 3.20, 1x 1.60 → 1.46 (the
  plan's estimate 13.6 → ~10.8); the belt 2x 2.02 → 1.17, Vimana 2.21 → 1.47, an arrival's frames 10.97 → 5.92.
* **The screens' dimming and blits.** `blendRectRGB` puts its rows on the pool from 100 rows (a dimmed screen: 800 at 4x; the status
  bar and the thousands of small blends stay direct calls); its three statements are verbatim (`(c - p) * a / 255` is a signed
  truncating division). The landing map's whole-world blit and its zoom go to the pool by rows, a texel's colour made once for the
  pixels that share it: along a row when the texel changes, and a row whose texels the band's row above had is copied from it (each
  band starts afresh, so no band reads another's row; at 1x the skipped texels are never made, at 3x the shared ones alternate as
  before). The sector map takes each pixel column's image column and each pixel row's image row from the per-pixel expressions into
  two tables, then its rows go to the pool. A blend that may dispatch must not run inside a pool job (the pool is not reentrant):
  every RGB pass runs on the main thread after the mush, and no pool job or game thread calls one (read). The decoder 4x 13.25 → 7.50
  ms (the cabin and the dimming), the landing map 4x 7.20 → 4.47 (one thread 20.19 → 17.38), its zoom 7.81 → 4.97, the sector map 9.87
  → 8.34.
* **`glowDisc`.** Nothing at all when `maxAdd` is 0 or less (every pixel's add would be 0: a dim veil walked a disc of 0.9 M pixels at
  4x), and the rows on the pool (a disc under 64 rows stays on the caller); its ten callers are all on the main thread outside a pool
  job (read).
* **The camera feeds by rows.** The nose camera's and the radar camera's noise is a xorshift32 (13, 17, 5) walk along the rows, which
  pinned both to one thread. The step is linear over GF(2): a row's FBW steps are a 32x32 bit matrix, kept by its 32 columns (the
  images of the 32 bits; `XorshiftJump`, made once per width by stepping each bit FBW times), and `xorshiftRowStarts` gives every
  row's starting state, so the rows go to the pool and each band starts from the state the serial walk reaches at its first row: every
  pixel draws the number it drew (the unit checks the starts against the walk at the four widths). Re-seeding each row from a hash
  would have been five lines and another speckle. The radar's interpolated glow row is each band's own. The buggy 4x 7.80 → 5.92 ms
  (the feed, the sky's grids and the sun's pass), 2x 5.12 → 4.65; the radar camera 4x 6.69 → 5.03.
* **The sky's grids.** The cloud grid (three `fbm2` and the front's reads a cell, 2,000-2,400 cells above the horizon at every scale)
  and the zodiacal grid go to the pool by rows, as the aurora's did: each cell reads pure functions and writes its own floats.
* **The floor and the reflections in the terrain's bands.** `drawFloor` is drawn first in each band (it was drawn before the rings);
  `drawReflections` is split into `gatherReflections`, serial after the bin (the walk, the sort and the shore cut: during the descent
  the lod0 cells it reads are not warmed and the caches fill on a miss), which keeps each polygon with the rows it can fill
  (`polygonRows`: F-04's piece bounds, now one function for the bin and the gather), and `drawReflectionBand`, which each band runs
  after its rings, in order. The section `terrain` now holds the bin, the gather and the bands; `warm` the warm alone. The forest 4x
  13.32 → 11.80 ms (at the game's 3x3 mush 12.47 → 10.96), 2x 8.13 → 7.51 (the plan's estimate 0.55 ms; the cloud grid's share was the
  larger).
* **The sun's glow when its cone misses the frame.** With the sun beside or behind the camera (no box: the cone reaches the camera
  plane), the glow's pass walked the whole frame and wrote nothing. `coneMissesFrame`: every pixel centre lies more than theta -
  cornerA from the sun (theta its angle off the view's axis, cornerA the farthest pixel centre's; the angle between two directions is
  at least the difference of their angles to a third), so the pass is skipped when that exceeds the cone's half-angle by 1e-5 rad;
  likewise the protostar's strip (its cone of 0.6 rad) and the black hole's frame copy, bend grid and lens pass (its region's cone;
  the jet's glows stay). The box path is untouched. `suncone` sweeps the sun toward the frame's corner across the edge (61 steps of
  1e-4 and of 1e-6 rad round it, and 101 from half the cone inside to half outside, where the glow still shows) for seven classes,
  three sizes, with an atmosphere and without, at 1x and 4x: both binaries' 84 cases the same (one hash of all 18,732 frames). The sun
  facing away 4x 7.98 → 6.17 ms, the sprint 7.67 → 5.71, city 4x 9.27 → 8.94; facing it 10.68 → 9.04 (the veil's disc on the pool).
* **KI-409 closed.** The glow loop reads its values from its band's own locals (the same arithmetic): replayed in the launch that
  showed it (a zsh background job from the same directory, `VESPERIS_THREADS=1` its only addition, the binaries at paths of one
  length), F-04's binary drew sunward 4x in 61.7 and 62.1 ms (the sky 50.7 and 52.5), F-05's in 25.5 and 25.8 (the sky 16.2 and 16.1);
  sixteen alignments of the stack (a padding variable of 0-240 bytes, a one-thread `bench forest`) slowed neither binary.
* **The numbers** (the medians of four alternating `bench check` pairs against F-04's binary, on mains power): as above; every other
  row within the pairs' spread (the surface rows, the telescope, the descents, the probe 2x, capital 4x, city 2x, the guide). With one
  thread (`env VESPERIS_THREADS=1`, one run each): nothing slower; space 4x 41.12 → 39.92 (the serial path pays nothing for the
  dispatch), the landing map 20.19 → 17.38 and its zoom 22.53 → 20.04 (the texel's colour once), sunaway 19.74 → 18.32, the sprint
  19.85 → 18.33, city 4x 22.23 → 20.84, probe 4x 35.37 → 33.37 (the cone test). The flow 27.8 → 24.2 s. The recorder's frame at 4x
  9.46 → 7.13 ms, its writer now setting the pace (the main thread waits 0.6-2.1 ms a frame for the queue, 0.1 before). Left: the
  cabin's hull fills the frame's top and bottom bands, which the static bands cannot balance (F-06's dealing).
* **What moved.** Nothing drawn: the regress's 22 hashes, with the workers and with one thread; the 67 scenes at 2x and 4x the
  previous binary's, and at 4x the same drawn by one thread in fresh runs; `screens` (96 pictures: the cabin's ten views at the four
  scales, its menus, the radar camera, the landing map and zoom, the drone bay, the descent, the ground, the menu, the sector map near
  and wide, the nose camera) the previous binary's set and the same with one thread (two runs of the baseline identical); `suncone`
  the same; `unit` 328 checks (one new: the feeds' row starts), 0 failures; TSan clean (`screens` at 2x, eight scenes at 4x: the
  shore, the forest, the zodiacal light, a front, a sunset, a protostar's night, a black hole's sky, the buggy's seat); the flow from
  one restored state the same 163 pictures as F-04's (F-04's twice), its wait for the scanner's thread held at one instant (frames at
  dt 0: frames of 1/30 s had put F-05's faster clock, and the 47 pictures after it, ahead); the 13 bench pictures that hold still
  between runs the same in all four pairs. New: `CabinFrame`, `drawCabinBand`, `droneRackFrame`, `drawDronePrims`, `xorshift32`,
  `XorshiftJump`, `xorshiftRowStarts`, `coneMissesFrame`, `gatherReflections`, `drawReflectionBand`, `polygonRows`, the harness's
  `screens` and `suncone`.

## F-06 (2026-10-14): the pool deals its rows

The sixth milestone of the performance plan (`docs/ideas/PLAN-performance.md`): `parallelFor` cut [0, n) into one equal band a thread
and gave band t to thread t for good, so an efficiency core at about 0.55x held up every pass it had a band of, and a pass whose rows
differ in cost (a globe's middle rows, the ground's horizon, a town on the skyline, the cabin's hull at the frame's top and bottom)
waited for its heaviest band. Nothing drawn changes: every picture is the previous binary's, with the workers and with one thread.

* **The dealing.** A dispatch cuts its range into chunks, a quarter of a thread's share and at least half its `minPerThread`
  (`parallelFor`'s new `chunk` sets it), which the caller and the workers take from an atomic counter until none is left
  (`core/parallel.cpp`). The caller starts at once and a worker joins as it wakes (a kernel wake is 10-30 us). A dispatch completes by
  its work: the caller waits only for the workers that entered, and a worker that wakes after the body closed never touches it (the
  close and the entry are one store-and-load pair, sequentially consistent: the caller sees the worker counted or the worker sees the
  body closed), so a descheduled worker cannot hold up the frame. The workers wake on one condition variable: a `notify_all` costs the
  caller 14 us after a rest, seven `notify_one`s 28-31 (F-05's pool 43-45; back to back 1.3 us against 34). `parallelBands` deals its
  exact bands the same way (the flora's layout stays B-423's); the probe's own counter (`dealt`) is the pool's dealing one column a
  chunk; the plate's blocks are dealt 64 at a time (a block costs a sample, a copy or nothing; the same blocks land in the same
  frames).
* **The band-raster passes.** The terrain and the cabin take the default chunks (thirty-odd at 4x): eight dealt bands were slower
  (space 4x 9.60 against 9.99 ms, forest 4x 10.61 against 11.41, city 4x 7.70 against 8.47, the decoder 6.29 against 6.84), and so
  were sixty-four at 1x-2x (surface 1x 1.55 against 1.74). The forms take chunks of eight rows or a sixty-fourth of the frame (a chunk
  costs them a test per polygon and a town fills a few rows: city 4x 7.16 → 6.83 against the default chunks, 2x within the spread).
* **The terrain's chunks by cost**, as the plan proposed (each cell 1 + 4 wet a row over its rows, 4 x threads chunks of equal cost),
  lost: surface 4x 4.31 → 5.09 ms, forest 4x 10.74 → 12.21. The per-row model put the near ground (few cells, many pixels) into one
  chunk of 1.97 ms and cut the horizon's thousands of small cells into chunks a row or two high that set them up again each. With the
  equal chunks the horizon's is the longest (1.43 ms of the forest's 1.8 ms section). Not built.
* **No spin.** Workers kept awake 50 us after a job (on the pause hint, then in the M4's low-power wait: `ldaxr` and `wfe`, woken 0.2
  us after the write, every 1.2 us without one) won 0.15-0.18 ms on the surface at 1x-3x and lost 0.1-0.4 on every space frame and at
  4x, the default; 15 us kept the loss and lost the gain (four pairs each). Each thread's own stretch of rows first, then the others'
  (the old bands' cache affinity), was no better either (space 1x 1.42 → 1.50, surface 2x 2.36 → 2.59, city 4x 6.77 → 6.56). The
  workers sleep between jobs.
* **The guard.** A dispatch from inside a dispatch runs alone in one call (`pool::inside`, a thread-local flag on the caller during
  its dispatch and on every worker), and a second thread's while the pool is held runs alone too (`try_lock`, counted by
  `pool::contended`, which the unit wants at 0: only the main thread dispatches). Before, either hung the pool.
* **The cap stays at eight.** Ten threads on the M4's ten cores won 0.53 ms on space 4x and 0.94 on the telescope 4x, and a recorded
  frame at 4x went 7.7 → 14.5 ms: the PNG writer's thread starved by a pool on every core, as B-422's prefetch had been. Two cores
  stay for the game's own threads (the writer, the drainage lanes, the ahead worker, the synth).
* **The numbers** (the medians of four alternating `bench check` pairs against F-05's binary, on battery): space 4x 10.65 → 9.31 ms,
  3x 5.95 → 4.89, 2x 3.19 → 2.91, 1x 1.46 → 1.32; the telescope 4x 24.44 → 23.05 (2x within the spread); surface 4x 4.81 → 4.18, 3x
  3.06 → 2.56, 2x 3.12 → 2.46, 1x 1.78 → 1.67; forest 4x 11.89 → 10.60 (11.03 → 9.46 at the game's 3x3), 2x 7.50 → 6.73; city 4x 8.93
  → 6.69, 2x 5.55 → 4.50; capital 4x 7.94 → 6.51, 2x 5.38 → 4.31; buggy 4x 5.96 → 5.07, drone 2x 3.09 → 2.84; sunward 9.04 → 7.18,
  sunaway 6.24 → 4.96, the sprint 5.71 → 4.68; the decoder 7.32 → 6.12, the sector map 8.12 → 6.57, the landing map 4.47 → 4.19 and
  its zoom 4.99 → 4.59, the radar camera 4.84 → 4.35; probe 4x 10.13 → 9.47; an arrival's frames 5.87 → 5.06 and its first 15.15 →
  13.10; the descent's first frame 19.70 → 18.70. With one thread (`env VESPERIS_THREADS=1`, one run each, two batches) every row
  within the runs' spread. Two rows the other way: the buggy 2x row 4.49 → 5.31 (its first ten frames run beside the drainage work of
  the drive's relocation, 10-17 ms with the dealing against 7-10, everything in them slow at once; its median frame 3.9 against
  4.3-4.9, and paced at 60 a second while driving, three alternations, p50 5.2-5.3 against 5.8, p99 5.9-6.7 against 6.6-7.0) and the
  recorded frame 9.37 → 11.74 (both binaries' runs 7-16 ms: unpaced, the frames come faster than the writer encodes them, and the
  dealt pool leaves it fewer idle moments).
* **What moved.** Nothing drawn: the regress's 22 hashes, with the workers and with one thread; the 67 scenes at 2x and 4x the
  previous binary's, at 4x the same drawn by one thread and in fresh runs at two, three and five threads (66 scenes, `aurora_moon`'s
  five-minute search aside); `screens` (96) the previous binary's set and the same with one thread; `suncone` the same (one hash of
  its 18,732 frames); `unit` 329 checks (one new: `threads: the picture whatever the count`, six scenes at 4x with one, two, four, six
  and eight threads, and no dispatch from a thread beside the main one), 0 failures; the `pool` mode's stress, 0 failures; TSan clean
  (`pool`; `screens` at 2x; nine scenes at 4x, the shore, the forest, the zodiacal light, a front, a sunset, a protostar's night, a
  black hole's sky, the buggy's seat and the modern town, with the pool's threads and with two: 20 runs, no warning); the flow from
  one restored state the same 163 pictures as F-05's; the 13 bench pictures that hold still between runs the same in all four pairs;
  `capsule` 0.00% at its eight heights. New: `core/parallel.cpp` (the pool: `pool::dispatch`, `pool::run`, `pool::inside`,
  `pool::contended`), `parallelFor`'s `chunk`, `POOL_MAX_THREADS`, the harness's `pool` mode; gone: `WorkerPool` and its
  `std::function` jobs.

## F-07 (2026-10-14): exact trims

The seventh milestone of the performance plan (`docs/ideas/PLAN-performance.md`): small changes in the hottest loops, each giving the
bits it gave before, so the pictures, the sounds and the generation are the previous binary's, with the workers and with one thread.

* **The globe's lookups.** `wrap2pi` returns a longitude within a turn of its range without `fmod` (the same bits: `fmod` returns
  `[0, TAU)` as it is, `a - TAU` over `[TAU, 2 TAU)` is exact by Sterbenz, `(-TAU, 0)` has `TAU` added either way; `-0.0`, NaN and
  the infinities fall through); the unit sweeps 2.2 million values and the turns' edges against the formula it had, bit for bit.
  `PlanetMap`'s lookups (`texelIndex`, `albedoAt`, `cloudAt`, `heightAt`, `materialAt`, the bilinear read) are inline in
  `planetmap.h` (the globe reads seven a pixel, each a call into another file), `TerrainCache::at` inline in `site.h` with its miss
  out of line (`fill`), `giantJetTurn` inline in `probe.h`.
* **The globe's pixels.** A moon or the parent whose shadow cannot fall on a lit point that frame is left out of the pixels' loop: its
  shadow's cylinder passes wide of the globe (a point's distance from the cylinder's axis is at least the centre's less R) or it is
  behind the globe from the star and clear of it (every lit point's `along` is under the centre's), with a slack (a hundredth of its
  radius and a millionth of the distances) far over the rounding of either side; a scratch test of 400,000 systems from 10^4 to 10^10
  km, grazing and edge-on, found no lit point a dropped body would have shaded. The plate's cloud turn is a sine and a cosine a plate
  a frame where the pixel has no jet (the angle as the pixel wrote it, `0.0 + drift - drift0`); the limb's `pow` runs on the lit side
  only (its term is +0 under `lit + 0.2 <= 0`); an opaque deck's and a giant's ground exposure, overwritten by their clouds', is not
  made. The plate's cores test the cosine of their reach before the `acos` (`cosReach - 1e-9`, the angle's margin far over the
  rounding).
* **The mush.** A pass reads the frame and writes `Framebuffer::back`, the two swapped after it (no copy of the frame), its rows on
  the pool, k = 2's too (rows independent; the last two copied as they were). A larger box slides the column sums of its k rows down a
  band of rows a row at a time and a k-wide window along them (the row under the pixel giving the bank and leaving the sums in one
  read), and divides by a multiply and a shift (`m = 2^32 / divisor + 1`, exact while sum x divisor < 2^32: every kernel to 38). A
  scratch oracle, F-06's mush verbatim, gave the same frames on 576 random frames (every scale, kernels 1-8, one to three passes, one
  thread and eight).
* **The rasteriser.** The grain's two divisions come after the band's early-outs; a span at one depth (`diz == 0`: a leaf cluster, a
  lily pad, a blob shadow) divides once; a polygon or triangle with no row in the thread's band returns before its fan (its triangles'
  rows lie within its own, and `ceil(y) > n` exactly when `y > n`); the vertices are sorted by reference; a polygon wholly in front
  projects its first 15 vertices in place instead of copying them through the clip (the clip keeps 15). Then, judged by its own four
  pairs: the opaque replace inline and the other blends and the quantized shade out of line (`emitPixelRare`), and the textured loop
  writes a pixel without testing its depth again (its float test passed, so the double one would: rounding to a float is monotone);
  forest 4x 9.25 → 8.84 ms, city 4x 5.76 → 5.41, the sprint 3.88 → 3.62, surface 4x 3.51 → 3.42, every row within the spread or
  faster.
* **The sky.** The horizon's `pow(.., 2.2)` is 1 under the horizon; the suns' glow `pow`s run where the sun is up and the pixel faces
  it, the airless world's `pow 300` and the comet's tail where their bases are over 0 (elsewhere each term is +0); the ring arc's
  `pow` is a table of the profile's 256 densities a frame; the 22-degree halo's `acos` and the pillar's `atan2` and `asin` wait for
  pre-tests on the cosine, the height and the azimuth's tangent a thousandth of a degree outside the tests after them.
* **The noise.** `grad3` picks its gradient by Perlin's selects instead of a jump on a random hash (the same sums: `a - b` is
  `a + (-b)`), and the 27-cell walks of `worley3` and `visitFeatures` mix each x and each (x, y) once (`hashX`, `hashXY`, `hashXYZ`,
  `hash3i` their composition). `bench noise`, the same results' hashes: `gnoise3` 88.9 → 11.2 ns a call, `worley3` 230 → 187, the
  features' walk 510 → 378.
* **The probe's steps.** A panorama's march distances (`pow(1 + STEP, ..)` a step of every column) are a table made once before the
  columns are dealt (`marchSteps`), up to the first at its reach.
* **The synth.** The speech's, a phantom's and the medium's constants (the smoothings' `1 - exp(-dt / tau)`, the fixed bandwidths'
  radii, the medium's poles) are made once a block in `render` (`AudioSynth::Coefs`) by the expressions that made them every sample;
  the 87 WAVs of `audio`, `decoder wav` and `signals wav` the previous binary's byte for byte.
* **The flora.** The cells' loop is timed whole (a clock read per cell and part was ~2,700 a frame; `lastFloraMs` keeps the trees and
  the cells), and the cells after the trees mark nothing in the coverage grid only the trees read.
* **`ifloor`.** `ifloorTrunc` (the truncation less one where it rounded a negative non-integer up: `std::floor`'s integer for every
  input in range) stands in for `std::floor` where the target has no rounding instruction (baseline x86-64, the Linux and Windows
  zips, where `std::floor` is a library call); arm64 and SSE4.1 keep `std::floor` (the truncation's form measured 11.3 → 12.5-13.2 ns
  a `gnoise3` on the M4). The unit checks both against `std::floor` (2 million values and the integers' neighbours to 2^40).
* **The backdrop kept while the camera is still.** Outside a flight and with no nebula patch the band's layer is a function of
  `BackdropKey` (the camera, the focal length and centre, the frame's size, `bandGen`, which both writers of the band map move on): a
  frame whose key matches the last one's keeps its layer (2 MB at 4x) and later frames with that key copy it instead of clearing and
  drawing. The sampler takes the map's size as a constant and wraps the longitude by a mask. The key's completeness is the contract
  (`02-rendering.md`).
* **The numbers** (the medians of four alternating `bench check` pairs against F-06's binary, on mains, the Mac swapping 11 of its 13
  GB and the system's media analysis, Spotlight and Docker's VM busy beside the runs): space 4x 9.43 → 6.86 ms (the plan's target 7),
  3x 4.97 → 3.71, 2x 2.95 → 2.18, 1x 1.37 → 1.02; the telescope 4x 23.06 → 18.37, 2x 10.30 → 9.01; surface 4x 4.18 → 3.54, 3x 2.58
  → 2.33, 2x 2.47 → 2.29, 1x 1.67 → 1.55; forest 4x 10.37 → 8.95 (9.49 → 8.39 at the game's 3x3; the target 10), 2x 6.88 → 6.28;
  city 4x 6.79 → 5.53, 2x 4.46 → 3.87; capital 4x 6.47 → 5.42, 2x 4.38 → 3.88; buggy 4x 5.06 → 4.12, 2x 5.15 → 3.66, drone 2x 2.80 →
  2.25; the belt 1.14 → 0.61, Vimana 1.42 → 1.31; probe 2x 6.72 → 5.97, 4x 9.45 → 8.46; the decoder 6.03 → 3.75, the landing map 4.14
  → 2.28 and its zoom 4.53 → 2.62, the radar camera 4.35 → 2.46, the sector map 6.55 → 5.54; sunward 7.18 → 6.17, sunaway 4.93 →
  3.87, the sprint 4.69 → 3.71; an arrival's frames 4.97 → 4.38, its first 13.40 → 12.45; the descent's first frame 18.95 → 17.65.
  With one thread: space 4x 40.37 → 28.80, the telescope 4x 101.65 → 80.63, surface 4x 15.71 → 13.17, forest 4x 38.05 → 33.65, city
  4x 20.67 → 16.84, the landing map 17.31 → 9.05, the decoder 23.26 → 14.23, the radar camera 18.28 → 9.87. By step (four pairs each
  against the step before): the globe's lookups and pixels space 4x 9.43 → 8.73, the telescope 4x 25.38 → 23.05, surface 4x 4.43 →
  4.14 (`TerrainCache::at`); the mush and the rasteriser's first trims surface 4x 4.10 → 3.57, forest 4x 10.59 → 9.86, space 4x 8.73
  → 8.16, the sector map 7.00 → 6.08; the sky, the noise, the probe's steps, the synth and the flora the telescope 4x 20.52 → 18.01
  (the plate's samples), probe 2x 6.69 → 5.90, forest 4x 9.86 → 9.18, the buggy's 2x 5.13 → 3.81, the drone 2.74 → 2.26; the backdrop
  kept space 4x 8.21 → 6.96, the landing map 3.33 → 2.33, the radar camera 3.68 → 2.45, the decoder 5.63 → 4.06 (each over a still
  cabin view), capital 4x 6.11 → 5.48 (the sky's band read by the sampler's constants); the rasteriser's second step as above. The
  recorded frame 9.52 → 8.88 and a photo's PNG the same (both set by the PNG writer's pace, 7-16 ms run to run).
* **What moved.** Nothing drawn or heard: the regress's 22 hashes, with the workers and with one thread; the 67 scenes at 2x and 4x
  the previous binary's, and at 4x drawn by one thread; `screens` (96) the previous binary's set and the same with one thread; the
  `compare` sheets (the mush's kernels 3, 4 and 5 at 2x-4x) and the probe's frames and sheets (`probe`, `probe stability`, `probe
  giants`) the previous binary's; `suncone` the same; `bench noise`'s hashes; the 87 WAVs; `unit` 332 checks (three new: the turn, the
  floors, the quotient), 0 failures; TSan clean (`screens` at 2x; seven scenes at 4x with the pool's threads and with two, two at 1x;
  the probe's panoramas); `capsule` 0.00% at its eight heights; the 13 bench pictures that hold still between runs the same in every
  pair; driving paced at 2x (`load drive 12 2`, three alternations) p99 9.1-10.1 ms against 10.5-11.2. The flow from one restored
  state: 162 of 163 pictures the same; `flow_probe_aim.png` had the gas giant's map at the shot with F-07's faster noise and not with
  F-06's (the flow waits for a landable world's map only; the plate's arrays were the same in both, and the shot the same once both
  waited), and `flow_space.png` varied run to run with one body's map (its readiness printed at the shot: the picture followed it).
  The flow now waits for the starting system's maps and the giant's (`Game::testWaitMapOf`), and with those waits F-06's binary and
  F-07's draw the same 163 pictures in every run (24.4 → 20.7 s). The unit's C-11 player check read its piece before the piece's
  records were written on their thread on a loaded machine (KI-410): it waits for them now at one instant (`testPlayerWaiting`). New:
  `ifloor`, `ifloor64`, `ifloorTrunc`, `hashX`, `hashXY`, `hashXYZ`, `Framebuffer::back`, `TerrainCache::fill`, `emitPixelRare`,
  `marchSteps`, `AudioSynth::Coefs`, `SpaceRenderer::BackdropKey` and `bandGen`, `PlanCore::cosReach`, `Game::testWaitMapOf`,
  `testPlayerWaiting`; `sampleGalaxyBand` without its size; `lastFloraMs` two sections.

## F-08 (2026-10-14): the frames' pacing

The eighth milestone of the performance plan (`docs/ideas/PLAN-performance.md`): the platform layer. F-01's window had dropped the
plan's audio thread (the synth's block 0.76 ms at most) and its unpack ring (the upload 0.4-0.5 ms) and left the pacing (KI-407); the
window was measured again here, its work half of F-01's since F-05 to F-07. Nothing drawn or heard changes.

* **What raylib's pacing did.** `SetTargetFPS(60)` waits after the swap for 1/60 s less the frame's time, asleep for 95% of it and
  spinning the rest; macOS wakes the sleep late (by about half a millisecond), so with 6-7.5 ms of work the frames came every 17.2 ms,
  58 a second, in every state, against a display that refreshes every 8.33 ms (the dev Mac's 120 Hz ProMotion panel: two refreshes a
  frame). With the swaps' returns written down (on the vsync a swap returns at the refresh its frame met) 93-97% of the frames reached
  the screen 16.7 ms after the one before in the calmest run (the decoder 69%) and 57-94% in the busier ones, and 2.0-3.4% stayed on
  it for 33 ms in every state. F-01's 33 ms frames in the cabin came from its 13.4 ms of work; with the work at 7 ms the late wake is
  what is left.
* **The modes tried** (a scratch copy of the window loop driven through seven states by script, F-01's with a turning cabin added, the
  pacing chosen by an environment variable, each frame's parts and its swap's return written; muted, 4x in a 1280 x 800 window): the
  vsync alone (the plan's experiment, `SetTargetFPS(0)`) lets the panel run between 60 and 120 Hz with the game: 71 frames a second in
  the quiet cabin, 31% of them 8.3 ms apart and the rest 16.7 (dropped: a judder, and a frame rate that is the display's). raylib's
  wait without the vsync paces as with it (17.2 ms). A 60 Hz schedule kept by the platform, each swap held until its frame's refresh
  less a margin and the schedule following the refreshes the swaps met: with 2 ms of margin every frame met its refresh while the Mac
  was calm (100% in two-second windows), but under load the compositor wants a frame earlier and whole stretches missed (33 ms apart:
  the decoder 41.7 frames a second, the walk 47.6); with 4 ms (60.0, 59.8 and 59.7 a second in three states) and 6.25 ms it held.
* **The platform paces the frames** (`platform_raylib.cpp`; raylib's `SetTargetFPS(0)` on the desktop, the browser's pacing as it
  was). A frame is meant for the refresh a whole number of the monitor's refreshes after the last one's (`floor(hz / 60 + 0.2)`: two
  at 120 Hz, two at 144 (72 a second), one at 60, 75, 90 or 100; the monitor's rate read every 60 frames, 60 when it reports none),
  and its swap waits until three quarters of a refresh before that one: past the refresh between, where the frame must not show, and
  early enough for the compositor. The wait sleeps until a millisecond before and spins the rest. A swap that waited for the display
  returned at its refresh, later only by the thread's wake: the schedule follows a return earlier than it expected, or half a refresh
  or more late (a refresh missed), all the way, and one a little late a tenth of the way (the display's clock against the CPU's,
  without the wakes' noise); a swap that did not wait (no vsync, a hidden window) leaves the schedule to the clock, and a frame a
  whole period late starts it again.
* **The numbers** (round three: raylib's pacing and the platform's alternated twice, 30 s a state, the Mac swapping 12.7 of its 13.3
  GB with a browser's GPU process busy beside the runs; on the beat = within 1.6 ms of 16.7 after the previous frame's refresh). On
  the beat, raylib's two runs against the platform's: the quiet cabin 86.3 and 94.3% against 91.1 and 99.5%, the cabin turning 82.9
  and 77.8 against 92.9 and 99.7, the ship's player 73.4 and 93.5 against 96.3 and 99.7, the radar camera 57.0 and 92.0 against 95.9
  and 98.4, the decoder with a piece playing 85.1 and 72.1 against 98.2 and 99.6, walking 80.0 and 85.8 against 99.9, driving 75.1 and
  81.0 against 95.9 (the platform's second walk and drive came straight after the Mac woke from a lid closed for 17 minutes, 59.2 and
  73.5%, and are set aside); 56.4-58.3 frames a second against 59.3-60.0; frames that stayed 33 ms 2.0-3.4% of every state against
  0-0.5%. The tree's own build afterwards (the same driver on `platform_raylib.o`): 59.7-60.0 a second, 94.2-98.5% on the beat, 0-0.1%
  at 33 ms; `VESPERIS_TRACE=1 ./vesperis --frames 600` at the title: the intervals' p50 16.67 ms and p99 20.6, the hold 2.4-3.4 ms and
  the swap 6.3-7.2 a frame.
* **The plan's other two items stay dropped.** The synth's block is 0.08 ms quiet and 0.98 on average, 2.08 at worst, with the radar
  and the ship's player on the loaded Mac (F-01: 0.36 and 0.76), one block in 5.4 frames whose work is 6-7.5 ms of 16.7: no frame
  waits for it, so no helper thread (the synth stays on the main thread). The upload is 0.42-0.77 ms on average (1.17 in the radar
  camera on the loaded Mac): no frame waits for it either, so no unpack ring.
* **Machines that are not this one.** On a 60 Hz display a frame is one refresh, its swap's wait three quarters of a refresh before it
  (the work mostly past that point already) and the vsync paces as before, without raylib's late wake after it; at 144 Hz the game
  runs at 72 frames a second; without a vsync (a driver that forces it off) the schedule alone paces, exact to the spin. Windows
  sleeps to the millisecond (raylib sets 1 ms timers), so a sleep a millisecond or two late still lands inside the margin. None of
  these was measured (no machine): the release builds are the check (F-09).
* **What moved.** Nothing drawn or heard; the harness does not link the platform layer (`vesperis_test` is F-07's binary byte for
  byte, so its checks stand as F-07 left them). The window build: the pacing (`platform_raylib.cpp`), `PlatformPresentTimes::hold`
  (`platform.h`), and the trace's `hold` column beside the swap, its work without either, and its summary counting the intervals over
  17.5 ms (paced frames sit at 16.67, give or take a wake). KI-407 closed; the decoder's part of KI-406 too (6.6-7.0 ms of work in the
  window, 60 frames a second).

## F-09 (2026-10-15): the release flags

The ninth milestone of the performance plan (`docs/ideas/PLAN-performance.md`): the six release builds' flags, and their scripts in
the tree. Stopped midway by the user on 2026-10-14 and finished the next day. The Makefile stays at `-O2` (the edit loop: the bench
measures the code, not the flags).

* **The release builds are scripts** (`packaging/`). They had lived in a session's `/tmp` scratchpad, which macOS wipes, and each
  release rebuilt them from an old transcript. `toolchains.sh` fetches raylib 5.5 and llvm-mingw 20260922 against their SHA-256 into
  `build/toolchains` (or `VESPERIS_TC`) and builds raylib static for macOS arm64 and x86_64 (the Apple clang, macOS 11) and Windows
  x86_64 and aarch64 (llvm-mingw, the UCRT): 22 s. `build-mac.sh` and `build-win.sh` build one target each (15-25 s; the Windows
  binary imports only system DLLs). `build-linux.sh` builds one Linux target in an Ubuntu 20.04 container (`linux.Dockerfile`, the
  image `vesperis-build:<arch>` made on first use: glibc 2.31 and GCC 9.4, so the zips start on older systems) through
  `build-linux-inner.sh`, which builds raylib (X11 only) on first use and links the C++ runtime in; amd64 runs emulated on the Mac.
  `build-release.sh <version> <out dir>` runs the six (the Mac and Windows ones side by side, then the Linux ones one at a time:
  beside other work 1.3.0's Docker VM went away mid-build), makes no zip unless all six built, and zips each as 1.3.0 did (the binary,
  `README.md`, `LICENSE`, `vesperis_keys.example.txt`; `Vesperis.app` beside the macOS binary).
* **Thin LTO on the clang lines** (macOS and Windows, `-O2 -flto=thin`). clang decides `-ffp-contract=on`'s fusions per source
  expression before the link, so the link's inlining cannot fuse another multiply-add. On the Mac the LTO build drew the 67 scenes at
  4x, 2x and 4x on one thread, the 96 screens, the 48 scale sheets and the probe's 49 frames as F-07's final sets, with the regress
  both ways, `suncone` and `capsule` the same; the x86_64 build under Rosetta its regress both ways and 18 scenes at 2x as its own
  `-O2` build's. Time (four alternating `bench check` pairs on battery): space 4x 6.86 → 6.80 ms, the telescope 2x 8.86 → 8.61, probe
  2x 5.92 → 5.81 and 3.48 → 3.39, buggy 4x 4.17 → 4.05, every other row within 1%; with one thread the same (space 4x 28.8-29.0 in
  both). The plan's -0.8-1.7 ms at space 4x was measured before F-07, whose inlined lookups took it. Kept: never slower, the pictures
  identical, one flag. A first batch, run while the machine was still settling, read space 4x 11% faster; that was its `-O2` side's
  spread (6.90-8.15 ms against 6.80-7.02), and the batch above was run after it, the order swapped.
* **Windows takes the Mac's line and nothing more.** No Windows machine runs the harness here, so its zips are untimed and the plan's
  `-fno-math-errno` for them is not taken (KI-411). Its builds' one warning was B-424, and a scan for the narrowings only Windows has
  (`-Wshorten-64-to-32` on both targets) found the rest of it; the Windows line now compiles with that warning.
* **Linux: `-O3`** (`build-linux-inner.sh`). GCC 9's `-O2` does not vectorise and inlines only small functions (clang's `-O2` does
  both; `-O3` was noise on the Mac). In the arm64 container (GCC 9.4, Docker's 10 CPUs and 8 GB) four variants were built beside `-O2`
  and gated against `-O2`'s own `regress bless`: `-O2 -flto=8`, `-O3`, `-O2 -fno-math-errno` and the three together each kept the
  regress with the workers and with one thread, and drew the 67 scenes and the 24 screens at 4x as `-O2` did. Timed in a rotation
  (each variant's `bench check` in turn, three rounds, nothing else running), against `-O2` over the 39 steady rows (the geometric
  mean of the medians): `-O3` -4.9%, 22 rows faster in every round (surface 4x 3.92 → 3.66 ms, 2x 2.72 → 2.48, forest 4x at the game's
  3x3 9.08 → 8.66, city 4x 6.38 → 5.96, buggy 4x 4.94 → 4.71, the telescope 4x 21.17 → 20.59, the radar camera 2.81 → 2.55, the
  landing map 2.52 → 2.28, the belt 0.86 → 0.72, Vimana 2.11 → 1.89, space 2x 2.77 → 2.63; space 4x 7.05 → 6.97 within the spread) and
  two slower (probe 4x 10.96 → 11.28, an arrival's frames 5.58 → 5.92); `-flto` +1.2% (no row faster in every round, four slower);
  `-fno-math-errno` +1.1%, within the noise; the three together -6.6%. `-O3` alone is taken, the one flag that measured on its own:
  the combination's further 1.7% would bring in two flags that gained nothing alone. The amd64 container (emulated: not timed) passed
  the same gate for `-O3`: the regress with the workers and with one thread against `-O2`'s bless, and 18 of the scenes
  (`aurora_moon`'s five-minute search among those left out: the emulation is slow) and the 24 screens at 4x as `-O2` drew them.
* **The hashes by platform** (KI-014). In the arm64 container GCC blesses the tree's 22 hashes exactly, the Mac's; the amd64 one
  differs in six: the three system hashes the x86_64 Mac differs in too, and three frames (`scene_cratered_noon`,
  `scene_molten_night`, `space_S00`). Each line is gated against its own `-O2`.
* **Not taken**, as the plan said: `-O3` on the Mac (noise), PGO (six targets, a training run a build), the micro-flags (`-fno-plt`,
  `-no-pie`, `-fno-stack-protector`), a newer Ubuntu (glibc), `-march`.
* **The release, dry run.** `build-release.sh f09 <a scratch dir>` from this tree, nothing uploaded (2026-10-15): the six built in
  316 s (the Mac and Windows four side by side in 57 s, then Linux arm64 in 27 s and amd64, emulated, in 3 min 49 s, each building its
  raylib first), every log `exit 0`; the Mac and Windows lines print no warning, the Linux ones GCC's 31 (one-line `if`s' misleading
  indentation and two set-but-unused variables in `shards.cpp`, all older than F-09). The zips hold the files 1.3.0's did (macOS
  arm64, Linux x86_64 and Windows arm64 compared with 1.3.0's download); the Mac binaries link only the system's frameworks and
  libraries (macOS 11), the Windows ones import the system DLLs 1.3.0's did, the Linux ones need only glibc 2.29's core libraries, as
  1.3.0's (no `libstdc++`). Sizes against 1.3.0: macOS arm64 7.86 → 7.24 MB, Windows arm64 9.01 → 9.12, Linux x86_64 11.27 → 11.74. No
  game window was opened.
* **What moved.** No game code but B-424's two lines; the Makefile's `vesperis` and `vesperis_test` are F-08's byte for byte. New in
  `packaging/`: `toolchains.sh`, `build-mac.sh`, `build-win.sh`, `build-linux.sh`, `build-linux-inner.sh`, `linux.Dockerfile`,
  `build-release.sh`; `CLAUDE.md`'s Release section names them. KI-411 (the Windows and Linux zips untimed on their systems); KI-404
  gained a note (the probe's fall and entry frames varied once under load).

## B-424 (2026-10-15): a telescope photo reads its plate whole on Windows too

`docs/iteration-3/bugs/resolved/B-424-the-windows-telescope-photo-left-its-plate-unread.md`; found by F-09's Windows builds.

* `SpaceRenderer::plateFill` took a budget of 0 ("all of it": a photo through the telescope, the harness's `telePlateFinish`) as
  `1L << 40` samples. Windows' `long` is 32 bits, the shift undefined, and clang folded the select to its other arm, so a photo on
  Windows read nothing more of the plate than the eyepiece's frames had (since 1.2.0). `long long` now, and the loop's
  `std::min<long>(…, left)` a `std::min<long long>`: the first fix (2026-10-14) had kept it, which on Windows makes 2^40 a 0 and the
  loop endless (a hang at the photo; it never left the working tree). The Mac's binaries are byte for byte as before (both types 64
  bits there). The photo itself is unchecked on Windows.
* **The rule**: a count that can pass 2^31 is `long long` (or `int64_t`), never `long`, and never narrowed through a `long`. The
  Windows builds compile with `-Wshorten-64-to-32` (`build-win.sh`): a narrowing only that target reports is Windows' 32-bit `long`.
  The tree has none (every game source compiled for Windows x86_64 and for the Mac with it, 2026-10-15; the first fix's loop was the
  one), and `build-release.sh` prints each build's warning count.

## F-10 (2026-10-15): the hash-moving batch, nothing to bless

The tenth milestone of the performance plan (`docs/ideas/PLAN-performance.md`): floating-point rewrites that cannot promise the last
bits of what they replace, batched so that one bless would cover them all. Three are kept, and none moved a hash or a pixel: the 22
hashes stand, every picture compared is the one F-08's binary drew, and nothing was blessed. The fourth, the plan's `atan2`/`asin`
pair, was built, measured slower in the frame than libm, and taken out.

* **The sky's powers of a cosine as products** (`surface_view.cpp`: `sunGlow`, `pow300`, `pow18`). The day's two glows round a sun
  (`9 c^6 + 14 c^50`), the sun's disc in an airless sky (`c^300`) and a comet's tail (`c^18`) are products of squares, within a few
  dozen ulp of libm's `pow` (c^50's chain rounds seven times, c^300's ten); `pow(1 - el, 2.2)` stays. The sky's loop went from eight
  `pow` calls to one and from 90 stack reloads to 56. Four `bench check` pairs against F-08's binary, the order swapped every pair:
  surface 4x 3.44 → 2.97 ms (the sky's section 1.3 → 0.8), 2x 2.27 → 1.82, sunward 4x 6.21 → 5.66 (its sky 3.5 → 2.9-3.0), forest 4x
  8.92 → 8.37 (8.46 → 7.97 at the game's 3x3), each faster in every pair; with one thread surface 4x 13.17 → 10.70 (the sky 6.1 →
  3.8), 2x 8.04 → 5.43. The plan's -0.65 ms at surface 4x was estimated on F-01's 8.8 ms frame. The bench's sunaway row (30 frames
  facing away from a sun 30 degrees up: no glow in view) read 3.90 → 4.10 in every pair: its frames follow the sunward row's, now
  faster, and meet more of the turn's background work; 300 frames of each view in a scratch harness drew sunaway alike in both
  binaries (3.57-3.59 ms, the sky 1.15-1.17) and sunward 5.60 → 5.07. On baseline x86-64 the airless products meet subnormal values
  within about five degrees of a right angle from the sun (c^300 is under 2e-308 there), as libm's `pow` did; untimed there (KI-411).
* **The clouds' drift wrapped once a frame** (`cloudDriftOf` in `space_view.cpp`, for the globe and the plate). Unwrapped it grew with
  the clock, tens of turns and more in play, so the globe's every cloud read (`cloudAt(lon + drift)`) went through `fmod`; within a
  turn it takes `wrap2pi`'s one-turn path (F-07). The plate's drift is wrapped the same way, so the two stay within a turn of each
  other and the plate's cloud turn keeps a small angle.
* **The limb's cube as products** (`edge * edge * edge` for `pow(1 - viewCos, 3)`). With the drift: space 4x 6.86 → 6.33 ms, 3x 3.71 →
  3.46, 2x 2.13 → 2.02 (one thread 28.99 → 26.65), the plan's -0.5; the telescope 2x 8.95 → 8.64, 4x 18.73 → 18.33, faster in every
  pair.
* **Not taken: the `atan2`/`asin` pair** (the plan's second implementation in `core/` for the globe's latitude and longitude and the
  galaxy band's). Built as planned: the smaller magnitude over the larger, folded past tan(pi/8), one division, an 11-term minimax
  polynomial fitted for it (error under 3e-17), the octant by selects, asin as the angle of (sqrt((1 - z)(1 + z)), z); within 2 ulp of
  macOS's libm over a million directions (4.4e-16 rad), and every picture the same. In the frame it lost: space 4x 6.77 ms against
  6.35 without it, the telescope 4x 19.80 against 18.57, Vimana 1.46 against 1.32 (a rotation of the three binaries, three rounds,
  every round alike). The plan's 13 ns for libm was a loop over random directions, whose signs defeat libm's branches (asin and atan2
  26 ns a pixel there, the pair 5.5); a globe's pixels in scanline order let them predict (libm 7.7 ns, the pair 5.7 on a performance
  core, no better on an efficiency core, where its two divisions and square root are slow), and inlined in the globe's loop the pair's
  234 instructions and 22 more stack reloads cost more than that. Taken out; the angles stay libm's.
* **Not taken: the raster's reciprocals** (the plan's optional item, without an estimate: one reciprocal a triangle for the two
  divisions each row makes). It would move every triangle's edges and lerps by an ulp, and an edge, unlike a shade, can sit exactly on
  a pixel's rounding (polygons drawn from the same corners every frame, the cabin's among them), so the odds that kept the other
  items' pixels say nothing for it.
* **The proof.** The regress 22 of 22 with the workers and with one thread, never blessed. Every picture compared is F-08's binary's:
  the 67 scenes at 4x and 2x (the day skies, the airless noons, both comet scenes, the night skies, moons in the sky), the `space`
  mode's 62 frames at 1x, 2x and 4x (the suns, a globe of every type, the black hole), the 24 screens at 4x (each drawn alike by one
  thread), the 163 flow pictures from one restored state (the game's clock, where the drift had run many turns), the probe's 49
  frames, the 48 scale sheets and the bench's 13 stable pictures; the unit 332 checks, 0 failures. The build with the pair drew the
  scenes, the `space` frames, the screens and the flow as F-08's did too. So the plan's method, the differing pixels' count, largest
  step and bank changes, had nothing to print, and no frame needs a look.
* **What moved.** No generation change (GEN_VERSION 13 holds); the regress unchanged, nothing blessed. Over the bench's 39 steady rows
  -4.0% (the geometric mean of the medians; 15 rows faster in every pair). Against the plan's estimates (surface 4x -0.65 ms, forest
  -0.7-0.9, space 4x -1-1.4, the telescope -0.5): -0.47, -0.55, -0.53, -0.31 at 2x; the space and telescope rows' shortfall is the
  pair's share.

## F-11 (2026-10-15): the close: the budgets lowered, the regress widened

The last milestone of the performance plan (`docs/ideas/PLAN-performance.md`): the gains of F-01 to F-10 guarded by the bench's
budgets and by two more frames in the regress, and the series' numbers and known issues written down. Only the harness changed; no
game code.

* **The budgets: about 1.5x this Mac's medians.** Every `bench check` budget is now about 1.5 times its row's median over F-10's four
  runs on the dev Mac (an M4 of four performance and six efficiency cores), and at least the median + 0.5 ms (the herd's draw, 0.07
  ms, and the belt, 0.60): a change that gives a gain back fails, where the old budgets (12-24 ms on rows of 2-9) let it through. The
  seventeen kept: space 2x 12 → 3.1 ms, surface 2x 16 → 2.8, forest 2x 14 → 8.2 and 4x 24 → 13, city 2x 16 → 5.7, the herd's draw 1 →
  0.6, buggy 2x 14 → 6.1 and 4x 24 → 6.1, drone 2x 16 → 3.3, the belt 12 → 1.1, Vimana 12 → 2.0, the telescope 2x 16 → 13, probe 2x 16
  → 8.9, capital 2x 16 → 5.4, the descent's first frame 35 → 26, beside the capital 70 → 62, and the sprint's worst frame 40 → 10 (one
  frame, a stall detector: one run of eight read 6.6 ms by the machine alone). Twenty new: space 4x 9.5 and surface 4x 4.5 (the
  default picture), the forest at the game's 3x3 mush 12, and the F-01 rows that held within a few percent over eight runs: capital 4x
  8.3, the sector map 8.2, the landing map 3.5, its zoom 3.9 and the zoom's build frame 22, the radar camera 3.8, a photo's PNG 11,
  the telescope 4x 28, the decoder 6.4, sunward 8.5 and sunaway 6.2, the sprint 5.7, city 4x 8.3, probe 4x 13, an arrival's frames
  6.6, the guide's save 1.5 and load 23 (`rowF`'s limit). Not budgeted: a recorded frame (the writer's pace on the efficiency cores:
  7-16 ms in F-06's runs on battery), an arrival's first frame (11.8-20.3 by its maps' threads), the rows at 1x and 3x (the 2x and 4x
  rows run the same code). `bench check <factor>` multiplies every limit: the budgets are this Mac's with the workers, so another
  machine or one thread passes its own factor.
* **The first frames after a scale's change, out of the timed rows.** The first three runs with the new budgets failed two rows in one
  run: space 4x 11.0 ms (6.4 and 6.6 in the others) and surface 2x 2.9 (1.8), both timed from the frame a scale changes, which
  allocates the new scale's buffers and direction table (the Mac swapped 8.8 of 10 GB; space 4x had read 7.5 in one run of four in
  F-08's and in F-10's batches too). The space and surface rows now draw five frames at one instant (dt 0) after the change before
  their timed frames, as every other row draws a warm-up of its own; at dt 0 the game's clock stands, so the rows after them run at
  the game times they did. Three more runs: all 37 within. The noisiest of them, on battery with the Mac swapping, came within 5% of
  one (capital 2x 5.11 ms of 5.4; the landing map 3.15 of 3.5, the sector map 7.36 of 8.2, a stretch of rows 20-45% over their
  medians): a failure on a busy machine is worth a second run before it is believed.
* **The regress widened: 24 hashes.** Two frames join the four at 1x: the forest at 4x (`scene_felisian_forest_4x`, the bench's forest
  and B-423's frame, its flora in bands pinned per scale) and the cabin at 1x through the game (`cabin_1x`: the start's parking, the
  light on and the hull polarised, every map of the system made, the game's picture with its HUD drawn at one instant once it holds
  still: the cabin's bands, the globe in the window, the backdrop, the HUD's text). Each is drawn again by one thread in the same run
  and the regress fails when it differs: the forest's serial-versus-workers check, which B-423 put in the unit (1x, 2x and 4x, one and
  three threads) and F-06 widened (six scenes, one to eight threads), is in the regress too, and `VESPERIS_THREADS=1 regress` still
  checks the frames between runs. `regress bless` refuses then: a frame one thread draws differently is no reference. Blessed for the
  two new lines alone, the other 22 byte for byte; the regress takes 9 s.
* **The series against its targets** (the plan's section 2: its "now" of 2026-10-12, F-10's medians, the target). Met: space 4x 13.6 →
  6.33 ms (7), with one thread 40.6 → 26.65 (30); surface 4x 8.8 → 2.97 (4.5), one thread 17.0 → 10.70 (12); surface 2x 6.6 → 1.82
  (4), one thread 9.95 → 5.43 (7); forest 4x 14.8 → 8.37 (10); buggy 4x 11.9 → 4.04 (8); the landing map at 4x (estimated 17-18) →
  2.29 (under 14); the descent's first frame 37 → 16.95 (20), beside a capital 77 → 41.1 (55); a logged event's guide save 20-35 →
  0.95 ms (+3); a photo's PNG off the frame (encoded on the writer's thread in 7.2 ms; the frame copies it); recording at 4x 33 → 6.47
  ms frames (16.7); a sprint's start and stop with no frame 1.5 ms over its median (+0.5); a capital's tree listing 1.0 → 0.7 ms (the
  plan's 20-120 was an estimate F-01 measured at 1.0). Missed: the telescope 2x 11.0 → 8.64 (8), one thread 28.4 → 24.95 (24); the
  forest with one thread 36.0 → 30.86 (28); the descent with one thread 65 → 63.8 (40: F-03 gave its fill and textures to the pool,
  which one thread does not have). The telescope at its deepest at 4x, 25.0 → 18.33 ms, is still over a 60 Hz frame (KI-406).
* **The known issues.** KI-312 closed: the terrain's cells binned once (F-04), the cabin in bands (F-05) and the rows dealt (F-06)
  were its parts; the frames at 4x are 3.6-4.3 times faster with the workers than with one thread (space 4.2, surface 3.6, the forest
  3.7, the telescope 4.3), about what four performance and six efficiency cores give; the flora's per-tree set-up in each pinned band
  its rows reach stays (2,080 trees listed, 4,804 visits at 4x), the forest the densest steady frame at 8.4 ms. KI-206 (iteration 2)
  closed: the densest forest at 2x 5.46 ms, under N2-04's hoped-for 6 (12 then); its budget 14 → 8.2. KI-331 unchanged (the planet
  function's cost is generation, outside the plan). KI-338 and KI-403 carry F-03's tiles since F-03, KI-022 F-02's writer since F-02.
  KI-406 (the telescope at 4x) and KI-411 (Windows and Linux untimed; now also the budgets, this Mac's, and the regress's two new
  frames, unchecked in the Linux containers) stay open.
* **What moved.** Nothing drawn: no game code changed, GEN_VERSION 13 holds, the regress's 22 hashes stand and two are added. The
  harness: `appendFrameHashes`' two frames and their both-ways check, `bench check`'s 37 budgets and its factor, the warm-up after a
  scale's change. Unit 332 checks, 0 failures; the regress 24 of 24 with the workers, one thread and three. The cookbook's recipe for
  timing a change (`09-cookbook.md`).

## R-418 (2026-10-17): the controller

The user: bindings for most of the game missing from the pad, a tester on an AYN Thor finding them wrong, and the suggestion of a
controller's own menu of actions to pick from, "so we can be sure game is playable if bindings are wrong", with the bindings still
configurable though "controller can be literally anything". `docs/iteration-3/plans/requests/R-418-the-controller.md`.

* **The controller is read raw, through GLFW, beside its standard layout.** raylib's gamepad calls see only the devices GLFW has a
  mapping for, and only through it: a controller nobody mapped reads as nothing, and nothing in-game can bind it. GLFW's joystick
  calls (`glfwGetJoystickButtons/Axes/Hats`, `glfwGetGamepadState`) give both; they live inside raylib's static library, which the
  release scripts link already; the dev Makefile now links Homebrew's `libraylib.a` too (its shared library keeps GLFW hidden), so
  both builds take the same path. The declarations sit in `platform_raylib.cpp`, the one file that sees raylib. SDL's
  `gamecontrollerdb.txt` beside the game is read for the layouts GLFW's list lacks (the community's file covers thousands).
* **Binding by role, not by action.** The game reads sixteen buttons by the Xbox names and places and two sticks; the CONTROLS
  screen's second page binds each to any physical control the device reports (a button, a trigger, a hat, an axis's half), kept by
  device. Per action across the game's contexts would have been a table of a hundred rows for the explorer to fill and keep; a role
  bound once makes every context's layout right, and an unknown device is set up by pressing sixteen buttons and pushing two sticks.
  The preference "jump on another button" is a swap of two roles (a control taken by one role is given back to the other). A raw
  device starts from a guess at the common driver order that reads its axes at rest (a stick axis off-centre or a trigger resting in
  the middle is left unbound: the first would walk by itself, the second hold its button).
* **A layout a context, run as the game's own keys.** The controller presses keys into the frame's copy of the `Input` (`padFrame`,
  before anything reads it): no action has a second code path, the keyboard and the controller work at once, and the harness drives
  the game with either. The contexts (the screens, the title, the ship, on foot, the buggy, the drone, the probe, a recording, an
  instrument) give the face and shoulder buttons what each needs most; the rest go to the command menu.
* **The command menu is the guarantee.** Y in every context: every action of the screen in words (with its state), the general
  ones, and a keyboard of every key the game reads with Shift, Ctrl and Alt latched for the next: whatever the bindings, anything a
  key does is a few presses away, and a wrong layout costs comfort, not reach. The menu runs a line as its keys; the context's page
  is written for each screen (curated rather than generated from the key handlers, which are code, not data: the cost is keeping the
  lines in step with the screens). START is Esc everywhere but the title, where it begins (pressing START on a title is what players
  do; there it would have quit), and B quits nothing there (the menu's QUIT does).
* **The maps take their own buttons.** On the landing map, the sector map, the star map and the galaxy chart a button presses only
  what does something there, and the legend names it in the map's words (`A LAND`, `X BACK TO YOU`, `LB HIGHER`); the right stick in
  the screens runs at 400 mouse pixels a second against a head's 900 (the sector map's cursor crossed its frame in half a second at
  full throw); the landing map reads the mouse's motion as a smooth cursor (only the stick gives it any there: the mouse is not held
  on that screen), the d-pad still stepping a degree.
* **The on-screen keyboard comes up by itself only where letters are wanted** (a text entry, the console) and only while the
  controller is the device in use, so a keyboard player never sees it; the aim's light years ask for it from the menu.
* **Not done**: hold actions from the menu (a line taps its key: stand tall, the jetpack, the orbit's Shift are the buttons'); a
  second controller (the first is read); the browser build's raw devices (it reads the browser's standard mapping through raylib);
  rumble; the hints of the screens in a controller's words (the menu's foot and the help's third page say the buttons instead).
* **Fixed on the way**: the left stick walked nothing in the cabin (the cabin read the keys alone); the right stick's look ran with
  the frame rate and the mouse's sensitivity twice (per second now, its own speed); `E` sat on the guide button the systems keep;
  the stick took both of the walk's axes from the keys at once (a trigger's throttle died under the stick's steering); the keyboard
  page's key capture could not be left with a controller.

## B-425 (2026-10-18): the companion stars keep their distance

The user, from Yaiphuyiabrus (a red dwarf with a red giant companion, `S06+S03 MULTIPLE`): a sun "ridiculously huge" over a felisian
world, "vegetation wouldn't survive it at all". `docs/iteration-3/bugs/resolved/B-425-a-red-giant-filling-the-sky-of-a-red-dwarfs-worlds.md`.

* **A floor under the drawn orbit, not a new draw.** A far companion keeps M5-01's draw (2.5-7.5 times the widest planetary orbit)
  and its eccentricity; the floor raises only the orbits under it, and every draw keeps its place in its stream (the eccentricity has
  its own), so a system whose companion already kept clear stays as it was (44% of the multiples) and no other property of any system
  moves.
* **Three reasons to keep away, each a number.** The sky: 3 degrees at most at the primary's farthest world, at the companion's
  periapsis (the primary spans 4-12 degrees from a temperate world: the companion stays the smaller sun; the giant now 0.65-1.2
  degrees). The warmth: the companion's light alone would keep that world at 100 K at most (the worlds are typed by the primary's
  light; 100 K adds two or three kelvin to a temperate world, and it is the perceptual factor's old floor, `278 x 0.36`). Its own
  worlds: within a third of the gap, with the primary 3 degrees wide at most from them (a red giant's worlds orbit 1.6e8 km and more
  out: they circled both suns of a pair 4.5e7 km apart). The bound on its worlds' reach is the widest its draws allow (the spacing at
  its top), so the floor cannot read their orbits before they are drawn.
* **Not the companion's light in the worlds' warmth.** The primary's worlds come first in the stream, and the companion's close or far
  choice reads them (the first orbit, the widest one): counting the companion in their temperatures would retype the worlds of a fifth
  of the systems, the worlds explorers land on, name and mark. Keeping the companion away keeps every world as it was and makes the
  primary's warmth the true one.
* **A close companion is a dim one.** A tenth of the primary's light at most (2.4% on a temperature) and no wider than the primary in
  the first world's sky at its nearest; else it is far, with up to three worlds of its own after every other body. Close twin suns
  are far pairs now. A black hole keeps its close companion (S-06's stream), over worlds typed by the hole's faint disc (KI-413).
* **A far sun lights as much as it does.** The floor of 0.62 is a world's own sun's (a dim star's day stays a day); the other sun has
  its true perceptual factor, so a far companion gives a night a twilight at most; a set primary adds nothing to the blend; the
  blended light's glow and a lone sun's blob shadows follow its share; on a companion's world the far primary's sky, disc, glare and
  flare follow its light, and the local time and the landing map's day are the companion's. The perceptual blend of the two
  directions stays M5-01's (KI-414). One-sun worlds are bit for bit as before: the new factors are exactly 1 there.
* **What moved.** `GEN_VERSION` 14 (the table above); the regress unchanged; four unit checks
  (336); one unit line, the site of Eliameavevas I-a, whose light blends a companion (40.1 degrees for 40 asked, 39.8 before). The
  reported place is on Yaiphuyiabrus IV (its people's world, a day of 2.4 hours) at three in its night: the giant, 95 degrees across,
  lit it as a day, and it is a night now under a sun 0.65 degrees across (B-426's screenshots, from the same visit, showed the world: its
  ruins; the first frames had been read on III, locked to the dwarf). KI-413 and KI-414 for what stays.

## B-426 (2026-10-18): an archive holds its count

The user, from the capital of Yaiphuyiabrus IV (a prehistoric people's): the shards "all located in the center, tons of them", which
looked like "some bug with placement that ignores other buildings and just stockpiles everything in one place", though "the center
having a bit more shards" was welcome. `docs/iteration-3/bugs/resolved/B-426-a-capitals-records-heaped-on-its-archives-floor.md`.

* **T-04's count, not the place's whole share.** An archive holds its class's count (the capital's 8 to 14, a prehistoric capital's 4 to
  6, a city's 5 to 8, a third of the towns' 3 to 5), the place's own first: the code had asked it for the larger of its count and the
  place's own, so a people of few places laid a third to a half of its records across one room (31 of 80 on a chief's longhouse's floor;
  a futuristic capital's 32 and more). The centre keeps a few more than a house: the archive is the find there.
* **The place's own over its buildings.** What the archive leaves of a place's own lie one a building, drawn by C-03's weights (the place
  draws as many buildings as it keeps records), then a second and a third in a room's spots, so a capital that kept 38 of its people's 80
  has them in 34 buildings across its 120 m (four in the archive, the last recording in the same longhouse, 33 huts one each).
* **Every place holds its own.** T-04's home "always holds it": where its rooms' spots run out (a lone menhir's foot holds two, a futuristic
  capital's pods two each) the rest go one at a time across the room or round the foot of each building holding one, the archive's rows
  last; a painted one as a slab only where a slab has room. The far future's structures keep theirs in rings round their feet as T-04 has
  them (a building alone, the archive that keeps itself). Three harbour places of the survey lay nothing, their spots read as the sea
  (B-421's rule): KI-415.
* **What moved.** No generation change (a record's index is its key in the guide wherever it lies); the places' layouts of their records,
  the archives of the prehistoric and futuristic capitals above all; a capital whose own records fill C-03's few lays no copies of other
  places' (its few were its own first already). The regress unchanged; one unit check (337).

## R-419 (2026-10-18): auto-explore

The user: "autoexploration. Some user requested it so game plays itself", several profiles ("focus on space, on planets, on surface of
planets, on civilizations, gas giants, random - you choose and give ideas"), no HUD while it plays, and the movements "soft,
non-robotic, everything is in chilling manner. So it feels like real person is playing". `docs/iteration-3/plans/requests/R-419-auto-explore.md`.

* **It plays through the input, not beside it.** The explorer's mind makes the frame's input (the keys, the mouse's motion, the stick)
  and the game reads it as the player's (`Game::autoInput`, after `padFrame`): no second code path for a walk, a drive, a flight, a
  landing or a take, so every rule holds (the walls, the slopes, the water, the vehicles' handling, a record's reach) and what it does
  is what a player could do. Two exceptions, where the input's own pace is not a hand's: the ship's nose (the arrows turn it at 69
  degrees a second on and off, `X` snaps) is eased by setting its angles, and a lap round the parked world (Shift and the arrows: twenty
  degrees a second on and off) by calling `orbitShip` with an eased rate. The choices a screen would make (a local target, a remote
  one, a landing site) are made in its head and set as the screens set them (`ship.localTarget` then Enter, `setRemoteStar` then V,
  `landLat/landLon` then the landing map's Enter on its first frame).
* **A hand's limits make the softness.** A critically damped spring with a speed and an acceleration limit (43 degrees a second and 80
  a second squared at a walk's pace, a third of that for a long look) eases every look off its rest and onto its target; a sway of a
  few tenths of a degree keeps a rest from being dead still; the stick ramps; the clock's pace changes in decades. The game's own turns
  of the view are taken up as the new rest, not fought; where the game turns the view by itself (the Vimana and the telescope face the
  front window, a vehicle's seat faces its heading) it looks that way first, so the turn is small; the roof deck's lift, the telescope's
  powers and a vehicle's seat stay cuts.
* **The fine approach smoothed for everyone.** Auto-explore's harness found the one fast turn left at every parking: the ship flew a
  straight line to a parking point beside its world with its nose on the world's centre each frame, so in the last tenth of a second the
  world's direction swooped 60 to 350 degrees a second. The approach is now taken from the world, as the probe's fall is: the distance
  closed by equal ratios, the direction swung onto the parking's in the first seven tenths, the last stretch straight in. A player's
  approach changed with it (the same start, end, duration and aim; the world a speck longer, then growing steadily): one path for both
  rather than a smoother one for the explorer alone (`updateShipMotion`, `APPROACH`).
* **No HUD, no screens.** Nothing the HUD draws is drawn (the status line, the probe's and the radar's frames, the capsule's caption,
  the controller's menu); its first five seconds say how to stop it. It opens no screen (the landing map's frame is the cabin's); a
  screen opened by chance is closed with Esc.
* **The expedition's, not a copy.** It plays the expedition: the log, the guide (the visits, the landings, the sightings, the records
  and instruments taken) and the game's own autosaves take what it does as the player's; it names nothing, marks nothing and saves into
  no slot. A copy thrown away at the end would keep nothing of an hour watched, and a player who wants none of it saves to a slot
  first.
* **Any input gives the controls back, unacted on.** Any key, click, wheel, a mouse's motion past a few pixels (none in a scene's first
  half second: the window taking the mouse moves it), a button or a stick; that press is swallowed (Esc opens no menu, `E` takes
  nothing) and the warp goes back to x1.
* **Six profiles, one mind.** The profiles share every step and differ in their weights (the worlds of a system, a long jump, a landing,
  a probe, the telescope, the roof deck, the glass hull, a lap; the time at a parking and on the ground; the ground's outings) and in
  what they look for (the stargazer's star classes; the giants' and the archaeologist's systems, which they pass through when they
  hold none). The wanderer takes one of the five at each star, the system's own leading.
* **Its ground walk plans its way** (`autoWay`): A* over 40 cm cells round a leg of up to 24 m, the colliders at a body's height (a
  settlement's walls are rows of discs: its doorways stay open), the slopes the walk cannot climb and the deep water; legs, not a whole
  way, because the colliders are gathered round a point (40 m of a settlement's pieces) and a way is planned again from wherever it
  stalls.

## B-427 (2026-10-18): the stripped tail, the giant's own gas

The user, parked at the gas giant of a Wolf-Rayet star (PARSIS +18071 +1 -19950): "this strange white lights behind it [...] with several
gas giants as well [...] it's just a blob of white light behind them". It was S-05's tail of the envelope the wind strips, which every
gas giant and substellar object of a Wolf-Rayet star has. `docs/iteration-3/bugs/resolved/B-427-a-white-blob-behind-a-wolf-rayet-stars-giants.md`.

* **Kept, made to read as gas.** The tail stays: S-05's look of the star's wind on its worlds, the same curve, length and widening.
  Its forty glow discs (14 shades each, crowded at the root) summed to the ramp's white. Now about ten splats each carry the light of
  their stretch of the tube, so the side-on column is the profile's (24 shades at the root) and a view down the tail stops at a soft 36,
  under the lit face's 40-50.
* **Drawn after the globe with a depth test.** It had been drawn before the globe (which covered its root), but from the night side the
  tail streams in front of the globe, so the globe hid what should have veiled it. Each splat is now hidden where the frame holds
  something nearer, softly over the splat's own depth.
* **The brighter light's bank, dithered between.** A pixel has one bank: the tail takes the giant's where it outshines the pixel's own,
  and an ordered 4 x 4 dither covers the range where neither does (the shares 0.3 to 0.7). A glow disc's rule (its bank on empty pixels
  only) had given the tail the band's blue, and the plain switch a hard outline across the band. The backdrop's nebula patches keep
  their own rule (the brighter's bank, no dither).
* **Filaments,** two octaves of noise across each splat in the axis's direction on the frame, streaming outward with the clock and faded
  end-on. They show the light as a stream, not a halo.
* **A sky by luminance.** The surface's sky bank is black below shade 30 (an airless world's, which every world of a Wolf-Rayet star
  is), so a shade added there showed nothing until it heaped. In a sky the tail's luminance (at most the bank's shade 40) is added to
  the pixel's, read from the palette, and written back as the bank's shade of the sum. The band's pass then carries it into its bank
  as it does the sky's own light.
* **Its cost held.** Sampled once per 2 x 2 block at 3x and 4x, in bands of rows on the workers (band-pinned, the same with one
  thread); about ten splats against forty full-resolution discs.
* **What moved.** No generation change; no regress scene has a Wolf-Rayet giant (24 of 24 unchanged). One unit check; a harness mode,
  `tail`.

## B-428 (2026-10-18): moons spread, a system with distance

The user: "quite often planets/moons are super close to each other when you are on startdrifter [...] I love when I am on the surface
and you seen a big beatiful moon or planet [...] there is no sense of it being an actual system with distance and stuff, makes it feel
more pocket". `docs/iteration-3/bugs/resolved/B-428-moons-crowding-their-worlds-from-the-ship.md`.

* **The ship's view and the ground's are one distance.** R-001 and M5-09 drew the first moons in (2.8-4.2 radii), B-314 put them at
  4.5-6 so a parent filled no more than 26 degrees, and the ship parks at 3.5 radii, inside that orbit: another world wider than 10
  degrees hung in its sky at 44% of the parkings near home. A moon wide from the ground is wide from the parked ship, and there the
  two worlds and the gap between them are seen together. No look of the space renderer separates the two (R-001's own rule: what is
  in the sky is where the maths puts it), so the spread is a trade, and it was put to the user as one question: the solid worlds' moons
  and the twins (recommended), the giants' as well, or a gentle spread to 5-20 radii. The user chose the first.
* **A solid world's first moon at 5-40 radii, even in the logarithm; a twin at 12-20.** On the same draws, so every other part of a system
  holds. The log keeps the close moon (one in eleven inside 6 radii, a big disc from the ground) as the exception, not every system.
  The parking (3.5 radii) stays: it frames the whole globe.
* **The giants' moons kept.** A giant from its first moon (4.5-6 radii, 19-26 degrees across) is the biggest disc in the skies and
  Jupiter's own over Io; from the ship it still looms behind its inner moons (34% of their parkings), KI-417.
* **Generation 15.** Kepler gives the periods, so a locked moon's day follows its month (a solid world's first moon 7.7 hours at the
  median, 1.9 before), not a constant re-fitted to keep the old days. Saves from 14 get the notice once (1.3.2 had already moved to 14
  with B-425).
* **Measured where it is decided.** `survey` prints the parkings next to R-001's ground line; the unit check bounds both sides (the ship
  under 30%, the ground's discs wider than 2 degrees at 28% of sites at least and wider than 20 degrees not gone).
* **Found on the way, not changed:** the comets' orbits are keyed to the last body generated (a moon in 87% of systems), not to the
  widest orbit as M5-07 says, so most sit at the floor of four star radii at periapsis (KI-416). Moving them is a generation change
  of its own, the user's call.

## R-420 (2026-10-19): auto-explore's tape

The user, with four MP3s they had downloaded: "add these for to auto-explore mode as an optional feature - you can enable playlist with
these 3 files with M button while in autoexplore and it will cycle. Use some 80s-90s cassete insertion sound between tracks. Investigate
authorship properly so that when track starts author and name of the track are displayed for a few seconds on top left and then fade
away. Also when music plays all other sounds are quiter". `docs/iteration-3/plans/requests/R-420-auto-explores-tape.md`.

* **The game's own tape, and the explorer's.** The four ship with the game: `music/` in the repository with its `playlist.txt` (the
  order, the credits, the gains), packed beside the binary in each zip and inside the macOS app's `Contents/Resources`, since the app
  runs from `~/Documents/vesperis` (`packaging/tape.sh` copies the folder's tracked files, never a player's own beside them). The game
  finds it from its executable's folder (`platformAppDir`). A `music` folder in the working directory that holds a track plays instead;
  a folder's playlist comes first, then the sound files it does not list, so a player can add to the tape by dropping files in.
* **Authorship from the tracks' own published credits.** The files carried no tags; each track's YouTube upload and its description gave
  the credits: *Forwards* by Jarrad Wright, from *Sassy the
  Sasquatch (Original Motion Picture Soundtrack)* (DistroKid, 2022-10-05); *It's time to wake up* and *BLUE-MEAN-EYES* by Jarrad Wright,
  posted as the "CI2 soundtrack" by The Big Lez Show's channel on 2015-09-29 (*Choomah Island 2*, the show's special of 21 August 2015;
  Wright writes and performs the show's music); *Planet Caravan* by Black Sabbath, from *Paranoid* (1970), the 2009 remaster. The caption
  shows the author and the title as the playlist gives them; the rest is in its comments.
* **raylib's stream, on a thread of its own.** The tracks are 44.1 kHz stereo; the synth is mono at 22,050 Hz, so the music does not
  go through it: raylib's music stream plays it beside the synth's stream, decoded as it plays (MP3, OGG, WAV, QOA; raylib's build has no
  FLAC). raylib, told not to loop, stops a track as it reads its last block, that block and the one playing still unheard, and the
  game's frames can stall past a stream's buffers: so a platform thread updates the stream every 5 ms, the stream loops in raylib and the
  thread stops it 30 ms short of its end. The game asks and hears back through `AudioState` (`music`, `musicGo`, `musicGain`; `musicT`,
  `musicEnded`, `musicFailed`), as for the rest of its sound; the harness stands in for the platform there (`TapePump`).
* **The deck is synthesized, not sampled.** No recording to license or ship: struck modes (a plastic part's three or four resonances, a
  body's thump, a spring's ring) on a millisecond of noise, a slide of band-passed noise sticking and slipping with the reels knocking,
  the motor an oscillator, the tape's hiss filtered noise, two early reflections for a room. One deck: its parts keep their pitches,
  each event a few per cent off them by a seed, the hands' pauses 15%, so no two changes are alike. The tape moves at the play key's
  latch plus the motor's half second (`deckTapeAt`), and the game starts the track then.
* **Ducked by a level, not a compressor.** While the tape is on (the deck's sequences included) the synth's sounds keep 0.35 of their
  level, eased over half a second; the deck rides over them. A fade (auto-explore's end) brings them back as the music goes.
* **Tracks evened in the playlist.** The four differ by 11.6 dB (EBU R128: -14.9, -26.5, -21.4 and -15.7 LUFS), so each line carries a
  gain bringing it to -16 LUFS (raylib does not clamp a stream's volume above 1; the quietest, raised 10.5 dB, peaks at -1 dBFS before
  the tape's level and the master). The game measures nothing itself: a player's own playlist evens its own.
* **The tape belongs to auto-explore.** `M` starts and stops it only while auto-explore plays (otherwise it is the star map's key in space
  and the waypoint's on the ground); when auto-explore ends the music fades, and it is taken up where it was, with the play key, as
  auto-explore next begins (`auto_music`, `auto_track`, `auto_track_at`). A cassette left in plays on from its place; one left at its very
  end is over, and the next goes in.
