// E-09: the medieval age's buildings (C-01's kinds, the temple, the wall's towers, the windmill) and C-08's features, with
// the rectangular body the later ages' houses are built on (`rectBody`) and the tops on a roof
#include "buildings_internal.h"

// ---- the medieval age: C-01's kinds, the temple, the wall's towers, the windmill; C-08's features --------------------------------

// the tops a people puts on a rectangular building: finials at its gable's ends or its corners, a mast at one corner, statues at the
// front's corners; those on a roof (a gable's ends, a hipped, conical, domed or stepped roof's crown) of the roof's material and only
// while the roof stands whole (a finial outlives no roof), those at the walls' corners of the walls'
void houseTops(Gen& g, int rf, double cx, double cz, double hw, double hd, double y, double ridge, int roofMat, int wallMat) {
    if (g.lod != 0) return;
    const int tp = g.gv(GV_TOPS);
    if (tp == TP_NOTHING || tp == TP_CRENELS) return;
    std::vector<std::pair<double, double>> pts;
    bool onRoof = true;
    if (rf == RF_PITCHED) { const bool alongZ = hd >= hw; if (alongZ) { pts = {{cx, cz + hd}, {cx, cz - hd}}; } else { pts = {{cx + hw, cz}, {cx - hw, cz}}; } y += ridge; }
    else if (rf == RF_HIPPED || rf == RF_CONICAL || rf == RF_DOMED || rf == RF_STEPPED) { pts = {{cx, cz}}; y += ridge; }
    else {
        onRoof = false;
        if (tp == TP_MASTS) pts = {{cx + hw * 0.7, cz - hd * 0.7}};
        else if (tp == TP_STATUES) pts = {{cx - hw, cz + hd}, {cx + hw, cz + hd}};
        else pts = {{cx - hw, cz + hd}, {cx + hw, cz + hd}, {cx - hw, cz - hd}, {cx + hw, cz - hd}};
    }
    if (onRoof && g.D(roofMat) >= 0.25) return;
    topsAt(g, pts, y, 1.0, onRoof ? roofMat : wallMat);
}
// the tops on a round roof's crown (a cone, a dome, a stepped crown; a flat roof's middle), while the roof stands whole
void roundTops(Gen& g, int rf, double y, double rise, double scale, int roofMat) {
    if (rf == RF_OPEN || g.D(roofMat) >= (rf == RF_FLAT ? 0.35 : 0.25)) return;
    topsAt(g, {{0, 0}}, y + (rf == RF_FLAT ? 0.35 : rise), scale, roofMat);
}

namespace {

// R-412: a futuristic people's pod's dress: what stands out from its ring (a ring gallery round it over its doorway's head, balconies,
// blisters of glass at its foot, oriels up its wall) and its doorway's (a tunnel out from it, a low step, two pylons, a shell over it)
void podDress(Gen& g, double rx, double rz, double h, double hmin, int segs, int door, int doorSegs) {
    if (g.lod > 1 || g.c.age != AGE_FUTURISTIC) return;
    const double Dc = g.D(MT_COMPOSITE);
    if (Dc > 0.6) return;
    const double doorA = (door + 0.5 * doorSegs) * TAU / segs;
    const bool whole = hmin >= 0.92 * h;   // what stands out from its wall only while its wall stands
    // its wall's spots: a round ring's anywhere round it, a polygon's at its sides' middles (out from them), none in its doorway
    std::vector<std::pair<double, double>> wallAt;   // the angle, the wall's distance from the middle there
    const bool round = segs >= 10;
    if (round) { for (int k = 0; k < 4; k++) { const double a = doorA + PI / 4 + k * PI / 2; wallAt.push_back({a, std::hypot(std::sin(a) * rx, std::cos(a) * rz)}); } }
    else for (int k = 0; k < segs; k++) {
        if (((k - door) % segs + segs) % segs < doorSegs) continue;
        wallAt.push_back({(k + 0.5) * TAU / segs, rx * std::cos(PI / segs)});
    }
    auto spot = [&](size_t i, double out, double& x, double& z) { x = std::sin(wallAt[i].first) * (wallAt[i].second + out); z = std::cos(wallAt[i].first) * (wallAt[i].second + out); };
    switch (whole ? g.gv(GV_PROJECTIONS) : PJ_NONE) {
        case PJ_BALCONIES: if (g.lod == 0) for (size_t i = 0; i < wallAt.size(); i++) { double x, z; spot(i, 0.4, x, z); Form& e = g.lathe(0, x, z, 1.3, h * 0.62, h * 0.62 + 0.22, 0, MT_COMPOSITE); e.part = 2; } break;
        case PJ_GALLERIES:
            if (round) { Form& t = g.add(FK_TORUS, 0, 0, 0, 0, std::max(rx, rz) + 0.8, std::max(rx, rz) + 0.8, h - 0.75, h - 0.15, 2, MT_COMPOSITE); t.p[0] = 0.3; }
            else for (size_t i = 0; i < wallAt.size(); i++) { double x, z; spot(i, 0.7, x, z); const double len = rx * std::sin(PI / segs); g.box(x, z, wallAt[i].first + PI / 2, len * 0.9, 0.7, h * 0.62 - 0.2, h * 0.62, 2, MT_COMPOSITE, 2); }
            break;
        case PJ_BAYS: for (size_t i = 0; i < wallAt.size() && i < 3; i++) { double x, z; spot(i, -0.6, x, z); g.add(FK_DOME, 1, x, z, 0, 2.1, 2.1, 0, 1.6, 2, MT_COMPOSITE, 2); } break;
        case PJ_ORIELS: for (size_t i = 0; i < wallAt.size() && i < 3; i++) { double x, z; spot(i, 0.1, x, z); Form& e = g.lathe(0, x, z, 0.95, h * 0.45, h + 0.5, 0, MT_COMPOSITE); e.pattern = 2; } break;
        default: break;
    }
    if (g.lod != 0) return;
    const double dx = std::sin(doorA), dz = std::cos(doorA), r = round ? std::sqrt(rx * rx * dx * dx + rz * rz * dz * dz) : rx * std::cos(PI / segs);
    auto at = [&](double across, double out, double& x, double& z) { x = dx * (r + out) + dz * across; z = dz * (r + out) - dx * across; };
    double x, z;
    switch (g.gv(GV_ENTRANCE)) {
        case EN_PORCH: { at(0, 1.0, x, z); Form& v = g.add(FK_ARCH, 0, x, z, doorA, 1.7, 1.0, 0, 3.1, 2, MT_COMPOSITE); v.p[0] = 1; v.p[1] = 0.32; v.p[2] = 2.1; break; }   // a deep arch out from its doorway
        case EN_STEPS: at(0, 1.0, x, z); g.box(x, z, doorA, 1.4, 1.0, 0, 0.15, 0, MT_COMPOSITE, 4); break;
        case EN_PORTICO: for (int s = -1; s <= 1; s += 2) { at(s * 1.6, 0.8, x, z); g.lathe(1, x, z, 0.3, 0, h + 1.8, 0, MT_COMPOSITE); } break;
        case EN_CANOPY: { at(0, 0.8, x, z); Form& s = g.add(FK_SHELL, 0, x, z, doorA, 1.7, 1.3, 2.7, 3.4, 2, MT_COMPOSITE, 2); (void)s; break; }
        default: break;
    }
}
}   // namespace

