// E-03: the forms (A-02): `forms`, the test city, the forms' checks
#include "harness/harness.h"

// ---- A-02: the forms ---------------------------------------------------------------------------------------------------------
// The sheet's samples: every kind and its variants, a few forms each (most one), with the box (metres wide and high) the near view
// frames. The test city: a grid of blocks two kilometres across, towers in the middle, halls and domes round them, low blocks,
// works and masts outside, a viaduct across it and a ring in its plaza, a third of it broken; the bench's city frame and the
// skyline's. Both stand in for what A-03 and T-02 will build: they exercise the renderer, not a people.
struct FormSample { const char* name; std::vector<Form> forms; double w, h; double eye = -1; };   // `eye` over the ground (-1: a quarter of the height)
static Form makeForm(int kind, int sub, double hx, double hz, double y0, double y1, std::initializer_list<double> p = {}, int sides = 0, int slot = 0, int pattern = 0) {
    Form f; f.kind = kind; f.sub = sub; f.hx = hx; f.hz = hz; f.y0 = y0; f.y1 = y1;
    int k = 0; for (double v : p) if (k < 4) f.p[k++] = v;
    f.sides = sides; f.slot = slot; f.pattern = pattern; f.building = 0; f.seed = 0xA02u;
    return f;
}
static Form at(Form f, double x, double z) { f.x = x; f.z = z; return f; }
static std::vector<FormSample> formSamples() {
    std::vector<FormSample> s;
    auto one = [&](const char* name, std::vector<Form> fs, double w, double h) { s.push_back({name, fs, w, h}); };
    one("BOX: A WALL", {makeForm(FK_BOX, 0, 6, 0.5, 0, 4.5)}, 13, 5);
    one("BOX: A BLOCK, WINDOWS", {makeForm(FK_BOX, 0, 9, 7, 0, 32, {}, 0, 0, 1)}, 24, 33);
    one("DOME: HEMISPHERE (C-01)", {makeForm(FK_DOME, 0, 9, 9, 0, 9)}, 19, 10);
    one("DOME: SAUCER ON A DRUM", {makeForm(FK_LATHE, 0, 12, 12, 0, 8), makeForm(FK_DOME, 1, 12, 12, 8, 13, {}, 0, 1)}, 25, 14);
    one("DOME: POINTED", {makeForm(FK_DOME, 2, 9, 9, 0, 16)}, 19, 17);
    one("DOME: ONION ON A DRUM", {makeForm(FK_LATHE, 0, 6, 6, 0, 10), makeForm(FK_DOME, 3, 6, 6, 10, 22, {}, 0, 1)}, 16, 23);
    one("LATHE: CYLINDER, WINDOWS", {makeForm(FK_LATHE, 0, 7, 7, 0, 45, {}, 0, 0, 2)}, 15, 46);
    one("LATHE: FRUSTUM", {makeForm(FK_LATHE, 1, 9, 9, 0, 22, {0.35})}, 19, 23);
    one("LATHE: CONE", {makeForm(FK_LATHE, 1, 6, 6, 0, 12, {0.0})}, 13, 13);
    one("LATHE: SPIRE, A NEEDLE", {makeForm(FK_LATHE, 2, 3.5, 3.5, 0, 50, {1.8})}, 9, 51);
    one("LATHE: COOLING TOWER", {makeForm(FK_LATHE, 3, 30, 30, 0, 90)}, 62, 92);
    one("LATHE: TANK", {makeForm(FK_LATHE, 4, 9, 9, 0, 15, {0.4}, 0, 2)}, 19, 16);
    one("LATHE: CHIMNEY", {makeForm(FK_LATHE, 5, 3, 3, 0, 70, {0.6})}, 9, 71);
    one("LATHE: OCTAGON, WINDOWS", {makeForm(FK_LATHE, 0, 8, 8, 0, 40, {}, 8, 0, 3)}, 18, 41);
    one("PYRAMID", {makeForm(FK_PYRAMID, 0, 30, 30, 0, 38)}, 62, 39);
    one("PYRAMID: ZIGGURAT", {makeForm(FK_PYRAMID, 0, 26, 20, 0, 24, {5, 0.3, 0.6})}, 54, 25);
    one("PYRAMID: SETBACK TOWER", {makeForm(FK_PYRAMID, 0, 16, 13, 0, 120, {4, 0.45, 1.0}, 0, 0, 1)}, 34, 121);
    one("VAULT: ROUND HALL", {makeForm(FK_VAULT, 0, 9, 18, 0, 10, {0.6, 1})}, 40, 11);
    one("VAULT: POINTED ON WALLS", {at(makeForm(FK_BOX, 0, 0.7, 16, 0, 6), -9, 0), at(makeForm(FK_BOX, 0, 0.7, 16, 0, 6), 9, 0), makeForm(FK_VAULT, 1, 9.7, 16, 6, 16, {0.5, 1}, 0, 1)}, 36, 17);
    one("ARCH: A GATE", {makeForm(FK_ARCH, 0, 5, 1.2, 0, 10, {1, 0.32, 5})}, 11, 11);
    one("ARCH: POINTED", {makeForm(FK_ARCH, 1, 4, 1, 0, 12, {1, 0.36, 5})}, 9, 13);
    one("ARCH: ARCADE, AN AQUEDUCT", {makeForm(FK_ARCH, 0, 45, 1.6, 0, 24, {7, 0.34, 12})}, 92, 25);
    one("SHELL: SADDLE", {makeForm(FK_SHELL, 0, 16, 16, 1, 13)}, 34, 14);
    { Form d = makeForm(FK_SHELL, 1, 12, 12, 7, 7, {3.5}, 0, 2); d.tilt = 0.7; one("SHELL: A DISH ON A MAST", {d}, 26, 22); }
    one("TRUSS: A LATTICE MAST", {makeForm(FK_TRUSS, 0, 4.5, 4.5, 0, 65, {0.22}, 4, 2)}, 11, 66);
    one("TRUSS: A GIRDER ON PIERS", {at(makeForm(FK_BOX, 0, 1.5, 1.5, 0, 9), 0, -24), at(makeForm(FK_BOX, 0, 1.5, 1.5, 0, 9), 0, 24), makeForm(FK_TRUSS, 1, 2.2, 26, 9, 13, {}, 0, 2)}, 54, 14);
    { Form m1 = at(makeForm(FK_TRUSS, 0, 1.5, 1.5, 0, 30, {0.3}, 4, 2), -30, 0), m2 = at(makeForm(FK_TRUSS, 0, 1.5, 1.5, 0, 30, {0.3}, 4, 2), 30, 0);
      Form c1 = at(makeForm(FK_CABLE, 0, 0.06, 0.06, 0, 29.5, {30, 0, 29.5, 5}, 0, 2), -30, 0), c2 = at(makeForm(FK_CABLE, 0, 0.06, 0.06, 0, 24, {30, 0, 24, 4}, 0, 2), -30, 0);
      one("CABLES BETWEEN MASTS", {m1, m2, c1, c2}, 64, 31); }
    one("TORUS: A RING LYING", {makeForm(FK_TORUS, 0, 18, 18, 0, 4, {2})}, 40, 5); s.back().eye = 30;
    { Form r = makeForm(FK_TORUS, 0, 20, 20, 0, 45, {2.5}, 0, 1); r.tilt = PI / 2; one("TORUS: A RING STANDING", {r}, 46, 46); }
    // an arc's foot is its centre: the samples are moved so the middle of the arc stands where the sheet looks
    one("SWEEP: A CURVED WALL", {at(makeForm(FK_SWEEP, 0, 25, 25, 0, 9, {2.4, 1.2, 0.7}, 0, 0, 3), -25 * std::sin(1.2), -25 * std::cos(1.2))}, 52, 10); s.back().eye = 16;
    one("SWEEP: AN AMPHITHEATRE", {at(makeForm(FK_SWEEP, 0, 32, 32, 0, 13, {3.6, 9, 1.5, 6}), -16 * std::sin(1.8), -16 * std::cos(1.8))}, 84, 14); s.back().eye = 60;
    one("SWEEP: A BATTERED DAM", {makeForm(FK_SWEEP, 1, 0, 35, 0, 22, {0, 7, 2})}, 72, 23);
    one("BLOB: A GROWN FORM", {makeForm(FK_BLOB, 0, 9, 8, 0, 13, {3, 0.7})}, 24, 22);
    { Form p = makeForm(FK_PLATE, 2, 5, 9, 0, 1.2, {0.6, 0.3, 3, 1.5}); one("PLATE: AN OUTLINED BODY", {p}, 22, 4); s.back().eye = 14; }
    {   // R-412: pales, spear-tipped bars on a low wall, wattle, a balustrade, end to end across the way
        auto across = [&](Form f, double x) { f = at(f, x, 0); f.heading = PI / 2; return f; };
        Form a = across(makeForm(FK_FENCE, 0, 0.03, 1.45, 0, 1.3, {0.2, 0.6, 0, 0.12}), -4.5), b = across(makeForm(FK_FENCE, 1, 0.015, 1.45, 0.5, 1.6, {0.13, 0.25, 0, 0.1}, 0, 3), -1.5);
        Form w = across(makeForm(FK_BOX, 0, 0.2, 1.45, 0, 0.5), -1.5), c = across(makeForm(FK_FENCE, 2, 0.06, 1.45, 0, 1.4, {}, 0, 3), 1.5), d = across(makeForm(FK_FENCE, 3, 0.1, 1.45, 0, 1.0), 4.5);
        one("FENCE: PALES, RAILINGS, WATTLE, A BALUSTRADE", {a, b, w, c, d}, 14, 3); s.back().eye = 10;
    }
    return s;
}
FormPalette testPalette() {   // the sheet's and the city's: concrete, a copper accent, steel, a warm light; four windows
    FormPalette P;
    P.colour[0] = RGB(0.62f, 0.60f, 0.56f); P.own[0] = false;
    P.colour[1] = RGB(0.58f, 0.34f, 0.22f); P.colour[2] = RGB(0.46f, 0.50f, 0.56f); P.colour[3] = RGB(1.0f, 0.8f, 0.5f);
    WindowSpec w;
    w.bay = 3.2; w.storey = 3.5; w.w = 0.45; w.h = 0.5; w.sill = 0.3; w.shape = 0; w.dark = 16; P.windows[0] = w;
    w.bay = 4.0; w.storey = 3.6; w.w = 1.0; w.h = 0.42; w.sill = 0.35; w.shape = 4; w.dark = 20; P.windows[1] = w;
    w.bay = 3.6; w.storey = 5.0; w.w = 0.38; w.h = 0.6; w.sill = 0.2; w.shape = 1; w.dark = 14; P.windows[2] = w;
    w.bay = 3.0; w.storey = 3.4; w.w = 0.4; w.h = 0.45; w.sill = 0.3; w.shape = 0; w.dark = 12; w.lit = true; P.windows[3] = w;
    P.nWindows = 4;
    return P;
}
// a settlement of the given forms (one building, all three levels the same list) placed with its centre at local (x, z)
static void placeForms(SurfaceView& sv, const std::vector<Form>& forms, double x, double z, double heading, double broken) {
    RuinSpec sp; sp.people = 0; sp.heading = heading;
    std::vector<Form> fs = forms;
    double reach = 4;
    const double ch = std::cos(heading), sh = std::sin(heading);
    for (size_t i = 0; i < fs.size(); i++) {
        Form& f = fs[i];
        double lx = f.x, lz = f.z;   // the sample's own frame turned by the heading
        f.x = ch * lx + sh * lz; f.z = -sh * lx + ch * lz;
        if (f.kind == FK_CABLE) { double ex = f.p[0], ez = f.p[1]; f.p[0] = ch * ex + sh * ez; f.p[1] = -sh * ex + ch * ez; }
        f.heading += heading; f.building = 0;
        // broken: what holds another form up stays whole (a drum under its dome, the walls under a vault, a girder's piers)
        bool holds = false;
        for (const Form& o : forms) if (&o != &forms[i] && o.y0 > 0.5 && o.kind != FK_CABLE && std::fabs(o.y0 - forms[i].y1) < 0.6) holds = true;
        if (broken > 0 && !holds) { f.broken = broken; f.seed = 0xB0u + (uint32_t)i * 7919u; }
        reach = std::max(reach, std::hypot(f.x, f.z) + formReach(f));
    }
    Building b; b.kind = BK_HALL; b.x = 0; b.z = 0; b.heading = heading; b.hw = reach * 0.7; b.hd = reach * 0.7;
    sp.buildings.push_back(b); sp.size = reach;
    std::vector<Form> pieces[3] = {fs, fs, fs};
    sv.testPlaceSettlement(x, z, sp, pieces);
}
// the test city: blocks on a grid, the tall in the middle; `pieces` the same list at every level (a form's own detail goes with
// its distance); returns the buildings
static int buildTestCity(uint64_t seed, RuinSpec& sp, std::vector<Form>& forms) {
    sp = RuinSpec(); sp.people = 0; forms.clear();
    Rng rng(seed);
    const double pitch = 52; const int half = 17;
    auto addBuilding = [&](double x, double z, double heading, double hw, double hd, const std::vector<Form>& fs) {
        int bi = (int)sp.buildings.size();
        Building b; b.kind = BK_HOUSE; b.x = x; b.z = z; b.heading = heading; b.hw = hw; b.hd = hd; b.seed = rng.next();
        sp.buildings.push_back(b);
        bool broken = rng.chance(0.3); double degree = 0.3 + 0.6 * rng.uni();
        const double ch = std::cos(heading), sh = std::sin(heading);
        for (Form f : fs) {
            double lx = f.x, lz = f.z; f.x = x + ch * lx + sh * lz; f.z = z - sh * lx + ch * lz; f.heading += heading; f.building = bi; f.seed = (uint32_t)rng.next();
            if (f.kind == FK_CABLE) { double ex = f.p[0], ez = f.p[1]; f.p[0] = x + ch * ex + sh * ez; f.p[1] = z - sh * ex + ch * ez; }
            if (broken) f.broken = degree;
            forms.push_back(f);
        }
    };
    for (int gz = -half; gz <= half; gz++)
        for (int gx = -half; gx <= half; gx++) {
            double cx = gx * pitch, cz = gz * pitch, rho = std::hypot(cx, cz);
            if (rho > half * pitch) continue;
            if (std::fabs(cx) < 40 && std::fabs(cz) < 40) {   // the plaza: a ring standing in it
                Form r = makeForm(FK_TORUS, 0, 26, 26, 0, 55, {3}, 0, 3); r.tilt = PI / 2;
                addBuilding(cx, cz, 0.4, 28, 4, {r});
                continue;
            }
            if (gz == 3) continue;   // the viaduct's line (below)
            const double heading = (rng.uni() - 0.5) * 0.2;
            if (rho < 260) {   // towers
                int n = 1 + rng.irange(2);
                for (int k = 0; k < n; k++) {
                    double ox = n == 1 ? 0 : (k == 0 ? -14 : 14), H = 90 + 360 * rng.uni() * (1 - rho / 330);   // supertall in the middle
                    double R = 8 + 6 * rng.uni();
                    std::vector<Form> fs;
                    int pick = rng.irange(4);
                    if (pick == 0) fs.push_back(at(makeForm(FK_LATHE, 0, R, R, 0, H, {}, 0, 0, 2), 0, 0));
                    else if (pick == 1) fs.push_back(at(makeForm(FK_LATHE, 0, R, R, 0, H, {}, rng.chance(0.5) ? 4 : 8, 0, 1), 0, 0));
                    else if (pick == 2) fs.push_back(makeForm(FK_PYRAMID, 0, R * 1.6, R * 1.3, 0, H, {(double)(3 + rng.irange(3)), 0.45, 1.0}, 0, 0, 1));
                    else fs.push_back(makeForm(FK_BOX, 0, R, R * 0.8, 0, H * 0.7, {}, 0, 0, 1));
                    if (rng.chance(0.35)) fs.push_back(makeForm(FK_LATHE, 2, std::min(R, 4.0), std::min(R, 4.0), fs.back().y1, fs.back().y1 + 20 + 40 * rng.uni(), {1.6}, 0, 2));
                    else if (rng.chance(0.2)) fs.push_back(makeForm(FK_DOME, rng.chance(0.5) ? 2 : 3, R * 0.8, R * 0.8, fs.back().y1, fs.back().y1 + R * 1.4, {}, 0, 1));
                    for (Form& f : fs) f.x += ox;
                    addBuilding(cx + ox * 0, cz, heading, R * 1.6 + std::fabs(ox), R * 1.4, fs);
                }
            } else if (rho < 600) {   // halls, domes, mid-rise blocks
                int pick = rng.irange(5);
                if (pick <= 1) {
                    double hx = 10 + 12 * rng.uni(), hz = 10 + 12 * rng.uni(), H = 15 + 30 * rng.uni();
                    addBuilding(cx, cz, heading, hx, hz, {makeForm(FK_BOX, 0, hx, hz, 0, H, {}, 0, 0, 1 + rng.irange(3))});
                } else if (pick == 2) {
                    double R = 9 + 8 * rng.uni(), H = 8 + 8 * rng.uni();
                    addBuilding(cx, cz, heading, R, R, {makeForm(FK_LATHE, 0, R, R, 0, H, {}, 0, 0, 3), makeForm(FK_DOME, 1 + rng.irange(3), R, R, H, H + R * (0.6 + 0.6 * rng.uni()), {}, 0, 1)});
                } else if (pick == 3) {
                    double hx = 10 + 6 * rng.uni(), hz = 20 + 10 * rng.uni();
                    addBuilding(cx, cz, heading, hx, hz, {makeForm(FK_VAULT, rng.irange(3), hx, hz, 0, hx * (0.9 + 0.3 * rng.uni()), {0.6, 1})});
                } else {
                    double R = 7 + 5 * rng.uni(), H = 40 + 40 * rng.uni();
                    addBuilding(cx, cz, heading, R, R, {makeForm(FK_LATHE, 0, R, R, 0, H, {}, 6, 0, 2)});
                }
            } else {   // the outer blocks: low houses, works, masts
                int pick = rng.irange(10);
                if (pick <= 4) {
                    int n = 2 + rng.irange(3);
                    for (int k = 0; k < n; k++) {
                        double hx = 5 + 5 * rng.uni(), hz = 5 + 5 * rng.uni(), H = 6 + 8 * rng.uni(), ox = (k - (n - 1) * 0.5) * 22;
                        addBuilding(cx + ox, cz, heading, hx, hz, {makeForm(FK_BOX, 0, hx, hz, 0, H, {}, 0, 0, 1)});
                    }
                } else if (pick == 5) { double R = 7 + 5 * rng.uni(); addBuilding(cx, cz, heading, R, R, {makeForm(FK_LATHE, 4, R, R, 0, R * 1.8, {0.4}, 0, 2)}); }
                else if (pick == 6) { addBuilding(cx, cz, heading, 4, 4, {makeForm(FK_LATHE, 5, 3, 3, 0, 40 + 40 * rng.uni(), {0.6})}); }
                else if (pick == 7) {
                    Form m1 = at(makeForm(FK_TRUSS, 0, 2, 2, 0, 40, {0.3}, 4, 2), -20, 0), m2 = at(makeForm(FK_TRUSS, 0, 2, 2, 0, 40, {0.3}, 4, 2), 20, 0);
                    Form c = at(makeForm(FK_CABLE, 0, 0.08, 0.08, 0, 39, {20, 0, 39, 4}, 0, 2), -20, 0);
                    addBuilding(cx, cz, heading, 22, 3, {m1, m2, c});
                } else if (pick == 8) { double R = 18 + 10 * rng.uni(); addBuilding(cx, cz, heading, R, R, {makeForm(FK_LATHE, 3, R, R, 0, R * 2.8)}); }
                else { double R = 6 + 4 * rng.uni(); addBuilding(cx, cz, heading, R, R, {makeForm(FK_BLOB, 0, R, R * 0.9, 0, R * 1.4, {(double)rng.irange(4), 0.7})}); }
            }
        }
    // the viaduct across the city: an arcade along its own line
    addBuilding(0, 3 * pitch, 0, half * pitch, 4, {makeForm(FK_ARCH, 0, half * pitch * 0.95, 2, 0, 26, {30, 0.36, 14})});
    // the amphitheatre in the east
    addBuilding(-420, -380, 0.3, 40, 40, {makeForm(FK_SWEEP, 0, 34, 34, 0, 14, {TAU, 9, 1.5, 6})});
    double R = 0; for (const Building& b : sp.buildings) R = std::max(R, std::hypot(b.x, b.z) + std::max(b.hw, b.hd));
    sp.size = R;
    return (int)sp.buildings.size();
}
void placeCity(SurfaceView& sv, double x, double z, uint64_t seed, int* nBuildings, int* nForms) {
    RuinSpec sp; std::vector<Form> forms;
    int nb = buildTestCity(seed, sp, forms);
    std::vector<Form> pieces[3] = {forms, forms, forms};
    sv.testPlaceSettlement(x, z, sp, pieces);
    if (nBuildings) *nBuildings = nb;
    if (nForms) *nForms = (int)forms.size();
}
// the free camera at the explorer's place `eyeH` over the ground, looking at local (tx, ty, tz)
void formsCamera(SurfaceView& sv, double eyeH, double tx, double ty, double tz, double t) {
    sv.enterFreeCam();
    double ex = sv.player.x, ez = sv.player.z, ey = sv.site.surfaceHeight(ex, ez) + eyeH;
    sv.freePos = Vec3(ex, ey, ez);
    sv.freeYaw = std::atan2(tx - ex, tz - ez);
    sv.freePitch = std::atan2(ty - ey, std::hypot(tx - ex, tz - ez));
    Input in; sv.update(1.0 / 30, in, t, false);
}

