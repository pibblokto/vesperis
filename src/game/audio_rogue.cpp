// D-03 (2026-10-23): the voices of the dark (`docs/ideas/ITERATION-1.4.0.md`, `docs/ideas/IDEAS-worlds-and-wanderers.md` section 3): a
// voice for every drifter, from its seed and its body, in the receiver's channel (`channelSample`'s SIG_ROGUE) on parameters of its own
// (`rogueDerive`: indices no other signal reads, so no other signal's sound moved). It is grown from what receivers hear of the planets and
// from C-16's static: Jupiter's decametric storms (the L-bursts' slow swishes like surf, the S-bursts' crackle falling in pitch), the
// kilometric radiation of Saturn and of the Earth's aurorae (whistled howls rising and falling), the Earth's chorus (flocks of rising
// chirps), whistlers (falling tones) and hiss, the pulses a brown dwarf's aurorae send once a turning (SIMP 0136's, LSR J1835's), and the
// arcs a moon's flux tube draws in a giant's (Io's in Jupiter's). Its parts:
// * the breath of its turning: the beam of its aurorae sweeps past once a turning (a breath every 20-90 s by its spin, `Signal::turnS`), a
//   main lobe and, by its seed, the other pole's weaker one; silence while the beam is turned away (its drones a breath of themselves)
// * the body by what it is (RV_*): a giant's storm (the surf, the S-bursts' trains, a roar); a brown dwarf's thicker, and its beam's
//   cyclotron howl as the beam comes round; a small world's the Earth's (chorus, whistlers, hiss, the kilometric howl wandering), a young
//   world's over the rumble of its dynamo, a metal world's with its iron core ringing now and then
// * its moon's flux tube (`Signal::moonS`, the innermost moon's orbit): an arc swept up and down again, the body pumped, every 0.9-16 s
// * its motif, its signature: a figure of four to six whistles on a scale of its own (seven to nineteen steps to the octave), each its own
//   pitch, spanning three quarters of an octave at least, with glides and a vibrato of its own, as the beam's centre passes; backwards,
//   softer and in a key of its own as the other pole's does; each whistle hanging on after it in the plasma, dying over its own 10-18 s
// * plasma vowels: a moan with no language in it (a voice's source through three formants sweeping through three vowels of its own) and
//   the plasma's breath (noise through two resonances wandering between those vowels)
// * detuned drones: a chord of three or four notes on its scale, each beating against a twin a little off, partials of its own
// * the balance of its parts its own, and the whole through its plasma's two resonances (the plasma's and the gyrofrequency's)
// No two alike: over the hundred drifters nearest home every pair's voice prints lie further apart than two magnetospheres' do (`rogues
// voices`). Far off the radar hears it worn (C-16's `radarWear`: its top and its bottom go, besides the worn bus's fades, dropouts and
// echoes); under another signal a little worn too. A drifter's beacon (D-05's people) is a machine of C-16's under the voice, slowed and
// darkened.
#include "audio.h"
#include "peoples/signals.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace {
constexpr double TAU_R = 6.283185307179586, PI_R = 3.141592653589793;
inline double u01R(uint64_t h) { return (h >> 11) * (1.0 / 9007199254740992.0); }
inline double wrap1(double p) { return p - std::floor(p); }
inline double clamp01(double x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
inline double smooth(double x) { return x * x * (3 - 2 * x); }
// the voice's bands (`RadarChannel::rgCo`): the fixed ones' coefficients made when the voice is read, the moving ones' every 32 samples
enum RogueBand { CO_PLASMA1 = 0, CO_PLASMA2, CO_SURF, CO_ROAR, CO_HISS, CO_RES1, CO_RES2, CO_WHISTLE, CO_ARC };
// the beam's lobe: 1 at its centre, a raised cosine to nothing `hw` of the turning off it
inline double lobe(double ph, double centre, double hw) { double d = std::fabs(wrap1(ph - centre + 0.5) - 0.5); return d < hw ? 0.5 + 0.5 * std::cos(PI_R * d / hw) : 0.0; }
// the vowels the plasma moans through (F1, F2, F3): u, o, a, the open o, a schwa, a dark i, a nasal hum
const double PLASMA[7][3] = {{300, 700, 2300}, {430, 820, 2450}, {720, 1150, 2550}, {560, 900, 2500}, {500, 1350, 2400}, {280, 1600, 2600}, {250, 1000, 2100}};
const char* const PLASMA_NAMES[7] = {"u", "o", "a", "aw", "uh", "ih", "m"};
const int EDOS[6] = {7, 9, 11, 13, 17, 19};   // its scale (the motif's and the drones'): its steps to the octave
// its parts' weights (`RadarVar::rgW`): a storm's surf, S-bursts and roar share their slots with a small world's chorus, whistlers and hiss
enum RogueWeight { RW_DRONES = 0, RW_MOAN, RW_PLASMA, RW_SURF_CHORUS, RW_CRACKLE_WHISTLERS, RW_ROAR_HISS, RW_HOWL, RW_COUNT };
const double RING[4] = {1.0, 2.756, 5.404, 8.933};   // a free bar's partials: the iron's ring
const char* const TIMBRES[4] = {"pure", "hollow", "breathy", "metallic"};
// what follows from the body: the breath's period (the turning's 0.33-3.7 h as 20-90 s; a drifter of no known spin its own), the moon's
// pulse's (its orbit of 1-28 hours, most 3.5-8, as 0.9 s times its hours to the 0.85: an hour 0.9 s, five 3.5 s, a day 13 s), the power's
// share (0 the faintest small world, 1 a loud brown dwarf)
double breathOf(const RogueVoice& r, double ownU) { return r.turnS > 0 ? std::max(20.0, std::min(90.0, 20 + 70 * (r.turnS - 1200) / 12000)) : 20 + 70 * ownU; }
double pulseOf(const RogueVoice& r) { return r.moonS > 0 ? std::max(0.9, std::min(16.0, 0.9 * std::pow(r.moonS / 3600.0, 0.85))) : 0.0; }
double powerShare(const RogueVoice& r) { return clamp01(std::log10(1 + 10 * std::max(0.0, r.power)) / std::log10(31.0)); }
}

