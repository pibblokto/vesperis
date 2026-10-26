// P-00: the kinds of body. Every kind is a row of KINDS: what it is (its name, size, air and the tags the code paths read), how
// the generator draws it (its weight in each zone of temperature and its answer to every star-class rule, its colour from
// afar; P-01: or, a made kind, the drawn kinds it is made from, `sources`), how `BodyGen::make` makes it (its draws, colours and the relief's grades), how `sampleSurface` shapes its ground (the
// ground function it shares, its crater stack, its plains), how it looks and feels on the ground (the sky, the fog, the
// weather, the temperature and pressure, the boulders, the far floor) and what happens there (its events and sounds). The
// traits declare the processes they need, a kind the processes that shape it (TRAITS, `traitEligible`). A new kind is a row
// and, where no ground function fits, a function of its own (docs/reference/09-cookbook.md). The ids are saved keys (the
// guide's walked kinds, the harness's arguments): a new kind is appended before PT_COUNT, never inserted.
#pragma once
#include "core/types.h"
#include <array>
#include <cstdint>

enum PlanetType {
    PT_MOLTEN = 0,   // internally hot, lava lakes, no atmosphere
    PT_CRATERED,     // small, dusty, craterised, no atmosphere
    PT_VENUSIAN,     // thick atmosphere, fully covered by clouds
    PT_FELISIAN,     // breathable atmosphere, oceans, vegetation
    PT_ROCKY,        // creased rocky surface, boulders, no atmosphere
    PT_THINATMO,     // small, thin atmosphere, exotic colours
    PT_GASGIANT,     // large, not consistent, dense clouds; not landable
    PT_ICY,          // icy, fractured surface, no atmosphere
    PT_QUARTZ,       // milky quartz surface, oxygen atmosphere
    PT_OCEAN,        // M9-11: a global sea with ice floes, no land
    PT_METAL,        // M9-11: dark iron world, glinting, craterised
    PT_VOLCANIC,     // M9-11: tidally heated moon, sulphur plains, active cones
    PT_CARBON,       // M9-11: black graphite plains with diamond crusts
    PT_SUBSTELLAR,   // M5-02: a brown dwarf, glowing with its own heat, with a moon system; not landable
    PT_COMET,        // M5-07: a small icy nucleus on a long eccentric orbit, with a tail near the star; landable since O4 (R-303)
    PT_COMPANION,    // M5-01: a second sun bound to the primary, with worlds of its own
    PT_EUROPAN,      // R-307: an ice shell over a hidden ocean: lineae, chaos terrain, water geysers; no atmosphere
    PT_TECTONIC,     // R-307: a seismic world: rift valleys, fissures running with lava, lava fountains, quakes; a thin sulphurous sky
    PT_DESERT,       // R-307: a dry world under a dusty ochre sky: hamada, mesas, canyons, ergs, salt pans, dust devils
    PT_HYDROCARBON,  // R-307: a cold world under an opaque orange haze: methane seas, tholin dunes, drizzle
    PT_BOMBARDED,    // R-307: a young airless surface under a rain of meteorites: fresh craters, rays, strikes seen from the ground
    PT_ACIDIC,       // R-307: a corrosive world: bleached karst, sulphur crusts, seas of acid, a yellow-green sky that rains
    // P-01 (GEN 16): the furnace worlds, made from the worlds that drew another kind (`kindOf`, the row's `sources`)
    PT_MAGMA_OCEAN,  // locked to its star: a sea of magma round the substellar point, black glass and frost on the night side; no atmosphere
    PT_CHTHONIAN,    // the bare core of a giant that came too close to its star: smooth heavy ground, a thin pale sky, a ring
    PT_GLASS,        // a scorched airless world: plains of black glass, fields of glass beads, tektites
    PT_STEAM,        // past the runaway greenhouse: a white sky of steam, seas boiling at the poles, the lowlands lost in fog
    PT_SALT,         // an ocean that dried away: evaporites in bands round the old basins, brine pools, salt domes and their glaciers
    PT_RUST,         // a red dwarf's world that lost its water: every stone rusted red, dry valleys, a pale oxygen sky
    // P-02: the living worlds, made from the felisian worlds whose day is not their year (and the lichen world from the thin airs)
    PT_ARCHEAN,      // young life: bare grey land, microbial mats on the flats, stromatolites in the shallows, a green sea under an orange haze
    PT_HOTHOUSE,     // no ice at either pole: forest and swamp from coast to pole, warm shallow seas, storms
    PT_SUPERHABITABLE,   // larger, warmer and wetter than a felisian world: shallow seas, a thousand islands, a thick air
    PT_STEPPE,       // a dry living world: grass to the horizon, small seas and salt lakes, forest along the rivers
    PT_TUNDRA,       // an ice age: ice sheets down to the middle latitudes, taiga and tundra below them, sea ice
    PT_LICHEN,       // cold and dry under a thin clear sky: bare rock, crusts of lichen, moss in the hollows
    // P-03: the locked and the deep, made from the red dwarfs' locked worlds, the water worlds, the cold seas and the dead stars' rock
    PT_EYEBALL,      // locked to a red dwarf: frozen over but for one round open sea under the fixed sun
    PT_TERMINATOR,   // locked: a desert under the fixed sun, an ice sheet over the night side, a ring of rivers and life between
    PT_HYCEAN,       // large: one deep warm ocean under a hydrogen sky, no land, a pale hazy horizon
    PT_AMMONIA,      // cold: seas of ammonia water that never freeze, slabs of ice drifting, a yellow-brown haze
    PT_SNOWBALL,     // frozen to the equator: ice sheets and sea glaciers, nunataks, a thin belt of open sea, blizzards
    PT_CHEMOTROPH,   // a dim world warmed from within: hot springs on dark rock, pale mats round them, spores glowing at night
    // P-04: the dead worlds, living worlds that died: their ground the felisian world they were under the present's cover, their peoples' the then-world's
    PT_FROZEN_FELISIAN,     // lost its sun: seas frozen at their level, forests standing dead and white with frost, the air fallen as snow (P-05's wanderers, D's drifters)
    PT_SCORCHED_FELISIAN,   // baked by its swelling sun: seas boiled off to salt, forests turned to stone, cracked clay and dust, a vast red sun
    // P-05: the far cold
    PT_NITROGEN,     // far out: plains of nitrogen ice churned into cells, mountains of water ice afloat in them, dark red tholin lands, a thin blue haze
    PT_CRYOVOLCANIC, // a captured moon: dimpled ground like a melon's skin, a pink frost cap, dark plumes rising kilometres and streaming downwind
    PT_DRY_ICE,      // cold under a thin sky of carbon dioxide: caps of dry ice, pits, spider-shaped channels, dark fans and the jets of spring
    PT_WANDERER,     // a rogue world a star caught on a wild orbit: an old world's ground under its air fallen as snow, domes of ice
    PT_BRINE_DWARF,  // dark rock and ice: bright salt on its young craters' floors, one lone mountain of ice streaked with salt
    PT_FROST_DWARF,  // the coldest: the brightest ground there is, methane frost, red streaks of tholin, sublimation pits
    // P-06: the strange moons, made from the moons of their kinds by what they are among their planet's moons (`MoonFact`)
    PT_GROOVED_ICE,  // a large moon of a giant: dark old polygons between bright lanes of parallel grooves, frost at its poles
    PT_TWO_FACED,    // a giant's outer moon facing its planet: its leading face black as coal, its trailing face white, a ridge along its equator
    PT_TIGER_STRIPED,   // a giant's inner moon: the whitest ice, smooth snow plains, four rifts at its south pole venting curtains of ice
    PT_PATCHWORK,    // a moon in a resonance: terrains in patches, coronae ringed by parallel ridges, chasms, a cliff twenty kilometres high
    PT_SPONGE,       // a small tumbling moon: deep sharp-rimmed pits packed together like a sponge's, dark dust in their floors
    // P-07: the giants, made from the giants that drew a gas giant by their warmth and youth, and from the worlds of the warm orbits (the
    // giants of the warm zones); in `kindOf`'s order: the super-puff first (a young star's giant is swollen whatever its warmth)
    PT_SUPER_PUFF,   // a giant of almost no density round a young star: a vast pale haze ballooning far over its core
    PT_WATER_CLOUD,  // warm enough for clouds of water: a bright white globe, its bands soft (Sudarsky's class II)
    PT_AZURE,        // too warm for any cloud: a deep blue globe, faint bands, its air clear to the deep (class III)
    PT_ALKALI,       // locked close to its star: dark with sodium and potassium in its air, its night side glowing dull red (class IV)
    PT_SILICATE,     // the hottest: clouds of rock and iron, glowing, iron rain on its night side (class V)
    PT_GAS_DWARF,    // a small world in a deep hydrogen envelope: a pale hazy globe without bands
    // D-04: the drifters' own, made from a rocky drifter by the dark's table (`drifterBody`), never by `kindOf`
    PT_DARK_OCEAN,   // no sun, kept warm by its own heat under a thick hydrogen sky: a liquid sea in endless night, islands of black basalt steaming at their vents
    PT_COUNT
};
// P-01: the kinds `pickPlanetType` draws (generation 15's); the later kinds are made from them (`kindOf`), never drawn
constexpr int PT_DRAWN_COUNT = PT_ACIDIC + 1;

