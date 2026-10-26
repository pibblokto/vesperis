// E-09: T-02's towns: a place's plan from the world's (its exits, its streets and squares and fields by the people and the
// age, its districts and their buildings), and the street furniture
#include "ruins_layout.h"

namespace {
// the class's radius in an age (the plan's cores for a capital and a city; C-01's town's and village's by the age's scale)
double classRadiusIn(int sclass, int age) {
    static const double CAP[AGE_COUNT] = {110, 470, 820, 1150, 480, 480}, CITY[AGE_COUNT] = {90, 270, 500, 700, 230, 230};
    const int a = clampi(age, 0, AGE_COUNT - 1);
    if (sclass == SC_CAPITAL) return CAP[a];
    if (sclass == SC_CITY) return CITY[a];
    return (sclass == SC_VILLAGE ? 57.0 : 75.0) * ageScale(a);
}
// the bearings of the plan's ways from a place of it (radians from north)
std::vector<double> planExits(const BodyGen& g, const RuinSpec& r) {
    std::vector<double> ex;
    if (r.rank < 0 || !worldHadCivilisation(g)) return ex;
    const std::shared_ptr<const WorldPlan> P = planFor(g);
    const PeoplePlan& pp = P->plans[r.people > 0 && P->peoples > 1 ? 1 : 0];
    if (r.rank >= (int)pp.places.size()) return ex;
    const PlanPlace& me = pp.places[r.rank];
    for (const PlanEdge& e : pp.edges) {
        if (e.a != r.rank && e.b != r.rank) continue;
        const PlanPlace& o = pp.places[e.a == r.rank ? e.b : e.a];
        ex.push_back(std::atan2(std::remainder(o.lon - me.lon, TAU) * std::cos(me.lat), o.lat - me.lat));
    }
    return ex;
}
}   // namespace
// a village's, a town's or a city's plan as its ages built it (see towns.h): its layers, its centre and its square, its streets, its buildings
// along them by district (each layer in its age's way, the people's genome through it), its wall and gates, its works of the water and the
// land, its fields. `n` the buildings its class asks for (a city's the plan's), `R` its reach. B-414: one layer, its people's last age
// the people's street furniture (`GV_FURNITURE`; `BK_FURNITURE`, its style the `Furniture`): a town's well, fountain or figure in each
// square and beside the middle of each long main street, its lamps along its main streets and round its squares; a futuristic people's
// round its great structures. Laid last, by a stream of its own (nothing of the plan moves for it)
void placeFurniture(Layout& L, RuinSpec& r, double R) {
    const int fu = L.G.of(GV_FURNITURE), age = L.age;
    if (fu == FU_NONE || age < AGE_MEDIEVAL || age > AGE_FUTURISTIC) return;
    Rng rng(r.seed ^ 0xF0412ULL);
    const double S = fu == FU_FOUNTAINS ? (age == AGE_MODERN ? 3.2 : 2.7) : (fu == FU_WELLS ? 1.0 : (fu == FU_STATUES ? 1.0 : 0.25));
    auto put = [&](double x, double z, double heading) {
        const size_t n0 = r.buildings.size();
        if (!L.add(BK_FURNITURE, x, z, heading, S, S, fu == FU_LAMPS ? 4.0 : 2.0, 0.3, true, fu)) return false;
        r.buildings[n0].district = 0;
        return true;
    };
    int lamps = 0, pieces = 0;
    if (r.streets.empty()) {   // a futuristic people's: round its middle, past its great structure
        const int n = fu == FU_LAMPS ? 10 : 3;
        for (int k = 0; k < n; k++) {
            const double a = r.heading + (k + 0.5) * TAU / n;
            for (int q = 0; q < 8; q++) { const double d = R * (0.2 + 0.1 * q) + 6 * rng.uni(); if (put(std::sin(a) * d, std::cos(a) * d, a + PI)) break; }
        }
        return;
    }
    for (const Plaza& p : r.plazas) {
        if (fu == FU_LAMPS) {
            for (int k = 0; k < 4 && lamps < 60; k++) {
                const double a = PI / 4 + k * PI / 2;
                if (put(p.x + std::sin(a) * p.r * 0.85, p.z + std::cos(a) * p.r * 0.85, a)) lamps++;
            }
            continue;
        }
        static const double OFF[5][2] = {{0, 0}, {0.5, 0}, {-0.5, 0}, {0, 0.5}, {0, -0.5}};
        for (int t = 0; t < 5 && pieces < 12; t++) if (put(p.x + OFF[t][0] * p.r, p.z + OFF[t][1] * p.r, std::atan2(-p.x, -p.z + 1e-9))) { pieces++; break; }
    }
    for (const Street& s : r.streets) {
        if (s.rank < SR_MAIN) continue;
        const double len = std::hypot(s.bx - s.ax, s.bz - s.az);
        if (len < 20) continue;
        const double ux = (s.bx - s.ax) / len, uz = (s.bz - s.az) / len, nx = -uz, nz = ux;
        if (fu == FU_LAMPS) {
            int side = 1;
            for (double t = 8; t < len - 6 && lamps < 60; t += 22, side = -side)
                if (put(s.ax + ux * t + nx * side * (s.half + 0.8), s.az + uz * t + nz * side * (s.half + 0.8), std::atan2(-nx * side, -nz * side))) lamps++;   // at the kerb, before the yards' bounds
        } else if (len > 50 && pieces < 12) {
            const double t = len * (0.35 + 0.3 * rng.uni()), off = s.half + S + 0.8;
            const int side = rng.chance(0.5) ? 1 : -1;
            if (put(s.ax + ux * t + nx * side * off, s.az + uz * t + nz * side * off, std::atan2(-nx * side, -nz * side))) pieces++;
        }
    }
}

