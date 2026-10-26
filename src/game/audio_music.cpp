// H-05 (2026-10-10): the music's player. `musicTrigger` and `musicSample`, for the ages'
// instruments (peoples/music.h): every family as partials on a sine table (a sung vowel's harmonics under its formants, brass that
// brightens as it swells, struck strings dying high first, an organ's stops, a music box's tines, an electric string overdriven, two-
// operator electric keys and struck glass, a synthesiser's partials under a moving low-pass, pads, swarms of detuned tones, a field's
// long tones beating), the kits' pieces (skins, a log drum, rattles, claps and sticks, a band's drums, a drum machine, clicks and a
// sub pulse, jingles, a bullroarer), twenty-eight voices, and a limiter on the music's bus so that stacked chords never clip.
#include "audio.h"
#include "peoples/music.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr double TAUM = 6.283185307179586, PIM = 3.141592653589793;
struct SinTab { float v[4097]; SinTab() { for (int i = 0; i <= 4096; i++) v[i] = (float)std::sin(TAUM * i / 4096.0); } };
const SinTab SINT;
inline double sn(double ph) { ph -= std::floor(ph); const double x = ph * 4096.0; const int i = (int)x; const double f = x - i; return SINT.v[i] + (SINT.v[i + 1] - SINT.v[i]) * f; }
// a vowel's three formants (a man's, Hz; the singers' tract scales them) and their widths
const double VOWEL_F[5][3] = {{730, 1090, 2440}, {530, 1840, 2480}, {270, 2290, 3010}, {570, 840, 2410}, {300, 870, 2240}};
const double VOWEL_B[3] = {90, 120, 160};
inline double rnd01(uint32_t& r) { r = r * 1664525u + 1013904223u; return ((r >> 8) & 0xFFFF) / 65536.0; }
inline double rndSym(uint32_t& r) { return rnd01(r) * 2.0 - 1.0; }
// a two-pole resonator, unity at its centre (Klatt's)
inline double reso(double x, double f, double bw, double dt, double& y1, double& y2) {
    const double c = -std::exp(-2 * PIM * bw * dt), b = 2 * std::exp(-PIM * bw * dt) * std::cos(TAUM * f * dt), a = 1 - b - c;
    const double y = a * x + b * y1 + c * y2; y2 = y1; y1 = y; return y;
}
const double METAL_R[6] = {1.0, 1.342, 1.787, 2.372, 3.211, 4.271};   // a cymbal's square waves (TR-808's six)
// a voice's partials under a vowel's formants: harmonics as far as `top` Hz, a falling source, the three peaks, a nasal murmur
int vowelPartials(double f, int vowel, double tract, double tilt, double nasal, int maxK, double top, float* ratio, float* amp) {
    const int v = vowel < 0 || vowel > 4 ? 0 : vowel;
    int K = std::max(3, std::min(maxK, (int)(top / std::max(f, 30.0))));
    double sum = 0;
    for (int k = 1; k <= K; k++) {
        const double x = k * f;
        double a = 0.04;
        for (int j = 0; j < 3; j++) { const double F = VOWEL_F[v][j] * tract, B = VOWEL_B[j] * tract; a += (j == 0 ? 1.0 : (j == 1 ? 0.7 : 0.35)) / (1 + ((x - F) / B) * ((x - F) / B)); }
        a += nasal * 0.5 / (1 + ((x - 260) / 70) * ((x - 260) / 70));
        a *= std::pow((double)k, -tilt);
        ratio[k - 1] = (float)k; amp[k - 1] = (float)a; sum += a;
    }
    for (int k = 0; k < K; k++) amp[k] = (float)(amp[k] / std::max(sum, 1e-6));
    return K;
}
}

