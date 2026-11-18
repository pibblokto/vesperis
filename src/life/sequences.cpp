// Z-06: the genomes and the tree of kin (life/sequences.h)
#include "life/sequences.h"
#include "galaxy/drifters.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <tuple>

namespace {
// the lineage's salts, each decision its own (nothing of generation 17 moves: the biota is read, never drawn again)
constexpr uint64_t SALT_CLADES = 0x5E9E0C0000000001ULL;    // when a genesis's deep branches parted
constexpr uint64_t SALT_PART = 0x5E9E0C0000000002ULL;      // when a kin cell's lines parted: its quarters', its sixteenths', a star's
constexpr uint64_t SALT_EXCHANGE = 0x5E9E0C0000000003ULL;  // a system's worlds' last exchange of rock
constexpr uint64_t SALT_ORDER = 0x5E9E0C0000000004ULL;     // a world's comb: its species' order
constexpr uint64_t SALT_HEIGHT = 0x5E9E0C0000000005ULL;    // and the heights between neighbours
constexpr uint64_t SALT_ROOT = 0x5E9E0C0000000006ULL;      // the genesis's ancestor's letters
constexpr uint64_t SALT_RATE = 0x5E9E0C0000000007ULL;      // each place's rate
constexpr uint64_t SALT_STEM = 0x5E9E0C0000000008ULL;      // the cell's line's changes, by its segments
constexpr uint64_t SALT_SYSTEM = 0x5E9E0C0000000009ULL;    // a system's line's
constexpr uint64_t SALT_WORLD = 0x5E9E0C000000000AULL;     // a world's stem's
constexpr uint64_t SALT_NODE = 0x5E9E0C000000000BULL;      // a comb's branches'
constexpr uint64_t SALT_GENOME = 0x5E9E0C000000000CULL;    // a genome's length
constexpr double MU = 0.12;           // a place's changes in a billion years at its rate (a slow marker's: 16S's about 1% in 50 million)
constexpr double KEPT = 1.0 / 3;      // the places that never change (a gene's conserved sites)
// a kin cell's lines part by where their stars are (a cluster's stars parted as it spread: Adams and Spergel 2005): the cell's sixteen-sector
// boxes at 0.86-0.90 of its genesis's age, the four-sector boxes within one at 0.78-0.84, the stars within one at 0.70-0.76
constexpr int BOX_WIDE = 16, BOX_NEAR = 4;

const char* const ALPHABETS[3] = {"ACGT", "ACGTZP", "ACGTZPSB"};

int64_t boxOf(int64_t s, int n) { const int64_t q = s / n; return (s % n != 0 && s < 0) ? q - 1 : q; }
double cladeOf(const Genesis& g) { return g.ageGyr * (0.93 + 0.05 * unitFromHash(mix64(g.id ^ SALT_CLADES))); }
// a line's partings, from the cell's down to its star's (a genesis of a system's own: none, its deep branch the system's line)
void partsOf(Lineage& L) {
    L.cladeGyr = cladeOf(L.genesis);
    if (L.genesis.origin != GO_CELL) { L.cellGyr = L.wideGyr = L.nearGyr = L.cladeGyr; }
    else {
        const Genesis& g = L.genesis;
        L.cellGyr = g.ageGyr * (0.86 + 0.04 * unitFromHash(mix64(g.id ^ SALT_PART)));
        L.wideGyr = g.ageGyr * (0.78 + 0.06 * unitFromHash(hash3i(boxOf(L.lx, BOX_WIDE), boxOf(L.ly, BOX_WIDE), boxOf(L.lz, BOX_WIDE), g.id ^ SALT_PART ^ 16)));
        L.nearGyr = g.ageGyr * (0.70 + 0.06 * unitFromHash(hash3i(boxOf(L.lx, BOX_NEAR), boxOf(L.ly, BOX_NEAR), boxOf(L.lz, BOX_NEAR), g.id ^ SALT_PART ^ 4)));
    }
    L.exchangeGyr = L.nearGyr * (0.5 + 0.3 * unitFromHash(hash3i(L.lx, L.ly, L.lz, L.genesis.id ^ SALT_EXCHANGE)));
}

// home's world: the home system's first felisian planet (the unit's, Z-01)
struct HomeWorld { StarSystem sys; int body = -1; };
const HomeWorld& homeWorld() {
    static const HomeWorld H = [] {
        HomeWorld h;
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
        h.sys.generate(home);
        for (int i = 0; i < (int)h.sys.bodies.size(); i++) if (h.sys.bodies[i].parent < 0 && h.sys.bodies[i].type == PT_FELISIAN && h.body < 0) h.body = i;
        return h;
    }();
    return H;
}

// a world's comb of a deep branch: its own species of that branch alive or asleep (its fossils stone, its carried spores another world's),
// home's world's with the ship's three, in an order by their seeds; the height between two neighbours is a function of the pair
struct Comb { std::vector<int> ids; std::vector<double> heights; };   // an id: the species' index, -1-k the ship's k
void combOf(const Biota& W, int clade, bool home, double exchange, Comb& out) {
    std::vector<std::pair<double, int>> order;
    std::vector<uint64_t> seeds;
    for (const Microbe& s : W.species)
        if ((s.state == SS_LIVING || s.state == SS_DORMANT) && s.fromBody < 0 && s.clade == clade) order.push_back({unitFromHash(mix64(s.seed ^ SALT_ORDER)), s.index});
    if (home && clade == 0) for (int k = 0; k < SHIP_SPECIES; k++) order.push_back({unitFromHash(mix64(shipSpecies(k).seed ^ SALT_ORDER)), -1 - k});
    std::sort(order.begin(), order.end());
    out.ids.clear(); out.heights.clear();
    for (const auto& o : order) { out.ids.push_back(o.second); seeds.push_back(o.second >= 0 ? W.species[o.second].seed : shipSpecies(-1 - o.second).seed); }
    for (size_t k = 0; k < seeds.size(); k++)
        out.heights.push_back(k == 0 ? 0.0 : exchange * (0.06 + 0.5 * unitFromHash(mix64(seeds[k - 1] ^ mix64(seeds[k] ^ SALT_HEIGHT)))));
}

// a lineage's line and world filled from its world's biota (`wb` the world it lives on as the comb reads it)
void place(Lineage& L, const Biota& W, bool home, int id, const Star& line, int64_t wx, int64_t wy, int64_t wz, int world) {
    L.genesis = W.genesis;
    L.lx = line.sx; L.ly = line.sy; L.lz = line.sz;
    L.wx = wx; L.wy = wy; L.wz = wz; L.world = world;
    partsOf(L);
    Comb C; combOf(W, L.clade, home, L.exchangeGyr, C);
    L.heights = C.heights; L.leaves = (int)C.ids.size();
    for (int k = 0; k < L.leaves; k++) if (C.ids[k] == id) L.leaf = k;
    L.valid = L.leaf >= 0 && L.genesis.id != 0;
}

// the star a drifter left (its line's), the last one asked kept
Star lineStarOf(const StarSystem& sys) {
    if (!sys.sunless()) return sys.star;
    static std::mutex mu; static Star lastD, lastP; static bool have = false, found = false;
    {
        std::lock_guard<std::mutex> lk(mu);
        if (have && lastD.sx == sys.star.sx && lastD.sy == sys.star.sy && lastD.sz == sys.star.sz && lastD.seed == sys.star.seed) return found ? lastP : sys.star;
    }
    const DrifterPast p = drifterPast(sys.star);
    std::lock_guard<std::mutex> lk(mu);
    lastD = sys.star; lastP = p.star; have = true; found = p.found;
    return p.found ? p.star : sys.star;
}

// the comb's nodes above a leaf, from its root down: node k (between leaves k-1 and k) is an ancestor of leaf i when every height between them
// is lower (a Cartesian tree's ancestors); the side it is reached from
struct Node { int k; double h; int side; };
void pathOf(const Lineage& L, std::vector<Node>& out) {
    out.clear();
    const int i = L.leaf;
    for (int k = 1; k < L.leaves; k++) {
        const double hk = L.heights[k];
        bool above = true;
        if (k <= i) { for (int j = k + 1; j <= i && above; j++) above = L.heights[j] < hk || (L.heights[j] == hk && j > k); }
        else { for (int j = i + 1; j < k && above; j++) above = L.heights[j] < hk || (L.heights[j] == hk && j > k); }
        if (above) out.push_back({k, hk, k <= i ? 1 : 0});
    }
    std::sort(out.begin(), out.end(), [](const Node& a, const Node& b) { return a.h > b.h; });
}
}   // namespace

