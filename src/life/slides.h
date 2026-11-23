#pragma once
// Z-04 (2026-11-17): a slide made from a vial (R-425; docs/archive/ideas/IDEAS-small-and-spacious.md 1.8, ITERATION-1.5.0.md Z-04). A vial's sample is
// put on the microscope's stage as a drop under its cover glass, prepared as a microbiologist would (concentrated or diluted by tens, so that its
// most crowded species stands a few of its bodies apart), or a rock's as a thin section of it. Each species it holds is a few of its cells that
// repeat across the slide at a period of their own (its tile: a few hundred cells in all, every species at its own density at every
// magnification), each at a depth in the drop (the settled on the glass, the swimmers through it, the gas-filled under the cover). The four
// things the explorer does to a slide are its events (a spot of light, the magnet beside the stage turned, the stage warmed, a drop of food); where
// its cells are and what they do at a time is closed-form from the events and the clock: a frame replayed is the frame seen, reached by skipping
// or not. A mount is a pure function of its sample, its microhabitat and its world's biota and forms; nothing of it is saved. R-438 (2026-11-19):
// the stage set to any warmth its drop stays liquid at (a kind moving in its range alone, at the pace its warmth gives it, stressed past its
// top), every cell that turns to the light or the food within its reach answering it, the dark's swimmers fleeing the light, lit light-feeders
// drawing the eaters; what each kind does told (`slideAnswer`)
#include "life/samples.h"
#include "life/morph_draw.h"
#include <cstdint>
#include <vector>

// what is done to a slide (IDEAS 1.8): a spot of light at the crosshair (phototaxis; R-438: a dark-dweller's swimmers flee it), the magnet beside
// the stage turned (magnetotaxis), the stage's warmth set (R-438: a kind moves in its range alone; past its top stressed: spores, cysts and the
// dying), a drop of food at the crosshair (chemotaxis; R-438: its scent spreading from it)
enum SlideDo { SD_LIGHT = 0, SD_MAGNET, SD_WARM, SD_FOOD, SD_COUNT };
extern const char* const SLIDE_DO_NAMES[SD_COUNT];
// an event on the slide's clock: the spot put at a place of a radius (or taken away), the magnet put beside the stage with its field at an angle
// `x` (R-436: turned, or taken away), the stage set to a warmth `x` (C; R-438), a drop of food put at a place (R-437: or the slide rinsed)
struct SlideEvent { int kind = SD_LIGHT; double t = 0, x = 0, y = 0, r = 0; bool on = true; };

// a species of the slide: its entry in the world's list and the sample's, its form as found, whether it lives and grows at the stage's
// warmth, what it turns toward and its speed, the warmth that stresses it and what it does then (the act, the form it ends in), its cells'
// period across the slide and their run in the mount's cells, the band of depths they keep, and the radius that holds one of them
struct SlideKind {
    int species = -1, held = -1;
    int form = CF_CELL;
    bool active = false;
    uint32_t taxis = 0;
    double speed = 0;
    double tMax = 1e9;
    int stress = -1, stressForm = CF_CELL;   // AN_SPORULATE to CF_SPORE, AN_ENCYST to CF_CYST, AN_LYSE to CF_GHOST; -1 none
    double tile = 100;
    int first = 0, n = 0;
    double z0 = 0, z1 = 0;
    double reach = 1, body = 1;   // the radius that holds one with its wander; its body across
    // R-438: whether it lives on the slide (a living cell its drop suits: it moves while the stage's warmth is in its range), its range's floor
    // and best, its pace's measure (the cardinal curve's at the vial's warmth: its speed is the species' there), a swimmer of the dark that
    // flees a bright light, a light-feeder (lit, what it gives off draws the eaters: Engelmann 1882)
    bool lives = false;
    double tMin = -1e9, tOpt = 0, paceRef = 1;
    bool flees = false, lightFeeder = false;
};
// a cell of the mount: its place in its kind's tile, its depth, its heading, its seed, its form (some of a species are found dividing or in a
// microcolony)
struct SlideCell { int kind = 0; double x = 0, y = 0, z = 0, angle = 0; uint64_t seed = 1; int form = CF_CELL; };
struct SlideMount {
    std::vector<SlideKind> kinds;
    std::vector<SlideCell> cells;
    double depthUm = 20;      // the drop under its cover glass (a thin section's 30)
    int prep = 0;             // tens it was concentrated (+) or diluted (-)
    double stageC = 20;       // the stage's warmth as found (the vial's)
    bool section = false;     // a thin section of rock
    uint64_t seed = 1;        // the slide's own: its grains, its copies' differences
    // where a first look rests: its way of seeing, magnification, place and the depth in focus (a cell of its most crowded species)
    int seeing = WS_BRIGHT;
    double mag = 400, cx = 0, cy = 0, focus = 0;
};
// a vial's mount: its sample, its microhabitat (the stage keeps its warmth), its world's biota and forms (`morphsOf`)
SlideMount mountOf(const Biota& B, const std::vector<Morph>& forms, const Sample& s, const Microhabitat& h);

