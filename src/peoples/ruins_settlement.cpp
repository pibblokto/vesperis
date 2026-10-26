// E-09: a settlement laid out (`layoutSettlement`: its class's plan, its buildings by the people's ways, its monument) and the
// features every age built round one (C-08's harbours, terraces, cisterns, windbreaks and cairns; T-02's reservoirs)
#include "ruins_layout.h"

namespace {
// a wall segment of half length `half` across heading `h` (its axis along x') is clear along its whole length (nine points)
bool segmentClear(const RuinSpec& r, double x, double z, double h, double half, double margin) {
    double ax = std::cos(h), az = -std::sin(h);
    for (int k = -4; k <= 4; k++) { double t = half * k / 4.0; if (!clearOfBuildings(r, x + ax * t, z + az * t, margin)) return false; }
    return true;
}
// A-03: the features each age built: a prehistoric people the cairns; a medieval one all of them; an industrial one no cairns; a modern
// one its harbours and terraces; the futuristic and far-future ages none. T-02: an industrial and a modern people's reservoirs
enum { FT_HARBOUR = 1, FT_TERRACES = 2, FT_CISTERN = 4, FT_WINDWALL = 8, FT_CAIRNS = 16, FT_RESERVOIR = 32 };
const int AGE_FEATURES[AGE_COUNT] = {FT_CAIRNS, 31, FT_HARBOUR | FT_TERRACES | FT_WINDWALL | FT_RESERVOIR, FT_HARBOUR | FT_TERRACES | FT_RESERVOIR, 0, 0};
}   // namespace
void addFeatures(Layout& L, RuinSpec& r, const Culture& c) {
    Rng& rng = L.rng;
    const SiteRead& st = r.site;
    const double R = r.size;
    const int ft = AGE_FEATURES[clampi(c.age, 0, AGE_COUNT - 1)];
    bool big = r.sclass == SC_VILLAGE || townOrGreater(r.sclass);
    // the harbour (T-02: by the age: C-08's quay along the shore and its mole; an industrial people's dock, its basin between two piers, its
    // cranes on the edge and its warehouse behind; a modern people's container port, its apron, its gantries over the berth, its stacks of
    // containers on the land behind; a city's berths side by side along its shore)
    if ((ft & FT_HARBOUR) && st.shoreDist < 500 + R && (big || rng.chance(0.5)) && r.sclass != SC_MONUMENT) {
        const double nx = std::sin(st.shoreDir), nz = std::cos(st.shoreDir);    // toward the water
        const double ax = std::cos(st.shoreDir), az = -std::sin(st.shoreDir);   // along the shore (the width of a work facing the water)
        const bool city = r.sclass == SC_CITY || r.sclass == SC_CAPITAL;
        const int age = c.age;
        const int berths = city ? (age == AGE_MODERN ? 2 : 3) : 1;
        const double q = st.shoreDist - 3.0;
        for (int k = 0; k < berths; k++) {
            const double hw = age == AGE_MODERN ? 55 + 45 * rng.uni() : (age == AGE_INDUSTRIAL ? 28 + 18 * rng.uni() : 10 + 12 * rng.uni());
            const double hd = age == AGE_MODERN ? 16 + 10 * rng.uni() : (age == AGE_INDUSTRIAL ? 5.0 : 2.8);
            const double off = berths > 1 ? (k - (berths - 1) * 0.5) * (2 * hw + 40) : 0;
            const double cx = nx * q + ax * off, cz = nz * q + az * off;
            if (!segmentClear(r, cx, cz, st.shoreDir, hw, hd + 1.5) || !L.circleOffStreets(cx - nx * hd, cz - nz * hd, 0.5)) continue;   // clear of the houses along its length
            if (!L.add(BK_QUAY, cx, cz, st.shoreDir, hw, hd, 1.2, 0.5, false)) continue;
            r.buildings.back().district = DI_HARBOUR;
            r.harbour = true; r.harbourKind = age <= AGE_MEDIEVAL ? 0 : (age == AGE_INDUSTRIAL ? 1 : 2);
            const size_t first = r.buildings.size();
            if (age <= AGE_MEDIEVAL) {   // C-08's mole out from an end, a warehouse behind
                const double end = rng.chance(0.5) ? 1 : -1, len = 12 + 12 * rng.uni();
                const double ex = cx + ax * end * (hw - 1.5), ez = cz + az * end * (hw - 1.5);
                L.add(BK_MOLE, ex + nx * (hd + len * 0.5), ez + nz * (hd + len * 0.5), st.shoreDir, 1.0, len * 0.5, 1.6, 0.9, false);
                L.add(BK_HALL, cx - nx * (hd + 7.5), cz - nz * (hd + 7.5), st.shoreDir, 4.5 + 1.5 * rng.uni(), 5.5 + 1.5 * rng.uni(), 4 + 1.5 * rng.uni(), 0.6);
            } else if (age == AGE_INDUSTRIAL) {   // the dock: two piers from its ends (the basin between), its cranes, the warehouse behind
                const double len = 30 + 30 * rng.uni();
                for (int s2 = -1; s2 <= 1; s2 += 2) L.add(BK_MOLE, cx + ax * s2 * (hw - 2) + nx * (hd + len * 0.5), cz + az * s2 * (hw - 2) + nz * (hd + len * 0.5), st.shoreDir, 2.0, len * 0.5, 1.8, 1.0, false);
                const int cranes = 2 + rng.irange(2);
                for (int j = 0; j < cranes; j++) { const double u = -hw + 2 * hw * (j + 0.5) / cranes; L.add(BK_CRANE, cx + ax * u - nx * hd * 0.3, cz + az * u - nz * hd * 0.3, st.shoreDir, 2.2, 2.2, 14 + 6 * rng.uni(), 0.5, false, 0); }
                L.add(BK_HALL, cx - nx * (hd + 12), cz - nz * (hd + 12), st.shoreDir, hw * 0.7, 8 + 3 * rng.uni(), 8 + 3 * rng.uni(), 0.7);
            } else {   // the container port: the gantries on the apron's edge, the stacks behind on the land
                const int cranes = 3 + rng.irange(3);
                for (int j = 0; j < cranes; j++) { const double u = -hw + 2 * hw * (j + 0.5) / cranes; L.add(BK_CRANE, cx + ax * u + nx * hd * 0.1, cz + az * u + nz * hd * 0.1, st.shoreDir, 9 + 3 * rng.uni(), 9, 30 + 12 * rng.uni(), 0.5, false, 1); }
                const int rows = 2 + rng.irange(2), cols = std::max(1, (int)(hw / 40));
                for (int i = 0; i < cols; i++)
                    for (int j = 0; j < rows; j++) {
                        const double u = -hw + 2 * hw * (i + 0.5) / cols, v = -(hd + 14 + j * 26);
                        L.add(BK_CONTAINERS, cx + ax * u + nx * v, cz + az * u + nz * v, st.shoreDir, std::min(32.0, hw / cols - 4), 9, 10, 0.5);
                    }
            }
            for (size_t i = first; i < r.buildings.size(); i++) r.buildings[i].district = DI_HARBOUR;
        }
    }
    // a reservoir upslope of an industrial or a modern town (on a desert where there is room; on a living world a town's on a slope): a
    // basin of masonry in its bank, a round tank of concrete
    if ((ft & FT_RESERVOIR) && r.sclass != SC_MONUMENT && r.sclass != SC_HAMLET && r.sclass != SC_WORKS &&
        (st.desert ? rng.chance(big ? 0.8 : 0.4) : (townOrGreater(r.sclass) && st.slope > 0.02 && rng.chance(0.5)))) {
        const double head = st.slope > 0.02 ? st.downhill + PI : rng.uni() * TAU;
        const bool round = c.age == AGE_MODERN;
        for (int tries = 0; tries < 8 && !r.reservoir; tries++) {
            const double rad = 18 + 14 * rng.uni(), hw = round ? rad : 16 + 12 * rng.uni(), hd = round ? rad : 12 + 8 * rng.uni();
            const double a = head + rng.sym(0.6), rr = R + std::max(hw, hd) + 14 + 24 * rng.uni();
            if (L.add(BK_RESERVOIR, std::sin(a) * rr, std::cos(a) * rr, a, hw, hd, round ? 7 + 3 * rng.uni() : 4 + 2 * rng.uni(), 0.8, true, round ? 1 : 0)) r.reservoir = true;
        }
    }
    // the terraces
    const bool terracer = c.genome.of(GV_LAND) == LD_TERRACED && c.age <= AGE_MODERN;   // a people of terraces terraced any slope, in any age
    if (((ft & FT_TERRACES) || terracer) && st.slope > (terracer ? 0.02 : 0.06) && r.sclass != SC_MONUMENT) {
        double step = clampd(1.4 / st.slope, 8, 30), h = 1.3 + 0.6 * rng.uni();
        double dx = std::sin(st.downhill), dz = std::cos(st.downhill), ax = std::cos(st.downhill), az = -std::sin(st.downhill);
        const double Rt = std::min(R, 80.0);   // the terraces of the gardens round a village's or a town's middle (a wider town's would run to hundreds)
        double d0 = -std::floor(Rt / step) * step + step * rng.uni();
        int segs = 0;
        for (double d = d0; d < Rt; d += step) {
            double w = std::sqrt(std::max(0.0, Rt * Rt - d * d)) * 0.9;
            for (double tt = -w + 3; tt < w - 3; tt += 6.2) {
                double x = dx * d + ax * tt, z = dz * d + az * tt;
                if (!segmentClear(r, x, z, st.downhill, 3.0, 1.2) || !L.offStreets(x, z, st.downhill, 3.0, 0.4)) continue;   // the whole segment, not only its middle; T-02: off the streets
                if (L.add(BK_TERRACE, x, z, st.downhill, 3.0, 0.4, h, 0.4, false)) segs++;
            }
        }
        r.terraced = segs > 0;
    }
    // a desert's cistern and its wind wall
    if (st.desert && r.sclass != SC_MONUMENT) {
        if ((ft & FT_CISTERN) && rng.chance(big ? 0.8 : 0.4)) {
            double head = st.slope > 0.02 ? st.downhill + PI : rng.uni() * TAU;
            for (int tries = 0; tries < 6 && !r.cistern; tries++) {
                double a = head + rng.sym(0.5), rr = R * (0.75 + 0.2 * rng.uni()), rad = 2.5 + 1.5 * rng.uni();
                if (L.add(BK_CISTERN, std::sin(a) * rr, std::cos(a) * rr, a, rad, rad, 2.0, 0.7)) r.cistern = true;
            }
        }
        if ((ft & FT_WINDWALL) && rng.chance(big ? 0.7 : 0.35)) {
            double rw = R + 14, hWall = 2.2 + 1.0 * rng.uni();
            int segs = std::max(4, (int)(rw * 1.9 / 7.4)), placed = 0;
            for (int k = 0; k < segs; k++) {
                double a = st.wind + (-0.95 + 1.9 * (k + 0.5) / segs);
                if (rng.chance(0.15)) continue;   // fallen
                if (!L.offStreets(std::sin(a) * rw, std::cos(a) * rw, a + PI / 2, 3.5, 0.5)) continue;   // a gap where a street goes out
                if (L.add(BK_WINDWALL, std::sin(a) * rw, std::cos(a) * rw, a + PI / 2, 3.5, 0.5, hWall, 0.5, false)) placed++;
            }
            r.windwall = placed > 0;
        }
    }
    // the cairns on the ridge
    if ((ft & FT_CAIRNS) && st.relief > 0.45 && rng.chance(0.7)) {
        int n = 3 + rng.irange(4), placed = 0;
        for (int k = 0; k < n; k++) {
            double d = R + 12 + 14 * k + rng.sym(3), a = st.ridgeDir + rng.sym(0.12);
            if (!L.offStreets(std::sin(a) * d, std::cos(a) * d, a, 1.1, 1.1)) continue;
            if (L.add(BK_CAIRN, std::sin(a) * d, std::cos(a) * d, a, 1.1, 1.1, 2.1, 0.5, false)) placed++;
        }
        r.cairns = placed > 0;
    }
}

