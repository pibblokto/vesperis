// J-10: a seeded world's overgrowth on its ruins (IDEAS-seeded-worlds.md section 7). The cover climbs each building from its foot as the steps
// go on (drawn as the forms' second bank: forms_draw.cpp); membranes grow across the openings it climbed past, nodules on the walls under it,
// drapes from the tops it nearly reached. A stone rotter's share of a settlement crumbles it (`Culture::stoneRot`)
#include "surface_view.h"
#include "galaxy/drainage.h"
#include <algorithm>
#include <cmath>

namespace {
// the cover's height up a wall `since` steps after its edge passed (its foot at the edge), faster as it goes on: a house's walls taken in about
// six steps (10 m), a tower's in twelve (36 m), a capital's tallest in about twenty-five
double climbOf(double since) { return 0.3 + since + 0.012 * since * since * since; }

// building b's pieces of the cover (its pieces E[i0, i1) read), into `out` after them
void growPieces(const SurfaceView::RuinCell& c, int b, const std::vector<Form>& E, size_t i0, size_t i1, const MadeGrowth& G, std::vector<Form>& out) {
    const double climb = c.climb[b];
    uint64_t h = mix64(c.spec.buildings[b].seed ^ 0x6E0E50ULL);
    auto U = [&]() { h = mix64(h + 0x9E3779B97F4A7C15ULL); return hash01(h); };
    auto piece = [&](int kind, int growth, double x, double z, double heading, double hx, double hz, double y0, double y1) -> Form& {
        Form f; f.kind = kind; f.part = FORM_PART_GROWTH; f.growth = growth; f.x = x; f.z = z; f.heading = heading; f.hx = hx; f.hz = hz;
        f.y0 = y0; f.y1 = y1; f.building = b; f.seed = (uint32_t)h; f.material = -1;
        out.push_back(f);
        return out.back();
    };
    // membranes across the openings it climbed half way up
    for (const FormOpening& o : c.openings)
        if (o.building == b && climb >= o.y0 + 0.5 * (o.y1 - o.y0)) piece(FK_BOX, FG_MEMBRANE, o.x, o.z, o.heading, o.half + 0.04, 0.03, o.y0 - 0.05, o.y1 + 0.03);
    // nodules on the walls under it (a coarse grain's bigger), drapes down the faces of the tops it nearly reached
    const double size = 0.6 + 0.8 * G.grain;
    int nodules = 0, drapes = 0;
    for (size_t k = i0; k < i1; k++) {
        const Form& w = E[k];
        if (w.kind != FK_BOX || w.part != 0 || std::fabs(w.tilt) > 0.12 || w.absolute) continue;
        const bool runsX = w.hx >= w.hz;   // the wall runs across its heading (its hx) or along it (its hz)
        const double len = 2 * std::max(w.hx, w.hz), thin = std::min(w.hx, w.hz), top = w.y1;
        if (len < 1.2 || thin > 1.5 || top - w.y0 < 0.8) continue;
        const double ch = std::cos(w.heading), sh = std::sin(w.heading);
        auto at = [&](double t, double side, double& x, double& z) {   // t along its run, `side` out of a face, in the settlement's metres
            const double a = runsX ? t : side, bb = runsX ? side : t;
            x = w.x + ch * a + sh * bb; z = w.z - sh * a + ch * bb;
        };
        // (the nodules ahead of the cover's edge on the bare wall, a few metres over it, and a little under it)
        const double lo = std::max(w.y0 + 0.2, 0.6 * climb), hi = std::min(top - 0.1, climb + 2.5);
        if (hi - lo > 0.3 && nodules < 10) {
            const int n = std::min(3, (int)(U() * (0.5 + len * (hi - lo) / 14.0)));
            for (int j = 0; j < n; j++, nodules++) {
                const double r = size * (0.18 + 0.32 * U()), y = lo + (hi - lo) * U(), side = (U() < 0.5 ? -1 : 1) * (thin + 0.35 * r);
                double x, z; at((U() * 2 - 1) * std::max(0.0, len * 0.5 - r), side, x, z);
                Form& f = piece(FK_BLOB, FG_NODULE, x, z, U() * TAU, r, r * (0.8 + 0.4 * U()), y - r * 0.75, y + r * 0.75);
                f.p[0] = (int)(U() * 3.99); f.p[1] = 0.3 + 0.5 * U();
            }
        }
        if (climb > 0.3 * top && climb < top && drapes < 8) {   // (it came over the top from the rooms inside)
            const int n = std::min(3, 1 + (int)(len / 5.0 * U()));
            for (int j = 0; j < n; j++, drapes++) {
                const double wd = 0.35 + 0.5 * U(), side = (U() < 0.5 ? -1 : 1) * (thin + 0.02);
                double x, z; at((U() * 2 - 1) * std::max(0.0, len * 0.5 - wd), side, x, z);
                const double hang = std::max(0.6, 2 * (top - climb) * (0.6 + 0.8 * U()));   // (cut out under its middle: its top half shows, ragged)
                piece(FK_BOX, FG_DRAPE, x, z, w.heading, runsX ? wd : 0.015, runsX ? 0.015 : wd, top - hang, top + 0.02);
            }
        }
    }
}
}   // namespace