// a note onto a free voice (else the quietest): its timbre's partials laid out, a kit piece's state
void AudioSynth::musicTrigger(const Note& nt, const Tradition& T, const Ensemble& E, double bps, double dt) {
    int slot = -1; double low = 1e9;
    for (int i = 0; i < MVOICES; i++) { if (!mv[i].on) { slot = i; break; } if (mv[i].lastEnv < low) { low = mv[i].lastEnv; slot = i; } }
    MVoice& v = mv[slot]; v = MVoice(); v.on = true;
    v.rnd = (uint32_t)(0x9E3779B9u * (uint32_t)(mNext + 1)) ^ (uint32_t)((nt.degree + 977) * 2654435761u) ^ 0x5bd1e995u;
    v.vel = nt.vel;
    v.gate = std::max(0.03, nt.dur / bps - 0.02);
    if (nt.voice < 0) {   // a kit piece
        if (nt.degree < 0 || nt.degree >= (int)E.kit.size()) { v.on = false; return; }
        v.tb = &E.kit[nt.degree]; v.fam = v.tb->family; v.kind = v.tb->kind; v.f = v.tb->tone; v.gate = 0.02;
        if (v.fam == TF_DRUM && v.f <= 0) v.f = v.kind ? v.tb->ratio[1] : v.tb->ratio[0];
        if (v.fam == TF_RATTLE) { v.aux = 0; v.aux2 = 4 + (int)(rnd01(v.rnd) * 4); v.aux3 = 1; }
        return;
    }
    if (nt.voice >= (int)E.voices.size()) { v.on = false; return; }
    v.tb = &E.voices[nt.voice];
    const Timbre& tb = *v.tb;
    v.fam = tb.family; v.kind = tb.kind;
    const double bend = std::pow(2.0, nt.bend / 1200.0);
    v.f = traditionHz(T, nt.degree) * bend;
    if (nt.slideFrom != -1000) {
        v.fFrom = traditionHz(T, nt.slideFrom) * bend;
        const bool slow = v.fam == TF_SWARM || v.fam == TF_PAD || v.fam == TF_FIELD;
        v.slideT = slow ? std::min(2.5, v.gate * 0.4) : (v.fam == TF_VOICE ? std::min(0.35, v.gate * 0.4) : std::min(0.12, v.gate * 0.3));
    }
    voiceShape(v, nt.vowel, dt);
}

