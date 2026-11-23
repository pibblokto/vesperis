// J-10: a seeded world's overgrowth on its ruins (IDEAS-seeded-worlds.md section 7). The cover climbs each building from its foot as the steps
// go on (drawn as the forms' second bank: forms_draw.cpp); membranes grow across the openings it climbed past, nodules on the walls under it,
// drapes from the tops it nearly reached. A stone rotter's share of a settlement crumbles it (`Culture::stoneRot`)
#include "surface_view.h"
#include "galaxy/drainage.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace {
// the cover's height up a wall `since` steps after its edge passed (its foot at the edge), faster as it goes on: a house's walls taken in about
// six steps (10 m), a tower's in twelve (36 m), a capital's tallest in about twenty-five
double climbOf(double since) { return 0.3 + since + 0.012 * since * since * since; }

// building b's pieces of the cover (its pieces E[i0, i1) read), into `out` after them; `openings` the cell's and its arches' bays after them (a
// membrane's index among them is its key with the cell's: one the explorer tore stays torn)
void growPieces(const SurfaceView::RuinCell& c, int b, const std::vector<Form>& E, size_t i0, size_t i1, const MadeGrowth& G, const std::vector<FormOpening>& openings,
                const std::unordered_set<uint64_t>& torn, std::vector<Form>& out) {
    uint64_t h = mix64(c.spec.buildings[b].seed ^ 0x6E0E50ULL);
    auto U = [&]() { h = mix64(h + 0x9E3779B97F4A7C15ULL); return hash01(h); };
    auto piece = [&](int kind, int growth, double x, double z, double heading, double hx, double hz, double y0, double y1) -> Form& {
        Form f; f.kind = kind; f.part = FORM_PART_GROWTH; f.growth = growth; f.x = x; f.z = z; f.heading = heading; f.hx = hx; f.hz = hz;
        f.y0 = y0; f.y1 = y1; f.building = b; f.seed = (uint32_t)h; f.material = -1;
        out.push_back(f);
        return out.back();
    };
    // membranes across the openings it climbed half way up (each opening's own wall's climb), an arch's shaped to its head
    for (size_t k = 0; k < openings.size(); k++) {
        const FormOpening& o = openings[k];
        if (o.building != b || SurfaceView::pieceClimb(c, b, o.x, o.z, (uint32_t)k) < o.y0 + 0.5 * (o.y1 - o.y0)) continue;
        Form& f = piece(FK_BOX, FG_MEMBRANE, o.x, o.z, o.heading, o.half + 0.04, 0.03, o.y0 - 0.05, o.y1 + 0.03);
        f.p[0] = o.arch + 1; f.p[1] = o.spring + 0.05; f.p[2] = o.rise; f.p[3] = (double)k;
        if (torn.count(SurfaceView::membraneKey(c, (int)k))) f.broken = 1;   // (torn: drawn slit, and never torn again)
    }
    // nodules on the walls under it (a coarse grain's bigger), drapes down the faces of the tops it nearly reached
    const double size = 0.6 + 0.8 * G.grain;
    int nodules = 0, drapes = 0;
    for (size_t k = i0; k < i1; k++) {
        const Form& w = E[k];
        if (w.kind != FK_BOX || w.part != 0 || std::fabs(w.tilt) > 0.12 || w.absolute) continue;
        const double climb = SurfaceView::pieceClimb(c, b, w.x, w.z, w.seed);   // (this wall's)
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

uint64_t SurfaceView::membraneKey(const RuinCell& c, int opening) { return mix64(c.key ^ ((uint64_t)(opening + 1) * 0x9E3779B97F4A7C15ULL)); }

double SurfaceView::pieceClimb(const RuinCell& c, int b, double x, double z, uint32_t seed) {
    if (b < 0 || b >= (int)c.climb.size() || c.climb[b] < 0) return -1;
    const int nb = (int)c.spec.buildings.size();
    const double bx = b < nb ? c.spec.buildings[b].x : 0, bz = b < nb ? c.spec.buildings[b].z : 0;
    const double dx = x - bx, dz = z - bz, d = std::hypot(dx, dz);
    const double side = d > 0.5 ? (dx * c.towardX + dz * c.towardZ) / d * std::min(1.0, d / 3.0) : 0.0;   // (-1 its far side .. 1 its near)
    return c.climb[b] * (1 + 0.35 * side) * (0.9 + 0.2 * hash01(mix64((uint64_t)seed ^ 0xC11B1ULL)));
}

double SurfaceView::stoneRotOf(double x, double z) const {
    const MadeWorld& m = ruinsMade;
    if (!m.on() || !site.sys || site.body < 0) return 0;
    const double Rkm = site.sys->bodies[site.body].radiusKm;
    const Vec3 u = site.unitAt(x, z);
    double rot = 0;
    SurfaceSample here; bool sampled = false;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeSpread& s = m.spread[i];
        if (s.pits <= 0) continue;
        const double share = madeSpreadAt(m, i, u, site.gen.seed);
        if (share <= 0) continue;
        if (s.tLo > -1e8 || s.tHi < 1e8) {   // KI-480: only where its line lives (its band of warmth at the building's place, its height as generated)
            if (!sampled) { DrainageOff off; here = sampleSurface(site.gen, u, 16); sampled = true; }
            if (!madeLives(s, false, 0, madeMeanAt(site.gen, m, u, here, false, 0))) continue;
        }
        const double d = std::acos(clampd(dot(u, s.place), -1, 1));
        int a = 1;   // the step its spread came
        while (a < MADE_STEPS && madeReachOfStep(a, Rkm) < d) a++;
        rot = std::max(rot, share * clampd((s.pits - a) / 12.0, 0, 1));   // crumbled over an hour of its steps
    }
    return rot;
}

bool SurfaceView::stoneRotsOf(RuinCell& c) const {
    bool changed = false;
    for (Building& B : c.spec.buildings) {
        const double rot = stoneRotOf(c.r.x + B.x, c.r.z + B.z);
        changed = changed || rot != B.stoneRot;
        B.stoneRot = rot;
    }
    return changed;
}

void SurfaceView::layRuinPieces(RuinCell& c) const {
    const int k = c.spec.people == 1 && peoples > 1 ? 1 : 0;
    stoneRotsOf(c);
    c.openings.clear();
    for (int l = 0; l < 3; l++) ruinElements(c.spec, cultures[k], l, c.elems[l], l == 0 ? &c.openings : nullptr);
    graveElements(c.graves, cultures[k], 0, c.elems[0], c.spec.work >= 0);
}

void SurfaceView::regrowRuins() {
    joinAhead();
    supportMeshes.clear(); supports.clear();
    auto regrow = [&](RuinCell& c, bool laid) {
        if (!c.has) return;
        if (laid && stoneRotsOf(c)) layRuinPieces(c);   // (its stone eaten further: its pieces laid out again)
        overgrowCell(c); groupPieces(c);
        for (std::vector<FootCache>& f : c.foot) f.clear();
    };
    for (auto& kv : ruinCells) regrow(kv.second, true);
    for (auto& kv : workCells) regrow(kv.second, true);
    for (RuinCell& c : placedCells) regrow(c, false);   // (tests: their pieces as placed)
    ruinsKey = ruinsMade.key;
}

void SurfaceView::overgrowCell(RuinCell& c) const {
    std::vector<Form>& E = c.elems[0];
    E.erase(std::remove_if(E.begin(), E.end(), [](const Form& f) { return f.part == FORM_PART_GROWTH; }), E.end());
    c.climb.clear();
    const MadeWorld& m = ruinsMade;
    if (!c.has || !m.on() || !m.grown() || m.growthLine < 0 || m.growthLine >= MADE_LINES) return;
    {   // a settlement on the ground the cover lies on: a snowfield's ruins (its centre's) stay bare
        DrainageOff off;
        const int mat = sampleSurface(site.gen, site.unitAt(c.r.x, c.r.z), 16).material;
        if (mat == MAT_WATER || mat == MAT_SNOW || mat == MAT_ICE || mat == MAT_LAVA) return;
    }
    const int nb = (int)c.spec.buildings.size();
    c.climb.assign(nb + 1, -1.f);
    c.towardX = c.towardZ = 0;
    {   // the way it came: toward the place of the line whose cover lies at the settlement's middle
        int line = -1;
        madeGrowthAt(m, site.unitAt(c.r.x, c.r.z), site.gen.seed, line);
        c.growthLine = line >= 0 && line < MADE_LINES && m.spread[line].growth.on() ? line : m.growthLine;   // (KI-473: its cover's own line)
        if (line >= 0 && line < MADE_LINES) {
            double px, pz; site.localAt(m.spread[line].place, px, pz);
            const double d = std::hypot(px - c.r.x, pz - c.r.z);
            if (d > 50) { c.towardX = (float)((px - c.r.x) / d); c.towardZ = (float)((pz - c.r.z) / d); }
        }
    }
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
    std::vector<FormOpening> openings = c.openings;   // KI-478: and the bays of its arches (gates, arcades, porches, the works' arches)
    for (const Form& f : E) if (f.kind == FK_ARCH && f.part != FORM_PART_GROWTH) archOpenings(f, openings);
    std::vector<Form> out;
    out.reserve(E.size() + E.size() / 3);
    size_t i = 0;
    for (int b = 0; b < nb; b++) {   // the pieces come building by building (`groupPieces`), each building's cover after its own
        const size_t i0 = i;
        while (i < E.size() && E[i].building == b) out.push_back(E[i++]);
        if (c.climb[b] > 0) growPieces(c, b, E, i0, i, G, openings, tornMembranes, out);
    }
    while (i < E.size()) out.push_back(E[i++]);   // the graves' stones
    E.swap(out);
}

void SurfaceView::testRegrowRuins() {
    ruinsMade = site.gen.made;   // (the layer the harness set on the ground)
    regrowRuins();
}
