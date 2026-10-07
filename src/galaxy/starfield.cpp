#include "starfield.h"
#include "core/noise.h"
#include "core/parallel.h"
#include <algorithm>
#include <cmath>
#include "core/rng.h"
#include <cmath>

const StarClassInfo STAR_CLASSES[STAR_CLASS_COUNT] = {
    {"S00", "YELLOW STAR", "medium size, yellow star, suitable for planets having indigenous lifeforms.",
     RGB(1.00f, 0.93f, 0.72f), 7.0e5, 0.25, 1.0, 1.0, 10, 30, 20, 8.0e6},
    {"S01", "ORANGE DWARF", "small, warm, long-lived orange star; compact systems, temperate worlds close in.",
     RGB(1.00f, 0.72f, 0.42f), 4.6e5, 0.25, 0.35, 0.7, 8, 28, 16, 5.0e6},
    {"S02", "BLUE GIANT", "very large, blue giant star, high energy radiations around; wide systems.",
     RGB(0.66f, 0.78f, 1.00f), 4.5e6, 0.35, 60.0, 12.0, 16, 8, 14, 5.0e7},
    {"S03", "RED GIANT", "very large, ancient, red giant star; inner planets consumed long ago.",
     RGB(1.00f, 0.45f, 0.25f), 3.5e7, 0.40, 22.0, 1.4, 6, 12, 6, 2.0e8},
    {"S04", "WHITE DWARF", "white dwarf star, tiny and dense remnant; cold, dead worlds; possible harmful radiations.",
     RGB(0.86f, 0.90f, 1.00f), 9.0e3, 0.30, 0.02, 0.8, 4, 14, 1, 6.0e6},
    {"S05", "PULSAR", "tiny pulsar object, unsafe, high radiation, strong gravity; pulsing light.",
     RGB(0.62f, 0.62f, 1.00f), 2.0e4, 0.20, 0.05, 1.6, 3, 8, 1, 1.0e7},
    // S-01: the varieties, rarity 0 (each takes a share of its family through starVariety, not a weight of its own)
    {"S06", "RED DWARF", "small, dim red star, the commonest kind; near worlds keep one face to it; flares.",
     RGB(1.00f, 0.50f, 0.32f), 2.6e5, 0.30, 0.06, 0.4, 6, 0, 12, 3.0e6},
    {"S07", "BLUE-WHITE STAR", "hot, bright, short-lived star; wide systems of bare rock and ice; hard ultraviolet.",
     RGB(0.82f, 0.87f, 1.00f), 1.3e6, 0.30, 8.0, 2.0, 12, 0, 16, 1.6e7},
    {"S08", "ORANGE GIANT", "old star grown large, warm amber light; inner worlds scorched, temperate ones among the ice.",
     RGB(1.00f, 0.66f, 0.34f), 8.0e6, 0.35, 12.0, 1.2, 7, 0, 6, 5.0e7},
    {"S09", "CARBON STAR", "dim ruby giant in its own soot; ember light, a hazy sky; carbon worlds.",
     RGB(1.00f, 0.38f, 0.20f), 3.0e7, 0.40, 9.0, 1.3, 5, 0, 6, 1.8e8},
    // S-03: the neutron star, a share of the pulsar family; its light is a point, its hazard the x-rays, its worlds the survivors
    {"S10", "NEUTRON STAR", "collapsed star, a point of white light; x-ray glare; only survivors: bare cores, captured rocks, worlds of glass.",
     RGB(0.72f, 0.80f, 1.00f), 1.4e4, 0.20, 0.03, 1.5, 3, 0, 1, 1.5e7},
    // S-04: the protostar, a share of the yellow family where stars form; a soft orange star in its cloud, a disc of dust, belts
    {"S11", "PROTOSTAR", "young star still condensing from its cloud; a dusty disc seen edge-on, belts and a few molten worlds; the nebula's glow all over the sky.",
     RGB(1.00f, 0.60f, 0.36f), 2.0e6, 0.30, 1.5, 0.8, 2, 0, 12, 1.5e7},
    // S-05: the Wolf-Rayet star, a share of the blue giants that stayed: blinding inside the shell it has shed, a wind that strips its worlds, radiation everywhere
    {"S12", "WOLF-RAYET STAR", "massive star blowing off its outer layers; blinding blue-white inside a ring of shed gas; a wind that strips atmospheres, radiation everywhere; few bare and bombarded worlds.",
     RGB(0.78f, 0.84f, 1.00f), 3.5e6, 0.35, 180.0, 16.0, 4, 0, 14, 7.0e7},
    // S-06: the black hole, a share of the pulsars that stayed: no light but its disc's (the radius is the stylised Schwarzschild radius the
    // renderer draws from, the mass is the real thing), its worlds the survivors, a companion drawn out
    {"S13", "BLACK HOLE", "collapsed star of no light at all: a hole in the sky ringed by the bent light of the stars behind it, a glowing disc where it feeds; survivors only: wandering rocks, a companion drawn out.",
     RGB(1.00f, 0.88f, 0.70f), 3.0e5, 0.30, 0.03, 40.0, 3, 0, 40, 2.0e7},
};

