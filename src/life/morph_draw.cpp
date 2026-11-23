// Z-02: a slide drawn (life/morph_draw.h): the ways of seeing's palettes, the light ways per pixel from the shapes (life/morph_shape.h), the
// polarised section; the electron's mesh is life/morph_mesh.cpp
#include "life/morph_shape_internal.h"
#include "core/parallel.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace morph { void drawElectron(Framebuffer& fb, const Slide& v, const std::vector<Shape>& shapes); }
using namespace morph;

namespace {
inline double h01(uint64_t h) { return unitFromHash(mix64(h)); }   // re-mixed: a shifted hash would span [0, 2^-n) alone
constexpr double BRIGHT_FIELD = 56, PHASE_FIELD = 34, DARK_FIELD = 3;
const RGB LAMP(1.0f, 0.96f, 0.87f);

// a bank's colour at absorbance 1 in a bright field (what it lets through: its colour seen thin and pale, dense and dark)
RGB brightTint(int bank) {
    if (bank >= SB_PIG && bank < SB_PIG + PG_COUNT - 1) return pigmentColour(bank - SB_PIG + 1);
    if (bank >= SB_MINERAL && bank < SB_MINERAL + MI_COUNT) {
        static const RGB M[MI_COUNT] = {RGB(0.72f, 0.62f, 0.42f), RGB(0.78f, 0.84f, 0.88f), RGB(0.88f, 0.84f, 0.74f), RGB(0.82f, 0.86f, 0.95f),
                                        RGB(0.58f, 0.36f, 0.18f), RGB(0.9f, 0.84f, 0.36f), RGB(0.74f, 0.68f, 0.84f)};
        return M[bank - SB_MINERAL];
    }
    switch (bank) {
        case SB_EYE: return RGB(0.92f, 0.32f, 0.1f);
        case SB_OIL: return RGB(0.95f, 0.84f, 0.5f);
        case SB_SULPHUR: return RGB(0.95f, 0.9f, 0.32f);
        case SB_GRAIN: return RGB(0.55f, 0.5f, 0.42f);
        case SB_GRAIN + 1: return RGB(0.82f, 0.78f, 0.7f);
        case SB_GRAIN + 2: return RGB(0.62f, 0.36f, 0.22f);
        case SB_CARBON: return RGB(0.36f, 0.22f, 0.1f);
        default: return RGB(0.64f, 0.62f, 0.58f);   // colourless matter
    }
}
double lum(const RGB& c) { return 0.3 * c.r + 0.59 * c.g + 0.11 * c.b; }

// a bright field's bank: shade s (0..63) is the light let through, s = BRIGHT_FIELD unhindered; below it the absorbance that dims the lamp to
// s, its colour the tint's power (so a thin layer is pale and a dense one saturated and dark); above it toward white (refractile highlights)
void brightRamp(uint8_t* pal, int bank, double lamp) {
    const RGB tint = brightTint(bank);
    const double fs = BRIGHT_FIELD * lamp;
    std::vector<std::pair<double, RGB>> stops;
    const double y0 = lum(LAMP);
    for (int i = 0; i < 64; i++) {
        RGB c;
        if (i <= fs) {
            const double want = std::max(1e-4, i / std::max(1e-6, fs));
            double lo = 0, hi = 14;
            for (int it = 0; it < 40; it++) {
                const double od = (lo + hi) * 0.5;
                const RGB t(std::pow(tint.r, (float)od), std::pow(tint.g, (float)od), std::pow(tint.b, (float)od));
                const double y = lum(RGB(LAMP.r * t.r, LAMP.g * t.g, LAMP.b * t.b)) / y0;
                if (y > want) lo = od; else hi = od;
            }
            const double od = (lo + hi) * 0.5;
            c = RGB(LAMP.r * std::pow(tint.r, (float)od), LAMP.g * std::pow(tint.g, (float)od), LAMP.b * std::pow(tint.b, (float)od));
            c = c * (float)(lamp * 0.92);
        } else {
            const double f = (i - fs) / std::max(1e-6, 63 - fs);
            c = lerp(LAMP * (float)(lamp * 0.92), RGB(1, 1, 1), (float)f);
        }
        stops.push_back({(double)i, c});
    }
    setRampFromVector(pal, bank, stops);
}
// a grey ramp tinted by a bank's colour, the tint gone at the shade where it meets the field
void tintedRamp(uint8_t* pal, int bank, const RGB& grey, const RGB& tint, double at, double weight, double gamma) {
    std::vector<std::pair<double, RGB>> stops;
    const double tl = std::max(0.05, lum(tint));
    const RGB nt(tint.r / (float)tl, tint.g / (float)tl, tint.b / (float)tl);
    for (int i = 0; i < 64; i++) {
        const double s = std::pow(i / 63.0, gamma);
        const double w = weight * clampd(std::fabs(i - at) / 14.0, 0, 1);
        const RGB g = grey * (float)s;
        RGB c = lerp(g, RGB(g.r * nt.r, g.g * nt.g, g.b * nt.b), (float)w);
        stops.push_back({(double)i, clampRGB(c)});
    }
    setRampFromVector(pal, bank, stops);
}
void colourRamp(uint8_t* pal, int bank, const RGB& c, double peak) {   // black, the colour at `peak`, whiter above
    setRamp(pal, bank, {{0, RGB(0, 0, 0)}, {peak, c}, {63, lerp(c, RGB(1, 1, 1), 0.45f)}});
}

// the section's interference colours (Michel-Levy's chart, the first orders) and a grain's retardation in them
const RGB IF_COLOURS[6] = {RGB(0.5f, 0.5f, 0.52f), RGB(0.93f, 0.92f, 0.86f), RGB(0.95f, 0.76f, 0.3f), RGB(0.5f, 0.35f, 0.82f), RGB(0.38f, 0.8f, 0.45f),
                           RGB(0.92f, 0.48f, 0.45f)};
}   // namespace

