// E-03: the peoples' genome (A-04): `genome`, its tiles and its checks
#include "harness/harness.h"
#include "harness/test_worlds.h"
bool formRests(const std::vector<Form>& el, size_t i) {   // a piece over the ground has another of its building under it that reaches it
    const Form& f = el[i];
    if (f.y0 < 0.6 || f.tilt > 0.3 || f.kind == FK_CABLE) return true;
    auto foot = [](const Form& e) { return e.kind == FK_BOX ? std::hypot(e.hx, e.hz) : formReach(e); };
    for (size_t j = 0; j < el.size(); j++) {
        if (j == i || el[j].building != f.building) continue;
        const Form& g = el[j];
        if (formTop(g) < f.y0 - 0.6 || g.y0 > f.y0 - 0.05) continue;
        if (std::hypot(g.x - f.x, g.z - f.z) <= foot(g) + foot(f) + 0.1) return true;
    }
    return false;
}
std::vector<NearPeople> peoplesNearHome(int want, int R) {
    std::vector<NearPeople> out;
    struct St { int64_t x, y, z; double d; };
    std::vector<St> stars;
    for (int64_t x = HOME_SX - R; x <= HOME_SX + R; x++) for (int64_t y = HOME_SY - R; y <= HOME_SY + R; y++) for (int64_t z = HOME_SZ - R; z <= HOME_SZ + R; z++) {
        double d = std::sqrt((double)((x - HOME_SX) * (x - HOME_SX) + (y - HOME_SY) * (y - HOME_SY) + (z - HOME_SZ) * (z - HOME_SZ)));
        Star s; if (d > R || !starInSector(x, y, z, s, false)) continue;
        stars.push_back({x, y, z, d});
    }
    std::sort(stars.begin(), stars.end(), [](const St& a, const St& b) { return a.d != b.d ? a.d < b.d : (a.x != b.x ? a.x < b.x : (a.y != b.y ? a.y < b.y : a.z < b.z)); });
    for (const St& n : stars) {
        Star s; starInSector(n.x, n.y, n.z, s, true);
        StarSystem sys; sys.generate(s);
        for (const Body& b : sys.bodies) {
            if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
            BodyGen g = BodyGen::make(b);
            const int np = peoplesOf(g);
            for (int k = 0; k < np; k++) {
                Culture c = cultureOf(g, k);
                out.push_back({n.x, n.y, n.z, n.d, b.index, k, peopleGen(g, k).seed, b.name + (k ? " (second)" : ""), c.age, c.ageYears});
                if ((int)out.size() >= want) return out;
            }
        }
    }
    return out;
}
// A-04: a culture on a world (its ground, its wet, its family) at an age and its years since the end, its genome and its ways another
// people's: the sheets set peoples side by side on one ground under one light
Culture cultureWithGenome(const BodyGen& g, int age, double years, uint64_t peopleSeed) {
    Culture c = cultureAs(g, 0, age, years);
    c.genome = genomeOf(peopleSeed);
    cultureWays(c, peopleSeed);
    return c;
}
// A-04: a genome as lines of a sheet (the variables in their order, about fifty characters a line), the technique of the age last
static std::vector<std::string> genomeLines(const Genome& G, int age, uint64_t seed, size_t width = 52) {
    std::vector<std::string> L; std::string line;
    for (int i = 0; i < GV_COUNT; i++) {
        const GenomeVarInfo& V = GENOME_VARS[i];
        std::string tok = std::string(V.name) + " " + (V.n > 0 ? std::string(V.values[G.of(i)]) : fmt("%.2f", G.at(i)));
        if (!line.empty() && line.size() + 2 + tok.size() > width) { L.push_back(line); line.clear(); }
        line += (line.empty() ? "" : "  ") + tok;
    }
    std::string tq = std::string("BUILT BY ") + TECHNIQUE_NAMES[techniqueOf(G, age, seed)];
    if (line.size() + 2 + tq.size() > width) { L.push_back(line); line = tq; } else line += "  " + tq;
    L.push_back(line);
    return L;
}
// A-04: a drawing's hash (the unit's reach and determinism checks): every piece's numbers, the colours of the slots the pieces use and
// the windows where a piece carries them
static uint64_t drawingHash(const std::vector<Form>& el, const FormPalette& P) {
    uint64_t h = 1469598103934665603ULL;
    auto mixd = [&](double v) { int64_t q = (int64_t)std::llround(v * 1000.0); h = (h ^ (uint64_t)q) * 1099511628211ULL; };
    bool slot[4] = {false, false, false, false}; bool pat = false;
    for (const Form& e : el) {
        mixd(e.kind); mixd(e.sub); mixd(e.x); mixd(e.z); mixd(e.heading); mixd(e.hx); mixd(e.hz); mixd(e.y0); mixd(e.y1);
        for (int k = 0; k < 4; k++) mixd(e.p[k]);
        mixd(e.sides); mixd(e.part); mixd(e.slot); mixd(e.pattern); mixd(e.tilt); mixd(e.broken); mixd(e.material); mixd(e.glyphs); mixd(e.tone);
        if (e.part < 7 || e.part == 4) slot[clampi(e.slot, 0, 3)] = true;
        if (e.pattern > 0) pat = true;
    }
    for (int k = 0; k < 4; k++) if (slot[k]) { mixd(P.colour[k].r); mixd(P.colour[k].g); mixd(P.colour[k].b); mixd(P.own[k]); mixd(P.tint[k]); }
    if (slot[3]) mixd(P.glow);
    if (pat) for (int k = 0; k < P.nWindows; k++) {
        const WindowSpec& W = P.windows[k]; mixd(W.bay); mixd(W.storey); mixd(W.w); mixd(W.h); mixd(W.sill); mixd(W.shape); mixd(W.group); mixd(W.scatter); mixd(W.lit);
        mixd(W.frame); mixd(W.frameShade); mixd(W.shutter); mixd(W.shutterShade); mixd(W.stripe); mixd(W.stripeShade); mixd(W.dado); mixd(W.dadoShade);   // R-412
    }
    return h;
}
// A-04: what a culture draws at a cell: the settlement of a class (the far future's monument), its pieces whole, its graves round it
static uint64_t cultureDrawing(const BodyGen& g, const StarSystem& sys, int gLat, int gLon, const Culture& c, int sclass, int* pieces = nullptr) {
    RuinSpec sp; settlementOf(g, gLat, gLon, c, c.age == AGE_FAR_FUTURE ? SC_MONUMENT : sclass, sp);
    std::vector<Form> el; ruinElements(sp, c, 0, el);
    const Lore L = loreOf(sys, sys.bodies[0], g, 0);
    std::vector<Grave> gr; gravesOf(L, sp, c, gr); graveElements(gr, c, 0, el);
    if (pieces) *pieces = (int)el.size();
    return drawingHash(el, c.palette);
}
// A-04: the genome's checks: deterministic (drawn twice, on the workers as serially: the same genomes, ways and drawings), every variable
// reaching the drawing in every age it says it acts in (each of its values set in three peoples' genomes in turn: some two differ),
// and the distinctness of the hundred peoples nearest home (no pair under `GENOME_ALIKE` on the strongest variables)
static int genomeReach(const BodyGen& g, const StarSystem& sys, int gLat0, int gLon0, std::string& report, bool verbose) {
    const uint64_t BASES[3] = {0xA04F00D1ULL, 0xA04F00D2ULL ^ 0x5DEECE66DULL, 0xA04F00D3ULL * 0x9E3779B97F4A7C15ULL};
    int missing = 0;
    report.clear();
    for (int v = 0; v < GV_COUNT; v++) {
        const GenomeVarInfo& V = GENOME_VARS[v];
        std::string ages;
        for (int a = 0; a < AGE_COUNT; a++) {
            if (!(V.ages >> a & 1)) continue;
            bool reached = false;
            for (int bi = 0; bi < 3 && !reached; bi++) {
                Culture c = cultureAs(g, 0, a, 30);
                c.genome = genomeOf(BASES[bi]);
                const int vals = V.n > 0 ? std::min(V.n, 7) : 3;
                uint64_t first = 0;
                for (int k = 0; k < vals && !reached; k++) {
                    Genome G = genomeOf(BASES[bi]);
                    if (V.n > 0) G.set(v, (k * ((V.n / vals) | 1)) % V.n); else G.v[v] = 0.1 + 0.4 * k;   // a step prime to the count: the accent's colours and places both
                    Culture cv = c; cv.genome = G; cultureWays(cv, BASES[bi]);
                    const int sclass = a == AGE_PREHISTORIC ? SC_TOWN : SC_TOWN;
                    uint64_t h = 0;
                    for (int cell = 0; cell < 2; cell++) h = h * 31 + cultureDrawing(g, sys, gLat0 + cell * 7, gLon0 + bi * 5, cv, cell ? SC_VILLAGE : sclass);
                    if (k == 0) first = h; else if (h != first) reached = true;
                }
            }
            if (!reached) { missing++; ages += fmt("%s%s NOT", ages.empty() ? "" : ", ", AGE_NAMES[a]); }
            else if (verbose) ages += fmt("%s%s", ages.empty() ? "" : ", ", AGE_NAMES[a]);
        }
        if (verbose || ages.find("NOT") != std::string::npos) report += fmt("%s%s: %s", report.empty() ? "" : "; ", V.name, ages.c_str());
    }
    return missing;
}
void genomeChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    Star st; StarSystem sys; BodyGen g; bool world = false;
    if (starInSector(AGE_WORLDS[AGE_MEDIEVAL].sx, 0, AGE_WORLDS[AGE_MEDIEVAL].sz, st, true)) { sys.generate(st); if (AGE_WORLDS[AGE_MEDIEVAL].body < (int)sys.bodies.size()) { g = BodyGen::make(sys.bodies[AGE_WORLDS[AGE_MEDIEVAL].body]); world = true; } }
    if (!world) { check("the genome (A-04)", false, "no test world"); return; }
    int gLat0, gLon0; ruinCellOf(g, 0.3, 1.1, gLat0, gLon0);
    double t0 = nowSec();
    const std::vector<NearPeople> near = peoplesNearHome(100);
    const double tNear = nowSec() - t0;
    {   // deterministic: twice, and on the workers
        bool same = true; int n = 0;
        std::vector<uint64_t> serial(near.size()), threaded(near.size());
        for (size_t i = 0; i < near.size(); i++) {
            const Genome a = genomeOf(near[i].seed), b = genomeOf(near[i].seed);
            if (!(a == b)) same = false;
            Culture c1 = cultureWithGenome(g, near[i].age, 30, near[i].seed), c2 = cultureWithGenome(g, near[i].age, 30, near[i].seed);
            if (!(c1.genome == c2.genome) || c1.roof != c2.roof || c1.temple != c2.temple || c1.towerForm != c2.towerForm || c1.great != c2.great || c1.technique != c2.technique || c1.tall != c2.tall || c1.life != c2.life) same = false;
            if (i < 24) { serial[i] = cultureDrawing(g, sys, gLat0 + (int)i, gLon0, c1, SC_TOWN); if (serial[i] != cultureDrawing(g, sys, gLat0 + (int)i, gLon0, c2, SC_TOWN)) same = false; n++; }
        }
        parallelFor(24, 1, [&](int i0, int i1) { for (int i = i0; i < i1 && i < (int)near.size(); i++) { Culture c = cultureWithGenome(g, near[i].age, 30, near[i].seed); threaded[i] = cultureDrawing(g, sys, gLat0 + i, gLon0, c, SC_TOWN); } });
        for (int i = 0; i < n; i++) if (serial[i] != threaded[i]) same = false;
        check("the genome is deterministic (A-04)", same && near.size() >= 100, fmt("%zu peoples near home (%.1f s to find): their genomes and ways twice the same, %d towns drawn twice and on the workers the same", near.size(), tNear, n));
    }
    {   // every variable reaches the drawing in the ages it acts in
        std::string report; const double t1 = nowSec();
        const int missing = genomeReach(g, sys, gLat0 + 30, gLon0, report, false);
        check("every variable reaches the drawing (A-04)", missing == 0, fmt("%d variables, each of its values in three peoples' genomes in every age it acts in, %.1f s%s%s", GV_COUNT, nowSec() - t1, missing ? ": " : "", report.c_str()));
    }
    {   // distinct: no two of the hundred alike on the strongest variables
        double best = 1e9; size_t bi = 0, bj = 0; int under = 0; std::string shared;
        for (size_t i = 0; i < near.size(); i++) for (size_t j = i + 1; j < near.size(); j++) {
            const double d = genomeDistance(genomeOf(near[i].seed), genomeOf(near[j].seed));
            if (d < GENOME_ALIKE) under++;
            if (d < best) { best = d; bi = i; bj = j; }
        }
        if (near.size() > 1) genomeDistance(genomeOf(near[bi].seed), genomeOf(near[bj].seed), &shared);
        check("no two of a hundred peoples alike (A-04)", under == 0 && near.size() >= 100,
              fmt("%zu peoples near home, %zu pairs; the nearest pair %.2f of %.1f apart (alike under %.1f): %s and %s, sharing %s", near.size(), near.size() * (near.size() - 1) / 2, best, genomeMaxDistance(), GENOME_ALIKE,
                  near.empty() ? "-" : near[bi].world.c_str(), near.empty() ? "-" : near[bj].world.c_str(), shared.c_str()));
    }
}

