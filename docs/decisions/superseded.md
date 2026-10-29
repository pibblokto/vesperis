# Superseded rules

Rules once in force and since replaced, by area, oldest first: the old rule with the entries that set it, then what replaced
it. The rules in force are the `rules-<area>.md` files; why each changed is in the entries (`log/`). A milestone that
replaces a rule moves its line here (`docs/CLAUDE.md`).

## Rendering and the frame

- Software rendering at 320x200 in 4 banks x 64 shades: four hues a scene, juggled by the scenes [table]; replaced by [M10-01],
  [M10-03]
- A pixel holds 16 banks of 4096 intensities (64 units a classic shade) [M10-01], [M10-03]; replaced by [O0-02], [B-305]
- Flat cells are the default terrain shading; the quantised gradient is a setting [M10-12]; replaced by [B-311]
- The default render scale is 2x with a mush box of scale + 1 (3x3) [M10-02]; replaced by [B-310]
- Visor glints are HUD arcs, drawn on the surface only [N5-02]; replaced by [B-309]
- Belt rocks share palette bank 3 with a second world's disc (all sixteen banks taken in space) [O3]; replaced by [O0-02], [B-305]
- SurfaceSite::sun floors the primary at 0.62 on every world, a companion's too; the companion keeps the 0.35-floored factor
  [O6-02], [KI-309]; replaced by [B-425]
- parallelFor gives each thread one equal band of rows for good [B-310]; replaced by [F-06]
- The galaxy band is normalised to the core's direction and toned with a 0.35 gamma [G-01]; replaced by [G-03]
- The galaxy band map is built serially on purpose, the terrain prefetch holding the pool [G-01]; replaced by [G-03]
- The stripped tail is forty glow discs drawn before its globe, each in its bank on empty pixels only [S-05]; replaced by [B-427]
- The eyepiece draws nothing differently from the window: a globe from its world map, only closer [W-06]; replaced by [W-06
  reworked]
- The probe's picture is drawn in palette banks 28-31 (its sun, rain and entry plasma) [X-01]; replaced by [X-02]
- The pool is not reentrant: nothing that may dispatch (blendRectRGB, glowDisc, a meso tile's rows) runs inside a pool job [G-07],
  [F-03], [F-05]; replaced by [F-06]

## The galaxy and its generation

- A first moon orbits at 2.8-4.2 radii, closer and bigger [M5-09]; replaced by [B-314]
- R-001's sky targets left partly short: big parents fill the sky often, but not so often the view turns ordinary [M5-09],
  [R-001]; replaced by [B-428]
- Every first moon, a solid world's as a giant's, orbits at 4.5-6 radii outside the rings (a parent at most ~26 degrees wide)
  [B-314]; replaced by [B-428]
- Variety comes from traits on the types: a new type only where it needs its own planet function, sky, environment or event [O10],
  [O11]; replaced by [P-00]
- The transmitter cells' salt is C-07's 0xC07F, pinned so home hears three peoples' signals (79, 85 and 179 ly) [C-07]; replaced
  by [A-01]
- The ship is always at a star: no state the game keeps (save, radar, system list) has a ship with no star under it [R-407];
  replaced by [D-01]
- The weak transmitters' power salt is 0xC15A0165, pinned together with the strong cells' 0xC1500165 [C-15]; replaced by [R-413]

## The planet and its surface

- Every fine feature, shaped ones (mesas, canyons, cones) too, fades with `lodFade` (twelve samples a wavelength) [table];
  replaced by [B-404]
- Rivers are a hashed worley network with local water levels, a pure function of position, not a traced flow map [M9-04], [O10];
  replaced by [O6-03..O6-07]
- The terrain caches are written by the main thread alone; a worker prefetches into a staging buffer the main thread copies in
  [M7-01]; replaced by [F-03], [F-04]
- Herds spawn round the landing only (within 180 m of the capsule), and the re-anchor clears them [N3]; replaced by [B-316]
- The 4 m near ring fades out with speed; above 20 m/s the buggy drives on the 16 m ring [O6-02]; replaced by [B-313]
- A lake is a feature with one level, the ground evaluated once at its centre (cached) [O10]; replaced by [O6-03..O6-07]
- Trait eligibility is a hand-made table of kinds by traits (`traitEligible`), not a weight matrix [O10]; replaced by [P-00]
- Cliff bands read the full relief spectrum's analytic slope [O6-03..O6-07]; replaced by [B-404]
- A 14 km cell holds one landmark, the first of a fixed order of kinds; its lake is the centroid of the drainage's lake cells
  [O6-03..O6-07]; replaced by [R-408]
- The landmark search runs on a thread at every landing [O6-03..O6-07]; replaced by [R-408]
- The warm's disc reaches two cells past the radius; a ring's square of 2r + 3 cells fits its torus [O6-03..O6-07], [F-03];
  replaced by [F-04]
- A collider is a disc on the ground that blocks at any height [C-01]; replaced by [B-405]
- A people's ground is never edited: nothing is flattened and the terrain cache is never touched (terraces are walls) [C-01],
  [C-08]; replaced by [T-02], [T-03]
- Only the terrain is a floor: no piece is stood on (a heap or a bank is a wall of discs) [B-405], [C-08]; replaced by [B-420]

## The civilisations and their grammar

- M4-07's monoliths keep their draw to the bit; a civilisation world's settlements take the ruin cells the monoliths leave [C-01];
  replaced by [B-417]
- A ruin's decay is a draw of its culture [C-01]; replaced by [C-08]
- A world's settlements are its ruin cells' own hash, the whole world over (hamlets, villages, towns by their shares) [C-01];
  replaced by [T-01]
- A settlement's plan places its buildings, and its streets are the gaps between them [C-01]; replaced by [T-02]
- A shard's text is a template of a kind drawn by tone, its slots filled from the world's data (`Grammar`, `tidy`, `kinds()`)
  [C-02]; replaced by [H-03], [H-04]
- A shard is a fragment: 10-40% of its words lost as gaps of dots (`Shard::fragment`) [C-02]; replaced by [C-06]
- The end's tone falls to a share of a world's late shards, so several of them say they are the last [C-02]; replaced by [C-12]
- A world's names (the lore's eleven) come from one name generator in three styles, keyed by the galaxy's region [C-02]; replaced
  by [H-04 second pass]