RogueVoice rogueVoiceFor(const Signal& s) {
    RogueVoice r;
    r.cls = rogueVoiceClass(s.bodyType); r.turnS = s.turnS; r.moonS = s.moonS; r.power = s.power;
    return r;
}

void AudioSynth::rogueDerive(RadarVar& v, uint64_t seed) {
    auto u = [&](int k) { return u01R(mix64(seed ^ ((uint64_t)k * 0x9E3779B97F4A7C15ULL) ^ 0xC07ULL)); };   // rvDerive's draw, on indices from 161
    v.rgLobe = 0.15 + 0.15 * u(161); v.rgPole2 = u(162) < 0.25 ? 0 : 0.25 + 0.6 * u(163); v.rgPole2At = 0.4 + 0.2 * u(164); v.rgBreathU = u(165);
    v.rgN = 4 + (int)(u(166) * 2.99);
    v.rgEdo = EDOS[(int)(u(167) * 5.99)];
    const double base = 400 * std::pow(2.0, 1.8 * u(168));
    // the whistles: steps of its scale from half an octave under its base to an octave over, each its own (none twice in the figure, none a
    // step from the one before), the figure spanning three quarters of an octave at least
    const int lo = -v.rgEdo / 2, hi = v.rgEdo;
    int steps[6];
    for (int i = 0; i < 6; i++) {
        int step = lo + (int)(u(170 + i) * (hi - lo + 1));
        for (int tries = 0; tries < 2 * (hi - lo + 1); tries++) {
            bool clash = i > 0 && std::abs(step - steps[i - 1]) < 2;
            for (int j = 0; j < i && !clash; j++) clash = steps[j] == step;
            if (!clash) break;
            step = step + 1 > hi ? lo : step + 1;   // the next free step up, round to the bottom
        }
        steps[i] = step;
    }
    {
        int mn = steps[0], mx = steps[0]; for (int i = 1; i < v.rgN; i++) { mn = std::min(mn, steps[i]); mx = std::max(mx, steps[i]); }
        const int need = (3 * v.rgEdo + 3) / 4; int& last = steps[v.rgN - 1];
        if (mx - mn < need) last = mn + need <= hi ? mn + need : mx - need;
    }
    v.rgMotifLen = 0;
    for (int i = 0; i < 6; i++) {
        v.rgNoteF[i] = base * std::pow(2.0, steps[i] / (double)v.rgEdo);
        v.rgNoteLen[i] = 0.16 + 0.44 * u(188 + i);
        v.rgGlide[i] = 0.7 * u(194 + i) * u(194 + i);
        if (i < v.rgN) v.rgMotifLen += v.rgNoteLen[i];
    }
    v.rgVibHz = 3.8 + 3.6 * u(200); v.rgVib = 0.002 + 0.013 * u(201); v.rgTimbre = (int)(u(202) * 3.99);
    {   // the other pole answers in a key of its own: a quarter to a little over half an octave of its scale up or down; its whistles hang 10-18 s
        const int sh = std::max(2, (int)std::lround(v.rgEdo * (0.25 + 0.3 * u(251))));
        v.rgAnswer = std::pow(2.0, (u(252) < 0.5 ? -sh : sh) / (double)v.rgEdo);
        v.rgHangS = 10 + 8 * u(253);
    }
    v.rgMoanF = 45 * std::pow(2.0, 1.75 * u(203)); v.rgMoanRate = 0.03 + 0.09 * u(204); v.rgPlasmaRate = 0.02 + 0.06 * u(205);
    for (int i = 0; i < 3; i++) { int w = (int)(u(206 + i) * 6.99); if (i > 0 && w == v.rgVowel[i - 1]) w = (w + 1 + (int)(u(209 + i) * 5.99)) % 7; v.rgVowel[i] = w; }
    // the drones: a chord of three or four notes on its scale from a root of 45-180 Hz, each a step of a third to three quarters of an octave over
    // the last, each beating against a twin 0.12-2.4 Hz off; six partials falling by a tilt of its own, the even ones by a share of their own
    v.rgChordN = 3 + (int)(u(231) * 1.99);
    {
        const double root = 45 * std::pow(2.0, 2.0 * u(212)); int step = 0;
        for (int i = 0; i < 4; i++) {
            if (i > 0) step += std::max(1, (int)std::lround(v.rgEdo * (0.3 + 0.45 * u(232 + i))));
            v.rgChordF[i] = root * std::pow(2.0, step / (double)v.rgEdo);
            v.rgBeat[i] = 0.12 + 2.3 * u(236 + i) * u(236 + i);
        }
        v.rgTilt = 0.4 + 1.0 * u(213); v.rgEven = 0.08 + 0.92 * u(214);
        double sum = 0;
        for (int h = 0; h < 6; h++) { v.rgPart[h] = std::pow(h + 1.0, -v.rgTilt) * ((h % 2) ? v.rgEven : 1.0); sum += v.rgPart[h] * v.rgPart[h]; }
        for (int h = 0; h < 6; h++) v.rgPart[h] /= std::sqrt(sum);   // each note's partials to one root mean square
        v.rgDroneAmp = 0.11 / std::sqrt((double)v.rgChordN);   // the chord about 0.11 rms whatever its size (two twins a note)
    }
    // its parts' weights, one often leading (a storm mostly crackle, another mostly surf): the tones (the drones 0.7-1.5, the moan, the howl 0.5-1.5),
    // the bursts and the whistlers 0.3-1.4, the bands of noise (the plasma's breath, the surf or the chorus, the roar or the hiss) 0.15-1.1;
    // a giant's howl in two of five
    for (int i = 0; i < RW_COUNT; i++) {
        const bool tone = i == RW_DRONES || i == RW_MOAN || i == RW_HOWL, burst = i == RW_CRACKLE_WHISTLERS;
        const double lo = i == RW_DRONES ? 0.7 : (tone ? 0.5 : (burst ? 0.3 : 0.15)), hi = tone ? 1.5 : (burst ? 1.4 : 1.1);
        v.rgW[i] = lo + (hi - lo) * std::pow(u(240 + i), 1.6);
    }
    v.rgGiantHowl = u(250) >= 0.6;
    // its plasma's resonances (the plasma frequency's and the gyrofrequency's, by its field and its density): 160-740 Hz and 0.9-3.6 kHz,
    // each lifting the voice by 5-12 dB at its centre
    v.rgResF[0] = 160 * std::pow(2.0, 2.2 * u(254)); v.rgResF[1] = 900 * std::pow(2.0, 2.0 * u(255));
    v.rgResG[0] = 0.8 + 2.2 * u(256); v.rgResG[1] = 0.8 + 2.2 * u(257);
    v.rgSurfF = 250 * std::pow(2.0, 3.3 * u(216)); v.rgSRate = 8 + 37 * u(217); v.rgSF = 900 * std::pow(2.0, 1.5 * u(218)); v.rgSFall = 0.2 + 0.25 * u(219);
    v.rgRoarF = 80 * std::pow(2.0, 2.3 * u(220)); v.rgRoarFl = 16 + 18 * u(221);
    v.rgChorusF = 350 * std::pow(2.0, 1.65 * u(222)); v.rgChorusRise = 1.6 + 1.6 * u(223); v.rgChirp = 0.12 + 0.28 * u(224);
    v.rgWhistF = 2500 + 2500 * u(225); v.rgHissF = 250 * std::pow(2.0, 3.3 * u(226)); v.rgHowlLo = 350 + 450 * u(227); v.rgHowlHi = v.rgHowlLo * (1.6 + 1.4 * u(228));
    v.rgArcF = 500 * std::pow(2.0, 1.5 * u(229)); v.rgRingF = 140 * std::pow(2.0, 1.6 * u(230));
}

