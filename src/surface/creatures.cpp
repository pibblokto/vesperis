// N3 (R-202): creatures. Herds are spawned from the world's bestiary at a site (planLife), driven by a
// small state machine (idle, walk, graze, drink, rest, alert, flee, curious; leaders and followers;
// stampedes from the buggy) and drawn as articulated bodies: segmented trunks, necks, heads, tails and
// jointed legs with gait cycles whose feet stand on the terrain. Flyers flap, glide, perch in trees at
// dusk; swimmers show fins and backs. Calls and hoof-steps are cues for the synth; first sightings go
// to the guide.
#include "surface_view.h"
#include "core/noise.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <chrono>

namespace {
inline double h01(uint64_t h) { return unitFromHash(h); }
inline double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
}

const char* CRITTER_STATE_NAMES[8] = {"IDLE", "WALKING", "GRAZING", "DRINKING", "RESTING", "ALERT", "FLEEING", "CURIOUS"};

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
    if (b.type != PT_FELISIAN && b.type != PT_OCEAN) return;
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
            c.kind = sp.plan == PLAN_HEXAPOD ? 1 : 0;
            c.state = CS_IDLE;
            TerrainVertex tv = s.sampleAt(c.x, c.z, 16);
            if (tv.material == MAT_WATER || (tv.water > -1e8f && tv.h < tv.water + 0.5)) { c.x = cx; c.z = cz; }
            critters.push_back(c);
        }
    }
}