- A shard's year counts the lore's own span, 150-600 years from the founding [C-02], [A-01]; replaced by [T-04]
- A world's catalogue is fifty shards (`SHARDS_PER_WORLD`), a hundred for two peoples (the second's at 50-99), copies in
  settlements [C-03], [C-13]; replaced by [T-04]
- The tongue is a function of the world and the word (`tongueWord`): one tongue a world, no two worlds' alike [C-06]; replaced by
  [C-13], [H-04 second pass]
- Grammar kinds append to the kinds table, a new kind a candidate only under its condition (an other people named) [C-13];
  replaced by [H-03], [D-05]
- The programmes on the air are C-07's whatever the people's age [A-01]; replaced by [C-16]
- Ruins weather by the years since the end over a durability by the age (`ageDurability`: 1, 1, 1.5, 2.5, 6, 40) [A-01]; replaced
  by [A-03]
- A people's world keeps its earlier ages on the ground: medieval cores in a modern city, megaliths in the fields [A-01]; replaced
  by [B-414]
- A world's second people draws its own age: the age is the people's, not the world's [A-01]; replaced by [R-413]
- Two peoples ending together draw one cause from the world's seed over both ages (a prehistoric pair may end in a nuclear war)
  [A-01]; replaced by [R-413]
- A people's genome speaks as loudly in every age on every building: a people of domes builds them in each age's material [A-04];
  replaced by [B-415]
- A history is built when asked, not prefetched at the approach [H-02]; replaced by [T-01], [T-02]
- A city founded in an older age is built in rings of each age's way (old windows in slot 3); the countryside keeps older villages
  [T-02]; replaced by [B-414]
- A world's works are one list (`worldWorksOf`), its everyday works among them, a handful of each a people [T-03]; replaced by
  [B-418]
- A people's first fifty records keep C-02's texts [T-04]; replaced by [H-03], [H-04]
- A record is dated over its people's years from the founding to the end [T-04]; replaced by [B-414]
- Past the first fifty, music and charts come at the people's own shares of its fifty [T-04]; replaced by [H-05]
- A found instrument is one of 49 kinds, chosen by the age with a coin among a family's few [C-11]; replaced by [C-17]
- Industrial and modern dwellings take the people's roof where the age built so, else the age's (`ageRoof`) [B-414]; replaced by
  [B-415]
- A drifter's people came from the star nearest its way's end, of any class (`drifterPast`) [D-01]; replaced by [D-05]

