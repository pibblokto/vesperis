// E-09: the works: the genome's own (the focal accent, the aqueduct, the joined towers, the portal), the harbour's and the
// water's (T-02), and the works' kinds outside the towns (T-03)
#include "buildings_internal.h"

// ---- A-04: the genome's own works -----------------------------------------------------------------------------------------------

// the focal accent (`Building::style`, the people's `Focus`) in its age's way: a spire on its base, a great dome on its drum, a free
// arch (a trilithon before the medieval age), a great tower (a broch), a colossus on its pedestal (a carved figure stone), a mast (a
// pole, a lattice tower, a guyed mast), a standing ring (a holed stone, a wheel), an obelisk (a menhir), a pool in its rim, a great tree
void genFocus(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat), H = b.hWall;
    const bool pre = b.age == AGE_PREHISTORIC;
    g.sink = -c.buried * Dm;
    switch (b.style) {
        case FO_SPIRE: {
            if (pre) { standingStone(g, 0, 0, 0, b.hw * 0.35, b.hw * 0.25, H, 100, true); break; }
            const double bh = H * 0.28;
            TowerOut T = towerShaft(g, 0, 0, b.hw, bh, mat, 10, 2);
            if (T.whole) towerTop(g, 0, 0, T.rTop, T.h, bh, mat, true, H - bh);
            break;
        }
        case FO_DOME: {
            if (pre) { const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet); g.add(FK_DOME, 1, 0, 0, 0, b.hw, b.hw, 0, std::max(0.6, H * (1 - 0.4 * De)), 0, MT_EARTH, 8); break; }   // a great mound
            Ring R; R.rx = b.hw; R.rz = b.hw; R.h = H * 0.4; R.thick = 1.2; R.mat = mat; R.segs = 16; R.doorSegs = ringDoorSegs(b.hw, 16); R.door = (16 - R.doorSegs / 2) % 16; R.pattern = 3;
            const double hmin = ringWalls(g, R);
            roundRoof(g, RF_DOMED, 0, 0, b.hw + 0.6, b.hw + 0.6, 16, hmin, R.h, 0.8, mat);
            if (hmin >= R.h * 0.92 && Dm < 0.3) topsAt(g, {{0, 0}}, g.sink + hmin + b.hw * (0.85 + 0.3 * g.gf(GV_PROPORTION)), 1.6, mat);
            break;
        }
        case FO_GATE: {
            if (pre) {   // a trilithon
                const double d0 = g.Dk(MT_DRYSTONE, 10), d1 = g.Dk(MT_DRYSTONE, 11);
                standingStone(g, -b.hw * 0.5, 0, 0, 0.9, 0.6, H, 10, false); standingStone(g, b.hw * 0.5, 0, 0, 0.9, 0.6, H, 11, false);
                if (d0 < 0.3 && d1 < 0.3) g.box(0, 0, 0, b.hw * 0.5 + 1.0, 0.6, H, H + 1.0, 0, MT_DRYSTONE, 4);
                break;
            }
            Form& e = g.add(FK_ARCH, archSub(g), 0, 0, 0, b.hw, std::max(1.2, b.hw * 0.18), g.sink, g.sink + H, g.accentOn(AP_DOORS) ? 1 : 0, mat);
            e.p[0] = 1; e.p[1] = 0.32; e.p[2] = H * 0.55; e.broken = Dm < 0.35 ? 0 : clampd(Dm, 0.35, 0.85);
            if (Dm < 0.3) topsAt(g, {{-b.hw * 0.75, 0}, {b.hw * 0.75, 0}}, g.sink + H, 1.5, mat);
            break;
        }
        case FO_TOWER: {
            TowerOut T = towerShaft(g, 0, 0, b.hw, H, pre ? MT_DRYSTONE : mat, 10, 2);
            if (T.whole && !pre && c.age >= AGE_MODERN && g.lod < 2) { Form& p = g.add(FK_LATHE, 4, 0, 0, 0, b.hw * 2.2, b.hw * 2.2, g.sink + H * 0.8, g.sink + H * 0.88, 0, mat); p.p[0] = 0.3; }   // B-415: a modern people's tower of the air, its pod
            if (T.whole && !pre) towerTop(g, 0, 0, T.rTop, T.h, H, mat, false, 0);
            break;
        }
        case FO_COLOSSUS: {
            if (pre) { standingStone(g, 0, 0, 0, b.hw * 0.4, b.hw * 0.3, H * 0.8, 100, true); if (g.Dk(MT_DRYSTONE, 100) < 0.3) g.add(FK_BLOB, 0, 0, 0, 0, b.hw * 0.3, b.hw * 0.3, H * 0.8, H * 0.8 + b.hw * 0.55, 0, MT_DRYSTONE); break; }
            const double ph = H * 0.18;
            g.box(0, 0, 0, b.hw, b.hw, g.sink, g.sink + ph, 0, mat, 5);
            figure(g, 0, 0, g.sink + ph, H - ph, g.accentOn(AP_TOPS) ? 1 : 0, mat, Dm);
            break;
        }
        case FO_MAST: {
            if (b.age <= AGE_MEDIEVAL) {   // a great pole with its cross arms (a timber's: a stump, its hole)
                g.post(0, 0, H, std::max(0.35, b.hw * 0.12), 10);
                if (g.D(MT_TIMBER) < 0.3 && g.lod < 2) for (int k = 0; k < 2; k++) g.box(0, 0, k * PI / 2, b.hw * 0.5, 0.15, H * 0.82, H * 0.82 + 0.4, g.timberSlot(), MT_TIMBER, 4);
                break;
            }
            const double Di = g.D(b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON);
            Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, b.hw, b.hw, 0, H, b.age >= AGE_FUTURISTIC ? 1 : g.ironSlot(), b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON); m.sides = 4; m.p[0] = b.age == AGE_INDUSTRIAL ? 0.12 : 0.5; m.p[2] = 0.2; m.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95);
            if (b.age >= AGE_MODERN && Di < 0.45 && g.lod < 2)
                for (int k = 0; k < 3; k++) { double a = k * TAU / 3, fx, fz; g.to(std::sin(a) * H * 0.5, std::cos(a) * H * 0.5, fx, fz); Form& cb = g.add(FK_CABLE, 0, 0, 0, 0, 0.06, 0.06, 0, H * 0.8, g.ironSlot(), MT_IRON); cb.p[0] = fx; cb.p[1] = fz; cb.p[2] = 0; cb.p[3] = H * 0.01; }
            break;
        }
        case FO_RING: {
            const double tube = std::max(0.6, b.hw * (pre ? 0.25 : 0.1));
            Form& e = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, pre ? 0 : 1.0, (pre ? 0 : 1.0) + 2 * b.hw, b.age == AGE_INDUSTRIAL ? g.ironSlot() : (g.accentOn(AP_TOPS) ? 1 : 0), b.age == AGE_INDUSTRIAL ? MT_IRON : (pre ? MT_DRYSTONE : mat));
            e.p[0] = tube; e.tilt = PI / 2; e.broken = Dm < 0.35 ? 0 : clampd(Dm, 0.35, 0.9);
            if (!pre) g.box(0, 0, 0, tube * 2.5, tube * 2.0, 0, 1.2, 0, mat, 2);
            break;
        }
        case FO_OBELISK: {
            if (pre) { standingStone(g, 0, 0, 0, b.hw * 0.4, b.hw * 0.3, H, 100, true); break; }
            const double h = H * (Dm < 0.4 ? 1 : 1 - 0.5 * Dm * g.U(10));
            Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw * 0.5, b.hw * 0.5, g.sink, g.sink + h, 0, mat, 5); e.p[1] = 0.62; e.sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 4;
            if (h >= H * 0.97) { Form& t = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw * 0.31, b.hw * 0.31, g.sink + h, g.sink + h + b.hw * 0.6, g.topSlot(mat), mat, 5); t.p[1] = 0; t.sides = e.sides; }
            g.box(0, 0, 0, b.hw * 0.9, b.hw * 0.9, g.sink, g.sink + 0.8, 0, mat, 4);
            break;
        }
        case FO_POOL: {   // a basin in its rim (no higher than a step), the water in it on a wet world, its floor dry on a desert
            Ring R; R.rx = b.hw; R.rz = b.hd; R.h = 0.8; R.thick = 0.8; R.mat = pre ? MT_DRYSTONE : mat; R.segs = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 9) : 20; R.open = true; R.dress = false; R.whole = true;
            ringWalls(g, R);
            if (c.wet > 0.35 && !g.st.desert) { Form& w = g.add(FK_LATHE, 0, 0, 0, 0, b.hw - 0.4, b.hd - 0.4, -0.4, 0.45, 0, -1, 9); w.sides = R.segs <= 8 ? R.segs : 0; }
            if (g.lod == 0 && b.age >= AGE_MEDIEVAL) { Form& f = g.lathe(0, 0, 0, 0.8, g.sink, g.sink + 2.4, g.accentOn(AP_TOPS) ? 1 : 0, mat, 8); (void)f; topsAt(g, {{0, 0}}, g.sink + 2.4, 0.8, mat); }
            break;
        }
        default: {   // a great tree: its trunk of bark, its crown of the ground's leaves while the world grows them (a dead one's trunk rotting)
            const bool alive = !g.st.desert && c.wet > 0.3;
            const double Dt = alive ? 0 : g.D(MT_TIMBER), r = b.hw * 0.16;
            if (Dt > 0.85) break;
            const double th = H * (Dt < 0.3 ? 0.55 : 0.55 * (1 - Dt));
            { Form& t = g.add(FK_LATHE, 1, 0, 0, 0, r, r, 0, th, 0, -1, 10); t.p[0] = 0.55; }
            if (!alive) break;
            for (int k = 0; k < 5; k++) {
                const double a = k * TAU / 5 + g.U(20 + k), d = k == 0 ? 0 : b.hw * 0.45, rr = b.hw * (k == 0 ? 0.55 : 0.4);
                Form& cr = g.add(FK_BLOB, 0, std::sin(a) * d, std::cos(a) * d, a, rr, rr * 0.9, th * 0.8, H * (k == 0 ? 1.0 : 0.85), 0, -1, 11); cr.p[0] = 2; cr.p[1] = 0.5;
            }
            break;
        }
    }
    if (b.style == FO_SPIRE || b.style == FO_TOWER || b.style == FO_OBELISK || b.style == FO_COLOSSUS) motifOn(g, 0, 0, b.hw * 0.6, b.hw * 0.6, 0, 1, mat);   // on the ground round it (statues, a star)
}

