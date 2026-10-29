// The giant probe (game/probe.cpp, probe_feed.cpp, probe_records.cpp): the probe falling and its tracks, and a recording's replay
#pragma once
#include "core/types.h"
#include "galaxy/probe.h"
#include "galaxy/starfield.h"
#include "galaxy/system.h"
#include <cstdint>
#include <string>
#include <vector>

struct ProbeState {
    bool active = false;                 // launched and not let go (falling, or its last frame held)
    int body = -1; uint64_t starSeed = 0;
    double aimLat = 0, aimLon = 0;       // the entry point (radians, the body's frame)
    double clock = 0;                    // real seconds since the launch (the relay's clock)
    double launchT = 0;                  // the game time at the launch
    Vec3 startRel;                       // the ship's place at the launch from the giant's centre (km)
    ProbeFlight flight;                  // the chute's clocks, the hull's limit
    GiantAtmosphere atm;
    uint64_t seed = 0;
    double headYaw = 0, descYaw = 0;     // local azimuths: the heading it comes in along (the fall's camera), the descent's camera (the sun's quarter)
    double camYaw = 0, camPitch = 0;     // the camera's turn from its default (the mouse, the arrows)
    double endClock = -1, endT = 0; int endCause = 0;   // the end: its clock, its game time, 1 the hull, 2 the heat
    double endBar = 0, endKm = 0, endK = 0;
    std::vector<uint32_t> lastFrame;     // the frame held at the end
    int lastFlashSlot = -1;              // the lightning's last slot heard (the thunder's trigger)
    bool tornNoted = false;              // the chute's tearing announced
    bool fast = false;                   // R-406: Shift held on the probe's screen this frame (set in Game::frame before the clock runs): eight times as fast
    GiantBands bands;                    // the globe's bands round the aim at the launch (built at the launch and after a load)
    GiantDecks decks;                    // the decks' fields (its bands pointer refreshed before every use)
    bool holePath = false;               // this descent falls through a clear-air hole (the decks' `path` holes over its way)
    double sightClock = -1;              // on the rare giant, the clock the sight passes the lamp at (-1 none)
    // what comes back (the record, galaxy-side numbers above, is kept in the guide at the end or the cut)
    int id = 0;                          // its number (PROBE 3)
    std::vector<double> camTrack;        // the explorer's turns of its camera where they changed (clock, yaw, pitch)
    std::vector<double> timeTrack;       // the game's time along the fall where its pace changed (clock, t)
    double prevClock = -1, prevT = 0, prevPace = -1;            // the clock's track: the last frame's, the pace of the segment from the last key (-1 not yet)
    double prevCamClock = 0, prevYaw = 0, prevPitch = 0;        // the camera's: the last frame's
    double prevHoleClock = 0;                                   // the clock the hole was looked for up to
    unsigned notes = 0;                  // the discoveries logged (bits: the storm, its wall, the aurora, a hole, hail)
    int stormKind = 0; std::string stormKey;   // the storm under the aim (1 one of the globe's, 2 the great storm) and its key
};

struct ReplayState {
    int rec = -1;                        // the record's index in guide.probes
    ProbeState p;
    bool here = true;                    // its giant is of the system the ship is in (else `sys` and `nb` below)
    StarSystem sys; StarNeighborhood nb;
    double clock = 0; bool playing = true, fast = false;   // fast: Shift held, eight times as fast
    double lookYaw = 0, lookPitch = 0;   // the viewer's own turn over the recording's
    std::vector<uint32_t> last;          // its last frame (drawn once)
};

