// The radar's receiver (C-07): a signal's sound from its seed (pulsars, beacons, voices, choirs, machines, the mathematics),
// its channels through the static, the camera's speaker
#include "audio_internal.h"
#include "peoples/music.h"
#include "peoples/voice.h"
#include "peoples/signals.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

// ---- C-07 the receiver (the radar camera) ----
namespace {
inline double u01(uint64_t h) { return (h >> 11) * (1.0 / 9007199254740992.0); }
const double VOWELS[7][3] = {{700, 1200, 2600}, {500, 1900, 2600}, {300, 2300, 3000}, {450, 800, 2500}, {320, 700, 2400}, {500, 1400, 2500}, {250, 1000, 2000}};
const double CHOIRS[4][3] = {{220, 226.5, 233}, {220, 330, 440}, {220, 223, 228}, {110, 220, 221.5}};   // quarter-tones, fifths and an octave, a cluster, octaves
const double BELL_PARTIALS[6] = {1.0, 1.19, 1.56, 2.0, 2.51, 2.66};
const double CLUSTER[4] = {1.0, 1.0595, 1.1225, 0.5};   // C-16: the choir in the static, a cluster of semitones over an octave down
// C-16: the beacon that is mathematics. Term `k` of a sequence as a group of `n` pips (a count in binary: `bits` the ones, sent high;
// the harmonic series: one pip at `tone` times the beacon's pitch); false past what a beacon sends before it rests and begins again
bool mathsGroup(int kind, int k, int& n, uint32_t& bits, double& tone) {
    static const int PRIMES[9] = {2, 3, 5, 7, 11, 13, 17, 19, 23};
    static const int FIB[8] = {1, 1, 2, 3, 5, 8, 13, 21};
    static const int DIGITS[12] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8};   // a constant's (pi's), a pip a unit
    bits = 0; tone = 1;
    switch (kind) {
        case 0: if (k >= 9) return false; n = PRIMES[k]; return true;                  // the primes
        case 1: if (k >= 5) return false; n = (k + 1) * (k + 1); return true;          // the squares
        case 2: if (k >= 8) return false; n = FIB[k]; return true;                     // Fibonacci's
        case 3: if (k >= 6) return false; n = 1 << k; return true;                     // the powers of two
        case 4: if (k >= 6) return false; n = (k + 1) * (k + 2) / 2; return true;      // the triangles
        case 5: if (k >= 12) return false; n = 1; tone = k + 1; return true;           // the harmonic series, a pitch a term
        case 6: { if (k >= 15) return false; n = 0; while (((k + 1) >> n) > 0) n++; bits = (uint32_t)(k + 1); return true; }   // a count in binary
        default: if (k >= 12) return false; n = DIGITS[k]; return true;
    }
}
inline bool speakingProgramme(int p) { return p == RP_VOICE || p == RP_WHISPERS || p == RP_CHANT || p == RP_NUMBERS || p == RP_LOOP || p == RP_MUSIC || p == RP_PHONOGRAPH || p == RP_SYNTHETIC; }
}

// the signal's own parameters from its seed: no two pulsars thump alike, no two comets sing alike, no two beacons share a pattern
void AudioSynth::rvDerive(RadarVar& v, uint64_t seed) {
    auto u = [&](int k) { return u01(mix64(seed ^ ((uint64_t)k * 0x9E3779B97F4A7C15ULL) ^ 0xC07ULL)); };
    v = RadarVar();
    v.thumpF = 45 + 50 * u(1); v.interpulse = u(2) < 0.5 ? 0 : 0.3 + 0.4 * u(3); v.nulling = u(4) < 0.3; v.nullOn = 6 + 12 * u(5); v.nullOff = 2 + 4 * u(6);
    v.growl = 0.4 + 1.1 * u(7); v.whineF = 1700 + 1500 * u(8); v.breath = u(9) < 0.45;
    static const double RATIOS[5] = {1.0595, 1.03, 1.5, 0.5, 1.4142};   // a semitone, a quarter-tone, a fifth, an octave under, a tritone
    v.songF = 60 + 70 * u(11); v.ratio = RATIOS[(int)(u(12) * 4.99)]; v.glide = u(13) < 0.4; v.geyserRate = 0.5 + 1.5 * u(14); v.crackle = 0.3 + 1.7 * u(15); v.moan = u(16) < 0.4; v.moanF = 80 + 90 * u(17);
    v.whistlerRate = 0.6 + 1.4 * u(21); v.chorusRate = 0.6 + 1.4 * u(22); v.sferics = u(23) < 0.5; v.sfRate = 1 + 4 * u(24); v.choirSet = (int)(u(25) * 3.99); v.choirRate = 0.7 + 0.8 * u(26); v.hissF = 300 + 600 * u(27); v.roar = u(28) < 0.35;
    v.beacN = 3 + (int)(u(31) * 4.99); for (int i = 0; i < 7; i++) v.beacF[i] = 500 * std::pow(2.0, 1.6 * u(40 + i));
    v.beacDur = 0.12 + 0.25 * u(32); v.beacGap = 0.08 + 0.2 * u(33); v.beacRest = 2 + 5 * u(34); v.beacSq = 0.15 + 0.35 * u(35); v.beacSag = u(36) < 0.5;
    v.dataF1 = 550 + 350 * u(51); v.dataF2 = v.dataF1 + 120 + 300 * u(52); v.dataRate = 12 + 50 * u(53); v.dataBurst = 0.8 + 2.5 * u(54); v.dataGap = 0.6 + 3.5 * u(55); v.dataHum = u(56) < 0.5 ? 50 : 60;
    v.bellF = 160 + 200 * u(61); v.bellPeriod = 3 + 5 * u(62); v.bellDecay = 3 + 4 * u(63); v.bellFar = 0.1 + 0.25 * u(64);
    v.sirenTwo = u(71) < 0.45; v.sirenF = 280 + 240 * u(72); v.sirenRatio = 1.25 + 0.4 * u(73); v.sirenCycle = v.sirenTwo ? 0.4 + 0.5 * u(74) : 1.2 + 2.0 * u(74); v.sirenSwell = 9 + 11 * u(75);
    v.murRate = 3 + 1.5 * u(81); v.murF0[0] = 95 + 35 * u(82); v.murF0[1] = 130 + 50 * u(83); v.murF0[2] = 180 + 70 * u(84);
    v.sparkHz = 260 + 480 * u(101); v.ringF = 1100 + 1400 * u(102); v.quench = u(103) < 0.5; v.morseUnit = 0.055 + 0.05 * u(104); v.sparkDecay = 2200 + 2800 * u(105);   // C-16
    v.turnHz = 1.25 + 0.1 * u(111); v.hornF = 850 + 450 * u(112); v.surface = 0.5 + u(113);
    v.ringHz = 38 + 34 * u(121); v.holdHz = 4500 + 3000 * u(122); v.whineHz = 3200 + 1400 * u(131);
    v.mathKind = (int)(u(141) * 7.99); v.mathF = 330 * std::pow(2.0, 1.4 * u(142)); v.mathPip = 0.05 + 0.035 * u(143); v.mathGap = 0.07 + 0.06 * u(144); v.mathRest = 0.55 + 0.5 * u(145);
    v.carrier2F = 1200 + 300 * (u(151) - 0.5);
    rogueDerive(v, seed);   // D-03
}

