// E-09: the other ages' buildings: the prehistoric, the industrial, the modern, the futuristic and the far future's
#include "buildings_internal.h"

// ---- the prehistoric age ------------------------------------------------------------------------------------------------------

namespace {
// the hut's roof (the people's): a cone of thatch (pitched, hipped, conical), a corbelled dome of dry stone (domed: a beehive), a
// turf roof on it (flat, vaulted), a saddle of hide, a stepped cone, nothing (open: a windbreak)
void hutRoof(Gen& g, double rx, double rz, double hmin, double h, bool stone) {
    const int rf = g.c.roof;
    if (hmin < 0.92 * h || rf == RF_OPEN) return;
    const double Dt = g.D(MT_TIMBER), r = std::max(rx, rz);
    hmin += g.sink;   // over a pit house's sunk floor
    if (rf == RF_DOMED && stone) {   // a beehive: the dry stone corbelled in to a point
        const double Ds = g.D(MT_DRYSTONE);
        if (Ds < 0.6) {
            Form& e = g.add(FK_DOME, 2, 0, 0, 0, rx + 0.2, rz + 0.2, hmin - 0.1, hmin - 0.1 + r * 1.15, g.roofSlot(MT_DRYSTONE), MT_DRYSTONE, 2);
            e.broken = Ds < 0.25 ? 0 : clampd(Ds, 0.3, 0.9);
        }
        return;
    }
    if (rf == RF_FLAT || rf == RF_VAULTED || rf == RF_DOMED) {   // turf on its timbers: a mound of the ground's own grass
        const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
        if (Dt < 0.5 && De < 0.8) {
            Form& e = g.add(FK_DOME, 1, 0, 0, 0, rx + 0.4, rz + 0.4, hmin - 0.1, hmin - 0.1 + r * (rf == RF_FLAT ? 0.25 : 0.55), 0, MT_EARTH, 8);
            e.broken = Dt < 0.3 ? 0 : 0.5;
        }
        return;
    }
    if (Dt >= 0.5) return;
    const int slot = g.roofSlot(MT_TIMBER);
    const double ev = g.eave() + 0.05, pk = g.pitchK();   // R-412: the people's eaves and pitch
    if (rf == RF_SADDLE) { Form& e = g.add(FK_SHELL, 0, 0, 0, 0, rx + ev, rz + ev, hmin - 0.1, hmin - 0.1 + r * 1.0 * pk, slot, MT_TIMBER, 2); e.broken = Dt < 0.22 ? 0 : 0.5; return; }
    if (rf == RF_STEPPED) {   // two cones, the upper set on the lower
        Form& a = g.add(FK_LATHE, 1, 0, 0, 0, rx + ev, rz + ev, hmin - 0.1, hmin - 0.1 + r * 0.6 * pk, slot, MT_TIMBER); a.p[0] = 0.45;
        Form& t = g.add(FK_LATHE, 1, 0, 0, 0, (rx + ev) * 0.5, (rz + ev) * 0.5, hmin + r * 0.45 * pk, hmin + r * 1.25 * pk, slot, MT_TIMBER); t.p[0] = 0.04;
        return;
    }
    Form& e = g.add(FK_LATHE, 1, 0, 0, 0, rx + ev, rz + ev, hmin - 0.1, hmin - 0.1 + r * (rf == RF_CONICAL ? 1.6 : 1.25) * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * pk, slot, MT_TIMBER);
    e.p[0] = 0.04; e.broken = Dt < 0.22 ? 0 : clampd((Dt - 0.22) * 2.4, 0.2, 0.8);
    if (g.flared() && g.lod < 2 && Dt < 0.3) { Form& k = g.add(FK_LATHE, 1, 0, 0, 0, rx + ev + 0.45, rz + ev + 0.45, hmin - 0.35, hmin + 0.2, slot, MT_TIMBER); k.p[0] = 0.7; }   // its skirt of thatch
}
}   // namespace

void genHut(Gen& g) {   // a hut on the people's plan: a ring of dry stone, or wattle on posts; its roof; a hearth of four stones; A-04: a rectangle of
    const Building& b = g.b; const Culture& c = g.c;   // posts under a ridge, two rings joined (free), a pit house (buried, or sunk where the people dug in)
    const double h = b.hWall, Dt = g.D(MT_TIMBER), Ds = g.D(MT_DRYSTONE);
    const bool stone = c.hut == 1;
    if (g.gv(GV_MASSING) == MS_BURIED || b.found == FD_PIT) g.sink = -std::min(0.9, 0.45 * h);
    if (b.form == PF_RECT || b.form == PF_COURT) {   // a rectangular hut: walls of stone or wattle on posts, a ridge over it
        Walls W;
        W.hw = b.hw;
        W.hd = b.hd;
        W.h = h;
        W.thick = stone ? 0.6 : 0.2;
        W.door = 0;
        W.doorPos = 0;
        W.mat = stone ? MT_DRYSTONE : MT_TIMBER;
        W.slot = stone ? 0 : g.timberSlot();
        W.dress = false;
        double hs[4] = {0, 0, 0, 0}, hmin = 0;
        if (stone || Dt < 0.45) hmin = walls4(g, W, hs);
        else if (g.lod == 0) { int k = 0; for (int s = -1; s <= 1; s += 2) for (double t = -b.hd; t <= b.hd + 0.01; t += 1.5, k++) g.post(s * b.hw, t, h, 0.13, 200 + k); }
        if (hmin > 0.92 * h) {
            const int rf = c.roof == RF_DOMED || c.roof == RF_VAULTED || c.roof == RF_FLAT ? RF_FLAT : (c.roof == RF_HIPPED || c.roof == RF_CONICAL ? RF_HIPPED : RF_PITCHED);
            if (rf == RF_FLAT) {
                const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet);
                if (Dt < 0.5 && De < 0.8) {
                    Form& e = g.box(0, 0, 0, b.hw + 0.4, b.hd + 0.4, g.sink + hmin - 0.1, g.sink + hmin + 0.35, 0, MT_EARTH, 8);
                    (void)e;
                }
            } else roof(g, rf, 0, 0, b.hw, b.hd, hmin, h, 1.05, MT_TIMBER);
        }
        if (g.lod == 0) for (int k = 0; k < 4; k++) { double a = k * PI / 2 + 0.4; g.box(std::sin(a) * 0.45, std::cos(a) * 0.45, a, 0.17, 0.13, 0, 0.16, 0, MT_DRYSTONE, 3); }
        if (hmin > 0.92 * h) hutEntrance(g, b.hw, b.hd, 0, h, stone);   // its entrance, its pole, its yard
        hutPole(g, b.hw * 0.6 + 1.0, b.hd + 1.5);
        if (!b.joined) hutRing(g, b.hw + 0.4, b.hd + 0.4, 0);
        return;
    }
    const int N = hutSegs(b), dseg = hutDoorSegs(b), door = hutDoorSeg(b);
    const double rx = b.hw, rz = b.form == PF_ELLIPSE ? b.hd : b.hw;
    auto inDoor = [&](int k) { int d = ((k - door) % N + N) % N; return d < dseg; };
    double hmin = h;
    if (g.lod == 2) {
        if (stone) g.add(FK_LATHE, 0, 0, 0, 0, rx, rz, g.sink, g.sink + std::max(0.4, h * wallShare(Ds, 0.5)), 0, MT_DRYSTONE);
        if (Dt < 0.35) { Form& e = g.add(FK_LATHE, 1, 0, 0, 0, rx + 0.5, rz + 0.5, g.sink + (stone ? h : 0), g.sink + h + rx * 1.2, g.roofSlot(MT_TIMBER), MT_TIMBER); e.p[0] = 0.04; }
        return;
    }
    if (stone) {
        Ring R;
        R.rx = rx;
        R.rz = rz;
        R.h = h;
        R.thick = 0.64;
        R.mat = MT_DRYSTONE;
        R.segs = N;
        R.door = door;
        R.doorSegs = dseg;
        R.dress = false;
        R.profile = g.gv(GV_PROFILE) == WP_BATTERED || g.gv(GV_PROFILE) == WP_BULGING ? WP_BATTERED : WP_UPRIGHT;
        hmin = ringWalls(g, R);
    } else {
        for (int k = 0; k < N; k++) {   // the posts at the joints (none inside the doorway), the wattle between them
            const bool inGap = k != door && inDoor(k) && inDoor((k + N - 1) % N);
            if (inGap) continue;
            const double a = k * TAU / N; g.post(std::sin(a) * rx, std::cos(a) * rz, h, 0.13, 10 + k);
        }
        if (Dt < 0.35)
            for (int k = 0; k < N; k++) {
                if (g.lod == 0 && inDoor(k)) continue;
                const double a0 = k * TAU / N, a1 = (k + 1) * TAU / N, x0 = std::sin(a0) * rx, z0 = std::cos(a0) * rz, x1 = std::sin(a1) * rx, z1 = std::cos(a1) * rz;
                const double dx = x1 - x0, dz = z1 - z0, ch = std::sqrt(dx * dx + dz * dz);
                g.box(0.5 * (x0 + x1), 0.5 * (z0 + z1), std::atan2(-dz, dx), ch * 0.5 + 0.05, 0.08, g.sink, g.sink + h * wallShare(Dt, g.U(30 + k)), g.timberSlot(), MT_TIMBER);
            }
        else hmin = 0;
    }
    if (b.form == PF_FREE && g.lod < 2) {   // a second, smaller ring joined at the back
        Ring R2;
        R2.cz = -(rz + rz * 0.45);
        R2.rx = rx * 0.6;
        R2.rz = rz * 0.6;
        R2.h = h * 0.85;
        R2.thick = stone ? 0.6 : 0.2;
        R2.mat = stone ? MT_DRYSTONE : MT_TIMBER;
        R2.slot = stone ? 0 : g.timberSlot();
        R2.segs = 8;
        R2.open = true;
        R2.dress = false;
        R2.k0 = 40;
        if (stone || Dt < 0.35) {
            const double h2 = ringWalls(g, R2);
            if (h2 > 0.92 * R2.h && Dt < 0.5) {
                Form& e = g.add(FK_LATHE, 1, 0, R2.cz, 0, R2.rx + 0.4, R2.rz + 0.4, g.sink + h2 - 0.1, g.sink + h2 + R2.rx * 1.1, g.roofSlot(MT_TIMBER), MT_TIMBER);
                e.p[0] = 0.04;
            }
        }
    }
    const double sinkSave = g.sink; hutRoof(g, rx, rz, hmin, h, stone); g.sink = sinkSave;
    if (g.lod == 0 && hmin > 0.92 * h) {   // the accent where the people put it
        const double am = (door + 0.5 * dseg) * TAU / N;
        if (g.accentOn(AP_BANDS) && (stone || Dt < 0.35))
            for (int k = 0; k < N; k++) {
                if (inDoor(k)) continue;
                const double a0 = k * TAU / N, a1 = (k + 1) * TAU / N, x0 = std::sin(a0) * rx, z0 = std::cos(a0) * rz, x1 = std::sin(a1) * rx, z1 = std::cos(a1) * rz, dx = x1 - x0, dz = z1 - z0,
                             ch = std::sqrt(dx * dx + dz * dz);
                g.box(0.5 * (x0 + x1) - dz / ch * 0.08, 0.5 * (z0 + z1) + dx / ch * 0.08, std::atan2(-dz, dx), ch * 0.5 + 0.1, stone ? 0.38 : 0.12, g.sink + h * 0.45, g.sink + h * 0.45 + 0.3, 1,
                        stone ? MT_DRYSTONE : MT_TIMBER, 4);
            }
        if (g.accentOn(AP_DOORS) && g.D(MT_TIMBER) < 0.4)
            for (int s = -1; s <= 1; s += 2) {
                const double a = am + s * (PI * dseg / N + 0.12);
                g.lathe(0, std::sin(a) * (rx + 0.25), std::cos(a) * (rz + 0.25), 0.14, 0, h * 1.25, 1, MT_TIMBER, 6);
            }
        if (g.accentOn(AP_TOPS) && g.D(MT_TIMBER) < 0.4 && c.roof != RF_OPEN) g.lathe(0, 0, 0, 0.09, g.sink + h, g.sink + h + std::max(rx, rz) * 1.7, 1, MT_TIMBER, 4);
    }
    if (g.lod == 0) for (int k = 0; k < 4; k++) { double a = k * PI / 2 + 0.4; g.box(std::sin(a) * 0.45, std::cos(a) * 0.45, a, 0.17, 0.13, 0, 0.16, 0, MT_DRYSTONE, 3); }
    {   // its entrance, its pole by the door, its yard
        const double am = (door + 0.5 * dseg) * TAU / N;
        if (hmin > 0.92 * h || !stone) hutEntrance(g, rx, rz, am, h, stone);
        const double pa = am + 0.62, pr = std::max(rx, rz) + 1.5;
        hutPole(g, std::sin(pa) * pr, std::cos(pa) * pr);
        if (!b.joined && b.form != PF_FREE) hutRing(g, rx, rz, am);
    }
}

