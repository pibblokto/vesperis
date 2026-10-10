// A-02: the forms a people builds with. C-01's ruins were boxes and domes; the ages need more: turned forms (cylinders,
// cones and frusta, spires, a cooling tower's waist, tanks, chimneys), pyramids and stepped masses, vaults, arches and
// arcades, thin shells, trusses, cables, rings, sections swept along a path and grown forms, each with its broken state for
// the decay (a tower snapped, a dome caved, a frame bent). A form is geometry alone, in the settlement's metres over the
// ground at its foot: the surface view draws it at a level of detail (`formMesh`) and as a silhouette beyond (`formProfile`),
// walks into it (`formColliders`), and the harness sheets it; nothing here reads a site, so the finders, the harness and
// the view see the same forms. A people's colours are four ramps (`FormPalette`) and its windows a pattern on a face's own
// metres (`WindowSpec`), not geometry.
#pragma once
#include "core/types.h"
#include <vector>
#include <cstdint>

enum FormKind {
    FK_BOX = 0,   // C-01's box: walls, columns, slabs, blocks. Broken: its top ragged in steps along its length
    FK_DOME,      // a dome of radius hx springing at y0 (sub 0 C-01's hemisphere, 1 a saucer, 2 pointed, 3 an onion). Broken: caved in
    FK_LATHE,     // a profile turned about the vertical (sub 0 a cylinder, 1 a frustum or cone, 2 a spire, 3 a cooling tower's waist,
                  // 4 a tank, 5 a chimney); radii hx across and hz along (an oval plan), `sides` 3..8 a prism. Broken: snapped, open, leaning
    FK_PYRAMID,   // a pyramid or a stepped mass: a ziggurat, a mastaba, a setback tower. Broken: its top tiers gone, its corners notched
    FK_VAULT,     // a vault of span 2 hx along the heading (length 2 hz) springing at y0, rising to y1 (sub 0 round, 1 pointed, 2 parabolic).
                  // Broken: a stretch of it fallen
    FK_ARCH,      // a wall 2 hx wide and 2 hz thick from y0 to y1 pierced by arched openings: a gate, a free arch, an arcade, an aqueduct,
                  // a viaduct (sub 0 round heads, 1 pointed, 2 flat). Broken: the keystones fallen, a pier snapped
    FK_SHELL,     // a thin shell (sub 0 a saddle over hx by hz, its low corners at y0 and its high ones at y1; 1 a dish of radius hx on a
                  // mast). Broken: holed, a corner sagging
    FK_TRUSS,     // a lattice (sub 0 a mast or pylon of `sides` legs, 1 a girder along the heading). Broken: bent, members gone
    FK_CABLE,     // a cable hung from the form's top to another point. Broken: snapped, hanging
    FK_TORUS,     // a ring of radius hx, flat on the ground, or standing on its rim (tilt pi/2). Broken: a sector gone
    FK_SWEEP,     // a section swept along a path (sub 0 an arc round the foot, 1 a straight line along the heading): a curved wall, a dam,
                  // an amphitheatre's tiers, an embankment. Broken: gaps, a ragged top
    FK_BLOB,      // a grown form: a lumpy body and its lobes. Broken: cracked open at the top
    FK_PLATE,     // C-17: a flat body of a drawn outline (an instrument's: a gourd's face, a box, a slab), its sides upright from y0 to y1, its
                  // top flat or domed. Broken: a bite out of its rim
    FK_COUNT
};
extern const char* const FORM_KIND_NAMES[FK_COUNT];