const char* AudioSynth::mathsName(uint64_t seed) {
    static const char* const N[8] = {"the primes", "the squares", "Fibonacci's numbers", "the powers of two", "the triangular numbers", "the harmonic series", "a count in binary", "the digits of pi"};
    RadarVar v; rvDerive(v, seed); return N[std::max(0, std::min(7, v.mathKind))];
}

// a phantom: a voice that is no one's. A glottal pulse (or breath) through three formants; with a syllable rate it babbles on its
// own (a new vowel, loudness and pitch each syllable, a third of them silent), without one the caller moves its targets
double AudioSynth::phantomSample(Phantom& p, double dt, double sylRate, double breathy, double rough) {
    if (sylRate > 0) {
        p.sylT += dt;
        if (p.sylT >= p.sylDur) {
            p.sylT = 0; p.sylDur = (0.6 + 0.8 * (0.5 + 0.5 * noise())) / sylRate;
            int v = (int)((0.5 + 0.5 * noise()) * 6.99); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[v][i];
            p.ampT = 0.5 + 0.5 * noise() < 0.3 ? 0 : 0.5 + 0.5 * (0.5 + 0.5 * noise());
            p.f0T = p.f0Base * (0.85 + 0.3 * (0.5 + 0.5 * noise()));
        }
    }
    const double kF = co.pkF, kA = co.pkA, kP = co.pkP;
    for (int i = 0; i < 3; i++) p.F[i] += (p.FT[i] - p.F[i]) * kF;
    p.amp += (p.ampT - p.amp) * kA; p.f0 += (p.f0T - p.f0) * kP;
    p.ph += dt * p.f0 * (1 + p.jit); if (p.ph >= 1) { p.ph -= 1; p.jit = rough * 0.08 * noise(); }
    float nz = noise(); p.lpN += (nz - p.lpN) * 0.25;
    double src = glottal(p.ph) * p.amp * (1 - 0.7 * breathy) + p.lpN * breathy * p.amp * 0.6;
    double y = src;
    for (int i = 0; i < 3; i++) { double r = co.rP[i], B = 2 * r * std::cos(TAU_ * p.F[i] * dt), C = -r * r, A = 1 - B - C; double o = A * y + B * p.y1[i] + C * p.y2[i]; p.y2[i] = p.y1[i]; p.y1[i] = o; y = o; }
    return std::tanh(y * 0.5) * 0.65;
}

