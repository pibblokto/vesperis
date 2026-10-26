// E-09: the generators' frame: what every age's buildings are made of (a figure, the tops, four walls and a ring of wall, the
// roofs and gables, a tower's shaft and its top, the foundation, stilts, drifts and rubble), declared in buildings_internal.h
#include "buildings_internal.h"

// ---- the generators' frame and helpers --------------------------------------------------------------------------------------

// A-04: a figure: a statue on the tops, a colossus at a town's heart: a robed body (a frustum), its shoulders, a head; `h` its height
void figure(Gen& g, double lx, double lz, double y0, double h, int slot, int mat, double D) {
    if (D > 0.8 || h < 0.5) return;
    const double r = h * 0.17;
    { Form& e = g.lathe(1, lx, lz, r, y0, y0 + h * 0.72, slot, mat, 8); e.p[0] = 0.62; e.broken = D > 0.45 ? 0.3 : 0; }
    if (D > 0.45) return;   // the head and the arms gone first
    { Form& e = g.add(FK_BLOB, 0, lx, lz, 0, h * 0.1, h * 0.1, y0 + h * 0.7, y0 + h * 0.92, slot, mat); e.p[0] = 0; e.p[1] = 0.1; }
    if (g.lod == 0) for (int s = -1; s <= 1; s += 2) { Form& a = g.box(lx + s * r * 0.75, lz, 0, r * 0.18, r * 0.2, y0 + h * 0.36, y0 + h * 0.68, slot, mat, 4); a.tilt = 0.05 * s; }
}

// A-04: the tops a people set on its roofs and towers at the given points (the building's frame): finials, a mast, a statue; crenels
// are the walls' (`walls4`), nothing is nothing
void topsAt(Gen& g, const std::vector<std::pair<double, double>>& pts, double y, double scale, int mat) {
    if (g.lod == 2) return;
    const int tp = g.gv(GV_TOPS); const double D = g.D(mat);
    if (D > 0.55 || tp == TP_CRENELS || tp == TP_NOTHING) return;
    const int slot = g.topSlot(mat), motif = g.gv(GV_MOTIF);
    for (const auto& p : pts) {
        switch (tp) {
            case TP_FINIALS:
                if (motif == MO_CIRCLES) { Form& e = g.add(FK_DOME, 0, p.first, p.second, 0, 0.32 * scale, 0.32 * scale, y, y + 0.32 * scale, slot, mat, 4); (void)e; }
                else { Form& e = g.lathe(2, p.first, p.second, 0.26 * scale, y, y + 1.6 * scale, slot, mat, motif == MO_STARS ? 4 : 0); e.p[0] = 1.2; }
                break;
            case TP_MASTS: { Form& e = g.lathe(0, p.first, p.second, 0.07 * scale, y, y + 4.5 * scale, g.timberSlot(), g.c.age >= AGE_INDUSTRIAL ? MT_IRON : MT_TIMBER, 4); (void)e; break; }
            default: figure(g, p.first, p.second, y, 1.9 * scale, slot, mat, D); break;
        }
    }
}