// C-11 (from `musicTrigger`): a melodic voice's partials laid out from its timbre (`v.tb`), its pitch (`v.f`), its velocity and its
// random stream, their amplitudes and phases set (a sung note's vowel, -1 the voice's own); a piece's note and an instrument's note alike
void AudioSynth::voiceShape(MVoice& v, int sungVowel, double dt) {
    const Timbre& tb = *v.tb;
    const double f = v.f;
    auto detuned = [&](int c, int copies) { if (copies <= 1) return 1.0; const double off[3] = {0.0, 0.5, -0.5}; return std::pow(2.0, tb.detune * off[c % 3] / 1200.0); };
    for (int k = 0; k < 32; k++) v.dec[k] = 1;
    switch (v.fam) {
        case TF_PLUCKED: case TF_PIPE: case TF_BOWED: case TF_METAL: case TF_REED: {   // C-04's six partials, the velocity opening the upper ones; a section's copies
            const int copies = std::max(1, std::min(3, tb.copies));
            const double bright = 0.55 + 0.45 * v.vel;
            for (int c = 0; c < copies; c++) for (int k = 0; k < 6; k++) {
                const int i = c * 6 + k;
                v.ratio[i] = (float)(tb.ratio[k] * detuned(c, copies)); v.base[i] = (float)(tb.amp[k] * (k == 0 ? 1.0 : bright) / copies);
                if (f * tb.ratio[k] > 9000) v.base[i] = 0;
            }
            v.np = copies * 6;
            break;
        }
        case TF_VOICE: case TF_SYNVOICE: {   // harmonics under the vowel's formants; a chorus's two copies a little apart
            const int copies = std::max(1, std::min(2, tb.copies)), vowel = sungVowel >= 0 ? sungVowel : 0;
            float r[16], a[16];
            const int K = vowelPartials(f, vowel, tb.tract * (f > 300 ? 1.12 : 1.0), v.fam == TF_SYNVOICE ? 0.25 : 0.55, tb.mod, 16, v.fam == TF_SYNVOICE ? 5200 : 4200, r, a);
            for (int c = 0; c < copies; c++) for (int k = 0; k < K; k++) { const int i = c * K + k; v.ratio[i] = (float)(r[k] * detuned(c, copies)); v.base[i] = (float)(a[k] / copies); }
            v.np = copies * K;
            break;
        }
        case TF_BRASS: {
            const int K = std::max(3, std::min(14, (int)(4800 / f)));
            double sum = 0; for (int k = 1; k <= K; k++) sum += std::pow((double)k, -0.8);
            for (int k = 1; k <= K; k++) { v.ratio[k - 1] = (float)k; v.base[k - 1] = (float)(std::pow((double)k, -0.8) / sum); }
            v.np = K;
            break;
        }
        case TF_KEYS: {   // two strings a key; the partials stretched by the strings' stiffness, the high ones dying first, the low notes longer
            const int K = std::max(3, std::min(10, (int)(5200 / f)));
            const double bright = 0.55 + 0.45 * v.vel;
            double sum = 0; for (int k = 1; k <= K; k++) sum += std::pow((double)k, -1.15) * std::pow(bright, k - 1);
            for (int c = 0; c < 2; c++) for (int k = 1; k <= K; k++) {
                const int i = c * K + k - 1;
                v.ratio[i] = (float)(k * std::sqrt(1 + tb.index * k * k) * detuned(c, 2));
                v.base[i] = (float)(std::pow((double)k, -1.15) * std::pow(bright, k - 1) / sum * 0.5);
                const double tau = tb.decay * std::pow(261.0 / f, 0.3) / (1 + 0.45 * (k - 1));
                v.dec[i] = (float)std::exp(-dt / std::max(0.05, tau));
            }
            v.np = 2 * K;
            break;
        }
        case TF_ORGAN: {
            double sum = 0; int i = 0;
            for (int k = 0; k < 6; k++) { if (tb.amp[k] <= 0 || f * tb.ratio[k] > 9000 || f * tb.ratio[k] < 25) continue; v.ratio[i] = (float)tb.ratio[k]; v.base[i] = (float)tb.amp[k]; sum += tb.amp[k]; i++; }
            for (int k = 0; k < i; k++) v.base[k] = (float)(v.base[k] / std::max(sum, 1e-6));
            v.np = i;
            break;
        }
        case TF_MUSICBOX: {   // a tine: its fundamental and two partials of a cantilever, the upper ones dying fast
            const double R[3] = {1.0, tb.mod, tb.mod * 2.8}, A[3] = {1.0, tb.bright, tb.bright * 0.35}, D[3] = {1.0, 0.3, 0.12};
            int i = 0;
            for (int k = 0; k < 3; k++) { if (f * R[k] > 10000) continue; v.ratio[i] = (float)R[k]; v.base[i] = (float)A[k]; v.dec[i] = (float)std::exp(-dt / std::max(0.02, tb.decay * D[k])); i++; }
            v.np = i;
            break;
        }
        case TF_ESTRING: {
            const int copies = tb.kind == 1 ? 2 : 1, K = std::max(3, std::min(tb.kind == 2 ? 8 : 12, (int)(5000 / f)));
            const double tilt = tb.kind == 2 ? 1.6 : 1.0 + 0.6 * (1 - std::min(1.0, tb.bright));
            double sum = 0; for (int k = 1; k <= K; k++) sum += std::pow((double)k, -tilt);
            for (int c = 0; c < copies; c++) for (int k = 1; k <= K; k++) {
                const int i = c * K + k - 1;
                v.ratio[i] = (float)(k * detuned(c, copies)); v.base[i] = (float)(std::pow((double)k, -tilt) / sum / copies);
                v.dec[i] = (float)std::exp(-dt / std::max(0.05, tb.decay / (1 + 0.25 * (k - 1))));
            }
            v.np = copies * K;
            break;
        }
        case TF_SYNTH: {   // a saw's every partial, a square's odd ones, a sub's first two; a lead's two copies apart
            const int copies = tb.copies >= 2 ? 2 : 1, maxK = copies == 2 ? 10 : 18;
            int i = 0;
            for (int c = 0; c < copies; c++)
                for (int k = 1; k <= (tb.mod >= 2 ? 2 : 40) && i < 32; k++) {
                    if (tb.mod == 1 && k % 2 == 0) continue;
                    if (k * f > 9000 || (tb.mod < 2 && (k > maxK * (tb.mod == 1 ? 2 : 1)))) break;
                    v.ratio[i] = (float)(k * detuned(c, copies)); v.base[i] = (float)((tb.mod >= 2 ? (k == 1 ? 1.0 : 0.15) : 1.0 / k) / copies); i++;
                }
            v.np = i;
            break;
        }
        case TF_PAD: {
            const int K = std::max(2, std::min(8, (int)(6000 / f)));
            for (int c = 0; c < 3; c++) for (int k = 1; k <= K; k++) { const int i = c * K + k - 1; v.ratio[i] = (float)(k * detuned(c, 3)); v.base[i] = (float)(1.0 / k / 3); }
            v.np = 3 * K;
            break;
        }
        case TF_SWARM: {   // ten tones round the pitch within the swarm's spread, a share of them an octave up, each swelling on its own
            for (int i = 0; i < 12; i++) {
                const bool up = i >= 10;
                if (up && rnd01(v.rnd) > tb.bright * 2.5) { v.base[i] = 0; v.ratio[i] = 2; continue; }
                v.ratio[i] = (float)((up ? 2.0 : 1.0) * std::pow(2.0, rndSym(v.rnd) * tb.detune / 1200.0));
                v.base[i] = (float)((up ? 0.5 : 1.0) / 10.5);
                v.lfr[i] = (float)(tb.vibRate * (0.6 + 0.8 * rnd01(v.rnd))); v.lfo[i] = (float)rnd01(v.rnd);
            }
            v.np = 12;
            break;
        }
        case TF_FIELD: {   // a few partials (all, the odd ones: hollow, or a bell's: struck), twice, the second a few tenths of a hertz off: the beating
            const double R0[3] = {1, 2, 3}, R1[3] = {1, 3, 5}, R2[3] = {1, 2.76, 5.4};
            const double* R = tb.kind == 1 ? R1 : (tb.kind == 2 ? R2 : R0);
            for (int c = 0; c < 2; c++) for (int k = 0; k < 3; k++) {
                const int i = c * 3 + k;
                v.ratio[i] = (float)(R[k] * (c == 0 ? 1.0 : (f + tb.tone) / f)); v.base[i] = (float)(std::pow(tb.bright, (double)k) / 2.0);
                if (f * R[k] > 9000) v.base[i] = 0;
            }
            v.np = 6;
            break;
        }
        default: v.np = 0; break;   // the electric keys (two operators) and the bullroarer (a noise) have no partials
    }
    for (int i = 0; i < v.np; i++) { v.amp[i] = v.base[i]; v.ph[i] = rnd01(v.rnd); }
    if (v.fam == TF_ESTRING || v.fam == TF_KEYS || v.fam == TF_MUSICBOX || v.fam == TF_PLUCKED) for (int i = 0; i < v.np; i++) v.ph[i] = 0;   // a string plucked or struck starts from rest
    v.ctl = 0;
}

