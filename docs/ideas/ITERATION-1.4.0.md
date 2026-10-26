# Iteration 1.4.0: the worlds and the wanderers

Planned 2026-10-21 on the user's request, on the branch `release/v1.4.0` (from `main` 9c4ebc2, 1.3.2's merge): "this
one will be focused on improved planets generation". Nothing built. The design and its lists are
`IDEAS-worlds-and-wanderers.md`; this file is the milestones, their checks and their order. The user starts each milestone
with "proceed".

**The kinds of world** (`R-421`): thirty-eight new kinds of body (the user: 30-50, "not too ridiculous so it's still
scientific enough"), each from a real world or a published model: six furnace worlds of the inner orbits, six living worlds,
six of the locked and the deep (red dwarfs' worlds, water worlds, other liquids), two dead felisian worlds that host peoples
(the user's "dead felisian world"), six of the far cold, five strange moons, six giants (Sudarsky's classes, the gas dwarfs, the
super-puffs) and the drifters' own dark ocean. 59 kinds of body after it, 51 landable.

**The qualities** (`R-422`): thirty new traits (the user: 20-30 "in addition to what we have now, also greater variety of
modifiers in general") in six kinds (landforms, water, sky, life, ground, sound), up to six traits a world instead of three
(the three a world has today kept), nine facts read from the orbit and the spin on a line of their own, and four traits only a
drifter has.

**The drifters** (`R-423`): rogue planets, worlds with no sun, any kind that can exist in the dark, found only by the radar,
each with a voice of its own in the cosmic horror the radar already leans toward; one in four of the dead living worlds among
them kept a people whose records tell, as no other people's do, how their world lost its sun and how they died.

**Generation.** One change, 16, from P-01: the new kinds come from the old draws by hashes of their own, so every system's
orbits, sizes, moons and rings stay where they are and most worlds keep their kind (about one in three changes); no world
with a people changes, so the peoples, their towns, their records and their signals stay where 1.3.2 put them, and the home
world stays felisian. P-00 changes nothing generated or drawn. The traits' new slots (Q-01) leave the three old ones as they
are. The drifters are additive: they live only in sectors with no star, so no star or system moves (D-01). Saves of 1.3.x load
with the regeneration notice.

**The repo for its readers** (added 2026-10-23, after D-05, on the user's request: "it's crucial to optimize it now before we go
any further"): thirteen milestones, E-01 to E-13, that make the repo cheaper for an agent to read and change (the harness split, the
docs indexed, game.h and the largest files by subsystem, a permanent proof kit, CLAUDE.md files and skills). Nothing generated,
drawn, played or saved changes. The plan, its measurements and each milestone's done-when are `PLAN-reorganisation.md`.

Left out on purpose: the V series (parked), S-07, W-05, the L and K series, the shepherd moon (P-11 of the old list, a place)
and the pelagic island (P-12, kept for a later trait). W-02 (tides) comes in as the GREAT TIDES trait (Q-02). The old P list's
twelve kinds are re-cut into the families below (`MILESTONES.md`).

## Order

| # | Phase | Id | Item | Size | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Groundwork | P-00 | The kind table: every kind a row, the planet function's shared skeleton, accent families, traits by tags; nothing drawn changes | XL | done 2026-10-21 |
| 2 | The kinds | P-01 | The furnace worlds: magma ocean, chthonian, glass, steam, salt, rust; the kinds drawn from the old draws; generation 16 | L | done 2026-10-21 |
| 3 | The kinds | P-02 | The living worlds: archean, hothouse, superhabitable, steppe, tundra, lichen | L | done 2026-10-21 |
| 4 | The kinds | P-03 | The locked and the deep: eyeball, terminator, hycean, ammonia, snowball, chemotroph | L | done 2026-10-21 |
| 5 | The kinds | P-04 | The dead worlds and their peoples: frozen felisian, scorched felisian | L | done 2026-10-21 |
| 6 | The kinds | P-05 | The far cold: nitrogen, cryovolcanic, dry ice, wanderer, brine dwarf, frost dwarf | L | done 2026-10-22 |
| 7 | The kinds | P-06 | The strange moons: grooved ice, two-faced, tiger-striped, patchwork, sponge | M | done 2026-10-22 |
| 8 | The kinds | P-07 | The giants: water-cloud, azure, alkali, silicate, gas dwarf, super-puff; giants in the warm zones; the probe's decks | L | done 2026-10-22 |
| 9 | The qualities | Q-01 | Six slots, the facts, and eleven landforms | L | done 2026-10-22 |
| 10 | The qualities | Q-02 | Water, sky, life and ground: sixteen traits (the tides among them) | L | done 2026-10-22 |
| 11 | The qualities | Q-03 | The sounds of the ground: singing dunes, singing ice, quakes | M | done 2026-10-22 |
| 12 | The drifters | D-01 | The dark between: the drifters' field, a system with no sun, the way there and back, the save | L | done 2026-10-23 |
| 13 | The drifters | D-02 | Heard, not seen: the radar finds them, the lock, the jump into the void, the black disc, the map without light | L | done 2026-10-23 |
| 14 | The drifters | D-03 | The voices of the dark: a voice for every drifter | L | done 2026-10-23 |
| 15 | The drifters | D-04 | The ground in the dark: the explorer's lamp, frozen air, embers, the lights left on, the dark ocean | L | done 2026-10-23 |
| 16 | The drifters | D-05 | The people who lost their sun: the history's new end, its records, charts and music, the beacon | XL | done 2026-10-23 |
| 17 | The repo | E-01 | What every session loads: the memory, the root CLAUDE.md with the map, the directory notes, the guards; the eight tasks measured | M | done 2026-10-23 |
| 18 | The repo | E-02 | The proof kept: fingerprints of every module, `tools/prove`, the almanac's order explicit, a safe `make clean`, `make check`, clangd | L | done 2026-10-24 |
| 19 | The repo | E-03 | The harness in pieces: `src/harness/`, a mode registry, the unit by groups, the scenes by family | L | done 2026-10-24 |
| 20 | The repo | E-04 | The docs for their readers: rules by area, the log by release, STATUS, the harness index, an archive, lean touch points | L | done 2026-10-24 |
| 21 | The repo | E-05 | The game's header: the hooks into the harness, game.h by subsystem, the platform's asserts | L | done 2026-10-24 |
| 22 | The repo | E-06 | The game's files: the mixed files split, the state table, the new-screen checklist | M | done 2026-10-24 |
| 23 | The repo | E-07 | The worlds: planetmap split, the engine headers' hook sections, `salt scan` | L | done 2026-10-24 |
| 24 | The repo | E-08 | The views: surface_view and space_view split | L | done 2026-10-24 |
| 25 | The repo | E-09 | The peoples' directory: `src/peoples/` with its grammar; buildings, works and ruins split | L | done 2026-10-25 |
| 26 | The repo | E-10 | The peoples' texts: history, genres, grammar.cpp, forms and music split, `lore.h`; the Linux check | L | done 2026-10-25 |
| 27 | The repo | E-11 | The grammar's shelves: a manifest, folders by role, files named by topic, `grammar where` | M | done 2026-10-25 |
| 28 | The repo | E-12 | The sweep: shared helpers, dead code, comments by the policy, long lines, the warnings | M | done 2026-10-26 |
| 29 | The repo | E-13 | The close: the CLAUDE.md tree audited, the eight tasks measured again, the old kits | S-M | planned |

Each row's `Status` moves to `done` (with the date) when its milestone is finished, and the matching row of `MILESTONES.md`
gets `1.4.0, done <date>`. This table is the order.

## Why this order

1. **The table before any kind.** A kind is spread today over 25-35 places in ten files, half of them positional tables with
   no check (a missing row is a null name and a crash), the rest switches; the solid branches of the planet function repeat one
   skeleton sixteen times. Thirty-eight kinds by hand would be over a thousand edits and as many ways to forget one. P-00 turns
   all of it into a row per kind, proven by changing nothing (the F series' discipline: the regress and `screens` identical),
   so every later milestone writes rows and ground, not plumbing.
2. **The kinds before the traits.** A trait declares what it needs and a kind what it is (P-00's tags), so the thirty traits
   are written once over all fifty-nine kinds; written before, they would be declared over twenty-one and revisited six times.
3. **The kinds in families**, each a milestone of five or six that share a family of ground (P-01 on the molten, metal, cratered
   and desert branches; P-02 on the felisian layer; P-05 and P-06 on the ice), the generation change in the first so the
   regress is re-blessed once a milestone and the pinned sites checked as the kinds arrive.
4. **The drifters last.** They are any kind that can be dark, so they come when every kind exists; their engine (a system with
   no sun, travel to a point that is not a star) is additive and touches no existing system, so a problem there changes no
   other milestone; D-05 needs P-04's dead worlds and every kind of record H-04 left. They are the release's last act: the
   explorer has learnt every kind of world, then hears something with no star.
5. **Could go earlier:** D-01 to D-03 need nothing from P or Q but P-00. If the user wants to hear the drifters sooner, they can
   follow P-00 (the rogue table then grows as the kinds arrive).

## Decided by default (2026-10-21)

The plan's defaults stand unless the user changes them; a milestone starts on "proceed" without asking them again (a new design
question that is the user's is still asked before building):

1. **One release:** all sixteen are 1.4.0, in the order above.
2. **The names are descriptive** (NITROGEN, CRYOVOLCANIC, GROOVED ICE, TWO-FACED, TIGER-STRIPED, PATCHWORK, SPONGE, BRINE DWARF,
   FROST DWARF), not our solar system's (PLUTONIAN, TRITONIAN, GANYMEDEAN, IAPETAN, ENCELADAN, MIRANDAN, HYPERIONIC, CERESIAN),
   though VENUSIAN and EUROPAN keep theirs; the sentence on the data sheet says what the world is like.
3. **About one world in three changes kind**, the rest stay; worlds with a people and the home world never change (16).
4. **Peoples** live on felisian and desert worlds as before and on the two dead felisian kinds (P-04); the new living kinds hold
   none in 1.4.0 (their own peoples would move every people's salt: a later iteration's).
5. **The drifters' rarity:** none at x1 (x1 stays C-07's list by rule), the first from home at the middle gains (x11-x18), six
   to ten within reach at x32; one in four of the frozen felisian and desert drifters kept a people.
6. **The drifters stay in place** (they drift at tens of km/s, nothing on the scale of the game's light years).
7. **The explorer's lamp** (D-04) is a helmet lamp on a key, for every night side too; the capsule and the buggy keep theirs.
8. **The drifters' peoples of the radio ages left a beacon** (D-05): their last broadcast under the drifter's own voice; their
   charts mark the star that was their sun, and Enter flies there (the star's own system is not changed by it).

## Phase 1: groundwork

Source: the reading of 2026-10-21 (every place a kind is wired; the worked example `PT_EUROPAN` on 34 lines in seven files,
`PT_DESERT` on 74 in twenty-five), `docs/reference/04-galaxy-and-systems.md`, `05-planet-function.md`, `09-cookbook.md`.

### P-00: the kind table

What a kind is today, and where: the enum and `PLANET_TYPES` (`system.*`), its zone weights and its rules under eleven star
classes in `pickPlanetType`, `typeColor`, the density copied five times in `generate`, the twin, glassable and inner-moon lists,
`shortType` (`game/ui.cpp`, ten characters), `traitEligible` (a switch of 33 traits over 22 kinds), `familyRep`,
`materialPalette`, `BodyGen::make`'s parameters, the relief, the cliffs and the plains, its `sampleSurface` branch, `typeSky`,
`describeBody`, `lookFor`'s fog, sky and clouds, `computeEnvironment`'s rain, wind, temperature and pressure, the events, the
boulders, the far floor's bank, the landmarks' crater grids and flat-type list, the drainage's dry or wet rule, the fronts, the
sounds, the harness's sweeps, scenes and site finders. Groups written out by hand in several places: "has a sea" (four),
"a people's world" (about twenty), "a dust world" (six), "a flat type", "crater sights".

* **A row per kind** (`KindDef`): its identity (name, short name, the data sheet's line, the sentence's parts, the radius
  range, the density, the albedo, the moons, the family whose code paths it shares where it has none of its own); its tags
  (opaque deck, lava world, plates, dry or wet drainage, fronts or dust fronts, a sea, flora, fauna, a people's world, twin,
  glassable, inner moon only, the sights it gives, giant); its generation (the five zone weights, the moon table's factors,
  its rules under the star classes as tags: the dead stars, the protostar, the Wolf-Rayet wind, the giants, the varieties),
  its colour (a base and its variation, drawn in today's order); its body (`make`'s ranges in the draw's order, the relief,
  cliffs and plains, the palette's overrides as material-colour-mix entries, the family representatives); its look and
  environment (zenith, horizon and night colours, fog, sky density, cloud layer, pressure, temperature offset and swing, wind,
  the rain rule, the dust threshold, the aurora's colours, boulders, the far floor's family); its liquid; its events (the five
  systems there are: eruptions, strikes, quakes, vent fields, devils, with parameters); its sounds (the wind, the rumble). The
  group predicates (`hasOpaqueDeck`, `isLavaWorld`, `typeHasDrainage`, `worldHasFronts`, `sightsWorld`, `isProbeGiant`) and
  the hand-written groups read the tags. One code path: the rows replace the switches, they do not sit beside them.
* **Traits by tags.** A trait declares what a world must be (dry, wet, cold, icy, airless, volcanic, living, crystalline...),
  a kind what it is; the 33 x 22 switch becomes a check of tags (67 traits over 59 kinds after Q and D, a switch of 4,000
  cases otherwise). The landform-or-world split stops being positional (`t <= TR_GEYSERS`): each trait carries its kind.
* **The planet function's skeleton.** The base, the relief spectrum, the shaping, the landforms and the drainage that sixteen
  branches repeat (the landforms block verbatim) become one helper, with archetypes for the families of ground (cratered
  airless, cracked ice, lava, eroded with air, wet, crystal, metal and carbon) set by the row's recipe and resolved once in
  `BodyGen::make` (no interpretation per sample: the bench's budgets hold); the branches that are their own (felisian, ocean,
  the giants, europan, tectonic, desert) stay as they are. The landmarks' crater grids read the recipe instead of copying
  `sampleSurface`.
* **Accent families.** Every material of a family shares one hue (six families, one bank each, all 32 banks of the ground
  taken): a new ground can only recolour its family, and today's tectonic sulphur is grey on the ground and yellow on the
  landing map. `matFamily` takes the kind: a world without flora lends its grass and forest banks (10 and 3) to two accent
  hues of its own (the mats, crusts, brines, salts and bands of P and Q), the same on the globe, the map and the ground.
* **Checks.** `static_assert` on every positional table (`PLANET_TYPES` or its successor, `shortType`, `TRAIT_NAMES`,
  `MATERIAL_NAMES`, the meso variants, the harness's `matCol` and `O5_COST`); the palette's unit check that would fail past the
  24th kind (`0.3 + 0.03 ty`); the statistics page's walked kinds (48 characters hold a handful: a count and a page); the
  cookbook's recipe for a kind (`09-cookbook.md`; `shortType` lives in `ui.cpp`).
* **The harness iterates the rows:** `vesperis_test kinds [family]` (a sheet per kind: the globe, the map, a standing and an
  aerial frame, its event staged), `survey`'s shares by kind, the scenes and site finders from the row.
* Done when: nothing generated or drawn changes: `regress` 24/24 unchanged, `screens` identical between the old binary and the
  new (`pngdiff`), `survey 1500` the same numbers, `unit` passing, `bench check` within every budget; `kinds` writes the 22
  sheets; the cookbook's recipe is a row and a ground function.
* **Done 2026-10-21.** `galaxy/kinds.*` (the rows built at compile time and checked, 28 tags for the code paths, 24 processes for the
  traits, the star-class rules as weight ops, the crater stacks, the look, the air and the events), `sampleSurface` a dispatch to
  eighteen ground functions sharing `GroundCtx::relief`, the accents' mechanism (no kind uses it yet), the walked kinds' line and page,
  `vesperis_test kinds` (21 sheets: the companion star is a sun), the cookbook's recipe, the static checks. As built against the plan:
  the archetypes are the old branches as ground functions (merging them would have moved bits; a new kind gives a ground its own
  parameters by its draws), and the scenes keep their table (the rows give the `kinds` sheet its sites through `eventMaterial`). The
  proof (`docs/decisions/log/`): a fingerprint of 233,347 bodies, their made generators and 782,922 samples the same; `regress` 24/24 on the
  workers and one thread; `survey 1500` the same; `screens` 96/96; the kinds' sweeps 187/187; `unit` passing; `bench check` within
  all 37 budgets, and against 1.3.2's binary in alternating pairs every row within the noise or faster (the globe reads its kind's
  tags once a globe: read at every pixel, the first build's space rows were 3-5% slower).

## Phase 2: the kinds of world

Source: `IDEAS-worlds-and-wanderers.md` section 1 (every kind: what it is, where, the ground, the sky, the event, the source),
`docs/reference/04-galaxy-and-systems.md` (the zones and the star classes' rules), `05-planet-function.md`, `06-surface.md`.
Every kind gets: its row; its ground (an archetype with its recipe, or a function of its own for what no archetype has); its
palette and accent hues; its sky, temperature, pressure, weather and sounds; its sentence and data sheet line (names unique and
safe as file names: the harness uses them as arguments); its traits' tags; its scene and site finder; its event where it has
one; its sheet in `kinds`; its share in `survey`.

### P-01: the furnace worlds (generation 16)

* **The kinds come from the old draws.** `kindOf` in the planet loop, after `pickPlanetType` and before the radius: a world
  becomes a new kind by a hash of its own (a salt per kind) under the kind's rules (the temperature, the star, the size, a moon
  or not and whose, locked or not). Every draw of the system's stream is made as the old kind made it, so the stream, the
  orbits, the moons and the rings hold: the radius from the new kind's range on the same `sizeRand`, `typeColor` called for the
  old kind and the colour then set from a hash, the rings drawn as the old kind's and the kind's own rule by a hash, the system's
  rules (the spacing after a giant, the twin, the glassing, the inner moons) reading the old draw, the moons drawn as the old
  kind's in the same number (so every body keeps its index: the guide's names, marks and shards are keyed by it), their orbits
  from the new radius; where a kind cannot avoid a draw of its own, `survey` counts the systems it moved. Never on a world with a people
  (C-01's hash is read first, so every people, town, record and signal stays); the home world stays felisian (the salts pinned
  to it, as the radar's are).
* **Magma ocean, chthonian, glass, steam, salt, rust** (section 1): the magma sea at the reference level round the substellar
  point and its crust rafts; the chthonian's heavy smooth ground and its ring; the glass sheets (`MAT_GLASS`) and glass dunes;
  the steam world's deck, highlands and the fog thickening downhill (the lowlands' supercritical "sea"); the salt world's
  evaporite rings, brine pools in accent hues, pillars and salt glaciers; the rust world's red ground and dry valleys. Events:
  fire fountains on the magma shore, steam bursts.
* Done when: `kinds furnace` writes the six sheets; the scenes `magma_shore`, `chthonian_plain`, `glass_flow`, `steam_highland`,
  `salt_basin`, `rust_valley`; `survey 1500` gives each kind its share near home and says how many worlds changed kind; unit
  checks (the draw deterministic, no world with a people changed, home felisian, the radar's x1 list unchanged); `GEN_VERSION`
  16 in `docs/decisions/log/`; the regress re-blessed, the pinned sites and the age worlds checked and re-pinned where they moved;
  `bench check` within budgets; the flow passes.
* **Done 2026-10-21.** The six made kinds (`kindOf`, the rows' sources; `makeKind`), five grounds of their own and the rust world on the
  desert's, three materials (mud, potash, brine), the accents' first users and the tectonic sulphur's own yellow, the steam world's
  lowland fog and bursts on its sinter (`steamVents`), the magma ocean's fountains, its night side and its tilt taken with its lock;
  `kinds furnace` (six sheets), the six scenes, the survey's made kinds, three unit checks; `GEN_VERSION` 16. As built against the
  plan: the sources' temperatures are the game's (its inner orbits reach 350-500 K: a magma ocean from a locked molten world over
  360 K, a chthonian or a glass world over 400 K), a made planet's moons keep their drawn orbits and periods (moved out only where the
  made world would reach them: a people's moon keeps its days), the steam world comes from worlds whose day is not their year and the
  chthonian from living stars only, the glass world has fields of glass beads and no dunes (no air). The proof (`docs/decisions/log/6-1.4.0.md`): every draw of 6,866
  systems the same as 1.3.2's (65,463 bodies), 1,086 made, seven moons moved clear, none of 917 people's worlds changed; `regress` 24/24
  unchanged both ways (none of its worlds is made: nothing to bless); `screens` 96/96; the kinds' sweeps 185 of P-00's 187 the same (the
  tectonic ground's sulphur; the binary world's globe by the sweep's own order, the made kinds drawn before it); the pinned test worlds
  and the age worlds as they were; `unit` 349 checks, none failing; the flow passes; `bench check` within all 37 budgets, and against
  P-00's binary in alternating pairs every row within the noise (no pixel loop changed; the Mac on battery).

### P-02: the living worlds

* **Archean, hothouse, superhabitable, steppe, tundra, lichen.** The flora and the fauna by tags (about fifteen felisian gates
  today), the climate's offsets (hothouse warmer and ice-free, superhabitable warmer with a thicker air and shallow shelves,
  tundra colder with ice sheets to the middle latitudes, steppe dry), new materials in the accent hues (microbial mats,
  stromatolites, lichen crusts, moss), the archean's green sea and orange haze, the bestiary by kind (none on the archean or the
  lichen world; cold grazers on the tundra).
* Done when: the six sheets; scenes `archean_flats`, `hothouse_swamp`, `superhabitable_shelf`, `steppe_plain`, `tundra_ice`,
  `lichen_hollow`; the flora's coverage per kind printed; `survey` shares; unit; regress; bench.
* **Done 2026-10-21.** Six made kinds: five from the felisian worlds whose day is not their year round the stars still burning their
  hydrogen (the locked ones are P-03's, a giant star's P-04's), the lichen world from the thin airs; the archean by its star's hash (a
  young star's living worlds alike: `byStar`). Five on the felisian layer by their rows (the climate's contrast, the seas' depth, the
  grass band, the day's swing, the islands, the continents' scale, the flora's silhouette, a favoured trait, the animals' style) and two
  tags for what the layer had not got (the archean's microbes: mats, reef flats, stromatolite domes; the tundra's ice sheets: a dome
  with a terminus cliff, patterned ground); the lichen world's ground its own on the thin air's. Four materials (mats, stromatolite,
  crust, moss), the snowball's own process (`KS_FREEZE`), cold grazers and the steppe's herds. `kinds living` and the land cover by the
  map in every living sheet (the flora's coverage), the six scenes, the survey's shares, two unit checks. As built against the plan: the
  sources' bands on the game's scale (the hothouse over 285 K, the steppe over 262, the tundra 212-248, the superhabitable an orange
  dwarf's 230-330), the steppe's salt pinned to keep the home world felisian (Heilya III is a tundra world), the archean's mats on the
  wetter half of the land a felisian world would grow its plants on (the flats alone were 1%, invisible from orbit) and its domes dark
  with mats on a pale flat, the superhabitable's land broken into archipelagos by a smaller continent field and its seas a tenth as
  deep, the tundra's sheet ending in a cliff. The proof (`docs/decisions/log/6-1.4.0.md`): every draw of 6,866 systems the same as P-01's, 1,765 made, two
  moons moved, none of 917 people's worlds changed; 43,217 of 44,479 bodies bit for bit P-01's in every field and sample (the rest made or
  the two moons); `regress` 24/24 unchanged both ways (nothing to bless); `screens` 96/96; the kinds' sweeps 219 of 235 the same and all
  235 with the new kinds skipped (the scan's order); `unit` 351 checks, none failing; `survey 1500` 3.7% made; the flow passes; `bench
  check` within all 37 budgets and, once the stromatolites' pass ran on a microbial world alone, level with P-01's binary in pairs.

### P-03: the locked and the deep

* **Eyeball, terminator, hycean, ammonia, snowball, chemotroph.** The locked kinds on M5-08's lock (the climate by the
  substellar point): the eyeball's floe line round the open sea under the sun instead of round the poles; the terminator's day
  desert, night ice and the drainage of meltwater running sunward into the ring of lakes and life; the hycean sea under its
  hydrogen haze; ammonia water as a liquid (its colour, its slabs of ice); the snowball's sea glaciers, nunataks and equatorial
  belt; the chemotroph's vents, pale mats and glowing spores at night.
* Done when: the six sheets; scenes `eyeball_shore`, `terminator_ring`, `hycean_sea`, `ammonia_shore`, `snowball_belt`,
  `chemotroph_vents`; `survey` (the red dwarfs' locked worlds by kind); unit; regress; bench.
* **Done 2026-10-21.** Six made kinds: the eyeball from a red dwarf's locked oceans, felisian and icy planets (its pupil open within its
  reach of the fixed sun, floes over its last degrees, the sea glacier past a cliff at the ice front, the temperature the sun's height);
  the terminator world from any star's locked felisian and desert planets (a drier air, the day side a desert whose seas boiled to salt, the
  night side under an ice sheet edged by the climate, a ring of rivers, forest and grass between, a sunset that never ends); the hycean from
  the warm oceans (a deep teal sea under a cream haze, no floes, no night ice); the ammonia world from the cold oceans, methane and icy worlds
  (the methane world's hills under a yellow-brown haze, a sea at its level that never freezes, slabs in its shallows, frost on its hills); the
  snowball from the coldest felisian and ocean worlds whose day is not their year (frozen to a belt of open sea, a sheet smooth over the
  continents with the ranges standing out as nunataks, sea glaciers with a cliff and floes before it, blizzards); the chemotroph from the
  tectonic and rocky worlds of the dead stars and the brown dwarfs' moons (hot springs on dark frosted rock: blue pools on sinter mounds,
  pale mats, sinter chimneys, steam, glowing spores at night). Two tags (`KT_FIXED_SUN`: the climate round the substellar point and its
  steady wind; `KT_VENT_LIFE`), one event (`EV_SPRINGS`), three grounds (`eyeball`, `ammonia`, `chemotroph`), the felisian layer's new row
  fields, a moon's source by its planet's kind (`parents`), no new material. As built against the plan: the eyeball's front stands over the
  open water (the plan's floe line round the pupil was ten degrees wide: no sea seen from the ice); the terminator world's air drier and its
  water little, so its ring lies at the terminator and on land; a locked world made one of the locked kinds that draws a twin is made again
  as one that turns (the twin takes its lock); the companion's clearance and the comets read the moons as drawn (a made world pushed a moon
  and moved a companion 8,000 km in the first stream); a black hole's worlds left out (S-06 keeps them airless); three salts pinned so the
  regress's and the bench's worlds stay as drawn; and a bug of P-02's fixed (the tundra's blue ice over its buried cliffs). The proof
  (`docs/decisions/log/6-1.4.0.md`): every draw of 6,866 systems P-02's, 1,867 made, 41 moons moved, no people's world and no test world changed; 43,131 of
  44,467 bodies bit for bit P-02's (the rest made, the moved moons, ten tundra worlds); `regress` 24/24 unchanged both ways (nothing to
  bless); `screens` 96/96; the sweeps P-02's but for the first hydrocarbon world (made an ammonia world), two trait sheets and the scan's
  order; `unit` 354 checks, none failing; `survey 1500` 7.0% made with the red dwarfs' locked worlds by kind; the flow passes; `bench check`
  within all 37 budgets, and against P-02's binary in eight pairs on the battery every row within the noise but the sector map at 4x (slower
  in P-03's runs in all eight; level when the scene was timed alone and in a second instrumented round: not found, to be timed on the mains).

### P-04: the dead worlds and their peoples

Every reader of a people's world reads the world as it is now (the reading of 2026-10-21). On a dead world that is the
wrong world: the plan scores ice ground 0, so a frozen world gets no capital and no places; a frozen sea is `MAT_ICE` at the
sea's level and every wet test of the placement (`wetSample`, `peopleDivide`, `readSite`, `ruinSiteOk`, the works, the roads)
takes it for land; the records' `cold`, the instruments' ice and the climate read the present. So a dead world has two: the
world in its people's time (then) and the world the explorer walks (now).

* **The then-world.** A generator of the world as it was (B-419's `unseasoned` pattern: the same seed, the living kind, the
  climate it had): the plan, the extent and the land share, the culture, the instruments and every placement read it; the
  drawing reads the now. The heights are the then-world's in both (the felisian glacial valleys and fjords follow the climate,
  so a now-climate would float or sink every town): the dead world is its living ground under a cover.
* **Frozen felisian** (the drifters, D; the wanderers, P-05): the felisian ground under the now: the sea frozen at its level
  (flagged as the old sea, C-01's `oldSea`, so every wet test sees the sea it was and the harbours face the ice), rivers and
  lakes frozen, the forests standing dead under frost (the dead forest's skeletons, white), snow drifts, much of the air frozen
  out toward the poles, a black-blue sky. A wanderer keeps no people: the lost sun is the drifters' story alone (D-05).
* **Scorched felisian** (red giants, orange giants, carbon stars: the felisian share they lose today becomes scorched; white
  dwarfs: the same world after the giant phase, cold, with frost on the salt): the felisian ground under C-01's dead seas
  (`oldSeaM`: the bed, the benches, salt in the deeps, pale reef limestone along the old shelves), petrified forests, cracked clay,
  dust, a vast red sun. Its people by a hash of its own, one world in eight (C-01's own hash never fires on a converted world:
  `kindOf` leaves the worlds it marks felisian), with the desert's world factors (the drying x6, the climate x3).
* **The people's world as a tag** read by the twenty hard-coded felisian-or-desert lists (the signals, the charts, the radar's
  readout, auto-explore, the descriptions, the harness's filters).
* **Ground of snow and ice for a people.** Roads, streets and fields drawn on snow (not today); the works' snow exclusions
  and the coast rules read the then-world; drifts of snow in the snow bank where a desert's are sand; the culture's `wet` low
  (the cold keeps what it holds).
* **On the air.** A scorched people of a radio age transmits as a desert's does (the transmitters' list reads the tag); the
  salt re-pinned so home's x1 list holds and each step still adds one to three peoples.
* Done when: the two sheets; scenes `frozen_felisian_shore` (on a wanderer the harness finds) and `scorched_felisian_seabed`;
  `peoples ages` lists the scorched peoples (a test world of each age near home pinned); a town and its fifty records printed on
  a scorched world; unit (no 1.3.2 people moved; the radar's pins hold or are re-pinned; at every town of a test world the
  then-world's height equals the now's; a frozen sea is water to the placement); regress; bench.
* **Done 2026-10-21.** Two dead kinds on the felisian layer under a cover of the present's (`KindDef::cover` at the end of `felisianSample`), their
  rows the felisian's draws, relief, greenhouse and tints (`thenGround`), so the ground is the same in both times; `thenWorld` (B-419's
  `unseasoned` widened) is the world in its people's time, and every reader of a people's world reads it (the plan, the extent, the histories,
  the settlements, the roads, the works, the records' sites, the instruments), the drainage's tiles its own for both times; what a culture
  built is its time's, how its ruins weathered the present's (`nowKind`). The scorched felisian from a giant's felisian worlds and a white
  dwarf's rocky and desert planets (dry seabeds of salt, silt and reef limestone kilometres under the old shores, cracked clay, dust, stone
  forests, frost where the present is cold); its people its own, one in eight, ending as a desert's. The frozen felisian (seas ice at their
  level, still water to the placement, frost, white dead forests, a black sky). A system's signal and charts speak for its C-01 people first
  (`systemPeoplesWorld`). On the ground the dead forests (frozen skeletons, stone stumps and trunks), roads, streets and fields under a frozen
  world's frost, snow drifts at its ruins, reservoirs dry or frozen. `kinds dead`, four scenes, `peoples ages`' scorched test worlds
  (`SCORCHED_WORLDS`), `peoples scorched` (a town and its fifty records), the survey's dead worlds, three unit checks. As built against the
  plan: the frozen felisian is made in no system yet (P-05's wanderers and D-01's drifters will make it): the harness stages it on a felisian
  world for its sheet, its scenes and the unit, its harbour scene on a staged world with a people (a drifter's, D-05's, rather than a
  wanderer's: a wanderer keeps none); the white dwarf's sources are its planets, not their moons (a 1,300 km felisian world); the scorched
  peoples' salt held the radar's lists as they were (no re-pin); the scorched test worlds lie 40-200 ly out on the plane (none within 25 ly of
  home); two more scenes, the dead forests. The proof (`docs/decisions/log/6-1.4.0.md`): every draw of 6,866 systems P-03's, 166 scorched, no moon moved, no people's world and no test world
  changed; 44,368 of 44,467 bodies bit for bit P-03's (the other 99 the scorched); `regress` 24/24 unchanged both ways (nothing to bless);
  `screens` 96/96; the sweeps P-03's but for the scan's order (330 of 331 with no scorched world made, the one a frame that flips from run to
  run); `unit` 357 checks, none failing; `survey 1500` 4 scorched of 109 made (7.2%); the flow passes; `bench check` within all 37 budgets,
  and against P-03's binary in eight pairs on the battery every row within the noise but two that follow the launch path, not the code:
  from the perf-baselines path P-04's probe at 2x read 5% slower and P-03's sector map at 4x 16% slower than the other's, from paths of
  `./vesperis_test`'s length both level (P-03's unexplained sector map row was this).

### P-05: the far cold

* **Nitrogen, cryovolcanic, dry ice, wanderer, brine dwarf, frost dwarf.** The nitrogen plains' convection cells and the
  ice blocks floating in them, the glaciers, the bladed ground and the layered blue haze at the limb; the cryovolcanic moons'
  dimpled ground and their plumes (a column, then a dark flag drifting with the wind, the streaks laid downwind); the dry ice
  caps' pits, spiders and fans, and the jets in the local spring; the wanderer, an extra body appended after the system's own
  (its own hash, so nothing else moves) on a far, eccentric, steep or retrograde orbit, its fallen air as snow; the brine
  dwarf's faculae and lone dome; the frost dwarf's fresh bright coat.
* Done when: the six sheets; scenes `nitrogen_cells`, `cryovolcanic_plume`, `dry_ice_spiders`, `wanderer_snow`, `brine_faculae`,
  `frost_dwarf`; the wanderers' share of systems in `survey`; unit; regress; bench.
* **Done 2026-10-22.** Six kinds of the cold orbits, each a ground of its own on the solid skeleton: Pluto's nitrogen world (a great plain of
  nitrogen ice in a hashed basin, its cells and dark troughs, pits, blocks of water ice along its shores, glaciers down its shores' valleys;
  dark red tholin at the low latitudes, blades of methane ice; a black sky with the haze's blue layers on the horizon), Triton's cryovolcanic
  world (the cantaloupe ground, a pink cap, dark plumes 6-9 km high whose flags stream 40-90 km downwind over the streaks they lay), Mars'
  south cap on a colder world (caps of dry ice pitted like cheese, the spider band, the seasonal frost with its dark fans, the jets of the local
  spring), the wanderer (a rogue world's old ground under its air fallen as snow, deep in the lowlands, bare highlands, domes of new ice),
  Ceres's brine dwarf (dark craters, faculae on the young ones' floors, one lone mountain of ice streaked with salt) and Eris's frost dwarf (the
  brightest frost, pits, tholin streaks drawn out east and west). Made from the icy, cratered, europan and thin-air worlds by their
  temperature, the cryovolcanic from a giant's captured moons first (`makeKind` runs on them now, as `KB_CAPTURED`, which no earlier kind
  names), no air round a black hole or in a Wolf-Rayet wind. The wanderers: one system in twenty-seven caught a rogue world, an extra last
  body from a stream of its own on a far, eccentric, steep or retrograde orbit, four in five the wanderer kind, one in five a frozen felisian
  (P-04's kind's first worlds), none with a people. As built against the plan: the dry ice's salt pinned for the regress's and home's cold
  worlds; a seventh scene (`brine_mountain`, the lone mountain); the frozen felisian's sheet and forest scene on a real wanderer now (its
  harbour still a staged world's: a wanderer keeps no people); the nitrogen and frost dwarf scenes under a sun-like star's light. The proof
  (`docs/decisions/log/6-1.4.0.md`): every draw of 6,866 systems P-04's, 1,529 bodies made far cold, 251 wanderers in 251 systems, 7 moons of made planets
  moved, no test system's body changed; 43,423 of 44,467 bodies bit for bit P-04's (the rest made, or the 7 moons); `regress` 24/24 unchanged
  both ways; `screens` 96/96; the sweeps P-04's but for the pictures drawn after a new kind's (339 of 339 with no far cold body made); `unit`
  361 checks, none failing; `survey 1500` 9.7% made, the wanderers 3.7% of the start region's systems; the flow passes; `bench check` within
  all 37 budgets.

### P-06: the strange moons

* **Grooved ice, two-faced, tiger-striped, patchwork, sponge.** The grooved lanes between dark polygons; the two faces by the
  leading hemisphere and the equatorial ridge; the tiger stripes and their plume curtains (R-307's geysers in lines); the
  patchwork's blocks, coronae and the cliff twenty kilometres high; the sponge's packed deep pits.
* Done when: the five sheets; scenes; `survey` (moons of giants by kind); unit; regress; bench.
* **Done 2026-10-22.** Five kinds of moon, each a ground of its own on the solid skeleton: Ganymede's grooved ice (dark old polygons, cratered, the
  arcs of ancient impacts' furrows on them, between them lanes of bright ice a few hundred metres lower filled with tilted blocks 3-8 km apart
  and 300-700 m high along them, a finer set, swaths at angles of their own; frost toward the poles), Iapetus's two-faced moon (the face it
  leads with coal-black to fifty degrees of latitude and a quarter turn and more round the equator, its edge ragged; along the edge the slopes
  that face the equator dark and those that face the poles bright; a ridge 8-15 km high along its equator, lone peaks beyond), Enceladus's
  tiger-striped moon (the whitest ground, old cratered and young ridged plains, the south polar terrain behind its folds, four stripes 500 m
  deep with blue ice, curtains of ice rising 25-60 km from them and their jets one by one near the explorer), Miranda's patchwork (blocks of
  unlike old ground, up to three coronae of banded ridges, two chasms, the great cliff of 15-20 km along a corona's edge) and Hyperion's sponge
  (pits packed edge to edge at five scales, kilometres deep, dark dust in their floors). Made from moons by what they are among their planet's
  moons (`MoonFact`, read from the draws as drawn: inner, outer, facing, turning, resonant, large, small): a giant's large icy moons grooved,
  its outer icy moons that face it two-faced, its inner icy and europan moons tiger-striped, cold icy and rocky moons in a resonance
  patchwork, small cold cratered and icy moons that tumble sponges. As built against the plan: a sixth scene (`two_faced_ridge`, from the
  bright ice: on the coal-black face its relief hardly shows); the sponge's salt pinned for the regress's Wyariothmai V-b; the patchwork and the
  sponge under 200 K (a rocky moon of 481 K would have been one of ice); the two-faced moon alone holds its apex (`Body::apexDir`, from its
  orbit's elements: four positions a moon by differences had doubled the generator's time); the sponge's pits on a grid's features projected
  onto the sphere (a `worley3` cut through it left a pit's middle on a fiftieth of the ground). The proof (`docs/decisions/log/6-1.4.0.md`): every draw of
  6,866 systems P-05's, 1,232 moons made strange (342 grooved, 242 two-faced, 255 sponges, 217 tiger-striped, 176 patchwork), no other body
  changed, no test system's body changed; 43,785 of 44,632 bodies bit for bit P-05's (the other 847 the made moons); `regress` 24/24 unchanged
  both ways; `screens` 96/96; the sweeps P-05's but for the pictures drawn after a new kind's (394 of 395 with no strange moon made, the one
  the chemotroph's run-to-run flip); `unit` 365 checks, none failing; `survey 1500` 11.9% made, the strange moons 7% of the giants' moons of
  the start region; the flow passes; `bench check` within all 37 budgets (its third run: the first two each one row over by a spike the other
  runs did not share, the machine in use on the battery), level with P-05's binary in six pairs.

### P-07: the giants

* **Water-cloud, azure, alkali, silicate, gas dwarf, super-puff.** Giants in the warm and hot zones (a small share: a hot
  Jupiter round a sun-like star in a hundred, with few moons), the globe's looks (the white of class II, the clear blue of class
  III, the dark of class IV with its night side dull red, class V glowing with iron rain at the terminator, the pale hazy gas
  dwarf, the super-puff's vast soft limb), the probe's decks by class (X-03's condensation table gains the salts, the silicates
  and iron; class III clear to the deep), `giantCharacterOf` by kind, the descriptions.
* Done when: the six sheets; a probe descent through each class (`probe giants`); `survey` (the giants by class against the
  temperature); unit; regress; bench.
* **Done 2026-10-22.** Six kinds of giant, each its class's globe, bands and air: the water-cloud giant (Sudarsky's class II, too warm for
  ammonia: a bright white globe of soft bands and many storms, its water deck on top and the salts deep under it), the azure giant (class III,
  too warm for any cloud: a deep blue globe with faint bands, its air clear and blue to the deep), the alkali giant (class IV, a hot Jupiter:
  dark, its sodium and potassium taking the light high in its air, its night side glowing dull red, decks of rock and iron deep under it), the
  silicate giant (class V, the hottest: clouds of rock high over clouds of iron, glowing, its night side brighter toward the evening
  terminator and streaked dark toward the morning where its iron rains out), the gas dwarf (a sub-Neptune: a small world under a deep hazy
  envelope, a pale globe without bands) and the super-puff (Kepler-51's: a young giant of almost no weight, its haze spread far over it by its
  weak pull, a wide soft limb). Made from a drawn gas giant by its warmth (water-cloud over 165 K, azure over 290) or youth (the super-puff,
  first in `kindOf`'s order: a protostar's, a blue star's or every giant of a young star by the star's hash), and the giants of the warm zones
  from the worlds of those orbits (their moons the drawn world's: few): a temperate water-cloud giant, the warm azure ones, the hot Jupiters
  alkali and silicate only from worlds locked to their star, round a yellow star about one in a hundred, and the gas dwarfs. As built against
  the plan: the classes' bands on the game's scale (its inner orbits reach 350-500 K, as P-01's furnace worlds: water-cloud 165-290 K, azure
  290-360, alkali 360-420, silicate over 420), the probe's one-bar temperatures the real classes' across them (600-1,000 K, 1,100-1,500,
  1,800-2,600); a double planet's half never a giant, and a made giant never moving a people's moon (both made again by their facts with no
  giant, the moons drawn again with the same draws: two giants over the stream's systems); the water-cloud's salt pinned for B-427's Wolf-Rayet
  giant (the user's, Syaniapheela II) and the probe's Thilawiane III, the gas dwarf's for the probe's Wailie II; the X-03 ice giant a drawn gas
  giant's only; no warm stratosphere over an alkali giant (Fortney et al. 2008: a probe met air hotter than where the heat then ended it) and an
  inversion over a silicate giant's; an azure giant's deep blue by Rayleigh's scattering and its camera set for the air round it (the salts'
  haze far under the probe had shown as a white floor); a giant's sheet shows its night side, its limb and the air a probe finds; seen from
  its moon a hot giant's night side stays dark (left out). The proof (`docs/decisions/log/6-1.4.0.md`): every draw of 6,866 systems P-06's, 1,037 bodies
  made giants (587 water-cloud, 278 gas dwarfs, 67 super-puffs, 66 azure, 28 alkali, 11 silicate), 341 moons of made giants moved (none a
  people's), no other body changed, no test system's body changed; 43,685 of 44,632 bodies bit for bit P-06's (697 made, 250 the moved
  moons); `regress` 24/24 unchanged both ways; `screens` 96/96; the sweeps P-06's but for the pictures drawn after a new kind's (434 of 435 with no giant made, the one the chemotroph's
  run-to-run flip); `unit` 369 checks, none failing; `survey 1500` 13.0% made, a hot
  giant round 6 of the start region's 670 yellow stars (0.9%); the flow passes; `bench check` within all 37 budgets, level with P-06's binary
  in six pairs (the generator too, once its pick and `kindOf` stopped walking every kind's row: the first build had cost it 15%).

## Phase 3: the qualities

Source: `IDEAS-worlds-and-wanderers.md` section 2, R-304 and R-305 (the traits and the landform library), `05-planet-function.md`.

### Q-01: six slots, the facts and eleven landforms

* **Six slots.** `BodyGen::traits[3]` becomes six: the three drawn today stay as they are (the same hashes), three new slots
  are drawn by hashes of their own, each from a different kind of trait (landform, water, sky, life, ground, sound) that fits
  the world (a fifth of the worlds get none of the new, most one or two, a few three); the exclusions (one colour, pangaea not
  with archipelago, giant flora not with dead forest) cover all six; `landformsAt`, `traitList`, the sentence and the drainage's
  cache key read six.
* **The facts** (no draw): sideways spin, retrograde spin, racing day, locked face, eccentric year, strong field, no field,
  captured, twin, ringed. The data sheet: `TRAITS` on two lines, `FACTS` on one.
* **Eleven landforms** in the landform library: great shield, tesserae, labyrinth, hollows, crater chains, penitentes,
  columnar basalt, mud volcanoes, inverted rivers, pingos, maars; each with its mark for the galleries and its sight for the
  scanner (O6-06's rule).
* **The karst found broken** (the reading of 2026-10-21): every branch but the felisian one passes a fixed moisture of 0.15
  to the landforms, under the karst's 0.45, so an acidic world with karst towers has the name and the sentence and no towers;
  the moisture comes from the world.
* Done when: `traits` writes a sheet per new landform (`shots/tests/trait_*.png`) and prints the distribution over six slots;
  the karst's towers on an acidic world; unit (the old three slots unchanged on 2,000 worlds); regress; bench.
* **Done 2026-10-22.** Six slots a world: R-304's three as drawn (all 61,460 landable bodies of the stream's systems, the unit's hash of the
  first 2,000), three new ones by hashes of their own, each from a kind of trait of its own that fits the world (`TraitKind`; in Q-01 the
  landforms alone, so a world takes one at most: 38,804 of the 61,460 do), never on a world with a people nor the home world (their ground as
  1.3.2's). Ten facts from the orbit and the spin (`bodyFacts`), the data sheet's TRAITS on two lines and FACTS on one. Eleven landforms
  (`landformsAt`), each with its processes, its mark and its sight for the scanner (eleven new sight kinds, the guide's file keeping R-408's
  numbers; the points by the features the landform builds, the fields by the cell's scan through the mark's trait); the drainage keeps a maar's
  depression as a lake. As built against the plan: the karst needed the world's warmth as well as its moisture (the landforms off the living
  ground read the thermometer's mean now, and the wet kinds their own mean moisture: a single wet field had taken the hydrocarbon worlds' dune
  seas away), so 120 bodies change by it alone (the wet worlds' dry landforms keep to their drier parts, the thin-aired worlds' glaciers and
  patterned ground follow their cold); the home world kept like a people's (it drew columnar basalt); RACING DAY on the game's scale (a day under
  half an hour); the facts ten, as listed; the data sheet over pages (a fifth of the sheets ran past the footer already, half would have); the
  penitentes each a pointed blade in rows; about half the maars crater lakes. The proof (`docs/decisions/log/6-1.4.0.md`): 18,085 of 44,632 bodies bit for
  bit P-07's, 26,427 with a new landform, 120 by the moisture and warmth; `regress` five maps of worlds that drew a landform blessed, then
  24/24 both ways; `screens` 93 of 96 (a home moon's globe in the decoder's window); the sweeps 386 of 447 the same (60 of worlds that drew a
  landform, the chemotroph's flip); `unit` 375 checks, none failing; the flow passes; `bench check` 36 of 37 (the photo's PNG at 11.1 of 11),
  level with P-07's in six pairs but for the landing map's zoom on a world that drew crater chains (+6%, the landform's cost).

### Q-02: water, sky, life and ground

* **Water:** great tides (W-02: the sea level following the moon and the sun by their masses and distances, the tidal flat,
  the tide line on the landing map and the sector map), hot springs (terraced pools in accent hues), pink lakes, milky seas (a
  glow on the sea at night, seen from orbit).
* **Sky:** noctilucent clouds, ice halos (22 degrees and sundogs round the sun and the moons), airglow, fog seas, great aurorae
  (R-402's field raised).
* **Life:** barrier reefs (from orbit too), fungal forests (the mushroom family, pale canopies), petrified forests, algal
  blooms.
* **Ground:** rainbow strata (O6-04's cliff bands alternating between the rock family and an accent hue, with steps of albedo:
  two hues and their shades, the banks hold no more), gypsum sands, meteorite fields (the scanner finds them).
* Done when: a sheet per trait; the tides' range against the moon's mass and distance printed for ten worlds and the capsule's
  shore moving over an hour (scene); unit; regress; bench.
* **Done 2026-10-22.** Sixteen traits in the new slots, a kind of trait each (`TraitKind`), each with a process of its own and three a fact of the
  world (`traitFits`: the tides a forcing, the aurorae a strong field, the reefs a warm sea); 14,852 of the stream's 61,460 landable bodies carry
  one, the new landforms' share falling (38,804 to 32,471); those that move the ground or take a bank never on a people's world or the home world,
  whose sky and sea may glow (Heilya II: noctilucent clouds, milky seas). The tides: a body's forcing (m / M) R^4 / d^3 of the pulls that turn
  over it, a derived fact (`Body::tideForcingM`); the coastal spring range its compression (5-16 m: the game's orbits are close); the sea's offset
  each pull's bulge less its daily mean (spring and neap, the high water every twelve minutes to fifteen hours by the bodies' own motions); the
  flats wet sand under the sea's plane, creeks; the drawing, the shore's cut, the explorer, the vehicles and auto-explore at the tide, the
  placements at the mean; the tide lines on the sector map and the landing map's zoom; `tides` lists ten worlds' pulls (masses, distances)
  against the range met, `tides scene` the capsule on its flat over an hour, dry by a metre then in half a metre of sea. The waters' colours one
  mechanism (`waterTint`, `lifeTint` through the vertices and the map): pink lakes, milky seas glowing at night (the ground, the globe's night
  side), algal blooms on two scales (regions from orbit, filaments 12 km apart), reef lagoons turquoise behind a crest awash at the shelf's
  edge. The sky: noctilucent clouds lit past sunset at high latitudes, the 22-degree ring with sundogs, the parhelic circle and the moon's ring,
  the green and red airglow, dawn fog seas filling the valleys, great aurorae down to the tropics. The life: fungal forests (the mushroom
  silhouette, pale caps and stems), petrified forests (stone trunks in jasper), the blooms, the reefs. The ground: rainbow strata (bands of the
  rock's colour and an accent), white gypsum dunes, meteorite fields (strewn ellipses, iron stones that glint, pits downrange, a scanner mode).
  As built against the plan: the meteorites the dry and frozen grounds' alone (on every airless rock they took a fifth of the scan region's
  worlds); the trait colours in the second people's banks (25-27) and the milky glow in bank 15; the tide lines drawn on the maps as the
  intertidal band's edges (the sector map's image is a still one). The proof (`docs/decisions/log/6-1.4.0.md`): the first three slots P-07's; every body whose
  six slots are Q-01's hashes as Q-01's (34,531 of 44,632; 10,101 took a Q-02 trait); `regress` three hashes of worlds that drew a trait blessed,
  then 24/24 both ways; `screens` 60 of 96 (the cabin's window at home: its milky seas); the sweeps 415 of 459 (the kinds' sweep worlds that took a
  trait, four landform sheets' worlds); `unit` 380 checks, none failing; the flow passes; `bench check` within all 37 budgets, the pairs with
  Q-01's binary too noisy to read (the machine loaded, on battery) and level at their bests but the telescope's at 2x, which the last check ran
  under Q-01's best (a cell wholly in a water's colour drawn in its bank alone since, pixel for pixel).

### Q-03: the sounds of the ground

* **Singing dunes** (a low hum with its harmonics when the sand of a slip face slides under the explorer or the buggy),
  **singing ice** (the pings and whoops of ice settling and cracking, dispersive chirps, on cold worlds with an air), **quakes**
  (the tectonic world's heave and rumble on the worlds that carry the trait). Synthesised in `game/audio.cpp`'s ambient bed,
  levels set from rendered WAVs as the radar's are.
* Done when: WAVs per sound (`shots/tests/sound_*.wav`) and their levels within the unit's bounds; scenes with the sound's cue;
  unit; regress.
* **Done 2026-10-22.** Three traits heard in the new slots (`TK_SOUND`), each with a process of its own, the dunes and the ice an air besides;
  7,409 of the stream's 61,460 landable bodies carry one (2,164 singing dunes, 1,275 singing ice, 3,970 quakes). They move nothing, so a people's
  world and the home world may take them: Heilya II drew singing dunes, its milky seas giving way (its order puts the sound before the water; the
  cabin's window at home is Q-01's again). The singing dunes hum where a dune's slip face slides under the explorer or the buggy (the sample's
  `slipFace`, the dune's lee whatever its steepness: the game's dunes stand from under 8 degrees to over 30), by the slide, the sand running on a
  few seconds, at the world's note (its grains' under its gravity: 70-100 Hz on an Earth, 30-50 on a Titan or a Mars) with its harmonics; the
  singing ice by the ice round the explorer and the day's warming or cooling of it (near cracks' dispersive chirps falling as 1/t^2, far ones',
  the sheet's whoops, the snow settling); the quakes R-307's every 90-240 s with a voice of their own (the P-wave's thump, the rumble under a
  heave, the stones' rattle; through the boots on an airless world), a tectonic world's too. The levels from the renders (`sounds`: nine WAVs,
  `shots/tests/sound_*.wav`; the hum twice the full wind, the ice's booms and the quake's rumble lowered from the clamp); the scenes
  `singing_dunes`, `singing_ice` and `quakes` with their cues and their sound. As built against the plan: the quakes the plates' worlds under an
  air and the hot moons' (the first build's airless rock took a quarter of the worlds); the scorched worlds keep no singing dunes (no dunes).
  The proof (`docs/decisions/log/6-1.4.0.md`): the first three slots P-07's; every body whose six slots are Q-02's hashes as Q-02's (39,558 of 44,632; 5,074
  took a sound); `regress` two hashes back to their earlier selves blessed, then 24/24 both ways; `screens` all 96 Q-01's again (60 Q-02's); the
  sweeps 456 of 475 the same (the sweep worlds and sheet carriers that took a sound); `unit` 383 checks, none failing; the flow passes; `bench
  check` within all 37 budgets, and six pairs with Q-02's binary level at their bests.

## Phase 4: the drifters

Source: `IDEAS-worlds-and-wanderers.md` section 3; the reading of 2026-10-21 (the radar from the galaxy's hash to the speaker;
the places, the travel and the light); `docs/reference/civilisations/radar.md` (C-07, C-15, C-16); `docs/decisions/log/` R-407 ("a
jump ends at a star") and O6-02 (the surface's light floor), both rewritten for a system with no sun.

### D-01: the dark between

* **The field.** A population of its own, hashed in large cells with its own salt, accepted only in a sector that holds no star
  (`starInSector` fails there: no star moves and the star field, the neighbourhood, the aim and `jumpTarget` never see a
  drifter), denser where the stars are (they were thrown out of systems) and in the old clusters. Each drifter: a seed, its kind
  by the dark's table (section 3), its size, its moons, its own heat, the star it came from (a real star back along a hashed
  path, as far as a few km/s carried it in the years since it left: a game light year is 1.5e10 km, so a millennium is tens
  of them) and how long ago it left (a people that lived there ended after it: D-05).
* **A system with no sun.** `STAR_ROGUE` appended to the star classes with no weight (the class pick's count stays fourteen:
  `Rng::pick` falls through to the last entry on a rounding), no light, no radius, a neutral colour; `StarSystem::sunless()`;
  the drifter body 0 at the system's anchor (no orbit, a huge period), its moons from the moon loop, no comets, belts or
  companion; `Body::heatK`, the world's own warmth, added to the star's in quadrature as a brown dwarf's moons are (0 everywhere
  else, so nothing moves): 30-60 K for a cold rock, more under a moon's tides, a giant's own heat.
* **The light gates.** `lightAt` gives 0, the sun sits at the nadir with no light (`SurfaceSite::sun`'s 0.62 floor and
  `starLightFactor`'s 0.35 are a sun's), no seasons, the almanac's sun events skipped, no zodiacal light, no planetshine phase:
  every one of the ~110 reads of the sun on the ground then behaves as a moonless night without being touched.
* **The way there and back.** One resolver, `placeInSector` (a star, else a drifter), at the 47 `starInSector` and 22
  `parseStarKey` sites that turn a key into a place (the save's `star` and `remote` lines, targeting by name, the previous and
  home, the radar's lock and accept, `arriveAtStar`, the history's regeneration, the probe's replay, the screens of the shards,
  the player and the instruments); the arrival's stand-off outside the drifter (40-80 radii: today's rule puts the ship at its
  centre); `toggleVimana`'s flight to it; the analyzer, the data sheet and the arrival log with no star (`NO STAR`, no
  equilibrium, no orbit, no season); the radiation warnings off. Until D-02 a drifter is reached by the harness and the console
  only.
* Done when: `vesperis_test rogues [n]` lists the drifters near home (kind, distance, heat, moons, the star it came from, how
  long ago) with their density against the stars'; unit checks (no drifter in a sector with a star; the field deterministic;
  a save at a drifter round-trips; the light 0 everywhere on it; no NaN from the anchor: positions, the almanac, the parking);
  frames of an arrival, the parked view and a landing (dark: the night's light); regress and `screens` unchanged (nothing of
  the stars moved); `docs/decisions/log/2-1.2.0.md` rewrites R-407's and O6-02's entries for a sunless system.
* **Done 2026-10-23.** The drifters' field (`galaxy/drifters.*`): cells of 256 sectors with a salt of their own, up to four candidates a
  cell kept as often as the stars are dense there (twice among the old populations; a globular's own besides) and only in a sector without a
  star, so the drifters are as dense as the stars times 1.4e-7: near home the nearest at 359 ly, 17 within 800, 29 within 1,000 (one for every
  7.4 million stars). Each a place of the class `STAR_ROGUE` (no light, no radius, no weight: every pick over the classes stops at
  `STAR_DRAWN_COUNT`), its kind by the dark's table (the giants and brown dwarfs 40 of 90, cold rock and ice, the far cold's dwarfs, carbon,
  metal, a desert, a europan world with its moon, a young molten or tectonic world, a living world frozen), its own heat (`Body::heatK`), its
  moons, the star it came from 1,500-20,000 years ago (6-340 ly back). A system with no sun: the drifter on the anchor and its moons;
  `sunless()` gates the light (`lightAt` 0; on the ground the sun at the nadir before the 0.62 floor), the seasons, the zodiacal light, the
  almanac's sun events, the sun drawn and the probe's sunlight; no people on them (D-05's). One resolver (`placeInSector`) where a key names a
  place; the flight and the arrival 40-80 of the drifter's radii off; the analyzer and the sheet `NO STAR`, its own heat, no orbit, no season.
  As built against the plan: the moons a brown dwarf drifter's glow warms past 205 K were drawn living worlds and are chemotrophs; the
  thermometer stays the airless night's (-270 C on a 36 K rock) for D-04. `rogues` (the list with the stars they left, the density, the
  kinds against the table, a flight there by `V`, the arrival, the parked view and a landing), five unit checks. The proof
  (`docs/decisions/log/6-1.4.0.md`): every body of the stream's systems as Q-03's; `regress` 24/24 both ways with nothing blessed; `screens` 96/96 Q-03's;
  the sweeps 474 of 475 (the chemotroph's known flip); the data sheets 24/24; `unit` 388 checks, none failing; the flow passes; `bench check` within all 37 budgets, level with Q-03's last check row by row.

### D-02: heard, not seen

* **The radar finds them.** `SIG_ROGUE` (kind 4) in `SignalKind`, the harness's names and the audio's channels; a third layer
  of the deep scan for the drifters' cells (in slices, on the main thread, as C-15's); their own reach law so none is ever heard
  at x1 (x1 stays C-07's list) and the first from home comes in at the middle gains; a power per drifter (a giant's aurorae
  loudest, a small world's faint).
* **The lock and the jump.** The readout after a lock: the distance and `NO STAR AT THE SOURCE`, nothing more (no class code:
  "the sound tells, the readout never does"); Enter flies there through the resolver; the chart's stops and its Enter snap to a
  locked drifter (`plotJump`: today it lands on the nearest star); the lock key is the sector's, so the chart's red ring and
  `A SIGNAL` work; visited, it is a place in the guide like a system, the explorer's to name.
* **The black disc.** The space view in the void: no sun drawn (`drawSun` skipped as the black hole's light is), the globe a
  disc where the stars stop, its hemisphere toward the bright band faintly lit (the band's light as the globe's only light, the
  globe's floor 0.05-0.10 today), its own glows (lava, aurorae, a brown dwarf's heat, its moons lit by it); the HUD's target
  marker on it.
* **The map without light.** The landing map, its zoom and the sector map draw a sunless world by its relief and heights (a
  survey look: the ship's altimeter, not its camera), as they go black at factor 0 today.
* Done when: `vesperis_test rogues heard` prints what home and the test worlds hear at each gain (none at x1; the gain each
  drifter needs; its distance); unit (x1 unchanged; C-15's checks exclude the drifters and check them by their own law; the
  slices find what the whole scan finds; a locked drifter plotted from the chart lands at it); frames of the radar's lock, the
  arrival, the black disc from 60 radii and from orbit, the landing map; the flow unchanged.
* **Done 2026-10-23.** `SIG_ROGUE` and a law of the drifters' own: a drifter of power p heard at gain G within 180 sqrt((G - 1) p) ly, so
  never at x1; the power by its kind (a brown dwarf the loudest, a giant next, a young or metal world's dynamo, a rock's, a small icy world's
  the faintest), its size and a spread of its own. From home none at x1 or x5, the first a brown dwarf at 437 ly at x11, six at x32 (the
  farthest a giant at 1,197 ly); the nine test places six or seven at x32, their first at x11. Two layers of the far scan (the drifters'
  cells, the globulars' own), in the same slices; the synth plays a drifter as a magnetosphere by its seed until D-03. The lock reads `NO
  STAR AT THE SOURCE` and the distance alone, its key the sector's (the chart's ring and `A SIGNAL` stop at the place); Enter flies; the
  chart's Enter plots a known drifter under the cursor itself; arrived, the drifter is the local target and its place the explorer's to
  name (`NAME THIS PLACE`). The black disc: the band's own light (its spherical harmonics' irradiance, drawn in the band's exposure: the
  coreward side faintly lit, the stars stopping at the edge), a brown dwarf's glow, lava, lightning, a giant's aurorae at its poles, a
  glowing drifter's moons lit by it. The survey: the landing map, its zoom, the sector map and the drone bay's map by heights and relief in
  the radar's cold phosphor. As built against the plan: the first light, through the ground's exposure, drew a giant brighter than the band
  behind it and gave way to the band's own exposure; the drone bay's map takes the survey too. `rogues heard` (the lists by step from ten
  places, the lock, the flight, the discs, a brown dwarf's moon, the survey's maps), three unit checks. The proof (`docs/decisions/log/6-1.4.0.md`): every
  body as Q-03's; `regress` 24/24 both ways with nothing blessed; `screens` 96/96, the sweeps 475/475 and the data sheets 24/24 D-01's;
  `signals` as D-01's binary prints it; `unit` 391 checks, none failing; the flow passes, its x32 sweep hearing the six. `bench check` within all 37 budgets, level with D-01's in pairs (the
  telescope's 2x a placement of the code: level in a scratch harness with D-01's globe code against D-02's).

### D-03: the voices of the dark

* **A voice for every drifter** (channel `case 4` of `channelSample`, its parameters from new `RadarVar` indices so no existing
  signal's sound moves): the breath of its turning (a swell every 20-90 s, the beam sweeping past), the body by what it is (a
  giant's decametric storm: slow L-burst swishes like surf, falling S-burst crackle, a roar; a small world's chorus of rising
  chirps, falling whistlers and hiss; a moon's flux tube pulsing at the moon's period), its motif (a figure of whistles at its
  own pitches that recurs each breath: its signature), plasma vowels (sweeping resonances that moan with no language in them),
  detuned drones beating under it, silences when the beam turns away. Built on the magnetosphere's channel and the static's
  events (the whistlers, the chorus, the choir, the moans and sweeps of C-16).
* **Where it is heard.** On the radar from afar, worn by the distance (C-16's wear) and mixed under what else is in the beam;
  whole when the ship is close; on the cabin's radio while the ship is at the drifter; faint in the suit's radio on its ground.
  The beacon's slot for D-05 (a machine of C-16's under the voice, its message D-05's).
* **No two alike.** A measure of distinctness over a hundred drifters (their parameters' spread and the spectra of their WAVs:
  every pair apart by more than two signals of one kind are).
* Done when: `vesperis_test rogues voices` writes twelve WAVs (`shots/tests/rogue_voice_*.wav`) with their parameters and
  levels (no peak over 1, rms within C-07's bounds, at most 20 clipped samples), the distinctness printed; the unit's level
  checks include the kind; the flow unchanged.
* **Done 2026-10-23.** A voice for every drifter in the receiver's `SIG_ROGUE` channel (`game/audio_rogue.cpp`), on parameters of its own
  (`rogueDerive`, indices from 161) and what its system says of it (`RogueVoice`: its class by its body, its turning and its innermost moon's
  orbit, `Signal::turnS` and `moonS`, its power): the breath of its turning (20-90 s, the other pole by its seed, silence between), the body
  by what it is (a giant's storm, a brown dwarf's thicker with its cyclotron howl, a small world's chorus, whistlers, hiss and kilometric
  howl, a young world's dynamo, a metal world's ring), its moon's flux tube (an arc every 0.9-16 s), its motif (four to six whistles on a
  scale of its own, the other pole answering backwards in a key of its own, each whistle hanging on 10-18 s), the moan and the plasma's
  breath, a drone chord on its scale, its parts weighed by its seed, the whole through its plasma's two resonances. Heard on the radar worn
  far and whole near, under another signal, on the cabin's radio at a drifter and the suit's on its ground (`Game::darkRadio`); the beacon's
  slot for D-05 (`radarVoice`: a machine of C-16's, slowed and darkened). No two alike: over the hundred drifters nearest home every pair's
  voice prints lie at least 4.55 dB apart (the median 7.2), two magnetospheres' 4.19 at the median. As built against the plan: the first
  voices were as alike as two magnetospheres, and the echoes, the answer, the chord, the weights and the plasma's resonances were what set
  them apart; the synth's cost halved to about 7 ms a second of sound. `rogues voices`, two unit checks. The proof (`docs/decisions/log/6-1.4.0.md`): every
  radar WAV of `signals wav` byte for byte D-02's but the drifter's three; every body as Q-03's; `regress` 24/24 both ways with nothing
  blessed; `screens` 96/96, `signals` and `rogues heard` as D-02's; `unit` 393 checks, none failing; the flow passes. `bench check` within
  all 37 budgets, every row level with D-02's in pairs but the decoder's (0.1-0.4 ms, a frame D-03's code does not run in: the layout).

### D-04: the ground in the dark

* **The explorer's lamp.** A helmet lamp on a key (and the pad's command menu): a cone of light from the head into the vertex
  light of the ground and into the six light formulas of the objects (flora, creatures, the buggy, the drone, the people's
  forms, the rocks), falling off over 40-60 m, for every night side as well as the dark; the capsule's floodlights by it on
  landing.
* **The drifter's own traits:** frozen air (the atmosphere fallen as nitrogen and oxygen snow: frost on everything, the sky
  black, no wind, the pressure a trace), embers (a young interior: lava lakes glowing in the dark, the ground lit by them, the
  globe's spots of light), lights left on (a dead people's city still lit: points on the night side from orbit, lit windows
  and lamps on the ground; D-05's peoples of the late ages), lights in the water (a dark ocean's glow, faint, in patches).
* **The dark ocean** (section 1): the sea liquid in the dark at 280-350 K under a hydrogen sky, black basalt islands with steam
  at their vents, the stars dimmed; frozen seas on every other drifter with water (ice at the sea's level); the temperatures by
  `heatK` on the HUD.
* Done when: scenes `rogue_lamp` (a rocky drifter, the lamp on and off), `rogue_embers`, `rogue_frozen_air`, `dark_ocean`;
  the lamp's cost within budget (`bench check` gains a lamp row); unit (the lamp on a dayside changes nothing visible over a
  threshold; the drifters' kinds by the dark's table); regress.
* **Done 2026-10-23.** The explorer's lamp on `U` (and the pad's commands), on foot, on every night side as in the dark: a cone of light from
  the eye along the view, full to 11 degrees and gone at 22, falling off to nothing from 42 m to 60 m, added into the ground's vertex light
  and every object's light (the rocks and the other ground objects, the capsule, the buggy, the drone, the creatures, the people's forms, the
  flora), by the night alone, so a dayside's frame is the same lit or not; the capsule's floodlights down and ahead as it comes down and goes
  up. A world in the dark: its thermometer by its own heat (-223 C on a 50 K rock, +57 C on a dark ocean), an air colder than its freezing
  fallen as snow (`bodyHasAir`: no wind, no weather, a black sky, a trace of pressure; snow as deep as its air was heavy, frost on the rest),
  its seas frozen at their level where colder than their liquid's freezing, and no trait it cannot have there (no sun's, no sun-fed life's,
  no air's where its air fell). The drifters' own traits, given: FROZEN AIR (78 bodies near home), EMBERS (every lava world in the dark and
  three in ten of the rock drifters: fields of lava lakes inside spatter ramparts, the ground lit by them, the globe's spots of light; 69),
  LIGHTS IN THE WATER (half the dark oceans: patches of light over the islands' flanks and in shoals, drifting; from orbit their sheen),
  LIGHTS LEFT ON (built for D-05's late peoples and staged: a third of the windows lit, far off a scatter of points, street lamps, the towns'
  points on the night side from orbit). The dark ocean, made from two in five rocky drifters (their moons, power and voice kept): a sea at
  280-350 K under 80-600 bar of hydrogen, archipelagos of black basalt shields on the hot spots' swells with black sand benches and steaming
  summit crusts, the stars dimmed and reddened. As built against the plan: the plan's frozen seas came with the traits a world in the dark
  cannot have, which the first survey of the drifters found (aurorae, haze and singing dunes at 40 K, halos round no sun); the dark ocean is
  made, not drawn, so no drifter's kind, power or voice moved; the lights left on are drawn but given to no world until D-05. `rogues dark`,
  the four scenes, `kinds dark`, four unit checks, the bench's lamp row. The proof (`docs/decisions/log/6-1.4.0.md`): every body of the stream's systems as
  Q-03's; `regress` 24/24 both ways with nothing blessed; `screens` 96/96 D-03's; the sweeps 475/475 D-02's; the data sheets 24/24, `signals`,
  `rogues heard`, every radar WAV and `rogues voices` as D-03's binary; `unit` 397 checks, none failing; the flow passes; `bench check`
  within all 38 budgets, every row level with D-03's binary over four pairs (the city's 4x, 0.1-0.3 ms over at first, by the window test's
  two flags more on every windowed pixel: one flag now, D-03's test unless a window is left lit).

### D-05: the people who lost their sun

The user: "civilization shards should tell how their planet lost the system, sun disappeared and they died (should be unique
for rogue planets)". One drifter in four of the frozen felisian and desert kinds kept a people (its own hash; two peoples end
together on a drifter). Their story is the lost sun's, and no other world's is.

* **The then-world** (P-04's, here with a system): `formerSystemOf(drifter)` is the star it came from (D-01's real star, with
  its own system as `generate` makes it) and the drifter at its former orbit in that system's temperate band (its year, its
  eccentricity, the moons that came with it; that system's giants, comets and belts as its sky). The profile (`readWorld`),
  the history (`buildHistory`, `historyOf`), the lore and the calendar (`calendarOf` divides by the orbit's year today: the
  former year, which their records keep counting after it ends), the records' world facts, the charts and the instruments read
  it. `historyOfWorld` gets a drifter's branch: without one a drifter's texts, record homes, town layers and works vanish
  without a word.
* **The cause.** `CAUSE_SUN_LOST` ("the loss of the sun"), appended to the causes outside the old pick (the 37 are drawn as
  before; `Rng::pick` falls through to the last entry on a rounding), forced on a drifter's people. Its way (`how`): the
  wanderer (a giant of their own system grew nearer each year, then the summers came late and short: planet-planet scattering,
  the commonest; the giant is a real world of the former system), the intruder (a star brighter each generation until there were
  two suns, then neither: a flyby), the dark (the stars bent round a point where nothing shone, the year pulled apart: a black
  hole or a neutron star passing), the second sun's dance (a companion stretching each year until one let go). Its length by
  the age: a prehistoric people's in a few generations, a futuristic people's over centuries below ground, a far-future
  people's over a thousand years.
* **The history** (H-02): a SCRIPTS row (the long cold from the onset, the harvests falling, deaths of the cold, the warmest
  ground last: the hot springs, the volcanic valleys, the deep), a dimming curve kept in the history (as the harvests and the
  numbers are) and a public event kind from it (`HE_SUN`, before `HE_BIRTH`: the sun smaller, the year that does not end, the
  sea frozen from the shore, the air falling as snow, the last sunrise, the stars at noon), the omen by the way (the wanderer
  growing, the new star, the bent stars, the dance), a new whither (below: `WH_UNDER`), the last shelter a deep hall by the
  warmest ground (T-03's last shelter).
* **The records** (H-04): year facts (`dark` 0-3, the years since the loss), genres of their own in `sunlost*.vg` for every age
  from the prehistoric (the collapse genres start at the medieval today): the myth of the sun walking away and the fires lit on
  the hills to call it back, the count of days without a dawn, the chronicle of omens and the last harvest, the observatory's
  table of the sun's diameter falling year by year, the broadcast and the rules of the deep halls, the letters to the sun, the
  lesson of what the sun was for the children born in the dark, the last broadcast; the rules whose every alternative is guarded
  by a cause given theirs (`cause.what`, `cause.noun`, `last.cause`, `em.cause`, `mod.cause.noun`); the ordinary genres darkened
  by `dark` (the `sun`, `season.now` and `weather.now` words). The grammar's measures run again (a new alternative moves the word
  fatigue's counts a little for every people, as every grammar pass has).
* **Charts and music** (H-05): a new way of chart, the sun's measure (its disc year by year, a light curve, drawn in the age's
  way: shrinking suns on a cave's ceiling to a printed graph); their star charts mark the star that was their sun (C-10's rule:
  Enter flies there); the music by the cold (a branch of `formOf` for the dawn song and the last sunrise, and the last song,
  PF_LAST, which no record draws today because the last recording is always a text).
* **The works of the end** (T-03): shelters below ground, domes over the hot springs and vents, greenhouses, the last city;
  the graves of the last generations; the lights left on (D-04) for the late ages.
* **The beacon.** A radio-age people's last broadcast, looped under the drifter's own voice (D-03's slot): a machine of C-16's
  and a spoken message in their tongue (C-05's voice, slowed and worn), its text a record of theirs (the last broadcast), caught
  by the radar (`Guide::heard`) and read as their language is learnt. The decoder's history page: `ENDED BY THE LOSS OF THE SUN`.
* **Streams kept.** Every drifter-only draw takes a salt of its own; nothing is drawn before a record's first uniform (it is the
  record's year, age and tone at once); the tones' new flag (a Lore field beside the desert's) joins the text cache's key; the
  instruments' lists are not grown (their ids are saved keys); the fixed tables grow in step (`CAUSE_NAMES`, `CAUSE_KEYS`,
  `SCRIPTS`, `LEFT_BY_CAUSE`, the event kinds' names and keys, `WHITHER_KEYS`, the harness's cause tables).
* Done when: `peoples rogues` lists the drifters' peoples near home (the age, the way, the years, the beacon); `shards <drifter>`
  prints fifty records each of two test drifters of different ages, for the user to read beside a felisian people's; a chart of
  the sun's measure in each age's way; the beacon heard in `rogues voices`; unit (no other people's records, salts or shards
  change; the grammar's checks pass with every new rule reachable; a drifter's people's history is the same built from the
  ground and from the decoder); regress; the flow.
* **Done 2026-10-23.** One frozen felisian or desert drifter in four kept a people (103 of the 406 within 9,000 ly; 17 peoples on 13
  drifters within 3,000), who lived on it at its former orbit round the star it left: a living sun near its way's end chosen to fit how the
  sun was lost (a giant for the wanderer, a companion for the dance), that star's system as `generate` makes it (`formerSystemOf`), the
  people's profile, lore and calendar, history, records, charts and instruments reading it (`peoplesWorldOf`), the ground its then-world.
  The loss of the sun is the 38th cause, given, outside the old pick, its four ways (9 by the wanderer, 1 the intruder, 4 the dark, 3 the
  dance near home), its timeline from the generator (the warning's omens, the short summers, the sun going, the last sunrise, the long dark
  below); `HE_SUN` and the sunlight in the history, the warmest ground left last, the whither below. Eight genres of records of their own
  for every age (`sunlost*.vg`: the omens and the last harvest, the myth of the sun walking away, the observatory's table of its disc,
  letters to the sun, the count of the days without a dawn, the rules and broadcasts of the halls below, the lesson for the children born in
  the dark, the last broadcast), the loss told in the ordinary registers, the cause's rules given theirs, the sun's, seasons' and weather's
  words darkened (two directives: `@apart` keeps their words out of every other people's statistics, `@instead` swaps a rule for them
  alone); the sun's measure as a chart in every age's way, their star charts marking the star that was their sun, the dawn song, the last
  sunrise's and the last song; the works of the end (shelters by the warmest ground, domes over it with their glasshouses: `BK_DOME`,
  `BK_GREENHOUSE`, the graves of the last generations; the lights left on for the late ages); the beacon, a radio people's last broadcast
  under the drifter's voice (its machine by the age in the cabin's and the suit's radios, the message slowed and worn on the radar camera,
  heard when locked: 32 beacons on the 40 radio peoples within 9,000 ly). As built against the plan: the way is drawn before the sun is
  chosen (the first cut's former systems held no giant: every way was the intruder or the dark); the beacon is the history's (a radio its
  history invented before the last sunrise), and such a people goes on inventing to the last sunrise, its railways after the railway; the
  last city is the last place (no dome over a town). `peoples rogues`, `shards drifters` (the test drifters Balyaphebist and Vreikheid,
  fifty records each), `shards rogue`, `grammar rogues`, `charts rogues` (12 of 12), `works rogue`, the beacon in `rogues voices`, the domes
  and glasshouses in `ruins kinds`; six unit checks. The proof (`docs/decisions/log/6-1.4.0.md`): every body as D-04's (fp5); every sampled people's
  records as D-04's, byte for byte (`recdump`), and the age and scene worlds' pinned; `regress` 24/24 both ways with nothing blessed;
  `screens` 96/96, the sweeps 475/475 and the data sheets 24/24 D-04's (two frames run beside other jobs moved, the same alone); `signals`,
  `rogues heard`, every radar WAV and `rogues voices` (but its beacon) as D-04's; `rogues dark`'s frames and the dark's scenes D-04's; `unit`
  403 checks, none failing; the flow passes (20.3 s); `bench check` within all 38 budgets in D-05's four runs and every row level with
  D-04's binary over four pairs (the buggy's 2x as noisy in both: D-04's binary 3.2-6.6 ms, once over its budget, D-05's 3.5-4.8).

## Phase 5: the repo for its readers (added 2026-10-23)

E-01 to E-13, planned after D-05 on the user's request. The scope, the rules, the proof and each milestone's done-when are in
`PLAN-reorganisation.md`, not repeated here. Generation 16 is unchanged and nothing is blessed: every step is proven identical
to the binary before it.

* **E-01 done 2026-10-23.** The memory cut to an index (its lessons in the CLAUDE.md files, `docs/reference/lessons.md` and
  `packaging/RELEASE.md`), the root CLAUDE.md with the map, seven directory notes and the harness's rule, the guards, and the eight
  tasks' baseline (the plan's section 11). No code changed.

* **E-02 done 2026-10-24.** The proof kept: the fingerprint modes in `src/harness/`, `tools/prove` with its battery of 27 items
  (about 15 minutes a compare; snapshots in `build/prove/`), the almanac's finders listed in their registry, a `make clean` that
  keeps `build/`'s kits and toolchains, `make check`, `compile_flags.txt`, the `prove`, `bench` and `look` skills and the `prover`
  agent. Proven from a clean rebuild: `tools/prove compare e02-base2` everything the same (27 items, 1,271 files; the unit's 403
  checks, the flow and the regress both ways among them); the bench pairs level but for rows bound to the disk.

* **E-03 done 2026-10-24.** The harness in pieces: `src/main_headless.cpp` is `src/harness/` (43 files, the largest 1,334 lines),
  moved by `tools/mover.py` and proven function by function by `tools/samecode.py`; `MODES` with `help` and `help md`; the unit by
  its 41 groups (`unit list`, `unit <group> -q`); the flow, the bench and the scene finder by stages and families; the scenes'
  codes named; the `harness` skill; a one-file harness edit rebuilds in 1-3 s. From a clean rebuild, `tools/prove compare e02`
  reported everything the same (27 items, 1,271 files: the unit's 403 checks, the flow and the regress both ways among them); the
  A/B batteries the same but for `parkings`' `_b` frames, which aim at a body without waiting for its map and draw one of two
  pictures from the old binary too (KI-419); all 41 unit groups pass alone; `bench check` within its 38 budgets in all eight runs
  of four AB/BA pairs against e02's binary, level but for `buggy 2x` (noisy) and two rows the new binary ran faster (`landzoom Z`
  0.96, `descent people` 0.94). `tools/prove snapshot e03` is E-04's base.
