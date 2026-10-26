// E-09: the world's works, a finder for each kind (`WB`'s members): the mines, the dams, the aqueducts, the observatories,
// the shrine and its way, the lines on a plain, the fortified line, the battlefields, the launch sites, the antennas, the
// colossi, the terraformed basins, the last shelter and a sunless people's works, the inexplicable, the lighthouses
#include "works_internal.h"

// the highest point between r0 and r1 of a point, refined to its summit
bool WB::summitNear(const Vec3& u0, double r0, double r1, Rng& rng, Vec3& best, double& hBest) {
    hBest = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    const int rings = 6;
    for (int ri = 0; ri < rings; ri++) {
        const double d = r0 + (r1 - r0) * (ri + 0.5) / rings;
        const int n = 12 + 4 * ri;
        for (int k = 0; k < n; k++) {
            const double a = a0 + k * TAU / n;
            const Vec3 u = offsetUnit(u0, std::sin(a) * d, std::cos(a) * d, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || s.height <= hBest) continue;
            hBest = s.height; best = u; any = true;
        }
    }
    if (!any) return false;
    for (double step : {300.0, 100.0, 35.0}) {   // up the slope to its top
        for (int it = 0; it < 6; it++) {
            Vec3 nb = best; double hb = hBest;
            for (int k = 0; k < 8; k++) {
                const double a = k * TAU / 8;
                const Vec3 u = offsetUnit(best, std::sin(a) * step, std::cos(a) * step, Rm);
                const SurfaceSample s = at(u);
                if (!wetS(s) && s.height > hb) { hb = s.height; nb = u; }
            }
            if (hb <= hBest) break;
            hBest = hb; best = nb;
        }
    }
    return true;
}

// the shoreline nearest a point within r0..r1 (a dry probe beside a wet one, bisected): its point and the heading toward the water; with
// `headland`, the shore point with the most water round it (a headland's)
bool WB::shoreNear(const Vec3& u0, double r0, double r1, Rng& rng, Vec3& shore, double& seaward, bool headland) {
    double bestScore = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (int ri = 0; ri < 6; ri++) {
        const double d = r0 + (r1 - r0) * ri / 5.0;
        for (int k = 0; k < 24; k++) {
            const double a = a0 + k * TAU / 24;
            const Vec3 dry = offsetUnit(u0, std::sin(a) * d, std::cos(a) * d, Rm);
            if (wetS(at(dry))) continue;
            for (int q = 0; q < 8; q++) {   // water within 300 m of it
                const double b = q * TAU / 8;
                const Vec3 wv = offsetUnit(dry, std::sin(b) * 300, std::cos(b) * 300, Rm);
                const SurfaceSample ws = at(wv);
                if (!wetS(ws) || ws.oldSea > 0.5) continue;
                double lo = 0, hi = 300;   // bisect the edge
                for (int it = 0; it < 7; it++) { const double mid = 0.5 * (lo + hi); if (wetS(at(offsetUnit(dry, std::sin(b) * mid, std::cos(b) * mid, Rm)))) hi = mid; else lo = mid; }
                const Vec3 pt = offsetUnit(dry, std::sin(b) * lo, std::cos(b) * lo, Rm);
                double score = -d / r1;
                if (headland) { int wn = 0; for (int m = 0; m < 12; m++) { const double c = m * TAU / 12; if (wetS(at(offsetUnit(pt, std::sin(c) * 400, std::cos(c) * 400, Rm)))) wn++; } score += wn * 0.3; }
                if (score > bestScore) { bestScore = score; shore = pt; seaward = bearingOf(pt, wv); any = true; }
                break;
            }
            if (any && !headland && ri > 0) return true;
        }
    }
    return any;
}

// ---- the mines the history sank (B-418: the land's where the plan read coal and ore, `landMine`) ---------------------------------------
void WB::mineAt(Work& w, int style, double h, double down, double pitR, Rng& rng) {   // a mine's style and its open pit
    w.style = style; w.heading = down + PI;
    if (style == 2) {
        const int nb = 5 + (int)(rng.uni() * 5);
        GroundEdit e; e.kind = GE_PIT; e.r = pitR; e.depth = 13 + 3 * rng.uni(); e.step = std::min(24.0, pitR * 0.55 / nb); e.level = h - nb * e.depth; e.hw = pitR - nb * e.step; e.fades = false;
        w.edits.push_back(e); w.floor = e.level; w.length = 2 * pitR;
    }
}
void WB::mines() {
    if (age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC) return;
    Rng rng(pseed ^ 0x70320ULL);
    struct Src { Vec3 u; double r; int a; double before; int hp, ev, year; };
    std::vector<Src> src;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_MINE || e.place < 0 || src.size() >= 6 || gone(e.place)) return;
        Src s; placeAt(e.place, s.u, s.r); s.a = age; int pa; placeAge(e.place, pa, s.before); s.hp = e.place; s.ev = i; s.year = e.year;
        src.push_back(s);
    });
    int k = 0;
    for (const Src& s : src) {
        const int a = clampi(s.a, AGE_MEDIEVAL, AGE_MODERN);
        const int style = a == AGE_MEDIEVAL ? 0 : (a == AGE_INDUSTRIAL ? 1 : 2);
        Vec3 best; double bestS = -1e18, bestDown = 0, bestH = 0; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {1500.0, 3000.0, 5000.0, 8000.0})
            for (int q = 0; q < 16; q++) {
                const double ang = a0 + q * TAU / 16, dd = s.r + d;
                const Vec3 u = offsetUnit(s.u, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                const SurfaceSample sm = at(u);
                if (wetS(sm) || matFamily(sm.material) == FAM_SNOW) continue;
                double down, h; const double sl = grade(u, 45, down, h);
                double sc = P.grid.res[P.grid.cellOf(u)] - dd / 20000;
                if (style == 0) { if (sl < 0.15) continue; sc += std::min(sl, 0.5); }
                else { if (sl > 0.14) continue; sc -= sl; }
                if (matFamily(sm.material) == FAM_ROCK) sc += 0.1;
                if (sc > bestS && clearOf(u, style == 2 ? 520 : 260)) { bestS = sc; best = u; bestDown = down; bestH = h; any = true; }
            }
        if (!any) { k++; continue; }
        const double pitR = 220 + 200 * rng.uni();
        Work& w = add(WO_MINE, best, style == 2 ? pitR + 120 : 240, seedOf(WO_MINE, k++));
        w.age = a; w.before = s.before; w.place = s.hp; w.event = s.ev; w.year = s.year;
        if (s.hp >= 0 && H->places[s.hp].plan >= 0) w.planRank = H->places[s.hp].plan;
        mineAt(w, style, bestH, bestDown, pitR, rng);
    }
}

