#include "space_view.h"
#include "galaxy/probe.h"
#include "galaxy/fronts.h"
#include "galaxy/nights.h"   // W-04: the earthshine
#include "core/noise.h"
#include "core/rng.h"
#include "core/parallel.h"
#include "galaxy/drainage.h"
#include <cmath>
#include <atomic>
#include <cstdio>
#include <algorithm>

static constexpr double BAND_SHADES = 24.0;   // G-03: the galactic band at full brightness, in shades of bank 0 (the ramp's dim blue-grey stop is at 20): the bulge 21, the plane 8-10

int SpaceRenderer::dbgX = -1, SpaceRenderer::dbgY = -1;

double SpaceRenderer::lightFactor(double luminosity, double distKm) { return StarSystem::starLightFactor(luminosity, distKm); }

void drawStarField(Framebuffer& fb, const std::vector<Star>& stars, const Vec3& observer, const Mat3& cam,
                   const Proj& pj, double intensityScale, int whiteBank, bool coloured, const Pix* skyMask,
                   double skyMaskThreshold, double streak, const Vec3& streakDirView, double twinkle, double time) {
    const int S = FB_SCALE;
    for (const Star& s : stars) {
        Vec3 d = s.pos - observer;
        double dist2 = length2(d);
        if (dist2 < 1) continue;
        Vec3 v = cam * d;
        if (v.z <= 0) continue;
        double iz = 1.0 / v.z;
        double sx = pj.cx + pj.f * v.x * iz, sy = pj.cy - pj.f * v.y * iz;
        if (sx < 0 || sy < 0 || sx >= FBW || sy >= FBH) continue;
        double distSectors2 = dist2 / (SECTOR_KM * SECTOR_KM);
        double b = s.luminosity / std::max(distSectors2, 0.0004);
        double inten = (46.0 + 9.0 * std::log10(b)) * intensityScale;
        if (twinkle > 0) inten *= 1.0 + twinkle * 0.35 * std::sin(time * 9.0 + (double)(s.seed % 628) * 0.01);
        if (inten < 2) continue;
        if (inten < 20.0 * intensityScale) inten = 20.0 * intensityScale;   // the neighbourhood stays visible
        if (inten > 63) inten = 63;
        int bank = whiteBank;
        if (coloured) {
            const RGB& c = s.color;
            if (c.b > c.r + 0.12f) bank = 4;
            else if (c.r > c.b + 0.25f) bank = 5;
        }
        auto plot = [&](int x, int y, double val) {
            if (x < 0 || y < 0 || x >= FBW || y >= FBH) return;
            int o = y * FBW + x;
            if (skyMask) {
                double sv = shadeOf(skyMask[o]);
                if (sv >= skyMaskThreshold || fb.invz[o] > 0) return;
                val *= 1.0 - sv / skyMaskThreshold;
            }
            Pix& p = fb.idx[o];
            int nv = toInten(val);
            if (nv > intenOf(p)) p = pixI(bank, nv);
        };
        int x = (int)sx, y = (int)sy;
        // a 2x2 core (in 1x pixels) survives the mush filter: a lone pixel keeps only a quarter
        // of its value; brighter stars get a halo ring one 1x pixel wide
        const int C = S + 1;   // core side: 2 px at 1x, 3 at 2x ... (the mush box grows the same way)
        for (int dy = -1; dy <= C; dy++)
            for (int dx = -1; dx <= C; dx++) {
                bool core = dx >= 0 && dy >= 0 && dx < C && dy < C;
                if (core) plot(x + dx, y + dy, inten);
                else if (inten > 34) plot(x + dx, y + dy, inten * 0.45);
            }
        if (streak > 0) {
            // M1-10 colour shift: stars ahead (near the centre) blue-shifted, the rest red-shifted
            { double ex = sx - pj.cx, ey = sy - pj.cy; bank = (ex * ex + ey * ey) < (0.3 * FBW) * (0.3 * FBW) ? 4 : 5; }
            // motion streak away from the direction of travel (stars stream past)
            double vx = v.x - streakDirView.x * v.z * 0.0, vy = v.y;
            (void)vx; (void)vy;
            double ox = sx - pj.cx, oy = sy - pj.cy;
            double len = streak * (0.02 + 0.08 * std::sqrt(ox * ox + oy * oy) / (160.0 * S)) * 60.0 * S;
            int n = (int)len;
            double nx = ox, ny = oy, nl = std::sqrt(nx * nx + ny * ny) + 1e-6;
            nx /= nl; ny /= nl;
            for (int i = 1; i <= n; i++) plot((int)(sx - nx * i), (int)(sy - ny * i), inten * (1.0 - (double)i / (n + 1)));
        }
    }
}

void SpaceRenderer::setSeason(const StarSystem& sys, double t) {
    for (int i = 0; i < (int)sys.bodies.size() && i < 64; i++) {
        double s = sys.seasonOf(i, t);
        int bucket = (int)std::floor((s + 1.0) * 3.999);   // eight steps per orbit
        if (bucket != seasonBucket[i] || std::fabs(s - curSeason[i]) > 0.3) {
            seasonBucket[i] = bucket; curSeason[i] = s;
            gens.erase(sys.bodies[i].seed);   // the map's seasonBucket no longer matches: regenerated on request
        }
    }
}

const BodyGen& SpaceRenderer::genFor(const Body& b) {
    auto it = gens.find(b.seed);
    if (it != gens.end()) return it->second;
    return gens.emplace(b.seed, BodyGen::make(b, curSeason[b.index < 64 ? b.index : 0])).first->second;
}

void SpaceRenderer::frontCoarse(FrontMap& fm) {   // W-03: the blocks that hold a band, each nonzero texel marking its block and its neighbours'
    const int W = PlanetMap::W, H = PlanetMap::H, B = FRONT_BLOCK, CW = W / B, CH = H / B;
    fm.coarse.assign((size_t)CW * CH, 0);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            if (!fm.cloud[(size_t)y * W + x]) continue;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int yy = clampi(y + dy, 0, H - 1), xx = ((x + dx) % W + W) % W;
                    fm.coarse[(size_t)(yy / B) * CW + xx / B] = 1;
                }
        }
}

const SpaceRenderer::FrontMap* SpaceRenderer::frontsFor(const Body& b, double t) {   // W-03
    if (!worldHasFronts(b)) return nullptr;
    for (FrontMap& fc : frontMaps)
        if (fc.seed == b.seed && !fc.cloud.empty()) {
            if (std::fabs(t - fc.t) > 240) { buildFrontMap(b, t, PlanetMap::W, PlanetMap::H, fc.cloud); frontCoarse(fc); fc.t = t; }
            return &fc;
        }
    FrontMap& fc = frontMaps[frontMapNext]; frontMapNext = (frontMapNext + 1) % 4;
    fc.seed = b.seed; fc.t = t;
    buildFrontMap(b, t, PlanetMap::W, PlanetMap::H, fc.cloud);
    frontCoarse(fc);
    return &fc;
}

SpaceRenderer::~SpaceRenderer() {
    for (auto& kv : mapThreads) if (kv.second && kv.second->joinable()) kv.second->join();
}

void SpaceRenderer::joinMap(uint64_t seed) {
    auto th = mapThreads.find(seed);
    if (th != mapThreads.end()) { if (th->second && th->second->joinable()) th->second->join(); mapThreads.erase(th); }
}

// Generation runs on a worker thread so an approach never stalls; the globe is drawn flat until it is done.
void SpaceRenderer::startMap(const Body& b, int bucket) {
    // M7-02: least-recently-used eviction down to the cap (never a map that is being generated)
    while ((int)maps.size() >= mapCacheCap) {
        uint64_t victim = 0, oldest = ~0ULL; bool found = false;
        for (auto& kv : maps) {
            if (kv.first == b.seed || kv.second->generating) continue;
            uint64_t stamp = mapLastUse.count(kv.first) ? mapLastUse[kv.first] : 0;
            if (stamp < oldest) { oldest = stamp; victim = kv.first; found = true; }
        }
        if (!found) break;
        joinMap(victim); maps.erase(victim); mapLastUse.erase(victim);
        mapStats.evicted++;
    }
    auto& up = maps[b.seed];
    if (!up) up.reset(new PlanetMap());
    PlanetMap* m = up.get();
    joinMap(b.seed);
    m->generating = true;
    m->seasonBucket = bucket;
    BodyGen g = genFor(b);
    mapStats.generated++;
    std::atomic<long>* micros = &genMicros;
    mapThreads[b.seed].reset(new std::thread([m, g, micros]() {
        auto t0 = std::chrono::steady_clock::now();
        m->generate(g);
        micros->fetch_add((long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count());
        m->generating = false;
    }));
}

const PlanetMap* SpaceRenderer::mapIfReady(const Body& b) {
    int bucket = b.index < 64 ? seasonBucket[b.index] : 0;
    mapLastUse[b.seed] = ++useStamp;
    auto it = maps.find(b.seed);
    if (it == maps.end()) { startMap(b, bucket); return nullptr; }
    PlanetMap* m = it->second.get();
    if (m->generating) return nullptr;
    if (!m->valid || m->seasonBucket != bucket) { startMap(b, bucket); return nullptr; }
    joinMap(b.seed);
    mapStats.hits++;
    return m;
}

const PlanetMap& SpaceRenderer::mapFor(const Body& b) {
    const PlanetMap* m = mapIfReady(b);
    if (m) return *m;
    joinMap(b.seed);
    PlanetMap* mm = maps[b.seed].get();
    if (!mm->valid) { auto t0 = std::chrono::steady_clock::now(); mm->generate(genFor(b)); mm->generating = false; genMicros.fetch_add((long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count()); }
    return *mm;
}

const std::vector<float>& SpaceRenderer::ringProfile(const StarSystem& sys, int bi) {
    const Body& b = sys.bodies[bi];
    auto it = ringProfiles.find(b.seed);
    if (it != ringProfiles.end()) return it->second;
    return ringProfiles.emplace(b.seed, sys.ringProfileOf(bi)).first->second;   // O0-01: one profile for the globe, the sky and the ground shadow
}

void SpaceRenderer::materialRamp(uint8_t* pal, int bank, RGB c, const RGB* skyLight) {
    MatRamp r = ::materialRamp(c, skyLight, 1.0);
    setRamp(pal, bank, {{0, r.stop[0]}, {14, r.stop[1]}, {34, r.stop[2]}, {47, r.stop[3]}, {56, r.stop[4]}, {63, r.stop[5]}});
}

// the noon sky light of a body with an atmosphere (the surface's zenith/horizon mix), for the hemisphere term
static bool skyLightOf(const BodyGen& g, const RGB& lightC, RGB& out) {
    RGB zen, hor;
    if (!typeSky(g, zen, hor)) return false;   // R-307: the one pair per type
    out = lerp(zen, hor, 0.5f) * lightC;
    return true;
}

// O0-02 (B-305): each of the two worlds with a disc owns six banks, so neither borrows the other's colours
int SpaceRenderer::globeBank(int bodyBank, int material) {
    if (bodyBank != 2 && bodyBank != 3) return bodyBank;
    bool second = bodyBank == 3;
    switch (matFamily(material)) {
        case FAM_WATER: return second ? 7 : 6;
        case FAM_FOREST: return second ? 16 : 11;
        case FAM_GRASS: return second ? 19 : 15;
        case FAM_SAND: return second ? 17 : 13;
        case FAM_SNOW: return second ? 18 : 14;
        default: return bodyBank;
    }
}

void SpaceRenderer::setupPalette(Framebuffer& fb, const StarSystem* sys, int bodyA, int bodyB, double fade) {
    RGB white(1, 1, 1);
    setRamp(fb.pal, 0, {{0, RGB(0, 0, 0)}, {20, RGB(0.18f, 0.2f, 0.28f)}, {45, RGB(0.7f, 0.72f, 0.8f)}, {63, white}});
    RGB sc = sys && sys->valid ? sys->star.color : RGB(1, 1, 1);
    setRamp(fb.pal, 1, {{0, RGB(0, 0, 0)}, {24, sc * 0.55f}, {48, sc}, {63, lerp(sc, white, 0.75f)}});
    // blue and red star ramps (KI-008: stars stay coloured whatever the bodies use)
    setRamp(fb.pal, 4, {{0, RGB(0, 0, 0)}, {30, RGB(0.35f, 0.45f, 0.9f)}, {63, RGB(0.85f, 0.9f, 1)}});
    setRamp(fb.pal, 5, {{0, RGB(0, 0, 0)}, {30, RGB(0.8f, 0.35f, 0.2f)}, {63, RGB(1, 0.85f, 0.7f)}});
    if (sys && sys->valid && sys->companion >= 0) {   // M5-01: the second sun in its own colour
        RGB kc = sys->bodies[sys->companion].color;
        setRamp(fb.pal, 12, {{0, RGB(0, 0, 0)}, {24, kc * 0.55f}, {48, kc}, {63, lerp(kc, white, 0.75f)}});
    }
    auto lightOf = [&]() { return sys && sys->valid ? lerp(sys->star.color, white, 0.5f) : white; };
    auto genOf = [&](int bi) -> const BodyGen* { return sys && bi >= 0 && bi < (int)sys->bodies.size() ? &genFor(sys->bodies[bi]) : nullptr; };
    // N1-02: the body bank carries the rock family in the material palette's colour; molten and substellar keep their glow ramps
    auto bodyRamp = [&](int bank, int bi) {
        const BodyGen* g = genOf(bi);
        if (!g) { setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {63, RGB(0.7f, 0.7f, 0.7f)}}); return; }
        const Body& b = sys->bodies[bi];
        RGB c = g->matColor[familyRep(b.type, FAM_ROCK)] * lightOf();
        if (isLavaWorld(b.type)) {
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {30, c * 0.8f}, {46, RGB(0.55f, 0.28f, 0.18f)}, {52, RGB(0.9f, 0.3f, 0.05f)}, {58, RGB(1, 0.6f, 0.1f)}, {63, RGB(1, 0.95f, 0.6f)}});
        } else if (b.type == PT_SUBSTELLAR) {   // M5-02: its own dull red glow, hotter toward the bright end
            RGB gc = b.color;
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {10, gc * 0.35f}, {30, gc}, {46, lerp(gc, RGB(1, 0.55f, 0.25f), 0.6f)}, {58, RGB(1, 0.75f, 0.4f)}, {63, RGB(1, 0.9f, 0.7f)}});
        } else if (hasOpaqueDeck(b.type)) {   // the globe is its cloud (or haze) deck
            materialRamp(fb.pal, bank, g->matColor[MAT_CLOUD] * lightOf(), nullptr);
        } else { RGB sk; bool hasSky = skyLightOf(*g, lightOf(), sk); materialRamp(fb.pal, bank, c, hasSky ? &sk : nullptr); }
    };
    bodyRamp(2, bodyA);
    bodyRamp(3, bodyB);
    {   // O3 bank 20: the belt rocks' warm grey under the star's light
        RGB lc = lightOf();
        setRamp(fb.pal, 20, {{0, RGB(0, 0, 0)}, {14, RGB(0.11f, 0.10f, 0.09f) * lc}, {34, RGB(0.30f, 0.27f, 0.25f) * lc}, {47, RGB(0.48f, 0.44f, 0.40f) * lc}, {56, RGB(0.66f, 0.62f, 0.58f) * lc}, {63, RGB(0.9f, 0.87f, 0.84f) * lc}});
    }
    auto waterRamp = [&](int bank, int bi) {
        const BodyGen* g = genOf(bi);
        RGB oc = (g ? g->matColor[MAT_WATER] : RGB(0.10f, 0.28f, 0.62f)) * lightOf();
        setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {14, oc * 0.3f}, {34, oc * 0.7f}, {47, oc}, {56, lerp(oc, white, 0.5f)}, {63, white}});
    };
    waterRamp(6, bodyA);
    waterRamp(7, bodyB);
    // O0-02 (B-305): each world with a disc gets its own family banks in its own colours (they used to be shared, and the
    // smaller world borrowed the bigger one's sand and snow: a world changed colour as you approached its neighbour)
    static const int famBankA[FAM_COUNT] = {-1, -1, 11, 15, 13, 14}, famBankB[FAM_COUNT] = {-1, -1, 16, 19, 17, 18};
    for (int which = 0; which < 2; which++) {
        int bi = which == 0 ? bodyA : bodyB;
        const BodyGen* g = genOf(bi);
        const int* banks = which == 0 ? famBankA : famBankB;
        RGB sk; bool hasSky = g && skyLightOf(*g, lightOf(), sk);
        for (int fam = FAM_FOREST; fam < FAM_COUNT; fam++) {
            RGB c = g ? g->matColor[familyRep(g->type, fam)] : RGB(0.6f, 0.6f, 0.6f);
            materialRamp(fb.pal, banks[fam], c * lightOf(), hasSky ? &sk : nullptr);
        }
    }
    if (fade < 1) scalePalette(fb.pal, fade);
}