double AudioSynth::rogueBreathS(uint64_t seed, const RogueVoice& r) { RadarVar v; rogueDerive(v, seed); return breathOf(r, v.rgBreathU); }

std::string AudioSynth::rogueLine(uint64_t seed, const RogueVoice& r) {
    RadarVar v; rogueDerive(v, seed);
    const int cls = std::max(0, std::min((int)RV_COUNT - 1, r.cls));
    std::string notes; for (int i = 0; i < v.rgN; i++) { char b[16]; std::snprintf(b, sizeof b, "%s%.0f", i ? " " : "", v.rgNoteF[i]); notes += b; }
    const double pulse = pulseOf(r);
    std::string body;
    char b[256];
    if (cls == RV_BROWN_DWARF || cls == RV_GIANT) std::snprintf(b, sizeof b, "the storm: surf at %.0f Hz, S-bursts %.0f a second from %.0f Hz falling %.0f%%, a roar at %.0f Hz fluttering %.0f a second", v.rgSurfF, v.rgSRate, v.rgSF, 100 * v.rgSFall, v.rgRoarF, v.rgRoarFl);
    else std::snprintf(b, sizeof b, "chorus from %.0f Hz rising x%.1f, whistlers from %.0f Hz, hiss at %.0f Hz", v.rgChorusF, v.rgChorusRise, v.rgWhistF, v.rgHissF);
    body = b;
    if (cls == RV_BROWN_DWARF) { std::snprintf(b, sizeof b, ", the cyclotron howl %.0f-%.0f Hz each turning", v.rgHowlLo, v.rgHowlHi); body += b; }
    else if (cls != RV_GIANT || v.rgGiantHowl) { std::snprintf(b, sizeof b, ", the kilometric howl %.0f-%.0f Hz", v.rgHowlLo, v.rgHowlHi); body += b; }
    if (cls == RV_YOUNG) body += ", the dynamo's rumble";
    if (cls == RV_METAL) { std::snprintf(b, sizeof b, ", the iron ringing at %.0f Hz", v.rgRingF); body += b; }
    std::snprintf(b, sizeof b, "; its plasma ringing at %.0f and %.0f Hz; weighed %.1f %.1f %.1f %.1f %.1f %.1f %.1f", v.rgResF[0], v.rgResF[1], v.rgW[0], v.rgW[1], v.rgW[2], v.rgW[3], v.rgW[4], v.rgW[5], v.rgW[6]); body += b;
    char line[1024];
    char moon[48]; if (pulse > 0) std::snprintf(moon, sizeof moon, "its moon pulsing every %.1f s", pulse); else std::snprintf(moon, sizeof moon, "no moon");
    std::string chord; for (int i = 0; i < v.rgChordN; i++) { char c[24]; std::snprintf(c, sizeof c, "%s%.1f", i ? " " : "", v.rgChordF[i]); chord += c; }
    char pole[64];
    if (v.rgPole2 > 0.3) std::snprintf(pole, sizeof pole, "the other pole answering x%.2f", v.rgAnswer); else std::snprintf(pole, sizeof pole, "%s", v.rgPole2 > 0 ? "the other pole faint" : "one pole");
    std::snprintf(line, sizeof line, "%s: a breath every %.0f s (the main lobe %.0f%% of it, %s), %s; the motif %d %s whistles on %d steps to the octave (%s Hz, %.1f s, hanging %.0f s); the drones %s Hz (tilt %.1f, even %.2f); the moan at %.0f Hz through %s-%s-%s; %s",
                  ROGUE_VOICE_NAMES[cls], breathOf(r, v.rgBreathU), 200 * v.rgLobe, pole, moon,
                  v.rgN, TIMBRES[v.rgTimbre], v.rgEdo, notes.c_str(), v.rgMotifLen, v.rgHangS, chord.c_str(), v.rgTilt, v.rgEven,
                  v.rgMoanF, PLASMA_NAMES[v.rgVowel[0]], PLASMA_NAMES[v.rgVowel[1]], PLASMA_NAMES[v.rgVowel[2]], body.c_str());
    return line;
}

