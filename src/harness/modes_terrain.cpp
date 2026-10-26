// E-03: the ground's statistics: `terrain`, `landforms`, `descent`, `matlod`, `landmarks`, `drain`, `water`, `climate`, the sites
// and their metrics
#include "harness/harness.h"
#include "harness/test_worlds.h"
static int classIndex(const char* cls) { return cls[0] == 'F' ? 0 : (cls[0] == 'H' ? 1 : 2); }

void gridHeights(const SurfaceSite& site, int N, double cell, std::vector<double>& h) {
    h.resize((size_t)(N + 1) * (N + 1));
    for (int j = 0; j <= N; j++)
        for (int i = 0; i <= N; i++) h[(size_t)j * (N + 1) + i] = site.sampleAt((i - N / 2) * cell, (j - N / 2) * cell, cell).h;
}
static void gridSlopes(const std::vector<double>& h, int N, double cell, std::vector<double>& out) {
    out.resize((size_t)N * N);
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            double gx = h[(size_t)j * (N + 1) + i + 1] - h[(size_t)j * (N + 1) + i], gz = h[(size_t)(j + 1) * (N + 1) + i] - h[(size_t)j * (N + 1) + i];
            out[(size_t)j * N + i] = std::sqrt(gx * gx + gz * gz) / cell;
        }
}
static double percentile(std::vector<double> v, double p) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); return v[std::min(v.size() - 1, (size_t)(p * v.size()))]; }
static double shareOver(const std::vector<double>& v, double thr) { if (v.empty()) return 0; int n = 0; for (double s : v) if (s > thr) n++; return (double)n / v.size(); }
static double reliefOf(const std::vector<double>& h) { double lo = 1e9, hi = -1e9; for (double v : h) { lo = std::min(lo, v); hi = std::max(hi, v); } return hi - lo; }

SiteMetrics measureSite(const SurfaceSite& site, bool timeIt) {
    SiteMetrics m;
    m.lat = site.lat0; m.lon = site.lon0;
    m.material = site.sampleAt(0, 0, 16).material;
    m.probe = Game::siteSlope(site);
    m.cls = classIndex(reliefClass(m.probe));
    std::vector<double> h, sl;
    gridHeights(site, 24, 4, h); gridSlopes(h, 24, 4, sl); m.med4 = percentile(sl, 0.5);
    double t0 = nowSec();
    gridHeights(site, 48, 16, h);
    if (timeIt) m.usPerSample = (nowSec() - t0) * 1e6 / (49.0 * 49.0);
    gridSlopes(h, 48, 16, m.slopes16); m.relief770 = reliefOf(h);
    gridHeights(site, 32, 64, h); m.relief2k = reliefOf(h);
    {
        gridSlopes(h, 32, 64, sl);
        // the lowest quarter of the cells and how many of them lie under a 5% slope: valley floors a buggy can follow
        std::vector<std::pair<double, double>> cells;   // height, slope
        int hi = 0; double best = -1e9;
        for (int j = 0; j < 32; j++)
            for (int i = 0; i < 32; i++) { double hv = h[(size_t)j * 33 + i]; cells.push_back({hv, sl[(size_t)j * 32 + i]}); if (hv > best) { best = hv; hi = j * 32 + i; } }
        m.hiYaw = std::atan2((double)(hi % 32 - 16), (double)(hi / 32 - 16));
        std::sort(cells.begin(), cells.end());
        int q = (int)cells.size() / 4, flat = 0;
        for (int k = 0; k < q; k++) if (cells[k].second < 0.05) flat++;
        m.floors = q ? (double)flat / q : 0;
    }
    gridHeights(site, 32, 512, h); gridSlopes(h, 32, 512, m.slopes512); m.relief16k = reliefOf(h);
    return m;
}

bool firstBodyOfType(int type, StarSystem& sysOut, int& biOut) {
    bool found = false;
    forTypeBodies(type, 1, false, [&](const StarSystem& sys, int bi) { sysOut = sys; biOut = bi; found = true; });
    return found;
}