// A-04: an aqueduct (a people of the valleys): its arcade along its heading, two tiers where it runs high, its channel on top; the
// prehistoric age's a leat (an earth channel)
void genAqueduct(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.age == AGE_PREHISTORIC) { const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet); Form& e = g.add(FK_SWEEP, 1, 0, 0, 0, 0, b.hd, 0, std::max(0.3, 0.8 * (1 - 0.6 * De)), 0, MT_EARTH, 8); e.p[1] = 1.6; e.p[2] = 0.6; return; }
    const int mat = greatOf(b, c); const double Dm = g.D(mat), H = b.hWall;
    const int sub = archSub(g);
    const int tiers = H > 28 ? 3 : (H > 14 ? 2 : 1), n = std::max(3, (int)std::lround(2 * b.hd / 7.0));
    const bool abs = b.base > -1e8;   // T-03: a work's arcade from its line's low ground to its channel, over the reference level
    const double y00 = abs ? b.base : g.sink;
    for (int t = 0; t < tiers; t++) {
        const double y0 = H * t / tiers, y1 = H * (t + 1) / tiers;
        if (t > 0 && Dm > 0.5) break;   // the upper tier gone first
        Form& e = g.add(FK_ARCH, sub, 0, 0, PI / 2, b.hd, 1.2, y00 + y0, y00 + y1, 0, mat, t ? 4 : 0);
        e.p[0] = n * (t ? 2 : 1); e.p[1] = 0.34; e.p[2] = (y1 - y0) * 0.55; e.broken = Dm < 0.3 ? 0 : clampd(Dm, 0.3, 0.9); e.absolute = abs;
    }
}

// A-04: two towers joined by a bridge (a people of bridges): towers in the people's section (keeps, chimneys, tower blocks, spires by the
// age), the span between them where both stand whole; the records at their foot between them
void genTwin(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = b.age >= AGE_MODERN ? (b.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE) : greatOf(b, c);
    const double r = std::min(b.hd, b.hw * 0.3), H = b.hWall;
    bool both = true; double hb = H;
    for (int s = -1; s <= 1; s += 2) {
        TowerOut T = towerShaft(g, s * (b.hw - r), 0, r, H, mat, 10 + (s > 0) * 3, 2);
        if (!T.whole) both = false; else hb = std::min(hb, T.h);
        if (T.whole && b.age <= AGE_INDUSTRIAL) towerTop(g, s * (b.hw - r), 0, T.rTop, T.h, H, mat, false, 0);
    }
    const double D = g.D(mat);
    if (both && D < 0.4 && g.lod < 2) {
        const double y = g.sink + H * 0.68, span = b.hw - r;
        Form& e = g.box(0, 0, 0, span, std::min(r * 0.45, 4.0), y, y + std::max(2.6, H * 0.03), g.accentOn(AP_BANDS) ? 1 : 0, mat); e.pattern = 2;
        if (b.age == AGE_FUTURISTIC) { Form& f = g.add(FK_TORUS, 0, 0, 0, 0, span * 0.3, span * 0.3, y - span * 0.3, y + span * 0.3, 1, mat, 4); f.tilt = PI / 2; f.p[0] = 0.8; }
    }
    (void)hb;
}

// A-04: a tunnel's mouth (a people of tunnels): an arched portal half sunk, its passage going down dark into the ground
void genPortal(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    const int sub = archSub(g);
    Form& e = g.add(FK_ARCH, sub, 0, 0, 0, b.hw, 0.8, -1.4, b.hWall - 1.4, g.doorSlot(), mat); e.p[0] = 1; e.p[1] = 0.4; e.p[2] = b.hWall * 0.45; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.8);
    g.box(0, -1.2, 0, b.hw * 0.82, 0.4, -1.4, b.hWall * 0.85 - 1.4, g.timberSlot(), mat, 4);   // the dark of the passage
    if (g.lod == 0) for (int s = -1; s <= 1; s += 2) g.box(s * (b.hw + 0.3), 2.5, 0, 0.35, 2.5, 0, 0.8, 0, mat, 4);   // the cutting's low walls
}

// ---- T-02: the harbour's and the water's works -------------------------------------------------------------------------------

// a crane on a quay, its jib or boom out along its heading over the water: an industrial people's jib crane (a lattice tower, its cab, the
// jib and its counterweight), a modern people's gantry (two portal frames astride the quay, the boom out over the berth and back over the
// yard); iron rusting: the jib or the boom down first, lying on the quay, then the tower bending over, then a heap
void genCrane(Gen& g) {
    const size_t first = g.out.size();
    struct Harbour { Gen& g; size_t first; ~Harbour() { for (size_t i = first; i < g.out.size(); i++) if (g.out[i].part == 0 || g.out[i].part == 3) g.out[i].part = 6; } } harbour{g, first};   // a harbour work: drawn from the water's level where its foot is wet
    const Building& b = g.b;
    const double Di = g.D(MT_IRON), H = b.hWall;
    const int slot = g.accentOn(AP_TOPS) ? 1 : g.ironSlot();
    if (Di > 0.9) { if (g.lod == 0) g.heap(0, 0, b.hw * 0.9, b.hd * 0.9, 1.4, g.ironSlot(), MT_IRON); return; }
    const double bent = Di < 0.4 ? 0 : clampd(Di, 0.4, 0.9);
    if (b.style == 0) {
        const double jib = H * (0.7 + 0.3 * g.U(5));
        Form& t = g.add(FK_TRUSS, 0, 0, 0, 0, std::min(b.hw, 1.6), std::min(b.hw, 1.6), 0, H, slot, MT_IRON); t.sides = 4; t.p[0] = 0.75; t.p[2] = 0.12; t.broken = bent;
        if (Di < 0.55) {
            g.box(0, 0, 0, 1.7, 2.0, H, H + 2.4, slot, MT_IRON);
            { Form& j = g.add(FK_TRUSS, 1, 0, jib * 0.5, 0.0, 0.55, jib * 0.5, H + 0.9, H + 2.0, slot, MT_IRON); j.p[2] = 0.07; j.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.7); }
            { Form& k = g.add(FK_TRUSS, 1, 0, -jib * 0.18, 0.0, 0.55, jib * 0.18, H + 0.9, H + 2.0, slot, MT_IRON); k.p[2] = 0.07; }
            g.box(0, -jib * 0.3, 0, 1.0, 1.0, H + 2.0, H + 3.2, 0, MT_MASS);   // the counterweight on its counter-jib
            if (g.lod < 2 && Di < 0.3) { Form& c = g.add(FK_CABLE, 0, 0, jib * 0.96, 0, 0.04, 0.04, 0, H + 0.9, slot, MT_IRON); double fx, fz; g.to(0, jib * 0.96, fx, fz); c.p[0] = fx; c.p[1] = fz; c.p[2] = H * 0.35; c.p[3] = 0.1; }
        } else if (g.lod == 0) g.fallen(FK_BOX, 0, 0, 0, 0.3 + 0.4 * g.U(6), 0.5, jib * 0.8, slot, MT_IRON);
        return;
    }
    // the gantry: its two portal frames b.hw apart across the heading, each two legs along the quay
    const double boomOut = 30 + 14 * g.U(5), boomBack = 14 + 6 * g.U(7), legs = 7.5;
    for (int s = -1; s <= 1; s += 2) {
        for (int q = -1; q <= 1; q += 2) { Form& l = g.box(s * b.hw, q * legs, 0, 0.7, 0.7, 0, H, slot, MT_IRON); l.broken = bent; }
        if (Di < 0.6) g.box(s * b.hw, 0, 0, 0.6, legs + 0.7, H - 1.6, H, slot, MT_IRON);
    }
    if (Di < 0.6) g.box(0, 0, 0, b.hw + 0.7, 1.0, H, H + 1.2, slot, MT_IRON);   // the cross girder over the two frames
    if (Di < 0.45) {
        Form& bm = g.add(FK_TRUSS, 1, 0, (boomOut - boomBack) * 0.5, 0, 1.4, (boomOut + boomBack) * 0.5, H + 1.2, H + 4.2, slot, MT_IRON); bm.p[2] = 0.18; bm.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.6);
        g.box(0, 0, 0, 2.6, 3.0, H + 4.2, H + 6.8, slot, MT_IRON);   // the machinery house over the portal
    } else if (g.lod == 0) g.fallen(FK_BOX, 0, 0, legs + 2, 0.2 * g.U(6) - 0.1, 1.2, boomOut * 0.7, slot, MT_IRON);
}

