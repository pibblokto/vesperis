// E-10: a form's colliders (`formColliders`: the discs the explorer stops at), the forms stood on rather than walked into
// (`formWalkable`) and the drawn surface they are stood on (`formSupportAt`)
#include "forms_internal.h"

// ---- colliders ----------------------------------------------------------------------------------------------------------------

namespace {
void outlineDiscs(const std::vector<std::pair<double, double>>& poly, double rd, double y0, double y1, std::vector<FormDisc>& out) {   // discs along a closed outline (local a, b)
    const int n = (int)poly.size();
    for (int i = 0; i < n; i++) {
        auto a = poly[i], b = poly[(i + 1) % n];
        double L = std::hypot(b.first - a.first, b.second - a.second);
        int k = std::max(1, (int)std::ceil(L / (1.4 * rd)));
        for (int j = 0; j < k; j++) { double t = (double)j / k; out.push_back({a.first + (b.first - a.first) * t, a.second + (b.second - a.second) * t, rd, y0, y1}); }
    }
}
}   // namespace

// B-420: earth and rubble heaped on the ground (a barrow, a bank, a spoil heap, a heap of what fell) and the tiers of an amphitheatre or a
// stadium are stood on, not walked into: their sides are slopes and steps, and a collider at their foot was a wall where the ground only began
// to rise. The view stands the explorer on their drawn surface (`formSupportAt`) and stops it where that rises more than a step; no discs
bool formWalkable(const Form& e) {
    if (e.tilt > 0.3 || e.y0 > 0.01 || e.absolute) return false;
    if (e.kind == FK_SWEEP && std::lround(e.p[3]) > 0) return true;
    if (e.part != 8 && e.part != 3) return false;
    return e.kind == FK_DOME || e.kind == FK_PYRAMID || e.kind == FK_SWEEP;
}

namespace {
// B-420: a pyramid's outline at a height (local a, b), as `pyramidMesh` lays its corners: its sides (at most eight), its tiers' faces, its corners
// worn back where it is broken (its skirt upright under its foot)
void pyramidOutline(const Form& e, double y, std::vector<std::pair<double, double>>& poly) {
    const int n = e.sides >= 3 ? std::min(e.sides, 8) : 4;
    const int T = std::max(1, (int)std::lround(e.p[0]));
    const double top = clampd(e.p[1], 0, 1), rs = e.p[0] > 0 ? clampd(e.p[2], 0, 1) : 0.0, H = std::max(0.1, e.y1 - e.y0);
    const double kz = e.hz / std::max(1e-6, e.hx), R = e.hx / std::cos(PI / n), c0 = PI / n;
    const bool grounded = e.y0 < 0.01 && e.tilt == 0;
    const int i = clampi((int)std::floor((y - e.y0) / H * T), 0, T - 1);
    const double yb = e.y0 + H * i / T, yt = e.y0 + H * (i + 1) / T;
    (void)grounded;
    const double s0 = 1 - (1 - top) * i / T, s1 = 1 - (1 - top) * (i + 1) / T, sTop = s0 - (s0 - s1) * (1 - rs);
    const int lb = 2 * i, lt = rs > 0 ? 2 * i + 1 : 2 * i + 2;
    const double f = clampd((y - yb) / std::max(1e-6, yt - yb), 0, 1);
    poly.clear();
    for (int j = 0; j < n; j++) {
        const double th = c0 + TAU * j / n;
        double g0 = s0, g1 = sTop;
        if (e.broken > 0) { g0 *= 1 - 0.18 * e.broken * H01(e.seed, lb * 16 + j); g1 *= 1 - 0.18 * e.broken * H01(e.seed, lt * 16 + j); }
        const double g = g0 + (g1 - g0) * f;
        poly.push_back({R * g * std::cos(th), R * g * std::sin(th) * kz});
    }
}
}   // namespace

