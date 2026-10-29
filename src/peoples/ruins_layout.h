#pragma once
// E-09: what ruins.cpp's parts share, for them alone (ruins, ruins_towns, ruins_settlement): the includes, a settlement's
// layout as it is built (`Layout`, its grid of buildings and their specs), a town's past (`TownHist`), the age's scale, a
// monument's pick and size, and each part's functions the others call, declared at the end
#include "ruins.h"
#include "galaxy/drainage.h"
#include "peoples.h"
#include "buildings.h"
#include "plan.h"
#include "towns.h"
#include "works.h"
#include "history.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <mutex>
#include <unordered_map>

// the age's scale of a settlement's plan: its spacing and its radius (a prehistoric camp's huts closer than a medieval town's
// houses, an industrial town's rows wider apart, a modern town's blocks wider still); the futuristic and far-future ages' own
inline double ageScale(int age) { static const double S[AGE_COUNT] = {0.85, 1.0, 1.85, 2.4, 1.0, 1.0}; return S[clampi(age, 0, AGE_COUNT - 1)]; }

// the class's size: the number of buildings and the radius, the first draws of the layout's stream (the landmark finder
// reads the radius without the layout). A-03: by the age (C-01's draws, scaled; the futuristic age's few great structures and the
// far future's one have radii of their own, drawn after)
// a lone monument, the age's: its kind and size drawn with the class's size, so its reach is the radius the finders read
struct MonPick { int kind = BK_PLATFORM, count = 1, style = 0; double hw = 5, hd = 5, hWall = 3.3; };
// A-04: the focal accent's size by its kind, its age and the people's ambition (a modern people's is twice a medieval one's, a
// futuristic one's four times)
inline void focusSize(const Culture& c, int focus, Rng& rng, double& hw, double& hd, double& h) {
    static const double AGE_K[AGE_COUNT] = {0.7, 1.0, 1.4, 2.2, 4.0, 4.0};
    const double k = AGE_K[clampi(c.age, 0, AGE_COUNT - 1)] * (0.6 + 0.9 * c.genome.at(GV_AMBITION)), u = rng.uni(), v = rng.uni();
    switch (focus) {
        case FO_SPIRE: hw = 2.5 + 1.0 * u; h = 30 + 20 * v; break;
        case FO_DOME: hw = 6 + 4 * u; h = 10 + 6 * v; break;
        case FO_GATE: hw = 3 + 2 * u; h = 9 + 6 * v; break;
        case FO_TOWER: hw = 2.5 + 1.5 * u; h = 18 + 14 * v; break;
        case FO_COLOSSUS: hw = 2.0 + 1.0 * u; h = 10 + 14 * v; break;
        case FO_MAST: hw = 1.5 + 1.0 * u; h = 20 + 20 * v; break;
        case FO_RING: hw = 4 + 4 * u; h = 2 * hw; break;
        case FO_OBELISK: hw = 1.4 + 0.8 * u; h = 14 + 12 * v; break;
        case FO_POOL: hw = 6 + 4 * u; hd = 4 + 4 * v; h = 0.8; hw *= std::sqrt(k); hd *= std::sqrt(k); return;
        default: hw = 7 + 4 * u; h = 16 + 10 * v; break;   // a great tree
    }
    hw *= std::sqrt(k); h *= k; hd = hw;
    if (focus == FO_RING) h = 2 * hw;
}
// T-02: the buildings placed so far by cells (a city's thousand and more: the fit test reads the cells round a spot, not every building)
struct BuildingGrid {
    double cell = 16, x0 = 0, z0 = 0; int n = 0;
    std::vector<std::vector<int>> cells;
    void init(double reach) { x0 = z0 = -reach; n = std::max(1, (int)std::ceil(2 * reach / cell)); cells.assign((size_t)n * n, {}); }
    void range(double x, double z, double rad, int& i0, int& i1, int& j0, int& j1) const {
        i0 = clampi((int)std::floor((x - rad - x0) / cell), 0, n - 1); i1 = clampi((int)std::floor((x + rad - x0) / cell), 0, n - 1);
        j0 = clampi((int)std::floor((z - rad - z0) / cell), 0, n - 1); j1 = clampi((int)std::floor((z + rad - z0) / cell), 0, n - 1);
    }
    void insert(const Building& b, int idx) {
        int i0, i1, j0, j1; range(b.x, b.z, std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2, i0, i1, j0, j1);
        for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) cells[(size_t)j * n + i].push_back(idx);
    }
};
// T-02: a building's kind and size before it is placed (the town's lots read its frontage and depth first)
struct BSpec { int kind = -1; double hw = 3, hd = 3, h = 3, thick = 0.45; int style = -1; bool rect = false; };   // `rect`: a row's house, on a rectangle