// B-307: the buggy rolls down a mountain slope. The steepest straight descent of 200 m within 500 m of the capsule of
// the pinned felisian mountain site (grade 0.12 .. 0.45, no water), driven at full throttle: it must stay on its wheels
// (under a tenth of the frames airborne, at most two landings) and get to the bottom fast. A buggy launched each time the
// ground drops faster than 3 m/s lands with a thump, 15% of its speed gone, and launches again: a ladder.
int runDescent(double latDeg, double lonDeg) {   // `descent [latDeg lonDeg]` tries another site of the world
    std::vector<StarSystem> systems; std::vector<int> bodyOf;
    forTypeBodies(PT_FELISIAN, 1, false, [&](const StarSystem& sys, int bi) { systems.push_back(sys); bodyOf.push_back(bi); });
    if (systems.empty()) { printf("descent: no felisian body\n"); return 1; }
    const StarSystem& sys = systems[0]; int bi = bodyOf[0];
    double lat = latDeg * DEG, lon = lonDeg * DEG;   // the landforms review site "felisian_mountains"
    SurfaceView sv; sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, 35 * DEG, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
    StarNeighborhood nb; nb.update(sv.site.worldPos(t, 0, 0, 0));
    // the run: start points on a 50 m grid within 500 m, 24 headings, every 10 m step must descend
    double bx = 0, bz = 0, bh = 0, bestGrade = 0;
    const double LEN = 200;
    for (double sx = -500; sx <= 500; sx += 50)
        for (double sz = -500; sz <= 500; sz += 50)
            for (int k = 0; k < 24; k++) {
                double h = k * TAU / 24, hx = std::sin(h), hz = std::cos(h);
                double h0 = sv.site.groundHeight(sx, sz), h1 = sv.site.groundHeight(sx + hx * LEN, sz + hz * LEN);
                double grade = (h0 - h1) / LEN;
                if (grade < 0.12 || grade > 0.45 || grade <= bestGrade) continue;
                bool ok = true; double hp = h0, hpp = h0;
                std::vector<SurfaceView::Collider> cols;
                // every 2 m (10 m steps let a ledge through: the buggy launched from it), from the start itself (a rock
                // beside the start held it for ten seconds); no step up over 0.3 m, none down over 1.6 m (a cliff), a steady
                // grade (the profile bends under 0.5 m per 2 m step: the GEN 10 slopes are broken ground, and a bump of a metre
                // at 100 km/h is a jump, not the ladder B-307 tested), no water
                for (double d = 0; d <= LEN + 30 && ok; d += 2) {
                    double x = sx + hx * d, z = sz + hz * d, hh = sv.site.groundHeight(x, z);
                    if (hh > hp + 0.3 || hh < hp - 1.6 || hh < sv.site.waterAt(x, z) + 0.5) ok = false;
                    if (d >= 4 && std::fabs(hpp - 2 * hp + hh) > 0.5) ok = false;
                    hpp = hp; hp = hh;
                    // no rock over 0.9 m, trunk, log or ruin within 3 m of the line, as `findOpenRun` asks (an alpine
                    // site's boulders stopped the buggy at 37 m); small rocks are bumps the wheels ride over
                    sv.collectColliders(x, z, cols);
                    for (const SurfaceView::Collider& c : cols) {
                        if (c.kind == 0 && c.r < 0.72) continue;
                        double dx = c.x - x, dz = c.z - z, along = dx * hx + dz * hz, across = -dx * hz + dz * hx;
                        if (std::fabs(along) < 9 && std::fabs(across) < 3 + c.r) { ok = false; break; }
                    }
                }
                if (ok) { bx = sx; bz = sz; bh = h; bestGrade = grade; }
            }
    if (bestGrade <= 0) { printf("descent: no slope found\n"); return 1; }
    sv.relocateCapsule(bx - std::sin(bh) * 6, bz - std::cos(bh) * 6);
    sv.player.x = bx; sv.player.z = bz; sv.player.y = sv.site.surfaceHeight(bx, bz);   // the buggy deploys beside the capsule, near the explorer
    if (!sv.deployBuggy()) { printf("descent: could not deploy\n"); return 1; }
    sv.buggy.x = bx; sv.buggy.z = bz; sv.buggy.y = sv.site.surfaceHeight(bx, bz); sv.buggy.heading = bh; sv.buggy.speed = 0; sv.buggy.unfold = 1;
    sv.player.x = bx + 1.5; sv.player.z = bz; sv.player.y = sv.buggy.y;
    if (!sv.inBuggy && !sv.toggleBuggy()) { printf("descent: could not get in\n"); return 1; }
    Input in;
    int frames = 0, air = 0, landings = 0; bool wasAir = false; double along = 0, sumSpeed = 0, arriveT = -1, thumps = 0;
    double hx = std::sin(bh), hz = std::cos(bh);
    std::string trace;   // where it got every two seconds (along, across, speed, heading error), to see a stall
    for (int i = 0; i < 60 * 25 && along < LEN; i++) {
        in.newFrame(); in.down[KEY_W] = true;
        double err = wrapAngle(bh - sv.buggy.heading);
        if (std::fabs(err) > 3 * DEG) in.down[err > 0 ? KEY_D : KEY_A] = true;
        if (i % 120 == 0) trace += fmt(" %ds: %.0f/%.0f m %.1f m/s %+.0f deg%s;", i / 60, along, (sv.buggy.x - bx) * hz - (sv.buggy.z - bz) * hx, sv.buggy.speed, err / DEG, sv.buggy.airborne ? " air" : "");
        sv.update(1.0 / 60, in, t + i / 60.0, true);
        frames++;
        if (sv.buggy.airborne) air++;
        if (wasAir && !sv.buggy.airborne) landings++;
        wasAir = sv.buggy.airborne;
        thumps = std::max(thumps, sv.buggy.thud);
        along = (sv.buggy.x - bx) * hx + (sv.buggy.z - bz) * hz;
        sumSpeed += std::fabs(sv.buggy.speed);
        if (along >= LEN && arriveT < 0) arriveT = i / 60.0;
    }
    double airFrac = frames ? (double)air / frames : 1.0, meanSpeed = frames ? sumSpeed / frames : 0;
    bool ok = arriveT > 0 && airFrac < 0.1 && landings <= 2 && meanSpeed > 8;
    printf("descent: grade %.2f over %.0f m from (%.0f, %.0f); %.1f s to the bottom, mean %.1f m/s, airborne %.0f%% of the frames, %d landings, worst thump %.2f\n",
           bestGrade, LEN, bx, bz, arriveT, meanSpeed, airFrac * 100, landings, thumps);
    printf("descent: %s\n", ok ? "ok (on its wheels, under 10%% airborne, at most two landings)" : "FAIL");
    if (!ok) printf("  trace:%s\n", trace.c_str());
    return ok ? 0 : 1;
}
// hashed land sites of a body between 60 S and 60 N (water when a world has no land within 50 draws)
void landSitesOf(const StarSystem& sys, int bi, uint64_t salt, int n, std::vector<std::pair<double, double>>& out) {
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    Rng r(b.seed ^ salt);
    for (int k = 0; k < n; k++) {
        double lat = 0, lon = 0;
        for (int tries = 0; tries < 50; tries++) {
            lat = r.range(-60 * DEG, 60 * DEG); lon = r.range(-PI, PI);
            if (sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), 64).material != MAT_WATER) break;
        }
        out.push_back({lat, lon});
    }
}

void materialAgreement(const StarSystem& sys, int bi, double lat, double lon, double halfM, double stepM, double out[2]) {
    SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
    drainagePrefetch(st.gen, st.up0, halfM * 1.5 + 3000.0, false);
    int N = (int)std::floor(halfM / stepM), W = 2 * N + 1; long n = 0, d[2] = {0, 0};
    for (int iz = 0; iz < W; iz++)
        for (int ix = 0; ix < W; ix++) {
            Vec3 u = st.unitAt((ix - N) * stepM, (iz - N) * stepM);
            int m4 = sampleSurface(st.gen, u, 4).material, m16 = sampleSurface(st.gen, u, 16).material, m64 = sampleSurface(st.gen, u, 64).material;
            n++; if (m4 != m16) d[0]++; if (m16 != m64) d[1]++;
        }
    out[0] = 100.0 * d[0] / std::max(1L, n); out[1] = 100.0 * d[1] / std::max(1L, n);
}

// The targets of section 2 of the plan per class; `later` names the item that owns a target that is not part of the
// spectrum work (cliffs and broken plains come with the landform library), so it is reported but not counted yet.
struct ClassAgg { int n = 0; std::vector<double> s16, s512; double med4 = 0, r770 = 0, r2k = 0, r16k = 0, floors = 0; };
static int judgeClass(int cls, const ClassAgg& a, bool smallBody, bool craterFields, std::string& notes, int& later) {
    int fails = 0;
    auto range = [&](const char* name, double v, double lo, double hi, const char* owner, const char* fmt) {
        bool ok = v >= lo && v <= hi;
        char buf[96]; snprintf(buf, sizeof buf, fmt, v);
        notes += std::string(name) + " " + buf + (ok ? " ok" : (owner ? std::string(" FAIL(") + owner + ")" : " FAIL")) + "; ";
        if (!ok) { if (owner) later++; else fails++; }
    };
    double med16 = percentile(a.s16, 0.5), p90 = percentile(a.s16, 0.9);
    double cliff = shareOver(a.s16, 1.0), walk = 1 - shareOver(a.s16, 1.35);
    if (smallBody) {   // a comet is smaller than the windows and craggy by nature (O4): only the walking rule applies
        range("walkable", walk * 100, 85, 100, nullptr, "%.0f%%");
    } else if (cls == 2) {
        range("median", med16, 0.25, 0.45, nullptr, "%.2f");
        range("2km", a.r2k, 500, 1200, nullptr, "%.0fm");
        range("16km", a.r16k, 1500, craterFields ? 5000 : 3500, nullptr, "%.0fm");   // a basin's wall in the window adds kilometres
        range("cliffs", cliff * 100, 3, 8, nullptr, "%.1f%%");
        range("walkable", walk * 100, 90, 100, nullptr, "%.0f%%");
        range("floors", a.floors * 100, 15, 100, nullptr, "%.0f%%");   // the valley floors
    } else if (cls == 1) {
        range("median", med16, 0.08, 0.15, nullptr, "%.2f");
        range("2km", a.r2k, 120, craterFields ? 450 : 250, nullptr, "%.0fm");   // a crater in the window adds its depth
        range("floors", a.floors * 100, 25, 100, craterFields ? "O6-05" : nullptr, "%.0f%%");   // crater floors and terraces come with the second landform library
    } else {
        range("median", med16, 0.01, craterFields ? 0.09 : 0.04, nullptr, "%.3f");   // a plain of a cratered type is a crater field
        range("broken p90", p90, 0.06, 10, nullptr, "%.3f");
        range("broken 770m", a.r770, 6, 1e9, nullptr, "%.0fm");
    }
    return fails;
}

