// E-09: a work's pieces, laid out as a settlement's (`workSpec`), and its line in the harness's listings (`workLine`)
#include "works_internal.h"

// ---- a work's pieces --------------------------------------------------------------------------------------------------------------
namespace {
struct Lay {
    RuinSpec& r; const Work& w; Rng rng; int n = 0;
    Lay(RuinSpec& rr, const Work& ww) : r(rr), w(ww), rng(ww.seed ^ 0x5BEC0ULL) {}
    // B-421: a building's doorway in its front (its heading's side): a keep, a house, a hall, a tower of the works stood with none, and the
    // records the rules laid in their rooms were shut in (C-01's settlements draw their own)
    Building& add(int kind, double x, double z, double heading, double hw, double hd, double h, int style = 0, int age = -2) {
        Building b; b.kind = kind; b.x = x; b.z = z; b.heading = heading; b.hw = hw; b.hd = hd; b.hWall = h; b.thick = 0.5; b.door = 0;
        b.age = age == -2 ? w.age : age; b.style = style; b.seed = mix64(w.seed ^ (0xB01D0000ULL + (uint64_t)(n++) * 0x9E3779B97F4A7C15ULL));
        b.luck = (unitFromHash(mix64(b.seed ^ 0x1CCULL)) - 0.5) * 0.5; b.before = w.before;
        r.buildings.push_back(b);
        return r.buildings.back();
    }
    // the work's metres of a point (across, along) in a frame of heading `h` about (cx, cz)
    void at(double h, double cx, double cz, double a, double b, double& x, double& z) const { double dx, dz; rot(h, a, b, dx, dz); x = cx + dx; z = cz + dz; }
    void plot(double x, double z, double heading, double hw, double hd) {
        FieldPatch f;
        f.x = x;
        f.z = z;
        f.heading = heading;
        f.hw = hw;
        f.hd = hd;
        f.kind = FI_PLOT;
        f.age = w.age;
        r.fields.push_back(f);
    }
};
}

