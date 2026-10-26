#pragma once
// E-10: the records' schema, for genres.cpp's parts alone (genres, genres_schema): the facts by index with their names, the
// inventions', institutions' and traits' facts, the entities, paths, fields and directives the grammar may name, and the
// history's words as the grammar's keys; `schemaOf` (genres_schema.cpp) gathers them for the grammar
#include "genres.h"
#include "genome.h"
#include "history.h"
#include "peoples.h"
#include "records.h"
#include "ruins.h"
#include "tongue.h"
#include "galaxy/starfield.h"
#include "../core/rng.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <set>
#include <unordered_map>

// ---- the schema ----------------------------------------------------------------------------------------------------------------------

// the facts by index: the names first (the decoder keeps them), then the world's, the people's, the record's, what the history offers
// at the date (the genres' guards), the charts' captions', a chronicle's year; then one an invention, one an institution, one a trait
enum RecordFact {
    F_PEOPLE = 0, F_GOD, F_RIVER, F_MOUNTAIN, F_SEA, F_CITY, F_CITY2, F_MOON, F_STAR, F_FESTIVAL, F_OTHER, F_MARKPEOPLE, F_FOUNDERNAME,
    F_DESERT, F_TILTED, F_COLD, F_MOONS, F_RINGS, F_AURORA, F_LOCKED, F_COMPANION, F_COMET, F_GIANT, F_LONGNIGHT, F_MOTHER, F_HIGHMOUNTAINS,
    F_SUNCLASS, F_FLORA, F_FLORA2, F_SEASONS, F_COAST,
    F_LASTAGE, F_FAITH, F_RITE, F_HOLY, F_ECONOMY, F_ECONOMY2, F_VALUE, F_VALUE2, F_NUMBER, F_FOCUS, F_CENTRE, F_DEADWAY, F_MONUMENTS, F_ORIENT,
    F_OTHERFATE, F_PEOPLES, F_CAUSE, F_SPEED,
    F_AGE, F_MEDIUM, F_YEAR, F_SEASON, F_STAGE, F_GOV, F_ERA, F_RULED, F_REIGN, F_HARVEST, F_LASTHARVEST, F_WAR, F_SICK, F_TEMPER, F_FAR,
    F_LAST, F_TONE, F_SHARE, F_CONTACT, F_OTHERWAR, F_OTHERGONE, F_MONTH, F_DAY, F_HOMERANK, F_FAMINE, F_ENDING,
    F_PAIR, F_WRITER, F_DEAD, F_WILL, F_NOTABLE, F_EVENT, F_BATTLE, F_BUILT, F_INVENTED, F_HOLYPLACE, F_RAILWAY, F_HOUSEHOLD, F_SIGN, F_FIGURE,
    F_FIGS, F_INFIG, F_MARKHOW, F_NAMED,
    F_Y, F_Y0, F_Y1, F_GAP,
    // H-04's second pass: the people's buildings as the ruins show them (the genome read through the age: `Culture`), its sacred colour
    F_PLAN, F_ROOF, F_ORNAMENT, F_MOTIF, F_TOPS, F_ACCENT, F_LAND, F_STREETS, F_PUBLIC, F_EDGE, F_LINKS, F_LIGHT, F_WALLS, F_HUTS, F_TOWERS,
    // H-05 (CHARTS): a chart's caption reads the way it was made and what that way shows (`shardChartRecord`, peoples/shards.cpp)
    F_CHARTSTYLE, F_TALLY, F_TALLYKIND, F_WANDERERS, F_NUMBERED, F_PLATE, F_MAGLIMIT, F_HEARD, F_ROUTES, F_REACHED, F_PLANNED, F_FARLY, F_FARKIND, F_SIGNS,
    F_PAST,   // B-414: the people had an age before the record's (a history's: `@past`)
    // D-05: a drifter's people's sun in the record's year (`dark`: 0 as ever, 1 dimming, 2 going, 3 gone: the years after the last sunrise), the
    // years since the last sunrise (`darkyears`), how they lost it (`sunway`: wanderer, intruder, dark, dance; empty for every other people)
    F_DARK, F_DARKYEARS, F_SUNWAY,
    F_SUNSIZE, F_SUNFROM,   // D-05: the sun's disc in a row of the observatory's table (`@suntable`) and at its first measure, in the people's own parts
    F_FIXED_COUNT
};
const char* const FACT_NAMES[F_FIXED_COUNT] = {
    "people", "god", "river", "mountain", "sea", "city", "city2", "moon", "star", "festival", "other", "markpeople", "founder",
    "desert", "tilted", "cold", "moons", "rings", "aurora", "locked", "companion", "comet", "giant", "longnight", "mother", "highmountains",
    "sunclass", "flora", "flora2", "seasons", "coast",
    "lastage", "faith", "rite", "holy", "economy", "economy2", "value", "value2", "number", "focus", "centre", "deadway", "monuments", "orient",
    "otherfate", "peoples", "cause", "speed",
    "age", "medium", "year", "season", "stage", "gov", "era", "ruled", "reign", "harvest", "lastharvest", "war", "sick", "temper", "far",
    "last", "tone", "share", "contact", "otherwar", "othergone", "month", "day", "homerank", "famine", "ending",
    "pair", "writer", "dead", "will", "notable", "event", "battle", "built", "invented", "holyplace", "railway", "household", "sign", "figure",
    "figs", "infig", "markhow", "named",
    "y", "y0", "y1", "gap",
    "plan", "roof", "ornament", "motif", "tops", "accent", "land", "streets", "public", "edge", "links", "light", "walls", "huts", "towers",
    "chartstyle", "tally", "tallykind", "wanderers", "numbered", "plate", "maglimit", "heard", "routes", "reached", "planned", "farly", "farkind", "signs",   // H-05 (CHARTS)
    "past",   // B-414
    "dark", "darkyears", "sunway", "sunsize", "sunfrom"};   // D-05
