// N3 (R-202): creatures. Herds are spawned from the world's bestiary at a site (planLife), driven by a
// small state machine (idle, walk, graze, drink, rest, alert, flee, curious; leaders and followers;
// stampedes from the buggy) and drawn as articulated bodies: segmented trunks, necks, heads, tails and
// jointed legs with gait cycles whose feet stand on the terrain. Flyers flap, glide, perch in trees at
// dusk; swimmers show fins and backs. Calls and hoof-steps are cues for the synth; first sightings go
// to the guide. J-07: the ways (browsing, hunting, trailing, burrowing, climbing, holding a ground; swarms,
// soarers, floaters on the wind; shoals, the shallows, leapers) and the record of what was drawn.
#include "surface_view.h"
#include "core/noise.h"
#include "core/clock.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cmath>
#include <algorithm>
#include <chrono>

const char* const DOING_NAMES[DO_COUNT] = {"IDLE", "WALKING", "GRAZING", "DRINKING", "RESTING", "ALERT", "FLEEING", "CURIOUS", "BROWSING",
                                           "STALKING", "CHASING", "TRAILING", "UNDERGROUND", "CLIMBING", "DISPLAYING", "CHARGING", "FLYING",
                                           "PERCHED", "SWARMING", "SOARING", "DRIFTING", "SWIMMING", "LEAPING", "FEEDING", "DEAD"};
const char* const REMAINS_NAMES[RS_COUNT] = {"", "FRESH", "BLOATED", "SUNKEN", "BONES", "SCATTERED"};
const char* const ANIMAL_MARK_NAMES[3] = {"DYING", "GONE", "CHANGED"};

// J-08: every line's steering of the changed young on a world (the air's and each spread's)
static int steerOf(const MadeWorld& m) {
    int s = m.fate.steer;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) s |= m.spread[i].fate.steer;
    return s;
}
// J-08: a species' fate at a point of a site (none on a world nobody seeded): the layer's fate there, with the species' changes on its world
static AnimalFate fateOn(const SurfaceSite& s, const Bestiary& B, int spi, double x, double z) {
    const MadeWorld& m = s.gen.made;
    if (!m.on() || !m.fated() || spi < 0 || spi >= (int)B.species.size()) return AnimalFate();
    int ch[3] = {-1, -1, -1};
    const int nc = animalChangesOf(B.species[spi], steerOf(m), ch);
    return animalFateOf(B.species[spi], madeFateAt(m, s.unitAt(x, z), s.gen.seed), ch, nc);
}
// J-08: a herd's size under its fate: more where it thrives; and how many of them died (the share rounded by the cell's hash `u`)
static void fateCounts(const AnimalFate& af, int n, double u, int& all, int& dead) {
    all = af.thrive > 0 ? (int)std::lround(n * (1 + 0.5 * af.thrive)) : n;
    dead = af.dead > 0 ? std::min(all, (int)std::floor(all * af.dead + u)) : 0;
}
// J-08: a member's fate: sick (its share of the survivors, by its own hash), born changed (its herd's share), a giant's or a dwarf's size, bigger
// where it thrives
static void fateMember(Critter& c, const AnimalFate& af) {
    if (!af.on()) return;
    if (af.sick > 0 && hash01(mix64(c.seed ^ 0x51C4ULL)) < af.sick) c.sick = af.sickness;
    if (af.changed > 0 && hash01(mix64(c.seed ^ 0xC4A6ULL)) < af.changed) {
        c.changed = true;
        if (af.has(CHG_GIANT)) c.size *= 1.6;
        if (af.has(CHG_DWARF)) c.size *= 0.55;
    }
    if (af.thrive > 0) c.size *= 1 + 0.15 * af.thrive;
}

AnimalFate SurfaceView::fateAt(int species, double x, double z) const { return fateOn(site, bestiary, species, x, z); }

// Encounter design (N3-04): the chance of a herd at a site by its biome; nothing on ice and open sea
static double encounterChance(int biome) {
    switch (biome) {
        case BIO_TROPICAL: case BIO_TEMPERATE: case BIO_GRASSLAND: case BIO_SAVANNA: case BIO_WETLAND: return 0.85;
        case BIO_TAIGA: return 0.55;
        case BIO_DESERT: case BIO_TUNDRA: return 0.35;
        case BIO_ALPINE: return 0.15;
        default: return 0.0;
    }
}

// B-469: a sprouting one's fall: four to sixteen steps after its fate came where it lives (by its hash), in seconds on the landing's clock
// (`lifeClock`: 0 at the landing, where its fate's age is `af.age`); 1e18 for one that does not sprout
static double sproutFallOf(uint64_t seed, int sick, const AnimalFate& af) {
    if (sick != MF_SPROUTING) return 1e18;
    return (4 + 12 * hash01(mix64(seed ^ 0x5B0DULL)) - af.age) * MADE_STEP_S;
}

// flocks of the world's flyers round the frame's middle (the landing's, a new anchor's: B-470), drawn from `r` as planLife always drew them
void SurfaceView::planFlocks(const SurfaceSite& s, const Bestiary& B, Rng& r, int biome, std::vector<Flock>& flocks) {
    int nf = B.flyerCount ? 1 + r.irange(3) : 0;
    if (biome == BIO_ICE) nf = 0;
    for (int i = 0; i < nf; i++) {
        Flock f;
        f.species = B.firstFlyer() + r.irange(std::max(1, B.flyerCount));
        f.cx = r.sym(250); f.cz = r.sym(250); f.alt = 25 + 90 * r.uni(); f.radius = 40 + 120 * r.uni();
        f.phase = r.range(0, TAU); f.speed = (0.04 + 0.06 * r.uni()) * (r.chance(0.5) ? 1 : -1);
        const Species& sp = B.species[f.species];
        f.n = sp.herdMin + r.irange(sp.herdMax - sp.herdMin + 1); f.seed = r.next();
        f.cruiseAlt = f.alt; f.restTimer = 40 + 80 * r.uni();
        // J-07: by the species' way, from the flock's own seed (r draws as before): a swarm low and tight, a soarer high and wide, a floater
        // turning slowly on the wind
        if (sp.way == WAY_SWARM) { f.alt = f.cruiseAlt = 1.2 + 2.3 * hash01(f.seed ^ 1); f.radius = 2 + 4 * hash01(f.seed ^ 2); f.speed *= 3; }
        else if (sp.way == WAY_SOARER) { f.alt = f.cruiseAlt = 70 + 80 * hash01(f.seed ^ 1); f.radius = 60 + 90 * hash01(f.seed ^ 2); f.speed *= 0.5; }
        else if (sp.plan == PLAN_FLOATER) { f.radius = 4 + 10 * hash01(f.seed ^ 2); f.speed *= 0.3; }
        {   // J-08: fewer where its kind died (none where all of it did), more where it thrives; B-469: its sick and its changed young
            const AnimalFate af = fateOn(s, B, f.species, f.cx, f.cz);
            int all, dead;
            fateCounts(af, f.n, hash01(f.seed ^ 3), all, dead);
            f.n = all - dead;
            if (f.n <= 0) continue;
            f.sickness = af.sick > 0 ? af.sickness : 0; f.sick = af.sick; f.changed = af.changed;
            f.changedK = af.has(CHG_GIANT) ? 1.6 : (af.has(CHG_DWARF) ? 0.55 : 1.0);
            f.fateAge = af.age;   // B-490
        }
        flocks.push_back(f);
    }
}

void SurfaceView::planLife(const SurfaceSite& s, const Bestiary& B, uint64_t seed, std::vector<Critter>& critters, std::vector<Flock>& flocks, std::vector<Herd>& herds) {
    critters.clear(); flocks.clear(); herds.clear();
    const Body& b = s.sys->bodies[s.body];
    if (!kindHas(b.type, KT_FAUNA)) return;
    Rng r(seed);
    int biome = s.sampleAt(0, 0, 16).biome;
    planFlocks(s, B, r, biome, flocks);
    if (!B.landCount) return;
    double chance = encounterChance(biome);
    int nh = r.chance(chance) ? 1 + (r.chance(0.5) ? 1 : 0) + (r.chance(0.3) ? 1 : 0) : 0;
    for (int h = 0; h < nh; h++) {
        // a herd centre on land within 180 m
        double cx = 0, cz = 0; bool ok = false;
        for (int tries = 0; tries < 12 && !ok; tries++) {
            cx = r.sym(180); cz = r.sym(180);
            TerrainVertex tv = s.sampleAt(cx, cz, 16);
            ok = tv.material != MAT_WATER && !(tv.water > -1e8f && tv.h < tv.water + 0.5) && tv.material != MAT_SNOW;
        }
        if (!ok) continue;
        int spi = B.pickLand(r.next(), biome);
        if (spi < 0) continue;
        Herd hd; hd.cx = cx; hd.cz = cz; hd.origin = true;
        herds.push_back(hd);
        const Species& sp = B.species[spi];
        int n = sp.herdMin + r.irange(sp.herdMax - sp.herdMin + 1);
        double spread = 4 + 2.0 * n;
        const AnimalFate af = fateOn(s, B, spi, cx, cz);   // J-08: its dead left out, its sick and its changed young among the rest
        int all, dead;
        fateCounts(af, n, hash01(mix64(seed ^ 0x0D1EULL ^ (uint64_t)h)), all, dead);
        if (dead >= all) herds.back().empty = true;
        for (int i = 0; i < all; i++) {
            Critter c;
            c.herd = (int)herds.size() - 1; c.species = spi;
            c.x = cx + r.sym(spread); c.z = cz + r.sym(spread);
            c.heading = r.range(0, TAU); c.speed = 0;
            c.timer = r.range(1, 6); c.size = sp.size * (0.85 + 0.3 * r.uni()); c.seed = r.next();
            c.kind = sp.plan == PLAN_HEXAPOD || sp.plan == PLAN_CRAWLER || sp.plan == PLAN_OCTOPOD ? 1 : 0;
            c.state = CS_IDLE;
            TerrainVertex tv = s.sampleAt(c.x, c.z, 16);
            if (tv.material == MAT_WATER || (tv.water > -1e8f && tv.h < tv.water + 0.5)) { c.x = cx; c.z = cz; }
            c.homeX = c.x; c.homeZ = c.z;   // J-07: a territorial's ground
            if (i >= all - dead) continue;
            fateMember(c, af);
            c.dieAt = sproutFallOf(c.seed, c.sick, af);
            if (c.dieAt <= 0) continue;   // (B-469: fallen before the landing, its remains with the herd's other dead)
            critters.push_back(c);
        }
    }
}

void SurfaceView::spawnLife(double lat, double lon) {
    const Body& b = site.sys->bodies[site.body];
    bestiary = Bestiary::make(b, site.gen);
    rigs.clear();
    for (const Species& sp : bestiary.species) rigs.push_back(rigOf(sp.body));   // J-06: built once a species
    // J-08: on a seeded world, each species' changes and its changed young's rig; whether every animal is gone
    speciesChanges.assign(bestiary.species.size(), SpeciesChanges());
    changedRigs.clear(); remains.clear(); silence = false;
    deadKept.clear(); lifeClock = 0; shallowsPaths.clear();   // B-470: the landing's clock and its dead; B-467: the shallows' places
    flockFalls.clear(); swimmersSunk.clear();   // B-490
    const MadeWorld& mw = site.gen.made;
    if (mw.on() && mw.fated()) {
        const int steer = steerOf(mw);
        for (size_t i = 0; i < bestiary.species.size(); i++) {
            SpeciesChanges& sc = speciesChanges[i];
            sc.n = animalChangesOf(bestiary.species[i], steer, sc.changes);
            for (int k = 0; k < sc.n; k++) if (changeDrawn(sc.changes[k])) sc.drawn |= 1ull << sc.changes[k];
            changedRigs.push_back(sc.n ? rigOf(changedBody(bestiary.species[i].body, sc.changes, sc.n)) : rigs[i]);
        }
        silence = (mw.fauna & MF_SILENCE) != 0;
    }
    uint64_t seed = site.gen.seed ^ (uint64_t)(lat * 1e6) ^ ((uint64_t)(lon * 1e6) << 20);
    planLife(site, bestiary, seed, critters, flocks, herds);
    seenSpecies.clear(); sightings.clear(); pendingCalls.clear();
    lifeSeen.assign(bestiary.species.size(), SeenAnimal()); lifeTarget = LifeTarget(); lifeDrawn.clear();   // J-07
    callsFired = 0; hoofLevel = 0;
    habitatT = -1e9;
}

// B-316: the habitat cells: 512 m squares of latitude and longitude on the body (like the ruins' 2 km cells), so they
// stay where they are through a re-anchor
void SurfaceView::lifeCellAt(const SurfaceSite& s, double x, double z, int& gLat, int& gLon) {
    double lat, lon;
    s.latLonAt(x, z, lat, lon);
    double dLat = 512.0 / s.R;
    gLat = (int)std::floor(lat / dLat);
    double latc = (gLat + 0.5) * dLat;
    double dLon = dLat / std::max(std::cos(latc), 0.05);
    gLon = (int)std::floor(lon / dLon);
}

// the chance of a herd in a habitat cell by its biome: open country most, forests less, the cold and the dry little
static double habitatChance(int biome) {   // B-321: raised by a third (0.55/0.4/0.3/0.18/0.1 before): about a herd per square kilometre of open country
    switch (biome) {
        case BIO_GRASSLAND: case BIO_SAVANNA: return 0.72;
        case BIO_TEMPERATE: case BIO_TROPICAL: case BIO_WETLAND: return 0.52;
        case BIO_TAIGA: return 0.4;
        case BIO_DESERT: case BIO_TUNDRA: return 0.26;
        case BIO_ALPINE: return 0.14;
        default: return 0.0;
    }
}

bool SurfaceView::planCellHerd(const SurfaceSite& s, const Bestiary& B, int gLat, int gLon, Herd& h, int& species, int& n) {
    if (!B.landCount) return false;
    double dLat = 512.0 / s.R;
    double latc = (gLat + 0.5) * dLat;
    double dLon = dLat / std::max(std::cos(latc), 0.05);
    uint64_t hs = hash2i(gLat, gLon, s.gen.seed ^ 0x4E4DULL);
    // the herd's spot: one of six hashed points of the cell, the first on dry land
    for (int k = 0; k < 6; k++) {
        uint64_t hk = mix64(hs + 3 * (k + 1));
        double lat = (gLat + 0.1 + 0.8 * unitFromHash(hk)) * dLat, lon = (gLon + 0.1 + 0.8 * unitFromHash(mix64(hk + 1))) * dLon;
        double x, z;
        s.localAt(StarSystem::bodyFromLatLon(lat, lon), x, z);
        TerrainVertex tv = s.sampleAt(x, z, 16);
        if (tv.material == MAT_WATER || (tv.water > -1e8f && tv.h < tv.water + 0.5) || tv.material == MAT_SNOW || tv.material == MAT_ICE) continue;
        if (unitFromHash(mix64(hs + 1)) >= habitatChance(tv.biome)) return false;
        species = B.pickLand(mix64(hs + 2), tv.biome);
        if (species < 0) return false;
        const Species& sp = B.species[species];
        n = sp.herdMin + (int)(unitFromHash(mix64(hs + 4)) * (sp.herdMax - sp.herdMin + 1));
        h.cx = x; h.cz = z; h.gLat = gLat; h.gLon = gLon; h.origin = false;
        return true;
    }
    return false;
}