namespace {
WallsOut walls4x(Gen& g, const Walls& W0, double hs[4]) {
    Walls W = W0;
    if (W.slot == 0 && g.painted) W.slot = 1;   // R-412: a house painted in the accent
    const double Dm = g.D(W.mat);
    const bool tall = W.h > 8;
    const int prof = W.profile >= 0 ? W.profile : (W.dress ? g.gv(GV_PROFILE) : WP_UPRIGHT);
    double thick = W.thick;
    if (W.dress && g.gv(GV_EXPRESSION) == EX_MASSIVE) thick *= 1.35;
    if (prof == WP_BATTERED) thick = std::max(thick * 1.3, std::min(2.4, 0.12 * W.h));
    else if (prof == WP_BULGING) thick *= 1.6;
    WallsOut R; R.thick = thick;
    double hmin = 1e9, hsum = 0;
    for (int s = 0; s < 4; s++) { hs[s] = W.whole ? W.h : std::max(0.35, W.h * (tall ? tallShare(Dm, g.U(W.k0 + s)) : wallShare(Dm, g.U(W.k0 + s)))); hmin = std::min(hmin, hs[s]); hsum += hs[s]; }
    R.hmin = hmin;
    const double base = g.sink + W.y0;
    auto rag = [&](Form& e, double h) { if (tall && h < W.h * 0.98) e.broken = clampd(3.0 / std::max(h, 1.0), 0.08, 0.35); };
    if (g.lod == 2) { Form& e = g.box(W.cx, W.cz, 0, W.hw, W.hd, base, base + std::max(0.4, hsum * 0.25), W.slot, W.mat, W.part); e.pattern = W.pattern; return R; }
    const double ht = thick * 0.5;
    const bool tiered = (prof == WP_LEANING || prof == WP_STEPPED) && W.h > 4.5;
    const double tierY = tiered ? clampd(W.h * 0.5, 2.9, 4.2) : 1e9, jetty = prof == WP_LEANING ? 0.5 : -0.45;
    // the lean of each wall about its foot, inward positive: battered (on its thickened foot), a bulging wall's slight one, a low wall
    // leaning out
    const double lean = prof == WP_BATTERED ? std::atan(std::min(0.1 * W.h, 1.25 * ht) / std::max(1.0, W.h)) : (prof == WP_BULGING ? 0.035 : (prof == WP_LEANING && !tiered ? -0.06 : 0.0));
    const bool plinth = W.dress && prof == WP_STEPPED && !tiered;
    const int glyphAll = W.dress && (g.carved() || g.gv(GV_MOTIF) == MO_GLYPHS);
    for (int side = 0; side < 4; side++) {
        const bool along = side == 0 || side == 2;
        const double L = along ? W.hw : W.hd, h = hs[side];
        const double nx = side == 1 ? 1 : (side == 3 ? -1 : 0), nz = side == 0 ? 1 : (side == 2 ? -1 : 0);   // the outward normal
        const double lx = W.cx + nx * W.hw, lz = W.cz + nz * W.hd;
        const double dh = along ? 0 : PI / 2, ax = along ? 1 : 0, az = along ? 0 : -1;   // the wall's own axis in the building's frame
        const double leanS = side == W.door ? 0.0 : lean;   // T-02: the doorway's side upright (a leaning jamb's colliders closed the doorway)
        const double tiltOut = (side == 0 || side == 1) ? -leanS : leanS;   // a box's own +along points out on sides 0 and 1, in on 2 and 3
        const bool window = W.window && side == (W.door + 2) % 4 && h > 2.4 && Dm < 0.55;
        const bool glyphs = W.glyphs && g.lod == 0 && (side == 0 || glyphAll) && h > 1.5;
        const double pos = W.doorPos;
        // a run of this side's wall from ya to yb over the base, set `off` outward (its length grown or cut by as much, to meet its
        // neighbours set as far), the doorway and the window left in it where `openings`
        auto run = [&](double ya, double yb, double off, bool openings) {
            if (yb - ya < 0.05) return;
            const double Ls = L + off, ox = lx + nx * off, oz = lz + nz * off;
            auto B = [&](double t0, double t1, double y0r, double y1r, int part) -> Form& {
                const double sIn = std::tan(leanS) * y0r;   // a piece over the foot of a leaning wall sits where the wall is at its height
                Form& e = g.box(ox + ax * (t0 + t1) * 0.5 - nx * sIn, oz + az * (t0 + t1) * 0.5 - nz * sIn, dh, (t1 - t0) * 0.5, ht, base + y0r, base + y1r, W.slot, W.mat, part);
                e.tilt = tiltOut; return e;
            };
            if (!openings || g.lod == 1 || (side != W.door && !window)) { Form& e = B(-Ls, Ls, ya, yb, W.part); e.glyphs = glyphs; e.pattern = W.pattern; rag(e, h); return; }
            if (side == W.door) {   // the doorway: a gap of 1.8 m, a lintel over it where the jambs stand over 2.4 m; A-04: its head 2.1 m over the
                const double gap = 0.9, dt = 2.1 + std::max(0.0, -base);   // ground outside however deep the walls are sunk (a buried people's, a desert's sand)
                { Form& e = B(-Ls, pos - gap, ya, yb, W.part); e.glyphs = glyphs; e.pattern = W.pattern; rag(e, h); }
                { Form& e = B(pos + gap, Ls, ya, yb, W.part); e.pattern = W.pattern; rag(e, h); }
                if (yb > dt + 0.3) B(pos - gap - 0.3, pos + gap + 0.3, dt, std::min(yb, dt + 0.5), 4);
                if (yb > dt + 1.1) { Form& e = B(pos - gap - 0.3, pos + gap + 0.3, dt + 0.5, yb, W.part); e.pattern = W.pattern; rag(e, h); }
            } else {   // C-01's window: a sill under it and the wall over it
                const double gap = 0.55, wp = (L - 1.2) * (g.U(21) - 0.5);
                B(-Ls, wp - gap, ya, yb, W.part); B(wp + gap, Ls, ya, yb, W.part);
                B(wp - gap, wp + gap, ya, std::min(yb, 1.0), W.part);
                if (yb > 1.9) B(wp - gap, wp + gap, 1.9, yb, W.part);
            }
        };
        if (tiered) { run(0, std::min(h, tierY), 0, true); if (h > tierY + 0.3) run(tierY, h, jetty, false); }
        else run(0, h, 0, true);
        if (plinth && g.lod == 0) {   // a stepped people's low wall on a plinth a little wider (no higher than a step: no collider)
            const double ph = 0.85;
            auto P = [&](double t0, double t1) { if (t1 - t0 > 0.05) g.box(lx + ax * (t0 + t1) * 0.5, lz + az * (t0 + t1) * 0.5, dh, (t1 - t0) * 0.5, ht + 0.22, base, base + ph, W.slot, W.mat, 4); };
            if (side == W.door) { P(-L - 0.22, pos - 1.0); P(pos + 1.0, L + 0.22); } else P(-L - 0.22, L + 0.22);
        }
        // the dress: bands of the accent, a cornice, an expressed frame, crenels (each set off the wall by its own part: no collider)
        if (!W.dress) continue;
        const bool standing = h >= W.h * 0.97;
        const double topOff = tiered && h > tierY + 0.3 ? jetty : 0.0, Lt = L + topOff, tx = lx + nx * topOff, tz = lz + nz * topOff;
        const double topIn = std::tan(lean) * h;
        auto band = [&](double y0b, double y1b, double off, double proud, int slot, int mat) {   // a course along the side at a height
            const double sIn = std::tan(lean) * y0b;
            Form& e = g.box(lx + nx * (off - sIn), lz + nz * (off - sIn), dh, L + off + proud, ht + proud, base + y0b, base + y1b, slot, mat, 4); e.tilt = tiltOut;
        };
        if (g.lod <= 1 && g.banded()) {
            const double bw = g.accentOn(AP_BANDS) ? 0.45 : 0.28;
            if (standing) { const double y = h - 0.1 - bw; Form& e = g.box(tx - nx * topIn, tz - nz * topIn, dh, Lt + 0.07, ht + 0.07, base + y, base + y + bw, 1, W.mat, 4); e.tilt = tiltOut; }
            if (tiered && h > tierY + 0.3) band(tierY - bw, tierY, 0, 0.07, 1, W.mat);
            else if (g.accentOn(AP_BANDS) && h > 2.8) band(std::min(h, W.h) * 0.48, std::min(h, W.h) * 0.48 + bw, 0, 0.07, 1, W.mat);
        }
        if (g.lod <= 1 && g.carved() && standing) { Form& e = g.box(tx - nx * topIn, tz - nz * topIn, dh, Lt + 0.24, ht + 0.24, base + h - 0.05, base + h + 0.35, g.accentOn(AP_TOPS) ? 1 : W.slot, W.mat, 4); e.tilt = tiltOut; }
        if (g.gv(GV_EXPRESSION) == EX_FRAME && g.lod == 0 && (g.c.age == AGE_INDUSTRIAL || g.c.age == AGE_MODERN)) {
            // B-415: an industrial people's frame its brick piers between the windows (its iron posts stood off its walls like a ruined
            // pergola), a modern people's its concrete grid: the columns between the windows and a floor's slab at every storey. Each
            // piece of the wall lays its windows from its own end, so the piers stand at its bays' edges piece by piece
            const bool modern = g.c.age == AGE_MODERN;
            const int fm = modern ? MT_CONCRETE : W.mat, fs = W.slot;
            const double ragged = tall && h < W.h * 0.98 ? clampd(3.0 / std::max(h, 1.0), 0.08, 0.35) : 0.0;   // under a ruined wall's ragged top (they stood over it like merlons)
            const double Df = g.D(fm), fh = std::min(h * (1 - ragged), tiered ? tierY : h);
            const WindowSpec* ws = W.pattern > 0 && W.pattern <= g.c.palette.nWindows ? &g.c.palette.windows[W.pattern - 1] : nullptr;
            const double bay = ws ? std::max(2.0, ws->bay) : 3.0, storey = ws ? std::max(2.6, ws->storey) : 3.2;
            if (Df < 0.6 && fh > 1.0) {
                auto piers = [&](double t0, double t1) {   // at the piece's bays' edges and its far end
                    for (int k = 0;; k++) {
                        const bool last = t0 + k * bay > t1 - 0.4;
                        const double t = last ? t1 : t0 + k * bay;
                        if (!(side == W.door && std::fabs(t - pos) < 1.1)) { Form& e = g.box(lx + ax * t + nx * 0.07, lz + az * t + nz * 0.07, dh, modern ? 0.2 : 0.18, ht + (modern ? 0.08 : 0.12), base, base + fh, fs, fm, 4); e.tilt = tiltOut; }
                        if (last) break;
                    }
                };
                if (side == W.door) { piers(-L, pos - 0.9); piers(pos + 0.9, L); } else piers(-L, L);
                if (modern) for (double y = storey; y < fh - 0.6; y += storey) band(y - 0.35, y, 0.04, 0.06, fs, fm);   // the floors' slabs
            }
        } else if (g.gv(GV_EXPRESSION) == EX_FRAME && g.lod == 0) {   // posts at the corners and every two and a half metres, a beam along the top
            const int fm = g.c.age <= AGE_MEDIEVAL ? MT_TIMBER : MT_COMPOSITE;
            const double Df = g.D(fm);
            if (Df < 0.5) {
                const int fs = fm == MT_TIMBER || fm == MT_IRON ? g.timberSlot() : (fm == MT_COMPOSITE ? 1 : W.slot);
                const double fh = std::min(h, tiered ? tierY : h);
                const int n = std::max(1, (int)std::lround(2 * L / 2.5));
                for (int k = 0; k <= n; k++) {
                    double t = -L + 2 * L * k / n;
                    if (side == W.door && std::fabs(t - pos) < 1.25) continue;
                    if (window && std::fabs(t - (L - 1.2) * (g.U(21) - 0.5)) < 0.8) continue;
                    Form& e = g.box(lx + ax * t + nx * 0.06, lz + az * t + nz * 0.06, dh, 0.13, ht + 0.06, base, base + fh, fs, fm, 4); e.tilt = tiltOut;
                }
                if (fh > 2.9) band(std::min(fh, 3.1) - 0.25, std::min(fh, 3.1), 0.06, 0.0, fs, fm);
            }
        }
        if (W.parapet > 0 && standing && g.lod <= 1) { Form& e = g.box(tx - nx * topIn, tz - nz * topIn, dh, Lt, ht, base + h - 0.02, base + h + W.parapet, W.slot, W.mat, 4); e.tilt = tiltOut; }   // B-415
        if (W.crenels && standing && g.lod == 0) {   // merlons along the top
            const int n = std::max(2, (int)(2 * Lt / 1.6));
            for (int k = 0; k < n; k++) {
                double t = -Lt + (k + 0.5) * 2 * Lt / n;
                Form& e = g.box(tx + ax * t - nx * topIn, tz + az * t - nz * topIn, dh, Lt / n * 0.5, ht, base + h, base + h + 0.8 + (g.gv(GV_MOTIF) == MO_STEPS && k % 2 ? 0.4 : 0), g.topSlot(W.mat), W.mat);
                e.tilt = tiltOut;
            }
        }
    }
    if (W.dress && W.door >= 0 && g.lod == 0 && (g.accentOn(AP_DOORS) || g.carved())) {   // the door's surround: its jambs and its head (no collider: a trim)
        const bool along = W.door == 0 || W.door == 2;
        const double nx = W.door == 1 ? 1 : (W.door == 3 ? -1 : 0), nz = W.door == 0 ? 1 : (W.door == 2 ? -1 : 0);
        const double lx = W.cx + nx * W.hw, lz = W.cz + nz * W.hd, dh = along ? 0 : PI / 2, ax = along ? 1 : 0, az = along ? 0 : -1;
        if (hs[W.door] > 2.7) {
            const int ds = g.doorSlot();
            for (int s = -1; s <= 1; s += 2) g.box(lx + ax * (W.doorPos + s * 1.04) + nx * 0.05, lz + az * (W.doorPos + s * 1.04) + nz * 0.05, dh, 0.14, ht + 0.08, base, base + 2.3, ds, W.mat, 4);
            g.box(lx + ax * W.doorPos + nx * 0.08, lz + az * W.doorPos + nz * 0.08, dh, 1.25, ht + 0.12, base + 2.22, base + 2.62, ds, W.mat, 4);
        }
    }
    if (W.dress && prof == WP_BULGING) {   // rounded buttresses at the corners, over the walls' tops where they stand
        for (int q = 0; q < 4; q++) {
            const int sa = q < 2 ? 0 : 2, sb = q % 2 ? 1 : 3;
            const double hb = std::min(hs[sa], hs[sb]);
            const double x = W.cx + (q % 2 ? 1 : -1) * W.hw, z = W.cz + (q < 2 ? 1 : -1) * W.hd;
            Form& e = g.lathe(1, x, z, ht + 0.32, base, base + hb + (hb >= W.h * 0.97 ? 0.7 : 0), W.slot, W.mat, g.lod == 0 ? 0 : 6); e.p[0] = 0.55;
        }
    }
    R.top = base;
    return R;
}
}   // namespace
double walls4(Gen& g, const Walls& W, double hs[4]) { return walls4x(g, W, hs).hmin; }
double ringWalls(Gen& g, const Ring& R0) {
    Ring R = R0;
    if (R.slot == 0 && g.painted) R.slot = 1;   // R-412: a house painted in the accent
    const double Dm = g.D(R.mat);
    const int prof = R.profile >= 0 ? R.profile : (R.dress ? g.gv(GV_PROFILE) : WP_UPRIGHT);
    double thick = R.thick;
    if (R.dress && g.gv(GV_EXPRESSION) == EX_MASSIVE) thick *= 1.35;
    if (prof == WP_BATTERED) thick = std::max(thick * 1.3, std::min(2.0, 0.12 * R.h));
    else if (prof == WP_BULGING) thick *= 1.6;
    const double ht = thick * 0.5, base = g.sink + R.y0;
    if (g.lod == 2) {
        Form& e = g.add(FK_LATHE, 0, R.cx, R.cz, 0, R.rx, R.rz, base, base + R.h * (R.whole ? 1 : wallShare(Dm, 0.5)), R.slot, R.mat, R.part);
        e.sides = R.segs <= 8 ? R.segs : 0; e.pattern = R.pattern;
        return R.whole ? R.h : R.h * wallShare(Dm, 0.5);
    }
    const bool tiered = (prof == WP_LEANING || prof == WP_STEPPED) && R.h > 4.5;
    const double tierY = tiered ? clampd(R.h * 0.5, 2.9, 4.2) : 1e9, jetty = prof == WP_LEANING ? 0.5 : -0.45;
    const double lean = prof == WP_BATTERED ? std::atan(std::min(0.1 * R.h, 1.25 * ht) / std::max(1.0, R.h)) : (prof == WP_BULGING ? 0.05 : (prof == WP_LEANING && !tiered ? -0.06 : 0.0));
    const int glyphAll = R.dress && (g.carved() || g.gv(GV_MOTIF) == MO_GLYPHS);
    double hmin = 1e9;
    const int N = R.segs;
    for (int k = 0; k < N; k++) {
        if (ringInDoor(R, k) && g.lod == 0) continue;
        const double a0 = k * TAU / N, a1 = (k + 1) * TAU / N;
        const double x0 = std::sin(a0) * R.rx, z0 = std::cos(a0) * R.rz, x1 = std::sin(a1) * R.rx, z1 = std::cos(a1) * R.rz;
        const double dx = x1 - x0, dz = z1 - z0, chord = std::sqrt(dx * dx + dz * dz), ux = dx / chord, uz = dz / chord;
        const double nx = -uz, nz = ux;   // the chord's normal; outward for a ring laid clockwise
        const double dh = std::atan2(-dz, dx);   // the box's across axis along the chord, its +along (nx, nz) outward
        const double mx = R.cx + 0.5 * (x0 + x1), mz = R.cz + 0.5 * (z0 + z1);
        const double h = R.whole ? R.h : std::max(0.35, R.h * wallShare(Dm, g.U(R.k0 + k)));
        hmin = std::min(hmin, h);
        const double rMid = std::sqrt(mx * mx + mz * mz) + 1e-6;
        // each segment overlaps its neighbours at the joints, but not into the doorway's gap (its width is the chord's)
        const double ovA = g.lod == 0 && ringInDoor(R, (k + N - 1) % N) ? 0 : ht * 0.6, ovB = g.lod == 0 && ringInDoor(R, (k + 1) % N) ? 0 : ht * 0.6, shift = 0.5 * (ovB - ovA);
        auto seg = [&](double ya, double yb, double off, int part) -> Form& {
            const double sIn = std::tan(lean) * ya, grow = 1 + off / rMid;
            Form& e = g.box(mx + ux * shift + nx * (off - sIn), mz + uz * shift + nz * (off - sIn), dh, (chord * 0.5 + 0.5 * (ovA + ovB)) * grow, ht, base + ya, base + yb, R.slot, R.mat, part);
            e.tilt = -lean; e.pattern = R.pattern; return e;
        };
        { Form& e = seg(0, std::min(h, tierY), 0, R.part); e.glyphs = g.lod == 0 && glyphAll && h > 1.5 && k % 3 == 0; }
        if (tiered && h > tierY + 0.3) seg(tierY, h, jetty, R.part);
        if (!R.dress || g.lod > 1) continue;
        const bool standing = h >= R.h * 0.97;
        const double topOff = tiered && h > tierY + 0.3 ? jetty : 0.0;
        if (standing && g.banded()) { const double bw = g.accentOn(AP_BANDS) ? 0.45 : 0.28; Form& e = seg(h - 0.1 - bw, h - 0.1, topOff + 0.07, 4); e.hz = ht + 0.07; e.slot = 1; e.pattern = 0; }
        if (standing && g.carved()) { Form& e = seg(h - 0.05, h + 0.35, topOff + 0.2, 4); e.hz = ht + 0.2; e.slot = g.accentOn(AP_TOPS) ? 1 : R.slot; e.pattern = 0; }
        if (standing && R.crenels && g.lod == 0 && k % 2 == 0) { Form& e = seg(h, h + 0.8, topOff, 0); e.hx *= 0.55; e.pattern = 0; e.slot = g.topSlot(R.mat); }
        if (g.gv(GV_EXPRESSION) == EX_FRAME && g.lod == 0 && !ringInDoor(R, k) && !ringInDoor(R, (k + N - 1) % N)) {   // a post at the segment's joint
            const int fm = g.c.age <= AGE_MEDIEVAL ? MT_TIMBER : (g.c.age == AGE_INDUSTRIAL ? R.mat : (g.c.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE));   // B-415: a brick pier
            if (g.D(fm) < 0.5) { Form& e = g.lathe(0, R.cx + x0 + std::sin(a0) * 0.08, R.cz + z0 + std::cos(a0) * 0.08, ht + 0.1, base, base + std::min(h, tierY), fm == MT_TIMBER || fm == MT_IRON ? g.timberSlot() : (fm == MT_COMPOSITE ? 1 : R.slot), fm, 6); e.part = 4; }
        }
    }
    return hmin;
}