// ---- the dams: at a valley's narrows near the place the history built one, holding the reservoir behind ----------------------------
// the valleys round the place (a grid of 300 m over 18 km, the flow down its steepest neighbours and gathered); the narrowest cross-section
// at the age's height among the larger valleys; then the reservoir: the water at a level filled from behind the wall over a grid of 50 m,
// as high as it stays behind the wall and inside the grid, the crest a few metres over it
bool WB::damNear(const Vec3& u0, double R0, int a, uint64_t seed, Work& out) {
    Rng rng(seed);
    const double Ht = a == AGE_INDUSTRIAL ? 18 + 16 * rng.uni() : (a == AGE_MODERN ? 45 + 60 * rng.uni() : 90 + 80 * rng.uni());
    const double Hmin = a == AGE_INDUSTRIAL ? 8 : (a == AGE_MODERN ? 18 : 35), maxHalf = a == AGE_INDUSTRIAL ? 700 : (a == AGE_MODERN ? 1100 : 1500);
    const int N = 61; const double cs = 300;
    const Frame F(u0, Rm);
    std::vector<float> h(N * N); std::vector<uint8_t> wt(N * N);
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) { const SurfaceSample s = at(F.unit((i - N / 2) * cs, (j - N / 2) * cs)); h[j * N + i] = (float)s.height; wt[j * N + i] = wetS(s); }
    std::vector<int> down(N * N, -1);
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        const int c = j * N + i; if (wt[c]) continue;
        double best = 0;
        for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
            if (!di && !dj) continue;
            const int ii = i + di, jj = j + dj; if (ii < 0 || jj < 0 || ii >= N || jj >= N) continue;
            const double drop = (h[c] - h[jj * N + ii]) / (cs * std::sqrt((double)(di * di + dj * dj)));
            if (drop > best) { best = drop; down[c] = jj * N + ii; }
        }
    }
    std::vector<int> order(N * N); std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int x, int y) { return h[x] > h[y]; });
    std::vector<float> acc(N * N, 1.0f);
    for (int c : order) if (down[c] >= 0) acc[down[c]] += acc[c];
    // the closed basins filled to their spill (a priority flood from the grid's edge and the water): the drainage the surface draws fills them
    // with a lake or a playa's flat, over a dam's floor (galaxy/drainage.cpp)
    std::vector<float> filled(N * N, 1e30f);
    {
        using QE = std::pair<float, int>;
        std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
        for (int c = 0; c < N * N; c++) { const int i = c % N, j = c / N; if (wt[c] || i == 0 || j == 0 || i == N - 1 || j == N - 1) { filled[c] = h[c]; pq.push({h[c], c}); } }
        while (!pq.empty()) {
            const QE q = pq.top(); pq.pop();
            if (q.first > filled[q.second]) continue;
            const int i = q.second % N, j = q.second / N;
            for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
                const int ii = i + di, jj = j + dj; if (ii < 0 || jj < 0 || ii >= N || jj >= N) continue;
                const int nb = jj * N + ii; const float f = std::max(h[nb], q.first);
                if (f < filled[nb]) { filled[nb] = f; pq.push({f, nb}); }
            }
        }
    }
    struct Cand { int c; double score, floor, tL, tR, ex, ez, fx, fz, crest; };
    std::vector<int> cand;   // every valley cell in reach
    for (int c = 0; c < N * N; c++) {
        if (wt[c] || down[c] < 0 || wt[down[c]] || acc[c] < 12 || acc[c] > 3000) continue;
        if (filled[c] > h[c] + 2 || filled[down[c]] > h[down[c]] + 2) continue;   // in a basin the drainage fills
        const double ex = (c % N - N / 2) * cs, ez = (c / N - N / 2) * cs, d = std::hypot(ex, ez);
        if (d < R0 + 1200 || d > 8500) continue;
        cand.push_back(c);
    }
    // each one's cross-section on the coarse grid first (bilinear, free), then the best few's with samples every 25 m
    auto hc = [&](double x, double z) {
        const double fi = clampd(x / cs + N / 2, 0, N - 1.001), fj = clampd(z / cs + N / 2, 0, N - 1.001);
        const int i0 = (int)fi, j0 = (int)fj; const double tx = fi - i0, tz = fj - j0;
        return (h[j0 * N + i0] * (1 - tx) + h[j0 * N + i0 + 1] * tx) * (1 - tz) + (h[(j0 + 1) * N + i0] * (1 - tx) + h[(j0 + 1) * N + i0 + 1] * tx) * tz;
    };
    auto section = [&](double ex, double ez, double axx, double axz, double step, const std::function<double(double, double)>& height, Cand& cd) -> bool {
        const int half = (int)(maxHalf / step), M = 2 * half + 1;
        std::vector<double> prof(M);
        for (int q = 0; q < M; q++) { const double t = (q - half) * step; prof[q] = height(ex + axx * t, ez + axz * t); }
        const int w0 = std::max(1, (int)std::lround(150 / step));
        int q0 = half; for (int q = half - w0; q <= half + w0; q++) if (prof[q] < prof[q0]) q0 = q;
        double hL = -1e18, hR = -1e18;   // the crest: the lower of the two shoulders within reach, no higher than the age built
        for (int q = 0; q <= q0; q++) hL = std::max(hL, prof[q]);
        for (int q = q0; q < M; q++) hR = std::max(hR, prof[q]);
        const double floor = prof[q0], crest = std::min(std::min(hL, hR) - 1, floor + Ht);
        if (crest - floor < Hmin) return false;
        int qa = q0, qb = q0;
        while (qa > 0 && prof[qa] < crest) qa--;
        while (qb < M - 1 && prof[qb] < crest) qb++;
        if (prof[qa] < crest || prof[qb] < crest) return false;
        cd.floor = floor; cd.crest = crest; cd.tL = (qa - q0) * step; cd.tR = (qb - q0) * step;
        cd.ex = ex + axx * (q0 - half) * step; cd.ez = ez + axz * (q0 - half) * step;
        cd.score = (crest - floor) / (cd.tR - cd.tL) + 0.004 * (crest - floor);
        return true;
    };
    std::vector<Cand> coarse;
    for (int c : cand) {
        const double ex = (c % N - N / 2) * cs, ez = (c / N - N / 2) * cs;
        const double dx = (down[c] % N - c % N) * cs, dz = (down[c] / N - c / N) * cs, dl = std::hypot(dx, dz);
        Cand cd; cd.c = c; cd.fx = dx / dl; cd.fz = dz / dl;
        if (!section(ex, ez, -cd.fz, cd.fx, 100, hc, cd)) continue;
        cd.score += 0.0002 * std::min(acc[c], 600.0f);
        coarse.push_back(cd);
    }
    std::sort(coarse.begin(), coarse.end(), [](const Cand& x, const Cand& y) { return x.score > y.score; });
    std::vector<Cand> good;
    for (int k = 0; k < (int)coarse.size() && k < 6; k++) {
        Cand cd = coarse[k];
        const double ex = (cd.c % N - N / 2) * cs, ez = (cd.c / N - N / 2) * cs;
        if (section(ex, ez, -cd.fz, cd.fx, 25, [&](double x, double z) { return at(F.unit(x, z)).height; }, cd)) good.push_back(cd);
    }
    std::sort(good.begin(), good.end(), [](const Cand& x, const Cand& y) { return x.score > y.score; });
    static const bool dbg = std::getenv("WORKS_DEBUG") != nullptr;
    if (dbg) { int big = 0; double hmin = 1e18, hmax = -1e18; for (int c = 0; c < N * N; c++) { if (acc[c] >= 25) big++; if (!wt[c]) { hmin = std::min(hmin, (double)h[c]); hmax = std::max(hmax, (double)h[c]); } } fprintf(stderr, "dam near: Ht %.0f, %d cells of acc >= 25, %zu candidates, %zu with walls; the grid's heights %.0f..%.0f\n", Ht, big, cand.size(), good.size(), hmin, hmax); }
    for (int gi = 0; gi < (int)good.size() && gi < 3; gi++) {
        const Cand& cd = good[gi];
        const Vec3 du = F.unit(cd.ex, cd.ez);   // the floor's point under the dam
        const Frame D(du, Rm);
        const double axx = -cd.fz, axz = cd.fx;
        // the reservoir's grid: 100 cells of 50 m, most of it upstream
        const int M = 100; const double fc = 50;
        const double gx0 = -cd.fx * 2000 - M * fc * 0.5, gz0 = -cd.fz * 2000 - M * fc * 0.5;
        std::vector<float> hf(M * M);
        for (int j = 0; j < M; j++) for (int i = 0; i < M; i++) hf[j * M + i] = (float)at(D.unit(gx0 + (i + 0.5) * fc, gz0 + (j + 0.5) * fc)).height;
        // the wall's line (beyond its abutments a little) as a barrier of cells
        std::vector<uint8_t> bar(M * M, 0);
        const double ax0 = axx * (cd.tL - 120), az0 = axz * (cd.tL - 120), ax1 = axx * (cd.tR + 120), az1 = axz * (cd.tR + 120);
        for (int j = 0; j < M; j++) for (int i = 0; i < M; i++) {
            const double x = gx0 + (i + 0.5) * fc, z = gz0 + (j + 0.5) * fc;
            const double vx = ax1 - ax0, vz = az1 - az0, L2 = vx * vx + vz * vz, t = clampd(((x - ax0) * vx + (z - az0) * vz) / L2, 0, 1);
            if (std::hypot(x - (ax0 + vx * t), z - (az0 + vz * t)) < fc * 0.9) bar[j * M + i] = 1;
        }
        const int si = (int)std::floor((-cd.fx * 90 - gx0) / fc), sj = (int)std::floor((-cd.fz * 90 - gz0) / fc);
        if (si < 1 || sj < 1 || si >= M - 1 || sj >= M - 1 || bar[sj * M + si]) continue;
        std::vector<uint8_t> fill(M * M);
        std::vector<int> stack;
        auto flood = [&](double L) -> int {   // the cells filled, -1 a leak (the grid's edge, or round the wall to its downstream side)
            std::fill(fill.begin(), fill.end(), 0);
            if (hf[sj * M + si] >= L) return 0;
            stack.clear(); stack.push_back(sj * M + si); fill[sj * M + si] = 1; int n = 0;
            while (!stack.empty()) {
                const int c = stack.back(); stack.pop_back(); n++;
                const int i = c % M, j = c / M;
                if (i == 0 || j == 0 || i == M - 1 || j == M - 1) return -1;
                const double x = gx0 + (i + 0.5) * fc, z = gz0 + (j + 0.5) * fc;
                if (x * cd.fx + z * cd.fz > 40) return -1;
                const int nb[4] = {c - 1, c + 1, c - M, c + M};
                for (int q = 0; q < 4; q++) { const int d = nb[q]; if (!fill[d] && !bar[d] && hf[d] < L) { fill[d] = 1; stack.push_back(d); } }
            }
            return n;
        };
        const double fb = a == AGE_INDUSTRIAL ? 2 : 4;
        double lo = cd.floor + 2, hi = cd.crest - fb, L = -1e9;
        if (flood(lo) < 0) { if (dbg) fprintf(stderr, "  cand %d: leaks at the floor + 2\n", gi); continue; }
        for (int it = 0; it < 14; it++) { const double mid = 0.5 * (lo + hi); if (flood(mid) >= 0) { L = mid; lo = mid; } else hi = mid; }
        if (L < -1e8) L = cd.floor + 2;
        const int cellsN = flood(L);
        if (dbg) fprintf(stderr, "  cand %d: floor %.0f, level %.1f (%.0f over), %d cells\n", gi, cd.floor, L, L - cd.floor, cellsN);
        if (cellsN < 30 || L - cd.floor + fb < Hmin) continue;
        const double crest = L + fb;
        // the abutments at the crest found: the cross-section again
        double tL = 0, tR = 0;
        for (int s = -1; s <= 1; s += 2) {
            double t = 0; bool hit = false;
            for (int q = 1; q <= (int)(maxHalf * 1.4 / 10); q++) { t = s * q * 10.0; if (at(D.unit(axx * t, axz * t)).height >= crest) { hit = true; break; } }
            if (!hit) { tL = tR = 0; break; }
            (s < 0 ? tL : tR) = t;
        }
        if (tR - tL < 20) continue;
        const double mx = axx * 0.5 * (tL + tR), mz = axz * 0.5 * (tL + tR);
        const Vec3 wu = D.unit(mx, mz);
        out.unit = wu; StarSystem::latLonFromBody(wu, out.lat, out.lon);
        out.floor = cd.floor; out.crest = crest; out.level = L; out.length = tR - tL; out.height = crest - cd.floor;
        out.heading = std::atan2(-cd.fx, -cd.fz);   // toward the reservoir
        out.axis = std::atan2(axx, axz);
        // the water's cells (filled at L, a cell more round them for the shore's cut), in the work's metres
        GroundEdit e; e.kind = GE_WATER; e.level = L; e.cell = fc; e.nx = M; e.nz = M; e.gx0 = gx0 - mx; e.gz0 = gz0 - mz; e.fades = false;
        e.mask.assign(M * M, 0);
        int area = 0;
        for (int j = 1; j < M - 1; j++) for (int i = 1; i < M - 1; i++) {
            const int c = j * M + i;
            if (fill[c]) { e.mask[c] = 1; area++; continue; }
            if (bar[c]) continue;
            const double x = gx0 + (i + 0.5) * fc, z = gz0 + (j + 0.5) * fc;
            if (x * cd.fx + z * cd.fz > -20) continue;
            if (fill[c - 1] || fill[c + 1] || fill[c - M] || fill[c + M]) e.mask[c] = 2;
        }
        out.width = area * fc * fc;   // the reservoir's area (m2)
        out.radiusM = std::max(out.length * 0.5 + 60, 0.0);
        double far = 0;
        for (int j = 0; j < M; j++) for (int i = 0; i < M; i++) if (fill[j * M + i]) far = std::max(far, std::hypot(gx0 + (i + 0.5) * fc - mx, gz0 + (j + 0.5) * fc - mz));
        out.radiusM = std::max(out.radiusM, far + 60);
        out.edits.push_back(e);
        return true;
    }
    return false;
}

