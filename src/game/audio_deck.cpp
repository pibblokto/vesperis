// R-420 (2026-10-19): the cassette deck auto-explore's tape is heard through, a front-loading deck of the 80s and 90s. Its sequences: a
// cassette put in (the eject key, the door let go on its spring, the cassette slid into the holder, its reels knocking in the shell, the
// door pushed shut onto its latch) and played (the key's click, then the mechanism's ka-chunk as the heads and the pinch roller come up
// to the tape, the capstan motor's whir and the tape's hiss before the music); a tape's end (the auto-stop throwing the key up, the motor
// running down), the cassette out and the next in; the play key alone; the stop key. A part is a few struck modes (a plastic part's
// resonances, a body's thump, a spring's ring) on a short burst of noise, a slide filtered noise sticking and slipping, the motor an
// oscillator. One deck: its parts keep their pitches from one sequence to the next, each event a little off them by the seed, and the
// hands' timing varies more, so no two changes sound alike.
#include "audio.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr double TAU2 = 6.283185307179586;

struct DeckRand {
    uint32_t s;
    explicit DeckRand(uint32_t seed) : s(seed * 2654435761u + 0x9E3779B9u) { if (!s) s = 1; for (int i = 0; i < 4; i++) next(); }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    double uni() { return (next() >> 8) / 16777216.0; }
    double around(double v, double frac) { return v * (1 + frac * (2 * uni() - 1)); }   // v, give or take that share of it
    double noise() { return 2 * uni() - 1; }
};

// the parts of a sequence and when they come, in seconds from its start
enum Part { P_KEY, P_DOOR_OPEN, P_SLIDE_IN, P_SLIDE_OUT, P_SEAT, P_DOOR_SHUT, P_PLAY, P_AUTOSTOP, P_STOPKEY };
struct DeckPlan {
    struct Ev { double t; int part; double dur; };
    std::vector<Ev> ev;
    double tapeAt = 0;                      // the tape moves (the music begins), or stops (DECK_STOP)
    double motorOn = -1, motorDown = -1;    // the motor comes up to speed; runs down
    double hissOn = -1, hissOff = -1;       // the tape's hiss before the music; its fade under it begins
    double length = 0;
};

DeckPlan deckPlan(int kind, uint32_t seed) {
    DeckRand r(seed ^ (0xDEC0u * (uint32_t)(kind + 1)));
    DeckPlan p; double t = 0;
    auto at = [&](int part, double dur = 0) { p.ev.push_back({t, part, dur}); };
    auto gap = [&](double s) { t += r.around(s, 0.15); };   // a hand's pause
    auto slideIn = [&]() { const double d = r.around(0.24, 0.15); at(P_SLIDE_IN, d); t += d; at(P_SEAT); };
    auto play = [&]() {   // the key, the mechanism 40 ms behind it, the motor, the hiss, the music
        at(P_PLAY);
        p.motorOn = t + r.around(0.05, 0.1); p.hissOn = t + r.around(0.11, 0.1);
        p.tapeAt = t + r.around(0.5, 0.08); p.hissOff = p.tapeAt + 0.45;
    };
    switch (kind) {
        case DECK_INSERT:
            at(P_KEY); t += 0.035; at(P_DOOR_OPEN); gap(0.5); slideIn(); gap(0.36); at(P_DOOR_SHUT); gap(0.45); play();
            break;
        case DECK_CHANGE:
            at(P_AUTOSTOP); p.motorDown = t + 0.01; gap(0.75);
            at(P_KEY); t += 0.035; at(P_DOOR_OPEN); gap(0.42);
            { const double d = r.around(0.2, 0.15); at(P_SLIDE_OUT, d); t += d; }
            gap(0.4); slideIn(); gap(0.36); at(P_DOOR_SHUT); gap(0.45); play();
            break;
        case DECK_PLAY:
            t = 0.02; play();
            break;
        case DECK_STOP:
            at(P_STOPKEY); p.tapeAt = t + 0.03; p.motorDown = p.tapeAt;
            break;
        default: break;
    }
    double end = t + 0.5;
    if (p.hissOff >= 0) end = std::max(end, p.hissOff + 1.3);
    if (p.motorDown >= 0) end = std::max(end, p.motorDown + 0.7);
    p.length = end;
    return p;
}

