// Software rasteriser: scanline triangles with a 1/z depth buffer, Gouraud
// intensity and an optional world-mapped grain texture; 3D lines and points.
// Vertices are given in view space: x right, y up, z forward.
#pragma once
#include "framebuffer.h"
#include "noise.h"

struct Proj {
    double f = 277;                 // focal length in pixels
    double cx = FBW * 0.5, cy = FBH * 0.5;
    static Proj fromHFov(double hfovDeg) {
        Proj p;
        p.f = (FBW * 0.5) / std::tan(hfovDeg * 0.5 * DEG);
        return p;
    }
};

struct RVert {
    double x = 0, y = 0, z = 0;   // view space
    double shade = 32;            // intensity 0..63
    double u = 0, v = 0;          // grain texture coordinates (texels)
    double w = 1;                 // B-311: weight of the first material (1 at its corners, 0 at the other's), two-material cells
};

enum BlendMode { BLEND_REPLACE = 0, BLEND_ADD = 1, BLEND_MAX = 2, BLEND_DARKEN = 3, BLEND_REFLECT = 4 };

// A-02: windows as a pattern on a face, by the face's own u and v in metres (along it, up it): a cell a bay wide and a storey
// high, the window inside it; where a cell spans few pixels the pattern fades to its mean, so a far tower does not shimmer
struct FacePattern {
    double bay = 3.2, storey = 3.5;          // the cell (metres)
    double x0 = 0.3, x1 = 0.7, y0 = 0.3, y1 = 0.8;   // the window within its cell (shares)
    int shape = 0;                           // 0 a rectangle, 1 an arched head, 2 a slit, 3 round, 4 a band, 5 a triangle (A-04), 6 a keyhole
    int group = 1;                           // lights to a window, split by mullions (a pair, a group)
    double scatter = 0;                      // the share of the cells left blank (by a hash of the cell)
    double dark = 14;                        // shades under the wall
    int litBank = -1;                        // >= 0: the window glows, drawn in this bank at `litShade` whatever the light
    double litShade = 50;
    double litShare = 0;                     // > 0: that share of the window cells lit, by a hash of the cell, and where the pattern fades to its mean its lit windows stay a
                                             // scatter of points (a city's lights left on, from afar); 0: every window lit where `litBank` (the far future's)
    double fromV = 0;                        // no windows under this height
    double meanShare = 0.2;                  // the window's share of its cell (the far mean)
    // the people's dress on its walls, by the face's own metres as the windows are: a surround round each window `frame` wide (a share of
    // the cell's width; as many metres up as across) `frameShade` lighter (+) or darker (-) than the wall; shutters each side of it `shutter`
    // wide (a share) at `shutterShade`; courses `stripe` metres high, every other one `stripeShade` off; a dado `dado` metres high `dadoShade` off
    double frame = 0, frameShade = 0, shutter = 0, shutterShade = 0, stripe = 0, stripeShade = 0, dado = 0, dadoShade = 0;
    double meanDress = 0;                    // the dress's mean shade over a cell (the far mean)
};

// I-01: a view-space vertex projected: where it lands, its 1/z, and its shade, u, v and weight over z (the fill interpolates them
// in screen space, perspective-correct)
struct ScreenVert { double sx, sy, iz, shade, u, v, w; };

struct RasterParams {   // (a field added here joins `sameParams` in raster.cpp)
    int bank = 0;                 // bank index 0..15
    const GrainTexture* grain = nullptr;    // micro grain (M10-05: faded by grainWeight)
    double grainScale = 1.0;      // texels per unit of u/v
    const GrainTexture* grain2 = nullptr;   // material meso tile, world-mapped
    double grain2Scale = 4.0;
    double grainWeight = 1.0;     // 0..1, distance fade of both grains
    // B-311: two-material cells. A pixel goes to `bank` where the interpolated vertex weight w, wobbled by the world-mapped
    // edge noise, is over a half, else to `bank2` with its own meso tile and a flat-shade offset: the boundary between
    // two materials becomes an irregular contour through the cell instead of the cell's square edge
    int bank2 = -1;
    const GrainTexture* grain2b = nullptr;
    double shade2 = 0;            // the second material's shade minus the first's
    const GrainTexture* edge = nullptr;     // signed noise, +-64
    double edgeScale = 0.25;      // texels per unit of u/v
    double edgeAmp = 0.6;         // weight units per full noise amplitude
    bool cutout = false;          // pixels whose wobbled weight falls under a half are not drawn at all (ragged leaf clusters); the edge noise and the vertex weights as for two materials
    double quantize = 0;          // >0: snap the interpolated shade to this step (quantised gradient mode)
    double zbias = 1.0;           // the depth tested and written is iz x zbias; under 1 pushes a surface back a little (a coarser terrain ring loses to the finer one where they coincide)
    double darken = 1.0;          // BLEND_DARKEN: multiply the existing intensity (blob shadows, a glass's tint)
    bool sheen = false;           // BLEND_DARKEN as tinted glass that catches a light: the face's own shade in its bank where
                                  // that is brighter than the pixel behind darkened (the outside shows through where it outshines it)
    int maskBank = -1;            // BLEND_REFLECT: only pixels of this bank are touched; intensity = mix of old and new
    double reflectMix = 0.5;
    bool ztest = true;
    bool zwrite = true;
    int blend = BLEND_REPLACE;
    double shadeMin = 0, shadeMax = 63;
    const FacePattern* pattern = nullptr;    // windows on the face (u and v in metres)
};