// a rectangular body: walls, its roof (split into cells where the people built in cells), its upper storey (a stepped people's), its
// tops; the room's walls `W` already set. Returns the walls' lowest height
double rectBody(Gen& g, Walls W, int rf, int roofMat, double hs[4], bool cells) {
    W.crenels = W.dress && g.gv(GV_TOPS) == TP_CRENELS && (rf == RF_FLAT || rf == RF_OPEN);
    const double hmin = walls4(g, W, hs);
    const bool stepped = W.dress && g.gv(GV_MASSING) == MS_STEPPED && hmin >= 0.97 * W.h && g.lod < 2 && W.hw > 2.2 && W.hd > 2.2;
    const int mainRoof = stepped ? RF_FLAT : rf;
    const double longL = std::max(W.hw, W.hd);
    const int n = cells && W.dress && g.gv(GV_MODULAR) == 1 && longL > 3.4 && mainRoof != RF_FLAT && mainRoof != RF_OPEN ? clampi((int)std::lround(longL / 2.8), 2, 4) : 1;
    double rise = 0;
    if (n == 1) {
        rise = roof(g, mainRoof, W.cx, W.cz, W.hw, W.hd, hmin, W.h, 0.8, mainRoof == RF_FLAT && !stepped ? roofMat : (stepped ? W.mat : roofMat));
        if (mainRoof == RF_PITCHED) gables(g, W, hs, 0.8);
    } else {   // a roof a cell: gables across the long side, a row of them
        const bool alongZ = W.hd >= W.hw;
        const int rl = g.c.age >= AGE_MEDIEVAL && g.c.age <= AGE_INDUSTRIAL && g.lod == 0 && dwellingKind(g.b.kind) ? g.gv(GV_ROOFLINE) : RL_PLAIN;
        for (int k = 0; k < n; k++) {
            const double t = -longL + (k + 0.5) * 2 * longL / n, half = longL / n;
            const double cxk = W.cx + (alongZ ? 0 : t), czk = W.cz + (alongZ ? t : 0);
            const double rk = roof(g, mainRoof, cxk, czk, alongZ ? W.hw : half, alongZ ? half : W.hd, hmin, W.h, 0.8, roofMat);
            // R-412: the cells' roofline: cresting along each cell's ridge, a lantern on the middle one's
            if (rk > 0.8 && g.D(roofMat) < 0.25 && (rl == RL_CRESTED || (rl == RL_LANTERN && k == n / 2))) {
                const double yTop = g.sink + W.y0 + hmin - 0.05 + rk;
                const bool cellZ = (alongZ ? half : W.hd) >= (alongZ ? W.hw : half);   // the cell's own ridge's run
                if (rl == RL_CRESTED) { Form& e = g.add(FK_FENCE, g.c.age == AGE_INDUSTRIAL ? 1 : 0, cxk, czk, cellZ ? 0 : PI / 2, 0.03, std::max(0.4, (cellZ ? (alongZ ? half : W.hd) : (alongZ ? W.hw : half)) - 0.2), yTop - 0.08, yTop + 0.4, g.roofSlot(roofMat), roofMat, 4); e.p[0] = 0.3; e.p[1] = 0.5; e.p[3] = 0.13; }
                else { g.lathe(0, cxk, czk, 0.45, yTop - 0.4, yTop + 0.9, wallsSlot(g, W), W.mat, 6); Form& s2 = g.lathe(1, cxk, czk, 0.58, yTop + 0.9, yTop + 0.9 + 1.2 * g.pitchK(), g.roofSlot(roofMat), roofMat, 6); s2.p[0] = 0.04; }
            }
        }
    }
    if (stepped) {   // the upper storey set back on the walls, its own roof
        Walls U = W; U.hw = W.hw * 0.62; U.hd = W.hd * 0.62; U.h = W.h * 0.75; U.y0 = W.y0 + hmin; U.door = -1; U.window = false; U.glyphs = false; U.whole = false; U.k0 = W.k0 + 40; U.crenels = false;
        double hu[4]; const double hmu = walls4(g, U, hu);
        const double riseU = roof(g, rf, U.cx, U.cz, U.hw, U.hd, hmu, U.h, 0.8, roofMat);
        if (rf == RF_PITCHED) gables(g, U, hu, 0.8);
        houseTops(g, rf, U.cx, U.cz, U.hw, U.hd, g.sink + U.y0 + hmu, riseU, roofMat, W.mat);
    } else if (hmin >= 0.92 * W.h) houseTops(g, n > 1 ? RF_FLAT : rf, W.cx, W.cz, W.hw, W.hd, g.sink + W.y0 + hmin, rise, roofMat, W.mat);
    houseDress(g, W, hs, hmin, mainRoof, rise, mainRoof == RF_FLAT && !stepped ? roofMat : roofMat, !stepped && n == 1);
    return hmin;
}

