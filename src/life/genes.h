#pragma once
// Y-02 (2026-11-19): a species' genes (R-443; docs/ideas/IDEAS-genes.md section 1, ITERATION-1.5.0.md Y-02). A genesis has one map of its genes'
// places (eighteen, each a gene's number: three vital, three quiet, a warmth, a salt, a size, a pigment, a food, a motion, three senses, a rest, a
// glow, a shape) and one code (its letters' weights, its tables), the same for all its kin: what is learned of gene 3 in one species holds in
// every species of its genesis. A species has the genes it uses, read from what it is (its `Microbe` and its form): nothing of generation 17
// moves, and its wild letters are chosen so that its genesis's code gives its own values (a heat-lover's warmth heavy in the heavy letters). A
// gene's working part is the run of letters its effect lies in: a sum of weights sets an amount (a warmth, a salt, a size, a motion's pace, a
// rest, a glow, a shape's length), a short run chooses from a table (a pigment, a food, a motion's kind, a sense's target, a small cell's
// shape). An edit moves a value from its parent's by the change in the code, within its limits (`applyEdits`); a gene cut out loses its work,
// and a vital gene changed or cut leaves a line that does not live. A strain's genes are its parent's with its move's gene changed
// (`strainGenes`), so the sequencer reads which gene a strain's move was in. How it lives and its plan's family never change. Y-04: a gene's
// switch (`GeneSwitch`, its kind's own as read): a gene off where its condition fails loses its work as one cut and pays no price, its culture
// laid out by its state (`CultureKind::switches`), its drop as its flask has it (`switchedLine`)
#include "life/cultures.h"
#include "life/morphs.h"
#include <cstdint>
#include <string>
#include <vector>

// what a gene does: its copying, its membrane or its making (vital: changed or cut, the line does not live); quiet (nothing in the lab shows
// it: most of a real genome, Baba et al. 2006); its warmth (its range), its salt (its band), its size (its cells'); a pigment (the light that
// feeds it, its colour), a food (what a chemical-eater burns), its motion (still or moving, its kind among its plan's, its pace), a sense (what
// it turns to), its rest (spores, cysts or the gatherers' fruit at its end), its glow (a luminous crowd's), its shape (a small cell's plan)
enum GeneKind { GN_VITAL = 0, GN_QUIET, GN_WARMTH, GN_SALT, GN_SIZE, GN_PIGMENT, GN_FOOD, GN_MOTION, GN_SENSE, GN_REST, GN_GLOW, GN_SHAPE, GN_COUNT };
extern const char* const GENE_KIND_NAMES[GN_COUNT];   // the map's words: "VITAL", "QUIET", "WARMTH", ... "SENSE", ... "SHAPE"
constexpr int GENE_PLACES = 18;                        // a genesis's map
int genePartLetters(int kind);                         // a kind's working part: warmth 12, salt and motion 6, vital and quiet 8, food 3, ...
// what a sense turns to: nothing (cut, or a run its code reads so), the light, away from it (a dark-dweller's flight), the food, along the
// field, the warmth (read and kept: the slide has no gradient of warmth)
enum GeneSense { SE_NONE = 0, SE_LIGHT, SE_DARK, SE_FOOD, SE_FIELD, SE_WARMTH, SE_COUNT };
extern const char* const GENE_SENSE_NAMES[SE_COUNT];   // "NOTHING", "LIGHT", "AWAY FROM LIGHT", "FOOD", "THE MAGNET", "WARMTH"

// a genesis's code: its map (place p is gene number p + 1: its kind, and which of its kind's places it is), each letter's weight (the alphabet's
// order, `alphabetOf`; home's pinned A 0, T 1, C 2, G 3), and its tables, each by its run of letters read as a number in the alphabet's base:
// a pigment's two letters, a food's three, a motion's kind's two (0 still, else the plan's kinds in turn), a sense's two, a shape's two
struct GeneCode {
    uint64_t genesis = 0;
    int letters = 4;
    int kind[GENE_PLACES] = {}, slot[GENE_PLACES] = {};
    int weight[8] = {};
    std::vector<int> pigment, food, motion, sense, shape;
};
GeneCode geneCodeOf(const Genesis& g);