// a people's programme's machine into `sig` (C-07; C-16 the ages'): the beacon, data, the bell, the siren, the murmur, a chant's hum,
// whispers, a numbers station's and a loop's, the spark gap's Morse, the phonograph's surface, the beacon of mathematics. D-03: a drifter's
// beacon under its voice (D-05's) is one of them
void AudioSynth::machineInto(double& sig, RadarChannel& c, double dt, int voice, float n2, AudioState& st, bool main) {
    const RadarVar& v = c.v;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    switch (voice) {
        case RP_BEACON: {   // a beacon: its pattern of tones, a rest, again; half of them sag a little, dying
            double cyc = v.beacN * (v.beacDur + v.beacGap) + v.beacRest;
            c.beacT += dt; if (c.beacT >= cyc) c.beacT -= cyc;
            int k = (int)(c.beacT / (v.beacDur + v.beacGap)); double tk = c.beacT - k * (v.beacDur + v.beacGap);
            bool on = k < v.beacN && tk < v.beacDur;
            c.beacEnv += ((on ? 1.0 : 0.0) - c.beacEnv) * co.kBeac;
            double f = v.beacF[std::min(k, 6)] * (v.beacSag ? 0.985 + 0.015 * std::cos(TAU_ * 0.03 * c.clock) : 1.0);
            c.beacPh += dt * f; double ph = c.beacPh - std::floor(c.beacPh);
            sig += 0.24 * c.beacEnv * ((1 - v.beacSq) * std::sin(TAU_ * ph) + v.beacSq * (ph < 0.5 ? 1 : -1));
            break;
        }
        case RP_DATA: {   // data: bursts of two tones switched by a shift register, a buzz of the mains under them
            c.dataT -= dt;
            if (c.dataT <= 0) { c.dataOn = !c.dataOn; c.dataT = c.dataOn ? v.dataBurst * (0.7 + 0.6 * uni()) : v.dataGap * (0.6 + 0.8 * uni()); }
            if (c.dataOn) {
                c.dataBitT += dt * v.dataRate;
                if (c.dataBitT >= 1) { c.dataBitT -= 1; uint32_t b = ((c.dataLfsr >> 0) ^ (c.dataLfsr >> 2) ^ (c.dataLfsr >> 3) ^ (c.dataLfsr >> 5)) & 1u; c.dataLfsr = (c.dataLfsr >> 1) | (b << 15); }
                double want = (c.dataLfsr & 1u) ? v.dataF2 : v.dataF1; c.dataF += (want - c.dataF) * co.kData;
                c.dataPh += dt * c.dataF; double ph = c.dataPh - std::floor(c.dataPh);
                sig += 0.17 * (0.55 * std::sin(TAU_ * ph) + 0.45 * (ph < 0.5 ? 1 : -1));
                c.dataHumPh += dt * v.dataHum; double hp = c.dataHumPh - std::floor(c.dataHumPh);
                sig += 0.09 * (2 * hp - 1) * (0.7 + 0.3 * std::sin(TAU_ * 0.9 * c.clock));
            }
            break;
        }
        case RP_TOLL: {   // a bell tolling far off: six partials, the period a little irregular, now and then struck twice
            c.bellNext -= dt;
            if (c.bellNext <= 0) { c.bellT = 0; c.bellNext = uni() < 0.2 ? 0.6 + 0.4 * uni() : v.bellPeriod * (0.8 + 0.4 * uni()); }
            if (c.bellT >= 0) {
                double b = 0;
                for (int i = 0; i < 6; i++) { c.bellPh[i] += dt * v.bellF * BELL_PARTIALS[i] * (i == 3 ? 1.003 : 1.0); b += std::sin(TAU_ * c.bellPh[i]) * std::exp(-c.bellT * (0.7 + 0.45 * i)) / (1 + 0.3 * i); }
                b *= std::exp(-c.bellT / v.bellDecay) * (c.bellT < 0.004 ? c.bellT / 0.004 : 1.0);
                c.bellLp += (b - c.bellLp) * v.bellFar;
                sig += 0.55 * c.bellLp;
                c.bellT += dt; if (c.bellT > v.bellDecay * 3) c.bellT = -1;
            }
            break;
        }
        case RP_SIREN: {   // a siren still running: a slow glide or two tones, a sawtooth through a resonance, carried in and out as if on a wind
            c.sirT += dt;
            double f;
            if (v.sirenTwo) f = ((int)(c.sirT / v.sirenCycle) % 2) ? v.sirenF * v.sirenRatio : v.sirenF;
            else { double ph = std::fmod(c.sirT, 2 * v.sirenCycle) / v.sirenCycle; double tri = ph < 1 ? ph : 2 - ph; f = v.sirenF * std::pow(v.sirenRatio, tri); }
            c.sirPh += dt * f; double saw = 2 * (c.sirPh - std::floor(c.sirPh)) - 1;
            double o = resonate(saw, f * 3, f * 0.6, dt, c.sirY1, c.sirY2);
            c.sirSubPh += dt * f * 0.5;
            double wind = std::pow(0.5 + 0.5 * std::sin(TAU_ * c.clock / v.sirenSwell), 2.0);
            sig += (0.5 * o + 0.1 * std::sin(TAU_ * c.sirSubPh)) * (0.2 + 0.8 * wind);
            break;
        }
        case RP_MURMUR: {   // a murmur of voices behind a wall, one of them raised now and then
            double m = 0;
            for (int i = 0; i < 3; i++) { if (c.phan[i].f0Base != v.murF0[i]) { c.phan[i] = Phantom(); c.phan[i].f0Base = v.murF0[i]; c.phan[i].f0 = c.phan[i].f0T = v.murF0[i]; } m += phantomSample(c.phan[i], dt, v.murRate * (i == 0 ? 1 : (i == 1 ? 1.12 : 0.9)), 0.25, 0.3); }
            c.shoutNext -= dt;
            if (c.shoutNext <= 0 && c.shoutT < 0) { c.shoutT = 0; c.shoutNext = 6 + 9 * uni(); c.phan[3] = Phantom(); c.phan[3].f0Base = v.murF0[2] * 1.3; c.phan[3].f0 = c.phan[3].f0T = c.phan[3].f0Base; }
            if (c.shoutT >= 0) { double u = c.shoutT / 0.7; if (u >= 1) c.shoutT = -1; else { m += 1.6 * std::sin(PI_ * u) * phantomSample(c.phan[3], dt, 6, 0.1, 0.8); c.shoutT += dt; } }
            c.murLp += (m - c.murLp) * 0.18;
            sig += 0.9 * c.murLp;
            break;
        }
        case RP_CHANT: {   // a chant: a hum of three voices an octave, a unison and a fifth round the chanter's pitch, swelling with it
            if (!main) break;
            double p = st.voice ? st.voice->pitch : 110;
            rdChantAmp += (spVoiceAmp - rdChantAmp) * co.kChant;
            double vib = 1 + 0.008 * std::sin(TAU_ * 4.3 * c.clock);
            rdChantPh[0] += dt * p * 0.5 * vib; rdChantPh[1] += dt * p * 1.003 * vib; rdChantPh[2] += dt * p * 1.5 * vib;
            sig += 0.2 * rdChantAmp * (std::sin(TAU_ * rdChantPh[0]) + 0.6 * std::sin(TAU_ * rdChantPh[1]) + 0.4 * std::sin(TAU_ * rdChantPh[2]));
            break;
        }
        case RP_WHISPERS: {   // whispers: someone breathing close to the microphone
            c.whBr += (n2 - c.whBr) * 0.06;
            double env = std::pow(std::max(0.0, std::sin(TAU_ * 0.28 * c.clock)), 2.5);
            sig += 0.5 * env * c.whBr;
            break;
        }
        case RP_NUMBERS: {   // a numbers station: a carrier with the mains on it
            c.numPh += dt * 400; sig += 0.05 * std::sin(TAU_ * c.numPh) * (0.75 + 0.25 * std::sin(TAU_ * 50 * c.clock));
            break;
        }
        case RP_LOOP: {   // a loop: the mechanism's whir while the phrase runs, tape hiss under
            c.loopPh += dt * 95; c.sparkLp += (n2 - c.sparkLp) * 0.6;
            sig += (main && spOn ? 0.05 : 0.0) * (2 * (c.loopPh - std::floor(c.loopPh)) - 1) + 0.05 * c.sparkLp;
            break;
        }
        case RP_MORSE: {   // C-16: spark-gap Morse. A spark at the gap's rate, each a damped ring and a crack, keyed in the people's code:
            // the game's keying for the centred signal (from its top when the programme begins), a hand of the synth's own under another
            bool on = false;
            if (main && st.radarCode && !st.radarCode->empty()) {
                const std::vector<uint8_t>& code = *st.radarCode; double unitS = std::max(0.02, st.radarCodeUnit);
                if (c.codeI < code.size()) {
                    uint8_t e = code[c.codeI];
                    on = !(e & MORSE_OFF);
                    c.codeT += dt;
                    if (c.codeT >= (e & 0x7F) * unitS) { c.codeT -= (e & 0x7F) * unitS; c.codeI++; }
                }
            } else {   // letters of one to four elements, a word every three to six letters
                c.codeT += dt;
                if (c.codeT >= c.codeLeft * v.morseUnit) {
                    c.codeT = 0;
                    auto next = [&]() { c.codeLcg = c.codeLcg * 6364136223846793005ULL + 1442695040888963407ULL; return (int)(c.codeLcg >> 33); };
                    if (c.codeOn) {
                        c.codeOn = false;
                        if (--c.elemLeft > 0) c.codeLeft = 1;
                        else if (--c.letterLeft > 0) { c.codeLeft = 3; c.elemLeft = 1 + next() % 4; }
                        else { c.codeLeft = 7; c.letterLeft = 3 + next() % 4; c.elemLeft = 1 + next() % 4; }
                    } else { c.codeOn = true; c.codeLeft = (next() & 1) ? 3 : 1; }
                }
                on = c.codeOn;
            }
            c.codeEnv += ((on ? 1.0 : 0.0) - c.codeEnv) * co.kCode;
            c.sparkT += dt;
            double period = 1.0 / v.sparkHz;
            if (c.sparkT >= period) { c.sparkT -= period * (v.quench ? 1.0 : 0.92 + 0.16 * uni()); c.sparkPh = 0; }   // a quenched gap's sparks even (a note), a plain one's ragged
            c.sparkPh += dt * v.ringF;
            if (c.codeEnv > 0.001) {
                double ring = std::exp(-c.sparkT * (v.quench ? 0.5 : 1.0) * v.sparkDecay) * std::sin(TAU_ * c.sparkPh);
                double crack = n2 * std::exp(-c.sparkT * v.sparkDecay * 3);
                sig += 0.6 * c.codeEnv * (ring + (v.quench ? 0.25 : 0.7) * crack);
            }
            break;
        }
        case RP_PHONOGRAPH: {   // C-16: the record's surface under the recording: a hiss, a crackle, a tick at every turn, the motor
            c.phonoPh += dt * v.turnHz;
            if (c.phonoPh >= 1) { c.phonoPh -= 1; c.phonoClickT = 0; }
            if (c.phonoClickT >= 0) { sig += 0.5 * v.surface * n2 * std::exp(-c.phonoClickT * 700); c.phonoClickT += dt; if (c.phonoClickT > 0.012) c.phonoClickT = -1; }
            if (c.cr < 0 && uni() < dt * 10 * v.surface) c.cr = 0;
            if (c.cr >= 0) { sig += 0.4 * n2 * std::exp(-c.cr * 1500); c.cr += dt; if (c.cr > 0.005) c.cr = -1; }
            c.phonoLp += (n2 - c.phonoLp) * 0.2;
            sig += 0.12 * v.surface * c.phonoLp + 0.05 * std::sin(TAU_ * 30 * c.clock) * (0.6 + 0.4 * std::sin(TAU_ * v.turnHz * c.clock));
            break;
        }
        case RP_MATHS: {   // C-16: the beacon that is mathematics: a group of pure pips a term, a pause, the next term; a rest, again
            c.mathT += dt;
            if (c.mathRest) { if (c.mathT >= 2.5 + 3 * v.mathRest) { c.mathRest = false; c.mathT = 0; c.mathTerm = 0; c.mathPip = 0; } break; }
            int n; uint32_t bits; double tone;
            if (!mathsGroup(v.mathKind, c.mathTerm, n, bits, tone)) { c.mathRest = true; c.mathT = 0; break; }
            double pip = v.mathKind == 5 ? 0.32 : v.mathPip, gap = v.mathKind == 5 ? 0.14 : v.mathGap;
            if (n > 8) { double s = std::max(0.45, std::min(1.0, 4.0 / (n * (pip + gap)))); pip *= s; gap *= s; }   // a long group quicker: four seconds at most
            if (c.mathPip < n) {
                double f = v.mathF * (v.mathKind == 5 ? 0.25 * tone : 1.0);
                if (v.mathKind == 6 && ((bits >> (n - 1 - c.mathPip)) & 1u)) f *= 1.5;   // a one high, a nought low
                double env = c.mathT < pip ? std::sin(PI_ * c.mathT / pip) : 0;
                c.mathPh += dt * f; c.mathPh2 += dt * f * 2;
                sig += 0.42 * env * (std::sin(TAU_ * c.mathPh) + 0.2 * std::sin(TAU_ * c.mathPh2));
                if (c.mathT >= pip + gap) { c.mathT = 0; c.mathPip++; }
            } else if (c.mathT >= v.mathRest) { c.mathT = 0; c.mathPip = 0; c.mathTerm++; }
            break;
        }
        default: break;
    }
}

