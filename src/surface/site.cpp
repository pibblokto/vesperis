#include "site.h"
#include "galaxy/plan.h"
#include "galaxy/towns.h"
#include "galaxy/peoples.h"
#include "core/parallel.h"
#include <chrono>
#include "space/space_view.h"
#include <cmath>

void TerrainCache::invalidate(double x0, double z0, double x1, double z1) {   // T-02
    if (v.empty()) return;
    const int n = 1 << logN;
    const int i0 = (int)std::floor(x0 / cellSize), i1 = (int)std::ceil(x1 / cellSize), j0 = (int)std::floor(z0 / cellSize), j1 = (int)std::ceil(z1 / cellSize);
    if (i1 - i0 >= 4 * n || j1 - j0 >= 4 * n) { for (TerrainVertex& t : v) t.cx = 0x7fffffff; return; }
    for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) { TerrainVertex& t = v[index(i, j)]; if (t.cx == i && t.cz == j) t.cx = 0x7fffffff; }
}

void TerrainCache::init(const SurfaceSite* s, int cell, int logn) {
    site = s; cellSize = cell; logN = logn;
    v.assign((size_t)1 << (2 * logN), TerrainVertex());
}

const TerrainVertex& TerrainCache::at(int cx, int cz) {
    TerrainVertex& t = v[index(cx, cz)];
    if (t.cx != cx || t.cz != cz) {
        t = site->sampleAt((double)cx * cellSize, (double)cz * cellSize, cellSize);
        t.cx = cx; t.cz = cz;
    }
    return t;
}

TerrainVertex SurfaceSite::sampleAt(double x, double z, double detailM) const {
    Vec3 u = unitAt(x, z);
    SurfaceSample s = sampleSurface(gen, u, detailM);
    TerrainVertex t;
    t.h = (float)s.height;
    t.albedo = (float)s.albedo;
    t.veg = (float)s.veg;
    t.material = (uint8_t)s.material;
    t.glow = (uint8_t)(s.glow * 255);
    t.biome = (uint8_t)s.biome;
    t.water = (float)s.water;
    t.shore = (float)s.shore;
    t.scree = (uint8_t)clampi((int)(s.scree * 255 + 0.5), 0, 255);
    if (!edits.empty()) applyEdits(x, z, detailM, t);   // T-03: the works' cuts, pits, ditches, craters, flats, reservoirs and lines
    if (!roads.empty() && detailM <= 100) {   // C-09: an old road through this vertex (the coarse rings see nothing: a seven-metre road is a hundredth of their cells)
        double d, along, hd; const SiteRoad* rd;
        if (roadAt(x, z, 0.5 * detailM + 8.0, d, along, hd, rd)) {
            double half = rd->half;
            double overlap = clampd(half + 0.5 * detailM - d, 0, std::min(2 * half, detailM));   // the road's share of the footprint: a box across it
            double cover = overlap / detailM * roadLeft(rd->id, along, rd->wear);                   // less what the wear took
            if (cover > 0.002) {
                t.road = (uint8_t)clampi((int)(cover * 255 + 0.5), 1, 255);
                bool dry = t.material != MAT_WATER && t.material != MAT_ICE && t.material != MAT_SNOW && t.material != MAT_LAVA && t.material != MAT_GLASS && !(t.water > -1e8f && t.h < t.water + 0.3f);
                double maxCover = std::min(1.0, 2 * half / detailM);
                if (dry && detailM <= 24 && cover >= 0.5 * maxCover) {   // on the bed (the fine rings): the paving, or the beaten earth, bare and darker
                    int fam = matFamily(t.material);
                    switch (rd->kind) {   // T-01: the networks' beds by their kind
                        case NK_RAIL: if (fam != FAM_ROCK) t.material = (uint8_t)familyRep(gen.type, FAM_ROCK); t.albedo *= 0.55f; break;   // the ballast, dark with the years' soot
                        case NK_CANAL: if (fam != FAM_SAND) t.material = (uint8_t)familyRep(gen.type, FAM_SAND); t.albedo *= 0.6f; break;   // the channel's silted bed
                        case NK_HIGHWAY: if (fam != FAM_ROCK) t.material = (uint8_t)familyRep(gen.type, FAM_ROCK); t.albedo *= fam == FAM_ROCK ? 0.85f : 1.0f; break;   // the slabs, pale
                        case NK_PATH: if (fam == FAM_GRASS || fam == FAM_FOREST) t.material = (uint8_t)familyRep(gen.type, FAM_SAND); t.albedo *= 0.9f; break;   // a worn line
                        default:
                            if (rd->paved) { if (fam != FAM_ROCK) t.material = (uint8_t)familyRep(gen.type, FAM_ROCK); t.albedo *= fam == FAM_ROCK ? 0.75f : 0.9f; }   // the paving: the rock family (on rock ground, darker stone)
                            else if (fam == FAM_GRASS || fam == FAM_FOREST) { t.material = (uint8_t)familyRep(gen.type, FAM_SAND); t.albedo *= 0.85f; }   // the beaten track: bare earth through the green
                            else { if (t.material == MAT_SAND) t.material = MAT_DUST; t.albedo *= 0.5f; }   // beaten sand is packed and dark (the dust tile: no ripples), beaten rock darker; a fifth less read as the sand's own patches
                            break;
                    }
                    t.veg *= rd->kind == NK_PATH ? 0.4f : 0.1f;
                } else if (dry) {   // the line from the air (the 64 m ring): the tone, darker the more of the footprint it holds
                    double k = std::min(1.0, cover * std::max(1.0, detailM / 16.0));
                    t.albedo *= (float)(1 - (rd->kind == NK_HIGHWAY ? 0.15 : 0.28) * k);
                    t.veg *= (float)(1 - 0.8 * k);
                }
            }
        }
    }
    if (!marks.empty() && detailM <= 100) applyMarks(x, z, detailM, t);   // T-02: the towns' streets, squares and fields
    return t;
}