const char* alphabetOf(int letters) { return ALPHABETS[letters >= 8 ? 2 : (letters >= 6 ? 1 : 0)]; }

Lineage lineageOf(const StarSystem& sys, int body, int species) {
    Lineage L;
    if (body < 0 || body >= (int)sys.bodies.size()) return L;
    auto life = lifeOf(sys);
    const Biota& B = life->bodies[body];
    if (species < 0 || species >= (int)B.species.size() || !B.genesis.id) return L;
    const Microbe& s = B.species[species];
    if (s.state == SS_FOSSIL || s.state == SS_PROTO || s.clade < 0) return L;
    L.clade = s.clade; L.plan = s.plan;
    int wb = body, id = species;
    if (s.fromBody >= 0 && s.fromBody < (int)life->bodies.size()) { wb = s.fromBody; id = s.fromIndex; L.carriedGyr = s.ageYears * 1e-9; }   // its source's
    const HomeWorld& H = homeWorld();
    const bool home = sys.star.sx == HOME_SX && sys.star.sy == HOME_SY && sys.star.sz == HOME_SZ && wb == H.body;
    place(L, life->bodies[wb], home, id, sys.bodies[wb].wanderer ? sys.star : lineStarOf(sys), sys.star.sx, sys.star.sy, sys.star.sz, wb);
    return L;
}