## The game and its screens

- The sector map draws a trail of where the explorer walked, a point every 25 m [M4-02]; replaced by [R-414]
- `Ctrl+T` runs a 25-second time-lapse of the sky at x600 with a still camera, then returns to x1 [M5-06]; replaced by [W-01]
- The gamepad mapping is fixed in code, not rebindable [M6-02]; replaced by [R-418]
- The ground's view carries the rangefinder, the sector line, the life sensor and the old road on the HUD [O1], [O2], [B-321],
  [C-09]; replaced by [R-414]
- The buggy's and the drone's views show channel, pan and tilt, trip, gear, heading, state, the capsule and the local time
  [R-204], [R-403]; replaced by [R-414]
- The maps show the landmarks as markers, only those already found (`landmarksSeen`: logged within a kilometre, or named) [O6-06],
  [B-401]; replaced by [R-408]
- The aim's markers name an unnamed star UNKNOWN beside its distance [R-401]; replaced by [R-407]
- A first reading waits 0.11 s a word while the words resolve left to right; Enter skips it, Esc stops it [C-06]; replaced by
  [C-05]
- The decoder's medium line names the voice that reads a record, or says it is read or played by the ship [C-05], [H-05]; replaced
  by [R-414]
- The radar's hold is on the strongest signal within the lock cone [C-07]; replaced by [C-15]
- The landing map shows every mark, ruins' and works' too, and its zoom every road unfound, as terrain like the rivers [C-09],
  [B-412]; replaced by [R-415]
- A world's second people keeps its shards at 50 + index under the same keys (`<body>/S57`) [C-13]; replaced by [T-04]
- Opening the telescope tracks the body under the crosshair at once (`telescopeAim`); Enter lets go or tracks the reticle's target
  [W-06]; replaced by [R-405]
- At a deep power the telescope holds the ground under the reticle, a body-frame point the ship is re-aimed at [W-06 reworked];
  replaced by [R-405]
- The eight nearest marks within 25 km are drawn over the view, each with its name and distance, a new mark at once [R-408];
  replaced by [B-412]
- The guide keeps the arrivals' order as `route` lines (`Guide::route`, at most 10,000) and the chart draws the legs between them
  [G-05]; replaced by [R-414]
- The case's instrument line says where it came from: `FROM <world> (<sector> / <body>)` [C-17]; replaced by [R-414]
- The drone bay's rack keeps a trip's finds and places on its screen until the next trip [C-18]; replaced by [R-414]

## Audio

- A world's voice comes from the body's seed, leaning with its names' style as the music's timbres do [C-05]; replaced by [C-13],
  [H-04 second pass]
- A piece and a voice are heard on the decoder's screen alone: `Game::frame` stops them off it [C-04], [C-05]; replaced by [C-11]
- A world's tradition comes from the body's seed and its lore's name style (`traditionOf`) [C-04]; replaced by [C-13]
- The synth cannot seek: a recording starts from its top, and a radar lock restarts it [C-07]; replaced by [C-11]
- A drifter (`SIG_ROGUE`, the synth's case 4) sounds as a magnetosphere by its own seed until it has a voice [D-02]; replaced by
  [D-03]

## The harness and the proof

- Performance budgets are generous: they catch a pathological regression, not a 20% slowdown, which is read from the printed
  timings [M7-04]; replaced by [F-03], [F-11]
- The worst-case forest frame is budgeted at 14 ms at 2x [N2-04]; replaced by [B-310], [F-11]
- The forest bench at 4x is budgeted at 24 ms [B-310]; replaced by [F-11]
- The descent's first frame (four rings warmed, drainage included) is budgeted at 80 ms [O6-03..O6-07]; replaced by [F-03], [F-11]
- The bench's budget check waits five seconds on the landing map before the descent [G-01]; replaced by [G-02]
- The bench holds a `drone 2x` budget of 16 ms [R-403]; replaced by [F-11]
- `bench check` budgets `telescope 2x` and `probe 2x` at 16 ms each [W-06], [X-02]; replaced by [F-11]
- Bench budgets: `vimana 2x` 12 ms, `city 2x` and `capital 2x` 16 ms, `descent people` 80 ms [G-07], [A-02], [T-01]; replaced by
  [F-03], [F-11]
- A change is proved by `regress` both ways, the 2x and 4x `scene` frames against the last binary (`pngdiff`), the unit and
  `bothways` [F-01]; replaced by [E-02]
- The unit's six scanner and marks frames follow what a previous run left in `shots/tests`: compared only from that state [F-01];
  replaced by [E-02]
- The flow is compared only from one restored copy of `shots/tests` (rsync), beside a baseline-against-baseline run [F-01],
  [F-07]; replaced by [E-02]
- The descent's first frame is budgeted at 35 ms, the descent onto a people's world (`descent people`) at 70 ms [F-03]; replaced
  by [F-11]
- `build/perf-baselines/` keeps each milestone's binary (`vesperis_test_p00`, `_p01`...) for the timing pairs [P-00]; replaced by
  [E-02]
- A generation step is proven by kits rebuilt in `build/<id>/` (the draw stream, `fp*.cpp`) against a copy of the old sources
  [P-01..P-07], [Q-01], [Q-02]; replaced by [E-02]
- An elsewhere kind's sheet, scenes and unit checks stand on a staged felisian world (`stageElsewhere`) [P-04]; replaced by [P-05]
- The chemotroph's north sweep frame flips between runs (the far ring's timing): its difference is no change [P-04], [P-06],
  [P-07], [Q-01]; replaced by [E-02]
