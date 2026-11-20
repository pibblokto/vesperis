# src/life: the living small (Z-01, Z-02, 1.5.0)

Microorganisms on every world, drawn from the hashes (R-425; `docs/ideas/IDEAS-small-and-spacious.md` section 1), and the slide
they are seen on, a vial's slide (Z-04). It reads `src/core` and `src/galaxy` only (`tools/check.py`'s layering); the harness reads it,
and from Z-03 the surface and the game.

## Files
- `biota` (`BIOTA`, a row per kind: the stages its worlds reach, its chemistry, its microhabitats; `biotaOf`, `lifeOf`: a world's
  stage, chemistry, genesis and species, a system's drawn together and cached; `shipSpecies`, `biotaMeanC`; the enums' names)
- `lineages` (the geneses: `systemGenesis`, `cellGenesis` in kin cells of `KIN_CELL` sectors, `wandererGenesis`, `homeGenesis`)
- `morphs` (Z-02: a species' form, `Morph`, about forty values in groups by its plan, stage, chemistry, genesis's style and seed;
  `morphsOf` a world's kept apart, `shipMorph`; `morphDistance`, `MORPH_ALIKE`; the ways of seeing and the forms' enums)
- `samples` (Z-03: `Microhabitat`, `sampleOf` what a vial holds, the activity, `placeKey` the square metre; `filmDensity`,
  `filmGlowColour` for the lamp's ultraviolet)
- `slides` (Z-04: a vial's slide: `mountOf` its mount (the preparation, each species' tile and cells, the first look), `SlideEvent`
  what is done to it, `slideStateAt`, `mountSpecimens` the specimens in a view at a time, `mountSpecimen` a tracked copy,
  `depthOfField`; Z-05: a `Held`'s form and dividing share, a culture's drop; R-438: the stage set to a warmth, a kind's pace clock
  (`PaceClock`, `Specimen::clock`), every turner answering, the dark's fleeing, lit light-feeders' gift; R-437: `slideAnswer`,
  `slideScentUm`)
- `cultures` (Z-05: a flask's culture: `Medium` (the cells, lights, energies, salts), `CultureKind`, `growthIn` a species' rate and crowd in
  a medium, `pairOf` two in a flask, `CultureEvent` what is done to a flask, `runCulture` its laws laid out, `cultureAt` its state at a time,
  `cultureStagesBy`, `applyStrain`; `mediumOfPlace` AS ITS WORLD) and `jars` (Z-05: a world in a jar: `jarRecipe` its bands, `jarAt` them at a
  time; R-439: `JarLayer`, `jarLayerOf` a band's place by what its species lives on, each place's bands under the one before)
- `sequences` (Z-06: a species' genome and its place in its genesis's tree: `Lineage`, `lineageOf`, `shipLineage`; `commonAncestorGyr`,
  `treeBefore` the tree's order; the marker's letters `markerLetter`, `markerOf`, `markerAlike`; `genomeLetters`, `watermarkLetters`,
  `alphabetOf`)
- `genes` (Y-02: a species' genes: `GeneCode` a genesis's map and code, `microbeGenesOf` a species' genes read from what it is (its wild
  letters its own values), `applyEdits` a line's values from its edits, `strainGenes` a strain's from its move, `genesDiffer`,
  `geneValueWords`; `fleesLight`, the dark-dweller's flight R-438 drew)
- `morph_draw.h` (the public slide: `Slide`, `Specimen`, the acts, `slidePalette`, `drawSlide`, `magnificationFor`, `seeingsFor`,
  `specimenExtentUm`, `specimenBox`, `specimenPlace`, `seeingBanks`)
