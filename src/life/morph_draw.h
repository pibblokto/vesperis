#pragma once
// Z-02 (2026-11-16): a form on a slide (life/morphs.h; IDEAS-small-and-spacious.md 1.5 and 1.6). A slide is drawn into a framebuffer of its
// own: every bank the slide's, the ramps its way of seeing's (`slidePalette`), the mush after as the scenes' (the caller's). A specimen is
// one cell or one group of them (a chain, a cluster, a colony) doing an act; everything it shows at a time is closed-form in the slide's
// clock from its form, its seed and its act's start, so a frame replayed is the frame seen, reached by skipping or not, at any thread
// count. The light ways of seeing draw each pixel from the bodies' distances (relief from the edge, the insides, the appendages as
// travelling waves, anything thin sized in screen pixels); the electron's is a mesh of the one cell lit from one side; the section's a
// polarised thin section of the rock with the fossils' carbon in it
#include "life/morphs.h"
#include "core/framebuffer.h"
#include <vector>

// what a specimen does (IDEAS 1.6): lives (moves and beats); divides (fission, many at once, a filament breaking); buds; lets its colony's
// daughters out; makes a spore and lyses; wakes from a spore; rounds into a cyst; lyses; engulfs another; joins another by a bridge; turns to
// a spot of light; to the field; glows as its crowd grows (quorum); gathers in spiral waves to a mound; rises into a stalk with a head of
// spores; spreads as a colony's branching front; grows a protocell's thread and splits it
enum Act { AN_LIVE = 0, AN_DIVIDE, AN_BUD, AN_RELEASE, AN_SPORULATE, AN_WAKE, AN_ENCYST, AN_LYSE, AN_ENGULF, AN_BRIDGE, AN_TO_LIGHT, AN_TO_FIELD,
           AN_GLOW, AN_GATHER, AN_FRUIT, AN_FRONT, AN_THREAD, AN_COUNT };
extern const char* const ACT_NAMES[AN_COUNT];
bool actSuits(const Morph& m, int act);     // its species can do it
double actSeconds(const Morph& m, int act); // its time (AN_LIVE's: a beat's cycle)

struct Slide {
    double cx = 0, cy = 0;          // the view's centre on the slide (um)
    double umPerPx = 0.125;         // um a 1x pixel (x1000: 40 um across the view)
    int seeing = WS_BRIGHT;
    double t = 0;                   // the slide's clock (s)
    double lamp = 1;                // the lamp, 0..1
    double turn = 0, tilt = 0.55;   // ELECTRON: its cell turned about the slide's normal, tilted toward the eye (radians)
    double stage = 0.3;             // SECTION: the stage's turn between the crossed polarisers (radians)
    double fieldAngle = 0;          // a magnetic field's direction across the slide (the turn to it)
    double spotX = 0, spotY = 0, spotR = 0;   // a spot of light (the turn to light), um
    uint64_t seed = 1;              // the slide's own: its grains, its debris
    bool lod = false;               // Z-04: a cell under 2.5 of the view's 1x pixels drawn as a speck, its body alone (the microscope's low powers)
    int step = 1;                   // Z-04: 2 at the higher scales: the light ways a pixel in four (each its block's), the electron's mesh half as fine
};

struct Specimen {
    const Morph* m = nullptr;
    int form = CF_CELL;
    int act = AN_LIVE;
    double x = 0, y = 0, angle = 0;   // where it is on the slide (um) and its heading
    double t0 = 0;                    // when its act began on the slide's clock
    uint64_t seed = 1;                // its own: the individuals of a species differ
    const Morph* other = nullptr;     // the prey it engulfs, the partner it joins
    // Z-04: the microscope's (their defaults draw it as the plates do): held still (a spore, a cell that does not grow at the stage's warmth:
    // its jiggle alone), its motion's reach scaled (a crowd in a spot, a stream along the field), the drift its place is on (um/s: its heading
    // turns with it), its depth's blur out of focus (um)
    bool still = false;
    double wander = 1, vx = 0, vy = 0;
    double blurUm = 0;
    double clock = -1;   // R-438: its own clock for its moves (its kind's pace through the stage's warmth); the slide's when negative
};

// the magnifications of the light ways (x20 .. x1000 the objectives, x2000 and x4000 the camera's zoom on the oil one) and the electron's
// (x50 .. x40,000); the one that holds a form's body (and its appendages to twice its size) in a share of the view's height
double magnificationFor(const Morph& m, int seeing, double share = 0.8);
inline double umPerPxAt(double mag) { return 40.0 * 1000.0 / mag / UW; }

void slidePalette(uint8_t* pal, int seeing, double lamp = 1);
// clears the framebuffer and draws the specimens (in order: a later one over an earlier); the mush is the caller's
void drawSlide(Framebuffer& fb, const Slide& v, const std::vector<Specimen>& cells);
// the banks a way of seeing sets (its palette's ramps), their count
int seeingBanks(int seeing, bool used[BANKS]);
// the radius round its place that holds a specimen at a time (um)
double specimenExtentUm(const Specimen& s, double t);
// the box its parts cover at a time (um; empty: x0 > x1)
void specimenBox(const Specimen& s, double t, double& x0, double& y0, double& x1, double& y1);
// where a specimen is at a time and its heading (the microscope's tracking follows it)
void specimenPlace(const Specimen& s, double t, double& x, double& y, double& angle);
// the ways of seeing that suit a form, the first the best (the plates')
std::vector<int> seeingsFor(const Morph& m);