namespace {

// C-01's house, hall and tower; A-04: in the people's plan (a rectangle, a ring of its polygon, an oval, an L, a court), roof, profile,
// dress and tops, on its foundation
void genKeep(Gen& g);
void genMarketHall(Gen& g);
void genPalace(Gen& g);
}   // namespace
void genHouse(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.kind == BK_TOWER) { genKeep(g); return; }
    if (b.kind == BK_HALL && b.style == 1) { genMarketHall(g); return; }
    if (b.kind == BK_HALL && b.style == 2) { genPalace(g); return; }
    const int mat = b.kind == BK_HOUSE ? houseMaterial(c) : stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (b.found == FD_PIT) g.sink -= std::min(1.2, 0.35 * b.hWall);
    if (g.gv(GV_MASSING) == MS_BURIED) g.sink -= 0.4 * b.hWall;
    double y0 = 0;
    if (b.found == FD_STILTS) y0 = stilts(g, b.hw, b.hd, 2.2) ? 2.2 : 0;
    const int rf = ageRoof(c, houseRoof(g)), roofMat = roofMaterial(g, rf, mat);   // B-415: an industrial people's warehouse under the age's roof
    const int doorSide = b.form == PF_FREE || b.form == PF_COURT ? 0 : b.door;
    double doorPos = 0;
    double hs[4] = {b.hWall, b.hWall, b.hWall, b.hWall};
    switch (b.form) {
        case PF_CIRCLE: case PF_POLYGON: case PF_ELLIPSE: {
            Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = b.thick; R.mat = mat; R.slot = wallSlot(g, mat); R.y0 = y0;
            R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs); R.pattern = 1;
            R.crenels = g.gv(GV_TOPS) == TP_CRENELS && (rf == RF_FLAT || rf == RF_OPEN);
            const double hmin = ringWalls(g, R);
            const double rise = roundRoof(g, rf, 0, 0, R.rx + b.thick * 0.5, R.rz + b.thick * 0.5, R.segs, hmin, b.hWall, 0.8, roofMat);
            if (hmin >= 0.92 * b.hWall) roundTops(g, rf, g.sink + y0 + hmin, rise, 1.0, roofMat);
            foundation(g, R.rx, R.rz, 0, 0, mat);
            ringDress(g, R, hmin);
            yardOf(g, R.rx, R.rz, -1, 0, mat);
            drifts(g, R.rx, R.rz, Dm);
            return;
        }
        case PF_FREE: {   // an L: the front wing with the doorway, a back wing on one side
            Walls W; W.cz = b.hd * 0.44; W.hw = b.hw; W.hd = b.hd * 0.56; W.h = b.hWall; W.thick = b.thick; W.door = 0; W.mat = mat; W.slot = wallSlot(g, mat); W.y0 = y0;
            W.doorPos = doorPos = houseDoorPos(b, b.hw); W.glyphs = true; W.pattern = 1;
            const double hmin = rectBody(g, W, rf, roofMat, hs, true);
            Walls V = W; V.cx = (g.U(43) < 0.5 ? -1 : 1) * b.hw * 0.5; V.cz = -b.hd * 0.57; V.hw = b.hw * 0.5; V.hd = b.hd * 0.43; V.h = b.hWall * (0.8 + 0.2 * g.U(44)); V.door = -1; V.glyphs = false; V.k0 = 30;
            double hv[4]; rectBody(g, V, rf, roofMat, hv, false);
            (void)hmin;
            break;
        }
        case PF_COURT: {   // four ranges round an open court, the doorway through the front range and on into the court
            Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = b.thick; W.door = 0; W.mat = mat; W.slot = wallSlot(g, mat); W.y0 = y0;
            W.doorPos = doorPos = houseDoorPos(b, b.hw - 3.6); W.glyphs = true; W.pattern = 1;
            W.crenels = g.gv(GV_TOPS) == TP_CRENELS;
            const double hmin = walls4(g, W, hs);
            Walls I = W; I.hw = b.hw - 3.4; I.hd = b.hd - 3.4; I.glyphs = false; I.k0 = 30; I.crenels = false; I.pattern = 1;
            double hi[4]; const double him = walls4(g, I, hi);
            const double hr = std::min(hmin, him);
            const int rr = rf == RF_PITCHED || rf == RF_HIPPED || rf == RF_CONICAL || rf == RF_SADDLE ? RF_PITCHED : (rf == RF_VAULTED ? RF_VAULTED : RF_FLAT);
            const int rm = roofMaterial(g, rr, mat);
            roof(g, rr, 0, b.hd - 1.7, b.hw, 1.7, hr, b.hWall, 0.7, rm);
            roof(g, rr, 0, -b.hd + 1.7, b.hw, 1.7, hr, b.hWall, 0.7, rm);
            roof(g, rr, b.hw - 1.7, 0, 1.7, b.hd - 3.4, hr, b.hWall, 0.7, rm);
            roof(g, rr, -b.hw + 1.7, 0, 1.7, b.hd - 3.4, hr, b.hWall, 0.7, rm);
            if (hr >= 0.92 * b.hWall) houseTops(g, RF_FLAT, 0, 0, b.hw, b.hd, g.sink + y0 + hr, 0, rm, mat);
            break;
        }
        default: {   // C-01's rectangle: the doorway on its side, the window opposite, an inner wall now and then, the columns of a hall
            Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = b.thick; W.door = b.door; W.mat = mat; W.slot = wallSlot(g, mat); W.y0 = y0;
            const double doorL = (b.door == 0 || b.door == 2) ? b.hw : b.hd; W.doorPos = doorPos = houseDoorPos(b, doorL);
            const bool works = c.age >= AGE_INDUSTRIAL;   // an industrial people's warehouse: its tall windows, its parapet, its iron columns
            W.window = !works; W.glyphs = !works; W.pattern = works ? 2 : 1; W.parapet = parapetRoof(c) ? 0.8 : 0;
            const double hmin = rectBody(g, W, b.kind == BK_HALL && rf == RF_FLAT ? RF_FLAT : rf, roofMat, hs, b.kind == BK_HOUSE);
            if (b.kind == BK_HOUSE && g.lod == 0 && g.U(30) < 0.45) {   // an inner wall from the wall opposite the door, three fifths of the way in, beside the
                double cx, cz, half, dh;                                    // centre line (the room's middle stays clear, the doorway too)
                if (b.door == 0 || b.door == 2) { double zs = b.door == 0 ? -1 : 1; cx = 0.5 * b.hw; cz = zs * 0.7 * b.hd; half = 0.3 * b.hd; dh = PI / 2; }
                else { double xs = b.door == 1 ? -1 : 1; cx = xs * 0.7 * b.hw; cz = 0.5 * b.hd; half = 0.3 * b.hw; dh = 0; }
                g.box(cx, cz, dh, half, b.thick * 0.4, g.sink + y0, g.sink + y0 + std::max(0.35, hmin * 0.9), wallSlot(g, mat), mat);
            }
            if (b.kind == BK_HALL && g.lod == 0) {   // two rows of columns inside, some fallen
                int k = 0;
                const bool round = g.gv(GV_PLAN) == PF_CIRCLE || g.gv(GV_PLAN) == PF_ELLIPSE;
                for (int rowS = -1; rowS <= 1; rowS += 2)
                    for (double tt = -b.hd + 2.0; tt < b.hd - 1.5; tt += 3.0, k++) {
                        if (works) { const double d = g.Dk(MT_IRON, 40 + k); if (d < 0.7) g.lathe(0, rowS * b.hw * 0.5, tt, 0.16, g.sink, g.sink + b.hWall * 0.9 * (d < 0.35 ? 1 : 1 - d), g.ironSlot(), MT_IRON, 6); continue; }
                        double f = 1 - Dm * (0.2 + 0.8 * g.U(40 + k));
                        if (f < 0.2) continue;
                        if (round) g.lathe(0, rowS * b.hw * 0.5, tt, 0.36, g.sink, g.sink + b.hWall * 0.9 * f, 0, mat, 8);
                        else g.box(rowS * b.hw * 0.5, tt, 0, 0.35, 0.35, g.sink, g.sink + b.hWall * 0.9 * f, 0, mat, 1);
                    }
            }
            break;
        }
    }
    double doorX, doorZ; doorMid(doorSide, doorPos, b.hw, b.form == PF_FREE ? b.hd : b.hd, doorX, doorZ);
    rubble(g, b.hw, b.hd, hs, b.hWall, doorX, doorZ, wallSlot(g, mat), mat);
    foundation(g, b.hw, b.hd, doorSide, doorPos, mat);
    yardOf(g, b.hw, b.hd, doorSide, doorPos, mat);
    drifts(g, b.hw, b.hd, Dm);
}

namespace {

// a keep: C-01's tower, its walls round a room in the people's plan (four walls, or a ring of its polygon), slits up them, its top
// crenellated, roofed in the people's way, or crowned with its tops
void genKeep(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const int rf = c.roof, tp = g.gv(GV_TOPS);
    const bool flatTop = rf == RF_FLAT || rf == RF_OPEN || rf == RF_STEPPED || tp == TP_CRENELS;
    double hmin, hs[4] = {b.hWall, b.hWall, b.hWall, b.hWall}, keepRise = 0;
    if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = std::max(b.thick, 0.7); R.mat = mat; R.pattern = 2;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        R.crenels = flatTop;
        hmin = ringWalls(g, R);
        if (!flatTop) keepRise = roundRoof(g, rf == RF_PITCHED || rf == RF_HIPPED ? RF_CONICAL : rf, 0, 0, R.rx + R.thick * 0.5, R.rz + R.thick * 0.5, R.segs, hmin, b.hWall, 1.0, roofMaterial(g, rf, mat));
    } else {
        Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = b.thick; W.door = b.door; W.mat = mat; W.pattern = 2;
        const double doorL = (b.door == 0 || b.door == 2) ? b.hw : b.hd; W.doorPos = houseDoorPos(b, doorL); W.glyphs = true; W.crenels = flatTop;
        hmin = walls4(g, W, hs);
        if (!flatTop) keepRise = roof(g, rf == RF_PITCHED ? RF_CONICAL : rf, 0, 0, b.hw, b.hd, hmin, b.hWall, 1.0, roofMaterial(g, rf, mat));
        double doorX, doorZ; doorMid(b.door, W.doorPos, b.hw, b.hd, doorX, doorZ);
        rubble(g, b.hw, b.hd, hs, b.hWall, doorX, doorZ, 0, mat);
    }
    if (hmin >= 0.97 * b.hWall && (tp == TP_FINIALS || tp == TP_STATUES || tp == TP_MASTS)) {
        const bool ring = b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE;
        if (flatTop) { const double q = ring ? 0.7 : 0.92; topsAt(g, {{-b.hw * q, b.hd * q}, {b.hw * q, -b.hd * q}}, g.sink + hmin + 0.8, 1.0, mat); }   // on the merlons at two corners (a ring's at its diagonals)
        else {   // on the roof's crown, the roof's own, while it stands
            const int rd = ring ? (rf == RF_PITCHED || rf == RF_HIPPED ? RF_CONICAL : rf) : (rf == RF_PITCHED ? RF_CONICAL : rf);
            if (keepRise > 0) roundTops(g, rd, g.sink + hmin, keepRise, 1.0, roofMaterial(g, rf, mat));
        }
    }
    drifts(g, b.hw, b.hd, Dm);
}

// A-04: a market hall: columns in rows under one roof, open on every side; the roof only while every column stands
void genMarketHall(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const int n = clampi(c.genome.number() + 1, 4, 8), rows = 2;
    const bool round = g.gv(GV_PLAN) == PF_CIRCLE || g.gv(GV_PLAN) == PF_ELLIPSE;
    const double H = b.hWall;
    bool all = true;
    for (int r = 0; r < rows; r++)
        for (int k = 0; k < n; k++) {
            const double x = (r ? 1 : -1) * (b.hw - 0.5), z = -b.hd + 0.5 + (2 * b.hd - 1) * k / (n - 1);
            const double f = std::max(0.3, wallShare(Dm, g.U(40 + r * 16 + k)));
            if (f < 0.97) all = false;
            if (g.lod == 2 && k % 2) continue;
            if (round) g.lathe(0, x, z, 0.38, g.sink, g.sink + H * f, 0, mat, 8);
            else g.box(x, z, 0, 0.36, 0.36, g.sink, g.sink + H * f, 0, mat, 1);
        }
    if (all) {
        const int rf = c.roof == RF_OPEN ? RF_FLAT : c.roof;
        const double rise = roof(g, rf, 0, 0, b.hw, b.hd, H, H, 0.6, roofMaterial(g, rf, mat));
        houseTops(g, rf, 0, 0, b.hw, b.hd, g.sink + H, rise, roofMaterial(g, rf, mat), mat);
    }
}