enum Material {
    MAT_ROCK = 0, MAT_SAND, MAT_GRASS, MAT_FOREST, MAT_SNOW, MAT_WATER, MAT_LAVA, MAT_ICE, MAT_CLOUD, MAT_QUARTZ,
    MAT_BASALT, MAT_DUST, MAT_GAS, MAT_METAL, MAT_SULPHUR, MAT_GRAPHITE, MAT_SALT,   // R-305: salt (playas, sinter)
    MAT_GLASS,    // S-03: the fused ground of a glassed world (family water: bank 2 in the glass's colour)
    MAT_MUD,      // P-01: the steam world's hot mud (family sand)
    MAT_POTASH,   // P-01: the last salts of a dried sea, red with iron (family snow, the salts'; the salt world's second accent)
    MAT_BRINE,    // P-01: a pool of brine coloured by its minerals, a flat ground like the lava (family water; the salt world's first accent)
    MAT_MATS,     // P-02: microbial mats on the wet flats (family grass; the archean's first accent)
    MAT_STROMATOLITE,   // P-02: a stromatolite reef's flat, the pale carbonate between its domes (family rock; the archean's second accent; the domes, objects, wear the mats)
    MAT_CRUST,    // P-02: lichen crusts on bare ground and stone (family rock; the lichen world's first accent)
    MAT_MOSS,     // P-02: moss mats in the wet hollows (family grass; the lichen world's second accent)
    MAT_COUNT
};
// N1-01: material families share a palette bank in every view (rock, water, forest, grass, sand, snow); P-00: a kind without flora
// may lend the grass and forest families to two accent materials of its own (`KindDef::accent`, `matFamilyOn`)
enum MatFamily { FAM_ROCK = 0, FAM_WATER, FAM_FOREST, FAM_GRASS, FAM_SAND, FAM_SNOW, FAM_COUNT };