// G-03: the dust. A thinner disc than the stars' (two fifths of their scale height), denser in the arms, in clouds of
// a few hundred light years (two octaves of noise, mean 1) where `clouds` is asked: the dark rifts of the band are the
// clouds a few hundred light years away, the far dust is smooth. GALAXY_DUST is the extinction per light year at the
// disc's centre in the plane: a thousandth at home, so the optical depth to the core along the plane is about four (its
// glow comes through above and below the rift) and one toward the anticentre.
static double galaxyDust(const GalaxyTerms& g, const Vec3& p, bool clouds) {
    double d = GALAXY_DUST * (g.disk / GALAXY_DISC_PEAK) * std::exp(-1.5 * std::fabs(p.y) / g.h) * (0.3 + 0.7 * g.arm);
    if (clouds) {
        double n = 0.5 + 0.5 * (gnoise3(p.x / 220.0, p.y / 220.0, p.z / 220.0, 0xD057ULL) + 0.5 * gnoise3(p.x / 90.0, p.y / 90.0, p.z / 90.0, 0xD058ULL)) / 1.5;
        d *= clampd(2.4 * n - 0.2, 0.0, 2.2);
    }
    return d;
}

// The stars' density of the field (the disc with its arms, the bulge, the cap), without the clusters: the far part of
// the band's integral, where a cluster is a point anyway.
static inline double fieldDensity(const GalaxyTerms& g) {
    double d = g.disk * (0.55 + 0.6 * g.arm) + g.bulge;
    return d > GALAXY_CAP ? GALAXY_CAP : d;
}

// G-03: the column of starlight along a direction, each star behind the extinction of the dust in front of it, in three
// scales: twelve geometric steps over the first 55 light years (the neighbourhood's knots, the clusters counted), twenty
// to 700 with the dust's clouds (the rifts: the steps stay under the clouds' 90 ly octave), then steps growing by 7% to
// 60,000 with the smooth dust (the disc, the arms, the bulge, the core; steps of 1,000 ly aliased the arms' crests into
// a comb along the band). `tau` returns the optical depth of the whole path.
double galaxyColumn(const Vec3& obs, const Vec3& d, double& tau) {
    double sum = 0; tau = 0;
    double s = 2;
    for (int k = 0; k < 12; k++) {
        double ds = s * 0.3; Vec3 p = obs + d * (s + 0.5 * ds);
        GalaxyTerms g = galaxyTerms(p.x, p.y, p.z);
        sum += galaxyDensity(p.x, p.y, p.z) * std::exp(-tau) * ds; tau += galaxyDust(g, p, true) * ds; s += ds;
    }
    for (int k = 0; k < 20; k++) {
        double ds = s * 0.136; Vec3 p = obs + d * (s + 0.5 * ds);
        GalaxyTerms g = galaxyTerms(p.x, p.y, p.z);
        sum += fieldDensity(g) * std::exp(-tau) * ds; tau += galaxyDust(g, p, true) * ds; s += ds;
    }
    while (s < 60000) {
        double ds = s * 0.07; Vec3 p = obs + d * (s + 0.5 * ds);
        GalaxyTerms g = galaxyTerms(p.x, p.y, p.z);
        sum += fieldDensity(g) * std::exp(-tau) * ds; tau += galaxyDust(g, p, false) * ds; s += ds;
    }
    return sum;
}

// The map's rows are stretched toward the plane: row fraction f (0..1) is the latitude (pi/2) sign(u) |u|^1.5 with
// u = 2f - 1, so the rows beside the plane are 0.4 degrees apart, 2.6 at 30 degrees and 4 at the poles; the band's
// cusp and the rift show, the smooth far sky costs few rows. `bandRowOf` is the inverse, for the sampler.
static inline double bandRowLat(double f) { double u = 2 * f - 1; return 0.5 * PI * (u < 0 ? -1 : 1) * std::pow(std::fabs(u), 1.5); }
static inline double bandRowOf(double lat) { double a = std::fabs(lat) / (0.5 * PI); double u = (lat < 0 ? -1 : 1) * std::cbrt(a * a); return 0.5 * (u + 1); }

// The brightness of a column: a 0.7 power of the column over the exposure, linear below 0.7 and a soft knee to 1.
double bandToneOf(double column) {
    double x = std::pow(column / BAND_EXPOSURE, BAND_GAMMA);
    return x / std::sqrt(std::sqrt(1 + x * x * x * x));
}