// a roof over walls that all stand to at least `hmin` of their `h` (a roof never floats: none where a wall fell under 92%), the people's
// `RoofForm`: a flat slab (C-01's stone roof: whole on a sound building, half on a worn one), a gable along the longer side, hipped, a
// dome over the room, a tall pyramid (conical), a stone barrel vault, a saddle's shell, a stepped crown, open (a parapet round the
// sky); of `mat` (stone or the roofing's timber, tiles, slate, sheet), whole, then holed, then gone
double roof(Gen& g, int form, double cx, double cz, double hw, double hd, double hmin, double h, double pitch, int mat) {   // returns the crown's rise over the walls (0: none)
    if (hmin < 0.92 * h) return 0;
    const int slot = g.roofSlot(mat);
    const double Dr = g.D(mat), y0 = g.sink + hmin - 0.05;
    const double eave = g.eave(), ps = pitch * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * g.pitchK();   // a slender people's roofs steeper; R-412: its pitch, its eaves
    if (form == RF_FLAT || form == RF_OPEN) {
        if (form == RF_OPEN) {   // a parapet round an open top
            if (g.lod == 0 && Dr < 0.5) for (int s = 0; s < 4; s++) { bool al = s % 2 == 0; g.box(cx + (s == 1 ? hw : (s == 3 ? -hw : 0)), cz + (s == 0 ? hd : (s == 2 ? -hd : 0)), al ? 0 : PI / 2, al ? hw : hd, 0.18, y0, y0 + 0.7, structural(mat) ? 0 : slot, mat, 4); }
            return 0;
        }
        if (structural(mat)) {   // R-412: a modern people's slab flush, a lip, deep, or a cornice
            const double se = g.slabEave(), th = g.c.age == AGE_MODERN && g.c.genome.of(GV_EAVES) == EV_FLARED ? 0.8 : 0.35;
            if (Dr < 0.35) g.box(cx, cz, 0, hw + se, hd + se, y0, y0 + th, slot, mat, 2);
            else if (Dr < 0.6) g.box(cx, cz - hd * 0.5, 0, hw + se, hd * 0.5 + se, y0, y0 + th, slot, mat, 2);
        } else if (Dr < 0.4) { Form& e = g.box(cx, cz, 0, hw + 0.3, hd + 0.3, y0, y0 + 0.3, slot, mat, 2); e.broken = Dr < 0.2 ? 0 : 0.4; }
        return 0;
    }
    if (Dr >= 0.55) return 0;
    const double broken = Dr < 0.25 ? 0 : clampd((Dr - 0.25) / 0.3, 0.2, 0.9);
    double top = y0;
    switch (form) {
        case RF_PITCHED: {   // a gable: a section swept along the longer side, the eaves over the walls
            const bool alongZ = hd >= hw; const double span = (alongZ ? hw : hd) + eave, len = (alongZ ? hd : hw) + eave * 0.6;
            Form& e = g.add(FK_SWEEP, 1, cx, cz, alongZ ? 0 : PI / 2, 0, len, y0, y0 + span * ps, slot, mat, 2);
            e.p[1] = span; e.p[2] = 0.12; e.broken = broken; top = e.y1;
            if (g.flared() && g.lod < 2) { Form& k = g.add(FK_SWEEP, 1, cx, cz, alongZ ? 0 : PI / 2, 0, len, y0 - 0.3, y0 + 0.3, slot, mat, 2); k.p[1] = span + 0.5; k.p[2] = std::max(0.2, span - 0.3); k.broken = broken; }   // R-412: its bellcast eaves
            break;
        }
        case RF_HIPPED: case RF_CONICAL: {
            const bool hip = form == RF_HIPPED;
            Form& e = g.add(FK_PYRAMID, 0, cx, cz, 0, hw + eave, hd + eave, y0, y0 + (hip ? std::min(hw, hd) : std::max(hw, hd) * 1.9) * ps, slot, mat, 2); e.p[1] = hip ? 0.28 : 0; e.broken = broken; top = e.y1;
            if (g.flared() && g.lod < 2) { Form& k = g.add(FK_PYRAMID, 0, cx, cz, 0, hw + eave + 0.5, hd + eave + 0.5, y0 - 0.3, y0 + 0.3, slot, mat, 2); k.p[1] = 0.72; k.broken = broken; }   // R-412: its bellcast eaves
            break;
        }
        case RF_VAULTED: {   // a barrel vault along the longer side, springing from the walls' top
            const bool alongZ = hd >= hw; const double span = (alongZ ? hw : hd) + 0.25, len = alongZ ? hd : hw;
            Form& e = g.add(FK_VAULT, g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 1 : 0, cx, cz, alongZ ? 0 : PI / 2, span, len + 0.25, y0, y0 + span * 0.85, slot, mat, 2);
            e.p[0] = 0.5; e.p[1] = 1; e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9); top = e.y1;
            break;
        }
        case RF_DOMED: {   // a dome on the room, a flat roof round it where the room is not square
            const double r = std::min(hw, hd) + 0.2;
            if (std::fabs(hw - hd) > 0.6 && structural(mat)) g.box(cx, cz, 0, hw + 0.2, hd + 0.2, y0, y0 + 0.3, structural(mat) ? 0 : slot, mat, 2);
            const int prof = ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : (g.c.age >= AGE_INDUSTRIAL ? 1 : 0)));
            Form& e = g.add(FK_DOME, prof, cx, cz, 0, r, r, y0 + 0.25, y0 + 0.25 + r * (prof == 1 ? 0.55 : (prof == 0 ? 1.0 : 1.35)) * (0.85 + 0.3 * g.gf(GV_PROPORTION)), slot, mat, 2);
            e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9); top = e.y1;
            break;
        }
        case RF_SADDLE: {   // a saddle's shell over the room, its high corners on the front and the back
            Form& e = g.add(FK_SHELL, 0, cx, cz, 0, hw + eave, hd + eave, y0, y0 + std::max(hw, hd) * ps * 0.9, slot, mat, 2);
            e.broken = broken; top = e.y1;
            break;
        }
        case RF_STEPPED: { Form& e = g.add(FK_PYRAMID, 0, cx, cz, 0, hw + 0.25, hd + 0.25, y0, y0 + std::min(hw, hd) * ps * 1.1, slot, mat, 2); e.p[0] = 3; e.p[1] = 0.3; e.p[2] = 1; e.broken = broken; top = e.y1; break; }
        default: { Form& e = g.lathe(1, cx, cz, hw + eave, y0, y0 + hw * ps * 1.3, slot, mat); e.p[0] = 0.04; e.broken = broken; top = e.y1; break; }   // a cone (the round roofs' and A-03's form 4)
    }
    return top - (g.sink + hmin);
}
// B-415: a pitched roof's gable ends in its walls' own material (stone, brick, adobe): each stands on its wall while the wall stands whole,
// whatever became of the roof, under the roof's slope (a roofless row keeps its gables, a stone house its ends: the age's roofs on a ruin);
// `W` the walls under a roof of `pitch` whose ridge runs along their longer side (across it where `across`: a row's gables to the street)
// R-412: the gable's end as the people shaped it (a medieval or an industrial people's): plain, its crow steps climbing over the roof's
// slope, a curved neck rising over the ridge under its cap, a pediment's cornice across its foot and its raking frame, or its top clipped
// by a little hip of the roofing
void gables(Gen& g, const Walls& W, const double hs[4], double pitch, bool across) {
    if (g.lod > 1 || g.c.age < AGE_MEDIEVAL || !structural(W.mat)) return;
    const bool alongZ = (W.hd >= W.hw) != across;   // the ridge's run
    const double ps = pitch * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * g.pitchK(), half = alongZ ? W.hw : W.hd, ht = W.thick * 0.5;
    const int gb = g.c.age <= AGE_INDUSTRIAL ? g.gv(GV_GABLES) : GB_PLAIN, slot = W.slot == 0 && g.painted ? 1 : W.slot;
    const double dh = alongZ ? 0 : PI / 2, rise = half * ps;
    for (int s = 0; s < 4; s++) {
        if ((s == 0 || s == 2) != alongZ || hs[s] < 0.97 * W.h) continue;
        const double nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0), y0 = g.sink + W.y0 + hs[s] - 0.03;
        const double gx = W.cx + nx * W.hw, gz = W.cz + nz * W.hd;
        if (gb == GB_STEPPED && g.lod == 0 && rise > 1.0) {   // its crow steps: each a course narrower, over the slope
            const int n = clampi((int)std::lround(rise / 0.75), 3, 6);
            for (int q = 0; q < n; q++) g.add(FK_BOX, 0, gx, gz, dh, (half + ht) * (1 - (double)q / n) - 0.04, ht, y0 + rise * q / n, y0 + rise * (q + 1) / n + 0.32, slot, W.mat);
            continue;
        }
        const bool clipped = gb == GB_HALF_HIPPED && rise > 1.2;
        Form& e = g.add(FK_SWEEP, 1, gx, gz, dh, 0, ht, y0, y0 + rise * (clipped ? 0.66 : 1.0), slot, W.mat);
        e.p[1] = half + ht; e.p[2] = clipped ? std::max(0.03, (half + ht) * 0.34) : 0.03;
        if (g.lod != 0) continue;
        if (gb == GB_CURVED && rise > 1.2) {   // its neck over the ridge, its cap
            const double nw = std::max(0.5, half * 0.3);
            g.add(FK_BOX, 0, gx, gz, dh, nw, ht, y0 + rise * 0.5, y0 + rise + 0.6, slot, W.mat);
            g.add(FK_DOME, 1, gx, gz, dh, nw, ht + 0.02, y0 + rise + 0.6, y0 + rise + 0.6 + nw * 0.7, slot, W.mat, 4);
            for (int k = -1; k <= 1; k += 2) { Form& sc = g.add(FK_DOME, 0, gx + (alongZ ? k * (nw + 0.2) : 0), gz + (alongZ ? 0 : k * (nw + 0.2)), dh, 0.35, ht + 0.02, y0 + rise * 0.5, y0 + rise * 0.5 + 0.35, slot, W.mat, 4); (void)sc; }   // its scrolls
        } else if (gb == GB_PEDIMENT) {   // its cornice across its foot and its raking frame
            { Form& c = g.add(FK_BOX, 0, gx + nx * 0.06, gz + nz * 0.06, dh, half + ht + 0.22, ht + 0.12, y0 - 0.22, y0 + 0.08, slot, W.mat, 4); c.tone = g.tone + 6; }
            { Form& f = g.add(FK_SWEEP, 1, gx + nx * 0.05, gz + nz * 0.05, dh, 0, ht + 0.06, y0 + 0.08, y0 + 0.08 + (half + 0.22) * ps + 0.22, slot, W.mat, 4); f.p[1] = half + ht + 0.22; f.p[2] = 0.05; f.tone = g.tone + 6; }
        } else if (clipped && g.D(MT_TIMBER) < 0.4) {   // the little hip over its clipped top
            Form& h = g.add(FK_PYRAMID, 0, gx + nx * 0.3, gz + nz * 0.3, dh, (half + ht) * 0.36, 0.75, y0 + rise * 0.64, y0 + rise * 1.0, g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
            h.p[1] = 0.05;
        }
    }
}
// a round roof over a ring (`rx` across, `rz` along): a cone (pitched, hipped, conical), a dome (domed, vaulted), a disc (flat), a
// saddle's shell, a stepped cone of `segs` sides, nothing (open)
double roundRoof(Gen& g, int form, double cx, double cz, double rx, double rz, int segs, double hmin, double h, double pitch, int mat) {   // returns the crown's rise (0: none)
    if (hmin < 0.92 * h) return 0;
    const int slot = g.roofSlot(mat);
    const double Dr = g.D(mat), y0 = g.sink + hmin - 0.1, ps = pitch * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * g.pitchK(), r = std::max(rx, rz), ev = g.eave() + 0.05;   // R-412
    if (form == RF_OPEN) return 0;
    if (form == RF_FLAT) { if (Dr < 0.45) { Form& e = g.add(FK_LATHE, 0, cx, cz, 0, rx + g.slabEave(), rz + g.slabEave(), y0, y0 + 0.35, slot, mat, 2); e.sides = segs <= 8 ? segs : 0; e.broken = Dr < 0.25 ? 0 : 0.5; return 0.25; } return 0; }
    if (Dr >= 0.6) return 0;
    const double broken = Dr < 0.25 ? 0 : clampd((Dr - 0.25) / 0.35, 0.2, 0.9);
    if (form == RF_DOMED || form == RF_VAULTED) {
        const int prof = ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : (g.c.age >= AGE_MODERN ? 1 : 0)));
        Form& e = g.add(FK_DOME, prof, cx, cz, 0, rx + 0.1, rz + 0.1, y0, y0 + r * (prof == 1 ? 0.55 : (prof == 0 ? 1.0 : 1.35)) * (0.85 + 0.3 * g.gf(GV_PROPORTION)), slot, mat, 2);
        e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9);
        return e.y1 - (g.sink + hmin);
    }
    if (form == RF_SADDLE) { Form& e = g.add(FK_SHELL, 0, cx, cz, 0, rx + ev - 0.1, rz + ev - 0.1, y0, y0 + r * ps * 0.9, slot, mat, 2); e.broken = broken; return e.y1 - (g.sink + hmin); }
    if (form == RF_STEPPED) { Form& e = g.add(FK_PYRAMID, 0, cx, cz, 0, rx + 0.3, rz + 0.3, y0, y0 + r * ps * 1.2, slot, mat, 2); e.sides = clampi(segs, 3, 8); e.p[0] = 3; e.p[1] = 0.25; e.p[2] = 1; e.broken = broken; return e.y1 - (g.sink + hmin); }
    Form& e = g.add(FK_LATHE, 1, cx, cz, 0, rx + ev, rz + ev, y0, y0 + r * ps * (form == RF_CONICAL ? 1.9 : 1.3), slot, mat, 2);
    e.sides = segs <= 8 && form != RF_PITCHED ? segs : 0; e.p[0] = 0.04; e.broken = broken;
    if (g.flared() && g.lod < 2) { Form& k = g.add(FK_LATHE, 1, cx, cz, 0, rx + ev + 0.5, rz + ev + 0.5, y0 - 0.3, y0 + 0.3, slot, mat, 2); k.sides = e.sides; k.p[0] = 0.72; k.broken = broken; }   // R-412: its bellcast eaves
    return e.y1 - (g.sink + hmin);
}
TowerOut towerShaft(Gen& g, double tx, double tz, double r, double H, int mat, int k, int pattern, bool fallTop) {
    TowerOut T;
    const double Dm = g.D(mat), h = H * tallShare(Dm, g.U(k));
    T.h = h; T.whole = h >= H * 0.97;
    const int plan = g.gv(GV_PLAN), prof = g.gv(GV_PROFILE);
    const int sides = plan == PF_POLYGON ? clampi(g.c.genome.number(), 3, 8) : (plan == PF_CIRCLE || plan == PF_ELLIPSE ? 0 : 4);
    const double rz = plan == PF_ELLIPSE ? r * 0.75 : r;
    auto shaft = [&](int sub, double ra, double y0, double y1, double p0, double p1) -> Form& {
        Form& e = g.add(FK_LATHE, sub, tx, tz, 0, ra, ra * rz / r, g.sink + y0, g.sink + y1, 0, mat); e.sides = sides; e.p[0] = p0; e.p[1] = p1; e.pattern = pattern; return e;
    };
    const double brk = T.whole ? 0 : 0.3;
    switch (prof) {
        case WP_BATTERED: { Form& e = shaft(1, r * 1.12, 0, h, 1 - 0.3 * h / H, 0); e.broken = brk; T.rTop = r * 1.12 * (1 - 0.3 * h / H); break; }
        case WP_BULGING: { Form& e = shaft(3, r, 0, h, 1 + 0.14 * std::min(1.0, h / H), 0.5 * H / std::max(h, 1.0)); e.broken = brk; T.rTop = r; break; }
        case WP_LEANING: { Form& e = shaft(1, r * 0.9, 0, h, 1 + 0.28 * h / H, 0); e.broken = brk; T.rTop = r * 0.9 * (1 + 0.28 * h / H); break; }
        case WP_STEPPED: {   // tiers, each set back
            const int tiers = 3; double ra = r;
            for (int i = 0; i < tiers; i++) {
                const double ya = H * i / tiers, yb = H * (i + 1) / tiers;
                if (h <= ya + 0.2) break;
                Form& e = shaft(0, ra, ya, std::min(h, yb), 0, 0); e.broken = h < yb ? brk : 0; T.rTop = ra;
                ra *= 0.8;
            }
            break;
        }
        default: { Form& e = shaft(0, r, 0, h, 0, 0); e.broken = brk; T.rTop = r; break; }
    }
    if (!T.whole && fallTop && g.lod == 0) {
        const double dir = std::atan2(tx, tz + 0.01) + g.U(k + 1) - 0.5;
        g.fallen(FK_LATHE, 0, tx, tz, dir, r * 0.8, (H - h) * 0.45, 0, mat, sides);
        g.heap(tx, tz, r * 1.7, r * 1.7, std::min(4.0, (H - h) * 0.07), 0, mat);
    }
    return T;
}
// a tower's top where its shaft stands whole: a spire (`spire`, or the people's pitched and conical roofs), a dome, crenels, a flat top,
// and the people's tops on it
// B-415: by the age: a modern or a later people's concrete or composite shaft topped with a needle, a saucer or a mast on its flat top; an
// industrial people's tower without crenels, a slate pyramid on it where it raised no spire or dome
void towerTop(Gen& g, double tx, double tz, double r, double h, double H, int mat, bool spire, double spireH) {
    const int roofF = g.c.roof, tp = g.gv(GV_TOPS);
    const int plan = g.gv(GV_PLAN), sides = plan == PF_POLYGON ? clampi(g.c.genome.number(), 3, 8) : (plan == PF_CIRCLE || plan == PF_ELLIPSE ? 0 : 4);
    double yTop = g.sink + h;   // where the tops stand: the spire's tip, the dome's crown, the shaft's top
    int topMat = mat;
    (void)H;
    if (g.c.age >= AGE_MODERN) {
        const double Dm = g.D(mat);
        if (Dm < 0.5) {
            if (spire || roofF == RF_PITCHED || roofF == RF_CONICAL || roofF == RF_HIPPED) { Form& s = g.lathe(2, tx, tz, r * 0.32, yTop, yTop + std::max(spireH * 0.8, r * 5), g.topSlot(mat), mat, sides >= 3 ? sides : 0); s.p[0] = 1.6; yTop += std::max(spireH * 0.8, r * 5); }
            else if (roofF == RF_DOMED || roofF == RF_VAULTED) { g.add(FK_DOME, 1, tx, tz, 0, r * 1.15, r * 1.15, yTop, yTop + r * 0.45, g.topSlot(mat), mat, 2); yTop += r * 0.45; }
            else if (g.lod < 2) { Form& m = g.add(FK_TRUSS, 0, tx, tz, 0, std::max(0.6, r * 0.18), std::max(0.6, r * 0.18), yTop, yTop + std::max(6.0, r * 3), g.ironSlot(), MT_IRON); m.sides = 3; m.p[0] = 0.3; m.broken = g.D(MT_IRON) < 0.3 ? 0 : 0.6; }
        }
        topsAt(g, {{tx, tz}}, yTop, std::max(1.0, r / 2.5), topMat);
        return;
    }
    if (spire || roofF == RF_PITCHED || roofF == RF_CONICAL || roofF == RF_HIPPED) {
        const int sm = g.c.age <= AGE_INDUSTRIAL && !structural(roofMaterial(g, RF_PITCHED, mat)) ? MT_TIMBER : mat;
        const double Ds = g.D(sm), sH = spire ? spireH : r * 2.6;
        if (Ds < 0.5) { Form& s = g.lathe(2, tx, tz, r * 1.04, g.sink + h, g.sink + h + sH, g.topSlot(sm), sm, sides >= 3 ? std::max(sides, 4) : 8); s.p[0] = spire ? 1.0 : 0.8; s.broken = Ds < 0.25 ? 0 : 0.5; if (Ds < 0.25) { yTop += sH; topMat = sm; } }
        if (g.gv(GV_MOTIF) == MO_SPIRALS && g.lod < 2) { Form& c = g.add(FK_TORUS, 0, tx, tz, 0, r * 1.05, r * 1.05, g.sink + h - 1.0, g.sink + h - 0.4, 1, mat); c.p[0] = 0.3; }
    } else if (roofF == RF_DOMED || roofF == RF_VAULTED) {
        const double Dd = g.D(mat);
        if (Dd < 0.55) { Form& d = g.add(FK_DOME, ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : 0), tx, tz, 0, r, r, g.sink + h, g.sink + h + r * 1.1, g.topSlot(mat), mat, 2); d.broken = Dd < 0.3 ? 0 : 0.5; if (Dd < 0.3) yTop += r * 1.1; }
    } else if (g.c.age == AGE_INDUSTRIAL) {   // a slate pyramid
        const double Dt = g.D(MT_TIMBER);
        if (Dt < 0.5) { Form& p = g.lathe(1, tx, tz, r * 1.08, yTop, yTop + r * 1.5, g.roofSlot(MT_TIMBER), MT_TIMBER, sides >= 3 ? sides : 0); p.p[0] = 0.04; p.broken = Dt < 0.25 ? 0 : 0.5; if (Dt < 0.25) { yTop += r * 1.5; topMat = MT_TIMBER; } }
    } else if (g.lod == 0 && (tp == TP_CRENELS || roofF == RF_STEPPED)) {   // merlons round the top
        const int n = std::max(6, (int)(TAU * r / 1.4));
        for (int k = 0; k < n; k += 2) { double a = k * TAU / n; g.box(tx + std::sin(a) * r * 0.92, tz + std::cos(a) * r * 0.92, a, r * PI / n, 0.3, g.sink + h, g.sink + h + 0.9, g.topSlot(mat), mat); }
    }
    topsAt(g, {{tx, tz}}, yTop, std::max(1.0, r / 2.5), topMat);
}

