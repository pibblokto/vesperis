// C-01: the ruins of a world (see ruins.h): a civilisation's settlements in the cells of its world's 2 km grid (B-417: M4-07's
// monoliths of the old ones no longer take one in twenty of them). A settlement is laid out once from the cell's hash: its class gives the
// number of buildings and the radius, its plan places them (a cluster, a street, rings round a centre, a grid of streets
// with a plaza), and every building draws its own pieces from its seed: walls that stand to a share of their height by
// its decay, a doorway with a lintel, a window, an inner wall, the columns of a hall, a roof where the culture built stone
// ones and the house is sound, rubble at the foot of a fallen wall.
#include "ruins.h"
#include "drainage.h"
#include "peoples.h"
#include "buildings.h"
#include "plan.h"
#include "towns.h"
#include "works.h"
#include "history.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <mutex>
#include <unordered_map>

const char* const SETTLEMENT_CLASS_NAMES[7] = {"HAMLET", "VILLAGE", "TOWN", "MONUMENT", "CITY", "CAPITAL", "WORKS"};
const char* const BUILDING_KIND_NAMES[BK_COUNT] = {"HOUSE", "HALL", "TOWER", "ROTUNDA", "GATE", "PLATFORM", "COLONNADE", "STELA", "RUBBLE", "WALL", "QUAY", "MOLE", "BOLLARD", "TERRACE", "CISTERN", "WIND WALL", "CAIRN",
    "HUT", "LONGHOUSE", "TENT RING", "STONE CIRCLE", "MENHIR", "DOLMEN", "PALISADE", "TEMPLE", "WALL TOWER",
    "ROW", "TENEMENT", "VILLA", "CHURCH", "TOWN HALL", "STATION", "MILL", "GASOMETER", "WATER TOWER", "HEADFRAME", "OBSERVATORY",
    "BLOCK", "SUBURB", "HIGHRISE", "STADIUM", "MAST", "POWER", "TANKS", "ARCOLOGY", "HABITAT", "SPIRE", "RING", "MAGLEV", "WRECK", "POD",
    "SPIKE", "HALO", "PILLARS", "SHAFT", "MONOLITH", "FOCUS", "WINDMILL", "AQUEDUCT", "TWIN TOWERS", "TUNNEL", "CRANE", "CONTAINERS", "RESERVOIR",
    "MOUND", "DAM", "DISH", "TOMB", "FORT", "BUNKER", "TEETH", "VEHICLE", "AIRCRAFT", "SHIP", "LAUNCH PAD", "HANGAR", "COLOSSUS", "TURBINE",
    "LIGHTHOUSE", "LATTICE", "DERRICK", "BLOCKS", "SPHERE", "TETHER", "TRILITHON", "CONVEYOR", "FURNITURE",
    "DOME", "GLASSHOUSE"};   // D-05
static_assert(BK_GREENHOUSE + 1 == BK_COUNT, "a name for every building kind");

double ruinCellLat(const BodyGen& g) { return 2000.0 / (g.R * 1000.0); }

void ruinCellOf(const BodyGen& g, double lat, double lon, int& gLat, int& gLon) {
    double dLat = ruinCellLat(g);
    gLat = (int)std::floor(lat / dLat);
    double dLon = dLat / std::max(std::cos((gLat + 0.5) * dLat), 0.05);
    gLon = (int)std::floor(lon / dLon);
}
int ruinCellLonIn(const BodyGen& g, int gLatFrom, int gLonFrom, int gLatTo) {
    if (gLatFrom == gLatTo) return gLonFrom;
    const double dLat = ruinCellLat(g);
    const double dLonA = dLat / std::max(std::cos((gLatFrom + 0.5) * dLat), 0.05), dLonB = dLat / std::max(std::cos((gLatTo + 0.5) * dLat), 0.05);
    return (int)std::floor((gLonFrom + 0.5) * dLonA / dLonB);
}
int ruinCellsAround(const BodyGen& g, int gLat) {
    double dLat = ruinCellLat(g);
    double dLon = dLat / std::max(std::cos((gLat + 0.5) * dLat), 0.05);
    return (int)std::ceil(TAU / dLon);
}

// C-08: the base decay from the age and the wet: a village of two centuries stands to its lintels, one of two millennia to
// half its walls, one of thirty millennia is foundations; the rain takes a fifth more
double decayOf(double ageYears, double wet) { return clampd(0.08 + 0.3 * std::log10(std::max(100.0, ageYears) / 100.0) + 0.2 * wet, 0.05, 0.95); }

// C-13: the peoples of a world (see ruins.h)
int peoplesOf(const BodyGen& g) { return g.hasTrait(TR_CIVILISATION) ? (unitFromHash(mix64(g.seed ^ 0xC13ULL)) < 0.2 ? 2 : 1) : 0; }
BodyGen peopleGen(const BodyGen& g, int people) { BodyGen pg = g; if (people > 0) pg.seed = mix64(g.seed ^ (0xC13A5ULL + (uint64_t)people * 0x9E3779B97F4A7C15ULL)); return pg; }
bool peoplesEndedTogether(const BodyGen& g) { return g.drifter || unitFromHash(mix64(g.seed ^ 0xC13E0ULL)) < 0.5; }   // D-05: a drifter's lost one sun
int systemPeoplesWorld(const StarSystem& sys) {
    for (int pass = 0; pass < 2; pass++)
        for (const Body& b : sys.bodies) {
            if (!kindHas(b.type, KT_PEOPLE) || deadKind(b.type) != (pass == 1)) continue;
            if (worldHadCivilisation(BodyGen::make(b))) return b.index;
        }
    return -1;
}
namespace {
std::mutex g_divideMutex;
std::unordered_map<uint64_t, Vec3> g_divides;   // a world's divide, by its seed (two threads may find the same one: they agree)
}
// the great circle: twelve poles drawn from the seed, forty-eight points along each sampled at 2 km (the sea, the inland water
// and a dead desert's old sea count as water), the one crossing the least land kept
Vec3 peopleDivide(const BodyGen& gSeason) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    { std::lock_guard<std::mutex> lock(g_divideMutex); auto it = g_divides.find(g.seed); if (it != g_divides.end()) return it->second; }
    DrainageOff off;
    Rng rng(mix64(g.seed ^ 0xD1F1DEULL));
    Vec3 best(0, 0, 1); int bestLand = 1 << 30;
    for (int c = 0; c < 12; c++) {
        double z = rng.range(-1, 1), a = rng.uni() * TAU, r = std::sqrt(std::max(0.0, 1 - z * z));
        Vec3 n(r * std::cos(a), r * std::sin(a), z);
        Vec3 e = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), f = cross(n, e);
        int land = 0;
        for (int k = 0; k < 48; k++) {
            double t = k * TAU / 48;
            SurfaceSample s = sampleSurface(g, normalize(e * std::cos(t) + f * std::sin(t)), 2048.0);
            if (!(s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5)) land++;
        }
        if (land < bestLand) { bestLand = land; best = n; }
    }
    std::lock_guard<std::mutex> lock(g_divideMutex);
    if (g_divides.size() > 4096) g_divides.clear();
    g_divides[g.seed] = best;
    return best;
}
int peopleAt(const BodyGen& g, double lat, double lon) {
    if (peoplesOf(g) < 2) return 0;
    return dot(StarSystem::bodyFromLatLon(lat, lon), peopleDivide(g)) >= 0 ? 0 : 1;
}

Culture cultureOf(const BodyGen& g, int people) {
    Culture c;
    const BodyGen pg = peopleGen(g, people);   // C-13: the people's draws (the world's own for the first)
    // P-04: what a people built is its world's in its time (a dead world's living kind), what the years did to it since is the present's: a
    // scorched world's dry air keeps its ruins as a desert's does and buries them in dust, a frozen world's cold keeps what it holds under frost
    const int then = livingKindOf(g.type), now = nowKind(g);
    const bool dryNow = kindHas(now, KT_DRY), frozenNow = KINDS[now].cover == DC_FROZEN;
    c.people = people;
    Rng r(pg.seed ^ 0xC01DULL);
    c.style = r.chance(0.55) ? 0 : 1;
    c.family = r.chance(kindHas(then, KT_DRY) ? 0.6 : 0.2) ? 1 : 0;
    c.tall = 0.75 + 0.55 * r.uni();
    double uDecay = r.uni();   // C-01's draw kept in the stream; the decay is the age's now
    c.stoneRoofs = r.chance(0.55);
    c.plan = r.chance(0.6) ? SP_GRID : SP_RADIAL;
    c.towers = r.chance(0.7);
    double uBuried = r.uni();
    // C-08 weathering by age: the years since the end and the world's wet; the walls sink the deeper the older. A-01: the years
    // are the profile's (C-08's draw, its span by the age; C-13's one catastrophe for both is the profile's too) and the weather
    // counts them over the age's durability
    PeopleAge pa = peopleAgeOf(g, people);
    c.age = pa.age;
    c.ageYears = pa.endedYearsAgo;
    // D-05: a drifter's people's works weathered only until the cold took the world (no rain, no wind, no thaw since): a century or two
    if (g.drifter) c.ageYears = std::min(c.ageYears, 60.0 + 190.0 * unitFromHash(mix64(pg.seed ^ 0xD05F1ULL)));
    c.weatherYears = c.ageYears / ageDurability(c.age);
    c.wet = dryNow || frozenNow ? 0.05 + 0.15 * uDecay : 0.5 + 0.5 * uDecay;
    c.decay = decayOf(c.weatherYears, c.wet);
    double ageK = clampd(std::log10(c.weatherYears / 100.0) / 2.6, 0.25, 1.0);
    c.buried = (dryNow ? 0.6 + 1.2 * uBuried : (frozenNow ? 0.3 + 0.8 * uBuried : 0.15 + 0.4 * uBuried)) * ageK;
    // C-09 the roads, by hashes of their own (the stream above stays): a desert people beat tracks as often as it paves
    c.paved = unitFromHash(mix64(pg.seed ^ 0xC09AULL)) < (kindHas(then, KT_DRY) ? 0.5 : 0.7);
    c.roadHalf = 2.5 + 1.5 * unitFromHash(mix64(pg.seed ^ 0xC09BULL));
    c.roadWear = clampd(0.1 + 0.55 * ageK * (0.35 + 0.65 * c.wet), 0.1, 0.6);
    // A-04: the architecture genome (galaxy/genome.h), the people's own through its ages, by hashes of its own (the stream above stays),
    // and the ways it builds in its age (`cultureWays`: C-01's proportion, plan, towers, style and stone roofs above are its now, and
    // A-03's roof, temple, huts, towers and great structure, the palette). The buildings' decay is their materials' at the years since
    // the end over the people's lifetimes (`buildingDecay`); `decay` above stays A-01's (the graves' stones read it)
    c.genome = genomeOf(pg.seed);
    c.seed = pg.seed;   // T-02
    cultureWays(c, pg.seed);
    return c;
}

// T-02: the people building in an older age's way (its genome through that age), weathered longer where it was left before the end; its
// banks the people's
Culture cultureAtAge(const Culture& c, int age, double extraYears) {
    age = clampi(age, 0, AGE_COUNT - 1);
    if (age == c.age && extraYears <= 0) return c;
    Culture v = c;
    v.age = age;
    v.ageYears = c.ageYears + std::max(0.0, extraYears);
    v.weatherYears = v.ageYears / ageDurability(age);
    v.decay = decayOf(v.weatherYears, v.wet);
    cultureWays(v, c.seed);
    v.palette = c.palette;
    return v;
}

namespace {
// A-03: the age's scale of a settlement's plan: its spacing and its radius (a prehistoric camp's huts closer than a medieval town's
// houses, an industrial town's rows wider apart, a modern town's blocks wider still); the futuristic and far-future ages' own
double ageScale(int age) { static const double S[AGE_COUNT] = {0.85, 1.0, 1.85, 2.4, 1.0, 1.0}; return S[clampi(age, 0, AGE_COUNT - 1)]; }

// the class's size: the number of buildings and the radius, the first draws of the layout's stream (the landmark finder
// reads the radius without the layout). A-03: by the age (C-01's draws, scaled; the futuristic age's few great structures and the
// far future's one have radii of their own, drawn after)
// a lone monument, the age's: its kind and size drawn with the class's size, so its reach is the radius the finders read
struct MonPick { int kind = BK_PLATFORM, count = 1, style = 0; double hw = 5, hd = 5, hWall = 3.3; };
double monumentReach(const MonPick& m, double tall) {
    switch (m.kind) {
        case BK_STONE_CIRCLE: return m.hw + 9;
        case BK_DOLMEN: return m.hd * 2.6 + 5;
        case BK_MENHIR: return m.hd + 3;
        case BK_OBSERVATORY: case BK_GASOMETER: case BK_WATER_TOWER: return m.hw + 3;
        case BK_HEADFRAME: return std::sqrt(m.hw * m.hw + m.hd * m.hd) + 4;
        case BK_MAST: return m.hWall * tall * 0.55 + 3;
        case BK_POWER: return std::sqrt(m.hw * m.hw + (m.hd + 26) * (m.hd + 26)) + 4;
        case BK_TANKS: return m.hw + 4;
        case BK_HIGHRISE: return m.hw * 1.5 + std::min(m.hWall * tall * 0.5, 120.0) * 0.6;
        case BK_RING: case BK_HALO: return m.hw + 6;
        case BK_WRECK: return m.hd * 2.4 + 40;
        case BK_SPIRE: return m.hw * 2 + 30;
        case BK_SPIKE: case BK_SHAFT: return m.hw + 4;
        case BK_PILLARS: return m.hd + m.hw + 4;
        case BK_MONOLITH: return m.hw * 1.45;
        case BK_FOCUS: return std::max(m.hw, m.hd) * 1.3 + 8;   // A-04
        default: return 12;   // C-01's
    }
}
// A-04: the focal accent's size by its kind, its age and the people's ambition (a modern people's is twice a medieval one's, a
// futuristic one's four times)
void focusSize(const Culture& c, int focus, Rng& rng, double& hw, double& hd, double& h) {
    static const double AGE_K[AGE_COUNT] = {0.7, 1.0, 1.4, 2.2, 4.0, 4.0};
    const double k = AGE_K[clampi(c.age, 0, AGE_COUNT - 1)] * (0.6 + 0.9 * c.genome.at(GV_AMBITION)), u = rng.uni(), v = rng.uni();
    switch (focus) {
        case FO_SPIRE: hw = 2.5 + 1.0 * u; h = 30 + 20 * v; break;
        case FO_DOME: hw = 6 + 4 * u; h = 10 + 6 * v; break;
        case FO_GATE: hw = 3 + 2 * u; h = 9 + 6 * v; break;
        case FO_TOWER: hw = 2.5 + 1.5 * u; h = 18 + 14 * v; break;
        case FO_COLOSSUS: hw = 2.0 + 1.0 * u; h = 10 + 14 * v; break;
        case FO_MAST: hw = 1.5 + 1.0 * u; h = 20 + 20 * v; break;
        case FO_RING: hw = 4 + 4 * u; h = 2 * hw; break;
        case FO_OBELISK: hw = 1.4 + 0.8 * u; h = 14 + 12 * v; break;
        case FO_POOL: hw = 6 + 4 * u; hd = 4 + 4 * v; h = 0.8; hw *= std::sqrt(k); hd *= std::sqrt(k); return;
        default: hw = 7 + 4 * u; h = 16 + 10 * v; break;   // a great tree
    }
    hw *= std::sqrt(k); h *= k; hd = hw;
    if (focus == FO_RING) h = 2 * hw;
}
void pickMonument(const Culture& c, Rng& rng, MonPick& m) {
    const double u = rng.uni();
    const double amb = 0.75 + 0.6 * c.genome.at(GV_AMBITION);   // A-04: the people's ambition
    auto set = [&](int k, double hw, double hd, double h) { m.kind = k; m.hw = hw; m.hd = hd; m.hWall = h; };
    switch (c.age) {
        case AGE_PREHISTORIC:
            if (u < 0.4) { double rr = (8 + 10 * rng.uni()) * amb; set(BK_STONE_CIRCLE, rr, rr, 3 + 2.5 * rng.uni()); }
            else if (u < 0.65) set(BK_DOLMEN, 1.3 + 0.4 * rng.uni(), 1.7 + 0.6 * rng.uni(), 1.8 + 0.4 * rng.uni());
            else if (u < 0.85) set(BK_MENHIR, 1.0, (12 + 18 * rng.uni()) * amb, 2.5 + 2.0 * rng.uni());
            else set(BK_MENHIR, 1.5, 0.8, (8 + 12 * rng.uni()) * amb);
            break;
        case AGE_INDUSTRIAL:
            if (u < 0.3) { double rr = 6 + 3 * rng.uni(); set(BK_OBSERVATORY, rr, rr, 6 + 2 * rng.uni()); }
            else if (u < 0.55) { double rr = 13 + 6 * rng.uni(); set(BK_GASOMETER, rr, rr, 18 + 10 * rng.uni()); }
            else if (u < 0.75) { double rr = 5 + 1.5 * rng.uni(); set(BK_WATER_TOWER, rr, rr, 18 + 8 * rng.uni()); }
            else set(BK_HEADFRAME, 22 + 8 * rng.uni(), 18 + 6 * rng.uni(), 22 + 8 * rng.uni());
            break;
        case AGE_MODERN:
            if (u < 0.35) set(BK_MAST, 3, 3, (150 + 150 * rng.uni()) * amb);
            else if (u < 0.6) set(BK_POWER, 70 + 40 * rng.uni(), 70 + 20 * rng.uni(), 1);
            else if (u < 0.85) set(BK_TANKS, 30 + 20 * rng.uni(), 14 + 6 * rng.uni(), 20 + 15 * rng.uni());
            else { double hw = 12 + 6 * rng.uni(); set(BK_HIGHRISE, hw, hw, (100 + 200 * rng.uni()) * amb); }
            break;
        case AGE_FUTURISTIC:
            if (u < 0.5) { double rr = (60 + 90 * rng.uni()) * amb; set(BK_RING, rr, 6, 2 * rr); }
            else if (u < 0.8) set(BK_WRECK, 12 + 14 * rng.uni(), 60 + 90 * rng.uni(), 30);
            else { double rr = 18 + 12 * rng.uni(); set(BK_SPIRE, rr, rr, (400 + 600 * rng.uni()) * amb); }
            break;
        case AGE_FAR_FUTURE:   // the people's one structure (A-04: its focus's), its size the people's ambition's
            switch (c.great) {
                case 0: { double rr = (40 + 80 * rng.uni()) * amb; set(BK_SPIKE, rr, rr, rr * (14 + 10 * rng.uni())); break; }
                case 1: { double rr = (150 + 300 * rng.uni()) * amb; set(BK_HALO, rr, 6, 2 * rr); break; }
                case 2: { double rr = (10 + 15 * rng.uni()) * amb; set(BK_PILLARS, rr, rr * 6 * (2 + rng.irange(5)), (200 + 400 * rng.uni()) * amb); break; }
                case 3: { double rr = (40 + 100 * rng.uni()) * amb; set(BK_SHAFT, rr, rr, 2); break; }
                default: { double hw = (80 + 200 * rng.uni()) * amb; set(BK_MONOLITH, hw, hw, 2 * hw); break; }
            }
            break;
        default:   // C-01's: a platform with its shrine, a tower, a rotunda; A-04: the people's focal accent, its monuments (a pair of stelae, an
                   // obelisk, a statue, an arch, a pair of pillars)
            if (u < 0.35) set(BK_PLATFORM, 5 + 3 * rng.uni(), 5 + 3 * rng.uni(), 3.3);
            else if (u < 0.5 && c.towers) set(BK_TOWER, 2.4 + 1.0 * rng.uni(), 2.4 + 1.0 * rng.uni(), 10 + 7 * rng.uni());
            else if (u < 0.62) { double rr = 4.0 + 2.0 * rng.uni(); set(BK_ROTUNDA, rr, rr, 3.0 + 1.0 * rng.uni()); }
            else if (u < 0.8) { double hw, hd, h; focusSize(c, c.genome.of(GV_FOCUS), rng, hw, hd, h); set(BK_FOCUS, hw, hd, h); m.style = c.genome.of(GV_FOCUS); }
            else {
                const int mn = c.genome.of(GV_MONUMENTS);
                m.style = mn;
                switch (mn) {
                    case MN_OBELISKS: set(BK_STELA, 0.6, 0.6, 5 + 3 * rng.uni()); break;
                    case MN_COLOSSI: set(BK_STELA, 0.9, 0.9, 5 + 3 * rng.uni()); break;
                    case MN_ARCHES: set(BK_STELA, 2.6, 0.8, 6 + 3 * rng.uni()); break;
                    case MN_PILLARS: set(BK_STELA, 0.7, 0.7, 5 + 2.5 * rng.uni()); m.count = 2; break;
                    default: set(BK_STELA, 0.45, 0.25, 3.5 + 2.0 * rng.uni()); m.count = 2; break;
                }
            }
            break;
    }
}

void settlementSize(RuinSpec& r, Rng& rng, int& n, double& R, const Culture& c, MonPick* mon = nullptr) {
    const int age = c.age;
    switch (r.sclass) {   // A-04: the plan the people's streets' (C-01's draws kept in the stream)
        case SC_HAMLET: { n = 3 + rng.irange(4); R = 22 + 12 * rng.uni(); const bool cl = rng.chance(0.6); r.plan = c.plan == SP_STREET ? SP_STREET : (c.plan == SP_GRID ? (cl ? SP_CLUSTER : SP_STREET) : SP_CLUSTER); break; }
        case SC_VILLAGE: n = 7 + rng.irange(8); R = 45 + 25 * rng.uni(); rng.chance(0.5); r.plan = c.plan; break;
        case SC_TOWN: n = 24 + rng.irange(21); R = 60 + 30 * rng.uni(); r.plan = c.plan; break;
        case SC_CITY: case SC_CAPITAL: {   // T-01: the plan's core, its radius and its count the plan's (no age scale: the plan's radius is the drawn one)
            n = r.wanted > 0 ? r.wanted : (r.sclass == SC_CAPITAL ? 120 : 60); R = r.size > 0 ? r.size : 200;
            r.plan = c.plan == SP_STREET ? SP_GRID : c.plan;   // a people of the line laid its city's streets along its axis, a grid
            if (age == AGE_PREHISTORIC) r.plan = SP_RADIAL;     // the gathering place: rings round its centre
            if (age == AGE_FUTURISTIC) r.plan = SP_GREAT;
            return;
        }
        default: { n = 1; r.plan = SP_LONE; MonPick m; pickMonument(c, rng, m); R = monumentReach(m, c.tall); if (mon) *mon = m; return; }
    }
    R *= ageScale(age);
    const int ms = c.genome.of(GV_MASSING);   // A-04: a compact people's towns tighter, a sprawling one's wider
    if (ms == MS_COMPACT) R *= 0.88; else if (ms == MS_SPRAWLING) R *= 1.18;
    if (age == AGE_PREHISTORIC && (r.plan == SP_STREET || r.plan == SP_GRID)) r.plan = r.sclass == SC_HAMLET ? SP_CLUSTER : SP_RADIAL;   // camps and rings of huts, no streets
    if (age == AGE_FUTURISTIC && r.sclass != SC_MONUMENT) {   // a few great structures and the land between left wild
        r.plan = SP_GREAT;
        double u = rng.uni();
        R = r.sclass == SC_HAMLET ? 40 + 20 * u : (r.sclass == SC_VILLAGE ? 150 + 90 * u : 300 + 150 * u);
        n = r.sclass == SC_HAMLET ? 2 + rng.irange(3) : (r.sclass == SC_VILLAGE ? 4 + rng.irange(3) : 7 + rng.irange(4));
        if (ms == MS_SPRAWLING) n += 2;
    }
}

}   // namespace

