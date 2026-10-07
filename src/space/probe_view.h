// X-02 (2026-10-07): the probe's camera inside a giant, the descent's stages drawn from the decks' fields (galaxy/probe.h).
// Over the clouds the ammonia deck is a landscape to the curved horizon: its tops (the globe's bands lifting the zones and
// dropping the belts, cloud streets, billows, thunderheads casting their shadows, the rings' shadow) marched outward one
// azimuth at a time into a panorama of the frame (front to back, each sample filling the rows it rises over), lit by the
// sun with the terminator crossing it; the air over it is the sky's in-scatter, through which the space renderer's frame
// shows (the stars, the sun's disc, the moons) as much as the air above lets it, and the rings' arc. Inside a deck the
// picture is a fog marched along each ray (a coarser grid): its density follows the deck's top and base with layers and
// pockets, its light comes down through it (bright under the top, dim at the base, toward the sun's side), the lamp's
// beam and the lightning light it from within. Between the decks the floor (the water deck's towers, the lightning inside
// them, a channel through the clear air now and then) and the ceiling (the ammonia deck's underside in its pouches) are
// two panoramas meeting in the haze. The layers composite in RGB (the fog over a deck of another colour, the haze over
// both): every colour still comes from a palette ramp over the 64 shades, at the logical resolution, spread over the frame.
// X-03 (2026-10-07): one to three decks (each its cloud's colour), the clear-air holes (a deck's panorama shows the deck under
// it through them, their walls round the probe falling through one), the deep's glow from below (through the decks' tops, onto
// their undersides, in the fog and the clear air), the polar aurorae's curtains over the clouds, a carbon-rich giant's diamond
// hail in the lamp, and the rare giant's sight.
#pragma once
#include "core/framebuffer.h"
#include "core/raster.h"
#include "galaxy/probe.h"
#include <vector>
#include <cstdint>

// a colour over the 64 shades: a palette ramp read as RGB (0..1)
struct ShadeRamp {
    float c[64][3] = {};
    void set(const std::vector<std::pair<double, RGB>>& stops, float gain = 1);
    void material(const RGB& col, float gain = 1);   // the globe's material ramp (planetmap's `materialRamp`): a deck reads as the globe paints it
    inline void at(double s, float* o) const {
        if (s <= 0) { o[0] = c[0][0]; o[1] = c[0][1]; o[2] = c[0][2]; return; }
        if (s >= 63) { o[0] = c[63][0]; o[1] = c[63][1]; o[2] = c[63][2]; return; }
        int i = (int)s; float t = (float)(s - i);
        o[0] = c[i][0] + (c[i + 1][0] - c[i][0]) * t; o[1] = c[i][1] + (c[i + 1][1] - c[i][1]) * t; o[2] = c[i][2] + (c[i + 1][2] - c[i][2]) * t;
    }
};