// a struck part: modes (frequency, decay time, share) on a burst of noise, the peak about `amp`; each mode a few per cent off its pitch
struct Mode { double f, tau, a; };
template <int N>
void strike(std::vector<float>& b, int sr, double t0, double amp, const Mode (&m)[N], double nz, double nzTau, DeckRand& r) {
    amp = r.around(amp, 0.15);
    double sum = 0; for (const Mode& k : m) sum += k.a;
    const double norm = 1.5 * amp / std::max(1e-9, sum);   // the modes' peaks never meet all at once
    const long i0 = std::lround(t0 * sr);
    for (const Mode& k : m) {
        const double f = r.around(k.f, 0.03), tau = r.around(k.tau, 0.15), a = norm * r.around(k.a, 0.2);
        const double w = TAU2 * f / sr, rr = std::exp(-1.0 / (tau * sr)), c = 2 * rr * std::cos(w), r2 = rr * rr;
        double y1 = a * rr * std::sin(w), y2 = 0;   // y[n] = a rr^n sin(w n), by its recurrence
        const long len = (long)(tau * 7 * sr);
        for (long n = 1; n < len; n++) {
            const long i = i0 + n;
            if (i >= (long)b.size()) break;
            if (i >= 0) b[i] += (float)y1;
            const double y = c * y1 - r2 * y2; y2 = y1; y1 = y;
        }
    }
    const long nl = (long)(nzTau * 7 * sr);   // the impact itself
    for (long n = 0; n < nl; n++) {
        const long i = i0 + n;
        if (i >= (long)b.size()) break;
        if (i >= 0) b[i] += (float)(nz * amp * std::exp(-(double)n / (nzTau * sr)) * r.noise());
    }
}

const Mode KEY_M[] = {{2600, 0.003, 1}, {4100, 0.002, 0.6}, {6200, 0.0015, 0.35}, {900, 0.006, 0.25}};                // a key's plastic under the finger
const Mode CLACK_M[] = {{1300, 0.009, 1}, {2350, 0.006, 0.7}, {3700, 0.004, 0.45}, {320, 0.022, 0.6}};               // the door let go
const Mode SEAT_M[] = {{1800, 0.005, 1}, {2900, 0.004, 0.6}, {600, 0.010, 0.5}};                                      // the cassette dropping into the holder
const Mode SHUT_M[] = {{1100, 0.010, 1}, {2000, 0.007, 0.7}, {3300, 0.005, 0.5}, {240, 0.028, 0.8}};                  // the door pushed shut
const Mode LATCH_M[] = {{3100, 0.003, 1}, {4800, 0.002, 0.5}, {1700, 0.004, 0.3}};                                    // its latch catching
const Mode THUNK_M[] = {{110, 0.040, 1}, {210, 0.025, 0.6}, {740, 0.009, 0.5}, {1900, 0.018, 0.35}, {2700, 0.012, 0.2}};   // the heads and the roller coming up
const Mode ROLLER_M[] = {{420, 0.014, 1}, {1250, 0.006, 0.5}};                                                        // the pinch roller on the capstan
const Mode POP_M[] = {{140, 0.030, 1}, {380, 0.018, 0.6}, {1500, 0.008, 0.6}, {2600, 0.010, 0.4}};                    // the auto-stop throwing the key up
const Mode RELEASE_M[] = {{160, 0.026, 1}, {520, 0.012, 0.6}, {1700, 0.007, 0.4}};                                    // the stop key letting the heads back
const Mode RATTLE_M[] = {{3200, 0.002, 1}, {5100, 0.0015, 0.5}, {1600, 0.003, 0.4}};                                  // a reel's hub knocking in the shell

// the door's spring: a ring that settles in pitch as it stops swinging
void spring(std::vector<float>& b, int sr, double t0, double amp, DeckRand& r) {
    const double f0 = r.around(520, 0.03), a = r.around(amp, 0.2);
    const long i0 = std::lround(t0 * sr), len = (long)(0.07 * 6 * sr);
    double ph1 = 0, ph2 = 0;
    for (long n = 0; n < len; n++) {
        const long i = i0 + n;
        if (i >= (long)b.size()) break;
        const double t = (double)n / sr, f = f0 * (1 + 0.12 * std::exp(-t / 0.025));
        ph1 += TAU2 * f / sr; ph2 += TAU2 * f * 2.76 / sr;
        if (i >= 0) b[i] += (float)(a * (std::exp(-t / 0.07) * std::sin(ph1) + 0.4 * std::exp(-t / 0.035) * std::sin(ph2)));
    }
}

