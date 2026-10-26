// E-03: the walls (B-420, B-421): `walls` and its checks
#include "harness/harness.h"
#include "harness/test_worlds.h"

// ---- B-420: invisible walls; B-421: where the records and the instruments lie -----------------------------------------------------------
// A collider is invisible where nothing is drawn at it: each disc is held against the drawn surface (the meshes as the view builds them, at
// their drawn foot) at the heights the explorer meets it: the walking band (0.05 to 1.7 m over the ground) and the span above (the jetpack)
namespace {
struct WSeg { double x0, z0, x1, z1; };
struct WPt { double x, z, w; };
// where a mesh's polygons cross the plane y = h (a polygon lying in it: its edges), and its lines' points with their half widths
void wallSlice(const FormMesh& m, double h, std::vector<WSeg>& segs, std::vector<WPt>& pts) {
    segs.clear(); pts.clear();
    for (const FormMesh::Poly& P : m.polys) {
        const FormMesh::V* v = &m.v[P.first];
        bool flat = true; for (int k = 0; k < P.n && flat; k++) flat = std::fabs(v[k].p.y - h) < 1e-4;
        if (flat) { for (int k = 0; k < P.n; k++) { const Vec3& a = v[k].p; const Vec3& b = v[(k + 1) % P.n].p; segs.push_back({a.x, a.z, b.x, b.z}); } continue; }
        double ix[16], iz[16]; int ni = 0;
        for (int k = 0; k < P.n && ni < 16; k++) {
            const Vec3& a = v[k].p; const Vec3& b = v[(k + 1) % P.n].p;
            const double da = a.y - h, db = b.y - h;
            if ((da < 0) == (db < 0)) continue;
            const double t = da / (da - db);
            ix[ni] = a.x + (b.x - a.x) * t; iz[ni] = a.z + (b.z - a.z) * t; ni++;
        }
        if (ni < 2) continue;
        int bi = 0, bj = 1; double best = -1;
        for (int i = 0; i < ni; i++) for (int j = i + 1; j < ni; j++) { const double d = (ix[i] - ix[j]) * (ix[i] - ix[j]) + (iz[i] - iz[j]) * (iz[i] - iz[j]); if (d > best) { best = d; bi = i; bj = j; } }
        segs.push_back({ix[bi], iz[bi], ix[bj], iz[bj]});
    }
    // two faces at one place (a pier's side and the strip's beside it) cross the plane in one segment: once only, or the crossings' parity fails
    for (size_t i = 0; i < segs.size(); i++)
        for (size_t j = i + 1; j < segs.size(); j++) {
            const WSeg& a = segs[i]; const WSeg& b = segs[j];
            const bool same = (std::fabs(a.x0 - b.x0) < 1e-4 && std::fabs(a.z0 - b.z0) < 1e-4 && std::fabs(a.x1 - b.x1) < 1e-4 && std::fabs(a.z1 - b.z1) < 1e-4) ||
                              (std::fabs(a.x0 - b.x1) < 1e-4 && std::fabs(a.z0 - b.z1) < 1e-4 && std::fabs(a.x1 - b.x0) < 1e-4 && std::fabs(a.z1 - b.z0) < 1e-4);
            if (same) { segs.erase(segs.begin() + j); j--; }
        }
    for (const FormMesh::Line& l : m.lines) {
        const double da = l.a.y - h, db = l.b.y - h;
        if ((da < 0) == (db < 0)) continue;
        const double t = da / (da - db);
        pts.push_back({l.a.x + (l.b.x - l.a.x) * t, l.a.z + (l.b.z - l.a.z) * t, l.width});
    }
}
double segDist(const WSeg& s, double x, double z) {
    const double dx = s.x1 - s.x0, dz = s.z1 - s.z0, L2 = dx * dx + dz * dz;
    const double t = L2 > 1e-12 ? clampd(((x - s.x0) * dx + (z - s.z0) * dz) / L2, 0, 1) : 0;
    return std::hypot(s.x0 + dx * t - x, s.z0 + dz * t - z);
}
// a point inside the solid a slice bounds (an odd count of crossings in each of the four directions: an open hall's two flanks do not
// enclose its floor) or within `tol` of its surface
bool sliceNear(const std::vector<WSeg>& segs, const std::vector<WPt>& pts, double x, double z, double tol) {
    for (const WSeg& s : segs) if (segDist(s, x, z) <= tol) return true;
    for (const WPt& p : pts) if (std::hypot(p.x - x, p.z - z) <= tol + p.w) return true;
    int c[4] = {0, 0, 0, 0};
    for (const WSeg& s : segs) {
        if ((s.z0 > z) != (s.z1 > z)) { const double xx = s.x0 + (z - s.z0) * (s.x1 - s.x0) / (s.z1 - s.z0); c[xx > x ? 0 : 1]++; }
        if ((s.x0 > x) != (s.x1 > x)) { const double zz = s.z0 + (x - s.x0) * (s.z1 - s.z0) / (s.x1 - s.x0); c[zz > z ? 2 : 3]++; }
    }
    return (c[0] & 1) && (c[1] & 1) && (c[2] & 1) && (c[3] & 1);
}
// the slice covers a disc: the explorer meets it on its rim, so each of eight points round its rim lies inside the solid or within `tol` of
// its surface (a disc with its middle inside but its rim far out past the drawn surface was a wall a metre before it)
bool wallCovered(const std::vector<WSeg>& segs, const std::vector<WPt>& pts, double cx, double cz, double r, double tol) {
    if (r <= 0) return sliceNear(segs, pts, cx, cz, tol);
    for (int k = 0; k < 8; k++) { const double a = k * TAU / 8; if (!sliceNear(segs, pts, cx + std::cos(a) * r, cz + std::sin(a) * r, tol)) return false; }
    return true;
}
// how much of a disc (its heights over the ground `gy`) the meshes cover: the walking band sampled every 0.2 m, the span above every metre or so
struct DiscCover { int walkN = 0, walkCov = 0, upN = 0, upCov = 0; double hiCov = -1e9; };
DiscCover discCover(const std::vector<const FormMesh*>& meshes, double qx, double qz, double qr, double y0, double y1, double gy, double tol = 0.25) {
    DiscCover D;
    static thread_local std::vector<WSeg> segs, all; static thread_local std::vector<WPt> pts, allPts;
    auto at = [&](double h) {   // the meshes' slices together (a disc at a wall's joint has its rim in the next wall)
        all.clear(); allPts.clear();
        for (const FormMesh* m : meshes) { wallSlice(*m, gy + h, segs, pts); all.insert(all.end(), segs.begin(), segs.end()); allPts.insert(allPts.end(), pts.begin(), pts.end()); }
        if (meshes.size() == 1) return wallCovered(all, allPts, qx, qz, qr, tol);
        for (int k = 0; k < 8; k++) {   // each rim point inside one of the solids or near a surface
            const double a = k * TAU / 8, x = qx + std::cos(a) * qr, z = qz + std::sin(a) * qr;
            bool ok = false;
            for (const FormMesh* m : meshes) { wallSlice(*m, gy + h, segs, pts); if (sliceNear(segs, pts, x, z, tol)) { ok = true; break; } }
            if (!ok) return false;
        }
        return true;
    };
    const double a = std::max(y0 - gy, 0.05), b = std::min(y1 - gy, 1.7);
    if (b > a) for (double h = a; h <= b + 1e-6; h += 0.2) { D.walkN++; if (at(h)) { D.walkCov++; D.hiCov = std::max(D.hiCov, h); } }
    const double c = std::max(y0 - gy, 1.7), d = y1 - gy;
    if (d > c + 0.3) { const int n = clampi((int)std::ceil((d - c) / 1.0), 1, 40); for (int k = 0; k < n; k++) { D.upN++; if (at(c + (d - c) * (k + 0.5) / n)) D.upCov++; } }
    return D;
}
// a disc the explorer meets where nothing is drawn: at the feet (it blocks a walk and nothing stands at it over the knee) or above (the jetpack
// meets it where under half of it is drawn)
bool ghostAtFeet(const DiscCover& D, double y1, double gy) { return D.walkN > 0 && y1 - gy > 0.95 && D.hiCov < 0.75; }
bool ghostAbove(const DiscCover& D) { return D.upN > 0 && D.upCov * 2 < D.upN; }

struct WallTally {
    long forms = 0, discs = 0, feet = 0, above = 0, pieces = 0, places = 0;
    struct K { long discs = 0, feet = 0, above = 0; std::string example; };
    std::map<std::string, K> byKey;
};
std::string wallKey(const Form& e, const RuinSpec* sp) {
    const char* bk = sp && e.building >= 0 && e.building < (int)sp->buildings.size() ? BUILDING_KIND_NAMES[sp->buildings[e.building].kind] : (e.building < 0 ? "GRAVE" : "?");
    return fmt("%-8s %d %-7s%s part %2d in %s", FORM_KIND_NAMES[e.kind], e.sub, e.tilt > 0.3 ? "lying" : (e.tilt != 0 ? "leaning" : "upright"), e.broken > 0 ? " broken" : "       ", e.part, bk);
}
// every piece of a settlement or a work against its own colliders on flat ground (the forms' own agreement, whatever the ground)
void wallsOfPieces(const std::vector<Form>& el, const RuinSpec* sp, const std::string& where, WallTally& T) {
    FormMesh mesh; std::vector<FormDisc> discs; std::vector<const FormMesh*> ms{&mesh};
    T.places++;
    for (size_t i = 0; i < el.size(); i++) {
        const Form& e = el[i];
        T.pieces++;
        discs.clear(); formColliders(e, discs);
        if (discs.empty()) continue;
        formMesh(e, 0, e.absolute ? Vec3(e.x, 0, e.z) : Vec3(e.x, 0, e.z), e.y0 < 0.01 && !e.absolute ? 1.0 : 0.0, mesh, 1e9);
        T.forms++;
        for (const FormDisc& q : discs) {
            T.discs++;
            const double base = e.absolute ? e.y0 : 0.0;   // an absolute piece's discs over its own foot (`settlementColliders`)
            const DiscCover D = discCover(ms, q.x, q.z, q.r, base + q.y0, base + q.y1, base);
            const bool f = ghostAtFeet(D, base + q.y1, base), a = ghostAbove(D);
            WallTally::K& k = T.byKey[wallKey(e, sp)];
            k.discs++;
            if (f) { T.feet++; k.feet++; }
            if (a) { T.above++; k.above++; }
            if ((f || a) && k.example.empty())
                k.example = fmt("%s: piece %zu (%.1f %.1f, hx %.2f hz %.2f, y %.2f..%.2f, tilt %.2f, broken %.2f), disc r %.2f at %.1f %.1f y %.2f..%.2f, walk %d/%d (top %.2f), above %d/%d\n      FORM %d %d %.4f %.4f %.6f %.4f %.4f %.4f %.4f %.6f %.6f %.6f %.6f %d %d %.6f %.6f %u %d",
                                where.c_str(), i, e.x, e.z, e.hx, e.hz, e.y0, e.y1, e.tilt, e.broken, q.r, q.x, q.z, q.y0, q.y1, D.walkCov, D.walkN, D.hiCov, D.upCov, D.upN,
                                e.kind, e.sub, e.x, e.z, e.heading, e.hx, e.hz, e.y0, e.y1, e.p[0], e.p[1], e.p[2], e.p[3], e.sides, e.part, e.broken, e.tilt, e.seed, (int)e.absolute);
        }
    }
}
void printWallTally(const WallTally& T, int top) {
    printf("  %ld places, %ld pieces, %ld with colliders, %ld discs: %ld at the feet over nothing drawn (%.3f%%), %ld above it mostly over nothing (%.3f%%)\n",
           T.places, T.pieces, T.forms, T.discs, T.feet, 100.0 * T.feet / std::max(1L, T.discs), T.above, 100.0 * T.above / std::max(1L, T.discs));
    std::vector<std::pair<long, std::string>> rows;
    for (const auto& kv : T.byKey) if (kv.second.feet + kv.second.above > 0) rows.push_back({kv.second.feet * 4 + kv.second.above, kv.first});
    std::sort(rows.begin(), rows.end(), [](const std::pair<long, std::string>& a, const std::pair<long, std::string>& b) { return a.first > b.first; });
    for (int i = 0; i < (int)rows.size() && i < top; i++) {
        const WallTally::K& k = T.byKey.at(rows[i].second);
        printf("    %s: %ld discs, %ld at the feet, %ld above\n      e.g. %s\n", rows[i].second.c_str(), k.discs, k.feet, k.above, k.example.c_str());
    }
}
// the places of a people's world the checks walk: its capital, its first city, a town, a village, a hamlet and a lone monument of the countryside
// round the capital, and its works (the world's within `worksKm` of the capital, the land's round its first places)
struct WallPlace { std::string what; RuinSpec sp; int people = 0; int gLat = 0, gLon = 0; uint64_t work = 0; };
void wallPlacesOf(const BodyGen& g, const std::string& name, int people, std::vector<WallPlace>& out, int countryside = 4, double worksKm = 400) {
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    if (people >= P->peoples) return;
    const PeoplePlan& pp = P->plans[people];
    if (pp.places.empty()) return;
    std::vector<int> ranks{0};
    for (const PlanPlace& p : pp.places) if (p.sclass == SC_CITY) { ranks.push_back(p.rank); break; }
    for (int rk : ranks) { WallPlace w; w.people = people; w.gLat = pp.places[rk].gLat; w.gLon = pp.places[rk].gLon; if (ruinOfCell(g, w.gLat, w.gLon, w.sp, true)) { w.what = fmt("%s's %s", name.c_str(), rk == 0 ? "capital" : "city"); out.push_back(w); } }
    int want[4] = {SC_TOWN, SC_VILLAGE, SC_HAMLET, SC_MONUMENT}; bool got[4] = {false, false, false, false}; int n = 0;
    for (int ring = 1; ring < 40 && n < std::min(4, countryside); ring++)
        for (int dl = -ring; dl <= ring && n < std::min(4, countryside); dl++)
            for (int dn = -ring; dn <= ring && n < std::min(4, countryside); dn++) {
                if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                const int row = pp.places[0].gLat + dl, col = ruinCellLonIn(g, pp.places[0].gLat, pp.places[0].gLon, row) + dn;
                RuinSpec q; if (!ruinOfCell(g, row, col, q, false) || q.rank >= 0 || q.people != people) continue;
                int k = -1; for (int j = 0; j < 4; j++) if (q.sclass == want[j] && !got[j]) k = j;
                if (k < 0) continue;
                WallPlace w; w.people = people; w.gLat = row; w.gLon = col;
                if (!ruinOfCell(g, row, col, w.sp, true)) continue;
                got[k] = true; n++;
                w.what = fmt("%s's countryside %s", name.c_str(), SETTLEMENT_CLASS_NAMES[q.sclass]);
                out.push_back(w);
            }
    if (worksKm <= 0) return;
    std::vector<std::shared_ptr<const Work>> W; worksAround(g, pp.places[0].unit, worksKm * 1000, W);
    std::vector<std::shared_ptr<const Work>> L; landWorksOf(g, people, 3, 30000, 6, L);
    for (const auto& w : L) W.push_back(w);
    std::set<uint64_t> seen;
    for (const auto& w : W) {
        if (w->people != people || !seen.insert(w->id).second) continue;
        WallPlace p; p.people = people; p.work = w->id; workSpec(g, *w, p.sp);
        p.what = fmt("%s's %s%s", name.c_str(), WORK_KIND_NAMES[w->kind], w->land ? "" : " (the world's)");
        out.push_back(p);
    }
}
// the drawn pieces near a point of a view: their meshes at their drawn foot, as the frame builds them near (the records' and the instruments'
// objects left out: they have no colliders)
struct NearMesh { FormMesh m; const Form* f = nullptr; };
void drawnNear(SurfaceView& sv, double x, double z, double reach, std::vector<NearMesh>& out) {
    out.clear();
    auto take = [&](const Ruin& r, const SurfaceView::RuinCell& cell) {
        for (const Form& e : cell.elems[0]) {
            const double ex = r.x + e.x, ez = r.z + e.z, rr = formReach(e) + reach;
            if ((ex - x) * (ex - x) + (ez - z) * (ez - z) > rr * rr) continue;
            NearMesh nm; nm.f = &e;
            if (e.absolute) formMesh(e, 0, Vec3(ex, 0, ez), 0.0, nm.m, 1e9);
            else { double gy, skirt; if (!sv.pieceFootNear(e, ex, ez, gy, skirt)) continue; formMesh(e, 0, Vec3(ex, gy, ez), skirt, nm.m, 1e9); }
            out.push_back(std::move(nm));
        }
    };
    sv.forNearbyRuins(x, z, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) { take(r, cell); });
    for (const auto& c : sv.placedCells) take(c.r, c);
}
// what stops the explorer at (x, z) walking along (dx, dz) with its feet at `feet`: the ground's slope, a heap's or a tier's side, a boulder, a
// trunk, a log, a drawn piece; "" nothing drawn there (an invisible wall)
std::string whatStops(SurfaceView& sv, double x, double z, double dx, double dz, double feet) {
    const double g0 = sv.site.groundHeight(x, z), g1 = sv.site.groundHeight(x + dx * 0.5, z + dz * 0.5);
    if ((g1 - g0) / 0.5 > 1.2) return "the ground's slope";
    sv.collectSupports(x, z);
    { bool bl = false; sv.floorAt(x + dx * 0.5, z + dz * 0.5, feet, &bl); if (bl) return "a heap's or a tier's side"; }
    { const double f0 = sv.floorAt(x, z, feet), f1 = sv.floorAt(x + dx * 0.5, z + dz * 0.5, 1e9); if ((f1 - f0) / 0.5 > 1.2) return "a heap's slope"; }
    std::vector<SurfaceView::Collider> cols; sv.collectColliders(x, z, cols);
    for (const auto& c : cols) {
        if (c.kind == 3 || std::hypot(c.x - x, c.z - z) > c.r + 0.6 || c.y1 < feet + 0.1 || c.y0 > feet + 1.6) continue;
        return c.kind == 0 ? "a boulder" : (c.kind == 1 ? "a trunk" : "a log");
    }
    std::vector<NearMesh> near; drawnNear(sv, x, z, 2.0, near);
    std::vector<WSeg> segs; std::vector<WPt> pts;
    for (const NearMesh& nm : near) for (double h : {0.3, 0.9, 1.5}) { wallSlice(nm.m, feet + h, segs, pts); if (sliceNear(segs, pts, x + dx * 0.3, z + dz * 0.3, 0.7)) return "a drawn piece"; }
    return "";
}
struct WalkTally { int walks = 0, stuck = 0, ghosts = 0, swum = 0, rocks = 0, rocksOnBeds = 0; double metres = 0; std::map<std::string, int> causes; std::vector<std::string> ex; };
// `n` straight walks through a place from random points within its radius (on dry ground, clear of every collider), each up to `secs` at a
// walk: where the explorer stops, what stopped it
void walkPlace(SurfaceView& sv, double px, double pz, double R, int n, double secs, uint64_t seed, const std::string& what, WalkTally& T) {
    Rng rng(seed);
    std::vector<SurfaceView::Collider> cols;
    for (int w = 0, tries = 0; w < n && tries < n * 20; tries++) {
        const double a = rng.uni() * TAU, rr = std::sqrt(rng.uni()) * (R + 20), sx = px + std::sin(a) * rr, sz = pz + std::cos(a) * rr;
        const double gy = sv.site.groundHeight(sx, sz), wl = sv.site.waterAt(sx, sz);
        if (wl > -1e8 && gy < wl + 0.2) continue;
        sv.collectColliders(sx, sz, cols);
        for (const auto& c : cols) {   // B-420: a boulder that blocks is one drawn: off a road's or a street's bed, off a work's ground
            if (c.kind != 0) continue;
            T.rocks++;
            if ((!sv.site.roads.empty() && sv.site.roadCover(c.x, c.z) > 0.3) || (!sv.site.marks.empty() && sv.site.markCover(c.x, c.z) > 0.3) || (!sv.site.edits.empty() && sv.site.editCover(c.x, c.z) > 0.3)) T.rocksOnBeds++;
        }
        bool clear = true; for (const auto& c : cols) if (std::hypot(c.x - sx, c.z - sz) < c.r + 0.5 && c.y0 < gy + 1.8 && c.y1 > gy + 0.1) { clear = false; break; }
        if (!clear) continue;
        sv.collectSupports(sx, sz); { bool bl = false; if (sv.floorAt(sx, sz, gy, &bl) > gy + 0.2 || bl) continue; }
        w++; T.walks++;
        const double heading = rng.uni() * TAU, hx = std::sin(heading), hz = std::cos(heading);
        sv.player.x = sx; sv.player.z = sz; sv.player.y = gy; sv.player.vx = sv.player.vz = sv.player.vy = 0; sv.player.yaw = heading; sv.player.pitch = 0;
        sv.player.onGround = true; sv.player.swimming = false; sv.player.autoWalk = 0; sv.player.stamina = 100;
        Input in; in.down[KEY_W] = true;
        double lx = sx, lz = sz; int still = 0;
        const int steps = (int)(secs * 30);
        for (int k = 0; k < steps; k++) {
            sv.player.yaw = heading;
            sv.update(1.0 / 30, in, 0.0, true);
            in.newFrame();
            const double mv = std::hypot(sv.player.x - lx, sv.player.z - lz);
            T.metres += mv; lx = sv.player.x; lz = sv.player.z;
            if (sv.player.swimming) { T.swum++; break; }
            still = mv < 0.02 ? still + 1 : 0;
            if (still < 6) continue;
            T.stuck++;
            const std::string why = whatStops(sv, sv.player.x, sv.player.z, hx, hz, sv.player.y);
            T.causes[why.empty() ? "NOTHING DRAWN" : why]++;
            if (why.empty()) {
                T.ghosts++;
                if (T.ex.size() < 12) {
                    std::string near;
                    sv.collectColliders(sv.player.x, sv.player.z, cols);
                    for (const auto& c : cols) if (std::hypot(c.x - sv.player.x, c.z - sv.player.z) < c.r + 0.8) near += fmt(" [kind %d r %.2f at %.1f %.1f y %.2f..%.2f]", c.kind, c.r, c.x - sv.player.x, c.z - sv.player.z, c.y0 - sv.player.y, c.y1 - sv.player.y);
                    double lat, lon, slat, slon, clat, clon; sv.site.latLonAt(sv.player.x, sv.player.z, lat, lon); sv.site.latLonAt(sx, sz, slat, slon); sv.site.latLonAt(0, 0, clat, clon);
                    T.ex.push_back(fmt("%s: stopped at %.5f %.5f heading %.1f, feet %.2f over the ground%s\n      (walls walk ... %.6f %.6f %.3f %.6f %.6f)", what.c_str(), lat / DEG, lon / DEG, heading / DEG, sv.player.y - sv.site.groundHeight(sv.player.x, sv.player.z), near.c_str(), slat / DEG, slon / DEG, heading / DEG, clat / DEG, clon / DEG));
                }
            }
            break;
        }
    }
}
// B-421: where a cell's records and instruments lie, as the explorer meets them: in water (not drawn), inside a piece, a boulder or a trunk,
// under a heap, inside a drawn piece without a collider (a low heap of rubble, a drift), out of reach (a walk on a 25 cm grid from round it
// to within E's reach, on its floor), an instrument through a wall or over a record
struct RecTally { int records = 0, instruments = 0, water = 0, inside = 0, heap = 0, hidden = 0, unreachable = 0, through = 0, overlap = 0; std::map<std::string, int> byPlace; std::vector<std::string> ex; };
bool reachableOnFoot(SurfaceView& sv, double ix, double iz, double reach, double& gyItem, double half = 10.0) {
    const double cell = half > 30 ? 0.4 : 0.25;
    const int N = (int)(2 * half / cell);
    const double x0 = ix - half, z0 = iz - half;
    gyItem = sv.site.groundHeight(ix, iz);
    std::vector<SurfaceView::Collider> all, cols;
    for (int q = 0; q < 5; q++) {
        const double qx = q == 0 ? ix : ix + ((q & 1) ? half : -half), qz = q == 0 ? iz : iz + ((q & 2) ? half : -half);
        sv.collectColliders(qx, qz, cols);
        for (const auto& c : cols) { bool dup = false; for (const auto& d : all) if (std::fabs(d.x - c.x) < 1e-6 && std::fabs(d.z - c.z) < 1e-6 && std::fabs(d.r - c.r) < 1e-6 && std::fabs(d.y0 - c.y0) < 1e-6) { dup = true; break; } if (!dup) all.push_back(c); }
    }
    sv.collectSupports(ix, iz, half + 2);
    std::vector<float> floorH((size_t)N * N);
    std::vector<char> blocked((size_t)N * N, 0);
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) { const double x = x0 + (i + 0.5) * cell, z = z0 + (j + 0.5) * cell; floorH[(size_t)j * N + i] = (float)sv.floorAt(x, z, 1e9); }   // a heap's whole top (it is solid)
    for (const auto& c : all) {
        const double rr = c.r + 0.35;
        const int i0 = std::max(0, (int)std::floor((c.x - rr - x0) / cell)), i1 = std::min(N - 1, (int)std::floor((c.x + rr - x0) / cell));
        const int j0 = std::max(0, (int)std::floor((c.z - rr - z0) / cell)), j1 = std::min(N - 1, (int)std::floor((c.z + rr - z0) / cell));
        for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) {
            const double x = x0 + (i + 0.5) * cell, z = z0 + (j + 0.5) * cell;
            if (std::hypot(x - c.x, z - c.z) >= rr) continue;
            const double f = floorH[(size_t)j * N + i];
            if (c.y0 < f + 1.6 && c.y1 > f + 0.1) blocked[(size_t)j * N + i] = 1;
        }
    }
    std::vector<char> seen((size_t)N * N, 0);
    std::vector<int> q;
    for (int k = 0; k < N; k++) for (int e = 0; e < 4; e++) {
        const int i = e == 0 ? k : (e == 1 ? k : (e == 2 ? 0 : N - 1)), j = e == 0 ? 0 : (e == 1 ? N - 1 : k);
        const size_t id = (size_t)j * N + i;
        if (!blocked[id] && !seen[id]) { seen[id] = 1; q.push_back((int)id); }
    }
    static int dumps = 0;
    auto dump = [&]() {   // `WALLS_DUMP`: the grid of a failure as a picture (blocked dark red, reached green, the item white, north up)
        if (!getenv("WALLS_DUMP") || dumps >= 6) return;
        std::vector<uint32_t> px((size_t)N * N);
        for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
            const size_t id = (size_t)j * N + i; const double x = x0 + (i + 0.5) * cell, z = z0 + (j + 0.5) * cell;
            uint32_t c = blocked[id] ? rgb(150, 30, 30) : (seen[id] ? rgb(40, 140, 60) : rgb(30, 30, 40));
            if (std::hypot(x - ix, z - iz) < 0.3) c = rgb(255, 255, 255);
            else if (std::hypot(x - ix, z - iz) < reach + 0.13 && std::hypot(x - ix, z - iz) > reach - 0.13) c = rgb(200, 200, 90);
            px[(size_t)(N - 1 - j) * N + i] = c;
        }
        writePNG(fmt("shots/tests/walls_reach_%d.png", dumps++).c_str(), px.data(), N, N);
    };
    for (size_t h = 0; h < q.size(); h++) {
        const int id = q[h], i = id % N, j = id / N;
        const double x = x0 + (i + 0.5) * cell, z = z0 + (j + 0.5) * cell;
        if (std::hypot(x - ix, z - iz) <= reach && std::fabs(floorH[id] - gyItem) <= 1.9) return true;
        const int di[4] = {1, -1, 0, 0}, dj[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; k++) {
            const int i2 = i + di[k], j2 = j + dj[k];
            if (i2 < 0 || j2 < 0 || i2 >= N || j2 >= N) continue;
            const int id2 = j2 * N + i2;
            if (seen[id2] || blocked[id2] || floorH[id2] - floorH[id] > 1.35 * cell) continue;   // a step up over the slope's 1.35 a run
            seen[id2] = 1; q.push_back(id2);
        }
    }
    dump();
    return false;
}
void recordsOfCell(SurfaceView& sv, const Ruin& r, const SurfaceView::RuinCell& cell, const std::string& what, RecTally& T) {
    std::vector<SurfaceView::Collider> cols;
    std::vector<NearMesh> near;
    std::vector<WSeg> segs; std::vector<WPt> pts;
    {   // the explorer at the cell first: its records laid with the water as drawn (`waterRecords`), then copied (the checks move the explorer)
        sv.player.x = r.x; sv.player.z = r.z; sv.player.y = sv.site.groundHeight(r.x, r.z);
        Input in0; sv.update(1.0 / 60, in0, 0.0, false);
    }
    const std::vector<ShardSite> shards = cell.shards;
    const std::vector<InstrumentSite> instruments = cell.instruments;
    const std::vector<int> ids = cell.instrumentIds, first = cell.instrumentFirst;
    const std::vector<Form> objs = cell.instrumentForms;
    auto check = [&](double x, double z, const std::string& kind, int building, const Form* objBegin, const Form* objEnd) {
        {   // the explorer beside it: the near ring (4 m) drawn there, as when it is found (a few frames: it is laid as the explorer comes)
            sv.player.x = x + 2.5; sv.player.z = z; sv.player.y = sv.site.groundHeight(sv.player.x, sv.player.z); sv.player.vx = sv.player.vz = sv.player.vy = 0;
            Input in0; for (int k = 0; k < 3; k++) sv.update(1.0 / 60, in0, 0.0, false);
        }
        const double gy = sv.site.groundHeight(x, z), w = sv.site.waterAt(x, z);
        const char* place = building >= 0 && building < (int)cell.spec.buildings.size() ? buildingPlaceName(cell.spec.buildings[building].kind) : "?";
        std::string bad;
        if (w > -1e8 && gy < w + 0.05) { T.water++; bad += " IN WATER"; }
        sv.collectColliders(x, z, cols);
        for (const auto& c : cols) if (std::hypot(c.x - x, c.z - z) < c.r + 0.12 && c.y0 < gy + 0.4 && c.y1 > gy + 0.05) { T.inside++; bad += fmt(" INSIDE A %s", c.kind == 0 ? "BOULDER" : (c.kind == 1 ? "TRUNK" : (c.kind == 2 ? "LOG" : "PIECE"))); break; }
        sv.collectSupports(x, z);
        if (sv.floorAt(x, z, 1e9) > gy + 0.25) { T.heap++; bad += " UNDER A HEAP"; }
        drawnNear(sv, x, z, 0.5, near);
        for (const NearMesh& nm : near) { wallSlice(nm.m, gy + 0.06, segs, pts); if (sliceNear(segs, pts, x, z, -0.02)) { T.hidden++; bad += fmt(" HIDDEN IN A %s (part %d)", FORM_KIND_NAMES[nm.f->kind], nm.f->part); break; } }
        double gi; if (!reachableOnFoot(sv, x, z, 2.0, gi, clampd(cell.spec.size + 8, 10, cell.spec.work >= 0 ? 90.0 : 30.0))) {
            T.unreachable++; { double la, lo; sv.site.latLonAt(x, z, la, lo); bad += fmt(" OUT OF REACH (at %.6f %.6f degrees: `walls reach`)", la / DEG, lo / DEG); }
            if (getenv("WALLS_DUMP"))   // what stands round it with a collider
                for (const Form& e : cell.elems[0]) {
                    const double ex = r.x + e.x, ez = r.z + e.z;
                    if (std::hypot(ex - x, ez - z) > 16 + formReach(e)) continue;
                    std::vector<FormDisc> d1; formColliders(e, d1);
                    if (d1.empty()) continue;
                    const Building* B = e.building >= 0 && e.building < (int)cell.spec.buildings.size() ? &cell.spec.buildings[e.building] : nullptr;
                    printf("      near: %s %d part %d of a %s at %.1f %.1f (%.1f off), hx %.2f hz %.2f y %.2f..%.2f tilt %.2f, %zu discs\n", FORM_KIND_NAMES[e.kind], e.sub, e.part, B ? BUILDING_KIND_NAMES[B->kind] : "-", ex - x, ez - z, std::hypot(ex - x, ez - z), e.hx, e.hz, e.y0, e.y1, e.tilt, d1.size());
                }
        }
        if (objBegin) {   // an instrument's body: clear of the pieces' colliders and of the drawn pieces at its height
            bool through = false;
            for (const Form* f = objBegin; f != objEnd && !through; f++) {
                if (f->kind == FK_CABLE) continue;
                const double ca = std::cos(f->heading), sa = std::sin(f->heading);
                for (int k = 0; k < 8 && !through; k++) {
                    const double a = f->hx * ((k & 1) ? 1 : -1) * ((k & 4) ? 0.5 : 1.0), b = f->hz * ((k & 2) ? 1 : -1) * ((k & 4) ? 0.5 : 1.0);
                    const double px = r.x + f->x + ca * a + sa * b, pz = r.z + f->z - sa * a + ca * b;
                    for (const auto& c : cols) if (c.kind == 3 && std::hypot(c.x - px, c.z - pz) < c.r - 0.05 && c.y0 < gy + f->y1 && c.y1 > gy + f->y0) { through = true; break; }
                }
            }
            if (through) { T.through++; bad += " THROUGH A WALL"; }
            for (const ShardSite& s : shards) if (std::hypot(r.x + s.x - x, r.z + s.z - z) < 0.5) { T.overlap++; bad += " OVER A RECORD"; break; }
        }
        T.byPlace[std::string(kind) + " in " + place]++;
        if (!bad.empty() && T.ex.size() < 40) { double lat, lon; sv.site.latLonAt(x, z, lat, lon); T.ex.push_back(fmt("%s: %s in %s at %.5f %.5f:%s", what.c_str(), kind.c_str(), place, lat / DEG, lon / DEG, bad.c_str())); }
    };
    for (size_t k = 0; k < shards.size(); k++) { const ShardSite& s = shards[k]; T.records++; check(r.x + s.x, r.z + s.z, s.wallD > 0 ? "a painted wall's stand" : "a record", s.building, nullptr, nullptr); }
    if (ids.size() == instruments.size() && first.size() == instruments.size() + 1)
        for (size_t k = 0; k < instruments.size(); k++) {
            if (ids[k] < 0) continue;
            const InstrumentSite& s = instruments[k]; T.instruments++;
            const Form* f0 = objs.data() + first[k], *f1 = objs.data() + first[k + 1];
            check(r.x + s.x, r.z + s.z, "an instrument", s.building, f0, f1);
        }
}
}   // namespace