double settlementRadiusOf(const Culture& c, int sclass, uint64_t seed) {   // T-01
    RuinSpec r; r.sclass = sclass; r.seed = seed;
    Rng rng(seed ^ 0x5E77ULL); int n; double R;
    settlementSize(r, rng, n, R, c);
    return sclass == SC_MONUMENT ? std::max(R, 12.0) : R;
}

namespace {
// T-02: the buildings placed so far by cells (a city's thousand and more: the fit test reads the cells round a spot, not every building)
struct BuildingGrid {
    double cell = 16, x0 = 0, z0 = 0; int n = 0;
    std::vector<std::vector<int>> cells;
    void init(double reach) { x0 = z0 = -reach; n = std::max(1, (int)std::ceil(2 * reach / cell)); cells.assign((size_t)n * n, {}); }
    void range(double x, double z, double rad, int& i0, int& i1, int& j0, int& j1) const {
        i0 = clampi((int)std::floor((x - rad - x0) / cell), 0, n - 1); i1 = clampi((int)std::floor((x + rad - x0) / cell), 0, n - 1);
        j0 = clampi((int)std::floor((z - rad - z0) / cell), 0, n - 1); j1 = clampi((int)std::floor((z + rad - z0) / cell), 0, n - 1);
    }
    void insert(const Building& b, int idx) {
        int i0, i1, j0, j1; range(b.x, b.z, std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2, i0, i1, j0, j1);
        for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) cells[(size_t)j * n + i].push_back(idx);
    }
};
// T-02: a building's kind and size before it is placed (the town's lots read its frontage and depth first)
struct BSpec { int kind = -1; double hw = 3, hd = 3, h = 3, thick = 0.45; int style = -1; bool rect = false; };   // `rect`: a row's house, on a rectangle