int g_terrainType = -1;   // `terrain <type id>`: one type only (the tuning loop)
int runTerrainStats() {
    // O5 (before O6-02): microseconds per 16 m sample on the development Mac, measured by this harness; the budget is 2x
    static const double O5_COST[] = {0.8, 1.1, 0.5, 0.8, 0.9, 1.3, 0, 0.9, 0.4, 0.2, 0.8, 0.4, 0.5, 0, 1.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};   // 0 for the kinds after O5 (no figure to hold them to)
static_assert(sizeof(O5_COST) / sizeof(O5_COST[0]) == PT_COUNT, "an O5 cost (or 0) for every kind");
    printf("%-16s %-9s %3s | %5s | %-40s | %5s %6s %6s | %6s | %-13s | %s\n", "type", "class", "n", "4m", "16 m: med  p90   max  cliff  walk", "770m", "2km", "16km", "floors", "512m: med p90", "cost us (O5, x)");
    int fails = 0, later = 0, rows = 0, rowsOk = 0;
    for (int type = 0; type < PT_COUNT; type++) {
        if (!KINDS[type].landable || (g_terrainType >= 0 && type != g_terrainType)) continue;
        std::vector<StarSystem> systems; std::vector<int> bodyOf; std::vector<std::pair<double, double>> sites; std::vector<int> sysOf;
        forTypeBodies(type, 4, false, [&](const StarSystem& sys, int bi) {
            systems.push_back(sys); bodyOf.push_back(bi);
            size_t n0 = sites.size();
            landSitesOf(sys, bi, 0x06, 12, sites);
            for (size_t k = n0; k < sites.size(); k++) sysOf.push_back((int)systems.size() - 1);
        });
        if (sites.empty()) { printf("%-16s no body found\n", KINDS[type].name); continue; }
        std::vector<SiteMetrics> ms(sites.size());
        parallelFor((int)sites.size(), 1, [&](int b, int e) {
            for (int i = b; i < e; i++) {
                SurfaceSite st; st.init(&systems[sysOf[i]], bodyOf[sysOf[i]], sites[i].first, sites[i].second, 1000.0); ms[i] = measureSite(st, false);
                if (std::getenv("VESPERIS_TRACE") && ms[i].cls == 0 && typeHasDrainage(type) && i < 6) {   // what flattens a plain (the flat class's median grade)
                    std::vector<double> sOther, sValley, sWater, sLake; int n = 0;
                    for (int z = -24; z < 24; z++) for (int x = -24; x < 24; x++) {
                        double h00 = st.sampleAt(x * 16.0, z * 16.0, 16).h, h10 = st.sampleAt(x * 16.0 + 16, z * 16.0, 16).h, h01 = st.sampleAt(x * 16.0, z * 16.0 + 16, 16).h;
                        double sl = std::sqrt((h10 - h00) * (h10 - h00) + (h01 - h00) * (h01 - h00)) / 16.0;
                        TerrainVertex tv = st.sampleAt(x * 16.0, z * 16.0, 16);
                        DrainInfo D = drainageAt(st.gen, st.unitAt(x * 16.0, z * 16.0), 16.0, 0.5);
                        n++;
                        if (tv.water > -1e8f && tv.h < tv.water) sWater.push_back(sl); else if (D.lake > 0.3) sLake.push_back(sl); else if (D.valley > 0.3) sValley.push_back(sl); else sOther.push_back(sl);
                    }
                    auto med = [](std::vector<double> v) { if (v.empty()) return 0.0; std::sort(v.begin(), v.end()); return v[v.size() / 2]; };
                    fprintf(stderr, "  flat site %.3f %.3f: water %zu (med %.4f) lake %zu (%.4f) valley %zu (%.4f) other %zu (%.4f) of %d; site median %.4f\n", sites[i].first / DEG, sites[i].second / DEG,
                            sWater.size(), med(sWater), sLake.size(), med(sLake), sValley.size(), med(sValley), sOther.size(), med(sOther), n, percentile(ms[i].slopes16, 0.5));
                }
                if (std::getenv("VESPERIS_TRACE") && ms[i].cls == 2 && typeHasDrainage(type)) {   // what the drainage says over a mountain window (the floors' tuning)
                    int nearN = 0, valleyN = 0, floorN = 0, lakeN = 0; double accMax = 0;
                    for (int z = -16; z < 16; z++) for (int x = -16; x < 16; x++) {
                        DrainInfo D = drainageAt(st.gen, st.unitAt(x * 64.0 + 32, z * 64.0 + 32), 64.0, 0.5);
                        if (D.near) nearN++; if (D.valley > 0.3) valleyN++; if (D.valley > 0.9) floorN++; if (D.lake > 0.5) lakeN++; accMax = std::max(accMax, D.acc);
                    }
                    fprintf(stderr, "  mtn site %.3f %.3f: near %d valley %d floor %d lake %d of 1024, max acc %.1f km2, floors %.0f%%\n", sites[i].first / DEG, sites[i].second / DEG, nearN, valleyN, floorN, lakeN, accMax, ms[i].floors * 100);
                    for (int z = -16; z < 16; z += 8) {   // rows of cells: the nearest channel's distance and accumulation, the valley
                        fprintf(stderr, "    row z=%d:", z);
                        for (int x = -16; x < 16; x += 2) { DrainInfo D = drainageAt(st.gen, st.unitAt(x * 64.0 + 32, z * 64.0 + 32), 64.0, 0.5); fprintf(stderr, " %s%.0f/%.1f/%.2f", D.near ? "" : "-", D.dist, D.acc, D.valley); }
                        fprintf(stderr, "\n");
                    }
                }
            }
        });
        double cost = 1e9;
        {   // the cost, serially, at the first site: the best of three passes over the 16 m grid
            SurfaceSite st; st.init(&systems[sysOf[0]], bodyOf[sysOf[0]], sites[0].first, sites[0].second, 1000.0);
            std::vector<double> h;
            for (int pass = 0; pass < 3; pass++) { double t0 = nowSec(); gridHeights(st, 48, 16, h); cost = std::min(cost, (nowSec() - t0) * 1e6 / (49.0 * 49.0)); }
        }
        ClassAgg agg[3];
        for (const SiteMetrics& m : ms) {
            ClassAgg& a = agg[m.cls];
            a.n++; a.s16.insert(a.s16.end(), m.slopes16.begin(), m.slopes16.end()); a.s512.insert(a.s512.end(), m.slopes512.begin(), m.slopes512.end());
            a.med4 += m.med4; a.r770 += m.relief770; a.r2k += m.relief2k; a.r16k += m.relief16k; a.floors += m.floors;
        }
        bool costOk = O5_COST[type] <= 0 || cost <= 2.0 * O5_COST[type];
        if (!costOk) fails++;
        for (int cls = 2; cls >= 0; cls--) {
            ClassAgg& a = agg[cls];
            if (!a.n) continue;
            a.med4 /= a.n; a.r770 /= a.n; a.r2k /= a.n; a.r16k /= a.n; a.floors /= a.n;
            std::string notes; int f = 0, l = 0;
            bool judged = type != PT_OCEAN && a.n >= 2;
            bool craterFields = type == PT_CRATERED || type == PT_THINATMO || type == PT_METAL || type == PT_ROCKY || type == PT_ICY || type == PT_CARBON || type == PT_BOMBARDED;
            if (judged) { f = judgeClass(cls, a, systems[0].bodies[bodyOf[0]].radiusKm < 300, craterFields, notes, l); fails += f; later += l; rows++; if (!f) rowsOk++; }
            printf("%-16s %-9s %3d | %.3f | %.3f %.3f %5.2f %4.1f%% %4.0f%% | %4.0fm %5.0fm %5.0fm | %4.0f%% | %.4f %.4f | %s\n", KINDS[type].name, CLASS_NAMES[cls], a.n, a.med4,
                   percentile(a.s16, 0.5), percentile(a.s16, 0.9), a.s16.empty() ? 0.0 : *std::max_element(a.s16.begin(), a.s16.end()), shareOver(a.s16, 1.0) * 100, (1 - shareOver(a.s16, 1.35)) * 100,
                   a.r770, a.r2k, a.r16k, a.floors * 100, percentile(a.s512, 0.5), percentile(a.s512, 0.9), judged ? notes.c_str() : (a.n < 2 ? "(one site: not judged)" : "(not judged)"));
        }
        printf("%-16s classes of %zu land sites: flat %d, hills %d, mountains %d; cost %.1f us per 16 m sample (O5 %.1f, x%.2f%s)\n", "", ms.size(), agg[0].n, agg[1].n, agg[2].n, cost, O5_COST[type],
               O5_COST[type] > 0 ? cost / O5_COST[type] : 0.0, costOk ? "" : ", OVER 2x");
    }
    printf("terrain: %d of %d class rows meet every target of the spectrum; %d spectrum targets failing, %d targets of later items (cliffs and broken plains: O6-04; mountain floors: O6-03) not yet met\n", rowsOk, rows, fails, later);
    return fails;
}