// B-308: the sun's disc and glow by angle. Each pixel's ray is compared with the star's direction, so the disc is right
// whatever its size and wherever its centre is. The old screen-space circle used the radius f tan(a) d/z, which grows
// without bound as the centre nears the camera plane: with the ship parked 3.5 radii from a companion star, a swing of
// the camera flooded the whole frame with the companion's colour. `dirView` is the unit direction to the star in view
// space, `angR` its angular radius. The rays of compact stars and the point of a tiny sun stay screen-space, drawn
// from the projected centre while it lies in front of the camera.
void SpaceRenderer::drawSun(Framebuffer& fb, const Star& star, const Vec3& dirView, double angR, double t,
                            double intensity, int bank, bool atmosphere, double atmosHaze, bool depthTest, const Proj& pj, const Vec3* discUp) {
    int cls = star.cls;
    if (cls == STAR_BLACK_HOLE) { drawBlackHole(fb, star, dirView, angR, t, intensity, bank, depthTest, pj, discUp); return; }   // S-06: no disc of light at all
    double glowMul = 1.0;
    double pulse = 1.0;
    double flare = starFlare(star, t);   // S-01: a red dwarf's flare brightens the disc and the corona for a minute
    if (cls == STAR_PULSAR) {
        double ph = std::fmod(t * star.pulseHz, 1.0);
        pulse = 0.15 + 0.85 * std::pow(std::max(0.0, std::cos(ph * TAU)), 12.0);
        glowMul = 0.6 + 1.4 * pulse;
    }
    if (cls == STAR_BLUE_GIANT) glowMul = 1.8;
    if (cls == STAR_RED_GIANT) glowMul = 0.6;
    if (cls == STAR_WHITE_DWARF) glowMul = 1.1;
    if (cls == STAR_ORANGE) glowMul = 0.9;
    if (cls == STAR_RED_DWARF) glowMul = 0.8 + 0.7 * flare;   // S-01
    if (cls == STAR_BLUE_WHITE) glowMul = 1.5;
    if (cls == STAR_ORANGE_GIANT) glowMul = 0.7;
    if (cls == STAR_CARBON) glowMul = 1.4;   // the soot's haze: wide and dim (its corona is halved below)
    if (cls == STAR_NEUTRON) glowMul = 1.3;   // S-03: a point of white light in a soft steady halo, no rays, no beams
    if (cls == STAR_PROTOSTAR) glowMul = 2.0;   // S-04: the accretion cloud, a wide mottled glow round a soft disc
    if (cls == STAR_WOLF_RAYET) glowMul = 1.6;   // S-05: blinding, inside the ring of its shell (drawn in the glow's pass below)
    const double S = FB_SCALE, invf = 1.0 / pj.f;
    double rA = std::max(angR, 0.6 * S * invf);   // the old floor of 0.6 px, as an angle
    double coronaA = std::max(rA * (atmosphere ? 3.2 : 2.4), 5.0 * S * invf) * glowMul;
    double outerA = std::min(coronaA * (atmosphere ? 3.0 : 1.6) * (1.0 + atmosHaze), PI - 0.01);
    int addOuter = (int)(intensity * (atmosphere ? 14 : 8) * pulse * (cls == STAR_CARBON ? 0.7 : 1.0));
    int addCorona = (int)(intensity * 30 * pulse * (cls == STAR_CARBON ? 0.5 : 1.0) * (1 + 0.5 * flare));
    // S-05: the Wolf-Rayet star's shell of shed gas, a ring at five radii (four pixels at least) a seventh of that wide, sharp
    // outside and diffuse inside, filamentary, brighter on the side it drives into (a fixed direction by the seed); the box
    // and the glow's pass reach past it, so it costs no pass of its own. Its pattern is fixed in the world when the caller
    // gives the world's up (`discUp`), else in the view
    double ringA = 0, ringW = 0, bowAng = 0;
    Vec3 r1, r2;
    if (cls == STAR_WOLF_RAYET) {
        ringA = std::max(rA * 5.0, 4.0 * S * invf); ringW = ringA / 7.0;
        outerA = std::min(std::max(outerA, ringA + 3 * ringW), PI - 0.01);
        bowAng = unitFromHash(star.seed ^ 0x5E11ULL) * TAU;
        Vec3 up = discUp ? *discUp : Vec3(0, 1, 0);
        r1 = cross(dirView, up);
        if (length2(r1) < 1e-6) r1 = cross(dirView, Vec3(1, 0, 0));
        r1 = normalize(r1); r2 = cross(dirView, r1);
    }
    // the box: the projection of the cone of half-angle outerA round the star, or the whole frame when the cone
    // reaches the camera plane
    double theta = std::acos(clampd(dirView.z, -1, 1));
    bool centreFront = dirView.z > 1e-6;
    double sx = 0, sy = 0;
    if (centreFront) { sx = pj.cx + pj.f * dirView.x / dirView.z; sy = pj.cy - pj.f * dirView.y / dirView.z; }
    int x0 = 0, x1 = FBW - 1, y0 = 0, y1 = FBH - 1;
    if (centreFront && theta + outerA < 85 * DEG) {
        double rb = pj.f * std::sin(outerA) / (std::cos(theta) * std::cos(theta + outerA)) + 2;
        x0 = std::max(0, (int)(sx - rb)); x1 = std::min(FBW - 1, (int)(sx + rb));
        y0 = std::max(0, (int)(sy - rb)); y1 = std::min(FBH - 1, (int)(sy + rb));
        if (x0 > x1 || y0 > y1) return;
    }
    bool disc = rA * pj.f >= 0.8;   // under 0.8 px the sun is a point
    // a tangent basis for the granulation pattern (6 units across the disc, as the old dx * ns)
    Vec3 e1 = cross(dirView, Vec3(0, 1, 0));
    if (length2(e1) < 1e-6) e1 = cross(dirView, Vec3(1, 0, 0));
    e1 = normalize(e1);
    Vec3 e2 = cross(dirView, e1);
    double cosOuter = std::cos(outerA);
    parallelFor(y1 - y0 + 1, 32, [&](int rb, int re) {
        for (int y = y0 + rb; y < y0 + re; y++) {
            for (int x = x0; x <= x1; x++) {
                Vec3 d((x + 0.5 - pj.cx) * invf, -(y + 0.5 - pj.cy) * invf, 1.0);
                double dl = std::sqrt(dot(d, d));
                double cosA = dot(d, dirView) / dl;
                if (cosA <= cosOuter) continue;
                int o = y * FBW + x;
                if (depthTest && fb.invz[o] > 1e-12f) continue;
                double a = std::acos(clampd(cosA, -1, 1));
                Pix& p = fb.idx[o];
                if (disc && a < rA) {
                    double q = a / rA;
                    double mu = std::sqrt(std::max(0.0, 1.0 - q * q));
                    double limb = 0.55 + 0.45 * mu;
                    double px = dot(d, e1) / dl, py = dot(d, e2) / dl;
                    double pl = std::sqrt(px * px + py * py);
                    double k = pl > 1e-9 ? 6.0 * q / pl : 0.0;
                    double nx = px * k, ny = py * k;
                    double v;
                    if (cls == STAR_RED_GIANT || cls == STAR_ORANGE_GIANT || cls == STAR_CARBON) {   // S-01: the giants' mottled convection; the carbon star's disc dark under its soot
                        double n = gnoise2(nx * 1.5 + t * 0.03, ny * 1.5, star.seed) * 0.5 +
                                   gnoise2(nx * 4 - t * 0.05, ny * 4, star.seed + 3) * 0.3;
                        v = (cls == STAR_CARBON ? 42 : 50) + 14 * n;
                        v *= limb * 0.95 + 0.05;
                    } else if (cls == STAR_ORANGE || cls == STAR_YELLOW || cls == STAR_RED_DWARF) {
                        double n = gnoise2(nx * 3 + t * 0.01, ny * 3, star.seed);
                        double spotFrom = cls == STAR_RED_DWARF ? 0.36 : 0.42;   // S-01: a red dwarf is spotted all over, and its flare whitens the disc
                        double spots = (cls != STAR_YELLOW && n > spotFrom) ? 18 * (n - spotFrom) / 0.3 : 0;
                        v = (58 + 4 * gnoise2(nx * 8, ny * 8 + t * 0.02, star.seed + 1)) * limb - spots * 2.0 + 12 * flare;
                    } else if (cls == STAR_BLUE_GIANT || cls == STAR_BLUE_WHITE || cls == STAR_WOLF_RAYET) {   // S-05: the Wolf-Rayet star's disc is the blue giant's
                        v = 63 * (0.8 + 0.2 * mu);
                    } else if (cls == STAR_PULSAR) {
                        v = 40 + 23 * pulse;
                    } else if (cls == STAR_PROTOSTAR) {   // S-04: a soft disc, strongly limb-darkened, its light through the dust
                        double n = gnoise2(nx * 2 + t * 0.02, ny * 2, star.seed) * 0.5 + gnoise2(nx * 5, ny * 5 - t * 0.03, star.seed + 3) * 0.3;
                        v = (47 + 8 * n) * (0.35 + 0.65 * mu);
                    } else {
                        v = 63 * (0.75 + 0.25 * mu);
                    }
                    v *= intensity;
                    // the disc is never dimmer than the glow-lit sky round it (a dim low sun inside a saturated corona read
                    // as a ring: the glow saturated the sky at 63 while the disc itself sat at 45)
                    double rim = shadeOf(p) + (addOuter + addCorona) * (1.0 - 0.2 * 0.2);
                    if (v < rim) v = rim;
                    p = pix(bank, clampd(v, 0, 63));
                    continue;
                }
                // the glow: an outer disc and the corona, both squared falloffs from the edge of the disc
                int add = 0;
                {
                    double t1 = a <= rA ? 1.0 : 1.0 - (a - rA) / (outerA - rA + 1e-9);
                    if (t1 > 0) add += (int)(addOuter * INTEN_PER_SHADE * t1 * t1 + 0.5);
                    double c0 = rA * 0.8;
                    double t2 = a <= c0 ? 1.0 : 1.0 - (a - c0) / (coronaA - c0 + 1e-9);
                    if (t2 > 0) add += (int)(addCorona * INTEN_PER_SHADE * t2 * t2 + 0.5);
                }
                if (cls == STAR_WOLF_RAYET && a > rA) {   // S-05: the shell's ring
                    double ex = (a - ringA) / ringW;
                    double ring = ex > 0 ? std::exp(-4.0 * ex * ex) : std::exp(-0.5 * ex * ex);
                    if (ring > 0.01) {
                        double px = dot(d, r1) / dl, py = dot(d, r2) / dl;
                        double ang = std::atan2(py, px);
                        double fil = 0.6 + 0.4 * gnoise2(std::cos(ang) * 4.5 + 3.1, std::sin(ang) * 4.5 + ex * 0.7, star.seed + 17);
                        double bow = 0.7 + 0.3 * std::cos(ang - bowAng);
                        add += (int)(intensity * 34 * ring * fil * bow * INTEN_PER_SHADE + 0.5);
                    }
                }
                if (add <= 0) continue;
                if (cls == STAR_PROTOSTAR && a > rA) add = (int)(add * (0.6 + 0.4 * (0.5 + 0.5 * gnoise3(d / dl * 6.0, star.seed + 9))));   // S-04: the cloud's wisps
                int bk = intenOf(p) == 0 ? bank : bankOf(p);
                p = pixI(bk, intenOf(p) + add);
            }
        }
    });
    // S-04: the protostar's dust disc, seen edge-on from the plane of its worlds: a band of glow along the orbital plane through
    // the star, narrow at the star and flaring to 0.05 + 0.3 a (radians) at the angle a from it, gone 0.6 rad out, with the dark
    // lane of the disc's own shadow along its middle. Its own pass over the frame, skipping cheaply every pixel outside the strip
    if (cls == STAR_PROTOSTAR && discUp) {
        const Vec3 n = *discUp;
        const double cosBand = std::cos(0.6);
        parallelFor(FBH, 32, [&](int rb, int re) {
            for (int y = rb; y < re; y++) {
                for (int x = 0; x < FBW; x++) {
                    Vec3 d((x + 0.5 - pj.cx) * invf, -(y + 0.5 - pj.cy) * invf, 1.0);
                    double il = 1.0 / std::sqrt(dot(d, d));
                    double hs = dot(d, n) * il;
                    if (hs > 0.3 || hs < -0.3) continue;
                    double cosA = dot(d, dirView) * il;
                    if (cosA <= cosBand) continue;
                    int o = y * FBW + x;
                    if (depthTest && fb.invz[o] > 1e-12f) continue;
                    double a = std::acos(clampd(cosA, -1, 1));
                    if (a <= rA) continue;
                    double hAng = std::asin(clampd(hs, -1, 1));
                    double hw = 0.05 + 0.3 * a, along = 1.0 - a / 0.6;
                    double g = std::exp(-(hAng * hAng) / (hw * hw)) * along;
                    double lw = 0.012 + 0.06 * a;
                    g *= 1 - 0.65 * std::exp(-(hAng * hAng) / (lw * lw)) * smoothstep(0.0, 0.08, a);
                    g *= 0.8 + 0.2 * (0.5 + 0.5 * gnoise3(d * il * 9.0, star.seed + 11));
                    int add = (int)(intensity * 24 * g * INTEN_PER_SHADE + 0.5);
                    if (add <= 0) continue;
                    Pix& p = fb.idx[o];
                    int bk = intenOf(p) == 0 ? bank : bankOf(p);
                    p = pixI(bk, intenOf(p) + add);
                }
            }
        });
    }
    if (!centreFront) return;
    // rays for compact stars, from the projected centre, outside the disc
    if (cls == STAR_WHITE_DWARF || cls == STAR_PULSAR || cls == STAR_BLUE_GIANT || cls == STAR_BLUE_WHITE || cls == STAR_WOLF_RAYET) {   // S-01: the blue-white star's cross of four; S-05: the Wolf-Rayet star's too
        double coronaPx = std::min(pj.f * std::tan(std::min(coronaA, 1.4)), (double)FBW);
        double rPx = pj.f * std::tan(std::min(rA, 1.4));
        double len = coronaPx * ((cls == STAR_BLUE_GIANT || cls == STAR_BLUE_WHITE || cls == STAR_WOLF_RAYET) ? 1.4 : 2.2) * (cls == STAR_PULSAR ? pulse : 1.0);
        int arms = (cls == STAR_WHITE_DWARF || cls == STAR_BLUE_WHITE || cls == STAR_WOLF_RAYET) ? 4 : (cls == STAR_BLUE_GIANT ? 3 : 2);
        double rot = cls == STAR_PULSAR ? t * 0.8 : 0.0;
        for (int a = 0; a < arms; a++) {
            double ang = rot + a * PI / arms * 2.0 / (arms == 4 ? 2.0 : 1.0);
            double dx = std::cos(ang), dy = std::sin(ang);
            int n = std::min((int)len, 2 * FBW);
            for (int i = std::max(1, (int)rPx); i < n; i++) {
                double f = 1.0 - (double)i / n;
                int x = (int)(sx + dx * i), y = (int)(sy + dy * i);
                int x2 = (int)(sx - dx * i), y2 = (int)(sy - dy * i);
                int add = (int)(40 * f * f * intensity);
                for (int k = 0; k < 2; k++) {
                    int px = k ? x2 : x, py = k ? y2 : y;
                    if (px < 0 || py < 0 || px >= FBW || py >= FBH) continue;
                    if (depthTest && fb.invz[py * FBW + px] > 1e-12f) continue;
                    Pix& p = fb.idx[py * FBW + px];
                    int bk = intenOf(p) == 0 ? bank : bankOf(p);
                    p = pixI(bk, intenOf(p) + toInten(add));
                }
            }
        }
    }
    if (!disc) {
        int x = (int)sx, y = (int)sy;
        for (int dy = 0; dy < FB_SCALE; dy++)
            for (int dx = 0; dx < FB_SCALE; dx++)
                if (x + dx >= 0 && y + dy >= 0 && x + dx < FBW && y + dy < FBH && !(depthTest && fb.invz[(y + dy) * FBW + x + dx] > 1e-12f)) fb.idx[(y + dy) * FBW + x + dx] = pix(bank, clampd(63 * intensity, 0, 63));
    }
}