void workSpec(const BodyGen& gSeason, const Work& w, RuinSpec& r) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    r = RuinSpec();
    r.sclass = SC_WORKS; r.plan = SP_LONE; r.lat = w.lat; r.lon = w.lon; r.heading = w.heading; r.size = w.radiusM;
    r.seed = w.seed; r.people = w.people; r.work = w.kind; r.buildAge = w.age; r.before = w.before;
    r.site.desert = kindHas(g.type, KT_DRY);
    { const int now = nowKind(g); r.site.drift = kindHas(now, KT_DRY) ? 1 : (KINDS[now].cover == DC_FROZEN ? 2 : 0); }   // the present's drifts
    r.site.wind = g.windAngle + (w.lat < 0 ? PI : 0) + (int)std::floor(std::fabs(w.lat) / (30 * DEG)) * 0.9;
    const double Rm = g.R * 1000.0;
    const Frame F(w.unit, Rm);
    DrainageOff off;
    auto ground = [&](double x, double z) { return sampleSurface(g, F.unit(x, z), 64.0).height; };
    auto wet = [&](double x, double z) { return wetS(sampleSurface(g, F.unit(x, z), 64.0)); };
    Lay L(r, w);
    Rng& rng = L.rng;
    const double H = w.heading;
    switch (w.kind) {
        case WO_QUARRY: {
            if (w.style == 0) {   // the flint pits' spoil, a ring round each
                for (const GroundEdit& e : w.edits) L.add(BK_MOUND, e.x, e.z, 0, e.r + 2.4, 1.5, 0.9, 4);
                break;
            }
            if (w.style >= 4) {   // a pit: a modern open pit's mill at its rim, its spoil and the trucks on its floor; a medieval stone pit's blocks
                                  // on its floor and its derrick at the rim; an industrial pit's, its shed and its heap
                const GroundEdit& e = w.edits[0];
                double x, z;
                const double r0 = std::max(6.0, e.hw);
                if (w.style == 4) {
                    L.at(H, 0, 0, 0, -(e.r + 60), x, z); L.add(BK_HANGAR, x, z, H, 16, 26, 18, 1);
                    L.at(H, 0, 0, e.r + 110, 30, x, z); L.add(BK_MOUND, x, z, H, 70, 45, 11, 5);
                    for (int k = 0; k < 3; k++) {
                        const double a = rng.uni() * TAU, d = r0 * 0.6 * rng.uni();
                        L.add(BK_VEHICLE, std::sin(a) * d, std::cos(a) * d, rng.uni() * TAU, 3.4, 5.6, 5.5, 1, AGE_MODERN);
                    }
                } else {
                    const bool iron = w.style == 6;
                    for (int k = 0; k < (iron ? 3 : 2); k++) {
                        const double a = rng.uni() * TAU, d = r0 * 0.5 * rng.uni();
                        L.add(BK_BLOCKS, std::sin(a) * d, std::cos(a) * d, rng.uni() * PI, 4.5, 3.2, 1.5);
                    }
                    L.at(H, 0, 0, 0, -(e.r + 3), x, z); L.add(BK_DERRICK, x, z, H + PI, 1, 1, iron ? 16 : 9, iron ? 1 : 0);
                    L.at(H, 0, 0, e.r + 10, -(e.r * 0.4), x, z);
                    if (iron) { L.add(BK_HALL, x, z, H + PI, 6, 10, 5.5, 0, AGE_INDUSTRIAL); L.at(H, 0, 0, -(e.r + 22), 0, x, z); L.add(BK_MOUND, x, z, 0, 15, 15, 10, 2); }
                    else L.add(BK_HOUSE, x, z, H + PI, 2.6, 3.2, 2.8, 0, AGE_MEDIEVAL);
                }
                break;
            }
            const GroundEdit& e = w.edits[0];
            const double step = e.step > 0 ? e.step : 2 * e.hd;
            double x, z;
            const int nb = w.style == 1 ? 2 : 3;
            for (int k = 0; k < nb; k++) { L.at(H, 0, 0, (k - (nb - 1) * 0.5) * e.hw * 0.55, -e.hd + step * 0.35, x, z); L.add(BK_BLOCKS, x, z, H + rng.sym(0.3), 4.5, 3.2, 1.5); }
            L.at(H, 0, 0, e.hw * 0.2, -e.hd + step * 0.7, x, z); L.add(BK_DERRICK, x, z, H + rng.sym(0.6), 1, 1, w.style == 1 ? 9 : 16, w.style == 1 ? 0 : 1);
            if (w.style == 1) { L.at(H, 0, 0, e.hw + 9, -e.hd - 9, x, z); L.add(BK_HOUSE, x, z, H + PI, 2.6, 3.2, 2.8, 0, AGE_MEDIEVAL); }
            else if (w.style == 2) {
                L.at(H, 0, 0, e.hw + 14, -e.hd - 16, x, z); L.add(BK_HALL, x, z, H + PI, 6, 10, 5.5, 0, AGE_INDUSTRIAL);
                L.at(H, 0, 0, -e.hw - 22, -e.hd - 14, x, z); L.add(BK_MOUND, x, z, 0, 15, 15, 10, 2);
            } else {
                L.at(H, 0, 0, e.hw + 36, -e.hd - 34, x, z); L.add(BK_HANGAR, x, z, H + PI, 14, 22, 16, 1);
                L.at(H, 0, 0, -e.hw - 60, -e.hd - 25, x, z); L.add(BK_MOUND, x, z, H, 45, 32, 10, 5);
                for (int k = 0; k < 2; k++) {
                    L.at(H, 0, 0, rng.sym(e.hw * 0.6), -e.hd + step * (0.3 + 0.4 * rng.uni()), x, z);
                    L.add(BK_VEHICLE, x, z, rng.uni() * TAU, 3.4, 5.6, 5.5, 1, AGE_MODERN);
                }
            }
            break;
        }
        case WO_MINE: {
            double x, z;
            if (w.style == 0) {   // adits along the hillside, the spoil tipped below each, a hut
                const int n = 2 + rng.irange(3);
                for (int k = 0; k < n; k++) {
                    const double a = (k - (n - 1) * 0.5) * (28 + 14 * rng.uni());
                    L.at(H, 0, 0, a, 0, x, z); L.add(BK_PORTAL, x, z, H + PI, 1.6, 1, 3.2);
                    L.at(H, 0, 0, a + rng.sym(4), -14, x, z); L.add(BK_MOUND, x, z, H, 5 + 3 * rng.uni(), 8 + 4 * rng.uni(), 1.6 + 1.6 * rng.uni(), 6);
                }
                L.at(H, 0, 0, 0, -40, x, z); L.add(BK_HOUSE, x, z, H + PI, 2.8, 3.5, 3, 0, AGE_MEDIEVAL);
            } else if (w.style == 1) {   // headframes over their shafts, the great spoil heap, the miners' houses
                const int n = 1 + rng.irange(3);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 75, 0, x, z); L.add(BK_HEADFRAME, x, z, H + rng.sym(0.3), 13, 16, 24 + 8 * rng.uni()); }
                L.at(H, 0, 0, 110 + 40 * rng.uni(), -60, x, z); L.add(BK_MOUND, x, z, 0, 28 + 18 * rng.uni(), 28, 20 + 12 * rng.uni(), 2);
                for (int k = 0; k < 4; k++) { L.at(H, 0, 0, -90 + k * 14, -95, x, z); L.add(BK_HOUSE, x, z, H + PI, 3, 4, 5, 0, AGE_INDUSTRIAL); }
            } else {   // the open pit, a headframe and the mill at its rim, the tailings, trucks on its floor
                const GroundEdit& e = w.edits[0];
                L.at(H, 0, 0, -e.r * 0.4, -(e.r + 50), x, z); L.add(BK_HEADFRAME, x, z, H, 13, 16, 34);
                L.at(H, 0, 0, e.r * 0.35, -(e.r + 70), x, z); L.add(BK_HANGAR, x, z, H + PI, 18, 30, 20, 1);
                L.at(H, 0, 0, e.r + 160, 0, x, z); L.add(BK_MOUND, x, z, H, 110, 80, 12, 5);
                for (int k = 0; k < 3; k++) {
                    const double a = rng.uni() * TAU, d = e.r * 0.25 * rng.uni();
                    L.add(BK_VEHICLE, std::sin(a) * d, std::cos(a) * d, rng.uni() * TAU, 3.4, 5.6, 5.5, 1, AGE_MODERN);
                }
            }
            break;
        }
        case WO_DAM: {
            const double foot = w.style == 2 ? 0.13 * w.height : (w.style == 0 ? 0.38 * w.height : (w.style == 3 ? 0.3 * w.height : 0.42 * w.height));
            Building& d = L.add(BK_DAM, 0, 0, H, w.length * 0.5 + 6, std::max(4.0, foot), w.crest - (w.floor - 6), w.style);
            d.base = w.floor - 6;
            if (w.style >= 1) {   // the power house at its foot downstream
                double x, z; L.at(H, 0, 0, 0, -(foot + 28), x, z);
                L.add(BK_HALL, x, z, H + PI, 12, 18, 12, 0);
            }
            break;
        }
        case WO_AQUEDUCT: {
            double sx, sz, ex, ez; F.local(w.sideA, sx, sz); F.local(w.sideB, ex, ez);
            const double len = std::hypot(ex - sx, ez - sz), hd = std::atan2(ex - sx, ez - sz);
            const int n = std::max(2, (int)(len / 40));
            std::vector<double> gr(n + 1);
            for (int k = 0; k <= n; k++) { const double t = (double)k / n; gr[k] = ground(sx + (ex - sx) * t, sz + (ez - sz) * t); }
            auto chan = [&](double t) { return w.level + (w.crest - w.level) * t; };
            int k = 0;
            while (k < n) {   // runs of the line where the channel stands over the ground: arcades of up to sixty metres
                if (chan((k + 0.5) / n) - std::min(gr[k], gr[k + 1]) < 3.5) { k++; continue; }
                int k1 = k + 1;
                while (k1 < n && k1 - k < 2 && chan((k1 + 0.5) / n) - std::min(gr[k1], gr[k1 + 1]) >= 3.5) k1++;
                const double t0 = (double)k / n, t1 = (double)k1 / n, tm = 0.5 * (t0 + t1);
                double lo = 1e18; for (int q = k; q <= k1; q++) lo = std::min(lo, gr[q]);
                const double top = chan(tm);
                Building& b = L.add(BK_AQUEDUCT, sx + (ex - sx) * tm, sz + (ez - sz) * tm, hd, 1.3, 0.5 * len * (t1 - t0), top - (lo - 1.5), 0);
                b.base = lo - 1.5;
                k = k1;
            }
            L.add(BK_CISTERN, sx, sz, hd + PI, 3.5, 3.5, 3, 0);   // the spring's house
            L.add(BK_CISTERN, ex, ez, hd, 4.5, 4.5, 4, 0);        // the castellum at the town's edge
            break;
        }
        case WO_OBSERVATORY: {
            double x, z;
            switch (w.style) {
                case 0:
                    L.add(BK_PLATFORM, 0, 0, H, 9, 9, 3);
                    L.at(H, 0, 0, 0, -18, x, z); L.add(BK_TOWER, x, z, H, 3.6, 3.6, 13);
                    for (int k = 0; k < 8; k++) { const double a = k * TAU / 8; L.add(BK_STELA, std::sin(a) * 26, std::cos(a) * 26, a + PI, 1, 1, 2.6, MN_STELAE); }
                    break;
                case 1: {
                    const int n = 1 + rng.irange(3);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 22, 0, x, z); L.add(BK_OBSERVATORY, x, z, H, 4 + 3 * rng.uni(), 4, 7 + 3 * rng.uni()); }
                    L.at(H, 0, 0, 0, -30, x, z); L.add(BK_VILLA, x, z, H, 6, 8, 7, 0, AGE_INDUSTRIAL);
                    break;
                }
                case 2: {
                    const int n = 1 + rng.irange(4);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 80, 40 + 30 * (k % 2), x, z); L.add(BK_DISH, x, z, H + rng.sym(1), 8 + 22 * rng.uni(), 1, 1); }
                    L.at(H, 0, 0, -30, -25, x, z); L.add(BK_OBSERVATORY, x, z, H, 9, 9, 13, 0, AGE_MODERN);
                    L.at(H, 0, 0, 35, -30, x, z); L.add(BK_HALL, x, z, H, 8, 14, 8, 0, AGE_MODERN);
                    break;
                }
                default: {
                    const int n = 8 + rng.irange(5);
                    for (int k = 0; k < n; k++) { const double a = k * TAU / n; L.add(BK_DISH, std::sin(a) * 85, std::cos(a) * 85, a, 7 + 4 * rng.uni(), 1, 1, 1); }
                    L.add(BK_POD, 0, 0, H, 14, 14, 9);
                    break;
                }
            }
            break;
        }
        case WO_SHRINE: {
            const int a = w.age; double x, z;
            switch (w.style) {
                case HOLY_CAVE:
                    L.add(BK_PORTAL, 0, 0, H, 3.2, 1, 4.2);
                    for (int s = -1; s <= 1; s += 2) { L.at(H, 0, 0, s * 6, 3, x, z); L.add(BK_STELA, x, z, H, 1, 1, 2.4, a == AGE_PREHISTORIC ? MN_STELAE : MN_OBELISKS); }
                    break;
                case HOLY_SPRING:
                    L.add(BK_CISTERN, 0, 0, H, 4, 4, 1.8);
                    L.at(H, 0, 0, 0, -17, x, z);   // its doorway facing the spring, a way between (at 12 m the cistern stood a metre before it)
                    if (a == AGE_PREHISTORIC) L.add(BK_STONE_CIRCLE, x, z, H, 6, 6, 1.8);
                    else L.add(BK_TEMPLE, x, z, H, 5, 7, 6);
                    break;
                case HOLY_GROVE:
                    for (int k = 0; k < 8; k++) { const double t = k * TAU / 8; L.add(BK_STELA, std::sin(t) * 13, std::cos(t) * 13, t + PI, 1, 1, 2.2, MN_STELAE); }
                    L.add(BK_PLATFORM, 0, 0, H, 2.2, 2.2, 1.1);
                    break;
                default:   // a summit or the open sky
                    if (a == AGE_PREHISTORIC) { L.add(BK_STONE_CIRCLE, 0, 0, H, w.style == HOLY_OPEN_SKY ? 15 : 8, 8, 2.6); L.at(H, 0, 0, 0, -14, x, z); L.add(BK_CAIRN, x, z, 0, 1.6, 1.6, 2);
                    } else if (a == AGE_MEDIEVAL) {
                        L.add(BK_TEMPLE, 0, 0, H, 6, 9, 7);
                        for (int k = 0; k < 3; k++) {
                            L.at(H, 0, 0, (k - 1) * 7, 16, x, z);
                            L.add(BK_STELA, x, z, H + PI, 1, 1, 2.4, MN_STELAE);
                        }
                    } else if (a == AGE_INDUSTRIAL) {
                        L.add(BK_CHURCH, 0, 0, H, 5, 10, 8);
                        L.at(H, 0, 0, 0, 18, x, z);
                        L.add(BK_STELA, x, z, H, 1, 1, 6, MN_OBELISKS);
                    } else {
                        L.add(BK_PLATFORM, 0, 0, H, 10, 10, 1.6);
                        L.add(BK_STELA, 0, 0, H, 1, 1, 12, MN_OBELISKS);
                    }
                    break;
            }
            break;
        }
        case WO_WAY: {   // a cairn or a stela every 600 m along the track, to one side and then the other
            double run = 0, next = 300; int side = 1;
            for (const GroundEdit& e : w.edits) {
                for (size_t q = 2; q + 1 < e.pts.size(); q += 2) {
                    const double x0 = e.pts[q - 2], z0 = e.pts[q - 1], x1 = e.pts[q], z1 = e.pts[q + 1], seg = std::hypot(x1 - x0, z1 - z0);
                    while (run + seg >= next) {
                        const double t = (next - run) / std::max(seg, 1e-6), x = x0 + (x1 - x0) * t, z = z0 + (z1 - z0) * t, hd = std::atan2(x1 - x0, z1 - z0);
                        const double ox = std::cos(hd) * 4 * side, oz = -std::sin(hd) * 4 * side;
                        if (w.age == AGE_PREHISTORIC) L.add(BK_CAIRN, x + ox, z + oz, 0, 1.0, 1.0, 1.4); else L.add(BK_STELA, x + ox, z + oz, hd + PI / 2 * side, 1, 1, 1.8, MN_STELAE);
                        next += 600; side = -side;
                    }
                    run += seg;
                }
            }
            break;
        }
        case WO_NECROPOLIS: {
            double x, z;
            switch (w.style) {
                case 0: {   // barrows along the slope, a long barrow, standing stones
                    const int n = 6 + rng.irange(10);
                    for (int k = 0; k < n; k++) {
                        L.at(H, 0, 0, (k - (n - 1) * 0.5) * 22 + rng.sym(6), rng.sym(20), x, z);
                        const double rr = 4 + 8 * rng.uni();
                        L.add(BK_MOUND, x, z, 0, rr, rr, 1.4 + rr * 0.25, 0);
                    }
                    L.at(H, 0, 0, rng.sym(40), 50, x, z); L.add(BK_MOUND, x, z, H + PI / 2 + rng.sym(0.3), 6 + 3 * rng.uni(), 20 + 12 * rng.uni(), 2.5, 1);
                    for (int k = 0; k < 1 + rng.irange(3); k++) {   // standing stones; B-414: a later people's own stelae
                        L.at(H, 0, 0, rng.sym(80), -30 + rng.sym(10), x, z);
                        const double turn = rng.uni() * TAU;
                        if (w.age == AGE_PREHISTORIC) L.add(BK_MENHIR, x, z, turn, 0.8, 0, 3.6, 0); else L.add(BK_STELA, x, z, turn, 1, 1, 3.6, MN_STELAE);
                    }
                    break;
                }
                case 1: {   // tombs cut into the slope above, the houses of the dead below, the graves between
                    const int nt = 3 + rng.irange(6);
                    for (int k = 0; k < nt; k++) { L.at(H, 0, 0, (k - (nt - 1) * 0.5) * 16, 45 + rng.sym(5), x, z); L.add(BK_TOMB, x, z, H, 2.4 + 1.4 * rng.uni(), 1, 4 + 3 * rng.uni(), 2); }
                    const int nm = 2 + rng.irange(5);
                    for (int k = 0; k < nm; k++) {
                        L.at(H, 0, 0, (k - (nm - 1) * 0.5) * 18, -10 + rng.sym(4), x, z);
                        L.add(BK_TOMB, x, z, H, 2.2 + 0.8 * rng.uni(), 3 + rng.uni(), 3.4 + 1.6 * rng.uni(), rng.irange(2));
                    }
                    const int np = 2 + rng.irange(3);
                    for (int k = 0; k < np; k++) { L.at(H, 0, 0, (k - (np - 1) * 0.5) * 32, -45, x, z); L.plot(x, z, H, 12, 14); }
                    break;
                }
                case 2: {   // a walled garden of graves: its gate toward the city, the tombs along its middle walk, obelisks, a chapel at its head
                    const double hw = 70 + 40 * rng.uni(), hd = 50 + 30 * rng.uni();
                    const int perSide = std::max(3, (int)(hw / 20));
                    for (int s = 0; s < 4; s++) {
                        const bool across = s % 2 == 0;
                        const double half = across ? hw : hd, off = across ? hd : hw;
                        const int m = across ? perSide : std::max(2, (int)(hd / 20));
                        for (int q = 0; q < m; q++) {
                            const double t = -half + (q + 0.5) * 2 * half / m;
                            if (s == 0 && std::fabs(t) < 8) continue;   // the gate's gap
                            const double lx = across ? t : (s == 1 ? off : -off), lz = across ? (s == 0 ? -off : off) : t;
                            L.at(H, 0, 0, lx, lz, x, z);
                            L.add(BK_WALL, x, z, across ? H : H + PI / 2, half / m + 0.4, 0.35, 2.4);
                        }
                    }
                    L.at(H, 0, 0, 0, -hd, x, z); L.add(BK_GATE, x, z, H, 4, 1.2, 5);
                    const int nm = 6 + rng.irange(9);
                    for (int k = 0; k < nm; k++) {
                        const int side = k % 2 ? 1 : -1;
                        L.at(H, 0, 0, side * 10, -hd + 18 + (k / 2) * (2 * hd - 36) / std::max(1, nm / 2), x, z);
                        L.add(BK_TOMB, x, z, H + (side > 0 ? -PI / 2 : PI / 2), 2.2, 2.8, 3.6 + 1.5 * rng.uni(), rng.irange(2));
                    }
                    for (int k = 0; k < 2 + rng.irange(4); k++) { L.at(H, 0, 0, rng.sym(hw * 0.7), rng.sym(hd * 0.6), x, z); L.add(BK_STELA, x, z, H, 1, 1, 3 + 3 * rng.uni(), MN_OBELISKS); }
                    L.at(H, 0, 0, 0, hd - 16, x, z); L.add(BK_CHURCH, x, z, H + PI, 5, 9, 8);
                    for (int s = -1; s <= 1; s += 2) for (int q = 0; q < 2; q++) { L.at(H, 0, 0, s * (hw * 0.55), -hd * 0.45 + q * hd * 0.8, x, z); L.plot(x, z, H, hw * 0.35, hd * 0.32); }
                    break;
                }
                case 3: {   // the modern cemetery: a grid of plots, a columbarium, a memorial
                    for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) { L.at(H, 0, 0, (i - 1.5) * 36, (j - 1.5) * 26, x, z); L.plot(x, z, H, 15, 10); }
                    L.at(H, 0, 0, 0, 72, x, z); L.add(BK_TOMB, x, z, H + PI, 34, 5, 4, 3);
                    L.at(H, 0, 0, 0, -68, x, z); L.add(BK_STELA, x, z, H, 1, 1, 12, MN_OBELISKS);
                    break;
                }
                case 4: {   // a futuristic people's field of memorial pillars
                    for (int j = 0; j < 6; j++) for (int i = 0; i < 6; i++) { L.at(H, 0, 0, (i - 2.5) * 14, (j - 2.5) * 14, x, z); L.add(BK_STELA, x, z, H, 1, 1, 2.2 + rng.uni(), MN_PILLARS); }
                    break;
                }
                default: {   // towers of silence on the slope
                    const int n = 1 + rng.irange(3);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 45, rng.sym(12), x, z); L.add(BK_TOMB, x, z, H, 8 + 3 * rng.uni(), 8, 7 + 4 * rng.uni(), 4); }
                    break;
                }
            }
            break;
        }
        case WO_GEOGLYPH: {   // cairns at the rays' centre
            for (int k = 0; k < 2; k++) L.add(BK_CAIRN, w.sideA.x + (k ? 6 : -4), w.sideA.z + (k ? -3 : 5), 0, 1.4, 1.4, 1.6, 0);
            break;
        }
        case WO_HENGE: {
            double x, z;
            switch (w.style) {
                case 0: {   // the circle within its ditch and bank, its avenue out of the entrance, barrows round it
                    const double R = w.width > 0 ? w.width : 50;
                    L.add(BK_MOUND, 0, 0, 0, R + 9, 3.6, 1.6, 4);
                    L.add(BK_STONE_CIRCLE, 0, 0, H, R * 0.5, R * 0.5, 3.4);
                    L.at(H, 0, 0, 0, R + 75, x, z); L.add(BK_MENHIR, x, z, H, 1, 60, 2.6, 1);
                    for (int k = 0; k < 2 + rng.irange(3); k++) {
                        const double a = rng.uni() * TAU, d = R + 60 + 90 * rng.uni();
                        const double rr = 5 + 6 * rng.uni();
                        L.add(BK_MOUND, std::sin(a) * d, std::cos(a) * d, 0, rr, rr, 1.6 + rr * 0.2, 0);
                    }
                    break;
                }
                case 1: {   // rows of standing stones across the land
                    const int rows = 3 + rng.irange(6);
                    const double half = 120 + 150 * rng.uni();
                    for (int k = 0; k < rows; k++) {
                        L.at(H, 0, 0, (k - (rows - 1) * 0.5) * 11, rng.sym(15), x, z);
                        L.add(BK_MENHIR, x, z, H, 1, half * (0.75 + 0.25 * rng.uni()), 2.2 + rng.uni(), 0);
                    }
                    L.at(H, 0, 0, 0, half + 30, x, z); L.add(BK_STONE_CIRCLE, x, z, H, 14, 14, 2.4);
                    break;
                }
                case 2: {   // the great circle: its lintelled ring, the horseshoe of trilithons, the heel stone
                    L.add(BK_STONE_CIRCLE, 0, 0, H, 16, 16, 4.4);
                    for (int k = 0; k < 5; k++) { const double a = H + (k - 2) * 0.55 + PI; L.add(BK_TRILITHON, std::sin(a) * 8.5, std::cos(a) * 8.5, a, 2.6, 0.7, 5.5 + (k == 2 ? 1.5 : 0)); }
                    L.at(H, 0, 0, 0, 42, x, z); L.add(BK_MENHIR, x, z, H, 1.1, 0, 4.6, 0);
                    break;
                }
                default: {   // a field of dolmens and barrows
                    for (int k = 0; k < 4 + rng.irange(7); k++) {
                        const double a = rng.uni() * TAU, d = 30 + 170 * rng.uni();
                        L.add(BK_DOLMEN, std::sin(a) * d, std::cos(a) * d, rng.uni() * TAU, 2.4, 3.4, 2);
                    }
                    for (int k = 0; k < 3; k++) {
                        const double a = rng.uni() * TAU, d = 40 + 160 * rng.uni();
                        const double rr = 6 + 5 * rng.uni();
                        L.add(BK_MOUND, std::sin(a) * d, std::cos(a) * d, 0, rr, rr, 2, 0);
                    }
                    break;
                }
            }
            break;
        }
        case WO_FORT_LINE: {
            const double ax = w.sideA.x, az = w.sideA.z, bx = w.sideB.x, bz = w.sideB.z, len = std::hypot(bx - ax, bz - az), brg = std::atan2(bx - ax, bz - az);
            auto pt = [&](double t, double off, double& x, double& z) { x = ax + (bx - ax) * t + std::cos(brg) * off; z = az + (bz - az) * t - std::sin(brg) * off; };
            double x, z;
            if (w.style == 0) {   // a wall in stretches with its towers, a fort at the middle of every other stretch
                const int n = std::max(2, (int)(len / 40));
                for (int k = 0; k < n; k++) {
                    const double t = (k + 0.5) / n; pt(t, 0, x, z);
                    if (wet(x, z)) continue;
                    if (k % 8 == 4) L.add(BK_WALL_TOWER, x, z, brg, 4, 4, 10);
                    else L.add(BK_WALL, x, z, brg - PI / 2, len / n * 0.5 + 0.3, 1.2, 6.5);
                }
                if (w.group % 2 == (uint32_t)(w.seed % 2)) {
                    pt(0.5, -45, x, z);
                    if (!wet(x, z)) {
                        L.add(BK_TOWER, x, z, brg, 6, 6, 14);
                        for (int q = 0; q < 4; q++) {
                            const double a = q * TAU / 4 + brg;
                            L.add(BK_WALL, x + std::sin(a) * 22, z + std::cos(a) * 22, a, 22, 0.9, 5);
                        }
                    }
                }   // a wall's length runs across its heading: a ring's stretch faces out
            } else if (w.style == 1) {   // the rampart behind the ditch, a redoubt
                pt(0.5, -2, x, z); L.add(BK_MOUND, x, z, brg, 5, len * 0.5, 3.2, 3);
                pt(0.5, -70, x, z); if (!wet(x, z)) L.add(BK_FORT, x, z, brg, 32, 32, 3, 0);
            } else {   // dragon's teeth behind the ditch, pillboxes, a blockhouse
                const int n = std::max(2, (int)(len / 60));
                for (int k = 0; k < n; k++) { pt((k + 0.5) / n, -4, x, z); if (!wet(x, z)) L.add(BK_TEETH, x, z, brg, 4, len / n * 0.5, 1.2); }
                for (int k = 0; k < (int)(len / 420); k++) { pt((k + 0.5) / (len / 420), -30, x, z); if (!wet(x, z)) L.add(BK_BUNKER, x, z, brg + PI / 2, 3.4, 3.4, 2.4, 0); }
                pt(0.5, -80, x, z); if (!wet(x, z)) L.add(BK_BUNKER, x, z, brg, 9, 7, 4, 1);
            }
            break;
        }
        case WO_BATTLEFIELD: {
            double x, z;
            const int a = w.age;
            r.founded = w.year;   // its war graves' dead all of the battle's year
            if (a == AGE_PREHISTORIC) {
                const int n = 8 + rng.irange(18);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(160), rng.sym(90), x, z); L.add(BK_CAIRN, x, z, rng.uni() * TAU, 1.0 + 0.8 * rng.uni(), 1.0 + 0.6 * rng.uni(), 1.5); }
                L.add(BK_MENHIR, 0, 0, H, 0.9, 0, 3.4, 0);
            } else if (a == AGE_MEDIEVAL) {
                const int n = 2 + rng.irange(5);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(140), rng.sym(70), x, z); const double rr = 6 + 8 * rng.uni(); L.add(BK_MOUND, x, z, 0, rr, rr, 2 + rr * 0.18, 0); }
                L.at(H, 0, 0, 0, -20, x, z); L.add(BK_STELA, x, z, H, 1, 1, 3.2, MN_STELAE);
            } else if (a == AGE_INDUSTRIAL || a == AGE_MODERN) {
                L.at(H, 0, 0, 220, -170, x, z); L.add(BK_STELA, x, z, H, 1, 1, a == AGE_INDUSTRIAL ? 8 : 11, MN_OBELISKS);
                for (int q = 0; q < 2; q++) { L.at(H, 0, 0, 220 + (q ? 26 : -26), -150, x, z); L.plot(x, z, H, 9, 13); }
                if (a == AGE_MODERN) {
                    const int n = 4 + rng.irange(9);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(260), rng.sym(70), x, z); L.add(BK_VEHICLE, x, z, H + rng.sym(1.2) + (rng.uni() < 0.4 ? PI : 0), 1.8, 3.6, 2.4, 0); }
                    if (rng.uni() < 0.6) { L.at(H, 0, 0, rng.sym(200), rng.sym(150), x, z); L.add(BK_AIRCRAFT, x, z, rng.uni() * TAU, 7, 6.5, 2, 0); }
                    for (int k = 0; k < 1 + rng.irange(3); k++) { L.at(H, 0, 0, rng.sym(200), 95, x, z); L.add(BK_BUNKER, x, z, H + PI, 3.4, 3.4, 2.4, 0); }
                }
            } else {
                const int n = 2 + rng.irange(5);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(250), rng.sym(120), x, z); L.add(BK_WRECK, x, z, rng.uni() * TAU, 3 + 3 * rng.uni(), 8 + 6 * rng.uni(), 3, 0); }
                L.at(H, 0, 0, 0, -200, x, z); L.add(BK_STELA, x, z, H, 1, 1, 6, MN_PILLARS);
            }
            break;
        }
        case WO_LAUNCH: {
            double x, z;
            if (w.style == 0) {
                for (const GroundEdit& e : w.edits) if (e.kind == GE_FLAT) {
                    L.add(BK_LAUNCHPAD, e.x, e.z, H, 55, 55, 70 + 40 * rng.uni());
                    L.at(H, e.x, e.z, 70, -45, x, z); L.add(BK_TANKS, x, z, H, 22, 7, 12);
                }
                L.at(H, 0, 0, 0, -380, x, z); L.add(BK_HANGAR, x, z, H, 45, 65, 95 + 30 * rng.uni(), 0);
                L.at(H, 0, 0, 160, 300, x, z); L.add(BK_BUNKER, x, z, H, 10, 7, 4, 1);
            } else if (w.style == 1) {   // a mass driver: its spans rising east from the hall
                const int n = 22;
                for (int k = 0; k < n; k++) {
                    const double t = (k + 0.5) / n, along = -1100 + 2300 * t, h = 8 + 520 * t * t;
                    L.at(H, 0, 0, 0, along, x, z); L.add(BK_MAGLEV, x, z, H, 3, 2300.0 / n * 0.5, h, 0);
                }
                L.at(H, 0, 0, 0, -1180, x, z); L.add(BK_HANGAR, x, z, H, 40, 60, 60, 0);
            } else {
                L.add(BK_TETHER, 0, 0, H, 70, 70, 380, 0, AGE_FUTURISTIC);
                for (int k = 0; k < 4; k++) { const double a = H + k * TAU / 4 + PI / 4; L.add(BK_HANGAR, std::sin(a) * 200, std::cos(a) * 200, a, 30, 45, 50, 0); }
            }
            break;
        }
        case WO_ANTENNA: {
            double x, z;
            if (w.style == 0) {
                const int n = 4 + rng.irange(5);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, 0, (k - (n - 1) * 0.5) * 200, x, z); L.add(BK_MAST, x, z, H, 2, 2, 120 + 80 * rng.uni()); }
                L.at(H, 0, 0, 90, 0, x, z); L.add(BK_HALL, x, z, H, 8, 14, 8, 0, AGE_INDUSTRIAL);
            } else if (w.style == 1) {
                const int m = 3 + rng.irange(3); const double rr = 6 + 8 * rng.uni();
                for (int j = 0; j < m; j++) for (int i = 0; i < m; i++) { L.at(H, 0, 0, (i - (m - 1) * 0.5) * rr * 4.5, (j - (m - 1) * 0.5) * rr * 4.5, x, z); L.add(BK_DISH, x, z, H, rr, 1, 1); }
                L.at(H, 0, 0, 0, -(m * rr * 2.6 + 30), x, z); L.add(BK_HALL, x, z, H, 8, 14, 7, 0);
            } else if (w.style == 2) {
                const int m = 4 + rng.irange(4); const double hh = 30 + 25 * rng.uni();
                for (int j = 0; j < m; j++) for (int i = 0; i < m; i++) { L.at(H, 0, 0, (i - (m - 1) * 0.5) * 45, (j - (m - 1) * 0.5) * 45, x, z); L.add(BK_MAST, x, z, H, 1.5, 1.5, hh); }
                L.at(H, 0, 0, 0, -(m * 23 + 40), x, z); L.add(BK_HALL, x, z, H, 8, 14, 7, 0);
            } else {
                for (int k = 0; k < 12; k++) { const double a = k * TAU / 12; L.add(BK_DISH, std::sin(a) * 200, std::cos(a) * 200, a, 9, 1, 1); }
                L.add(BK_POD, 0, 0, H, 18, 18, 10);
            }
            break;
        }
        case WO_WRECK: {
            switch (w.style) {
                case 0: L.add(BK_SHIP, 0, 0, H, 3.4, 13 + 5 * rng.uni(), 3.6, 0); break;
                case 1: L.add(BK_SHIP, 0, 0, H, 6, 30 + 14 * rng.uni(), 7.5, 1); break;
                case 2: L.add(BK_SHIP, 0, 0, H, 12, 70 + 40 * rng.uni(), 13, 2); break;
                case 3: L.add(BK_AIRCRAFT, 0, 0, H, 16 + 10 * rng.uni(), 14 + 8 * rng.uni(), 4, 0); break;
                default: L.add(BK_WRECK, 0, 0, H, 14 + 12 * rng.uni(), 50 + 60 * rng.uni(), 8, 0); break;
            }
            break;
        }
        case WO_COLOSSUS: L.add(BK_COLOSSUS, 0, 0, H, std::max(5.0, w.height * 0.2), std::max(5.0, w.height * 0.2), w.height, w.style); break;
        case WO_TERRAFORM: {
            const double R = w.length * 0.5;
            L.add(BK_MOUND, 0, 0, 0, R, 10, 4, 4);
            for (int k = 0; k < 4; k++) { const double a = H + k * TAU / 4; L.add(BK_POD, std::sin(a) * (R - 60), std::cos(a) * (R - 60), a, 12, 12, 8); }
            break;
        }
        case WO_SHELTER: {
            double x, z;
            const bool hill = w.height > 0.12;
            switch (w.style) {
                case 0: L.add(BK_PORTAL, 0, 0, H, 3.6, 1, 3); L.at(H, 0, 0, 0, 7, x, z); L.add(BK_TENT_RING, x, z, H, 2.4, 2.4, 0.4); break;
                case 1:
                    L.add(BK_TOWER, 0, 0, H, 6, 6, 16);
                    for (int q = 0; q < 6; q++) {
                        const double a = q * TAU / 6 + H;
                        if (q == 0) {
                            L.add(BK_GATE, std::sin(a) * 26, std::cos(a) * 26, a, 3.5, 1.4, 6);
                            continue;
                        }
                        L.add(BK_WALL, std::sin(a) * 26, std::cos(a) * 26, a, 13.5, 1.0, 6);
                    }
                    break;
                case 2: L.add(BK_PORTAL, 0, 0, H, 4, 1, 5); L.at(H, 0, 0, 0, 30, x, z); if (!hill) L.add(BK_FORT, x, z, H, 14, 14, 4, 1); break;
                case 3:
                    if (hill) L.add(BK_PORTAL, 0, 0, H, 3, 1, 4.5); else L.add(BK_BUNKER, 0, 0, H, 9, 7, 4, 1);
                    L.at(H, 0, 0, 6, hill ? -30 : -24, x, z); L.add(BK_BUNKER, x, z, H, 2, 2, 2, 2);
                    for (int s = -1; s <= 1; s += 2) { L.at(H, 0, 0, s * 18, hill ? -40 : -30, x, z); L.add(BK_MAST, x, z, H, 1, 1, 22 + 6 * rng.uni()); }
                    L.at(H, 0, 0, 14, 16, x, z); L.add(BK_BUNKER, x, z, H, 4, 3, 3, 1);
                    break;
                case 5:   // a medieval people that lost its sun's halls under the mountain: their mouth in the rock, a gatehouse before it, walls either side
                    L.add(BK_PORTAL, 0, 0, H, 4.5, 1, 6);
                    L.at(H, 0, 0, 0, 16, x, z); L.add(BK_GATE, x, z, H, 3.5, 1.4, 6);
                    for (int s = -1; s <= 1; s += 2) { L.at(H, 0, 0, s * 11.5, 16, x, z); L.add(BK_WALL, x, z, H, 7, 1.0, 5); }
                    break;
                default:
                    L.add(BK_HABITAT, 0, 0, H, 22, 22, 8);
                    if (hill) { L.at(H, 0, 0, 0, 30, x, z); L.add(BK_PORTAL, x, z, H, 4, 1, 6); }
                    break;
            }
            break;
        }
        case WO_DOME: {   // the dome over the warm ground (`width` its radius), its door toward its place
            const double rad = w.width;
            if (w.style == 0) L.add(BK_ROTUNDA, 0, 0, H, rad, rad, 6 + rad * 0.25);   // a medieval people's vaulted hall over its spring
            else if (w.style == 3) L.add(BK_HABITAT, 0, 0, H, rad, rad, 8);
            else L.add(BK_DOME, 0, 0, H, rad, rad, 2.6, w.style == 1 ? 0 : (w.style == 2 ? 1 : 2));
            break;
        }
        case WO_GREENHOUSE: {   // the glasshouses in rows (`width` and `length` the block's half extents), their long sides along `heading`
            const int st = clampi(w.style, 0, 2);
            const double hw = st == 0 ? 4.0 : (st == 1 ? 4.5 : 6.0), hd = st == 0 ? 12.0 : (st == 1 ? 18.0 : 20.0), h = st == 2 ? hw * 0.9 : hw;
            const int rows = std::max(1, (int)std::lround((2 * w.width + 3) / (2 * hw + 3))), per = std::max(1, (int)std::lround((2 * w.length + 4) / (2 * hd + 4)));
            for (int i = 0; i < rows; i++)
                for (int j = 0; j < per; j++) {
                    double x, z; rot(H, (i - (rows - 1) * 0.5) * (2 * hw + 3), (j - (per - 1) * 0.5) * (2 * hd + 4), x, z);
                    if (wet(x, z)) continue;
                    L.add(BK_GREENHOUSE, x, z, H, hw, hd, h, st);
                }
            break;
        }
        case WO_STRANGE: {
            switch (w.style) {
                case 1: {   // the pillars of this stretch, one every 400 m along the line
                    const double hd = w.length * 0.5;
                    for (double t = -hd + 200; t <= hd - 199; t += 400) { double x, z; rot(H, 0, t, x, z); L.add(BK_SPHERE, x, z, H, 4, 4, 160, 1); }
                    break;
                }
                case 3: { Building& b = L.add(BK_LATTICE, 0, 0, H, w.length * 0.5, w.width * 0.5, w.crest - w.floor, 0); b.base = w.floor; break; }
                case 4: L.add(BK_SPHERE, 0, 0, H, w.height, w.height, w.height * 2, 0); break;
                default: break;   // a plain made flat, a mountain cut into terraces: the ground alone
            }
            break;
        }
        case WO_WINDFARM: {
            const double top = w.ground;
            for (int row = -1; row <= 1; row++)
                for (int k = -4; k <= 4; k++) {
                    double x, z; rot(H, row * 380 + rng.sym(30), k * 330 + rng.sym(40), x, z);
                    if (wet(x, z) || ground(x, z) < top - 70) continue;
                    L.add(BK_TURBINE, x, z, r.site.wind + PI, 42 + 12 * rng.uni(), 1, w.height, 0, AGE_MODERN);
                }
            break;
        }
        case WO_LIGHTHOUSE: {
            L.add(BK_LIGHTHOUSE, 0, 0, H, 3.4 + 1.2 * rng.uni(), 3.4, w.height, 0);
            break;
        }
        case WO_FORT: {   // on its height, its gate toward the settlement it guarded (`heading`)
            double x, z;
            switch (w.style) {
                case 0: {   // a hill fort: its rings of bank inside their ditches (the edits) in stretches each on its own ground (the rampart follows
                            // the hill), a gap at the gate; the huts of its last people inside
                    const int rings = std::max(1, (int)w.height);
                    for (int k = 0; k < rings; k++) {
                        const double R = w.width - k * 18;
                        const int n = std::max(12, (int)std::ceil(TAU * R / 16));
                        for (int q = 0; q < n; q++) {
                            const double a = (q + 0.5) * TAU / n;
                            // the gate (R-412: the stretch nearest it whatever the ring's: a quarter of the forts had none)
                            if (std::fabs(std::remainder(a - H, TAU)) < std::max(0.1, PI / n)) continue;
                            L.add(BK_MOUND, std::sin(a) * R, std::cos(a) * R, a + PI / 2, 2.6, PI * R / n + 0.6, 2.4 - 0.3 * k, 3);
                        }
                    }
                    const double inner = std::max(10.0, w.width - rings * 18);
                    const int nh = 3 + rng.irange(5);
                    std::vector<std::pair<double, double>> huts;   // each clear of the others (two stood through each other, one's doorway in the other's wall)
                    for (int k = 0; k < nh; k++) {
                        const double a = rng.uni() * TAU, d = inner * 0.6 * std::sqrt(rng.uni()), hh = rng.uni() * TAU, hw = 2.6 + rng.uni(), hd = 2.6 + rng.uni();
                        const double hx = std::sin(a) * d, hz = std::cos(a) * d;
                        bool clear = true; for (const auto& q : huts) clear = clear && std::hypot(q.first - hx, q.second - hz) > 2 * 3.6 + 2.5;
                        if (!clear) continue;
                        huts.push_back({hx, hz});
                        L.add(BK_HUT, hx, hz, hh, hw, hd, 2.2);
                    }
                    break;
                }
                case 1: {   // a castle: its curtain wall of `height` sides with a tower at each corner, the gatehouse in the side toward its settlement, the keep
                    const int n = clampi((int)w.height, 4, 8);
                    const double R = w.width, half = R * std::sin(PI / n), apo = R * std::cos(PI / n);
                    for (int q = 0; q < n; q++) {
                        const double a = H + q * TAU / n;   // the middle of side q faces out at `a`
                        if (q == 0) {
                            L.add(BK_GATE, std::sin(a) * apo, std::cos(a) * apo, a, 3.6, 1.6, 7.5);
                            for (int sd = -1; sd <= 1; sd += 2) { L.at(a, 0, 0, sd * (half + 3.6) * 0.5, apo, x, z); L.add(BK_WALL, x, z, a, (half - 3.6) * 0.5 + 0.3, 1.2, 7); }
                        } else L.add(BK_WALL, std::sin(a) * apo, std::cos(a) * apo, a, half + 0.3, 1.2, 7);
                        const double c = a + PI / n;   // the corner after it
                        L.add(BK_WALL_TOWER, std::sin(c) * R, std::cos(c) * R, c, 3.6, 3.6, 11);
                    }
                    L.at(H, 0, 0, 0, -R * 0.35, x, z); L.add(BK_TOWER, x, z, H, 6.5, 6.5, 20);   // the keep, back from the gate
                    break;
                }
                case 2: L.add(BK_FORT, 0, 0, H, w.width, w.width, 3, 0); break;   // a redoubt: its ring bank and casemate (its ditch in the edits)
                default: {   // a strongpoint: a blockhouse, pillboxes round it, dragon's teeth across the way up
                    L.add(BK_BUNKER, 0, 0, H, 9, 7, 4, 1);
                    const int np = 3 + rng.irange(3);
                    for (int k = 0; k < np; k++) { const double a = H + k * TAU / np + rng.sym(0.3), d = 40 + 18 * rng.uni(); L.add(BK_BUNKER, std::sin(a) * d, std::cos(a) * d, a, 3.4, 3.4, 2.4, 0); }
                    L.at(H, 0, 0, 0, 70, x, z); L.add(BK_TEETH, x, z, H + PI / 2, 4, 28, 1.2);
                    break;
                }
            }
            break;
        }
        default: break;
    }
}

