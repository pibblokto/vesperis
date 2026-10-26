// C-01: the ruins of a world (see ruins.h): a civilisation's settlements in the cells of its world's 2 km grid.
// A settlement is laid out once from the cell's hash: its class gives the
// number of buildings and the radius, its plan places them (a cluster, a street, rings round a centre, a grid of streets
// with a plaza), and every building draws its own pieces from its seed: walls that stand to a share of their height by
// its decay, a doorway with a lintel, a window, an inner wall, the columns of a hall, a roof where the culture built stone
// ones and the house is sound, rubble at the foot of a fallen wall.
#include "ruins.h"
#include "ruins_layout.h"

const char* const SETTLEMENT_CLASS_NAMES[7] = {"HAMLET", "VILLAGE", "TOWN", "MONUMENT", "CITY", "CAPITAL", "WORKS"};
const char* const BUILDING_KIND_NAMES[BK_COUNT] = {"HOUSE", "HALL", "TOWER", "ROTUNDA", "GATE", "PLATFORM", "COLONNADE", "STELA", "RUBBLE", "WALL", "QUAY", "MOLE", "BOLLARD", "TERRACE", "CISTERN",
        "WIND WALL", "CAIRN", "HUT", "LONGHOUSE", "TENT RING", "STONE CIRCLE", "MENHIR", "DOLMEN", "PALISADE", "TEMPLE", "WALL TOWER", "ROW", "TENEMENT", "VILLA", "CHURCH", "TOWN HALL", "STATION",
        "MILL", "GASOMETER", "WATER TOWER", "HEADFRAME", "OBSERVATORY", "BLOCK", "SUBURB", "HIGHRISE", "STADIUM", "MAST", "POWER", "TANKS", "ARCOLOGY", "HABITAT", "SPIRE", "RING", "MAGLEV", "WRECK",
        "POD", "SPIKE", "HALO", "PILLARS", "SHAFT", "MONOLITH", "FOCUS", "WINDMILL", "AQUEDUCT", "TWIN TOWERS", "TUNNEL", "CRANE", "CONTAINERS", "RESERVOIR", "MOUND", "DAM", "DISH", "TOMB", "FORT",
        "BUNKER", "TEETH", "VEHICLE", "AIRCRAFT", "SHIP", "LAUNCH PAD", "HANGAR", "COLOSSUS", "TURBINE", "LIGHTHOUSE", "LATTICE", "DERRICK", "BLOCKS", "SPHERE", "TETHER", "TRILITHON", "CONVEYOR",
        "FURNITURE", "DOME", "GLASSHOUSE"};
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
    // what a people built is its world's in its time (a dead world's living kind), what the years did to it since is the present's: a
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
    // weathering by age: the years since the end and the world's wet; the walls sink the deeper the older. A-01: the years
    // are the profile's (C-08's draw, its span by the age; C-13's one catastrophe for both is the profile's too) and the weather
    // counts them over the age's durability
    PeopleAge pa = peopleAgeOf(g, people);
    c.age = pa.age;
    c.ageYears = pa.endedYearsAgo;
    // a drifter's people's works weathered only until the cold took the world (no rain, no wind, no thaw since): a century or two
    if (g.drifter) c.ageYears = std::min(c.ageYears, 60.0 + 190.0 * unitFromHash(mix64(pg.seed ^ 0xD05F1ULL)));
    c.weatherYears = c.ageYears / ageDurability(c.age);
    c.wet = dryNow || frozenNow ? 0.05 + 0.15 * uDecay : 0.5 + 0.5 * uDecay;
    c.decay = decayOf(c.weatherYears, c.wet);
    double ageK = clampd(std::log10(c.weatherYears / 100.0) / 2.6, 0.25, 1.0);
    c.buried = (dryNow ? 0.6 + 1.2 * uBuried : (frozenNow ? 0.3 + 0.8 * uBuried : 0.15 + 0.4 * uBuried)) * ageK;
    // the roads, by hashes of their own (the stream above stays): a desert people beat tracks as often as it paves
    c.paved = unitFromHash(mix64(pg.seed ^ 0xC09AULL)) < (kindHas(then, KT_DRY) ? 0.5 : 0.7);
    c.roadHalf = 2.5 + 1.5 * unitFromHash(mix64(pg.seed ^ 0xC09BULL));
    c.roadWear = clampd(0.1 + 0.55 * ageK * (0.35 + 0.65 * c.wet), 0.1, 0.6);
    // the architecture genome (peoples/genome.h), the people's own through its ages, by hashes of its own (the stream above stays),
    // and the ways it builds in its age (`cultureWays`: C-01's proportion, plan, towers, style and stone roofs above are its now, and
    // A-03's roof, temple, huts, towers and great structure, the palette). The buildings' decay is their materials' at the years since
    // the end over the people's lifetimes (`buildingDecay`); `decay` above stays A-01's (the graves' stones read it)
    c.genome = genomeOf(pg.seed);
    c.seed = pg.seed;
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
        case BK_FOCUS: return std::max(m.hw, m.hd) * 1.3 + 8;
        default: return 12;   // C-01's
    }
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
}   // namespace