void buildGalaxyBand(const Vec3& obs, std::vector<float>& map, int W, int H) {
    map.assign((size_t)W * H, 0.f);
    // the rows in parallel: 8,192 directions of 70 samples each, 10 ms; the landing's site build and the space view both
    // run on the main thread with the pool free (the drainage prefetch has threads of its own)
    parallelFor(H, 2, [&](int jb, int je) {
        for (int j = jb; j < je; j++)
            for (int i = 0; i < W; i++) {
                double lon = (i + 0.5) / W * TAU, lat = bandRowLat((j + 0.5) / H);
                Vec3 d(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon));
                double tau, col = galaxyColumn(obs, d, tau);
                map[(size_t)j * W + i] = (float)bandToneOf(col);
            }
    });
}

double sampleGalaxyBand(const std::vector<float>& map, int W, int H, const Vec3& d) {
    if (map.empty()) return 0;
    double lon = std::atan2(d.z, d.x); if (lon < 0) lon += TAU;
    double lat = std::asin(clampd(d.y, -1, 1));
    double fx = lon / TAU * W - 0.5, fy = bandRowOf(lat) * H - 0.5;
    int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    double tx = fx - x0, ty = fy - y0;
    auto at = [&](int x, int y) { x = ((x % W) + W) % W; y = clampi(y, 0, H - 1); return (double)map[(size_t)y * W + x]; };
    return (at(x0, y0) * (1 - tx) + at(x0 + 1, y0) * tx) * (1 - ty) + (at(x0, y0 + 1) * (1 - tx) + at(x0 + 1, y0 + 1) * tx) * ty;
}

const char* REGION_NAMES[REGION_COUNT] = {"GALACTIC CORE", "THE BULGE", "SPIRAL ARM", "THE DISK", "THE HALO", "GLOBULAR CLUSTER", "STAR-FORMING REGION", "OPEN CLUSTER"};

// G-03: a star-forming complex: one in a NEBULA_CELL column of the arms with probability NEBULA_RATE, its centre inside
// the cell near the plane (the cell test first: it is asked per density call of the region and per frame of the sky).
bool nebulaInCell(int64_t cx, int64_t cz, Vec3& centre) {
    uint64_t h = hash2i(cx, cz, 0x4EBULL);
    if (unitFromHash(h) >= NEBULA_RATE) return false;
    double x = (cx + 0.5) * NEBULA_CELL, z = (cz + 0.5) * NEBULA_CELL;
    if (galaxyTerms(x, 0, z).arm <= 0.45) return false;   // only in the arms
    centre = Vec3(cx * NEBULA_CELL + NEBULA_CELL * (0.3 + 0.4 * unitFromHash(mix64(h + 1))), (unitFromHash(mix64(h + 2)) - 0.5) * 4.0, cz * NEBULA_CELL + NEBULA_CELL * (0.3 + 0.4 * unitFromHash(mix64(h + 3))));
    return true;
}

int nebulaPatches(const Vec3& obs, NebulaPatch* out, int maxN) {
    int n = 0;
    int64_t cx0 = (int64_t)std::floor(obs.x / NEBULA_CELL), cz0 = (int64_t)std::floor(obs.z / NEBULA_CELL);
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++) {
            int64_t cx = cx0 + dx, cz = cz0 + dz;
            Vec3 centre; if (!nebulaInCell(cx, cz, centre)) continue;
            if (length(centre - obs) > NEBULA_SEEN + NEBULA_RADIUS) continue;   // every patch of it is out of sight
            uint64_t h = hash2i(cx, cz, 0x4EBAULL);
            int np = 3 + (int)(unitFromHash(h) * 4);
            for (int k = 0; k < np && n < maxN; k++) {
                uint64_t hk = mix64(h + (uint64_t)k * 77);
                // the patches within NEBULA_RADIUS of the centre, flattened to the plane
                Vec3 pos = centre + Vec3((unitFromHash(hk) - 0.5) * 2 * NEBULA_RADIUS, (unitFromHash(mix64(hk + 1)) - 0.5) * 6.0, (unitFromHash(mix64(hk + 2)) - 0.5) * 2 * NEBULA_RADIUS);
                Vec3 rel = pos - obs;
                double dist = length(rel);
                if (dist < 0.5 || dist > NEBULA_SEEN) continue;
                double size = 2.0 + 3.0 * unitFromHash(mix64(hk + 3));
                NebulaPatch p;
                p.dir = rel / dist;
                p.radius = clampd(std::atan(size / dist), 2 * DEG, 30 * DEG);
                double u = unitFromHash(mix64(hk + 4));
                p.tone = u < 0.45 ? 0 : (u < 0.8 ? 1 : 2);
                p.inten = smoothstep(NEBULA_SEEN, NEBULA_RADIUS, dist) * (0.6 + 0.4 * unitFromHash(mix64(hk + 5)));   // full inside the complex, gone at NEBULA_SEEN
                out[n++] = p;
            }
        }
    return n;
}