void SurfaceView::spawnLife(double lat, double lon) {
    const Body& b = site.sys->bodies[site.body];
    bestiary = Bestiary::make(b, site.gen);
    uint64_t seed = site.gen.seed ^ (uint64_t)(lat * 1e6) ^ ((uint64_t)(lon * 1e6) << 20);
    planLife(site, bestiary, seed, critters, flocks, herds);
    seenSpecies.clear(); sightings.clear(); pendingCalls.clear();
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
    if (site.gen.type != PT_FELISIAN || !bestiary.landCount) return;
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
                c.kind = sp.plan == PLAN_HEXAPOD ? 1 : 0;
                c.state = CS_IDLE;
                TerrainVertex tv = site.sampleAt(c.x, c.z, 16);
                if (tv.material == MAT_WATER || (tv.water > -1e8f && tv.h < tv.water + 0.5)) { c.x = hd.cx; c.z = hd.cz; }
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

void SurfaceView::updateLife(double dt, double t) {
    double t0 = nowMs();
    lastT = t;
    updateHabitat();   // B-316
    double sky = env.skyBrightness;
    // ---- flocks: circle, rest on the ground or in a tree, perch through the night (day flyers) or the day (dusk flyers)
    for (Flock& f : flocks) {
        const Species* sp = f.species >= 0 && f.species < (int)bestiary.species.size() ? &bestiary.species[f.species] : nullptr;
        bool wantsRest = sp && ((sp->activity == ACT_DAY && sky < 0.3) || (sp->activity == ACT_DUSK && sky > 0.6));
        f.restTimer -= dt;
        if (f.landed > 0) {
            f.landed -= dt;
            if (wantsRest) f.landed = std::max(f.landed, 5.0);   // stay down until the light changes
            double target = f.perched ? f.perchY - std::max(site.groundHeight(f.cx, f.cz), site.waterAt(f.cx, f.cz)) : 0.0;
            f.alt += (target - f.alt) * (1 - std::exp(-dt * 0.4));
            if (f.landed <= 0) { f.restTimer = 60 + 80 * h01(f.seed ^ (uint64_t)t); f.perched = false; }
        } else {
            f.alt += (f.cruiseAlt - f.alt) * (1 - std::exp(-dt * 0.5));
            if (f.restTimer <= 0 || wantsRest) {
                f.landed = 20 + 20 * h01(f.seed);
                // a perch: the tallest tree near the flock's centre
                double a = f.phase + t * f.speed;
                double fcx = f.cx + std::cos(a) * f.radius, fcz = f.cz + std::sin(a) * f.radius;
                int pcx = (int)std::floor(fcx / 16), pcz = (int)std::floor(fcz / 16);
                double bestH = 0; f.perched = false;
                for (int cz = pcz - 2; cz <= pcz + 2; cz++)
                    for (int cx = pcx - 2; cx <= pcx + 2; cx++)
                        forTrees(cx, cz, [&](const TreeInst& T) { if (!T.dead && T.h > bestH) { bestH = T.h; f.perched = true; f.perchX = T.x; f.perchZ = T.z; f.perchY = T.gy + T.cy + T.r * 0.4; f.perchR = T.r; } });
                if (f.perched) { f.cx = f.perchX; f.cz = f.perchZ; f.radius = 0; }
            }
        }
    }
    // ---- herds
    double localTime = env.localTime;
    double hoof = 0;
    for (size_t i = 0; i < critters.size(); i++) {
        Critter& c = critters[i];
        const Species* spp = c.species >= 0 && c.species < (int)bestiary.species.size() ? &bestiary.species[c.species] : nullptr;
        if (!spp) continue;
        const Species& sp = *spp;
        c.timer -= dt;
        double dpx = player.x - c.x, dpz = player.z - c.z, dp = std::sqrt(dpx * dpx + dpz * dpz);
        // threat: the explorer within 25 m (shy; curious 15 m), the moving buggy within 60 m; giants ignore everything
        double threatR = sp.temperament == TEMP_INDIFFERENT ? 0 : (sp.temperament == TEMP_CURIOUS ? 15 : 25);
        if (inBuggy && std::fabs(buggy.speed) > 3 && threatR > 0) threatR = 60;
        bool threatened = dp < threatR;
        if (threatened && c.state != CS_ALERT && c.state != CS_FLEE && !(c.state == CS_CURIOUS && dp > 8)) {
            c.state = CS_ALERT; c.timer = 0.8 + 0.8 * h01(c.seed ^ (uint64_t)(t * 3)); c.speed = 0; c.headPitch = 0; c.lift = std::min(c.lift, 0.5);
            if (t - c.calledT > 6) { pendingCalls.push_back({sp.call, sp.callPitch * (0.9 + 0.2 * h01(c.seed))}); c.calledT = t; callsFired++; }
        }
        // a first sighting within 60 m
        if (dp < 60 && !seenSpecies.count(c.species)) { seenSpecies.insert(c.species); sightings.push_back(sp.name + "|" + PLAN_NAMES[sp.plan] + "|" + std::to_string((int)dp)); }
        if (c.timer <= 0) {
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
                        if (j == i || o.herd != c.herd || o.state == CS_FLEE || o.state == CS_ALERT) continue;
                        double sx = o.x - c.x, sz = o.z - c.z;
                        if (sx * sx + sz * sz < 40.0 * 40.0) { o.state = CS_ALERT; o.timer = 0.3 + 0.5 * r.uni(); o.speed = 0; }
                    }
                }
            } else if (c.state == CS_FLEE) {
                c.state = CS_IDLE; c.timer = 2 + 3 * r.uni(); c.speed = 0;
            } else {
                bool restWanted = (sp.activity == ACT_DAY && ((localTime > 0.46 && localTime < 0.54) || sky < 0.2)) || (sp.activity == ACT_NIGHT && sky > 0.45) || (sp.activity == ACT_DUSK && (sky > 0.7 || sky < 0.05));
                double wx = 0, wz = 0;
                bool thirsty = localTime > 0.55 && localTime < 0.78 && sp.diet != 1 && waterNear(site, c.x, c.z, wx, wz);
                double dwx = wx - c.x, dwz = wz - c.z, dw = thirsty ? std::sqrt(dwx * dwx + dwz * dwz) : 1e9;
                double roll = r.uni();
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
                } else { c.state = CS_IDLE; c.timer = 2 + 3 * r.uni(); c.speed = 0; }
                // a call now and then from the leader
                if (leader && roll > 0.93 && t - c.calledT > 15 && dp < 120) { pendingCalls.push_back({sp.call, sp.callPitch}); c.calledT = t; callsFired++; }
            }
        }
        if (c.state == CS_CURIOUS) {   // walk toward the explorer and stop at 10 m, facing them
            if (dp > 10) { c.heading = std::atan2(dpx, dpz); c.speed = sp.walkSpeed; } else { c.speed = 0; c.heading = std::atan2(dpx, dpz); }
        }
        // posture targets
        double wantHead = (c.state == CS_GRAZE || c.state == CS_DRINK) ? 1.0 : 0.0, wantLift = c.state == CS_REST ? 1.0 : 0.0;
        c.headPitch += (wantHead - c.headPitch) * (1 - std::exp(-dt * 1.5));
        c.lift += (wantLift - c.lift) * (1 - std::exp(-dt * 0.8));
        if (c.state == CS_GRAZE && std::fmod(t + c.seed % 13, 7.0) < 1.5) c.headPitch += (0 - c.headPitch) * (1 - std::exp(-dt * 3));   // looks up now and then
        // movement, feet on the ground, no water, no steep slopes
        if (c.speed > 0 && c.lift < 0.5) {
            double nx = c.x + std::sin(c.heading) * c.speed * dt, nz = c.z + std::cos(c.heading) * c.speed * dt;
            double gy = site.groundHeight(nx, nz);
            double slope = std::fabs(gy - site.groundHeight(c.x, c.z));
            bool wet = gy < site.waterAt(nx, nz) + 0.3;
            if (slope > 0.6 * c.speed * dt + 0.3 || wet) { c.heading += 2.2; c.timer = std::min(c.timer, 1.0); }
            else {
                double moved = std::sqrt((nx - c.x) * (nx - c.x) + (nz - c.z) * (nz - c.z));
                c.x = nx; c.z = nz;
                double stride = sp.legLen * ((c.state == CS_FLEE) ? 1.3 : 0.75);
                c.gaitPhase += moved / std::max(stride, 0.05);
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
            { double hx = herds[c.herd].cx - c.x, hz = herds[c.herd].cz - c.z; if (hx * hx + hz * hz > 300.0 * 300.0) c.heading = std::atan2(hx, hz); }   // B-316: a herd stays within 300 m of its spot
        }
    }
    hoofLevel = std::min(1.0, hoof);
    lastLifeMs[0] = nowMs() - t0;
}

