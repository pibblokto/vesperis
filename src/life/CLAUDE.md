# src/life: the living small (Z-01, Z-02, 1.5.0)

Microorganisms on every world, drawn from the hashes (R-425; `docs/ideas/IDEAS-small-and-spacious.md` section 1), and the slide they are
seen on, made from a vial (Z-04). It reads only `src/core` and `src/galaxy` (`tools/check.py`'s layering). The harness reads it, and
since Z-03 the surface and the game do too.

## Files
- `biota` (`BIOTA`, a row per kind: the stages its worlds reach, chemistry, microhabitats. `biotaOf`, `lifeOf`: a world's stage,
  chemistry, genesis and species, drawn and cached a system at a time. `shipSpecies`, `biotaMeanC`, the enums' names)
- `lineages` (the geneses: `systemGenesis`, `cellGenesis` in kin cells of `KIN_CELL` sectors, `wandererGenesis`, `homeGenesis`)
- `morphs` (Z-02: a species' form. `Morph`: about forty values in groups, by plan, stage, chemistry, genesis style and seed. `morphsOf`:
  a world's forms, kept apart. `shipMorph`, `morphDistance`, `MORPH_ALIKE`, the enums of the ways of seeing and the forms)
- `samples` (Z-03: `Microhabitat`, `sampleOf` (what a vial holds), the activity, `placeKey` (the square metre), `filmDensity` and
  `filmGlowColour` for the lamp's ultraviolet)
- `slides` (Z-04: a vial's slide. `mountOf` makes its mount: the preparation, each species' tile and cells, the first look.
  `SlideEvent`: what is done to a slide. `slideStateAt`, `mountSpecimens` (the specimens in a view at a time), `mountSpecimen` (a
  tracked copy), `depthOfField`. Z-05: a culture's drop, with each `Held`'s form and dividing share. R-438: the stage set to a warmth, a
  pace clock per kind (`PaceClock`, `Specimen::clock`), every cell that turns to light or food answering, dark-dwellers fleeing light,
  lit light-feeders drawing eaters. R-437: `slideAnswer`, `slideScentUm`)
- `cultures` (Z-05: a flask's culture. `Medium` (cells, lights, energies, salts), `CultureKind`, `growthIn` (a species' rate and crowd
  in a medium), `pairOf` (two in a flask), `CultureEvent` (what is done to a flask), `runCulture` (lays out a culture's laws),
  `cultureAt` (the state at a time), `cultureStagesBy`, `applyStrain`, `mediumOfPlace` (AS ITS WORLD). Y-04: `GeneSwitch`, a line's
  switches as its culture reads them (`CultureSwitch`, `switchedKind`, `cultureWorkOff`), `CultureState`) and `jars` (Z-05: a world in a
  jar. `jarRecipe` gives the bands, `jarAt` the bands at a time. R-439: `JarLayer`, and `jarLayerOf`, which places a band by what its
  species lives on, each place's bands under the place before)
- `sequences` (Z-06: a species' genome and place in the tree of its genesis: `Lineage`, `lineageOf`, `shipLineage`, `commonAncestorGyr`,
  `treeBefore` (tree order), the marker's letters (`markerLetter`, `markerOf`, `markerAlike`), `genomeLetters`, `watermarkLetters`,
  `alphabetOf`)
- `genes` (Y-02: a species' genes. `GeneCode`: a genesis's map and code. `microbeGenesOf`: a species' genes, read from what it is (wild
  letters chosen to give its own values). `applyEdits`: a line's values after its edits. `strainGenes`: a strain's genes after its move.
  `genesDiffer`, `geneValueWords`. `fleesLight`: dark-dwellers fleeing light, as R-438 drew it. Y-03: an edit moved in or copied
  (`GeneEdit::how`), a moved gene taking its donor's value (`donorValue`), `geneBurden` (a line's price), `geneNeeds` (what a gene
  needs), `kindPace`. Y-04: a gene's switch (`MicrobeGene::sw`, and `geneOwnSwitch`, the default for each kind of gene), `genePrice`
  (one gene's part of the price), `switchedLine` (a line in a given state))
- `morph_draw.h` (the slide's public interface: `Slide`, `Specimen`, the acts, `slidePalette`, `drawSlide`, `magnificationFor`,
  `seeingsFor`, `specimenExtentUm`, `specimenBox`, `specimenPlace`, `seeingBanks`)
- `morph_shape.h` (internal: what a specimen is at a time: bodies, dots, strands and waves in slide micrometres, and the slide's banks),
  `morph_shape_internal.h` (the builder, the pose, a cell's frame for its insides, `CellCtx`), `morph_shape.cpp` (distances, ornaments,
  the pose by motion), `morph_plans.cpp` (each plan's cell and its insides), `morph_acts.cpp` (life-cycle forms, the acts, `buildShape`,
  extents), `morph_draw.cpp` (palettes, the light ways of seeing per pixel, the section), `morph_mesh.cpp` (the electron view's mesh)

## Rules and traps
- Generation 17 is the biota. It draws on separate salts, after every draw of 16, from the body, its system and `BodyGen::make`'s
  traits, never inside a galaxy stream. A new decision takes a new salt (`SALT_*` in the .cpp files). Each species draws from a stream
  of its own (`SALT_SPECIES` and the species index), so a field added at a stream's end moves no other species.
- The enums are saved keys since Z-04 (the guide's species): append before `*_COUNT`, never insert. A world's species keep their
  indices: the world's own first, then fossils, then spores carried to it (`fromBody`, `fromIndex`).
- No life in a giant's clouds (X-03), none fed by light without a sun or under ice, none on a furnace. Nothing is named here: names are
  the explorer's (Z-04). Home's genesis is pinned by two salts (`HOME_GENESIS_ID`, the unit's `life` group).
- `BIOTA` is built at compile time (`biotaOk`: a row per kind in order, a furnace sterile, a stage or spores in every row with
  habitats). The unit checks it against `KINDS`: every landable kind has habitats, every other kind none.
- `lifeOf` is keyed by the star and every body's seed, kind and parent (a staged body is another system). Its pointer keeps a system's
  biotas alive while held.
- The harness includes every module's headers, so a name another module has clashes (the bestiary's `Species` and `PLAN_NAMES`, the
  peoples' `STAGE_NAMES`): hence `Microbe`, `MicrobePlan`, `LIFE_STAGE_NAMES`. Species are drawn only in a world's livable habitats (a
  venusian world's ground burns under its living clouds).
- A form's groups each draw from their own stream (`SALT_MORPH` and the group), so a value added at a group's end moves no other group.
  A world's forms are drawn again (a nudge) while one is too like an earlier one. A carried spore keeps the form it has on its source
  world.
- `unitFromHash(h >> n)` alone spans [0, 2^-n): the drawing's `h01` re-mixes first (a shifted hash made every grain pyrite).
- A slide is a pure function of the view, the specimens and the clock (no state between frames, rows pure). The unit checks that a frame
  comes out the same when reached by skipping and on one thread. Thin things are sized in screen pixels (strands `g_mushKernel + 1`
  wide, dots at least that radius) and fade as they widen.
- A cell's insides go in a frame of the cell's own (`frameOf(B, body, ...)`: centre and axis in the builder's coordinates). Each cell of
  a colony has one. `using namespace morph` clashes with the galaxy's `Body`: write `morph::Body`.
- The unit's bounds check holds every pixel within `specimenExtentUm`. A builder that reaches further widens it, or stops its tips there
  (the mycelia).
- The electron view skips gel (`Look::gel`: a colony's jelly, a slime trail). A colony's cells carry their height on its sphere (`lift`).
- A sample is a pure function of its microhabitat and the biota (`SALT_SAMPLE` by place and species index): only the place and the
  microhabitat are saved (the vial's line). A slide's mount (`mountOf`) is pure too: where the cells are and what they do is closed-form
  from the slide's events and the clock (`mountSpecimens`). The Z-04 fields of `Specimen` and `Slide` (`still`, `wander`, `vx`/`vy`,
  `blurUm`, `lod`, `step`, R-438's `clock`) default to how the plates draw, and `life_look` holds them. On a slide whose stage is never
  set, every kind keeps the slide's clock (frames as before R-438). A kind's answer (`slideAnswer`) must be what its copies do (a
  magnetotactic kind answers the field alone, as the copies' branch in `copyAt` does).
- A `thread_local` named inside a pool lambda is each worker's own: bind it to a reference first (`drawElectron`'s scratch). Red snow
  (`TR_SNOW_ALGAE`) adds snow to a world's habitats and makes its snow species a red alga of the melt (fixes after the draws). The
  alga's cysts are carried like any spore former's.
- A culture is closed-form from its events (`runCulture`: segments, one law each; `cultureAt`: a binary search and a formula). Nothing
  is stepped frame by frame, so the state is the same whether skipped, stepped or laid out again from the saved events (the unit's `lab`
  group). An event the clock reaches (a strain) is appended at its own time, never the frame's. The harness also has the peoples'
  `Culture`, the probe's `CS_` and the pad's `PR_`: hence `FlaskCulture`, `SG_` (stages) and `PA_` (pairs).
- A lineage is closed-form from the biota and its own salts (`SALT_*` in sequences.cpp; nothing of 17 is drawn again). It is made of a
  genesis's deep branches, a kin cell's boxes of sixteen and four sectors, a star's line (for a drifter, the star it left), the last
  exchange between the star's worlds, and a world's comb (the order of its species and the heights between neighbours, with the ship's
  three on the home world). Keep every height under the one above it (a comb's under the exchange, the exchange under the near box's):
  the unit's ranking check holds this. A carried spore sits on the leaf of its source. A marker's letter walks its branches root first.
  Find a lineage's comb path once a run (`pathOf`), never a letter at a time.
- The made species are the peoples' (peoples/made.h, the only file there that reads life): this module never names a people.
- The genes draw nothing of 17: a species' genes are read from its values and form, on salts of their own (`SALT_GENE_*`). An unedited
  line is its species bit for bit (the unit's `genes` group, over forty worlds: values, culture segments, slide kinds). An edit is
  relative to the parent's letters, so a strain's exact move and the wild value hold. A cut gene stays cut (a moved gene can fill it).
  The harness also has the peoples' `Genome` and `genomeOf` and the giants' `GK_`: hence `MicrobeGenes`, `microbeGenesOf`, `GN_` and
  `SE_`. A new field of `Microbe` or `CultureKind` (Y-02's `flees`, `dead`, `quorum`; Y-03's `burden`) defaults to the wild species'
  value. Y-03: the game reads a moved gene's donor and hands it in with the edit (the donor's kind, form and gene). This module never
  parses a key (the peoples have `GE_`, hence `GX_`). `flees` is -1 (its niche's rule) until a sense is edited: compare `fleesLight`,
  never the field.
- Y-04's switches: a line's kind has all its genes on. The switched genes are listed for the culture (`CultureKind::switches`), which
  switches each off where its condition fails (`switchedKind`). A gene switched off works as if cut out, as `applyEdits` cuts, and costs
  nothing. That is never baked into the kind (a cut size would halve again: not idempotent). A kind with no switches takes `advance`'s
  old path bit for bit (the unit's `wild` law, forty worlds). A switched kind's state is judged on the line as written (starving, say),
  and a law ends when the population passes the quorum share, with N set exactly on the share (an ulp short, it loops). A law that
  reaches its end sets the state exactly (B-444: a hold that ended an ulp short held the culture for good). In a predator cycle a
  switched strain's state follows its numbers: the cycle is built in parts, split where a strain crosses its quorum share (`cycleCross`,
  B-446); a pair with no switched strain takes the old single law. The peoples' `SW_`, `WK_`
  and `ON_` and
  the cultures harness's `CW_` are taken: hence `GW_`, `CX_` and `CST_`.
