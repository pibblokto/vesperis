#include "starfield.h"
#include "core/noise.h"
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
};

void buildGalaxyBand(const Vec3& obs, std::vector<float>& map, int W, int H) {
    map.assign((size_t)W * H, 0.f);
    float mx = 1e-9f;
    for (int j = 0; j < H; j++)
        for (int i = 0; i < W; i++) {
            double lon = (i + 0.5) / W * TAU, lat = ((j + 0.5) / H - 0.5) * PI;
            Vec3 d(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon));
            double sum = 0, s = 2;
            for (int k = 0; k < 28; k++) { Vec3 p = obs + d * s; double ds = s * 0.18; sum += galaxyDensity(p.x, p.y, p.z) * ds; s += ds; }
            map[(size_t)j * W + i] = (float)sum;
            mx = std::max(mx, (float)sum);
        }
    for (float& v : map) v /= mx;
}

double sampleGalaxyBand(const std::vector<float>& map, int W, int H, const Vec3& d) {
    if (map.empty()) return 0;
    double lon = std::atan2(d.z, d.x); if (lon < 0) lon += TAU;
    double lat = std::asin(clampd(d.y, -1, 1));
    double fx = lon / TAU * W - 0.5, fy = (lat / PI + 0.5) * H - 0.5;
    int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    double tx = fx - x0, ty = fy - y0;
    auto at = [&](int x, int y) { x = ((x % W) + W) % W; y = clampi(y, 0, H - 1); return (double)map[(size_t)y * W + x]; };
    return (at(x0, y0) * (1 - tx) + at(x0 + 1, y0) * tx) * (1 - ty) + (at(x0, y0 + 1) * (1 - tx) + at(x0 + 1, y0 + 1) * tx) * ty;
}

const char* REGION_NAMES[REGION_COUNT] = {"GALACTIC CORE", "THE BULGE", "SPIRAL ARM", "THE DISK", "THE HALO", "STAR CLUSTER", "STAR-FORMING REGION"};

namespace {
inline bool clusterAt(double sx, double sy, double sz) {
    return unitFromHash(hash3i((int64_t)std::floor(sx / 6.0), (int64_t)std::floor(sy / 6.0), (int64_t)std::floor(sz / 6.0), 0xC157ULL)) < 0.015;
}
inline bool nebulaAt(double sx, double sz) {
    return unitFromHash(hash2i((int64_t)std::floor(sx / 14.0), (int64_t)std::floor(sz / 14.0), 0x4EBULL)) < 0.15;
}
}

int nebulaPatches(const Vec3& obs, NebulaPatch* out, int maxN) {
    int n = 0;
    int64_t cx0 = (int64_t)std::floor(obs.x / 14.0), cz0 = (int64_t)std::floor(obs.z / 14.0);
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++) {
            int64_t cx = cx0 + dx, cz = cz0 + dz;
            if (unitFromHash(hash2i(cx, cz, 0x4EBULL)) >= 0.15) continue;   // the nebula cells of galaxyRegion
            double x = (cx + 0.5) * 14.0, z = (cz + 0.5) * 14.0;
            double r = std::sqrt(x * x + z * z), ang = std::atan2(z, x);
            if (0.5 + 0.5 * std::cos(2.0 * ang - r / 45.0) <= 0.72) continue;   // only in the arms
            uint64_t h = hash2i(cx, cz, 0x4EBAULL);
            int np = 3 + (int)(unitFromHash(h) * 4);
            for (int k = 0; k < np && n < maxN; k++) {
                uint64_t hk = mix64(h + (uint64_t)k * 77);
                Vec3 pos(cx * 14.0 + unitFromHash(hk) * 14.0, (unitFromHash(mix64(hk + 1)) - 0.5) * 6.0, cz * 14.0 + unitFromHash(mix64(hk + 2)) * 14.0);
                Vec3 rel = pos - obs;
                double dist = length(rel);
                if (dist < 0.5) continue;
                double size = 2.0 + 3.0 * unitFromHash(mix64(hk + 3));
                NebulaPatch p;
                p.dir = rel / dist;
                p.radius = clampd(std::atan(size / dist), 2 * DEG, 30 * DEG);
                double u = unitFromHash(mix64(hk + 4));
                p.tone = u < 0.45 ? 0 : (u < 0.8 ? 1 : 2);
                p.inten = clampd(1.3 - dist / 22.0, 0.12, 1.0) * (0.6 + 0.4 * unitFromHash(mix64(hk + 5)));
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

double galaxyDensity(double sx, double sy, double sz) {
    double r = std::sqrt(sx * sx + sz * sz);
    double h = 10.0 + 45.0 * std::exp(-r / 130.0);
    double disk = 0.85 * std::exp(-r / 230.0) * std::exp(-std::fabs(sy) / h);
    double bulge = 0.9 * std::exp(-(r * r + 4 * sy * sy) / (70.0 * 70.0));
    // a couple of spiral arms, mild modulation
    double ang = std::atan2(sz, sx);
    double arm = 0.5 + 0.5 * std::cos(2.0 * ang - r / 45.0);
    double d = disk * (0.55 + 0.6 * arm) + bulge;
    if (clusterAt(sx, sy, sz)) d *= 2.5;   // M9-13 globular clusters: dense knots
    return d > 0.97 ? 0.97 : d;
}

int galaxyRegion(int64_t sx, int64_t sy, int64_t sz) {
    double x = (double)sx, y = (double)sy, z = (double)sz;
    double r = std::sqrt(x * x + z * z);
    double h = 10.0 + 45.0 * std::exp(-r / 130.0);
    double disk = 0.85 * std::exp(-r / 230.0) * std::exp(-std::fabs(y) / h);
    double bulge = 0.9 * std::exp(-(r * r + 4 * y * y) / (70.0 * 70.0));
    double ang = std::atan2(z, x);
    double arm = 0.5 + 0.5 * std::cos(2.0 * ang - r / 45.0);
    if (r < 35 && std::fabs(y) < 12) return REGION_CORE;
    if (bulge > disk && r < 110) return REGION_BULGE;
    if (std::fabs(y) > 2.5 * h) return REGION_HALO;
    if (clusterAt(x, y, z)) return REGION_CLUSTER;
    if (arm > 0.72 && nebulaAt(x, z)) return REGION_NEBULA;
    if (arm > 0.7) return REGION_ARM;
    return REGION_DISK;
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
        case REGION_CLUSTER: w[STAR_RED_GIANT] *= 1.8; w[STAR_ORANGE] *= 1.4; w[STAR_BLUE_GIANT] *= 0.4; break;
        case REGION_NEBULA: w[STAR_BLUE_GIANT] *= 3.5; w[STAR_YELLOW] *= 1.2; w[STAR_WHITE_DWARF] *= 0.4; break;
        case REGION_ARM: w[STAR_BLUE_GIANT] *= 1.6; break;
        default: break;
    }
    out.cls = rng.pick(w, STAR_CLASS_COUNT);
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
    out.name = withName ? generateName(seed, (region == REGION_CORE || region == REGION_HALO || region == REGION_BULGE) ? 1 : ((region == REGION_NEBULA || region == REGION_ARM) ? 2 : 0)) : std::string();
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