void SurfaceView::updateHabitat() {
    if (lastT - habitatT < 0.5 && habitatT > -1e8) return;
    habitatT = lastT;
    double px = inBuggy ? buggy.x : player.x, pz = inBuggy ? buggy.z : player.z;
    // B-470: a kept dead's remains leave with its cell or its distance, its hunters' feeding kept, and come back where they fell
    auto unshow = [&](const Remains& r) {
        if (r.diedAt < -1e17) return;
        for (DeadKept& d : deadKept) if (d.seed == r.seed) { d.shown = false; d.eaten = r.eaten; }
    };
    if (kindHas(site.gen.type, KT_FLORA) && bestiary.landCount) {
        // forget the cell herds that fell behind (J-08: their remains with them, and kills as far off)
        for (size_t h = 0; h < herds.size();) {
            Herd& hd = herds[h];
            double dx = hd.cx - px, dz = hd.cz - pz;
            if (!hd.origin && dx * dx + dz * dz > 1300.0 * 1300.0) {
                critters.erase(std::remove_if(critters.begin(), critters.end(), [&](const Critter& c) { return c.herd == (int)h; }), critters.end());
                for (Critter& c : critters) if (c.herd > (int)h) c.herd--;
                const int gLat = hd.gLat, gLon = hd.gLon;
                remains.erase(std::remove_if(remains.begin(), remains.end(), [&](const Remains& r) {
                    const bool drop = r.cell && r.gLat == gLat && r.gLon == gLon;
                    if (drop) unshow(r);
                    return drop; }), remains.end());
                herds.erase(herds.begin() + h);
            } else h++;
        }
    }
    remains.erase(std::remove_if(remains.begin(), remains.end(), [&](const Remains& r) {
        const bool drop = !r.cell && (r.x - px) * (r.x - px) + (r.z - pz) * (r.z - pz) > 1300.0 * 1300.0;
        if (drop) unshow(r);
        return drop; }), remains.end());
    if (kindHas(site.gen.type, KT_FLORA) && bestiary.landCount) {
        // spawn the herds of the cells that came within reach (J-08: on a seeded world a cell with no herd too, held for its remains); B-470: a
        // member caught or fallen this landing stays dead; B-469: a sprouting one past its fall lies on the high ground (`cellRemains`)
        const bool fated = site.gen.made.on() && site.gen.made.fated();
        int gLat0, gLon0;
        lifeCellAt(site, px, pz, gLat0, gLon0);
        std::vector<Remains> rs;
        std::vector<Critter> members;
        for (int dl = -2; dl <= 2; dl++)
            for (int dn = -3; dn <= 3; dn++) {
                int gLat = gLat0 + dl, gLon = gLon0 + dn;
                bool have = false;
                for (const Herd& hd : herds) if (!hd.origin && hd.gLat == gLat && hd.gLon == gLon) { have = true; break; }
                if (have) continue;
                Herd hd; int spi = -1, n = 0;
                const bool herd = planCellHerd(site, bestiary, gLat, gLon, hd, spi, n);
                if (!herd) {
                    if (!fated) continue;
                    const double dLat = 512.0 / site.R, latc = (gLat + 0.5) * dLat, dLon = dLat / std::max(std::cos(latc), 0.05);
                    site.localAt(StarSystem::bodyFromLatLon(latc, (gLon + 0.5) * dLon), hd.cx, hd.cz);
                    hd.gLat = gLat; hd.gLon = gLon; hd.origin = false; hd.empty = true;
                }
                double dx = hd.cx - px, dz = hd.cz - pz;
                if (dx * dx + dz * dz > 900.0 * 900.0) continue;
                cellRemains(gLat, gLon, herd ? &hd : nullptr, spi, n, rs);
                if (herd) {
                    const AnimalFate af = fateOn(site, bestiary, spi, hd.cx, hd.cz);
                    int all, dead;
                    fateCounts(af, n, hash01(mix64(hash2i(gLat, gLon, site.gen.seed ^ 0x4E4FULL) + 51)), all, dead);
                    cellMembers(hd, spi, all, hash2i(gLat, gLon, site.gen.seed ^ 0x4E4EULL), members);
                    if (dead >= all) hd.empty = true;
                    herds.push_back(hd);
                    for (int i = 0; i < all - dead; i++) {
                        Critter& c = members[i];
                        if (keptDead(c.seed)) continue;
                        c.herd = (int)herds.size() - 1;
                        fateMember(c, af);
                        c.dieAt = sproutFallOf(c.seed, c.sick, af);
                        if (c.dieAt <= lifeClock) continue;
                        critters.push_back(c);
                    }
                } else herds.push_back(hd);
                remains.insert(remains.end(), rs.begin(), rs.end());
            }
    }
    // B-470: the kept dead within reach again, where they fell
    for (DeadKept& d : deadKept) {
        if (d.shown) continue;
        double x, z;
        site.localAt(d.unit, x, z);
        if ((x - px) * (x - px) + (z - pz) * (z - pz) > 900.0 * 900.0) continue;
        Remains r;
        r.species = d.species; r.x = x; r.z = z; r.heading = d.heading; r.size = d.size; r.seed = d.seed; r.gLat = d.gLat; r.gLon = d.gLon;
        r.cell = false; r.kill = !d.stalks; r.stalks = d.stalks; r.diedAt = d.diedAt; r.eaten = d.eaten;
        remains.push_back(r);
        d.shown = true;
    }
    // B-490: the sprouting swimmers past their fall within reach, washed up dead on the nearest dry ground within 60 m of where they were (one
    // with none so near sank), kept
    if (site.hasWater && bestiary.swimmerCount > 0 && ((site.gen.made.on() && site.gen.made.fated()) || testSwimmerFate.on())) {
        std::vector<SwimmerAt> alive, dead;
        swimmersNear(px, pz, lastT, alive, &dead);
        for (const SwimmerAt& w : dead) {
            if (keptDead(w.seed) || std::find(swimmersSunk.begin(), swimmersSunk.end(), w.seed) != swimmersSunk.end()) continue;
            double bx = 0, bz = 0, best = 1e9;
            for (int a = 0; a < 16; a++)
                for (double d = 2; d <= 60 && d < best; d += 2) {
                    const double qx = w.x + std::sin(a * TAU / 16) * d, qz = w.z + std::cos(a * TAU / 16) * d;
                    const TerrainVertex v = site.sampleAt(qx, qz, 16);
                    if (!(v.material == MAT_WATER || (v.water > -1e8f && v.h < v.water + 0.3))) { best = d; bx = qx; bz = qz; }
                }
            if (best < 1e9) keepDead(w.species, bx, bz, w.heading, bestiary.species[w.species].size * w.sizeK, w.seed, w.fellAt);   // (aged since its fall)
            else swimmersSunk.push_back(w.seed);
        }
    }
    restageRemains();
}

bool SurfaceView::keptDead(uint64_t seed) const {
    for (const DeadKept& d : deadKept) if (d.seed == seed) return true;
    return false;
}

// B-489: the dead its world kept from the landings before (the guide's), at the landing: a landing's own herds were planned already, so their
// members among them go; the cells come without theirs, and their remains lie where they fell once the explorer is within reach
void SurfaceView::deadIn(const std::vector<DeadKept>& kept) {
    deadKept = kept;
    for (DeadKept& d : deadKept) d.shown = false;
    critters.erase(std::remove_if(critters.begin(), critters.end(), [&](const Critter& c) { return keptDead(c.seed); }), critters.end());
    habitatT = -1e9;
}

// B-470: every remains' stage by the clock now: a cell's dead by their fate's age where they lie and the time since the landing, a kill's or a
// fall's by the time since it died (three times as fast with a flesh rotter); old bones as they lie
void SurfaceView::restageRemains() {
    for (Remains& r : remains) {
        if (r.old) continue;
        if (r.diedAt > -1e17) r.stage = remainsStageAfter((lifeClock - r.diedAt) / MADE_STEP_S);
        else if (r.cell) r.stage = remainsStageOf(r.seed, false, r.age0 + lifeClock / MADE_STEP_S);   // (the harness's own as it put them)
    }
}

// B-470: the landing's frame anchored again: its flocks planned round the new middle (as the landing's were), the herds' cells and their dead
// come again with the cells, the kept dead where they fell
void SurfaceView::lifeAnchored(double lat, double lon) {
    for (const Remains& r : remains)
        if (r.diedAt > -1e17) for (DeadKept& d : deadKept) if (d.seed == r.seed) d.eaten = r.eaten;
    for (DeadKept& d : deadKept) d.shown = false;
    remains.clear(); critters.clear(); herds.clear(); flocks.clear(); habitatT = -1e9;
    shallowsPaths.clear(); flockFalls.clear();   // (B-490: one falling then is gone with the old frame)
    if (!kindHas(site.gen.type, KT_FAUNA)) return;
    Rng r(site.gen.seed ^ (uint64_t)(lat * 1e6) ^ ((uint64_t)(lon * 1e6) << 20));
    planFlocks(site, bestiary, r, site.sampleAt(0, 0, 16).biome, flocks);
    for (Flock& f : flocks) f.fallsTo = lifeClock;   // (B-490: those fallen before now fell out of sight)
}

void SurfaceView::cellMembers(const Herd& hd, int spi, int n, uint64_t hs, std::vector<Critter>& out) const {
    out.clear();
    const Species& sp = bestiary.species[spi];
    Rng r(hs);
    double spread = 4 + 2.0 * n;
    for (int i = 0; i < n; i++) {
        Critter c;
        c.herd = -1; c.species = spi;
        c.x = hd.cx + r.sym(spread); c.z = hd.cz + r.sym(spread);
        c.heading = r.range(0, TAU); c.speed = 0;
        c.timer = r.range(1, 6); c.size = sp.size * (0.85 + 0.3 * r.uni()); c.seed = r.next();
        c.kind = sp.plan == PLAN_HEXAPOD || sp.plan == PLAN_CRAWLER || sp.plan == PLAN_OCTOPOD ? 1 : 0;
        c.state = CS_IDLE;
        TerrainVertex tv = site.sampleAt(c.x, c.z, 16);
        if (tv.material == MAT_WATER || (tv.water > -1e8f && tv.h < tv.water + 0.5)) { c.x = hd.cx; c.z = hd.cz; }
        c.homeX = c.x; c.homeZ = c.z;   // J-07: a territorial's ground
        out.push_back(c);
    }
}

// the nearest water within 150 m of a point, false when there is none
static bool waterNear(SurfaceSite& s, double x, double z, double& wx, double& wz) {
    double best = 1e9;
    for (int a = 0; a < 12; a++)
        for (double d = 8; d <= 150; d += 12) {
            double ang = a * TAU / 12, px = x + std::sin(ang) * d, pz = z + std::cos(ang) * d;
            if (s.waterAt(px, pz) > -1e8 && d < best) { best = d; wx = px; wz = pz; }
        }
    return best < 1e9;
}

int SurfaceView::remainsStageOf(uint64_t seed, bool old, double age) const {
    if (old) return hash01(mix64(seed ^ 0x01DBULL)) < 0.5 ? RS_BONES : RS_SCATTERED;
    const double died = hash01(mix64(seed ^ 0xDEADULL)) * std::min(age, 18.0);   // (the deaths spread over the fate's first steps there)
    return remainsStageAfter(age - died);
}
// the stage `steps` after a death (three times as fast with a flesh rotter)
int SurfaceView::remainsStageAfter(double steps) const {
    const double since = steps * (site.gen.made.rotFast ? 3.0 : 1.0);
    return since < 2 ? RS_FRESH : (since < 5 ? RS_BLOATED : (since < 10 ? RS_SUNKEN : (since < 20 ? RS_BONES : RS_SCATTERED)));
}

void SurfaceView::cellRemains(int gLat, int gLon, const Herd* hd, int spi, int n, std::vector<Remains>& out) {
    out.clear();
    const MadeWorld& m = site.gen.made;
    const bool fated = m.on() && m.fated();
    if (!hd && !fated) return;
    const uint64_t hr = hash2i(gLat, gLon, site.gen.seed ^ 0x4E4FULL);   // the remains' own hashes (the members' are 0x4E4E's)
    auto dry = [&](double x, double z) { const TerrainVertex v = site.sampleAt(x, z, 16); return !(v.material == MAT_WATER || (v.water > -1e8f && v.h < v.water + 0.3)); };
    auto put = [&](int species, double x, double z, double heading, double size, uint64_t seed, bool old, int age) {
        Remains r;
        r.species = species; r.x = x; r.z = z; r.heading = heading; r.size = size; r.seed = seed; r.gLat = gLat; r.gLon = gLon; r.old = old;
        r.age0 = age;
        r.stage = remainsStageOf(seed, old, age + lifeClock / MADE_STEP_S);   // (B-470: the clock since the landing too)
        out.push_back(r);
    };
    // the cell's middle where no herd marks it; the shore nearest it within 150 m (its water's edge on the dry side), where the sick go: read
    // from the planet function as the cell's herd is (`sampleAt`), never the drawn ground's (only near the explorer)
    double cx = hd ? hd->cx : 0, cz = hd ? hd->cz : 0;
    if (!hd) {
        const double dLat = 512.0 / site.R, latc = (gLat + 0.5) * dLat, dLon = dLat / std::max(std::cos(latc), 0.05);
        site.localAt(StarSystem::bodyFromLatLon(latc, (gLon + 0.5) * dLon), cx, cz);
    }
    double sx = 0, sz = 0;
    bool hasShore = false, shoreRead = false;
    auto shore = [&]() {   // (read once, when a cell's dead want it: a cell nothing died in reads none)
        if (shoreRead) return hasShore;
        shoreRead = true;
        double wx = 0, wz = 0, best = 1e9;
        for (int a = 0; a < 12; a++)
            for (double d = 8; d <= 150 && d < best; d += 12) {
                const double px = cx + std::sin(a * TAU / 12) * d, pz = cz + std::cos(a * TAU / 12) * d;
                if (!dry(px, pz)) { best = d; wx = px; wz = pz; }
            }
        if (best < 1e9) {
            const double dx = cx - wx, dz = cz - wz, d = std::sqrt(dx * dx + dz * dz);
            for (double q = 0; q <= d && !hasShore && d > 1e-6; q += 1.5) { sx = wx + dx / d * q; sz = wz + dz / d * q; hasShore = dry(sx, sz); }
        }
        return hasShore;
    };
    if (hd && spi >= 0) {
        const Species& sp = bestiary.species[spi];
        if (hash01(mix64(hr + 61)) < 0.12) {   // a few old bones, on any world: a herd's cell in eight
            const double ox = cx + (hash01(mix64(hr + 62)) - 0.5) * 50, oz = cz + (hash01(mix64(hr + 63)) - 0.5) * 50;
            if (dry(ox, oz)) put(spi, ox, oz, hash01(mix64(hr + 64)) * TAU, sp.size * (0.85 + 0.3 * hash01(mix64(hr + 65))), mix64(hr + 66), true, 0);
        }
        if (fated) {   // its dead where they lived, two in five by the water, half a sprouting one's up on the high ground with their stalks
            const AnimalFate af = fateOn(site, bestiary, spi, cx, cz);
            int all, dead;
            fateCounts(af, n, hash01(mix64(hr + 51)), all, dead);
            if (dead > 0 || (af.sickness == MF_SPROUTING && af.sick > 0)) {
                std::vector<Critter> members;
                cellMembers(*hd, spi, all, hash2i(gLat, gLon, site.gen.seed ^ 0x4E4EULL), members);
                double hx = cx, hz = cz, hy = -1e18;   // the high ground: the highest of twelve points within 100 m
                if (af.sickness == MF_SPROUTING)
                    for (int k = 0; k < 12; k++) {
                        const double a = k * TAU / 12, d = 30 + 70 * hash01(mix64(hr + 70 + k));
                        const double qx = cx + std::sin(a) * d, qz = cz + std::cos(a) * d, qy = site.sampleAt(qx, qz, 16).h;
                        if (qy > hy && dry(qx, qz)) { hy = qy; hx = qx; hz = qz; }
                    }
                for (int i = 0; i < all - dead && af.sickness == MF_SPROUTING; i++) {   // B-469: the sick fallen since, by their fall's time
                    Critter c = members[i];
                    if (keptDead(c.seed)) continue;
                    fateMember(c, af);
                    const double fell = sproutFallOf(c.seed, c.sick, af);
                    if (fell > lifeClock) continue;
                    put(spi, hx + (hash01(mix64(c.seed + 1)) - 0.5) * 6, hz + (hash01(mix64(c.seed + 2)) - 0.5) * 6, c.heading, c.size, c.seed, false, af.age);
                    out.back().stalks = true; out.back().diedAt = fell; out.back().stage = remainsStageAfter((lifeClock - fell) / MADE_STEP_S);
                }
                for (int i = all - dead; i < all; i++) {
                    const Critter& c = members[i];
                    double x = c.x, z = c.z;
                    const double u = hash01(mix64(c.seed ^ 0x5A7EULL));
                    bool stalks = false;
                    if (af.sickness == MF_SPROUTING && u < 0.5) {
                        x = hx + (hash01(mix64(c.seed + 1)) - 0.5) * 6; z = hz + (hash01(mix64(c.seed + 2)) - 0.5) * 6; stalks = true;
                    } else if (u < 0.4 && shore()) {
                        x = sx + (hash01(mix64(c.seed + 3)) - 0.5) * 14; z = sz + (hash01(mix64(c.seed + 4)) - 0.5) * 14;
                        if (!dry(x, z)) { x = sx; z = sz; }
                    }
                    put(spi, x, z, c.heading, c.size, c.seed, false, af.age);
                    out.back().stalks = stalks;
                }
            }
        }
    }
    if (!fated) return;
    // the flyers fallen over the cell (two in five by the water), the swimmers washed up on its shore: up to three of each kind by its dead
    for (int f = 0; f < bestiary.flyerCount; f++) {
        const int fi = bestiary.firstFlyer() + f;
        const AnimalFate af = fateOn(site, bestiary, fi, cx, cz);
        if (af.dead <= 0) continue;
        const int count = (int)std::floor(3 * af.dead + hash01(mix64(hr + 80 + f)));
        for (int k = 0; k < count; k++) {
            const uint64_t h = mix64(hr + 0x100 * (uint64_t)(f + 1) + k);
            double x = cx + (hash01(h) - 0.5) * 160, z = cz + (hash01(mix64(h + 1)) - 0.5) * 160;
            if (hash01(mix64(h + 2)) < 0.4 && shore()) { x = sx + (hash01(mix64(h + 3)) - 0.5) * 16; z = sz + (hash01(mix64(h + 4)) - 0.5) * 16; }
            if (!dry(x, z)) continue;
            put(fi, x, z, hash01(mix64(h + 5)) * TAU, bestiary.species[fi].size * (0.9 + 0.2 * hash01(mix64(h + 6))), h, false, af.age);
        }
    }
    bool waterHarm = m.fate.harm[MFC_WATER] > 0;   // (the shore read only where a swimmer can have died)
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) waterHarm = waterHarm || m.spread[i].fate.harm[MFC_WATER] > 0;
    if (!waterHarm || !shore()) return;
    for (int w = 0; w < bestiary.swimmerCount; w++) {
        const int wi = bestiary.firstSwimmer() + w;
        const AnimalFate af = fateOn(site, bestiary, wi, sx, sz);
        if (af.dead <= 0) continue;
        const int count = (int)std::floor(3 * af.dead + hash01(mix64(hr + 90 + w)));
        for (int k = 0; k < count; k++) {
            const uint64_t h = mix64(hr + 0x200 * (uint64_t)(w + 1) + k);
            double x = sx + (hash01(h) - 0.5) * 20, z = sz + (hash01(mix64(h + 1)) - 0.5) * 20;
            if (!dry(x, z)) { x = sx; z = sz; }
            put(wi, x, z, hash01(mix64(h + 5)) * TAU, bestiary.species[wi].size * (0.9 + 0.2 * hash01(mix64(h + 6))), h, false, af.age);
        }
    }
}