// the lightning in the probe's picture (probe_feed.cpp, probe_records.cpp; the harness reads the flashes too)
const double FLASH_SLOT = 0.5;                        // the lightning's slots (seconds of the relay's clock)
// a flash in a lightning slot: its strength now (0 none), its slot, and (given the decks) where it is (local, km from the
// probe) and whether a channel crosses the clear air from it. X-03: lightning lives in the convective decks (water, the salts):
// a glow under the clouds seen from over them, close by in the fog of a deck, in the towers of a convective deck under the
// clear band (a channel up from one in three; under a quiet deck a glow), over the deep; as often as the giant's rate
struct Flash { double strength = 0; int slot = -1; Vec3 pos; bool bolt = false; Vec3 boltTo; uint64_t seed = 0; };
inline Flash flashAt(const GiantAtmosphere& a, const ProbeFlight& f, uint64_t seed, double clock, const GiantDecks* D, double yaw, double zp, const double* px, const double* pz) {
    Flash F;
    if (clock < probeDescentStart()) return F;
    const double bar = probeBarAt(f, clock);
    const int st = giantStageAt(a, bar), kc = giantStormDeckFrom(a, bar), kin = giantDeckAt(a, bar), kb = giantDeckAbove(a, bar) + 1;
    double odds = 0;
    switch (st) {
        case PS_ABOVE: odds = kc >= 0 ? 0.02 : 0; break;
        case PS_HAZE: odds = kc >= 0 ? 0.03 : 0; break;
        case PS_DECK1: case PS_DECK2: odds = kin == kc ? 0.1 : 0.03; break;
        case PS_BETWEEN: odds = kc == kb ? 0.16 : (kc >= 0 ? 0.05 : 0.01); break;
        case PS_DEEP: odds = 0.05; break;
        default: break;
    }
    odds *= a.ch.lightning;
    int slot = (int)std::floor(clock / FLASH_SLOT);
    uint64_t h = mix64(seed ^ ((uint64_t)slot * 0x9E3779B97F4A7C15ULL) ^ 0xF1A5ULL);
    if (unitFromHash(h) >= odds) return F;
    double t0 = slot * FLASH_SLOT + unitFromHash(mix64(h ^ 1)) * FLASH_SLOT * 0.5, age = clock - t0;
    if (age < 0 || age > 0.45) return F;
    F.slot = slot; F.seed = h;
    double flick = 0.6 + 0.4 * std::cos(age * 60) * std::cos(age * 23);   // the strokes of one flash
    F.strength = (0.5 + 0.5 * unitFromHash(mix64(h ^ 4))) * std::exp(-age * 7) * flick;
    if (!D) return F;
    double u = unitFromHash(mix64(h ^ 2)), w = unitFromHash(mix64(h ^ 3));
    const bool betweenOver = st == PS_BETWEEN && kb < D->n;
    double az = yaw + (u - 0.5) * 2 * (betweenOver ? 25 : 55) * DEG, r, y;
    switch (st) {
        case PS_ABOVE: case PS_HAZE: r = 40 + 160 * w; break;
        case PS_DECK1: case PS_DECK2: r = 1.0 + 4 * w; break;
        case PS_BETWEEN: r = (35 + 60 * w) * D->sh; break;   // where the camera sees the floor
        default: r = 4 + 20 * w; break;
    }
    const double x = std::sin(az) * r, z = std::cos(az) * r;
    switch (st) {
        case PS_ABOVE: case PS_HAZE: y = D->top(0, x + px[0], z + pz[0], 1.0) - 3 * D->sh; break;
        case PS_DECK1: case PS_DECK2: y = zp + (unitFromHash(mix64(h ^ 5)) - 0.5) * 4 * D->sh; break;
        case PS_BETWEEN: y = betweenOver ? D->top(kb, x + px[kb], z + pz[kb], 1.0) - (kb == kc ? 2 : 3) * D->sh : zp - 20 * D->sh; break;
        default: y = D->lvBase[D->n - 1] + 1.5 * D->sh; break;
    }
    F.pos = Vec3(x, y - zp, z);
    if (betweenOver && kb == kc && kb > 0 && unitFromHash(mix64(h ^ 6)) < 0.35) {
        double ba = az + (unitFromHash(mix64(h ^ 7)) - 0.5) * 2.0, run = (3 + 9 * unitFromHash(mix64(h ^ 8))) * D->sh;
        F.bolt = true;
        F.boltTo = F.pos + Vec3(std::sin(ba) * run, (D->lvBase[kb - 1] - y) * (0.3 + 0.5 * unitFromHash(mix64(h ^ 9))), std::cos(ba) * run);
    }
    return F;
}