// R-304: the traits of a body: up to three, drawn by kind from the seed; each changes the ground, the water, the climate, the flora or
// the colours (`landformsAt`, `BodyGen::make`, `materialPalette`, the flora and the look). The ids are hashed into the draw's order
// (the eligible traits are listed in id order): a new trait is appended before TR_COUNT
enum Trait {
    TR_NONE = 0,
    TR_CANYONS, TR_MESAS, TR_KARST, TR_RIFT, TR_ESCARPMENTS, TR_GLACIAL, TR_INSELBERGS, TR_CINDER_FIELD, TR_CHAOS, TR_POLYGONS,
    TR_YARDANGS, TR_ERG, TR_SALT_FLATS, TR_TRAPS, TR_GREAT_BASIN, TR_CORONAE, TR_SPIRES, TR_GEYSERS,
    TR_ARCHIPELAGO, TR_PANGAEA, TR_LAKELAND, TR_SNOWBALL, TR_EXOTIC_SEAS, TR_STORMS, TR_HAZE,
    TR_GIANT_FLORA, TR_LUMINOUS_FLORA, TR_DEAD_FOREST, TR_RED_SOIL, TR_BLACK_SAND, TR_CHALK,
    TR_GLASSED,   // S-03: never drawn; the generator gives it to a neutron star's outer survivors (`Body::glassed`): the flats fused into sheets of glass
    TR_CIVILISATION,   // C-01: never drawn by the trait draw; `BodyGen::make` gives it to one world in eight of the kinds a people may live on, by a hash of its own (`peoples/ruins.*`; a dry world's old seas and rivers, `oldSeaM`)
    // Q-01: the new slots' traits (`TraitDef::kind`), drawn in the slots 3-5 alone (the first three draw R-304's, so they stay as drawn): the landforms
    TR_GREAT_SHIELD, TR_TESSERAE, TR_LABYRINTH, TR_HOLLOWS, TR_CRATER_CHAINS, TR_PENITENTES, TR_COLUMNS, TR_MUD_VOLCANOES, TR_INVERTED_RIVERS, TR_PINGOS, TR_MAARS,
    // Q-02: the water, the sky, the life and the ground
    TR_GREAT_TIDES, TR_HOT_SPRINGS, TR_PINK_LAKES, TR_MILKY_SEAS,
    TR_NOCTILUCENT, TR_ICE_HALOS, TR_AIRGLOW, TR_FOG_SEAS, TR_GREAT_AURORAE,
    TR_REEFS, TR_FUNGAL, TR_PETRIFIED, TR_BLOOMS,
    TR_STRATA, TR_GYPSUM, TR_METEORITES,
    // Q-03: the sounds of the ground
    TR_SINGING_DUNES, TR_SINGING_ICE, TR_QUAKES,
    // D-04: the drifters' own (`TK_DARK`): never drawn, given to the bodies of a system with no sun by what they are (`darkTraitsOf`)
    TR_FROZEN_AIR, TR_EMBERS, TR_LIGHTS_LEFT_ON, TR_LIGHTS_IN_WATER,
    TR_COUNT
};
constexpr int TRAIT_SLOTS = 6;   // Q-01: a world's traits (`BodyGen::traits`): R-304's three, then three by kind of trait

// N1-01: the material palette's tints, mixed with a body's hue (`materialPalette`; the kinds' overrides in KINDS mix them too)
namespace tint {
constexpr RGB ROCK_GREY(0.50f, 0.46f, 0.42f), BASALT_DARK(0.24f, 0.22f, 0.21f), DUST_WARM(0.62f, 0.56f, 0.48f), SAND_TAN(0.82f, 0.72f, 0.50f);
constexpr RGB SNOW_WHITE(0.92f, 0.93f, 0.97f), ICE_BLUE(0.72f, 0.80f, 0.92f), WATER(0.10f, 0.28f, 0.62f), LAVA(1.0f, 0.5f, 0.1f);
constexpr RGB QUARTZ_PALE(0.90f, 0.86f, 0.90f), METAL_GREY(0.45f, 0.43f, 0.42f), SULPHUR_YELLOW(0.88f, 0.78f, 0.30f), GRAPHITE(0.17f, 0.17f, 0.18f), CLOUD_WHITE(0.95f, 0.95f, 0.97f);
}

// P-00: what a kind is, the groups the code paths read (a new kind takes the bits whose behaviour it shares)
enum KindTag : uint64_t {
    KT_GIANT          = 1ull << 0,    // no surface: a giant's moon table and wider orbit spacing, the probe falls into it (gas giants, brown dwarfs)
    KT_GAS_GIANT      = 1ull << 1,    // a gas giant's globe: the polar vortex, the jets, storms that grow and fade, the night's lightning, its rings
    KT_GLOWS          = 1ull << 2,    // shines with its own heat: its luminosity drawn, its moons warmed, its glow ramps (the brown dwarf)
    KT_OPAQUE_DECK    = 1ull << 3,    // the globe and the sky are one cloud or haze deck: no sun disc, no cast shadows, no aurora
    KT_LAVA           = 1ull << 4,    // bank 2 is lava, the ground glows by it
    KT_PLATES         = 1ull << 5,    // the plates shape the ground (`BodyGen::tectonic`)
    KT_DRAINAGE       = 1ull << 6,    // rivers and lakes from the flood of the ground (galaxy/drainage)
    KT_WET            = 1ull << 7,    // its drainage carries a liquid to lakes (a dry world's leaves wadis and playas)
    KT_FRONTS         = 1ull << 8,    // weather fronts cross it (W-03)
    KT_DUST           = 1ull << 9,    // its fronts are of dust (and the almanac and the globe say so)
    KT_FRONT_RAIN     = 1ull << 10,   // a front rains on it (a front of a dry air is only wind)
    KT_WATER_WEATHER  = 1ull << 11,   // its clouds snow and hail in the cold, fog banks drift over it
    KT_SEA            = 1ull << 12,   // a sea of water: bank 2 is the sea, the swimmers
    KT_FLORA          = 1ull << 13,   // plants: the forest and grass banks are theirs, the biomes, the land animals
    KT_FAUNA          = 1ull << 14,   // animals (the bestiary)
    KT_PEOPLE         = 1ull << 15,   // a people may have lived on it (C-01's hash, one world in eight)
    KT_DRY            = 1ull << 16,   // a dry world: its people's ground is sand, nothing green round their towns
    KT_DEAD_SEAS      = 1ull << 17,   // where a people lived its seas are dry: the old seabed and the wadis that ran to it (C-01)
    KT_TWIN           = 1ull << 18,   // may be half of a double planet
    KT_GLASSABLE      = 1ull << 19,   // a neutron star's blast can fuse its plains to glass (S-03)
    KT_INNER_MOON     = 1ull << 20,   // only a giant's two inner moons are heated enough to be one (an outer one is icy)
    KT_NO_SIGHTS      = 1ull << 21,   // no landmarks (no land, or a nucleus)
    KT_CRYSTAL        = 1ull << 22,   // its ground grows crystal: its spires are crystal, crystal fields are sights, bank 2 a crystal tint
    KT_CLUSTERS       = 1ull << 23,   // clusters of crystals stand among its boulders
    KT_FLAT           = 1ull << 24,   // a flat world: a peak of 50 m over its landmark cell is a sight (90 m elsewhere)
    KT_FROZEN         = 1ull << 25,   // frozen through: its ground is cold everywhere (patterned ground anywhere, not only where it is cold)
    KT_EXOTIC_TWILIGHT = 1ull << 26,  // the sunset takes the sky's own tint
    KT_CLOUD_SWIRL    = 1ull << 27,   // its deck swirls (the cloud pattern's dense swirl, the venusian's)
    KT_MICROBIAL      = 1ull << 28,   // P-02: its life is microbial: the living ground bare, mats on the wet flats, stromatolites in the shallows (the archean)
    KT_ICE_SHEETS     = 1ull << 29,   // P-02: an ice age: the polar ice stands as sheets over the land, the cold flats patterned in polygons (the tundra)
    KT_FIXED_SUN      = 1ull << 30,   // P-03: locked to its star under an air: its climate ordered round the substellar point, its wind steady toward it
    KT_VENT_LIFE      = 1ull << 31,   // P-03: its life lives on the ground's heat: pale mats round the hot springs, sinter chimneys, spores glowing at night
};