void slidePalette(uint8_t* pal, int seeing, double lamp) {
    lamp = clampd(lamp, 0.05, 1.0);
    switch (seeing) {
        case WS_BRIGHT:
            for (int b = 0; b < BANKS; b++) brightRamp(pal, b, lamp);
            break;
        case WS_PHASE: case WS_DARK: {
            const bool dark = seeing == WS_DARK;
            const RGB grey = dark ? RGB(0.92f, 0.95f, 1.0f) : RGB(0.9f, 0.95f, 0.9f) * (float)(0.4 + 0.6 * lamp);
            for (int b = 0; b < BANKS; b++) tintedRamp(pal, b, grey, brightTint(b), dark ? DARK_FIELD : PHASE_FIELD * lamp, b == SB_FIELD ? 0 : (dark ? 0.6 : 0.45), dark ? 1.25 : 1.0);
            break;
        }
        case WS_GLOW:
            for (int b = 0; b < BANKS; b++) setRamp(pal, b, {{0, RGB(0, 0, 0)}, {63, RGB(0.35f, 0.35f, 0.38f)}});
            colourRamp(pal, SB_STAIN, RGB(0.28f, 0.48f, 1.0f), 46);
            colourRamp(pal, SB_RED, RGB(1.0f, 0.18f, 0.1f), 46);
            colourRamp(pal, SB_LUM, RGB(0.3f, 1.0f, 0.75f), 46);
            colourRamp(pal, SB_AUTO, RGB(0.42f, 0.8f, 0.28f), 52);
            break;
        case WS_ELECTRON:
            for (int b = 0; b < BANKS; b++) setRamp(pal, b, {{0, RGB(0, 0, 0)}, {40, RGB(0.6f, 0.61f, 0.63f)}, {63, RGB(1, 1, 1)}});
            break;
        case WS_SECTION:
            for (int b = 0; b < BANKS; b++) setRamp(pal, b, {{0, RGB(0, 0, 0)}, {63, RGB(0.3f, 0.3f, 0.3f)}});
            setRamp(pal, SB_FIELD, {{0, RGB(0, 0, 0)}, {63, RGB(0.25f, 0.25f, 0.27f)}});
            for (int k = 0; k < 6; k++) colourRamp(pal, SB_IF + k, IF_COLOURS[k], 54);
            colourRamp(pal, SB_MINERAL + MI_CALCITE, RGB(0.95f, 0.9f, 0.8f), 54);
            setRamp(pal, SB_CARBON, {{0, RGB(0, 0, 0)}, {30, RGB(0.32f, 0.18f, 0.08f)}, {63, RGB(0.6f, 0.4f, 0.2f)}});
            setRamp(pal, SB_PIG + PG_BLACK - 1, {{0, RGB(0, 0, 0)}, {63, RGB(0.15f, 0.14f, 0.12f)}});
            break;
        default: break;
    }
}

int seeingBanks(int seeing, bool used[BANKS]) {
    for (int b = 0; b < BANKS; b++) used[b] = false;
    switch (seeing) {
        case WS_BRIGHT: case WS_PHASE: case WS_DARK: for (int b = 0; b <= SB_CARBON; b++) used[b] = true; break;
        case WS_GLOW: used[SB_FIELD] = used[SB_STAIN] = used[SB_RED] = used[SB_LUM] = used[SB_AUTO] = true; break;
        case WS_ELECTRON: used[SB_FIELD] = true; break;
        case WS_SECTION:
            used[SB_FIELD] = used[SB_CARBON] = used[SB_MINERAL + MI_CALCITE] = used[SB_PIG + PG_BLACK - 1] = true;
            for (int k = 0; k < 6; k++) used[SB_IF + k] = true;
            break;
        default: break;
    }
    int n = 0; for (int b = 0; b < BANKS; b++) n += used[b];
    return n;
}

double magnificationFor(const Morph& m, int seeing, double share) {   // the body's view (the plates, the microscope's first look)
    // the objectives (x20 .. x1000) and the camera's zoom between them; the electron's
    static const double LIGHT[] = {4000, 3000, 2000, 1500, 1000, 600, 400, 300, 200, 150, 100, 60, 40, 30, 20};
    static const double ELECTRON[] = {40000, 30000, 20000, 15000, 10000, 7000, 5000, 3000, 2000, 1500, 1000, 700, 500, 300, 200, 150, 100, 70, 50};
    const int n = seeing == WS_ELECTRON ? 19 : 15;
    const double* steps = seeing == WS_ELECTRON ? ELECTRON : LIGHT;
    // the body, and its appendages to 1.6 times its size (a longer flagellum or stalk runs out of the view)
    const double body = m.viewUm(), e = seeing == WS_ELECTRON ? body * 1.25 : std::max(body * 1.15, std::min(m.extentUm(), body * 1.6));
    for (int i = 0; i < n; i++) if (2 * e <= share * umPerPxAt(steps[i]) * UH) return steps[i];
    return steps[n - 1];
}