struct Layout {
    RuinSpec& r; const Culture& c; const Genome& G; Rng rng; double luckS = 0; int age;
    // A-04: the people's ways in the plan: how far its houses' sizes spread (alike: a fifth of C-01's spread; varied: half again), its
    // massing's footprint and height, its appetite for height, an arcade on the next house
    double spread = 1, footK = 1, hdK = 1, heightK = 1, towerK = 1;
    bool arcadeNext = false, rectNext = false;   // the next house arcaded along its front; the next on a rectangle (a row's)
    // T-02: a town's plan: the buildings by cells, and its streets (a building placed with its fit tested keeps off them)
    BuildingGrid* grid = nullptr;
    const StreetGrid* sgrid = nullptr;
    const std::vector<Street>* streets = nullptr;
    Layout(RuinSpec& rr, const Culture& cc, uint64_t salt = 0) : r(rr), c(cc), G(cc.genome), rng(rr.seed ^ 0x5E77ULL ^ salt), age(cc.age) {
        spread = 0.2 + 1.3 * G.at(GV_SAMENESS);
        switch (G.of(GV_MASSING)) {
            case MS_COMPACT: footK = 0.88; heightK = 1.25; break;
            case MS_SPRAWLING: footK = 1.15; hdK = 1.25; heightK = 0.8; break;
            case MS_VERTICAL: footK = 0.85; heightK = 1.7; break;
            case MS_BURIED: heightK = 0.9; break;
            default: footK = 1.06; heightK = 1.15; break;   // stepped
        }
        towerK = 0.7 + 0.8 * G.at(GV_HEIGHT);
        if (age >= AGE_MEDIEVAL && age <= AGE_FUTURISTIC) heightK *= 0.82 + 0.56 * G.at(GV_STOREYS);   // R-412: its dwellings' storeys within its age's
    }
    // R-412: a house's or a villa's frontage as its people turned it to the street (a medieval or an industrial people's): its gable (a narrow,
    // deep house, its ridge running back from the street) or its eaves (a wide, shallow one, its ridge along the street)
    void frontage(BSpec& s) const {
        if (!(age == AGE_MEDIEVAL || age == AGE_INDUSTRIAL) || !(s.kind == BK_HOUSE || s.kind == BK_VILLA)) return;
        if (G.of(GV_FRONTAGE) == FR_GABLES ? s.hw > s.hd : s.hd > s.hw) std::swap(s.hw, s.hd);
    }
    double var(double lo, double span) { return lo + span * clampd(0.5 + (rng.uni() - 0.5) * spread, 0, 1); }   // a draw spread by the people's sameness
    bool fits(double x, double z, double rad) const {
        if (grid) {   // T-02
            int i0, i1, j0, j1; grid->range(x, z, rad + 1.2, i0, i1, j0, j1);
            for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++)
                for (int k : grid->cells[(size_t)j * grid->n + i]) {
                    const Building& b = r.buildings[k];
                    double dx = b.x - x, dz = b.z - z, rb = std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2;
                    if (dx * dx + dz * dz < (rad + rb) * (rad + rb)) return false;
                }
            return true;
        }
        for (const Building& b : r.buildings) {
            double dx = b.x - x, dz = b.z - z, rb = std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2;
            if (dx * dx + dz * dz < (rad + rb) * (rad + rb)) return false;
        }
        return true;
    }
    // T-02: a town's buildings are tested as rectangles (a row's houses side by side, a terrace's long rows), not as their circles
    bool obb = false;
    bool fitsRect(double x, double z, double heading, double hw, double hd, double margin) const {
        const double ux = std::cos(heading), uz = -std::sin(heading), vx = std::sin(heading), vz = std::cos(heading);
        const double rad = std::sqrt(hw * hw + hd * hd);
        auto overlaps = [&](const Building& b) {
            const double dx = b.x - x, dz = b.z - z, rb = std::sqrt(b.hw * b.hw + b.hd * b.hd);
            if (dx * dx + dz * dz >= (rad + rb + margin) * (rad + rb + margin)) return false;
            const double bux = std::cos(b.heading), buz = -std::sin(b.heading), bvx = std::sin(b.heading), bvz = std::cos(b.heading);
            const double ax[4][2] = {{ux, uz}, {vx, vz}, {bux, buz}, {bvx, bvz}};
            for (const auto& a : ax) {
                const double ra = hw * std::fabs(ux * a[0] + uz * a[1]) + hd * std::fabs(vx * a[0] + vz * a[1]);
                const double rbb = b.hw * std::fabs(bux * a[0] + buz * a[1]) + b.hd * std::fabs(bvx * a[0] + bvz * a[1]);
                if (std::fabs(dx * a[0] + dz * a[1]) >= ra + rbb + margin) return false;   // a gap along this axis
            }
            return true;
        };
        if (grid) {
            int i0, i1, j0, j1; grid->range(x, z, rad + margin + 1.2, i0, i1, j0, j1);
            for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) for (int k : grid->cells[(size_t)j * grid->n + i]) if (overlaps(r.buildings[k])) return false;
            return true;
        }
        for (const Building& b : r.buildings) if (overlaps(b)) return false;
        return true;
    }
    // T-02: the buildings from index n on taken back (a town's lot that did not fit its street after all)
    void undoTo(size_t n) {
        while (r.buildings.size() > n) {
            const int idx = (int)r.buildings.size() - 1;
            if (grid) {
                const Building& b = r.buildings.back();
                int i0, i1, j0, j1; grid->range(b.x, b.z, std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2, i0, i1, j0, j1);
                for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) { std::vector<int>& v = grid->cells[(size_t)j * grid->n + i]; v.erase(std::remove(v.begin(), v.end(), idx), v.end()); }
            }
            r.buildings.pop_back();
        }
    }
    // T-02: how far the standing pieces of the buildings from index n on reach into a street's corridor where the explorer walks (their
    // colliders from the ankle to over the head: a mill's chimney, a mound's bank, a jetty, a porch's posts; what fell is walked round),
    // 0 none: what a building's own pieces reach past its footprint
    double intrusion(size_t n, const SiteRead& st, double exX = 1e18, double exZ = 1e18) const {   // (exX, exZ): the streets through it left out (a gate's own)
        if (!sgrid || !streets) return 0;
        double worst = 0;
        std::vector<Form> el; std::vector<FormDisc> discs;
        for (size_t i = n; i < r.buildings.size(); i++) {
            el.clear(); discs.clear();
            buildingForms(r.buildings[i], c, st, 0, el, (int)i);
            for (const Form& e : el) if (e.tilt <= 0.3 && e.part != 3) formColliders(e, discs);
            for (const FormDisc& d : discs) {
                if (d.y0 > 1.9 || d.y1 < 0.3) continue;
                sgrid->near(d.x - d.r - 1, d.z - d.r - 1, d.x + d.r + 1, d.z + d.r + 1, [&](int k) {
                    const Street& s = (*streets)[k];
                    if (exX < 1e17 && streetPointDist(s, exX, exZ) < s.half + 0.5) return;
                    worst = std::max(worst, s.half + 0.4 + d.r - streetPointDist(s, d.x, d.z));
                });
            }
        }
        return worst;
    }
    // T-02: a rectangle keeps off the town's streets (their corridors and half a metre)
    bool offStreets(double x, double z, double heading, double hw, double hd) const {
        if (!sgrid || !streets) return true;
        const double rad = std::sqrt(hw * hw + hd * hd) + 1;
        bool clear = true;
        sgrid->near(x - rad, z - rad, x + rad, z + rad, [&](int k) { if (clear && streetRectGap((*streets)[k], x, z, heading, hw + 0.5, hd + 0.5) < 0) clear = false; });
        return clear;
    }
    bool circleOffStreets(double x, double z, double rad) const {
        if (!sgrid || !streets) return true;
        bool clear = true;
        sgrid->near(x - rad, z - rad, x + rad, z + rad, [&](int k) { if (clear && streetPointDist((*streets)[k], x, z) < rad + (*streets)[k].half) clear = false; });
        return clear;
    }
    // A-04: a building's plan's form (its people's; another now and then where its houses vary; a court where the people built inward)
    int formOf(int kind, double x, double z) const {
        switch (kind) {
            case BK_HOUSE: case BK_HUT: case BK_VILLA: case BK_SUBURB: case BK_BLOCK: case BK_TOWER: case BK_ROTUNDA: case BK_POD: case BK_TEMPLE: break;
            default: return PF_RECT;
        }
        const uint64_t h = mix64(r.seed ^ 0xA04D0ULL ^ ((uint64_t)(int64_t)std::lround(x * 8) * 0x9E3779B97F4A7C15ULL) ^ ((uint64_t)(int64_t)std::lround(z * 8) * 0xD1B54A32D192ED03ULL) ^ (uint64_t)kind);
        if (rectNext) return PF_RECT;
        int f = G.of(GV_PLAN);
        const double var = G.at(GV_SAMENESS);
        if (var > 0.6 && unitFromHash(h) < (var - 0.6) * 1.5) { static const int ALT[3] = {PF_RECT, PF_CIRCLE, PF_POLYGON}; f = ALT[std::min(2, (int)(unitFromHash(mix64(h + 1)) * 3))]; }
        if (G.of(GV_PUBLIC) == PU_COURTS && age >= AGE_MEDIEVAL && (kind == BK_HOUSE || kind == BK_VILLA || kind == BK_BLOCK) && unitFromHash(mix64(h + 2)) < 0.6) f = PF_COURT;
        // B-414: an industrial or a modern people's villas, houses and blocks on the age's plans (a rectangle, an L, a court); the round and the
        // many-sided plans are the hand-made ages' and the futuristic's, kept on the people's own temples, rotundas and towers
        if (age >= AGE_INDUSTRIAL && age <= AGE_MODERN && (kind == BK_VILLA || kind == BK_SUBURB || kind == BK_BLOCK) && (f == PF_CIRCLE || f == PF_POLYGON || f == PF_ELLIPSE)) f = PF_RECT;
        switch (kind) {
            case BK_TOWER: if (f == PF_FREE || f == PF_COURT) f = PF_RECT; break;
            case BK_HUT: if (f == PF_COURT) f = PF_RECT; break;
            case BK_SUBURB: case BK_VILLA: if (f == PF_COURT && kind == BK_SUBURB) f = PF_RECT; if (f == PF_FREE && kind == BK_VILLA) f = PF_RECT; break;
            case BK_ROTUNDA: case BK_POD: if (f == PF_RECT || f == PF_FREE || f == PF_COURT) f = PF_CIRCLE; break;
            case BK_TEMPLE: if (f == PF_RECT || f == PF_FREE || f == PF_COURT) f = PF_CIRCLE; break;
            default: break;
        }
        return f;
    }
    // A-04: what a dwelling stands on, by its people's relation to the land. B-415: a pit, a cut in the rock and a terrace's kerb are the
    // hand-made ages' (an industrial row or a modern block stands on the street); a podium (a people of mounds) and a modern block's pilotis
    // (a people of stilts) are any age's
    int foundOf(int kind) const {
        const bool dwellingK = kind == BK_HOUSE || kind == BK_VILLA || kind == BK_SUBURB || kind == BK_BLOCK || kind == BK_ROW || kind == BK_TENEMENT || kind == BK_HUT;
        if (!dwellingK) return FD_GROUND;
        const bool handMade = age <= AGE_MEDIEVAL;
        switch (G.of(GV_LAND)) {
            case LD_STILTS: return (kind == BK_HOUSE && age == AGE_MEDIEVAL) || (kind == BK_BLOCK && age == AGE_MODERN) ? FD_STILTS : FD_GROUND;
            case LD_MOUNDS: return FD_MOUND;
            case LD_PITS: return handMade ? FD_PIT : FD_GROUND;
            case LD_CLIFFS: return !handMade ? FD_GROUND : (kind == BK_HUT ? FD_PIT : FD_CUT);
            case LD_TERRACED: return handMade ? FD_TERRACE : FD_GROUND;
            default: return FD_GROUND;
        }
    }
    bool add(int kind, double x, double z, double heading, double hw, double hd, double hWall, double thick, bool checkFit = true, int style = -1) {
        const int form = formOf(kind, x, z);
        if (form == PF_COURT) { const double m = kind == BK_BLOCK ? 12.0 : 6.4; hw = std::max(hw, m); hd = std::max(hd, m); }   // a court needs its room
        if (form == PF_FREE && kind == BK_HOUSE) hd = std::max(hd, 4.2);
        double rad = std::sqrt(hw * hw + hd * hd) + 1.2;
        if (checkFit && !(obb ? fitsRect(x, z, heading, hw, hd, 0.02) : fits(x, z, rad))) return false;
        if (checkFit && !offStreets(x, z, heading, hw, hd)) return false;   // T-02
        Building b; b.kind = kind; b.x = x; b.z = z; b.heading = heading; b.hw = hw; b.hd = hd; b.hWall = hWall * c.tall; b.thick = thick;
        b.luck = clampd(luckS + rng.sym(0.25), -0.4, 0.4);   // C-01's decay draw: the building's fortune now (A-03)
        b.door = kind == BK_HOUSE ? rng.irange(4) : 0;
        b.seed = rng.next();
        b.age = age;
        b.style = style >= 0 ? style : (kind == BK_TEMPLE ? c.temple : (kind == BK_ARCOLOGY ? c.great % 3 : 0));
        if (kind == BK_HIGHRISE) b.style = unitFromHash(mix64(b.seed ^ 0xA03F0ULL)) < 0.6 ? c.towerForm : std::min(3, (int)(unitFromHash(mix64(b.seed ^ 0xA03F1ULL)) * 4));   // the people's form, and others
        if (kind == BK_HOUSE && arcadeNext) b.style = 1;   // an arcade along its front
        if (kind == BK_HIGHRISE || (kind == BK_TWIN && age == AGE_MODERN)) b.hWall = std::min(b.hWall, 650.0);   // the tallest a modern people raised
        // A-04: what its room reads of the genome
        b.form = kind == BK_TEMPLE && b.style != 3 ? PF_RECT : form;
        b.sides = G.number();
        b.found = foundOf(kind);
        if (b.found == FD_STILTS && b.form != PF_RECT) b.found = FD_GROUND;
        b.centred = G.of(GV_SYMMETRY) == SY_BILATERAL || G.of(GV_SYMMETRY) == SY_RADIAL || G.of(GV_PROFILE) == WP_BULGING;
        if (b.form == PF_FREE || b.form == PF_COURT) b.door = 0;
        b.before = r.before;   // T-02: a place left before the end
        r.buildings.push_back(b);
        if (grid) grid->insert(r.buildings.back(), (int)r.buildings.size() - 1);
        return true;
    }
    // a dwelling at a spot, the age's: C-01's houses (a rotunda now and then, a heap where one fell); a camp's tent rings and huts, a
    // village's longhouses; terraced rows, tenements near an industrial town's middle and villas at its edge; blocks of flats and
    // suburban houses; a futuristic people's pods. A-04: their sizes by the people's massing and sameness
    // T-02: the dwelling's kind and size alone (`d` its distance from the centre over the radius), the town's lots placing it
    bool dwellingSpec(double d, BSpec& s) {
        const double u = rng.uni();
        const double F = footK, Hk = heightK;
        auto set = [&](int k, double hw, double hd, double h, double t) { s.kind = k; s.hw = hw; s.hd = hd; s.h = h; s.thick = t; s.style = -1; frontage(s); return true; };
        switch (age) {
            case AGE_PREHISTORIC: {
                if (r.sclass == SC_HAMLET && u < 0.55) { double rr = var(2.0, 0.9); return set(BK_TENT_RING, rr, rr, 2.8, 0.2); }
                if (u < 0.8 || r.sclass == SC_HAMLET) { double rr = var(2.3, 1.1) * F; return set(BK_HUT, rr, rr * (G.of(GV_PLAN) == PF_ELLIPSE ? 1.35 : (G.of(GV_PLAN) == PF_RECT ? 1.3 : 1.0)), var(1.5, 0.5) * Hk, 0.5); }
                return set(BK_LONGHOUSE, var(2.6, 0.9) * F, var(6.0, 4.0) * F * hdK, var(1.7, 0.4) * Hk, 0.8);
            }
            case AGE_INDUSTRIAL: {
                if (u < 0.05) return set(BK_RUBBLE, 4.5, 4.5, 1.0, 0.5);
                if (r.sclass == SC_HAMLET || (r.sclass == SC_VILLAGE && u < 0.3) || (townOrGreater(r.sclass) && d > 0.8 && u < 0.4) || (G.of(GV_GROUPING) == GR_DETACHED && u < 0.5))
                    return set(BK_VILLA, var(4.5, 2.0) * F, var(4.5, 1.5) * F * hdK, var(6.5, 1.5) * Hk, 0.45);
                if (townOrGreater(r.sclass) && d < 0.55 && u < 0.45) return set(BK_TENEMENT, var(6.0, 2.0) * F, var(6.0, 2.0) * F, var(13, 6) * Hk, 0.55);
                return set(BK_ROW, var(10.0, 4.0) * F, var(4.2, 1.0) * F * hdK, var(6.5, 3.0) * Hk, 0.45);
            }
            case AGE_MODERN: {
                if (u < 0.04) return set(BK_RUBBLE, 6, 6, 1.0, 0.5);
                if ((r.sclass == SC_CITY || r.sclass == SC_CAPITAL) && d < 0.7) {   // T-01: a city's middle, its towers by the people's appetite for height, blocks round them
                    if (d < 0.4 && rng.uni() < 0.25 + 0.6 * G.at(GV_HEIGHT)) { const double hw = 10 + 5 * rng.uni(); return set(BK_HIGHRISE, hw, hw * (0.75 + 0.25 * rng.uni()), (60 + 220 * rng.uni()) * (1.1 - d) * towerK, 0.5); }
                    return set(BK_BLOCK, var(11, 4) * F, var(6.0, 2.5) * F * hdK, (18 + 30 * rng.uni() * (1.2 - d)) * Hk, 0.4);
                }
                if (r.sclass == SC_HAMLET || (r.sclass == SC_VILLAGE && u < 0.55) || (townOrGreater(r.sclass) && d > 0.85 && u < 0.5))
                    return set(BK_SUBURB, var(4.0, 1.5) * F, var(5.0, 1.5) * F * hdK, (rng.chance(0.5) ? 2.9 : 5.8) * std::min(1.4, Hk), 0.35);
                return set(BK_BLOCK, var(11, 4) * F, var(6.0, 2.5) * F * hdK, (15 + 30 * rng.uni() * (1.2 - d)) * Hk, 0.4);
            }
            case AGE_FUTURISTIC: { double rr = var(9, 5) * F; return set(BK_POD, rr, rr * (G.of(GV_PLAN) == PF_ELLIPSE ? 1.3 : 1.0), 3.4 * std::min(1.5, Hk), 0.5); }
            case AGE_FAR_FUTURE: return false;
            default: {
                if (u < 0.08) return set(BK_RUBBLE, 3.5, 3.5, 1.0, 0.5);
                if (u < 0.2 && G.of(GV_PLAN) != PF_RECT) { double rr = var(3.0, 2.5) * F; return set(BK_ROTUNDA, rr, rr, var(2.8, 1.0) * Hk, 0.5); }
                return set(BK_HOUSE, var(2.4, 1.6) * F, var(3.0, 2.0) * F * hdK, var(2.6, 0.8) * Hk, 0.45);
            }
        }
    }
    bool dwelling(double x, double z, double heading) {
        BSpec s;
        if (!dwellingSpec(std::sqrt(x * x + z * z) / std::max(1.0, r.size), s)) return false;
        return add(s.kind, x, z, heading, s.hw, s.hd, s.h, s.thick, true, s.style);
    }
    // A-04: a dwelling's spot as the people grouped its houses: alone; three joined in a row, their fronts to the street; a compound of
    // two or three round a court, a low wall round them, its gap to the street. Returns the houses placed
    int dwellingSpot(double x, double z, double heading) {
        const int gr = G.of(GV_GROUPING);
        if (age >= AGE_FUTURISTIC || gr == GR_DETACHED || (gr == GR_ROWS && age == AGE_INDUSTRIAL)) return dwelling(x, z, heading) ? 1 : 0;
        // T-01: an industrial or a modern city's middle is built up, not grouped (its tenements, its blocks and towers)
        if ((r.sclass == SC_CITY || r.sclass == SC_CAPITAL) && (age == AGE_INDUSTRIAL || age == AGE_MODERN) && std::sqrt(x * x + z * z) < 0.6 * std::max(1.0, r.size)) return dwelling(x, z, heading) ? 1 : 0;
        const double fxh = std::sin(heading), fzh = std::cos(heading), sxh = std::cos(heading), szh = -std::sin(heading);
        if (gr == GR_ROWS) {   // three joined side by side (the medieval houses, the prehistoric longhouses, a modern street of terraced houses)
            double hw = age == AGE_MODERN ? 3.4 : (age == AGE_PREHISTORIC ? 2.2 : 2.7), hd = age == AGE_MODERN ? 5.0 : (age == AGE_PREHISTORIC ? 5.5 : 3.6);
            if (age == AGE_MEDIEVAL && G.of(GV_FRONTAGE) == FR_EAVES) std::swap(hw, hd);   // R-412: their eaves to the street
            const double step = 2 * hw + 0.05;
            if (!fits(x, z, std::sqrt(9 * hw * hw + hd * hd) + 1.0)) return 0;
            const int kind = age == AGE_MODERN ? BK_SUBURB : (age == AGE_PREHISTORIC ? BK_LONGHOUSE : BK_HOUSE);
            const double h = age == AGE_MODERN ? 5.8 : (age == AGE_PREHISTORIC ? 1.8 : var(2.8, 0.6) * heightK);
            int placed = 0;
            rectNext = true;
            for (int k = -1; k <= 1; k++) {
                const double ox = x + sxh * k * step, oz = z + szh * k * step;
                if (kind == BK_LONGHOUSE) { if (add(kind, ox * 1.0 + sxh * k * 1.6, oz + szh * k * 1.6, heading, hw, hd, h, 0.8, false)) placed++; }   // a longhouse's walls of turf apart
                else if (add(kind, ox, oz, heading, hw, hd, h, 0.45, false)) placed++;
                if (kind == BK_HOUSE && placed) r.buildings.back().door = 0;   // their doorways to the street
                if (placed) r.buildings.back().joined = true;   // R-412
            }
            rectNext = false;
            return placed;
        }
        // a compound: two or three round a court, a low wall on the court's open sides. T-02: the houses drawn first and set as far out as
        // their width needs, so neighbours never overlap (a people of wide houses overlapped them, their doorways shut)
        const int n = 2 + rng.irange(2);
        const double spreadA = n == 2 ? 1.6 : 1.2;
        struct H { double hw, hd, h; } hs[3];
        double wMax = 0, dMax = 0;
        for (int k = 0; k < n; k++) {
            if (age == AGE_PREHISTORIC) { const double rr = var(2.0, 0.6); hs[k] = {rr, rr, 1.6}; }
            else if (age == AGE_MODERN) hs[k] = {var(3.4, 1.0), var(4.2, 1.0), 2.9};
            else if (age == AGE_INDUSTRIAL) hs[k] = {var(3.6, 1.0), var(3.6, 1.0), 6.5};
            else hs[k] = {var(2.2, 0.8), var(2.6, 0.8), var(2.6, 0.6) * heightK};
            wMax = std::max(wMax, std::max(hs[k].hw, age == AGE_PREHISTORIC ? hs[k].hd : 0.0)); dMax = std::max(dMax, hs[k].hd);
        }
        const double Rp = std::max((age == AGE_MODERN ? 9.0 : 6.5) * 0.62, (wMax + 0.6) / std::sin(spreadA * 0.5) + (age >= AGE_MEDIEVAL ? 0.8 : 0.0));   // a court's form (a court, an L) is wider
        const double R = Rp + dMax * 0.4;
        if (!fits(x, z, R + 2.5) || !circleOffStreets(x, z, Rp + dMax + 0.5)) return 0;
        int placed = 0;
        for (int k = 0; k < n; k++) {
            const double a = heading + PI + (k - (n - 1) * 0.5) * spreadA, hx = x + std::sin(a) * (Rp + hs[k].hd * 0.3), hz = z + std::cos(a) * (Rp + hs[k].hd * 0.3);
            const double hdg = a + PI;   // facing the court
            if (age == AGE_PREHISTORIC) { if (add(BK_HUT, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.5, false)) placed++; }
            else if (age == AGE_MODERN) { if (add(BK_SUBURB, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.35, false)) placed++; }
            else if (age == AGE_INDUSTRIAL) { if (add(BK_VILLA, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.45, false)) placed++; }
            else { if (add(BK_HOUSE, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.45, false)) { placed++; if (r.buildings.back().form == PF_RECT || r.buildings.back().form == PF_FREE) r.buildings.back().door = 0; } }
        }
        if (placed == 0) return 0;
        for (size_t i = r.buildings.size() - placed; i < r.buildings.size(); i++) r.buildings[i].joined = true;   // R-412: round their court
        for (int s = -1; s <= 1; s += 2)   // the court's walls on its two open sides (its gap to the street)
            add(BK_WALL, x + sxh * s * (R + 0.6) + fxh * 0.5, z + szh * s * (R + 0.6) + fzh * 0.5, heading + PI / 2, R * 0.55, 0.3, age == AGE_PREHISTORIC ? 1.0 : 1.4, 0.6, false);
        return placed;
    }
    // the centre of a radial village or a town, the age's: A-04 by the people's centre (a temple, a palace, an assembly, a market, a tomb,
    // a machine) and its ambition; `big` a town's
    bool centre(double x, double z, double heading, bool big, bool checkFit) {
        const double u = rng.uni(), A = 0.75 + 0.8 * G.at(GV_AMBITION), Ah = 0.7 + 1.0 * G.at(GV_AMBITION);
        const int ce = G.of(GV_CENTRE);
        (void)u;
        switch (age) {
            case AGE_PREHISTORIC:
                switch (ce) {
                    case CE_PALACE: return add(BK_LONGHOUSE, x, z, heading, (4.0 + 1.5 * rng.uni()) * A, (12 + 6 * rng.uni()) * A, (2.2 + 0.4 * rng.uni()) * Ah, 1.0, checkFit);
                    case CE_ASSEMBLY: { double rr = ((big ? 12 : 8) + 5 * rng.uni()) * A; return add(BK_STONE_CIRCLE, x, z, heading, rr, rr, 3.0 + 1.5 * rng.uni(), 0.5, checkFit, 1); }
                    case CE_MARKET: return add(BK_MENHIR, x, z, heading, 1.0, (12 + 8 * rng.uni()) * A, 2.6 + 1.0 * rng.uni(), 0.5, checkFit, 1);
                    case CE_TOMB: return add(BK_DOLMEN, x, z, heading, 1.6 * A, 2.4 * A, 2.2 * Ah, 0.5, checkFit, 1);
                    case CE_MACHINE: return add(BK_MENHIR, x, z, heading, 1.0, (16 + 10 * rng.uni()) * A, 2.8 + 1.5 * rng.uni(), 0.5, checkFit);
                    default: { double rr = ((big ? 10 : 6) + 6 * rng.uni()) * A; return add(BK_STONE_CIRCLE, x, z, heading, rr, rr, (2.8 + 1.5 * rng.uni()) * Ah, 0.5, checkFit); }
                }
            case AGE_INDUSTRIAL:
                switch (ce) {
                    case CE_PALACE: return add(BK_TOWN_HALL, x, z, heading, (14 + 5 * rng.uni()) * A, (8.5 + 2.5 * rng.uni()) * A, (14 + 4 * rng.uni()) * Ah, 0.8, checkFit, G.of(GV_FOCUS) == FO_SPIRE || G.of(GV_FOCUS) == FO_TOWER ? 2 : 0);
                    case CE_ASSEMBLY: return add(BK_TOWN_HALL, x, z, heading, (14 + 5 * rng.uni()) * A, (10 + 2.5 * rng.uni()) * A, (14 + 4 * rng.uni()) * Ah, 0.8, checkFit, 1);
                    case CE_MARKET: return add(BK_STATION, x, z, heading, (12 + 4 * rng.uni()) * A, (18 + 8 * rng.uni()) * A, 10 + 3 * rng.uni(), 0.8, checkFit, 1);
                    case CE_TOMB: { double rr = (6 + 2 * rng.uni()) * A; return add(BK_TEMPLE, x, z, heading, rr, rr, (9 + 3 * rng.uni()) * Ah, 1.0, checkFit, 3); }
                    case CE_MACHINE: return add(BK_MILL, x, z, heading, (8 + 3 * rng.uni()) * A, (15 + 7 * rng.uni()) * A, (18 + 7 * rng.uni()) * Ah, 0.7, checkFit);
                    default: return add(BK_CHURCH, x, z, heading, (6.5 + 2 * rng.uni()) * A, (13 + 6 * rng.uni()) * A, (11 + 4 * rng.uni()) * Ah, 0.8, checkFit);
                }
            case AGE_MODERN:
                switch (ce) {
                    case CE_TEMPLE: return add(BK_TEMPLE, x, z, heading, (9 + 3 * rng.uni()) * A, (18 + 6 * rng.uni()) * A, (16 + 6 * rng.uni()) * Ah, 1.1, checkFit);
                    case CE_ASSEMBLY: { double R = (40 + 25 * rng.uni()) * A; return add(BK_STADIUM, x, z, heading, R, R, (18 + 14 * rng.uni()) * Ah, 0.5, checkFit); }
                    case CE_MARKET: return add(BK_BLOCK, x, z, heading, (28 + 10 * rng.uni()) * A, (18 + 6 * rng.uni()) * A, (10 + 4 * rng.uni()) * Ah, 0.5, checkFit, 1);
                    case CE_TOMB: return add(BK_TEMPLE, x, z, heading, (10 + 3 * rng.uni()) * A, (16 + 4 * rng.uni()) * A, (14 + 4 * rng.uni()) * Ah, 1.1, checkFit, 2);
                    case CE_MACHINE: return add(BK_POWER, x, z, heading, (45 + 15 * rng.uni()) * A, (40 + 10 * rng.uni()) * A, 1, 0.5, checkFit);
                    default: { double hw = 13 + 5 * rng.uni(); return add(BK_HIGHRISE, x, z, heading, hw, hw * (0.75 + 0.25 * rng.uni()), ((big ? 180 : 70) + (big ? 260 : 60) * rng.uni()) * Ah * towerK, 0.5, checkFit); }
                }
            default:
                switch (ce) {
                    case CE_PALACE: return add(BK_HALL, x, z, heading, (10 + 3 * rng.uni()) * A, (10 + 3 * rng.uni()) * A, (6 + 2 * rng.uni()) * Ah, 0.8, checkFit, 2);
                    case CE_ASSEMBLY: { double R = ((big ? 15 : 11) + 5 * rng.uni()) * A; return add(BK_STADIUM, x, z, heading, R, R, (5.5 + 2.5 * rng.uni()) * Ah, 0.5, checkFit); }
                    case CE_MARKET: return add(BK_HALL, x, z, heading, (5.5 + 1.5 * rng.uni()) * A, (9 + 4 * rng.uni()) * A, (5 + 2 * rng.uni()) * Ah, 0.6, checkFit, 1);
                    case CE_TOMB: return add(BK_TEMPLE, x, z, heading, ((big ? 7 : 5) + 2 * rng.uni()) * A, ((big ? 15 : 10) + 5 * rng.uni()) * A, ((big ? 11 : 8) + 3 * rng.uni()) * Ah, 1.1, checkFit, c.roof == RF_DOMED || c.roof == RF_CONICAL ? 3 : 2);
                    case CE_MACHINE: return add(BK_WINDMILL, x, z, heading, (3.5 + 1.5 * rng.uni()) * A, (3.5 + 1.5 * rng.uni()) * A, (12 + 4 * rng.uni()) * Ah * towerK, 0.7, checkFit);
                    default:
                        if (big || u < 0.5) return add(BK_TEMPLE, x, z, heading, ((big ? 7 : 5) + 2 * rng.uni()) * A, ((big ? 15 : 10) + 5 * rng.uni()) * A, ((big ? 11 : 8) + 3 * rng.uni()) * Ah, 1.1, checkFit);
                        if (u < 0.75) return add(BK_PLATFORM, x, z, heading, 6 + 3 * rng.uni(), 6 + 3 * rng.uni(), 3.3, 0.5, checkFit);
                        return add(BK_HALL, x, z, heading, 5 + 2 * rng.uni(), 8 + 4 * rng.uni(), 5 + 2 * rng.uni(), 0.7, checkFit);
                }
        }
    }
    // the people's focal accent near its centre, where the centre does not carry it already (a basilica's spires, a domed temple's dome)
    bool focusCarried() const {
        if (r.buildings.empty()) return false;
        const Building& cb = r.buildings.front();
        const int fo = G.of(GV_FOCUS);
        if (fo == FO_SPIRE) return (cb.kind == BK_TEMPLE && cb.style == 0) || cb.kind == BK_CHURCH || (cb.kind == BK_TOWN_HALL && cb.style == 2);
        if (fo == FO_DOME) return (cb.kind == BK_TEMPLE && (cb.style == 1 || cb.style == 3)) || (cb.kind == BK_TOWN_HALL && cb.style == 1);
        return false;
    }
    bool focus(double x, double z, double heading, double scale = 1.0) {
        const int fo = G.of(GV_FOCUS);
        double hw, hd, h; focusSize(c, fo, rng, hw, hd, h);
        hw *= std::sqrt(scale); hd *= std::sqrt(scale); if (fo != FO_POOL) h *= scale;
        if (fo == FO_RING) h = 2 * hw;
        return add(BK_FOCUS, x, z, heading, hw, hd, fo == FO_MAST || fo == FO_SPIRE || fo == FO_TOWER || fo == FO_OBELISK || fo == FO_COLOSSUS ? h * towerK : h, 0.5, true, fo);
    }
    // a tower toward a town's edge, the age's: C-01's tower (a keep); a standing stone; a mill with its chimney or a water tower; a tower;
    // A-04: two joined by their bridge (a people of bridges), all of them as tall as the people's appetite for height
    bool tower(double x, double z, double heading) {
        const double u = rng.uni();
        const bool twin = G.of(GV_LINKS) == LK_BRIDGES && age >= AGE_MEDIEVAL;
        switch (age) {
            case AGE_PREHISTORIC: return add(BK_MENHIR, x, z, heading, 1.0, 0.8, (4 + 4 * rng.uni()) * towerK, 0.5);
            case AGE_INDUSTRIAL:
                if (twin) { const double rr = 3 + rng.uni(); return add(BK_TWIN, x, z, heading, rr * 3.4, rr, (28 + 12 * rng.uni()) * towerK, 0.6, true); }
                if (u < 0.72) return add(BK_MILL, x, z, heading, 7 + 3 * rng.uni(), 13 + 7 * rng.uni(), (15 + 7 * rng.uni()) * towerK, 0.7);
                if (u < 0.88) return add(BK_WATER_TOWER, x, z, heading, 5 + 1.5 * rng.uni(), 5 + 1.5 * rng.uni(), (16 + 8 * rng.uni()) * towerK, 0.5);
                return add(BK_GASOMETER, x, z, heading, 13 + 6 * rng.uni(), 13 + 6 * rng.uni(), 16 + 10 * rng.uni(), 0.5);
            case AGE_MODERN:
                if (twin && u < 0.7) { const double rr = 9 + 4 * rng.uni(); return add(BK_TWIN, x, z, heading, rr * 3.2, rr, (70 + 130 * rng.uni()) * towerK, 0.5, true); }
                if (u < 0.7) { double hw = 10 + 5 * rng.uni(); return add(BK_HIGHRISE, x, z, heading, hw, hw * (0.75 + 0.25 * rng.uni()), (60 + 140 * rng.uni()) * towerK, 0.5); }
                return add(BK_MAST, x, z, heading, 3, 3, (120 + 120 * rng.uni()) * towerK, 0.5);
            default:
                if (twin) { const double rr = 2.6 + rng.uni(); return add(BK_TWIN, x, z, heading, rr * 3.6, rr, (14 + 8 * rng.uni()) * towerK, 0.7, true); }
                return add(BK_TOWER, x, z, heading, 2.2 + 1.0 * rng.uni(), 2.2 + 1.0 * rng.uni(), (9 + 7 * rng.uni()) * towerK, 0.7);
        }
    }
    // the hall beside a town's centre: C-01's; a longhouse; a station; a stadium
    bool hall(double x, double z, double heading) {
        switch (age) {
            case AGE_PREHISTORIC: return add(BK_LONGHOUSE, x, z, heading, 3.2 + 1.0 * rng.uni(), 9 + 4 * rng.uni(), 1.9 + 0.4 * rng.uni(), 0.9);
            case AGE_INDUSTRIAL: return add(BK_STATION, x, z, heading, 14 + 4 * rng.uni(), 24 + 10 * rng.uni(), 11 + 3 * rng.uni(), 0.8);
            case AGE_MODERN: { double R = 40 + 25 * rng.uni(); return add(BK_STADIUM, x, z, heading, R, R, 18 + 14 * rng.uni(), 0.5); }
            default: return add(BK_HALL, x, z, heading, 5 + 1.5 * rng.uni(), 6.0 + 1.5 * rng.uni(), 5 + 2 * rng.uni(), 0.7);
        }
    }
    // a stela's spot: A-04 the people's monument (a stela, an obelisk, a statue, an arch, a pillar); a standing stone (a trilithon for a
    // people of arches) before the medieval age
    bool stela(double x, double z, double heading, double h, bool checkFit) {
        const int mn = G.of(GV_MONUMENTS);
        if (age == AGE_PREHISTORIC) {
            if (mn == MN_ARCHES) return add(BK_FOCUS, x, z, heading, 2.4, 0.8, h * 1.1, 0.4, checkFit, FO_GATE);
            return add(BK_MENHIR, x, z, heading, 0.8, 0.6, h * (mn == MN_OBELISKS || mn == MN_PILLARS ? 1.6 : 1.1), 0.4, checkFit);
        }
        if (age >= AGE_FUTURISTIC) return false;
        switch (mn) {
            case MN_OBELISKS: return add(BK_STELA, x, z, heading, 0.6, 0.6, h, 0.3, checkFit, MN_OBELISKS);
            case MN_COLOSSI: return add(BK_STELA, x, z, heading, 0.9, 0.9, h * 1.2, 0.3, checkFit, MN_COLOSSI);
            case MN_ARCHES: return add(BK_STELA, x, z, heading, 2.6, 0.8, h, 0.3, checkFit, MN_ARCHES);
            case MN_PILLARS: return add(BK_STELA, x, z, heading, 0.7, 0.7, h, 0.3, checkFit, MN_PILLARS);
            default: return add(BK_STELA, x, z, heading, 0.45, 0.25, h, 0.3, checkFit, MN_STELAE);
        }
    }
};

