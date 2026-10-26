# Rules: the planet and its surface

The rules in force for the planet function, the ground and what stands and moves on it, a line each with the entries behind
them (`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md`
(`docs/CLAUDE.md`). The site, what stands and moves on it: `rules-surface-site.md`.

## The planet function
- One planet function serves every view (globe, maps, telescope, probe, ground): what is seen from orbit is what is walked on [table], [X-03]
- A sample is a pure function of the sphere point and the detail (no global simulation), the same on any thread and in any order [table], [M9-04], [F-03]
- A formula a ground and a finder share lives once, in planetmap.h or planet_features.h (`iceSheetIn`, `springVents`): never copied [P-02], [P-03]
- Each old ground stays its own ground function over `GroundCtx::relief`: merging two would move their bits [P-00]
- A row field of the felisian layer (`livingGround`) acts only where it differs from the felisian value: old kinds' bits stand [P-02], [P-03]
- A dead row takes its living kind's draws, relief and tints (`thenGround`); its cover changes materials and water, never a dry place's height [P-04]
- Flat classes stay flat (cinematic realism, the user's call): small landforms break a plain, not raised relief [O6-02]
- The relief class is the regional mean grade over 150 and 600 m (cut at 0.05 and 0.2), never the steepest probe [O6-02]
- Caves and overhangs await an agreed design (a height field cannot carry them): nothing may block them; pits and cave mouths stay openable [table]
- KI-408 (the breach's two sinking cells) is generation: its fix waits for a `GEN_VERSION` bump [F-03]

## Materials and landforms
- A material never follows a faded value (`matVisible`: unfaded where its patch spans a quarter cell), so globe, map and rings agree [B-404], [P-05]
- Slope rules (rock, scree, cliffs) read the material chain's slope (octaves of 48 m and longer, unfaded): one number in every ring [O6-03..O6-07], [B-404]
- Shaped features (mesas, canyons, cones) fade by their own width (`featFade`); `lodFade` is for noise and periodic landforms [B-404]
- Materials append at the enum's end (`MAT_GLASS`): an appended material moves no map hash [S-03]
- Landforms are additive terms of `landformsAt`, each with its LOD gate, mask and material; big ones move `hS` (`dhSmooth`), so water follows [O10]
- Trait landform materials come after the ground's own (a ground that covers them clears them); no ice kind takes a trait laying basalt or sand [P-05], [P-06]
- A feature at a cell's middle (a pit, a dome) stands on grid features on the sphere (`surfaceCells`, `visitFeatures`), never `worley3`'s [P-06]
- A landform's mark (`LandOut::markTrait`, `SurfaceSample::landformTrait`) is set at every scale, the scan's 500 m too: a mark is no material [Q-01]

## Traits
- `BodyGen::traits` holds six: the first three R-304's draw over `TK_FIRST`'s traits alone (the unit hashes 2,000), the rest by own hashes [Q-01]
- A new process is a `KS_` bit given in one block of `buildKinds`; one split off (`KS_FREEZE`) goes to all that had it: no old list moves [P-02], [Q-01], [Q-02]
- One colour trait a world; pangaea never with the archipelago, giant flora never with dead forests, penitentes never with spire fields [Q-01]

## Water and drainage
- The drainage is a per-tile layer of the planet function (1-degree tiles, 0.3 margin, blended over 0.2): no global map, no per-sample search [O6-03..O6-07]
- A tile is the same bytes from any thread or order: `t_neigh` from its cell alone, one integer sort key a cell, `drainageSink` exactly the step loop [F-03]
- A tile is made whole or not at all; a prefetch takes all cores but two (a waiting caller all but one) and, superseded, drops tiles nobody waits for [B-422]
- Only a landing waits for drainage tiles (`SurfaceView::init`'s prefetch; the fill builds none); a far mark reads built ones, asking every 2 s [F-03], [B-422]
- A people's generation reads no drainage tile (layouts, works, searches: `DrainageOff`); the view re-lays records by drawn water [C-08], [T-03], [B-421]
- A lake has one flat level, its basin filled to the spill by the drainage's flood, never a local function's (which tilts across a basin) [O10], [O6-03..O6-07]
- A dead desert's old seabed (`oldSeaM`) is sea to the drainage, settlements and roads: wadis end at its shore, nothing is built or routed on it [C-01], [C-09]
- A dead world's drainage tiles are its then-world's (`genKey` by the living kind, `buildTile` on `thenWorld`): the rivers its people knew [P-04]

## Sights
- Sights are found in the terrain, not placed: a ~14 km cell's best of each kind (`sightsOfCell`), ruins one by one, a lake on water [O6-03..O6-07], [R-408]
- O6-06's one landmark a cell and centroid lake live on only in `legacyLandmarkOf`, which moves the explorer's old names to marks [R-408]

## Weather
- Fronts are one function of body, place and time (`galaxy/fronts.*`): the overlay, maps, ground weather, forecast, almanac and `LocalFront` read it [W-03]
- A front rains only from its line (its forecast time; `THE FRONT CLEARS` is the rain's end); pattern rain reads `env.cloudPattern` alone [W-03]

[table]: log/0-1.0.0.md
[M9-04]: log/0-1.0.0.md
[O6-02]: log/0-1.0.0.md
[O10]: log/0-1.0.0.md
[O6-03..O6-07]: log/0-1.0.0.md
[B-404]: log/1-1.1.0.md
[S-03]: log/1-1.1.0.md
[C-01]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-09]: log/1-1.1.0.md
[X-03]: log/2-1.2.0.md
[W-03]: log/2-1.2.0.md
[R-408]: log/2-1.2.0.md
[T-03]: log/3-1.3.0.md
[B-421]: log/3-1.3.0.md
[B-422]: log/3-1.3.0.md
[F-03]: log/4-1.3.1.md
[P-00]: log/6-1.4.0.md
[P-02]: log/6-1.4.0.md
[P-03]: log/6-1.4.0.md
[P-04]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[P-06]: log/6-1.4.0.md
[Q-01]: log/6-1.4.0.md
[Q-02]: log/6-1.4.0.md