// A-04: what a building stands on (`Building::found`): stilts (posts under a floor, the walls on it: drawn by the generator, which
// lifts them), a mound, a pit it is sunk in (the generator sinks it), a cut in the rock round it (a rim of the walls' stone, its gap at the
// doorway), a terrace's kerb. `rx`, `rz` the footprint's half extents. B-415: a mound's earth bank and a pit's rim
// are gone: each building stood in a ring of the ground's own grass or sand as wide as its longest side, over its neighbours and the street
// (the user: "round green things around each building [...] they overlap with each other and other buildings a lot"); a people of mounds
// shows it in where it built its towns (`plan.cpp`), a people of pits in its sunk floors
void foundation(Gen& g, double rx, double rz, int doorSide, double doorPos, int mat) {
    if (g.lod != 0) return;
    switch (g.b.found) {
        case FD_CUT: {   // the rock's rim, 1.6 m out, a gap of 2.6 m at the doorway
            const double o = 1.6, H = 1.4 * wallShare(g.D(mat), g.U(600));
            for (int s = 0; s < 4; s++) {
                const bool al = s == 0 || s == 2; const double L = (al ? rx : rz) + o, nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0);
                const double lx = nx * (rx + o), lz = nz * (rz + o), ax = al ? 1 : 0, az = al ? 0 : -1;
                auto P = [&](double t0, double t1) { if (t1 - t0 > 0.2) g.box(lx + ax * (t0 + t1) * 0.5, lz + az * (t0 + t1) * 0.5, al ? 0 : PI / 2, (t1 - t0) * 0.5, 0.45, 0, std::max(0.4, H), 0, mat); };
                if (s == doorSide) { P(-L, doorPos - 1.3); P(doorPos + 1.3, L); } else P(-L, L);
            }
            break;
        }
        case FD_TERRACE: {   // a kerb of stone a step high round it
            const double o = 1.1;
            for (int s = 0; s < 4; s++) {
                const bool al = s == 0 || s == 2; const double L = (al ? rx : rz) + o;
                g.box((s == 1 ? 1 : (s == 3 ? -1 : 0)) * (rx + o), (s == 0 ? 1 : (s == 2 ? -1 : 0)) * (rz + o), al ? 0 : PI / 2, L, 0.3, 0, 0.7, 0, mat, 4);
            }
            break;
        }
        default: break;
    }
}
// stilts under a floor at `fy`: posts at the corners and along the sides (timber, or the age's own), the floor on them while every post
// stands; false when the posts are gone (the house lies on the ground)
bool stilts(Gen& g, double rx, double rz, double fy) {
    const int m = g.c.age >= AGE_INDUSTRIAL ? (g.c.age == AGE_INDUSTRIAL ? MT_IRON : (g.c.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE)) : (g.c.age == AGE_MEDIEVAL && g.c.hut == 1 ? MT_MASONRY : MT_TIMBER);
    const int slot = m == MT_TIMBER || m == MT_IRON ? g.timberSlot() : 0;
    const double D = g.D(m), r = m == MT_TIMBER ? 0.16 : 0.3;
    const bool standing = D < 0.45;
    const int nx = std::max(2, (int)std::lround(rx / 2.2) + 1), nz = std::max(2, (int)std::lround(rz / 2.2) + 1);
    for (int i = 0; i < nx; i++) for (int j = 0; j < nz; j++) {
        if (i != 0 && i != nx - 1 && j != 0 && j != nz - 1) continue;
        const double x = -rx * 0.85 + 1.7 * rx * i / (nx - 1), z = -rz * 0.85 + 1.7 * rz * j / (nz - 1);
        if (m == MT_TIMBER) g.post(x, z, fy, r, 500 + i * 7 + j);
        else { const double hh = standing ? fy : fy * wallShare(D, g.U(500 + i * 7 + j)); g.lathe(0, x, z, r, 0, std::max(0.3, hh), slot, m, 6); }
    }
    if (standing && g.lod < 2) g.box(0, 0, 0, rx + 0.2, rz + 0.2, fy - 0.25, fy, slot, m, 2);
    return standing;
}

