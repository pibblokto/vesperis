// Z-01: the geneses (life/lineages.h)
#include "life/lineages.h"
#include "core/rng.h"
#include <algorithm>

const char* const CHEMISTRY_NAMES[CH_COUNT] = {"NONE", "WATER", "ACID", "AMMONIA", "HYDROCARBON"};
const char* const GENESIS_ORIGIN_NAMES[GO_COUNT] = {"NONE", "OWN", "CELL", "WANDERER"};

namespace {
// the geneses' salts, each decision its own (generation 17). Pinned: KIN and TRAITS, so that home's water life is its cell's oldest
// genesis, ours in hand, of four letters (`homeGenesis`; the unit's `life` group holds HOME_GENESIS_ID)
constexpr uint64_t SALT_OWN = 0x6E7E515000000001ULL;      // a system's own genesis, by its star's seed and the chemistry
constexpr uint64_t SALT_KIN = 0x6E7E515000000702ULL;      // pinned: whether a system's water life is its cell's, and which
constexpr uint64_t SALT_CELL = 0x6E7E515000000003ULL;     // a cell's geneses
constexpr uint64_t SALT_COUNT = 0x6E7E515000000004ULL;    // how many a cell holds
constexpr uint64_t SALT_TRAITS = 0x6E7E515000000605ULL;   // pinned: a genesis's handedness, letters, style and age
constexpr uint64_t SALT_WANDERER = 0x6E7E515000000006ULL; // a caught rogue's, by its seed

// what a genesis is, from its id: its handedness (a bit), its letters (four half the time), its style, its age (a cell's first the oldest)
void traitsOf(Genesis& g) {
    Rng r(g.id ^ SALT_TRAITS);
    g.mirrored = r.uni() < 0.5;
    const double u = r.uni();
    g.letters = u < 0.5 ? 4 : (u < 0.75 ? 6 : 8);
    g.style = (uint32_t)(r.next() >> 32);
    const double a = r.uni();
    g.ageGyr = g.origin != GO_CELL ? 0.6 + 3.6 * a : (g.cellIndex == 0 ? 6.0 + 2.5 * a : 4.0 + 2.0 * a);   // a cell's first the oldest
}
}   // namespace

int cellGenesisCount(int64_t cx, int64_t cy, int64_t cz) { return unitFromHash(hash3i(cx, cy, cz, SALT_COUNT)) < CELL_SECOND ? 2 : 1; }

Genesis cellGenesis(int64_t cx, int64_t cy, int64_t cz, int k) {
    Genesis g;
    g.id = hashCombine(hash3i(cx, cy, cz, SALT_CELL), (uint64_t)k);
    g.chemistry = CH_WATER; g.origin = GO_CELL;
    g.cx = cx; g.cy = cy; g.cz = cz; g.cellIndex = k;
    traitsOf(g);
    return g;
}

Genesis systemGenesis(const Star& star, int chemistry) {
    Rng r(star.seed ^ SALT_KIN);
    const bool kin = r.uni() < KIN_SHARE;
    const double pickK = r.uni();
    if (chemistry == CH_WATER && kin) {
        const int64_t cx = kinCellOf(star.sx), cy = kinCellOf(star.sy), cz = kinCellOf(star.sz);
        const int n = cellGenesisCount(cx, cy, cz);
        return cellGenesis(cx, cy, cz, std::min((int)(pickK * n), n - 1));
    }
    Genesis g;
    g.id = hashCombine(star.seed ^ SALT_OWN, (uint64_t)chemistry);
    g.chemistry = chemistry; g.origin = GO_OWN;
    traitsOf(g);
    return g;
}

Genesis wandererGenesis(uint64_t bodySeed, int chemistry) {
    Genesis g;
    g.id = hashCombine(bodySeed ^ SALT_WANDERER, (uint64_t)chemistry);
    g.chemistry = chemistry; g.origin = GO_WANDERER;
    traitsOf(g);
    return g;
}

Genesis homeGenesis() {
    Star home;
    starInSector(HOME_SX, HOME_SY, HOME_SZ, home, false);
    return systemGenesis(home, CH_WATER);
}