// P-00: the processes that shape a kind's ground, and the traits that need them (a trait is eligible where a process it needs acts)
enum KindShape : uint64_t {
    KS_INCISION    = 1ull << 0,    // canyons: flowing liquid or faulting cut into a crust
    KS_LAYERED     = 1ull << 1,    // badlands: layered rock eroded into tables
    KS_SOLUTION    = 1ull << 2,    // karst: the rock dissolves
    KS_EXTENSION   = 1ull << 3,    // a great rift: the crust pulled apart
    KS_SCARPS      = 1ull << 4,    // escarpments: faults and contraction
    KS_GLACIERS    = 1ull << 5,    // glaciers in cold high country
    KS_PEDIMENTS   = 1ull << 6,    // inselbergs: old plains worn round hard hills
    KS_BASALTIC    = 1ull << 7,    // cinder fields: small basaltic volcanoes
    KS_BRITTLE     = 1ull << 8,    // chaos terrain: a crust broken into blocks
    KS_FROST       = 1ull << 9,    // patterned ground: freeze and thaw, or sublimation
    KS_WIND        = 1ull << 10,   // yardangs and dune seas: wind and sand
    KS_EVAPORITES  = 1ull << 11,   // salt flats in closed basins
    KS_FLOOD_LAVA  = 1ull << 12,   // trap terraces of old lava sheets
    KS_IMPACTS     = 1ull << 13,   // one vast basin kept
    KS_PLUMES      = 1ull << 14,   // coronae: mantle plumes under a crust
    KS_SPIRES      = 1ull << 15,   // spires: sublimation pits or crystal growth
    KS_HYDROTHERMAL = 1ull << 16,  // geyser basins
    KS_OPEN_SEA    = 1ull << 17,   // an archipelago, ice to the tropics: a sea round the land
    KS_CONTINENTS  = 1ull << 18,   // one great continent, lake country: the living world's land
    KS_FLORA       = 1ull << 19,   // giant, luminous or dead flora
    KS_COLD_SEAS   = 1ull << 20,   // seas of something that is not water
    KS_STORMS      = 1ull << 21,   // storms that never end
    KS_HAZE        = 1ull << 22,   // a thick haze
    KS_SOILS       = 1ull << 23,   // red soils, black sands, chalk
    KS_FREEZE      = 1ull << 24,   // P-02: a sea that can freeze to the tropics (the snowball; generation 15's kinds with an open sea have it, a hothouse not)
    // Q-01: the new landforms' processes
    KS_HOT_SPOT    = 1ull << 25,   // a great shield: a hot spot under a crust that stays (no plates carry it off), one volcano growing for an age
    KS_FOLDING     = 1ull << 26,   // tesserae: an old crust folded and broken in two directions, its highlands woven
    KS_COLLAPSE    = 1ull << 27,   // a labyrinth: fault troughs over a rise collapsing as the ground's ice went
    KS_VOLATILES   = 1ull << 28,   // hollows: a volatile in the rock sublimes under a hot sun with no air over it
    KS_TIDAL_BREAKUP = 1ull << 29, // crater chains: comets torn apart by a giant's tides strike in lines
    KS_SUBLIMATION = 1ull << 30,   // penitentes: ice sublimes into blades under a strong sun in dry air
    KS_JOINTING    = 1ull << 31,   // columnar basalt: thick lava cooled slowly and cracked into columns
    KS_MUD         = 1ull << 32,   // mud volcanoes: wet sediments and the gas pushed up through them
    KS_EXHUMATION  = 1ull << 33,   // inverted rivers: the riverbeds of a world that had water left standing as the land round them wore away
    KS_PERMAFROST  = 1ull << 34,   // pingos: water freezing in permafrost lifts hills of ice
    KS_PHREATIC    = 1ull << 35,   // maars: magma meets the groundwater and blows round craters
    // Q-02: what the water, the sky, the life and the ground's new traits need
    KS_TIDES       = 1ull << 36,   // great tides: a sea with shores (a moon's or the sun's pull on it is the world's own fact: `Body::tideForcingM`)
    KS_SPRINGS     = 1ull << 37,   // hot springs: groundwater over a heat that brings it up, depositing its minerals in terraces
    KS_HALOPHILES  = 1ull << 38,   // pink lakes: salt lakes in dry basins with life that loves the salt
    KS_GLOWING_SEA = 1ull << 39,   // milky seas: a sea with life that makes light
    KS_MESOSPHERE  = 1ull << 40,   // noctilucent clouds: a cold upper air with water (or carbon dioxide) to freeze in it
    KS_ICE_CRYSTALS = 1ull << 41,  // ice halos: ice crystals in the air under a sun the sky does not hide
    KS_OXYGEN_AIR  = 1ull << 42,   // airglow: an oxygen air whose atoms glow at night
    KS_VALLEY_FOG  = 1ull << 43,   // fog seas: a humid air over valleys, cooling at night
    KS_AURORAE     = 1ull << 44,   // great aurorae: an air the aurora lights (a strong field is the world's own fact: R-402)
    KS_CORALS      = 1ull << 45,   // barrier reefs: warm shelf seas where life builds reefs
    KS_FUNGI       = 1ull << 46,   // fungal forests: forests to be fungi's
    KS_DRIED_FORESTS = 1ull << 47, // petrified forests: a world that dried, its old forests turned to stone
    KS_BLOOMS      = 1ull << 48,   // algal blooms: seas and lakes with the life that blooms in them
    KS_SEDIMENTS   = 1ull << 49,   // rainbow strata: layered sediments of many minerals, oxidised in bands
    KS_GYPSUM      = 1ull << 50,   // gypsum sands: an evaporite basin and the wind that blows its gypsum into dunes
    KS_METEORITES  = 1ull << 51,   // meteorite fields: a stable dry surface that keeps the stones that fall on it
    // Q-03: what the sounds of the ground need
    KS_SINGING_SAND = 1ull << 52,  // singing dunes: dunes of dry, sorted sand with slip faces, and an air to carry their hum
    KS_ICE_SONG    = 1ull << 53,   // singing ice: ice under an air that cracks and settles as the day warms and cools it
    KS_SEISMIC     = 1ull << 54,   // quakes: a crust that still moves (plates, a hot interior, a giant's tides) where the kind has no quakes of its own
};