double nebulaGlow(const NebulaPatch* p, int n, const Vec3& dir, int& tone) {
    double best = 0; tone = 2;
    for (int i = 0; i < n; i++) {
        double c = dot(dir, p[i].dir);
        if (c < std::cos(p[i].radius)) continue;
        double a = std::acos(clampd(c, -1, 1));
        double w = smoothstep(p[i].radius, p[i].radius * 0.2, a);
        double f = 0.55 + 0.45 * (0.5 + 0.5 * gnoise3(dir * 4.0 + p[i].dir * 7.0, 0x4EBCULL));   // wisps
        w *= f * p[i].inten;
        if (w > best) { best = w; tone = p[i].tone; }
    }
    return best;
}

int starNebulaTone(const Star& s) {
    double u = unitFromHash(mix64(s.seed ^ 0x4EB1ULL));
    return u < 0.4 ? 0 : (u < 0.75 ? 1 : 2);
}

int starNebulaPatches(const Star& s, const Vec3& dirToStar, NebulaPatch* out) {
    if (s.cls != STAR_PROTOSTAR) return 0;
    int tone = starNebulaTone(s);
    out[0].dir = dirToStar; out[0].radius = 1.1; out[0].tone = tone; out[0].inten = 0.8;          // the cloud lit round the star
    out[1].dir = dirToStar; out[1].radius = PI - 0.01; out[1].tone = tone; out[1].inten = 0.3;    // its faint light over the whole sky
    return 2;
}

// G-01: the density function. An exponential disc of scale length 8,000 ly whose thickness grows toward the centre, a
// gaussian bulge, four logarithmic arms of 12 degrees pitch that modulate the disc between 0.55 and 1.15 of its value
// (squared cosine: narrow crests, wide gaps), the hashed cluster knots, and the cap of one star per sector. The
// integral over the disc is 200 billion stars (`vesperis_test galaxy`); home sits on an arm crest at 0.15 stars per
// sector, the gap beside it holds 0.07, the outer disc at 25,000 ly 0.03.
static const double GALAXY_ARM_K = GALAXY_ARMS / std::tan(12.0 * DEG);   // the log spiral's winding: phase = arms * angle - K ln(r / r0)

GalaxyTerms galaxyTerms(double sx, double sy, double sz) {
    GalaxyTerms g;
    double x = sx - GALAXY_CENTRE_SX, z = sz - GALAXY_CENTRE_SZ;
    g.r = std::sqrt(x * x + z * z);
    g.h = GALAXY_THICK_FAR + GALAXY_THICK_NEAR * std::exp(-g.r / GALAXY_THICK_SCALE);
    g.disk = GALAXY_DISC_PEAK * std::exp(-g.r / GALAXY_DISC_SCALE) * std::exp(-std::fabs(sy) / g.h);
    g.bulge = GALAXY_BULGE_PEAK * std::exp(-(g.r * g.r + 4 * sy * sy) / (GALAXY_BULGE_R * GALAXY_BULGE_R));
    double a = 0.5 + 0.5 * std::cos(GALAXY_ARMS * std::atan2(z, x) - GALAXY_ARM_K * std::log(std::max(g.r, 1.0) / GALAXY_ARM_R0));
    g.arm = a * a;
    return g;
}

// G-02: the clusters. A cell decides by one hash whether it holds a cluster, and the cluster lies far enough inside
// its cell (its radius at most 50 ly in a 2,000 ly cell, 15 in a 100 ly cell) that a point need only ask its own cell.
bool globularInCell(int64_t cx, int64_t cy, int64_t cz, Vec3& centre, double& radius) {
    uint64_t h = hash3i(cx, cy, cz, 0x610BULL);
    double mx = (cx + 0.5) * GLOBULAR_CELL, my = (cy + 0.5) * GLOBULAR_CELL, mz = (cz + 0.5) * GLOBULAR_CELL;
    double r = std::sqrt(mx * mx + my * my + mz * mz) / 6000.0;
    double p = 0.6 / ((1 + r * r) * (1 + r * r));   // a cored r^-4 halo: ~145 clusters within 60,000 ly, half of them within 10,000 of the centre
    if (unitFromHash(h) >= p) return false;
    centre = Vec3(GALAXY_CENTRE_SX + cx * GLOBULAR_CELL + 100 + 1800 * unitFromHash(mix64(h + 1)), cy * GLOBULAR_CELL + 100 + 1800 * unitFromHash(mix64(h + 2)),
                  GALAXY_CENTRE_SZ + cz * GLOBULAR_CELL + 100 + 1800 * unitFromHash(mix64(h + 3)));
    radius = 15 + 35 * unitFromHash(mix64(h + 4));
    return true;
}

