// H-05 (2026-10-10): what a recording on the decoder was made on, heard (`AudioState::medium`, peoples/records.h's RM_*). The
// industrial age cut its voices and its music in wax: a narrow band through a horn, the needle's crackle, the hiss of the surface
// swishing round at the cylinder's turn and the turn's wow. The modern age pressed discs (a wider band, a quieter surface, a pop now
// and then, the slow wow of the turntable) and wound tape on reels (hiss, wow and flutter, a little saturation, a dropout where the
// oxide wore). The late ages kept them on drives, whose bits rotted (a stutter, a moment gone coarse, a gap), and in crystals (a faint
// shimmer of the light read twice); the far future's something that hums carries a drone under what it says. A record with no sound of
// its own (a tablet, a scroll, a page, a bone flute's holes) is the ship's own rendering: it passes untouched. C-11: a recording the radar
// caught plays on the ship's player as the ship kept it off the air (`MEDIUM_AIR`): the receiver's band, the air's hiss, its slow fading.
#include "audio.h"
#include "peoples/records.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr double TAU_M = 6.283185307179586, PI_M = 3.141592653589793;
inline double onePole(double f, double dt) { return 1.0 - std::exp(-TAU_M * f * dt); }
// a medium's numbers: the band (high-pass and low-pass corners), the horn, the gain back, the saturation, the speed's wander (the wow's
// rate and its pitch share, the flutter's), the surface (the hiss's level and its band, the rumble, the crackle's rate and size)
struct MediumSound {
    double hp, lp; int lpPoles;
    double hornF, hornBw, horn;
    double gain, drive;
    double wowHz, wow, flutHz, flut;
    double hiss, hissLo, hissHi, rumble, crackRate, crackSize;
};
const MediumSound* soundOf(int medium) {
    static const MediumSound CYLINDER = {320, 2900, 3, 1250, 700, 0.8, 1.25, 1.6, 2.2, 0.0035, 0, 0, 0.027, 900, 4200, 0.012, 8.0, 0.3};
    static const MediumSound DISC = {55, 7200, 2, 0, 0, 0, 1.08, 0, 0.55, 0.0012, 0, 0, 0.0045, 2500, 9000, 0.004, 1.4, 0.07};
    static const MediumSound REEL = {40, 7800, 1, 0, 0, 0, 1.05, 1.3, 0.6, 0.0011, 7.0, 0.0005, 0.010, 2200, 10000, 0.002, 0, 0};
    static const MediumSound DRIVE = {20, 0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static const MediumSound CRYSTAL = {20, 0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static const MediumSound HUM = {70, 6500, 1, 0, 0, 0, 1.14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static const MediumSound AIR = {260, 2700, 2, 0, 0, 0, 1.45, 1.3, 0.33, 0.0016, 0, 0, 0.022, 1800, 7000, 0.006, 2.2, 0.09};   // C-11: the receiver's band
    switch (medium) {
        case MEDIUM_AIR: return &AIR;
        case RM_CYLINDER: return &CYLINDER;
        case RM_DISC: return &DISC;
        case RM_REEL: return &REEL;
        case RM_DRIVE: return &DRIVE;
        case RM_CRYSTAL: return &CRYSTAL;
        case RM_HUM: return &HUM;
        default: return nullptr;   // no sound of its own: the ship's rendering, untouched
    }
}
}

void AudioSynth::mediumCoefs(double dt, int medium) {
    co.mdUp = 1 - std::exp(-dt * 4.0); co.mdDown = 1 - std::exp(-dt * 2.0); co.mdDropK = 1 - std::exp(-dt * 40); co.mdSr = 1.0 / dt;
    const MediumSound* M = medium >= 0 ? soundOf(medium) : nullptr;
    if (!M) return;
    co.mdAh = onePole(M->hp, dt); co.mdAl = M->lpPoles > 0 ? onePole(M->lp, dt) : 0;
    co.mdHornR = std::exp(-PI_M * M->hornBw * dt); co.mdHornB = 2 * co.mdHornR * std::cos(TAU_M * M->hornF * dt);
    co.mdHissHi = onePole(M->hissHi, dt); co.mdHissLo = onePole(M->hissLo, dt); co.mdRum = onePole(45, dt);
}

double AudioSynth::mediumSample(double x, double dt, AudioState& st) {
    const int medium = st.medium;
    if (medium != mdKind) {   // another record: the filters, the delay line and the wear begin again
        mdKind = medium; mdHp1 = mdHp2 = mdLp1 = mdLp2 = mdLp3 = mdHy1 = mdHy2 = mdNzLp = mdNzHp = mdRum = 0; mdT = 0;
        mdCrkT = -1; mdCrkNext = 0.3; mdDropT = 6; mdDrop = 1; mdDropLvl = 1; mdGlT = -1; mdGlNext = 4 + 6 * (0.5 + 0.5 * noise()); mdShimPh = mdShimLp = 0;
        mdBuf.assign(16384, 0.0f); mdI = 0;   // 0.74 s: a drive's stutter reads back half a second at most
    }
    const MediumSound* M = medium >= 0 ? soundOf(medium) : nullptr;
    // the surface sounds while something plays: it comes up as the record starts and dies with it (a pause stops the turning)
    const bool playing = (mOn || spOn) && !(st.piecePause && mOn) && !(st.speechPause && spOn);
    const double target = M && playing ? 1.0 : 0.0;
    mdLvl += (target - mdLvl) * (target > mdLvl ? co.mdUp : co.mdDown);
    if (!M) return x;
    mdT += dt;
    // the band: two high-passes, the low-passes, the horn's hump
    const double ah = co.mdAh;
    mdHp1 += (x - mdHp1) * ah; double y = x - mdHp1;
    mdHp2 += (y - mdHp2) * ah; y -= mdHp2;
    if (M->lpPoles > 0) { const double al = co.mdAl; mdLp1 += (y - mdLp1) * al; y = mdLp1; if (M->lpPoles > 1) { mdLp2 += (y - mdLp2) * al; y = mdLp2; } if (M->lpPoles > 2) { mdLp3 += (y - mdLp3) * al; y = mdLp3; } }
    if (M->horn > 0) {   // Klatt's resonator, unity at the centre (its radius and pole the block's)
        const double r = co.mdHornR, B = co.mdHornB, C = -r * r;
        const double o = (1 - r) * y + B * mdHy1 + C * mdHy2; mdHy2 = mdHy1; mdHy1 = o;
        y += M->horn * o;
    }
    y *= M->gain;
    if (M->drive > 0) y = std::tanh(M->drive * y) / M->drive;
    // the speed: the delay line read at a wandering delay (a cylinder's turn, a turntable's, a reel's wow and its capstan's flutter)
    mdBuf[mdI] = (float)y;
    double d = 0.006;
    if (M->wow > 0) d += M->wow / (TAU_M * M->wowHz) * (1 + std::sin(TAU_M * M->wowHz * mdT));
    if (M->flut > 0) d += M->flut / (TAU_M * M->flutHz) * (1 + std::sin(TAU_M * M->flutHz * mdT + 1.3));
    const double sr = co.mdSr;
    auto tap = [&](double back) { double pos = (double)mdI - back * sr; while (pos < 0) pos += mdBuf.size(); size_t i0 = (size_t)pos % mdBuf.size(), i1 = (i0 + 1) % mdBuf.size(); double fr = pos - std::floor(pos); return mdBuf[i0] * (1 - fr) + mdBuf[i1] * fr; };
    double out = tap(d);
    // the wear and the media of the late ages
    if (medium == RM_REEL) {   // a dropout where the oxide wore: a dip of a tenth of a second or two now and then
        mdDropT -= dt;
        if (mdDropT <= 0) { const bool gap = 0.5 + 0.5 * noise() < 0.3; mdDrop = gap ? 0.25 : 1.0; mdDropT = gap ? 0.06 + 0.14 * (0.5 + 0.5 * noise()) : 7 + 18 * (0.5 + 0.5 * noise()); }
        mdDropLvl += (mdDrop - mdDropLvl) * co.mdDropK;
        out *= mdDropLvl;
    } else if (medium == RM_DRIVE) {   // the rot: a stutter (the last scrap read again), a moment gone coarse, a gap
        mdGlNext -= dt;
        if (mdGlT < 0 && mdGlNext <= 0 && playing) {
            mdGlT = 0; const double u = 0.5 + 0.5 * noise();
            mdGlKind = u < 0.45 ? 0 : (u < 0.8 ? 1 : 2);
            mdGlDur = mdGlKind == 0 ? 0.03 + 0.05 * (0.5 + 0.5 * noise()) : (mdGlKind == 1 ? 0.12 + 0.3 * (0.5 + 0.5 * noise()) : 0.04 + 0.08 * (0.5 + 0.5 * noise()));
            mdGlLen = mdGlKind == 0 ? mdGlDur * (2 + (int)(3.99 * (0.5 + 0.5 * noise()))) : mdGlDur;   // a stutter: the scrap two to five times
            mdGlStep = 0; mdGlHold = 0;
            mdGlNext = 6 + 14 * (0.5 + 0.5 * noise());
        }
        if (mdGlT >= 0) {
            if (mdGlKind == 0) out = tap(d + mdGlT + mdGlDur - std::fmod(mdGlT, mdGlDur));   // the scrap read just before, again and again
            else if (mdGlKind == 1) { mdGlHold += dt * 5512; if (mdGlHold >= 1) { mdGlHold -= 1; mdGlStep = std::round(out * 12) / 12; } out = mdGlStep; }   // a quarter the rate, a dozen steps
            else out = 0;
            mdGlT += dt; if (mdGlT >= mdGlLen) mdGlT = -1;
        }
    } else if (medium == RM_CRYSTAL) {   // the light read twice: a faint second reading a few milliseconds behind, wandering
        out = 0.92 * out + 0.32 * tap(d + 0.009 + 0.003 * std::sin(TAU_M * 0.23 * mdT));
    } else if (medium == MEDIUM_AIR) {   // C-11: as it came over the air: a slow fading, a faint ring of the receiver under it
        out *= 0.74 + 0.26 * std::sin(TAU_M * 0.13 * mdT) * std::sin(TAU_M * 0.31 * mdT + 0.7);
        out *= 0.85 + 0.15 * std::sin(TAU_M * 31.0 * mdT);
    } else if (medium == RM_HUM) {   // the drone it hums, the voice riding on it
        double hum = 0;
        const double f[3] = {55.0, 55.4, 110.3}, a[3] = {1.0, 0.7, 0.35};
        for (int k = 0; k < 3; k++) { mdHumPh[k] += dt * f[k]; if (mdHumPh[k] > 1) mdHumPh[k] -= 1; hum += a[k] * std::sin(TAU_M * mdHumPh[k]); }
        out = out * (0.86 + 0.14 * std::sin(TAU_M * 0.37 * mdT)) + 0.022 * hum * mdLvl;
    }
    mdI = (mdI + 1) % mdBuf.size();
    // the surface: hiss in its band (swishing round at a cylinder's turn), a rumble, the crackle
    double surface = 0;
    if (M->hiss > 0) {
        const double n = noise();
        mdNzLp += (n - mdNzLp) * co.mdHissHi; mdNzHp += (mdNzLp - mdNzHp) * co.mdHissLo;
        double h = (mdNzLp - mdNzHp) * M->hiss * 2.2;
        if (medium == RM_CYLINDER) { const double sw = std::sin(PI_M * M->wowHz * mdT); h *= 0.55 + 0.45 * sw * sw; }
        surface += h;
    }
    if (M->rumble > 0) { mdRum += (noise() - mdRum) * co.mdRum; surface += M->rumble * mdRum * 6; }
    if (M->crackRate > 0) {
        mdCrkNext -= dt;
        if (mdCrkNext <= 0) {   // a tick, now and then a pop
            const double u = 0.5 + 0.5 * noise();
            mdCrkT = 0; mdCrkA = M->crackSize * (u < 0.85 ? 0.25 + 0.5 * u : 1.0) * (noise() < 0 ? -1 : 1);
            mdCrkNext = -std::log(std::max(1e-4, 0.5 + 0.5 * noise())) / M->crackRate;
        }
        if (mdCrkT >= 0) { const double c = mdCrkA * std::exp(-mdCrkT / 0.0009); mdCrkLp += (c - mdCrkLp) * 0.6; surface += mdCrkLp; mdCrkT += dt; if (mdCrkT > 0.01) mdCrkT = -1; }
    }
    return out + surface * mdLvl;
}