// P-00: a star-class rule's effect on a kind's weight in `pickPlanetType`, w = w x mul + add ({1, 0} leaves it, {0, 0} rules it out),
// the rules in the order the pick applies them (the dead stars' before each dead class's own; the moons' by their parent; the
// Wolf-Rayet wind after the air it strips, which the pick takes from `atmosphere`; the small worlds' last)
struct WeightOp { double mul = 1, add = 0; };
enum WeightRule { WR_DEAD = 0, WR_PULSAR, WR_NEUTRON, WR_BLACK_HOLE, WR_PROTOSTAR, WR_RED_GIANT, WR_BLUE_GIANT, WR_BLUE_WHITE, WR_ORANGE_GIANT,
                  WR_CARBON_STAR, WR_MOON_OF_WORLD, WR_MOON_OF_GIANT, WR_WOLF_RAYET, WR_SMALL, WR_COUNT };

// P-00: the colour seen from afar (`typeColor`, one draw h): each channel base + var h (or base + var (1 - h)); or, after h, one
// of four colours by a second draw (the thin airs, the gas giants)
struct KindChannel { float base = 0.6f, var = 0; bool flip = false; };
struct KindColour { KindChannel ch[3]; RGB pal[4]; bool palette = false; };

// P-00: `BodyGen::make`'s draws, in the kind's order: the field gets lo + span x uni() (DK_SYM: sym(span); DK_CONST: lo, no draw)
enum GenField { GF_NONE = 0, GF_SEA_LEVEL, GF_MOUNTAIN, GF_HILL, GF_ICE_CAP, GF_SNOW_LINE, GF_MOISTURE, GF_CLOUD_COVER, GF_LAVA_LEVEL,
                GF_VOLCANOES, GF_CRATER_DENSITY, GF_CRATER_DEPTH, GF_BOULDERS, GF_DUNE_AMP, GF_FRACTURE, GF_BAND_COUNT, GF_LIQUID_LEVEL,
                GF_SEA_REACH,     // P-01: a magma ocean's reach from the substellar point (degrees)
                GF_ISLANDS,       // P-02: the volcanic islands' density (a superhabitable world's thousand islands)
                GF_CONT_SCALE };  // P-02: the continents' wavelength factor (a superhabitable world's land broken into archipelagos)
enum DrawKind { DK_UNI = 0, DK_SYM, DK_CONST };
struct GenDraw { int field = GF_NONE; double lo = 0, span = 0; int kind = DK_UNI; };
constexpr int MAX_GEN_DRAWS = 10;

// P-00: a colour `make` sets: a constant, the body's colour scaled per channel, the thin air's sky from the body's colour, or
// drawn per channel c + var uni() (three draws, after the kind's draws)
enum TintMode { TINT_NONE = 0, TINT_CONST, TINT_BODY_SCALED, TINT_THIN_AIR, TINT_DRAWN };
struct KindTint { int mode = TINT_NONE; RGB c, var; };

// P-00: the material palette's overrides (`materialPalette`): the material's colour from a tint c mixed with the body's hue (base),
// c alone, c mixed with the sky tint, the second colour mixed with the hue, c mixed with d, or the hue alone
enum MixMode { MM_BASE_MIX = 0, MM_CONST, MM_SKY_MIX, MM_C2_MIX, MM_MIX, MM_BASE };
struct MatMix { int mat = -1, mode = MM_CONST; RGB c, d; float t = 0; };
constexpr int MAX_MAT_MIXES = 9;   // P-04: a dead world recolours its old seabed, its clay and its stone forests as well

// P-00: the relief spectrum's grades (`make`): the ridge parameter changed (added, scaled or set), the low ground's smoothing set
// (hybrid >= 0), the amplitudes per 16 km of base wavelength on the plains, the hills and the mountains, the cliff bands scaled
enum RidgeOp { RO_NONE = 0, RO_ADD, RO_MUL, RO_SET };
struct KindRelief { int ridgeOp = RO_NONE; double ridge = 0, hybrid = -1, flat = 0.10, hill = 0.5, mtn = 2.6, cliffAmount = 1, cliffStep = 1; };

// P-00: the ground functions of `sampleSurface` (the archetypes a kind shares: its own parameters come from its row and `make`)
enum GroundFn { GR_NONE = 0, GR_FELISIAN, GR_CRATERED, GR_MOLTEN, GR_VENUSIAN, GR_ROCKY, GR_THIN_AIR, GR_ICE, GR_QUARTZ, GR_OCEAN, GR_EUROPAN,
                GR_TECTONIC, GR_DESERT, GR_HYDROCARBON, GR_BOMBARDED, GR_ACIDIC, GR_METAL, GR_VOLCANIC, GR_CARBON, GR_GIANT,
                GR_MAGMA, GR_CHTHONIAN, GR_GLASS, GR_STEAM, GR_SALT,   // P-01: the furnace worlds' own (the rust world's is the desert's)
                GR_LICHEN,   // P-02: the thin air's ground with crusts and moss (the living worlds' others are the felisian's, by their rows)
                GR_EYEBALL, GR_AMMONIA, GR_CHEMOTROPH,   // P-03: the ocean's sea round the substellar point, the methane world's under ammonia water, the tectonic crust's springs
                GR_NITROGEN, GR_CRYOVOLCANIC, GR_DRY_ICE, GR_WANDERER, GR_BRINE, GR_FROST,   // P-05: the far cold's
                GR_GROOVED, GR_TWO_FACED, GR_TIGER, GR_PATCHWORK, GR_SPONGE,   // P-06: the strange moons'
                GR_DARK_OCEAN,   // D-04: a sea over the whole world, islands of black basalt where its volcanoes break the surface
                GR_COUNT };
