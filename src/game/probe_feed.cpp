// The probe's camera (X-02, X-03): the decks round the aim as fields, the sky's in-scatter, the rings' arc, the lightning
#include "probe_internal.h"
#include "ui.h"
#include "core/rng.h"
#include "space/probe_view.h"
#include "core/parallel.h"
#include "core/noise.h"
#include "core/png.h"
#include "core/fs.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

namespace {
const double HANDOVER0 = 0.62, HANDOVER1 = 0.92;       // the entry's share over which the probe's own picture takes over from the space renderer's (the plasma at its height)
const double BANDS_KM = 4800;           // X-02: the bands' reach round the aim (the horizon from the entry's end)
const int BANDS_N = 160;
}

// the probe's place: in the fall from the ship's place at the launch to the entry over the aim point (the height falling by
// equal ratios, so the giant grows evenly, and the direction swinging from the start's to the aim's), then over the aim point
// at the height of its pressure
Vec3 Game::probeWorldPos(double c, double tt, Vec3& up) const {
    const Body& g = sys.bodies[probe.body];
    Vec3 gp = sys.bodyPos(probe.body, tt);
    Vec3 aimW = normalize(sys.bodyFrame(probe.body, tt).transposed() * StarSystem::bodyFromLatLon(probe.aimLat, probe.aimLon));
    double R = g.radiusKm;
    if (c < PROBE_FALL_S) {
        double u = clampd(c / PROBE_FALL_S, 0, 1);
        double h0 = std::max(length(probe.startRel) - R, 2000.0), h1 = giantAltitudeKm(probe.atm, PROBE_ENTRY_BAR);
        double h = h0 * std::pow(h1 / h0, u);
        up = normalize(lerp(normalize(probe.startRel), aimW, smoothstep(0.0, 1.0, u)));
        return gp + up * (R + h);
    }
    up = aimW;
    return gp + up * (R + giantAltitudeKm(probe.atm, probeBarAt(probe.flight, c)));
}

// X-02: the bands of a probe launched or loaded (the globe's round the aim at the launch), the decks' fields over them. X-03: on
// a giant with clear-air holes the descent falls through one about half the time: a hole in each deck over the last, where the
// probe crosses that deck's top (falling free); on the rare giant, the clock the sight passes the lamp at
void Game::probeEnsureClouds() {
    if (!probe.active || !sys.valid || probe.body < 0 || probe.body >= (int)sys.bodies.size()) return;
    if (!probe.bands.valid()) {
        const Body& g = sys.bodies[probe.body];
        probe.bands = giantBandsOf(g, spaceR.genFor(g), probe.aimLat, probe.aimLon, probe.launchT, BANDS_KM, BANDS_N);
    }
    probe.decks.init(probe.atm, &probe.bands, probe.seed);
    probe.sightClock = -1;
    if (probe.atm.ch.sight) {   // the rare giant: once in the deep (where the sun's light is gone and the lamp is on), at a depth of the probe's own
        const GiantAtmosphere& a = probe.atm;
        const double endBar = probeEndBar(a, probe.flight);
        double dark = endBar;
        for (double l = std::log(a.top[0]); l < std::log(endBar); l += 0.02) if (giantSunlightAt(a, std::exp(l)) < 0.02) { dark = std::exp(l); break; }
        const double lo = std::log(std::max(dark, a.base[0] * 1.3)), hi = std::log(endBar * 0.85);
        if (hi > lo) probe.sightClock = clockAtBar(probe.flight, std::exp(lo + (hi - lo) * (0.3 + 0.6 * unitFromHash(mix64(probe.seed ^ 0x5167ULL)))));   // its clock follows the chute
    }
    probe.holePath = probe.atm.ch.holes > 0 && probe.decks.holeable(0) && unitFromHash(mix64(probe.seed ^ 0x4011EULL)) < 0.55;
    if (probe.holePath) {   // through the first deck, and on through the next while each opens onto a deck close under it
        ProbeFlight free = probe.flight; free.chuteOpen = free.chuteClose = -1;
        const double r = (22 + 30 * unitFromHash(mix64(probe.seed ^ 0x4012EULL))) * probe.decks.sh;
        for (int k = 0; probe.decks.holeable(k); k++) {
            GiantDecks::Hole& h = probe.decks.path[k];
            h.on = true; h.r = r; h.cz = 0;
            h.cx = -probeDeckDrift(k, clockAtBar(free, probe.atm.top[k]));
        }
    }
}