// Y-03: how far a gene not its own came, by its donor's place (the tree's distance as the stars have it): its own species', its world's, its
// system's, another star's; and what it costs a line's growth (Bentley et al. 1990's burden: 0, 3, 6 and 10%)
enum GeneFrom { GFROM_OWN = 0, GFROM_WORLD, GFROM_SYSTEM, GFROM_STAR, GFROM_COUNT };
extern const double GENE_PRICE[GFROM_COUNT];

// a gene of a species or a line: its place in its genesis's map, its kind (and which of its kind's places), its letters in all, its working
// part's letters (the alphabet's), whether it is cut out (a line's); Y-03: how far it came and whose it was (the guide's key of the species it
// came from, "" its own), kept down its lines; Y-04: its switch (`GeneSwitch`: its kind's own as read, kept down its lines, moved and copied with
// it)
struct MicrobeGene {
    int place = 0, kind = GN_QUIET, slot = 0;
    int length = 0;
    std::string part;
    bool cut = false;
    int from = GFROM_OWN;
    std::string origin;
    int sw = GW_ALWAYS;
};
// Y-04: a kind's own switch, as every species has it: a glow's WHEN CROWDED (its quorum), a rest's WHEN STARVING (at its end), every other
// ALWAYS; a rest's ALWAYS works as its own (it rests only as it ends)
int geneOwnSwitch(int kind);
// a species' or a line's genes, in its map's order (a gap where it lacks one its kin have); what its rest and its glow keep while their letters
// say so (its life cycle's resting bits, its glow's crowd)
struct MicrobeGenes {
    bool valid = false;           // false: no genome (a protocell's chemistry, a fossil's stone)
    Genesis genesis;              // its genesis (its map, its code, its forms' style)
    int letters = 4;
    std::vector<MicrobeGene> genes;
    uint32_t restCycle = 0;       // the LifeCycle bits its rest gene keeps (spores, cysts, the gatherers')
    double sporeYears = 0;        // and how long its spores last
    const MicrobeGene* at(int place) const;
    MicrobeGene* at(int place);
};
// a species' genes, read from what it is (its values; its form's body and length): its genesis's map, a gene for each thing it uses (a pigment
// for one that has one or lives by light, a food for a chemical-eater, a motion for one that moves, a sense for each thing it turns to, a rest
// for one that ends resting, a glow for a luminous one, a shape for a sphere, a rod, a curved rod or a spiral), one to three quiet
MicrobeGenes microbeGenesOf(const Microbe& s, const Morph& m, const Genesis& g);
// a dark-dweller's swimmers flee a bright light (R-438): one that moves, no light-feeder, turned to neither the light nor the field, of the
// soil, the silt, a vent, the rock or a plume; a line's own where its senses were edited (`Microbe::flees`)
bool fleesLight(const Microbe& s);

// an edit of a gene: its working part's letters as changed by hand, or the gene cut out; a line's edits each a gene once (its parent's genes
// the rest). As the guide keeps them: "3=ACCG;8=-" (the place's number, its letters or a cut). Y-03: or a gene moved in from a donor of its
// genesis (a species read, a culture kept): its gene at that place, its letters and its value the donor's, into a gene the line has, a gap or a
// gene cut out ("5+ACCGTA@<the donor's key>"); or a copy of its own gene `from` at a free place of its kind (Ohno 1970: "14*AC@13"), then
// changed by hand as any. A moved gene's donor is read by the game before the edit is applied (`read`: its kind and form, its gene, whose it
// was and how far it came), never kept. Y-04: and its switch set (`sw`, -1 left as it was: "16~DARK", or after its letters "5=ACCG~COLD")
enum GeneEditHow { GX_HAND = 0, GX_MOVED, GX_COPY };
struct GeneEdit {
    int place = 0; bool cut = false; std::string part;
    int how = GX_HAND;
    int from = -1;
    std::string donor;
    bool read = false; CultureKind donorKind; Morph donorForm; MicrobeGene donorGene; int far = GFROM_OWN; std::string origin;
    int sw = -1;
    bool switchOnly() const { return sw >= 0 && !cut && how == GX_HAND && part.empty(); }
};
std::string geneEditsText(const std::vector<GeneEdit>& e);
std::vector<GeneEdit> geneEditsOf(const std::string& text);