struct Layout {
    RuinSpec& r; const Culture& c; const Genome& G; Rng rng; double luckS = 0; int age;
    // A-04: the people's ways in the plan: how far its houses' sizes spread (alike: a fifth of C-01's spread; varied: half again), its
    // massing's footprint and height, its appetite for height, an arcade on the next house
    double spread = 1, footK = 1, hdK = 1, heightK = 1, towerK = 1;
    bool arcadeNext = false, rectNext = false;   // the next house arcaded along its front; the next on a rectangle (a row's)
    // a town's plan: the buildings by cells, and its streets (a building placed with its fit tested keeps off them)
    BuildingGrid* grid = nullptr;
    const StreetGrid* sgrid = nullptr;
    const std::vector<Street>* streets = nullptr;
    Layout(RuinSpec& rr, const Culture& cc, uint64_t salt = 0) : r(rr), c(cc), G(cc.genome), rng(rr.seed ^ 0x5E77ULL ^ salt), age(cc.age) {
        spread = 0.2 + 1.3 * G.at(GV_SAMENESS);
        switch (G.of(GV_MASSING)) {
            case MS_COMPACT: footK = 0.88; heightK = 1.25; break;
            case MS_SPRAWLING: footK = 1.15; hdK = 1.25; heightK = 0.8; break;
            case MS_VERTICAL: footK = 0.85; heightK = 1.7; break;
            case MS_BURIED: heightK = 0.9; break;
            default: footK = 1.06; heightK = 1.15; break;   // stepped
        }
        towerK = 0.7 + 0.8 * G.at(GV_HEIGHT);
        if (age >= AGE_MEDIEVAL && age <= AGE_FUTURISTIC) heightK *= 0.82 + 0.56 * G.at(GV_STOREYS);   // its dwellings' storeys within its age's
    }
    // a house's or a villa's frontage as its people turned it to the street (a medieval or an industrial people's): its gable (a narrow,
    // deep house, its ridge running back from the street) or its eaves (a wide, shallow one, its ridge along the street)
    void frontage(BSpec& s) const {
        if (!(age == AGE_MEDIEVAL || age == AGE_INDUSTRIAL) || !(s.kind == BK_HOUSE || s.kind == BK_VILLA)) return;
        if (G.of(GV_FRONTAGE) == FR_GABLES ? s.hw > s.hd : s.hd > s.hw) std::swap(s.hw, s.hd);
    }
    double var(double lo, double span) { return lo + span * clampd(0.5 + (rng.uni() - 0.5) * spread, 0, 1); }   // a draw spread by the people's sameness
    bool fits(double x, double z, double rad) const {
        if (grid) {
            int i0, i1, j0, j1; grid->range(x, z, rad + 1.2, i0, i1, j0, j1);
            for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++)
                for (int k : grid->cells[(size_t)j * grid->n + i]) {
                    const Building& b = r.buildings[k];
                    double dx = b.x - x, dz = b.z - z, rb = std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2;
                    if (dx * dx + dz * dz < (rad + rb) * (rad + rb)) return false;
                }
            return true;
        }
        for (const Building& b : r.buildings) {
            double dx = b.x - x, dz = b.z - z, rb = std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2;
            if (dx * dx + dz * dz < (rad + rb) * (rad + rb)) return false;
        }
        return true;
    }
    // a town's buildings are tested as rectangles (a row's houses side by side, a terrace's long rows), not as their circles
    bool obb = false;
    bool fitsRect(double x, double z, double heading, double hw, double hd, double margin) const {
        const double ux = std::cos(heading), uz = -std::sin(heading), vx = std::sin(heading), vz = std::cos(heading);
        const double rad = std::sqrt(hw * hw + hd * hd);
        auto overlaps = [&](const Building& b) {
            const double dx = b.x - x, dz = b.z - z, rb = std::sqrt(b.hw * b.hw + b.hd * b.hd);
            if (dx * dx + dz * dz >= (rad + rb + margin) * (rad + rb + margin)) return false;
            const double bux = std::cos(b.heading), buz = -std::sin(b.heading), bvx = std::sin(b.heading), bvz = std::cos(b.heading);
            const double ax[4][2] = {{ux, uz}, {vx, vz}, {bux, buz}, {bvx, bvz}};
            for (const auto& a : ax) {
                const double ra = hw * std::fabs(ux * a[0] + uz * a[1]) + hd * std::fabs(vx * a[0] + vz * a[1]);
                const double rbb = b.hw * std::fabs(bux * a[0] + buz * a[1]) + b.hd * std::fabs(bvx * a[0] + bvz * a[1]);
                if (std::fabs(dx * a[0] + dz * a[1]) >= ra + rbb + margin) return false;   // a gap along this axis
            }
            return true;
        };
        if (grid) {
            int i0, i1, j0, j1; grid->range(x, z, rad + margin + 1.2, i0, i1, j0, j1);
            for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) for (int k : grid->cells[(size_t)j * grid->n + i]) if (overlaps(r.buildings[k])) return false;
            return true;
        }
        for (const Building& b : r.buildings) if (overlaps(b)) return false;
        return true;
    }
    // the buildings from index n on taken back (a town's lot that did not fit its street after all)
    void undoTo(size_t n) {
        while (r.buildings.size() > n) {
            const int idx = (int)r.buildings.size() - 1;
            if (grid) {
                const Building& b = r.buildings.back();
                int i0, i1, j0, j1; grid->range(b.x, b.z, std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2, i0, i1, j0, j1);
                for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) { std::vector<int>& v = grid->cells[(size_t)j * grid->n + i]; v.erase(std::remove(v.begin(), v.end(), idx), v.end()); }
            }
            r.buildings.pop_back();
        }
    }
    // how far the standing pieces of the buildings from index n on reach into a street's corridor where the explorer walks (their
    // colliders from the ankle to over the head: a mill's chimney, a mound's bank, a jetty, a porch's posts; what fell is walked round),
    // 0 none: what a building's own pieces reach past its footprint
    double intrusion(size_t n, const SiteRead& st, double exX = 1e18, double exZ = 1e18) const {   // (exX, exZ): the streets through it left out (a gate's own)
        if (!sgrid || !streets) return 0;
        double worst = 0;
        std::vector<Form> el; std::vector<FormDisc> discs;
        for (size_t i = n; i < r.buildings.size(); i++) {
            el.clear(); discs.clear();
            buildingForms(r.buildings[i], c, st, 0, el, (int)i);
            for (const Form& e : el) if (e.tilt <= 0.3 && e.part != 3) formColliders(e, discs);
            for (const FormDisc& d : discs) {
                if (d.y0 > 1.9 || d.y1 < 0.3) continue;
                sgrid->near(d.x - d.r - 1, d.z - d.r - 1, d.x + d.r + 1, d.z + d.r + 1, [&](int k) {
                    const Street& s = (*streets)[k];
                    if (exX < 1e17 && streetPointDist(s, exX, exZ) < s.half + 0.5) return;
                    worst = std::max(worst, s.half + 0.4 + d.r - streetPointDist(s, d.x, d.z));
                });
            }
        }
        return worst;
    }
    // a rectangle keeps off the town's streets (their corridors and half a metre)
    bool offStreets(double x, double z, double heading, double hw, double hd) const {
        if (!sgrid || !streets) return true;
        const double rad = std::sqrt(hw * hw + hd * hd) + 1;
        bool clear = true;
        sgrid->near(x - rad, z - rad, x + rad, z + rad, [&](int k) { if (clear && streetRectGap((*streets)[k], x, z, heading, hw + 0.5, hd + 0.5) < 0) clear = false; });
        return clear;
    }
    bool circleOffStreets(double x, double z, double rad) const {
        if (!sgrid || !streets) return true;
        bool clear = true;
        sgrid->near(x - rad, z - rad, x + rad, z + rad, [&](int k) { if (clear && streetPointDist((*streets)[k], x, z) < rad + (*streets)[k].half) clear = false; });
        return clear;
    }
    // a building's plan's form (its people's; another now and then where its houses vary; a court where the people built inward)
    int formOf(int kind, double x, double z) const {
        switch (kind) {
            case BK_HOUSE: case BK_HUT: case BK_VILLA: case BK_SUBURB: case BK_BLOCK: case BK_TOWER: case BK_ROTUNDA: case BK_POD: case BK_TEMPLE: break;
            default: return PF_RECT;
        }
        const uint64_t h =
                mix64(r.seed ^ 0xA04D0ULL ^ ((uint64_t)(int64_t)std::lround(x * 8) * 0x9E3779B97F4A7C15ULL) ^ ((uint64_t)(int64_t)std::lround(z * 8) * 0xD1B54A32D192ED03ULL) ^ (uint64_t)kind);
        if (rectNext) return PF_RECT;
        int f = G.of(GV_PLAN);
        const double var = G.at(GV_SAMENESS);
        if (var > 0.6 && unitFromHash(h) < (var - 0.6) * 1.5) { static const int ALT[3] = {PF_RECT, PF_CIRCLE, PF_POLYGON}; f = ALT[std::min(2, (int)(unitFromHash(mix64(h + 1)) * 3))]; }
        if (G.of(GV_PUBLIC) == PU_COURTS && age >= AGE_MEDIEVAL && (kind == BK_HOUSE || kind == BK_VILLA || kind == BK_BLOCK) && unitFromHash(mix64(h + 2)) < 0.6) f = PF_COURT;
        // an industrial or a modern people's villas, houses and blocks on the age's plans (a rectangle, an L, a court); the round and the
        // many-sided plans are the hand-made ages' and the futuristic's, kept on the people's own temples, rotundas and towers
        if (age >= AGE_INDUSTRIAL && age <= AGE_MODERN && (kind == BK_VILLA || kind == BK_SUBURB || kind == BK_BLOCK) && (f == PF_CIRCLE || f == PF_POLYGON || f == PF_ELLIPSE)) f = PF_RECT;
        switch (kind) {
            case BK_TOWER: if (f == PF_FREE || f == PF_COURT) f = PF_RECT; break;
            case BK_HUT: if (f == PF_COURT) f = PF_RECT; break;
            case BK_SUBURB: case BK_VILLA: if (f == PF_COURT && kind == BK_SUBURB) f = PF_RECT; if (f == PF_FREE && kind == BK_VILLA) f = PF_RECT; break;
            case BK_ROTUNDA: case BK_POD: if (f == PF_RECT || f == PF_FREE || f == PF_COURT) f = PF_CIRCLE; break;
            case BK_TEMPLE: if (f == PF_RECT || f == PF_FREE || f == PF_COURT) f = PF_CIRCLE; break;
            default: break;
        }
        return f;
    }
    // what a dwelling stands on, by its people's relation to the land. B-415: a pit, a cut in the rock and a terrace's kerb are the
    // hand-made ages' (an industrial row or a modern block stands on the street); a podium (a people of mounds) and a modern block's pilotis
    // (a people of stilts) are any age's
    int foundOf(int kind) const {
        const bool dwellingK = kind == BK_HOUSE || kind == BK_VILLA || kind == BK_SUBURB || kind == BK_BLOCK || kind == BK_ROW || kind == BK_TENEMENT || kind == BK_HUT;
        if (!dwellingK) return FD_GROUND;
        const bool handMade = age <= AGE_MEDIEVAL;
        switch (G.of(GV_LAND)) {
            case LD_STILTS: return (kind == BK_HOUSE && age == AGE_MEDIEVAL) || (kind == BK_BLOCK && age == AGE_MODERN) ? FD_STILTS : FD_GROUND;
            case LD_MOUNDS: return FD_MOUND;
            case LD_PITS: return handMade ? FD_PIT : FD_GROUND;
            case LD_CLIFFS: return !handMade ? FD_GROUND : (kind == BK_HUT ? FD_PIT : FD_CUT);
            case LD_TERRACED: return handMade ? FD_TERRACE : FD_GROUND;
            default: return FD_GROUND;
        }
    }
    bool add(int kind, double x, double z, double heading, double hw, double hd, double hWall, double thick, bool checkFit = true, int style = -1) {
        const int form = formOf(kind, x, z);
        if (form == PF_COURT) { const double m = kind == BK_BLOCK ? 12.0 : 6.4; hw = std::max(hw, m); hd = std::max(hd, m); }   // a court needs its room
        if (form == PF_FREE && kind == BK_HOUSE) hd = std::max(hd, 4.2);
        double rad = std::sqrt(hw * hw + hd * hd) + 1.2;
        if (checkFit && !(obb ? fitsRect(x, z, heading, hw, hd, 0.02) : fits(x, z, rad))) return false;
        if (checkFit && !offStreets(x, z, heading, hw, hd)) return false;
        Building b; b.kind = kind; b.x = x; b.z = z; b.heading = heading; b.hw = hw; b.hd = hd; b.hWall = hWall * c.tall; b.thick = thick;
        b.luck = clampd(luckS + rng.sym(0.25), -0.4, 0.4);   // C-01's decay draw: the building's fortune now
        b.door = kind == BK_HOUSE ? rng.irange(4) : 0;
        b.seed = rng.next();
        b.age = age;
        b.style = style >= 0 ? style : (kind == BK_TEMPLE ? c.temple : (kind == BK_ARCOLOGY ? c.great % 3 : 0));
        // the people's form, and others
        if (kind == BK_HIGHRISE) b.style = unitFromHash(mix64(b.seed ^ 0xA03F0ULL)) < 0.6 ? c.towerForm : std::min(3, (int)(unitFromHash(mix64(b.seed ^ 0xA03F1ULL)) * 4));
        if (kind == BK_HOUSE && arcadeNext) b.style = 1;   // an arcade along its front
        if (kind == BK_HIGHRISE || (kind == BK_TWIN && age == AGE_MODERN)) b.hWall = std::min(b.hWall, 650.0);   // the tallest a modern people raised
        // what its room reads of the genome
        b.form = kind == BK_TEMPLE && b.style != 3 ? PF_RECT : form;
        b.sides = G.number();
        b.found = foundOf(kind);
        if (b.found == FD_STILTS && b.form != PF_RECT) b.found = FD_GROUND;
        b.centred = G.of(GV_SYMMETRY) == SY_BILATERAL || G.of(GV_SYMMETRY) == SY_RADIAL || G.of(GV_PROFILE) == WP_BULGING;
        if (b.form == PF_FREE || b.form == PF_COURT) b.door = 0;
        b.before = r.before;   // a place left before the end
        r.buildings.push_back(b);
        if (grid) grid->insert(r.buildings.back(), (int)r.buildings.size() - 1);
        return true;
    }
    // a dwelling at a spot, the age's: C-01's houses (a rotunda now and then, a heap where one fell); a camp's tent rings and huts, a
    // village's longhouses; terraced rows, tenements near an industrial town's middle and villas at its edge; blocks of flats and
    // suburban houses; a futuristic people's pods. A-04: their sizes by the people's massing and sameness
    // the dwelling's kind and size alone (`d` its distance from the centre over the radius), the town's lots placing it
    bool dwellingSpec(double d, BSpec& s) {
        const double u = rng.uni();
        const double F = footK, Hk = heightK;
        auto set = [&](int k, double hw, double hd, double h, double t) { s.kind = k; s.hw = hw; s.hd = hd; s.h = h; s.thick = t; s.style = -1; frontage(s); return true; };
        switch (age) {
            case AGE_PREHISTORIC: {
                if (r.sclass == SC_HAMLET && u < 0.55) { double rr = var(2.0, 0.9); return set(BK_TENT_RING, rr, rr, 2.8, 0.2); }
                if (u < 0.8 || r.sclass == SC_HAMLET) {
                    double rr = var(2.3, 1.1) * F;
                    return set(BK_HUT, rr, rr * (G.of(GV_PLAN) == PF_ELLIPSE ? 1.35 : (G.of(GV_PLAN) == PF_RECT ? 1.3 : 1.0)), var(1.5, 0.5) * Hk, 0.5);
                }
                return set(BK_LONGHOUSE, var(2.6, 0.9) * F, var(6.0, 4.0) * F * hdK, var(1.7, 0.4) * Hk, 0.8);
            }
            case AGE_INDUSTRIAL: {
                if (u < 0.05) return set(BK_RUBBLE, 4.5, 4.5, 1.0, 0.5);
                if (r.sclass == SC_HAMLET || (r.sclass == SC_VILLAGE && u < 0.3) || (townOrGreater(r.sclass) && d > 0.8 && u < 0.4) || (G.of(GV_GROUPING) == GR_DETACHED && u < 0.5))
                    return set(BK_VILLA, var(4.5, 2.0) * F, var(4.5, 1.5) * F * hdK, var(6.5, 1.5) * Hk, 0.45);
                if (townOrGreater(r.sclass) && d < 0.55 && u < 0.45) return set(BK_TENEMENT, var(6.0, 2.0) * F, var(6.0, 2.0) * F, var(13, 6) * Hk, 0.55);
                return set(BK_ROW, var(10.0, 4.0) * F, var(4.2, 1.0) * F * hdK, var(6.5, 3.0) * Hk, 0.45);
            }
            case AGE_MODERN: {
                if (u < 0.04) return set(BK_RUBBLE, 6, 6, 1.0, 0.5);
                if ((r.sclass == SC_CITY || r.sclass == SC_CAPITAL) && d < 0.7) {   // a city's middle, its towers by the people's appetite for height, blocks round them
                    if (d < 0.4 && rng.uni() < 0.25 + 0.6 * G.at(GV_HEIGHT)) {
                        const double hw = 10 + 5 * rng.uni();
                        return set(BK_HIGHRISE, hw, hw * (0.75 + 0.25 * rng.uni()), (60 + 220 * rng.uni()) * (1.1 - d) * towerK, 0.5);
                    }
                    return set(BK_BLOCK, var(11, 4) * F, var(6.0, 2.5) * F * hdK, (18 + 30 * rng.uni() * (1.2 - d)) * Hk, 0.4);
                }
                if (r.sclass == SC_HAMLET || (r.sclass == SC_VILLAGE && u < 0.55) || (townOrGreater(r.sclass) && d > 0.85 && u < 0.5))
                    return set(BK_SUBURB, var(4.0, 1.5) * F, var(5.0, 1.5) * F * hdK, (rng.chance(0.5) ? 2.9 : 5.8) * std::min(1.4, Hk), 0.35);
                return set(BK_BLOCK, var(11, 4) * F, var(6.0, 2.5) * F * hdK, (15 + 30 * rng.uni() * (1.2 - d)) * Hk, 0.4);
            }
            case AGE_FUTURISTIC: { double rr = var(9, 5) * F; return set(BK_POD, rr, rr * (G.of(GV_PLAN) == PF_ELLIPSE ? 1.3 : 1.0), 3.4 * std::min(1.5, Hk), 0.5); }
            case AGE_FAR_FUTURE: return false;
            default: {
                if (u < 0.08) return set(BK_RUBBLE, 3.5, 3.5, 1.0, 0.5);
                if (u < 0.2 && G.of(GV_PLAN) != PF_RECT) { double rr = var(3.0, 2.5) * F; return set(BK_ROTUNDA, rr, rr, var(2.8, 1.0) * Hk, 0.5); }
                return set(BK_HOUSE, var(2.4, 1.6) * F, var(3.0, 2.0) * F * hdK, var(2.6, 0.8) * Hk, 0.45);
            }
        }
    }
    bool dwelling(double x, double z, double heading) {
        BSpec s;
        if (!dwellingSpec(std::sqrt(x * x + z * z) / std::max(1.0, r.size), s)) return false;
        return add(s.kind, x, z, heading, s.hw, s.hd, s.h, s.thick, true, s.style);
    }
    // a dwelling's spot as the people grouped its houses: alone; three joined in a row, their fronts to the street; a compound of
    // two or three round a court, a low wall round them, its gap to the street. Returns the houses placed
    int dwellingSpot(double x, double z, double heading) {
        const int gr = G.of(GV_GROUPING);
        if (age >= AGE_FUTURISTIC || gr == GR_DETACHED || (gr == GR_ROWS && age == AGE_INDUSTRIAL)) return dwelling(x, z, heading) ? 1 : 0;
        // an industrial or a modern city's middle is built up, not grouped (its tenements, its blocks and towers)
        if ((r.sclass == SC_CITY || r.sclass == SC_CAPITAL) && (age == AGE_INDUSTRIAL || age == AGE_MODERN) && std::sqrt(x * x + z * z) < 0.6 * std::max(1.0, r.size))
            return dwelling(x, z, heading) ? 1 : 0;
        const double fxh = std::sin(heading), fzh = std::cos(heading), sxh = std::cos(heading), szh = -std::sin(heading);
        if (gr == GR_ROWS) {   // three joined side by side (the medieval houses, the prehistoric longhouses, a modern street of terraced houses)
            double hw = age == AGE_MODERN ? 3.4 : (age == AGE_PREHISTORIC ? 2.2 : 2.7), hd = age == AGE_MODERN ? 5.0 : (age == AGE_PREHISTORIC ? 5.5 : 3.6);
            if (age == AGE_MEDIEVAL && G.of(GV_FRONTAGE) == FR_EAVES) std::swap(hw, hd);   // their eaves to the street
            const double step = 2 * hw + 0.05;
            if (!fits(x, z, std::sqrt(9 * hw * hw + hd * hd) + 1.0)) return 0;
            const int kind = age == AGE_MODERN ? BK_SUBURB : (age == AGE_PREHISTORIC ? BK_LONGHOUSE : BK_HOUSE);
            const double h = age == AGE_MODERN ? 5.8 : (age == AGE_PREHISTORIC ? 1.8 : var(2.8, 0.6) * heightK);
            int placed = 0;
            rectNext = true;
            for (int k = -1; k <= 1; k++) {
                const double ox = x + sxh * k * step, oz = z + szh * k * step;
                if (kind == BK_LONGHOUSE) { if (add(kind, ox * 1.0 + sxh * k * 1.6, oz + szh * k * 1.6, heading, hw, hd, h, 0.8, false)) placed++; }   // a longhouse's walls of turf apart
                else if (add(kind, ox, oz, heading, hw, hd, h, 0.45, false)) placed++;
                if (kind == BK_HOUSE && placed) r.buildings.back().door = 0;   // their doorways to the street
                if (placed) r.buildings.back().joined = true;
            }
            rectNext = false;
            return placed;
        }
        // a compound: two or three round a court, a low wall on the court's open sides. T-02: the houses drawn first and set as far out as
        // their width needs, so neighbours never overlap (a people of wide houses overlapped them, their doorways shut)
        const int n = 2 + rng.irange(2);
        const double spreadA = n == 2 ? 1.6 : 1.2;
        struct H { double hw, hd, h; } hs[3];
        double wMax = 0, dMax = 0;
        for (int k = 0; k < n; k++) {
            if (age == AGE_PREHISTORIC) { const double rr = var(2.0, 0.6); hs[k] = {rr, rr, 1.6}; }
            else if (age == AGE_MODERN) hs[k] = {var(3.4, 1.0), var(4.2, 1.0), 2.9};
            else if (age == AGE_INDUSTRIAL) hs[k] = {var(3.6, 1.0), var(3.6, 1.0), 6.5};
            else hs[k] = {var(2.2, 0.8), var(2.6, 0.8), var(2.6, 0.6) * heightK};
            wMax = std::max(wMax, std::max(hs[k].hw, age == AGE_PREHISTORIC ? hs[k].hd : 0.0)); dMax = std::max(dMax, hs[k].hd);
        }
        const double Rp = std::max((age == AGE_MODERN ? 9.0 : 6.5) * 0.62, (wMax + 0.6) / std::sin(spreadA * 0.5) + (age >= AGE_MEDIEVAL ? 0.8 : 0.0));   // a court's form (a court, an L) is wider
        const double R = Rp + dMax * 0.4;
        if (!fits(x, z, R + 2.5) || !circleOffStreets(x, z, Rp + dMax + 0.5)) return 0;
        int placed = 0;
        for (int k = 0; k < n; k++) {
            const double a = heading + PI + (k - (n - 1) * 0.5) * spreadA, hx = x + std::sin(a) * (Rp + hs[k].hd * 0.3), hz = z + std::cos(a) * (Rp + hs[k].hd * 0.3);
            const double hdg = a + PI;   // facing the court
            if (age == AGE_PREHISTORIC) { if (add(BK_HUT, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.5, false)) placed++; }
            else if (age == AGE_MODERN) { if (add(BK_SUBURB, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.35, false)) placed++; }
            else if (age == AGE_INDUSTRIAL) { if (add(BK_VILLA, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.45, false)) placed++;
            } else {
                if (add(BK_HOUSE, hx, hz, hdg, hs[k].hw, hs[k].hd, hs[k].h, 0.45, false)) {
                    placed++;
                    if (r.buildings.back().form == PF_RECT || r.buildings.back().form == PF_FREE) r.buildings.back().door = 0;
                }
            }
        }
        if (placed == 0) return 0;
        for (size_t i = r.buildings.size() - placed; i < r.buildings.size(); i++) r.buildings[i].joined = true;   // round their court
        for (int s = -1; s <= 1; s += 2)   // the court's walls on its two open sides (its gap to the street)
            add(BK_WALL, x + sxh * s * (R + 0.6) + fxh * 0.5, z + szh * s * (R + 0.6) + fzh * 0.5, heading + PI / 2, R * 0.55, 0.3, age == AGE_PREHISTORIC ? 1.0 : 1.4, 0.6, false);
        return placed;
    }
    // the centre of a radial village or a town, the age's: A-04 by the people's centre (a temple, a palace, an assembly, a market, a tomb,
    // a machine) and its ambition; `big` a town's
    bool centre(double x, double z, double heading, bool big, bool checkFit) {
        const double u = rng.uni(), A = 0.75 + 0.8 * G.at(GV_AMBITION), Ah = 0.7 + 1.0 * G.at(GV_AMBITION);
        const int ce = G.of(GV_CENTRE);
        (void)u;
        switch (age) {
            case AGE_PREHISTORIC:
                switch (ce) {
                    case CE_PALACE: return add(BK_LONGHOUSE, x, z, heading, (4.0 + 1.5 * rng.uni()) * A, (12 + 6 * rng.uni()) * A, (2.2 + 0.4 * rng.uni()) * Ah, 1.0, checkFit);
                    case CE_ASSEMBLY: { double rr = ((big ? 12 : 8) + 5 * rng.uni()) * A; return add(BK_STONE_CIRCLE, x, z, heading, rr, rr, 3.0 + 1.5 * rng.uni(), 0.5, checkFit, 1); }
                    case CE_MARKET: return add(BK_MENHIR, x, z, heading, 1.0, (12 + 8 * rng.uni()) * A, 2.6 + 1.0 * rng.uni(), 0.5, checkFit, 1);
                    case CE_TOMB: return add(BK_DOLMEN, x, z, heading, 1.6 * A, 2.4 * A, 2.2 * Ah, 0.5, checkFit, 1);
                    case CE_MACHINE: return add(BK_MENHIR, x, z, heading, 1.0, (16 + 10 * rng.uni()) * A, 2.8 + 1.5 * rng.uni(), 0.5, checkFit);
                    default: { double rr = ((big ? 10 : 6) + 6 * rng.uni()) * A; return add(BK_STONE_CIRCLE, x, z, heading, rr, rr, (2.8 + 1.5 * rng.uni()) * Ah, 0.5, checkFit); }
                }
            case AGE_INDUSTRIAL:
                switch (ce) {
                    case CE_PALACE:
                        return add(BK_TOWN_HALL, x, z, heading, (14 + 5 * rng.uni()) * A, (8.5 + 2.5 * rng.uni()) * A, (14 + 4 * rng.uni()) * Ah, 0.8, checkFit,
                                G.of(GV_FOCUS) == FO_SPIRE || G.of(GV_FOCUS) == FO_TOWER ? 2 : 0);
                    case CE_ASSEMBLY: return add(BK_TOWN_HALL, x, z, heading, (14 + 5 * rng.uni()) * A, (10 + 2.5 * rng.uni()) * A, (14 + 4 * rng.uni()) * Ah, 0.8, checkFit, 1);
                    case CE_MARKET: return add(BK_STATION, x, z, heading, (12 + 4 * rng.uni()) * A, (18 + 8 * rng.uni()) * A, 10 + 3 * rng.uni(), 0.8, checkFit, 1);
                    case CE_TOMB: { double rr = (6 + 2 * rng.uni()) * A; return add(BK_TEMPLE, x, z, heading, rr, rr, (9 + 3 * rng.uni()) * Ah, 1.0, checkFit, 3); }
                    case CE_MACHINE: return add(BK_MILL, x, z, heading, (8 + 3 * rng.uni()) * A, (15 + 7 * rng.uni()) * A, (18 + 7 * rng.uni()) * Ah, 0.7, checkFit);
                    default: return add(BK_CHURCH, x, z, heading, (6.5 + 2 * rng.uni()) * A, (13 + 6 * rng.uni()) * A, (11 + 4 * rng.uni()) * Ah, 0.8, checkFit);
                }
            case AGE_MODERN:
                switch (ce) {
                    case CE_TEMPLE: return add(BK_TEMPLE, x, z, heading, (9 + 3 * rng.uni()) * A, (18 + 6 * rng.uni()) * A, (16 + 6 * rng.uni()) * Ah, 1.1, checkFit);
                    case CE_ASSEMBLY: { double R = (40 + 25 * rng.uni()) * A; return add(BK_STADIUM, x, z, heading, R, R, (18 + 14 * rng.uni()) * Ah, 0.5, checkFit); }
                    case CE_MARKET: return add(BK_BLOCK, x, z, heading, (28 + 10 * rng.uni()) * A, (18 + 6 * rng.uni()) * A, (10 + 4 * rng.uni()) * Ah, 0.5, checkFit, 1);
                    case CE_TOMB: return add(BK_TEMPLE, x, z, heading, (10 + 3 * rng.uni()) * A, (16 + 4 * rng.uni()) * A, (14 + 4 * rng.uni()) * Ah, 1.1, checkFit, 2);
                    case CE_MACHINE: return add(BK_POWER, x, z, heading, (45 + 15 * rng.uni()) * A, (40 + 10 * rng.uni()) * A, 1, 0.5, checkFit);
                    default: {
                        double hw = 13 + 5 * rng.uni();
                        return add(BK_HIGHRISE, x, z, heading, hw, hw * (0.75 + 0.25 * rng.uni()), ((big ? 180 : 70) + (big ? 260 : 60) * rng.uni()) * Ah * towerK, 0.5, checkFit);
                    }
                }
            default:
                switch (ce) {
                    case CE_PALACE: return add(BK_HALL, x, z, heading, (10 + 3 * rng.uni()) * A, (10 + 3 * rng.uni()) * A, (6 + 2 * rng.uni()) * Ah, 0.8, checkFit, 2);
                    case CE_ASSEMBLY: { double R = ((big ? 15 : 11) + 5 * rng.uni()) * A; return add(BK_STADIUM, x, z, heading, R, R, (5.5 + 2.5 * rng.uni()) * Ah, 0.5, checkFit); }
                    case CE_MARKET: return add(BK_HALL, x, z, heading, (5.5 + 1.5 * rng.uni()) * A, (9 + 4 * rng.uni()) * A, (5 + 2 * rng.uni()) * Ah, 0.6, checkFit, 1);
                    case CE_TOMB:
                        return add(BK_TEMPLE, x, z, heading, ((big ? 7 : 5) + 2 * rng.uni()) * A, ((big ? 15 : 10) + 5 * rng.uni()) * A, ((big ? 11 : 8) + 3 * rng.uni()) * Ah, 1.1, checkFit,
                                c.roof == RF_DOMED || c.roof == RF_CONICAL ? 3 : 2);
                    case CE_MACHINE: return add(BK_WINDMILL, x, z, heading, (3.5 + 1.5 * rng.uni()) * A, (3.5 + 1.5 * rng.uni()) * A, (12 + 4 * rng.uni()) * Ah * towerK, 0.7, checkFit);
                    default:
                        if (big || u < 0.5)
                            return add(BK_TEMPLE, x, z, heading, ((big ? 7 : 5) + 2 * rng.uni()) * A, ((big ? 15 : 10) + 5 * rng.uni()) * A, ((big ? 11 : 8) + 3 * rng.uni()) * Ah, 1.1, checkFit);
                        if (u < 0.75) return add(BK_PLATFORM, x, z, heading, 6 + 3 * rng.uni(), 6 + 3 * rng.uni(), 3.3, 0.5, checkFit);
                        return add(BK_HALL, x, z, heading, 5 + 2 * rng.uni(), 8 + 4 * rng.uni(), 5 + 2 * rng.uni(), 0.7, checkFit);
                }
        }
    }
    // the people's focal accent near its centre, where the centre does not carry it already (a basilica's spires, a domed temple's dome)
    bool focusCarried() const {
        if (r.buildings.empty()) return false;
        const Building& cb = r.buildings.front();
        const int fo = G.of(GV_FOCUS);
        if (fo == FO_SPIRE) return (cb.kind == BK_TEMPLE && cb.style == 0) || cb.kind == BK_CHURCH || (cb.kind == BK_TOWN_HALL && cb.style == 2);
        if (fo == FO_DOME) return (cb.kind == BK_TEMPLE && (cb.style == 1 || cb.style == 3)) || (cb.kind == BK_TOWN_HALL && cb.style == 1);
        return false;
    }
    bool focus(double x, double z, double heading, double scale = 1.0) {
        const int fo = G.of(GV_FOCUS);
        double hw, hd, h; focusSize(c, fo, rng, hw, hd, h);
        hw *= std::sqrt(scale); hd *= std::sqrt(scale); if (fo != FO_POOL) h *= scale;
        if (fo == FO_RING) h = 2 * hw;
        return add(BK_FOCUS, x, z, heading, hw, hd, fo == FO_MAST || fo == FO_SPIRE || fo == FO_TOWER || fo == FO_OBELISK || fo == FO_COLOSSUS ? h * towerK : h, 0.5, true, fo);
    }
    // a tower toward a town's edge, the age's: C-01's tower (a keep); a standing stone; a mill with its chimney or a water tower; a tower;
    // two joined by their bridge (a people of bridges), all of them as tall as the people's appetite for height
    bool tower(double x, double z, double heading) {
        const double u = rng.uni();
        const bool twin = G.of(GV_LINKS) == LK_BRIDGES && age >= AGE_MEDIEVAL;
        switch (age) {
            case AGE_PREHISTORIC: return add(BK_MENHIR, x, z, heading, 1.0, 0.8, (4 + 4 * rng.uni()) * towerK, 0.5);
            case AGE_INDUSTRIAL:
                if (twin) { const double rr = 3 + rng.uni(); return add(BK_TWIN, x, z, heading, rr * 3.4, rr, (28 + 12 * rng.uni()) * towerK, 0.6, true); }
                if (u < 0.72) return add(BK_MILL, x, z, heading, 7 + 3 * rng.uni(), 13 + 7 * rng.uni(), (15 + 7 * rng.uni()) * towerK, 0.7);
                if (u < 0.88) return add(BK_WATER_TOWER, x, z, heading, 5 + 1.5 * rng.uni(), 5 + 1.5 * rng.uni(), (16 + 8 * rng.uni()) * towerK, 0.5);
                return add(BK_GASOMETER, x, z, heading, 13 + 6 * rng.uni(), 13 + 6 * rng.uni(), 16 + 10 * rng.uni(), 0.5);
            case AGE_MODERN:
                if (twin && u < 0.7) { const double rr = 9 + 4 * rng.uni(); return add(BK_TWIN, x, z, heading, rr * 3.2, rr, (70 + 130 * rng.uni()) * towerK, 0.5, true); }
                if (u < 0.7) { double hw = 10 + 5 * rng.uni(); return add(BK_HIGHRISE, x, z, heading, hw, hw * (0.75 + 0.25 * rng.uni()), (60 + 140 * rng.uni()) * towerK, 0.5); }
                return add(BK_MAST, x, z, heading, 3, 3, (120 + 120 * rng.uni()) * towerK, 0.5);
            default:
                if (twin) { const double rr = 2.6 + rng.uni(); return add(BK_TWIN, x, z, heading, rr * 3.6, rr, (14 + 8 * rng.uni()) * towerK, 0.7, true); }
                return add(BK_TOWER, x, z, heading, 2.2 + 1.0 * rng.uni(), 2.2 + 1.0 * rng.uni(), (9 + 7 * rng.uni()) * towerK, 0.7);
        }
    }
    // the hall beside a town's centre: C-01's; a longhouse; a station; a stadium
    bool hall(double x, double z, double heading) {
        switch (age) {
            case AGE_PREHISTORIC: return add(BK_LONGHOUSE, x, z, heading, 3.2 + 1.0 * rng.uni(), 9 + 4 * rng.uni(), 1.9 + 0.4 * rng.uni(), 0.9);
            case AGE_INDUSTRIAL: return add(BK_STATION, x, z, heading, 14 + 4 * rng.uni(), 24 + 10 * rng.uni(), 11 + 3 * rng.uni(), 0.8);
            case AGE_MODERN: { double R = 40 + 25 * rng.uni(); return add(BK_STADIUM, x, z, heading, R, R, 18 + 14 * rng.uni(), 0.5); }
            default: return add(BK_HALL, x, z, heading, 5 + 1.5 * rng.uni(), 6.0 + 1.5 * rng.uni(), 5 + 2 * rng.uni(), 0.7);
        }
    }
    // a stela's spot: A-04 the people's monument (a stela, an obelisk, a statue, an arch, a pillar); a standing stone (a trilithon for a
    // people of arches) before the medieval age
    bool stela(double x, double z, double heading, double h, bool checkFit) {
        const int mn = G.of(GV_MONUMENTS);
        if (age == AGE_PREHISTORIC) {
            if (mn == MN_ARCHES) return add(BK_FOCUS, x, z, heading, 2.4, 0.8, h * 1.1, 0.4, checkFit, FO_GATE);
            return add(BK_MENHIR, x, z, heading, 0.8, 0.6, h * (mn == MN_OBELISKS || mn == MN_PILLARS ? 1.6 : 1.1), 0.4, checkFit);
        }
        if (age >= AGE_FUTURISTIC) return false;
        switch (mn) {
            case MN_OBELISKS: return add(BK_STELA, x, z, heading, 0.6, 0.6, h, 0.3, checkFit, MN_OBELISKS);
            case MN_COLOSSI: return add(BK_STELA, x, z, heading, 0.9, 0.9, h * 1.2, 0.3, checkFit, MN_COLOSSI);
            case MN_ARCHES: return add(BK_STELA, x, z, heading, 2.6, 0.8, h, 0.3, checkFit, MN_ARCHES);
            case MN_PILLARS: return add(BK_STELA, x, z, heading, 0.7, 0.7, h, 0.3, checkFit, MN_PILLARS);
            default: return add(BK_STELA, x, z, heading, 0.45, 0.25, h, 0.3, checkFit, MN_STELAE);
        }
    }
};