// `walls forms [peoples] [years ...]`: every piece of the age worlds' places and works, the scene worlds' and BUBAS's (the user's world), and of
// `peoples` peoples near home as if each had reached every age (towns at the years given), against its own colliders on flat ground
int runWalls(int argc, char** argv) {
    const std::string what = argc > 2 ? argv[2] : "forms";
    if (what == "forms") {
        const int nPeoples = argc > 3 ? atoi(argv[3]) : 12;
        std::vector<double> years; for (int i = 4; i < argc; i++) years.push_back(atof(argv[i]));
        if (years.empty()) years = {30, 600, 6000, 40000};
        const double t0 = nowSec();
        WallTally T;
        struct WW { int64_t sx, sy, sz; int body; const char* label; };
        std::vector<WW> worlds;
        for (int a = 0; a < AGE_COUNT; a++) worlds.push_back({AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, AGE_WORLDS[a].body, "age"});
        for (int a = 0; a < AGE_COUNT; a++) if (AGE_SCENE_WORLDS[a].sx != AGE_WORLDS[a].sx || AGE_SCENE_WORLDS[a].sz != AGE_WORLDS[a].sz) worlds.push_back({AGE_SCENE_WORLDS[a].sx, 0, AGE_SCENE_WORLDS[a].sz, AGE_SCENE_WORLDS[a].body, "scene"});
        worlds.push_back({10549, -8, -5606, 2, "BUBAS"});
        for (const WW& w : worlds) {
            Star st; StarSystem sys; if (!systemAt(w.sx, w.sy, w.sz, st, sys, true)) continue;
            if (w.body >= (int)sys.bodies.size()) continue;
            const BodyGen g = BodyGen::make(sys.bodies[w.body]);
            if (!worldHadCivilisation(g)) continue;
            for (int p = 0; p < peoplesOf(g); p++) {
                const Culture cu = cultureOf(g, p);
                const Lore L = loreOf(sys, sys.bodies[w.body], g, p);
                std::vector<WallPlace> places; wallPlacesOf(g, sys.bodies[w.body].name + (p ? " (second)" : ""), p, places);
                for (const WallPlace& wp : places) {
                    std::vector<Form> el; ruinElements(wp.sp, cu, 0, el);
                    std::vector<Grave> gr; gravesOf(L, wp.sp, cu, gr); graveElements(gr, cu, 0, el, wp.sp.work >= 0);
                    wallsOfPieces(el, &wp.sp, wp.what + fmt(" (%s, %.0f years)", AGE_NAMES[cu.age], cu.ageYears), T);
                }
                printf("  %s%s: %zu places and works (%.1f s)\n", sys.bodies[w.body].name.c_str(), p ? " (second)" : "", places.size(), nowSec() - t0);
            }
        }
        {   // the peoples near home in every age, a town and a village at each of the years
            Star st; StarSystem sys; const AgeWorld& mw = AGE_WORLDS[AGE_MEDIEVAL];
            if (starInSector(mw.sx, 0, mw.sz, st, true)) {
                sys.generate(st);
                const BodyGen g = BodyGen::make(sys.bodies[mw.body]);
                double la = 0.2, lo = 0.7; planPlacePoint(g, 0, la, lo);
                int gLat0, gLon0; ruinCellOf(g, la, lo, gLat0, gLon0);
                const std::vector<NearPeople> NP = peoplesNearHome(nPeoples);
                for (size_t k = 0; k < NP.size(); k++)
                    for (int age = 0; age < AGE_COUNT; age++)
                        for (size_t y = 0; y < years.size(); y++)
                            for (int sc = 0; sc < 2; sc++) {
                                const Culture c = cultureWithGenome(g, age, years[y], NP[k].seed);
                                RuinSpec sp; settlementOf(g, gLat0 + 2 * (int)k, gLon0 + 3 * age + (int)y, c, age == AGE_FAR_FUTURE ? SC_MONUMENT : (sc ? SC_VILLAGE : SC_TOWN), sp);
                                std::vector<Form> el; ruinElements(sp, c, 0, el);
                                wallsOfPieces(el, &sp, fmt("%s as %s, %s, %.0f years", NP[k].world.c_str(), AGE_NAMES[age], sc ? "village" : "town", years[y]), T);
                            }
                printf("  %zu peoples near home in every age at %zu years (%.1f s)\n", NP.size(), years.size(), nowSec() - t0);
            }
        }
        printWallTally(T, 40);
        printf("(%.1f s)\n", nowSec() - t0);
        return 0;
    }
    if (what == "view") {   // `walls view [worlds] [walks]` (the test worlds and BUBAS) or `walls view near <peoples> [walks]` (the worlds of the peoples nearest
                            // home): the places in the view: the explorer walked through them, their records and instruments
        const bool nearHome = argc > 3 && std::string(argv[3]) == "near";
        const int nWorlds = nearHome ? 999 : (argc > 3 ? atoi(argv[3]) : 99), walkScale = argc > (nearHome ? 5 : 4) ? atoi(argv[nearHome ? 5 : 4]) : 1;
        const double t0 = nowSec();
        WalkTally W; RecTally R;
        struct WW { int64_t sx, sy, sz; int body; };
        std::vector<WW> worlds;
        if (nearHome) {
            for (const NearPeople& np : peoplesNearHome(argc > 4 ? atoi(argv[4]) : 30)) {
                bool dup = false; for (const WW& w : worlds) dup = dup || (w.sx == np.x && w.sy == np.y && w.sz == np.z && w.body == np.body);
                if (!dup) worlds.push_back({np.x, np.y, np.z, np.body});
            }
        } else {
            for (int a = 0; a < AGE_COUNT; a++) worlds.push_back({AGE_SCENE_WORLDS[a].sx, 0, AGE_SCENE_WORLDS[a].sz, AGE_SCENE_WORLDS[a].body});
            for (int a = 0; a < AGE_COUNT; a++) if (AGE_SCENE_WORLDS[a].sx != AGE_WORLDS[a].sx || AGE_SCENE_WORLDS[a].sz != AGE_WORLDS[a].sz) worlds.push_back({AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, AGE_WORLDS[a].body});
            worlds.push_back({10549, -8, -5606, 2});
        }
        int done = 0;
        for (const WW& wd : worlds) {
            if (done >= nWorlds) break;
            Star st; StarSystem sys; if (!systemAt(wd.sx, wd.sy, wd.sz, st, sys, true)) continue;
            if (wd.body >= (int)sys.bodies.size()) continue;
            const BodyGen g = BodyGen::make(sys.bodies[wd.body]);
            if (!worldHadCivilisation(g)) continue;
            done++;
            worldWorksOf(g);
            for (int people = 0; people < peoplesOf(g); people++) {
                std::vector<WallPlace> places; wallPlacesOf(g, sys.bodies[wd.body].name + (people ? " (second)" : ""), people, places, 4, 60);
                // the works: one of each kind first (those that keep records and the earthworks), at most ten
                std::vector<WallPlace> pick; std::set<int> kinds; int works = 0;
                for (const WallPlace& wp : places) { if (wp.work == 0) { pick.push_back(wp); continue; } if (works < 10 && kinds.insert(wp.sp.work).second) { pick.push_back(wp); works++; } }
                for (const WallPlace& wp : pick) {
                    SurfaceView sv;
                    sv.init(&sys, wd.body, wp.sp.lat, wp.sp.lon, 0.0);
                    { Input in0; sv.update(1.0 / 30, in0, 0.0, false); }
                    const SurfaceView::RuinCell* c = nullptr;
                    if (wp.work == 0) c = sv.ruinCell(wp.gLat, wp.gLon);
                    else for (const SurfaceView::WorkNear& wn : sv.workNear) if (wn.work->id == wp.work) { c = sv.workCell(*wn.work); break; }
                    if (!c || !c->has) { printf("  %s: NOT IN THE VIEW\n", wp.what.c_str()); continue; }
                    const std::string what = wp.what + fmt(" [%lld %lld %lld %d]", (long long)wd.sx, (long long)wd.sy, (long long)wd.sz, wd.body);
                    recordsOfCell(sv, c->r, *c, what, R);
                    const int n = (wp.sp.sclass == SC_CAPITAL ? 16 : (wp.sp.sclass == SC_CITY ? 10 : (wp.work ? 4 : 6))) * walkScale;
                    walkPlace(sv, c->r.x, c->r.z, std::min(wp.sp.size, 600.0), n, 6.0, wp.sp.seed ^ 0xB420ULL, what, W);
                }
                printf("  %s%s: %zu places and works (%.1f s): %d walks, %d stopped, %d by nothing drawn; %d records, %d instruments\n", sys.bodies[wd.body].name.c_str(), people ? " (second)" : "", pick.size(), nowSec() - t0, W.walks, W.stuck, W.ghosts, R.records, R.instruments);
                fflush(stdout);
            }
        }
        printf("walks: %d (%.0f m walked), %d stopped, %d swam off; what stopped them:", W.walks, W.metres, W.stuck, W.swum);
        for (const auto& kv : W.causes) printf(" %s %d,", kv.first.c_str(), kv.second);
        printf("\n");
        for (const std::string& e : W.ex) printf("    %s\n", e.c_str());
        printf("records %d, instruments %d: in water %d, inside a piece, a boulder or a trunk %d, under a heap %d, hidden in a drawn piece %d, out of reach %d; instruments through a wall %d, over a record %d\n",
               R.records, R.instruments, R.water, R.inside, R.heap, R.hidden, R.unreachable, R.through, R.overlap);
        for (const std::string& e : R.ex) printf("    %s\n", e.c_str());
        printf("(%.1f s)\n", nowSec() - t0);
        return 0;
    }
    if (what == "check") {   // the unit's B-420 and B-421 checks alone
        int failures = 0;
        wallsChecks(modeCheck(failures));
        printf("walls check: %d failure%s\n", failures, failures == 1 ? "" : "s");
        return failures ? 1 : 0;
    }
    if (what == "one" && argc > 21) {   // `walls one <a FORM line's 19 numbers>`: one piece's discs against its own mesh on flat ground (a measure's example)
        Form e; int k = 3;
        e.kind = atoi(argv[k++]); e.sub = atoi(argv[k++]); e.x = atof(argv[k++]); e.z = atof(argv[k++]); e.heading = atof(argv[k++]); e.hx = atof(argv[k++]); e.hz = atof(argv[k++]);
        e.y0 = atof(argv[k++]); e.y1 = atof(argv[k++]); for (int q = 0; q < 4; q++) e.p[q] = atof(argv[k++]); e.sides = atoi(argv[k++]); e.part = atoi(argv[k++]);
        e.broken = atof(argv[k++]); e.tilt = atof(argv[k++]); e.seed = (uint32_t)strtoul(argv[k++], nullptr, 10); e.absolute = atoi(argv[k++]) != 0;
        std::vector<FormDisc> discs; formColliders(e, discs);
        FormMesh mesh; formMesh(e, 0, Vec3(e.x, 0, e.z), e.y0 < 0.01 && !e.absolute ? 1.0 : 0.0, mesh, 1e9);
        std::vector<const FormMesh*> ms{&mesh};
        printf("%s: %zu discs, %zu polygons, %zu lines\n", FORM_KIND_NAMES[e.kind], discs.size(), mesh.polys.size(), mesh.lines.size());
        int ghosts = 0;
        for (const FormDisc& q : discs) {
            const DiscCover D = discCover(ms, q.x, q.z, q.r, q.y0, q.y1, 0.0);
            const bool f = ghostAtFeet(D, q.y1, 0.0);
            if (f) ghosts++;
            if (f || getenv("WALLS_ALL")) {
                std::vector<WSeg> segs; std::vector<WPt> pts; const double h = std::max(0.05, 0.5 * (q.y0 + std::min(q.y1, 1.7)));
                wallSlice(mesh, h, segs, pts);
                double best = 1e9; for (const WSeg& sg : segs) best = std::min(best, segDist(sg, q.x, q.z));
                printf("  disc %.2f %.2f r %.2f y %.2f..%.2f: walk %d/%d hi %.2f%s; at %.2f %zu segments, the nearest %.2f off its middle\n", q.x, q.z, q.r, q.y0, q.y1, D.walkCov, D.walkN, D.hiCov, f ? " GHOST" : "", h, segs.size(), best);
            }
        }
        printf("%d ghosts at the feet\n", ghosts);
        return 0;
    }
    if (what == "at" && argc > 8) {   // `walls at <sx> <sy> <sz> <body> <latDeg> <lonDeg> [radius]`: the pieces near a point, their discs and the drawn slices
        Star st; StarSystem sys; if (!systemAt(atoll(argv[3]), atoll(argv[4]), atoll(argv[5]), st, sys, true)) { printf("no star\n"); return 1; }
        const int bi = atoi(argv[6]); if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
        const BodyGen g = BodyGen::make(sys.bodies[bi]); if (worldHadCivilisation(g)) worldWorksOf(g);
        const double lat = atof(argv[7]) * DEG, lon = atof(argv[8]) * DEG, rad = argc > 9 ? atof(argv[9]) : 4.0;
        SurfaceView sv; sv.init(&sys, bi, lat, lon, 0.0);
        { Input in0; sv.update(1.0 / 30, in0, 0.0, false); }
        double tx, tz; sv.site.localAt(StarSystem::bodyFromLatLon(lat, lon), tx, tz);
        sv.player.x = tx; sv.player.z = tz; { Input in0; sv.update(1.0 / 30, in0, 0.0, false); }
        sv.forNearbyRuins(tx, tz, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
            for (const Form& e : cell.elems[0]) {
                const double ex = r.x + e.x, ez = r.z + e.z, d = std::hypot(ex - tx, ez - tz);
                if (d > formReach(e) + rad) continue;
                std::vector<FormDisc> q; formColliders(e, q);
                int near = 0; for (const FormDisc& f : q) if (std::hypot(r.x + f.x - tx, r.z + f.z - tz) < f.r + rad) near++;
                const Building* B = e.building >= 0 && e.building < (int)cell.spec.buildings.size() ? &cell.spec.buildings[e.building] : nullptr;
                printf("  %s %d part %d of %s %d (door %d, style %d): at %.2f %.2f, heading %.2f, hx %.2f hz %.2f, y %.2f..%.2f, tilt %.2f, broken %.2f: %zu discs, %d within %.1f m\n",
                       FORM_KIND_NAMES[e.kind], e.sub, e.part, B ? BUILDING_KIND_NAMES[B->kind] : "-", e.building, B ? B->door : -9, B ? B->style : -9, ex - tx, ez - tz, e.heading, e.hx, e.hz, e.y0, e.y1, e.tilt, e.broken, q.size(), near, rad);
            }
            for (const ShardSite& s2 : cell.shards) if (std::hypot(r.x + s2.x - tx, r.z + s2.z - tz) < rad + 2) printf("  a record (building %d, %s) at %.2f %.2f\n", s2.building, buildingPlaceName(s2.place), r.x + s2.x - tx, r.z + s2.z - tz);
        });
        std::vector<SurfaceView::Collider> cols; sv.collectColliders(tx, tz, cols);
        for (const auto& c : cols) if (c.kind != 3 && std::hypot(c.x - tx, c.z - tz) < c.r + rad) printf("  collider kind %d r %.2f at %.2f %.2f\n", c.kind, c.r, c.x - tx, c.z - tz);
        const SurfaceView::RuinCell* nr[4]; const int nn = sv.settlementsNear(std::floor(tx / 16) * 16 + 8, std::floor(tz / 16) * 16 + 8, nr);
        printf("  %d settlements near the 16 m cell:", nn); for (int i = 0; i < nn; i++) printf(" [%s at %.0f %.0f, r %.0f]", SETTLEMENT_CLASS_NAMES[nr[i]->spec.sclass], nr[i]->r.x - tx, nr[i]->r.z - tz, nr[i]->r.size); printf("\n");
        sv.forNearbyRuins(tx, tz, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int ring) { printf("  cell %s (work %d) at %.0f %.0f, size %.0f, ring %d, %zu buildings\n", SETTLEMENT_CLASS_NAMES[cell.spec.sclass], cell.spec.work, r.x - tx, r.z - tz, r.size, ring, cell.spec.buildings.size()); });
        return 0;
    }
    if (what == "place" && argc > 8) {   // `walls place <sx> <sy> <sz> <body> <latDeg> <lonDeg>`: the works round a point, their records and what lies at each
        Star st; StarSystem sys; if (!systemAt(atoll(argv[3]), atoll(argv[4]), atoll(argv[5]), st, sys, true)) { printf("no star\n"); return 1; }
        const int bi = atoi(argv[6]); if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
        const BodyGen g = BodyGen::make(sys.bodies[bi]);
        const double lat = atof(argv[7]) * DEG, lon = atof(argv[8]) * DEG;
        std::vector<std::shared_ptr<const Work>> W; worksAround(g, StarSystem::bodyFromLatLon(lat, lon), 300, W);
        for (const auto& w : W) {
            RuinSpec sp; workSpec(g, *w, sp);
            const Culture cu = cultureOf(g, w->people);
            std::vector<ShardSite> sites; recordSitesOf(g, sp, cu, sites);
            std::vector<Form> el; ruinElements(sp, cu, 0, el);
            printf("%s at %.5f %.5f: %zu buildings, %zu pieces, %zu records\n", WORK_KIND_NAMES[w->kind], w->lat / DEG, w->lon / DEG, sp.buildings.size(), el.size(), sites.size());
            for (const ShardSite& s : sites) {
                printf("  a record of building %d (%s) at %.2f %.2f\n", s.building, BUILDING_KIND_NAMES[sp.buildings[s.building].kind], s.x, s.z);
                for (const Form& e : el) {
                    if (!formWalkable(e) || std::hypot(e.x - s.x, e.z - s.z) > formReach(e) + 1) continue;
                    FormMesh m; formMesh(e, 0, Vec3(e.x, 0, e.z), 1.0, m, 1e9);
                    std::vector<WSeg> segs; std::vector<WPt> pts; wallSlice(m, 0.02, segs, pts);
                    printf("    under %s %d of building %d at %.2f %.2f (reach %.1f): support %.2f, in its slice at 0.02 %d (%zu segments)\n", FORM_KIND_NAMES[e.kind], e.sub, e.building, e.x, e.z, formReach(e),
                           formSupportAt(m, s.x, s.z), (int)sliceNear(segs, pts, s.x, s.z, 0.3), segs.size());
                }
            }
        }
        return 0;
    }
    if (what == "reach" && argc > 8) {   // `walls reach <sx> <sy> <sz> <body> <latDeg> <lonDeg> [half]`: the walk to a point from round it (the grid as a picture)
        Star st; StarSystem sys; if (!systemAt(atoll(argv[3]), atoll(argv[4]), atoll(argv[5]), st, sys, true)) { printf("no star\n"); return 1; }
        const int bi = atoi(argv[6]); if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
        const BodyGen g = BodyGen::make(sys.bodies[bi]); if (worldHadCivilisation(g)) worldWorksOf(g);
        const double lat = atof(argv[7]) * DEG, lon = atof(argv[8]) * DEG, half = argc > 9 ? atof(argv[9]) : 30.0;
        SurfaceView sv; sv.init(&sys, bi, lat, lon, 0.0);
        double tx, tz; sv.site.localAt(StarSystem::bodyFromLatLon(lat, lon), tx, tz);
        sv.player.x = tx + 2.5; sv.player.z = tz; { Input in0; sv.update(1.0 / 30, in0, 0.0, false); sv.update(1.0 / 30, in0, 0.0, false); }
        setenv("WALLS_DUMP", "1", 1);
        sv.collectSupports(tx, tz, half + 2);
        printf("%zu supports; along +x from it:\n", sv.supports.size());
        for (double d = 0; d <= half; d += 1.0) { const double x = tx + d, gr = sv.site.groundHeight(x, tz); bool bl = false; const double f = sv.floorAt(x, tz, gr + 1.0, &bl); printf("  %4.0f m: ground %.2f floor %.2f (top %.2f)%s\n", d, gr, f, sv.floorAt(x, tz, 1e9), bl ? " BLOCKED" : ""); }
        double gi; const bool ok = reachableOnFoot(sv, tx, tz, 2.0, gi, half);
        printf("%s (shots/tests/walls_reach_0.png when not)\n", ok ? "reached" : "NOT reached");
        return 0;
    }
    if (what == "walk" && argc > 9) {   // `walls walk <sx> <sy> <sz> <body> <latDeg> <lonDeg> <headingDeg> [initLat initLon]`: one walk from a point, frame
                                         // by frame (the view set up at the second point, as `walls view` sets it up at the place's middle)
        Star st; StarSystem sys; if (!systemAt(atoll(argv[3]), atoll(argv[4]), atoll(argv[5]), st, sys, true)) { printf("no star\n"); return 1; }
        const int bi = atoi(argv[6]); if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
        const BodyGen g = BodyGen::make(sys.bodies[bi]); if (worldHadCivilisation(g)) worldWorksOf(g);
        const double lat = atof(argv[7]) * DEG, lon = atof(argv[8]) * DEG, heading = atof(argv[9]) * DEG;
        const double ilat = argc > 11 ? atof(argv[10]) * DEG : lat, ilon = argc > 11 ? atof(argv[11]) * DEG : lon;
        SurfaceView sv; sv.init(&sys, bi, ilat, ilon, 0.0);
        { Input in0; sv.update(1.0 / 30, in0, 0.0, false); }
        double tx, tz; sv.site.localAt(StarSystem::bodyFromLatLon(lat, lon), tx, tz);
        const double hx = std::sin(heading), hz = std::cos(heading);
        sv.player.x = tx; sv.player.z = tz; sv.player.y = sv.site.groundHeight(sv.player.x, sv.player.z); sv.player.yaw = heading;
        sv.player.vx = sv.player.vz = sv.player.vy = 0; sv.player.onGround = true;
        Input in; in.down[KEY_W] = true;
        std::vector<SurfaceView::Collider> cols;
        for (int k = 0; k < 150; k++) {
            sv.player.yaw = heading;
            sv.update(1.0 / 30, in, 0.0, true); in.newFrame();
            const double gy = sv.site.groundHeight(sv.player.x, sv.player.z);
            sv.collectSupports(sv.player.x, sv.player.z);
            bool here = false, ahead = false; const double f = sv.floorAt(sv.player.x, sv.player.z, sv.player.y, &here); sv.floorAt(sv.player.x + hx * 0.5, sv.player.z + hz * 0.5, sv.player.y, &ahead);
            if (k % 5 && k < 140) { /* every fifth frame */ }
            printf("%3d at %.2f %.2f (%.2f m on): feet %.2f ground %.2f floor %.2f%s%s, onGround %d, v %.2f %.2f, %zu supports\n", k, sv.player.x - tx, sv.player.z - tz, (sv.player.x - tx) * hx + (sv.player.z - tz) * hz, sv.player.y, gy, f, here ? " INSIDE" : "", ahead ? " BLOCKED AHEAD" : "", (int)sv.player.onGround, sv.player.vx, sv.player.vz, sv.supports.size());
            sv.collectColliders(sv.player.x, sv.player.z, cols);
            for (const auto& c : cols) if (std::hypot(c.x - sv.player.x, c.z - sv.player.z) < c.r + 0.6) printf("      collider kind %d r %.2f at %.2f %.2f y %.2f..%.2f (feet-relative)\n", c.kind, c.r, c.x - sv.player.x, c.z - sv.player.z, c.y0 - sv.player.y, c.y1 - sv.player.y);
        }
        printf("what stops it: '%s'\n", whatStops(sv, sv.player.x, sv.player.z, hx, hz, sv.player.y).c_str());
        sv.forNearbyRuins(sv.player.x, sv.player.z, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
            for (const Form& e : cell.elems[0]) {
                const double ex = r.x + e.x, ez = r.z + e.z, d = std::hypot(ex - sv.player.x, ez - sv.player.z);
                if (d > formReach(e) + 3) continue;
                double gy = 0, skirt = 0; const bool drawn = e.absolute || sv.pieceFootNear(e, ex, ez, gy, skirt);
                std::vector<FormDisc> q; formColliders(e, q);
                printf("  form %s %d part %d in %s (%s): at %.2f %.2f (%.1f m), hx %.2f hz %.2f y %.2f..%.2f, tilt %.2f broken %.2f, walkable %d, foot %.2f (feet %.2f) skirt %.2f, %s, %zu discs\n",
                       FORM_KIND_NAMES[e.kind], e.sub, e.part, e.building >= 0 && e.building < (int)cell.spec.buildings.size() ? BUILDING_KIND_NAMES[cell.spec.buildings[e.building].kind] : "-", SETTLEMENT_CLASS_NAMES[cell.spec.sclass],
                       ex - sv.player.x, ez - sv.player.z, d, e.hx, e.hz, e.y0, e.y1, e.tilt, e.broken, (int)formWalkable(e), gy, sv.player.y, skirt, drawn ? "drawn" : "NOT DRAWN", q.size());
                printf("      FORM %d %d %.4f %.4f %.6f %.4f %.4f %.4f %.4f %.6f %.6f %.6f %.6f %d %d %.6f %.6f %u %d\n", e.kind, e.sub, e.x, e.z, e.heading, e.hx, e.hz, e.y0, e.y1, e.p[0], e.p[1], e.p[2], e.p[3], e.sides, e.part, e.broken, e.tilt, e.seed, (int)e.absolute);
            }
        });
        return 0;
    }
    printf("walls forms [peoples] [years ...] | walls view [worlds] [walks] | walls view near <peoples> [walks] | walls walk <sx> <sy> <sz> <body> <lat> <lon> <heading> [back]\n");
    return 1;
}

