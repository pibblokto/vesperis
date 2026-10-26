#include "space_view.h"
#include "space_internal.h"

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
    auto tb = std::chrono::steady_clock::now();   // F-01: the caller waits from here (a generation under way joined, or the map made here)
    joinMap(b.seed);
    PlanetMap* mm = maps[b.seed].get();
    if (!mm->valid) { auto t0 = std::chrono::steady_clock::now(); mm->generate(genFor(b)); mm->generating = false; genMicros.fetch_add((long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count()); }
    mapStats.blocked++; mapStats.blockedMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tb).count();
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
    auroraBank12 = sys && sys->valid && sys->sunless();   // D-02: no second sun: bank 12 holds a drifter's aurorae (hydrogen's pink and violet)
    if (auroraBank12) setRamp(fb.pal, 12, {{0, RGB(0, 0, 0)}, {12, RGB(0.22f, 0.05f, 0.2f)}, {24, RGB(0.55f, 0.16f, 0.5f)}, {40, RGB(0.85f, 0.4f, 0.75f)}, {63, RGB(1, 0.85f, 0.95f)}});
    auto lightOf = [&]() { return sys && sys->valid ? lerp(sys->star.color, white, 0.5f) : white; };
    auto genOf = [&](int bi) -> const BodyGen* { return sys && bi >= 0 && bi < (int)sys->bodies.size() ? &genFor(sys->bodies[bi]) : nullptr; };
    // N1-02: the body bank carries the rock family in the material palette's colour; molten and substellar keep their glow ramps
    auto bodyRamp = [&](int bank, int bi) {
        const BodyGen* g = genOf(bi);
        if (!g) { setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {63, RGB(0.7f, 0.7f, 0.7f)}}); return; }
        const Body& b = sys->bodies[bi];
        RGB c = g->matColor[familyRep(b.type, FAM_ROCK)] * lightOf();
        if (isLavaWorld(b.type) || (g->dark && g->hasTrait(TR_EMBERS))) {   // D-04: a drifter's ember fields glow in the lava's stops
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {30, c * 0.8f}, {46, RGB(0.55f, 0.28f, 0.18f)}, {52, RGB(0.9f, 0.3f, 0.05f)}, {58, RGB(1, 0.6f, 0.1f)}, {63, RGB(1, 0.95f, 0.6f)}});
        } else if (kindHas(b.type, KT_GLOWS)) {   // M5-02: its own dull red glow, hotter toward the bright end
            RGB gc = b.color;
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {10, gc * 0.35f}, {30, gc}, {46, lerp(gc, RGB(1, 0.55f, 0.25f), 0.6f)}, {58, RGB(1, 0.75f, 0.4f)}, {63, RGB(1, 0.9f, 0.7f)}});
        } else if (KINDS[b.type].giantClass == GC_ALKALI) {   // P-07: its night's dull red glow low in the ramp, the dark slate of its lit air over it
            const RGB red(0.40f, 0.08f, 0.04f);
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {6, RGB(0.19f, 0.035f, 0.02f)}, {13, red}, {19, RGB(0.11f, 0.05f, 0.07f)}, {32, c * 0.8f}, {46, c}, {56, lerp(c, white, 0.12f)}, {63, lerp(c, white, 0.28f)}});
        } else if (KINDS[b.type].giantClass == GC_SILICATE) {   // P-07: the glow of its night and its terminator low in the ramp (red to orange), its clouds of rock over it
            const RGB ember(0.78f, 0.24f, 0.05f), flame(0.96f, 0.52f, 0.16f);
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {7, RGB(0.34f, 0.05f, 0.02f)}, {15, ember}, {21, flame}, {30, lerp(flame, c, 0.6f)}, {44, c}, {56, lerp(c, white, 0.18f)}, {63, lerp(c, white, 0.4f)}});
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
        RGB oc = (g ? g->matColor[familyRep(g->type, FAM_WATER)] : RGB(0.10f, 0.28f, 0.62f)) * lightOf();   // P-01: the family's own (the glass world's glass)
        if (g && g->hasTrait(TR_LIGHTS_IN_WATER)) {   // D-04: a dark ocean's own light in the upper stops (its water never lit higher)
            const RGB L = g->waterTintColour;
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {14, oc * 0.3f}, {34, oc * 0.7f}, {44, L * 0.35f}, {54, L * 0.75f}, {63, lerp(L, white, 0.35f)}});
            return;
        }
        setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {14, oc * 0.3f}, {34, oc * 0.7f}, {47, oc}, {56, lerp(oc, white, 0.5f)}, {63, white}});
    };
    waterRamp(6, bodyA);
    waterRamp(7, bodyB);
    {   // Q-02 bank 31: the first disc's water trait colour (a bloom's or a lagoon's, else a pink lake's) in a water's ramp (the second disc's water
        // stays its own: the cabin holds the other free banks)
        const BodyGen* g = genOf(bodyA);
        if (g && (g->hasTrait(TR_BLOOMS) || g->hasTrait(TR_REEFS) || g->hasTrait(TR_PINK_LAKES))) {
            const RGB oc = (g->hasTrait(TR_BLOOMS) ? lerp(g->matColor[MAT_WATER], g->lifeTintColour, 0.75f) : (g->hasTrait(TR_REEFS) ? g->lifeTintColour : g->waterTintColour)) * lightOf();
            setRamp(fb.pal, 31, {{0, RGB(0, 0, 0)}, {14, oc * 0.3f}, {34, oc * 0.7f}, {47, oc}, {56, lerp(oc, white, 0.5f)}, {63, white}});
        }
    }
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

