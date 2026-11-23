# Rules: the galaxy's systems and worlds

Rules in force for systems, orbits and moons, kinds of world, the almanac and the giants, one line per rule with its log entries
(`log/`, by release). A milestone that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md`
(`docs/CLAUDE.md`). GEN_VERSION, draws, protected worlds, stars and drifters are in `rules-galaxy.md`.

## Bodies and orbits
- Body indices never change: new bodies (a far companion's worlds, a wanderer) come after all others, on their own streams [B-425], [P-05]
- A world's type depends only on the primary's light; the companion reads the primary's worlds, drawn first, never an orbit drawn after it [B-425]
- From the farthest world a far companion spans at most 3 degrees and adds at most 100 K; its own worlds stay within a third of the gap [B-425]
- A close companion has at most a tenth of the primary's light and looks no wider than it from the first world, else is far (a black hole's stays close) [B-425]
- A world's first moon orbits outside its rings: a giant's at 4.5-6 radii, a solid world's at 5-40 (log-uniform), a twin at 12-20 [B-314], [B-428]
- A drawn moon orbits beyond the parking orbit (3.5 radii), so seen from parking it passes behind its world, never in front [W-01]
- Moon periods follow Kepler; a locked moon's day equals its month, never a constant fitted to keep older days [B-428]
- Facts (`bodyFacts`, `Body::tideForcingM`, the magnetic field) come from orbit, spin and seed, and take no draw [B-403], [Q-01], [Q-02]
- A wanderer has no moons and keeps B-425's clearance (a system where it fails gets none); only wanderers and drifters carry the frozen felisian [P-05], [D-01]
- A red dwarf's locked planets are never retrograde and have eccentricities scaled x0.3, so M5-08's lock test holds [S-01]
- One solid world in eight has rings, so a ringed world can be landed on (the user's ask) [O0-01]
- Thresholds are set on the game's compressed scales (inner orbits 350-500 K, days 20 min-4 h, a fictional AU), not on real worlds [table], [P-01], [Q-01]
- Two things stay as drawn until the user calls a generation change: comets keyed to the last body (KI-416) and locked worlds' drawn tilt [B-428], [P-01]

## Kinds and made kinds
- A new kind is a `KINDS` row (`buildKinds`) on a shared ground unless none fits; code reads rows or `KT_` tags, never a switch or a list [O11], [P-00]
- Made kinds take no draw: `generate` draws each body as its drawn kind, then `kindOf` remakes it by hashes after the last draw [P-01], [P-03], [P-06]
- `kindOf` picks the first made kind by id whose source fits and hash is under its share; a new source (`KB_CAPTURED`) belongs to no older kind [P-01], [P-05]
- A new made kind's salt takes no protected world: `salt scan` finds the first salt, counting from the kind's own, that takes none; it is then pinned [E-07]
- System rules (giant spacing, the moons, the twin, glassing, a dark kind's power) read `Body::drawnType`, never the made kind [P-01], [D-04]
- A made world moves no star or comet (they read moons as drawn: `drawnOrbitKm`); a moon it would reach moves out to 3 radii, past the rings [P-01], [P-03]
- A made row has a family, a salt and sources, never a weight, zone or rule (`kindsOk`); an `elsewhere` row (the frozen felisian) has no source [P-01], [P-04]
- No kind with air forms round a black hole or in a Wolf-Rayet wind (`KEEPS_AIR`); without a sun, life is chemotrophic, never sun-fed [P-03], [P-05], [D-01]
- A chemotroph is made only from a cool world, under 300 K as drawn (its mean under 50 C): a hot one stays as drawn [B-504]
- A twin is never made a locked kind or a giant: it is remade as drawn (`asDrawn`, `drawnFacts`, `KindFacts::paired`), with no draw [P-03], [P-07]
- Lock and sync in `KindFacts` stay the star's (a companion's worlds are never locked); a moon's own facts go in `KindFacts::moon` [P-06], [P-07]
- Code that reads air asks `bodyHasAir` (false where `airFrozen`, in a drifter's cold). A sea below its `seaFreezeK` freezes; `TK_DARK` traits are given [D-04]
- A trait that depends on the star is forced: the system marks the body (`Body::glassed`) and `BodyGen::make` puts the trait first [S-03]
- Trait slots take categories in each world's hashed order; filling an empty category takes the slots it heads and displaces their traits [Q-01], [Q-03]
- `isIceGiant` reads a drawn gas giant's radius (under 45,000 km) and blue tint, and the kind follows; an azure giant or gas dwarf is not one [X-03], [P-07]

## The almanac and the giants
- Every view of a thing (globe, maps, ground, almanac, probe) reads one function, not a copy: `cometActivityAt`, `stormShare`, `giantJetAt` [W-04], [X-03]
- The almanac's finders repeat the sky's geometry (`SurfaceSite::sun`, cover, ring and shadow rules): a change to one changes both [W-01]
- An event kind is an `AL_*` entry; its finder sits in `registry()` in a fixed order (`findFronts`, `findShowers` last), none added at start-up [W-01], [E-02]
- Moving things are functions of time: belt rocks hashed per ring at that ring's rate, meteors (`showerMeteorsAt`) from orbits (`meteorShowersOf`) [O3], [W-04]
- The probe's fall is closed-form in its clock (`probeBarAt`): what it shows comes from that and the chute clocks, all that a save or skip needs [X-01]
- No life in a giant's clouds: at most one ambiguous pale sight (one giant in 40), never named, logged, sounded or explained [X-03]

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
[B-504]: log/7-1.5.0.md
