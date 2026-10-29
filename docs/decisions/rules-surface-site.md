# Rules: the planet and its surface (the site)

The rules in force for the planet function, the ground and what stands and moves on it, a line each with the entries behind
them (`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md`
(`docs/CLAUDE.md`). The planet function, materials, traits, drainage, sights and fronts: `rules-surface.md`.

## Caches and threads
- Caches are written by the main thread or, in a joined region (the warm's fill, its vertex rows), by one pool thread a slot: no locks [M7-01], [F-03], [F-04]
- A band thread makes no tile (`buildMesoTiles` runs after the fill); the ahead worker writes only a staging buffer the main thread copies in [M7-01], [F-03]
- What may fill a cache on a miss runs serially on the main thread (`gatherSettlement`, `gatherReflections`) [A-02], [F-05]
- A ring's cache is a torus tagged by cell; its square of 2r + 5 cells must fit it (`1 << logN`): the unit checks [table], [F-03], [F-04]
- The warm's disc reaches 3 cells past the radius (4.5 on a lava world's near rings): every cell a corner reads is sampled before the vertices [F-04]
- A terrain sample is `sampleAt`: the planet function, then the site's work edits, roads and town marks; none enters the planet function [C-09], [T-02], [T-03]
- Roads, town marks and work edits grow only with the ahead worker joined (`joinAhead`): other threads read them [C-09], [T-02], [T-03], [F-03]
- The view never waits on slow work: the works come by `worksAroundIfReady`, the scan on a thread of its own (`startScan`) [R-408], [B-418]
- The site frame is azimuthal equidistant round the landing site (exact, pole-safe, in metres), no longer the local vertical past ~100 km [table]

## The rings and the ground drawn
- `groundHeight` is the ground drawn (the mesh's own blend and morph weights): feet and wheels stay on what is drawn [O6-02], [B-313]
- Every ring geomorphs onto the next coarser (height and shade) by continuous camera distance; the unit wants a zero gap at every edge [B-313]
- Rings are warmed coarse to fine and overlap by one cell; a depth bias only ranks rings: a ring's ground and water share one [B-313], [O11]
- The water's edge is cut into the mesh from the shore field, never smoothed into the planet function; the morph carries `shore` [O11]
- Detail a coarser ring cannot carry (sub-cell water) fades across the morph band; a river is drawn at least 1.6 samples wide [O11], [O6-03..O6-07]
- The 4 m near ring is drawn at every speed; the buggy rides a suspension (its hull's height averaged over 16 m) [B-313]
- The sea stands at `SurfaceSite::tideM`: drawing, shore cut, explorer, vehicles, rangefinder and auto-explore read the tide; placements the mean [Q-02]

## What stands and walks
- Colliders come from what draws (`forRocks`, `forTrees`, `forLogs`, `pieceFootNear`, `formColliders`), measured: never a wall in the air [A-02], [B-420]
- A collider spans heights (`Collider::y0..y1`): the explorer, vehicles and `findOpenRun` skip one wholly below or above them [B-405]
- Only the terrain and `formWalkable` pieces (heaps, banks, barrows, tiers: `formSupportAt`, `floorAt`) are floors, never a collider's top [B-405], [B-420]
- Forms stand on the drawn ground (`drawnGround`): walls on the footprint's highest, earth on its lowest, `Form::absolute` at its height [A-02], [B-420], [T-03]
- Pieces under 0.9 m (yard walls, fences) carry no collider; to the buggy rocks under 0.9 m are bumps, bigger ones, trunks and logs stop it [N4-02], [R-412]
- No tree, rock or log stands in a building (`settlementsNear` once a 16 m cell), on a street, road or work, or in water [C-01], [C-08], [T-02], [T-03], [B-420]
- Swimming is the body's height at the float level (feet within 0.3 m, 0.6 once swimming), never the depth under it: the air over water is air [B-406]

## The vehicles
- `B` (`F`) at the capsule always unfolds a fresh buggy (drone), the old one gone: no one is stranded; on a comet both refuse [B-205], [O4], [R-303], [R-403]
- Comets keep honest microgravity: ground speed and jets capped under the escape velocity, `Ctrl` brings a jump down; never fake gravity [O4], [R-303]
- Vehicles are closed, seen from inside only through the nose camera (a 70-degree pan); the drone's picture stays level [N4-06], [R-204], [R-403]
- The buggy tops out at 50 m/s (180 km/h), the drone at 320 km/h under a 400 m ceiling: the user's numbers [O1-05], [R-403]
- No traps: held Space afloat fires the jetpack, as there is no jump from water, and no vehicle needs fuel [R-403], [B-406]
- The buggy leaves the ground only when following it needs over four g downward (40 m/s^2 at least): `vesperis_test descent` guards it [B-307]
- The drone lands on anything level, water included (`surfaceHeight`: it floats); `E` leaves it only when landed [R-403]

## Life and weather
- Herds live in hashed 512 m habitat cells by biome chance (none on ice or open sea), spawned within 900 m, forgotten past 1300 m [N3-04], [B-316]
- Anything moving at a varying rate integrates per frame (cloud drift integrates the wind), never rate x t: game time reaches millions of seconds [B-206]

[table]: log/0-1.0.0.md
[M7-01]: log/0-1.0.0.md
[O6-02]: log/0-1.0.0.md
[B-313]: log/0-1.0.0.md
[O11]: log/0-1.0.0.md
[O6-03..O6-07]: log/0-1.0.0.md
[N4-02]: log/0-1.0.0.md
[B-205]: log/0-1.0.0.md
[O4]: log/0-1.0.0.md
[R-303]: log/0-1.0.0.md
[N4-06]: log/0-1.0.0.md
[R-204]: log/0-1.0.0.md
[O1-05]: log/0-1.0.0.md
[B-307]: log/0-1.0.0.md
[N3-04]: log/0-1.0.0.md
[B-316]: log/0-1.0.0.md
[B-206]: log/0-1.0.0.md
[C-01]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-09]: log/1-1.1.0.md
[B-405]: log/1-1.1.0.md
[R-403]: log/1-1.1.0.md
[B-406]: log/1-1.1.0.md
[R-408]: log/2-1.2.0.md
[A-02]: log/3-1.3.0.md
[T-02]: log/3-1.3.0.md
[T-03]: log/3-1.3.0.md
[B-418]: log/3-1.3.0.md
[B-420]: log/3-1.3.0.md
[R-412]: log/3-1.3.0.md
[F-03]: log/4-1.3.1.md
[F-04]: log/4-1.3.1.md
[F-05]: log/4-1.3.1.md
[Q-02]: log/6-1.4.0.md
