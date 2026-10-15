# Cookbook

## Add a planet type

1. `galaxy/system.h`: add the enum value before `PT_COUNT`; `system.cpp`: add a `PLANET_TYPES` row (name, description, radius range, landable, atmosphere, max moons, albedo), a `typeColor` case, and weights in `pickPlanetType` (every zone that may host it, plus star-class and moon modifiers).
2. `galaxy/planetmap.cpp`: `BodyGen::make` case (amplitudes, densities, colours) and a `sampleSurface` case built from the `fb/rg/crater/domeField/volcanoField` helpers; assign materials, albedo, relief, veg, glow.
3. `surface/surface_view.cpp`: `lookFor` case (ground/secondary/tertiary colours, sky colours, fog distance, sky density, clouds), `materialLook` bank choices if the type uses new materials, rock density in `drawObjects`, pressure in `computeEnvironment`, any temperature offset.
4. `game/game.cpp`: `shortType` name table (10 chars max); landing map `matCol` if a new material was added (also in `main_headless.cpp`'s map renderer and `MATERIAL_NAMES`).
5. Run `./vesperis_test maps`, `space`, `surface <id>` and look at the PNGs.

## Add a star class

1. `galaxy/starfield.h/.cpp`: enum value and `STAR_CLASSES` row (code, name, description, colour, radius, variance, luminosity, mass, max planets, rarity, first orbit multiplier, minimum first orbit). Since S-01 a new class is a *variety* with rarity 0: it takes a share of one of the six families in `starVariety` (by region), so the stars it does not claim keep their class and the pinned sites hold; a family row with a weight of its own would re-deal every star.
2. `space/space_view.cpp` `drawSun`: glow multiplier and disc appearance branch (and the rays for compact stars; S-04: a pass of its own for a feature along the orbital plane, `discUp`; S-05: a feature round the star within a few radii goes in the glow's pass, widening `outerA` to reach it; S-06: a star that is not a disc of light at all takes a function of its own at the top of `drawSun`, `drawBlackHole` is the model, with the frame copied before it bends anything); `starNebulaPatches` if the star lights its own sky; `drawGlobe` if its worlds look different (S-05: `drawStrippedTail`; S-06: `drawAccretionStream` on the companion).
3. `galaxy/system.cpp` `pickPlanetType`: class modifiers if the class should shape its worlds; `generate` for the locking, the companion chance (`pMultiple`, one entry per class) and `starActivity` for the star's activity (the aurora, the scene finders and the unit check read it).
4. `game/hud.cpp`: radiation warning distances if hazardous; `galaxy/describe.cpp`: the data sheet's sentence. The star map's filter, the statistics and the analyzer read the table (S-02); nothing in the UI needs a change. A new class is the next code (the plan's table is renumbered as the classes arrive: S10 is the neutron star).
5. `./vesperis_test space` renders every class from the first orbit; `unit` checks the class mix near home.

## Add a building kind or a settlement plan (C-01)

`galaxy/ruins.h` `BuildingKind` / `SettlementPlan` and the name tables in `ruins.cpp`; the layout in `layoutSettlement` (place with `Layout::add`, which keeps buildings apart and draws the decay and the seed) and the pieces in `elemsBuilding` (`Form`s, A-02's `galaxy/forms.h`: C-01's boxes and domes or any of the twelve kinds, from the building's own seed at the three levels of detail, building by building in order (the view groups them so each building is drawn at its own level); a piece standing on the ground with `y0` under 0.01 is sunk a metre by the view, a big one stands on the highest ground under it with its foot down to the lowest; each kind's colliders are `formColliders`'). The surface view (`gatherSettlement`, `drawFormItems`, `collectColliders`, `forTrees`) and the landmark finder need no change; `vesperis_test forms sheet` shows a new kind whole and broken at every level, `forms check` walks into it; `vesperis_test unit` checks every house's room and doorway, `scene civilisation_town` shows a town from its gate.

## Add a material

`galaxy/planetmap.h` enum before `MAT_COUNT`; `planetmap.cpp`: `MATERIAL_NAMES`, `matFamily` (the bank it shares: a material of its own colour needs a family whose rep it recolours, as the glass recolours the water), `materialPalette` (its colour per body); `surface/textures.cpp`: `materialHasMesoTile`, a `buildMesoTile` case and the `mesoVariantName` row; `surface/surface_view.cpp`: the specular list if it glints, `drawObjects` and `collectColliders` if rocks avoid it; `main_headless.cpp`: the `matCol` of the map renderer. The landing map, the globe and the surface take the colour from `matColor`.

## Add a HUD readout

Draw in `renderSpaceHUD` or `renderSurfaceHUD` with `drawTextShadow(canvas, x, y, text, colour, HUD_SHADOW)`. Keep lines under ~52 characters (320 px / 6 px per glyph); use `trunc(name, n)` for names. Text is uppercase only.

## Add a key

Keys are GLFW codes in `core/input.h`; the platform copies raylib's `IsKeyDown/IsKeyPressed` by number. Read `in.wasPressed(KEY_X)` for edges, `in.isDown` for holds. Document it in the help pages (`renderHelp`) and in `README.md`.

## Add a game state

Add to `GameState`, handle update in `Game::frame`'s first switch and rendering in the second; overlays should render the underlying scene first and set `returnState`. Update `mouseCaptureWanted()` if the cursor should be released.

## Add a save field

Append a `key value...` line in `save()` and parse it in `load()`; unknown keys are ignored by older builds, so this is backward compatible. Bump the header only for incompatible layouts.

## Add a palette effect

Palettes are rebuilt every frame (`setupPalette`), so time-varying effects are cheap: modify the ramps or scale `fb.pal` after `setupPalette`.

## Change terrain detail or draw distance

`SurfaceSite::init` sets cache cell sizes and torus sizes (the torus must be larger than twice the draw radius in cells); `SurfaceView::render` sets radii `r0, r1, r2` and the inner skips (`(r0*16)/64 - 1` etc.). Keep LOD1's inner skip just inside LOD0's radius to avoid gaps.

## Add a record's genre, a register, a rule or a word (C-02, H-03, H-04)

The texts are written by the grammar engine from `src/galaxy/grammar/*.vg` (the format: `12-civilisations.md`, "The grammar engine";
the records: "The records (H-04)").

1. Edit the `.vg` file and read the result without a rebuild: `vesperis_test grammar` reads the tree's files, prints the link's errors
   (an unknown rule, fact, entity or field, a guard's unknown key, a rule no root reaches, "a" before a plural list: each with its file and
   line), runs every people's records of the test worlds, the age worlds and the forty nearest home through the checks (every name the
   history's at the record's year), and writes them all to `shots/tests/grammar_texts.txt`; `grammar ages` prints fifty of one people of
   each age to `shots/tests/records_<age>.txt`. Read fifty before judging: the checks catch the grammar, not the sense (a door drunk from,
   twelve elk "turned at bay, its head down", a diary that says "you"). Then `make` (the files are compiled in) and `unit` (it checks the
   tree's files are the text compiled in, each under 16 KB: split a file that grows past it).
2. A genre is an alternative of `record` in `records.vg`: its age and its media (`age=industrial medium=printed|medium=codex`), what the
   history must offer (`has:pair`, `has:writer`, `has:dead`, `has:event`, an invention `inv.telegraph=1`), the collapse's `stage` for its
   witnesses, and a weight (the rule is `steady`: a people's shares hold). Its rule `genre.<name>` (`words=lo..hi`: what it writes, the
   length check) in the age's file; its heading and closing `(formula)` rules, left out of the measures. Bind its persons with `@cast`
   and tell the history with `@news`, `@entries`, `@years`, `@death`, `@life` or `@event`; a register's rules are `<register>.<kind>`.
3. A register is a way to tell an event: a rule per event kind (`annal.war`, `letter.sickness`), the event bound as `$e` (its fields:
   `{$e.place}`, `{#$e.value:about}`, `$e.won`, `$e.wave`, `$e.out`), `.none` when there is nothing to tell and `.quiet` for an empty year;
   `registerOf` in `galaxy/genres.cpp` names it (the salience gives a great event told in another register its other side).
4. Compose within a sentence: a part a people draws a hundred times needs many frames (`[a|b|c]` inside a line, two or three slots a
   sentence), and a tail that only fits one part goes with that part (`{=.many}` and `(.many=1)`, or the part and its reactions on one
   line). Persons are entities: `{$me}`, `{$you}`, `{$x}`, `{$kin}`, `{$child}`, `{$spouse}`, `{$ruler}`, `{$founder}` (the realiser writes
   "my mother", then "she" or "my mother" again where a pronoun would not be clear), their cases `{$kin:obj}`, `{$kin:poss}`, a vocative
   `{$you:voc}` (the given name), the name `{$child:name}`; a line that speaks to the reader is guarded `has:$you` (a diary has none). Never
   write "he", "his" or a verb's ending for a person; a verb whose subject changes is `{walk:$x}`, `{be:.w}` after `{rule>.w}`.
5. A word list is a rule in `world.vg` or `people.vg` with guards on the facts (`desert=1`, `cold=1`, `moons>0`, a trait `erg=1`) and
   its number (`(pl)`, `(mass)`, `(sing)`: no "a" before a plural); `(keep)` makes it the same all through a record. A new fact is the
   model's: add it to `RecordFact` and its name in `galaxy/genres.cpp` (the same order) and set it in `recordWorldFacts` (the world and the
   people) or `yearFacts` (the record's year), never from a site. A word of a later age goes behind a guard (`(age>=medieval) iron`) and
   into `lexicon.vg`, by age or by the invention that brings it (`@lexicon inv.railway train trains`).
6. A number with its noun is `{#n|day}` ("one day", "five days"); a rule of counts is written as it is read ("twenty", "a hundred").
7. H-04's second pass: make it a people's own. A thing a people names its own way is a culture pool (`culture*.vg`) with `people=K`
   (each people keeps K of those whose guards hold): one number for every word in it (no "the" in one entry and not in the next: a
   writer will put "the" before it, or not), and a word of a later age behind its guard. A thing with no word of ours is `{~key}` (their
   coin, their dish: lowercase, kept by the decoder). A quip, an image, a joke a reader would notice twice is in a rule with `share=0.3`
   to `0.5`, so each people has its own part of them. A sentence a people draws often is built of parts with two or three of the people's
   own words in it (its crop, its vessel, its craft, its house, its colour): that is what makes it no other people's; names do not help
   (the measures mask them). Read `grammar`'s genre table: "between peoples" (the share of the genre's sentences on a frame another
   people wrote) and "within one" (the people's own repeats); `VESPERIS_TOPFRAMES=8` prints each people's worst. A heading or a closing
   that is the genre's habit is `(formula)`.
8. Several writers side by side: each copies the tree (`cp src/galaxy/grammar/*.vg D/`) and the binary, works in its own files with its
   own rule prefix, runs `vesperis_test grammar dir D out D/texts.txt` (from the repository's root), and copies its own files back; a new
   file is read from disk at once, and compiled in by its line in `galaxy/shards.cpp` (`SRC_...` and `GRAMMAR_FILES`; the unit fails a
   file the build does not list).

## Add a tongue's sound, a name's shape, or an everyday event (H-04's second pass)

1. A people's tongue is `galaxy/tongue.cpp`: a consonant set is a row of `CSETS` (codes from `CONS`, each sound with its spellings), a vowel
   set a row of `VSETS`, a syllable shape a string of `SHAPES` (C, V, L, S, F, `?` after a part it may leave out). A name's shape by its
   kind is `tongueName`'s switch (a place by its rank: a root, a root and a generic word before or after it, an adjective and a root); the
   generic words are the `TM_` meanings. A name that reads as a word of ours is caught by `unfortunate` (add the word). `vesperis_test
   tongues 60` shows sixty peoples' names side by side and how often a name tells its people; the unit asks 65% over 24.
2. An everyday event is a row of `EVERYDAY` in `galaxy/history.cpp` (its kind in `HistoryEventKind`, before `HE_BIRTH` for a public one,
   after `HE_DEATH` for a family's; its name in `HISTORY_EVENT_NAMES`, `KIND_KEYS` and the event's noun in `galaxy/genres.cpp`; its words
   with the ages and what each needs; its rate by age and its salience): drawn from the year's own stream (`Builder::everyday`) or a
   person's (`Builder::family`), it never harms anyone or makes a person, so the rest of the history stays. Then the registers that tell it
   (`annal.<kind>`, `letter.<kind>`, `news.<kind>`, `log.<kind>`, `song.<kind>`, `sermon.<kind>`) with `$e.what` for its word.

## Move the shards, or add a place they lie (C-03)

1. The counts per settlement class and the buildings' weights are in `recordSitesOf` (`galaxy/records.cpp`; T-04 replaced C-03's
   `shardSitesOf`, see "Add a record's medium, or change where records lie");
   since A-03 the candidate spots are the building's room's (`buildingRoom`, `galaxy/buildings.cpp`: a `case` per kind, in the
   building's own frame, x' across the heading, z' along it; a rectangular room's by its door, `rectSpots`), and the place is
   the building's kind, named by `buildingPlaceName` (the log reads "TAKEN FROM <place>"). A new kind with a room gets a `case`
   there that matches its generator's walls and doorway.
2. Every spot is tested against the colliders of the building's pieces (`ruinElements` at level 0, `formColliders`) and its
   doorway (`BuildingRoom::door`); keep that, or the explorer cannot stand at the shard. Nothing in the view or the game knows where a
   shard lies beyond `RuinCell::shards`.
3. `vesperis_test ruins 151 25 1 12 4.22` lists the sites, `scene civilisation_shard` shows one from its room, `unit` checks
   the counts, the clearance and the view's reach; a changed placement moves no regress hash (no regress scene has a
   settlement in view) and never what a saved guide's `shard` lines point at (they name a people's record by its index).

## Tune the decoder: the learning curve or the people's tongue (C-06)

1. `languageShare` (`galaxy/shards.cpp`) is the curve: 0.3 with one shard, linear to 1 with ten; `languageKnown` turns it
   into a count of words from the most used down (the share is of the fifty's word uses, so the first words known are the
   articles and the pronouns, and the names come free). A gentler or steeper curve changes nothing saved: the guide keeps
   the count a shard was read with, not the share.
2. The tongue's pools are `T_ON`/`T_NU`/`T_CO` by the names' style; `tongueOf` draws a people's subset and its coda and
   bare-vowel chances, `tongueWord` builds one to three syllables by our word's length (never under two letters) and
   `languageOf` re-draws a word that would repeat one already given. A word's form is a hash of the world and the word,
   so changing the pools changes every people's words at once; C-05 (the voice) reads the same `Tongue`.
3. `vesperis_test shards 151 25 1 12` prints the first reading, `... 12 3` one with three held, `... 12 10 full` the whole
   text under each; `unit` checks the shares (25-50% with one, over 55% with five, all with ten) and the tongue; the flow's
   `shards_*` frames show the screens. No regress hash sees the decoder.

## Tune the music: a people's tradition or the pieces (C-04)

1. `traditionOf` (`galaxy/music.cpp`) draws a world's tradition from the body's seed: the period (the octave, now and
   then stretched or narrow, rarely the twelfth), the scale (five to nine notes from an equal division of 5-24 steps or
   from just ratios, 75-450 cents between neighbours), the resting degree (nearest a fifth, else a fourth), the tonic
   (165-330 Hz), the cycle (3-12 beats in groups of two and three, a four now and then), the subdivision, the tempo
   (66-144) and its drift, two or three timbres (`makeTimbre` per family; the names' style leans the pick: hard names to
   struck and plucked things, flowing ones to pipes and bows), the drum, the drone, the second voice's kind and the ornament
   rule. The pools and weights are the tables at the top of each block; `vesperis_test music <sx> <sz> <body>` prints what
   a world got.
2. `pieceOf` is the piece: the form by the shard's tone (the `FP` table: tempo factor, note density, velocities, rest chance,
   register, drum, whether the answer and the return repeat), phrases of one or two cycles (`makePhrase`: a rhythm per group
   from `groupRhythm`, a walk of degrees from `nextDegree`, the closing group one long note), the structure statement /
   variation / answer / return (/ answer / return) and a close of the tonic held a cycle, then the ornaments on the lead,
   the second voice, the drone and the drum pattern. The notation print of the `music` mode is the place to judge a change
   on paper; `music ... wav` renders it through the real synth for the ear, and the `unit` checks the bounds (15-90 s,
   notes in order and in range, three forms at least a world, the two test worlds' traditions distinct).
3. The synth (`AudioSynth::musicSample`, `game/audio.cpp`): ten voices, each a note of a timbre (six partials with the
   velocity opening the upper ones, an attack-decay-sustain-release envelope, vibrato after 0.3 s, breath noise; the drum
   a falling thump and noise), the clock in beats wandering by the tradition's drift. `AudioState::pieceGain` is the
   level; a null `piece` stops everything at once (the pointers must outlive the play, which is why the decoder clears
   them before `shardWorld` can move). Which shards are music is `shardIsMusic` (a shuffle of the fifty from the world's
   seed, twelve to eighteen of them): changing that moves the texts' indices, so the sites of C-03 keep their shards but
   some rooms hold a piece where a text was.

## Tune the voice: a people's speech (C-05)

1. `PHONES` (`galaxy/voice.cpp`) is the table of sounds, one row per spelling the tongue's pools and the names use (longest
   spellings first: `sh` before `s`): a vowel's three formants (a diphthong's two sets), a stop's locus and burst, a
   fricative's noise band, a sonorant's formants, each with a length. A new spelling in `T_ON`/`T_NU`/`T_CO`
   (`galaxy/shards.cpp`) or in `generateName`'s pools needs a row, or its letters fall back to the single-letter rows
   (`parsePhones` skips what has none).
