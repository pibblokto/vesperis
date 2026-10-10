// H-04 (2026-10-09): the records' texts (`docs/ideas/IDEAS-civilisation-expansion.md`, section 9; `docs/ideas/RESEARCH-records.md`,
// sections 3 and 6). A people's records are written from its history by the grammar (galaxy/grammar.h; the files in galaxy/grammar/):
//
// * The genre is the grammar's (`record`, its root): by the age of the record's year and the medium T-04 made it in (a tally on a bone,
//   an annal on a tablet, a newspaper on printed pages, a machine's log on a drive), weighed by what the history offers at the date (a
//   letter needs two of a family alive, an epitaph a death, a timetable a railway), so a people writes as its age wrote.
// * The model (`RecordModel`) gives the grammar the dozens of inputs: the world (its sky, its land, its water and its seasons), the
//   people (its faith, what it lived by and valued, its sacred number and its focal work, how it kept its dead), the history at the
//   record's date (the era and its government, the ruler and the reign's year, the war or the sickness of the time, the harvest, the
//   inventions there were, the institutions, the stage of the collapse), the record (its writer, reader and subject, cast from the
//   history's persons alive at the date; its place; its season; the writer's temper) and the other people.
// * The plan's directives pick what a record tells (`@news`, `@entries`, `@years`, `@death`, `@deed`, `@life`, `@event`): the events
//   near its date by salience (the event's weight, how near it was, its place, the writer's stake), each told in the register of the
//   genre (`annal.<kind>`, `letter.<kind>`, `news.<kind>`, `song.<kind>`, `log.<kind>`, `picture.<kind>`). A people's records are one
//   catalogue: an event told already counts half, a great one told by another register counts more (the annal, the letter and the song
//   of one event), and the grammar's memory spreads its alternatives.
// * Every person and place a record names is the history's, alive or standing at the date (the harness and the unit check it); the
//   collapse is told late and from many sides because the history puts it late and the genres of its years are its witnesses.
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

// the names a record may write at its year (the unit's "no unknown place or figure"): the lore's, the places founded by then, the persons
// born by then; a name outside them is unknown
bool recordNameKnown(const History& H, const Lore& L, int year, const std::string& name);