constexpr int NAME_FACTS = F_FOUNDERNAME + 1;

// the inventions by the history's names (`historyInventionName`), each a fact while it exists: `inv.railway`
struct InvKey { const char* name; const char* key; };
const InvKey INV_KEYS[] = {
    {"the bow", "bow"}, {"the dugout boat", "boat"}, {"fired pots", "pots"}, {"the loom", "loom"}, {"the working of copper", "copper"},
    {"writing", "writing"}, {"the working of iron", "iron"}, {"the plough", "plough"}, {"the arch", "arch"}, {"the water mill", "watermill"},
    {"the calendar of the stars", "calendar"}, {"glass", "glass"}, {"the windmill", "windmill"}, {"the compass", "compass"}, {"the clock", "clock"},
    {"the lens", "lens"}, {"the printing press", "press"},
    {"the steam engine", "steam"}, {"the railway", "railway"}, {"the telegraph", "telegraph"}, {"photography", "photography"}, {"the steamship", "steamship"},
    {"the electric light", "electric"}, {"the telephone", "telephone"}, {"radio", "radio"}, {"the engine", "engine"}, {"flight", "flight"},
    {"the reactor", "reactor"}, {"the computer", "computer"}, {"the rocket", "rocket"}, {"the first craft in orbit", "orbit"}, {"the network", "network"},
    {"a landing on the moon", "moonlanding"}, {"the made genes", "genes"},
    {"the thinking machine", "mind"}, {"fusion", "fusion"}, {"the first habitat", "habitat"}, {"the elevator to orbit", "elevator"}, {"the long sleep", "sleep"},
    {"the small machines", "nano"}, {"the copied mind", "copied"}, {"the ship to the stars", "starship"},
    {"the shaping of matter", "shaping"}, {"the end of ageing", "ageless"}, {"the mind of the world", "worldmind"}, {"the folding of distance", "folding"},
    {"the measure of all things", "measure"}};
constexpr int INV_COUNT = (int)(sizeof(INV_KEYS) / sizeof(INV_KEYS[0]));
const char* const INST_KEYS[HI_COUNT] = {"council", "priesthood", "guilds", "academy", "company", "gazette", "university", "assembly", "broadcaster", "network", "mind", "army"};
struct TraitFact { int trait; const char* fact; };
const TraitFact TRAIT_FACTS[] = {{TR_CANYONS, "canyons"}, {TR_MESAS, "mesas"}, {TR_KARST, "karst"}, {TR_RIFT, "rift"}, {TR_ESCARPMENTS, "scarps"},
    {TR_GLACIAL, "glacial"}, {TR_INSELBERGS, "inselbergs"}, {TR_CINDER_FIELD, "cinders"}, {TR_CHAOS, "chaos"}, {TR_POLYGONS, "polygons"},
    {TR_YARDANGS, "yardangs"}, {TR_ERG, "erg"}, {TR_SALT_FLATS, "saltflats"}, {TR_TRAPS, "traps"}, {TR_GREAT_BASIN, "basin"},
    {TR_CORONAE, "coronae"}, {TR_SPIRES, "spires"}, {TR_GEYSERS, "geysers"}, {TR_ARCHIPELAGO, "archipelago"}, {TR_PANGAEA, "pangaea"},
    {TR_LAKELAND, "lakeland"}, {TR_SNOWBALL, "snowball"}, {TR_STORMS, "storms"}, {TR_HAZE, "haze"}, {TR_GIANT_FLORA, "greatflora"},
    {TR_LUMINOUS_FLORA, "glowflora"}, {TR_DEAD_FOREST, "deadforest"}, {TR_RED_SOIL, "redsoil"}, {TR_BLACK_SAND, "blacksand"}, {TR_CHALK, "chalk"}};
