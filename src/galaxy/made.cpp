// J-02: the made world's hash and its spreads (galaxy/made.h)
#include "made.h"
#include "core/noise.h"
#include "core/rng.h"
#include <algorithm>
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
        if (s.deep != MADE_SURFACE) h = mix64(h ^ (uint64_t)s.deep * 0xDEE9ULL);   // (J-13: a surface line's key as before)
    }
    h = mix64(h ^ (uint64_t)(m.cover + 1) * 0xC2B2AE3D27D4EB4FULL);
    if (m.cover >= 0) h = hashOf(h, m.coverAlbedo);
    if (m.tinted) { h = hashOf(h, m.tint.r); h = hashOf(h, m.tint.g); h = hashOf(h, m.tint.b); }
    h = mix64(h ^ ((uint64_t)m.tintOn << 16 | (uint64_t)m.dye << 4 | (uint64_t)m.sunless));   // J-03
    if (m.glow > 0) { h = hashOf(h, m.glow); h = hashOf(h, m.glowTint.r); h = hashOf(h, m.glowTint.g); h = hashOf(h, m.glowTint.b); }
    h = hashOf(h, m.tempK); h = hashOf(h, m.freezeC);
    for (int i = 0; i < m.lines && i < MADE_LINES; i++)   // KI-460: each line's own waters' colour and glow
        if (m.spread[i].waterTinted || m.spread[i].glow > 0) {
            const MadeSpread& s = m.spread[i];
            h = hashOf(h, s.waterTint.r); h = hashOf(h, s.waterTint.g); h = hashOf(h, s.waterTint.b); h = mix64(h ^ (uint64_t)s.waterColours);
            h = hashOf(h, s.glow); h = hashOf(h, s.glowTint.r); h = hashOf(h, s.glowTint.g); h = hashOf(h, s.glowTint.b);
        }
    if (m.aired() || m.runaway > 0) {   // J-04 (a layer with no air keeps J-03's key)
        auto rgbOf = [&](const RGB& c) { h = hashOf(h, c.r); h = hashOf(h, c.g); h = hashOf(h, c.b); };
        h = mix64(h ^ ((uint64_t)m.air << 20 | (uint64_t)(m.ozone + 1) << 16 | (uint64_t)m.cloudDye << 8 | (uint64_t)m.rainDye << 4 | (uint64_t)m.cloudGlowForm));
        h = hashOf(h, m.warmC); h = hashOf(h, m.pressureK); h = hashOf(h, m.haze); rgbOf(m.hazeTint); h = hashOf(h, m.thick); h = hashOf(h, m.sulphur);
        h = hashOf(h, m.wilt); h = hashOf(h, m.runaway); h = hashOf(h, m.cloudMore); h = hashOf(h, m.rainMore); h = hashOf(h, m.snowMore);
        h = hashOf(h, m.cloudTintK); rgbOf(m.cloudTint); h = hashOf(h, m.cloudGlow); rgbOf(m.cloudGlowTint); h = hashOf(h, m.rainTintK); rgbOf(m.rainTint);
        h = hashOf(h, m.seaSmoke); h = hashOf(h, m.spores); rgbOf(m.sporeTint);
        if (m.silver > 0) h = hashOf(h, m.silver);   // (KI-483: a layer with none keeps its key)
        h = hashOf(h, m.worldC);
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
            lookOf(s.look[MC_LAND]); lookOf(s.look[MC_SNOW]); lookOf(s.look[MC_PLANTS]);   // KI-460: each line's own
        }
        h = mix64(h ^ ((uint64_t)m.seas << 40 | (uint64_t)m.grounds << 20 | (uint64_t)m.flora << 8 | (uint64_t)m.waterLook));
        h = hashOf(h, m.foam); rgbOf(m.foamTint); h = hashOf(h, m.jelly); h = hashOf(h, m.boil); h = hashOf(h, m.seaShade); rgbOf(m.seaShadeTint);
        h = hashOf(h, m.sour); lookOf(m.land); lookOf(m.snow); lookOf(m.plant);
        h = hashOf(h, m.plantShare); h = hashOf(h, m.plantGlow); rgbOf(m.plantGlowTint); h = hashOf(h, m.galls); h = hashOf(h, m.rot);
        if (m.landGlow > 0) { h = hashOf(h, m.landGlow); rgbOf(m.landGlowTint); }   // (J-16: a layer with none keeps its key)
        h = hashOf(h, m.rustGround); h = hashOf(h, m.lush); h = hashOf(h, m.bleached);
    }
    if (m.fated()) {   // J-08 (a layer without the animals' fates keeps J-05's key)
        auto fateOf = [&](const MadeFate& f) {
            h = mix64(h ^ ((uint64_t)f.fauna << 32 | (uint64_t)f.death << 24 | (uint64_t)f.sickness << 8));
            for (double v : f.harm) h = hashOf(h, v);
            h = hashOf(h, f.shelled); h = hashOf(h, f.starve); h = hashOf(h, f.sick); h = hashOf(h, f.changed); h = hashOf(h, f.thrive);
            h = mix64(h ^ ((uint64_t)f.steer << 16 | (uint64_t)f.dye | (uint64_t)f.age << 40));
        };
        for (int i = 0; i < m.lines && i < MADE_LINES; i++) fateOf(m.spread[i].fate);
        fateOf(m.fate);
        h = mix64(h ^ ((uint64_t)m.fauna << 20 | (uint64_t)m.fateAge << 4 | (uint64_t)m.rotFast));
        h = hashOf(h, m.radiusKm);
    }
    if (m.grown()) {   // J-09 (a layer with no overgrowth keeps J-08's key)
        auto rgbOf = [&](const RGB& c) { h = hashOf(h, c.r); h = hashOf(h, c.g); h = hashOf(h, c.b); };
        for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
            const MadeGrowth& G = m.spread[i].growth;
            h = mix64(h ^ ((uint64_t)G.type << 40 | (uint64_t)G.texture << 32 | (uint64_t)G.hard << 24 | (uint64_t)G.hair << 16 | (uint64_t)G.age << 8 |
                           (uint64_t)(G.material + 1)));
            rgbOf(G.colour); rgbOf(G.veins); rgbOf(G.glowTint);
            h = hashOf(h, G.grain); h = hashOf(h, G.wet); h = hashOf(h, G.glow); h = hashOf(h, G.reach);
            h = mix64(h ^ (uint64_t)G.steps * 0xD1B54A32D192ED03ULL);   // J-10
        }
        h = mix64(h ^ ((uint64_t)m.growths << 8 | (uint64_t)(m.growthLine + 1)));
        h = hashOf(h, m.radiusKm);
    }
    for (int i = 0; i < m.lines && i < MADE_LINES; i++)   // J-10: a stone rotter's steps (a layer with none keeps J-09's key)
        if (m.spread[i].pits > 0) h = mix64(h ^ ((uint64_t)(i + 1) << 56 | (uint64_t)m.spread[i].pits));
    if (m.kind != MDK_NONE || m.freeze > 0) {   // J-11 (a layer with no made kind and no ice running keeps J-10's key)
        h = mix64(h ^ ((uint64_t)m.kind << 48 | (uint64_t)m.kindGrowth << 40 | (uint64_t)m.kindCold << 36));
        h = hashOf(h, m.freeze);
        for (int i = 0; i < m.lines && i < MADE_LINES; i++) h = hashOf(h, m.spread[i].growth.land);
    }
    for (int i = 0; i < m.lines && i < MADE_LINES; i++)   // J-15: a carpet's grass and shrubs (a layer with none keeps J-11's key)
        if (m.spread[i].growth.greens) {
            const MadeGrowth& G = m.spread[i].growth;
            h = mix64(h ^ (uint64_t)(i + 1) * 0x6E15ULL); h = hashOf(h, G.greenReach); h = hashOf(h, G.thickReach);
            h = hashOf(h, G.plants.r); h = hashOf(h, G.plants.g); h = hashOf(h, G.plants.b);
        }
    if (m.green > 0) h = hashOf(h, m.green);
    return h ? h : 1;
}