void WB::dams() {
    if (!H || age < AGE_INDUSTRIAL || age == AGE_FAR_FUTURE) return;
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_DAM || e.place < 0 || made[WO_DAM] >= 5 || gone(e.place)) return;
        const int a = clampi(age, AGE_INDUSTRIAL, AGE_FUTURISTIC);
        Vec3 u; double r; placeAt(e.place, u, r);
        Work w; w.kind = WO_DAM;
        const uint64_t sd = seedOf(WO_DAM, k++);
        // by its town, else up its river into the hills (the plan's coarse rivers: the upstream cell that brings the most water, three times)
        bool found = damNear(u, r, a, sd, w);
        const PlanGrid& G = P.grid;
        int c = G.cellOf(u);
        const double h0 = G.height[c];
        for (int step = 0; step < 4 && !found; step++) {
            int nb[8]; const int nn = G.neighbours(c, nb); int up = -1; float bf = 0;
            for (int q = 0; q < nn; q++) if (G.down[nb[q]] == c && G.land[nb[q]] && G.river[nb[q]] > bf) { bf = G.river[nb[q]]; up = nb[q]; }
            if (up < 0) break;
            c = up;
            if (G.relief[c] < 0.12f && G.height[c] < h0 + 120) continue;   // still the plain
            found = damNear(G.unitOf(c), 0, a, sd ^ (uint64_t)(step + 1), w);
        }
        if (!found) return;
        if (!clearOf(w.unit, std::min(w.radiusM, w.length * 0.5 + 200))) return;
        Work& x = add(WO_DAM, w.unit, w.radiusM, sd);
        const Vec3 keep = x.unit; const uint64_t id = x.id; const double ground = x.ground;
        x = w; x.unit = keep; x.id = id; x.seed = sd; x.people = p; x.ground = ground;
        x.age = a; x.style = a == AGE_INDUSTRIAL ? 0 : (a == AGE_FUTURISTIC ? 3 : (x.length / std::max(1.0, x.height) < 4.5 ? 2 : 1));
        x.place = e.place; x.event = i; x.year = e.year; int pa; placeAge(e.place, pa, x.before);
        if (H->places[e.place].plan >= 0) x.planRank = H->places[e.place].plan;
        if (damBreached(x, C)) for (GroundEdit& ge : x.edits) if (ge.kind == GE_WATER) ge.tone = ET_DARK;   // breached: the reservoir drained, its bed left
    });
}

// ---- the aqueducts: from a spring on higher ground to the town, over the low ground on arches -----------------------------------------
void WB::aqueducts() {
    if (!H || age < AGE_MEDIEVAL || age > AGE_INDUSTRIAL) return;   // B-414: a medieval or an industrial people's (its arches in its age's way)
    Rng rng(pseed ^ 0x70340ULL);
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_AQUEDUCT || e.place < 0 || made[WO_AQUEDUCT] >= 3 || gone(e.place)) return;
        Vec3 u0; double r0; placeAt(e.place, u0, r0);
        const double h0 = at(u0).height;
        Vec3 src; double hs = -1e18, ds = 0; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {3000.0, 5000.0, 7500.0, 10000.0, 13000.0}) {
            for (int q = 0; q < 24; q++) {
                const double ang = a0 + q * TAU / 24;
                const Vec3 u = offsetUnit(u0, std::sin(ang) * d, std::cos(ang) * d, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s) || s.height < h0 + 25 + 0.002 * d) continue;
                if (!any || s.height > hs) { hs = s.height; src = u; ds = d; any = true; }
            }
            if (any) break;
        }
        if (!any) return;
        const Vec3 end = towardThrough(u0, src, r0 + 60, Rm);
        const double len = arcLength(src, end, Rm);
        if (len < 1500) return;
        const Vec3 mid = slerpUnit(src, end, 0.5);
        if (!clearOf(mid, 120, true)) return;
        const Frame F(mid, Rm);
        const double hEnd = at(end).height, c0 = hs - 3, c1 = std::max(hEnd + 5, c0 - 0.004 * len);
        Work& w = add(WO_AQUEDUCT, mid, len * 0.5 + 80, seedOf(WO_AQUEDUCT, k++));
        w.age = age; w.place = e.place; w.event = i; w.year = e.year; int pa; placeAge(e.place, pa, w.before);
        if (H->places[e.place].plan >= 0) w.planRank = H->places[e.place].plan;
        w.length = len; w.heading = bearingOf(src, end); w.level = c0; w.crest = c1;
        double sx, sz, ex, ez; F.local(src, sx, sz); F.local(end, ex, ez);
        w.sideA = src; w.sideB = end;
        // the conduit's line on the ground (its stretches the arches do not carry)
        GroundEdit line; line.kind = GE_LINE; line.width = 2.5; line.tone = ET_DARK;
        addPt(line, sx, sz); addPt(line, ex, ez);
        w.edits.push_back(line);
        (void)ds;
    });
}

// ---- the observatories: on the highest summit near their town -------------------------------------------------------------------
void WB::observatories() {
    if (age < AGE_MEDIEVAL || age == AGE_FAR_FUTURE) return;   // B-414: the far future's own sky is read by nothing a futuristic people built
    Rng rng(pseed ^ 0x70350ULL);
    std::vector<std::pair<int, int>> src;   // (history place, event)
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) { if (e.thing == WK_OBSERVATORY && e.place >= 0 && src.size() < 3 && !gone(e.place)) src.push_back({e.place, i}); });
    if (src.empty() && age >= AGE_FUTURISTIC && !pp->places.empty()) src.push_back({0, -1});   // a futuristic people's array by its first great place
    int k = 0;
    for (auto [hp, ev] : src) {
        Vec3 u0; double r0;
        if (H && hp < (int)H->places.size()) placeAt(hp, u0, r0); else { u0 = pp->places[0].unit; r0 = pp->places[0].radiusM; }
        Vec3 top; double hTop;
        if (!summitNear(u0, r0 + 1500, r0 + 24000, rng, top, hTop)) continue;
        if (!clearOf(top, 180)) continue;
        Work& w = add(WO_OBSERVATORY, top, 180, seedOf(WO_OBSERVATORY, k++));
        w.age = clampi(age, AGE_MEDIEVAL, AGE_FUTURISTIC);
        if (ev >= 0) { w.event = ev; w.year = H->events[ev].year; }
        w.place = hp; int pa; placeAge(hp, pa, w.before);
        if (H && hp < (int)H->places.size() && H->places[hp].plan >= 0) w.planRank = H->places[hp].plan;
        w.style = w.age - AGE_MEDIEVAL; w.height = hTop; w.heading = bearingOf(top, u0);
    }
}

// ---- the shrine at the holy place, and the pilgrims' way to it ---------------------------------------------------------------------
void WB::shrine() {
    if (!H || age >= AGE_FAR_FUTURE) return;
    const int holy = H->profile.holy;
    if (holy != HOLY_HIGH_PLACE && holy != HOLY_SPRING && holy != HOLY_CAVE && holy != HOLY_GROVE && holy != HOLY_OPEN_SKY) return;
    Rng rng(pseed ^ 0x70360ULL);
    int hp = 0, ev = -1;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) { if (ev < 0 && e.thing == WK_HOLY && e.place >= 0) { hp = e.place; ev = i; } });
    if (hp >= (int)H->places.size() || gone(hp)) return;
    Vec3 u0; double r0; placeAt(hp, u0, r0);
    Vec3 site; bool any = false; double heading = 0;
    if (holy == HOLY_HIGH_PLACE) { double h; any = summitNear(u0, r0 + 4000, r0 + 38000, rng, site, h); }
    else {
        double bestS = -1e18;
        const double a0 = rng.uni() * TAU;
        for (double d : {3000.0, 6000.0, 10000.0, 15000.0, 21000.0})
            for (int q = 0; q < 20; q++) {
                const double ang = a0 + q * TAU / 20;
                const Vec3 u = offsetUnit(u0, std::sin(ang) * (r0 + d), std::cos(ang) * (r0 + d), Rm);
                const SurfaceSample s = at(u);
                if (wetS(s)) continue;
                double down, h; const double sl = grade(u, 45, down, h);
                double sc = -d / 30000;
                switch (holy) {
                    case HOLY_CAVE: if (sl < 0.3) continue; sc += sl + (matFamily(s.material) == FAM_ROCK ? 0.4 : 0); break;
                    case HOLY_GROVE: if (s.material != MAT_FOREST && s.veg < 0.55) continue; sc += s.veg; break;
                    case HOLY_SPRING: { double lowest = 1e18; for (int m = 0; m < 6; m++) { const double b = m * TAU / 6; lowest = std::min(lowest, at(offsetUnit(u, std::sin(b) * 250, std::cos(b) * 250, Rm)).height); } if (lowest < h - 2) continue; sc += 0.5 - sl; break; }
                    default: if (sl > 0.05 || s.material == MAT_FOREST) continue; sc += 0.5 - sl * 5; break;
                }
                if (sc > bestS) { bestS = sc; site = u; heading = down; any = true; }
            }
    }
    if (!any || !clearOf(site, 60)) return;
    Work& w = add(WO_SHRINE, site, 60, seedOf(WO_SHRINE, 0));
    w.age = age;
    if (ev >= 0) { w.event = ev; w.year = H->events[ev].year; }
    w.place = hp; w.planRank = H->places[hp].plan; w.style = holy; w.heading = holy == HOLY_CAVE ? heading : bearingOf(site, u0);
    if (holy == HOLY_GROVE) { GroundEdit e; e.kind = GE_FLAT; e.r = 20; e.width = 8; e.level = at(site).height; e.tone = ET_CLEARED; e.fades = false; w.edits.push_back(e); }   // its clearing
    const int sAge = w.age; const double sBefore = w.before; const Vec3 sU = site;
    // the way: from the place's edge to the shrine, a beaten track with a cairn or a stela every 600 m, in stretches of three kilometres
    const Vec3 a = towardThrough(u0, sU, r0 + 80, Rm);
    const double L = arcLength(a, sU, Rm);
    if (L < 1200) return;
    const double ph1 = rng.uni() * TAU, ph2 = rng.uni() * TAU, amp = std::min(400.0, L * 0.025);
    const int nPts = std::max(2, (int)(L / 100));
    std::vector<Vec3> pts;
    for (int q = 0; q <= nPts; q++) {
        const double t = (double)q / nPts;
        Vec3 c = slerpUnit(a, sU, t);
        const double side = amp * std::sin(PI * t) * (std::sin(ph1 + t * 7.0) * 0.7 + std::sin(ph2 + t * 17.0) * 0.3);
        const double brg = bearingOf(a, sU) + PI / 2;
        c = offsetUnit(c, std::sin(brg) * side, std::cos(brg) * side, Rm);
        pts.push_back(c);
    }
    const uint32_t group = (uint32_t)mix64(pseed ^ 0x7036AAULL);
    int seg = 0;
    for (size_t q0 = 0; q0 + 1 < pts.size(); q0 += 30) {
        const size_t q1 = std::min(pts.size() - 1, q0 + 30);
        const Vec3 mid = pts[(q0 + q1) / 2];
        Work& s = add(WO_WAY, mid, arcLength(pts[q0], pts[q1], Rm) * 0.5 + 40, seedOf(WO_WAY, seg));
        s.age = sAge; s.before = sBefore; s.place = hp; s.planRank = H->places[hp].plan; s.heard = false; s.group = group; s.style = seg++;
        const Frame F(mid, Rm);
        GroundEdit e; e.kind = GE_LINE; e.width = 2.4; e.tone = ET_DARK;
        for (size_t q = q0; q <= q1; q++) {
            if (wetS(at(pts[q]))) { if (e.pts.size() >= 4) s.edits.push_back(e); e.pts.clear(); continue; }
            double x, z; F.local(pts[q], x, z); addPt(e, x, z);
        }
        if (e.pts.size() >= 4) s.edits.push_back(e);
    }
}