// ---- T-02: the towns' ground ----------------------------------------------------------------------------------------------------
namespace {
constexpr double MARK_CELL = 32;
inline int64_t markKey(int64_t i, int64_t j) { return (i << 32) ^ (j & 0xffffffffLL); }
double segDist(double x, double z, double ax, double az, double bx, double bz, double* along = nullptr) {
    const double dx = bx - ax, dz = bz - az, L2 = dx * dx + dz * dz;
    const double t = L2 > 1e-12 ? clampd(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0;
    if (along) *along = t * std::sqrt(L2);
    return std::hypot(x - (ax + dx * t), z - (az + dz * t));
}
}
void SurfaceSite::addTownMarks(const RuinSpec& sp, double ox, double oz, double years, double wet, double wear) {
    double x0 = 1e18, z0 = 1e18, x1 = -1e18, z1 = -1e18;
    auto index = [&](int k, double ax, double az, double bx, double bz) {
        x0 = std::min(x0, ax); z0 = std::min(z0, az); x1 = std::max(x1, bx); z1 = std::max(z1, bz);
        for (int64_t j = (int64_t)std::floor(az / MARK_CELL); j <= (int64_t)std::floor(bz / MARK_CELL); j++)
            for (int64_t i = (int64_t)std::floor(ax / MARK_CELL); i <= (int64_t)std::floor(bx / MARK_CELL); i++) markCells[markKey(i, j)].push_back(k);
    };
    // what the years left: a paved street stands its wear, a beaten one less, a field's tone and its edges fade in centuries (a dry world's
    // last longer)
    const double y = std::max(1.0, years);
    const float paved = (float)clampd(1 - 0.75 * wear, 0.15, 1), beaten = (float)clampd(1 - 1.2 * wear, 0.1, 1);
    const float field = (float)clampd(1 - (std::log10(std::max(100.0, y)) - 2) / (wet > 0.4 ? 1.4 : 2.2), 0.08, 1);
    for (const Street& s : sp.streets) {
        GroundMark m; m.kind = 0; m.sub = s.paved ? (s.age >= AGE_MODERN ? 2 : 1) : 0;
        m.ax = ox + s.ax; m.az = oz + s.az; m.bx = ox + s.bx; m.bz = oz + s.bz; m.half = s.half; m.left = s.paved ? paved : beaten;
        m.id = (uint32_t)mix64(sp.seed ^ (uint64_t)marks.size());
        const double e = s.half + 1;
        marks.push_back(m);
        index((int)marks.size() - 1, std::min(m.ax, m.bx) - e, std::min(m.az, m.bz) - e, std::max(m.ax, m.bx) + e, std::max(m.az, m.bz) + e);
    }
    for (const Plaza& p : sp.plazas) {
        GroundMark m; m.kind = 1; m.sub = p.age >= AGE_MODERN ? 2 : (p.age >= AGE_MEDIEVAL ? 1 : 0); m.ax = ox + p.x; m.az = oz + p.z; m.r = p.r; m.left = m.sub ? paved : beaten;
        marks.push_back(m);
        index((int)marks.size() - 1, m.ax - m.r - 1, m.az - m.r - 1, m.ax + m.r + 1, m.az + m.r + 1);
    }
    for (const FieldPatch& f : sp.fields) {
        GroundMark m; m.kind = f.kind == FI_CIRCLE ? 3 : 2; m.sub = f.kind; m.ax = ox + f.x; m.az = oz + f.z; m.r = f.hw; m.hw = f.hw; m.hd = f.hd; m.heading = f.heading;
        m.left = f.kind == FI_TERRACE ? std::max(field, 0.35f) : field;   // a terrace's walls outlast a field's tone
        m.id = (uint32_t)mix64(sp.seed ^ 0xF1E1DULL ^ (uint64_t)marks.size());
        const double e = std::hypot(f.hw, f.hd) + 2;
        marks.push_back(m);
        index((int)marks.size() - 1, m.ax - e, m.az - e, m.ax + e, m.az + e);
    }
    if (x0 > x1) return;
    // the fine rings' vertices under it sampled again (the coarse ones never draw it)
    for (TerrainCache* c : {&lodN, &lod0, &lod1}) c->invalidate(x0 - c->cellSize, z0 - c->cellSize, x1 + c->cellSize, z1 + c->cellSize);
}

double SurfaceSite::markCover(double x, double z) const {
    if (markCells.empty()) return 0;
    auto it = markCells.find(markKey((int64_t)std::floor(x / MARK_CELL), (int64_t)std::floor(z / MARK_CELL)));
    if (it == markCells.end()) return 0;
    double best = 0;
    for (int k : it->second) {
        const GroundMark& m = marks[k];
        if (m.kind == 0) best = std::max(best, m.left * clampd(m.half + 1.0 - segDist(x, z, m.ax, m.az, m.bx, m.bz), 0, 1));
        else if (m.kind == 1) best = std::max(best, m.left * clampd(m.r + 1.0 - std::hypot(x - m.ax, z - m.az), 0, 1));
    }
    return best;
}

void SurfaceSite::applyMarks(double x, double z, double detailM, TerrainVertex& t) const {
    auto it = markCells.find(markKey((int64_t)std::floor(x / MARK_CELL), (int64_t)std::floor(z / MARK_CELL)));
    if (it == markCells.end()) return;
    const bool dry = t.material != MAT_WATER && t.material != MAT_ICE && t.material != MAT_SNOW && t.material != MAT_LAVA && t.material != MAT_GLASS && !(t.water > -1e8f && t.h < t.water + 0.3f);
    if (!dry) return;
    // the street or the square under the vertex (its share of the footprint), and the field it lies in
    double cover = 0, maxCover = 1; int paved = 1;
    const GroundMark* fld = nullptr; double lx = 0, lz = 0;
    for (int k : it->second) {
        const GroundMark& m = marks[k];
        if (m.kind == 0) {
            const double d = segDist(x, z, m.ax, m.az, m.bx, m.bz);
            const double c = clampd(m.half + 0.5 * detailM - d, 0, std::min(2 * m.half, detailM)) / detailM * m.left;
            if (c > cover) { cover = c; paved = m.sub; maxCover = std::min(1.0, 2 * m.half / detailM) * m.left; }
        } else if (m.kind == 1) {
            const double c = clampd((m.r + 0.5 * detailM - std::hypot(x - m.ax, z - m.az)) / detailM, 0, 1) * m.left;
            if (c > cover) { cover = c; paved = m.sub; maxCover = m.left; }
        } else if (!fld) {
            const double ex = x - m.ax, ez = z - m.az;
            if (m.kind == 3) { if (std::hypot(ex, ez) <= m.r) { fld = &m; lx = std::hypot(ex, ez); } }
            else {
                const double ch = std::cos(m.heading), sh = std::sin(m.heading), a = ex * ch - ez * sh, b = ex * sh + ez * ch;
                if (std::fabs(a) <= m.hw && std::fabs(b) <= m.hd) { fld = &m; lx = a; lz = b; }
            }
        }
    }
    if (cover > 0.002) {   // the street's bed: the paving (the rock family, darker on rock ground) or the beaten earth, as C-09's roads
        t.road = (uint8_t)clampi((int)(cover * 255 + 0.5), std::max<int>(t.road, 1), 255);
        if (detailM <= 24 && cover >= 0.5 * maxCover) {
            const int fam = matFamily(t.material);
            // the cut is deep: a fifth of the albedo vanishes in the ground's tone tile (C-09's lesson)
            if (paved == 2) { if (fam != FAM_ROCK) t.material = (uint8_t)familyRep(gen.type, FAM_ROCK); t.albedo *= 0.45f; }   // a modern street's asphalt and concrete
            else if (paved) { if (fam != FAM_ROCK) t.material = (uint8_t)familyRep(gen.type, FAM_ROCK); t.albedo *= 0.6f; }   // setts, flags
            else if (fam == FAM_GRASS || fam == FAM_FOREST) { t.material = (uint8_t)familyRep(gen.type, FAM_SAND); t.albedo *= 0.85f; }
            else { if (t.material == MAT_SAND) t.material = MAT_DUST; t.albedo *= 0.6f; }
            t.veg *= 0.1f;
        } else {   // the streets from the air (the 64 m ring): the tone
            const double k = std::min(1.0, cover * std::max(1.0, detailM / 16.0));
            t.albedo *= (float)(1 - 0.36 * k);
            t.veg *= (float)(1 - 0.75 * k);
        }
        return;
    }
    if (!fld) return;
    const float L = fld->left;
    const bool green = gen.type != PT_DESERT;
    if (fld->kind == 3) {   // an irrigation circle: its old soil darker on a desert, greener on a living world; its rim (the cuts are deep: a fifth
                            // of the albedo is lost in the ground's tone tile)
        const double rim = clampd(1 - std::fabs(lx - fld->r + 2) / (1.5 + 0.25 * detailM), 0, 1);
        if (green) { t.veg = std::min(1.0f, t.veg * (1 + 0.6f * L) + 0.08f * L); t.albedo *= 1 - 0.12f * L; } else t.albedo *= 1 - 0.38f * L;
        t.albedo *= (float)(1 - 0.2 * rim * L);
        return;
    }
    // a field: its strips (alternate tones across it), a plot's or an enclosure's own tone, its edge a hedge on a living world, a wall or a
    // bank on a dry one
    const double edge = std::min(fld->hw - std::fabs(lx), fld->hd - std::fabs(lz));
    const double line = clampd(1 - edge / (1.2 + 0.25 * detailM), 0, 1);
    double tone = 0;
    if (fld->sub == FI_STRIP || fld->sub == FI_TERRACE) { const int strip = (int)std::floor((lx + fld->hw) / (fld->sub == FI_TERRACE ? 6.0 : 14.0)); tone = (strip & 1) ? 0.12 : -0.1; }
    else tone = (unitFromHash(fld->id) - 0.5) * 0.3;
    t.albedo *= (float)(1 + tone * L);
    if (green) t.veg = std::min(1.0f, std::max(0.0f, t.veg * (float)(1 + 2 * tone * L)));
    if (line > 0) {
        if (green && fld->sub != FI_TERRACE) { t.veg = std::min(1.0f, t.veg * (float)(1 + 0.8 * line * L)); t.albedo *= (float)(1 - 0.25 * line * L); }   // a hedge
        else t.albedo *= (float)(1 - 0.38 * line * L);   // a wall, a bank, a terrace's riser
    }
}

// ---- T-03: the works' ground --------------------------------------------------------------------------------------------------
namespace {
constexpr double EDIT_CELL = 64, EDIT_BIG = 600;
// a point's distance to a polyline (x, z pairs) and the share along the whole of it, and its side (+1 right of the way it runs)
double polyDist(const std::vector<float>& p, double x, double z, double* along = nullptr, int* side = nullptr) {
    double best = 1e18, bestT = 0, total = 0, run = 0; int bs = 1;
    for (size_t i = 2; i + 1 < p.size(); i += 2) total += std::hypot(p[i] - p[i - 2], p[i + 1] - p[i - 1]);
    for (size_t i = 2; i + 1 < p.size(); i += 2) {
        const double ax = p[i - 2], az = p[i - 1], bx = p[i], bz = p[i + 1], dx = bx - ax, dz = bz - az, L2 = dx * dx + dz * dz, L = std::sqrt(L2);
        const double t = L2 > 1e-12 ? clampd(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0;
        const double d = std::hypot(x - (ax + dx * t), z - (az + dz * t));
        if (d < best) { best = d; bestT = total > 0 ? (run + t * L) / total : 0; bs = (dx * (z - az) - dz * (x - ax)) < 0 ? 1 : -1; }
        run += L;
    }
    if (along) *along = bestT;
    if (side) *side = bs;
    return best;
}
}

void SurfaceSite::addWorkEdits(const Work& w, double ox, double oz, double years, double wet) {
    const double y = std::max(1.0, years);
    const float left = (float)editLeft(y, wet), lineLeft = (float)editLeft(y, wet, true), fresh = (float)clampd(1 - (std::log10(y) - 1.5) / 2.5, 0, 1);
    for (const GroundEdit& g : w.edits) {
        SiteEdit e;
        e.kind = g.kind; e.tone = g.tone; e.x = ox + g.x; e.z = oz + g.z; e.heading = g.heading; e.hw = g.hw; e.hd = g.hd; e.r = g.r;
        e.level = g.level; e.depth = g.depth; e.step = g.step; e.width = g.width; e.width2 = g.width2; e.bank = g.bank;
        e.left = g.fades ? (g.kind == GE_LINE ? lineLeft : left) : 1.0f; e.fresh = fresh;
        double ext = 0;
        switch (g.kind) {
            case GE_CUT: ext = std::hypot(g.hw, g.hd) + 2; e.x0 = e.x - ext; e.x1 = e.x + ext; e.z0 = e.z - ext; e.z1 = e.z + ext; break;
            case GE_PIT: case GE_BOWL: case GE_FLAT: case GE_TERRACES: ext = g.r * (g.kind == GE_BOWL ? 1.4 : 1.0) + 2; e.x0 = e.x - ext; e.x1 = e.x + ext; e.z0 = e.z - ext; e.z1 = e.z + ext; e.pts = g.pts; break;
            case GE_WATER:
                e.gx0 = e.x + g.gx0; e.gz0 = e.z + g.gz0; e.cell = g.cell; e.nx = g.nx; e.nz = g.nz; e.mask = g.mask;
                e.x0 = e.gx0; e.z0 = e.gz0; e.x1 = e.gx0 + g.nx * g.cell; e.z1 = e.gz0 + g.nz * g.cell;
                break;
            default: {   // a ditch's or a line's points
                const double half = 0.5 * std::max(g.width, g.width2) + std::fabs(g.bank) * 2.5 + 2;
                e.x0 = e.z0 = 1e18; e.x1 = e.z1 = -1e18;
                for (size_t i = 0; i + 1 < g.pts.size(); i += 2) {
                    const double px = ox + g.pts[i], pz = oz + g.pts[i + 1];
                    e.pts.push_back((float)px); e.pts.push_back((float)pz);
                    e.x0 = std::min(e.x0, px - half); e.x1 = std::max(e.x1, px + half); e.z0 = std::min(e.z0, pz - half); e.z1 = std::max(e.z1, pz + half);
                }
                if (e.pts.size() < 4) continue;
                break;
            }
        }
        edits.push_back(e);
        const int k = (int)edits.size() - 1;
        if (std::max(e.x1 - e.x0, e.z1 - e.z0) > EDIT_BIG) bigEdits.push_back(k);
        else
            for (int64_t j = (int64_t)std::floor(e.z0 / EDIT_CELL); j <= (int64_t)std::floor(e.z1 / EDIT_CELL); j++)
                for (int64_t i = (int64_t)std::floor(e.x0 / EDIT_CELL); i <= (int64_t)std::floor(e.x1 / EDIT_CELL); i++) editCells[markKey(i, j)].push_back(k);
        for (TerrainCache* c : {&lodN, &lod0, &lod1, &lod2, &lod3}) c->invalidate(e.x0 - c->cellSize, e.z0 - c->cellSize, e.x1 + c->cellSize, e.z1 + c->cellSize);
    }
}

void SurfaceSite::applyEdits(double x, double z, double detailM, TerrainVertex& t) const {
    auto one = [&](const SiteEdit& e) {
        if (x < e.x0 || x > e.x1 || z < e.z0 || z > e.z1) return;
        const int rock = familyRep(gen.type, FAM_ROCK);
        const bool wetHere = t.material == MAT_WATER || (t.water > -1e8f && t.h < t.water + 0.3f);
        switch (e.kind) {
            case GE_CUT: {   // a face into the hill: the floor and its benches, the rock fresh and pale for a few centuries
                if (detailM > std::max(e.hw, e.hd) * 1.5) return;
                const double dx = x - e.x, dz = z - e.z, ch = std::cos(e.heading), sh = std::sin(e.heading);
                const double u = dx * ch - dz * sh, v = dx * sh + dz * ch;
                if (std::fabs(u) > e.hw || std::fabs(v) > e.hd) return;
                const double s = e.level + (e.step > 0 && e.depth > 0 ? e.depth * std::floor((v + e.hd) / e.step) : 0);
                if (s >= t.h) return;
                t.h = (float)s; t.material = (uint8_t)rock; t.albedo *= 1 + 0.25f * e.fresh; t.veg *= 1 - 0.9f * e.fresh; t.scree = 0;
                return;
            }
            case GE_PIT: {   // an open pit's benches down to its floor (a flint pit: one small shaft)
                if (detailM > e.r * 1.2) return;
                const double r = std::hypot(x - e.x, z - e.z);
                if (r > e.r) return;
                const double r0 = e.hw > 0 ? e.hw : e.r;
                const double s = r <= r0 || e.step <= 0 ? e.level : e.level + e.depth * std::ceil((r - r0) / e.step);
                if (s >= t.h) return;
                t.h = (float)(e.left < 1 ? t.h + (s - t.h) * e.left : s);
                if (e.r > 30) { t.material = (uint8_t)rock; t.albedo *= 1 + 0.2f * e.fresh; t.veg *= 1 - 0.9f * e.fresh; t.scree = 0; }
                else t.albedo *= 1 - 0.25f * e.left;
                return;
            }
            case GE_TERRACES: {   // a mountain cut into terraces: each ring band flat at its level, a riser at its inner edge
                if (detailM > e.step * 1.2 || e.pts.empty()) return;
                const double r = std::hypot(x - e.x, z - e.z);
                if (r > e.r) return;
                const double s = e.pts[(size_t)clampi((int)((e.r - r) / e.step), 0, (int)e.pts.size() - 1)];
                if (s >= t.h) return;
                t.h = (float)s; t.material = (uint8_t)(e.tone == ET_GLASS ? MAT_GLASS : rock); t.veg = 0; t.scree = 0; t.albedo = e.tone == ET_GLASS ? 0.18f : t.albedo * 1.1f;
                return;
            }
            case GE_FLAT: {   // the ground made flat at its level: cut where it was higher, banked where lower
                if (detailM > e.r * 0.8) return;
                const double r = std::hypot(x - e.x, z - e.z);
                if (r > e.r) return;
                const double k = smoothstep(e.r, e.r - std::max(1.0, e.width), r);
                t.h = (float)(t.h + (e.level - t.h) * k);
                if (k > 0.5) {
                    if (e.tone == ET_GLASS) { t.material = MAT_GLASS; t.albedo = 0.2f; t.veg = 0; }
                    else if (e.tone == ET_PAVED) { t.material = (uint8_t)rock; t.albedo = std::min(0.75f, t.albedo * 1.35f + 0.08f); t.veg = 0; }
                    else { t.veg *= 0.35f; }
                    t.scree = 0;
                }
                return;
            }
            case GE_WATER: {   // the reservoir: its water over the cells it filled, the plane under its banks a cell further; on a world whose water is
                               // gone (a dead desert), its old bed: silt laid flat and pale under the old waterline
                const int i = (int)std::floor((x - e.gx0) / e.cell), j = (int)std::floor((z - e.gz0) / e.cell);
                if (i < 0 || j < 0 || i >= e.nx || j >= e.nz || !e.mask[(size_t)j * e.nx + i]) return;
                if (gen.type == PT_DESERT || e.tone == ET_DARK) {   // a dead desert's, or drained by a breach: silt laid flat under the old waterline
                    if (t.h < e.level) {
                        t.h = (float)(e.level - (e.level - t.h) * 0.55 - 1.5);
                        if (gen.type == PT_DESERT) { t.material = MAT_SALT; t.albedo = std::min(0.8f, t.albedo * 1.2f + 0.1f); t.veg = 0; }
                        else { t.albedo *= 0.85f; t.veg *= 0.6f; }
                        t.scree = 0;
                    }
                    return;
                }
                t.water = (float)e.level; t.shore = 1e9f;
                if (t.h < e.level) { t.material = t.material == MAT_SNOW || t.material == MAT_ICE ? MAT_ICE : MAT_WATER; t.albedo = 0.3f; t.veg = 0; t.scree = 0; }
                return;
            }
            case GE_DITCH: {   // a ditch (a trench, a henge's, a line's) silting up, its bank beside it
                const double half = 0.5 * e.width;
                if (detailM > std::max(e.width * 2.0, 4.0)) return;
                int side = 1; const double d = polyDist(e.pts, x, z, nullptr, &side);
                if (d < half) { const double q = d / half; t.h -= (float)(e.depth * e.left * (1 - q * q)); t.albedo *= 1 - 0.18f * e.left; if (wetHere) return; t.veg *= 1 - 0.3f * e.left; }
                else if (e.bank != 0 && (e.bank > 0 ? side > 0 : side < 0) && d < half + 3 * std::fabs(e.bank)) {
                    const double q = (d - half) / (3 * std::fabs(e.bank));
                    t.h += (float)(std::fabs(e.bank) * e.left * std::sin(PI * q));
                }
                return;
            }
            case GE_BOWL: {   // a crater: its hollow and its rim, scorched or glassed; filling with the years
                if (detailM > e.r * 1.5) return;
                const double r = std::hypot(x - e.x, z - e.z);
                if (r > e.r * 1.4) return;
                if (r < e.r) {
                    const double q = r / e.r;
                    t.h -= (float)(e.depth * e.left * (1 - q * q));
                    if (e.tone == ET_GLASS && q < 0.85) { t.material = MAT_GLASS; t.albedo = 0.16f + 0.1f * (1 - e.left); t.veg = 0; }
                    else { t.albedo *= 1 - 0.3f * e.left; t.veg *= 1 - 0.7f * e.left; }
                } else t.h += (float)(0.22 * e.depth * e.left * std::sin(PI * (r - e.r) / (0.4 * e.r)));
                t.scree = 0;
                return;
            }
            default: {   // lines: their share of the vertex's footprint, cleared (paler), dark, paved; the bed's material on the fine rings
                if (detailM > 100 || wetHere) return;
                double along = 0; const double d = polyDist(e.pts, x, z, &along);
                const double half = 0.5 * (e.width2 >= 0 ? e.width + (e.width2 - e.width) * along : e.width);
                const double overlap = clampd(half + 0.5 * detailM - d, 0, std::min(2 * half, detailM));
                const double cover = overlap / detailM * e.left;
                if (cover < 0.002) return;
                const double maxCover = std::min(1.0, 2 * half / detailM);
                const bool bed = detailM <= 24 && cover >= 0.5 * maxCover * e.left;
                const double k = std::min(1.0, cover * std::max(1.0, detailM / 16.0));
                const int fam = matFamily(t.material);
                switch (e.tone) {
                    case ET_CLEARED:   // the dark stones swept aside: the paler ground under them (a fifth of the albedo is lost in the tone tile: twice it)
                        if (bed && (fam == FAM_GRASS || fam == FAM_FOREST)) t.material = (uint8_t)familyRep(gen.type, FAM_SAND);
                        t.albedo = std::min(0.92f, (float)(t.albedo * (1 + 1.2 * k)));
                        t.veg *= (float)(1 - 0.95 * k);
                        break;
                    case ET_PAVED:
                        if (bed) t.material = (uint8_t)rock;
                        t.albedo = std::min(0.8f, (float)(t.albedo * (1 + 0.3 * k)));
                        t.veg *= (float)(1 - 0.9 * k);
                        break;
                    default:   // a beaten track, a channel, a conduit's line
                        if (bed && (fam == FAM_GRASS || fam == FAM_FOREST)) t.material = (uint8_t)familyRep(gen.type, FAM_SAND);
                        t.albedo *= (float)(1 - 0.35 * k);
                        t.veg *= (float)(1 - 0.6 * k);
                        break;
                }
                return;
            }
        }
    };
    for (int k : bigEdits) one(edits[k]);
    if (editCells.empty()) return;
    auto it = editCells.find(markKey((int64_t)std::floor(x / EDIT_CELL), (int64_t)std::floor(z / EDIT_CELL)));
    if (it != editCells.end()) for (int k : it->second) one(edits[k]);
}

double SurfaceSite::editCover(double x, double z) const {
    double best = 0;
    auto one = [&](const SiteEdit& e) {
        if (x < e.x0 || x > e.x1 || z < e.z0 || z > e.z1) return;
        switch (e.kind) {
            case GE_CUT: {
                const double dx = x - e.x, dz = z - e.z, ch = std::cos(e.heading), sh = std::sin(e.heading);
                if (std::fabs(dx * ch - dz * sh) < e.hw + 1 && std::fabs(dx * sh + dz * ch) < e.hd + 1) best = std::max(best, (double)e.fresh);
                break;
            }
            case GE_PIT: if (e.r > 30 && std::hypot(x - e.x, z - e.z) < e.r) best = std::max(best, (double)e.fresh); break;
            case GE_FLAT: case GE_TERRACES: if (std::hypot(x - e.x, z - e.z) < e.r) best = 1; break;   // a grove's clearing, a plain made level: no tree
            case GE_LINE: { double along = 0; const double d = polyDist(e.pts, x, z, &along); const double half = 0.5 * (e.width2 >= 0 ? e.width + (e.width2 - e.width) * along : e.width); best = std::max(best, e.left * clampd(half + 1.0 - d, 0, 1)); break; }
            default: break;
        }
    };
    for (int k : bigEdits) one(edits[k]);
    auto it = editCells.find(markKey((int64_t)std::floor(x / EDIT_CELL), (int64_t)std::floor(z / EDIT_CELL)));
    if (it != editCells.end()) for (int k : it->second) one(edits[k]);
    return best;
}

// C-09: the roads of the site. Walked once per pair from the cells looked at so far (`RoadNodeCache`), kept in local metres
// with the metres along each for the wear's gaps; `roadAt` is the nearest road's segment within reach (the bounds first, so
// a vertex far from every road costs a few comparisons)
void SurfaceSite::clearRoads() { roads.clear(); roadIds.clear(); roadCells.clear(); roadNodes.reset(&gen); roadsMs = 0; }

void SurfaceSite::ensureRoads(double x, double z) {
    if (!worldHadCivilisation(gen)) return;
    auto t0 = std::chrono::steady_clock::now();
    if (roadCells.empty()) { roadCultures[0] = cultureOf(gen, 0); roadCultures[1] = peoplesOf(gen) > 1 ? cultureOf(gen, 1) : roadCultures[0]; roadNodes.reset(&gen); }   // C-13: each people's
    double lat, lon; latLonAt(x, z, lat, lon);
    int gLat0, gLon0; ruinCellOf(gen, lat, lon, gLat0, gLon0);
    std::vector<RoadPair> pairs;
    for (int dl = -ROAD_BLOCK; dl <= ROAD_BLOCK; dl++) {
        const int row = ruinCellLonIn(gen, gLat0, gLon0, gLat0 + dl);   // T-01: the row's own cells at this longitude
        for (int dn = -ROAD_BLOCK; dn <= ROAD_BLOCK; dn++) {
            if (!roadCells.insert(roadCellKey(gen, gLat0 + dl, row + dn)).second) continue;
            roadNodes.pairsOf(gLat0 + dl, row + dn, pairs);
        }
    }
    std::vector<RoadPair> todo;
    for (const RoadPair& p : pairs) if (roadIds.insert(p.id).second) todo.push_back(p);
    std::vector<Road> rs(todo.size()); std::vector<char> ok(todo.size(), 0);
    auto walkRange = [&](int i0, int i1) { for (int i = i0; i < i1; i++) ok[i] = roadWay(gen, roadCultures[roadPeopleOf(todo[i].a, todo[i].b)], todo[i].a, todo[i].b, rs[i]) ? 1 : 0; };   // C-13: built as its people built
    if (todo.size() > 4) parallelFor((int)todo.size(), 2, walkRange); else walkRange(0, (int)todo.size());
    for (size_t i = 0; i < todo.size(); i++) {
        if (!ok[i]) continue;
        const Road& r = rs[i];
        SiteRoad sr; sr.id = r.id; sr.half = (float)r.halfWidth; sr.paved = roadCultures[r.people].paved; sr.wear = (float)roadCultures[r.people].roadWear;   // C-13
        double along = 0, px = 0, pz = 0;
        for (size_t k = 0; k < r.pts.size(); k++) {
            double rx, rz; localAt(r.pts[k], rx, rz);
            if (k) along += std::hypot(rx - px, rz - pz);
            sr.x.push_back((float)rx); sr.z.push_back((float)rz); sr.along.push_back((float)along);
            if (!k) { sr.x0 = sr.x1 = (float)rx; sr.z0 = sr.z1 = (float)rz; }
            sr.x0 = std::min(sr.x0, (float)rx); sr.x1 = std::max(sr.x1, (float)rx); sr.z0 = std::min(sr.z0, (float)rz); sr.z1 = std::max(sr.z1, (float)rz);
            px = rx; pz = rz;
        }
        roads.push_back(std::move(sr));
    }
    // T-01: the plan's ways through the block (galaxy/plan.h): each leg within reach walked once by its kind (a maglev's runs on its pylons:
    // no bed), its id the way's and the leg's
    if (std::shared_ptr<const WorldPlan> P = worldPlanOf(gen)) {
        const double blockM = (ROAD_BLOCK + 1) * ruinCellLat(gen) * R;
        std::vector<PlanEdgeRef> es; planEdgesNear(*P, unitAt(x, z), blockM + 3000, es);
        struct Leg { Vec3 a, b; uint64_t id; int kind, people; bool trunk; };
        std::vector<Leg> legs;
        for (const PlanEdgeRef& r : es) {
            if (r.e->kind == NK_MAGLEV || r.e->kind == NK_SEA) continue;
            const std::vector<Vec3>& way = planEdgeWay(gen, *P, r);
            for (size_t k = 0; k + 1 < way.size(); k++) {
                double ax, az, bx, bz; localAt(way[k], ax, az); localAt(way[k + 1], bx, bz);
                const double dx = bx - ax, dz = bz - az, L2 = dx * dx + dz * dz;
                const double tt = L2 > 1e-9 ? clampd(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0;
                if (std::hypot(x - (ax + dx * tt), z - (az + dz * tt)) > blockM) continue;
                const uint64_t id = mix64(r.e->id ^ ((uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL));
                if (!roadIds.insert(id).second) continue;
                legs.push_back({way[k], way[k + 1], id, r.e->kind, r.people, r.e->trunk});
            }
        }
        std::vector<Road> lr(legs.size()); std::vector<char> lok(legs.size(), 0);
        auto legRange = [&](int i0, int i1) { for (int i = i0; i < i1; i++) lok[i] = roadLegWalk(gen, legs[i].kind, legs[i].a, legs[i].b, legs[i].id, lr[i]) ? 1 : 0; };
        if (legs.size() > 4) parallelFor((int)legs.size(), 2, legRange); else legRange(0, (int)legs.size());
        for (size_t i = 0; i < legs.size(); i++) {
            if (!lok[i]) continue;
            const Culture& cu = roadCultures[legs[i].people > 0 && peoplesOf(gen) > 1 ? 1 : 0];
            SiteRoad sr; sr.id = legs[i].id; sr.kind = legs[i].kind; sr.half = (float)netHalfWidth(legs[i].kind, cu, legs[i].trunk);
            sr.paved = legs[i].kind != NK_PATH && (legs[i].kind != NK_ROAD || cu.paved || legs[i].trunk); sr.wear = (float)cu.roadWear;
            double along = 0, px = 0, pz = 0;
            for (size_t k = 0; k < lr[i].pts.size(); k++) {
                double rx, rz; localAt(lr[i].pts[k], rx, rz);
                if (k) along += std::hypot(rx - px, rz - pz);
                sr.x.push_back((float)rx); sr.z.push_back((float)rz); sr.along.push_back((float)along);
                if (!k) { sr.x0 = sr.x1 = (float)rx; sr.z0 = sr.z1 = (float)rz; }
                sr.x0 = std::min(sr.x0, (float)rx); sr.x1 = std::max(sr.x1, (float)rx); sr.z0 = std::min(sr.z0, (float)rz); sr.z1 = std::max(sr.z1, (float)rz);
                px = rx; pz = rz;
            }
            roads.push_back(std::move(sr));
        }
    }
    roadsMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

bool SurfaceSite::roadAt(double x, double z, double within, double& dist, double& along, double& heading, const SiteRoad*& road) const {
    double best = within; road = nullptr;
    for (const SiteRoad& r : roads) {
        if (x < r.x0 - within || x > r.x1 + within || z < r.z0 - within || z > r.z1 + within) continue;
        for (size_t i = 0; i + 1 < r.x.size(); i++) {
            double ax = r.x[i], az = r.z[i], bx = r.x[i + 1], bz = r.z[i + 1];
            if (std::min(ax, bx) > x + best || std::max(ax, bx) < x - best || std::min(az, bz) > z + best || std::max(az, bz) < z - best) continue;
            double dx = bx - ax, dz = bz - az, L2 = dx * dx + dz * dz;
            double t = L2 > 1e-9 ? clampd(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0;
            double d = std::hypot(x - (ax + dx * t), z - (az + dz * t));
            if (d < best) { best = d; road = &r; along = r.along[i] + t * (r.along[i + 1] - r.along[i]); heading = std::atan2(dx, dz); }
        }
    }
    dist = best;
    return road != nullptr;
}

double SurfaceSite::roadCover(double x, double z) const {
    if (roads.empty()) return 0;
    double d, along, hd; const SiteRoad* r;
    if (!roadAt(x, z, 10.0, d, along, hd, r)) return 0;
    return roadLeft(r->id, along, r->wear) * clampd(r->half + 1.0 - d, 0, 1);
}

double vertexShore(TerrainCache& c, int cx, int cz) {
    const TerrainVertex& tv = c.at(cx, cz);
    if (tv.shore < 1e8f) return tv.shore;             // a river's or a lake's outline, on the bank beyond the plane too
    if (tv.water > -1e8f) return tv.h - tv.water;
    double sum = 0; int n = 0;   // no water field here: the height over the plane of the neighbours that have one
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dz) continue;
            const TerrainVertex& nv = c.at(cx + dx, cz + dz);
            if (nv.water > -1e8f) { sum += nv.water; n++; }
        }
    return n ? tv.h - sum / n : 1e9;
}

void shoreSplitTriangle(const double x[3], const double z[3], const double h[3], const double wl[3], const double sh[3], ShoreSplit& out) {
    out.nWet = out.nDry = 0;
    for (int e = 0; e < 3; e++) {
        int a = e, b = (e + 1) % 3;
        bool aw = sh[a] < 0, bw = sh[b] < 0;
        ShorePt pa{x[a], z[a], h[a], a, a, 0.0};
        if (aw) out.wet[out.nWet++] = pa; else out.dry[out.nDry++] = pa;
        if (aw != bw) {
            double t = sh[a] / (sh[a] - sh[b]);
            ShorePt pc{x[a] + (x[b] - x[a]) * t, z[a] + (z[b] - z[a]) * t, wl[a] + (wl[b] - wl[a]) * t - 0.03, a, b, t};
            out.wet[out.nWet++] = pc; out.dry[out.nDry++] = pc;
        }
    }
}

void SurfaceSite::init(const StarSystem* s, int bodyIndex, double lat, double lon, double t) {
    sys = s; body = bodyIndex; lat0 = lat; lon0 = lon;
    const Body& b = s->bodies[bodyIndex];
    season = s->seasonOf(bodyIndex, t);
    gen = BodyGen::make(b, season);
    R = b.radiusKm * 1000.0;
    gravity = b.gravity;
    up0 = StarSystem::bodyFromLatLon(lat0, lon0);
    Vec3 pole(0, 0, 1);
    east0 = normalize(cross(pole, up0));
    if (length(cross(pole, up0)) < 1e-6) east0 = Vec3(0, 1, 0);
    north0 = cross(up0, east0);
    atmosphere = PLANET_TYPES[b.type].atmosphere;
    hasWater = b.type == PT_FELISIAN || b.type == PT_OCEAN || gen.liquidLevel > -1e8;   // R-304: exotic seas are a sea too
    seaLevel = gen.liquidLevel > -1e8 ? gen.liquidLevel : 0;
    lavaLevel = b.type == PT_MOLTEN ? gen.lavaLevel : -1e9;
    lod0.init(this, 16, 7);
    lod1.init(this, 64, 7);
    lod2.init(this, 512, 6);
    lod3.init(this, 2048, 6);
    lodN.init(this, 4, 8);   // O6-02
    nearBlend = 0; nearR = 0;
    smallBody = R < 300e3;
    escapeVelocity = std::sqrt(2 * gravity * R);
    ringProf = b.rings ? s->ringProfileOf(bodyIndex) : std::vector<float>();
}

Vec3 SurfaceSite::unitAt(double x, double z) const {
    double d = std::sqrt(x * x + z * z);
    if (d < 1e-9) return up0;
    double ang = d / R;
    Vec3 dir = (east0 * x + north0 * z) / d;
    return normalize(up0 * std::cos(ang) + dir * std::sin(ang));
}

void SurfaceSite::localAt(const Vec3& unit, double& x, double& z) const {
    double c = clampd(dot(unit, up0), -1, 1);
    double ang = std::acos(c);
    Vec3 tang = unit - up0 * c;
    double tl = length(tang);
    if (tl < 1e-12 || ang < 1e-12) { x = 0; z = 0; return; }
    tang = tang / tl;
    x = R * ang * dot(tang, east0);
    z = R * ang * dot(tang, north0);
}

void SurfaceSite::latLonAt(double x, double z, double& lat, double& lon) const {
    StarSystem::latLonFromBody(unitAt(x, z), lat, lon);
}

Mat3 SurfaceSite::localFrame(double t) const {
    Mat3 bodyT = sys->bodyFrame(body, t).transposed();   // body -> world
    Vec3 e = bodyT * east0, u = bodyT * up0, n = bodyT * north0;
    return Mat3::fromRows(e, u, n);
}

Vec3 SurfaceSite::worldPos(double t, double x, double z, double alt) const {
    Mat3 bodyT = sys->bodyFrame(body, t).transposed();
    Vec3 u = bodyT * unitAt(x, z);
    return sys->bodyPos(body, t) + u * ((R + alt) / 1000.0);
}

SunInfo SurfaceSite::sun(double t) const {
    SunInfo si;
    Vec3 bp = sys->bodyPos(body, t);
    Vec3 dW = normalize(sys->star.pos - bp);
    Mat3 L = localFrame(t);
    si.dirLocal = L * dW;
    si.altitude = std::asin(clampd(si.dirLocal.y, -1, 1));
    si.azimuth = wrap2pi(std::atan2(si.dirLocal.x, si.dirLocal.z));
    double dist = length(sys->star.pos - bp);
    double flare = starFlare(sys->star, t);   // S-01: a red dwarf's flare doubles its light for a minute and whitens it: a quarter more light past the exposure clamp (its near worlds sit at the clamp already)
    si.lightFactor = std::max(0.62, SpaceRenderer::lightFactor(sys->star.luminosity, dist)) * (1 + 0.25 * flare);
    si.angularRadius = std::asin(clampd(sys->star.radiusKm / dist, 0, 1));
    // M1-08: other bodies between the site and the star cover part of its disc
    si.eclipse = 0;
    {
        Vec3 sitePos = worldPos(t, 0, 0, 0);
        for (int j = 0; j < (int)sys->bodies.size(); j++) {
            if (j == body) continue;
            Vec3 rel = sys->bodyPos(j, t) - sitePos;
            double d = length(rel);
            if (d <= 0) continue;
            double rb = std::asin(clampd(sys->bodies[j].radiusKm / d, 0, 1));
            double theta = std::acos(clampd(dot(rel / d, dW), -1, 1));
            double rs = si.angularRadius;
            double cov = 0;
            if (theta >= rs + rb) cov = 0;
            else if (theta <= std::fabs(rb - rs)) cov = rb >= rs ? 1.0 : (rb * rb) / (rs * rs);
            else cov = clampd((rs + rb - theta) / (2 * std::min(rs, rb)), 0, 1) * std::min(1.0, (rb * rb) / (rs * rs));
            si.eclipse = std::max(si.eclipse, cov);
        }
    }
    si.lightFactor *= (1 - 0.97 * si.eclipse);
    // O0-01: the world's own ring between the site and the sun: a dusky band on the winter side of the equator
    si.ringShadow = 0;
    if (!ringProf.empty()) {
        const Body& b = sys->bodies[body];
        Vec3 P = worldPos(t, 0, 0, 0) - bp;   // km from the centre
        double denom = dot(dW, b.spinAxis);
        if (std::fabs(denom) > 1e-9) {
            double s = -dot(P, b.spinAxis) / denom;
            if (s > 0) {
                double rr = length(P + dW * s);
                double r0 = b.ringInner * b.radiusKm, r1 = b.ringOuter * b.radiusKm;
                if (rr >= r0 && rr <= r1) si.ringShadow = 0.85 * ringProf[clampi((int)((rr - r0) / (r1 - r0) * 255), 0, 255)];
            }
        }
        si.lightFactor *= (1 - 0.8 * si.ringShadow);
    }
    // local time from the hour angle: sub-solar longitude in body frame
    Vec3 sunBody = sys->bodyFrame(body, t) * dW;
    double subLon = std::atan2(sunBody.y, sunBody.x);
    double ha = wrapAngle(lon0 - subLon);   // hour angle, 0 = noon
    si.dayFraction = wrap2pi(ha + PI) / TAU;
    si.color = flare > 0 ? lerp(sys->star.color, RGB(1, 1, 1), (float)(0.35 * std::min(1.0, flare))) : sys->star.color;
    return si;
}

bool SurfaceSite::sun2(double t, SunInfo& si) const {
    if (sys->companion < 0) return false;
    const Body& k = sys->bodies[sys->companion];
    Vec3 bp = sys->bodyPos(body, t);
    Vec3 kp = sys->bodyPos(sys->companion, t);
    double dist = length(kp - bp);
    Vec3 dW = (kp - bp) / std::max(dist, 1.0);
    Mat3 L = localFrame(t);
    si.dirLocal = L * dW;
    si.altitude = std::asin(clampd(si.dirLocal.y, -1, 1));
    si.azimuth = wrap2pi(std::atan2(si.dirLocal.x, si.dirLocal.z));
    si.lightFactor = SpaceRenderer::lightFactor(k.luminosity, dist);
    si.angularRadius = std::asin(clampd(k.radiusKm / dist, 0, 1));
    si.eclipse = 0;
    si.dayFraction = 0;
    si.color = k.color;
    return true;
}

double SurfaceSite::waterAt(double x, double z) {   // B-320: bilinear over the wet corners (the level the sheet is drawn at); it was the highest corner
    double fx = x / lod0.cellSize, fz = z / lod0.cellSize;
    int cx = (int)std::floor(fx), cz = (int)std::floor(fz);
    fx -= cx; fz -= cz;
    double ws[4] = {lod0.at(cx, cz).water, lod0.at(cx + 1, cz).water, lod0.at(cx, cz + 1).water, lod0.at(cx + 1, cz + 1).water};
    double wgt[4] = {(1 - fx) * (1 - fz), fx * (1 - fz), (1 - fx) * fz, fx * fz};
    double sum = 0, wsum = 0;
    for (int k = 0; k < 4; k++) if (ws[k] > -1e8) { sum += ws[k] * wgt[k]; wsum += wgt[k]; }
    return wsum > 1e-6 ? sum / wsum : -1e9;
}

namespace {
// the height of a point inside a triangle piece (fan from [0]), by barycentric interpolation of the fan triangle that holds it
double pieceHeight(const ShorePt* p, int n, double x, double z) {
    double best = p[0].h, bestErr = 1e18;
    for (int k = 1; k + 1 < n; k++) {
        const ShorePt& A = p[0]; const ShorePt& B = p[k]; const ShorePt& C = p[k + 1];
        double det = (B.x - A.x) * (C.z - A.z) - (C.x - A.x) * (B.z - A.z);
        if (std::fabs(det) < 1e-12) continue;
        double u = ((x - A.x) * (C.z - A.z) - (C.x - A.x) * (z - A.z)) / det;
        double v = ((B.x - A.x) * (z - A.z) - (x - A.x) * (B.z - A.z)) / det;
        double err = std::max(0.0, -u) + std::max(0.0, -v) + std::max(0.0, u + v - 1);   // 0 inside
        if (err < bestErr) { bestErr = err; best = A.h + (B.h - A.h) * u + (C.h - A.h) * v; }
    }
    return best;
}

}

// the height of the two triangles of a cell (the split the terrain mesh draws), on any cache; B-322: a cell cut by a
// water's edge is read as the drawn pieces (`shoreSplitTriangle`)
double cacheHeightAt(TerrainCache& c, double x, double z) {
    double cs = c.cellSize;
    double fx = x / cs, fz = z / cs;
    int cx = (int)std::floor(fx), cz = (int)std::floor(fz);
    double tx = fx - cx, tz = fz - cz;
    const TerrainVertex& t1 = c.at(cx, cz); const TerrainVertex& t2 = c.at(cx + 1, cz); const TerrainVertex& t3 = c.at(cx + 1, cz + 1); const TerrainVertex& t4 = c.at(cx, cz + 1);
    double h1 = t1.h, h2 = t2.h, h3 = t3.h, h4 = t4.h;
    bool lower = tx + tz < 1;
    double plain = lower ? h1 + (h2 - h1) * tx + (h4 - h1) * tz : h3 + (h4 - h3) * (1 - tx) + (h2 - h3) * (1 - tz);
    const TerrainVertex* tv[3] = {lower ? &t1 : &t2, lower ? &t2 : &t3, lower ? &t4 : &t4};   // the mesh's triangles: (00, 10, 01) and (10, 11, 01)
    double wMean = 0; int wN = 0;
    for (const TerrainVertex* v : {&t1, &t2, &t3, &t4}) if (v->water > -1e8f) { wMean += v->water; wN++; }
    if (!wN) return plain;
    wMean /= wN;
    double sh[3], xs[3], zs[3], hs[3], wl[3];
    int ox[3] = {lower ? 0 : 1, lower ? 1 : 1, 0}, oz[3] = {0, lower ? 0 : 1, 1};
    bool anyWet = false, anyDry = false;
    for (int k = 0; k < 3; k++) {
        sh[k] = vertexShore(c, cx + ox[k], cz + oz[k]);
        xs[k] = (cx + ox[k]) * cs; zs[k] = (cz + oz[k]) * cs; hs[k] = tv[k]->h;
        wl[k] = tv[k]->water > -1e8f ? tv[k]->water : wMean;
        if (sh[k] < 0) anyWet = true; else anyDry = true;
    }
    if (!anyWet || !anyDry) return plain;
    ShoreSplit sp;
    shoreSplitTriangle(xs, zs, hs, wl, sh, sp);
    // which side of the edge the point lies on: the shore distance is linear over the triangle
    double det = (xs[1] - xs[0]) * (zs[2] - zs[0]) - (xs[2] - xs[0]) * (zs[1] - zs[0]);
    double u = ((x - xs[0]) * (zs[2] - zs[0]) - (xs[2] - xs[0]) * (z - zs[0])) / det;
    double v = ((xs[1] - xs[0]) * (z - zs[0]) - (x - xs[0]) * (zs[1] - zs[0])) / det;
    double shHere = sh[0] + (sh[1] - sh[0]) * u + (sh[2] - sh[0]) * v;
    if (shHere < 0) return sp.nWet >= 3 ? pieceHeight(sp.wet, sp.nWet, x, z) : plain;
    if (sp.nDry < 3) return plain;
    ShorePt dry[4];   // O6-03: the dry piece as drawn: its edge on the cut stands SHORE_LIP over the water
    for (int k = 0; k < sp.nDry; k++) { dry[k] = sp.dry[k]; if (dry[k].a != dry[k].b) dry[k].h += SHORE_LIP; }
    return pieceHeight(dry, sp.nDry, x, z);
}

double SurfaceSite::groundHeightCoarse(double x, double z) { return cacheHeightAt(lod0, x, z); }

double SurfaceSite::groundHeight(double x, double z) {
    double h0 = cacheHeightAt(lod0, x, z);
    if (lod0R > 0 && lod0Band > 0) {   // B-319: across lod0's outer band the drawn ground slides onto lod1's (the ring's edge vertices sit on it)
        double dx = (x - camX) / lod0.cellSize, dz = (z - camZ) / lod0.cellSize;
        double m = smoothstep(lod0R - lod0Band, lod0R - 0.5, std::sqrt(dx * dx + dz * dz));
        if (m > 0) h0 += (cacheHeightAt(lod1, x, z) - h0) * m;
    }
    if (!inNearRing(x, z)) return h0;
    return h0 + (cacheHeightAt(lodN, x, z) - h0) * (1 - nearMorph(x, z));   // O6-02: the near ring's ground, as drawn (B-313: morphed onto lod0 toward its edge)
}
