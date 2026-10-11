// C-12: graves and names (`graves.h`)
#include "graves.h"
#include "buildings.h"
#include "peoples.h"
#include "../core/rng.h"
#include <cmath>
#include <algorithm>

namespace {
// B-414: a death in the years of the age the people reached (`Lore::ageFrom`) while the place stood (one left early: before it was left)
int diedIn(const Lore& L, const RuinSpec& r, Rng& rng) {
    const int end = L.endYear > 0 ? L.endYear : L.spanYears + 1, from = std::max(1, std::min(L.ageFrom, L.spanYears));
    const int to = std::max(from, std::min(L.spanYears, end - (int)std::lround(std::max(0.0, r.before))));
    return from + rng.irange(to - from + 1);
}
}

void gravesOf(const Lore& L, const RuinSpec& r, const Culture& c, std::vector<Grave>& out) {
    out.clear();
    if (r.sclass == SC_WORKS) {   // T-01: a line's pylons hold no grave; T-03: a work's plots (a necropolis's, a battlefield's war graves), rows of stones
        if (r.work < 0) return;
        Rng rng(r.seed ^ 0x6A4E7ULL);
        int k = 0;
        for (const FieldPatch& f : r.fields) {
            if (f.kind != 0) continue;   // FI_PLOT (galaxy/towns.h)
            const double fx = std::sin(f.heading), fz = std::cos(f.heading), sx = std::cos(f.heading), sz = -std::sin(f.heading);
            const int cols = std::max(1, (int)(2 * f.hw / 2.2)), rows = std::max(1, (int)(2 * f.hd / 2.6));
            for (int j = 0; j < rows && out.size() < 420; j++)
                for (int i = 0; i < cols && out.size() < 420; i++) {
                    const bool gap = rng.chance(0.12);
                    const double a = -f.hw + (i + 0.5) * 2 * f.hw / cols + rng.sym(0.15), b = -f.hd + (j + 0.5) * 2 * f.hd / rows + rng.sym(0.15);
                    Grave g;
                    g.x = f.x + sx * a + fx * b; g.z = f.z + sz * a + fz * b; g.heading = std::fmod(f.heading + rng.sym(0.05) + 2 * TAU, TAU);
                    g.height = 0.7 + 0.4 * rng.uni();
                    g.fallen = rng.chance(c.decay * 0.6);
                    g.died = r.founded > 0 ? r.founded : diedIn(L, r, rng);   // a battle's dead all in its year
                    g.born = g.died - (r.founded > 0 ? 17 + rng.irange(30) : 20 + rng.irange(70));
                    if (g.born < 1) g.born = 0;
                    g.name = personName(L, rng.next());
                    g.id = mix64(r.seed ^ 0x6A4E8ULL ^ ((uint64_t)(++k) * 0x9E3779B97F4A7C15ULL));
                    if (gap || !settlementClear(r, g.x, g.z, 0.8)) continue;
                    out.push_back(g);
                }
        }
        return;
    }
    Rng rng(r.seed ^ 0x6A4E5ULL);
    int want = 0;
    switch (r.sclass) {
        case SC_HAMLET: want = 1 + rng.irange(3); break;
        case SC_VILLAGE: want = 3 + rng.irange(4); break;
        case SC_TOWN: want = 6 + rng.irange(7); break;
        case SC_CITY: want = 10 + rng.irange(9); break;      // T-01
        case SC_CAPITAL: want = 14 + rng.irange(11); break;
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
    for (int k = 0; k < want; k++) {
        int row = k / perRow, col = k % perRow;
        double off = (col - (perRow - 1) * 0.5) * 2.2 + rng.sym(0.3), outM = R0 + row * 2.6 + rng.sym(0.3);
        double x = fx * outM + sx * off, z = fz * outM + sz * off;
        Grave g;
        g.x = x; g.z = z; g.heading = std::fmod(dir + PI + rng.sym(0.08) + 2 * TAU, TAU);   // the face toward the settlement
        g.height = 0.7 + 0.5 * rng.uni();
        g.fallen = rng.chance(c.decay * 0.6);
        g.died = diedIn(L, r, rng);
        g.born = g.died - (30 + rng.irange(61));
        if (g.born < 1) g.born = 0;
        uint64_t nameSeed = rng.next();
        g.name = personName(L, nameSeed);
        for (int tries = 0; tries < 8; tries++) {   // distinct within the settlement
            bool dup = false; for (const Grave& o : out) if (o.name == g.name) dup = true;
            if (!dup) break;
            g.name = personName(L, mix64(nameSeed ^ (uint64_t)(tries + 1) * 0x9E3779B97F4A7C15ULL));
        }
        g.id = mix64(r.seed ^ 0x6A4E6ULL ^ ((uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL));
        if (!settlementClear(r, x, z, 1.0)) continue;   // a fallen stretch of wall, a cairn, a cistern's channel: the stone is skipped (after its draws, so the rest stay)
        out.push_back(g);
    }
}

void graveElements(const std::vector<Grave>& graves, const Culture& c, int lod, std::vector<Form>& out, bool stonesOnly) {
    if (lod != 0 || graves.empty()) return;
    if (stonesOnly) {   // T-03: a work's plots: its own tombs and walls stand as its buildings
        for (const Grave& g : graves) {
            Form e; e.kind = FK_BOX; e.part = 5; e.building = -1; e.heading = g.heading;
            if (!g.fallen) { e.x = g.x; e.z = g.z; e.hx = 0.32; e.hz = 0.08; e.y0 = 0; e.y1 = g.height; e.glyphs = true; }
            else { double fx = std::sin(g.heading), fz = std::cos(g.heading); e.x = g.x + fx * (g.height * 0.5 + 0.1); e.z = g.z + fz * (g.height * 0.5 + 0.1); e.hx = 0.32; e.hz = g.height * 0.5; e.y0 = 0; e.y1 = 0.16; }
            out.push_back(e);
        }
        return;
    }
    const int dead = c.genome.of(GV_DEAD);
    const int mat = c.age == AGE_PREHISTORIC ? MT_DRYSTONE : (c.age == AGE_MODERN ? MT_CONCRETE : (c.age >= AGE_FUTURISTIC ? MT_COMPOSITE : (c.family == 1 ? MT_EARTH : MT_MASONRY)));
    const double D = materialDecay(mat, c.ageYears / std::max(0.1, c.life), c.wet), De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet);
    auto piece = [&](int kind, double x, double z, double heading, double hx, double hz, double y0, double y1, int part, int m) -> Form& {
        Form e; e.kind = kind; e.x = x; e.z = z; e.heading = heading; e.hx = hx; e.hz = hz; e.y0 = y0; e.y1 = y1; e.part = part; e.building = -1; e.material = m;
        out.push_back(e); return out.back();
    };
    // A-04: the people's way with its dead round C-01's stones: a barrow behind each (earth, no higher than a step), a tomb (a small
    // house of the age's stone, open toward its stone), a low wall round the cemetery, a tower of silence (a ring of high wall round them,
    // its gap toward the settlement)
    double cx = 0, cz = 0, rr = 0;
    for (const Grave& g : graves) { cx += g.x; cz += g.z; }
    cx /= graves.size(); cz /= graves.size();
    for (const Grave& g : graves) rr = std::max(rr, std::hypot(g.x - cx, g.z - cz));
    const double back = std::atan2(cx, cz);   // from the settlement out to the ground
    for (const Grave& g : graves) {
        const double bx = std::sin(g.heading), bz = std::cos(g.heading);   // the stone's face looks toward the settlement: behind it lies the dead
        if (dead == DE_BARROWS) { Form& m = piece(FK_DOME, g.x - bx * 1.6, g.z - bz * 1.6, g.heading, 1.3, 1.3, 0, std::max(0.3, 0.75 * (1 - 0.5 * De)), 8, MT_EARTH); m.sub = 1; }
        if (dead == DE_TOMBS && D < 0.85) {
            const double h = 1.6 * (D < 0.4 ? 1 : 1 - 0.6 * D);
            piece(FK_BOX, g.x - bx * 1.5, g.z - bz * 1.5, g.heading, 0.75, 0.9, 0, h, 0, mat);
            if (D < 0.4) { Form& r = piece(FK_SWEEP, g.x - bx * 1.5, g.z - bz * 1.5, g.heading + PI / 2, 0, 1.0, h, h + 0.6, 2, mat); r.sub = 1; r.p[1] = 0.95; r.p[2] = 0.1; r.slot = c.genome.accentPlace() == AP_ROOFS ? 1 : 0; }
        }
    }
    if (dead == DE_CEMETERIES || dead == DE_TOWERS) {   // round the ground: a low wall, or the tower's high one
        const double R = rr + (dead == DE_TOWERS ? 3.0 : 2.4), H = dead == DE_TOWERS ? 5.5 * (D < 0.3 ? 1 : 1 - 0.7 * D) : 0.85;
        const int N = std::max(12, (int)(TAU * R / 2.2));
        for (int k = 0; k < N; k++) {
            const double a = (k + 0.5) * TAU / N;
            if (std::fabs(std::remainder(a - back - PI, TAU)) < (dead == DE_TOWERS ? 0.32 : 0.25)) continue;   // the gap toward the settlement
            if (dead == DE_TOWERS && D > 0.4 && std::fmod(k * 0.618, 1.0) < D - 0.4) continue;   // stretches fallen
            Form& w = piece(FK_BOX, cx + std::sin(a) * R, cz + std::cos(a) * R, a, R * std::sin(PI / N) + 0.3, 0.35, 0, std::max(0.4, H), dead == DE_TOWERS ? 0 : 4, mat);
            w.material = mat;
        }
    }
    for (const Grave& g : graves) {
        Form e; e.kind = FK_BOX; e.part = 5; e.building = -1; e.heading = g.heading;
        if (!g.fallen) { e.x = g.x; e.z = g.z; e.hx = 0.32; e.hz = 0.08; e.y0 = 0; e.y1 = g.height; e.glyphs = true; }
        else { double fx = std::sin(g.heading), fz = std::cos(g.heading); e.x = g.x + fx * (g.height * 0.5 + 0.1); e.z = g.z + fz * (g.height * 0.5 + 0.1); e.hx = 0.32; e.hz = g.height * 0.5; e.y0 = 0; e.y1 = 0.16; e.glyphs = false; }
        out.push_back(e);
    }
}

std::string graveLine(const Lore& L, const Grave& g) {
    if (g.born > 0) return g.name + ", years " + std::to_string(g.born) + " to " + std::to_string(g.died) + " of " + L.city;
    return g.name + ", to year " + std::to_string(g.died) + " of " + L.city;
}