std::vector<int> seeingsFor(const Morph& m) {
    std::vector<int> w;
    if (m.state == SS_FOSSIL) return {WS_SECTION, WS_BRIGHT};
    if (m.state == SS_DORMANT) return {WS_PHASE, WS_ELECTRON};
    if (m.state == SS_PROTO) return {WS_PHASE, WS_DARK};
    const bool coloured = m.pigment != PG_NONE || m.way == LW_LIGHT || m.plan == MP_TEST || m.plan == MP_CHAMBERED;
    w.push_back(coloured ? WS_BRIGHT : WS_PHASE);
    const bool walled = m.plan == MP_SHELLED || m.plan == MP_LATTICE || m.plan == MP_TEST || m.plan == MP_ARMOURED || m.plan == MP_CHAMBERED ||
                        (m.mark != MK_SMOOTH && smallCellPlan(m.plan)) || m.spines > 0;
    if (m.glows) w.insert(w.begin(), WS_GLOW);
    else if (smallCellPlan(m.plan) && (m.flagella > 0 || m.plan == MP_CORKSCREW)) w.push_back(WS_DARK);
    else if (walled) w.push_back(WS_ELECTRON);
    else if (m.way == LW_LIGHT) w.push_back(WS_GLOW);
    else w.push_back(coloured ? WS_PHASE : WS_DARK);
    return w;
}