// ---- lines on a plain: a figure, rays and cleared trapezoids where the ground is flat and dry ---------------------------------------
namespace {
// a figure's lines in a unit box (x across, z along its length): a creature drawn in one line as the plain's figures are
void figureLines(Rng& r, std::vector<std::vector<std::pair<double, double>>>& out) {
    const int kind = r.irange(5);
    auto ellipse = [&](double cx, double cz, double ax, double az, int n, double a0 = 0, double a1 = TAU) {
        std::vector<std::pair<double, double>> l;
        for (int k = 0; k <= n; k++) { const double t = a0 + (a1 - a0) * k / n; l.push_back({cx + std::sin(t) * ax, cz + std::cos(t) * az}); }
        out.push_back(l);
    };
    auto limb = [&](double x0, double z0, double ang, double len, int fingers) {
        std::vector<std::pair<double, double>> l = {{x0, z0}};
        double x = x0, z = z0, a = ang;
        for (int s = 0; s < 2; s++) { a += r.sym(0.6); x += std::sin(a) * len * 0.5; z += std::cos(a) * len * 0.5; l.push_back({x, z}); }
        out.push_back(l);
        for (int f = 0; f < fingers; f++) { const double fa = a + (f - (fingers - 1) * 0.5) * 0.45; out.push_back({{x, z}, {x + std::sin(fa) * len * 0.22, z + std::cos(fa) * len * 0.22}}); }
    };
    switch (kind) {
        case 0: {   // a bird: its body, the wings out wide, the tail's fan, a long beak
            ellipse(0, 0, 0.09, 0.32, 16);
            for (int s = -1; s <= 1; s += 2) {
                std::vector<std::pair<double, double>> w = {{s * 0.08, 0.05}};
                double x = s * 0.08, z = 0.05;
                const int steps = 5 + r.irange(3);
                for (int k = 0; k < steps; k++) { x += s * 0.16; z += (k % 2 ? 0.08 : -0.05) + r.sym(0.02); w.push_back({x, z}); }
                out.push_back(w);
            }
            for (int f = -2; f <= 2; f++) out.push_back({{0, -0.3}, {f * 0.08, -0.55}});
            out.push_back({{0, 0.32}, {0, 0.75}});
            break;
        }
        case 1: {   // a spiral's creature: a body and its tail wound three times
            ellipse(0, 0.25, 0.2, 0.25, 18);
            std::vector<std::pair<double, double>> sp;
            for (int k = 0; k <= 90; k++) { const double t = k / 90.0, a = t * TAU * 3, rr = 0.36 * (1 - t) + 0.02; sp.push_back({0.2 + std::cos(a) * rr * 0.9, -0.45 + std::sin(a) * rr * 0.9}); }
            out.push_back(sp);
            for (int s = -1; s <= 1; s += 2) limb(s * 0.18, 0.35, s * 1.1, 0.4, 4);
            ellipse(0, 0.58, 0.08, 0.08, 10);
            break;
        }
        case 2: {   // a spider: its two bodies and eight legs
            ellipse(0, 0.18, 0.12, 0.16, 14); ellipse(0, -0.12, 0.17, 0.2, 16);
            for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 4; k++) limb(s * 0.12, 0.25 - k * 0.12, s * (0.7 + k * 0.45), 0.6, 0);
            break;
        }
        case 3: {   // a fish or a whale: its outline, the fins, the forked tail
            ellipse(0, 0.05, 0.22, 0.55, 22, 0.3, TAU - 0.3);
            out.push_back({{0.065, -0.47}, {0.25, -0.8}, {0, -0.62}, {-0.25, -0.8}, {-0.065, -0.47}});
            for (int s = -1; s <= 1; s += 2) out.push_back({{s * 0.2, 0.1}, {s * 0.42, -0.05}, {s * 0.2, -0.12}});
            ellipse(0.06, 0.38, 0.03, 0.03, 8);
            break;
        }
        default: {   // a lizard: its body, four legs, the long tail
            ellipse(0, 0.12, 0.11, 0.28, 16);
            ellipse(0, 0.48, 0.07, 0.09, 10);
            for (int s = -1; s <= 1; s += 2) { limb(s * 0.1, 0.25, s * 1.0, 0.36, 3); limb(s * 0.1, -0.05, s * 2.1, 0.36, 3); }
            std::vector<std::pair<double, double>> t = {{0, -0.16}};
            for (int k = 1; k <= 8; k++) t.push_back({std::sin(k * 0.8) * 0.06 * k / 8.0 + 0.02 * k, -0.16 - k * 0.08});
            out.push_back(t);
            break;
        }
    }
}
}

void WB::geoglyphs() {
    if (age > AGE_MEDIEVAL) return;
    Rng rng(pseed ^ 0x70380ULL);
    std::vector<std::pair<float, int>> plains;
    for (int c = 0; c < P.grid.cells; c++)
        if (pp->territory[c] && P.grid.land[c] && P.grid.relief[c] < 0.14f && P.grid.veg[c] < 0.45f && P.grid.tempC[c] > -3) plains.push_back({P.grid.relief[c] + 0.3f * (float)unitFromHash(mix64(pseed ^ (uint64_t)c)), c});
    std::sort(plains.begin(), plains.end());
    const int want = 1 + (pp->areaKm2 > 400000 ? 1 : 0) + (pp->areaKm2 > 1500000 ? 1 : 0);
    int k = 0;
    for (const auto& pc : plains) {
        if (made[WO_GEOGLYPH] >= want || k > 12) break;
        k++;
        const Vec3 c0 = offsetUnit(P.grid.unitOf(pc.second), rng.sym(6000), rng.sym(6000), Rm);
        Vec3 best; double bestRange = 1e18; bool any = false;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) {
            const Vec3 u = offsetUnit(c0, i * 1500.0, j * 1500.0, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || s.material == MAT_FOREST || matFamily(s.material) == FAM_SNOW) continue;
            double lo = s.height, hi = s.height;
            for (int m = 0; m < 6; m++) { const double b = m * TAU / 6; const SurfaceSample q = at(offsetUnit(u, std::sin(b) * 450, std::cos(b) * 450, Rm)); if (wetS(q)) { lo = -1e18; break; } lo = std::min(lo, q.height); hi = std::max(hi, q.height); }
            if (lo < -1e17) continue;
            if (hi - lo < bestRange) { bestRange = hi - lo; best = u; any = true; }
        }
        if (!any || bestRange > 14 || !clearOf(best, 900)) continue;
        Work& w = add(WO_GEOGLYPH, best, 900, seedOf(WO_GEOGLYPH, k));
        w.age = age; w.style = 0;
        Rng r2(w.seed);
        const double S = 160 + 140 * r2.uni(), hd = r2.uni() * TAU;   // the figure's half size: its lines cleared bands wide enough to read from the air
        std::vector<std::vector<std::pair<double, double>>> lines;
        figureLines(r2, lines);
        const double lw = 8 + 4 * r2.uni();
        for (const auto& l : lines) {
            GroundEdit e; e.kind = GE_LINE; e.width = lw; e.tone = ET_CLEARED;
            for (const auto& q : l) { double x, z; rot(hd, q.first * S * 2, q.second * S * 2, x, z); addPt(e, x, z); }
            w.edits.push_back(e);
        }
        // the rays from a centre beside it, and the cleared trapezoids
        const double ca = r2.uni() * TAU, cd = S * 1.6 + 120;
        const double rx = std::sin(ca) * cd, rz = std::cos(ca) * cd;
        const int nr = 3 + r2.irange(6);
        for (int q = 0; q < nr; q++) {
            const double a = r2.uni() * TAU, len = 900 + 1500 * r2.uni();
            GroundEdit e; e.kind = GE_LINE; e.width = 6; e.tone = ET_CLEARED;
            addPt(e, rx, rz); addPt(e, rx + std::sin(a) * len, rz + std::cos(a) * len);
            w.edits.push_back(e);
        }
        const int nt = r2.irange(3);
        for (int q = 0; q < nt; q++) {
            const double a = r2.uni() * TAU, len = 500 + 900 * r2.uni(), off = r2.sym(250);
            GroundEdit e; e.kind = GE_LINE; e.width = 55 + 40 * r2.uni(); e.width2 = 14 + 10 * r2.uni(); e.tone = ET_CLEARED;
            const double sx = rx + std::cos(a) * off, sz = rz - std::sin(a) * off;
            addPt(e, sx, sz); addPt(e, sx + std::sin(a) * len, sz + std::cos(a) * len);
            w.edits.push_back(e);
        }
        w.sideA = Vec3(rx, 0, rz);   // the rays' centre, in the work's metres (its cairns)
        w.length = S * 2;
    }
}