// A-04: `genome sheet [scale] [years] [town|village|hamlet] [desert|felisian]`: the architecture sheet, a page an age
// (`shots/tests/genome_<age>_<scale>x.png`): the twelve peoples nearest home, each as if it had reached the page's age, a settlement of
// the class at the same cell of the scene world under the same light and the same years since the end (30 unless given), its genome
// printed under it. `genome ages [scale] [peoples] [years]`: the same peoples (rows) at every age (columns), one page
// (`genome_ages_<scale>x.png`): does a people read as itself? `genome distinct [n]`: the distinctness of the n peoples nearest home
// (100). `genome print <sx> <sz> <body> [people]`: a people's genome and its techniques by age. `genome reach`: every variable's ages
static void genomeTile(SurfaceView& sv, SpaceRenderer& sr, StarNeighborhood& nb, double t, const BodyGen& g, int gLat, int gLon, const Culture& cu, int sclass, Framebuffer& fb, std::vector<uint32_t>& rgbBuf, std::string* line = nullptr) {
    RuinSpec sp; settlementOf(g, gLat, gLon, cu, cu.age == AGE_FAR_FUTURE ? SC_MONUMENT : sclass, sp);
    std::vector<Form> pieces[3];
    for (int l = 0; l < 3; l++) ruinElements(sp, cu, l, pieces[l]);
    double hMax = 0, reach = sp.size;
    for (const Form& e : pieces[0]) { hMax = std::max(hMax, formTop(e)); reach = std::max(reach, std::hypot(e.x, e.z) * 0.8); }
    sv.testClearPlaced();
    sv.cultures[0] = cu; sv.palettes[0] = cu.palette;
    sv.testPlaceSettlement(0, 0, sp, pieces);
    const double h0 = sp.heading + 0.6;
    const double D = cu.age == AGE_FAR_FUTURE ? std::max(reach * 2.4, hMax * 1.7) : (cu.age == AGE_FUTURISTIC ? std::max(reach * 2.3, hMax * 0.95) : std::max(reach * 1.35 + 25, std::min(hMax, 900.0) * 0.62)), eye = 2 + D * 0.12;
    sv.player.x = std::sin(h0) * D; sv.player.z = std::cos(h0) * D;
    formsCamera(sv, eye, 0, sv.site.surfaceHeight(0, 0) + std::min((cu.age == AGE_FUTURISTIC ? hMax : std::min(hMax, 900.0)) * 0.3, 0.35 * D), 0, t);
    sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(rgbBuf.data(), 1.0);
    if (line) {
        std::map<std::string, int> kinds; for (const Building& b : sp.buildings) kinds[BUILDING_KIND_NAMES[b.kind]]++;
        std::string ks; for (auto& k : kinds) ks += fmt("%s%d %s", ks.empty() ? "" : ", ", k.second, k.first.c_str());
        *line = fmt("%zu buildings (%s), %zu pieces, the tallest %.0f m", sp.buildings.size(), ks.c_str(), pieces[0].size(), hMax);
    }
}
// B-415: a people's town from inside it: from a spot half way out (of 24 round it, the one 4 m clear of every building with the most of
// them ahead), looking at its middle at a walker's height; `drone` 35 m up at its edge, as the drone sees it
static void genomeStreetTile(SurfaceView& sv, SpaceRenderer& sr, StarNeighborhood& nb, double t, const BodyGen& g, int gLat, int gLon, const Culture& cu, bool drone, Framebuffer& fb, std::vector<uint32_t>& rgbBuf, std::string* line = nullptr) {
    RuinSpec sp; settlementOf(g, gLat, gLon, cu, SC_TOWN, sp);
    std::vector<Form> pieces[3];
    for (int l = 0; l < 3; l++) ruinElements(sp, cu, l, pieces[l]);
    sv.testClearPlaced();
    sv.cultures[0] = cu; sv.palettes[0] = cu.palette;
    sv.testPlaceSettlement(0, 0, sp, pieces);
    const double R = std::max(30.0, sp.size);
    double bx = 0, bz = R * 0.5; int best = -2;
    for (int ring = 0; ring < 3; ring++)
        for (int k = 0; k < 24; k++) {
            const double a = sp.heading + k * TAU / 24, rr = R * (drone ? 0.95 : (ring == 0 ? 0.5 : (ring == 1 ? 0.65 : 0.35))), px = std::sin(a) * rr, pz = std::cos(a) * rr;
            const double ux = -px / rr, uz = -pz / rr;   // toward the middle
            bool clear = true; int ahead = 0;
            for (const Building& b : sp.buildings) {
                const double vx = b.x - px, vz = b.z - pz, vl = std::hypot(vx, vz), rb = std::max(b.hw, b.hd);
                const double along = clampd(vx * ux + vz * uz, 0, 18), off = std::hypot(vx - ux * along, vz - uz * along);   // the first 18 m of the sight line kept clear
                if (!drone && (vl - rb < 4 || off - rb < 1.5)) clear = false;
                if (vl > 1 && vl < R * 1.1 && (vx * ux + vz * uz) / vl > 0.8) ahead++;
            }
            const int score = clear ? ahead : -1;
            if (score > best) { best = score; bx = px; bz = pz; }
        }
    sv.player.x = bx; sv.player.z = bz;
    formsCamera(sv, drone ? 35 : 1.8, 0, sv.site.surfaceHeight(0, 0) + (drone ? 0 : 5), 0, t);
    sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(rgbBuf.data(), 1.0);
    if (line) {
        std::map<std::string, int> kinds; for (const Building& b : sp.buildings) kinds[BUILDING_KIND_NAMES[b.kind]]++;
        std::string ks; for (auto& k : kinds) ks += fmt("%s%d %s", ks.empty() ? "" : ", ", k.second, k.first.c_str());
        *line = fmt("%zu buildings (%s), seen from %.0f m out", sp.buildings.size(), ks.c_str(), std::hypot(bx, bz));
    }
}
// R-412: a little street of a people's dwellings in an age under the genome `G`, seen from the street (or from over it where `high`): its
// dwellings fronting the street in their yards by the layout's own rules (their storeys, their frontage, their setback), a piece of its
// furniture at the kerb
static void genomeDressTile(SurfaceView& sv, SpaceRenderer& sr, StarNeighborhood& nb, double t, const BodyGen& g, int age, const Genome& G, uint64_t seed, bool high, double years, Framebuffer& fb, std::vector<uint32_t>& rgbBuf) {
    Culture cu = cultureAs(g, 0, age, years);
    cu.genome = G; cultureWays(cu, seed);
    RuinSpec sp; sp.size = 40; sp.seed = seed ^ 0xD2E55ULL; sp.sclass = SC_VILLAGE; sp.heading = 0;
    const double st = age >= AGE_MEDIEVAL && age <= AGE_FUTURISTIC ? 0.82 + 0.56 * G.at(GV_STOREYS) : 1.0, S = G.at(GV_SETBACK);
    const double yard = age == AGE_MEDIEVAL ? 0.4 + 4.5 * S * S : (age == AGE_INDUSTRIAL ? 0.8 + 5.0 * S * S : (age == AGE_MODERN ? 2.0 + 7.0 * S * S : 0.0));
    struct Lot { int kind; double x, hw, hd, h, thick; };
    std::vector<Lot> lots;
    switch (age) {
        case AGE_PREHISTORIC: lots = {{BK_HUT, -9, 2.6, 2.6, 1.9, 0.5}, {BK_LONGHOUSE, 0.5, 2.8, 6.0, 1.9, 0.8}, {BK_HUT, 9.5, 2.4, 2.4, 1.8, 0.5}}; break;
        case AGE_MEDIEVAL: lots = {{BK_HOUSE, -10, 3.4, 4.2, 3.0 * st, 0.45}, {BK_HOUSE, 0, 3.0, 4.6, 3.6 * st, 0.45}, {BK_HOUSE, 10, 3.6, 4.0, 2.8 * st, 0.45}}; break;
        case AGE_INDUSTRIAL: lots = {{BK_VILLA, -16, 4.8, 4.6, 7.0 * st, 0.45}, {BK_ROW, 0, 8.5, 4.4, 7.5 * st, 0.45}, {BK_VILLA, 16, 4.6, 4.8, 7.0 * st, 0.45}}; break;
        case AGE_MODERN: lots = {{BK_SUBURB, -18, 4.2, 5.0, 5.8 * std::min(1.4, st), 0.35}, {BK_BLOCK, 0, 10, 6.0, 16 * st, 0.4}, {BK_SUBURB, 18, 4.0, 5.0, 2.9 * std::min(1.4, st), 0.35}}; break;
        default: lots = {{BK_POD, -13, 9, 9, 3.4 * std::min(1.5, st), 0.5}, {BK_POD, 13, 8, 8, 3.4 * std::min(1.5, st), 0.5}}; break;
    }
    const double half = 3.0;   // the street's half width, along x at z = 0
    for (size_t k = 0; k < lots.size(); k++) {
        const Lot& l = lots[k];
        Building b; b.kind = l.kind; b.hw = l.hw; b.hd = l.hd; b.hWall = l.h * cu.tall; b.thick = l.thick;
        if ((age == AGE_MEDIEVAL || age == AGE_INDUSTRIAL) && (l.kind == BK_HOUSE || l.kind == BK_VILLA) && (G.of(GV_FRONTAGE) == FR_GABLES ? b.hw > b.hd : b.hd > b.hw)) std::swap(b.hw, b.hd);
        b.x = l.x; b.z = half + yard + b.hd; b.heading = PI; b.door = 0; b.age = age; b.seed = mix64(seed + 977 * (k + 1)); b.style = 0;
        b.form = l.kind == BK_HUT || l.kind == BK_POD ? (G.of(GV_PLAN) == PF_ELLIPSE ? PF_ELLIPSE : (G.of(GV_PLAN) == PF_POLYGON ? PF_POLYGON : PF_CIRCLE)) : PF_RECT;
        b.sides = G.number(); b.yard = yard; b.luck = 0;
        sp.buildings.push_back(b);
    }
    if (G.of(GV_FURNITURE) != FU_NONE && age >= AGE_MEDIEVAL && age <= AGE_FUTURISTIC) {   // at the kerb, between the first two
        Building f; f.kind = BK_FURNITURE; f.style = G.of(GV_FURNITURE); f.age = age; f.seed = mix64(seed ^ 0xF0F0ULL);
        const double S2 = f.style == FU_FOUNTAINS ? 2.7 : (f.style == FU_LAMPS ? 0.25 : 1.0);
        f.hw = f.hd = S2; f.hWall = 2; f.x = 0.5 * (lots[0].x + lots[1].x); f.z = f.style == FU_LAMPS ? half + 0.8 : half + S2 + 0.6; f.heading = PI;
        sp.buildings.push_back(f);
    }
    std::vector<Form> pieces[3];
    for (int l = 0; l < 3; l++) ruinElements(sp, cu, l, pieces[l]);
    if (getenv("DRESS_DEBUG")) {
        printf("    roof %d, tall %.2f, decay timber %.2f, the first's pieces:\n", cu.roof, cu.tall, buildingDecay(sp.buildings[0], cu, MT_TIMBER));
        for (const Form& e : pieces[0]) if (e.building == 0) printf("      %s %d part %d slot %d y %.2f..%.2f hx %.2f hz %.2f broken %.2f\n", FORM_KIND_NAMES[e.kind], e.sub, e.part, e.slot, e.y0, e.y1, e.hx, e.hz, e.broken);
    }
    sv.testClearPlaced();
    sv.cultures[0] = cu; sv.palettes[0] = cu.palette;
    sv.testPlaceSettlement(0, 0, sp, pieces);
    const double D = age == AGE_MODERN ? 27 : (age == AGE_INDUSTRIAL ? 23 : (age == AGE_FUTURISTIC ? 30 : 13));
    sv.player.x = 0; sv.player.z = -D;
    const double gy = sv.site.surfaceHeight(0, 6);
    formsCamera(sv, high ? D * 0.5 : 1.8, 0, gy + (high ? 1.0 : (age == AGE_MODERN ? 7 : (age == AGE_INDUSTRIAL ? 4.5 : 2.6))), 6, t);
    sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(rgbBuf.data(), 1.0);
}