void SpaceRenderer::drawLensFlare(Framebuffer& fb, double sx, double sy, double sunRadiusPx, double intensity, int bank, const Proj& pj) {
    double r = std::min(sunRadiusPx, 0.2 * FBW);   // B-308: a sun close to the camera plane projects to an absurd radius
    if (sx < -FBW * 0.15 || sx > FBW * 1.15 || sy < -FBH * 0.15 || sy > FBH * 1.15) return;
    double dx = pj.cx - sx, dy = pj.cy - sy;
    double S = FB_SCALE;
    const double ks[4] = {0.45, 0.8, 1.2, 1.6};
    const double rad[4] = {0.45, 0.8, 0.3, 1.1};
    const int add[4] = {6, 4, 5, 3};
    for (int i = 0; i < 4; i++) {
        double px = sx + dx * ks[i], py = sy + dy * ks[i];
        double rr = std::max(2.0 * S, r * rad[i]) + 2 * S;
        fb.glowDisc(px, py, rr, rr * 0.3, (int)(add[i] * intensity + 0.5), bank);
    }
    // a faint streak through the sun along the same line
    int n = (int)(std::sqrt(dx * dx + dy * dy) * 0.5);
    double nx = dx, ny = dy, nl = std::sqrt(nx * nx + ny * ny) + 1e-9;
    nx /= nl; ny /= nl;
    for (int i = -n; i <= n; i++) {
        double f = 1.0 - std::fabs((double)i) / (n + 1);
        int x = (int)(sx + nx * i * 2), y = (int)(sy + ny * i * 2);
        if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue;
        Pix& p = fb.idx[y * FBW + x];
        if (fb.invz[y * FBW + x] > 1e-12f) continue;
        int bk = intenOf(p) == 0 ? bank : bankOf(p);
        p = pixI(bk, intenOf(p) + toInten(3.0 * f * f * intensity));
    }
}

// B-312: the screen box of a sphere of radius R at the view-space centre c: the exact extents of its projection (the
// tangent planes through the camera's axes; on the plane z = 1 the extents are (a cz +- R sqrt(a^2 + cz^2 - R^2)) /
// (cz^2 - R^2) for a = cx and a = cy), or the whole frame when the sphere reaches the camera plane
static void sphereBox(const Vec3& c, double R, const Proj& pj, int& x0, int& x1, int& y0, int& y1) {
    x0 = 0; x1 = FBW - 1; y0 = 0; y1 = FBH - 1;
    double R2 = R * R;
    if (c.z <= 0 || c.z * c.z <= R2 * 1.0001) return;
    double d = c.z * c.z - R2;
    auto ext = [&](double a, double& lo, double& hi) {
        double s = std::sqrt(std::max(0.0, a * a + c.z * c.z - R2));
        lo = (a * c.z - R * s) / d; hi = (a * c.z + R * s) / d;
    };
    double xl, xh, yl, yh;
    ext(c.x, xl, xh); ext(c.y, yl, yh);
    x0 = std::max(0, (int)std::floor(pj.cx + pj.f * xl) - 1); x1 = std::min(FBW - 1, (int)std::ceil(pj.cx + pj.f * xh) + 1);
    y0 = std::max(0, (int)std::floor(pj.cy - pj.f * yh) - 1); y1 = std::min(FBH - 1, (int)std::ceil(pj.cy - pj.f * yl) + 1);
}

// B-312: the screen box of a ring's outer circle (radius ringR1 round c in the plane normal to axisV), from 48 points
// of it; any point at or behind the camera plane makes it the whole frame. The projection of a circle is an ellipse,
// convex, so the chords hug it within a fraction of a percent; a margin covers that
static void ringBox(const Vec3& c, const Vec3& axisV, double ringR1, const Proj& pj, int& x0, int& x1, int& y0, int& y1) {
    x0 = 0; x1 = FBW - 1; y0 = 0; y1 = FBH - 1;
    Vec3 u = normalize(cross(axisV, std::fabs(axisV.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)));
    Vec3 v = cross(axisV, u);
    double xl = 1e18, xh = -1e18, yl = 1e18, yh = -1e18;
    for (int k = 0; k < 48; k++) {
        double a = k * TAU / 48;
        Vec3 p = c + (u * std::cos(a) + v * std::sin(a)) * ringR1;
        if (p.z <= ringR1 * 1e-4) return;
        double sx = pj.cx + pj.f * p.x / p.z, sy = pj.cy - pj.f * p.y / p.z;
        xl = std::min(xl, sx); xh = std::max(xh, sx); yl = std::min(yl, sy); yh = std::max(yh, sy);
    }
    double mx = (xh - xl) * 0.01 + 2, my = (yh - yl) * 0.01 + 2;
    x0 = std::max(0, (int)std::floor(xl - mx)); x1 = std::min(FBW - 1, (int)std::ceil(xh + mx));
    y0 = std::max(0, (int)std::floor(yl - my)); y1 = std::min(FBH - 1, (int)std::ceil(yh + my));
}

// ---------------------------------------------------------------------------
// W-06: the telescope's plate (see space_view.h)
// ---------------------------------------------------------------------------

constexpr int DetailPlate::BLOCKS[4];

double DetailPlate::progress() const {
    if (!valid()) return 0;
    if (done()) return 1;
    static const double cum[5] = {0, 1.0 / 64, 4.0 / 64, 16.0 / 64, 1.0};
    int L = BLOCKS[pass], bw = (W + L - 1) / L, bh = (H + L - 1) / L;
    return cum[pass] + (cum[pass + 1] - cum[pass]) * (double)next / std::max(1, bw * bh);
}

Vec3 SpaceRenderer::plateCellUnit(const DetailPlate& p, int i, int j) {
    double a = (i - p.W * 0.5 + 0.5) * p.texel, b = (j - p.H * 0.5 + 0.5) * p.texel, r2 = a * a + b * b;
    if (r2 >= 0.9999) return Vec3(0, 0, 0);
    return normalize(p.u0 * std::sqrt(1 - r2) + p.e1 * a + p.e2 * b);
}

namespace {
// a body-frame point turned about the spin axis (the jets' and the clouds' drift are turns of the lookup, the plate being fixed)
inline Vec3 rotZ(const Vec3& v, double ang) { double cs = std::cos(ang), sn = std::sin(ang); return Vec3(v.x * cs - v.y * sn, v.x * sn + v.y * cs, v.z); }
// the plate's cell under a body-frame point: the cell coordinates (a cell's centre at integer fx, fy), the nearest cell,
// filled and on the sphere, else no hit
struct PlateHit { const DetailPlate* p = nullptr; double fx = 0, fy = 0; int in = 0, jn = 0; bool ok = false; };
inline PlateHit plateFind(const DetailPlate& p, const Vec3& bf) {
    PlateHit h;
    if (dot(bf, p.u0) <= 0.02) return h;
    double fx = dot(bf, p.e1) / p.texel + p.W * 0.5 - 0.5, fy = dot(bf, p.e2) / p.texel + p.H * 0.5 - 0.5;
    if (!(fx >= 0 && fy >= 0 && fx < p.W - 1 && fy < p.H - 1)) return h;
    h.fx = fx; h.fy = fy; h.in = (int)(fx + 0.5); h.jn = (int)(fy + 0.5);
    size_t o = (size_t)h.jn * p.W + h.in;
    if (!p.level[o] || p.material[o] == 255) return h;
    h.p = &p; h.ok = true;
    return h;
}
// the four samples round the point at the nearest cell's block step (a coarse pass interpolates between the blocks'
// own samples, so the picture is smooth at every pass), all filled and on the sphere, else false
inline bool plateQuad(const PlateHit& h, size_t o[4], double& tx, double& ty) {
    const DetailPlate& p = *h.p;
    int L = p.level[(size_t)h.jn * p.W + h.in];
    double u = h.fx / L, v = h.fy / L;
    int m = (int)std::floor(u), n = (int)std::floor(v);
    tx = u - m; ty = v - n;
    int i0 = m * L, i1 = i0 + L, j0 = n * L, j1 = j0 + L;
    if (i0 < 0 || j0 < 0 || i1 >= p.W || j1 >= p.H) return false;
    o[0] = (size_t)j0 * p.W + i0; o[1] = o[0] + L; o[2] = (size_t)j1 * p.W + i0; o[3] = o[2] + L;
    for (int k = 0; k < 4; k++) if (!p.level[o[k]] || p.material[o[k]] == 255) return false;
    return true;
}
inline double plateLerp4(const std::vector<uint8_t>& v, const size_t o[4], double tx, double ty) {
    double a = v[o[0]] + (v[o[1]] - v[o[0]]) * tx, b = v[o[2]] + (v[o[3]] - v[o[2]]) * tx;
    return (a + (b - a) * ty) / 255.0;
}
inline double plateAlbedo(const PlateHit& h) { size_t o[4]; double tx, ty; if (plateQuad(h, o, tx, ty)) return plateLerp4(h.p->albedo, o, tx, ty); return h.p->albedo[(size_t)h.jn * h.p->W + h.in] / 255.0; }
inline double plateCloud(const PlateHit& h) { size_t o[4]; double tx, ty; if (plateQuad(h, o, tx, ty)) return plateLerp4(h.p->cloud, o, tx, ty); return h.p->cloud[(size_t)h.jn * h.p->W + h.in] / 255.0; }
inline double plateVeg(const PlateHit& h) { return h.p->veg[(size_t)h.jn * h.p->W + h.in] / 255.0; }
inline int plateMaterial(const PlateHit& h) { return h.p->material[(size_t)h.jn * h.p->W + h.in]; }
// the slopes (rise over run) along e1 and e2 over the nearest cell's block step
inline void plateSlopes(const PlateHit& h, double& ga, double& gb) {
    const DetailPlate& p = *h.p;
    size_t on = (size_t)h.jn * p.W + h.in;
    int L = p.level[on];
    double h0 = p.height[on];
    auto hAt = [&](int i, int j) { size_t o = (size_t)j * p.W + i; return p.level[o] && p.material[o] != 255 ? (double)p.height[o] : h0; };
    int iL = std::max(0, h.in - L), iR = std::min(p.W - 1, h.in + L), jD = std::max(0, h.jn - L), jU = std::min(p.H - 1, h.jn + L);
    ga = iR > iL ? (hAt(iR, h.jn) - hAt(iL, h.jn)) / ((iR - iL) * p.texelM) : 0.0;
    gb = jU > jD ? (hAt(h.in, jU) - hAt(h.in, jD)) / ((jU - jD) * p.texelM) : 0.0;
}
}

