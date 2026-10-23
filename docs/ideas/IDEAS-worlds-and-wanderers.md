# Ideas: the worlds and the wanderers (new kinds, new traits, rogue planets)

Planned 2026-10-21 on the user's request (`docs/iteration-3/plans/requests/R-421`, `R-422`, `R-423`), for 1.4.0 as three
blocks of milestones (`ITERATION-1.4.0.md`): **P, the kinds of world**; **Q, the qualities** (the traits); **D, the
drifters** (rogue planets). Nothing here is built. This note holds the intent and the lists; the iteration file holds the
milestones, their checks, the order they are built in and the decisions taken with it.

The request, in the user's words: "improved planets generation (there should be 30-50 new planet types, not too
ridiculous so it's still scientific enough), each planet should have way more unique modifiers (again they should make
sense) - 20 to 30 more unique modifiers in addition to what we have now, also greater variety of modifiers in general.
One interesting request/idea for planets - consider rogue planets that lost their system and just fly in space. They
should be able to be any kind of available planets and the only way to find them is by radio signal (they should be very
unique, cosmic horror like sounds). If rogue planet is civilized desert (it shouldn't be felisian since it would be cold
already but you can add dead felisian world also among these 30-50 new types) then civilization shards should tell how
their planet lost the system, sun disappeared and they died (should be unique for rogue planets)."

The rule for everything below: a kind, a trait or a wanderer exists because a real world shows it, a mission measured it or
a published model predicts it (section 5 lists the sources). Where the science is a proposal rather than an observation
(hycean worlds, superhabitable worlds, life without light) the data sheet's sentence says what is seen, never more.

## 0. What exists (generation 15)

* **Twenty-one kinds of body** (`PlanetType`, `src/galaxy/system.h`), nineteen landable: molten, cratered, venusian,
  felisian, rocky, thin atmosphere, gas giant, icy, quartz, ocean, metal, volcanic, carbon, substellar (brown dwarf),
  comet, europan, tectonic, desert, hydrocarbon, bombarded, acidic (and the companion star, a body of type
  `PT_COMPANION`). Each landable kind has its own branch of the planet function (`sampleSurface`), its palette
  (`materialPalette`), its parameters (`BodyGen::make`), its sky (`typeSky`), its weight in five temperature zones and under
  each star class (`pickPlanetType`), its sentence (`describeBody`), and some an event on the ground (R-307's geysers, lava
  fountains, quakes, dust devils, meteorite strikes). The giants carry a character (X-03) the probe descends through.
* **Thirty-three traits** (`Trait`, `src/galaxy/planetmap.h`): eighteen landforms (R-305: canyon lands to geyser basins),
  thirteen world traits (R-304: archipelago, pangaea, lake country, snowball, exotic seas, storm world, haze, giant flora,
  luminous flora, dead forests and three ground colours), and two never drawn (glassed, S-03; dead civilisation, C-01). A
  world draws up to three, hashed from its seed: none for a fifth of the worlds, one for two fifths, two for a bit over a
  quarter, three for the rest (`BodyGen::traits[3]`). The data sheet prints them on one `TRAITS` line.
* **Signals** (C-07, C-15, C-16): peoples' worlds in hashed transmitter cells, pulsars, and in the system the ship is in,
  comets, magnetospheres and the system's own people; a gain of x1 to x32 hears the strongest to about 6,000 ly; the
  natural voices are synthesised from the source's seed (the magnetosphere already has whistlers, chorus, hiss, sferics, a
  roar and a choir). Every signal is a star's: the lock, the jump, the arrival and the save all find the source through
  the star of its sector.
* **The dark**: no body exists without a star. The black hole (S-06) is the one system whose sun gives no light; the
  nights (W-04) light the ground by starlight, the band, planetshine and aurorae.

## 1. The kinds of world (P): thirty-eight new

Thirty-eight new kinds, thirty-two of them landable: 59 kinds of body after it, 51 landable. Each has the machinery
R-307 gave its six: a ground of its own (built on the nearest existing kind's branch, `section 4`), a palette, a sky, a
temperature and pressure, a sentence for the data sheet, a list of the traits that fit it, a weight where it forms, a
harness scene, and where its world has one, an event on the ground.

**Where they come from.** The new kinds do not take new slots in `pickPlanetType`'s draw: each comes from a world that
drew an existing kind, by a hash of its own and the world's facts (its temperature, its star, its size, whether it is a
moon, whose moon, locked or not). A felisian world near the warm edge of its zone may become a hothouse world; a gas giant
at 900 K an alkali giant. So the system's draws stay as they were (orbits, sizes, moons, rings: the same) and most worlds
keep their kind; one in three changes, and the galaxy reads differently without moving. A world with a people never
changes kind (its people, its towns and its signal stay where 1.3.2 put them), and the home world stays felisian.

### P-01: the furnace worlds (the inner orbits)