extern const char* const GROUND_NAMES[GR_COUNT];   // "felisian", "thin air"... (the harness's `kinds <ground>`)

// P-00: one grid of the crater stack (`sampleSurface` sums them in order; the ones marked `sight` are the landmarks' crater grids):
// its cell (km, or a fraction of the radius), its density (absolute, the body's crater density times it, or the secondary field's
// 0.7 + 0.3 x the secondary craters, at most 1), its depth (absolute, or the body's crater depth times it)
enum CraterDensityMode { CD_ABS = 0, CD_REL, CD_SECONDARY };
struct CraterGrid { double cell = 0; bool cellRel = false; double dens = 0; int densMode = CD_ABS; double depth = 0; bool depthRel = false; bool sight = false; };
constexpr int MAX_CRATER_GRIDS = 8;

// P-00: the sky on the ground and from orbit (`typeSky`): from the sky tint (zenith 0.75 of it, horizon half-way to white), a thin
// air's (0.35 and 0.75 of it), or a fixed pair
enum SkyMode { SKY_TINT = 0, SKY_THIN, SKY_FIXED };
// the rain: none, from the clouds' pattern over a threshold (thr, the span to full), or a drizzle now and then (the hydrocarbon's)
enum RainMode { RAIN_NONE = 0, RAIN_CLOUDS, RAIN_DRIZZLE };
// the aurora's colours: the oxygen airs' (green under red or violet), a thin air's, a desert's, a sulphur sky's, an acid sky's
enum AuroraStyle { AU_OXYGEN = 0, AU_THIN, AU_DESERT, AU_SULPHUR, AU_ACID };
// the ground's events (the surface's systems): eruptions round the explorer, fountains from the fissures, meteorite strikes, quakes,
// dust devils (always, or in a dust storm), vents along stained cracks, fumaroles and springs on crusts, a nucleus's jets
enum KindEvent : uint32_t { EV_ERUPTIONS = 1, EV_FOUNTAINS = 2, EV_STRIKES = 4, EV_QUAKES = 8, EV_DEVILS = 16, EV_STORM_DEVILS = 32,
                            EV_CRACK_VENTS = 64, EV_FUMAROLES = 128, EV_COMET_JETS = 256,
                            EV_STEAM_BURSTS = 512,     // P-01: steam bursting now and then from vents on the kind's `eventMaterial` (the steam world's sinter)
                            EV_SPRINGS = 1024,         // P-03: hot springs steaming on the kind's `eventMaterial` (the chemotroph's sinter), spores over them at night
                            EV_PLUMES = 2048,          // P-05: dark plumes kilometres high over the ground's vents (`plumeVents`), their tops streaming downwind as flags
                            EV_SPRING_JETS = 4096,     // P-05: jets of gas and dark dust through the seasonal ice of the spider fields (`springJets`) in the local spring
                            EV_CURTAINS = 8192 };      // P-06: curtains of jets along the tiger stripes (`curtainVents`): R-307's geysers in lines, rising kilometres
// P-01 (GEN 16): where a made kind comes from. `kindOf` makes a world that drew one of the `from` kinds (a bit each) this kind when it
// qualifies (its equilibrium temperature in the band, its body one of `bodies`, its star one of `stars` (a bit each, 0 any), locked to
// its star where `lock` is 1, its day not its year where it is -1) and the kind's hash of the world's seed falls under `share`
enum KindBody : uint32_t { KB_PLANET = 1, KB_COMPANION_WORLD = 2, KB_TWIN = 4, KB_MOON = 8,   // a primary's planet, a companion's, a double planet's second half, a moon
                           KB_CAPTURED = 16 };   // P-05: a giant's captured moon (M9-12's: retrograde, far out, tilted), made only by the kinds that name it
// P-06: what a moon is among its planet's moons (`KindFacts::moon`, read from the draws as drawn): one of its planet's two inner moons (the
// tides that heat them, M9-12's), its fourth or further, its day its month (one face to its planet) or not, its period in a resonance with
// the moon inside it (M9-12's 2:1 or 3:2), 1,500 km or more or under 1,100 as drawn. A source's `moon` needs every fact it names
enum MoonFact : uint32_t { MF_INNER = 1, MF_OUTER = 2, MF_FACING = 4, MF_TURNING = 8, MF_RESONANT = 16, MF_LARGE = 32, MF_SMALL = 64 };
// P-02: `byStar`: the hash of the star's seed, not the world's, so every world of a star that qualifies is made alike (a young star's)
// P-03: `parents`: a moon's source, the kinds its planet drew (a bit each, 0 any: the brown dwarfs' moons)
struct KindSource { uint32_t from = 0; double tLo = 0, tHi = 1e9; uint32_t bodies = 0; uint32_t stars = 0; int lock = 0; double share = 0; bool byStar = false; uint32_t parents = 0; uint32_t moon = 0; };
constexpr int MAX_KIND_SOURCES = 3;
// the sentence of `describeBody`: the first phrase, the first or the second by a coin, the first or the second by the body's parameter
// (craters, dune seas, fractures), the felisian seas' three, the dead seas' own or the coin
enum SentenceRule { SR_ONE = 0, SR_COIN, SR_CRATERS, SR_DUNES, SR_FRACTURES, SR_SEAS, SR_DEAD_SEAS };