void settlementSize(RuinSpec& r, Rng& rng, int& n, double& R, const Culture& c, MonPick* mon) {
    const int age = c.age;
    switch (r.sclass) {   // A-04: the plan the people's streets' (C-01's draws kept in the stream)
        case SC_HAMLET: {
            n = 3 + rng.irange(4);
            R = 22 + 12 * rng.uni();
            const bool cl = rng.chance(0.6);
            r.plan = c.plan == SP_STREET ? SP_STREET : (c.plan == SP_GRID ? (cl ? SP_CLUSTER : SP_STREET) : SP_CLUSTER);
            break;
        }
        case SC_VILLAGE: n = 7 + rng.irange(8); R = 45 + 25 * rng.uni(); rng.chance(0.5); r.plan = c.plan; break;
        case SC_TOWN: n = 24 + rng.irange(21); R = 60 + 30 * rng.uni(); r.plan = c.plan; break;
        case SC_CITY: case SC_CAPITAL: {   // the plan's core, its radius and its count the plan's (no age scale: the plan's radius is the drawn one)
            n = r.wanted > 0 ? r.wanted : (r.sclass == SC_CAPITAL ? 120 : 60); R = r.size > 0 ? r.size : 200;
            r.plan = c.plan == SP_STREET ? SP_GRID : c.plan;   // a people of the line laid its city's streets along its axis, a grid
            if (age == AGE_PREHISTORIC) r.plan = SP_RADIAL;     // the gathering place: rings round its centre
            if (age == AGE_FUTURISTIC) r.plan = SP_GREAT;
            return;
        }
        default: { n = 1; r.plan = SP_LONE; MonPick m; pickMonument(c, rng, m); R = monumentReach(m, c.tall); if (mon) *mon = m; return; }
    }
    R *= ageScale(age);
    const int ms = c.genome.of(GV_MASSING);   // a compact people's towns tighter, a sprawling one's wider
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

double settlementRadiusOf(const Culture& c, int sclass, uint64_t seed) {
    RuinSpec r; r.sclass = sclass; r.seed = seed;
    Rng rng(seed ^ 0x5E77ULL); int n; double R;
    settlementSize(r, rng, n, R, c);
    return sclass == SC_MONUMENT ? std::max(R, 12.0) : R;
}

bool settlementClear(const RuinSpec& r, double x, double z, double margin) { return clearOfBuildings(r, x, z, margin); }

// C-08: the terrain round a settlement. The planet function at 64 m (no tile: the drainage is off here) at the centre and in
// sixteen directions at the radius (the slope and its fall, the highest ground), then out to 240 m past the edge for the
// nearest water or old sea out to 500 m past the edge (the shore bisected between the last dry sample and the first wet); the wind by the dune fields'
// rule (the world's angle, flipped per hemisphere, turned per 30-degree band) as a heading
SiteRead readSite(const BodyGen& gSeason, const RuinSpec& r) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // the world's, not its season's (P-04: in its people's time)
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
    const int now = nowKind(g), cover = KINDS[now].cover;   // the present over a dead world's: its seas dry or ice, sand or snow at the walls
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
    if (r.reservoir) add("a reservoir");
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
        BodyGen gTmp; const BodyGen& gt = thenWorld(g, gTmp);   // the ground of its time
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

// T-01: the world's plan, kept a thread (the cells of one world are asked for in runs: no lock a cell)
std::shared_ptr<const WorldPlan> planFor(const BodyGen& g) {
    thread_local uint64_t seed = 0;
    thread_local std::shared_ptr<const WorldPlan> plan;
    if (!plan || seed != g.seed) { plan = worldPlanOf(g); seed = g.seed; }
    return plan;
}

namespace {
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
// T-02: a countryside settlement's past: B-414, its people's own age, used to the end (a world shows only the age its people reached)
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
        out.heading = orientedHeading(g, c, out.lat, out.lon, out.heading);
        const TownHist th = placeHist(g, c, out.people, pl->rank);   // its past from the history
        if (th.gone) return false;   // left before its people's last age: nothing of it stands
        if (withBuildings) layoutSettlement(out, c, g, &th);
        else { Rng rng(out.seed ^ 0x5E77ULL); int n; settlementSize(out, rng, n, out.size, c); if (out.sclass == SC_MONUMENT) out.size = std::max(out.size, 12.0); }
        return true;
    }
    const Vec3 at = StarSystem::bodyFromLatLon(out.lat, out.lon);
    out.people = peopleAt(g, out.lat, out.lon);   // whose side of the divide the settlement stands on
    // the countryside: a cell's settlement by the plan's density here (the people's territory, its ground, the nearness of its
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
    if (out.sclass != SC_MONUMENT || c.age == AGE_FAR_FUTURE) out.heading = orientedHeading(g, c, out.lat, out.lon, out.heading);
    if (withBuildings) layoutSettlement(out, c, g, &th);
    else { Rng rng(out.seed ^ 0x5E77ULL); int n; settlementSize(out, rng, n, out.size, c); if (out.sclass == SC_MONUMENT) out.size = std::max(out.size, 12.0); }
    return true;
}
}   // namespace

// T-01: a cell's settlement, and the plan's works that cross it (a maglev's spans on their pylons): added to its settlement's buildings, or
// a cell of works alone (`SC_WORKS`)
bool ruinOfCell(const BodyGen& gSeason, int gLat, int gLon, RuinSpec& out, bool withBuildings) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // the world's, not its season's (P-04: in its people's time)
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
    if (sclass != SC_MONUMENT || c.age == AGE_FAR_FUTURE) out.heading = orientedHeading(g, c, out.lat, out.lon, out.heading);
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
    if (r.sclass == SC_WORKS) return true;   // a line's pylons stand over whatever the line crosses
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
    // a work's ground (a quarry's pit, a reservoir, a shelter's door, a necropolis), the compact ones: out to its reach (the works when built;
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
    // a city's core anchored farther off
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
