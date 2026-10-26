// E-09: R-412's dress on the dwellings (the doorway, the corners, the fronts and their projections, the roofline, the yard, a
// hut's pole and entrance) and the street furniture
#include "buildings_internal.h"

// ---- R-412: the people's dress on its dwellings -----------------------------------------------------------------------------------
// what its genome adds to a house's body, read through its age (B-415: the age speaks first): the doorway's porch, steps, portico or
// canopy; its corners' quoins, pilasters or turrets (a modern people's fins and rounded corners); its fronts' balconies, galleries, bays and
// oriels; its roof's dormers, cross gables, cresting and lantern (a modern people's flat roof's pergola, tank, rails and penthouse); its yard's
// wall, fence, hedge or railings; a prehistoric people's pole by its hut's door

namespace {
// a side of four walls: its outer face's middle, its outward normal, its axis (the walls' own: +x' on the front and the back, -z' on the
// right and the left), its half length, the heading a box laid along it turns by
struct Face { double fx = 0, fz = 0, nx = 0, nz = 0, ax = 1, az = 0, L = 1, dh = 0; };
Face faceOf(const Walls& W, int side, double ht) {
    Face f;
    f.nx = side == 1 ? 1 : (side == 3 ? -1 : 0); f.nz = side == 0 ? 1 : (side == 2 ? -1 : 0);
    const bool along = side == 0 || side == 2;
    f.ax = along ? 1 : 0; f.az = along ? 0 : -1; f.L = along ? W.hw : W.hd; f.dh = along ? 0 : PI / 2;
    f.fx = W.cx + f.nx * (W.hw + ht); f.fz = W.cz + f.nz * (W.hd + ht);
    return f;
}
void faceAt(const Face& f, double t, double o, double& x, double& z) { x = f.fx + f.ax * t + f.nx * o; z = f.fz + f.az * t + f.nz * o; }
// a box against a face: `t` along its axis, its middle `o` out from it, `a` its half length along, `d` its half depth out
Form& faceBox(Gen& g, const Face& f, double t, double o, double a, double d, double y0, double y1, int slot, int mat, int part = 0) {
    double x, z; faceAt(f, t, o, x, z);
    return g.box(x, z, f.dh, a, d, y0, y1, slot, mat, part);
}
// a fence along a face (`across` false) or out from it (true): its middle `t` along and `o` out, `a` its half length
Form& faceFence(Gen& g, const Face& f, bool across, double t, double o, double a, double y0, double y1, int sub, int slot, int mat, int part = 4) {
    double x, z; faceAt(f, t, o, x, z);
    Form& e = g.add(FK_FENCE, sub, x, z, across ? f.dh : f.dh + PI / 2, 0.025, a, y0, y1, slot, mat, part);
    return e;
}
}   // namespace
// the walls' half thickness as `walls4x` builds them (a massive people's thicker, a battered or a bulging wall's more)
double wallsHalf(const Gen& g, const Walls& W) {
    const int prof = W.profile >= 0 ? W.profile : (W.dress ? g.gv(GV_PROFILE) : WP_UPRIGHT);
    double t = W.thick;
    if (W.dress && g.gv(GV_EXPRESSION) == EX_MASSIVE) t *= 1.35;
    if (prof == WP_BATTERED) t = std::max(t * 1.3, std::min(2.4, 0.12 * W.h)); else if (prof == WP_BULGING) t *= 1.6;
    return 0.5 * t;
}

namespace {
// a railing's sub and stuff by the age: a medieval people's timber pales (a balustrade of its stone where `stone`), an industrial people's
// iron bars, a modern people's steel ones, a futuristic people's composite
void railOf(const Gen& g, bool stone, int& sub, int& slot, int& mat) {
    switch (g.c.age) {
        case AGE_MEDIEVAL: if (stone) { sub = 3; slot = 0; mat = stoneOf(g.c); } else { sub = 0; slot = g.timberSlot(); mat = MT_TIMBER; } return;
        case AGE_INDUSTRIAL: sub = 1; slot = g.ironSlot(); mat = MT_IRON; return;
        case AGE_MODERN: sub = 1; slot = g.ironSlot(); mat = MT_IRON; return;
        default: sub = 1; slot = 2; mat = MT_COMPOSITE; return;
    }
}
}   // namespace