int runGenome(int argc, char** argv) {
    const std::string what = argc > 2 ? argv[2] : "sheet";
    if (what == "print" && argc >= 6) {
        int64_t sx = atoll(argv[3]), sz = atoll(argv[4]); int bi = atoi(argv[5]), which = argc > 6 ? atoi(argv[6]) : 0;
        Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
        StarSystem sys; sys.generate(s);
        if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d\n", bi); return 1; }
        BodyGen g = BodyGen::make(sys.bodies[bi]);
        if (which >= peoplesOf(g)) { printf("%s: no people %d\n", sys.bodies[bi].name.c_str(), which); return 1; }
        const uint64_t seed = peopleGen(g, which).seed; const Genome G = genomeOf(seed); const Culture c = cultureOf(g, which);
        printf("%s, people %d (%s, ended %.0f years ago): its architecture genome\n", sys.bodies[bi].name.c_str(), which + 1, AGE_NAMES[c.age], c.ageYears);
        for (int i = 0; i < GV_COUNT; i++) printf("  %-8s %-10s %s\n", GENOME_VARS[i].group, GENOME_VARS[i].name, genomeValue(G, i).c_str());
        printf("  techniques:"); for (int a = 0; a < AGE_COUNT; a++) printf(" %s %s;", AGE_NAMES[a], TECHNIQUE_NAMES[techniqueOf(G, a, seed)]); printf("\n");
        return 0;
    }
    if (what == "distinct") {
        const int n = argc > 3 ? std::max(2, atoi(argv[3])) : 100;
        const double t0 = nowSec();
        const std::vector<NearPeople> P = peoplesNearHome(n);
        struct Pair { double d; size_t i, j; };
        std::vector<Pair> pairs; int hist[16] = {0};
        for (size_t i = 0; i < P.size(); i++) for (size_t j = i + 1; j < P.size(); j++) {
            const double d = genomeDistance(genomeOf(P[i].seed), genomeOf(P[j].seed));
            pairs.push_back({d, i, j}); hist[clampi((int)d, 0, 15)]++;
        }
        std::sort(pairs.begin(), pairs.end(), [](const Pair& a, const Pair& b) { return a.d < b.d; });
        printf("genome distinct: %zu peoples near home (%.1f s), %zu pairs, the strongest variables' distance 0 .. %.1f, alike under %.1f\n", P.size(), nowSec() - t0, pairs.size(), genomeMaxDistance(), GENOME_ALIKE);
        printf("  pairs by distance:"); for (int k = 0; k < 16; k++) if (hist[k]) printf(" %d-%d: %d;", k, k + 1, hist[k]); printf("\n");
        for (size_t k = 0; k < pairs.size() && k < 8; k++) {
            std::string sh; genomeDistance(genomeOf(P[pairs[k].i].seed), genomeOf(P[pairs[k].j].seed), &sh);
            printf("  %.2f  %s (%lld %lld %lld) and %s (%lld %lld %lld): sharing %s\n", pairs[k].d, P[pairs[k].i].world.c_str(), (long long)P[pairs[k].i].x, (long long)P[pairs[k].i].y, (long long)P[pairs[k].i].z,
                   P[pairs[k].j].world.c_str(), (long long)P[pairs[k].j].x, (long long)P[pairs[k].j].y, (long long)P[pairs[k].j].z, sh.c_str());
        }
        int under = 0; for (const Pair& p : pairs) if (p.d < GENOME_ALIKE) under++;
        // the share of random pairs under each distance, from 20,000 genomes drawn at random (how likely a pair alike is, anywhere)
        Rng rr(0xA04D157ULL); int tail[8] = {0}; const int N = 200000;
        for (int k = 0; k < N; k++) { const double d = genomeDistance(genomeOf(rr.next()), genomeOf(rr.next())); for (int q = 0; q < 8; q++) if (d < 1.0 + q * 0.5) tail[q]++; }
        printf("  random pairs under:"); for (int q = 0; q < 8; q++) printf(" %.1f: %.5f%%;", 1.0 + q * 0.5, 100.0 * tail[q] / N); printf("\n");
        printf("  %s: %d pairs alike\n", under == 0 ? "DISTINCT" : "NOT DISTINCT", under);
        if (!pairs.empty()) printf("  the nearest pair to look at: `vesperis_test genome pair %lld %lld %lld %d %d %lld %lld %lld %d %d`\n", (long long)P[pairs[0].i].x, (long long)P[pairs[0].i].y, (long long)P[pairs[0].i].z, P[pairs[0].i].body, P[pairs[0].i].which,
                                   (long long)P[pairs[0].j].x, (long long)P[pairs[0].j].y, (long long)P[pairs[0].j].z, P[pairs[0].j].body, P[pairs[0].j].which);
        return under == 0 ? 0 : 1;
    }
    if (what == "why" && argc > 4) {   // `genome why <VARIABLE> <age>`: the reach check's three peoples, each value's pieces in the town and the village (what the variable acts on)
        Star st; StarSystem sys; if (!starInSector(AGE_WORLDS[AGE_MEDIEVAL].sx, 0, AGE_WORLDS[AGE_MEDIEVAL].sz, st, true)) return 1;
        sys.generate(st); BodyGen g = BodyGen::make(sys.bodies[AGE_WORLDS[AGE_MEDIEVAL].body]);
        int gLat0, gLon0; ruinCellOf(g, 0.3, 1.1, gLat0, gLon0); gLat0 += 30;
        int v = -1; for (int i = 0; i < GV_COUNT; i++) if (std::string(GENOME_VARS[i].name) == argv[3]) v = i;
        int a = -1; for (int i = 0; i < AGE_COUNT; i++) if (std::string(AGE_NAMES[i]) == argv[4]) a = i;
        if (v < 0 || a < 0) { printf("no such variable or age\n"); return 1; }
        const uint64_t BASES[3] = {0xA04F00D1ULL, 0xA04F00D2ULL ^ 0x5DEECE66DULL, 0xA04F00D3ULL * 0x9E3779B97F4A7C15ULL};
        for (int bi = 0; bi < 3; bi++) {
            Culture c = cultureAs(g, 0, a, 30); c.genome = genomeOf(BASES[bi]); cultureWays(c, BASES[bi]);
            printf("people %d: roof %d, profile %d, massing %d, modules %d, plan %d\n", bi, c.roof, c.genome.of(GV_PROFILE), c.genome.of(GV_MASSING), c.genome.of(GV_MODULAR), c.genome.of(GV_PLAN));
            const int n = GENOME_VARS[v].n > 0 ? std::min(GENOME_VARS[v].n, 7) : 3;
            for (int k = 0; k < n; k++) {
                Genome G = genomeOf(BASES[bi]); if (GENOME_VARS[v].n > 0) G.set(v, (k * ((GENOME_VARS[v].n / n) | 1)) % GENOME_VARS[v].n); else G.v[v] = 0.1 + 0.4 * k;
                Culture cv = c; cv.genome = G; cultureWays(cv, BASES[bi]);
                std::map<std::string, int> kinds; int total = 0;
                for (int cell = 0; cell < 2; cell++) {
                    RuinSpec sp; settlementOf(g, gLat0 + cell * 7, gLon0 + bi * 5, cv, cell ? SC_VILLAGE : SC_TOWN, sp);
                    std::vector<Form> el; ruinElements(sp, cv, 0, el);
                    for (const Form& e : el) { total++; kinds[fmt("%s%d/%d", FORM_KIND_NAMES[e.kind], e.sub, e.part)]++; }
                    if (k == 0 && cell == 0) { std::map<std::string, int> bk; for (const Building& b : sp.buildings) bk[BUILDING_KIND_NAMES[b.kind]]++; std::string ks; for (auto& q : bk) ks += fmt("%d %s, ", q.second, q.first.c_str()); printf("  town: %s\n", ks.c_str()); }
                }
                std::string ks; for (auto& q : kinds) ks += fmt("%s %d; ", q.first.c_str(), q.second);
                printf("  %s: %d pieces: %s\n", genomeValue(G, v).c_str(), total, ks.substr(0, 400).c_str());
            }
        }
        return 0;
    }
    if (what == "floats") {   // R-412: the A-03 check's pieces on nothing and doorways shut, tallied by what they are (the unit's message is cut short)
        Star st; StarSystem sys; if (!starInSector(AGE_WORLDS[AGE_MEDIEVAL].sx, 0, AGE_WORLDS[AGE_MEDIEVAL].sz, st, true)) return 1;
        sys.generate(st); BodyGen g = BodyGen::make(sys.bodies[AGE_WORLDS[AGE_MEDIEVAL].body]);
        int gLat0, gLon0; ruinCellOf(g, 0.3, 1.1, gLat0, gLon0);
        std::map<std::string, int> floats, shut;
        std::vector<FormDisc> discs;
        for (int a = 0; a < AGE_COUNT; a++) {
            Culture c = cultureAs(g, 0, a, 300);
            for (int cls = 0; cls < 4; cls++) {
                if (a == AGE_FAR_FUTURE && cls != SC_MONUMENT) continue;
                for (int k = 0; k < 6; k++) {
                    RuinSpec sp; settlementOf(g, gLat0 + k * 3, gLon0 + cls * 5, c, cls, sp);
                    std::vector<Form> el; ruinElements(sp, c, 0, el);
                    for (size_t i = 0; i < el.size(); i++) if (!formRests(el, i)) {
                        if (floats.empty() && getenv("FLOATS_DEBUG")) {
                            const Form& f = el[i];
                            printf("  first: %s at %.2f %.2f y %.2f..%.2f building %d\n", FORM_KIND_NAMES[f.kind], f.x, f.z, f.y0, f.y1, f.building);
                            for (const Form& e : el) if (e.building == f.building && &e != &f) printf("    %s %d part %d at %.2f %.2f hx %.2f hz %.2f y %.2f..%.2f top %.2f dist %.2f\n", FORM_KIND_NAMES[e.kind], e.sub, e.part, e.x, e.z, e.hx, e.hz, e.y0, e.y1, formTop(e), std::hypot(e.x - f.x, e.z - f.z));
                        }
                        floats[fmt("%s %s %d part %d (%.2fx%.2f, %.1f..%.1f) of a %s", AGE_NAMES[a], FORM_KIND_NAMES[el[i].kind], el[i].sub, el[i].part, el[i].hx, el[i].hz, el[i].y0, el[i].y1, BUILDING_KIND_NAMES[sp.buildings[el[i].building].kind])]++;
                    }
                    for (size_t bi = 0; bi < sp.buildings.size(); bi++) {
                        const Building& b = sp.buildings[bi]; BuildingRoom R;
                        if (!buildingRoom(b, R) || !R.door) continue;
                        discs.clear(); for (const Form& e : el) if (e.building == (int)bi) formColliders(e, discs);
                        auto blockedAt = [&](double x, double z, double margin) { for (const FormDisc& q : discs) if (q.y0 < 1.0 && q.y1 > 0.6 && std::hypot(q.x - x, q.z - z) < q.r + margin) return true; return false; };
                        int run = 0, best = 0;
                        for (double t = -R.doorL; t <= R.doorL; t += 0.1) { run = blockedAt(R.doorX + R.doorAx * t, R.doorZ + R.doorAz * t, 0.35) ? 0 : run + 1; best = std::max(best, run); }
                        if (best * 0.1 >= 1.0) continue;
                        std::string by = "nothing at its middle";
                        for (const Form& e : el) { if (e.building != (int)bi) continue; std::vector<FormDisc> d1; formColliders(e, d1); for (const FormDisc& q : d1) if (q.y0 < 1.0 && q.y1 > 0.6 && std::hypot(q.x - R.doorX, q.z - R.doorZ) < q.r + 0.35) { by = fmt("%s %d part %d %.2fx%.2f y %.1f..%.1f", FORM_KIND_NAMES[e.kind], e.sub, e.part, e.hx, e.hz, e.y0, e.y1); break; } if (by[0] != 'n') break; }
                        shut[fmt("%s %s (form %d, door %d) by %s", AGE_NAMES[a], BUILDING_KIND_NAMES[b.kind], b.form, b.door, by.c_str())]++;
                    }
                }
            }
        }
        printf("pieces on nothing: %zu kinds\n", floats.size()); for (auto& f : floats) printf("  %4d  %s\n", f.second, f.first.c_str());
        printf("doorways shut: %zu kinds\n", shut.size()); for (auto& f : shut) printf("  %4d  %s\n", f.second, f.first.c_str());
        return 0;
    }
    if (what == "reach") {
        Star st; StarSystem sys; if (!starInSector(AGE_WORLDS[AGE_MEDIEVAL].sx, 0, AGE_WORLDS[AGE_MEDIEVAL].sz, st, true)) return 1;
        sys.generate(st); BodyGen g = BodyGen::make(sys.bodies[AGE_WORLDS[AGE_MEDIEVAL].body]);
        int gLat0, gLon0; ruinCellOf(g, 0.3, 1.1, gLat0, gLon0);
        std::string report; const double t0 = nowSec();
        const int missing = genomeReach(g, sys, gLat0 + 30, gLon0, report, true);
        printf("genome reach: %d variables, %.1f s, %d ages not reached\n", GV_COUNT, nowSec() - t0, missing);
        size_t p = 0; while (p < report.size()) { size_t q = report.find("; ", p); if (q == std::string::npos) q = report.size(); printf("  %s\n", report.substr(p, q - p).c_str()); p = q + 2; }
        return missing ? 1 : 0;
    }
    // the sheets
    const bool pairMode = what == "pair";
    const int scale = pairMode ? g_testScale : (argc > 3 ? std::max(1, atoi(argv[3])) : 1);   // the pair's at `scale=N`
    setFramebufferScale(scale);
    mkdir("shots/tests", 0755);
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    const std::string worldName = (what == "sheet" || what == "street") && argc > 6 ? argv[6] : (what == "dress" && argc > 5 ? argv[5] : "desert");
    if (!setupSceneForType(worldName == "felisian" ? PT_FELISIAN : PT_DESERT, 15, 40 * DEG, 1.3, 0.0, sv, sys, nb, t, worldName == "felisian" ? SCENE_BIOME - BIO_GRASSLAND : SCENE_ANY)) { printf("no body\n"); return 1; }   // B-415: a felisian world's grassland (its first site was the sea)
    sv.flocks.clear(); sv.critters.clear(); sv.relocateCapsule(-4000, -4000);
    const BodyGen& g = sv.site.gen;
    int gLat, gLon; sv.ruinCellAt(0, 0, gLat, gLon);
    Framebuffer fb; std::vector<uint32_t> rgbBuf((size_t)FBW * FBH);
    const uint32_t ink = rgb(200, 205, 190), head = rgb(235, 225, 170);
    if (what == "dress") {   // R-412: `genome dress [scale] [age|all] [desert|felisian] [high]`: each of the age's own variables a row, its values across, on one plain genome
        const std::string which = argc > 4 ? argv[4] : "all";
        const bool high = argc > 6 && std::string(argv[6]) == "high";
        const double years = argc > 7 ? atof(argv[7]) : 5;
        const uint64_t seed = peoplesNearHome(1)[0].seed;
        for (int a = AGE_PREHISTORIC; a <= AGE_FUTURISTIC; a++) {
            if (which != "all" && which != AGE_NAMES[a]) continue;
            Genome B = genomeOf(seed);   // the plain one: every R-412 variable at its first value, an upright, rectangular, pitched people
            for (int v = GV_PITCH; v < GV_COUNT; v++) B.set(v, 0);
            B.v[GV_PITCH] = 0.5; B.v[GV_STOREYS] = 0.5; B.v[GV_SETBACK] = 0.75;
            B.set(GV_PROFILE, WP_UPRIGHT); B.set(GV_PLAN, PF_RECT); B.set(GV_ROOF, RF_PITCHED); B.set(GV_MASSING, MS_COMPACT); B.set(GV_GROUPING, GR_DETACHED);
            B.set(GV_EXPRESSION, EX_MONOLITH); B.set(GV_TOPS, TP_NOTHING); B.set(GV_ORNAMENT, ON_BARE); B.v[GV_SAMENESS] = 0.1; B.v[GV_LASTING] = 0.6;
            std::vector<int> vars;
            for (int v = GV_PITCH; v < GV_COUNT; v++) if (GENOME_VARS[v].ages >> a & 1) vars.push_back(v);
            int cols = 1; for (int v : vars) cols = std::max(cols, GENOME_VARS[v].n > 0 ? GENOME_VARS[v].n : 3);
            const int textH = FONT_LINE + 4, tileH = FBH + textH * scale, W = cols * FBW, H = (int)vars.size() * tileH + 16 * scale;
            std::vector<uint32_t> sheet((size_t)W * H, rgb(16, 16, 20));
            RGBCanvas cv{sheet.data(), W, H, scale};
            drawText(cv, 4, 4, fmt("R-412: THE %s AGE'S OWN VARIABLES, EACH VALUE ON ONE PLAIN GENOME, %.0f YEARS SINCE THE END%s", AGE_NAMES[a], years, high ? ", FROM OVER THE STREET" : "").c_str(), head);
            for (size_t r = 0; r < vars.size(); r++) {
                const int v = vars[r], n = GENOME_VARS[v].n > 0 ? GENOME_VARS[v].n : 3;
                for (int k = 0; k < n; k++) {
                    Genome G = B;
                    if (GENOME_VARS[v].n > 0) G.set(v, k); else G.v[v] = 0.05 + 0.45 * k;
                    genomeDressTile(sv, sr, nb, t, g, a, G, seed, high, years, fb, rgbBuf);
                    const int dx0 = k * FBW, dy0 = 16 * scale + (int)r * tileH;
                    for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(dy0 + y) * W + dx0 + x] = rgbBuf[(size_t)y * FBW + x];
                    drawText(cv, dx0 / scale + 3, (dy0 + FBH) / scale + 2, fmt("%s %s", GENOME_VARS[v].name, genomeValue(G, v).c_str()).c_str(), ink);
                }
                printf("  %s %s: %d values\n", AGE_NAMES[a], GENOME_VARS[v].name, n); fflush(stdout);
            }
            const std::string fn = fmt("shots/tests/genome_dress_%s%s%s_%dx.png", AGE_NAMES[a], high ? "_high" : "", years > 50 ? "_old" : "", scale);
            writePNG(fn.c_str(), sheet.data(), W, H);
            printf("  -> %s\n", fn.c_str());
        }
        return 0;
    }
    if (what == "sheet") {
        const double years = argc > 4 ? atof(argv[4]) : 30;
        const std::string cls = argc > 5 ? argv[5] : "town";
        const int sclass = cls == "hamlet" ? SC_HAMLET : (cls == "village" ? SC_VILLAGE : SC_TOWN);
        const std::vector<NearPeople> P = peoplesNearHome(12);
        printf("genome sheet: the %zu peoples nearest home, a %s each at %.0f years since the end on %s (%s), %dx\n", P.size(), SETTLEMENT_CLASS_NAMES[sclass], years, sys.bodies[sv.site.body].name.c_str(), KINDS[sys.bodies[sv.site.body].type].name, scale);
        const int cols = 4, rows = (int)(P.size() + cols - 1) / cols, textH = 15 * FONT_LINE + 4, tileH = FBH + textH * scale;
        for (int a = 0; a < AGE_COUNT; a++) {
            const int W = cols * FBW, H = rows * tileH + 16 * scale;
            std::vector<uint32_t> sheet((size_t)W * H, rgb(16, 16, 20));
            RGBCanvas cv{sheet.data(), W, H, scale};
            drawText(cv, 4, 4, fmt("THE ARCHITECTURE GENOME: %s, %.0f YEARS SINCE THE END, THE TWELVE PEOPLES NEAREST HOME", AGE_NAMES[a], years).c_str(), head);
            for (size_t i = 0; i < P.size(); i++) {
                const int cx = (int)(i % cols), cy = (int)(i / cols);
                Culture cu = cultureWithGenome(g, a, years, P[i].seed);
                std::string line; genomeTile(sv, sr, nb, t, g, gLat, gLon, cu, sclass, fb, rgbBuf, &line);
                const int dx0 = cx * FBW, dy0 = 16 * scale + cy * tileH;
                for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(dy0 + y) * W + dx0 + x] = rgbBuf[(size_t)y * FBW + x];
                const int tx = dx0 / scale + 3, ty = (dy0 + FBH) / scale + 3;
                drawText(cv, tx, ty, fmt("%zu. %s, %.0f LY", i + 1, P[i].world.c_str(), P[i].d).c_str(), head);
                std::vector<std::string> L = genomeLines(genomeOf(P[i].seed), a, P[i].seed);
                for (size_t k = 0; k < L.size(); k++) drawText(cv, tx, ty + (int)(k + 1) * FONT_LINE, L[k].c_str(), ink);
                printf("  %-11s %2zu %-28s %s\n", AGE_NAMES[a], i + 1, P[i].world.c_str(), line.c_str());
            }
            std::string fn = fmt("shots/tests/genome_%s_%dx.png", a == AGE_FAR_FUTURE ? "far_future" : AGE_NAMES[a], scale);
            writePNG(fn.c_str(), sheet.data(), W, H);
            printf("  -> %s\n", fn.c_str());
        }
        return 0;
    }
    if (what == "ages" || pairMode) {   // the same peoples at every age (or the nearest pair of `genome distinct`, side by side)
        const double years = !pairMode && argc > 5 ? atof(argv[5]) : 30;
        std::vector<NearPeople> P;
        if (pairMode && argc >= 13) {   // `genome pair <sx> <sy> <sz> <body> <people>` twice
            for (int k = 0; k < 2; k++) {
                int64_t sx = atoll(argv[3 + 5 * k]), sy = atoll(argv[4 + 5 * k]), sz = atoll(argv[5 + 5 * k]); int bi = atoi(argv[6 + 5 * k]), which = atoi(argv[7 + 5 * k]);
                Star s; if (!starInSector(sx, sy, sz, s, true)) { printf("  no star at %lld %lld %lld\n", (long long)sx, (long long)sy, (long long)sz); continue; }
                StarSystem s2; s2.generate(s); if (bi >= (int)s2.bodies.size()) continue;
                BodyGen gg = BodyGen::make(s2.bodies[bi]); if (which >= peoplesOf(gg)) continue;
                P.push_back({sx, sy, sz, 0, bi, which, peopleGen(gg, which).seed, s2.bodies[bi].name, 0, 0});
            }
        } else P = peoplesNearHome(argc > 4 && !pairMode ? std::max(1, atoi(argv[4])) : 6);
        const int rows = (int)P.size(), textH = 2 * FONT_LINE + 4, rowH = FBH + textH * scale;
        const int W = AGE_COUNT * FBW, H = rows * rowH + 16 * scale;
        std::vector<uint32_t> sheet((size_t)W * H, rgb(16, 16, 20));
        RGBCanvas cv{sheet.data(), W, H, scale};
        drawText(cv, 4, 4, fmt("%s, %.0f YEARS SINCE THE END: A PEOPLE A ROW, ITS TOWN IN EVERY AGE", pairMode ? "THE NEAREST PAIR" : "ONE PEOPLE IN EVERY AGE", years).c_str(), head);
        for (int r = 0; r < rows; r++) {
            for (int a = 0; a < AGE_COUNT; a++) {
                Culture cu = cultureWithGenome(g, a, years, P[r].seed);
                std::string line; genomeTile(sv, sr, nb, t, g, gLat, gLon, cu, SC_TOWN, fb, rgbBuf, &line);
                const int dx0 = a * FBW, dy0 = 16 * scale + r * rowH;
                for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(dy0 + y) * W + dx0 + x] = rgbBuf[(size_t)y * FBW + x];
                if (r == 0) drawTextShadow(cv, dx0 / scale + 3, (dy0) / scale + 2, AGE_NAMES[a], head, rgb(0, 0, 0));
                printf("  %-24s %-11s %s\n", P[r].world.c_str(), AGE_NAMES[a], line.c_str());
            }
            const Genome G = genomeOf(P[r].seed);
            const int ty = (16 * scale + r * rowH + FBH) / scale + 2;
            drawText(cv, 3, ty, fmt("%d. %s: %s %s, ROOF %s, PLAN %s, %s, FOCUS %s, %s %s, CENTRE %s", r + 1, P[r].world.c_str(), genomeValue(G, GV_BASE).c_str(), genomeValue(G, GV_ACCENT).c_str(), genomeValue(G, GV_ROOF).c_str(), genomeValue(G, GV_PLAN).c_str(),
                                         genomeValue(G, GV_MASSING).c_str(), genomeValue(G, GV_FOCUS).c_str(), genomeValue(G, GV_PROFILE).c_str(), genomeValue(G, GV_OPENING).c_str(), genomeValue(G, GV_CENTRE).c_str()).c_str(), ink);
        }
        std::string fn = fmt("shots/tests/genome_%s_%dx.png", pairMode ? "pair" : "ages", scale);
        writePNG(fn.c_str(), sheet.data(), W, H);
        printf("  -> %s\n", fn.c_str());
        return 0;
    }
    if (what == "street") {   // B-415: the same peoples at every age but the far future's, each town seen from inside it
        const int n = argc > 4 ? std::max(1, atoi(argv[4])) : 6;
        const double years = argc > 5 ? atof(argv[5]) : 30;
        const bool drone = argc > 7 && std::string(argv[7]) == "drone";
        const std::vector<NearPeople> P = peoplesNearHome(n);
        const int ages = AGE_FAR_FUTURE, rows = (int)P.size(), textH = 2 * FONT_LINE + 6, rowH = FBH + textH * scale;
        const int W = ages * FBW, H = rows * rowH + 16 * scale;
        std::vector<uint32_t> sheet((size_t)W * H, rgb(16, 16, 20));
        RGBCanvas cv{sheet.data(), W, H, scale};
        drawText(cv, 4, 4, fmt("EVERY AGE FROM %s, %.0f YEARS SINCE THE END: A PEOPLE A ROW", drone ? "THE DRONE" : "THE STREET", years).c_str(), head);
        for (int r = 0; r < rows; r++) {
            for (int a = 0; a < ages; a++) {
                Culture cu = cultureWithGenome(g, a, years, P[r].seed);
                std::string line; genomeStreetTile(sv, sr, nb, t, g, gLat, gLon, cu, drone, fb, rgbBuf, &line);
                const int dx0 = a * FBW, dy0 = 16 * scale + r * rowH;
                for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(dy0 + y) * W + dx0 + x] = rgbBuf[(size_t)y * FBW + x];
                if (r == 0) drawTextShadow(cv, dx0 / scale + 3, dy0 / scale + 2, AGE_NAMES[a], head, rgb(0, 0, 0));
                printf("  %-24s %-11s %s\n", P[r].world.c_str(), AGE_NAMES[a], line.c_str());
            }
            const Genome G = genomeOf(P[r].seed);
            drawText(cv, 3, (16 * scale + r * rowH + FBH) / scale + 2, fmt("%d. %s: %s %s %s, ROOF %s, PLAN %s, %s %s, GLAZING %.2f %s", r + 1, P[r].world.c_str(), genomeValue(G, GV_BASE).c_str(), genomeValue(G, GV_PAINTED).c_str(), genomeValue(G, GV_ACCENT).c_str(),
                                                                                  genomeValue(G, GV_ROOF).c_str(), genomeValue(G, GV_PLAN).c_str(), genomeValue(G, GV_PROFILE).c_str(), genomeValue(G, GV_OPENING).c_str(), G.at(GV_SHARE), genomeValue(G, GV_RHYTHM).c_str()).c_str(), ink);
            // R-412: its own within its age's ways
            drawText(cv, 3, (16 * scale + r * rowH + FBH) / scale + 3 + FONT_LINE, fmt("   %s ROOFING, PITCH %.2f, EAVES %s, ROOFLINE %s, GABLES %s; %s, TRIM %s, %s, ENTRANCE %s, CORNERS %s; STOREYS %.2f, %s, SETBACK %.2f, BOUNDARY %s, FURNITURE %s, POLES %s",
                                                                                                  genomeValue(G, GV_ROOFING).c_str(), G.at(GV_PITCH), genomeValue(G, GV_EAVES).c_str(), genomeValue(G, GV_ROOFLINE).c_str(), genomeValue(G, GV_GABLES).c_str(), genomeValue(G, GV_POLYCHROMY).c_str(),
                                                                                                  genomeValue(G, GV_TRIM).c_str(), genomeValue(G, GV_PROJECTIONS).c_str(), genomeValue(G, GV_ENTRANCE).c_str(), genomeValue(G, GV_CORNERS).c_str(), G.at(GV_STOREYS), genomeValue(G, GV_FRONTAGE).c_str(),
                                                                                                  G.at(GV_SETBACK), genomeValue(G, GV_BOUNDARY).c_str(), genomeValue(G, GV_FURNITURE).c_str(), genomeValue(G, GV_POLES).c_str()).c_str(), ink);
        }
        std::string fn = fmt("shots/tests/genome_street%s_%dx.png", drone ? "_drone" : "", scale);
        writePNG(fn.c_str(), sheet.data(), W, H);
        printf("  -> %s\n", fn.c_str());
        return 0;
    }
    printf("genome sheet [scale] [years] [town|village|hamlet] [desert|felisian] | genome ages [scale] [peoples] [years] | genome street [scale] [peoples] [years] [desert|felisian] [drone] | genome distinct [n] | genome print <sx> <sz> <body> [people] | genome pair <sx sy sz body people> x 2 | genome reach\n");
    return 1;
}