// C-08: on a desert the sand drifts against the walls the wind meets (the sides whose outward normal faces within 60 degrees of where
// the wind comes from), a low wedge the age has built up; part 7, drawn in the sand's bank (P-04: a frozen world's snow, in the snow's)
void drifts(Gen& g, double hw, double hd, double D) {
    if (!g.st.drift || g.lod != 0 || g.c.buried < 0.15) return;
    const double hDrift = 0.25 + 0.6 * std::min(1.0, g.c.buried / 1.2) * (0.5 + 0.5 * D);
    for (int side = 0; side < 4; side++) {
        double nAng = g.b.heading + side * PI / 2;   // the side's outward normal: 0 front (+z'), 1 right (+x'), 2 back, 3 left
        if (std::fabs(std::remainder(nAng - g.st.wind, TAU)) > PI / 3) continue;
        bool along = side == 0 || side == 2; double L = along ? hw : hd;
        double o = g.b.thick * 0.5 + 0.95;   // from the wall's outer face outward (clear of the wall's centre line, where the doorway is read)
        double lx = side == 1 ? hw + o : (side == 3 ? -hw - o : 0), lz = side == 0 ? hd + o : (side == 2 ? -hd - o : 0);
        g.box(lx, lz, along ? 0 : PI / 2, L, 0.9, 0, hDrift, 0, -1, 7);
    }
}