// One drawn piece (C-01's `RuinElem`): metres from the settlement's centre. The kind's numbers `p[]`:
//   FK_LATHE    1 the top's radius over the foot's (0 a cone); 2 the flank's power (1 straight, 2 a needle's hollow flank); 3 p0 the
//               waist's radius share, p1 its height share; 4 p0 the caps' rise over the radius; 5 p0 the top's radius share (a lip
//               over it). p3 > 0: open at the top, hollow
//   FK_PYRAMID  p0 the tiers (0 smooth), p1 the top's size over the base's (0 an apex), p2 the risers' share (0 sloped, 1 upright:
//               setbacks)
//   FK_VAULT    p0 the shell's thickness (m), p1 > 0 closed ends (walls under its two arches: a hall, a hangar)
//   FK_ARCH     p0 the openings (1 a gate .. an arcade), p1 an opening's half width over its bay, p2 the springing over y0 (m)
//   FK_SHELL    0: nothing; 1: p0 the dish's depth, p1 the mast's height (the dish's foot), the dish aimed by `tilt`
//   FK_TRUSS    p0 the top's half width over the foot's (a mast), p1 the panels, p2 a member's half thickness (m)
//   FK_CABLE    p0 p1 the far end (metres from the settlement's centre), p2 its height over the ground at the foot, p3 the sag (m);
//               hx the cable's half thickness
//   FK_TORUS    p0 the tube's radius
//   FK_SWEEP    p0 the arc's angle (sub 0, from the heading clockwise), p1 the section's half width at its foot, p2 at its top,
//               p3 the tiers stepped on its inner side (an amphitheatre)
//   FK_BLOB     p0 the lobes beside the body (0..4), p1 the lumpiness (0..1)
//   FK_PLATE    a superellipse hx across by hz along: p2 its corners' power (2 an ellipse, 8 near a rectangle, 1 a lozenge; `sides` 3..9 a
//               polygon instead), p0 its far end's width (+along) over its near end's, p1 a waist pinched just past its middle (negative: a
//               bulge), `sub` its middle leaning sideways by tenths of hx (a free-handed body), p3 its top's rise over its middle (m)
struct Form {
    int kind = FK_BOX;      // FormKind (C-01's `shape`: 0 a box, 1 a dome)
    int sub = 0;            // the kind's variant
    double x = 0, z = 0, heading = 0;
    double hx = 0.5, hz = 0.5;   // half extents across and along the heading; a round form's radius is hx
    double y0 = 0, y1 = 1;       // bottom and top over the ground at (x, z); a form with y0 under 0.01 stands on the ground (its foot sunk)
    double p[4] = {0, 0, 0, 0};
    int sides = 0;          // a turned form's faces: 0 round (as many as its size and the distance want), 3..8 a prism's flat faces
    int part = 0;           // what it is to the settlement: 0 a wall, 1 a column, 2 a slab, 3 a block, 4 a lintel, 5 a stela, 6 a harbour
                            // work, 7 a drift (C-08: the sand's bank), 8 earth (A-03: a mound, a bank, a spoil heap in the ground's own
                            // soil, the grass of a living world or a desert's sand); A-04: 9 water (a pool's, the water's bank), 10 bark
                            // and 11 leaves (a great tree, the vegetation's banks); T-04: 12..16 a record's (`FORM_PART_PALE`..`FORM_PART_METAL`).
                            // A lintel (4: a trim, a band, a post on a wall) has no collider
    bool glyphs = false;    // the front face carries a line of glyphs
    double broken = 0;      // 0 whole .. 1: how much of it fell (C-01's dome: the share of its height)
    double tilt = 0;        // the axis tipped from the vertical toward +along (radians): a broken tower's lean, a form lying on its side (pi/2)
    uint32_t seed = 0;      // a broken form's jagged edges, holes and gaps
    int slot = 0;           // the people's palette: 0 the structure, 1 the accent, 2 metal and glass, 3 light
    int pattern = 0;        // windows on its faces: 0 none, k the people's `windows[k - 1]`
    int building = -1;
    int material = -1;      // A-03: what it is built of (`BuiltMaterial`, galaxy/buildings.h), whose decay it shows; -1 none (the drifts' sand)
    bool absolute = false;  // T-03: y0 and y1 are heights over the reference level, not over the ground at its foot (a dam's wall across its
                            // valley, an aqueduct's arches to their channel): drawn and walked into where they are whatever the ground does
};

// T-04: a record's object (galaxy/records.h) in the records' own banks: bone, parchment and paper pale; wax, leather, vinyl, a glass plate and a
// drive's case dark; a painting's ochre; clay; metal (an engraved plate, a reel's flanges)
enum { FORM_PART_PALE = 12, FORM_PART_DARK, FORM_PART_OCHRE, FORM_PART_CLAY, FORM_PART_METAL };