// C-08: the terrain's features. A harbour where a shore is within reach (a quay along the shore, a mole out from its end into the
// water, bollards, a warehouse behind; on a dead desert world the harbour stands over the dry bed), terraces where the ground
// slopes (retaining walls along the contour between the houses, a wall's height apart), on a desert a cistern at the upslope
// edge with its channel and a crescent of wall against the wind, and cairns out along the ridge where the relief is mountain
inline bool clearOfBuildings(const RuinSpec& r, double x, double z, double margin) {
    for (const Building& b : r.buildings) {
        double dx = x - b.x, dz = z - b.z;
        double lx = dx * std::cos(b.heading) - dz * std::sin(b.heading), lz = dx * std::sin(b.heading) + dz * std::cos(b.heading);
        if (std::fabs(lx) < b.hw + margin && std::fabs(lz) < b.hd + margin) return false;
    }
    return true;
}

// ---- T-02: the towns ------------------------------------------------------------------------------------------------------------

// what is known of a place's past: the ages it was built in, inside out (B-414: one, its people's last: a world shows the age its people
// reached alone, T-02's old cores and a place left in an older age's way are gone with that age), the years before its people's end it was
// left (0: used to the end), its founding year (the history's; 0 the countryside's), and whether it was left before its people's last age
// began (`gone`: nothing of it on the ground)
struct TownHist { std::vector<int> ages; double before = 0; int founded = 0; bool gone = false; };

// ---- each part's functions the others call, defined once in it

// the class's size (ruins.cpp; the cells and the settlement's layout read it)
void settlementSize(RuinSpec& r, Rng& rng, int& n, double& R, const Culture& c, MonPick* mon = nullptr);
// the world's plan, kept a thread (ruins.cpp: its state there alone)
std::shared_ptr<const WorldPlan> planFor(const BodyGen& g);
// a settlement (ruins_settlement.cpp)
void addFeatures(Layout& L, RuinSpec& r, const Culture& c);
void layoutSettlement(RuinSpec& r, const Culture& c, const BodyGen& g, const TownHist* th = nullptr);
// a town (ruins_towns.cpp)
void placeFurniture(Layout& L, RuinSpec& r, double R);
void layoutTown(RuinSpec& r, const Culture& c, const BodyGen& g, int n, double R, const TownHist& th, double luckS);