// J-08: a quarry caught (B-469: a sprouting one fallen, its stalks up): gone from its herd after the frame, its remains fresh where it fell (a
// kill's: the hunters feed at it). B-470: kept on the body for the landing, so its cell comes back without it and its remains with it
Remains SurfaceView::killCritter(Critter& q, bool fell) {
    q.killed = true;
    Remains r;
    r.species = q.species; r.x = q.x; r.z = q.z; r.heading = q.heading; r.size = q.size; r.stage = RS_FRESH; r.seed = q.seed;
    r.cell = false; r.kill = !fell; r.stalks = fell; r.diedAt = lifeClock;
    DeadKept d;
    d.seed = q.seed; d.unit = site.unitAt(q.x, q.z); d.species = q.species; d.heading = q.heading; d.size = q.size; d.diedAt = lifeClock;
    d.stalks = fell; d.shown = true;
    if (q.herd >= 0 && q.herd < (int)herds.size() && !herds[q.herd].origin) { d.cell = true; d.gLat = herds[q.herd].gLat; d.gLon = herds[q.herd].gLon; }
    r.gLat = d.gLat; r.gLon = d.gLon;
    deadKept.push_back(d);
    remains.push_back(r);
    return r;
}

// B-490: an animal fallen dead where no herd holds it (a sprouting flyer fallen from its flock, a sprouting swimmer washed up): its remains
// where it lies, staged by its death on the landing's clock, its stalks up, kept on the body as a kill is (B-470)
Remains SurfaceView::keepDead(int species, double x, double z, double heading, double size, uint64_t seed, double diedAt) {
    Remains r;
    r.species = species; r.x = x; r.z = z; r.heading = heading; r.size = size; r.seed = seed;
    r.cell = false; r.stalks = true; r.diedAt = diedAt; r.stage = remainsStageAfter((lifeClock - diedAt) / MADE_STEP_S);
    DeadKept d;
    d.seed = seed; d.unit = site.unitAt(x, z); d.species = species; d.heading = heading; d.size = size; d.diedAt = diedAt; d.stalks = true; d.shown = true;
    deadKept.push_back(d);
    remains.push_back(r);
    return r;
}

uint64_t SurfaceView::testKillCritter(int i) {
    if (i < 0 || i >= (int)critters.size()) return 0;
    const uint64_t seed = critters[i].seed;
    killCritter(critters[i]);
    critters.erase(critters.begin() + i);
    return seed;
}

// B-468: a climber's trunk at a height over its foot: the girth tapering from the foot to the top, the axis leaning with the top (as `drawTree`
// draws it from 0.2 m under the ground), and the point on the bark on its side
double SurfaceView::trunkRadius(const Critter& c, double y) const {
    const double f = clampd((y + 0.2) / std::max(0.2, c.trunkTopY + 0.2), 0, 1);
    return std::max(0.02, c.trunkR0 + (c.trunkR1 - c.trunkR0) * f);
}
void SurfaceView::trunkPoint(const Critter& c, double y, double& x, double& z) const {
    const double f = clampd((y + 0.2) / std::max(0.2, c.trunkTopY + 0.2), 0, 1), r = trunkRadius(c, y);
    x = c.treeX + c.trunkLx * f + std::sin(c.treeA) * r; z = c.treeZ + c.trunkLz * f + std::cos(c.treeA) * r;
}
// B-468: the side of its trunk a climber takes: `want` where it is clear of those on the trunk already (an angle as wide as its body with its
// legs out at the girth where it will cling, or a body's length up or down), else the next side either way; with none clear, a body's length
// under the lowest of them (over the highest where that leaves too little trunk), on its own side
double SurfaceView::climbSide(size_t self, double want, const Species& sp, double k) {
    Critter& c = critters[self];
    const double r = trunkRadius(c, c.treeTop), wide = (sp.body.width + 0.8 * sp.body.leg) * k + 0.08;
    const double sep = std::min(PI, wide / std::max(r, 0.03)), len = (sp.body.length + 0.3 * sp.body.neck + 0.2 * sp.body.tailLen) * k + 0.1;
    auto onIt = [&](size_t j) {
        const Critter& o = critters[j];
        return j != self && o.state == CS_CLIMB && o.climbY > 0 && std::fabs(o.treeX - c.treeX) < 0.01 && std::fabs(o.treeZ - c.treeZ) < 0.01;
    };
    auto clear = [&](double a) {   // (apart by its width at the thinner of their two heights)
        for (size_t j = 0; j < critters.size(); j++) {
            if (!onIt(j) || std::fabs(critters[j].treeTop - c.treeTop) >= len) continue;
            const double rr = std::max(0.03, std::min(r, trunkRadius(critters[j], critters[j].treeTop)));
            if (std::fabs(std::remainder(a - critters[j].treeA, TAU)) * rr < wide) return false;
        }
        return true;
    };
    for (int q = 0; q < 12 && ((q + 1) / 2) * sep <= PI; q++) {
        const double a = want + (q % 2 ? 1 : -1) * ((q + 1) / 2) * sep;
        if (clear(a)) return a;
    }
    double lo = 1e9, hi = -1e9;
    for (size_t j = 0; j < critters.size(); j++) if (onIt(j)) { lo = std::min(lo, critters[j].treeTop); hi = std::max(hi, critters[j].treeTop); }
    c.treeTop = lo - 1.05 * len >= 0.6 ? lo - 1.05 * len : hi + 1.05 * len;
    return want;
}

// B-491: a climber's trunk: the nearest live tree within 25 m whose trunk as drawn is as thick as its body is wide at the most it climbs to
// (`upATree`'s top; a thinner one it would hug as a pole), false when there is none
bool SurfaceView::climbTreeNear(double x, double z, const Species& sp, double k, TreeInst& out) {
    constexpr double radius = 25;
    double best = radius * radius;
    bool found = false;
    const int c0x = (int)std::floor((x - radius) / 16), c1x = (int)std::floor((x + radius) / 16);
    const int c0z = (int)std::floor((z - radius) / 16), c1z = (int)std::floor((z + radius) / 16);
    for (int cz = c0z; cz <= c1z; cz++)
        for (int cx = c0x; cx <= c1x; cx++)
            forTrees(cx, cz, [&](const TreeInst& T) {
                if (T.dead || T.stump) return;
                const double dx = T.x - x, dz = T.z - z, d2 = dx * dx + dz * dz;
                if (d2 >= best) return;
                double r0, r1, top, lx, lz;
                trunkOf(T, r0, r1, top, lx, lz);
                const double y = clampd(std::min(T.cy - 0.5 * T.r, 0.45 * T.h), 1.0, 6.0), f = clampd((y + 0.2) / std::max(0.2, top + 0.2), 0, 1);
                if (2 * std::max(0.02, r0 + (r1 - r0) * f) < sp.body.width * k) return;
                best = d2; out = T; found = true;
            });
    return found;
}
// B-468, B-491: a climber on its trunk: its feet on the bark round the girth where it clings, its belly to its side, its body along the trunk's
// axis there (from 0.2 m under the ground to the top, leaning with it)
void SurfaceView::climbPose(const Critter& c, AnimalPoseIn& in) const {
    in.climbing = true; in.wall = Vec3(std::sin(c.treeA), 0, std::cos(c.treeA));
    in.trunkR = trunkRadius(c, c.climbY);
    in.axis = normalize(Vec3(c.trunkLx, c.trunkTopY + 0.2, c.trunkLz));
}
// J-07: the nearest live tree within `radius` of a point (a browser's, a climber's; `any`: a dead one or a stump too), false when none
bool SurfaceView::treeNear(double x, double z, double radius, TreeInst& out, bool any) {
    double best = radius * radius;
    bool found = false;
    const int c0x = (int)std::floor((x - radius) / 16), c1x = (int)std::floor((x + radius) / 16);
    const int c0z = (int)std::floor((z - radius) / 16), c1z = (int)std::floor((z + radius) / 16);
    for (int cz = c0z; cz <= c1z; cz++)
        for (int cx = c0x; cx <= c1x; cx++)
            forTrees(cx, cz, [&](const TreeInst& T) {
                if ((T.dead || T.stump) && !any) return;
                const double dx = T.x - x, dz = T.z - z, d2 = dx * dx + dz * dz;
                if (d2 < best) { best = d2; out = T; found = true; }
            });
    return found;
}

