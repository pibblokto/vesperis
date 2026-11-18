#pragma once
// Z-01 (2026-11-16): the geneses, the origins of life (R-425; docs/ideas/IDEAS-small-and-spacious.md section 1.4). A genesis is one
// beginning of life: its descendants share a chemistry, a handedness (mirrored or ours: a mirrored genesis is as likely as home's), an
// alphabet of their code (four letters, six or eight: hachimoji DNA, Hoshika et al. 2019) and a style of form (Z-02). Most systems'
// life is a genesis of its own; impacts carry rock between a system's worlds (Melosh 1988, Mileikowsky et al. 2000), so its living
// worlds of one chemistry share one; and stars born in one cluster trade rocks before they part (Adams and Spergel 2005), so one
// living system in five descends from its kin cell's oldest genesis (a cell of KIN_CELL sectors a side holds one or two). A drifter's
// life is the star's it left (`drifterPast`), a wanderer's a genesis no star near it shares. Everything is a function of the star or
// the body and salts of its own: no star, system or body moves (generation 17)
#include "galaxy/starfield.h"
#include <cstdint>

// the solvent a life is built on (R-425's four; the screens say what is seen, never that it is so: three are proposals)
enum Chemistry { CH_NONE = 0, CH_WATER, CH_ACID, CH_AMMONIA, CH_HYDROCARBON, CH_COUNT };
extern const char* const CHEMISTRY_NAMES[CH_COUNT];   // "NONE", "WATER", "ACID", "AMMONIA", "HYDROCARBON"

// where a genesis is known from: its system's own, its kin cell's (a cell's oldest), a wanderer's (a caught rogue's: no star near shares it)
enum GenesisOrigin { GO_NONE = 0, GO_OWN, GO_CELL, GO_WANDERER, GO_COUNT };
extern const char* const GENESIS_ORIGIN_NAMES[GO_COUNT];   // "NONE", "OWN", "CELL", "WANDERER"

constexpr int KIN_CELL = 64;           // the kin cells' side, in sectors (a light year each)
constexpr double KIN_SHARE = 0.2;      // the living systems whose water life descends from their cell's oldest genesis
constexpr double CELL_SECOND = 0.4;    // the cells that hold a second old genesis

struct Genesis {
    uint64_t id = 0;            // what names it: the hash its handedness, its letters, its style and its age come from (0 none)
    int chemistry = CH_NONE;
    int origin = GO_NONE;
    bool mirrored = false;      // its handedness the mirror of home's (a grazer of one cannot eat the other)
    int letters = 4;            // its code's alphabet: 4, 6 or 8 letters
    uint32_t style = 0;         // the style its forms share (a wall's mineral, a symmetry, a flagellum's kind: Z-02)
    double ageGyr = 0;          // how long ago it began, in billions of years
    int64_t cx = 0, cy = 0, cz = 0;   // a cell's genesis: its cell
    int cellIndex = -1;               // and which of the cell's (0 its oldest)
};

inline int64_t kinCellOf(int64_t s) { const int64_t q = s / KIN_CELL; return (s % KIN_CELL != 0 && s < 0) ? q - 1 : q; }
int cellGenesisCount(int64_t cx, int64_t cy, int64_t cz);    // 1 or 2
Genesis cellGenesis(int64_t cx, int64_t cy, int64_t cz, int k);
// a system's life of a chemistry: its water life its kin cell's genesis one time in five (KIN_SHARE, by the star's hash), else its own;
// every other chemistry its own
Genesis systemGenesis(const Star& star, int chemistry);
Genesis wandererGenesis(uint64_t bodySeed, int chemistry);   // a caught rogue's, by the body's seed
// home's: pinned (the salts chosen so that Heilya's water life is its cell's oldest genesis, ours in hand and of four letters); the
// ship's own species are of it (`shipSpecies`, life/biota.h) and root the tree
Genesis homeGenesis();
constexpr uint64_t HOME_GENESIS_ID = 0xd11f08219d5004d6ULL;   // pinned: what `homeGenesis().id` is (the unit's `life` group)