// C-08: the terrain's features. A harbour where a shore is within reach (a quay along the shore, a mole out from its end into the
// water, bollards, a warehouse behind; on a dead desert world the harbour stands over the dry bed), terraces where the ground
// slopes (retaining walls along the contour between the houses, a wall's height apart), on a desert a cistern at the upslope
// edge with its channel and a crescent of wall against the wind, and cairns out along the ridge where the relief is mountain
bool clearOfBuildings(const RuinSpec& r, double x, double z, double margin) {
    for (const Building& b : r.buildings) {
        double dx = x - b.x, dz = z - b.z;
        double lx = dx * std::cos(b.heading) - dz * std::sin(b.heading), lz = dx * std::sin(b.heading) + dz * std::cos(b.heading);
        if (std::fabs(lx) < b.hw + margin && std::fabs(lz) < b.hd + margin) return false;
    }
    return true;
}
// a wall segment of half length `half` across heading `h` (its axis along x') is clear along its whole length (nine points)
bool segmentClear(const RuinSpec& r, double x, double z, double h, double half, double margin) {
    double ax = std::cos(h), az = -std::sin(h);
    for (int k = -4; k <= 4; k++) { double t = half * k / 4.0; if (!clearOfBuildings(r, x + ax * t, z + az * t, margin)) return false; }
    return true;
}
// A-03: the features each age built: a prehistoric people the cairns; a medieval one all of them; an industrial one no cairns; a modern
// one its harbours and terraces; the futuristic and far-future ages none. T-02: an industrial and a modern people's reservoirs
enum { FT_HARBOUR = 1, FT_TERRACES = 2, FT_CISTERN = 4, FT_WINDWALL = 8, FT_CAIRNS = 16, FT_RESERVOIR = 32 };
const int AGE_FEATURES[AGE_COUNT] = {FT_CAIRNS, 31, FT_HARBOUR | FT_TERRACES | FT_WINDWALL | FT_RESERVOIR, FT_HARBOUR | FT_TERRACES | FT_RESERVOIR, 0, 0};
void addFeatures(Layout& L, RuinSpec& r, const Culture& c) {
    Rng& rng = L.rng;
    const SiteRead& st = r.site;
    const double R = r.size;
    const int ft = AGE_FEATURES[clampi(c.age, 0, AGE_COUNT - 1)];
    bool big = r.sclass == SC_VILLAGE || townOrGreater(r.sclass);
    // the harbour (T-02: by the age: C-08's quay along the shore and its mole; an industrial people's dock, its basin between two piers, its
    // cranes on the edge and its warehouse behind; a modern people's container port, its apron, its gantries over the berth, its stacks of
    // containers on the land behind; a city's berths side by side along its shore)
    if ((ft & FT_HARBOUR) && st.shoreDist < 500 + R && (big || rng.chance(0.5)) && r.sclass != SC_MONUMENT) {
        const double nx = std::sin(st.shoreDir), nz = std::cos(st.shoreDir);    // toward the water
        const double ax = std::cos(st.shoreDir), az = -std::sin(st.shoreDir);   // along the shore (the width of a work facing the water)
        const bool city = r.sclass == SC_CITY || r.sclass == SC_CAPITAL;
        const int age = c.age;
        const int berths = city ? (age == AGE_MODERN ? 2 : 3) : 1;
        const double q = st.shoreDist - 3.0;
        for (int k = 0; k < berths; k++) {
            const double hw = age == AGE_MODERN ? 55 + 45 * rng.uni() : (age == AGE_INDUSTRIAL ? 28 + 18 * rng.uni() : 10 + 12 * rng.uni());
            const double hd = age == AGE_MODERN ? 16 + 10 * rng.uni() : (age == AGE_INDUSTRIAL ? 5.0 : 2.8);
            const double off = berths > 1 ? (k - (berths - 1) * 0.5) * (2 * hw + 40) : 0;
            const double cx = nx * q + ax * off, cz = nz * q + az * off;
            if (!segmentClear(r, cx, cz, st.shoreDir, hw, hd + 1.5) || !L.circleOffStreets(cx - nx * hd, cz - nz * hd, 0.5)) continue;   // clear of the houses along its length
            if (!L.add(BK_QUAY, cx, cz, st.shoreDir, hw, hd, 1.2, 0.5, false)) continue;
            r.buildings.back().district = DI_HARBOUR;
            r.harbour = true; r.harbourKind = age <= AGE_MEDIEVAL ? 0 : (age == AGE_INDUSTRIAL ? 1 : 2);
            const size_t first = r.buildings.size();
            if (age <= AGE_MEDIEVAL) {   // C-08's mole out from an end, a warehouse behind
                const double end = rng.chance(0.5) ? 1 : -1, len = 12 + 12 * rng.uni();
                const double ex = cx + ax * end * (hw - 1.5), ez = cz + az * end * (hw - 1.5);
                L.add(BK_MOLE, ex + nx * (hd + len * 0.5), ez + nz * (hd + len * 0.5), st.shoreDir, 1.0, len * 0.5, 1.6, 0.9, false);
                L.add(BK_HALL, cx - nx * (hd + 7.5), cz - nz * (hd + 7.5), st.shoreDir, 4.5 + 1.5 * rng.uni(), 5.5 + 1.5 * rng.uni(), 4 + 1.5 * rng.uni(), 0.6);
            } else if (age == AGE_INDUSTRIAL) {   // the dock: two piers from its ends (the basin between), its cranes, the warehouse behind
                const double len = 30 + 30 * rng.uni();
                for (int s2 = -1; s2 <= 1; s2 += 2) L.add(BK_MOLE, cx + ax * s2 * (hw - 2) + nx * (hd + len * 0.5), cz + az * s2 * (hw - 2) + nz * (hd + len * 0.5), st.shoreDir, 2.0, len * 0.5, 1.8, 1.0, false);
                const int cranes = 2 + rng.irange(2);
                for (int j = 0; j < cranes; j++) { const double u = -hw + 2 * hw * (j + 0.5) / cranes; L.add(BK_CRANE, cx + ax * u - nx * hd * 0.3, cz + az * u - nz * hd * 0.3, st.shoreDir, 2.2, 2.2, 14 + 6 * rng.uni(), 0.5, false, 0); }
                L.add(BK_HALL, cx - nx * (hd + 12), cz - nz * (hd + 12), st.shoreDir, hw * 0.7, 8 + 3 * rng.uni(), 8 + 3 * rng.uni(), 0.7);
            } else {   // the container port: the gantries on the apron's edge, the stacks behind on the land
                const int cranes = 3 + rng.irange(3);
                for (int j = 0; j < cranes; j++) { const double u = -hw + 2 * hw * (j + 0.5) / cranes; L.add(BK_CRANE, cx + ax * u + nx * hd * 0.1, cz + az * u + nz * hd * 0.1, st.shoreDir, 9 + 3 * rng.uni(), 9, 30 + 12 * rng.uni(), 0.5, false, 1); }
                const int rows = 2 + rng.irange(2), cols = std::max(1, (int)(hw / 40));
                for (int i = 0; i < cols; i++)
                    for (int j = 0; j < rows; j++) {
                        const double u = -hw + 2 * hw * (i + 0.5) / cols, v = -(hd + 14 + j * 26);
                        L.add(BK_CONTAINERS, cx + ax * u + nx * v, cz + az * u + nz * v, st.shoreDir, std::min(32.0, hw / cols - 4), 9, 10, 0.5);
                    }
            }
            for (size_t i = first; i < r.buildings.size(); i++) r.buildings[i].district = DI_HARBOUR;
        }
    }
    // T-02: a reservoir upslope of an industrial or a modern town (on a desert where there is room; on a living world a town's on a slope): a
    // basin of masonry in its bank, a round tank of concrete
    if ((ft & FT_RESERVOIR) && r.sclass != SC_MONUMENT && r.sclass != SC_HAMLET && r.sclass != SC_WORKS &&
        (st.desert ? rng.chance(big ? 0.8 : 0.4) : (townOrGreater(r.sclass) && st.slope > 0.02 && rng.chance(0.5)))) {
        const double head = st.slope > 0.02 ? st.downhill + PI : rng.uni() * TAU;
        const bool round = c.age == AGE_MODERN;
        for (int tries = 0; tries < 8 && !r.reservoir; tries++) {
            const double rad = 18 + 14 * rng.uni(), hw = round ? rad : 16 + 12 * rng.uni(), hd = round ? rad : 12 + 8 * rng.uni();
            const double a = head + rng.sym(0.6), rr = R + std::max(hw, hd) + 14 + 24 * rng.uni();
            if (L.add(BK_RESERVOIR, std::sin(a) * rr, std::cos(a) * rr, a, hw, hd, round ? 7 + 3 * rng.uni() : 4 + 2 * rng.uni(), 0.8, true, round ? 1 : 0)) r.reservoir = true;
        }
    }
    // the terraces
    const bool terracer = c.genome.of(GV_LAND) == LD_TERRACED && c.age <= AGE_MODERN;   // A-04: a people of terraces terraced any slope, in any age
    if (((ft & FT_TERRACES) || terracer) && st.slope > (terracer ? 0.02 : 0.06) && r.sclass != SC_MONUMENT) {
        double step = clampd(1.4 / st.slope, 8, 30), h = 1.3 + 0.6 * rng.uni();
        double dx = std::sin(st.downhill), dz = std::cos(st.downhill), ax = std::cos(st.downhill), az = -std::sin(st.downhill);
        const double Rt = std::min(R, 80.0);   // A-03: the terraces of the gardens round a village's or a town's middle (a wider town's would run to hundreds)
        double d0 = -std::floor(Rt / step) * step + step * rng.uni();
        int segs = 0;
        for (double d = d0; d < Rt; d += step) {
            double w = std::sqrt(std::max(0.0, Rt * Rt - d * d)) * 0.9;
            for (double tt = -w + 3; tt < w - 3; tt += 6.2) {
                double x = dx * d + ax * tt, z = dz * d + az * tt;
                if (!segmentClear(r, x, z, st.downhill, 3.0, 1.2) || !L.offStreets(x, z, st.downhill, 3.0, 0.4)) continue;   // the whole segment, not only its middle; T-02: off the streets
                if (L.add(BK_TERRACE, x, z, st.downhill, 3.0, 0.4, h, 0.4, false)) segs++;
            }
        }
        r.terraced = segs > 0;
    }
    // a desert's cistern and its wind wall
    if (st.desert && r.sclass != SC_MONUMENT) {
        if ((ft & FT_CISTERN) && rng.chance(big ? 0.8 : 0.4)) {
            double head = st.slope > 0.02 ? st.downhill + PI : rng.uni() * TAU;
            for (int tries = 0; tries < 6 && !r.cistern; tries++) {
                double a = head + rng.sym(0.5), rr = R * (0.75 + 0.2 * rng.uni()), rad = 2.5 + 1.5 * rng.uni();
                if (L.add(BK_CISTERN, std::sin(a) * rr, std::cos(a) * rr, a, rad, rad, 2.0, 0.7)) r.cistern = true;
            }
        }
        if ((ft & FT_WINDWALL) && rng.chance(big ? 0.7 : 0.35)) {
            double rw = R + 14, hWall = 2.2 + 1.0 * rng.uni();
            int segs = std::max(4, (int)(rw * 1.9 / 7.4)), placed = 0;
            for (int k = 0; k < segs; k++) {
                double a = st.wind + (-0.95 + 1.9 * (k + 0.5) / segs);
                if (rng.chance(0.15)) continue;   // fallen
                if (!L.offStreets(std::sin(a) * rw, std::cos(a) * rw, a + PI / 2, 3.5, 0.5)) continue;   // T-02: a gap where a street goes out
                if (L.add(BK_WINDWALL, std::sin(a) * rw, std::cos(a) * rw, a + PI / 2, 3.5, 0.5, hWall, 0.5, false)) placed++;
            }
            r.windwall = placed > 0;
        }
    }
    // the cairns on the ridge
    if ((ft & FT_CAIRNS) && st.relief > 0.45 && rng.chance(0.7)) {
        int n = 3 + rng.irange(4), placed = 0;
        for (int k = 0; k < n; k++) {
            double d = R + 12 + 14 * k + rng.sym(3), a = st.ridgeDir + rng.sym(0.12);
            if (!L.offStreets(std::sin(a) * d, std::cos(a) * d, a, 1.1, 1.1)) continue;   // T-02
            if (L.add(BK_CAIRN, std::sin(a) * d, std::cos(a) * d, a, 1.1, 1.1, 2.1, 0.5, false)) placed++;
        }
        r.cairns = placed > 0;
    }
}

// ---- T-02: the towns ------------------------------------------------------------------------------------------------------------

// what is known of a place's past: the ages it was built in, inside out (B-414: one, its people's last: a world shows the age its people
// reached alone, T-02's old cores and a place left in an older age's way are gone with that age), the years before its people's end it was
// left (0: used to the end), its founding year (the history's; 0 the countryside's), and whether it was left before its people's last age
// began (`gone`: nothing of it on the ground)
struct TownHist { std::vector<int> ages; double before = 0; int founded = 0; bool gone = false; };

std::shared_ptr<const WorldPlan> planFor(const BodyGen& g);   // below

// the class's radius in an age (the plan's cores for a capital and a city; C-01's town's and village's by the age's scale)
double classRadiusIn(int sclass, int age) {
    static const double CAP[AGE_COUNT] = {110, 470, 820, 1150, 480, 480}, CITY[AGE_COUNT] = {90, 270, 500, 700, 230, 230};
    const int a = clampi(age, 0, AGE_COUNT - 1);
    if (sclass == SC_CAPITAL) return CAP[a];
    if (sclass == SC_CITY) return CITY[a];
    return (sclass == SC_VILLAGE ? 57.0 : 75.0) * ageScale(a);
}
// the bearings of the plan's ways from a place of it (radians from north)
std::vector<double> planExits(const BodyGen& g, const RuinSpec& r) {
    std::vector<double> ex;
    if (r.rank < 0 || !worldHadCivilisation(g)) return ex;
    const std::shared_ptr<const WorldPlan> P = planFor(g);
    const PeoplePlan& pp = P->plans[r.people > 0 && P->peoples > 1 ? 1 : 0];
    if (r.rank >= (int)pp.places.size()) return ex;
    const PlanPlace& me = pp.places[r.rank];
    for (const PlanEdge& e : pp.edges) {
        if (e.a != r.rank && e.b != r.rank) continue;
        const PlanPlace& o = pp.places[e.a == r.rank ? e.b : e.a];
        ex.push_back(std::atan2(std::remainder(o.lon - me.lon, TAU) * std::cos(me.lat), o.lat - me.lat));
    }
    return ex;
}
// a village's, a town's or a city's plan as its ages built it (see towns.h): its layers, its centre and its square, its streets, its buildings
// along them by district (each layer in its age's way, the people's genome through it), its wall and gates, its works of the water and the
// land, its fields. `n` the buildings its class asks for (a city's the plan's), `R` its reach. B-414: one layer, its people's last age
// R-412: the people's street furniture (`GV_FURNITURE`; `BK_FURNITURE`, its style the `Furniture`): a town's well, fountain or figure in each
// square and beside the middle of each long main street, its lamps along its main streets and round its squares; a futuristic people's
// round its great structures. Laid last, by a stream of its own (nothing of the plan moves for it)
void placeFurniture(Layout& L, RuinSpec& r, double R) {
    const int fu = L.G.of(GV_FURNITURE), age = L.age;
    if (fu == FU_NONE || age < AGE_MEDIEVAL || age > AGE_FUTURISTIC) return;
    Rng rng(r.seed ^ 0xF0412ULL);
    const double S = fu == FU_FOUNTAINS ? (age == AGE_MODERN ? 3.2 : 2.7) : (fu == FU_WELLS ? 1.0 : (fu == FU_STATUES ? 1.0 : 0.25));
    auto put = [&](double x, double z, double heading) {
        const size_t n0 = r.buildings.size();
        if (!L.add(BK_FURNITURE, x, z, heading, S, S, fu == FU_LAMPS ? 4.0 : 2.0, 0.3, true, fu)) return false;
        r.buildings[n0].district = 0;
        return true;
    };
    int lamps = 0, pieces = 0;
    if (r.streets.empty()) {   // a futuristic people's: round its middle, past its great structure
        const int n = fu == FU_LAMPS ? 10 : 3;
        for (int k = 0; k < n; k++) {
            const double a = r.heading + (k + 0.5) * TAU / n;
            for (int q = 0; q < 8; q++) { const double d = R * (0.2 + 0.1 * q) + 6 * rng.uni(); if (put(std::sin(a) * d, std::cos(a) * d, a + PI)) break; }
        }
        return;
    }
    for (const Plaza& p : r.plazas) {
        if (fu == FU_LAMPS) { for (int k = 0; k < 4 && lamps < 60; k++) { const double a = PI / 4 + k * PI / 2; if (put(p.x + std::sin(a) * p.r * 0.85, p.z + std::cos(a) * p.r * 0.85, a)) lamps++; } continue; }
        static const double OFF[5][2] = {{0, 0}, {0.5, 0}, {-0.5, 0}, {0, 0.5}, {0, -0.5}};
        for (int t = 0; t < 5 && pieces < 12; t++) if (put(p.x + OFF[t][0] * p.r, p.z + OFF[t][1] * p.r, std::atan2(-p.x, -p.z + 1e-9))) { pieces++; break; }
    }
    for (const Street& s : r.streets) {
        if (s.rank < SR_MAIN) continue;
        const double len = std::hypot(s.bx - s.ax, s.bz - s.az);
        if (len < 20) continue;
        const double ux = (s.bx - s.ax) / len, uz = (s.bz - s.az) / len, nx = -uz, nz = ux;
        if (fu == FU_LAMPS) {
            int side = 1;
            for (double t = 8; t < len - 6 && lamps < 60; t += 22, side = -side)
                if (put(s.ax + ux * t + nx * side * (s.half + 0.8), s.az + uz * t + nz * side * (s.half + 0.8), std::atan2(-nx * side, -nz * side))) lamps++;   // at the kerb, before the yards' bounds
        } else if (len > 50 && pieces < 12) {
            const double t = len * (0.35 + 0.3 * rng.uni()), off = s.half + S + 0.8;
            const int side = rng.chance(0.5) ? 1 : -1;
            if (put(s.ax + ux * t + nx * side * off, s.az + uz * t + nz * side * off, std::atan2(-nx * side, -nz * side))) pieces++;
        }
    }
}