2. `voiceOf` draws a world's voice from the body's seed, leaning with the names' style (hard names lower, rougher,
   clicking and rolling more; flowing names quicker, breathier, trembling more): the pitch (85-286 Hz, the tract longer
   for the deeper voices), the range, the rate (3.0-5.6 syllables a second), the qualities (breath, with 7% a whisper;
   roughness; a nasal murmur; a tremor; a second tone an octave or a fifth below), the stress rule, the sentence melody,
   tones on the syllables, clicks for the voiceless stops, rolled r's. `voiceLine` is the sentence the screen shows
   (52 characters at most: the unit checks it). `vesperis_test voice <sx> <sz> <body>` prints what a world got and
   `shards` one line of it.
3. `speechOf` times a decoded shard: a syllable's sounds sum to 0.28 s at the base rate, the stressed syllable's vowel
   1.3 times as long and the whole 1.2 times as loud, the last syllable before a pause 1.35 times as long, a pause by the
   punctuation (a comma, a semicolon or colon, a full stop: 0.18, 0.28, 0.45 s at the base rate); the pitch is the
   sentence's melody (`contourAt`) with the stressed syllable raised, or a tone a syllable in a tonal voice, and a fall at
   a full stop, a rise before a comma or at a question mark. Every word's span (`wordStart`/`wordEnd`) is what the
   screen aligns the text to; the numerals are spoken digit by digit in the people's words for the digits
   (`decodeShard`). `voice ... wav` renders a shard through the real synth; what paper can check is the WAV's envelope
   peaks a second (near the rate) and the pitch of its loudest windows (near the base), a few lines of python with the
   `wave` module.