// A shaded relief of an N x N grid of `cell` metres round the site, 2 px per cell: the light from the north-west at 45
// degrees, water blue, snow and ice pale; what a DEM hillshade shows a geologist.
void hillshade(const SurfaceSite& site, int N, double cell, double zFactor, std::vector<uint32_t>& px) {
    std::vector<double> h((size_t)(N + 1) * (N + 1)), w(h.size()); std::vector<uint8_t> mat(h.size());
    for (int j = 0; j <= N; j++)
        for (int i = 0; i <= N; i++) { TerrainVertex v = site.sampleAt((i - N / 2) * cell, (j - N / 2) * cell, cell); size_t k = (size_t)j * (N + 1) + i; h[k] = v.h; w[k] = v.water; mat[k] = v.material; }
    px.assign((size_t)N * N * 4, 0);
    Vec3 L = normalize(Vec3(-1, 1.414, 1));
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            size_t k = (size_t)j * (N + 1) + i;
            double gx = (h[k + 1] - h[k]) / cell * zFactor, gz = (h[k + N + 1] - h[k]) / cell * zFactor;
            Vec3 n = normalize(Vec3(-gx, 1, -gz));
            double l = 0.18 + 0.82 * std::max(0.0, dot(n, L));
            uint32_t c;
            if (mat[k] == MAT_WATER || (w[k] > -1e8 && w[k] > h[k])) c = rgb((int)(30 + 40 * l), (int)(50 + 70 * l), (int)(110 + 100 * l));
            else if (mat[k] == MAT_SNOW || mat[k] == MAT_ICE) c = rgb((int)(90 + 165 * l), (int)(95 + 160 * l), (int)(110 + 145 * l));
            else c = rgb((int)(20 + 215 * l), (int)(20 + 205 * l), (int)(20 + 195 * l));
            int oy = (N - 1 - j) * 2, ox = i * 2;   // north up
            for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) px[(size_t)(oy + dy) * N * 2 + ox + dx] = c;
        }
}

// one frame of a site: standing at eye height (alt < 0), or `alt` metres above the ground looking down `pitch`; the sun at 35 deg
double renderSiteFrame(const StarSystem& sys, int bi, double lat, double lon, double yaw, double alt, double pitch, SpaceRenderer& sr, StarNeighborhood& nb, std::vector<uint32_t>& px) {
    SurfaceView sv;
    sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, 35 * DEG, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
    sv.lamp = sys.sunless();   // D-04: in a drifter's dark the explorer's lamp (no light else)
    sv.player.yaw = yaw; sv.player.pitch = pitch;
    sv.player.x = std::sin(yaw) * 14; sv.player.z = std::cos(yaw) * 14;   // 14 m ahead of the capsule, which stays behind the camera
    sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
    if (alt >= 0) { sv.cameraOverrideAlt = alt; sv.cameraOverridePitch = pitch; }
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    Input in; sv.update(0.016, in, t, false);
    Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
    px.resize((size_t)FBW * FBH); fb.toRGB(px.data(), 1.0);
    return t;
}
static std::string metricsShort(const SiteMetrics& m) {
    char buf[128];
    snprintf(buf, sizeof buf, "%s %.2f MED %.2f P90 %.2f 2KM %.0fM 16KM %.0fM", CLASS_NAMES[m.cls], m.probe, percentile(m.slopes16, 0.5), percentile(m.slopes16, 0.9), m.relief2k, m.relief16k);
    return buf;
}
static std::string metricsLine(const SiteMetrics& m) {
    char buf[256];
    snprintf(buf, sizeof buf, "%s %.2f  MED %.2f P90 %.2f CLIFF %.0f%%  770M %.0fM 2KM %.0fM 16KM %.0fM FLOORS %.0f%%", CLASS_NAMES[m.cls], m.probe, percentile(m.slopes16, 0.5), percentile(m.slopes16, 0.9),
             shareOver(m.slopes16, 1.0) * 100, m.relief770, m.relief2k, m.relief16k, m.floors * 100);
    return buf;
}

