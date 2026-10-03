#pragma once
// C-12 (2026-10-03): graves and names. "A ruin holds a few marked places, each with a generated name in the world's language.
// The guide records them like landmarks. Nothing else happens" (`docs/ideas/IDEAS-ruins-shards-signals.md`, section 3). A
// settlement's burial ground lies outside its edge (beyond the wall and the buildings' reach, away from the shore where it has
// one), rows of stones a couple of metres apart facing the settlement: a slab a metre tall with a line of glyphs, fallen by the
// culture's decay. Each stone carries a name in the people's style (`personName`) and the years it covers in the people's own
// reckoning (`shards.h`: the calendar, the span from the founding to the last recording). One function of the settlement's seed
// and the lore, the same for the view, the game and the harness (`vesperis_test graves`).
#include "ruins.h"
#include "shards.h"

struct Grave {
    double x = 0, z = 0, heading = 0;   // metres east and north of the settlement's centre; the stone's face looks this way
    double height = 0.9;                // the slab's height when it stood
    bool fallen = false;                // lying flat, its face to the sky
    std::string name;                   // in the people's language (a name is kept as it is: the decoder translates none)
    int born = 0, died = 1;             // their years (born 0: before the founding, not cut)
    uint64_t id = 0;                    // stable: the settlement's seed and the stone's index
};
// a settlement's graves: a hamlet one to three, a village three to six, a town six to twelve, a lone monument one in three times
void gravesOf(const Lore& L, const RuinSpec& r, const Culture& c, std::vector<Grave>& out);
// the stones as pieces to draw at the whole level of detail (lod 0; the far levels draw none), appended: a standing slab 0.64 m
// wide and 0.16 m thick with its glyphs, or the same lying flat
void graveElements(const std::vector<Grave>& graves, int lod, std::vector<RuinElem>& out);
std::string graveLine(const Lore& L, const Grave& g);   // "Thaelu, years 140 to 212 of Kethra" (born before the founding: "Thaelu, to year 212 of Kethra")