// A-02: a people's windows: a pattern laid on a face by the face's own metres (along it and up it, like the grain), so a
// tower of thirty storeys costs what a blank one does. Far off, where a bay is a pixel or two, the pattern fades to its mean
struct WindowSpec {
    double bay = 3.2, storey = 3.5;   // a window's bay and a storey (metres)
    double w = 0.45, h = 0.5;         // the window's share of its bay and of its storey
    double sill = 0.3;                // the sill's height as a share of the storey
    int shape = 0;                    // 0 a rectangle, 1 an arched head, 2 a slit, 3 round, 4 a band (the storey glazed along its length)
    double dark = 14;                 // how much darker than its wall (shades)
    bool lit = false;                 // it glows (the light's ramp), whatever the sun
    double fromY = 0;                 // none under this height over the foot (a plinth, the doors' storey)
    int group = 1;                    // A-04: lights to a window (a pair, a group), split by their mullions
    double scatter = 0;               // A-04: the share of the cells left blank (a people that set its windows at random)
};
// A-02: a people's colours: the structure, the accent, metal and glass, and light, each a ramp in the banks the surface leaves
// free (`FORM_BANKS`). `own` takes the ground's material for the slot (C-01's walls: the rock, or the sand of adobe)
struct FormPalette {
    RGB colour[4] = {RGB(0.5f, 0.48f, 0.45f), RGB(0.55f, 0.36f, 0.26f), RGB(0.5f, 0.53f, 0.58f), RGB(1.0f, 0.78f, 0.45f)};
    bool own[4] = {true, false, false, false};
    WindowSpec windows[4];
    int nWindows = 0;
    float tint[4] = {1, 1, 1, 1};     // A-04: where a slot is the ground's own, how far it is tinted toward `colour` (0 the ground's, 1 the colour)
    bool glow = true;                 // A-04: slot 3 is light (the futuristic and the far future's); else a material lit like the rest (timber, rust)
};
constexpr int FORM_SLOTS = 4;
extern const int FORM_BANKS[2][FORM_SLOTS];   // a people's four banks: the first people's 18, 19, 20, 24, the second's 25, 26, 27, 28

// the drawn surface: polygons of up to eight corners (outward by their winding: the normal is (v1 - v0) x (v[n-1] - v0)) and lines
struct FormMesh {
    enum { SMOOTH = 1, TWO_SIDED = 2, FACE_UV = 4, PATTERN = 8 };   // per-corner normals; drawn from behind too (an open or broken form's
                                                                     // inside); u and v are the face's own metres; the windows go on it
    struct V { Vec3 p, n; double u = 0, v = 0; };
    struct Poly { int first = 0, n = 0, flags = 0; };
    struct Line { Vec3 a, b; double width = 0.1; bool main = false; };   // a truss's members far off, a cable: metres of thickness; `main`
                                                                          // (a leg, a chord, a cable) drawn however thin it is
    std::vector<V> v;
    std::vector<Poly> polys;
    std::vector<Line> lines;
    void clear() { v.clear(); polys.clear(); lines.clear(); }
};
// a form's surface at a level of detail (0 near, 1 middle, 2 far), its foot at `foot` (the view's metres: the settlement's
// centre plus (x, z), at the ground's height there) and `skirt` metres of it under the foot where it stands on the ground
// (a slope must not show under it). C-01's boxes and hemispheres come out as they always were drawn, to the bit
// `pxPerM` the pixels (of the 320 x 200 picture) a metre spans at its distance: a lattice's members are prisms only where they span
// a pixel and a half, lines where thinner
void formMesh(const Form& f, int lod, const Vec3& foot, double skirt, FormMesh& out, double pxPerM = 1e9);

// the discs it blocks the explorer with (B-405: each with the span of heights it stands over), in the settlement's metres and
// over the ground at the form's foot: rows along a wall, slices up a tower, an arch's piers and not its opening
struct FormDisc { double x, z, r, y0, y1; };
void formColliders(const Form& f, std::vector<FormDisc>& out);

// C-17: a plate's outline, `n` corners (at most 64) round its middle, a across and b along in the form's frame (the instruments' pictures
// draw the same outline: galaxy/instruments.h)
void formPlateOutline(const Form& f, int n, double* a, double* b);

// the silhouette far off: its width at a share t of its height (0 the foot .. 1 the top) over its widest, and its half width
// across a line of sight of the given heading (radians from north); its top over the ground at its foot; the radius of the
// ground it stands over (the cull and the colliders' reach)
double formProfile(const Form& f, double t);
double formHalfWidth(const Form& f, double sightHeading);
double formTop(const Form& f);
double formReach(const Form& f);
