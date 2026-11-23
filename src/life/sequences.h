#pragma once
// Z-06 (2026-11-17): the genomes and the tree of kin (R-425; docs/archive/ideas/IDEAS-small-and-spacious.md 1.4 and 1.8, ITERATION-1.5.0.md Z-06). A
// species' genome is a pure function of the species and its genesis: its alphabet the genesis's (four letters, six with Benner's Z and P, eight
// with hachimoji's S and B: Hoshika et al. 2019), its helix's hand the genesis's, and a marker every species of a genesis keeps (as 16S rRNA is
// ours: Woese and Fox 1977) changed down the branches of its lineage. The lineage is a tree in closed form, from the beginning down: the
// genesis's deep branches (`Microbe::clade`) part soon after it began; a kin cell's lines part as their cluster spread (Adams and Spergel
// 2005), by where their stars are (its sixteen-sector boxes first, then the four-sector boxes, then the stars), a system of its own genesis
// at once; a system's worlds part at their last exchange of rock (Melosh 1988),
// a drifter one of the star's it left; a world's species within that, by an order and the heights between neighbours (a comb: two species'
// common ancestor the highest between them); a carried spore is its source's species since the impact. Within a deep branch the distances
// rank as the lineage does: a world's nearest, then a system's, then a cell's, and none across geneses. A marker's letter at a place is its
// genesis's ancestor's, changed on each branch down to the species where that branch's hash at that place falls under its length's chance
// (a third of the places kept: a gene's conserved sites), so two kin share their letters as far as their common ancestor. Nothing is saved:
// what the explorer sequenced is the guide's
#include "life/biota.h"
#include <string>
#include <vector>

constexpr double SEQUENCE_S = 10;        // the sequencer's reading of a genome: ten seconds on the lab's clock (R-449; the plan's 8 had 90)
constexpr int MARKER_LETTERS = 1536;     // the marker's length (16S rRNA's about 1,500)
const char* alphabetOf(int letters);     // "ACGT", "ACGTZP", "ACGTZPSB"

// a species' place in its genesis's tree: its genesis and deep branch, its system's line (a drifter's: the star it left), its world (a carried
// spore's: the world it came from), its leaf in that world's comb of its branch, and the ages of the nodes above it (billions of years ago)
struct Lineage {
    bool valid = false;           // false: no genome to read (a protocell's chemistry, a fossil's stone, nothing there)
    Genesis genesis;
    int clade = 0;
    int64_t lx = 0, ly = 0, lz = 0;                 // its system's line: the sector of its star (a drifter's: the star it left)
    int64_t wx = 0, wy = 0, wz = 0; int world = -1; // its world: its star's sector and the body (a carried spore's: its source)
    int leaf = -1, leaves = 0;                      // its place in its world's comb of its branch, and how many the comb has
    std::vector<double> heights;                    // that comb's heights between neighbours (Gyr): heights[k] between leaf k-1 and k (heights[0] 0)
    double cladeGyr = 0;          // the deep branches parted
    // a kin cell's lines parted by where their stars are: its sixteen-sector boxes', the four-sector boxes' within one, the stars' within one
    // (a system of its own genesis: all `cladeGyr`)
    double cellGyr = 0, wideGyr = 0, nearGyr = 0;
    double exchangeGyr = 0;       // its system's worlds' last exchange of rock
    double carriedGyr = 0;        // a carried spore: since the impact that carried it
    bool ship = false;            // one of the ship's own (`shipSpecies`)
    bool made = false;            // a people's made species (peoples/made.h: the caller's to say)
    int plan = MP_SPHERE;
};
// a species of a world's list (its system's biotas read: `lifeOf`); a drifter's line its old star's (`drifterPast`)
Lineage lineageOf(const StarSystem& sys, int body, int species);
Lineage shipLineage(int k);       // the ship's own: home's world's, of home's genesis
// how long ago two had their last common ancestor (billions of years); -1 none (two geneses, or no genome)
double commonAncestorGyr(const Lineage& a, const Lineage& b);
// the tree's order: home's genesis first, then by genesis, deep branch, a kin cell's boxes, the system's line, the world, its comb, the carried
// last: every clade a run of neighbours, so two leaves' common ancestor is the oldest between them (the tree drawn from the neighbours')
bool treeBefore(const Lineage& a, const Lineage& b);
// the marker's letter at a place (0 .. letters-1) and a run of them as the alphabet's letters; the share of places two markers agree at
int markerLetter(const Lineage& L, int place);
std::string markerOf(const Lineage& L, int from, int n);
double markerAlike(const Lineage& a, const Lineage& b);
// its genome's length in letters (a small cell's 0.6-12 million, a large cell's 15-900, a colony's 30-2,000: its seed's)
double genomeLetters(const Microbe& s);
// a made genome's watermark in its letters (three letters a sign in four or six, two in eight; the text over and over): JCVI-syn1.0's way
// (Gibson et al. 2010), its makers' text written into the alphabet
std::string watermarkLetters(const std::string& text, int letters, int from, int n);