// B-420, B-421: the colliders over what is drawn (every piece of three age worlds' places and works), the explorer walked through the scene
// worlds' places stopped only by what is drawn, the boulders that block those drawn, a barrow or a bank stood on and walked over, the records
// and the instruments where the explorer finds them (none in water, in or under a piece, out of reach)
void wallsChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    const double t0 = nowSec();
    WallTally T;
    for (int a : {AGE_PREHISTORIC, AGE_INDUSTRIAL, AGE_MODERN}) {
        Star st; StarSystem sys; if (!systemAt(AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, st, sys, true)) continue;
        const BodyGen g = BodyGen::make(sys.bodies[AGE_WORLDS[a].body]);
        if (!worldHadCivilisation(g)) continue;
        const int p = std::min(AGE_WORLDS[a].which, std::max(1, peoplesOf(g)) - 1);
        const Culture cu = cultureOf(g, p);
        const Lore L = loreOf(sys, sys.bodies[AGE_WORLDS[a].body], g, p);
        std::vector<WallPlace> places; wallPlacesOf(g, sys.bodies[AGE_WORLDS[a].body].name, p, places, 4, 120);
        for (const WallPlace& wp : places) {
            std::vector<Form> el; ruinElements(wp.sp, cu, 0, el);
            std::vector<Grave> gr; gravesOf(L, wp.sp, cu, gr); graveElements(gr, cu, 0, el, wp.sp.work >= 0);
            wallsOfPieces(el, &wp.sp, wp.what, T);
        }
    }
    std::string worst; for (const auto& kv : T.byKey) if (kv.second.feet > 0 && worst.size() < 300) worst += fmt("; %s %ld", kv.first.c_str(), kv.second.feet);
    check("walls: every collider over what is drawn (B-420)", T.discs > 200000 && T.feet * 20000 <= T.discs,   // KI-398: a few of the rare forms' approximations
          fmt("%ld places and works of three age worlds, %ld pieces, %ld discs: %ld at the feet over nothing drawn (at most one in 20,000), %ld above them mostly over nothing (the jetpack's: a snapped tower's holes)%s (%.1f s)",
              T.places, T.pieces, T.discs, T.feet, T.above, worst.c_str(), nowSec() - t0));
    // the scene worlds (their peoples ended lately: most of their works stand): walks, records, a heap stood on
    WalkTally W; RecTally R; int climbed = 0, tried = 0; double bestRise = 0; std::string climbWhat;
    for (int a : {AGE_PREHISTORIC, AGE_MEDIEVAL, AGE_MODERN}) {
        Star st; StarSystem sys; if (!systemAt(AGE_SCENE_WORLDS[a].sx, 0, AGE_SCENE_WORLDS[a].sz, st, sys, true)) continue;
        const int bi = AGE_SCENE_WORLDS[a].body;
        const BodyGen g = BodyGen::make(sys.bodies[bi]);
        if (!worldHadCivilisation(g)) continue;
        worldWorksOf(g);
        std::vector<WallPlace> places; wallPlacesOf(g, sys.bodies[bi].name, 0, places, 4, 60);
        std::vector<WallPlace> pick; std::set<int> kinds; int works = 0;
        for (const WallPlace& wp : places) { if (wp.work == 0) { pick.push_back(wp); continue; } if (works < 8 && kinds.insert(wp.sp.work).second) { pick.push_back(wp); works++; } }
        for (const WallPlace& wp : pick) {
            SurfaceView sv; sv.init(&sys, bi, wp.sp.lat, wp.sp.lon, 0.0);
            { Input in0; sv.update(1.0 / 30, in0, 0.0, false); }
            const SurfaceView::RuinCell* c = nullptr;
            if (wp.work == 0) c = sv.ruinCell(wp.gLat, wp.gLon);
            else for (const SurfaceView::WorkNear& wn : sv.workNear) if (wn.work->id == wp.work) { c = sv.workCell(*wn.work); break; }
            if (!c || !c->has) continue;
            const std::string what = wp.what + fmt(" [%lld 0 %lld %d]", (long long)AGE_SCENE_WORLDS[a].sx, (long long)AGE_SCENE_WORLDS[a].sz, bi);
            recordsOfCell(sv, c->r, *c, what, R);
            walkPlace(sv, c->r.x, c->r.z, std::min(wp.sp.size, 400.0), wp.sp.sclass == SC_CAPITAL ? 10 : 4, 6.0, wp.sp.seed ^ 0xB420ULL, what, W);
            if (climbed == 0 && tried < 6)   // a heap, a bank or a barrow of it walked over through its middle
                for (const Form& e : c->elems[0]) {
                    if (!formWalkable(e) || e.kind == FK_SWEEP || tried >= 6) continue;
                    tried++;
                    const double ex = c->r.x + e.x, ez = c->r.z + e.z, reach = formReach(e), hd = 0.7;
                    const double hx = std::sin(hd), hz = std::cos(hd);
                    sv.player.x = ex - hx * (reach + 3); sv.player.z = ez - hz * (reach + 3); sv.player.y = sv.site.groundHeight(sv.player.x, sv.player.z);
                    sv.player.vx = sv.player.vz = sv.player.vy = 0; sv.player.onGround = true; sv.player.yaw = hd;
                    Input in; in.down[KEY_W] = true; double rise = 0;
                    for (int k = 0; k < (int)((2 * reach + 6) / 4.2 * 30) + 30; k++) { sv.player.yaw = hd; sv.update(1.0 / 30, in, 0.0, true); in.newFrame(); rise = std::max(rise, sv.player.y - sv.site.groundHeight(sv.player.x, sv.player.z)); }
                    const double past = (sv.player.x - ex) * hx + (sv.player.z - ez) * hz;
                    bestRise = std::max(bestRise, rise);
                    if (rise > 0.6 * (formTop(e) - e.y0) && past > reach) { climbed++; climbWhat = fmt("a %s of %s %.1f m high, %.0f m across: up %.1f m and over", FORM_KIND_NAMES[e.kind], what.c_str(), formTop(e), 2 * reach, rise); }
                }
        }
    }
    std::string causes; for (const auto& kv : W.causes) causes += fmt("%s%s %d", causes.empty() ? "" : ", ", kv.first.c_str(), kv.second);
    check("walls: walked through the places, stopped by what is drawn alone (B-420)", W.walks >= 150 && W.ghosts == 0 && W.rocks > 20 && W.rocksOnBeds == 0,
          fmt("%d walks (%.0f m) through the scene worlds' places and works, %d stopped (%s), %d by nothing drawn%s%s; %d boulders that block round their starts, %d on a road's, a street's or a work's bed",
              W.walks, W.metres, W.stuck, causes.c_str(), W.ghosts, W.ex.empty() ? "" : ": ", W.ex.empty() ? "" : W.ex[0].c_str(), W.rocks, W.rocksOnBeds));
    check("walls: a heap, a bank or a barrow stood on and walked over (B-420)", climbed > 0, climbed ? climbWhat : fmt("%d tried, the best %.1f m up", tried, bestRise));
    check("records: none in water, in or under a piece, out of reach (B-421)", R.records > 100 && R.instruments > 0 && R.water + R.inside + R.heap + R.hidden + R.unreachable + R.through + R.overlap == 0,
          fmt("%d records and %d instruments in the scene worlds' places and works: %d in water, %d in a piece or a boulder, %d under a heap, %d hidden in a drawn piece, %d out of reach, %d instruments through a wall, %d over a record%s%s (%.1f s)",
              R.records, R.instruments, R.water, R.inside, R.heap, R.hidden, R.unreachable, R.through, R.overlap, R.ex.empty() ? "" : ": ", R.ex.empty() ? "" : R.ex[0].c_str(), nowSec() - t0));
}