bool openClusterInCell(int64_t cx, int64_t cy, int64_t cz, Vec3& centre, double& radius) {
    uint64_t h = hash3i(cx, cy, cz, 0x0C1AULL);
    double u = unitFromHash(h);
    if (u >= 0.2) return false;   // the cheap test first: four cells in five hold nothing whatever the place
    GalaxyTerms g = galaxyTerms(GALAXY_CENTRE_SX + (cx + 0.5) * OPEN_CLUSTER_CELL, (cy + 0.5) * OPEN_CLUSTER_CELL, GALAXY_CENTRE_SZ + (cz + 0.5) * OPEN_CLUSTER_CELL);
    double p = 0.2 * (0.4 + 0.6 * g.arm) * std::exp(-std::fabs((cy + 0.5) * OPEN_CLUSTER_CELL) / g.h) * std::min(1.0, g.disk / 0.1);
    if (u >= p) return false;
    centre = Vec3(GALAXY_CENTRE_SX + cx * OPEN_CLUSTER_CELL + 15 + 70 * unitFromHash(mix64(h + 1)), cy * OPEN_CLUSTER_CELL + 15 + 70 * unitFromHash(mix64(h + 2)),
                  GALAXY_CENTRE_SZ + cz * OPEN_CLUSTER_CELL + 15 + 70 * unitFromHash(mix64(h + 3)));
    radius = 5 + 10 * unitFromHash(mix64(h + 4));
    return true;
}

int clusterAt(double sx, double sy, double sz, double& dens) {
    dens = 0;
    Vec3 c; double R;
    double x = sx - GALAXY_CENTRE_SX, z = sz - GALAXY_CENTRE_SZ;
    if (globularInCell((int64_t)std::floor(x / GLOBULAR_CELL), (int64_t)std::floor(sy / GLOBULAR_CELL), (int64_t)std::floor(z / GLOBULAR_CELL), c, R)) {
        double d = length(Vec3(sx, sy, sz) - c);
        if (d < R) { double q = d / (R / 2); dens = GALAXY_CAP / ((1 + q * q) * (1 + q * q)); return 2; }   // a Plummer-like profile, the core half the radius: solid stars inside, a fifth of the cap at the edge, ~1.2 R^3 stars
    }
    if (openClusterInCell((int64_t)std::floor(x / OPEN_CLUSTER_CELL), (int64_t)std::floor(sy / OPEN_CLUSTER_CELL), (int64_t)std::floor(z / OPEN_CLUSTER_CELL), c, R)) {
        double d = length(Vec3(sx, sy, sz) - c);
        if (d < R) { double q = d / (R / 2); dens = GALAXY_CAP / ((1 + q * q) * (1 + q * q)); return 1; }
    }
    return 0;
}

double galaxyDensity(double sx, double sy, double sz) {
    double d = fieldDensity(galaxyTerms(sx, sy, sz));
    double cd; if (clusterAt(sx, sy, sz, cd)) d = std::max(d, cd);   // G-02: a cluster's knot over the field
    return d > GALAXY_CAP ? GALAXY_CAP : d;
}

int galaxyRegion(int64_t sx, int64_t sy, int64_t sz) {
    double x = (double)sx, y = (double)sy, z = (double)sz;
    GalaxyTerms g = galaxyTerms(x, y, z);
    if (g.r < 0.5 * GALAXY_BULGE_R && std::fabs(y) < 0.16 * GALAXY_BULGE_R) return REGION_CORE;   // 1,250 ly across, 400 thick
    if (g.bulge > 0.6 * g.disk && g.r < 1.6 * GALAXY_BULGE_R) return REGION_BULGE;   // G-02: to 2,600 ly in the plane, 4,000 above it (the bare term won only to 1,750)
    double cd; int ck = clusterAt(x, y, z, cd);
    if (ck == 2) return REGION_CLUSTER;
    if (ck == 1) return REGION_OPEN;
    if (std::fabs(y) > 2.5 * g.h) return REGION_HALO;
    if (g.arm > 0.45) {   // G-03: within a star-forming complex
        Vec3 c; if (nebulaInCell((int64_t)std::floor(x / NEBULA_CELL), (int64_t)std::floor(z / NEBULA_CELL), c) && length(Vec3(x, y, z) - c) < NEBULA_RADIUS) return REGION_NEBULA;
    }
    if (g.arm > 0.4) return REGION_ARM;   // the arm's bright two fifths of the cycle (the density above the disc's mean)
    return REGION_DISK;
}