// A-04: a palace: ranges round a court, a tower at each corner in the people's section and profile, its doorway in the front range
void genPalace(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = std::max(b.thick, 0.7); W.door = 0; W.doorPos = 0; W.mat = mat; W.glyphs = true; W.pattern = 3;
    W.crenels = g.gv(GV_TOPS) == TP_CRENELS;
    double hs[4]; const double hmin = walls4(g, W, hs);
    Walls I = W; I.hw = b.hw - 4.0; I.hd = b.hd - 4.0; I.glyphs = false; I.k0 = 30; I.crenels = false;
    double hi[4]; const double him = walls4(g, I, hi);
    const double hr = std::min(hmin, him);
    const int rf = c.roof == RF_PITCHED || c.roof == RF_HIPPED || c.roof == RF_CONICAL || c.roof == RF_SADDLE ? RF_PITCHED : RF_FLAT, rm = roofMaterial(g, rf, mat);
    roof(g, rf, 0, b.hd - 2.0, b.hw, 2.0, hr, b.hWall, 0.6, rm); roof(g, rf, 0, -b.hd + 2.0, b.hw, 2.0, hr, b.hWall, 0.6, rm);
    roof(g, rf, b.hw - 2.0, 0, 2.0, b.hd - 4.0, hr, b.hWall, 0.6, rm); roof(g, rf, -b.hw + 2.0, 0, 2.0, b.hd - 4.0, hr, b.hWall, 0.6, rm);
    const double tr = 2.6 + 0.04 * b.hw, tH = b.hWall * (1.5 + 0.8 * g.gf(GV_HEIGHT));
    for (int q = 0; q < 4; q++) {
        const double x = (q % 2 ? 1 : -1) * (b.hw - tr * 0.6), z = (q < 2 ? 1 : -1) * (b.hd - tr * 0.6);
        TowerOut T = towerShaft(g, x, z, tr, tH, mat, 50 + q * 3, 2);
        if (T.whole) towerTop(g, x, z, T.rTop, T.h, tH, mat, g.gv(GV_FOCUS) == FO_SPIRE, tH * 0.6);
    }
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    drifts(g, b.hw, b.hd, Dm);
}
}   // namespace

void genRotunda(Gen& g) {   // C-01's: a ring of wall, its doorway a segment left out, its roof; A-04: the people's ring and roof; a pod the futuristic age's
    const Building& b = g.b; const Culture& c = g.c;
    const bool pod = b.kind == BK_POD;
    const int mat = pod ? MT_COMPOSITE : stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = b.thick; R.mat = mat; R.pattern = pod ? 2 : 1;
    R.segs = ringSegs(b.form == PF_RECT || b.form == PF_FREE || b.form == PF_COURT ? PF_CIRCLE : b.form, b.sides, std::min(R.rx, R.rz));
    R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
    const int rf = pod && (c.roof == RF_PITCHED || c.roof == RF_HIPPED) ? RF_DOMED : c.roof;
    R.crenels = !pod && g.gv(GV_TOPS) == TP_CRENELS && (rf == RF_FLAT || rf == RF_OPEN);
    const double hmin = ringWalls(g, R);
    if (hmin < b.hWall * 0.7 && !(g.lod == 2)) { drifts(g, R.rx, R.rz, Dm); return; }
    const int rm = pod ? MT_COMPOSITE : roofMaterial(g, rf, mat);
    double rise = roundRoof(g, rf, 0, 0, R.rx + b.thick * 0.5, R.rz + b.thick * 0.5, R.segs, hmin, b.hWall, 0.8, rm);
    if (pod && rf == RF_DOMED) { for (Form& e : g.out) if (e.building == g.bi && e.kind == FK_DOME) { e.slot = 2; e.sub = 1; e.y1 = e.y0 + (e.hx) * 0.55; rise = e.y1 - (g.sink + hmin); } }   // a pod's saucer of glass
    if (hmin >= 0.92 * b.hWall) roundTops(g, rf, g.sink + hmin, rise, 1.0, rm);
    if (pod) podDress(g, R.rx, R.rz, b.hWall, hmin, R.segs, R.door, R.doorSegs);
    drifts(g, R.rx, R.rz, Dm);
}

void genGate(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.age == AGE_PREHISTORIC) {   // two great posts in the palisade's gap
        g.post(-b.hw, 0, b.hWall + 1.0, 0.3, 10); g.post(b.hw, 0, b.hWall + 1.0, 0.3, 11);
        return;
    }
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (b.hWall < 3.5) {   // C-01's gate: two piers and the lintel across them
        double h = b.hWall * (1 - 0.5 * Dm), f0 = 1 - Dm * g.U(10), f1 = 1 - Dm * g.U(11);
        g.box(-b.hw, 0, 0, 0.5, 0.5, g.sink, g.sink + std::max(0.5, h * f0), 0, mat, 1);
        g.box(b.hw, 0, 0, 0.5, 0.5, g.sink, g.sink + std::max(0.5, h * f1), 0, mat, 1);
        if (f0 > 0.8 && f1 > 0.8 && g.lod < 2) g.box(0, 0, 0, b.hw + 0.5, 0.5, g.sink + h - 0.7, g.sink + h, 0, mat, 4);
        return;
    }
    // a gatehouse: two towers over the wall (the people's section) and the passage's arch between them (its opening's shape)
    const double h = b.hWall, tw = 1.25;
    const bool round = g.gv(GV_PLAN) == PF_CIRCLE || g.gv(GV_PLAN) == PF_POLYGON || g.gv(GV_PLAN) == PF_ELLIPSE;
    double t0 = h * wallShare(Dm, g.U(10)), t1 = h * wallShare(Dm, g.U(11));
    for (int s = -1; s <= 1; s += 2) {
        const double x = s * (b.hw + tw - 0.1), th = std::max(0.6, s < 0 ? t0 : t1);
        if (round) g.lathe(0, x, 0, 1.5, g.sink, g.sink + th, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 0);
        else g.box(x, 0, 0, tw, 1.4, g.sink, g.sink + th, 0, mat);
        if (th >= h * 0.97 && g.lod == 0 && g.gv(GV_TOPS) == TP_CRENELS) for (int k = -1; k <= 1; k += 2) g.box(x + k * 0.6, 0, 0, 0.35, 1.2, g.sink + th, g.sink + th + 0.8, g.topSlot(mat), mat);
    }
    const double archTop = std::min(h - 1.0, b.hw * 1.1 + 3.2);
    const int sub = archSub(g);
    if (std::min(t0, t1) > archTop && g.lod < 2) { Form& e = g.add(FK_ARCH, sub, 0, 0, 0, b.hw + 0.05, 1.0, g.sink, g.sink + archTop + 0.8, g.doorSlot(), mat); e.p[0] = 1; e.p[1] = 0.46; e.p[2] = 3.0; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.8); }
}

void genWallTower(Gen& g) {   // a tower astride the town wall in the people's section and profile, its top the people's
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    TowerOut T = towerShaft(g, 0, 0, b.hw, b.hWall, mat, 10, 2, false);
    if (T.whole) towerTop(g, 0, 0, T.rTop, T.h, b.hWall, mat, false, 0);
}