// one sample of a voice, its envelope and level applied; the voice ends itself
double AudioSynth::musicVoice(MVoice& v, double dt) {
    const Timbre& tb = *v.tb;
    const double t = v.t;
    double out = 0;
    if (timbrePercussive(v.fam)) {
        double env = 1;
        switch (v.fam) {
            case TF_DRUM: {   // C-04's skin: a pitch that falls as it is struck, and its noise
                const double fd = v.f * (1 + 1.5 * std::exp(-t * 40));
                v.ph[0] += dt * fd;
                v.lp += (rndSym(v.rnd) - v.lp) * (v.kind == 0 ? 0.15 : 0.5);
                env = std::min(1.0, t / 0.001) * std::exp(-t / std::max(0.03, tb.decay));
                out = sn(v.ph[0]) * (v.kind == 0 ? 1.0 : 0.5) + tb.breath * v.lp * (v.kind == 0 ? 1.0 : 1.6);
                break;
            }
            case TF_LOG: {   // a hollow log: two partials of the slot's tongue, a knock
                v.ph[0] += dt * v.f; v.ph[1] += dt * v.f * tb.mod;
                env = std::exp(-t / std::max(0.05, tb.decay));
                out = sn(v.ph[0]) + 0.4 * sn(v.ph[1]) * std::exp(-t / std::max(0.03, tb.decay * 0.4)) + (t < 0.004 ? 0.6 * rndSym(v.rnd) * (1 - t / 0.004) : 0);
                break;
            }
            case TF_RATTLE: {   // seeds: grains of high noise a few milliseconds apart
                v.aux -= dt; v.aux3 += dt;
                if (v.aux <= 0 && v.aux2 > 0) { v.aux2 -= 1; v.aux3 = 0; v.aux = tb.sweep / 5 * (0.5 + rnd01(v.rnd)); }
                const double g = std::exp(-v.aux3 / 0.004);
                const double nz = rndSym(v.rnd); v.lp += (nz - v.lp) * 0.25;
                out = reso(nz - v.lp, v.f, 3000, dt, v.y1, v.y2) * g * 1.6;
                env = 1;
                if (v.aux2 <= 0 && v.aux3 > 0.03) { v.on = false; return 0; }
                break;
            }
            case TF_CLAP: {
                if (v.kind == 0) {   // hands: three bursts and a tail
                    double g = 0; const double at[3] = {0, 0.011, 0.023};
                    for (double a : at) if (t >= a) g += std::exp(-(t - a) / 0.004);
                    if (t > 0.023) g += 0.6 * std::exp(-(t - 0.023) / std::max(0.02, tb.decay));
                    out = reso(rndSym(v.rnd), v.f, 1200, dt, v.y1, v.y2) * g * 1.4;
                    env = t > 0.6 ? 0 : 1;
                } else {   // sticks: a knock through the wood's two resonances
                    const double x = t < 0.0015 ? rndSym(v.rnd) : 0;
                    out = 3.0 * reso(x, v.f, 180, dt, v.y1, v.y2) + 1.5 * reso(x, v.f * 2.3, 300, dt, v.aux2, v.aux3);
                    env = std::exp(-t / std::max(0.02, tb.decay));
                }
                break;
            }
            case TF_BASSDRUM: {
                v.ph[0] += dt * v.f * (1 + 0.6 * std::exp(-t * 30));
                v.lp += (rndSym(v.rnd) - v.lp) * 0.1;
                env = std::min(1.0, t / 0.002) * std::exp(-t / std::max(0.05, tb.decay));
                out = sn(v.ph[0]) + 0.4 * v.lp * std::exp(-t / 0.02);
                break;
            }
            case TF_SNARE: {
                const double nz = rndSym(v.rnd); v.lp += (nz - v.lp) * 0.65; v.lp2 += (v.lp - v.lp2) * 0.08;
                v.ph[0] += dt * v.tb->tone;
                env = std::min(1.0, t / 0.001);
                out = (v.lp - v.lp2) * (v.kind == 1 ? 1.5 : 1.2) * tb.bright * std::exp(-t / std::max(0.03, tb.decay)) + 0.5 * sn(v.ph[0]) * std::exp(-t / 0.07);
                break;
            }
            case TF_CYMBAL: case TF_HAT: case TF_JINGLE: {   // metal: square waves at a cymbal's ratios and noise, high-passed
                double m = 0;
                if (v.fam != TF_HAT) for (int k = 0; k < 6; k++) { v.ph[k] += dt * v.f * METAL_R[k] * (v.fam == TF_JINGLE ? 9 : 1); m += sn(v.ph[k]) > 0 ? 1 : -1; }
                const double nz = rndSym(v.rnd);
                const double x = m / 6 * 0.6 + nz * (v.fam == TF_HAT ? 1.0 : 0.6);
                v.lp += (x - v.lp) * 0.5; const double h1 = x - v.lp; v.lp2 += (h1 - v.lp2) * 0.5; const double h2 = h1 - v.lp2;
                double g = std::exp(-t / std::max(0.01, tb.decay));
                if (v.fam == TF_JINGLE) { g = 0; const double at[3] = {0, 0.018, 0.037}; for (double a : at) if (t >= a) g += std::exp(-(t - a) / std::max(0.02, tb.decay)); g *= 0.6; }
                out = h2 * g * (v.fam == TF_CYMBAL ? 1.2 : 1.6);
                env = std::min(1.0, t / 0.0015);
                if (g < 0.002 && t > 0.05) { v.on = false; return 0; }
                break;
            }
            case TF_KICK: {   // a machine's: a sine falling from its click to its note
                const double fk = v.f + (tb.mod - v.f) * std::exp(-t / std::max(0.005, tb.sweep));
                v.ph[0] += dt * fk;
                env = std::min(1.0, t / 0.001) * std::exp(-t / std::max(0.05, tb.decay));
                out = sn(v.ph[0]) + (t < 0.002 ? 0.5 * rndSym(v.rnd) : 0);
                break;
            }
            case TF_CLICK: {
                if (v.kind == 0) { v.ph[0] += dt * v.f; out = sn(v.ph[0]); }
                else out = 4 * reso(t < 0.001 ? rndSym(v.rnd) : 0, v.f, 400, dt, v.y1, v.y2);
                env = std::exp(-t / std::max(0.002, tb.decay));
                break;
            }
            case TF_SUB: {
                v.ph[0] += dt * v.f;
                env = std::min(1.0, t / 0.004) * std::exp(-t / std::max(0.05, tb.decay));
                out = sn(v.ph[0]);
                break;
            }
            default: env = 0; break;
        }
        if (v.fam != TF_RATTLE && v.fam != TF_CYMBAL && v.fam != TF_HAT && v.fam != TF_JINGLE && t > 0.01 && env < 0.002) { v.on = false; return 0; }
        if (t > 4) { v.on = false; return 0; }
        v.lastEnv = env;
        return out * env * v.vel * tb.gain;
    }
    // the melodic families: C-04's envelope (the attack, the decay to the sustain, a linear release after the gate)
    const double a = std::max(0.001, tb.attack);
    double env = t < a ? t / a : tb.sustain + (1 - tb.sustain) * std::exp(-(t - a) / std::max(0.01, tb.decay));
    if (t >= v.gate) { const double r = (t - v.gate) / std::max(0.01, tb.release); if (r >= 1) { v.on = false; return 0; } env *= 1 - r; }
    if (tb.sustain == 0 && t > a && env < 0.002) { v.on = false; return 0; }
    v.lastEnv = env;
    // the pitch: a slide from the note before, the vibrato after a third of a second, a bone flute's breath wavering, a rough voice's jitter
    double f = v.f;
    if (v.fFrom > 0 && t < v.slideT) f = v.fFrom * std::pow(v.f / v.fFrom, t / v.slideT);
    const bool pitchVib = v.fam == TF_PIPE || v.fam == TF_BOWED || v.fam == TF_REED || v.fam == TF_VOICE || v.fam == TF_BRASS || v.fam == TF_ESTRING || v.fam == TF_SYNTH || (v.fam == TF_PLUCKED && v.kind == 0);
    if (pitchVib && tb.vibDepth > 0) f *= 1 + tb.vibDepth * std::sin(TAUM * tb.vibRate * t) * std::min(1.0, t / 0.3);
    if ((v.fam == TF_PIPE || v.fam == TF_VOICE) && tb.index > 0) {
        v.wobT -= dt;
        if (v.wobT <= 0) { v.aux = rndSym(v.rnd); v.wobT = 0.15 + 0.3 * rnd01(v.rnd); }
        v.wob += (v.aux - v.wob) * 0.0008;
        f *= 1 + (v.fam == TF_PIPE ? tb.index : 0.006 * tb.index) * v.wob;
    }
    // the control rate: the families whose partials move (brass's flare, a synth's filter, a pad's drift, a swarm's swells, a field's breath, the bow's mouth)
    if (--v.ctl <= 0) {
        v.ctl = 32;
        switch (v.fam) {
            case TF_BRASS: {   // the flare: the upper partials come in as the note swells (the colour changes, the loudness is the envelope's)
                const double lev = std::min(1.0, env * (0.55 + 0.45 * v.vel));
                double sum = 0;
                for (int k = 0; k < v.np; k++) { v.amp[k] = (float)(v.base[k] * std::pow(std::max(lev, 1e-3), 0.35 * k / std::max(0.2, tb.bright))); sum += v.amp[k]; }
                for (int k = 0; k < v.np; k++) v.amp[k] = (float)(v.amp[k] / std::max(sum, 1e-6));
                break;
            }
            case TF_SYNTH: {
                const double base = tb.kind == 0 ? 0.35 * tb.bright : (tb.kind == 1 ? 1.3 : (tb.kind == 2 ? 1.0 : 1.2));
                const double fc = f * (base + (tb.bright - base) * std::exp(-t / std::max(0.02, tb.sweep)));
                for (int k = 0; k < v.np; k++) { const double x = v.ratio[k] * f / fc; const double lp = 1 / std::sqrt(1 + x * x * x * x); const double pk = tb.index * std::exp(-(std::log(std::max(x, 1e-3))) * std::log(std::max(x, 1e-3)) * 8); v.amp[k] = (float)(v.base[k] * (lp + pk)); }
                break;
            }
            case TF_PAD: { const double fc = f * tb.bright * (1 + 0.25 * std::sin(TAUM * tb.vibRate * t)); for (int k = 0; k < v.np; k++) { const double x = v.ratio[k] * f / fc; v.amp[k] = (float)(v.base[k] / std::sqrt(1 + x * x * x * x)); } break; }
            case TF_SWARM: for (int k = 0; k < v.np; k++) { v.lfo[k] += (float)(v.lfr[k] * 32 * dt); v.amp[k] = (float)(v.base[k] * (0.55 + 0.45 * std::sin(TAUM * v.lfo[k]))); } break;
            case TF_FIELD: for (int k = 0; k < v.np; k++) v.amp[k] = (float)(v.base[k] * (0.7 + 0.3 * std::sin(TAUM * tb.vibRate * t + k * 1.7))); break;
            case TF_PLUCKED: if (v.kind == 1) { const double c = 2 + 2.5 * (0.5 + 0.5 * std::sin(TAUM * tb.vibRate * t)); for (int k = 0; k < v.np; k++) { const double d = (k % 6) + 1 - c; v.amp[k] = (float)(v.base[k] * (0.2 + 1.6 * std::exp(-d * d / 1.2)) * std::exp(-t * 0.3)); } } break;
            default: break;
        }
    }
    // the partials
    const double inc = f * dt;
    for (int k = 0; k < v.np; k++) { v.ph[k] += inc * v.ratio[k]; if (v.ph[k] > 1) v.ph[k] -= std::floor(v.ph[k]); out += v.amp[k] * sn(v.ph[k]); v.amp[k] *= v.dec[k]; }
    switch (v.fam) {
        case TF_PIPE: case TF_BOWED: case TF_REED: case TF_VOICE: if (tb.breath > 0) { v.lp += (rndSym(v.rnd) - v.lp) * 0.3; out += tb.breath * v.lp * (v.fam == TF_VOICE ? 0.5 : 1.0); } break;
        case TF_ORGAN:
            if (t < 0.045 && tb.breath > 0) { v.lp += (rndSym(v.rnd) - v.lp) * 0.4; out += tb.breath * 3 * v.lp * (1 - t / 0.045); }   // the chiff
            if (tb.vibDepth > 0) out *= 1 - tb.vibDepth * 0.5 + tb.vibDepth * 0.5 * std::sin(TAUM * tb.vibRate * t);           // a barrel organ's tremolo
            break;
        case TF_KEYS: if (t < 0.012) { v.lp += (rndSym(v.rnd) - v.lp) * 0.35; out += 0.3 * v.vel * v.lp * (1 - t / 0.012); } break;   // the hammer
        case TF_MUSICBOX: if (t < 0.002) out += 0.3 * rndSym(v.rnd); break;
        case TF_ESTRING: if (tb.index > 0) { const double d = tb.index * 0.9; out = std::tanh(d * out) / std::tanh(d) * 0.7; v.y1 += (out - v.y1) * 0.55; out = v.y1; } break;   // the overdrive, then a cabinet's roll-off (the drive's own partials past the band)
        case TF_EKEYS: {   // two operators: the modulator's index falling from the strike
            v.ph[0] += inc; v.ph[1] += inc * tb.mod;
            const double I = tb.index * (0.15 + 0.85 * std::exp(-t / std::max(0.02, tb.sweep))) * (0.6 + 0.4 * v.vel);
            out = sn(v.ph[0] + I * sn(v.ph[1]) / TAUM);
            if (tb.vibDepth > 0) out *= 1 - tb.vibDepth * 0.5 + tb.vibDepth * 0.5 * std::sin(TAUM * tb.vibRate * t);
            break;
        }
        case TF_SYNVOICE: {   // held on a coarse clock, a ring under it
            v.aux += tb.tone * dt; if (v.aux >= 1) { v.aux -= std::floor(v.aux); v.held = out; }
            out = v.held * (1 - tb.sweep + tb.sweep * std::sin(TAUM * tb.vibRate * t));
            break;
        }
        case TF_ROARER: {   // a slat whirled on a cord: noise through a band that rises and falls with each turn (C-11: an instrument's
                            // whirled faster for a higher note: `held` the note's share of the tonic, 0 in a piece)
            const double turn = std::sin(TAUM * tb.vibRate * (v.held > 0 ? v.held : 1) * t);
            const double fc = tb.tone * (v.held > 0 ? v.held : 1) * (1 + tb.vibDepth * turn);
            out = 6 * reso(rndSym(v.rnd), fc, fc / 5, dt, v.y1, v.y2) * (0.55 + 0.45 * turn * turn);
            break;
        }
        default: break;
    }
    return out * env * v.vel * tb.gain;
}