void AudioSynth::rogueTraits(uint64_t seed, const RogueVoice& r, std::vector<double>& out) {
    RadarVar v; rogueDerive(v, seed);
    auto oct = [](double f, double lo, double hi) { return clamp01(std::log2(f / lo) / std::log2(hi / lo)); };
    out.clear();
    out.push_back(std::max(0, std::min((int)RV_COUNT - 1, r.cls)) / (double)(RV_COUNT - 1));
    out.push_back((breathOf(r, v.rgBreathU) - 20) / 70); out.push_back(pulseOf(r) / 16); out.push_back(powerShare(r));
    out.push_back((v.rgLobe - 0.15) / 0.15); out.push_back(v.rgPole2 / 0.85); out.push_back((v.rgN - 4) / 2.0); out.push_back(v.rgTimbre / 3.0);
    for (int i = 0; i < 6; i++) out.push_back(i < v.rgN ? oct(v.rgNoteF[i], 200, 3200) : 0);
    out.push_back(oct(v.rgMoanF, 45, 152)); out.push_back(v.rgChordN - 3.0);
    for (int i = 0; i < 4; i++) out.push_back(i < v.rgChordN ? oct(v.rgChordF[i], 45, 960) : 0);
    out.push_back((v.rgTilt - 0.4) / 1.0); out.push_back(v.rgEven);
    for (int i = 0; i < 3; i++) out.push_back(v.rgVowel[i] / 6.0);
    const bool storm = r.cls == RV_BROWN_DWARF || r.cls == RV_GIANT;
    out.push_back(oct(storm ? v.rgSurfF : v.rgChorusF, 350, 1300)); out.push_back(oct(storm ? v.rgSF : v.rgHissF, 300, 2600)); out.push_back(oct(v.rgHowlLo, 350, 800));
    for (int i = 0; i < RW_COUNT; i++) out.push_back((v.rgW[i] - 0.15) / 1.45);
    out.push_back(oct(v.rgResF[0], 160, 740)); out.push_back(oct(v.rgResF[1], 900, 3600)); out.push_back((v.rgResG[0] - 0.8) / 2.2); out.push_back((v.rgResG[1] - 0.8) / 2.2);
}