void genLonghouse(Gen& g) {   // walls of turf or planks, two rows of posts under its roof (the people's: a ridge of thatch, hipped, a turf vault), a hearth or two
    const Building& b = g.b; const Culture& c = g.c;
    const double Dt = g.D(MT_TIMBER), h = b.hWall;
    const bool turf = c.hut == 1 || g.U(3) < 0.4;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = h; W.thick = turf ? 1.0 : 0.25; W.door = 0; W.doorPos = 0;
    W.mat = turf ? MT_EARTH : MT_TIMBER; W.slot = turf ? 0 : g.timberSlot(); W.part = turf ? 8 : 0; W.dress = false;
    W.profile = g.gv(GV_PROFILE) == WP_BULGING || g.gv(GV_PROFILE) == WP_BATTERED ? WP_BATTERED : WP_UPRIGHT;
    double hs[4] = {0, 0, 0, 0}, hmin = 0;
    if (turf || Dt < 0.45) hmin = walls4(g, W, hs);
    else if (g.lod == 0)   // the planks gone: the posts of the walls in their rows
        for (int side = -1; side <= 1; side += 2) { int k = 0; for (double t = -b.hd; t <= b.hd + 0.01; t += 1.6, k++) g.post(side * b.hw, t, h, 0.12, 200 + k * 2 + (side > 0)); }
    if (g.lod == 2) return;
    if (g.lod == 0) {
        int k = 0;
        for (double t = -b.hd + 1.5; t < b.hd - 1.0; t += 2.6, k++)
            for (int side = -1; side <= 1; side += 2) g.post(side * b.hw * 0.5, t, h + b.hw * 0.7, 0.14, 100 + k * 2 + (side > 0));
    }
    if (hmin > 0.92 * h) {
        const int rf = c.roof;
        if (rf == RF_VAULTED || rf == RF_DOMED || rf == RF_FLAT) {   // a turf roof over its timbers, bowed
            const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet);
            if (Dt < 0.5 && De < 0.8) {
                Form& e = g.add(FK_VAULT, 0, 0, 0, 0, b.hw + 0.3, b.hd + 0.3, hmin - 0.05, hmin - 0.05 + b.hw * (rf == RF_FLAT ? 0.35 : 0.8), 0, MT_EARTH, 8);
                e.p[0] = 0.6;
                e.p[1] = 1;
                e.broken = Dt < 0.3 ? 0 : 0.5;
            }
        } else roof(g, rf == RF_HIPPED || rf == RF_CONICAL || rf == RF_STEPPED ? RF_HIPPED : (rf == RF_SADDLE ? RF_SADDLE : RF_PITCHED), 0, 0, b.hw, b.hd, hmin, h, 1.05, MT_TIMBER);
    }
    if (g.lod == 0)
        for (int q = 0; q < 2; q++) {
            double z = (q ? -0.3 : 0.3) * b.hd;
            if (q && b.hd < 8) break;
            for (int k = 0; k < 4; k++) {
                double a = k * PI / 2 + 0.4;
                g.box(std::sin(a) * 0.45, z + std::cos(a) * 0.45, a, 0.17, 0.13, 0, 0.16, 0, MT_DRYSTONE, 3);
            }
        }
    if (hmin > 0.92 * h) hutEntrance(g, b.hw, b.hd, 0, h, turf);   // R-412: its entrance, its pole
    hutPole(g, b.hw + 1.3, b.hd + 1.2);
}

void genTentRing(Gen& g) {   // the stones that held a tent's hide down, in a ring; the hide itself gone within a lifetime
    const Building& b = g.b;
    if (g.lod > 0) return;
    const int n = 8 + (int)(g.U(1) * 5);
    for (int k = 0; k < n; k++) {
        double a = (k + 0.3 * g.U(10 + k)) * TAU / n, rr = b.hw * (0.92 + 0.16 * g.U(30 + k)), s = 0.18 + 0.14 * g.U(50 + k);
        g.box(std::sin(a) * rr, std::cos(a) * rr, a, s, s * 0.8, 0, s * 1.1, 0, MT_DRYSTONE, 3);
    }
    const double Dt = g.D(MT_TIMBER);
    if (Dt < 0.4) { Form& e = g.lathe(1, 0, 0, b.hw, 0, b.hWall, g.roofSlot(MT_TIMBER), MT_TIMBER); e.p[0] = 0.06; e.broken = Dt < 0.2 ? 0 : 0.5; }
}

// a standing stone: upright while the dry stone holds, then leaning (the tilt about its foot), then lying where it fell, broken up
// at the last; `k` its draws
void standingStone(Gen& g, double lx, double lz, double dh, double hx, double hz, double h, int k, bool tapered) {
    const double d = g.Dk(MT_DRYSTONE, k);
    if (d > 0.93) return;
    const int kind = tapered ? FK_PYRAMID : FK_BOX;
    if (d < 0.3) { Form& e = g.add(kind, 0, lx, lz, dh, hx, hz, 0, h, 0, MT_DRYSTONE, 5); e.p[1] = tapered ? 0.6 : 0; return; }
    const double fallDir = g.U(k + 300) * TAU;
    if (d < 0.55) { Form& e = g.add(kind, 0, lx, lz, fallDir, hx, hz, 0, h, 0, MT_DRYSTONE, 5); e.p[1] = tapered ? 0.6 : 0; e.tilt = 0.1 + 0.18 * (d - 0.3) / 0.25; return; }
    Form& e = g.add(kind, 0, lx, lz, fallDir, hx, hz, 0, h, 0, MT_DRYSTONE, 5); e.p[1] = tapered ? 0.6 : 0; e.tilt = PI / 2; e.broken = d > 0.78 ? 0.4 : 0;
}

// standing stones in a ring (A-04: their count a multiple of the people's number, their faces to the middle), a lintel over a pair here
// and there, an altar in the middle, a bank round it all; a ring of great posts instead where the people's centre was its assembly
// (`Building::style` 1)
void genStoneCircle(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double R = b.hw, h = b.hWall * (0.8 + 0.4 * g.gf(GV_PROPORTION));
    const int num = c.genome.number();
    const int n0 = std::max(9, (int)std::lround(TAU * R / (2.6 + 1.9 * g.U(1)))), n = std::max(num * 2, (n0 + num - 1) / num * num);
    if (b.style == 1) {   // posts, the wood henge
        for (int k = 0; k < n; k++) { double a = k * TAU / n; g.post(std::sin(a) * R, std::cos(a) * R, h * 1.3, 0.3, 100 + k); }
    } else {
        const bool lintels = g.U(2) < 0.35 && n >= 12;
        std::vector<int> up(n, 0);
        for (int k = 0; k < n; k++) {
            double a = k * TAU / n, w = 0.45 + 0.4 * g.U(10 + k), hh = h * (0.75 + 0.35 * g.U(40 + k));
            if (lintels) hh = h;
            standingStone(g, std::sin(a) * R, std::cos(a) * R, a, w, 0.3 + 0.2 * g.U(70 + k), hh, 100 + k, false);   // its broad face to the middle
            up[k] = g.Dk(MT_DRYSTONE, 100 + k) < 0.3;
        }
        if (lintels && g.lod < 2)   // a lintel on each upright pair of the even ones; fallen beside them otherwise
            for (int k = 0; k + 1 < n; k += 2) {
                double a = (k + 0.5) * TAU / n, len = R * std::sin(PI / n) + 0.6;
                if (up[k] && up[k + 1]) g.box(std::sin(a) * R * std::cos(PI / n), std::cos(a) * R * std::cos(PI / n), a, len, 0.35, h, h + 0.7, 0, MT_DRYSTONE, 4);
                else if (g.lod == 0 && g.Dk(MT_DRYSTONE, 400 + k) < 0.9) g.box(std::sin(a) * (R + 1.6), std::cos(a) * (R + 1.6), a + 0.3, len, 0.35, 0, 0.7, 0, MT_DRYSTONE, 3);
            }
    }
    if (g.lod == 0) g.box(0, 0, g.U(3) * PI, 1.1, 0.55, 0, 0.55, 0, MT_DRYSTONE, 3);   // the altar stone, lying
    if (R > 9 || g.gv(GV_EDGE) == ED_DITCHES) {   // the bank, its gap toward the front (the earth of a mound lasts ten times a wall's)
        const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
        Form& e = g.add(FK_SWEEP, 0, 0, 0, 0.25, R + 6, R + 6, 0, std::max(0.25, (g.gv(GV_EDGE) == ED_DITCHES ? 2.2 : 1.5) * (1 - 0.7 * De)), 0, MT_EARTH, 8);
        e.p[0] = TAU - 0.5;
        e.p[1] = 2.4;
        e.p[2] = 0.5;
    }
}

void genMenhir(Gen& g) {   // a standing stone, or a row of them along the heading, the tallest at its head (A-04: the people's number of them; an avenue
    const Building& b = g.b;   // of two rows where the people's centre was its market, `style` 1)
    const int num = g.c.genome.number();
    const int n = b.hd > 1.5 ? std::max(2, std::max(num, (int)std::lround(2 * b.hd / (3.0 + 1.5 * g.U(1))))) : 1;
    const double hp = 0.8 + 0.45 * g.gf(GV_PROPORTION);
    for (int row = 0; row < (b.style == 1 ? 2 : 1); row++)
        for (int k = 0; k < n; k++) {
            double t = n > 1 ? -b.hd + 2 * b.hd * k / (n - 1) : 0, f = n > 1 ? 1.0 - 0.45 * k / (n - 1) : 1.0;
            const double x = b.style == 1 ? (row ? 1 : -1) * 3.5 : 0;
            standingStone(g, x, t, g.U(20 + k) * 0.4 - 0.2, 0.55 + 0.45 * g.U(10 + k), 0.4 + 0.3 * g.U(30 + k), b.hWall * hp * f * (0.85 + 0.3 * g.U(40 + k)), 100 + k + row * 50, true);
        }
}