// X-02: km a deck has slid east under the probe since the descent began
// km a deck has slid east under the probe since the descent began: the shear between the deck's wind (at its middle) and the
// probe's at each moment, summed over the descent
double Game::probeDeckDrift(int k, double c) const {
    const double c0 = probeDescentStart();
    if (c <= c0) return 0;
    const GiantAtmosphere& a = probe.atm;
    double level = k < GIANT_MAX_DECKS ? std::sqrt(a.top[k] * a.base[k]) : 0.3;
    double wDeck = giantWindEast(a, probe.aimLat, level), sum = 0;
    const int n = 48;
    for (int i = 0; i < n; i++) sum += wDeck - giantWindEast(a, probe.aimLat, probeBarAt(probe.flight, c0 + (c - c0) * (i + 0.5) / n));
    return sum * (c - c0) / n * 0.001;
}

// X-03: the great storm's wall within sight of the aim (2,500 km of its edge): the way to its nearest stretch (local, east and
// north: away from the centre inside it, toward it outside) and the edge's distance (km, negative inside)
bool Game::probeStormWall(double& dist, double& ex, double& ez) const {
    const GiantBands& B = probe.bands;
    if (!B.gsOn) return false;
    const double d = B.greatStormAt(0, 0), edge = (d - 1) * std::sqrt(B.gsAKm * B.gsBKm), l = std::hypot(B.gsX, B.gsZ);
    if (std::fabs(edge) > 2500 || l < 1e-6) return false;
    ex = B.gsX / l * (d < 1 ? -1 : 1); ez = B.gsZ / l * (d < 1 ? -1 : 1); dist = edge;
    return true;
}

// X-03: the aurora's way from the aim: the local horizontal toward the nearer magnetic pole and the oval's distance along it (km;
// negative with the aim inside the oval, between it and the pole)
bool Game::probeAuroraWay(double& ex, double& ez, double& p0) const {
    const GiantCharacter& ch = probe.atm.ch;
    if (ch.aurora <= 0 || !sys.valid || probe.body < 0 || probe.body >= (int)sys.bodies.size()) return false;
    const Vec3 aimB = StarSystem::bodyFromLatLon(probe.aimLat, probe.aimLon);
    Vec3 pole = StarSystem::bodyFromLatLon(PI / 2 - ch.magColat, ch.magLon);
    if (dot(aimB, pole) < 0) pole = pole * -1.0;
    const double th = std::acos(clampd(dot(aimB, pole), -1, 1));
    Vec3 E = cross(Vec3(0, 0, 1), aimB);
    E = length(E) < 1e-9 ? Vec3(0, 1, 0) : normalize(E);
    const Vec3 N = cross(aimB, E), tw = pole - aimB * dot(aimB, pole);
    const double l = length(tw);
    ex = l > 1e-9 ? dot(tw, E) / l : 0; ez = l > 1e-9 ? dot(tw, N) / l : 1;
    p0 = (th - ch.ovalRad) * sys.bodies[probe.body].radiusKm;
    return true;
}