// C-01's: steps (A-04: the people's number of them), a stair up the front, a shrine on top now and then. B-415: an industrial people's a
// bandstand (its base of brick, iron columns under an iron cone), a modern or a later people's a plinth of two tiers and its mast (A-04's
// stepped platform and shrine in a mill town read as an ancient one's)
void genPlatform(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (c.age == AGE_INDUSTRIAL) {
        const double rb = std::min(b.hw, b.hd) * 0.75, Di = g.D(MT_IRON);
        { Form& e = g.lathe(0, 0, 0, rb, g.sink, g.sink + 1.0, 0, MT_MASONRY, 8); e.part = 2; }
        if (g.lod == 2) return;
        if (g.lod == 0) g.box(0, rb + 0.6, 0, 1.0, 0.6, 0, 0.5, 0, MT_MASONRY, 2);
        bool all = true;
        for (int k = 0; k < 8; k++) { const double a = (k + 0.5) * TAU / 8, d = g.Dk(MT_IRON, 20 + k); if (d > 0.7) { all = false; continue; } if (d > 0.35) all = false; g.lathe(0, std::sin(a) * rb * 0.85, std::cos(a) * rb * 0.85, 0.1, g.sink + 1.0, g.sink + 1.0 + 3.2 * (d < 0.35 ? 1 : 1 - d), g.ironSlot(), MT_IRON, 6); }
        if (all && Di < 0.4) { Form& r = g.lathe(1, 0, 0, rb * 1.05, g.sink + 4.2, g.sink + 4.2 + rb * 0.6, g.roofSlot(MT_IRON), MT_IRON, 8); r.p[0] = 0.04; }
        return;
    }
    if (c.age >= AGE_MODERN) {
        g.box(0, 0, 0, b.hw, b.hd, g.sink, g.sink + 0.9, 0, mat, 2);
        if (g.lod == 2) return;
        g.box(0, -b.hd * 0.2, 0, b.hw * 0.6, b.hd * 0.5, g.sink + 0.9, g.sink + 1.8, 0, mat, 2);
        if (g.D(MT_IRON) < 0.5) g.lathe(0, 0, -b.hd * 0.2, 0.09, g.sink + 1.8, g.sink + 13, g.ironSlot(), MT_IRON, 6);   // its mast
        return;
    }
    if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, g.sink, g.sink + 3.3, 0, mat, 2); return; }
    const int T = clampi(c.genome.number() - 1, 2, 6); const double th = 3.3 / T;
    for (int i = 0; i < T; i++) g.box(0, 0, 0, b.hw * (1 - 0.84 * i / T), b.hd * (1 - 0.84 * i / T), g.sink + th * i, g.sink + th * (i + 1), 0, mat, 2);
    if (g.lod == 0) for (int k = 0; k < 4; k++) g.box(0, b.hd + 0.35 + 0.7 * (3 - k), 0, 1.6, 0.35, 0, 0.28 * (k + 1), 0, mat, 2);
    if (g.U(7) < 0.5 && Dm < 0.6) { Form& e = g.box(0, 0, 0, 1.3, 1.3, g.sink + 3.3, g.sink + 3.3 + 2.4 * (1 - 0.5 * Dm), 0, mat); e.glyphs = g.lod == 0; if (Dm < 0.3) topsAt(g, {{0, 0}}, g.sink + 3.3 + 2.4, 0.8, mat); }
}

// C-01's columns in a row, some fallen (A-04: the people's number of them, round or square); an arcade where the people built arches. B-415:
// an industrial people's a covered walk, its cast-iron columns under a canopy of iron and glass; a modern people's its pilotis under a
// slab of concrete; a later people's of composite (BUBAS's columns of stone along its main streets read as medieval)
void genColonnade(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (g.lod == 2) return;
    if (c.age >= AGE_INDUSTRIAL) {
        const bool iron = c.age == AGE_INDUSTRIAL;
        const int cm = iron ? MT_IRON : (c.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE), cs = iron ? g.ironSlot() : 0;
        const double H = b.hWall;
        const int n = std::max(3, (int)std::lround(2 * b.hd / 3.2) + 1);
        bool all = true;
        for (int k = 0; k < n; k++) {
            const double tt = -b.hd + 2 * b.hd * k / (n - 1), d = g.Dk(cm, 10 + k);
            if (d > 0.75) { all = false; continue; }
            const double f = d < 0.35 ? 1 : 1 - (d - 0.35) * 1.5;
            if (f < 0.97) all = false;
            if (iron) g.lathe(0, 0, tt, 0.12, 0, H * f, cs, cm, 6);
            else g.box(0, tt, 0, 0.2, 0.2, 0, H * f, cs, cm, 1);
        }
        if (all && g.D(cm) < (iron ? 0.4 : 0.55)) g.box(0, 0, 0, 1.4, b.hd + 0.4, H, H + (iron ? 0.15 : 0.3), iron ? 2 : 0, cm, 2);   // the canopy
        return;
    }
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (c.technique == TQ_ARCH_VAULT && g.U(3) < 0.75) {
        Form& e = g.add(FK_ARCH, archSub(g), 0, 0, PI / 2, b.hd, 0.45, g.sink, g.sink + b.hWall * 1.25, 0, mat);
        e.p[0] = std::max(2.0, std::round(b.hd * 2 / 3.2)); e.p[1] = 0.34; e.p[2] = b.hWall * 0.65; e.broken = Dm < 0.3 ? 0 : clampd(Dm, 0.3, 0.9);
        return;
    }
    const int n = clampi(c.genome.number() + 2, 4, 11);
    const bool round = g.gv(GV_PLAN) != PF_RECT && g.gv(GV_PLAN) != PF_COURT;
    for (int k = 0; k < n; k++) {
        const double tt = -b.hd + 1.0 + (2 * b.hd - 2.0) * k / (n - 1), f = 1 - Dm * (0.2 + 0.8 * g.U(10 + k));
        if (f < 0.25) continue;
        if (round) g.lathe(0, 0, tt, 0.42, g.sink, g.sink + b.hWall * f, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 8);
        else g.box(0, tt, 0, 0.4, 0.4, g.sink, g.sink + b.hWall * f, 0, mat, 1);
        if (f > 0.97 && g.lod == 0 && g.carved()) g.box(0, tt, 0, 0.6, 0.6, g.sink + b.hWall - 0.3, g.sink + b.hWall, g.accentOn(AP_TOPS) ? 1 : 0, mat, 4);   // a capital
    }
}

// C-01's stela; A-04: the people's monument (`Building::style`): a stela, an obelisk, a statue on its pedestal, a free arch, a pillar
// with its capital (the prehistoric age's are standing stones: `genMenhir`)
void genStela(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    switch (b.style) {
        case MN_OBELISKS: {   // an obelisk with its point
            double h = b.hWall * 1.6 * (1 - 0.5 * Dm * g.U(10));
            Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, 0.6, 0.6, g.sink, g.sink + h, 0, mat, 5); e.p[1] = 0.62;
            if (h > b.hWall * 1.5) { Form& t = g.add(FK_PYRAMID, 0, 0, 0, 0, 0.6 * 0.62, 0.6 * 0.62, g.sink + h, g.sink + h + 0.7, g.topSlot(mat), mat, 5); t.p[1] = 0; }
            return;
        }
        case MN_COLOSSI: {   // a statue on its pedestal
            const double ph = b.hWall * 0.3;
            g.box(0, 0, 0, 0.9, 0.9, g.sink, g.sink + ph, 0, mat, 5);
            figure(g, 0, 0, g.sink + ph, b.hWall * 0.9, g.accentOn(AP_TOPS) ? 1 : 0, mat, Dm);
            return;
        }
        case MN_ARCHES: {   // a free arch
            Form& e = g.add(FK_ARCH, archSub(g), 0, 0, 0, 2.4, 0.7, g.sink, g.sink + b.hWall * 1.4, 0, mat, 5);
            e.p[0] = 1; e.p[1] = 0.36; e.p[2] = b.hWall * 0.7; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.85);
            return;
        }
        case MN_PILLARS: {   // a column alone, its capital
            const double h = b.hWall * 1.8 * (Dm < 0.4 ? 1 : 1 - 0.6 * Dm * g.U(10));
            Form& e = g.lathe(0, 0, 0, 0.45, g.sink, g.sink + h, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 0); e.part = 5;
            if (h >= b.hWall * 1.75 && g.lod < 2) { g.box(0, 0, 0, 0.7, 0.7, g.sink + h, g.sink + h + 0.4, g.topSlot(mat), mat, 5); topsAt(g, {{0, 0}}, g.sink + h + 0.4, 0.7, mat); }
            return;
        }
        default: { Form& e = g.box(0, 0, 0, 0.45, 0.25, g.sink, g.sink + b.hWall * (1 - Dm * 0.6 * g.U(10)), 0, mat, 5); e.glyphs = g.lod == 0; return; }
    }
}