void genDolmen(Gen& g) {   // a chamber of slabs under its capstone, open at the front; the ring of what was its mound round it (A-04: the mound whole over it
    const Building& b = g.b;   // where the people's centre was a tomb, `style` 1, its passage open)
    const double hw = b.hw, hd = b.hd, h = b.hWall;
    const double d0 = g.Dk(MT_DRYSTONE, 10), d1 = g.Dk(MT_DRYSTONE, 11), d2 = g.Dk(MT_DRYSTONE, 12);
    // a slab fallen lies outward (tipping along the chamber, it would fall into it half the
    // time), its foot at its wall's line
    auto slab = [&](double lx, double lz, double dh, double out, double hx, double d) {
        if (d > 0.9) return;
        Form& e = g.box(lx, lz, dh, hx, 0.22, 0, h, 0, MT_DRYSTONE, 0);
        if (d > 0.55) { e.heading = g.b.heading + out; e.tilt = PI / 2; }
        else if (d > 0.3) e.tilt = 0.12;
    };
    slab(-hw, 0, PI / 2, -PI / 2, hd, d0); slab(hw, 0, PI / 2, PI / 2, hd, d1); slab(0, -hd, 0, PI, hw, d2);
    if (std::max(d0, std::max(d1, d2)) < 0.3) { Form& e = g.box(0, -0.2, 0, hw + 0.6, hd + 0.5, h, h + 0.7, 0, MT_DRYSTONE, 2); e.tilt = 0.04; }
    else if (g.lod == 0 && g.Dk(MT_DRYSTONE, 13) < 0.9) { Form& e = g.box(2 * hw + 1.5, 0.4, 0.4, hw + 0.6, hd + 0.5, 0, 0.7, 0, MT_DRYSTONE, 3); e.tilt = 0.18; }   // the capstone slid off beside it
    const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
    if (b.style == 1) {   // the mound whole round the chamber, its passage open at the front (the earth's dome, the chamber's sides inside it)
        const double R = hd * 2.6 + 2;
        Form& m = g.add(FK_SWEEP, 0, 0, 0, 0.35, R * 0.62, R * 0.62, 0, std::max(0.4, h * 1.6 * (1 - 0.5 * De)), 0, MT_EARTH, 8); m.p[0] = TAU - 0.7; m.p[1] = R * 0.45; m.p[2] = 0.8;
        return;
    }
    Form& m = g.add(FK_SWEEP, 0, 0, 0, 0.35, hd * 2.6 + 2, hd * 2.6 + 2, 0, std::max(0.25, 1.6 * (1 - 0.7 * De)), 0, MT_EARTH, 8); m.p[0] = TAU - 0.7; m.p[1] = 2.2; m.p[2] = 0.6;
}

void genPalisade(Gen& g) {   // a stretch of posts on an earth bank: the posts rot to their holes, the bank slumps (A-04: a ditched people's bank alone, higher)
    const Building& b = g.b;
    const double Dt = g.D(MT_TIMBER), h = b.hWall;
    const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
    const bool ditch = g.gv(GV_EDGE) == ED_DITCHES;
    Form& bank = g.add(FK_SWEEP, 1, 0, 0, PI / 2, 0, b.hw, 0, std::max(0.25, (ditch ? 2.2 : 1.0) * (1 - 0.6 * De)), 0, MT_EARTH, 8); bank.p[1] = ditch ? 3.2 : 1.8; bank.p[2] = 0.5;
    if (ditch) return;
    if (Dt < 0.35) { Form& e = g.box(0, 0, 0, b.hw, 0.2, 0.8, 0.8 + h, g.timberSlot(), MT_TIMBER); e.broken = 0.15; return; }
    if (g.lod > 0) return;
    int k = 0;
    for (double t = -b.hw + 0.4; t < b.hw; t += 1.5, k++) g.post(t, 0, 0.8 + h * 0.6, 0.15, 10 + k);
}

// ---- the industrial age -------------------------------------------------------------------------------------------------------

// B-415: a chimney stack of brick through an industrial people's roof, from `y0` to `y1` at (x, z) (its half extents across and along), and
// its two pots: the age's own tops
void stack(Gen& g, double x, double z, double hx, double hz, double y0, double y1, int mat) {
    g.box(x, z, 0, hx, hz, y0, y1, 0, mat);
    if (g.lod == 0) for (int k = -1; k <= 1; k += 2) g.lathe(0, x + (hx > hz ? k * hx * 0.5 : 0), z + (hx > hz ? 0 : k * hz * 0.5), 0.13, y1, y1 + 0.55, 3, mat, 6);
}

// a terraced row: brick walls of two or three storeys, windows, its roof along the street (A-04: a row of gables to the street, a house to
// each, where the people built in cells; crow steps on a people of steps), stacks on the party walls. B-415: the age's slate roof (a people
// of flat roofs behind a parapet), its brick gables standing on its walls after the roof, the stacks' pots
void genRow(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    const bool parapet = parapetRoof(c);
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = mat; W.pattern = 1; W.parapet = parapet ? 0.8 : 0;
    const int rf = ageRoof(c, c.roof);
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int houses = std::max(2, (int)std::lround(b.hw * 2 / 5.5));
    const bool cells = !parapet && (g.gv(GV_MODULAR) == 1 || c.roof == RF_STEPPED || g.gv(GV_FRONTAGE) == FR_GABLES);   // a gable a house to the street
    const int rm = roofMaterial(g, rf, mat);
    double rise = 0;
    if (cells) {   // a roof a house, gabled to the street
        for (int k = 0; k < houses; k++) {
            const double t = -b.hw + (k + 0.5) * 2 * b.hw / houses, half = b.hw / houses;
            roof(g, RF_PITCHED, t, 0, half, b.hd, hmin, b.hWall, 0.75, rm);
            Walls Wk = W; Wk.cx = t; Wk.hw = half; gables(g, Wk, hs, 0.75);
            if (hmin > 0.92 * b.hWall && g.lod == 0 && (c.roof == RF_STEPPED || g.gv(GV_MOTIF) == MO_STEPS) && g.gv(GV_GABLES) != GB_STEPPED)   // crow steps up the gable's front
                for (int q = 0; q < 3; q++) { const double w = half * (0.85 - 0.28 * q); g.box(t, b.hd, 0, w, 0.3, g.sink + hmin + q * half * 0.28, g.sink + hmin + (q + 1) * half * 0.28, 0, mat, 4); }
        }
    } else {
        rise = roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, parapet ? 0.35 : 0.7, rm);
        if (rf == RF_PITCHED) gables(g, W, hs, 0.7);   // the row's two ends
    }
    houseDress(g, W, hs, hmin, rf, rise, rm, !cells && !parapet);
    if (g.lod < 2 && hmin > 0.92 * b.hWall) {   // the chimney stacks on the party walls, through the ridge
        for (int k = 0; k <= houses; k++) stack(g, (-b.hw + 2 * b.hw * k / houses) * 0.97, 0, 0.35, 0.7, g.sink + hmin, g.sink + hmin + b.hd * 0.7 + 1.1, mat);
        if (g.gv(GV_TOPS) == TP_FINIALS) houseTops(g, cells ? RF_FLAT : rf, 0, 0, b.hw, b.hd, g.sink + hmin, rise, rm, mat);
    }
    double dx, dz; doorMid(0, W.doorPos, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    foundation(g, b.hw, b.hd, 0, W.doorPos, mat);
    yardOf(g, b.hw, b.hd, 0, W.doorPos, mat);
    drifts(g, b.hw, b.hd, Dm);
}

// four to six storeys round a stair, a cornice, its roof. B-415: the age's hipped slate (behind a parapet for a people of flat roofs), its
// stacks
void genTenement(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    const bool parapet = parapetRoof(c);
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.55; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = mat; W.pattern = 1; W.parapet = parapet ? 0.9 : 0;
    double hs[4]; const double hmin = walls4(g, W, hs);
    if (hmin > 0.92 * b.hWall && g.lod < 2) {
        g.box(0, 0, 0, b.hw + 0.4, b.hd + 0.4, g.sink + hmin - 0.6, g.sink + hmin, g.accentOn(AP_TOPS) ? 1 : 0, mat, 2);   // the cornice
        const int rf = RF_HIPPED, rm = roofMaterial(g, rf, mat);
        const double rise = roof(g, rf, 0, 0, b.hw * 0.95, b.hd * 0.95, hmin, b.hWall, parapet ? 0.3 : 0.5, rm);
        for (int q = 0; q < 4; q++) stack(g, (q % 2 ? 1 : -1) * b.hw * 0.62, (q < 2 ? 1 : -1) * b.hd * 0.3, 0.6, 0.35, g.sink + hmin, g.sink + hmin + rise * 0.75 + 1.4, mat);
        if (g.gv(GV_TOPS) == TP_FINIALS) houseTops(g, rf, 0, 0, b.hw, b.hd, g.sink + hmin, rise, rm, mat);
        houseDress(g, W, hs, hmin, rf, rise, rm, !parapet);
    } else houseDress(g, W, hs, hmin, RF_HIPPED, 0, MT_TIMBER, false);
    double dx, dz; doorMid(0, W.doorPos, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    foundation(g, b.hw, b.hd, 0, W.doorPos, mat);
    yardOf(g, b.hw, b.hd, 0, W.doorPos, mat);
}

// two storeys under its roof, its chimney (A-04: on the people's plan: a round villa, an L, a court). B-415: the age's slate roof (behind a
// parapet for a people of flat roofs), its gables, a chimney at each end of a big one
void genVilla(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    const int rf = ageRoof(c, c.roof), rm = roofMaterial(g, rf, mat);
    if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = 0.45; R.mat = mat; R.pattern = 1;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        const double hmin = ringWalls(g, R);
        roundRoof(g, rf, 0, 0, R.rx + 0.3, R.rz + 0.3, R.segs, hmin, b.hWall, 0.62, rm);
        foundation(g, R.rx, R.rz, 0, 0, mat);
        ringDress(g, R, hmin);
        yardOf(g, R.rx, R.rz, -1, 0, mat);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 1; W.parapet = parapetRoof(c) ? 0.8 : 0;
    double hs[4]; const double hmin = rectBody(g, W, rf, rm, hs, false);
    if (hmin > 0.92 * b.hWall && g.lod < 2) {
        const double top = g.sink + hmin + std::min(b.hw, b.hd) * 0.62 + 1.2;
        stack(g, b.hw * 0.55, -b.hd * 0.3, 0.45, 0.45, g.sink + hmin, top, mat);
        if (b.hw > 5) stack(g, -b.hw * 0.55, -b.hd * 0.3, 0.45, 0.45, g.sink + hmin, top, mat);
    }
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    foundation(g, b.hw, b.hd, 0, 0, mat);
    yardOf(g, b.hw, b.hd, 0, 0, mat);
}

// a church: a nave under its roof (the people's), its towers where the people's symmetry sets them, their spires (a people of spires:
// tall), or a dome over the crossing where its focus is a dome
void genChurch(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.8; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 3; W.glyphs = true;   // B-415: its own windows, tall
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = c.roof == RF_FLAT || c.roof == RF_OPEN || c.roof == RF_SADDLE || c.roof == RF_STEPPED ? RF_PITCHED : c.roof;   // a saddle's shell and a stepped crown gabled
    roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, 1.0, roofMaterial(g, rf, mat));
    if (rf == RF_PITCHED) gables(g, W, hs, 1.0);
    entranceAt(g, W, wallsHalf(g, W), W.h);   // its doorway's and its corners' (its towers stand at its front's corners: no turrets)
    if (g.gv(GV_CORNERS) != CO_TURRETS) cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
    const double hgt = 0.7 + 0.8 * g.gf(GV_HEIGHT), tr = std::max(3.0, b.hw * 0.5);
    if (g.gv(GV_FOCUS) == FO_DOME && hmin > 0.92 * b.hWall) {   // a dome on a drum over the crossing
        const double rd = b.hw * 0.85, Dd = g.D(mat);
        Form& d = g.lathe(0, 0, -b.hd * 0.2, rd, g.sink + hmin, g.sink + hmin + 3.5, 0, mat, 8); d.pattern = 2;
        if (Dd < 0.6) {
            Form& e = g.add(FK_DOME, ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : 0), 0, -b.hd * 0.2, 0, rd, rd, g.sink + hmin + 3.5, g.sink + hmin + 3.5 + rd,
                    g.roofSlot(mat), mat, 2);
            e.broken = Dd < 0.3 ? 0 : 0.5;
        }
    }
    const int sym = g.gv(GV_SYMMETRY);
    const double side = g.U(17) < 0.5 ? -1 : 1;
    std::vector<std::pair<double, double>> at;
    if (sym == SY_BILATERAL) at = {{-(b.hw + tr - 0.5), b.hd - tr}, {b.hw + tr - 0.5, b.hd - tr}};
    else if (sym == SY_ROTATIONAL) at = {{side * (b.hw + tr - 0.5), b.hd - tr}, {-side * (b.hw + tr - 0.5), -b.hd + tr}};
    else at = {{side * (b.hw + tr - 0.5), b.hd - tr}};   // the tower at a front corner, beside the nave's doorway
    const bool spire = g.gv(GV_FOCUS) == FO_SPIRE || g.U(16) < 0.6;
    for (size_t k = 0; k < at.size(); k++) {
        const double tH = b.hWall * 2.6 * hgt;
        TowerOut T = towerShaft(g, at[k].first, at[k].second, tr, tH, mat, 15 + (int)k * 3, 2);
        if (T.whole) towerTop(g, at[k].first, at[k].second, T.rTop, T.h, tH, mat, spire, b.hWall * (g.gv(GV_FOCUS) == FO_SPIRE ? 4.0 : 3.2) * hgt);
    }
    motifOn(g, 0, 0, b.hw, b.hd, std::min(hmin, b.hWall), b.hWall, mat);
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
}