- `tour` opens every state as the game does, each from a fresh game, by its stop in `STOPS` (asserted: a new state needs one)
  [U-01]; replaced by [U-06]

## Process and release

- A milestone's entry ends with What moved (GEN_VERSION, the regress, files and names, modes and checks) and the KI rows it leaves
  [G-01], [G-04], [B-404], [C-01], [C-07], [C-08], [C-09], [C-10], [C-13], [G-05..T-04]; replaced by [E-04]
- Release zips are built on the merged tree, so they carry the tagged commit [Release 1.1.0], [Release 1.2.0], [Release 1.3.0];
  replaced by [E-01]
- A peoples milestone puts its numbers, and where to see them, in its own section of reference/12-civilisations.md [A-01], [C-15],
  [C-16], [A-02], [A-03], [A-04], [T-01], [H-02], [T-02], [T-03], [T-04]; replaced by [E-04]
- A milestone's log entry ends with "Found on the way, not changed:", what was seen and left as it is [P-00..P-06]; replaced by
  [E-04]
- The harness's traps are a path-scoped rule, .claude/rules/harness.md, loaded with src/main_headless.cpp [E-01]; replaced by
  [E-03]
- make clean is refused until the Makefile spares build/toolchains [E-01]; replaced by [E-02]
- A milestone or a bug writes a PROGRESS.md bullet before its ## Next, whose paragraph is the state [E-01]; replaced by [E-04]
- Every decision is an entry at the end of reference/10-decisions.md, whose section a rule's change reads first [E-01]; replaced
  by [E-04]
- Rules to keep go in the CLAUDE.md files, their detail in reference/lessons.md by area [E-01]; replaced by [E-04]
- A subsystem split over files shares its helpers through its `_internal.h` (`inline`); a helper one file uses stays in it [E-06];
  replaced by [E-07]