void formColliders(const Form& e, std::vector<FormDisc>& out) {
    if (formTop(e) < 0.9) return;   // A-03: what is lower than C-01's walls that block (0.9 m) is stepped over: a post's hole, a ring of stones, a stump
    if (formWalkable(e)) return;    // stood on (the view's `formSupportAt`)
    const double ch = std::cos(e.heading), sh = std::sin(e.heading);
    auto toS = [&](double a, double b, double& x, double& z) { x = e.x + ch * a + sh * b; z = e.z - sh * a + ch * b; };   // local (across, along) -> the settlement's metres
    auto push = [&](double a, double b, double r, double y0, double y1) { double x, z; toS(a, b, x, z); out.push_back({x, z, r, y0, y1}); };
    switch (e.kind) {
        case FK_BOX: {   // a standing piece as rows of discs along it
            if (e.part == 4) return;
            const bool alongLong = e.hz >= e.hx;
            const double L = alongLong ? e.hz : e.hx, W = alongLong ? e.hx : e.hz;   // its long and its short half extents
            if (std::fabs(e.tilt - PI / 2) < 0.2 && e.broken > 0) {   // lying flat and broken: step by step (its steps along its long side are layers
                                                                          // of it one over another where that side stood up, strips side by side where it lay across)
                const int steps = boxSteps(e);
                for (int i = 0; i < steps; i++) {
                    const double t0 = -L + 2 * L * i / steps, t1 = -L + 2 * L * (i + 1) / steps, top = boxStepTop(e, i);
                    double ya, yb, a0, a1;   // its span up, and across
                    if (alongLong) { ya = e.y0 + e.hz - t1; yb = e.y0 + e.hz - t0; a0 = -e.hx; a1 = e.hx; }
                    else { ya = e.y0; yb = e.y0 + 2 * e.hz; a0 = t0; a1 = t1; }
                    if (yb < 0.9 || top - e.y0 < 0.3) continue;
                    const double rr = clampd(std::min(0.5 * (a1 - a0), 0.5 * (top - e.y0)), 0.2, 0.6);
                    const int n = std::max(1, (int)std::ceil(0.5 * (top - e.y0) / rr)), m = std::max(1, (int)std::ceil(0.5 * (a1 - a0) / rr));
                    for (int k = 0; k < n; k++) {
                        const double b = n > 1 ? e.y0 + rr + (top - e.y0 - 2 * rr) * k / (n - 1) : 0.5 * (e.y0 + top);
                        for (int j = 0; j < m; j++) push(m > 1 ? a0 + rr + (a1 - a0 - 2 * rr) * j / (m - 1) : 0.5 * (a0 + a1), b, rr, ya, yb);
                    }
                }
                return;
            }
            if (e.tilt > 0.3) {   // lying (a fallen stone, a tower's top where it fell): along the heading from its foot as the mesh lays it, as high as it lies
                const double ct = std::cos(e.tilt), st = std::sin(e.tilt);
                double top = e.y1;
                if (e.broken > 0) { top = e.y0; for (int i = 0, n = boxSteps(e); i < n; i++) top = std::max(top, boxStepTop(e, i)); }
                double a0 = 1e18, a1 = -1e18, u0 = 1e18, u1 = -1e18;   // the tipped section's reach along the heading and up
                for (int q = 0; q < 4; q++) {
                    const double y = (q & 1) ? top : e.y0, b = (q & 2) ? e.hz : -e.hz, al = y * st + b * ct, up = y * ct - b * st;
                    a0 = std::min(a0, al); a1 = std::max(a1, al); u0 = std::min(u0, up); u1 = std::max(u1, up);
                }
                const double rise = u1 - u0;
                if (rise < 0.9) return;   // lying lower than a wall that blocks: stepped over
                const double rr = clampd(std::min(e.hx, 0.5 * (a1 - a0)), 0.2, 0.6);
                const int n = std::max(1, (int)std::ceil(0.5 * (a1 - a0) / rr)), m = std::max(1, (int)std::ceil(e.hx / rr));
                for (int i = 0; i < n; i++) {
                    const double b = n > 1 ? a0 + rr + (a1 - a0 - 2 * rr) * i / (n - 1) : 0.5 * (a0 + a1);
                    for (int j = 0; j < m; j++) push(m > 1 ? -e.hx + rr + (2 * e.hx - 2 * rr) * j / (m - 1) : 0.0, b, rr, e.y0, e.y0 + rise);
                }
                return;
            }
            if (e.y0 > 1.2 || e.y1 < 0.9) return;
            // rows along its long side across its short one; B-420: a broken wall's ragged top step by step as the mesh stands it, each step's
            // discs as tall as it stands (a step under a wall's height that blocks is stepped over); a leaning one (a battered wall, a ship's hull
            // aground) where it leans: each disc's span its own (a long hull's raised end over the explorer's head) and over its knee's place
            const double rr = clampd(W, 0.2, 0.6), ct = std::cos(e.tilt), st = std::sin(e.tilt);
            const int m = std::max(1, (int)std::ceil(W / rr)), steps = boxSteps(e);
            for (int i = 0; i < steps; i++) {
                const double t0 = -L + 2 * L * i / steps, t1 = -L + 2 * L * (i + 1) / steps, top = boxStepTop(e, i);
                if (top < 0.9) continue;
                const int n = std::max(1, (int)std::ceil(0.5 * (t1 - t0) / rr));
                for (int k = 0; k < n; k++) {
                    const double s = n > 1 ? t0 + rr + (t1 - t0 - 2 * rr) * k / (n - 1) : 0.5 * (t0 + t1);
                    for (int j = 0; j < m; j++) {
                        const double o = m > 1 ? -W + rr + (2 * W - 2 * rr) * j / (m - 1) : 0.0;
                        const double a = alongLong ? o : s, b = alongLong ? s : o;
                        if (e.tilt == 0) { push(a, b, rr, e.y0, top); continue; }
                        const double lo = e.y0 * ct - b * st, hi = top * ct - b * st;
                        if (hi < 0.9 || lo > 1.2) continue;
                        const int pieces = clampi((int)std::ceil((top - e.y0) / 2.0), 1, 20);   // up it in pieces, each where the lean has it
                        for (int q = 0; q < pieces; q++) {
                            const double ya = e.y0 + (top - e.y0) * q / pieces, yb = e.y0 + (top - e.y0) * (q + 1) / pieces;
                            push(a, b * ct + 0.5 * (ya + yb) * st, rr, ya * ct - b * st, yb * ct - b * st);
                        }
                    }
                }
            }
            return;
        }
        case FK_DOME: {   // its rim where it springs from the ground; a roof (C-01's on a rotunda's walls: part 2) leaves it to its walls
            if (e.y0 > 1.2 || e.part == 2) return;
            const int n = std::max(8, (int)std::ceil(TAU * e.hx / 1.2));
            for (int j = 0; j < n; j++) { double a = TAU * j / n; push(e.hx * std::cos(a), (e.hz > 0 ? e.hz : e.hx) * std::sin(a), 0.5, e.y0, e.y0 + 0.5 * (e.y1 - e.y0)); }
            return;
        }
        case FK_LATHE: case FK_BLOB: case FK_PYRAMID: {   // slices up the form: discs along each slice's outline
            if (e.tilt > 1.0) {   // lying: a row along its length, from its foot along the heading as the mesh lies (A-03: it was centred on the foot);
                                  // as long as it is after its break (a snapped shaft's top, a pyramid's top tiers), each disc where its section
                                  // is at the explorer's knee (a round body lying is narrower at the ground than its radius; a prism of three sides lies
                                  // on a corner, off its axis), tapering with its profile; a short fat one a row across it
                const double cut = e.broken > 0 ? (e.kind == FK_LATHE ? 1 - 0.85 * e.broken : (e.kind == FK_PYRAMID ? 1 - 0.65 * e.broken : 1.0)) : 1.0;
                const double H = std::max(1e-6, e.y1 - e.y0);
                const double jag = e.kind == FK_LATHE && e.broken > 0 ? std::min(0.25 * H * cut, 0.06 * H + 0.5 * e.hx) * e.broken : 0.0;   // its break's jags
                const double len = std::max(0.2, H * cut - jag), c0 = 0.5 * len * std::sin(e.tilt) - 0.5 * len;   // its foot's end along the heading
                const bool round = e.kind == FK_BLOB || (e.kind == FK_LATHE && e.sides < 3);
                const int n = e.kind == FK_PYRAMID ? (e.sides >= 3 ? std::min(e.sides, 8) : 4) : (round ? 16 : std::min(e.sides, 8));
                const double ca0 = round ? 0.0 : PI / n, scale = round ? 1.0 : 1.0 / std::cos(PI / n), kz = e.hz > 0 ? e.hz / std::max(1e-6, e.hx) : 1.0;
                auto corner = [&](double R, int j, double& a, double& v) { const double th = ca0 + TAU * j / n; a = R * scale * std::cos(th); v = -R * scale * std::sin(th) * kz; };   // across; up (its b turned down)
                double lowest = 1e18; { const double R0 = e.hx * formProfile(e, 0.0); for (int j = 0; j < n; j++) { double ca, cv; corner(R0, j, ca, cv); lowest = std::min(lowest, cv); } }
                const double lift = e.y0 - lowest;   // the axis's height: the mesh set down on its lowest corner (`formMesh`)
                auto section = [&](double sAt, double& aMid, double& half, double& lo, double& hi) {   // its chord at the knee sAt along it, and its span up
                    const double R = std::max(0.05, e.hx * formProfile(e, std::min(1.0, sAt / H))), kh = e.y0 + 0.9;
                    double aL = 1e18, aR = -1e18; lo = 1e18; hi = -1e18;
                    for (int j = 0; j < n; j++) {
                        double a0, v0, a1, v1; corner(R, j, a0, v0); corner(R, (j + 1) % n, a1, v1); v0 += lift; v1 += lift;
                        lo = std::min(lo, v0); hi = std::max(hi, v0);
                        if ((v0 - kh) * (v1 - kh) <= 0 && std::fabs(v1 - v0) > 1e-9) { const double t = (kh - v0) / (v1 - v0), aa = a0 + (a1 - a0) * t; aL = std::min(aL, aa); aR = std::max(aR, aa); }
                    }
                    if (aR < aL) for (int j = 0; j < n; j++) { double ca, cv; corner(R, j, ca, cv); aL = std::min(aL, ca); aR = std::max(aR, ca); }   // all of it over or under the knee
                    aMid = 0.5 * (aL + aR); half = std::max(0.05, 0.5 * (aR - aL));
                };
                double am, w0, lo0, hi0; section(0.0, am, w0, lo0, hi0);
                if (hi0 - lo0 < 0.9) return;   // lying lower than a wall that blocks: stepped over
                if (len < 2 * w0) {   // short and fat: across it
                    const double rr = std::max(0.2, 0.5 * len), half = std::max(0.0, w0 - rr);
                    const int m = std::max(1, (int)std::ceil(half / (0.7 * rr)));
                    for (int j = 0; j <= m; j++) push(am - half + 2 * half * j / m, c0 + 0.5 * len, rr, lo0, hi0);
                    return;
                }
                for (double sAt = w0; sAt < len; ) {
                    double aMid, w, lo, hi; section(sAt, aMid, w, lo, hi); w = std::max(0.2, w);
                    if (sAt + w > len + 0.01 && sAt > w0) { const double ww = std::max(0.2, std::min(w, 0.5 * (len - (sAt - w)))); section(len - ww, aMid, w, lo, hi); push(aMid, c0 + len - ww, std::min(w, ww), lo, hi); break; }
                    push(aMid, c0 + sAt, std::min(w, sAt), lo, hi);
                    sAt += std::max(0.2, 0.7 * w);
                }
                return;
            }
            // where the mesh breaks off (a pyramid's top tiers at 0.65 of the break, a shaft at 0.85), and the slices: the explorer's two
            // bands (to the knee and to 1.8 m over the foot), then one every eight metres or so up the rest, each with its narrowest outline (the
            // mesh never inside it), inset by the discs' radius; a small one a disc inside it
            const double H = e.y1 - e.y0;
            double yTop = e.y0 + H * (e.broken > 0 ? (e.kind == FK_PYRAMID ? 1 - 0.65 * e.broken : 1 - 0.85 * e.broken) : 1.0);
            if (e.kind == FK_LATHE && e.broken > 0) yTop -= 0.5 * std::min(0.25 * (yTop - e.y0), 0.06 * H + 0.5 * e.hx) * e.broken;   // its rim's jags, half way down
            const double lean = std::sin(e.tilt), up = std::cos(e.tilt);   // a leaning or tipped one's slices along the lean, lower by it
            double lift = 0;   // a tipped one (past 0.3) is set down on its lowest corner, as the mesh is (`formMesh`)
            if (e.tilt > 0.3) { const double hz = std::max(e.hx, e.hz); lift = e.y0 - std::min(e.y0 * up - hz * lean, e.y1 * up - hz * lean); }
            if (yTop - std::max(0.0, e.y0) < 0.3) return;
            std::vector<double> ys{e.y0};
            for (double band : {0.9, 1.8}) { const double y = std::max(e.y0, 0.0) + band; if (y > ys.back() + 0.2 && y < yTop - 0.3) ys.push_back(y); }
            { const double rest = yTop - ys.back(); const int k = clampi((int)std::ceil(rest / 8.0), 1, 16); const double y0 = ys.back(); for (int i = 1; i <= k; i++) ys.push_back(y0 + rest * i / k); }
            std::vector<std::pair<double, double>> poly;
            auto slicePush = [&](double a, double b, double r, double ya, double yb) { push(a, b, r, ya * up + lift, yb * up + lift); };   // a slice's span as it leans
            auto outline = [&](double ya, double yb, double apothem, int n) {   // the polygon in `poly` (its apothem given, n sides): inset and its discs
                const double rd = clampd(0.25 * apothem, 0.3, 1.5);
                if (apothem < 2 * rd) { slicePush(0, lean * 0.5 * (ya + yb), std::max(0.05, apothem), ya, yb); return; }   // a disc inside it
                {   // each side moved in by the discs' radius, the corners where the moved sides meet (worn corners leave the sides uneven)
                    const int m = (int)poly.size();
                    double area = 0; for (int i = 0; i < m; i++) { const auto& p0 = poly[i]; const auto& p1 = poly[(i + 1) % m]; area += p0.first * p1.second - p1.first * p0.second; }
                    const double sgn = area >= 0 ? 1.0 : -1.0;   // inward is left of a side for a counter-clockwise outline
                    std::vector<std::pair<double, double>> in(m);
                    for (int i = 0; i < m; i++) {
                        const auto& a0 = poly[(i + m - 1) % m]; const auto& a1 = poly[i]; const auto& b1 = poly[(i + 1) % m];
                        auto offsetLine = [&](std::pair<double, double> p, std::pair<double, double> q, double& px, double& pz, double& dx, double& dz) {
                            dx = q.first - p.first; dz = q.second - p.second; const double L = std::max(1e-9, std::hypot(dx, dz)); dx /= L; dz /= L;
                            px = p.first - sgn * dz * rd; pz = p.second + sgn * dx * rd;
                        };
                        double ax, az, adx, adz, bx, bz, bdx, bdz; offsetLine(a0, a1, ax, az, adx, adz); offsetLine(a1, b1, bx, bz, bdx, bdz);
                        const double den = adx * bdz - adz * bdx;
                        if (std::fabs(den) < 1e-9) { in[i] = {bx, bz}; continue; }
                        const double t = ((bx - ax) * bdz - (bz - az) * bdx) / den;
                        in[i] = {ax + adx * t, az + adz * t};
                    }
                    poly = in;
                }
                std::vector<FormDisc> d; outlineDiscs(poly, rd, ya, yb, d);
                for (const FormDisc& q : d) slicePush(q.x, q.z + lean * 0.5 * (ya + yb), q.r, q.y0, q.y1);
                (void)n;
            };
            for (size_t si = 0; si + 1 < ys.size(); si++) {
                const double ya = ys[si], yb = ys[si + 1];
                if (e.kind == FK_PYRAMID) {   // the mesh's own corners at the slice's top (a pyramid narrows upward)
                    pyramidOutline(e, yb, poly);
                    const int n = (int)poly.size();
                    double m = 1e18; for (auto& q : poly) m = std::min(m, std::hypot(q.first, q.second));
                    outline(ya, yb, m * std::cos(PI / std::max(3, n)), n);
                    continue;
                }
                double w = 1e18; for (int q = 0; q <= 2; q++) w = std::min(w, formProfile(e, (ya + (yb - ya) * q / 2 - e.y0) / std::max(1e-6, H)));
                if (e.kind == FK_BLOB) w *= 1 - 0.3 * clampd(e.p[1], 0, 1);   // a grown form's lumps: inside its hollows
                double ra = e.hx * w, rb = (e.hz > 0 ? e.hz : e.hx) * w;
                if (ra < 0.05) continue;
                if (e.kind == FK_LATHE && e.sides >= 3) {   // a prism: the mesh's faces (at most eight), its corners out from the profile's apothem
                    const int n = std::min(e.sides, 8);
                    poly.clear();
                    const double R = 1.0 / std::cos(PI / n);
                    for (int j = 0; j < n; j++) { double th = PI / n + TAU * j / n; poly.push_back({ra * R * std::cos(th), rb * R * std::sin(th)}); }
                    outline(ya, yb, std::min(ra, rb), n);
                } else if (std::fabs(ra - rb) < 0.05 * ra) slicePush(0, lean * 0.5 * (ya + yb), ra, ya, yb);
                else {   // an oval: a row of discs along its long axis
                    bool longA = ra > rb; double L = longA ? ra : rb, r = longA ? rb : ra;
                    int k = std::max(1, (int)std::ceil((L - r) / (0.7 * r)));
                    for (int i = 0; i <= k; i++) { double c = -(L - r) + 2 * (L - r) * i / std::max(1, k); double rr = r * std::sqrt(std::max(0.2, 1 - (c / L) * (c / L))); if (longA) slicePush(c, lean * 0.5 * (ya + yb), rr, ya, yb); else slicePush(0, c + lean * 0.5 * (ya + yb), rr, ya, yb); }
                }
            }
            return;
        }
        case FK_VAULT: {   // springing from the ground: its two flanks (and its closed ends), the hall inside free; not where a stretch fell
            if (e.y0 > 1.2) return;
            const double H = std::max(0.2, e.y1 - e.y0), th = e.p[0] > 0 ? e.p[0] : std::max(0.25, 0.04 * e.hx);
            // the flank where its shell is at the explorer's two bands (to the knee, to 1.8 m: a parabolic vault leans in fast), each
            // disc in the shell's thickness there
            auto halfAt = [&](double y) {   // the outer face's half span at a height over the springing
                const double t = clampd(y / H, 0, 1);
                if (e.sub == 2) return e.hx * std::sqrt(1 - t);
                if (e.sub == 1) { const double c = 0.5 * e.hx, rho = e.hx + c, apex = std::sqrt(rho * rho - c * c); const double yy = t * apex; return std::max(0.0, -c + std::sqrt(std::max(0.0, rho * rho - yy * yy))); }
                return e.hx * std::sqrt(std::max(0.0, 1 - t * t));
            };
            double g0 = 1e9, g1 = -1e9;
            if (e.broken > 0) { g0 = -e.hz + 2 * e.hz * (0.2 + 0.25 * H01(e.seed, 1)); g1 = std::min(e.hz * (e.broken > 0.85 ? 0.85 : 1.0), g0 + 2 * e.hz * 0.6 * e.broken); }   // as `vaultMesh` lays the gap
            const double rd = std::min(std::max(0.3, 0.5 * th + 0.1), std::max(0.1, e.hz));   // no longer than it is (a station's iron rib)
            const double bEnd = std::max(0.0, e.hz - rd);
            int k = std::max(1, (int)std::ceil(2 * bEnd / 0.7));
            for (int band = 0; band < 2; band++) {
                const double ya = band == 0 ? 0.0 : 0.9, yb = std::min(band == 0 ? 0.9 : 1.8, H);
                if (yb <= ya) break;
                const double a = halfAt(yb) - 0.5 * th;   // the shell's middle at the band's top (the narrowest)
                if (a < rd) break;
                for (int i = 0; i <= k; i++) { double b = -bEnd + 2 * bEnd * i / k; if (b > g0 - rd && b < g1 + rd) continue; push(a, b, rd, e.y0 + ya, e.y0 + yb); push(-a, b, rd, e.y0 + ya, e.y0 + yb); }
            }
            if (e.p[1] > 0) { int k2 = std::max(1, (int)std::ceil(2 * e.hx / 0.7)); for (int i = 0; i <= k2; i++) { double a = -e.hx + 2 * e.hx * i / k2; if (std::fabs(a) > halfAt(0.9)) continue; push(a, e.hz - rd, rd, e.y0, e.y1); push(a, -e.hz + rd, rd, e.y0, e.y1); } }
            return;
        }
        case FK_ARCH: {   // the piers, the openings free
            if (e.y0 > 1.2) return;
            const int N = std::max(1, (int)std::lround(e.p[0] > 0 ? e.p[0] : 1));
            const double W = 2 * e.hx / N, w = W * clampd(e.p[1] > 0 ? e.p[1] : 0.32, 0.08, 0.46);
            for (int j = 0; j <= N; j++) {   // pier j between bay j - 1 and bay j (the ends' piers to the wall's ends)
                double xa = j == 0 ? -e.hx : -e.hx + W * (j - 0.5) + w, xb = j == N ? e.hx : -e.hx + W * (j + 0.5) - w;
                if (xb - xa < 1e-6) continue;
                double top = e.y1;   // a snapped pier as tall as `archMesh` leaves it
                if (e.broken > 0 && N > 1 && H01(e.seed, 40 + j) < e.broken * 0.3) top = std::max(e.y0 + (e.y1 - e.y0) * (0.25 + 0.5 * H01(e.seed, 80 + j)), e.y0 + 0.5);
                const double rr = clampd(std::min(e.hz, 0.5 * (xb - xa)), 0.1, 0.6);   // no wider than the pier (a gate's thin piers)
                int n = std::max(1, (int)std::ceil((xb - xa) * 0.5 / rr));
                for (int i = 0; i < n; i++) {
                    double a = n > 1 ? xa + rr + (xb - xa - 2 * rr) * i / (n - 1) : 0.5 * (xa + xb);
                    int m = std::max(1, (int)std::ceil(e.hz / rr));
                    for (int q = 0; q < m; q++) { double b = m > 1 ? -e.hz + rr + (2 * e.hz - 2 * rr) * q / (m - 1) : 0; push(a, b, rr, e.y0, top); }
                }
            }
            return;
        }
        case FK_SHELL: {
            if (e.sub == 0) for (int sd = -1; sd <= 1; sd += 2) push(sd * e.hx * 0.92, -sd * e.hz * 0.92, 0.6, 0, std::max(1.5, e.y0 + 0.5));   // the low corners (on their posts)
            else if (e.y0 > 0.9) push(0, 0, std::max(0.3, 0.06 * e.hx), 0, e.y0);   // its mast as drawn (none under 0.2 m; under a wall's height stepped over)
            return;
        }
        case FK_TRUSS: {   // each leg's members from the foot up (the explorer's reach and over it), where they stand: not fallen, under the
                           // buckle, tipped with the mast as `trussMesh` builds it (a derrick's jib): the legs alone, the bracing between them open
            if (e.sub != 0) return;
            const int L = e.sides >= 3 ? std::min(e.sides, 4) : 4;
            const double H = std::max(0.5, e.y1 - e.y0), topShare = e.p[0] > 0 ? e.p[0] : 0.3, t = e.p[2] > 0 ? e.p[2] : clampd(0.04 * e.hx, 0.04, 0.5);
            const int K = e.p[1] > 0 ? (int)e.p[1] : clampi((int)std::lround(H / std::max(0.8, e.hx * (1 + topShare))), 2, 40);
            const int kb = e.broken > 0 ? std::max(1, (int)(K * (1 - 0.7 * e.broken))) : K + 1;
            const double ct = std::cos(e.tilt), st = std::sin(e.tilt), reach = std::max(6.0, 0.25 * H);
            auto gone = [&](int id) { return e.broken > 0 && H01(e.seed, id) < 0.3 * e.broken; };
            auto pt = [&](int i, int k, double& a, double& al, double& up) {   // leg i's point k: across, along and up over the foot (tipped)
                const double shk = 1 - (1 - topShare) * (double)k / K, r = e.hx / std::cos(PI / L) * shk, th = PI / L + TAU * i / L;
                const double b = r * std::sin(th), y = e.y0 + H * k / K;
                a = r * std::cos(th); al = y * st + b * ct; up = y * ct - b * st;
            };
            double lift = 0;   // a tipped mast is set down on its lowest point, as the mesh is (`formMesh`)
            if (e.tilt > 0.3) { double lo = 1e18; for (int i = 0; i < L; i++) for (int k = 0; k <= K; k++) { double a, al, up; pt(i, k, a, al, up); lo = std::min(lo, up); } lift = e.y0 - lo; }
            for (int i = 0; i < L; i++)
                for (int k = 0; k < K && k < kb; k++) {
                    if (gone(k * 64 + i)) break;   // the leg ends where a member of it fell
                    double a0, b0, u0, a1, b1, u1; pt(i, k, a0, b0, u0); pt(i, k + 1, a1, b1, u1);
                    u0 += lift; u1 += lift;
                    if (std::min(u0, u1) > e.y0 + reach) break;
                    // a leaning leg (a squat stump's, a jib's) in short steps, each disc only as high as the leg stays inside it (a disc
                    // as tall as a 0.5 m step stood a hand off the leg at its top and its foot); an upright leg's padded by its thickness
                    const double hyp = std::hypot(a1 - a0, b1 - b0), lean = hyp / std::max(1e-6, std::fabs(u1 - u0)), pad = t * std::max(0.0, 1 - 2 * lean);
                    const int n = std::max(1, (int)std::ceil(hyp / (lean > 0.08 ? 0.18 : 0.5)));
                    for (int q = 0; q < n; q++) {
                        const double f0 = (double)q / n, f1 = (double)(q + 1) / n, fm = 0.5 * (f0 + f1);
                        const double ya = u0 + (u1 - u0) * f0, yb = u0 + (u1 - u0) * f1;
                        push(a0 + (a1 - a0) * fm, b0 + (b1 - b0) * fm, t + 0.12, std::min(ya, yb) - pad, std::max(ya, yb) + pad);
                    }
                }
            return;
        }
        case FK_PLATE: {   // discs along its outline (a plate as tall as a wall: the instruments' are lower, stepped over)
            double a[32], b[32]; formPlateOutline(e, 32, a, b);
            const double rr = clampd(0.5 * std::min(e.hx, e.hz), 0.2, 0.6), top = e.y1 + std::max(0.0, e.p[3]);
            for (int i = 0; i < 32; i++) {
                const int j = (i + 1) % 32;
                const double len = std::hypot(a[j] - a[i], b[j] - b[i]);
                const int n = std::max(1, (int)std::ceil(len / rr));
                for (int k = 0; k < n; k++) { const double t = (double)k / n; push(a[i] + (a[j] - a[i]) * t, b[i] + (b[j] - b[i]) * t, rr, e.y0, top); }
            }
            return;
        }
        case FK_FENCE: {   // discs along it where its pales stand (a stretch of them fallen lets the explorer through, a pale gone does not)
            if (e.part == 4 || e.tilt > 0.3) return;
            const int n = fencePales(e);
            const double sp = 2 * e.hz / n, rr = clampd(e.hx + 0.08, 0.12, 0.25);
            const int q = std::max(1, (int)std::ceil(sp / 0.3));
            for (int k = 0; k < n; k++) {
                if (!fenceStanding(e, k)) continue;
                for (int j = 0; j < q; j++) push(0, -e.hz + k * sp + (j + 0.5) * sp / q, rr, e.y0, e.y1);
            }
            return;
        }
        case FK_TORUS: {   // the tube where it lies (a broken ring's missing sector left out, as `torusMesh` lays it)
            const double R = e.hx, r = e.p[0] > 0 ? e.p[0] : 0.15 * e.hx;
            double gap0 = 0, gap1 = -1;
            if (e.broken > 0) { gap0 = TAU * H01(e.seed, 1); gap1 = gap0 + 1.8 * e.broken; }
            auto inGap = [&](double u) { if (gap1 < gap0) return false; double d = std::fmod(u - gap0 + 4 * TAU, TAU); return d < gap1 - gap0; };
            if (e.tilt < 0.3) {   // lying flat, as wide as its tube is at the explorer's knee
                const double k = std::min(r, 0.9), w = std::max(0.05, std::sqrt(std::max(0.0, r * r - (r - k) * (r - k))));
                int n = std::max(8, (int)std::ceil(TAU * R / (1.4 * w))); for (int j = 0; j < n; j++) { double a = TAU * j / n; if (!inGap(a)) push(R * std::cos(a), R * std::sin(a), w, e.y0, e.y0 + 2 * r); }
            }
            else {   // standing on its rim: the tube where it is across the ring's plane, its lower arc and its upper (each with the tube's
                     // height there: the hole between them free, and the lower arc's ends not walls to the ground)
                const double cy = e.y0 + R + r, rr = std::max(r, 0.05), ends = std::max(0.0, R + r - rr);
                const int n = std::max(2, (int)std::ceil(2 * ends / (0.7 * rr)));
                auto outerAt = [&](double x) { return std::sqrt(std::max(0.0, (R + r) * (R + r) - x * x)); };
                auto innerAt = [&](double x) { return std::fabs(x) >= R - r ? 0.0 : std::sqrt(std::max(0.0, (R - r) * (R - r) - x * x)); };
                for (int i = 0; i <= n; i++) {
                    const double a = -ends + 2 * ends * i / n;
                    // each disc's span the tube's over the whole of its width (where the arc curves the tube's height moves across it)
                    double oMin = 1e18, iMax = 0; for (int q = -1; q <= 1; q++) { const double x = clampd(a + q * rr, -(R + r), R + r); oMin = std::min(oMin, outerAt(x)); iMax = std::max(iMax, innerAt(x)); }
                    if (oMin <= iMax) continue;
                    if (std::fabs(a) >= R - r && iMax <= 0) { if (!inGap(std::acos(clampd(a / R, -1, 1)))) push(a, 0, rr, cy - oMin, cy + oMin); continue; }
                    if (!inGap(std::acos(clampd(a / R, -1, 1)))) push(a, 0, rr, cy - oMin, cy - iMax);   // the lower arc
                    if (!inGap(TAU - std::acos(clampd(a / R, -1, 1)))) push(a, 0, rr, cy + iMax, cy + oMin);   // the upper
                }
            }
            return;
        }
        case FK_SWEEP: {
            if (e.y0 > 1.2) return;
            const double H = e.y1 - e.y0, w0 = e.p[1] > 0 ? e.p[1] : 0.6, w1 = e.p[2] > 0 ? std::min(e.p[2], w0) : w0, rd = std::min(0.6, std::max(0.2, w0));
            auto along = [&](double t, double off, double& a, double& b) {
                if (e.sub == 0) { double ang = (e.p[0] > 0 ? std::min(e.p[0], TAU) : TAU) * t; a = (e.hx + off) * std::sin(ang); b = (e.hx + off) * std::cos(ang); }
                else { a = off; b = -e.hz + 2 * e.hz * t; }
            };
            double len = e.sub == 0 ? e.hx * (e.p[0] > 0 ? std::min(e.p[0], TAU) : TAU) : 2 * e.hz;
            const int P = clampi((int)std::ceil(len / 6.0), e.sub == 0 ? 8 : 1, 64);   // the near mesh's pieces (a broken one's gaps and ragged tops, as `sweepMesh` lays them)
            // in bands up its height (the explorer's two, to the knee and to 1.8 m, then two metres at a time), each with its two faces where
            // they are at the band's top (a battered wall's face draws in as it rises: a disc at its foot stood out from it higher up, one at its
            // top let the feet into its base); a thin section a disc in its middle
            auto faceAt = [&](double y) { const double f = clampd((y - e.y0) / std::max(1e-6, H), 0, 1); return w0 - f * (w0 - w1); };   // its face's offset at a height
            const bool closed = e.sub == 0 && (e.p[0] <= 0 || e.p[0] >= TAU - 1e-6);
            int n = std::max(1, (int)std::ceil(len / (1.4 * rd)));
            const double tIn = closed ? 0.0 : std::min(0.5, rd / std::max(len, 1e-6));   // an open one's end discs inside its ends (they stood out past them)
            for (int i = 0; i <= n; i++) {
                const double t = tIn + (1 - 2 * tIn) * i / n;
                const int piece = std::min(P - 1, (int)(t * P));
                if (e.broken > 0 && H01(e.seed, piece) < 0.35 * e.broken) continue;
                const double top = e.y1 - (e.broken > 0 ? H * 0.6 * e.broken * H01(e.seed, 100 + piece) : 0.0);
                if (top - std::max(e.y0, 0.0) < 0.9) continue;   // lower than a wall that blocks: stepped over
                const double base = std::max(e.y0, 0.0);
                for (double ya = e.y0; ya < top - 1e-6; ) {
                    const double yb = std::min(top, ya < base + 0.85 ? base + 0.9 : (ya < base + 1.75 ? base + 1.8 : ya + 2.0));
                    const double f = faceAt(yb), r = std::min(rd, std::max(0.05, f));
                    double a, bb;
                    if (w0 <= 0.7 || f - r < 0.05) { along(t, 0, a, bb); push(a, bb, std::max(0.05, f), ya, yb); }
                    else {
                        for (int sd = -1; sd <= 1; sd += 2) { along(t, sd * (f - r), a, bb); push(a, bb, r, ya, yb); }
                        if (!closed && (i == 0 || i == n)) {   // across the ends of a wide section
                            const int m = std::max(1, (int)std::ceil((f - r) / (0.7 * r)));
                            for (int q = -m + 1; q < m; q++) { along(t, (f - r) * q / m, a, bb); push(a, bb, r, ya, yb); }
                        }
                    }
                    ya = yb;
                }
            }
            return;
        }
        default: return;
    }
}

