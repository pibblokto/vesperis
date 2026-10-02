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

`galaxy/ruins.h` `BuildingKind` / `SettlementPlan` and the name tables in `ruins.cpp`; the layout in `layoutSettlement` (place with `Layout::add`, which keeps buildings apart and draws the decay and the seed) and the pieces in `elemsBuilding` (boxes and domes from the building's own seed at the three levels of detail; a piece standing on the ground with `y0` under 0.01 is sunk a metre by the view and collides when it stands 0.9 m or more). The surface view (`drawSettlement`, `collectColliders`, `forTrees`) and the landmark finder need no change; `vesperis_test unit` checks every house's room and doorway, `scene civilisation_town` shows a town from its gate.

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

## Add a shard kind or a slot (C-02)

1. A kind is a row of `kinds()` in `galaxy/shards.cpp`: a name, a tone (`ST_ORDINARY`, `ST_ELEGY`, `ST_WARNING`, `ST_END`),
   the world it belongs to (0 both, 1 felisian, 2 desert), a weight, the last age it is told at on a desert (0.5 for what
   speaks of living water, 1 otherwise) and its templates. Write two to four templates of two to four sentences each;
   draw on the lore (`{god}`, `{river}`, `{founder}`...) so the names recur; use `{skyone}` where a singular verb follows,
   `{beast}` for one animal and `{herd}` where the same animal returns in the text; `{he}`, `{him}`, `{his}` follow the
   shard's `{kin}`.
2. A slot is a `G.set(...)` in `buildGrammar`: read it from the body, the system or the `BodyGen` (a trait, a class, a
   number), never from a site (the shards are a property of the world). An entry may hold marks of its own.
3. `vesperis_test shards 151 25 1 50 full` and `shards 153 70 2 50 full` to read the result; `unit` checks the marks, the
   lengths, the distinctness, the names and the tones. The texts are translations: plain English, no digits over the year.

## Move the shards, or add a place they lie (C-03)

1. The counts per settlement class and the buildings' weights are the first lines of `shardSitesOf` (`galaxy/shards.cpp`);
   the candidate spots per building kind are its `switch`, in the building's own frame (x' across the heading, z' along
   it; a house's in the door's frame, v toward the door). A new place is a `ShardPlace` and a `SHARD_PLACE_NAMES` entry
   (the log reads "TAKEN FROM <place>").
2. Every spot is tested against the building's pieces (`ruinElements` at level 0, the ground ones) and its doorway
   (`buildingDoor`); keep that, or the explorer cannot stand at the shard. Nothing in the view or the game knows where a
   shard lies beyond `RuinCell::shards`.
3. `vesperis_test ruins 151 25 1 12 4.22` lists the sites, `scene civilisation_shard` shows one from its room, `unit` checks
   the counts, the clearance and the view's reach; a changed placement moves no regress hash (no regress scene has a
   settlement in view) but changes what a saved guide's `shard` lines point at only if the indices move, which the
   settlement's own draw order decides.

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
