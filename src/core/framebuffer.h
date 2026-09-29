// Software framebuffer in the spirit of the VGA mode 13h target of the original
// game: an indexed image smoothed by the "mush" filter and converted through a
// palette of intensity ramps. Since M10-03 a pixel is 16 bits; since O0-02
// (2026-09-27) the top 5 bits select one of 32 banks (a "material" ramp) and
// the low 11 bits are its intensity (32 units per classic shade step, so the
// classic range 0..63 is 0..2047). 16 banks were not enough for two worlds with
// discs in one frame: they shared the forest/sand/snow/grass banks and the
// smaller one borrowed the bigger one's colours (B-305).
// The mush averages only intensities: colours never bleed across banks but
// edges melt, which is the soft, smeared, low-fi look of Noctis. The resolution
// is 320x200 times a runtime scale (M10-01); the mush box grows with the scale.
#pragma once
#include "types.h"
#include <cstdint>
#include <vector>
#include <initializer_list>
#include <utility>

typedef uint16_t Pix;
constexpr int BANKS = 32;
constexpr int BANK_SHIFT = 11;
constexpr int INTEN_MASK = 2047;
constexpr Pix BANK_MASK = 0xF800;
constexpr int INTEN_PER_SHADE = 32;          // 2048 / 64 classic shades
constexpr int UW = 320, UH = 200;            // logical (1x) size used by the HUD layout

extern int FBW, FBH;        // current framebuffer size (UW * scale, UH * scale)
extern int FB_SCALE;        // 1..4
extern int g_mushKernel;    // box size of the mush filter (scale + 1 by default)
// Set the resolution scale; every Framebuffer must be re-created afterwards.
void setFramebufferScale(int scale);

inline int bankOf(Pix p) { return p >> BANK_SHIFT; }
inline int intenOf(Pix p) { return p & INTEN_MASK; }
inline double shadeOf(Pix p) { return (p & INTEN_MASK) * (1.0 / INTEN_PER_SHADE); }
inline Pix pixI(int bank, int inten) {
    if (inten < 0) inten = 0; else if (inten > INTEN_MASK) inten = INTEN_MASK;
    return (Pix)((bank << BANK_SHIFT) | inten);
}
// Classic shade units 0..63 (fractions kept).
inline Pix pix(int bank, double shade) { return pixI(bank, (int)(shade * INTEN_PER_SHADE + 0.5)); }
inline int toInten(double shade) { return (int)(shade * INTEN_PER_SHADE + 0.5); }

struct Framebuffer {
    std::vector<Pix> idx;       // bank | intensity
    std::vector<float> invz;    // 1/z per pixel, 0 = infinitely far
    uint8_t pal[BANKS * 64 * 3]; // 64 colour stops per bank, 0..255 per channel

    Framebuffer();
    void clear(Pix v = 0);
    void clearDepth();
    inline Pix& at(int x, int y) { return idx[y * FBW + x]; }
    inline Pix at(int x, int y) const { return idx[y * FBW + x]; }
    // Noctis' psmooth_64 generalised: k x k box average of the intensities starting one row
    // below, bank kept from the pixel one row below.
    void mush(int passes);
    // Additive glow of the intensity inside a disc (used for suns/flares). maxAdd in shade units.
    void glowDisc(double cx, double cy, double radius, double coreRadius, int maxAdd, int bankIfEmpty, bool skipOccluded = false);
    // Convert to 0xAABBGGRR (little endian R,G,B,A) with brightness gain; the 64 stops of
    // each bank are interpolated to 256 steps.
    void toRGB(uint32_t* out, double gain, bool dither = false) const;   // dither: 2x2 ordered dither on the intensity (M6-01)
};

// Define the 64 stops of a bank as a piecewise-linear ramp through colour stops
// (position 0..63, colour 0..1).
void setRamp(uint8_t* pal, int bank, std::initializer_list<std::pair<double, RGB>> stops);
void setRampFromVector(uint8_t* pal, int bank, const std::vector<std::pair<double, RGB>>& stops);
// Multiply every palette entry (fade to black).
void scalePalette(uint8_t* pal, double f);