// one sample of a signal's own sound in its channel: by its kind and its seed, and a people's carrier (by the age, C-16) and its
// programme's machine (the hum under a chant and the cue at a loop's or a numbers station's start are the centred one's: `main`).
// Under another (C-16), a people's speaking programme is the shape of a voice: one of the channel's phantoms babbling
double AudioSynth::channelSample(RadarChannel& c, double dt, int kind, int voice, double pulseHz, uint64_t seed, int age, AudioState& st, bool main) {
    if (seed != c.seed) {   // a signal of its own: its parameters, and the keying and the count from their tops
        rvDerive(c.v, seed); c.seed = seed;
        c.codeI = 0; c.codeT = 0; c.codeLcg = seed | 1; c.codeLeft = 4; c.elemLeft = 2; c.letterLeft = 4; c.codeOn = false;
        c.mathTerm = 0; c.mathPip = 0; c.mathT = 0; c.mathRest = false;
        c.rgCls = -1; c.rgPh = -1; c.rgMotT = -1; c.rgPulseT = -1; c.rgArcT = -1; c.rgMoonT = 0;   // D-03: a drifter's turning from where it is first heard
    }
    const RadarVar& v = c.v;
    float n2 = noise();
    double sig = 0;
    c.clock += dt;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    switch (kind) {
        case 0: {   // a people's transmission: the carrier, and the programme on the air
            // C-16: the carrier by the age. The modern age's is C-07's (a tone that drifts under the voice, beating slowly); the late
            // industrial age has none, only the spark's rasp, faint, between the keying; the futuristic a thin whine high up that
            // flutters as data runs under it; the far future a pure tone that never moves (the hydrogen line's 1,420, in the audio)
            switch (age) {
                case AGE_INDUSTRIAL: { c.sparkLp += (n2 - c.sparkLp) * 0.3; sig += 0.03 * c.sparkLp * (0.5 + 0.5 * std::sin(TAU_ * 0.6 * c.clock)); break; }
                case AGE_FUTURISTIC: {
                    c.digPh += dt * v.whineHz * (1 + 0.003 * std::sin(TAU_ * 0.4 * c.clock));
                    c.digT -= dt; if (c.digT <= 0) { c.digT = 0.012 + 0.03 * uni(); c.digHash = uni() < 0.5 ? 0.3 : 1.0; }
                    sig += 0.022 * c.digHash * std::sin(TAU_ * c.digPh);
                    break;
                }
                case AGE_FAR_FUTURE: { c.h2Ph += dt * 1420.0; sig += 0.02 * std::sin(TAU_ * c.h2Ph); break; }
                default: {
                    double base = main ? 1200 : v.carrier2F;
                    c.carrierF += (base + 40 * std::sin(lfo * TAU_ * 0.17) - c.carrierF) * 0.001; c.carrier += dt * c.carrierF;
                    sig += 0.04 * std::sin(TAU_ * c.carrier) * (0.6 + 0.4 * std::sin(lfo * TAU_ * 2.3));
                }
            }
            machineInto(sig, c, dt, voice, n2, st, main);
            if (!main && speakingProgramme(voice)) {   // C-16: under another, a broadcast's voice is the shape of one, babbling
                Phantom& p = c.phan[0];
                if (p.f0Base != v.murF0[1]) { p = Phantom(); p.f0Base = v.murF0[1]; p.f0 = p.f0T = p.f0Base; }
                double x = phantomSample(p, dt, v.murRate, voice == RP_WHISPERS ? 0.95 : 0.2, voice == RP_SYNTHETIC ? 0.0 : 0.3);
                if (voice == RP_SYNTHETIC) x *= 0.4 + 0.6 * std::sin(TAU_ * v.ringHz * c.clock);
                sig += 0.8 * x;
            }
            if (main && rdCueT >= 0) {   // the cue at a speech's start: a numbers station's chime (three notes falling), a loop's relay and blip
                if (voice == RP_NUMBERS) {
                    int k = (int)(rdCueT / 0.2); double tk = rdCueT - k * 0.2;
                    if (k < 3) { double f = k == 0 ? 1320 : (k == 1 ? 1100 : 880); rdCuePh += dt * f; sig += 0.2 * std::exp(-tk * 7) * std::sin(TAU_ * rdCuePh); }
                    else rdCueT = -1;
                } else if (voice == RP_LOOP) {
                    rdCueLp += (n2 - rdCueLp) * 0.3;
                    if (rdCueT < 0.012) sig += 0.9 * rdCueLp;
                    else if (rdCueT < 0.13) { rdCuePh += dt * 1000; sig += 0.12 * std::sin(TAU_ * rdCuePh); }
                    else rdCueT = -1;
                } else rdCueT = -1;
                if (rdCueT >= 0) rdCueT += dt;
            }
            break;
        }
        case 1: {   // a pulsar: a thump far down at its rate, a growl falling away after each, a thin whine that circles; by its seed an interpulse half a period on, spells of silence (a nulling one), a breath drawn before each thump (the slow ones)
            double rate = std::max(0.2, pulseHz);
            double gate = 1;
            if (v.nulling) { c.nullT += dt; double cyc = v.nullOn + v.nullOff; if (std::fmod(c.nullT, cyc) >= v.nullOn) gate = 0; }
            c.ph += dt * rate;
            if (c.ph >= 1) { c.ph -= 1; c.half = false; if (gate > 0) c.T = 0; }
            if (v.interpulse > 0 && c.ph >= 0.5 && !c.half) { c.half = true; if (gate > 0) c.T2 = 0; }
            auto thump = [&](double& T, double& ph, double& gph, double amp) {
                if (T < 0) return;
                double env = std::exp(-T * 9), f = v.thumpF * std::exp(-T * 2.5) + 28;
                ph += dt * f; sig += 0.75 * amp * env * std::sin(TAU_ * ph);
                double fg = 160 * std::exp(-T * 1.5) + 50; gph += dt * fg; double saw = 2 * (gph - std::floor(gph)) - 1;
                sig += amp * (0.16 * v.growl * std::exp(-T * 4) * saw + 0.5 * std::exp(-T * 60) * n2);
                T += dt; if (T > 1.2) T = -1;
            };
            thump(c.T, c.thumpPh, c.growlPh, 1.0);
            thump(c.T2, c.thumpPh2, c.growlPh2, v.interpulse);
            if (v.breath && rate < 1.3) { double env = c.ph > 0.55 ? std::pow((c.ph - 0.55) / 0.45, 2.0) : 0; c.breathLp += (n2 - c.breathLp) * 0.08; sig += 0.9 * env * gate * c.breathLp; }
            c.whine += dt * (v.whineF + 300 * std::sin(lfo * TAU_ * 0.8));
            sig += 0.03 * std::sin(TAU_ * c.whine) * (0.5 + 0.5 * std::sin(lfo * TAU_ * 0.33));
            break;
        }
        case 2: {   // a comet: its song (a warble far down with a second voice an interval off, by its seed), geysers of hiss, ice cracking, a rumble; some glide, some moan
            double wob = 1 + 0.12 * std::sin(lfo * TAU_ * 1.9) + 0.05 * std::sin(lfo * TAU_ * 0.27);
            double gl = v.glide ? 1 + 0.2 * std::sin(TAU_ * 0.07 * c.clock) * std::sin(TAU_ * 0.031 * c.clock + 0.7) : 1;
            c.warb += dt * v.songF * wob * gl; c.warb2 += dt * v.songF * v.ratio * wob * gl;
            double songEnv = 0.5 + 0.5 * std::sin(lfo * TAU_ * 0.09);
            sig += 0.26 * songEnv * (std::sin(TAU_ * c.warb) + 0.6 * std::sin(TAU_ * c.warb2) + 0.3 * std::sin(TAU_ * c.warb * 2));
            c.geyserNext -= dt;
            if (c.geyserNext <= 0 && c.geyser < 0) { c.geyser = 0; c.geyserNext = (2 + 4 * uni()) / v.geyserRate; }
            c.sparkLp += (n2 - c.sparkLp) * 0.6;
            if (c.geyser >= 0) { double u = c.geyser / 1.4; if (u >= 1) c.geyser = -1; else { sig += 0.7 * std::sin(PI_ * u) * c.sparkLp; c.geyser += dt; } }
            sig += 2.5 * rdBrown2;
            if (c.cr < 0 && uni() < dt * 4 * v.crackle) c.cr = 0;
            if (c.cr >= 0) { sig += 0.7 * n2 * std::exp(-c.cr * 400); c.cr += dt; if (c.cr > 0.03) c.cr = -1; }
            if (v.moan) {
                Phantom& p = c.phan[3];
                p.f0T = v.moanF * gl * (1 + 0.03 * std::sin(TAU_ * 0.13 * c.clock)); p.ampT = 0.35 + 0.35 * std::sin(TAU_ * 0.05 * c.clock);
                double m = 0.5 + 0.5 * std::sin(TAU_ * 0.09 * c.clock); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[3][i] * (1 - m) + VOWELS[0][i] * m;
                sig += 0.45 * phantomSample(p, dt, 0, 0.35, 0.5);
            }
            break;
        }
        case 4:     // D-03: a drifter's voice (audio_rogue.cpp)
            sig = rogueSample(c, dt, main ? st.radarRogue : rdRogue2, voice, n2, st, main);
            break;
        case 3: {   // a magnetosphere: whistlers falling slowly from 2 kHz, bursts of chorus, a band of hiss that breathes, and a choir of three voices (by its seed quarter-tones, fifths, a cluster or octaves) that comes and goes; some crack with lightning (sferics and tweeks), some roar
            c.whNext -= dt;
            if (c.whNext <= 0 && c.whT < 0) { c.whT = 0; c.whNext = (2.0 + 4.0 * uni()) / v.whistlerRate; }
            if (c.whT >= 0) { double u = c.whT / 1.8; if (u >= 1) c.whT = -1; else { double f = 2200 * std::exp(-u * 2.6) + 150; c.whPh += dt * f; sig += 0.45 * std::sin(PI_ * u) * std::sin(TAU_ * c.whPh); c.whT += dt; } }
            c.chNext -= dt;
            if (c.chNext <= 0 && c.chT < 0) { c.chT = 0; c.chN = 2 + (int)(3.99 * uni()); c.chNext = (4 + 6.0 * uni()) / v.chorusRate; }
            if (c.chT >= 0) { const double per = 0.26; int kk = (int)(c.chT / per); if (kk >= c.chN) c.chT = -1; else { double u = c.chT / per - kk; double f = 700 * std::pow(2.5, u); c.chPh += dt * f; sig += 0.3 * std::sin(PI_ * u) * std::sin(TAU_ * c.chPh); c.chT += dt; } }
            sig += 0.4 * resonate(n2, v.hissF, 120, dt, c.hissY1, c.hissY2) * (0.5 + 0.5 * std::sin(lfo * TAU_ * 0.4));
            if (v.sferics) {
                if (c.sfT < 0 && uni() < dt * v.sfRate) { c.sfT = 0; c.sfPh = 0; }
                if (c.sfT >= 0) { double f = 2200 * std::exp(-c.sfT * 4) + 1500; c.sfPh += dt * f; sig += 0.9 * n2 * std::exp(-c.sfT * 600) + 0.25 * std::exp(-c.sfT * 30) * std::sin(TAU_ * c.sfPh); c.sfT += dt; if (c.sfT > 0.25) c.sfT = -1; }
            }
            if (v.roar) {
                c.roarNext -= dt;
                if (c.roarNext <= 0 && c.roarT < 0) { c.roarT = 0; c.roarNext = 6 + 8 * uni(); }
                if (c.roarT >= 0) { double u = c.roarT / 1.6; if (u >= 1) c.roarT = -1; else { sig += 0.6 * std::sin(PI_ * u) * resonate(n2, 300, 150, dt, c.roarY1, c.roarY2) * (0.6 + 0.4 * std::sin(TAU_ * 28 * c.roarT)); c.roarT += dt; } }
            }
            c.choirNext -= dt;
            if (c.choirNext <= 0 && c.choirT < 0) { c.choirT = 0; c.choirNext = (8 + 6 * uni()) / v.choirRate; }
            if (c.choirT >= 0) {
                double u = c.choirT / 7.0;
                if (u >= 1) c.choirT = -1;
                else {
                    double env = std::sin(PI_ * u), vib = 1 + 0.01 * std::sin(TAU_ * 5.3 * c.choirT);
                    const double* ch = CHOIRS[v.choirSet];
                    c.choirPh[0] += dt * ch[0] * vib; c.choirPh[1] += dt * ch[1] * vib; c.choirPh[2] += dt * ch[2] * vib;
                    sig += 0.13 * env * (std::sin(TAU_ * c.choirPh[0]) + std::sin(TAU_ * c.choirPh[1]) + std::sin(TAU_ * c.choirPh[2]));
                    c.choirT += dt;
                }
            }
            break;
        }
        default: break;
    }
    return sig;
}