void layoutTown(RuinSpec& r, const Culture& c, const BodyGen& g, int n, double R, const TownHist& th, double luckS) {
    const Genome& G = c.genome;
    const int nl = (int)th.ages.size(), townAge = th.ages.back();
    const bool big = r.sclass == SC_CITY || r.sclass == SC_CAPITAL, village = r.sclass == SC_VILLAGE;
    const double Rt = std::max(30.0, R * classRadiusIn(r.sclass, townAge) / classRadiusIn(r.sclass, c.age));
    const double h0 = r.heading;
    r.buildAge = -1;   // B-414: its people's own age
    // the layers, each its ring and its builder: the people's genome through the layer's age, weathered from when the place was left
    std::vector<Culture> lc(nl);
    std::vector<std::unique_ptr<Layout>> LL;
    r.layers.clear();
    double prev = 0;
    for (int k = 0; k < nl; k++) {
        lc[k] = cultureAtAge(c, th.ages[k], th.before);
        TownLayer tl; tl.age = th.ages[k]; tl.r0 = prev;
        tl.r1 = k + 1 == nl ? Rt : std::min(Rt - 40, std::max(prev + 30, Rt * classRadiusIn(r.sclass, th.ages[k]) / classRadiusIn(r.sclass, townAge)));
        prev = tl.r1;
        r.layers.push_back(tl);
    }
    for (int k = 0; k < nl; k++) { LL.emplace_back(new Layout(r, lc[k], 0x7051ULL * (k + 1))); LL.back()->luckS = luckS; LL.back()->obb = true; }
    Layout& Lc = *LL.front();   // the oldest: the centre's
    Layout& Lo = *LL.back();    // the town's own age
    Rng& rng = Lo.rng;
    BuildingGrid bg; bg.init(Rt + 500);
    for (auto& l : LL) l->grid = &bg;
    const SiteRead& st = r.site;
    const int pub = G.of(GV_PUBLIC), links = G.of(GV_LINKS), grouping = G.of(GV_GROUPING), edge = G.of(GV_EDGE);
    const double fx = std::sin(h0), fz = std::cos(h0), sx = std::cos(h0), sz = -std::sin(h0);
    // the centre, in the oldest layer's way (a modern city's medieval temple at its heart), its monuments and its focal accent before it; the
    // square round them
    Lc.centre(0, 0, h0, !village, false);
    double cReach = 6;
    if (!r.buildings.empty()) { const Building& cb = r.buildings.back(); cReach = std::sqrt(cb.hw * cb.hw + cb.hd * cb.hd); }
    {
        const Building cb = r.buildings.empty() ? Building() : r.buildings.back();
        const double front = (r.buildings.empty() ? 6 : cb.hd) + 5;
        if (Lc.age < AGE_FUTURISTIC) for (int side = -1; side <= 1; side += 2) if (rng.chance(0.7)) Lc.stela(side * 4.5 * sx + fx * (front + 2), side * 4.5 * sz + fz * (front + 2), h0, 3.0 + 2.0 * rng.uni(), true);
        if (!Lc.focusCarried()) for (int k = 0; k < 4; k++) { const double d = front + 6 + k * 7; if (Lc.focus(fx * d, fz * d, h0 + PI, village ? 0.6 : 1.0)) break; }
    }
    for (Building& b : r.buildings) { b.district = DI_CENTRE; cReach = std::max(cReach, std::hypot(b.x, b.z) + std::sqrt(b.hw * b.hw + b.hd * b.hd)); }
    double plaza = (townAge <= AGE_MEDIEVAL ? 12 : (townAge == AGE_INDUSTRIAL ? 18 : 26)) * (big ? 1.6 : (village ? 0.7 : 1.0));
    if (pub == PU_PLAZAS) plaza *= 1.4; else if (pub == PU_NONE) plaza *= 0.75;
    plaza = std::max(plaza, cReach + 4 + streetHalf(SR_MAIN, townAge) * (big ? 1.0 : (village ? 0.6 : 0.75)));   // the main streets' ends clear of it
    // the streets: the people's pattern through each layer's age (a village one street or two, a green, or lanes)
    TownSpec T; T.R = Rt; T.h0 = h0; T.sclass = r.sclass; T.number = G.number(); T.layers = r.layers; T.plaza = plaza; T.seed = r.seed;
    T.pattern = G.of(GV_STREETS);
    if (village) T.pattern = T.pattern == ST_GRID || T.pattern == ST_CONTOUR ? ST_LINE : (T.pattern == ST_RADIAL ? ST_CONCENTRIC : T.pattern);
    T.exits = planExits(g, r);
    // the wall of the medieval layer (decided before the streets: its lesser streets end inside it, a street runs round inside it, the main
    // streets go out by its gates): the town's edge, or an old core's inside a later city (its line the later boulevard)
    int walledK = -1;
    for (int k = 0; k < nl; k++) if (th.ages[k] == AGE_MEDIEVAL) walledK = k;
    r.walled = walledK >= 0 && edge != ED_NOTHING && rng.chance(big || r.sclass == SC_TOWN ? 0.85 : 0.3);
    const double wallR = r.walled ? (walledK + 1 == nl ? Rt * 1.02 : r.layers[walledK].r1 - 1.5) : 1e9;
    if (r.walled) { T.walled = true; T.wallR = wallR; T.wallK = walledK; }
    // the shore: the streets end before the water (and the harbour's room)
    const bool shoreNear = st.shoreDist < Rt + 80;
    if (shoreNear) { T.shore = true; T.shoreNx = std::sin(st.shoreDir); T.shoreNz = std::cos(st.shoreDir); T.shoreLimit = st.shoreDist - (townAge >= AGE_INDUSTRIAL ? 34 : 8); }
    townStreets(T, r.streets, r.plazas);
    // what its streets could hold against what it built: a town that could not line its streets lays wider blocks (the streets again, the
    // blocks the root of the ratio wider: the frontage goes as the blocks' inverse)
    auto frontage = [&]() {
        double lots = 0;
        for (const Street& s : r.streets) {
            if (s.rank == SR_RING) continue;
            const double rr = std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5);
            if (rr > Rt + 4 || rr < plaza + 3) continue;
            const int age = th.ages[std::min(nl - 1, [&] { for (int k = 0; k < nl; k++) if (rr < r.layers[k].r1 + 1) return k; return nl - 1; }())];
            lots += 2 * std::hypot(s.bx - s.ax, s.bz - s.az) / (age <= AGE_MEDIEVAL ? 7.0 : (age == AGE_INDUSTRIAL ? 22.0 : 28.0));
        }
        return lots;
    };
    {
        const double want = (big ? n : (village ? 1.8 * n : 3.2 * n)) * 1.6;
        const double have = frontage();
        if (have > want * 1.25) {
            T.spacing = clampd(have / want, 1.0, 3.0);
            townStreets(T, r.streets, r.plazas);
        }
    }
    StreetGrid sg; sg.build(r.streets, Rt + 500);
    for (auto& l : LL) { l->sgrid = &sg; l->streets = &r.streets; }
    // the districts
    double ringR = 1e9;
    for (const Street& s : r.streets) if (s.rank == SR_RING) ringR = std::min(ringR, std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5));
    const double coreR = nl > 1 ? r.layers[0].r1 : 0;
    const double worksDir = st.wind + PI;   // downwind: the smoke away from the town
    const bool harbourOn = shoreNear && townAge >= AGE_MEDIEVAL && !village;
    auto layerAt = [&](double rr) { for (int k = 0; k < nl; k++) if (rr < r.layers[k].r1 + 1) return k; return nl - 1; };
    auto districtAt = [&](double x, double z, int k) {
        const double rr = std::hypot(x, z), dd = rr / Rt, a = std::atan2(x, z);
        if (rr < plaza + 3) return (int)DI_CENTRE;
        if (harbourOn && std::fabs(std::remainder(a - st.shoreDir, TAU)) < 0.85 && rr > std::max(0.45 * Rt, st.shoreDist - std::max(70.0, 0.3 * Rt))) return (int)DI_HARBOUR;
        if (k == 0 && nl > 1) return (int)DI_CORE;
        const int age = th.ages[k];
        // a modern city's towers round its old core (the modern age building over the ring before it) or in its middle
        if (townAge == AGE_MODERN && big && rr < std::max(coreR, plaza) + std::max(90.0, 0.13 * Rt)) return (int)DI_TOWERS;
        if ((age == AGE_INDUSTRIAL || age == AGE_MODERN) && !village && dd > 0.3 && std::fabs(std::remainder(a - worksDir, TAU)) < (big ? 0.55 : 0.45)) return (int)DI_WORKS;
        if (age == AGE_MODERN && k + 1 == nl) {
            if (rr > ringR + 5) return (int)DI_SUBURBS;
            if (!big && dd > 0.78) return (int)DI_SUBURBS;
        }
        return (int)DI_HOUSES;
    };
    auto density = [&](int di, int age, double dd) {
        switch (di) {
            case DI_CORE: return 0.95;
            case DI_TOWERS: return 0.85;
            case DI_SUBURBS: return 0.6 - 0.3 * clampd((dd - 0.7) / 0.3, 0, 1);   // thinning into the fields
            case DI_WORKS: return 0.6;
            case DI_HARBOUR: return 0.55;
            default: return (age <= AGE_MEDIEVAL ? 0.97 : 0.9) - 0.4 * dd * dd;   // falling from the centre
        }
    };
    auto lotWidth = [&](int di, int age) {   // a lot's frontage, its gap with it
        switch (di) {
            case DI_TOWERS: return 34.0;
            case DI_SUBURBS: return 15.0;
            case DI_WORKS: return 30.0;
            case DI_HARBOUR: return 26.0;
            default: return age <= AGE_MEDIEVAL ? 6.5 : (age == AGE_INDUSTRIAL ? 21.0 : 26.0);
        }
    };
    // how much of the frontage is built: the class's count of buildings over what the streets could hold
    double expected = 0;
    for (const Street& s : r.streets) {
        if (s.rank == SR_RING) continue;
        const double mx = (s.ax + s.bx) * 0.5, mz = (s.az + s.bz) * 0.5, rr = std::hypot(mx, mz);
        if (rr > Rt + 4) continue;
        const int k = layerAt(rr), di = districtAt(mx, mz, k);
        if (di == DI_CENTRE) continue;
        expected += 2 * std::hypot(s.bx - s.ax, s.bz - s.az) / lotWidth(di, th.ages[k]) * density(di, th.ages[k], rr / Rt);
    }
    const double target = big ? n : (village ? 1.8 * n : 3.2 * n);
    const double p0 = clampd(target / std::max(1.0, expected), 0.35, 1.0);
    const int cap = (int)std::lround(target * 1.35) + 12;
    // a lot's building by its district and its ring's age: 0 one building, 1 a compound (houses round a court), 2 a tower, 3 a walled garden
    bool mainFront = false;   // the lot fronts a main street near the middle (a medieval town's main streets are rows whatever the people's grouping)
    auto lotSpec = [&](Layout& Lk, int di, int age, double dd, BSpec& sp, int& kind) {
        kind = 0;
        const double F = Lk.footK, Hk = Lk.heightK, u = Lk.rng.uni();
        auto set = [&](int k, double hw, double hd, double h, double t, int style = -1) { sp.kind = k; sp.hw = hw; sp.hd = hd; sp.h = h; sp.thick = t; sp.style = style; return true; };
        switch (di) {
            case DI_TOWERS:
                if (u < 0.3 + 0.55 * G.at(GV_HEIGHT)) { const double hw = 10 + 5 * Lk.rng.uni(); return set(BK_HIGHRISE, hw, hw * (0.75 + 0.25 * Lk.rng.uni()), (70 + 260 * Lk.rng.uni()) * clampd(1.25 - dd, 0.35, 1.2) * Lk.towerK, 0.5); }
                return set(BK_BLOCK, Lk.var(11, 4) * F, Lk.var(6.0, 2.5) * F * Lk.hdK, (22 + 30 * Lk.rng.uni()) * clampd(1.25 - dd, 0.5, 1.2) * Hk, 0.4);
            case DI_SUBURBS: return set(BK_SUBURB, Lk.var(4.0, 1.5) * F, Lk.var(5.0, 1.5) * F * Lk.hdK, (Lk.rng.chance(0.5) ? 2.9 : 5.8) * std::min(1.4, Hk), 0.35);
            case DI_WORKS:
                if (age == AGE_INDUSTRIAL) {
                    if (u < 0.42) return set(BK_MILL, 7 + 3 * Lk.rng.uni(), 13 + 7 * Lk.rng.uni(), (15 + 7 * Lk.rng.uni()) * Lk.towerK, 0.7);
                    if (u < 0.62) return set(BK_HALL, 7 + 3 * Lk.rng.uni(), 12 + 8 * Lk.rng.uni(), 7 + 2 * Lk.rng.uni(), 0.7);   // a warehouse
                    if (u < 0.72) { const double rr = 5 + 1.5 * Lk.rng.uni(); return set(BK_WATER_TOWER, rr, rr, (16 + 8 * Lk.rng.uni()) * Lk.towerK, 0.5); }
                    if (u < 0.8) { const double rr = 13 + 6 * Lk.rng.uni(); return set(BK_GASOMETER, rr, rr, 16 + 10 * Lk.rng.uni(), 0.5); }
                    return set(BK_ROW, Lk.var(10.0, 4.0) * F, Lk.var(4.2, 1.0) * F * Lk.hdK, Lk.var(6.5, 3.0) * Hk, 0.45);   // the workers' rows
                }
                if (u < 0.3) return set(BK_TANKS, 18 + 10 * Lk.rng.uni(), 9 + 4 * Lk.rng.uni(), 15 + 10 * Lk.rng.uni(), 0.5);
                if (u < 0.8) return set(BK_BLOCK, 18 + 10 * Lk.rng.uni(), 11 + 6 * Lk.rng.uni(), 8 + 5 * Lk.rng.uni(), 0.4);   // a warehouse, a works hall
                if (u < 0.88) return set(BK_MAST, 3, 3, (60 + 80 * Lk.rng.uni()) * Lk.towerK, 0.5);
                return set(BK_BLOCK, Lk.var(11, 4) * F, Lk.var(6.0, 2.5) * F * Lk.hdK, (15 + 20 * Lk.rng.uni()) * Hk, 0.4);
            case DI_HARBOUR:   // the warehouses of the waterside
                if (age <= AGE_MEDIEVAL) return set(BK_HALL, 4.5 + 1.5 * Lk.rng.uni(), 6 + 3 * Lk.rng.uni(), 4 + 1.5 * Lk.rng.uni(), 0.6);
                if (age == AGE_INDUSTRIAL) return set(BK_HALL, 8 + 3 * Lk.rng.uni(), 14 + 8 * Lk.rng.uni(), 8 + 2 * Lk.rng.uni(), 0.7);
                return set(BK_BLOCK, 18 + 10 * Lk.rng.uni(), 12 + 6 * Lk.rng.uni(), 9 + 5 * Lk.rng.uni(), 0.4);
            default: break;
        }
        // the houses: a tower now and then toward the edge, a walled garden, a compound, or the age's dwelling (taller toward a city's middle)
        if (age <= AGE_INDUSTRIAL && dd > 0.45 && c.towers && Lk.rng.chance(0.035 * (0.5 + 1.5 * G.at(GV_HEIGHT)))) { kind = 2; sp.hw = sp.hd = 4; return true; }
        if (pub == PU_GARDENS && age >= AGE_MEDIEVAL && age <= AGE_INDUSTRIAL && Lk.rng.chance(0.12)) { kind = 3; sp.hw = sp.hd = std::max(5.0, 0.38 * streetSpacing(age) * 0.5); return true; }
        if (grouping == GR_COMPOUNDS && age <= AGE_MODERN && age != AGE_INDUSTRIAL && di == DI_HOUSES && !(big && dd < 0.6)) { kind = 1; sp.hw = sp.hd = (age == AGE_MODERN ? 9.0 : 6.5) + 2.5; return true; }
        if ((grouping == GR_ROWS || mainFront) && age == AGE_MEDIEVAL && Lk.rng.uni() < 0.85) {   // a row's houses joined along the street, gabled cell by cell where the people built in cells
            set(BK_HOUSE, 2.7 * (0.95 + 0.1 * Lk.rng.uni()), 3.6 * (0.92 + 0.16 * Lk.rng.uni()), Lk.var(2.8, 0.6) * Hk, 0.45);
            Lk.frontage(sp);   // R-412
            sp.rect = true;
            if (big) sp.h *= 0.85 + 0.55 * (1 - dd);
            return true;
        }
        if (!Lk.dwellingSpec(dd, sp)) return false;
        if (sp.kind == BK_HOUSE || sp.kind == BK_ROW || sp.kind == BK_TENEMENT || sp.kind == BK_BLOCK || sp.kind == BK_VILLA) sp.h *= big ? 0.85 + 0.55 * (1 - dd) : 0.92 + 0.2 * (1 - dd);
        return true;
    };
    // R-412: how far a medieval, an industrial or a modern people set its houses back from the street (`GV_SETBACK`): at its edge, or in
    // front yards and gardens
    auto setback = [&](int di, int age, int kind, const Genome& G) {
        const double S = age >= AGE_MEDIEVAL && age <= AGE_MODERN ? G.at(GV_SETBACK) : 0.3;
        if (kind == BK_VILLA || kind == BK_SUBURB) return (4.0 + 3.0 * rng.uni()) * (0.6 + 0.8 * S);
        switch (di) {
            case DI_TOWERS: return 6.0 + 6.0 * rng.uni();
            case DI_WORKS: case DI_HARBOUR: return 3.0 + 3.0 * rng.uni();
            default: return age <= AGE_MEDIEVAL ? 0.4 + 4.5 * S * S : (age == AGE_INDUSTRIAL ? 0.8 + 5.0 * S * S : 2.0 + 7.0 * S * S);
        }
    };
    auto gapAfter = [&](int di, int age, int kind) {
        if (kind == BK_SUBURB) return 4.0 + 6.0 * rng.uni();
        switch (di) {
            case DI_TOWERS: return 6.0 + 8.0 * rng.uni();
            case DI_WORKS: case DI_HARBOUR: return 4.0 + 5.0 * rng.uni();
            default:
                if (age == AGE_INDUSTRIAL) return kind == BK_ROW ? 0.2 : 3.0 + 3.0 * rng.uni();
                if (age == AGE_MODERN) return 4.0 + 4.0 * rng.uni();
                return grouping == GR_ROWS || (mainFront && kind == BK_HOUSE) ? 0.06 : (grouping == GR_DETACHED ? 1.5 + 3.0 * rng.uni() : 0.6 + 1.2 * rng.uni());
        }
    };
    // a special building fronting a street: the first spot it fits along the streets that pass the test
    auto front = [&](Layout& Lk, const BSpec& sp, const std::function<bool(const Street&)>& want, int di) {
        for (const Street& s : r.streets) {
            if (!want(s)) continue;
            const double L = std::hypot(s.bx - s.ax, s.bz - s.az), ux = (s.bx - s.ax) / L, uz = (s.bz - s.az) / L, nx = -uz, nz = ux;
            for (int side = -1; side <= 1; side += 2)
                for (double t = sp.hw + 2; t < L - sp.hw - 2; t += 6) {
                    const double off = s.half + 3 + sp.hd, cx = s.ax + ux * t + nx * side * off, cz = s.az + uz * t + nz * side * off;
                    if (std::hypot(cx, cz) > Rt + 5) continue;
                    const size_t n0 = r.buildings.size();
                    if (Lk.add(sp.kind, cx, cz, std::atan2(-nx * side, -nz * side), sp.hw, sp.hd, sp.h, sp.thick, true, sp.style)) {
                        if (Lk.intrusion(n0, r.site) > 0) { Lk.undoTo(n0); continue; }
                        r.buildings.back().district = di; return true;
                    }
                }
        }
        return false;
    };
    // the station of an industrial ring on a main street a third of the way out; a medieval town's market hall by the square
    for (int k = 0; k < nl; k++) {
        Layout& Lk = *LL[k];
        if (th.ages[k] == AGE_INDUSTRIAL && !village) {
            BSpec sp; sp.kind = BK_STATION; sp.hw = 14 + 4 * Lk.rng.uni(); sp.hd = 24 + 10 * Lk.rng.uni(); sp.h = 11 + 3 * Lk.rng.uni(); sp.thick = 0.8;
            front(Lk, sp, [&](const Street& s) { const double rr = std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5); return s.rank >= SR_MAIN && s.age == AGE_INDUSTRIAL && rr > std::max(r.layers[k].r0, Rt * 0.25) && rr < Rt * 0.8; }, DI_HOUSES);
        }
        if (th.ages[k] == AGE_MEDIEVAL && k == 0 && !village && Lk.rng.chance(0.7)) {
            BSpec sp; sp.kind = BK_HALL; sp.style = 1; sp.hw = 5.5 + 1.5 * Lk.rng.uni(); sp.hd = 9 + 4 * Lk.rng.uni(); sp.h = 5 + 2 * Lk.rng.uni(); sp.thick = 0.6;
            front(Lk, sp, [&](const Street& s) { return std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5) < plaza + 40; }, DI_CENTRE);
        }
    }
    // the lots: the streets inner first, each side walked from end to end, a building fronting the street where its lot is built
    std::vector<int> order(r.streets.size());
    for (size_t i = 0; i < order.size(); i++) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const Street& A = r.streets[a]; const Street& B = r.streets[b];
        const double da = std::hypot(A.ax + A.bx, A.az + A.bz), db = std::hypot(B.ax + B.bx, B.az + B.bz);
        return da != db ? da < db : a < b;
    });
    int placed = 0, gardens = 0;
    const Street* lotStreet = nullptr;
    // a lot's building fronting its street: a house's door to it (R-412: before its pieces, a porch among them, are tested against the streets),
    // its front yard's depth to it
    auto fronting = [&](size_t i, double off) {
        Building& b = r.buildings[i];
        if (b.kind == BK_HOUSE && b.form != PF_FREE && b.form != PF_COURT) b.door = 0;
        b.yard = std::max(0.0, off - lotStreet->half - b.hd);
    };
    for (int si : order) {
        if (placed >= cap) break;
        const Street s = r.streets[si];
        lotStreet = &s;
        if (s.rank == SR_RING) continue;
        const double L = std::hypot(s.bx - s.ax, s.bz - s.az);
        if (L < 6) continue;
        const double ux = (s.bx - s.ax) / L, uz = (s.bz - s.az) / L, nx = -uz, nz = ux;
        for (int side = -1; side <= 1 && placed < cap; side += 2) {
            double t = 1.0;
            while (t < L - 2 && placed < cap) {
                const double mx = s.ax + ux * t, mz = s.az + uz * t;
                const double qx = mx + nx * side * (s.half + 6), qz = mz + nz * side * (s.half + 6), qr = std::hypot(qx, qz);
                if (qr > Rt + 6) { t += 4; continue; }
                const int lr = layerAt(qr), di = districtAt(qx, qz, lr);
                const int lk = di == DI_TOWERS ? nl - 1 : lr;   // the towers the town's own age's, whatever ring they rose in
                Layout& Lk = *LL[lk];
                const int age = th.ages[lk];
                if (di == DI_CENTRE) { t += 4; continue; }
                BSpec sp; int kind = 0;
                mainFront = s.rank >= SR_MAIN && qr < 0.55 * Rt && (di == DI_HOUSES || di == DI_CORE);
                if (!lotSpec(Lk, di, age, qr / Rt, sp, kind)) { t += 4; continue; }
                const double rankK = s.rank >= SR_MAIN ? 1.35 : (s.rank == SR_LANE ? 0.8 : 1.0);   // the main streets built up the most
                if (Lk.rng.uni() > p0 * density(di, age, qr / Rt) * rankK) { t += 2 * sp.hw + gapAfter(di, age, sp.kind); continue; }   // an empty lot: a yard, a gap
                const double along = t + sp.hw;
                if (along + sp.hw > L - 1) break;
                const double sb = setback(di, age, sp.kind, Lk.G), heading = std::atan2(-nx * side, -nz * side);
                double off = s.half + sb + sp.hd;
                double cx = s.ax + ux * along + nx * side * off, cz = s.az + uz * along + nz * side * off;
                const double reach = std::sqrt(sp.hw * sp.hw + sp.hd * sp.hd);
                if (lr == walledK && std::hypot(cx, cz) + reach > wallR - 1.5) { t += 3; continue; }   // inside its wall
                bool ok = false;
                const size_t before = r.buildings.size();
                if (kind == 1) { if (Lk.circleOffStreets(cx, cz, sp.hw + 0.5) && Lk.dwellingSpot(cx, cz, heading) > 0) ok = true; }
                else if (kind == 2) { if (Lk.circleOffStreets(cx, cz, 6.0) && Lk.tower(cx, cz, heading)) ok = true; }
                else if (kind == 3) {   // a walled garden, its pool
                    const double gw = sp.hw;
                    if (Lk.fits(cx, cz, gw + 1) && Lk.circleOffStreets(cx, cz, gw * 1.42 + 0.5)) {
                        for (int s4 = 0; s4 < 4; s4++) { const bool al = s4 % 2 == 0; const double ox = (s4 == 1 ? gw : (s4 == 3 ? -gw : 0)), oz = (s4 == 0 ? gw : (s4 == 2 ? -gw : 0)); const double hc = std::cos(heading), hs = std::sin(heading); Lk.add(BK_WALL, cx + hc * ox + hs * oz, cz - hs * ox + hc * oz, heading + (al ? 0 : PI / 2), gw * (s4 == 0 ? 0.6 : 1.0), 0.3, 0.8, 0.5, false); }
                        Lk.add(BK_FOCUS, cx, cz, heading, gw * 0.4, gw * 0.3, 0.8, 0.5, false, FO_POOL);
                        ok = true; gardens++;
                    }
                } else {
                    Lk.arcadeNext = links == LK_ARCADES && s.rank >= SR_MAIN && qr < 0.4 * Rt && age <= AGE_INDUSTRIAL;
                    Lk.rectNext = sp.rect;
                    ok = Lk.add(sp.kind, cx, cz, heading, sp.hw, sp.hd, sp.h, sp.thick, true, sp.style);
                    if (!ok && sp.hd > 3.5 && sp.kind != BK_HIGHRISE) {   // a shallower building where the block is narrow
                        sp.hd *= 0.65; off = s.half + sb + sp.hd;
                        cx = s.ax + ux * along + nx * side * off; cz = s.az + uz * along + nz * side * off;
                        ok = Lk.add(sp.kind, cx, cz, heading, sp.hw, sp.hd, sp.h, sp.thick, true, sp.style);
                    }
                    Lk.arcadeNext = false;
                    if (ok) fronting(before, off);   // R-412: its door to the street before its pieces are tested against the streets
                }
                if (ok) {   // its own pieces off the street (a chimney, a bank, a jetty past its footprint): set back by as much once, else not built
                    const double in1 = Lk.intrusion(before, r.site);
                    if (in1 > 0) {
                        Lk.undoTo(before);
                        ok = false;
                        if (kind == 0) {
                            off += in1 + 0.3;
                            cx = s.ax + ux * along + nx * side * off; cz = s.az + uz * along + nz * side * off;
                            Lk.rectNext = sp.rect;
                            if ((lr != walledK || std::hypot(cx, cz) + reach <= wallR - 1.5) && Lk.add(sp.kind, cx, cz, heading, sp.hw, sp.hd, sp.h, sp.thick, true, sp.style)) {
                                fronting(before, off);
                                if (Lk.intrusion(before, r.site) > 0) Lk.undoTo(before); else ok = true;
                            }
                            Lk.rectNext = false;
                        }
                    }
                }
                Lk.rectNext = false;
                if (!ok) { t += 2.5; continue; }
                double hwNow = sp.hw;
                for (size_t i = before; i < r.buildings.size(); i++) {
                    Building& b = r.buildings[i];
                    b.district = di;
                    if (kind == 1 || (di != DI_TOWERS && di != DI_WORKS && di != DI_HARBOUR && ((age == AGE_INDUSTRIAL && b.kind == BK_ROW) || (age <= AGE_MEDIEVAL && (grouping == GR_ROWS || (mainFront && b.kind == BK_HOUSE))))))
                        b.joined = true;   // in a row or a compound
                    if (i == before && kind == 0) hwNow = std::max(hwNow, b.hw);
                }
                placed += kind == 3 ? 0 : (int)std::max<size_t>(1, r.buildings.size() - before);
                // colonnades along a main street near the centre (a people of colonnades)
                if (links == LK_COLONNADES && kind == 0 && s.rank >= SR_MAIN && qr < 0.35 * Rt && age >= AGE_MEDIEVAL && age <= AGE_INDUSTRIAL)
                    Lk.add(BK_COLONNADE, s.ax + ux * along + nx * side * (s.half + 1.2), s.az + uz * along + nz * side * (s.half + 1.2), std::atan2(ux, uz), 0.5, hwNow * 0.8, 3.5, 0.4, true);
                t = along + hwNow + gapAfter(di, age, sp.kind);
            }
        }
    }
    (void)gardens;
    // a modern city's stadium and its power station, beyond its edge (along a way out; downwind)
    if (townAge == AGE_MODERN && big) {
        for (int tries = 0; tries < 10; tries++) {
            const double Rs = 40 + 25 * rng.uni(), a = (T.exits.empty() ? h0 : T.exits[tries % T.exits.size()]) + 0.45 + rng.sym(0.2), d = Rt + Rs + 30;
            if (Lo.add(BK_STADIUM, std::sin(a) * d, std::cos(a) * d, a + PI, Rs, Rs, 18 + 14 * rng.uni(), 0.5, true)) { r.buildings.back().district = DI_SUBURBS; break; }
        }
        if (G.of(GV_CENTRE) != CE_MACHINE)
            for (int tries = 0; tries < 8; tries++) {
                const double a = worksDir + rng.sym(0.4), d = Rt + 120 + 40 * tries;
                if (Lo.add(BK_POWER, std::sin(a) * d, std::cos(a) * d, a + PI, 45 + 15 * rng.uni(), 40 + 10 * rng.uni(), 1, 0.5, true)) { r.buildings.back().district = DI_WORKS; break; }
            }
    }
    // the wall: a ring round the walled layer, its gates where the main streets go through it (an arch over the street between its towers),
    // the stretches between them, a breach where a lesser street crosses; an old core's inside a later city mostly gone (stretches kept)
    if (r.walled) {
        Layout& Lw = *LL[walledK];
        const bool old = walledK + 1 < nl;
        const bool pal = edge == ED_DITCHES;
        double hWall = 4.0 + 2.0 * rng.uni(); if (pal) hWall *= 0.65;
        // the gates
        std::vector<std::pair<double, double>> gates;   // the angle, the half width of the street
        for (const Street& s : r.streets) {
            if (s.rank < SR_MAIN) continue;
            const double dx = s.bx - s.ax, dz = s.bz - s.az, A = dx * dx + dz * dz, B = 2 * (s.ax * dx + s.az * dz), C = s.ax * s.ax + s.az * s.az - wallR * wallR, D = B * B - 4 * A * C;
            if (A < 1e-9 || D < 0) continue;
            for (int e = -1; e <= 1; e += 2) {
                const double tt = (-B + e * std::sqrt(D)) / (2 * A);
                if (tt < 0 || tt > 1) continue;
                const double a = std::atan2(s.ax + dx * tt, s.az + dz * tt);
                bool dup = false; for (auto& gt : gates) if (std::fabs(std::remainder(gt.first - a, TAU)) * wallR < 12) dup = true;
                if (!dup) gates.push_back({a, s.half});
            }
        }
        for (const auto& gt : gates) {
            const double a = gt.first, gx = std::sin(a) * wallR, gz = std::cos(a) * wallR, hwg = std::max(1.9, gt.second + 0.8);
            if (old && Lw.rng.chance(0.3)) continue;   // a gate pulled down with its wall
            const size_t n0 = r.buildings.size();
            if (Lw.add(BK_GATE, gx, gz, a, hwg, 0.6, hWall + 1.0, 0.8, false)) {
                if (Lw.intrusion(n0, r.site, gx, gz) > 0) { Lw.undoTo(n0); continue; }   // its piers off the streets beside it
                r.buildings.back().district = old ? DI_CORE : DI_HOUSES;
            }
        }
        const int segs = std::max(10, (int)(TAU * wallR / (big ? 22.0 : 14.0)));
        for (int k = 0; k < segs; k++) {
            const double aA = h0 + k * TAU / segs, aB = h0 + (k + 1) * TAU / segs;
            const double xA = std::sin(aA) * wallR, zA = std::cos(aA) * wallR, xB = std::sin(aB) * wallR, zB = std::cos(aB) * wallR;
            const double mx = (xA + xB) * 0.5, mz = (zA + zB) * 0.5, dx = xB - xA, dz = zB - zA, len = std::hypot(dx, dz);
            const double wh = std::atan2(dx, dz) + PI / 2;
            bool nearGate = false;
            for (const auto& gt : gates) if (std::fabs(std::remainder(gt.first - (aA + aB) * 0.5, TAU)) * wallR < len * 0.5 + gt.second + 3) nearGate = true;
            if (nearGate) continue;
            if (!Lw.offStreets(mx, mz, wh, len * 0.5, 0.45)) continue;   // a breach where a lane goes through
            if (Lw.rng.chance(old ? 0.55 : 0.12)) continue;            // a fallen stretch (an old core's wall mostly gone)
            if (!pal && c.towers && k % 2 == 0 && Lw.offStreets(xA, zA, aA, 3.4, 3.4)) {
                const size_t n0 = r.buildings.size();
                if (Lw.add(BK_WALL_TOWER, xA, zA, aA, 2.6 + 0.8 * rng.uni(), 0, (hWall + 3 + 2 * rng.uni()) * (0.8 + 0.4 * G.at(GV_HEIGHT)), 0.5, false) && Lw.intrusion(n0, r.site) > 0) Lw.undoTo(n0);
            }
            if (Lw.add(pal ? BK_PALISADE : BK_WALL, mx, mz, wh, len * 0.5, 0.45, hWall, 0.9, false)) r.buildings.back().district = old ? DI_CORE : DI_HOUSES;
        }
    }
    // the people's relation to the land (a high place toward the ridge, an aqueduct down from the high ground) and its tunnels
    {
        const int land = G.of(GV_LAND);
        const double rd = st.ridgeDir;
        if (land == LD_RIDGES) for (int k = 0; k < 4; k++) { const double d = Rt * (0.72 + 0.1 * k); if (Lo.add(BK_PLATFORM, std::sin(rd) * d, std::cos(rd) * d, rd + PI, 7 + 3 * rng.uni(), 7 + 3 * rng.uni(), 3.3, 0.5, true)) break; }
        if (land == LD_VALLEYS) {
            const double len = 70 + 50 * rng.uni(), d = Rt * 1.04 + len * 0.5 + 6;
            Lo.add(BK_AQUEDUCT, std::sin(rd) * d, std::cos(rd) * d, rd, 1.4, len * 0.5, (9 + 9 * rng.uni()) * (0.8 + 0.4 * G.at(GV_AMBITION)), 1.2, true);
        }
        if (links == LK_TUNNELS)
            for (int k = 0, made = 0; k < 12 && made < 3; k++) { const double a = rng.uni() * TAU, d = Rt * (0.3 + 0.6 * rng.uni()); if (Lo.add(BK_PORTAL, std::sin(a) * d, std::cos(a) * d, a, 2.4, 3.5, 4.8, 0.8, true)) made++; }
    }
    // the works of the water and the land in the town's own age (C-08's by the age: the harbour, the terraces, the cistern or the reservoir)
    const double rSaved = r.size; r.size = Rt;
    addFeatures(Lo, r, Lo.c);
    r.size = rSaved;
    // the fields round it
    townFields(T, townAge, st, kindHas(livingKindOf(g.type), KT_DRY), r.fields);   // P-04: the fields of its time
    placeFurniture(Lo, r, Rt);   // R-412
}

