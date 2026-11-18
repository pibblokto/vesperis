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

JarRecipe jarRecipe(const Biota& B, const Sample& s, const Microhabitat& h) {
    JarRecipe r;
    switch (B.chemistry) {
        case CH_ACID: r.water = RGB(0.75f, 0.7f, 0.36f); break;
        case CH_AMMONIA: r.water = RGB(0.66f, 0.7f, 0.78f); break;
        case CH_HYDROCARBON: r.water = RGB(0.62f, 0.46f, 0.26f); break;
        default: r.water = RGB(0.5f, 0.62f, 0.68f); break;
    }
    r.silt = h.niche == NI_MAT ? RGB(0.36f, 0.31f, 0.21f) : (h.niche == NI_SOIL ? RGB(0.34f, 0.25f, 0.17f) : RGB(0.4f, 0.35f, 0.29f));
    double next[6] = {0, 0.12, 0.05, 0.2, 0.45, 0.7};   // each layer's next free depth: the oxygen-makers, the others of the light, iron, sulphide, the deep, the foot
    for (const Held& H : s.held) {
        if (H.state != SS_LIVING || H.species < 0 || H.species >= (int)B.species.size()) continue;
        const Microbe& m = B.species[H.species];
        int layer = -1, tone = m.pigment;
        RGB col = pigmentColour(m.pigment);
        if (m.way == LW_LIGHT) layer = m.oxygen >= 1 ? 0 : 1;
        else if (m.way == LW_CHEMICAL) {
            layer = m.substrate == SU_IRON ? 2 : (m.substrate == SU_SULPHIDE ? 3 : 4);
            if (m.pigment == PG_NONE) {
                tone = m.substrate == SU_IRON ? JT_RUST : (m.substrate == SU_SULPHIDE ? JT_SULPHUR : JT_GREY);
                col = tone == JT_RUST ? RGB(0.62f, 0.3f, 0.12f) : (tone == JT_SULPHUR ? RGB(0.88f, 0.86f, 0.74f) : RGB(0.38f, 0.37f, 0.35f));
            }
        } else if (m.way == LW_DEAD && m.oxygen == 0) { layer = 5; if (m.pigment == PG_NONE) { col = RGB(0.07f, 0.07f, 0.07f); tone = JT_BLACK; } }
        if (tone == PG_NONE) tone = JT_GREY;
        if (layer < 0) continue;   // the air-breathing heterotrophs and the grazers make no band of their own
        if (m.way == LW_LIGHT && (m.niches & nicheBit(NI_LIQUID))) r.water = mixRGB(r.water, col, 0.3f);   // the floating ones tint the water
        const double th = clampd(0.04 + 0.03 * (std::log10(std::max(1.0, H.cells)) - 4), 0.03, layer == 5 ? 0.3 : 0.16);
        JarBand b;
        b.top = std::min(0.97, next[layer]); b.bottom = std::min(1.0, b.top + th);
        next[layer] = b.bottom;
        b.col = col; b.tone = tone; b.species = H.species;
        const double depth = 0.5 * (b.top + b.bottom);
        b.fullS = clampd(360 + 720 * depth + 120 * (h01(m.seed ^ SALT_JAR) - 0.5), 300, JAR_SETTLED_S - 30);
        r.bands.push_back(b);
    }
    std::stable_sort(r.bands.begin(), r.bands.end(), [](const JarBand& a, const JarBand& b) { return a.top < b.top; });
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