constexpr double NEAR_Z = 0.05;

// B-310: band-parallel drawing. Every fill of the calling thread (triangles, lines, points) is clipped to the rows
// [y0, y1); the drivers set a band per thread and reset it to the whole frame afterwards.
void setRasterBand(int y0, int y1);
void rasterTriangle(Framebuffer& fb, const RVert* v, const RasterParams& rp, const Proj& pj);
// Convenience: polygon (convex) fan.
void rasterPolygon(Framebuffer& fb, const RVert* v, int n, const RasterParams& rp, const Proj& pj);
// Lines and points take a bank index; thickness and size are in 1x pixels (scaled with the resolution).
void rasterLine3(Framebuffer& fb, RVert a, RVert b, int bank, const Proj& pj, bool ztest, int thickness = 1);
// Point with depth test; returns true if drawn.
bool rasterPoint3(Framebuffer& fb, const RVert& a, int bank, const Proj& pj, bool ztest, int size = 1);
// Projects a view-space point; returns false if behind the camera.
bool projectPoint(const RVert& a, const Proj& pj, double& sx, double& sy);

// I-01: a shape projected once and filled band by band, the same pixels as `rasterPolygon`'s and `rasterLine3`'s (their two halves).
// A convex polygon clipped at the near plane and projected into `out` (16 room): its vertex count, 0 when it fills nothing in the
// frame, and its rows' extremes; then its fill in the calling thread's band
int projectPolygon(const RVert* v, int n, const Proj& pj, ScreenVert* out, double& miny, double& maxy);
void fillPolygon(Framebuffer& fb, const ScreenVert* p, int m, double miny, double maxy, const RasterParams& rp, const Proj& pj);
// a line clipped and projected (its thickness in 1x pixels, scaled): false when it draws nothing; then its steps in the band
struct ProjectedLine { ScreenVert p, q; int steps = 1, thickness = 1; };
bool projectLine(const RVert& a, const RVert& b, const Proj& pj, int thickness, ProjectedLine& out);
void fillLine(Framebuffer& fb, const ProjectedLine& l, int bank, bool ztest);

// I-01: where a drawing's shapes go, in view space: rasterised at once (`RasterNow`), or kept in a `ShapeList`
struct ShapeSink {
    virtual void polygon(const RVert* q, int n, const RasterParams& rp) = 0;
    virtual void line(const RVert& a, const RVert& b, int bank, bool ztest, int thickness) = 0;
    virtual void point(const RVert& a, int bank, bool ztest, int size) = 0;
protected:
    ~ShapeSink() = default;
};
struct RasterNow final : ShapeSink {
    Framebuffer& fb; const Proj& pj;
    RasterNow(Framebuffer& f, const Proj& p) : fb(f), pj(p) {}
    void polygon(const RVert* q, int n, const RasterParams& rp) override { rasterPolygon(fb, q, n, rp, pj); }
    void line(const RVert& a, const RVert& b, int bank, bool ztest, int thickness) override { rasterLine3(fb, a, b, bank, pj, ztest, thickness); }
    void point(const RVert& a, int bank, bool ztest, int size) override { rasterPoint3(fb, a, bank, pj, ztest, size); }
};
// I-01: shapes projected once (what a band pass shares, made before the bands and only read by them), then filled by each band:
// every shape whose rows reach the calling thread's band (`setRasterBand`), in the order they came. A polygon keeps its params
// (the same as an earlier shape's shared). I-02: a clip (a box of the frame, the ship's room seen through its arch): the shapes
// that come while it is set are left out when they miss it, and fill only its rows (a row's pixels are its own: the same pixels
// in them as unclipped)
class ShapeList final : public ShapeSink {
public:
    void begin(const Proj& pj);   // emptied, for a frame projected by `pj`
    void clip(double x0, double y0, double x1, double y1);
    void noClip();
    void polygon(const RVert* q, int n, const RasterParams& rp) override;
    void line(const RVert& a, const RVert& b, int bank, bool ztest, int thickness) override;
    void point(const RVert& a, int bank, bool ztest, int size) override;
    void fill(Framebuffer& fb) const;
    int count() const { return (int)shapes.size(); }
private:
    enum { POLYGON, LINE, POINT };
    struct Shape { int kind = POLYGON, first = 0, n = 0, params = 0, bank = 0, size = 1; bool ztest = true; double miny = 0, maxy = 0; int y0 = -(1 << 30), y1 = 1 << 30; };
    bool clipped = false;
    double cx0 = 0, cy0 = 0, cx1 = 0, cy1 = 0;
    bool outside(double x0, double y0, double x1, double y1) const { return clipped && (x1 < cx0 - 1 || x0 > cx1 + 1 || y1 < cy0 - 1 || y0 > cy1 + 1); }
    void rows(Shape& s) const { if (clipped) { s.y0 = (int)std::floor(cy0) - 1; s.y1 = (int)std::ceil(cy1) + 2; } }
    Proj pj;
    std::vector<Shape> shapes;
    std::vector<ScreenVert> sv;
    std::vector<ProjectedLine> lines;
    std::vector<RVert> pts;
    std::vector<RasterParams> params;
};