void layoutSettlement(RuinSpec& r, const Culture& c, const BodyGen& g, const TownHist* th = nullptr) {
    Layout L(r, c);
    Rng& rng = L.rng;
    const Genome& G = c.genome;
    int n = 1; double R = 12; MonPick mon;
    settlementSize(r, rng, n, R, c, &mon);
    r.size = R;
    r.site = readSite(g, r);   // C-08: the terrain round it, read before anything is placed (no draw of the layout's stream)
    L.luckS = rng.sym(0.15);   // C-01's settlement's decay draw: its fortune now (A-03)
    // T-02: a village, a town or a city of a medieval, an industrial or a modern people (or of a place left in one of those ages): its plan by
    // its ages (`layoutTown`); a camp, a hamlet, a futuristic people's great places and a lone monument as before
    {
        TownHist own; own.ages.push_back(c.age);
        const TownHist& t = th && !th->ages.empty() ? *th : own;
        r.before = t.before; r.founded = t.founded;
        const int townAge = t.ages.back();
        if ((r.sclass == SC_VILLAGE || townOrGreater(r.sclass)) && townAge >= AGE_MEDIEVAL && townAge <= AGE_MODERN) {
            layoutTown(r, c, g, n, R, t, L.luckS);
            return;
        }
    }
    const double h0 = r.heading, S = ageScale(c.age);
    const double fx = std::sin(h0), fz = std::cos(h0), sx = std::cos(h0), sz = -std::sin(h0);   // along the heading, and across it
    // A-04: the people's edge (a wall, a bank and its ditch, nothing) round its towns, and a village now and then, to the industrial age
    const int edge = G.of(GV_EDGE), streets = G.of(GV_STREETS), pub = G.of(GV_PUBLIC), links = G.of(GV_LINKS), land = G.of(GV_LAND);
    r.walled = c.age <= AGE_INDUSTRIAL && edge != ED_NOTHING && r.sclass >= SC_VILLAGE && r.sclass != SC_MONUMENT && rng.chance(townOrGreater(r.sclass) ? 0.85 : 0.3);
    const double towerShare = 0.5 + 1.5 * G.at(GV_HEIGHT);   // A-04: how many towers the people raised
    // A-03: a work of the industrial or modern age beside a hamlet or a village (a pit's headframe, a mill; tanks, a mast)
    auto work = [&](double x, double z, double head) {
        if (c.age == AGE_INDUSTRIAL) {
            if (r.site.relief > 0.15 && rng.chance(0.5)) return L.add(BK_HEADFRAME, x, z, head, 20 + 6 * rng.uni(), 16 + 4 * rng.uni(), 20 + 8 * rng.uni(), 0.5);
            return L.tower(x, z, head);
        }
        if (c.age == AGE_MODERN) {
            if (rng.chance(0.5)) return L.add(BK_TANKS, x, z, head, 18 + 10 * rng.uni(), 9 + 4 * rng.uni(), 15 + 10 * rng.uni(), 0.5);
            return L.add(BK_MAST, x, z, head, 3, 3, (80 + 100 * rng.uni()) * L.towerK, 0.5);
        }
        return false;
    };
    // A-04: the focal accent before the centre (a town's, and a village's half the time), where the centre does not carry it
    auto focusBefore = [&](double front, double scale) {
        if (r.plan != SP_GRID && c.age < AGE_FUTURISTIC)   // the people's monuments flanking the centre's front (the grid's are its plaza's)
            for (int side = -1; side <= 1; side += 2) L.stela(side * 4.5 * sx + fx * (front + 3), side * 4.5 * sz + fz * (front + 3), h0, 3.0 + 2.0 * rng.uni(), true);
        if (L.focusCarried()) return false;
        for (int k = 0; k < 4; k++) {
            const double d = front + 6 + k * 7;
            if (L.focus(fx * d, fz * d, h0 + PI, scale)) return true;
        }
        return false;
    };
    switch (r.plan) {
        case SP_LONE: {   // one monument alone, the age's
            const int st = mon.kind == BK_FOCUS || mon.kind == BK_STELA ? mon.style : -1;
            if (mon.count == 2) { L.add(mon.kind, -2.5 - mon.hw, 0, h0, mon.hw, mon.hd, mon.hWall, 0.3, false, st); L.add(mon.kind, 2.5 + mon.hw, 0, h0, mon.hw, mon.hd, mon.hWall * (0.75 + 0.5 * rng.uni()), 0.3, false, st); }
            else L.add(mon.kind, 0, 0, h0, mon.hw, mon.hd, mon.hWall, mon.kind == BK_TOWER ? 0.7 : 0.5, false, st);
            r.size = std::max(r.size, 12.0);
            break;
        }
        case SP_CLUSTER: {   // houses scattered (A-04: an organic people's towns and villages round their centre)
            if (r.sclass != SC_HAMLET) { L.centre(0, 0, h0, townOrGreater(r.sclass), false); const Building& cb = r.buildings.back(); focusBefore(cb.hd + 2, townOrGreater(r.sclass) ? 1.0 : 0.6); }
            const int triesMax = r.sclass == SC_CITY || r.sclass == SC_CAPITAL ? 8 : 20;   // T-01: a city's core is full sooner
            for (int k = 0, placed = 0; k < n && placed < n; k++)
                for (int tries = 0; tries < triesMax; tries++) {
                    double a = rng.uni() * TAU, rr = R * 0.85 * std::sqrt(rng.uni());
                    const int p = L.dwellingSpot(std::sin(a) * rr, std::cos(a) * rr, rng.uni() * TAU);
                    if (p) { placed += p; break; }
                }
            if (c.age >= AGE_INDUSTRIAL && rng.chance(0.45)) { double a = rng.uni() * TAU; work(std::sin(a) * (R + 12), std::cos(a) * (R + 12), a + PI); }
            break;
        }
        case SP_STREET: {
            // a street through the centre along the heading (A-04: along a line, on the contour, branching: its side streets); the dwellings
            // on both sides, their fronts to it; a town's centre on its middle
            if (townOrGreater(r.sclass)) { L.centre(sx * (16 + 4 * S), sz * (16 + 4 * S), h0 - PI / 2, true, true); if (!r.buildings.empty()) focusBefore(r.buildings.back().hd, 0.8); }
            const bool branching = streets == ST_BRANCHING;
            const bool arcades = links == LK_ARCADES, colonnades = links == LK_COLONNADES;
            double t = -R * 0.9;
            int placed = 0, step = 0;
            while (t < R * 0.9 && placed < n) {
                for (int side = -1; side <= 1; side += 2) {
                    if (placed >= n) break;
                    if (rng.chance(pub == PU_NONE ? 0.08 : 0.2)) continue;   // a gap
                    double off = (7.5 + 1.5 * rng.uni()) * S;
                    L.arcadeNext = arcades && std::fabs(t) < R * 0.4;
                    placed += L.dwellingSpot(fx * t + sx * side * off, fz * t + sz * side * off, h0 - side * PI / 2);
                    L.arcadeNext = false;
                    if (colonnades && std::fabs(t) < R * 0.35 && step % 2 == 0) L.add(BK_COLONNADE, fx * t + sx * side * 3.2 * S, fz * t + sz * side * 3.2 * S, h0, 0.5, 4.5 * S, 3.5, 0.4, true);
                }
                if (branching && step % 3 == 1 && placed < n - 2) {   // a side street, its houses along it
                    const int side = step % 2 ? 1 : -1;
                    for (int q = 1; q <= 3 && placed < n; q++) {
                        const double d = (12 + 11 * q) * S, cxs = fx * t + sx * side * d, czs = fz * t + sz * side * d;
                        for (int w = -1; w <= 1; w += 2) placed += L.dwellingSpot(cxs + fx * w * 7.5 * S, czs + fz * w * 7.5 * S, w > 0 ? h0 + PI : h0);
                    }
                }
                t += (9.0 + 4.0 * rng.uni()) * S;
                step++;
            }
            if (r.sclass == SC_VILLAGE && rng.chance(0.5)) {   // at the street's head: a stela, a church, a work (A-04: the people's focus half the time)
                const double hx = fx * (R * 0.95), hz = fz * (R * 0.95);
                if (c.age == AGE_INDUSTRIAL && rng.chance(0.6)) L.centre(hx + fx * 12, hz + fz * 12, h0 + PI, false, true);
                else if (c.age >= AGE_INDUSTRIAL) work(hx + fx * 15, hz + fz * 15, h0 + PI);
                else if (rng.chance(0.5)) L.focus(hx + fx * 8, hz + fz * 8, h0 + PI, 0.6);
                else L.stela(hx, hz, h0, 3.0 + 2.0 * rng.uni(), true);
            }
            break;
        }
        case SP_RADIAL: {
            // rings round a centre: the age's centre in the middle, the dwellings facing in (A-04: on spokes from it where the people's
            // streets ran out from the middle, the people's number of them)
            L.centre(0, 0, h0, (townOrGreater(r.sclass) && c.age != AGE_PREHISTORIC) || r.sclass == SC_CAPITAL, true);
            if (townOrGreater(r.sclass) || rng.chance(0.5)) { const double cr = r.buildings.empty() ? 6 : std::max(r.buildings.back().hw, r.buildings.back().hd); focusBefore(cr, townOrGreater(r.sclass) ? 1.0 : 0.6); }
            int placed = 0;
            if (streets == ST_RADIAL && c.age != AGE_PREHISTORIC) {
                const int spokes = clampi(G.number(), 3, 8);
                for (double d = R * 0.32; d < R * 0.95 && placed < n; d += 12.0 * S)
                    for (int k = 0; k < spokes && placed < n; k++) {
                        const double a = h0 + (k + 0.5) * TAU / spokes;
                        for (int side = -1; side <= 1; side += 2) {
                            const double ax = std::sin(a) * d + std::cos(a) * side * 7.5 * S, az = std::cos(a) * d - std::sin(a) * side * 7.5 * S;
                            if (rng.chance(0.15)) continue;
                            placed += L.dwellingSpot(ax, az, a - side * PI / 2);
                        }
                    }
            } else {
                const bool bigCore = r.sclass == SC_CITY || r.sclass == SC_CAPITAL;   // T-01: a city's rings, a street's width apart
                int rings = bigCore ? std::max(2, (int)(R / (14.0 * S))) : (r.sclass == SC_TOWN ? 2 : 1);
                for (int ring = 0; ring < rings && placed < n; ring++) {
                    double rr = bigCore ? R * (0.22 + 0.73 * ring / (rings - 1.0)) : R * (rings == 1 ? 0.6 : (ring == 0 ? 0.45 : 0.82));
                    int m = std::max(4, (int)(TAU * rr / (12.0 * S)));
                    double a0 = rng.uni() * TAU;
                    for (int k = 0; k < m && placed < n; k++) {
                        if (rng.chance(0.15)) continue;
                        double a = a0 + k * TAU / m + rng.sym(0.3 / m);
                        if (ring == rings - 1 && ring > 0 && c.towers && placed < n - 1 && rng.chance(0.08 * towerShare)) {
                            if (L.tower(std::sin(a) * rr, std::cos(a) * rr, a + PI)) placed++;
                            continue;
                        }
                        placed += L.dwellingSpot(std::sin(a) * rr, std::cos(a) * rr, a + PI);
                    }
                }
            }
            if (r.sclass == SC_VILLAGE && c.age >= AGE_INDUSTRIAL && rng.chance(0.5)) { double a = rng.uni() * TAU; work(std::sin(a) * (R + 14), std::cos(a) * (R + 14), a + PI); }
            break;
        }
        case SP_GRID: {
            // streets along the heading and across it, blocks of 17 x 15 m (the age's scale); the plaza at the centre: C-01's platform,
            // colonnade and stelae, or the age's centre (A-04: the people's) with its monuments before it and its focal accent; a hall
            // beside it (a station, a stadium farther out); a tower or two toward the edge (a modern town's towers in its middle);
            // A-04: a plaza kept open round the centre, gardens, the plaza's houses arcaded, colonnades along the main street
            double bx = (17.0 + 3.0 * rng.uni()) * S, bz = (15.0 + 3.0 * rng.uni()) * S;
            const bool c01 = c.age == AGE_MEDIEVAL && G.of(GV_CENTRE) == CE_TEMPLE && rng.chance(0.35);   // C-01's plaza
            if (c01) {
                L.add(BK_PLATFORM, 0, 0, h0, 5 + 2 * rng.uni(), 5 + 2 * rng.uni(), 3.3, 0.5, false);
                L.add(BK_COLONNADE, sx * bx * 0.75, sz * bx * 0.75, h0, 0.5, bz * 0.6, 3.5, 0.4, false);
                if (rng.chance(0.6)) L.stela(-sx * bx * 0.5 + fx * bz * 0.55, -sz * bx * 0.5 + fz * bz * 0.55, h0, 3.0 + 2.0 * rng.uni(), false);
                if (rng.chance(0.6)) L.stela(-sx * bx * 0.5 - fx * bz * 0.55, -sz * bx * 0.5 - fz * bz * 0.55, h0, 3.0 + 2.0 * rng.uni(), false);
            } else {
                L.centre(0, 0, h0, true, false);
                const Building& cb = r.buildings.back();
                double front = cb.hd + 7;
                for (int side = -1; side <= 1; side += 2) if (rng.chance(0.6)) L.stela(side * 5 * sx + fx * front, side * 5 * sz + fz * front, h0, 3.0 + 2.0 * rng.uni(), true);
                focusBefore(front + 2, 1.0);
            }
            if (c.age == AGE_INDUSTRIAL || c.age == AGE_MODERN) {   // the station or the stadium beside the centre, on the cross axis
                const Building& cb = r.buildings[0];
                double rc = std::sqrt(cb.hw * cb.hw + cb.hd * cb.hd) + 1.2;
                for (int side = -1; side <= 1; side += 2) {
                    size_t before = r.buildings.size();
                    double est = c.age == AGE_MODERN ? 95 : 40, d = rc + est + 3;
                    L.hall(sx * side * d, sz * side * d, h0 + side * PI / 2);
                    if (r.buildings.size() > before) break;
                }
            }
            int nx = (int)std::ceil(R / bx), nz = (int)std::ceil(R / bz);
            std::vector<std::pair<double, std::pair<int, int>>> nodes;
            for (int i = -nx; i <= nx; i++)
                for (int j = -nz; j <= nz; j++) {
                    if (i == 0 && j == 0) continue;
                    double d = std::sqrt(i * bx * i * bx + j * bz * j * bz);
                    if (d > R * 0.95) continue;
                    nodes.push_back({d + rng.uni() * 4.0, {i, j}});
                }
            std::sort(nodes.begin(), nodes.end());
            bool hallDone = c.age == AGE_INDUSTRIAL || c.age == AGE_MODERN; int towersDone = 0, placed = 0;
            const int towersMax = (int)std::lround((c.age == AGE_INDUSTRIAL ? 4 : 2) * towerShare * (r.sclass == SC_CITY || r.sclass == SC_CAPITAL ? R / 120.0 : 1.0));   // T-01: a city's many
            const double plaza = pub == PU_PLAZAS ? R * 0.24 : 0;
            int gardenK = 0;
            for (const auto& nd : nodes) {
                if (placed >= n) break;
                int i = nd.second.first, j = nd.second.second;
                if (rng.chance(pub == PU_NONE ? 0.04 : 0.12)) continue;   // a yard, or a house gone to nothing
                double x = sx * i * bx + fx * j * bz, z = sz * i * bx + fz * j * bz;
                double head = std::abs(i) >= std::abs(j) ? (i > 0 ? h0 - PI / 2 : h0 + PI / 2) : (j > 0 ? h0 + PI : h0);   // the fronts to the centre
                if (!hallDone && std::abs(i) + std::abs(j) == 1) {
                    hallDone = L.hall(x, z, head);
                    if (hallDone) { placed++; continue; }
                }
                if (nd.first < plaza) continue;   // the plaza kept open round the centre
                if (pub == PU_GARDENS && (++gardenK) % 5 == 0) {   // a walled garden, its pool
                    const double gw = std::min(bx, bz) * 0.38;
                    for (int s4 = 0; s4 < 4; s4++) { const bool al = s4 % 2 == 0; const double ox = (s4 == 1 ? gw : (s4 == 3 ? -gw : 0)), oz = (s4 == 0 ? gw : (s4 == 2 ? -gw : 0)); L.add(BK_WALL, x + sx * ox + fx * oz, z + sz * ox + fz * oz, h0 + (al ? 0 : PI / 2), gw * (s4 == 0 ? 0.6 : 1.0), 0.3, 0.8, 0.5, false); }
                    if (c.age >= AGE_MEDIEVAL) L.add(BK_FOCUS, x, z, h0, gw * 0.4, gw * 0.3, 0.8, 0.5, false, FO_POOL);
                    continue;
                }
                if (c.age == AGE_MODERN && nd.first < R * 0.45 && rng.chance(0.25 + 0.6 * G.at(GV_HEIGHT))) {   // the towers of the middle, lower outward
                    double hw = 10 + 5 * rng.uni();
                    if (L.add(BK_HIGHRISE, x, z, head, hw, hw * (0.75 + 0.25 * rng.uni()), (80 + 260 * rng.uni()) * (1.1 - nd.first / R) * L.towerK, 0.5)) { placed++; continue; }
                }
                if (c.towers && towersDone < towersMax && nd.first > R * 0.5 && rng.chance(c.age == AGE_INDUSTRIAL ? 0.4 : 0.3)) {
                    if (L.tower(x, z, head)) { towersDone++; placed++; continue; }
                }
                L.arcadeNext = links == LK_ARCADES && std::abs(i) + std::abs(j) <= 2;
                placed += L.dwellingSpot(x, z, head);
                L.arcadeNext = false;
            }
            if (links == LK_COLONNADES)   // colonnades along the main street
                for (int j = -2; j <= 2; j++) for (int side = -1; side <= 1; side += 2) { if (j == 0) continue; L.add(BK_COLONNADE, sx * side * bx * 0.5 + fx * j * bz, sz * side * bx * 0.5 + fz * j * bz, h0, 0.5, bz * 0.42, 3.5, 0.4, true); }
            break;
        }
        case SP_GREAT: {   // A-03: a futuristic people's few great structures and the land between left wild
            if (r.sclass == SC_HAMLET) {
                for (int k = 0; k < n; k++)
                    for (int tries = 0; tries < 20; tries++) { double a = rng.uni() * TAU, rr = R * 0.8 * std::sqrt(rng.uni()); if (L.dwelling(std::sin(a) * rr, std::cos(a) * rr, rng.uni() * TAU)) break; }
                break;
            }
            const bool town = townOrGreater(r.sclass);
            const double A = (0.75 + 0.8 * G.at(GV_AMBITION)) * (r.sclass == SC_CAPITAL ? 1.25 : 1.0);
            switch (G.of(GV_CENTRE)) {   // A-04: the people's centre: a spire, its arcology, a dome, a ring, a stepped tomb, a ring's machine
                case CE_TEMPLE: { double rr = (18 + 10 * rng.uni()) * A; L.add(BK_SPIRE, 0, 0, h0, rr, rr, ((town ? 600 : 350) + 400 * rng.uni()) * L.towerK, 0.5, false); break; }
                case CE_ASSEMBLY: { double rr = ((town ? 70 : 40) + 30 * rng.uni()) * A; L.add(BK_HABITAT, 0, 0, h0, rr, rr, rr * 0.6, 0.5, false); break; }
                case CE_MARKET: case CE_MACHINE: { double rr = ((town ? 70 : 45) + 40 * rng.uni()) * A; L.add(BK_RING, 0, 0, h0 + PI / 2, rr, 6, 2 * rr, 0.5, false); break; }
                case CE_TOMB: { double rr = ((town ? 70 : 40) + 30 * rng.uni()) * A; L.add(BK_ARCOLOGY, 0, 0, h0, rr, rr, rr * 1.4, 0.5, false, 0); break; }
                default:
                    if (town) { double rr = (90 + 60 * rng.uni()) * A; L.add(BK_ARCOLOGY, 0, 0, h0, rr, rr, (300 + 500 * rng.uni()) * L.towerK, 0.5, false); }
                    else { double rr = (40 + 30 * rng.uni()) * A; L.add(BK_HABITAT, 0, 0, h0, rr, rr, rr * 0.6, 0.5, false); }
                    break;
            }
            { const Building& cb = r.buildings.back(); focusBefore(std::max(cb.hw, cb.hd) + 30, 1.0); }
            // the spires: as many as the people's height allows, set round by its symmetry (evenly; in mirrored pairs; turned; at random)
            const int spires = std::max(1, (int)std::lround((town ? 2.5 : 1.0) * towerShare));
            const int sym = G.of(GV_SYMMETRY);
            for (int k = 0; k < spires; k++) {
                double a;
                switch (sym) {
                    case SY_RADIAL: a = h0 + (k + 0.5) * TAU / spires; break;
                    case SY_BILATERAL: a = h0 + (k % 2 ? 1 : -1) * (0.6 + 0.5 * (k / 2)); break;
                    case SY_ROTATIONAL: a = h0 + k * TAU / spires + 0.6; break;
                    default: a = h0 + (k + 0.3 + 0.4 * rng.uni()) * TAU / spires; break;
                }
                double d = R * (0.68 + 0.2 * rng.uni()), rr = 15 + 10 * rng.uni();
                if (links == LK_BRIDGES && k == 0) { L.add(BK_TWIN, std::sin(a) * d, std::cos(a) * d, a + PI / 2, rr * 3.4, rr, ((town ? 400 : 250) + 300 * rng.uni()) * L.towerK, 0.5, true); continue; }
                L.add(BK_SPIRE, std::sin(a) * d, std::cos(a) * d, a + PI, rr, rr, ((town ? 500 : 300) + (town ? 700 : 300) * rng.uni()) * L.towerK, 0.5);
            }
            if (town) { double a = h0 + PI + rng.sym(0.6), d = R * 0.55, rr = 50 + 50 * rng.uni(); L.add(BK_RING, std::sin(a) * d, std::cos(a) * d, a + PI / 2, rr, 6, 2 * rr, 0.5); }
            if (town || rng.chance(0.5) || links == LK_ARCADES || links == LK_TUNNELS) L.add(BK_MAGLEV, sx * R * 0.5, sz * R * 0.5, h0, 3, R * 0.95, 18 + 10 * rng.uni(), 0.5, false);
            int placed = (int)r.buildings.size();
            const double habShare = G.of(GV_MASSING) == MS_SPRAWLING ? 0.45 : (G.of(GV_MASSING) == MS_COMPACT ? 0.12 : (town ? 0.3 : 0.15));
            for (int tries = 0; tries < 60 && placed < n; tries++) {
                double a = rng.uni() * TAU, rr = R * (0.35 + 0.6 * std::sqrt(rng.uni())) * (G.of(GV_MASSING) == MS_COMPACT ? 0.75 : 1.0);
                if (rng.chance(habShare)) { double hr = 30 + 25 * rng.uni(); if (L.add(BK_HABITAT, std::sin(a) * rr, std::cos(a) * rr, a + PI, hr, hr * (G.of(GV_PLAN) == PF_ELLIPSE ? 1.3 : 1.0), hr * 0.6, 0.5)) placed++; }
                else if (L.dwelling(std::sin(a) * rr, std::cos(a) * rr, a + PI)) placed++;
            }
            break;
        }
    }
    if (r.walled) {   // the town wall: a ring of segments, a stretch fallen here and there, a gate at each end of the street's axis;
                      // A-03: a palisade on its bank and a gate of two posts, or walls with round towers at their joints and gatehouses;
                      // A-04: a bank and its ditch where the people dug its edge
        double reach = 0;   // C-08: outside every building (a wall through a house blocked its doorway on the desert test world)
        for (const Building& b : r.buildings) reach = std::max(reach, std::sqrt(b.x * b.x + b.z * b.z) + std::sqrt(b.hw * b.hw + b.hd * b.hd));
        const bool pal = c.age == AGE_PREHISTORIC || edge == ED_DITCHES;
        double rw = std::max(R * 1.04, reach + 3.0 + (!pal && c.towers ? 3.6 : 0.0));   // A-03: and its round towers' width (they stood in two doorways)
        int segs = 10 + rng.irange(5);
        if (r.sclass == SC_CITY || r.sclass == SC_CAPITAL) segs = std::max(segs, (int)(TAU * rw / 45.0));   // T-01: a city's wall in stretches of a town's
        double hWall = 4.0 + 2.0 * rng.uni();
        if (pal) hWall *= 0.65;
        std::vector<double> rad(segs);
        for (int k = 0; k < segs; k++) rad[k] = rw * (0.96 + 0.08 * rng.uni());
        for (int k = 0; k < segs; k++) {
            double aA = h0 + (k + 0.5) * TAU / segs, aB = h0 + (k + 1.5) * TAU / segs;
            double xA = std::sin(aA) * rad[k], zA = std::cos(aA) * rad[k], xB = std::sin(aB) * rad[(k + 1) % segs], zB = std::cos(aB) * rad[(k + 1) % segs];
            double mx = (xA + xB) * 0.5, mz = (zA + zB) * 0.5, dx = xB - xA, dz = zB - zA, len = std::sqrt(dx * dx + dz * dz);
            double wh = std::atan2(dx, dz) + PI / 2;   // the wall runs across its heading
            double am = h0 + (k + 1.0) * TAU / segs;
            double gA = std::fabs(std::remainder(am - h0, TAU)), gB = std::fabs(std::remainder(am - h0 - PI, TAU));
            bool gate = std::min(gA, gB) < PI / segs;
            if (!pal && c.towers) L.add(BK_WALL_TOWER, xA, zA, aA, 2.6 + 0.8 * rng.uni(), 0, (hWall + 3 + 2 * rng.uni()) * (0.8 + 0.4 * G.at(GV_HEIGHT)), 0.5, false);
            if (!gate && rng.chance(0.12)) continue;   // a fallen stretch
            const int wk = pal ? BK_PALISADE : BK_WALL;
            if (gate) {
                double ux = dx / len, uz = dz / len, q = len * 0.25 + 1.5;
                L.add(wk, mx - ux * q, mz - uz * q, wh, len * 0.25 - 1.5, 0.45, hWall, 0.9, false);
                L.add(wk, mx + ux * q, mz + uz * q, wh, len * 0.25 - 1.5, 0.45, hWall, 0.9, false);
                if (c.age == AGE_PREHISTORIC || !pal) L.add(BK_GATE, mx, mz, wh, 1.9, 0.6, hWall + 1.0, 0.8, false);
            } else {
                L.add(wk, mx, mz, wh, len * 0.5, 0.45, hWall, 0.9, false);
            }
        }
    }
    // A-04: the people's relation to the land: a high place toward the ridge (a stepped platform and its shrine), an aqueduct coming down
    // from the high ground; the people's tunnels' mouths
    if (r.sclass >= SC_VILLAGE && r.sclass != SC_MONUMENT && c.age <= AGE_MODERN) {
        const double rd = r.site.ridgeDir;
        if (land == LD_RIDGES) for (int k = 0; k < 4; k++) { const double d = R * (0.72 + 0.1 * k); if (L.add(BK_PLATFORM, std::sin(rd) * d, std::cos(rd) * d, rd + PI, 7 + 3 * rng.uni(), 7 + 3 * rng.uni(), 3.3, 0.5, true)) break; }
        if (land == LD_VALLEYS) {
            const double len = 70 + 50 * rng.uni(), d = R * 1.04 + len * 0.5 + 6;
            L.add(BK_AQUEDUCT, std::sin(rd) * d, std::cos(rd) * d, rd, 1.4, len * 0.5, (9 + 9 * rng.uni()) * (0.8 + 0.4 * G.at(GV_AMBITION)), 1.2, true);
        }
        if (links == LK_TUNNELS && c.age >= AGE_MEDIEVAL)
            for (int k = 0, made = 0; k < 12 && made < 3; k++) { const double a = rng.uni() * TAU, d = R * (0.3 + 0.6 * rng.uni()); if (L.add(BK_PORTAL, std::sin(a) * d, std::cos(a) * d, a, 2.4, 3.5, 4.8, 0.8, true)) made++; }
    }
    addFeatures(L, r, c);   // C-08: after the plan and the wall, so the earlier draws stay where C-01 left them
    if (c.age == AGE_FUTURISTIC && r.plan == SP_GREAT && r.sclass != SC_HAMLET) placeFurniture(L, r, R);   // R-412
}
}   // namespace