// J-11: a made kind's words. An overgrown world's by its type (films lie by the waters alone and make none)
namespace {
const char* const KIND_NAMES[MDK_COUNT] = {"", "MADE HOTHOUSE", "MADE SNOWBALL", "HAZE WORLD", "PURPLE WORLD", "RUST WORLD", "OVERGROWN WORLD", "CHARNEL WORLD",
                                          "GREENED WORLD", "GREENED DESERT"};
const char* const GROWN_NAMES[OG_COUNT] = {"OVERGROWN WORLD", "FLESH WORLD", "FUNGUS WORLD", "SLIME MOULD WORLD", "LICHEN WORLD", "CORAL WORLD", "SPONGE WORLD",
                                           "HAIR WORLD", "BLADDER WORLD", "CARPET WORLD", "OVERGROWN WORLD"};
// what covers an overgrown world, as a sentence's subject and as a phrase's ("living tissue", "fungus", "bladders"), and whether it is many
const char* const GROWN_WHAT[OG_COUNT] = {"growth", "living tissue", "fungus", "slime mould", "lichen", "coral", "sponge", "living hair", "bladders",
                                          "carpets of cells", "films"};
bool grownMany(int type) { return type == OG_BLADDERS || type == OG_CARPETS || type == OG_FILMS; }
const char* const KIND_SHORT[MDK_COUNT] = {"", "HOTHOUSE", "SNOWBALL", "HAZE", "PURPLE", "RUST", "OVERGROWN", "CHARNEL", "GREENED", "GREENED"};
const char* const GROWN_SHORT[OG_COUNT] = {"OVERGROWN", "FLESH", "FUNGUS", "SLIME", "LICHEN", "CORAL", "SPONGE", "HAIR", "BLADDERS", "CARPETS", "OVERGROWN"};
}   // namespace

