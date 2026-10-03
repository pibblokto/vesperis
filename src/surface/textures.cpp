#include "textures.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr int N = 128;

// Periodic value noise on the tile lattice: repeats every `period` texels.
double pvalue(double x, double y, int period, uint64_t seed) {
    double fx = x * period / N, fy = y * period / N;
    int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    double tx = fx - x0, ty = fy - y0;
    tx = tx * tx * (3 - 2 * tx); ty = ty * ty * (3 - 2 * ty);
    auto h = [&](int ix, int iy) { return unitFromHash(hash2i(((ix % period) + period) % period, ((iy % period) + period) % period, seed)) * 2 - 1; };
    double a = h(x0, y0), b = h(x0 + 1, y0), c = h(x0, y0 + 1), d = h(x0 + 1, y0 + 1);
    return (a + (b - a) * tx) * (1 - ty) + (c + (d - c) * tx) * ty;
}

// Periodic Worley noise: one feature point per lattice cell of `cells` per tile side.
void pworley(double x, double y, int cells, uint64_t seed, double& f1, double& f2, uint64_t& id1) {
    double fx = x * cells / N, fy = y * cells / N;
    int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    f1 = 1e9; f2 = 1e9; id1 = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int cx = x0 + dx, cy = y0 + dy;
            uint64_t h = hash2i(((cx % cells) + cells) % cells, ((cy % cells) + cells) % cells, seed);
            double px = cx + unitFromHash(h), py = cy + unitFromHash(mix64(h + 1));
            double d = std::sqrt((px - fx) * (px - fx) + (py - fy) * (py - fy));
            if (d < f1) { f2 = f1; f1 = d; id1 = h; }
            else if (d < f2) f2 = d;
        }
}

// Periodic gradient (Perlin) noise on the tile lattice: no lattice showing through, unlike value noise at low periods.
double pgrad(double x, double y, int period, uint64_t seed) {
    double fx = x * period / N, fy = y * period / N;
    int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    double tx = fx - x0, ty = fy - y0;
    double sx = tx * tx * tx * (tx * (tx * 6 - 15) + 10), sy = ty * ty * ty * (ty * (ty * 6 - 15) + 10);
    auto g = [&](int ix, int iy, double dx, double dy) {
        uint64_t h = hash2i(((ix % period) + period) % period, ((iy % period) + period) % period, seed);
        double a = unitFromHash(h) * TAU;
        return std::cos(a) * dx + std::sin(a) * dy;
    };
    double n00 = g(x0, y0, tx, ty), n10 = g(x0 + 1, y0, tx - 1, ty), n01 = g(x0, y0 + 1, tx, ty - 1), n11 = g(x0 + 1, y0 + 1, tx - 1, ty - 1);
    return ((n00 + (n10 - n00) * sx) * (1 - sy) + (n01 + (n11 - n01) * sx) * sy) * 1.4;
}

// a periodic ripple field: sines across a direction, bent by low noise (sand, snow, ropy lava, rippled ice)
double ripples(double x, double y, double ang, double wavelength, double bend, uint64_t seed) {
    double along = x * std::cos(ang) + y * std::sin(ang);
    return std::sin(TAU * (along / wavelength + bend * pvalue(x, y, 8, seed)));
}

int variantsOf(int material) {
    switch (material) {
        case MAT_ROCK: case MAT_BASALT: case MAT_SAND: case MAT_DUST: case MAT_GRASS: case MAT_SNOW: case MAT_ICE: return 3;
        case MAT_METAL: case MAT_SULPHUR: case MAT_GRAPHITE: case MAT_FOREST: case MAT_QUARTZ: return 2;
        default: return 1;
    }
}
}

bool materialHasMesoTile(int material) {
    switch (material) {
        case MAT_SAND: case MAT_DUST: case MAT_ROCK: case MAT_BASALT: case MAT_ICE: case MAT_SNOW:
        case MAT_GRASS: case MAT_FOREST: case MAT_QUARTZ: case MAT_METAL: case MAT_SULPHUR: case MAT_GRAPHITE: case MAT_SALT: case MAT_GLASS: return true;
        default: return false;
    }
}

int mesoVariant(int material, uint64_t seed) {
    int n = variantsOf(material);
    if (n <= 1) return 0;
    return (int)(unitFromHash(mix64(seed ^ (0x7E5ULL + (uint64_t)material * 0x9E3779B97F4A7C15ULL))) * n) % n;
}