struct ProbeView {
    Mat3 cam;                        // local (x east, y up, z north) -> view
    Proj pj;                         // physical pixels
    double zKm = 0;                  // the probe's altitude over the one-bar level
    double radiusKm = 70000;         // the giant's (the horizon's curve, the terminator across the decks)
    const GiantAtmosphere* atm = nullptr;   // the clear air's haze by altitude (galaxy/probe's giantHazeTau)
    const GiantDecks* decks = nullptr;
    double px[GIANT_MAX_DECKS] = {0, 0, 0}, pz[GIANT_MAX_DECKS] = {0, 0, 0};   // the probe's place in each deck's frame (km east, north of the aim: the decks drift with their own winds)
    // the light, relative to the camera's exposure
    Vec3 sun = Vec3(0, 1, 0);        // local, unit (under the horizon at night)
    double sunVis = 1;               // the sun's direct light on the probe (the sky's aureole)
    double direct[GIANT_MAX_DECKS] = {1, 0, 0};          // the sun's direct light on a deck's top
    double diffuse[GIANT_MAX_DECKS] = {0.35, 0.3, 0.3};  // the sky's light on a deck's top (the light coming down through what lies over it)
    double inside[GIANT_MAX_DECKS] = {1, 0.3, 0.3};      // the light in a deck under its top (the fog's, down to `under` at its base)
    double under[GIANT_MAX_DECKS] = {0.2, 0.02, 0.02};   // the light come through a deck onto its underside
    ShadeRamp air, deck[GIANT_MAX_DECKS], below[GIANT_MAX_DECKS], ring;   // the air (the sky, the haze, the clear band's light), the decks' tops (and their fog), their undersides, the rings
    // X-03: the deep's glow from below, against the exposure: come up through a deck's top, onto its underside, in the clear air
    // round the probe (scattered toward the camera, most from under it)
    double glowTop[GIANT_MAX_DECKS] = {0, 0, 0}, glowUnder[GIANT_MAX_DECKS] = {0, 0, 0}, glowAir = 0;
    RGB glowCol = RGB(0.78f, 0.24f, 0.08f);
    // X-03: the aurora's curtains: their strength (against the exposure, over the dark of the sky), the way toward the magnetic
    // pole (local, horizontal), the oval's distance along it (km), the curtains' lower border (km over one bar) and their
    // profile's depth (km), the lower curtain's and the tops' colours
    double aurora = 0, auroraP0 = 0, auroraH0 = 400, auroraH = 60; Vec3 auroraDir = Vec3(0, 0, 1);
    RGB auroraLow = RGB(0.85f, 0.25f, 0.65f), auroraHigh = RGB(0.45f, 0.2f, 0.85f);
    double hail = 0;                 // X-03: diamond hail glinting in the lamp (0..1)
    double sight = 0; Vec3 sightPos, sightAxis = Vec3(1, 0, 0);   // X-03: the rare giant's sight (0..1), its place and its long axis (local, km)
    double zenShade = 24, horShade = 54;      // the air's shades overhead and at the horizon
    // the rings (local, km from the probe)
    bool rings = false; Vec3 ringN = Vec3(0, 1, 0), ringC; double ringR0 = 0, ringR1 = 0, ringLit = 1; const std::vector<float>* ringProf = nullptr;
    // the cirrus: a layer of streaks along the wind
    double cirrusKm = 0, cirrus = 0, windAz = 0, cirrusRun = 0;   // its altitude, its strength, the wind's heading (from the north toward the east), the streaks' run along it (km)
    // the lightning, the lamp, the rain, the heat
    double flash = 0; Vec3 flashPos;          // a flash's strength now and its place (local, km from the probe)
    bool bolt = false; Vec3 boltTo; uint64_t boltSeed = 0;   // a channel through the clear air from the flash's tower
    double lamp = 0, rain = 0, motes = 0, heat = 0;   // the lamp (0..1), the rain rising past the lens, the motes in the deep, the deep's heat glow
    RGB lampCol = RGB(1.0f, 0.86f, 0.62f), flashCol = RGB(0.75f, 0.82f, 1.0f), heatCol = RGB(0.5f, 0.1f, 0.02f);
    double time = 0;
    uint64_t seed = 0;
};
// the picture into `out` (FBW x FBH): the layers over `space` (the space renderer's frame in RGB: what lies beyond the air;
// null when it does not show)
void renderProbeView(const ProbeView& v, const uint32_t* space, uint32_t* out);
// the most of the space beyond the air the frame can show (0: the caller skips the space renderer)
double probeSpaceShows(const ProbeView& v);
// the last frame's layers and costs, for the harness
struct ProbeViewStats {
    int floor = -1, ceil = -1, fog = -1;      // the decks drawn as the floor, the ceiling, the fog (-1 none)
    bool inHole = false;                      // X-03: the probe in a clear-air hole (the floor's walls round it)
    double aurora = 0;                        // X-03: the aurora's mean light over the frame
    double covFloor = 0, covCeil = 0, fogT = 1;   // the frame's share the floor and the ceiling fill, the fog's mean transmittance
    double shadeFloor = 0, shadeCeil = 0;     // their mean shades
    int columns = 0, rows = 0;                // a panorama's size
    double sunElDeg = 0, direct0 = 0, diffuse0 = 0, inside0 = 0, alb0 = 0;   // the inputs that set its light
    double flash = 0; Vec3 flashPos; double flashSx = -1, flashSy = -1;      // the lightning and where it lands in the frame
    double msPano = 0, msFog = 0, msNodes = 0, msSpread = 0;
};
const ProbeViewStats& probeViewStats();