std::string madeKindName(const MadeWorld& m) {
    if (m.kind <= MDK_NONE || m.kind >= MDK_COUNT) return "";
    if (m.kind == MDK_OVERGROWN) return GROWN_NAMES[m.kindGrowth >= 0 && m.kindGrowth < OG_COUNT ? m.kindGrowth : OG_NONE];
    return KIND_NAMES[m.kind];
}

std::string madeKindShort(const MadeWorld& m) {
    if (m.kind <= MDK_NONE || m.kind >= MDK_COUNT) return "";
    if (m.kind == MDK_OVERGROWN) return GROWN_SHORT[m.kindGrowth >= 0 && m.kindGrowth < OG_COUNT ? m.kindGrowth : OG_NONE];
    return KIND_SHORT[m.kind];
}

std::string madeKindSentence(const MadeWorld& m) {
    switch (m.kind) {
        case MDK_HOTHOUSE: return m.kindFlora ? "Its seas boiled away into a thick hot air. Salt lies in the old basins, and its forests have turned to stone."
                                              : "Its seas boiled away into a thick hot air, and salt lies white on the old sea floor.";
        case MDK_SNOWBALL: return "Frozen from pole to pole: its seas are ice, its land lies under snow, and its forests stand dead and white.";
        case MDK_HAZE: return "A thick orange haze hides its sky. The light on the ground is dim and brown.";
        case MDK_PURPLE: return "Its seas have turned purple with sulphur bacteria, and its air smells of rotten eggs.";
        case MDK_RUST: return "Oxygen made by new life has rusted its rocks and sands red.";
        case MDK_OVERGROWN: {
            const int t = m.kindGrowth >= 0 && m.kindGrowth < OG_COUNT ? m.kindGrowth : OG_NONE;
            std::string w = GROWN_WHAT[t];
            w[0] = (char)(w[0] - 'a' + 'A');
            return w + (grownMany(t) ? " cover" : " covers") + " the land from coast to coast.";
        }
        case MDK_CHARNEL: return "Every animal and plant on it has died, and their remains lie everywhere.";
        case MDK_GREENED: return m.kindCold ? "Once cold, it is warmer now: its ice has drawn back and its plants are spreading."
                                           : "Once dry, it has rain now, and its plants are spreading.";
        case MDK_GREENED_DESERT: return m.kindGrowth == OG_LICHEN ? "Its lichen has grown into grass and shrubs over most of its bare land."
                                                                  : "A carpet of cells has grown into grass and shrubs over most of its bare land.";
        default: return "";
    }
}