// C-04: one sample of the piece: the clock in beats (the tempo wandering by the piece's drift), the notes due, the voices; H-05: the
// ensemble of the piece's age, and a limiter on the bus (instant down, slow back up)
double AudioSynth::musicSample(double dt, AudioState& st) {
    if (!mOn || !mPiece || !st.tradition) return 0;
    const Piece& P = *mPiece; const Tradition& T = *st.tradition; const Ensemble& E = pieceEnsemble(T, P);
    if (mT == 0 && mNext == 0) { mLimEnv = 0; mLimGain = 1; }
    if (!st.piecePause) {
        double bps = P.bpm / 60.0 * (1 + P.drift * std::sin(TAUM * 0.045 * mT)) * std::max(0.1, st.pieceSpeed);   // slowed through the receiver
        mBeat += dt * bps; mT += dt;
        while (mNext < P.notes.size() && P.notes[mNext].t <= mBeat) { musicTrigger(P.notes[mNext], T, E, bps, dt); mNext++; }
    }
    double s = 0; bool any = false;
    for (int i = 0; i < MVOICES; i++) {
        MVoice& v = mv[i]; if (!v.on) continue;
        any = true;
        if (st.piecePause) continue;
        s += musicVoice(v, dt);
        v.t += dt;
    }
    if (mNext >= P.notes.size() && (!any || mBeat > P.beats + 8)) { mOn = false; st.pieceDone = true; }
    const double a = std::fabs(s);
    mLimEnv = a > mLimEnv ? a : mLimEnv * (1 - dt / 0.3);
    const double target = mLimEnv > 0.82 ? 0.82 / mLimEnv : 1.0;
    mLimGain += (target - mLimGain) * (target < mLimGain ? 0.6 : dt / 0.25);
    return s * mLimGain * st.pieceGain;
}