Lineage shipLineage(int k) {
    Lineage L;
    const HomeWorld& H = homeWorld();
    if (H.body < 0 || k < 0 || k >= SHIP_SPECIES) return L;
    L.clade = shipSpecies(k).clade; L.plan = shipSpecies(k).plan; L.ship = true;
    place(L, lifeOf(H.sys)->bodies[H.body], true, -1 - k, H.sys.star, H.sys.star.sx, H.sys.star.sy, H.sys.star.sz, H.body);
    return L;
}

double commonAncestorGyr(const Lineage& a, const Lineage& b) {
    if (!a.valid || !b.valid || a.genesis.id != b.genesis.id) return -1;
    if (a.clade != b.clade) return a.cladeGyr;
    auto apart = [&](int n) { return boxOf(a.lx, n) != boxOf(b.lx, n) || boxOf(a.ly, n) != boxOf(b.ly, n) || boxOf(a.lz, n) != boxOf(b.lz, n); };
    if (a.genesis.origin == GO_CELL && apart(BOX_WIDE)) return a.cellGyr;   // a kin cell's lines: by the boxes their stars share
    if (a.genesis.origin == GO_CELL && apart(BOX_NEAR)) return a.wideGyr;
    if (a.lx != b.lx || a.ly != b.ly || a.lz != b.lz) return a.nearGyr;
    if (a.wx != b.wx || a.wy != b.wy || a.wz != b.wz || a.world != b.world) return a.exchangeGyr;
    if (a.leaf != b.leaf) {
        double h = 0;
        for (int k = std::min(a.leaf, b.leaf) + 1; k <= std::max(a.leaf, b.leaf); k++) h = std::max(h, a.heights[k]);
        return h;
    }
    return std::max(a.carriedGyr, b.carriedGyr);   // one species: the older of the impacts that carried it apart
}

namespace {
// a letter with its lineage's comb path found once (a marker's whole run reads one)
int letterAt(const Lineage& L, const std::vector<Node>& path, int p) {
    const int n = std::max(4, L.genesis.letters);
    const uint64_t g = L.genesis.id, pk = (uint64_t)p * 0x9E3779B97F4A7C15ULL;
    int x = (int)(mix64(g ^ SALT_ROOT ^ pk) % (uint64_t)n);
    if (!L.valid) return x;
    const uint64_t hr = mix64(g ^ SALT_RATE ^ pk);
    if (unitFromHash(hr) < KEPT) return x;
    const double rate = MU * std::min(3.0, -std::log(std::max(1e-12, unitFromHash(mix64(hr)))));
    const uint64_t clade = hashCombine(g, 0xC1ADE00ULL + (uint64_t)L.clade);
    auto turn = [&](uint64_t h) { x = (x + 1 + (int)((h >> 7) % (uint64_t)(n - 1))) % n; };
    auto change = [&](uint64_t branch, double len) {
        if (len <= 0) return;
        const uint64_t h = mix64(branch ^ (uint64_t)p * 0xC2B2AE3D27D4EB4FULL);
        if (unitFromHash(h) < 1 - std::exp(-rate * len)) turn(h);
    };
    // the deep branch's stem, a kin cell's sixteen-sector box's line and its four-sector box's (a system's own genesis: none), the star's line
    change(clade ^ SALT_STEM, L.cladeGyr - L.cellGyr);
    change(hash3i(boxOf(L.lx, BOX_WIDE), boxOf(L.ly, BOX_WIDE), boxOf(L.lz, BOX_WIDE), clade ^ SALT_STEM ^ 16), L.cellGyr - L.wideGyr);
    change(hash3i(boxOf(L.lx, BOX_NEAR), boxOf(L.ly, BOX_NEAR), boxOf(L.lz, BOX_NEAR), clade ^ SALT_STEM ^ 4), L.wideGyr - L.nearGyr);
    change(hash3i(L.lx, L.ly, L.lz, clade ^ SALT_SYSTEM), L.nearGyr - L.exchangeGyr);
    const uint64_t world = hashCombine(hash3i(L.wx, L.wy, L.wz, clade ^ SALT_WORLD), (uint64_t)(L.world + 1));
    change(world, L.exchangeGyr - (path.empty() ? 0 : path[0].h));
    for (size_t j = 0; j < path.size(); j++)
        change(hashCombine(world ^ SALT_NODE, (uint64_t)path[j].k * 2 + (uint64_t)path[j].side), path[j].h - (j + 1 < path.size() ? path[j + 1].h : 0.0));
    return x;
}
}   // namespace

