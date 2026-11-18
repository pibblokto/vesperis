// Z-03: what a vial holds (life/samples.h): a microhabitat's species and their counts, its activity, the living films' density and glow
#include "life/samples.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

const char* const VIAL_ACTIVITY_NAMES[VA_COUNT] = {"NONE", "FAINT", "STRONG"};

namespace {
constexpr uint64_t SALT_SAMPLE = 0x11FE57A6E0000010ULL;   // a species' presence and count in a square metre
constexpr uint64_t SALT_PLACE = 0x11FE57A6E0000011ULL;    // the square metre's key
constexpr uint64_t SALT_COOKED = 0x11FE57A6E0000012ULL;   // a sterile world's cooked organics

// how crowded a niche's life is, log10 cells in a millilitre (or a gram) where its species is common: a mat's and a stromatolite's skin
// 10^9, wet silt and soil 10^8 to 10^9 (Whitman et al. 1998), a spring's mats 10^8, the open water 10^6, the deep rock 10^4 (Pedersen 2000),
// the snow 10^4 (red snow's 10^4-10^5 algae a millilitre: Hoham and Remias 2020), the ice 10^3, the dust 10^2, the clouds' droplets 10^3
constexpr double CROWD[NI_COUNT] = {6.0, 8.5, 8.5, 9.0, 7.5, 8.0, 7.5, 3.0, 4.3, 6.5, 4.0, 3.0, 6.5, 3.0, 2.0};

double h01(uint64_t h) { return unitFromHash(mix64(h)); }
}   // namespace

uint64_t placeKey(uint64_t bodySeed, const Vec3& unit, double radiusKm) {
    const double R = radiusKm * 1000.0;
    const int64_t ix = (int64_t)std::llround(unit.x * R), iy = (int64_t)std::llround(unit.y * R), iz = (int64_t)std::llround(unit.z * R);
    return mix64(hashCombine(hashCombine(hashCombine(bodySeed ^ SALT_PLACE, (uint64_t)ix), (uint64_t)iy), (uint64_t)iz));
}

// a species lives in a square metre where its niche is the place's and the place's temperature, salt and light let it; it is there by its
// share (a patchy world: a common species in most square metres, a rare one in few), its count its niche's crowd by its share, a decade either
// way by the place; out of its growth range it waits there, alive and inactive (a psychrophile's cold snow, a thermophile's cooled pool's edge)
Sample sampleOf(const Biota& B, const Microhabitat& h) {
    Sample S;
    if (h.niche < 0 || h.niche >= NI_COUNT) return S;
    const uint32_t bit = nicheBit(h.niche);
    double activeLog = -1;
    bool proto = false;
    for (const Microbe& s : B.species) {
        const uint64_t k = hashCombine(h.place ^ SALT_SAMPLE, (uint64_t)s.index * 0x9E3779B97F4A7C15ULL + 1);
        const double there = h01(k), spread = h01(k + 1);
        Held H; H.species = s.index; H.state = s.state;
        if (s.state == SS_FOSSIL) {   // carbon outlines in the world's old rock, a few a section
            if (h.niche != NI_ROCK || !h.oldRock || there > 0.6) continue;
            H.cells = 1 + std::floor(6 * spread);
            S.held.push_back(H);
            continue;
        }
        if (!(s.niches & bit)) continue;
        if (s.state == SS_PROTO) {   // protocells where their chemistry gathers (no limits: they are not yet alive)
            if (there > 0.75) continue;
            H.cells = std::pow(10.0, CROWD[h.niche] - 1.5 + 1.2 * (spread - 0.5));
            proto = true;
            S.held.push_back(H);
            continue;
        }
        const double share = std::clamp(s.abundance, 0.0, 1.0);
        if (there > 0.55 + 0.45 * std::min(1.0, 3.0 * share)) continue;   // not in this square metre
        if (h.salt < s.saltLo - 1 || h.salt > s.saltHi + 1) continue;   // a salt it cannot bear
        if (h.meanC > s.tMax + 8 || h.meanC < s.tMin - 40) continue;   // killed by the heat; the cold only stops it
        double crowd = CROWD[h.niche] + std::log10(std::max(0.01, share)) + 1.0 * (spread - 0.5);
        if (h.salt < s.saltLo || h.salt > s.saltHi) crowd -= 2;          // washed in, a salt it bears and does not grow in
        if (h.shade && s.way == LW_LIGHT) crowd -= 1.5;                  // a light-eater in the shade
        if (h.bloom && s.niche == h.niche) crowd += 1.3;                 // its bloom
        if (s.state == SS_DORMANT) crowd = std::min(crowd, 4.0) - 1.0;    // spores and cysts keep few
        H.cells = std::pow(10.0, crowd);
        H.active = s.state == SS_LIVING && h.meanC >= s.tMin && h.meanC <= s.tMax && !(h.salt < s.saltLo || h.salt > s.saltHi);
        if (H.active) activeLog = std::max(activeLog, crowd);
        S.held.push_back(H);
    }
    // what the labelled release reads: a crowd at work strong, a few faint; a protocell's chemistry faint too; spores and fossils nothing
    S.activity = activeLog >= 5.5 ? VA_STRONG : (activeLog >= 2.0 || proto ? VA_FAINT : VA_NONE);
    if (B.stage == LS_STERILE && S.held.empty()) S.organics = h01(h.place ^ SALT_COOKED) < 0.3;
    return S;
}

double filmDensity(const Biota& B, int niche) {
    if (niche < 0 || niche >= NI_COUNT || !livingStage(B.stage)) return 0;
    double d = 0;
    for (const Microbe& s : B.species) if (s.state == SS_LIVING && (s.niches & nicheBit(niche))) d += s.niche == niche ? s.abundance : 0.5 * s.abundance;
    return std::min(1.0, 2.0 * d);
}

Vec3 filmGlowColour(const Biota& B) {
    double w[PG_COUNT] = {0};
    for (const Microbe& s : B.species) if (s.state == SS_LIVING) w[s.pigment] += s.abundance;
    int best = PG_NONE;
    for (int p = 0; p < PG_COUNT; p++) if (w[p] > w[best]) best = p;
    switch (best) {
        case PG_GREEN: case PG_BLUE_GREEN: return Vec3(0.92, 0.16, 0.20);   // chlorophyll's fluorescence, deep red
        case PG_PURPLE: case PG_ORANGE: case PG_RED: return Vec3(1.0, 0.56, 0.20);
        case PG_BROWN: case PG_BLACK: return Vec3(0.62, 0.70, 0.95);
        default: return Vec3(0.30, 0.95, 0.80);   // a chemotroph's flavins
    }
}