std::string madeKindPhrase(const MadeWorld& m) {
    switch (m.kind) {
        case MDK_HOTHOUSE: return m.kindFlora ? "world whose seas boiled away under a thick hot air, its old seabeds white with salt and its forests turned to stone"
                                              : "world whose seas boiled away under a thick hot air, its old sea floor white with salt";
        case MDK_SNOWBALL: return "world frozen from pole to pole, its seas ice and its forests standing dead and white with frost";
        case MDK_HAZE: return "world dim under a thick orange haze";
        case MDK_PURPLE: return "world of purple seas under an air that smells of sulphide";
        case MDK_RUST: return "world of rusted red ground under a new oxygen sky";
        case MDK_OVERGROWN: return std::string("world covered from coast to coast in ") + GROWN_WHAT[m.kindGrowth >= 0 && m.kindGrowth < OG_COUNT ? m.kindGrowth : OG_NONE];
        case MDK_CHARNEL: return "dead world where every animal and plant has died, their remains everywhere";
        case MDK_GREENED: return m.kindCold ? "world once cold, made warmer, its ice drawn back and its plants spreading" : "world once dry, made wetter, its plants spreading";
        case MDK_GREENED_DESERT: return m.kindGrowth == OG_LICHEN ? "desert world greened by a lichen, grass and shrubs over most of its land"
                                                                  : "desert world greened by a carpet of cells, grass and shrubs over most of its land";
        default: return "";
    }
}

double madeGrowthAt(const MadeWorld& m, const Vec3& unit, uint64_t seed, int& line) {
    line = -1;
    if (!m.grown()) return 0;
    double best = 0;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeGrowth& G = m.spread[i].growth;
        if (!G.on()) continue;
        double k;
        if (G.reach >= PI) k = 1;   // the whole world
        else {
            const double R = std::max(m.radiusKm, 1.0), fingers = 0.35 / R;   // (radians: 350 m)
            const double c = dot(unit, m.spread[i].place);
            if (c < std::cos(std::min(PI, 1.3 * G.reach + 2 * fingers))) continue;
            const double d = std::acos(clampd(c, -1, 1));
            // its lobes a world's fortieth across, its fingers a few hundred metres, both fixed to the place (the noise's sums stay within one): the
            // edge only moves out as its reach grows, so what it covered stays covered
            const double edge = G.reach * (1 + 0.25 * fbm3(unit * 40.0, hashCombine(seed, 0x6E0E10 + (uint64_t)i), 3)) +
                                fingers * fbm3(unit * (1.0 / fingers), hashCombine(seed, 0x6E0E20 + (uint64_t)i), 3);
            const double band = std::max(0.02 * G.reach, 0.2 * fingers);
            k = 1 - smoothstep(edge - band, edge + band, d);
        }
        if (k >= best && k > 0) { best = k; line = i; }
    }
    return best;
}

double madeGrowthSince(const MadeWorld& m, const Vec3& unit, uint64_t seed) {
    if (!m.grown()) return -1;
    double bestK = 0, since = -1;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeGrowth& G = m.spread[i].growth;
        if (!G.on()) continue;
        const double R = std::max(m.radiusKm, 1.0), fingers = 0.35 / R;
        const double c = dot(unit, m.spread[i].place);
        if (G.reach < PI && c < std::cos(std::min(PI, 1.3 * G.reach + 2 * fingers))) continue;
        const double d = std::acos(clampd(c, -1, 1));
        const double n1 = fbm3(unit * 40.0, hashCombine(seed, 0x6E0E10 + (uint64_t)i), 3), n2 = fbm3(unit * (1.0 / fingers), hashCombine(seed, 0x6E0E20 + (uint64_t)i), 3);
        double k = 1;
        if (G.reach < PI) {   // (madeGrowthAt's edge)
            const double edge = G.reach * (1 + 0.25 * n1) + fingers * n2, band = std::max(0.02 * G.reach, 0.2 * fingers);
            k = 1 - smoothstep(edge - band, edge + band, d);
        }
        if (k < 0.5 || k < bestK) continue;
        bestK = k;
        // the first step its edge stood past the point, and how far past: the steps since, from that step's reach
        double prev = fingers * n2, s = -1;
        for (int a = 1; a <= G.age; a++) {
            const double e = madeReachOfStep(a, R) * (1 + 0.25 * n1) + fingers * n2;
            if (e >= d || madeReachOfStep(a, R) >= PI) { s = G.steps - a + clampd((e - d) / std::max(1e-12, e - prev), 0, 1); break; }
            prev = e;
        }
        since = std::max(0.0, s);
    }
    return since;
}

// J-16 (KI-510): a bare world with air, no plants or seas of its own (a desert, a rust, a salt, a thin-aired world and the rest): its grass in
// its line's own bank, never the world's grass bank (KI-509)
bool madeGreens(int type) { return KINDS[type].landable && KINDS[type].atmosphere && !kindHas(type, KT_FLORA) && !kindHas(type, KT_SEA); }