// rubble at the foot of a wall that fell (C-01's: three blocks a side, none in the doorway)
void rubble(Gen& g, double hw, double hd, const double hs[4], double h, double doorX, double doorZ, int slot, int mat) {
    if (g.lod != 0) return;
    for (int side = 0; side < 4; side++) {
        if (hs[side] > 0.5 * h) continue;
        bool along = side == 0 || side == 2; double L = along ? hw : hd;
        double lx = side == 1 ? hw : (side == 3 ? -hw : 0), lz = side == 0 ? hd : (side == 2 ? -hd : 0);
        for (int k = 0; k < 3; k++) {
            double u1 = g.U(60 + side * 3 + k), u2 = g.U(80 + side * 3 + k), sz = 0.3 + 0.5 * g.U(100 + side * 3 + k);
            double tt = (u1 - 0.5) * 2 * L, off = 0.8 + 1.5 * u2;
            double px = along ? tt : lx + (side == 1 ? off : -off), pz = along ? lz + (side == 0 ? off : -off) : tt;
            if ((px - doorX) * (px - doorX) + (pz - doorZ) * (pz - doorZ) < (1.9 + sz) * (1.9 + sz)) continue;   // not in the doorway
            g.box(px, pz, u1 * 1.2, sz, sz * 0.8, 0, sz * 1.1, slot, mat, 3);
        }
    }
}