// ---- the fortified line: along the frontier with the other people, or across the way into the land the people held -----------------
void WB::fortLine() {
    if (age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC || pp->places.empty()) return;
    Rng rng(pseed ^ 0x703A0ULL);
    const PlanGrid& G = P.grid;
    Vec3 mid, dir; bool any = false;
    if (P.peoples > 1) {   // the frontier: our cell nearest the divide where the other's territory comes near
        const Vec3 pole = peopleDivide(g);
        const double sideSign = p == 0 ? 1 : -1;
        double best = 1e18;
        for (int c = 0; c < G.cells; c++) {
            if (!pp->territory[c] || !G.land[c]) continue;
            const Vec3 u = G.unitOf(c);
            const double dd = std::fabs(dot(u, pole));
            if (dd > best) continue;
            int nb[8]; const int nn = G.neighbours(c, nb);
            bool near = false; for (int q = 0; q < nn; q++) if (P.plans[1 - p].territory[nb[q]]) near = true;
            for (int q = 0; q < nn && !near; q++) { int nb2[8]; const int n2 = G.neighbours(nb[q], nb2); for (int m = 0; m < n2; m++) if (P.plans[1 - p].territory[nb2[m]]) near = true; }
            if (!near) continue;
            best = dd; mid = u; any = true;
        }
        if (any) {
            mid = normalize(mid + pole * (sideSign * 0.25 * G.sizeKm() / G.R));   // a few kilometres in on our side
            dir = normalize(cross(pole, mid));
        }
    }
    if (!any) {   // the border nearest the capital that faces land the people did not hold
        const Vec3 cap = pp->places[0].unit;
        double best = 1e18; int bc = -1, outC = -1;
        for (int c = 0; c < G.cells; c++) {
            if (!pp->territory[c] || !G.land[c]) continue;
            int nb[8]; const int nn = G.neighbours(c, nb);
            for (int q = 0; q < nn; q++) if (!pp->territory[nb[q]] && G.land[nb[q]]) {
                const double d = arcLength(G.unitOf(c), cap, Rm);
                if (d < best && d > 20000) { best = d; bc = c; outC = nb[q]; }
                break;
            }
        }
        if (bc < 0) return;
        mid = G.unitOf(bc);
        const Vec3 out = G.unitOf(outC), fwd = normalize(out - mid * dot(out, mid));
        dir = normalize(cross(mid, fwd));
        any = true;
    }
    if (!any) return;
    const double total = 6000 + 9000 * rng.uni(), segL = 2500;
    const int nSeg = std::max(1, (int)std::lround(total / segL));
    const uint32_t group = (uint32_t)mix64(pseed ^ 0x703AAAULL);
    for (int s = 0; s < nSeg; s++) {
        const double t0 = (s - nSeg * 0.5) * segL, t1 = t0 + segL;
        const Vec3 a = normalize(mid * std::cos(t0 / Rm) + dir * std::sin(t0 / Rm)), b = normalize(mid * std::cos(t1 / Rm) + dir * std::sin(t1 / Rm));
        const Vec3 m = slerpUnit(a, b, 0.5);
        if (wetS(at(m)) || planCoreAt(P, m, 300)) continue;
        Work& w = add(WO_FORT_LINE, m, segL * 0.5 + 60, seedOf(WO_FORT_LINE, s));
        w.age = age; w.style = age - AGE_MEDIEVAL; w.group = group; w.heard = s == nSeg / 2; w.heading = bearingOf(a, b); w.length = segL;
        const Frame F(m, Rm);
        double ax, az, bx, bz; F.local(a, ax, az); F.local(b, bx, bz);
        w.sideA = Vec3(ax, 0, az); w.sideB = Vec3(bx, 0, bz);
        if (w.style >= 1) {   // the ditch before the line (toward the far side)
            GroundEdit e; e.kind = GE_DITCH; e.width = w.style == 1 ? 6 : 8; e.depth = w.style == 1 ? 2.4 : 3.0; e.bank = w.style == 1 ? 1.6 : 1.0; e.tone = ET_DARK;
            const double nx = std::cos(w.heading) * 9, nz = -std::sin(w.heading) * 9;
            const int steps = 25;
            for (int q = 0; q <= steps; q++) {
                const double t = (double)q / steps;
                const Vec3 u = slerpUnit(a, b, t);
                if (wetS(at(u))) { if (e.pts.size() >= 4) w.edits.push_back(e); e.pts.clear(); continue; }
                double x, z; F.local(u, x, z); addPt(e, x + nx, z + nz);
            }
            if (e.pts.size() >= 4) w.edits.push_back(e);
        }
    }
}

// ---- the battlefields: where the history fought its battles, between the two sides' places -----------------------------------------------
void WB::battlefields() {
    if (!H) return;
    Rng rng(pseed ^ 0x703B0ULL);
    // each war's greatest battle (and the next of a great war), the greatest wars first
    std::vector<std::pair<int64_t, int>> evs;
    std::map<int, int> perWar;
    forEvents(HE_BATTLE, [&](int i, const HistoryEvent& e) { if (e.place >= 0 && e.thing >= 0 && e.thing < (int)H->wars.size()) evs.push_back({-e.value, i}); });
    std::sort(evs.begin(), evs.end());
    int k = 0;
    for (const auto& pe : evs) {
        if (made[WO_BATTLEFIELD] >= 8) break;
        const HistoryEvent& e = H->events[pe.second];
        if (e.year < ageFrom) continue;   // B-414: a battle of an earlier age left nothing on the ground
        const HistoryWar& war = H->wars[e.thing];
        int& nw = perWar[e.thing];
        if (nw >= (war.dead > 20000 ? 2 : 1)) continue;
        // the two sides: the battle's place, and the place the other side came from (the other people's place, the side of the war's
        // head, or the seat)
        Vec3 ub; double rb; placeAt(e.place, ub, rb);
        Vec3 ua; double ra = 0; bool joint = false;
        if (e.other >= 0 && P.peoples > 1 && e.other < (int)P.plans[1 - p].places.size()) {
            if (p != 0) continue;   // a battle with the other people: the first people's history lays it down (both hold it)
            ua = P.plans[1 - p].places[e.other].unit; ra = P.plans[1 - p].places[e.other].radiusM; joint = true;
        } else {
            int other = war.sideA >= 0 ? war.sideA : historySeat(*H, e.year);
            if (other == e.place && war.sideB >= 0) other = war.sideB;
            if (other < 0 || other == e.place) {   // a war at its own seat: they came from the nearest other place
                double bd = 1e18; for (int q = 0; q < (int)H->places.size(); q++) if (q != e.place && H->placeStands(q, e.year)) { Vec3 uq; double rq; placeAt(q, uq, rq); const double d = arcLength(uq, ub, Rm); if (d < bd) { bd = d; other = q; } }
            }
            if (other < 0) continue;
            placeAt(other, ua, ra);
        }
        const double sep = arcLength(ua, ub, Rm);
        if (sep < rb + ra + 2500) continue;
        const double d = clampd(0.3 * sep, rb + 1500, std::min(rb + 7000, sep - ra - 1500));
        const Vec3 c0 = towardThrough(ub, ua, d, Rm);
        Vec3 best; double bestS = -1e18; bool any = false;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) {
            const Vec3 u = offsetUnit(c0, i * 350.0, j * 350.0, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || matFamily(s.material) == FAM_SNOW) continue;
            double down, h; const double sl = grade(u, 60, down, h);
            const double sc = -sl * 3 - (s.material == MAT_FOREST ? 0.3 : 0) - std::hypot(i, j) * 0.05;
            if (sc > bestS && clearOf(u, 420)) { bestS = sc; best = u; any = true; }
        }
        if (!any) continue;
        nw++;
        Work& w = add(WO_BATTLEFIELD, best, 420, seedOf(WO_BATTLEFIELD, k++));
        w.age = age; w.before = std::max(0, H->last - e.year); w.event = pe.second; w.year = e.year; w.war = e.thing; w.place = e.place;
        if (H->places[e.place].plan >= 0) w.planRank = H->places[e.place].plan;
        w.sideA = ua; w.sideB = ub; w.heading = bearingOf(ub, ua); w.style = joint ? 1 : 0;
        w.length = sep;
        // the ground: trenches facing each other and the shells' craters between (the machine ages), the glassed craters of the futuristic
        Rng r2(w.seed ^ 0xBA77ULL);
        const int a = w.age;
        const double hx = std::cos(w.heading), hz = -std::sin(w.heading), fx = std::sin(w.heading), fz = std::cos(w.heading);   // across, toward A
        if (a == AGE_INDUSTRIAL || a == AGE_MODERN) {
            for (int side = -1; side <= 1; side += 2) {
                const int lines = a == AGE_INDUSTRIAL ? 2 : 1;
                for (int l = 0; l < lines; l++) {
                    GroundEdit e2; e2.kind = GE_DITCH; e2.width = 2.4; e2.depth = 2.0; e2.bank = 0.8 * side; e2.tone = ET_DARK;
                    const double off = side * (90 + l * 35 + r2.uni() * 20), half = 260 + 120 * r2.uni();
                    for (int q = 0; q <= 26; q++) {
                        const double t = -half + 2 * half * q / 26, zig = (q % 2 ? 7.0 : -7.0) + r2.sym(2);
                        addPt(e2, hx * t + fx * (off + zig), hz * t + fz * (off + zig));
                    }
                    w.edits.push_back(e2);
                }
            }
            const int nc = a == AGE_INDUSTRIAL ? 60 + r2.irange(60) : 40 + r2.irange(50);
            for (int q = 0; q < nc; q++) {
                GroundEdit c; c.kind = GE_BOWL; c.r = a == AGE_INDUSTRIAL ? 2 + 3 * r2.uni() : 3 + 7 * r2.uni() * r2.uni(); c.depth = c.r * 0.45; c.tone = ET_DARK;
                const double t = r2.sym(300), s = r2.sym(120);
                c.x = hx * t + fx * s; c.z = hz * t + fz * s;
                w.edits.push_back(c);
            }
        } else if (a == AGE_FUTURISTIC) {
            const int nc = 5 + r2.irange(10);
            for (int q = 0; q < nc; q++) {
                GroundEdit c; c.kind = GE_BOWL; c.r = 10 + 28 * r2.uni(); c.depth = c.r * 0.3; c.tone = ET_GLASS;
                const double t = r2.sym(320), s = r2.sym(160);
                c.x = hx * t + fx * s; c.z = hz * t + fz * s;
                w.edits.push_back(c);
            }
        }
    }
}