int starVariety(int family, int region, uint64_t hash) {
    double u = unitFromHash(hash);
    bool old = region == REGION_CORE || region == REGION_BULGE || region == REGION_HALO || region == REGION_CLUSTER;   // the old populations
    bool young = region == REGION_NEBULA || region == REGION_OPEN;                                                      // where stars form
    switch (family) {
        case STAR_YELLOW: {
            if (u < (old ? 0.5 : (young ? 0.35 : 0.4))) return STAR_RED_DWARF;
            // S-04: the protostars, out of the stars that stayed yellow, by a hash of their own: half of them in a star-forming
            // complex, a fifth in an open cluster, one in fifty along the arms (the clouds of the arms), none elsewhere
            double ps = region == REGION_NEBULA ? 0.5 : (region == REGION_OPEN ? 0.2 : (region == REGION_ARM ? 0.02 : 0.0));
            return ps > 0 && unitFromHash(mix64(hash ^ 0x9A07ULL)) < ps ? STAR_PROTOSTAR : family;
        }
        case STAR_ORANGE: return u < (old ? 0.85 : (young ? 0.65 : 0.75)) ? STAR_RED_DWARF : family;
        case STAR_BLUE_GIANT: {
            if (u < (old ? 0.3 : (young ? 0.65 : (region == REGION_ARM ? 0.6 : 0.5)))) return STAR_BLUE_WHITE;
            // S-05: the Wolf-Rayet stars, out of the blue giants that stayed, by a hash of their own: a fifth along the arms and where
            // stars form, a tenth in the core, one in twenty in the bulge and the disc, none in the halo or a globular (no massive star is old)
            double ws = (region == REGION_ARM || region == REGION_NEBULA || region == REGION_OPEN) ? 0.2 : (region == REGION_CORE ? 0.1 : ((region == REGION_BULGE || region == REGION_DISK) ? 0.05 : 0.0));
            return ws > 0 && unitFromHash(mix64(hash ^ 0x3E5FULL)) < ws ? STAR_WOLF_RAYET : family;
        }
        case STAR_RED_GIANT: { double og = old ? 0.4 : 0.3, cs = old ? 0.2 : 0.1; return u < og ? STAR_ORANGE_GIANT : (u < og + cs ? STAR_CARBON : family); }
        case STAR_PULSAR: {
            if (u < (old ? 0.55 : 0.35)) return STAR_NEUTRON;   // S-03: the quiet remnants, more among the old stars of the core, the bulge and the globulars
            // S-06: the black holes, out of the pulsars that stayed, by a hash of their own: a quarter in the core, a fifth in the bulge,
            // one in seven in a globular, one in ten in the halo, one in sixteen in the arms and the disc, one in thirty where stars form
            double bs = region == REGION_CORE ? 0.25 : (region == REGION_BULGE ? 0.2 : (region == REGION_CLUSTER ? 0.15 : (region == REGION_HALO ? 0.1 : (young ? 0.03 : 0.06))));
            return unitFromHash(mix64(hash ^ 0x8B1EULL)) < bs ? STAR_BLACK_HOLE : family;
        }
        default: return family;
    }
}

double starFlare(const Star& s, double t) {
    if (s.cls != STAR_RED_DWARF) return 0;
    double best = 0;
    int64_t cell = (int64_t)std::floor(t / FLARE_CELL);
    for (int64_t c = cell - 1; c <= cell; c++) {   // the last cell's flare may still be fading
        uint64_t h = hash2i(c, 0, s.seed ^ 0xF1A2EULL);
        if (unitFromHash(h) > 0.4) continue;
        double t0 = c * FLARE_CELL + unitFromHash(mix64(h)) * (FLARE_CELL - 90), amp = 0.6 + 0.8 * unitFromHash(mix64(h ^ 0x9E3779B97F4A7C15ULL));
        double dt = t - t0;
        if (dt < 0 || dt > 240) continue;
        best = std::max(best, amp * (dt < 6 ? dt / 6 : std::exp(-(dt - 6) / 40)));
    }
    return best;
}