// the doorway's: a porch on two posts under a little gable, a stoop of three steps, a portico of columns under its pediment, a canopy over
// it; a modern people's porch a slab on two square posts, its portico a deep slab on round columns
void entranceAt(Gen& g, const Walls& W, double ht, double H) {
    const Culture& c = g.c;
    if (W.door < 0 || W.y0 > 0.5 || c.age < AGE_MEDIEVAL || c.age > AGE_MODERN) return;
    const Face f = faceOf(W, W.door, ht);
    const double t = W.doorPos, gnd = 0.0;
    const int en = g.gv(GV_ENTRANCE), modern = c.age == AGE_MODERN;
    const double Dt = g.D(MT_TIMBER), Dm = g.D(W.mat);
    switch (en) {
        case EN_PORCH: {
            if (std::fabs(t) + 1.45 > f.L + 0.3) return;
            if (modern) {
                if (g.D(MT_CONCRETE) > 0.6) return;
                for (int s = -1; s <= 1; s += 2) faceBox(g, f, t + s * 1.4, 1.75, 0.12, 0.12, gnd, gnd + 2.6, 0, MT_CONCRETE);
                faceBox(g, f, t, 1.0, 1.65, 1.05, gnd + 2.6, gnd + 2.85, 0, MT_CONCRETE, 2);
                return;
            }
            for (int s = -1; s <= 1; s += 2) { double x, z; faceAt(f, t + s * 1.35, 1.65, x, z); g.post(x, z, 2.55, 0.1, 520 + (s > 0)); }
            if (Dt < 0.35 && g.lod == 0) {
                double x, z; faceAt(f, t, 0.8, x, z);
                const double y0 = gnd + 2.5;
                Form& r = g.add(FK_SWEEP, 1, x, z, f.dh, 0, 1.05, y0, y0 + 1.6 * 0.75 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
                r.p[1] = 1.6; r.p[2] = 0.08;
            }
            return;
        }
        case EN_STEPS: {
            if (g.lod != 0) return;
            for (int k = 0; k < 3; k++) faceBox(g, f, t, (1.15 - 0.33 * k) * 0.5, 1.25 - 0.1 * k, (1.15 - 0.33 * k) * 0.5, gnd, gnd + 0.17 * (k + 1), W.slot, W.mat, 4);
            if (c.age == AGE_INDUSTRIAL && g.D(MT_IRON) < 0.5)
                for (int s = -1; s <= 1; s += 2) {
                    Form& e = faceFence(g, f, true, t + s * 1.28, 0.55, 0.55, gnd + 0.51, gnd + 1.35, 1, g.ironSlot(), MT_IRON);
                    e.p[3] = 0.08;
                }
            if (modern) for (int s = -1; s <= 1; s += 2) faceBox(g, f, t + s * 1.38, 0.6, 0.12, 0.6, gnd, gnd + 0.75, 0, MT_CONCRETE, 4);
            return;
        }
        case EN_PORTICO: {
            if (H < 3.0 || Dm > 0.5) return;
            const bool four = std::fabs(t) + 2.9 <= f.L;
            const double colH = std::min(H - 0.3, modern ? 3.6 : 4.4), out = 2.2, span = four ? 2.75 : 1.5;
            const int cols = four ? 4 : 2;
            for (int k = 0; k < cols; k++) {
                const double tc = t + (cols == 4 ? (k < 2 ? -1 : 1) * (k % 2 ? 2.55 : 1.3) : (k ? 1.3 : -1.3));
                double x, z; faceAt(f, tc, out - 0.25, x, z);
                g.lathe(0, x, z, modern ? 0.16 : 0.24, gnd, gnd + colH, W.slot, W.mat, modern ? 0 : (g.gv(GV_PLAN) == PF_RECT ? 4 : 0));
            }
            if (Dm > 0.35 || g.lod > 1) return;
            faceBox(g, f, t, out * 0.5, span + 0.3, out * 0.5, gnd + colH, gnd + colH + (modern ? 0.35 : 0.45), W.slot, W.mat, 2);
            if (!modern) {   // its pediment, its gable to the street
                double x, z; faceAt(f, t, out * 0.5, x, z);
                const double y0 = gnd + colH + 0.45;
                Form& p = g.add(FK_SWEEP, 1, x, z, f.dh, 0, out * 0.5, y0, y0 + (span + 0.3) * 0.32, W.slot, W.mat, 4); p.p[1] = span + 0.3; p.p[2] = 0.05;
            }
            return;
        }
        case EN_CANOPY: {
            if (g.lod != 0) return;
            if (modern) { if (g.D(MT_CONCRETE) < 0.6) faceBox(g, f, t, 1.0, 1.9, 1.0, gnd + 2.55, gnd + 2.8, 0, MT_CONCRETE, 2); return; }
            if (c.age == AGE_INDUSTRIAL) {   // an iron canopy on its brackets
                if (g.D(MT_IRON) > 0.5) return;
                faceBox(g, f, t, 0.75, 1.45, 0.75, gnd + 2.6, gnd + 2.7, g.ironSlot(), MT_IRON, 2);
                for (int s = -1; s <= 1; s += 2) { Form& b = faceBox(g, f, t + s * 1.3, 0.35, 0.04, 0.35, gnd + 2.1, gnd + 2.6, g.ironSlot(), MT_IRON, 4); (void)b; }
                return;
            }
            // a hood over the door, its outer edge lower (a box's +along is out on the front and the right)
            if (Dt < 0.4) { Form& h = faceBox(g, f, t, 0.55, 1.3, 0.55, gnd + 2.45, gnd + 2.6, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); h.tilt = W.door <= 1 ? 0.28 : -0.28; }
            return;
        }
        default: return;
    }
}

// the corners': quoins (alternate long and short blocks up the front's two corners, lighter than the wall), pilasters with their capitals, a
// turret corbelled out over each front corner under its cap; a modern people's fins at its front's corners or its rounded corners
void cornersOf(Gen& g, const Walls& W, const double hs[4], double ht, int roofMat) {
    const Culture& c = g.c;
    if (c.age < AGE_MEDIEVAL || c.age > AGE_MODERN) return;
    const int co = g.gv(GV_CORNERS);
    if (co == CO_PLAIN) return;
    const bool modern = c.age == AGE_MODERN;
    const double base = g.sink + W.y0, H = W.h;
    if (g.b.joined && (co == CO_TURRETS || modern)) return;   // a house in a row: its neighbours at its corners
    for (int q = 0; q < 4; q++) {
        const int sx = q % 2 ? 1 : -1, szs = q < 2 ? 1 : -1;   // q 0, 1 the front's corners
        if (szs < 0 && (co != CO_PILASTERS || g.lod != 0)) continue;
        const int sa = szs > 0 ? 0 : 2, sb = sx > 0 ? 1 : 3;
        const double hc = std::min(hs[sa], hs[sb]);
        const double Cx = W.cx + sx * (W.hw + ht), Cz = W.cz + szs * (W.hd + ht);   // the outer corner
        if (modern) {
            if (hc < 0.97 * H) continue;
            if (co == CO_QUOINS && szs > 0 && H > 5) g.box(Cx - sx * 0.58, Cz - 0.58, 0, 0.6, 0.6, base + 2.8, base + H - 0.4, g.ironSlot(), MT_CONCRETE, 4);   // its corners glazed (dark)
            if (co == CO_PILASTERS && szs > 0) g.box(Cx - sx * 0.12, Cz + 0.45, 0, 0.12, 0.45, base, base + H, W.slot, W.mat, 4);   // a fin
            // a rounded corner
            else if (co == CO_TURRETS && szs > 0 && H > 5) { Form& e = g.lathe(0, Cx - sx * 0.9, Cz - 0.9, 1.6, base, base + H + 0.3, wallsSlot(g, W), W.mat); e.pattern = W.pattern; }
            continue;
        }
        if (co == CO_QUOINS) {
            if (g.lod != 0 || hc < 1.0) continue;
            const int n = std::min(9, (int)(hc / 0.62));
            for (int k = 0; k < n; k++) {
                const bool longX = k % 2 == 0;
                const double ex = longX ? 0.55 : 0.3, ez = longX ? 0.3 : 0.55;
                Form& e = g.box(Cx - sx * (ex - 0.05), Cz - szs * (ez - 0.05), 0, ex, ez, base + k * 0.62, base + k * 0.62 + 0.54, W.slot, W.mat, 4);
                e.tone = g.tone + 9;
            }
        } else if (co == CO_PILASTERS) {
            if (hc < 0.97 * H || g.lod > 1) continue;
            g.box(Cx - sx * 0.25, Cz - szs * 0.25, 0, 0.37, 0.37, base, base + H - 0.25, W.slot, W.mat, 4);
            g.box(Cx - sx * 0.25, Cz - szs * 0.25, 0, 0.47, 0.47, base + H - 0.55, base + H - 0.05, W.slot, W.mat, 4);
        } else if (co == CO_TURRETS) {
            if (hc < 0.97 * H) continue;
            const bool low = H < 4.8;   // a low house's: a round tower at its front corner away from its door, from the ground
            if (low && (sx > 0) == (W.door == 0 && W.doorPos > 0)) continue;
            const double rt = clampd(0.2 * std::min(W.hw, W.hd), 0.75, 1.35), tx = Cx + sx * rt * 0.25, tz = Cz + szs * rt * 0.25, y0 = low ? base : base + std::max(2.6, 0.45 * H);
            if (!low) { Form& k = g.lathe(1, tx, tz, 0.3, y0 - 1.1, y0, wallsSlot(g, W), W.mat); k.p[0] = rt / 0.3; }   // its corbel
            {
                Form& e = g.lathe(0, tx, tz, rt, y0, base + H + (low ? 1.3 : 0.6), wallsSlot(g, W), W.mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : 0);
                e.pattern = g.lod == 0 ? W.pattern : 0;
            }
            const double Dr = g.D(roofMat);
            if (Dr < 0.4) {
                const double y1 = base + H + (low ? 1.3 : 0.6);
                if (c.roof == RF_DOMED) { Form& d = g.add(FK_DOME, ageDome(g, 2), tx, tz, 0, rt + 0.1, rt + 0.1, y1, y1 + rt * 1.2, g.roofSlot(roofMat), roofMat, 2); (void)d; }
                else { Form& s = g.lathe(1, tx, tz, rt + 0.15, y1, y1 + rt * 2.4 * g.pitchK(), g.roofSlot(roofMat), roofMat); s.p[0] = 0.04; }
            }
        }
    }
}

namespace {

// what stands out from its front (side 0) over its doorway's storey: a balcony to a bay of each upper storey, a gallery along each, two
// bay windows from the ground (a canted one an industrial people's), an oriel on the first floor
void projectionsOf(Gen& g, const Walls& W, const double hs[4], double ht, int prof) {
    const Culture& c = g.c;
    if (c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || hs[0] < 0.97 * W.h || W.y0 > 0.5 || g.sink < -0.6) return;   // none on a sunk house (its first floor at the ground)
    const int pj = g.gv(GV_PROJECTIONS);
    if (pj == PJ_NONE) return;
    Face f = faceOf(W, 0, ht);
    const double base = g.sink + W.y0, H = W.h, L = f.L;
    const WindowSpec& ws = c.palette.windows[clampi(W.pattern > 0 ? W.pattern - 1 : 0, 0, 3)];
    const double storey = c.age == AGE_MEDIEVAL ? 2.7 : (c.age == AGE_INDUSTRIAL ? 3.3 : 3.0), bay = std::max(2.4, ws.bay);
    const int storeys = std::min(c.age == AGE_MODERN ? 8 : 5, (int)std::lround(H / storey));
    // the front's face at a height over the foot as `walls4x` leans or jetties it: a battered wall leaning in, a bulging one a little, an
    // upper storey jettied out or set back over its tier
    const bool tiered = (prof == WP_LEANING || prof == WP_STEPPED) && H > 4.5;
    const double tierY = tiered ? clampd(H * 0.5, 2.9, 4.2) : 1e9, jetty = prof == WP_LEANING ? 0.5 : -0.45;
    const double lean = prof == WP_BATTERED ? std::atan(std::min(0.1 * H, 1.25 * ht) / std::max(1.0, H)) : (prof == WP_BULGING ? 0.035 : (prof == WP_LEANING && !tiered ? -0.06 : 0.0));
    auto faceOff = [&](double y) { return (y - base >= tierY - 0.01 ? jetty : 0.0) - std::tan(lean) * (y - base); };
    auto upAt = [&](double y) { Face q = f; const double o = faceOff(y); q.fx += q.nx * o; q.fz += q.nz * o; return q; };   // the face as it stands at y
    const bool doorFront = W.door == 0;
    auto clearOfDoor = [&](double t, double a) { return !doorFront || std::fabs(t - W.doorPos) > a + 1.25; };
    int sub, rs, rm; railOf(g, false, sub, rs, rm);
    const bool railUp = g.D(rm) < 0.55;
    const int slabMat = c.age == AGE_MEDIEVAL ? MT_TIMBER : (c.age == AGE_MODERN ? MT_CONCRETE : W.mat), slabSlot = slabMat == MT_TIMBER ? g.timberSlot() : W.slot;
    const bool slabUp = g.D(slabMat) < 0.5;
    switch (pj) {
        case PJ_BALCONIES: {
            if (storeys < 2 || !slabUp || g.lod != 0) return;
            const int nb = clampi((int)std::lround(L / bay), 1, 3);
            for (int s = 1; s < storeys; s++)
                for (int j = 0; j < nb; j++) {
                    const double tb = -L + (j + 0.5) * 2 * L / nb, a = std::min(1.1, L / nb - 0.25), y = base + s * storey;
                    if (a < 0.5) continue;
                    const Face q = upAt(y);
                    faceBox(g, q, tb, 0.5, a, 0.5, y - 0.18, y, slabSlot, slabMat, 2);
                    if (railUp) { Form& r = faceFence(g, q, false, tb, 0.95, a, y, y + 1.0, sub, rs, rm); r.p[3] = 0; }
                }
            return;
        }
        case PJ_GALLERIES: {
            if (!slabUp || g.lod != 0) return;
            if (storeys < 2) {   // one storey: a verandah along its front, its posts and its lean-to roof under the eaves
                if (H < 2.6 || c.age == AGE_MODERN) return;
                const double y = base + std::min(H - 0.15, 2.7);
                const int np = std::max(1, (int)std::lround(2 * L / 2.6));
                for (int k = 0; k <= np; k++) {
                    const double tp = -L + 2 * L * k / np;
                    if (!clearOfDoor(tp, 0.2)) continue;
                    double x, z; faceAt(f, tp, 1.55, x, z);
                    if (c.age == AGE_INDUSTRIAL) { if (g.D(MT_IRON) < 0.6) g.lathe(0, x, z, 0.08, 0, y - 0.1, g.ironSlot(), MT_IRON, 6); }
                    else g.post(x, z, y - 0.1, 0.11, 560 + k);
                }
                if (g.D(MT_TIMBER) < 0.35) { Form& r = faceBox(g, f, 0, 0.85, L + 0.25, 0.85, y - 0.1, y + 0.06, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); r.tilt = 0.2; }   // its outer edge lower
                return;
            }
            for (int s = 1; s < storeys; s++) {
                const double y = base + s * storey;
                const Face q = upAt(y);
                faceBox(g, q, 0, 0.75, L + 0.2, 0.75, y - 0.2, y, slabSlot, slabMat, 2);
                if (railUp) faceFence(g, q, false, 0, 1.45, L + 0.15, y, y + 1.0, sub, rs, rm);
                if (s == 1 && c.age != AGE_MODERN)   // its posts down to the ground: a verandah under the first
                    for (int k = 0; k <= std::max(1, (int)std::lround(2 * L / 2.6)); k++) {
                        const double tp = -L + 2 * L * k / std::max(1, (int)std::lround(2 * L / 2.6));
                        if (!clearOfDoor(tp, 0.2)) continue;
                        double x, z; faceAt(f, tp, 1.35, x, z);
                        if (c.age == AGE_INDUSTRIAL) { if (g.D(MT_IRON) < 0.6) g.lathe(0, x, z, 0.09, 0, y - 0.2, g.ironSlot(), MT_IRON, 6); }
                        else g.post(x, z, y - 0.2 - base, 0.12, 540 + k);
                    }
            }
            return;
        }
        case PJ_BAYS: {
            if (prof == WP_BATTERED) return;   // nothing stands against a wall leaning in
            const double a = std::min(1.2, L * 0.3), top = std::min(tiered ? tierY - 0.05 : 1e9, H > 8 ? std::min(H - 0.3, 2 * storey) : H - 0.3);
            if (a < 0.7 || top < 2.4) return;
            for (int s = -1; s <= 1; s += 2) {
                const double tb = s * L * 0.55;
                if (!clearOfDoor(tb, a) || std::fabs(tb) + a > L - 0.2) continue;
                { Form& e = faceBox(g, f, tb, 0.45, a, 0.45, base, base + top, wallsSlot(g, W), W.mat); e.pattern = W.pattern; }
                if (g.lod == 0) {   // its roof
                    if (c.age == AGE_MODERN) faceBox(g, f, tb, 0.5, a + 0.12, 0.55, base + top, base + top + 0.25, W.slot, W.mat, 2);
                    else if (g.D(MT_TIMBER) < 0.4) {
                        double x, z;
                        faceAt(f, tb, 0.35, x, z);
                        Form& r = g.add(FK_PYRAMID, 0, x, z, f.dh, a + 0.15, 0.7, base + top, base + top + 0.55, g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
                        r.p[1] = 0.25;
                    }
                }
            }
            return;
        }
        case PJ_ORIELS: {
            if (H < 3.6) return;
            const double a = std::min(0.95, L * 0.28), y0 = base + std::max(tiered ? tierY : 2.0, std::min(storey, H * 0.45)),
                         y1 = base + std::min(H - 0.3, y0 - base + storey * (storeys > 2 ? 2 : 1) - 0.2);
            if (a < 0.6 || y1 - y0 < 1.2) return;
            const int n = L > 4.2 ? 2 : 1;
            for (int j = 0; j < n; j++) {
                const double tb = n == 1 ? (doorFront ? (W.doorPos > 0 ? -L * 0.45 : L * 0.45) : 0) : (j ? 1 : -1) * L * 0.5;
                const Face q = upAt(y0);
                Form& e = faceBox(g, q, tb, 0.42, a, 0.42, y0, y1, wallsSlot(g, W), W.mat); e.pattern = W.pattern;
                faceBox(g, q, tb, 0.25, a * 0.7, 0.25, y0 - 0.7, y0, wallsSlot(g, W), W.mat, 4);   // its corbel
                if (g.lod == 0) {
                    if (c.age == AGE_MODERN) faceBox(g, q, tb, 0.45, a + 0.08, 0.47, y1, y1 + 0.2, W.slot, W.mat, 2);
                    else if (g.D(MT_TIMBER) < 0.4) {
                        double x, z;
                        faceAt(q, tb, 0.42, x, z);
                        Form& r = g.add(FK_PYRAMID, 0, x, z, f.dh, a + 0.1, 0.52, y1, y1 + 0.9 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
                        r.p[1] = 0;
                    }
                }
            }
            return;
        }
        default: return;
    }
}
}   // namespace

// what rises from its roof: on a pitched or a hipped roof dormers along its front slope, a cross gable in the middle of it, cresting along its
// ridge (a medieval people's pales of its roofing, an industrial people's iron), a lantern on it (a medieval people's bell-cote, an
// industrial people's ventilator); on a modern people's flat roof a pergola, a water tank on its legs, rails round its edge, a penthouse
void rooflineOf(Gen& g, const Walls& W, double hmin, int rf, double rise, int roofMat) {
    const Culture& c = g.c;
    if (c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || g.lod != 0 || hmin < 0.92 * W.h || W.hw < 1.6 || W.hd < 1.6) return;
    const int rl = g.gv(GV_ROOFLINE);
    if (rl == RL_PLAIN) return;
    const double y0 = g.sink + W.y0 + hmin - 0.05;
    if (rf == RF_FLAT && c.age != AGE_MODERN) {   // a medieval or an industrial people's flat roof: a pavilion on it, a stair house, a balustrade
        const int stone = W.mat;                     // round its edge, a cupola on its drum
        if (g.D(stone) > 0.45 || !structural(stone)) return;
        const double top = g.sink + W.y0 + hmin + 0.3, hw = W.hw, hd = W.hd;
        switch (rl) {
            case RL_DORMERS: {
                if (hw < 2.0 || hd < 2.0) return;
                const double px = W.cx - hw * 0.35, pz = W.cz - hd * 0.3;
                Form& p = g.box(px, pz, 0, hw * 0.42, hd * 0.42, top, top + 2.2, wallsSlot(g, W), stone); p.pattern = W.pattern;
                g.box(px, pz, 0, hw * 0.42 + 0.2, hd * 0.42 + 0.2, top + 2.2, top + 2.45, W.slot, stone, 2);
                return;
            }
            case RL_GABLETS:
                g.box(W.cx + hw - 0.9, W.cz - hd + 0.9, 0, 0.75, 0.75, top, top + 2.3, wallsSlot(g, W), stone);
                g.box(W.cx + hw - 0.9, W.cz - hd + 0.9, 0, 0.9, 0.9, top + 2.3, top + 2.5, W.slot, stone, 2);
                return;
            case RL_CRESTED:
                for (int s = 0; s < 4; s++) {
                    const bool al = s % 2 == 0;
                    const double x = W.cx + (s == 1 ? hw : (s == 3 ? -hw : 0)), z = W.cz + (s == 0 ? hd : (s == 2 ? -hd : 0));
                    g.add(FK_FENCE, 3, x, z, al ? PI / 2 : 0, 0.1, (al ? hw : hd) - 0.05, top, top + 0.8, W.slot, stone);
                }
                return;
            default: {
                const double r = clampd(0.22 * std::min(hw, hd), 0.6, 1.2);
                g.lathe(0, W.cx, W.cz, r, top, top + 0.9, W.slot, stone, 8);
                g.add(FK_DOME, ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : 0)), W.cx, W.cz, 0, r + 0.1, r + 0.1, top + 0.9,
                        top + 0.9 + r * 1.1, g.roofSlot(stone), stone, 2);
                return;
            }
        }
    }
    if (rf == RF_FLAT) {
        if (c.age != AGE_MODERN || g.D(MT_CONCRETE) > 0.45) return;
        const double top = g.sink + W.y0 + hmin + 0.3, hw = W.hw, hd = W.hd;
        switch (rl) {
            case RL_DORMERS: {   // a pergola over a corner of it
                if (g.D(MT_IRON) > 0.5 || hw < 2.5 || hd < 2.5) return;
                const double px = W.cx + hw * 0.45, pz = W.cz + hd * 0.4, ex = std::min(2.0, hw * 0.4), ez = std::min(2.0, hd * 0.45);
                for (int k = 0; k < 4; k++) g.lathe(0, px + (k % 2 ? ex : -ex), pz + (k < 2 ? ez : -ez), 0.07, top, top + 2.3, g.ironSlot(), MT_IRON, 4);
                for (int k = -1; k <= 1; k += 2) g.box(px, pz + k * ez, 0, ex + 0.1, 0.05, top + 2.15, top + 2.3, g.ironSlot(), MT_IRON, 4);   // its girts on the posts
                for (int k = -2; k <= 2; k++) g.box(px + k * ex * 0.45, pz, 0, 0.05, ez + 0.2, top + 2.3, top + 2.45, g.ironSlot(), MT_IRON, 4);   // its beams on them
                return;
            }
            case RL_GABLETS: {   // a water tank on its legs
                const double tx = W.cx - hw * 0.4, tz = W.cz - hd * 0.3, r = clampd(0.22 * std::min(hw, hd), 0.9, 1.8);
                for (int k = 0; k < 4; k++) {
                    const double a = PI / 4 + k * PI / 2;
                    g.lathe(0, tx + std::sin(a) * r * 0.8, tz + std::cos(a) * r * 0.8, 0.08, top, top + 1.4, g.ironSlot(), MT_IRON, 4);
                }
                Form& t = g.lathe(4, tx, tz, r, top + 1.4, top + 1.4 + 2.2 * r, 0, MT_CONCRETE); t.p[0] = 0.35;
                return;
            }
            case RL_CRESTED: {   // rails round its edge
                if (g.D(MT_IRON) > 0.5) return;
                for (int s = 0; s < 4; s++) {
                    const bool al = s % 2 == 0;
                    const double x = W.cx + (s == 1 ? hw : (s == 3 ? -hw : 0)), z = W.cz + (s == 0 ? hd : (s == 2 ? -hd : 0));
                    g.add(FK_FENCE, 1, x, z, al ? PI / 2 : 0, 0.025, (al ? hw : hd) - 0.05, top, top + 1.0, g.ironSlot(), MT_IRON);
                }
                return;
            }
            default: {   // a penthouse
                if (hw < 3 || hd < 3) return;
                Form& p = g.box(W.cx - hw * 0.2, W.cz - hd * 0.15, 0, hw * 0.55, hd * 0.55, top, top + 2.9, wallsSlot(g, W), W.mat); p.pattern = W.pattern;
                g.box(W.cx - hw * 0.2, W.cz - hd * 0.15, 0, hw * 0.55 + 0.3, hd * 0.55 + 0.3, top + 2.9, top + 3.15, W.slot, W.mat, 2);
                return;
            }
        }
    }
    if (rise < 0.8 || g.D(roofMat) > 0.25) return;
    if (rf == RF_DOMED || rf == RF_CONICAL || rf == RF_VAULTED) {   // a lantern on a dome's or a cone's crown, a vault's ridge crested or its lantern
        const double yTop = y0 + rise, along = std::max(W.hw, W.hd) - 0.3;
        if (rl == RL_LANTERN) {
            const double r = clampd(0.15 * std::min(W.hw, W.hd), 0.45, 0.9);
            g.lathe(0, W.cx, W.cz, r, yTop - 0.3, yTop + 1.1, wallsSlot(g, W), W.mat, 6);
            Form& s = g.lathe(1, W.cx, W.cz, r + 0.12, yTop + 1.1, yTop + 1.1 + r * 2.2 * g.pitchK(), g.roofSlot(roofMat), roofMat, 6); s.p[0] = 0.04;
        } else if (rl == RL_CRESTED && rf == RF_VAULTED) {
            Form& e = g.add(FK_FENCE, c.age == AGE_INDUSTRIAL ? 1 : 0, W.cx, W.cz, W.hd >= W.hw ? 0 : PI / 2, 0.03, along, yTop - 0.08, yTop + 0.42, g.roofSlot(roofMat), roofMat, 4);
            e.p[0] = 0.3; e.p[1] = 0.5; e.p[3] = 0.13;
        }
        return;
    }
    if (!(rf == RF_PITCHED || rf == RF_HIPPED)) return;
    const bool alongZ = W.hd >= W.hw, hip = rf == RF_HIPPED;   // the ridge's run (`roof`)
    const double eave = g.eave(), span = (alongZ ? W.hw : W.hd) + eave, len = (alongZ ? W.hd : W.hw) + (hip ? eave : eave * 0.6);
    const double topHalf = hip ? 0.28 * span : 0.12, ridgeLen = hip ? std::max(0.3, len - span + 0.28 * span) : len, yTop = y0 + rise;
    auto roofAt = [&](double w) { return y0 + rise * clampd((span - w) / std::max(0.1, span - topHalf), 0, 1); };
    // the frame along the ridge: u along it, w across it toward the front slope (+z' where the ridge runs along x', +x' where along z')
    auto at = [&](double u, double w, double& x, double& z) { if (alongZ) { x = W.cx + w; z = W.cz + u; } else { x = W.cx + u; z = W.cz + w; } };
    const double dhRun = alongZ ? 0 : PI / 2, dhCross = alongZ ? PI / 2 : 0;   // a sweep or a fence along the ridge, or across it (a box across it lies along it)
    const int rslot = g.roofSlot(roofMat);
    const double wall = (alongZ ? W.hw : W.hd);
    switch (rl) {
        case RL_DORMERS: {
            const int n = hip ? 1 : clampi((int)std::lround(2 * (len - eave) / 4.2), 1, 3);
            const double wf = span * 0.5, yr = roofAt(wf + 0.12);
            for (int k = 0; k < n; k++) {
                const double u = n == 1 ? 0 : -(len - eave) + (k + 0.5) * 2 * (len - eave) / n;
                double x, z;
                at(u, wf - 0.5, x, z); g.box(x, z, dhCross, 0.62, 0.62, yr - 0.45, yr + 1.1, wallsSlot(g, W), W.mat);
                at(u, wf + 0.13, x, z); g.box(x, z, dhCross, 0.32, 0.03, yr + 0.1, yr + 0.85, g.timberSlot(), MT_TIMBER, 4);   // its window
                at(u, wf - 0.45, x, z); Form& r = g.add(FK_SWEEP, 1, x, z, dhCross, 0, 0.78, yr + 1.05, yr + 1.05 + 0.82 * 0.8 * g.pitchK(), rslot, roofMat, 2); r.p[1] = 0.82; r.p[2] = 0.06;
            }
            return;
        }
        case RL_GABLETS: {   // a cross gable in the middle of the front slope, its own gable end in the walls' material over the front wall
            const double cw = std::min({len * 0.42, 2.4, span * 0.85}), ps = rise / std::max(0.1, span - topHalf);
            if (cw < 1.0) return;
            double x, z;
            at(0, span * 0.5, x, z); Form& r = g.add(FK_SWEEP, 1, x, z, dhCross, 0, span * 0.5 + 0.1, y0, y0 + cw * ps, rslot, roofMat, 2); r.p[1] = cw; r.p[2] = 0.1;
            at(0, wall, x, z); Form& e = g.add(FK_SWEEP, 1, x, z, dhCross, 0, 0.2, y0 + 0.02, y0 + (cw - eave) * ps, wallsSlot(g, W), W.mat); e.p[1] = cw - eave + 0.2; e.p[2] = 0.03;
            return;
        }
        case RL_CRESTED: {
            int sub = 0, slot = rslot, mat = roofMat;
            if (c.age == AGE_INDUSTRIAL) { sub = 1; slot = g.ironSlot(); mat = MT_IRON; if (g.D(MT_IRON) > 0.5) return; }
            Form& e = g.add(FK_FENCE, sub, W.cx, W.cz, dhRun, 0.03, std::max(0.4, (hip ? ridgeLen : len) - 0.2), yTop - 0.08, yTop + (sub ? 0.5 : 0.38), slot, mat, 4);
            e.p[0] = sub ? 0.16 : 0.32; e.p[1] = sub ? 0.25 : 0.55; e.p[3] = sub ? 0.1 : 0.14;
            return;
        }
        default: {   // a lantern on the ridge's middle
            if (c.age == AGE_INDUSTRIAL) {   // a ventilator: a louvred box under its own little roof
                g.box(W.cx, W.cz, dhRun, 0.45, std::min(1.0, ridgeLen * 0.4), yTop - 0.4, yTop + 0.75, g.timberSlot(), MT_TIMBER);
                Form& r = g.add(FK_SWEEP, 1, W.cx, W.cz, dhRun, 0, std::min(1.0, ridgeLen * 0.4) + 0.15, yTop + 0.7, yTop + 1.15, rslot, roofMat, 2); r.p[1] = 0.7; r.p[2] = 0.05;
                return;
            }
            Form& l = g.lathe(0, W.cx, W.cz, 0.55, yTop - 0.4, yTop + 1.05, wallsSlot(g, W), W.mat, 6); l.pattern = 0;
            if (c.roof == RF_DOMED) g.add(FK_DOME, ageDome(g, 3), W.cx, W.cz, 0, 0.62, 0.62, yTop + 1.05, yTop + 1.9, rslot, roofMat, 2);
            else { Form& s = g.lathe(1, W.cx, W.cz, 0.68, yTop + 1.05, yTop + 1.05 + 1.3 * g.pitchK(), rslot, roofMat, 6); s.p[0] = 0.04; }
            return;
        }
    }
}

// R-412: what stands out from a round house's wall (a medieval, an industrial or a modern people's): a verandah round it on its posts under
// a ring of roof, round bays from the ground under their cones, oriels on their corbels, balconies on an upper storey; at its sides'
// middles for a polygon, round it for a circle or an oval, none in its doorway
void ringDress(Gen& g, const Ring& R, double hmin) {
    const Culture& c = g.c;
    if (g.lod > 1 || c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || hmin < 0.92 * R.h || !dwellingKind(g.b.kind) || g.sink < -0.6 || R.y0 > 0.5) return;
    const int pj = g.gv(GV_PROJECTIONS);
    if (pj == PJ_NONE) return;
    const int segs = R.segs;
    const bool round = segs >= 10;
    std::vector<std::pair<double, double>> wallAt;   // the angle, the wall's distance from the middle there
    for (int k = 0; k < segs; k++) {
        if (ringInDoor(R, k) || ringInDoor(R, (k + segs - 1) % segs) || ringInDoor(R, (k + 1) % segs)) continue;   // clear of the doorway
        const double a = (k + 0.5) * TAU / segs;
        wallAt.push_back({a, round ? std::hypot(std::sin(a) * R.rx, std::cos(a) * R.rz) : R.rx * std::cos(PI / segs)});
    }
    if (wallAt.empty()) return;
    const int want = std::min<int>(3, (int)wallAt.size());
    const double base = g.sink + R.y0, H = R.h, ht = 0.5 * R.thick;
    const int wslot = R.slot == 0 && g.painted ? 1 : R.slot;
    auto pick = [&](int j) { return wallAt[(size_t)((j * wallAt.size()) / want)]; };
    auto spot = [&](const std::pair<double, double>& w, double out, double& x, double& z) { x = R.cx + std::sin(w.first) * (w.second + out); z = R.cz + std::cos(w.first) * (w.second + out); };
    switch (pj) {
        case PJ_GALLERIES: {   // a verandah round it
            if (H < 2.6 || g.lod != 0 || c.age == AGE_MODERN) return;
            const double y = base + std::min(H - 0.15, 2.6), rr = std::max(R.rx, R.rz) + 1.6;
            const int np = clampi((int)std::lround(TAU * rr / 2.8), 6, 24);
            const double doorA = (R.door + 0.5 * R.doorSegs) * TAU / segs;
            for (int k = 0; k < np; k++) {
                const double a = (k + 0.5) * TAU / np;
                if (std::fabs(std::remainder(a - doorA, TAU)) * rr < 1.4) continue;   // its doorway's path
                const double x = R.cx + std::sin(a) * (R.rx + 1.5), z = R.cz + std::cos(a) * (R.rz + 1.5);
                if (c.age == AGE_INDUSTRIAL) { if (g.D(MT_IRON) < 0.6) g.lathe(0, x, z, 0.08, 0, y - 0.1, g.ironSlot(), MT_IRON, 6); }
                else g.post(x, z, y - 0.1, 0.11, 580 + k);
            }
            if (g.D(MT_TIMBER) < 0.35) {   // its lean-to roof: a ring of boards sloping out from the wall (a hollow cone's collider filled the room)
                const int nb = clampi((int)std::lround(TAU * rr / 2.4), 8, 20);
                for (int k = 0; k < nb; k++) {
                    const double a = (k + 0.5) * TAU / nb, x = R.cx + std::sin(a) * (R.rx + ht + 0.85), z = R.cz + std::cos(a) * (R.rz + ht + 0.85);
                    Form& b = g.add(FK_BOX, 0, x, z, a, rr * std::sin(PI / nb) + 0.12, 0.9, y - 0.05, y + 0.07, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); b.tilt = 0.28;   // its outer edge lower
                }
            }
            return;
        }
        case PJ_BAYS: {   // round bays from the ground under their cones
            if (H < 2.4) return;
            for (int j = 0; j < want; j++) {
                double x, z; spot(pick(j), 0.25, x, z);
                Form& e = g.lathe(0, x, z, 0.85, base, base + H - 0.3, wslot, R.mat, 6); e.pattern = R.pattern;
                if (g.lod == 0 && g.D(MT_TIMBER) < 0.4) { Form& k = g.lathe(1, x, z, 0.95, base + H - 0.3, base + H + 0.5 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 6); k.p[0] = 0.05; }
            }
            return;
        }
        case PJ_ORIELS: {   // on their corbels, up its wall
            if (H < 3.4) return;
            for (int j = 0; j < want; j++) {
                double x, z; spot(pick(j), 0.3, x, z);
                const double y0 = base + std::max(1.9, 0.45 * H);
                { Form& k = g.lathe(1, x, z, 0.25, y0 - 0.8, y0, wslot, R.mat, 6); k.p[0] = 2.6; }
                Form& e = g.lathe(0, x, z, 0.65, y0, base + H - 0.2, wslot, R.mat, 6); e.pattern = R.pattern;
                if (g.lod == 0 && g.D(MT_TIMBER) < 0.4) { Form& cap = g.lathe(1, x, z, 0.75, base + H - 0.2, base + H + 0.7 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 6); cap.p[0] = 0.05; }
            }
            return;
        }
        default: {   // balconies on an upper storey
            if (H < 4.6 || g.lod != 0) return;
            const int slabMat = c.age == AGE_MEDIEVAL ? MT_TIMBER : (c.age == AGE_MODERN ? MT_CONCRETE : R.mat);
            if (g.D(slabMat) > 0.5) return;
            for (int j = 0; j < want; j++) {
                double x, z; spot(pick(j), 0.3, x, z);
                const double y = base + std::min(H - 1.2, c.age == AGE_MEDIEVAL ? 2.7 : 3.2);
                Form& e = g.lathe(0, x, z, 1.0, y - 0.18, y, slabMat == MT_TIMBER ? g.timberSlot() : wslot, slabMat); e.part = 2;
            }
            return;
        }
    }
}

// R-412: the dress a dwelling's body takes from its people (its doorway, its corners, its front, its roof), each where its age had it
void houseDress(Gen& g, const Walls& W, const double hs[4], double hmin, int rf, double rise, int roofMat, bool roofline) {
    if (g.lod > 1 || !W.dress) return;
    const int kind = g.b.kind;
    const bool dwelling = dwellingKind(kind) && !(kind == BK_HOUSE && g.c.age >= AGE_INDUSTRIAL);   // an industrial people's warehouse goes bare
    if (!dwelling && kind != BK_HALL) return;
    const int prof = W.profile >= 0 ? W.profile : g.gv(GV_PROFILE);
    const double ht = wallsHalf(g, W);
    entranceAt(g, W, ht, W.h);
    if (dwelling && roofline) rooflineOf(g, W, hmin, rf, rise, roofMat);
    if (prof == WP_UPRIGHT || prof == WP_BULGING) cornersOf(g, W, hs, ht, roofMat);   // a battered or a jettied front's corners its profile's
    if (dwelling) projectionsOf(g, W, hs, ht, prof);
}

namespace {

// R-412: a yard's boundary: along the front to the street where a town's house stands back from it (`Building::yard`), its gate before the
// doorway, its returns back to the house's front corners; else round a garden behind a house that stands alone. A low wall of the walls'
// stone, brick or concrete; a fence of pales (a medieval people's timber, a later people's painted); a hedge (a wall on a desert); railings
// (an industrial people's iron on a low wall, a medieval people's balustrade, a modern people's steel on a kerb). Each under C-01's 0.9 m,
// stepped over as a low wall is
void yardRun(Gen& g, double x0, double z0, double x1, double z1, int mat) {
    const double len = std::hypot(x1 - x0, z1 - z0);
    if (len < 0.4) return;
    const double cx = 0.5 * (x0 + x1), cz = 0.5 * (z0 + z1), dh = std::atan2(x1 - x0, z1 - z0);   // its length along its heading
    const Culture& c = g.c;
    switch (g.gv(GV_BOUNDARY)) {
        case BD_WALLS: { const double D = g.D(mat); if (D < 0.85) g.box(cx, cz, dh, 0.18, len * 0.5, 0, 0.8 * wallShare(D, g.U(611)), 0, mat); return; }
        case BD_FENCES: {
            const double D = g.D(MT_TIMBER);
            if (D > 0.85) return;
            Form& e = g.add(FK_FENCE, 0, cx, cz, dh, 0.025, len * 0.5, 0, 0.85, c.age == AGE_MEDIEVAL || c.age == AGE_PREHISTORIC ? g.timberSlot() : 0, MT_TIMBER);
            e.p[3] = 0.1; e.broken = D < 0.25 ? 0 : clampd(D, 0.3, 0.9);
            return;
        }
        case BD_HEDGES:
            if (g.st.desert) { g.box(cx, cz, dh, 0.18, len * 0.5, 0, 0.7, 0, mat); return; }
            g.box(cx, cz, dh, 0.32, len * 0.5, 0, 0.85, 0, -1, 11);
            return;
        default: {   // railings
            if (c.age == AGE_MEDIEVAL) { Form& e = g.add(FK_FENCE, 3, cx, cz, dh, 0.1, len * 0.5, 0, 0.8, 0, mat); e.broken = clampd(g.D(mat) - 0.3, 0, 0.9); return; }
            const double kerb = c.age == AGE_INDUSTRIAL ? 0.4 : 0.15;
            g.box(cx, cz, dh, 0.16, len * 0.5, 0, kerb, 0, c.age == AGE_INDUSTRIAL ? mat : MT_CONCRETE);
            const double Di = g.D(MT_IRON);
            if (Di < 0.6) {
                Form& e = g.add(FK_FENCE, 1, cx, cz, dh, 0.02, len * 0.5, kerb, 0.85, g.ironSlot(), MT_IRON);
                e.p[3] = c.age == AGE_INDUSTRIAL ? 0.09 : 0;
                e.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.9);
            }
            return;
        }
    }
}
}   // namespace
void yardOf(Gen& g, double hw, double hd, int doorSide, double doorPos, int mat) {
    const Building& b = g.b; const Culture& c = g.c;
    if (g.lod != 0 || c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || g.gv(GV_BOUNDARY) == BD_OPEN) return;
    if (!dwellingKind(b.kind) || b.kind == BK_POD || (b.kind == BK_HOUSE && c.age >= AGE_INDUSTRIAL)) return;
    if (b.yard >= 2.2) {   // its front yard
        const double zf = hd + b.yard - 1.4, xa = b.joined ? -hw : -hw - 0.5, xb = b.joined ? hw : hw + 0.5;   // its kerb before it (the lamps')
        if (doorSide == 0) { yardRun(g, xa, zf, doorPos - 1.1, zf, mat); yardRun(g, doorPos + 1.1, zf, xb, zf, mat); }
        else yardRun(g, xa, zf, xb, zf, mat);
        if (!b.joined) { yardRun(g, xa, hd + 0.35, xa, zf, mat); yardRun(g, xb, hd + 0.35, xb, zf, mat); }
        return;
    }
    if (b.joined || b.kind == BK_BLOCK) return;
    const double zb = -hd - 3.2, xa = -hw - 0.4, xb = hw + 0.4;   // a garden behind it, its gate in its back
    yardRun(g, xa, -hd - 0.35, xa, zb, mat); yardRun(g, xb, -hd - 0.35, xb, zb, mat);
    yardRun(g, xa, zb, -1.0, zb, mat); yardRun(g, 1.0, zb, xb, zb, mat);
    (void)doorPos;
}