// `landforms <type> [flat|hills|mountains]`: twelve hashed land sites of the type (of that class) on one sheet, each a
// standing frame toward the highest ground within a kilometre over hillshades of 770 m, 3 km and 24 km
void renderLandforms(int type, int wantCls) {
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    const int TW = 320, TH = 200 + 96 + 14;
    Sheet sheet(TW * 3, TH * 4);
    int n = 0;
    std::vector<StarSystem> systems; std::vector<int> bodyOf;
    forTypeBodies(type, 4, false, [&](const StarSystem& sys, int bi) { systems.push_back(sys); bodyOf.push_back(bi); });
    for (size_t si = 0; si < systems.size() && n < 12; si++) {
        std::vector<std::pair<double, double>> sites;
        landSitesOf(systems[si], bodyOf[si], 0x60 + wantCls, 60, sites);
        for (auto& s : sites) {
            if (n >= 12) break;
            SurfaceSite st; st.init(&systems[si], bodyOf[si], s.first, s.second, 1000.0);
            SiteMetrics m = measureSite(st, false);
            if (wantCls >= 0 && m.cls != wantCls) continue;
            if (m.material == MAT_WATER || m.material == MAT_SNOW || m.material == MAT_ICE) continue;
            std::vector<uint32_t> px;
            renderSiteFrame(systems[si], bodyOf[si], s.first, s.second, m.hiYaw, -1, 0.02, sr, nb, px);
            int ox = (n % 3) * TW, oy = (n / 3) * TH;
            sheet.blit(px, FBW, FBH, ox, oy);
            const double cells[3] = {16, 64, 512}, zf[3] = {1, 2, 3}; const char* names[3] = {"770 M", "3 KM Z2", "24 KM Z3"};
            for (int k = 0; k < 3; k++) { hillshade(st, 48, cells[k], zf[k], px); sheet.blit(px, 96, 96, ox + 8 + k * 104, oy + 200 + 2); sheet.label(ox + 8 + k * 104, oy + 200 + 2, names[k]); }
            std::string label = systems[si].bodies[bodyOf[si]].name + "  " + std::to_string((int)std::lround(s.first / DEG)) + "/" + std::to_string((int)std::lround(s.second / DEG)) + "  " + MATERIAL_NAMES[m.material];
            for (char& c : label) c = (char)toupper((unsigned char)c);
            sheet.label(ox, oy, label);
            sheet.label(ox, oy + 200 + 98, metricsShort(m), rgb(200, 230, 255));
            printf("%2d %-24s %s\n", n, label.c_str(), metricsLine(m).c_str());
            n++;
        }
    }
    std::string tn = KINDS[type].name;
    for (char& ch : tn) if (ch == ' ') ch = '_';
    sheet.save("shots/tests/landforms_" + tn + (wantCls >= 0 ? std::string("_") + CLASS_NAMES[wantCls] : std::string()) + ".png");
    printf("landforms: %d sites of %s%s%s\n", n, KINDS[type].name, wantCls >= 0 ? " of class " : "", wantCls >= 0 ? CLASS_NAMES[wantCls] : "");
}

// `landforms sites`: the fixed sites of the O6 review, each on its own sheet at three distances (standing, 150 m up
// looking down 12 deg, 2 km up looking down 30 deg) over the three hillshades and the metrics; the latitude and
// longitude are fixed so the sheets before and after a generator change show the same ground (0/0 searches a site of
// the wanted class on the first body of the type and prints what to pin)
struct LandformSite { const char* name; int type; bool moon; int wantCls; double latDeg, lonDeg; };
static const LandformSite LANDFORM_SITES[] = {
    {"felisian_mountains", PT_FELISIAN, false, 2, PIN_MOUNTAIN_LAT, PIN_MOUNTAIN_LON},   // pinned 2026-09-27 on the generation-5 function; O6-03: re-pinned on the GEN 10 bodies (the type table of GEN 9 had moved every world)
    {"felisian_plain", PT_FELISIAN, false, 0, PIN_PLAIN_LAT, PIN_PLAIN_LON},   // re-pinned on Wyariothmai I's day side (a sand plain: the lit face of the locked world is dry); 7.424 / 75.762 before, in the sea now
    {"thinatmo_hills", PT_THINATMO, false, 1, 44.433, 135.970},
    {"cratered_moon", PT_CRATERED, true, 1, -13.184, 175.712},   // re-pinned on Wyariothmai II-a, the scan's first cratered moon since the claim (43.934 / -41.488 before)
};
void renderLandformSites(bool search) {   // G-04: `landforms sites search` lists six candidates of each site's class on its world (as a 0/0 pin does)
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    for (const LandformSite& L : LANDFORM_SITES) {
        std::vector<StarSystem> systems; std::vector<int> bodyOf;
        forTypeBodies(L.type, 1, L.moon, [&](const StarSystem& sys, int bi) { systems.push_back(sys); bodyOf.push_back(bi); });
        if (systems.empty()) { printf("%s: no body\n", L.name); continue; }
        const StarSystem& sys = systems[0]; int bi = bodyOf[0];
        double lat = L.latDeg * DEG, lon = L.lonDeg * DEG;
        if (search || (L.latDeg == 0 && L.lonDeg == 0)) {
            std::vector<std::pair<double, double>> sites;
            landSitesOf(sys, bi, 0x51, 400, sites);
            bool found = false; int listed = 0;
            for (auto& s : sites) {
                SurfaceSite st; st.init(&sys, bi, s.first, s.second, 1000.0);
                if (sys.bodies[bi].locked && st.sun(1000.0).altitude < 10 * DEG) continue;   // a world locked to its star has a day side and a night side for good: the sheets and the stability frames want the light
                SiteMetrics m = measureSite(st, false);
                if (m.cls != L.wantCls || m.material == MAT_WATER || m.material == MAT_SNOW || m.material == MAT_ICE) continue;
                if (!found) { lat = s.first; lon = s.second; found = true; }
                printf("%s: %s lat %.3f lon %.3f (%s, %s)\n", L.name, found && listed == 0 ? "pin" : "or ", s.first / DEG, s.second / DEG, CLASS_NAMES[m.cls], MATERIAL_NAMES[m.material]);   // the first six candidates, for `descent lat lon`
                if (++listed >= 6) break;
            }
            if (!found) { printf("%s: no site of class %s found\n", L.name, CLASS_NAMES[L.wantCls]); continue; }
        }
        SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
        SiteMetrics m = measureSite(st, true);
        Sheet sheet(960, 200 + 96 + 30);
        const double alts[3] = {-1, 150, 2000}, pitches[3] = {0.02, -12 * DEG, -30 * DEG}; const char* dist[3] = {"STANDING", "150 M UP", "2 KM UP"};
        std::vector<uint32_t> px;
        for (int k = 0; k < 3; k++) { renderSiteFrame(sys, bi, lat, lon, m.hiYaw, alts[k], pitches[k], sr, nb, px); sheet.blit(px, FBW, FBH, k * 320, 0); sheet.label(k * 320, 0, dist[k]); }
        const double cells[3] = {16, 64, 512}, zf[3] = {1, 2, 3}; const char* names[3] = {"770 M", "3 KM Z2", "24 KM Z3"};
        for (int k = 0; k < 3; k++) { hillshade(st, 48, cells[k], zf[k], px); sheet.blit(px, 96, 96, 8 + k * 104, 202); sheet.label(8 + k * 104, 202, names[k]); }
        std::string title = std::string(L.name) + "  " + sys.bodies[bi].name + "  LAT " + std::to_string(lat / DEG).substr(0, 6) + " LON " + std::to_string(lon / DEG).substr(0, 7) + "  " + MATERIAL_NAMES[m.material];
        for (char& c : title) c = (char)toupper((unsigned char)c);
        char l2[160]; snprintf(l2, sizeof l2, "16 M: MEDIAN %.3f  P90 %.3f  MAX %.2f  CLIFF %.1f%%  WALKABLE %.0f%%  4 M MEDIAN %.3f", percentile(m.slopes16, 0.5), percentile(m.slopes16, 0.9), *std::max_element(m.slopes16.begin(), m.slopes16.end()),
                 shareOver(m.slopes16, 1.0) * 100, (1 - shareOver(m.slopes16, 1.35)) * 100, m.med4);
        char l3[160]; snprintf(l3, sizeof l3, "RELIEF 770 M %.0f M   2 KM %.0f M   16 KM %.0f M   FLOORS %.0f%%   512 M: MEDIAN %.4f P90 %.4f", m.relief770, m.relief2k, m.relief16k, m.floors * 100, percentile(m.slopes512, 0.5), percentile(m.slopes512, 0.9));
        char l4[160]; snprintf(l4, sizeof l4, "LANDING MAP CLASS %s (PROBE %.3f)   COST %.1f US PER 16 M SAMPLE   GEN %d", CLASS_NAMES[m.cls], m.probe, m.usPerSample, GEN_VERSION);
        const char* const lines[4] = {title.c_str(), l2, l3, l4};
        for (int k = 0; k < 4; k++) drawText(sheet.cv, 330, 204 + k * 11, lines[k], k ? rgb(200, 230, 255) : rgb(255, 220, 120));
        sheet.save(std::string("shots/tests/landforms_site_") + L.name + ".png");
        printf("%-20s %s\n", L.name, metricsLine(m).c_str());
    }
}