// a container yard's stacks: rows of boxes 12 m long, stacked one to four, each stack its colour (the accent, the structure, the iron);
// rusting through, the stacks sink to their lowest boxes, then to stains
void genContainers(Gen& g) {
    const Building& b = g.b;
    const double Di = g.D(MT_IRON);
    if (Di > 0.92) return;
    if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, 0, std::max(0.5, 5.2 * (1 - 0.8 * Di)), 3, MT_IRON); return; }
    const int cols = std::max(1, (int)(b.hw * 2 / 12.8)), rows = std::max(1, (int)(b.hd * 2 / 2.75));
    for (int i = 0; i < cols; i++)
        for (int j = 0; j < rows; j++) {
            const double u = g.U(100 + i * 64 + j);
            if (u < 0.08) continue;   // an empty slot
            const int stack = 1 + (int)(g.U(400 + i * 64 + j) * 4);
            const double lost = Di * (0.6 + 0.6 * g.U(700 + i * 64 + j));
            const double h = std::max(0.6, 2.6 * stack * (1 - std::min(0.85, lost)));
            const int slot = u < 0.4 ? 1 : (u < 0.7 ? 3 : 0);
            Form& e = g.box(-b.hw + 6.4 + 12.8 * i, -b.hd + 1.375 + 2.75 * j, 0, 6.0, 1.2, 0, h, slot, MT_IRON);
            e.broken = lost > 0.4 ? clampd(lost, 0.3, 0.8) : 0;
        }
}

// a town's reservoir: an industrial people's basin of masonry in its earth bank, a pump house at a corner; a modern people's round tank of
// concrete open to the sky; the walls falling with their material, the bank lasting
void genReservoir(Gen& g) {
    const Building& b = g.b;
    if (b.style == 0) {
        const double Dm = g.D(MT_MASONRY), De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
        const double h = b.hWall, t = 0.9;
        for (int k = 0; k < 4; k++) {
            const bool along = k % 2 == 0;
            const double len = along ? b.hw : b.hd, off = along ? b.hd : b.hw;
            const double share = wallShare(Dm, g.U(10 + k));
            if (share < 0.06) continue;
            Form& w = g.box(along ? 0 : (k == 1 ? off : -off), along ? (k == 0 ? off : -off) : 0, along ? 0 : PI / 2, len + t, t * 0.5, 0, h * share, 0, MT_MASONRY);
            w.broken = share < 0.9 ? clampd(1 - share, 0.1, 0.9) : 0;
            if (g.lod < 2) { Form& bk = g.box(along ? 0 : (k == 1 ? off + 2.2 : -off - 2.2), along ? (k == 0 ? off + 2.2 : -off - 2.2) : 0, along ? 0 : PI / 2, len + 3.5, 1.8, 0, std::max(0.4, h * 0.55 * (1 - 0.4 * De)), 0, MT_EARTH, 8); (void)bk; }
        }
        if (g.lod < 2) {   // the pump house
            const double share = wallShare(Dm, g.U(20));
            if (share > 0.1) { Form& p = g.box(b.hw + 5.5, b.hd + 5.5, 0, 3.0, 3.0, 0, 4.5 * share, 0, MT_MASONRY); p.pattern = share > 0.7 ? 1 : 0; }
        }
        return;
    }
    const double Dc = g.D(MT_CONCRETE);
    const double share = 1 - Dc * (0.2 + 0.6 * g.U(10));
    Form& t = g.lathe(0, 0, 0, b.hw, 0, std::max(0.6, b.hWall * share), 0, MT_CONCRETE); t.p[3] = 1; t.broken = Dc < 0.3 ? 0 : clampd(Dc, 0.3, 0.9);
    if (g.lod == 0 && Dc < 0.5) for (int k = 0; k < 4; k++) { const double a = g.U(30) * TAU + k * TAU / 4; g.box(std::sin(a) * (b.hw + 1.2), std::cos(a) * (b.hw + 1.2), a, 0.6, 1.2, 0, b.hWall * share * 0.8, 0, MT_CONCRETE); }   // its buttresses
}

// ---- T-03: the works' kinds (peoples/works.h) --------------------------------------------------------------------------------

namespace {
// the earth's slump: what the rain and the years left of a mound, a bank, a spoil heap (A-03's earth: a desert keeps them)
double earthLeft(const Gen& g) { return 1 - 0.55 * materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet); }
int frameSlot(const Gen& g, int mat) { return mat == MT_TIMBER ? g.timberSlot() : (mat == MT_COMPOSITE ? 0 : g.ironSlot()); }
}   // namespace

// earth: a round barrow (`style` 0), a long barrow (1), a conical spoil heap (2), a bank along its length (3: a rampart, a dyke), a ring bank
// round its middle (4: a henge's, a redoubt's), a flat heap of tailings (5), spoil tipped down a slope below an adit (6); grassed over or bare
// by the world
void genMound(Gen& g) {
    const Building& b = g.b;
    const double k = earthLeft(g), h = std::max(0.4, b.hWall * k);
    switch (b.style) {
        case 1: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hd, 0, h, 0, MT_EARTH, 8); e.sides = 8; e.p[1] = 0.45; e.broken = 0.6; break; }
        case 2: { Form& e = g.lathe(1, 0, 0, b.hw, 0, h, 0, MT_EARTH); e.part = 8; e.p[0] = 0.08; break; }   // a tip's cone
        case 6: { Form& e = g.add(FK_BLOB, 0, 0, 0, 0, b.hw, b.hd, 0, h, 0, MT_EARTH, 8); e.p[0] = 2; e.p[1] = 0.55; break; }   // spoil tipped down a slope: lumps
        case 3: { Form& e = g.add(FK_SWEEP, 1, 0, 0, 0, 0, b.hd, 0, h, 0, MT_EARTH, 8); e.p[1] = b.hw; e.p[2] = std::max(0.4, b.hw * 0.3); break; }
        case 4: { Form& e = g.add(FK_SWEEP, 0, 0, 0, 0, b.hw, b.hw, 0, h, 0, MT_EARTH, 8); e.p[0] = TAU; e.p[1] = b.hd; e.p[2] = std::max(0.4, b.hd * 0.3); break; }
        case 5: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hd, 0, h, 0, MT_EARTH, 8); e.sides = 7; e.p[1] = 0.8; e.broken = 0.5; break; }
        default: { Form& e = g.add(FK_DOME, 1, 0, 0, 0, b.hw, b.hw, 0, h, 0, MT_EARTH, 8); (void)e; break; }
    }
}

// a dam's wall across its valley, its forms from `base` (under the floor) to its crest at base + hWall, over the reference level (they stand
// where the valley is whatever the ground under each does): the heading looks up the reservoir, the crest runs across it (hw its half
// length), its foot hd thick. `style` 0 a masonry gravity dam (its buttresses down the face, a parapet); 1 a concrete gravity dam (its
// spillway down one side, its intake tower in the water); 2 a concrete arch bowed into the reservoir; 3 the futuristic age's wall of
// composite, a line of light along its crest. Breached when its material is half gone: a gap down its middle (the reservoir drains)
void genDam(Gen& g) {
    const Building& b = g.b;
    const int mat = b.style == 0 ? MT_MASONRY : (b.style == 3 ? MT_COMPOSITE : MT_MASS);
    const double D = g.D(mat), H = b.hWall, y0 = b.base, y1 = b.base + H, L = b.hw;
    const double top = b.style == 2 ? 2.2 : (b.style == 0 ? 2.6 : 4.5), foot = std::max(top + 1, b.hd);
    const bool breach = D > 0.5;
    const double gapHalf = breach ? L * (0.08 + 0.3 * (D - 0.5)) : 0;
    auto mark = [&](Form& e) { e.absolute = true; };
    if (b.style == 2) {   // the arch: an arc of radius Ra round a centre downstream, its chord the crest's length
        const double Ra = L * 1.45, half = std::asin(clampd(L / Ra, 0, 1)), cz = -Ra * std::cos(half);
        const double gapA = breach ? std::asin(clampd(gapHalf / Ra, 0, 1)) : 0;
        for (int side = breach ? 0 : -1; side <= (breach ? 1 : -1); side++) {
            double a0 = -half, a1 = half;
            if (breach) { if (side == 0) a1 = -gapA; else a0 = gapA; }
            Form& e = g.add(FK_SWEEP, 0, 0, cz, a0, Ra, Ra, y0, y1, 0, mat); e.p[0] = a1 - a0; e.p[1] = foot * 0.6; e.p[2] = top; e.broken = breach ? 0.15 : 0; mark(e);
        }
    } else {
        for (int side = breach ? 0 : -1; side <= (breach ? 1 : -1); side++) {
            double xa = -L, xb = L;
            if (breach) { if (side == 0) xb = -gapHalf; else xa = gapHalf; }
            Form& e = g.add(FK_SWEEP, 1, 0.5 * (xa + xb), 0, PI / 2, 0, 0.5 * (xb - xa), y0, y1, 0, mat); e.p[1] = foot; e.p[2] = top; e.broken = breach ? 0.2 : 0; mark(e);
        }
        if (g.lod < 2 && b.style == 0) {   // buttresses down the downstream face
            const int n = std::max(3, (int)(2 * L / 28));
            for (int k = 1; k < n; k++) {
                const double x = -L + 2 * L * k / n;
                if (std::fabs(x) < gapHalf + 4) continue;
                Form& e = g.box(x, -foot * 0.55, 0, 1.6, foot * 0.45, y0, y1 - 1.5, 0, mat); e.absolute = true;
            }
        }
    }
    if (g.lod == 2 || D > 0.85) return;
    // the parapet along the crest, a line of light on a futuristic wall, the intake tower, a modern spillway's chute
    if (b.style != 2 && g.lod == 0) {
        for (int s = -1; s <= 1; s += 2) {
            const double half = L - gapHalf;
            if (half < 4) continue;
            if (breach) { for (int q = -1; q <= 1; q += 2) { Form& p = g.box(q * (gapHalf + half * 0.5), s * (top - 0.3), 0, half * 0.5, 0.3, y1, y1 + 1.1, b.style == 3 ? 3 : 0, mat, 4); p.absolute = true; } }
            else { Form& p = g.box(0, s * (top - 0.3), 0, L, 0.3, y1, y1 + 1.1, b.style == 3 ? 3 : 0, mat, 4); p.absolute = true; }
        }
    }
    if (b.style == 1 || b.style == 3) {
        { Form& t = g.lathe(0, L * 0.3, foot + 22, 5.5, y0, y1 + 6, 0, mat); t.absolute = true; t.broken = D > 0.4 ? 0.4 : 0; }
        if (D < 0.4) { Form& c = g.add(FK_DOME, 1, L * 0.3, foot + 22, 0, 6.2, 6.2, y1 + 6, y1 + 8.5, g.roofSlot(mat), mat, 2); c.absolute = true; }
    }
    if (b.style == 1 && g.lod < 2) {   // the spillway: a stepped chute down the face at one end
        const int n = std::max(3, (int)(H / 8));
        for (int k = 0; k < n; k++) {
            const double t = (double)k / n;
            Form& e = g.box(L * 0.82, -top - (foot - top) * t - 3, 0, 9, 4, y0, y1 - H * t - 2, 0, mat); e.absolute = true;
        }
    }
}

