#pragma once
// Z-06 (2026-11-17): the made organisms (R-425; docs/ideas/IDEAS-small-and-spacious.md 1.8, ITERATION-1.5.0.md Z-06). On the worlds of the
// peoples of the futuristic and far-future ages one species is theirs: a genome too regular to have evolved, and in it a watermark (as
// JCVI-syn1.0's genome held its makers' names: Gibson et al. 2010), a record of a genre of its own (the grammar's root `watermark`,
// core/watermark.vg) in the people's tongue, keyed with the species and read on the ship's decoder with the language its shards taught.
// Which species is a function of the world and its biota (one of its own, alive or asleep, by the world's hash), which of its peoples made it
// and the year of their count (in their last age) the generator's; its text the people's history at that year, written outside the catalogue
// (nothing of any people's records moves). The peoples' one file that reads src/life (tools/check.py's exception): this header names none
#include "shards.h"

// a world's made species: its index in the world's list (-1 none), the people that made it (0 or 1), the year of their count
struct MadeSpecies { int species = -1; int people = 0; int year = 0; };
MadeSpecies madeSpeciesOf(const StarSystem& sys, int body);
// its watermark as a record of the people (`Shard::watermark`: its text, the names it wrote, its year; no medium of the records'), with the
// people's lore (the decoder's, or `loreQuick`'s); `rec` the engine's record (the harness's checks)
bool madeWatermarkOf(const StarSystem& sys, int body, const Lore& L, const MadeSpecies& m, Shard& out, VgRecord* rec = nullptr);