void layoutTown(RuinSpec& r, const Culture& c, const BodyGen& g, int n, double R, const TownHist& th, double luckS) {
    const Genome& G = c.genome;
    const int nl = (int)th.ages.size(), townAge = th.ages.back();
    const bool big = r.sclass == SC_CITY || r.sclass == SC_CAPITAL, village = r.sclass == SC_VILLAGE;
    const double Rt = std::max(30.0, R * classRadiusIn(r.sclass, townAge) / classRadiusIn(r.sclass, c.age));
    const double h0 = r.heading;
    r.buildAge = -1;   // B-414: its people's own age
    // the layers, each its ring and its builder: the people's genome through the layer's age, weathered from when the place was left
    std::vector<Culture> lc(nl);
    std::vector<std::unique_ptr<Layout>> LL;
    r.layers.clear();
    double prev = 0;
    for (int k = 0; k < nl; k++) {
        lc[k] = cultureAtAge(c, th.ages[k], th.before);
        TownLayer tl; tl.age = th.ages[k]; tl.r0 = prev;
        tl.r1 = k + 1 == nl ? Rt : std::min(Rt - 40, std::max(prev + 30, Rt * classRadiusIn(r.sclass, th.ages[k]) / classRadiusIn(r.sclass, townAge)));
        prev = tl.r1;
        r.layers.push_back(tl);
    }
    for (int k = 0; k < nl; k++) { LL.emplace_back(new Layout(r, lc[k], 0x7051ULL * (k + 1))); LL.back()->luckS = luckS; LL.back()->obb = true; }
    Layout& Lc = *LL.front();   // the oldest: the centre's
    Layout& Lo = *LL.back();    // the town's own age
    Rng& rng = Lo.rng;
    BuildingGrid bg; bg.init(Rt + 500);
    for (auto& l : LL) l->grid = &bg;
    const SiteRead& st = r.site;
    const int pub = G.of(GV_PUBLIC), links = G.of(GV_LINKS), grouping = G.of(GV_GROUPING), edge = G.of(GV_EDGE);
    const double fx = std::sin(h0), fz = std::cos(h0), sx = std::cos(h0), sz = -std::sin(h0);
    // the centre, in the oldest layer's way (a modern city's medieval temple at its heart), its monuments and its focal accent before it; the
    // square round them
    Lc.centre(0, 0, h0, !village, false);
    double cReach = 6;
    if (!r.buildings.empty()) { const Building& cb = r.buildings.back(); cReach = std::sqrt(cb.hw * cb.hw + cb.hd * cb.hd); }
    {
        const Building cb = r.buildings.empty() ? Building() : r.buildings.back();
        const double front = (r.buildings.empty() ? 6 : cb.hd) + 5;
        if (Lc.age < AGE_FUTURISTIC)
            for (int side = -1; side <= 1; side += 2)
                if (rng.chance(0.7)) Lc.stela(side * 4.5 * sx + fx * (front + 2), side * 4.5 * sz + fz * (front + 2), h0, 3.0 + 2.0 * rng.uni(), true);
        if (!Lc.focusCarried()) for (int k = 0; k < 4; k++) { const double d = front + 6 + k * 7; if (Lc.focus(fx * d, fz * d, h0 + PI, village ? 0.6 : 1.0)) break; }
    }
    for (Building& b : r.buildings) { b.district = DI_CENTRE; cReach = std::max(cReach, std::hypot(b.x, b.z) + std::sqrt(b.hw * b.hw + b.hd * b.hd)); }
    double plaza = (townAge <= AGE_MEDIEVAL ? 12 : (townAge == AGE_INDUSTRIAL ? 18 : 26)) * (big ? 1.6 : (village ? 0.7 : 1.0));
    if (pub == PU_PLAZAS) plaza *= 1.4; else if (pub == PU_NONE) plaza *= 0.75;
    plaza = std::max(plaza, cReach + 4 + streetHalf(SR_MAIN, townAge) * (big ? 1.0 : (village ? 0.6 : 0.75)));   // the main streets' ends clear of it
    // the streets: the people's pattern through each layer's age (a village one street or two, a green, or lanes)
    TownSpec T; T.R = Rt; T.h0 = h0; T.sclass = r.sclass; T.number = G.number(); T.layers = r.layers; T.plaza = plaza; T.seed = r.seed;
    T.pattern = G.of(GV_STREETS);
    if (village) T.pattern = T.pattern == ST_GRID || T.pattern == ST_CONTOUR ? ST_LINE : (T.pattern == ST_RADIAL ? ST_CONCENTRIC : T.pattern);
    T.exits = planExits(g, r);
    // the wall of the medieval layer (decided before the streets: its lesser streets end inside it, a street runs round inside it, the main
    // streets go out by its gates): the town's edge, or an old core's inside a later city (its line the later boulevard)
    int walledK = -1;
    for (int k = 0; k < nl; k++) if (th.ages[k] == AGE_MEDIEVAL) walledK = k;
    r.walled = walledK >= 0 && edge != ED_NOTHING && rng.chance(big || r.sclass == SC_TOWN ? 0.85 : 0.3);
    const double wallR = r.walled ? (walledK + 1 == nl ? Rt * 1.02 : r.layers[walledK].r1 - 1.5) : 1e9;
    if (r.walled) { T.walled = true; T.wallR = wallR; T.wallK = walledK; }
    // the shore: the streets end before the water (and the harbour's room)
    const bool shoreNear = st.shoreDist < Rt + 80;
    if (shoreNear) { T.shore = true; T.shoreNx = std::sin(st.shoreDir); T.shoreNz = std::cos(st.shoreDir); T.shoreLimit = st.shoreDist - (townAge >= AGE_INDUSTRIAL ? 34 : 8); }
    townStreets(T, r.streets, r.plazas);
    // what its streets could hold against what it built: a town that could not line its streets lays wider blocks (the streets again, the
    // blocks the root of the ratio wider: the frontage goes as the blocks' inverse)
    auto frontage = [&]() {
        double lots = 0;
        for (const Street& s : r.streets) {
            if (s.rank == SR_RING) continue;
            const double rr = std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5);
            if (rr > Rt + 4 || rr < plaza + 3) continue;
            const int age = th.ages[std::min(nl - 1, [&] { for (int k = 0; k < nl; k++) if (rr < r.layers[k].r1 + 1) return k; return nl - 1; }())];
            lots += 2 * std::hypot(s.bx - s.ax, s.bz - s.az) / (age <= AGE_MEDIEVAL ? 7.0 : (age == AGE_INDUSTRIAL ? 22.0 : 28.0));
        }
        return lots;
    };
    {
        const double want = (big ? n : (village ? 1.8 * n : 3.2 * n)) * 1.6;
        const double have = frontage();
        if (have > want * 1.25) {
            T.spacing = clampd(have / want, 1.0, 3.0);
            townStreets(T, r.streets, r.plazas);
        }
    }
    StreetGrid sg; sg.build(r.streets, Rt + 500);
    for (auto& l : LL) { l->sgrid = &sg; l->streets = &r.streets; }
    // the districts
    double ringR = 1e9;
    for (const Street& s : r.streets) if (s.rank == SR_RING) ringR = std::min(ringR, std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5));
    const double coreR = nl > 1 ? r.layers[0].r1 : 0;
    const double worksDir = st.wind + PI;   // downwind: the smoke away from the town
    const bool harbourOn = shoreNear && townAge >= AGE_MEDIEVAL && !village;
    auto layerAt = [&](double rr) { for (int k = 0; k < nl; k++) if (rr < r.layers[k].r1 + 1) return k; return nl - 1; };
    auto districtAt = [&](double x, double z, int k) {
        const double rr = std::hypot(x, z), dd = rr / Rt, a = std::atan2(x, z);
        if (rr < plaza + 3) return (int)DI_CENTRE;
        if (harbourOn && std::fabs(std::remainder(a - st.shoreDir, TAU)) < 0.85 && rr > std::max(0.45 * Rt, st.shoreDist - std::max(70.0, 0.3 * Rt))) return (int)DI_HARBOUR;
        if (k == 0 && nl > 1) return (int)DI_CORE;
        const int age = th.ages[k];
        // a modern city's towers round its old core (the modern age building over the ring before it) or in its middle
        if (townAge == AGE_MODERN && big && rr < std::max(coreR, plaza) + std::max(90.0, 0.13 * Rt)) return (int)DI_TOWERS;
        if ((age == AGE_INDUSTRIAL || age == AGE_MODERN) && !village && dd > 0.3 && std::fabs(std::remainder(a - worksDir, TAU)) < (big ? 0.55 : 0.45)) return (int)DI_WORKS;
        if (age == AGE_MODERN && k + 1 == nl) {
            if (rr > ringR + 5) return (int)DI_SUBURBS;
            if (!big && dd > 0.78) return (int)DI_SUBURBS;
        }
        return (int)DI_HOUSES;
    };
    auto density = [&](int di, int age, double dd) {
        switch (di) {
            case DI_CORE: return 0.95;
            case DI_TOWERS: return 0.85;
            case DI_SUBURBS: return 0.6 - 0.3 * clampd((dd - 0.7) / 0.3, 0, 1);   // thinning into the fields
            case DI_WORKS: return 0.6;
            case DI_HARBOUR: return 0.55;
            default: return (age <= AGE_MEDIEVAL ? 0.97 : 0.9) - 0.4 * dd * dd;   // falling from the centre
        }
    };
    auto lotWidth = [&](int di, int age) {   // a lot's frontage, its gap with it
        switch (di) {
            case DI_TOWERS: return 34.0;
            case DI_SUBURBS: return 15.0;
            case DI_WORKS: return 30.0;
            case DI_HARBOUR: return 26.0;
            default: return age <= AGE_MEDIEVAL ? 6.5 : (age == AGE_INDUSTRIAL ? 21.0 : 26.0);
        }
    };
    // how much of the frontage is built: the class's count of buildings over what the streets could hold
    double expected = 0;
    for (const Street& s : r.streets) {
        if (s.rank == SR_RING) continue;
        const double mx = (s.ax + s.bx) * 0.5, mz = (s.az + s.bz) * 0.5, rr = std::hypot(mx, mz);
        if (rr > Rt + 4) continue;
        const int k = layerAt(rr), di = districtAt(mx, mz, k);
        if (di == DI_CENTRE) continue;
        expected += 2 * std::hypot(s.bx - s.ax, s.bz - s.az) / lotWidth(di, th.ages[k]) * density(di, th.ages[k], rr / Rt);
    }
    const double target = big ? n : (village ? 1.8 * n : 3.2 * n);
    const double p0 = clampd(target / std::max(1.0, expected), 0.35, 1.0);
    const int cap = (int)std::lround(target * 1.35) + 12;
    // a lot's building by its district and its ring's age: 0 one building, 1 a compound (houses round a court), 2 a tower, 3 a walled garden
    bool mainFront = false;   // the lot fronts a main street near the middle (a medieval town's main streets are rows whatever the people's grouping)
    auto lotSpec = [&](Layout& Lk, int di, int age, double dd, BSpec& sp, int& kind) {
        kind = 0;
        const double F = Lk.footK, Hk = Lk.heightK, u = Lk.rng.uni();
        auto set = [&](int k, double hw, double hd, double h, double t, int style = -1) { sp.kind = k; sp.hw = hw; sp.hd = hd; sp.h = h; sp.thick = t; sp.style = style; return true; };
        switch (di) {
            case DI_TOWERS:
                if (u < 0.3 + 0.55 * G.at(GV_HEIGHT)) {
                    const double hw = 10 + 5 * Lk.rng.uni();
                    return set(BK_HIGHRISE, hw, hw * (0.75 + 0.25 * Lk.rng.uni()), (70 + 260 * Lk.rng.uni()) * clampd(1.25 - dd, 0.35, 1.2) * Lk.towerK, 0.5);
                }
                return set(BK_BLOCK, Lk.var(11, 4) * F, Lk.var(6.0, 2.5) * F * Lk.hdK, (22 + 30 * Lk.rng.uni()) * clampd(1.25 - dd, 0.5, 1.2) * Hk, 0.4);
            case DI_SUBURBS: return set(BK_SUBURB, Lk.var(4.0, 1.5) * F, Lk.var(5.0, 1.5) * F * Lk.hdK, (Lk.rng.chance(0.5) ? 2.9 : 5.8) * std::min(1.4, Hk), 0.35);
            case DI_WORKS:
                if (age == AGE_INDUSTRIAL) {
                    if (u < 0.42) return set(BK_MILL, 7 + 3 * Lk.rng.uni(), 13 + 7 * Lk.rng.uni(), (15 + 7 * Lk.rng.uni()) * Lk.towerK, 0.7);
                    if (u < 0.62) return set(BK_HALL, 7 + 3 * Lk.rng.uni(), 12 + 8 * Lk.rng.uni(), 7 + 2 * Lk.rng.uni(), 0.7);   // a warehouse
                    if (u < 0.72) { const double rr = 5 + 1.5 * Lk.rng.uni(); return set(BK_WATER_TOWER, rr, rr, (16 + 8 * Lk.rng.uni()) * Lk.towerK, 0.5); }
                    if (u < 0.8) { const double rr = 13 + 6 * Lk.rng.uni(); return set(BK_GASOMETER, rr, rr, 16 + 10 * Lk.rng.uni(), 0.5); }
                    return set(BK_ROW, Lk.var(10.0, 4.0) * F, Lk.var(4.2, 1.0) * F * Lk.hdK, Lk.var(6.5, 3.0) * Hk, 0.45);   // the workers' rows
                }
                if (u < 0.3) return set(BK_TANKS, 18 + 10 * Lk.rng.uni(), 9 + 4 * Lk.rng.uni(), 15 + 10 * Lk.rng.uni(), 0.5);
                if (u < 0.8) return set(BK_BLOCK, 18 + 10 * Lk.rng.uni(), 11 + 6 * Lk.rng.uni(), 8 + 5 * Lk.rng.uni(), 0.4);   // a warehouse, a works hall
                if (u < 0.88) return set(BK_MAST, 3, 3, (60 + 80 * Lk.rng.uni()) * Lk.towerK, 0.5);
                return set(BK_BLOCK, Lk.var(11, 4) * F, Lk.var(6.0, 2.5) * F * Lk.hdK, (15 + 20 * Lk.rng.uni()) * Hk, 0.4);
            case DI_HARBOUR:   // the warehouses of the waterside
                if (age <= AGE_MEDIEVAL) return set(BK_HALL, 4.5 + 1.5 * Lk.rng.uni(), 6 + 3 * Lk.rng.uni(), 4 + 1.5 * Lk.rng.uni(), 0.6);
                if (age == AGE_INDUSTRIAL) return set(BK_HALL, 8 + 3 * Lk.rng.uni(), 14 + 8 * Lk.rng.uni(), 8 + 2 * Lk.rng.uni(), 0.7);
                return set(BK_BLOCK, 18 + 10 * Lk.rng.uni(), 12 + 6 * Lk.rng.uni(), 9 + 5 * Lk.rng.uni(), 0.4);
            default: break;
        }
        // the houses: a tower now and then toward the edge, a walled garden, a compound, or the age's dwelling (taller toward a city's middle)
        if (age <= AGE_INDUSTRIAL && dd > 0.45 && c.towers && Lk.rng.chance(0.035 * (0.5 + 1.5 * G.at(GV_HEIGHT)))) { kind = 2; sp.hw = sp.hd = 4; return true; }
        if (pub == PU_GARDENS && age >= AGE_MEDIEVAL && age <= AGE_INDUSTRIAL && Lk.rng.chance(0.12)) { kind = 3; sp.hw = sp.hd = std::max(5.0, 0.38 * streetSpacing(age) * 0.5); return true; }
        if (grouping == GR_COMPOUNDS && age <= AGE_MODERN && age != AGE_INDUSTRIAL && di == DI_HOUSES && !(big && dd < 0.6)) {
            kind = 1;
            sp.hw = sp.hd = (age == AGE_MODERN ? 9.0 : 6.5) + 2.5;
            return true;
        }
        if ((grouping == GR_ROWS || mainFront) && age == AGE_MEDIEVAL && Lk.rng.uni() < 0.85) {   // a row's houses joined along the street, gabled cell by cell where the people built in cells
            set(BK_HOUSE, 2.7 * (0.95 + 0.1 * Lk.rng.uni()), 3.6 * (0.92 + 0.16 * Lk.rng.uni()), Lk.var(2.8, 0.6) * Hk, 0.45);
            Lk.frontage(sp);
            sp.rect = true;
            if (big) sp.h *= 0.85 + 0.55 * (1 - dd);
            return true;
        }
        if (!Lk.dwellingSpec(dd, sp)) return false;
        if (sp.kind == BK_HOUSE || sp.kind == BK_ROW || sp.kind == BK_TENEMENT || sp.kind == BK_BLOCK || sp.kind == BK_VILLA) sp.h *= big ? 0.85 + 0.55 * (1 - dd) : 0.92 + 0.2 * (1 - dd);
        return true;
    };
    // how far a medieval, an industrial or a modern people set its houses back from the street (`GV_SETBACK`): at its edge, or in
    // front yards and gardens
    auto setback = [&](int di, int age, int kind, const Genome& G) {
        const double S = age >= AGE_MEDIEVAL && age <= AGE_MODERN ? G.at(GV_SETBACK) : 0.3;
        if (kind == BK_VILLA || kind == BK_SUBURB) return (4.0 + 3.0 * rng.uni()) * (0.6 + 0.8 * S);
        switch (di) {
            case DI_TOWERS: return 6.0 + 6.0 * rng.uni();
            case DI_WORKS: case DI_HARBOUR: return 3.0 + 3.0 * rng.uni();
            default: return age <= AGE_MEDIEVAL ? 0.4 + 4.5 * S * S : (age == AGE_INDUSTRIAL ? 0.8 + 5.0 * S * S : 2.0 + 7.0 * S * S);
        }
    };
    auto gapAfter = [&](int di, int age, int kind) {
        if (kind == BK_SUBURB) return 4.0 + 6.0 * rng.uni();
        switch (di) {
            case DI_TOWERS: return 6.0 + 8.0 * rng.uni();
            case DI_WORKS: case DI_HARBOUR: return 4.0 + 5.0 * rng.uni();
            default:
                if (age == AGE_INDUSTRIAL) return kind == BK_ROW ? 0.2 : 3.0 + 3.0 * rng.uni();
                if (age == AGE_MODERN) return 4.0 + 4.0 * rng.uni();
                return grouping == GR_ROWS || (mainFront && kind == BK_HOUSE) ? 0.06 : (grouping == GR_DETACHED ? 1.5 + 3.0 * rng.uni() : 0.6 + 1.2 * rng.uni());
        }
    };
    // a special building fronting a street: the first spot it fits along the streets that pass the test
    auto front = [&](Layout& Lk, const BSpec& sp, const std::function<bool(const Street&)>& want, int di) {
        for (const Street& s : r.streets) {
            if (!want(s)) continue;
            const double L = std::hypot(s.bx - s.ax, s.bz - s.az), ux = (s.bx - s.ax) / L, uz = (s.bz - s.az) / L, nx = -uz, nz = ux;
            for (int side = -1; side <= 1; side += 2)
                for (double t = sp.hw + 2; t < L - sp.hw - 2; t += 6) {
                    const double off = s.half + 3 + sp.hd, cx = s.ax + ux * t + nx * side * off, cz = s.az + uz * t + nz * side * off;
                    if (std::hypot(cx, cz) > Rt + 5) continue;
                    const size_t n0 = r.buildings.size();
                    if (Lk.add(sp.kind, cx, cz, std::atan2(-nx * side, -nz * side), sp.hw, sp.hd, sp.h, sp.thick, true, sp.style)) {
                        if (Lk.intrusion(n0, r.site) > 0) { Lk.undoTo(n0); continue; }
                        r.buildings.back().district = di; return true;
                    }
                }
        }
        return false;
    };
    // the station of an industrial ring on a main street a third of the way out; a medieval town's market hall by the square
    for (int k = 0; k < nl; k++) {
        Layout& Lk = *LL[k];
        if (th.ages[k] == AGE_INDUSTRIAL && !village) {
            BSpec sp; sp.kind = BK_STATION; sp.hw = 14 + 4 * Lk.rng.uni(); sp.hd = 24 + 10 * Lk.rng.uni(); sp.h = 11 + 3 * Lk.rng.uni(); sp.thick = 0.8;
            front(
                    Lk, sp,
                    [&](const Street& s) {
                        const double rr = std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5);
                        return s.rank >= SR_MAIN && s.age == AGE_INDUSTRIAL && rr > std::max(r.layers[k].r0, Rt * 0.25) && rr < Rt * 0.8;
                    },
                    DI_HOUSES);
        }
        if (th.ages[k] == AGE_MEDIEVAL && k == 0 && !village && Lk.rng.chance(0.7)) {
            BSpec sp; sp.kind = BK_HALL; sp.style = 1; sp.hw = 5.5 + 1.5 * Lk.rng.uni(); sp.hd = 9 + 4 * Lk.rng.uni(); sp.h = 5 + 2 * Lk.rng.uni(); sp.thick = 0.6;
            front(Lk, sp, [&](const Street& s) { return std::hypot((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5) < plaza + 40; }, DI_CENTRE);
        }
    }
    // the lots: the streets inner first, each side walked from end to end, a building fronting the street where its lot is built
    std::vector<int> order(r.streets.size());
    for (size_t i = 0; i < order.size(); i++) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const Street& A = r.streets[a]; const Street& B = r.streets[b];
        const double da = std::hypot(A.ax + A.bx, A.az + A.bz), db = std::hypot(B.ax + B.bx, B.az + B.bz);
        return da != db ? da < db : a < b;
    });
    int placed = 0, gardens = 0;
    const Street* lotStreet = nullptr;
    // a lot's building fronting its street: a house's door to it (R-412: before its pieces, a porch among them, are tested against the streets),
    // its front yard's depth to it
    auto fronting = [&](size_t i, double off) {
        Building& b = r.buildings[i];
        if (b.kind == BK_HOUSE && b.form != PF_FREE && b.form != PF_COURT) b.door = 0;
        b.yard = std::max(0.0, off - lotStreet->half - b.hd);
    };
    for (int si : order) {
        if (placed >= cap) break;
        const Street s = r.streets[si];
        lotStreet = &s;
        if (s.rank == SR_RING) continue;
        const double L = std::hypot(s.bx - s.ax, s.bz - s.az);
        if (L < 6) continue;
        const double ux = (s.bx - s.ax) / L, uz = (s.bz - s.az) / L, nx = -uz, nz = ux;
        for (int side = -1; side <= 1 && placed < cap; side += 2) {
            double t = 1.0;
            while (t < L - 2 && placed < cap) {
                const double mx = s.ax + ux * t, mz = s.az + uz * t;
                const double qx = mx + nx * side * (s.half + 6), qz = mz + nz * side * (s.half + 6), qr = std::hypot(qx, qz);
                if (qr > Rt + 6) { t += 4; continue; }
                const int lr = layerAt(qr), di = districtAt(qx, qz, lr);
                const int lk = di == DI_TOWERS ? nl - 1 : lr;   // the towers the town's own age's, whatever ring they rose in
                Layout& Lk = *LL[lk];
                const int age = th.ages[lk];
                if (di == DI_CENTRE) { t += 4; continue; }
                BSpec sp; int kind = 0;
                mainFront = s.rank >= SR_MAIN && qr < 0.55 * Rt && (di == DI_HOUSES || di == DI_CORE);
                if (!lotSpec(Lk, di, age, qr / Rt, sp, kind)) { t += 4; continue; }
                const double rankK = s.rank >= SR_MAIN ? 1.35 : (s.rank == SR_LANE ? 0.8 : 1.0);   // the main streets built up the most
                if (Lk.rng.uni() > p0 * density(di, age, qr / Rt) * rankK) { t += 2 * sp.hw + gapAfter(di, age, sp.kind); continue; }   // an empty lot: a yard, a gap
                const double along = t + sp.hw;
                if (along + sp.hw > L - 1) break;
                const double sb = setback(di, age, sp.kind, Lk.G), heading = std::atan2(-nx * side, -nz * side);
                double off = s.half + sb + sp.hd;
                double cx = s.ax + ux * along + nx * side * off, cz = s.az + uz * along + nz * side * off;
                const double reach = std::sqrt(sp.hw * sp.hw + sp.hd * sp.hd);
                if (lr == walledK && std::hypot(cx, cz) + reach > wallR - 1.5) { t += 3; continue; }   // inside its wall
                bool ok = false;
                const size_t before = r.buildings.size();
                if (kind == 1) { if (Lk.circleOffStreets(cx, cz, sp.hw + 0.5) && Lk.dwellingSpot(cx, cz, heading) > 0) ok = true; }
                else if (kind == 2) { if (Lk.circleOffStreets(cx, cz, 6.0) && Lk.tower(cx, cz, heading)) ok = true; }
                else if (kind == 3) {   // a walled garden, its pool
                    const double gw = sp.hw;
                    if (Lk.fits(cx, cz, gw + 1) && Lk.circleOffStreets(cx, cz, gw * 1.42 + 0.5)) {
                        for (int s4 = 0; s4 < 4; s4++) {
                            const bool al = s4 % 2 == 0;
                            const double ox = (s4 == 1 ? gw : (s4 == 3 ? -gw : 0)), oz = (s4 == 0 ? gw : (s4 == 2 ? -gw : 0));
                            const double hc = std::cos(heading), hs = std::sin(heading);
                            Lk.add(BK_WALL, cx + hc * ox + hs * oz, cz - hs * ox + hc * oz, heading + (al ? 0 : PI / 2), gw * (s4 == 0 ? 0.6 : 1.0), 0.3, 0.8, 0.5, false);
                        }
                        Lk.add(BK_FOCUS, cx, cz, heading, gw * 0.4, gw * 0.3, 0.8, 0.5, false, FO_POOL);
                        ok = true; gardens++;
                    }
                } else {
                    Lk.arcadeNext = links == LK_ARCADES && s.rank >= SR_MAIN && qr < 0.4 * Rt && age <= AGE_INDUSTRIAL;
                    Lk.rectNext = sp.rect;
                    ok = Lk.add(sp.kind, cx, cz, heading, sp.hw, sp.hd, sp.h, sp.thick, true, sp.style);
                    if (!ok && sp.hd > 3.5 && sp.kind != BK_HIGHRISE) {   // a shallower building where the block is narrow
                        sp.hd *= 0.65; off = s.half + sb + sp.hd;
                        cx = s.ax + ux * along + nx * side * off; cz = s.az + uz * along + nz * side * off;
                        ok = Lk.add(sp.kind, cx, cz, heading, sp.hw, sp.hd, sp.h, sp.thick, true, sp.style);
                    }
                    Lk.arcadeNext = false;
                    if (ok) fronting(before, off);   // its door to the street before its pieces are tested against the streets
                }
                if (ok) {   // its own pieces off the street (a chimney, a bank, a jetty past its footprint): set back by as much once, else not built
                    const double in1 = Lk.intrusion(before, r.site);
                    if (in1 > 0) {
                        Lk.undoTo(before);
                        ok = false;
                        if (kind == 0) {
                            off += in1 + 0.3;
                            cx = s.ax + ux * along + nx * side * off; cz = s.az + uz * along + nz * side * off;
                            Lk.rectNext = sp.rect;
                            if ((lr != walledK || std::hypot(cx, cz) + reach <= wallR - 1.5) && Lk.add(sp.kind, cx, cz, heading, sp.hw, sp.hd, sp.h, sp.thick, true, sp.style)) {
                                fronting(before, off);
                                if (Lk.intrusion(before, r.site) > 0) Lk.undoTo(before); else ok = true;
                            }
                            Lk.rectNext = false;
                        }
                    }
                }
                Lk.rectNext = false;
                if (!ok) { t += 2.5; continue; }
                double hwNow = sp.hw;
                for (size_t i = before; i < r.buildings.size(); i++) {
                    Building& b = r.buildings[i];
                    b.district = di;
                    if (kind == 1 ||
                            (di != DI_TOWERS && di != DI_WORKS && di != DI_HARBOUR &&
                                    ((age == AGE_INDUSTRIAL && b.kind == BK_ROW) || (age <= AGE_MEDIEVAL && (grouping == GR_ROWS || (mainFront && b.kind == BK_HOUSE))))))
                        b.joined = true;   // in a row or a compound
                    if (i == before && kind == 0) hwNow = std::max(hwNow, b.hw);
                }
                placed += kind == 3 ? 0 : (int)std::max<size_t>(1, r.buildings.size() - before);
                // colonnades along a main street near the centre (a people of colonnades)
                if (links == LK_COLONNADES && kind == 0 && s.rank >= SR_MAIN && qr < 0.35 * Rt && age >= AGE_MEDIEVAL && age <= AGE_INDUSTRIAL)
                    Lk.add(BK_COLONNADE, s.ax + ux * along + nx * side * (s.half + 1.2), s.az + uz * along + nz * side * (s.half + 1.2), std::atan2(ux, uz), 0.5, hwNow * 0.8, 3.5, 0.4, true);
                t = along + hwNow + gapAfter(di, age, sp.kind);
            }
        }
    }
    (void)gardens;
    // a modern city's stadium and its power station, beyond its edge (along a way out; downwind)
    if (townAge == AGE_MODERN && big) {
        for (int tries = 0; tries < 10; tries++) {
            const double Rs = 40 + 25 * rng.uni(), a = (T.exits.empty() ? h0 : T.exits[tries % T.exits.size()]) + 0.45 + rng.sym(0.2), d = Rt + Rs + 30;
            if (Lo.add(BK_STADIUM, std::sin(a) * d, std::cos(a) * d, a + PI, Rs, Rs, 18 + 14 * rng.uni(), 0.5, true)) { r.buildings.back().district = DI_SUBURBS; break; }
        }
        if (G.of(GV_CENTRE) != CE_MACHINE)
            for (int tries = 0; tries < 8; tries++) {
                const double a = worksDir + rng.sym(0.4), d = Rt + 120 + 40 * tries;
                if (Lo.add(BK_POWER, std::sin(a) * d, std::cos(a) * d, a + PI, 45 + 15 * rng.uni(), 40 + 10 * rng.uni(), 1, 0.5, true)) { r.buildings.back().district = DI_WORKS; break; }
            }
    }
    // the wall: a ring round the walled layer, its gates where the main streets go through it (an arch over the street between its towers),
    // the stretches between them, a breach where a lesser street crosses; an old core's inside a later city mostly gone (stretches kept)
    if (r.walled) {
        Layout& Lw = *LL[walledK];
        const bool old = walledK + 1 < nl;
        const bool pal = edge == ED_DITCHES;
        double hWall = 4.0 + 2.0 * rng.uni(); if (pal) hWall *= 0.65;
        // the gates
        std::vector<std::pair<double, double>> gates;   // the angle, the half width of the street
        for (const Street& s : r.streets) {
            if (s.rank < SR_MAIN) continue;
            const double dx = s.bx - s.ax, dz = s.bz - s.az, A = dx * dx + dz * dz, B = 2 * (s.ax * dx + s.az * dz), C = s.ax * s.ax + s.az * s.az - wallR * wallR, D = B * B - 4 * A * C;
            if (A < 1e-9 || D < 0) continue;
            for (int e = -1; e <= 1; e += 2) {
                const double tt = (-B + e * std::sqrt(D)) / (2 * A);
                if (tt < 0 || tt > 1) continue;
                const double a = std::atan2(s.ax + dx * tt, s.az + dz * tt);
                bool dup = false; for (auto& gt : gates) if (std::fabs(std::remainder(gt.first - a, TAU)) * wallR < 12) dup = true;
                if (!dup) gates.push_back({a, s.half});
            }
        }
        for (const auto& gt : gates) {
            const double a = gt.first, gx = std::sin(a) * wallR, gz = std::cos(a) * wallR, hwg = std::max(1.9, gt.second + 0.8);
            if (old && Lw.rng.chance(0.3)) continue;   // a gate pulled down with its wall
            const size_t n0 = r.buildings.size();
            if (Lw.add(BK_GATE, gx, gz, a, hwg, 0.6, hWall + 1.0, 0.8, false)) {
                if (Lw.intrusion(n0, r.site, gx, gz) > 0) { Lw.undoTo(n0); continue; }   // its piers off the streets beside it
                r.buildings.back().district = old ? DI_CORE : DI_HOUSES;
            }
        }
        const int segs = std::max(10, (int)(TAU * wallR / (big ? 22.0 : 14.0)));
        for (int k = 0; k < segs; k++) {
            const double aA = h0 + k * TAU / segs, aB = h0 + (k + 1) * TAU / segs;
            const double xA = std::sin(aA) * wallR, zA = std::cos(aA) * wallR, xB = std::sin(aB) * wallR, zB = std::cos(aB) * wallR;
            const double mx = (xA + xB) * 0.5, mz = (zA + zB) * 0.5, dx = xB - xA, dz = zB - zA, len = std::hypot(dx, dz);
            const double wh = std::atan2(dx, dz) + PI / 2;
            bool nearGate = false;
            for (const auto& gt : gates) if (std::fabs(std::remainder(gt.first - (aA + aB) * 0.5, TAU)) * wallR < len * 0.5 + gt.second + 3) nearGate = true;
            if (nearGate) continue;
            if (!Lw.offStreets(mx, mz, wh, len * 0.5, 0.45)) continue;   // a breach where a lane goes through
            if (Lw.rng.chance(old ? 0.55 : 0.12)) continue;            // a fallen stretch (an old core's wall mostly gone)
            if (!pal && c.towers && k % 2 == 0 && Lw.offStreets(xA, zA, aA, 3.4, 3.4)) {
                const size_t n0 = r.buildings.size();
                if (Lw.add(BK_WALL_TOWER, xA, zA, aA, 2.6 + 0.8 * rng.uni(), 0, (hWall + 3 + 2 * rng.uni()) * (0.8 + 0.4 * G.at(GV_HEIGHT)), 0.5, false) && Lw.intrusion(n0, r.site) > 0) Lw.undoTo(n0);
            }
            if (Lw.add(pal ? BK_PALISADE : BK_WALL, mx, mz, wh, len * 0.5, 0.45, hWall, 0.9, false)) r.buildings.back().district = old ? DI_CORE : DI_HOUSES;
        }
    }
    // the people's relation to the land (a high place toward the ridge, an aqueduct down from the high ground) and its tunnels
    {
        const int land = G.of(GV_LAND);
        const double rd = st.ridgeDir;
        if (land == LD_RIDGES)
            for (int k = 0; k < 4; k++) {
                const double d = Rt * (0.72 + 0.1 * k);
                if (Lo.add(BK_PLATFORM, std::sin(rd) * d, std::cos(rd) * d, rd + PI, 7 + 3 * rng.uni(), 7 + 3 * rng.uni(), 3.3, 0.5, true)) break;
            }
        if (land == LD_VALLEYS) {
            const double len = 70 + 50 * rng.uni(), d = Rt * 1.04 + len * 0.5 + 6;
            Lo.add(BK_AQUEDUCT, std::sin(rd) * d, std::cos(rd) * d, rd, 1.4, len * 0.5, (9 + 9 * rng.uni()) * (0.8 + 0.4 * G.at(GV_AMBITION)), 1.2, true);
        }
        if (links == LK_TUNNELS)
            for (int k = 0, made = 0; k < 12 && made < 3; k++) {
                const double a = rng.uni() * TAU, d = Rt * (0.3 + 0.6 * rng.uni());
                if (Lo.add(BK_PORTAL, std::sin(a) * d, std::cos(a) * d, a, 2.4, 3.5, 4.8, 0.8, true)) made++;
            }
    }
    // the works of the water and the land in the town's own age (C-08's by the age: the harbour, the terraces, the cistern or the reservoir)
    const double rSaved = r.size; r.size = Rt;
    addFeatures(Lo, r, Lo.c);
    r.size = rSaved;
    // the fields round it
    townFields(T, townAge, st, kindHas(livingKindOf(g.type), KT_DRY), r.fields);   // the fields of its time
    placeFurniture(Lo, r, Rt);
}