void genRubble(Gen& g) {   // C-01's heap where a house fell
    const Building& b = g.b; const int mat = worksOf(b, g.c);
    if (g.lod == 2) { g.box(0, 0, 0, b.hw * 0.8, b.hd * 0.8, 0, 1.0, 0, mat, 3); return; }
    int nb = 5 + (int)(g.U(1) * 4);
    for (int k = 0; k < nb; k++) {
        double u1 = g.U(10 + k), u2 = g.U(30 + k), sz = 0.4 + 0.7 * g.U(50 + k);
        g.box((u1 - 0.5) * 1.6 * b.hw, (u2 - 0.5) * 1.6 * b.hd, u1 * 2.0, sz, sz * 0.7, 0, sz * 1.2, 0, mat, 3);
    }
}

void genWall(Gen& g) {   // C-01's stretch of the town wall; A-04: battered where the people battered its walls, crenellated where it crenellated
    const Building& b = g.b; const int mat = worksOf(b, g.c); const double Dm = g.D(mat);
    g.sink = -g.c.buried * Dm;
    const double h = std::max(0.4, b.hWall * wallShare(Dm, g.U(10)));
    const int prof = g.gv(GV_PROFILE);
    Form& e = g.box(0, 0, 0, b.hw, b.hd * (prof == WP_BATTERED ? 1.5 : 1.0), g.sink, g.sink + h, 0, mat);
    if (prof == WP_BATTERED) e.tilt = g.U(11) < 0.5 ? 0.06 : -0.06;   // its outer face leaning back
    if (h >= b.hWall * 0.97 && g.lod == 0 && g.gv(GV_TOPS) == TP_CRENELS) {
        const int n = std::max(2, (int)(2 * b.hw / 1.6));
        for (int k = 0; k < n; k += 2) { double t = -b.hw + (k + 0.5) * 2 * b.hw / n; g.box(t, 0, 0, b.hw / n * 0.5, b.hd, g.sink + h, g.sink + h + 0.8, g.topSlot(mat), mat); }
    }
}

// C-08: the terrain's features, of the age's works' material
void genFeature(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const double sink = g.sink;
    switch (b.kind) {
        case BK_QUAY: {   // two tiers along the shore, the top one worn away toward one end, bollards on the seaward edge
            double keep = 1 - 0.5 * Dm;
            g.box(0, 0, 0, b.hw, b.hd, sink - 0.6, sink + 0.6, 0, mat, 6);
            g.box(-b.hw * (1 - keep), 0, 0, b.hw * keep, b.hd - 0.2, sink + 0.6, sink + 1.2, 0, mat, 6);
            if (g.lod == 0)
                for (int k = 0; k < 3; k++) {
                    if (g.U(20 + k) < Dm * 0.6) continue;
                    g.box(-b.hw * 0.7 + b.hw * 0.7 * k, b.hd - 0.5, 0, 0.22, 0.22, sink + 1.2, sink + 1.9, 0, mat, 1);
                }
            break;
        }
        case BK_MOLE: {   // a wall out into the water, its far end fallen to the waterline
            if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, sink - 0.6, sink + b.hWall * (1 - 0.5 * Dm), 0, mat, 6); break; }
            int segs = std::max(2, (int)(b.hd / 3.0));
            for (int k = 0; k < segs; k++) {
                double u = (k + 0.5) / segs, f = 1 - Dm * (0.1 + 0.9 * u) * (0.6 + 0.4 * g.U(10 + k));
                if (f < 0.12) continue;
                g.box(0, -b.hd + b.hd * 2 * u, 0, b.hw, b.hd / segs, sink - 0.6, sink + std::max(0.1, b.hWall * f), 0, mat, 6);
            }
            break;
        }
        case BK_BOLLARD: g.box(0, 0, 0, 0.22, 0.22, sink, sink + 0.8, 0, mat, 1); break;
        case BK_TERRACE: { double f = 1 - Dm * (0.2 + 0.8 * g.U(10)); g.box(0, 0, 0, b.hw, b.hd, sink, sink + std::max(0.3, b.hWall * f), 0, mat); break; }
        case BK_CISTERN: {   // a round tank of thick wall, its lid fallen in, the channel that fed it from upslope
            double rr = b.hw;
            if (g.lod == 2) { g.box(0, 0, 0, rr, rr, sink, sink + b.hWall * (1 - 0.4 * Dm), 0, mat); break; }
            const int N = 10; double ht = b.thick * 0.5, half = rr * std::sin(PI / N) + ht * 0.6, hmin = 1e9;
            for (int k = 0; k < N; k++) {
                double a = (k + 0.5) * TAU / N, h = std::max(0.4, b.hWall * (1 - Dm * (0.1 + 0.6 * g.U(10 + k))));
                hmin = std::min(hmin, h);
                g.box(std::sin(a) * rr, std::cos(a) * rr, a, half, ht, sink, sink + h, 0, mat);   // along the ring (C-08's stood across it)
            }
            if (Dm < 0.5) g.box(0, 0, 0, rr * 0.5, rr * 0.5, sink + hmin - 0.1, sink + hmin + 0.25, 0, mat, 2);   // what is left of the lid
            if (g.lod == 0) {   // the channel: two low walls a metre apart, 6-14 m out along the heading (upslope)
                double len = 6 + 8 * g.U(30);
                for (int side = -1; side <= 1; side += 2) g.box(side * 0.6, rr + 0.7 + len * 0.5, 0, 0.15, len * 0.5, sink, sink + 0.45, 0, mat);
            }
            break;
        }
        case BK_WINDWALL: {   // a segment of the crescent, the sand against its windward face
            double f = 1 - Dm * (0.15 + 0.85 * g.U(10));
            g.box(0, 0, 0, b.hw, b.hd, sink, sink + std::max(0.4, b.hWall * f), 0, mat);
            if (g.lod == 0 && c.buried >= 0.15) g.box(0, b.hd + 0.9, 0, b.hw, 0.9, 0, 0.25 + 0.6 * std::min(1.0, c.buried / 1.2), 0, -1, 7);
            break;
        }
        case BK_CAIRN: {   // three tiers of stones, the top ones tumbled by the age
            const double Dc = g.D(MT_DRYSTONE);
            int tiers = Dc < 0.35 ? 3 : (Dc < 0.7 ? 2 : 1);
            for (int i = 0; i < tiers; i++) g.box(0, 0, i * 0.3, b.hw * (1 - 0.3 * i), b.hd * (1 - 0.3 * i), 0.7 * i, 0.7 * (i + 1), 0, MT_DRYSTONE, 3);
            if (g.lod == 0 && tiers < 3) g.box(1.4, 0.6, 0.8, 0.4, 0.3, 0, 0.4, 0, MT_DRYSTONE, 3);
            break;
        }
        default: break;
    }
}

