# Cookbook

<!-- contents -->
Contents:
- Add a kind of world (P-00)
  - A kind made from the old draws (P-01)
- Add a trait (Q-01)
- Add a quality (Q-02)
- Add a sound of the ground (Q-03)
- Reach a place that is not a star, or read the sun where there may be none (D-01)
- Let the radar hear a new kind of source, or draw where no sun shines (D-02)
- Give a signal a voice of its own without moving another's (D-03)
- Light the ground, or give a world in the dark something of its own (D-04)
- Give a people an end of its own without moving another's (D-05)
- Add a star class
- Add a building kind or a settlement plan (C-01)
- Add a material
- Add a HUD readout
- Add a key
- Add a game state
- Add a save field
- Add a vehicle
- Add a palette effect
- Change terrain detail or draw distance
- Add a record's genre, a register, a rule or a word (C-02, H-03, H-04)
- Add a tongue's sound, a name's shape, or an everyday event (H-04's second pass)
- Move the shards, or add a place they lie (C-03)
- Tune the decoder: the learning curve or the people's tongue (C-06)
- Tune the music: a people's tradition or the pieces (C-04)
- Tune the voice: a people's speech (C-05)
- Tune the drone (R-403)
- Read the world's plan from a feature, or add a network kind (T-01)
- Change a town's plan: a street pattern, a district, a work of the water (T-02)
- Add a record's medium, or change where records lie (T-04)
- Add a work, or change where one goes (T-03)
- Read a people's history, or add an event to it (H-02)
- Add a variable to the architecture genome (A-04)
- Add a kind of building to an age (A-03)
- Time a change, or prove that it draws nothing new (F-01 to F-11)
<!-- /contents -->

## Add a kind of world (P-00)

A kind is a row of `KINDS` (`galaxy/kinds.cpp`, `buildKinds`) and, where no ground function fits, a ground function. The code
paths read a kind's row or its tags; a kind is named in code only where it is the rule itself (the home world is felisian, a
comet and a companion star are made apart from the pick, a giant's volcanic moon feeds its torus). A pixel loop reads the
tags it needs before the loop (`drawGlobe`'s `gasGiant`, `glows`, `deck`, `matBank`): read at every pixel, the table's loads
cost the space view 3-5%.

1. `galaxy/kinds.h`: the id before `PT_COUNT` (never between: the ids are saved keys, the guide's walked kinds and the
   harness's arguments; `Rng::pick` falls through to the last id on rounding, so a kind's place in the pick matters only
   at its end). The checks at the end of `kinds.cpp` refuse a row left out, out of order or incomplete.
2. The row in `buildKinds` (the existing rows are the examples):
   * identity: `identity(...)` (the name, unique and safe as a file name; the short name, ten characters; the data
     sheet's line; the radius range; landable; air; the moons; the albedo), the density, the ring chance, the sentence
     (`sentence(k, SR_..., ...)`: one phrase, a coin, or a phrase chosen by a parameter of the body);
   * the tags (`KT_...`: what the code paths read, the groups a kind shares: air with fronts, a sea, flora, a people's
     world, the giants, the lava worlds...) and the shapes (`KS_...`: the processes that act on its ground; a trait is
     eligible where a process it needs acts, `TRAITS`);
   * generation: the zone weights (`zones(k, >600 K, 345-600, 205-345, 120-205, <120)`) and its answer to every
     star-class rule (`rule(k, WR_..., mul, add)`, `moons(k, ...)`; the Wolf-Rayet wind strips its air by itself), its
     colour from afar (`colour(k, ch(base, var), ...)` or `palette4`);
   * the body: `make`'s draws in order (`draw(k, GF_..., lo, span)`; a draw the kind does not make keeps the
     generator's default), the second colour and the sky tint (`tintOf`), the relief's grades (`k.relief`), the palette's
     mixes (`mix(k, MAT_..., MM_..., tint, t)`), the family representatives (`famRep`), the exotic sea, the accents (a
     kind without flora may lend its grass and forest banks to two materials of its own: `accent`);
   * the ground: `ground` (an existing `GR_...` function shares its code; its parameters are the kind's draws), the
     plains (`plainK`, `gullyK`), the drainage's area factor, the crater stack (`crater(k, ...)`: the grids
     `sampleSurface` sums, the ones marked `sight` the landmarks' crater grids);
   * the look and the air on the ground: the sky (`SKY_TINT`, `SKY_THIN` or `fixedSky`), the fog, the sky's density,
     the cloud layer, the night (`night`), the far floor's family, the boulders (`rocks`), the snow, the aurora's style,
     the pressure (lo + span x a hash), the temperature (offset and swing with air, the ground's own heat), the wind,
     the rain (`RAIN_CLOUDS` with its threshold and span, `RAIN_DRIZZLE`), the dust storms' threshold;
   * the events (`EV_...`: eruptions, fountains, strikes, quakes, devils, vents, jets; `eventMaterial` the ground an
     event stands on) and the rumble.
3. A ground of its own: a `void groundX(GroundCtx& c)` in `galaxy/planet_grounds2.cpp` after the others, declared with
   them in `planet_internal.h`: its base, then `c.relief(h, rel, hf, reliefAmp(g, rel, hf))` (the relief spectrum, the cliff
   bands and the plains, the traits' landforms), its features (`c.fb`, `c.rg`, `c.crackAt`, `c.craterStack`, the feature
   fields), its material, albedo and relief; a `GR_...` id and name (`GROUND_NAMES`) and its case in `sampleSurface`'s
   dispatch (`planetmap.cpp`). A material that matters on the ground at a scale the far rings do not draw follows B-404's
   rules (`matVisible`, `featFade`).
4. A new material: `Material` (before `MAT_COUNT`, 32 at most), `MATERIAL_NAMES`, the meso tile and its variants
   (`surface/textures.cpp`), the harness's `matCol`; its family (`matFamily`), or an accent of the kinds that use it.
5. Look: `vesperis_test kinds <NAME>` (the globe, the world map in its own colours, the row's facts, standing, from 350 m
   up and its event staged: `shots/tests/kind_<NAME>.png`), `survey` (its share and its materials), `maps`, `space`,
   `surface <id>`, `landmaps`, `consistency`, `traits`; `unit` checks the rows. A kind that changes what is generated
   bumps `GEN_VERSION`.

### A kind made from the old draws (P-01)

1.4.0's kinds take no place in the pick (a weight anywhere would re-deal every system: the totals, `typeColor`'s extra draw,
the twin's chance, the moons' counts). The row has no zone weight and no rule (the checks refuse one), a `family` (the
milestone's: `kinds <family>` writes its sheets) and a `salt` of its own, and one to three sources: `source(k, of(PT_...),
tLo, tHi, KB_..., stars, lock, share)`, the drawn kinds it replaces (`of` takes up to three; two sources for two shares), the
band of equilibrium temperature (the game's scale: the inner orbits reach 350-500 K), the bodies (`KB_PLANET` of the
primary, `KB_COMPANION_WORLD`, `KB_TWIN`, `KB_MOON`), the star classes (`of(STAR_...)`, 0 every class; `LIVE_STARS` the stars
a giant could have lived beside), 1 locked to its star or -1 turning, and the share of the qualifying worlds. `kindOf` tries
the made kinds in id order, so a kind later in the table takes what the earlier ones left of a drawn kind. A world with a
people never changes. The draws stay the drawn kind's: the size comes from the made kind's range on the same draw, the
colour from its row by hashes, the rings by its own chance where it differs, `untilted` takes a locked world's tilt. Then:
`survey` says how many changed and from what; `build/p01/stream.cpp` (built against the last release's sources and the
tree's) proves every draw held; the pinned test worlds (`docs/decisions/log/6-1.4.0.md`, P-01) are checked.

**A kind on the felisian layer (P-02).** A living kind is a row whose `ground` is `GR_FELISIAN`: the climate, the biomes, the seas, the
drainage, the flora, the animals and the sounds come with it, and the row says how its world differs (`greenhouseK`, `latGradient`,
`seaDepth`, `grassBand`, `daySwing`, its draws: the sea level, the ice cap, the snow line, the moisture, `GF_ISLANDS`, `GF_CONT_SCALE`;
`floraFamily`, `favourTrait`, `fauna`). A way of its own that the layer has not got is a tag (`KT_MICROBIAL`, `KT_ICE_SHEETS`) read where
it acts, never a test of the kind; every new term reads its field only where the field is not the felisian's, so the felisian world's
bits stand (`build/p02/fp2.cpp` proves every unmade body the same, field by field and sample by sample). A source `byStar` hashes the
system's star (every world of a young star alike). Pin a salt where a test world or the home world would move (`salt scan`).
A trait the kind should never draw takes a process of its own (P-02 split the snowball's `KS_FREEZE` off the open sea: every kind that
had the sea got both, so no list moved).

**A kind of the locked, or of a moon's planet (P-03).** A source with lock 1 takes a world that keeps a face to its star; give the row
`untilted` (the tides took the tilt, so the sun stands still over one point) and, for a climate round the substellar point, `KT_FIXED_SUN`
(the wind steady toward it; on the felisian layer `subContrast`, `ringWet`, `dayDry`, `daySeasDry`, a sheet `sheetByCold`; on the ground
`reachClimate`). A planet made such a kind that then draws a twin is made again as one whose day is not its year (`generate`): nothing to
do but know it. A moon's source may name its planet's drawn kinds (`source(..., byStar, of(PT_SUBSTELLAR))`: the brown dwarfs' moons).
A made world that grows pushes its moons out; the companion's clearance and the comets read the drawn orbits, so no star moves. Check
the regress's and the bench's worlds after a new source (`salt scan` finds a salt that leaves them as drawn).

**A dead kind (P-04).** A world that was a living kind and died: `thenGround(k, K[PT_FELISIAN])` gives the row the living kind's draws,
relief, greenhouse and tints (the same ground in both times) and its `thenKind`; a `cover` says what the present laid over it (a branch at
the end of `felisianSample`: change materials and the water, never a dry place's height, or a town floats); `peopleShare` and `peopleSalt`
its own peoples (C-01's hash never fires on a made world); `elsewhere` for a kind made where no system is (no source: the harness stages
it, `stageElsewhere`). Every reader of a people's world goes through `thenWorld` already (B-419's `unseasoned` for the living), and what
the present did to its ruins reads `nowKind` (`cultureOf`'s weathering, `readSite`'s drifts and frozen shores); `unit` checks every dry
place of its test worlds' plans stands at the then-world's height.

**A body of its own step (P-05's wanderer).** A body no draw of the system makes (a rogue world a star caught) is appended after the system's
own in `StarSystem::generate`, from a stream of its own (`Rng` seeded by a hash of the star's seed with a salt: no draw of the system's, so
every other body keeps its index and its draws) and by a share of systems on that hash; its kind is `elsewhere` (no source in `kindOf`). Keep
it clear of what the system's rules promise (B-425's far companion: the new body is the primary's farthest world), flag it (`Body::wanderer`)
where a reader must know (no people: `BodyGen::make`), and run the stream against the last milestone's sources (`build/p05/stream.cpp`,
`analyze.py` keyed by system and index): every old body's draws the same, the new bodies extra. A moon class no kind took before (the
captured moons, `KB_CAPTURED`) goes through `makeKind` with a body bit of its own, so the earlier kinds' sources (which name `KB_MOON`) take
none of them.

**A kind made from what a moon is (P-06's strange moons).** A source names a moon's facts among its planet's moons (`KindSource::moon`, the
`MoonFact` bits: inner, outer, facing, turning, resonant, large, small as drawn) with `KB_MOON` and, where it is a giant's, `parents`; the
moon loop of `generate` reads them from its draws before `makeKind` (`KindFacts::moon`: nothing drawn, so nothing moves). A new fact is a bit
of the enum read there from what the loop already has (never a new draw), and the sources that do not name it take a moon whatever it is.
Do not reuse `lock` or `sync` for a moon: the living kinds' `lock -1` sources take every body, and their moons would change. A fact a reader
needs later on the body itself (the two-faced moon's apex, `Body::apexDir`) is set at the end of `generate` for the kind that reads it only,
from the orbit's elements rather than its positions (four `bodyPos` a moon by differences doubled the generator's time). A test system's moon
the stream shows changed is kept by pinning the kind's salt `salt scan` gives.

**A giant made from a world (P-07's giants).** A made kind with `KT_GIANT` needs a class (`KindDef::giantClass`, `GiantClass` in `kinds.h`):
the globe's ramp and night side (`space_view.cpp`'s `bodyRamp`, `space_globe.cpp`'s `drawGlobe`), its bands and storms (`giantGlobe`), its
kind in the probe (`GiantKind` in `galaxy/probe.h`: its deck list, heat, glow, haze, stratosphere, winds) and its light in the descent
(`MadeLight` in `game/probe_feed.cpp`) read it, and a class whose look is its warmth gives its one-bar temperature across its band of the
game's temperatures (`oneBarLo..Hi` over `oneBarTLo..THi`). Made from a world, it keeps the drawn world's moons (pushed out three of its
radii by `clear`: the companion and the comets read their drawn orbits): a twin drawn after it makes it again by the facts it was made by
with no giant (`drawnFacts`, `KindFacts::paired`), and so does a people's moon it would push (the moons drawn again from the state the
planet's turn began with: `rngAt`). The X-03 ice giant is a drawn gas giant's: `isIceGiant` reads the class. The salts of the drawn giants'
classes were pinned so the protected planets (and B-427's Wolf-Rayet giant) stay as drawn.

**The salts (E-07).** `./vesperis_test salt scan [kind|family]` reads every world of the protected systems (`PROTECTED_SYSTEMS`
in `src/harness/test_worlds.h`: the regress's, the scenes', home's, the probe's, B-427's) with the facts `generate` gives
`kindOf`, walks each through the made kinds as `kindOf` does, and says for each kind the worlds it reaches and takes, and where it
takes any, the first salt (its high word stepped) that takes none: a new kind's to pin. The `world-kind` skill has the rest.

## Add a trait (Q-01)

A trait is a row of `TRAITS` (`galaxy/kinds.cpp`, `buildTraits`) and what reads it.

1. `galaxy/kinds.h`: the id before `TR_COUNT`, after the given ones (`TR_GLASSED`, `TR_CIVILISATION`): R-304's draw lists the traits of
   `TK_FIRST` in id order, so a trait among them would re-deal every world's first three slots. `traitsOk` refuses a new slot's trait before
   the given ones, a trait without a kind, a landform that does not move the ground.
2. The row: `landform(T, id, NAME, phrase, needs, excludes)` for a landform (`TK_LANDFORM`, `moves`), or a `TraitDef` with its kind of trait
   (`TK_WATER`, `TK_SKY`, `TK_LIFE`, `TK_GROUND`, `TK_SOUND`): the name (the data sheet's `TRAITS`), the sentence's phrase (`", ..."`), the
   processes it needs (`KS_...`, any one; a process of its own is a bit the kinds that have it take in `buildKinds`' Q-01 block, so no other
   trait's list moves), a colour trait (one a world), the trait it never comes with (both rows say it), `moves` when it moves the ground or
   what a people's plan reads (then never on a world with a people nor on the home world). The new slots find it by its kind: nothing else to
   wire for the draw.
3. A landform: its case in `landformsAt` (`planet_landforms.cpp`): its province (`prov(k, lo, hi)`, a k of its own), its terms
   (`dhSmooth` moves the smooth ground the rivers and lakes are cut into, `dhFine` the ground alone, `flatten`), the fades (`featFade`
   by a shaped feature's width, `lodFade` a periodic pattern's, `matVisible` where a material may be decided: B-404, never by a faded
   value), its material (`setMat`), its `mark` (1 where it is most itself: the galleries' spot and the scanner's samples) at every scale
   the scanner's 500 m scan reads. Point features stand on a grid of their own (`PointGrid`) with their sizes from the hit's hashes,
   read by the landform and by `landformFeatures` alike.
4. Its sight: a `LandmarkKind` before `LM_COUNT` (its name and glyph, `landformSight`, `SCAN_ORDER`, `SCAN_MODE_NAMES` (twelve characters at
   most: the HUD's panel), `echoDetail`, auto-explore's weight and where it stands), its finder in `sightsOfCellUncached` (points from
   `landformFeatures`, kept where their ground carries the mark; a field from the cell's scan, `SurfaceSample::landformTrait`). The guide's
   file maps the kinds after `LM_WORK` one up (`markKindToFile`): nothing to do.
5. A world trait reads its parameters in `make`'s switch (`for k < TRAIT_SLOTS`), the palette, the look or the surface by `hasTrait`.
6. Look: `traits <name>` (`traits new`: the new slots' landforms), `unit` (the slots' rules, the first three slots' hash against P-07's,
   each new landform's mark and sight), `matlod` (B-404), `regress` (a regress world that drew it moves its map: look, then bless).


## Add a quality (Q-02)

A trait of the water, the sky, the life or the ground (not a landform) is a row as a landform is (`Add a trait`), `quality(T, id, NAME, phrase,
needs, kind, moves, colour, excludes)`, and what draws it.

1. Where it needs a fact of the world besides its kind's process (a forcing, a field, a warm sea), its test in `traitFits`
   (`planet_bodygen.cpp`, read by the new slots' draw): a fact the generator can read from the `Body` (a derived one is set in
   `StarSystem::generate`'s last step, no draw).
2. `moves` when it moves the ground or what stands on it, or takes one of the second people's palette banks on the ground (25-27: free on a world
   without a people); a trait of the sky or a sea's light leaves it off, and then it may come to a people's world and the home world.
3. What it does: a field in the ground (`landformsAt`, `qualityWater`, a ground function), a colour (`materialPalette`; a water's second colour
   through `SurfaceSample::waterTint`/`lifeTint` and the vertex's and the map's channels; the ground's through `groundTint`), the sky
   (`drawSky`, `computeEnvironment`), objects (an enumerator the drawing and the colliders share, as `forMeteorites`), the globe (bank 31: the
   first disc's water colour). Every field unfaded where it decides a material or a colour (B-404).
4. Its sheet: a case in `qualitySheet` (`src/harness/modes_kinds.cpp`): the frames at the hour and the place it shows, beside the frame without it.

## Add a sound of the ground (Q-03)

A trait heard, not seen: its row is a quality's (`TK_SOUND`, `moves` false: it may come to a people's world and the home world).

1. The cue in `SurfaceView::updateGroundSongs` (`surface_env.cpp`, each frame): a level of the surface's (0..1) from what the explorer or the
   vehicle stands on and does and from the hour; a field of the planet function's where it needs the ground's shape (as `slipFace`: an output
   beside the others, no height or material moved), read at the near ring's 4 m where its wavelength is short. One-shots a cue of their own
   (`quakeCue`), consumed by the game.
2. The level in `AudioState` (`audio.h`): cleared in `surfaceOff` (B-413: nothing of the ground aboard), set in `groundSoundsTo`
   (`surface_play.cpp`; the harness's scenes hear through it too), as much of it as `air` carries.
3. The sound in `AudioSynth::groundSample` (`audio.cpp`): its smoothing in `render`'s loop, its filters' constants in `coefs` (F-07), its gate in
   `render`'s call (silent costs nothing).
4. Its levels: a row of `groundSounds()` (`src/harness/modes_signals.cpp`): `vesperis_test sounds` writes its WAV with the peak and the rms, and the unit
   check bounds them (none at the synth's clamp, the master's 0.8; none silent). A scene with its cue (a finder code and a few lines in the scene
   runner), and the cue check: where its world has it and not without the trait.
## Reach a place that is not a star, or read the sun where there may be none (D-01)

A drifter is a place as a star is: a `Star` of class `STAR_ROGUE` in a sector without a star (`galaxy/drifters.h`), its key the sector's.

1. A key that names a place turns into it through `placeInSector` (the sector's star, else its drifter), never `starInSector`: the save, a
   target, the history, a record's world, a chart's mark. `starInSector` stays where the stars alone are meant: the star field, the
   neighbourhood and what is drawn from it (the aim, the star map, `N`), the jumps, a new game's home, the radar's scans of the stars.
2. A reader of the sun asks `StarSystem::sunless()` where the stars' rule would light or place something: `lightAt` (factor 0), `seasonOf`
   (0) and `SurfaceSite::sun` (the nadir, no light) already answer for a drifter, so a reader of those needs nothing; one that reads
   `star.pos`, `star.radiusKm` or `star.luminosity` itself does (body 0 sits on the anchor: `normalize` of a zero vector is (0, 0, 1), and a
   division by the distance to the sun is a division by zero there). The almanac's finders skip the sun's events by it.
3. A ship's arrival, its Vimana's end and anything that stands off a place: a drifter's distance is its body's (`drifterStandOffKm`), not a
   star's first orbit (0 there).
4. `vesperis_test rogues` flies to the nearest landable drifter and lands; `unit`'s drifter checks look for undefined numbers from the anchor,
   a lit place or a season, a save that does not come back.

## Let the radar hear a new kind of source, or draw where no sun shines (D-02)

1. A signal kind is appended to `SignalKind` (never inserted: the guide's `signal` lines keep a lock's kind), with its name in
   `SIGNAL_KIND_NAMES` (the harness's; the game shows none), its sound in `AudioSynth::channelSample` (a `case` of its own, or a kind's
   whose sound it borrows for now), a line in `signalSourceLine` and the harness's `signals wav` names.
2. Where it is heard from: a population of sources far off is a layer of `SignalScan` (its cells, its own reach at the dial's top so the
   scan's box is right, each candidate tested by its own law before any system is generated, the list kept whole for the dial to filter by
   `needGain`); x1 is C-07's list by rule, so a new law gives nothing at x1. A source of the system the ship is in is a branch of
   `localSignals`. C-15's unit check measures the peoples and the pulsars by their law; a new kind is left out of it and gets a check of
   its own (`drifters: heard by their own law`).
3. The lock: its readout, status and log in `renderRadarCamera` and `radarLock` say what the explorer may know (the sound tells, the
   readout never does), and its key in `Guide::signals` is the one the chart and `N` read (a sector's key is the place's).
4. A globe, a map or a sky drawn in a sunless system asks `sys.sunless()`: in space the band's light (`galaxyBandLight`, kept with the
   band's build) is its floor and every term of the sun's is off (`lit` -1); a map colours by `mapColor`'s `survey` with the map's height
   range (`surveyRange`). Their numbers, not their pictures, prove a dark frame (`rogues heard` prints a disc's middle row).

## Give a signal a voice of its own without moving another's (D-03)

1. A kind's parameters are drawn in `AudioSynth::rvDerive` from the seed by `u(k)`; a new voice takes indices no kind reads yet (D-03's
   from 161, in a function of its own called at the end of `rvDerive`), so every old parameter, and every old sound, stays as it was. The
   synth's noise is one stream (`AudioSynth::noise`): a new voice may draw from it, but only inside its own `case`.
2. What the voice needs besides the seed travels on `AudioState` beside `radarKind` (`radarPulseHz`, D-03's `radarRogue`), with its twin for
   the signal under the centred one (`radarKind2`, `radarRogue2`) kept by the synth while that one fades (`rdRogue2`); the radar sets both from
   the `Signal`, which carries what its source is (`turnS`, `moonS`).
3. Shared code moved for the new voice must keep its arithmetic: the programmes' machines moved out of `case 0` into `machineInto` add into
   the caller's `sig` by reference in the old order, so a people's WAV is the same file.
4. The proof is `signals wav` against the last binary's: every old WAV byte for byte, only the new kind's moving (`build/d03/verify.sh`).
   Levels by rendered WAVs (C-07's rms 0.02-0.13, no peak at 1, at most 20 clipped); distinctness by voice prints against two signals of
   one kind (`rogueDistinctness`); a picture of the sound (a spectrogram drawn by a scratch tool) shows what the numbers cannot, and the
   synth's cost per second of sound matters (the game pumps it on the main thread): a band's coefficients made when the voice changes, a
   slow pitch every 32 samples, a steady tone as a turning phasor.

## Light the ground, or give a world in the dark something of its own (D-04)

1. A light on the ground goes through `SurfaceView::lampAt(p, n)`, not a formula of its own: `setupLamps` sets the frame's lights once
   (`LampLight`: a place, an axis, the cone's cosines, the falloff, the power, already scaled by the night), and every light formula adds
   `(1 - ambient) x lampAt` (the ground's vertex light, each object's face or point; the flora raises its band's `light` and `leafBase` for a
   tree or a cell). Scale a light by the night (`1 - sunUp`) as the vehicles' are, and keep it off when nothing is lit (`nLamps` 0): a
   dayside's frame stays the same lit or not (a unit check), and the regress with it.
2. A body's air is `bodyHasAir(b)` (D-04: an air fallen as snow in the dark is none), never `KINDS[type].atmosphere`: the site, the almanac,
   the fronts, the aurorae, the landing map and the globe all read it.
3. What only a drifter's world has is a `TK_DARK` trait (`dark` in `buildTraits`: given, never drawn; `darkTraitsOf` decides it from the
   body), placed first in the new slots, and its ground behind `BodyGen::dark` (`darkGround` after the kind's own: no other world samples
   it, so fp5 proves the rest unmoved). A process a body in the dark lacks goes into `DARK_LACKS` (or the frozen air's or the frozen seas'
   masks) so the slots never draw a trait it cannot have. `thenWorld` clears `dark`: a people's time had its sun.
4. A new kind for the drifters alone is made from the drawn one (`drifterBody`: the same uniforms in its own ranges, `drawnType` and
   `drawnRadiusKm` kept), never a new row of `DARK_KINDS`: a weight added there re-deals every drifter's kind, their moons, their power
   and their voices.
5. A light seen from orbit is a point drawn over the globe after its disc (`drawDarkLights`): small ones as squares two pixels wider than
   the scale (the frame's two passes of the mush keep their middle), in a bank whose ramp holds the light's colour at the shades drawn.

## Give a people an end of its own without moving another's (D-05)

1. A new cause goes after the drawn ones (`CAUSE_SUN_LOST` after the 37: `CAUSE_DRAWN` is the old count every pick runs over, `Rng::pick`
   falling through to the last on a rounding), given where it applies (`peopleProfileOf`), never drawn; the tables sized by it grow in step
   (`CAUSE_NAMES`, `CAUSE_KEYS`, `SCRIPTS`, `LEFT_BY_CAUSE`, the harness's cause tables) with a `static_assert` on each unsized one. A new
   event kind, hazard, order or whither is appended too; where one must sit before others (`HE_SUN` before the families' kinds), every
   table and every saved number of the kinds after it moves with it: prove the old histories with the event kinds mapped back
   (`build/d05/recdump.cpp`).
2. Every draw of the new people's own takes a salt of its own (the drifter's people, its way, its former sun, its works of the end, its
   beacon), and none comes before a record's first uniform (it is the record's year, age and tone at once). A step that other peoples take
   too (the inventions' stop, the ways' years, a cast's fallback) branches on the cause, so theirs runs as it did.
3. Its records' words go in files of their own under `@apart` (or a guard `@apart key=value` for its lines in the shared rules): the
   distinctive-word statistics never count them, so no other people's word becomes common. A shared rule the new people's facts reach
   wrongly is swapped for it alone (`@instead cause=sunlost letter.apprentice letter.apprentice.lost`), never edited: an alternative's key is
   its rule's name and its whole line, and a people's dialect reads the key. A new alternative in an existing rule must keep its rule's
   flags (no directive, no stop where it has none) and never join a rule of one alternative (that changes `many`).
4. A grammar file holds at most 16 KB (a unit check); a rule cannot continue in another file, so a full file's rule points at a new rule
   elsewhere (`{ann.sunomen}`). A genre's `words=` is the check's alone: widening it changes no text.
5. Prove it: the unit's pin over the age and scene worlds' peoples (their histories, catalogues, homes, texts and charts' marks), and
   `recdump` over every 25th people's world near home against the last milestone's objects, byte for byte.

## Add a star class

1. `galaxy/starfield.h/.cpp`: enum value and `STAR_CLASSES` row (D-01: before `STAR_ROGUE`, which stays the last: the picks over the classes stop at `STAR_DRAWN_COUNT`, and a drawn class appended after the drifters' would move it) (code, name, description, colour, radius, variance, luminosity, mass, max planets, rarity, first orbit multiplier, minimum first orbit). Since S-01 a new class is a *variety* with rarity 0: it takes a share of one of the six families in `starVariety` (by region), so the stars it does not claim keep their class and the pinned sites hold; a family row with a weight of its own would re-deal every star.
2. `space/space_sun.cpp` `drawSun`: glow multiplier and disc appearance branch (and the rays for compact stars; S-04: a pass of its own for a feature along the orbital plane, `discUp`; S-05: a feature round the star within a few radii goes in the glow's pass, widening `outerA` to reach it; S-06: a star that is not a disc of light at all takes a function of its own at the top of `drawSun`, `drawBlackHole` is the model, with the frame copied before it bends anything); `starNebulaPatches` if the star lights its own sky; `drawGlobe` if its worlds look different (S-05: `drawStrippedTail`; S-06: `drawAccretionStream` on the companion).
3. `galaxy/system.cpp` `pickPlanetType`: class modifiers if the class should shape its worlds; `generate` for the locking, the companion chance (`pMultiple`, one entry per class) and `starActivity` for the star's activity (the aurora, the scene finders and the unit check read it).
4. `game/hud.cpp`: radiation warning distances if hazardous; `galaxy/describe.cpp`: the data sheet's sentence. The star map's filter, the statistics and the analyzer read the table (S-02); nothing in the UI needs a change. A new class is the next code (the plan's table is renumbered as the classes arrive: S10 is the neutron star).
5. `./vesperis_test space` renders every class from the first orbit; `unit` checks the class mix near home.

## Add a building kind or a settlement plan (C-01)

`peoples/ruins.h` `BuildingKind` / `SettlementPlan` and the name tables in `ruins.cpp`; the layout in `layoutSettlement` (place with `Layout::add`, which keeps buildings apart and draws the decay and the seed) and the pieces in `elemsBuilding` (`Form`s, A-02's `peoples/forms.h`: C-01's boxes and domes or any of the twelve kinds, from the building's own seed at the three levels of detail, building by building in order (the view groups them so each building is drawn at its own level); a piece standing on the ground with `y0` under 0.01 is sunk a metre by the view, a big one stands on the highest ground under it with its foot down to the lowest; each kind's colliders are `formColliders`'). The surface view (`gatherSettlement`, `drawFormItems`, `collectColliders`, `forTrees`) and the landmark finder need no change; `vesperis_test forms sheet` shows a new kind whole and broken at every level, `forms check` walks into it; `vesperis_test unit` checks every house's room and doorway, `scene civilisation_town` shows a town from its gate.

## Add a material

`galaxy/kinds.h` enum before `MAT_COUNT` (P-01: the mud, the potash and the brine; P-02: the mats, the stromatolite, the crust and the moss); `planetmap.cpp`: `MATERIAL_NAMES`; `planet_bodygen.cpp`: `matFamily` (the bank it shares: a material of its own colour needs a family whose rep it recolours, as the glass recolours the water), `materialPalette` (its colour per body); `surface/textures.cpp`: `materialHasMesoTile` and a `buildMesoTile` case; `surface/surface_view.cpp`: the specular list if it glints, `drawObjects` and `collectColliders` if rocks avoid it; `src/harness/modes_space.cpp`: the `matCol` of the `maps` renderer. The landing map, the globe and the surface take the colour from `matColor`.

## Add a HUD readout

Draw in `renderSpaceHUD` or `renderSurfaceHUD` with `drawTextShadow(canvas, x, y, text, colour, HUD_SHADOW)`. Keep lines under ~52 characters (320 px / 6 px per glyph); use `trunc(name, n)` for names. Text is uppercase only.

## Add a key

Keys are GLFW codes in `core/input.h`; the platform copies raylib's `IsKeyDown/IsKeyPressed` by number. Read `in.wasPressed(KEY_X)` for edges, `in.isDown` for holds. Document it in the help pages (`renderHelp`) and in `README.md`.

## Add a game state

Append it to `GameState` (`game/game_types.h`; `STATE_COUNT` names the last state) and its row to `Game::STATE_ROWS`
(`game/states.cpp`, U-02): its name; its traits (the clock runs, an overlay drawn over its `returnState`'s scene, the cameras
stowed, the dark radio silent, a save's place, the mouse, the photo tools, typing, the ground, the ship moving on); its
controller context; its `update` and its picture, which the frame runs; an overlay's `covers` if it ever covers the whole frame,
and an `away` if its screen stops something off it (a sound). The compiler checks the order, and that the row names its update
and its picture. Code asks `stateHas(state, ST_...)`; a trait new code needs is a column, not a list of states (E-06). Then:
- its file of `Game` methods, the row's among them (an update takes `(const Input& in, double dt, double realDt)`), declared in
  game.h under a banner naming the file; the function that opens it sets `state`, `returnState` and `guideReturn`;
- pad.cpp's screen names and `padItems`; `autoScene`;
- its way in: a cabin spot (a `CabinFacing`: `updateCabin`'s spots, its colour in `SCREENS`), a flight computer row (`PAGE1` or
  `PAGE2` appended, `counts[]` and `n` both), the Esc menu (game.cpp's `menuStep(in, menuSel, 8)` and menus.cpp's `items[]`) or
  the guide menu (`GUIDE_ITEMS`, the shifted `case` numbers, 16 rows at 9 px); a menu's labels are its `MenuLabels`
  (game_types.h), by which the harness finds a row;
- in the harness: its stop in `modes_tour.cpp`'s `STOPS` (how the game opens it, its keys; the count and the order are
  asserted, so the build fails without it), a `screens` capture if a frame needs its scales, and a TestGame hook if a check needs
  one; `tools/prove compare <base> tour` then shows its run.

## Add a save field

Append a `key value...` line in `save()` and parse it in `load()`; unknown keys are ignored by older builds, so this is backward compatible. Bump the header only for incompatible layouts.

## Add a vehicle

The buggy's pattern: a struct in `surface/surface_types.h`, its own `surface/*.cpp`, the camera branch in `render`, the update
dispatch, `reanchor` and `init`, the seat's HUD branch in `game/hud.cpp`, the keys and the audio in `game/surface_play.cpp`, a
save line, and `inVehicle()` wherever "in the buggy" meant "seated".

## Add a palette effect

Palettes are rebuilt every frame (`setupPalette`), so time-varying effects are cheap: modify the ramps or scale `fb.pal` after `setupPalette`.

## Change terrain detail or draw distance

`SurfaceSite::init` sets cache cell sizes and torus sizes (the torus must be larger than twice the draw radius in cells); `SurfaceView::render` sets radii `r0, r1, r2` and the inner skips (`(r0*16)/64 - 1` etc.). Keep LOD1's inner skip just inside LOD0's radius to avoid gaps.

## Add a record's genre, a register, a rule or a word (C-02, H-03, H-04)

The texts are written by the grammar engine from the `.vg` files on `src/peoples/grammar/`'s shelves (its note; the format:
`civilisations/records-and-grammar.md`, "The grammar engine"; the records: "The records (H-04)"; the `grammar` skill).

1. Find the rule first: `vesperis_test grammar where <rule|prefix.*>` (its file and line, what names it, what builds its name).
   Edit the `.vg` file and read the result without a rebuild: `vesperis_test grammar` reads the tree's files, prints the link's errors
   (an unknown rule, fact, entity or field, a guard's unknown key, a rule no root reaches, "a" before a plural list: each with its file and
   line), runs every people's records of the test worlds, the age worlds and the forty nearest home through the checks (every name the
   history's at the record's year), and writes them all to `shots/tests/grammar_texts.txt`; `grammar ages` prints fifty of one people of
   each age to `shots/tests/records_<age>.txt`. Read fifty before judging: the checks catch the grammar, not the sense (a door drunk from,
   twelve elk "turned at bay, its head down", a diary that says "you"). Then `make` (the files are compiled in) and `unit` (it checks the
   tree's files are the text compiled in, each under 16 KB: split a file that grows past it).
2. A genre is an alternative of `record` in `core/records.vg`: its age and its media (`age=industrial medium=printed|medium=codex`), what
   the history must offer (`has:pair`, `has:writer`, `has:dead`, `has:event`, an invention `inv.telegraph=1`), the collapse's `stage` for
   its witnesses, and a weight (the rule is `steady`: a people's shares hold). Its rule `genre.<name>` (`words=lo..hi`: what it writes, the
   length check) in a file of its age's shelf (`ages/<age>/`) named for it; its heading and closing `(formula)` rules, left out of the
   measures. Bind its persons with `@cast` and tell the history with `@news`, `@entries`, `@years`, `@death`, `@life` or `@event`; a
   register's rules are `<register>.<kind>`.
3. A register is a way to tell an event: a rule per event kind (`annal.war`, `letter.sickness`), the event bound as `$e` (its fields:
   `{$e.place}`, `{#$e.value:about}`, `$e.won`, `$e.wave`, `$e.out`), `.none` when there is nothing to tell and `.quiet` for an empty year;
   `registerOf` in `peoples/genres.cpp` names it (the salience gives a great event told in another register its other side).
4. Compose within a sentence: a part a people draws a hundred times needs many frames (`[a|b|c]` inside a line, two or three slots a
   sentence), and a tail that only fits one part goes with that part (`{=.many}` and `(.many=1)`, or the part and its reactions on one
   line). Persons are entities: `{$me}`, `{$you}`, `{$x}`, `{$kin}`, `{$child}`, `{$spouse}`, `{$ruler}`, `{$founder}` (the realiser writes
   "my mother", then "she" or "my mother" again where a pronoun would not be clear), their cases `{$kin:obj}`, `{$kin:poss}`, a vocative
   `{$you:voc}` (the given name), the name `{$child:name}`; a line that speaks to the reader is guarded `has:$you` (a diary has none). Never
   write "he", "his" or a verb's ending for a person; a verb whose subject changes is `{walk:$x}`, `{be:.w}` after `{rule>.w}`.
5. A word list is a rule in `core/world.vg` or `core/people.vg` with guards on the facts (`desert=1`, `cold=1`, `moons>0`, a trait `erg=1`)
   and its number (`(pl)`, `(mass)`, `(sing)`: no "a" before a plural); `(keep)` makes it the same all through a record. A new fact is the
   model's: add it to `RecordFact` and its name in `peoples/genres_schema.h` (the same order) and set it in `recordWorldFacts` (the world
   and the people) or `yearFacts` (the record's year), never from a site. A word of a later age goes behind a guard (`(age>=medieval) iron`)
   and into `core/lexicon.vg`, by age or by the invention that brings it (`@lexicon inv.railway train trains`).
6. A number with its noun is `{#n|day}` ("one day", "five days"); a rule of counts is written as it is read ("twenty", "a hundred").
7. H-04's second pass: make it a people's own. A thing a people names its own way is a culture pool (`core/culture_*.vg`) with `people=K`
   (each people keeps K of those whose guards hold): one number for every word in it (no "the" in one entry and not in the next: a writer
   will put "the" before it, or not), and a word of a later age behind its guard. A thing with no word of ours is `{~key}` (their coin,
   their dish: lowercase, kept by the decoder). A quip, an image, a joke a reader would notice twice is in a rule with `share=0.3` to `0.5`,
   so each people has its own part of them. A sentence a people draws often is built of parts with two or three of the people's own words in
   it (its crop, its vessel, its craft, its house, its colour): that is what makes it no other people's; names do not help (the measures
   mask them). Read `grammar`'s genre table: "between peoples" (the share of the genre's sentences on a frame another people wrote) and
   "within one" (the people's own repeats); `VESPERIS_TOPFRAMES=8` prints each people's worst. A heading or a closing that is the genre's
   habit is `(formula)`.
8. Several writers side by side: each copies the tree (`cp -R src/peoples/grammar D`) and the binary, works in its own files with its
   own rule prefix, runs `vesperis_test grammar dir D out D/texts.txt` (from the repository's root), and copies its own files back; a new
   file is read from disk at once (the shelves walked), and compiled in by its three lines at the end of `grammar/files.inc` (the unit
   fails a file the build does not list).

## Add a tongue's sound, a name's shape, or an everyday event (H-04's second pass)

1. A people's tongue is `peoples/tongue.cpp`: a consonant set is a row of `CSETS` (codes from `CONS`, each sound with its spellings), a
   vowel set a row of `VSETS`, a syllable shape a string of `SHAPES` (C, V, L, S, F, `?` after a part it may leave out). A name's shape by
   its kind is `tongueName`'s switch (a place by its rank: a root, a root and a generic word before or after it, an adjective and a root);
   the generic words are the `TM_` meanings. A name that reads as a word of ours is caught by `unfortunate` (add the word). `vesperis_test
   tongues 60` shows sixty peoples' names side by side and how often a name tells its people; the unit asks 65% over 24.
2. An everyday event is a row of `EVERYDAY` in `peoples/history_tables.h` (its kind in `HistoryEventKind`, before `HE_BIRTH` for a public
   one, after `HE_DEATH` for a family's; its name in `HISTORY_EVENT_NAMES` (`history_names.cpp`), `KIND_KEYS` (`genres_schema.h`) and the
   event's noun in `peoples/genres.cpp`; its words with the ages and what each needs; its rate by age and its salience): drawn from the
   year's own stream (`Builder::everyday`) or a person's (`Builder::family`), it never harms anyone or makes a person, so the rest of the
   history stays. Then the registers that tell it (`annal.<kind>`, `letter.<kind>`, `news.<kind>`, `log.<kind>`, `song.<kind>`,
   `sermon.<kind>`) with `$e.what` for its word.

## Move the shards, or add a place they lie (C-03)

1. The counts per settlement class and the buildings' weights are in `recordSitesOf` (`peoples/records.cpp`; T-04 replaced C-03's
   `shardSitesOf`, see "Add a record's medium, or change where records lie");
   since A-03 the candidate spots are the building's room's (`buildingRoom`, `peoples/buildings.cpp`: a `case` per kind, in the
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

1. `languageShare` (`peoples/shards.cpp`) is the curve: 0.3 with one shard, linear to 1 with ten; `languageKnown` turns it
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

1. `traditionOf` (`peoples/music.cpp`) draws a world's tradition from the body's seed: the period (the octave, now and
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
3. The synth (`AudioSynth::musicSample`, `game/audio_music.cpp`): ten voices, each a note of a timbre (six partials with the
   velocity opening the upper ones, an attack-decay-sustain-release envelope, vibrato after 0.3 s, breath noise; the drum
   a falling thump and noise), the clock in beats wandering by the tradition's drift. `AudioState::pieceGain` is the
   level; a null `piece` stops everything at once (the pointers must outlive the play, which is why the decoder clears
   them before `shardWorld` can move). Which shards are music is `shardIsMusic` (a shuffle of the fifty from the world's
   seed, twelve to eighteen of them): changing that moves the texts' indices, so the sites of C-03 keep their shards but
   some rooms hold a piece where a text was.

## Tune the voice: a people's speech (C-05)

1. `PHONES` (`peoples/voice.cpp`) is the table of sounds, one row per spelling the tongue's pools and the names use (longest
   spellings first: `sh` before `s`): a vowel's three formants (a diphthong's two sets), a stop's locus and burst, a
   fricative's noise band, a sonorant's formants, each with a length. A new spelling in `T_ON`/`T_NU`/`T_CO`
   (`peoples/shards.cpp`) or in `generateName`'s pools needs a row, or its letters fall back to the single-letter rows
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

1. The streets are `townStreets` (`peoples/towns.cpp`): a pattern is a function of `Net` over a ring (`grid`, `spokes`, `rings`, `organic`,
   `lineTown`, `branching`, `suburbs`), called by `Net::pattern` for each layer's age; a street is laid by `line` (a medieval one crooked by
   its wobble) or `arc`, clipped to its ring by `seg`. The end of `townStreets` clips them to the shore and keeps the one web: a pattern
   needs only lay streets that meet. Widths are `streetHalf` (by rank and age, times the class's `widthK`), the blocks `streetSpacing`.
2. The buildings are `layoutTown` (`peoples/ruins.cpp`): `districtAt` says a lot's district, `lotSpec` its building (a dwelling through the
   layer's `Layout::dwellingSpec`, a work, a tower, a compound, a garden), `setback` and `gapAfter` its place on the street, `density` how
   often a lot is built. A new building placed off a street: `Layout::add` with its fit tested (a rectangle against the others, off the
   streets), then `intrusion` (its own pieces off the streets) and `undoTo` if they reach one; mark its `district`.
3. A work of the water or the land of an age: `addFeatures` (the harbour by `c.age`, the reservoir, the terraces): its buildings
   `L.add` with their fit tested or `segmentClear`/`offStreets` first; a new kind gets its generator in `peoples/buildings.cpp` (see A-03's
   recipe) and its feature flag in `RuinSpec` and `featureList`.
4. `vesperis_test towns <sx> <sz> <body>` (the plans drawn from above), `towns view` (through the renderer), `towns check` (no building
   across a street, the streets walkable, the old cores, the districts, the harbours, the ground's marks), `ruins check`, `genome reach`
   (a variable that acted in the old plans must still reach the drawing).

## Add a record's medium, or change where records lie (T-04)

1. A medium is a `RecordMedium` (`peoples/records.h`, before `RM_COUNT`) with its harness name (`RECORD_MEDIUM_NAMES`) and what the HUD and
   the log say of it (`RECORD_MEDIUM_HUD`: a description, never a name); the ages and categories that made it are `mediaOf`'s table in
   `peoples/records.cpp` (a weight by the record's age and category, `f` how far into its era the year falls), which `recordMediumFits`
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

1. A kind is a `WorkKind` (`peoples/works.h`, after the last: the guide keeps none, only `LM_WORK` marks) with its name in `WORK_KIND_NAMES`
   and what the scanner hears in `WORK_HEARD` (a description, never a name), and a glyph in the harness's `WORK_GLYPHS`.
2. Its rule is a member of `WB` (`peoples/works.cpp`) called in `buildWorldWorks`'s order (the history's works first): probe the ground with
   `at` (64 m, the rivers off) and `grade`, find a place (`summitNear`, `shoreNear`, rings round a place, the plan grid's cells), keep it
   `clearOf` the settlements, the cores and the works before it, then `add` it with its age, its place and its event (`placeAge`, `eraAge`
   for the history's years; `cleared` for a place its people cleared). What it does to the ground is a `GroundEdit` in its own metres (a cut,
   a pit, a ditch, a bowl, a flat, water, lines, terraces); a new kind of edit is drawn in `SurfaceSite::applyEdits` on the rings that can
   show it and listed in `editCover` if nothing should grow on it.
3. Its pieces are `workSpec`'s case: `Lay::add` buildings in the work's metres (a new kind's generator in `peoples/buildings.cpp`, A-03's
   recipe; heights over the reference level with `Building::base` and `Form::absolute`), `Lay::plot` for a plot of graves.
4. `vesperis_test works <sx> <sz> <body>` (its line), `works view ... <word>` (through the renderer), `works gallery`, `works map` (with its
   place), `works check` (and a check of its rule beside the dam's, the quarry's and the battlefield's).

## Read a people's history, or add an event to it (H-02)

1. A feature that reads what happened (a town's old core, a battlefield, a record) takes the history once (`historyOf(sys, b, g,
   people)`, cached; it builds the world's plan first if it must, so ask it off the frame, as the plan is) and reads it by year
   (`eventsIn`, `reignAt`, `harvestAt`, `inventionYear`, `historySeat`), by person (`eventsOf`, `historyKinship`)
   or by its lists (`places`: the plan's by rank, then a later people's cleared cities; `wars`, `sicknesses`,
   `institutions`, `families`). A place's `age` is the age of its old core, its `ended` and `fate` how it was left.
2. A new kind of event: its entry in `HistoryEventKind` with its name (`HISTORY_EVENT_NAMES`) and the slots it fills (the comment by
   it); where the year loop makes it (`randomEvents` for a rate, `setupPlanned` for a year fixed in advance, `collapseYear` for the
   collapse's) with its cause and its places standing; its sentence in `historyEventLine`. A person it names must be alive that year
   and a place it names standing (the unit checks both); a harm that kills goes through `harm` so the families' deaths name it.
3. `vesperis_test history <sx> <sz> <body>` (the legends), `history ages`, `history check` (the unit's five checks in a few seconds).

## Add a variable to the architecture genome (A-04)

1. Its entry in `GenomeVar` (`peoples/genome.h`; the `static_assert` counts thirty-five: a thirty-sixth moves it), its values' enum,
   and its row in `GENOME_VARS` (`genome.cpp`): its printed name, its group, its categories (0 a measure with its two ends), the ages
   it acts in (a bit an age: the unit's reach check holds it to them) and its weight among the strongest (0 not one of them; a weight
   moves `genomeMaxDistance` and the alike threshold's meaning). Its draw is its own hash (`genomeOf`): nothing else moves.
2. Where it acts: a way of the culture (`cultureWays`, `peoples/buildings.cpp`), a generator reading it (`Gen::gv`, `gf`), the palette
   (`culturePalette`), a plan (`Layout` in `ruins.cpp`: `formOf`, `foundOf`, `dwelling`, `dwellingSpot`, `centre`, `focus`, `stela`,
   the plan's `case`). What a room depends on goes into the `Building` (as `form`, `sides`, `found`, `centred`), so `buildingRoom`
   stays a function of the building alone.
3. `vesperis_test genome reach` (every age it claims), `genome sheet` and `genome ages` to see it act at a scale the picture shows,
   `ruins check` (nothing on air, every doorway open), `genome distinct` if it weighs among the strongest.

## Add a kind of building to an age (A-03)

1. A `BuildingKind` (`peoples/ruins.h`, with its `BUILDING_KIND_NAMES` entry: a `static_assert` counts them), its age in
   `buildingAge` and its place's name in `buildingPlaceName` (`peoples/buildings.cpp`).
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
   `08-testing-and-tools.md`'s reference paragraphs, the decision and what was dropped into `docs/decisions/log/`.