// a town hall: a long front of tall windows, its roof, over its middle a clock tower with a dome on it (`style` 0), a great dome on a
// drum (1: the people's assembly), or a spire (2)
void genTownHall(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.8; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2; W.glyphs = true;
    W.parapet = parapetRoof(c) ? 1.1 : 0;   // B-415: a flat roof's people's hipped behind its parapet; a saddle and steps hipped
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = c.roof == RF_PITCHED || c.roof == RF_FLAT || c.roof == RF_OPEN || c.roof == RF_SADDLE || c.roof == RF_STEPPED ? RF_HIPPED : c.roof;
    roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, 0.45, roofMaterial(g, rf, mat));
    entranceAt(g, W, wallsHalf(g, W), W.h);   // its doorway's and its corners'
    cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
    if (hmin > 0.92 * b.hWall) {
        const double hgt = 0.7 + 0.8 * g.gf(GV_HEIGHT);
        if (b.style == 1) {   // the assembly's dome on its drum
            const double rd = std::min(b.hw, b.hd) * 0.8, Dd = g.D(mat);
            Form& d = g.lathe(0, 0, 0, rd, g.sink + hmin, g.sink + hmin + 4.0, 0, mat, 0); d.pattern = 2;
            if (Dd < 0.6) {
                Form& e = g.add(FK_DOME, ageDome(g, g.gv(GV_OPENING) == OP_ARCHED ? 2 : 0), 0, 0, 0, rd, rd, g.sink + hmin + 4.0, g.sink + hmin + 4.0 + rd * 1.05, g.roofSlot(mat), mat, 2);
                e.broken = Dd < 0.3 ? 0 : 0.5;
                if (Dd < 0.3) topsAt(g, {{0, 0}}, g.sink + hmin + 4.0 + rd * 1.05, 1.5, mat);
            }
        } else {
            const double tr = 3.2, tH = b.hWall * (b.style == 2 ? 2.2 : 1.35) * hgt;
            const double sinkSave = g.sink; g.sink += hmin;
            TowerOut T = towerShaft(g, 0, b.hd - tr, tr, tH, mat, 15, 2, false);
            if (T.whole) {
                if (b.style == 2) towerTop(g, 0, b.hd - tr, T.rTop, T.h, tH, mat, true, tH * 1.2);
                else if (g.D(MT_IRON) < 0.6) {
                    Form& d = g.add(FK_DOME, ageDome(g, 3), 0, b.hd - tr, 0, tr, tr, g.sink + T.h, g.sink + T.h + tr * 1.6, g.roofSlot(MT_IRON), MT_IRON, 2);
                    d.broken = g.D(MT_IRON) < 0.3 ? 0 : 0.5;
                }
            }
            g.sink = sinkSave;
        }
    }
    motifOn(g, 0, 0, b.hw, b.hd, std::min(hmin, b.hWall), b.hWall, mat);
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
}

// a station: a front block over the street and the train shed behind it: an iron vault, its glass gone at once, its ribs left (A-04: a
// people of arches fronts it with an arcade; `style` 1 a market hall, the vault alone with brick ends)
void genStation(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat), Di = g.D(MT_IRON);
    g.sink = -c.buried * Dm * 0.5;
    const double fd = b.style == 1 ? 0 : 5.5, shedHz = b.hd - fd;   // the front block's half depth; the shed fills the rest
    double hs[4] = {b.hWall, b.hWall, b.hWall, b.hWall};
    Walls W; W.cz = b.hd - fd; W.hw = b.hw; W.hd = fd; W.h = b.hWall; W.thick = 0.8; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2;
    if (b.style == 0) {
        W.parapet = parapetRoof(c) ? 1.0 : 0;
        const double hmin = walls4(g, W, hs);
        const int rf = ageRoof(c, c.roof);   // the age's
        roof(g, rf, 0, W.cz, b.hw, fd, hmin, b.hWall, 0.45, roofMaterial(g, rf, mat));
        if (rf == RF_PITCHED) gables(g, W, hs, 0.45);
        entranceAt(g, W, wallsHalf(g, W), W.h);
        cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
        if (c.technique == TQ_ARCH_VAULT && g.lod < 2 && hmin > 0.92 * b.hWall) {   // an arcade along its front
            Form& e = g.add(FK_ARCH, archSub(g), 0, b.hd + 2.2, 0, b.hw, 0.4, g.sink, g.sink + std::min(b.hWall * 0.55, 6.0), g.doorSlot(), mat, 4);
            e.p[0] = std::max(3.0, std::round(b.hw / 2.4)); e.p[1] = 0.36; e.p[2] = 2.6;
        }
    } else {   // the market hall's two brick ends
        for (int s = -1; s <= 1; s += 2) {
            const double h = std::max(0.5, b.hWall * wallShare(Dm, g.U(50 + s)));
            if (s > 0) {
                g.box(-b.hw * 0.6, b.hd, 0, b.hw * 0.4, 0.4, g.sink, g.sink + h, 0, mat);
                g.box(b.hw * 0.6, b.hd, 0, b.hw * 0.4, 0.4, g.sink, g.sink + h, 0, mat);
            } else g.box(0, -b.hd, 0, b.hw, 0.4, g.sink, g.sink + h, 0, mat);
        }
    }
    const double span = std::min(b.hw * 0.9, 18.0), sz = -fd, H = span * (0.7 + 0.4 * g.gf(GV_PROPORTION)) + 4;
    const int vs = c.technique == TQ_ARCH_VAULT ? 0 : 2;   // round, or a frame's parabola
    if (Di < 0.25) { Form& v = g.add(FK_VAULT, vs, 0, sz, 0, span, shedHz, 0, H, g.ironSlot(), MT_IRON); v.p[0] = 0.3; v.p[1] = 0; }
    else if (Di < 0.85 && g.lod < 2) {   // the ribs, one by one
        const int n = std::max(3, (int)(shedHz * 2 / 7));
        for (int k = 0; k < n; k++) {
            if (g.Dk(MT_IRON, 40 + k) > 0.7) continue;
            double z = sz - shedHz + (k + 0.5) * 2 * shedHz / n;
            Form& v = g.add(FK_VAULT, vs, 0, z, 0, span, 0.22, 0, H, g.ironSlot(), MT_IRON); v.p[0] = 0.35; v.p[1] = 0;
        }
    }
    if (g.lod == 0 && b.style == 0) for (int k = -1; k <= 1; k += 2) g.box(k * span * 0.35, sz, 0, 2.2, shedHz * 0.95, 0, 1.0, 0, mat, 2);   // the platforms
    if (b.style == 0) { double dx, dz; doorMid(0, 0, b.hw, fd, dx, dz); rubble(g, b.hw, fd, hs, b.hWall, dx, dz + W.cz, 0, mat); }
}

// a long mill of five storeys, its windows in a grid, its roof, its chimney at the end, snapped and fallen when worn. B-415: the age's slate
// roof (a row of gables across it where the people built in cells, a parapet round a people of flat roofs' low one), its brick gables, its
// chimney's lip black with soot (a painted people's white chimneys stood in rows like columns: BUBAS read as medieval)
void genMill(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    // its doorway in its front to the street (on its side a neighbour of the works stood against it) and its chimney within its own
    // ground at its back end (beside it, it stood at a neighbour's doorway): an industrial people's towns are all mills and rows now
    const bool parapet = parapetRoof(c);
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.7; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2; W.parapet = parapet ? 1.0 : 0;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = ageRoof(c, c.roof), rm = roofMaterial(g, rf, mat);
    if (hmin > 0.92 * b.hWall) {
        if (g.gv(GV_MODULAR) == 1 && !parapet) {   // gables across it in a row, a roof a bay
            const int n = clampi((int)std::lround(b.hd / 3.5), 3, 8);
            for (int k = 0; k < n; k++) roof(g, RF_PITCHED, 0, -b.hd + (k + 0.5) * 2 * b.hd / n, b.hw, b.hd / n, hmin, b.hWall, 0.6, rm);
        } else {
            const double rise = roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, parapet ? 0.3 : 0.5, rm);
            if (rf == RF_PITCHED) gables(g, W, hs, 0.5);
            if (!parapet) rooflineOf(g, W, hmin, rf, rise, rm);   // its ventilators, its cresting
        }
    }
    cornersOf(g, W, hs, wallsHalf(g, W), rm);
    const double cr = millChimneyR(b), cH = b.hWall * 3.4 * (0.7 + 0.6 * g.gf(GV_HEIGHT)), cz = -(b.hd - cr - 0.8), h = cH * tallShare(Dm * 1.1, g.U(33));
    Form& ch = g.lathe(5, 0, cz, cr, g.sink, g.sink + h, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 0); ch.p[0] = 0.6; ch.broken = h < cH * 0.97 ? 0.25 : 0;
    if (h < cH * 0.97 && g.lod == 0) {
        g.fallen(FK_LATHE, 0, 0, cz, PI / 2 + (g.U(34) - 0.5), cr * 0.75, (cH - h) * 0.5, 0, mat);
        g.heap(0, cz, cr * 2, cr * 2, std::min(3.5, (cH - h) * 0.05), 0, mat);
    }
    if (h >= cH * 0.97 && g.lod < 2) g.lathe(0, 0, cz, cr * 0.72, g.sink + cH - std::min(1.5, 0.04 * cH) - 1.6, g.sink + cH + 0.05, 3, mat);   // the soot of its lip
    if (g.banded() && g.lod < 2 && h >= cH * 0.97)
        for (int k = 1; k <= 2; k++) {
            const double t = 0.35 + 0.25 * k, rr = cr * (1 - 0.4 * t) + 0.15;
            Form& e = g.add(FK_TORUS, 0, 0, cz, 0, rr, rr, g.sink + cH * t, g.sink + cH * t + 0.8, 1, mat, 4);
            e.p[0] = 0.35;
        }   // bands round the chimney, under its lip
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
}