void SurfaceView::drawLife(Framebuffer& fb, double t) {
    double t0 = nowMs();
    if (flocks.empty() && critters.empty()) { lastLifeMs[1] = 0; return; }
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    double lf = env.sun.lightFactor;
    auto vert = [&](const Vec3& p, double shade) { RVert q; Vec3 v = toView(p.x, p.y, p.z); q.x = v.x; q.y = v.y; q.z = v.z; q.shade = shade; return q; };
    auto fogOf = [&](double dist) { return 1 - std::exp(-dist / env.fogDistance); };
    // ---- flyers: silhouettes with flapping or gliding wings, perched with folded wings
    for (const Flock& f : flocks) {
        const Species* sp = f.species >= 0 && f.species < (int)bestiary.species.size() ? &bestiary.species[f.species] : nullptr;
        double bsize = sp ? sp->size : 0.5;
        int bank = sp ? sp->bank : 1;
        double a = f.phase + t * f.speed;
        double fcx = f.cx + std::cos(a) * f.radius, fcz = f.cz + std::sin(a) * f.radius;
        double ground = site.groundHeight(fcx, fcz);
        double base = std::max(ground, site.waterAt(fcx, fcz)) + f.alt;
        double dist = std::sqrt((fcx - camPos.x) * (fcx - camPos.x) + (fcz - camPos.z) * (fcz - camPos.z));
        if (dist > 900) continue;
        Rng r(f.seed);
        bool down = f.landed > 0 && f.alt < 3;
        for (int i = 0; i < f.n; i++) {
            double ox = r.sym(18), oz = r.sym(18), oy = r.sym(6);
            double ph = r.range(0, TAU), wf = 6 + 4 * r.uni();
            double bx, by, bz;
            if (down && f.perched) { double pa = ph, pr = f.perchR * 0.8 * (0.3 + 0.7 * r.uni()); bx = f.perchX + std::cos(pa) * pr; bz = f.perchZ + std::sin(pa) * pr; by = f.perchY + r.sym(0.4); }
            else { bx = fcx + ox + 3 * std::sin(t * 0.7 + ph); bz = fcz + oz + 3 * std::cos(t * 0.5 + ph); by = base + oy + 1.5 * std::sin(t * 0.9 + ph); }
            Vec3 v = toView(bx, by, bz);
            if (v.z < 1) continue;
            double shade = 4 + 6 * env.skyBrightness;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = shade;
            int size = dist < 150 ? 2 : 1;
            rasterPoint3(fb, p, bank == 1 ? 1 : bank, proj, true, size);
            if (dist < 250) {
                bool glide = !down && std::sin(t * 0.35 + ph) > 0.45;   // long glides between flapping
                double flap = down ? 0.1 : (glide ? 0.2 : std::sin(t * wf + ph));
                double span = 1.8 * bsize * (down ? 0.35 : 1.0) * (dist < 80 ? 1.0 : 0.6);
                Vec3 l = toView(bx - span, by + 0.4 * flap * span, bz), rgt = toView(bx + span, by + 0.4 * flap * span, bz);
                RVert a1 = p, a2 = p;
                a1.x = l.x; a1.y = l.y; a1.z = l.z; a2.x = rgt.x; a2.y = rgt.y; a2.z = rgt.z;
                rasterLine3(fb, a1, p, bank == 1 ? 1 : bank, proj, true, 1);
                rasterLine3(fb, p, a2, bank == 1 ? 1 : bank, proj, true, 1);
            }
        }
    }
    // ---- night eyes (M4-05): pairs of glints in the dark
    if (env.skyBrightness < 0.3 && site.gen.type == PT_FELISIAN) {
        for (int i = 0; i < 8; i++) {
            uint64_t he = hash2i((int64_t)std::floor(camPos.x / 60), (int64_t)std::floor(camPos.z / 60) + i * 7, site.gen.seed ^ 0xE7E);
            if (h01(he) < 0.5) continue;
            double ex = std::floor(camPos.x / 60) * 60 + h01(mix64(he + 1)) * 60, ez = std::floor(camPos.z / 60) * 60 + h01(mix64(he + 2)) * 60;
            double blink = std::sin(t * (0.6 + h01(mix64(he + 3))) + h01(mix64(he + 4)) * TAU);
            if (blink < -0.2) continue;
            double gy = site.groundHeight(ex, ez) + 0.5;
            double hd = h01(mix64(he + 5)) * TAU;
            for (int k = -1; k <= 1; k += 2) {
                Vec3 v = toView(ex + std::cos(hd) * 0.12 * k, gy, ez - std::sin(hd) * 0.12 * k);
                if (v.z < NEAR_Z) continue;
                RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 58;
                rasterPoint3(fb, p, 1, proj, true, 1);
            }
        }
    }
    // ---- swimmers: fins, a back arc, the odd breach, in deep water within 250 m
    if (site.hasWater && (site.gen.type == PT_FELISIAN || site.gen.type == PT_OCEAN)) {
        const Species* sw = bestiary.firstSwimmer() >= 0 ? &bestiary.species[bestiary.firstSwimmer()] : nullptr;
        double ssz = sw ? sw->size : 1.5;
        int ccx = (int)std::floor(camPos.x / 100), ccz = (int)std::floor(camPos.z / 100);
        for (int dz = -2; dz <= 2; dz++)
            for (int dx = -2; dx <= 2; dx++) {
                uint64_t hs = hash2i(ccx + dx, ccz + dz, site.gen.seed ^ 0xF1A);
                if (h01(hs) > 0.35) continue;
                double cx = (ccx + dx) * 100.0 + 20 + h01(mix64(hs + 1)) * 60, cz = (ccz + dz) * 100.0 + 20 + h01(mix64(hs + 2)) * 60;
                double a = t * (0.03 + 0.03 * h01(mix64(hs + 3))) + h01(mix64(hs + 4)) * TAU;
                double sx = cx + std::cos(a) * 25, sz = cz + std::sin(a) * 25;
                double wl = site.waterAt(sx, sz);
                if (wl < -1e8 || site.groundHeight(sx, sz) > wl - 1.5) continue;
                double surf = std::sin(t * 0.7 + h01(mix64(hs + 5)) * TAU);   // above the water only part of the time
                if (surf < 0.2) continue;
                double dist = std::sqrt((sx - camPos.x) * (sx - camPos.x) + (sz - camPos.z) * (sz - camPos.z));
                if (dist > 250 || ssz / dist * proj.f < 1.0) continue;
                double hx = -std::sin(a), hz = std::cos(a);   // tangent of the circle
                double fh = (0.25 + 0.45 * (surf - 0.2)) * ssz * 0.6;
                double shade = 8 + 55 * fogOf(dist);
                RasterParams rp; rp.bank = 3;
                RVert p[3] = {vert(Vec3(sx - hx * 0.4 * ssz, wl + 0.02, sz - hz * 0.4 * ssz), shade), vert(Vec3(sx, wl + fh, sz), shade), vert(Vec3(sx + hx * 0.4 * ssz, wl + 0.02, sz + hz * 0.4 * ssz), shade)};
                if (p[0].z < NEAR_Z || p[1].z < NEAR_Z || p[2].z < NEAR_Z) continue;
                rasterPolygon(fb, p, 3, rp, proj);
                // the back: a low arc behind the fin; a breach lifts it high for a moment
                double breach = surf > 0.97 ? 1.0 : 0.0;
                double bh = (0.12 + 0.6 * breach) * ssz;
                RVert q[4] = {vert(Vec3(sx - hx * 1.6 * ssz, wl + 0.02, sz - hz * 1.6 * ssz), shade), vert(Vec3(sx - hx * 1.0 * ssz, wl + bh, sz - hz * 1.0 * ssz), shade + 6),
                              vert(Vec3(sx - hx * 0.45 * ssz, wl + bh * 0.8, sz - hz * 0.45 * ssz), shade + 6), vert(Vec3(sx - hx * 0.4 * ssz, wl + 0.02, sz - hz * 0.4 * ssz), shade)};
                bool ok = true; for (RVert& v : q) if (v.z < NEAR_Z) ok = false;
                if (ok) rasterPolygon(fb, q, 4, rp, proj);
            }
    }
    // ---- land creatures: articulated bodies
    for (const Critter& c : critters) {
        const Species* spp = c.species >= 0 && c.species < (int)bestiary.species.size() ? &bestiary.species[c.species] : nullptr;
        if (!spp) continue;
        const Species& sp = *spp;
        double dist = std::sqrt((c.x - camPos.x) * (c.x - camPos.x) + (c.z - camPos.z) * (c.z - camPos.z));
        if (dist > std::min(600.0, 250.0 + 120.0 * c.size)) continue;   // B-316: the big ones show from farther
        double sz = c.size, k = c.size / std::max(sp.size, 0.05);   // this animal's scale of its species
        if (sz / dist * proj.f < 1.0) continue;
        double gy = site.groundHeight(c.x, c.z);
        double hx = std::sin(c.heading), hz = std::cos(c.heading);
        Vec3 fwd(hx, 0, hz), side(-hz, 0, hx), up(0, 1, 0);
        double bodyLen = sp.bodyLen * k, legLen = sp.legLen * k, neckLen = sp.neckLen * k, headLen = sp.headLen * k, tailLen = sp.tailLen * k;
        double wid = sz * (sp.plan == PLAN_HEXAPOD ? 0.55 : 0.45), bodyH = sz * (sp.plan == PLAN_HEXAPOD ? 0.3 : 0.5);
        bool moving = c.speed > 0.05 && c.lift < 0.5;
        double ph = c.gaitPhase;
        double bob = moving ? 0.04 * sz * std::sin(ph * TAU * 2) : 0;
        double hop = (sp.gait == GAIT_HOP && moving) ? 0.5 * legLen * std::max(0.0, std::sin(ph * TAU)) : 0;
        double standH = legLen * (1 - 0.72 * c.lift) + hop + bob;
        Vec3 c0(c.x, gy + standH, c.z);
        int bank = sp.bank;
        double fog = fogOf(dist);
        auto shadeOf2 = [&](const Vec3& n, double mul) {
            double light = ambient + (1 - ambient) * std::max(0.0, dot(n, sd)) * sunUp * lf;
            double s = 44 * std::pow(light, 0.7) * sp.tone * mul;
            return s + (63 - s) * fog;
        };
        auto quad = [&](const Vec3& a, const Vec3& b, const Vec3& cc, const Vec3& d, const Vec3& n, double mul) {
            RVert q[4] = {vert(a, shadeOf2(n, mul)), vert(b, shadeOf2(n, mul)), vert(cc, shadeOf2(n, mul)), vert(d, shadeOf2(n, mul))};
            for (RVert& v : q) if (v.z < NEAR_Z) return;
            RasterParams rp; rp.bank = bank;
            rasterPolygon(fb, q, 4, rp, proj);
        };
        auto seg = [&](const Vec3& a, const Vec3& b, double sh, int bk, int th) {
            RVert va = vert(a, sh), vb = vert(b, sh);
            if (va.z < NEAR_Z || vb.z < NEAR_Z) return;
            rasterLine3(fb, va, vb, bk, proj, true, th);
        };
        // a box segment (top and two sides) along the body axis
        auto box = [&](const Vec3& centre, double len, double w, double h, double mul) {
            Vec3 corners[8];
            for (int i = 0; i < 8; i++) corners[i] = centre + fwd * ((i & 1) ? len * 0.5 : -len * 0.5) + side * ((i & 2) ? w * 0.5 : -w * 0.5) + up * ((i & 4) ? h * 0.5 : -h * 0.5);
            double topMul = sp.pattern == 3 ? 0.85 * mul : mul, sideMul = sp.pattern == 3 ? 1.1 * mul : mul;
            quad(corners[4], corners[5], corners[7], corners[6], up, topMul);
            quad(corners[0], corners[1], corners[5], corners[4], -side, sideMul);
            quad(corners[2], corners[3], corners[7], corners[6], side, sideMul);
        };
        drawBlobShadow(fb, c.x, c.z, 0.5 * bodyLen, sz);
        bool full = dist < 60, mid = dist < 150;
        int th = dist < 25 ? 2 : 1;
        // the trunk: three segments, striped when patterned
        for (int i = 0; i < 3; i++) {
            double mul = sp.pattern == 1 ? (i == 1 ? 0.78 : 1.08) : 1.0;
            double w = wid * (i == 1 ? 1.0 : 0.85), h = bodyH * (i == 1 ? 1.0 : 0.85);
            box(c0 + fwd * (bodyLen * (i - 1) / 3.0), bodyLen / 3.0 + 0.02, w, h, mul);
        }
        if (full && sp.pattern == 2) {   // spots on the flanks
            for (int i = 0; i < 8; i++) {
                uint64_t hs = mix64(c.seed + 300 + i);
                Vec3 p = c0 + fwd * ((h01(hs) - 0.5) * bodyLen * 0.9) + side * ((i & 1 ? 1 : -1) * wid * 0.52) + up * ((h01(mix64(hs + 1)) - 0.5) * bodyH * 0.7);
                RVert q = vert(p, shadeOf2(i & 1 ? side : -side, 0.55));
                if (q.z >= NEAR_Z) rasterPoint3(fb, q, bank, proj, true, 1);
            }
        }
        // the neck and head: pitched by the plan, lowered to the ground when grazing or drinking
        double neckUp = sp.plan == PLAN_LONGNECK ? 55 * DEG : (sp.plan == PLAN_BIPED ? 60 * DEG : (sp.plan == PLAN_HOPPER ? 45 * DEG : (sp.plan == PLAN_GIANT ? 35 * DEG : (sp.plan == PLAN_HEXAPOD ? 0 : 22 * DEG))));
        double np = neckUp - c.headPitch * (neckUp + 42 * DEG);
        Vec3 neckDir = fwd * std::cos(np) + up * std::sin(np);
        Vec3 neck0 = c0 + fwd * (bodyLen * 0.5) + up * (bodyH * 0.3);
        Vec3 neck1 = neck0 + neckDir * neckLen;
        if (c.headPitch > 0.5 && neck1.y < gy + 0.05) neck1.y = gy + 0.05;
        {
            double nw = wid * 0.45, nh = bodyH * 0.45;
            Vec3 nside = side * (nw * 0.5), nup = normalize(cross(neckDir, side)) * (nh * 0.5);
            if (nup.y < 0) nup = -nup;
            quad(neck0 - nside + nup, neck0 + nside + nup, neck1 + nside + nup, neck1 - nside + nup, up, 1.0);
            quad(neck0 - nside - nup, neck0 - nside + nup, neck1 - nside + nup, neck1 - nside - nup, -side, 1.0);
            quad(neck0 + nside - nup, neck0 + nside + nup, neck1 + nside + nup, neck1 + nside - nup, side, 1.0);
        }
        Vec3 headDir = normalize(neckDir * 0.6 + fwd * 0.5 - up * 0.25);
        Vec3 head0 = neck1, head1 = neck1 + headDir * headLen;
        {
            double hw = wid * 0.42, hh = bodyH * 0.42;
            Vec3 hside = side * (hw * 0.5), hup = normalize(cross(headDir, side)) * (hh * 0.5);
            if (hup.y < 0) hup = -hup;
            quad(head0 - hside + hup, head0 + hside + hup, head1 + hside + hup * 0.6, head1 - hside + hup * 0.6, up, 1.0);
            quad(head0 - hside - hup, head0 - hside + hup, head1 - hside + hup * 0.6, head1 - hside - hup * 0.6, -side, 0.95);
            quad(head0 + hside - hup, head0 + hside + hup, head1 + hside + hup * 0.6, head1 + hside - hup * 0.6, side, 0.95);
            if (full && dist < 30) {   // eyes
                for (int e = -1; e <= 1; e += 2) {
                    RVert q = vert(head0 + headDir * (headLen * 0.55) + side * (e * hw * 0.55) + hup * 0.5, 58);
                    if (q.z >= NEAR_Z) rasterPoint3(fb, q, 8, proj, true, 1);
                }
            }
            if (mid) {   // crests: horns, ears, a crest along the neck, antlers
                double dark = shadeOf2(up, 0.5);
                switch (sp.crest) {
                    case 1: for (int e = -1; e <= 1; e += 2) seg(head0 + hup + side * (e * hw * 0.4), head0 + hup + side * (e * hw * 0.9) + up * (sz * 0.3) + headDir * (sz * 0.15), dark, bank, th); break;
                    case 2: for (int e = -1; e <= 1; e += 2) seg(head0 + hup + side * (e * hw * 0.4), head0 + hup + side * (e * hw * 0.8) + up * (sz * 0.18), dark, bank, 1); break;
                    case 3: seg(neck0 + up * (bodyH * 0.35), neck1 + up * (bodyH * 0.35), dark, bank, th); break;
                    case 4: for (int e = -1; e <= 1; e += 2) { Vec3 a0 = head0 + hup + side * (e * hw * 0.4), a1 = a0 + up * (sz * 0.35) + side * (e * sz * 0.2); seg(a0, a1, dark, bank, 1); seg(a0 + (a1 - a0) * 0.5, a0 + (a1 - a0) * 0.5 + up * (sz * 0.2) - side * (e * sz * 0.08), dark, bank, 1); } break;
                    default: break;
                }
            }
        }
        // the tail
        {
            double wag = std::sin(t * 3 + c.seed % 7) * (moving ? 0.35 : 0.12);
            Vec3 t0 = c0 - fwd * (bodyLen * 0.5) + up * (bodyH * 0.2);
            Vec3 t1 = t0 - fwd * (tailLen * 0.55) + side * (wag * tailLen * 0.4) + up * (sp.plan == PLAN_HOPPER ? -tailLen * 0.1 : tailLen * 0.15);
            Vec3 t2 = t1 - fwd * (tailLen * 0.45) + side * (wag * tailLen * 0.6) - up * (tailLen * 0.35);
            double ts = shadeOf2(up, 0.8);
            seg(t0, t1, ts, bank, th); seg(t1, t2, ts, bank, 1);
        }
        // the legs: gait cycles per plan, feet on the terrain
        int nlegs = sp.plan == PLAN_HEXAPOD ? 6 : ((sp.plan == PLAN_BIPED || sp.plan == PLAN_HOPPER) ? 2 : 4);
        double stride = legLen * (c.state == CS_FLEE ? 1.1 : 0.6);
        double legShade = shadeOf2(side, 0.7);
        for (int L = 0; L < nlegs; L++) {
            double fx, sx, off;
            if (nlegs == 6) { fx = bodyLen * (0.38 - 0.38 * (L / 2)); sx = (L & 1) ? 1 : -1; off = ((L / 2) & 1) ? ((L & 1) ? 0.5 : 0.0) : ((L & 1) ? 0.0 : 0.5); }
            else if (nlegs == 2) { fx = -bodyLen * 0.1; sx = (L & 1) ? 1 : -1; off = sp.gait == GAIT_HOP ? 0.0 : (L & 1) * 0.5; }
            else { fx = (L & 1) ? bodyLen * 0.38 : -bodyLen * 0.38; sx = (L & 2) ? 1 : -1; off = sp.runGait == GAIT_GALLOP && c.state == CS_FLEE ? ((L & 1) ? 0.0 + 0.1 * ((L & 2) ? 1 : 0) : 0.5 + 0.1 * ((L & 2) ? 1 : 0)) : (((L & 1) ^ ((L & 2) >> 1)) ? 0.0 : 0.5); }
            Vec3 hip = c0 + fwd * fx + side * (sx * wid * 0.45) - up * (bodyH * 0.4);
            Vec3 knee, foot;
            if (c.lift > 0.5) {   // folded under the body
                knee = hip + fwd * (legLen * 0.3) - up * (legLen * 0.2);
                foot = knee + fwd * (legLen * 0.35);
            } else {
                double swing = moving ? std::sin((ph + off) * TAU) : 0, liftA = moving ? std::max(0.0, std::cos((ph + off) * TAU)) : 0;
                if (sp.gait == GAIT_HOP && moving) { swing = -0.3 + 0.6 * std::max(0.0, std::sin(ph * TAU)); liftA = 0; }
                double fy = site.groundHeight(hip.x + hx * swing * stride, hip.z + hz * swing * stride);
                foot = Vec3(hip.x + hx * swing * stride, fy + liftA * 0.25 * legLen, hip.z + hz * swing * stride);
                knee = (hip + foot) * 0.5 + fwd * ((sp.plan == PLAN_HEXAPOD ? -0.15 : 0.18) * legLen + liftA * 0.2 * legLen) + side * (sx * (sp.plan == PLAN_HEXAPOD ? 0.35 : 0.05) * legLen);
            }
            if (full || mid) { seg(hip, knee, legShade, bank, th); seg(knee, foot, legShade, bank, th); }
            else seg(hip, foot, legShade, bank, 1);
        }
        if (sp.plan == PLAN_BIPED && mid) {   // arms swinging against the legs
            for (int e = -1; e <= 1; e += 2) {
                double swing = moving ? std::sin((ph + (e > 0 ? 0.5 : 0.0)) * TAU) : 0.1;
                Vec3 sh = c0 + fwd * (bodyLen * 0.35) + side * (e * wid * 0.5) + up * (bodyH * 0.3);
                seg(sh, sh + fwd * (swing * legLen * 0.3) - up * (legLen * 0.35), legShade, bank, 1);
            }
        }
        // tracks in sand and snow behind it
        if (dist < 80 && !c.tracks.empty()) {
            for (const Critter::Track& tr : c.tracks) {
                RVert q = vert(Vec3(tr.x, site.groundHeight(tr.x, tr.z) + 0.03, tr.z), 10 + 40 * fog);
                if (q.z >= NEAR_Z) rasterPoint3(fb, q, 0, proj, true, 1);
            }
        }
    }
    lastLifeMs[1] = nowMs() - t0;
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
    int counts[8] = {0};
    for (const Critter& c : critters) counts[clampi(c.state, 0, 7)]++;
    std::string s;
    for (int i = 0; i < 8; i++) if (counts[i]) s += std::string(s.empty() ? "" : " ") + CRITTER_STATE_NAMES[i] + " " + std::to_string(counts[i]);
    return s + " (calls " + std::to_string(callsFired) + ")";
}