// what a gene's working part gives (for the editor's words and the said): the amounts by its letters' weights, a table's choice
int geneWeight(const GeneCode& c, const std::string& part, int from = 0, int n = -1);
int geneTable(const GeneCode& c, const std::vector<int>& table, const std::string& part, int from, int n);
// the plan's moving kinds a motion's table chooses among (a rod's runs and tumbles, glides, twitches); none for a still plan
int planMotions(int plan, int* out);   // out holds 4
double kindPace(int motion, const Morph& m);   // Y-03: a motion kind's pace at its kind's middle, in a form's body (a moved motion's share is of it)

// a line from its parent: its kind and form (read from its world and its lines before it) and its genes (changed to the line's) with its edits
// applied: a weight's change moves its value from the parent's (a warmth's range a step of its cell's a weight, a salt's band, a size by two to
// the third of the change, a pace by two to the quarter), a table's choice is what its run reads; a gene cut loses its work (its warmth only
// round its best, fresh water, half its size, no pigment, no food, still, no sense, dying at its end, no glow, a sphere); a vital gene changed
// or cut, `dead`. Its values stay in its cell's range and its family's sizes; how it lives and its plan's family never move. Y-03: a gene moved
// in takes its donor's value where the donor was read (its warmth's range, its salt, its cells' width, its pace's share of its kind's, its glow's
// crowd, its shape's length), then its letters' change from the donor's as any edit; one in a gap or a gene cut out (a copy, a donor not read)
// its letters alone (a table's run, an amount at the middle of its weights), its rest its family's (spores, cysts; the gatherers' fruit). Copies
// after the rest, so each copies its gene as edited. Y-04: a switch set; a gene moved in keeps its donor's switch, a copy its gene's. Its kind and
// form are its genes all on; its genes switched from their own to a condition (or NEVER) listed for its culture (`CultureKind::switches`: what
// each loses off, its price the game's), its glow lit by its switch where that is not its crowd (`glowAny`)
void applyEdits(CultureKind& k, Morph& m, MicrobeGenes& g, const std::vector<GeneEdit>& edits);
// Y-04: a line as a state has it (its flask's light, the cold, its crowd, starving: `CultureState`): each gene its switch turns off there loses its
// work as one cut (its form too: the drop's), not its price; the kind left with no switches
void switchedLine(CultureKind& k, Morph& m, const MicrobeGenes& g, uint32_t state);

// Y-03: what a line pays for its genes, as a share of its growth (`CultureKind::burden`): each gene not its own by how far it came
// (`GENE_PRICE`), a motion's power past its species' own (4% a doubling of its pace; a motion where its species is still 5%), a glow not its
// own (4%) or lit at a smaller crowd than its own (2% a halving; Y-04: lit by its switch, as at its least, 0.5%); at most half (`traitsOf`).
// Y-04: a gene's part of it (`genePrice`: a switched gene's price, not paid while it is off)
double geneBurden(const CultureKind& wild, const Morph& wildForm, const CultureKind& k, const Morph& m, const MicrobeGenes& g);
double genePrice(const CultureKind& wild, const Morph& wildForm, const CultureKind& k, const Morph& m, const MicrobeGene& gene);
// Y-03: what a gene needs to do its work in a line (found by trying, said when seen): a sense its cells' motion, a food a chemical-eater, a
// pigment's light a light-feeder (it colours any), a shape one of the four small cells' plans, a motion a plan that moves
enum GeneNeed { NEED_NONE = 0, NEED_MOTION, NEED_EATER, NEED_LIGHT, NEED_SHAPE, NEED_MOVER };
int geneNeeds(const CultureKind& k, const MicrobeGene& gene);

// a strain's genes from its parent's (its kind before its move): its move's gene's letters stepped its way (a highest warmth up, heavier)
void strainGenes(MicrobeGenes& g, const GeneCode& c, const CultureKind& before, int what, double value, uint64_t seed);
// the genes two lines differ in (their places: Y-04, their letters or their switch), and a kind's value in words as its letters give it (the
// editor's, once the gene is known)
std::vector<int> genesDiffer(const MicrobeGenes& a, const MicrobeGenes& b);
std::string geneValueWords(const CultureKind& k, const Morph& m, const MicrobeGene& gene, const GeneCode& c);