void genGasometer(Gen& g) {   // a ring of iron columns with their girders (A-04: twice the people's number), the bell inside: the sheet goes first, the frame after
    const Building& b = g.b;
    const double R = b.hw, H = b.hWall, Di = g.D(MT_IRON);
    const int n = clampi(2 * g.c.genome.number(), 6, 16), is = g.ironSlot();
    for (int k = 0; k < n; k++) {
        double a = k * TAU / n, d = g.Dk(MT_IRON, 10 + k);
        if (d > 0.85) continue;
        double h = H * (d < 0.35 ? 1 : 1 - (d - 0.35) * 1.4 * g.U(30 + k));
        Form& e = g.lathe(0, std::sin(a) * (R + 1), std::cos(a) * (R + 1), 0.45, 0, std::max(1.0, h), is, MT_IRON, 6); e.broken = h < H ? 0.3 : 0;
    }
    if (Di < 0.25 && g.lod < 2)
        for (int q = 1; q <= 2; q++) {
            Form& t = g.add(FK_TORUS, 0, 0, 0, 0, R + 1, R + 1, H * q / 2 - 0.6, H * q / 2, is, MT_IRON);
            t.p[0] = 0.3;
            t.broken = Di < 0.3 ? 0 : 0.6;
        }
    const double Db = g.D(MT_IRON) * 1.4;
    if (Db < 0.8) { Form& e = g.lathe(0, 0, 0, R, 0, H * (0.35 + 0.5 * g.U(2)), g.roofSlot(MT_IRON), MT_IRON); e.broken = Db < 0.3 ? 0 : clampd(Db, 0.3, 0.9); }
}

void genWaterTower(Gen& g) {   // a shaft of brick in the people's section and the iron tank on it
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat), Di = g.D(MT_IRON);
    const double r = b.hw * 0.55, H = b.hWall, h = H * tallShare(Dm, g.U(10));
    const int sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : (g.gv(GV_PLAN) == PF_CIRCLE ? 0 : 8);
    Form& t = g.lathe(g.gv(GV_PROFILE) == WP_BATTERED ? 1 : 0, 0, 0, r, 0, h, 0, mat, sides); t.p[0] = 0.8; t.pattern = 2; t.broken = h < H * 0.97 ? 0.3 : 0;
    if (h >= H * 0.97 && Di < 0.6) {
        Form& k = g.lathe(4, 0, 0, b.hw, H, H + b.hw * 1.1, g.roofSlot(MT_IRON), MT_IRON);
        k.p[0] = 0.4;
        k.broken = Di < 0.3 ? 0 : 0.5;
        if (Di < 0.3) topsAt(g, {{0, 0}}, H + b.hw * 1.1, 1.0, MT_IRON);
    } else if (g.lod == 0 && Di < 0.8) g.fallen(FK_LATHE, 0, 0, 0, g.U(11) * TAU, b.hw * 0.9, b.hw * 1.6, g.roofSlot(MT_IRON), MT_IRON);
}

void genHeadframe(Gen& g) {   // the iron frame over a shaft with its wheel, the engine house with its stack, the spoil heap
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Di = g.D(MT_IRON), Dm = g.D(mat);
    const double H = b.hWall;
    const int ex = g.gv(GV_EXPRESSION);   // R-412: an iron lattice (a people of frames), a brick tower over the shaft (a massive people's), two legs of an A-frame
    if (ex == EX_MASSIVE) {
        const double Dh = g.D(mat), share = tallShare(Dh, g.U(41));
        Form& t = g.box(0, b.hd * 0.45, 0, 3.6, 3.6, 0, H * share, 0, mat); t.pattern = 2; t.broken = share < 0.97 ? 0.3 : 0;
        if (share >= 0.97 && g.lod < 2) { Walls P; (void)P; g.box(0, b.hd * 0.45, 0, 3.85, 3.85, H, H + 0.6, g.topSlot(mat), mat, 4); }
    } else if (ex == EX_MONOLITH) {
        for (int s = -1; s <= 1; s += 2) {
            Form& l = g.add(FK_TRUSS, 0, 0, b.hd * 0.45 + s * 2.6, 0, 0.9, 0.9, 0, H * 1.02, g.ironSlot(), MT_IRON);
            l.sides = 4;
            l.p[0] = 0.6;
            l.p[2] = 0.12;
            l.tilt = -0.17 * s;
            l.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95);
        }
    } else {
        Form& t = g.add(FK_TRUSS, 0, 0, b.hd * 0.45, 0, 3.5, 3.5, 0, H, g.ironSlot(), MT_IRON); t.sides = 4; t.p[0] = 0.35; t.p[2] = 0.18;
        t.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95);
    }
    if (Di < 0.45) { Form& w = g.add(FK_TORUS, 0, 0, b.hd * 0.45, 0, 2.2, 2.2, H - 4.6, H, g.ironSlot(), MT_IRON); w.p[0] = 0.22; w.tilt = PI / 2; }
    Walls W; W.cz = -b.hd * 0.3; W.hw = 5; W.hd = 6.5; W.h = 8; W.thick = 0.6; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2; W.k0 = 30;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = ageRoof(c, c.roof);   // the age's
    const double rise = roof(g, rf, 0, W.cz, W.hw, W.hd, hmin, W.h, 0.6, roofMaterial(g, rf, mat));
    if (rf == RF_PITCHED) gables(g, W, hs, 0.6);
    rooflineOf(g, W, hmin, rf, rise, roofMaterial(g, rf, mat));   // its ventilator, its cresting
    cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
    const double sH = 26, h = sH * tallShare(Dm * 1.1, g.U(40));
    Form& s = g.lathe(5, 5.5, W.cz - 4, 1.2, 0, h, 0, mat); s.p[0] = 0.7; s.broken = h < sH * 0.97 ? 0.25 : 0;
    const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet);
    Form& heap = g.lathe(1, -b.hw * 0.75, b.hd * 0.5, b.hw * 0.5, 0, b.hw * 0.5 * (0.75 - 0.25 * De), 0, MT_EARTH); heap.part = 8; heap.p[0] = 0.12;
}

void genObservatory(Gen& g) {   // a drum of masonry and its turning dome of iron sheet, which goes first
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Di = g.D(MT_IRON), rr = b.hw;
    const int segs = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 5, 9) : 12;   // R-412: its drum the people's polygon, its dome the people's
    Ring R; R.rx = rr; R.rz = rr; R.h = b.hWall; R.thick = 0.8; R.mat = mat; R.segs = segs; R.doorSegs = ringDoorSegs(rr, segs); R.door = ringDoorFirst(b, segs, R.doorSegs); R.pattern = 2;
    const double hmin = ringWalls(g, R);
    const int prof = g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : 0);
    if (hmin > b.hWall * 0.92 && Di < 0.6) {
        Form& d = g.add(FK_DOME, ageDome(g, prof), 0, 0, 0, rr + 0.4, rr + 0.4, hmin - 0.1, hmin - 0.1 + rr * (prof == 0 ? 0.95 : 1.25), g.roofSlot(MT_IRON), MT_IRON, 2);
        d.broken = Di < 0.3 ? 0 : 0.6;
    }
}