std::string workLine(const Work& w) {
    char buf[512];
    std::string what;
    switch (w.kind) {
        case WO_DAM: { bool drained = false; for (const GroundEdit& e : w.edits) if (e.kind == GE_WATER && e.tone == ET_DARK) drained = true;
            char b2[220];
            snprintf(b2, sizeof b2, "%.0f m over its floor, %.0f m long; its reservoir at %.0f m, %.2f km2%s", w.height, w.length, w.level, w.width / 1e6, drained ? ", breached and drained" : "");
            what = b2;
            break;
        }
        case WO_QUARRY:
        case WO_MINE: {
            static const char* const QS[7] = {"flint pits", "a face", "a face in benches", "a face in great benches", "an open pit", "a stone pit", "a pit in benches"};
            static const char* const MS[3] = {"adits", "headframes", "an open pit"};
            what = w.kind == WO_QUARRY ? QS[clampi(w.style, 0, 6)] : MS[clampi(w.style, 0, 2)]; break;
        }
        case WO_BATTLEFIELD: { char b2[160]; snprintf(b2, sizeof b2, "the war %d's battle of year %d, %.0f km between its sides", w.war, w.year, w.length / 1000.0); what = b2; break; }
        case WO_AQUEDUCT: { char b2[120]; snprintf(b2, sizeof b2, "%.1f km from its spring at %.0f m", w.length / 1000.0, w.level); what = b2; break; }
        case WO_OBSERVATORY: { char b2[80]; snprintf(b2, sizeof b2, "on a summit of %.0f m", w.height); what = b2; break; }
        default: { char b2[60]; snprintf(b2, sizeof b2, "style %d", w.style); what = b2; break; }
    }
    snprintf(buf, sizeof buf, "%s (%s%s) at %.3f %.3f, %.0f m across%s%s; %s", w.kind == WO_SHELTER && w.before > 0 ? "a shelter" : WORK_KIND_NAMES[w.kind],   // D-05: a shelter not the last
             AGE_NAMES[clampi(w.age, 0, AGE_COUNT - 1)], w.before > 0 ? ", left early" : "",
             w.lat / DEG, w.lon / DEG, 2 * w.radiusM, w.year ? (std::string(", year ") + std::to_string(w.year)).c_str() : "", w.heard ? "" : ", unheard", what.c_str());
    return buf;
}
