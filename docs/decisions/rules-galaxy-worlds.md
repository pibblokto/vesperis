# Rules: the galaxy's systems and worlds

The rules in force for the systems, their orbits and moons, the kinds of world, the almanac and the giants, a line each with the
entries behind them (`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to
`superseded.md` (`docs/CLAUDE.md`). GEN_VERSION, the draws, the protected worlds, the stars, the drifters: `rules-galaxy.md`.

## Bodies and orbits
- Body indices never change: new bodies (a far companion's worlds, a wanderer) come after every other body, on streams of their own [B-425], [P-05]
- Worlds are typed by their primary's light alone; the companion reads the primary's worlds, drawn first, never an orbit drawn after it [B-425]
- A far companion spans 3 degrees at most from the farthest world and warms it 100 K at most; its own worlds keep within a third of the gap [B-425]
- A close companion has a tenth of the primary's light at most and is no wider than it from the first world, else it is far (a black hole's stays) [B-425]
- A first moon orbits outside its parent's rings: a giant's at 4.5-6 radii, a solid world's at 5-40 (log-uniform), a twin at 12-20 [B-314], [B-428]
- A drawn moon orbits beyond the parking (3.5 radii): from a parking it passes behind its world, never across it [W-01]
- Moon periods follow Kepler; a locked moon's day is its month, never a constant fitted to keep older days [B-428]
- Facts (`bodyFacts`, `Body::tideForcingM`, the magnetic field) derive from the orbit, the spin and the seed, and take no draw [B-403], [Q-01], [Q-02]
- A wanderer has no moons and keeps B-425's clearance (a system where that fails keeps none); it and drifters alone carry the frozen felisian [P-05], [D-01]
- A red dwarf's locked planets are never retrograde and keep eccentricities x0.3, so M5-08's lock test holds [S-01]
- Solid worlds wear rings one time in eight, so a ringed world can be landed on (the user's ask) [O0-01]
- Thresholds are set on the game's compressed scales (inner orbits 350-500 K, days 20 min-4 h, a fictional AU), not on real worlds' [table], [P-01], [Q-01]
- Left as drawn, each a generation change for the user to call: comets key to the last body (KI-416), locked worlds keep their drawn tilt [B-428], [P-01]

## Kinds and made kinds
- A new kind is a `KINDS` row (`buildKinds`) on a shared ground, its own only where none fits; code reads rows or `KT_` tags, no switch or list [O11], [P-00]
- Made kinds take no draw: `generate` draws each body as its drawn kind, then `kindOf` remakes it by hashes after its last draw [P-01], [P-03], [P-06]
- `kindOf` takes the first made kind in id order whose source fits and hash is under its share; a new source (`KB_CAPTURED`) is no earlier kind's [P-01], [P-05]
- A new made kind's salt takes no protected world: `salt scan` gives the first from its own that takes none, then pinned [E-07]
- System rules (a giant's spacing, the moons, the twin, the glassing, a dark kind's power) read `Body::drawnType`, never the made kind [P-01], [D-04]
- A made world moves no star or comet (they read moons as drawn: `drawnOrbitKm`); a moon it would reach moves out to 3 radii, past its rings [P-01], [P-03]
- A made row has a family, a salt and sources, never a weight, zone or rule (`kindsOk`); an `elsewhere` row (the frozen felisian) has no source [P-01], [P-04]
- No kind with an air comes round a black hole or in a Wolf-Rayet wind (`KEEPS_AIR`); no sun-fed life without a sun (a chemotroph) [P-03], [P-05], [D-01]
- A twin is never made a locked kind or a giant: it is made again as drawn (`asDrawn`, `drawnFacts`, `KindFacts::paired`), with no draw [P-03], [P-07]
- `KindFacts`' lock and sync stay the star's (a companion's worlds are never locked); a moon's own facts go in `KindFacts::moon` [P-06], [P-07]
- Every reader of an air asks `bodyHasAir` (false where `airFrozen`: a drifter's cold); a sea under its `seaFreezeK` freezes; `TK_DARK` traits are given [D-04]
- A trait that depends on the star is forced: the system marks the body (`Body::glassed`), `BodyGen::make` puts it first [S-03]
- Trait slots take categories in each world's hashed order: filling an empty category takes the slots it heads, displacing their traits [Q-01], [Q-03]
- `isIceGiant` reads a drawn gas giant's radius (under 45,000 km) and blue: change either, the kind follows; an azure giant or gas dwarf is none [X-03], [P-07]

## The almanac and the giants
- Each view of a thing (globe, maps, ground, almanac, probe) reads one function, not a copy: `cometActivityAt`, `stormShare`, `giantJetAt` [W-04], [X-03]
- The almanac's finders repeat the sky's geometry (`SurfaceSite::sun`, the cover, the ring and shadow rules): change one, change both [W-01]
- An event kind is an `AL_*` entry, its finder in `registry()` in a fixed order (`findFronts`, `findShowers` last); nothing registers at start-up [W-01], [E-02]
- What moves is a function of time: belt rocks hashed per ring at its own rate, meteors (`showerMeteorsAt`) from the orbits (`meteorShowersOf`) [O3], [W-04]
- The probe's fall is closed-form in its clock (`probeBarAt`): all it shows derives from it and the chute's clocks, all a save or a skip needs [X-01]
- No life in a giant's clouds: one ambiguous pale sight at most (a giant in 40), never named, logged, sounded or explained [X-03]

[table]: log/0-1.0.0.md
[O0-01]: log/0-1.0.0.md
[O3]: log/0-1.0.0.md
[O11]: log/0-1.0.0.md
[B-314]: log/0-1.0.0.md
[B-403]: log/1-1.1.0.md
[S-01]: log/1-1.1.0.md
[S-03]: log/1-1.1.0.md
[W-01]: log/2-1.2.0.md
[W-04]: log/2-1.2.0.md
[X-01]: log/2-1.2.0.md
[X-03]: log/2-1.2.0.md
[B-425]: log/5-1.3.2.md
[B-428]: log/5-1.3.2.md
[P-00]: log/6-1.4.0.md
[P-01]: log/6-1.4.0.md
[P-03]: log/6-1.4.0.md
[P-04]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[P-06]: log/6-1.4.0.md
[P-07]: log/6-1.4.0.md
[Q-01]: log/6-1.4.0.md
[Q-02]: log/6-1.4.0.md
[Q-03]: log/6-1.4.0.md
[D-01]: log/6-1.4.0.md
[D-04]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
[E-07]: log/6-1.4.0.md