// P-02: the land animals of a kind with fauna (`Bestiary::make`): the felisian's, cold grazers (big, woolly, in herds on the tundra, the
// taiga and the cold grass), the herds of the open grass (grazers in big herds on the grassland and the savanna)
enum FaunaStyle { FA_DEFAULT = 0, FA_COLD, FA_HERDS };
// P-04: the cover the present laid over a dead world's ground (`felisianSample`'s end): none; frozen (the seas ice at their level, the land under
// frost, the forests dead and white); scorched (the seas' beds dry: salt, silt and reef limestone; the soils cracked clay and dust, the forests stone)
enum DeadCover { DC_NONE = 0, DC_FROZEN, DC_SCORCHED };
// P-07: a made giant's class (none: the gas giant and the brown dwarf, X-03's): Sudarsky et al. (2000)'s four by warmth, the gas dwarf and the
// super-puff. It sets the globe's looks (space_view: the ramp, the night's glow, the haze at the limb), the bands' character (`giantGlobe`), the
// probe's air and decks (galaxy/probe) and the descent's light (game/probe)
enum GiantClass { GC_NONE = 0, GC_WATER, GC_AZURE, GC_ALKALI, GC_SILICATE, GC_DWARF, GC_PUFF, GC_COUNT };

struct KindDef {
    int id = -1;
    // identity
    const char* name = nullptr;          // the data sheet's type ("THIN ATMOSPHERE"); unique, safe as a file name
    const char* shortName = nullptr;     // ten characters at most (the system list, the statistics)
    const char* family = nullptr;        // P-01: the made kinds' family ("furnace"): the milestone that made them (`vesperis_test kinds furnace`)
    const char* description = nullptr;   // the data sheet's line
    const char* sentence[3] = {nullptr, nullptr, nullptr};   // `describeBody`'s phrases
    int sentenceRule = SR_ONE;
    double minRadiusKm = 0, maxRadiusKm = 0;
    bool landable = false, atmosphere = false;
    int maxMoons = 0;
    double albedo = 0.3;
    double density = 1.0;                // relative to rock (the mass from the radius)
    double ringChance = 0.12;            // the chance a planet of the kind wears rings
    uint64_t tags = 0;
    uint64_t shapes = 0;
    // generation
    double zones[5] = {0, 0, 0, 0, 0};   // the weights over 600 K, 345-600, 205-345, 120-205, under 120
    WeightOp rules[WR_COUNT];
    KindSource sources[MAX_KIND_SOURCES];   // P-01: a made kind's (a drawn kind has none and a made one no weight)
    int nSources = 0;
    uint64_t salt = 0;                   // P-01: a made kind's hash of the world's seed (its own, so each kind's share is apart)
    bool untilted = false;               // P-01: the tides that locked it took its tilt (its substellar point stays put: the magma ocean's sea under its sun)
    KindColour colour;
    // the body (`BodyGen::make`)
    double greenhouseK = 0;              // its air's warming over the equilibrium temperature (the climate)
    double latGradient = 1;              // P-02: the living climate's contrast from the equator to the poles (Earth's 46 degrees x it: a hothouse's is flat)
    double seaDepth = 1;                 // P-02: the living ground's seas' depth x it (a superhabitable world's shelf seas)
    double daySwing = 10;                // P-02: the living ground's day-night swing (degrees: a steppe's is wide, a hothouse's narrow)
    double grassBand = 0;                // P-02: the living biomes' grass widened by this much of moisture on both sides (a steppe's: forest only along the rivers)
    int floraFamily = -1;                // P-02: the silhouette its flora's first family always takes (the tundra's conifers); -1 hashed
    int favourTrait = TR_NONE;           // P-02: a trait it takes more often (the hothouse's giant flora): added to a free slot by a hash under `favourChance`
    double favourChance = 0;
    int fauna = FA_DEFAULT;              // P-02: its land animals' style (`FaunaStyle`)
    // P-03: a locked living world's climate (M5-08's on the felisian layer: the day side `subContrast` degrees warmer at the substellar point,
    // the night side as much colder, its moisture taken under the sun); a KT_FIXED_SUN kind's own: the moisture by the climate at the sea's
    // level (the ring `ringWet` wetter round 8 C, the day side `dayDry` drier from 15 C to 45), the seas dry where it is too hot to hold them
    // (`daySeasDry`), an ice sheet over the cold side (KT_ICE_SHEETS with `sheetByCold`: its edge where that climate falls under -4 C)
    double subContrast = 45, dayDry = 0, ringWet = 0;
    bool daySeasDry = false, sheetByCold = false;
    bool nunataks = false;               // P-03: an ice sheet with a smooth surface over the continents' base: the ranges stand out of it as rock (the snowball's)
    double sheetM = 2200;                // P-03: an ice sheet's thickness where it is full (P-02's tundra's 2.2 km; the snowball's thinner)
    bool seaGlaciers = false;            // P-03: its frozen seas glacier ice tens of metres over the water, ending in a cliff, a band of floes before it (the snowball's)
    bool reachClimate = false;           // P-03: the temperature on the ground by the fixed sun's height, `tempOffset` at the edge of the substellar sea (the eyeball's)
    bool evenClimate = false;            // P-03: its air carries the day round to the night: no night-side ice on a locked world (a hycean's hydrogen)
    // P-04: a dead world: the living kind it was in its people's time (its then-world: `thenWorld`; its row's draws and relief that kind's, so
    // the ground is the same in both times), the present's cover over it, its peoples by a hash of their own (`peoplesHash`: C-01's never
    // fires on a made world) and a kind made by no source of `kindOf` (`elsewhere`: P-05's wanderers, made by their own step of `generate`,
    // and D's drifters)
    int thenKind = -1;
    int cover = DC_NONE;
    double peopleShare = 0;
    uint64_t peopleSalt = 0;
    bool elsewhere = false;
    // P-07: a made giant's class, and its temperature at one bar across a band of equilibrium temperature (the game's scale: the classes take
    // the warmest giants the game has; 0 the gas giant's rule, the equilibrium's 1.5 warmed by its heat)
    int giantClass = GC_NONE;
    double oneBarTLo = 0, oneBarTHi = 0, oneBarLo = 0, oneBarHi = 0;
    // D-04: in the dark (a body of a system with no sun) its air falls as snow under this temperature (a carbon dioxide air's 150 K, Titan's
    // nitrogen holds at 94: 60; a hydrogen air never: 0) and its sea freezes at its level under this one (water's 271 K, ammonia water's 176,
    // methane's and the exotic liquids' 90)
    double airFreezeK = 150, seaFreezeK = 271;
    double starLight = 1;                // D-04: the share of the stars' light its air lets through to the ground at night (the dark ocean's hydrogen dims and reddens them)
    GenDraw draws[MAX_GEN_DRAWS];
    int nDraws = 0;
    KindTint color2, skyTint;
    KindRelief relief;
    MatMix palette[MAX_MAT_MIXES];
    int nPalette = 0;
    int famRep[FAM_COUNT] = {MAT_ROCK, MAT_WATER, MAT_FOREST, MAT_GRASS, MAT_DUST, MAT_ICE};   // the material that stands for each family
    int accent[2] = {-1, -1};            // a kind without flora: the materials that take the grass and the forest families (-1 none)
    double exoticLo = -250, exoticSpan = -500;   // TR_EXOTIC_SEAS: the liquid's level lo + span u, and its colour
    RGB exoticColour = RGB(0.42f, 0.28f, 0.08f);
    // the ground
    int ground = GR_NONE;
    double plainK = 1.0, gullyK = 0.6;   // the plains' small relief and its gullies (O6-04)
    double drainArea = 1.0;              // the channels' area factor (a dry world's rivers need more ground)
    CraterGrid craters[MAX_CRATER_GRIDS];
    int nCraters = 0;
    // the look on the ground
    int sky = SKY_TINT;
    RGB zenith, horizon;                 // SKY_FIXED's pair
    bool seaSky = false;                 // the ground's own sky from the tint (0.8 of it, the horizon half-way to white: the ocean's)
    bool lavaHorizon = false;            // an airless horizon lit by the ground (`horizonGlow`)
    RGB horizonGlow;
    double fog = 6000, skyDensity = 1.0;
    double lowFogM = -1e9, lowFogSpan = 1;   // P-01: under this height the fog closes, over the span above it (the steam world's lowlands); -1e9 none
    double limbHaze = 0;                 // P-05: a band of haze on the horizon over a dark sky by day (its shade in the sky's bank: Pluto's blue layers)
    bool clouds = false;                 // a cloud layer over the ground
    bool ownNight = false;               // its own night sky (else the default's)
    RGB nightZenith, nightHorizon;
    int floorFamily = FAM_ROCK;          // the far floor's family (its bank)
    double rockDensity = 0, rockScale = 1.0;   // the boulders
    double snowAlbedo = 0.9;             // the snow's albedo on the ground
    int aurora = AU_OXYGEN;
    // the air on the ground
    double pressureLo = 0, pressureSpan = 0;   // atmospheres: lo + span x a hash
    double tempOffset = 6, tempSwing = 12;     // with air: the greenhouse and the day-night swing
    double tempAdd = 0;                  // the ground's own heat
    double landMoist = -1;               // Q-01: off the living ground, the mean moisture its landforms read (the living ground's field round it); -1 a dry world's 0.15
    double nightSideC = -1e9;            // P-01: an airless world locked to its star: its night side's temperature (the magma ocean's); -1e9 the airless rule's
    double wind = 10;                    // the base wind in knots with air
    int rain = RAIN_NONE;
    double rainThr = 0, rainSpan = 1;
    double dustThreshold = 0;            // dust storms: the storm noise over it raises the dust (0: none)
    uint32_t events = 0;
    int eventMaterial = -1;              // the ground its event stands on (a fissure's lava, a crack's stain): the harness's sites (`kinds`)
    double rumble = 0, rumbleQuake = 0;  // the lava's sound: its level, and what a quake adds
};