int markerLetter(const Lineage& L, int p) { std::vector<Node> path; pathOf(L, path); return letterAt(L, path, p); }

bool treeBefore(const Lineage& a, const Lineage& b) {
    const bool ha = a.genesis.id == HOME_GENESIS_ID, hb = b.genesis.id == HOME_GENESIS_ID;
    if (ha != hb) return ha;
    if (a.genesis.id != b.genesis.id) return a.genesis.id < b.genesis.id;
    if (a.clade != b.clade) return a.clade < b.clade;
    auto box = [](const Lineage& l, int n) { return std::make_tuple(boxOf(l.lx, n), boxOf(l.ly, n), boxOf(l.lz, n)); };
    if (a.genesis.origin == GO_CELL) {
        if (box(a, BOX_WIDE) != box(b, BOX_WIDE)) return box(a, BOX_WIDE) < box(b, BOX_WIDE);
        if (box(a, BOX_NEAR) != box(b, BOX_NEAR)) return box(a, BOX_NEAR) < box(b, BOX_NEAR);
    }
    if (std::make_tuple(a.lx, a.ly, a.lz) != std::make_tuple(b.lx, b.ly, b.lz)) return std::make_tuple(a.lx, a.ly, a.lz) < std::make_tuple(b.lx, b.ly, b.lz);
    if (std::make_tuple(a.wx, a.wy, a.wz, a.world) != std::make_tuple(b.wx, b.wy, b.wz, b.world))
        return std::make_tuple(a.wx, a.wy, a.wz, a.world) < std::make_tuple(b.wx, b.wy, b.wz, b.world);
    if (a.leaf != b.leaf) return a.leaf < b.leaf;
    return a.carriedGyr < b.carriedGyr;
}

std::string markerOf(const Lineage& L, int from, int n) {
    const char* abc = alphabetOf(L.genesis.letters);
    std::vector<Node> path; pathOf(L, path);
    std::string s;
    for (int p = from; p < from + n; p++) s += abc[letterAt(L, path, ((p % MARKER_LETTERS) + MARKER_LETTERS) % MARKER_LETTERS)];
    return s;
}

double markerAlike(const Lineage& a, const Lineage& b) {
    if (!a.valid || !b.valid || a.genesis.letters != b.genesis.letters) return 0;
    std::vector<Node> pa, pb; pathOf(a, pa); pathOf(b, pb);
    int same = 0;
    for (int p = 0; p < MARKER_LETTERS; p++) same += letterAt(a, pa, p) == letterAt(b, pb, p);
    return same / (double)MARKER_LETTERS;
}

double genomeLetters(const Microbe& s) {
    const double u = unitFromHash(mix64(s.seed ^ SALT_GENOME));
    if (protocellPlan(s.plan)) return 0;
    if (smallCellPlan(s.plan)) return std::pow(10.0, 5.8 + 1.3 * u);
    if (largeCellPlan(s.plan)) return std::pow(10.0, 7.2 + 1.75 * u);
    return std::pow(10.0, 7.5 + 1.8 * u);
}

std::string watermarkLetters(const std::string& text, int letters, int from, int n) {
    const char* abc = alphabetOf(letters);
    const int L = (int)std::strlen(abc), digits = L >= 8 ? 2 : 3;
    auto code = [](char c) {
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (c >= 'A' && c <= 'Z') return 1 + (c - 'A');
        if (c >= '0' && c <= '9') return 27 + (c - '0');
        const char* p = std::strchr(".,'-:?!", c);
        return p && c ? 37 + (int)(p - ".,'-:?!") : 0;
    };
    std::string s;
    if (text.empty()) return s;
    const int len = (int)text.size() * digits;
    for (int q = from; q < from + n; q++) {
        const int at = ((q % len) + len) % len, c = code(text[at / digits]);
        int v = c;
        for (int d = digits - 1 - at % digits; d > 0; d--) v /= L;
        s += abc[v % L];
    }
    return s;
}