constexpr int TRAIT_COUNT = (int)(sizeof(TRAIT_FACTS) / sizeof(TRAIT_FACTS[0]));
constexpr int F_INV0 = F_FIXED_COUNT, F_INST0 = F_INV0 + INV_COUNT, F_TRAIT0 = F_INST0 + HI_COUNT;

enum RecordEntity { E_ME = 0, E_YOU, E_X, E_E, E_RULER, E_HOME, E_SEAT, E_FOUNDER, E_KIN, E_C, E_D, E_LAST, E_F, E_P, E_CHILD, E_SPOUSE, E_COUNT };
const char* const ENTITY_NAMES[E_COUNT] = {"$me", "$you", "$x", "$e", "$ruler", "$home", "$seat", "$founder", "$kin", "$c", "$d", "$last", "$f", "$p", "$child", "$spouse"};
enum RecordPath { P_A = 0, P_B, P_C, P_PLACE, P_PLACE2, P_CAUSE, P_MOTHER, P_FATHER, P_SPOUSE, P_CHILD, P_HOME, P_FOUNDER, P_FROM, P_RULER, P_COUNT };
const char* const PATH_NAMES[P_COUNT] = {"a", "b", "c", "place", "place2", "cause", "mother", "father", "spouse", "child", "home", "founder", "from", "ruler"};
enum RecordField {
    FD_KIND = 0, FD_SEASON, FD_YEAR, FD_AGO, FD_VALUE, FD_OUT, FD_HERE, FD_PUB, FD_WAR, FD_WARKIND, FD_SICKNESS, FD_WAVE, FD_WORK, FD_WORKKEY,
    FD_INVENTION, FD_INV, FD_OMEN, FD_HARVEST, FD_FLOOD, FD_IMPACT, FD_HOW, FD_GOV, FD_GOVOLD, FD_GOVHOW, FD_ACCESSION, FD_RULEEND, FD_INSTITUTION,
    FD_INSTNAME, FD_DISCOVERY, FD_DEED, FD_UNREST, FD_WHITHER, FD_SIGN, FD_STAGE, FD_LEFT, FD_FAITH, FD_TREATY, FD_FOUNDING, FD_WHENCE, FD_FIRST, FD_WON,
    FD_AGE, FD_BORN, FD_DIED, FD_TRADE, FD_TRADES, FD_TITLE, FD_ROLE, FD_ALIVE, FD_KIN, FD_YOURKIN, FD_FAMILY, FD_DEAD, FD_REIGNED, FD_CHILDREN,
    FD_RANK, FD_COAST, FD_RIVER, FD_FOUNDED, FD_YEARS, FD_STANDS, FD_GONE, FD_FAR,
    FD_WHAT, FD_VERDICT, FD_EPITHET, FD_TRAIT, FD_PTRADE, FD_LANDMARK, FD_QUARTER,
    FD_SUN,   // D-05: a HE_SUN event's: smaller, noend, seafrozen, airsnow, lastsunrise, noonstars
    FD_COUNT };
const char* const FIELD_NAMES[FD_COUNT] = {
    "kind", "season", "year", "ago", "value", "out", "here", "pub", "war", "warkind", "sickness", "wave", "work", "workkey",
    "invention", "inv", "omen", "harvest", "flood", "impact", "how", "gov", "govold", "govhow", "accession", "ruleend", "institution",
    "instname", "discovery", "deed", "unrest", "whither", "sign", "stage", "left", "faith", "treaty", "founding", "whence", "first", "won",
    "age", "born", "died", "trade", "trades", "title", "role", "alive", "kin", "yourkin", "family", "dead", "reigned", "children",
    "rank", "coast", "river", "founded", "years", "stands", "gone", "far",
    "what", "verdict", "epithet", "trait", "ptrade", "landmark", "quarter",
    "sun"};   // D-05
enum RecordDirective { D_CAST = 0, D_NEWS, D_ENTRIES, D_YEARS, D_DEATH, D_DEED, D_LIFE, D_EVENT, D_DIGEST, D_PAST, D_SUNTABLE, D_COUNT };
const char* const DIRECTIVE_NAMES[D_COUNT] = {"cast", "news", "entries", "years", "death", "deed", "life", "event", "digest", "past", "suntable"};   // D-05: `suntable`