| Kind | The data sheet's line | Size km | Air | From |
| --- | --- | --- | --- | --- |
| MAGMA OCEAN | large, locked to its star: a sea of magma under the sun, black glass on the night side | 4,000-9,000 | none to speak of | molten, over 1,100 K |
| CHTHONIAN | large and heavy: the bare core of a giant that came too close to its star, a ring of what it lost | 9,000-20,000 | a thin remnant | molten, metal, over 900 K, by hash |
| GLASS | small: plains of black volcanic glass, dunes of glass sand, tektites glinting | 1,500-5,000 | none | bombarded, cratered, over 500 K |
| STEAM | medium to large: inside the runaway greenhouse, a white sky of steam, seas boiling at the poles, the lowlands lost in fog | 5,000-11,000 | thick, opaque | ocean, felisian, venusian, 345-600 K |
| SALT | medium: an ocean that dried away, white crusts to the horizon, brine pools, salt pillars | 3,000-6,500 | thin, dry | desert, 300-500 K |
| RUST | medium: its water lost to space and its oxygen left behind, every stone rusted red | 3,500-7,500 | thin, oxygen | desert, thin, felisian rolls of red dwarfs |

* **Magma ocean.** K2-141b, 55 Cancri e and CoRoT-7b orbit their stars in hours and are locked: the day side is a sea of
  magma, the air a thin vapour of rock that condenses on the night side. The ground: within 60-75 degrees of the
  substellar point the sea at the reference level (the lava bank, glowing), its shore a crust of rafts breaking and
  sinking (the chaos blocks set in lava), the night side black glass and basalt under a pale frost of condensed rock
  (sodium, silica). The sun huge and fixed; fire fountains along the shore; on the night side, nothing moves. Landable on
  the crust.