void SpaceRenderer::plateBegin(DetailPlate& p, int bi, uint64_t seed, int bucket, const Vec3& u0, const Vec3& e1, const Vec3& e2, double texel, double R, double cloudDrift, double halfA, double halfB) {
    plateBegun++;
    p = DetailPlate();
    p.body = bi; p.seed = seed; p.seasonBucket = bucket; p.u0 = u0; p.e1 = e1; p.e2 = e2;
    int W = (int)std::ceil(2 * halfA / texel) + 2, H = (int)std::ceil(2 * halfB / texel) + 2;
    const double CAP = 900000;   // cells: 2x's frame with its margin and an oblique view's stretch (3x and 4x read it at 2x's step); a wider field (the limb at a deep power) takes a coarser step
    if ((double)W * H > CAP) { double sc = std::sqrt(CAP / ((double)W * H)); texel /= sc; W = (int)std::ceil(2 * halfA / texel) + 2; H = (int)std::ceil(2 * halfB / texel) + 2; }
    W = std::max(8, (W + 7) / 8 * 8); H = std::max(8, (H + 7) / 8 * 8);
    p.W = W; p.H = H; p.texel = texel; p.texelM = texel * R * 1000.0;
    size_t n = (size_t)W * H;
    p.height.assign(n, 0.f); p.material.assign(n, 0); p.albedo.assign(n, 0); p.cloud.assign(n, 0); p.veg.assign(n, 0); p.level.assign(n, 0); p.exact.assign(n, 0);
    p.cloudDrift0 = cloudDrift;
    p.radiusM = 0.5 * std::sqrt((double)W * W + (double)H * H) * p.texelM;
}

// the build: the block passes in order, the blocks of a pass in rows, `budget` samples at most (0: all of it)
void SpaceRenderer::plateFill(DetailPlate& p, const BodyGen& g, int budget) {
    long left = budget > 0 ? budget : (1L << 40);
    while (!p.done() && left > 0) {
        int L = DetailPlate::BLOCKS[p.pass];
        int bw = (p.W + L - 1) / L, bh = (p.H + L - 1) / L, nBlocks = bw * bh;
        if (p.next >= nBlocks) { p.pass++; p.next = 0; continue; }
        int count = (int)std::min<long>(nBlocks - p.next, left), first = p.next;
        std::atomic<long> taken{0};
        bool noDrain = p.noDrainage;
        parallelFor(count, 32, [&](int k0, int k1) {
            DrainageOff off(noDrain);
            long mine = 0;
            for (int k = k0; k < k1; k++) {
                int blk = first + k, i = (blk % bw) * L, j = (blk / bw) * L;
                size_t o = (size_t)j * p.W + i;
                if (!p.exact[o]) {
                    double a = (i - p.W * 0.5 + 0.5) * p.texel, b = (j - p.H * 0.5 + 0.5) * p.texel, r2 = a * a + b * b;
                    if (r2 < 0.9999) {
                        Vec3 u = normalize(p.u0 * std::sqrt(1 - r2) + p.e1 * a + p.e2 * b);
                        SurfaceSample s = sampleSurface(g, u, p.texelM);
                        double lat, lon; StarSystem::latLonFromBody(u, lat, lon);
                        double alb = s.albedo;
                        if (g.type == PT_GASGIANT && std::fabs(lat) > 76 * DEG) alb *= 1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat));   // M9-10: the polar vortex, as the map has it
                        p.height[o] = (float)s.height; p.material[o] = (uint8_t)s.material;
                        p.albedo[o] = (uint8_t)clampi((int)(alb * 255 + 0.5), 0, 255);
                        p.veg[o] = (uint8_t)clampi((int)(s.veg * 255 + 0.5), 0, 255);
                        p.cloud[o] = (uint8_t)clampi((int)(sampleCloudPattern(g, lon + p.cloudDrift0, lat) * 255 + 0.5), 0, 255);
                        mine++;
                    } else p.material[o] = 255;   // off the sphere
                    p.exact[o] = 1;
                }
                p.level[o] = (uint8_t)L;
                int i1 = std::min(i + L, p.W), j1 = std::min(j + L, p.H);
                for (int jj = j; jj < j1; jj++)
                    for (int ii = i; ii < i1; ii++) {
                        size_t q = (size_t)jj * p.W + ii;
                        if (q == o || p.exact[q]) continue;
                        p.height[q] = p.height[o]; p.material[q] = p.material[o]; p.albedo[q] = p.albedo[o]; p.cloud[q] = p.cloud[o]; p.veg[q] = p.veg[o]; p.level[q] = (uint8_t)L;
                    }
            }
            taken += mine;
        });
        p.samples += taken; plateSamplesFrame += taken; p.next += count; left -= count;
    }
}

// every frame: the plate the eyepiece needs for `c.detailBody` at the aim point's footprint; the current plate kept while it
// fits (the body, the season, the step within a quarter, the field inside it: the whole near hemisphere when it holds it,
// within forty degrees of its centre), else the other slot checked for a fit (a step out after a step in), else a new
// plate begun in the other slot (the old one staying as the fallback); then the build within the budget
void SpaceRenderer::updatePlate(const SpaceContext& c) {
    plateSamplesFrame = 0;
    int bi = c.detailBody;
    if (!c.sys || !c.sys->valid || c.skyMode || bi < 0 || bi >= (int)c.sys->bodies.size()) return;
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    if (b.type == PT_COMPANION) return;
    Vec3 posW = sys.bodyPos(bi, c.t), cv = c.cam * (posW - c.shipPos);
    double dist = length(cv), R = b.radiusKm;
    if (cv.z <= 0 || dist <= R * 1.0001) return;
    double angR = std::asin(clampd(R / dist, 0, 1)), rpx = proj.f * std::tan(angR) * (dist / cv.z);
    double texelPx = FB_SCALE >= 3 ? FB_SCALE * 0.5 : 1.0;   // at 3x and 4x the plate is 2x's: the mush melts the rest
    if (rpx < 8 * texelPx) return;   // a dot: the map is enough
    // the aim point: where the central ray meets the sphere, else the point of the sphere nearest the ray (the limb beside the reticle)
    double perp2 = cv.x * cv.x + cv.y * cv.y;
    Vec3 nV; double dHit;
    if (perp2 < R * R) { double tHit = cv.z - std::sqrt(R * R - perp2); nV = (Vec3(0, 0, tHit) - cv) / R; dHit = tHit; }
    else { nV = normalize(Vec3(0, 0, cv.z) - cv); dHit = length(cv + nV * R); }
    Mat3 frame = sys.bodyFrame(bi, c.t), camT = c.cam.transposed();
    Vec3 u0 = normalize(frame * (camT * nV));
    double texel = (dHit / proj.f) * texelPx / R;   // a pixel's footprint at the aim point, in radii
    if (texel * R * 1000.0 >= 0.75 * TAU * R * 1000.0 / PlanetMap::W) return;   // the map's texel is finer than the eyepiece's: the map serves (a disc under about 80 px at 1x)
    // the axes along the screen's (the field then lies along the grid); a degenerate one (the limb beside the reticle seen
    // edge-on) falls back on the spin axis
    Vec3 ex = frame * (camT * Vec3(1, 0, 0)); ex = ex - u0 * dot(ex, u0);
    if (length(ex) < 0.3) { Vec3 ax = std::fabs(u0.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0); ex = cross(ax, u0); }
    Vec3 e1 = normalize(ex), e2 = cross(u0, e1);
    // the field's points on the sphere (the corners, the edges' middles, the centre): where the pixel's ray meets it, else
    // the point of the sphere nearest the ray (the limb beside it), in the body's frame; the plate must hold them all, so an
    // oblique view (the ground stretched along one axis) and the field's own turn are sized for, and a limb point counts
    // only up to two and a half fields (looking past the limb must not ask for the hemisphere at a fine step)
    struct FieldPt { Vec3 n; bool hit; };
    FieldPt fp[9]; int nfp = 0;
    const double pxs[3] = {0.5, FBW - 0.5, FBW * 0.5}, pys[3] = {0.5, FBH - 0.5, FBH * 0.5};
    for (int iy = 0; iy < 3; iy++)
        for (int ix = 0; ix < 3; ix++) {
            Vec3 d = normalize(Vec3((pxs[ix] - proj.cx) / proj.f, -(pys[iy] - proj.cy) / proj.f, 1.0));
            double tc = dot(d, cv), perp2 = dot(cv, cv) - tc * tc;
            bool hit = tc > 0 && perp2 < R * R;
            Vec3 nV = hit ? (d * (tc - std::sqrt(R * R - perp2)) - cv) / R : normalize(d * std::max(tc, 0.0) - cv);
            fp[nfp++] = {normalize(frame * (camT * nV)), hit};
        }
    double needA = 0.5 * FBW / texelPx * texel, needB = 0.5 * FBH / texelPx * texel;   // the field's half extent in radii, face on
    auto extent = [&](const Vec3& ea, const Vec3& eb, double& extA, double& extB) {
        extA = 0; extB = 0;
        for (int k = 0; k < nfp; k++) {
            double a = std::fabs(dot(fp[k].n, ea)), b = std::fabs(dot(fp[k].n, eb));
            if (!fp[k].hit) { a = std::min(a, 2.5 * needA); b = std::min(b, 2.5 * needB); }
            extA = std::max(extA, a); extB = std::max(extB, b);
        }
    };
    int bucket = bi < 64 ? seasonBucket[bi] : 0;
    const BodyGen& g = genFor(b);
    double cloudDrift = c.t * TAU / (b.rotPeriod * 3.7) + 1.0;
    auto fits = [&](const DetailPlate& p) {
        if (!p.valid() || p.body != bi || p.seed != b.seed || p.seasonBucket != bucket || std::fabs(p.texel / texel - 1) >= 0.25) return false;
        if (dot(u0, p.u0) <= 0.77) return false;   // the aim within forty degrees of the plate's centre
        double extA, extB; extent(p.e1, p.e2, extA, extB);
        if (extA > 0.5 * p.W * p.texel || extB > 0.5 * p.H * p.texel) return false;   // the field left the plate
        for (int k = 0; k < nfp; k++) if (fp[k].hit && dot(fp[k].n, p.u0) <= 0.05) return false;
        if (p.noDrainage && p.drainageWanted && drainageReady(g, p.u0, p.radiusM)) return false;   // the rivers' tiles are in: again with them
        return true;
    };
    if (!fits(plates[plateCur])) {
        if (fits(plates[1 - plateCur])) plateCur = 1 - plateCur;
        else {
            plateCur = 1 - plateCur;
            DetailPlate& q = plates[plateCur];
            double extA, extB; extent(e1, e2, extA, extB);
            plateBegin(q, bi, b.seed, bucket, u0, e1, e2, texel, R, cloudDrift, std::min(1.0, extA * 1.25 + 4 * texel), std::min(1.0, extB * 1.25 + 4 * texel));
            // the rivers and the lakes: their tiles for the plate's ground when the step is under the drainage's own limit and
            // the ground is a few tiles (a hemisphere at 1500 m a step would be a hundred); built now for a photo or a harness
            // frame, in the background otherwise (the plate sampled without them meanwhile and begun again when they are in)
            q.drainageWanted = typeHasDrainage(b.type) && q.texelM < 2048 && q.radiusM <= 200000 && drainageEnabled();
            q.noDrainage = !q.drainageWanted;   // a wider plate samples without the drainage outright: a sample would build its tile (a second each) on the spot
            if (q.drainageWanted && !drainageReady(g, u0, q.radiusM)) {
                if (c.detailBudget <= 0) drainagePrefetch(g, u0, q.radiusM, false);
                else { drainagePrefetch(g, u0, q.radiusM, true); q.noDrainage = true; }
            }
        }
    }
    plateFill(plates[plateCur], g, c.detailBudget);
}