// ---- the history's words as the grammar's keys -----------------------------------------------------------------------------------

const char* const KIND_KEYS[] = {"coming", "first", "stones", "founding", "accession", "ruleend", "regency", "government", "war", "battle", "siege",
    "sack", "peace", "sickness", "harvest", "famine", "flood", "drought", "quake", "eruption", "impact", "flare", "winter", "built", "way", "festival", "omen",
    "invention", "discovery", "faith", "institution", "deed", "unrest", "migration", "abandoned", "contact", "treaty", "otherend", "sign", "stage", "exodus",
    "end", "fair", "fire", "storm", "pest", "wonder", "games", "law", "trial", "embassy", "accident", "price", "beasts", "show", "visit", "sun", "birth", "marriage", "move", "death",
    "illness", "injury", "apprentice", "house", "quarrel", "journey", "betrothal"};
static_assert(sizeof(KIND_KEYS) / sizeof(KIND_KEYS[0]) == HE_COUNT, "a key an event kind");
const char* const AGE_KEYS[AGE_COUNT] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "farfuture"};
const char* const MEDIUM_KEYS[RM_COUNT] = {"bone", "stone", "painted", "tablet", "scroll", "codex", "plate", "cylinder", "glass", "printed", "reel", "disc", "drive", "crystal", "hum"};
const char* const GOV_KEYS[GOV_COUNT] = {"chiefs", "monarchy", "council", "theocracy", "republic", "assembly", "corporate", "machine"};
const char* const FAITH_KEYS[FAITH_COUNT] = {"ancestors", "land", "sky", "manygods", "onegod", "philosophy", "machine"};
const char* const RITE_KEYS[RITE_COUNT] = {"firstfruits", "fire", "song", "water", "silence", "fast", "pilgrimage", "animals", "memory"};
const char* const HOLY_KEYS[HOLY_COUNT] = {"temple", "grove", "highplace", "spring", "cave", "house", "tombs", "opensky", "hall"};
const char* const ECONOMY_KEYS[ECO_COUNT] = {"hunting", "herding", "farming", "trade", "industry", "information"};
const char* const VALUE_KEYS[VAL_COUNT] = {"war", "piety", "trade", "learning", "art"};
const char* const CAUSE_KEYS[] = {"famine", "iceage", "volcanic", "flood", "sickness", "bandwar", "leaving",
    "greatsickness", "conquest", "cropfailure", "drying", "cold", "civilwar", "impact",
    "poisoned", "coalout", "industrialwar", "revolution", "epidemics",
    "nuclear", "engineered", "climate", "foodsystems", "cascade", "waterwar", "fewerchildren",
    "machine", "nanotech", "geoengineering", "flare", "weapons", "retreat", "exodus",
    "elsewhere", "physics", "chosen", "undescribed", "sunlost"};
static_assert(sizeof(CAUSE_KEYS) / sizeof(CAUSE_KEYS[0]) == CAUSE_COUNT, "a key a cause");
const char* const FOCUS_KEYS[10] = {"spire", "dome", "gate", "tower", "colossus", "mast", "ring", "obelisk", "pool", "tree"};
const char* const CENTRE_KEYS[6] = {"temple", "palace", "assembly", "market", "tomb", "machine"};
const char* const DEAD_KEYS[5] = {"barrows", "tombs", "cemeteries", "towers", "none"};
const char* const MONUMENT_KEYS[5] = {"stelae", "obelisks", "colossi", "arches", "pillars"};
const char* const ORIENT_KEYS[6] = {"sunrise", "star", "moon", "river", "wind", "none"};
const char* const WORK_KEYS[WK_COUNT] = {"holy", "temple", "walls", "fortress", "palace", "bridge", "aqueduct", "harbour", "market", "library", "observatory", "tomb",
    "monument", "theatre", "lighthouse", "granary", "mill", "focus", "stones", "dam", "mine", "station", "factory", "school", "hospital", "tower", "stadium",
    "power", "mast", "launch", "arcology", "habitat", "elevator", "ring", "great"};
const char* const WAR_KEYS[HW_COUNT] = {"bands", "cities", "rebellion", "succession", "revolution", "civil", "other", "conquest"};
const char* const DEATH_KEYS[16] = {"old", "sickness", "childbirth", "battle", "famine", "flood", "quake", "accident", "murdered", "executed", "end", "young",
    "fire", "cold", "sea", "hard"};
