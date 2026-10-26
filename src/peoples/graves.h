#pragma once
// C-12 (2026-10-03): graves and names. "A ruin holds a few marked places, each with a generated name in the world's language.
// The guide records them like landmarks. Nothing else happens" (`docs/archive/ideas/IDEAS-ruins-shards-signals.md`, section 3). A
// settlement's burial ground lies outside its edge (beyond the wall and the buildings' reach, away from the shore where it has
// one), rows of stones a couple of metres apart facing the settlement: a slab a metre tall with a line of glyphs, fallen by the
// culture's decay. Each stone carries a name in the people's style (`personName`) and the years it covers in the people's own
// reckoning (`shards.h`: the calendar; B-414: a death in the years of the age they reached, while the place stood). One function of the settlement's seed
// and the lore, the same for the view, the game and the harness (`vesperis_test graves`).
#include "ruins.h"
#include "lore.h"

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
// wide and 0.16 m thick with its glyphs, or the same lying flat; A-04: round them the people's way with its dead (`GV_DEAD`): a barrow
// behind each stone, a tomb, a low wall round the cemetery, a tower of silence, nothing
// `stonesOnly` for a work's plots (a necropolis, a battlefield's war graves): the stones alone, its tombs and walls its own buildings
void graveElements(const std::vector<Grave>& graves, const Culture& c, int lod, std::vector<Form>& out, bool stonesOnly = false);
std::string graveLine(const Lore& L, const Grave& g);   // "Thaelu, years 140 to 212 of Kethra" (born before the founding: "Thaelu, to year 212 of Kethra")
