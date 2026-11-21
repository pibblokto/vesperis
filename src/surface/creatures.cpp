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
                                           "PERCHED", "SWARMING", "SOARING", "DRIFTING", "SWIMMING", "LEAPING"};

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

void SurfaceView::planLife(const SurfaceSite& s, const Bestiary& B, uint64_t seed, std::vector<Critter>& critters, std::vector<Flock>& flocks, std::vector<Herd>& herds) {
    critters.clear(); flocks.clear(); herds.clear();
    const Body& b = s.sys->bodies[s.body];
    if (!kindHas(b.type, KT_FAUNA)) return;
    Rng r(seed);
    int biome = s.sampleAt(0, 0, 16).biome;
    // flocks of the world's flyers
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
        flocks.push_back(f);
    }
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
        for (int i = 0; i < n; i++) {
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
            critters.push_back(c);
        }
    }
}

void SurfaceView::spawnLife(double lat, double lon) {
    const Body& b = site.sys->bodies[site.body];
    bestiary = Bestiary::make(b, site.gen);
    rigs.clear();
    for (const Species& sp : bestiary.species) rigs.push_back(rigOf(sp.body));   // J-06: built once a species
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
    if (!kindHas(site.gen.type, KT_FLORA) || !bestiary.landCount) return;
    if (lastT - habitatT < 0.5 && habitatT > -1e8) return;
    habitatT = lastT;
    double px = inBuggy ? buggy.x : player.x, pz = inBuggy ? buggy.z : player.z;
    // forget the cell herds that fell behind
    for (size_t h = 0; h < herds.size();) {
        Herd& hd = herds[h];
        double dx = hd.cx - px, dz = hd.cz - pz;
        if (!hd.origin && dx * dx + dz * dz > 1300.0 * 1300.0) {
            critters.erase(std::remove_if(critters.begin(), critters.end(), [&](const Critter& c) { return c.herd == (int)h; }), critters.end());
            for (Critter& c : critters) if (c.herd > (int)h) c.herd--;
            herds.erase(herds.begin() + h);
        } else h++;
    }
    // spawn the herds of the cells that came within reach
    int gLat0, gLon0;
    lifeCellAt(site, px, pz, gLat0, gLon0);
    for (int dl = -2; dl <= 2; dl++)
        for (int dn = -3; dn <= 3; dn++) {
            int gLat = gLat0 + dl, gLon = gLon0 + dn;
            bool have = false;
            for (const Herd& hd : herds) if (!hd.origin && hd.gLat == gLat && hd.gLon == gLon) { have = true; break; }
            if (have) continue;
            Herd hd; int spi, n;
            if (!planCellHerd(site, bestiary, gLat, gLon, hd, spi, n)) continue;
            double dx = hd.cx - px, dz = hd.cz - pz;
            if (dx * dx + dz * dz > 900.0 * 900.0) continue;
            herds.push_back(hd);
            const Species& sp = bestiary.species[spi];
            uint64_t hs = hash2i(gLat, gLon, site.gen.seed ^ 0x4E4EULL);
            Rng r(hs);
            double spread = 4 + 2.0 * n;
            for (int i = 0; i < n; i++) {
                Critter c;
                c.herd = (int)herds.size() - 1; c.species = spi;
                c.x = hd.cx + r.sym(spread); c.z = hd.cz + r.sym(spread);
                c.heading = r.range(0, TAU); c.speed = 0;
                c.timer = r.range(1, 6); c.size = sp.size * (0.85 + 0.3 * r.uni()); c.seed = r.next();
                c.kind = sp.plan == PLAN_HEXAPOD || sp.plan == PLAN_CRAWLER || sp.plan == PLAN_OCTOPOD ? 1 : 0;
                c.state = CS_IDLE;
                TerrainVertex tv = site.sampleAt(c.x, c.z, 16);
                if (tv.material == MAT_WATER || (tv.water > -1e8f && tv.h < tv.water + 0.5)) { c.x = hd.cx; c.z = hd.cz; }
                c.homeX = c.x; c.homeZ = c.z;   // J-07: a territorial's ground
                critters.push_back(c);
            }
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
        double dpx = player.x - c.x, dpz = player.z - c.z, dp = std::sqrt(dpx * dpx + dpz * dpz);
        // threat: the explorer within 25 m (shy; curious 15 m), the moving buggy within 60 m; giants ignore everything
        double threatR = sp.temperament == TEMP_INDIFFERENT ? 0 : (sp.temperament == TEMP_CURIOUS ? 15 : 25);
        const bool rolling = inBuggy && std::fabs(buggy.speed) > 3;
        if (rolling && threatR > 0) threatR = 60;
        const bool territorial = sp.social == SOCIAL_TERRITORIAL && !rolling;
        if (territorial) threatR = 22;   // J-07: it holds its ground against one on foot
        if ((sp.way == WAY_BURROWER || sp.way == WAY_CLIMBER) && !rolling) threatR = std::max(threatR, 18.0);   // J-07: they hide from anyone
        bool threatened = dp < threatR;
        c.calmT = threatened ? 0 : c.calmT + dt;
        const bool restWanted = (sp.activity == ACT_DAY && ((localTime > 0.46 && localTime < 0.54) || sky < 0.2)) || (sp.activity == ACT_NIGHT && sky > 0.45) ||
                                (sp.activity == ACT_DUSK && (sky > 0.7 || sky < 0.05));
        auto underGround = [&](double secs) { c.state = CS_UNDER; c.timer = secs; c.speed = 0; c.homeX = c.x; c.homeZ = c.z; c.lift = 0; };
        auto upATree = [&](double secs) {   // to the nearest trunk within 25 m and up it, false when there is none
            TreeInst T;
            if (!treeNear(c.x, c.z, 25, T)) return false;
            c.state = CS_CLIMB; c.timer = secs; c.climbY = 0; c.lift = 0;
            c.treeX = T.x; c.treeZ = T.z; c.treeGy = T.gy; c.treeR = 0.12 + T.h * 0.02;
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
                if (c.climbY <= 0) {   // at the trunk: onto it, near the side it came from (each its own way round and its own height)
                    const double a = std::atan2(td > 1e-6 ? -tdx : 0, td > 1e-6 ? -tdz : 1) + 1.6 * (hash01(c.seed ^ 0xC11BULL) - 0.5);
                    c.x = c.treeX + std::sin(a) * c.treeR; c.z = c.treeZ + std::cos(a) * c.treeR;
                    c.treeTop *= 0.55 + 0.45 * hash01(c.seed ^ 0xC11CULL);
                    c.climbY = 0.01;
                }
                c.speed = 0;
                if (threatened) c.timer = std::max(c.timer, 6.0);
                const bool down = c.timer <= 0 && c.calmT > 8;
                const double rate = 0.5 * sp.walkSpeed;
                const double y0 = c.climbY;
                c.climbY = clampd(c.climbY + (down ? -rate : rate) * dt, 0, c.treeTop);
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
        // J-07: a quarry's flight: a hunter of its kind chasing within 40 m or stalking within 12 m (its herd mates startle too)
        if (c.state != CS_FLEE && c.state != CS_DISPLAY && c.state != CS_CHARGE && c.state != CS_CLIMB)
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
                if (qd < 1.0 + 0.5 * (c.size + q->size)) q->heading += hash01(q->seed ^ (uint64_t)(t * 7)) < 0.5 ? 1.7 : -1.7;
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
        if (c.timer <= 0 && c.state != CS_DISPLAY && c.state != CS_CHARGE && c.state != CS_STALK && c.state != CS_CHASE && c.state != CS_CLIMB) {
            Rng r(c.seed ^ (uint64_t)(t * 10));
            bool leader = true;
            for (size_t j = 0; j < i; j++) if (critters[j].herd == c.herd) { leader = false; break; }
            const Critter* lead = nullptr;
            if (!leader) for (size_t j = 0; j < i; j++) if (critters[j].herd == c.herd) { lead = &critters[j]; break; }
            if (c.state == CS_ALERT) {
                // decide: flee, or come closer when curious; the herd mates within 40 m startle too (stampede)
                if (sp.temperament == TEMP_CURIOUS && dp > 10 && !(inBuggy && std::fabs(buggy.speed) > 3)) {
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
                TreeInst T;
                bool chosen = true;
                if (restWanted && roll < 0.8 && sp.way == WAY_BURROWER) underGround(20 + 20 * w.uni());
                else if (restWanted && roll < 0.8 && sp.way == WAY_CLIMBER && upATree(20 + 20 * w.uni())) {}
                else if (sp.way == WAY_HUNTER && lead && (lead->state == CS_STALK || lead->state == CS_CHASE)) {
                    c.state = lead->state; c.quarry = lead->quarry; c.timer = lead->timer;
                } else if (sp.way == WAY_HUNTER && leader && !restWanted && c.hungerT <= 0 && sp.prey >= 0) {
                    const Critter* best = nullptr; double bd = 150.0 * 150.0;
                    for (const Critter& o : critters) {
                        if (o.species != sp.prey || o.state == CS_UNDER || o.climbY > 0) continue;
                        const double d2 = (o.x - c.x) * (o.x - c.x) + (o.z - c.z) * (o.z - c.z);
                        if (d2 < bd) { bd = d2; best = &o; }
                    }
                    if (best) { c.state = CS_STALK; c.quarry = best->seed; c.timer = 30 + 10 * w.uni(); c.speed = 0.6 * sp.walkSpeed + 0.1 * sp.runSpeed; }
                    else { c.hungerT = 20 + 20 * w.uni(); chosen = false; }
                } else if (hunter) { c.state = CS_TRAIL; c.quarry = hunter->seed; c.timer = 6 + 4 * w.uni(); }
                else if (sp.way == WAY_BROWSER && roll < 0.4 && !restWanted && treeNear(c.x, c.z, 30, T)) {
                    c.state = CS_BROWSE; c.timer = 10 + 8 * w.uni(); c.treeX = T.x; c.treeZ = T.z; c.treeR = 0.12 + T.h * 0.02; c.speed = sp.walkSpeed;
                } else chosen = false;
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
                            if (ld > 12) c.heading = std::atan2(lx, lz) + r.sym(0.3);
                            else if (ld < 3) c.heading = std::atan2(-lx, -lz) + r.sym(0.5);
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
        double wantHead = (c.state == CS_GRAZE || c.state == CS_DRINK) ? 1.0 : (c.state == CS_TRAIL ? 0.55 : 0.0), wantLift = c.state == CS_REST ? 1.0 : 0.0;
        c.headPitch += (wantHead - c.headPitch) * (1 - std::exp(-dt * 1.5));
        c.lift += (wantLift - c.lift) * (1 - std::exp(-dt * 0.8));
        c.reach += ((atTree ? 1.0 : 0.0) - c.reach) * (1 - std::exp(-dt * 1.2));
        c.crouch += ((c.state == CS_STALK ? 1.0 : 0.0) - c.crouch) * (1 - std::exp(-dt * 2.0));
        if (c.state == CS_GRAZE && std::fmod(t + c.seed % 13, 7.0) < 1.5) c.headPitch += (0 - c.headPitch) * (1 - std::exp(-dt * 3));   // looks up now and then
        // movement, feet on the ground, no water, no steep slopes (J-07: a climber takes them)
        const bool running = c.state == CS_FLEE || c.state == CS_CHASE || c.state == CS_CHARGE || (c.state == CS_CLIMB && c.climbY <= 0);
        c.gait = speciesGait(sp, k, c.speed, g, running);
        c.stride = speciesStride(sp, k, c.speed, g);
        if (c.speed > 0 && c.lift < 0.5) {
            double nx = c.x + std::sin(c.heading) * c.speed * dt, nz = c.z + std::cos(c.heading) * c.speed * dt;
            double gy = site.groundHeight(nx, nz);
            double slope = std::fabs(gy - site.groundHeight(c.x, c.z));
            bool wet = gy < site.waterNowAt(nx, nz) + 0.3;   // Q-02: the tide's
            const double steep = (0.6 * c.speed * dt + 0.3) * (sp.way == WAY_CLIMBER ? 3.0 : 1.0);
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
                if (dp < 20) hoof += std::min(1.0, c.size) * (c.speed / std::max(sp.runSpeed, 0.1)) * 0.5;
            }
            { double hx = herds[c.herd].cx - c.x, hz = herds[c.herd].cz - c.z; if (hx * hx + hz * hz > 300.0 * 300.0) c.heading = std::atan2(hx, hz); }   // a herd stays within 300 m of its spot
        }
    }
    hoofLevel = std::min(1.0, hoof);
    lastLifeMs[0] = nowMs() - t0;
}

// the world's swimmers within two 100 m cells of a point at a time, those at the surface or in a leap: each cell's spot (a hash's third of
// them) holds one of its swimmer species circling, where the water is deep enough for it (J-07: a shoal's three in line; one of the shallows
// round the first of eight places of its cell where the water is 0.3 to 1.5 m deep, its back always up; a leaper under 2 m of water at
// least, out in an arc every 7 to 13 s)
void SurfaceView::swimmersNear(double px, double pz, double t, std::vector<SwimmerAt>& out) {
    out.clear();
    if (!site.hasWater || !kindHas(site.gen.type, KT_FAUNA) || bestiary.swimmerCount <= 0) return;
    const int ccx = (int)std::floor(px / 100), ccz = (int)std::floor(pz / 100);
    for (int dz = -2; dz <= 2; dz++)
        for (int dx = -2; dx <= 2; dx++) {
            uint64_t hs = hash2i(ccx + dx, ccz + dz, site.gen.seed ^ 0xF1A);
            if (hash01(hs) > 0.35) continue;
            const int spi = bestiary.firstSwimmer() + (int)(hash01(mix64(hs + 6)) * bestiary.swimmerCount) % bestiary.swimmerCount;
            const Species& sw = bestiary.species[spi];
            const int way = sw.way, members = way == WAY_SHOAL ? 3 : 1;
            double cx = (ccx + dx) * 100.0 + 20 + hash01(mix64(hs + 1)) * 60, cz = (ccz + dz) * 100.0 + 20 + hash01(mix64(hs + 2)) * 60;
            if (way == WAY_SHALLOWS) {   // the first of eight places of its cell where the water is 0.3 to 1.5 m deep, else none
                bool found = false;
                for (int k = 0; k < 8 && !found; k++) {
                    const double qx = (ccx + dx) * 100.0 + 5 + hash01(mix64(hs + 31 + 2 * k)) * 90, qz = (ccz + dz) * 100.0 + 5 + hash01(mix64(hs + 32 + 2 * k)) * 90;
                    const double wl = site.waterNowAt(qx, qz), dm = wl - site.groundHeight(qx, qz);
                    if (wl > -1e8 && dm > 0.3 && dm < 1.5) { cx = qx; cz = qz; found = true; }
                }
                if (!found) continue;
            }
            for (int m = 0; m < members; m++) {
                const uint64_t hm = m ? mix64(hs + 101 * m) : hs;
                const double circ = way == WAY_SHALLOWS ? 5 : 25, rate = way == WAY_SHALLOWS ? 1.5 : 1.0;
                const double a = t * (0.03 + 0.03 * hash01(mix64(hs + 3))) * rate + hash01(mix64(hs + 4)) * TAU - m * (2.5 + sw.size) / circ;   // a shoal's in line
                SwimmerAt w;
                w.species = spi; w.seed = hm;
                w.x = cx + std::cos(a) * (circ + 2.0 * m); w.z = cz + std::sin(a) * (circ + 2.0 * m);
                w.water = site.waterNowAt(w.x, w.z);
                if (w.water < -1e8) continue;
                const double depthM = w.water - site.groundHeight(w.x, w.z);
                if (way == WAY_SHALLOWS ? (depthM < 0.25 || depthM > 1.8) : depthM < (way == WAY_LEAPER ? 2.0 : 1.5)) continue;
                w.surf = std::sin(t * 0.7 + hash01(mix64(hm + 5)) * TAU);   // above the water only part of the time
                if (way == WAY_SHALLOWS) w.surf = 0.6 + 0.4 * w.surf;
                w.hx = -std::sin(a); w.hz = std::cos(a);   // the circle's tangent
                w.heading = std::atan2(w.hx, w.hz);
                if (way == WAY_LEAPER) {   // its leap: every 7 to 13 s an arc out of the water of a second or so
                    const double period = 7 + 6 * hash01(mix64(hm + 7)), span = 1.0 + 0.5 * hash01(mix64(hm + 8));
                    w.leap = std::fmod(t + period * hash01(mix64(hm + 9)), period) / span;
                }
                if (w.leap < 1.0 || w.surf >= (way == WAY_LEAPER ? 0.6 : 0.2)) out.push_back(w);
            }
        }
}

void SurfaceView::drawLife(Framebuffer& fb, double t) {
    double t0 = nowMs();
    lifeDrawn.clear();
    const bool swimmersHere = site.hasWater && kindHas(site.gen.type, KT_FAUNA) && bestiary.swimmerCount > 0;   // J-07: with no herd or flock near too
    if (flocks.empty() && critters.empty() && specimens.empty() && !swimmersHere) { lifeTarget = LifeTarget(); lastLifeMs[1] = 0; return; }
    const Vec3& sd = env.sun.dirLocal;
    const double sunUp = smoothstep(-0.03, 0.06, sd.y);
    const double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    const double lf = env.sun.lightFactor;
    const bool dark = env.skyBrightness < 0.25;
    auto vert = [&](const Vec3& p, double shade) { RVert q; Vec3 v = toView(p.x, p.y, p.z); q.x = v.x; q.y = v.y; q.z = v.z; q.shade = shade; return q; };
    auto fogOf = [&](double dist) { return 1 - std::exp(-dist / env.fogDistance); };
    auto speciesAt = [&](int i) { return i >= 0 && i < (int)bestiary.species.size() && i < (int)rigs.size() ? &bestiary.species[i] : nullptr; };
    lifeJobs.clear();
    // J-06: an animal to pose: its ground (3 by 3 heights round it, read here on the main thread: a miss fills the cache), its detail; J-07: what
    // it is doing (the record's; -1 the harness's)
    auto job = [&](int spi, const Vec3& at, double heading, double scale, double dist, bool ground, int doing) -> LifeJob& {
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
    // members darting; a soarer mostly gliding)
    for (const Flock& f : flocks) {
        const Species* sp = speciesAt(f.species);
        double bsize = sp ? sp->size : 0.5;
        int bank = sp ? hideBanks[sp->body.bank & 1] : 1;
        const int way = sp ? sp->way : WAY_FLOCK;
        const bool swarm = way == WAY_SWARM;
        double a = f.phase + t * f.speed;
        double fcx = f.cx + std::cos(a) * f.radius, fcz = f.cz + std::sin(a) * f.radius;
        double ground = site.groundHeight(fcx, fcz);
        double base = std::max(ground, site.waterAt(fcx, fcz)) + f.alt;
        double dist = std::sqrt((fcx - camPos.x) * (fcx - camPos.x) + (fcz - camPos.z) * (fcz - camPos.z));
        if (dist > 900) continue;
        Rng r(f.seed);
        bool down = f.landed > 0 && f.alt < 3;
        const double course = std::atan2(-std::sin(a) * (f.speed >= 0 ? 1 : -1), std::cos(a) * (f.speed >= 0 ? 1 : -1));
        const int doing = sp && sp->plan == PLAN_FLOATER ? DO_DRIFTING : (down ? DO_PERCHED : (swarm ? DO_SWARMING : (way == WAY_SOARER ? DO_SOARING : DO_FLYING)));
        for (int i = 0; i < f.n; i++) {
            double ox = r.sym(18), oz = r.sym(18), oy = r.sym(6);
            double ph = r.range(0, TAU), wf = 6 + 4 * r.uni();
            double bx, by, bz;
            if (down && f.perched) { double pa = ph, pr = f.perchR * 0.8 * (0.3 + 0.7 * r.uni()); bx = f.perchX + std::cos(pa) * pr; bz = f.perchZ + std::sin(pa) * pr; by = f.perchY + r.sym(0.4); }
            else if (swarm) {   // a cloud a few metres across, each member darting about its place
                const double cr = 0.25 * (2 + f.radius);
                bx = fcx + ox / 18 * cr + 0.5 * std::sin(t * (3.1 + wf * 0.3) + ph); bz = fcz + oz / 18 * cr + 0.5 * std::cos(t * (2.7 + wf * 0.25) + ph);
                by = (down ? ground + 0.15 + 0.1 * std::fabs(oy) / 6 : base + oy / 6 * 0.8 + 0.3 * std::sin(t * 4.3 + ph));
                wf = 30;
            } else { bx = fcx + ox + 3 * std::sin(t * 0.7 + ph); bz = fcz + oz + 3 * std::cos(t * 0.5 + ph); by = base + oy + 1.5 * std::sin(t * 0.9 + ph); }
            const double md = std::sqrt((bx - camPos.x) * (bx - camPos.x) + (bz - camPos.z) * (bz - camPos.z) + (by - camPos.y) * (by - camPos.y));
            const bool glide = !down && std::sin(t * 0.35 + ph) > (way == WAY_SOARER ? -0.7 : 0.45);   // long glides between flapping (a soarer's most of the time)
            if (sp && md < 160 && bsize * proj.f / std::max(md, 0.5) >= 3.0) {
                const bool landed = down && sp->plan != PLAN_FLOATER;
                const double hd = swarm ? course + 2.0 * std::sin(t * 1.3 + ph) : course + 0.3 * std::sin(t * 0.4 + ph);
                LifeJob& J = job(f.species, Vec3(bx, by, bz), landed ? ph : hd, 1.0, md, landed, doing);
                J.in.aloft = !landed;
                J.in.folded = landed;
                J.in.flap = sp->plan == PLAN_FLOATER ? 0 : (sp->plan == PLAN_GLIDER || glide ? 0.12 : std::sin(t * wf + ph));
                J.in.gait = sp->gait; J.in.seed = f.seed + i;
                if (landed) {   // perched on its branch or the ground: standing on it
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
    // ---- night eyes (M4-05): pairs of glints in the dark
    if (env.skyBrightness < 0.3 && kindHas(site.gen.type, KT_FLORA)) {
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
                    LifeJob& J = job(w.species, at, std::atan2(hx, hz), 1.0, dist, false, DO_LEAPING);
                    J.in.afloat = true; J.in.gait = GAIT_SWIM; J.in.seed = w.seed; J.in.tilt = std::atan2(H * 4 * (1 - 2 * u), len);
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
                LifeJob& J = job(w.species, Vec3(sx, wl - depth, sz), std::atan2(hx, hz), 1.0, dist, false, DO_SWIMMING);
                J.in.afloat = true; J.in.gait = GAIT_SWIM; J.in.seed = w.seed;
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
        if (onTrunk) {
            const double nx = c.x - c.treeX, nz = c.z - c.treeZ, nl = std::max(1e-6, std::sqrt(nx * nx + nz * nz));
            J.in.climbing = true; J.in.wall = Vec3(nx / nl, 0, nz / nl);
        }
        // tracks in sand and snow behind it
        if (dist < 80 && !c.tracks.empty()) {
            const double fog = fogOf(dist);
            for (const Critter::Track& tr : c.tracks) {
                RVert q = vert(Vec3(tr.x, site.groundHeight(tr.x, tr.z) + 0.03, tr.z), 10 + 40 * fog);
                if (q.z >= NEAR_Z) rasterPoint3(fb, q, 0, proj, true, 1);
            }
        }
    }
    for (const Specimen& s : specimens) {   // the harness's
        const Species* sp = speciesAt(s.species);
        if (!sp) continue;
        const double dist = length(s.at - camPos);
        Vec3 at = s.at;
        if (s.how == 0) { at.y = site.groundHeight(at.x, at.z); drawBlobShadow(fb, at.x, at.z, 0.5 * sp->bodyLen, sp->size); }
        LifeJob& J = job(s.species, at, s.heading, 1.0, dist, s.how == 0, -1);
        J.in.aloft = s.how == 1; J.in.afloat = s.how == 2; J.in.flap = s.flap; J.in.gait = sp->gait; J.in.seed = (uint64_t)s.species + 1;
        J.eyeshine = false;
        if (s.gait >= 0) { J.in.moving = true; J.in.gait = s.gait; J.in.phase = s.phase; J.in.stride = s.stride; J.in.running = s.running; }   // J-07
    }
    // ---- J-06: the animals posed and their shapes made on the pool, a list each; then filled in bands, in their order
    const int nj = (int)lifeJobs.size();
    if ((int)lifeLists.size() < nj) lifeLists.resize(nj);
    struct LifeView final : AnimalView {
        const SurfaceView* sv = nullptr;
        Vec3 sun; double ambient = 0, sunUp = 0, lf = 0;
        Vec3 view(const Vec3& p) const override { return sv->toView(p.x, p.y, p.z); }
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
            poseAnimal(rigs[J.rig], J.in, P);
            LifeView V;
            V.sv = this; V.sun = sd; V.ambient = ambient; V.sunUp = sunUp; V.lf = lf;
            V.fog = J.fog; V.eye = camPos; V.hide[0] = hideBanks[0]; V.hide[1] = hideBanks[1];
            V.pxPerM = J.pxPerM; V.fbScale = FB_SCALE; V.eyeshine = J.eyeshine;
            emitAnimal(rigs[J.rig], P, V, J.detail, L);
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
        lifeDrawn.push_back({J.rig, J.doing, J.dist, sx, sy, 0.6 * std::max(sp.size, 0.5 * sp.body.length) * J.in.scale * J.pxPerM});
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
        s.most = std::max(s.most, ++count[d.species]);
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
    int counts[CS_STATES] = {0};
    for (const Critter& c : critters) counts[clampi(c.state, 0, CS_STATES - 1)]++;
    std::string s;
    for (int i = 0; i < CS_STATES; i++) if (counts[i]) s += std::string(s.empty() ? "" : " ") + DOING_NAMES[i] + " " + std::to_string(counts[i]);
    return s + " (calls " + std::to_string(callsFired) + ")";
}