// a radio dish on its pedestal: the dish of radius hw aimed up by the style's seed, rusting through (holed), then down on the ground beside
// its pedestal
void genDish(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON;
    const double D = g.D(mat), r = b.hw, mast = std::max(4.0, r * 0.75);
    { Form& p = g.lathe(1, 0, 0, std::max(1.4, r * 0.16), 0, mast * 0.45, 0, MT_CONCRETE, 6); p.p[0] = 0.6; }
    if (D > 0.78) {   // fallen: the dish lies on its rim beside its mount
        if (g.lod == 0) { Form& e = g.add(FK_SHELL, 1, r * 0.7, 0, g.U(4) * TAU, r, r, 0, 0.1, frameSlot(g, mat), mat); e.p[0] = r * 0.3; e.p[1] = 0; e.tilt = 1.3; e.broken = 0.7; }
        return;
    }
    Form& e = g.add(FK_SHELL, 1, 0, 0, 0, r, r, 0, mast, b.style == 1 ? 0 : frameSlot(g, mat), mat);
    e.p[0] = r * 0.32; e.p[1] = mast; e.tilt = 0.25 + 0.8 * g.U(5) + D * 0.4; e.broken = D < 0.35 ? 0 : clampd(D, 0.35, 0.8);
}

// a tomb: a gabled house of the dead (`style` 0), a domed one (1), a façade cut into the rock (2: its door the dark of the chamber), a
// columbarium's long block of niches (3), a tower of silence (4: a high blind ring open to the sky, its one gap)
void genTomb(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    if (b.style == 4) {
        Ring R; R.rx = b.hw; R.rz = b.hw; R.h = b.hWall; R.thick = 1.2; R.mat = mat; R.segs = 16; R.doorSegs = 1; R.door = (int)(g.U(3) * 16); R.pattern = 0; R.dress = false;
        ringWalls(g, R);
        return;
    }
    if (b.style == 2) {
        Form& e = g.add(FK_ARCH, 0, 0, 0, 0, b.hw, 0.9, -1.0, b.hWall, g.doorSlot(), mat); e.p[0] = 1; e.p[1] = 0.32; e.p[2] = b.hWall * 0.4; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.8);
        g.box(0, -1.0, 0, b.hw * 0.62, 0.3, -1.0, b.hWall * 0.62, g.timberSlot(), mat, 4);
        if (Dm < 0.5 && g.lod == 0) { Form& p = g.add(FK_PYRAMID, 0, 0, 0.2, 0, b.hw * 1.05, 0.7, b.hWall, b.hWall + b.hw * 0.4, 0, mat, 4); p.sides = 3; }
        return;
    }
    if (b.style == 3) {
        const double share = wallShare(Dm, g.U(7));
        Form& e = g.box(0, 0, 0, b.hw, b.hd, 0, std::max(0.6, b.hWall * share), 0, mat); e.pattern = 2; e.broken = share < 0.95 ? 0.3 : 0;
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = 0; W.mat = mat; W.k0 = 20; W.dress = true;
    double hs[4]; const double hmin = walls4(g, W, hs);
    roof(g, b.style == 1 ? RF_DOMED : RF_PITCHED, 0, 0, b.hw, b.hd, hmin, b.hWall, 0.5, mat);
    if (b.style != 1) gables(g, W, hs, 0.5);   // R-412: its gables and its doorway the people's
    entranceAt(g, W, wallsHalf(g, W), W.h);
    cornersOf(g, W, hs, wallsHalf(g, W), mat);
}

// a redoubt (`style` 0: a ring bank of earth with its ditch outside and a casemate of masonry in it) or a casemate fort of concrete (1: a low
// polygon, its gun ports, a cupola)
void genFort(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.style == 0) {
        const double k = earthLeft(g);
        { Form& e = g.add(FK_SWEEP, 0, 0, 0, 0, b.hw, b.hw, 0, std::max(0.5, 3.2 * k), 0, MT_EARTH, 8); e.p[0] = TAU; e.p[1] = 4.5; e.p[2] = 1.4; }
        const int mat = greatOf(b, c); const double Dm = g.D(mat), share = wallShare(Dm, g.U(5));
        Form& h = g.box(0, 0, 0, b.hw * 0.3, b.hw * 0.22, 0, std::max(0.5, 3.0 * share), 0, mat); h.pattern = share > 0.8 ? 2 : 0;
        return;
    }
    const double D = g.D(MT_MASS);
    const int pl = g.gv(GV_PLAN);   // R-412: the people's section
    Form& e = g.lathe(1, 0, 0, b.hw, 0, b.hWall, 0, MT_MASS, pl == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : (pl == PF_CIRCLE || pl == PF_ELLIPSE ? 0 : 6)); e.p[0] = 0.86; e.pattern = 2; e.broken = D < 0.4 ? 0 : clampd(D, 0.4, 0.7);
    if (D < 0.6) { Form& d = g.add(FK_DOME, 1, b.hw * 0.25, 0, 0, b.hw * 0.25, b.hw * 0.25, b.hWall, b.hWall + b.hw * 0.12, frameSlot(g, MT_IRON), MT_IRON, 2); (void)d; }
}

// a pillbox (`style` 0: round or of six sides, its slits, its slab), a blockhouse (1), a shelter's vents (2: short shafts with their caps)
void genBunker(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_MASS;
    const double D = g.D(mat);
    if (b.style == 2) {
        const int n = 2 + (int)(g.U(3) * 3);
        for (int k = 0; k < n; k++) {
            const double x = (k - (n - 1) * 0.5) * 3.2;
            g.lathe(0, x, 0, 0.7, 0, 1.6, 0, mat);
            if (D < 0.6) g.box(x, 0, 0, 1.0, 1.0, 1.6, 1.9, 0, mat, 4);
        }
        return;
    }
    if (b.style == 1) {
        Form& e = g.box(0, 0, 0, b.hw, b.hd, 0, b.hWall, 0, mat); e.pattern = 2; e.broken = D < 0.45 ? 0 : 0.3;
        return;
    }
    const int pl = g.gv(GV_PLAN);   // R-412: the people's section (round, its polygon, square)
    Form& e = g.lathe(0, 0, 0, b.hw, 0, b.hWall, 0, mat, pl == PF_POLYGON ? clampi(g.c.genome.number(), 5, 8) : (pl == PF_RECT || pl == PF_COURT ? 4 : (pl == PF_FREE ? 6 : 0))); e.pattern = 2;
    if (g.lod < 2) g.lathe(0, 0, 0, b.hw + g.slabEave() + 0.3, b.hWall, b.hWall + 0.45, 0, mat, e.sides);
}

// dragon's teeth: rows of concrete pyramids along the line, staggered, some tipped
void genTeeth(Gen& g) {
    const Building& b = g.b;
    const int rows = 2 + (int)(g.U(2) * 3), n = std::max(2, (int)(2 * b.hd / 2.2));
    if (g.lod == 2) { g.box(0, 0, 0, rows * 1.1, b.hd, 0, 1.0, 0, MT_MASS); return; }
    for (int r = 0; r < rows; r++)
        for (int k = 0; k < n; k++) {
            const double z = -b.hd + (k + 0.5 + (r % 2) * 0.5) * 2 * b.hd / n, x = (r - (rows - 1) * 0.5) * 2.2;
            if (z > b.hd) continue;
            const double d = g.Dk(MT_MASS, 100 + r * 64 + k);
            if (d > 0.9) continue;
            Form& e = g.add(FK_PYRAMID, 0, x, z, g.U(300 + k) * 0.6, 0.65, 0.65, 0, 1.0 + 0.6 * g.U(200 + r), 0, MT_MASS); e.p[1] = 0.12;
            if (d > 0.4) e.tilt = 0.2 + 0.6 * (d - 0.4);
        }
}

