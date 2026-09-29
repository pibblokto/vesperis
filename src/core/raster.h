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

struct RasterParams {
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
    bool cutout = false;          // B-315: pixels whose wobbled weight falls under a half are not drawn at all (ragged leaf clusters); the edge noise and the vertex weights as for two materials
    double quantize = 0;          // >0: snap the interpolated shade to this step (quantised gradient mode)
    double zbias = 1.0;           // B-313: the depth tested and written is iz x zbias; under 1 pushes a surface back a little (a coarser terrain ring loses to the finer one where they coincide)
    double darken = 1.0;          // BLEND_DARKEN: multiply the existing intensity (blob shadows)
    int maskBank = -1;            // BLEND_REFLECT: only pixels of this bank are touched; intensity = mix of old and new
    double reflectMix = 0.5;
    bool ztest = true;
    bool zwrite = true;
    int blend = BLEND_REPLACE;
    double shadeMin = 0, shadeMax = 63;
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
// 2D helpers on the index buffer (no depth).
void fillRect(Framebuffer& fb, int x0, int y0, int x1, int y1, Pix value);
void fillDisc(Framebuffer& fb, double cx, double cy, double r, Pix value);
void drawLine2D(Framebuffer& fb, int x0, int y0, int x1, int y1, Pix value);