// one sample of a drifter's voice (`channelSample`'s SIG_ROGUE): `R` what it is, `voice` the beacon under it (a programme; -1 none)
double AudioSynth::rogueSample(RadarChannel& c, double dt, const RogueVoice& R, int voice, float n2, AudioState& st, bool main) {
    const RadarVar& v = c.v;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    // a two-pole band: `norm` brings it to about unity at its centre (the resonator's gain there is about 1 / (2 sin(2 pi f dt))), so a band of
    // the noise is as loud as it is wide (white noise of -1..1 through 70 Hz: about 0.058 rms, as the root of the width); else the resonator's own
    auto setBand = [&](int k, double f, double bw, bool norm) {
        RadarChannel::BandCo& q = c.rgCo[k]; const double r = std::exp(-PI_R * bw * dt);
        q.B = 2 * r * std::cos(TAU_R * f * dt); q.C = -r * r; q.a = (1 - r) * (norm ? 2 * std::sin(TAU_R * f * dt) : 1.0);
    };
    auto band = [&](int k, double x, double& y1, double& y2) { const RadarChannel::BandCo& q = c.rgCo[k]; const double o = q.a * x + q.B * y1 + q.C * y2; y2 = y1; y1 = o; return o; };
    if (R.cls != c.rgCls || R.turnS != c.rgTurn || R.moonS != c.rgMoon || R.power != c.rgPow || dt != c.rgCoDt) {   // what it is, read again when it changes
        c.rgCls = R.cls; c.rgTurn = R.turnS; c.rgMoon = R.moonS; c.rgPow = R.power; c.rgCoDt = dt; c.rgCoN = 0;
        c.rgBreathS = breathOf(R, v.rgBreathU); c.rgPulseS = pulseOf(R); c.rgPw = powerShare(R);
        setBand(CO_ROAR, v.rgRoarF, 140, true); setBand(CO_HISS, v.rgHissF, 0.35 * v.rgHissF, true);
        setBand(CO_RES1, v.rgResF[0], 0.35 * v.rgResF[0], true); setBand(CO_RES2, v.rgResF[1], 0.3 * v.rgResF[1], true);
        const double low = R.cls == RV_YOUNG ? 0.75 : 1.0;   // a young world's drones a fourth lower
        for (int i = 0; i < 2 * v.rgChordN; i++) { const double a = TAU_R * dt * (v.rgChordF[i >> 1] * low + ((i & 1) ? v.rgBeat[i >> 1] : 0.0)); c.rgDrRc[i] = std::cos(a); c.rgDrRs[i] = std::sin(a); }
    }
    const bool retune = c.rgCoN == 0;   // the moving bands' turn
    c.rgCoN = (c.rgCoN + 1) & 31;
    const int cls = R.cls;
    const bool storm = cls == RV_BROWN_DWARF || cls == RV_GIANT, dwarf = cls == RV_BROWN_DWARF;
    double sig = 0, body = 0;
    // the breath of its turning: the main lobe at a quarter of the turning, the other pole's half a turning on (by its seed), silence between
    const double hw = v.rgLobe;
    if (c.rgPh < 0) c.rgPh = 0.25 - 0.5 * hw;   // first heard as the beam comes round, half up
    const double prevPh = c.rgPh;
    c.rgPh = wrap1(c.rgPh + dt / c.rgBreathS);
    auto crossed = [&](double at) { at = wrap1(at); return c.rgPh >= prevPh ? (prevPh < at && at <= c.rgPh) : (at > prevPh || at <= c.rgPh); };
    const double env = std::max(lobe(c.rgPh, 0.25, hw), v.rgPole2 * lobe(c.rgPh, 0.25 + v.rgPole2At, 0.6 * hw));
    // its motif as the beam's centre passes, backwards and softer at the other pole's
    if (crossed(0.25)) { c.rgMotT = 0; c.rgMotRev = false; c.rgMotAmp = 1.0; }
    else if (v.rgPole2 > 0.3 && crossed(0.25 + v.rgPole2At)) { c.rgMotT = 0; c.rgMotRev = true; c.rgMotAmp = 0.35 + 0.5 * v.rgPole2; }
    if (c.rgMotT >= 0) {
        double acc = 0; int k = 0;
        while (k < v.rgN && c.rgMotT >= acc + v.rgNoteLen[c.rgMotRev ? v.rgN - 1 - k : k]) { acc += v.rgNoteLen[c.rgMotRev ? v.rgN - 1 - k : k]; k++; }
        if (k >= v.rgN) c.rgMotT = -1;
        else {
            const int i = c.rgMotRev ? v.rgN - 1 - k : k, from = c.rgMotRev ? i + 1 : i - 1;
            const double u = (c.rgMotT - acc) / v.rgNoteLen[i], key = c.rgMotRev ? v.rgAnswer : 1.0;
            double f = v.rgNoteF[i] * key;
            if (k > 0 && v.rgGlide[i] > 0.01 && u < v.rgGlide[i]) f = v.rgNoteF[from] * key * std::pow(f / (v.rgNoteF[from] * key), smooth(u / v.rgGlide[i]));   // a whistle sliding up or down to its note
            f *= 1 + v.rgVib * std::sin(TAU_R * v.rgVibHz * c.rgMotT);
            const double e = std::min(1.0, c.rgMotT / 0.03) * std::min(1.0, (v.rgMotifLen - c.rgMotT) / 0.12) * (0.8 + 0.2 * std::sin(PI_R * u));
            c.rgMotPh = wrap1(c.rgMotPh + dt * f);
            const double s1 = std::sin(TAU_R * c.rgMotPh);
            double s;
            switch (v.rgTimbre) {
                case 0: s = s1 + 0.15 * std::sin(2 * TAU_R * c.rgMotPh); break;                                                    // a pure whistle
                case 1: s = 0.85 * s1 + 0.3 * std::sin(3 * TAU_R * c.rgMotPh); break;                                              // hollow
                case 2: if (retune) setBand(CO_WHISTLE, f, 50, true); s = 0.8 * s1 + 4.0 * band(CO_WHISTLE, n2, c.rgMotY1, c.rgMotY2); break;   // breathy
                default: c.rgMotPh2 = wrap1(c.rgMotPh2 + dt * f * 1.414); s = s1 * (0.55 + 0.45 * std::sin(TAU_R * c.rgMotPh2)); break;   // metallic
            }
            sig += 0.42 * c.rgMotAmp * e * s;
            const int h = i + (c.rgMotRev ? 6 : 0); c.rgHang[h] = std::max(c.rgHang[h], 0.36 * c.rgMotAmp * e);   // the whistle hangs on in the plasma
            c.rgMotT += dt;
        }
    }
    // the figure's whistles and the answer's hanging on after them in their timbre (a pure one with its octave, a hollow one with its twelfth,
    // a breathy one with its octave faint, a metallic one with a bar's overtone), dying over its own 10-18 s, on through the silence fainter still
    {
        const double k = std::exp(-dt / v.rgHangS);
        for (int i = 0; i < 12; i++) if (c.rgHang[i] > 1e-4) {
            if (retune || c.rgHangF[i] <= 0) c.rgHangF[i] = v.rgNoteF[i % 6] * (i >= 6 ? v.rgAnswer : 1.0) * (1 + 0.003 * std::sin(TAU_R * (0.21 + 0.05 * i) * c.clock));
            const double f = c.rgHangF[i];
            c.rgHangPh[i] = wrap1(c.rgHangPh[i] + dt * f);
            const double p = TAU_R * c.rgHangPh[i], sp = std::sin(p);
            double x = sp;
            switch (v.rgTimbre) {
                case 0: x += 0.6 * sp * std::cos(p); break;              // its octave, 0.3 (sin 2p = 2 sin p cos p)
                case 1: x += 0.35 * (3 * sp - 4 * sp * sp * sp); break;  // its twelfth (sin 3p)
                case 3: c.rgHangPh2[i] = wrap1(c.rgHangPh2[i] + dt * f * RING[1]); x += 0.35 * std::sin(TAU_R * c.rgHangPh2[i]); break;
                default: x += 0.4 * sp * std::cos(p); break;             // its octave, 0.2
            }
            sig += c.rgHang[i] * (0.25 + 0.75 * env) * x;
            c.rgHang[i] *= k;
        }
    }
    // detuned drones, beating: its chord, each note against its twin, a breath of themselves in the silence (each a phasor turned a sample's
    // angle; brought back to unit length at each retune)
    {
        const double gate = 0.06 + 0.94 * env;
        double d = 0;
        for (int i = 0; i < 2 * v.rgChordN; i++) {
            const double cn = c.rgDrC[i] * c.rgDrRc[i] - c.rgDrS[i] * c.rgDrRs[i], sn = c.rgDrS[i] * c.rgDrRc[i] + c.rgDrC[i] * c.rgDrRs[i];
            c.rgDrC[i] = cn; c.rgDrS[i] = sn;
            if (retune) { const double g = 1.5 - 0.5 * (cn * cn + sn * sn); c.rgDrC[i] *= g; c.rgDrS[i] *= g; }
            const double c2 = 2 * c.rgDrC[i];
            double sPrev = 0, sCur = c.rgDrS[i], x = v.rgPart[0] * sCur;
            for (int h = 1; h < 6; h++) { const double sNext = c2 * sCur - sPrev; sPrev = sCur; sCur = sNext; x += v.rgPart[h] * sNext; }   // sin((h + 1) p) by the recurrence
            d += x;
        }
        sig += v.rgW[RW_DRONES] * v.rgDroneAmp * gate * d;
    }
    // plasma vowels: the moan (a voice's source through formants sweeping through its three vowels, sagging as the beam leaves) and the
    // plasma's breath (noise through two resonances wandering between the same vowels at a pace of their own)
    {
        Phantom& p = c.phan[0];
        if (p.f0Base != v.rgMoanF) { p = Phantom(); p.f0Base = v.rgMoanF; p.f0 = p.f0T = v.rgMoanF; }
        const double s = std::fmod(c.clock * v.rgMoanRate, 3.0); const int a = (int)s; const double m = smooth(s - a);
        const double* A = PLASMA[v.rgVowel[a]]; const double* B = PLASMA[v.rgVowel[(a + 1) % 3]];
        for (int i = 0; i < 3; i++) p.FT[i] = A[i] + (B[i] - A[i]) * m;
        p.f0T = v.rgMoanF * (1 + 0.05 * std::sin(TAU_R * 0.043 * c.clock) - 0.08 * (1 - env));
        p.ampT = 0.9 * env * std::sqrt(env);
        sig += 0.65 * v.rgW[RW_MOAN] * phantomSample(p, dt, 0, 0.45, 0.55);
        const double s2 = std::fmod(c.clock * v.rgPlasmaRate + 1.5, 3.0); const int a2 = (int)s2; const double m2 = smooth(s2 - a2);
        const double* C = PLASMA[v.rgVowel[(a2 + 2) % 3]]; const double* D = PLASMA[v.rgVowel[a2]];
        const double F1 = C[0] + (D[0] - C[0]) * m2, F2 = C[1] + (D[1] - C[1]) * m2;
        if (retune) { setBand(CO_PLASMA1, F1, 70, true); setBand(CO_PLASMA2, F2, 110, true); }
        sig += 0.5 * v.rgW[RW_PLASMA] * env * (band(CO_PLASMA1, n2, c.rgPlY[0], c.rgPlY[1]) + 0.8 * band(CO_PLASMA2, n2, c.rgPlY[2], c.rgPlY[3]));
    }
    // the kilometric howl (Saturn's, the Earth's aurorae's): a whistled line wandering up and down its range, loudest at the lobe's heart
    auto howl = [&]() {
        if (retune) {   // its pitch wanders slowly (and trembles at 5.1 Hz): set every 32 samples
            const double w = 0.5 + 0.5 * std::sin(TAU_R * 0.071 * c.clock) * std::sin(TAU_R * 0.029 * c.clock + 1.1);
            c.rgHowlF = v.rgHowlLo * std::pow(v.rgHowlHi / v.rgHowlLo, w) * (1 + 0.008 * std::sin(TAU_R * 5.1 * c.clock));
        }
        c.rgHowlPh = wrap1(c.rgHowlPh + dt * c.rgHowlF);
        const double p = TAU_R * c.rgHowlPh, sp = std::sin(p);
        return 0.09 * v.rgW[RW_HOWL] * env * env * sp * (1 + 0.4 * std::cos(p));   // with its octave, 0.2
    };
    // the body by what it is
    if (storm) {
        const double dens = (0.6 + 0.9 * c.rgPw) * (dwarf ? 1.35 : 1.0);
        // the L-bursts: swells of surf (two at once at most) in a broad band wandering slowly, its wash under it
        c.rgSurfNext -= dt * dens;
        if (c.rgSurfNext <= 0) {
            const int k = c.rgSurfT[0] < 0 ? 0 : (c.rgSurfT[1] < 0 ? 1 : -1);
            if (k >= 0) { c.rgSurfT[k] = 0; c.rgSurfDur[k] = 1.4 + 2.2 * uni(); c.rgSurfA[k] = 0.4 + 0.6 * uni(); }
            c.rgSurfNext = 0.5 + 1.3 * uni();
        }
        double swell = 0;
        for (int k = 0; k < 2; k++) if (c.rgSurfT[k] >= 0) {
            const double u = c.rgSurfT[k] / c.rgSurfDur[k];
            if (u >= 1) { c.rgSurfT[k] = -1; continue; }
            const double e = std::sin(PI_R * std::pow(u, 0.7)); swell += c.rgSurfA[k] * e * e;
            c.rgSurfT[k] += dt;
        }
        c.rgSurfLp += (n2 - c.rgSurfLp) * 0.05;
        if (retune) setBand(CO_SURF, v.rgSurfF * (1 + 0.18 * std::sin(TAU_R * 0.11 * c.clock)), 0.45 * v.rgSurfF, true);
        body += v.rgW[RW_SURF_CHORUS] * swell * (1.5 * band(CO_SURF, n2, c.rgSurfY1, c.rgSurfY2) + 0.3 * c.rgSurfLp);
        // the S-bursts: trains of millisecond tones, each falling, the train's pitch sinking as it runs
        if (c.rgTrainT < 0) { c.rgTrainNext -= dt * dens; if (c.rgTrainNext <= 0) { c.rgTrainT = 0; c.rgTrainDur = 0.3 + 1.0 * uni(); c.rgTrainNext = 0.6 + 2.4 * uni(); c.rgBurstNext = 0; } }
        if (c.rgTrainT >= 0) {
            const double tu = c.rgTrainT / c.rgTrainDur;
            if (tu >= 1) c.rgTrainT = -1;
            else {
                c.rgBurstNext -= dt;
                if (c.rgBurstNext <= 0 && c.rgBurstT < 0) {
                    c.rgBurstT = 0; c.rgBurstDur = 0.006 + 0.02 * uni(); c.rgBurstF = v.rgSF * (0.75 + 0.5 * uni()) * (1 - 0.3 * tu); c.rgBurstA = 0.5 + 0.5 * uni();
                    c.rgBurstNext = -std::log(std::max(1e-3, uni())) / (v.rgSRate * dens);
                }
                c.rgTrainT += dt;
            }
        }
        if (c.rgBurstT >= 0) {
            const double bu = c.rgBurstT / c.rgBurstDur;
            if (bu >= 1) c.rgBurstT = -1;
            else {
                c.rgBurstPh = wrap1(c.rgBurstPh + dt * c.rgBurstF * (1 - v.rgSFall * bu));
                body += 0.3 * v.rgW[RW_CRACKLE_WHISTLERS] * c.rgBurstA * std::min(1.0, bu * 12) * std::exp(-bu * 3) * std::sin(TAU_R * c.rgBurstPh);
                c.rgBurstT += dt;
            }
        }
        // the roar, fluttering, loudest at the lobe's heart
        body += v.rgW[RW_ROAR_HISS] * (1.0 + 0.8 * c.rgPw) * env * band(CO_ROAR, n2, c.rgRoarY1, c.rgRoarY2) * (0.65 + 0.35 * std::sin(TAU_R * v.rgRoarFl * c.clock));
        if (cls == RV_GIANT && v.rgGiantHowl) sig += howl();
        // a brown dwarf's cyclotron howl as its beam comes round: rising fast, falling slower, a pulse a turning
        if (dwarf && crossed(0.25 - 0.45 * hw)) c.rgPulseT = 0;
        if (c.rgPulseT >= 0) {
            const double u = c.rgPulseT / 2.2;
            if (u >= 1) c.rgPulseT = -1;
            else {
                const double f = v.rgHowlLo * std::pow(v.rgHowlHi / v.rgHowlLo, std::sin(PI_R * std::pow(u, 0.6))) * (1 + 0.012 * std::sin(TAU_R * 6.3 * c.rgPulseT));
                c.rgPulsePh = wrap1(c.rgPulsePh + dt * f);
                sig += 0.28 * std::pow(std::sin(PI_R * u), 0.6) * (std::sin(TAU_R * c.rgPulsePh) + 0.25 * std::sin(2 * TAU_R * c.rgPulsePh));
                c.rgPulseT += dt;
            }
        }
    } else {
        const double dens = 0.5 + 1.0 * c.rgPw;
        // chorus: flocks of rising chirps, three at once at most
        if (c.rgFlockT < 0) { c.rgFlockNext -= dt * dens; if (c.rgFlockNext <= 0) { c.rgFlockT = 0; c.rgFlockDur = 0.8 + 1.7 * uni(); c.rgFlockNext = 2 + 5 * uni(); } }
        if (c.rgFlockT >= 0) {
            if (c.rgFlockT >= c.rgFlockDur) c.rgFlockT = -1;
            else {
                for (int k = 0; k < 3; k++) if (c.rgChT[k] < 0 && uni() < dt * 9) { c.rgChT[k] = 0; c.rgChF0[k] = v.rgChorusF * (0.85 + 0.35 * uni()); c.rgChLen[k] = v.rgChirp * (0.7 + 0.6 * uni()); }
                c.rgFlockT += dt;
            }
        }
        for (int k = 0; k < 3; k++) if (c.rgChT[k] >= 0) {
            const double u = c.rgChT[k] / c.rgChLen[k];
            if (u >= 1) { c.rgChT[k] = -1; continue; }
            c.rgChPh[k] = wrap1(c.rgChPh[k] + dt * c.rgChF0[k] * std::pow(v.rgChorusRise, u));
            body += 0.18 * v.rgW[RW_SURF_CHORUS] * std::sin(PI_R * u) * std::sin(TAU_R * c.rgChPh[k]);
            c.rgChT[k] += dt;
        }
        // whistlers falling from high now and then
        c.rgWhNext -= dt * dens;
        if (c.rgWhNext <= 0 && c.rgWhT < 0) { c.rgWhT = 0; c.rgWhDur = 1.2 + 1.0 * uni(); c.rgWhNext = 3 + 6 * uni(); }
        if (c.rgWhT >= 0) {
            const double u = c.rgWhT / c.rgWhDur;
            if (u >= 1) c.rgWhT = -1;
            else { c.rgWhPh = wrap1(c.rgWhPh + dt * (v.rgWhistF * std::exp(-u * 2.8) + 160)); body += 0.26 * v.rgW[RW_CRACKLE_WHISTLERS] * std::sin(PI_R * u) * std::sin(TAU_R * c.rgWhPh); c.rgWhT += dt; }
        }
        // hiss, breathing
        body += 0.45 * v.rgW[RW_ROAR_HISS] * band(CO_HISS, n2, c.rgHissY1, c.rgHissY2) * (0.55 + 0.45 * std::sin(TAU_R * 0.13 * c.clock));
        sig += howl();
        if (cls == RV_YOUNG) {   // the dynamo's rumble, breathing with the turning
            c.rgRumLp1 += (n2 - c.rgRumLp1) * 0.01; c.rgRumLp2 += (c.rgRumLp1 - c.rgRumLp2) * 0.01;
            sig += 3.2 * c.rgRumLp2 * (0.35 + 0.65 * env);
        }
        if (cls == RV_METAL) {   // the iron ringing now and then: a free bar's partials, dying over seconds
            c.rgRingNext -= dt;
            if (c.rgRingNext <= 0 && c.rgRingT < 0) { c.rgRingT = 0; c.rgRingNext = 3 + 5 * uni(); }
            if (c.rgRingT >= 0) {
                double x = 0;
                for (int i = 0; i < 4; i++) { c.rgRingPh[i] = wrap1(c.rgRingPh[i] + dt * v.rgRingF * RING[i]); x += std::sin(TAU_R * c.rgRingPh[i]) * std::exp(-c.rgRingT * (0.6 + 0.5 * i)) / (1 + 0.6 * i); }
                sig += 0.1 * x * std::min(1.0, c.rgRingT / 0.003);
                c.rgRingT += dt; if (c.rgRingT > 5) c.rgRingT = -1;
            }
        }
    }
    // its moon's flux tube: an arc swept up and down again, the body pumped, once a pulse; fainter while the beam is turned away
    double pump = 1;
    if (c.rgPulseS > 0) {
        c.rgMoonT += dt;
        if (c.rgMoonT >= c.rgPulseS) { c.rgMoonT -= c.rgPulseS; c.rgArcT = 0; }
        if (c.rgArcT >= 0) {
            const double dur = std::min(1.4, 0.55 * c.rgPulseS), u = c.rgArcT / dur;
            if (u >= 1) c.rgArcT = -1;
            else {
                const double e = std::pow(std::sin(PI_R * u), 1.5), f = v.rgArcF * (1 + 0.8 * std::sin(PI_R * u));
                c.rgArcPh = wrap1(c.rgArcPh + dt * f);
                if (retune) setBand(CO_ARC, 2 * f, 0.5 * f, false);
                sig += 0.32 * e * (0.25 + 0.75 * env) * band(CO_ARC, 2 * c.rgArcPh - 1, c.rgArcY1, c.rgArcY2);
                pump = 1 + 0.6 * e;
                c.rgArcT += dt;
            }
        }
    }
    sig += body * env * pump;
    // the whole voice through its plasma's two resonances, its loudness kept whatever their lift
    sig += v.rgResG[0] * band(CO_RES1, sig, c.rgResY[0], c.rgResY[1]) + v.rgResG[1] * band(CO_RES2, sig, c.rgResY[2], c.rgResY[3]);
    sig *= 0.8 / (1 + 0.15 * (v.rgResG[0] + v.rgResG[1]));
    // far off, worn: its top and its bottom go (under another signal a little worn too)
    const double wear = main ? rdWearLvl : 0.35;
    if (wear > 0.005) {
        c.rgWornLp += (sig - c.rgWornLp) * (1 - 0.82 * wear);
        c.rgWornHp += (c.rgWornLp - c.rgWornHp) * 0.02 * wear;
        sig = c.rgWornLp - c.rgWornHp;
    }
    if (voice >= 0) {   // a beacon under the voice (D-05's): a machine of C-16's, slowed and darkened
        double b = 0;
        machineInto(b, c, dt * 0.7, voice, n2, st, main);
        c.rgBeacLp += (b - c.rgBeacLp) * 0.22; c.rgBeacLp2 += (c.rgBeacLp - c.rgBeacLp2) * 0.22;
        sig += 0.7 * c.rgBeacLp2;
    }
    return 1.2 * std::tanh(sig / 1.2);
}