double madeGreenAt(const MadeWorld& m, int line, const Vec3& unit, uint64_t seed, double& thick) {
    thick = 0;
    if (line < 0 || line >= m.lines || line >= MADE_LINES) return 0;
    const MadeGrowth& G = m.spread[line].growth;
    if (!G.on() || !G.greens || G.greenReach <= 0) return 0;
    const double R = std::max(m.radiusKm, 1.0), fingers = 0.35 / R;   // (madeGrowthAt's edge, at the plants' reaches)
    const double c = dot(unit, m.spread[line].place);
    double n1 = 0, n2 = 0;
    const bool noise = G.greenReach < PI || G.thickReach < PI;
    if (noise) {
        if (G.greenReach < PI && c < std::cos(std::min(PI, 1.3 * G.greenReach + 2 * fingers))) return 0;
        n1 = fbm3(unit * 40.0, hashCombine(seed, 0x6E0E10 + (uint64_t)line), 3); n2 = fbm3(unit * (1.0 / fingers), hashCombine(seed, 0x6E0E20 + (uint64_t)line), 3);
    }
    const double d = noise ? std::acos(clampd(c, -1, 1)) : 0.0;
    auto within = [&](double reach) {
        if (reach >= PI) return 1.0;
        if (reach <= 0) return 0.0;
        const double edge = reach * (1 + 0.25 * n1) + fingers * n2, band = std::max(0.02 * reach, 0.2 * fingers);
        return 1 - smoothstep(edge - band, edge + band, d);
    };
    const double k = within(G.greenReach);
    if (k > 0) thick = within(G.thickReach);
    return k;
}

MadeFate madeFateAt(const MadeWorld& m, const Vec3& unit, uint64_t seed) {
    MadeFate f = m.fate;
    if (!m.fated()) return f;
    if (!m.fate.on()) f.age = 0;
    double spare[MFC_COUNT], spareShelled = 1 - f.shelled, spareStarve = 1 - f.starve, spareSick = 1 - f.sick, spareChanged = 1 - f.changed;
    for (int c = 0; c < MFC_COUNT; c++) spare[c] = 1 - f.harm[c];
    double worst = 0;   // (the most harmful line's death, unless the air's is worse)
    for (int c = 0; c < MFC_COUNT; c++) worst = std::max(worst, f.harm[c]);
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeFate& L = m.spread[i].fate;
        if (!L.on()) continue;
        const double k = madeSpreadAt(m, i, unit, seed);
        if (k <= 0) continue;
        double most = 0;
        for (int c = 0; c < MFC_COUNT; c++) { spare[c] *= 1 - k * L.harm[c]; most = std::max(most, k * L.harm[c]); }
        spareShelled *= 1 - k * L.shelled; spareStarve *= 1 - k * L.starve;
        most = std::max(most, std::max(k * L.shelled, k * L.starve));
        if (most > worst && L.death != MD_NONE) { worst = most; f.death = L.death; }
        if (L.sickness && k * L.sick > 0) { f.sickness = L.sickness; spareSick *= 1 - k * L.sick; }
        if (L.changed > 0) { spareChanged *= 1 - k * L.changed; f.steer |= L.steer; }
        if (L.dye) f.dye = L.dye;
        f.thrive = std::max(f.thrive, k * L.thrive);
        if (k >= 0.5) f.fauna |= L.fauna;
        {   // its age here: from the first step its spread reached the point
            const double d = std::acos(clampd(dot(unit, m.spread[i].place), -1, 1));
            int arrive = 0;
            while (arrive < MADE_STEPS && madeReachOfStep(arrive, m.radiusKm) < d) arrive++;
            f.age = std::max(f.age, std::max(0, L.age - std::max(0, arrive - 2)));
        }
    }
    for (int c = 0; c < MFC_COUNT; c++) f.harm[c] = 1 - spare[c];
    f.shelled = 1 - spareShelled; f.starve = 1 - spareStarve; f.sick = 1 - spareSick; f.changed = 1 - spareChanged;
    return f;
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
    if (s.deep != MADE_SURFACE) return !wet && (s.deep == MADE_UNDER_ICE || s.deep == MADE_DEEP_ROCK);   // J-13: its look on the cracks over its sea, whatever
    // their warmth; J-16 (KI-506): a rock line's on the seeps its water comes up through (a cloud line's in the air alone)
    if (wet ? !(s.waters & (1 << clampi(salt, 0, 2))) : !s.land) return false;
    return meanC > s.tLo && meanC < s.tHi;
}
