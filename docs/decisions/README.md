# Decisions

The rules in force, one line per rule with the log entries behind it, in one file per area of at most 6 KB. Read an area's file
before changing anything there.

- `rules-rendering.md`: the pixel and the palette, banks, light, one world in every view, small things and the mush.
- `rules-rendering-frame.md`: draw order and depth, the same pixels at any thread count, the pool, the frame's time, exact
  arithmetic.
- `rules-galaxy.md`: `GEN_VERSION`, draws, hashes and salts, protected and pinned worlds, stars and drifters.
- `rules-galaxy-worlds.md`: bodies and orbits, kinds and made kinds, the almanac and the giants.
- `rules-surface.md`: the planet function, materials and landforms, traits, water and drainage, sights, the activity vision,
  weather.
- `rules-surface-site.md`: the site's caches and threads, the rings, what stands and walks, vehicles, life.
- `rules-surface-animals.md`: the animals (1.5.0, J-06 on): the bestiary and each species' own seed, bodies of parts in sixteen plans, a
  world's style and hides, drawing on the pool.
- `rules-life.md`: microbes (1.5.0): where life is, species, geneses and kin, carried spores, the cache; forms and the slide;
  genomes and the tree, the made species.
- `rules-life-lab.md`: the search (samples, the kit, the ultraviolet, the signs) and the microscope (lists by world and folder,
  the four things done to a slide and how the cells answer).
- `rules-life-cultures.md`: the incubator (flasks, their names and choosers; J-13: the coldest a line grows) and the culture collection
  (kept automatically and by hand).
- `rules-life-record.md`: the sequencer and the tree; the record (the catalogue, the counts).
- `rules-life-genes.md`: genes (a genesis's map and code, a species' genes read from what it is, edit values, cuts), the gene
  editor and its lines, learning what a gene does.
- `rules-life-world.md`: the world genes (places 19 to 32, their wild values, what they do in a flask and how the lab shows them) and the
  seeded worlds' layer (what a seeding made of a world, where it is read and what keeps the world as generated): the J series.
- `rules-life-seeding.md`: sending a culture down (the drone bay's seed trip, the verdict, the drift, the spread; J-13: a place at its own
  warmth, down a drill hole; J-14: up into a cloud deck) and a seeded world's record, J-12 on (the natives died out, a line found wild, the
  catalogue's rows and the LIFE page's, auto-explore).
- `rules-life-made.md`: what a seeded world is made into, J-04 on: the made air and its climate (each gas's answer by its world, a
  runaway, what lift carries up, every kind's caps, the snow and the plants moved), the waters', the ground's and the plants' looks.
- `rules-life-made-drawn.md`: how a seeded world's made traits are drawn, J-04 on: the sky and clouds, the banks the made colours take on
  the ground and on the globe (a line each), the maps' ramps.
- `rules-life-overgrowth.md`: what a seeded line's overgrowth makes of a world, J-09 on: its cover and structures, the ruins it takes and
  crumbles, its sounds; J-15: a desert's carpet growing grass and shrubs.
- `rules-life-kinds.md`: a seeded world's made kind, J-11 on: the lines each kind passes (a hothouse, a snowball, the overgrown and the
  rest; J-15: a greened desert), the dead covers laid over a living ground, the data sheet's kind line.
- `rules-peoples.md`: who lived where and when, the plan and the history, what stands on the ground, buildings and the genome.
- `rules-peoples-grammar.md`: where the records lie, signals and charts, texts and their draws, the grammar's files, tongues.
- `rules-game.md`: the explorer's rules, saves and the guide, the HUD and screens, controls and the frame, auto-explore.
- `rules-game-devices.md`: the cabin's instruments, the radar, the decoder and the player, the probe and photos, flight, maps and
  marks.
- `rules-game-screens.md`: how a state is made: its row in the states' table and what the frame runs for it, menus and devices
  as rows, the controller page, how a screen opens and closes, and its own save lines.
- `rules-ship.md`: the Stardrifter's interior (1.5.0's I series): its plan as rows, the mesh and the frame, the walk and the places.
- `rules-audio.md`: the synth, levels, voices, music and instruments, radar and drifter sounds, the tape.
- `rules-harness.md`: harness code, the unit, the flow and the tour, frames that repeat.
- `rules-harness-scenes.md`: scenes, pinned sites and test worlds.
- `rules-harness-proof.md`: regress, the battery, proofs by area, reading a difference, the bench, timing pairs.
- `rules-process.md`: working with the user, proving and fixing, recording the work.
- `rules-process-build.md`: the platform and the names, the builds, warnings, releases.

`superseded.md`: the rules replaced, and by what. `generations.md`: what each `GEN_VERSION` changed. `log/`: every entry by
release, from `0-1.0.0.md` to the current release's, newest last. `grep -n '^## B-428' log/*.md` finds an entry; read that
entry alone, never a whole log.