namespace {
// what a pixel gathers from the parts over it
struct Acc {
    float od = 0, bankOd = 0, phase = 0, halo = 0, edge = 0, refr = 0, cover = 0, carbon = 0;
    float fl[4] = {0, 0, 0, 0};   // the stain, the plastids' red, its own light, the walls' faint glow
    int bank = SB_FIELD, fillBank = -1;
    void tint(int b, float a) { if (a > bankOd && b != SB_FIELD) { bankOd = a; bank = b; } }
};

double dome(double e, double R) { const double x = clampd(e / std::max(1e-9, R), 0, 1); return std::sqrt(x * (2 - x)); }

// a body's ornament where a pixel falls: -1 a hole, 1 a ridge (life/morph_shape.h's layouts), the mosaic's cell
double ornament(const morph::Body& b, double u, double v, double phi, double h, double tq, int seg, double side, int* cell, bool back) {
    const Look& L = b.look;
    if (L.mark == MK_SMOOTH || L.mark == MK_SPINES) return 0;
    const double n = std::max(2, L.marks);
    if (b.kind == BODY_RADIAL) {
        const double R = b.R, Lh = R * b.aspect;
        if (L.layout != 0) return flatOrnament(b, u, v);
        {
            const double z = back ? -h : h;
            return markAt(L.mark, L.marks, L.markSeed, u / std::max(1e-9, Lh) * b.aspect * 0.5 + u / Lh * 0.5, v / R, z, cell);
        }
    }
    // a tube: along it in widths, round it
    const double r = b.rad[seg] + (b.rad[std::min((int)b.rad.size() - 1, seg + 1)] - b.rad[seg]) * tq;
    double along = 0;
    for (int i = 0; i < seg; i++) { const double dx = b.line[i + 1].x - b.line[i].x, dy = b.line[i + 1].y - b.line[i].y; along += std::sqrt(dx * dx + dy * dy); }
    {
        const int j = std::min((int)b.line.size() - 1, seg + 1);
        const double dx = b.line[j].x - b.line[seg].x, dy = b.line[j].y - b.line[seg].y;
        along += std::sqrt(dx * dx + dy * dy) * tq;
    }
    const double w = clampd(side * (1 - h * 0.0), -1, 1);
    const double s = along / std::max(1e-9, 2 * r);
    if (L.layout == 3) { const double f = w * n * 0.25; return std::fabs(f - std::round(f)) < 0.14 ? 1.0 : 0.0; }
    const double c = back ? -h : h;
    return markAt(L.mark, L.marks, L.markSeed, s, w, c, cell);
}

// a body's part of a pixel (the light ways and the section)
void bodyAt(const morph::Body& b, double px, double py, double pz, int seeing, Acc& A) {
    double u = 0, v = 0, phi = 0, tq = 0, side = 0;
    int seg = 0;
    double d, Rh;
    if (b.kind == BODY_RADIAL) {
        d = radialDistance(b, px, py, u, v, phi);
        Rh = b.R * clampd(outlineAt(b, phi), 0.25, 1.0);
    } else {
        d = tubeDistance(b, px, py, seg, tq, side);
        Rh = b.rad[seg] + (b.rad[std::min((int)b.rad.size() - 1, seg + 1)] - b.rad[seg]) * tq;
        const P2 a = b.line[seg], c = b.line[std::min((int)b.line.size() - 1, seg + 1)];
        const double ex = c.x - a.x, ey = c.y - a.y, l = std::sqrt(ex * ex + ey * ey);
        const double qx = a.x + ex * tq, qy = a.y + ey * tq;
        const double lat = l > 0 ? ((px - qx) * -ey + (py - qy) * ex) / l : 0;
        side = clampd(lat / std::max(1e-9, Rh), -1, 1);
    }
    const Look& L = b.look;
    const float ghost = L.ghost;
    const double haloW = std::max(pz * 3, Rh * 0.35) + L.halo;
    if (d > haloW) return;
    if (d > 0) {   // outside: phase contrast's halo, a capsule's
        const double f = 1 - d / haloW;
        A.halo += (float)(0.55 * f * f * (1 - ghost) * std::min(1.0, (double)L.phase + L.refract));
        if (L.halo > 0 && d < L.halo) { A.halo += 0.12f * (float)(1 - d / L.halo); A.edge += 0.08f * (float)(1 - d / L.halo); A.od += 0.01f; }
        if (seeing == WS_DARK && d < pz * 1.5) A.edge += (float)(0.5 * (1 - d / (pz * 1.5)) * (1 - ghost * 0.7));
        return;
    }
    const double e = -d;
    const double cov = clampd(e / pz + 0.5, 0, 1);
    const double h = dome(e, Rh);
    // the wall, open at an aperture, broken where lysed
    bool inGap = false;
    if (L.gapW > 0 && b.kind == BODY_RADIAL) inGap = std::fabs(wrapAngle(phi - L.gapPhi)) < L.gapW;
    double wallW = std::max((double)L.wall, pz * 1.2);
    double wall = e < wallW && !inGap ? 1 - e / wallW : 0;
    if (ghost > 0 && wall > 0) {
        const double brk = std::sin(phi * 7 + u * 0.9) * 0.5 + 0.5;
        if (brk < ghost * 0.8) wall *= 0.15;
    }
    int cell = 0;
    double orn = ornament(b, u, v, phi, h, tq, seg, side, &cell, false);
    const bool clear = L.density < 0.35f;
    double ornBack = clear && L.mark != MK_SMOOTH && L.layout == 0 ? ornament(b, u, v, phi, h, tq, seg, side, nullptr, true) : 0;
    if (L.fossil || seeing == WS_SECTION) {   // turned to carbon: its wall a brown line, broken; a faint film within
        const double cw = std::max((double)L.wall, pz * 3.0);
        const double cwall = e < cw ? 1 - e / cw : 0;
        const double brk = 0.5 + 0.5 * std::sin(phi * 5 + std::cos(u * 1.3) * 2 + v * 0.7);
        double c = (cwall > 0 && brk > 0.2 ? cwall * 1.2 : 0) + (orn > 0.5 ? 0.25 : 0) + 0.02 * h;
        A.carbon += (float)(c * cov);
        A.cover = std::max(A.cover, (float)cov);
        if (seeing == WS_SECTION) return;
        A.od += (float)(c * 1.3 * cov); A.tint(SB_CARBON, (float)(c * 1.3 * cov)); A.phase += (float)(c * cov); A.edge += (float)(c * 0.8 * cov);
        return;
    }
    const float dens = L.density * (1 - 0.85f * ghost);
    double od = dens * h;
    double wd = L.wallDensity * wall * (1 - 0.7 * ghost);
    if (orn < 0) { wd *= 0.2; od *= 0.75; }
    if (orn > 0) wd += 0.25 * orn * (wall > 0 ? 1 : 0.5);
    if (L.mark == MK_LATTICE) { od = orn < 0 ? dens * h * 0.3 : dens * h + 0.35; if (orn >= 0 && wall == 0) wd += 0.25; }
    if (ornBack != 0) od += ornBack > 0 ? 0.12 : (ornBack < 0 ? -0.05 : 0);
    double total = (od + wd) * cov;
    const bool grains = L.mark == MK_GRAINS;
    int bank = L.bank;
    if (grains) { bank = SB_GRAIN + (cell % 3); total += orn > 0 ? 0.25 * orn : 0.3; }
    // relief from the edge: a little oblique light, so a body reads as round
    const double relief = (b.kind == BODY_RADIAL ? (u * 0.5 - v * 0.5) : side * 0.6) / std::max(1e-9, Rh) * (1 - h) * 0.12;
    A.od += (float)std::max(0.0, total + relief * cov);
    A.tint(bank, (float)((od + wd) * cov));
    A.phase += (float)(L.phase * (h * 0.9 + wall * 0.6 + (orn > 0 ? 0.2 : 0)) * cov * (1 - 0.8 * ghost));
    A.edge += (float)((wall * 0.9 + (orn != 0 ? 0.35 : 0) + (1 - h) * 0.25) * cov * (1 - 0.6 * ghost));
    if (L.refract > 0) {   // refractile: a dark rim, a bright core
        A.refr += (float)(L.refract * h * h * cov);
        A.od += (float)(L.refract * wall * 0.5 * cov);
    }
    if (L.layers > 0) {   // a layered vesicle's membranes
        const double rr = e / std::max(1e-9, b.R);
        const double f = rr * L.layers * 1.2;
        if (rr < 0.85 && std::fabs(f - std::round(f)) < 0.12) { A.od += 0.12f; A.phase += 0.35f; A.edge += 0.35f; }
    }
    A.fl[0] += (float)(L.stain * h * h * cov);
    A.fl[1] += (float)(L.red * h * cov);
    A.fl[2] += (float)(L.lum * (0.4 + 0.6 * h) * cov);
    A.fl[3] += (float)((0.05 + 0.3 * wall) * cov * (1 - ghost));
    A.cover = std::max(A.cover, (float)cov);
}

void dotAt(const Dot& d, double px, double py, double pz, int seeing, double minR, Acc& A) {
    const double dx = px - d.c.x, dy = py - d.c.y;
    const double qx = dx * d.ca + dy * d.sa, qy = -dx * d.sa + dy * d.ca;
    double rx = d.rx, ry = d.ry, st = d.strength;
    const double mr = std::min(rx, ry);
    if (mr < minR) { const double k = minR / mr; st *= std::pow(1 / k, 0.8); rx *= k; ry *= k; }
    const double n = std::sqrt((qx / rx) * (qx / rx) + (qy / ry) * (qy / ry));
    const double r = std::min(rx, ry);
    const double e = (1 - n) * r;   // inside distance
    if (e < -pz) return;
    const double cov = clampd(e / pz + 0.5, 0, 1);
    const double h = dome(std::max(0.0, e), r);
    const float s = (float)(st * cov);
    switch (d.kind) {
        case DOT_NUCLEUS: {
            const double rim = e < r * 0.18 ? 1 : 0;
            A.od += s * (float)(0.12 * h + 0.15 * rim); A.phase += s * (float)(0.35 * h + 0.3 * rim); A.edge += s * (float)(0.25 * rim);
            A.fl[0] += s * (float)(1.2 * (0.4 + 0.6 * h));
            break;
        }
        case DOT_NUCLEOLUS: A.od += s * 0.25f * (float)h; A.phase += s * 0.5f * (float)h; A.fl[0] += s * 0.3f; break;
        case DOT_VACUOLE: case DOT_FOOD: {
            const double rim = e < std::max(pz, r * 0.15) ? 1 : 0;
            A.od = std::max(0.0f, A.od - s * 0.12f * (float)h) + s * 0.06f * (float)rim;
            A.phase = std::max(0.0f, A.phase - s * 0.45f * (float)h) + s * 0.25f * (float)rim;
            A.halo += s * 0.15f * (float)h;
            A.edge += s * 0.3f * (float)rim;
            break;
        }
        case DOT_CANAL: A.od = std::max(0.0f, A.od - s * 0.05f); A.phase = std::max(0.0f, A.phase - s * 0.2f); A.halo += s * 0.08f; break;
        case DOT_NUCLEOID: A.od = std::max(0.0f, A.od - s * 0.03f * (float)h); A.phase = std::max(0.0f, A.phase - s * 0.45f * (float)h); A.fl[0] += s * 1.1f * (float)h; break;
        case DOT_GRANULE: case DOT_DEBRIS: A.od += s * 0.28f * (float)h; A.phase += s * 0.55f * (float)h; A.edge += s * 0.55f * (float)h; break;
        case DOT_PLASTID: A.od += s * 0.6f * (float)(0.4 + 0.6 * h); A.tint(d.bank, s * 0.9f); A.phase += s * 0.35f; A.fl[1] += s * 1.3f * (float)(0.5 + 0.5 * h);
                          A.edge += s * 0.15f; break;
        case DOT_PYRENOID: A.refr += s * 0.4f * (float)h; A.phase += s * 0.3f; A.od += s * 0.05f; break;
        case DOT_EYESPOT: A.od += s * 1.1f * (float)(0.5 + 0.5 * h); A.tint(SB_EYE, s * 2.0f); A.phase += s * 0.4f; break;
        case DOT_DROPLET: {
            const double rim = e < r * 0.3 ? 1 : 0;
            A.refr += s * 0.9f * (float)(h * h); A.od += s * (float)(0.35 * rim + 0.05); A.tint(d.bank, s * 0.6f); A.edge += s * 0.7f * (float)h;
            break;
        }
        case DOT_GAS: A.refr += s * 0.55f * (float)h; A.edge += s * 0.5f * (float)h; A.od += s * 0.04f; break;
        case DOT_MAGNET: A.od += s * 2.2f; A.edge += s * 0.6f; A.phase += s * 0.8f; break;
        case DOT_GRAIN: A.od += s * (float)(0.45 + 0.3 * h); A.tint(d.bank == SB_FIELD ? SB_GRAIN : d.bank, s * 0.7f); A.edge += s * 0.8f * (float)h; A.phase += s * 0.7f; break;
        case DOT_SPORE: {
            const double rim = e < r * 0.25 ? 1 : 0;
            A.od += (float)(cov * (0.15 + 0.25 * rim)); A.phase += (float)(cov * (0.6 - 0.9 * st)); A.refr += s * 1.1f * (float)h; A.edge += (float)(cov * 0.5 * rim);
            break;
        }
        default: break;
    }
    if (seeing == WS_SECTION && (d.kind == DOT_GRANULE || d.kind == DOT_NUCLEUS)) A.carbon += s * 0.4f;
}

void strandAt(const Strand& s, double px, double py, double pz, int seeing, double minW, Acc& A) {
    double best = 1e30, along = 0;
    const int n = (int)s.pts.size();
    for (int i = 0; i + 1 < n; i++) {
        const P2 a = s.pts[i], b = s.pts[i + 1];
        const double ex = b.x - a.x, ey = b.y - a.y, l2 = ex * ex + ey * ey;
        double q = l2 > 0 ? ((px - a.x) * ex + (py - a.y) * ey) / l2 : 0;
        q = clampd(q, 0, 1);
        const double cx = a.x + ex * q - px, cy = a.y + ey * q - py, dd = cx * cx + cy * cy;
        if (dd < best) { best = dd; along = (i + q) / (n - 1); }
    }
    double w = s.w0 + (s.w1 - s.w0) * along, st = s.strength;
    if (w < minW) { st *= std::pow(w / minW, 0.5); w = minW; }
    const double d = std::sqrt(best) - w * 0.5;
    if (d > pz) return;
    const double cov = clampd(-d / pz + 0.5, 0, 1) * clampd(1 - std::sqrt(best) / (w * 0.5 + pz), 0.25, 1);
    const float c = (float)(cov * st);
    static const float OD[ST_COUNT] = {0.05f, 0.25f, 0.0f, 0.55f, 0.45f, 0.2f, 0.14f, 0.35f, 0.0f, 0.15f, 0.08f};
    static const float PH[ST_COUNT] = {0.2f, 0.6f, 0.02f, 0.9f, 0.8f, 0.5f, 0.35f, 0.7f, 0.0f, 0.45f, 0.2f};
    static const float ED[ST_COUNT] = {1.0f, 1.1f, 0.25f, 1.2f, 1.0f, 0.9f, 0.7f, 0.9f, 0.0f, 0.8f, 0.3f};
    A.od += OD[s.kind] * c; A.phase += PH[s.kind] * c; A.edge += ED[s.kind] * c;
    if (s.bank != SB_FIELD) A.tint(s.bank, OD[s.kind] * c * 1.5f);
    if (s.kind == ST_SPINE) A.fl[3] += 0.1f * c;
    if (seeing == WS_SECTION && (s.kind == ST_SPINE || s.kind == ST_STALK)) A.carbon += 0.5f * c;
}

void waveAt(const Wave& w, double px, double py, Acc& A) {
    const double dx = px - w.c.x, dy = py - w.c.y, r = std::sqrt(dx * dx + dy * dy);
    if (r > w.R) return;
    const double f = 0.5 + 0.5 * std::cos(w.arms * std::atan2(dy, dx) - w.k * r + w.phase);
    const double band = std::pow(f, 6) * w.strength * (1 - r / w.R);
    A.edge += (float)(band * 0.35); A.halo += (float)(band * 0.2); A.od += (float)(band * 0.03);
}

// Z-04: a cell too small on the screen to show more than its body (`Slide::lod`): a speck its body's size (a pixel at least, fainter as it
// grows to one), its look its cell's (its pigment's bank, its density, its DNA's stain, its plastids' red, its own light; a spore bright in
// phase, a fossil's carbon)
struct Speck { P2 c; double r = 0.3; int bank = SB_FIELD; float od = 0.3f, phase = 1, refr = 0, stain = 0.7f, red = 0, lum = 0, carbon = 0; };
Speck speckOf(const Specimen& s, double t) {
    const Morph& m = *s.m;
    Speck k;
    double x, y, a;
    specimenPlace(s, t, x, y, a);
    k.c = {x, y};
    k.r = std::max(0.15, 0.6 * m.bodyUm());
    const Look L = cellLook(m, m.widthUm * 0.5);
    k.bank = L.bank; k.od = 0.2f + L.density; k.stain = L.stain + (m.nucleus > 0 ? 0.6f : 0); k.red = L.red + (m.plastids > 0 ? 0.6f : 0); k.lum = L.lum;
    if (s.form == CF_SPORE || s.form == CF_CYST) { k.refr = 0.9f; k.phase = 0.4f; k.stain = 0.1f; k.red = 0; }
    if (s.form == CF_FOSSIL) { k.carbon = 0.9f; k.stain = 0; k.red = 0; k.lum = 0; k.bank = SB_CARBON; }
    if (s.form == CF_GHOST) { k.od *= 0.3f; k.phase = 0.3f; k.stain = 0.05f; k.red = 0; k.lum = 0; }
    return k;
}
void speckAt(const Speck& k, double px, double py, double pz, double minR, Acc& A) {
    double r = k.r, st = 1;
    if (r < minR) { st = std::pow(r / minR, 0.8); r = minR; }
    const double e = r - std::sqrt((px - k.c.x) * (px - k.c.x) + (py - k.c.y) * (py - k.c.y));
    if (e < -pz) return;
    const double cov = clampd(e / pz + 0.5, 0, 1), h = dome(std::max(0.0, e), r);
    const float s = (float)(st * cov), sh = (float)(st * cov * h);
    A.od += s * k.od * (0.5f + 0.5f * (float)h); A.tint(k.bank, s * k.od);
    A.phase += sh * k.phase; A.edge += sh * 0.7f; A.refr += sh * k.refr; A.halo += s * 0.1f * (float)(1 - h);
    A.fl[0] += sh * k.stain; A.fl[1] += sh * k.red; A.fl[2] += s * k.lum; A.fl[3] += s * 0.05f;
    A.carbon += s * k.carbon;
    A.cover = std::max(A.cover, (float)cov);
}
// Z-04: what a part out of focus gives a pixel, gathered alone (its edges spread by the blur) and added at its share (fainter as it spreads)
void addScaled(Acc& A, const Acc& a, float f) {
    A.od += a.od * f; A.phase += a.phase * f; A.halo += a.halo * f; A.edge += a.edge * f; A.refr += a.refr * f; A.carbon += a.carbon * f;
    for (int k = 0; k < 4; k++) A.fl[k] += a.fl[k] * f;
    A.cover = std::max(A.cover, a.cover * f);
    A.tint(a.bank, a.bankOd * f);
}

// the section's rock: grains of chert (its first-order greys and whites), calcite (its high-order pale white), a few of mica and feldspar
// (brighter orders), pyrite (opaque), each its retardation and its extinction's angle against the stage; where the grains are small on the
// screen they go to their mean (no speckle: far thin things are sized in screen pixels)
void rockAt(const Slide& v, double px, double py, double pz, int& bank, double& shade) {
    const double gs = 14.0;
    const double x = px / gs, y = py / gs;
    const int64_t ix = (int64_t)std::floor(x), iy = (int64_t)std::floor(y);
    double d1 = 1e9, d2 = 1e9; uint64_t id = 0;
    for (int64_t a = ix - 1; a <= ix + 1; a++)
        for (int64_t b = iy - 1; b <= iy + 1; b++) {
            const uint64_t h = hash2i(a, b, v.seed ^ 0x5EC7ULL);
            const double cx = a + h01(h), cy = b + h01(h + 1);
            const double d = (cx - x) * (cx - x) + (cy - y) * (cy - y);
            if (d < d1) { d2 = d1; d1 = d; id = h; } else if (d < d2) d2 = d;
        }
    const double k = h01(id + 2), bright = 0.6 + 0.4 * h01(id + 3);
    // its extinction's angle, bent a little across it (the strained quartz's undulose extinction)
    const double strain = 0.18 * std::sin(px * 0.21 + h01(id + 6) * TAU) * std::cos(py * 0.17 + h01(id + 7) * TAU);
    const double ext = std::pow(std::sin(2 * (h01(id + 4) * PI + strain - v.stage)), 2);
    if (k < 0.04) { bank = SB_PIG + PG_BLACK - 1; shade = 3; }
    else if (k < 0.16) { bank = SB_MINERAL + MI_CALCITE; shade = 20 + 12 * (0.5 + 0.5 * ext) * bright; }
    else if (k < 0.985) { bank = SB_IF + (k < 0.6 ? 0 : 1); shade = 12 + 22 * ext * bright; }
    else { bank = SB_IF + 2 + (int)(h01(id + 5) * 3.99); shade = 12 + 22 * ext * bright; }
    if (std::sqrt(d2) - std::sqrt(d1) < 0.03 + 1.5 * pz / gs) shade *= 0.7;   // the boundaries
    // fluid inclusions: dark specks of a few tenths of a micrometre, seen once they are a pixel or more
    if (pz < 0.3) {
        const double ig = 0.7;
        const int64_t gx = (int64_t)std::floor(px / ig), gy = (int64_t)std::floor(py / ig);
        const uint64_t h = hash2i(gx, gy, v.seed ^ 0x1C1ULL);
        if (h01(h) < 0.12) {
            const double cx = (gx + 0.2 + 0.6 * h01(h + 1)) * ig, cy = (gy + 0.2 + 0.6 * h01(h + 2)) * ig, r = 0.06 + 0.1 * h01(h + 3);
            if ((px - cx) * (px - cx) + (py - cy) * (py - cy) < r * r) shade *= 0.35;
        }
    }
    const double grainPx = gs / pz;
    if (grainPx < 8) { const double f = clampd((8 - grainPx) / 6, 0, 1); shade += (22 - shade) * f; if (f > 0.5) bank = SB_IF; }
}
}   // namespace