std::string SpaceRenderer::plateInfo() const {
    const DetailPlate& p = plates[plateCur];
    if (!p.valid()) return "no plate";
    char buf[320];
    int L = p.done() ? 1 : DetailPlate::BLOCKS[p.pass];
    snprintf(buf, sizeof buf, "plate: body %d, %d x %d cells of %.0f m (%.3g radii), %s, %ld samples, %.0f%%%s", p.body, p.W, p.H, p.texelM, p.texel,
             p.done() ? "done" : (std::string("blocks of ") + std::to_string(L) + " " + std::to_string(p.next) + "/" + std::to_string(((p.W + L - 1) / L) * ((p.H + L - 1) / L))).c_str(),
             p.samples, 100 * p.progress(), p.noDrainage && p.drainageWanted ? ", no rivers yet" : "");
    std::string s = buf;
    if (p.samples > 0) {   // the heights' range and the slopes over the cells' own step (rise over run), for the harness
        double lo = 1e18, hi = -1e18, sl = 0; long n = 0;
        for (int j = 1; j < p.H - 1; j++)
            for (int i = 1; i < p.W - 1; i++) {
                size_t o = (size_t)j * p.W + i;
                if (!p.exact[o] || p.material[o] == 255) continue;
                lo = std::min(lo, (double)p.height[o]); hi = std::max(hi, (double)p.height[o]);
                int L = p.level[o];
                if (i + L < p.W && p.exact[(size_t)j * p.W + i + L] && p.material[(size_t)j * p.W + i + L] != 255) { sl += std::fabs(p.height[(size_t)j * p.W + i + L] - p.height[o]) / (L * p.texelM); n++; }
            }
        if (n) { snprintf(buf, sizeof buf, ", heights %.0f..%.0f m, mean slope %.3f", lo, hi, sl / n); s += buf; }
    }
    const DetailPlate& q = plates[1 - plateCur];
    if (q.valid() && q.body == p.body && q.seed == p.seed) { snprintf(buf, sizeof buf, "; the previous %d x %d at %.0f m, %.0f%%", q.W, q.H, q.texelM, 100 * q.progress()); s += buf; }
    return s;
}