// ---- the modern age -----------------------------------------------------------------------------------------------------------
// a slab of flats: concrete walls in bands of windows, the glass long gone, its roof (the people's: a flat roof and its lift house, a
// gable, a vault, a saddle, ...) on the people's plan (a round block, a bent one, a block round its court); on pilotis where the people
// raised its houses; a mall (`style` 1): long and low under a shell or a vault
void genBlock(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Dc = g.D(MT_CONCRETE);
    g.sink = -c.buried * Dc * 0.4;
    if (g.gv(GV_MASSING) == MS_BURIED) g.sink -= std::min(6.0, 0.25 * b.hWall);
    double y0 = 0;
    if (b.found == FD_STILTS && b.style == 0) y0 = stilts(g, b.hw, b.hd, 4.5) ? 4.5 : 0;
    // a mall's shell or vault (a people of domes or saddles its own), a slab's roof the age's (B-414)
    const int rf = b.style == 1 ? (c.roof == RF_DOMED || c.roof == RF_SADDLE || c.roof == RF_VAULTED ? c.roof : (c.technique == TQ_SHELL ? RF_SADDLE : RF_VAULTED)) : ageRoof(c, c.roof);
    const int rm = roofMaterial(g, rf, MT_CONCRETE);
    if (b.style == 0 && (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE)) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = 0.45; R.mat = MT_CONCRETE; R.pattern = 1; R.y0 = y0;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        R.crenels = g.gv(GV_TOPS) == TP_CRENELS;
        const double hmin = ringWalls(g, R);
        const double rise = roundRoof(g, rf, 0, 0, R.rx + 0.2, R.rz + 0.2, R.segs, hmin, b.hWall, 0.5, rm);
        if (hmin >= 0.92 * b.hWall) roundTops(g, rf, g.sink + y0 + hmin, rise, 1.4, rm);
        if (g.lod == 0 && hmin < 0.7 * b.hWall) g.heap(0, -R.rz - 3, R.rx * 0.9, 4, std::min(5.0, (b.hWall - hmin) * 0.12), 0, MT_CONCRETE);
        return;
    }
    if (b.style == 0 && b.form == PF_COURT) {   // a block round its court
        Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = frontDoorPos(b, b.hw - 4); W.mat = MT_CONCRETE; W.pattern = 1; W.y0 = y0;
        double hs[4]; const double hmin = walls4(g, W, hs);
        Walls I = W; I.hw = b.hw - 6; I.hd = b.hd - 6; I.k0 = 30; I.glyphs = false;
        double hi[4]; const double him = walls4(g, I, hi);
        const double hr = std::min(hmin, him);
        if (hr > 0.95 * b.hWall)
            for (int s = 0; s < 4; s++) {
                const bool al = s % 2 == 0;
                g.box(al ? 0 : (s == 1 ? 1 : -1) * (b.hw - 3), al ? (s == 0 ? 1 : -1) * (b.hd - 3) : 0, 0, al ? b.hw + 0.15 : 3.15, al ? 3.15 : b.hd - 6, g.sink + y0 + hr - 0.3,
                        g.sink + y0 + hr + 0.3, 0, MT_CONCRETE, 2);
            }
        if (g.lod == 0 && hmin < 0.7 * b.hWall) g.heap(0, -b.hd - 3, b.hw * 0.9, 4, std::min(5.0, (b.hWall - hmin) * 0.12), 0, MT_CONCRETE);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.4; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = MT_CONCRETE; W.pattern = b.style == 1 ? 2 : 1; W.y0 = y0;
    const bool bent = b.form == PF_FREE && b.style == 0;
    if (bent) { W.cz = b.hd * 0.55; W.hd = b.hd * 0.45; }   // a bar along the front and a wing back from one end (`buildingRoom` reads the bar)
    double hs[4];
    double hmin;
    if (rf == RF_FLAT && b.style == 0) {
        W.crenels = g.gv(GV_TOPS) == TP_CRENELS;
        hmin = walls4(g, W, hs);
        if (hmin > 0.95 * b.hWall) {   // the flat roof, its lift house (B-415: a lift house a module's where the people built in cells)
            const double se = g.slabEave(), th = g.c.genome.of(GV_EAVES) == EV_FLARED ? 0.9 : 0.3;   // its slab flush, a lip, deep, a cornice
            g.box(W.cx, W.cz, 0, W.hw + se, W.hd + se, g.sink + y0 + hmin - 0.3, g.sink + y0 + hmin + th, g.roofSlot(MT_CONCRETE), MT_CONCRETE, 2);
            const int nl = g.gv(GV_MODULAR) == 1 ? std::max(2, (int)std::lround(W.hw * 2 / 11)) : 1;
            if (g.lod < 2)
                for (int k = 0; k < nl; k++)
                    g.box(W.cx + (nl == 1 ? W.hw * 0.3 : -W.hw + (k + 0.5) * 2 * W.hw / nl), W.cz, 0, nl == 1 ? 2.5 : std::min(2.5, W.hw / nl * 0.55), std::min(2.2, W.hd - 0.3),
                            g.sink + y0 + hmin + 0.3, g.sink + y0 + hmin + 3.2, 0, MT_CONCRETE);
            houseTops(g, RF_FLAT, W.cx, W.cz, W.hw, W.hd, g.sink + y0 + hmin + 0.3, 0, MT_CONCRETE, MT_CONCRETE);
        }
        if (b.style == 0 && !bent) houseDress(g, W, hs, hmin, RF_FLAT, 0.3, MT_CONCRETE, true);
    } else hmin = rectBody(g, W, rf, rm, hs, b.style == 0);
    if (bent && g.lod < 2) {   // the wing bent back from one end
        Walls V = W; V.cx = (g.U(43) < 0.5 ? -1 : 1) * b.hw * 0.7; V.cz = -b.hd * 0.45; V.hw = b.hw * 0.3; V.hd = b.hd * 0.55; V.door = -1; V.k0 = 30; V.glyphs = false; V.crenels = false;
        double hv[4]; const double hmv = walls4(g, V, hv);
        roof(g, rf == RF_OPEN ? RF_FLAT : rf, V.cx, V.cz, V.hw, V.hd, hmv, V.h, 0.5, rm);
    }
    if (g.lod == 0 && hmin < 0.7 * b.hWall) g.heap(0, -b.hd - 3, b.hw * 0.9, 4, std::min(5.0, (b.hWall - hmin) * 0.12), 0, MT_CONCRETE);
    foundation(g, b.hw, b.hd, 0, W.doorPos, MT_CONCRETE);
    if (b.style == 0) yardOf(g, b.hw, b.hd, 0, W.doorPos, MT_CONCRETE);
}

void genSuburb(Gen& g) {   // a house of one or two storeys, its roof (the people's, or another where its houses vary), on the people's plan
    const Building& b = g.b; const Culture& c = g.c;
    const double Dm = g.D(MT_MASONRY);
    g.sink = -c.buried * Dm * 0.4;
    const int rf = ageRoof(c, houseRoof(g)), rm = roofMaterial(g, rf, MT_MASONRY);   // B-414: the age's
    if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = 0.35; R.mat = MT_MASONRY; R.pattern = 1;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        const double hmin = ringWalls(g, R);
        roundRoof(g, rf, 0, 0, R.rx + 0.3, R.rz + 0.3, R.segs, hmin, b.hWall, 0.5, rm);
        foundation(g, R.rx, R.rz, 0, 0, MT_MASONRY);
        ringDress(g, R, hmin);
        yardOf(g, R.rx, R.rz, -1, 0, MT_MASONRY);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.35; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = MT_MASONRY; W.slot = 0; W.pattern = 1;
    double hs[4]; rectBody(g, W, rf, rm, hs, false);
    double dx, dz; doorMid(0, W.doorPos, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, MT_MASONRY);
    foundation(g, b.hw, b.hd, 0, W.doorPos, MT_MASONRY);
    yardOf(g, b.hw, b.hd, 0, W.doorPos, MT_MASONRY);
}

// a tower: its lobby a storey of walls round a room with its doorway, the shaft over it to `H` in the people's form (`Building::style`:
// 0 a slab, 1 setbacks, 2 a cylinder, 3 a prism of the people's number, 4 boxes cantilevered off a core, 5 a shell's waist), its tops
// (a mast, a spire, a crown of merlons, a statue); the glass of its curtain gone at once (the bands of its frame empty), the concrete
// falling over centuries: the shaft snapped lower and lower, its top lying across the ground beside it, to a stump
void genHighrise(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Dc = g.D(MT_CONCRETE), H = b.hWall, lob = 6.0;
    g.sink = -c.buried * Dc * 0.3;
    const double share = tallShare(Dc, g.U(10)), top = std::max(lob + 2, H * share);
    const int form = b.style;
    // the lobby stands whole while the shaft stands on it, and falls with the rest once the shaft is gone
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = lob; W.thick = 0.5; W.door = 0; W.doorPos = 0; W.mat = MT_CONCRETE; W.pattern = 2; W.k0 = 50; W.whole = Dc < 0.9; W.dress = false;
    double hs[4];
    if (g.lod == 0 || Dc >= 0.9) walls4(g, W, hs);
    const double y0 = g.sink + (g.lod == 0 ? lob : 0);
    const int sides = clampi(c.genome.number(), 3, 8);
    if (top > lob + 1 && Dc < 0.9) {
        const double brk = share < 0.97 ? clampd(4.0 / std::max(8.0, top - lob), 0.05, 0.4) : 0;
        const double cut = (g.sink + top - y0) / std::max(1.0, g.sink + H - y0);   // the share of the shaft standing
        if (form == 1 && cut >= 0.36) {   // setbacks, their top tiers gone (a pyramid's break cuts at most two thirds of it)
            Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hd, y0, g.sink + H, 0, MT_CONCRETE); e.p[0] = clampi(c.genome.number(), 3, 6); e.p[1] = 0.45; e.p[2] = 1; e.pattern = 2;
            e.broken = share < 0.97 ? clampd((1 - cut) / 0.65, 0.02, 1) : 0;
        } else if (form == 2 || form == 3) {
            Form& e = g.add(FK_LATHE, 0, 0, 0, 0, std::min(b.hw, b.hd), b.form == PF_ELLIPSE ? std::max(b.hw, b.hd) : std::min(b.hw, b.hd), y0, g.sink + top, 0, MT_CONCRETE);
            e.sides = form == 3 ? sides : 0;
            e.pattern = 2;
            e.broken = brk;
        } else if (form == 4) {   // boxes stacked off a core, each turned or slid off the one under it
            const int n = clampi((int)((H - lob) / 40) + 2, 3, 7);
            for (int k = 0; k < n; k++) {
                const double ya = y0 + (g.sink + H - y0) * k / n, yb = y0 + (g.sink + H - y0) * (k + 1) / n;
                if (ya >= g.sink + top) break;
                const double off = (k % 2 ? 1 : -1) * b.hw * 0.22;
                Form& e = g.box(off, -off * 0.5, k % 2 ? 0.0 : 0.12, b.hw * 0.8, b.hd * 0.85, ya, std::min(yb, g.sink + top), 0, MT_CONCRETE); e.pattern = 2; e.broken = yb > g.sink + top ? brk : 0;
            }
        } else if (form == 5) {
            Form& e = g.lathe(3, 0, 0, std::min(b.hw, b.hd), y0, g.sink + H, 0, MT_CONCRETE);
            e.p[0] = 0.66;
            e.p[1] = 0.62;
            e.pattern = 2;
            e.broken = share < 0.97 ? clampd(1 - cut, 0.05, 0.95) : 0;
        } else {
            Form& e = g.box(0, 0, 0, b.hw, b.hd, y0, g.sink + top, 0, MT_CONCRETE);
            e.pattern = 2;
            e.broken = brk;
        }
        if (share >= 0.97 && g.lod < 2) {   // its tops: the people's
            const int tp = g.gv(GV_TOPS);
            const double rr = std::min(b.hw, b.hd) * (form == 5 ? 0.66 : 1.0);
            const double yT = form == 1 || form == 5 ? g.sink + H : g.sink + top;   // B-415: on the shaft's top as it stands (a mast floated over a slab short of its height)
            if (tp == TP_MASTS || (tp == TP_NOTHING && g.U(12) < 0.3)) {
                Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 1.2, 1.2, yT, yT + H * 0.15, g.ironSlot(), MT_IRON);
                m.sides = 3;
                m.p[0] = 0.3;
                m.broken = g.D(MT_IRON) < 0.3 ? 0 : 0.6;
            } else if (tp == TP_FINIALS || g.gv(GV_FOCUS) == FO_SPIRE) {
                Form& s = g.lathe(2, 0, 0, rr * 0.5, yT, yT + H * (g.gv(GV_FOCUS) == FO_SPIRE ? 0.3 : 0.12), g.topSlot(MT_CONCRETE), MT_CONCRETE, form == 3 ? sides : 0);
                s.p[0] = 1.4;
            } else if (tp == TP_CRENELS && g.lod == 0) {
                const int n = 12;
                for (int k = 0; k < n; k += 2) {
                    double a = k * TAU / n;
                    g.box(std::sin(a) * rr * 0.9, std::cos(a) * rr * 0.9, a, rr * PI / n, 0.5, yT, yT + 4.0, g.topSlot(MT_CONCRETE), MT_CONCRETE);
                }
            } else if (tp == TP_STATUES) figure(g, 0, 0, yT, std::min(40.0, H * 0.1), g.topSlot(MT_CONCRETE), MT_CONCRETE, Dc);
        }
        if (share < 0.97 && g.lod == 0) {   // the top where it fell: across the ground away from the town's middle, and the heap at the foot on that side (the lobby clear)
            const double dir = std::atan2(b.x, b.z) - b.heading + (g.U(13) - 0.5) * 1.2, len = std::min((H - top) * 0.5, 120.0);
            const double rr = std::min(b.hw, b.hd) * 0.85, out = std::max(b.hw, b.hd) * 1.9, fx = std::sin(dir) * (std::max(b.hw, b.hd) + 1), fz = std::cos(dir) * (std::max(b.hw, b.hd) + 1);
            if (form == 2 || form == 3 || form == 5) g.fallen(FK_LATHE, 0, fx, fz, dir, rr, len, 0, MT_CONCRETE, form == 3 ? sides : 0, 2);
            else g.fallen(FK_BOX, 0, fx, fz, dir, rr, len, 0, MT_CONCRETE, 0, 2);
            g.heap(std::sin(dir) * out, std::cos(dir) * out, b.hw * 0.9, b.hd * 0.9, std::min(9.0, (H - top) * 0.04 + 1), 0, MT_CONCRETE);
        }
    } else if (g.lod == 0) {
        const double dir = std::atan2(b.x, b.z) - b.heading, out = std::max(b.hw, b.hd) * 1.9;
        g.heap(std::sin(dir) * out, std::cos(dir) * out, b.hw * 0.9, b.hd * 0.9, std::min(10.0, H * 0.05 + 1), 0, MT_CONCRETE);
    }
}