// what the events make of the slide at a time: the stage's warmth and the warmth it is set to (the vial's till another is set), the magnet's
// field (and whether it is there), the spot, the food (R-437: when each was put, the food's scent spreading from then)
struct SlideState {
    double stageC = 20, setC = 20;
    bool magnet = false; double field = 0;
    bool spot = false; double spotX = 0, spotY = 0, spotR = 0, spotT = 0;
    bool food = false; double foodX = 0, foodY = 0, foodR = 0, foodT = 0;
};
SlideState slideStateAt(const SlideMount& M, const std::vector<SlideEvent>& ev, double t);

// a copy of a mount's cell (its cell and its tile) and, from `mountSpecimens`, where it is then (its place as drawn, um)
struct SlideRef { int cell = -1; int64_t ti = 0, tj = 0; double x = 0, y = 0; };
// the view a slide is looked at in: its centre and half its size (um), the depth in focus, the magnification (the depth of field), the way
// of seeing (the electron's and the section's in focus throughout)
struct SlideLook { double cx = 0, cy = 0, halfW = 20, halfH = 12.5, focus = 0, mag = 400; int seeing = WS_BRIGHT; };
// the specimens of a mount in a view at a time: each cell's copies there, where the events have taken them, doing what they make them do,
// blurred by their depth out of focus; `refs` what each is (closed-form in the clock; the forms are the world's, `morphsOf`)
void mountSpecimens(const SlideMount& M, const std::vector<Morph>& forms, const std::vector<SlideEvent>& ev, double t, const SlideLook& look,
                    std::vector<Specimen>& out, std::vector<SlideRef>* refs = nullptr);
// one copy as a specimen at a time (what tracking follows); false when there is none
bool mountSpecimen(const SlideMount& M, const std::vector<Morph>& forms, const std::vector<SlideEvent>& ev, double t, const SlideLook& look, const SlideRef& r,
                   Specimen& out);
// the copies' depth in the drop at a time (a swimmer's rises and falls; R-438: by its pace, given the events)
double mountDepth(const SlideMount& M, const SlideRef& r, double t, const std::vector<SlideEvent>* ev = nullptr);
// the depth of field at a magnification (um): the whole drop at x40, about a micrometre at x1,000
double depthOfField(double mag);
// R-437: how far a drop of food's scent has spread in a time (um: its front, a small molecule's diffusion)
double slideScentUm(double seconds);
// R-437: what a kind of the slide does at a time (its kind's index in the mount), for what is said of it and kept on its sheet: drawn into the
// light, fleeing it, drawn to the food (its scent reached it), gathered round lit light-feeders, along the field; moving in its range, still
// under or over it, stressed (its act begun)
enum SlideAnswerBits : uint32_t { SA_LIGHT = 1, SA_FLEES = 2, SA_FOOD = 4, SA_LIT = 8, SA_FIELD = 16, SA_MOVING = 32, SA_COLD = 64, SA_HOT = 128,
                                  SA_STRESSED = 256 };
uint32_t slideAnswer(const SlideMount& M, const std::vector<SlideEvent>& ev, int kind, double t);
