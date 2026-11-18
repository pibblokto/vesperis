#pragma once
// Z-02: what a specimen is at a time, as the drawing reads it (life/morph_draw.h): bodies (a radial outline by the superformula, or a tube
// along a centreline), the parts inside them (dots: a nucleus, vacuoles, granules, plastids, an eyespot, droplets, gas vesicles,
// magnetosomes, grains, a spore) and the thin things (strands: flagella, cilia, pili, spines, stalks, threads, bridges), all in the slide's
// micrometres. `buildShape` makes them from a specimen's form, its act and the clock (life/morph_shape.cpp); the light ways draw them per
// pixel, the electron meshes them (life/morph_mesh.cpp). Internal to src/life
#include "life/morph_draw.h"
#include <vector>

namespace morph {

struct P2 { double x = 0, y = 0; };
constexpr int RTAB = 128;   // a radial outline's samples

enum BodyKind { BODY_RADIAL = 0, BODY_TUBE };
// the slide's banks (each way of seeing sets their ramps: life/morph_draw.cpp): the field and colourless matter; the pigments (PG_GREEN ..
// PG_BLACK at PIG + pigment - 1); the stain's blue, the plastids' red, a species' own light, the faint glow of walls; an eyespot; oil and
// sulphur; the walls' minerals; the world's grains; the section's interference colours and the fossils' carbon
enum SlideBank { SB_FIELD = 0, SB_PIG = 1, SB_STAIN = 8, SB_RED, SB_LUM, SB_AUTO, SB_EYE, SB_OIL, SB_SULPHUR, SB_MINERAL /* + Mineral, 7 */,
                 SB_GRAIN = 22 /* 3 */, SB_CARBON = 25, SB_IF = 26 /* the section's colours, 6 */, SB_COUNT = 32 };
inline int pigmentBank(int pg) { return pg > PG_NONE && pg < PG_COUNT ? SB_PIG + pg - 1 : SB_FIELD; }

struct Look {
    int bank = SB_FIELD;
    float density = 0.15f;      // absorbance through its full thickness (a bright field)
    float wall = 0.1f;          // the wall's thickness (um)
    float wallDensity = 0.35f;
    float phase = 1;            // its path difference at full thickness (phase contrast's dark)
    float refract = 0;          // refractile (a spore, a droplet): phase-bright, a dark rim in a bright field
    float halo = 0;             // a capsule's halo (um)
    float stain = 0.6f;         // the DNA stain's glow (a nucleoid's)
    float red = 0, lum = 0;     // chlorophyll's red, its own light
    int mark = MK_SMOOTH, marks = 0;
    uint64_t markSeed = 0;
    float zScale = 1;           // the electron's height over the half-width
    float ghost = 0;            // 0 whole .. 1 lysed: faint and broken
    int layers = 0;             // a layered vesicle's layers
    int mineral = MI_ORGANIC;
    int layout = 0;             // its ornament's layout: 0 over the surface, 1 radial rows (a centric box), 2 rows across a raphe (a
                                // pennate one), 3 along it (a ciliate's rows, a trumpet's stripes), 4 helical (a euglenoid's strips)
    float gapPhi = 0, gapW = 0; // an aperture: the wall open over gapW radians either side of gapPhi
    bool fossil = false;        // turned to carbon in old rock
    bool gel = false;           // a colony's jelly, a sheath of slime: the electron's preparation leaves none
};

struct Body {
    int kind = BODY_RADIAL;
    Look look;
    // radial: the centre, its axis (cos, sin), the half-width, length over width, the outline's radius by angle (its largest 1)
    P2 c; double ca = 1, sa = 0, R = 1, aspect = 1;
    float head = 0, tail = 0;   // how far it narrows toward its front (+u) and its rear (a teardrop, a spindle)
    float bend = 0;             // its axis's sag at its ends over its half-length (a bow)
    float r[RTAB] = {};
    // a tube: the centreline and its radius at each point; its height over the slide (the electron's helix)
    std::vector<P2> line; std::vector<float> rad, z;
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;   // bounds (um)
    int group = 0;              // the cell it belongs to (a colony's many)
    float lift = -1;            // the electron's: its height over the slide where it rides a colony's sphere (-1: resting on it)
};

enum DotKind { DOT_NUCLEUS = 0, DOT_NUCLEOLUS, DOT_VACUOLE, DOT_FOOD, DOT_GRANULE, DOT_PLASTID, DOT_PYRENOID, DOT_EYESPOT, DOT_DROPLET, DOT_GAS,
               DOT_MAGNET, DOT_GRAIN, DOT_SPORE, DOT_DEBRIS, DOT_CANAL, DOT_NUCLEOID, DOT_COUNT };
struct Dot {
    P2 c; double rx = 0.1, ry = 0.1, ca = 1, sa = 0;
    int kind = DOT_GRANULE, bank = SB_FIELD;
    float strength = 1;
    int group = 0;
    float lift = -1;            // the electron's: a colony's cell on its sphere, its height (-1: inside, unseen)
};

enum StrandKind { ST_FLAGELLUM = 0, ST_CILIUM, ST_PILUS, ST_SPINE, ST_STALK, ST_THREAD, ST_NET, ST_BRIDGE, ST_HAIR, ST_COLLAR, ST_SHEATH, ST_COUNT };
struct Strand {
    std::vector<P2> pts;
    float w0 = 0.02f, w1 = 0.02f;   // its width at the base and the tip (um)
    int kind = ST_FLAGELLUM, bank = SB_FIELD;
    float strength = 1;
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int group = 0;
};

// the gathering amoebae's signal: spiral bands round a centre (seen in a dark field: Gross et al. 1976)
struct Wave { P2 c; double R = 0, k = 0, phase = 0; int arms = 1; float strength = 0; };

struct Shape {
    std::vector<Body> bodies;
    std::vector<Dot> dots;
    std::vector<Strand> strands;
    std::vector<Wave> waves;
    double x0 = 1e30, y0 = 1e30, x1 = -1e30, y1 = -1e30;   // every part's bounds (um)
    void grow(double ax, double ay, double bx, double by) { x0 = std::min(x0, ax); y0 = std::min(y0, ay); x1 = std::max(x1, bx); y1 = std::max(y1, by); }
};

// a specimen at a time: its bodies, dots and strands on the slide (pixel: um a framebuffer pixel, for what is sized in pixels)
void buildShape(const Specimen& s, double t, double pixel, Shape& out);
// a radial body's signed distance (um, negative inside) at a point, and its local coordinates: along the axis, across, the angle
double radialDistance(const Body& b, double px, double py, double& u, double& v, double& phi);
// a tube's: the distance to its centreline less its radius; the segment's index and its share, the side
double tubeDistance(const Body& b, double px, double py, int& seg, double& t, double& side);
// the outline table's value at an angle (radians), interpolated
double outlineAt(const Body& b, double phi);
// a wall's ornament at a point of its surface (a unit direction for a radial body, (along, round) for a tube): its relief, -1 a hole .. 1 a
// ridge, and the cell of a mosaic it falls in (plates, grains)
double markAt(int mark, int marks, uint64_t seed, double a, double b, double c, int* cell = nullptr);
// the flat layouts' ornament (Look::layout 1-4) at a radial body's own coordinates (along its axis, across)
double flatOrnament(const Body& b, double u, double v);

}   // namespace morph