* **Chthonian.** TOI-849b is the exposed core of a giant: forty Earth masses in three Earth radii, iron, rock and water,
  almost no hydrogen left. The ground is smooth under three to five g (low, wide relief, broad shallow basins), dark iron
  basalt, a thin pale sky; rings two times in three (the lost envelope's dust). The heaviest ground the explorer walks.
* **Glass.** Obsidian flows, the impact melt sheets and tektites of Earth, the orange glass beads of the Moon: on a hot
  airless world flash-heated by flares or impacts, the plains are sheets of black glass (conchoidal ridges, cooling
  cracks), the sand is glass, strewn fields of tektites glint at low sun (`MAT_GLASS`, S-03's bank, is ready).
* **Steam.** A world past the runaway greenhouse limit (Kasting 1988; Goldblatt 2013): its ocean is in the air, hundreds of
  bars of steam over 400-700 K; below the critical point's pressure height the "sea" and the sky are one supercritical
  fog. Landable on the highlands: mud, fumaroles, boiling lakes, scalding rain; walking downhill the fog thickens until
  nothing can be seen. The globe and the sky are a white deck (`hasOpaqueDeck`).
* **Salt.** The Mediterranean dried to a basin of salt in the Messinian crisis; the Danakil, Uyuni and the chloride
  basins of Mars are the same at smaller scale. Concentric bands of gypsum, halite and potash round the old basins (the
  sea's last shores), polygon crusts (R-305's salt flats, here the whole world), brine pools coloured by their minerals,
  salt pillars and domes, salt glaciers flowing out of the diapirs; a white, dazzling ground, heat shimmer.
* **Rust.** Luger and Barnes (2015): the worlds of a red dwarf's habitable zone spend their first hundreds of millions of
  years in a runaway greenhouse while the star is young and bright; their oceans split and the hydrogen escapes, leaving
  oxygen behind, hundreds of bars of it. Every rock oxidised red to purple-brown, hematite concretions as gravel (Mars'
  "blueberries"), the valleys of the lost water dry, a pale sky over red dust.

### P-02: the living worlds (the temperate band)

| Kind | The data sheet's line | Size km | From |
| --- | --- | --- | --- |
| ARCHEAN | medium: young life, microbial mats in the shallows, stromatolite reefs, a green sea under an orange haze | 4,800-7,600 | felisian, young stars by hash |
| HOTHOUSE | medium: no ice anywhere, forest and swamp from coast to pole, warm shallow seas, storms | 4,800-7,600 | felisian, the warm edge |
| SUPERHABITABLE | large: warmer and wetter than a felisian world, shallow seas, a thousand islands, thick air | 7,000-9,500 | felisian, orange dwarfs by hash |
| STEPPE | medium: a dry living world, grass to the horizon, small seas, salt lakes, forest along the rivers | 4,800-7,600 | felisian, dry by hash; the inner edge |
| TUNDRA | medium: an ice age, ice sheets to the middle latitudes, taiga and tundra below them | 4,800-7,600 | felisian, the cold edge |
| LICHEN | medium: cold and dry, life only as crusts and mosses in the hollows, a thin clear sky | 2,400-4,600 | thin atmosphere, 205-260 K |

* **Archean.** Earth from 3.8 to 2.5 billion years ago: no oxygen, a methane greenhouse and an organic haze that turned
  the sky orange (Arney 2016, "the pale orange dot"), an ocean green with dissolved iron, life only microbial: mats in the
  shallows banded purple, green and orange (retinal and chlorophyll: DasSarma's "purple Earth"), stromatolite domes along
  the tidal flats, bare grey and brown land, volcanic island arcs. No trees: the flora pass draws none, the mats are the
  ground's colour.
* **Hothouse.** The Eocene and the Cretaceous: no ice at either pole, palms and crocodiles in the Arctic, warm shallow
  seas. The felisian ground with the cold biomes gone (tropical forest and wetland to high latitudes), a humid haze,
  thunderstorms most afternoons, giant flora more often.
* **Superhabitable.** Heller and Armstrong (2014): a world two or three Earth masses round an orange dwarf, older, a
  little warmer, shallower seas and a thicker air, its land broken into archipelagos: more coast, more shelf, more life.
  Wide turquoise shelves, reefs, many islands, dense forest; the commonest kind round orange dwarfs' temperate worlds.
* **Steppe.** Abe et al. (2011): a "land planet" with little water keeps a habitable zone wider than an ocean world's
  (liquid water to 170% of Earth's sunlight at its poles, and it resists freezing to 77%). Grassland, savanna and scrub as
  the main biomes, small inland seas and salt lakes, forest only along the rivers, big swings of temperature.
* **Tundra.** The last glacial maximum: ice sheets down to 45-50 degrees, glaciers in every range, the mammoth steppe,
  taiga, permafrost patterned into polygons and pingos, sea ice; dry cold winds.
* **Lichen.** The Antarctic Dry Valleys' rocks hold lichens inside them (Friedmann 1982); the first land life was crusts
  and mosses. A thin-air world in the temperate band: bare rock with patches of orange, black and green crust, moss mats
  in the wet hollows and along the meltwater, nothing taller than a hand.

### P-03: the locked and the deep (red dwarfs, water worlds, other liquids)

| Kind | The data sheet's line | Size km | From |
| --- | --- | --- | --- |
| EYEBALL | medium, locked to a red dwarf: one round open sea under the fixed sun, ice everywhere else | 4,800-7,800 | ocean, felisian, icy rolls of red dwarfs' locked worlds |
| TERMINATOR | medium, locked: a desert under the fixed sun, ice on the night side, a ring of rivers and life between | 4,800-7,600 | felisian, desert rolls of locked worlds |
| HYCEAN | large: a deep warm ocean under a hydrogen sky, no land, a pale hazy horizon | 10,000-16,500 | ocean, large, 250-400 K |
| AMMONIA | medium, cold: seas of ammonia water that never freeze, a yellow-brown haze, slabs of ice | 3,000-7,000 | ocean, icy, hydrocarbon, 150-230 K |
| SNOWBALL | medium: frozen to the equator, ice sheets everywhere, a thin band of open sea, blizzards | 4,800-7,800 | felisian, ocean, colder than tundra |
| CHEMOTROPH | medium: life fed by the ground's heat, not by light: pale mats round hot springs, glowing spores at night | 2,400-5,200 | tectonic, rocky, of dim stars |

* **Eyeball.** Pierrehumbert (2011): a world of water locked to a red dwarf freezes over but for a round open sea under the
  fixed sun, the pupil of the eye. An ice sheet over the whole world, the open sea ringed by floes (the ocean branch with
  its floe line round the substellar point instead of the poles), the sun never moving, winds that never change.
* **Terminator.** Lobo et al. (2023): a locked world with little water can be scorching under the sun and frozen on the
  night side and keep a temperate ring at the terminator (an ocean world cannot: its day side steams the whole sky). The
  day side a desert of dunes and salt, the night side an ice sheet, between them rivers of meltwater running sunward into
  lakes, and a green ring of life along them.
* **Hycean.** Madhusudhan et al. (2021): a world of two to two and a half Earth radii with a deep ocean under a
  hydrogen-rich air, habitable over a wider band than an Earth. No land, a cream-white haze, a dark teal sea; no floes
  where it is warm. The data sheet says what is seen (the air, the sea), not that it lives.
* **Ammonia.** A mixture of ammonia and water stays liquid down to about 176 K; Titan's buried ocean is thought to hold
  ammonia. Seas of ammonia water at the reference level (pale grey-blue), slabs of ice drifting, ammonia frost on the
  heights, a yellow-brown haze the sun shows through dimly, rounded hills of ice and rock.
* **Snowball.** Earth froze to the equator at least twice (720-635 million years ago; Kirschvink 1992, Hoffman 1998).
  Ice sheets and sea glaciers everywhere, nunataks of rock standing out of them, a thin belt of open or slushy sea at the
  equator, blizzards, ice halos round the sun.
* **Chemotroph.** The hydrothermal vents' life lives on chemistry, not light (Corliss 1979). A dim world of a white dwarf,
  a brown dwarf or a neutron star, warmed from inside: dark volcanic rock, hot springs and fumaroles everywhere, pale
  mats (white, cream, pink) spreading from the vents, sinter chimneys, and at night spores glowing over the springs.

### P-04: the dead worlds

| Kind | The data sheet's line | Size km | From |
| --- | --- | --- | --- |
| FROZEN FELISIAN | medium: a living world that lost its warmth, seas frozen to the floor, forests dead under frost | 4,800-7,600 | the drifters (D) and the wanderers (P-05) |
| SCORCHED FELISIAN | medium: a living world baked by its swelling sun, seas boiled off to salt, forests turned to stone | 4,800-7,600 | felisian rolls of red giants, orange giants and carbon stars; rocky and desert rolls of white dwarfs' temperate worlds |

* **Frozen felisian** (the user's dead felisian world). What Earth would become without its sun: the seas frozen at their
  level (flat ice with pressure ridges and cracks), the rivers and lakes ice, the forests standing dead and white with
  frost, snow drifts, much of the air frozen out as frost toward the poles, a thin black-blue sky. Most drifters with a
  people are this kind.
* **Scorched felisian.** Earth in a billion years (the brightening sun ends the oceans; the red giant ends the rest): the
  old seas' beds white with salt and pale reef limestone, the shores benches high above them (C-01's dry seas: the same
  machinery), forests turned to stone, cracked clay and dust, a thin hot sky under a vast red sun. Round a white dwarf
  the same world after the giant phase, cold, with frost on the salt.
* **Both host peoples.** The dead worlds join the felisian and desert worlds as the only worlds a people lived on, each read
  in two times: the world in its people's time (then: the living world, its seas liquid, its climate theirs) for where they
  lived and what they made, and the world now for what is drawn. A frozen felisian world's people is a drifter's (D-05: how
  they lost their sun; a wanderer keeps none, so the story stays the drifters'); a scorched world's people lived while its sun
  grew and died as a desert's people did, of the drying and the climate.

### P-05: the far cold

| Kind | The data sheet's line | Size km | From |
| --- | --- | --- | --- |
| NITROGEN | small, far out: plains of nitrogen ice churned into cells, mountains of water ice, red lands, a thin blue haze | 900-2,500 | icy, under 60 K |
| CRYOVOLCANIC | small, a captured moon: dimpled ground, pink frost, dark plumes rising kilometres into a thin sky | 1,000-2,000 | icy, europan moons of giants, retrograde ones first |
| DRY ICE | small, cold, a thin sky of carbon dioxide: caps of dry ice, spider-shaped channels, dark fans | 2,000-4,500 | thin, icy, 140-200 K |
| WANDERER | small to medium: a frozen rogue the star caught in a wild orbit, its air fallen as snow | 2,000-6,000 | an extra outermost world, by hash |
| BRINE DWARF | small, dark rock and ice with bright salt in its craters and one lone ice mountain | 900-2,000 | cratered, icy, 120-260 K |
| FROST DWARF | small, the coldest: bright methane frost, red streaks, almost no air | 600-1,500 | icy, under 45 K |

* **Nitrogen.** Pluto (New Horizons, 2015): Sputnik Planitia's nitrogen ice churns in convection cells 15-40 km across
  bounded by troughs; blocks of water ice float in it and stand as mountains 3 km high; nitrogen glaciers flow from the
  uplands; the dark red tholin lands; bladed methane ice hundreds of metres tall; a blue haze in layers over the limb.
* **Cryovolcanic.** Triton (Voyager 2, 1989): dark plumes rise 8 km and stream sideways in the thin wind, laying dark
  streaks downwind; the "cantaloupe" ground of overlapping dimples; a pink-white frost cap. Event: the plumes (a column,
  then a dark flag drifting with the wind).
* **Dry ice.** Mars' south polar cap: "Swiss cheese" pits in the carbon dioxide ice, araneiform "spiders" of radial
  channels, dark fans of dust laid by the jets that break through the ice each spring (Kieffer 2006). Event: the jets in
  the local spring.
* **Wanderer.** A rogue planet caught by a star passing through it, or by a cluster's tides (Parker and Quanz 2012;
  Perets and Kouwenhoven 2012): far out, eccentric (0.4-0.8), steeply inclined or retrograde. A former world's landforms
  under its fallen air (nitrogen and carbon monoxide snow dunes), cryovolcanic domes, seasons of centuries. The drifters'
  cousin: what a drifter becomes when a star takes it (P-06 of the old list).
* **Brine dwarf.** Ceres (Dawn, 2015-18): dark carbon-rich ground, bright faculae of sodium carbonate on the floors of the
  young craters, Ahuna Mons, a lone ice volcano 4 km high with bright streaks down its flanks.
* **Frost dwarf.** Eris (albedo 0.96) and Makemake: a fresh coat of methane and nitrogen frost, the brightest ground in the
  galaxy, red streaks of tholin, few craters, sublimation pits.

### P-06: the strange moons

| Kind | The data sheet's line | Size km | From |
| --- | --- | --- | --- |
| GROOVED ICE | medium, an icy moon: dark old ground cut by bright lanes of parallel grooves | 1,500-2,700 | icy, large moons of giants |
| TWO-FACED | medium, an icy moon with one face black as coal and one white as snow, a ridge round its equator | 700-1,500 | icy, outer moons of giants |
| TIGER-STRIPED | small, the whitest ice: smooth snow plains, four great rifts at one pole venting plumes | 600-900 | europan, icy, inner moons of giants |
| PATCHWORK | small, broken and put back together: terrains in patches, chasms, cliffs twenty kilometres high | 600-1,500 | icy, rocky, moons |
| SPONGE | small and porous: deep sharp-edged pits like a sponge, dark dust in their floors | 600-900 | cratered, icy, small moons |

* **Grooved ice.** Ganymede: dark, ancient, cratered polygons (the regiones) between bright lanes of grooved ground (the
  sulci, parallel ridges and troughs a few kilometres apart and hundreds of metres high).
* **Two-faced.** Iapetus: its leading face coal-dark, its trailing face bright ice (dark dust swept up, then the ice
  sublimed from the darkened side, a thermal runaway: Spencer and Denk 2010), the boundary ragged, a ridge 10-20 km high
  along much of the equator.
* **Tiger-striped.** Enceladus: the most reflective ground known (plume fallout), smooth young plains, the south polar
  terrain cut by four parallel rifts 130 km long venting curtains of ice. Event: the curtains (R-307's geysers in lines).
* **Patchwork.** Miranda: blocks of unlike terrain fitted together, ovoid coronae ringed by parallel ridges, Verona Rupes,
  a cliff about 20 km high (a fall of minutes in its gravity).
* **Sponge.** Hyperion: a porous body (40% void) whose craters are deep, sharp-rimmed and packed together without ejecta,
  dark material pooled in their floors.

### P-07: the giants (not landable; the probe descends)

| Kind | The data sheet's line | Size km | From |
| --- | --- | --- | --- |
| WATER-CLOUD GIANT | large, warm enough for clouds of water: a bright white globe | 24,000-72,000 | gas giants at 150-250 K |
| AZURE GIANT | large, too warm for clouds: a deep blue globe, faint bands | 24,000-72,000 | gas giants at 350-800 K |
| ALKALI GIANT | large, close to its star: a dark globe of sodium and potassium air, its night side glowing dull red | 60,000-130,000 | gas giants at 800-1,400 K |
| SILICATE GIANT | large and glowing: clouds of rock and iron, iron rain on its night side | 60,000-130,000 | gas giants over 1,400 K |
| GAS DWARF | medium to large: a small world in a deep hydrogen envelope, a pale hazy globe without bands | 10,000-24,000 | small gas giants; the warm zones |
| SUPER-PUFF | very large, a giant of almost no density: a vast pale haze ballooning far over its core | 50,000-110,000 | gas giants of young stars, by hash |

* **The Sudarsky classes** (Sudarsky et al. 2000): a giant's look follows its temperature. Class I, ammonia clouds, is
  today's gas giant (under 150 K); class II, water clouds, is white (Bond albedo about 0.81); class III, too warm for any
  cloud, is a clear blue; class IV, the hot Jupiters, dark (sodium and potassium absorb: TrES-2b reflects under 1%), its
  night side faintly red; class V, over 1,400 K, has silicate and iron clouds and glows (WASP-76b rains iron on its night
  side, Ehrenreich 2020). Today a gas giant forms only under 215 K: the warm and hot classes come with a small share of
  giants in the warm zones (the hot Jupiters round a sun-like star in a hundred), each with few moons. The probe's decks
  follow (X-03's condensation already places them by the adiabat: water high in class II, none in class III, salts and
  silicates in IV and V).
* **Gas dwarf.** The sub-Neptunes, the commonest planets Kepler found (1.7-4 Earth radii, either side of the radius valley:
  Fulton 2017): pale, hazy, almost featureless, few moons.
* **Super-puff.** Kepler-51's three planets: Saturn's size at a few Earth masses (densities under 0.1 g/cm3), flat hazy
  spectra (Libby-Roberts 2020). A soft vast limb, rare, round young stars.

### D: the drifters' own kind

| Kind | The data sheet's line | Size km | From |
| --- | --- | --- | --- |
| DARK OCEAN | medium to large: no sun, kept warm by its own heat under a thick hydrogen sky, a liquid sea in endless night | 5,000-9,000 | drifters only |

* **Dark ocean.** Stevenson (1999): an Earth-mass embryo thrown out of its system young can keep a hydrogen air of 100 to
  10,000 bar; the hydrogen holds in the heat of its own rock, and the surface can be warmer than the melting point of
  water although nothing lights it: oceans like the floor of Earth's. The sea liquid at 280-350 K in the dark, islands of
  black basalt with steam over their vents, a sky with the stars dimmed and reddened. On some, "lights in the water"
  (section 3).

### Left out (and why)

Considered and not kept: an olivine world (an exposed mantle: green plains; a colour more than a world, kept as the
green sands of section 2's list if wanted), a habitable moon (a placement: the felisian moons of giants exist), a
supercritical world apart from the steam world (the same physics; the steam world's lowland fog carries it), the
shepherd moon in a ring (P-11 of the old list: a place, not a kind), contact binaries and ellipsoids (the game's bodies
are spheres), a synestia (it lasts years), a magnetar's world (S-07's star first).

## 2. The qualities (Q): thirty new traits, six kinds, up to six a world

**More of them on every world.** A world carries up to six traits (three today): the three it draws today stay where
they are (the same draw, so nothing a player has seen changes), and three new slots are drawn by hashes of their own, one
from each of three of the new kinds below that fit the world. A fifth of the worlds keep none of the new ones; most get
one or two; a few get three. The data sheet's `TRAITS` takes two lines; the facts (below) a line of their own.

| Kind of trait | The new traits |
| --- | --- |
| Landforms (11) | GREAT SHIELD, TESSERAE, LABYRINTH, HOLLOWS, CRATER CHAINS, PENITENTES, COLUMNAR BASALT, MUD VOLCANOES, INVERTED RIVERS, PINGOS, MAARS |
| Water (4) | GREAT TIDES, HOT SPRINGS, PINK LAKES, MILKY SEAS |
| Sky (5) | NOCTILUCENT CLOUDS, ICE HALOS, AIRGLOW, FOG SEAS, GREAT AURORAE |
| Life (4) | BARRIER REEFS, FUNGAL FORESTS, PETRIFIED FORESTS, ALGAL BLOOMS |
| Ground (3) | RAINBOW STRATA, GYPSUM SANDS, METEORITE FIELDS |
| Sound (3) | SINGING DUNES, SINGING ICE, QUAKES |

* **Great shield.** One shield volcano 12-25 km high and 300-600 km across (Olympus Mons), a caldera complex at the top,
  a cliff ring at its foot; the world's landmark from orbit.
* **Tesserae.** Venus' oldest highlands: plateaus crossed by ridges and troughs in two directions, a woven ground.
* **Labyrinth.** Noctis Labyrinthus, the maze of intersecting fault troughs on Mars' Tharsis rise, 2-6 km deep (and the
  original's name: Noctis).
* **Hollows.** Mercury's bright shallow flat-floored pits where a volatile sublimed out of the rock, clustered on crater
  floors and walls.
* **Crater chains.** Catenae: lines of equal craters where a comet torn apart by a giant's tides struck in sequence
  (Ganymede, Callisto, Shoemaker-Levy 9 on Jupiter).
* **Penitentes.** Blades of ice or snow a metre to five tall in fields facing the sun (the high Andes; Pluto's bladed
  terrain at a hundred times the size on the nitrogen worlds).
* **Columnar basalt.** Hexagonal columns in the cliffs and pavements of thick lava (the Giant's Causeway, Fingal's Cave).
* **Mud volcanoes.** Fields of grey cones a few to fifty metres high that bubble and spill (Azerbaijan; mounds on Mars'
  plains).
* **Inverted rivers.** Old river channels standing as sinuous ridges where the land around them wore away (Mars'
  Aeolis Dorsa): the rivers of a world that had water, upside down.
* **Pingos.** Ice-cored hills 10-60 m high on permafrost plains (the Mackenzie delta).
* **Maars.** Round crater lakes of steam explosions in volcanic fields (the Eifel).
* **Great tides.** A big close moon or a close sun: the sea level follows the moon (W-02 of the old list), a tidal flat
  kilometres wide, the capsule left on the sand in the water when the explorer comes back.
* **Hot springs.** Terraced pools in the colours of their microbes and minerals (Grand Prismatic, Pamukkale's travertine).
* **Pink lakes.** Salt lakes pink to red with salt-loving microbes (Lake Hillier, Lake Retba).
* **Milky seas.** Seas glowing white at night over thousands of square kilometres (bacteria; seen from orbit by satellites,
  Miller 2005).
* **Noctilucent clouds.** Silver-blue clouds high in the air, lit after sunset at high latitudes.
* **Ice halos.** Rings at 22 degrees and sundogs round the sun and the moons from ice crystals in the air.
* **Airglow.** The air's own faint green and red glow over the whole night sky (oxygen's lines at 558 and 630 nm).
* **Fog seas.** Valley fog at dawn, seas of cloud seen from the heights.
* **Great aurorae.** A strong field: aurorae reaching low latitudes most nights (R-402's field set high).
* **Barrier reefs.** Reefs along the shelf edge: a turquoise rim, white sand, a lagoon, seen from orbit.
* **Fungal forests.** The forests pale and capped (the mushroom silhouette, cream, ochre and white canopies).
* **Petrified forests.** Trunks of stone lying in dry ground (Arizona): the forests of a world that dried.
* **Algal blooms.** Seas and lakes streaked red or green with blooms (red tides), seen from orbit.
* **Rainbow strata.** Hills and cliffs banded red, cream, orange and green (Zhangye Danxia, the Painted Hills): O6-04's
  cliff bands coloured by layer.
* **Gypsum sands.** White dunes (White Sands).
* **Meteorite fields.** Iron meteorites strewn over a plain, dark and glinting (Campo del Cielo; Antarctica's blue ice).
* **Singing dunes.** Dunes that boom and hum when their sand slides (the booming dunes of the Namib, the Mojave).
* **Singing ice.** Ice that pings, whoops and sings as it cracks and settles (lake ice; the "singing" of thin ice).
* **Quakes.** The ground heaves every few minutes, the tectonic world's quakes on other worlds.

**The facts** (no draw; read from the system, so no generation change): SIDEWAYS SPIN (a tilt over 60 degrees), RETROGRADE
SPIN, RACING DAY (a day under three hours), LOCKED FACE, ECCENTRIC YEAR (an eccentricity over 0.2), STRONG FIELD and NO
FIELD (R-402's classes), CAPTURED (a moon on a retrograde orbit), TWIN (a double planet), RINGED. The data sheet's
`FACTS` line; the description's sentence reads them already in part.

**The drifters' own traits** (section 3, drawn only on a drifter): FROZEN AIR, EMBERS, LIGHTS LEFT ON, LIGHTS IN THE WATER.

## 3. The drifters (D): rogue planets

**What they are.** A planet with no sun: thrown out of its system young by a giant (the commonest way), torn away by a
passing star, or let go when its star lost mass. Microlensing counts about twenty for every star in the galaxy (Sumi et al.
2023), most of them small and all of them dark. The ones the game holds are the few that call: SIMP 0136, twelve Jupiter
masses with no star twenty light years away, was found by its radio aurorae (Kao et al. 2016, 2018), and the paper said
that radio can find rogue planets. Here it is the only way.

**Where they are.** A population of their own in the galaxy, hashed in large cells with its own salt, placed only in
sectors without a star (so no star or system moves), denser where the stars are (they were thrown out of systems) and in
the old clusters. Fixed in place: they drift at a few tens of kilometres a second, nothing on the scale of the game's light
years. Each knows the star it came from (a real star, a few to tens of light years back along a hashed path) and when it
left.

**What they are made of.** Any kind of world, by the rule of the dark: a drifter's type is drawn from every kind that can
be what it is with no sun (the frozen felisian for a living world, never a living one; the dark ocean; rocky, cratered,
icy and its far-cold kinds, carbon, metal, desert, europan worlds kept warm inside by a moon; a molten or tectonic world
young enough to keep its heat; a gas giant or brown dwarf, the commonest, with its moons). Its warmth is its own: 30-60 K at
the surface of a cold one, more under a moon's tides or a hydrogen sky. Its traits are drawn as any world's, plus one of its
own: FROZEN AIR (its atmosphere fell as nitrogen and oxygen snow), EMBERS (a young interior: lava lakes glowing in the dark),
LIGHTS LEFT ON (a dead people's city still lit), LIGHTS IN THE WATER (a dark ocean's glow, if anything lives there).

**Heard, not seen.** Not in the sky, not on the star map, not on the chart: only the radar finds a drifter, and only above
x1 (x1 stays C-07's list by rule): the first from home at a few hundred light years at the middle gains, several within reach
at x32, none alike. The readout after a lock gives the distance and `NO STAR AT THE SOURCE`, nothing else; Enter flies there.
Then it is a mark: the chart's red ring, the guide's visited place, the explorer's to name.

**The voice.** Each drifter has its own, from its seed and its body, the cosmic horror the user asked for and the radar's
magnetosphere already leans toward, grounded in what real receivers hear from planets: Jupiter's decametric storms (the
slow swishes of L-bursts like surf, the crackle of S-bursts falling in pitch), the kilometric radiation of Saturn and of
Earth's aurorae (eerie whistled howls rising and falling), chorus (flocks of rising chirps), whistlers (falling tones), hiss.
Everything swells and fades with the drifter's turning (the beam sweeping past, one breath every 20-90 s), a motif of its
own recurs each breath (its signature), plasma "vowels" moan through sweeping resonances with no language in them, detuned
drones beat under it. A giant roars; a small world with a moon pulses at the moon's period; a dead people's drifter carries
under it their beacon: the last broadcast, looped for thousands of years, slowed and darkened, its words their tongue's.
Far voices come worn (C-16), and close in the ship hears it whole.

**The dark.** Arriving, the ship is in the void: a black disc where the stars stop, faintly lit on the side toward the
galaxy's bright band, aurorae at its poles, lava glowing on a warm one, the few lights of a dead city. On the ground the
night's light only (the stars, the band; W-04), and the explorer's lamp: a cone of light from the helmet, the capsule's and
the buggy's floodlights, for every night side as well. Frost on everything, seas frozen, no wind, no sound but the suit's
radio carrying the voice.

**The people who lost their sun.** One drifter in four of the frozen felisian and desert kinds kept a people's ruins; their
records tell, and only a drifter's do, how the world lost its sun: by a giant of their own system that came closer each
year until a summer came late and short and the sun began to shrink ("the wanderer": planet-planet scattering, the commonest
cause), by a star that grew brighter for generations until there were two suns and then neither ("the intruder": a flyby in a
cluster), by a dark that bent the stars round a point in the sky and pulled the year apart ("the dark": a black hole or a
neutron star passing), or by the second sun's dance stretching every year further until one let go (a binary's
perturbation). Then the long cold: the year that does not end, the last harvest, the seas freezing, the move below or to the
hot springs, the sun a star among the others; how each age met it (a prehistoric people's myth of the sun walking away and
its fires on the hills; a medieval chronicle of omens and famine; an industrial observatory's table of the sun's diameter
falling year by year; a modern people's broadcast and its shelters; a futuristic people's sunless cities and arks; a far
future people that knew for a thousand years, stayed with its world and left a beacon and the lights on). Their charts mark
the star that was their sun, and Enter flies there.

## 4. How it plugs in

Four readings of the code (2026-10-21) found where each part goes; the iteration file's milestones carry the details.

* **A kind is spread over 25-35 places in ten files** (`PT_EUROPAN` on 34 lines in seven, `PT_DESERT` on 74 in twenty-five),
  half positional tables with no check, the rest switches; the sixteen solid branches of the planet function repeat one
  skeleton. Hence P-00: a row per kind, tags for the groups, traits by tags, the skeleton as a helper and archetypes, accent
  hues for worlds without flora (six families, one hue a bank, every bank of the ground taken), the whole proven by changing
  no picture. Any change to `pickPlanetType`'s weights re-deals every system (the totals, `typeColor`'s extra draw for two
  kinds, the twin's draw, the moons' counts), so the new kinds come from the old draws (P-01), as S-01's varieties came from
  the old star families.
* **A signal can already be heard and locked whatever made it** (the radar is kind-agnostic: a natural voice is synthesised
  from its seed), but the lock, the jump, the arrival, the chart's plot and the save all find the source through the star of
  its sector, and a jump ends at a star by rule (R-407). Hence D-01's resolver and D-02's snap. The magnetosphere's channel
  already whistles, chirps, hisses and roars: D-03 grows from it on indices of its own, so no existing sound moves.
* **No light is not luminosity 0**: the light floors (0.35 for a star's light, 0.62 on the ground) would give a drifter a
  full day, the arrival would put the ship at its centre, the almanac divides by the sun's distance, the landing map goes
  black. Hence `sunless()` with the sun at the nadir (about 110 reads of the sun on the ground then behave as night untouched),
  the map's survey look, the lamp (none exists for a walker; the buggy, the drone and the capsule have lights).
* **A people reads its world as it is now**: on a dead world the plan scores the ice 0 (no people at all), a frozen sea is
  land to every wet test, the records' cold and the instruments read the present, and `historyOfWorld` regenerates a people's
  system from its star (a drifter has none: its texts would vanish without a word). Hence the then-world (P-04) and
  `formerSystemOf` (D-05).

## 5. Sources

* Armstrong et al. 2020, Nature 583: TOI-849b, a remnant planetary core (chthonian).
* Nguyen et al. 2020, MNRAS: K2-141b's rock-vapour atmosphere and magma ocean.
* Kasting 1988, Icarus; Goldblatt et al. 2013, Nature Geoscience: the runaway and moist greenhouse (steam).
* Luger and Barnes 2015, Astrobiology 15: extreme water loss and abiotic O2 on M-dwarf planets (rust).
* Arney et al. 2016, Astrobiology: the pale orange dot, Archean haze; DasSarma and Schwieterman 2018: the purple Earth.
* Heller and Armstrong 2014, Astrobiology 14: superhabitable worlds.
* Abe et al. 2011, Astrobiology 11: habitable zone limits for dry planets (steppe, terminator).
* Pierrehumbert 2011, ApJ Letters: eyeball Earths; Lobo et al. 2023, ApJ: terminator habitability.
* Madhusudhan et al. 2021, ApJ: hycean worlds.
* Kirschvink 1992; Hoffman et al. 1998, Science: Snowball Earth.
* Corliss et al. 1979, Science: hydrothermal vent communities (chemotroph).
* Stern et al. 2015 and McKinnon et al. 2016, Science and Nature: Pluto, Sputnik Planitia's convection; Moores et al. 2017:
  the bladed terrain.
* Soderblom et al. 1990, Science: Triton's plumes.
* Kieffer et al. 2006, Nature: Mars' CO2 jets and spiders.
* De Sanctis et al. 2016, Nature; Ruesch et al. 2016, Science: Ceres' faculae and Ahuna Mons.
* Spencer and Denk 2010, Science: Iapetus' two faces; Porco et al. 2006, Science: Enceladus' plumes.
* Thomas et al. 2007, Nature: Hyperion's porosity.
* Sudarsky et al. 2000 and 2003, ApJ: giant classes I-V; Kipping and Spiegel 2011: TrES-2b; Ehrenreich et al. 2020, Nature:
  WASP-76b's iron rain.
* Fulton et al. 2017, AJ: the radius valley (gas dwarfs); Masuda 2014, Libby-Roberts et al. 2020: Kepler-51's super-puffs.
* Stevenson 1999, Nature 400: life-sustaining planets in interstellar space (dark ocean).
* Sumi et al. 2023, AJ 166: the free-floating planet mass function (about 21 per star).
* Kao et al. 2016 and 2018, ApJ: SIMP J01365663+0933473's radio aurorae.
* Avila et al. 2021, International Journal of Astrobiology: water on exomoons of free-floating planets.
* Parker and Quanz 2012, MNRAS; Perets and Kouwenhoven 2012, ApJ: captured free-floating planets (wanderer).
* Miller et al. 2005, PNAS: a milky sea seen from orbit.
