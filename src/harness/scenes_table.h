#pragma once
// E-03: the scenes, `scene <name>` and the battery's `scenes`: a row a scene, its type, latitude, height,
// view and code (setupSceneForType places it)
#include "harness/harness.h"

struct CmpScene { const char* name; int type; double latDeg, alt, yawOff, pitch; int wantMat = -1; };
static const CmpScene CMP_SCENES[] = {
    {"felisian_forest", PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, SCENE_FOREST},
    {"biome_tropical", PT_FELISIAN, 0, 40 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_TROPICAL},
    {"biome_savanna", PT_FELISIAN, 5, 40 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_SAVANNA},
    {"biome_desert", PT_FELISIAN, 10, 40 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_DESERT},
    {"biome_temperate", PT_FELISIAN, 25, 35 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_TEMPERATE},
    {"biome_grassland", PT_FELISIAN, 20, 35 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_GRASSLAND},
    {"biome_taiga", PT_FELISIAN, 45, 25 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_TAIGA},
    {"biome_tundra", PT_FELISIAN, 55, 15 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_TUNDRA},
    {"biome_wetland", PT_FELISIAN, 5, 30 * DEG, 0.5, -0.03, SCENE_BIOME - BIO_WETLAND},
    {"tundra_lowsun", PT_FELISIAN, 55, 5 * DEG, 0.0, 0.05, SCENE_BIOME - BIO_TUNDRA},
    {"felisian_herd", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, SCENE_HERD},
    {"felisian_herd_near", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, SCENE_HERD_NEAR},
    {"felisian_herd_far", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, SCENE_HERD_FAR},
    {"creatures_lineup", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, SCENE_CREATURES},
    {"buggy_seat", PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, SCENE_BUGGY_SEAT},
    {"buggy_parked", PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, SCENE_BUGGY_PARKED},
    {"felisian_shore", PT_FELISIAN, 12, 30 * DEG, 0.0, 0.0, MAT_SAND},
    {"felisian_river", PT_FELISIAN, 10, 40 * DEG, 0.0, -0.03, SCENE_RIVER},   // B-320: on the bank of a river above the sea, looking across it
    {"felisian_lake", PT_FELISIAN, 10, 40 * DEG, 0.0, -0.03, SCENE_LAKE},    // on a lake shore
    // the six new types, each at its signature
    {"europan_crack", PT_EUROPAN, 20, 30 * DEG, 0.6, -0.02, SCENE_STAINED_CRACK},
    {"tectonic_fissure", PT_TECTONIC, 15, 35 * DEG, 0.6, -0.03, SCENE_LAVA_FISSURE},
    {"desert_erg", PT_DESERT, 15, 40 * DEG, 0.5, -0.02, SCENE_DUNES},
    {"hydrocarbon_shore", PT_HYDROCARBON, 10, 30 * DEG, 0.0, -0.02, SCENE_METHANE_SHORE},
    {"bombarded_plain", PT_BOMBARDED, 15, 35 * DEG, 0.5, -0.02, SCENE_RAYED_PLAIN},
    {"acidic_shore", PT_ACIDIC, 15, 40 * DEG, 0.0, -0.02, SCENE_ACID_SHORE},
    // the furnace worlds, each at its signature: 60-150 m from the magma sea where the fixed sun stands 22 degrees up, facing it; a
    // chthonian plain; by a glass flow, facing it; by a field of steam vents on the highlands; by a brine pool on a salt floor; across a dry valley
    {"magma_shore", PT_MAGMA_OCEAN, 15, 22 * DEG, 0.0, -0.03, SCENE_MAGMA_SHORE},
    {"chthonian_plain", PT_CHTHONIAN, 12, 35 * DEG, 0.5, -0.02},
    {"glass_flow", PT_GLASS, 15, 30 * DEG, 0.0, -0.04, SCENE_GLASS_FLOW},
    {"steam_highland", PT_STEAM, 15, 40 * DEG, 0.0, -0.02, SCENE_STEAM_VENT},
    {"salt_basin", PT_SALT, 15, 40 * DEG, 0.0, -0.04, SCENE_BRINE_POOL},
    {"rust_valley", PT_RUST, 12, 35 * DEG, 0.0, -0.06, SCENE_RUST_VALLEY},
    // the living worlds, each at its signature: on an archean reef flat among the stromatolites, facing the green sea; in a hothouse's
    // swamp past fifty degrees of latitude; on a superhabitable world's beach over its shelf sea; on a steppe's grass by a herd; on a tundra
    // half a kilometre from the ice sheet's wall, facing it; by the moss of a lichen world's hollow, facing it
    {"archean_flats", PT_ARCHEAN, 8, 30 * DEG, 0.0, -0.06, SCENE_ARCHEAN_FLATS},
    {"hothouse_swamp", PT_HOTHOUSE, 50, 30 * DEG, 0.5, -0.03, SCENE_HOTHOUSE_SWAMP},
    {"superhabitable_shelf", PT_SUPERHABITABLE, 8, 40 * DEG, 0.0, -0.02, MAT_SAND},
    {"steppe_plain", PT_STEPPE, 12, 35 * DEG, 0.6, 0.0, SCENE_STEPPE},
    {"tundra_ice", PT_TUNDRA, 34, 30 * DEG, 0.0, 0.03, SCENE_TUNDRA_ICE},
    {"lichen_hollow", PT_LICHEN, 15, 25 * DEG, 0.0, -0.08, SCENE_LICHEN},
    // the locked and the deep, each at its signature: on an eyeball's ice front over its floes and the open sea, the fixed sun ahead (the
    // light its height there); in a terminator world's ring on grass by its forest, the fixed sun six degrees up; on a hycean's sea in a clear
    // hour under its haze; on an ammonia sea's shore by a slab of ice; on a snowball's ice over the belt of open sea at an equinox; by a
    // chemotroph's spring at dusk, its steam and its spores rising
    {"eyeball_shore", PT_EYEBALL, 0, 30 * DEG, 0.0, -0.05, SCENE_EYEBALL_SHORE},
    {"terminator_ring", PT_TERMINATOR, 10, 6 * DEG, 0.0, 0.0, SCENE_TERMINATOR_RING},
    {"hycean_sea", PT_HYCEAN, 12, 35 * DEG, 0.6, 0.02, SCENE_HYCEAN_SEA},
    {"ammonia_shore", PT_AMMONIA, 10, 30 * DEG, 0.0, -0.03, SCENE_AMMONIA_SHORE},
    {"snowball_belt", PT_SNOWBALL, 0, 30 * DEG, 0.0, -0.04, SCENE_SNOWBALL_BELT},
    {"chemotroph_vents", PT_CHEMOTROPH, 10, -5 * DEG, 0.0, -0.03, SCENE_CHEMOTROPH_VENTS},
    // the dead worlds: a frozen world's harbour (the kind staged on a felisian world of the scan with a people: a drifter's of D-05), from the quay
    // over the sea's ice, the sun low; on a scorched world's old seabed 600 m out from the old shore, facing it
    {"frozen_felisian_shore", PT_FROZEN_FELISIAN, 12, 25 * DEG, 0.0, 0.02, SCENE_HARBOUR},
    {"scorched_felisian_seabed", PT_SCORCHED_FELISIAN, 10, 35 * DEG, 0.0, 0.02, SCENE_SCORCHED_SEABED},
    {"frozen_felisian_forest", PT_FROZEN_FELISIAN, 20, 25 * DEG, 0.0, 0.02, SCENE_DEAD_FOREST},     // in their old forests: frozen standing dead and white,
    {"scorched_felisian_forest", PT_SCORCHED_FELISIAN, 10, 35 * DEG, 0.0, 0.0, SCENE_DEAD_FOREST},   // scorched turned to stone
    // the far cold: on a nitrogen plain among its cells, a block of water ice ahead, the sun low; 25 km across the wind from a cryovolcanic
    // world's plume, its flag over the cap; by a spider on a dry ice world's spring frost as its jet breaks through; on a wanderer's snow, the old
    // world's rock standing out of it; on a brine dwarf's dark crater floor before its faculae; on a frost dwarf's frost before a streak of tholin
    {"nitrogen_cells", PT_NITROGEN, 0, 12 * DEG, 0.0, -0.05, SCENE_NITROGEN_CELLS},
    {"cryovolcanic_plume", PT_CRYOVOLCANIC, -40, 25 * DEG, 0.0, 0.14, SCENE_CRYO_PLUME},
    {"dry_ice_spiders", PT_DRY_ICE, 40, 25 * DEG, 0.0, -0.06, SCENE_SPIDERS},
    {"wanderer_snow", PT_WANDERER, 10, 20 * DEG, 0.0, 0.02, SCENE_WANDERER_SNOW},
    {"brine_faculae", PT_BRINE_DWARF, 10, 30 * DEG, 0.0, -0.03, SCENE_BRINE_FACULAE},
    {"frost_dwarf", PT_FROST_DWARF, 10, 18 * DEG, 0.0, -0.12, SCENE_FROST_DWARF},
    {"brine_mountain", PT_BRINE_DWARF, 0, 25 * DEG, 0.0, 0.04, SCENE_BRINE_MOUNTAIN},   // the lone mountain of ice, its flanks streaked with salt, from 25 km
    // the strange moons: on a grooved moon's dark ground at a lane's edge, facing across its grooves; on a two-faced moon's bright ice before
    // its dark face, and its equatorial ridge from 35 km; 3 km from a tiger stripe, its jets and the curtains over it, the sun low; the patchwork's
    // great cliff from its corona's floor; from the rim of a sponge's pit down to its dark floor
    {"grooved_lanes", PT_GROOVED_ICE, 10, 25 * DEG, 0.0, -0.02, SCENE_GROOVED_LANES},
    {"two_faced_boundary", PT_TWO_FACED, 10, 20 * DEG, 0.0, -0.03, SCENE_TWO_FACED_BOUNDARY},
    {"two_faced_ridge", PT_TWO_FACED, 0, 25 * DEG, 0.0, 0.08, SCENE_TWO_FACED_RIDGE},
    {"tiger_stripes", PT_TIGER_STRIPED, -70, 8 * DEG, 0.0, 0.10, SCENE_TIGER_STRIPES},
    {"patchwork_cliff", PT_PATCHWORK, 0, 25 * DEG, 0.0, 0.10, SCENE_PATCHWORK_CLIFF},
    {"sponge_pits", PT_SPONGE, 10, 30 * DEG, 0.0, -0.25, SCENE_SPONGE_PITS},
    // the sounds of the ground, each on the first world of the scan with the trait under an air of a third of a bar or more (the sound carried
    // whole), each heard (`scene_<name>.wav`, the cue's sound alone as the game sets it): at the top of a slip face of the singing dunes (no haze
    // over them), looking down it, then walking two and a half seconds down it; on the singing ice with the sun four degrees up (the day warming or cooling it fast), twenty
    // seconds of it; on the ground of a world with quakes as one strikes (the frame a second and a half in), twelve seconds of it
    {"singing_dunes", -1, 10, 35 * DEG, 0.0, -0.3, SCENE_SINGING_DUNES},
    {"singing_ice", -1, 30, 4 * DEG, 0.0, 0.0, SCENE_SINGING_ICE},
    {"quakes", -1, 10, 30 * DEG, 0.0, 0.0, SCENE_QUAKES},
    {"geyser_basin", -1, 10, 40 * DEG, 0.0, 0.0, SCENE_GEYSERS},   // 150 m from a geyser basin's vent on any world with the trait, facing it; `warm` waits for its jet
    // the ground in the dark, each on the nearest drifter from home that shows it (`setupDarkScene`): a rocky drifter under the explorer's
    // lamp (and `scene_rogue_lamp_off.png` without it), a young interior's lava lake, an air fallen as snow, a dark ocean's shore
    {"rogue_lamp", -1, 20, 0, 0.0, -0.10, SCENE_ROGUE_LAMP},
    {"rogue_embers", -1, 0, 0, 0.0, -0.04, SCENE_ROGUE_EMBERS},
    {"rogue_frozen_air", -1, 20, 0, 0.0, -0.06, SCENE_ROGUE_FROZEN_AIR},
    {"dark_ocean", -1, 0, 0, 0.0, -0.09, SCENE_DARK_OCEAN},
    {"felisian_sunset", PT_FELISIAN, 12, 4 * DEG, 0.0, 0.05},
    {"felisian_mountains", PT_FELISIAN, 0, 40 * DEG, 0.0, 0.02, SCENE_PINNED_MOUNTAIN},   // the pinned mountain review site of O6, facing its highest ground
    // the fronts' site (`PIN_FRONT_*`, flat land under a clear sky) facing the next front when its deck's solid wall is 18 km off
    {"front_far", PT_FELISIAN, 0, 40 * DEG, 0.0, 0.03, SCENE_FRONT_FAR},
    {"front_arriving", PT_FELISIAN, 0, 40 * DEG, 0.0, 0.03, SCENE_FRONT_ARRIVING},   // four minutes before: the wall over the near ground, the wind up, the light down
    {"front_through", PT_FELISIAN, 0, 40 * DEG, 0.0, 0.03, SCENE_FRONT_THROUGH},    // at the rain's peak, 25 km behind the line
    {"front_behind", PT_FELISIAN, 0, 40 * DEG, 0.0, 0.03, SCENE_FRONT_BEHIND},     // an hour after the rain's end, facing where it came from: the sky clearing behind it, the cold
    // the nights (each searched for near home in a fixed order and printed): a shower at its peak under its radiant, the
    // zodiacal cone over where the sun set, a crescent moon with the earthshine on the rest of its disc, a moon's ground under its
    // full parent at midnight with the hills' shadows
    {"meteor_shower", -1, 0, 0, 0.0, 0.0, SCENE_METEOR_SHOWER},
    {"zodiacal_light", -1, 0, 0, 0.0, 0.0, SCENE_ZODIACAL_LIGHT},
    {"earthshine", -1, 0, 0, 0.0, 0.0, SCENE_EARTHSHINE},
    {"moonlit_night", -1, 0, 0, 0.0, 0.0, SCENE_MOONLIT},
    {"cratered_noon", PT_CRATERED, 12, 45 * DEG, 0.0, 0.0},
    {"thinatmo_sunset", PT_THINATMO, 12, 4 * DEG, 0.0, 0.05},
    {"molten_night", PT_MOLTEN, 12, -40 * DEG, 0.5, 0.15},
    {"icy_noon", PT_ICY, 12, 40 * DEG, 0.3, 0.05},
    {"comet_day", PT_COMET, 10, 40 * DEG, 0.4, 0.05},       // the nucleus by day, jets on the sunlit ground
    {"comet_night", PT_COMET, 10, -30 * DEG, PI, 0.35},     // and at night, looking away from the sun: the tail overhead
    // the aurora: a thin-atmosphere world round a pulsar or a blue giant, 64 degrees of latitude, the darkest hour,
    // facing the pole (the sun's azimuth at its lowest), and the same looking straight up
    {"thinatmo_aurora", PT_THINATMO, 58, -30 * DEG, 0.0, 0.25, SCENE_AURORA},
    {"thinatmo_aurora_zenith", PT_THINATMO, 64, -30 * DEG, 0.0, 1.5, SCENE_AURORA_ZENITH},
    // any clear-air world with a moon or the parent in the poleward sky at the curtains' height, facing it: its night side shows the
    // curtains in front
    {"aurora_moon", -1, 64, -30 * DEG, 0.0, 0.25, SCENE_AURORA_CURTAIN},
    {"neutron_glass", -1, 15, 25 * DEG, 0.6, -0.03, SCENE_NEUTRON_GLASS},   // standing on a glass sheet of a neutron star's glassed world, the star low and off to the side for its glints
    // any world of a protostar, the star 12 degrees under the horizon, facing it: the cloud's glow and the disc's band over the horizon
    {"protostar_night", -1, 20, -12 * DEG, 0.0, 0.12, SCENE_PROTOSTAR},
    // any world of a Wolf-Rayet star at the type's default spot, the star 38 degrees up and 29 off the view, looking 20 up: the shell's
    // ring round the blinding sun over a bare plain
    {"wolf_rayet_day", -1, 10, 38 * DEG, 0.5, 0.35, SCENE_WOLF_RAYET},
    // any world of a black hole at the type's default spot, the hole 30 degrees up and 23 off the view, looking 17 up: the shadow, the disc
    // and the bent stars over a bare plain
    {"black_hole_sky", -1, 10, 30 * DEG, 0.4, 0.3, SCENE_BLACK_HOLE},
    {"civilisation_town", PT_FELISIAN, 12, 35 * DEG, 0.0, 0.02, SCENE_TOWN},   // the nearest settlement of the scan's first felisian world that had a people, from 30 m outside its edge
    {"civilisation_desert", PT_DESERT, 15, 40 * DEG, 0.0, 0.02, SCENE_TOWN},   // the same on a dead desert world
    {"civilisation_harbour", PT_FELISIAN, 12, 35 * DEG, 0.0, 0.02, SCENE_HARBOUR},   // the nearest settlement with a harbour, from the quay's landward side looking out along the mole
    {"civilisation_dry_harbour", PT_DESERT, 15, 40 * DEG, 0.0, 0.02, SCENE_HARBOUR},   // the same on a dead desert world: a harbour on the dry shore
    {"civilisation_terraces", PT_FELISIAN, 12, 35 * DEG, 0.0, 0.02, SCENE_TERRACES},   // the nearest terraced settlement, from below its slope looking up across the terraces
    {"civilisation_road", PT_FELISIAN, 12, 35 * DEG, 0.0, -0.04, SCENE_ROAD},      // on the nearest settlement's longest road, 60 m out from its edge, looking along it toward the other end
    {"civilisation_road_desert", PT_DESERT, 15, 40 * DEG, 0.0, -0.04, SCENE_ROAD},   // the same on a dead desert world
    // before the nearest settlement's graves, two metres from the first row, looking along the rows at the stones with the ruins behind
    {"civilisation_graves", PT_FELISIAN, 12, 35 * DEG, 0.0, -0.12, SCENE_GRAVES},
    {"civilisation_graves_desert", PT_DESERT, 15, 40 * DEG, 0.0, -0.12, SCENE_GRAVES},   // the same on a dead desert world
    // the nearest settlement of the second people of the scan's first world of two, from outside its gate (a culture of its own beside the
    // first's)
    {"civilisation_second_people", PT_FELISIAN, 12, 35 * DEG, 0.0, 0.02, SCENE_SECOND_PEOPLE},
    {"civilisation_shard", PT_FELISIAN, 12, 35 * DEG, 0.0, -0.42, SCENE_SHARD},  // on the floor of the room that holds the first shard of that settlement, 1.9 m from it, looking down at it
    // a settlement of each age on its age's test world (`AGE_WORLDS`): the greatest near a land point on the people's side, from
    // outside its edge three quarters along its main street, the light 35 degrees up
    {"civilisation_prehistoric", -1, 0, 35 * DEG, 0.0, 0.02, SCENE_AGE_PREHISTORIC},
    {"civilisation_medieval", -1, 0, 35 * DEG, 0.0, 0.02, SCENE_AGE_MEDIEVAL},
    {"civilisation_industrial", -1, 0, 35 * DEG, 0.0, 0.02, SCENE_AGE_INDUSTRIAL},
    {"civilisation_modern", -1, 0, 35 * DEG, 0.0, 0.02, SCENE_AGE_MODERN},
    {"civilisation_futuristic", -1, 0, 35 * DEG, 0.0, 0.02, SCENE_AGE_FUTURISTIC},
    {"civilisation_far_future", -1, 0, 35 * DEG, 0.0, 0.02, SCENE_AGE_FAR_FUTURE},
    // on the old shore of a dead desert world, looking out over the dry seabed
    {"desert_dead_sea", PT_DESERT, 15, 35 * DEG, 0.0, -0.02, SCENE_DEAD_SEA},
};

// E-03: the scene of that name, or none (the names are unique)
static inline const CmpScene* findScene(const std::string& name) {
    for (const CmpScene& sc : CMP_SCENES) if (name == sc.name) return &sc;
    return nullptr;
}