// a machine's wreck: a tank (`style` 0: its hull, tracks, turret and gun; the turret blown off beside it), a haul truck (1: its bed, its cab,
// its wheels), a truck (2); rusting to a shell, then a stain of iron
void genVehicle(Gen& g) {
    const Building& b = g.b;
    const double D = g.D(MT_IRON);
    if (D > 0.88) { if (g.lod == 0) g.heap(0, 0, b.hw * 0.8, b.hd * 0.8, 0.8, g.ironSlot(), MT_IRON); return; }
    const int sl = g.ironSlot();
    if (b.style == 0) {
        g.box(0, 0, 0, b.hw * 0.72, b.hd, 0.5, 1.6, sl, MT_IRON);
        for (int s = -1; s <= 1; s += 2) g.box(s * b.hw * 0.86, 0, 0, b.hw * 0.16, b.hd * 0.96, 0, 1.1, sl, MT_IRON);
        if (g.lod == 2) return;
        const bool off = g.U(5) < 0.5 || D > 0.5;
        const double tx = off ? b.hw * 2.2 : 0, tz = off ? b.hd * 0.3 : -b.hd * 0.1, ty = off ? 0 : 1.6;
        Form& t = g.lathe(1, tx, tz, b.hw * 0.55, ty, ty + 0.9, sl, MT_IRON, 6); t.p[0] = 0.75; t.tilt = off ? 0.5 : 0;
        if (!off && g.lod == 0) { Form& gun = g.lathe(0, 0, tz + 0.3, 0.12, ty + 0.45, ty + 0.45 + b.hd * 1.1, sl, MT_IRON); gun.tilt = PI / 2 - 0.08; }
        return;
    }
    const double bedH = b.style == 1 ? b.hWall * 0.55 : 1.4;
    g.box(0, -b.hd * 0.15, 0, b.hw, b.hd * 0.82, bedH * 0.45, bedH, sl, MT_IRON);
    g.box(0, b.hd * 0.78, 0, b.hw * 0.7, b.hd * 0.2, bedH * 0.45, bedH + (b.style == 1 ? 1.6 : 1.0), sl, MT_IRON);
    if (g.lod == 0 && D < 0.6)
        for (int s = -1; s <= 1; s += 2) for (int q = -1; q <= 1; q += 2) {
            Form& w = g.add(FK_TORUS, 0, s * b.hw * 0.95, q * b.hd * 0.6, PI / 2, bedH * 0.42, bedH * 0.42, 0, 0.1, sl, MT_IRON); w.p[0] = bedH * 0.14; w.tilt = PI / 2;
        }
}

// an aircraft's wreck: its fuselage broken behind the wings, a wing torn off, the tail fin; in the people's age's materials
void genAircraft(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON;
    const double D = g.D(mat), r = b.hw * 0.14, L = b.hd * 2;
    if (D > 0.9) { if (g.lod == 0) g.heap(0, 0, b.hw * 0.4, b.hd * 0.5, 0.7, g.ironSlot(), mat); return; }
    const int sl = mat == MT_COMPOSITE ? 2 : g.ironSlot();
    { Form& f = g.lathe(0, 0, -b.hd, r, 0, L * 0.6, sl, mat); f.tilt = PI / 2; f.broken = 0.3; }
    { Form& f = g.lathe(1, 0.6, -b.hd + L * 0.64, r, 0, L * 0.36, sl, mat); f.tilt = PI / 2; f.heading += 0.25; f.p[0] = 0.45; f.broken = 0.4; }
    if (g.lod == 2) return;
    g.box(b.hw * 0.5, -b.hd + L * 0.4, 0.05, b.hw * 0.5, b.hd * 0.08, 0.2, 0.55, sl, mat);
    if (g.U(3) < 0.6) { Form& w = g.box(-b.hw * 0.9, -b.hd + L * 0.3, 0.6, b.hw * 0.45, b.hd * 0.07, 0, 0.4, sl, mat); w.tilt = 0.15; }
    if (D < 0.6) g.box(0.6, -b.hd + L * 0.94, 0.25, 0.15, b.hd * 0.1, 0.4, 0.4 + r * 3.5, sl, mat);
}

// a ship's wreck on the shore: a wooden hull's ribs and keel (`style` 0), an iron steamer listing with its funnel (1), a freighter broken
// in two, its superstructure aft (2); the hull in the water drawn from the water's level (a harbour work's foot)
void genShip(Gen& g) {
    const size_t first = g.out.size();
    struct Wet { Gen& g; size_t first; ~Wet() { for (size_t i = first; i < g.out.size(); i++) if (g.out[i].part == 0 || g.out[i].part == 3) g.out[i].part = 6; } } wet{g, first};
    const Building& b = g.b;
    const double L = b.hd, B = b.hw, H = b.hWall;
    if (b.style == 0) {
        const double Dt = g.D(MT_TIMBER);
        if (Dt > 0.9) return;
        g.box(0, 0, 0, 0.35, L, 0, 0.6, g.timberSlot(), MT_TIMBER);
        if (g.lod == 2) { g.box(0, 0, 0, B * 0.8, L * 0.8, 0, H * 0.4, g.timberSlot(), MT_TIMBER); return; }
        const int n = std::max(5, (int)(2 * L / 2.2));
        for (int k = 0; k < n; k++) {
            const double t = -1 + 2.0 * (k + 0.5) / n, w = B * std::sqrt(std::max(0.05, 1 - t * t * 0.85));
            if (g.Dk(MT_TIMBER, 40 + k) > 0.7) continue;
            for (int s = -1; s <= 1; s += 2) { Form& rb = g.box(s * w * 0.55, t * L, 0, 0.12, 0.12, 0, H * (0.6 + 0.4 * g.U(60 + k)), g.timberSlot(), MT_TIMBER); rb.tilt = s * 0.35; rb.heading += PI / 2; }
        }
        return;
    }
    const double Di = g.D(MT_IRON);
    if (Di > 0.92) { if (g.lod == 0) g.heap(0, 0, B, L * 0.6, 1.2, g.ironSlot(), MT_IRON); return; }
    const int sl = g.ironSlot();
    const double list = 0.08 + 0.12 * g.U(4);
    auto hull = [&](double z0, double z1, bool bow, double tilt, double dh) {
        const double len = 0.5 * (z1 - z0), mid = 0.5 * (z0 + z1);
        Form& h = g.box(0, mid, dh + PI / 2, len, B, 0, H, sl, MT_IRON); h.broken = Di < 0.4 ? 0.1 : clampd(Di, 0.2, 0.7); h.tilt = tilt; h.pattern = 0;
        if (bow) { const double hd = h.heading - PI / 2; Form& p = g.lathe(1, 0, z1 + B * 0.2, B, 0, H, sl, MT_IRON, 3); p.p[0] = 0.9; p.heading = hd; }
    };
    if (b.style == 2 && Di > 0.25) {   // broken in two: the bow's half turned off the stern's
        hull(-L, -L * 0.15, false, list, 0);
        hull(L * 0.05, L, true, list * 1.6, 0.18);
    } else hull(-L, L * 0.85, true, list, 0);
    if (g.lod == 2 || Di > 0.6) return;
    if (b.style == 1) { Form& f = g.lathe(0, 0, -L * 0.1, B * 0.22, H, H + B * 1.4, sl, MT_IRON); f.tilt = 0.2; g.box(0, -L * 0.5, 0, B * 0.55, L * 0.12, H, H + 3, sl, MT_IRON); }
    else { Form& s = g.box(0, -L * 0.8, 0, B * 0.85, L * 0.1, H, H + B * 1.1, sl, MT_IRON); s.pattern = 2; if (g.lod == 0) { Form& m = g.add(FK_TRUSS, 0, 0, L * 0.4, 0, 0.6, 0.6, H, H + B * 1.4, sl, MT_IRON); m.sides = 4; m.p[0] = 0.6; m.p[2] = 0.08; } }
}

// a launch pad: its slab, the service tower beside it with its arm out over the pad, the lightning masts round it, its water tower; the iron
// rusting: the arm down first, the tower bending
void genLaunchpad(Gen& g) {
    const Building& b = g.b;
    const double Di = g.D(MT_IRON), H = b.hWall;
    g.box(0, 0, 0, b.hw * 0.42, b.hw * 0.42, 0, 7, 0, MT_MASS);
    if (g.lod < 2) g.box(0, b.hw * 0.42 + 9, 0, 6, 9, 0, 1.5, 0, MT_MASS);   // the flame deflector's wedge
    const int sl = g.ironSlot();
    if (Di < 0.9) { Form& t = g.add(FK_TRUSS, 0, -b.hw * 0.42 - 8, 0, 0, 5, 5, 0, H, sl, MT_IRON); t.sides = 4; t.p[0] = 0.9; t.p[2] = 0.3; t.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95); }
    if (Di < 0.4 && g.lod < 2) { Form& a = g.add(FK_TRUSS, 1, -b.hw * 0.21, 0, PI / 2, 0.9, b.hw * 0.21 + 4, H * 0.7, H * 0.7 + 2, sl, MT_IRON); a.p[2] = 0.1; }
    if (g.lod == 0)
        for (int k = 0; k < 3; k++) {
            const double a = k * TAU / 3 + 0.4, d = b.hw * 0.9;
            const double dk = g.Dk(MT_IRON, 50 + k);
            if (dk > 0.85) continue;
            Form& m = g.add(FK_TRUSS, 0, std::sin(a) * d, std::cos(a) * d, 0, 1.2, 1.2, 0, H * 1.15, sl, MT_IRON); m.sides = 3; m.p[0] = 0.4; m.p[2] = 0.1; m.broken = dk < 0.3 ? 0 : clampd(dk, 0.3, 0.9);
        }
    if (g.lod < 2) { g.lathe(0, b.hw * 0.75, -b.hw * 0.75, 1.3, 0, 26, 0, MT_MASS); Form& tk = g.lathe(4, b.hw * 0.75, -b.hw * 0.75, 5, 26, 34, sl, MT_IRON); tk.p[0] = 0.3; tk.broken = Di > 0.5 ? 0.6 : 0; }
}