void layoutSettlement(RuinSpec& r, const Culture& c, const BodyGen& g, const TownHist* th) {
    Layout L(r, c);
    Rng& rng = L.rng;
    const Genome& G = c.genome;
    int n = 1; double R = 12; MonPick mon;
    settlementSize(r, rng, n, R, c, &mon);
    r.size = R;
    r.site = readSite(g, r);   // C-08: the terrain round it, read before anything is placed (no draw of the layout's stream)
    L.luckS = rng.sym(0.15);   // C-01's settlement's decay draw: its fortune now
    // a village, a town or a city of a medieval, an industrial or a modern people (or of a place left in one of those ages): its plan by
    // its ages (`layoutTown`); a camp, a hamlet, a futuristic people's great places and a lone monument as before
    {
        TownHist own; own.ages.push_back(c.age);
        const TownHist& t = th && !th->ages.empty() ? *th : own;
        r.before = t.before; r.founded = t.founded;
        const int townAge = t.ages.back();
        if ((r.sclass == SC_VILLAGE || townOrGreater(r.sclass)) && townAge >= AGE_MEDIEVAL && townAge <= AGE_MODERN) {
            layoutTown(r, c, g, n, R, t, L.luckS);
            return;
        }
    }
    const double h0 = r.heading, S = ageScale(c.age);
    const double fx = std::sin(h0), fz = std::cos(h0), sx = std::cos(h0), sz = -std::sin(h0);   // along the heading, and across it
    // the people's edge (a wall, a bank and its ditch, nothing) round its towns, and a village now and then, to the industrial age
    const int edge = G.of(GV_EDGE), streets = G.of(GV_STREETS), pub = G.of(GV_PUBLIC), links = G.of(GV_LINKS), land = G.of(GV_LAND);
    r.walled = c.age <= AGE_INDUSTRIAL && edge != ED_NOTHING && r.sclass >= SC_VILLAGE && r.sclass != SC_MONUMENT && rng.chance(townOrGreater(r.sclass) ? 0.85 : 0.3);
    const double towerShare = 0.5 + 1.5 * G.at(GV_HEIGHT);   // how many towers the people raised
    // a work of the industrial or modern age beside a hamlet or a village (a pit's headframe, a mill; tanks, a mast)
    auto work = [&](double x, double z, double head) {
        if (c.age == AGE_INDUSTRIAL) {
            if (r.site.relief > 0.15 && rng.chance(0.5)) return L.add(BK_HEADFRAME, x, z, head, 20 + 6 * rng.uni(), 16 + 4 * rng.uni(), 20 + 8 * rng.uni(), 0.5);
            return L.tower(x, z, head);
        }
        if (c.age == AGE_MODERN) {
            if (rng.chance(0.5)) return L.add(BK_TANKS, x, z, head, 18 + 10 * rng.uni(), 9 + 4 * rng.uni(), 15 + 10 * rng.uni(), 0.5);
            return L.add(BK_MAST, x, z, head, 3, 3, (80 + 100 * rng.uni()) * L.towerK, 0.5);
        }
        return false;
    };
    // the focal accent before the centre (a town's, and a village's half the time), where the centre does not carry it
    auto focusBefore = [&](double front, double scale) {
        if (r.plan != SP_GRID && c.age < AGE_FUTURISTIC)   // the people's monuments flanking the centre's front (the grid's are its plaza's)
            for (int side = -1; side <= 1; side += 2) L.stela(side * 4.5 * sx + fx * (front + 3), side * 4.5 * sz + fz * (front + 3), h0, 3.0 + 2.0 * rng.uni(), true);
        if (L.focusCarried()) return false;
        for (int k = 0; k < 4; k++) {
            const double d = front + 6 + k * 7;
            if (L.focus(fx * d, fz * d, h0 + PI, scale)) return true;
        }
        return false;
    };
    switch (r.plan) {
        case SP_LONE: {   // one monument alone, the age's
            const int st = mon.kind == BK_FOCUS || mon.kind == BK_STELA ? mon.style : -1;
            if (mon.count == 2) { L.add(mon.kind, -2.5 - mon.hw, 0, h0, mon.hw, mon.hd, mon.hWall, 0.3, false, st); L.add(mon.kind, 2.5 + mon.hw, 0, h0, mon.hw, mon.hd, mon.hWall * (0.75 + 0.5 * rng.uni()), 0.3, false, st); }
            else L.add(mon.kind, 0, 0, h0, mon.hw, mon.hd, mon.hWall, mon.kind == BK_TOWER ? 0.7 : 0.5, false, st);
            r.size = std::max(r.size, 12.0);
            break;
        }
        case SP_CLUSTER: {   // houses scattered (A-04: an organic people's towns and villages round their centre)
            if (r.sclass != SC_HAMLET) { L.centre(0, 0, h0, townOrGreater(r.sclass), false); const Building& cb = r.buildings.back(); focusBefore(cb.hd + 2, townOrGreater(r.sclass) ? 1.0 : 0.6); }
            const int triesMax = r.sclass == SC_CITY || r.sclass == SC_CAPITAL ? 8 : 20;   // a city's core is full sooner
            for (int k = 0, placed = 0; k < n && placed < n; k++)
                for (int tries = 0; tries < triesMax; tries++) {
                    double a = rng.uni() * TAU, rr = R * 0.85 * std::sqrt(rng.uni());
                    const int p = L.dwellingSpot(std::sin(a) * rr, std::cos(a) * rr, rng.uni() * TAU);
                    if (p) { placed += p; break; }
                }
            if (c.age >= AGE_INDUSTRIAL && rng.chance(0.45)) { double a = rng.uni() * TAU; work(std::sin(a) * (R + 12), std::cos(a) * (R + 12), a + PI); }
            break;
        }
        case SP_STREET: {
            // a street through the centre along the heading (A-04: along a line, on the contour, branching: its side streets); the dwellings
            // on both sides, their fronts to it; a town's centre on its middle
            if (townOrGreater(r.sclass)) { L.centre(sx * (16 + 4 * S), sz * (16 + 4 * S), h0 - PI / 2, true, true); if (!r.buildings.empty()) focusBefore(r.buildings.back().hd, 0.8); }
            const bool branching = streets == ST_BRANCHING;
            const bool arcades = links == LK_ARCADES, colonnades = links == LK_COLONNADES;
            double t = -R * 0.9;
            int placed = 0, step = 0;
            while (t < R * 0.9 && placed < n) {
                for (int side = -1; side <= 1; side += 2) {
                    if (placed >= n) break;
                    if (rng.chance(pub == PU_NONE ? 0.08 : 0.2)) continue;   // a gap
                    double off = (7.5 + 1.5 * rng.uni()) * S;
                    L.arcadeNext = arcades && std::fabs(t) < R * 0.4;
                    placed += L.dwellingSpot(fx * t + sx * side * off, fz * t + sz * side * off, h0 - side * PI / 2);
                    L.arcadeNext = false;
                    if (colonnades && std::fabs(t) < R * 0.35 && step % 2 == 0) L.add(BK_COLONNADE, fx * t + sx * side * 3.2 * S, fz * t + sz * side * 3.2 * S, h0, 0.5, 4.5 * S, 3.5, 0.4, true);
                }
                if (branching && step % 3 == 1 && placed < n - 2) {   // a side street, its houses along it
                    const int side = step % 2 ? 1 : -1;
                    for (int q = 1; q <= 3 && placed < n; q++) {
                        const double d = (12 + 11 * q) * S, cxs = fx * t + sx * side * d, czs = fz * t + sz * side * d;
                        for (int w = -1; w <= 1; w += 2) placed += L.dwellingSpot(cxs + fx * w * 7.5 * S, czs + fz * w * 7.5 * S, w > 0 ? h0 + PI : h0);
                    }
                }
                t += (9.0 + 4.0 * rng.uni()) * S;
                step++;
            }
            if (r.sclass == SC_VILLAGE && rng.chance(0.5)) {   // at the street's head: a stela, a church, a work (A-04: the people's focus half the time)
                const double hx = fx * (R * 0.95), hz = fz * (R * 0.95);
                if (c.age == AGE_INDUSTRIAL && rng.chance(0.6)) L.centre(hx + fx * 12, hz + fz * 12, h0 + PI, false, true);
                else if (c.age >= AGE_INDUSTRIAL) work(hx + fx * 15, hz + fz * 15, h0 + PI);
                else if (rng.chance(0.5)) L.focus(hx + fx * 8, hz + fz * 8, h0 + PI, 0.6);
                else L.stela(hx, hz, h0, 3.0 + 2.0 * rng.uni(), true);
            }
            break;
        }
        case SP_RADIAL: {
            // rings round a centre: the age's centre in the middle, the dwellings facing in (A-04: on spokes from it where the people's
            // streets ran out from the middle, the people's number of them)
            L.centre(0, 0, h0, (townOrGreater(r.sclass) && c.age != AGE_PREHISTORIC) || r.sclass == SC_CAPITAL, true);
            if (townOrGreater(r.sclass) || rng.chance(0.5)) { const double cr = r.buildings.empty() ? 6 : std::max(r.buildings.back().hw, r.buildings.back().hd); focusBefore(cr, townOrGreater(r.sclass) ? 1.0 : 0.6); }
            int placed = 0;
            if (streets == ST_RADIAL && c.age != AGE_PREHISTORIC) {
                const int spokes = clampi(G.number(), 3, 8);
                for (double d = R * 0.32; d < R * 0.95 && placed < n; d += 12.0 * S)
                    for (int k = 0; k < spokes && placed < n; k++) {
                        const double a = h0 + (k + 0.5) * TAU / spokes;
                        for (int side = -1; side <= 1; side += 2) {
                            const double ax = std::sin(a) * d + std::cos(a) * side * 7.5 * S, az = std::cos(a) * d - std::sin(a) * side * 7.5 * S;
                            if (rng.chance(0.15)) continue;
                            placed += L.dwellingSpot(ax, az, a - side * PI / 2);
                        }
                    }
            } else {
                const bool bigCore = r.sclass == SC_CITY || r.sclass == SC_CAPITAL;   // a city's rings, a street's width apart
                int rings = bigCore ? std::max(2, (int)(R / (14.0 * S))) : (r.sclass == SC_TOWN ? 2 : 1);
                for (int ring = 0; ring < rings && placed < n; ring++) {
                    double rr = bigCore ? R * (0.22 + 0.73 * ring / (rings - 1.0)) : R * (rings == 1 ? 0.6 : (ring == 0 ? 0.45 : 0.82));
                    int m = std::max(4, (int)(TAU * rr / (12.0 * S)));
                    double a0 = rng.uni() * TAU;
                    for (int k = 0; k < m && placed < n; k++) {
                        if (rng.chance(0.15)) continue;
                        double a = a0 + k * TAU / m + rng.sym(0.3 / m);
                        if (ring == rings - 1 && ring > 0 && c.towers && placed < n - 1 && rng.chance(0.08 * towerShare)) {
                            if (L.tower(std::sin(a) * rr, std::cos(a) * rr, a + PI)) placed++;
                            continue;
                        }
                        placed += L.dwellingSpot(std::sin(a) * rr, std::cos(a) * rr, a + PI);
                    }
                }
            }
            if (r.sclass == SC_VILLAGE && c.age >= AGE_INDUSTRIAL && rng.chance(0.5)) { double a = rng.uni() * TAU; work(std::sin(a) * (R + 14), std::cos(a) * (R + 14), a + PI); }
            break;
        }
        case SP_GRID: {
            // streets along the heading and across it, blocks of 17 x 15 m (the age's scale); the plaza at the centre: C-01's platform,
            // colonnade and stelae, or the age's centre (A-04: the people's) with its monuments before it and its focal accent; a hall
            // beside it (a station, a stadium farther out); a tower or two toward the edge (a modern town's towers in its middle);
            // a plaza kept open round the centre, gardens, the plaza's houses arcaded, colonnades along the main street
            double bx = (17.0 + 3.0 * rng.uni()) * S, bz = (15.0 + 3.0 * rng.uni()) * S;
            const bool c01 = c.age == AGE_MEDIEVAL && G.of(GV_CENTRE) == CE_TEMPLE && rng.chance(0.35);   // C-01's plaza
            if (c01) {
                L.add(BK_PLATFORM, 0, 0, h0, 5 + 2 * rng.uni(), 5 + 2 * rng.uni(), 3.3, 0.5, false);
                L.add(BK_COLONNADE, sx * bx * 0.75, sz * bx * 0.75, h0, 0.5, bz * 0.6, 3.5, 0.4, false);
                if (rng.chance(0.6)) L.stela(-sx * bx * 0.5 + fx * bz * 0.55, -sz * bx * 0.5 + fz * bz * 0.55, h0, 3.0 + 2.0 * rng.uni(), false);
                if (rng.chance(0.6)) L.stela(-sx * bx * 0.5 - fx * bz * 0.55, -sz * bx * 0.5 - fz * bz * 0.55, h0, 3.0 + 2.0 * rng.uni(), false);
            } else {
                L.centre(0, 0, h0, true, false);
                const Building& cb = r.buildings.back();
                double front = cb.hd + 7;
                for (int side = -1; side <= 1; side += 2) if (rng.chance(0.6)) L.stela(side * 5 * sx + fx * front, side * 5 * sz + fz * front, h0, 3.0 + 2.0 * rng.uni(), true);
                focusBefore(front + 2, 1.0);
            }
            if (c.age == AGE_INDUSTRIAL || c.age == AGE_MODERN) {   // the station or the stadium beside the centre, on the cross axis
                const Building& cb = r.buildings[0];
                double rc = std::sqrt(cb.hw * cb.hw + cb.hd * cb.hd) + 1.2;
                for (int side = -1; side <= 1; side += 2) {
                    size_t before = r.buildings.size();
                    double est = c.age == AGE_MODERN ? 95 : 40, d = rc + est + 3;
                    L.hall(sx * side * d, sz * side * d, h0 + side * PI / 2);
                    if (r.buildings.size() > before) break;
                }
            }
            int nx = (int)std::ceil(R / bx), nz = (int)std::ceil(R / bz);
            std::vector<std::pair<double, std::pair<int, int>>> nodes;
            for (int i = -nx; i <= nx; i++)
                for (int j = -nz; j <= nz; j++) {
                    if (i == 0 && j == 0) continue;
                    double d = std::sqrt(i * bx * i * bx + j * bz * j * bz);
                    if (d > R * 0.95) continue;
                    nodes.push_back({d + rng.uni() * 4.0, {i, j}});
                }
            std::sort(nodes.begin(), nodes.end());
            bool hallDone = c.age == AGE_INDUSTRIAL || c.age == AGE_MODERN; int towersDone = 0, placed = 0;
            const int towersMax = (int)std::lround((c.age == AGE_INDUSTRIAL ? 4 : 2) * towerShare * (r.sclass == SC_CITY || r.sclass == SC_CAPITAL ? R / 120.0 : 1.0));   // a city's many
            const double plaza = pub == PU_PLAZAS ? R * 0.24 : 0;
            int gardenK = 0;
            for (const auto& nd : nodes) {
                if (placed >= n) break;
                int i = nd.second.first, j = nd.second.second;
                if (rng.chance(pub == PU_NONE ? 0.04 : 0.12)) continue;   // a yard, or a house gone to nothing
                double x = sx * i * bx + fx * j * bz, z = sz * i * bx + fz * j * bz;
                double head = std::abs(i) >= std::abs(j) ? (i > 0 ? h0 - PI / 2 : h0 + PI / 2) : (j > 0 ? h0 + PI : h0);   // the fronts to the centre
                if (!hallDone && std::abs(i) + std::abs(j) == 1) {
                    hallDone = L.hall(x, z, head);
                    if (hallDone) { placed++; continue; }
                }
                if (nd.first < plaza) continue;   // the plaza kept open round the centre
                if (pub == PU_GARDENS && (++gardenK) % 5 == 0) {   // a walled garden, its pool
                    const double gw = std::min(bx, bz) * 0.38;
                    for (int s4 = 0; s4 < 4; s4++) { const bool al = s4 % 2 == 0; const double ox = (s4 == 1 ? gw : (s4 == 3 ? -gw : 0)), oz = (s4 == 0 ? gw : (s4 == 2 ? -gw : 0)); L.add(BK_WALL, x + sx * ox + fx * oz, z + sz * ox + fz * oz, h0 + (al ? 0 : PI / 2), gw * (s4 == 0 ? 0.6 : 1.0), 0.3, 0.8, 0.5, false); }
                    if (c.age >= AGE_MEDIEVAL) L.add(BK_FOCUS, x, z, h0, gw * 0.4, gw * 0.3, 0.8, 0.5, false, FO_POOL);
                    continue;
                }
                if (c.age == AGE_MODERN && nd.first < R * 0.45 && rng.chance(0.25 + 0.6 * G.at(GV_HEIGHT))) {   // the towers of the middle, lower outward
                    double hw = 10 + 5 * rng.uni();
                    if (L.add(BK_HIGHRISE, x, z, head, hw, hw * (0.75 + 0.25 * rng.uni()), (80 + 260 * rng.uni()) * (1.1 - nd.first / R) * L.towerK, 0.5)) { placed++; continue; }
                }
                if (c.towers && towersDone < towersMax && nd.first > R * 0.5 && rng.chance(c.age == AGE_INDUSTRIAL ? 0.4 : 0.3)) {
                    if (L.tower(x, z, head)) { towersDone++; placed++; continue; }
                }
                L.arcadeNext = links == LK_ARCADES && std::abs(i) + std::abs(j) <= 2;
                placed += L.dwellingSpot(x, z, head);
                L.arcadeNext = false;
            }
            if (links == LK_COLONNADES)   // colonnades along the main street
                for (int j = -2; j <= 2; j++) for (int side = -1; side <= 1; side += 2) { if (j == 0) continue; L.add(BK_COLONNADE, sx * side * bx * 0.5 + fx * j * bz, sz * side * bx * 0.5 + fz * j * bz, h0, 0.5, bz * 0.42, 3.5, 0.4, true); }
            break;
        }
        case SP_GREAT: {   // a futuristic people's few great structures and the land between left wild
            if (r.sclass == SC_HAMLET) {
                for (int k = 0; k < n; k++)
                    for (int tries = 0; tries < 20; tries++) { double a = rng.uni() * TAU, rr = R * 0.8 * std::sqrt(rng.uni()); if (L.dwelling(std::sin(a) * rr, std::cos(a) * rr, rng.uni() * TAU)) break; }
                break;
            }
            const bool town = townOrGreater(r.sclass);
            const double A = (0.75 + 0.8 * G.at(GV_AMBITION)) * (r.sclass == SC_CAPITAL ? 1.25 : 1.0);
            switch (G.of(GV_CENTRE)) {   // the people's centre: a spire, its arcology, a dome, a ring, a stepped tomb, a ring's machine
                case CE_TEMPLE: { double rr = (18 + 10 * rng.uni()) * A; L.add(BK_SPIRE, 0, 0, h0, rr, rr, ((town ? 600 : 350) + 400 * rng.uni()) * L.towerK, 0.5, false); break; }
                case CE_ASSEMBLY: { double rr = ((town ? 70 : 40) + 30 * rng.uni()) * A; L.add(BK_HABITAT, 0, 0, h0, rr, rr, rr * 0.6, 0.5, false); break; }
                case CE_MARKET: case CE_MACHINE: { double rr = ((town ? 70 : 45) + 40 * rng.uni()) * A; L.add(BK_RING, 0, 0, h0 + PI / 2, rr, 6, 2 * rr, 0.5, false); break; }
                case CE_TOMB: { double rr = ((town ? 70 : 40) + 30 * rng.uni()) * A; L.add(BK_ARCOLOGY, 0, 0, h0, rr, rr, rr * 1.4, 0.5, false, 0); break; }
                default:
                    if (town) { double rr = (90 + 60 * rng.uni()) * A; L.add(BK_ARCOLOGY, 0, 0, h0, rr, rr, (300 + 500 * rng.uni()) * L.towerK, 0.5, false); }
                    else { double rr = (40 + 30 * rng.uni()) * A; L.add(BK_HABITAT, 0, 0, h0, rr, rr, rr * 0.6, 0.5, false); }
                    break;
            }
            { const Building& cb = r.buildings.back(); focusBefore(std::max(cb.hw, cb.hd) + 30, 1.0); }
            // the spires: as many as the people's height allows, set round by its symmetry (evenly; in mirrored pairs; turned; at random)
            const int spires = std::max(1, (int)std::lround((town ? 2.5 : 1.0) * towerShare));
            const int sym = G.of(GV_SYMMETRY);
            for (int k = 0; k < spires; k++) {
                double a;
                switch (sym) {
                    case SY_RADIAL: a = h0 + (k + 0.5) * TAU / spires; break;
                    case SY_BILATERAL: a = h0 + (k % 2 ? 1 : -1) * (0.6 + 0.5 * (k / 2)); break;
                    case SY_ROTATIONAL: a = h0 + k * TAU / spires + 0.6; break;
                    default: a = h0 + (k + 0.3 + 0.4 * rng.uni()) * TAU / spires; break;
                }
                double d = R * (0.68 + 0.2 * rng.uni()), rr = 15 + 10 * rng.uni();
                if (links == LK_BRIDGES && k == 0) { L.add(BK_TWIN, std::sin(a) * d, std::cos(a) * d, a + PI / 2, rr * 3.4, rr, ((town ? 400 : 250) + 300 * rng.uni()) * L.towerK, 0.5, true); continue; }
                L.add(BK_SPIRE, std::sin(a) * d, std::cos(a) * d, a + PI, rr, rr, ((town ? 500 : 300) + (town ? 700 : 300) * rng.uni()) * L.towerK, 0.5);
            }
            if (town) { double a = h0 + PI + rng.sym(0.6), d = R * 0.55, rr = 50 + 50 * rng.uni(); L.add(BK_RING, std::sin(a) * d, std::cos(a) * d, a + PI / 2, rr, 6, 2 * rr, 0.5); }
            if (town || rng.chance(0.5) || links == LK_ARCADES || links == LK_TUNNELS) L.add(BK_MAGLEV, sx * R * 0.5, sz * R * 0.5, h0, 3, R * 0.95, 18 + 10 * rng.uni(), 0.5, false);
            int placed = (int)r.buildings.size();
            const double habShare = G.of(GV_MASSING) == MS_SPRAWLING ? 0.45 : (G.of(GV_MASSING) == MS_COMPACT ? 0.12 : (town ? 0.3 : 0.15));
            for (int tries = 0; tries < 60 && placed < n; tries++) {
                double a = rng.uni() * TAU, rr = R * (0.35 + 0.6 * std::sqrt(rng.uni())) * (G.of(GV_MASSING) == MS_COMPACT ? 0.75 : 1.0);
                if (rng.chance(habShare)) { double hr = 30 + 25 * rng.uni(); if (L.add(BK_HABITAT, std::sin(a) * rr, std::cos(a) * rr, a + PI, hr, hr * (G.of(GV_PLAN) == PF_ELLIPSE ? 1.3 : 1.0), hr * 0.6, 0.5)) placed++; }
                else if (L.dwelling(std::sin(a) * rr, std::cos(a) * rr, a + PI)) placed++;
            }
            break;
        }
    }
    if (r.walled) {   // the town wall: a ring of segments, a stretch fallen here and there, a gate at each end of the street's axis;
                      // a palisade on its bank and a gate of two posts, or walls with round towers at their joints and gatehouses;
                      // a bank and its ditch where the people dug its edge
        double reach = 0;   // outside every building (a wall through a house blocked its doorway on the desert test world)
        for (const Building& b : r.buildings) reach = std::max(reach, std::sqrt(b.x * b.x + b.z * b.z) + std::sqrt(b.hw * b.hw + b.hd * b.hd));
        const bool pal = c.age == AGE_PREHISTORIC || edge == ED_DITCHES;
        double rw = std::max(R * 1.04, reach + 3.0 + (!pal && c.towers ? 3.6 : 0.0));   // and its round towers' width (they stood in two doorways)
        int segs = 10 + rng.irange(5);
        if (r.sclass == SC_CITY || r.sclass == SC_CAPITAL) segs = std::max(segs, (int)(TAU * rw / 45.0));   // a city's wall in stretches of a town's
        double hWall = 4.0 + 2.0 * rng.uni();
        if (pal) hWall *= 0.65;
        std::vector<double> rad(segs);
        for (int k = 0; k < segs; k++) rad[k] = rw * (0.96 + 0.08 * rng.uni());
        for (int k = 0; k < segs; k++) {
            double aA = h0 + (k + 0.5) * TAU / segs, aB = h0 + (k + 1.5) * TAU / segs;
            double xA = std::sin(aA) * rad[k], zA = std::cos(aA) * rad[k], xB = std::sin(aB) * rad[(k + 1) % segs], zB = std::cos(aB) * rad[(k + 1) % segs];
            double mx = (xA + xB) * 0.5, mz = (zA + zB) * 0.5, dx = xB - xA, dz = zB - zA, len = std::sqrt(dx * dx + dz * dz);
            double wh = std::atan2(dx, dz) + PI / 2;   // the wall runs across its heading
            double am = h0 + (k + 1.0) * TAU / segs;
            double gA = std::fabs(std::remainder(am - h0, TAU)), gB = std::fabs(std::remainder(am - h0 - PI, TAU));
            bool gate = std::min(gA, gB) < PI / segs;
            if (!pal && c.towers) L.add(BK_WALL_TOWER, xA, zA, aA, 2.6 + 0.8 * rng.uni(), 0, (hWall + 3 + 2 * rng.uni()) * (0.8 + 0.4 * G.at(GV_HEIGHT)), 0.5, false);
            if (!gate && rng.chance(0.12)) continue;   // a fallen stretch
            const int wk = pal ? BK_PALISADE : BK_WALL;
            if (gate) {
                double ux = dx / len, uz = dz / len, q = len * 0.25 + 1.5;
                L.add(wk, mx - ux * q, mz - uz * q, wh, len * 0.25 - 1.5, 0.45, hWall, 0.9, false);
                L.add(wk, mx + ux * q, mz + uz * q, wh, len * 0.25 - 1.5, 0.45, hWall, 0.9, false);
                if (c.age == AGE_PREHISTORIC || !pal) L.add(BK_GATE, mx, mz, wh, 1.9, 0.6, hWall + 1.0, 0.8, false);
            } else {
                L.add(wk, mx, mz, wh, len * 0.5, 0.45, hWall, 0.9, false);
            }
        }
    }
    // the people's relation to the land: a high place toward the ridge (a stepped platform and its shrine), an aqueduct coming down
    // from the high ground; the people's tunnels' mouths
    if (r.sclass >= SC_VILLAGE && r.sclass != SC_MONUMENT && c.age <= AGE_MODERN) {
        const double rd = r.site.ridgeDir;
        if (land == LD_RIDGES) for (int k = 0; k < 4; k++) { const double d = R * (0.72 + 0.1 * k); if (L.add(BK_PLATFORM, std::sin(rd) * d, std::cos(rd) * d, rd + PI, 7 + 3 * rng.uni(), 7 + 3 * rng.uni(), 3.3, 0.5, true)) break; }
        if (land == LD_VALLEYS) {
            const double len = 70 + 50 * rng.uni(), d = R * 1.04 + len * 0.5 + 6;
            L.add(BK_AQUEDUCT, std::sin(rd) * d, std::cos(rd) * d, rd, 1.4, len * 0.5, (9 + 9 * rng.uni()) * (0.8 + 0.4 * G.at(GV_AMBITION)), 1.2, true);
        }
        if (links == LK_TUNNELS && c.age >= AGE_MEDIEVAL)
            for (int k = 0, made = 0; k < 12 && made < 3; k++) { const double a = rng.uni() * TAU, d = R * (0.3 + 0.6 * rng.uni()); if (L.add(BK_PORTAL, std::sin(a) * d, std::cos(a) * d, a, 2.4, 3.5, 4.8, 0.8, true)) made++; }
    }
    addFeatures(L, r, c);   // after the plan and the wall, so the earlier draws stay where C-01 left them
    if (c.age == AGE_FUTURISTIC && r.plan == SP_GREAT && r.sclass != SC_HAMLET) placeFurniture(L, r, R);
}