const char* mesoVariantName(int material, int variant) {
    static const char* names[MAT_COUNT][3] = {
        {"CRACKED", "BOULDERS", "LAYERED"},          // rock
        {"RIPPLES", "PLAYA", "PEBBLY"},              // sand
        {"TUFTS", "MEADOW", "SCRUB"},                // grass
        {"LITTER", "ROOTS", ""},                     // forest
        {"MOTTLED", "SASTRUGI", "CRUSTED"},          // snow
        {"", "", ""},                                // water
        {"", "", ""},                                // lava
        {"FACETED", "RIPPLED", "SHATTERED"},         // ice
        {"", "", ""},                                // cloud
        {"FACETED", "DRUSY", ""},                    // quartz
        {"PLATES", "ROPY", "BLOCKY"},                // basalt
        {"PEBBLES", "REGOLITH", "RUBBLE"},           // dust
        {"", "", ""},                                // gas
        {"HAMMERED", "DENDRITIC", ""},               // metal
        {"CRUSTY", "LOBES", ""},                     // sulphur
        {"FLAKY", "SOOTY", ""},                      // graphite
        {"POLYGONS", "", ""},                        // salt (R-305)
        {"RIPPLED", "", ""},                         // glass (S-03)
    };
    if (material < 0 || material >= MAT_COUNT || variant < 0 || variant > 2) return "";
    return names[material][variant];
}

