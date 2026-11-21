// J-02: the made world's hash and its spreads (galaxy/made.h)
#include "made.h"
#include "core/noise.h"
#include "core/rng.h"
#include <cstring>

namespace {
uint64_t hashOf(uint64_t h, double v) { uint64_t b; std::memcpy(&b, &v, 8); return mix64(h ^ (b + 0x9E3779B97F4A7C15ULL)); }
}

uint64_t madeKey(const MadeWorld& m) {
    uint64_t h = mix64(0x3ADE000000000001ULL ^ (uint64_t)m.step);
    h = mix64(h ^ (uint64_t)m.lines * 0x9E37ULL);
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeSpread& s = m.spread[i];
        h = hashOf(h, s.place.x); h = hashOf(h, s.place.y); h = hashOf(h, s.place.z); h = hashOf(h, s.reach);
        h = mix64(h ^ ((uint64_t)s.waters << 8 | (uint64_t)s.land << 4 | (uint64_t)s.looks)); h = hashOf(h, s.tLo); h = hashOf(h, s.tHi);   // J-03
    }
    h = mix64(h ^ (uint64_t)(m.cover + 1) * 0xC2B2AE3D27D4EB4FULL);
    if (m.cover >= 0) h = hashOf(h, m.coverAlbedo);
    if (m.tinted) { h = hashOf(h, m.tint.r); h = hashOf(h, m.tint.g); h = hashOf(h, m.tint.b); }
    h = mix64(h ^ ((uint64_t)m.tintOn << 16 | (uint64_t)m.dye << 4 | (uint64_t)m.sunless));   // J-03
    if (m.glow > 0) { h = hashOf(h, m.glow); h = hashOf(h, m.glowTint.r); h = hashOf(h, m.glowTint.g); h = hashOf(h, m.glowTint.b); }
    h = hashOf(h, m.tempK); h = hashOf(h, m.freezeC);
    if (m.aired() || m.runaway > 0) {   // J-04 (a layer with no air keeps J-03's key)
        auto rgbOf = [&](const RGB& c) { h = hashOf(h, c.r); h = hashOf(h, c.g); h = hashOf(h, c.b); };
        h = mix64(h ^ ((uint64_t)m.air << 20 | (uint64_t)(m.ozone + 1) << 16 | (uint64_t)m.cloudDye << 8 | (uint64_t)m.rainDye << 4 | (uint64_t)m.cloudGlowForm));
        h = hashOf(h, m.warmC); h = hashOf(h, m.pressureK); h = hashOf(h, m.haze); rgbOf(m.hazeTint); h = hashOf(h, m.thick); h = hashOf(h, m.sulphur);
        h = hashOf(h, m.wilt); h = hashOf(h, m.runaway); h = hashOf(h, m.cloudMore); h = hashOf(h, m.rainMore); h = hashOf(h, m.snowMore);
        h = hashOf(h, m.cloudTintK); rgbOf(m.cloudTint); h = hashOf(h, m.cloudGlow); rgbOf(m.cloudGlowTint); h = hashOf(h, m.rainTintK); rgbOf(m.rainTint);
        h = hashOf(h, m.seaSmoke); h = hashOf(h, m.spores); rgbOf(m.sporeTint);
    }
    bool spreads05 = false;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) spreads05 = spreads05 || m.spread[i].j05();
    if (m.j05() || spreads05) {   // J-05 (a layer without its waters', ground's or plants' looks keeps J-04's key)
        auto rgbOf = [&](const RGB& c) { h = hashOf(h, c.r); h = hashOf(h, c.g); h = hashOf(h, c.b); };
        auto lookOf = [&](const MadeLook& L) {
            h = mix64(h ^ ((uint64_t)L.on << 40 | (uint64_t)L.tinted << 36 | (uint64_t)L.dye << 28 | (uint64_t)L.kind << 8 | (uint64_t)L.variant));
            if (L.tinted) rgbOf(L.tint);
            h = hashOf(h, L.albedo);
        };
        for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
            const MadeSpread& s = m.spread[i];
            h = mix64(h ^ ((uint64_t)s.makes << 24 | (uint64_t)s.plants << 20 | (uint64_t)s.landOn << 12 | (uint64_t)s.lookWaters << 4));
            h = mix64(h ^ (uint64_t)(s.cover + 1) * 0x9E3779B97F4A7C15ULL); h = hashOf(h, s.coverAlbedo); h = hashOf(h, s.coldC);
        }
        h = mix64(h ^ ((uint64_t)m.seas << 40 | (uint64_t)m.grounds << 20 | (uint64_t)m.flora << 8 | (uint64_t)m.waterLook));
        h = hashOf(h, m.foam); rgbOf(m.foamTint); h = hashOf(h, m.jelly); h = hashOf(h, m.boil); h = hashOf(h, m.seaShade); rgbOf(m.seaShadeTint);
        h = hashOf(h, m.sour); lookOf(m.land); lookOf(m.snow); lookOf(m.plant);
        h = hashOf(h, m.plantShare); h = hashOf(h, m.plantGlow); rgbOf(m.plantGlowTint); h = hashOf(h, m.galls); h = hashOf(h, m.rot);
        h = hashOf(h, m.rustGround); h = hashOf(h, m.lush); h = hashOf(h, m.bleached);
    }
    return h ? h : 1;
}

double madeCapShare(double reach) { return reach <= 0 ? 0.0 : (reach >= PI ? 1.0 : (1 - std::cos(reach)) / 2); }

double madeReachOfStep(int step, double radiusKm) {
    if (step <= 0 || radiusKm <= 0) return 0;
    const double whole = PI * radiusKm;   // the antipode's distance
    double km;
    if (step <= 6) km = 2.0 * std::pow(75.0, (step - 1) / 5.0);    // 2 km, then the region's 150 km by step 6
    else km = 150.0 + (whole - 150.0) * (step - 6) / (double)(MADE_STEPS - 6);
    return std::min(PI, km / radiusKm);
}

double madeSpreadAt(const MadeWorld& m, int i, const Vec3& unit, uint64_t seed) {
    const MadeSpread& s = m.spread[i];
    if (s.reach <= 0) return 0;
    if (s.reach >= PI) return 1;   // the whole world
    const double c = dot(unit, s.place);
    if (c < std::cos(std::min(PI, 1.3 * s.reach))) return 0;   // past the furthest its ragged edge goes
    const double d = std::acos(clampd(c, -1, 1));
    // the edge wanders by a quarter of the reach, in lobes a quarter of it across (the noise's normalised sum stays within one)
    const double edge = s.reach * (1 + 0.25 * fbm3(unit * (4.0 / s.reach), hashCombine(seed, 0x3ADE10 + (uint64_t)i), 3));
    const double band = 0.03 * s.reach;
    return 1 - smoothstep(edge - band, edge + band, d);
}

double madeReachAt(const MadeWorld& m, const Vec3& unit, uint64_t seed) {
    double best = 0;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        best = std::max(best, madeSpreadAt(m, i, unit, seed));
        if (best >= 1) break;
    }
    return best;
}

bool madeLives(const MadeSpread& s, bool wet, int salt, double meanC) {
    if (wet ? !(s.waters & (1 << clampi(salt, 0, 2))) : !s.land) return false;
    return meanC > s.tLo && meanC < s.tHi;
}