// a great hall: an assembly building's tall box with its door up a whole side (`style` 0), a mill's stepped sheds (1); the cladding goes
// first and the frame stands
void genHangar(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : (b.age >= AGE_MODERN ? MT_CONCRETE : MT_MASONRY);
    const double D = g.D(mat), Di = g.D(MT_IRON);
    if (b.style == 1) {
        for (int k = 0; k < 3; k++) {
            const double z = -b.hd + b.hd * (0.33 + 0.67 * k), h = b.hWall * (1 - 0.25 * k) * wallShare(D, g.U(10 + k));
            Form& e = g.box(0, z, 0, b.hw * (1 - 0.1 * k), b.hd * 0.34, 0, std::max(0.8, h), 0, mat); e.pattern = 2;
            if (h > b.hWall * 0.6 && Di < 0.6) { Form& r = g.add(FK_PYRAMID, 0, 0, z, 0, b.hw * (1 - 0.1 * k), b.hd * 0.34, h, h + 3, g.roofSlot(MT_IRON), MT_IRON, 2); r.sides = 4; r.p[1] = 0.1; }
        }
        return;
    }
    const double share = tallShare(D, g.U(3));
    if (Di < 0.75 || D < 0.4) {
        for (int s = 0; s < 4; s++) {
            const bool along = s % 2 == 0;
            const double off = along ? b.hd : b.hw, len = along ? b.hw : b.hd;
            if (s == 0) {   // the door's side: two jambs and the lintel high up
                for (int q = -1; q <= 1; q += 2) { Form& j = g.box(q * b.hw * 0.78, off, 0, b.hw * 0.22, 1.2, 0, b.hWall * share, 0, mat); j.pattern = 0; }
                Form& l = g.box(0, off, 0, b.hw * 0.56, 1.2, b.hWall * 0.82 * share, b.hWall * share, 0, mat); l.pattern = 0;
                continue;
            }
            Form& w = g.box(along ? 0 : (s == 1 ? off : -off), along ? -off : 0, along ? 0 : PI / 2, len, 1.2, 0, b.hWall * share, 0, mat); w.broken = share < 0.97 ? 0.3 : 0; w.pattern = 1;
        }
        if (share > 0.97 && D < 0.5) {   // R-412: its roof the people's: a barrel vault, a gable, a slab
            const int rf = g.c.roof;
            if (rf == RF_VAULTED || rf == RF_DOMED || rf == RF_SADDLE) { Form& v = g.add(FK_VAULT, rf == RF_SADDLE ? 2 : 0, 0, 0, 0, b.hw + 0.3, b.hd + 0.3, b.hWall, b.hWall + b.hw * 0.45, g.roofSlot(mat), mat, 2); v.p[0] = 0.5; v.p[1] = 1; }
            else if (rf == RF_PITCHED || rf == RF_HIPPED || rf == RF_CONICAL) { Form& r = g.add(FK_SWEEP, 1, 0, 0, 0, 0, b.hd + 0.5, b.hWall, b.hWall + b.hw * 0.35 * g.pitchK(), g.roofSlot(mat), mat, 2); r.p[1] = b.hw + g.eave(); r.p[2] = 0.15; }
            else g.box(0, 0, 0, b.hw + g.slabEave() * 0.5, b.hd + g.slabEave() * 0.5, b.hWall, b.hWall + 2, g.roofSlot(mat), mat, 2);
        }
    } else g.heap(0, 0, b.hw * 0.8, b.hd * 0.8, b.hWall * 0.08, 0, mat);
}

// a colossus on its plinth: the people's figure (its robed body, its shoulders and head, an arm raised with what it held where `style` 1);
// worn, its head and its arms gone; fallen, it lies along the ground beside the plinth, its head a way off
void genColossus(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double D = g.D(mat), H = b.hWall, ph = std::max(3.0, H * 0.18);
    g.sink = -c.buried * D;
    { Form& p = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hw, g.sink, g.sink + ph, 0, mat); p.p[0] = 2; p.p[1] = 0.8; p.p[2] = 0.7; }
    const double h = H - ph, r = h * 0.17;
    if (D < 0.6) {
        figure(g, 0, 0, g.sink + ph, h, g.accentOn(AP_TOPS) ? 1 : 0, mat, D * 0.7);
        if (b.style == 1 && D < 0.35 && g.lod < 2) { Form& a = g.lathe(0, r * 0.8, 0, r * 0.2, g.sink + ph + h * 0.62, g.sink + ph + h * 1.12, g.accentOn(AP_TOPS) ? 1 : 0, mat); a.tilt = 0.12; a.heading += PI / 2; }
        return;
    }
    const double dir = g.U(9) * TAU;   // fallen: the body along the ground, the head beyond it
    if (D < 0.9) {
        Form& e = g.lathe(1, std::sin(dir) * (b.hw + 1), std::cos(dir) * (b.hw + 1), r, 0, h * 0.72, 0, mat, 8); e.tilt = PI / 2; e.heading = b.heading + dir; e.p[0] = 0.62; e.broken = 0.4;
        if (g.lod == 0) { Form& hd = g.add(FK_BLOB, 0, std::sin(dir) * (b.hw + h * 0.95), std::cos(dir) * (b.hw + h * 0.95), 0, h * 0.1, h * 0.1, 0, h * 0.2, 0, mat); hd.p[0] = 0; hd.p[1] = 0.25; }
    }
}

// a wind turbine: its tower, the nacelle and hub, three blades facing the wind; the blades go first, then the nacelle, then the tower bends
void genTurbine(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Di = g.D(MT_IRON), Dc = g.D(MT_COMPOSITE), H = b.hWall, R = b.hw;
    if (Di > 0.9) { if (g.lod == 0) g.fallen(FK_LATHE, 0, 0, 0, g.U(3) * TAU, 2.0, H * 0.6, 0, MT_IRON); return; }
    const double share = Di < 0.5 ? 1 : tallShare(Di, g.U(4));
    // R-412: a lattice tower where the people expressed its frames, its section the people's; its nacelle rounded where its roofs were domes or
    // vaults; two blades or three or four by its number
    if (g.gv(GV_EXPRESSION) == EX_FRAME) { Form& t = g.add(FK_TRUSS, 0, 0, 0, 0, 3.2, 3.2, 0, H * share, 0, MT_IRON); t.sides = 4; t.p[0] = 0.3; t.p[2] = 0.14; t.broken = share < 0.97 ? 0.5 : 0; }
    else { const int pl = g.gv(GV_PLAN); Form& t = g.lathe(1, 0, 0, 2.4, 0, H * share, 0, MT_IRON, pl == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : 0); t.p[0] = 0.62; t.broken = share < 0.97 ? 0.3 : 0; }
    if (share < 0.97 || g.lod == 2) return;
    const bool round = c.roof == RF_DOMED || c.roof == RF_VAULTED || c.roof == RF_SADDLE;
    if (round) { g.box(0, -1.2, 0, 1.6, 3.9, H - 1.4, H + 0.1, 0, MT_IRON); Form& v = g.add(FK_VAULT, 0, 0, -1.2, 0, 1.6, 3.9, H + 0.1, H + 1.7, 0, MT_IRON); v.p[0] = 1.5; v.p[1] = 1; }   // a barrel over its body
    else g.box(0, -1.2, 0, 1.6, 4.5, H - 1.4, H + 1.8, 0, MT_IRON);
    g.lathe(0, 0, 3.6, 1.3, H - 0.9, H + 1.3, 0, MT_IRON);
    const int nb = clampi(c.genome.number() % 3 + 2, 2, 4);
    const int left = Dc < 0.3 ? nb : (Dc < 0.55 ? nb - 1 : (Dc < 0.75 ? 1 : 0));
    const double ph = g.U(6) * TAU;
    for (int k = 0; k < left; k++) {   // each blade a straight line from the hub in the rotor's plane (across the nacelle)
        const double a = ph + k * TAU / nb;
        double fx, fz; g.to(std::sin(a) * R, 3.6, fx, fz);
        Form& e = g.add(FK_CABLE, 0, 0, 3.6, 0, 0.8, 0.8, H, H + 0.2, 0, MT_COMPOSITE); e.p[0] = fx; e.p[1] = fz; e.p[2] = H + 0.2 + std::cos(a) * R; e.p[3] = 0;
    }
}