void SurfaceView::updateLife(double dt, double t) {
    double t0 = nowMs();
    lastT = t;
    updateHabitat();
    double sky = env.skyBrightness;
    const double g = clampd(site.gravity, 2.0, 25.0);
    const double px = inBuggy ? buggy.x : player.x, pz = inBuggy ? buggy.z : player.z;
    const int nsp = (int)bestiary.species.size();
    auto speciesOf = [&](int i) { return i >= 0 && i < nsp ? &bestiary.species[i] : nullptr; };
    // J-07: the hunters (a soarer circles over the nearest, a scavenger trails it) and those out hunting, whose quarry flees them
    struct Hunting { double x, z; int prey; bool chasing; };
    std::vector<Hunting> hunting;
    auto nearestHunter = [&](double x, double z, double within) -> const Critter* {
        const Critter* best = nullptr;
        double bd = within * within;
        for (const Critter& o : critters) {
            const Species* os = speciesOf(o.species);
            if (!os || os->way != WAY_HUNTER) continue;
            const double d2 = (o.x - x) * (o.x - x) + (o.z - z) * (o.z - z);
            if (d2 < bd) { bd = d2; best = &o; }
        }
        return best;
    };
    for (const Critter& c : critters)
        if (c.state == CS_STALK || c.state == CS_CHASE)
            if (const Species* cs = speciesOf(c.species)) hunting.push_back({c.x, c.z, cs->prey, c.state == CS_CHASE});
    // ---- flocks: circle, rest on the ground or in a tree, perch through the night (day flyers) or the day (dusk flyers)
    for (Flock& f : flocks) {
        const Species* sp = speciesOf(f.species);
        const int way = sp ? sp->way : WAY_FLOCK;
        bool wantsRest = sp && ((sp->activity == ACT_DAY && sky < 0.3) || (sp->activity == ACT_DUSK && sky > 0.6));
        if (sp && sp->plan == PLAN_FLOATER) {   // J-06: a floater drifts on, day and night; J-07: on the wind, at most of its speed
            const double ws = 0.8 * 0.514 * env.windKnots, wx = std::sin(env.windDir), wz = std::cos(env.windDir);
            f.cx += wx * ws * dt; f.cz += wz * ws * dt;
            const double dx = f.cx - px, dz = f.cz - pz;
            if (dx * dx + dz * dz > 700.0 * 700.0) {   // gone past 700 m: in again 600 m upwind
                f.wraps++;
                const double across = (hash01(mix64(f.seed + (uint64_t)f.wraps)) - 0.5) * 500;
                f.cx = px - wx * 600 + wz * across; f.cz = pz - wz * 600 - wx * across;
            }
            f.alt += (f.cruiseAlt - f.alt) * (1 - std::exp(-dt * 0.2));
            continue;
        }
        if (way == WAY_SOARER && f.landed <= 0)   // J-07: circling over the nearest hunter within 600 m, drifting there at 3 m/s
            if (const Critter* h = nearestHunter(f.cx, f.cz, 600)) {
                const double dx = h->x - f.cx, dz = h->z - f.cz, d = std::sqrt(dx * dx + dz * dz);
                if (d > 20) { f.cx += dx / d * 3 * dt; f.cz += dz / d * 3 * dt; }
            }
        f.restTimer -= dt;
        if (f.landed > 0) {
            f.landed -= dt;
            if (wantsRest) f.landed = std::max(f.landed, 5.0);   // stay down until the light changes
            double target = f.perched ? f.perchY - std::max(site.groundHeight(f.cx, f.cz), site.waterAt(f.cx, f.cz)) : 0.0;
            f.alt += (target - f.alt) * (1 - std::exp(-dt * 0.4));
            if (f.landed <= 0) { f.restTimer = 60 + 80 * hash01(f.seed ^ (uint64_t)t); f.perched = false; }
        } else {
            f.alt += (f.cruiseAlt - f.alt) * (1 - std::exp(-dt * 0.5));
            if (f.restTimer <= 0 || wantsRest) {
                f.landed = 20 + 20 * hash01(f.seed);
                // a perch: the tallest tree near the flock's centre (a swarm settles on the ground)
                double a = f.phase + t * f.speed;
                double fcx = f.cx + std::cos(a) * f.radius, fcz = f.cz + std::sin(a) * f.radius;
                int pcx = (int)std::floor(fcx / 16), pcz = (int)std::floor(fcz / 16);
                double bestH = 0; f.perched = false;
                if (way != WAY_SWARM)
                    for (int cz = pcz - 2; cz <= pcz + 2; cz++)
                        for (int cx = pcx - 2; cx <= pcx + 2; cx++)
                            forTrees(cx, cz, [&](const TreeInst& T) {
                                if (!T.dead && T.h > bestH) {
                                    bestH = T.h;
                                    f.perched = true;
                                    f.perchX = T.x;
                                    f.perchZ = T.z;
                                    f.perchY = T.gy + T.cy + T.r * 0.4;
                                    f.perchR = T.r;
                                }
                            });
                if (f.perched) { f.cx = f.perchX; f.cz = f.perchZ; f.radius = 0; }
            }
        }
    }
    // B-490: a sprouting flock member falls dead at its time from where it flies (none fell before the landing in sight); it falls, and its
    // remains are kept where it comes down (in the water it sinks)
    std::vector<FlockMember> members;
    for (Flock& f : flocks) {
        if (f.sickness != MF_SPROUTING || f.sick <= 0 || lifeClock <= f.fallsTo) { f.fallsTo = std::max(f.fallsTo, lifeClock); continue; }
        flockMembers(f, t, members);
        AnimalFate af;
        af.age = f.fateAge;
        for (const FlockMember& m : members) {
            if (m.sick != MF_SPROUTING || keptDead(m.seed)) continue;
            const double at = sproutFallOf(m.seed, MF_SPROUTING, af);
            if (at <= f.fallsTo || at > lifeClock) continue;
            FlockFall F;
            F.species = f.species; F.x = m.x; F.y = m.y; F.z = m.z; F.heading = m.ph; F.seed = m.seed; F.sick = m.sick; F.changed = m.changed;
            F.size = (speciesOf(f.species) ? speciesOf(f.species)->size : 0.5) * (m.changed ? f.changedK : 1.0);
            flockFalls.push_back(F);
        }
        f.fallsTo = lifeClock;
    }
    for (size_t k = 0; k < flockFalls.size();) {
        FlockFall& F = flockFalls[k];
        F.vy -= g * dt; F.y += F.vy * dt; F.heading += 2.5 * dt;   // (turning over as it falls)
        const double gy = site.groundHeight(F.x, F.z), wy = site.waterAt(F.x, F.z);
        if (F.y > std::max(gy, wy)) { k++; continue; }
        if (wy <= gy) keepDead(F.species, F.x, F.z, F.heading, F.size, F.seed, lifeClock);
        flockFalls.erase(flockFalls.begin() + (long)k);
    }
    // ---- herds
    double localTime = env.localTime;
    double hoof = 0;
    for (size_t i = 0; i < critters.size(); i++) {
        Critter& c = critters[i];
        const Species* spp = speciesOf(c.species);
        if (!spp) continue;
        const Species& sp = *spp;
        const double k = c.size / std::max(sp.size, 0.05);
        c.timer -= dt; c.hungerT -= dt;
        // B-469: a sprouting one falls dead at its time once it stands on its high ground (or a minute past its time, wherever it is), its
        // stalks up, kept for the landing
        if (c.sick == MF_SPROUTING && lifeClock >= c.dieAt && c.state != CS_CLIMB && c.state != CS_UNDER &&
            ((c.state == CS_IDLE && c.speed == 0) || lifeClock >= c.dieAt + 60)) {
            killCritter(c, true);
            continue;
        }
        double dpx = player.x - c.x, dpz = player.z - c.z, dp = std::sqrt(dpx * dpx + dpz * dpz);
        // J-08: a changed young's ways (calm, bold, by night), a crazed one's or a stumbler's falls
        auto changedTo = [&](int ch) {
            if (!c.changed || c.species >= (int)speciesChanges.size()) return false;
            const SpeciesChanges& sc = speciesChanges[c.species];
            for (int q = 0; q < sc.n; q++) if (sc.changes[q] == ch) return true;
            return false;
        };
        const bool crazed = c.sick == MF_CRAZED;
        const int temper = changedTo(CHG_CALM) ? TEMP_INDIFFERENT : (changedTo(CHG_BOLD) ? TEMP_CURIOUS : sp.temperament);
        const int activity = changedTo(CHG_NIGHT) ? ACT_NIGHT : sp.activity;
        if ((crazed || changedTo(CHG_STUMBLES)) && c.speed > 0.05 && c.fallT <= 0 && c.state != CS_UNDER && c.state != CS_CLIMB) {
            const int64_t win = (int64_t)std::floor(t / 4.0);
            if (win != c.fallWin && hash01(mix64(c.seed ^ (uint64_t)win)) < (crazed ? 0.35 : 0.15)) { c.fallT = 1.6; c.fallWin = win; }
        }
        if (c.fallT > 0) {   // down on its side a moment, then up
            c.fallT -= dt; c.speed = 0;
            c.lift += (1 - c.lift) * (1 - std::exp(-dt * 8));
            if (c.fallT <= 0) c.timer = std::min(c.timer, 0.5);
            continue;
        }
        // threat: the explorer within 25 m (shy; curious 15 m), the moving buggy within 60 m; giants ignore everything
        double threatR = temper == TEMP_INDIFFERENT ? 0 : (temper == TEMP_CURIOUS ? 15 : 25);
        const bool rolling = inBuggy && std::fabs(buggy.speed) > 3;
        if (rolling && threatR > 0) threatR = 60;
        const bool territorial = sp.social == SOCIAL_TERRITORIAL && !rolling;
        if (territorial) threatR = 22;   // J-07: it holds its ground against one on foot
        if ((sp.way == WAY_BURROWER || sp.way == WAY_CLIMBER) && !rolling) threatR = std::max(threatR, 18.0);   // J-07: they hide from anyone
        bool threatened = dp < threatR;
        c.calmT = threatened ? 0 : c.calmT + dt;
        const bool restWanted = (activity == ACT_DAY && ((localTime > 0.46 && localTime < 0.54) || sky < 0.2)) || (activity == ACT_NIGHT && sky > 0.45) ||
                                (activity == ACT_DUSK && (sky > 0.7 || sky < 0.05));
        auto underGround = [&](double secs) { c.state = CS_UNDER; c.timer = secs; c.speed = 0; c.homeX = c.x; c.homeZ = c.z; c.lift = 0; };
        auto upATree = [&](double secs) {   // to the nearest trunk within 25 m and up it, false when there is none (B-468: the trunk as drawn)
            TreeInst T;
            if (!climbTreeNear(c.x, c.z, sp, k, T)) return false;
            c.state = CS_CLIMB; c.timer = secs; c.climbY = 0; c.lift = 0;
            c.treeX = T.x; c.treeZ = T.z; c.treeGy = T.gy;
            trunkOf(T, c.trunkR0, c.trunkR1, c.trunkTopY, c.trunkLx, c.trunkLz);
            c.treeR = c.trunkR0;
            c.treeTop = clampd(std::min(T.cy - 0.5 * T.r, 0.45 * T.h), 1.0, 6.0);
            return true;
        };
        // J-07: the ways' own states first, which the explorer's nearness does not interrupt as it does the rest
        if (c.state == CS_UNDER) {   // a burrower going under, out of sight, then out again where it went down
            if (threatened) c.timer = std::max(c.timer, 4.0);
            const bool out = c.timer <= 0;
            c.sink = clampd(c.sink + (out ? -1.0 : 1.0) * dt / 1.2, 0, 1);
            c.speed = 0;
            if (out && c.sink <= 0) { c.state = CS_IDLE; c.timer = 2 + 2 * hash01(c.seed ^ (uint64_t)(t * 3)); }
            continue;
        }
        if (c.state == CS_CLIMB) {   // a climber to its trunk, up it, holding on while the explorer is near, down again
            const double tdx = c.treeX - c.x, tdz = c.treeZ - c.z, td = std::sqrt(tdx * tdx + tdz * tdz);
            if (c.climbY <= 0 && td > c.treeR + 0.5) {   // running to it (the move below)
                c.heading = std::atan2(tdx, tdz); c.speed = 0.8 * sp.runSpeed;
                if (c.timer < -20) { c.state = CS_IDLE; c.timer = 1; c.speed = 0; }   // it never got there
            } else {
                if (c.climbY <= 0) {   // at the trunk: onto it, near the side it came from (each its own way round and its own height), clear of the others
                    c.treeTop *= 0.55 + 0.45 * hash01(c.seed ^ 0xC11CULL);
                    c.treeA = climbSide(i, std::atan2(td > 1e-6 ? -tdx : 0, td > 1e-6 ? -tdz : 1) + 1.6 * (hash01(c.seed ^ 0xC11BULL) - 0.5), sp, k);
                    c.climbY = 0.01;
                }
                c.speed = 0;
                if (threatened) c.timer = std::max(c.timer, 6.0);
                const bool down = c.timer <= 0 && c.calmT > 8;
                const double rate = 0.5 * sp.walkSpeed;
                const double y0 = c.climbY;
                c.climbY = clampd(c.climbY + (down ? -rate : rate) * dt, 0, c.treeTop);
                trunkPoint(c, c.climbY, c.x, c.z);   // (B-468: on its side of the trunk, the axis leaning and the girth tapering as it goes)
                c.stride = speciesStride(sp, k, rate, g); c.gait = sp.gait;
                c.gaitPhase += std::fabs(c.climbY - y0) / std::max(c.stride, 0.05);
                if (down && c.climbY <= 0) {   // off it, a step out from the trunk
                    const double nx = c.x - c.treeX, nz = c.z - c.treeZ, nl = std::max(1e-6, std::sqrt(nx * nx + nz * nz));
                    c.x = c.treeX + nx / nl * (c.treeR + 0.3 + 0.5 * sp.body.length * k); c.z = c.treeZ + nz / nl * (c.treeR + 0.3 + 0.5 * sp.body.length * k);
                    c.state = CS_IDLE; c.timer = 2;
                }
                continue;
            }
        }
        if (c.state == CS_DISPLAY) {   // one on its ground facing the explorer, head up, calling; then a charge if they are still near
            c.heading = std::atan2(dpx, dpz); c.speed = 0;
            if (c.timer <= 0) {
                const double hx = c.homeX - c.x, hz = c.homeZ - c.z;
                if (dp < 14 && c.hungerT <= 0 && !rolling) { c.state = CS_CHARGE; c.timer = 2.5; }
                else if (dp > 26) { c.state = CS_WALK; c.timer = 4; c.heading = std::atan2(hx, hz); c.speed = sp.walkSpeed; }   // back to its ground
                else c.timer = 1.5;
            }
        }
        if (c.state == CS_CHARGE) {   // straight at the explorer, stopping short; back to watching
            c.heading = std::atan2(dpx, dpz); c.speed = 0.8 * sp.runSpeed;
            const double hx = c.homeX - c.x, hz = c.homeZ - c.z;
            if (dp < 4 + 0.5 * c.size || c.timer <= 0 || hx * hx + hz * hz > 45.0 * 45.0) { c.state = CS_DISPLAY; c.timer = 2; c.speed = 0; c.hungerT = 8; }
        }
        if (c.state == CS_FEED) {   // J-08: to its remains, its head down at them, then a rest (a kill eaten to its ribs)
            Remains* f = nullptr;
            for (Remains& r : remains) if (r.seed == c.food) { f = &r; break; }
            if (!f || f->stage >= RS_BONES) { c.state = CS_IDLE; c.timer = 2; c.food = 0; c.speed = 0; }
            else {
                const double fx = f->x - c.x, fz = f->z - c.z, fd = std::sqrt(fx * fx + fz * fz);
                const double stand = 0.45 * sp.body.length * k + 0.35 * f->size + 0.2;
                c.heading = std::atan2(fx, fz);
                if (fd > stand + 0.4) c.speed = sp.walkSpeed * (fd > 20 ? 1.6 : 1.0);
                else {
                    c.speed = 0;
                    if (f->kill) {
                        f->eaten += dt;
                        for (DeadKept& d : deadKept) if (d.seed == f->seed) d.eaten = f->eaten;   // (B-489: kept as it goes, for the guide)
                    }
                }
                if (c.timer <= 0) {
                    c.state = CS_REST; c.timer = 15 + 10 * hash01(c.seed ^ (uint64_t)(t * 3)); c.speed = 0; c.food = 0;
                    c.hungerT = 120 + 60 * hash01(c.seed ^ (uint64_t)(t * 13));
                }
            }
        }
        // J-07: a quarry's flight: a hunter of its kind chasing within 40 m or stalking within 12 m (its herd mates startle too)
        if (c.state != CS_FLEE && c.state != CS_DISPLAY && c.state != CS_CHARGE && c.state != CS_CLIMB && c.state != CS_FEED)
            for (const Hunting& h : hunting) {
                if (h.prey != c.species) continue;
                const double hx = c.x - h.x, hz = c.z - h.z, hd = std::sqrt(hx * hx + hz * hz);
                if (hd > (h.chasing ? 40.0 : 12.0)) continue;
                if (sp.way == WAY_BURROWER) { underGround(12 + 10 * hash01(c.seed ^ (uint64_t)t)); break; }
                if (sp.way == WAY_CLIMBER && upATree(10)) break;
                c.state = CS_FLEE; c.timer = 6 + 3 * hash01(c.seed ^ (uint64_t)(t * 5)); c.heading = std::atan2(hx, hz) + 0.6 * (hash01(c.seed ^ (uint64_t)(t * 2)) - 0.5);
                c.speed = sp.runSpeed; c.headPitch = 0; c.lift = 0; c.reach = 0;
                for (size_t j = 0; j < critters.size(); j++) {   // the herd mates within 40 m run the same way (a stampede)
                    Critter& o = critters[j];
                    if (j == i || o.herd != c.herd || o.state == CS_FLEE || o.state == CS_UNDER || o.state == CS_CLIMB) continue;
                    const double sx = o.x - c.x, sz = o.z - c.z;
                    if (sx * sx + sz * sz < 40.0 * 40.0) {
                        o.state = CS_FLEE; o.timer = c.timer + 0.5 * hash01(o.seed ^ (uint64_t)(t * 9)); o.heading = c.heading + 0.5 * (hash01(o.seed ^ (uint64_t)(t * 4)) - 0.5);
                        o.speed = sp.runSpeed; o.headPitch = 0; o.lift = 0; o.reach = 0;
                    }
                }
                break;
            }
        if (threatened && c.state != CS_ALERT && c.state != CS_FLEE && c.state != CS_DISPLAY && c.state != CS_CHARGE && c.state != CS_UNDER && c.state != CS_CLIMB &&
            !(c.state == CS_CURIOUS && dp > 8)) {
            if (territorial) {   // J-07: it faces the explorer and calls
                c.state = CS_DISPLAY; c.timer = 2.5; c.speed = 0; c.headPitch = 0; c.lift = 0; c.reach = 0;
                if (t - c.calledT > 4) { pendingCalls.push_back({sp.call, sp.callPitch * 0.85}); c.calledT = t; callsFired++; }
            } else if (sp.way == WAY_BURROWER) underGround(15 + 15 * hash01(c.seed ^ (uint64_t)(t * 3)));   // J-07: down its burrow
            else if (!(sp.way == WAY_CLIMBER && upATree(10))) {   // J-07: a climber up the nearest trunk; any other alert
                c.state = CS_ALERT; c.timer = 0.8 + 0.8 * hash01(c.seed ^ (uint64_t)(t * 3)); c.speed = 0; c.headPitch = 0; c.lift = std::min(c.lift, 0.5); c.reach = 0;
                if (t - c.calledT > 6) { pendingCalls.push_back({sp.call, sp.callPitch * (0.9 + 0.2 * hash01(c.seed))}); c.calledT = t; callsFired++; }
            }
            if (c.state == CS_UNDER || c.state == CS_CLIMB) continue;
        }
        // J-07: a hunter after its quarry: low and slow, then the chase (the quarry swerves at the last moment), then a rest
        if (c.state == CS_STALK || c.state == CS_CHASE) {
            Critter* q = nullptr;
            for (Critter& o : critters) if (o.seed == c.quarry) { q = &o; break; }
            const double qx = q ? q->x - c.x : 0, qz = q ? q->z - c.z : 0, qd = std::sqrt(qx * qx + qz * qz);
            if (!q || q->state == CS_UNDER || q->climbY > 0.3 || qd > 180) { c.state = CS_IDLE; c.timer = 3; c.speed = 0; c.hungerT = 40; c.quarry = 0; }
            else if (c.state == CS_STALK) {   // the chase from about two and a half seconds' run off, or once the quarry runs
                c.heading = std::atan2(qx, qz); c.speed = 0.6 * sp.walkSpeed + 0.1 * sp.runSpeed;
                if (qd < clampd(2.5 * sp.runSpeed, std::max(9.0, 5 * c.size), 30.0) || q->state == CS_FLEE || c.timer <= 0) { c.state = CS_CHASE; c.timer = 6 + 3 * hash01(c.seed ^ (uint64_t)(t * 3)); }
            } else {
                const double lead = std::min(1.0, qd / std::max(sp.runSpeed, 0.1));
                c.heading = std::atan2(qx + std::sin(q->heading) * q->speed * lead, qz + std::cos(q->heading) * q->speed * lead);
                c.speed = sp.runSpeed;
                if (qd < 1.0 + 0.5 * (c.size + q->size)) {
                    if (!c.rolled) {   // J-08: one try a chase: caught by a chance that grows with the hunter's size against its quarry's
                        c.rolled = true;
                        const double p = clampd(0.2 + 0.25 * c.size / std::max(q->size, 0.05), 0.15, 0.6);
                        if (hash01(mix64(c.seed ^ q->seed ^ (uint64_t)(t * 3))) < p) {
                            const Remains r = killCritter(*q);
                            for (Critter& o : critters)   // the pack to it
                                if (o.quarry == q->seed && (o.state == CS_CHASE || o.state == CS_STALK)) {
                                    o.state = CS_FEED; o.food = r.seed; o.quarry = 0; o.timer = 40 + 30 * hash01(o.seed ^ (uint64_t)(t * 5));
                                }
                        }
                    }
                    if (!q->killed) q->heading += hash01(q->seed ^ (uint64_t)(t * 7)) < 0.5 ? 1.7 : -1.7;   // swerving at the last moment
                }
                if (c.timer <= 0) {
                    c.state = CS_REST; c.timer = 12 + 10 * hash01(c.seed ^ (uint64_t)(t * 3)); c.speed = 0; c.quarry = 0;
                    c.hungerT = 70 + 60 * hash01(c.seed ^ (uint64_t)(t * 11));
                }
            }
        }
        if (c.state == CS_TRAIL) {   // J-07: a scavenger keeping 25 to 60 m from its hunter, nose to the ground between
            const Critter* h = nullptr;
            for (const Critter& o : critters) if (o.seed == c.quarry) { h = &o; break; }
            if (!h) { c.state = CS_IDLE; c.timer = 2; c.speed = 0; }
            else {
                const double hx = h->x - c.x, hz = h->z - c.z, hd = std::sqrt(hx * hx + hz * hz);
                if (hd > 60) { c.heading = std::atan2(hx, hz); c.speed = sp.walkSpeed; }
                else if (hd < 25) { c.heading = std::atan2(-hx, -hz); c.speed = 0.7 * sp.walkSpeed; }
                else c.speed = 0;
            }
        }
        if (c.state == CS_BROWSE) {   // J-07: a browser to its tree, then its neck up into the leaves
            const double tdx = c.treeX - c.x, tdz = c.treeZ - c.z, td = std::sqrt(tdx * tdx + tdz * tdz);
            const double stand = c.treeR + 0.4 * sp.body.length * k + 0.5 * sp.body.neck * k;
            if (td > stand + 0.5) { c.heading = std::atan2(tdx, tdz); c.speed = sp.walkSpeed; }
            else { c.speed = 0; c.heading = std::atan2(tdx, tdz); }
        }
        if (c.timer <= 0 && c.state != CS_DISPLAY && c.state != CS_CHARGE && c.state != CS_STALK && c.state != CS_CHASE && c.state != CS_CLIMB && c.state != CS_FEED) {
            Rng r(c.seed ^ (uint64_t)(t * 10));
            bool leader = true;
            for (size_t j = 0; j < i; j++) if (critters[j].herd == c.herd) { leader = false; break; }
            const Critter* lead = nullptr;
            if (!leader) for (size_t j = 0; j < i; j++) if (critters[j].herd == c.herd) { lead = &critters[j]; break; }
            if (c.state == CS_ALERT) {
                // decide: flee, or come closer when curious; the herd mates within 40 m startle too (stampede)
                if (temper == TEMP_CURIOUS && dp > 10 && !(inBuggy && std::fabs(buggy.speed) > 3)) {
                    c.state = CS_CURIOUS; c.timer = 6 + 6 * r.uni(); c.speed = sp.walkSpeed;
                } else {
                    c.state = CS_FLEE; c.timer = 5 + 3 * r.uni(); c.heading = std::atan2(-dpx, -dpz) + r.sym(0.35); c.speed = sp.runSpeed;
                    for (size_t j = 0; j < critters.size(); j++) {
                        Critter& o = critters[j];
                        if (j == i || o.herd != c.herd || o.state == CS_FLEE || o.state == CS_ALERT || o.state == CS_UNDER || o.state == CS_CLIMB) continue;
                        double sx = o.x - c.x, sz = o.z - c.z;
                        if (sx * sx + sz * sz < 40.0 * 40.0) { o.state = CS_ALERT; o.timer = 0.3 + 0.5 * r.uni(); o.speed = 0; }
                    }
                }
            } else if (c.state == CS_FLEE) {
                c.state = CS_IDLE; c.timer = 2 + 3 * r.uni(); c.speed = 0;
            } else {
                double wx = 0, wz = 0;
                bool thirsty = localTime > 0.55 && localTime < 0.78 && sp.diet != 1 && waterNear(site, c.x, c.z, wx, wz);
                double dwx = wx - c.x, dwz = wz - c.z, dw = thirsty ? std::sqrt(dwx * dwx + dwz * dwz) : 1e9;
                double roll = r.uni();
                // J-07: the ways' own choices (a burrower rests under the ground, a climber up a tree; a hunter hunts, a pack with its leader;
                // a scavenger trails a hunter; a browser browses where there are trees); the rest as before
                Rng w(c.seed ^ (uint64_t)(t * 10) ^ 0x57A1ULL);
                const Critter* hunter = sp.way == WAY_SCAVENGER && roll < 0.55 ? nearestHunter(c.x, c.z, 300) : nullptr;
                // J-08: remains to feed at: the pack leader's, else the nearest fresh, bloated or sunken ones within 120 m (a hunter's) or 300 m (a
                // scavenger's, where no hunter feeds); false with none
                auto feedAt = [&](const Critter* ld, double u) {
                    uint64_t food = ld && ld->state == CS_FEED ? ld->food : 0;
                    if (!food) {
                        const double within = sp.way == WAY_SCAVENGER ? 300.0 : 120.0;
                        double bd = within * within;
                        for (const Remains& r : remains) {
                            if (r.old || r.stage > RS_SUNKEN || r.species == c.species) continue;
                            const double d2 = (r.x - c.x) * (r.x - c.x) + (r.z - c.z) * (r.z - c.z);
                            if (d2 >= bd) continue;
                            bool taken = false;
                            if (sp.way == WAY_SCAVENGER)
                                for (const Critter& o : critters)
                                    if (o.state == CS_FEED && o.food == r.seed && speciesOf(o.species) && speciesOf(o.species)->way == WAY_HUNTER) { taken = true; break; }
                            if (!taken) { bd = d2; food = r.seed; }
                        }
                    }
                    if (!food) return false;
                    c.state = CS_FEED; c.food = food; c.timer = ld && ld->state == CS_FEED ? ld->timer : 30 + 30 * u; c.speed = sp.walkSpeed;
                    return true;
                };
                TreeInst T;
                bool chosen = true;
                if (restWanted && roll < 0.8 && sp.way == WAY_BURROWER) underGround(20 + 20 * w.uni());
                else if (restWanted && roll < 0.8 && sp.way == WAY_CLIMBER && upATree(20 + 20 * w.uni())) {}
                else if (sp.way == WAY_HUNTER && lead && (lead->state == CS_STALK || lead->state == CS_CHASE)) {
                    c.state = lead->state; c.quarry = lead->quarry; c.timer = lead->timer; c.rolled = false;
                } else if (!remains.empty() && ((sp.way == WAY_HUNTER && (lead ? lead->state == CS_FEED : (c.hungerT <= 0 && !restWanted))) ||
                                                sp.way == WAY_SCAVENGER) && feedAt(lead, w.uni())) {
                    // J-08: at remains (the pack's leader's, the nearest fresh ones for a hungry hunter or a scavenger)
                } else if (sp.way == WAY_HUNTER && leader && !restWanted && c.hungerT <= 0 && sp.prey >= 0) {
                    const Critter* best = nullptr; double bd = 150.0 * 150.0;
                    for (const Critter& o : critters) {
                        if (o.species != sp.prey || o.state == CS_UNDER || o.climbY > 0) continue;
                        const double d2 = (o.x - c.x) * (o.x - c.x) + (o.z - c.z) * (o.z - c.z);
                        if (d2 < bd) { bd = d2; best = &o; }
                    }
                    if (best) { c.state = CS_STALK; c.quarry = best->seed; c.timer = 30 + 10 * w.uni(); c.speed = 0.6 * sp.walkSpeed + 0.1 * sp.runSpeed; c.rolled = false; }
                    else { c.hungerT = 20 + 20 * w.uni(); chosen = false; }
                } else if (hunter) { c.state = CS_TRAIL; c.quarry = hunter->seed; c.timer = 6 + 4 * w.uni(); }
                else if (sp.way == WAY_BROWSER && roll < 0.4 && !restWanted && treeNear(c.x, c.z, 30, T)) {
                    c.state = CS_BROWSE; c.timer = 10 + 8 * w.uni(); c.treeX = T.x; c.treeZ = T.z; c.treeR = 0.12 + T.h * 0.02; c.speed = sp.walkSpeed;
                } else chosen = false;
                if (!chosen && (c.sick == MF_SPROUTING || (changedTo(CHG_CLIMBS) && roll < 0.5))) {   // J-08: up to the high ground, to stay there
                    double best = site.groundHeight(c.x, c.z) + 0.3, bh = c.heading;
                    bool up = false;
                    for (int q = 0; q < 8; q++) {
                        const double a = q * TAU / 8, y = site.groundHeight(c.x + std::sin(a) * 12, c.z + std::cos(a) * 12);
                        if (y > best) { best = y; bh = a; up = true; }
                    }
                    if (up) { c.state = CS_WALK; c.timer = 4 + 3 * r.uni(); c.speed = sp.walkSpeed; c.heading = bh; }
                    else { c.state = CS_IDLE; c.timer = 10 + 10 * r.uni(); c.speed = 0; }
                    chosen = true;
                }
                if (!chosen) {
                    if (restWanted && roll < 0.8) { c.state = CS_REST; c.timer = 20 + 20 * r.uni(); c.speed = 0; }
                    else if (thirsty && dw < 5) { c.state = CS_DRINK; c.timer = 4 + 4 * r.uni(); c.speed = 0; c.heading = std::atan2(dwx, dwz); }
                    else if (thirsty && roll < 0.6) { c.state = CS_WALK; c.timer = 5 + 5 * r.uni(); c.speed = sp.walkSpeed; c.heading = std::atan2(dwx, dwz) + r.sym(0.2); }
                    else if (roll < 0.4) { c.state = CS_GRAZE; c.timer = 5 + 4 * r.uni(); c.speed = 0; }
                    else if (roll < 0.75) {
                        c.state = CS_WALK; c.timer = 4 + 5 * r.uni(); c.speed = sp.walkSpeed * (0.6 + 0.5 * r.uni());
                        c.heading += r.sym(1.2);
                        if (lead) {   // followers keep 3-12 m from the leader and take its heading with noise
                            double lx = lead->x - c.x, lz = lead->z - c.z, ld = std::sqrt(lx * lx + lz * lz);
                            const double farD = changedTo(CHG_SWARMS) ? 4.0 : 12.0, nearD = changedTo(CHG_SWARMS) ? 1.5 : 3.0;   // J-08: a swarming one close in
                            if (ld > farD) c.heading = std::atan2(lx, lz) + r.sym(0.3);
                            else if (ld < nearD) c.heading = std::atan2(-lx, -lz) + r.sym(0.5);
                            else c.heading = lead->heading + r.sym(0.6);
                        }
                        double hx = herds[c.herd].cx - c.x, hz = herds[c.herd].cz - c.z;
                        if (hx * hx + hz * hz > 60.0 * 60.0) c.heading = std::atan2(hx, hz) + r.sym(0.4);
                        if (sp.social == SOCIAL_TERRITORIAL) {   // J-07: within 40 m of its ground
                            const double gx = c.homeX - c.x, gz = c.homeZ - c.z;
                            if (gx * gx + gz * gz > 40.0 * 40.0) c.heading = std::atan2(gx, gz);
                        }
                    } else { c.state = CS_IDLE; c.timer = 2 + 3 * r.uni(); c.speed = 0; }
                }
                // a call now and then from the leader
                if (leader && roll > 0.93 && t - c.calledT > 15 && dp < 120) { pendingCalls.push_back({sp.call, sp.callPitch}); c.calledT = t; callsFired++; }
            }
        }
        if (c.state == CS_CURIOUS) {   // walk toward the explorer and stop at 10 m, facing them
            if (dp > 10) { c.heading = std::atan2(dpx, dpz); c.speed = sp.walkSpeed; } else { c.speed = 0; c.heading = std::atan2(dpx, dpz); }
        }
        // posture targets (J-07: a scavenger's nose down on the trail, a browser's neck up at its tree, a stalker low)
        const bool atTree = c.state == CS_BROWSE && c.speed == 0;
        double wantHead = (c.state == CS_GRAZE || c.state == CS_DRINK || (c.state == CS_FEED && c.speed == 0)) ? 1.0 : (c.state == CS_TRAIL ? 0.55 : 0.0), wantLift = c.state == CS_REST ? 1.0 : 0.0;
        c.headPitch += (wantHead - c.headPitch) * (1 - std::exp(-dt * 1.5));
        c.lift += (wantLift - c.lift) * (1 - std::exp(-dt * 0.8));
        c.reach += ((atTree ? 1.0 : 0.0) - c.reach) * (1 - std::exp(-dt * 1.2));
        c.crouch += ((c.state == CS_STALK ? 1.0 : 0.0) - c.crouch) * (1 - std::exp(-dt * 2.0));
        if (c.state == CS_GRAZE && std::fmod(t + c.seed % 13, 7.0) < 1.5) c.headPitch += (0 - c.headPitch) * (1 - std::exp(-dt * 3));   // looks up now and then
        // movement, feet on the ground, no water, no steep slopes (J-07: a climber takes them)
        const bool running = c.state == CS_FLEE || c.state == CS_CHASE || c.state == CS_CHARGE || (c.state == CS_CLIMB && c.climbY <= 0);
        // J-08: the sick slow (a sprouting one most), a limping one and one with a leg gone too; a crazed one and a circler going round
        double slow = 1;
        switch (c.sick) {
            case MF_WASTING: case MF_STONE_SKIN: slow = 0.55; break;
            case MF_MELTING: case MF_SPROUTING: slow = 0.45; break;
            case MF_BLOATED: slow = 0.6; break;
            case MF_GROWTHS: slow = 0.75; break;
            default: break;
        }
        if (changedTo(CHG_LIMPS) || changedTo(CHG_MISSING_LEG)) slow *= 0.6;
        if ((crazed || changedTo(CHG_CIRCLES)) && c.speed > 0) c.heading += (crazed ? 1.4 : 0.9) * dt;
        const double speed = c.speed * slow;
        c.gait = speciesGait(sp, k, speed, g, running);
        c.stride = speciesStride(sp, k, speed, g);
        if (c.speed > 0 && c.lift < 0.5) {
            double nx = c.x + std::sin(c.heading) * speed * dt, nz = c.z + std::cos(c.heading) * speed * dt;
            double gy = site.groundHeight(nx, nz);
            double slope = std::fabs(gy - site.groundHeight(c.x, c.z));
            bool wet = gy < site.waterNowAt(nx, nz) + 0.3;   // Q-02: the tide's
            const double steep = (0.6 * speed * dt + 0.3) * (sp.way == WAY_CLIMBER ? 3.0 : 1.0);
            if (slope > steep || wet) { c.heading += 2.2; c.timer = std::min(c.timer, 1.0); }
            else {
                double moved = std::sqrt((nx - c.x) * (nx - c.x) + (nz - c.z) * (nz - c.z));
                c.x = nx; c.z = nz;
                c.gaitPhase += moved / std::max(c.stride, 0.05);
                // tracks in sand and snow
                c.trackAccum += moved;
                if (c.trackAccum > 0.7) {
                    c.trackAccum = 0;
                    int mat = site.lod0.at((int)std::floor(c.x / 16), (int)std::floor(c.z / 16)).material;
                    if (mat == MAT_SAND || mat == MAT_SNOW) {
                        if (c.tracks.size() < 16) c.tracks.push_back({(float)c.x, (float)c.z});
                        else { c.tracks[c.trackHead] = {(float)c.x, (float)c.z}; c.trackHead = (c.trackHead + 1) % 16; }
                    }
                }
                if (dp < 20) hoof += std::min(1.0, c.size) * (speed / std::max(sp.runSpeed, 0.1)) * 0.5;
            }
            { double hx = herds[c.herd].cx - c.x, hz = herds[c.herd].cz - c.z; if (hx * hx + hz * hz > 300.0 * 300.0) c.heading = std::atan2(hx, hz); }   // a herd stays within 300 m of its spot
        }
    }
    hoofLevel = std::min(1.0, hoof);
    // J-08: the killed gone from the herds (their remains lie where they fell)
    if (std::any_of(critters.begin(), critters.end(), [](const Critter& c) { return c.killed; }))
        critters.erase(std::remove_if(critters.begin(), critters.end(), [](const Critter& c) { return c.killed; }), critters.end());
    lastLifeMs[0] = nowMs() - t0;
}