// B-420: where a form's drawn surface holds the explorer up at a point (x, z of the mesh's own metres): the highest face looking upward over
// the point at or under `ceiling`, -1e18 where none
double formSupportAt(const FormMesh& m, double x, double z, double ceiling) {
    double best = -1e18;
    for (const FormMesh::Poly& P : m.polys) {
        const FormMesh::V* v = &m.v[P.first];
        Vec3 nn(0, 0, 0);
        for (int k = 0; k < P.n; k++) { const Vec3& a = v[k].p; const Vec3& b = v[(k + 1) % P.n].p; nn.x += (a.y - b.y) * (a.z + b.z); nn.y += (a.z - b.z) * (a.x + b.x); nn.z += (a.x - b.x) * (a.y + b.y); }
        const double ln = std::sqrt(nn.x * nn.x + nn.y * nn.y + nn.z * nn.z);
        if (ln < 1e-12 || std::fabs(nn.y) < 0.2 * ln) continue;   // a wall, a riser
        bool pos = false, neg = false;   // inside the polygon's footprint: the point on one side of every edge
        for (int k = 0; k < P.n && !(pos && neg); k++) {
            const Vec3& a = v[k].p; const Vec3& b = v[(k + 1) % P.n].p;
            const double c = (b.x - a.x) * (z - a.z) - (b.z - a.z) * (x - a.x);
            if (c > 1e-9) pos = true; else if (c < -1e-9) neg = true;
        }
        if (pos && neg) continue;
        const double y = v[0].p.y - (nn.x * (x - v[0].p.x) + nn.z * (z - v[0].p.z)) / nn.y;
        if (y <= ceiling && y > best) best = y;
    }
    return best;
}