void buildMesoTile(GrainTexture& g, int material, uint64_t seed) {
    g.resize(N);
    Rng rng(seed ^ (0x51ED1ULL * (material + 1)));
    uint64_t sA = rng.next(), sB = rng.next(), sC = rng.next();
    double windA = rng.range(0, TAU);
    int var = mesoVariant(material, seed);
    double strength = 0.8 + 0.5 * unitFromHash(mix64(seed ^ (0xA57ULL + (uint64_t)material * 977)));   // B-311: 0.8 .. 1.3 per planet
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            double v = 0;
            double f1, f2; uint64_t id;
            switch (material) {
                case MAT_SAND: {
                    if (var == 0) {
                        // ripples across the wind, bent by low noise, plus a few pebble specks
                        v = 4.5 * ripples(x, y, windA, 6.0, 0.5, sA);
                        pworley(x, y, 16, sB, f1, f2, id);
                        if (f1 < 0.18 && (id % 3) == 0) v += 6;
                    } else if (var == 1) {
                        // playa: a flat pan cracked into polygons, faint tonal patches
                        pworley(x, y, 7, sA, f1, f2, id);
                        if (f2 - f1 < 0.08) v -= 9 * (1 - (f2 - f1) / 0.08);
                        v += (double)((int)(id % 5) - 2) * 0.7 + 2 * pvalue(x, y, 4, sB);
                    } else {
                        // pebbly sand: fine bumps over broad ripples
                        v = 2.5 * ripples(x, y, windA, 14.0, 0.6, sA);
                        pworley(x, y, 28, sB, f1, f2, id);
                        if (f1 < 0.25) v += 4 * (1 - f1 / 0.25) - 1;
                        v += 1.5 * pvalue(x, y, 32, sC);
                    }
                    break;
                }
                case MAT_DUST: {
                    if (var == 0) {
                        // pebble field: bright bumps with a dark rim; sparse craterlets
                        pworley(x, y, 24, sA, f1, f2, id);
                        if (f1 < 0.3) v += 5 * (1 - f1 / 0.3) - 1.5;
                        else if (f1 < 0.4) v -= 2.5 * (1 - (f1 - 0.3) / 0.1);
                        pworley(x, y, 6, sB, f1, f2, id);
                        if ((id % 5) == 0 && f1 < 0.45) v -= 7 * (1 - f1 / 0.45);
                        v += 2 * pvalue(x, y, 16, sC);
                    } else if (var == 1) {
                        // regolith: smooth, with craterlets (a bright rim round a dark bowl)
                        v += 3 * pvalue(x, y, 6, sA) + 1.5 * pvalue(x, y, 24, sB);
                        pworley(x, y, 9, sC, f1, f2, id);
                        if ((id % 3) == 0) { if (f1 < 0.3) v -= 8 * (1 - f1 / 0.3); else if (f1 < 0.42) v += 5 * (1 - (f1 - 0.3) / 0.12); }
                    } else {
                        // rubble: angular blocks of several tones, dark gaps between
                        pworley(x, y, 14, sA, f1, f2, id);
                        v += (double)((int)(id % 7) - 3) * 1.4;
                        if (f2 - f1 < 0.08) v -= 7;
                        v += 1.5 * pvalue(x, y, 32, sB);
                    }
                    break;
                }
                case MAT_ROCK: {
                    if (var == 0) {
                        // cracks along cell borders and faint strata
                        pworley(x, y, 8, sA, f1, f2, id);
                        if (f2 - f1 < 0.09) v -= 10 * (1 - (f2 - f1) / 0.09);
                        double strata = std::sin(TAU * ((x * 0.3 + y * 0.95) / 20.0 + 0.8 * pvalue(x, y, 4, sB)));
                        v += 3 * strata + 2 * pvalue(x, y, 32, sC);
                    } else if (var == 1) {
                        // boulders: domes lit on top, a dark rim, bare ground between
                        pworley(x, y, 10, sA, f1, f2, id);
                        double rr = 0.25 + 0.2 * unitFromHash(mix64(id + 7));
                        if (f1 < rr) { double q = f1 / rr; v += 9 * (1 - q * q) - 2; }
                        else if (f1 < rr + 0.08) v -= 6 * (1 - (f1 - rr) / 0.08);
                        v += 2 * pvalue(x, y, 32, sB);
                    } else {
                        // layered: strong strata with a warped phase, sparse cracks across them
                        double strata = std::sin(TAU * ((x * 0.2 + y) / 11.0 + 1.2 * pvalue(x, y, 3, sA)));
                        v += 6 * strata + 2 * std::sin(TAU * ((x * 0.2 + y) / 3.7 + 0.5 * pvalue(x, y, 5, sB)));
                        pworley(x, y, 5, sC, f1, f2, id);
                        if (f2 - f1 < 0.05) v -= 8;
                    }
                    break;
                }
                case MAT_BASALT: {
                    if (var == 0) {
                        // cooled plates: flat cells with dark boundaries, per-plate tone
                        pworley(x, y, 6, sA, f1, f2, id);
                        if (f2 - f1 < 0.07) v -= 12 * (1 - (f2 - f1) / 0.07);
                        v += (double)((int)(id % 7) - 3) * 0.8;
                        v += 1.5 * pvalue(x, y, 32, sB);
                    } else if (var == 1) {
                        // ropy: the folds of a pahoehoe flow
                        v += 5 * ripples(x, y, windA, 7.0, 1.4, sA) + 2 * ripples(x, y, windA + 0.4, 3.0, 0.8, sB);
                        v += 2 * pvalue(x, y, 16, sC);
                    } else {
                        // blocky: the clinker of an aa flow, dark gaps everywhere
                        pworley(x, y, 18, sA, f1, f2, id);
                        v += (double)((int)(id % 9) - 4) * 1.3;
                        if (f2 - f1 < 0.1) v -= 9 * (1 - (f2 - f1) / 0.1);
                    }
                    break;
                }
                case MAT_ICE: {
                    if (var == 0) {
                        // crystalline facets and dark fracture lines
                        pworley(x, y, 9, sA, f1, f2, id);
                        v += (double)((int)(id % 9) - 4) * 1.3;
                        pworley(x, y, 3, sB, f1, f2, id);
                        if (f2 - f1 < 0.05) v -= 14 * (1 - (f2 - f1) / 0.05);
                    } else if (var == 1) {
                        // rippled: wind-polished waves and a few fractures
                        v += 4 * ripples(x, y, windA, 9.0, 0.7, sA);
                        pworley(x, y, 3, sB, f1, f2, id);
                        if (f2 - f1 < 0.04) v -= 12;
                        v += 1.5 * pvalue(x, y, 32, sC);
                    } else {
                        // shattered: a dense mosaic of shards in many tones
                        pworley(x, y, 16, sA, f1, f2, id);
                        v += (double)((int)(id % 11) - 5) * 1.6;
                        if (f2 - f1 < 0.07) v -= 10 * (1 - (f2 - f1) / 0.07);
                    }
                    break;
                }
                case MAT_SNOW: {
                    if (var == 0) {
                        // soft mottling and rare sparkle points
                        v += 3 * pvalue(x, y, 8, sA) + 1.5 * pvalue(x, y, 32, sB);
                    } else if (var == 1) {
                        // sastrugi: wind-cut ridges
                        v += 4 * ripples(x, y, windA, 10.0, 1.0, sA) + 1.5 * pvalue(x, y, 32, sB);
                    } else {
                        // crusted: plates of wind crust with bright broken rims
                        pworley(x, y, 7, sA, f1, f2, id);
                        v += (double)((int)(id % 5) - 2) * 1.0;
                        if (f2 - f1 < 0.06) v += 6;
                        v += 1.5 * pvalue(x, y, 32, sB);
                    }
                    if (unitFromHash(hash2i(x, y, sC)) < 0.004) v += 24;
                    break;
                }
                case MAT_GRASS: {
                    if (var == 0) {
                        // tufts (a bright top texel over a darker one) and bare patches
                        pworley(x, y, 26, sA, f1, f2, id);
                        if (f1 < 0.2 && (id % 3) == 0) v += 5;
                        pworley(x, y + 1, 26, sA, f1, f2, id);
                        if (f1 < 0.2 && (id % 3) == 0) v -= 3;
                        if (pvalue(x, y, 4, sB) > 0.55) v -= 6;
                        v += 2 * pvalue(x, y, 32, sC);
                    } else if (var == 1) {
                        // meadow: soft mottle, few tufts
                        v += 4 * pvalue(x, y, 6, sA) + 2 * pvalue(x, y, 20, sB);
                        pworley(x, y, 20, sC, f1, f2, id);
                        if (f1 < 0.15 && (id % 5) == 0) v += 4;
                    } else {
                        // scrub: dark clumps on paler dry ground
                        pworley(x, y, 12, sA, f1, f2, id);
                        double rr = 0.25 + 0.25 * unitFromHash(mix64(id + 3));
                        if (f1 < rr) v -= 7 * (1 - f1 / rr);
                        else v += 2;
                        v += 2 * pvalue(x, y, 32, sB);
                    }
                    break;
                }
                case MAT_METAL: {
                    if (var == 0) {
                        // hammered plates and pits
                        pworley(x, y, 12, sA, f1, f2, id);
                        v += (double)((int)(id % 7) - 3) * 1.5;
                        if (f1 < 0.15) v -= 8 * (1 - f1 / 0.15);
                        if (f2 - f1 < 0.05) v += 6;
                    } else {
                        // dendritic: bright branching veins over dark pitted ground
                        pworley(x, y, 8, sA, f1, f2, id);
                        if (f2 - f1 < 0.06) v += 10 * (1 - (f2 - f1) / 0.06);
                        pworley(x, y, 20, sB, f1, f2, id);
                        if (f1 < 0.2) v -= 6 * (1 - f1 / 0.2);
                        v += 2 * pvalue(x, y, 32, sC);
                    }
                    break;
                }
                case MAT_SULPHUR: {
                    if (var == 0) {
                        // crusty deposits and flow lobes
                        v += 5 * pvalue(x, y, 8, sA) + 3 * pvalue(x, y, 24, sB);
                        pworley(x, y, 10, sC, f1, f2, id);
                        if (f2 - f1 < 0.06) v -= 6;
                    } else {
                        // lobes: overlapping rounded flows with bright edges
                        pworley(x, y, 6, sA, f1, f2, id);
                        v += (double)((int)(id % 5) - 2) * 1.5;
                        if (f2 - f1 < 0.1) v += 5 * (1 - (f2 - f1) / 0.1);
                        v += 3 * pvalue(x, y, 16, sB);
                    }
                    break;
                }
                case MAT_GRAPHITE: {
                    if (var == 0) {
                        // flaky plates
                        pworley(x, y, 16, sA, f1, f2, id);
                        v += (double)((int)(id % 5) - 2) * 1.2;
                        if (f2 - f1 < 0.05) v += 4;
                    } else {
                        // sooty: dark smooth mottle with a few bright flakes
                        v += 3 * pvalue(x, y, 5, sA) + 1.5 * pvalue(x, y, 24, sB);
                        pworley(x, y, 22, sC, f1, f2, id);
                        if (f1 < 0.12 && (id % 4) == 0) v += 8;
                    }
                    break;
                }
                case MAT_FOREST: {
                    if (var == 0) {
                        // leaf litter mottling
                        v += 6 * pvalue(x, y, 32, sA) + 3 * pvalue(x, y, 11, sB);
                    } else {
                        // roots: dark lines through the litter
                        v += 4 * pvalue(x, y, 32, sA) + 3 * pvalue(x, y, 9, sB);
                        pworley(x, y, 5, sC, f1, f2, id);
                        if (f2 - f1 < 0.05) v -= 7 * (1 - (f2 - f1) / 0.05);
                    }
                    break;
                }
                case MAT_SALT: {   // R-305: a crust cracked into polygons, the plates faintly domed, the cracks dark
                    pworley(x, y, 9, sA, f1, f2, id);
                    v += (double)((int)(id % 5) - 2) * 1.5 + 4.0 * (1 - clampd(f1 / 0.5, 0, 1));
                    if (f2 - f1 < 0.07) v -= 12 * (1 - (f2 - f1) / 0.07);
                    break;
                }
                case MAT_GLASS: {   // S-03: a melt frozen in ripples, pocked with bubble pits, a few hairline cracks
                    v += 4 * pvalue(x, y, 7, sA) + 2 * pvalue(x, y, 19, sB);
                    pworley(x, y, 16, sC, f1, f2, id);
                    if (f1 < 0.1 && (id % 3) == 0) v -= 7 * (1 - f1 / 0.1);
                    if (f2 - f1 < 0.025 && (id % 5) == 0) v -= 5;
                    break;
                }
                case MAT_QUARTZ: {
                    if (var == 0) {
                        // translucent facets with a few glints
                        pworley(x, y, 13, sA, f1, f2, id);
                        v += (double)((int)(id % 11) - 5) * 1.2;
                        if (f2 - f1 < 0.06) v -= 5;
                        if (unitFromHash(hash2i(x, y, sC)) < 0.006) v += 30;
                    } else {
                        // drusy: a carpet of small crystals, many glints
                        pworley(x, y, 24, sA, f1, f2, id);
                        v += (double)((int)(id % 7) - 3) * 1.4;
                        if (f1 < 0.12) v += 6;
                        if (unitFromHash(hash2i(x, y, sC)) < 0.012) v += 26;
                    }
                    break;
                }
                default: break;
            }
            g.v[(y << g.shift) | x] = (int8_t)clampi((int)std::lround(v * strength), -60, 60);
        }
}