// a lighthouse: its tower tapering to the gallery, the lantern of glass under its cap, the keeper's house at its foot
// R-412: in the people's section (round, its polygon, square) and taper (a battered people's broad-footed, a slender people's a near cylinder),
// painted in bands of its accent where its people painted in stripes or two tones, its lantern under its people's cap (a dome, a cone, a
// flat lid), its gallery's rail; the keeper's house in the people's dress
void genLighthouse(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat), H = b.hWall;
    const double share = tallShare(Dm, g.U(3));
    const int plan = g.gv(GV_PLAN), prof = g.gv(GV_PROFILE), sides = plan == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : (plan == PF_RECT || plan == PF_COURT ? 4 : 0);
    const double taper = prof == WP_BATTERED ? 0.5 : (prof == WP_UPRIGHT ? 0.72 + 0.2 * g.gf(GV_PROPORTION) : 0.62);
    { Form& t = g.lathe(1, 0, 0, b.hw, 0, H * share, 0, mat, sides); t.p[0] = taper; t.broken = share < 0.97 ? 0.25 : 0; t.pattern = 2; }
    const int pc = g.gv(GV_POLYCHROMY);
    if ((pc == PC_STRIPED || pc == PC_TWO_TONE) && g.lod < 2 && Dm < 0.5) {   // its painted bands
        const int bands = pc == PC_STRIPED ? 3 : 1;
        for (int k = 0; k < bands; k++) {
            const double ya = H * (pc == PC_STRIPED ? (0.12 + 0.28 * k) : 0.5), yb = ya + H * (pc == PC_STRIPED ? 0.13 : 0.5);
            if (yb > H * share) break;
            const double ra = b.hw * (1 - (1 - taper) * ya / H) + 0.04, rb = b.hw * (1 - (1 - taper) * yb / H) + 0.04;
            Form& e = g.lathe(1, 0, 0, ra, ya, std::min(yb, H * share), 1, mat, sides); e.p[0] = rb / ra; e.part = 4;
        }
    }
    if (share >= 0.97 && g.lod < 2) {
        const double rt = b.hw * taper;
        g.lathe(0, 0, 0, rt * 1.25, H, H + 0.5, 0, mat, sides);
        if (g.lod == 0 && g.D(MT_IRON) < 0.5) { Form& r = g.add(FK_TORUS, 0, 0, 0, 0, rt * 1.18, rt * 1.18, H + 0.5, H + 1.6, g.ironSlot(), MT_IRON); r.p[0] = 0.05; }   // the gallery's rail
        if (g.D(MT_GLASS) < 0.9) g.lathe(0, 0, 0, rt * 0.8, H + 0.5, H + 3.2, 2, MT_GLASS, 8);
        if (g.D(MT_IRON) < 0.6) {
            const int rf = c.roof;
            if (rf == RF_FLAT || rf == RF_OPEN || rf == RF_STEPPED) g.lathe(0, 0, 0, rt * 0.95, H + 3.2, H + 3.55, g.roofSlot(MT_IRON), MT_IRON, 8);
            else if (rf == RF_PITCHED || rf == RF_HIPPED || rf == RF_CONICAL) { Form& k = g.lathe(1, 0, 0, rt * 0.92, H + 3.2, H + 3.2 + rt * 1.6 * g.pitchK(), g.roofSlot(MT_IRON), MT_IRON, 8); k.p[0] = 0.05; }
            else g.add(FK_DOME, ageDome(g, rf == RF_DOMED && (g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES) ? 3 : 2), 0, 0, 0, rt * 0.88, rt * 0.88, H + 3.2, H + 4.6, g.roofSlot(MT_IRON), MT_IRON, 2);
        }
    }
    if (g.lod == 2) return;
    Walls W; W.cx = b.hw + 6; W.hw = 3.5; W.hd = 4.5; W.h = 3.2; W.thick = 0.5; W.door = 3; W.doorPos = 0; W.mat = stoneOf(c); W.k0 = 30; W.pattern = 1;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = c.age >= AGE_MODERN ? RF_FLAT : (c.roof == RF_OPEN ? RF_PITCHED : c.roof), rm = roofMaterial(g, rf, W.mat);
    const double rise = roof(g, rf, W.cx, 0, W.hw, W.hd, hmin, W.h, 0.6, rm);
    if (rf == RF_PITCHED) gables(g, W, hs, 0.6);
    entranceAt(g, W, wallsHalf(g, W), W.h);
    cornersOf(g, W, hs, wallsHalf(g, W), rm);
    rooflineOf(g, W, hmin, rf, rise, rm);
}

// the far future's lattice over a valley: girders across and along at one height over the reference level (`base` + hWall), a pillar down
// to the ground at its corners; nothing of it weathers
void genLattice(Gen& g) {
    const Building& b = g.b;
    const double y = b.base + b.hWall, cell = std::max(40.0, std::min(b.hw, b.hd) / 3);
    const int nx = std::max(1, (int)std::lround(b.hw / cell)), nz = std::max(1, (int)std::lround(b.hd / cell));
    for (int i = -nx; i <= nx; i++) { Form& e = g.add(FK_TRUSS, 1, i * b.hw / nx, 0, 0, 1.6, b.hd, y - 3, y, 0, MT_ETERNAL); e.p[2] = 0.35; e.absolute = true; }
    for (int j = -nz; j <= nz; j++) { Form& e = g.add(FK_TRUSS, 1, 0, j * b.hd / nz, PI / 2, 1.6, b.hw, y - 3, y, 0, MT_ETERNAL); e.p[2] = 0.35; e.absolute = true; }
    for (int s = -1; s <= 1; s += 2) for (int q = -1; q <= 1; q += 2) { Form& p = g.lathe(0, s * b.hw, q * b.hd, 3.5, b.base - 40, y, 0, MT_ETERNAL, 4); p.absolute = true; }
    if (g.lod == 0) { Form& l = g.box(0, 0, 0, b.hw, 0.4, y + 0.05, y + 0.35, 3, MT_ETERNAL, 4); l.absolute = true; }
}

// a quarry's derrick: a timber mast and its boom on guys (`style` 0), or an iron one of lattice (1)
void genDerrick(Gen& g) {
    const Building& b = g.b;
    const int mat = b.style == 0 ? MT_TIMBER : MT_IRON;
    const double D = g.D(mat), H = b.hWall;
    if (D > 0.8) { if (g.lod == 0) g.fallen(b.style == 0 ? FK_LATHE : FK_TRUSS, b.style == 0 ? 0 : 0, 0, 0, g.U(3) * TAU, 0.3, H * 0.7, frameSlot(g, mat), mat, b.style == 0 ? 6 : 4); return; }
    const int sl = frameSlot(g, mat);
    if (b.style == 0) g.lathe(0, 0, 0, 0.28, 0, H, sl, mat, 6);
    else { Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 0.9, 0.9, 0, H, sl, mat); m.sides = 4; m.p[0] = 0.5; m.p[2] = 0.08; m.broken = D < 0.3 ? 0 : D; }
    if (g.lod == 2 || D > 0.45) return;
    Form& boom = b.style == 0 ? g.lathe(0, 0, 0.4, 0.2, 1.2, 1.2 + H * 0.85, sl, mat, 6) : g.add(FK_TRUSS, 0, 0, 0.4, 0, 0.5, 0.5, 1.2, 1.2 + H * 0.85, sl, mat);
    boom.tilt = 0.75; if (b.style == 1) { boom.sides = 3; boom.p[2] = 0.06; }
    if (g.lod == 0)
        for (int k = 0; k < 3; k++) {
            double a = g.U(11) * TAU + k * TAU / 3, d = H * 0.8, fx, fz; g.to(std::sin(a) * d, std::cos(a) * d, fx, fz);
            Form& cb = g.add(FK_CABLE, 0, 0, 0, 0, 0.03, 0.03, 0, H, sl, mat); cb.p[0] = fx; cb.p[1] = fz; cb.p[2] = 0; cb.p[3] = H * 0.01;
        }
}

// blocks cut and left: in rows on the floor, a few stacked; the rock's own
void genBlocks(Gen& g) {
    const Building& b = g.b;
    const int n = 4 + (int)(g.U(2) * 8);
    if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, 0, 1.2, 0, MT_DRYSTONE); return; }
    for (int k = 0; k < n; k++) {
        const double x = -b.hw + 2 * b.hw * g.U(10 + k), z = -b.hd + 2 * b.hd * g.U(30 + k);
        const double sx = 0.7 + 0.5 * g.U(50 + k), sz = 0.5 + 0.35 * g.U(70 + k), h = 0.8 + 0.6 * g.U(90 + k);
        g.box(x, z, (g.U(110 + k) - 0.5) * 0.3, sx, sz, 0, h, 0, MT_DRYSTONE, 3);
        if (k % 3 == 0) g.box(x, z, (g.U(130 + k) - 0.5) * 0.4, sx * 0.85, sz * 0.9, h, h + 0.7 + 0.4 * g.U(150 + k), 0, MT_DRYSTONE, 3);
    }
}