// the people's motif on a great building of half extents hw x hd, its walls standing to `h` of their `H`: ribs up its walls (lines),
// spheres on its corners (circles), steps along its top (steps), a ramp climbing round it (spirals), bands of glyphs (the walls'),
// statues at its front's corners (figures), the points of a star round its foot (stars); what stands on its walls only while they do
void motifOn(Gen& g, double cx, double cz, double hw, double hd, double h, double H, int mat) {
    if (g.lod == 2) return;
    const double base = g.sink;
    const bool whole = h >= 0.92 * H;
    switch (g.gv(GV_MOTIF)) {
        case MO_LINES:
            if (g.lod == 0 && h > 1.5) for (int s = 0; s < 4; s++) {
                const bool al = s % 2 == 0; const double L = al ? hw : hd, nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0);
                const int n = std::max(2, (int)(2 * L / 2.2));
                for (int k = 1; k < n; k++) { const double t = -L + 2 * L * k / n; if (s == 0 && std::fabs(t) < 1.4) continue; g.box(cx + (al ? t : nx * (hw + 0.3)), cz + (al ? nz * (hd + 0.3) : -t), al ? 0 : PI / 2, 0.16, 0.2, base, base + h, 0, mat, 4); }
            }
            break;
        case MO_CIRCLES: if (whole) { const double r = std::min(hw, hd) * 0.18 + 0.3; for (int q = 0; q < 4; q++) g.add(FK_DOME, 0, cx + (q % 2 ? hw : -hw), cz + (q < 2 ? hd : -hd), 0, r, r, base + h, base + h + r, g.topSlot(mat), mat, 4); } break;
        case MO_STEPS:
            if (whole && g.lod == 0) for (int s = 0; s < 4; s += 2) for (int k = 0; k < 3; k++) { const double w = hw * (0.8 - 0.25 * k); g.box(cx, cz + (s == 0 ? hd : -hd), 0, w, 0.35, base + h + 0.6 * k, base + h + 0.6 * (k + 1), g.topSlot(mat), mat, 4); }
            break;
        case MO_SPIRALS:   // a ramp round the walls, a side a quarter of its rise, the front's doorway's stretch left out
            if (whole && g.lod == 0) for (int k = 0; k < 8; k++) {
                const int s = (k / 2 + 1) % 4; const bool al = s % 2 == 0; const double L = al ? hw : hd, nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0);
                const double t = (k % 2 ? 0.5 : -0.5) * L, y = base + h * (k + 1) / 9.0;
                if (s == 0 && std::fabs(t) < 1.6) continue;
                g.box(cx + (al ? t : nx * (hw + 0.45)), cz + (al ? nz * (hd + 0.45) : -t), al ? 0 : PI / 2, L * 0.5, 0.45, y - 0.35, y, 1, mat, 4);
            }
            break;
        case MO_FIGURES: for (int s = -1; s <= 1; s += 2) figure(g, cx + s * (hw + 1.2), cz + hd + 1.2, base, std::max(2.4, H * 0.45), g.accentOn(AP_TOPS) ? 1 : 0, mat, g.D(mat)); break;
        case MO_STARS:
            if (g.lod == 0) for (int k = 0; k < 8; k++) {
                const double a = k * PI / 4 + PI / 8, R = std::max(hw, hd) * 1.15 + 2.0;
                if (std::fabs(std::remainder(a, TAU)) < 0.5) continue;   // the front's point left out (the doorway)
                Form& e = g.add(FK_PYRAMID, 0, cx + std::sin(a) * R, cz + std::cos(a) * R, a, 1.4, 2.6, base, base + 0.6, g.accentOn(AP_TOPS) ? 1 : 0, mat, 4); e.sides = 3; e.p[1] = 0;
            }
            break;
        default: break;   // glyph bands: the walls' own
    }
}