// the bowl of tiers round its field, its gate a gap (A-04: of its age's material: an amphitheatre of stone or brick, a stadium of
// concrete with its light masts, guyed where the people built in tension)
void genStadium(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_MODERN ? MT_CONCRETE : greatOf(b, g.c);
    const double R = b.hw, D = g.D(mat);
    const double foot = std::max(9.0, R * 0.22);
    {
        Form& e = g.add(FK_SWEEP, 0, 0, 0, 0.18, R, R, 0, b.hWall, 0, mat);
        e.p[0] = TAU - 0.36;
        e.p[1] = foot;
        e.p[2] = 2.0;
        e.p[3] = 8;
        e.broken = D < 0.3 ? 0 : clampd(D, 0.3, 0.95);
        e.pattern = b.age >= AGE_MODERN ? 2 : 3;
    }
    if (b.age < AGE_MODERN || g.lod == 2) return;
    const bool tension = g.c.technique == TQ_TENSION;
    for (int k = 0; k < 4; k++) {
        double a = (k + 0.5) * PI / 2, rr = R + foot + 3, d = g.Dk(MT_IRON, 20 + k);
        if (d > 0.8) continue;
        const double mh = b.hWall * (tension ? 2.6 : 1.9);
        Form& m = g.add(FK_TRUSS, 0, std::sin(a) * rr, std::cos(a) * rr, 0, 1.4, 1.4, 0, mh, g.ironSlot(), MT_IRON); m.sides = 3; m.p[0] = 0.4; m.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.95);
        if (tension && d < 0.45) {
            double fx, fz;
            g.to(std::sin(a) * (R - foot * 0.3), std::cos(a) * (R - foot * 0.3), fx, fz);
            Form& cb = g.add(FK_CABLE, 0, std::sin(a) * rr, std::cos(a) * rr, 0, 0.06, 0.06, 0, mh, g.ironSlot(), MT_IRON);
            cb.p[0] = fx;
            cb.p[1] = fz;
            cb.p[2] = b.hWall;
            cb.p[3] = mh * 0.03;
        }
    }
}

void genMast(Gen& g) {   // a lattice mast held by its guys: they snap first, then the mast bends over and falls
    const Building& b = g.b;
    const double H = b.hWall, Di = g.D(MT_IRON);
    if (Di > 0.85) { if (g.lod == 0) g.fallen(FK_BOX, 0, 0, 0, g.U(10) * TAU, 1.0, H * 0.5, g.ironSlot(), MT_IRON); return; }
    Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 1.6, 1.6, 0, H, g.ironSlot(), MT_IRON); m.sides = 3; m.p[0] = 0.8; m.p[2] = 0.12; m.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95);
    if (Di < 0.45 && g.lod < 2)
        for (int k = 0; k < 3; k++) {
            double a = g.U(11) * TAU + k * TAU / 3, d = H * 0.55, fx, fz; g.to(std::sin(a) * d, std::cos(a) * d, fx, fz);
            for (int q = 0; q < 2; q++) {
                Form& cb = g.add(FK_CABLE, 0, 0, 0, 0, 0.05, 0.05, 0, H * (q ? 0.9 : 0.5), g.ironSlot(), MT_IRON);
                cb.p[0] = fx;
                cb.p[1] = fz;
                cb.p[2] = 0;
                cb.p[3] = H * 0.01;
            }
            if (g.lod == 0) g.box(std::sin(a) * d, std::cos(a) * d, a, 1.2, 1.2, 0, 1.0, 0, MT_MASS);
        }
}

void genPower(Gen& g) {   // the cooling towers in a row (A-04: as many as the people's number allows), the stack, the turbine hall
    const Building& b = g.b;
    const double Dc = g.D(MT_CONCRETE);
    const int n = clampi(g.c.genome.number() - 2, 1, 5);
    const double R = b.hd * 0.42, H = R * 2.9;
    for (int k = 0; k < n; k++) {
        double x = -b.hw + R + (2 * b.hw - 2 * R) * (n > 1 ? (double)k / (n - 1) : 0.5), d = g.Dk(MT_CONCRETE, 10 + k);
        double h = H * tallShare(d, g.U(20 + k));
        Form& t = g.lathe(3, x, 0, R, 0, h < H * 0.97 ? h : H, 0, MT_CONCRETE); t.broken = h < H * 0.97 ? clampd(1 - h / H, 0.1, 0.9) : 0;
    }
    const double sH = H * 1.8, sh = sH * tallShare(Dc * 1.05, g.U(30));
    Form& s = g.lathe(5, b.hw * 0.2, b.hd + 8, 5.5, 0, sh, 0, MT_CONCRETE); s.p[0] = 0.55; s.broken = sh < sH * 0.97 ? 0.25 : 0;
    if (sh < sH * 0.97 && g.lod == 0) g.fallen(FK_LATHE, 0, b.hw * 0.2, b.hd + 8, PI / 2 + g.U(31) - 0.5, 4.5, (sH - sh) * 0.5, 0, MT_CONCRETE);
    Form& hall = g.box(-b.hw * 0.3, -b.hd - 14, 0, b.hw * 0.5, 12, 0, H * 0.3 * wallShare(Dc, g.U(32)), 0, MT_CONCRETE); hall.pattern = 1;
}

void genTanks(Gen& g) {   // a row of tanks or silos: iron rusting through, or concrete with their domed caps
    const Building& b = g.b;
    const int n = clampi(g.c.genome.number() - 1, 2, 6);
    const bool silos = g.U(2) < 0.5;
    const double r = std::min(b.hd, b.hw * 2 / n) * 0.8;
    for (int k = 0; k < n; k++) {
        double x = -b.hw + b.hw * 2 * (k + 0.5) / n;
        if (silos) {
            double d = g.Dk(MT_CONCRETE, 10 + k), h = b.hWall * tallShare(d, g.U(20 + k));
            Form& s = g.lathe(0, x, 0, r * 0.7, 0, h, 0, MT_CONCRETE); s.broken = h < b.hWall * 0.97 ? 0.3 : 0;
            if (h >= b.hWall * 0.97) g.add(FK_DOME, 1, x, 0, 0, r * 0.7, r * 0.7, h, h + r * 0.35, g.roofSlot(MT_CONCRETE), MT_CONCRETE, 2);
        } else {
            double d = g.Dk(MT_IRON, 10 + k);
            if (d > 0.9) continue;
            Form& t = g.lathe(4, x, 0, r, 0, b.hWall * 0.6, g.roofSlot(MT_IRON), MT_IRON); t.p[0] = 0.25; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.9);
        }
    }
}

// ---- the futuristic age -------------------------------------------------------------------------------------------------------

namespace {
void portal(Gen& g, double z0, int mat) {
    if (g.lod == 2) return;
    const double d = g.D(mat);
    Form& v = g.add(FK_VAULT, 2, 0, z0 + PORTAL_HD, 0, PORTAL_HW, PORTAL_HD, 0, PORTAL_H, g.accentOn(AP_DOORS) ? 1 : 0, mat); v.p[0] = 0.8; v.p[1] = 0;
    v.broken = d < 0.35 ? 0 : clampd(d, 0.35, 0.9);
}
// the people's bands round a great turned form (an ornamented people's) at shares of its height
void bandsUp(Gen& g, double r, double H, double pw, int n) {
    if (g.lod == 2 || !g.banded()) return;
    for (int k = 1; k <= n; k++) {
        const double t = (double)k / (n + 1), rr = r * std::pow(std::max(0.0, 1 - t), pw) + 0.6;
        Form& e = g.add(FK_TORUS, 0, 0, 0, 0, rr, rr, H * t, H * t + std::max(1.5, H * 0.006), 1, MT_COMPOSITE, 4);
        e.p[0] = std::max(0.6, H * 0.004);
    }
}
}   // namespace

// the great building of a futuristic town (`Building::style`, the people's technique): a stepped pyramid of the people's sides
// (printed), a cone (a shell), grown (a cluster of the people's number of tapering towers); its gate hall before it
void genArcology(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Dc = g.D(MT_COMPOSITE), H = b.hWall, r = b.hw;
    const double share = tallShare(Dc, g.U(10));
    const int num = c.genome.number();
    switch (b.style % 3) {
        case 0: {
            Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, r, b.form == PF_ELLIPSE ? r * 0.75 : r, 0, H, 0, MT_COMPOSITE);
            e.sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : (g.gv(GV_PLAN) == PF_CIRCLE ? 8 : 4);
            e.p[0] = 6 + num;
            e.p[1] = 0.18;
            e.p[2] = 0.7;
            e.pattern = 1;
            e.broken = share < 0.97 ? clampd((1 - share) / 0.65, 0.05, 1) : 0;
            break;
        }
        case 1: {
            Form& e = g.add(FK_LATHE, 1, 0, 0, 0, r, b.form == PF_ELLIPSE ? r * 0.75 : r, 0, H, 0, MT_COMPOSITE);
            e.p[0] = 0.22;
            e.pattern = 1;
            e.sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : 0;
            e.broken = share < 0.97 ? clampd((1 - share) / 0.85, 0.05, 1) : 0;
            if (share >= 0.97) bandsUp(g, r, H, 1.0, 3);
            break;
        }   // snapped where it stands to, its flank's slope kept
        default: {   // grown: a cluster of tapering towers fused at their feet, the tallest in the middle
            const int n = clampi(num, 4, 8);
            for (int k = 0; k < n; k++) {
                double a = k * TAU / n + g.U(20 + k), d = k == 0 ? 0 : r * (0.45 + 0.25 * g.U(30 + k)), rr = r * (k == 0 ? 0.55 : 0.32 + 0.14 * g.U(40 + k));
                double h = H * (k == 0 ? 1.0 : 0.35 + 0.4 * g.U(50 + k)), sk = tallShare(g.Dk(MT_COMPOSITE, 60 + k), g.U(70 + k));
                Form& e = g.lathe(2, std::sin(a) * d, std::cos(a) * d, rr, 0, h, 0, MT_COMPOSITE); e.p[0] = 1.25; e.pattern = 2;
                e.broken = sk < 0.97 ? clampd((1 - sk) / 0.85, 0.05, 1) : 0;
            }
            break;
        }
    }
    if (share >= 0.97 && g.gv(GV_TOPS) == TP_MASTS && g.lod < 2 && b.style % 3 != 2) { Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 3.0, 3.0, H, H + H * 0.18, 1, MT_COMPOSITE); m.sides = 3; m.p[0] = 0.2; }
    if (g.lod == 0 && share < 0.97) g.heap(r * 0.6, r * 0.6, r * 0.35, r * 0.3, std::min(30.0, H * 0.03 + 2), 0, MT_COMPOSITE);
    portal(g, portalZ(r), MT_COMPOSITE);
}