// ---- the launch sites: near the equator, on flat ground, the sea to the east where it can be ------------------------------------------
void WB::launchSites() {
    if (!H || age < AGE_MODERN || age == AGE_FAR_FUTURE) return;
    std::vector<std::pair<int, int>> src;   // (event, the work: WK_LAUNCH or WK_ELEVATOR)
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) { if ((e.thing == WK_LAUNCH || e.thing == WK_ELEVATOR) && src.size() < 2) src.push_back({i, e.thing}); });
    if (src.empty()) return;
    Rng rng(pseed ^ 0x703C0ULL);
    const PlanGrid& G = P.grid;
    std::vector<std::pair<double, int>> low;
    for (int c = 0; c < G.cells; c++) {
        if (!pp->territory[c] || !G.land[c] || G.relief[c] > 0.35f) continue;
        double la, lo; G.latLonOf(c, la, lo);
        double sc = std::fabs(la);
        int nb[8]; const int nn = G.neighbours(c, nb);
        for (int q = 0; q < nn; q++) if (!G.land[nb[q]]) { double la2, lo2; G.latLonOf(nb[q], la2, lo2); if (std::remainder(lo2 - lo, TAU) > 0) sc -= 0.03; }   // the sea to the east
        low.push_back({sc, c});
    }
    std::sort(low.begin(), low.end());
    int k = 0;
    for (const auto& se : src) {
        bool done = false;
        for (int li = 0; li < (int)low.size() && li < 12 && !done; li++) {
            const Vec3 c0 = offsetUnit(G.unitOf(low[li].second), rng.sym(8000), rng.sym(8000), Rm);
            Vec3 best; double bestS = -1e18; bool any = false;
            for (int j = -3; j <= 3; j++) for (int i = -3; i <= 3; i++) {
                const Vec3 u = offsetUnit(c0, i * 700.0, j * 700.0, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s) || s.material == MAT_FOREST) continue;
                double down, h; const double sl = grade(u, 120, down, h);
                if (sl > 0.03) continue;
                const double sc = -sl * 20 - std::hypot(i, j) * 0.02;
                if (sc > bestS && clearOf(u, 1300)) { bestS = sc; best = u; any = true; }
            }
            if (!any) continue;
            const HistoryEvent& e = H->events[se.first];
            Work& w = add(WO_LAUNCH, best, 1300, seedOf(WO_LAUNCH, k++));
            w.age = clampi(age, AGE_MODERN, AGE_FUTURISTIC); w.event = se.first; w.year = e.year;   // stood where the latitude put it, not by its town
            w.style = se.second == WK_ELEVATOR ? 2 : (w.age == AGE_FUTURISTIC ? 1 : 0);
            w.heading = PI / 2 + rng.sym(0.3);   // the pads toward the east
            Rng r2(w.seed);
            if (w.style == 0) {   // the pads' aprons made flat, the crawlerways from the hall to each
                const int pads = 1 + r2.irange(3);
                w.height = pads;
                for (int q = 0; q < pads; q++) {
                    const double t = (q - (pads - 1) * 0.5) * 700, along = 650;
                    double x, z; rot(w.heading, t, along, x, z);
                    GroundEdit f; f.kind = GE_FLAT; f.x = x; f.z = z; f.r = 95; f.width = 25; f.level = at(Frame(best, Rm).unit(x, z)).height; f.tone = ET_PAVED; f.fades = false;
                    w.edits.push_back(f);
                    GroundEdit l; l.kind = GE_LINE; l.width = 28; l.tone = ET_PAVED; l.fades = false;
                    double hx, hz; rot(w.heading, 0, -380, hx, hz);
                    addPt(l, hx, hz); addPt(l, x, z);
                    w.edits.push_back(l);
                }
            } else if (w.style == 1) {   // a mass driver's track rising east
                GroundEdit l; l.kind = GE_LINE; l.width = 14; l.tone = ET_PAVED; l.fades = false;
                double x0, z0, x1, z1; rot(w.heading, 0, -1100, x0, z0); rot(w.heading, 0, 1200, x1, z1);
                addPt(l, x0, z0); addPt(l, x1, z1);
                w.edits.push_back(l);
            } else {
                GroundEdit f; f.kind = GE_FLAT; f.r = 260; f.width = 40; f.level = at(best).height; f.tone = ET_PAVED; f.fades = false;
                w.edits.push_back(f);
            }
            done = true;
        }
    }
}

// ---- the antenna field: on a high plain, away from the towns ------------------------------------------------------------------------
void WB::antennas() {
    if (!H || !H->profile.radio || age < AGE_INDUSTRIAL || age == AGE_FAR_FUTURE || pp->places.empty()) return;
    Rng rng(pseed ^ 0x703D0ULL);
    const PlanGrid& G = P.grid;
    const Vec3 cap = pp->places[0].unit;
    double mean = 0; int n = 0;
    for (int c = 0; c < G.cells; c++) if (pp->territory[c] && G.land[c]) { mean += G.height[c]; n++; }
    if (!n) return;
    mean /= n;
    std::vector<std::pair<double, int>> high;
    for (int c = 0; c < G.cells; c++) {
        if (!pp->territory[c] || !G.land[c] || G.relief[c] > 0.3f) continue;
        const double d = arcLength(G.unitOf(c), cap, Rm);
        if (d < 20000 || d > 300000) continue;
        high.push_back({-(G.height[c] - mean) / 400.0 + G.relief[c] * 2 + rng.uni() * 0.3, c});
    }
    std::sort(high.begin(), high.end());
    for (int li = 0; li < (int)high.size() && li < 10; li++) {
        const Vec3 c0 = offsetUnit(G.unitOf(high[li].second), rng.sym(6000), rng.sym(6000), Rm);
        Vec3 best; double bestS = -1e18; bool any = false;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) {
            const Vec3 u = offsetUnit(c0, i * 800.0, j * 800.0, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || s.material == MAT_FOREST || matFamily(s.material) == FAM_SNOW) continue;
            double down, h; const double sl = grade(u, 100, down, h);
            if (sl > 0.05) continue;
            const double sc = -sl * 10 + h * 0.001;
            if (sc > bestS && clearOf(u, 700)) { bestS = sc; best = u; any = true; }
        }
        if (!any) continue;
        Work& w = add(WO_ANTENNA, best, 700, seedOf(WO_ANTENNA, 0));
        w.age = age; w.year = H->profile.radioYear; w.heading = rng.uni() * PI;
        w.style = age == AGE_INDUSTRIAL ? 0 : (age == AGE_FUTURISTIC ? 3 : (rng.uni() < 0.5 ? 1 : 2));
        return;
    }
}

// ---- the colossi: over the capital, or at its harbour's mouth -----------------------------------------------------------------------
void WB::colossi() {
    if (age < AGE_MEDIEVAL || age == AGE_FAR_FUTURE || pp->places.empty()) return;
    const bool want = C.genome.of(GV_FOCUS) == FO_COLOSSUS || C.genome.of(GV_MONUMENTS) == MN_COLOSSI;
    if (!want) return;
    Rng rng(pseed ^ 0x703F0ULL);
    const PlanPlace& cap = pp->places[0];
    Vec3 site; double facing = 0; bool any = false;
    if (cap.coast) { double seaward; if (shoreNear(cap.unit, cap.radiusM + 200, cap.radiusM + 4000, rng, site, seaward, true)) { facing = seaward; any = true; } }
    if (!any) { double h; if (summitNear(cap.unit, cap.radiusM + 600, cap.radiusM + 5000, rng, site, h)) { facing = bearingOf(site, cap.unit); any = true; } }
    if (!any || !clearOf(site, 80)) return;
    Work& w = add(WO_COLOSSUS, site, 80, seedOf(WO_COLOSSUS, 0));
    w.age = age; w.place = 0; w.planRank = 0; w.heading = facing; w.style = (int)(rng.uni() * 2);
    w.height = age == AGE_MEDIEVAL ? 24 + 12 * rng.uni() : (age == AGE_INDUSTRIAL ? 32 + 14 * rng.uni() : (age == AGE_MODERN ? 40 + 20 * rng.uni() : 70 + 40 * rng.uni()));
}