void SpaceRenderer::drawGlobe(Framebuffer& fb, const SpaceContext& c, int bi, int bank, BodyScreenInfo& info) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    Vec3 posW = sys.bodyPos(bi, c.t);
    Vec3 relW = posW - c.shipPos;
    Vec3 cv = c.cam * relW;             // view-space centre
    double dist = length(cv);
    info.body = bi;
    info.distKm = dist;
    info.inFront = cv.z > 0;
    double R = b.radiusKm;
    // O0-03 (B-304): a globe is skipped only when the whole sphere lies behind the camera plane. It used to be skipped as
    // soon as its centre did, so a huge parent in a moon's sky vanished the moment the camera looked down while its
    // limb still filled the top of the frame. With the centre behind, the rays are tested over the whole frame.
    if (cv.z + R <= 0) { info.radiusPx = 0; return; }
    double cz = cv.z;
    double angR = std::asin(clampd(R / dist, 0, 1));
    double rpx;
    if (cz > 1e-3 * R) {
        info.sx = proj.cx + proj.f * cv.x / cz;
        info.sy = proj.cy - proj.f * cv.y / cz;
        rpx = proj.f * std::tan(angR) * (dist / cz);
    } else {
        info.sx = proj.cx; info.sy = proj.cy; rpx = 1e9;   // the centre is behind or beside the camera: no projection, the box is the frame
    }
    info.radiusPx = rpx;
    if (b.type == PT_COMPANION) {
        // M5-01: the second sun; the surface draws its own (see SurfaceView::drawSky), space draws it here, depth-tested
        if (c.skyMode) { info.radiusPx = 0; return; }
        Star ks = sys.companionStar();
        double inten = clampd(0.6 + 0.4 * std::log10(1 + ks.luminosity), 0.7, 1.0);
        if (sys.star.cls == STAR_BLACK_HOLE) drawAccretionStream(fb, c, bi, 12);   // S-06: drawn out: its gas streams to the hole's disc, under its own disc
        drawSun(fb, ks, cv / dist, angR, c.t, inten, 12, false, 0, true, proj);   // B-308: by angle, whatever the size
        return;
    }
    StarSystem::Light light = sys.lightAt(posW, c.t);   // M5-01: bodies near the companion are lit by it
    Vec3 starDirW = light.dir;
    double lf = clampd(light.factor, 0, 1.15);
    if (b.type == PT_COMET) drawCometTail(fb, c, bi, c.skyMode ? 1 : bank, rpx);
    if ((b.type == PT_GASGIANT || b.type == PT_SUBSTELLAR) && b.parent < 0 && sys.star.cls == STAR_WOLF_RAYET) drawStrippedTail(fb, c, bi, c.skyMode ? 1 : bank);   // S-05: its envelope streams away in the wind
    const PlanetTypeInfo& pt = PLANET_TYPES[b.type];
    // B-317: in a sky the depth is written in metres, the terrain's unit (it was the kilometre: a moon 20,000 km away read
    // as 20 km, so hills beyond that failed the depth test against it and the moon stood in the ground); in space the
    // kilometre stays, since the whole scene is in kilometres
    const double depthUnit = c.skyMode ? 1000.0 : 1.0;
    if (rpx < 1.2 * FB_SCALE && cz > 0) {
        // far body: a point like a bright star. In a sky (M5-09) it is a wandering star: a scale+1 square
        // that survives the mush, bright with phase, albedo and closeness, hidden where the sky is brighter
        Vec3 toShip = normalize(c.shipPos - posW);
        double phase = 0.5 + 0.5 * dot(toShip, starDirW);
        double v = c.skyMode ? clampd((30 + 26 * phase * pt.albedo * lf + 8 * (rpx / (1.2 * FB_SCALE))) * (0.35 + 0.65 * c.skyDark), 6, 60)
                             : clampd(18 + 30 * phase * pt.albedo * lf + 10 * (rpx / (1.2 * FB_SCALE)), 6, 55);
        if (b.type == PT_SUBSTELLAR) v = std::max(v, 40.0);
        if (b.type == PT_COMET) v = std::max(v, 36.0 * c.skyDark);
        int side = c.skyMode ? FB_SCALE + 1 : FB_SCALE;
        int x = (int)info.sx - (side - FB_SCALE) / 2, y = (int)info.sy - (side - FB_SCALE) / 2;
        for (int dy = 0; dy < side; dy++)
            for (int dx = 0; dx < side; dx++) {
                int px = x + dx, py = y + dy;
                if (px < 0 || py < 0 || px >= FBW || py >= FBH) continue;
                int o = py * FBW + px;
                if (shadeOf(fb.idx[o]) < v) { fb.idx[o] = pix(bank, v); fb.invz[o] = (float)(1.0 / (cz * depthUnit)); }
            }
        return;
    }
    const PlanetMap* mapPtr = mapIfReady(b);   // drawn flat until the background generation is done (M9-15)
    const bool haveMap = mapPtr != nullptr;
    static const PlanetMap emptyMap;
    const PlanetMap& map = haveMap ? *mapPtr : emptyMap;
    const BodyGen& bgen = genFor(b);
    Mat3 frame = sys.bodyFrame(bi, c.t);
    Mat3 camT = c.cam.transposed();
    Mat3 frameT = frame.transposed();
    // W-06: the telescope's plates for this body (the current, then the previous as the fallback), their axes in the world
    // and the weight of their relief shading (the map's is 5 over its 50 km, where the function's slopes are a few
    // thousandths; a plate's slopes at a few hundred metres are a tenth, so the weight runs from 2.5 there to 6 at 1.5 km)
    const DetailPlate* pl[2] = {nullptr, nullptr}; Vec3 plE1W[2], plE2W[2]; double plK[2] = {0, 0}; int npl = 0;
    if (bi == c.detailBody && !c.skyMode)
        for (int k = 0; k < 2; k++) {
            const DetailPlate& q = plates[k == 0 ? plateCur : 1 - plateCur];
            if (!q.valid() || q.body != bi || q.seed != b.seed || q.samples <= 0) continue;
            pl[npl] = &q; plE1W[npl] = frameT * q.e1; plE2W[npl] = frameT * q.e2; plK[npl] = 2.5 + 3.5 * smoothstep(200.0, 1500.0, q.texelM); npl++;
        }
    bool atmosphere = pt.atmosphere;
    double bubble = atmosphere ? 1.045 : 1.0;
    const FrontMap* fmap = (atmosphere && !hasOpaqueDeck(b.type) && worldHasFronts(b)) ? frontsFor(b, c.t) : nullptr;   // W-03: the fronts' bands
    const int frontCW = PlanetMap::W / FRONT_BLOCK, frontCH = PlanetMap::H / FRONT_BLOCK;
    const bool dustFronts = b.type == PT_DESERT || b.type == PT_THINATMO;
    // B-312: the box is the exact screen extent of the sphere (its projection is a conic that reaches farther from the
    // projected centre the nearer the centre is to the edge of the view: a box of radius rpx round the centre cut the
    // limb off, and a huge parent vanished from a moon's sky as soon as its centre left the frame to the left or the
    // right), the whole frame when the sphere reaches the camera plane, and the projected outer circle of the rings
    int x0, x1, y0, y1;
    sphereBox(cv, R * bubble, proj, x0, x1, y0, y1);
    double ambient = atmosphere ? 0.10 : 0.05;
    // W-04: the earthshine: a moon's night side lit by its parent's day side (`bodyShineAt`, the light the ground's planetshine
    // reads at night), so a thin crescent shows the rest of its disc faintly and a moon's night seen from space is its parent's
    double shineP = 0; Vec3 shineDirW;
    if (b.parent >= 0 && b.parent < (int)sys.bodies.size()) {
        shineP = bodyShineAt(sys, b.parent, posW, c.t, lf);
        shineDirW = normalize(sys.bodyPos(b.parent, c.t) - posW);
    }
    double cloudDrift = c.t * TAU / (b.rotPeriod * 3.7) + 1.0;
    double R2 = R * R;
    double c2 = dot(cv, cv);
    double rings = b.rings ? 1 : 0;
    Vec3 axisV = c.cam * b.spinAxis;
    const std::vector<float>* prof = b.rings ? &ringProfile(sys, bi) : nullptr;
    double ringR0 = b.ringInner * R, ringR1 = b.ringOuter * R;
    if (b.rings) {
        int rx0, rx1, ry0, ry1;
        ringBox(cv, axisV, ringR1, proj, rx0, rx1, ry0, ry1);
        x0 = std::min(x0, rx0); x1 = std::max(x1, rx1); y0 = std::min(y0, ry0); y1 = std::max(y1, ry1);
    }
    if (x0 > x1 || y0 > y1) return;
    // M1-08: moons and the parent cast shadows on this globe; rings shade it too
    struct Occ { Vec3 pos; double r; };
    std::vector<Occ> occ;
    if (!c.skyMode)
        for (int j = 0; j < (int)sys.bodies.size(); j++) {
            if (j == bi) continue;
            if (sys.bodies[j].parent == bi || j == b.parent) occ.push_back({sys.bodyPos(j, c.t), sys.bodies[j].radiusKm});
        }
    double invf = 1.0 / proj.f;
    parallelFor(y1 - y0 + 1, 48, [&](int rb, int re) {
    for (int y = y0 + rb; y < y0 + re; y++) {
        for (int x = x0; x <= x1; x++) {
            Vec3 d((x + 0.5 - proj.cx) * invf, -(y + 0.5 - proj.cy) * invf, 1.0);
            double dl = length(d);
            d = d / dl;
            double tc = dot(d, cv);
            double disc = tc * tc - c2 + R2;
            int o = y * FBW + x;
            bool hitSphere = disc > 0 && tc > 0;
            double tHit = 0;
            double sphereVal = -1;
            if (hitSphere) {
                tHit = tc - std::sqrt(disc);
                Vec3 p = d * tHit;
                Vec3 nV = (p - cv) / R;
                Vec3 nW = camT * nV;
                double lit = dot(nW, starDirW);
                if (lit > 0 && (!occ.empty() || rings)) {
                    Vec3 surfW = posW + nW * R;
                    double shadow = 0;
                    for (const Occ& o : occ) {
                        Vec3 rel = o.pos - surfW;
                        double along = dot(rel, starDirW);
                        if (along <= 0) continue;
                        double dp2 = length2(rel) - along * along;
                        double dp = std::sqrt(std::max(0.0, dp2));
                        if (dp < o.r) shadow = std::max(shadow, clampd((o.r - dp) / (o.r * 0.12), 0, 1));
                    }
                    if (rings) {
                        double denom = dot(starDirW, b.spinAxis);
                        if (std::fabs(denom) > 1e-6) {
                            double tr = dot(posW - surfW, b.spinAxis) / denom;
                            if (tr > 0) {
                                double rr = length(surfW + starDirW * tr - posW);
                                if (rr >= ringR0 && rr <= ringR1) shadow = std::max(shadow, 0.75 * (*prof)[clampi((int)((rr - ringR0) / (ringR1 - ringR0) * 255), 0, 255)]);
                            }
                        }
                    }
                    lit *= (1 - 0.92 * shadow);
                }
                double viewCos = -dot(nV, d);
                Vec3 bf = frame * nW;
                double lat = std::asin(clampd(bf.z, -1, 1));
                double lon = std::atan2(bf.y, bf.x);
                double jet = giantJetTurn(b, bgen.bandCount, lat, c.t);   // M9-10 zonal jets (X-02: galaxy/probe.h, the descent reads the same)
                lon += jet;
                // W-06: the plate's cell under this point (the point turned by the jet, the plate being body-fixed): the
                // current plate first, the previous where the current has no cell yet, the map where neither has
                Vec3 bfG = jet != 0.0 ? rotZ(bf, jet) : bf;
                PlateHit ph; int phk = -1;
                for (int k = 0; k < npl && !ph.ok; k++) { ph = plateFind(*pl[k], bfG); if (ph.ok) phk = k; }
                double alb = ph.ok ? plateAlbedo(ph) : (haveMap ? map.albedoAt(lon, lat) : pt.albedo);
                int mat = ph.ok ? plateMaterial(ph) : (haveMap ? map.materialAt(lon, lat) : MAT_ROCK);
                if ((ph.ok || haveMap) && b.type == PT_GASGIANT) {   // N5-03 (M9-10): storms grow and fade over about a day, band by band
                    double st = ph.ok ? plateVeg(ph) : map.veg[map.texelIndex(lon, lat)] / 255.0;
                    if (st > 0.01) {
                        double gain = 0.55 + 0.45 * std::sin(c.t / 86400.0 * 0.9 + lat * 4.0 + (double)(b.seed & 255) * 0.02);
                        alb = clampd(alb - 0.35 * st + 0.35 * st * gain, 0.22, 1.0);
                    }
                }
                if (b.type != PT_GASGIANT && !c.skyMode) {
                    // M9-15 relief shading: slopes facing the star brighten, slopes facing away darken
                    // B-407 (2026-10-06): a surface rising toward the star tilts its normal away from it (a ramp rising east
                    // faces west), so the slope's term is taken off `lit`; M9-15 added it, and ridges were lit from the wrong side
                    if (ph.ok) {   // W-06: from the plate's heights along its axes, the rim's foreshortening taken out, held within +-0.7
                        double ga, gb; plateSlopes(ph, ga, gb);
                        double cc = clampd(dot(bfG, ph.p->u0), 0.05, 1.0);
                        lit = clampd(lit - clampd(plK[phk] * cc * (ga * dot(starDirW, plE1W[phk]) + gb * dot(starDirW, plE2W[phk])), -0.7, 0.7), -1, 1);
                    } else if (haveMap) {
                        Vec3 east = normalize(cross(b.spinAxis, nW)), north = cross(nW, east);
                        double sE = dot(starDirW, east), sN = dot(starDirW, north);
                        double dl = 0.7 * TAU / PlanetMap::W;
                        double hE = map.heightAt(lon + dl, lat) - map.heightAt(lon - dl, lat);
                        double hN = map.heightAt(lon, std::min(PI * 0.5, lat + dl)) - map.heightAt(lon, std::max(-PI * 0.5, lat - dl));
                        double run = 2 * dl * R * 1000.0;
                        lit = clampd(lit - 5.0 * (hE * sE + hN * sN) / run, -1, 1);
                    }
                }
                double shade = ambient + (1 - ambient) * smoothstep(-0.08, 0.25, lit) * std::max(0.0, lit * 0.6 + 0.4 * std::max(0.0, lit));
                shade = ambient + (1 - ambient) * clampd(lit * 1.1 + 0.05, 0, 1) * lf;
                // W-04: the parent's light past the terminator (it fades in over the first tenth, where the sun's own fades out)
                double earth = shineP > 0.002 && lit < 0 ? (1 - ambient) * 0.6 * shineP * std::max(0.0, dot(nW, shineDirW)) * smoothstep(0.0, -0.1, lit) : 0.0;
                if (earth > 0 && !c.skyMode) shade += earth;
                if (b.type == PT_SUBSTELLAR) shade = std::max(shade, bgen.selfGlow * (0.3 + 0.7 * alb));   // M5-02: it glows on its own, bands and all
                double v = exposureStop(alb, shade, atmosphere);   // N1-05: the ground's exposure rule
                if (mat == MAT_LAVA) v = std::max(v, 50.0 + 10.0 * alb);
                bool cloudy = false;
                if (atmosphere) {
                    double cl = 0.0; bool gotCl = false;
                    for (int k = 0; k < npl && !gotCl; k++) {   // W-06: the plate's cloud, the point turned by the drift since the build
                        PlateHit pc = plateFind(*pl[k], rotZ(bf, jet + cloudDrift - pl[k]->cloudDrift0));
                        if (pc.ok) { cl = plateCloud(pc); gotCl = true; }
                    }
                    if (!gotCl) cl = haveMap ? map.cloudAt(lon + cloudDrift, lat) : 0.0;
                    if (fmap) {   // W-03: a front's band over the pattern (body-fixed moving features: no drift); a dust front is a haze in the ground's own bank
                        double u = (lon + PI) * (1.0 / TAU); if (u < 0) u += 1; else if (u >= 1) u -= 1;
                        int bx = std::min(frontCW - 1, (int)(u * frontCW)), by = clampi((int)((0.5 - lat / PI) * frontCH), 0, frontCH - 1);
                        if (fmap->coarse[(size_t)by * frontCW + bx]) {
                            double fc = frontMapAt(fmap->cloud, PlanetMap::W, PlanetMap::H, lon, lat);
                            if (dustFronts) v = v + (63.0 * 0.8 * shade - v) * fc * 0.7;
                            else cl = std::max(cl, fc);
                        }
                    }
                    if (hasOpaqueDeck(b.type)) { v = 63.0 * (0.6 + 0.4 * cl) * shade; }
                    else if (b.type == PT_GASGIANT) { v = 63.0 * std::pow(alb, 0.7) * shade * 1.05; }
                    else { v = v + (63.0 * 0.92 * shade - v) * cl; cloudy = cl > 0.6; }
                    double limb = std::pow(1.0 - clampd(viewCos, 0, 1), 3.0);
                    v += 14.0 * limb * clampd(lit + 0.2, 0, 1) * lf;
                }
                if (b.type == PT_GASGIANT && lit < -0.05 && !c.skyMode) {   // M9-10 lightning on the night side
                    if (unitFromHash(hash3i(x >> 2, y >> 2, (int64_t)(c.t * 4), b.seed)) < 0.0025) v = 58;
                }
                sphereVal = clampd(v, 0, 63);
                double iz = 1.0 / (tHit * d.z * depthUnit);
                if (c.skyMode) {
                    // lit parts map onto the bright, neutral top of the sky ramp; the dark side only occludes
                    if (iz > fb.invz[o]) {
                        double lv = sphereVal / 63.0;
                        // everything that faces the star is drawn as a solid disc; the night side only occludes,
                        // except a substellar object, whose glow shows through the night (M5-02)
                        if (b.type == PT_SUBSTELLAR) fb.idx[o] = pix(13, 16 + 28 * clampd(lv, 0, 1));   // bank 13: its own dull red glow
                        else if (lit > -0.03) fb.idx[o] = pix(bank, 43 + 20 * clampd(lv * 1.15, 0.05, 1.0));
                        else if (earth > 0.0005) {   // W-04: the earthshine, just over the black of the sky bank and under the lit side's 43, where it beats the sky there
                            double ev = 30 + 0.45 * exposureStop(alb, earth, false);
                            if (ev > shadeOf(fb.idx[o])) fb.idx[o] = pix(bank, std::min(42.0, ev));
                        }
                        fb.invz[o] = (float)iz;
                    }
                } else if (iz > fb.invz[o]) {
                    // N1-02: the material's family bank (oceans since M1-06); thick cloud goes to the snow bank so it reads white
                    int pb = (b.type == PT_GASGIANT || hasOpaqueDeck(b.type) || b.type == PT_SUBSTELLAR) ? bank : globeBank(bank, cloudy ? MAT_CLOUD : mat);
                    fb.idx[o] = pix(pb, sphereVal); fb.invz[o] = (float)iz;
                }
            } else if (atmosphere && !c.skyMode) {
                // glass bubble: thin bright rim outside the disc
                double tcc = tc;
                double dperp2 = c2 - tcc * tcc;
                double dperp = std::sqrt(std::max(0.0, dperp2));
                double rim = (dperp - R) / (R * (bubble - 1.0));
                if (rim < 1 && tcc > 0) {
                    Vec3 p = d * tcc;
                    Vec3 nW = camT * ((p - cv) / dperp);
                    double lit = clampd(dot(nW, starDirW) + 0.15, 0, 1);
                    double add = 26 * (1 - rim) * lit * lf;
                    Pix& px = fb.idx[o];
                    if (add > shadeOf(px)) px = pix(bank, add);
                }
            }
            if (rings) {
                // ring plane through cv with normal axisV
                double denom = dot(d, axisV);
                if (std::fabs(denom) > 1e-6) {
                    double tr = dot(cv, axisV) / denom;
                    if (tr > 0) {
                        Vec3 p = d * tr;
                        double rr = length(p - cv);
                        if (rr >= ringR0 && rr <= ringR1) {
                            double u = (rr - ringR0) / (ringR1 - ringR0);
                            double dens = (*prof)[clampi((int)(u * 255), 0, 255)];
                            if (dens > 0.02) {
                                Vec3 pW = camT * p + c.shipPos;
                                // shadow of the planet on the ring: ray from pW toward the star
                                Vec3 toStar = starDirW;
                                Vec3 rel = pW - posW;
                                double tcs = -dot(rel, toStar);
                                double closest2 = length2(rel + toStar * tcs);
                                double shadow = (tcs > 0 && closest2 < R2) ? 0.12 : 1.0;
                                double litRing = std::fabs(dot(b.spinAxis, starDirW)) * 0.7 + 0.3;
                                double v = 63.0 * dens * litRing * shadow * lf * 0.9;
                                if (c.skyMode) v = 36.0 + 27.0 * std::pow(clampd(dens * 1.3, 0, 1), 0.4) * (0.5 + 0.5 * litRing) * shadow * lf;   // M5-04/O0-01: rings in a sky read like a moon (the sky bank is black below 30)
                                double iz = 1.0 / (tr * d.z * depthUnit);
                                if (iz > fb.invz[o] || (!hitSphere)) {
                                    if (iz > fb.invz[o]) {
                                        // translucent: blend with what is behind
                                        double cur = shadeOf(fb.idx[o]);
                                        double a = clampd(dens * (c.skyMode ? 2.2 : 1.6), c.skyMode ? 0.4 : 0.25, 1.0);
                                        double nv = v * a + cur * (1 - a);
                                        fb.idx[o] = pix(bank, clampd(nv, 0, 63));
                                        fb.invz[o] = (float)iz;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    });
}

void SpaceRenderer::render(Framebuffer& fb, const SpaceContext& c) {
    bodyInfo.clear();
    if (c.sys && c.sys->valid) setSeason(*c.sys, c.t);
    if (c.vimana) {
        // fade previous frame for streaks
        for (auto& p : fb.idx) { int v = intenOf(p) - 9 * INTEN_PER_SHADE; p = v > 0 ? pixI(bankOf(p), v) : 0; }
    } else {
        fb.clear(0);
    }
    fb.clearDepth();
    // M1-09: the galactic backdrop, a faint band of unresolved stars (rebuilt when the ship moved)
    if (!c.vimana) {
        Vec3 obs = c.shipPos / SECTOR_KM;
        if (!bandValid || length(obs - bandPos) > 10.0) { buildGalaxyBand(obs, bandMap, BAND_MAP_W, BAND_MAP_H); bandPos = obs; bandValid = true; }   // G-03: rebuilt after a hop of ten sectors (the near dust's rifts move by a couple of degrees per hundred)
        Mat3 camT = c.cam.transposed();
        double invf = 1.0 / proj.f;
        const int step = 2;
        NebulaPatch nebP[18];
        int nebN = nebulaPatches(obs, nebP, 16);   // N5-01: the star-forming region's patches, blue, red or white
        if (c.sys && c.sys->valid) nebN += starNebulaPatches(c.sys->star, normalize(c.sys->star.pos - c.shipPos), nebP + nebN);   // S-04: a protostar's own cloud
        parallelFor(FBH / step, 40, [&](int rb, int re) {
            for (int y = rb * step; y < re * step && y < FBH; y += step)
                for (int x = 0; x < FBW; x += step) {
                    Vec3 d = normalize(Vec3((x + 0.5 - proj.cx) * invf, -(y + 0.5 - proj.cy) * invf, 1.0));
                    Vec3 dw = camT * d;
                    double band = sampleGalaxyBand(bandMap, BAND_MAP_W, BAND_MAP_H, dw);
                    Pix pv = pix(0, band * BAND_SHADES);   // G-03: the map is the brightness itself
                    if (nebN) {
                        int tone;
                        double g = nebulaGlow(nebP, nebN, dw, tone);
                        if (g > 0.02) { Pix pn = pix(tone == 0 ? 4 : (tone == 1 ? 5 : 0), g * 17.0); if (intenOf(pn) > intenOf(pv)) pv = pn; }
                    }
                    for (int yy = y; yy < y + step && yy < FBH; yy++)
                        for (int xx = x; xx < x + step && xx < FBW; xx++) if (intenOf(fb.idx[yy * FBW + xx]) < intenOf(pv)) fb.idx[yy * FBW + xx] = pv;
                }
        });
    }
    if (c.stars)
        drawStarField(fb, *c.stars, c.shipPos, c.cam, proj, c.starIntensity, 0, true, nullptr, 0, c.vimana ? c.vimanaSpeed : 0, c.vimanaDirView);
    if (!c.sys || !c.sys->valid) return;
    const StarSystem& sys = *c.sys;
    // M9-12 asteroid belts: a sparkle band of hashed rocks in the orbital plane
    for (size_t bi = 0; bi < sys.belts.size(); bi++) {
        const Belt& bl = sys.belts[bi];
        double angT = TAU * c.t / bl.period;
        for (int i = 0; i < 260; i++) {
            uint64_t h = hash2i((int64_t)i, (int64_t)bi, sys.star.seed ^ 0xBE17);
            double rr = bl.innerKm + (bl.outerKm - bl.innerKm) * unitFromHash(h);
            double ang = bl.phase0 + unitFromHash(mix64(h + 1)) * TAU + angT * std::pow(bl.innerKm / rr, 1.5);
            double y = (unitFromHash(mix64(h + 2)) - 0.5) * 0.03 * rr;
            Vec3 pos = sys.star.pos + Vec3(std::cos(ang) * rr, y, std::sin(ang) * rr);
            Vec3 v = c.cam * (pos - c.shipPos);
            if (v.z <= 1) continue;
            double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
            int x = (int)sx, yy = (int)sy;
            if (x < 0 || yy < 0 || x >= FBW || yy >= FBH) continue;
            double dist = length(v);
            double inten = clampd(30 - 6 * std::log10(dist / 1e6), 8, 40) * lightFactor(sys.star.luminosity, length(pos - sys.star.pos));
            int o = yy * FBW + x;
            if (intenOf(fb.idx[o]) < toInten(inten)) { fb.idx[o] = pix(0, inten); fb.invz[o] = (float)(1.0 / v.z); }
        }
    }
    // sun (B-308: drawn by angle, so it is right even with its centre beside or behind the camera)
    {
        Vec3 rel = sys.star.pos - c.shipPos;
        Vec3 v = c.cam * rel;
        double dist = length(v);
        double angR = std::asin(clampd(sys.star.radiusKm / dist, 0, 1));
        double inten = clampd(0.6 + 0.4 * std::log10(1 + sys.star.luminosity), 0.7, 1.0);
        Vec3 discUp = c.cam * Vec3(0, 1, 0);   // S-04: the orbital plane's normal, for a protostar's disc
        drawSun(fb, sys.star, v / dist, angR, c.t, inten, 1, false, 0, false, proj, &discUp);
        if (!c.vimana && v.z > 0 && sys.star.cls != STAR_BLACK_HOLE) {   // S-06: no flare from a hole
            double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
            double rpx = proj.f * std::tan(angR) * (dist / v.z);
            drawLensFlare(fb, sx, sy, std::max(rpx, 2.0 * FB_SCALE), inten * 0.7, 1, proj);
        }
    }
    updatePlate(c);   // W-06: the telescope's plate of its body, kept, begun or built on within the budget
    // bodies sorted far to near so nearer globes overwrite (depth buffer handles the rest)
    std::vector<std::pair<double, int>> order;
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        double d = length2(sys.bodyPos(i, c.t) - c.shipPos);
        order.push_back({d, i});
    }
    std::sort(order.begin(), order.end(), [](auto& a, auto& b) { return a.first > b.first; });
    bodyInfo.resize(sys.bodies.size());
    for (auto& pr : order) {
        int i = pr.second;
        if (i == c.excludeBody) continue;   // X-02: the probe's sky draws the space beyond a giant's air without the giant
        int bank = (i == c.bankBodyA) ? 2 : (i == c.bankBodyB ? 3 : 0);
        drawGlobe(fb, c, i, bank, bodyInfo[i]);
    }
    lastBeltRocks = lastBeltMeshes = 0;
    if (!c.skyMode) for (size_t k = 0; k < sys.belts.size(); k++) drawBeltRocks(fb, c, (int)k);   // O3: the rocks around a ship inside a belt
}

// O3 (R-302): the rocks of a belt around the ship, hashed in co-rotating cells (StarSystem::beltRockAt): a point when
// under two pixels across, else a lumpy mesh (a sphere with fbm radii) tumbling slowly, lit by the star, depth-tested
// against the globes drawn before it
void SpaceRenderer::drawBeltRocks(Framebuffer& fb, const SpaceContext& c, int k) {
    const StarSystem& sys = *c.sys;
    const Belt& bl = sys.belts[k];
    Vec3 rel = c.shipPos - sys.star.pos;
    double r = std::sqrt(rel.x * rel.x + rel.z * rel.z), w = bl.outerKm - bl.innerKm;
    if (r < bl.innerKm - 0.1 * w || r > bl.outerKm + 0.1 * w || std::fabs(rel.y) > 0.03 * r + 2 * BELT_CELL_KM) return;   // the ship is not in this belt
    struct Seen { double d; BeltRock rk; };
    std::vector<Seen> seen;
    const double S = FB_SCALE;
    sys.forBeltRocksNear(k, c.shipPos, c.t, 4, 4, 1, [&](const BeltRock& rk) {
        Vec3 v = c.cam * (rk.pos - c.shipPos);
        if (v.z <= rk.radiusKm * 1.5) return;
        double d = length(v);
        double rpx = proj.f * rk.radiusKm / d;
        if (rpx < 0.35 * S) return;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        if (sx < -rpx * 2 - 4 || sy < -rpx * 2 - 4 || sx > FBW + rpx * 2 + 4 || sy > FBH + rpx * 2 + 4) return;
        seen.push_back({d, rk});
    });
    std::sort(seen.begin(), seen.end(), [](const Seen& a, const Seen& b) { return a.d > b.d; });
    Vec3 starDir = normalize(sys.star.pos - c.shipPos);   // the star is far: one direction serves every rock
    double lf = clampd(lightFactor(sys.star.luminosity, length(sys.star.pos - c.shipPos)), 0, 1.15);
    RasterParams rp; rp.bank = 20;   // bank 20: the rock ramp (see setupPalette)
    for (const Seen& sn : seen) {
        const BeltRock& rk = sn.rk;
        Vec3 v = c.cam * (rk.pos - c.shipPos);
        double rpx = proj.f * rk.radiusKm / sn.d;
        lastBeltRocks++;
        if (rpx < 1.6 * S) {   // a point, bright with its phase
            double phase = 0.5 + 0.5 * dot(normalize(c.shipPos - rk.pos), starDir);
            double val = clampd(14 + 34 * phase * lf + 8 * rpx / S, 6, 50);
            int x = (int)(proj.cx + proj.f * v.x / v.z), y = (int)(proj.cy - proj.f * v.y / v.z);
            float iz = (float)(1.0 / v.z);
            for (int dy = 0; dy < FB_SCALE; dy++)
                for (int dx = 0; dx < FB_SCALE; dx++) {
                    int px = x + dx, py = y + dy;
                    if (px < 0 || py < 0 || px >= FBW || py >= FBH) continue;
                    int o = py * FBW + px;
                    if (iz > fb.invz[o]) { fb.idx[o] = pix(20, val); fb.invz[o] = iz; }
                }
            continue;
        }
        lastBeltMeshes++;
        const int NA = rpx > 8 * S ? 12 : 6, NE = rpx > 8 * S ? 6 : 3;
        Mat3 rot = Mat3::axisAngle(rk.axis, rk.tumble);
        Vec3 pts[13][7];
        for (int e = 0; e <= NE; e++)
            for (int a = 0; a <= NA; a++) {
                double el = (e / (double)NE - 0.5) * PI, az = a * TAU / NA;
                Vec3 n = e == 0 ? Vec3(0, -1, 0) : (e == NE ? Vec3(0, 1, 0) : Vec3(std::cos(el) * std::cos(az), std::sin(el), std::cos(el) * std::sin(az)));
                double rr = rk.radiusKm * (1 + 0.38 * fbm3(n * 1.7 + Vec3(3.1, 1.7, 5.3), rk.seed, 3));
                pts[a][e] = rk.pos + rot * (n * rr);
            }
        auto face = [&](const Vec3* p, int n) {
            Vec3 nrm = normalize(cross(p[1] - p[0], p[n - 1] - p[0]));
            if (dot(nrm, p[0] - c.shipPos) > 0) return;   // faces away from the ship
            double light = 0.06 + 0.94 * std::max(0.0, dot(nrm, starDir)) * lf;
            double shade = 52 * std::pow(light, 0.6);
            RVert q[4];
            for (int i = 0; i < n; i++) { Vec3 vv = c.cam * (p[i] - c.shipPos); q[i].x = vv.x; q[i].y = vv.y; q[i].z = vv.z; q[i].shade = shade; }
            rasterPolygon(fb, q, n, rp, proj);
        };
        for (int e = 0; e < NE; e++)
            for (int a = 0; a < NA; a++) {
                if (e == 0) { Vec3 tri[3] = {pts[a][0], pts[a + 1][1], pts[a][1]}; face(tri, 3); }
                else if (e == NE - 1) { Vec3 tri[3] = {pts[a][e], pts[a + 1][e], pts[a][NE]}; face(tri, 3); }
                else { Vec3 quad[4] = {pts[a][e], pts[a + 1][e], pts[a + 1][e + 1], pts[a][e + 1]}; face(quad, 4); }
            }
    }
}

void SpaceRenderer::drawSkyBodies(Framebuffer& fb, const SpaceContext& c) {
    if (!c.sys || !c.sys->valid) return;
    const StarSystem& sys = *c.sys;
    std::vector<std::pair<double, int>> order;
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        if (i == c.excludeBody) continue;
        order.push_back({length2(sys.bodyPos(i, c.t) - c.shipPos), i});
    }
    std::sort(order.begin(), order.end(), [](auto& a, auto& b) { return a.first > b.first; });
    bodyInfo.assign(sys.bodies.size(), BodyScreenInfo());
    for (auto& pr : order) { if (sys.bodies[pr.second].type == PT_COMPANION) continue; drawGlobe(fb, c, pr.second, 1, bodyInfo[pr.second]); }
}

// S-06: a black hole's companion is being drawn out: a stream of its gas from the limb facing the hole to the edge of the
// disc, thin at the companion and flaring into the disc, bent back against the companion's motion (the Coriolis side) and
// wavering slowly, drawn before the companion's own disc in its bank. The hole's disc and shadow are drawn with the sun
void SpaceRenderer::drawAccretionStream(Framebuffer& fb, const SpaceContext& c, int bi, int bank) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    Vec3 posW = sys.bodyPos(bi, c.t);
    Vec3 toHole = sys.star.pos - posW;
    double dist = length(toHole);
    if (dist < 1e-6) return;
    toHole = toHole / dist;
    Vec3 fwd = sys.bodyVel(bi, c.t);
    fwd = fwd - toHole * dot(fwd, toHole);
    double fl = length(fwd);
    fwd = fl > 1e-9 ? fwd / fl : Vec3(0, 1, 0);
    const double rc = b.radiusKm, rEnd = sys.star.radiusKm * 9.0, ph = (double)(b.seed % 5);
    const double span = std::max(dist - rc * 0.9 - rEnd, rc * 0.5);
    const int N = 40;
    for (int k = 0; k <= N; k++) {
        double f = (double)k / N;
        double wob = 0.06 * std::sin(f * 11.0 + c.t * 0.02 + ph);
        Vec3 p = posW + toHole * (rc * 0.9 + span * f) - fwd * (span * (0.22 * f * f + wob * f));
        Vec3 v = c.cam * (p - c.shipPos);
        if (v.z <= 1) continue;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        if (sx < -80 * FB_SCALE || sy < -80 * FB_SCALE || sx > FBW + 80 * FB_SCALE || sy > FBH + 80 * FB_SCALE) continue;
        double rKm = rc * (0.12 + 0.3 * (1 - f)) + rEnd * 0.12 * f;
        double rpx = std::max(proj.f * rKm / length(v), 1.0 * FB_SCALE);
        if (rpx > 80 * FB_SCALE) rpx = 80 * FB_SCALE;
        int a = (int)((9 + 9 * f) * (0.85 + 0.15 * std::sin(f * 23.0 - c.t * 0.05)) + 0.5);
        if (a < 1) continue;
        fb.glowDisc(sx, sy, rpx, rpx * 0.4, a, bank);
    }
}

// S-06: the black hole. No disc of light: a shadow of 2.6 radii (the photon ring; the class's radius stands for the Schwarzschild
// radius) in the sky, and behind it the stars and the band bent into arcs by a lens: a pixel at the angle th from the hole shows
// what lies at th - thE^2 / th along the same great circle (the other side when that is negative), with the Einstein angle
// thE = sqrt(R / D), the weak-field deflection of a source at infinity; the bend fades out toward the edge of the region (2.6 thE)
// so there is no seam. The bent directions are computed on a grid of one logical pixel and interpolated, the frame before the
// lens copied once (the bent rays read anywhere in it). The accretion disc lies in the plane of the worlds, a slab half a radius
// thick from 3 to 9 radii, bright and hot inside to a dim orange outside, brighter on the side that comes toward the viewer (the
// spin by the seed): its near half is seen along the straight ray and drawn over the shadow, its far half along the bent ray from
// the lens plane, so it arches over and under the shadow. Two in five blow a jet along the plane's normal, drawn before the lens
// so the shadow covers its root and the lens bends it. Rows in parallel; the whole frame when the cone reaches the camera plane
void SpaceRenderer::drawBlackHole(Framebuffer& fb, const Star& star, const Vec3& dirView, double angR, double t, double intensity,
                                  int bank, bool depthTest, const Proj& pj, const Vec3* discUp) {
    (void)t;
    const double S = FB_SCALE, invf = 1.0 / pj.f;
    const double rA = std::max(angR, 0.8 * S * invf);
    const double thE = std::sqrt(rA);
    const double regionA = std::min(2.6 * thE, 80 * DEG);
    const double shadowA = 2.6 * rA;
    const double cosRegion = std::cos(regionA), cosShadow = std::cos(shadowA);
    const double R = star.radiusKm, D = R / std::sin(std::max(angR, 1e-7));
    const Vec3 H = dirView * D;
    Vec3 n = discUp ? *discUp : Vec3(0, 1, 0);
    if (length2(n) < 1e-9) n = Vec3(0, 1, 0);
    n = normalize(n);
    const double spin = unitFromHash(star.seed ^ 0x5B1AULL) < 0.5 ? 1.0 : -1.0;
    const bool jet = unitFromHash(star.seed ^ 0x3E7AULL) < 0.4;
    const double rIn = 3.0 * R, rOut = 9.0 * R, half = 0.25 * R;
    const double thDisc = std::min(regionA, std::asin(std::min(1.0, rOut / D)) * 1.15 + 0.02);
    const double cosTest = std::cos(std::max(thDisc, std::min(regionA, 1.5 * thE)));
    // the box: the cone of regionA round the hole, or the whole frame when it reaches the camera plane
    double theta = std::acos(clampd(dirView.z, -1, 1));
    bool centreFront = dirView.z > 1e-6;
    double sx = 0, sy = 0;
    if (centreFront) { sx = pj.cx + pj.f * dirView.x / dirView.z; sy = pj.cy - pj.f * dirView.y / dirView.z; }
    int x0 = 0, x1 = FBW - 1, y0 = 0, y1 = FBH - 1;
    if (centreFront && theta + regionA < 85 * DEG) {
        double rb = pj.f * std::sin(regionA) / (std::cos(theta) * std::cos(theta + regionA)) + 2;
        x0 = std::max(0, (int)(sx - rb)); x1 = std::min(FBW - 1, (int)(sx + rb));
        y0 = std::max(0, (int)(sy - rb)); y1 = std::min(FBH - 1, (int)(sy + rb));
        if (x0 > x1 || y0 > y1) return;
    } else if (theta - regionA > 95 * DEG) return;
    // the jet, before the lens
    if (jet) {
        for (int side = -1; side <= 1; side += 2) {
            for (int k = 0; k < 20; k++) {
                double f = (double)k / 19;
                Vec3 p = H + n * (side * R * (4.0 + 26.0 * f));
                if (p.z <= 1) continue;
                double px = pj.cx + pj.f * p.x / p.z, py = pj.cy - pj.f * p.y / p.z;
                double rpx = std::max(pj.f * R * (0.5 + 1.2 * f) / length(p), 1.0 * S);
                if (rpx > 60 * S) rpx = 60 * S;
                int a = (int)(intensity * 9 * (1 - f) * (1 - f) + 0.5);
                if (a >= 1) fb.glowDisc(px, py, rpx, rpx * 0.3, a, bank, depthTest);
            }
        }
    }
    // the frame before the lens
    std::vector<Pix> before(fb.idx);
    // the bent directions on the grid (not unit: only their projection and their line are used)
    const int g = std::max(1, FB_SCALE);
    const int gw = (x1 - x0) / g + 2, gh = (y1 - y0) / g + 2;
    std::vector<float> gx(gw * gh), gy(gw * gh), gz(gw * gh);
    parallelFor(gh, 8, [&](int jb, int je) {
        for (int j = jb; j < je; j++) for (int i = 0; i < gw; i++) {
            Vec3 d((x0 + i * g + 0.5 - pj.cx) * invf, -(y0 + j * g + 0.5 - pj.cy) * invf, 1.0);
            Vec3 dn = d / std::sqrt(dot(d, d));
            double cosA = clampd(dot(dn, dirView), -1, 1);
            Vec3 u = dn - dirView * cosA;
            double ul = std::sqrt(dot(u, u));
            double th = std::atan2(ul, cosA);
            Vec3 out = dn;
            if (ul > 1e-9 && th > 1e-6) {
                u = u / ul;
                double thc = std::max(th, shadowA);
                double fade = smoothstep(regionA, 0.6 * regionA, thc);
                double ths = thc - fade * thE * thE / thc;
                out = dirView * std::cos(ths) + u * std::sin(ths);
            }
            gx[j * gw + i] = (float)out.x; gy[j * gw + i] = (float)out.y; gz[j * gw + i] = (float)out.z;
        }
    });
    // the disc along a ray from `o` in the direction `dd`: the brightest of the slab's three planes, hits before the hole's
    // transverse plane (the near half) or beyond it (the far half); 0 = miss
    auto discAlong = [&](const Vec3& o, const Vec3& dd, bool farHalf) -> double {
        double dn_ = dot(dd, n);
        if (std::fabs(dn_) < 1e-12) return 0;
        double ddl = std::sqrt(dot(dd, dd));
        double best = 0;
        for (int k = -1; k <= 1; k++) {
            double tt = (dot(H - o, n) + k * half) / dn_;
            if (tt <= 0) continue;
            Vec3 rel = o + dd * tt - H;
            double along = dot(rel, dirView);
            if (farHalf ? along < 0 : along > 0) continue;
            Vec3 rp = rel - n * dot(rel, n);
            double r = std::sqrt(dot(rp, rp));
            if (r < rIn || r > rOut) continue;
            double rr = (r - rIn) / (rOut - rIn);
            Vec3 vel = cross(n, rp / r) * spin;
            double dop = 1.0 + 0.6 * dot(vel, dd) * (-1.0 / ddl);
            double b = (1.0 - 0.72 * rr) * dop;
            if (b > best) best = b;
        }
        return best;
    };
    parallelFor(y1 - y0 + 1, 16, [&](int rb, int re) {
        for (int y = y0 + rb; y < y0 + re; y++) {
            int j = (y - y0) / g; double fy = (double)((y - y0) - j * g) / g;
            for (int x = x0; x <= x1; x++) {
                Vec3 d((x + 0.5 - pj.cx) * invf, -(y + 0.5 - pj.cy) * invf, 1.0);
                double dl = std::sqrt(dot(d, d));
                double cosA = dot(d, dirView) / dl;
                if (cosA <= cosRegion) continue;
                int o = y * FBW + x;
                if (depthTest && fb.invz[o] > 1e-12f) continue;
                Pix& p = fb.idx[o];
                Vec3 dn = d / dl;
                double b = 0;
                if (cosA > cosShadow) {
                    p = pixI(0, 0);
                    b = discAlong(Vec3(0, 0, 0), dn, false);
                } else {
                    int i = (x - x0) / g; double fx = (double)((x - x0) - i * g) / g;
                    int k = j * gw + i;
                    double w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
                    Vec3 ds(gx[k] * w00 + gx[k + 1] * w10 + gx[k + gw] * w01 + gx[k + gw + 1] * w11,
                            gy[k] * w00 + gy[k + 1] * w10 + gy[k + gw] * w01 + gy[k + gw + 1] * w11,
                            gz[k] * w00 + gz[k + 1] * w10 + gz[k + gw] * w01 + gz[k + gw + 1] * w11);
                    if (ds.z > 1e-6) {
                        int ix = (int)(pj.cx + pj.f * ds.x / ds.z), iy = (int)(pj.cy - pj.f * ds.y / ds.z);
                        p = (ix >= 0 && iy >= 0 && ix < FBW && iy < FBH) ? before[iy * FBW + ix] : pixI(0, 0);
                    } else p = pixI(0, 0);
                    if (cosA > cosTest) {
                        b = discAlong(Vec3(0, 0, 0), dn, false);
                        if (b <= 0) b = discAlong(dn * (D / cosA), ds, true);
                    }
                }
                if (b > 0) p = pix(bank, clampd(63 * intensity * b, 0, 63));
            }
        }
    });
}

// S-05: a gas giant of a Wolf-Rayet star has its envelope stripped by the wind: a plume of its own cloud bank from the limb
// away from the star, four radii long, widening and thinning, curling back along the orbit and wavering slowly, drawn before
// the globe (which covers its root). The giant itself is untouched; the plume is light, like the comet's tail
void SpaceRenderer::drawStrippedTail(Framebuffer& fb, const SpaceContext& c, int bi, int bank) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    Vec3 posW = sys.bodyPos(bi, c.t);
    double dark = c.skyMode ? c.skyDark : 1.0;
    if (dark < 0.05) return;
    Vec3 away = normalize(posW - sys.star.pos);
    Vec3 back = -sys.bodyVel(bi, c.t);
    back = back - away * dot(back, away);
    double bl = length(back);
    back = bl > 1e-9 ? back / bl : Vec3(0, 1, 0);
    Vec3 side = cross(away, back);
    const double R = b.radiusKm, ph = (double)(b.seed % 7);
    const int N = 40;
    for (int k = 0; k <= N; k++) {
        double f = (double)k / N;
        double wob = 0.25 * std::sin(f * 7.0 + c.t * 0.01 + ph) + 0.15 * std::sin(f * 13.0 - c.t * 0.016);
        Vec3 p = posW + away * (R * (1.0 + 4.0 * f * f)) + back * (R * 0.6 * f * f) + side * (R * wob * f);
        Vec3 v = c.cam * (p - c.shipPos);
        if (v.z <= 1) continue;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        if (sx < -80 * FB_SCALE || sy < -80 * FB_SCALE || sx > FBW + 80 * FB_SCALE || sy > FBH + 80 * FB_SCALE) continue;
        double rpx = std::max(proj.f * (R * (0.55 + 0.9 * f)) / length(v), 1.0 * FB_SCALE);
        if (rpx > 80 * FB_SCALE) rpx = 80 * FB_SCALE;
        int a = (int)(14 * (1 - f) * (1 - f) * dark + 0.5);
        if (a < 1) continue;
        fb.glowDisc(sx, sy, rpx, rpx * 0.4, a, bank);
    }
}

// M5-07/N0-01 (B-201): a comet's coma, its straight ion tail with knots streaming outward, a dust tail
// curving back along the orbit and, close up, jets from the sunlit side of the nucleus. The orbit itself
// is untouched (the sky stays honest); the internal motion is a deterministic function of the time.
double SpaceRenderer::cometKnot(int k, double t) {
    double ph = k * 0.2 + (k * 0.618034 - std::floor(k * 0.618034)) * 0.3 + t / 40.0;
    return ph - std::floor(ph);
}

void SpaceRenderer::drawCometTail(Framebuffer& fb, const SpaceContext& c, int bi, int bank, double nucleusRpx) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    Vec3 posW = sys.bodyPos(bi, c.t);
    double dStar = length(posW - sys.star.pos);
    double activity = cometActivityAt(sys, dStar);
    if (activity < 0.02) return;
    double refKm = cometRefKm(sys);   // the tail's length
    double dark = c.skyMode ? c.skyDark : 1.0;
    if (dark < 0.05) return;
    Vec3 away = normalize(posW - sys.star.pos);
    Vec3 back = -sys.bodyVel(bi, c.t);
    back = back - away * dot(back, away);   // the part of "where it came from" that is not along the tail
    double bl = length(back);
    back = bl > 1e-9 ? back / bl : Vec3(0, 1, 0);
    double lenKm = activity * refKm * 0.5;
    double sqa = std::sqrt(activity);
    auto disc = [&](const Vec3& p, double rKm, double rMinPx, double add) {
        Vec3 v = c.cam * (p - c.shipPos);
        if (v.z <= 1) return;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        if (sx < -50 || sy < -50 || sx > FBW + 50 || sy > FBH + 50) return;
        double rpx = std::max(proj.f * rKm / length(v), rMinPx);
        if (rpx > 70 * FB_SCALE) rpx = 70 * FB_SCALE;
        int a = (int)(add * dark + 0.5);
        if (a < 1) return;
        fb.glowDisc(sx, sy, rpx, rpx * 0.4, a, bank);
    };
    // the coma, breathing a little
    double flicker = 1.0 + 0.10 * std::sin(c.t * 1.7) * std::sin(c.t * 0.61 + 1.3) + 0.05 * std::sin(c.t * 3.1);
    disc(posW, 6e4 * sqa, 2.0 * FB_SCALE, 30 * activity * flicker);
    // the ion tail: straight away from the star; five knots of brightness stream outward, accelerating
    const int N = 36;
    for (int k = 1; k <= N; k++) {
        double f = (double)k / N;
        double knots = 0;
        for (int j = 0; j < 5; j++) { double d = f - cometKnot(j, c.t); knots += std::exp(-d * d / (2 * 0.06 * 0.06)); }
        Vec3 p = posW + away * (lenKm * std::pow(f, 1.3));
        disc(p, (6e4 + 9e5 * f) * sqa, 1.0 * FB_SCALE, 18 * (1 - f) * (1 - f) * (0.82 + 0.36 * knots) * activity);
    }
    // the dust tail: heavier grains lag along the orbit, so it is shorter, wider, dimmer and curved
    const int N2 = 24;
    for (int k = 1; k <= N2; k++) {
        double g = (double)k / N2;
        Vec3 p = posW + away * (lenKm * 0.55 * g) + back * (lenKm * 0.28 * g * g);
        disc(p, (1.0e5 + 1.1e6 * g) * sqa, 1.0 * FB_SCALE, 9 * (1 - g) * (1 - g) * activity);
    }
    // close up: two to four jets vent from the sunlit hemisphere and bend back into the tail, knots
    // streaming out every 8 s; they turn with the nucleus
    if (nucleusRpx >= 3 * FB_SCALE) {
        Mat3 frame = sys.bodyFrame(bi, c.t);
        Mat3 inv = frame.transposed();
        Vec3 awayB = frame * away;
        int nj = 2 + (int)(b.seed % 3);
        for (int j = 0; j < nj; j++) {
            uint64_t hj = mix64(b.seed ^ (0x1E75ULL + (uint64_t)j * 0x9E37ULL));
            double lat = ((hj & 0xffff) / 65535.0 - 0.5) * 1.6, lon = ((hj >> 16) & 0xffff) / 65535.0 * TAU;
            Vec3 dB = StarSystem::bodyFromLatLon(lat, lon);
            if (dot(dB, awayB) > -0.15) dB = normalize(dB - awayB * (dot(dB, awayB) + 0.35));   // vents face the star
            Vec3 d = inv * dB;
            double strength = 0.6 + 0.4 * (((hj >> 32) & 0xff) / 255.0);
            for (int k = 0; k < 14; k++) {
                double s = (k + 0.5) / 14;
                double kn = 0;
                for (int q = 0; q < 3; q++) { double ph = q / 3.0 + j * 0.37 + c.t / 8.0; ph -= std::floor(ph); double dd = s - ph; kn += std::exp(-dd * dd / (2 * 0.06 * 0.06)); }
                Vec3 p = posW + d * (b.radiusKm * (1.02 + 3.0 * s)) + away * (b.radiusKm * 2.2 * s * s);
                disc(p, b.radiusKm * (0.10 + 0.35 * s), 1.0 * FB_SCALE, 12 * (1 - s) * (0.6 + 0.8 * kn) * strength * activity);
            }
        }
    }
}