// one sample of the receiver: the bed of the void (and, now and then, something in it), the signal centred and (C-16) the one under
// it, the content ghosted above, the heterodyne between two carriers and the intermodulation's ring, the far signal worn, all
// through an echo. `rdContent` was set by the caller from the piece or the speech on the air
double AudioSynth::radarSample(double dt, AudioState& st) {
    float n2 = noise();
    double bed = 0, ev = 0, close = 0;   // C-16: `close` is heard dry (a breath at the microphone, a knock's answer), out of the echo
    rdClock += dt;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    const double gT = rdGainLvl;
    // the bed: a rumble far down that breathes, a drone of two tones a quarter-tone apart beating with their octave, a wisp of
    // resonant static wandering between 300 and 1500 Hz, and the hiss; thinned as a signal comes through. C-16: the gain lifts the
    // floor (the hiss up, a crackle of clicks, the rumble breathing harder: deeper and quicker, the whole held near C-07's level)
    rdBrown += (n2 - rdBrown) * 0.012; rdBrown2 += (rdBrown - rdBrown2) * 0.012;
    rdBreathePh += dt * 0.23 * (1 + 1.4 * gT);
    double breathe = 0.55 + 0.45 * std::sin(rdBreathePh * TAU_ * 0.07) * std::sin(rdBreathePh * TAU_ * 0.11 + 1.0);
    if (gT > 0) breathe = std::pow(breathe, 1 + 1.5 * gT) * (1 + 0.5 * gT);
    bed += 3.0 * (1 - 0.3 * gT) * rdBrown2 * breathe;
    rdDr1 += dt * 48.0; rdDr2 += dt * 50.7; rdDr3 += dt * 97.3;
    double droneEnv = 0.45 + 0.55 * std::sin(lfo * TAU_ * 0.05);
    bed += 0.08 * droneEnv * (std::sin(TAU_ * rdDr1) + std::sin(TAU_ * rdDr2) + 0.4 * std::sin(TAU_ * rdDr3));
    rdWispT += dt;
    { double wf = 800 + 550 * std::sin(rdWispT * 0.37) * std::sin(rdWispT * 0.11 + 2.0);
      bed += 1.0 * resonate(n2, wf, 30, dt, rdWispY1, rdWispY2) * (0.5 + 0.5 * std::sin(rdWispT * 0.23)); }
    rdN1 += (n2 - rdN1) * 0.6;
    double swell = 0.75 + 0.25 * std::sin(lfo * TAU_ * 0.31) * std::sin(lfo * TAU_ * 0.53);
    bed += 0.07 * (1 + 1.6 * gT) * (n2 - 0.5 * rdN1) * swell;
    if (gT > 0) {   // C-16: the crackle, clicks at up to fifty-five a second, now and then a louder one
        if (rdCrkT < 0 && uni() < dt * 55 * gT) { rdCrkT = 0; double a = uni(); rdCrkA = (0.08 + 0.4 * a * a) * (uni() < 0.08 ? 2.2 : 1.0); }
        if (rdCrkT >= 0) { bed += rdCrkA * n2 * std::exp(-rdCrkT * 2500); rdCrkT += dt; if (rdCrkT > 0.004) rdCrkT = -1; }
    }
    {   // C-16: the galaxy's own noise in the beam, a rushing that scintillates: loud toward the band and the core, faint at the poles
        double g = rdSkyLvl * (0.15 + 0.85 * gT);
        if (g > 0.002) {
            float n3 = noise();
            rdGalLp += (n3 - rdGalLp) * 0.3; rdGalHp += (rdGalLp - rdGalHp) * 0.03;
            rdScint = std::max(-0.4, std::min(0.5, rdScint + (uni() - 0.5) * dt * 6 - rdScint * dt * 0.8));
            bed += 0.5 * g * (rdGalLp - rdGalHp) * (0.7 + rdScint);
        }
    }
    if (rdGhostLvl > 0.003) {   // C-16: a false rise, the static gathering into almost a tone, wavering, then gone again
        rdGhostF += ((520 + 380 * std::sin(TAU_ * 0.13 * rdClock) * std::sin(TAU_ * 0.041 * rdClock + 1.0)) - rdGhostF) * 0.0005;
        bed += 2.6 * rdGhostLvl * resonate(n2, rdGhostF, 4 + 26 * (1 - rdGhostLvl), dt, rdGhY1, rdGhY2);
    }
    bed *= 1 - 0.6 * radarSigLvl;
    // something in the static, now and then (every twelve to forty seconds): a carrier passing, a boom far off, a moan, knocking, a
    // murmur of voices, a long breath drawn, a scream far away; quieter while a signal is clear. C-16: oftener with the gain (four to
    // thirteen seconds at x32), and at the top steps stranger ones join: speech running backwards, counting whispered in no one's
    // language, a breath close to the microphone, a knock answering a knock, a choir swelling and stopping
    rdEvNext -= dt * (1 + 2.2 * gT);
    if (rdEvNext <= 0 && rdEvT < 0) {
        if (gT > 0.7 && uni() < 0.55 * (gT - 0.7) / 0.3) rdEvKind = 7 + std::min(4, (int)(uni() * 5));
        else { double r = uni(); rdEvKind = r < 0.25 ? 0 : (r < 0.40 ? 1 : (r < 0.60 ? 2 : (r < 0.75 ? 3 : (r < 0.87 ? 4 : (r < 0.95 ? 5 : 6))))); }
        st.radarEvents++; st.radarEventKinds |= 1 << rdEvKind;
        rdEvT = 0; rdEvPh = 0; rdEvLp = 0; rdEvY1 = rdEvY2 = 0;
        Phantom* P = rdEvPhan;
        switch (rdEvKind) {
            case 0: rdEvDur = 2.2 + 1.5 * uni(); rdEvA = 1800 + 1600 * uni(); rdEvB = 4 + 6 * uni(); break;             // the carrier: from where to how fast it beats
            case 1: rdEvDur = 3.5; rdEvA = 36 + 14 * uni(); break;                                                        // the boom's pitch
            case 2: rdEvDur = 2.5 + 2.0 * uni(); rdEvA = 90 + 70 * uni(); rdEvB = 0.75 + 0.2 * uni(); P[4] = Phantom(); P[4].f0 = P[4].f0T = rdEvA; P[4].amp = 0; break;   // the moan: its pitch and where it sinks to
            case 3: rdEvN = 3 + (int)(uni() * 2.99); rdEvA = 0.35 + 0.35 * uni(); rdEvDur = rdEvN * rdEvA + 0.8; rdEvB = 90 + 60 * uni(); break;   // knocks: how many, how spaced, the wood's note
            case 4: rdEvDur = 1.5 + 1.0 * uni(); for (int i = 0; i < 3; i++) { P[i] = Phantom(); P[i].f0Base = rdCh[0].v.murF0[i] * (0.9 + 0.2 * uni()); } break;   // a murmur
            case 5: rdEvDur = 3.2; break;                                                                                  // a breath drawn
            case 6: rdEvDur = 2.4 + 0.6 * uni(); rdEvA = 150 + 60 * uni(); P[4] = Phantom(); P[4].f0 = P[4].f0T = rdEvA; break;   // a scream, far away
            case 7: rdEvDur = 1.6 + 1.2 * uni(); rdEvN = 4 + (int)(uni() * 3.99); rdEvA = 100 + 70 * uni(); rdEvB = uni(); P[0] = Phantom(); P[0].f0 = P[0].f0T = rdEvA; break;   // backwards: the syllables, the pitch
            case 8: rdEvN = 4 + (int)(uni() * 5.99); rdEvA = 0.55 + 0.25 * uni(); rdEvDur = rdEvN * rdEvA + 0.5; rdEvB = uni(); P[0] = Phantom(); break;   // counting: how far, how quick
            case 9: rdEvDur = 3.4; break;                                                                                  // a breath at the microphone
            case 10: rdEvN = 2 + (int)(uni() * 2.99); rdEvA = 0.3 + 0.3 * uni(); rdEvB = 80 + 50 * uni(); rdEvC = 1.3 + 1.4 * uni(); rdEvDur = 2 * rdEvN * rdEvA + rdEvC + 0.8; break;   // a knock, and the answer after a silence
            default: rdEvDur = 3.0 + 2.5 * uni(); rdEvA = 115 + 60 * uni(); for (int i = 0; i < 4; i++) { P[i] = Phantom(); P[i].f0 = P[i].f0T = rdEvA * CLUSTER[i]; } break;   // a choir
        }
    }
    if (rdEvT >= 0) {
        double u = rdEvT / rdEvDur;
        Phantom* P = rdEvPhan;
        if (u >= 1) { rdEvT = -1; rdEvNext = 12 + 28 * uni(); }
        else {
            switch (rdEvKind) {
                case 0: { double f = rdEvA * std::exp(-2.7 * u) + 180; rdEvPh += dt * f; ev += 0.09 * std::sin(PI_ * u) * std::sin(TAU_ * rdEvPh) * (0.7 + 0.3 * std::sin(TAU_ * rdEvB * rdEvT)); break; }
                case 1: { rdEvPh += dt * rdEvA; ev += 0.5 * std::exp(-rdEvT * 1.2) * std::sin(TAU_ * rdEvPh) + 4.0 * rdBrown2 * std::exp(-rdEvT) + 0.4 * n2 * std::exp(-rdEvT * 80); break; }
                case 2: { Phantom& p = P[4]; double env = std::pow(std::sin(PI_ * u), 0.7);
                          p.f0T = rdEvA * (1 - (1 - rdEvB) * u) * (1 + 0.02 * std::sin(TAU_ * 4.7 * rdEvT)); p.ampT = 0.8 * env;
                          double m = 0.5 + 0.5 * std::sin(TAU_ * 0.17 * rdEvT + 1.0); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[3][i] * (1 - m) + VOWELS[4][i] * m;   // o to u and back
                          ev += 0.55 * phantomSample(p, dt, 0, 0.45, 0.6); break; }
                case 3: { double tk = std::fmod(rdEvT, rdEvA); int k = (int)(rdEvT / rdEvA);
                          double x = k < rdEvN && tk < 0.012 ? n2 : 0;
                          ev += 1.4 * resonate(x, rdEvB, 25, dt, rdEvY1, rdEvY2) * (k < rdEvN ? std::exp(-tk * 18) : 0); break; }
                case 4: { double m = 0; for (int i = 0; i < 3; i++) m += phantomSample(P[i], dt, rdCh[0].v.murRate * (i == 0 ? 1 : (i == 1 ? 1.12 : 0.9)), 0.25, 0.3);
                          rdEvLp += (m - rdEvLp) * 0.18; ev += 0.4 * std::sin(PI_ * u) * rdEvLp; break; }
                case 5: { rdEvLp += (n2 - rdEvLp) * 0.09; double env = u < 0.85 ? std::pow(u / 0.85, 2.0) : 0; ev += 0.7 * env * rdEvLp; break; }
                case 6: { Phantom& p = P[4]; double env = std::pow(std::sin(PI_ * u), 0.5), g = std::pow(u, 1.5);
                          p.f0T = rdEvA * (1 + 1.6 * g); p.ampT = 0.9 * env;
                          for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[0][i] * (1 - g) + VOWELS[2][i] * g;   // a to i
                          ev += 0.35 * phantomSample(p, dt, 0, 0.15, 1.2); break; }
                case 7: {   // speech running backwards: each syllable swelling out of its own echo and cut off, the phrase's melody rising
                    Phantom& p = P[0];
                    int k = std::min(rdEvN - 1, (int)(u * rdEvN)); double su = u * rdEvN - k;
                    uint64_t h = mix64((uint64_t)(rdEvB * 4294967296.0) + (uint64_t)k * 77);
                    int v1 = (int)(u01(h) * 6.99), v2 = (int)(u01(mix64(h ^ 1)) * 6.99);
                    double m = su < 0.7 ? 0 : (su - 0.7) / 0.3;
                    for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[v1][i] * (1 - m) + VOWELS[v2][i] * m;
                    p.ampT = su < 0.93 ? 0.95 * std::pow(su / 0.93, 2.2) : 0;
                    p.f0T = rdEvA * (0.8 + 0.45 * u);
                    ev += 0.6 * phantomSample(p, dt, 0, 0.25 + 0.45 * (1 - su), 0.35);
                    break;
                }
                case 8: {   // counting whispered in no one's language: a word of two syllables for each number, evenly, the last drawn out
                    Phantom& p = P[0];
                    int k = (int)(rdEvT / rdEvA); double tk = rdEvT - k * rdEvA;
                    int syl = tk < 0.19 ? 0 : 1; double t0 = syl ? 0.19 : 0.0, len = (k == rdEvN - 1 && syl) ? 0.32 : 0.16;
                    double env = k < rdEvN && tk - t0 < len ? std::pow(std::sin(PI_ * (tk - t0) / len), 0.8) : 0;
                    int vw = (int)(u01(mix64((uint64_t)(rdEvB * 4294967296.0) ^ ((uint64_t)(k * 2 + syl + 1) * 0x9E37ULL))) * 6.99);
                    for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[vw][i];
                    p.ampT = 0.95 * env;
                    ev += 0.8 * phantomSample(p, dt, 0, 1.0, 0.0);
                    break;
                }
                case 9: {   // a breath close to the microphone: the pop and the lips, a long breath in, a held moment, out; dry
                    double t = rdEvT, x = 0;
                    if (t < 0.06) x += 0.7 * std::exp(-t * 55) * std::sin(TAU_ * 48 * t);
                    if (t > 0.13 && t < 0.135) x += 0.6 * n2;
                    if (t > 0.18 && t < 1.55) x += 1.3 * std::pow(std::sin(PI_ * (t - 0.18) / 1.37), 1.3) * resonate(n2, 1900, 1400, dt, rdEvY1, rdEvY2);
                    if (t > 1.85 && t < 3.3) { rdEvLp += (n2 - rdEvLp) * 0.15; x += 1.1 * std::pow(std::sin(PI_ * (t - 1.85) / 1.45), 0.8) * rdEvLp; }
                    close += 0.5 * x;
                    break;
                }
                case 10: {   // a knock answering a knock: the knocks far off, a silence, the same knocks again, deeper and near, dry
                    double t1 = rdEvN * rdEvA, t2 = t1 + rdEvC;
                    if (rdEvT < t1) { double tk = std::fmod(rdEvT, rdEvA); double x = tk < 0.012 ? n2 : 0; ev += 1.4 * resonate(x, rdEvB, 25, dt, rdEvY1, rdEvY2) * std::exp(-tk * 18); }
                    else if (rdEvT >= t2 && rdEvT < t2 + rdEvN * rdEvA) { double tk = std::fmod(rdEvT - t2, rdEvA); double x = tk < 0.014 ? n2 : 0; close += 1.8 * resonate(x, rdEvB * 0.78, 30, dt, rdEvY1, rdEvY2) * std::exp(-tk * 16); }
                    break;
                }
                default: {   // a choir swelling and stopping: four voices in a cluster, out of nothing to full, then cut off at once
                    double env = std::pow(u, 1.8) * std::min(1.0, (1 - u) * rdEvDur / 0.04), x = 0;
                    for (int i = 0; i < 4; i++) {
                        Phantom& p = P[i];
                        p.f0T = rdEvA * CLUSTER[i] * (1 + 0.006 * std::sin(TAU_ * (4.6 + 0.4 * i) * rdEvT + i));
                        p.ampT = p.amp = 0.8 * env;   // the stop must be a stop: no smoothing
                        for (int j = 0; j < 3; j++) p.FT[j] = VOWELS[0][j] * (1 - u) + VOWELS[3][j] * u;
                        x += phantomSample(p, dt, 0, 0.2, 0.15);
                    }
                    ev += 0.3 * x;
                    break;
                }
            }
            rdEvT += dt;
        }
    }
    ev *= 1 - 0.7 * radarSigLvl;
    close *= 1 - 0.5 * radarSigLvl;
    // the signal centred, and (C-16) the one under it bleeding in from the wide lobe (kept while it fades)
    double sig = channelSample(rdCh[0], dt, st.radarKind, st.radarVoice, st.radarPulseHz, st.radarSeed, st.radarAge, st, true);
    if (st.radarKind2 >= 0) { rdKind2 = st.radarKind2; rdVoice2 = st.radarVoice2; rdAge2 = st.radarAge2; rdSeed2 = st.radarSeed2; rdPulse2 = st.radarPulseHz2; rdRogue2 = st.radarRogue2; }
    double sig2 = rdSig2Lvl > 0.002 && rdKind2 >= 0 ? channelSample(rdCh[1], dt, rdKind2, rdVoice2, rdPulse2, rdSeed2, rdAge2, st, false) : 0;
    double main = 0.45 * sig * radarSigLvl + 0.8 * rdContent, under = 0.4 * sig2 * rdSig2Lvl, mix = 0;
    // C-16: a far signal worn by the way: deep slow fades, dropouts, and two or three echoes of itself (the paths it came by); a word
    // heard before, back out of the static, echoes as from as far
    double echoes = std::max(rdWearLvl, rdGhostWordLvl > 0.01 ? 1.0 : 0.0);
    if (rdWearLvl > 0.005) {
        rdFadeT += dt;
        double fd = 0.5 + 0.5 * std::sin(TAU_ * 0.057 * rdFadeT) * std::sin(TAU_ * 0.0213 * rdFadeT + 1.3);
        double fade = 1 - rdWearLvl * 0.9 * std::pow(1 - fd, 1.6);
        rdWDropT -= dt;
        if (rdWDropT <= 0) { bool gap = uni() < 0.18 + 0.32 * rdWearLvl; rdWDrop = gap ? 0.0 : 1.0; rdWDropT = gap ? (0.06 + 0.4 * uni()) * (1 + 1.5 * rdWearLvl) : 0.7 + 3.0 * uni(); }
        rdWDropLvl += (rdWDrop - rdWDropLvl) * 0.006;
        main *= fade * (1 - rdWearLvl * (1 - rdWDropLvl));
    }
    if (echoes > 0.005) {
        if (rdWorn.size() != 16384) { rdWorn.assign(16384, 0.0f); rdWornI = 0; }
        rdWorn[rdWornI] = (float)main;
        auto tapW = [&](double s) { size_t back = (size_t)(s / dt) % rdWorn.size(); return (double)rdWorn[(rdWornI + rdWorn.size() - back) % rdWorn.size()]; };
        main += echoes * (0.42 * tapW(0.131) + 0.27 * tapW(0.293) + 0.17 * tapW(0.467));
        rdWornI = (rdWornI + 1) % rdWorn.size();
    }
    // C-16: two peoples' carriers beat into a whistle that slides as the beam moves; at the top steps the two ring against each other
    rdHetF += (st.radarHetHz - rdHetF) * co.kHet;
    rdHetPh += dt * rdHetF;
    if (rdHetLvl > 0.002) mix += rdHetLvl * 0.075 * (std::sin(TAU_ * rdHetPh) + 0.15 * std::sin(2 * TAU_ * rdHetPh)) * (0.75 + 0.25 * std::sin(TAU_ * 5.5 * rdClock));
    if (rdImLvl > 0.002) { rdImPh += dt * (rdHetF * 1.37 + 230); mix += rdImLvl * (0.9 * main * std::sin(TAU_ * rdImPh) + 0.5 * std::tanh(5 * main) * std::tanh(5 * under)); }
    double hot = main + under + mix;
    if (rdDriveLvl > 0.002) {   // C-15: a near signal through a high gain overdrives the receiver: the signal pushed into a hard knee, buzzing and flat
        double sq = 0.22 * std::tanh(hot * 9.0);
        hot += (sq - hot) * rdDriveLvl;
    }
    double dry = bed + ev + hot;
    // the echo: a third of a second behind, half of it back (more under a chant, whispers, a murmur, a bell), dulled; the void everything is heard in
    double fb = st.radarKind == 0 ? (st.radarVoice == RP_CHANT ? 0.72 : (st.radarVoice == RP_WHISPERS ? 0.7 : (st.radarVoice == RP_MURMUR ? 0.72 : (st.radarVoice == RP_TOLL ? 0.65 : 0.5)))) : 0.5;
    if (rdEcho.size() != 8192) { rdEcho.assign(8192, 0.0f); rdEchoI = 0; }
    double e = rdEcho[rdEchoI]; rdEchoLp += (e - rdEchoLp) * 0.3;
    rdEcho[rdEchoI] = (float)(dry + rdEchoLp * fb); rdEchoI = (rdEchoI + 1) % rdEcho.size();
    const double out = radarLvl * ((dry + 0.55 * rdEchoLp) * (1 - 0.15 * gT) + close);   // C-16: the whole held near C-07's level as the floor rises
    if (st.radarSet != RADIO_SUIT) return out;
    // D-03: the suit's radio on a drifter's ground, a small set's band (about 300 Hz to 3 kHz), driven a little into its speaker
    rdSetHp += (out - rdSetHp) * (1 - std::exp(-TAU_ * 300 * dt)); const double h = out - rdSetHp;
    const double k = 1 - std::exp(-TAU_ * 3000 * dt);
    rdSetLp1 += (h - rdSetLp1) * k; rdSetLp2 += (rdSetLp1 - rdSetLp2) * k;
    return 0.5 * std::tanh(4.0 * rdSetLp2);
}