bool starInSector(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName) {
    uint64_t seed = sectorSeed(sx, sy, sz);
    double d = galaxyDensity((double)sx, (double)sy, (double)sz);
    if (unitFromHash(seed) >= d) { out.valid = false; return false; }
    Rng rng(seed ^ 0x1234ABCDULL);
    out.sx = sx; out.sy = sy; out.sz = sz;
    out.pos = Vec3((sx + 0.1 + 0.8 * rng.uni()) * SECTOR_KM, (sy + 0.1 + 0.8 * rng.uni()) * SECTOR_KM,
                   (sz + 0.1 + 0.8 * rng.uni()) * SECTOR_KM);
    // class weights by region (M9-13): remnants and giants in the core, bulge and halo, young blue
    // giants where stars form, old red giants in clusters
    int region = galaxyRegion(sx, sy, sz);
    double w[STAR_CLASS_COUNT];
    for (int i = 0; i < STAR_CLASS_COUNT; i++) w[i] = STAR_CLASSES[i].rarity;
    switch (region) {
        case REGION_CORE: w[STAR_PULSAR] *= 3.0; w[STAR_RED_GIANT] *= 2.0; w[STAR_WHITE_DWARF] *= 1.5; w[STAR_BLUE_GIANT] *= 1.5; break;
        case REGION_BULGE: w[STAR_RED_GIANT] *= 2.2; w[STAR_WHITE_DWARF] *= 1.6; w[STAR_BLUE_GIANT] *= 0.5; break;
        case REGION_HALO: w[STAR_WHITE_DWARF] *= 2.5; w[STAR_RED_GIANT] *= 2.0; w[STAR_BLUE_GIANT] *= 0.2; w[STAR_YELLOW] *= 0.6; break;
        case REGION_CLUSTER: w[STAR_RED_GIANT] *= 2.5; w[STAR_ORANGE] *= 1.3; w[STAR_WHITE_DWARF] *= 1.5; w[STAR_YELLOW] *= 0.7; w[STAR_BLUE_GIANT] *= 0.1; w[STAR_PULSAR] *= 0.5; break;   // G-02: a globular is old
        case REGION_OPEN: w[STAR_BLUE_GIANT] *= 3.0; w[STAR_YELLOW] *= 1.3; w[STAR_RED_GIANT] *= 0.6; w[STAR_WHITE_DWARF] *= 0.2; w[STAR_PULSAR] *= 0.5; break;   // G-02: an open cluster is young
        case REGION_NEBULA: w[STAR_BLUE_GIANT] *= 3.5; w[STAR_YELLOW] *= 1.2; w[STAR_WHITE_DWARF] *= 0.4; break;
        case REGION_ARM: w[STAR_BLUE_GIANT] *= 1.6; break;
        default: break;
    }
    out.cls = starVariety(rng.pick(w, STAR_CLASS_COUNT), region, mix64(seed ^ 0x5A11E7ULL));   // S-01: the family by weight, then its variety by a hash of its own (mixed: the seed's own uniform is the existence test, under the density)
    const StarClassInfo& ci = STAR_CLASSES[out.cls];
    double rv = 1.0 + rng.sym(ci.radiusVar);
    out.radiusKm = ci.radiusKm * rv;
    out.luminosity = ci.luminosity * rv * rv * (0.85 + 0.3 * rng.uni());
    out.massFactor = ci.massFactor * (0.8 + 0.4 * rng.uni());
    out.color = ci.color;
    // small tint variation
    float tv = (float)rng.sym(0.06);
    out.color = clampRGB(RGB(out.color.r + tv, out.color.g, out.color.b - tv));
    out.seed = seed;
    out.name = withName ? generateName(seed, (region == REGION_CORE || region == REGION_HALO || region == REGION_BULGE || region == REGION_CLUSTER) ? 1 : ((region == REGION_NEBULA || region == REGION_ARM || region == REGION_OPEN) ? 2 : 0)) : std::string();   // G-02: hard names in the old globulars, flowing in the young open clusters
    out.pulseHz = out.cls == STAR_PULSAR ? 0.6 + 3.0 * rng.uni() : 0.0;
    out.valid = true;
    return true;
}

std::string generateName(uint64_t seed, int style) {
    static const char* onsets0[] = {"b", "bal", "br", "c", "cr", "d", "dr", "f", "fel", "g", "gr", "k", "kr", "l", "ly",
                                    "m", "n", "p", "r", "s", "st", "str", "t", "tr", "th", "v", "vr", "w", "y", "z",
                                    "zh", "sh", "ph", "kl", "gl", "sk", "ts", "x"};
    static const char* nuclei0[] = {"a", "e", "i", "o", "u", "ai", "ea", "io", "ya", "ey", "oo", "ia", "au", "ei"};
    static const char* codas0[] = {"", "", "", "", "n", "l", "r", "s", "st", "ck", "sk", "th", "m", "nd", "x", "ng", "rn",
                                   "lt", "ss"};
    // hard names of the old core and halo; flowing names of the young arms
    static const char* onsets1[] = {"k", "kr", "g", "gr", "th", "z", "x", "sk", "d", "dr", "t", "tr", "kh", "gh", "br", "vr"};
    static const char* nuclei1[] = {"a", "o", "u", "ei", "aa", "au", "o", "u"};
    static const char* codas1[] = {"k", "r", "n", "x", "th", "rk", "rt", "", "g", "d"};
    static const char* onsets2[] = {"l", "ly", "m", "n", "s", "sh", "v", "f", "y", "r", "el", "al", "ph", "th", "w", "h"};
    static const char* nuclei2[] = {"ia", "ea", "ai", "io", "ya", "e", "i", "a", "ie", "ei", "ee"};
    static const char* codas2[] = {"l", "n", "s", "", "", "", "la", "ne", "ri", "sh", "th"};
    const char* const* onsets = style == 1 ? onsets1 : (style == 2 ? onsets2 : onsets0);
    const char* const* nuclei = style == 1 ? nuclei1 : (style == 2 ? nuclei2 : nuclei0);
    const char* const* codas = style == 1 ? codas1 : (style == 2 ? codas2 : codas0);
    const int nOn = style == 1 ? 16 : (style == 2 ? 16 : 38), nNu = style == 1 ? 8 : (style == 2 ? 11 : 14), nCo = style == 1 ? 10 : (style == 2 ? 11 : 19);
    Rng rng(seed ^ 0x9A3E1F00ULL);
    int words = rng.chance(0.08) ? 2 : 1;
    std::string out;
    for (int attempt = 0; attempt < 4; attempt++) {
    out.clear();
    for (int wI = 0; wI < words; wI++) {
        int syl = 2 + rng.irange(2);
        if (rng.chance(0.12)) syl++;
        std::string word;
        for (int s = 0; s < syl; s++) {
            if (s > 0 || rng.chance(0.85)) word += onsets[rng.irange(nOn)];
            word += nuclei[rng.irange(nNu)];
            if (s == syl - 1 || rng.chance(0.35)) word += codas[rng.irange(nCo)];
        }
        word[0] = (char)toupper(word[0]);
        if (!out.empty()) out += " ";
        out += word;
    }
    if (out.size() <= 14) break;
    words = 1;
    }
    return out;
}