bool settlementClear(const RuinSpec& r, double x, double z, double margin) { return clearOfBuildings(r, x, z, margin); }   // C-12

// C-08: the terrain round a settlement. The planet function at 64 m (no tile: the drainage is off here) at the centre and in
// sixteen directions at the radius (the slope and its fall, the highest ground), then out to 240 m past the edge for the
// nearest water or old sea out to 500 m past the edge (the shore bisected between the last dry sample and the first wet); the wind by the dune fields'
// rule (the world's angle, flipped per hemisphere, turned per 30-degree band) as a heading
SiteRead readSite(const BodyGen& gSeason, const RuinSpec& r) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    DrainageOff off;
    SiteRead st;
    st.desert = kindHas(g.type, KT_DRY);
    const double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(r.lat), 0.05);
    auto at = [&](double east, double north) { return sampleSurface(g, StarSystem::bodyFromLatLon(r.lat + north / mPerRad, r.lon + east / (mPerRad * cosLat)), 64.0); };
    auto wet = [](const SurfaceSample& q) { return q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water) || q.oldSea > 0.5; };
    SurfaceSample c0 = at(0, 0);
    st.relief = c0.relief;
    double R = std::max(r.size, 12.0), sumX = 0, sumZ = 0, hBest = -1e18;
    for (int k = 0; k < 16; k++) {
        double a = k * TAU / 16, ex = std::sin(a) * R, nz = std::cos(a) * R;
        SurfaceSample q = at(ex, nz);
        double dh = q.height - c0.height;
        sumX += dh * std::sin(a); sumZ += dh * std::cos(a);   // the gradient's direction (the rise)
        if (q.height > hBest) { hBest = q.height; st.ridgeDir = a; }
    }
    double gx = sumX / 8.0 / R, gz = sumZ / 8.0 / R;   // rise over run toward the high side
    st.slope = std::sqrt(gx * gx + gz * gz);
    st.downhill = std::atan2(-gx, -gz);
    // the shore: rings out from the edge
    static const double RINGS[7] = {20, 60, 110, 170, 240, 340, 500};   // out to half a kilometre: a village's port is a walk away
    double bestD = 1e9, bestA = 0; bool dry = false;
    for (int ring = 0; ring < 7 && bestD > 1e8; ring++)
        for (int k = 0; k < 16; k++) {
            double a = k * TAU / 16, d = R + RINGS[ring];
            SurfaceSample q = at(std::sin(a) * d, std::cos(a) * d);
            if (!wet(q)) continue;
            double lo = ring == 0 ? R : R + RINGS[ring - 1], hi = d;   // the last dry ring and this one: bisect six times
            for (int it = 0; it < 6; it++) { double mid = 0.5 * (lo + hi); if (wet(at(std::sin(a) * mid, std::cos(a) * mid))) hi = mid; else lo = mid; }
            if (hi < bestD) { bestD = hi; bestA = a; dry = q.oldSea > 0.5 && q.material != MAT_WATER; }
        }
    if (bestD < 1e8) { st.shoreDist = bestD; st.shoreDir = bestA; st.shoreDry = dry; }
    int band = (int)std::floor(std::fabs(r.lat) / (30 * DEG));
    st.wind = g.windAngle + (r.lat < 0 ? PI : 0) + band * 0.9;
    const int now = nowKind(g), cover = KINDS[now].cover;   // P-04: the present over a dead world's: its seas dry or ice, sand or snow at the walls
    st.drift = kindHas(now, KT_DRY) ? 1 : (cover == DC_FROZEN ? 2 : 0);
    if (bestD < 1e8 && cover == DC_SCORCHED) st.shoreDry = true;
    if (bestD < 1e8 && cover == DC_FROZEN) st.shoreIce = true;
    return st;
}

