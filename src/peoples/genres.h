// The records' texts (H-04; docs/reference/civilisations/records-and-grammar.md): written from a people's history by the grammar
// (peoples/grammar.h, peoples/grammar/). The genre is the grammar's root's (`record`) by the record's age and medium, weighed by what the
// history offers at its date; the model (`RecordModel`) gives the grammar its inputs (the world, the people, the history at the date, the
// record's writer, reader, subject, place and season, the other people); the plan's directives (`@news`, `@entries`, `@years`, `@death`,
// `@deed`, `@life`, `@event`) pick the events near the date by salience, each told in the genre's register (`annal.<kind>`, ...). A
// people's records are one catalogue (an event told already counts less), and every person and place a record names is the history's, alive
// or standing at the date.
#pragma once
#include "grammar.h"
#include "shards.h"
#include <memory>
#include <string>
#include <vector>

struct History;

// the facts, the entities, their paths and fields, the directives the records' grammar reads (one schema for every grammar file)
const VgSchema& recordSchema();
// the world's facts into a model (the names, the sky, the land, the climate, the people's ways): the records' and the charts' captions'
void recordWorldFacts(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, const History* H, VgModel& M);

// a people's text records written (`out.items`, by index; the music and the charts left empty): the history read once, the records
// in index order with the grammar's memory and the events told kept across them
void writeRecordTexts(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, const VgGrammar& G, ShardTexts& out);

// the records' genres: the root's alternatives by their first rule ("letter", "annal", "the last")
std::vector<std::string> recordGenreNames(const VgGrammar& G);
// E-11: the words a fact (`age`) or an entity's field (`.kind`, `$e.kind`) takes, a table each in the schema's order (`vesperis_test grammar
// schema`; `grammar where` reads `.kind` and `.how` for the names the directives build)
struct RecordKeyTable { const char* name; std::vector<std::string> keys; };
const std::vector<RecordKeyTable>& recordSchemaKeys();

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do

// the names a record may write at its year (the unit's "no unknown place or figure"): the lore's, the places founded by then, the persons
// born by then; a name outside them is unknown
bool recordNameKnown(const History& H, const Lore& L, int year, const std::string& name);