// B-404: the planet function's materials (and heights) at the five ring scales, point by point.
int runMatlod(int argc, char** argv) {
    // `matlod [all|<type>] [bodies] [sites] [halfKm] [stepM]` over random land sites, or `matlod spot <sx> <sy> <sz> <body>
    // <latDeg> <lonDeg> [halfKm] [stepM]` round one place: the share of points whose material differs between
    // neighbouring scales (4/16, 16/64, 64/512, 512/2048 m), the pairs that differ, and the rms height difference
    static const double SCALES[5] = {4, 16, 64, 512, 2048};
    static const char* const STEPS[4] = {"4/16", "16/64", "64/512", "512/2k"};
    struct Agg { long n = 0; long diff[4] = {0, 0, 0, 0}; double h2[4] = {0, 0, 0, 0}; std::map<std::pair<int, int>, long> pairs[4]; };
    auto measure = [&](const StarSystem& sys, int bi, double lat, double lon, double halfM, double stepM, Agg& a) {
        SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
        drainagePrefetch(st.gen, st.up0, halfM * 1.5 + 3000.0, false);
        int N = (int)std::floor(halfM / stepM), W = 2 * N + 1;
        std::vector<uint8_t> mat((size_t)W * W * 5); std::vector<float> hgt((size_t)W * W * 5);
        parallelFor(W, 2, [&](int b, int e) {
            for (int iz = b; iz < e; iz++)
                for (int ix = 0; ix < W; ix++) {
                    Vec3 u = st.unitAt((ix - N) * stepM, (iz - N) * stepM);
                    for (int k = 0; k < 5; k++) { SurfaceSample s = sampleSurface(st.gen, u, SCALES[k]); size_t i = ((size_t)iz * W + ix) * 5 + k; mat[i] = (uint8_t)s.material; hgt[i] = (float)s.height; }
                }
        });
        for (size_t i = 0; i < (size_t)W * W; i++) {
            a.n++;
            for (int k = 0; k < 4; k++) {
                int ma = mat[i * 5 + k], mb = mat[i * 5 + k + 1];
                if (ma != mb) { a.diff[k]++; a.pairs[k][{ma, mb}]++; }
                double dh = hgt[i * 5 + k] - hgt[i * 5 + k + 1]; a.h2[k] += dh * dh;
            }
        }
    };
    auto report = [&](const std::string& name, const Agg& a) {
        if (!a.n) { printf("%-18s no sites\n", name.c_str()); return; }
        printf("%-18s %7ld pts |", name.c_str(), a.n);
        for (int k = 0; k < 4; k++) printf("  %s %5.2f%% (h %5.1f m)", STEPS[k], 100.0 * a.diff[k] / a.n, std::sqrt(a.h2[k] / a.n));
        printf("\n");
        for (int k = 0; k < 4; k++) {
            if (!a.diff[k]) continue;
            std::vector<std::pair<long, std::pair<int, int>>> v; for (auto& kv : a.pairs[k]) v.push_back({kv.second, kv.first});
            std::sort(v.rbegin(), v.rend());
            printf("    %-7s", STEPS[k]);
            for (size_t j = 0; j < v.size() && j < 4; j++) printf("  %s>%s %.2f%%", MATERIAL_NAMES[v[j].second.first], MATERIAL_NAMES[v[j].second.second], 100.0 * v[j].first / a.n);
            printf("\n");
        }
    };
    if (argc > 2 && std::string(argv[2]) == "spot") {
        if (argc < 9) { printf("matlod spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> [halfKm] [stepM]\n"); return 1; }
        Star s; StarSystem sys; if (!systemAt(atoll(argv[3]), atoll(argv[4]), atoll(argv[5]), s, sys)) { printf("no star\n"); return 1; }
        int bi = atoi(argv[6]); if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
        double halfM = (argc > 9 ? atof(argv[9]) : 3.0) * 1000.0, stepM = argc > 10 ? atof(argv[10]) : 50.0;
        Agg a; measure(sys, bi, atof(argv[7]) * DEG, atof(argv[8]) * DEG, halfM, stepM, a);
        report(sys.bodies[bi].name + " " + KINDS[sys.bodies[bi].type].name, a);
        return 0;
    }
    int only = -1;
    if (argc > 2 && std::string(argv[2]) != "all") for (int k = 0; k < PT_COUNT; k++) { std::string nm = KINDS[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) only = k; }
    int bodies = argc > 3 ? atoi(argv[3]) : 3, perBody = argc > 4 ? atoi(argv[4]) : 4;
    double halfM = (argc > 5 ? atof(argv[5]) : 3.0) * 1000.0, stepM = argc > 6 ? atof(argv[6]) : 75.0;
    Agg all;
    for (int type = 0; type < PT_COUNT; type++) {
        if (!KINDS[type].landable || (only >= 0 && type != only)) continue;
        Agg a;
        forTypeBodies(type, bodies, false, [&](const StarSystem& sys, int bi) {
            std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x4A, perBody, sites);
            Agg b;
            for (auto& st : sites) measure(sys, bi, st.first, st.second, halfM, stepM, b);
            std::string tr = traitList(BodyGen::make(sys.bodies[bi]));
            printf("  %-26s %s: 16/64 %.2f%%  64/512 %.2f%%  512/2k %.2f%%\n", sys.bodies[bi].name.c_str(), tr.empty() ? "-" : tr.c_str(), 100.0 * b.diff[1] / std::max(1L, b.n), 100.0 * b.diff[2] / std::max(1L, b.n), 100.0 * b.diff[3] / std::max(1L, b.n));
            a.n += b.n; for (int k = 0; k < 4; k++) { a.diff[k] += b.diff[k]; a.h2[k] += b.h2[k]; for (auto& kv : b.pairs[k]) a.pairs[k][kv.first] += kv.second; }
        });
        report(KINDS[type].name, a);
        all.n += a.n; for (int k = 0; k < 4; k++) { all.diff[k] += a.diff[k]; all.h2[k] += a.h2[k]; for (auto& kv : a.pairs[k]) all.pairs[k][kv.first] += kv.second; }
    }
    if (only < 0) report("ALL", all);
    return 0;
}