[M4-02]: ../archive/iteration-1/plans/ROADMAP.md
[M5-06]: ../archive/iteration-1/plans/ROADMAP.md
[KI-309]: ../iteration-3/bugs/KNOWN-ISSUES.md
[B-305]: log/0-1.0.0.md
[B-309]: log/0-1.0.0.md
[B-310]: log/0-1.0.0.md
[B-311]: log/0-1.0.0.md
[B-313]: log/0-1.0.0.md
[B-314]: log/0-1.0.0.md
[B-316]: log/0-1.0.0.md
[B-321]: log/0-1.0.0.md
[B-401]: log/0-1.0.0.md
[M10-01]: log/0-1.0.0.md
[M10-02]: log/0-1.0.0.md
[M10-03]: log/0-1.0.0.md
[M10-12]: log/0-1.0.0.md
[M5-09]: log/0-1.0.0.md
[M6-02]: log/0-1.0.0.md
[M7-01]: log/0-1.0.0.md
[M7-04]: log/0-1.0.0.md
[M9-04]: log/0-1.0.0.md
[N2-04]: log/0-1.0.0.md
[N3]: log/0-1.0.0.md
[N5-02]: log/0-1.0.0.md
[O0-02]: log/0-1.0.0.md
[O1]: log/0-1.0.0.md
[O10]: log/0-1.0.0.md
[O11]: log/0-1.0.0.md
[O2]: log/0-1.0.0.md
[O3]: log/0-1.0.0.md
[O6-02]: log/0-1.0.0.md
[O6-03..O6-07]: log/0-1.0.0.md
[O6-06]: log/0-1.0.0.md
[R-001]: log/0-1.0.0.md
[R-204]: log/0-1.0.0.md
[R-401]: log/0-1.0.0.md
[Release 1.1.0]: log/0-1.0.0.md
[Release 1.2.0]: log/0-1.0.0.md
[Release 1.3.0]: log/0-1.0.0.md
[table]: log/0-1.0.0.md
[B-404]: log/1-1.1.0.md
[B-405]: log/1-1.1.0.md
[C-01]: log/1-1.1.0.md
[C-02]: log/1-1.1.0.md
[C-03]: log/1-1.1.0.md
[C-04]: log/1-1.1.0.md
[C-05]: log/1-1.1.0.md
[C-06]: log/1-1.1.0.md
[C-07]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-09]: log/1-1.1.0.md
[C-10]: log/1-1.1.0.md
[C-12]: log/1-1.1.0.md
[C-13]: log/1-1.1.0.md
[G-01]: log/1-1.1.0.md
[G-02]: log/1-1.1.0.md
[G-03]: log/1-1.1.0.md
[G-04]: log/1-1.1.0.md
[R-403]: log/1-1.1.0.md
[S-05]: log/1-1.1.0.md
[R-405]: log/2-1.2.0.md
[R-407]: log/2-1.2.0.md
[R-408]: log/2-1.2.0.md
[W-01]: log/2-1.2.0.md
[W-06]: log/2-1.2.0.md
[W-06 reworked]: log/2-1.2.0.md
[X-01]: log/2-1.2.0.md
[X-02]: log/2-1.2.0.md
[A-01]: log/3-1.3.0.md
[A-02]: log/3-1.3.0.md
[A-03]: log/3-1.3.0.md
[A-04]: log/3-1.3.0.md
[B-412]: log/3-1.3.0.md
[B-414]: log/3-1.3.0.md
[B-415]: log/3-1.3.0.md
[B-417]: log/3-1.3.0.md
[B-418]: log/3-1.3.0.md
[B-420]: log/3-1.3.0.md
[C-11]: log/3-1.3.0.md
[C-15]: log/3-1.3.0.md
[C-16]: log/3-1.3.0.md
[C-17]: log/3-1.3.0.md
[C-18]: log/3-1.3.0.md
[G-05]: log/3-1.3.0.md
[G-05..T-04]: log/3-1.3.0.md
[G-07]: log/3-1.3.0.md
[H-02]: log/3-1.3.0.md
[H-03]: log/3-1.3.0.md
[H-04]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[H-05]: log/3-1.3.0.md
[R-413]: log/3-1.3.0.md
[R-414]: log/3-1.3.0.md
[R-415]: log/3-1.3.0.md
[T-01]: log/3-1.3.0.md
[T-02]: log/3-1.3.0.md
[T-03]: log/3-1.3.0.md
[T-04]: log/3-1.3.0.md
[F-01]: log/4-1.3.1.md
[F-03]: log/4-1.3.1.md
[F-04]: log/4-1.3.1.md
[F-05]: log/4-1.3.1.md
[F-06]: log/4-1.3.1.md
[F-07]: log/4-1.3.1.md
[F-11]: log/4-1.3.1.md
[B-425]: log/5-1.3.2.md
[B-427]: log/5-1.3.2.md
[B-428]: log/5-1.3.2.md
[R-418]: log/5-1.3.2.md
[D-01]: log/6-1.4.0.md
[D-02]: log/6-1.4.0.md
[D-03]: log/6-1.4.0.md
[D-05]: log/6-1.4.0.md
[E-01]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
[E-03]: log/6-1.4.0.md
[E-04]: log/6-1.4.0.md
[P-00]: log/6-1.4.0.md
[P-00..P-06]: log/6-1.4.0.md
[P-01..P-07]: log/6-1.4.0.md
[P-04]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[P-06]: log/6-1.4.0.md
[P-07]: log/6-1.4.0.md
[Q-01]: log/6-1.4.0.md
[Q-02]: log/6-1.4.0.md
[E-06]: log/6-1.4.0.md
[E-07]: log/6-1.4.0.md
[U-01]: log/6-1.4.0.md
[U-06]: log/6-1.4.0.md