// R-412: a prehistoric people's pole by a hut's door (`GV_POLES`): a carved post (a totem), a pole flying a cloth of its accent, a pole with
// a skull on it, a cairn of three stones; and its yard (`GV_BOUNDARY`): a ring of wattle, of dry stone, of thorn, of pales round the hut
void hutPole(Gen& g, double px, double pz) {
    if (g.lod != 0) return;
    const double Dt = g.D(MT_TIMBER);
    switch (g.gv(GV_POLES)) {
        case PO_TOTEMS: {
            if (Dt > 0.6) return;
            const double h = 3.2 + 1.4 * g.U(620), r = 0.2, hh = Dt < 0.3 ? h : h * (1 - Dt);
            g.lathe(0, px, pz, r, 0, hh, g.timberSlot(), MT_TIMBER, 4);
            for (int k = 1; k <= 2; k++)
                if (hh > h * k / 3 + 0.3) {
                    Form& e = g.box(px, pz, 0, r * 1.5, r * 1.5, h * k / 3 - 0.25, h * k / 3 + 0.25, k == 2 ? 1 : g.timberSlot(), MT_TIMBER, 4);
                    e.heading += PI / 4;
                }
            if (Dt < 0.3) g.box(px, pz, 0, 0.75, 0.12, h - 0.55, h - 0.35, 1, MT_TIMBER, 4);   // its wings
            return;
        }
        case PO_BANNERS: {
            if (Dt > 0.5) return;
            g.lathe(0, px, pz, 0.06, 0, 4.6, g.timberSlot(), MT_TIMBER, 4);
            if (Dt < 0.25) { g.box(px, pz, 0, 0.04, 0.55, 4.45, 4.55, g.timberSlot(), MT_TIMBER, 4); g.box(px, pz + 0.05, 0, 0.02, 0.5, 3.0, 4.45, 1, MT_TIMBER, 4); }   // its cloth
            return;
        }
        case PO_SKULLS: {
            if (Dt > 0.6) return;
            g.lathe(0, px, pz, 0.07, 0, 2.3, g.timberSlot(), MT_TIMBER, 4);
            Form& s = g.add(FK_BLOB, 0, px, pz, 0, 0.16, 0.19, 2.25, 2.6, 0, -1, FORM_PART_PALE); s.p[0] = 0; s.p[1] = 0.1;
            for (int k = -1; k <= 1; k += 2) { Form& h = g.box(px + k * 0.22, pz, 0, 0.16, 0.03, 2.5, 2.56, 0, -1, FORM_PART_PALE); h.tilt = 0.5 * k; }   // its horns
            return;
        }
        case PO_CAIRNS: {
            const double Ds = g.D(MT_DRYSTONE);
            const int tiers = Ds < 0.35 ? 3 : (Ds < 0.7 ? 2 : 1);
            for (int i = 0; i < tiers; i++) g.box(px, pz, i * 0.4, 0.45 * (1 - 0.28 * i), 0.4 * (1 - 0.28 * i), 0.28 * i, 0.28 * (i + 1), 0, MT_DRYSTONE, 3);
            return;
        }
        default: return;
    }
}
void hutRing(Gen& g, double rx, double rz, double doorA) {
    if (g.lod != 0) return;
    const int bd = g.gv(GV_BOUNDARY);
    if (bd == BD_OPEN) return;
    const double Rx = rx + 1.0, Rz = rz + 1.0;
    const int N = 10;
    for (int k = 0; k < N; k++) {
        const double a0 = (k + 0.08) * TAU / N, a1 = (k + 0.92) * TAU / N, am = 0.5 * (a0 + a1);
        if (std::fabs(std::remainder(am - doorA, TAU)) < 0.45) continue;   // its gap at the doorway
        const double x0 = std::sin(a0) * Rx, z0 = std::cos(a0) * Rz, x1 = std::sin(a1) * Rx, z1 = std::cos(a1) * Rz;
        const double cx = 0.5 * (x0 + x1), cz = 0.5 * (z0 + z1), len = std::hypot(x1 - x0, z1 - z0), dh = std::atan2(x1 - x0, z1 - z0);
        switch (bd) {
            case BD_WALLS: { const double D = g.D(MT_DRYSTONE); g.box(cx, cz, dh, 0.25, len * 0.5, 0, std::max(0.25, 0.75 * wallShare(D, g.U(640 + k))), 0, MT_DRYSTONE, 3); break; }
            case BD_FENCES: {
                const double D = g.D(MT_TIMBER);
                if (D < 0.8) {
                    Form& e = g.add(FK_FENCE, 2, cx, cz, dh, 0.05, len * 0.5, 0, 0.85, g.timberSlot(), MT_TIMBER);
                    e.broken = D < 0.25 ? 0 : clampd(D, 0.3, 0.9);
                }
                break;
            }
            case BD_HEDGES: if (!g.st.desert) g.box(cx, cz, dh, 0.35, len * 0.5, 0, 0.8, 0, -1, 11); break;
            default: {
                const double D = g.D(MT_TIMBER);
                if (D < 0.7) {
                    Form& e = g.add(FK_FENCE, 0, cx, cz, dh, 0.06, len * 0.5, 0, 0.85, g.timberSlot(), MT_TIMBER);
                    e.p[0] = 0.24;
                    e.p[1] = 0.8;
                    e.p[3] = 0.12;
                    e.broken = D < 0.25 ? 0 : clampd(D, 0.3, 0.9);
                }
                break;
            }   // a ring of stakes
        }
    }
}
// R-412: a hut's entrance (`GV_ENTRANCE`): a porch passage of two short walls or rows of posts and a lintel, a threshold of stones, two
// portal posts, a hood of thatch
void hutEntrance(Gen& g, double rx, double rz, double doorA, double h, bool stone) {
    if (g.lod != 0) return;
    const double dx = std::sin(doorA), dz = std::cos(doorA), r = std::sqrt(rx * rx * dx * dx + rz * rz * dz * dz);
    const double heading = doorA, Dt = g.D(MT_TIMBER);
    auto at = [&](double across, double out, double& x, double& z) { x = dx * (r + out) + dz * across; z = dz * (r + out) - dx * across; };
    switch (g.gv(GV_ENTRANCE)) {
        case EN_PORCH: {   // a passage out from the doorway: its two cheeks and the lintel over it
            double x, z;
            for (int s = -1; s <= 1; s += 2) {   // as wide apart as the doorway
                at(s * 1.17, 0.7, x, z);
                if (stone) g.box(x, z, heading, 0.22, 0.75, 0, std::max(0.4, h * 0.85 * wallShare(g.D(MT_DRYSTONE), g.U(650 + (s > 0)))), 0, MT_DRYSTONE);
                else { at(s * 1.1, 1.3, x, z); g.post(x, z, h * 0.95, 0.11, 652 + (s > 0)); }
            }
            if (Dt < 0.35) {
                at(0, 0.75, x, z);
                Form& t = g.add(FK_SWEEP, 1, x, z, heading, 0, 0.8, h * 0.9, h * 0.9 + 0.75 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
                t.p[1] = 1.5;
                t.p[2] = 0.05;
            }
            return;
        }
        case EN_STEPS: { double x, z; at(0, 0.45, x, z); g.box(x, z, heading, 0.9, 0.42, 0, 0.16, 0, MT_DRYSTONE, 4); return; }   // a threshold slab
        case EN_PORTICO: { double x, z; for (int s = -1; s <= 1; s += 2) { at(s * 1.1, 0.6, x, z); g.post(x, z, h * 1.6, 0.15, 656 + (s > 0)); } return; }   // two tall portal posts
        case EN_CANOPY: {
            if (Dt < 0.4) {
                double x, z;
                at(0, 0.5, x, z);
                Form& c = g.box(x, z, heading, 1.1, 0.55, h * 0.95, h * 0.95 + 0.14, g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
                c.tilt = 0.28;
            }
            return;
        }
        default: return;
    }
}

// R-412: the people's street furniture (`BK_FURNITURE`, its style the `Furniture`): a well (a medieval people's curb under its little
// roof, an industrial people's iron pump, a modern people's drinking fountain, a futuristic people's column of water), a lamp (a lantern on
// its post, an iron lamp post, a street light on its arm, a pylon of light), a fountain's basin and its bowls, a figure on its plinth (a
// modern people's abstract one, a futuristic people's ring)
void genFurniture(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (g.lod == 2) return;
    const int age = c.age;
    const int stone = age == AGE_MODERN ? MT_CONCRETE : (age >= AGE_FUTURISTIC ? MT_COMPOSITE : (age == AGE_INDUSTRIAL ? MT_MASONRY : stoneOf(c)));
    const double Ds = g.D(stone), Di = g.D(MT_IRON), Dt = g.D(MT_TIMBER);
    switch (b.style) {
        case FU_WELLS:
            if (age == AGE_MEDIEVAL) {
                { Form& e = g.lathe(0, 0, 0, 0.85, 0, 0.8 * wallShare(Ds, g.U(10)), 0, stone, 10); e.p[3] = 1; }
                for (int s = -1; s <= 1; s += 2) g.post(s * 0.75, 0, 2.3, 0.08, 20 + (s > 0));
                if (Dt < 0.3 && g.lod == 0) {
                    g.box(0, 0, 0, 0.85, 0.06, 2.05, 2.17, g.timberSlot(), MT_TIMBER, 4);
                    Form& r = g.add(FK_SWEEP, 1, 0, 0, PI / 2, 0, 0.95, 2.2, 2.85, g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
                    r.p[1] = 0.85;
                    r.p[2] = 0.05;
                }
            } else if (age == AGE_INDUSTRIAL) {
                if (Di > 0.7) return;
                g.lathe(0, 0, 0, 0.13, 0, 1.45, g.ironSlot(), MT_IRON, 8);
                g.add(FK_DOME, 0, 0, 0, 0, 0.17, 0.17, 1.45, 1.62, g.ironSlot(), MT_IRON);
                if (g.lod == 0) {
                    g.box(0, 0.2, 0, 0.04, 0.2, 1.0, 1.08, g.ironSlot(), MT_IRON, 4);
                    Form& h = g.box(0, -0.25, 0, 0.03, 0.3, 1.3, 1.36, g.ironSlot(), MT_IRON, 4);
                    h.tilt = 0.35;
                    g.box(0, 0.55, 0, 0.35, 0.25, 0, 0.35, 0, MT_MASONRY, 4);
                }
            } else if (age == AGE_MODERN) {
                if (Ds > 0.7) return;
                g.box(0, 0, 0, 0.18, 0.18, 0, 0.92, 0, MT_CONCRETE, 4); g.box(0, 0.1, 0, 0.3, 0.25, 0.85, 1.0, 0, MT_CONCRETE, 4);
            } else { Form& e = g.lathe(0, 0, 0, 0.4, 0, 2.3, 2, MT_COMPOSITE); (void)e; }
            return;
        case FU_LAMPS:
            switch (age) {
                case AGE_MEDIEVAL:
                    g.post(0, 0, 2.8, 0.09, 30);
                    // its bracket, its lantern under it
                    if (Dt < 0.35 && g.lod == 0) { g.box(0, 0.22, 0, 0.05, 0.24, 2.6, 2.68, g.timberSlot(), MT_TIMBER, 4); g.box(0, 0.34, 0, 0.13, 0.13, 2.2, 2.55, g.ironSlot(), MT_IRON, 4); }
                    return;
                case AGE_INDUSTRIAL: {
                    if (Di > 0.7) return;
                    const double h = 3.9 * (Di < 0.35 ? 1.0 : 1 - Di);
                    g.lathe(0, 0, 0, 0.17, 0, 0.6, g.ironSlot(), MT_IRON, 8);
                    g.lathe(0, 0, 0, 0.07, 0.6, h, g.ironSlot(), MT_IRON, 6);
                    if (Di < 0.35 && g.lod == 0) {
                        Form& l = g.lathe(1, 0, 0, 0.13, h, h + 0.48, g.ironSlot(), MT_IRON, 4);
                        l.p[0] = 1.7;
                        Form& t = g.lathe(1, 0, 0, 0.26, h + 0.48, h + 0.72, g.ironSlot(), MT_IRON, 4);
                        t.p[0] = 0.1;
                        g.box(0, 0, 0, 0.36, 0.03, h - 0.5, h - 0.44, g.ironSlot(), MT_IRON, 4);
                    }
                    return;
                }
                case AGE_MODERN: {
                    if (Di > 0.75) return;
                    const double h = 7.5 * (Di < 0.4 ? 1.0 : 1.2 - Di);
                    g.lathe(1, 0, 0, 0.12, 0, h, g.ironSlot(), MT_IRON, 8).p[0] = 0.6;
                    // its arm, its lamp under the arm's end
                    if (Di < 0.4 && g.lod == 0) { g.box(0, 0.7, 0, 0.05, 0.75, h - 0.3, h, g.ironSlot(), MT_IRON, 4); g.box(0, 1.35, 0, 0.16, 0.3, h - 0.24, h - 0.04, g.ironSlot(), MT_IRON, 4); }
                    return;
                }
                default: {
                    g.lathe(1, 0, 0, 0.16, 0, 5.6, 2, MT_COMPOSITE, 6).p[0] = 0.5;
                    Form& t = g.add(FK_TORUS, 0, 0, 0, 0, 0.45, 0.45, 5.5, 5.62, 3, MT_COMPOSITE); t.p[0] = 0.06;
                    return;
                }
            }
        case FU_FOUNTAINS: {
            const double R = age == AGE_MODERN ? 3.0 : (age >= AGE_FUTURISTIC ? 2.6 : 2.2);
            if (age >= AGE_FUTURISTIC) { Form& t = g.add(FK_TORUS, 0, 0, 0, 0, R, R, 0, 0.5, 2, MT_COMPOSITE); t.p[0] = 0.25; g.lathe(0, 0, 0, 0.35, 0, 3.0, 2, MT_COMPOSITE); return; }
            { Form& e = g.lathe(0, 0, 0, R, 0, (age == AGE_MODERN ? 0.45 : 0.6) * wallShare(Ds, g.U(40)), 0, stone, 16); e.p[3] = 1; }
            if (age == AGE_MODERN) { if (Ds < 0.5) g.box(0, 0, 0.6, 0.3, 0.9, 0, 1.6, 0, MT_CONCRETE); return; }
            if (Ds > 0.55) return;
            g.lathe(0, 0, 0, 0.24, 0, 1.6, 0, stone, 8);
            { Form& w = g.lathe(1, 0, 0, 0.3, 1.6, 1.9, 0, stone, 12); w.p[0] = 2.7; }
            if (age == AGE_INDUSTRIAL && g.lod == 0) {
                g.lathe(0, 0, 0, 0.14, 1.9, 2.5, g.ironSlot(), MT_IRON, 8);
                Form& w2 = g.lathe(1, 0, 0, 0.16, 2.5, 2.7, g.ironSlot(), MT_IRON, 8);
                w2.p[0] = 2.8;
            }
            return;
        }
        default: {   // a figure on its plinth
            if (age >= AGE_FUTURISTIC) {
                g.lathe(0, 0, 0, 0.6, 0, 0.6, 0, MT_COMPOSITE);
                Form& t = g.add(FK_TORUS, 0, 0, 0, 0, 1.4, 1.4, 1.2, 4.24, 3, MT_COMPOSITE);
                t.p[0] = 0.12;
                t.tilt = PI / 2;
                return;
            }
            if (age == AGE_MODERN) {
                g.box(0, 0, 0, 0.9, 0.6, 0, 0.5, 0, MT_CONCRETE);
                if (Ds < 0.5) { Form& e = g.box(0, 0, 0.4, 0.22, 0.5, 0.5, 3.4, g.ironSlot(), MT_IRON); e.tilt = 0.22; }
                return;
            }
            g.box(0, 0, 0, 0.55, 0.55, 0, 1.5 * wallShare(Ds, g.U(50)), 0, stone);
            if (Ds < 0.45) figure(g, 0, 0, 1.5, 2.3, age == AGE_INDUSTRIAL ? g.ironSlot() : 0, age == AGE_INDUSTRIAL ? MT_IRON : stone, Ds);
            return;
        }
    }
}