- `morph_shape.h` (internal: what a specimen is at a time: bodies, dots, strands, waves in slide micrometres; the slide's banks),
  `morph_shape_internal.h` (the builder, the pose, a cell's frame for its insides, `CellCtx`), `morph_shape.cpp` (distances, the
  ornaments, the pose by motion), `morph_plans.cpp` (every plan's cell and its insides), `morph_acts.cpp` (the forms of the life
  cycle, the acts, `buildShape`, the extents), `morph_draw.cpp` (the palettes, the light ways per pixel, the section),
  `morph_mesh.cpp` (the electron's mesh)

## Rules and traps
- Generation 17 is the biota: drawn on salts of its own after every draw of 16, from the body, its system and `BodyGen::make`'s
  traits, never inside a stream of the galaxy's. A new decision takes a salt of its own (`SALT_*` in the .cpp files); a species
  draws from its own stream (`SALT_SPECIES` and its index), so a field added at a stream's end moves no other species.
- The enums are saved keys from Z-04 (the guide's species): appended before `*_COUNT`, never inserted. A world's species keep
  their indices: its own first, then its fossils, then the spores carried to it (`fromBody`, `fromIndex`).
- No life in a giant's clouds (X-03), none fed by light without a sun or under ice, none on a furnace; nothing is named here
  (the names are the explorer's, Z-04). Home's genesis is pinned by two salts (`HOME_GENESIS_ID`, the unit's `life` group).
- `BIOTA` is built at compile time (`biotaOk`: a row a kind in order, a furnace sterile, every row with habitats a stage or the
  spores); the unit holds it against `KINDS` (every landable kind has habitats, every other none).
- `lifeOf` is keyed by the star and every body's seed, kind and parent (a staged body is another system); its pointer keeps a
  system's biotas alive while held.
- The harness includes every module's headers: a name another module has clashes (the bestiary's `Species` and `PLAN_NAMES`, the
  peoples' `STAGE_NAMES`), hence `Microbe`, `MicrobePlan`, `LIFE_STAGE_NAMES`. Species are drawn only in a world's livable
  habitats (a venusian's ground burns under its living clouds).
- A form's groups each draw from their own stream (`SALT_MORPH` and the group): a value added at a group's end moves no other.
  A world's forms are drawn again (a nudge) while one is alike an earlier; a carried spore takes its source world's form.
- `unitFromHash(h >> n)` spans [0, 2^-n) alone: the drawing's `h01` re-mixes first (a shifted hash made every grain pyrite).
- A slide is a pure function of its view, its specimens and the clock (no state between frames; rows pure): the unit holds a frame
  the same reached by skipping and at one thread. Thin things are sized in screen pixels (strands `g_mushKernel + 1` wide, dots
  that radius at least) and fade as they widen.
- A cell's insides go in its own frame (`frameOf(B, body, ...)`: its centre and axis in the builder's coordinates); a colony's
  cells each have one. `using namespace morph` meets the galaxy's `Body`: write `morph::Body`.
- `specimenExtentUm` is what the unit's bounds check holds every pixel to: a builder that reaches further widens it, or stops
  its tips there (the mycelia).
- The electron skips gel (`Look::gel`: a colony's jelly, a slime trail); a colony's cells carry their height on its sphere (`lift`).
- A sample is a pure function of its microhabitat and the biota (`SALT_SAMPLE` by the place and the species' index): nothing of it is
  saved but the place and the microhabitat (the vial's line). So is a slide's mount (`mountOf`); where its cells are and what they do
  is closed-form from its events and the clock (`mountSpecimens`). The Specimen's and the Slide's Z-04 fields (`still`, `wander`,
  `vx`/`vy`, `blurUm`, `lod`, `step`; R-438's `clock`) default to the plates' drawing: `life_look` holds them. A slide with no setting of its
  stage keeps each kind's clock the slide's (its frames as before R-438); a kind's answer (`slideAnswer`) must be what its copies do (a
  magnetotactic kind answers the field alone, as its copies' branch in `copyAt` does).
- A `thread_local` named inside a pool's lambda is each worker's own: bind it to a reference first (`drawElectron`'s scratch). Red snow (`TR_SNOW_ALGAE`) puts the snow among a world's habitats and
  makes its snow species a red alga of the melt (the fixes after the draws); its cysts are carried as any spore former's.
- A culture is closed-form from its events (`runCulture`: segments, each one law; `cultureAt` a binary search and a formula): nothing is
  stepped a frame at a time, so the same state comes skipped, stepped or laid out again from the saved events (the unit's `lab` group).
  An event the clock reaches (a strain) is appended at its own time, never the frame's. The harness meets the peoples' `Culture` and
  the probe's `CS_` and the pad's `PR_` names: hence `FlaskCulture`, `SG_` (the stages) and `PA_` (the pairs).
- A lineage is closed-form from the biota and salts of its own (`SALT_*` in sequences.cpp; nothing of 17 drawn again): a genesis's deep
  branches, a kin cell's boxes of sixteen and four sectors, a star's line (a drifter's: the star it left), its worlds' last exchange, a world's
  comb (its species' order and the heights between neighbours, the ship's three on home's world). Keep every height under the one above it
  (a comb's under the exchange, the exchange under the near box's): the unit's ranking check holds it. A carried spore is its source's leaf.
  A marker's letter walks its branches root first; find a lineage's comb path once a run (`pathOf`), never a letter at a time.
- The made species are the peoples' (peoples/made.h, the one file there that reads life): this module never names a people.
- The genes draw nothing of 17: a species' are read from its values and form on salts of their own (`SALT_GENE_*`); an unedited line is
  its species bit for bit (the unit's `genes` group over forty worlds: its values, its culture's segments, its slide's kinds). An edit is
  relative to its parent's letters, so a strain's exact move and the wild value hold; a cut gene stays cut down its lines. The harness
  meets the peoples' `Genome` and `genomeOf` and the giants' `GK_`: hence `MicrobeGenes`, `microbeGenesOf`, `GN_` and `SE_`. A new field
  of `Microbe` or `CultureKind` (Y-02's `flees`, `dead`, `quorum`) defaults to what the wild species was.