// plastic on plastic: noise through the shell's high band and the holder's low one, swelling over the slide and stuttering as it sticks
// and slips, the reels knocking now and then
void slide(std::vector<float>& b, int sr, double t0, double dur, double amp, double fc, DeckRand& r) {
    struct BP { double b0, b2, a1, a2, x1 = 0, x2 = 0, y1 = 0, y2 = 0; };
    auto bandpass = [&](double f, double q) {   // RBJ's, unity at the centre
        const double w = TAU2 * f / sr, al = std::sin(w) / (2 * q), a0 = 1 + al;
        BP p; p.b0 = al / a0; p.b2 = -al / a0; p.a1 = -2 * std::cos(w) / a0; p.a2 = (1 - al) / a0; return p;
    };
    BP hi = bandpass(r.around(fc, 0.1), 1.4), lo = bandpass(r.around(fc * 0.3, 0.1), 1.0);
    auto run = [](BP& p, double x) { const double y = p.b0 * x + p.b2 * p.x2 - p.a1 * p.y1 - p.a2 * p.y2; p.x2 = p.x1; p.x1 = x; p.y2 = p.y1; p.y1 = y; return y; };
    const long i0 = std::lround(t0 * sr), len = (long)(dur * sr);
    const double a = r.around(amp, 0.2), kTop = 1 - std::exp(-TAU2 * 5000.0 / sr);   // the plastic's top rolled off: a scrape, not a hiss
    double grip = 0.7, gripT = 0, gripLvl = 0.7, top = 0;
    for (long n = 0; n < len; n++) {
        const long i = i0 + n;
        if (i >= (long)b.size()) break;
        const double u = (double)n / std::max(1L, len);
        if ((gripT -= 1.0 / sr) <= 0) { grip = 0.35 + 0.65 * r.uni(); gripT = 0.006 + 0.012 * r.uni(); }
        gripLvl += (grip - gripLvl) * (1 - std::exp(-1.0 / (0.002 * sr)));
        const double env = std::pow(std::sin(3.14159265 * std::pow(u, 0.7)), 1.2);
        const double x = r.noise();
        top += ((run(hi, x) + 0.5 * run(lo, x)) - top) * kTop;
        if (i >= 0) b[i] += (float)(a * env * gripLvl * top * 3.4);
    }
    const int knocks = 2 + (int)(r.uni() * 3);
    for (int k = 0; k < knocks; k++) strike(b, sr, t0 + dur * (0.15 + 0.8 * r.uni()), amp * (0.35 + 0.4 * r.uni()), RATTLE_M, 0.3, 0.0005, r);
}

// the capstan motor coming up to speed (from `on`), its whir fading under the music from `off`; or running down from `down`
void motorUp(std::vector<float>& b, int sr, double on, double off, DeckRand& r) {
    const double fm = r.around(165, 0.02), A = r.around(0.035, 0.15);
    const long i0 = std::lround(on * sr), len = (long)((off - on + 1.1) * sr);
    double ph = 0, wph = 0;
    for (long n = 0; n < len; n++) {
        const long i = i0 + n;
        if (i >= (long)b.size()) break;
        const double t = (double)n / sr, tt = on + t;
        const double f = fm * (0.3 + 0.7 * (1 - std::exp(-t / 0.05)));
        const double env = std::min(1.0, t / 0.08) * (tt < off ? 1.0 : std::max(0.0, std::cos(std::min(1.0, (tt - off) / 1.0) * 1.5707963)));
        ph += TAU2 * f / sr; wph += TAU2 * f * 18 / sr;
        const double y = std::sin(ph) + 0.6 * std::sin(2 * ph) + 0.3 * std::sin(3 * ph) + 0.15 * std::sin(4 * ph) + 0.08 * std::sin(wph);
        if (i >= 0) b[i] += (float)(A * env * (1 + 0.15 * std::sin(TAU2 * 7 * t)) * y / 2.13);
    }
}
void motorDown(std::vector<float>& b, int sr, double down, DeckRand& r) {
    const double fm = r.around(165, 0.02), A = r.around(0.03, 0.15);
    const long i0 = std::lround(down * sr), len = (long)(0.65 * sr);
    double ph = 0;
    for (long n = 0; n < len; n++) {
        const long i = i0 + n;
        if (i >= (long)b.size()) break;
        const double t = (double)n / sr, f = fm * (0.15 + 0.85 * std::exp(-t / 0.12));
        ph += TAU2 * f / sr;
        const double y = std::sin(ph) + 0.6 * std::sin(2 * ph) + 0.3 * std::sin(3 * ph) + 0.15 * std::sin(4 * ph);
        if (i >= 0) b[i] += (float)(A * std::exp(-t / 0.18) * y / 2.05);
    }
}

