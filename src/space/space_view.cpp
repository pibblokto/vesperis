#include "space_view.h"
#include "core/noise.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cmath>
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
                            double intensity, int bank, bool atmosphere, double atmosHaze, bool depthTest, const Proj& pj) {
    int cls = star.cls;
    double glowMul = 1.0;
    double pulse = 1.0;
    if (cls == STAR_PULSAR) {
        double ph = std::fmod(t * star.pulseHz, 1.0);
        pulse = 0.15 + 0.85 * std::pow(std::max(0.0, std::cos(ph * TAU)), 12.0);
        glowMul = 0.6 + 1.4 * pulse;
    }
    if (cls == STAR_BLUE_GIANT) glowMul = 1.8;
    if (cls == STAR_RED_GIANT) glowMul = 0.6;
    if (cls == STAR_WHITE_DWARF) glowMul = 1.1;
    if (cls == STAR_ORANGE) glowMul = 0.9;
    const double S = FB_SCALE, invf = 1.0 / pj.f;
    double rA = std::max(angR, 0.6 * S * invf);   // the old floor of 0.6 px, as an angle
    double coronaA = std::max(rA * (atmosphere ? 3.2 : 2.4), 5.0 * S * invf) * glowMul;
    double outerA = std::min(coronaA * (atmosphere ? 3.0 : 1.6) * (1.0 + atmosHaze), PI - 0.01);
    int addOuter = (int)(intensity * (atmosphere ? 14 : 8) * pulse);
    int addCorona = (int)(intensity * 30 * pulse);
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
                    if (cls == STAR_RED_GIANT) {
                        double n = gnoise2(nx * 1.5 + t * 0.03, ny * 1.5, star.seed) * 0.5 +
                                   gnoise2(nx * 4 - t * 0.05, ny * 4, star.seed + 3) * 0.3;
                        v = 50 + 14 * n;
                        v *= limb * 0.95 + 0.05;
                    } else if (cls == STAR_ORANGE || cls == STAR_YELLOW) {
                        double n = gnoise2(nx * 3 + t * 0.01, ny * 3, star.seed);
                        double spots = (cls == STAR_ORANGE && n > 0.42) ? 18 * (n - 0.42) / 0.3 : 0;
                        v = (58 + 4 * gnoise2(nx * 8, ny * 8 + t * 0.02, star.seed + 1)) * limb - spots * 2.0;
                    } else if (cls == STAR_BLUE_GIANT) {
                        v = 63 * (0.8 + 0.2 * mu);
                    } else if (cls == STAR_PULSAR) {
                        v = 40 + 23 * pulse;
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
                if (add <= 0) continue;
                int bk = intenOf(p) == 0 ? bank : bankOf(p);
                p = pixI(bk, intenOf(p) + add);
            }
        }
    });
    if (!centreFront) return;
    // rays for compact stars, from the projected centre, outside the disc
    if (cls == STAR_WHITE_DWARF || cls == STAR_PULSAR || cls == STAR_BLUE_GIANT) {
        double coronaPx = std::min(pj.f * std::tan(std::min(coronaA, 1.4)), (double)FBW);
        double rPx = pj.f * std::tan(std::min(rA, 1.4));
        double len = coronaPx * (cls == STAR_BLUE_GIANT ? 1.4 : 2.2) * (cls == STAR_PULSAR ? pulse : 1.0);
        int arms = cls == STAR_WHITE_DWARF ? 4 : (cls == STAR_BLUE_GIANT ? 3 : 2);
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
        drawSun(fb, ks, cv / dist, angR, c.t, inten, 12, false, 0, true, proj);   // B-308: by angle, whatever the size
        return;
    }
    StarSystem::Light light = sys.lightAt(posW, c.t);   // M5-01: bodies near the companion are lit by it
    Vec3 starDirW = light.dir;
    double lf = clampd(light.factor, 0, 1.15);
    if (b.type == PT_COMET) drawCometTail(fb, c, bi, c.skyMode ? 1 : bank, rpx);
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
    bool atmosphere = pt.atmosphere;
    double bubble = atmosphere ? 1.045 : 1.0;
    // B-312: the box is the exact screen extent of the sphere (its projection is a conic that reaches farther from the
    // projected centre the nearer the centre is to the edge of the view: a box of radius rpx round the centre cut the
    // limb off, and a huge parent vanished from a moon's sky as soon as its centre left the frame to the left or the
    // right), the whole frame when the sphere reaches the camera plane, and the projected outer circle of the rings
    int x0, x1, y0, y1;
    sphereBox(cv, R * bubble, proj, x0, x1, y0, y1);
    double ambient = atmosphere ? 0.10 : 0.05;
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
                if (b.type == PT_GASGIANT) lon += TAU * c.t / b.rotPeriod * 0.06 * std::sin(lat * bgen.bandCount);   // M9-10 zonal jets
                double alb = haveMap ? map.albedoAt(lon, lat) : pt.albedo;
                int mat = haveMap ? map.materialAt(lon, lat) : MAT_ROCK;
                if (haveMap && b.type == PT_GASGIANT) {   // N5-03 (M9-10): storms grow and fade over about a day, band by band
                    double st = map.veg[map.texelIndex(lon, lat)] / 255.0;
                    if (st > 0.01) {
                        double gain = 0.55 + 0.45 * std::sin(c.t / 86400.0 * 0.9 + lat * 4.0 + (double)(b.seed & 255) * 0.02);
                        alb = clampd(alb - 0.35 * st + 0.35 * st * gain, 0.22, 1.0);
                    }
                }
                if (haveMap && b.type != PT_GASGIANT && !c.skyMode) {
                    // M9-15 relief shading: slopes facing the star brighten, slopes facing away darken
                    Vec3 east = normalize(cross(b.spinAxis, nW)), north = cross(nW, east);
                    double sE = dot(starDirW, east), sN = dot(starDirW, north);
                    double dl = 0.7 * TAU / PlanetMap::W;
                    double hE = map.heightAt(lon + dl, lat) - map.heightAt(lon - dl, lat);
                    double hN = map.heightAt(lon, std::min(PI * 0.5, lat + dl)) - map.heightAt(lon, std::max(-PI * 0.5, lat - dl));
                    double run = 2 * dl * R * 1000.0;
                    lit = clampd(lit + 5.0 * (hE * sE + hN * sN) / run, -1, 1);
                }
                double shade = ambient + (1 - ambient) * smoothstep(-0.08, 0.25, lit) * std::max(0.0, lit * 0.6 + 0.4 * std::max(0.0, lit));
                shade = ambient + (1 - ambient) * clampd(lit * 1.1 + 0.05, 0, 1) * lf;
                if (b.type == PT_SUBSTELLAR) shade = std::max(shade, bgen.selfGlow * (0.3 + 0.7 * alb));   // M5-02: it glows on its own, bands and all
                double v = exposureStop(alb, shade, atmosphere);   // N1-05: the ground's exposure rule
                if (mat == MAT_LAVA) v = std::max(v, 50.0 + 10.0 * alb);
                bool cloudy = false;
                if (atmosphere) {
                    double cl = haveMap ? map.cloudAt(lon + cloudDrift, lat) : 0.0;
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
        NebulaPatch nebP[16];
        int nebN = nebulaPatches(obs, nebP, 16);   // N5-01: the star-forming region's patches, blue, red or white
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
        drawSun(fb, sys.star, v / dist, angR, c.t, inten, 1, false, 0, false, proj);
        if (!c.vimana && v.z > 0) {
            double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
            double rpx = proj.f * std::tan(angR) * (dist / v.z);
            drawLensFlare(fb, sx, sy, std::max(rpx, 2.0 * FB_SCALE), inten * 0.7, 1, proj);
        }
    }
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
    double refKm = sys.bodies.empty() ? AU_GAME_KM : std::max(AU_GAME_KM * 0.5, sys.bodies[0].type == PT_COMET ? AU_GAME_KM : sys.bodies[0].orbitRadiusKm);
    double activity = clampd((refKm * 1.6 / dStar) * (refKm * 1.6 / dStar), 0, 1);
    if (activity < 0.02) return;
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