double SurfaceView::stoneRotOf(double x, double z) const {
    const MadeWorld& m = site.gen.made;
    if (!m.on() || !site.sys || site.body < 0) return 0;
    const double Rkm = site.sys->bodies[site.body].radiusKm;
    const Vec3 u = site.unitAt(x, z);
    double rot = 0;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeSpread& s = m.spread[i];
        if (s.pits <= 0) continue;
        const double share = madeSpreadAt(m, i, u, site.gen.seed);
        if (share <= 0) continue;
        const double d = std::acos(clampd(dot(u, s.place), -1, 1));
        int a = 1;   // the step its spread came
        while (a < MADE_STEPS && madeReachOfStep(a, Rkm) < d) a++;
        rot = std::max(rot, share * clampd((s.pits - a) / 12.0, 0, 1));   // crumbled over an hour of its steps
    }
    return rot;
}

void SurfaceView::overgrowCell(RuinCell& c) const {
    std::vector<Form>& E = c.elems[0];
    E.erase(std::remove_if(E.begin(), E.end(), [](const Form& f) { return f.part == FORM_PART_GROWTH; }), E.end());
    c.climb.clear();
    const MadeWorld& m = site.gen.made;
    if (!c.has || !m.on() || !m.grown() || m.growthLine < 0 || m.growthLine >= MADE_LINES) return;
    {   // a settlement on the ground the cover lies on: a snowfield's ruins (its centre's) stay bare
        DrainageOff off;
        const int mat = sampleSurface(site.gen, site.unitAt(c.r.x, c.r.z), 16).material;
        if (mat == MAT_WATER || mat == MAT_SNOW || mat == MAT_ICE || mat == MAT_LAVA) return;
    }
    const int nb = (int)c.spec.buildings.size();
    c.climb.assign(nb + 1, -1.f);
    bool any = false;
    for (int b = 0; b <= nb; b++) {   // (nb: the graves' stones, at the settlement's centre)
        double x = c.r.x, z = c.r.z; uint64_t hs = c.spec.seed;
        if (b < nb) { x += c.spec.buildings[b].x; z += c.spec.buildings[b].z; hs = c.spec.buildings[b].seed; }
        const double since = madeGrowthSince(m, site.unitAt(x, z), site.gen.seed);
        if (since < 0) continue;
        c.climb[b] = (float)(climbOf(since) * (0.85 + 0.3 * hash01(mix64(hs ^ 0xC11B0ULL))));
        any = true;
    }
    if (!any) { c.climb.clear(); return; }
    const MadeGrowth& G = m.spread[m.growthLine].growth;
    std::vector<Form> out;
    out.reserve(E.size() + E.size() / 3);
    size_t i = 0;
    for (int b = 0; b < nb; b++) {   // the pieces come building by building (`groupPieces`), each building's cover after its own
        const size_t i0 = i;
        while (i < E.size() && E[i].building == b) out.push_back(E[i++]);
        if (c.climb[b] > 0) growPieces(c, b, E, i0, i, G, out);
    }
    while (i < E.size()) out.push_back(E[i++]);   // the graves' stones
    E.swap(out);
}

void SurfaceView::testRegrowRuins() {
    supportMeshes.clear(); supports.clear();
    auto regrow = [&](RuinCell& c) {
        if (!c.has) return;
        overgrowCell(c); groupPieces(c);
        for (std::vector<FootCache>& f : c.foot) f.clear();
    };
    for (auto& kv : ruinCells) regrow(kv.second);
    for (auto& kv : workCells) regrow(kv.second);
    for (RuinCell& c : placedCells) regrow(c);
}