// the probe's picture at a clock and a game time, into rgbBuf: the space renderer from the probe's place through the fall and
// the entry, handing over to the probe's own picture (space/probe_view.h) as the plasma peaks; the plasma over both; the
// link's static over all of it as the signal weakens
void Game::renderProbeFeed(double c, double tt) {
    const Body& g = sys.bodies[probe.body];
    const GiantAtmosphere& a = probe.atm;
    probeEnsureClouds();
    Vec3 up; Vec3 pos = probeWorldPos(c, tt, up);
    Vec3 E, N; localFrame(up, sys.bodyFrame(probe.body, tt).row(2), E, N);
    Mat3 W = Mat3::fromRows(E, up, N);   // world -> local
    double bar = probeBarAt(probe.flight, c);
    int stage = probeStageAt(a, probe.flight, std::min(c, probeEndClock(a, probe.flight) - 1e-6));
    // the camera: in the fall toward the aim point (the heading held as it comes under the probe), easing in the entry to the
    // heading fourteen degrees under the horizon; the player's turn on top, the chute's swing, the entry's shake
    double yaw = probe.descYaw, pitch = -22 * DEG;   // X-02: high over the clouds the deck fills the frame under a strip of sky
    if (c >= probeDescentStart()) {
        const GiantAtmosphere& A = probe.atm;
        double lb = std::log(std::max(probeBarAt(probe.flight, c), 1e-6));
        pitch = (-22 + 8 * smoothstep(std::log(0.02), std::log(A.hazeBar), lb) + 14 * smoothstep(std::log(A.top[0]), std::log(std::sqrt(A.top[0] * A.base[0])), lb) - 2 * smoothstep(std::log(A.base[0]), std::log(A.base[0] * 1.3), lb)
                 + (A.decks > 1 ? 1 * smoothstep(std::log(A.top[1] * 0.8), std::log(A.top[1] * 1.2), lb) : 0.0)) * DEG;
        if (probe.holePath)   // falling into a clear-air hole the camera looks down into it, and down the shaft once in it
            pitch -= (30 + 26 * smoothstep(std::log(A.top[0]), std::log(std::sqrt(A.top[0] * A.base[0])), lb)) * DEG * smoothstep(std::log(A.hazeBar * 0.25), std::log(A.hazeBar), lb) *
                     (1 - smoothstep(std::log(A.top[A.decks - 1] * 0.7), std::log(A.top[A.decks - 1]), lb));
    }
    if (c < probeDescentStart()) {
        Vec3 gp = sys.bodyPos(probe.body, tt);
        Vec3 aimPt = gp + normalize(sys.bodyFrame(probe.body, tt).transposed() * StarSystem::bodyFromLatLon(probe.aimLat, probe.aimLon)) * g.radiusKm;
        double u = clampd(c / PROBE_FALL_S, 0, 1);
        Vec3 to = W * normalize(lerp(normalize(gp - pos), normalize(aimPt - pos), smoothstep(0.0, 0.6, u)));
        double h = std::max(1.0, length(pos - gp) - g.radiusKm), dip = std::acos(g.radiusKm / (g.radiusKm + h));   // the limb's angle under the horizontal
        double fp = clampd(std::asin(clampd(to.y, -1, 1)), -(dip + 12 * DEG), 20 * DEG);   // never steeper than a little under the limb: the whole giant at first, then its curve and its sky in the frame as it rushes up
        double fy = std::atan2(to.x, to.z);
        double hold = smoothstep(-20 * DEG, -60 * DEG, fp);
        double fallYaw = probe.headYaw + wrapAngle(fy - probe.headYaw) * (1 - hold);
        double e = c < PROBE_FALL_S ? 0 : smoothstep(0.25, 0.9, (c - PROBE_FALL_S) / PROBE_ENTRY_S);
        yaw = fallYaw + wrapAngle(probe.descYaw - fallYaw) * e;
        pitch = fp + (-22 * DEG - fp) * e;
    }
    if (probe.flight.chuteOpen >= 0 && c >= probe.flight.chuteOpen && c < probe.flight.chuteClose) {   // the swing under the canopy
        yaw += 3 * DEG * std::sin(c * 0.37); pitch += 2 * DEG * std::sin(c * 0.9);
    }
    double shake = 0;
    if (c >= PROBE_FALL_S && c < probeDescentStart()) shake = std::sin(PI * (c - PROBE_FALL_S) / PROBE_ENTRY_S);
    else if (c >= probeDescentStart()) shake = 0.15 * smoothstep(0.3, 1.0, bar) * (probe.flight.chuteOpen >= 0 && c < probe.flight.chuteClose ? 0.5 : 1.0);
    if (shake > 0 && !harness.probeQuiet) { yaw += shake * 0.6 * DEG * std::sin(c * 37.0) ; pitch += shake * 0.8 * DEG * std::sin(c * 53.0 + 1.3); }
    yaw += probe.camYaw; pitch = clampd(pitch + probe.camPitch, -88 * DEG, 88 * DEG);
    Mat3 camLocal = cameraBasis(yaw, pitch);
    Mat3 camW = camLocal * W;
    Proj pj = Proj::fromHFov(settings.fovDeg);
    const double mixIn = c < PROBE_FALL_S ? 0.0 : (c < probeDescentStart() ? smoothstep(HANDOVER0, HANDOVER1, (c - PROBE_FALL_S) / PROBE_ENTRY_S) : 1.0);
    static std::vector<uint32_t> spacePic, skyPic;
    if (mixIn < 1) {   // the fall and the entry: the space renderer from the probe's place, the giant and all
        spaceR.proj = pj;
        int other = -1; double best = 0;
        for (int i = 0; i < (int)sys.bodies.size(); i++) if (i != probe.body) { double ang = sys.bodies[i].radiusKm / std::max(1.0, length(sys.bodyPos(i, tt) - pos)); if (ang > best) { best = ang; other = i; } }
        if (best < 1.2 / pj.f) other = -1;
        spaceR.setupPalette(fb, &sys, probe.body, other, 1);
        SpaceContext sc;
        sc.sys = &sys; sc.stars = &nb.stars; sc.t = tt; sc.shipPos = pos; sc.cam = camW;
        sc.bankBodyA = probe.body; sc.bankBodyB = other; sc.detailBody = -1;
        spaceR.render(fb, sc);
        fb.mush(2);
        fb.toRGB(rgbBuf.data(), 1.0, settings.dither);
        if (mixIn > 0) spacePic = rgbBuf;
    }
    if (mixIn > 0) {   // the probe's own picture
        const double zp = giantAltitudeKm(a, std::max(bar, 1e-9));
        const Vec3 sunW = normalize(sys.star.pos - pos), sunL = W * sunW;
        const double lf = sys.sunless() ? 0.0 : SpaceRenderer::lightFactor(sys.star.luminosity, length(sys.star.pos - pos));   // a drifter's giant has no sun (the floor is a sun's)
        const BodyGen& gg = spaceR.genFor(g);
        const GiantCharacter& ch = a.ch;
        const int nd = a.decks, last = nd - 1;
        const bool ice = ch.kind == GK_ICE;
        const RGB white(1, 1, 1), lightCol = lerp(sys.star.color, white, 0.5f);
        const RGB band = gg.matColor[familyRep(g.type, FAM_ROCK)] * lightCol;   // the globe's colour under the star's light
        auto lpOf = [](double b) { return std::log(std::max(b, 1e-9)); };
        const double lp = lpOf(bar);
        double px[GIANT_MAX_DECKS], pz[GIANT_MAX_DECKS] = {0, 0, 0};
        for (int k = 0; k < GIANT_MAX_DECKS; k++) px[k] = -probeDeckDrift(k, c);   // the probe's place over each deck (km east of the aim in its frame)
        unsigned cut = 0;   // the decks a clear-air hole takes away over the probe
        for (int k = 0; k < last; k++) if (bar > a.top[k] && probe.decks.holeAt(k, px[k], pz[k]) >= 0.5) cut |= 1u << k;
        const double Lp = giantSunlightAt(a, std::max(bar, 1e-9), cut);
        // the camera's gain takes back most of the dark (the light on the decks is given against it); what it cannot take
        // back dims the colours
        // over the clouds it exposes for the deck under it, in a deck for the fog round it, in the clear band for the ceiling
        // (the light come through it), easing into the next deck's own over the band's last stretch; X-03: the deep's glow
        // counts with the sun's (a brown dwarf's clouds are lit from under them as much as from over them)
        const int kAbove = giantDeckAbove(a, bar), kIn = giantDeckAt(a, bar);
        const bool airGlobe = ch.kind == GK_AZURE || ch.kind == GK_ALKALI;   // its globe is its air's (no deck seen from orbit: it exposes for the air round it)
        double Lref = bar < a.top[0] && !airGlobe ? giantSunlightAt(a, a.top[0], cut) : Lp;
        if (kIn < 0 && kAbove >= 0 && kAbove < last)
            Lref = std::exp(lerpd(std::log(giantSunlightAt(a, a.base[kAbove], cut)), std::log(Lp), smoothstep(lpOf(a.top[kAbove + 1] * 0.7), lpOf(a.top[kAbove + 1]), lp)));
        const double glowRef = bar < a.top[0] ? giantGlowAt(a, a.top[0]) : 0.6 * giantGlowAt(a, bar, cut);
        const double expo = clampd(std::pow(std::max(Lref + glowRef, 1e-6), bar >= a.base[0] ? -0.95 : -0.85), 0.25, 12.0);
        double albAim = 0.55, stormAim = 0;
        probe.bands.at(px[0], 0, albAim, stormAim);
        const double bandGain = std::pow(0.75 / std::max(albAim, 0.2), 0.42);   // and takes back some of a dark belt's dark (the band's colour stays)
        // a made giant's light by its class (the tints down the decks, the deep's colours, whether it changes from high in the air and toward
        // the blue): a water cloud's white, an azure giant's blue from high in its clear air (Rayleigh's), an alkali giant's blue and dim (its
        // sodium takes the yellow, its potassium the red), a silicate giant's orange from the glow under it, a hazy giant's pale amber
        struct MadeLight { RGB t1, t2, dz, dh, warmK; bool high; double gainExp; };   // warmK: the haze's shift (a plain giant's toward the amber)
        static const MadeLight MADE[6] = {
            {RGB(0.92f, 0.94f, 1.0f), RGB(0.78f, 0.82f, 0.9f), RGB(0.30f, 0.32f, 0.36f), RGB(0.40f, 0.42f, 0.46f), RGB(0.94f, 0.96f, 1.0f), false, 0.08},
            {RGB(0.62f, 0.8f, 1.0f), RGB(0.34f, 0.5f, 0.95f), RGB(0.10f, 0.18f, 0.42f), RGB(0.16f, 0.26f, 0.55f), RGB(0.45f, 0.85f, 1.08f), true, 0.05},
            {RGB(0.55f, 0.58f, 0.92f), RGB(0.38f, 0.30f, 0.72f), RGB(0.22f, 0.05f, 0.04f), RGB(0.34f, 0.08f, 0.05f), RGB(0.5f, 0.6f, 1.0f), true, 0.35},
            {RGB(1.0f, 0.72f, 0.48f), RGB(1.0f, 0.5f, 0.22f), RGB(0.55f, 0.2f, 0.06f), RGB(0.75f, 0.32f, 0.1f), RGB(1.0f, 0.7f, 0.45f), false, 0.1},
            {RGB(0.95f, 0.92f, 0.84f), RGB(0.8f, 0.74f, 0.62f), RGB(0.36f, 0.34f, 0.31f), RGB(0.46f, 0.44f, 0.40f), RGB(1.0f, 0.9f, 0.76f), true, 0.1},
            {RGB(1.0f, 0.86f, 0.66f), RGB(0.92f, 0.66f, 0.42f), RGB(0.40f, 0.30f, 0.22f), RGB(0.50f, 0.40f, 0.30f), RGB(1.0f, 0.8f, 0.6f), true, 0.1}};
        const MadeLight* ml = ch.kind >= GK_WATER ? &MADE[ch.kind - GK_WATER] : nullptr;
        const bool high = ice || (ml && ml->high);
        const float colGain = (float)(clampd(std::pow(std::max(Lp + giantGlowAt(a, bar, cut), 1e-6), ice ? 0.06 : (ml ? ml->gainExp : 0.1)), 0.45, 1.0) * clampd(lf, 0.6, 1.1));   // (an ice giant's blue light carries deeper)
        // the light come down to a pressure, reddened through the decks (an ice giant's: the methane takes the red, the light goes
        // blue-green)
        const RGB tint1 = ml ? ml->t1 : (ice ? RGB(0.66f, 0.9f, 1.0f) : RGB(1.0f, 0.76f, 0.56f)), tint2 = ml ? ml->t2 : (ice ? RGB(0.36f, 0.72f, 0.95f) : RGB(0.95f, 0.56f, 0.38f));
        auto tint = [&](double b) {   // (an ice giant's methane blues the light from high in the air)
            RGB t = lerp(white, tint1, (float)smoothstep(lpOf(high ? 0.005 : a.top[0]), lpOf(a.base[0]), lpOf(b)));
            return lerp(t, tint2, (float)smoothstep(lpOf(a.base[0]), lpOf(a.base[last]), lpOf(b)));
        };
        auto warm = [&](RGB c0, double w) {
            if (ml) return lerp(c0, RGB(c0.r * ml->warmK.r, c0.g * ml->warmK.g, std::min(1.0f, c0.b * ml->warmK.b)), (float)w);
            return ice ? lerp(c0, RGB(c0.r * 0.45f, c0.g * 0.85f, std::min(1.0f, c0.b * 1.08f)), (float)w) : lerp(c0, RGB(c0.r, c0.g * 0.72f, c0.b * 0.5f), (float)w);
        };
        // a deck's colour: the first is the globe's (what was seen from orbit), the ones under it their clouds' own in its light (P-07: an azure or
        // an alkali giant's first too: its globe is its air's)
        auto deckCol = [&](int k) { return k == 0 && !airGlobe ? band : lerp(band, CLOUD_SPECIES[a.species[k]].color * lightCol, k == 0 ? 0.85f : 0.55f); };
        ProbeView v;
        v.cam = camLocal; v.pj = pj; v.zKm = zp; v.radiusKm = g.radiusKm; v.atm = &probe.atm; v.decks = &probe.decks;
        for (int k = 0; k < GIANT_MAX_DECKS; k++) { v.px[k] = px[k]; v.pz[k] = pz[k]; }
        v.sun = sunL;
        v.sunVis = std::sqrt(Lp) * (stage <= PS_HAZE || (cut & 1u) ? 1.0 : 0.0);
        for (int k = 0; k < nd; k++) {   // the light on the decks against the exposure: the sun's own on the first's top only, the light coming down through them
            const double top = giantSunlightAt(a, a.top[k], cut), base = giantSunlightAt(a, a.base[k], cut);
            v.direct[k] = k ? 0.0 : 1.2 * top * expo * bandGain;
            v.diffuse[k] = (k ? 1.0 : 0.32 * bandGain) * top * expo;
            v.inside[k] = top * expo; v.under[k] = base * expo;
            v.glowTop[k] = giantGlowAt(a, a.top[k]) * expo; v.glowUnder[k] = giantGlowAt(a, a.base[k]) * expo;
            const RGB dc = deckCol(k);
            v.deck[k].material(dc * tint(a.top[k]), colGain);
            v.below[k].material(dc * tint(a.base[k]) * (k == last ? 0.8f : 0.9f), colGain);
        }
        for (int k = nd; k < GIANT_MAX_DECKS; k++) { v.deck[k] = v.deck[last]; v.below[k] = v.below[last]; }
        v.glowAir = 0.25 * giantGlowAt(a, bar, cut) * expo; v.glowCol = ch.glowCol;
        // the air's looks by the pressure's logarithm: the dark sky high over the clouds, the haze (amber; an ice giant's
        // cyan), each deck's top and base (the clear bands' light reddened under them), the deep's grey (an ice giant's
        // blue, a brown dwarf's ember)
        struct Look { double lp; RGB zen, hor; double zenS, horS; };
        const RGB ink(0.06f, 0.08f, 0.2f);
        std::vector<Look> looks = {
            {lpOf(PROBE_TOP_BAR), lerp(ink, band, 0.12f), lerp(band, white, 0.5f), 12, 50},
            {lpOf(0.02), lerp(ink, band, 0.2f), lerp(band, white, 0.45f), 20, 52},
            {lpOf(a.hazeBar), lerp(lerp(ink, band, 0.2f), warm(band, 1) * 0.9f, 0.6f), warm(lerp(band, white, 0.4f), 0.35), 34, 54},
            {lpOf(a.top[0]), warm(band, 0.8) * 0.95f, warm(lerp(band, white, 0.35f), 0.55), 44, 54}};
        const RGB deepZ = ml ? ml->dz : (ice ? RGB(0.2f, 0.28f, 0.34f) : (ch.kind == GK_BROWN ? RGB(0.3f, 0.14f, 0.1f) : RGB(0.32f, 0.27f, 0.26f)));
        const RGB deepH = ml ? ml->dh : (ice ? RGB(0.28f, 0.36f, 0.42f) : (ch.kind == GK_BROWN ? RGB(0.42f, 0.2f, 0.14f) : RGB(0.42f, 0.37f, 0.36f)));
        for (int k = 0; k < nd; k++) {
            const RGB dc = deckCol(k);
            if (k > 0) looks.push_back({lpOf(a.top[k]), dc * tint(a.top[k]) * 0.7f, lerp(dc, white, 0.15f) * tint(a.top[k]) * 0.9f, 28, 34});
            if (k == last && k > 0) looks.push_back({lpOf(a.base[k]), deepZ, deepH, 30, 36});
            else looks.push_back({lpOf(a.base[k]), dc * tint(a.base[k]) * 0.8f, lerp(dc, white, 0.2f) * tint(a.base[k]), 32, 38});
        }
        for (size_t q = 1; q < looks.size(); q++) looks[q].lp = std::max(looks[q].lp, looks[q - 1].lp + 1e-3);
        size_t k0 = 0;
        while (k0 + 2 < looks.size() && lp > looks[k0 + 1].lp) k0++;
        const float u = (float)clampd((lp - looks[k0].lp) / (looks[k0 + 1].lp - looks[k0].lp), 0, 1);
        const RGB zen = lerp(looks[k0].zen, looks[k0 + 1].zen, u), hor = lerp(looks[k0].hor, looks[k0 + 1].hor, u);
        v.zenShade = looks[k0].zenS + (looks[k0 + 1].zenS - looks[k0].zenS) * u;
        v.horShade = looks[k0].horS + (looks[k0 + 1].horS - looks[k0].horS) * u;
        v.air.set({{0, RGB(0, 0, 0)}, {12, zen * 0.5f}, {26, zen}, {42, lerp(zen, hor, 0.55f)}, {54, hor}, {63, lerp(hor, white, 0.45f)}}, colGain);
        v.ring.material(band, (float)clampd(lf, 0.6, 1.1));
        if (g.rings) {   // the rings over the probe: their plane through the giant's centre, its normal the spin axis
            v.rings = true; v.ringN = W * g.spinAxis; v.ringC = W * (sys.bodyPos(probe.body, tt) - pos);
            v.ringR0 = g.ringInner * g.radiusKm; v.ringR1 = g.ringOuter * g.radiusKm; v.ringProf = &spaceR.ringProfile(sys, probe.body);
            v.ringLit = std::fabs(dot(g.spinAxis, sunW)) * 0.7 + 0.3;
        }
        const double windE = giantWindEast(a, probe.aimLat, bar);
        v.windAz = windE >= 0 ? 90 * DEG : -90 * DEG;
        v.cirrusKm = giantAltitudeKm(a, std::min(0.3, 0.6 * a.top[0]));   // (X-03: over the first deck, whatever its height)
        v.cirrus = 1 - smoothstep(lpOf(a.top[0]), lpOf(a.base[0]), lp);
        v.cirrusRun = 0.25 * a.windPeak * 0.001 * std::max(0.0, c - probeDescentStart());   // the jet's core racing past
        Flash fl = flashAt(a, probe.flight, probe.seed, c, &probe.decks, probe.descYaw + probe.camYaw, zp, v.px, v.pz);
        v.flash = fl.strength; v.flashPos = fl.pos; v.bolt = fl.bolt; v.boltTo = fl.boltTo; v.boltSeed = fl.seed;
        v.lamp = 1 - smoothstep(std::log(0.03), std::log(0.15), std::log(std::max(Lp, 1e-9)));   // where the sun's light is nearly gone
        v.rain = 0;
        for (int k = 0; k < nd; k++) if (CLOUD_SPECIES[a.species[k]].convective && bar >= a.top[k]) v.rain = std::max(v.rain, 0.7 * (1 - smoothstep(a.base[k], a.base[k] * 2.2, bar)));
        v.motes = smoothstep(a.base[last], a.base[last] * 1.6, bar);
        v.heat = 0.35 * std::pow(clampd((giantTemperatureK(a, bar) - 520) / (a.heatK - 520), 0, 1), 2);   // the deep's air near the probe's end glows dull red
        v.hail = ch.hail ? smoothstep(a.top[last], std::sqrt(a.top[last] * a.base[last]), bar) : 0.0;   // falling out of the lowest deck
        {   // the aurora: the curtains over the oval round the nearer magnetic pole, over the clouds (the haze washes them out)
            double ex, ez, p0;
            if (probeAuroraWay(ex, ez, p0)) {
                v.aurora = ch.aurora * expo * 0.8 * (1 - smoothstep(lpOf(a.hazeBar * 0.5), lpOf(a.top[0]), lp));
                v.auroraDir = Vec3(ex, 0, ez); v.auroraP0 = p0;
                v.auroraH0 = giantAltitudeKm(a, 2e-7); v.auroraH = 60 * probe.decks.sh;
                v.auroraLow = ch.auroraLow; v.auroraHigh = ch.auroraHigh;
            }
        }
        if (probe.sightClock > 0 && std::fabs(c - probe.sightClock) < 6) {   // it passes across the beam, once
            const double su = (c - probe.sightClock) / 6;   // -1 .. 1
            v.sight = std::pow(std::cos(su * PI / 2), 0.7);
            const double hy = probe.descYaw;
            const Vec3 fwd(std::sin(hy), 0, std::cos(hy)), side(std::cos(hy), 0, -std::sin(hy));
            v.sightPos = fwd * 0.2 + side * (0.42 * su) + Vec3(0, -0.06 - 0.02 * su, 0);
            v.sightAxis = side;
        }
        v.time = c; v.seed = probe.seed;
        const uint32_t* space = nullptr;
        if (probeSpaceShows(v) > 0.003) {   // the sky beyond the air: the space renderer without the giant (its rings are the view's)
            int ba = -1, bb = -1; double da = 1e300, db = 1e300;
            for (int i = 0; i < (int)sys.bodies.size(); i++) {
                if (i == probe.body || sys.bodies[i].type == PT_COMPANION) continue;
                double d = length(sys.bodyPos(i, tt) - pos);
                if (d < da) { bb = ba; db = da; ba = i; da = d; } else if (d < db) { bb = i; db = d; }
            }
            spaceR.proj = pj;
            spaceR.setupPalette(fb, &sys, ba, bb, 1);
            SpaceContext sc;
            sc.sys = &sys; sc.stars = &nb.stars; sc.t = tt; sc.shipPos = pos; sc.cam = camW;
            sc.bankBodyA = ba; sc.bankBodyB = bb; sc.detailBody = -1; sc.excludeBody = probe.body;
            sc.starIntensity = 1 - 0.8 * smoothstep(-0.12, 0.12, sunL.y);
            spaceR.render(fb, sc);
            fb.mush(2);
            skyPic.resize(rgbBuf.size());
            fb.toRGB(skyPic.data(), 1.0, settings.dither);
            space = skyPic.data();
        }
        renderProbeView(v, space, rgbBuf.data());
        if (mixIn < 1) {   // the crossfade
            uint8_t* o = (uint8_t*)rgbBuf.data(); const uint8_t* s = (const uint8_t*)spacePic.data();
            int w = (int)(mixIn * 256);
            for (size_t i = 0, n = rgbBuf.size() * 4; i < n; i++) if ((i & 3) != 3) o[i] = (uint8_t)((s[i] * (256 - w) + o[i] * w) >> 8);
        }
    }
    if (c >= PROBE_FALL_S && c < probeDescentStart()) {   // the entry: the plasma round the heat shield, ahead along the fall (down), streaked; the scene through it in its colour
        double u = (c - PROBE_FALL_S) / PROBE_ENTRY_S, heat = std::pow(std::sin(PI * std::min(1.0, u * 1.15)), 1.5);
        ShadeRamp plasma;
        plasma.set({{0, RGB(0, 0, 0)}, {20, RGB(0.45f, 0.12f, 0.03f)}, {40, RGB(1.0f, 0.45f, 0.12f)}, {54, RGB(1.0f, 0.8f, 0.45f)}, {63, RGB(1, 1, 0.92f)}});
        Vec3 dv = camW * (-up);   // the fall's direction in the view: the plasma is brightest round it
        const double f = pj.f, cx = pj.cx, cy = pj.cy;
        double vx = dv.z > 0.05 ? cx + f * dv.x / dv.z : cx, vy = dv.z > 0.05 ? cy - f * dv.y / dv.z : FBH * 2.0;   // its vanishing point (the streaks run from it)
        uint64_t flick = probe.seed ^ (uint64_t)(c * 30);
        static float streaks[1024];   // the streaks by the angle round the vanishing point, this frame's
        for (int i = 0; i < 1024; i++) streaks[i] = (float)(0.75 + 0.25 * gnoise2((i / 1024.0 * TAU - PI) * 14, c * 2.5, flick));
        parallelFor(FBH, 8, [&](int y0, int y1) {
            float o[3];
            for (int y = y0; y < y1; y++) for (int x = 0; x < FBW; x++) {
                double rx = (x + 0.5 - cx) / f, ry = -(y + 0.5 - cy) / f;
                double cs = (rx * dv.x + ry * dv.y + dv.z) / std::sqrt(rx * rx + ry * ry + 1);
                if (cs < 0.15) continue;
                double ang = std::atan2(y + 0.5 - vy, x + 0.5 - vx);
                double pv = heat * 62 * std::pow((cs - 0.15) / 0.85, 1.4) * streaks[clampi((int)((ang + PI) / TAU * 1024), 0, 1023)];
                if (pv < 2) continue;
                uint32_t& p = rgbBuf[(size_t)y * FBW + x];
                double sceneShade = (0.3 * (p & 255) + 0.59 * ((p >> 8) & 255) + 0.11 * ((p >> 16) & 255)) / 255.0 * 63;
                plasma.at(std::min(63.0, pv + sceneShade * (1 - 0.6 * std::min(1.0, pv / 40))), o);
                p = 0xFF000000u | ((uint32_t)clampi((int)(o[2] * 255 + 0.5f), 0, 255) << 16) | ((uint32_t)clampi((int)(o[1] * 255 + 0.5f), 0, 255) << 8) | (uint32_t)clampi((int)(o[0] * 255 + 0.5f), 0, 255);
            }
        });
    }
    if (harness.probeQuiet) return;
    // the link: its static grows as the probe goes deeper (the signal through ever more air)
    double lost = c >= probeDescentStart() ? clampd(std::log(std::max(bar, 1e-3) / 0.5) / std::log(probe.flight.crushBar / 0.5), 0, 1) : 0.0;
    double amp = 4 + 34 * lost * lost;
    uint32_t rs = (uint32_t)(c * 60) * 2654435761u + 0x9B0Bu;
    uint8_t* o = (uint8_t*)rgbBuf.data();
    for (size_t i = 0, n = rgbBuf.size(); i < n; i++) {
        rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
        int d = (int)(rs % (uint32_t)(2 * amp + 1)) - (int)amp;
        if ((rs >> 20) % 4096 < (uint32_t)(30 * lost * lost)) d = 120;   // a fleck of snow
        for (int k = 0; k < 3; k++) o[i * 4 + k] = (uint8_t)clampi(o[i * 4 + k] + d, 0, 255);
    }
}