// ---- the terraformed basins: a futuristic people's land made level and laid out in channels -----------------------------------------
void WB::terraforms() {
    if (age != AGE_FUTURISTIC || pp->places.empty()) return;
    Rng rng(pseed ^ 0x70400ULL);
    const PlanGrid& G = P.grid;
    std::vector<std::pair<double, int>> low;
    for (int c = 0; c < G.cells; c++) if (pp->territory[c] && G.land[c] && G.relief[c] < 0.25f) low.push_back({G.height[c] + rng.uni() * 300, c});
    std::sort(low.begin(), low.end());
    int k = 0;
    for (int li = 0; li < (int)low.size() && li < 10 && k < 2; li++) {
        const Vec3 u = offsetUnit(G.unitOf(low[li].second), rng.sym(5000), rng.sym(5000), Rm);
        const SurfaceSample s = at(u);
        if (wetS(s)) continue;
        const double R = 1500 + 2000 * rng.uni();
        if (!clearOf(u, R + 200)) continue;
        double mean = s.height; int nm = 1;
        for (int m = 0; m < 12; m++) { const double b = m * TAU / 12; const SurfaceSample q = at(offsetUnit(u, std::sin(b) * R * 0.7, std::cos(b) * R * 0.7, Rm)); if (!wetS(q)) { mean += q.height; nm++; } }
        mean /= nm;
        Work& w = add(WO_TERRAFORM, u, R + 200, seedOf(WO_TERRAFORM, k++));
        w.age = AGE_FUTURISTIC; w.heading = rng.uni() * PI; w.level = mean; w.length = 2 * R;
        GroundEdit f; f.kind = GE_FLAT; f.r = R; f.width = 160; f.level = mean; f.tone = ET_CLEARED; f.fades = false;
        w.edits.push_back(f);
        const double step = 180;
        for (int q = -(int)(R / step); q <= (int)(R / step); q++) {   // the channels across it
            const double off = q * step, half = std::sqrt(std::max(0.0, (R - 120) * (R - 120) - off * off));
            if (half < 50) continue;
            GroundEdit l; l.kind = GE_LINE; l.width = 6; l.tone = ET_DARK; l.fades = false;
            double x0, z0, x1, z1; rot(w.heading, off, -half, x0, z0); rot(w.heading, off, half, x1, z1);
            addPt(l, x0, z0); addPt(l, x1, z1);
            w.edits.push_back(l);
        }
    }
}

// ---- the last shelter: by the place the collapse ended in, in the nearest steep rock ---------------------------------------------------
// a shelter's site near a place: steep rock, near (a medieval people's citadel on the height); D-05: a people that lost its sun's a hall into the
// rock by the warmest ground
bool WB::shelterSite(const Vec3& u0, double r0, Rng& rng, Vec3& best, double& bestDown) {
    const bool sunlost = H->cause == CAUSE_SUN_LOST, embers = sunlost && g.hasTrait(TR_EMBERS);
    double bestS = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (double d : {600.0, 1400.0, 2600.0, 4200.0, 6500.0})
        for (int q = 0; q < 20; q++) {
            const double ang = a0 + q * TAU / 20, dd = r0 + d;
            const Vec3 u = offsetUnit(u0, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, 40, down, h);
            double sc = std::min(sl, 0.8) - d / 15000 + (matFamily(s.material) == FAM_ROCK ? 0.2 : 0);
            if (age == AGE_MEDIEVAL && !sunlost) sc = h * 0.004 - d / 15000;   // a citadel on the height
            if (embers) sc += 1.5 * emberFieldAt(g, u * g.R);
            if (sc > bestS && clearOf(u, 140)) { bestS = sc; best = u; bestDown = down; any = true; }
        }
    return any;
}
// its way by the age (D-05: a medieval people that lost its sun's the halls under the mountain, a far-future one's a habitat's)
int WB::shelterStyle() const {
    if (age == AGE_MEDIEVAL && H && H->cause == CAUSE_SUN_LOST) return 5;
    return age == AGE_PREHISTORIC ? 0 : (age == AGE_MEDIEVAL ? 1 : (age == AGE_INDUSTRIAL ? 2 : (age == AGE_MODERN ? 3 : 4)));
}
void WB::shelter() {
    if (!H || H->lastPlace < 0 || (age == AGE_FAR_FUTURE && H->cause != CAUSE_SUN_LOST)) return;
    Rng rng(pseed ^ 0x70410ULL);
    Vec3 u0; double r0; placeAt(H->lastPlace, u0, r0);
    Vec3 best; double bestDown = 0;
    if (!shelterSite(u0, r0, rng, best, bestDown)) return;
    Work& w = add(WO_SHELTER, best, 140, seedOf(WO_SHELTER, 0));
    w.age = age; w.place = H->lastPlace; w.planRank = H->places[H->lastPlace].plan; w.year = H->last;
    forEvents(HE_END, [&](int i, const HistoryEvent&) { w.event = i; });
    double down2, h2; const double sl = grade(best, 40, down2, h2);
    w.style = shelterStyle();
    w.heading = sl > 0.12 ? bestDown : bearingOf(best, u0);   // its door out of the hill, else toward the place
    w.height = sl;
}

// ---- D-05: the works of the end of a people that lost its sun ---------------------------------------------------------------------------
// the warmest open ground near a place for a work of `reach`: a field of embers where the world has them, else the lowest dry ground (the
// deep), flat enough
bool WB::warmGround(const Vec3& u0, double r0, double reach, Rng& rng, Vec3& best) {
    const bool embers = g.hasTrait(TR_EMBERS);
    double bestS = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (double d : {250.0, 600.0, 1100.0, 1800.0, 2800.0, 4000.0})
        for (int q = 0; q < 14; q++) {
            const double ang = a0 + q * TAU / 14, dd = r0 + reach + d;
            const Vec3 u = offsetUnit(u0, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, std::max(30.0, reach * 0.6), down, h);
            if (sl > 0.14) continue;
            double sc = -h / 400.0 - d / 6000.0 - 3 * sl;
            if (embers) sc += 3.0 * emberFieldAt(g, u * g.R);
            if (sc > bestS && clearOf(u, reach)) { bestS = sc; best = u; any = true; }
        }
    return any;
}
// the places that lived below the longest (the last, then the two left last after the last sunrise): a shelter by each of the others (the last's
// is `shelter`'s; their last records with them, `before` the years before the end it was left), a dome over the warm ground by each (a medieval
// people's vaulted hall over its spring, none for a prehistoric people's: the warm cave was its shelter), its glasshouses beside it (an
// industrial people's and later); the graves of the last generations on a slope facing the last place. Every draw its own stream
void WB::sunlostWorks() {
    if (!H || H->cause != CAUSE_SUN_LOST || H->lastPlace < 0) return;
    std::vector<int> ps;
    for (int i = 0; i < (int)H->places.size(); i++) {
        const HistoryPlace& q = H->places[i];
        if (i == H->lastPlace || q.fate != HP_COLLAPSE || gone(i) || q.ended < H->profile.lastSunrise || q.ended >= H->last) continue;
        ps.push_back(i);
    }
    std::sort(ps.begin(), ps.end(), [&](int a, int b) { return H->places[a].ended != H->places[b].ended ? H->places[a].ended > H->places[b].ended : a < b; });
    if (ps.size() > 2) ps.resize(2);
    for (size_t k = 0; k < ps.size(); k++) {   // their shelters
        const HistoryPlace& q = H->places[ps[k]];
        Rng rng(pseed ^ (0xD05510ULL + k));
        Vec3 u0; double r0; placeAt(ps[k], u0, r0);
        Vec3 best; double bestDown = 0;
        if (!shelterSite(u0, r0, rng, best, bestDown)) continue;
        double down2, h2; const double sl = grade(best, 40, down2, h2);
        Work& w = add(WO_SHELTER, best, 140, seedOf(WO_SHELTER, 1 + (int)k));
        w.place = ps[k]; w.planRank = q.plan; w.year = q.ended; w.event = q.endEvent; w.before = H->last - q.ended;
        w.style = shelterStyle(); w.heading = sl > 0.12 ? bestDown : bearingOf(best, u0); w.height = sl;
    }
    if (age >= AGE_MEDIEVAL) {   // the domes and their glasshouses
        std::vector<int> by = {H->lastPlace}; by.insert(by.end(), ps.begin(), ps.end());
        const int style = age == AGE_MEDIEVAL ? 0 : (age == AGE_INDUSTRIAL ? 1 : (age == AGE_MODERN ? 2 : (age == AGE_FUTURISTIC ? 3 : 4)));
        for (size_t k = 0; k < by.size(); k++) {
            const HistoryPlace& q = H->places[by[k]];
            Rng rng(pseed ^ (0xD05520ULL + k));
            Vec3 u0; double r0; placeAt(by[k], u0, r0);
            const double rad = style == 0 ? 11 + 5 * rng.uni() : 24 + 16 * rng.uni(), reach = rad + 25;
            Vec3 u; if (!warmGround(u0, r0, reach, rng, u)) continue;
            {
                Work& w = add(WO_DOME, u, reach, seedOf(WO_DOME, (int)k));
                w.style = style; w.place = by[k]; w.planRank = q.plan; w.year = q.ended; w.width = rad; w.heading = bearingOf(u, u0);
                w.before = by[k] == H->lastPlace ? 0 : H->last - q.ended;
            }
            if (age < AGE_INDUSTRIAL) continue;
            // its glasshouses: rows of them on flat dry ground beside it, their long sides toward it
            const int gs = age >= AGE_FUTURISTIC ? 2 : (age == AGE_MODERN ? 1 : 0);
            const double hw = gs == 0 ? 4.0 : (gs == 1 ? 4.5 : 6.0), hd = gs == 0 ? 12.0 : (gs == 1 ? 18.0 : 20.0);
            const int rows = 3 + rng.irange(4), per = 1 + rng.irange(2);
            const double across = 0.5 * (rows * (2 * hw + 3) - 3), along = 0.5 * (per * 2 * hd + (per - 1) * 4), greach = std::hypot(across, along) + 6;
            Vec3 best; double bestS = -1e18; bool any = false;
            const double a0 = rng.uni() * TAU;
            for (double d : {20.0, 60.0, 120.0, 220.0, 360.0})
                for (int qq = 0; qq < 12; qq++) {
                    const double ang = a0 + qq * TAU / 12, dd = reach + greach + d;
                    const Vec3 v = offsetUnit(u, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                    if (wetS(at(v))) continue;
                    double down, h; const double sl = grade(v, std::max(25.0, greach * 0.6), down, h);
                    if (sl > 0.08) continue;
                    const double sc = -sl * 10 - d / 300.0;
                    if (sc > bestS && clearOf(v, greach)) { bestS = sc; best = v; any = true; }
                }
            if (!any) continue;
            Work& w = add(WO_GREENHOUSE, best, greach, seedOf(WO_GREENHOUSE, (int)k));
            w.style = gs; w.place = by[k]; w.planRank = q.plan; w.year = q.ended; w.width = across; w.length = along; w.heading = bearingOf(best, u);
            w.before = by[k] == H->lastPlace ? 0 : H->last - q.ended;
        }
    }
    {   // the graves of the last generations: on a slope facing the last place, in its people's way of the dead
        Rng rng(pseed ^ 0xD05530ULL);
        Vec3 u0; double r0; placeAt(H->lastPlace, u0, r0);
        const int dead = C.genome.of(GV_DEAD), a = age;
        int style;
        if (a == AGE_PREHISTORIC || dead == DE_BARROWS) style = 0;
        else if (dead == DE_TOWERS) style = 5;
        else if (a >= AGE_FUTURISTIC) style = 4;
        else if (dead == DE_TOMBS) style = 1;
        else style = a == AGE_MODERN ? 3 : 2;
        const double reach = style == 2 || style == 3 ? 170 : 120, ph = at(u0).height;
        Vec3 best; double bestS = -1e18, bestDown = 0; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {250.0, 500.0, 900.0, 1600.0, 2500.0})
            for (int q = 0; q < 16; q++) {
                const double ang = a0 + q * TAU / 16, dd = r0 + reach + d;
                const Vec3 u = offsetUnit(u0, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s) || s.material == MAT_FOREST) continue;
                double down, h; const double sl = grade(u, 50, down, h);
                if (sl > 0.25) continue;
                const double sc = std::cos(down - (ang + PI)) * std::min(sl, 0.15) * 4 - d / 8000 - std::max(0.0, h - ph) / 2000;   // a slope that looks at the place
                if (sc > bestS && clearOf(u, reach)) { bestS = sc; best = u; bestDown = down; any = true; }
            }
        if (any) {
            Work& w = add(WO_NECROPOLIS, best, reach, seedOf(WO_NECROPOLIS, 0));
            w.style = style; w.place = H->lastPlace; w.planRank = H->places[H->lastPlace].plan; w.year = H->last; w.heading = bestDown;
        }
    }
}