// S-05: a gas giant or a substellar object of a Wolf-Rayet star streams its envelope away in the wind
static bool strippedByWind(const StarSystem& sys, int bi) {
    const Body& b = sys.bodies[bi];
    return kindHas(b.type, KT_GIANT) && b.parent < 0 && sys.star.cls == STAR_WOLF_RAYET;
}

// G-07: the keyframes' spacing at a flight time: half a second, or the time the galactic centre takes to turn two degrees past
// the ship (the flight's speed over its distance from the centre, 1,000 ly at least), a twentieth of a second at the least
static double flightKeyStep(const Vec3& from, const Vec3& to, double dur, double t) {
    const double u = clampd(t / dur, 0, 1);
    const double speed = length(to - from) * 6 * u * (1 - u) / dur;   // ly a second: the smoothstep's slope
    const Vec3 p = lerp(from, to, vimanaProgress(u));
    const double w = speed / std::max(1000.0, length(p - Vec3(GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ)));
    return clampd(w > 1e-9 ? 2 * DEG / w : 0.5, 0.05, 0.5);
}

Vec3 SpaceRenderer::flightBandPos(double tt) const { return lerp(fband.from, fband.to, vimanaProgress(fband.dur > 0 ? tt / fband.dur : 1)); }

const std::vector<float>& SpaceRenderer::flightBand(const SpaceContext& c) {
    FlightBand& F = fband;
    const int W = BAND_MAP_W, H = BAND_MAP_H, N = W * H;
    const Vec3 from = c.flightFrom / SECTOR_KM, to = c.flightTo / SECTOR_KM;
    if (F.dur != c.flightDur || length(F.from - from) > 1e-6 || length(F.to - to) > 1e-6) {   // a new flight: its keyframes' times
        F = FlightBand();
        F.from = from; F.to = to; F.dur = std::max(1e-3, c.flightDur);
        F.tau.push_back(0);
        while (F.tau.back() < F.dur) {
            double next = F.tau.back() + flightKeyStep(from, to, F.dur, F.tau.back());
            if (next > F.dur - 0.025) next = F.dur;   // no sliver of an interval at the end
            F.tau.push_back(next);
        }
        if (bandValid && length(bandPos - from) <= 10.0) { F.keys[0].k = 0; F.keys[0].map = bandMap; F.keys[0].done = N; }   // the band the ship was seeing
    }
    const double t = clampd(c.flightT, 0, F.dur);
    const int last = (int)F.tau.size() - 1;
    const int k = clampi((int)(std::upper_bound(F.tau.begin(), F.tau.end(), t) - F.tau.begin()) - 1, 0, last - 1);
    auto find = [&](int kk) -> FlightBand::Key* { for (FlightBand::Key& key : F.keys) if (key.k == kk) return &key; return nullptr; };
    auto slot = [&](int kk) -> FlightBand::Key& {   // keyframe kk's key: its own, else one holding none of k .. k+2
        if (FlightBand::Key* key = find(kk)) return *key;
        for (FlightBand::Key& key : F.keys)
            if (key.k < k || key.k > k + 2) { key.k = kk; key.done = 0; key.map.assign((size_t)N, 0.f); return key; }
        return F.keys[0];
    };
    auto build = [&](FlightBand::Key& key, int cells) {   // the next `cells` of a keyframe (all that is left: -1)
        const int c1 = cells < 0 ? N : std::min(N, key.done + cells);
        if (c1 <= key.done) return 0;
        buildGalaxyBandCells(flightBandPos(F.tau[key.k]), key.map, W, H, key.done, c1);
        const int n = c1 - key.done;
        key.done = c1;
        if (key.done == N) F.built++;
        return n;
    };
    F.cellsFrame = 0;
    FlightBand::Key* A = find(k);
    if (!A || A->done < N) { FlightBand::Key& a = slot(k); F.cellsFrame += build(a, -1); F.forced++; A = &a; }   // the start without a parked band, a skip of the clock
    // the frame's share: a keyframe whole within half its interval at the clock's pace, a sixteenth of one at the least (a still
    // clock finishes too); the next keyframe first, then the one after it
    const double dtF = F.lastT >= 0 ? std::max(0.0, t - F.lastT) : 0.0;
    int budget = std::max(N / 16, (int)std::ceil(N * dtF / std::max(0.025, 0.5 * (F.tau[k + 1] - F.tau[k]))));
    for (int kk = k + 1; kk <= std::min(k + 2, last) && budget > 0; kk++) {
        FlightBand::Key& key = slot(kk);
        if (key.done == N) continue;
        const int n = build(key, budget);
        budget -= n; F.cellsFrame += n;
        if (key.done < N) break;
    }
    A = find(k);
    const FlightBand::Key* B = find(k + 1);
    const bool both = B && B->done == N;
    const double w = both ? clampd((t - F.tau[k]) / std::max(1e-9, F.tau[k + 1] - F.tau[k]), 0, 1) : 0;
    F.shown.resize((size_t)N);
    for (int i = 0; i < N; i++) F.shown[i] = both ? (float)(A->map[i] + ((double)B->map[i] - A->map[i]) * w) : A->map[i];
    if (both && k + 1 == last && !F.handed) { bandMap = B->map; bandPos = to; bandValid = true; bandGen++; F.handed = true; }   // the arrival's band, ready before it
    F.lastT = t; F.k = k;
    return F.shown;
}