// O6-06, R-408: the sights round random land sites of a type (default felisian): each kind's share of sites with one within 10 km (the
int runLandmarks(int argc, char** argv) {
    // cells' sights by `sightsNear`, every ruin by `ruinsNear`), the cost of a cell and of the scanner's whole scan of 40 km
    int type = PT_FELISIAN;
    if (argc > 2) for (int k = 0; k < PT_COUNT; k++) { std::string nm = KINDS[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) type = k; }
    int sites = 0, within10[LM_COUNT] = {0}, kinds[LM_COUNT] = {0}, anyNear = 0; double cellMs = 0, scanMs = 0, scanMax = 0; int cells = 0, scans = 0;
    forTypeBodies(type, 3, false, [&](const StarSystem& sys, int bi) {
        const Body& b = sys.bodies[bi];
        BodyGen g = BodyGen::make(b);
        std::vector<std::pair<double, double>> ls; landSitesOf(sys, bi, 0x1A, 8, ls);
        for (auto& st : ls) {
            Vec3 u = StarSystem::bodyFromLatLon(st.first, st.second);
            std::vector<Landmark> found, ruins;
            double t0 = nowSec();
            sightsNear(g, u, 10000.0, sightKindsOf(g) & ~(1u << LM_RUIN), found);
            double dt = (nowSec() - t0) * 1e3;
            ruinsNear(g, u, 10000.0, ruins); found.insert(found.end(), ruins.begin(), ruins.end());
            double cellDeg = landmarkCellDeg(g);
            int nCells = (int)std::ceil(20000.0 / (cellDeg * DEG * g.R * 1000.0) + 1); nCells *= nCells;
            cellMs += dt; cells += nCells;
            sites++;
            bool near[LM_COUNT] = {false};
            for (const Landmark& L : found) {
                kinds[L.kind]++;
                double dlat, dlon; StarSystem::latLonFromBody(L.unit, dlat, dlon);
                double d = std::acos(clampd(dot(L.unit, u), -1, 1)) * g.R * 1000.0;
                if (d < 10000.0) near[L.kind] = true;
                if (sites <= 2) printf("  %-16s %s %-12s %+.3f %+.3f  h %.0f m, prominence %.0f m, radius %.0f m, %.1f km off\n", b.name.c_str(), LANDMARK_KIND_SYMBOLS[L.kind], LANDMARK_KIND_NAMES[L.kind], dlat / DEG, dlon / DEG, L.heightM, L.prominenceM, L.radiusM, d / 1000.0);
            }
            bool any = false; for (int k = 0; k < LM_COUNT; k++) { if (near[k]) { within10[k]++; any = true; } }
            if (any) anyNear++;
            if (scans < 4) {   // the scanner's whole search (40 km, every kind), as its thread runs it
                double s0 = nowSec(); std::vector<Landmark> a, r2;
                sightsNear(g, u, 40000.0, sightKindsOf(g) & ~(1u << LM_RUIN), a); ruinsNear(g, u, 40000.0, r2);
                double ms = (nowSec() - s0) * 1e3; scanMs += ms; scanMax = std::max(scanMax, ms); scans++;
            }
        }
    });
    printf("sights (%s): %d sites, %d (%.0f%%) with one within 10 km; sites with each kind within 10 km:", KINDS[type].name, sites, anyNear, sites ? 100.0 * anyNear / sites : 0.0);
    for (int k = 0; k < LM_COUNT; k++) printf(" %s %d", LANDMARK_KIND_NAMES[k], within10[k]);
    printf("; found:"); for (int k = 0; k < LM_COUNT; k++) printf(" %d", kinds[k]);
    printf("\n  %.2f ms per grid cell (%d searched: scan %.1f ms, lakes %.1f ms each); the scanner's 40 km: %.0f ms average, %.0f ms at most (%d scans)\n", cells ? cellMs / cells : 0.0, landmarkCellsSearched(), landmarkCellsSearched() ? landmarkScanMs(0) / landmarkCellsSearched() : 0.0, landmarkCellsSearched() ? landmarkScanMs(1) / landmarkCellsSearched() : 0.0, scans ? scanMs / scans : 0.0, scanMax, scans);
    return 0;
}

// O6-03: the flood round the first land sites of a type (default felisian): the tile's cost, its channels, lakes and monotony, and the sample cost with and without it
int runDrain(int argc, char** argv) {
    int type = PT_FELISIAN;
    if (argc > 2) for (int k = 0; k < PT_COUNT; k++) { std::string nm = KINDS[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) type = k; }
    int shown = 0;
    forTypeBodies(type, 3, false, [&](const StarSystem& sys, int bi) {
        const Body& b = sys.bodies[bi];
        BodyGen g = BodyGen::make(b);
        std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x3A, 2, sites);
        for (auto& st : sites) {
            Vec3 u = StarSystem::bodyFromLatLon(st.first, st.second);
            double t0 = nowSec();
            DrainStats ds = drainageStats(g, u, 60);
            double tBuild = (nowSec() - t0) * 1e3;
            SurfaceSite site; site.init(&sys, bi, st.first, st.second, 1000.0);
            std::vector<double> h; double c1 = 1e9, c0 = 1e9;
            for (int pass = 0; pass < 2; pass++) { double ta = nowSec(); gridHeights(site, 48, 16, h); c1 = std::min(c1, (nowSec() - ta) * 1e6 / (49.0 * 49.0)); }
            setDrainageEnabled(false);
            for (int pass = 0; pass < 2; pass++) { double ta = nowSec(); gridHeights(site, 48, 16, h); c0 = std::min(c0, (nowSec() - ta) * 1e6 / (49.0 * 49.0)); }
            setDrainageEnabled(true);
            int rivers = 0, lakes = 0, wetCells = 0, salt = 0;
            for (int j = -60; j <= 60; j += 3) for (int i = -60; i <= 60; i += 3) { TerrainVertex tv = site.sampleAt(i * 64, j * 64, 64); if (tv.water > -1e8f && tv.h < tv.water) { wetCells++; if (tv.shore < 1e8f) { SurfaceSample sx = sampleSurface(g, site.unitAt(i * 64, j * 64), 64); if (sx.waterKind == 1) rivers++; else if (sx.waterKind == 2) lakes++; } } if (tv.material == MAT_SALT) salt++; }
            SurfaceSample s0 = sampleSurface(g, u, 16);
            printf("%-16s %6.2f %7.2f [%s] N0 %d: window of %d cells: %d channels (%.1f%%), %d lake cells (tile: %d lakes kept, %d breached), %d sea, largest %.0f km2, downstream lower or level %d/%d; tile %.0f ms (first touch %.0f ms); 16 m sample %.2f us (%.2f without); of 1681 cells at 64 m: %d wet (%d river, %d lake), %d salt (%s here)\n",
                   b.name.c_str(), st.first / DEG, st.second / DEG, traitList(g).c_str(), drainageLatticeN0(b.radiusKm), ds.cells, ds.channels, ds.cells ? 100.0 * ds.channels / ds.cells : 0.0, ds.lakes, ds.lakesKept, ds.lakesBreached, ds.sea, ds.maxAcc, ds.monotone, ds.tested, ds.msBuild, tBuild, c1, c0, wetCells, rivers, lakes, salt,
                   s0.waterKind == 1 ? "river" : (s0.waterKind == 2 ? "lake" : MATERIAL_NAMES[s0.material]));
            shown++;
        }
    });
    printf("drain: %d sites, %d tiles held\n", shown, drainageTilesHeld());
    return 0;
}