4. The synth (`AudioSynth::speechSample`, `game/audio.cpp`): a glottal pulse (the derivative of a Rosenberg flow)
   with jitter and shimmer by the roughness, a tremor, the second tone; breath as dulled noise added to the source; three
   formants moving at a 20 ms time constant toward the segment's targets and a fourth fixed, in cascade (`BW`: 80, 100,
   150, 250 Hz); a nasal resonance beside; the fricatives' and bursts' noise through a band of its own (unity at the
   centre). The level is the `tanh(y * 0.5) * 0.65` line (a text peaks near 0.5, rms 0.13-0.15; the music's dance peaks at
   0.8). `AudioState::speechGain` is the level; a null `speech` stops it at once.

## Tune the drone (R-403)

1. The numbers are the constants at the top of `surface/drone.cpp` (`TOP` is `SurfaceView::DRONE_TOP_SPEED`, the ceiling
   `DRONE_CEILING`, both in `surface_view.h` because the HUD and the game read them; `CLIMB`/`DESCEND` the vertical speeds;
   the thrust, the drag and the turn's lock in `updateDrone`). The landing rule is the `d.y <= surfaceN` block: the speed
   under 3 m/s with Space released lands it.
2. The hull is the buggy's stations shortened (`NOSE_F`..`TAIL_F`, the widths and heights); the pods `POD_F/POD_S/POD_U/POD_R`,
   the skids `SKID_S`. `drawDrone` draws it; from the seat nothing is drawn (R-204's rule).
3. Judge it with `vesperis_test flow` (`flow_drone_seat/flight/chase.png` and the `drone:` line), the unit's drone check (the
   top speed at 25 s, the height, the landing time) and `bench check`'s `drone 2x` row; the sound is `AudioState::rotor`
   and `rotorPitch` in `audio.cpp` (a whine at 70-160 Hz with two harmonics over low-passed noise).

## Read the world's plan from a feature, or add a network kind (T-01)

1. A feature that stands where a people lived reads the plan (`worldPlanOf(g)` once a world, cached; in a frame `worldPlanIfReady`,
   which never waits): `planPlacesNear` for the places in reach (their rank, class, core and population), `planCountryside` for the
   density of the countryside at a point, `planCoreAt` for a core under a point, `planEdgesNear` and `planEdgeWay` for the ways and
   their waypoints. Keep the feature a function of the plan and the world (no site), as the cells' reading is.
2. A new network kind: its entry in `NetKind` with its names (`NET_KIND_NAMES`, `NET_KIND_HUD`), where the plan lays it (`planPeople`'s
   kinds by the age, after the spanner), its walk's weights (`walkCostOf` in `roads.cpp`), its half width (`netHalfWidth`), its bed
   (`SurfaceSite::sampleAt`'s switch), its line on the zoom (`KL` in `renderLandingMap`) and on the plate (`KD` in `plateFill`); a kind
   on pylons or other works puts them in the cells it crosses (`planSpansIn`, `ruinOfCell`'s works).
3. `vesperis_test plan <sx> <sz> <body>` (the plan of every age), `plan view` (the telescope), `zoom` over a place, the unit's T-01
   checks (connected, dry, the ways on the ground) and `bench check` (`descent people`, `capital 2x`).

## Change a town's plan: a street pattern, a district, a work of the water (T-02)

1. The streets are `townStreets` (`galaxy/towns.cpp`): a pattern is a function of `Net` over a ring (`grid`, `spokes`, `rings`, `organic`,
   `lineTown`, `branching`, `suburbs`), called by `Net::pattern` for each layer's age; a street is laid by `line` (a medieval one crooked by
   its wobble) or `arc`, clipped to its ring by `seg`. The end of `townStreets` clips them to the shore and keeps the one web: a pattern
   needs only lay streets that meet. Widths are `streetHalf` (by rank and age, times the class's `widthK`), the blocks `streetSpacing`.
2. The buildings are `layoutTown` (`galaxy/ruins.cpp`): `districtAt` says a lot's district, `lotSpec` its building (a dwelling through the
   layer's `Layout::dwellingSpec`, a work, a tower, a compound, a garden), `setback` and `gapAfter` its place on the street, `density` how
   often a lot is built. A new building placed off a street: `Layout::add` with its fit tested (a rectangle against the others, off the
   streets), then `intrusion` (its own pieces off the streets) and `undoTo` if they reach one; mark its `district`.
3. A work of the water or the land of an age: `addFeatures` (the harbour by `c.age`, the reservoir, the terraces): its buildings
   `L.add` with their fit tested or `segmentClear`/`offStreets` first; a new kind gets its generator in `galaxy/buildings.cpp` (see A-03's
   recipe) and its feature flag in `RuinSpec` and `featureList`.
4. `vesperis_test towns <sx> <sz> <body>` (the plans drawn from above), `towns view` (through the renderer), `towns check` (no building
   across a street, the streets walkable, the old cores, the districts, the harbours, the ground's marks), `ruins check`, `genome reach`
   (a variable that acted in the old plans must still reach the drawing).

## Add a record's medium, or change where records lie (T-04)

1. A medium is a `RecordMedium` (`galaxy/records.h`, before `RM_COUNT`) with its harness name (`RECORD_MEDIUM_NAMES`) and what the HUD and
   the log say of it (`RECORD_MEDIUM_HUD`: a description, never a name); the ages and categories that made it are `mediaOf`'s table in
   `galaxy/records.cpp` (a weight by the record's age and category, `f` how far into its era the year falls), which `recordMediumFits`
   (the unit) reads too. Its object is `recordForms`'s case: a few forms in the object's frame (`Obj::add`, `lying`), small, on the floor,
   in the records' parts (`FORM_PART_PALE` .. `FORM_PART_METAL`: banks 29, 30, 31, 17 and 7) or its people's slots (a stone's 0, a
   sleeve's accent 1, a light's 3); no collider (it is picked up).
2. Where records lie is `recordSitesOf`: a building kind's weight for one (`recordWeight`), where an age kept its archive (`archiveKinds`)
   and its last words (`lastKinds`), how many by the class (the `want` and the archive's `k`), the works that kept records (the `SC_WORKS`
   switch: a work's pieces need a `buildingRoom`, at least its foot). A record's home (`recordHomesOf`) is the place that always holds it:
   change its weights there, and keep every record on some place of the plan (the unit counts the copies).
3. The count is `recordCountOf` (`BASE` by the age, `REF` the age's middling numbers); it must stay within 80 to 200 and rise with the age.
   C-02's first fifty keep their categories (`shardIsMusic`, `chartIndexOf`); past them `recordIsMusic` and `recordChartIndex`.
4. `vesperis_test records <sx> <sz> <body>` (the catalogue and its sites), `records ages`, `records scenes [age]` (each object drawn and taken
   in the game), `records check` (the unit's five).

## Add a work, or change where one goes (T-03)

1. A kind is a `WorkKind` (`galaxy/works.h`, after the last: the guide keeps none, only `LM_WORK` marks) with its name in `WORK_KIND_NAMES`
   and what the scanner hears in `WORK_HEARD` (a description, never a name), and a glyph in the harness's `WORK_GLYPHS`.
2. Its rule is a member of `WB` (`galaxy/works.cpp`) called in `buildWorldWorks`'s order (the history's works first): probe the ground with
   `at` (64 m, the rivers off) and `grade`, find a place (`summitNear`, `shoreNear`, rings round a place, the plan grid's cells), keep it
   `clearOf` the settlements, the cores and the works before it, then `add` it with its age, its place and its event (`placeAge`, `eraAge`
   for the history's years; `cleared` for a place its people cleared). What it does to the ground is a `GroundEdit` in its own metres (a cut,
   a pit, a ditch, a bowl, a flat, water, lines, terraces); a new kind of edit is drawn in `SurfaceSite::applyEdits` on the rings that can
   show it and listed in `editCover` if nothing should grow on it.
3. Its pieces are `workSpec`'s case: `Lay::add` buildings in the work's metres (a new kind's generator in `galaxy/buildings.cpp`, A-03's
   recipe; heights over the reference level with `Building::base` and `Form::absolute`), `Lay::plot` for a plot of graves.
4. `vesperis_test works <sx> <sz> <body>` (its line), `works view ... <word>` (through the renderer), `works gallery`, `works map` (with its
   place), `works check` (and a check of its rule beside the dam's, the quarry's and the battlefield's).

## Read a people's history, or add an event to it (H-02)

1. A feature that reads what happened (a town's old core, a battlefield, a record) takes the history once (`historyOf(sys, b, g,
   people)`, cached; it builds the world's plan first if it must, so ask it off the frame, as the plan is) and reads it by year
   (`eventsIn`, `reignAt`, `harvestAt`, `populationAt`, `inventionYear`, `historySeat`), by person or place (`eventsOf`, `eventsAt`,
   `historyKinship`) or by its lists (`places`: the plan's by rank, then a later people's cleared cities; `wars`, `sicknesses`,
   `institutions`, `families`). A place's `age` is the age of its old core, its `ended` and `fate` how it was left.
2. A new kind of event: its entry in `HistoryEventKind` with its name (`HISTORY_EVENT_NAMES`) and the slots it fills (the comment by
   it); where the year loop makes it (`randomEvents` for a rate, `setupPlanned` for a year fixed in advance, `collapseYear` for the
   collapse's) with its cause and its places standing; its sentence in `historyEventLine`. A person it names must be alive that year
   and a place it names standing (the unit checks both); a harm that kills goes through `harm` so the families' deaths name it.
3. `vesperis_test history <sx> <sz> <body>` (the legends), `history ages`, `history check` (the unit's five checks in a few seconds).

## Add a variable to the architecture genome (A-04)

1. Its entry in `GenomeVar` (`galaxy/genome.h`; the `static_assert` counts thirty-five: a thirty-sixth moves it), its values' enum,
   and its row in `GENOME_VARS` (`genome.cpp`): its printed name, its group, its categories (0 a measure with its two ends), the ages
   it acts in (a bit an age: the unit's reach check holds it to them) and its weight among the strongest (0 not one of them; a weight
   moves `genomeMaxDistance` and the alike threshold's meaning). Its draw is its own hash (`genomeOf`): nothing else moves.
2. Where it acts: a way of the culture (`cultureWays`, `galaxy/buildings.cpp`), a generator reading it (`Gen::gv`, `gf`), the palette
   (`culturePalette`), a plan (`Layout` in `ruins.cpp`: `formOf`, `foundOf`, `dwelling`, `dwellingSpot`, `centre`, `focus`, `stela`,
   the plan's `case`). What a room depends on goes into the `Building` (as `form`, `sides`, `found`, `centred`), so `buildingRoom`
   stays a function of the building alone.
3. `vesperis_test genome reach` (every age it claims), `genome sheet` and `genome ages` to see it act at a scale the picture shows,
   `ruins check` (nothing on air, every doorway open), `genome distinct` if it weighs among the strongest.

## Add a kind of building to an age (A-03)

1. A `BuildingKind` (`galaxy/ruins.h`, with its `BUILDING_KIND_NAMES` entry: a `static_assert` counts them), its age in
   `buildingAge` and its place's name in `buildingPlaceName` (`galaxy/buildings.cpp`).
2. Its generator in `buildings.cpp`: a `Gen` gives the building's frame (`add`, `box`, `lathe` in its metres, the heading turned
   by `dh`), its draws (`U`), its materials' decay (`D`, `Dk` for a piece), and the helpers: `walls4` (a room with its doorway and
   windows), `roof` (never on walls under 92% of their height), `post` (a post to its stump and its hole), `heap`, `fallen` (a tall
   thing's top across the ground). Tag every piece with its material and its palette slot; copy what a piece's reference holds
   before adding the next (`out` grows and moves). Route the kind in `buildingForms`, and its room in `buildingRoom`.
3. Where it stands: a place of the plans asks the age (`Layout::dwelling`, `centre`, `tower`, `hall`, `stela` in `ruins.cpp`,
   the monuments in `pickMonument` with their reach in `monumentReach`).
4. `vesperis_test ruins kinds` (add it to the sheet's table) and `ruins ages` to see it; `ruins check` checks that nothing of it
   rests on air, its room's doorway is open, its spots are clear, and that its age builds nothing later than itself.

## Time a change, or prove that it draws nothing new (F-01 to F-11)

1. Keep the previous binary (`cp vesperis_test build/perf-baselines/vesperis_test_<id>`; never `make clean`, it deletes `build/`).
   Before timing, the Mac's state: `pmset -g batt` (mains or battery: compare only runs of one batch), `sysctl vm.swapusage` and
   `top -l 2 -s 2` (a build, Spotlight or Docker beside the runs shows in them); wrap a batch in `caffeinate -i`.
2. The pixels: `regress` and `VESPERIS_THREADS=1 regress`; `scene <name> 2` and `4` by both binaries, then `pngdiff` (zero differing
   for an identical change); `screens` for the cabin, the menus, the maps and the feeds; `bothways <scene>`. The `flow` only from one
   restored state, beside a baseline-against-baseline run (KI-404).
3. The time: `bench check` by the two binaries in turn, the order swapped every pair, four pairs; a row is faster when it is faster in
   every pair. Several variants: a rotation, each in turn, three rounds. One thread: `env VESPERIS_THREADS=1` (a zsh background job
   with that variable alone once ran the sun's loop five times slower: KI-409). A row of 30 frames after a row that got faster can
   read slower: time that view alone over hundreds of frames before calling it a loss.
4. Where the time goes: `VESPERIS_TRACE=1` (the sections and the bands), `sample vesperis_test 5` on `bench forest` or on a mode run
   under a pseudo-terminal (`python3 -c 'import pty,sys; pty.spawn(sys.argv[1:])' ./vesperis_test bench`), and the scaling curve over
   `VESPERIS_THREADS=1, 2, 4, 6, 8` (`08-testing-and-tools.md`). A pure function is judged in the frame, on the renderer's own inputs:
   F-10's `atan2`/`asin` pair won a loop of random directions and lost the globe's scanlines.
5. After: a row that fell gets its budget lowered to about 1.5 times the new median (`runBench`), the numbers go into
   `08-testing-and-tools.md`'s reference paragraphs, the decision and what was dropped into `10-decisions.md`.