// B-467: a swimmer of the shallows' place in its 100 m cell, kept for the landing: a stretch of the water 0.9 m deep (the middle of the 0.3 to
// 1.5 m it keeps to) it swims along, read on the 16 m ground and the mean water (`groundHeightCoarse`, `waterAt`: the same from anywhere at the
// landing, whatever the tide). From the first of the cell's eight hashed places that deep, halved toward the first of them shallower (dry ground
// too: the depth runs through 0.9 m between); with none shallower, the shallowest of them over 0.3 m, under 3 m; with all eight dry, the same
// in a grid of 7 by 7 over the cell; then up to three points 4 m apart either way along the line of that depth (B-492: each step along the
// line where the last point lies, its chord within the band)
const SurfaceView::ShallowsPath& SurfaceView::shallowsPathOf(int ccx, int ccz, uint64_t hs) {
    const uint64_t key = (uint64_t)(uint32_t)ccx << 32 | (uint32_t)ccz;
    const auto it = shallowsPaths.find(key);
    if (it != shallowsPaths.end()) return it->second;
    ShallowsPath& P = shallowsPaths[key];
    constexpr double WANT = 0.9;
    auto depth = [&](double x, double z) { const double w = site.waterAt(x, z); return w > -1e8 ? w - site.groundHeightCoarse(x, z) : -1.0; };
    double px[8], pz[8], pd[8];
    int deep = -1, shallower = -1, shallowest = -1;
    for (int k = 0; k < 8; k++) {
        px[k] = ccx * 100.0 + 5 + hash01(mix64(hs + 31 + 2 * k)) * 90; pz[k] = ccz * 100.0 + 5 + hash01(mix64(hs + 32 + 2 * k)) * 90;
        pd[k] = depth(px[k], pz[k]);
        if (pd[k] >= WANT && deep < 0) deep = k;
        if (pd[k] < WANT && shallower < 0) shallower = k;
        if (pd[k] > 0.3 && (shallowest < 0 || pd[k] < pd[shallowest])) shallowest = k;
    }
    // the point at WANT halved for between a deep one and a shallower one, and the line of that depth there (across its slope)
    auto onDepth = [&](double ax, double az, double bx, double bz, double& x, double& z) {
        for (int q = 0; q < 14; q++) {
            const double mx = 0.5 * (ax + bx), mz = 0.5 * (az + bz);
            if (depth(mx, mz) >= WANT) { ax = mx; az = mz; } else { bx = mx; bz = mz; }
        }
        x = 0.5 * (ax + bx); z = 0.5 * (az + bz);
    };
    double sx, sz;
    if (deep >= 0 && shallower >= 0) onDepth(px[deep], pz[deep], px[shallower], pz[shallower], sx, sz);
    else if (shallowest >= 0 && pd[shallowest] < 3.0) { sx = px[shallowest]; sz = pz[shallowest]; }
    else {   // (the eight all dry: the cell's water in a grid of 7 by 7, the first deep place beside a shallower one, else its deepest over 0.3 m)
        double gd[7][7];
        for (int j = 0; j < 7; j++) for (int i = 0; i < 7; i++) gd[j][i] = depth(ccx * 100.0 + 11 + 13 * i, ccz * 100.0 + 11 + 13 * j);
        int bi = -1, bj = -1, ni = -1, nj = -1;
        for (int j = 0; j < 7 && bi < 0; j++)
            for (int i = 0; i < 7 && bi < 0; i++) {
                if (gd[j][i] < WANT) continue;
                static const int N[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (const auto& d : N) {
                    const int ii = i + d[0], jj = j + d[1];
                    if (ii >= 0 && ii < 7 && jj >= 0 && jj < 7 && gd[jj][ii] < WANT) { bi = i; bj = j; ni = ii; nj = jj; break; }
                }
            }
        if (bi >= 0) onDepth(ccx * 100.0 + 11 + 13 * bi, ccz * 100.0 + 11 + 13 * bj, ccx * 100.0 + 11 + 13 * ni, ccz * 100.0 + 11 + 13 * nj, sx, sz);
        else {
            double best = 0.3;
            for (int j = 0; j < 7; j++)
                for (int i = 0; i < 7; i++)
                    if (gd[j][i] > best && gd[j][i] < 3.0) { best = gd[j][i]; bi = i; bj = j; }
            if (bi < 0) return P;
            sx = ccx * 100.0 + 11 + 13 * bi; sz = ccz * 100.0 + 11 + 13 * bj;
        }
    }
    // B-492: each point's own slope (toward the deeper water), so the stretch bends with a curving shore; a step whose chord leaves the 0.3 to
    // 1.5 m band is tried again at half its length, then the stretch ends there
    auto slopeAt = [&](double x, double z, double& ux, double& uz) {
        const double gx = depth(x + 2, z) - depth(x - 2, z), gz = depth(x, z + 2) - depth(x, z - 2), gl = std::hypot(gx, gz);
        if (gl < 1e-6) return false;
        ux = gx / gl; uz = gz / gl;
        return true;
    };
    auto inBand = [&](double x, double z) { const double d = depth(x, z); return d >= 0.3 && d <= 1.5; };
    double gx = 1, gz = 0;
    slopeAt(sx, sz, gx, gz);
    double xs[7], zs[7];
    xs[3] = sx; zs[3] = sz;
    int lo = 3, hi = 3;
    for (int side = -1; side <= 1; side += 2) {
        double tx = -gz * side, tz = gx * side;   // the way along the line this side goes (the last step's)
        for (int j = 1; j <= 3; j++) {
            const int at = 3 + side * j, from = at - side;
            double ux, uz;
            if (!slopeAt(xs[from], zs[from], ux, uz)) break;
            double nx = -uz, nz = ux;   // along the line there, the way it was going
            if (nx * tx + nz * tz < 0) { nx = -nx; nz = -nz; }
            bool ok = false;
            for (double step = 4; step >= 2 && !ok; step -= 2) {
                const double qx = xs[from] + nx * step, qz = zs[from] + nz * step;   // along the line, then back onto the depth across it
                if (depth(qx + ux * 8, qz + uz * 8) < WANT || depth(qx - ux * 8, qz - uz * 8) >= WANT) continue;
                double px2, pz2;
                onDepth(qx + ux * 8, qz + uz * 8, qx - ux * 8, qz - uz * 8, px2, pz2);
                const double cl = std::hypot(px2 - xs[from], pz2 - zs[from]);
                if (cl < 0.5) continue;
                bool chord = true;
                for (double u = 0.25; u < cl && chord; u += 0.25) chord = inBand(xs[from] + (px2 - xs[from]) * u / cl, zs[from] + (pz2 - zs[from]) * u / cl);
                if (!chord) continue;
                xs[at] = px2; zs[at] = pz2; tx = (px2 - xs[from]) / cl; tz = (pz2 - zs[from]) / cl;
                ok = true;
            }
            if (!ok) break;
            if (side < 0) lo = at; else hi = at;
        }
    }
    P.n = hi - lo + 1;
    for (int q = 0; q < P.n; q++) { P.x[q] = xs[lo + q]; P.z[q] = zs[lo + q]; }
    P.ok = true;
    return P;
}
bool SurfaceView::shallowsSpot(int ccx, int ccz, uint64_t hs, double& cx, double& cz) {
    const ShallowsPath& P = shallowsPathOf(ccx, ccz, hs);
    if (!P.ok) return false;
    cx = P.x[P.n / 2]; cz = P.z[P.n / 2];
    return true;
}
bool SurfaceView::testShallowsSpot(int ccx, int ccz, double& x, double& z) {
    return shallowsSpot(ccx, ccz, hash2i(ccx, ccz, site.gen.seed ^ 0xF1A), x, z);
}
const SurfaceView::ShallowsPath& SurfaceView::testShallowsPath(int ccx, int ccz) { return shallowsPathOf(ccx, ccz, hash2i(ccx, ccz, site.gen.seed ^ 0xF1A)); }

// the world's swimmers within two 100 m cells of a point at a time, those at the surface or in a leap: each cell's spot (a hash's third of
// them) holds one of its swimmer species circling, where the water is deep enough for it (J-07: a shoal's three in line; one of the shallows
// round the first of eight places of its cell where the water is 0.3 to 1.5 m deep, its back always up; a leaper under 2 m of water at
// least, out in an arc every 7 to 13 s)
void SurfaceView::swimmersNear(double px, double pz, double t, std::vector<SwimmerAt>& out, std::vector<SwimmerAt>* dead) {
    out.clear();
    if (dead) dead->clear();
    if (!site.hasWater || !kindHas(site.gen.type, KT_FAUNA) || bestiary.swimmerCount <= 0) return;
    const int ccx = (int)std::floor(px / 100), ccz = (int)std::floor(pz / 100);
    const bool fated = site.gen.made.on() && site.gen.made.fated();
    for (int dz = -2; dz <= 2; dz++)
        for (int dx = -2; dx <= 2; dx++) {
            uint64_t hs = hash2i(ccx + dx, ccz + dz, site.gen.seed ^ 0xF1A);
            if (hash01(hs) > 0.35) continue;
            const int spi = bestiary.firstSwimmer() + (int)(hash01(mix64(hs + 6)) * bestiary.swimmerCount) % bestiary.swimmerCount;
            const Species& sw = bestiary.species[spi];
            const int way = sw.way, members = way == WAY_SHOAL ? 3 : 1;
            double cx = (ccx + dx) * 100.0 + 20 + hash01(mix64(hs + 1)) * 60, cz = (ccz + dz) * 100.0 + 20 + hash01(mix64(hs + 2)) * 60;
            const ShallowsPath* path = way == WAY_SHALLOWS ? &shallowsPathOf(ccx + dx, ccz + dz, hs) : nullptr;
            if (path && !path->ok) continue;
            if (path) { cx = path->x[path->n / 2]; cz = path->z[path->n / 2]; }
            if (silence) continue;   // J-08: none where every animal is gone, the dead's share of the rest gone from their spots
            AnimalFate af;
            if (testSwimmerFate.on()) af = testSwimmerFate;
            else if (fated) {
                af = fateOn(site, bestiary, spi, cx, cz);
                if (af.dead > 0 && hash01(mix64(hs + 77)) < af.dead) continue;
            }
            bool circles = false, limps = false;   // B-490: a changed young's ways in the water (a circler round on its own, a limper at half pace)
            if (af.changed > 0 && spi < (int)speciesChanges.size())
                for (int q = 0; q < speciesChanges[spi].n; q++) { circles |= speciesChanges[spi].changes[q] == CHG_CIRCLES; limps |= speciesChanges[spi].changes[q] == CHG_LIMPS; }
            for (int m = 0; m < members; m++) {
                const uint64_t hm = m ? mix64(hs + 101 * m) : hs;
                SwimmerAt w;
                w.species = spi; w.seed = hm;
                // B-469: sick among the rest and born changed as a herd's members are, by its own hash (a crazed one round twice as fast)
                if (af.on()) {
                    w.sick = af.sick > 0 && hash01(mix64(hm ^ 0x51C4ULL)) < af.sick ? af.sickness : 0;
                    w.changed = af.changed > 0 && hash01(mix64(hm ^ 0xC4A6ULL)) < af.changed;
                    if (w.changed) w.sizeK = af.has(CHG_GIANT) ? 1.6 : (af.has(CHG_DWARF) ? 0.55 : 1.0);
                }
                const double crazed = (w.sick == MF_CRAZED ? 2.2 : 1.0) * (w.changed && limps ? 0.5 : 1.0);   // (its pace)
                if (path) {   // B-467: along its stretch of the shallows and back, its back up
                    double len = 0;
                    for (int q = 1; q < path->n; q++) len += std::hypot(path->x[q] - path->x[q - 1], path->z[q] - path->z[q - 1]);
                    const double v = (0.3 + 0.4 * hash01(mix64(hs + 3))) * crazed, u0 = len > 0.5 ? t * v / len + hash01(mix64(hs + 4)) * 2 : 0.0;
                    const double u = u0 - 2 * std::floor(u0 / 2), along = (u < 1 ? u : 2 - u) * len;   // there and back
                    double acc = 0;
                    w.x = path->x[0]; w.z = path->z[0]; w.hx = 1; w.hz = 0;
                    for (int q = 1; q < path->n; q++) {
                        const double sx = path->x[q] - path->x[q - 1], sz = path->z[q] - path->z[q - 1], sl = std::hypot(sx, sz);
                        if (sl < 1e-6) continue;
                        const double f = clampd((along - acc) / sl, 0, 1);
                        if (along >= acc - 1e-9) { w.x = path->x[q - 1] + sx * f; w.z = path->z[q - 1] + sz * f; w.hx = sx / sl; w.hz = sz / sl; }
                        acc += sl;
                        if (acc >= along) break;
                    }
                    if (u >= 1) { w.hx = -w.hx; w.hz = -w.hz; }
                    if (len <= 0.5) {   // (a stretch of one point: round it, two metres out)
                        const double a = t * 0.25 * crazed + hash01(mix64(hs + 4)) * TAU;
                        w.x = cx + std::cos(a) * 2; w.z = cz + std::sin(a) * 2; w.hx = -std::sin(a); w.hz = std::cos(a);
                    }
                } else {
                    const double circ = 25;
                    const double a = t * (0.03 + 0.03 * hash01(mix64(hs + 3))) * crazed + hash01(mix64(hs + 4)) * TAU - m * (2.5 + sw.size) / circ;   // a shoal's in line
                    w.x = cx + std::cos(a) * (circ + 2.0 * m); w.z = cz + std::sin(a) * (circ + 2.0 * m);
                    w.hx = -std::sin(a); w.hz = std::cos(a);   // the circle's tangent
                }
                if (w.changed && circles) {   // B-490: round on its own, two metres about its place
                    const double ca = t * 1.3 + hash01(mix64(hm + 11)) * TAU;
                    w.x += 2 * std::cos(ca); w.z += 2 * std::sin(ca); w.hx = -std::sin(ca); w.hz = std::cos(ca);
                }
                if (w.sick == MF_SPROUTING && (w.fellAt = sproutFallOf(hm, w.sick, af)) <= lifeClock) {   // B-490: past its fall, gone from the water
                    if (dead) { w.heading = std::atan2(w.hx, w.hz); dead->push_back(w); }
                    continue;
                }
                w.water = site.waterNowAt(w.x, w.z);
                if (w.water < -1e8) continue;
                const double depthM = w.water - site.groundHeight(w.x, w.z);
                if (path ? (depthM < 0.12 || depthM > 3.0) : depthM < (way == WAY_LEAPER ? 2.0 : 1.5)) continue;   // (the tide out, or a drawn ground's knoll)
                w.surf = std::sin(t * 0.7 + hash01(mix64(hm + 5)) * TAU);   // above the water only part of the time
                if (path) w.surf = 0.6 + 0.4 * w.surf;
                w.heading = std::atan2(w.hx, w.hz);
                if (way == WAY_LEAPER) {   // its leap: every 7 to 13 s an arc out of the water of a second or so
                    const double period = 7 + 6 * hash01(mix64(hm + 7)), span = 1.0 + 0.5 * hash01(mix64(hm + 8));
                    w.leap = std::fmod(t + period * hash01(mix64(hm + 9)), period) / span;
                }
                if (w.leap < 1.0 || w.surf >= (way == WAY_LEAPER ? 0.6 : 0.2)) out.push_back(w);
            }
        }
}

// B-490: a flock's members where they fly at `t` (drawLife's, and the update's for a sprouting one's fall): each one's place in the flock from
// the flock's own draws in their order (as J-07 drew them), its sickness and changed young by its own hash (B-469), and what they make of its
// flight: a crazed one falls to the ground in some of its four seconds' windows (as a land one's falls), lies there a moment and flies up again,
// a stumbler drops a few metres and catches itself; a changed young circles on its own, limps (trailing its place, bobbing) or, born to the
// night, flies on over its resting flock after dark; a sprouting one past its fall is gone (B-490: `flockFalls`)
void SurfaceView::flockMembers(const Flock& f, double t, std::vector<FlockMember>& out) {
    out.clear();
    const Species* sp = f.species >= 0 && f.species < (int)bestiary.species.size() ? &bestiary.species[f.species] : nullptr;
    const int way = sp ? sp->way : WAY_FLOCK;
    const bool swarm = way == WAY_SWARM;
    const double a = f.phase + t * f.speed;
    const double fcx = f.cx + std::cos(a) * f.radius, fcz = f.cz + std::sin(a) * f.radius;
    const double ground = site.groundHeight(fcx, fcz);
    const double base = std::max(ground, site.waterAt(fcx, fcz)) + f.alt;
    const bool down = f.landed > 0 && f.alt < 3;
    const double course = std::atan2(-std::sin(a) * (f.speed >= 0 ? 1 : -1), std::cos(a) * (f.speed >= 0 ? 1 : -1));
    auto changedTo = [&](int ch) {
        if (f.species < 0 || f.species >= (int)speciesChanges.size()) return false;
        const SpeciesChanges& sc = speciesChanges[f.species];
        for (int q = 0; q < sc.n; q++) if (sc.changes[q] == ch) return true;
        return false;
    };
    const bool circles = changedTo(CHG_CIRCLES), limps = changedTo(CHG_LIMPS), stumbles = changedTo(CHG_STUMBLES), night = changedTo(CHG_NIGHT);
    AnimalFate af;
    af.age = f.fateAge;
    Rng r(f.seed);
    for (int i = 0; i < f.n; i++) {
        FlockMember m;
        const double ox = r.sym(18), oz = r.sym(18), oy = r.sym(6);
        m.ph = r.range(0, TAU); m.wf = 6 + 4 * r.uni();
        const double ph = m.ph;
        const uint64_t ms = f.seed + i;
        m.seed = ms;
        m.sick = f.sick > 0 && hash01(mix64(ms ^ 0x51C4ULL)) < f.sick ? f.sickness : 0;
        m.changed = f.changed > 0 && hash01(mix64(ms ^ 0xC4A6ULL)) < f.changed;
        m.aloft = !down;
        if (down && f.perched) { double pa = ph, pr = f.perchR * 0.8 * (0.3 + 0.7 * r.uni()); m.x = f.perchX + std::cos(pa) * pr; m.z = f.perchZ + std::sin(pa) * pr; m.y = f.perchY + r.sym(0.4); }
        else if (swarm) {   // a cloud a few metres across, each member darting about its place
            const double cr = 0.25 * (2 + f.radius);
            m.x = fcx + ox / 18 * cr + 0.5 * std::sin(t * (3.1 + m.wf * 0.3) + ph); m.z = fcz + oz / 18 * cr + 0.5 * std::cos(t * (2.7 + m.wf * 0.25) + ph);
            m.y = (down ? ground + 0.15 + 0.1 * std::fabs(oy) / 6 : base + oy / 6 * 0.8 + 0.3 * std::sin(t * 4.3 + ph));
            m.wf = 30;
        } else { m.x = fcx + ox + 3 * std::sin(t * 0.7 + ph); m.z = fcz + oz + 3 * std::cos(t * 0.5 + ph); m.y = base + oy + 1.5 * std::sin(t * 0.9 + ph); }
        if (m.sick == MF_CRAZED && !down) { m.x += 2.5 * std::sin(t * 3.7 + ph); m.z += 2.5 * std::cos(t * 4.3 + ph); m.y += 1.2 * std::sin(t * 5.9 + ph); }   // (erratic)
        if (m.changed) {   // B-490: a changed young's ways in the air
            if (night && down && env.skyBrightness < 0.3) {   // on over its flock resting after dark
                const double cx = f.perched ? f.perchX : fcx, cz = f.perched ? f.perchZ : fcz, cy = f.perched ? f.perchY : ground;
                m.aloft = true;
                m.x = cx + 8 * std::cos(t * 0.45 + ph); m.z = cz + 8 * std::sin(t * 0.45 + ph); m.y = cy + 6 + 2 * std::sin(t * 0.7 + ph);
            }
            if (m.aloft && circles) { m.x += 3 * std::cos(t * 1.6 + ph); m.z += 3 * std::sin(t * 1.6 + ph); }
            if (m.aloft && limps) { m.x -= 5 * std::sin(course); m.z -= 5 * std::cos(course); m.y += 0.8 * std::sin(t * 2.2 + ph); }
        }
        if (m.aloft && (m.sick == MF_CRAZED || (m.changed && stumbles))) {   // B-490: a fall in some of its four seconds' windows
            const bool crazed = m.sick == MF_CRAZED;
            const double u0 = t + 4 * hash01(mix64(ms ^ 0xFA11ULL));
            const int64_t win = (int64_t)std::floor(u0 / 4.0);
            if (hash01(mix64(ms ^ (uint64_t)win ^ 0xFA12ULL)) < (crazed ? 0.35 : 0.15)) {
                const double u = u0 - 4.0 * (double)win;
                if (crazed) {   // down to the ground, a moment there, up again
                    const double gy = std::max(site.groundHeight(m.x, m.z), site.waterAt(m.x, m.z)), h = std::max(0.0, m.y - gy);
                    if (u < 1.0) m.y = gy + h * (1 - u * u);
                    else if (u < 2.6) { m.y = gy; m.lying = true; }
                    else m.y = gy + h * smoothstep(2.6, 4.0, u);
                } else m.y -= 3 * (u < 0.5 ? u / 0.5 : std::max(0.0, 1 - (u - 0.5)));   // a drop, then caught
            }
        }
        if (m.sick == MF_SPROUTING && sproutFallOf(ms, MF_SPROUTING, af) <= lifeClock) m.gone = true;
        out.push_back(m);
    }
}

void SurfaceView::drawLife(Framebuffer& fb, double t) {
    double t0 = nowMs();
    lifeDrawn.clear();
    const bool swimmersHere = site.hasWater && kindHas(site.gen.type, KT_FAUNA) && bestiary.swimmerCount > 0;   // J-07: with no herd or flock near too
    if (flocks.empty() && critters.empty() && specimens.empty() && remains.empty() && !swimmersHere) { lifeTarget = LifeTarget(); lastLifeMs[1] = 0; return; }
    const Vec3& sd = env.sun.dirLocal;
    const double sunUp = smoothstep(-0.03, 0.06, sd.y);
    const double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    const double lf = env.sun.lightFactor;
    const bool dark = env.skyBrightness < 0.25;
    auto vert = [&](const Vec3& p, double shade) { RVert q; Vec3 v = toView(p.x, p.y, p.z); q.x = v.x; q.y = v.y; q.z = v.z; q.shade = shade; return q; };
    auto fogOf = [&](double dist) { return 1 - std::exp(-dist / env.fogDistance); };
    auto speciesAt = [&](int i) { return i >= 0 && i < (int)bestiary.species.size() && i < (int)rigs.size() ? &bestiary.species[i] : nullptr; };
    lifeJobs.clear();
    // J-08: a sick one's and a changed young's look, the record's marks they give (B-469: a flock's and a shoal's members too)
    auto fateLook = [&](LifeJob& J, int species, int sick, bool changed, uint64_t seed) {
        if (!sick && !changed) return;
        J.sickness = sick; J.seed = seed;
        J.changed = changed && species < (int)changedRigs.size();
        if (J.changed) J.looks = speciesChanges[species].drawn;
        J.in.girth = sick == MF_WASTING ? 0.75 : (sick == MF_BLOATED ? 1.35 : 1.0);
        J.in.sag = sick == MF_MELTING ? 1.0 : 0.0;
        J.marks = (sick ? AM_DYING : 0) | (changed ? AM_CHANGED : 0);
    };
    // J-06: an animal to pose: its ground (3 by 3 heights round it, read here on the main thread: a miss fills the cache), its detail; J-07: what
    // it is doing (the record's; -1 the harness's)
    auto job = [&](int spi, const Vec3& at, double heading, double scale, double dist, bool ground, int doing, bool lying = false) -> LifeJob& {
        const Species& sp = bestiary.species[spi];
        const AnimalRig& R = rigs[spi];
        LifeJob J;
        J.rig = spi;
        J.in.at = at; J.in.heading = heading; J.in.scale = scale; J.in.t = t;
        J.pxPerM = proj.f / std::max(dist, 0.5);
        J.detail = animalDetail(sp.size * scale * J.pxPerM, FB_SCALE);
        J.fog = fogOf(dist);
        J.doing = doing; J.dist = dist;
        J.eyeshine = dark && sp.activity == ACT_NIGHT;   // a night walker's eyes
        if (ground) {
            double spread = 0, reach = 0;
            for (int p = 0; p < std::min(sp.body.pairs, 6); p++) { spread = std::max(spread, R.legSpread[p]); reach = std::max(reach, R.legReach[p]); }
            J.in.gx = std::max(0.3, (0.5 * sp.body.length + 0.7 * reach + 0.3 * sp.body.neck) * scale);
            J.in.gz = std::max(0.2, (0.5 * sp.body.width + spread + 0.1) * scale);
            if (lying) { J.in.gx = std::max(J.in.gx, (0.5 * sp.body.length + 0.6 * sp.body.neck + 0.3 * sp.body.tailLen) * scale); J.in.gz = std::max(J.in.gz, (sp.body.leg + 0.6 * sp.body.width + 0.5 * sp.body.span) * scale); }   // J-08: on its side
            const double hx = std::sin(heading), hz = std::cos(heading);
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++) {
                    const double fx = (i - 1) * J.in.gx, sx = (j - 1) * J.in.gz;
                    J.in.ground[i][j] = site.groundHeight(at.x + hx * fx - hz * sx, at.z + hz * fx + hx * sx);
                }
        }
        lifeJobs.push_back(J);
        return lifeJobs.back();
    };
    // ---- flyers: bodies near, a point and two strokes farther, perched with folded wings (J-07: a swarm in a cloud low over the ground, its
    // members darting; a soarer mostly gliding; B-490: each member where `flockMembers` puts it, a crazed one fallen lying on the ground)
    std::vector<FlockMember> members;
    for (const Flock& f : flocks) {
        const Species* sp = speciesAt(f.species);
        double bsize = sp ? sp->size : 0.5;
        int bank = sp ? hideBanks[sp->body.bank & 1] : 1;
        const int way = sp ? sp->way : WAY_FLOCK;
        const bool swarm = way == WAY_SWARM;
        double a = f.phase + t * f.speed;
        double fcx = f.cx + std::cos(a) * f.radius, fcz = f.cz + std::sin(a) * f.radius;
        double dist = std::sqrt((fcx - camPos.x) * (fcx - camPos.x) + (fcz - camPos.z) * (fcz - camPos.z));
        if (dist > 900) continue;
        const bool flockDown = f.landed > 0 && f.alt < 3;
        const double course = std::atan2(-std::sin(a) * (f.speed >= 0 ? 1 : -1), std::cos(a) * (f.speed >= 0 ? 1 : -1));
        const int doing = sp && sp->plan == PLAN_FLOATER ? DO_DRIFTING : (flockDown ? DO_PERCHED : (swarm ? DO_SWARMING : (way == WAY_SOARER ? DO_SOARING : DO_FLYING)));
        flockMembers(f, t, members);
        for (const FlockMember& m : members) {
            if (m.gone) continue;
            const double bx = m.x, by = m.y, bz = m.z, ph = m.ph, wf = m.wf;
            const int sick = m.sick;
            const bool changed = m.changed, down = !m.aloft;
            const uint64_t ms = m.seed;
            const double md = std::sqrt((bx - camPos.x) * (bx - camPos.x) + (bz - camPos.z) * (bz - camPos.z) + (by - camPos.y) * (by - camPos.y));
            const bool glide = !down && !m.lying && std::sin(t * 0.35 + ph) > (way == WAY_SOARER ? -0.7 : 0.45);   // long glides between flapping (a soarer's most of the time)
            if (sp && md < 160 && bsize * proj.f / std::max(md, 0.5) >= 3.0) {
                const bool landed = (down && sp->plan != PLAN_FLOATER) || m.lying;
                const double hd = swarm ? course + 2.0 * std::sin(t * 1.3 + ph) : course + 0.3 * std::sin(t * 0.4 + ph);
                LifeJob& J = job(f.species, Vec3(bx, by, bz), landed ? ph : hd, changed ? f.changedK : 1.0, md, landed, doing);
                J.in.aloft = !landed;
                J.in.folded = landed && !m.lying;
                J.in.flap = sp->plan == PLAN_FLOATER ? 0 : (sp->plan == PLAN_GLIDER || glide ? 0.12 : std::sin(t * wf + ph));
                J.in.gait = sp->gait; J.in.seed = ms;
                fateLook(J, f.species, sick, changed, ms);
                if (m.lying) {   // B-490: on its side on the ground a moment, its wings beating
                    for (auto& row : J.in.ground) for (double& g : row) g = by;
                    J.in.at.y = by; J.in.lift = 1; J.in.flap = 0.6 * std::sin(t * 13 + ph);
                } else if (landed) {   // perched on its branch or the ground: standing on it
                    const double gy = by - rigs[f.species].spineY;
                    for (auto& row : J.in.ground) for (double& g : row) g = gy;
                    J.in.at.y = gy;
                }
                continue;
            }
            Vec3 v = toView(bx, by, bz);
            if (v.z < 1) continue;
            double shade = 4 + 6 * env.skyBrightness;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = shade;
            int size = dist < 150 && !swarm ? 2 : 1;
            rasterPoint3(fb, p, bank, proj, true, size);
            if (dist < 250 && !swarm && !(sp && sp->plan == PLAN_FLOATER)) {
                double flap = down ? 0.1 : (glide ? 0.2 : std::sin(t * wf + ph));
                double span = 0.5 * (sp ? sp->body.span : 1.8 * bsize) * (down ? 0.35 : 1.0) * (dist < 80 ? 1.0 : 0.6);
                Vec3 l = toView(bx - span, by + 0.4 * flap * span, bz), rgt = toView(bx + span, by + 0.4 * flap * span, bz);
                RVert a1 = p, a2 = p;
                a1.x = l.x; a1.y = l.y; a1.z = l.z; a2.x = rgt.x; a2.y = rgt.y; a2.z = rgt.z;
                rasterLine3(fb, a1, p, bank, proj, true, 1);
                rasterLine3(fb, p, a2, bank, proj, true, 1);
            }
        }
    }
    // B-490: a sprouting flyer falling dead from its flock, its wings folded, turning over
    for (const FlockFall& F : flockFalls) {
        const Species* sp = speciesAt(F.species);
        if (!sp) continue;
        const double md = std::sqrt((F.x - camPos.x) * (F.x - camPos.x) + (F.z - camPos.z) * (F.z - camPos.z) + (F.y - camPos.y) * (F.y - camPos.y));
        if (md > 160 || F.size * proj.f / std::max(md, 0.5) < 3.0) continue;
        LifeJob& J = job(F.species, Vec3(F.x, F.y, F.z), F.heading, F.size / std::max(sp->size, 0.05), md, false, DO_FLYING);
        J.in.aloft = true; J.in.folded = true; J.in.flap = 0; J.in.gait = sp->gait; J.in.seed = F.seed; J.in.tilt = 2.2 * std::sin(F.heading);
        fateLook(J, F.species, F.sick, F.changed, F.seed);
    }
    // ---- night eyes (M4-05): pairs of glints in the dark
    if (env.skyBrightness < 0.3 && kindHas(site.gen.type, KT_FLORA) && !silence) {   // (J-08: none where every animal is gone)
        for (int i = 0; i < 8; i++) {
            uint64_t he = hash2i((int64_t)std::floor(camPos.x / 60), (int64_t)std::floor(camPos.z / 60) + i * 7, site.gen.seed ^ 0xE7E);
            if (hash01(he) < 0.5) continue;
            double ex = std::floor(camPos.x / 60) * 60 + hash01(mix64(he + 1)) * 60, ez = std::floor(camPos.z / 60) * 60 + hash01(mix64(he + 2)) * 60;
            double blink = std::sin(t * (0.6 + hash01(mix64(he + 3))) + hash01(mix64(he + 4)) * TAU);
            if (blink < -0.2) continue;
            double gy = site.groundHeight(ex, ez) + 0.5;
            double hd = hash01(mix64(he + 5)) * TAU;
            for (int k = -1; k <= 1; k += 2) {
                Vec3 v = toView(ex + std::cos(hd) * 0.12 * k, gy, ez - std::sin(hd) * 0.12 * k);
                if (v.z < NEAR_Z) continue;
                RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 58;
                rasterPoint3(fb, p, 1, proj, true, 1);
            }
        }
    }
    // ---- swimmers: the world's several species, each spot one of them: a body near, breaking the surface; fins and a back farther (J-07:
    // `swimmersNear`; a leaper's arc and its splashes)
    if (swimmersHere) {
        std::vector<SwimmerAt> swimmers;
        swimmersNear(camPos.x, camPos.z, t, swimmers);
        for (const SwimmerAt& w : swimmers) {
            const Species* sw = speciesAt(w.species);
            const double ssz = sw ? sw->size : 1.5;
            const double sx = w.x, sz = w.z, wl = w.water, hx = w.hx, hz = w.hz, surf = w.surf, u = w.leap;
            const bool leaping = u < 1.0;
            double dist = std::sqrt((sx - camPos.x) * (sx - camPos.x) + (sz - camPos.z) * (sz - camPos.z));
            if (dist > 250 || ssz / dist * proj.f < 1.0) continue;
            const double breach = sw && sw->way != WAY_LEAPER && surf > 0.97 ? 1.0 : 0.0;
            if (sw && dist < 120 && ssz * proj.f / dist >= 4.0) {   // the body itself, its back at the surface (under it the water hides it)
                if (leaping) {
                    const double len = 1.5 * sw->body.length + 1, H = 0.5 * sw->body.length + 0.5 + 0.3 * sw->body.depth;
                    const double y = wl - 0.3 * sw->body.depth + H * 4 * u * (1 - u);
                    const Vec3 at(sx + hx * (u - 0.5) * len, y, sz + hz * (u - 0.5) * len);
                    LifeJob& J = job(w.species, at, std::atan2(hx, hz), w.sizeK, dist, false, DO_LEAPING);
                    J.in.afloat = true; J.in.gait = GAIT_SWIM; J.in.seed = w.seed; J.in.tilt = std::atan2(H * 4 * (1 - 2 * u), len);
                    fateLook(J, w.species, w.sick, w.changed, w.seed);
                    for (int e = 0; e < 2; e++) {   // the splashes where it left the water and where it comes down
                        const double ue = e ? u - 1.0 + 0.15 : u, sh = 1 - std::fabs(ue) / 0.15;
                        if (sh <= 0) continue;
                        const double ex = sx + hx * (e ? 0.5 : -0.5) * len, ez = sz + hz * (e ? 0.5 : -0.5) * len;
                        for (int q = 0; q < 10; q++) {
                            const double qa = q * TAU / 10, qr = (0.3 + 0.5 * (1 - sh)) * sw->body.length;
                            RVert pt = vert(Vec3(ex + std::cos(qa) * qr, wl + 0.05 + 0.25 * sh * sw->body.length * hash01(mix64(w.seed + q)), ez + std::sin(qa) * qr), 40 + 15 * sh);
                            if (pt.z >= NEAR_Z) rasterPoint3(fb, pt, 8, proj, true, 1);
                        }
                    }
                    continue;
                }
                const double depth = sw->body.depth * (sw->way == WAY_SHALLOWS ? 0.3 : 0.25 + 0.6 * (1 - surf)) - breach * sw->body.depth * 1.6;   // J-07: a back up at the top
                LifeJob& J = job(w.species, Vec3(sx, wl - depth, sz), std::atan2(hx, hz), w.sizeK, dist, false, DO_SWIMMING);
                J.in.afloat = true; J.in.gait = GAIT_SWIM; J.in.seed = w.seed;
                fateLook(J, w.species, w.sick, w.changed, w.seed);
                continue;
            }
            if (leaping) continue;   // far off a leap is a splash too small to draw
            double fh = (0.25 + 0.45 * (surf - 0.2)) * ssz * 0.6;
            double shade = 8 + 55 * fogOf(dist);
            RasterParams rp; rp.bank = sw ? hideBanks[sw->body.bank & 1] : 3;
            RVert p[3] = {vert(Vec3(sx - hx * 0.4 * ssz, wl + 0.02, sz - hz * 0.4 * ssz), shade), vert(Vec3(sx, wl + fh, sz), shade),
                          vert(Vec3(sx + hx * 0.4 * ssz, wl + 0.02, sz + hz * 0.4 * ssz), shade)};
            if (p[0].z < NEAR_Z || p[1].z < NEAR_Z || p[2].z < NEAR_Z) continue;
            rasterPolygon(fb, p, 3, rp, proj);
            // the back: a low arc behind the fin; a breach lifts it high for a moment
            double bh = (0.12 + 0.6 * breach) * ssz;
            RVert q[4] = {vert(Vec3(sx - hx * 1.6 * ssz, wl + 0.02, sz - hz * 1.6 * ssz), shade), vert(Vec3(sx - hx * 1.0 * ssz, wl + bh, sz - hz * 1.0 * ssz), shade + 6),
                          vert(Vec3(sx - hx * 0.45 * ssz, wl + bh * 0.8, sz - hz * 0.45 * ssz), shade + 6), vert(Vec3(sx - hx * 0.4 * ssz, wl + 0.02, sz - hz * 0.4 * ssz), shade)};
            bool ok = true; for (RVert& v : q) if (v.z < NEAR_Z) ok = false;
            if (ok) rasterPolygon(fb, q, 4, rp, proj);
        }
    }
    // ---- land animals: each posed from its species' rig on the ground under it, its shadow first (J-07: a climber on its trunk, a burrower's
    // hole while it is under)
    for (const Critter& c : critters) {
        const Species* spp = speciesAt(c.species);
        if (!spp) continue;
        const Species& sp = *spp;
        double dist = std::sqrt((c.x - camPos.x) * (c.x - camPos.x) + (c.z - camPos.z) * (c.z - camPos.z));
        if (dist > std::min(600.0, 250.0 + 120.0 * c.size)) continue;   // B-316: the big ones show from farther
        double sz = c.size, k = c.size / std::max(sp.size, 0.05);   // this animal's scale of its species
        if (sz / dist * proj.f < 1.0) continue;
        if (c.state == CS_UNDER && dist < 80) {   // the burrow's mouth, dark on the ground
            const double hr = std::max(0.1, 0.45 * sp.body.width * k + 0.05), hg = site.groundHeight(c.homeX, c.homeZ) + 0.03;
            RVert q[8];
            bool ok = true;
            for (int e = 0; e < 8; e++) {
                const double qa = -e * TAU / 8;
                q[e] = vert(Vec3(c.homeX + std::cos(qa) * hr, hg, c.homeZ + std::sin(qa) * hr * 0.8), 4 + 20 * fogOf(dist));
                if (q[e].z < NEAR_Z) ok = false;
            }
            RasterParams rp; rp.bank = 30;
            if (ok) rasterPolygon(fb, q, 8, rp, proj);
        }
        if (c.state == CS_UNDER && c.sink >= 0.98) continue;   // out of sight
        const bool onTrunk = c.state == CS_CLIMB && c.climbY > 0;
        double gy = site.groundHeight(c.x, c.z);
        if (!onTrunk) drawBlobShadow(fb, c.x, c.z, 0.5 * sp.bodyLen * k * (1 - c.sink), sz);
        LifeJob& J = onTrunk ? job(c.species, Vec3(c.x, c.treeGy + c.climbY, c.z), c.heading, k, dist, false, c.state)
                             : job(c.species, Vec3(c.x, gy, c.z), c.heading, k, dist, true, c.state);
        J.in.phase = c.gaitPhase;
        J.in.moving = (c.speed > 0.05 && c.lift < 0.5) || (onTrunk && c.climbY < c.treeTop - 0.01);
        J.in.running = c.state == CS_FLEE || c.state == CS_CHASE || c.state == CS_CHARGE;
        J.in.gait = c.gait; J.in.stride = c.stride;
        J.in.lift = c.lift; J.in.headPitch = c.headPitch; J.in.seed = c.seed;
        J.in.reach = c.reach; J.in.crouch = c.crouch; J.in.sink = c.sink;
        if (sp.way == WAY_CLIMBER) J.in.tiltMax = 1.1;
        fateLook(J, c.species, c.sick, c.changed, c.seed);
        if (onTrunk) climbPose(c, J.in);   // (B-468: its feet on the bark, round the trunk's girth where it clings)
        // tracks in sand and snow behind it
        if (dist < 80 && !c.tracks.empty()) {
            const double fog = fogOf(dist);
            for (const Critter::Track& tr : c.tracks) {
                RVert q = vert(Vec3(tr.x, site.groundHeight(tr.x, tr.z) + 0.03, tr.z), 10 + 40 * fog);
                if (q.z >= NEAR_Z) rasterPoint3(fb, q, 0, proj, true, 1);
            }
        }
    }
    // ---- J-08: remains on the ground: posed dead from their species' rig, a dark stain under the fresh, flies over them; the record's marks
    // (dying, or gone where none of its kind lives there now; old bones mark nothing)
    for (const Remains& r : remains) {
        const Species* spp = speciesAt(r.species);
        if (!spp) continue;
        const double dist = std::sqrt((r.x - camPos.x) * (r.x - camPos.x) + (r.z - camPos.z) * (r.z - camPos.z));
        if (dist > 160 || r.size / std::max(dist, 0.5) * proj.f < 1.0) continue;
        const double k = r.size / std::max(spp->size, 0.05);
        const int stage = r.kill && r.eaten > 45 ? std::max(r.stage, (int)RS_SUNKEN) : r.stage;
        const double gy = site.groundHeight(r.x, r.z);
        // J-09: on an overgrowth's cover it took them in: their shape under it, in its bank, with no stain and no flies
        const bool takenIn = growthBank >= 0 && madeGrowthMaterial(site.lod0.at((int)std::floor(r.x / 16), (int)std::floor(r.z / 16)).material);
        if (stage <= RS_BLOATED && dist < 90 && dist > 1.5 && !takenIn) {   // the stain of what ran out of it
            const double hr = (0.3 * spp->body.length + 0.25 * spp->body.width) * k + 0.05, fog = fogOf(dist);
            RVert q[8];
            bool ok = true;
            for (int e = 0; e < 8; e++) {
                const double qa = -e * TAU / 8 + r.heading, qr = hr * (0.8 + 0.4 * hash01(mix64(r.seed + e)));
                q[e] = vert(Vec3(r.x + std::sin(qa) * qr, site.groundHeight(r.x + std::sin(qa) * qr, r.z + std::cos(qa) * qr) + 0.02, r.z + std::cos(qa) * qr), 12 + 30 * fog);
                if (q[e].z < NEAR_Z) ok = false;
            }
            RasterParams rp; rp.bank = 31;   // (dried blood's brown)
            if (ok) rasterPolygon(fb, q, 8, rp, proj);
        }
        LifeJob& J = job(r.species, Vec3(r.x, gy, r.z), r.heading, k, dist, true, DO_DEAD, true);
        J.dead = stage; J.in.dead = stage; J.in.seed = r.seed; J.seed = r.seed;
        if (r.stalks) J.sickness = MF_SPROUTING;
        J.eyeshine = false;
        if (!r.old) J.marks = fateAt(r.species, r.x, r.z).gone() ? AM_GONE : AM_DYING;
        if (takenIn) J.over = growthBankOf(site.lod0.at((int)std::floor(r.x / 16), (int)std::floor(r.z / 16)).mline - 1);   // (KI-473: its cover's line's)
        if (stage <= RS_BLOATED && dist < 30 && !takenIn) {   // flies circling over it, as many and as high as it is big
            const double big = std::max(0.2, std::min(2.0, r.size));
            for (int q = 0; q < (big < 0.5 ? 4 : 9); q++) {
                const double ph = hash01(mix64(r.seed + 0xF1E5ULL + q)) * TAU, rr = (0.3 + 0.5 * hash01(mix64(r.seed + 0xF1E6ULL + q))) * (0.4 * spp->body.length * k + 0.15 * big);
                const double a = ph + t * (2.5 + 2 * hash01(mix64(r.seed + 0xF1E7ULL + q)));
                RVert p = vert(Vec3(r.x + std::cos(a) * rr, gy + (0.15 + 0.45 * hash01(mix64(r.seed + 0xF1E8ULL + q))) * big + 0.08 * big * std::sin(t * 7 + ph), r.z + std::sin(a) * rr), 8);
                if (p.z >= NEAR_Z) rasterPoint3(fb, p, 30, proj, true, 1);
            }
        }
    }
    for (const Specimen& s : specimens) {   // the harness's
        const Species* sp = speciesAt(s.species);
        if (!sp) continue;
        const double dist = length(s.at - camPos);
        Vec3 at = s.at;
        if (s.how == 0) { at.y = site.groundHeight(at.x, at.z); drawBlobShadow(fb, at.x, at.z, 0.5 * sp->bodyLen, sp->size); }
        LifeJob& J = job(s.species, at, s.heading, s.scale, dist, s.how == 0, -1);
        J.in.aloft = s.how == 1; J.in.afloat = s.how == 2; J.in.flap = s.flap; J.in.gait = sp->gait; J.in.seed = (uint64_t)s.species + 1;
        J.eyeshine = false;
        if (s.gait >= 0) { J.in.moving = true; J.in.gait = s.gait; J.in.phase = s.phase; J.in.stride = s.stride; J.in.running = s.running; }   // J-07
        if (s.dead || s.sickness || s.changed) {   // J-08
            J.dead = J.in.dead = s.dead; J.sickness = s.sickness; J.seed = J.in.seed;
            J.changed = s.changed && s.species < (int)changedRigs.size();
            if (J.changed) J.looks = speciesChanges[s.species].drawn;
            J.in.girth = s.sickness == MF_WASTING ? 0.75 : (s.sickness == MF_BLOATED ? 1.35 : 1.0);
            J.in.sag = s.sickness == MF_MELTING ? 1.0 : 0.0;
            if (s.dead) {   // lying: the ground wider round it
                J.in.gz = std::max(J.in.gz, (sp->body.leg + 0.6 * sp->body.width + 0.5 * sp->body.span));
                J.in.gx = std::max(J.in.gx, 0.5 * sp->body.length + 0.6 * sp->body.neck + 0.3 * sp->body.tailLen);
                for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) J.in.ground[i][j] = site.groundHeight(at.x + std::sin(s.heading) * (i - 1) * J.in.gx - std::cos(s.heading) * (j - 1) * J.in.gz, at.z + std::cos(s.heading) * (i - 1) * J.in.gx + std::sin(s.heading) * (j - 1) * J.in.gz);
                J.in.at.y = site.groundHeight(at.x, at.z);
            }
        }
    }
    // ---- J-06: the animals posed and their shapes made on the pool, a list each; then filled in bands, in their order
    const int nj = (int)lifeJobs.size();
    if ((int)lifeLists.size() < nj) lifeLists.resize(nj);
    struct LifeView final : AnimalView {
        const SurfaceView* sv = nullptr;
        Vec3 sun; double ambient = 0, sunUp = 0, lf = 0;
        double pull = 0;   // J-08: remains drawn a few centimetres toward the eye, so the ground they lie on never hides them
        Vec3 view(const Vec3& p) const override {
            if (pull > 0) { const Vec3 q = p + normalize(eye - p) * pull; return sv->toView(q.x, q.y, q.z); }
            return sv->toView(p.x, p.y, p.z);
        }
        double light(const Vec3& p, const Vec3& n) const override {
            double l = ambient + (1 - ambient) * std::max(0.0, dot(n, sun)) * sunUp * lf;
            if (sv->nLamps) l += (1 - ambient) * sv->lampAt(p, &n);   // the explorer's lamp
            return 52 * std::pow(std::max(0.0, l), 0.65);   // a sunlit flank at its hide's colour (the ramp's 47) where its tone is 0.9
        }
    };
    parallelFor(nj, 1, [&](int jb, int je) {
        for (int j = jb; j < je; j++) {
            const LifeJob& J = lifeJobs[j];
            ShapeList& L = lifeLists[j];
            L.begin(proj);
            AnimalPose P;
            const AnimalRig& R = J.changed ? changedRigs[J.rig] : rigs[J.rig];   // J-08: a changed young's own
            poseAnimal(R, J.in, P);
            LifeView V;
            V.sv = this; V.sun = sd; V.ambient = ambient; V.sunUp = sunUp; V.lf = lf;
            V.fog = J.fog; V.eye = camPos; V.hide[0] = J.over >= 0 ? J.over : hideBanks[0]; V.hide[1] = J.over >= 0 ? J.over : hideBanks[1];
            V.pxPerM = J.pxPerM; V.fbScale = FB_SCALE; V.eyeshine = J.eyeshine;
            V.dead = J.dead; V.sickness = J.sickness; V.marks = J.looks; V.seed = J.seed; V.fate = fateBank; V.night = dark;   // J-08
            if (J.dead) V.pull = 0.06 + 0.04 * std::min(2.0, bestiary.species[J.rig].size * J.in.scale);
            emitAnimal(R, P, V, J.detail, L);
        }
    });
    if (nj) parallelFor(FBH, 32, [&](int yb, int ye) {
        setRasterBand(yb, ye);
        for (int j = 0; j < nj; j++) lifeLists[j].fill(fb);
        setRasterBand(0, 1 << 30);
    });
    // J-07: what was drawn whole goes to the record (the first sightings, what each was doing) and to the crosshair
    for (const LifeJob& J : lifeJobs) {
        if (J.doing < 0 || J.detail < 1) continue;
        const Species& sp = bestiary.species[J.rig];
        const double mid = J.in.aloft || J.in.afloat || J.in.climbing ? 0 : 0.6 * sp.size * J.in.scale;
        double sx, sy;
        if (!projectPoint(J.in.at.x, J.in.at.y + mid, J.in.at.z, sx, sy)) continue;
        lifeDrawn.push_back({J.rig, J.doing, J.dist, sx, sy, 0.6 * std::max(sp.size, 0.5 * sp.body.length) * J.in.scale * J.pxPerM, J.marks});
    }
    lifeSaw(lifeDrawn);
    lastLifeMs[1] = nowMs() - t0;
}