// the far future's sphere resting on its point (`style` 0): it never weathers, and nothing holds it up but a needle; or a pillar (1)
void genSphere(Gen& g) {
    const Building& b = g.b;
    if (b.style == 1) {   // a pillar of a line to the horizon: a square prism, a line of light up one face
        g.lathe(0, 0, 0, b.hw, 0, b.hWall, 0, MT_ETERNAL, 4);
        if (g.lod < 2) g.box(0, b.hw * 0.72, 0, 0.25, 0.1, 0, b.hWall, 3, MT_ETERNAL, 4);
        return;
    }
    const double r = b.hw, y0 = std::max(4.0, r * 0.08);
    { Form& s = g.lathe(4, 0, 0, r, y0, y0 + 2 * r, 0, MT_ETERNAL); s.p[0] = 1.0; }
    { Form& n = g.lathe(2, 0, 0, std::max(0.6, r * 0.02), 0, y0 + r * 0.2, 3, MT_ETERNAL); n.p[0] = 1.0; }
}

// a space elevator's anchor: its ring, the tower, and the ribbon up out of sight; the ribbon gone once the composite is a third worn
void genTether(Gen& g) {
    const Building& b = g.b;
    const double D = g.D(MT_COMPOSITE), H = b.hWall;
    { Form& r = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, 0, 8, 0, MT_COMPOSITE); r.p[0] = 6; }
    const double share = D < 0.3 ? 1 : tallShare(D, g.U(3));
    { Form& t = g.lathe(1, 0, 0, b.hw * 0.25, 0, H * share, 0, MT_COMPOSITE, 6); t.p[0] = 0.35; t.broken = share < 0.97 ? 0.3 : 0; t.pattern = 1; }
    if (share >= 0.97 && D < 0.3 && g.lod < 2) {
        double fx, fz; g.to(0, 0, fx, fz);
        Form& c = g.add(FK_CABLE, 0, 0, 0, 0, 0.8, 0.8, H, H + 1, 3, MT_COMPOSITE); c.p[0] = fx; c.p[1] = fz; c.p[2] = 60000; c.p[3] = 0;
    }
}

// a trilithon: two uprights and the lintel across them; the lintel down beside them first
void genTrilithon(Gen& g) {
    const Building& b = g.b;
    const double H = b.hWall, w = b.hw * 0.32;
    for (int s = -1; s <= 1; s += 2) standingStone(g, s * (b.hw - w), 0, 0, w, b.hd, H, 100 + s, false);
    const double d = std::max(g.Dk(MT_DRYSTONE, 99), std::max(g.Dk(MT_DRYSTONE, 101), g.Dk(MT_DRYSTONE, 102)));
    if (d < 0.3) g.box(0, 0, 0, b.hw, b.hd * 0.9, H, H + b.hd * 1.4, 0, MT_DRYSTONE, 5);
    else if (d < 0.9 && g.lod == 0) { Form& l = g.box(0, b.hd + 1.4, 0.3 * (g.U(5) - 0.5), b.hw, b.hd * 0.9, 0, b.hd * 1.4, 0, MT_DRYSTONE, 5); l.tilt = 0.05; }
}

// a conveyor: its girder along the heading at hWall over the ground, on legs every twenty metres
void genConveyor(Gen& g) {
    const Building& b = g.b;
    const double Di = g.D(MT_IRON);
    if (Di > 0.85) return;
    const int sl = g.ironSlot();
    { Form& e = g.add(FK_TRUSS, 1, 0, 0, 0, 1.2, b.hd, b.hWall, b.hWall + 2, sl, MT_IRON); e.p[2] = 0.1; e.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.9); }
    if (g.lod == 2) return;
    const int n = std::max(1, (int)(2 * b.hd / 20));
    for (int k = 0; k <= n; k++) { Form& l = g.lathe(0, 0, -b.hd + 2 * b.hd * k / n, 0.3, 0, b.hWall, sl, MT_IRON, 4); (void)l; }
}

// D-05: a dome over the warm ground, a people that lost its sun's (`hw` its radius, `hWall` its ring's height): an industrial people's iron ribs
// and their glass on a ring of brick (`style` 0), a modern people's steel and glass on a ring of concrete (1), the far future's whole (2). The glass
// went at once; the ribs (half rings standing across the ring, crossing at its crown, and hoops round them) rust to stumps over centuries; the
// ring stays. A futuristic people's dome is a habitat's (`BK_HABITAT`)
int domeRingSegs(double r) { return clampi((int)std::lround(r * 0.6), 16, 28); }
void genDome(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double r = b.hw;
    if (b.style == 2) {   // the far future's: nothing weathers
        { Form& t = g.add(FK_TORUS, 0, 0, 0, 0, r, r, 0, 4, 1, MT_ETERNAL); t.p[0] = 2.0; }
        g.add(FK_DOME, 1, 0, 0, 0, r, r, 2.0, 2.0 + r * 0.55, 2, MT_ETERNAL);
        return;
    }
    const int mat = b.style == 0 ? brickOf(c) : MT_CONCRETE;
    const double Dm = g.D(mat), Dg = g.D(MT_GLASS);
    g.sink = -c.buried * Dm;
    Ring R; R.rx = R.rz = r; R.h = b.hWall; R.thick = 1.0; R.mat = mat; R.dress = false; R.k0 = 10;
    R.segs = domeRingSegs(r); R.doorSegs = ringDoorSegs(r, R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
    ringWalls(g, R);
    if (g.lod == 2) return;
    const double yTop = g.sink + b.hWall, tube = clampd(r * 0.018, 0.45, 0.8);
    const int n = b.style == 0 ? 6 : 8, is = g.ironSlot();
    for (int k = 0; k < n; k++) {   // the ribs: a ring standing on its rim, its middle at the ring's top (its lower half in the ground)
        const double d = g.Dk(MT_IRON, 20 + k);
        if (d > 0.9) continue;
        Form& t = g.add(FK_TORUS, 0, 0, 0, k * PI / n, r, r, yTop - r - tube, yTop + r + tube, is, MT_IRON);
        t.p[0] = tube; t.tilt = PI / 2; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.95);
    }
    for (int q = 1; q <= (b.style == 0 ? 2 : 3); q++) {   // the hoops round them
        const double ph = q * (PI / 2) / (b.style == 0 ? 3 : 4), d = g.Dk(MT_IRON, 40 + q);
        if (d > 0.8) continue;
        const double rr = r * std::cos(ph), y = yTop + r * std::sin(ph);
        Form& t = g.add(FK_TORUS, 0, 0, 0, 0, rr, rr, y - tube, y + tube, is, MT_IRON); t.p[0] = tube * 0.8; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.9);
    }
    if (Dg < 0.8) { Form& e = g.add(FK_DOME, 0, 0, 0, 0, r, r, yTop, yTop + r, 2, MT_GLASS); e.broken = Dg < 0.3 ? 0 : clampd(Dg, 0.3, 0.95); }
}

// D-05: a glasshouse, a people that lost its sun's (`hw` across, `hd` along its heading): an industrial people's iron hoops and their glass on low
// walls of brick (`style` 0, a conservatory's curve), a modern people's tunnel of steel hoops from the ground under its sheet (1), a futuristic or
// a later people's vault of composite (2). The glass and the sheet went at once; the hoops stand over their walls
void genGreenhouse(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double hw = b.hw, hd = b.hd;
    if (b.style >= 2) {
        const int mat = c.age >= AGE_FAR_FUTURE ? MT_ETERNAL : MT_COMPOSITE; const double D = g.D(mat);
        Form& v = g.add(FK_VAULT, 2, 0, 0, 0, hw, hd, 0, b.hWall, 2, mat); v.p[0] = 0.3; v.p[1] = 1; v.broken = D < 0.3 ? 0 : clampd(D, 0.3, 0.95);
        return;
    }
    const int mat = b.style == 0 ? brickOf(c) : MT_CONCRETE;
    const double Dm = g.D(mat), Dg = g.D(MT_GLASS);
    g.sink = -c.buried * Dm;
    const double wall = b.style == 0 ? 0.9 : 0.3, th = b.style == 0 ? 0.22 : 0.15;
    for (int s = -1; s <= 1; s += 2) g.box(s * hw, 0, 0, th, hd, g.sink, g.sink + std::max(0.2, wall * wallShare(Dm, g.U(10 + s))), 0, mat);
    if (b.style == 0 && g.lod < 2) {   // its ends: the back whole, the front either side of its door
        g.box(0, -hd, 0, hw, th, g.sink, g.sink + wall * wallShare(Dm, g.U(12)), 0, mat);
        for (int s = -1; s <= 1; s += 2) g.box(s * (hw * 0.5 + 0.6), hd, 0, hw * 0.5 - 0.6, th, g.sink, g.sink + wall * wallShare(Dm, g.U(13 + s)), 0, mat);
    }
    if (g.lod == 2) return;
    const double yTop = g.sink + wall, tube = b.style == 0 ? 0.12 : 0.08, step = b.style == 0 ? 2.5 : 2.0;
    const int n = std::max(2, (int)std::lround(2 * hd / step)), is = g.ironSlot();
    for (int k = 0; k <= n; k++) {   // a hoop across it every few metres (a ring standing on its rim, facing along the house)
        const double d = g.Dk(MT_IRON, 30 + k);
        if (d > 0.9) continue;
        Form& t = g.add(FK_TORUS, 0, 0, -hd + 2 * hd * k / n, 0, hw, hw, yTop - hw - tube, yTop + hw + tube, is, MT_IRON);
        t.p[0] = tube; t.tilt = PI / 2; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.95);
    }
    if (Dg < 0.8) { Form& v = g.add(FK_VAULT, 0, 0, 0, 0, hw, hd, yTop, yTop + hw, 2, MT_GLASS); v.p[0] = 0.05; v.broken = Dg < 0.3 ? 0 : clampd(Dg, 0.3, 0.95); }
}