// C-11: a note of the instrument played in the cabin onto a voice of its own (a free one, else the quietest): its pitch, its velocity, held
// while its key is down (a note without a key for its `hold`); the bullroarer whirled faster for a higher note (its pitch over 220 Hz)
void AudioSynth::instrumentTrigger(const AudioState::InstrumentNote& h, double dt) {
    const Timbre* tb = h.timbre ? h.timbre : ivTimbre;
    if (!tb || (h.hz <= 0 && !timbrePercussive(tb->family))) return;
    int slot = -1; double low = 1e9;
    for (int i = 0; i < IVOICES; i++) { if (!iv[i].on) { slot = i; break; } if (iv[i].lastEnv < low) { low = iv[i].lastEnv; slot = i; } }
    MVoice& v = iv[slot]; v = MVoice(); v.on = true;
    v.rnd = (uint32_t)(0x9E3779B9u * (++ivCount)) ^ (uint32_t)(h.key * 2654435761u) ^ 0x2545F491u;
    v.vel = std::max(0.05, std::min(1.0, h.vel));
    v.gate = h.key ? 600.0 : std::max(0.05, h.hold);
    v.tb = tb; v.fam = tb->family; v.kind = tb->kind; v.f = h.hz;
    ivKey[slot] = h.key;
    if (timbrePercussive(v.fam)) {   // a drum struck (tuned to the key's degree, or a pad's own tone), as a piece's kit is
        v.gate = 0.02;
        if (v.f <= 0) v.f = tb->tone;
        if (v.fam == TF_DRUM && v.f <= 0) v.f = v.kind ? tb->ratio[1] : tb->ratio[0];
        if (v.fam == TF_RATTLE) { v.aux = 0; v.aux2 = 4 + (int)(rnd01(v.rnd) * 4); v.aux3 = 1; }
        return;
    }
    if (v.fam == TF_ROARER) v.held = std::max(0.25, h.hz / 220.0);
    voiceShape(v, -1, dt);
}

// C-11: one sample of the instrument: its voices, a limiter of its own (instant down, slow back up)
double AudioSynth::instrumentSample(double dt, AudioState& st) {
    double s = 0; int n = 0;
    for (int i = 0; i < IVOICES; i++) { MVoice& v = iv[i]; if (!v.on) continue; s += musicVoice(v, dt); v.t += dt; n++; }
    st.instrumentSounding = n;
    const double a = std::fabs(s);
    iLimEnv = a > iLimEnv ? a : iLimEnv * (1 - dt / 0.3);
    const double target = iLimEnv > 0.82 ? 0.82 / iLimEnv : 1.0;
    iLimGain += (target - iLimGain) * (target < iLimGain ? 0.6 : dt / 0.25);
    return s * iLimGain * st.instrumentGain;
}