std::string featureList(const RuinSpec& r) {
    std::string s;
    auto add = [&](const char* w) { if (!s.empty()) s += ", "; s += w; };
    if (r.harbour) {   // T-02: by the age; P-04: a frozen world's facing the ice
        static const char* const H[3] = {"a harbour", "a dock", "a container port"}, * const HD[3] = {"a harbour on the dry shore", "a dock on the dry shore", "a container port on the dry shore"};
        static const char* const HI[3] = {"a harbour on the ice", "a dock on the ice", "a container port on the ice"};
        add((r.site.shoreIce ? HI : (r.site.shoreDry ? HD : H))[clampi(r.harbourKind, 0, 2)]);
    }
    if (r.terraced) add("terraces");
    if (r.cistern) add("a cistern");
    if (r.windwall) add("a wind wall");
    if (r.cairns) add("cairns");
    if (r.reservoir) add("a reservoir");   // T-02
    return s;
}

// A-04: a settlement's axis, its people's: to the sun's rise (east), a star of its own, the moon's rising (bearings from its seed), the
// river's way (the ground's fall here: three samples of the planet function at 64 m, no tile), the wind's, the cell's own draw; a people
// that laid its streets on the contour turns the axis across the fall. A little of the cell's draw in every one
double orientedHeading(const BodyGen& g, const Culture& c, double lat, double lon, double cellHeading) {
    const Genome& G = c.genome;
    const uint64_t ps = peopleGen(g, c.people).seed;
    const double jitter = std::remainder(cellHeading, TAU) * 0.02;
    auto fall = [&]() {
        DrainageOff off;
        BodyGen gTmp; const BodyGen& gt = thenWorld(g, gTmp);   // P-04: the ground of its time
        const double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(lat), 0.05), d = 150;
        auto h = [&](double e, double nn) { return sampleSurface(gt, StarSystem::bodyFromLatLon(lat + nn / mPerRad, lon + e / (mPerRad * cosLat)), 64.0).height; };
        const double h0 = h(0, 0), gx = h(d, 0) - h0, gz = h(0, d) - h0;
        return std::atan2(-gx, -gz);   // downhill
    };
    if (G.of(GV_STREETS) == ST_CONTOUR && c.age >= AGE_MEDIEVAL && c.age <= AGE_MODERN) return fall() + PI / 2 + jitter;
    switch (G.of(GV_ORIENT)) {
        case OT_SUNRISE: return PI / 2 + jitter;
        case OT_STAR: return unitFromHash(mix64(ps ^ 0xA04E1ULL)) * TAU + jitter;
        case OT_MOON: return PI / 2 + (unitFromHash(mix64(ps ^ 0xA04E2ULL)) - 0.5) * 1.2 + jitter;
        case OT_RIVER: return fall() + jitter;
        case OT_WIND: { const int band = (int)std::floor(std::fabs(lat) / (30 * DEG)); return g.windAngle + (lat < 0 ? PI : 0) + band * 0.9 + jitter; }
        default: return cellHeading;
    }
}

namespace {
// T-01: the world's plan, kept a thread (the cells of one world are asked for in runs: no lock a cell)
std::shared_ptr<const WorldPlan> planFor(const BodyGen& g) {
    thread_local uint64_t seed = 0;
    thread_local std::shared_ptr<const WorldPlan> plan;
    if (!plan || seed != g.seed) { plan = worldPlanOf(g); seed = g.seed; }
    return plan;
}
// T-01: the countryside's shares a cell (hamlets, villages, market towns, a lone monument in the draw's first twentieth) where the plan's
// density is full (the medieval age's C-01's, a little thinner: its towns are the plan's), by the age
const double COUNTRYSIDE[AGE_COUNT][4] = {{0.05, 0.03, 0.0, 0.6}, {0.16, 0.10, 0.02, 0.4}, {0.12, 0.10, 0.04, 0.3}, {0.08, 0.08, 0.04, 0.3}, {0.03, 0, 0, 0}, {0, 0, 0, 0}};
}

namespace {
// T-02: a place of the plan's past from its people's history (its system found again from the world's star, `historyOfWorld`): its founding
// and the years before the end it was left. B-414: it stands in its people's last age's way whatever age founded it, and a place left
// before that age began is gone (its cell holds nothing: its records, its works and its dead went with it)
TownHist placeHist(const BodyGen& g, const Culture& c, int people, int rank) {
    TownHist t;
    t.ages.push_back(c.age);
    const std::shared_ptr<const History> H = historyOfWorld(g, people);
    if (!H || rank < 0 || rank >= (int)H->places.size() || H->places[rank].plan != rank) return t;
    const HistoryPlace& hp = H->places[rank];
    t.founded = hp.founded;
    if (hp.fate == HP_EARLY) {
        if (hp.ended < ageFromOf(H->profile)) t.gone = true;
        else t.before = std::max(0, H->last - hp.ended);
    }
    return t;
}
// T-02: a countryside settlement's past: B-414, its people's own age, used to the end (T-02 drew a later people's villages round an older
// core and its lone monuments as megaliths of its old time: those ages are not on the ground now)
TownHist countrysideHist(const Culture& c) {
    TownHist t;
    t.ages.push_back(c.age);
    return t;
}

bool cellSettlement(const BodyGen& g, int gLat, int gLon, RuinSpec& out, bool withBuildings) {
    if (!worldHadCivilisation(g)) return false;
    double dLat = ruinCellLat(g);
    double latc = (gLat + 0.5) * dLat;
    if (std::fabs(latc) > PI / 2 - dLat) return false;
    double dLon = dLat / std::max(std::cos(latc), 0.05);
    int nLon = (int)std::ceil(TAU / dLon);
    int gl = ((gLon % nLon) + nLon) % nLon;
    uint64_t h = hash2i(gLat, gl, g.seed ^ 0x2711);
    double u = unitFromHash(h);
    // T-01: a place of the world's plan stands in this cell: the capital, a city, a town, a band's seat, a great place, a work
    const std::shared_ptr<const WorldPlan> plan = planFor(g);
    const PlanPlace* pl = planPlaceIn(*plan, gLat, gl);
    if (!pl && u > 0.35) return false;
    out.buildings.clear();
    out.lat = (gLat + 0.2 + 0.6 * unitFromHash(mix64(h + 1))) * dLat;
    out.lon = (gl + 0.2 + 0.6 * unitFromHash(mix64(h + 2))) * dLon;
    out.heading = unitFromHash(mix64(h + 3)) * TAU;
    out.seed = h;
    out.sclass = 0; out.plan = 0; out.walled = false; out.rank = -1; out.wanted = 0;
    if (pl) {
        out.lat = pl->lat; out.lon = pl->lon; out.people = pl->people;
        out.sclass = pl->sclass; out.rank = pl->rank; out.wanted = pl->buildings;
        if (pl->sclass == SC_CITY || pl->sclass == SC_CAPITAL) out.size = pl->radiusM;
        const Culture c = cultureOf(g, out.people);
        out.heading = orientedHeading(g, c, out.lat, out.lon, out.heading);   // A-04
        const TownHist th = placeHist(g, c, out.people, pl->rank);   // T-02: its past from the history
        if (th.gone) return false;   // B-414: left before its people's last age: nothing of it stands
        if (withBuildings) layoutSettlement(out, c, g, &th);
        else { Rng rng(out.seed ^ 0x5E77ULL); int n; settlementSize(out, rng, n, out.size, c); if (out.sclass == SC_MONUMENT) out.size = std::max(out.size, 12.0); }
        return true;
    }
    const Vec3 at = StarSystem::bodyFromLatLon(out.lat, out.lon);
    out.people = peopleAt(g, out.lat, out.lon);   // C-13: whose side of the divide the settlement stands on
    // T-01: the countryside: a cell's settlement by the plan's density here (the people's territory, its ground, the nearness of its
    // places; nothing outside the land the people held, nothing in a city's core), the age's shares (C-01's hamlets, villages and towns
    // a medieval people's; its lone monuments in the draw's first twentieth, B-417: on every people's world, the old ones' gone)
    const double d = planCountryside(*plan, out.lat, out.lon, out.people);
    if (d <= 0) return false;
    const int age = plan->plans[out.people > 0 && plan->peoples > 1 ? 1 : 0].age;
    const double* sh = COUNTRYSIDE[clampi(age, 0, AGE_COUNT - 1)];
    const double t0 = 0.05 + sh[0] * d, t1 = t0 + sh[1] * d, t2 = t1 + sh[2] * d;
    if (u <= 0.05) { if (u > 0.05 * sh[3] * d) return false; out.sclass = SC_MONUMENT; }
    else if (u <= t0) out.sclass = SC_HAMLET;
    else if (u <= t1) out.sclass = SC_VILLAGE;
    else if (u <= t2) out.sclass = SC_TOWN;
    else return false;
    if (planCoreAt(*plan, at, 250.0)) return false;   // within a city's core and its edge: the city's own ground
    const Culture c = cultureOf(g, out.people);
    const TownHist th = countrysideHist(c);   // T-02; B-414: its people's own age (a monument too)
    if (out.sclass != SC_MONUMENT || c.age == AGE_FAR_FUTURE) out.heading = orientedHeading(g, c, out.lat, out.lon, out.heading);   // A-04
    if (withBuildings) layoutSettlement(out, c, g, &th);
    else { Rng rng(out.seed ^ 0x5E77ULL); int n; settlementSize(out, rng, n, out.size, c); if (out.sclass == SC_MONUMENT) out.size = std::max(out.size, 12.0); }
    return true;
}
}   // namespace

// T-01: a cell's settlement, and the plan's works that cross it (a maglev's spans on their pylons): added to its settlement's buildings, or
// a cell of works alone (`SC_WORKS`)
bool ruinOfCell(const BodyGen& gSeason, int gLat, int gLon, RuinSpec& out, bool withBuildings) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    const bool has = cellSettlement(g, gLat, gLon, out, withBuildings);
    if (!withBuildings || !worldHadCivilisation(g)) return has;
    const std::shared_ptr<const WorldPlan> plan = planFor(g);
    if (!plan->works) return has;
    const int nLon = std::max(1, ruinCellsAround(g, gLat)), gl = ((gLon % nLon) + nLon) % nLon;
    std::vector<PlanSpan> spans; planSpansIn(g, *plan, gLat, gl, spans);
    if (spans.empty()) return has;
    if (!has) {   // a cell of works alone, centred on the cell
        const double dLat = ruinCellLat(g), latc = (gLat + 0.5) * dLat, dLon = dLat / std::max(std::cos(latc), 0.05);
        out = RuinSpec();
        out.sclass = SC_WORKS; out.plan = SP_LONE;
        out.lat = latc; out.lon = (gl + 0.5) * dLon; out.seed = hash2i(gLat, gl, g.seed ^ 0x2711); out.people = spans[0].people; out.size = 12;
    }
    const double Rm = g.R * 1000.0, cosLat = std::max(std::cos(out.lat), 0.05);
    auto local = [&](const Vec3& u, double& x, double& z) { double la, lo; StarSystem::latLonFromBody(u, la, lo); x = std::remainder(lo - out.lon, TAU) * Rm * cosLat; z = (la - out.lat) * Rm; };
    for (const PlanSpan& sp : spans) {
        double ax, az, bx, bz; local(sp.a, ax, az); local(sp.b, bx, bz);
        Building b; b.kind = BK_MAGLEV; b.x = 0.5 * (ax + bx); b.z = 0.5 * (az + bz); b.heading = std::atan2(bx - ax, bz - az);
        b.hw = 3; b.hd = 0.5 * std::hypot(bx - ax, bz - az); b.hWall = sp.h; b.thick = 0.5; b.door = -1; b.age = AGE_FUTURISTIC; b.seed = sp.seed;
        b.luck = (unitFromHash(mix64(sp.seed ^ 0x7011AAULL)) - 0.5) * 0.5;
        out.buildings.push_back(b);
        if (!has) out.size = std::max(out.size, std::hypot(b.x, b.z) + b.hd + 5);   // a cell of works alone: its reach (a settlement keeps its own radius)
    }
    return true;
}

void settlementOf(const BodyGen& gSeason, int gLat, int gLon, const Culture& c, int sclass, RuinSpec& out) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    const double dLat = ruinCellLat(g), latc = (gLat + 0.5) * dLat, dLon = dLat / std::max(std::cos(latc), 0.05);
    const int nLon = (int)std::ceil(TAU / dLon), gl = ((gLon % nLon) + nLon) % nLon;
    const uint64_t h = hash2i(gLat, gl, g.seed ^ 0x2711);
    out = RuinSpec();
    out.lat = (gLat + 0.2 + 0.6 * unitFromHash(mix64(h + 1))) * dLat;
    out.lon = (gl + 0.2 + 0.6 * unitFromHash(mix64(h + 2))) * dLon;
    out.heading = unitFromHash(mix64(h + 3)) * TAU;
    out.seed = h; out.sclass = sclass; out.people = c.people;
    if (sclass != SC_MONUMENT || c.age == AGE_FAR_FUTURE) out.heading = orientedHeading(g, c, out.lat, out.lon, out.heading);   // A-04
    layoutSettlement(out, c, g);
}

Culture cultureAs(const BodyGen& g, int people, int age, double years) {
    Culture c = cultureOf(g, people);
    c.age = clampi(age, 0, AGE_COUNT - 1); c.ageYears = years; c.weatherYears = years / ageDurability(c.age); c.decay = decayOf(c.weatherYears, c.wet);
    cultureWays(c, peopleGen(g, people).seed);   // A-04: its genome read through the age
    return c;
}

bool ruinSiteOk(const BodyGen& gSeason, const RuinSpec& r) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    if (r.sclass == SC_WORKS) return true;   // T-01: a line's pylons stand over whatever the line crosses
    SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(r.lat, r.lon), 64.0);
    if (s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water)) return false;
    if (s.oldSea > 0.5) return false;   // nobody lived on the sea's floor while it was a sea
    double dAng = r.size / (g.R * 1000.0), hmin = s.height, hmax = s.height;
    for (int k = 0; k < 4; k++) {
        double a = k * PI / 2;
        double lat = r.lat + std::cos(a) * dAng, lon = r.lon + std::sin(a) * dAng / std::max(std::cos(r.lat), 0.05);
        SurfaceSample q = sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), 64.0);
        if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water) || q.oldSea > 0.5) return false;
        hmin = std::min(hmin, q.height); hmax = std::max(hmax, q.height);
    }
    return hmax - hmin < 0.5 * r.size;   // a grade of a quarter across the settlement at most
}

void ruinElements(const RuinSpec& r, const Culture& c, int lod, std::vector<Form>& out) {
    out.clear();
    // T-02: a building left before the end (a place left early) weathered from when it was left; one of another age (a work's piece built
    // its age's way: a modern quarry's trucks) in that age's way (B-414: a world's settlements and works are all its people's last age's)
    struct Way { int age; double before; Culture c; };
    std::vector<Way> ways;
    for (size_t i = 0; i < r.buildings.size(); i++) {
        const Building& b = r.buildings[i];
        const int wa = b.age >= 0 ? b.age : c.age;
        if (wa == c.age && b.before <= 0) { buildingForms(b, c, r.site, lod, out, (int)i); continue; }
        const Way* w = nullptr;
        for (const Way& x : ways) if (x.age == wa && x.before == b.before) w = &x;
        if (!w) { ways.push_back({wa, b.before, cultureAtAge(c, wa, b.before)}); w = &ways.back(); }
        buildingForms(b, w->c, r.site, lod, out, (int)i);
    }
}

bool buildingDoor(const Building& b, double& dx, double& dz) {   // C-03; A-03: the room's doorway
    BuildingRoom R;
    if (!buildingRoom(b, R) || !R.door) return false;
    dx = R.doorX; dz = R.doorZ;
    return true;
}

bool settlementClearance(const BodyGen& gSeason, double& lat, double& lon, double marginM) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    if (!worldHadCivilisation(g)) return false;
    int gLat, gLon; ruinCellOf(g, lat, lon, gLat, gLon);
    bool moved = false;
    for (int dl = -1; dl <= 1; dl++)
        for (int dn = -1; dn <= 1; dn++) {
            RuinSpec sp;
            if (!ruinOfCell(g, gLat + dl, ruinCellLonIn(g, gLat, gLon, gLat + dl) + dn, sp, false)) continue;
            double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(sp.lat), 0.05);
            double north = (lat - sp.lat) * mPerRad, east = std::remainder(lon - sp.lon, TAU) * mPerRad * cosLat, d = std::sqrt(north * north + east * east);
            double want = sp.size + marginM;
            if (d >= want) continue;
            if (d < 1) { east = 1; north = 0; d = 1; }
            lat = sp.lat + north / d * want / mPerRad; lon = sp.lon + east / d * want / (mPerRad * cosLat);
            moved = true;
        }
    // T-03: a work's ground (a quarry's pit, a reservoir, a shelter's door, a necropolis), the compact ones: out to its reach (the works when built;
    // a line, a field of turbines or masts, a battlefield's craters and a launch site's spread are landed among); B-418: the land's too
    std::vector<std::shared_ptr<const Work>> near;
    if (worksAroundIfReady(g, StarSystem::bodyFromLatLon(lat, lon), marginM, near)) {
        for (const std::shared_ptr<const Work>& w : near) {
            if (w->radiusM > 1500 || w->kind == WO_WAY || w->kind == WO_FORT_LINE || w->kind == WO_GEOGLYPH || w->kind == WO_BATTLEFIELD || w->kind == WO_WINDFARM ||
                w->kind == WO_ANTENNA || w->kind == WO_LAUNCH || (w->kind == WO_STRANGE && w->style == 1)) continue;
            double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(w->lat), 0.05);
            double north = (lat - w->lat) * mPerRad, east = std::remainder(lon - w->lon, TAU) * mPerRad * cosLat, d = std::sqrt(north * north + east * east);
            const double want = w->radiusM + marginM;
            if (d >= want) continue;
            if (d < 1) { east = 1; north = 0; d = 1; }
            lat = w->lat + north / d * want / mPerRad; lon = w->lon + east / d * want / (mPerRad * cosLat);
            moved = true;
        }
    }
    // T-01: a city's core anchored farther off
    if (const PlanPlace* p = planCoreAt(*planFor(g), StarSystem::bodyFromLatLon(lat, lon), marginM)) {
        double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(p->lat), 0.05);
        double north = (lat - p->lat) * mPerRad, east = std::remainder(lon - p->lon, TAU) * mPerRad * cosLat, d = std::sqrt(north * north + east * east);
        const double want = p->radiusM + marginM;
        if (d < 1) { east = 1; north = 0; d = 1; }
        lat = p->lat + north / d * want / mPerRad; lon = p->lon + east / d * want / (mPerRad * cosLat);
        moved = true;
    }
    return moved;
}