// B-320: the inland water of the first felisian worlds at 16 m detail: rivers, lakes, pools per thousand samples, the first of each
int runWater(int argc, char** argv) {
    int wanted = argc > 2 ? atoi(argv[2]) : 3, shown = 0;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return shown < wanted; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && shown < wanted; bi++) {
            const Body& b = sys.bodies[bi];
            if (b.type != PT_FELISIAN) continue;
            BodyGen g = BodyGen::make(b);
            long n = 0, land = 0, kinds[5] = {0, 0, 0, 0, 0}, wet[5] = {0, 0, 0, 0, 0};
            double firstLat[5] = {0, 0, 0, 0, 0}, firstLon[5] = {0, 0, 0, 0, 0}; bool have[5] = {false, false, false, false, false};
            // a window of 1.2 x 1.2 degrees round the first land site of the body, at 16 m (the planet's drainage would
            // take a tile per sample over the whole globe), 400 x 400 samples
            std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x3A, 1, sites);
            double cla = sites[0].first / DEG, cln = sites[0].second;
            DrainStats ds = drainageStats(g, StarSystem::bodyFromLatLon(cla * DEG, cln), 40);
            printf("%-16s flood round %.1f %.1f: %d cells, %d channels, %d lake cells, %d sea; largest %.0f km2; downstream lower or level %d of %d; tile %.0f ms\n", b.name.c_str(), cla, cln / DEG,
                   ds.cells, ds.channels, ds.lakes, ds.sea, ds.maxAcc, ds.monotone, ds.tested, ds.msBuild);
            for (double la = cla - 0.6; la <= cla + 0.6; la += 0.003)
                for (int k = 0; k < 400; k++) {
                    double ln = cln + (k - 200) * 0.003 * DEG / std::max(0.2, std::cos(cla * DEG));
                    SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                    n++;
                    if (ss.height > 0) land++;
                    if (ss.waterKind) { kinds[ss.waterKind]++; if (ss.material == MAT_WATER) { wet[ss.waterKind]++; if (!have[ss.waterKind]) { have[ss.waterKind] = true; firstLat[ss.waterKind] = la; firstLon[ss.waterKind] = ln / DEG; } } }
                }
            printf("%-16s [%s] land %.0f%%: per 1000 samples rivers %.1f (wet %.1f), lakes %.1f (wet %.1f), pools %.1f (wet %.1f), rift lakes %.1f\n", b.name.c_str(), traitList(g).c_str(), 100.0 * land / n,
                   1000.0 * kinds[1] / n, 1000.0 * wet[1] / n, 1000.0 * kinds[2] / n, 1000.0 * wet[2] / n, 1000.0 * kinds[3] / n, 1000.0 * wet[3] / n, 1000.0 * kinds[4] / n);
            for (int kd = 1; kd < 5; kd++) if (have[kd]) printf("    first %s at %.2f %.2f\n", kd == 1 ? "river" : (kd == 2 ? "lake" : (kd == 3 ? "pool" : "rift lake")), firstLat[kd], firstLon[kd]);
            shown++;
        }
    }
    return 0;
}

// B-321: the biome shares of the first felisian worlds, area-weighted, and why the ice is ice
int runClimate(int argc, char** argv) {
    int wanted = argc > 2 ? atoi(argv[2]) : 24, shown = 0;
    int totBio[BIO_COUNT] = {0}; long totN = 0; int iceT = 0, iceCap = 0, iceSnow = 0;
    Rng r(0xC11A);
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return shown < wanted; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && shown < wanted; bi++) {
            const Body& b = sys.bodies[bi];
            if (b.type != PT_FELISIAN) continue;
            BodyGen g = BodyGen::make(b);
            setDrainageEnabled(false);   // 1500 random points over the globe would build a tile each; the biome shares do not need the rivers
            int bio[BIO_COUNT] = {0}; int n = 0, land = 0, inland = 0;
            for (int k = 0; k < 1500; k++) {
                double lat = std::asin(r.range(-1, 1)), lon = r.range(-PI, PI);
                SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), 400);
                n++; bio[ss.biome]++; if (ss.height > 0) land++;
                if (ss.material == MAT_WATER && ss.water > 0.5) inland++;   // a count of the wetlands' lakes was never printed
                if (ss.biome == BIO_ICE && ss.height > 0) {
                    double T = climateTempC(g, std::fabs(lat) / DEG, ss.height, 0, 0);
                    double lf = std::fabs(lat) / DEG / 85.0, snowLine = g.snowLine * std::max(0.0, 1.0 - lf * lf);
                    if (T < -8) iceT++; else if (ss.height > snowLine) iceSnow++; else iceCap++;
                }
            }
            printf("%-14s tempK %3.0f bias %+5.1f sea %+.2f cap %2.0f snow %4.0f  land %3.0f%% inland water %.1f%% [%s]:", b.name.c_str(), b.tempK, g.tempBias, g.seaLevel, g.iceCapLat, g.snowLine, 100.0 * land / n, 100.0 * inland / n, traitList(g).c_str());
            for (int k = 1; k < BIO_COUNT; k++) if (bio[k] * 100 / n >= 1) printf(" %s %d%%", BIOME_NAMES[k], bio[k] * 100 / n);
            printf("\n");
            for (int k = 0; k < BIO_COUNT; k++) totBio[k] += bio[k];
            totN += n; shown++;
        }
    }
    printf("all %d worlds:", shown);
    for (int k = 1; k < BIO_COUNT; k++) printf(" %s %.0f%%", BIOME_NAMES[k], 100.0 * totBio[k] / totN);
    printf("\n  land ice by cause: cold %d, over the snow line %d, polar cap %d\n", iceT, iceSnow, iceCap);
    return 0;
}

// O6-01: `landforms sites` (the fixed review sites) or `landforms <type> [flat|hills|mountains]`
int runLandforms(int argc, char** argv) {
    std::string a = argc > 2 ? argv[2] : "3";
    if (a == "sites") { renderLandformSites(argc > 3 && std::string(argv[3]) == "search"); return 0; }
    int cls = -1;
    if (argc > 3) { std::string c = argv[3]; cls = c == "flat" ? 0 : (c == "hills" ? 1 : (c == "mountains" ? 2 : -1)); }
    renderLandforms(atoi(a.c_str()), cls);
    return 0;
}
