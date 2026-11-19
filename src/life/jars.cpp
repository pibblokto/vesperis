// Z-05: a world in a jar (life/jars.h): its sample's living species in bands by what each lives on, coming in over twenty minutes
#include "life/jars.h"
#include "life/morphs.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr uint64_t SALT_JAR = 0x11FE57A6E0000030ULL;   // a band's own pace
double h01(uint64_t h) { return unitFromHash(mix64(h)); }
RGB mixRGB(const RGB& a, const RGB& b, float t) { return RGB(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t); }
}   // namespace

int jarLayerOf(const Microbe& m) {
    if (m.way == LW_LIGHT) return m.oxygen >= 1 ? JL_OXYGEN : JL_DARK_LIGHT;
    if (m.way == LW_CHEMICAL) {   // R-439: one that needs air where the air runs out, as the iron's
        if (m.substrate == SU_IRON) return JL_EDGE;
        if (m.substrate == SU_SULPHIDE) return JL_SULPHIDE;
        return m.oxygen >= 2 ? JL_EDGE : JL_DEEP;
    }
    if (m.way == LW_DEAD && m.oxygen == 0) return JL_FOOT;
    return -1;   // the air-breathing heterotrophs and the grazers make no band of their own
}

JarRecipe jarRecipe(const Biota& B, const Sample& s, const Microhabitat& h) {
    JarRecipe r;
    switch (B.chemistry) {
        case CH_ACID: r.water = RGB(0.75f, 0.7f, 0.36f); break;
        case CH_AMMONIA: r.water = RGB(0.66f, 0.7f, 0.78f); break;
        case CH_HYDROCARBON: r.water = RGB(0.62f, 0.46f, 0.26f); break;
        default: r.water = RGB(0.5f, 0.62f, 0.68f); break;
    }
    r.silt = h.niche == NI_MAT ? RGB(0.36f, 0.31f, 0.21f) : (h.niche == NI_SOIL ? RGB(0.34f, 0.25f, 0.17f) : RGB(0.4f, 0.35f, 0.29f));
    // R-439: each place's bands under the one before, never over it, each place no higher than its own top (the deep's under 0.45, the foot's
    // under 0.7): a band's thickness kept in `bottom` till they are laid out
    const double least[JL_COUNT] = {0, 0.05, 0.12, 0.2, 0.45, 0.7};
    std::vector<JarBand> at[JL_COUNT];
    for (const Held& H : s.held) {
        if (H.state != SS_LIVING || H.species < 0 || H.species >= (int)B.species.size()) continue;
        const Microbe& m = B.species[H.species];
        const int layer = jarLayerOf(m);
        if (layer < 0) continue;
        int tone = m.pigment;
        RGB col = pigmentColour(m.pigment);
        if (m.way == LW_CHEMICAL && m.pigment == PG_NONE) {   // the deep's grey a stone's, lighter than the silt (R-439: it showed on it no more)
            tone = m.substrate == SU_IRON ? JT_RUST : (m.substrate == SU_SULPHIDE ? JT_SULPHUR : JT_GREY);
            col = tone == JT_RUST ? RGB(0.62f, 0.3f, 0.12f) : (tone == JT_SULPHUR ? RGB(0.88f, 0.86f, 0.74f) : RGB(0.58f, 0.58f, 0.56f));
        } else if (layer == JL_FOOT && m.pigment == PG_NONE) { col = RGB(0.07f, 0.07f, 0.07f); tone = JT_BLACK; }
        if (tone == PG_NONE) tone = JT_GREY;
        if (m.way == LW_LIGHT && (m.niches & nicheBit(NI_LIQUID))) r.water = mixRGB(r.water, col, 0.3f);   // the floating ones tint the water
        JarBand b;
        b.bottom = clampd(0.04 + 0.03 * (std::log10(std::max(1.0, H.cells)) - 4), 0.03, layer == JL_FOOT ? 0.3 : 0.16);
        b.col = col; b.tone = tone; b.species = H.species; b.layer = layer; b.substrate = m.substrate;
        at[layer].push_back(b);
    }
    double under = 0;
    for (int l = 0; l < JL_COUNT; l++)
        for (JarBand& b : at[l]) {
            const double th = b.bottom;
            b.top = std::min(0.97, std::max(under, least[l])); b.bottom = std::min(1.0, b.top + th);
            under = b.bottom;
            const double depth = 0.5 * (b.top + b.bottom);
            b.fullS = clampd(360 + 720 * depth + 120 * (h01(B.species[b.species].seed ^ SALT_JAR) - 0.5), 300, JAR_SETTLED_S - 30);
            r.bands.push_back(b);
        }
    return r;
}

JarNow jarAt(const JarRecipe& r, double t) {
    JarNow n;
    t = std::max(0.0, t);
    n.cloud = std::exp(-t / 50);
    for (const JarBand& b : r.bands) {
        const double x = clampd((t - 0.35 * b.fullS) / (0.65 * b.fullS), 0, 1);
        n.strength.push_back(x * x * (3 - 2 * x));
    }
    return n;
}
