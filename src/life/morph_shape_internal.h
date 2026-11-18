#pragma once
// Z-02: the shape builders' own (life/morph_shape.cpp, morph_plans.cpp, morph_acts.cpp): the builder that places parts in a specimen's frame,
// where a specimen is at a time, and what an act asks of a cell's drawing
#include "life/morph_shape.h"
#include "core/types.h"

namespace morph {

// where a specimen is: its place and heading, its roll about its axis (the helical swim, the colony's), its beat's phase; mirrored (its
// genesis's hand), tumbling (a run's end), gliding backward
struct Pose { double x = 0, y = 0, angle = 0, roll = 0, beat = 0; bool mirror = false, tumbling = false, backward = false; };
double motionRadius(const Morph& m);
double frontRadiusUm(const Morph& m);   // a colony's front's reach on a plate (um)
Pose poseAt(const Morph& m, const Specimen& sp, double t);

struct Lobe { double phi = 0, amp = 0, w = 0.4; };
void fillOutline(Body& b, int sym, double n1, double n2, double n3, double wander, uint64_t seed, double t, double wobble, const Lobe* lobes = nullptr,
                 int nl = 0);
void edgeOf(const Body& b, double phi, P2& p, P2& n);

// places parts in a frame (u along the axis, v across) on the slide; `group` tags them (a colony's cells)
struct Build {
    const Morph& m;
    const Specimen& sp;
    double t, pixel;
    Shape& out;
    double x = 0, y = 0, ca = 1, sa = 0;
    bool mirror = false;
    int group = 0;
    Build(const Morph& M, const Specimen& S, double T, double px, Shape& o) : m(M), sp(S), t(T), pixel(px), out(o) {}
    void place(double X, double Y, double ang, bool mir) { x = X; y = Y; ca = std::cos(ang); sa = std::sin(ang); mirror = mir; }
    P2 at(double u, double v) const;
    double angle(double a) const;
    Body& radial(double u, double v, double ang, double R, double aspect, const Look& lk);   // its outline a circle until filled, then `finish`
    void finish(Body& b);
    Body& tube(const std::vector<P2>& local, const std::vector<float>& rad, const Look& lk, const std::vector<float>* z = nullptr);
    void dot(double u, double v, double rx, double ry, double ang, int kind, int bank, float strength = 1);
    void strand(const std::vector<P2>& local, float w0, float w1, int kind, int bank, float strength = 1);
    void bound(Body& b);
};

// what an act asks of one cell's drawing (life/morph_acts.cpp sets it, life/morph_plans.cpp honours what applies to its plan)
struct CellCtx {
    double scale = 1;        // its size over its form's
    double grow = 0;         // a dividing cell's growth, 0..1
    double pinch = 0;        // its constriction, 0..1 (1: two)
    double part = 0;         // how far its two daughters have parted (over its width)
    double ghost = 0;        // its lysis, 0..1
    double round = 0;        // its rounding into a cyst, 0..1
    double limbs = 1;        // its flagella's, cilia's, pods' share (they draw back)
    double spore = -1;       // a forespore inside it: its stage 0..1 (-1 none)
    double bud = 0;          // a bud's growth, 0..1
    double split = 0;        // many daughters inside it (multiple fission), 0..1
    double glow = -1;        // its crowd's glow 0..1 (-1: its own)
    double wake = -1;        // a spore waking, 0..1 (-1 none)
    double gape = 0;         // a hole opened in its wall, 0..1 (lysis, a colony's release)
    double toward = 0;       // a lobe's reach toward the act's other (engulfing), 0..1
    double towardAngle = 0;
    int form = CF_CELL;
    bool single = false;     // one cell of a group's kind (an act's: division, a spore, a bridge show one)
};

// a plan's cell (or the plan's group: a chain, a cluster, a colony) at the builder's place, as the context asks
void buildCell(Build& B, const Pose& P, const CellCtx& C);
// the parts every plan shares: a cell's look; a flagellum as a travelling wave from a base; cilia round a radial body's edge; the inside
Look cellLook(const Morph& m, double R);
void flagellum(Build& B, double u, double v, double dir, double len, double width, double phase, double amp, double waveLen, int kind, float strength);
void ciliaRound(Build& B, const Body& body, double from, double to, int n, double len, double width, double beat, double wave, float strength,
                double curl = 0, int kind = ST_CILIUM);
void pushStrand(Shape& out, std::vector<P2>&& pts, float w0, float w1, int kind, int bank, float strength, int group);
// a cell's own frame for its insides, in the builder's coordinates: a radial body's axis (centred, half-length L, half-width R, tapers and
// bend), or a tube's centreline
struct InFrame {
    bool tube = false;
    double L = 1, R = 1, head = 0, tail = 0, bend = 0;
    double cu = 0, cv = 0, ca = 1, sa = 0;   // a radial body's centre and axis in the builder's coordinates
    std::vector<P2> line; std::vector<float> rad;
    P2 point(double s, double w) const;   // s -1 rear .. 1 front, w -1 .. 1 across
    double half(double s) const;          // the half-width there
    double dir(double s) const;           // the axis's angle there
};
InFrame frameOf(const Build& B, const Body& b, double L, double R);   // a radial body's, where it lies in the builder's frame
void insides(Build& B, const InFrame& F, const CellCtx& C, int cellIndex);

}   // namespace morph