// A-02: the forms' unit checks (also `forms check`, alone)
void formsChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    {   // the forms (peoples/forms.h): every kind whole and broken at the three levels, walked into, the city frame the serial loop's,
        // the peoples' banks
        std::vector<FormSample> samples = formSamples();
        int meshes = 0, empty = 0, nonFinite = 0, outside = 0, unbroken = 0, kinds = 0; std::string firstBad;
        bool seenKind[FK_COUNT] = {};
        FormMesh m, mw;
        for (const FormSample& S : samples) for (const Form& f0 : S.forms) for (int lod = 0; lod < 3; lod++) {
            seenKind[f0.kind] = true;
            const Vec3 foot(f0.x, 100, f0.z);
            for (int br = 0; br < 2; br++) {
                Form f = f0; if (br) { f.broken = 0.6; f.seed = 0x5EEDu; }
                formMesh(f, lod, foot, 1.0, m, 10.0);
                meshes++;
                if (m.polys.empty() && m.lines.empty()) { empty++; if (firstBad.empty()) firstBad = fmt("%s lod %d%s empty", S.name, lod, br ? " broken" : ""); continue; }
                double reach = (f.kind == FK_CABLE ? std::hypot(f.p[0] - f.x, f.p[1] - f.z) : formReach(f)) * 1.25 + 2, top = formTop(f) + 2;
                auto inBounds = [&](const Vec3& p) {
                    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) { nonFinite++; return; }
                    if (std::hypot(p.x - foot.x, p.z - foot.z) > reach || p.y - 100 > top || p.y - 100 < -3) {
                        outside++;
                        if (firstBad.empty())
                            firstBad = fmt("%s lod %d%s: a corner at %.1f m out, %.1f m up (reach %.1f, top %.1f)", S.name, lod, br ? " broken" : "", std::hypot(p.x - foot.x, p.z - foot.z), p.y - 100,
                                    reach, top);
                    }
                };
                for (const FormMesh::V& v : m.v) inBounds(v.p);
                for (const FormMesh::Line& l : m.lines) { inBounds(l.a); inBounds(l.b); }
                if (!br) mw = m;
                else {
                    bool same = m.v.size() == mw.v.size() && m.lines.size() == mw.lines.size();
                    for (size_t k = 0; same && k < m.v.size(); k++) if (length2(m.v[k].p - mw.v[k].p) > 1e-12) same = false;
                    for (size_t k = 0; same && k < m.lines.size(); k++) if (length2(m.lines[k].a - mw.lines[k].a) + length2(m.lines[k].b - mw.lines[k].b) > 1e-12) same = false;
                    if (same) { unbroken++; if (firstBad.empty()) firstBad = fmt("%s lod %d: broken is the whole", S.name, lod); }
                }
            }
        }
        for (int k = 0; k < FK_COUNT; k++) if (seenKind[k]) kinds++;
        Form box = makeForm(FK_BOX, 0, 2, 0.4, 0, 3), dome = makeForm(FK_DOME, 0, 4, 4, 3, 7);
        formMesh(box, 0, Vec3(0, 0, 0), 1.0, m); size_t boxPolys = m.polys.size();
        formMesh(dome, 2, Vec3(0, 0, 0), 0.0, m); size_t domePolys = m.polys.size();
        check("forms: every kind whole and broken (A-02)", kinds == FK_COUNT && empty == 0 && nonFinite == 0 && outside == 0 && unbroken == 0 && boxPolys == 5 && domePolys == 40,
                fmt("%d of %d kinds in %zu samples, %d meshes: %d empty, %d corners not finite, %d out of their bounds, %d broken ones the same as the whole; C-01's box %zu faces, its hemisphere %zu "
                    "quads%s%s",
                        kinds, FK_COUNT, samples.size(), meshes, empty, nonFinite, outside, unbroken, boxPolys, domePolys, firstBad.empty() ? "" : "; ", firstBad.c_str()));
        // walking into each: the explorer starts eight metres short of a sample (facing it) and walks straight at it for twenty seconds; a
        // form stops it outside every disc it stands on, while an arch's opening, the space between two masts or a lattice's legs and the
        // ground under a saddle let it through
        SpaceRenderer srW; StarNeighborhood nbW; StarSystem sysW; SurfaceView svW; double tW = 0;
        int walks = 0, walkOk = 0, passes = 0; std::string walkBad;
        if (setupSceneForType(PT_DESERT, 15, 40 * DEG, 1.3, 0.0, svW, sysW, nbW, tW, SCENE_ANY)) {
            svW.flocks.clear(); svW.critters.clear();
            const double yaw = svW.player.yaw, px0 = svW.player.x, pz0 = svW.player.z;
            static const char* const THROUGH[] = {"ARCH: A GATE", "ARCH: POINTED", "ARCH: ARCADE, AN AQUEDUCT", "SHELL: SADDLE", "TRUSS: A LATTICE MAST", "CABLES BETWEEN MASTS",
                    "VAULT: POINTED ON WALLS"};   // the last a hall open at its ends
            for (const FormSample& S : samples)
                for (int br = 0; br < 2; br++) {
                    bool through = false; for (const char* n : THROUGH) if (std::string(n) == S.name) through = true;
                    double reach = 0; for (const Form& f : S.forms) if (f.kind != FK_CABLE) reach = std::max(reach, std::hypot(f.x, f.z) + formReach(f));
                    const double start = reach + 8, fx = px0 + std::sin(yaw) * start, fz = pz0 + std::cos(yaw) * start;
                    svW.testClearPlaced();
                    placeForms(svW, S.forms, fx, fz, yaw + PI, br ? 0.6 : 0.0);
                    svW.player = Player(); svW.player.x = px0; svW.player.z = pz0; svW.player.y = svW.site.surfaceHeight(px0, pz0); svW.player.yaw = yaw; svW.player.onGround = true;
                    Input in;
                    double pen = 0, gapMin = 1e9;   // every frame: how far into a disc of the form (never more than a few centimetres), how near one came
                    std::vector<SurfaceView::Collider> cols;
                    for (int k = 0; k < 600; k++) {
                        in.down[KEY_W] = true; svW.update(1.0 / 30, in, tW + k / 30.0, true); in.newFrame();
                        svW.collectColliders(svW.player.x, svW.player.z, cols);
                        for (const SurfaceView::Collider& c : cols) {
                            if (c.kind != 3 || svW.player.y > c.y1 - 0.1 || svW.player.y + 1.6 < c.y0) continue;
                            double g = std::hypot(svW.player.x - c.x, svW.player.z - c.z) - (c.r + 0.35);
                            pen = std::max(pen, -g); gapMin = std::min(gapMin, g);
                        }
                    }
                    const double ax = svW.player.x - fx, az = svW.player.z - fz, along = ax * std::sin(yaw) + az * std::cos(yaw);
                    // a form is touched (the explorer stopped against it or slid round it) and never entered; an opening is walked through;
                    // one stood on (a bank, a heap, an amphitheatre's tiers) stops the explorer at a side more than a step high or is climbed
                    bool walkable = true; for (const Form& f : S.forms) walkable = walkable && formWalkable(f);
                    bool ok = walkable ? (along < 0 || svW.player.y - svW.site.groundHeight(svW.player.x, svW.player.z) > 0.3) : pen < 0.06 && (through ? along > 2 : gapMin < 0.15);
                    walks++; if (ok) walkOk++; if (through && ok) passes++;
                    if (!ok && walkBad.empty())
                        walkBad = fmt(
                                "%s%s: %.1f m from its middle along the way (reach %.1f), %.2f m into a disc at worst, the nearest %.2f m off", S.name, br ? " broken" : "", along, reach, pen, gapMin);
                }
        }
        check("forms: walking into each one (A-02)", walks > 0 && walkOk == walks && passes == 14,
                fmt("%d walks (each sample whole and broken), %d as they should (touched and never entered; through a gate, an arcade, a hall, between masts or a lattice's legs, under a saddle: "
                    "%d)%s%s",
                        walks, walkOk, passes, walkBad.empty() ? "" : "; ", walkBad.c_str()));
        // the peoples' banks: a people that built of its ground's own material has its structure's ramp that bank's
        {
            svW.palettes[0].own[0] = true; svW.palettes[0].tint[0] = 0;   // a people of its ground's own stone, untinted (a people's own is tinted toward its base colour)
            Framebuffer fbP; Input inP; svW.testClearPlaced(); svW.update(1.0 / 30, inP, tW, false); svW.render(fbP, tW, nbW.stars, srW, 1.0);
            const int own = svW.cultures[0].family == 1 ? 9 : 0;
            bool same = svW.palettes[0].own[0] && std::memcmp(&fbP.pal[FORM_BANKS[0][0] * 64 * 3], &fbP.pal[own * 64 * 3], 64 * 3) == 0;
            bool lightSet = fbP.pal[(FORM_BANKS[0][3] * 64 + 46) * 3] > 0 || fbP.pal[(FORM_BANKS[0][3] * 64 + 46) * 3 + 1] > 0;
            check("forms: the peoples' banks (A-02)", same && lightSet,
                    fmt("the first people's structure in bank %d %s bank %d's ramp (its walls the %s), its light's ramp %s", FORM_BANKS[0][0], same ? "is" : "is NOT", own, own == 9 ? "sand" : "rock",
                            lightSet ? "set" : "NOT SET"));
        }
        // the city frame at 2x drawn by the workers is the serial loop's, pixel for pixel
        {
            int saved = g_testScale; setFramebufferScale(2);
            SpaceRenderer srC; StarNeighborhood nbC; StarSystem sysC; SurfaceView svC; double tC = 0;
            uint64_t hw = 0, hs = 0; int polys = 0; double msW = 0, msS = 0;
            if (setupSceneForType(PT_DESERT, 15, 40 * DEG, 1.3, 0.0, svC, sysC, nbC, tC, SCENE_TOWN)) {
                svC.palettes[0] = testPalette(); svC.flocks.clear(); svC.critters.clear();
                double yaw = svC.player.yaw, cx = svC.player.x + std::sin(yaw) * 1250, cz = svC.player.z + std::cos(yaw) * 1250;
                placeCity(svC, cx, cz, 0xC17E5EEDULL);
                formsCamera(svC, 12.0, cx, svC.site.surfaceHeight(cx, cz) + 30, cz, tC);
                Framebuffer fbC;
                for (int i = 0; i < 2; i++) svC.render(fbC, tC, nbC.stars, srC, 1.0);
                double t0 = nowSec(); svC.render(fbC, tC, nbC.stars, srC, 1.0); msW = svC.lastRuinMs; (void)t0;
                hw = fnv(1469598103934665603ULL, fbC.idx.data(), fbC.idx.size() * sizeof(Pix)); polys = svC.lastFormPolys;
                parallelThreadsOverride() = 1;
                svC.render(fbC, tC, nbC.stars, srC, 1.0); msS = svC.lastRuinMs;
                hs = fnv(1469598103934665603ULL, fbC.idx.data(), fbC.idx.size() * sizeof(Pix));
                parallelThreadsOverride() = 0;
            }
            setFramebufferScale(saved);
            check("forms: the city frame is the serial loop's (A-02)", hw != 0 && hw == hs && polys > 5000,
                  fmt("%d polygons; %d threads %016llx in %.2f ms, one %016llx in %.2f ms", polys, parallelThreads(), (unsigned long long)hw, msW, (unsigned long long)hs, msS));
        }
    }
}