void genHabitat(Gen& g) {   // a dome of diamond glass on its ring (A-04: the people's dome: a saucer, a hemisphere, pointed, an onion; a cone; a vault; on its oval), its gate hall at its edge
    const Building& b = g.b; const Culture& c = g.c;
    const double r = b.hw, rz = b.form == PF_ELLIPSE ? b.hw * 0.75 : b.hw, Dc = g.D(MT_COMPOSITE);
    const double tube = std::max(2.5, r * 0.06);
    { Form& ring = g.add(FK_TORUS, 0, 0, 0, 0, r, r, 0, 8, 0, MT_COMPOSITE); ring.p[0] = tube; }
    if (Dc < 0.8) {
        const double brk = Dc < 0.3 ? 0 : clampd(Dc, 0.3, 0.95), y0 = tube * 1.2;
        switch (c.roof) {
            case RF_CONICAL: case RF_PITCHED: { Form& e = g.add(FK_LATHE, 1, 0, 0, 0, r, rz, y0, y0 + r * 0.9, 2, MT_COMPOSITE); e.p[0] = 0.05; e.broken = brk; break; }
            case RF_VAULTED: { Form& e = g.add(FK_VAULT, 2, 0, 0, 0, rz, r, y0, y0 + rz * 0.8, 2, MT_COMPOSITE); e.p[0] = 0.5; e.p[1] = 1; e.broken = brk; break; }
            case RF_STEPPED: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, r, rz, y0, y0 + r * 0.55, 2, MT_COMPOSITE); e.sides = 8; e.p[0] = 4; e.p[1] = 0.2; e.p[2] = 0.6; e.broken = brk; break; }
            case RF_SADDLE: { Form& e = g.add(FK_SHELL, 0, 0, 0, 0, r, rz, y0, y0 + r * 0.5, 2, MT_COMPOSITE); e.broken = brk; break; }
            default: {
                const int prof = c.roof == RF_DOMED ? (g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED ? 2 : 0)) : 1;
                Form& d = g.add(FK_DOME, prof, 0, 0, 0, r, rz, y0, y0 + r * (prof == 1 ? 0.55 : (prof == 0 ? 0.9 : 1.2)), 2, MT_COMPOSITE); d.broken = brk;
                break;
            }
        }
    }
    portal(g, portalZ(r), MT_COMPOSITE);
}

void genSpire(Gen& g) {   // a needle of a vertical town (A-04: its flank by the people's profile, its collars by its motif, a mast on its tip), the gate hall at its foot
    const Building& b = g.b;
    const double r = b.hw, H = b.hWall, Dc = g.D(MT_COMPOSITE), share = tallShare(Dc, g.U(10));
    const int prof = g.gv(GV_PROFILE);
    const double pw = (prof == WP_BATTERED ? 0.9 : (prof == WP_LEANING ? 2.4 : (prof == WP_BULGING ? 0.7 : 1.5))) + 0.6 * g.U(11), broken = share < 0.97 ? clampd((1 - share) / 0.85, 0.05, 1) : 0;
    const int sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(g.c.genome.number(), 3, 8) : 0;
    { Form& s = g.lathe(2, 0, 0, r, 0, H, 0, MT_COMPOSITE, sides); s.p[0] = pw; s.pattern = 2; s.broken = broken; }   // (a reference to a piece dies with the next piece added)
    if (g.lod < 2) {
        const int collars = g.gv(GV_MOTIF) == MO_SPIRALS ? 5 : (g.gv(GV_MOTIF) == MO_CIRCLES ? 3 : 2);
        for (int k = 1; k <= collars; k++) {
            double t = (0.66 / collars) * k + 0.1 * g.U(12 + k); if (t > 1 - 0.85 * broken) break;
            double rr = r * std::pow(1 - t, pw) + 4;
            Form& c = g.add(FK_TORUS, 0, 0, 0, 0, rr, rr, H * t, H * t + 4, 1, MT_COMPOSITE); c.p[0] = 2.2;
        }
    }
    if (share >= 0.97 && g.lod < 2 && g.gv(GV_TOPS) == TP_MASTS) { Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 1.6, 1.6, H, H + H * 0.12, 1, MT_COMPOSITE); m.sides = 3; m.p[0] = 0.25; }
    if (share < 0.97 && g.lod == 0)
        g.fallen(FK_LATHE, 1, 0, 0, std::atan2(b.x, b.z) - b.heading + (g.U(13) - 0.5), r * std::pow(1 - share, pw) * 0.9 + 1, std::min(H * (1 - share) * 0.4, 400.0), 0, MT_COMPOSITE);
    portal(g, portalZ(r), MT_COMPOSITE);
}

void genRing(Gen& g) {   // a ring standing on its rim on a plinth, facing along its heading
    const Building& b = g.b;
    const double Dc = g.D(MT_COMPOSITE);
    const double tube = std::max(3.0, b.hw * 0.06);
    const bool far = b.age == AGE_FAR_FUTURE;
    { Form& e = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, 0, 2 * b.hw, far ? 0 : 1, far ? MT_ETERNAL : MT_COMPOSITE); e.p[0] = tube; e.tilt = PI / 2;
      e.broken = far || Dc < 0.35 ? 0 : clampd(Dc, 0.35, 0.95); e.pattern = far && g.gv(GV_LIGHT) != LT_NONE ? 1 : 0; }
    if (!far && g.lod < 2) g.box(0, 0, 0, tube * 3, tube * 2.2, 0, tube * 0.8, 0, MT_COMPOSITE, 2);
}

void genMaglev(Gen& g) {   // pylons along the heading, the guideway's spans on them; a span falls where its own decay says, the pylons later
    const Building& b = g.b;
    const double H = b.hWall, step = 45;
    const int n = std::max(2, (int)std::lround(2 * b.hd / step));
    std::vector<double> top(n + 1, 0);
    for (int k = 0; k <= n; k++) {
        double z = -b.hd + 2 * b.hd * k / n, d = g.Dk(MT_COMPOSITE, 10 + k);
        if (d > 0.9) continue;
        double h = H * (d < 0.5 ? 1 : tallShare(d, g.U(30 + k)));
        Form& p = g.lathe(1, 0, z, 2.2, 0, h, 0, MT_COMPOSITE, 6); p.p[0] = 0.7; p.broken = h < H * 0.97 ? 0.3 : 0;
        top[k] = h;
    }
    for (int k = 0; k < n; k++) {
        double z0 = -b.hd + 2 * b.hd * k / n, z1 = -b.hd + 2 * b.hd * (k + 1) / n, d = g.Dk(MT_COMPOSITE, 60 + k);
        if (top[k] >= H * 0.99 && top[k + 1] >= H * 0.99 && d < 0.55) g.box(0, 0.5 * (z0 + z1), 0, 1.8, 0.5 * (z1 - z0), H, H + 2.4, 1, MT_COMPOSITE);
        else if (g.lod == 0 && d < 0.9) g.box(1.5 * (g.U(80 + k) - 0.5), 0.5 * (z0 + z1), 0.06, 1.8, 0.5 * (z1 - z0) * 0.9, 0, 2.4, 1, MT_COMPOSITE, 3);
    }
}

void genWreck(Gen& g) {   // a hull fallen from orbit: lying broken, half in the ground, its pieces strewn behind it along its furrow
    const Building& b = g.b;
    const double r = b.hw, L = b.hd * 2;
    Form& e = g.lathe(0, 0, -b.hd, r, -r * 0.45, -r * 0.45 + L, 2, MT_COMPOSITE, g.U(1) < 0.5 ? 0 : 6); e.tilt = PI / 2; e.broken = 0.5 + 0.4 * g.U(2);
    if (g.lod == 0)
        for (int k = 0; k < 4; k++) {
            double t = g.U(10 + k);
            Form& f = g.add(FK_BLOB, 0, (g.U(20 + k) - 0.5) * r * 3, -b.hd - 20 - t * L, g.U(30 + k) * TAU, r * 0.35, r * 0.3, 0, r * 0.5, 2, MT_COMPOSITE, 3);
            f.p[0] = 1;
            f.p[1] = 0.8;
        }
    const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
    Form& furrow = g.add(FK_SWEEP, 1, 0, -b.hd - L * 0.6, 0, 0, L * 0.6, 0, std::max(0.3, r * 0.25 * (1 - 0.7 * De)), 0, MT_EARTH, 8); furrow.p[1] = r * 1.3; furrow.p[2] = r * 0.4;
}

// ---- the far future: one structure, nothing weathering ---------------------------------------------------------------------

// the people's one structure (its focus: a spike, a ring, pillars, a shaft, a solid), its proportions, its number (a spike's facets, the
// pillars), its plan (the solid: a cube, a sphere half sunk, a prism of its number, an ellipsoid, a tetrahedron), its light (glowing
// lines up it, lamps)
void genFar(Gen& g) {
    const Building& b = g.b;
    const int num = g.c.genome.number(), light = g.gv(GV_LIGHT), pat = light != LT_NONE ? 1 : 0;
    switch (b.kind) {
        case BK_SPIKE: {
            Form& e = g.lathe(2, 0, 0, b.hw, 0, b.hWall, 0, MT_ETERNAL, g.U(1) < 0.4 || g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : 0);
            e.p[0] = 2.2 + 0.8 * g.U(2);
            e.pattern = pat;
            break;
        }
        case BK_HALO: genRing(g); break;
        case BK_PILLARS: {
            const int n = std::max(3, std::min(num, (int)std::lround(2 * b.hd / (b.hw * 2.5)))), sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : (g.gv(GV_PLAN) == PF_RECT ? 4 : 0);
            for (int k = 0; k < n; k++) { Form& e = g.lathe(0, 0, -b.hd + 2 * b.hd * k / std::max(1, n - 1), b.hw, 0, b.hWall, 0, MT_ETERNAL, sides); e.pattern = pat; }
            break;
        }
        case BK_SHAFT: {   // a ring flat on the ground round a disc of black: the mouth of a shaft
            const double tube = std::max(3.0, b.hw * 0.07);
            { Form& e = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, 0, 2, 1, MT_ETERNAL); e.p[0] = tube; e.pattern = pat; }
            g.lathe(0, 0, 0, b.hw - tube * 0.5, 0, 0.25, 2, MT_ETERNAL);
            break;
        }
        default: {   // a solid: by the people's plan
            switch (g.gv(GV_PLAN)) {
                case PF_RECT: case PF_COURT: { Form& e = g.box(0, 0, 0, b.hw, b.hw, 0, b.hWall, 0, MT_ETERNAL); e.pattern = pat; break; }
                case PF_POLYGON: { Form& e = g.lathe(0, 0, 0, b.hw, 0, b.hWall, 0, MT_ETERNAL, clampi(num, 3, 8)); e.pattern = pat; break; }
                case PF_ELLIPSE: { Form& e = g.add(FK_LATHE, 4, 0, 0, 0, b.hw, b.hw * 0.6, -b.hw * 0.2, b.hw * 1.4, 0, MT_ETERNAL); e.p[0] = 1.0; e.pattern = pat; break; }
                case PF_FREE: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hw, 0, b.hWall, 0, MT_ETERNAL); e.sides = 3; e.p[1] = 0; e.pattern = pat; break; }
                default: { Form& e = g.lathe(4, 0, 0, b.hw, -b.hw * 0.2, b.hw * 1.8, 0, MT_ETERNAL); e.p[0] = 1.0; e.pattern = pat; break; }
            }
            break;
        }
    }
}