// B-311: the broad tone tile: three octaves of periodic gradient noise, +-10, that the rasteriser adds at two metres a texel
// (patches of 23, 43 and 85 m) so the ground of one material is not one flat tone from cell to cell.
void buildMacroTile(GrainTexture& g, uint64_t seed) {
    g.resize(N);
    uint64_t sA = mix64(seed ^ 0x3A11ULL), sB = mix64(seed ^ 0x3A12ULL), sC = mix64(seed ^ 0x3A13ULL);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            double v = 6.5 * pgrad(x, y, 3, sA) + 4 * pgrad(x, y, 6, sB) + 2.5 * pgrad(x, y, 11, sC);
            g.v[(y << g.shift) | x] = (int8_t)clampi((int)std::lround(v), -60, 60);
        }
}

void buildLeafTiles(GrainTexture& leaf, GrainTexture& edge, uint64_t seed) {
    leaf.resize(N); edge.resize(N);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            // leaves: small bright and dark lumps (3-4 texels) over a softer mottle, with a few sharp glints
            double n = 9.0 * pgrad(x, y, 32, seed ^ 0x1EAF1ULL) + 5.0 * pgrad(x, y, 16, seed ^ 0x1EAF2ULL) + 4.0 * pgrad(x, y, 8, seed ^ 0x1EAF3ULL);
            double glint = pvalue(x, y, 64, seed ^ 0x1EAF4ULL);
            if (glint > 0.72) n += 10.0 * (glint - 0.72) / 0.28;
            leaf.v[(size_t)y * N + x] = (int8_t)clampi((int)std::lround(n), -127, 127);
            // the rim: lobes of 3-6 texels, so a cluster's edge breaks into leafy tufts and a few holes open inside
            double e = 0.65 * pgrad(x, y, 24, seed ^ 0x1EAF5ULL) + 0.35 * pgrad(x, y, 48, seed ^ 0x1EAF6ULL);
            edge.v[(size_t)y * N + x] = (int8_t)clampi((int)std::lround(e * 30.0), -127, 127);   // +-0.45 weight units at ragged 1 (the rasteriser divides by 64)
        }
}

void buildBarkTile(GrainTexture& g, uint64_t seed) {
    g.resize(N);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            // ridges and furrows across (x), long along (y): the trunk maps x at 8 texels a metre and y at 2, so a furrow
            // 3 texels wide is 4 cm across and runs a metre and a half
            double n = 12.0 * pgrad(x, y, 24, seed ^ 0xBA4C1ULL) + 9.0 * pgrad(x, y, 12, seed ^ 0xBA4C2ULL) + 4.0 * pvalue(x, y, 64, seed ^ 0xBA4C3ULL);
            double f1, f2; uint64_t id;
            pworley(x, y * 0.35, 10, seed ^ 0xBA4C4ULL, f1, f2, id);
            n -= 10.0 * (1 - smoothstep(0.05, 0.22, f2 - f1));   // the crack lines between the plates
            g.v[(size_t)y * N + x] = (int8_t)clampi((int)std::lround(n), -127, 127);
        }
}
