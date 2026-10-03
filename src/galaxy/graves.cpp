// C-12: graves and names (`graves.h`)
#include "graves.h"
#include "../core/rng.h"
#include <cmath>
#include <algorithm>

void gravesOf(const Lore& L, const RuinSpec& r, const Culture& c, std::vector<Grave>& out) {
    out.clear();
    if (r.kind != RK_SETTLEMENT) return;
    Rng rng(r.seed ^ 0x6A4E5ULL);
    int want = 0;
    switch (r.sclass) {
        case SC_HAMLET: want = 1 + rng.irange(3); break;
        case SC_VILLAGE: want = 3 + rng.irange(4); break;
        case SC_TOWN: want = 6 + rng.irange(7); break;
        default: want = rng.chance(0.33) ? 1 : 0; break;
    }
    if (want == 0) return;
    // the ground: beyond the wall and every building, on a heading away from the shore where there is one, else the seed's
    double reach = 0;
    for (const Building& b : r.buildings) reach = std::max(reach, std::sqrt(b.x * b.x + b.z * b.z) + std::sqrt(b.hw * b.hw + b.hd * b.hd));
    double R0 = std::max(r.size * 1.1, reach + 4.0) + 2.0;
    const SiteRead& st = r.site;
    double dir = st.shoreDist < 600 ? st.shoreDir + PI + rng.sym(PI / 3) : rng.uni() * TAU;
    double fx = std::sin(dir), fz = std::cos(dir), sx = std::cos(dir), sz = -std::sin(dir);
    int perRow = std::min(want, 3 + rng.irange(3));
    const int style = L.style;
    for (int k = 0; k < want; k++) {
        int row = k / perRow, col = k % perRow;
        double off = (col - (perRow - 1) * 0.5) * 2.2 + rng.sym(0.3), outM = R0 + row * 2.6 + rng.sym(0.3);
        double x = fx * outM + sx * off, z = fz * outM + sz * off;
        Grave g;
        g.x = x; g.z = z; g.heading = std::fmod(dir + PI + rng.sym(0.08) + 2 * TAU, TAU);   // the face toward the settlement
        g.height = 0.7 + 0.5 * rng.uni();
        g.fallen = rng.chance(c.decay * 0.6);
        g.died = 1 + rng.irange(std::max(1, L.spanYears));
        g.born = g.died - (30 + rng.irange(61));
        if (g.born < 1) g.born = 0;
        uint64_t nameSeed = rng.next();
        g.name = personName(style, nameSeed);
        for (int tries = 0; tries < 8; tries++) {   // distinct within the settlement
            bool dup = false; for (const Grave& o : out) if (o.name == g.name) dup = true;
            if (!dup) break;
            g.name = personName(style, mix64(nameSeed ^ (uint64_t)(tries + 1) * 0x9E3779B97F4A7C15ULL));
        }
        g.id = mix64(r.seed ^ 0x6A4E6ULL ^ ((uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL));
        if (!settlementClear(r, x, z, 1.0)) continue;   // a fallen stretch of wall, a cairn, a cistern's channel: the stone is skipped (after its draws, so the rest stay)
        out.push_back(g);
    }
}

void graveElements(const std::vector<Grave>& graves, int lod, std::vector<RuinElem>& out) {
    if (lod != 0) return;
    for (const Grave& g : graves) {
        RuinElem e; e.shape = 0; e.part = 5; e.building = -1; e.heading = g.heading;
        if (!g.fallen) { e.x = g.x; e.z = g.z; e.hx = 0.32; e.hz = 0.08; e.y0 = 0; e.y1 = g.height; e.glyphs = true; }
        else { double fx = std::sin(g.heading), fz = std::cos(g.heading); e.x = g.x + fx * (g.height * 0.5 + 0.1); e.z = g.z + fz * (g.height * 0.5 + 0.1); e.hx = 0.32; e.hz = g.height * 0.5; e.y0 = 0; e.y1 = 0.16; e.glyphs = false; }
        out.push_back(e);
    }
}

std::string graveLine(const Lore& L, const Grave& g) {
    if (g.born > 0) return g.name + ", years " + std::to_string(g.born) + " to " + std::to_string(g.died) + " of " + L.city;
    return g.name + ", to year " + std::to_string(g.died) + " of " + L.city;
}