// the tape's hiss, from the motor's start to a fade under the music
void hiss(std::vector<float>& b, int sr, double on, double off, DeckRand& r) {
    const double A = r.around(0.02, 0.15);
    const double kh = std::exp(-TAU2 * 1800.0 / sr), kl = 1 - std::exp(-TAU2 * 7000.0 / sr);
    const long i0 = std::lround(on * sr), len = (long)((off - on + 1.25) * sr);
    double hpY = 0, hpX = 0, lpY = 0;
    for (long n = 0; n < len; n++) {
        const long i = i0 + n;
        if (i >= (long)b.size()) break;
        const double t = (double)n / sr, tt = on + t;
        const double w = r.noise();
        hpY = kh * (hpY + w - hpX); hpX = w;   // a first-order high-pass
        lpY += (hpY - lpY) * kl;
        const double env = std::min(1.0, t / 0.08) * (tt < off ? 1.0 : std::max(0.0, std::cos(std::min(1.0, (tt - off) / 1.2) * 1.5707963)));
        if (i >= 0) b[i] += (float)(A * env * lpY);
    }
}
}   // namespace

double deckTapeAt(int kind, uint32_t seed) { return deckPlan(kind, seed).tapeAt; }
double deckLength(int kind, uint32_t seed) { return deckPlan(kind, seed).length; }

std::vector<float> deckRender(int kind, uint32_t seed, int sr) {
    const DeckPlan p = deckPlan(kind, seed);
    std::vector<float> b((size_t)std::ceil(p.length * sr) + 1, 0.0f);
    DeckRand r(seed * 0x51ED27u + 7u + (uint32_t)kind);
    for (const DeckPlan::Ev& e : p.ev) {
        switch (e.part) {
            case P_KEY: strike(b, sr, e.t, 0.16, KEY_M, 0.5, 0.0006, r); break;
            case P_DOOR_OPEN:
                strike(b, sr, e.t, 0.40, CLACK_M, 0.6, 0.001, r); spring(b, sr, e.t + 0.004, 0.09, r);
                strike(b, sr, e.t + r.around(0.035, 0.3), 0.05, RATTLE_M, 0.3, 0.0005, r); strike(b, sr, e.t + r.around(0.07, 0.3), 0.04, RATTLE_M, 0.3, 0.0005, r);
                break;
            case P_SLIDE_IN: slide(b, sr, e.t, e.dur, 0.10, 2400, r); break;
            case P_SLIDE_OUT: strike(b, sr, e.t, 0.10, SEAT_M, 0.3, 0.0008, r); slide(b, sr, e.t + 0.01, e.dur, 0.09, 2100, r); break;   // lifted off the holder, drawn out
            case P_SEAT: strike(b, sr, e.t, 0.20, SEAT_M, 0.4, 0.0008, r); break;
            case P_DOOR_SHUT: strike(b, sr, e.t, 0.46, SHUT_M, 0.6, 0.001, r); strike(b, sr, e.t + r.around(0.022, 0.15), 0.17, LATCH_M, 0.4, 0.0005, r); break;
            case P_PLAY:
                strike(b, sr, e.t, 0.14, KEY_M, 0.5, 0.0006, r);
                strike(b, sr, e.t + r.around(0.04, 0.1), 0.50, THUNK_M, 0.5, 0.0012, r);
                strike(b, sr, e.t + r.around(0.09, 0.1), 0.15, ROLLER_M, 0.3, 0.0008, r);
                break;
            case P_AUTOSTOP: strike(b, sr, e.t, 0.40, POP_M, 0.5, 0.001, r); strike(b, sr, e.t + r.around(0.03, 0.15), 0.10, KEY_M, 0.4, 0.0006, r); break;
            case P_STOPKEY: strike(b, sr, e.t, 0.15, KEY_M, 0.5, 0.0006, r); strike(b, sr, e.t + 0.03, 0.34, RELEASE_M, 0.5, 0.001, r); break;
        }
    }
    if (p.motorOn >= 0) motorUp(b, sr, p.motorOn, p.hissOff, r);
    if (p.motorDown >= 0) motorDown(b, sr, p.motorDown, r);
    if (p.hissOn >= 0) hiss(b, sr, p.hissOn, p.hissOff, r);
    // the deck in a room: two early reflections
    const long d1 = std::lround(0.011 * sr), d2 = std::lround(0.019 * sr);
    for (long i = (long)b.size() - 1; i >= d1; i--) b[i] += 0.22f * b[i - d1] + (i >= d2 ? 0.12f * b[i - d2] : 0.0f);
    return b;
}