std::string romanNumeral(int n) {
    static const int v[] = {10, 9, 5, 4, 1};
    static const char* s[] = {"X", "IX", "V", "IV", "I"};
    std::string r;
    if (n <= 0) return "0";
    for (int i = 0; i < 5; i++) while (n >= v[i]) { r += s[i]; n -= v[i]; }
    return r;
}

void StarNeighborhood::update(const Vec3& posKm) {
    int64_t sx = sectorOf(posKm.x), sy = sectorOf(posKm.y), sz = sectorOf(posKm.z);
    if (sx == cx && sy == cy && sz == cz) return;
    cx = sx; cy = sy; cz = sz;
    stars.clear();
    for (int64_t x = sx - radius; x <= sx + radius; x++)
        for (int64_t y = sy - radius; y <= sy + radius; y++)
            for (int64_t z = sz - radius; z <= sz + radius; z++) {
                Star s;
                if (starInSector(x, y, z, s, false)) stars.push_back(s);
            }
}

bool nearestStarTo(const Vec3& p, int radius, Star& out, uint64_t skipSeed) {
    int64_t cx = sectorOf(p.x), cy = sectorOf(p.y), cz = sectorOf(p.z);
    double best = 1e300; bool found = false;
    for (int64_t x = cx - radius; x <= cx + radius; x++)
        for (int64_t y = cy - radius; y <= cy + radius; y++)
            for (int64_t z = cz - radius; z <= cz + radius; z++) {
                Star s;
                if (!starInSector(x, y, z, s, false) || (skipSeed && s.seed == skipSeed)) continue;
                double d = length2(s.pos - p);
                if (d < best) { best = d; out = s; found = true; }
            }
    return found;
}

// the end of the line first; a cube of 21 sectors is searched (1.4 ms at home) only where the density says it holds half a star or
// more, else the point comes back ten light years and the density is asked again (70 ns): a jump out of the disc costs a few
// thousand questions at most
bool jumpTarget(const Vec3& from, const Vec3& dir, double ly, Star& out, double& shortLy, uint64_t skipSeed) {
    ly = clampd(ly, 0, JUMP_MAX_LY);
    const int R = 10;
    const double cube = (2.0 * R + 1) * (2.0 * R + 1) * (2.0 * R + 1);
    for (double back = 0;; back = std::min(ly, back + 10)) {
        Vec3 q = from + dir * ((ly - back) * SECTOR_KM);
        bool last = back >= ly;
        if (last || galaxyDensity(q.x / SECTOR_KM, q.y / SECTOR_KM, q.z / SECTOR_KM) * cube >= 0.5) {
            if (nearestStarTo(q, R, out, skipSeed)) { shortLy = back; return true; }
        }
        if (last) return false;
    }
}

std::string galacticHeading(const Vec3& from, const Vec3& dir) {
    if (dir.y > 0.766) return "NORTH OUT OF THE DISC";   // fifty degrees out of the plane
    if (dir.y < -0.766) return "SOUTH OUT OF THE DISC";
    double hl = std::sqrt(dir.x * dir.x + dir.z * dir.z), cx = GALAXY_CENTRE_SX - from.x, cz = GALAXY_CENTRE_SZ - from.z, cl = std::sqrt(cx * cx + cz * cz);
    if (hl < 1e-9 || cl < 1e-9) return "ALONG THE DISC";
    double c = (dir.x * cx + dir.z * cz) / (hl * cl);
    return c > 0.707 ? "COREWARD" : (c < -0.707 ? "RIMWARD" : "ALONG THE DISC");
}

double vimanaSeconds(double ly) {
    ly = std::max(0.0, ly);
    return ly <= 100 ? 7.0 + 2.0 * std::sqrt(ly) : 27.0 + 6.0 * std::log2(ly / 100.0);
}

const Star* StarNeighborhood::nearest(const Vec3& posKm, double* distOut) const {
    const Star* best = nullptr;
    double bd = 1e300;
    for (const Star& s : stars) {
        double d = length2(s.pos - posKm);
        if (d < bd) { bd = d; best = &s; }
    }
    if (distOut) *distOut = std::sqrt(bd);
    return best;
}