// ---- the far future's inexplicable: a plain made flat, pillars to the horizon, a mountain cut into terraces, a lattice over a valley, a
// sphere on its point -------------------------------------------------------------------------------------------------------------------
void WB::strange() {
    if (age != AGE_FAR_FUTURE || pp->places.empty()) return;
    Rng rng(pseed ^ 0x70420ULL);
    const PlanGrid& G = P.grid;
    const int want = 2 + rng.irange(3);
    std::vector<int> kinds = {0, 1, 2, 3, 4};
    for (int q = 4; q > 0; q--) std::swap(kinds[q], kinds[rng.irange(q + 1)]);
    int k = 0;
    for (int kk = 0; kk < 5 && k < want; kk++) {
        const int st = kinds[kk];
        if (st == 2) {   // the territory's highest mountain cut into terraces
            int bc = -1; double bh = -1e18;
            for (int c = 0; c < G.cells; c++) if (pp->territory[c] && G.land[c] && G.height[c] > bh) { bh = G.height[c]; bc = c; }
            if (bc < 0) continue;
            Vec3 top; double hTop;
            if (!summitNear(G.unitOf(bc), 500, 15000, rng, top, hTop)) continue;
            const double R = 1800 + 1200 * rng.uni();
            if (!clearOf(top, R)) continue;
            Work& w = add(WO_STRANGE, top, R + 100, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 2; w.height = hTop;
            // each ring band cut flat at the lowest ground of its ring, rising inward: treads cut into the mountain, a riser at each band's
            // inner edge (the levels in `pts`, the outermost first)
            const int nb = clampi((int)(R / 160), 6, 14);
            GroundEdit e; e.kind = GE_TERRACES; e.r = R; e.step = R / nb; e.fades = false; e.tone = ET_GLASS;
            double prev = -1e18;
            for (int b = 0; b < nb; b++) {
                const double rr = R - (b + 0.5) * e.step;
                double lo = 1e18; for (int m = 0; m < 16; m++) { const double a = m * TAU / 16; lo = std::min(lo, at(offsetUnit(top, std::sin(a) * rr, std::cos(a) * rr, Rm)).height); }
                lo = std::max(lo - 4, prev + 6);
                e.pts.push_back((float)lo); prev = lo;
            }
            e.level = e.pts[0]; e.depth = (e.pts.back() - e.pts[0]) / std::max(1, nb - 1);
            w.edits.push_back(e); w.level = e.level;
            continue;
        }
        // a site on the people's land: flat for the plain, the pillars, the sphere; a valley for the lattice
        Vec3 best; bool any = false; double bestS = -1e18;
        for (int tries = 0; tries < 24; tries++) {
            const int c = (int)(rng.uni() * G.cells);
            if (!pp->territory[c] || !G.land[c]) continue;
            const Vec3 u = offsetUnit(G.unitOf(c), rng.sym(8000), rng.sym(8000), Rm);
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, 150, down, h);
            const double sc = st == 3 ? -std::fabs(sl - 0.12) : -sl;
            if (sc > bestS) { bestS = sc; best = u; any = true; }
        }
        if (!any) continue;
        if (st == 0) {   // a plain made flat
            const double R = 3000 + 3000 * rng.uni();
            if (!clearOf(best, R)) continue;
            Work& w = add(WO_STRANGE, best, R + 100, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 0;
            double mean = at(best).height; for (int m = 0; m < 12; m++) { const double b = m * TAU / 12; mean += at(offsetUnit(best, std::sin(b) * R * 0.6, std::cos(b) * R * 0.6, Rm)).height; } mean /= 13;
            GroundEdit f; f.kind = GE_FLAT; f.r = R; f.width = 30; f.level = mean; f.tone = ET_GLASS; f.fades = false;
            w.edits.push_back(f); w.level = mean;
        } else if (st == 1) {   // pillars to the horizon: one every 400 m along a great circle, in stretches of three kilometres
            const double hd = rng.uni() * TAU, total = 24000 + 16000 * rng.uni(), segL = 3000;
            const Vec3 dir = normalize(offsetUnit(best, std::sin(hd) * 1000, std::cos(hd) * 1000, Rm) - best);
            const uint32_t group = (uint32_t)mix64(pseed ^ 0x7042AAULL);
            const int nSeg = (int)(total / segL);
            int placed = 0;
            for (int s = 0; s < nSeg; s++) {
                const double t0 = (s - nSeg * 0.5) * segL, t1 = t0 + segL;
                const Vec3 a = normalize(best * std::cos(t0 / Rm) + dir * std::sin(t0 / Rm)), b = normalize(best * std::cos(t1 / Rm) + dir * std::sin(t1 / Rm));
                const Vec3 m = slerpUnit(a, b, 0.5);
                if (planCoreAt(P, m, 400)) continue;
                Work& w = add(WO_STRANGE, m, segL * 0.5 + 60, seedOf(WO_STRANGE, 50 + s));
                w.age = AGE_FAR_FUTURE; w.style = 1; w.group = group; w.heard = s == nSeg / 2; w.heading = bearingOf(a, b); w.length = segL;
                placed++;
            }
            if (placed) k++;
        } else if (st == 3) {   // a lattice over the valley: its girders at the height of the valley's shoulders
            double down, h; grade(best, 150, down, h);
            const double hw = 300 + 300 * rng.uni(), hd = 200 + 200 * rng.uni();
            if (!clearOf(best, std::hypot(hw, hd))) continue;
            double top = -1e18; for (int m = 0; m < 8; m++) { const double b = m * TAU / 8; top = std::max(top, at(offsetUnit(best, std::sin(b) * hw, std::cos(b) * hw, Rm)).height); }
            Work& w = add(WO_STRANGE, best, std::hypot(hw, hd) + 60, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 3; w.heading = down + PI / 2; w.length = 2 * hw; w.width = 2 * hd; w.floor = h; w.crest = std::max(top, h + 60) + 20;
        } else {   // a sphere resting on its point
            const double R = 60 + 80 * rng.uni();
            if (!clearOf(best, R * 2)) continue;
            Work& w = add(WO_STRANGE, best, R * 2, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 4; w.height = R;
        }
    }
}

// ---- the lighthouses: on the headland nearest the harbour the history lit ----------------------------------------------------------------
void WB::lighthouses() {
    if (!H || age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC) return;
    Rng rng(pseed ^ 0x70440ULL);
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_LIGHTHOUSE || e.place < 0 || made[WO_LIGHTHOUSE] >= 3 || gone(e.place)) return;
        Vec3 u0; double r0; placeAt(e.place, u0, r0);
        Vec3 shore; double seaward;
        if (!shoreNear(u0, r0 + 400, r0 + 9000, rng, shore, seaward, true)) return;
        const Vec3 site = offsetUnit(shore, -std::sin(seaward) * 25, -std::cos(seaward) * 25, Rm);   // a little back from the edge
        if (!clearOf(site, 50)) return;
        Work& w = add(WO_LIGHTHOUSE, site, 50, seedOf(WO_LIGHTHOUSE, k++));
        w.age = clampi(age, AGE_MEDIEVAL, AGE_MODERN); w.event = i; w.year = e.year; w.place = e.place; int pa; placeAge(e.place, pa, w.before);
        if (H->places[e.place].plan >= 0) w.planRank = H->places[e.place].plan;
        w.heading = seaward; w.height = w.age == AGE_MEDIEVAL ? 22 + 10 * rng.uni() : 30 + 25 * rng.uni();
    });
}
