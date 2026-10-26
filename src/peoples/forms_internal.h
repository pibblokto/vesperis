#pragma once
// E-10: forms.cpp's small helpers, for its parts alone (forms, forms_colliders): a form's hash, a broken box's steps, a fence's
// pales (inline, as they were in their one file: the colliders run as the explorer walks)
#include "forms.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

inline double H01(uint32_t seed, int k) { return unitFromHash(mix64(((uint64_t)seed * 0x9E3779B97F4A7C15ULL) ^ (0xA02F00000000ULL + (uint64_t)(k + 1) * 0xD1B54A32D192ED03ULL))); }

// a broken box stands in steps along its longer side (the mesh's and, B-420, the colliders'): how many, and each one's top
inline int boxSteps(const Form& e) {
    const double L = std::max(e.hx, e.hz), W = e.hz >= e.hx ? e.hx : e.hz;
    int steps = e.broken > 0 ? clampi((int)std::lround(L / std::max(0.8, 2.2 * W)), 1, 6) : 1;
    if (e.broken > 0 && steps == 1 && L > 1.2) steps = 2;
    return steps;
}
inline double boxStepTop(const Form& e, int i) {
    const double H = e.y1 - e.y0;
    const double top = e.y1 - (e.broken > 0 ? e.broken * H * (0.15 + 0.85 * H01(e.seed, i)) : 0.0);
    return std::max(top, e.y0 + std::min(0.3, H));
}

// R-412: a fence's pales (`FK_FENCE`): as many as its spacing fits along it (wattle's hurdles about 1.6 m), each standing unless a broken
// fence lost it, alone or in a stretch of four
inline int fencePales(const Form& e) {
    if (e.sub == 2) return std::max(1, (int)std::lround(2 * e.hz / 1.6));
    const double sp = e.p[0] > 0 ? e.p[0] : (e.sub == 1 ? 0.14 : (e.sub == 3 ? 0.3 : 0.22));
    return clampi((int)std::lround(2 * e.hz / std::max(0.05, sp)), 1, 400);
}
inline bool fenceStanding(const Form& e, int k) { return !(e.broken > 0 && (H01(e.seed, 300 + k / 4) < 0.6 * e.broken || H01(e.seed, 700 + k) < 0.3 * e.broken)); }