const char* const OMEN_KEYS[10] = {"comet", "eclipse", "aurora", "falling", "newstar", "redmoon", "wanderer", "intruder", "bent", "dance"};   // D-05: the lost sun's four
const char* const SUN_KEYS[6] = {"smaller", "noend", "seafrozen", "airsnow", "lastsunrise", "noonstars"};   // D-05: a HE_SUN event's (`$e.sun`)
const char* const SUN_WAY_KEYS[SW_COUNT] = {"wanderer", "intruder", "dark", "dance"};   // D-05: how a drifter's people lost their sun (`sunway`)
const char* const ACCESSION_KEYS[11] = {"founded", "inherited", "chosen", "elected", "seized", "appointed", "won", "ofage", "mind", "revolution", "again"};
const char* const RULEEND_KEYS[7] = {"gaveover", "deposed", "steppeddown", "term", "overthrown", "fled", "ended"};
const char* const GOVHOW_KEYS[9] = {"reform", "revolution", "kingdom", "priests", "companies", "machine", "coup", "elders", "assembly"};
const char* const UNREST_KEYS[4] = {"bread", "strike", "rising", "riot"};
const char* const FAITHEV_KEYS[5] = {"preached", "schism", "reformed", "closed", "persecution"};
const char* const TREATY_KEYS[3] = {"friendship", "trade", "border"};
const char* const FOUNDING_KEYS[13] = {"plain", "fort", "harbour", "crossing", "mine", "market", "shrine", "station", "gathering", "winterseat", "greatplace", "work", "capital"};
const char* const LEFT_KEYS[10] = {"cause", "river", "wells", "mine", "soil", "cities", "band", "cleared", "collapse", "taken"};
const char* const DISCOVERY_KEYS[9] = {"farshore", "source", "pass", "round", "farside", "poles", "orbit", "moon", "worlds"};
const char* const DEED_KEYS[13] = {"song", "chronicle", "laws", "map", "hunt", "sailed", "healed", "frieze", "taught", "book", "theory", "pictures", "gardens"};
const char* const WHITHER_KEYS[9] = {"place", "mountains", "sea", "south", "north", "sky", "elsewhere", "machines", "under"};   // D-05: below
const char* const WHENCE_KEYS[8] = {"mountain", "sea", "north", "south", "east", "west", "river", "dry"};
const char* const FIRST_KEYS[6] = {"fields", "herds", "boats", "pots", "copper", "village"};
const char* const ROLE_KEYS[HR_COUNT] = {"none", "ruler", "regent", "founder", "general", "prophet", "maker", "inventor", "explorer", "rebel", "envoy", "artist", "last"};
const char* const TEMPER_KEYS[5] = {"plain", "wry", "anxious", "pious", "proud"};
const char* const TONE_KEYS[ST_TONE_COUNT] = {"ordinary", "elegy", "warning", "end"};
const char* const RANK_KEYS[] = {"hamlet", "village", "town", "monument", "city", "capital"};
// H-04's second pass: the buildings' words (the genome's categories, `peoples/genome.h`), the accent's colour, a trial's end
const char* const PLAN_KEYS[] = {"square", "round", "polygon", "oval", "free", "court"};
const char* const ROOF_KEYS[] = {"flat", "pitched", "hipped", "domed", "conical", "vaulted", "saddle", "stepped", "open"};
const char* const ORNAMENT_KEYS[] = {"bare", "banded", "carved", "encrusted"};
const char* const MOTIF_KEYS[] = {"lines", "circles", "steps", "spirals", "glyphs", "figures", "stars"};
const char* const TOPS_KEYS[] = {"finials", "crenels", "masts", "statues", "nothing"};
const char* const ACCENT_KEYS[] = {"red", "ochre", "blue", "green", "teal", "white", "black"};
const char* const LAND_KEYS[] = {"ridges", "valleys", "terraced", "cliffs", "stilts", "mounds", "pits"};
const char* const STREETS_KEYS[] = {"grid", "radial", "concentric", "organic", "line", "contour", "branching"};
const char* const PUBLIC_KEYS[] = {"plazas", "courts", "gardens", "none"};
const char* const EDGE_KEYS[] = {"walls", "ditches", "nothing"};
const char* const LINKS_KEYS[] = {"arcades", "bridges", "tunnels", "colonnades", "none"};
const char* const LIGHT_KEYS[] = {"none", "lamps", "lines"};
const char* const VERDICT_KEYS[] = {"acquitted", "condemned", "exiled", "fined"};
template <size_t N> const char* keyOf(const char* const (&t)[N], int i) { return t[i < 0 ? 0 : (i >= (int)N ? (int)N - 1 : i)]; }

// the schema the grammar reads, built once (genres_schema.cpp): the facts in the order above, the entities, paths, fields, directives
const VgSchema& schemaOf();