// J-07: the record of a frame's animals drawn whole: each species seen within its reach (land and water 60 m, the air 100 m) on the screen,
// what it was doing, the nearest and the most at once; a species' first of the landing is a sighting. The crosshair's: the nearest the middle
// of the screen within its size (12 pixels at least) and within naming reach (land 40 m, water 50 m, the air 80 m)
void SurfaceView::lifeSaw(const std::vector<LifeDrawn>& drawn) {
    if (lifeSeen.size() != bestiary.species.size()) lifeSeen.assign(bestiary.species.size(), SeenAnimal());
    lifeTarget = LifeTarget();
    double best = 1e18;
    std::vector<int> count(bestiary.species.size(), 0);
    for (const LifeDrawn& d : drawn) {
        const Species& sp = bestiary.species[d.species];
        const bool air = planFlies(sp.plan), water = planSwims(sp.plan);
        if (d.sx < 0 || d.sy < 0 || d.sx >= FBW || d.sy >= FBH || d.dist > (air ? 100.0 : 60.0)) continue;
        SeenAnimal& s = lifeSeen[d.species];
        s.doing |= 1u << clampi(d.doing, 0, DO_COUNT - 1);
        s.nearest = std::min(s.nearest, d.dist);
        if (d.doing != DO_DEAD) s.most = std::max(s.most, ++count[d.species]);   // (J-08: the most living at once)
        s.marks |= (uint32_t)d.marks;
        s.fresh = true;
        if (seenSpecies.insert(d.species).second) sightings.push_back({d.species, d.doing, d.dist});
        const double ex = d.sx - FBW * 0.5, ey = d.sy - FBH * 0.5, e2 = ex * ex + ey * ey, rr = std::max(d.r, 12.0 * FB_SCALE);
        if (e2 < rr * rr && e2 < best && d.dist <= (air ? 80.0 : (water ? 50.0 : 40.0))) { best = e2; lifeTarget.species = d.species; lifeTarget.dist = d.dist; }
    }
}

// tests: stand `dist` metres south of herd 0, facing it
bool SurfaceView::testGotoHerd(double dist) {
    double cx = 0, cz = 0; int n = 0;
    for (const Critter& c : critters) if (c.herd == 0) { cx += c.x; cz += c.z; n++; }
    if (!n) return false;
    cx /= n; cz /= n;
    player.x = cx; player.z = cz - dist; player.y = site.surfaceHeight(player.x, player.z);
    player.yaw = 0; player.pitch = -0.03;
    return true;
}

std::string SurfaceView::testHerdStates() const {
    int counts[DO_COUNT] = {0};   // (J-08: the land's states and feeding)
    for (const Critter& c : critters) counts[clampi(c.state, 0, DO_COUNT - 1)]++;
    std::string s;
    for (int i = 0; i < DO_COUNT; i++) if (counts[i]) s += std::string(s.empty() ? "" : " ") + DOING_NAMES[i] + " " + std::to_string(counts[i]);
    return s + " (calls " + std::to_string(callsFired) + ")";
}