extern const std::array<KindDef, PT_COUNT> KINDS;
inline bool kindHas(int type, uint64_t tag) { return (KINDS[type].tags & tag) != 0; }
inline bool hasOpaqueDeck(int t) { return kindHas(t, KT_OPAQUE_DECK); }   // the globe and the sky are one cloud or haze deck; no sun disc, no cast shadows
inline bool isLavaWorld(int t) { return kindHas(t, KT_LAVA); }            // bank 2 is lava, the ground glows by it, eruptions
inline bool livingGround(int t) { return KINDS[t].ground == GR_FELISIAN && KINDS[t].cover == DC_NONE; }   // the living world's ground: its climate, biomes, snow and seas (P-04: a dead world's lies under its cover)
inline bool deadKind(int t) { return KINDS[t].thenKind >= 0; }                                   // P-04: a living world that died
inline int livingKindOf(int t) { return KINDS[t].thenKind >= 0 ? KINDS[t].thenKind : t; }     // P-04: the kind a world was in its people's time
// P-03: a made kind of the locked: every source of it needs its world locked to its star (the magma ocean, the eyeball, the terminator)
inline bool lockedKind(int t) { const KindDef& K = KINDS[t]; if (K.nSources == 0) return false; for (int i = 0; i < K.nSources; i++) if (K.sources[i].lock <= 0) return false; return true; }
int landableTypeCount();

// P-00: the traits as rows: the name, the description's phrase, the class (a landform, a world trait, or given by the generator and
// never drawn), the processes it needs (any of them), one colour trait at most, a trait it never comes with
enum TraitClass { TC_LANDFORM = 0, TC_WORLD, TC_GIVEN };
// Q-01: the kinds of trait the three new slots draw from, one slot a kind (R-304's traits are TK_FIRST: the first three slots draw them alone);
// `moves`: the trait moves the ground or what stands on it, so a world with a people never takes it (its towns stand where 1.3.2 put them)
// D-04: TK_DARK, the drifters' own: never drawn into a slot by a hash, given by what a body in the dark is (`darkTraitsOf`)
enum TraitKind { TK_FIRST = 0, TK_LANDFORM, TK_WATER, TK_SKY, TK_LIFE, TK_GROUND, TK_SOUND, TK_DARK, TK_COUNT };
struct TraitDef { int id = -1; const char* name = ""; const char* phrase = ""; int cls = TC_GIVEN; uint64_t needs = 0; bool colour = false; int excludes = TR_NONE; int kind = TK_FIRST; bool moves = false; };
extern const std::array<TraitDef, TR_COUNT> TRAITS;
inline bool traitEligible(int type, int trait) { return TRAITS[trait].cls != TC_GIVEN && (KINDS[type].shapes & TRAITS[trait].needs) != 0; }