void SpaceRenderer::render(Framebuffer& fb, const SpaceContext& c) {
    bodyInfo.clear();
    if (c.sys && c.sys->valid) setSeason(*c.sys, c.t);
    if (c.vimana) {
        // fade previous frame for streaks
        for (auto& p : fb.idx) { int v = intenOf(p) - 9 * INTEN_PER_SHADE; p = v > 0 ? pixI(bankOf(p), v) : 0; }
    }   // (outside a flight the frame is cleared by the backdrop's pass below, or replaced by its kept layer)
    fb.clearDepth();
    // M1-09: the galactic backdrop, a faint band of unresolved stars (rebuilt when the ship moved); G-07: in a Vimana flight
    // too, from the flight's keyframes (KI-364: the band was not drawn in a flight), under the streaks that fade into it
    if (!c.vimana || c.flightDur > 0) {
        Vec3 obs = c.shipPos / SECTOR_KM;
        const std::vector<float>* bandSrc = &bandMap;
        if (c.vimana) bandSrc = &flightBand(c);
        else if (!bandValid || length(obs - bandPos) > 10.0) { buildGalaxyBand(obs, bandMap, BAND_MAP_W, BAND_MAP_H); bandPos = obs; bandValid = true; bandGen++; parkedBandBuilds++; }   // G-03: rebuilt after a hop of ten sectors (the near dust's rifts move by a couple of degrees per hundred)
        Mat3 camT = c.cam.transposed();
        double invf = 1.0 / proj.f;
        const int step = 2;
        NebulaPatch nebP[18];
        int nebN = c.nebulae ? nebulaPatches(obs, nebP, 16) : 0;   // N5-01: the star-forming region's patches, blue, red or white
        if (c.vimana && c.flightDur > 0 && nebN) {
            // G-07: the patches lie within 42 ly, so past a few tens of light years a second they would come and go within a frame or two
            // (a 30-degree disc for one frame): a flight fades them by its speed, half at 15 ly a second, seen as it sets off and arrives
            const double u = clampd(c.flightT / c.flightDur, 0, 1), v = length(c.flightTo - c.flightFrom) / SECTOR_KM * 6 * u * (1 - u) / c.flightDur;
            const double fade = 1.0 / (1.0 + (v / 15) * (v / 15));
            for (int i = 0; i < nebN; i++) nebP[i].inten *= fade;
        }
        if (c.sys && c.sys->valid) nebN += starNebulaPatches(c.sys->star, normalize(c.sys->star.pos - c.shipPos), nebP + nebN);   // S-04: a protostar's own cloud
        // F-07: outside a flight and without patches the layer is a function of the key: kept while the camera is still
        const bool keyed = !c.vimana && nebN == 0;
        BackdropKey key{};
        if (keyed) {
            for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) key.cam[i * 3 + j] = c.cam.m[i][j];
            key.f = proj.f; key.cx = proj.cx; key.cy = proj.cy; key.w = FBW; key.h = FBH; key.band = bandGen;
        }
        auto same = [](const BackdropKey& a, const BackdropKey& b) { return std::memcmp(&a, &b, sizeof a) == 0; };
        const bool replay = keyed && backHave && same(key, backKey) && fb.idx.size() == backLayer.size();
        if (replay) std::memcpy(fb.idx.data(), backLayer.data(), backLayer.size() * sizeof(Pix));
        else {
            if (!c.vimana) fb.clear(0);
            parallelFor(FBH / step, 40, [&](int rb, int re) {
                for (int y = rb * step; y < re * step && y < FBH; y += step)
                    for (int x = 0; x < FBW; x += step) {
                        Vec3 d = normalize(Vec3((x + 0.5 - proj.cx) * invf, -(y + 0.5 - proj.cy) * invf, 1.0));
                        Vec3 dw = camT * d;
                        double band = sampleGalaxyBand(*bandSrc, dw);
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
            if (keyed && backPrevHave && same(key, backPrev)) { backLayer = fb.idx; backKey = key; backHave = true; }   // still for two frames: kept
        }
        backPrevHave = keyed;
        if (keyed) backPrev = key;
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
    // sun (B-308: drawn by angle, so it is right even with its centre beside or behind the camera; D-01: a drifter's place has none)
    if (!sys.sunless()) {
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
        if (strippedByWind(sys, i)) drawStrippedTail(fb, c, i, bank);   // S-05, B-427: after its globe, behind what is nearer
    }
    lastBeltRocks = lastBeltMeshes = 0;
    if (!c.skyMode) for (size_t k = 0; k < sys.belts.size(); k++) drawBeltRocks(fb, c, (int)k);   // O3: the rocks around a ship inside a belt
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
    for (auto& pr : order) {
        if (sys.bodies[pr.second].type == PT_COMPANION) continue;
        drawGlobe(fb, c, pr.second, 1, bodyInfo[pr.second]);
        if (strippedByWind(sys, pr.second)) drawStrippedTail(fb, c, pr.second, 1);   // S-05: in a sky in the sky's bank, as the globe's lit side
    }
}
