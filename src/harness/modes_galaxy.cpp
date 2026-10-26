// E-03: the galaxy's modes: `stars`, `galaxy`, `chart`, `stability flight`, `crossing`, `home`, `survey`
#include "harness/harness.h"

static bool galaxySkyFrames(double coreLon);   // G-03, defined with the space renderer below

void testStars() {
    int counts[STAR_DRAWN_COUNT] = {0};
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
    for (int i = 0; i < STAR_DRAWN_COUNT; i++) printf("  %s %-14s %d\n", STAR_CLASSES[i].code, STAR_CLASSES[i].name, counts[i]);
    int typeCounts[PT_COUNT] = {0};
    int moons = 0, planets = 0, systems = 0;
    for (auto& [x, z, s, sys] : scanSystems(150, 190, 20, 60)) {
        systems++;
        for (auto& b : sys.bodies) { typeCounts[b.type]++; if (b.parent < 0) planets++; else moons++; }
    }
    printf("Systems: %d planets: %d moons: %d\n", systems, planets, moons);
    for (int i = 0; i < PT_COUNT; i++) printf("  %-16s %d\n", KINDS[i].name, typeCounts[i]);
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
                   b.name.c_str(), KINDS[b.type].name, b.radiusKm, b.orbitRadiusKm, b.orbitPeriod / 3600,
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
void runGalaxy() {
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
    // the clusters: every globular of the galaxy (the cells within 60,000 ly of the centre), the open clusters within 1,500 ly of home
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
        int sectors = 0, stars = 0, cls[STAR_DRAWN_COUNT] = {0};
        int64_t x0 = (int64_t)std::floor(px), y0 = (int64_t)std::floor(py), z0 = (int64_t)std::floor(pz);
        for (int64_t x = x0 - 7; x <= x0 + 7; x++) for (int64_t y = y0 - 7; y <= y0 + 7; y++) for (int64_t z = z0 - 7; z <= z0 + 7; z++) {
            if (galaxyRegion(x, y, z) != want) continue;
            sectors++;
            Star st; if (!starInSector(x, y, z, st, false)) continue;
            stars++; cls[st.cls]++;
        }
        printf("  %-19s at %6.0f %5.0f %6.0f: %4d of %4d sectors hold a star (%4.1f%%)", label, px, py, pz, stars, sectors, sectors ? 100.0 * stars / sectors : 0.0);
        for (int i = 0; i < STAR_DRAWN_COUNT; i++) printf("  %s %2.0f%%", STAR_CLASSES[i].code, stars ? 100.0 * cls[i] / stars : 0.0);
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
    int stars = 0, cls[STAR_DRAWN_COUNT] = {0}, reg[REGION_COUNT] = {0};
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
    for (int i = 0; i < STAR_DRAWN_COUNT; i++) printf(" %s %d", STAR_CLASSES[i].code, cls[i]);
    printf("; regions:"); for (int k = 0; k < REGION_COUNT; k++) if (reg[k]) printf(" %s %d", REGION_NAMES[k], reg[k]); printf("\n");
    {   // the pinned home star, and the search's own pick (the fallback)
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
    {   // the star-forming complexes within 300 ly of home, and the nearest
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
    {   // the band from home: the columns of starlight and the dust in the named directions, the map's cost, its picture and the sky from the ship
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
        printf("  along the plane from the core:"); for (int k = 0; k < 12; k++) { double lon = coreLon + k * 30 * DEG; printf(" %d:%.2f", k * 30, sampleGalaxyBand(band, Vec3(std::cos(lon), 0, std::sin(lon)))); } printf("\n");
        for (int a = 0; a < 2; a++) {
            double lon = coreLon + a * PI; printf("  by latitude at the %s:", a ? "anticentre" : "core");
            for (double lat : {0.0, 1.0, 2.0, 3.0, 5.0, 8.0, 12.0, 20.0, 30.0, 60.0, 90.0}) printf(" %.0f:%.2f", lat, sampleGalaxyBand(band, Vec3(std::cos(lat * DEG) * std::cos(lon), std::sin(lat * DEG), std::cos(lat * DEG) * std::sin(lon))));
            printf("\n");
        }
        {   // the whole sky as the game samples it, 720 x 360, the core at the centre
            const int BW = 720, BH = 360; std::vector<uint32_t> bimg((size_t)BW * BH);
            for (int j = 0; j < BH; j++)
                for (int i = 0; i < BW; i++) {
                    double lon = coreLon + ((i + 0.5) / BW - 0.5) * TAU, lat = (0.5 - (j + 0.5) / BH) * PI;
                    int g = clampi((int)(255 * sampleGalaxyBand(band, Vec3(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon)))), 0, 255);
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
        for (const Knot& g : globs) {   // the globulars as orange dots
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

// G-03: the sky from a ship above the home star, toward the core, along the arm and toward the anticentre (`galaxy`)
static bool galaxySkyFrames(double coreLon) {
    Star hs; StarSystem hsys; if (!systemAt(HOME_SX, HOME_SY, HOME_SZ, hs, hsys)) return false;
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

// ---- the galaxy chart (G-05) ----------------------------------------------------------------------------------------------------
// `chart`: the star map's galaxy page at every level from three places (home; 2,000 ly over home's plane; a star of the core), a frame
// a level (`shots/tests/chart_<place>_<level>.png`) with the level's build time and where the ship's crosshair was drawn; first the
// closed-form column against a numerical integral at a few places (the face's light comes from it, a column a pixel)
int runChart() {
    printf("chart: the closed-form face-on column (galaxyFaceOnColumn) against the field integrated in 2 ly steps\n");
    struct P { const char* name; double x, z; } pts[] = {{"home", HOME_SX + 0.5, HOME_SZ + 0.5}, {"the centre", GALAXY_CENTRE_SX, GALAXY_CENTRE_SZ},
        {"500 ly out", GALAXY_CENTRE_SX + 500, GALAXY_CENTRE_SZ}, {"1,500 ly out", GALAXY_CENTRE_SX + 1500, GALAXY_CENTRE_SZ}, {"3,000 ly out", GALAXY_CENTRE_SX, GALAXY_CENTRE_SZ + 3000},
        {"25,000 ly out", GALAXY_CENTRE_SX - 25000, GALAXY_CENTRE_SZ}, {"45,000 ly out", GALAXY_CENTRE_SX, GALAXY_CENTRE_SZ - 45000}};
    double worst = 0;
    for (const P& p : pts) {
        const FaceOnColumn c = galaxyFaceOnColumn(p.x, p.z);
        double n = 0; for (double y = -12000 + 1; y < 12000; y += 2) n += galaxyFieldDensity(p.x, y, p.z) * 2;
        worst = std::max(worst, std::fabs(c.column - n) / n);
        printf("  %-14s closed %9.3f  numeric %9.3f  (%+.4f%%)  shares: disc %.2f, arm %.2f, bulge %.2f\n", p.name, c.column, n, 100 * (c.column - n) / n, c.disk, c.arm, c.bulge);
    }
    {
        const double t0 = nowSec(); double acc = 0; int n = 0;
        for (int j = 0; j < 128; j++) for (int i = 0; i < 320; i++, n++) acc += galaxyFaceOnColumn(GALAXY_CENTRE_SX + (i - 160) * 300.0, GALAXY_CENTRE_SZ + (j - 64) * 300.0).column;
        printf("  worst difference %.4f%%; %d columns in %.2f ms on one thread (%.0f ns each)%s\n", 100 * worst, n, (nowSec() - t0) * 1e3, (nowSec() - t0) * 1e9 / n, acc > 0 ? "" : " (none?)");
    }
    TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
    game.newGame(); game.setState(GameState::SPACE);
    Input in;
    auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
    auto press = [&](int key) { pressKey(game, in, key); };
    run(0.3);
    press(KEY_M); press(KEY_TAB); run(0.1);
    printf("the star map's page after M and Tab: %d (1 is the galaxy)\n", game.testMapPage());
    const Vec3 home = game.testShipPos();
    auto places = [&](const char* tag) {
        const Vec3 sp = game.testShipPos() / SECTOR_KM;
        printf("%s: the ship at sector %.0f %.0f %.0f\n", tag, sp.x, sp.y, sp.z);
        for (int L = 0; L < Game::CHART_LEVELS; L++) {
            game.testChartLevel(L); game.testChartFollow();
            game.frame(in, 1.0 / 30); in.newFrame();
            const double built = game.testChartBuildMs();
            const double t0 = nowSec(); game.frame(in, 1.0 / 30); in.newFrame(); const double again = (nowSec() - t0) * 1e3;   // the view unchanged: no build
            const double s = Game::chartScale(L);
            const double ex = UW / 2 + (sp.x - (L == 0 ? GALAXY_CENTRE_SX : sp.x)) / s, ey = Game::CHART_FACE_Y0 + Game::CHART_FACE_H / 2 + (sp.z - (L == 0 ? GALAXY_CENTRE_SZ : sp.z)) / s;
            const std::string path = std::string("shots/tests/chart_") + tag + "_" + std::to_string(L) + ".png";
            writePNG(path.c_str(), game.output(), FBW, FBH);
            printf("  level %d: %7s ly tall, %6.1f ly a pixel; built in %5.2f ms, a frame after it %5.2f ms; the crosshair at %d %d (its place %.1f %.1f)\n", L,
                   fmt("%.0f", Game::CHART_TALL_LY / std::ldexp(1.0, L)).c_str(), s, built, again, game.testChartYouX(), game.testChartYouY(), ex, ey);
        }
    };
    places("home");
    game.testPlaceShip(home + Vec3(0, 2000 * SECTOR_KM, 0));
    places("north");
    Star core;
    if (nearestStarTo(Vec3((GALAXY_CENTRE_SX + 300) * SECTOR_KM, 0.5 * SECTOR_KM, GALAXY_CENTRE_SZ * SECTOR_KM), 10, core)) {
        Star full; starInSector(core.sx, core.sy, core.sz, full, true);
        game.testArriveAt(full); run(0.1);
        places("core");
    } else printf("no star found by the core\n");
    printf("frames: shots/tests/chart_home_<level>.png, chart_north_<level>.png, chart_core_<level>.png\n");
    {   // the next jump chosen on the chart, from home: the cursor 5,000 ly coreward in the plane, plotted (Enter), the look round its
        // end (Tab), flown (Tab back, Enter) and the chart again at the arrival; a mark named over the disc (L); the stops (N); into the halo
        TestGame g; g.savePrefix = "shots/tests/test_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt"; g.guidePath = "shots/tests/test_guide.txt";
        g.newGame(); g.setState(GameState::SPACE);
        Input gi;
        auto grun = [&](double secs) { int n = std::max(1, (int)(secs * 30)); for (int i = 0; i < n; i++) { g.frame(gi, 1.0 / 30); gi.newFrame(); } };
        auto gpress = [&](int key) { gi.pressed[key] = true; gi.down[key] = true; g.frame(gi, 1.0 / 30); gi.newFrame(); gi.down[key] = false; };
        auto gshot = [&](const char* name) { writePNG((std::string("shots/tests/chart_g06_") + name + ".png").c_str(), g.output(), FBW, FBH); };
        grun(0.3);
        gpress(KEY_M); if (g.testMapPage() != 1) gpress(KEY_TAB);
        g.testChartLevel(2); grun(0.05);
        const Vec3 h = g.testShipPos() / SECTOR_KM;
        const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z)), aim = Vec3(h.x, 0, h.z) + toCore * 5000;
        g.testChartCursor(aim); grun(0.05); gshot("cursor");
        printf("G-06: the cursor at %.0f %.0f %.0f, drawn at %d %d\n", aim.x, aim.y, aim.z, g.testChartCurX(), g.testChartCurY());
        double t0 = nowSec(); gpress(KEY_ENTER); const double plotMs = (nowSec() - t0) * 1e3; grun(0.05); gshot("plotted");
        int64_t rx = 0, ry = 0, rz = 0; const bool remote = g.testRemote(rx, ry, rz);
        Star dest; starInSector(rx, ry, rz, dest, false);
        printf("  Enter: plotted %d (a frame with the plot %.1f ms), the target %d at %lld %lld %lld, %.1f ly from the cursor's point, '%s'\n", g.testChartPlotted(), plotMs, remote, (long long)rx, (long long)ry, (long long)rz, length(dest.pos / SECTOR_KM - aim), g.testStatus().c_str());
        t0 = nowSec(); gpress(KEY_TAB); const double lookMs = (nowSec() - t0) * 1e3; grun(0.1); gshot("look");
        printf("  Tab: the look %d, page %d, %zu stars round the plotted end (a frame with the look %.1f ms)\n", g.testChartLook(), g.testMapPage(), g.testLookStars().stars.size(), lookMs);
        gpress(KEY_TAB); grun(0.05);
        printf("  Tab back: page %d, still plotted %d\n", g.testMapPage(), g.testChartPlotted());
        gpress(KEY_ENTER); const double dur = g.testFlightDur(); grun(1.0); gshot("flight_space");
        printf("  Enter: state %d, the flight %.1f s (vimanaSeconds(5000) %.1f), '%s'\n", (int)g.state, dur, vimanaSeconds(5000), g.testStatus().c_str());
        gpress(KEY_M); grun(0.1); gshot("flight"); gpress(KEY_ESCAPE);
        grun(dur + 1.0);
        const Vec3 at = g.testShipPos() / SECTOR_KM;
        printf("  arrived: %d at %.0f %.0f %.0f, %.1f ly from the cursor's point; the log: %s\n", g.testSystem().valid, at.x, at.y, at.z, length(at - aim), g.guide.log.empty() ? "-" : g.guide.log[g.guide.log.size() >= 2 ? g.guide.log.size() - 2 : 0].text.c_str());
        gpress(KEY_M); grun(0.1); gshot("arrived");
        printf("  the chart at the arrival: page %d, the crosshair at %d %d, the cursor on the ship %d\n", g.testMapPage(), g.testChartYouX(), g.testChartYouY(), g.testChartOnShip());
        // a mark over the disc, 1,500 ly north of the ship
        g.testChartCursor(at + Vec3(0, 1500, 0)); grun(0.05);
        gpress(KEY_L); g.testTypeText("THE HIGH VOID"); grun(0.1); gshot("mark");
        printf("  L: %zu marks of the galaxy, the last '%s' at %.0f %.0f %.0f; '%s'\n", g.guide.galaxyMarks.size(), g.guide.galaxyMarks.empty() ? "-" : g.guide.galaxyMarks.back().name.c_str(),
               g.guide.galaxyMarks.empty() ? 0.0 : g.guide.galaxyMarks.back().x, g.guide.galaxyMarks.empty() ? 0.0 : g.guide.galaxyMarks.back().y, g.guide.galaxyMarks.empty() ? 0.0 : g.guide.galaxyMarks.back().z, g.testStatus().c_str());
        g.testChartLevel(0); grun(0.05);
        for (int k = 0; k < 4; k++) { gpress(KEY_N); grun(0.05); const Vec3 c = g.testChartCursorAt(); printf("  N: '%s' (the cursor at %.0f %.0f %.0f)\n", g.testStatus().c_str(), c.x, c.y, c.z); }
        gshot("stops");
        // into the halo: 20,000 ly over the plane
        g.testChartLevel(3); g.testChartCursor(at + Vec3(0, 20000, 0)); grun(0.05); gshot("halo_cursor");
        gpress(KEY_ENTER); grun(0.05); gshot("halo_plotted");
        g.testRemote(rx, ry, rz);
        printf("  into the halo: plotted %d, %.0f ly short, the star at height %lld\n", g.testChartPlotted(), g.testJumpShort(), (long long)ry);
        gpress(KEY_ESCAPE);
        printf("frames: shots/tests/chart_g06_*.png\n");
    }
    return 0;
}

// G-07: a Vimana flight drawn by a SpaceRenderer alone (no cabin, no HUD), as the game draws it: the flight's line and clock in
// the context (the band's keyframes), the ship at `vimanaProgress` along the line, the view along the flight or back along it,
// the neighbourhood's stars streaking when `stars`; the frame then goes through the mush and to RGB as the game's does
struct BareFlight {
    SpaceRenderer sr; StarNeighborhood nb; Framebuffer fb;
    Vec3 from, to; double dur = 0;
    std::vector<uint32_t> px;
    BareFlight(const Vec3& fromKm, const Vec3& dir, double ly) : from(fromKm), to(fromKm + dir * (ly * SECTOR_KM)), dur(vimanaSeconds(ly)), px((size_t)FBW * FBH) {
        SpaceContext c; c.shipPos = from; c.cam = cameraBasis(0, 0);   // the band the ship sees before it flies: the first keyframe
        sr.setupPalette(fb, nullptr, -1, -1, 1.0); sr.render(fb, c);
    }
    Vec3 at(double t) const { return lerp(from, to, vimanaProgress(t / dur)); }
    // the view: 0 ahead, 1 back, 2 aside (toward the galactic centre square to the line, or along the disc when the line runs at it)
    static Vec3 aside(const Vec3& fromSectors, const Vec3& fwd) {
        const Vec3 core = Vec3(GALAXY_CENTRE_SX - fromSectors.x, 0, GALAXY_CENTRE_SZ - fromSectors.z);
        const Vec3 side = core - fwd * dot(core, fwd);
        return normalize(length(side) < 0.1 * length(core) ? cross(fwd, Vec3(0, 1, 0)) : side);
    }
    Vec3 lookOf(int view) const {
        const Vec3 fwd = normalize(to - from);
        return view == 0 ? fwd : (view == 1 ? fwd * -1.0 : aside(from / SECTOR_KM, fwd));
    }
    void frame(double t, int view, bool stars, bool nebulae = true) {
        SpaceContext c;
        c.vimana = true; c.flightFrom = from; c.flightTo = to; c.flightDur = dur; c.flightT = t; c.nebulae = nebulae;
        const double u = clampd(t / dur, 0, 1);
        c.shipPos = at(t);
        const Vec3 fwd = normalize(to - from), look = lookOf(view);
        c.cam = cameraBasis(std::atan2(look.x, look.z), std::asin(clampd(look.y, -1, 1)));
        c.vimanaSpeed = std::sin(u * PI); c.vimanaDirView = c.cam * fwd;
        if (stars) { nb.update(c.shipPos); c.stars = &nb.stars; }
        sr.setupPalette(fb, nullptr, -1, -1, 1.0);
        sr.render(fb, c);
        fb.mush(2); fb.toRGB(px.data(), 1.0);
    }
};

// G-07: `stability flight [scale]`: the band in the two flights of `crossing` (10,000 ly coreward and 3,000 ly north from home), every
// frame a thirtieth of a second from the start to the arrival, the view ahead, back and aside, the stars left out (their streaks move by
// design). The band alone (the star-forming regions' patches left out too): the mean absolute change of the luminance between
// consecutive frames, the worst frame's, the worst pixel's and the share of pixels changing by more than 6 levels (steady: no pixel
// over 6, a shade of the band's ramp); then with the patches: the passages through the star-forming regions (runs of frames whose
// mean changes by more than one level), the longest and the worst frame. A sheet of the band at the middle of the flight beside the
// heat map of the 16 frames round it (`shots/tests/stability_flight_<core|north>_<ahead|back|aside>.png`)
int runFlightStability() {
    Star homeStar; starInSector(HOME_SX, HOME_SY, HOME_SZ, homeStar, true);
    const Vec3 h = homeStar.pos / SECTOR_KM;
    const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z));
    struct Fl { const char* tag; double ly; Vec3 dir; } fl[2] = {{"core", 10000, toCore}, {"north", 3000, Vec3(0, 1, 0)}};
    static const char* const VIEWS[3] = {"ahead", "back", "aside"};
    auto lum = [](uint32_t c) { return 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255); };
    int bad = 0;
    for (const Fl& f : fl)
        for (int view = 0; view < 3; view++) {
            double bandMean = 0, bandWorst = 0, bandWorstAt = 0, bandPix = 0, bandShare = 0, nebWorst = 0, nebLongest = 0; int passages = 0;
            size_t keys = 0; long built = 0, forced = 0; std::string fn;
            const double t0 = nowSec();
            for (int pass = 0; pass < 2; pass++) {
                BareFlight F(homeStar.pos, f.dir, f.ly);
                const int frames = (int)std::ceil(F.dur * 30);
                std::vector<uint32_t> prev, mid;
                std::vector<double> heat((size_t)FBW * FBH, 0);
                double sumMean = 0, worstMean = 0, worstAt = 0, worstPix = 0, runStart = -1; long jumps = 0, n = 0;
                for (int i = 0; i <= frames; i++) {
                    const double t = std::min(F.dur, i / 30.0);
                    F.frame(t, view, false, pass == 1);
                    if (!prev.empty()) {
                        double sum = 0, mx = 0; long j = 0;
                        const bool inWin = std::fabs(t - 0.5 * F.dur) < 8.5 / 30;
                        for (size_t p = 0; p < F.px.size(); p++) {
                            const double d = std::fabs(lum(F.px[p]) - lum(prev[p]));
                            sum += d; mx = std::max(mx, d); if (d > 6) j++;
                            if (inWin) heat[p] += d;
                        }
                        sum /= F.px.size(); sumMean += sum; n++; jumps += j;
                        if (sum > worstMean) { worstMean = sum; worstAt = t; }
                        worstPix = std::max(worstPix, mx);
                        if (sum > 1) { if (runStart < 0) { runStart = t; passages += pass; } nebLongest = std::max(nebLongest, pass ? t - runStart : 0.0); } else runStart = -1;
                    }
                    if (std::fabs(t - 0.5 * F.dur) < 0.5 / 30) mid = F.px;
                    prev = F.px;
                }
                if (pass == 0) {
                    bandMean = n ? sumMean / n : 0; bandWorst = worstMean; bandWorstAt = worstAt; bandPix = worstPix; bandShare = 100.0 * jumps / ((double)n * F.px.size());
                    keys = F.sr.fband.tau.size(); built = F.sr.fband.built; forced = F.sr.fband.forced;
                    std::vector<uint32_t> sheet((size_t)FBW * 2 * FBH);
                    for (int y = 0; y < FBH; y++)
                        for (int x = 0; x < FBW; x++) {
                            sheet[(size_t)y * FBW * 2 + x] = mid.empty() ? 0 : mid[(size_t)y * FBW + x];
                            const int v = (int)std::min(255.0, heat[(size_t)y * FBW + x] / 16 * 40.0);
                            sheet[(size_t)y * FBW * 2 + FBW + x] = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
                        }
                    fn = fmt("shots/tests/stability_flight_%s_%s.png", f.tag, VIEWS[view]);
                    writePNG(fn.c_str(), sheet.data(), FBW * 2, FBH);
                } else nebWorst = worstMean;
            }
            const bool ok = bandPix <= 6;
            if (!ok) bad++;
            printf("stability flight %-5s %-5s %.1f s: the band mean |dL| %.3f a frame (worst %.3f at %.1f s), worst pixel %.0f, %.4f%% change over 6; %zu keyframes, %ld built, %ld at once -> %s (%s); "
                   "with the star-forming regions %d passages, the longest %.2f s, the worst frame %.2f; %.1f s\n",
                   f.tag, VIEWS[view], vimanaSeconds(f.ly), bandMean, bandWorst, bandWorstAt, bandPix, bandShare, keys, built, forced, fn.c_str(), ok ? "steady" : "FLICKERS", passages, nebLongest, nebWorst, nowSec() - t0);
        }
    return bad;
}

// G-07: `crossing`: the galaxy round the ship in two long flights from home, R-407's jump 10,000 ly coreward and 3,000 ly north:
// frames at shares of each flight through the front window and aside (the head turned in the cabin toward the galactic centre square
// to the line: a jump north leaves the disc below it), the galaxy page in the flight, the band's keyframes (how many, how far apart, built at once, the cells a
// frame), the arrival's band (handed over, not built), and a flight frame's time at 2x; `shots/tests/crossing_*.png`
int runCrossing() {
    TestGame g; g.savePrefix = "shots/tests/crossing_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt"; g.guidePath = "shots/tests/crossing_guide.txt";
    g.newGame(); g.setState(GameState::SPACE);
    Input in;
    auto run = [&](double secs) { int n = std::max(1, (int)std::lround(secs * 60)); for (int i = 0; i < n; i++) { g.frame(in, 1.0 / 60); in.newFrame(); } };
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; g.frame(in, 1.0 / 60); in.newFrame(); in.down[key] = false; };
    auto shot = [&](const std::string& name) { writePNG(("shots/tests/crossing_" + name + ".png").c_str(), g.output(), FBW, FBH); };
    run(0.5);
    Star homeStar; starInSector(HOME_SX, HOME_SY, HOME_SZ, homeStar, true);
    const Vec3 h = g.testShipPos() / SECTOR_KM;
    const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z));
    struct Fl { const char* tag; double ly; Vec3 dir; } fl[2] = {{"core", 10000, toCore}, {"north", 3000, Vec3(0, 1, 0)}};
    for (int f = 0; f < 2; f++) {
        if (f > 0) { g.testArriveAt(homeStar); g.setState(GameState::SPACE); run(0.5); }
        const long parked0 = g.testParkedBandBuilds();
        g.testStartJump(fl[f].ly, fl[f].dir);
        if (!g.testFlying()) { printf("%s: no flight ('%s')\n", fl[f].tag, g.testStatus().c_str()); continue; }
        const double dur = g.testFlightDur();
        int64_t rx = 0, ry = 0, rz = 0; g.testRemote(rx, ry, rz);
        printf("%s: %.0f ly %s from home to %lld %lld %lld (%s), %.1f s\n", fl[f].tag, fl[f].ly, galacticHeading(h, fl[f].dir).c_str(), (long long)rx, (long long)ry, (long long)rz,
               REGION_NAMES[galaxyRegion(rx, ry, rz)], dur);
        const double shares[] = {0.02, 0.25, 0.5, 0.75, 0.98};
        for (double s : shares) {
            const double at = s * dur;
            if (at - 0.5 > g.testFlightT()) g.testFlightSkip(at - 0.5);
            g.testCabinTurn(0, 0); run(0.5);
            const std::string pct = fmt("%02d", (int)std::lround(s * 100));
            shot(std::string(fl[f].tag) + "_" + pct + "_front");
            const Vec3 p = g.testShipPos() / SECTOR_KM;
            const SpaceRenderer::FlightBand& F = g.testFlightBand();
            printf("  %3.0f%% (%.1f s): at %.0f %.0f %.0f, %s, %.0f ly from the core; keyframe %d of %zu (%.2f s apart), %d cells this frame\n", s * 100, g.testFlightT(), p.x, p.y, p.z,
                   REGION_NAMES[galaxyRegion((int64_t)std::floor(p.x), (int64_t)std::floor(p.y), (int64_t)std::floor(p.z))], length(p - Vec3(GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ)),
                   F.k, F.tau.size(), F.tau[std::min((size_t)F.k + 1, F.tau.size() - 1)] - F.tau[F.k], F.cellsFrame);
            if (s == 0.25 || s == 0.5 || s == 0.98) { g.testCabinLook(BareFlight::aside(h, fl[f].dir)); run(0.25); shot(std::string(fl[f].tag) + "_" + pct + "_aside"); g.testCabinTurn(0, 0); }
            if (s == 0.5) {
                press(KEY_M); run(0.1); shot(std::string(fl[f].tag) + "_50_chart"); press(KEY_ESCAPE); run(0.1);
                g.settings.renderScale = 2; g.applySettings(); run(0.5);
                double sum = 0, worst = 0;
                for (int i = 0; i < 60; i++) { const double t0 = nowSec(); g.frame(in, 1.0 / 60); in.newFrame(); const double ms = (nowSec() - t0) * 1e3; sum += ms; worst = std::max(worst, ms); }
                printf("  a flight frame at 2x: %.2f ms, worst %.1f ms\n", sum / 60, worst);
                g.settings.renderScale = 4; g.applySettings(); run(0.25);
            }
        }
        const SpaceRenderer::FlightBand F = g.testFlightBand();
        double minDt = 1e9, maxDt = 0;
        for (size_t i = 1; i < F.tau.size(); i++) { minDt = std::min(minDt, F.tau[i] - F.tau[i - 1]); maxDt = std::max(maxDt, F.tau[i] - F.tau[i - 1]); }
        run(dur - g.testFlightT() + 1.0);
        printf("  the keyframes: %zu, %.2f-%.2f s apart; %ld built, %ld at once (the skips of the harness's clock); arrived %d, '%s'; the parked band built %ld times since the start (%s)\n",
               F.tau.size(), minDt, maxDt, F.built, F.forced, g.testSystem().valid, g.testStatus().c_str(), g.testParkedBandBuilds() - parked0,
               g.testParkedBandBuilds() == parked0 ? "the flight's last keyframe handed over" : "BUILT AGAIN");
        shot(std::string(fl[f].tag) + "_arrived");
    }
    // the same lines drawn bare (the renderer alone: no cabin, no HUD), ahead and back, the stars streaking; the luminance of the
    // frame, its middle (a box a fifth of the frame's height) and its top and bottom fifths, 0..255
    auto lum = [](uint32_t c) { return 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255); };
    for (int f = 0; f < 2; f++)
        for (int view = 0; view < 3; view++) {
            static const char* const VIEWS[3] = {"ahead", "back", "aside"};
            BareFlight B(homeStar.pos, fl[f].dir, fl[f].ly);
            printf("bare %s %s:", fl[f].tag, VIEWS[view]);
            for (double s : {0.02, 0.25, 0.5, 0.75, 0.98}) {
                const double at = s * B.dur;
                for (double t = std::max(0.0, at - 0.5); t < at; t += 1.0 / 60) B.frame(t, view, true);
                B.frame(at, view, true);
                writePNG(fmt("shots/tests/crossing_%s_%02d_bare_%s.png", fl[f].tag, (int)std::lround(s * 100), VIEWS[view]).c_str(), B.px.data(), FBW, FBH);
                double all = 0, mid = 0, top = 0, bot = 0; long nm = 0, nt = 0, nb = 0;
                for (int y = 0; y < FBH; y++)
                    for (int x = 0; x < FBW; x++) {
                        const double L = lum(B.px[(size_t)y * FBW + x]); all += L;
                        if (std::abs(x - FBW / 2) < FBH / 10 && std::abs(y - FBH / 2) < FBH / 10) { mid += L; nm++; }
                        if (y < FBH / 5) { top += L; nt++; } else if (y >= FBH * 4 / 5) { bot += L; nb++; }
                    }
                printf("  %2.0f%% %.0f (mid %.0f, top %.0f, bottom %.0f)", s * 100, all / ((double)FBW * FBH), mid / std::max(1L, nm), top / std::max(1L, nt), bot / std::max(1L, nb));
            }
            printf("\n");
        }
    printf("frames: shots/tests/crossing_<core|north>_<share>_<front|aside>.png (the cabin), crossing_<core|north>_<share>_bare_<ahead|back|aside>.png, crossing_<core|north>_50_chart.png, crossing_<core|north>_arrived.png\n");
    return 0;
}
// G-02: `home`: the candidates of the new-game search (yellow or orange stars with a living planet in sectors 176..196 x
// 36..56), each parked at its first living planet with the landing map open as a new game does, timed until the default
// site's drainage tiles are in the cache (the bench's descent budget, KI-338); prints what the pick needs: no nebula
// patches in the sky, tiles under 2.5 s, a temperate world, the score, and the sector to pin in Game::newGame.
void runHome() {
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
            TestGame game;
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

// ---- survey and gallery (M9-01) ------------------------------------------------------------
// survey [N]: distributions over N bodies of the start region; gallery <type>: 16 random sites in one sheet.
// what a parked ship sees of the other worlds. From the game's parking (`testParkAt`'s side of the world, `parkDistanceFor`
// out) at time t: the angular radius in degrees of the biggest other world anywhere in the sky, and of the biggest in the default
// 70-degree view of the world
void parkedNeighbours(const StarSystem& sys, int bi, double t, double& skyDeg, double& viewDeg) {
    const Vec3 bp = sys.bodyPos(bi, t), toStar = normalize(sys.star.pos - bp), side = normalize(cross(toStar, Vec3(0, 1, 0)));
    const Vec3 P = bp + normalize(toStar * 0.7 + side * 0.7 + Vec3(0, 0.2, 0)) * Game::parkDistanceFor(sys.bodies[bi]);
    const Vec3 f = normalize(bp - P), r = normalize(cross(Vec3(0, 1, 0), f)), u = cross(f, r);
    const double tx = std::tan(35 * DEG), ty = tx * 200 / 320;
    skyDeg = viewDeg = 0;
    for (int j = 0; j < (int)sys.bodies.size(); j++) {
        if (j == bi || sys.bodies[j].type == PT_COMPANION) continue;
        const Vec3 rel = sys.bodyPos(j, t) - P;
        const double a = std::asin(clampd(sys.bodies[j].radiusKm / length(rel), 0, 1)) / DEG, z = dot(rel, f), m = std::tan(std::min(a, 80.0) * DEG);
        skyDeg = std::max(skyDeg, a);
        if (z > 0 && std::fabs(dot(rel, r) / z) <= tx + m && std::fabs(dot(rel, u) / z) <= ty + m) viewDeg = std::max(viewDeg, a);
    }
}

void runSurvey(int wanted) {
    int types[PT_COUNT] = {0}, planets = 0, moons = 0, systems = 0, felSystems = 0;
    int madeFrom[PT_COUNT][PT_DRAWN_COUNT] = {{0}}, changed = 0, changedSystems = 0;   // P-01: the made kinds and what they were drawn as
    int rdLocked[PT_COUNT] = {0}, rdLockedN = 0;   // the red dwarfs' locked worlds by kind
    int deadBy[STAR_CLASS_COUNT] = {0}, deadN = 0, deadPeople = 0, giantLiving = 0;   // the scorched worlds by their sun, with a people; a giant's felisian worlds still living
    int tempHist[6] = {0}, gravHist[5] = {0}, moonHist[8] = {0};
    double matCov[PT_COUNT][MAT_COUNT] = {{0}};
    int matN[PT_COUNT] = {0};
    int bodies = 0;
    // what hangs in the sky at random landing sites and times
    int skySites = 0, sky05 = 0, sky1 = 0, sky3 = 0, sky10 = 0, skyPoint = 0, skyParent = 0, skyMoon = 0, skySister = 0, skyOther = 0, skyTwoSuns = 0;
    int multiples = 0, substellar = 0, comets = 0, doubles = 0, lockedN = 0;
    int parkN[5] = {0}, parkSky[5] = {0}, parkView[5] = {0};   // at a solid planet, its moon, a giant, a giant's moon, a twin
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return bodies < wanted; })) {
        systems++;
        if (sys.companion >= 0) multiples++;
        bool fel = false;
        for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
            const Body& b = sys.bodies[bi];
            if (b.type == PT_SUBSTELLAR) substellar++;
            if (b.type == PT_COMET) comets++;
            if (b.doublePlanet && b.parent >= 0) doubles++;
            if (b.locked) lockedN++;
            if (b.type != PT_COMPANION && b.type != PT_COMET) {   // what the parked ship sees of the other worlds, twice a body
                const bool moon = b.parent >= 0 && sys.bodies[b.parent].type != PT_COMPANION;
                const Body& w = moon ? sys.bodies[b.parent] : b;
                const bool gi = w.type == PT_GASGIANT || w.type == PT_SUBSTELLAR;
                const int cat = b.doublePlanet ? 4 : (moon ? (gi ? 3 : 1) : (gi ? 2 : 0));
                Rng pr(b.seed ^ 0xC40DULL);
                for (int k = 0; k < 2; k++) {
                    double sky, view; parkedNeighbours(sys, bi, pr.range(0, 4e6), sky, view);
                    parkN[cat]++; if (sky > 5) parkSky[cat]++; if (view > 3) parkView[cat]++;
                }
            }
            if (!KINDS[b.type].landable) continue;
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
        bool anyChanged = false;
        for (const Body& b : sys.bodies) {
            bodies++;
            types[b.type]++;
            if (b.type != drawnKind(b)) { madeFrom[b.type][drawnKind(b)]++; changed++; anyChanged = true; }
            if (b.locked && sys.star.cls == STAR_RED_DWARF) { rdLocked[b.type]++; rdLockedN++; }
            {   const int sun = b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION ? sys.bodies[b.parent].starClass : sys.star.cls;
                if (b.type == PT_SCORCHED_FELISIAN) { deadN++; if (sun >= 0) deadBy[sun]++; if (peoplesHash(b.type, b.seed)) deadPeople++; }
                if (b.type == PT_FELISIAN && (sun == STAR_RED_GIANT || sun == STAR_ORANGE_GIANT || sun == STAR_CARBON)) giantLiving++; }
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
        if (anyChanged) changedSystems++;
    }
    printf("survey: %d systems, %d bodies (%d planets, %d moons); felisian in %d systems (%.0f%%)\n", systems, bodies, planets, moons, felSystems, 100.0 * felSystems / std::max(1, systems));
    printf("types:"); for (int i = 0; i < PT_COUNT; i++) printf("  %s %.1f%%", KINDS[i].name, 100.0 * types[i] / std::max(1, bodies)); printf("\n");
    // the made kinds: how many bodies changed kind (every draw of their systems made as the drawn kind's), and from what
    printf("made kinds: %d bodies of %d changed kind (%.1f%%), in %d of %d systems\n", changed, bodies, 100.0 * changed / std::max(1, bodies), changedSystems, systems);
    for (int k = PT_DRAWN_COUNT; k < PT_COUNT; k++) {
        printf("  %-12s %4d:", KINDS[k].name, types[k]);
        for (int d = 0; d < PT_DRAWN_COUNT; d++) if (madeFrom[k][d]) printf(" %d from %s", madeFrom[k][d], KINDS[d].name);
        printf("\n");
    }
    {   // what the red dwarfs' locked worlds are, the commonest first
        printf("red dwarfs' locked worlds: %d:", rdLockedN);
        int order[PT_COUNT]; for (int k = 0; k < PT_COUNT; k++) order[k] = k;
        std::sort(order, order + PT_COUNT, [&](int a, int b) { return rdLocked[a] != rdLocked[b] ? rdLocked[a] > rdLocked[b] : a < b; });
        for (int i = 0; i < PT_COUNT && rdLocked[order[i]] > 0; i++) printf(" %s %d", KINDS[order[i]].name, rdLocked[order[i]]);
        printf("\n");
    }
    {   // the dead worlds: the scorched by their sun, those with a people; the giants' felisian worlds left living (C-01's peoples' among them)
        printf("dead worlds: %d scorched (", deadN);
        bool first = true; for (int c = 0; c < STAR_CLASS_COUNT; c++) if (deadBy[c]) { printf("%s%s %d", first ? "" : ", ", STAR_CLASSES[c].name, deadBy[c]); first = false; }
        printf("), %d with a people; a giant's felisian worlds left living %d\n", deadPeople, giantLiving);
    }
    {   // the wanderers over the whole start region (the sample above holds a handful): how many systems caught one, of what kind, how
        // far out past the last planet (its nearest over that orbit), how eccentric, which way round
        int allSys = 0, wanderN = 0, wanderFrozen = 0, wanderRetro = 0, wanderPeople = 0; double wanderEcc = 0, wanderOut = 0;
        for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160)) {
allSys++;
            for (const Body& b : sys.bodies) {
                if (!b.wanderer) continue;
                wanderN++; if (b.type == PT_FROZEN_FELISIAN) wanderFrozen++; if (b.orbitIncl > PI / 2) wanderRetro++; wanderEcc += b.ecc;
                if (BodyGen::make(b).hasTrait(TR_CIVILISATION)) wanderPeople++;
                double last = 0; for (const Body& q : sys.bodies) if (q.parent < 0 && !q.wanderer && q.type != PT_COMET && q.type != PT_COMPANION) last = std::max(last, q.orbitRadiusKm);
                if (last > 0) wanderOut += b.orbitRadiusKm * (1 - b.ecc) / last;
            }
        }
        printf("wanderers: %d in the start region's %d systems (%.1f%%), %d frozen felisian, %d with a people; %d retrograde, eccentricity %.2f on average, the nearest %.2f times the last planet's orbit\n",
               wanderN, allSys, 100.0 * wanderN / std::max(1, allSys), wanderFrozen, wanderPeople, wanderRetro, wanderEcc / std::max(1, wanderN), wanderOut / std::max(1, wanderN));
    }
    {   // the giants' moons over the start region by kind, the commonest first (their captured moons apart), and the strange moons among all moons
        int giantMoons = 0, kinds[PT_COUNT] = {0}, captured = 0, strange[PT_COUNT] = {0}, allMoons = 0;
        for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160)) {
            for (const Body& b : sys.bodies) {
                if (b.parent < 0 || b.type == PT_COMET || sys.bodies[b.parent].type == PT_COMPANION) continue;
                allMoons++; if (b.type >= PT_GROOVED_ICE && b.type <= PT_SPONGE) strange[b.type]++;
                if (!kindHas(drawnKind(sys.bodies[b.parent]), KT_GIANT)) continue;
                if (b.orbitPeriod < 0) { captured++; continue; }
                giantMoons++; kinds[b.type]++;
            }
        }
        int order[PT_COUNT]; for (int k = 0; k < PT_COUNT; k++) order[k] = k;
        std::sort(order, order + PT_COUNT, [&](int a, int b) { return kinds[a] != kinds[b] ? kinds[a] > kinds[b] : a < b; });
        printf("giants' moons: %d in the start region (and %d captured):", giantMoons, captured);
        for (int i = 0; i < PT_COUNT && kinds[order[i]] > 0; i++) printf(" %s %.1f%%", KINDS[order[i]].name, 100.0 * kinds[order[i]] / std::max(1, giantMoons));
        printf("\nstrange moons: of %d moons", allMoons);
        for (int k = PT_GROOVED_ICE; k <= PT_SPONGE; k++) printf("%s %s %d", k > PT_GROOVED_ICE ? "," : "", KINDS[k].name, strange[k]);
        printf("\n");
    }
    {   // the giants over the start region by class against their equilibrium temperature (the game's scale), the giants made from a world
        // (the warm zones'), and the yellow stars with a hot one (an alkali or a silicate giant)
        std::vector<double> temps[GK_COUNT]; int fromWorld = 0, yellow = 0, yellowHot = 0;
        for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160)) {
            bool hot = false;
            for (const Body& b : sys.bodies) {
                if (!isProbeGiant(b.type)) continue;
                const int cls = KINDS[b.type].giantClass;
                const int gk = cls != GC_NONE ? GK_WATER + (cls - GC_WATER) : (kindHas(b.type, KT_GLOWS) ? GK_BROWN : (isIceGiant(b) ? GK_ICE : GK_GAS));
                temps[gk].push_back(b.tempK);
                if (!kindHas(drawnKind(b), KT_GIANT)) fromWorld++;
                if (b.type == PT_ALKALI || b.type == PT_SILICATE) hot = true;
            }
            if (s.cls == STAR_YELLOW) { yellow++; if (hot) yellowHot++; }
        }
        printf("giants by class (equilibrium K, the game's: the least, the median, the most):");
        for (int k = 0; k < GK_COUNT; k++) {
            std::vector<double>& v = temps[k]; std::sort(v.begin(), v.end());
            if (v.empty()) printf("%s %s none", k ? ";" : "", GIANT_KIND_NAMES[k]);
            else printf("%s %s %zu, %.0f/%.0f/%.0f", k ? ";" : "", GIANT_KIND_NAMES[k], v.size(), v.front(), v[v.size() / 2], v.back());
        }
        printf("\n  %d giants made from a world (the warm zones'); %d of %d yellow stars (%.1f%%) hold a hot giant\n", fromWorld, yellowHot, yellow, 100.0 * yellowHot / std::max(1, yellow));
    }
    printf("temperature K: <150 %d, 150-300 %d, 300-450 %d, 450-600 %d, 600-750 %d, >750 %d\n", tempHist[0], tempHist[1], tempHist[2], tempHist[3], tempHist[4], tempHist[5]);
    printf("gravity m/s2: <5 %d, 5-10 %d, 10-15 %d, 15-20 %d, >20 %d\n", gravHist[0], gravHist[1], gravHist[2], gravHist[3], gravHist[4]);
    printf("moons per planet:"); for (int i = 0; i < 8; i++) printf(" %d:%d", i, moonHist[i]); printf("\n");
    printf("M5: multiple systems %d (%.0f%%), substellar objects %d, comets %d, double planets %d, locked planets %d\n", multiples, 100.0 * multiples / std::max(1, systems), substellar, comets, doubles, lockedN);
    if (skySites) {
        printf("sky at %d random sites (R-001 targets in brackets): disc >0.5 deg %.0f%%, >1 deg %.0f%% [45-50], >3 deg %.0f%% [35], >10 deg %.0f%% [12-15]; a planet as a point %.0f%%; two suns up %.0f%%\n",
               skySites, 100.0 * sky05 / skySites, 100.0 * sky1 / skySites, 100.0 * sky3 / skySites, 100.0 * sky10 / skySites, 100.0 * skyPoint / skySites, 100.0 * skyTwoSuns / skySites);
        printf("  the disc is: the parent %.0f%%, an own moon %.0f%%, a sister moon %.0f%%, another world %.0f%%\n", 100.0 * skyParent / std::max(1, sky05), 100.0 * skyMoon / std::max(1, sky05), 100.0 * skySister / std::max(1, sky05), 100.0 * skyOther / std::max(1, sky05));
    }
    {   // from the parking, another world wider than 10 deg anywhere in the sky / wider than 6 deg in the view of the world
        const char* const kinds[5] = {"solid planets", "their moons", "giants", "giants' moons", "twins"};
        int n = 0, sk = 0, vw = 0;
        printf("from the ship parked at a world (B-428), another world wider than 10 deg in the sky / wider than 6 deg in the view:");
        for (int k = 0; k < 5; k++) { printf(" %s %.0f%% / %.0f%% of %d;", kinds[k], 100.0 * parkSky[k] / std::max(1, parkN[k]), 100.0 * parkView[k] / std::max(1, parkN[k]), parkN[k]); n += parkN[k]; sk += parkSky[k]; vw += parkView[k]; }
        printf(" all %.0f%% / %.0f%% of %d\n", 100.0 * sk / std::max(1, n), 100.0 * vw / std::max(1, n), n);
    }
    for (int t = 0; t < PT_COUNT; t++) {
        if (!matN[t]) continue;
        printf("  %-16s", KINDS[t].name);
        for (int m = 0; m < MAT_COUNT; m++) if (matCov[t][m] / matN[t] > 0.005) printf(" %s %.0f%%", MATERIAL_NAMES[m], 100.0 * matCov[t][m] / matN[t]);
        printf("\n");
    }
}