void drawSlide(Framebuffer& fb, const Slide& v, const std::vector<Specimen>& cells) {
    const double pz = v.umPerPx / FB_SCALE;
    // Z-04: a cell too small to show more than its body is a speck (`Slide::lod`: the microscope's low powers), the rest built
    std::vector<Shape> shapes(cells.size());
    std::vector<Speck> specks;
    std::vector<size_t> speckCell;
    for (size_t i = 0; i < cells.size(); i++) {
        if (v.lod && cells[i].m && v.seeing != WS_ELECTRON && cells[i].m->bodyUm() * 2 < 2.5 * v.umPerPx) { specks.push_back(speckOf(cells[i], v.t)); speckCell.push_back(i); continue; }
        buildShape(cells[i], v.t, pz, shapes[i]);
    }
    fb.clearDepth();
    if (v.seeing == WS_ELECTRON) { drawElectron(fb, v, shapes); return; }
    const int W = FBW, H = FBH, seeing = v.seeing;
    const double x0 = v.cx - W * 0.5 * pz, y0 = v.cy - H * 0.5 * pz;
    const double minR = pz * 0.5 * (g_mushKernel + 1), minW = pz * (g_mushKernel + 1);
    // Z-04: a part of a cell out of focus (`Specimen::blurUm`) is drawn with its edges spread over its blur and added at its share
    struct Item { int kind; const void* p; int px0, px1, py0, py1; double pz; float fade; };
    std::vector<Item> items;
    auto add = [&](int kind, const void* p, double ax, double ay, double bx, double by, double pad, double blur, float fade) {
        const int a = (int)std::floor((ax - pad - blur - x0) / pz), b = (int)std::ceil((bx + pad + blur - x0) / pz);
        const int c = (int)std::floor((ay - pad - blur - y0) / pz), d = (int)std::ceil((by + pad + blur - y0) / pz);
        if (b < 0 || d < 0 || a >= W || c >= H) return;
        items.push_back({kind, p, std::max(0, a), std::min(W - 1, b), std::max(0, c), std::min(H - 1, d), pz + blur, fade});
    };
    auto fadeOf = [&](const Specimen& c) { return c.blurUm > 0 && c.m ? (float)clampd(std::max(0.3, c.m->widthUm) / (std::max(0.3, c.m->widthUm) + 1.5 * c.blurUm), 0.2, 0.999) : 1.0f; };
    for (size_t i = 0; i < shapes.size(); i++) {
        const Shape& s = shapes[i];
        const double blur = std::max(0.0, cells[i].blurUm);
        const float fade = fadeOf(cells[i]);
        for (const Wave& w : s.waves) add(3, &w, w.c.x - w.R, w.c.y - w.R, w.c.x + w.R, w.c.y + w.R, 0, blur, fade);
        for (const morph::Body& b : s.bodies) {
            const double R = b.kind == BODY_RADIAL ? b.R : 0;
            add(0, &b, b.x0, b.y0, b.x1, b.y1, std::max(pz * 3, R * 0.4) + b.look.halo + pz * 2, blur, fade);
        }
        for (const Dot& d : s.dots) { const double e = std::max(std::max(d.rx, d.ry), minR) * 1.3; add(1, &d, d.c.x - e, d.c.y - e, d.c.x + e, d.c.y + e, pz, blur, fade); }
        for (const Strand& st : s.strands) add(2, &st, st.x0, st.y0, st.x1, st.y1, minW + pz, blur, fade);
    }
    for (size_t k = 0; k < specks.size(); k++) {
        const Speck& sp = specks[k];
        const double e = std::max(sp.r, minR) + pz;
        add(4, &sp, sp.c.x - e, sp.c.y - e, sp.c.x + e, sp.c.y + e, pz, std::max(0.0, cells[speckCell[k]].blurUm), fadeOf(cells[speckCell[k]]));
    }
    const double lamp = clampd(v.lamp, 0.05, 1.0);
    // Z-04: a step of 2 evaluates every other pixel of every other row, each the block's (the higher scales, where the mush spans more)
    const int step = v.step >= 2 ? 2 : 1;
    auto first = [&](int a) { return a + (a % step ? step - a % step : 0); };
    parallelFor((H + step - 1) / step, 8 / step, [&](int ba, int bb) {
        std::vector<Acc> acc((size_t)W);
        for (int b = ba; b < bb; b++) {
            const int y = b * step;
            std::fill(acc.begin(), acc.end(), Acc());
            const double py = y0 + (y + 0.5) * pz;
            for (const Item& it : items) {
                if (y < it.py0 || y > it.py1) continue;
                if (it.fade >= 1) {
                    for (int x = first(it.px0); x <= it.px1; x += step) {
                        const double px = x0 + (x + 0.5) * pz;
                        switch (it.kind) {
                            case 0: bodyAt(*(const morph::Body*)it.p, px, py, pz, seeing, acc[x]); break;
                            case 1: dotAt(*(const Dot*)it.p, px, py, pz, seeing, minR, acc[x]); break;
                            case 2: strandAt(*(const Strand*)it.p, px, py, pz, seeing, minW, acc[x]); break;
                            case 4: speckAt(*(const Speck*)it.p, px, py, pz, minR, acc[x]); break;
                            default: waveAt(*(const Wave*)it.p, px, py, acc[x]); break;
                        }
                    }
                    continue;
                }
                for (int x = first(it.px0); x <= it.px1; x += step) {   // out of focus
                    const double px = x0 + (x + 0.5) * pz;
                    Acc a;
                    switch (it.kind) {
                        case 0: bodyAt(*(const morph::Body*)it.p, px, py, it.pz, seeing, a); break;
                        case 1: dotAt(*(const Dot*)it.p, px, py, it.pz, seeing, minR, a); break;
                        case 2: strandAt(*(const Strand*)it.p, px, py, it.pz, seeing, minW, a); break;
                        case 4: speckAt(*(const Speck*)it.p, px, py, it.pz, minR, a); break;
                        default: waveAt(*(const Wave*)it.p, px, py, a); break;
                    }
                    addScaled(acc[x], a, it.fade);
                }
            }
            Pix* row = &fb.idx[(size_t)y * W];
            for (int x = 0; x < W; x += step) {
                const Acc& A = acc[x];
                const double px = x0 + (x + 0.5) * pz;
                const double sx = (x + 0.5) / W - 0.5, sy = (y + 0.5) / H - 0.5;
                const double vig = 1 - 0.28 * (sx * sx + sy * sy) * 2.2;   // the field's fall-off toward its edge
                double shade = 0;
                int bank = SB_FIELD;
                const bool coloured = A.bankOd > 0.02f && A.bankOd >= 0.3f * A.od;
                switch (seeing) {
                    case WS_BRIGHT: {
                        double light = lamp;
                        if (v.spotR > 0) { const double dd = (px - v.spotX) * (px - v.spotX) + (py - v.spotY) * (py - v.spotY); light *= 1 + 0.12 * std::exp(-dd / (v.spotR * v.spotR)); }
                        shade = BRIGHT_FIELD * light * std::exp(-(double)A.od) * vig + 7 * A.refr;
                        bank = coloured ? A.bank : SB_FIELD;
                        break;
                    }
                    case WS_PHASE:
                        shade = PHASE_FIELD * lamp * vig - 24 * (1 - std::exp(-(double)A.phase)) + 26 * std::min(1.2f, A.halo) + 22 * std::min(1.2f, A.refr);
                        bank = coloured ? A.bank : SB_FIELD;
                        break;
                    case WS_DARK:
                        shade = DARK_FIELD + 52 * (1 - std::exp(-1.4 * (double)A.edge)) * lamp + 18 * A.refr;
                        bank = coloured ? A.bank : SB_FIELD;
                        break;
                    case WS_GLOW: {
                        int best = 3; float bv = A.fl[3] * 0.35f;
                        for (int k = 0; k < 3; k++) if (A.fl[k] > bv) { bv = A.fl[k]; best = k; }
                        shade = 62 * (1 - std::exp(-1.1 * (double)bv));
                        bank = best == 0 ? SB_STAIN : (best == 1 ? SB_RED : (best == 2 ? SB_LUM : SB_AUTO));
                        if (bv < 0.01f) bank = SB_FIELD;
                        break;
                    }
                    case WS_SECTION: {
                        int rb; double rs;
                        rockAt(v, px, py, pz, rb, rs);
                        bank = rb; shade = rs * lamp;
                        if (A.cover > 0.05f) { shade += (30 - shade) * 0.55 * A.cover; if (A.cover > 0.5f) bank = SB_IF + 1; }   // a fossil's paler fill
                        if (A.carbon > 0.08f) { bank = SB_CARBON; shade = 5 + 20 * std::exp(-2.6 * (double)A.carbon); }
                        break;
                    }
                    default: break;
                }
                row[x] = pix(bank, clampd(shade, 0, 63));
                if (step > 1) {   // the block's other pixels
                    if (x + 1 < W) row[x + 1] = row[x];
                    if (y + 1 < H) { Pix* next = row + W; next[x] = row[x]; if (x + 1 < W) next[x + 1] = row[x]; }
                }
            }
        }
    });
}
