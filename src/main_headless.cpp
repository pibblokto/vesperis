// Headless harness: dumps generation statistics and renders test frames to PNG
// so the look can be checked without opening a window.
#include "core/framebuffer.h"
#include "core/raster.h"
#include "core/font.h"
#include "core/png.h"
#include "core/parallel.h"
#include "core/fs.h"
#include "galaxy/starfield.h"
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include "galaxy/drainage.h"
#include "galaxy/landmarks.h"
#include "galaxy/ruins.h"
#include "galaxy/shards.h"
#include "galaxy/music.h"
#include "galaxy/voice.h"
#include "galaxy/signals.h"
#include <set>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <cmath>
#include <map>
#include <functional>

static int g_testScale = 1;
// the pinned felisian mountain site (the O6 review site, the `felisian_mountains` scene, `descent`); O6-03: re-pinned on the GEN 10 bodies
static constexpr double PIN_MOUNTAIN_LAT = -48.030, PIN_MOUNTAIN_LON = 152.547;   // S-01: re-pinned on Wyariothmai I, the day side of a world locked to its red dwarf (the G-04 pin's world, Wyariothmai III, is an ocean world since the star was claimed; of the five lit mountain candidates this one has a steady slope for `descent`)   // `scale=N` on the command line renders every Game-based mode at that scale

static bool galaxySkyFrames(double coreLon);   // G-03, defined with the space renderer below
static double nowSec() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

static void testStars() {
    int counts[STAR_CLASS_COUNT] = {0};
    int total = 0, sectors = 0;
    for (int64_t x = 150; x < 200; x++)
        for (int64_t y = -6; y < 6; y++)
            for (int64_t z = 20; z < 70; z++) {
                Star s;
                sectors++;
                if (starInSector(x, y, z, s)) { total++; counts[s.cls]++; }
            }
    printf("Stars near start: %d in %d sectors (%.1f%%)\n", total, sectors, 100.0 * total / sectors);
    {
        int reg[REGION_COUNT] = {0};
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) reg[galaxyRegion(x, y, z)]++;
        printf("regions of those sectors:"); for (int i = 0; i < REGION_COUNT; i++) if (reg[i]) printf("  %s %d", REGION_NAMES[i], reg[i]); printf("\n");
        int belts = 0, captured = 0, sysN = 0;
        for (int64_t x = 150; x < 190; x++) for (int64_t z = 20; z < 60; z++) { Star s; if (!starInSector(x, 0, z, s)) continue; StarSystem sys; sys.generate(s); sysN++; belts += (int)sys.belts.size(); for (auto& b : sys.bodies) if (b.orbitPeriod < 0) captured++; }
        printf("belts: %d in %d systems, captured retrograde moons: %d\n", belts, sysN, captured);
    }
    for (int i = 0; i < STAR_CLASS_COUNT; i++) printf("  %s %-14s %d\n", STAR_CLASSES[i].code, STAR_CLASSES[i].name, counts[i]);
    int typeCounts[PT_COUNT] = {0};
    int moons = 0, planets = 0, systems = 0;
    for (int64_t x = 150; x < 190; x++)
        for (int64_t z = 20; z < 60; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys;
            sys.generate(s);
            systems++;
            for (auto& b : sys.bodies) { typeCounts[b.type]++; if (b.parent < 0) planets++; else moons++; }
        }
    printf("Systems: %d planets: %d moons: %d\n", systems, planets, moons);
    for (int i = 0; i < PT_COUNT; i++) printf("  %-16s %d\n", PLANET_TYPES[i].name, typeCounts[i]);
    // print a few systems in detail
    int shown = 0;
    for (int64_t x = 180; x < 200 && shown < 4; x++) {
        Star s;
        if (!starInSector(x, 0, 40, s)) continue;
        StarSystem sys;
        sys.generate(s);
        printf("\n%s  class %s (%s)  R=%.0f km  L=%.2f  bodies=%zu\n", s.name.c_str(), STAR_CLASSES[s.cls].code,
               STAR_CLASSES[s.cls].name, s.radiusKm, s.luminosity, sys.bodies.size());
        for (auto& b : sys.bodies)
            printf("   %-28s %-16s R=%6.0f km  orbit=%.3g km  T=%.1f h  rot=%.1f h  tilt=%.0f  temp=%.0fK moons=%d %s\n",
                   b.name.c_str(), PLANET_TYPES[b.type].name, b.radiusKm, b.orbitRadiusKm, b.orbitPeriod / 3600,
                   std::fabs(b.rotPeriod) / 3600, b.axialTilt / DEG, b.tempK, b.moonCount, b.rings ? "rings" : "");
        shown++;
    }
}

// ---- the galaxy (G-01) ------------------------------------------------------------------------
// `galaxy`: the density function of PLAN-galaxy-scale.md in numbers. The midplane profile along the radius (the arm
// crest and the gap at each radius, the scale height, the region at the crest), the star count by a numerical
// integration over the disc with the regions' shares, the home sectors (density, region, the stars of the new-game
// search and the star the game picks, the neighbourhood within ten light years) and two column-density maps:
// `shots/tests/galaxy_faceon.png` (110,000 ly across, home ringed in green) and `galaxy_edgeon.png` (the same width,
// 5,500 ly tall: the vertical axis stretched ten times).
static void runGalaxy() {
    printf("galaxy: centre at sector %+.0f 0 %+.0f; disc scale %.0f ly, peak %.2f; thickness %.0f + %.0f e^(-r/%.0f); bulge %.0f ly, peak %.2f; %d arms; cap %.2f\n",
           GALAXY_CENTRE_SX, GALAXY_CENTRE_SZ, GALAXY_DISC_SCALE, GALAXY_DISC_PEAK, GALAXY_THICK_FAR, GALAXY_THICK_NEAR, GALAXY_THICK_SCALE,
           GALAXY_BULGE_R, GALAXY_BULGE_PEAK, GALAXY_ARMS, GALAXY_CAP);
    const double hx = 185, hz = 45;   // the centre of the new-game search (Game::newGame: sectors 176..196 x 36..56)
    GalaxyTerms gh = galaxyTerms(hx, 0, hz);
    double bearing = std::atan2(GALAXY_CENTRE_SX - hx, GALAXY_CENTRE_SZ - hz) / DEG;
    if (bearing < 0) bearing += 360;
    printf("home (sector %.0f 0 %.0f): %.0f ly from the centre, the core at bearing %.0f (yaw, 0 = +z); density %.3f = disc %.3f x (0.55 + 0.6 x arm %.2f) + bulge %.5f; scale height %.0f ly; %s\n",
           hx, hz, gh.r, bearing, galaxyDensity(hx, 0, hz), gh.disk, gh.arm, gh.bulge, gh.h, REGION_NAMES[galaxyRegion((int64_t)hx, 0, (int64_t)hz)]);
    // the profile along the radius: the densest and the thinnest angle at each radius, without the cluster knots
    auto smooth = [](double x, double y, double z) { GalaxyTerms g = galaxyTerms(x, y, z); return std::min(GALAXY_CAP, g.disk * (0.55 + 0.6 * g.arm) + g.bulge); };
    printf("  radius ly  height ly    crest      gap    bulge  region at the crest\n");
    const double radii[] = {0, 1000, 2500, 4000, 5000, 7500, 10000, 12500, 13050, 15000, 20000, 25000, 30000, 40000, 50000};
    for (double r : radii) {
        double mx = 0, mn = 9, mxAng = 0;
        for (int ia = 0; ia < 720; ia++) {
            double ang = ia / 720.0 * TAU;
            double d = smooth(GALAXY_CENTRE_SX + r * std::cos(ang), 0, GALAXY_CENTRE_SZ + r * std::sin(ang));
            if (d > mx) { mx = d; mxAng = ang; }
            if (d < mn) mn = d;
        }
        GalaxyTerms g = galaxyTerms(GALAXY_CENTRE_SX + r, 0, GALAXY_CENTRE_SZ);
        int64_t cx = (int64_t)std::floor(GALAXY_CENTRE_SX + r * std::cos(mxAng)), cz = (int64_t)std::floor(GALAXY_CENTRE_SZ + r * std::sin(mxAng));
        printf("  %9.0f  %9.0f  %7.4f  %7.4f  %7.4f  %s\n", r, g.h, mx, mn, g.bulge, REGION_NAMES[galaxyRegion(cx, 0, cz)]);
    }
    // the count: cylindrical shells of 100 ly, 64 angles, 25 ly steps in height out to six scale heights (three bulge radii at least)
    const int NR = 600, NA = 64; const double dr = 100, dy = 25;
    std::vector<double> shell((size_t)NR * (REGION_COUNT + 1), 0.0);
    parallelFor(NR, 4, [&](int b, int e) {
        for (int ir = b; ir < e; ir++) {
            double r = (ir + 0.5) * dr;
            double ymax = std::max(6.0 * galaxyTerms(GALAXY_CENTRE_SX + r, 0, GALAXY_CENTRE_SZ).h, 3.0 * GALAXY_BULGE_R);
            double* acc = &shell[(size_t)ir * (REGION_COUNT + 1)];
            for (double y = dy / 2; y < ymax; y += dy)
                for (int ia = 0; ia < NA; ia++) {
                    double ang = (ia + 0.5) / NA * TAU;
                    double x = GALAXY_CENTRE_SX + r * std::cos(ang), z = GALAXY_CENTRE_SZ + r * std::sin(ang);
                    double w = galaxyDensity(x, y, z) * dy * 2 * (TAU * r * dr / NA);   // both sides of the plane
                    acc[0] += w;
                    acc[1 + galaxyRegion((int64_t)std::floor(x), (int64_t)std::floor(y), (int64_t)std::floor(z))] += w;
                }
        }
    });
    double total = 0, byRegion[REGION_COUNT] = {0}, within[4] = {0};   // within 5,000 / 13,050 / 25,000 / 50,000 ly
    for (int ir = 0; ir < NR; ir++) {
        const double* acc = &shell[(size_t)ir * (REGION_COUNT + 1)];
        total += acc[0];
        for (int k = 0; k < REGION_COUNT; k++) byRegion[k] += acc[1 + k];
        double r = (ir + 0.5) * dr;
        if (r < 5000) within[0] += acc[0];
        if (r < gh.r) within[1] += acc[0];
        if (r < 25000) within[2] += acc[0];
        if (r < 50000) within[3] += acc[0];
    }
    printf("stars: %.1f billion (%.0f%% within 5,000 ly, %.0f%% inside home's radius, %.0f%% within 25,000, %.1f%% beyond 50,000)\n", total / 1e9,
           100 * within[0] / total, 100 * within[1] / total, 100 * within[2] / total, 100 * (1 - within[3] / total));
    printf("  by region:"); for (int k = 0; k < REGION_COUNT; k++) printf("  %s %.1f%%", REGION_NAMES[k], 100 * byRegion[k] / total); printf("\n");
    // G-02: the clusters: every globular of the galaxy (the cells within 60,000 ly of the centre), the open clusters within 1,500 ly of home
    Vec3 homeS(hx + 0.5, 0.5, hz + 0.5);
    struct Knot { double dist; Vec3 c; double R; };
    std::vector<Knot> globs, opens;
    int globBins[4] = {0};
    for (int64_t cx = -30; cx < 30; cx++) for (int64_t cy = -30; cy < 30; cy++) for (int64_t cz = -30; cz < 30; cz++) {
        Vec3 c; double R;
        if (!globularInCell(cx, cy, cz, c, R)) continue;
        double rc = length(c - Vec3(GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ));
        globBins[rc < 5000 ? 0 : (rc < 10000 ? 1 : (rc < 20000 ? 2 : 3))]++;
        globs.push_back({length(c - homeS), c, R});
    }
    std::sort(globs.begin(), globs.end(), [](const Knot& a, const Knot& b) { return a.dist < b.dist; });
    printf("globular clusters: %zu (%d within 5,000 ly of the centre, %d to 10,000, %d to 20,000, %d beyond)", globs.size(), globBins[0], globBins[1], globBins[2], globBins[3]);
    for (size_t k = 0; k < globs.size() && k < 3; k++) printf("%s %.0f ly from home at sector %.0f %.0f %.0f, %.0f ly across, ~%.0f stars", k ? ";" : "; the nearest:", globs[k].dist, globs[k].c.x, globs[k].c.y, globs[k].c.z, 2 * globs[k].R, 9.57 * std::pow(globs[k].R / 2, 3));
    printf("\n");
    {
        int64_t ox = (int64_t)std::floor((hx - GALAXY_CENTRE_SX) / OPEN_CLUSTER_CELL), oy = 0, oz = (int64_t)std::floor((hz - GALAXY_CENTRE_SZ) / OPEN_CLUSTER_CELL);
        for (int64_t cx = ox - 15; cx <= ox + 15; cx++) for (int64_t cy = oy - 8; cy <= oy + 8; cy++) for (int64_t cz = oz - 15; cz <= oz + 15; cz++) {
            Vec3 c; double R;
            if (!openClusterInCell(cx, cy, cz, c, R)) continue;
            double d = length(c - homeS);
            if (d < 1500) opens.push_back({d, c, R});
        }
        std::sort(opens.begin(), opens.end(), [](const Knot& a, const Knot& b) { return a.dist < b.dist; });
        printf("open clusters within 1,500 ly of home: %zu", opens.size());
        for (size_t k = 0; k < opens.size() && k < 3; k++) printf("%s %.0f ly away at sector %.0f %.0f %.0f, %.0f ly across, ~%.0f stars", k ? ";" : "; the nearest:", opens[k].dist, opens[k].c.x, opens[k].c.y, opens[k].c.z, 2 * opens[k].R, 9.57 * std::pow(opens[k].R / 2, 3));
        printf("\n");
    }
    // the class mix per region: a cube of 15 sectors round a point of each region, the stars of that region's sectors only
    auto mix = [&](const char* label, int want, double px, double py, double pz) {
        int sectors = 0, stars = 0, cls[STAR_CLASS_COUNT] = {0};
        int64_t x0 = (int64_t)std::floor(px), y0 = (int64_t)std::floor(py), z0 = (int64_t)std::floor(pz);
        for (int64_t x = x0 - 7; x <= x0 + 7; x++) for (int64_t y = y0 - 7; y <= y0 + 7; y++) for (int64_t z = z0 - 7; z <= z0 + 7; z++) {
            if (galaxyRegion(x, y, z) != want) continue;
            sectors++;
            Star st; if (!starInSector(x, y, z, st, false)) continue;
            stars++; cls[st.cls]++;
        }
        printf("  %-19s at %6.0f %5.0f %6.0f: %4d of %4d sectors hold a star (%4.1f%%)", label, px, py, pz, stars, sectors, sectors ? 100.0 * stars / sectors : 0.0);
        for (int i = 0; i < STAR_CLASS_COUNT; i++) printf("  %s %2.0f%%", STAR_CLASSES[i].code, stars ? 100.0 * cls[i] / stars : 0.0);
        printf("\n");
    };
    printf("the class mix by region (a cube of 15 sectors round a point of each):\n");
    mix("GALACTIC CORE", REGION_CORE, GALAXY_CENTRE_SX + 300, 0, GALAXY_CENTRE_SZ);
    for (int ia = 0; ia < 72; ia++) {   // the bulge where its term beats the disc's: an angle between the arms at 2,000 ly
        double ang = ia / 72.0 * TAU, bx = GALAXY_CENTRE_SX + 2000 * std::cos(ang), bz = GALAXY_CENTRE_SZ + 2000 * std::sin(ang);
        if (galaxyRegion((int64_t)std::floor(bx), 0, (int64_t)std::floor(bz)) == REGION_BULGE) { mix("THE BULGE", REGION_BULGE, bx, 0, bz); break; }
    }
    mix("SPIRAL ARM", REGION_ARM, hx, 0, hz);
    {   // the gap beside home's arm: the thinnest angle at home's radius
        double best = 9, bAng = 0;
        for (int ia = 0; ia < 720; ia++) { double ang = ia / 720.0 * TAU; double a = galaxyTerms(GALAXY_CENTRE_SX + gh.r * std::cos(ang), 0, GALAXY_CENTRE_SZ + gh.r * std::sin(ang)).arm; if (a < best) { best = a; bAng = ang; } }
        mix("THE DISK", REGION_DISK, GALAXY_CENTRE_SX + gh.r * std::cos(bAng), 0, GALAXY_CENTRE_SZ + gh.r * std::sin(bAng));
    }
    mix("THE HALO", REGION_HALO, hx, 1500, hz);
    if (!globs.empty()) mix("GLOBULAR CLUSTER", REGION_CLUSTER, globs[0].c.x, globs[0].c.y, globs[0].c.z);
    if (!opens.empty()) mix("OPEN CLUSTER", REGION_OPEN, opens[0].c.x, opens[0].c.y, opens[0].c.z);
    {   // a star-forming cell near home
        bool found = false;
        for (int64_t x = (int64_t)hx - 200; x <= (int64_t)hx + 200 && !found; x += 7) for (int64_t z = (int64_t)hz - 200; z <= (int64_t)hz + 200 && !found; z += 7)
            if (galaxyRegion(x, 0, z) == REGION_NEBULA) { mix("STAR-FORMING REGION", REGION_NEBULA, (double)x, 0, (double)z); found = true; }
        if (!found) printf("  no star-forming cell within 200 ly of home\n");
    }
    // the home sectors: the new-game search and the star the game picks (Game::newGame's score)
    int stars = 0, cls[STAR_CLASS_COUNT] = {0}, reg[REGION_COUNT] = {0};
    Star best; int bestScore = -1; bool found = false;
    for (int64_t x = 176; x < 196; x++)
        for (int64_t z = 36; z < 56; z++) {
            reg[galaxyRegion(x, 0, z)]++;
            Star s; if (!starInSector(x, 0, z, s)) continue;
            stars++; cls[s.cls]++;
            if (bestScore >= 100 || (s.cls != STAR_YELLOW && s.cls != STAR_ORANGE)) continue;   // the game stops at the first column with a living world (score 100)
            StarSystem ss; ss.generate(s);
            int score = 0;
            for (auto& b : ss.bodies) { if (b.type == PT_FELISIAN && b.parent < 0) score += 50; if (b.parent < 0) score += 3; if (b.rings) score += 5; }
            if (score > bestScore) { bestScore = score; best = s; found = true; }
        }
    printf("the new-game search (400 sectors): %d stars;", stars);
    for (int i = 0; i < STAR_CLASS_COUNT; i++) printf(" %s %d", STAR_CLASSES[i].code, cls[i]);
    printf("; regions:"); for (int k = 0; k < REGION_COUNT; k++) if (reg[k]) printf(" %s %d", REGION_NAMES[k], reg[k]); printf("\n");
    {   // G-02: the pinned home star, and the search's own pick (the fallback)
        Star hs; bool ok = starInSector(HOME_SX, HOME_SY, HOME_SZ, hs);
        if (ok) printf("  the home star is pinned at sector %lld %lld %lld: %s (%s, exists while the density stays above %.3f)\n", (long long)HOME_SX, (long long)HOME_SY, (long long)HOME_SZ, hs.name.c_str(), STAR_CLASSES[hs.cls].name, unitFromHash(sectorSeed(HOME_SX, HOME_SY, HOME_SZ)));
        else printf("  the pinned home sector %lld %lld %lld holds NO star: the game falls back to the search\n", (long long)HOME_SX, (long long)HOME_SY, (long long)HOME_SZ);
    }
    if (found) printf("  the search's own pick would be %s at sector %lld 0 %lld (%s, score %d, exists while the density stays above %.3f)\n", best.name.c_str(), (long long)best.sx, (long long)best.sz, STAR_CLASSES[best.cls].name, bestScore, unitFromHash(sectorSeed(best.sx, best.sy, best.sz)));
    else printf("  no yellow or orange star with a living world there: the search falls back to sector 180 0 40\n");
    // the neighbourhood: the cube StarNeighborhood scans, the nearest-neighbour distances, the stars within ten light years
    StarNeighborhood nb; Vec3 homeKm((hx + 0.5) * SECTOR_KM, 0.5 * SECTOR_KM, (hz + 0.5) * SECTOR_KM); nb.update(homeKm);
    int near10 = 0; double nnSum = 0; int nnN = 0;
    for (const Star& a : nb.stars) {
        if (length(a.pos - homeKm) < 10 * SECTOR_KM) near10++;
        if (std::llabs(a.sx - (int64_t)hx) > 5 || std::llabs(a.sy) > 5 || std::llabs(a.sz - (int64_t)hz) > 5) continue;   // the inner cube: every neighbour is in the scan
        double bd = 1e300;
        for (const Star& b : nb.stars) { if (&a == &b) continue; bd = std::min(bd, length2(a.pos - b.pos)); }
        nnSum += std::sqrt(bd) / SECTOR_KM; nnN++;
    }
    NebulaPatch np[16]; int nn = nebulaPatches(Vec3(hx + 0.5, 0.5, hz + 0.5), np, 16);
    printf("the neighbourhood: %zu stars in the 21-sector cube (%.1f%% of its sectors), %d within 10 ly of home, the nearest star %.2f ly away on average, %d nebula patches in home's sky\n",
           nb.stars.size(), 100.0 * nb.stars.size() / (21.0 * 21 * 21), near10, nnN ? nnSum / nnN : 0.0, nn);
    {   // G-03: the star-forming complexes within 300 ly of home, and the nearest
        int nComplex = 0; double nearest = 1e9; Vec3 nearC;
        for (int64_t cz = (int64_t)std::floor((hz - 300) / NEBULA_CELL); cz <= (int64_t)std::floor((hz + 300) / NEBULA_CELL); cz++)
            for (int64_t cx = (int64_t)std::floor((hx - 300) / NEBULA_CELL); cx <= (int64_t)std::floor((hx + 300) / NEBULA_CELL); cx++) {
                Vec3 c; if (!nebulaInCell(cx, cz, c)) continue;
                double dd = length(c - Vec3(hx + 0.5, 0.5, hz + 0.5)); if (dd > 300) continue;
                nComplex++; if (dd < nearest) { nearest = dd; nearC = c; }
            }
        printf("  star-forming complexes within 300 ly of home: %d (one per %.0f ly of the arm's plane on average); the nearest %.0f ly away at sector %lld %lld %lld, its patches in sight from %.0f ly\n",
               nComplex, nComplex ? std::sqrt(PI * 300 * 300 / nComplex) : 0.0, nearest, (long long)std::floor(nearC.x), (long long)std::floor(nearC.y), (long long)std::floor(nearC.z), NEBULA_SEEN);
    }
    {   // G-03: the band from home: the columns of starlight and the dust in the named directions, the map's cost, its picture and the sky from the ship
        Vec3 obs(hx + 0.5, 0.5, hz + 0.5);
        double coreLon = std::atan2(GALAXY_CENTRE_SZ - obs.z, GALAXY_CENTRE_SX - obs.x);
        struct D { const char* name; double lon, lat; } dirs[] = {{"the core, in the plane", coreLon, 0}, {"the core, 4 degrees above", coreLon, 4 * DEG}, {"the core, 12 above", coreLon, 12 * DEG}, {"along the arm", coreLon + PI / 2, 0}, {"the anticentre", coreLon + PI, 0}, {"the anticentre, 10 above", coreLon + PI, 10 * DEG}, {"30 degrees up", coreLon + PI, 30 * DEG}, {"the pole", 0, 0.5 * PI - 1e-6}};
        printf("the band from home (the column of starlight in stars per square light year, the dust's optical depth, the brightness 0..1):\n");
        for (const D& dd : dirs) {
            Vec3 d(std::cos(dd.lat) * std::cos(dd.lon), std::sin(dd.lat), std::cos(dd.lat) * std::sin(dd.lon));
            double tau, col = galaxyColumn(obs, d, tau);
            printf("  %-28s column %7.0f  tau %5.2f  brightness %.2f\n", dd.name, col, tau, bandToneOf(col));
        }
        std::vector<float> band; double tb0 = nowSec();
        buildGalaxyBand(obs, band, BAND_MAP_W, BAND_MAP_H);
        printf("  the %d x %d map builds in %.1f ms (%d threads)\n", BAND_MAP_W, BAND_MAP_H, (nowSec() - tb0) * 1000, parallelThreads());
        // the profile along the plane every 30 degrees from the core, and by latitude at the core and the anticentre
        printf("  along the plane from the core:"); for (int k = 0; k < 12; k++) { double lon = coreLon + k * 30 * DEG; printf(" %d:%.2f", k * 30, sampleGalaxyBand(band, BAND_MAP_W, BAND_MAP_H, Vec3(std::cos(lon), 0, std::sin(lon)))); } printf("\n");
        for (int a = 0; a < 2; a++) {
            double lon = coreLon + a * PI; printf("  by latitude at the %s:", a ? "anticentre" : "core");
            for (double lat : {0.0, 1.0, 2.0, 3.0, 5.0, 8.0, 12.0, 20.0, 30.0, 60.0, 90.0}) printf(" %.0f:%.2f", lat, sampleGalaxyBand(band, BAND_MAP_W, BAND_MAP_H, Vec3(std::cos(lat * DEG) * std::cos(lon), std::sin(lat * DEG), std::cos(lat * DEG) * std::sin(lon))));
            printf("\n");
        }
        {   // the whole sky as the game samples it, 720 x 360, the core at the centre
            const int BW = 720, BH = 360; std::vector<uint32_t> bimg((size_t)BW * BH);
            for (int j = 0; j < BH; j++)
                for (int i = 0; i < BW; i++) {
                    double lon = coreLon + ((i + 0.5) / BW - 0.5) * TAU, lat = (0.5 - (j + 0.5) / BH) * PI;
                    int g = clampi((int)(255 * sampleGalaxyBand(band, BAND_MAP_W, BAND_MAP_H, Vec3(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon)))), 0, 255);
                    bimg[(size_t)j * BW + i] = 0xFF000000u | (uint32_t)g << 16 | (uint32_t)g << 8 | (uint32_t)g;
                }
            writePNG("shots/tests/galaxy_band.png", bimg.data(), BW, BH);
        }
        if (galaxySkyFrames(coreLon)) printf("  shots/tests/galaxy_band.png (the sky as the game samples the map, the core at the centre), galaxy_sky_core/arm/anticentre.png (from above the home star)\n");
    }
    // the maps: column densities through the disc, log-toned
    const int W = 400, H = 400; const double half = 55000;
    std::vector<uint32_t> img((size_t)W * H);
    auto grey = [](double v) { int g = clampi((int)(255 * v), 0, 255); return 0xFF000000u | (uint32_t)g << 16 | (uint32_t)g << 8 | (uint32_t)g; };
    parallelFor(H, 4, [&](int b, int e) {
        for (int j = b; j < e; j++)
            for (int i = 0; i < W; i++) {
                double x = GALAXY_CENTRE_SX + ((i + 0.5) / W * 2 - 1) * half, z = GALAXY_CENTRE_SZ + ((j + 0.5) / H * 2 - 1) * half;
                double col = 0;
                for (double y = -3000; y <= 3000; y += 100) col += galaxyDensity(x, y, z) * 100;
                img[(size_t)j * W + i] = grey(std::log1p(col / 40) / std::log1p(1200 / 40.0));
            }
    });
    {
        int hi = (int)((hx - GALAXY_CENTRE_SX + half) / (2 * half) * W), hj = (int)((hz - GALAXY_CENTRE_SZ + half) / (2 * half) * H);
        for (int a = 0; a < 48; a++) { int x = hi + (int)std::lround(5 * std::cos(a / 48.0 * TAU)), y = hj + (int)std::lround(5 * std::sin(a / 48.0 * TAU)); if (x >= 0 && x < W && y >= 0 && y < H) img[(size_t)y * W + x] = 0xFF00FF00u; }
        for (const Knot& g : globs) {   // G-02: the globulars as orange dots
            int gi = (int)((g.c.x - GALAXY_CENTRE_SX + half) / (2 * half) * W), gj = (int)((g.c.z - GALAXY_CENTRE_SZ + half) / (2 * half) * H);
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) { int x = gi + dx, y = gj + dy; if (x >= 0 && x < W && y >= 0 && y < H) img[(size_t)y * W + x] = 0xFF2090FFu; }
        }
        writePNG("shots/tests/galaxy_faceon.png", img.data(), W, H);
    }
    const int EH = 100; const double ehalf = 2750;
    std::vector<uint32_t> edge((size_t)W * EH);
    parallelFor(EH, 2, [&](int b, int e) {
        for (int j = b; j < e; j++)
            for (int i = 0; i < W; i++) {
                double x = GALAXY_CENTRE_SX + ((i + 0.5) / W * 2 - 1) * half, y = ((j + 0.5) / EH * 2 - 1) * ehalf;
                double col = 0;
                for (double z = -half; z <= half; z += 250) col += galaxyDensity(x, y, GALAXY_CENTRE_SZ + z) * 250;
                edge[(size_t)j * W + i] = grey(std::log1p(col / 50) / std::log1p(10000 / 50.0));
            }
    });
    writePNG("shots/tests/galaxy_edgeon.png", edge.data(), W, EH);
    printf("maps: shots/tests/galaxy_faceon.png (110,000 ly across, home ringed), galaxy_edgeon.png (5,500 ly tall, stretched x10)\n");
}

static void renderMaps() {
    // find one body of each type and dump its map
    bool done[PT_COUNT] = {false};
    bool lockedDone = false;
    int remaining = PT_COUNT;
    std::vector<uint32_t> img(PlanetMap::W * PlanetMap::H * 2);
    for (int64_t x = 150; x < 260 && (remaining > 0 || !lockedDone); x++)
        for (int64_t z = 20; z < 120 && (remaining > 0 || !lockedDone); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys;
            sys.generate(s);
            for (auto& b : sys.bodies) {
                if (done[b.type] && !(b.type == PT_FELISIAN && b.locked && !lockedDone)) continue;
                if (b.type == PT_COMPANION) { done[b.type] = true; remaining--; continue; }   // a sun has no map
                bool lockedShot = b.type == PT_FELISIAN && b.locked && done[b.type];   // M5-08: one extra map of a locked living world
                if (lockedShot) lockedDone = true;
                else { done[b.type] = true; remaining--; }
                BodyGen g = BodyGen::make(b);
                PlanetMap m;
                double t0 = nowSec();
                m.generate(g);
                double t1 = nowSec();
                printf("map %-16s %s: %.0f ms\n", PLANET_TYPES[b.type].name, b.name.c_str(), (t1 - t0) * 1000);
                for (int y = 0; y < PlanetMap::H; y++)
                    for (int xx = 0; xx < PlanetMap::W; xx++) {
                        int i = y * PlanetMap::W + xx;
                        int a = m.albedo[i];
                        int c = m.cloud[i];
                        RGB col = b.color;
                        double v = a / 255.0;
                        RGB pixc = col * (float)v;
                        pixc = lerp(pixc, RGB(1, 1, 1) * (float)(0.9), (float)(c / 255.0) * 0.8f);
                        img[i] = rgb((int)(pixc.r * 255), (int)(pixc.g * 255), (int)(pixc.b * 255));
                        // second row: height map
                        double h = m.height[i];
                        int hv = clampi((int)(128 + h / 40.0), 0, 255);
                        uint32_t hc = rgb(hv, hv, hv);
                        static const uint32_t matCol[MAT_COUNT] = {rgb(120, 110, 100), rgb(220, 200, 140), rgb(90, 160, 60), rgb(30, 90, 30), rgb(240, 240, 250), rgb(20, 50, 140), rgb(255, 120, 20), rgb(200, 220, 255), rgb(255, 255, 255), rgb(240, 220, 235), rgb(60, 55, 55), rgb(160, 130, 110), rgb(200, 180, 140), rgb(120, 115, 112), rgb(230, 200, 90), rgb(40, 40, 42), rgb(236, 233, 224), rgb(30, 42, 36)};
                        uint32_t mc = matCol[m.material[i]];
                        img[PlanetMap::W * PlanetMap::H + i] = (xx < PlanetMap::W / 2) ? hc : mc;
                    }
                std::string fn = std::string("shots/tests/map_") + (lockedShot ? "LOCKED_" : "") + PLANET_TYPES[b.type].name + ".png";
                for (char& ch : fn) if (ch == ' ') ch = '_';
                writePNG(fn.c_str(), img.data(), PlanetMap::W, PlanetMap::H * 2);
                if (lockedShot) printf("  locked: sub-stellar point lat %.0f lon %.0f -> %s\n", std::asin(b.lockedDir.z) / DEG, std::atan2(b.lockedDir.y, b.lockedDir.x) / DEG, fn.c_str());
            }
        }
}


// ---- space scene tests -------------------------------------------------------
#include "space/space_view.h"
static void saveFB(Framebuffer& fb, const char* path, double gain = 1.0) {
    std::vector<uint32_t> rgbBuf(FBW * FBH);
    fb.toRGB(rgbBuf.data(), gain);
    writePNG(path, rgbBuf.data(), FBW, FBH);
}

// G-03: the sky from a ship above the home star, toward the core, along the arm and toward the anticentre (`galaxy`)
static bool galaxySkyFrames(double coreLon) {
    Star hs; if (!starInSector(HOME_SX, HOME_SY, HOME_SZ, hs)) return false;
    StarSystem hsys; hsys.generate(hs);
    double dd = std::max(hs.radiusKm * STAR_CLASSES[hs.cls].firstOrbitMult, STAR_CLASSES[hs.cls].minFirstOrbitKm);
    Vec3 shipPos = hs.pos + Vec3(0, dd, 0);   // above the star: no sun in the frames
    StarNeighborhood snb; snb.update(shipPos);
    SpaceRenderer ssr;
    struct V { const char* name; double lon; } views[] = {{"core", coreLon}, {"arm", coreLon + PI / 2}, {"anticentre", coreLon + PI}};
    for (const V& v : views) {
        Framebuffer fb; SpaceContext c;
        c.sys = &hsys; c.stars = &snb.stars; c.t = 1234.0; c.shipPos = shipPos;
        c.cam = cameraBasis(std::atan2(std::cos(v.lon), std::sin(v.lon)), 0);   // the heading of a longitude measured from +x toward +z
        ssr.setupPalette(fb, &hsys, -1, -1, 1.0);
        ssr.render(fb, c); fb.mush(2);
        saveFB(fb, (std::string("shots/tests/galaxy_sky_") + v.name + ".png").c_str());
    }
    return true;
}

// maps are generated on worker threads (M9-15): the tests wait for the ones they are about to draw
static void warmMaps(SpaceRenderer& sr, const StarSystem& sys, int only = -1) {
    for (const Body& b : sys.bodies) {
        if (b.type == PT_COMPANION) continue;
        if (only >= 0 && b.index != only) continue;
        sr.mapFor(b);
    }
}

static uint64_t fnv(uint64_t h, const void* p, size_t n);

static void renderSpace() {
    SpaceRenderer sr;
    StarNeighborhood nb;
    // 1) one sun per class, seen from the first orbit distance
    bool done[STAR_CLASS_COUNT] = {false};
    int remaining = STAR_CLASS_COUNT;
    for (int64_t x = 150; x < 300 && remaining; x++)
        for (int64_t z = 20; z < 120 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            if (done[s.cls]) continue;
            done[s.cls] = true; remaining--;
            StarSystem sys; sys.generate(s);
            double d = std::max(s.radiusKm * STAR_CLASSES[s.cls].firstOrbitMult, STAR_CLASSES[s.cls].minFirstOrbitKm);
            Vec3 shipPos = s.pos + Vec3(0, 0, -d);
            nb.update(shipPos);
            Framebuffer fb;
            SpaceContext c;
            c.sys = &sys; c.stars = &nb.stars; c.t = 1234.0; c.shipPos = shipPos;
            c.cam = cameraBasis(0, 0);
            sr.setupPalette(fb, &sys, -1, -1, 1.0);
            sr.render(fb, c);
            fb.mush(2);
            std::string fn = std::string("shots/tests/sun_") + STAR_CLASSES[s.cls].code + ".png";
            saveFB(fb, fn.c_str());
            printf("sun %s %s R=%.0f d=%.3g\n", STAR_CLASSES[s.cls].code, s.name.c_str(), s.radiusKm, d);
        }
    // S-06: the first black hole with a companion, from 20 degrees above the plane of its worlds at six tenths of the first orbit:
    // the shadow, the disc arching over and under it, the bent stars and the companion's stream into the disc
    for (int64_t x = 150; x < 300; x++) {
        bool got = false;
        for (int64_t z = 20; z < 120 && !got; z++) {
            Star s;
            if (!starInSector(x, 0, z, s) || s.cls != STAR_BLACK_HOLE) continue;
            StarSystem sys; sys.generate(s);
            if (sys.companion < 0) continue;
            got = true;
            double d = STAR_CLASSES[s.cls].minFirstOrbitKm * 0.6;
            Vec3 shipPos = s.pos + Vec3(0, 0.34 * d, -0.94 * d);
            nb.update(shipPos);
            Framebuffer fb;
            SpaceContext c;
            c.sys = &sys; c.stars = &nb.stars; c.t = 1234.0; c.shipPos = shipPos;
            c.cam = cameraBasis(0, -0.35);
            sr.setupPalette(fb, &sys, -1, -1, 1.0);
            sr.render(fb, c);
            fb.mush(2);
            saveFB(fb, "shots/tests/space_black_hole.png");
            const Body& k = sys.bodies[sys.companion];
            printf("black hole %s with a companion (%s, %.3g km out, %d bodies, %d belts): shots/tests/space_black_hole.png\n", s.name.c_str(), STAR_CLASSES[k.starClass].name, k.orbitRadiusKm, (int)sys.bodies.size(), (int)sys.belts.size());
        }
        if (got) break;
    }
    // 2) planet globes: one per type, parked view
    bool doneT[PT_COUNT] = {false};
    int remT = PT_COUNT;
    int gasWithRings = 0;
    for (int64_t x = 150; x < 300 && remT; x++)
        for (int64_t z = 20; z < 120 && remT; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (doneT[b.type] && !(b.type == PT_GASGIANT && b.rings && !gasWithRings)) continue;
                if (b.type == PT_GASGIANT && b.rings) gasWithRings = 1;
                if (!doneT[b.type]) { doneT[b.type] = true; remT--; }
                double t = 5000.0;
                Vec3 bp = sys.bodyPos(bi, t);
                // park: on the sunlit side, slightly off, at 3.5 radii (more for rings)
                Vec3 toStar = normalize(sys.star.pos - bp);
                Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
                double parkR = b.radiusKm * (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5);
                Vec3 shipPos = bp + normalize(toStar * 0.6 + side * 0.8 + Vec3(0, 0.25, 0)) * parkR;
                Vec3 fwd = normalize(bp - shipPos);
                double yaw = std::atan2(fwd.x, fwd.z), pitch = std::asin(fwd.y);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                c.cam = cameraBasis(yaw, pitch);
                c.bankBodyA = bi;
                // second nearest body gets bank 3
                int second = -1; double sd = 1e300;
                for (int j = 0; j < (int)sys.bodies.size(); j++) if (j != bi) { double dd = length2(sys.bodyPos(j, t) - shipPos); if (dd < sd) { sd = dd; second = j; } }
                c.bankBodyB = second;
                warmMaps(sr, sys, bi); if (second >= 0) warmMaps(sr, sys, second);
                sr.setupPalette(fb, &sys, bi, second, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                std::string fn = std::string("shots/tests/globe_") + PLANET_TYPES[b.type].name + (b.rings ? "_rings" : "") + ".png";
                for (char& ch : fn) if (ch == ' ') ch = '_';
                saveFB(fb, fn.c_str());
                printf("globe %-16s %s R=%.0f rings=%d star=%s\n", PLANET_TYPES[b.type].name, b.name.c_str(), b.radiusKm, (int)b.rings, STAR_CLASSES[s.cls].code);
                if (b.type == PT_GASGIANT) {   // N5-03: the storms a day later
                    const uint64_t FNV0 = 1469598103934665603ULL;
                    uint64_t h0 = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                    Framebuffer fb2; SpaceContext c2 = c; c2.t = t + 86400;
                    c2.shipPos = sys.bodyPos(bi, c2.t) + (shipPos - bp);   // ride along
                    sr.setupPalette(fb2, &sys, bi, second, 1.0); sr.render(fb2, c2); fb2.mush(2);
                    saveFB(fb2, "shots/tests/globe_GAS_GIANT_day_later.png");
                    uint64_t h1 = fnv(FNV0, fb2.idx.data(), fb2.idx.size() * sizeof(Pix));
                    printf("  storms a day later: frame %s -> shots/globe_GAS_GIANT_day_later.png\n", h0 != h1 ? "differs" : "IDENTICAL (FAIL)");
                }
            }
        }
    // O3 (R-302): the rocks of a belt seen from beside the biggest rock near an anchor in the belt
    {
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                if (sys.belts.empty()) continue;
                const Belt& bl = sys.belts[0];
                double t = 4321.0, mid = 0.5 * (bl.innerKm + bl.outerKm);
                Vec3 anchor = sys.star.pos + Vec3(mid, 0, 0);
                BeltRock best; double bestR = -1; int n = 0;
                sys.forBeltRocksNear(0, anchor, t, 2, 2, 1, [&](const BeltRock& rk) { n++; if (rk.radiusKm > bestR) { bestR = rk.radiusKm; best = rk; } });
                if (bestR < 0) continue;
                Vec3 toStar = normalize(sys.star.pos - best.pos), side = normalize(cross(toStar, Vec3(0, 1, 0)));
                Vec3 shipPos = best.pos + normalize(toStar * 0.55 + side * 0.75 + Vec3(0, 0.18, 0)) * std::max(6.0 * best.radiusKm, 2.0);
                Vec3 fwd = normalize(best.pos - shipPos);
                nb.update(shipPos);
                Framebuffer fb; SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                sr.setupPalette(fb, &sys, -1, -1, 1.0); sr.render(fb, c); fb.mush(2);
                saveFB(fb, "shots/tests/space_belt.png");
                BeltRock later; bool same = sys.beltRockAt(0, best.ir, best.ia, best.iy, best.m, t + 3600, later);
                printf("belt %s: %d rocks within two cells of the anchor, parked beside one %.1f km across, %d drawn (%d as meshes) -> shots/space_belt.png; an hour later the rock is %s, %.0f km along its orbit\n",
                       bl.name.c_str(), n, 2 * best.radiusKm, sr.lastBeltRocks, sr.lastBeltMeshes, same && std::fabs(later.radiusKm - best.radiusKm) < 1e-9 ? "still there" : "GONE (FAIL)", same ? length(later.pos - best.pos) : 0.0);
                found = true;
            }
        if (!found) printf("belt: none found in the scanned sectors\n");
    }
    // N5-01: the sky from a star-forming region: nebula patches over the star field
    {
        bool found = false;
        for (int64_t x = 100; x < 500 && !found; x += 2)
            for (int64_t z = 20; z < 400 && !found; z += 2) {
                if (galaxyRegion(x, 0, z) != REGION_NEBULA) continue;
                Vec3 pos((x + 0.5) * SECTOR_KM, 0, (z + 0.5) * SECTOR_KM);
                nb.update(pos);
                NebulaPatch np[16]; int n = nebulaPatches(pos / SECTOR_KM, np, 16);
                Framebuffer fb;
                SpaceContext c;
                c.sys = nullptr; c.stars = &nb.stars; c.t = 100.0; c.shipPos = pos;
                double bestYaw = 0; double bestG = -1;   // look toward the strongest patch
                for (int i = 0; i < n; i++) { double g = np[i].inten; if (g > bestG) { bestG = g; bestYaw = std::atan2(np[i].dir.x, np[i].dir.z); } }
                c.cam = cameraBasis(bestYaw, 0);
                sr.setupPalette(fb, nullptr, -1, -1, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                saveFB(fb, "shots/tests/space_nebula.png");
                printf("nebula region at sector %lld,0,%lld: %d patches -> shots/space_nebula.png\n", (long long)x, (long long)z, n);
                found = true;
            }
        if (!found) printf("nebula region: none found in the scanned sectors\n");
    }
    // 3) M5-01 a multiple system: both suns in one frame, and the companion's first world if it has one
    // 4) M5-07 a comet at periapsis, tail streaming away from the star
    bool binaryDone = false, cometDone = false;
    for (int64_t x = 150; x < 320 && !(binaryDone && cometDone); x++)
        for (int64_t z = 20; z < 140 && !(binaryDone && cometDone); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            if (!binaryDone && sys.companion >= 0) {
                double t = 2000.0;
                const Body& k = sys.bodies[sys.companion];
                Vec3 kp = sys.bodyPos(sys.companion, t);
                Vec3 along = normalize(kp - s.pos);
                Vec3 nrm = normalize(cross(along, Vec3(0, 1, 0)));
                // behind the primary (a real disc at the edge), the companion ahead as the brightest point in the sky
                double R = s.radiusKm;
                Vec3 shipPos = s.pos - along * (10.0 * R) + nrm * (4.0 * R) + Vec3(0, 1.0 * R, 0);
                Vec3 fwd = normalize(along * 10.0 - nrm * 2.5);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y));
                sr.setupPalette(fb, &sys, -1, -1, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                saveFB(fb, "shots/tests/space_binary.png");
                printf("binary %s: %s, companion %s R=%.0f L=%.3f at %.3g km (ecc %.2f), %d worlds of its own -> shots/space_binary.png\n", s.name.c_str(), sys.classString().c_str(),
                       STAR_CLASSES[k.starClass].name, k.radiusKm, k.luminosity, k.orbitRadiusKm, k.ecc, k.moonCount);
                for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.parent != sys.companion) continue;
                    Vec3 bp = sys.bodyPos(bi, t);
                    Vec3 toK = normalize(kp - bp);
                    Vec3 side = normalize(cross(toK, Vec3(0, 1, 0)));
                    Vec3 sp = bp + normalize(toK * 0.75 + side * 0.6 + Vec3(0, 0.2, 0)) * (b.radiusKm * 3.5);   // from the lit side
                    Vec3 f2 = normalize(bp - sp);
                    nb.update(sp);
                    Framebuffer fb2;
                    SpaceContext c2;
                    c2.sys = &sys; c2.stars = &nb.stars; c2.t = t; c2.shipPos = sp;
                    c2.cam = cameraBasis(std::atan2(f2.x, f2.z), std::asin(f2.y));
                    c2.bankBodyA = bi;
                    warmMaps(sr, sys, bi);
                    sr.setupPalette(fb2, &sys, bi, -1, 1.0);
                    sr.render(fb2, c2);
                    fb2.mush(2);
                    saveFB(fb2, "shots/tests/space_binary_world.png");
                    printf("  world of the companion: %s (%s) -> shots/space_binary_world.png\n", b.name.c_str(), PLANET_TYPES[b.type].name);
                    break;
                }
                binaryDone = true;
            }
            if (!cometDone)
                for (int bi = 0; bi < (int)sys.bodies.size() && !cometDone; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != PT_COMET) continue;
                    double t = (TAU - b.orbitPhase0) / TAU * b.orbitPeriod;   // mean anomaly 0: periapsis
                    Vec3 bp = sys.bodyPos(bi, t);
                    Vec3 away = normalize(bp - s.pos);
                    double refKm = sys.bodies[0].type == PT_COMET ? AU_GAME_KM : sys.bodies[0].orbitRadiusKm;
                    double lenKm = refKm * 0.5;
                    // seen from above the orbital plane, so the dust tail's curve along the orbit shows (N0-01)
                    Vec3 vel = sys.bodyVel(bi, t);
                    Vec3 side = normalize(cross(away, vel));
                    Vec3 back = normalize(-vel);
                    Vec3 shipPos = bp + side * (lenKm * 0.7) + away * (lenKm * 0.25) + back * (lenKm * 0.1);
                    Vec3 fwd = normalize(bp + away * (lenKm * 0.3) - shipPos);
                    nb.update(shipPos);
                    Framebuffer fb;
                    SpaceContext c;
                    c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                    c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y));
                    c.bankBodyA = bi;
                    sr.setupPalette(fb, &sys, bi, -1, 1.0);
                    sr.render(fb, c);
                    fb.mush(2);
                    saveFB(fb, "shots/tests/space_comet.png");
                    printf("comet %s: a=%.3g km e=%.2f q=%.3g km period %.1f d, at periapsis %.3g km from the star, %.0f km/s -> shots/space_comet.png\n", b.name.c_str(), b.orbitRadiusKm, b.ecc,
                           b.orbitRadiusKm * (1 - b.ecc), b.orbitPeriod / 86400, length(bp - s.pos), length(sys.bodyVel(bi, t)));
                    // N0-01 (B-201): the tail streams (frames 5 s apart differ, the knots moved), the comet drifts against
                    // the sky at x100 seen from the first planet, and the parking view shows the jets moving
                    {
                        const uint64_t FNV0 = 1469598103934665603ULL;
                        uint64_t h0 = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                        Framebuffer fb5; SpaceContext c5 = c; c5.t = t + 5;
                        sr.setupPalette(fb5, &sys, bi, -1, 1.0); sr.render(fb5, c5); fb5.mush(2);
                        uint64_t h5 = fnv(FNV0, fb5.idx.data(), fb5.idx.size() * sizeof(Pix));
                        printf("  tail 5 s later: frame %s; knot 0 at %.3f -> %.3f of the tail\n", h0 != h5 ? "differs" : "IDENTICAL (FAIL)", SpaceRenderer::cometKnot(0, t), SpaceRenderer::cometKnot(0, t + 5));
                        int obs = -1;
                        for (int j = 0; j < (int)sys.bodies.size(); j++) if (sys.bodies[j].parent < 0 && sys.bodies[j].type != PT_COMET && sys.bodies[j].type != PT_COMPANION) { obs = j; break; }
                        if (obs >= 0) {
                            Vec3 d0 = normalize(bp - sys.bodyPos(obs, t)), d1 = normalize(sys.bodyPos(bi, t + 500) - sys.bodyPos(obs, t + 500));
                            double ang = std::acos(clampd(dot(d0, d1), -1, 1));
                            double px = ang / (70 * DEG / 320);
                            printf("  seen from %s at x100 for 500 s: moved %.2f deg = %.1f px at 1x (%s)\n", sys.bodies[obs].name.c_str(), ang / DEG, px, px > 4 ? "ok" : "FAIL");
                        }
                        Vec3 toStar = normalize(s.pos - bp);
                        Vec3 side2 = normalize(cross(toStar, Vec3(0, 1, 0)));
                        Vec3 parkDir = normalize(toStar * 0.55 + side2 * 0.75 + Vec3(0, 0.18, 0));   // as startApproach parks
                        Vec3 sp = bp + parkDir * (b.radiusKm * 12.0);
                        Vec3 f2 = normalize(bp - sp);
                        nb.update(sp);
                        Framebuffer fbc; SpaceContext cc;
                        cc.sys = &sys; cc.stars = &nb.stars; cc.t = t; cc.shipPos = sp; cc.cam = cameraBasis(std::atan2(f2.x, f2.z), std::asin(f2.y)); cc.bankBodyA = bi;
                        warmMaps(sr, sys, bi);
                        sr.setupPalette(fbc, &sys, bi, -1, 1.0); sr.render(fbc, cc); fbc.mush(2);
                        saveFB(fbc, "shots/tests/space_comet_close.png");
                        uint64_t hc0 = fnv(FNV0, fbc.idx.data(), fbc.idx.size() * sizeof(Pix));
                        double nucPx = 2 * sr.bodyInfo[bi].radiusPx;
                        Framebuffer fbc3; SpaceContext cc3 = cc; cc3.t = t + 3;
                        cc3.shipPos = sys.bodyPos(bi, t + 3) + parkDir * (b.radiusKm * 12.0);   // parked: the ship rides along
                        sr.setupPalette(fbc3, &sys, bi, -1, 1.0); sr.render(fbc3, cc3); fbc3.mush(2);
                        uint64_t hc3 = fnv(FNV0, fbc3.idx.data(), fbc3.idx.size() * sizeof(Pix));
                        int changed = 0;
                        for (size_t k = 0; k < fbc.idx.size(); k++) if (fbc.idx[k] != fbc3.idx[k]) changed++;
                        (void)hc0; (void)hc3;
                        printf("  parked at 12 radii (%.0f km): nucleus %.0f px across -> shots/space_comet_close.png; 3 s later %d px changed (jets %s)\n", b.radiusKm * 12, nucPx, changed, changed > 500 ? "stream" : "DO NOT MOVE (FAIL)");
                    }
                    cometDone = true;
                }
        }
}

// ---- surface tests ------------------------------------------------------------
#include "surface/surface_view.h"
#include "core/input.h"

// find a time when the sun's altitude at the site is close to the target (radians): over one rotation from t0, and when that
// misses by over a degree (G-04: a season without that light at this latitude, a polar day) over eleven more epochs spread
// through the year (the parent's for a moon); `errOut` is the miss, so a finder can tell a world locked to its star (the sun
// never moves: another longitude) from a latitude that never sees that light
// G-04: the altitude of the light the scene is lit by: round a binary, the two suns blended by their strength above the horizon
// as `SurfaceView::computeEnvironment` blends them (the finders used to match the primary alone: a "night" under the companion)
static double effectiveSunAlt(const SurfaceSite& site, double t) {
    SunInfo s1 = site.sun(t), s2;
    if (!site.sun2(t, s2)) return s1.altitude;
    double w1 = s1.lightFactor * smoothstep(-0.12, 0.05, s1.dirLocal.y), w2 = s2.lightFactor * smoothstep(-0.12, 0.05, s2.dirLocal.y);
    if (w1 + w2 < 1e-6 || w2 / (w1 + w2) <= 0.02) return s1.altitude;
    return std::asin(clampd(normalize(s1.dirLocal * w1 + s2.dirLocal * w2).y, -1, 1));
}
static double findTime(SurfaceSite& site, double targetAlt, double t0, double* errOut = nullptr) {
    const Body& b = site.sys->bodies[site.body];
    double period = std::fabs(b.rotPeriod), year = b.parent >= 0 ? site.sys->bodies[b.parent].orbitPeriod : b.orbitPeriod;
    double best = t0, bestErr = 1e9;
    auto scanDay = [&](double from) {
        for (int i = 0; i < 720; i++) {
            double t = from + period * i / 720.0;
            double err = std::fabs(effectiveSunAlt(site, t) - targetAlt);
            if (err < bestErr) { bestErr = err; best = t; }
        }
    };
    scanDay(t0);
    for (int k = 1; k < 12 && bestErr > 1.0 * DEG; k++) scanDay(t0 + year * k / 12.0);
    if (errOut) *errOut = bestErr;
    return best;
}

static void renderSurface(int onlyType) {
    SpaceRenderer sr;
    StarNeighborhood nb;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (PLANET_TYPES[i].landable && (onlyType < 0 || i == onlyType)) remaining++; else done[i] = true;
    for (int64_t x = 150; x < 320 && remaining; x++)
        for (int64_t z = 20; z < 140 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
                const Body& b = sys.bodies[bi];
                if (done[b.type]) continue;
                done[b.type] = true; remaining--;
                std::string tn = PLANET_TYPES[b.type].name;
                for (char& ch : tn) if (ch == ' ') ch = '_';
                struct Scene { const char* tag; double latDeg; double alt; double yawOff; double pitch; };
                Scene scenes[] = {{"noon", 12, 45 * DEG, 0.0, 0.0}, {"sunset", 12, 4 * DEG, 0.0, 0.05}, {"night", 12, -40 * DEG, 0.5, 0.15}, {"north", 82, 5 * DEG, 0.0, 0.0}};
                for (const Scene& sc : scenes) {
                    SurfaceView sv;
                    double lon = 0.7 + bi * 0.3;
                    // choose a forested land site for felisian (full-detail check)
                    double latUse = sc.latDeg * DEG;
                    if (b.type == PT_FELISIAN) {
                        BodyGen g = BodyGen::make(b);
                        bool found = false;
                        for (double la = sc.latDeg; la < sc.latDeg + 30 && !found; la += 2)
                            for (int k = 0; k < 256 && !found; k++) {
                                double ln = k * TAU / 256;
                                SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                                if (ss.material == MAT_FOREST && ss.height > 20) { lon = ln; latUse = la * DEG; found = true; }
                            }
                    }
                    if (b.type == PT_MOLTEN) {
                        // land 400 m from a lava lake
                        BodyGen g = BodyGen::make(b);
                        bool found = false;
                        for (double la = sc.latDeg; la < sc.latDeg + 20 && !found; la += 0.02)
                            for (double ln = 0; ln < TAU && !found; ln += 0.0006) {
                                SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                                if (ss.material == MAT_LAVA && ss.glow > 0.9) { lon = ln + 400.0 / (b.radiusKm * 1000.0); latUse = la * DEG; found = true; }
                            }
                    }
                    sv.init(&sys, bi, latUse, lon, 1000.0);
                    double t = findTime(sv.site, sc.alt, 1000.0);
                    sv.init(&sys, bi, latUse, lon, t);
                    // look toward the sun's azimuth (plus offset)
                    SunInfo si = sv.site.sun(t);
                    sv.player.yaw = si.azimuth + sc.yawOff;
                    sv.player.pitch = sc.pitch;
                    nb.update(sv.site.worldPos(t, 0, 0, 0));
                    Input in;
                    sv.update(0.016, in, t, false);
                    Framebuffer fb;
                    double t0 = nowSec();
                    sv.render(fb, t, nb.stars, sr, 1.0);
                    double t1 = nowSec();
                    fb.mush(2);
                    std::string fn = "shots/tests/surf_" + tn + "_" + sc.tag + ".png";
                    saveFB(fb, fn.c_str());
                    printf("%-16s %-7s %s lat=%.0f sunAlt=%.1f az=%.0f time=%.2f render=%.1f ms  T=%.0fC P=%.2f g=%.1f cloud=%.2f rain=%.2f snow=%.2f sky=%.2f uw=%d rocks=%d/%d\n",
                           PLANET_TYPES[b.type].name, sc.tag, b.name.c_str(), sc.latDeg, si.altitude / DEG, si.azimuth / DEG, si.dayFraction,
                           (t1 - t0) * 1000, sv.env.temperatureC, sv.env.pressureAtm, sv.site.gravity, sv.env.cloudCover, sv.env.rain, sv.env.snow, sv.env.skyBrightness, (int)sv.player.underwater, sv.lastRocksDrawn, sv.lastRocksCandidates);
                }
            }
        }
}

// ---- scripted game flow --------------------------------------------------------
#include "game/game.h"
#include "game/ui.h"
static void applyTestScale(Game& g) { g.settings.renderScale = g_testScale; g.applySettings(); }

static bool firstBodyOfType(int type, StarSystem& sysOut, int& biOut);   // O6-03: the first body of a type in the scan region (defined with forTypeBodies below)
static void landSitesOf(const StarSystem& sys, int bi, uint64_t salt, int n, std::vector<std::pair<double, double>>& out);   // random land sites of a body (defined below)
// B-404: the share of points round (lat, lon) whose material differs between the 4 and 16 m samples (out[0]) and the 16
// and 64 m samples (out[1]) of the planet function, over a square of +-halfM at stepM; the mode `matlod` measures all
// five ring scales, the unit test guards the walked rings' agreement with this
static void materialAgreement(const StarSystem& sys, int bi, double lat, double lon, double halfM, double stepM, double out[2]);
// G-02: `home`: the candidates of the new-game search (yellow or orange stars with a living planet in sectors 176..196 x
// 36..56), each parked at its first living planet with the landing map open as a new game does, timed until the default
// site's drainage tiles are in the cache (the bench's descent budget, KI-338); prints what the pick needs: no nebula
// patches in the sky, tiles under 2.5 s, a temperate world, the score, and the sector to pin in Game::newGame.
static void runHome() {
    struct Cand { Star s; int body; int score; double tempK, radiusKm; int rings, moons, nebula; double tiles; };
    std::vector<Cand> cands;
    for (int64_t x = 176; x < 196; x++)
        for (int64_t z = 36; z < 56; z++) {
            Star st; if (!starInSector(x, 0, z, st)) continue;
            if (st.cls != STAR_YELLOW && st.cls != STAR_ORANGE) continue;
            StarSystem ss; ss.generate(st);
            Cand c; c.s = st; c.body = -1; c.score = 0; c.rings = 0; c.moons = 0;
            for (auto& b : ss.bodies) {
                if (b.type == PT_FELISIAN && b.parent < 0) { c.score += 50; if (c.body < 0) { c.body = b.index; c.tempK = b.tempK; c.radiusKm = b.radiusKm; c.rings = b.rings ? 1 : 0; c.moons = b.moonCount; } }
                if (b.parent < 0) c.score += 3;
                if (b.rings) c.score += 5;
            }
            if (c.body < 0) continue;
            NebulaPatch np[16]; c.nebula = nebulaPatches(Vec3(x + 0.5, 0.5, z + 0.5), np, 16);
            Game game;
            game.savePrefix = "shots/tests/home_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/home_guide.txt";
            game.newGame();
            game.settings.renderScale = 1; game.applySettings();
            game.testParkAt(st, c.body);
            Input in;
            in.pressed[KEY_C] = true; in.down[KEY_C] = true; game.frame(in, 1.0 / 60); in.newFrame(); in.down[KEY_C] = false;
            double t0 = nowSec();
            while (nowSec() - t0 < 8.0 && !game.testDrainageReady()) { game.frame(in, 1.0 / 60); in.newFrame(); }
            c.tiles = nowSec() - t0;
            cands.push_back(c);
            printf("  %-14s %3lld 0 %3lld  %-12s score %3d  %s R %4.0f km  %3.0f K  rings %d moons %d  nebula patches %d  tiles %.1f s%s  u %.3f%s\n", st.name.c_str(), (long long)x, (long long)z, STAR_CLASSES[st.cls].name, c.score,
                   ss.bodies[c.body].name.c_str(), c.radiusKm, c.tempK + 33, c.rings, c.moons, c.nebula, c.tiles, c.tiles >= 8.0 ? " (not ready)" : "", unitFromHash(sectorSeed(x, 0, z)),
                   x == HOME_SX && z == HOME_SZ ? "  <- pinned" : "");
        }
    // the pick: a yellow star first, two living worlds, the first of them temperate (275-320 K at the surface), its tiles under
    // a second, the fewest nebula patches (every candidate of the region has some: the arm's star-forming cells), then the score;
    // the sector hash against the density says how safe the pin is
    const Cand* best = nullptr;
    auto rank = [](const Cand& c) { return (c.s.cls == STAR_YELLOW ? 1000 : 0) + (c.score >= 100 ? 500 : 0) - 40 * c.nebula + c.score; };
    for (const Cand& c : cands) {
        if (c.tiles >= 1.0 || c.tempK + 33 < 275 || c.tempK + 33 > 320) continue;
        if (!best || rank(c) > rank(*best)) best = &c;
    }
    if (best) {
        double u = unitFromHash(sectorSeed(best->s.sx, 0, best->s.sz)), dens = galaxyDensity((double)best->s.sx, 0, (double)best->s.sz);
        printf("home: pin sector %lld 0 %lld (%s, score %d, its world %.0f K at the surface, tiles %.1f s, %d nebula patches, exists while the density stays above %.3f; it is %.3f here%s)%s\n", (long long)best->s.sx, (long long)best->s.sz, best->s.name.c_str(), best->score, best->tempK + 33, best->tiles, best->nebula, u, dens,
               u > 0.8 * dens ? ": a thin margin, the fallback search takes over if the galaxy is retuned" : "", best->s.sx == HOME_SX && best->s.sz == HOME_SZ ? ", as pinned" : ", NOT the pinned one");
    }
    else printf("home: no candidate passes (tiles under a second, a world of 275-320 K)\n");
}

static void runGameFlow() {
    Game game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.shotsDir = "shots/tests/test_gallery";
    game.moviesDir = "shots/tests/test_movies"; game.recFixedTake = true;   // M6-05: the recorder writes into shots/, take 1 overwritten
    remove("shots/tests/test_guide.txt");
    applyTestScale(game);
    Input in;
    auto run = [&](double seconds) {
        int n = (int)(seconds * 30);
        for (int i = 0; i < n; i++) { game.frame(in, 1.0 / 30); if (game.recording) game.recordFrame(); in.newFrame(); }
    };
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; };
    auto shot = [&](const char* name) {
        std::string fn = std::string("shots/tests/flow_") + name + ".png";
        writePNG(fn.c_str(), game.output(), FBW, FBH);
        printf("%-14s state=%d status='%s'\n", name, (int)game.state, game.testStatus().c_str());
    };
    double t0 = nowSec();
    run(0.5); shot("title");
    game.testAttract(30); run(0.3); shot("attract");   // M6-06
    press(KEY_C); run(0.1); shot("credits"); press(KEY_ESCAPE); run(0.1);
    press(KEY_ENTER); run(1.0); shot("space"); printf("  %s\n", game.testDebugInfo().c_str());
    // M2: walk to the GOES console, use it, then the front computer, the roof and the glass hull
    game.testCabinGoto(-1.2, 0.0, -PI / 2); run(0.2); shot("cabin"); printf("  facing %d\n", game.testCabinFacing());
    press(KEY_E); run(0.1); game.testTypeText("SL 4"); run(0.1); shot("console"); press(KEY_ESCAPE);
    game.testCabinGoto(0.0, 1.3, 0.0); run(0.2); press(KEY_E); run(0.1); shot("shipscreen"); press(KEY_ESCAPE);
    {   // C-06: the shard decoder on the back wall: empty; with three shards of Aieliaalas II in the guide (the first reading waits, the words resolving); with ten (every shard re-read, the whole text)
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); shot("decoder"); printf("  facing %d (7 is the decoder)\n", game.testCabinFacing());   // looking down at the desk from 1.5 m
        press(KEY_E); run(0.1); shot("shards_none"); press(KEY_ESCAPE); run(0.1);
        for (int i = 0; i < 3; i++) game.guide.shards.insert("151,0,25/1/S" + std::to_string(i));
        press(KEY_E); run(0.1); shot("shards_worlds"); press(KEY_ENTER); run(0.1); shot("shards_list");
        game.testShardsOpenIndex(2); run(1.2); shot("shards_decoding"); printf("  %s; the synth: %s\n", game.testShardsInfo().c_str(), game.audio.speech ? "a speech set" : "NO speech set");   // C-04: by index, S1 of the three being a piece; C-05: the words resolve as the recording speaks
        press(KEY_ENTER); run(0.2); shot("shards_text"); printf("  the wait skipped: %s\n", game.testShardsInfo().c_str());
        press(KEY_RIGHT); run(0.1); press(KEY_ENTER); run(0.2); printf("  the next, the wait skipped: %s\n", game.testShardsInfo().c_str());
        press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        for (int i = 3; i < 10; i++) game.guide.shards.insert("151,0,25/1/S" + std::to_string(i));
        press(KEY_E); run(0.1); press(KEY_ENTER); run(0.1); shot("shards_list10");
        game.testShardsOpenIndex(0); run(0.2); press(KEY_ENTER); run(0.2); shot("shards_full"); printf("  %s\n", game.testShardsInfo().c_str());
        press(KEY_ENTER); run(1.5); shot("shards_speaking"); printf("  spoken again: %s; the synth: %s\n", game.testShardsInfo().c_str(), game.audio.speech ? "a speech set" : "NO speech set");   // C-05: a read shard spoken again, the word being spoken underlined
        press(KEY_SPACE); run(0.3); printf("  paused: %s\n", game.testShardsInfo().c_str());
        press(KEY_ESCAPE); run(0.1); printf("  after esc: level %d, the synth: %s\n", game.testShardsLevel(), game.audio.speech ? "a speech STILL set" : "stopped");
        {   // C-04: the first piece of music among the ten plays on the screen, is named, and stops when the screen is left
            int mi = -1;
            for (int i = 0; i < 10 && mi < 0; i++) { Star ms; StarSystem msys; if (starInSector(151, 0, 25, ms, true)) { msys.generate(ms); const Body& mb = msys.bodies[1]; BodyGen mg = BodyGen::make(mb); Lore mL = loreOf(msys, mb, mg); if (shardOf(msys, mb, mg, mL, i).music) mi = i; } }
            if (mi >= 0 && game.testShardsOpenIndex(mi)) {
                run(2.0); shot("shards_music"); printf("  %s; the synth: %s\n", game.testShardsInfo().c_str(), game.audio.piece ? "a piece set" : "NO piece set");
                press(KEY_N); run(0.1); shot("shards_music_name"); game.testTypeText("THE RIVER AT NIGHT"); run(0.3); shot("shards_music_named");
                printf("  named: '%s'; %s\n", game.guide.names.count("151,0,25/1/S" + std::to_string(mi)) ? game.guide.names["151,0,25/1/S" + std::to_string(mi)].c_str() : "-", game.testShardsInfo().c_str());
                press(KEY_ESCAPE); run(0.1); printf("  after esc: level %d, the synth: %s\n", game.testShardsLevel(), game.audio.piece ? "a piece STILL set" : "stopped");
            } else printf("  no piece of music among the first ten shards (index %d)\n", mi);
        }
        press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        printf("  decoder: state %d after leaving, %zu shards decoded in the guide\n", (int)game.state, game.guide.decoded.size());
    }
    press(KEY_PAGE_UP); run(0.3); shot("roof"); press(KEY_PAGE_DOWN); run(0.1);
    press(KEY_Y); press(KEY_U); game.testCabinGoto(0.4, -1.6, 0.3); run(0.6); shot("cabin_glass"); press(KEY_Y); press(KEY_U); run(0.1);
    press(KEY_TAB); run(0.1); shot("list"); press(KEY_ESCAPE);
    press(KEY_I); run(0.1); shot("data"); press(KEY_SPACE);
    press(KEY_H); run(0.1); shot("help"); press(KEY_ESCAPE);
    press(KEY_O); run(0.5);
    {   // C-07: the signal radar: on at the start, a sweep of the sky (the scope filling), the view turned onto the first people's
        // signal within reach by the hook (the recording heard through the static), the hold, the lock, Enter targets its star, off
        game.testCabinGoto(-1.4, -1.1, PI, -0.6); run(0.2); shot("radar_set"); printf("  facing %d (8 is the radar set)\n", game.testCabinFacing());   // the set beside the decoder, looked down at from 1.6 m
        press(KEY_E); run(0.3); shot("radar_on"); printf("  %s\n", game.testRadarInfo().c_str());   // E on the set switches the receiver on
        game.testCabinGoto(0.4, -1.6, 0.0, 0.0);
        in.down[KEY_RIGHT] = true; run(2.0); in.down[KEY_RIGHT] = false; run(0.3); shot("radar_sweep");
        int who = -1;
        for (int i = 0; i < game.testRadarSignals() && who < 0; i++) if (game.testRadarKind(i) == SIG_PEOPLE) who = i;
        if (who < 0 && game.testRadarSignals() > 0) who = 0;
        if (who >= 0 && game.testAimAtSignal(who)) {
            run(1.5); shot("radar_hold"); printf("  %s\n", game.testRadarInfo().c_str());
            run(5.5); shot("radar_lock"); printf("  %s\n", game.testRadarInfo().c_str());
            press(KEY_ENTER); run(1.0); shot("radar_flight"); printf("  Enter: '%s'; %s; the synth: %s\n", game.testStatus().c_str(), game.testRadarInfo().c_str(), game.audio.radio ? "STILL the radar's" : "released");   // the Vimana flight to the signal's star, the camera left
            press(KEY_V); run(0.3); printf("  the flight aborted for the rest of the flow: '%s' (%s)\n", game.testStatus().c_str(), game.testDebugInfo().c_str());
        } else printf("  no signal within reach of the start\n");
    }
    // M3: rename the star through the guide, look at the star map
    press(KEY_G); run(0.1); shot("guide"); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("text_entry");
    for (int i = 0; i < 20; i++) press(KEY_BACKSPACE);
    game.testTypeText("HOME OF CATS"); run(0.1);
    printf("  renamed star: '%s'\n", game.testStarName().c_str());
    press(KEY_M); run(0.3); shot("starmap"); press(KEY_ESCAPE);
    press(KEY_R); press(KEY_N); run(0.2); shot("targeting");
    press(KEY_ENTER); run(0.2);
    press(KEY_V); run(3.0); shot("vimana");
    run(12.0); shot("arrived");
    int lb = game.testLandableBody(); bool parkedByTest = false;
    printf("landable body %d of %s (the generator calls this star %s)\n", lb, game.testBodyTypes().c_str(), game.testStarGenName().c_str());
    if (lb >= 0 && game.testBodyType(lb) == PT_COMET) {
        // O6-03: the nearest system holds nothing but comets since GEN 9; the walk, the buggy and the water below need a world
        // with gravity: park at the first felisian world of the scan instead (the approach frames come from the comet section)
        StarSystem fs; int fb = -1;
        if (firstBodyOfType(PT_FELISIAN, fs, fb)) { game.testParkAt(fs.star, fb); lb = fb; run(0.5); parkedByTest = true; printf("  only comets here: parked at %s body %d for the landing\n", fs.star.name.c_str(), fb); }
    }
    if (lb >= 0) {
        // G-01: approach unless the test parked the ship itself (the old check compared the star's generated name with
        // the scan's first felisian system, Skeatoltdos, which the comet case parks at; a pinned name breaks with the galaxy)
        if (!parkedByTest) {
            game.testAimAtBody(lb);
            press(KEY_TAB); run(0.1);
            for (int i = 0; i < lb; i++) press(KEY_DOWN);
            press(KEY_ENTER); run(2.0); shot("approach");
            run(9.0); shot("orbit"); printf("  %s\n", game.testDebugInfo().c_str());
            press(KEY_C); run(0.2);
        }
        if (game.testBodyType(lb) != PT_FELISIAN) {
            // G-04: the landing below (the walk, the herd, the water) wants a living world: the arrival's world gave the approach
            // frames; park at the scan's first felisian world now (the pinned home's nearest star holds none)
            StarSystem fs; int fb = -1;
            if (firstBodyOfType(PT_FELISIAN, fs, fb)) { game.testParkAt(fs.star, fb); lb = fb; run(0.5); printf("  no living world at this star: parked at %s body %d for the landing\n", fs.star.name.c_str(), fb); }
        }
        shot("landing_map");
        press(KEY_R); run(0.1); shot("landing_map2");
        press(KEY_Z); run(0.3); shot("landing_zoom"); press(KEY_Z); run(0.1);   // O2 (R-301): the sector zoom
        press(KEY_J); run(0.1); shot("landing_sky");   // M5-09
        game.testLandSite(); run(0.1);   // G-04: a lit land site from the body's seed (J leaves the cursor under the moon: in the sea at Heilya II; R is seeded by the clock)
        press(KEY_ENTER); run(2.0); shot("descent");
        game.currentSlot = 2; in.down[KEY_LEFT_CONTROL] = true; press(KEY_S); in.down[KEY_LEFT_CONTROL] = false;   // save mid-descent to slot 2
        game.currentSlot = 1;
        run(6.0); shot("surface");
        printf("  %s\n", game.testRangeInfo().c_str());   // O1: the rangefinder
        in.down[KEY_W] = true; run(3.0); in.down[KEY_W] = false;
        in.mouseDx = 400; game.frame(in, 1.0 / 30); in.newFrame();
        run(0.5); shot("surface2");
        // M8: sprint, jump + jetpack, the buggy
        in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true; run(2.5); shot("sprint"); in.down[KEY_LEFT_SHIFT] = false; in.down[KEY_W] = false;
        printf("  sprint: stamina %.0f, speed %.1f m/s\n", game.testPlayerStamina(), game.testPlayerSpeed());
        press(KEY_SPACE); in.down[KEY_SPACE] = true; run(2.5); shot("jetpack"); in.down[KEY_SPACE] = false;
        printf("  jetpack: %.1f m above ground\n", game.testPlayerAlt());
        run(4.0);
        press(KEY_K); run(0.3);                      // capsule next to us
        press(KEY_B); run(2.5);                      // unfold
        game.testWalkToBuggy();
        press(KEY_E); run(0.2); shot("buggy_seat");
        // N4-02: a long drive on open ground; frames at speed, airborne if it happens, the chase view
        {
            bool airShot = false; double top = 0;
            double openRun = game.testDriveOpen();
            if (openRun > 0) printf("  open run of %.0f m found for the drive\n", openRun);
            else printf("  no open run near this landing (forest): a short smoke drive; vesperis_test drive is the long one\n");
            in.down[KEY_W] = true;
            for (int i = 0; i < 30 * 45; i++) {
                game.frame(in, 1.0 / 30); in.newFrame();
                if (i == 30 * 8) { in.down[KEY_D] = true; }
                if (i == 30 * 9) { in.down[KEY_D] = false; }
                if (i == 30 * 20) shot("buggy_drive");
                top = std::max(top, game.testBuggySpeed());
                if (!airShot && game.testBuggyAirborne()) { shot("buggy_air"); airShot = true; }
            }
            printf("  drive: %s, top %.0f km/h%s\n", game.testBuggyInfo().c_str(), top * 3.6, airShot ? ", airborne once" : "");
        }
        press(KEY_V); run(0.2); shot("buggy_chase"); in.down[KEY_W] = false; run(1.5);
        printf("  buggy: %s\n", game.testBuggyInfo().c_str());
        press(KEY_E); run(0.5);
        {   // B-205 / R-204: the capsule always comes; B at it unfolds a new buggy and scraps the old one
            press(KEY_K); run(0.3);
            bool recalled = game.testStatus().find("RECALLED") != std::string::npos;
            press(KEY_B); run(0.3);
            bool scrapped = game.testStatus().find("SCRAPPED") != std::string::npos;
            printf("  recall with the buggy out: %s; B redeploys: %s (%s)\n", recalled ? "ok" : "FAIL", scrapped ? "ok" : "FAIL", game.testBuggyInfo().c_str());
        }
        {   // R-403: the drone: F unfolds it at the capsule, E gets in, Space lifts it and climbs, a flight at full thrust with a turn,
            // the chase view, S brakes it, Shift brings it down and lands it, E out; the capsule is recalled beside the landing
            press(KEY_F); run(2.5);
            bool unfolded = game.testDroneInfo().find("deployed=1") != std::string::npos;
            game.testWalkToDrone(); press(KEY_E); run(0.2); shot("drone_seat");
            bool seated = game.testDroneInfo().find("in=1") != std::string::npos;
            in.down[KEY_SPACE] = true; run(6.0);   // lift off, 6 s of climb
            double top = 0, altMax = 0; bool lifted = !game.testDroneLanded();
            in.down[KEY_W] = true;
            for (int i = 0; i < 30 * 16; i++) {
                game.frame(in, 1.0 / 30); in.newFrame();
                if (i == 30 * 6) in.down[KEY_SPACE] = false;
                if (i == 30 * 9) in.down[KEY_D] = true;
                if (i == 30 * 11) in.down[KEY_D] = false;
                if (i == 30 * 12) shot("drone_flight");
                top = std::max(top, game.testDroneSpeed()); altMax = std::max(altMax, game.testDroneAlt());
            }
            press(KEY_V); run(0.2); shot("drone_chase"); press(KEY_V); run(0.1);
            in.down[KEY_W] = false; in.down[KEY_S] = true;
            int n = 0; while (game.testDroneSpeed() > 2 && n++ < 30 * 30) { game.frame(in, 1.0 / 30); in.newFrame(); }
            in.down[KEY_S] = false; in.down[KEY_LEFT_SHIFT] = true;
            int landT = 0; while (!game.testDroneLanded() && landT++ < 30 * 90) { game.frame(in, 1.0 / 30); in.newFrame(); }
            in.down[KEY_LEFT_SHIFT] = false; run(0.3);
            bool landed = game.testDroneLanded();
            press(KEY_E); run(0.3);
            bool out = game.testDroneInfo().find("in=0") != std::string::npos;
            printf("  drone: unfolded %s, in %s, lifted %s, top %.0f km/h, %.0f m up, landed %s after %.0f s, out %s; %s\n", unfolded ? "ok" : "FAIL", seated ? "ok" : "FAIL", lifted ? "ok" : "FAIL", top * 3.6, altMax, landed ? "ok" : "FAIL", landT / 30.0, out ? "ok" : "FAIL", game.testDroneInfo().c_str());
            press(KEY_K); run(0.3);
        }
        press(KEY_M); run(0.2);
        press(KEY_N); run(0.2); shot("sectormap");
        for (int i = 0; i < 5; i++) {   // O1-04: out to the widest level this world allows, a frame per level (B-402: the grid's step)
            press(KEY_MINUS); run(0.3);
            if (i < 4) shot((std::string("sectormap_z") + std::to_string(i + 1)).c_str());
        }
        shot("sectormap_wide"); printf("  %s\n", game.testMarksInfo().c_str());
        for (int i = 0; i < 5; i++) press(KEY_EQUAL);
        press(KEY_ESCAPE);
        press(KEY_X); run(0.2); shot("creatures");
        // N3-03: walk up to the first herd and log how it reacts
        if (game.testGotoHerd(34)) {
            run(0.5); shot("herd_approach"); printf("  herd before: %s\n", game.testHerdStates().c_str());
            in.down[KEY_W] = true; run(4.0); in.down[KEY_W] = false; run(0.5);
            press(KEY_X); run(0.1); shot("herd_flee"); printf("  herd after walking in: %s\n", game.testHerdStates().c_str());
        } else printf("  no herd at this site\n");
        // M6-05 photo tools: photo mode with the free camera, a panorama, three recorded frames
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_P); in.down[KEY_LEFT_CONTROL] = false; run(0.1); shot("photo");
        in.down[KEY_W] = true; in.down[KEY_SPACE] = true; run(0.6); in.down[KEY_W] = false; in.down[KEY_SPACE] = false; shot("photo_flight");
        {
            std::vector<uint32_t> pano; int pw, ph;
            game.renderPanorama(pano, pw, ph);
            writePNG("shots/tests/flow_panorama.png", pano.data(), pw, ph);
            printf("panorama %dx%d -> shots/flow_panorama.png\n", pw, ph);
        }
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_P); in.down[KEY_LEFT_CONTROL] = false; run(0.1);
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_R); in.down[KEY_LEFT_CONTROL] = false; run(0.1);
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_R); in.down[KEY_LEFT_CONTROL] = false;
        printf("recorder: take %d, %d frames%s\n", game.recTake, game.recFrame, game.recFrame >= 3 ? " (ok)" : " FAIL");
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_T); in.down[KEY_LEFT_CONTROL] = false; run(0.5); shot("timelapse");   // M5-06
        press(KEY_T); run(0.1);   // back to x1
        press(KEY_V); press(KEY_V); run(0.1); shot("supervision"); press(KEY_V); press(KEY_V); press(KEY_V);
        // M3: a note, the log, the statistics and the gallery
        press(KEY_G); press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER); game.testTypeText("STRANGE RED TREES HERE"); run(0.1);
        press(KEY_J); run(0.1); shot("log"); press(KEY_ESCAPE);
        press(KEY_G); for (int i = 0; i < 6; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("stats"); press(KEY_ESCAPE);
        mkdir("shots/tests/test_gallery", 0755);
        writePNG("shots/tests/test_gallery/screenshot_0.png", game.output(), FBW, FBH);
        { FILE* sc = fopen("shots/tests/test_gallery/screenshot_0.txt", "w"); if (sc) { fprintf(sc, "%s\n", game.screenshotCaption().c_str()); fclose(sc); } }
        press(KEY_G); for (int i = 0; i < 5; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("gallery"); press(KEY_ESCAPE);
        printf("  log entries: %d\n", game.testLogEntries());
        for (int i = 0; i < 3; i++) press(KEY_T);
        run(4.0); shot("surface3");
        press(KEY_I); run(0.1); shot("surface_data"); press(KEY_SPACE);
        press(KEY_ESCAPE); run(0.1); shot("menu");
        press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("settings"); press(KEY_ESCAPE); press(KEY_ESCAPE);
        press(KEY_ESCAPE); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("slots"); press(KEY_ENTER); run(0.1);   // menu > save > slot 1
        press(KEY_K); run(0.5);
        press(KEY_Q); run(3.0); shot("ascent");
        run(6.0); shot("back_in_space");
        {   // B-205: a second landing (same world, another site) starts with no buggy
            printf("  before the second landing: %s\n", game.testMarksInfo().c_str());
            press(KEY_C); run(0.3); press(KEY_ENTER); run(8.0); shot("second_landing");
            bool none = game.testBuggyInfo().find("deployed=0") != std::string::npos;
            printf("  second landing: %s -> %s\n", game.testBuggyInfo().c_str(), none ? "no buggy (ok)" : "FAIL: the buggy carried over");
            std::string marks = game.testMarksInfo();   // B-303: the trail and the waypoint of the first landing must be gone
            bool fresh = marks.find("trail=1 ") != std::string::npos && marks.find("waypoint=0") != std::string::npos;
            press(KEY_N); run(0.2); shot("second_sectormap"); press(KEY_ESCAPE);
            printf("  marks after the second landing: %s -> %s\n", marks.c_str(), fresh ? "fresh (ok)" : "FAIL: the old landing's marks carried over");
            press(KEY_K); run(0.3); press(KEY_Q); run(9.0);   // back to space for the rest of the flow
        }
    }
    // N0-01 (B-201): park at the first comet near the start; the HUD shows its speed and plunge, the data sheet its distance now
    {
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && !found; bi++) {
                    if (sys.bodies[bi].type != PT_COMET) continue;
                    game.testParkAt(s, bi); run(0.5); shot("comet_map");   // O4: comets are landable, so parking opens the landing map
                    press(KEY_ESCAPE); run(0.3); shot("comet");
                    printf("  comet HUD: %s\n", game.cometMotionString(bi).c_str());
                    press(KEY_I); run(0.1); shot("comet_data"); press(KEY_SPACE);
                    press(KEY_TAB); run(0.1); shot("comet_list"); press(KEY_ESCAPE);
                    // O4 (R-303): land on the nucleus: microgravity, no buggy, floating jumps that Ctrl brings down, the tail overhead
                    press(KEY_C); run(0.2); press(KEY_ENTER); run(8.0); shot("comet_surface");
                    printf("  on the comet: escape velocity %.1f m/s, %s, '%s'\n", game.testEscapeVelocity(), game.testRangeInfo().c_str(), game.testStatus().c_str());
                    press(KEY_B); run(0.2); printf("  B on the comet: '%s'\n", game.testStatus().c_str());
                    in.down[KEY_W] = true; run(3.0); printf("  walking at %.2f m/s (the cap is %.2f)\n", game.testPlayerSpeed(), 0.3 * game.testEscapeVelocity()); in.down[KEY_W] = false;
                    press(KEY_SPACE); run(4.0); shot("comet_jump"); double up = game.testPlayerAlt();
                    in.down[KEY_LEFT_CONTROL] = true; run(3.0); in.down[KEY_LEFT_CONTROL] = false;
                    printf("  a jump: %.2f m up after 4 s, %.2f m after 3 s of Ctrl\n", up, game.testPlayerAlt());
                    in.mouseDy = -700; game.frame(in, 1.0 / 30); in.newFrame(); run(0.2); shot("comet_sky");
                    in.mouseDy = 700; game.frame(in, 1.0 / 30); in.newFrame();
                    press(KEY_N); run(0.2); shot("comet_sectormap"); press(KEY_ESCAPE);
                    press(KEY_K); run(0.3); press(KEY_Q); run(9.0);
                    found = true;
                }
            }
    }
    // O3 (R-302): fly to the first belt near the start, park beside one of its rocks; the capsule refuses; the sheet
    {
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                if (sys.belts.empty()) continue;
                game.testParkAt(s, -1); run(0.3);
                press(KEY_TAB); run(0.1);
                for (int i = 0; i < (int)sys.bodies.size(); i++) press(KEY_DOWN);
                shot("belt_list");
                press(KEY_ENTER); run(3.0); shot("belt_approach");
                run(12.0); shot("belt");
                printf("  belt: %s; parked belt %d; '%s'\n", game.testDebugInfo().c_str(), game.testParkedBelt(), game.testStatus().c_str());
                press(KEY_I); run(0.1); shot("belt_data"); press(KEY_SPACE);
                press(KEY_C); run(0.2); printf("  C in the belt: '%s'\n", game.testStatus().c_str());
                press(KEY_X); run(0.3); shot("belt_rock");
                printf("  %s\n", game.testBeltDraw().c_str());
                found = true;
            }
        if (!found) printf("belt: none found\n");
    }
    Game g2; g2.savePrefix = "shots/tests/test_save"; g2.settingsPath = "shots/tests/test_settings.txt"; g2.guidePath = "shots/tests/test_guide.txt";
    g2.guide.load(g2.guidePath);
    printf("load: %d (newest -> slot %d, state %d)\n", (int)g2.loadNewest(), g2.currentSlot, (int)g2.state);
    printf("  guide reloaded: %zu names, %d log entries, %zu shards and %zu decoded, star '%s'\n", g2.guide.names.size(), g2.testLogEntries(), g2.guide.shards.size(), g2.guide.decoded.size(), g2.testStarName().c_str());   // C-06: the decoded lines round-trip
    g2.frame(in, 1.0 / 30);
    writePNG("shots/tests/flow_loaded.png", g2.output(), FBW, FBH);
    Game g3; g3.savePrefix = "shots/tests/test_save"; g3.settingsPath = "shots/tests/test_settings.txt"; g3.guidePath = "shots/tests/test_guide.txt";
    bool ok3 = g3.loadSlot(2);
    int st0 = (int)g3.state;
    for (int i = 0; i < 30 * 8; i++) { g3.frame(in, 1.0 / 30); in.newFrame(); }
    printf("load slot 2 (mid-descent): %d state %d -> after 8 s state %d; autosave: '%s'\n", (int)ok3, st0, (int)g3.state, g3.saveSummary(g3.slotPath(0)).c_str());
    printf("flow done in %.1f s\n", nowSec() - t0);
}

// N0-02 (B-202): the diagnosis sheet. For every landable type: the landing map around the sub-solar point,
// the globe patch at the same point seen from 3.5 radii, and the ground there at noon looking down 30 deg.
// Prints the average colours and their RGB distances (0-441); N1 has to bring them under the threshold.
static void runConsistency() {
    SpaceRenderer sr; StarNeighborhood nb;
    Game game;
    game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/test_guide.txt";
    Input in;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (PLANET_TYPES[i].landable) remaining++; else done[i] = true;
    printf("%-14s %-13s %-13s %-13s %6s %6s %6s\n", "type", "map", "globe", "ground", "m-g", "g-s", "m-s");
    double worst = 0; int over = 0; const double THRESH = 60;
    for (int64_t x = 150; x < 320 && remaining; x++)
        for (int64_t z = 20; z < 140 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
                const Body& b = sys.bodies[bi];
                if (done[b.type]) continue;
                done[b.type] = true; remaining--;
                double t = 5000.0;
                Vec3 bp = sys.bodyPos(bi, t);
                Vec3 toStar = normalize(sys.star.pos - bp);
                Mat3 frame = sys.bodyFrame(bi, t);
                double lat, lon;
                StarSystem::latLonFromBody(frame * toStar, lat, lon);   // the sub-solar point: noon, sun overhead
                // the site: within 3 deg of that point, standing on the material that covers most of the globe patch
                const PlanetMap& pm = sr.mapFor(b);
                int want = MAT_ROCK;
                {
                    int hist[MAT_COUNT] = {0};
                    for (int yy = -12; yy < 12; yy++) for (int xx = -12; xx < 12; xx++) hist[pm.materialAt(lon + xx * TAU / PlanetMap::W, clampd(lat + yy * PI / PlanetMap::H, -PI / 2, PI / 2))]++;
                    for (int mm = 1; mm < MAT_COUNT; mm++) if (hist[mm] > hist[want]) want = mm;
                }
                {
                    BodyGen g0 = BodyGen::make(b);
                    bool found = false;
                    for (double r = 0; r < 3.0 * DEG && !found; r += 0.15 * DEG)
                        for (double a = 0; a < TAU && !found; a += 0.5) {
                            double la = lat + r * std::cos(a), lo = lon + r * std::sin(a);
                            SurfaceSample ss = sampleSurface(g0, StarSystem::bodyFromLatLon(la, lo), 16);
                            if (ss.material == want && (ss.water < -1e8 || ss.material == MAT_WATER)) { lat = la; lon = lo; found = true; }
                        }
                }
                // 1) the globe from 3.5 radii, straight above the point
                Vec3 shipPos = bp + toStar * (b.radiusKm * 3.5);
                Vec3 fwd = normalize(bp - shipPos);
                nb.update(shipPos);
                Framebuffer fbg; SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y)); c.bankBodyA = bi;
                warmMaps(sr, sys, bi);
                sr.setupPalette(fbg, &sys, bi, -1, 1.0); sr.render(fbg, c); fbg.mush(2);
                std::vector<uint32_t> globeRGB(FBW * FBH); fbg.toRGB(globeRGB.data(), 1.0);
                int wantBank = SpaceRenderer::globeBank(2, want);
                // the patch around the site's own point on the disc
                Vec3 siteW = sys.surfacePointWorld(bi, t, lat, lon, 0);
                Vec3 sv3 = c.cam * (siteW - shipPos);
                int gx = (int)(sr.proj.cx + sr.proj.f * sv3.x / sv3.z), gy = (int)(sr.proj.cy - sr.proj.f * sv3.y / sv3.z);
                // 2) the ground at that point, noon, looking down 30 deg
                SurfaceView sv;
                sv.init(&sys, bi, lat, lon, t);
                sv.player.yaw = 0.7; sv.player.pitch = -30 * DEG;
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                sv.update(0.016, in, t, false);
                Framebuffer fbs; sv.render(fbs, t, nb.stars, sr, 1.0); fbs.mush(2);
                std::vector<uint32_t> surfRGB(FBW * FBH); fbs.toRGB(surfRGB.data(), 1.0);
                // 3) the landing map of the same body, without the cloud overlay
                game.testParkAt(s, bi);
                int tx = (int)(wrap2pi(lon + PI) / TAU * PlanetMap::W), ty = (int)((0.5 - lat / PI) * PlanetMap::H);
                // averages and the sheet: 24 texels of map (x4), 24 px of globe (x4), 96 px of ground (x1), swatches
                const int T = 96, GAP = 4;
                const int SW = 4 * T + 3 * GAP + GAP + 3 * 24 + 2 * 2, SH = T;
                std::vector<uint32_t> sheet(SW * SH, rgb(0, 0, 0));
                auto put = [&](int x0, int y0, uint32_t c) { if (x0 >= 0 && y0 >= 0 && x0 < SW && y0 < SH) sheet[y0 * SW + x0] = c; };
                // averages over the wanted material only (map texels by material, globe pixels by bank), the sheet shows everything
                double avg[3][3] = {{0}};
                int nm = 0, ng = 0;
                for (int yy = 0; yy < 24; yy++)
                    for (int xx = 0; xx < 24; xx++) {
                        int mtx = ((tx - 12 + xx) % PlanetMap::W + PlanetMap::W) % PlanetMap::W, mty = clampi(ty - 12 + yy, 0, PlanetMap::H - 1);
                        uint32_t cm = game.testLandingMapTexel(mtx, mty, false);
                        int gxx = gx - 12 + xx, gyy = gy - 12 + yy;
                        bool gin = gxx >= 0 && gyy >= 0 && gxx < FBW && gyy < FBH;
                        uint32_t cg = gin ? globeRGB[gyy * FBW + gxx] : 0;
                        if (pm.material[mty * PlanetMap::W + mtx] == want) { for (int k = 0; k < 3; k++) avg[0][k] += (cm >> (8 * k)) & 255; nm++; }
                        if (gin && bankOf(fbg.idx[gyy * FBW + gxx]) == wantBank) { for (int k = 0; k < 3; k++) avg[1][k] += (cg >> (8 * k)) & 255; ng++; }
                        for (int dy = 0; dy < 4; dy++) for (int dx = 0; dx < 4; dx++) { put(xx * 4 + dx, yy * 4 + dy, cm); put(T + GAP + xx * 4 + dx, yy * 4 + dy, cg); }
                    }
                if (nm < 8 || ng < 8) {   // too little of the material in view: fall back to everything
                    nm = ng = 0; for (int k = 0; k < 3; k++) avg[0][k] = avg[1][k] = 0;
                    for (int yy = 0; yy < 24; yy++)
                        for (int xx = 0; xx < 24; xx++) {
                            uint32_t cm = game.testLandingMapTexel(tx - 12 + xx, ty - 12 + yy, false);
                            int gxx = gx - 12 + xx, gyy = gy - 12 + yy;
                            uint32_t cg = (gxx >= 0 && gyy >= 0 && gxx < FBW && gyy < FBH) ? globeRGB[gyy * FBW + gxx] : 0;
                            for (int k = 0; k < 3; k++) { avg[0][k] += (cm >> (8 * k)) & 255; avg[1][k] += (cg >> (8 * k)) & 255; }
                            nm++; ng++;
                        }
                }
                int sx0 = FBW / 2 - T / 2, sy0 = (int)(FBH * 0.72) - T / 2;
                double shadeSum = 0; int bankHist[BANKS] = {0};
                for (int yy = 0; yy < T; yy++)
                    for (int xx = 0; xx < T; xx++) {
                        int px = clampi(sx0 + xx, 0, FBW - 1), py = clampi(sy0 + yy, 0, FBH - 1);
                        uint32_t cs = surfRGB[py * FBW + px];
                        for (int k = 0; k < 3; k++) avg[2][k] += (cs >> (8 * k)) & 255;
                        shadeSum += shadeOf(fbs.idx[py * FBW + px]); bankHist[bankOf(fbs.idx[py * FBW + px])]++;
                        put(2 * (T + GAP) + xx, yy, cs);
                    }
                int groundBank = 0; for (int k = 1; k < BANKS; k++) if (bankHist[k] > bankHist[groundBank]) groundBank = k;
                for (int k = 0; k < 3; k++) { avg[0][k] /= nm; avg[1][k] /= ng; avg[2][k] /= (double)T * T; }
                auto dist = [&](int a, int b2) { double d = 0; for (int k = 0; k < 3; k++) d += (avg[a][k] - avg[b2][k]) * (avg[a][k] - avg[b2][k]); return std::sqrt(d); };
                for (int i = 0; i < 3; i++) {
                    uint32_t cc = rgb((int)avg[i][0], (int)avg[i][1], (int)avg[i][2]);
                    for (int yy = 0; yy < T; yy++) for (int xx = 0; xx < 24; xx++) put(3 * (T + GAP) + i * 26 + xx, yy, cc);
                }
                std::string tn = PLANET_TYPES[b.type].name;
                for (char& ch : tn) if (ch == ' ') ch = '_';
                std::string fn = "shots/tests/consistency_" + tn + ".png";
                writePNG(fn.c_str(), sheet.data(), SW, SH);
                double dmg = dist(0, 1), dgs = dist(1, 2), dms = dist(0, 2);
                worst = std::max(worst, std::max(dmg, std::max(dgs, dms)));
                if (b.type == PT_VENUSIAN) { if (dms > THRESH) over++; }   // the globe is its cloud deck; the map (C) and the ground are the same basalt
                else if (dmg > THRESH || dgs > THRESH || dms > THRESH) over++;
                // N1-06: the landing map's relief class against the slope measured on the real terrain
                const char* cls = game.testReliefClass(lat, lon);
                double slope = 0;   // O6-02: the regional grade of Game::siteSlope, on the ground's own caches
                for (int k = 0; k < 4; k++) {
                    double dx = (k & 1) ? 0 : (k ? -1 : 1), dz = (k & 1) ? (k == 1 ? 1 : -1) : 0;
                    slope += std::fabs(sv.site.groundHeight(dx * 150, dz * 150) - sv.site.groundHeight(0, 0)) / 150.0 / 8;
                    slope += std::fabs(sv.site.groundHeight(dx * 600, dz * 600) - sv.site.groundHeight(0, 0)) / 600.0 / 8;
                }
                slope /= 0.64;
                // the map probes points of the function, the ground reads its caches: the classes agree, or the two grades are within 30% (a boundary case)
                double mapSlope = Game::siteSlope(sv.site);
                bool reliefOk = std::string(cls) == reliefClass(slope) || std::fabs(mapSlope - slope) <= 0.3 * std::max(mapSlope, slope);
                printf("%-14s %3.0f,%3.0f,%3.0f   %3.0f,%3.0f,%3.0f   %3.0f,%3.0f,%3.0f   %6.0f %6.0f %6.0f  %s (%s, sun %.0f deg, light %.2f (globe %.2f), %s slope %.3f%s, %s)\n", PLANET_TYPES[b.type].name,
                       avg[0][0], avg[0][1], avg[0][2], avg[1][0], avg[1][1], avg[1][2], avg[2][0], avg[2][1], avg[2][2], dmg, dgs, dms,
                       b.name.c_str(), MATERIAL_NAMES[sv.site.sampleAt(0, 0, 64).material], sv.site.sun(t).altitude / DEG, sv.site.sun(t).lightFactor, SpaceRenderer::lightFactor(sys.star.luminosity, length(sys.star.pos - bp)),
                       cls, slope, reliefOk ? "" : " RELIEF MISMATCH", fn.c_str());
                printf("%-14s ground crop: mean shade %.1f, bank %d (%d%% of the crop); ramp stops 14/34/47 = %d,%d,%d / %d,%d,%d / %d,%d,%d\n", "", shadeSum / (T * T), groundBank, bankHist[groundBank] * 100 / (T * T),
                       fbs.pal[(groundBank * 64 + 14) * 3], fbs.pal[(groundBank * 64 + 14) * 3 + 1], fbs.pal[(groundBank * 64 + 14) * 3 + 2],
                       fbs.pal[(groundBank * 64 + 34) * 3], fbs.pal[(groundBank * 64 + 34) * 3 + 1], fbs.pal[(groundBank * 64 + 34) * 3 + 2],
                       fbs.pal[(groundBank * 64 + 47) * 3], fbs.pal[(groundBank * 64 + 47) * 3 + 1], fbs.pal[(groundBank * 64 + 47) * 3 + 2]);
                if (!reliefOk) over += 100;
            }
        }
    printf("consistency: %d of %d types over the threshold of %.0f (venusian: map against ground only); worst distance %.0f%s\n", over % 100, landableTypeCount(), THRESH, worst, over >= 100 ? "; RELIEF CLASS MISMATCH" : "");
}

static bool setupSceneForType(int type, double latDeg, double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut, int wantMat);

// N4-02: a drive on open grassland through the surface renderer: 60 s at full throttle, frames from the seat, airborne and chase
static int runDrive() {
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    // a desert (no trees, no rocks on sand) first, else open grassland
    bool haveSite = setupSceneForType(PT_FELISIAN, 10, 40 * DEG, 0.6, 0.0, sv, sys, nb, t, -10 - BIO_DESERT);
    const char* where = "desert";
    if (!haveSite) { haveSite = setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, sv, sys, nb, t, -9); where = "grassland"; }
    if (!haveSite) { printf("drive: no site\n"); return 1; }
    if (!sv.buggy.deployed) { sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2); if (!sv.deployBuggy()) { printf("drive: could not deploy\n"); return 1; } sv.buggy.unfold = 1; }
    double x, z, heading;
    double run = sv.findOpenRun(x, z, heading);
    sv.buggy.x = x; sv.buggy.z = z; sv.buggy.y = sv.site.surfaceHeight(x, z); sv.buggy.heading = heading; sv.buggy.speed = 0; sv.buggy.unfold = 1;
    sv.player.x = x + 1.5; sv.player.z = z; sv.player.y = sv.buggy.y;
    if (!sv.inBuggy && !sv.toggleBuggy()) { printf("drive: could not get in\n"); return 1; }
    printf("drive: %s site on %s, open run of %.0f m from (%.0f, %.0f) heading %.0f deg\n", where, sv.site.sys->bodies[sv.site.body].name.c_str(), run, x, z, heading / DEG);
    Input in; in.down[KEY_W] = true;
    Framebuffer fb;
    double top = 0, worstMs = 0, sumMs = 0; int n = 0; bool airShot = false, stalled = false;
    double stuckT = 0, reverseT = 0; int steerDir = 1;
    double runX = x, runZ = z, runH = heading, runLen = run; int dir = 1;   // along the run (+1) or back (-1)
    double turnT = 0;
    std::vector<SurfaceView::Collider> cols;
    const int FRAMES = 30 * 90;
    for (int i = 0; i < FRAMES; i++) {
        double tt = t + i / 30.0;
        sv.update(1.0 / 30, in, tt, true);
        in.newFrame();
        in.down[KEY_W] = in.down[KEY_S] = in.down[KEY_A] = in.down[KEY_D] = in.down[KEY_SPACE] = false;
        double hx = std::sin(sv.buggy.heading), hz = std::cos(sv.buggy.heading);
        double along = (sv.buggy.x - runX) * std::sin(runH) + (sv.buggy.z - runZ) * std::cos(runH);   // position along the run
        auto aimAt = [&](double a) { double tx = runX + std::sin(runH) * a, tz = runZ + std::cos(runH) * a; return wrapAngle(std::atan2(tx - sv.buggy.x, tz - sv.buggy.z) - sv.buggy.heading); };
        double want = dir > 0 ? runH : runH + PI;
        double err = aimAt(clampd(along + dir * 40, 0, runLen));
        // U-turn near either end of the run: brake, then swing round until the heading points back
        if (turnT <= 0 && ((dir > 0 && along > runLen - 45) || (dir < 0 && along < 45))) { dir = -dir; turnT = 6.0; }
        if (turnT > 0) {
            turnT -= 1.0 / 30;
            want = dir > 0 ? runH : runH + PI; err = wrapAngle(want - sv.buggy.heading);
            if (std::fabs(sv.buggy.speed) > 6) in.down[KEY_SPACE] = true; else in.down[KEY_W] = true;
            in.down[err > 0 ? KEY_D : KEY_A] = true;
            if (std::fabs(err) < 15 * DEG) turnT = 0;
        } else {
            // a driver's eye: steer round rocks, trunks and water in the corridor ahead, back out when stuck, else hold the run
            double look = 14 + std::fabs(sv.buggy.speed) * 1.4;
            int steer = 0; double nearest = 1e9;
            for (double d = 8; d <= look; d += 12) {
                sv.collectColliders(sv.buggy.x + hx * d, sv.buggy.z + hz * d, cols);
                for (const SurfaceView::Collider& c : cols) {
                    if (c.kind == 0 && c.r < 0.72) continue;
                    double dx = c.x - sv.buggy.x, dz = c.z - sv.buggy.z;
                    double al = dx * hx + dz * hz, ac = -dx * hz + dz * hx;
                    if (al < 0 || al > look || std::fabs(ac) > 2.8 + c.r) continue;
                    if (al < nearest) { nearest = al; steer = ac > 0 ? -1 : 1; }
                }
            }
            for (double d = 10; d <= look && !steer; d += 5) if (sv.site.groundHeight(sv.buggy.x + hx * d, sv.buggy.z + hz * d) < sv.site.waterAt(sv.buggy.x + hx * d, sv.buggy.z + hz * d) - 0.3) steer = steerDir;
            if (std::fabs(sv.buggy.speed) < 0.5 && i > 30) stuckT += 1.0 / 30; else stuckT = 0;
            if (stuckT > 1.0 && reverseT <= 0) { reverseT = 1.5; steerDir = -steerDir; }
            if (reverseT > 0) { reverseT -= 1.0 / 30; in.down[KEY_S] = true; in.down[steerDir > 0 ? KEY_D : KEY_A] = true; }
            else {
                in.down[KEY_W] = true;
                if (steer > 0) in.down[KEY_D] = true; else if (steer < 0) in.down[KEY_A] = true;
                else if (std::fabs(err) > 4 * DEG) in.down[err > 0 ? KEY_D : KEY_A] = true;   // hold the run's line
                if (nearest < 12 && std::fabs(sv.buggy.speed) > 14) in.down[KEY_W] = false;   // ease off before something close
            }
        }
        top = std::max(top, std::fabs(sv.buggy.speed));
        if (i % 5 == 0 || i == 30 * 12 || i == 30 * 40) {
            double t0 = nowSec();
            sv.render(fb, tt, nb.stars, sr, 1.0);
            double ms = (nowSec() - t0) * 1000; sumMs += ms; worstMs = std::max(worstMs, ms); n++;
            if (i == 30 * 12) { fb.mush(2); sv.cameraFeed(fb, tt); saveFB(fb, "shots/tests/drive_seat.png"); }
            if (!airShot && sv.buggy.airborne) { fb.mush(2); sv.cameraFeed(fb, tt); saveFB(fb, "shots/tests/drive_air.png"); airShot = true; }
            if (i == 30 * 40) { sv.chaseCam = true; sv.render(fb, tt, nb.stars, sr, 1.0); fb.mush(2); saveFB(fb, "shots/tests/drive_chase.png"); sv.chaseCam = false; }
        }
        if (i % 300 == 299) printf("  %2d s: %5.1f m/s, %6.0f m, gear %d, skid %.2f, %.0f m along the run, dir %+d%s\n", (i + 1) / 30, sv.buggy.speed, sv.buggy.odometer, sv.buggy.gear, sv.buggy.skid, along, dir, sv.buggy.airborne ? ", airborne" : "");
        if (i > 30 * 20 && std::fabs(sv.buggy.speed) < 0.5 && turnT <= 0) stalled = true;
    }
    {   // a night frame with the headlights on
        double tn = t + std::fabs(sys.bodies[sv.site.body].rotPeriod) * 0.5;
        Input in2; sv.update(1.0 / 30, in2, tn, true);
        sv.render(fb, tn, nb.stars, sr, 1.0); fb.mush(2); sv.cameraFeed(fb, tn); saveFB(fb, "shots/tests/drive_night.png");
        sv.chaseCam = true; sv.render(fb, tn, nb.stars, sr, 1.0); fb.mush(2); saveFB(fb, "shots/tests/drive_night_chase.png"); sv.chaseCam = false;
    }
    printf("drive: %.0f m in 90 s, top %.0f km/h, render %.1f ms mean / %.1f worst at 1x%s%s; frames drive_seat, drive_chase, drive_night[_chase]\n", sv.buggy.odometer, top * 3.6, sumMs / std::max(1, n), worstMs, airShot ? ", airborne once (shots/drive_air.png)" : "", stalled ? " (STALLED at some point)" : "");
    bool ok = top * 3.6 > 100 && sv.buggy.odometer > 2000;
    printf("drive: %s\n", ok ? "ok (over 2 km, over 100 km/h)" : "FAIL: under 2 km or under 100 km/h");
    return ok ? 0 : 1;
}

// N3-01: the bestiary of the first living worlds near the start, and a determinism check
static int runBestiary() {
    int shown = 0, fails = 0;
    for (int64_t x = 150; x < 320 && shown < 6; x++)
        for (int64_t z = 20; z < 140 && shown < 6; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && shown < 6; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN && b.type != PT_OCEAN) continue;
                BodyGen g = BodyGen::make(b);
                Bestiary B = Bestiary::make(b, g), B2 = Bestiary::make(b, g);
                printf("%s (%s): %d land, %d flyers, %d swimmers\n", b.name.c_str(), PLANET_TYPES[b.type].name, B.landCount, B.flyerCount, B.swimmerCount);
                for (size_t i = 0; i < B.species.size(); i++) {
                    const Species& sp = B.species[i];
                    printf("  %-12s %-20s %4.1f m  gait %d/%d  bank %2d tone %.2f pattern %d crest %d  habitat %-16s diet %d  %-8s %-11s call %d x%.2f  herd %d-%d\n", sp.name.c_str(), PLAN_NAMES[sp.plan], sp.size, sp.gait, sp.runGait,
                           sp.bank, sp.tone, sp.pattern, sp.crest, sp.habitat ? BIOME_NAMES[sp.habitat] : "ANY", sp.diet, ACTIVITY_NAMES[sp.activity], TEMPERAMENT_NAMES[sp.temperament], sp.call, sp.callPitch, sp.herdMin, sp.herdMax);
                    if (sp.name != B2.species[i].name || sp.size != B2.species[i].size || sp.plan != B2.species[i].plan) fails++;
                }
                if (B.species.empty()) fails++;
                shown++;
            }
        }
    printf("bestiary: %d worlds, %s\n", shown, fails ? "NOT DETERMINISTIC (FAIL)" : "deterministic");
    return fails ? 1 : 0;
}

// N3-04: encounter density over random felisian sites: the share with a herd within 200 m, per biome
static void runEncounters(int wanted) {
    setDrainageEnabled(false);   // O6-03: hundreds of sites over the globe, a tile each; the herds do not need the rivers
    int total = 0, withLife = 0, cellHerds = 0, cellAnimals = 0;
    int perBiome[BIO_COUNT] = {0}, lifeBiome[BIO_COUNT] = {0};
    Rng r(0xE7C0);
    for (int64_t x = 150; x < 320 && total < wanted; x++)
        for (int64_t z = 20; z < 140 && total < wanted; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && total < wanted; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN) continue;
                for (int k = 0; k < 6 && total < wanted; k++) {
                    double lat = r.range(-70 * DEG, 70 * DEG), lon = r.range(-PI, PI);
                    SurfaceSite site; site.init(&sys, bi, lat, lon, 5000.0);
                    TerrainVertex tv = site.sampleAt(0, 0, 16);
                    if (tv.material == MAT_WATER) continue;   // sea landings are not in the design
                    Bestiary B = Bestiary::make(b, site.gen);
                    std::vector<Critter> cr; std::vector<Flock> fl; std::vector<SurfaceView::Herd> hs;
                    SurfaceView::planLife(site, B, site.gen.seed ^ (uint64_t)(lat * 1e6) ^ ((uint64_t)(lon * 1e6) << 20), cr, fl, hs);
                    bool near = false;
                    for (const Critter& c : cr) if (c.x * c.x + c.z * c.z < 200.0 * 200.0) near = true;
                    total++; perBiome[tv.biome]++;
                    if (near) { withLife++; lifeBiome[tv.biome]++; }
                    {   // B-316: the habitat cells within 900 m of the landing
                        int gLat0, gLon0; SurfaceView::lifeCellAt(site, 0, 0, gLat0, gLon0);
                        for (int dl = -2; dl <= 2; dl++)
                            for (int dn = -3; dn <= 3; dn++) {
                                SurfaceView::Herd hd; int spi, n;
                                if (!SurfaceView::planCellHerd(site, B, gLat0 + dl, gLon0 + dn, hd, spi, n)) continue;
                                if (hd.cx * hd.cx + hd.cz * hd.cz > 900.0 * 900.0) continue;
                                cellHerds++; cellAnimals += n;
                            }
                    }
                }
            }
        }
    printf("encounters: a herd within 200 m at %d of %d felisian land sites (%.0f%%); B-316: %.1f habitat herds (%.0f animals) within 900 m per site\n", withLife, total, total ? 100.0 * withLife / total : 0.0, total ? (double)cellHerds / total : 0.0, total ? (double)cellAnimals / total : 0.0);
    for (int bm = 0; bm < BIO_COUNT; bm++) if (perBiome[bm]) printf("  %-16s %3d sites, %3.0f%%\n", BIOME_NAMES[bm], perBiome[bm], 100.0 * lifeBiome[bm] / perBiome[bm]);
}

// One landing-map screen per landable planet type (bug B-004 check).
static void renderLandingMaps() {
    Game game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";   // every harness Game keeps off the real guide.txt (2026-09-27)
    applyTestScale(game);
    Input in;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (PLANET_TYPES[i].landable) remaining++; else done[i] = true;
    for (int64_t x = 150; x < 320 && remaining; x++)
        for (int64_t z = 20; z < 140 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
                const Body& b = sys.bodies[bi];
                if (done[b.type]) continue;
                done[b.type] = true; remaining--;
                game.testParkAt(s, bi);
                for (int f = 0; f < 3; f++) { game.frame(in, 1.0 / 30); in.newFrame(); }
                std::string tn = PLANET_TYPES[b.type].name;
                for (char& ch : tn) if (ch == ' ') ch = '_';
                std::string fn = "shots/tests/landmap_" + tn + ".png";
                writePNG(fn.c_str(), game.output(), FBW, FBH);
                printf("landing map %-16s %s state=%d, darkest shadow on the map now %.2f\n", PLANET_TYPES[b.type].name, b.name.c_str(), (int)game.state, game.testMapShadowMax());   // O0-04
            }
        }
}

// Settings file round trip and the settings screen (M0-02).
static void testSettings() {
    Settings s;
    s.mouseSensitivity = 1.7; s.invertY = true; s.fovDeg = 85; s.masterVolume = 0.35; s.scanlines = false; s.windowScale = 3;
    s.save("shots/tests/test_settings.txt");
    Settings r;
    bool ok = r.load("shots/tests/test_settings.txt");
    bool same = ok && std::fabs(r.mouseSensitivity - 1.7) < 1e-9 && r.invertY && r.fovDeg == 85 && std::fabs(r.masterVolume - 0.35) < 1e-9 && !r.scanlines && r.windowScale == 3;
    printf("settings round trip: %s\n", same ? "ok" : "MISMATCH");
    Game game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";   // every harness Game keeps off the real guide.txt (2026-09-27)
    game.loadFromDisk();
    printf("loaded into game: fov=%.0f sens=%.1f scale=%d\n", game.settings.fovDeg, game.settings.mouseSensitivity, game.settings.windowScale);
    Input in;
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; };
    press(KEY_ENTER); press(KEY_ESCAPE);
    press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER);
    press(KEY_DOWN); press(KEY_DOWN); press(KEY_RIGHT);   // field of view +5
    writePNG("shots/tests/settings.png", game.output(), FBW, FBH);
    press(KEY_ESCAPE);
    Settings r2; r2.load("shots/tests/test_settings.txt");
    printf("after +5 on the screen: fov=%.0f (file %.0f) state=%d\n", game.settings.fovDeg, r2.fovDeg, (int)game.state);
    Settings d;
    d.save("shots/tests/test_settings.txt");   // leave defaults for the other tests
}

// ---- determinism regression (M0-03) ----------------------------------------------
// Hashes generated content that must not change by accident: the first systems met in the
// start region, ten planet maps, and height profiles at three sites. `regress bless`
// rewrites tests/regress_baseline.txt; `regress` compares against it (exit code 1 on change).
static uint64_t fnv(uint64_t h, const void* p, size_t n) {
    const uint8_t* b = (const uint8_t*)p;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
    return h;
}
static void appendFrameHashes(std::vector<std::pair<std::string, uint64_t>>& lines, std::vector<std::string>& notes);
static int runRegress(bool bless) {
    const uint64_t FNV0 = 1469598103934665603ULL;
    std::vector<std::pair<std::string, uint64_t>> lines;
    std::vector<std::string> notes;
    int nsys = 0, nmap = 0, nsite = 0;
    for (int64_t x = 150; x < 260 && (nsys < 5 || nmap < 10 || nsite < 3); x++)
        for (int64_t z = 20; z < 120 && (nsys < 5 || nmap < 10 || nsite < 3); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            if (nsys < 5) {
                uint64_t h = FNV0;
                h = fnv(h, s.name.data(), s.name.size()); h = fnv(h, &s.cls, sizeof s.cls); h = fnv(h, &s.radiusKm, 8); h = fnv(h, &s.luminosity, 8);
                for (auto& b : sys.bodies) {
                    h = fnv(h, b.name.data(), b.name.size()); h = fnv(h, &b.type, sizeof b.type); h = fnv(h, &b.radiusKm, 8);
                    h = fnv(h, &b.orbitRadiusKm, 8); h = fnv(h, &b.orbitPeriod, 8); h = fnv(h, &b.rotPeriod, 8); h = fnv(h, &b.axialTilt, 8);
                }
                lines.push_back({"system_" + std::to_string(nsys), h});
                notes.push_back(s.name + " (" + std::to_string(sys.bodies.size()) + " bodies)");
                nsys++;
            }
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (nmap < 10) {
                    PlanetMap m; m.generate(BodyGen::make(b));
                    uint64_t h = FNV0;
                    h = fnv(h, m.height.data(), m.height.size() * sizeof(float));
                    h = fnv(h, m.material.data(), m.material.size()); h = fnv(h, m.albedo.data(), m.albedo.size());
                    h = fnv(h, m.cloud.data(), m.cloud.size()); h = fnv(h, m.veg.data(), m.veg.size());
                    lines.push_back({"map_" + std::to_string(nmap), h});
                    notes.push_back(b.name + " " + PLANET_TYPES[b.type].name);
                    nmap++;
                }
                if (nsite < 3 && PLANET_TYPES[b.type].landable) {
                    SurfaceSite site;
                    site.init(&sys, bi, 0.3, 0.7, 0.0);
                    uint64_t h = FNV0;
                    for (int i = 0; i <= 80; i++) {
                        TerrainVertex v = site.sampleAt(-2000 + i * 50.0, 0.0, 16);
                        h = fnv(h, &v.h, 4); h = fnv(h, &v.albedo, 4); h = fnv(h, &v.material, 1);
                        TerrainVertex w = site.sampleAt(0.0, -2000 + i * 50.0, 16);
                        h = fnv(h, &w.h, 4); h = fnv(h, &w.albedo, 4); h = fnv(h, &w.material, 1);
                    }
                    lines.push_back({"site_" + std::to_string(nsite), h});
                    notes.push_back(b.name + " profile at 0.3 rad N 0.7 rad E");
                    nsite++;
                }
            }
        }
    appendFrameHashes(lines, notes);
    const char* path = "tests/regress_baseline.txt";
    if (bless) {
        std::ofstream f(path);
        f << "# vesperis determinism baseline; regenerate with ./vesperis_test regress bless\n";
        for (size_t i = 0; i < lines.size(); i++) f << lines[i].first << " " << std::hex << lines[i].second << std::dec << "  # " << notes[i] << "\n";
        printf("blessed %zu hashes into %s\n", lines.size(), path);
        return 0;
    }
    std::ifstream f(path);
    if (!f) { printf("no baseline at %s: run ./vesperis_test regress bless\n", path); return 1; }
    std::vector<std::pair<std::string, uint64_t>> base;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::string key; uint64_t v = 0;
        std::istringstream is(line);
        is >> key >> std::hex >> v;
        base.push_back({key, v});
    }
    int changed = 0, missing = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        const uint64_t* bv = nullptr;
        for (auto& b : base) if (b.first == lines[i].first) bv = &b.second;
        if (!bv) { printf("  %-10s new (no baseline)  %s\n", lines[i].first.c_str(), notes[i].c_str()); missing++; }
        else if (*bv != lines[i].second) { printf("  %-10s CHANGED %016llx -> %016llx  %s\n", lines[i].first.c_str(), (unsigned long long)*bv, (unsigned long long)lines[i].second, notes[i].c_str()); changed++; }
    }
    printf("regress: %zu hashes, %d changed, %d without baseline -> %s\n", lines.size(), changed, missing, (changed || missing) ? "FAIL" : "PASS");
    return (changed || missing) ? 1 : 0;
}

// ---- look comparison across resolution scales (M10-02) ---------------------------------
static bool setupPinnedMountainScene(double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut);
static bool setupSceneForType(int type, double latDeg, double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys,
                              StarNeighborhood& nb, double& tOut, int wantMat = -1) {
    if (wantMat == -20) return setupPinnedMountainScene(alt, yawOff, pitch, sv, sys, nb, tOut);   // B-313: the pinned mountain site of the O6 review
    for (int64_t x = 150; x < 320; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            sys.generate(s);
            if (wantMat <= -40 && wantMat >= -42 && starActivity(sys.star.cls) < 0.7) continue;   // B-403: an active star (S-02: the activity table, so every class that drives the nights counts)
            if (wantMat == -43 && sys.star.cls != STAR_NEUTRON) continue;   // S-03: a glassed world belongs to a neutron star
            if (wantMat == -44 && sys.star.cls != STAR_PROTOSTAR) continue;   // S-04: a world of a protostar
            if (wantMat == -45 && sys.star.cls != STAR_WOLF_RAYET) continue;   // S-05: a world of a Wolf-Rayet star
            if (wantMat == -46 && sys.star.cls != STAR_BLACK_HOLE) continue;   // S-06: a world of a black hole
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (type >= 0 && b.type != type) continue;
                if (type < 0 && (!PLANET_TYPES[b.type].landable || b.type == PT_COMET)) continue;   // R-307: a trait scene on any world
                double lon = 0.7 + bi * 0.3, latUse = latDeg * DEG;
                if (wantMat == -41) latUse = SurfaceView::auroraOvalLat(b) * DEG;   // B-403: right under the auroral oval
                if (wantMat == -42) latUse = (SurfaceView::auroraOvalLat(b) - 1) * DEG;   // R-402: the main curtain a degree poleward: overhead and across the sky
                BodyGen g = BodyGen::make(b);
                if (wantMat == -36 && !g.hasTrait(TR_GEYSERS)) continue;
                if ((wantMat == -47 || wantMat == -48 || wantMat == -49) && !g.hasTrait(TR_CIVILISATION)) continue;   // C-01: a world that had a people
                // O6-03: the scans below sample a whole planet at 16 m: without the drainage (a tile per sample); the river and lake
                // finders (-21, -22) scan the flood's own cells instead (`drainageStats`) and turn it back on for the fine samples
                struct DrainOff { bool was; DrainOff() : was(drainageEnabled()) { setDrainageEnabled(false); } ~DrainOff() { setDrainageEnabled(was); } } drainOff;
                bool lookAtWater = false;
                double faceYawOut = 1e9, facePitchOut = 1e9;   // R-307: a scene finder may choose the view's direction
                double t0Use = 1000.0;   // R-402: the aurora scenes choose the night of the month with the strongest potential
                bool placed = false;     // G-04: the finder chose the spot (or the longitude matters): the light must come from the time alone
                if (wantMat == MAT_SAND) {
                    // a beach: sand near the sea level, the view turned toward the water
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.5)
                        for (int k = 0; k < 720 && !found; k++) {
                            double ln = k * TAU / 720;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material != MAT_SAND || ss.height < 0.5 || ss.height > 4) continue;
                            // enough water within 150 m in some direction
                            int best = 0;
                            for (int a = 0; a < 16; a++) {
                                int cnt = 0;
                                for (int dd = 20; dd <= 150; dd += 10) {
                                    double ang = a * TAU / 16;
                                    double dlat = std::cos(ang) * dd / (b.radiusKm * 1000.0), dlon = std::sin(ang) * dd / (b.radiusKm * 1000.0 * std::cos(la * DEG));
                                    if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16).material == MAT_WATER) cnt++;
                                }
                                best = std::max(best, cnt);
                            }
                            if (best >= 6) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                    lookAtWater = true; placed = true;
                } else if (wantMat == -36) {
                    // R-307: a geyser basin: the first sinter sample of the scan, then the nearest vent of `geyserVents` that stands on
                    // sinter; the site 150 m from it, facing it
                    bool found = false;
                    double Rm = b.radiusKm * 1000.0;
                    for (double la = latDeg; la < latDeg + 50 && !found; la += 1.0)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            Vec3 u = StarSystem::bodyFromLatLon(la * DEG, ln);
                            if (sampleSurface(g, u, 16).material != MAT_SALT) continue;
                            std::vector<GeyserVent> vents; geyserVents(g, u, vents);
                            for (const GeyserVent& v : vents) {
                                if (sampleSurface(g, v.unit, 16).material != MAT_SALT) continue;
                                double vla, vlo; StarSystem::latLonFromBody(v.unit, vla, vlo);
                                double dlat = 150.0 / Rm;   // stand 150 m south of the vent, looking north
                                latUse = vla - dlat; lon = vlo; faceYawOut = 0.0; found = true;
                                break;
                            }
                        }
                    if (!found) continue;
                    placed = true;
                } else if (wantMat <= -40 && wantMat >= -42) {
                    // B-403: the aurora scenes: any site of the type at the latitude asked (the star was chosen above); -42 wants a
                    // clear-air world with another body of the system (a moon, the parent) at least 0.6 degrees across and 35-65
                    // degrees up at the darkest hour (the main curtain hangs overhead there), the biggest of them, and faces it
                    if (!PLANET_TYPES[b.type].atmosphere || hasOpaqueDeck(b.type)) continue;
                    {   // R-402: a night with curtains: of the next thirty nights, the darkest hour with the strongest potential, over 0.45
                        SurfaceSite probe; probe.init(&sys, bi, latUse, lon, 1000.0);
                        double bestPot = 0, period = std::fabs(b.rotPeriod);
                        for (int dd = 0; dd < 30; dd++) {
                            double t0 = 1000.0 + dd * period, pot = auroraPotential(sys, b, findTime(probe, alt, t0));
                            if (pot > bestPot) { bestPot = pot; t0Use = t0; }
                        }
                        if (bestPot < 0.45) continue;
                    }
                    if (wantMat == -42) {
                        SurfaceSite probe; probe.init(&sys, bi, latUse, lon, t0Use);
                        double tp = findTime(probe, alt, t0Use);
                        Vec3 obs = probe.worldPos(tp, 0, 0, 0);
                        Mat3 L = probe.localFrame(tp);
                        double bestR = 0; int bestJ = -1; Vec3 bestDl;
                        for (int bj = 0; bj < (int)sys.bodies.size(); bj++) {
                            if (bj == bi || sys.bodies[bj].type == PT_COMPANION) continue;
                            Vec3 dW = sys.bodyPos(bj, tp) - obs;
                            double dist = std::max(length(dW), 1.0), ang = sys.bodies[bj].radiusKm / dist;
                            Vec3 dl = L * (dW / dist);
                            double hl = std::sqrt(std::max(1e-9, 1 - dl.y * dl.y));
                            (void)hl;
                            if (ang < 0.005 || dl.y < 0.57 || dl.y > 0.9 || ang < bestR) continue;
                            bestR = ang; bestJ = bj; bestDl = dl;
                        }
                        if (bestJ < 0) continue;
                        // the curtains must be in front of it: a trial frame, counting aurora pixels within 1.6 radii of its centre
                        double yaw = std::atan2(bestDl.x, bestDl.z), pitchB = std::asin(bestDl.y) - 0.08;
                        sv.init(&sys, bi, latUse, lon, tp);
                        nb.update(sv.site.worldPos(tp, 0, 0, 0));
                        sv.player.yaw = yaw; sv.player.pitch = pitchB;
                        Input in; sv.update(0.016, in, tp, false);
                        SpaceRenderer srTrial; Framebuffer fb; sv.render(fb, tp, nb.stars, srTrial, 1.0);
                        double sx, sy; int over = 0, disc = 0;
                        if (sv.projectPoint(sv.player.x + bestDl.x * 1e5, sv.player.y + 1.65 + bestDl.y * 1e5, sv.player.z + bestDl.z * 1e5, sx, sy)) {
                            double rpx = 1.6 * bestR * sv.proj.f;
                            for (int y = std::max(0, (int)(sy - rpx)); y < std::min(FBH, (int)(sy + rpx) + 1); y++)
                                for (int x = std::max(0, (int)(sx - rpx)); x < std::min(FBW, (int)(sx + rpx) + 1); x++) {
                                    if ((x - sx) * (x - sx) + (y - sy) * (y - sy) > rpx * rpx) continue;
                                    disc++; if (bankOf(fb.idx[y * FBW + x]) == 11) over++;
                                }
                        }
                        printf("  %s: %s in the sky, %.1f deg across, %.0f deg up, %.0f deg from the pole, aurora over %d%% of its surroundings\n", b.name.c_str(), PLANET_TYPES[sys.bodies[bestJ].type].name, 2 * bestR / DEG, std::asin(bestDl.y) / DEG,
                               std::acos(clampd(bestDl.z * (latUse >= 0 ? 1 : -1) / std::sqrt(std::max(1e-9, 1 - bestDl.y * bestDl.y)), -1, 1)) / DEG, disc ? 100 * over / disc : 0);
                        if (disc == 0 || over < disc / 12) continue;
                        faceYawOut = yaw; facePitchOut = pitchB; placed = true;
                    }
                } else if (wantMat <= -30 && wantMat != -43 && wantMat != -44 && wantMat != -45 && wantMat != -46 && wantMat != -47 && wantMat != -48 && wantMat != -49) {   // S-03: -43 (the glassed world) has its own branch below; S-04/S-05/S-06: -44 to -46 take the type's default spot
                    // R-307: the new types' scenes: -30 a stained crack (europan), -31 a lava fissure (tectonic), -32 dunes
                    // (desert), -33 / -35 the shore of a methane or an acid sea, -34 a rayed plain (bombarded)
                    int mat = wantMat == -30 ? MAT_DUST : (wantMat == -31 ? MAT_LAVA : (wantMat == -32 ? MAT_SAND : MAT_WATER));
                    bool nearOnly = wantMat == -33 || wantMat == -35 || wantMat == -31;   // stand 60-150 m from it
                    bool found = false;
                    double faceYaw = 1e9;
                    double Rm = b.radiusKm * 1000.0;
                    for (double la = latDeg; la < latDeg + 40 && !found; la += 1.0)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (wantMat == -34) { if (ss.height > 0 && ss.albedo > 0.3) { lon = ln; latUse = la * DEG; found = true; } continue; }
                            if (ss.material != mat) continue;
                            if (wantMat == -30) {   // a narrow linea: ice 120 m out on both sides in some direction; stand on it 70 m off, facing the stain
                                for (int a = 0; a < 8 && !found; a++) {
                                    double ang = a * TAU / 16;   // eight directions over a half turn: the pair is (ang, ang + pi)
                                    double dlat = std::cos(ang) * 120.0 / Rm, dlon = std::sin(ang) * 120.0 / (Rm * std::cos(la * DEG));
                                    SurfaceSample sA = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                                    SurfaceSample sB = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG - dlat, ln - dlon), 16);
                                    if (sA.material != MAT_ICE || sB.material != MAT_ICE) continue;
                                    lon = ln + dlon * 70.0 / 120.0; latUse = la * DEG + dlat * 70.0 / 120.0; found = true;
                                    faceYaw = std::atan2(-dlon * Rm * std::cos(la * DEG), -dlat * Rm);   // toward the stain (east = x, north = z)
                                }
                                continue;
                            }
                            if (!nearOnly) { lon = ln; latUse = la * DEG; found = true; continue; }
                            for (int a = 0; a < 8 && !found; a++) {   // the nearest firm ground 60-150 m out
                                for (double dd = 60; dd <= 150 && !found; dd += 30) {
                                    double ang = a * TAU / 8;
                                    double dlat = std::cos(ang) * dd / Rm, dlon = std::sin(ang) * dd / (Rm * std::cos(la * DEG));
                                    SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                                    if (s2.material == mat || (s2.water > -1e8 && s2.height < s2.water + 0.3)) continue;
                                    lon = ln + dlon; latUse = la * DEG + dlat; found = true;
                                }
                            }
                        }
                    if (!found) continue;
                    lookAtWater = wantMat == -33 || wantMat == -35; placed = true;
                    if (faceYaw < 1e8) faceYawOut = faceYaw;
                } else if (wantMat == -43) {
                    // S-03: a glassed world: the first glass sheet of the scan at 16 m with glass on the eight samples 100 m round it
                    if (!g.hasTrait(TR_GLASSED)) continue;
                    bool found = false; double Rm = b.radiusKm * 1000.0;
                    for (double la = latDeg; la < latDeg + 40 && !found; la += 1.0)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16).material != MAT_GLASS) continue;
                            int glassy = 0;
                            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + j * 100.0 / Rm, ln + i * 100.0 / (Rm * std::cos(la * DEG))), 16).material == MAT_GLASS) glassy++;
                            if (glassy >= 9) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                    placed = true;
                } else if (wantMat <= -10 && wantMat > -21) {
                    // N2: a site of one biome (wantMat = -10 - biome), on land (S-04: the codes under -21 are the other finders', and
                    // -44 wants the type's default spot: this branch swallowed it and found no biome 34)
                    int wantBio = -10 - wantMat;
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 34 && !found; la += 1.5)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.biome == wantBio && ss.material != MAT_WATER && ss.water < -1e8) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                    placed = true;
                } else if (wantMat == -21 || wantMat == -22) {
                    // B-320: inland water: the first river (-21) or lake (-22) of the scan (`SurfaceSample::waterKind`), then
                    // the nearest dry land round it, 20-300 m out; the site stands there facing the water
                    bool found = false;
                    double Rm = b.radiusKm * 1000.0;
                    int wantKind = wantMat == -21 ? 1 : 2;
                    int hits = 0, noLand = 0;
                    // O6-03: a land window first (coarse, no drainage), then its tile's flood: the window is scanned at 16 m only
                    // where the flood has channels (or lakes), the drainage on
                    double wla = 1e9, wln = 0;
                    for (double la = latDeg; la < latDeg + 40 && wla > 1e8; la += 1.0)
                        for (int k = 0; k < 360 && wla > 1e8; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512);
                            if (ss.material == MAT_WATER || ss.height < 20 || ss.biome == BIO_ICE || ss.biome == BIO_DESERT) continue;
                            setDrainageEnabled(true);
                            DrainStats ds = drainageStats(g, StarSystem::bodyFromLatLon(la * DEG, ln), 12);
                            setDrainageEnabled(false);
                            if ((wantKind == 1 && ds.channels >= 6) || (wantKind == 2 && ds.lakes >= 12)) { wla = la; wln = ln; }
                        }
                    if (wla > 1e8) continue;
                    setDrainageEnabled(true);
                    double span = 12 * 400.0 / Rm / DEG;   // the window's half-size in degrees
                    for (double la = wla - span; la < wla + span && !found; la += span / 40)
                        for (double ln = wln - span / std::cos(wla * DEG); ln < wln + span / std::cos(wla * DEG) && !found; ln += span / std::cos(wla * DEG) / 40) {
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material != MAT_WATER || ss.waterKind != wantKind) continue;
                            hits++;
                            double bestD = 1e9, bla = 0, bln = 0;
                            for (int a = 0; a < 16; a++)
                                for (double dd = 20; dd <= 300 && dd < bestD; dd += 20) {
                                    double ang = a * TAU / 16;
                                    double dlat = std::cos(ang) * dd / Rm, dlon = std::sin(ang) * dd / (Rm * std::cos(la * DEG));
                                    SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                                    if (s2.material == MAT_WATER || (s2.water > -1e8 && s2.height < s2.water + 0.3)) continue;
                                    bestD = dd; bla = la * DEG + dlat; bln = ln + dlon; break;
                                }
                            if (bestD > 1e8) { noLand++; continue; }
                            lon = bln; latUse = bla; found = true;
                        }
                    if (hits && !found) printf("  water finder: %s: %d hits of kind %d, none with land within 300 m\n", b.name.c_str(), hits, wantKind);
                    (void)noLand;
                    setDrainageEnabled(false);
                    if (!found) continue;
                    lookAtWater = true; placed = true;
                } else if (wantMat == -3 || wantMat == -5 || wantMat == -6 || wantMat == -8 || wantMat == -9) {
                    // N3/N4: open grassland, so a herd or the buggy can be seen
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 30 && !found; la += 1.5)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_GRASS && (ss.biome == BIO_GRASSLAND || ss.biome == BIO_SAVANNA) && ss.height > 2) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                    placed = true;
                } else if (wantMat == -4) {
                    // N2: deep forest (dense vegetation), for the coverage and the bench
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 30 && !found; la += 1.5)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material != MAT_FOREST || ss.veg <= 0.75 || ss.height <= 5) continue;
                            // G-04: the eight samples 100 m round it as well: the scan's first dense cell stood at a wood's edge (a fifth of the ground under canopy)
                            int dense = 0; double Rm = b.radiusKm * 1000.0;
                            for (int j = -1; j <= 1; j++)
                                for (int i = -1; i <= 1; i++) {
                                    if (!i && !j) continue;
                                    SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + j * 100.0 / Rm, ln + i * 100.0 / (Rm * std::cos(la * DEG))), 16);
                                    if (s2.material == MAT_FOREST && s2.veg > 0.75) dense++;
                                }
                            if (dense >= 8) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                    placed = true;
                } else if (b.type == PT_FELISIAN) {
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 30 && !found; la += 2)
                        for (int k = 0; k < 256 && !found; k++) {
                            double ln = k * TAU / 256;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_FOREST && ss.height > 20) { lon = ln; latUse = la * DEG; found = true; placed = true; }
                        }
                }
                if (b.type == PT_MOLTEN) {
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 20 && !found; la += 0.02)
                        for (double ln = 0; ln < TAU && !found; ln += 0.0006) {
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_LAVA && ss.glow > 0.9) { lon = ln + 400.0 / (b.radiusKm * 1000.0); latUse = la * DEG; found = true; placed = true; }
                        }
                }
                setDrainageEnabled(drainOff.was);   // O6-03: the site samples the real ground
                sv.init(&sys, bi, latUse, lon, t0Use);
                double sunErr = 0, t = findTime(sv.site, alt, t0Use, &sunErr);
                if (sunErr > 1.0 * DEG && !placed) {
                    // G-04: a world locked to its star has one longitude with that light (the scan's first cratered world, the regress's
                    // cratered_noon, kept its night side): the longitude whose sun stands nearest the target, then the time there
                    double bestLon = lon;
                    for (int k = 0; k < 72; k++) {
                        double ln = k * TAU / 72; SurfaceSite probe; probe.init(&sys, bi, latUse, ln, t);
                        double e2 = std::fabs(effectiveSunAlt(probe, t) - alt);
                        if (e2 < sunErr) { sunErr = e2; bestLon = ln; }
                    }
                    if (bestLon != lon) { lon = bestLon; sv.init(&sys, bi, latUse, lon, t0Use); t = findTime(sv.site, alt, t0Use, &sunErr); }
                }
                if (sunErr > 3.0 * DEG) {   // no such light at this latitude in any season: the next body
                    printf("  %s: the light never stands at %.0f degrees at latitude %.0f (closest %.0f): skipped\n", b.name.c_str(), alt / DEG, latUse / DEG, effectiveSunAlt(sv.site, t) / DEG);
                    continue;
                }
                sv.init(&sys, bi, latUse, lon, t);
                SunInfo si = sv.site.sun(t);
                sv.player.yaw = faceYawOut < 1e8 ? faceYawOut : si.azimuth + yawOff;
                sv.player.pitch = facePitchOut < 1e8 ? facePitchOut : pitch;
                printf("  site: %s of %s (sector %lld 0 %lld), lat %.2f lon %.2f, the light at %.1f degrees (%.0f asked)\n", b.name.c_str(), s.name.c_str(), (long long)x, (long long)z, latUse / DEG, lon / DEG, effectiveSunAlt(sv.site, t) / DEG, alt / DEG);
                if (wantMat == -2) {
                    // stand 60 m west of the nearest ruin of this world (M4-07); the grid is in latitude/longitude
                    bool found = false;
                    int gLat0, gLon0;
                    sv.ruinCellAt(0, 0, gLat0, gLon0);
                    for (int ring = 0; ring < 40 && !found; ring++)
                        for (int dl = -ring; dl <= ring && !found; dl++)
                            for (int dn = -ring; dn <= ring && !found; dn++) {
                                if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                                Ruin ru;
                                if (!sv.ruinAt(gLat0 + dl, gLon0 + dn, ru)) continue;
                                TerrainVertex tvr = sv.site.sampleAt(ru.x, ru.z, 16);
                                if (tvr.water > -1e8f && tvr.h < tvr.water) continue;
                                double la2, lo2;
                                sv.site.latLonAt(ru.x - 60, ru.z, la2, lo2);
                                sv.init(&sys, bi, la2, lo2, t);
                                si = sv.site.sun(t);
                                sv.player.yaw = PI / 2; sv.player.pitch = 0.05;   // the ruin lies to the east
                                found = true;
                                sv.ruinAt(gLat0 + dl, gLon0 + dn, ru);
                                printf("ruin kind %d style %d size %.0f m at local (%.0f, %.0f)\n", ru.kind, ru.style, ru.size, ru.x, ru.z);
                            }
                    if (!found) continue;
                }
                if (wantMat == -47 || wantMat == -49) {   // C-01: the nearest settlement (a town before a village before a hamlet, within the first five rings of
                    // cells), from 30 m outside its edge, facing its centre
                    bool found = false; int gLat0, gLon0, bestClass = -1; double bestD = 1e18; Ruin best; RuinSpec bestSpec;
                    sv.ruinCellAt(0, 0, gLat0, gLon0);
                    for (int ring = 0; ring < 14 && !(found && ring > 4); ring++)
                        for (int dl = -ring; dl <= ring; dl++)
                            for (int dn = -ring; dn <= ring; dn++) {
                                if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                                Ruin ru;
                                if (!sv.ruinAt(gLat0 + dl, gLon0 + dn, ru) || ru.kind != RK_SETTLEMENT) continue;
                                double d = std::sqrt(ru.x * ru.x + ru.z * ru.z);
                                int cls = settlementRank(ru.spec->sclass);
                                if (cls > bestClass || (cls == bestClass && d < bestD)) { best = ru; bestSpec = *ru.spec; bestClass = cls; bestD = d; found = true; }
                            }
                    if (!found) { printf("  %s: no settlement within 14 cells\n", b.name.c_str()); continue; }
                    double h0 = bestSpec.heading, ext = 0;   // the buildings' reach along the street's axis (a grid town is more compact than its radius)
                    for (const Building& bd : bestSpec.buildings) {
                        double along = bd.x * std::sin(h0) + bd.z * std::cos(h0), across = bd.x * std::cos(h0) - bd.z * std::sin(h0);
                        if (std::fabs(across) < 12) ext = std::max(ext, along + std::max(bd.hw, bd.hd));
                    }
                    double la2, lo2, out = ext + 9;   // outside the gate (the street's axis), looking down the street
                    double standX = best.x + std::sin(h0) * out, standZ = best.z + std::cos(h0) * out, yaw = h0 + PI, pitch = 0.0;
                    std::string shardLine;
                    if (wantMat == -49) {   // C-03: on the floor of the room of the settlement's first shard, 1.9 m from it toward the room's middle (a stela: in front of it), looking down at it
                        std::vector<ShardSite> sites; shardSitesOf(bestSpec, sv.culture, sites);
                        if (sites.empty()) { printf("  %s: the settlement holds no shard\n", b.name.c_str()); continue; }
                        const ShardSite& s = sites[0]; const Building& bd = bestSpec.buildings[s.building];
                        double shx = best.x + s.x, shz = best.z + s.z;
                        double ax = bd.x - s.x, az = bd.z - s.z, L = std::sqrt(ax * ax + az * az);
                        if (L < 1e-6) { ax = 0; az = 1; L = 1; }
                        if (s.place == SHARD_AT_STELA) { ax = -ax; az = -az; }
                        standX = shx + ax / L * 1.9; standZ = shz + az / L * 1.9;
                        yaw = std::atan2(shx - standX, shz - standZ); pitch = -0.42;
                        shardLine = fmt("  shard %d of the settlement's %zu: %s (%s %d), %.1f m from the building's centre; the explorer 1.9 m from it", s.index, sites.size(), SHARD_PLACE_NAMES[s.place], BUILDING_KIND_NAMES[bd.kind], s.building, L);
                    }
                    sv.site.latLonAt(standX, standZ, la2, lo2);
                    sv.init(&sys, bi, la2, lo2, t);
                    si = sv.site.sun(t);
                    sv.player.yaw = yaw; sv.player.pitch = pitch;
                    if (wantMat == -49) { sv.player.x = 0; sv.player.z = 0; sv.player.y = sv.site.surfaceHeight(0, 0); }   // at the stand point itself (`init` puts the explorer 6, -4 from the origin)
                    sv.relocateCapsule(-std::sin(yaw) * 14, -std::cos(yaw) * 14);   // the capsule behind the camera, out of the picture
                    if (!shardLine.empty()) printf("%s\n", shardLine.c_str());
                    int houses = 0, towers = 0; for (const Building& bd : bestSpec.buildings) { if (bd.kind == BK_HOUSE) houses++; if (bd.kind == BK_TOWER) towers++; }
                    printf("  settlement: a %s of %zu buildings (%d houses, %d towers%s), plan %d, radius %.0f m, %.1f km from the type's spot; culture style %d, family %d, tall %.2f, decay %.2f, %s roofs, buried %.1f m\n",
                           SETTLEMENT_CLASS_NAMES[bestSpec.sclass], bestSpec.buildings.size(), houses, towers, bestSpec.walled ? ", walled" : "", bestSpec.plan, bestSpec.size, bestD / 1000.0,
                           sv.culture.style, sv.culture.family, sv.culture.tall, sv.culture.decay, sv.culture.stoneRoofs ? "stone" : "no", sv.culture.buried);
                }
                if (wantMat == -48) {   // C-01: the old shore of a dead desert world (its seas are the size of continents, so the whole world is scanned at
                    // 2048 m for the shore nearest the asked latitude, then 30 km round it at 512 m), standing 250 m up the land side (the beach
                    // ridges), facing out over the bed
                    const BodyGen& gg = sv.site.gen;
                    double bestScore = -1e18, sLat = 0, sLon = 0; bool found = false;
                    { DrainageOff off; for (int j = 0; j < 180; j++) for (int i = 0; i < 360; i++) {   // the steepest shore within 25 degrees of the asked latitude (a flat coast is a ramp)
                        double la = (-89.5 + j) * DEG, lo = (-179.5 + i) * DEG;
                        if (std::fabs(la - latUse) > 25 * DEG) continue;
                        SurfaceSample ss = sampleSurface(gg, StarSystem::bodyFromLatLon(la, lo), 2048);
                        if (ss.oldSea < 0.3 || ss.oldSea > 0.7) continue;
                        double dA = 1500.0 / (gg.R * 1000.0), lo2 = dA / std::max(std::cos(la), 0.05);   // a shore that drops 20-120 m over 3 km (a scarp, not a canyon wall nor a ramp)
                        double s4[4] = {sampleSurface(gg, StarSystem::bodyFromLatLon(la + dA, lo), 2048).height, sampleSurface(gg, StarSystem::bodyFromLatLon(la - dA, lo), 2048).height,
                                        sampleSurface(gg, StarSystem::bodyFromLatLon(la, lo + lo2), 2048).height, sampleSurface(gg, StarSystem::bodyFromLatLon(la, lo - lo2), 2048).height};
                        double drop = std::max(std::fabs(s4[0] - s4[1]), std::fabs(s4[2] - s4[3]));
                        double score = -std::fabs(std::log(std::max(drop, 1.0) / 50.0)) - 0.004 * std::fabs(la - latUse) / DEG;
                        if (score > bestScore) { bestScore = score; sLat = la; sLon = lo; found = true; }
                    } }
                    if (!found) { printf("  %s: no old shore near the asked latitude\n", b.name.c_str()); continue; }
                    sv.init(&sys, bi, sLat, sLon, t);
                    double bestD = 1e18, bx = 0, bz = 0; found = false;
                    { DrainageOff off; for (int iz = -30; iz <= 30; iz++) for (int ix = -30; ix <= 30; ix++) {
                        double px = ix * 1000.0, pz = iz * 1000.0;
                        SurfaceSample ss = sampleSurface(gg, sv.site.unitAt(px, pz), 512);
                        if (ss.oldSea < 0.35 || ss.oldSea > 0.65) continue;
                        double d = px * px + pz * pz;
                        if (d < bestD) { bestD = d; bx = px; bz = pz; found = true; }
                    } }
                    if (!found) { printf("  %s: no old shore within 30 km of the coarse one\n", b.name.c_str()); continue; }
                    double gx, gz;
                    { DrainageOff off;
                      gx = sampleSurface(gg, sv.site.unitAt(bx + 400, bz), 512).oldSea - sampleSurface(gg, sv.site.unitAt(bx - 400, bz), 512).oldSea;
                      gz = sampleSurface(gg, sv.site.unitAt(bx, bz + 400), 512).oldSea - sampleSurface(gg, sv.site.unitAt(bx, bz - 400), 512).oldSea; }
                    double gl = std::sqrt(gx * gx + gz * gz); if (gl < 1e-9) { gx = 1; gz = 0; gl = 1; }
                    gx /= gl; gz /= gl;
                    double la2, lo2, up = 60;   // the lip of the shore: the last point up the land side where the bed's mask is under 0.1, within 400 m; then 120 m back from it
                    { DrainageOff off; for (double q = 60; q <= 400; q += 20) { up = q; if (sampleSurface(gg, sv.site.unitAt(bx - gx * q, bz - gz * q), 64).oldSea < 0.1) break; } }
                    up += 120;
                    sv.site.latLonAt(bx - gx * up, bz - gz * up, la2, lo2);
                    sv.init(&sys, bi, la2, lo2, t);
                    { double err2 = 0; t = findTime(sv.site, alt, t0Use, &err2); sv.init(&sys, bi, la2, lo2, t); }   // the shore's own daylight (it may lie half a world from the type's spot)
                    si = sv.site.sun(t);
                    sv.player.yaw = std::atan2(gx, gz); sv.player.pitch = -0.1;
                    sv.relocateCapsule(-gx * 14, -gz * 14);
                    printf("  the ground at the camera %.0f m, 20 m ahead %.0f, 100 m ahead %.0f, 300 m ahead %.0f, 1 km ahead %.0f (eye %.1f m up)\n", sv.site.groundHeight(0, 0), sv.site.sampleAt(gx * 20, gz * 20, 16).h,
                           sv.site.sampleAt(gx * 100, gz * 100, 16).h, sv.site.sampleAt(gx * 300, gz * 300, 16).h, sv.site.sampleAt(gx * 1000, gz * 1000, 64).h, sv.player.y - sv.site.groundHeight(0, 0));
                    SurfaceSample here = sampleSurface(gg, sv.site.unitAt(0, 0), 16), sea = sampleSurface(gg, sv.site.unitAt(gx * 600, gz * 600), 16), deep = sampleSurface(gg, sv.site.unitAt(gx * 3000, gz * 3000), 64);
                    printf("  old shore at lat %.1f lon %.1f; the sea stood at %.0f m: here h %.0f m %s (sea %.2f), 600 m out h %.0f m %s (sea %.2f), 3 km out h %.0f m %s (sea %.2f)\n",
                           sv.site.lat0 / DEG, sv.site.lon0 / DEG, gg.oldSeaM, here.height, MATERIAL_NAMES[here.material], here.oldSea, sea.height, MATERIAL_NAMES[sea.material], sea.oldSea, deep.height, MATERIAL_NAMES[deep.material], deep.oldSea);
                }
                if (wantMat == -8 || wantMat == -9) {   // N4: the buggy from the seat (-8) and parked, seen from 5 m (-9), by day on open ground
                    sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
                    if (!sv.deployBuggy()) continue;
                    sv.buggy.unfold = 1;
                    sv.buggy.heading = 0.4;
                    if (wantMat == -8) { sv.player.x = sv.buggy.x + 1.5; sv.player.z = sv.buggy.z; if (!sv.toggleBuggy()) continue; sv.buggy.steer = 12 * DEG; sv.player.pitch = -0.06; }
                    else { sv.player.x = sv.buggy.x - 4.2; sv.player.z = sv.buggy.z - 3.0; double dx = sv.buggy.x - sv.player.x, dz = sv.buggy.z - sv.player.z; sv.player.yaw = std::atan2(dx, dz); sv.player.pitch = -0.12; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z); }
                }
                if (wantMat == -7) {   // N3: one creature of each land species 5 m ahead, side on, standing still
                    if (sv.critters.empty()) continue;
                    std::vector<Critter> keep;
                    for (int spi = 0; spi < sv.bestiary.landCount; spi++) {
                        Critter c; c.species = spi; c.herd = 0; c.seed = 1234 + spi; c.state = CS_IDLE; c.timer = 1e9;
                        c.size = sv.bestiary.species[spi].size; c.x = sv.player.x + (spi - (sv.bestiary.landCount - 1) * 0.5) * (2.5 + c.size * 2.5); c.z = sv.player.z + 5 + c.size * 2.5; c.heading = PI / 2;
                        keep.push_back(c);
                    }
                    sv.critters = keep;
                    sv.player.yaw = 0; sv.player.pitch = -0.08;
                    for (const Critter& c : sv.critters) printf("  %s (%s, %.1f m) at (%.1f, %.1f), ground %.1f\n", sv.bestiary.species[c.species].name.c_str(), PLAN_NAMES[sv.bestiary.species[c.species].plan], c.size, c.x, c.z, sv.site.groundHeight(c.x, c.z));
                }
                if (wantMat == -3 || wantMat == -5 || wantMat == -6) {
                    // stand 18 m (M4-05), 6 m or 60 m (N3) south of the first herd, facing it
                    if (sv.critters.empty()) continue;
                    double d = wantMat == -3 ? 18 : (wantMat == -5 ? 6 : 60);
                    if (!sv.testGotoHerd(d)) continue;
                    if (sv.site.sampleAt(sv.player.x, sv.player.z + d, 16).material == MAT_FOREST) continue;   // the herd must stand in the open
                    int n = 0; for (const Critter& c : sv.critters) if (c.herd == 0) n++;
                    const Species& sp = sv.bestiary.species[sv.critters[0].species];
                    printf("herd of %d %s (%s, %.1f m, %s, %s) at %.0f m\n", n, sp.name.c_str(), PLAN_NAMES[sp.plan], sp.size, ACTIVITY_NAMES[sp.activity], TEMPERAMENT_NAMES[sp.temperament], d);
                }
                if (lookAtWater) {
                    // face the direction with the most water within 150 m
                    double bestYaw = 0; int bestWater = -1;
                    for (int a = 0; a < 16; a++) {
                        double yaw = a * TAU / 16; int cnt = 0;
                        for (int d = 20; d <= 150; d += 10) if (sv.site.sampleAt(std::sin(yaw) * d, std::cos(yaw) * d, 16).material == MAT_WATER) cnt++;
                        if (cnt > bestWater) { bestWater = cnt; bestYaw = yaw; }
                    }
                    sv.player.yaw = bestYaw; sv.player.pitch = -0.02;
                }
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                Input in;
                sv.update(0.016, in, t, false);
                if (wantMat == -4) {   // G-04: the canopy the scene's check wants (three fifths of the forest ground within 200 m), or the next world: a temperate family covers a fifth
                    double cov = sv.testCanopyCoverage(200);
                    if (cov < 0.6) { printf("  %s: %.0f%% of the forest ground under canopy at its densest wood: the next world\n", b.name.c_str(), cov * 100); continue; }
                }
                tOut = t;
                return true;
            }
        }
    return false;
}

static void saveRaw(const char* path, const uint32_t* px, int w, int h) {
    std::ofstream f(path, std::ios::binary);
    f.write((const char*)&w, 4); f.write((const char*)&h, 4);
    f.write((const char*)px, (size_t)w * h * 4);
}
static bool loadRaw(const char* path, std::vector<uint32_t>& px, int& w, int& h) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.read((char*)&w, 4); f.read((char*)&h, 4);
    px.resize((size_t)w * h);
    f.read((char*)px.data(), (size_t)w * h * 4);
    return (bool)f;
}

struct CmpScene { const char* name; int type; double latDeg, alt, yawOff, pitch; int wantMat = -1; };
static const CmpScene CMP_SCENES[] = {
    {"felisian_forest", PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, -4},
    {"biome_tropical", PT_FELISIAN, 0, 40 * DEG, 0.5, -0.03, -10 - BIO_TROPICAL},
    {"biome_savanna", PT_FELISIAN, 5, 40 * DEG, 0.5, -0.03, -10 - BIO_SAVANNA},
    {"biome_desert", PT_FELISIAN, 10, 40 * DEG, 0.5, -0.03, -10 - BIO_DESERT},
    {"biome_temperate", PT_FELISIAN, 25, 35 * DEG, 0.5, -0.03, -10 - BIO_TEMPERATE},
    {"biome_grassland", PT_FELISIAN, 20, 35 * DEG, 0.5, -0.03, -10 - BIO_GRASSLAND},
    {"biome_taiga", PT_FELISIAN, 45, 25 * DEG, 0.5, -0.03, -10 - BIO_TAIGA},
    {"biome_tundra", PT_FELISIAN, 55, 15 * DEG, 0.5, -0.03, -10 - BIO_TUNDRA},
    {"biome_wetland", PT_FELISIAN, 5, 30 * DEG, 0.5, -0.03, -10 - BIO_WETLAND},
    {"tundra_lowsun", PT_FELISIAN, 55, 5 * DEG, 0.0, 0.05, -10 - BIO_TUNDRA},
    {"felisian_ruin", PT_FELISIAN, 12, 35 * DEG, 0.0, 0.0, -2},
    {"felisian_herd", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -3},
    {"felisian_herd_near", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -5},
    {"felisian_herd_far", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -6},
    {"creatures_lineup", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -7},
    {"buggy_seat", PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, -8},
    {"buggy_parked", PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, -9},
    {"felisian_shore", PT_FELISIAN, 12, 30 * DEG, 0.0, 0.0, MAT_SAND},
    {"felisian_river", PT_FELISIAN, 10, 40 * DEG, 0.0, -0.03, -21},   // B-320: on the bank of a river above the sea, looking across it
    {"felisian_lake", PT_FELISIAN, 10, 40 * DEG, 0.0, -0.03, -22},    // B-320: on a lake shore
    // R-307: the six new types, each at its signature
    {"europan_crack", PT_EUROPAN, 20, 30 * DEG, 0.6, -0.02, -30},
    {"tectonic_fissure", PT_TECTONIC, 15, 35 * DEG, 0.6, -0.03, -31},
    {"desert_erg", PT_DESERT, 15, 40 * DEG, 0.5, -0.02, -32},
    {"hydrocarbon_shore", PT_HYDROCARBON, 10, 30 * DEG, 0.0, -0.02, -33},
    {"bombarded_plain", PT_BOMBARDED, 15, 35 * DEG, 0.5, -0.02, -34},
    {"acidic_shore", PT_ACIDIC, 15, 40 * DEG, 0.0, -0.02, -35},
    {"geyser_basin", -1, 10, 40 * DEG, 0.0, 0.0, -36},   // R-307: 150 m from a geyser basin's vent on any world with the trait, facing it; `warm` waits for its jet
    {"felisian_sunset", PT_FELISIAN, 12, 4 * DEG, 0.0, 0.05},
    {"felisian_mountains", PT_FELISIAN, 0, 40 * DEG, 0.0, 0.02, -20},   // B-313: the pinned mountain review site of O6, facing its highest ground
    {"cratered_noon", PT_CRATERED, 12, 45 * DEG, 0.0, 0.0},
    {"thinatmo_sunset", PT_THINATMO, 12, 4 * DEG, 0.0, 0.05},
    {"molten_night", PT_MOLTEN, 12, -40 * DEG, 0.5, 0.15},
    {"icy_noon", PT_ICY, 12, 40 * DEG, 0.3, 0.05},
    {"comet_day", PT_COMET, 10, 40 * DEG, 0.4, 0.05},       // O4 (R-303): the nucleus by day, jets on the sunlit ground
    {"comet_night", PT_COMET, 10, -30 * DEG, PI, 0.35},     // and at night, looking away from the sun: the tail overhead
    // B-403: the aurora: a thin-atmosphere world round a pulsar or a blue giant, 64 degrees of latitude, the darkest hour,
    // facing the pole (the sun's azimuth at its lowest), and the same looking straight up
    {"thinatmo_aurora", PT_THINATMO, 58, -30 * DEG, 0.0, 0.25, -40},
    {"thinatmo_aurora_zenith", PT_THINATMO, 64, -30 * DEG, 0.0, 1.5, -41},
    {"aurora_moon", -1, 64, -30 * DEG, 0.0, 0.25, -42},   // any clear-air world with a moon or the parent in the poleward sky at the curtains' height, facing it: its night side shows the curtains in front
    {"neutron_glass", -1, 15, 25 * DEG, 0.6, -0.03, -43},   // S-03: standing on a glass sheet of a neutron star's glassed world, the star low and off to the side for its glints
    {"protostar_night", -1, 20, -12 * DEG, 0.0, 0.12, -44},   // S-04: any world of a protostar, the star 12 degrees under the horizon, facing it: the cloud's glow and the disc's band over the horizon
    {"wolf_rayet_day", -1, 10, 38 * DEG, 0.5, 0.35, -45},   // S-05: any world of a Wolf-Rayet star at the type's default spot, the star 38 degrees up and 29 off the view, looking 20 up: the shell's ring round the blinding sun over a bare plain
    {"black_hole_sky", -1, 10, 30 * DEG, 0.4, 0.3, -46},
    {"civilisation_town", PT_FELISIAN, 12, 35 * DEG, 0.0, 0.02, -47},   // C-01: the nearest settlement of the scan's first felisian world that had a people, from 30 m outside its edge
    {"civilisation_desert", PT_DESERT, 15, 40 * DEG, 0.0, 0.02, -47},   // C-01: the same on a dead desert world
    {"civilisation_shard", PT_FELISIAN, 12, 35 * DEG, 0.0, -0.42, -49},  // C-03: on the floor of the room that holds the first shard of that settlement, 1.9 m from it, looking down at it
    {"desert_dead_sea", PT_DESERT, 15, 35 * DEG, 0.0, -0.02, -48},      // C-01: on the old shore of a dead desert world, looking out over the dry seabed   // S-06: any world of a black hole at the type's default spot, the hole 30 degrees up and 23 off the view, looking 17 up: the shadow, the disc and the bent stars over a bare plain
};


// B-309: temporal stability of a scene: eight frames a thirtieth of a second apart, the mean absolute change of the
// luminance between consecutive frames, per region (a box of 40 logical pixels round the sun, the sky, the ground),
// and a heat map of where the picture moves (`shots/tests/stability_<scene>.png`, the last frame beside it). What
// should move: water, leaves, weather, creatures, the granulation of the sun. What should not: the sun's glow.
// B-313: `stability <scene> <metres per frame>` walks the camera forward instead: the far-ground band (rows 42-62%
// of the frame, ground beyond a few tens of metres) reports the mean change and the share of its pixels that jump by
// more than 40 levels in a frame, which is what a terrain ring popping cell by cell does and parallax does not; the
// sheet is `stability_<scene>_walk.png`
static int runStability(const char* want, double walk) {
    SpaceRenderer sr; StarNeighborhood nb;
    int shown = 0;
    for (const CmpScene& sc : CMP_SCENES) {
        if (want && std::string(want) != sc.name) continue;
        StarSystem sys; SurfaceView sv; double t = 0;
        if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("%s: no body\n", sc.name); continue; }
        const int N = walk > 0 ? 16 : 8;
        if (walk > 0) { sv.flocks.clear(); sv.critters.clear(); }   // the skyline count below must not see birds
        // the walking camera is the free camera 1.7 m over the ground, without the explorer's bob; the explorer walks along
        // under it so the rings follow
        if (walk > 0) { sv.enterFreeCam(); sv.freePos = Vec3(sv.player.x, sv.player.y + 1.7, sv.player.z); sv.freeYaw = sv.player.yaw; sv.freePitch = sv.player.pitch; }
        Framebuffer fb; Input in;
        std::vector<uint32_t> px((size_t)FBW * FBH), prev;
        std::vector<double> heat((size_t)FBW * FBH, 0);
        std::vector<int> pops((size_t)FBW * FBH, 0);
        double sunSx = -1, sunSy = -1;
        long flips = 0;   // B-319: pixels that swap between the sky bank and anything else from one frame to the next (a far tree flickering against the sky)
        std::vector<uint8_t> banks((size_t)FBW * FBH), prevBanks;
        auto lum = [](uint32_t c) { return 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255); };
        for (int k = 0; k < N; k++) {
            double tt = t + k / 30.0;
            if (walk > 0 && k > 0) {
                sv.player.x += std::sin(sv.player.yaw) * walk; sv.player.z += std::cos(sv.player.yaw) * walk;
                sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                sv.freePos = Vec3(sv.player.x, sv.player.y + 1.7, sv.player.z);
            }
            sv.update(1.0 / 30, in, tt, false);
            sv.render(fb, tt, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(px.data(), 1.0);
            if (k == 0) { const Vec3& sd = sv.env.sunDisc.dirLocal; if (!sv.projectPoint(sv.player.x + sd.x * 1e5, sv.player.y + 1.65 + sd.y * 1e5, sv.player.z + sd.z * 1e5, sunSx, sunSy)) sunSx = -1; }
            if (!prev.empty()) for (size_t i = 0; i < px.size(); i++) { double d = std::fabs(lum(px[i]) - lum(prev[i])); heat[i] += d; if (d > 40) pops[i]++; }
            for (size_t i = 0; i < banks.size(); i++) banks[i] = (uint8_t)(bankOf(fb.idx[i]) == 1 && fb.invz[i] == 0 ? 1 : 0);
            if (!prevBanks.empty()) for (size_t i = 0; i < banks.size(); i++) if (banks[i] != prevBanks[i]) flips++;
            prevBanks = banks;
            prev = px;
        }
        double sunSum = 0, skySum = 0, gndSum = 0, farSum = 0; int sunN = 0, skyN = 0, gndN = 0, farN = 0, farPops = 0;
        double box = 20.0 * FB_SCALE;
        for (int y = 0; y < FBH; y++)
            for (int x = 0; x < FBW; x++) {
                double h = heat[(size_t)y * FBW + x] / (N - 1);
                if (sunSx >= 0 && std::fabs(x - sunSx) < box && std::fabs(y - sunSy) < box) { sunSum += h; sunN++; }
                if (y < FBH * 0.45) { skySum += h; skyN++; } else if (y > FBH * 0.55) { gndSum += h; gndN++; }
                if (y >= FBH * 0.42 && y < FBH * 0.62) { farSum += h; farN++; farPops += pops[(size_t)y * FBW + x]; }
            }
        double mx = 0; for (double h : heat) mx = std::max(mx, h);
        std::vector<uint32_t> sheet((size_t)FBW * 2 * FBH);
        for (int y = 0; y < FBH; y++)
            for (int x = 0; x < FBW; x++) {
                sheet[(size_t)y * FBW * 2 + x] = prev[(size_t)y * FBW + x];
                int v = (int)std::min(255.0, heat[(size_t)y * FBW + x] / (N - 1) * 8.0);
                sheet[(size_t)y * FBW * 2 + FBW + x] = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
            }
        std::string fn = std::string("shots/tests/stability_") + sc.name + (walk > 0 ? "_walk" : "") + ".png";
        writePNG(fn.c_str(), sheet.data(), FBW * 2, FBH);
        if (walk > 0)
            printf("%-18s walking %.1f m/frame: far band mean %.2f, %.2f%% of its pixels jump over 40 per frame; skyline flips %.0f px/frame; sky %.2f ground %.2f (peak %.0f) -> %s\n", sc.name, walk,
                   farN ? farSum / farN : 0.0, farN ? 100.0 * farPops / ((double)farN * (N - 1)) : 0.0, (double)flips / (N - 1) / (FB_SCALE * FB_SCALE), skyN ? skySum / skyN : 0.0, gndN ? gndSum / gndN : 0.0, mx / (N - 1), fn.c_str());
        else
        printf("%-18s sun %s%.2f  sky %.2f  ground %.2f  (mean |dL| per frame, 0..255; peak %.0f) -> %s\n", sc.name,
               sunSx >= 0 ? (sunSx >= 0 && sunSx < FBW && sunSy >= 0 && sunSy < FBH ? "in frame " : "off frame ") : "below ", sunN ? sunSum / sunN : 0.0, skyN ? skySum / skyN : 0.0, gndN ? gndSum / gndN : 0.0, mx / (N - 1), fn.c_str());
        shown++;
    }
    return shown ? 0 : 1;
}
// M7-04 frame hashes (defined after the scene helpers it uses)
static void appendFrameHashes(std::vector<std::pair<std::string, uint64_t>>& lines, std::vector<std::string>& notes) {
    const uint64_t FNV0 = 1469598103934665603ULL;
    // M7-04 frame hashes: three fixed surface scenes and one space frame at 1x (the framebuffer indices, before the RGB conversion)
    {
        int savedScale = g_testScale;
        setFramebufferScale(1);
        SpaceRenderer sr; StarNeighborhood nb;
        const char* want[3] = {"felisian_shore", "cratered_noon", "molten_night"};
        for (const char* name : want)
            for (const CmpScene& sc : CMP_SCENES) {
                if (std::string(name) != sc.name) continue;
                StarSystem sys; SurfaceView sv; double t = 0;
                if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) continue;
                for (const Body& b : sys.bodies) if (b.type != PT_COMPANION) sr.mapFor(b);   // sky bodies drawn textured, never mid-generation
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                uint64_t h = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                lines.push_back({std::string("scene_") + name, h});
                notes.push_back(std::string("frame of scene ") + name + " at 1x (" + sys.bodies[sv.site.body].name + ")");
            }
        for (int64_t x = 150; x < 300; x++) {
            bool done = false;
            for (int64_t z = 20; z < 120 && !done; z++) {
                Star s;
                if (!starInSector(x, 0, z, s) || s.cls != STAR_YELLOW) continue;
                StarSystem sys; sys.generate(s);
                double d = std::max(s.radiusKm * STAR_CLASSES[s.cls].firstOrbitMult, STAR_CLASSES[s.cls].minFirstOrbitKm);
                Vec3 shipPos = s.pos + Vec3(0, 0, -d);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = 1234.0; c.shipPos = shipPos; c.cam = cameraBasis(0, 0);
                for (const Body& b : sys.bodies) if (b.type != PT_COMPANION) sr.mapFor(b);
                sr.setupPalette(fb, &sys, -1, -1, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                lines.push_back({"space_S00", fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix))});
                notes.push_back("space frame from the first orbit of " + s.name);
                done = true;
            }
            if (done) break;
        }
        setFramebufferScale(savedScale);
    }
}


// Renders the fixed scenes at every scale with that scale's kernel: shots/scale/<scene>_<s>x.png (+ .raw).
// B-310: the resolution sheets. Each configuration is a render scale and a mush box; every scene is rendered with each, then
// `sheet` shows them as the 1280 x 800 window would (the integer scales nearest, 3x through the window's bilinear pass,
// the scanlines on), cropped to the same window region, so the eye compares like with like.
struct ScaleCfg { int scale, kernel; const char* tag; const char* label; };
static const ScaleCfg SCALE_CFGS[] = {
    {2, 3, "2x_k3", "A  2X 640X400, 3X3 MUSH (THE OLD DEFAULT)"},
    {3, 3, "3x_k3", "B  3X 960X600, 3X3 MUSH"},
    {3, 4, "3x_k4", "C  3X 960X600, 4X4 MUSH"},
    {4, 3, "4x_k3", "D  4X 1280X800, 3X3 MUSH"},
    {4, 4, "4x_k4", "E  4X 1280X800, 4X4 MUSH"},
    {4, 5, "4x_k5", "F  4X 1280X800, 5X5 MUSH (CLASSIC)"},
};
static const char* SHEET_SCENES[] = {"felisian_forest", "felisian_shore", "felisian_sunset", "cratered_noon", "thinatmo_sunset", "icy_noon", "buggy_seat", "orbit"};

static void renderCompare() {
    mkdir("shots/tests/scale", 0755);
    for (const ScaleCfg& cfg : SCALE_CFGS) {
        setFramebufferScale(cfg.scale);
        g_mushKernel = cfg.kernel;
        SpaceRenderer sr;
        StarNeighborhood nb;
        std::vector<uint32_t> rgbBuf((size_t)FBW * FBH);
        for (const char* name : SHEET_SCENES) {
            if (std::string(name) == "orbit") continue;
            for (const CmpScene& sc : CMP_SCENES) {
                if (std::string(name) != sc.name) continue;
                StarSystem sys;
                SurfaceView sv;
                double t = 0;
                if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("scene %s: no body found\n", sc.name); continue; }
                Framebuffer fb;
                double t0 = nowSec();
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                if (sv.inBuggy && !sv.chaseCam) sv.cameraFeed(fb, t);
                double ms = (nowSec() - t0) * 1000;
                fb.toRGB(rgbBuf.data(), 1.0);
                std::string base = std::string("shots/tests/scale/") + sc.name + "_" + cfg.tag;
                saveRaw((base + ".raw").c_str(), rgbBuf.data(), FBW, FBH);
                writePNG((base + ".png").c_str(), rgbBuf.data(), FBW, FBH);
                printf("%-16s %s %dx%d kernel %d  %.1f ms\n", sc.name, cfg.tag, FBW, FBH, g_mushKernel, ms);
            }
        }
        // the felisian globe from orbit
        bool done = false;
        for (int64_t x = 150; x < 320 && !done; x++)
            for (int64_t z = 20; z < 140 && !done; z++) {
                Star st;
                if (!starInSector(x, 0, z, st)) continue;
                StarSystem sys; sys.generate(st);
                for (int bi = 0; bi < (int)sys.bodies.size() && !done; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != PT_FELISIAN || b.parent >= 0) continue;
                    double t = 5000.0;
                    Vec3 bp = sys.bodyPos(bi, t);
                    Vec3 toStar = normalize(sys.star.pos - bp);
                    Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
                    Vec3 shipPos = bp + normalize(toStar * 0.6 + side * 0.8 + Vec3(0, 0.25, 0)) * (b.radiusKm * 3.5);
                    Vec3 fwd = normalize(bp - shipPos);
                    nb.update(shipPos);
                    Framebuffer fb;
                    SpaceContext c;
                    c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                    c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y));
                    c.bankBodyA = bi;
                    sr.setupPalette(fb, &sys, bi, -1, 1.0);
                    double t0 = nowSec();
                    sr.render(fb, c);
                    fb.mush(2);
                    double ms = (nowSec() - t0) * 1000;
                    fb.toRGB(rgbBuf.data(), 1.0);
                    std::string base = std::string("shots/tests/scale/orbit_") + cfg.tag;
                    saveRaw((base + ".raw").c_str(), rgbBuf.data(), FBW, FBH);
                    writePNG((base + ".png").c_str(), rgbBuf.data(), FBW, FBH);
                    printf("%-16s %s %dx%d kernel %d  %.1f ms\n", "orbit", cfg.tag, FBW, FBH, g_mushKernel, ms);
                    done = true;
                }
            }
    }
    setFramebufferScale(1);
}

// the frame as the 1280 x 800 window shows it: nearest for integer factors, bilinear for 3x, the scanlines every four rows
static void windowImage(const std::vector<uint32_t>& px, int w, int h, std::vector<uint32_t>& out) {
    const int W = 1280, H = 800;
    out.assign((size_t)W * H, 0xFF000000u);
    double fx = (double)w / W, fy = (double)h / H;
    bool integer = (W % w) == 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            uint32_t c;
            if (integer) c = px[(size_t)(y * h / H) * w + x * w / W];
            else {
                double sx = (x + 0.5) * fx - 0.5, sy = (y + 0.5) * fy - 0.5;
                int x0 = clampi((int)std::floor(sx), 0, w - 1), y0 = clampi((int)std::floor(sy), 0, h - 1), x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
                double tx = clampd(sx - x0, 0, 1), ty = clampd(sy - y0, 0, 1);
                uint32_t p00 = px[(size_t)y0 * w + x0], p10 = px[(size_t)y0 * w + x1], p01 = px[(size_t)y1 * w + x0], p11 = px[(size_t)y1 * w + x1];
                int ch[3];
                for (int k = 0; k < 3; k++) {
                    double a = ((p00 >> (8 * k)) & 255) * (1 - tx) + ((p10 >> (8 * k)) & 255) * tx, b = ((p01 >> (8 * k)) & 255) * (1 - tx) + ((p11 >> (8 * k)) & 255) * tx;
                    ch[k] = (int)(a * (1 - ty) + b * ty + 0.5);
                }
                c = 0xFF000000u | ((uint32_t)ch[2] << 16) | ((uint32_t)ch[1] << 8) | (uint32_t)ch[0];
            }
            if ((y & 3) == 3) {   // the scanline: black at alpha 70/255
                uint32_t r = (c & 255) * 185 / 255, g = ((c >> 8) & 255) * 185 / 255, b = ((c >> 16) & 255) * 185 / 255;
                c = 0xFF000000u | (b << 16) | (g << 8) | r;
            }
            out[(size_t)y * W + x] = c;
        }
}

// Composes shots/tests/scale_<scene>.png: the six configurations as the window shows them, the same 640 x 400 region of the
// window (the lower middle: ground, horizon, the sun) in a 2 x 3 grid; the whole window images go to shots/tests/scale/.
static void composeSheets() {
    const int TW = 640, TH = 400, COLS = 2, ROWS = 3, SW = TW * COLS, SH = TH * ROWS;
    const int CX = 320, CY = 300;   // the crop's origin in the window
    for (const char* scene : SHEET_SCENES) {
        std::vector<uint32_t> sheet((size_t)SW * SH, rgb(20, 20, 20));
        RGBCanvas cv{sheet.data(), SW, SH, 1};
        bool any = false;
        int i = 0;
        for (const ScaleCfg& cfg : SCALE_CFGS) {
            int slot = i++;
            std::vector<uint32_t> px; int w = 0, h = 0;
            std::string fn = std::string("shots/tests/scale/") + scene + "_" + cfg.tag + ".raw";
            if (!loadRaw(fn.c_str(), px, w, h)) continue;
            any = true;
            std::vector<uint32_t> win;
            windowImage(px, w, h, win);
            writePNG((std::string("shots/tests/scale/") + scene + "_" + cfg.tag + "_window.png").c_str(), win.data(), 1280, 800);
            int ox = (slot % COLS) * TW, oy = (slot / COLS) * TH;
            for (int y = 0; y < TH; y++)
                for (int x = 0; x < TW; x++) sheet[(size_t)(oy + y) * SW + ox + x] = win[(size_t)(CY + y) * 1280 + CX + x];
            fillRectRGB(cv, ox, oy, ox + textWidth(cfg.label, 2) + 8, oy + 18, rgb(0, 0, 0));
            drawText(cv, ox + 4, oy + 3, cfg.label, rgb(255, 220, 120), 2);
        }
        if (!any) continue;
        std::string out = std::string("shots/tests/scale_") + scene + ".png";
        writePNG(out.c_str(), sheet.data(), SW, SH);
        printf("wrote %s\n", out.c_str());
    }
}

// ---- survey and gallery (M9-01) ------------------------------------------------------------
// survey [N]: distributions over N bodies of the start region; gallery <type>: 16 random sites in one sheet.
static void runSurvey(int wanted) {
    int types[PT_COUNT] = {0}, planets = 0, moons = 0, systems = 0, felSystems = 0;
    int tempHist[6] = {0}, gravHist[5] = {0}, moonHist[8] = {0};
    double matCov[PT_COUNT][MAT_COUNT] = {{0}};
    int matN[PT_COUNT] = {0};
    int bodies = 0;
    // M5-09 (R-001): what hangs in the sky at random landing sites and times
    int skySites = 0, sky05 = 0, sky1 = 0, sky3 = 0, sky10 = 0, skyPoint = 0, skyParent = 0, skyMoon = 0, skySister = 0, skyOther = 0, skyTwoSuns = 0;
    int multiples = 0, substellar = 0, comets = 0, doubles = 0, lockedN = 0;
    for (int64_t x = 150; x < 320 && bodies < wanted; x++)
        for (int64_t z = 20; z < 140 && bodies < wanted; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            systems++;
            if (sys.companion >= 0) multiples++;
            bool fel = false;
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type == PT_SUBSTELLAR) substellar++;
                if (b.type == PT_COMET) comets++;
                if (b.doublePlanet && b.parent >= 0) doubles++;
                if (b.locked) lockedN++;
                if (!PLANET_TYPES[b.type].landable) continue;
                Rng sr(b.seed ^ 0x5C1EULL);
                for (int k = 0; k < 2; k++) {
                    double lat = std::asin(sr.sym(1.0)), lon = sr.range(0, TAU), t = sr.range(0, 4e6);
                    Vec3 site = sys.surfacePointWorld(bi, t, lat, lon, 0.002);
                    Vec3 up = normalize(site - sys.bodyPos(bi, t));
                    double best = 0; int bestKind = -1; bool point = false; int sunsUp = 0;
                    for (int j = 0; j < (int)sys.bodies.size(); j++) {
                        if (j == bi) continue;
                        Vec3 rel = sys.bodyPos(j, t) - site;
                        double d = length(rel);
                        if (dot(rel / d, up) < 0.02) continue;
                        double ang = std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1)) / DEG;
                        if (sys.bodies[j].type == PT_COMPANION) { sunsUp++; continue; }
                        if (sys.bodies[j].parent < 0 && ang < 0.25) point = true;
                        if (ang > best) { best = ang; bestKind = j == b.parent ? 0 : (sys.bodies[j].parent == bi ? 1 : (sys.bodies[j].parent == b.parent && b.parent >= 0 ? 2 : 3)); }
                    }
                    if (dot(normalize(sys.star.pos - site), up) > 0.02) sunsUp++;
                    skySites++;
                    if (best > 0.5) { sky05++; if (bestKind == 0) skyParent++; else if (bestKind == 1) skyMoon++; else if (bestKind == 2) skySister++; else skyOther++; }
                    if (best > 1) sky1++;
                    if (best > 3) sky3++;
                    if (best > 10) sky10++;
                    if (point) skyPoint++;
                    if (sunsUp >= 2) skyTwoSuns++;
                }
            }
            for (const Body& b : sys.bodies) {
                bodies++;
                types[b.type]++;
                if (b.parent < 0) { planets++; moonHist[std::min(7, b.moonCount)]++; } else moons++;
                if (b.type == PT_FELISIAN) fel = true;
                tempHist[std::min(5, (int)(b.tempK / 150))]++;
                gravHist[std::min(4, (int)(b.gravity / 5.0))]++;
                if (b.type != PT_GASGIANT && matN[b.type] < 40) {
                    BodyGen g = BodyGen::make(b);
                    double detail = TAU * g.R * 1000.0 / 48.0 * 0.6;
                    int cnt[MAT_COUNT] = {0}, tot = 0;
                    for (int j = 0; j < 12; j++)
                        for (int i = 0; i < 24; i++) {
                            double lat = (0.5 - (j + 0.5) / 12) * PI, lon = ((i + 0.5) / 24) * TAU - PI;
                            cnt[sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), detail).material]++; tot++;
                        }
                    for (int m = 0; m < MAT_COUNT; m++) matCov[b.type][m] += (double)cnt[m] / tot;
                    matN[b.type]++;
                }
            }
            if (fel) felSystems++;
        }
    printf("survey: %d systems, %d bodies (%d planets, %d moons); felisian in %d systems (%.0f%%)\n", systems, bodies, planets, moons, felSystems, 100.0 * felSystems / std::max(1, systems));
    printf("types:"); for (int i = 0; i < PT_COUNT; i++) printf("  %s %.1f%%", PLANET_TYPES[i].name, 100.0 * types[i] / std::max(1, bodies)); printf("\n");
    printf("temperature K: <150 %d, 150-300 %d, 300-450 %d, 450-600 %d, 600-750 %d, >750 %d\n", tempHist[0], tempHist[1], tempHist[2], tempHist[3], tempHist[4], tempHist[5]);
    printf("gravity m/s2: <5 %d, 5-10 %d, 10-15 %d, 15-20 %d, >20 %d\n", gravHist[0], gravHist[1], gravHist[2], gravHist[3], gravHist[4]);
    printf("moons per planet:"); for (int i = 0; i < 8; i++) printf(" %d:%d", i, moonHist[i]); printf("\n");
    printf("M5: multiple systems %d (%.0f%%), substellar objects %d, comets %d, double planets %d, locked planets %d\n", multiples, 100.0 * multiples / std::max(1, systems), substellar, comets, doubles, lockedN);
    if (skySites) {
        printf("sky at %d random sites (R-001 targets in brackets): disc >0.5 deg %.0f%%, >1 deg %.0f%% [45-50], >3 deg %.0f%% [35], >10 deg %.0f%% [12-15]; a planet as a point %.0f%%; two suns up %.0f%%\n",
               skySites, 100.0 * sky05 / skySites, 100.0 * sky1 / skySites, 100.0 * sky3 / skySites, 100.0 * sky10 / skySites, 100.0 * skyPoint / skySites, 100.0 * skyTwoSuns / skySites);
        printf("  the disc is: the parent %.0f%%, an own moon %.0f%%, a sister moon %.0f%%, another world %.0f%%\n", 100.0 * skyParent / std::max(1, sky05), 100.0 * skyMoon / std::max(1, sky05), 100.0 * skySister / std::max(1, sky05), 100.0 * skyOther / std::max(1, sky05));
    }
    for (int t = 0; t < PT_COUNT; t++) {
        if (!matN[t]) continue;
        printf("  %-16s", PLANET_TYPES[t].name);
        for (int m = 0; m < MAT_COUNT; m++) if (matCov[t][m] / matN[t] > 0.005) printf(" %s %.0f%%", MATERIAL_NAMES[m], 100.0 * matCov[t][m] / matN[t]);
        printf("\n");
    }
}

static void renderGallery(int type) {
    SpaceRenderer sr;
    StarNeighborhood nb;
    const int TW = 320, TH = 200;
    std::vector<uint32_t> sheet((size_t)TW * 4 * TH * 4, rgb(10, 10, 10));
    RGBCanvas cv{sheet.data(), TW * 4, TH * 4, 1};
    int n = 0;
    Rng r(777 + type);
    setFramebufferScale(1);
    for (int64_t x = 150; x < 320 && n < 16; x++)
        for (int64_t z = 20; z < 140 && n < 16; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && n < 16; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != type || !PLANET_TYPES[b.type].landable) continue;
                if (r.chance(0.5)) continue;
                double lat = std::asin(r.sym(0.9)), lon = r.range(-PI, PI);
                SurfaceView sv;
                sv.init(&sys, bi, lat, lon, 1000.0);
                double t = findTime(sv.site, 35 * DEG, 1000.0);
                sv.init(&sys, bi, lat, lon, t);
                sv.player.yaw = r.range(0, TAU); sv.player.pitch = 0.02;
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                Input in;
                sv.update(0.016, in, t, false);
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                std::vector<uint32_t> px((size_t)FBW * FBH);
                fb.toRGB(px.data(), 1.0);
                int ox = (n % 4) * TW, oy = (n / 4) * TH;
                for (int y = 0; y < TH; y++) for (int xx = 0; xx < TW; xx++) sheet[(size_t)(oy + y) * TW * 4 + ox + xx] = px[(size_t)y * FBW + xx];
                std::string label = b.name + "  " + std::to_string((int)(lat / DEG)) + "/" + std::to_string((int)(lon / DEG)) + "  " + MATERIAL_NAMES[sv.site.sampleAt(0, 0, 16).material];
                for (char& c : label) c = (char)toupper((unsigned char)c);
                fillRectRGB(cv, ox, oy, ox + textWidth(label.c_str()) + 4, oy + 9, rgb(0, 0, 0));
                drawText(cv, ox + 2, oy + 1, label.c_str(), rgb(255, 220, 120));
                n++;
            }
        }
    std::string tn = PLANET_TYPES[type].name;
    for (char& ch : tn) if (ch == ' ') ch = '_';
    std::string fn = "shots/tests/gallery_" + tn + ".png";
    writePNG(fn.c_str(), sheet.data(), TW * 4, TH * 4);
    printf("gallery: %d sites of %s -> %s\n", n, PLANET_TYPES[type].name, fn.c_str());
}

// ---- random-walk fuzz (M0-08): random keys and mouse across every state -------------------
static int runFuzz(int frames) {
    Game game;
    game.savePrefix = "shots/tests/fuzz_save";
    game.moviesDir = "shots/tests/test_movies"; game.recFixedTake = true;   // random Ctrl+R must not litter movies/
    game.settingsPath = "shots/tests/fuzz_settings.txt";
    game.guidePath = "shots/tests/fuzz_guide.txt"; game.keysPath = "shots/tests/fuzz_keys.txt";
    applyTestScale(game);
    Input in;
    Rng r(20260926);
    static const int keys[] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE, KEY_TAB, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_A, KEY_S, KEY_D,
                               KEY_R, KEY_V, KEY_L, KEY_X, KEY_O, KEY_C, KEY_T, KEY_Q, KEY_E, KEY_K, KEY_I, KEY_H, KEY_N, KEY_P,
                               KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_F1, KEY_F2, KEY_F5, KEY_F9, KEY_PAGE_UP, KEY_PAGE_DOWN};
    const int nk = sizeof(keys) / sizeof(keys[0]);
    int bad = 0, states[16] = {0};
    double t0 = nowSec();
    for (int f = 0; f < frames; f++) {
        in.newFrame();
        for (int i = 0; i < nk; i++) {
            int k = keys[i];
            if (r.chance(0.02)) { in.pressed[k] = true; in.down[k] = true; }
            else if (in.down[k] && r.chance(0.15)) in.down[k] = false;
        }
        in.mouseDx = r.sym(40); in.mouseDy = r.sym(25); in.wheel = r.irange(3) - 1;
        in.mousePressed[0] = r.chance(0.02); in.mouseDown[0] = in.mousePressed[0];
        game.frame(in, 1.0 / 60);
        game.wantsQuit = false; game.wantsScreenshot = false; game.wantsFullscreenToggle = false; game.wantsWindowScale = 0;
        states[(int)game.state & 15]++;
        std::string h = game.testHealth();
        if (!h.empty()) { printf("frame %d: non-finite %s\n", f, h.c_str()); if (++bad > 5) break; }
    }
    printf("fuzz: %d frames in %.1f s, %d health failures; frames per state:", frames, nowSec() - t0, bad);
    for (int s = 0; s < 12; s++) printf(" %d:%d", s, states[s]);
    printf("\n");
    return bad ? 1 : 0;
}

// Re-anchoring check (KI-007): walk 51 km, the frame moves, positions and headings stay consistent.
static void testReanchor() {
    for (int64_t x = 150; x < 320; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_ROCKY) continue;
                SurfaceView sv;
                double t = 1000.0;
                sv.init(&sys, bi, 62 * DEG, 0.4, t);   // high latitude: meridians converge noticeably
                sv.relocateCapsule(120, -80);
                sv.player.x = 51e3; sv.player.z = 3e3; sv.player.yaw = 0.7;
                Input in;
                // "before": measured straight from the old frame (update() would already re-anchor)
                auto measure = [&](double& lat, double& lon, double& cap, double& bear, Vec3& fwd) {
                    sv.site.latLonAt(sv.player.x, sv.player.z, lat, lon); lat /= DEG; lon /= DEG;
                    double dx = sv.capsuleX - sv.player.x, dz = sv.capsuleZ - sv.player.z;
                    cap = std::sqrt(dx * dx + dz * dz); bear = wrap2pi(std::atan2(dx, dz)) / DEG;
                    fwd = sv.site.unitAt(sv.player.x + 1000 * std::sin(sv.player.yaw), sv.player.z + 1000 * std::cos(sv.player.yaw));
                };
                double latB, lonB, capB, bearB, yawB = sv.player.yaw; Vec3 fwdB;
                measure(latB, lonB, capB, bearB, fwdB);
                sv.update(0.016, in, t, false);   // re-anchors (distance > 50 km)
                double latA, lonA, capA, bearA, yawA = sv.player.yaw; Vec3 fwdA;
                measure(latA, lonA, capA, bearA, fwdA);
                double fwdErrM = length(fwdA - fwdB) * b.radiusKm * 1000;
                printf("reanchor on %s: site (%.2f,%.2f) -> (%.4f,%.4f), player at (%.0f,%.0f) m; lat/lon %.4f/%.4f -> %.4f/%.4f; capsule %.0f m %.1f deg -> %.0f m %.1f deg; yaw %.3f -> %.3f; forward point moved %.1f m\n",
                       b.name.c_str(), 62 * DEG, 0.4, sv.site.lat0, sv.site.lon0, sv.player.x, sv.player.z, latB, lonB, latA, lonA, capB, bearB, capA, bearA, yawB, yawA, fwdErrM);
                return;
            }
        }
}

// M4-09: render a few seconds of every sound family into a WAV file and check for NaN and clipping.
static void testAudio() {
    AudioSynth synth;
    const int sr = 22050;
    std::vector<int16_t> pcm;
    int nan = 0, clip = 0;
    struct Stage { const char* name; double hum, engine, wind, tone, rain, lava, surf, birds, breath; int step; double leaves; };
    const Stage stages[] = {{"ship", 0.6, 0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0}, {"vimana", 0.6, 0.9, 0, 0.5, 0, 0, 0, 0, 0, 0, 0}, {"shore", 0.05, 0, 0.4, 0.25, 0, 0, 0.6, 0.6, 0, 1, 0},
                            {"desert storm", 0.05, 0, 1.0, 0.6, 0, 0, 0, 0, 0.4, 2, 0}, {"mountain rain", 0.05, 0, 0.6, 1.0, 0.8, 0, 0, 0, 0, 3, 0}, {"lava", 0.05, 0, 0, 0.5, 0, 0.6, 0, 0, 0, 0, 0},
                            {"forest", 0.05, 0, 0.35, 0.5, 0, 0, 0, 0.4, 0, 1, 0.8},   // N2-06
                            {"buggy", 0.05, 0.6, 0.5, 0.5, 0, 0, 0, 0, 0, 0, 0}};   // N4-04 (engine, rpm and skid set below)
    std::vector<float> buf(2048);
    for (const Stage& sg : stages) {
        AudioState st; st.hum = sg.hum; st.engine = sg.engine; st.wind = sg.wind; st.windTone = sg.tone; st.rain = sg.rain; st.lava = sg.lava; st.surf = sg.surf; st.birds = sg.birds; st.breath = sg.breath; st.leaves = sg.leaves;
        if (std::string(sg.name) == "buggy") { st.engineRpm = 0.7; st.skid = 0.4; st.thud = 0.8; }
        int frames = 0;
        while (frames < sr * 3) {
            if ((frames / 2048) % 6 == 2 && sg.step) st.step = sg.step;
            if (frames == 0 && sg.engine == 0 && sg.lava == 0) st.beep = 1;
            synth.render(buf.data(), (int)buf.size(), sr, st);
            for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32000)); }
            frames += (int)buf.size();
        }
        printf("audio stage %-14s ok\n", sg.name);
    }
    FILE* f = fopen("shots/tests/audio_test.wav", "wb");
    if (f) {
        uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
        fwrite("RIFF", 1, 4, f); fwrite(&riffLen, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtLen, 4, 1, f); fwrite(&fmtTag, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f); fwrite(&blockAlign, 2, 1, f); fwrite(&bits, 2, 1, f);
        fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
    }
    printf("audio: %zu samples, %d non-finite, %d clipped -> shots/audio_test.wav\n", pcm.size(), nan, clip);
}

// B-310: `bench forest`: the forest scene at 4x rendered 150 times, for a profiler (`sample vesperis_test 5` on macOS)
static int runBenchForest() {
    setFramebufferScale(4);
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    if (!setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv, sys, nb, t, -4)) return 1;
    Framebuffer fb; Input in;
    double t0 = nowSec();
    for (int i = 0; i < 150; i++) { sv.update(1.0 / 30, in, t + i * 0.033, false); sv.render(fb, t + i * 0.033, nb.stars, sr, 1.0); fb.mush(2); }
    printf("forest 4x x150: %.2f ms/frame (sections sky %.1f warm %.1f terrain %.1f objects %.1f flora %.1f rest %.1f; trees %.1f ms)\n", (nowSec() - t0) * 1000 / 150,
           sv.lastRenderMs[0], sv.lastRenderMs[1], sv.lastRenderMs[2], sv.lastRenderMs[3], sv.lastRenderMs[4], sv.lastRenderMs[5], sv.lastFloraMs[0]);
    return 0;
}

static int runBench(bool check) {
    Game game;
    double surface2x = 0, space2x = 0, descentFirst = 0, runWorst = 0;
    game.savePrefix = "shots/tests/bench_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/bench_guide.txt";
    Input in;
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 60); in.newFrame(); in.down[key] = false; };
    auto bench = [&](const char* name, int frames) {
        double t0 = nowSec();
        for (int i = 0; i < frames; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
        double dt = (nowSec() - t0) / frames * 1000;
        printf("%-12s %.2f ms/frame\n", name, dt);
        return dt;
    };
    press(KEY_ENTER);
    for (int s = 1; s <= 4; s++) { game.settings.renderScale = s; game.applySettings(); double ms = bench(("space " + std::to_string(s) + "x").c_str(), 60); if (s == 2) space2x = ms; }
    game.settings.renderScale = 1; game.applySettings();
    press(KEY_C);   // G-02: no R: it picked a random lit site seeded by the clock, whose tiles took 0.4-3.2 s by luck (KI-338); the default site is the measure
    {   // O6-03: a player looks at the map for a moment; the drainage tiles of the site are computed meanwhile (up to 3 s here)
        int waited = 0; double tw0 = nowSec();
        for (; nowSec() - tw0 < 3.0 && !game.testDrainageReady(); waited++) { game.frame(in, 1.0 / 60); in.newFrame(); }
        printf("landing map  %.2f s (%d frames) until the drainage tiles were ready (%s, %d tiles held)\n", nowSec() - tw0, waited, game.testBodyTypes().c_str(), drainageTilesHeld());
    }
    {
        // the first descent frames used to hitch (KI-009): time them
        in.pressed[KEY_ENTER] = true; in.down[KEY_ENTER] = true;
        double worst = 0, first = 0;
        for (int i = 0; i < 60; i++) {
            double t0 = nowSec();
            game.frame(in, 1.0 / 60);
            double ms = (nowSec() - t0) * 1000;
            in.newFrame(); in.down[KEY_ENTER] = false;
            if (i == 0) first = ms;
            worst = std::max(worst, ms);
        }
        printf("descent      first frame %.1f ms, worst of 60: %.1f ms\n", first, worst);
        { const double* ms = game.testRenderMs(); printf("  last frame's render sections: palette+sky %.1f, floor+warm %.1f, terrain %.1f, reflections..objects %.1f, flora %.1f, rest %.1f ms\n", ms[0], ms[1], ms[2], ms[3], ms[4], ms[5]); }
        descentFirst = first;
    }
    for (int i = 0; i < 60 * 7; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
    {   // M7-01: the first surface frames after landing (the caches were warmed during the descent)
        double first = 0, worst = 0;
        for (int i = 0; i < 30; i++) { double t0 = nowSec(); game.frame(in, 1.0 / 60); double ms = (nowSec() - t0) * 1000; in.newFrame(); if (i == 0) first = ms; worst = std::max(worst, ms); }
        printf("landing      first frame %.1f ms, worst of 30: %.1f ms\n", first, worst);
    }
    for (int s = 4; s >= 2; s--) {
        game.settings.renderScale = s; game.applySettings();
        double ms = bench(("surface " + std::to_string(s) + "x").c_str(), 40);
        if (s == 2) surface2x = ms;
        const double* r = game.testRenderMs();
        printf("             sections: sky %.1f, warm %.1f, terrain %.1f, objects %.1f, flora %.1f, rest %.1f ms\n", r[0], r[1], r[2], r[3], r[4], r[5]);
    }
    game.settings.renderScale = 1; game.applySettings();
    bench("surface", 120);
    writePNG("shots/tests/bench_surface.png", game.output(), FBW, FBH);
    printf("  state=%d %s\n", (int)game.state, game.testStatus().c_str());
    in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true;
    {   // M7-01: a sprint of four seconds; the worst frame shows whether the caches stall
        double worst = 0, sum = 0;
        for (int i = 0; i < 240; i++) { double t0 = nowSec(); game.frame(in, 1.0 / 60); double ms = (nowSec() - t0) * 1000; in.newFrame(); worst = std::max(worst, ms); sum += ms; }
        printf("surface-run  %.2f ms/frame, worst %.1f ms, %d cells delivered by the prefetch worker\n", sum / 240, worst, game.testAheadCells());
        runWorst = worst;
    }
    in.down[KEY_W] = false; in.down[KEY_LEFT_SHIFT] = false;
    in.mouseDx = 300; game.frame(in, 1.0 / 60); in.newFrame();
    bench("surface2", 120);
    writePNG("shots/tests/bench_surface2.png", game.output(), FBW, FBH);
    in.mouseDx = 300; game.frame(in, 1.0 / 60); in.newFrame();
    bench("surface3", 120);
    writePNG("shots/tests/bench_surface3.png", game.output(), FBW, FBH);
    for (int i = 0; i < 3; i++) press(KEY_T);
    for (int i = 0; i < 60 * 6; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
    writePNG("shots/tests/bench_surface4.png", game.output(), FBW, FBH);
    // M7-02 map cache counters
    const SpaceRenderer::MapStats& ms = game.testMapStats();
    printf("maps         %d generated (%.0f ms each on the worker), %d hits, %d evicted, %zu held (cap 24)\n", ms.generated, ms.generated ? game.testGenMicros() / 1000.0 / ms.generated : 0.0, ms.hits, ms.evicted, game.testMapsHeld());
    if (!check) return 0;
    // M7-04 performance budgets, generous enough for a CI runner
    int fails = 0;
    auto budget = [&](const char* name, double ms, double limit) { bool ok = ms <= limit; printf("budget %-14s %.1f ms (limit %.0f) %s\n", name, ms, limit, ok ? "ok" : "EXCEEDED"); if (!ok) fails++; };
    {   // N2-04: a forest view at 2x through the surface renderer alone
        int saved = g_testScale;
        setFramebufferScale(2);
        SpaceRenderer sr2; StarNeighborhood nb2; StarSystem sys2; SurfaceView sv2; double t2 = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv2, sys2, nb2, t2, -4)) {
            Framebuffer fb2; Input in2;
            for (int i = 0; i < 5; i++) { sv2.update(1.0 / 30, in2, t2, false); sv2.render(fb2, t2, nb2.stars, sr2, 1.0); }
            double t0 = nowSec();
            for (int i = 0; i < 30; i++) { sv2.update(1.0 / 30, in2, t2 + i * 0.033, false); sv2.render(fb2, t2 + i * 0.033, nb2.stars, sr2, 1.0); fb2.mush(2); }
            double ms = (nowSec() - t0) * 1000 / 30;
            printf("forest 2x    %.2f ms/frame (%d trees: near %d mid %d far %d, %d hidden behind canopy, %d flora points; flora sections trees %.1f, logs+undergrowth %.1f, meadows/shores %.1f, deserts %.1f ms)\n", ms, sv2.lastTreesDrawn,
                   sv2.lastNearTrees, sv2.lastMidTrees, sv2.lastFarTrees, sv2.lastTreesHidden, sv2.lastFloraPoints, sv2.lastFloraMs[0], sv2.lastFloraMs[1], sv2.lastFloraMs[2], sv2.lastFloraMs[3]);
            saveFB(fb2, "shots/tests/bench_forest.png");
            budget("forest 2x", ms, 14);   // N2-04: a tropical giant forest, the densest case (about 12 ms on an M4; CI headroom)
        }
        setFramebufferScale(4);   // the same forest at 4x (the default picture since B-310)
        { SpaceRenderer sr4; StarNeighborhood nb4; StarSystem sys4; SurfaceView sv4; double t4 = 0;
          if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv4, sys4, nb4, t4, -4)) {
            Framebuffer fb4; Input in4;
            for (int i = 0; i < 5; i++) { sv4.update(1.0 / 30, in4, t4, false); sv4.render(fb4, t4, nb4.stars, sr4, 1.0); }
            double t0 = nowSec();
            for (int i = 0; i < 30; i++) { sv4.update(1.0 / 30, in4, t4 + i * 0.033, false); sv4.render(fb4, t4 + i * 0.033, nb4.stars, sr4, 1.0); fb4.mush(2); }
            double ms = (nowSec() - t0) * 1000 / 30;
            printf("forest 4x    %.2f ms/frame (flora sections trees %.1f, logs+undergrowth %.1f, meadows/shores %.1f, deserts %.1f ms; %d trees listed, %d visits over the bands)\n", ms, sv4.lastFloraMs[0], sv4.lastFloraMs[1], sv4.lastFloraMs[2], sv4.lastFloraMs[3], sv4.lastTreesListed, sv4.lastTreesVisited);
            budget("forest 4x", ms, 24);
        } }
        setFramebufferScale(saved);
    }
    {   // N3: a herd at 18 m at 1x through the surface renderer: the cost of updating and drawing life
        SpaceRenderer sr3; StarNeighborhood nb3; StarSystem sys3; SurfaceView sv3; double t3 = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, sv3, sys3, nb3, t3, -3)) {
            Framebuffer fb3; Input in3;
            double upd = 0, drw = 0; int n = 0; for (const Critter& c : sv3.critters) if (c.herd == 0) n++;
            for (int i = 0; i < 20; i++) { sv3.update(1.0 / 30, in3, t3 + i * 0.033, false); sv3.render(fb3, t3 + i * 0.033, nb3.stars, sr3, 1.0); if (i >= 5) { upd += sv3.lastLifeMs[0]; drw += sv3.lastLifeMs[1]; } }
            printf("herd 1x      update %.2f ms, draw %.2f ms per frame (%d in the herd, %zu creatures at the site)\n", upd / 15, drw / 15, n, sv3.critters.size());
            budget("herd draw 1x", drw / 15, 1.0);
        }
    }
    {   // N4-02: the driving frame at 2x (the game, seat view, 60 frames at full throttle)
        game.settings.renderScale = 2; game.applySettings();
        if (game.testDriveSetup()) {
            Input in4; in4.down[KEY_W] = true;
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in4, 1.0 / 30); in4.newFrame(); in4.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("buggy 2x     %.2f ms/frame, worst %.1f ms (%s)\n", sum / 60, worst, game.testBuggyInfo().c_str());
            writePNG("shots/tests/bench_buggy.png", game.output(), FBW, FBH);
            budget("buggy 2x", sum / 60, 14);
        }
        if (game.testFlySetup(200)) {   // R-403: the drone 200 m up at full thrust, the camera tilted down 30 deg: the ground from the air
            Input in5; in5.down[KEY_W] = true; game.testSetPitch(-30 * DEG);
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in5, 1.0 / 30); in5.newFrame(); in5.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("drone 2x     %.2f ms/frame, worst %.1f ms (%s)\n", sum / 60, worst, game.testDroneInfo().c_str());
            writePNG("shots/tests/bench_drone.png", game.output(), FBW, FBH);
            budget("drone 2x", sum / 60, 16);
        }
        game.settings.renderScale = 4; game.applySettings();   // and at 4x, the default picture (B-310)
        if (game.testDriveSetup()) {
            Input in4; in4.down[KEY_W] = true;
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in4, 1.0 / 30); in4.newFrame(); in4.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("buggy 4x     %.2f ms/frame, worst %.1f ms\n", sum / 60, worst);
            budget("buggy 4x", sum / 60, 24);
        }
        game.settings.renderScale = 1; game.applySettings();
    }
    {   // O3: a frame inside a belt at 2x, parked beside a rock (the rock field is the cost)
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sy; sy.generate(s);
                if (sy.belts.empty()) continue;
                game.settings.renderScale = 2; game.applySettings();
                game.testParkAtBelt(s, 0);
                Input in5;
                for (int i = 0; i < 10; i++) { game.frame(in5, 1.0 / 60); in5.newFrame(); }
                in5.pressed[KEY_X] = true; in5.down[KEY_X] = true; game.frame(in5, 1.0 / 60); in5.newFrame(); in5.down[KEY_X] = false;
                double sum = 0;
                for (int i = 0; i < 60; i++) { double t0 = nowSec(); game.frame(in5, 1.0 / 60); in5.newFrame(); sum += (nowSec() - t0) * 1000; }
                printf("belt 2x      %.2f ms/frame (%s)\n", sum / 60, game.testDebugInfo().c_str());
                writePNG("shots/tests/bench_belt.png", game.output(), FBW, FBH);
                budget("belt 2x", sum / 60, 12);
                game.settings.renderScale = 1; game.applySettings();
                found = true;
            }
    }
    budget("space 2x", space2x, 12);
    budget("surface 2x", surface2x, 16);
    budget("descent first", descentFirst, 80);   // O6-03: the four rings' first fill reads the drainage (12000 samples, +15 ms); the frame is under the fade-in
    budget("sprint worst", runWorst, 40);
    return fails;
}

// ---- moon sky test -------------------------------------------------------------
// M1-08: find a partial solar eclipse from a moon of a gas giant and render it.
static void renderEclipse(SpaceRenderer& sr, StarNeighborhood& nb, StarSystem& sys, int bi) {
    const Body& b = sys.bodies[bi];
    // site at the sub-parent point: the parent hangs overhead and the sun passes behind it
    double t0 = 4000.0;
    Mat3 frame = sys.bodyFrame(bi, t0);
    Vec3 toParent = frame * normalize(sys.bodyPos(b.parent, t0) - sys.bodyPos(bi, t0));
    double plat, plon;
    StarSystem::latLonFromBody(toParent, plat, plon);
    SurfaceSite probe;
    probe.init(&sys, bi, clampd(plat, -70 * DEG, 70 * DEG), plon, t0);
    double bestT = t0, bestErr = 1e9, period = b.orbitPeriod;
    for (int i = 0; i < 2400; i++) {
        double t = t0 + period * 3.0 * i / 2400.0;
        SunInfo si = probe.sun(t);
        if (si.altitude < 5 * DEG) continue;
        double err = std::fabs(si.eclipse - 0.85);
        if (err < bestErr) { bestErr = err; bestT = t; }
    }
    SurfaceView sv;
    sv.init(&sys, bi, probe.lat0, probe.lon0, bestT);
    SunInfo si = sv.site.sun(bestT);
    sv.player.yaw = si.azimuth;
    sv.player.pitch = si.altitude - 4 * DEG;
    nb.update(sv.site.worldPos(bestT, 0, 0, 0));
    Input in;
    sv.update(0.016, in, bestT, false);
    Framebuffer fb;
    sv.render(fb, bestT, nb.stars, sr, 1.0);
    fb.mush(2);
    saveFB(fb, "shots/tests/eclipse.png");
    printf("eclipse on %s: coverage %.2f at t=%.0f, sun alt %.1f, sky brightness %.2f -> shots/eclipse.png\n", b.name.c_str(), si.eclipse, bestT, si.altitude / DEG, sv.env.skyBrightness);
}

static void renderMoonSky() {
    SpaceRenderer sr;
    StarNeighborhood nb;
    int shots = 0;
    bool eclipseDone = false;
    for (int64_t x = 150; x < 320 && shots < 3; x++)
        for (int64_t z = 20; z < 140 && shots < 3; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && shots < 3; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.parent < 0 || !PLANET_TYPES[b.type].landable) continue;
                const Body& par = sys.bodies[b.parent];
                if (par.type != PT_GASGIANT && shots < 2) continue;   // first two: gas giant parents
                if (b.orbitRadiusKm / par.radiusKm > 9) continue;
                // land at the sub-planet point: the parent is at the zenith for a locked moon;
                // choose a spot 60 degrees away so it sits mid-sky.
                double t = 4000.0;
                Mat3 frame = sys.bodyFrame(bi, t);
                Vec3 toParent = frame * normalize(sys.bodyPos(b.parent, t) - sys.bodyPos(bi, t));
                double plat, plon;
                StarSystem::latLonFromBody(toParent, plat, plon);
                double lat = clampd(plat, -60 * DEG, 60 * DEG), lon = plon - 55 * DEG;
                SurfaceView sv;
                sv.init(&sys, bi, lat, lon, t);
                warmMaps(sr, sys, b.parent);
                // aim at the parent
                Mat3 L = sv.site.localFrame(t);
                Vec3 obs = sv.site.worldPos(t, 0, 0, 0.002);
                Vec3 dl = L * normalize(sys.bodyPos(b.parent, t) - obs);
                sv.player.yaw = std::atan2(dl.x, dl.z);
                sv.player.pitch = std::asin(dl.y) - 8 * DEG;
                nb.update(obs);
                Input in;
                sv.update(0.016, in, t, false);
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                std::string fn = "shots/tests/moonsky_" + std::to_string(shots) + ".png";
                saveFB(fb, fn.c_str());
                double ang = std::asin(par.radiusKm / b.orbitRadiusKm) / DEG;
                printf("moon %s (%s) of %s (%s): parent angular radius %.1f deg, alt %.1f, sunAlt %.1f\n", b.name.c_str(), PLANET_TYPES[b.type].name,
                       par.name.c_str(), PLANET_TYPES[par.type].name, ang, std::asin(dl.y) / DEG, sv.env.sun.altitude / DEG);
                if (shots == 0) {
                    // M5-03: the same sky a quarter of an hour later: close moons visibly move
                    double t2 = t + 900.0;
                    sv.init(&sys, bi, lat, lon, t2);
                    sv.player.yaw = std::atan2(dl.x, dl.z); sv.player.pitch = std::asin(dl.y) - 8 * DEG;
                    sv.update(0.016, in, t2, false);
                    Framebuffer fb2;
                    sv.render(fb2, t2, nb.stars, sr, 1.0);
                    fb2.mush(2);
                    saveFB(fb2, "shots/tests/moonsky_0_later.png");
                    Mat3 L2 = sv.site.localFrame(t2);
                    Vec3 dl2 = L2 * normalize(sys.bodyPos(b.parent, t2) - sv.site.worldPos(t2, 0, 0, 0.002));
                    printf("  15 min later: parent alt %.1f -> %.1f, az %.1f -> %.1f (orbit %.1f h) -> shots/moonsky_0_later.png\n", std::asin(dl.y) / DEG, std::asin(dl2.y) / DEG,
                           wrap2pi(std::atan2(dl.x, dl.z)) / DEG, wrap2pi(std::atan2(dl2.x, dl2.z)) / DEG, b.orbitPeriod / 3600);
                }
                shots++;
                if (!eclipseDone && par.type == PT_GASGIANT) { renderEclipse(sr, nb, sys, bi); eclipseDone = true; }
            }
        }
    // M5-04 / M5-02: a moon of a ringed planet, and a moon of a substellar object at night
    bool ringsDone = false, subDone = false;
    for (int64_t x = 150; x < 340 && !(ringsDone && subDone); x++)
        for (int64_t z = 20; z < 160 && !(ringsDone && subDone); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && !(ringsDone && subDone); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.parent < 0 || !PLANET_TYPES[b.type].landable) continue;
                const Body& par = sys.bodies[b.parent];
                Vec3 orbN(std::sin(b.orbitIncl) * std::cos(b.orbitNode), std::cos(b.orbitIncl), std::sin(b.orbitIncl) * std::sin(b.orbitNode));
                bool wantRings = !ringsDone && par.rings && b.orbitRadiusKm / par.radiusKm < 12 && std::fabs(dot(par.spinAxis, Vec3(0, 1, 0))) < 0.94;   // rings open, not edge-on
                bool wantSub = !subDone && par.type == PT_SUBSTELLAR && b.orbitRadiusKm / par.radiusKm < 14;
                if (!wantRings && !wantSub) continue;
                // sweep the time so the parent stands mid-sky and, for the substellar case, the sun is down
                double tBest = -1; double bestScore = -1e9; double latB = 0, lonB = 0;
                for (int k = 0; k < 96; k++) {
                    double t = 3000.0 + k * b.orbitPeriod / 96.0 * (wantSub ? 1.0 : 0.5);
                    Mat3 frame = sys.bodyFrame(bi, t);
                    Vec3 toParent = frame * normalize(sys.bodyPos(b.parent, t) - sys.bodyPos(bi, t));
                    double plat, plon;
                    StarSystem::latLonFromBody(toParent, plat, plon);
                    double lat = clampd(plat, -60 * DEG, 60 * DEG), lon = plon - 50 * DEG;
                    SurfaceSite site; site.init(&sys, bi, lat, lon, t);
                    double sunAlt = site.sun(t).altitude / DEG;
                    double phase = 0.5 * (1 + dot(normalize(sys.star.pos - sys.bodyPos(b.parent, t)), normalize(sys.bodyPos(bi, t) - sys.bodyPos(b.parent, t))));
                    double opening = std::fabs(dot(normalize(sys.bodyPos(b.parent, t) - sys.bodyPos(bi, t)), par.spinAxis));   // sine of the ring tilt toward us
                    double score = wantSub ? -sunAlt : (sunAlt < 0 ? -1000 : 0) + 100 * phase + 80 * opening;
                    if (score > bestScore) { bestScore = score; tBest = t; latB = lat; lonB = lon; }
                }
                if (tBest < 0) continue;
                double t = tBest;
                SurfaceView sv;
                sv.init(&sys, bi, latB, lonB, t);
                warmMaps(sr, sys, b.parent);
                Mat3 L = sv.site.localFrame(t);
                Vec3 obs = sv.site.worldPos(t, 0, 0, 0.002);
                Vec3 dl = L * normalize(sys.bodyPos(b.parent, t) - obs);
                sv.player.yaw = std::atan2(dl.x, dl.z);
                sv.player.pitch = std::asin(dl.y) - 6 * DEG;
                nb.update(obs);
                Input in;
                sv.update(0.016, in, t, false);
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                const char* fn = wantSub ? "shots/tests/moonsky_substellar.png" : "shots/tests/moonsky_rings.png";
                saveFB(fb, fn);
                if (!wantSub) {
                    // the same view rendered by the space renderer (no sky), to compare the ring drawing paths
                    Framebuffer fs;
                    SpaceContext c;
                    c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = obs;
                    c.cam = sv.testCamWorld(t);
                    c.bankBodyA = b.parent;
                    sr.setupPalette(fs, &sys, b.parent, -1, 1.0);
                    sr.render(fs, c);
                    fs.mush(2);
                    saveFB(fs, "shots/tests/moonsky_rings_space.png");
                    double opening = std::fabs(dot(normalize(sys.bodyPos(b.parent, t) - obs), par.spinAxis));
                    printf("  ring opening %.1f deg, ring %.2f-%.2f R -> shots/moonsky_rings_space.png\n", std::asin(opening) / DEG, par.ringInner, par.ringOuter);
                }
                printf("%s: moon %s (%s) of %s (%s), parent alt %.1f, sun alt %.1f, night glow %.2f -> %s\n", wantSub ? "substellar" : "rings", b.name.c_str(), PLANET_TYPES[b.type].name,
                       par.name.c_str(), PLANET_TYPES[par.type].name, std::asin(dl.y) / DEG, sv.env.sun.altitude / DEG, sv.env.moonLight, fn);
                if (wantSub) subDone = true; else ringsDone = true;
            }
        }
    // M5-04: the rings of a ringed world seen from its own surface, by day and by night
    bool surfDone = false;
    for (int64_t x = 150; x < 340 && !surfDone; x++)
        for (int64_t z = 20; z < 160 && !surfDone; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && !surfDone; bi++) {
                const Body& b = sys.bodies[bi];
                if (!b.rings || !PLANET_TYPES[b.type].landable || b.parent >= 0) continue;
                for (int k = 0; k < 2; k++) {
                    double lat = 28 * DEG, lon = 1.0;
                    SurfaceView sv;
                    sv.init(&sys, bi, lat, lon, 1000.0);
                    double t = findTime(sv.site, k == 0 ? 30 * DEG : -40 * DEG, 1000.0);
                    sv.init(&sys, bi, lat, lon, t);
                    // look toward the equator (the ring's arc), a little up
                    sv.player.yaw = lat > 0 ? PI : 0; sv.player.pitch = 40 * DEG;
                    Vec3 obs = sv.site.worldPos(t, 0, 0, 0.002);
                    nb.update(obs);
                    Input in;
                    sv.update(0.016, in, t, false);
                    Framebuffer fb;
                    sv.render(fb, t, nb.stars, sr, 1.0);
                    fb.mush(2);
                    std::string fn = k == 0 ? "shots/tests/rings_surface_day.png" : "shots/tests/rings_surface_night.png";
                    saveFB(fb, fn.c_str());
                    printf("rings from the surface of %s (%s, tilt %.0f deg, ring %.2f-%.2f R): sun alt %.1f -> %s\n", b.name.c_str(), PLANET_TYPES[b.type].name, b.axialTilt / DEG, b.ringInner, b.ringOuter, sv.env.sun.altitude / DEG, fn.c_str());
                }
                surfDone = true;
            }
        }
}

// M5-01: a surface with two suns up (part of `moonsky`)
static void renderBinarySurface(SpaceRenderer& sr, StarNeighborhood& nb) {
    for (int64_t x = 150; x < 340; x++)
        for (int64_t z = 20; z < 160; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            if (sys.companion < 0) continue;
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (!PLANET_TYPES[b.type].landable || b.parent >= 0) continue;
                // sweep sites and times for both suns above 12 degrees with the companion a visible disc
                double bestScore = -1e9, tB = 0, latB = 0, lonB = 0;
                for (int k = 0; k < 160; k++) {
                    double t = 2000.0 + k * 1811.0;
                    double lat = 20 * DEG, lon = k * 0.7;
                    SurfaceSite site; site.init(&sys, bi, lat, lon, t);
                    SunInfo s1 = site.sun(t), s2;
                    if (!site.sun2(t, s2)) continue;
                    if (s2.angularRadius < 0.08 * DEG) continue;
                    double score = std::min(s1.altitude, s2.altitude) / DEG;
                    if (s1.altitude < 12 * DEG || s2.altitude < 12 * DEG) score -= 100;
                    if (score > bestScore) { bestScore = score; tB = t; latB = lat; lonB = lon; }
                }
                if (bestScore < 0) continue;
                SurfaceView sv;
                sv.init(&sys, bi, latB, lonB, tB);
                SunInfo s1 = sv.site.sun(tB), s2; sv.site.sun2(tB, s2);
                // look between the two suns
                Vec3 mid = normalize(s1.dirLocal + s2.dirLocal);
                sv.player.yaw = std::atan2(mid.x, mid.z); sv.player.pitch = std::asin(mid.y) - 12 * DEG;
                Vec3 obs = sv.site.worldPos(tB, 0, 0, 0.002);
                nb.update(obs);
                Input in;
                sv.update(0.016, in, tB, false);
                Framebuffer fb;
                sv.render(fb, tB, nb.stars, sr, 1.0);
                fb.mush(2);
                saveFB(fb, "shots/tests/binary_surface.png");
                printf("binary surface: %s (%s) of %s: primary alt %.0f az %.0f, companion %s alt %.0f az %.0f radius %.2f deg, share %.2f -> shots/binary_surface.png\n", b.name.c_str(), PLANET_TYPES[b.type].name,
                       sys.classString().c_str(), s1.altitude / DEG, s1.azimuth / DEG, STAR_CLASSES[sys.bodies[sys.companion].starClass].name, s2.altitude / DEG, s2.azimuth / DEG, s2.angularRadius / DEG, sv.env.sun2Share);
                return;
            }
        }
    printf("binary surface: none found\n");
}

// M7-03 unit tests: frame maths, the sun against analytic cases, noise statistics, name lengths, the Kepler solver
// C-07: the receiver's renders (the unit and `signals wav`): a people's world (its voice, tongue, shards, language and tradition),
// a programme of it through the synth at full clarity with a loop's and a numbers station's repeats as the game does them, a
// natural signal by its seed, and a WAV writer
struct PeopleWorld { bool valid = false; StarSystem sys; int body = -1; Lore lore; Tongue tongue; Voice voice; std::vector<Shard> shards; Language lang; Tradition tradition; int firstText = -1, firstMusic = -1; uint64_t seed = 0; std::string name; };
static bool peopleWorldOf(const Star& s, int bi, PeopleWorld& w) {
    w = PeopleWorld(); w.sys.generate(s);
    if (bi < 0 || bi >= (int)w.sys.bodies.size()) return false;
    const Body& b = w.sys.bodies[bi]; BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) return false;
    w.body = bi; w.seed = b.seed; w.name = b.name;
    w.lore = loreOf(w.sys, b, g); w.tongue = tongueOf(g, w.lore); w.voice = voiceOf(g, w.lore, w.tongue);
    shardsOf(w.sys, b, g, w.lore, SHARDS_PER_WORLD, w.shards); languageOf(w.lore, w.shards, w.tongue, w.lang); w.tradition = traditionOf(g, w.lore);
    for (int i = 0; i < (int)w.shards.size(); i++) { if (w.shards[i].music) { if (w.firstMusic < 0) w.firstMusic = i; } else if (w.firstText < 0) w.firstText = i; }
    w.valid = w.firstText >= 0 && w.firstMusic >= 0;
    return w.valid;
}
struct RenderStats { double rms = 0, peak = 0; int nan = 0, clip = 0; };
static RenderStats renderReceiver(AudioSynth& synth, AudioState& st, double seconds, std::vector<int16_t>* pcm, const std::function<void(double)>& between = nullptr) {
    const int sr = 22050; std::vector<float> buf(2048); RenderStats r; double sum = 0; long cnt = 0;
    for (int frames = 0; frames < sr * seconds; frames += (int)buf.size()) {
        synth.render(buf.data(), (int)buf.size(), sr, st);
        for (float v : buf) { if (!std::isfinite(v)) r.nan++; if (std::fabs(v) >= 0.999f) r.clip++; r.peak = std::max(r.peak, (double)std::fabs(v)); sum += (double)v * v; cnt++; if (pcm) pcm->push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32767)); }
        if (between) between(buf.size() / (double)sr);
    }
    r.rms = std::sqrt(sum / std::max(1L, cnt));
    return r;
}
static RenderStats renderProgramme(const PeopleWorld& w, int p, double seconds, std::vector<int16_t>* pcm, Programme* progOut = nullptr) {
    AudioSynth synth; AudioState st; Programme prog; Speech speech; Piece piece; std::vector<DecodedWord> words;
    int slot = p == RP_MUSIC ? w.firstMusic : w.firstText;
    const Shard& sh = w.shards[slot];
    if (!sh.music) decodeShard(sh, w.lang, w.tongue, SHARDS_PER_WORLD, words);
    programmeFor(w.seed, slot, sh.music, sh.year, w.voice, w.tongue, words, prog, p);
    st.radar = 1; st.radarSignal = 1; st.radarKind = SIG_PEOPLE; st.radarVoice = prog.kind; st.radarSeed = prog.seed; st.radio = true; st.master = 0.8;
    if (prog.kind == RP_MUSIC) { pieceOf(w.tradition, sh, piece); st.piece = &piece; st.tradition = &w.tradition; st.pieceSpeed = prog.pieceSpeed; st.pieceStart = true; }
    else if (!prog.machine) { speechOf(prog.voice, prog.words, sh.seed, speech); st.speech = &speech; st.voice = &prog.voice; st.speechStart = true; }
    int rep = 0; double gapT = -1;
    RenderStats r = renderReceiver(synth, st, seconds, pcm, [&](double dt) {
        if (st.speech && st.speechDone && prog.repeats > 1 && rep < prog.repeats - 1) {
            gapT = gapT < 0 ? 0 : gapT + dt;
            if (gapT >= prog.gap) { rep++; gapT = -1; std::vector<DecodedWord> ww; programmeRepeatWords(prog, rep, w.tongue, ww); st.speech = nullptr; speechOf(prog.voice, ww, sh.seed ^ (uint64_t)(rep * 0x9E37), speech); st.speech = &speech; st.speechStart = true; st.speechDone = false; }
        }
    });
    if (progOut) *progOut = prog;
    return r;
}
static RenderStats renderKind(int kind, uint64_t seed, double pulseHz, double seconds, std::vector<int16_t>* pcm) {
    AudioSynth synth; AudioState st;
    st.radar = 1; st.radarSignal = kind < 0 ? 0 : 1; st.radarKind = kind; st.radarSeed = seed; st.radarPulseHz = pulseHz; st.master = 0.8;
    return renderReceiver(synth, st, seconds, pcm);
}
static bool writeWav16(const std::string& fn, const std::vector<int16_t>& pcm, int sr) {
    FILE* f = fopen(fn.c_str(), "wb"); if (!f) return false;
    uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
    fwrite("RIFF", 1, 4, f); fwrite(&riffLen, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtLen, 4, 1, f); fwrite(&fmtTag, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f); fwrite(&blockAlign, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
    return true;
}
// the first people's world heard from home (the signals' order: the nearest)
static bool firstPeopleWorld(PeopleWorld& w, Signal* sigOut = nullptr) {
    Vec3 obs((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
    std::vector<Signal> far; signalsNear(obs, far);
    for (const Signal& sg : far) {
        if (sg.kind != SIG_PEOPLE) continue;
        Star named = sg.star; starInSector(sg.star.sx, sg.star.sy, sg.star.sz, named, true);
        if (peopleWorldOf(named, sg.body, w)) { if (sigOut) *sigOut = sg; return true; }
    }
    return false;
}

static int testUnit() {
    int fails = 0;
    auto check = [&](const char* name, bool ok, const std::string& detail = "") { printf("  %-34s %s %s\n", name, ok ? "ok" : "FAIL", detail.c_str()); if (!ok) fails++; };
    {   // O6-02: the analytic gradient of gnoise3d matches central differences and its value is gnoise3's
        double worst = 0, worstV = 0; Rng r(0x616);
        for (int i = 0; i < 200; i++) {
            double x = r.range(-50, 50), y = r.range(-50, 50), z = r.range(-50, 50); uint64_t sd = r.next();
            Vec3 g; double v = gnoise3d(x, y, z, sd, g);
            worstV = std::max(worstV, std::fabs(v - gnoise3(x, y, z, sd)));
            const double e = 1e-5;
            Vec3 fd((gnoise3(x + e, y, z, sd) - gnoise3(x - e, y, z, sd)) / (2 * e), (gnoise3(x, y + e, z, sd) - gnoise3(x, y - e, z, sd)) / (2 * e), (gnoise3(x, y, z + e, sd) - gnoise3(x, y, z - e, sd)) / (2 * e));
            worst = std::max(worst, length(fd - g));
        }
        check("gnoise3d value equals gnoise3", worstV < 1e-12, "worst " + std::to_string(worstV));
        check("gnoise3d gradient by finite differences", worst < 1e-5, "worst " + std::to_string(worst));
    }
    {   // B-404: the walked rings (4, 16 and 64 m) decide the same materials: a material follows no faded value, so no
        // patch ends at a ring's edge in front of the explorer (the europan stain, the lava tongues, the cliffs' rock).
        // The felisian and desert allowances are the drainage's lines, a cell and a half wide at every scale (KI-341)
        struct Want { int type; double max4, max16; };
        static const Want wants[] = {{PT_EUROPAN, 0.05, 0.05}, {PT_VOLCANIC, 0.2, 0.2}, {PT_DESERT, 0.6, 5.0}, {PT_FELISIAN, 0.6, 6.0}};
        for (const Want& w : wants) {
            StarSystem sys; int bi;
            if (!firstBodyOfType(w.type, sys, bi)) { check("B-404 materials agree across the rings", false, std::string(PLANET_TYPES[w.type].name) + ": no body"); continue; }
            std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x4A, 2, sites);
            double worst[2] = {0, 0};
            for (auto& st : sites) { double o[2]; materialAgreement(sys, bi, st.first, st.second, 1500.0, 100.0, o); worst[0] = std::max(worst[0], o[0]); worst[1] = std::max(worst[1], o[1]); }
            char buf[200]; snprintf(buf, sizeof buf, "%s (%s): 4/16 %.2f%% of %.2f, 16/64 %.2f%% of %.2f", PLANET_TYPES[w.type].name, sys.bodies[bi].name.c_str(), worst[0], w.max4, worst[1], w.max16);
            check("B-404 materials agree across the rings", worst[0] <= w.max4 && worst[1] <= w.max16, buf);
        }
    }
    {   // O6-02: the relief spectrum is deterministic and its analytic slope tracks the finite difference of its height
        Body b; b.type = PT_FELISIAN; b.seed = 0xC0FFEEULL; b.radiusKm = 5000; b.tempK = 285; b.color = RGB(0.3f, 0.5f, 0.4f);
        BodyGen g = BodyGen::make(b);
        Rng r(0x617); double sumErr = 0, sumFd = 0; bool det = true;
        for (int i = 0; i < 24; i++) {
            Vec3 u = normalize(Vec3(r.sym(1), r.sym(1), r.sym(1)));
            Relief r1 = reliefAt(g, u, 1400, 0.5, 16), r2 = reliefAt(g, u, 1400, 0.5, 16);
            if (r1.h != r2.h || r1.slope != r2.slope || std::fabs(r1.h) > 1400 * 4) det = false;
            Vec3 e = normalize(cross(u, std::fabs(u.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), n = cross(u, e);
            double dm = 2.0 / (g.R * 1000);
            double hx = (reliefAt(g, normalize(u + e * dm), 1400, 0.5, 16).h - reliefAt(g, normalize(u - e * dm), 1400, 0.5, 16).h) / 4.0;
            double hz = (reliefAt(g, normalize(u + n * dm), 1400, 0.5, 16).h - reliefAt(g, normalize(u - n * dm), 1400, 0.5, 16).h) / 4.0;
            double fd = std::sqrt(hx * hx + hz * hz);
            sumErr += std::fabs(fd - r1.slope); sumFd += fd;
        }
        check("relief deterministic and bounded", det);
        check("relief slope tracks finite differences", sumErr < 0.4 * sumFd, "mean error " + std::to_string(sumErr / 24) + " of " + std::to_string(sumFd / 24));
    }
    // N1-01: every material of every type gets a colour, the same for the same seed
    {
        bool ok = true; std::string why;
        for (int ty = 0; ty < PT_COUNT && ok; ty++) {
            Body b; b.type = ty; b.seed = 0xABCD1234ULL + ty; b.radiusKm = 3000; b.tempK = 280;
            b.color = RGB(0.3f + 0.03f * ty, 0.62f, 0.5f);   // R-307: stays under 1 for the 22 types
            BodyGen g1 = BodyGen::make(b), g2 = BodyGen::make(b);
            for (int mm = 0; mm < MAT_COUNT && ok; mm++) {
                const RGB& c = g1.matColor[mm];
                if (!(c.r >= 0 && c.g >= 0 && c.b >= 0 && c.r + c.g + c.b > 0.05 && c.r <= 1.001 && c.g <= 1.001 && c.b <= 1.001)) { ok = false; why = std::string(PLANET_TYPES[ty].name) + "/" + MATERIAL_NAMES[mm]; }
                if (std::fabs(c.r - g2.matColor[mm].r) > 1e-9 || std::fabs(c.g - g2.matColor[mm].g) > 1e-9) { ok = false; why = "not deterministic"; }
            }
        }
        check("material palette per type", ok, why);
        check("noon exposure rule", noonStop(0.4) > 37 && noonStop(0.4) < 38 && noonStop(1.0) == 60 && noonStop(0.0) == 25, std::to_string(noonStop(0.4)));
        RGB rc = materialRampColor(RGB(1, 0.5f, 0), 47);
        check("material ramp hits c at 47", std::fabs(rc.r - 1) < 1e-6 && std::fabs(rc.g - 0.5) < 1e-6, "");
    }
    // lat/lon round trips on the sphere
    {
        double worst = 0;
        for (int i = 0; i < 200; i++) {
            double lat = (i / 200.0 - 0.5) * PI * 0.98, lon = std::fmod(i * 0.7, TAU) - PI;
            double la2, lo2; StarSystem::latLonFromBody(StarSystem::bodyFromLatLon(lat, lon), la2, lo2);
            worst = std::max(worst, std::fabs(la2 - lat) + std::fabs(wrapAngle(lo2 - lon)));
        }
        check("lat/lon round trip", worst < 1e-9, "worst " + std::to_string(worst));
    }
    Star s; StarSystem sys; int bi = -1;
    for (int64_t x = 150; x < 300 && bi < 0; x++)
        for (int64_t z = 20; z < 120 && bi < 0; z++) {
            if (!starInSector(x, 0, z, s)) continue;
            sys.generate(s);
            for (int i = 0; i < (int)sys.bodies.size(); i++) if (PLANET_TYPES[sys.bodies[i].type].landable && sys.bodies[i].parent < 0 && sys.bodies[i].ecc < 0.01) { bi = i; break; }
        }
    check("a landable planet found", bi >= 0);
    if (bi >= 0) {
        // the site frame: local metres -> unit vector -> local metres
        SurfaceSite site; site.init(&sys, bi, 0.4, 1.1, 0.0);
        double worst = 0;
        for (int i = 0; i < 100; i++) {
            double x = (i - 50) * 137.0, z = (i * 71) % 5000 - 2500.0, x2, z2;
            site.localAt(site.unitAt(x, z), x2, z2);
            worst = std::max(worst, std::fabs(x2 - x) + std::fabs(z2 - z));
        }
        check("site local/unit round trip", worst < 1e-6, "worst m " + std::to_string(worst));
        // the sun: at the sub-solar point it stands at the zenith; 30 degrees away along the meridian at 60 degrees
        double t = 5000.0;
        Vec3 sunBody = sys.bodyFrame(bi, t) * normalize(sys.star.pos - sys.bodyPos(bi, t));
        double slat, slon; StarSystem::latLonFromBody(sunBody, slat, slon);
        SurfaceSite sub; sub.init(&sys, bi, slat, slon, t);
        double altZen = sub.sun(t).altitude / DEG;
        double lat2 = slat + 30 * DEG; bool north = lat2 < 85 * DEG; if (!north) lat2 = slat - 30 * DEG;
        SurfaceSite off; off.init(&sys, bi, lat2, slon, t);
        double alt30 = off.sun(t).altitude / DEG, az30 = off.sun(t).azimuth / DEG;
        check("sun at the zenith over the sub-solar point", std::fabs(altZen - 90) < 0.05, "alt " + std::to_string(altZen));
        check("sun at 60 deg 30 deg along the meridian", std::fabs(alt30 - 60) < 0.05, "alt " + std::to_string(alt30) + " az " + std::to_string(az30));
        check("sun due south/north from there", north ? std::fabs(az30 - 180) < 0.5 : (az30 < 0.5 || az30 > 359.5), "az " + std::to_string(az30));
        // local noon: dayFraction 0.5 at the sub-solar longitude
        check("local time at the sub-solar point is noon", std::fabs(sub.sun(t).dayFraction - 0.5) < 0.002, std::to_string(sub.sun(t).dayFraction));
    }
    // Kepler solver residuals
    {
        double worst = 0;
        for (int i = 0; i < 400; i++) {
            double e = (i % 20) / 20.0 * 0.95, M = (i / 20) * 0.31;
            double rOverA; double nu = StarSystem::trueAnomaly(M, e, rOverA);
            double E = 2 * std::atan2(std::sqrt(1 - e) * std::sin(nu / 2), std::sqrt(1 + e) * std::cos(nu / 2));
            double resid = std::fabs(wrapAngle(E - e * std::sin(E) - M));
            worst = std::max(worst, resid);
            worst = std::max(worst, std::fabs(rOverA - (1 - e * std::cos(E))));
        }
        check("Kepler equation residual", worst < 1e-8, "worst " + std::to_string(worst));
        double r0; check("circular orbit is the identity", std::fabs(StarSystem::trueAnomaly(1.234, 0, r0) - 1.234) < 1e-12 && r0 == 1);
    }
    // noise statistics: zero mean, bounded, hash uniform
    {
        double sum = 0, lo = 1e9, hi = -1e9; int n = 0;
        for (int i = 0; i < 20000; i++) { double v = gnoise3(i * 0.371, i * 0.173 + 5, i * 0.091 - 3, 77); sum += v; lo = std::min(lo, v); hi = std::max(hi, v); n++; }
        check("gnoise3 mean near zero", std::fabs(sum / n) < 0.03, std::to_string(sum / n));
        check("gnoise3 within [-1.1, 1.1]", lo > -1.1 && hi < 1.1, std::to_string(lo) + ".." + std::to_string(hi));
        double hs = 0, hs2 = 0;
        for (int i = 0; i < 100000; i++) { double u = unitFromHash(mix64(i * 0x9E3779B97F4A7C15ULL + 11)); hs += u; hs2 += u * u; }
        double mean = hs / 100000, var = hs2 / 100000 - mean * mean;
        check("hash uniform (mean 0.5, var 1/12)", std::fabs(mean - 0.5) < 0.005 && std::fabs(var - 1.0 / 12) < 0.003, std::to_string(mean) + " " + std::to_string(var));
        double a = fbm3(Vec3(1.5, 2.5, 3.5), 9, 5), b = fbm3(Vec3(1.5, 2.5, 3.5), 9, 5);
        check("noise is deterministic", a == b);
    }
    // names: bounded length, letters and spaces only, deterministic
    {
        size_t lo = 99, hi = 0; bool clean = true;
        for (int i = 0; i < 3000; i++) {
            std::string nm = generateName(mix64(i + 1), i % 3);
            lo = std::min(lo, nm.size()); hi = std::max(hi, nm.size());
            for (char ch : nm) if (!(isalpha((unsigned char)ch) || ch == ' ' || ch == '\'')) clean = false;
        }
        check("name lengths 3..16", lo >= 3 && hi <= 16, std::to_string(lo) + ".." + std::to_string(hi));
        check("names are letters, spaces, apostrophes", clean);
        check("names deterministic", generateName(42, 1) == generateName(42, 1));
    }
    // B-205 / R-204: a new landing starts with no buggy, B replaces a buggy left anywhere, the nose camera cannot look back
    if (bi >= 0) {
        SurfaceView sv;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
        check("buggy deploys at the capsule", sv.deployBuggy() && sv.buggy.deployed);
        sv.buggy.unfold = 1; sv.buggy.x += 300; sv.buggy.odometer = 500;   // as if left 300 m away after a drive
        check("B replaces a buggy left far away", sv.deployBuggy() && sv.buggy.odometer == 0 && sv.buggyDist() < 12, "dist " + std::to_string(sv.buggyDist()));
        sv.buggy.unfold = 1; sv.player.x = sv.buggy.x + 1.5; sv.player.z = sv.buggy.z;
        check("get in", sv.toggleBuggy());
        Input in; sv.player.yaw = sv.buggy.heading + PI; sv.player.pitch = -80 * DEG;
        sv.update(1.0 / 60, in, 0.0, true);
        double look = std::fabs(wrapAngle(sv.player.yaw - sv.buggy.heading));
        check("nose camera pans 70 deg at most", look <= SurfaceView::CAM_PAN + 1e-9 && sv.player.pitch >= -SurfaceView::CAM_TILT_DOWN - 1e-9, "look " + std::to_string(look / DEG) + " tilt " + std::to_string(sv.player.pitch / DEG));
        sv.init(&sys, bi, 0.5, 1.2, 0.0);
        check("no buggy on a new landing", !sv.buggy.deployed && !sv.inBuggy);
    }
    // B-303: a new landing clears the trail and the waypoint; a re-anchor carries both to the new frame at the same place
    if (bi >= 0) {
        SurfaceView sv;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        Input in;
        for (int i = 0; i < 40; i++) { sv.player.x += 30; sv.update(1.0 / 30, in, i / 30.0, false); }   // the trail collects points every 25 m
        sv.hasWaypoint = true; sv.wpX = 900; sv.wpZ = -400;
        Vec3 wpU = sv.site.unitAt(sv.wpX, sv.wpZ), trU = sv.site.unitAt(sv.trail[3].first, sv.trail[3].second);
        size_t nTrail = sv.trail.size();
        check("the trail collects points", nTrail >= 30, std::to_string(nTrail));
        sv.player.x = 51e3; sv.update(1.0 / 30, in, 2.0, false);   // beyond 50 km: reanchor
        double wx, wz, tx, tz; sv.site.localAt(wpU, wx, wz); sv.site.localAt(trU, tx, tz);
        check("reanchor keeps the waypoint in place", sv.hasWaypoint && std::fabs(wx - sv.wpX) < 0.05 && std::fabs(wz - sv.wpZ) < 0.05 && std::fabs(sv.wpX) > 1000, "wp " + std::to_string(sv.wpX) + "," + std::to_string(sv.wpZ));
        check("reanchor keeps the trail in place", sv.trail.size() >= nTrail && std::fabs(tx - sv.trail[3].first) < 0.5 && std::fabs(tz - sv.trail[3].second) < 0.5, std::to_string(sv.trail.size()) + " points");   // one more: the point at the new origin
        int epoch = sv.siteEpoch;
        sv.init(&sys, bi, 0.5, 1.2, 0.0);
        check("a new landing clears the trail and the waypoint", sv.trail.empty() && !sv.hasWaypoint && sv.siteEpoch > epoch);
    }
    // B-206: on a thin-atmosphere world in a dust storm the wind stays a wind (it compounded every frame to 1e13 knots)
    {
        Star ts; StarSystem tsys; int tb = -1;
        for (int64_t x = 150; x < 300 && tb < 0; x++)
            for (int64_t z = 20; z < 120 && tb < 0; z++) {
                if (!starInSector(x, 0, z, ts)) continue;
                tsys.generate(ts);
                for (int i = 0; i < (int)tsys.bodies.size(); i++) if (tsys.bodies[i].type == PT_THINATMO) { tb = i; break; }
            }
        check("a thin-atmosphere world found", tb >= 0);
        if (tb >= 0) {
            SurfaceView sv;
            double t0 = 0; double dustAt = 0;
            for (double tt = 0; tt < 4e5 && dustAt < 0.3; tt += 300) { sv.init(&tsys, tb, 0.3, 0.9, tt); dustAt = sv.env.dust; t0 = tt; }
            check("a dust storm found", dustAt >= 0.3, "dust " + std::to_string(dustAt));
            Input in; double worst = 0;
            for (int i = 0; i < 300; i++) { sv.update(1.0 / 30, in, t0 + i / 30.0, true); worst = std::max(worst, sv.env.windKnots); }
            check("storm wind under 200 knots", std::isfinite(worst) && worst < 200 && worst > 5, "worst " + std::to_string(worst));
        }
    }
    // the classic case: a body's rotation period sets its day; seasonOf is bounded
    if (bi >= 0) {
        double mn = 2, mx = -2;
        for (int k = 0; k < 64; k++) { double v = sys.seasonOf(bi, k * sys.bodies[bi].orbitPeriod / 64); mn = std::min(mn, v); mx = std::max(mx, v); }
        check("seasonOf bounded by the tilt", mx <= std::sin(sys.bodies[bi].axialTilt) + 1e-9 && mn >= -std::sin(sys.bodies[bi].axialTilt) - 1e-9);
    }
    // M7-02: the map cache evicts the least recently used map once the cap is reached
    {
        SpaceRenderer sr;
        sr.mapCacheCap = 3;
        int n = 0;
        for (int64_t x = 150; x < 300 && n < 6; x++)
            for (int64_t z = 20; z < 120 && n < 6; z++) {
                Star st;
                if (!starInSector(x, 0, z, st)) continue;
                StarSystem sy; sy.generate(st);
                for (const Body& b : sy.bodies) { if (n >= 6) break; if (b.type == PT_COMPANION) continue; sr.mapFor(b); n++; }
            }
        check("map cache honours its cap", sr.mapsHeld() <= 3 && sr.mapStats.evicted >= 3, std::to_string(sr.mapsHeld()) + " held, " + std::to_string(sr.mapStats.evicted) + " evicted");
    }
    // O0-02 (B-305): 32 banks, and the second world's families have their own
    check("32 palette banks of 2048 intensities", BANKS == 32 && INTEN_MASK == 2047 && INTEN_PER_SHADE == 32 && (BANK_MASK >> BANK_SHIFT) == 31);
    check("body B's sand has its own bank", SpaceRenderer::globeBank(3, MAT_SAND) == 17 && SpaceRenderer::globeBank(2, MAT_SAND) == 13 && SpaceRenderer::globeBank(3, MAT_SNOW) == 18 && SpaceRenderer::globeBank(3, MAT_FOREST) == 16);
    // O0-03 (B-304): a globe whose centre is behind the camera plane still draws its limb
    if (bi >= 0) {
        SpaceRenderer sr; StarNeighborhood nb;
        double t = 3000.0;
        const Body& b = sys.bodies[bi];
        Vec3 bp = sys.bodyPos(bi, t);
        Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
        Vec3 shipPos = bp + dir * (1.15 * b.radiusKm);   // the disc is 121 degrees across from here
        Vec3 toBody = -dir, side = normalize(cross(toBody, Vec3(0, 1, 0)));
        Vec3 fwd = normalize(toBody * std::cos(92 * DEG) + side * std::sin(92 * DEG));   // the centre 92 degrees off the view axis: behind the camera plane, the limb 31 degrees off
        nb.update(shipPos);
        Framebuffer fb; SpaceContext c;
        c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
        c.bankBodyA = bi;
        warmMaps(sr, sys, bi);
        sr.setupPalette(fb, &sys, bi, -1, 1.0);
        sr.render(fb, c);
        int lit = 0;
        for (Pix p : fb.idx) { int bk = bankOf(p); if ((bk == 2 || bk == 6 || bk == 11 || bk == 13 || bk == 14 || bk == 15) && intenOf(p) > 0) lit++; }
        check("a globe with its centre behind the camera still shows its limb", lit > 200, std::to_string(lit) + " pixels");
    }
    // B-308: a companion star beside or behind the camera never floods the frame; ahead it is a disc of the right size
    {
        Star cs; StarSystem csys; int ck = -1;
        for (int64_t x = 150; x < 320 && ck < 0; x++)
            for (int64_t z = 20; z < 140 && ck < 0; z++) {
                if (!starInSector(x, 0, z, cs)) continue;
                NebulaPatch np[16]; if (nebulaPatches(Vec3(x + 0.5, 0.5, z + 0.5), np, 16) > 0) continue;   // G-01: a sky without nebula patches, which would raise the mean shade (the test measures the companion's flood)
                csys.generate(cs); if (csys.companion >= 0) ck = csys.companion;
            }
        check("a system with a companion star found", ck >= 0);
        if (ck >= 0) {
            SpaceRenderer sr; StarNeighborhood nb; double t = 3000.0;
            const Body& k = csys.bodies[ck];
            Vec3 kp = csys.bodyPos(ck, t);
            Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
            Vec3 shipPos = kp + dir * (3.5 * k.radiusKm);   // where the ship parks (Game::parkDistanceFor)
            Vec3 toStar = -dir, side = normalize(cross(toStar, Vec3(0, 1, 0)));
            nb.update(shipPos);
            double angR = std::asin(1.0 / 3.5);
            double expect = PI * std::pow(sr.proj.f * std::tan(angR), 2) / (FBW * FBH);   // the disc's share of the frame when centred
            const int offs[4] = {0, 70, 95, 150};
            std::vector<uint32_t> strip((size_t)FBW * 4 * FBH, 0xFF101010u), px((size_t)FBW * FBH);
            double frac[4] = {0}, mean[4] = {0};
            for (int i = 0; i < 4; i++) {
                double a = offs[i] * DEG;
                Vec3 fwd = normalize(toStar * std::cos(a) + side * std::sin(a));
                Framebuffer fb; SpaceContext c;
                c.sys = &csys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                sr.setupPalette(fb, &csys, -1, -1, 1.0);
                sr.render(fb, c);
                int disc = 0; double sum = 0;
                for (Pix q : fb.idx) { if (bankOf(q) == 12 && shadeOf(q) > 25) disc++; sum += shadeOf(q); }
                frac[i] = (double)disc / (FBW * FBH); mean[i] = sum / (FBW * FBH);
                fb.mush(2); fb.toRGB(px.data(), 1.0);
                for (int y = 0; y < FBH; y++) memcpy(&strip[(size_t)y * FBW * 4 + i * FBW], &px[(size_t)y * FBW], FBW * 4);
            }
            writePNG("shots/tests/unit_companion_swing.png", strip.data(), FBW * 4, FBH);
            check("the companion ahead is a disc of the expected size", frac[0] > 0.5 * expect && frac[0] < 1.6 * expect, fmt("%.1f%% of the frame, expected %.1f%%", frac[0] * 100, expect * 100));
            check("the companion 70 deg off axis leaves no disc", frac[1] == 0 && mean[1] < 8, fmt("disc %.2f%%, mean shade %.1f", frac[1] * 100, mean[1]));
            // G-01: measured against the 70-degree frame, so the galaxy band's own brightness (now the whole disc, by direction) does not count
            check("the companion beside the camera (95 deg) does not flood", frac[2] == 0 && mean[2] < mean[1] + 1.5, fmt("disc %.2f%%, mean shade %.1f against %.1f at 70 deg", frac[2] * 100, mean[2], mean[1]));
            check("the companion behind (150 deg) does not flood", frac[3] == 0 && mean[3] < mean[1] + 1.5, fmt("disc %.2f%%, mean shade %.1f against %.1f at 70 deg", frac[3] * 100, mean[3], mean[1]));
        }
    }
    // B-312: a big ringed globe whose centre lies outside the frame to the left or the right still shows its limb (its
    // projection is a conic that stretches away from the centre; the old box round the projected centre cut it off);
    // B-317: in a sky the bodies' depths are written in metres, beyond the farthest terrain (they were kilometres, so a
    // moon 20,000 km away hid the ground beyond 20 km)
    {
        Star rs; StarSystem rsys; int rb = -1;
        for (int64_t x = 150; x < 320 && rb < 0; x++)
            for (int64_t z = 20; z < 140 && rb < 0; z++) {
                if (!starInSector(x, 0, z, rs)) continue;
                rsys.generate(rs);
                for (int i = 0; i < (int)rsys.bodies.size() && rb < 0; i++) if (rsys.bodies[i].rings && rsys.bodies[i].type != PT_COMPANION) rb = i;
            }
        check("a ringed world found", rb >= 0);
        if (rb >= 0) {
            SpaceRenderer sr; StarNeighborhood nb; double t = 3000.0;
            const Body& b = rsys.bodies[rb];
            Vec3 bp = rsys.bodyPos(rb, t);
            Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
            Vec3 shipPos = bp + dir * (3.6 * b.radiusKm);   // the disc is 32 degrees across from here
            Vec3 toBody = -dir, side = normalize(cross(toBody, Vec3(0, 1, 0)));
            nb.update(shipPos);
            warmMaps(sr, rsys, rb);
            int lit[3] = {0, 0, 0};
            float deepest = 0;
            const double offs[3] = {-45 * DEG, 45 * DEG, 45 * DEG};   // the centre 10 degrees outside the frame on either side, the limb 6 degrees inside
            for (int i = 0; i < 3; i++) {
                Vec3 fwd = normalize(toBody * std::cos(offs[i]) + side * std::sin(offs[i]));
                Framebuffer fb; SpaceContext c;
                c.sys = &rsys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                c.bankBodyA = rb;
                sr.setupPalette(fb, &rsys, rb, -1, 1.0);
                if (i < 2) {
                    sr.render(fb, c);
                    for (Pix p : fb.idx) { int bk = bankOf(p); if ((bk == 2 || bk == 6 || bk == 11 || bk == 13 || bk == 14 || bk == 15) && intenOf(p) > 0) lit[i]++; }
                } else {   // the same view as a moon's sky sees it (drawSkyBodies: the sky bank, depth in metres)
                    c.skyMode = true; c.skyDark = 1; c.excludeBody = -1;
                    sr.drawSkyBodies(fb, c);
                    for (size_t o = 0; o < fb.idx.size(); o++) { if (bankOf(fb.idx[o]) == 1 && shadeOf(fb.idx[o]) > 35) lit[i]++; deepest = std::max(deepest, fb.invz[o]); }
                }
            }
            check("a globe centred left of the frame shows its limb", lit[0] > 100, std::to_string(lit[0]) + " pixels");
            check("a globe centred right of the frame shows its limb", lit[1] > 100, std::to_string(lit[1]) + " pixels");
            check("the same globe in a sky shows its limb", lit[2] > 100, std::to_string(lit[2]) + " pixels");
            check("sky bodies lie beyond the far ring", deepest > 0 && deepest < 1.0f / 53000.0f, fmt("nearest 1/%.0f m", deepest > 0 ? 1.0 / deepest : 0.0));
            // a small body close by: at 3.6 radii of a 1000 km moon the old kilometre depth read as 3,600 m
            int sb = -1;
            for (int i = 0; i < (int)rsys.bodies.size() && sb < 0; i++) if (rsys.bodies[i].type != PT_COMPANION && rsys.bodies[i].radiusKm < 3000) sb = i;
            if (sb >= 0) {
                Vec3 sp = rsys.bodyPos(sb, t);
                Vec3 ship2 = sp + dir * (3.6 * rsys.bodies[sb].radiusKm);
                Framebuffer fb; SpaceContext c;
                c.sys = &rsys; c.stars = &nb.stars; c.t = t; c.shipPos = ship2; c.cam = cameraBasis(std::atan2(toBody.x, toBody.z), std::asin(clampd(toBody.y, -1, 1)));
                c.skyMode = true; c.skyDark = 1; c.excludeBody = -1;
                sr.setupPalette(fb, &rsys, sb, -1, 1.0);
                sr.drawSkyBodies(fb, c);
                float deep2 = 0; for (float z : fb.invz) deep2 = std::max(deep2, z);
                check("a small body close by lies beyond the far ring too", deep2 > 0 && deep2 < 1.0f / 53000.0f, fmt("nearest 1/%.0f m (it would read 1/%.0f in kilometres)", deep2 > 0 ? 1.0 / deep2 : 0.0, 3.6 * rsys.bodies[sb].radiusKm));
            }
        }
    }
    // B-313: the terrain rings meet at their edges (geomorphing): a ring's edge vertices are drawn on the coarser ring's
    // surface, where the unmorphed vertices differed by metres to hundreds of metres (the cells popped as the edge swept)
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        bool ok = setupSceneForType(PT_FELISIAN, 0, 40 * DEG, 0.0, 0.02, sv, sys, nb, t, -20);
        check("the mountain review site set up", ok);
        if (ok) {
            Framebuffer fb; Input in;
            sv.update(1.0 / 30, in, t, false);
            sv.render(fb, t, nb.stars, sr, 1.0);
            double seam[4], raw[4];
            sv.testRingSeams(seam, raw);
            const char* names[4] = {"near ring on lod0", "lod0 on lod1", "lod1 on lod2", "lod2 on lod3"};
            for (int k = 0; k < 4; k++)
                check((std::string("rings meet: ") + names[k]).c_str(), seam[k] >= 0 && seam[k] < 0.01, fmt("gap %.4f m (unmorphed %.1f m)", seam[k], raw[k]));
        }
    }
    // B-316: the habitat cells put herds round the explorer wherever the biome has them, not only at the landing
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        bool ok = setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, sv, sys, nb, t, -3);
        check("a grassland herd site set up", ok);
        if (ok) {
            Input in;
            for (int i = 0; i < 3; i++) sv.update(1.0 / 30, in, t + i, false);
            int origin = 0, cells = 0;
            for (const SurfaceView::Herd& h : sv.herds) (h.origin ? origin : cells)++;
            check("the landing's own herds are there", origin >= 1, std::to_string(origin) + " herds");
            check("habitat herds spawned within 900 m", cells >= 1, std::to_string(cells) + " herds, " + std::to_string(sv.critters.size()) + " animals in all");
            // walk 3 km away: the landing's herds stay listed, the cells behind are forgotten and new ones spawn
            sv.player.x += 3000; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
            for (int i = 0; i < 3; i++) sv.update(1.0 / 30, in, t + 10 + i, false);
            int far = 0, near = 0;
            for (const SurfaceView::Herd& h : sv.herds) { if (h.origin) continue; double dx = h.cx - sv.player.x, dz = h.cz - sv.player.z; (dx * dx + dz * dz > 1300.0 * 1300.0 ? far : near)++; }
            check("3 km on, the herds behind are forgotten", far == 0, std::to_string(far) + " herds beyond 1300 m");
            check("3 km on, new herds are within reach", near >= 1, std::to_string(near) + " herds within 1300 m");
            bool consistent = true;
            for (const Critter& c : sv.critters) if (c.herd < 0 || c.herd >= (int)sv.herds.size()) consistent = false;
            check("every animal belongs to a listed herd", consistent);
        }
    }
    // O0-04 (B-306): the landing map carries the shadow of a moon at some point of the moon's orbit
    {
        Star ms; StarSystem msys; int mb = -1;
        for (int64_t x = 150; x < 320 && mb < 0; x++)
            for (int64_t z = 20; z < 140 && mb < 0; z++) {
                if (!starInSector(x, 0, z, ms)) continue;
                msys.generate(ms);
                for (int i = 0; i < (int)msys.bodies.size(); i++) if (msys.bodies[i].parent < 0 && msys.bodies[i].moonCount > 0 && PLANET_TYPES[msys.bodies[i].type].landable && msys.bodies[i].orbitIncl < 3 * DEG) { mb = i; break; }
            }
        check("a landable planet with a moon found", mb >= 0);
        if (mb >= 0) {
            Game g; g.savePrefix = "shots/tests/unit_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_guide.txt";
            g.testParkAt(ms, mb);
            int moon = -1; for (int i = 0; i < (int)msys.bodies.size(); i++) if (msys.bodies[i].parent == mb) { moon = i; break; }
            double P = msys.bodies[moon].orbitPeriod, mx = 0;
            for (int k = 0; k < 96 && mx < 0.5; k++) { g.testSetTime(3.6e6 + k * P / 96.0); mx = std::max(mx, g.testMapShadowMax()); }
            check("the moon's shadow crosses the landing map", mx >= 0.5, "darkest " + std::to_string(mx));
        }
    }
    // O2 (R-301): sector names
    {
        int sx, sy;
        sectorIndexOf(0, 0, sx, sy); check("sector of 0,0 is 180:090", sx == 180 && sy == 90);
        sectorIndexOf(-89.9, -179.9, sx, sy); check("sector of the south-west corner is 000:000", sx == 0 && sy == 0);
        sectorIndexOf(89.9, 179.99, sx, sy); check("sector of the north-east corner is 359:179", sx == 359 && sy == 179);
    }
    // O0-01 (B-301): a ringed landable planet exists near the start; its ring profile is shared and bounded; the ring's
    // shadow falls on its ground somewhere at low latitude
    {
        Star rs; StarSystem rsys; int rb = -1;
        for (int64_t x = 150; x < 340 && rb < 0; x++)
            for (int64_t z = 20; z < 160 && rb < 0; z++) {
                if (!starInSector(x, 0, z, rs)) continue;
                rsys.generate(rs);
                for (int i = 0; i < (int)rsys.bodies.size(); i++) if (rsys.bodies[i].rings && rsys.bodies[i].parent < 0 && PLANET_TYPES[rsys.bodies[i].type].landable) { rb = i; break; }
            }
        check("a ringed landable planet found", rb >= 0);
        if (rb >= 0) {
            std::vector<float> prof = rsys.ringProfileOf(rb);
            float mx = 0; bool bounded = true;
            for (float v : prof) { mx = std::max(mx, v); if (v < 0 || v > 1) bounded = false; }
            check("ring profile bounded with a dense part", bounded && mx > 0.3, "max " + std::to_string(mx));
            double worst = 0;
            for (int la = -30; la <= 30 && worst < 0.2; la += 3)
                for (int k = 0; k < 16 && worst < 0.2; k++) {
                    double tt = 2000.0 + k * rsys.bodies[rb].orbitPeriod / 16.0;
                    SurfaceSite site; site.init(&rsys, rb, la * DEG, 0.5, tt);
                    worst = std::max(worst, site.sun(tt).ringShadow);
                }
            check("the ring shadows the ground somewhere", worst >= 0.2, "shadow " + std::to_string(worst));
        }
    }
    // O1 (B-302): the rangefinder: 45 degrees down from eye height on the ground hits the ground a few metres off
    if (bi >= 0) {
        SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        nb.update(sv.site.worldPos(0, 0, 0, 0));
        sv.player.pitch = -45 * DEG;
        Input in; sv.update(1.0 / 60, in, 0.0, false);
        Framebuffer fb; sv.render(fb, 0.0, nb.stars, sr, 1.0);
        check("rangefinder 45 deg down hits within 1..6 m", sv.lastRange > 1.0 && sv.lastRange < 6.0, "range " + std::to_string(sv.lastRange));
        check("the far ring reaches 53 km", sv.drawRadiusM() >= 53000, std::to_string(sv.drawRadiusM()));
    }
    // O4 (R-303): comets are landable; on one the explorer is held under the escape velocity and the buggy refuses
    {
        Star cs; StarSystem csys; int cb = -1;
        for (int64_t x = 150; x < 320 && cb < 0; x++)
            for (int64_t z = 20; z < 140 && cb < 0; z++) {
                if (!starInSector(x, 0, z, cs)) continue;
                csys.generate(cs);
                for (int i = 0; i < (int)csys.bodies.size(); i++) if (csys.bodies[i].type == PT_COMET) { cb = i; break; }
            }
        check("comets are landable", PLANET_TYPES[PT_COMET].landable && cb >= 0);
        if (cb >= 0) {
            SurfaceView sv; sv.init(&csys, cb, 0.2, 0.5, 0.0);
            double ve = sv.site.escapeVelocity;
            check("comet escape velocity under 30 m/s", ve > 0.1 && ve < 30, "v " + std::to_string(ve));
            check("small body: terrain rings capped near the horizon", sv.site.smallBody && sv.drawRadiusM() <= 1.2 * sv.site.R, std::to_string(sv.drawRadiusM()));
            sv.relocateCapsule(sv.player.x + 3, sv.player.z + 2);
            check("no buggy on a comet", !sv.deployBuggy());
            Input in; in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true;
            for (int i = 0; i < 150; i++) sv.update(1.0 / 30, in, i / 30.0, true);
            double spd = std::sqrt(sv.player.vx * sv.player.vx + sv.player.vz * sv.player.vz);
            check("sprinting on a comet stays under 0.3 escape velocity", spd > 0.05 && spd <= 0.3 * ve + 0.01, "speed " + std::to_string(spd));
        }
    }
    // R-403: the drone lifts off, reaches 320 km/h, climbs to its ceiling, never goes under the ground, lands under Shift and lets the
    // explorer out; a second one unfolds beside the capsule and scraps the first
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem dsys; SurfaceView sv; double t = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, sv, dsys, nb, t, -9)) {
            if (sv.inBuggy) sv.toggleBuggy();
            sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
            bool dep = sv.deployDrone(); sv.drone.unfold = 1;
            sv.player.x = sv.drone.x + 1.5; sv.player.z = sv.drone.z;
            bool got = sv.toggleDrone();
            Input in2; double under = 0, top = 0, altMax = 0; bool ceilingSeen = false; int frames = 0;
            auto step = [&](int n, bool w, bool s, bool space, bool shift, bool a) {
                for (int i = 0; i < n; i++) {
                    in2.down[KEY_W] = w; in2.down[KEY_S] = s; in2.down[KEY_SPACE] = space; in2.down[KEY_LEFT_SHIFT] = shift; in2.down[KEY_A] = a;
                    sv.update(1.0 / 30, in2, t, true); t += 1.0 / 30; in2.newFrame(); frames++;
                    under = std::max(under, sv.site.surfaceHeight(sv.drone.x, sv.drone.z) - sv.drone.y);
                    top = std::max(top, std::fabs(sv.drone.speed)); altMax = std::max(altMax, sv.drone.altAboveGround); ceilingSeen = ceilingSeen || sv.drone.ceiling;
                }
            };
            step(90, false, false, true, false, false);
            bool lifted = !sv.drone.landed && sv.drone.altAboveGround > 5;
            step(30 * 25, true, false, true, false, false);
            double topAt25 = top;
            step(30 * 25, true, false, true, false, true);
            int n = 0; while (std::fabs(sv.drone.speed) > 2 && n++ < 30 * 30) step(1, false, true, false, false, false);
            double braked = std::fabs(sv.drone.speed);
            n = 0; while (!sv.drone.landed && n++ < 30 * 90) step(1, false, false, false, true, false);
            bool landed = sv.drone.landed; double landS = n / 30.0;
            bool out = sv.toggleDrone() && !sv.inDrone;
            sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
            double oldX = sv.drone.x; bool redeployed = sv.deployDrone() && sv.drone.x != oldX && sv.drone.odometer == 0;
            check("drone: lifts, 320 km/h, ceiling, lands, out (R-403)", dep && got && lifted && topAt25 * 3.6 > 300 && top <= SurfaceView::DRONE_TOP_SPEED + 0.01 && ceilingSeen && altMax > 380 && braked <= 2 && landed && out && under < 0.01 && redeployed,
                  fmt("top %.0f km/h at 25 s, %.0f m up, under %.3f m, landed in %.0f s, %d frames", topAt25 * 3.6, altMax, under, landS, frames));
        } else check("drone: a grassland site for the flight", false);
    }
    // B-406: the jetpack flies over deep water (it used to be pulled down into it) and lifts out of it. A shore with water over
    // 0.6 m deep within 110 m: the beach scene, else the river bank, else the lake shore; the flight goes there
    {
        struct WaterRun { bool ran = false, flewOver = false, fellIn = false, liftedOut = false; double dist = 0, deepT = 0, maxDeep = 0, swimT = 0, minAlt = 1e9, heat = 0; const char* where = "none"; };
        WaterRun R;
        auto tryScene = [&](int want, double lat, double alt, const char* name) {
            if (R.ran) return;
            SpaceRenderer sr; StarNeighborhood nb; StarSystem wsys; SurfaceView sv; double t = 0;
            if (!setupSceneForType(PT_FELISIAN, lat, alt, 0.0, 0.0, sv, wsys, nb, t, want)) return;
            double bestD = 1e9, yawTo = 0;
            for (int a = 0; a < 32; a++) for (double dd = 6; dd <= 110; dd += 2) {
                double ang = a * TAU / 32, px = sv.player.x + std::sin(ang) * dd, pz = sv.player.z + std::cos(ang) * dd;
                double g = sv.site.groundHeight(px, pz), wl = sv.site.waterAt(px, pz);
                if (wl > -1e8 && g < wl - 0.6) { if (dd + 6 < bestD) { bestD = dd + 6; yawTo = ang; } break; }
            }
            if (bestD > 1e8) return;
            R.ran = true; R.where = name; R.dist = bestD;
            sv.player.yaw = yawTo; sv.player.pitch = 0;
            int flyFrames = (int)(30 * std::min(18.0, 2.0 + bestD / 3.5));
            Input in2; bool overDeep = false;
            auto step = [&](int n, bool w, bool space, bool spaceTap) {
                for (int i = 0; i < n; i++) {
                    in2.down[KEY_W] = w; in2.down[KEY_SPACE] = space; if (spaceTap && i == 0) in2.pressed[KEY_SPACE] = true;
                    sv.update(1.0 / 30, in2, t, true); t += 1.0 / 30; in2.newFrame();
                    double g = sv.site.groundHeight(sv.player.x, sv.player.z), wl = sv.site.waterAt(sv.player.x, sv.player.z);
                    bool deep = wl > -1e8 && g < wl - 0.4;
                    if (deep) { overDeep = true; R.deepT += 1.0 / 30; R.maxDeep = std::max(R.maxDeep, wl - g); if (sv.player.swimming) R.swimT += 1.0 / 30; else R.minAlt = std::min(R.minAlt, sv.player.y - wl); }
                }
            };
            step(1, true, true, true);           // a jump, then the jet held with W: out over the water, a second past its edge
            for (int i = 1; i < flyFrames && R.deepT < 1.0; i++) step(1, true, true, false);
            R.flewOver = overDeep && R.deepT > 0.5 && R.swimT < 0.2 && R.minAlt > 0.0;
            R.heat = sv.player.jetHeat;
            step(30 * 6, false, false, false);   // the jet released: it drops into the water
            R.fellIn = sv.player.swimming;
            step(30 * 3, false, true, false);    // Space from the surface: the jet lifts it out
            R.liftedOut = !sv.player.swimming && sv.player.jetOn && sv.player.vy > 0;
        };
        tryScene(MAT_SAND, 12, 30 * DEG, "beach");
        tryScene(-21, 10, 40 * DEG, "river bank");
        tryScene(-22, 10, 40 * DEG, "lake shore");
        check("jetpack over and out of the water (B-406)", R.ran && R.flewOver && R.fellIn && R.liftedOut,
              R.ran ? fmt("%s: deep water %.0f m off; %.1f s over it (%.1f m deep), swam %.1f s of it, lowest %.1f m over it, heat %.0f, fell in %s, lifted out %s", R.where, R.dist, R.deepT, R.maxDeep, R.swimT, R.minAlt < 1e8 ? R.minAlt : 0.0, R.heat, R.fellIn ? "yes" : "no", R.liftedOut ? "yes" : "no") : "no shore scene with deep water within 110 m");
    }
    {   // C-07: the signals are a property of the galaxy: the same twice from home, every people's signal's star has a people's world in a
        // transmitter sector, every pulsar signal's star a pulsar, all within reach and sorted by distance, rare (one to a dozen within
        // 200 ly on the arm); the beam's gain falls off its axis; the broadcast's chain never repeats a recording twice running
        Vec3 home((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
        std::vector<Signal> a, b; double t0 = nowSec(); int cells = signalsNear(home, a); double ms = (nowSec() - t0) * 1e3; signalsNear(home, b);
        bool same = a.size() == b.size(); int people = 0, pulsars = 0, bad = 0; double maxLy = 0;
        for (size_t i = 0; i < a.size(); i++) {
            const Signal& s = a[i];
            if (same && (s.seed != b[i].seed || s.kind != b[i].kind || s.distLy != b[i].distLy)) same = false;
            if (i > 0 && s.distLy < a[i - 1].distLy) bad++;
            maxLy = std::max(maxLy, s.distLy);
            if (s.kind == SIG_PEOPLE) {
                people++;
                Signal again; bool has = starSignal(s.star, again);
                if (!has || again.kind != SIG_PEOPLE || again.body != s.body || !sectorTransmits(s.star.sx, s.star.sy, s.star.sz) || s.distLy > SIGNAL_REACH_LY) bad++;
                if (s.bodyType != PT_FELISIAN && s.bodyType != PT_DESERT) bad++;
            } else if (s.kind == SIG_PULSAR) { pulsars++; if (s.star.cls != STAR_PULSAR || s.pulseHz <= 0 || s.distLy > SIGNAL_REACH_LY) bad++; }
            else bad++;
            if (s.strength <= 0 || s.strength > 1) bad++;
        }
        bool gainOk = beamGain(0) > 0.99 && beamGain(2 * DEG) > beamGain(5 * DEG) && beamGain(5 * DEG) > beamGain(15 * DEG) && beamGain(15 * DEG) > beamGain(40 * DEG) && beamGain(90 * DEG) < 0.03;
        int chainBad = 0; { int prev = transmittedShard(0x1234, 3.6e6); if (prev < 0 || prev >= 50) chainBad++; for (int k = 0; k < 200; k++) { int nx = nextTransmittedShard(0x1234, prev); if (nx == prev || nx < 0 || nx >= 50) chainBad++; prev = nx; } }
        std::string first = a.empty() ? "none" : fmt("the nearest %s at %.0f ly (sector %lld %lld %lld, strength %.2f)", SIGNAL_KIND_NAMES[a[0].kind], a[0].distLy, (long long)a[0].star.sx, (long long)a[0].star.sy, (long long)a[0].star.sz, a[0].strength);
        check("signals: hashed, rare and far (C-07)", same && bad == 0 && people >= 1 && people <= 12 && gainOk && chainBad == 0,
              fmt("%zu signals from home (%d of a people, %d pulsars) over %d cells in %.1f ms, %s, the farthest %.0f ly, %d wrong, the chain %d wrong", a.size(), people, pulsars, cells, ms, first.c_str(), maxLy, bad, chainBad));
    }
    {   // C-07: the radar in the game: B switches it on at home, the view turned onto the first people's signal hears its recording through
        // the static (a piece or a speech on the synth, the radar's), the hold locks within eight seconds, Enter sets the remote target on
        // the signal's star (the local target when it is of this system), the view turned forty degrees away loses the lock, B off releases the synth
        Game game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { int n = (int)(secs * 30); for (int i = 0; i < n; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); } };
        auto press = [&](int key) { gi.pressed[key] = true; gi.down[key] = true; game.frame(gi, 1.0 / 30); gi.newFrame(); gi.down[key] = false; };
        run(0.2); press(KEY_B); run(0.2);
        bool on = game.testRadarOn(); int n = game.testRadarSignals();
        int who = -1; for (int i = 0; i < n && who < 0; i++) if (game.testRadarKind(i) == SIG_PEOPLE) who = i;
        bool aimed = who >= 0 && game.testAimAtSignal(who);
        run(1.0);
        bool heard = game.audio.radio && (game.audio.piece || game.audio.speech || game.testRadarProgramme() >= RP_BEACON);   // a programme on the air: a recording, or one of the world's machines
        int programme = game.testRadarProgramme();
        std::string early = game.testRadarInfo(); (void)early;   // the detail stays short: `fmt` has a bound
        double lockAt = -1; for (int i = 0; i < 30 * 8 && lockAt < 0; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); if (game.testRadarLocked() >= 0) lockAt = (i + 1) / 30.0; }
        bool locked = game.testRadarLocked() == who;
        std::string atLock = game.testRadarInfo(); (void)atLock;
        gi.down[KEY_RIGHT] = true; run(0.6); gi.down[KEY_RIGHT] = false; run(2.0);   // 0.6 s of the arrows: 41 degrees
        bool lost = game.testRadarLocked() < 0;
        bool again = game.testAimAtSignal(who); run(8.0);
        bool relocked = again && game.testRadarLocked() == who;
        Star want; std::string wantName; bool local = game.testRadarLocal(who);
        {   // the signal's star by its sector, for the arrival below
            int64_t sx = 0, sy = 0, sz = 0;
            if (game.testRadarSector(who, sx, sy, sz) && starInSector(sx, sy, sz, want, true)) wantName = want.name;
        }
        press(KEY_ENTER); run(0.1);
        std::string st = game.testStatus();
        bool flying = who >= 0 && (local ? st.rfind("FINE APPROACH", 0) == 0 : st.rfind("VIMANA FLIGHT", 0) == 0);
        bool released = !game.testRadarOn() && !game.audio.radio && !game.audio.piece && !game.audio.speech && game.audio.radar == 0;
        double flewFor = 0; bool arrived = false;
        if (!local) { for (int i = 0; i < 30 * 40 && !arrived; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); flewFor = (i + 1) / 30.0; if (game.testStarGenName() == wantName && !wantName.empty()) arrived = true; } }
        bool people = arrived && game.testCivilisedHere();
        check("radar: the sweep hears, locks, flies (C-07)", on && n > 0 && aimed && heard && lockAt > 0 && locked && lost && relocked && flying && released && (local || (arrived && people)),
              fmt("%s, %d signals, the people's %d aimed %s; heard %s a second in (%s); locked after %.1f s; lost after the turn %s, locked again %s; Enter: '%s' (%s), the camera %s; %s", on ? "on" : "NOT on", n, who, aimed ? "yes" : "no", heard ? "yes" : "NO", programme >= 0 && programme < RP_COUNT ? RADIO_PROGRAMME_NAMES[programme] : "nothing", lockAt, lost ? "yes" : "NO", relocked ? "yes" : "NO", st.c_str(), flying ? "flying" : "NOT flying", released ? "left" : "STILL on",
                  local ? "a signal of this system" : fmt("arrived at %s after %.0f s: %s", arrived ? wantName.c_str() : "NOWHERE", flewFor, people ? "a people's world there" : "NO people's world").c_str()));
        remove("shots/tests/test_guide.txt");
    }
    {   // C-07: the receiver through the synth: the bed alone and each kind of signal at full clarity, three seconds each, finite, under the ceiling, audible; silent once off
        AudioSynth synth; AudioState st; const int sr = 22050; std::vector<float> buf(2048);
        std::string detail; bool ok = true;
        for (int kind = -1; kind < SIG_KIND_COUNT; kind++) {
            st.radar = 1; st.radarSignal = kind < 0 ? 0 : 1; st.radarKind = kind; st.radarPulseHz = 1.7;
            int nan = 0, clip = 0; double sum = 0, peak = 0; long cnt = 0;
            for (int frames = 0; frames < sr * 3; frames += (int)buf.size()) {
                synth.render(buf.data(), (int)buf.size(), sr, st);
                for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; peak = std::max(peak, (double)std::fabs(v)); sum += (double)v * v; cnt++; }
            }
            double rms = std::sqrt(sum / std::max(1L, cnt));
            if (nan || clip > 20 || rms < 0.01 || rms > 0.4) ok = false;
            detail += fmt("%s rms %.3f peak %.2f%s; ", kind < 0 ? "the bed" : SIGNAL_KIND_NAMES[kind], rms, peak, nan ? " NON-FINITE" : (clip > 20 ? " CLIPPING" : ""));
        }
        st.radar = 0; st.radarSignal = 0; st.radarKind = -1;
        double ssum = 0; long scnt = 0;
        for (int k = 0; k < 40; k++) { synth.render(buf.data(), (int)buf.size(), sr, st); if (k >= 30) for (float v : buf) { ssum += (double)v * v; scnt++; } }
        double stopRms = std::sqrt(ssum / std::max(1L, scnt));
        check("radar: the receiver through the synth (C-07)", ok && stopRms < 0.003, detail + fmt("off: %.4f", stopRms));
    }
    {   // C-07 (the user's second call, more sounds): the receiver's variety. Three seeds of each natural kind and every programme of the
        // first people's world heard from home, three seconds each, finite, under the ceiling, audible; the world's broadcast has a
        // character (four to six of the eleven programmes over its fifty slots, the voice the commonest, the same twice); the bed's
        // events over forty seconds leave it finite and under the ceiling
        PeopleWorld w; Signal sg; bool have = firstPeopleWorld(w, &sg);
        std::string detail; bool ok = have; int rendered = 0;
        static const double RATES[3] = {0.7, 1.7, 3.2};
        for (int kind = SIG_PULSAR; kind < SIG_KIND_COUNT && ok; kind++)
            for (int k = 0; k < 3; k++) {
                RenderStats r = renderKind(kind, 0x51 + (uint64_t)k * 0x1F3, RATES[k], 3.0, nullptr); rendered++;
                if (r.nan || r.clip > 20 || r.rms < 0.01 || r.rms > 0.4) { ok = false; detail += fmt("%s seed %d rms %.3f peak %.2f%s%s; ", SIGNAL_KIND_NAMES[kind], k, r.rms, r.peak, r.nan ? " NON-FINITE" : "", r.clip > 20 ? " CLIPPING" : ""); }
            }
        double lo = 1, hi = 0;
        for (int p = 0; p < RP_COUNT && have; p++) {
            RenderStats r = renderProgramme(w, p, 3.0, nullptr); rendered++;
            lo = std::min(lo, r.rms); hi = std::max(hi, r.rms);
            if (r.nan || r.clip > 20 || r.rms < 0.01 || r.rms > 0.4) { ok = false; detail += fmt("%s rms %.3f peak %.2f%s%s; ", RADIO_PROGRAMME_NAMES[p], r.rms, r.peak, r.nan ? " NON-FINITE" : "", r.clip > 20 ? " CLIPPING" : ""); }
        }
        { RenderStats r = renderKind(-1, 0, 1, 40.0, nullptr); rendered++; if (r.nan || r.clip > 20 || r.rms < 0.01 || r.rms > 0.4) { ok = false; detail += fmt("the bed over 40 s rms %.3f peak %.2f; ", r.rms, r.peak); } }
        int counts[RP_COUNT] = {0}, distinct = 0, most = 0; bool same = true;
        if (have) for (int i = 0; i < SHARDS_PER_WORLD; i++) { int p = programmeOf(w.seed, i, w.shards[i].music); if (p != programmeOf(w.seed, i, w.shards[i].music)) same = false; counts[p]++; }
        for (int p = 0; p < RP_COUNT; p++) { if (counts[p]) distinct++; if (counts[p] > counts[most]) most = p; }
        std::string mix; for (int p = 0; p < RP_COUNT; p++) if (counts[p]) mix += fmt("%s%s %d", mix.empty() ? "" : ", ", RADIO_PROGRAMME_NAMES[p], counts[p]);
        bool character = have && same && distinct >= 4 && distinct <= 6 && most == RP_VOICE;
        check("radar: the receiver's variety (C-07)", ok && character, fmt("%s: %d renders%s, the programmes' rms %.3f-%.3f; the broadcast of %s: %s", have ? "ok" : "NO people's world", rendered, detail.empty() ? "" : (" - " + detail).c_str(), lo, hi, have ? w.name.c_str() : "?", mix.c_str()));
    }
    // O3 (R-302): belt rocks keep their identity as the belt shears
    {
        Star bs; StarSystem bsys; bool have = false;
        for (int64_t x = 150; x < 320 && !have; x++)
            for (int64_t z = 20; z < 140 && !have; z++) {
                if (!starInSector(x, 0, z, bs)) continue;
                bsys.generate(bs);
                if (!bsys.belts.empty()) have = true;
            }
        check("a belt found near the start", have);
        if (have) {
            const Belt& bl = bsys.belts[0];
            Vec3 anchor = bsys.star.pos + Vec3(0.5 * (bl.innerKm + bl.outerKm), 0, 0);
            int n = 0; BeltRock first; bool got = false;
            bsys.forBeltRocksNear(0, anchor, 1000.0, 2, 2, 1, [&](const BeltRock& rk) { n++; if (!got) { first = rk; got = true; } });
            check("rocks within two cells of the anchor", n >= 3, std::to_string(n) + " rocks");
            BeltRock later; bool same = got && bsys.beltRockAt(0, first.ir, first.ia, first.iy, first.m, 1000.0 + 7200, later);
            check("a rock is the same rock two hours later", same && std::fabs(later.radiusKm - first.radiusKm) < 1e-12 && later.seed == first.seed);
            double rr = std::sqrt((later.pos.x - bsys.star.pos.x) * (later.pos.x - bsys.star.pos.x) + (later.pos.z - bsys.star.pos.z) * (later.pos.z - bsys.star.pos.z));
            check("the rock stays inside the belt", same && rr >= bl.innerKm && rr <= bl.outerKm);
            // late in an expedition (t = 4e6 s) the rings have sheared by hundreds of cells: the rock beside the ship must still be found
            {
                double tl = 4.0e6; BeltRock rk0; bool ok0 = false;
                bsys.forBeltRocksNear(0, anchor, tl, 1, 1, 1, [&](const BeltRock& rk) { if (!ok0) { rk0 = rk; ok0 = true; } });
                int near = 0;
                if (ok0) bsys.forBeltRocksNear(0, rk0.pos, tl, 4, 4, 1, [&](const BeltRock& rk) { if (length(rk.pos - rk0.pos) < 3 * BELT_CELL_KM) near++; });
                check("rocks are found round a rock after the rings sheared", ok0 && near >= 5, std::to_string(near) + " within three cells");
            }
        }
    }
    {   // R-402: magnetic fields and the star's storms: every class occurs; at the oval, a strong field round an active star keeps an
        // aurora on most nights (over the HUD's 0.15), a weak field has one rarely; the storms come and go
        int cls[4] = {0, 0, 0, 0}, n = 0, strongN = 0, weakN = 0, stormy = 0, nights = 0;
        double strongLit = 0, weakLit = 0;
        for (int64_t x = 150; x < 200; x++)
            for (int64_t z = 20; z < 70; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int d = 0; d < 20; d++) { nights++; if (auroralStorm(s, d * 86400.0 + 500) > 0.3) stormy++; }
                for (const Body& b : sys.bodies) {
                    if (!PLANET_TYPES[b.type].landable || b.type == PT_COMET) continue;
                    double m = magneticField(b); cls[magneticClass(m)]++; n++;
                    if (!PLANET_TYPES[b.type].atmosphere || hasOpaqueDeck(b.type)) continue;
                    int lit = 0; for (int d = 0; d < 60; d++) if (auroraPotential(sys, b, d * 86400.0 + 1000) > 0.15) lit++;
                    bool active = starActivity(s.cls) >= 0.7;   // S-01: activity 0.7 and over; S-02: from the table
                    if (m >= 0.6 && active) { strongLit += lit / 60.0; strongN++; }
                    else if (m >= 0.08 && m < 0.3) { weakLit += lit / 60.0; weakN++; }
                }
            }
        check("magnetic field: every class occurs", n > 50 && cls[0] > 0 && cls[1] > 0 && cls[2] > 0 && cls[3] > 0, fmt("of %d worlds: none %d, weak %d, moderate %d, strong %d", n, cls[0], cls[1], cls[2], cls[3]));
        check("storms on some nights, not most", nights > 0 && stormy > nights / 20 && stormy < nights / 2, fmt("%d of %d nights over 0.3", stormy, nights));
        check("strong field, active star: most nights", strongN > 0 && strongLit / strongN > 0.7, fmt("%.0f%% of nights lit over %d worlds", strongN ? 100 * strongLit / strongN : 0.0, strongN));
        check("weak field: rarely", weakN > 0 && weakLit / weakN < 0.25, fmt("%.0f%% of nights lit over %d worlds", weakN ? 100 * weakLit / weakN : 0.0, weakN));
    }
    {   // S-01: the star classes. The varieties take their share of the families near home (red dwarfs the commonest class, every class present)
        int n = 0, cls[STAR_CLASS_COUNT] = {0};
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (starInSector(x, y, z, s, false)) { n++; cls[s.cls]++; } }
        int most = 0; bool all = true; std::string mixs;
        for (int i = 0; i < STAR_CLASS_COUNT; i++) { if (cls[i] > cls[most]) most = i; if (cls[i] == 0) all = false; mixs += fmt("%s %.0f%% ", STAR_CLASSES[i].code, n ? 100.0 * cls[i] / n : 0.0); }
        check("star classes: red dwarfs the commonest", n > 1000 && most == STAR_RED_DWARF && cls[STAR_RED_DWARF] > n / 4 && cls[STAR_RED_DWARF] < n / 2, fmt("%d stars: %s", n, mixs.c_str()));
        check("star classes: every class near home", all, mixs);
        // the red dwarf's flares: several a day, a minute each, the same at the same time
        Star rd; bool found = false;
        for (int64_t x = 150; x < 200 && !found; x++) for (int64_t z = 20; z < 70 && !found; z++) if (starInSector(x, 0, z, rd, false) && rd.cls == STAR_RED_DWARF) found = true;
        int samples = 0, flaring = 0, flares = 0; double peak = 0; bool det = true, was = false;
        for (double t = 0; t < 10 * 86400.0 && found; t += 2) {
            double f = starFlare(rd, t); samples++;
            if (f != starFlare(rd, t)) det = false;
            bool on = f > 0.3; if (on) flaring++; if (on && !was) flares++; was = on;
            peak = std::max(peak, f);
        }
        check("red dwarf flares: several a day", found && det && flares >= 50 && flares <= 200 && flaring < samples / 15 && flaring > samples / 500, fmt("%d flares in ten days, %.1f%% of the time over 0.3, peak %.2f", flares, samples ? 100.0 * flaring / samples : 0.0, peak));
        // the red dwarf's near worlds keep one face to it; a carbon star's worlds are carbon more often than a yellow star's
        int rdWorlds = 0, rdLocked = 0, csSys = 0, csCarbon = 0, ySys = 0, yCarbon = 0;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false)) continue;
            if (s.cls != STAR_RED_DWARF && s.cls != STAR_CARBON && s.cls != STAR_YELLOW) continue;
            StarSystem sys; sys.generate(s);
            bool carbon = false; for (auto& b : sys.bodies) if (b.type == PT_CARBON) carbon = true;
            if (s.cls == STAR_RED_DWARF) { int k = 0; for (auto& b : sys.bodies) { if (b.parent >= 0 || b.type == PT_COMET || b.type == PT_COMPANION) continue; if (k++ < 3) { rdWorlds++; if (b.locked) rdLocked++; } } }
            else if (s.cls == STAR_CARBON) { csSys++; if (carbon) csCarbon++; }
            else { ySys++; if (carbon) yCarbon++; }
        }
        check("red dwarf: near worlds locked", rdWorlds > 30 && rdLocked > rdWorlds * 0.7, fmt("%d of %d", rdLocked, rdWorlds));
        check("carbon star: carbon worlds", csSys > 5 && ySys > 5 && (double)csCarbon / csSys > 2.0 * (double)yCarbon / ySys, fmt("%d of %d systems against %d of %d round yellow stars", csCarbon, csSys, yCarbon, ySys));
    }
    {   // S-03: the neutron star. A share of the pulsars near home; its systems keep no living, cloudy, oceanic, quartz or giant world
        // and most of those with planets have a glassed outer survivor; on such a world the glass covers 15-85% of the land at 16 m
        // and the 512 m ring (the sheets do not follow the rings' fades, B-404); every neutron star of the scan is described
        int ns = 0, bad = 0, withPlanets = 0, glassedSys = 0, belts = 0;
        std::vector<Body> glassedBodies;   // the first five, for the sheets' share
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_NEUTRON) continue;
            StarSystem sys; sys.generate(s); ns++; belts += (int)sys.belts.size();
            bool planets = false, glassed = false;
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type == PT_COMET || b.type == PT_COMPANION || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;   // a companion's worlds are its own class's
                if (b.type == PT_FELISIAN || b.type == PT_VENUSIAN || b.type == PT_GASGIANT || b.type == PT_OCEAN || b.type == PT_QUARTZ || b.type == PT_ACIDIC) bad++;
                if (b.parent < 0) planets = true;
                if (b.glassed) { glassed = true; if (glassedBodies.size() < 5) glassedBodies.push_back(b); }
            }
            if (planets) withPlanets++;
            if (glassed) glassedSys++;
        }
        check("neutron stars: some near home, none with a living or giant world", ns >= 5 && bad == 0, fmt("%d stars, %d such worlds, %d belts", ns, bad, belts));
        check("neutron stars: a glassed survivor in most systems with planets", withPlanets > 0 && glassedSys >= withPlanets * 0.6, fmt("%d of %d systems with planets", glassedSys, withPlanets));
        {   // the sheets: over the first five glassed worlds a share of the land between 15 and 85% on average (a crater-saturated small
            // world keeps little flat ground, a metal world half), and on each the same share at 512 m within 8 points
            double sum16 = 0, worst = 0; std::string names;
            for (const Body& gb : glassedBodies) {
                BodyGen g = BodyGen::make(gb);
                int land16 = 0, glass16 = 0, land512 = 0, glass512 = 0;
                for (int k = 0; k < 2000; k++) {
                    double u1 = unitFromHash(hash2i(k, 1, 0x61A5ULL)), u2 = unitFromHash(hash2i(k, 2, 0x61A5ULL));
                    Vec3 unit = StarSystem::bodyFromLatLon(std::asin(2 * u1 - 1), u2 * TAU);
                    SurfaceSample a = sampleSurface(g, unit, 16), b2 = sampleSurface(g, unit, 512);
                    if (a.material != MAT_WATER) { land16++; if (a.material == MAT_GLASS) glass16++; }
                    if (b2.material != MAT_WATER) { land512++; if (b2.material == MAT_GLASS) glass512++; }
                }
                double sh16 = land16 ? (double)glass16 / land16 : 0, sh512 = land512 ? (double)glass512 / land512 : 0;
                if (!g.hasTrait(TR_GLASSED)) worst = 1;
                sum16 += sh16; worst = std::max(worst, std::fabs(sh16 - sh512));
                names += fmt("%s%s %.0f/%.0f%%", names.empty() ? "" : ", ", PLANET_TYPES[gb.type].name, 100 * sh16, 100 * sh512);
            }
            double mean = glassedBodies.empty() ? 0 : sum16 / glassedBodies.size();
            check("glassed worlds: sheets over 15-85% of the land, the same at 512 m", glassedBodies.size() >= 3 && mean > 0.15 && mean < 0.85 && worst < 0.08,
                  fmt("%zu worlds, %.0f%% of the land on average, the 16 m and 512 m shares %.0f points apart at most (%s)", glassedBodies.size(), 100 * mean, 100 * worst, names.c_str()));
        }
    }
    {   // S-04: the protostar. A trickle along the arm (a few near home), a large share of the yellow family in a star-forming complex;
        // its systems are belts with at most two young worlds and no evolved one; its cloud glows toward the star from its worlds
        int armY = 0, armP = 0, nebY = 0, nebP = 0, ps = 0, withBelt = 0, evolved = 0, worlds = 0; bool nebFound = false;
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (!starInSector(x, y, z, s, false)) continue; if (s.cls == STAR_YELLOW) armY++; if (s.cls == STAR_PROTOSTAR) armP++; }
        for (int64_t x = HOME_SX - 200; x <= HOME_SX + 200 && !nebFound; x += 7) for (int64_t z = HOME_SZ - 200; z <= HOME_SZ + 200 && !nebFound; z += 7) {
            if (galaxyRegion(x, 0, z) != REGION_NEBULA) continue;
            nebFound = true;
            for (int64_t dx = -7; dx <= 7; dx++) for (int64_t dy = -7; dy <= 7; dy++) for (int64_t dz = -7; dz <= 7; dz++) {
                if (galaxyRegion(x + dx, dy, z + dz) != REGION_NEBULA) continue;
                Star s; if (!starInSector(x + dx, dy, z + dz, s, false)) continue;
                if (s.cls == STAR_YELLOW) nebY++; if (s.cls == STAR_PROTOSTAR) nebP++;
            }
        }
        double glow = 0; std::string first;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_PROTOSTAR) continue;
            StarSystem sys; sys.generate(s); ps++;
            if (!sys.belts.empty()) withBelt++;
            for (const Body& b : sys.bodies) {
                if (b.type == PT_COMET || b.type == PT_COMPANION || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;
                worlds++;
                if (b.type == PT_FELISIAN || b.type == PT_VENUSIAN || b.type == PT_QUARTZ || b.type == PT_OCEAN || b.type == PT_ACIDIC || b.type == PT_DESERT || b.type == PT_HYDROCARBON || b.type == PT_EUROPAN) evolved++;
                if (first.empty() && b.parent < 0) {
                    NebulaPatch np[4]; int nn = starNebulaPatches(s, Vec3(0, 0, 1), np); int tone;
                    glow = nebulaGlow(np, nn, Vec3(0.3, 0.1, 0.95), tone);   // a fifth of a radian from the star
                    first = fmt("%s (%d patches, tone %d)", PLANET_TYPES[b.type].name, nn, tone);
                }
            }
        }
        check("protostars: a trickle in the arm, half the yellow family in a complex", armP > 0 && armP < armY / 10 && nebFound && nebP > nebY / 2, fmt("near home %d against %d yellow stars; in the complex %d against %d", armP, armY, nebP, nebY));
        check("protostars: belts and young worlds only", ps >= 3 && withBelt >= ps * 0.85 && evolved == 0, fmt("%d stars, %d with a belt, %d worlds, %d evolved", ps, withBelt, worlds, evolved));
        check("protostar: its cloud glows round it", !first.empty() && glow > 0.4, fmt("glow %.2f a fifth of a radian from the star, the first world %s", glow, first.c_str()));
    }
    {   // S-05: the Wolf-Rayet star. A share of the blue giants that stayed near home (rare; none in a globular), systems of few
        // bare worlds under a wind that stripped every atmosphere, and the shell of shed gas drawn as a ring round the sun
        int armB = 0, armW = 0, globN = 0, globW = 0, ws = 0, worlds = 0, aired = 0, bomb = 0, withPlanets = 0, withBelt = 0, giants = 0;
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (!starInSector(x, y, z, s, false)) continue; if (s.cls == STAR_BLUE_GIANT) armB++; if (s.cls == STAR_WOLF_RAYET) armW++; }
        for (int64_t x = 485; x <= 499; x++) for (int64_t y = -5854; y <= -5840; y++) for (int64_t z = 2374; z <= 2388; z++) {   // the nearest globular (`galaxy`), only its own sectors
            if (galaxyRegion(x, y, z) != REGION_CLUSTER) continue;
            Star s; if (!starInSector(x, y, z, s, false)) continue;
            globN++; if (s.cls == STAR_WOLF_RAYET) globW++;
        }
        Star w; bool found = false;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_WOLF_RAYET) continue;
            if (!found) { starInSector(x, 0, z, w, true); found = true; }
            StarSystem sys; sys.generate(s); ws++;
            bool any = false, hasBomb = false;
            for (const Body& b : sys.bodies) {
                if (b.type == PT_COMET || b.type == PT_COMPANION || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;
                worlds++; any = true;
                if (b.type == PT_GASGIANT || b.type == PT_SUBSTELLAR) giants++;
                else if (PLANET_TYPES[b.type].atmosphere) aired++;
                if (b.type == PT_BOMBARDED) hasBomb = true;
            }
            if (any) { withPlanets++; if (hasBomb) bomb++; }
            if (!sys.belts.empty()) withBelt++;
        }
        check("Wolf-Rayet stars: rare in the arm, none in a globular", armW > 0 && armW * 2 < armB && globN > 100 && globW == 0, fmt("near home %d against %d blue giants; %d of the globular's %d stars", armW, armB, globW, globN));
        check("Wolf-Rayet stars: bare worlds only, bombarded in most systems", ws >= 3 && aired == 0 && bomb * 2 >= withPlanets, fmt("%d stars, %d worlds (%d giants), %d with an atmosphere, bombarded in %d of %d systems with planets, %d with a belt", ws, worlds, giants, aired, bomb, withPlanets, withBelt));
        // the shell: the sun rendered straight ahead as from the second orbit; the shade along a radius peaks at the ring (five radii)
        // over the glow's skirt either side of it
        Framebuffer fb; fb.clear(); fb.clearDepth();
        Proj pj = Proj::fromHFov(60);
        const double angR = 0.03; Vec3 up(0, 1, 0);
        if (found) SpaceRenderer::drawSun(fb, w, Vec3(0, 0, 1), angR, 1234.0, 1.0, 1, false, 0, false, pj, &up);
        auto radial = [&](double mult) { double r = pj.f * std::tan(angR * mult), sum = 0; int n = 0; for (int k = 0; k < 360; k += 15) { int x = (int)(pj.cx + r * std::cos(k * DEG)), y = (int)(pj.cy + r * std::sin(k * DEG)); if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue; sum += shadeOf(fb.at(x, y)); n++; } return n ? sum / n : 0.0; };
        double atRing = radial(5.0), inside = radial(3.2), outside = radial(7.0);
        check("Wolf-Rayet star: the shell's ring round the sun", found && atRing > inside + 3 && atRing > outside + 3, fmt("shade %.1f at 5 radii, %.1f at 3.2, %.1f at 7 (%s)", atRing, inside, outside, found ? w.name.c_str() : "no star"));
    }
    {   // S-06: the black hole. A share of the pulsars that stayed, most in the core; systems of rubble and rocks with a close companion
        // in half; and the lens: a star right behind the hole becomes an Einstein ring, the shadow is black, the far sky untouched
        int armP = 0, armB = 0, armN = 0, coreN = 0, coreB = 0, bs = 0, worlds = 0, bad = 0, comps = 0, closeComps = 0, withBelt = 0;
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (!starInSector(x, y, z, s, false)) continue; armN++; if (s.cls == STAR_PULSAR) armP++; if (s.cls == STAR_BLACK_HOLE) armB++; }
        for (int64_t x = 12293; x <= 12307; x++) for (int64_t y = -7; y <= 7; y++) for (int64_t z = -5507; z <= -5493; z++) {   // the core cube of `galaxy`
            if (galaxyRegion(x, y, z) != REGION_CORE) continue;
            Star s; if (!starInSector(x, y, z, s, false)) continue;
            coreN++; if (s.cls == STAR_BLACK_HOLE) coreB++;
        }
        Star bh; bool found = false;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_BLACK_HOLE) continue;
            if (!found) { starInSector(x, 0, z, bh, true); found = true; }
            StarSystem sys; sys.generate(s); bs++;
            for (const Body& b : sys.bodies) {
                if (b.type == PT_COMET || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;
                if (b.type == PT_COMPANION) { comps++; if (b.orbitRadiusKm < 1e8) closeComps++; continue; }
                worlds++;
                if (PLANET_TYPES[b.type].atmosphere || b.type == PT_GASGIANT || b.type == PT_SUBSTELLAR || b.type == PT_MOLTEN) bad++;
            }
            if (!sys.belts.empty()) withBelt++;
        }
        double armShare = armN ? (double)armB / armN : 0, coreShare = coreN ? (double)coreB / coreN : 0;
        check("black holes: a few near home, several times as many in the core", armB > 0 && armB * 4 < armP && coreN > 1000 && coreShare > 2.5 * armShare, fmt("near home %d against %d pulsars (%.2f%% of %d stars); in the core %d of %d (%.2f%%)", armB, armP, 100 * armShare, armN, coreB, coreN, 100 * coreShare));
        check("black holes: rocks, rubble and a close companion", bs >= 3 && bad == 0 && comps > 0 && closeComps == comps && withBelt * 2 >= bs, fmt("%d stars, %d worlds, %d with an air, a giant or lava, %d companions (%d close), %d with a belt", bs, worlds, bad, comps, closeComps, withBelt));
        // the lens: a star dot straight behind the hole and the sky dark elsewhere; after the hole the dot is a ring at the Einstein
        // angle, the shadow is black and the sky outside the region keeps its dark
        Framebuffer fb; fb.clear(); fb.clearDepth();
        Proj pj = Proj::fromHFov(60);
        const double angR = 0.02, thE = std::sqrt(angR);
        int cx = (int)pj.cx, cy = (int)pj.cy;
        for (int dy = -2 * FB_SCALE; dy <= 2 * FB_SCALE; dy++) for (int dx = -2 * FB_SCALE; dx <= 2 * FB_SCALE; dx++) fb.at(cx + dx, cy + dy) = pix(0, 60);   // a dot of two logical pixels' radius
        Vec3 up(0, 1, 0);   // the disc edge-on, a line along x through the hole: the ring samples on that axis are skipped (the disc is opaque)
        auto t0 = std::chrono::steady_clock::now();
        if (found) SpaceRenderer::drawSun(fb, bh, Vec3(0, 0, 1), angR, 1234.0, 1.0, 1, false, 0, false, pj, &up);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        int ringLit = 0, ringN = 0; double rE = pj.f * std::tan(thE);
        auto maxAround = [&](int x, int y) { double m = 0; for (int dy = -FB_SCALE; dy <= FB_SCALE; dy++) for (int dx = -FB_SCALE; dx <= FB_SCALE; dx++) { int xx = x + dx, yy = y + dy; if (xx < 0 || yy < 0 || xx >= FBW || yy >= FBH) continue; m = std::max(m, shadeOf(fb.at(xx, yy))); } return m; };
        for (int k = 30; k < 360; k += 30) { if (k == 180) continue; int x = (int)(pj.cx + rE * std::cos(k * DEG)), y = (int)(pj.cy + rE * std::sin(k * DEG)); if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue; ringN++; if (maxAround(x, y) > 30) ringLit++; }
        double centre = shadeOf(fb.at(cx, cy)), shadowEdge = shadeOf(fb.at(cx + (int)(pj.f * std::tan(2.2 * angR)), cy));
        double far = shadeOf(fb.at(cx + (int)(pj.f * std::tan(3.0 * thE)), cy));
        check("black hole: the star behind it becomes an Einstein ring, the shadow black", found && ringN >= 8 && ringLit >= ringN - 1 && centre < 1 && shadowEdge < 1 && far < 1, fmt("%d of %d ring samples lit, the centre %.1f, the shadow's edge %.1f, far %.1f; %.1f ms at %dx (%s)", ringLit, ringN, centre, shadowEdge, far, ms, FB_SCALE, found ? bh.name.c_str() : "no black hole"));
    }
    {   // C-01: the civilisations. One felisian or desert world in eight near home had a people; a dead desert world keeps its seas as dry
        // beds the wadis run down to; its settlements are hamlets, villages and towns whose houses are rooms to walk into
        int fel = 0, felC = 0, des = 0, desC = 0, bodyFel = -1, bodyDes = -1; Star firstFel, firstDes; bool haveFel = false, haveDes = false;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN && b.type != PT_DESERT) continue;
                bool civ = BodyGen::make(b).hasTrait(TR_CIVILISATION);
                if (b.type == PT_FELISIAN) { fel++; if (civ) { felC++; if (!haveFel) { starInSector(x, 0, z, firstFel, true); bodyFel = bi; haveFel = true; } } }
                else { des++; if (civ) { desC++; if (!haveDes) { starInSector(x, 0, z, firstDes, true); bodyDes = bi; haveDes = true; } } }
            }
        }
        check("civilisations: one felisian or desert world in eight", felC > 0 && desC > 0 && felC * 100 >= fel * 5 && felC * 100 <= fel * 22 && desC * 100 >= des * 5 && desC * 100 <= des * 22, fmt("felisian %d of %d, desert %d of %d", felC, fel, desC, des));
        if (haveDes) {   // the old sea: a fifth to two thirds of the face, its bed under the land, salt in its deeps; at its shore the flood's outlet and the wadis
            StarSystem sys; sys.generate(firstDes);
            const Body& b = sys.bodies[bodyDes];
            BodyGen g = BodyGen::make(b);
            int n = 0, sea = 0, salt = 0, nLand = 0; double hSea = 0, hLand = 0; bool shoreFound = false; Vec3 shoreU;
            for (int j = 0; j < 36; j++) for (int i = 0; i < 72; i++) {
                double lat = (-85 + 170.0 * (j + 0.5) / 36) * DEG, lon = (-180 + 360.0 * (i + 0.5) / 72) * DEG;
                Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
                SurfaceSample s = sampleSurface(g, u, 2048);
                n++;
                if (s.oldSea > 0.5) { sea++; hSea += s.height; if (s.material == MAT_SALT) salt++; } else { nLand++; hLand += s.height; }
                if (!shoreFound && s.oldSea > 0.35 && s.oldSea < 0.65 && std::fabs(lat) < 60 * DEG) { shoreFound = true; shoreU = u; }
            }
            hSea /= std::max(1, sea); hLand /= std::max(1, nLand);
            DrainStats st; if (shoreFound) { bool wasOn = drainageEnabled(); setDrainageEnabled(true); st = drainageStats(g, shoreU, 40); setDrainageEnabled(wasOn); }   // an earlier check leaves the drainage off
            check("dead desert world: dry seas, the bed under the land, wadis to the shore", sea * 100 >= n * 15 && sea * 100 <= n * 70 && hSea < hLand && salt > 0 && shoreFound && st.sea > 0 && st.channels > 0,
                  fmt("%s (the sea stood at %.0f m): the old sea on %d%% of %d points (salt on %d), its bed %.0f m against the land's %.0f; at the shore %d sea cells, %d channels of %d", b.name.c_str(), g.oldSeaM, sea * 100 / n, n, salt, hSea, hLand, st.sea, st.channels, st.cells));
        } else check("dead desert world: dry seas", false, "no dead desert world in the scan");
        if (haveFel || haveDes) {   // the settlements of the first world: the classes' shares, every house a room with a doorway (the pieces), and the view's colliders
            StarSystem sys; sys.generate(haveFel ? firstFel : firstDes);
            int bi = haveFel ? bodyFel : bodyDes;
            const Body& b = sys.bodies[bi];
            BodyGen g = BodyGen::make(b);
            Culture cu = cultureOf(g);
            int cls[4] = {0, 0, 0, 0}, mono = 0, cells = 0, houses = 0, roomsOk = 0, doorsOk = 0, gLat0, gLon0;
            int shSettle = 0, shTotal = 0, shByClass[4] = {0, 0, 0, 0}, shCountBad = 0, shBad = 0, shDup = 0, shOut = 0, shBlocked = 0, shDoor = 0, shPlaces[SHARD_PLACE_COUNT] = {0, 0, 0, 0, 0}; bool shDeterm = true;   // C-03
            ruinCellOf(g, 0.2, 0.7, gLat0, gLon0);
            auto inside = [](double px, double pz, const RuinElem& e) { double dx = px - e.x, dz = pz - e.z; double qx = dx * std::cos(e.heading) - dz * std::sin(e.heading), qz = dx * std::sin(e.heading) + dz * std::cos(e.heading); return std::fabs(qx) < e.hx && std::fabs(qz) < e.hz; };
            auto doorAxis = [](const Building& bd, double& cx, double& cz, double& ax, double& az, double& L) {
                int side = bd.door; bool along = side == 0 || side == 2; L = along ? bd.hw : bd.hd;
                double lx0 = side == 1 ? bd.hw : (side == 3 ? -bd.hw : 0), lz0 = side == 0 ? bd.hd : (side == 2 ? -bd.hd : 0);
                double sx = std::cos(bd.heading), sz = -std::sin(bd.heading), fx = std::sin(bd.heading), fz = std::cos(bd.heading);
                cx = bd.x + sx * lx0 + fx * lz0; cz = bd.z + sz * lx0 + fz * lz0; ax = along ? sx : -fx; az = along ? sz : -fz;
            };
            std::vector<RuinElem> el;
            for (int dl = -15; dl <= 15; dl++) for (int dn = -15; dn <= 15; dn++) {
                RuinSpec sp; cells++;
                if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, true)) continue;
                if (sp.kind != RK_SETTLEMENT) { mono++; continue; }
                cls[sp.sclass]++;
                ruinElements(sp, cu, 0, el);
                {   // C-03: the settlement's shards: counted by the class, distinct, inside their building, clear of its pieces and its doorway, the same twice
                    std::vector<ShardSite> ss, ss2; shardSitesOf(sp, cu, ss); shardSitesOf(sp, cu, ss2);
                    shSettle++; shTotal += (int)ss.size(); shByClass[sp.sclass] += (int)ss.size();
                    int lo = sp.sclass == SC_VILLAGE ? 1 : (sp.sclass == SC_TOWN ? 2 : 0), hi = sp.sclass == SC_TOWN ? 4 : (sp.sclass == SC_VILLAGE ? 2 : 1);
                    if ((int)ss.size() < lo || (int)ss.size() > hi) shCountBad++;
                    if (ss.size() != ss2.size()) shDeterm = false; else for (size_t i = 0; i < ss.size(); i++) if (ss[i].index != ss2[i].index || ss[i].x != ss2[i].x || ss[i].z != ss2[i].z) shDeterm = false;
                    for (size_t i = 0; i < ss.size(); i++) {
                        const ShardSite& s = ss[i];
                        if (s.index < 0 || s.index >= SHARDS_PER_WORLD || s.place < 0 || s.place >= SHARD_PLACE_COUNT) { shBad++; continue; }
                        for (size_t j = 0; j < i; j++) if (ss[j].index == s.index) shDup++;
                        if (s.building < 0 || s.building >= (int)sp.buildings.size()) { shBad++; continue; }
                        const Building& bd = sp.buildings[s.building];
                        double dx = s.x - bd.x, dz = s.z - bd.z, qx = dx * std::cos(bd.heading) - dz * std::sin(bd.heading), qz = dx * std::sin(bd.heading) + dz * std::cos(bd.heading);
                        bool in = bd.kind == BK_STELA ? std::hypot(dx, dz) < 1.2 : (bd.kind == BK_ROTUNDA ? std::hypot(dx, dz) < bd.hw - 0.4 : (std::fabs(qx) < bd.hw - 0.5 && std::fabs(qz) < bd.hd - 0.5));
                        if (!in) shOut++;
                        shPlaces[s.place]++;
                        for (const RuinElem& e : el) { if (e.building != s.building || e.shape != 0 || e.y0 > 1.0) continue; double ex = s.x - e.x, ez = s.z - e.z; double px = ex * std::cos(e.heading) - ez * std::sin(e.heading), pz = ex * std::sin(e.heading) + ez * std::cos(e.heading); if (std::fabs(px) < e.hx + 0.4 && std::fabs(pz) < e.hz + 0.4) { shBlocked++; break; } }
                        double ddx, ddz; if (buildingDoor(bd, ddx, ddz) && std::hypot(s.x - ddx, s.z - ddz) < 1.4) shDoor++;
                    }
                }
                for (size_t k = 0; k < sp.buildings.size(); k++) {
                    const Building& bd = sp.buildings[k];
                    if (bd.kind != BK_HOUSE) continue;
                    houses++;
                    bool clear = true;
                    for (const RuinElem& e : el) { if (e.building != (int)k || e.shape != 0 || e.part == 4 || e.y0 > 1.2) continue; double dx = bd.x - e.x, dz = bd.z - e.z; double qx = dx * std::cos(e.heading) - dz * std::sin(e.heading), qz = dx * std::sin(e.heading) + dz * std::cos(e.heading); if (std::fabs(qx) < e.hx + 0.9 && std::fabs(qz) < e.hz + 0.9) clear = false; }
                    if (clear) roomsOk++;
                    double cx, cz, ax, az, L; doorAxis(bd, cx, cz, ax, az, L);
                    int run = 0, bestRun = 0;
                    for (double tt = -L; tt <= L; tt += 0.1) {
                        bool in = false;
                        for (const RuinElem& e : el) { if (e.building != (int)k || e.shape != 0 || e.part == 4 || e.y0 > 1.2) continue; if (inside(cx + ax * tt, cz + az * tt, e)) { in = true; break; } }
                        run = in ? 0 : run + 1; bestRun = std::max(bestRun, run);
                    }
                    if (bestRun * 0.1 >= 1.6) doorsOk++;
                }
            }
            check("settlements: hamlets, villages, towns and lone monuments", cls[0] > cls[1] && cls[1] >= cls[2] && cls[2] > 0 && mono + cls[3] > 0, fmt("%s: %d hamlets, %d villages, %d towns, %d monuments, %d monoliths of the old ones in %d cells", b.name.c_str(), cls[0], cls[1], cls[2], cls[3], mono, cells));
            check("settlements: every house a room with a doorway", houses > 0 && roomsOk == houses && doorsOk == houses, fmt("%d houses, %d with the centre clear, %d with a doorway of 1.6 m", houses, roomsOk, doorsOk));
            check("shards: a few per settlement, in a room or at a stela, clear of the pieces and the doorway, distinct, deterministic",   // C-03
                  shSettle > 0 && shTotal > 0 && shCountBad == 0 && shBad == 0 && shDup == 0 && shOut == 0 && shBlocked == 0 && shDoor == 0 && shDeterm && shByClass[2] > 0 && shTotal * 10 >= shSettle * 5,
                  fmt("%d shards in %d settlements (in hamlets %d, villages %d, towns %d, monuments %d; in houses %d, halls %d, towers %d, rotundas %d, at stelae %d); %d counts out of bounds, %d out of their building, %d in a piece, %d in a doorway, %d repeats, %s",
                      shTotal, shSettle, shByClass[0], shByClass[1], shByClass[2], shByClass[3], shPlaces[0], shPlaces[1], shPlaces[2], shPlaces[3], shPlaces[4], shCountBad, shOut, shBlocked, shDoor, shDup, shDeterm ? "the same twice" : "NOT deterministic"));
            {   // the view: the first town's houses, the colliders leaving the centre clear and the doorway open; the town drawn from outside its edge, timed
                int tLat = 0, tLon = 0; bool tFound = false; RuinSpec tsp;
                {   // from a point on low land (the fixed start of the counts above may lie in an ocean): the first of a 2048 m grid under 40 degrees of latitude
                    int lLat = gLat0, lLon = gLon0; bool land = false;
                    for (int j = 0; j < 36 && !land; j++) for (int i = 0; i < 72 && !land; i++) {
                        double la = (-85 + 170.0 * (j + 0.5) / 36) * DEG, lo = (-180 + 360.0 * (i + 0.5) / 72) * DEG;
                        if (std::fabs(la) > 40 * DEG) continue;
                        SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 2048);
                        if (ss.material != MAT_WATER && ss.height > 10 && ss.relief < 0.3 && ss.oldSea < 0.5) { ruinCellOf(g, la, lo, lLat, lLon); land = true; }
                    }
                    for (int ring = 0; ring < 40 && !tFound; ring++) for (int dl = -ring; dl <= ring && !tFound; dl++) for (int dn = -ring; dn <= ring && !tFound; dn++) {
                        if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                        RuinSpec sp; if (ruinOfCell(g, lLat + dl, lLon + dn, sp, false) && sp.kind == RK_SETTLEMENT && sp.sclass == SC_TOWN && ruinSiteOk(g, sp)) { tLat = lLat + dl; tLon = lLon + dn; tsp = sp; tFound = true; }
                    }
                    if (!tFound) printf("    (no town passed the site test within 40 rings of %d:%d%s)\n", lLat, lLon, land ? "" : ", no low land found");
                }
                int clearN = 0, openN = 0, hN = 0, spanN = 0, spanBad = 0; double ms = 0, tallest = 0; int pieces = 0; std::string where = tFound ? "the view found no town at the cell" : "no town within 40 rings of low land";
                int vShards = 0, vClear = 0, vRocks = 0, vDrawn = 0; bool vReach = false, vGone = false;   // C-03
                if (tFound) {
                    SurfaceView sv;
                    sv.init(&sys, bi, tsp.lat, tsp.lon, 0.0);
                    Ruin ru;
                    if (sv.ruinAt(tLat, tLon, ru) && ru.kind == RK_SETTLEMENT) {
                        std::vector<SurfaceView::Collider> cols;
                        for (size_t k = 0; k < ru.spec->buildings.size(); k++) {
                            const Building& bd = ru.spec->buildings[k];
                            if (bd.kind != BK_HOUSE) continue;
                            hN++;
                            double hx = ru.x + bd.x, hz = ru.z + bd.z;
                            sv.collectColliders(hx, hz, cols);
                            bool clear = true;
                            for (const auto& c : cols) if (c.kind == 3 && std::hypot(c.x - hx, c.z - hz) < c.r + 0.9) clear = false;
                            // B-405: every piece's collider spans its own height over the ground it stands on, so the jetpack clears what it has risen above
                            for (const auto& c : cols) {
                                if (c.kind != 3) continue;
                                double gnd = sv.site.groundHeight(c.x, c.z); spanN++;
                                if (c.y1 - c.y0 < 0.85 || c.y1 - c.y0 > 30 || std::fabs(c.y0 - gnd) > 8) spanBad++;
                                tallest = std::max(tallest, c.y1 - gnd);
                            }
                            if (clear) clearN++;
                            double cx, cz, ax, az, L; doorAxis(bd, cx, cz, ax, az, L);
                            int run = 0, bestRun = 0;
                            for (double tt = -L; tt <= L; tt += 0.1) {
                                double px = ru.x + cx + ax * tt, pz = ru.z + cz + az * tt; bool blocked = false;
                                for (const auto& c : cols) if (c.kind == 3 && std::hypot(c.x - px, c.z - pz) < c.r + 0.35) { blocked = true; break; }
                                run = blocked ? 0 : run + 1; bestRun = std::max(bestRun, run);
                            }
                            if (bestRun * 0.1 >= 1.0) openN++;
                        }
                        {   // C-03: the town's shards in the view: no piece's collider within 0.45 m of a site (the explorer can stand at it), the first
                            // within reach when the explorer stands a metre from it, drawn from there, and neither offered once taken
                            const SurfaceView::RuinCell* rc = sv.ruinCell(tLat, tLon);
                            vShards = (int)rc->shards.size();
                            for (const ShardSite& s : rc->shards) {
                                double x = ru.x + s.x, z = ru.z + s.z, gnd = sv.site.groundHeight(x, z);
                                sv.collectColliders(x, z, cols);
                                bool clear = true;
                                for (const auto& c : cols) { if (std::hypot(c.x - x, c.z - z) >= c.r + 0.45 || c.y0 > gnd + 1.0) continue; if (c.kind == 3) clear = false; else vRocks++; }
                                if (clear) vClear++;
                            }
                            if (!rc->shards.empty()) {
                                const ShardSite& s = rc->shards[0]; const Building& bd = rc->spec.buildings[s.building];   // (`tsp` was read without its buildings)
                                double x = ru.x + s.x, z = ru.z + s.z, ax = bd.x - s.x, az = bd.z - s.z, L = std::max(1e-6, std::sqrt(ax * ax + az * az));
                                if (s.place == SHARD_AT_STELA) { ax = -ax; az = -az; }
                                sv.player.x = x + ax / L; sv.player.z = z + az / L; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                                sv.player.yaw = std::atan2(x - sv.player.x, z - sv.player.z); sv.player.pitch = -0.9;
                                Input in0; sv.update(0.016, in0, 0.0, false);
                                vReach = sv.nearShard.index == s.index;
                                Framebuffer fb0; SpaceRenderer sr0; StarNeighborhood nb0;
                                sv.render(fb0, 0.0, nb0.stars, sr0, 1.0);
                                vDrawn = sv.lastShardsDrawn;
                                saveFB(fb0, "shots/tests/unit_shard_room.png");
                                sv.shardsFound.insert(s.index); sv.update(0.016, in0, 0.0, false); vGone = sv.nearShard.index != s.index; sv.shardsFound.clear();
                            }
                        }
                        sv.player.x = ru.x - tsp.size - 30; sv.player.z = ru.z; sv.player.yaw = PI / 2; sv.player.pitch = 0.02;
                        Framebuffer fb; SpaceRenderer sr; StarNeighborhood nb;
                        sv.render(fb, 0.0, nb.stars, sr, 1.0);
                        ms = sv.lastRuinMs; pieces = sv.lastRuinElems;
                        where = fmt("%s at %.2f %.2f", b.name.c_str(), tsp.lat / DEG, tsp.lon / DEG);
                    }
                }
                check("settlements: the colliders leave the rooms and doorways open and stand as tall as their walls", tFound && hN > 0 && clearN == hN && openN == hN && spanN > 0 && spanBad == 0 && tallest > 0.9 && tallest < 30,
                      fmt("%s: %d houses of a town, %d clear, %d open; %d colliders, %d without a wall's span, the tallest %.1f m over its ground; the town drawn in %.1f ms (%d pieces)", where.c_str(), hN, clearN, openN, spanN, spanBad, tallest, ms, pieces));
                check("shards: the town's sites are clear in the view, within reach at a metre, drawn, and gone once taken", tFound && vShards >= 2 && vClear == vShards && vReach && vDrawn >= 1 && vGone,   // C-03
                      fmt("%d shards in the town, %d clear of the pieces' colliders (%d rocks on them), the first %s within reach at 1 m, %d drawn from inside (shots/tests/unit_shard_room.png), %s", vShards, vClear, vRocks, vReach ? "is" : "is NOT", vDrawn, vGone ? "gone once taken" : "STILL offered once taken"));

            }
        }
    }
    {   // C-02: the story grammar on the two civilisation worlds of C-01 (Aieliaalas II, Leileashphail III), then across the worlds near home
        struct W { int64_t sx, sz; int bi; };
        const W ws[2] = {{151, 25, 1}, {153, 70, 2}};
        int okWorlds = 0, catPct[2] = {0, 0}, lorePct[2] = {0, 0}, seaPct = 0, minW = 999, maxW = 0, musicN[2] = {0, 0};
        bool determ = true, clean = true, lengths = true, distinct = true;
        bool tradSame = true, tradOk = true, piecesOk = true, pieceSame = true, formsVary = true, tradDiffer = true; int pieceMin = 999, pieceMax = 0, notesMin = 99999, noteTot = 0, badOrder = 0, badTime = 0, badVel = 0, badDeg = 0, badVoice = 0; Tradition trads[2];   // C-04
        double readPct[2][11] = {}; bool mono = true, namesKnown = true, tongueSame = true, tongueDistinct = true; int langWords[2] = {0, 0};   // C-06
        bool voiceSame = true, voiceOk = true, speechSame = true, speechOk = true, aligned = true, inventoryOk = true; int badSeg = 0, badWord = 0, speechN = 0; double speechMin = 999, speechMax = 0, speechTot = 0; Voice voices[2];   // C-05
        for (int wi = 0; wi < 2; wi++) {
            Star s; if (!starInSector(ws[wi].sx, 0, ws[wi].sz, s, true)) continue;
            StarSystem sys; sys.generate(s);
            if (ws[wi].bi >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[ws[wi].bi];
            BodyGen g = BodyGen::make(b);
            if (!g.hasTrait(TR_CIVILISATION)) continue;
            okWorlds++;
            Lore L = loreOf(sys, b, g);
            std::vector<Shard> sh; shardsOf(sys, b, g, L, 50, sh);
            std::set<std::string> texts;
            int cat = 0, lore = 0, sea = 0, textN = 0;
            const std::string names[] = {L.people, L.god, L.river, L.mountain, L.sea, L.city, L.city2, L.founder, L.moon, L.star, L.festival};
            for (int i = 0; i < 50; i++) {
                const Shard& x = sh[i];
                Shard y = shardOf(sys, b, g, L, i);
                if (y.text != x.text || y.child != x.child || y.music != x.music) determ = false;
                if (x.tone >= ST_WARNING) cat++;
                if (x.music) { musicN[wi]++; continue; }   // C-04: a piece, no text
                textN++;
                if (x.text.find_first_of("{}[]|") != std::string::npos) clean = false;
                int nw = shardWords(x.text); minW = std::min(minW, nw); maxW = std::max(maxW, nw);
                if (nw < 8 || nw > 70) lengths = false;
                texts.insert(x.text);
                bool hasName = false; for (const std::string& nm : names) if (x.text.find(nm) != std::string::npos) hasName = true;
                if (hasName) lore++;
                std::string lt = x.text; for (char& c : lt) c = (char)tolower((unsigned char)c);
                if (lt.find("sea") != std::string::npos || lt.find("salt") != std::string::npos || x.text.find(L.river) != std::string::npos) sea++;
            }
            if ((int)texts.size() < textN) distinct = false;
            catPct[wi] = cat * 2; lorePct[wi] = 100 * lore / std::max(1, textN); if (wi == 1) seaPct = 100 * sea / std::max(1, textN);
            {   // C-04: the tradition the same twice, its scale five to nine notes ascending with 70 cents between neighbours, its cycle's groups summing, two or three voices; the
                // pieces the same twice, 15-90 s, notes in order and within the range, their forms varied; the two worlds' traditions differ
                Tradition T = traditionOf(g, L), T2 = traditionOf(g, L); trads[wi] = T;
                if (T.scale != T2.scale || T.groups != T2.groups || T.bpm != T2.bpm || T.voices.size() != T2.voices.size()) tradSame = false;
                int n = (int)T.scale.size(); int gsum = 0; for (int gb : T.groups) gsum += gb;
                if (n < 5 || n > 9 || gsum != T.beats || T.voices.size() < 2 || T.voices.size() > 3 || T.scale[0] != 1.0 || T.rest <= 0 || T.rest >= n) tradOk = false;
                for (int d = 1; d < n && tradOk; d++) if (traditionCents(T, d) - traditionCents(T, d - 1) < 70) tradOk = false;
                if (1200 * std::log2(T.period) - traditionCents(T, n - 1) < 70) tradOk = false;
                std::set<int> forms;
                for (int i = 0; i < 50; i++) {
                    if (!sh[i].music) continue;
                    Piece P, P2; pieceOf(T, sh[i], P); pieceOf(T, sh[i], P2);
                    if (P.notes.size() != P2.notes.size() || P.form != P2.form || P.bpm != P2.bpm) pieceSame = false;
                    forms.insert(P.form);
                    pieceMin = std::min(pieceMin, (int)P.seconds); pieceMax = std::max(pieceMax, (int)P.seconds); notesMin = std::min(notesMin, (int)P.notes.size()); noteTot += (int)P.notes.size();
                    if (P.seconds < 15 || P.seconds > 90 || P.notes.size() < 20 || P.form < 0 || P.form >= PF_COUNT) piecesOk = false;
                    for (size_t k = 0; k < P.notes.size(); k++) {
                        const Note& nt = P.notes[k];
                        if (k > 0 && nt.t < P.notes[k - 1].t) badOrder++;
                        if (nt.t < 0 || nt.t + nt.dur > P.beats + 1e-6 || nt.dur <= 0) badTime++;
                        if (nt.vel <= 0 || nt.vel > 1.0001) badVel++;
                        if (nt.voice >= 0 && (nt.degree < -3 * n || nt.degree > 3 * n)) badDeg++;
                        if (nt.voice >= (int)T.voices.size()) badVoice++;
                    }
                    if (badOrder || badTime || badVel || badDeg || badVoice) piecesOk = false;
                }
                if (forms.size() < 3) formsVary = false;
            }
            {   // C-06: the decoding: the share read with one to ten shards held, never fewer per shard with more held, the names from the start, the tongue the same twice and its forty most used words distinct
                Tongue T = tongueOf(g, L), T2 = tongueOf(g, L);
                Language lang; languageOf(L, sh, T, lang); langWords[wi] = (int)lang.words.size();
                std::vector<int> prev(50, 0);
                for (int held = 1; held <= 10; held++) {
                    int read = 0, tot = 0;
                    for (int i = 0; i < 50; i++) {
                        std::vector<DecodedWord> w; int r = decodeShard(sh[i], lang, T, held, w);
                        read += r; tot += (int)w.size();
                        if (r < prev[i]) mono = false; prev[i] = r;
                        if (held == 1) for (const DecodedWord& d : w) { std::string lw = d.ours; for (char& c : lw) c = (char)tolower((unsigned char)c); if (lang.names.count(lw) && !(d.name && d.known)) namesKnown = false; }
                    }
                    readPct[wi][held] = tot > 0 ? 100.0 * read / tot : 0;
                }
                for (int k = 0; k < (int)lang.words.size() && k < 40; k++) {
                    if (tongueWord(T, lang.words[k]) != tongueWord(T2, lang.words[k])) tongueSame = false;
                    for (int j = 0; j < k; j++) if (lang.theirs[k] == lang.theirs[j]) tongueDistinct = false;
                }
                {   // C-05: the voice the same twice and in bounds, its inventory the tongue's sounds; every text's speech the same twice, 4-45 s, its segments in order
                    // without a gap or an overlap, finite and in pitch, every word with sounds spoken within its span in order, the spans the same at one shard held and at ten
                    Voice V = voiceOf(g, L, T), V2 = voiceOf(g, L, T); voices[wi] = V;
                    if (V.pitch != V2.pitch || V.rate != V2.rate || V.stress != V2.stress || V.inventory != V2.inventory) voiceSame = false;
                    if (V.pitch < 80 || V.pitch > 320 || V.rate < 2.5 || V.rate > 5.6 || V.tract < 0.7 || V.tract > 1.4 || V.range < 0.4 || V.range > 1.7 || V.inventory.size() < 8 || voiceLine(V).size() > 52) voiceOk = false;
                    for (int p : V.inventory) if (p < 0 || p >= phoneCount()) inventoryOk = false;
                    bool vowel = false; for (int p : V.inventory) if (PHONES[p].kind == PK_VOWEL) vowel = true;
                    if (!vowel) inventoryOk = false;
                    for (int i = 0; i < 50; i++) {
                        if (sh[i].music) continue;
                        std::vector<DecodedWord> w1, w10; decodeShard(sh[i], lang, T, 1, w1); decodeShard(sh[i], lang, T, 10, w10);
                        Speech S, S2, S10; speechOf(V, w10, sh[i].seed, S); speechOf(V, w10, sh[i].seed, S2); speechOf(V, w1, sh[i].seed, S10);
                        speechN++; speechMin = std::min(speechMin, S.seconds); speechMax = std::max(speechMax, S.seconds); speechTot += S.seconds;
                        if (S.segs.size() != S2.segs.size() || S.seconds != S2.seconds || S.syllables != S2.syllables) speechSame = false;
                        if (S.wordStart != S10.wordStart || S.wordEnd != S10.wordEnd) aligned = false;   // the speech does not depend on the share read
                        if (S.seconds < 4 || S.seconds > 45 || S.syllables < (int)w10.size() || S.segs.empty()) speechOk = false;
                        double tEnd = 0;
                        for (size_t k = 0; k < S.segs.size(); k++) {
                            const Segment& sg = S.segs[k];
                            if (!std::isfinite(sg.t) || !std::isfinite(sg.dur) || sg.dur <= 0 || std::fabs(sg.t - tEnd) > 1e-6 || sg.pitch < 0.4 || sg.pitch > 2.5 || sg.pitchEnd < 0.4 || sg.pitchEnd > 2.5 || sg.amp < 0 || sg.amp > 2 || sg.phone >= phoneCount()) badSeg++;
                            if (sg.phone >= 0 && (sg.word < 0 || sg.word >= (int)w10.size() || sg.t < S.wordStart[sg.word] - 1e-6 || sg.t + sg.dur > S.wordEnd[sg.word] + 1e-6)) badWord++;
                            tEnd = sg.t + sg.dur;
                        }
                        if (std::fabs(tEnd - S.seconds) > 1e-6) badSeg++;
                        for (size_t k = 0; k < w10.size(); k++) {
                            if (S.wordEnd[k] < S.wordStart[k] || (k > 0 && S.wordStart[k] < S.wordEnd[k - 1] - 1e-6)) badWord++;
                            std::string core = w10[k].ours; bool letters = false; for (char c : core) if (std::isalpha((unsigned char)c)) letters = true;
                            if (letters && S.wordEnd[k] <= S.wordStart[k]) badWord++;   // a word with letters has sounds
                        }
                    }
                    if (badSeg || badWord) speechOk = false;
                }
            }
        }
        check("shards: deterministic, complete, 8-70 words, the texts distinct", okWorlds == 2 && determ && clean && lengths && distinct && musicN[0] >= 8 && musicN[0] <= 22 && musicN[1] >= 8 && musicN[1] <= 22,
              fmt("%d worlds of 2 with the trait; %s; %s; %d-%d words; %s; %d and %d of fifty are music", okWorlds, determ ? "the same twice" : "NOT the same twice", clean ? "no mark left" : "a mark left", minW, maxW, distinct ? "all distinct" : "repeats", musicN[0], musicN[1]));
        check("shards: the world's names recur", okWorlds == 2 && lorePct[0] >= 70 && lorePct[1] >= 70, fmt("felisian %d%%, desert %d%% of the text shards name something of the world", lorePct[0], lorePct[1]));
        check("shards: tone by world type", okWorlds == 2 && catPct[0] <= 20 && catPct[1] >= 30 && catPct[1] <= 65 && seaPct >= 25,
              fmt("warnings and the end: felisian %d%%, desert %d%%; the desert's sea, salt or river in %d%%", catPct[0], catPct[1], seaPct));
        check("decoding: a third of the words read with one shard, all with ten, never fewer with more, the names from the start, the people's words the same wherever they recur",   // C-06
              okWorlds == 2 && readPct[0][1] >= 25 && readPct[0][1] <= 50 && readPct[1][1] >= 25 && readPct[1][1] <= 50 && readPct[0][5] >= 55 && readPct[1][5] >= 55 && readPct[0][10] >= 99.9 && readPct[1][10] >= 99.9 && mono && namesKnown && tongueSame && tongueDistinct,
              fmt("read with 1/3/5/10 shards: felisian %.0f/%.0f/%.0f/%.0f%% of the words (%d in the language), desert %.0f/%.0f/%.0f/%.0f%% (%d); %s; %s; %s; %s", readPct[0][1], readPct[0][3], readPct[0][5], readPct[0][10], langWords[0], readPct[1][1], readPct[1][3], readPct[1][5], readPct[1][10], langWords[1],
                  mono ? "never fewer with more held" : "FEWER read with more held", namesKnown ? "the names from the start" : "a name NOT read at one", tongueSame ? "the tongue the same twice" : "the tongue NOT the same twice", tongueDistinct ? "its forty most used words distinct" : "two of its most used words the SAME"));
        if (okWorlds == 2 && trads[0].scale == trads[1].scale && trads[0].groups == trads[1].groups) tradDiffer = false;
        check("music: a tradition a world, its pieces in form",   // C-04
              okWorlds == 2 && tradSame && tradOk && piecesOk && pieceSame && formsVary && tradDiffer,
              fmt("%s; %s; %d and %d notes to the scale, cycles of %d and %d beats, %zu and %zu voices; pieces %s, %s, %d-%d s, %d notes at least, %d in all; forms %s; the two traditions %s",
                  tradSame ? "the tradition the same twice" : "the tradition NOT the same twice", tradOk ? "scale, cycle and voices in bounds" : "scale, cycle or voices OUT of bounds",
                  (int)trads[0].scale.size(), (int)trads[1].scale.size(), trads[0].beats, trads[1].beats, trads[0].voices.size(), trads[1].voices.size(),
                  pieceSame ? "the same twice" : "NOT the same twice", piecesOk ? "in order and in range" : fmt("OUT of order or range (%d out of order, %d out of time, %d velocity, %d degree, %d voice)", badOrder, badTime, badVel, badDeg, badVoice).c_str(), pieceMin, pieceMax, notesMin, noteTot, formsVary ? "vary" : "do NOT vary", tradDiffer ? "differ" : "are the SAME"));
        {   // C-05: the two worlds' voices differ
            bool voicesDiffer = okWorlds == 2 && (voices[0].pitch != voices[1].pitch || voices[0].rate != voices[1].rate);
            check("voice: a voice a world, every text spoken in time with its words",
                  okWorlds == 2 && voiceSame && voiceOk && inventoryOk && speechSame && speechOk && aligned && voicesDiffer,
                  fmt("%s; %s (felisian %.0f Hz %.1f syl/s, %zu sounds, '%s'; desert %.0f Hz %.1f syl/s, %zu sounds, '%s'); %d texts spoken %s, %.0f-%.0f s (%.0f s in all), %s, %s; the two voices %s",
                      voiceSame ? "the voice the same twice" : "the voice NOT the same twice", voiceOk && inventoryOk ? "in bounds" : "OUT of bounds", voices[0].pitch, voices[0].rate, voices[0].inventory.size(), voiceLine(voices[0]).c_str(),
                      voices[1].pitch, voices[1].rate, voices[1].inventory.size(), voiceLine(voices[1]).c_str(), speechN, speechSame ? "the same twice" : "NOT the same twice", speechMin, speechMax, speechTot,
                      speechOk ? "segments and words in order" : fmt("%d segments and %d words OUT of order", badSeg, badWord).c_str(), aligned ? "aligned at any share" : "NOT aligned across shares", voicesDiffer ? "differ" : "are the SAME"));
        }
        {   // C-05: a shard through the synth: finite, under the ceiling, loud enough, over when the speech is, silent once stopped
            Star s; StarSystem sys; bool rendered = false; int nan = 0, clip = 0; double rms = 0, tailRms = 0, stopRms = 0, secs = 0, peak = 0; bool done = false;
            if (starInSector(151, 0, 25, s, true)) {
                sys.generate(s); const Body& b = sys.bodies[1]; BodyGen g = BodyGen::make(b); Lore L = loreOf(sys, b, g);
                Tongue T = tongueOf(g, L); std::vector<Shard> sh; shardsOf(sys, b, g, L, 50, sh); Language lang; languageOf(L, sh, T, lang); Voice V = voiceOf(g, L, T);
                for (int i = 0; i < 50 && !rendered; i++) {
                    if (sh[i].music) continue;
                    std::vector<DecodedWord> w; decodeShard(sh[i], lang, T, 10, w);
                    Speech S; speechOf(V, w, sh[i].seed, S); secs = S.seconds;
                    AudioSynth synth; AudioState st; st.speech = &S; st.voice = &V; st.speechStart = true;
                    const int sr = 22050; std::vector<float> buf(2048); double sum = 0; long cnt = 0, tailCnt = 0; double tailSum = 0;
                    int frames = 0, total = (int)(sr * (S.seconds + 3));
                    while (frames < total) {
                        synth.render(buf.data(), (int)buf.size(), sr, st);
                        bool tail = frames > sr * (S.seconds + 1.5);
                        for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; peak = std::max(peak, (double)std::fabs(v)); sum += (double)v * v; cnt++; if (tail) { tailSum += (double)v * v; tailCnt++; } }
                        frames += (int)buf.size();
                    }
                    done = st.speechDone; rms = std::sqrt(sum / std::max(1L, cnt)); tailRms = std::sqrt(tailSum / std::max(1L, tailCnt));
                    st.speech = nullptr; st.voice = nullptr;
                    double ssum = 0; long scnt = 0;
                    for (int k = 0; k < 10; k++) { synth.render(buf.data(), (int)buf.size(), sr, st); for (float v : buf) { ssum += (double)v * v; scnt++; } }
                    stopRms = std::sqrt(ssum / std::max(1L, scnt));
                    rendered = true;
                }
            }
            check("voice: a shard through the synth", rendered && nan == 0 && clip < 50 && rms > 0.02 && rms < 0.35 && peak > 0.2 && done && tailRms < 0.005 && stopRms < 1e-6,
                  fmt("%.1f s rendered: %d non-finite, %d at the ceiling, peak %.2f, rms %.3f, %s, the tail %.4f, after the stop %.6f", secs, nan, clip, peak, rms, done ? "ended on time" : "NOT ended", tailRms, stopRms));
        }
        {   // C-04: a piece through the synth: finite, under the ceiling, loud enough, over when the piece is, silent once stopped
            Star s; StarSystem sys; bool rendered = false; int nan = 0, clip = 0; double rms = 0, tailRms = 0, stopRms = 0, secs = 0; bool done = false;
            if (starInSector(151, 0, 25, s, true)) {
                sys.generate(s); const Body& b = sys.bodies[1]; BodyGen g = BodyGen::make(b); Lore L = loreOf(sys, b, g);
                Tradition T = traditionOf(g, L); std::vector<Shard> sh; shardsOf(sys, b, g, L, 50, sh);
                for (int i = 0; i < 50 && !rendered; i++) {
                    if (!sh[i].music) continue;
                    Piece P; pieceOf(T, sh[i], P); secs = P.seconds;
                    AudioSynth synth; AudioState st; st.piece = &P; st.tradition = &T; st.pieceStart = true;
                    const int sr = 22050; std::vector<float> buf(2048); double sum = 0; long cnt = 0, tailCnt = 0; double tailSum = 0;
                    int frames = 0, total = (int)(sr * (P.seconds + 4));
                    while (frames < total) {
                        synth.render(buf.data(), (int)buf.size(), sr, st);
                        bool tail = frames > sr * (P.seconds + 2.5);
                        for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; sum += (double)v * v; cnt++; if (tail) { tailSum += (double)v * v; tailCnt++; } }
                        frames += (int)buf.size();
                    }
                    done = st.pieceDone; rms = std::sqrt(sum / std::max(1L, cnt)); tailRms = std::sqrt(tailSum / std::max(1L, tailCnt));
                    st.piece = nullptr; st.tradition = nullptr;
                    double ssum = 0; long scnt = 0;
                    for (int k = 0; k < 10; k++) { synth.render(buf.data(), (int)buf.size(), sr, st); for (float v : buf) { ssum += (double)v * v; scnt++; } }
                    stopRms = std::sqrt(ssum / std::max(1L, scnt));
                    rendered = true;
                }
            }
            check("music: a piece through the synth", rendered && nan == 0 && clip < 50 && rms > 0.02 && rms < 0.35 && done && tailRms < 0.01 && stopRms < 1e-6,
                  fmt("%.0f s rendered: %d non-finite, %d at the ceiling, rms %.3f, %s, the tail %.4f, after the stop %.6f", secs, nan, clip, rms, done ? "ended on time" : "NOT ended", tailRms, stopRms));
        }
        int worlds = 0, bad = 0;   // every people's world of the sectors near home: twelve shards each, clean and in length
        for (int64_t sx = 150; sx <= 160 && worlds < 60; sx++) for (int64_t sz = 20; sz <= 80 && worlds < 60; sz++) {
            Star s; if (!starInSector(sx, 0, sz, s, true)) continue;
            StarSystem sys; sys.generate(s);
            for (const Body& b : sys.bodies) {
                if (b.type != PT_FELISIAN && b.type != PT_DESERT) continue;
                BodyGen g = BodyGen::make(b);
                if (!g.hasTrait(TR_CIVILISATION)) continue;
                worlds++;
                Lore L = loreOf(sys, b, g);
                for (int i = 0; i < 12; i++) { Shard x = shardOf(sys, b, g, L, i); if (x.music) continue; int nw = shardWords(x.text); if (x.text.find_first_of("{}[]|") != std::string::npos || nw < 8 || nw > 70) bad++; }
            }
        }
        check("shards: every people's world near home reads clean", worlds >= 10 && bad == 0, fmt("%d worlds of sectors 150-160 x 20-80, %d shards of %d with a mark left or out of length", worlds, bad, worlds * 12));
    }
    printf("unit: %d failures\n", fails);
    return fails;
}

// O5 (the terrain plan): slope and relief statistics of the planet function per landable type: two bodies, three land
// sites each, a 48 x 48 grid at 16 m (what you walk on), a 32 x 32 grid at 64 m and one at 512 m (what you see)
// ---- O6-01: terrain metrics and the landform gallery -------------------------------------------------------------
// The acceptance harness of Terrain III (PLAN-terrain-3.md, section 2). A site is measured on four grids of the planet
// function: 24 x 24 cells at 4 m (the near ring), 48 x 48 at 16 m (770 m: what you walk on), 32 x 32 at 64 m (2 km) and
// 32 x 32 at 512 m (16 km: what you see from a ridge); a slope is the rise over run between neighbouring cells. Sites are
// grouped by the relief class the landing map prints (`Game::siteSlope`, `reliefClass`) and judged against that class's
// targets; the cost of a 16 m sample is timed serially and compared with the O5 figure of the type.
#include "core/parallel.h"

static const char* CLASS_NAMES[3] = {"FLAT", "HILLS", "MOUNTAINS"};
static int classIndex(const char* cls) { return cls[0] == 'F' ? 0 : (cls[0] == 'H' ? 1 : 2); }

struct SiteMetrics {
    int cls = 0;                                   // the landing map's class: 0 flat, 1 hills, 2 mountains
    double probe = 0;                              // its slope probe
    double med4 = 0;                               // median slope of the 4 m grid
    std::vector<double> slopes16, slopes512;       // every cell's slope (pooled per class by the table)
    double relief770 = 0, relief2k = 0, relief16k = 0;
    double floors = 0;                             // share of the lowest quarter of the 64 m cells under a slope of 0.05
    double usPerSample = 0;                        // the 16 m grid's cost, when timed
    double hiYaw = 0;                              // toward the highest ground within a kilometre (the gallery looks there)
    double lat = 0, lon = 0;
    int material = MAT_ROCK;
};

static void gridHeights(const SurfaceSite& site, int N, double cell, std::vector<double>& h) {
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

static SiteMetrics measureSite(const SurfaceSite& site, bool timeIt) {
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

// the first `maxBodies` bodies of a type in the start region, in the scan order every harness mode uses; `fn(sys, bi)`
template <class F>
static int forTypeBodies(int type, int maxBodies, bool preferMoon, F fn) {
    int found = 0;
    for (int pass = preferMoon ? 0 : 1; pass < 2 && found < maxBodies; pass++)
        for (int64_t x = 150; x < 340 && found < maxBodies; x++)
            for (int64_t z = 20; z < 160 && found < maxBodies; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && found < maxBodies; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != type) continue;
                    if (pass == 0 && b.parent < 0) continue;   // the first pass looks for moons only
                    found++;
                    fn(sys, bi);
                }
            }
    return found;
}

static bool firstBodyOfType(int type, StarSystem& sysOut, int& biOut) {
    bool found = false;
    forTypeBodies(type, 1, false, [&](const StarSystem& sys, int bi) { sysOut = sys; biOut = bi; found = true; });
    return found;
}

// B-307: the buggy rolls down a mountain slope. The steepest straight descent of 200 m within 500 m of the capsule of
// the pinned felisian mountain site (grade 0.12 .. 0.45, no water), driven at full throttle: it must stay on its wheels
// (under a tenth of the frames airborne, at most two landings) and get to the bottom fast. It used to launch every time
// the ground dropped faster than 3 m/s, land with a thump and 15% of its speed gone, and launch again: a ladder.
static int runDescent(double latDeg = PIN_MOUNTAIN_LAT, double lonDeg = PIN_MOUNTAIN_LON) {   // G-04: `descent [latDeg lonDeg]` tries another site of the world
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
                // G-04: every 2 m (10 m steps let a ledge through: the buggy launched from it), from the start itself (a rock
                // beside the start held it for ten seconds); no step up over 0.3 m, none down over 1.6 m (a cliff), a steady
                // grade (the profile bends under 0.5 m per 2 m step: the GEN 10 slopes are broken ground, and a bump of a metre
                // at 100 km/h is a jump, not the ladder B-307 tested), no water
                for (double d = 0; d <= LEN + 30 && ok; d += 2) {
                    double x = sx + hx * d, z = sz + hz * d, hh = sv.site.groundHeight(x, z);
                    if (hh > hp + 0.3 || hh < hp - 1.6 || hh < sv.site.waterAt(x, z) + 0.5) ok = false;
                    if (d >= 4 && std::fabs(hpp - 2 * hp + hh) > 0.5) ok = false;
                    hpp = hp; hp = hh;
                    // G-04: no rock over 0.9 m, trunk, log or ruin within 3 m of the line, as `findOpenRun` asks (an alpine
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
    std::string trace;   // G-04: where it got every two seconds (along, across, speed, heading error), to see a stall
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
static void landSitesOf(const StarSystem& sys, int bi, uint64_t salt, int n, std::vector<std::pair<double, double>>& out) {
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

static void materialAgreement(const StarSystem& sys, int bi, double lat, double lon, double halfM, double stepM, double out[2]) {
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
        range("cliffs", cliff * 100, 3, 8, nullptr, "%.1f%%");   // O6-04
        range("walkable", walk * 100, 90, 100, nullptr, "%.0f%%");
        range("floors", a.floors * 100, 15, 100, nullptr, "%.0f%%");   // O6-03: the valley floors
    } else if (cls == 1) {
        range("median", med16, 0.08, 0.15, nullptr, "%.2f");
        range("2km", a.r2k, 120, craterFields ? 450 : 250, nullptr, "%.0fm");   // a crater in the window adds its depth
        range("floors", a.floors * 100, 25, 100, craterFields ? "O6-05" : nullptr, "%.0f%%");   // crater floors and terraces come with the second landform library
    } else {
        range("median", med16, 0.01, craterFields ? 0.09 : 0.04, nullptr, "%.3f");   // a plain of a cratered type is a crater field
        range("broken p90", p90, 0.06, 10, nullptr, "%.3f");   // O6-04
        range("broken 770m", a.r770, 6, 1e9, nullptr, "%.0fm");
    }
    return fails;
}

static int g_terrainType = -1;   // `terrain <type id>`: one type only (the tuning loop)
static int runTerrainStats() {
    // O5 (before O6-02): microseconds per 16 m sample on the development Mac, measured by this harness; the budget is 2x
    static const double O5_COST[PT_COUNT] = {0.8, 1.1, 0.5, 0.8, 0.9, 1.3, 0, 0.9, 0.4, 0.2, 0.8, 0.4, 0.5, 0, 1.0, 0};
    printf("%-16s %-9s %3s | %5s | %-40s | %5s %6s %6s | %6s | %-13s | %s\n", "type", "class", "n", "4m", "16 m: med  p90   max  cliff  walk", "770m", "2km", "16km", "floors", "512m: med p90", "cost us (O5, x)");
    int fails = 0, later = 0, rows = 0, rowsOk = 0;
    for (int type = 0; type < PT_COUNT; type++) {
        if (!PLANET_TYPES[type].landable || (g_terrainType >= 0 && type != g_terrainType)) continue;
        std::vector<StarSystem> systems; std::vector<int> bodyOf; std::vector<std::pair<double, double>> sites; std::vector<int> sysOf;
        forTypeBodies(type, 4, false, [&](const StarSystem& sys, int bi) {
            systems.push_back(sys); bodyOf.push_back(bi);
            size_t n0 = sites.size();
            landSitesOf(sys, bi, 0x06, 12, sites);
            for (size_t k = n0; k < sites.size(); k++) sysOf.push_back((int)systems.size() - 1);
        });
        if (sites.empty()) { printf("%-16s no body found\n", PLANET_TYPES[type].name); continue; }
        std::vector<SiteMetrics> ms(sites.size());
        parallelFor((int)sites.size(), 1, [&](int b, int e) {
            for (int i = b; i < e; i++) {
                SurfaceSite st; st.init(&systems[sysOf[i]], bodyOf[sysOf[i]], sites[i].first, sites[i].second, 1000.0); ms[i] = measureSite(st, false);
                if (std::getenv("VESPERIS_TRACE") && ms[i].cls == 0 && typeHasDrainage(type) && i < 6) {   // O6-07: what flattens a plain (the flat class's median grade)
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
                if (std::getenv("VESPERIS_TRACE") && ms[i].cls == 2 && typeHasDrainage(type)) {   // O6-07: what the drainage says over a mountain window (the floors' tuning)
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
            printf("%-16s %-9s %3d | %.3f | %.3f %.3f %5.2f %4.1f%% %4.0f%% | %4.0fm %5.0fm %5.0fm | %4.0f%% | %.4f %.4f | %s\n", PLANET_TYPES[type].name, CLASS_NAMES[cls], a.n, a.med4,
                   percentile(a.s16, 0.5), percentile(a.s16, 0.9), a.s16.empty() ? 0.0 : *std::max_element(a.s16.begin(), a.s16.end()), shareOver(a.s16, 1.0) * 100, (1 - shareOver(a.s16, 1.35)) * 100,
                   a.r770, a.r2k, a.r16k, a.floors * 100, percentile(a.s512, 0.5), percentile(a.s512, 0.9), judged ? notes.c_str() : (a.n < 2 ? "(one site: not judged)" : "(not judged)"));
        }
        printf("%-16s classes of %zu land sites: flat %d, hills %d, mountains %d; cost %.1f us per 16 m sample (O5 %.1f, x%.2f%s)\n", "", ms.size(), agg[0].n, agg[1].n, agg[2].n, cost, O5_COST[type],
               O5_COST[type] > 0 ? cost / O5_COST[type] : 0.0, costOk ? "" : ", OVER 2x");
    }
    printf("terrain: %d of %d class rows meet every target of the spectrum; %d spectrum targets failing, %d targets of later items (cliffs and broken plains: O6-04; mountain floors: O6-03) not yet met\n", rowsOk, rows, fails, later);
    return fails;
}

// ---- the landform gallery ------------------------------------------------------------------------------------------
struct Sheet {
    int w, h; std::vector<uint32_t> px; RGBCanvas cv;
    Sheet(int W, int H) : w(W), h(H), px((size_t)W * H, rgb(12, 12, 14)), cv{px.data(), W, H, 1} {}
    void blit(const std::vector<uint32_t>& src, int sw, int sh, int ox, int oy) {
        for (int y = 0; y < sh; y++) for (int x = 0; x < sw; x++) { int X = ox + x, Y = oy + y; if (X >= 0 && Y >= 0 && X < w && Y < h) px[(size_t)Y * w + X] = src[(size_t)y * sw + x]; }
    }
    void label(int x, int y, const std::string& s, uint32_t c = rgb(255, 220, 120)) { fillRectRGB(cv, x, y, x + textWidth(s.c_str()) + 3, y + 8, rgb(0, 0, 0)); drawText(cv, x + 2, y + 1, s.c_str(), c); }
    void save(const std::string& fn) { writePNG(fn.c_str(), px.data(), w, h); printf("wrote %s\n", fn.c_str()); }
};

// A shaded relief of an N x N grid of `cell` metres round the site, 2 px per cell: the light from the north-west at 45
// degrees, water blue, snow and ice pale; what a DEM hillshade shows a geologist.
static void hillshade(const SurfaceSite& site, int N, double cell, double zFactor, std::vector<uint32_t>& px) {
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
static double renderSiteFrame(const StarSystem& sys, int bi, double lat, double lon, double yaw, double alt, double pitch, SpaceRenderer& sr, StarNeighborhood& nb, std::vector<uint32_t>& px) {
    SurfaceView sv;
    sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, 35 * DEG, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
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

// B-313: the scene `felisian_mountains`: the pinned felisian mountain site of the O6 review (LANDFORM_SITES[0]), standing,
// facing the highest ground within a kilometre, for the walking stability test and the frames
static bool setupPinnedMountainScene(double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut) {
    std::vector<StarSystem> systems; std::vector<int> bodyOf;
    forTypeBodies(PT_FELISIAN, 1, false, [&](const StarSystem& sy, int bi) { systems.push_back(sy); bodyOf.push_back(bi); });
    if (systems.empty()) return false;
    sys = systems[0]; int bi = bodyOf[0];
    double lat = PIN_MOUNTAIN_LAT * DEG, lon = PIN_MOUNTAIN_LON * DEG;
    sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, alt, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
    SiteMetrics m = measureSite(sv.site, false);
    sv.player.yaw = m.hiYaw + yawOff; sv.player.pitch = pitch;
    sv.player.x = 30; sv.player.z = -20; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);   // away from the capsule at the origin
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    Input in; sv.update(0.016, in, t, false);
    tOut = t;
    return true;
}

// R-304/R-305: `traits [name]`: for every landform trait the first body that carries it, the spot on it where the landform
// is strongest (a scan of the sphere at 200 m detail by `SurfaceSample::landform`), a hillshade of 8 km round the spot, a
// standing frame from 250 m west of it and an aerial frame 350 m up; one sheet per trait in `shots/tests/trait_*.png`.
// Then the share of the first 300 bodies carrying each trait
// C-01: `ruins <sx> <sz> <body index> [latDeg lonDeg]`: a world's culture, the old seas, and the settlements of the 31 x 31 ruin cells
// round a point with the site test's verdict and its reasons (the planet function read without the drainage, as the view reads it)
// C-02: `shards <sx> <sz> <body index> [n] [full | <held> [full]]`: a world's lore, then (C-06) its people's tongue and the words of
// its language the computer learns first, then its first n shards (default fifty) as the decoder reads them with `held` shards
// of the world in hand (default one: about a third of the words, the rest in the people's tongue; ten: the whole text), in the
// order of their years with the tone and the kind of each; `full` prints the whole text under each
static int runShards(int argc, char** argv) {
    if (argc < 5) { printf("shards <sx> <sz> <body index> [n] [full | <held> [full]]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    int n = argc > 5 ? atoi(argv[5]) : 50; bool full = false; int held = 1;
    if (argc > 6) { if (std::string(argv[6]) == "full") full = true; else held = atoi(argv[6]); }
    if (argc > 7 && std::string(argv[7]) == "full") full = true;
    Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    StarSystem sys; sys.generate(s);
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) printf("(%s had no people: the shards below are what it would have said)\n", b.name.c_str());
    Lore L = loreOf(sys, b, g);
    printf("%s, %s of %s (%s): the %s; god %s, river %s, mountain %s, sea %s, towns %s and %s, founder %s, moon %s, star %s, festival %s; names of style %d; a day of %.0f min of play, a year of %.0f of its days, %d moons, %d years from the founding\n",
           b.name.c_str(), PLANET_TYPES[b.type].name, s.name.c_str(), STAR_CLASSES[s.cls].name, L.people.c_str(), L.god.c_str(), L.river.c_str(), L.mountain.c_str(), L.sea.c_str(), L.city.c_str(), L.city2.c_str(),
           L.founder.c_str(), L.moon.c_str(), L.star.c_str(), L.festival.c_str(), L.style, L.dayHours * 60, L.yearDays, L.moons, L.spanYears);
    // C-06: the tongue and the language
    Tongue T = tongueOf(g, L);
    std::vector<Shard> fifty; shardsOf(sys, b, g, L, SHARDS_PER_WORLD, fifty);
    Language lang; languageOf(L, fifty, T, lang);
    auto join = [](const std::vector<std::string>& v) { std::string r; for (const std::string& x : v) { if (!r.empty()) r += " "; r += x; } return r; };
    printf("tongue: onsets %s; nuclei %s; codas %s; a coda %.0f%% of the time, a bare first vowel %.0f%%\n", join(T.onsets).c_str(), join(T.nuclei).c_str(), join(T.codas).c_str(), 100 * T.codaChance, 100 * T.vowelStart);
    { Voice V = voiceOf(g, L, T); printf("voice: %s; %.0f Hz, %.1f syllables a second (C-05: `voice %lld %lld %d`)\n", voiceLine(V).c_str(), V.pitch, V.rate, (long long)sx, (long long)sz, bi); }
    printf("language: %d words used %d times in the fifty, %zu names; known with 1/2/3/5/7/10 shards: %d/%d/%d/%d/%d/%d words (%.0f/%.0f/%.0f/%.0f/%.0f/%.0f%% of the uses)\n", (int)lang.words.size(), lang.tokens, lang.names.size(),
           languageKnown(lang, 1), languageKnown(lang, 2), languageKnown(lang, 3), languageKnown(lang, 5), languageKnown(lang, 7), languageKnown(lang, 10),
           100 * languageShare(1), 100 * languageShare(2), 100 * languageShare(3), 100 * languageShare(5), 100 * languageShare(7), 100 * languageShare(10));
    printf("  the most used:");
    for (int k = 0; k < (int)lang.words.size() && k < 16; k++) printf(" %s=%s(%d)", lang.words[k].c_str(), lang.theirs[k].c_str(), lang.counts[k]);
    printf("\n");
    std::vector<Shard> sh; if (n <= SHARDS_PER_WORLD) sh.assign(fifty.begin(), fifty.begin() + n); else shardsOf(sys, b, g, L, n, sh);
    std::vector<int> order(n); for (int i = 0; i < n; i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int c) { return sh[a].year < sh[c].year; });
    int tones[ST_TONE_COUNT] = {0, 0, 0, 0}; int words = 0, read = 0, pieces = 0;
    Tradition trad = traditionOf(g, L);   // C-04
    for (int i : order) {
        const Shard& x = sh[i];
        tones[x.tone]++;
        if (x.music) { Piece P; pieceOf(trad, x, P); pieces++; printf("%3d  year %3d  %-8s  %-14s [music] %s, %.0f s, %zu notes\n", i, x.year, SHARD_TONE_NAMES[x.tone], "music", PIECE_FORM_NAMES[P.form], P.seconds, P.notes.size()); continue; }
        words += shardWords(x.text);
        std::vector<DecodedWord> w; read += decodeShard(x, lang, T, held, w);
        printf("%3d  year %3d  %-8s  %-14s %s\n", i, x.year, SHARD_TONE_NAMES[x.tone], shardKindName(x.kind), decodedText(w).c_str());
        if (full) printf("%40s %s\n", "", x.text.c_str());
    }
    printf("tones: ordinary %d, elegy %d, warning %d, the end %d; %d pieces of music; %.0f words a text; %d of %d words read with %d shard%s held\n", tones[0], tones[1], tones[2], tones[3], pieces, n - pieces > 0 ? (double)words / (n - pieces) : 0.0, read, words, held, held == 1 ? "" : "s");
    return 0;
}

// C-04: `music <sx> <sz> <body index> [index] [wav]`: a world's musical tradition (its scale in cents, its cycle, its tempo, its
// timbres, its ornament and texture), the pieces among its fifty shards (index, year, tone, form, length, notes), then one piece
// (the given index, else the first piece) as notation: a line per voice per cycle, a degree at each subdivision ('.' a held note,
// '-' silence, 'o'/'x' the drum's skins); `wav` renders it through the synth to shots/tests/music_<sx>_<sz>_<body>_S<index>.wav
static int runMusic(int argc, char** argv) {
    if (argc < 5) { printf("music <sx> <sz> <body index> [index] [wav]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    int want = argc > 5 && std::string(argv[5]) != "wav" ? atoi(argv[5]) : -1;
    bool wav = (argc > 5 && std::string(argv[5]) == "wav") || (argc > 6 && std::string(argv[6]) == "wav");
    Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    StarSystem sys; sys.generate(s);
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) printf("(%s had no people: the music below is what it would have played)\n", b.name.c_str());
    Lore L = loreOf(sys, b, g);
    Tradition T = traditionOf(g, L);
    printf("%s, %s of %s: the music of the %s (names of style %d)\n", b.name.c_str(), PLANET_TYPES[b.type].name, s.name.c_str(), L.people.c_str(), L.style);
    printf("  %s (%s), the period %.3f, the tonic %.1f Hz; degrees in cents:", traditionLine(T).c_str(), T.tuning == 0 ? fmt("%d equal steps", T.division).c_str() : "just ratios", T.period, T.base);
    for (int d = 0; d < (int)T.scale.size(); d++) printf(" %.0f", traditionCents(T, d));
    printf("; resting on degree %d (%.0f cents)\n", T.rest, traditionCents(T, T.rest));
    printf("  %s, each beat in %d, drifting %.0f%%\n", cycleLine(T, T.bpm).c_str(), T.sub, 100 * T.drift);
    static const char* SECOND[4] = {"no second voice", "a second voice in parallel below", "a second voice answering a phrase later", "a second voice doubling a period below"};
    static const char* ORN[5] = {"no ornament", "grace notes before a leap", "trills on long notes", "slides between neighbours", "mordents"};
    printf("  voices:");
    for (const Timbre& t : T.voices) printf(" %s (attack %.0f ms, decay %.2f s, sustain %.2f, vibrato %.1f Hz x %.1f%%, breath %.2f);", TIMBRE_FAMILY_NAMES[t.family], 1000 * t.attack, t.decay, t.sustain, t.vibRate, 100 * t.vibDepth, t.breath);
    printf(" a drum at %.0f and %.0f Hz%s; %s; %s; %s %.0f%% of the time\n", T.drum.ratio[0], T.drum.ratio[1], T.hasDrum ? "" : " (the pieces that ask for one)", T.drone ? "a drone" : "no drone", SECOND[T.second], ORN[T.ornament], 100 * T.ornamentChance);
    std::vector<Shard> fifty; shardsOf(sys, b, g, L, SHARDS_PER_WORLD, fifty);
    std::vector<int> order(SHARDS_PER_WORLD); for (int i = 0; i < SHARDS_PER_WORLD; i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int c) { return fifty[a].year < fifty[c].year; });
    int pieces = 0, first = -1; int forms[PF_COUNT] = {0}; double secs = 0;
    for (int i : order) {
        if (!fifty[i].music) continue;
        Piece P; pieceOf(T, fifty[i], P); pieces++; forms[P.form]++; secs += P.seconds; if (first < 0) first = i;
        printf("  %3d  year %3d  %-8s  %-22s %3.0f s  %3d bpm  %2d cycles  %3zu notes%s\n", i, fifty[i].year, SHARD_TONE_NAMES[fifty[i].tone], PIECE_FORM_NAMES[P.form], P.seconds, (int)std::lround(P.bpm), P.cycles, P.notes.size(), pieceHasDrum(P) ? "  drum" : "");
    }
    printf("%d pieces of fifty, %.0f s in all:", pieces, secs);
    for (int f = 0; f < PF_COUNT; f++) if (forms[f]) printf(" %s %d;", PIECE_FORM_NAMES[f], forms[f]);
    printf("\n");
    int idx = want >= 0 ? want : first;
    if (idx < 0 || idx >= SHARDS_PER_WORLD || !fifty[idx].music) { printf("shard %d is not a piece of music\n", idx); return pieces ? 0 : 1; }
    Piece P; pieceOf(T, fifty[idx], P);
    printf("shard %d, year %d: %s, %.0f s at %.0f bpm, %d cycles, %zu notes\n", idx, fifty[idx].year, PIECE_FORM_NAMES[P.form], P.seconds, P.bpm, P.cycles, P.notes.size());
    int nv = (int)T.voices.size(), cu = T.beats * T.sub;
    for (int c = 0; c < P.cycles; c++) {
        for (int v = -1; v < nv; v++) {
            std::string line(cu, '-'); bool any = false;
            for (const Note& nt : P.notes) {
                if (nt.voice != v) continue;
                int u0 = (int)std::floor(nt.t * T.sub + 1e-6) - c * cu, u1 = (int)std::ceil((nt.t + nt.dur) * T.sub - 1e-6) - c * cu;
                if (u1 <= 0 || u0 >= cu) continue;
                for (int u = std::max(u0, 0); u < std::min(u1, cu); u++) line[u] = u == u0 ? (v < 0 ? (nt.degree == 0 ? 'o' : 'x') : (char)(nt.degree >= 0 && nt.degree < 10 ? '0' + nt.degree : (nt.degree >= 10 ? 'A' + std::min(nt.degree - 10, 25) : 'a' + std::min(-nt.degree - 1, 25)))) : '.';
                any = true;
            }
            if (any || v == 0) printf("  cycle %2d %s: %s\n", c + 1, v < 0 ? "drum" : fmt("v%d  ", v).c_str(), line.c_str());
        }
    }
    printf("  (degrees: 0-9 the tonic up, A-Z past nine, a-z below the tonic; the lead v0, the others as the tradition has them)\n");
    if (wav) {
        AudioSynth synth; AudioState st; st.piece = &P; st.tradition = &T; st.pieceStart = true;
        const int sr = 22050; std::vector<float> buf(2048); std::vector<int16_t> pcm; int nan = 0, clip = 0; double sum = 0; float peak = 0;
        int frames = 0, total = (int)(sr * (P.seconds + 3));
        while (frames < total) {
            synth.render(buf.data(), (int)buf.size(), sr, st);
            for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; peak = std::max(peak, std::fabs(v)); sum += (double)v * v; pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32000)); }
            frames += (int)buf.size();
        }
        makeDir("shots"); makeDir("shots/tests");
        std::string fn = fmt("shots/tests/music_%lld_%lld_%d_S%d.wav", (long long)sx, (long long)sz, bi, idx);
        FILE* f = fopen(fn.c_str(), "wb");
        if (f) {
            uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
            fwrite("RIFF", 1, 4, f); fwrite(&riffLen, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtLen, 4, 1, f); fwrite(&fmtTag, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f); fwrite(&blockAlign, 2, 1, f); fwrite(&bits, 2, 1, f);
            fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
        }
        printf("rendered %.1f s: peak %.3f, rms %.3f, %d non-finite, %d at the ceiling, %s -> %s\n", (double)pcm.size() / sr, peak, std::sqrt(sum / std::max<size_t>(1, pcm.size())), nan, clip, st.pieceDone ? "the piece ended" : "the piece NOT ended", fn.c_str());
    }
    return 0;
}

// C-05: `voice <sx> <sz> <body index> [index] [wav]`: a world's voice (its pitch and range, its rate, its tract, its qualities,
// its stress and melody, its marks) and the sounds its tongue uses, then one text shard (the given index, else the first text)
// as it is spoken: each word as the decoder gives it with the language whole, the people's word, its sounds and its span in
// seconds; `wav` renders it through the synth to shots/tests/voice_<sx>_<sz>_<body>_S<index>.wav
// C-07: `signals [sx sy sz] [reach]`: the far signals heard from a sector (home by default): each with its kind, its distance (the
// age), its direction (galactic longitude and latitude from the observer), its star's class and sector, the world and the
// recording on the air at the game's first hour; then the system's own signals there, if the sector holds a star
static int runSignals(int argc, char** argv) {
    int64_t sx = HOME_SX, sy = HOME_SY, sz = HOME_SZ; double reach = SIGNAL_REACH_LY;
    if (argc >= 5) { sx = atoll(argv[2]); sy = atoll(argv[3]); sz = atoll(argv[4]); }
    if (argc >= 6 && std::string(argv[5]) != "wav") reach = atof(argv[5]);
    Vec3 obs((sx + 0.5) * SECTOR_KM, (sy + 0.5) * SECTOR_KM, (sz + 0.5) * SECTOR_KM);
    std::vector<Signal> far; double t0 = nowSec(); int cells = signalsNear(obs, far, reach); double ms = (nowSec() - t0) * 1e3;
    printf("signals within %.0f ly of sector %lld %lld %lld: %zu (%d cells of %d sectors scanned in %.1f ms)\n", reach, (long long)sx, (long long)sy, (long long)sz, far.size(), cells, SIGNAL_CELL, ms);
    for (const Signal& s : far) {
        Vec3 d = normalize(s.pos - obs);
        double lon = std::atan2(d.z, d.x) / DEG, lat = std::asin(clampd(d.y, -1, 1)) / DEG;
        std::string what;
        if (s.kind == SIG_PEOPLE) {
            Star named = s.star; starInSector(s.star.sx, s.star.sy, s.star.sz, named, true);   // the scan leaves the names out; the print wants them
            StarSystem sys; sys.generate(named);
            const Body& b = sys.bodies[s.body];
            BodyGen g = BodyGen::make(b); Lore L = loreOf(sys, b, g);
            int idx = transmittedShard(s.seed, 3.6e6);
            Shard sh = shardOf(sys, b, g, L, idx);
            what = fmt("%s (body %d, the %s): on the air S%d, %s", b.name.c_str(), s.body, L.people.c_str(), idx, sh.music ? "a piece of music" : fmt("a voice: \"%s\"", trunc(sh.text, 60).c_str()).c_str());
        } else what = fmt("%.2f pulses a second", s.pulseHz);
        printf("  %-24s %6.1f ly  lon %6.1f lat %5.1f  strength %.2f  %s %s at %lld %lld %lld: %s\n", SIGNAL_KIND_NAMES[s.kind], s.distLy, lon, lat, s.strength, STAR_CLASSES[s.star.cls].code, STAR_CLASSES[s.star.cls].name,
               (long long)s.star.sx, (long long)s.star.sy, (long long)s.star.sz, what.c_str());
    }
    if (argc > 2 && std::string(argv[argc - 1]) == "wav") {   // the receiver's sounds for the ear: the bed with its events, three of each natural kind, every programme of the first people's world
        makeDir("shots"); makeDir("shots/tests");
        static const char* NAMES[SIG_KIND_COUNT] = {"people", "pulsar", "comet", "magnetosphere"};
        static const char* PNAMES[RP_COUNT] = {"voice", "whispers", "chant", "numbers", "loop", "beacon", "data", "bell", "siren", "murmur", "music"};
        static const double RATES[3] = {0.7, 1.7, 3.2};
        { std::vector<int16_t> pcm; RenderStats r = renderKind(-1, 0, 1, 30.0, &pcm); std::string fn = "shots/tests/radar_bed.wav"; writeWav16(fn, pcm, 22050); printf("  bed: 30 s rendered (something in the static every twelve to forty seconds), peak %.3f, rms %.3f -> %s\n", r.peak, r.rms, fn.c_str()); }
        for (int kind = SIG_PULSAR; kind < SIG_KIND_COUNT; kind++)
            for (int k = 0; k < 3; k++) {
                std::vector<int16_t> pcm; RenderStats r = renderKind(kind, 0x51 + (uint64_t)k * 0x1F3, RATES[k], 12.0, &pcm);
                std::string fn = fmt("shots/tests/radar_%s_%d.wav", NAMES[kind], k + 1); writeWav16(fn, pcm, 22050);
                printf("  %s %d: 12 s rendered, peak %.3f, rms %.3f -> %s\n", NAMES[kind], k + 1, r.peak, r.rms, fn.c_str());
            }
        PeopleWorld w;
        if (firstPeopleWorld(w)) {
            printf("  the programmes of %s (%s):\n", w.name.c_str(), w.lore.people.c_str());
            for (int p = 0; p < RP_COUNT; p++) {
                std::vector<int16_t> pcm; Programme prog; RenderStats r = renderProgramme(w, p, 12.0, &pcm, &prog);
                std::string fn = fmt("shots/tests/radar_people_%s.wav", PNAMES[p]); writeWav16(fn, pcm, 22050);
                std::string what = prog.machine ? fmt("%.0f s on the air", prog.seconds) : (prog.kind == RP_MUSIC ? fmt("the piece at %.2f of its speed", prog.pieceSpeed) : fmt("\"%s\"%s", trunc(decodedText(prog.words), 40).c_str(), prog.repeats > 1 ? fmt(", %d times with %.1f s between", prog.repeats, prog.gap).c_str() : ""));
                printf("    %-18s 12 s, peak %.3f, rms %.3f: %s -> %s\n", RADIO_PROGRAMME_NAMES[p], r.peak, r.rms, what.c_str(), fn.c_str());
            }
            int counts[RP_COUNT] = {0};
            for (int i = 0; i < SHARDS_PER_WORLD; i++) counts[programmeOf(w.seed, i, w.shards[i].music)]++;
            printf("  its broadcast over the fifty slots:"); for (int p = 0; p < RP_COUNT; p++) if (counts[p]) printf(" %s %d;", RADIO_PROGRAMME_NAMES[p], counts[p]); printf("\n");
        } else printf("  no people's world within reach of home: no programmes rendered\n");
    }
    Star here; if (starInSector(sx, sy, sz, here, true)) {
        StarSystem sys; sys.generate(here);
        std::vector<Signal> local; localSignals(sys, obs, 3.6e6, local);
        printf("the system of %s (%s) here: %zu local signals%s\n", here.name.c_str(), STAR_CLASSES[here.cls].name, local.size(), sectorTransmits(sx, sy, sz) ? " (a transmitter sector)" : "");
        for (const Signal& s : local) printf("  %-24s %s (body %d, %s)  strength %.2f  %s\n", SIGNAL_KIND_NAMES[s.kind], sys.bodies[s.body].name.c_str(), s.body, PLANET_TYPES[s.bodyType].name, s.strength, signalSourceLine(s).c_str());
    } else printf("no star in that sector\n");
    return 0;
}

static int runVoice(int argc, char** argv) {
    if (argc < 5) { printf("voice <sx> <sz> <body index> [index] [wav]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    int want = argc > 5 && std::string(argv[5]) != "wav" ? atoi(argv[5]) : -1;
    bool wav = (argc > 5 && std::string(argv[5]) == "wav") || (argc > 6 && std::string(argv[6]) == "wav");
    Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    StarSystem sys; sys.generate(s);
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) printf("(%s had no people: the voice below is what it would have had)\n", b.name.c_str());
    Lore L = loreOf(sys, b, g);
    Tongue T = tongueOf(g, L);
    Voice V = voiceOf(g, L, T);
    static const char* STRESS[4] = {"the first syllable", "the last syllable", "the last syllable but one", "no syllable (all even)"};
    static const char* CONTOUR[3] = {"falling through the sentence", "rising to a peak then falling", "level with a drop at the end"};
    printf("%s, %s of %s: the voice of the %s (names of style %d)\n", b.name.c_str(), PLANET_TYPES[b.type].name, s.name.c_str(), L.people.c_str(), L.style);
    printf("  %s: %.0f Hz with a range of %.2f, %.1f syllables a second, the tract %.2f; breath %.2f, roughness %.2f, nasal %.2f, tremor %.2f, a second tone %.2f at %.2f; the stress on %s, the melody %s%s%s%s\n",
           voiceLine(V).c_str(), V.pitch, V.range, V.rate, V.tract, V.breath, V.rough, V.nasal, V.tremor, V.sub, V.subRatio, STRESS[V.stress], CONTOUR[V.contour],
           V.tonal ? ", a tone on each syllable" : "", V.clicky ? ", clicks for the voiceless stops" : "", V.trill ? ", the r's rolled" : "");
    printf("  sounds (%zu):", V.inventory.size());
    static const char* KINDS[PK_COUNT] = {"vowel", "stop", "fricative", "nasal", "liquid", "glide", "pause"};
    for (int p : V.inventory) printf(" %s(%s%s)", PHONES[p].sym, PHONES[p].kind == PK_VOWEL ? "" : (PHONES[p].voiced ? "voiced " : "voiceless "), KINDS[PHONES[p].kind]);
    printf("\n");
    std::vector<Shard> fifty; shardsOf(sys, b, g, L, SHARDS_PER_WORLD, fifty);
    Language lang; languageOf(L, fifty, T, lang);
    int idx = want;
    if (idx < 0) for (int i = 0; i < SHARDS_PER_WORLD && idx < 0; i++) if (!fifty[i].music) idx = i;
    if (idx < 0 || idx >= SHARDS_PER_WORLD || fifty[idx].music) { printf("shard %d is not a text\n", idx); return 1; }
    std::vector<DecodedWord> w; decodeShard(fifty[idx], lang, T, 10, w);
    Speech S; speechOf(V, w, fifty[idx].seed, S);
    printf("shard %d, year %d, %s: %zu words, %d syllables, %.1f s, %zu segments\n", idx, fifty[idx].year, SHARD_TONE_NAMES[fifty[idx].tone], w.size(), S.syllables, S.seconds, S.segs.size());
    printf("  %s\n", fifty[idx].text.c_str());
    for (size_t k = 0; k < w.size(); k++) {
        std::string spoken = w[k].name ? w[k].ours : w[k].theirs;
        printf("  %5.2f-%5.2f  %-14s %-12s %s%s\n", S.wordStart[k], S.wordEnd[k], (w[k].ours + w[k].post).c_str(), spoken.c_str(), S.phones[k].c_str(), w[k].name ? "  (a name)" : "");
    }
    if (wav) {
        AudioSynth synth; AudioState st; st.speech = &S; st.voice = &V; st.speechStart = true;
        const int sr = 22050; std::vector<float> buf(2048); std::vector<int16_t> pcm; int nan = 0, clip = 0; double sum = 0; float peak = 0;
        int frames = 0, total = (int)(sr * (S.seconds + 1));
        while (frames < total) {
            synth.render(buf.data(), (int)buf.size(), sr, st);
            for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; peak = std::max(peak, std::fabs(v)); sum += (double)v * v; pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32000)); }
            frames += (int)buf.size();
        }
        makeDir("shots"); makeDir("shots/tests");
        std::string fn = fmt("shots/tests/voice_%lld_%lld_%d_S%d.wav", (long long)sx, (long long)sz, bi, idx);
        FILE* f = fopen(fn.c_str(), "wb");
        if (f) {
            uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
            fwrite("RIFF", 1, 4, f); fwrite(&riffLen, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtLen, 4, 1, f); fwrite(&fmtTag, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f); fwrite(&blockAlign, 2, 1, f); fwrite(&bits, 2, 1, f);
            fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
        }
        printf("rendered %.1f s: peak %.3f, rms %.3f, %d non-finite, %d at the ceiling, %s -> %s\n", (double)pcm.size() / sr, peak, std::sqrt(sum / std::max<size_t>(1, pcm.size())), nan, clip, st.speechDone ? "the speech ended" : "the speech NOT ended", fn.c_str());
    }
    return 0;
}

static int runRuins(int argc, char** argv) {
    if (argc < 5) { printf("ruins <sx> <sz> <body index> [latDeg lonDeg]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    double lat = argc > 6 ? atof(argv[5]) * DEG : 0.2, lon = argc > 6 ? atof(argv[6]) * DEG : 0.7;
    Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    StarSystem sys; sys.generate(s);
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    Culture c = cultureOf(g);
    printf("%s, %s, R %.0f km: %s; culture style %d, family %d, tall %.2f, decay %.2f, %s roofs, towns' plan %d, towers %d, buried %.1f m; the sea stood at %.0f m\n",
           b.name.c_str(), PLANET_TYPES[b.type].name, b.radiusKm, g.hasTrait(TR_CIVILISATION) ? "a people lived here" : "no civilisation", c.style, c.family, c.tall, c.decay,
           c.stoneRoofs ? "stone" : "no", c.plan, (int)c.towers, c.buried, g.oldSeaM > -1e8 ? g.oldSeaM : 0.0);
    int gLat0, gLon0; ruinCellOf(g, lat, lon, gLat0, gLon0);
    int cnt[4] = {0, 0, 0, 0}, ok[4] = {0, 0, 0, 0}, mono = 0, printed = 0, shardsN[4] = {0, 0, 0, 0};
    DrainageOff off;
    for (int dl = -15; dl <= 15; dl++)
        for (int dn = -15; dn <= 15; dn++) {
            RuinSpec sp;
            if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, true)) continue;
            if (sp.kind != RK_SETTLEMENT) { mono++; continue; }
            cnt[sp.sclass]++;
            bool good = ruinSiteOk(g, sp);
            std::vector<ShardSite> ss; shardSitesOf(sp, c, ss); shardsN[sp.sclass] += (int)ss.size();   // C-03
            if (good) ok[sp.sclass]++;
            if (printed < 14 || (good && printed < 20)) {
                Vec3 u = StarSystem::bodyFromLatLon(sp.lat, sp.lon);
                SurfaceSample s0 = sampleSurface(g, u, 64.0);
                double dAng = sp.size / (g.R * 1000.0), hmin = s0.height, hmax = s0.height; int wet = 0, sea = s0.oldSea > 0.5;
                for (int k = 0; k < 4; k++) {
                    double a = k * PI / 2;
                    SurfaceSample q = sampleSurface(g, StarSystem::bodyFromLatLon(sp.lat + std::cos(a) * dAng, sp.lon + std::sin(a) * dAng / std::max(std::cos(sp.lat), 0.05)), 64.0);
                    if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water)) wet++;
                    if (q.oldSea > 0.5) sea++;
                    hmin = std::min(hmin, q.height); hmax = std::max(hmax, q.height);
                }
                printf("  %-8s at %7.3f %8.3f: %2zu buildings, plan %d, r %3.0f m: %s (h %.0f %s, water %s, %d corners wet, %d on the old sea, spread %.0f m of %.0f)\n",
                       SETTLEMENT_CLASS_NAMES[sp.sclass], sp.lat / DEG, sp.lon / DEG, sp.buildings.size(), sp.plan, sp.size, good ? "ok" : "no", s0.height, MATERIAL_NAMES[s0.material],
                       s0.water > -1e8 ? "yes" : "none", wet, sea, hmax - hmin, 0.5 * sp.size);
                if (!ss.empty()) { printf("           shards:"); for (const ShardSite& s : ss) printf("  %d in %s %d (%s, %.0f %.0f)", s.index, BUILDING_KIND_NAMES[sp.buildings[s.building].kind], s.building, SHARD_PLACE_NAMES[s.place], s.x, s.z); printf("\n"); }   // C-03
                printed++;
            }
        }
    printf("31 x 31 cells round %.2f %.2f: %d hamlets (%d sited), %d villages (%d), %d towns (%d), %d monuments (%d), %d monoliths of the old ones; shards: %d in the hamlets, %d in the villages, %d in the towns, %d at the monuments\n",
           lat / DEG, lon / DEG, cnt[0], ok[0], cnt[1], ok[1], cnt[2], ok[2], cnt[3], ok[3], mono, shardsN[0], shardsN[1], shardsN[2], shardsN[3]);
    return 0;
}

static void renderTraits(const char* want) {
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    for (int tr = 1; tr <= TR_GEYSERS; tr++) {
        std::string nm = TRAIT_NAMES[tr];
        for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch);
        if (want && nm != want) continue;
        bool done = false;
        for (int64_t x = 150; x < 340 && !done; x++)
            for (int64_t z = 20; z < 160 && !done; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && !done; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (!PLANET_TYPES[b.type].landable) continue;
                    BodyGen g = BodyGen::make(b);
                    if (!g.hasTrait(tr)) continue;
                    // the strongest spot on a 2 x 1 degree grid, then the best of a finer grid round it; the scan sees this
                    // trait alone (a copy of the body with the others removed), so a canyon's walls do not hide the spires
                    BodyGen gOnly = g; gOnly.traits[0] = tr; gOnly.traits[1] = 0; gOnly.traits[2] = 0;
                    setDrainageEnabled(false);   // O6-03: the scans sample the whole globe at 4-64 m; the frames below turn it back on
                    double bestS = 0, bestLat = 0, bestLon = 0;
                    bool fine = tr == TR_YARDANGS || tr == TR_POLYGONS || tr == TR_SPIRES || tr == TR_SALT_FLATS;   // the ones the 16 m ring (or the 4 m ring) alone carries
                    double scanD = (tr == TR_SPIRES && b.type != PT_QUARTZ && b.type != PT_CARBON) || tr == TR_POLYGONS ? 4 : (fine ? 16 : 64);
                    for (double la = -66; la <= 66; la += 2)
                        for (int k = 0; k < 360; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(la * DEG, ln), scanD);
                            if (ss.material == MAT_WATER) continue;
                            if (ss.landform > bestS) { bestS = ss.landform; bestLat = la * DEG; bestLon = ln; }
                        }
                    if (bestS < 4) { printf("%-16s %s: nothing over 4 m (best %.1f)\n", TRAIT_NAMES[tr], b.name.c_str(), bestS); done = true; break; }
                    double fineS = 0, fLat = bestLat, fLon = bestLon;
                    for (int j = -12; j <= 12; j++)
                        for (int i = -12; i <= 12; i++) {
                            double la = bestLat + j * 800.0 / (b.radiusKm * 1000.0), ln = bestLon + i * 800.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(bestLat)));
                            SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(la, ln), scanD);
                            if (ss.material == MAT_WATER) continue;
                            if (ss.landform > fineS) { fineS = ss.landform; fLat = la; fLon = ln; }
                        }
                    // the standing camera: west of the spot, on dry ground outside the landform's mark (a mesa's or an inselberg's
                    // body would swallow it), 100 m past the first such point within 3 km
                    double standLon = fLon - 250.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
                    for (double dd = 60; dd <= 3000; dd += 60) {
                        double ln = fLon - dd / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
                        SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(fLat, ln), 16);
                        if (ss.material == MAT_WATER || (ss.water > -1e8 && ss.height < ss.water + 0.3) || ss.landform >= 1000) continue;
                        standLon = ln - 100.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
                        break;
                    }
                    // the sheet: the hillshade, a standing frame looking east at the spot, an aerial frame
                    setDrainageEnabled(true);   // O6-03: the frames show the real ground
                    const int N = 200; std::vector<uint32_t> hs;
                    SurfaceSite site; site.init(&sys, bi, fLat, fLon, 1000.0);
                    bool big = tr == TR_RIFT || tr == TR_ESCARPMENTS || tr == TR_GREAT_BASIN || tr == TR_CORONAE, mid = tr == TR_CANYONS || tr == TR_CINDER_FIELD || tr == TR_CHAOS || tr == TR_INSELBERGS || tr == TR_ERG || tr == TR_MESAS;
                    double cellM = big ? 800.0 : (mid ? 120.0 : 40.0);
                    hillshade(site, N, cellM, big ? 0.35 : 1.0, hs);
                    std::vector<uint32_t> f1, f2;
                    double la2 = fLat, lo2 = standLon;
                    renderSiteFrame(sys, bi, la2, lo2, PI / 2, -1, 0.04, sr, nb, f1);
                    renderSiteFrame(sys, bi, la2, lo2, PI / 2, 350, -0.6, sr, nb, f2);
                    Sheet sheet(N * 2 + FBW * 2, std::max(N * 2, FBH));
                    sheet.blit(hs, N * 2, N * 2, 0, 0);
                    sheet.blit(f1, FBW, FBH, N * 2, 0);
                    sheet.blit(f2, FBW, FBH, N * 2 + FBW, 0);
                    sheet.label(4, 4, fmt("%s  %s (%s)  %.1f%s %.1f%s  strength %.0f", TRAIT_NAMES[tr], b.name.c_str(), PLANET_TYPES[b.type].name, std::fabs(fLat / DEG), fLat >= 0 ? "N" : "S", std::fabs(fLon / DEG), fLon >= 0 ? "E" : "W", fineS));
                    sheet.label(4, N * 2 - 12, fmt("%.0f KM, %.0f M CELLS, NORTH UP", N * cellM / 1000, cellM));
                    sheet.label(N * 2 + 4, 4, fmt("STANDING %.0f M WEST OF THE SPOT", (fLon - standLon) * b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat))));
                    sheet.label(N * 2 + FBW + 4, 4, "350 M UP, LOOKING EAST AND DOWN");
                    sheet.save("shots/tests/trait_" + nm + ".png");
                    if (tr == TR_GEYSERS) {   // R-307: the vents the surface will put its geysers on
            std::vector<GeyserVent> vents; geyserVents(g, StarSystem::bodyFromLatLon(bestLat, bestLon), vents);
            int onSinter = 0; double nearest = 1e9;
            for (const GeyserVent& v : vents) {
                double la, lo; StarSystem::latLonFromBody(v.unit, la, lo);
                double d = std::acos(clampd(dot(v.unit, StarSystem::bodyFromLatLon(bestLat, bestLon)), -1, 1)) * b.radiusKm * 1000.0;
                nearest = std::min(nearest, d);
                if (sampleSurface(g, v.unit, 16).material == MAT_SALT) onSinter++;
            }
            printf("  geyser vents near the spot: %d, %d on sinter, the nearest %.0f m\n", (int)vents.size(), onSinter, nearest);
        }
        printf("%-16s %s (%s): traits [%s], spot %.2f %.2f, strength %.0f m\n", TRAIT_NAMES[tr], b.name.c_str(), PLANET_TYPES[b.type].name, traitList(g).c_str(), fLat / DEG, fLon / DEG, fineS);
                    done = true;
                }
            }
        if (!done) printf("%-16s: no body carries it in the scanned sectors\n", TRAIT_NAMES[tr]);
    }
    if (!want) {   // the distribution over the first 300 landable bodies
        int cnt[TR_COUNT] = {0}, n = 0, withN[4] = {0, 0, 0, 0};
        for (int64_t x = 150; x < 340 && n < 300; x++)
            for (int64_t z = 20; z < 160 && n < 300; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && n < 300; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (!PLANET_TYPES[b.type].landable) continue;
                    BodyGen g = BodyGen::make(b);
                    int k = 0; for (int j = 0; j < 3; j++) if (g.traits[j]) { cnt[g.traits[j]]++; k++; }
                    withN[k]++; n++;
                }
            }
        printf("traits over %d landable bodies: none %d, one %d, two %d, three %d\n", n, withN[0], withN[1], withN[2], withN[3]);
        for (int tr = 1; tr < TR_COUNT; tr++) printf("  %-16s %3d\n", TRAIT_NAMES[tr], cnt[tr]);
    }
}

// `landforms <type> [flat|hills|mountains]`: twelve hashed land sites of the type (of that class) on one sheet, each a
// standing frame toward the highest ground within a kilometre over hillshades of 770 m, 3 km and 24 km
static void renderLandforms(int type, int wantCls) {
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
    std::string tn = PLANET_TYPES[type].name;
    for (char& ch : tn) if (ch == ' ') ch = '_';
    sheet.save("shots/tests/landforms_" + tn + (wantCls >= 0 ? std::string("_") + CLASS_NAMES[wantCls] : std::string()) + ".png");
    printf("landforms: %d sites of %s%s%s\n", n, PLANET_TYPES[type].name, wantCls >= 0 ? " of class " : "", wantCls >= 0 ? CLASS_NAMES[wantCls] : "");
}

// `landforms sites`: the fixed sites of the O6 review, each on its own sheet at three distances (standing, 150 m up
// looking down 12 deg, 2 km up looking down 30 deg) over the three hillshades and the metrics; the latitude and
// longitude are fixed so the sheets before and after a generator change show the same ground (0/0 searches a site of
// the wanted class on the first body of the type and prints what to pin)
struct LandformSite { const char* name; int type; bool moon; int wantCls; double latDeg, lonDeg; };
static const LandformSite LANDFORM_SITES[] = {
    {"felisian_mountains", PT_FELISIAN, false, 2, PIN_MOUNTAIN_LAT, PIN_MOUNTAIN_LON},   // pinned 2026-09-27 on the generation-5 function; O6-03: re-pinned on the GEN 10 bodies (the type table of GEN 9 had moved every world)
    {"felisian_plain", PT_FELISIAN, false, 0, -55.154, -122.371},   // S-01: re-pinned on Wyariothmai I's day side (a sand plain: the lit face of the locked world is dry); 7.424 / 75.762 before, in the sea now
    {"thinatmo_hills", PT_THINATMO, false, 1, 44.433, 135.970},
    {"cratered_moon", PT_CRATERED, true, 1, -13.184, 175.712},   // S-01: re-pinned on Wyariothmai II-a, the scan's first cratered moon since the claim (43.934 / -41.488 before)
};
static void renderLandformSites(bool search) {   // G-04: `landforms sites search` lists six candidates of each site's class on its world (as a 0/0 pin does)
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
                if (sys.bodies[bi].locked && st.sun(1000.0).altitude < 10 * DEG) continue;   // S-01: a world locked to its star has a day side and a night side for good: the sheets and the stability frames want the light
                SiteMetrics m = measureSite(st, false);
                if (m.cls != L.wantCls || m.material == MAT_WATER || m.material == MAT_SNOW || m.material == MAT_ICE) continue;
                if (!found) { lat = s.first; lon = s.second; found = true; }
                printf("%s: %s lat %.3f lon %.3f (%s, %s)\n", L.name, found && listed == 0 ? "pin" : "or ", s.first / DEG, s.second / DEG, CLASS_NAMES[m.cls], MATERIAL_NAMES[m.material]);   // G-04: the first six candidates, for `descent lat lon`
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
        const char* lines[4] = {title.c_str(), l2, l3, l4};
        for (int k = 0; k < 4; k++) drawText(sheet.cv, 330, 204 + k * 11, lines[k], k ? rgb(200, 230, 255) : rgb(255, 220, 120));
        sheet.save(std::string("shots/tests/landforms_site_") + L.name + ".png");
        printf("%-20s %s\n", L.name, metricsLine(m).c_str());
    }
}

// M6-02: the key map file and the gamepad mapping, checked without a window
static int testInput() {
    int fails = 0;
    {
        FILE* f = fopen("shots/tests/test_keys.txt", "w");
        fprintf(f, "# AZERTY: Z acts as W and W as Z, Q as A and A as Q\nZ W\nW Z\nQ A\nA Q\nF7 P\n");
        fclose(f);
        KeyMap km;
        bool ok = km.load("shots/tests/test_keys.txt");
        Input in;
        in.down[KEY_Z] = true; in.pressed[KEY_Z] = true; in.down[KEY_F7] = true; in.pressed[KEY_F7] = true;
        km.apply(in);
        bool good = ok && in.isDown(KEY_W) && in.wasPressed(KEY_W) && !in.isDown(KEY_Z) && in.wasPressed(KEY_P) && !in.isDown(KEY_F7);
        printf("keymap: %s (Z->W, F7->P)\n", good ? "PASS" : "FAIL");
        if (!good) fails++;
        // N5-05: the key bindings screen: open it from the settings, rebind FORWARD to Z, reset it, save
        {
            Game game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys2.txt"; game.guidePath = "shots/tests/test_guide.txt";
            KeyMap km2; game.keymap = &km2;
            Input gi;
            auto press = [&](int key) { gi.pressed[key] = true; gi.down[key] = true; game.frame(gi, 1.0 / 30); gi.newFrame(); gi.down[key] = false; };
            press(KEY_ENTER); for (int i = 0; i < 3; i++) game.frame(gi, 1.0 / 30);
            press(KEY_ESCAPE); press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER);   // menu > settings
            for (int i = 0; i < 18; i++) press(KEY_DOWN);   // KEY BINDINGS
            press(KEY_ENTER);
            bool opened = game.state == GameState::KEYS;
            press(KEY_ENTER);                                 // rebind FORWARD
            game.rawKey = KEY_Z; game.frame(gi, 1.0 / 30); game.rawKey = -1;
            bool bound = km2.map[KEY_Z] == KEY_W;
            writePNG("shots/tests/keys.png", game.output(), FBW, FBH);
            press(KEY_BACKSPACE);
            bool reset = km2.map[KEY_Z] == KEY_Z;
            press(KEY_ENTER); game.rawKey = KEY_Z; game.frame(gi, 1.0 / 30); game.rawKey = -1;
            press(KEY_ESCAPE);
            KeyMap km3; bool saved = km3.load("shots/tests/test_keys2.txt") && km3.map[KEY_Z] == KEY_W;
            bool ok2 = opened && bound && reset && saved && game.state == GameState::SETTINGS;
            printf("key bindings screen: %s (opened %d, Z->W %d, reset %d, saved and reloaded %d) -> shots/keys.png\n", ok2 ? "PASS" : "FAIL", (int)opened, (int)bound, (int)reset, (int)saved);
            if (!ok2) fails++;
        }
        if (!good) fails++;
        printf("key names: %s %s %s %s\n", keyName(keyFromName("lshift")).c_str(), keyName(keyFromName("F12")).c_str(), keyName(keyFromName("space")).c_str(), keyName(KEY_PAGE_UP).c_str());
    }
    {
        PadState cur, prev;
        cur.present = true; cur.lx = 0.9; cur.ly = -1.0; cur.rx = 0.5; cur.buttons[0] = true; cur.rt = 0.9;
        Input in;
        applyPad(cur, prev, in, 1.0);
        bool good = in.moveY > 0.9 && in.moveX > 0.8 && in.mouseDx > 1 && in.isDown(KEY_ENTER) && in.wasPressed(KEY_ENTER) && in.isDown(KEY_LEFT_SHIFT);
        Input in2;
        applyPad(cur, prev, in2, 1.0);   // held: down but not pressed again
        good = good && in2.isDown(KEY_ENTER) && !in2.wasPressed(KEY_ENTER);
        PadState none;
        Input in3;
        applyPad(none, prev, in3, 1.0);
        good = good && in3.moveX == 0 && !in3.isDown(KEY_ENTER);
        printf("gamepad: %s (stick -> move, A -> enter once, trigger -> sprint, absent pad -> nothing)\n", good ? "PASS" : "FAIL");
        if (!good) fails++;
    }
    return fails;
}

// M5-06: day length at 82 degrees latitude on a tilted felisian world, half an orbit apart
static void testSeasons() {
    for (int64_t x = 150; x < 320; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN || b.parent >= 0 || b.axialTilt < 18 * DEG || b.locked) continue;
                double lat = 82 * DEG;
                auto dayLen = [&](double t) {
                    double dec = std::asin(clampd(sys.seasonOf(bi, t), -1, 1));
                    double cosH0 = -std::tan(lat) * std::tan(dec);
                    return cosH0 <= -1 ? 1e9 : (cosH0 >= 1 ? 0.0 : std::fabs(b.rotPeriod) * std::acos(cosH0) / PI);
                };
                // find the northern summer solstice by scanning one orbit
                double P = std::fabs(b.orbitPeriod), tBest = 0, best = -2;
                for (int k = 0; k < 96; k++) { double t = 1000.0 + P * k / 96.0; double v = sys.seasonOf(bi, t); if (v > best) { best = v; tBest = t; } }
                double d1 = dayLen(tBest), d2 = dayLen(tBest + P * 0.5);
                auto show = [&](double d) { char buf[64]; if (d > 1e8) return std::string("polar day"); if (d <= 0) return std::string("polar night"); snprintf(buf, sizeof buf, "%.1f h of %.1f", d / 3600, std::fabs(b.rotPeriod) / 3600); return std::string(buf); };
                printf("seasons: %s tilt %.1f deg, at 82 N: summer solstice %s, winter solstice %s (orbit %.1f d)\n", b.name.c_str(), b.axialTilt / DEG, show(d1).c_str(), show(d2).c_str(), P / 86400);
                bool ok = (d1 > 1e8 || d1 > d2 + 3600);
                printf("seasons: %s\n", ok ? "PASS (different day lengths)" : "FAIL");
                return;
            }
        }
    printf("seasons: no tilted felisian world found\n");
}

int main(int argc, char** argv) {
    makeDir("shots");
    makeDir("shots/tests");   // O7-01: every harness output lands here; shots/ itself holds the player's screenshots
    // `scale=N` anywhere on the command line renders at that resolution scale
    std::vector<char*> args;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "scale=", 6) == 0) g_testScale = atoi(argv[i] + 6); else args.push_back(argv[i]);
    }
    argc = (int)args.size(); argv = args.data();
    if (g_testScale < 1) g_testScale = 1;
    if (g_testScale > 4) g_testScale = 4;
    setFramebufferScale(g_testScale);
    std::string mode = argc > 1 ? argv[1] : "stars";
    if (mode == "scene") {
        std::string want = argc > 2 ? argv[2] : "felisian_shore";
        if (argc > 3) setFramebufferScale(atoi(argv[3]));   // `scene <name> [scale] [warm]` (B-310): the frame at 1x .. 4x; R-307: warm > 0 runs that many seconds of the world first and stages the type's event in view
        double warm = argc > 4 ? atof(argv[4]) : 0;
        SpaceRenderer sr; StarNeighborhood nb;
        for (const CmpScene& sc : CMP_SCENES) {
            if (want != sc.name) continue;
            StarSystem sys; SurfaceView sv; double t = 0;
            if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no body\n"); return 1; }
            if (warm > 0) {
                Input in;
                for (double w = 0; w < warm; w += 0.05) { t += 0.05; sv.update(0.05, in, t, false); }
                double fx = std::sin(sv.player.yaw), fz = std::cos(sv.player.yaw);
                if (sc.type == PT_TECTONIC) {   // a fountain on the nearest lava cell ahead, six seconds in
                    bool found = false;
                    for (double d = 160; d <= 420 && !found; d += 8)
                        for (double s2 = -d * 0.45; s2 <= d * 0.45 && !found; s2 += 8) {   // within the view's cone, far enough for the column to fit the frame
                            double x = sv.player.x + fx * d - fz * s2, z = sv.player.z + fz * d + fx * s2;
                            if (sv.site.lod0.at((int)std::floor(x / 16), (int)std::floor(z / 16)).material == MAT_LAVA) { sv.eruptions.push_back({x, z, t - 6, 1}); found = true; }
                        }
                    if (found) { const SurfaceView::Eruption& e = sv.eruptions.back(); printf("  fountain staged %.0f m ahead\n", std::sqrt((e.x - sv.player.x) * (e.x - sv.player.x) + (e.z - sv.player.z) * (e.z - sv.player.z))); }
                    else printf("  fountain staged: no lava 160-420 m ahead\n");
                    sv.quake = 0.8;
                }
                if (sc.type == PT_BOMBARDED) sv.eruptions.push_back({sv.player.x + fx * 220, sv.player.z + fz * 220, t - 1.5, 2});
                if (sc.wantMat == -36) {   // move the clock to a moment the nearest vent's jet is at full strength
                    std::vector<GeyserVent> vents; geyserVents(sv.site.gen, sv.site.unitAt(sv.player.x, sv.player.z), vents);
                    uint64_t best = 0; double bd = 1e18;
                    for (const GeyserVent& v : vents) { double vx, vz; sv.site.localAt(v.unit, vx, vz); double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z); if (d < bd) { bd = d; best = v.id; } }
                    double t0 = t;
                    for (double dtq = 0; dtq < 400 && SurfaceView::basinJetStrength(best, t) < 0.9; dtq += 0.5) t = t0 + dtq;
                    Input in2; sv.update(0.05, in2, t, false);
                    printf("  nearest vent %.0f m off, strength %.2f at t+%.0f s\n", std::sqrt(bd), SurfaceView::basinJetStrength(best, t), t - t0);
                }
                if (sc.type == PT_DESERT) { SurfaceView::DustDevil d; d.x = sv.player.x + fx * 130 - fz * 20; d.z = sv.player.z + fz * 130 + fx * 20; d.height = 70; d.radius = 12; d.age = 40; d.life = 300; d.phase = 1.0; sv.devils.push_back(d); }
            }
            Framebuffer fb;
            sv.render(fb, t, nb.stars, sr, 1.0);
            fb.mush(2);
            if (sv.inBuggy && !sv.chaseCam) sv.cameraFeed(fb, t);   // R-204: the nose camera's picture
            std::string fn = std::string("shots/tests/scene_") + sc.name + ".png";
            saveFB(fb, fn.c_str());
            printf("%s -> %s (sun alt %.1f)\n", sc.name, fn.c_str(), sv.env.sun.altitude / DEG);
            if (sc.wantMat <= -40 && sc.wantMat >= -42)   // B-403: the aurora's numbers
                printf("  aurora %.2f under a %s at lat %.1f (oval at %.1f, centre %.0f km poleward), field %.2f (%s), storm %.2f, R %.0f km, sky %.2f, cloud %.2f, flocks %zu\n", sv.env.aurora, STAR_CLASSES[sys.star.cls].name, sv.env.latDeg,
                       SurfaceView::auroraOvalLat(sys.bodies[sv.site.body]), sv.lastAuroraP0, magneticField(sys.bodies[sv.site.body]), MAGNETIC_CLASS_NAMES[magneticClass(magneticField(sys.bodies[sv.site.body]))], sv.env.auroraStorm,
                       sv.site.R / 1000.0, sv.env.skyBrightness, sv.env.cloudCover, sv.flocks.size());
            if (sc.wantMat == -47 || sc.wantMat == -49)   // C-01; C-03: the shards drawn and the one within reach
                printf("  ruins: %d pieces drawn in %.2f ms, %d shards drawn%s\n", sv.lastRuinElems, sv.lastRuinMs, sv.lastShardsDrawn,
                       sv.nearShard.index >= 0 ? fmt(", shard %d within reach (%s, %.1f m)", sv.nearShard.index, SHARD_PLACE_NAMES[sv.nearShard.place], sv.nearShard.dist).c_str() : "");
            if (sc.wantMat == -3 || sc.wantMat == -5 || sc.wantMat == -6 || sc.wantMat == -7)   // N3: the herd's states and the cost of life
                printf("  herd: %s; life update %.2f ms, draw %.2f ms\n", sv.testHerdStates().c_str(), sv.lastLifeMs[0], sv.lastLifeMs[1]);
            if (sc.wantMat == -4 || sc.wantMat <= -10) {   // N2: what the vegetation did
                const TerrainVertex& tvp = sv.site.lod0.at((int)std::floor(sv.player.x / 16), (int)std::floor(sv.player.z / 16));
                double cov = sv.testCanopyCoverage(200);
                printf("  biome %s, veg %.2f; trees drawn %d (near %d, mid %d, far %d), flora points %d; canopy coverage of the forest ground within 200 m: %.0f%%%s\n", BIOME_NAMES[tvp.biome], tvp.veg,
                       sv.lastTreesDrawn, sv.lastNearTrees, sv.lastMidTrees, sv.lastFarTrees, sv.lastFloraPoints, cov * 100, (sc.wantMat == -4 && cov < 0.6) ? " (FAIL: under 60%)" : "");
                printf("  families %d/%d, vegColor %.2f,%.2f,%.2f, vegColor2 %.2f,%.2f,%.2f, season phase %.2f, fog %.0f m, wind %.0f kn; bank 0 stops 14/34 = %d,%d,%d / %d,%d,%d; pixel(160,120) bank %d shade %.0f\n",
                       sv.site.gen.floraFamily, sv.site.gen.floraFamily2, sv.site.gen.vegColor.r, sv.site.gen.vegColor.g, sv.site.gen.vegColor.b, sv.site.gen.vegColor2.r, sv.site.gen.vegColor2.g, sv.site.gen.vegColor2.b,
                       sv.seasonPhaseLocal(), sv.env.fogDistance, sv.env.windKnots, fb.pal[14 * 3], fb.pal[14 * 3 + 1], fb.pal[14 * 3 + 2], fb.pal[34 * 3], fb.pal[34 * 3 + 1], fb.pal[34 * 3 + 2],
                       bankOf(fb.idx[(FBH * 3 / 5) * FBW + FBW / 2]), shadeOf(fb.idx[(FBH * 3 / 5) * FBW + FBW / 2]));
            }
            {   // N1: what the lower half is drawn with (bank histogram, mean shade, the majority bank's ramp stops)
                int hist[BANKS] = {0}; double sum = 0; int n = 0;
                for (int y = FBH / 2; y < FBH; y++) for (int x = 0; x < FBW; x++) { hist[bankOf(fb.idx[y * FBW + x])]++; sum += shadeOf(fb.idx[y * FBW + x]); n++; }
                int mb = 0; for (int k = 1; k < BANKS; k++) if (hist[k] > hist[mb]) mb = k;
                printf("  lower half: bank %d %d%%, mean shade %.1f; bank %d stops 14/34/47 = %d,%d,%d / %d,%d,%d / %d,%d,%d; sky %.2f snow %.2f dust %.2f underwater %d\n", mb, hist[mb] * 100 / n, sum / n, mb,
                       fb.pal[(mb * 64 + 14) * 3], fb.pal[(mb * 64 + 14) * 3 + 1], fb.pal[(mb * 64 + 14) * 3 + 2], fb.pal[(mb * 64 + 34) * 3], fb.pal[(mb * 64 + 34) * 3 + 1], fb.pal[(mb * 64 + 34) * 3 + 2],
                       fb.pal[(mb * 64 + 47) * 3], fb.pal[(mb * 64 + 47) * 3 + 1], fb.pal[(mb * 64 + 47) * 3 + 2], sv.env.skyBrightness, sv.env.snow, sv.env.dust, (int)sv.player.underwater);
            }
        }
        return 0;
    }
    if (mode == "capsule") {   // B-318: the surface as the capsule sees it on the way down: eight altitudes of the descent, the share of sky below the horizon
        std::string want = argc > 2 ? argv[2] : "felisian_mountains";
        SpaceRenderer sr; StarNeighborhood nb;
        for (const CmpScene& sc : CMP_SCENES) {
            if (want != sc.name) continue;
            StarSystem sys; SurfaceView sv; double t = 0;
            if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no body\n"); return 1; }
            printf("  site: star %lld %lld %lld body %d lat %.4f lon %.4f t %.1f\n", (long long)sys.star.sx, (long long)sys.star.sy, (long long)sys.star.sz, sv.site.body, sv.site.lat0 / DEG, sv.site.lon0 / DEG, t);
            const double alts[8] = {1800, 1300, 900, 600, 400, 250, 120, 40};
            std::vector<uint32_t> sheet((size_t)FBW * 4 * FBH * 2, 0xFF000000u), px((size_t)FBW * FBH);
            bool bad = false;
            for (int k = 0; k < 8; k++) {
                double s = 1 - alts[k] / 1800.0;
                sv.cameraOverrideAlt = alts[k]; sv.cameraOverridePitch = (-60 + 60 * s) * DEG;
                Input in; sv.update(0.016, in, t, false);
                Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0);
                // sky-bank pixels below the horizon row are holes in the ground
                double dip = std::sqrt(2 * alts[k] / sv.site.R);   // the horizon sits below the level line by the curvature's dip at this height
                double horizon = sv.proj.cy - sv.proj.f * std::tan(-sv.cameraOverridePitch - dip) + 1;
                int holes = 0, rows = 0;
                for (int y = std::max(0, (int)std::ceil(horizon)); y < FBH; y++) for (int x = 0; x < FBW; x++) { rows++; if (bankOf(fb.idx[y * FBW + x]) == 1 && fb.invz[y * FBW + x] == 0) holes++; }   // the beacon beam is on the sky bank too, but has a depth
                double share = rows ? 100.0 * holes / rows : 0.0;
                if (share > 0.5) bad = true;
                printf("  %4.0f m pitch %5.1f: sky below the horizon %.2f%%%s\n", alts[k], sv.cameraOverridePitch / DEG, share, share > 0.5 ? "  <-- HOLES" : "");
                fb.mush(2); fb.toRGB(px.data(), 1.0);
                int ox = (k % 4) * FBW, oy = (k / 4) * FBH;
                for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(oy + y) * FBW * 4 + ox + x] = px[(size_t)y * FBW + x];
            }
            sv.cameraOverrideAlt = -1;
            std::string fn = std::string("shots/tests/capsule_") + sc.name + ".png";
            writePNG(fn.c_str(), sheet.data(), FBW * 4, FBH * 2);
            printf("%s: %s -> %s\n", sc.name, bad ? "FAIL (sky through the ground)" : "ok", fn.c_str());
            return bad ? 1 : 0;
        }
        printf("no such scene\n");
        return 1;
    }
    if (mode == "spot") {   // B-322: a frame at an exact place: `spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> <t> <swim|ascent|stand> [pitchDeg] [yawDeg] [alt]`
        if (argc < 9) { printf("spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> <t> <swim|ascent|stand> [pitchDeg] [yawDeg] [alt]\n"); return 1; }
        Star s;
        if (!starInSector(atoll(argv[2]), atoll(argv[3]), atoll(argv[4]), s)) { printf("no star\n"); return 1; }
        StarSystem sys; sys.generate(s);
        int bi = atoi(argv[5]);
        if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
        double lat = atof(argv[6]) * DEG, lon = atof(argv[7]) * DEG, t = atof(argv[8]);
        std::string kind = argv[9];
        double pitchDeg = argc > 10 ? atof(argv[10]) : 0, yawDeg = argc > 11 ? atof(argv[11]) : 0, alt = argc > 12 ? atof(argv[12]) : 1500;
        SpaceRenderer sr; StarNeighborhood nb; SurfaceView sv;
        sv.init(&sys, bi, lat, lon, t);
        nb.update(sv.site.worldPos(t, 0, 0, 0));
        const Body& b = sys.bodies[bi];
        printf("%s (%s) at %.3f %.3f, t %.1f, sea %s %.1f, ground %.1f water %.1f\n", b.name.c_str(), PLANET_TYPES[b.type].name, lat / DEG, lon / DEG, t, sv.site.hasWater ? "yes" : "no", sv.site.seaLevel, sv.site.groundHeight(0, 0), sv.site.waterAt(0, 0));
        if (kind == "swim") {
            // the nearest water at least 3 m deep within 3 km, the player afloat in it
            double bx = 0, bz = 0, bd = 1e9;
            double minDepth = argc > 12 ? alt : 3.0;   // the twelfth argument: the depth wanted (default 3 m)
            for (double r = 0; r <= 3000 && bd > 1e8; r += 4)
                for (int a = 0; a < 64; a++) {
                    double x = std::sin(a * TAU / 64) * r, z = std::cos(a * TAU / 64) * r;
                    double w = sv.site.waterAt(x, z), g = sv.site.groundHeight(x, z);
                    if (w > -1e8 && w - g > minDepth && r < bd) { bd = r; bx = x; bz = z; }
                }
            if (bd > 1e8) { printf("no water within 3 km\n"); return 1; }
            double w = sv.site.waterAt(bx, bz);
            sv.player.x = bx; sv.player.z = bz; sv.player.y = w - 1.15; sv.player.vy = 0;
            sv.player.yaw = yawDeg * DEG; sv.player.pitch = pitchDeg * DEG;
            Input in;
            for (int k = 0; k < 30; k++) sv.update(0.016, in, t, false);
            { Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0); }   // the rings (and the near ring's morph of groundHeight) exist after a frame
            if (sv.site.groundHeight(bx, bz) > w - minDepth) {   // the near ring (4 m) puts land here: look again with the drawn ground
                double bd2 = 1e9, bx2 = bx, bz2 = bz;
                for (double r = 0; r <= 400 && bd2 > 1e8; r += 2)
                    for (int a = 0; a < 64; a++) {
                        double x = bx + std::sin(a * TAU / 64) * r, z = bz + std::cos(a * TAU / 64) * r;
                        double ww = sv.site.waterAt(x, z), g = sv.site.groundHeight(x, z);
                        if (ww > -1e8 && ww - g > minDepth && r < bd2) { bd2 = r; bx2 = x; bz2 = z; }
                    }
                bx = bx2; bz = bz2; w = sv.site.waterAt(bx, bz);
                sv.player.x = bx; sv.player.z = bz; sv.player.y = w - 1.15; sv.player.vy = 0;
                for (int k = 0; k < 30; k++) sv.update(0.016, in, t, false);
            }
            double eyeOff = argc > 13 ? atof(argv[13]) : 1e9;   // the thirteenth argument: the eye's height over the water, forced
            if (eyeOff < 1e8) { sv.player.y = w + eyeOff - sv.player.eyeHeight; sv.player.bobY = 0; sv.player.landDip = 0; sv.player.underwater = eyeOff < -0.05; }
            printf("swimming %d underwater %d at (%.0f, %.0f) eye %.2f water %.2f ground %.2f, %.0f m from the site\n", (int)sv.player.swimming, (int)sv.player.underwater, bx, bz, sv.player.y + sv.player.eyeHeight, w, sv.site.groundHeight(bx, bz), bd);
            Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
            {   // the lod0 and near-ring vertices round the swimmer: height / water / material
                int cx0 = (int)std::floor(bx / 16), cz0 = (int)std::floor(bz / 16);
                for (int dz = 2; dz >= -2; dz--) {
                    for (int dx = -2; dx <= 2; dx++) { const TerrainVertex& v = sv.site.lod0.at(cx0 + dx, cz0 + dz); printf("  %7.1f/%6.1f/m%d", v.h, v.water > -1e8 ? v.water : -99.0, v.material); }
                    printf("\n");
                }
                printf("  near ring (4 m):\n");
                int nx0 = (int)std::floor(bx / 4), nz0 = (int)std::floor(bz / 4);
                for (int dz = 2; dz >= -2; dz--) {
                    for (int dx = -2; dx <= 2; dx++) { const TerrainVertex& v = sv.site.lodN.at(nx0 + dx, nz0 + dz); printf("  %7.1f/%6.1f/m%d", v.h, v.water > -1e8 ? v.water : -99.0, v.material); }
                    printf("\n");
                }
                printf("  groundHeight %.2f coarse %.2f waterAt %.2f nearBand %d lod0R %d nearR %d nearBlend %.2f\n", sv.site.groundHeight(bx, bz), sv.site.groundHeightCoarse(bx, bz), sv.site.waterAt(bx, bz), sv.site.nearBand, sv.site.lod0R, sv.site.nearR, sv.site.nearBlend);
            }
            std::string fn = std::string("shots/tests/spot_swim_") + argv[10 < argc ? 10 : 9] + "_" + argv[11 < argc ? 11 : 9] + (argc > 13 ? std::string("_e") + argv[13] : std::string()) + ".png";
            saveFB(fb, fn.c_str());
            printf("-> %s\n", fn.c_str());
        } else if (kind == "ascent") {
            sv.cameraOverrideAlt = alt; sv.cameraOverridePitch = pitchDeg * DEG; sv.player.yaw = yawDeg * DEG;
            Input in; sv.update(0.016, in, t, false);
            Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
            {   // the 64 m ring's vertices under the camera: height / material (the water is 0 everywhere on a sea)
                int cx0 = (int)std::floor(sv.player.x / 64), cz0 = (int)std::floor(sv.player.z / 64);
                for (int dz = 6; dz >= -6; dz--) {
                    for (int dx = -6; dx <= 6; dx++) { const TerrainVertex& v = sv.site.lod1.at(cx0 + dx, cz0 + dz); printf(" %6.1f/%d", v.h, v.material); }
                    printf("\n");
                }
                printf("  (64 m cells; the same at 16 m:)\n");
                cx0 = (int)std::floor(sv.player.x / 16); cz0 = (int)std::floor(sv.player.z / 16);
                for (int dz = 6; dz >= -6; dz--) {
                    for (int dx = -6; dx <= 6; dx++) { const TerrainVertex& v = sv.site.lod0.at(cx0 + dx, cz0 + dz); printf(" %6.1f/%d", v.h, v.material); }
                    printf("\n");
                }
            }
            std::string fn = std::string("shots/tests/spot_ascent_") + std::to_string((int)alt) + ".png";
            saveFB(fb, fn.c_str());
            printf("-> %s\n", fn.c_str());
        } else if (kind == "dump") {   // the 64 m ring's heights (metres, integer) round the site: W = water material, i = ice, s = sand, . = other
            sv.cameraOverrideAlt = 1000; sv.cameraOverridePitch = -60 * DEG;
            Input in; sv.update(0.016, in, t, false);
            { Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0); }
            int half = (int)(std::fabs(alt) > 0 ? std::fabs(alt) : 15);
            for (int cz = half; cz >= -half; cz--) {
                for (int cx = -half; cx <= half; cx++) {
                    const TerrainVertex& v = sv.site.lod1.at(cx, cz);
                    char m = v.material == MAT_WATER ? 'W' : (v.material == MAT_ICE ? 'i' : (v.material == MAT_SAND ? 's' : (v.material == MAT_DUST ? 'd' : (v.material == MAT_LAVA ? 'L' : (v.material == MAT_BASALT ? 'b' : (v.material == MAT_SULPHUR ? 'y' : (v.material == MAT_SALT ? 'S' : '.')))))));
                    if (alt < 0) {   // negative alt: shore distance / height over the water (metres); "  ----  " = no water here
                        if (v.water > -1e8f) printf(" %4.0f/%+5.1f", v.shore < 1e8f ? v.shore : 9999.0, v.h - v.water); else printf("   ----   ");
                    } else printf("%5d%c", (int)std::lround(v.h), m);
                }
                printf("\n");
            }
        } else {
            sv.player.yaw = yawDeg * DEG; sv.player.pitch = pitchDeg * DEG;
            Input in; sv.update(0.016, in, t, false);
            Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
            saveFB(fb, "shots/tests/spot_stand.png");
            printf("-> shots/tests/spot_stand.png\n");
        }
        return 0;
    }
    if (mode == "matlod") {   // B-404: the planet function's materials (and heights) at the five ring scales, point by point.
        // `matlod [all|<type>] [bodies] [sites] [halfKm] [stepM]` over random land sites, or `matlod spot <sx> <sy> <sz> <body>
        // <latDeg> <lonDeg> [halfKm] [stepM]` round one place: the share of points whose material differs between
        // neighbouring scales (4/16, 16/64, 64/512, 512/2048 m), the pairs that differ, and the rms height difference
        static const double SCALES[5] = {4, 16, 64, 512, 2048};
        static const char* STEPS[4] = {"4/16", "16/64", "64/512", "512/2k"};
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
            Star s; if (!starInSector(atoll(argv[3]), atoll(argv[4]), atoll(argv[5]), s)) { printf("no star\n"); return 1; }
            StarSystem sys; sys.generate(s);
            int bi = atoi(argv[6]); if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
            double halfM = (argc > 9 ? atof(argv[9]) : 3.0) * 1000.0, stepM = argc > 10 ? atof(argv[10]) : 50.0;
            Agg a; measure(sys, bi, atof(argv[7]) * DEG, atof(argv[8]) * DEG, halfM, stepM, a);
            report(sys.bodies[bi].name + " " + PLANET_TYPES[sys.bodies[bi].type].name, a);
            return 0;
        }
        int only = -1;
        if (argc > 2 && std::string(argv[2]) != "all") for (int k = 0; k < PT_COUNT; k++) { std::string nm = PLANET_TYPES[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) only = k; }
        int bodies = argc > 3 ? atoi(argv[3]) : 3, perBody = argc > 4 ? atoi(argv[4]) : 4;
        double halfM = (argc > 5 ? atof(argv[5]) : 3.0) * 1000.0, stepM = argc > 6 ? atof(argv[6]) : 75.0;
        Agg all;
        for (int type = 0; type < PT_COUNT; type++) {
            if (!PLANET_TYPES[type].landable || (only >= 0 && type != only)) continue;
            Agg a;
            forTypeBodies(type, bodies, false, [&](const StarSystem& sys, int bi) {
                std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x4A, perBody, sites);
                Agg b;
                for (auto& st : sites) measure(sys, bi, st.first, st.second, halfM, stepM, b);
                std::string tr = traitList(BodyGen::make(sys.bodies[bi]));
                printf("  %-26s %s: 16/64 %.2f%%  64/512 %.2f%%  512/2k %.2f%%\n", sys.bodies[bi].name.c_str(), tr.empty() ? "-" : tr.c_str(), 100.0 * b.diff[1] / std::max(1L, b.n), 100.0 * b.diff[2] / std::max(1L, b.n), 100.0 * b.diff[3] / std::max(1L, b.n));
                a.n += b.n; for (int k = 0; k < 4; k++) { a.diff[k] += b.diff[k]; a.h2[k] += b.h2[k]; for (auto& kv : b.pairs[k]) a.pairs[k][kv.first] += kv.second; }
            });
            report(PLANET_TYPES[type].name, a);
            all.n += a.n; for (int k = 0; k < 4; k++) { all.diff[k] += a.diff[k]; all.h2[k] += a.h2[k]; for (auto& kv : a.pairs[k]) all.pairs[k][kv.first] += kv.second; }
        }
        if (only < 0) report("ALL", all);
        return 0;
    }
    if (mode == "landmarks") {   // O6-06: the sights round random land sites of a type (default felisian): kinds, the share of sites with one within 10 km, the cost of a cell
        int type = PT_FELISIAN;
        if (argc > 2) for (int k = 0; k < PT_COUNT; k++) { std::string nm = PLANET_TYPES[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) type = k; }
        int sites = 0, within10 = 0, kinds[LM_COUNT] = {0}; double cellMs = 0; int cells = 0;
        forTypeBodies(type, 3, false, [&](const StarSystem& sys, int bi) {
            const Body& b = sys.bodies[bi];
            BodyGen g = BodyGen::make(b);
            std::vector<std::pair<double, double>> ls; landSitesOf(sys, bi, 0x1A, 8, ls);
            for (auto& st : ls) {
                Vec3 u = StarSystem::bodyFromLatLon(st.first, st.second);
                std::vector<Landmark> found;
                double t0 = nowSec();
                landmarksNear(g, b.name, u, 10000.0, found);
                double dt = (nowSec() - t0) * 1e3;
                int ci, cj; landmarkCellOf(g, u, ci, cj);
                double cellDeg = landmarkCellDeg(g);
                int nCells = (int)std::ceil(20000.0 / (cellDeg * DEG * g.R * 1000.0) + 1); nCells *= nCells;
                cellMs += dt; cells += nCells;
                sites++;
                bool near = false;
                for (const Landmark& L : found) {
                    kinds[L.kind]++;
                    double dlat, dlon; StarSystem::latLonFromBody(L.unit, dlat, dlon);
                    double d = std::acos(clampd(dot(L.unit, u), -1, 1)) * g.R * 1000.0;
                    if (d < 10000.0) near = true;
                    if (sites <= 3) printf("  %-16s %s %-14s %+.3f %+.3f  h %.0f m, prominence %.0f m, radius %.0f m, %.1f km off\n", b.name.c_str(), LANDMARK_KIND_SYMBOLS[L.kind], L.name.c_str(), dlat / DEG, dlon / DEG, L.heightM, L.prominenceM, L.radiusM, d / 1000.0);
                }
                if (near) within10++;
            }
        });
        printf("landmarks (%s): %d sites, %d (%.0f%%) with a landmark within 10 km; kinds:", PLANET_TYPES[type].name, sites, within10, sites ? 100.0 * within10 / sites : 0.0);
        for (int k = 0; k < LM_COUNT; k++) printf(" %s %d", LANDMARK_KIND_NAMES[k], kinds[k]);
        printf("; %.2f ms per grid cell searched (%d searched: scan %.1f ms, lakes %.1f ms each)\n", cells ? cellMs / cells : 0.0, landmarkCellsSearched(), landmarkCellsSearched() ? landmarkScanMs(0) / landmarkCellsSearched() : 0.0, landmarkCellsSearched() ? landmarkScanMs(1) / landmarkCellsSearched() : 0.0);
        return 0;
    }
    if (mode == "drain") {   // O6-03: the flood round the first land sites of a type (default felisian): the tile's cost, its channels, lakes and monotony, and the sample cost with and without it
        int type = PT_FELISIAN;
        if (argc > 2) for (int k = 0; k < PT_COUNT; k++) { std::string nm = PLANET_TYPES[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) type = k; }
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
    if (mode == "water") {   // B-320: the inland water of the first felisian worlds at 16 m detail: rivers, lakes, pools per thousand samples, the first of each
        int wanted = argc > 2 ? atoi(argv[2]) : 3, shown = 0;
        for (int64_t x = 150; x < 320 && shown < wanted; x++)
            for (int64_t z = 20; z < 140 && shown < wanted; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && shown < wanted; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != PT_FELISIAN) continue;
                    BodyGen g = BodyGen::make(b);
                    long n = 0, land = 0, kinds[5] = {0, 0, 0, 0, 0}, wet[5] = {0, 0, 0, 0, 0};
                    double firstLat[5] = {0, 0, 0, 0, 0}, firstLon[5] = {0, 0, 0, 0, 0}; bool have[5] = {false, false, false, false, false};
                    // O6-03: a window of 1.2 x 1.2 degrees round the first land site of the body, at 16 m (the planet's drainage would
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
    if (mode == "climate") {   // B-321: the biome shares of the first felisian worlds, area-weighted, and why the ice is ice
        int wanted = argc > 2 ? atoi(argv[2]) : 24, shown = 0;
        int totBio[BIO_COUNT] = {0}; long totN = 0; int iceT = 0, iceCap = 0, iceSnow = 0;
        Rng r(0xC11A);
        for (int64_t x = 150; x < 320 && shown < wanted; x++)
            for (int64_t z = 20; z < 140 && shown < wanted; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && shown < wanted; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != PT_FELISIAN) continue;
                    BodyGen g = BodyGen::make(b);
                    setDrainageEnabled(false);   // O6-03: 1500 random points over the globe would build a tile each; the biome shares do not need the rivers
                    int bio[BIO_COUNT] = {0}; int n = 0, land = 0, inland = 0, lakes = 0;
                    for (int k = 0; k < 1500; k++) {
                        double lat = std::asin(r.range(-1, 1)), lon = r.range(-PI, PI);
                        SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), 400);
                        n++; bio[ss.biome]++; if (ss.height > 0) land++;
                        if (ss.material == MAT_WATER && ss.water > 0.5) { inland++; if (ss.biome == BIO_WETLAND) lakes++; }
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
    if (mode == "compare") { renderCompare(); return 0; }
    if (mode == "sheet") { composeSheets(); return 0; }
    if (mode == "moonsky") { renderMoonSky(); { SpaceRenderer sr2; StarNeighborhood nb2; renderBinarySurface(sr2, nb2); } return 0; }
    if (mode == "bench") return argc > 2 && std::string(argv[2]) == "forest" ? runBenchForest() : runBench(argc > 2 && std::string(argv[2]) == "check");
    if (mode == "flow") { runGameFlow(); return 0; }
    if (mode == "landmaps") { renderLandingMaps(); return 0; }
    if (mode == "consistency") { runConsistency(); return 0; }
    if (mode == "bestiary") return runBestiary();
    if (mode == "drive") return runDrive();
    if (mode == "descent") return runDescent(argc > 3 ? atof(argv[2]) : PIN_MOUNTAIN_LAT, argc > 3 ? atof(argv[3]) : PIN_MOUNTAIN_LON);   // B-307; G-04: `descent [latDeg lonDeg]`
    if (mode == "stability") { if (argc > 4) setFramebufferScale(atoi(argv[4])); return runStability(argc > 2 ? argv[2] : nullptr, argc > 3 ? atof(argv[3]) : 0.0); }   // B-309; B-313: `stability <scene> [metres per frame] [scale]`
    if (mode == "encounters") { runEncounters(argc > 2 ? atoi(argv[2]) : 300); return 0; }
    if (mode == "settings") { testSettings(); return 0; }
    if (mode == "regress") return runRegress(argc > 2 && std::string(argv[2]) == "bless");
    if (mode == "fuzz") return runFuzz(argc > 2 ? atoi(argv[2]) : 10000);
    if (mode == "reanchor") { testReanchor(); return 0; }
    if (mode == "survey") { runSurvey(argc > 2 ? atoi(argv[2]) : 2000); return 0; }
    if (mode == "galaxy") { runGalaxy(); return 0; }   // G-01
    if (mode == "home") { runHome(); return 0; }   // G-02
    if (mode == "audio") { testAudio(); return 0; }
    if (mode == "seasons") { testSeasons(); return 0; }
    if (mode == "input") return testInput();
    if (mode == "unit") return testUnit();
    if (mode == "terrain") { if (argc > 2) g_terrainType = atoi(argv[2]); return runTerrainStats(); }
    if (mode == "landforms") {   // O6-01: `landforms sites` (the fixed review sites) or `landforms <type> [flat|hills|mountains]`
        std::string a = argc > 2 ? argv[2] : "3";
        if (a == "sites") { renderLandformSites(argc > 3 && std::string(argv[3]) == "search"); return 0; }
        int cls = -1;
        if (argc > 3) { std::string c = argv[3]; cls = c == "flat" ? 0 : (c == "hills" ? 1 : (c == "mountains" ? 2 : -1)); }
        renderLandforms(atoi(a.c_str()), cls);
        return 0;
    }
    if (mode == "gallery") { renderGallery(argc > 2 ? atoi(argv[2]) : PT_FELISIAN); return 0; }
    if (mode == "traits") { renderTraits(argc > 2 ? argv[2] : nullptr); return 0; }   // R-304/R-305
    if (mode == "ruins") return runRuins(argc, argv);   // C-01
    if (mode == "music") return runMusic(argc, argv);   // C-04
    if (mode == "voice") return runVoice(argc, argv);   // C-05
    if (mode == "signals") return runSignals(argc, argv);   // C-07
    if (mode == "shards") return runShards(argc, argv);   // C-02
    if (mode == "surface") { renderSurface(argc > 2 ? atoi(argv[2]) : -1); return 0; }
    if (mode == "stars") testStars();
    else if (mode == "maps") renderMaps();
    else if (mode == "space") renderSpace();
    else printf("unknown mode\n");
    return 0;
}