// A-02: `forms`: the sheet, the test city and the skyline (the samples and the city are defined with the view's helpers, above)
int runForms(int argc, char** argv) {
    std::string what = argc > 2 ? argv[2] : "sheet";
    mkdir("shots/tests", 0755);
    const std::string world = argc > 4 ? argv[4] : "desert";
    const int type = world == "felisian" ? PT_FELISIAN : (world == "airless" ? PT_CRATERED : PT_DESERT);   // airless: clear air (the renderer's skyline without the haze)
    if (what == "sheet") {   // `forms sheet [scale]`: every sample whole and broken, near (and its middle and far meshes up close), at 600 m and 2.5 km
        int scale = argc > 3 ? atoi(argv[3]) : 1;
        setFramebufferScale(scale);
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        if (!setupSceneForType(type, 15, 40 * DEG, 1.3, 0.0, sv, sys, nb, t, SCENE_ANY)) { printf("no body\n"); return 1; }
        sv.palettes[0] = testPalette(); sv.flocks.clear(); sv.critters.clear();
        printf("forms sheet at %dx on %s (%s), the sun at %.0f degrees\n", scale, sys.bodies[sv.site.body].name.c_str(), KINDS[sys.bodies[sv.site.body].type].name, sv.env.sun.altitude / DEG);
        std::vector<FormSample> samples = formSamples();
        const int CW = 120, CH = 80, cols = 8, perSheet = 6;   // a cell in 1x pixels
        const double yaw0 = sv.player.yaw;
        std::vector<uint32_t> rgbBuf((size_t)FBW * FBH);
        Framebuffer fb;
        for (int g0 = 0, sheetNo = 0; g0 < (int)samples.size(); g0 += perSheet, sheetNo++) {
            int rows = std::min(perSheet, (int)samples.size() - g0);
            int W = cols * CW * scale, H = (rows * (CH + 10) + 12) * scale;
            std::vector<uint32_t> sheet((size_t)W * H, rgb(16, 16, 20));
            RGBCanvas cv{sheet.data(), W, H, scale};
            const char* const heads[cols] = {"NEAR", "NEAR, BROKEN", "LOD 1 CLOSE", "LOD 2 CLOSE", "600 M (X2)", "600 M BROKEN", "2.5 KM (X2)", "2.5 KM BROKEN"};
            for (int c = 0; c < cols; c++) drawText(cv, c * CW + 3, 2, heads[c], rgb(200, 200, 160));   // the canvas's scale sizes the glyphs
            for (int r = 0; r < rows; r++) {
                const FormSample& S = samples[g0 + r];
                double size = std::max(S.h * 3.4, S.w * 2.4) + 10;
                for (int c = 0; c < cols; c++) {
                    double D = c < 4 ? size : (c < 6 ? 600 : 2500), broken = (c == 1 || c == 5 || c == 7) ? 0.6 : 0.0;
                    int lodForce = c == 2 ? 1 : (c == 3 ? 2 : -1);
                    double fx = sv.player.x + std::sin(yaw0) * D, fz = sv.player.z + std::cos(yaw0) * D;
                    sv.testClearPlaced();
                    placeForms(sv, S.forms, fx, fz, yaw0 + PI + 0.55, broken);
                    sv.testForceLod = lodForce;
                    double gy = sv.site.surfaceHeight(fx, fz);
                    formsCamera(sv, c < 4 ? (S.eye > 0 ? S.eye : 1.7 + 0.25 * S.h) : 40.0, fx, gy + 0.5 * S.h, fz, t);
                    sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(rgbBuf.data(), 1.0);
                    double sx = FBW * 0.5, sy = FBH * 0.5; sv.projectPoint(fx, gy + 0.5 * S.h, fz, sx, sy);
                    const bool mag = c >= 4;
                    const int cw = (mag ? CW / 2 : CW) * scale, chh = (mag ? CH / 2 : CH) * scale;
                    int ox = (int)sx - cw / 2, oy = (int)sy - chh / 2;
                    int dx0 = c * CW * scale, dy0 = (12 + r * (CH + 10)) * scale;
                    for (int y = 0; y < CH * scale; y++)
                        for (int x = 0; x < CW * scale; x++) {
                            int px = ox + (mag ? x / 2 : x), py = oy + (mag ? y / 2 : y);
                            uint32_t v = (px >= 0 && py >= 0 && px < FBW && py < FBH) ? rgbBuf[(size_t)py * FBW + px] : rgb(0, 0, 0);
                            sheet[(size_t)(dy0 + y) * W + dx0 + x] = v;
                        }
                    if (c == 0) printf("  %-28s near %4.0f m: %d polygons, %d lines\n", S.name, D, sv.lastFormPolys, sv.lastFormLines);
                }
                drawTextShadow(cv, 3, 12 + r * (CH + 10) + CH + 1, S.name, rgb(230, 235, 240), rgb(0, 0, 0));
            }
            sv.testForceLod = -1;
            std::string fn = fmt("shots/tests/forms_%dx_%d.png", scale, sheetNo);
            writePNG(fn.c_str(), sheet.data(), W, H);
            printf("  -> %s\n", fn.c_str());
        }
        return 0;
    }
    if (what == "city" || what == "skyline") {   // `forms city [scale]`: the test city from its edge; `forms skyline [scale] [felisian] [km]`
        int scale = argc > 3 ? atoi(argv[3]) : 2;
        double km = argc > 5 ? atof(argv[5]) : 30;
        setFramebufferScale(scale);
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        // the peoples' own worlds: the civilisation scenes' sites (on land, near a town), the sun at 35-40 degrees
        // (the skyline on the felisian type takes its default world, whose air is clear: the civilisation scene's world is a hazy one)
        if (!setupSceneForType(type, type == PT_FELISIAN ? 12 : 15, (type == PT_FELISIAN ? 35 : 40) * DEG, 1.3, 0.0, sv, sys, nb, t,
                    (type == PT_CRATERED || (type == PT_FELISIAN && what == "skyline")) ? SCENE_ANY : SCENE_TOWN)) {
            printf("no body\n");
            return 1;
        }
        sv.palettes[0] = testPalette(); sv.flocks.clear(); sv.critters.clear();
        double yaw0 = sv.player.yaw;
        const double D = what == "city" ? 1250 : km * 1000;
        const double eyeH = what == "city" ? 12.0 : 60.0;
        if (what == "skyline") {   // the skyline from a vantage: the highest dry ground within six kilometres
            DrainageOff off;
            double bx = sv.player.x, bz = sv.player.z, bh = -1e18;
            for (int j = -30; j <= 30; j++)
                for (int i = -30; i <= 30; i++) {
                    double x = sv.player.x + i * 200, z = sv.player.z + j * 200;
                    TerrainVertex tv = sv.site.sampleAt(x, z, 64);
                    if (tv.water > -1e8 && tv.h < tv.water + 1) continue;
                    if (tv.h > bh) {
                        bh = tv.h;
                        bx = x;
                        bz = z;
                    }
                }
            sv.player.x = bx; sv.player.z = bz; sv.player.y = sv.site.surfaceHeight(bx, bz);
            Input in0; sv.update(1.0 / 30, in0, t, false);
        }
        int found = -1;
        {   // the city on dry land and in sight: turn until its middle and four points a kilometre out are above the water and the ground
            // between does not rise over the line from the eye to 150 m over the city's middle (the world's curve counted)
            DrainageOff off;
            const double ey = sv.site.surfaceHeight(sv.player.x, sv.player.z) + eyeH, inv2R = 1.0 / (2 * sv.site.R);
            for (int k = 0; k < 48; k++) {
                double y = sv.player.yaw + k * TAU / 48, sy = std::sin(y), cy = std::cos(y), x0 = sv.player.x + sy * D, z0 = sv.player.z + cy * D;
                bool ok = true;
                for (int q = 0; q < 5 && ok; q++) {
                    double a = q * PI / 2;
                    TerrainVertex tv = sv.site.sampleAt(x0 + (q ? 1000 * std::sin(a) : 0), z0 + (q ? 1000 * std::cos(a) : 0), 64);
                    if (tv.water > -1e8 && tv.h < tv.water + 1) ok = false;
                }
                if (ok && what == "skyline") {
                    double top = sv.site.sampleAt(x0, z0, 64).h + 150 - D * D * inv2R;
                    for (int j = 1; j < 240 && ok; j++) {
                        double d = D * j / 240, gh = sv.site.sampleAt(sv.player.x + sy * d, sv.player.z + cy * d, 64).h + 20 - d * d * inv2R;
                        if (gh > ey + (top - ey) * d / D) ok = false;
                    }
                }
                if (ok) { yaw0 = y; found = k; break; }
            }
        }
        double cx = sv.player.x + std::sin(yaw0) * D, cz = sv.player.z + std::cos(yaw0) * D;
        int nbld = 0, nf = 0; double tb = nowSec();
        placeCity(sv, cx, cz, 0xC17E5EEDULL, &nbld, &nf);
        double buildMs = (nowSec() - tb) * 1000;
        double gy = sv.site.surfaceHeight(cx, cz);
        formsCamera(sv, eyeH, cx, gy + (what == "city" ? 30.0 : 60.0), cz, t);
        Framebuffer fb;
        for (int i = 0; i < 3; i++) { sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); }
        double t0 = nowSec(); const int N = 20;
        for (int i = 0; i < N; i++) { Input in; sv.update(1.0 / 30, in, t, false); sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); }
        double ms = (nowSec() - t0) * 1000 / N;
        uint64_t h = fnv(1469598103934665603ULL, fb.idx.data(), fb.idx.size() * sizeof(Pix));
        std::string fn = what == "city" ? fmt("shots/tests/forms_city_%s_%dx.png", world.c_str(), scale) : fmt("shots/tests/forms_skyline_%s_%.0fkm_%dx.png", world.c_str(), km, scale);
        saveFB(fb, fn.c_str());
        printf("  the view: %s (turned %d of 48), the eye %.0f m over the ground at %.0f m\n", found >= 0 ? "dry land in sight" : "NO clear direction found", found, eyeH,
                sv.site.surfaceHeight(sv.player.x, sv.player.z));
        printf("%s at %dx on %s: %d buildings, %d forms (built in %.1f ms), %.1f km away; %.2f ms a frame (ruins %.2f ms: %d items, %d polygons, %d lines, %d silhouettes), fog %.0f m; frame hash "
               "%016llx (threads %d) -> %s\n",
                what.c_str(), scale, sys.bodies[sv.site.body].name.c_str(), nbld, nf, buildMs, D / 1000, ms, sv.lastRuinMs, sv.lastRuinElems, sv.lastFormPolys, sv.lastFormLines, sv.lastSilhouettes,
                sv.env.fogDistance, (unsigned long long)h, parallelThreads(), fn.c_str());
        return 0;
    }
    if (what == "check") {   // `forms check`: the unit's A-02 checks alone
        int fails = 0;
        formsChecks(Check{nullptr, &fails});
        return fails;
    }
    if (what == "stability") {   // `stability skyline [scale]`: the test city's skyline while the explorer walks toward it (1.4 m a frame, 16 frames): the
        // desert at 15 km (the haze's) and the airless world at 30 km; the mean change of a pixel's light a frame over the city and the share
        // of its pixels jumping more than 40 (B-309's measure), beside the same over the empty horizon
        int scale = argc > 3 ? atoi(argv[3]) : 2;
        setFramebufferScale(scale);
        int fails = 0;
        for (int w = 0; w < 2; w++) {
            const int wt = w == 0 ? PT_DESERT : PT_CRATERED; const double D = w == 0 ? 15000 : 30000;
            SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
            if (!setupSceneForType(wt, 15, 40 * DEG, 1.3, 0.0, sv, sys, nb, t, wt == PT_CRATERED ? SCENE_ANY : SCENE_TOWN)) { printf("no body\n"); return 1; }
            sv.palettes[0] = testPalette(); sv.flocks.clear(); sv.critters.clear();
            {   // a summit within six kilometres, and the way to the city clear of the ground (as `forms skyline`)
                DrainageOff off;
                double bx = sv.player.x, bz = sv.player.z, bh = -1e18;
                for (int j = -30; j <= 30; j++)
                    for (int i = -30; i <= 30; i++) {
                        double x = sv.player.x + i * 200, z = sv.player.z + j * 200;
                        TerrainVertex tv = sv.site.sampleAt(x, z, 64);
                        if (tv.water > -1e8 && tv.h < tv.water + 1) continue;
                        if (tv.h > bh) {
                            bh = tv.h;
                            bx = x;
                            bz = z;
                        }
                    }
                sv.player.x = bx; sv.player.z = bz;
            }
            const double ey = sv.site.surfaceHeight(sv.player.x, sv.player.z) + 60, inv2R = 1.0 / (2 * sv.site.R);
            double yaw = sv.player.yaw;
            {
                DrainageOff off;
                for (int k = 0; k < 48; k++) {
                    double y = sv.player.yaw + k * TAU / 48, sy = std::sin(y), cy = std::cos(y), x0 = sv.player.x + sy * D, z0 = sv.player.z + cy * D;
                    bool ok = true;
                    double top = sv.site.sampleAt(x0, z0, 64).h + 150 - D * D * inv2R;
                    for (int j = 1; j < 240 && ok; j++) {
                        double d = D * j / 240, gh = sv.site.sampleAt(sv.player.x + sy * d, sv.player.z + cy * d, 64).h + 20 - d * d * inv2R;
                        if (gh > ey + (top - ey) * d / D) ok = false;
                    }
                    if (ok) {
                        yaw = y;
                        break;
                    }
                }
            }
            const double cx = sv.player.x + std::sin(yaw) * D, cz = sv.player.z + std::cos(yaw) * D;
            placeCity(sv, cx, cz, 0xC17E5EEDULL);
            const int N = 16;
            Framebuffer fb; std::vector<uint32_t> px((size_t)FBW * FBH), prev;
            std::vector<double> heat((size_t)FBW * FBH, 0); std::vector<int> pops((size_t)FBW * FBH, 0);
            auto lum = [](uint32_t c) { return 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255); };
            double sxC = 0, syC = 0;
            for (int k = 0; k < N; k++) {
                sv.player.x += std::sin(yaw) * (k ? 1.4 : 0); sv.player.z += std::cos(yaw) * (k ? 1.4 : 0);
                formsCamera(sv, 60.0, cx, sv.site.surfaceHeight(cx, cz) + 60, cz, t + k / 30.0);
                sv.render(fb, t + k / 30.0, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(px.data(), 1.0);
                if (k == 0) sv.projectPoint(cx, sv.site.surfaceHeight(cx, cz) + 60, cz, sxC, syC);
                if (!prev.empty()) for (size_t i = 0; i < px.size(); i++) { double d = std::fabs(lum(px[i]) - lum(prev[i])); heat[i] += d; if (d > 40) pops[i]++; }
                prev = px;
            }
            // the city's box: 2.2 km wide at its distance, from the ground to 600 m; the control box beside it, as wide, off to the side
            const double halfW = 1100.0 / D * sv.proj.f, hTop = 600.0 / D * sv.proj.f;
            auto boxStats = [&](double x0, double x1, double y0, double y1, double& mean, double& popPct) {
                double sum = 0; long n = 0, pp = 0;
                for (int y = std::max(0, (int)y0); y < std::min(FBH, (int)y1); y++)
                    for (int x = std::max(0, (int)x0); x < std::min(FBW, (int)x1); x++) {
                        sum += heat[(size_t)y * FBW + x] / (N - 1);
                        pp += pops[(size_t)y * FBW + x];
                        n++;
                    }
                mean = n ? sum / n : 0; popPct = n ? 100.0 * pp / ((double)n * (N - 1)) : 0;
            };
            double cm, cp, km, kp;
            boxStats(sxC - halfW, sxC + halfW, syC - hTop, syC + 20 * FB_SCALE, cm, cp);
            boxStats(sxC + 3 * halfW, sxC + 5 * halfW, syC - hTop, syC + 20 * FB_SCALE, km, kp);
            std::vector<uint32_t> sheet((size_t)FBW * 2 * FBH);
            for (int y = 0; y < FBH; y++)
                for (int x = 0; x < FBW; x++) {
                    sheet[(size_t)y * FBW * 2 + x] = prev[(size_t)y * FBW + x];
                    int v = (int)std::min(255.0, heat[(size_t)y * FBW + x] / (N - 1) * 8.0);
                    sheet[(size_t)y * FBW * 2 + FBW + x] = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
                }
            std::string fn = fmt("shots/tests/stability_skyline_%s.png", w == 0 ? "desert" : "airless");
            writePNG(fn.c_str(), sheet.data(), FBW * 2, FBH);
            bool ok = cp < 0.5;
            if (!ok) fails++;
            printf("skyline %-8s %.0f km at %dx, walking 1.4 m a frame: over the city mean %.2f, %.2f%% of its pixels jump over 40 a frame (%d silhouettes, %d polygons); the empty horizon beside it "
                   "%.2f, %.2f%% -> %s (%s)\n",
                    w == 0 ? "desert" : "airless", D / 1000, scale, cm, cp, sv.lastSilhouettes, sv.lastFormPolys, km, kp, fn.c_str(), ok ? "steady" : "SHIMMERS");
        }
        return fails;
    }
    printf("forms sheet [scale] [desert|felisian|airless] | forms city [scale] [world] | forms skyline [scale] [world] [km] | forms stability [scale]\n");
    return 1;
}