// the temple at a medieval town's centre: a basilica (a nave under a gable or a vault, its towers as its people placed them: a pair
// at the front, one over the crossing, two at opposite corners, one at a corner), a square hall under a dome on its drum with its
// minarets, a stepped mass with a shrine on top and a small house at the stair's foot, or (A-04) a round temple under a dome or a cone
// in a ring of columns. A-04: in its age's material (a modern people's temple of concrete), its towers in the people's section,
// profile and tops, its motif. B-415: an industrial people's round temple a brick rotunda under a saucer of iron (no peristyle); a modern
// people's of its age: its stepped mass a low mausoleum of three broad setbacks, its round temple a drum under a saucer or a cone, its
// domed hall a saucer and needles for minarets, its basilica's roof a shell of concrete (a steep gable, a vault, a saddle, a saucer, a
// tent; steps a flat roof) and its towers' tops needles, saucers or masts (A-04's concrete ziggurats, onion domes and peristyles read as
// a medieval or an ancient town's heart)
void genTemple(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const double big = b.hWall;   // the walls' height (the nave's, the hall's); the rest in proportion
    const double hgt = 0.7 + 0.8 * g.gf(GV_HEIGHT);   // the people's appetite for height: its towers and minarets
    const bool modern = c.age >= AGE_MODERN, machine = c.age >= AGE_INDUSTRIAL;
    if (b.style == 2) {   // the stepped mass: tiers, the shrine on top, the stair up its front, a house at the stair's foot
        const double H = big * (modern ? 0.9 : 2.2), hx = b.hw * (modern ? 0.9 : 0.8), hz = b.hd * 0.62, cz = -b.hd * 0.3;
        const double top = H * tallShare(Dm * 0.8, g.U(10));
        Form& z = g.add(FK_PYRAMID, 0, 0, cz, 0, hx, hz, g.sink, g.sink + H, 0, mat); z.p[0] = modern ? 3 : clampi(c.genome.number(), 3, 7); z.p[1] = modern ? 0.62 : 0.38; z.p[2] = modern ? 1 : 0.55;
        if (modern) z.pattern = 3;
        if (g.gv(GV_PLAN) == PF_POLYGON) z.sides = clampi(c.genome.number(), 3, 8);
        z.broken = top < H * 0.97 ? clampd(1 - top / H, 0.1, 0.95) : 0;
        if (z.broken == 0 && g.lod < 2 && !modern) { Form& s = g.box(0, cz, 0, hx * 0.3, hz * 0.3, g.sink + H, g.sink + H + big * 0.5, 0, mat); s.glyphs = g.lod == 0; topsAt(g, {{0, cz}}, g.sink + H + big * 0.5, 1.2, mat); }
        if (g.lod == 0) for (int k = 0; k < 6; k++) { double t = (k + 0.5) / 6; g.box(0, cz + hz + 1.2 - t * hz * 0.85, 0, 1.6, 1.2, g.sink, g.sink + H * t * 0.8 * (1 - 0.5 * Dm), 0, mat, 2); }
        Walls W; W.cz = b.hd - 3.5; W.hw = 3.2; W.hd = 3.0; W.h = 4.0; W.thick = 0.6; W.door = 0; W.mat = mat; W.glyphs = true; W.k0 = 30;
        double hs[4]; double hmin = walls4(g, W, hs);
        roof(g, RF_FLAT, 0, W.cz, W.hw, W.hd, hmin, W.h, 0.8, mat);
        drifts(g, b.hw, b.hd, Dm);
        return;
    }
    if (b.style == 3) {   // the round temple: a ring of wall in the people's section, its dome or cone, a ring of columns round it
        const double r = b.hw;
        Ring R; R.rx = r; R.h = big; R.thick = 1.0; R.mat = mat; R.pattern = 3;
        R.segs = templeRingSegs(b);   // the room reads the same (`buildingRoom`)
        R.rz = b.form == PF_ELLIPSE ? r * 1.25 : r;
        R.doorSegs = ringDoorSegs(r, R.segs); R.door = ((R.segs - R.doorSegs / 2) % R.segs + R.segs) % R.segs;
        const double hmin = ringWalls(g, R);
        const int rf = c.roof == RF_DOMED || c.roof == RF_VAULTED || c.roof == RF_FLAT || c.roof == RF_OPEN || c.roof == RF_STEPPED ? (c.roof == RF_STEPPED && !machine ? RF_STEPPED : RF_DOMED) : RF_CONICAL;
        const int rm = machine && rf == RF_DOMED ? (modern ? MT_CONCRETE : MT_IRON) : roofMaterial(g, rf, mat);
        double rise = 0;
        if (machine && rf == RF_DOMED) {   // a saucer: of iron and glass, or a concrete shell
            const double Dr = g.D(rm);
            if (hmin >= big * 0.92 && Dr < 0.6) { Form& e = g.add(FK_DOME, 1, 0, 0, 0, R.rx + 0.5, R.rz + 0.5, g.sink + hmin - 0.1, g.sink + hmin - 0.1 + r * 0.5, g.roofSlot(rm), rm, 2); e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9); rise = r * 0.4; }
        } else rise = roundRoof(g, rf, 0, 0, R.rx + 0.5, R.rz + 0.5, R.segs, hmin, big, 0.8, rm);
        if (hmin >= big * 0.92) roundTops(g, rf, g.sink + hmin, rise, 1.4, rm);
        if (g.lod < 2 && !machine) {   // the peristyle
            const int n = std::max(8, 2 * c.genome.number());
            for (int k = 0; k < n; k++) {
                const double a = (k + 0.5) * TAU / n; if (std::fabs(std::remainder(a, TAU)) < 0.4) continue;   // the front's left open
                const double f = 1 - Dm * (0.2 + 0.8 * g.U(60 + k)); if (f < 0.25) continue;
                g.lathe(0, std::sin(a) * (R.rx + 2.6), std::cos(a) * (R.rz + 2.6), 0.45, g.sink, g.sink + big * 0.85 * f, 0, mat, 8);
            }
        }
        motifOn(g, 0, 0, r * 0.75, r * 0.75, std::min(hmin, big), big, mat);
        drifts(g, R.rx, R.rz, Dm);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = big; W.thick = 1.1; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 3; W.glyphs = true;
    if (b.style == 1) { W.hd = b.hw; W.cz = 0; }   // the square hall
    W.crenels = b.style == 1 && g.gv(GV_TOPS) == TP_CRENELS;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int sym = g.gv(GV_SYMMETRY);
    if (b.style == 1) {   // the drum, the dome, the minarets (two at the front, four at the corners, or round it by the people's symmetry and number)
        const double rd = b.hw * 0.72, drumTop = big + 3.5 + b.hw * 0.15;
        if (hmin > big * 0.92) {
            Form& d = g.lathe(0, 0, 0, rd, g.sink + hmin, g.sink + drumTop, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 8); d.pattern = 3;
            if (Dm < 0.65) {
                const int prof = modern ? 1 : (g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : (g.gv(GV_MOTIF) == MO_CIRCLES ? 0 : (b.age >= AGE_MODERN ? 1 : 0))));
                const double dH = rd * (prof == 3 ? 1.5 : (prof == 2 ? 1.35 : (prof == 1 ? 0.55 : 1.0)));
                Form& e = g.add(FK_DOME, prof, 0, 0, 0, rd, rd, g.sink + drumTop, g.sink + drumTop + dH, g.roofSlot(mat), mat, 2); e.broken = Dm < 0.3 ? 0 : clampd(Dm, 0.3, 0.9);
                if (Dm < 0.3) topsAt(g, {{0, 0}}, g.sink + drumTop + dH, 1.5, mat);
            }
        }
        const double mh = big * 3.4 * hgt;
        const int nm = sym == SY_NONE ? 1 : (sym == SY_BILATERAL ? 2 : (sym == SY_RADIAL ? clampi(c.genome.number(), 4, 8) : 4));
        for (int k = 0; k < nm; k++) {
            double lx, lz;
            if (sym == SY_RADIAL) { const double a = (k + 0.5) * TAU / nm, rr = b.hw * 1.42 + 3.0; lx = std::sin(a) * rr; lz = std::cos(a) * rr; }
            else if (sym == SY_ROTATIONAL) { const double a = k * PI / 2; lx = std::cos(a) * (b.hw + 3.0) - std::sin(a) * (b.hw * 0.5); lz = std::sin(a) * (b.hw + 3.0) + std::cos(a) * (b.hw * 0.5); }
            else { lx = (k % 2 ? 1 : -1) * (b.hw + 3.0); lz = (k < 2 ? 1 : -1) * (b.hw + 3.0); }
            TowerOut T = towerShaft(g, lx, lz, modern ? 1.0 : 1.5, mh, mat, 13 + k * 3, 2);
            if (T.whole && modern) { Form& t = g.lathe(2, lx, lz, 0.9, g.sink + T.h, g.sink + T.h + mh * 0.18, g.topSlot(mat), mat); t.p[0] = 1.4; }   // a needle
            else if (T.whole) { if (g.c.roof == RF_DOMED) { Form& t = g.add(FK_DOME, 3, lx, lz, 0, 1.6, 1.6, g.sink + T.h, g.sink + T.h + 3.0, g.topSlot(mat), mat); (void)t; } else { Form& t = g.lathe(1, lx, lz, 1.8, g.sink + T.h, g.sink + T.h + 3.5, g.topSlot(mat), mat); t.p[0] = 0; } }
        }
    } else {   // the basilica: its roof, its towers with their spires (the people's focus a spire: tall)
        const int rf = modern ? (c.roof == RF_STEPPED || c.roof == RF_OPEN ? RF_FLAT : c.roof) : (c.roof == RF_FLAT ? RF_VAULTED : (c.roof == RF_OPEN ? RF_PITCHED : c.roof));
        roof(g, rf, 0, 0, b.hw, b.hd, hmin, big, modern && rf == RF_PITCHED ? 1.6 : 0.9, modern ? MT_CONCRETE : roofMaterial(g, rf, mat));   // a modern one's shell of concrete
        if (rf == RF_PITCHED) gables(g, W, hs, modern ? 1.6 : 0.9);
        const double tr = std::max(3.0, b.hw * 0.42), tH = big * 2.6 * hgt, sH = big * (g.gv(GV_FOCUS) == FO_SPIRE ? 3.4 : 2.2);
        std::vector<std::pair<double, double>> at;
        const double side = (g.U(14) < 0.5 ? -1 : 1);
        switch (sym) {
            case SY_BILATERAL: at = {{-(b.hw + tr - 0.6), b.hd - tr}, {b.hw + tr - 0.6, b.hd - tr}}; break;
            case SY_RADIAL: at = {{0, 0}}; break;   // over the crossing
            case SY_ROTATIONAL: at = {{side * (b.hw + tr - 0.6), b.hd - tr}, {-side * (b.hw + tr - 0.6), -b.hd + tr}}; break;
            default: at = {{side * (b.hw + tr - 0.6), b.hd - tr}}; break;
        }
        for (size_t k = 0; k < at.size(); k++) {
            double tx = at[k].first, tz = at[k].second, H0 = 0;
            if (sym == SY_RADIAL) { if (hmin < big * 0.92) continue; H0 = hmin + 0.3; }   // the crossing's tower stands on the walls
            const double sinkSave = g.sink; g.sink += H0;
            TowerOut T = towerShaft(g, tx, tz, sym == SY_RADIAL ? std::min(tr, b.hw * 0.8) : tr, sym == SY_RADIAL ? tH * 0.7 : tH, mat, 15 + (int)k * 3, 2, sym != SY_RADIAL);
            if (T.whole) towerTop(g, tx, tz, T.rTop, T.h, tH, mat, true, sH);
            g.sink = sinkSave;
        }
    }
    motifOn(g, 0, W.cz, W.hw, W.hd, std::min(hmin, big), big, mat);
    double doorX, doorZ; doorMid(0, 0, W.hw, W.hd, doorX, doorZ);
    rubble(g, W.hw, W.hd, hs, big, doorX, doorZ + W.cz, 0, mat);
    drifts(g, W.hw, W.hd, Dm);
}

// A-04: a windmill (a medieval people's machine at its town's heart): a tower in the people's section, tapering, its cap and its sails
void genWindmill(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const double r = b.hw, H = b.hWall;
    const double h = H * tallShare(Dm, g.U(10));
    const int sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : (g.gv(GV_PLAN) == PF_RECT ? 8 : 0);
    Form& t = g.lathe(1, 0, 0, r, g.sink, g.sink + h, 0, mat, sides); t.p[0] = 1 - 0.32 * h / H; t.pattern = 2; t.broken = h < H * 0.97 ? 0.3 : 0;
    if (h < H * 0.97) return;
    const double rt = r * 0.68, Dt = g.D(MT_TIMBER);
    if (Dt < 0.5) {
        const int rf = c.roof == RF_DOMED || c.roof == RF_VAULTED ? RF_DOMED : RF_CONICAL;
        if (rf == RF_DOMED) g.add(FK_DOME, 0, 0, 0, 0, rt + 0.3, rt + 0.3, g.sink + h, g.sink + h + rt, g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
        else { Form& e = g.lathe(1, 0, 0, rt + 0.4, g.sink + h, g.sink + h + rt * 1.3, g.roofSlot(MT_TIMBER), MT_TIMBER); e.p[0] = 0.05; }
    }
    if (Dt < 0.3 && g.lod < 2) {   // the sails: four arms on the front, crossed
        const double hub = g.sink + h - 0.4, L = H * 0.55;
        for (int k = 0; k < 4; k++) { Form& s = g.box(0, rt + 0.6, PI / 2, 0.9, 0.12, hub, hub + L, g.timberSlot(), MT_TIMBER, 4); s.tilt = 0.4 + k * PI / 2; }   // tipped in the front's plane
    }
}
