#include "audio.h"
#include "galaxy/music.h"
#include "galaxy/voice.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr double TAU_ = 6.283185307179586, PI_ = 3.141592653589793;
// C-05: the derivative of the glottal flow over one period (phase 0..1): a rise, a sharp fall at the closure, nothing while closed
inline double glottal(double p) {
    const double Tp = 0.4, Tn = 0.16;
    if (p < Tp) return 0.4 * std::sin(PI_ * p / Tp);
    if (p < Tp + Tn) return -std::sin(PI_ * (p - Tp) / (2 * Tn));
    return 0;
}
}

// C-04: a note onto a free voice (else the quietest)
void AudioSynth::musicTrigger(const Note& nt, const Tradition& T, double bps) {
    int slot = -1; double low = 1e9;
    for (int i = 0; i < MVOICES; i++) { if (!mv[i].on) { slot = i; break; } if (mv[i].lastEnv < low) { low = mv[i].lastEnv; slot = i; } }
    MVoice& v = mv[slot]; v = MVoice(); v.on = true;
    v.gate = std::max(0.03, nt.dur / bps - 0.02);
    v.vel = nt.vel;
    if (nt.voice < 0) { v.tb = &T.drum; v.kind = nt.degree; v.f = nt.degree == 0 ? T.drum.ratio[0] : T.drum.ratio[1]; v.gate = 0.02; }
    else {
        v.tb = &T.voices[std::min<size_t>((size_t)std::max(nt.voice, 0), T.voices.size() - 1)];
        v.f = traditionHz(T, nt.degree);
        if (nt.slideFrom != -1000) v.fFrom = traditionHz(T, nt.slideFrom);
    }
}

// C-04: one sample of the piece: the clock in beats (the tempo wandering by the tradition's drift), the notes due, the voices
double AudioSynth::musicSample(double dt, AudioState& st) {
    if (!mOn || !mPiece || !st.tradition) return 0;
    const Piece& P = *mPiece; const Tradition& T = *st.tradition;
    if (!st.piecePause) {
        double bps = P.bpm / 60.0 * (1 + T.drift * std::sin(TAU_ * 0.045 * mT)) * std::max(0.1, st.pieceSpeed);   // C-07: slowed through the receiver
        mBeat += dt * bps; mT += dt;
        while (mNext < P.notes.size() && P.notes[mNext].t <= mBeat) { musicTrigger(P.notes[mNext], T, bps); mNext++; }
    }
    double s = 0; bool any = false;
    for (int i = 0; i < MVOICES; i++) {
        MVoice& v = mv[i]; if (!v.on) continue;
        any = true;
        if (st.piecePause) continue;
        const Timbre& tb = *v.tb;
        double env = v.t < tb.attack ? v.t / tb.attack : tb.sustain + (1 - tb.sustain) * std::exp(-(v.t - tb.attack) / tb.decay);
        if (v.t >= v.gate) { double r = (v.t - v.gate) / tb.release; if (r >= 1) { v.on = false; continue; } env *= 1 - r; }
        if (tb.sustain == 0 && v.t > tb.attack && env < 0.002) { v.on = false; continue; }
        v.lastEnv = env;
        double out = 0;
        if (v.kind >= 0) {   // the drum: a skin whose pitch falls as it is struck, and its noise
            double fd = v.f * (1 + 1.5 * std::exp(-v.t * 40));
            v.ph[0] += dt * fd; if (v.ph[0] > 1) v.ph[0] -= 1;
            float nz = noise(); v.lp += (nz - v.lp) * (v.kind == 0 ? 0.15 : 0.5);
            out = std::sin(TAU_ * v.ph[0]) * (v.kind == 0 ? 1.0 : 0.5) + tb.breath * v.lp * (v.kind == 0 ? 1.0 : 1.6);
        } else {
            double f = v.f;
            if (v.fFrom > 0) { double slide = std::min(0.12, v.gate * 0.3); if (v.t < slide) f = v.fFrom * std::pow(v.f / v.fFrom, v.t / slide); }
            if (tb.vibDepth > 0) f *= 1 + tb.vibDepth * std::sin(TAU_ * tb.vibRate * v.t) * std::min(1.0, v.t / 0.3);
            double bright = 0.55 + 0.45 * v.vel;
            for (int k = 0; k < 6; k++) {
                if (tb.amp[k] <= 0) continue;
                double fk = f * tb.ratio[k];
                if (fk > 9000) continue;   // above the band
                v.ph[k] += dt * fk; if (v.ph[k] > 1) v.ph[k] -= 1;
                out += tb.amp[k] * (k == 0 ? 1.0 : bright) * std::sin(TAU_ * v.ph[k]);
            }
            if (tb.breath > 0) { float nz = noise(); v.lp += (nz - v.lp) * 0.3; out += tb.breath * v.lp; }
        }
        s += out * env * v.vel * tb.gain;
        v.t += dt;
    }
    if (mNext >= P.notes.size() && (!any || mBeat > P.beats + 8)) { mOn = false; st.pieceDone = true; }
    return s * st.pieceGain;
}

// C-05: one sample of the speech: the segment due and its sound's targets (the formants, the voicing, the breath, the noise band)
// smoothed into the running values; the glottal source (jitter and shimmer by the roughness, a tremor, a second tone under);
// four formants in cascade and a nasal resonance beside; the noise through its band. A stop's closure is silence (a voice bar
// when voiced), its burst noise decaying with aspiration after, a click a short sharp burst
double AudioSynth::speechSample(double dt, AudioState& st) {
    if (!spOn || !spSpeech || !st.voice) return 0;
    const Speech& S = *spSpeech; const Voice& V = *st.voice;
    if (st.speechPause) return 0;
    while (spNext < S.segs.size() && S.segs[spNext].t <= spT) { spSeg = (int)spNext; spNext++; }
    const Segment* sg = spSeg >= 0 && spSeg < (int)S.segs.size() ? &S.segs[spSeg] : nullptr;
    if (sg && spT >= sg->t + sg->dur) sg = nullptr;
    double tf[3] = {spF[0], spF[1], spF[2]}, tVoice = 0, tBreath = 0, tNoise = 0, nzF = 0, nzBw = 0, pitch = 1, nasalAmp = 0; bool trill = false;
    if (sg && sg->phone >= 0) {
        const Phone& p = PHONES[sg->phone];
        double u = sg->dur > 1e-9 ? (spT - sg->t) / sg->dur : 1; if (u < 0) u = 0; if (u > 1) u = 1;
        tf[0] = (p.f1 + (p.f1e - p.f1) * u) * V.tract; tf[1] = (p.f2 + (p.f2e - p.f2) * u) * V.tract; tf[2] = (p.f3 + (p.f3e - p.f3) * u) * V.tract;
        pitch = sg->pitch + (sg->pitchEnd - sg->pitch) * u;
        switch (p.kind) {
            case PK_VOWEL: tVoice = sg->amp; tBreath = V.breath; nasalAmp = V.nasal; break;
            case PK_NASAL: tVoice = 0.5 * sg->amp; tBreath = V.breath * 0.4; nasalAmp = 1.0; break;
            case PK_LIQUID: tVoice = 0.7 * sg->amp; tBreath = V.breath * 0.5; break;
            case PK_GLIDE: tVoice = 0.6 * sg->amp; tBreath = V.breath * 0.5; break;
            case PK_FRIC:
                if (p.noiseF <= 0) tBreath = p.noiseAmp * sg->amp * 1.2;   // h: breath through the formants
                else { tNoise = p.noiseAmp * sg->amp; nzF = p.noiseF; nzBw = p.noiseBw; }
                if (p.voiced) tVoice = 0.45 * sg->amp;
                break;
            case PK_STOP:
                if (sg->part == 0) tVoice = p.voiced ? 0.12 * sg->amp : 0;
                else if (sg->part == 1) { double e = std::exp(-u * (p.voiced ? 6 : 3.5)); tNoise = p.noiseAmp * sg->amp * e; nzF = p.noiseF; nzBw = p.noiseBw; tBreath = p.voiced ? 0 : (0.3 + V.breath) * (1 - u); }
                else { tNoise = 1.6 * sg->amp * std::exp(-u * 10); nzF = 2200; nzBw = 1500; }
                break;
            default: break;
        }
        trill = sg->trill;
    }
    double kF = 1 - std::exp(-dt / 0.02), kUp = 1 - std::exp(-dt / 0.006), kDown = 1 - std::exp(-dt / 0.012), kN = 1 - std::exp(-dt / 0.003);
    for (int i = 0; i < 3; i++) spF[i] += (tf[i] - spF[i]) * kF;
    spVoiceAmp += (tVoice - spVoiceAmp) * (tVoice > spVoiceAmp ? kUp : kDown);
    spBreathAmp += (tBreath - spBreathAmp) * kUp;
    spNzAmp += (tNoise - spNzAmp) * kN;
    if (nzF > 0) { spNzF = nzF; spNzBw = nzBw; }
    spNasal += (nasalAmp - spNasal) * kF;
    // the source
    double f0 = V.pitch * pitch * spJit * (1 + V.tremor * 0.025 * std::sin(TAU_ * 5.5 * spT));
    spPh += dt * f0;
    if (spPh >= 1) { spPh -= 1; spJit = 1 + V.rough * 0.08 * noise(); spShim = 1 + V.rough * 0.3 * noise(); }
    double voicing = V.breath >= 0.9 ? 0.12 : 1.0;
    double src = glottal(spPh) * spShim * spVoiceAmp * voicing;
    if (V.sub > 0) { spSubPh += dt * f0 * V.subRatio; if (spSubPh >= 1) spSubPh -= 1; src += glottal(spSubPh) * V.sub * spVoiceAmp * voicing; }
    if (trill) src *= 0.55 + 0.45 * std::sin(TAU_ * spTrillPh);
    float nz = noise();
    spLpB += (nz - spLpB) * 0.25;
    double x = src + spLpB * spBreathAmp * 0.5;
    // the cascade: three formants moving and a fourth fixed, then the nasal resonance and the noise band beside
    static const double BW[4] = {80, 100, 150, 250};
    double y = x;
    for (int i = 0; i < 4; i++) {
        double f = i < 3 ? spF[i] : 3300 * V.tract;
        double r = std::exp(-PI_ * BW[i] * dt), B = 2 * r * std::cos(TAU_ * f * dt), C = -r * r, A = 1 - B - C;
        double o = A * y + B * spY1[i] + C * spY2[i];
        spY2[i] = spY1[i]; spY1[i] = o; y = o;
    }
    if (spNasal > 0.001 || std::fabs(spY1[4]) > 1e-7) { double r = std::exp(-PI_ * 120 * dt), B = 2 * r * std::cos(TAU_ * 280 * V.tract * dt), C = -r * r, A = 1 - B - C; double o = A * src + B * spY1[4] + C * spY2[4]; spY2[4] = spY1[4]; spY1[4] = o; y += 0.8 * spNasal * o; }
    if (spNzAmp > 0.0005 || std::fabs(spY1[5]) > 1e-7) { double r = std::exp(-PI_ * spNzBw * dt), B = 2 * r * std::cos(TAU_ * spNzF * dt), C = -r * r; double o = (1 - r) * nz + B * spY1[5] + C * spY2[5]; spY2[5] = spY1[5]; spY1[5] = o; y += spNzAmp * o * 1.5; }
    spTrillPh += dt * 26; if (spTrillPh >= 1) spTrillPh -= 1;
    spT += dt;
    if (spNext >= S.segs.size() && spT >= S.seconds) { spOn = false; st.speechDone = true; }
    return std::tanh(y * 0.5) * 0.65 * st.speechGain;
}

// ---- C-07 the receiver (the radar camera) ----
namespace {
inline double u01(uint64_t h) { return (h >> 11) * (1.0 / 9007199254740992.0); }
const double VOWELS[7][3] = {{700, 1200, 2600}, {500, 1900, 2600}, {300, 2300, 3000}, {450, 800, 2500}, {320, 700, 2400}, {500, 1400, 2500}, {250, 1000, 2000}};
const double CHOIRS[4][3] = {{220, 226.5, 233}, {220, 330, 440}, {220, 223, 228}, {110, 220, 221.5}};   // quarter-tones, fifths and an octave, a cluster, octaves
const double BELL_PARTIALS[6] = {1.0, 1.19, 1.56, 2.0, 2.51, 2.66};
inline double resonate(double x, double f, double bw, double dt, double& y1, double& y2) {   // Klatt's resonator, unity at the centre
    double r = std::exp(-PI_ * bw * dt), B = 2 * r * std::cos(TAU_ * f * dt), C = -r * r;
    double o = (1 - r) * x + B * y1 + C * y2; y2 = y1; y1 = o; return o;
}
}

// the signal's own parameters from its seed: no two pulsars thump alike, no two comets sing alike, no two beacons share a pattern
void AudioSynth::rvDerive(uint64_t seed) {
    rvSeed = seed;
    auto u = [&](int k) { return u01(mix64(seed ^ ((uint64_t)k * 0x9E3779B97F4A7C15ULL) ^ 0xC07ULL)); };
    RadarVar& v = rv; v = RadarVar();
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
    double kF = 1 - std::exp(-dt / 0.05), kA = 1 - std::exp(-dt / 0.03), kP = 1 - std::exp(-dt / 0.1);
    for (int i = 0; i < 3; i++) p.F[i] += (p.FT[i] - p.F[i]) * kF;
    p.amp += (p.ampT - p.amp) * kA; p.f0 += (p.f0T - p.f0) * kP;
    p.ph += dt * p.f0 * (1 + p.jit); if (p.ph >= 1) { p.ph -= 1; p.jit = rough * 0.08 * noise(); }
    float nz = noise(); p.lpN += (nz - p.lpN) * 0.25;
    double src = glottal(p.ph) * p.amp * (1 - 0.7 * breathy) + p.lpN * breathy * p.amp * 0.6;
    static const double BW[3] = {90, 110, 160};
    double y = src;
    for (int i = 0; i < 3; i++) { double r = std::exp(-PI_ * BW[i] * dt), B = 2 * r * std::cos(TAU_ * p.F[i] * dt), C = -r * r, A = 1 - B - C; double o = A * y + B * p.y1[i] + C * p.y2[i]; p.y2[i] = p.y1[i]; p.y1[i] = o; y = o; }
    return std::tanh(y * 0.5) * 0.65;
}

// one sample of the receiver: the bed of the void (and, now and then, something in it), the signal's own sound by its kind and its
// seed, a people's programme (the machines, the hum under a chant, the cue at a loop's or a numbers station's start) and the
// content ghosted above, all through an echo. `rdContent` was set by the caller from the piece or the speech on the air
double AudioSynth::radarSample(double dt, AudioState& st) {
    if (st.radarSeed != rvSeed) rvDerive(st.radarSeed);
    const RadarVar& v = rv;
    float n2 = noise();
    double bed = 0, sig = 0, ev = 0;
    rdClock += dt;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    // the bed: a rumble far down that breathes, a drone of two tones a quarter-tone apart beating with their octave, a wisp of
    // resonant static wandering between 300 and 1500 Hz, and the hiss; thinned as a signal comes through
    rdBrown += (n2 - rdBrown) * 0.012; rdBrown2 += (rdBrown - rdBrown2) * 0.012;
    double breathe = 0.55 + 0.45 * std::sin(lfo * 6.2831853 * 0.07) * std::sin(lfo * 6.2831853 * 0.11 + 1.0);
    bed += 3.0 * rdBrown2 * breathe;
    rdDr1 += dt * 48.0; rdDr2 += dt * 50.7; rdDr3 += dt * 97.3;
    double droneEnv = 0.45 + 0.55 * std::sin(lfo * 6.2831853 * 0.05);
    bed += 0.08 * droneEnv * (std::sin(6.2831853 * rdDr1) + std::sin(6.2831853 * rdDr2) + 0.4 * std::sin(6.2831853 * rdDr3));
    rdWispT += dt;
    { double wf = 800 + 550 * std::sin(rdWispT * 0.37) * std::sin(rdWispT * 0.11 + 2.0);
      bed += 1.0 * resonate(n2, wf, 30, dt, rdWispY1, rdWispY2) * (0.5 + 0.5 * std::sin(rdWispT * 0.23)); }
    rdN1 += (n2 - rdN1) * 0.6;
    double swell = 0.75 + 0.25 * std::sin(lfo * 6.2831853 * 0.31) * std::sin(lfo * 6.2831853 * 0.53);
    bed += 0.07 * (n2 - 0.5 * rdN1) * swell;
    bed *= 1 - 0.6 * radarSigLvl;
    // something in the static, now and then (every twelve to forty seconds): a carrier passing, a boom far off, a moan, knocking, a
    // murmur of voices, a long breath drawn, a scream far away; quieter while a signal is clear
    rdEvNext -= dt;
    if (rdEvNext <= 0 && rdEvT < 0) {
        double r = uni();
        rdEvKind = r < 0.25 ? 0 : (r < 0.40 ? 1 : (r < 0.60 ? 2 : (r < 0.75 ? 3 : (r < 0.87 ? 4 : (r < 0.95 ? 5 : 6)))));
        rdEvT = 0; rdEvPh = 0; rdEvLp = 0; rdEvY1 = rdEvY2 = 0;
        switch (rdEvKind) {
            case 0: rdEvDur = 2.2 + 1.5 * uni(); rdEvA = 1800 + 1600 * uni(); rdEvB = 4 + 6 * uni(); break;             // the carrier: from where to how fast it beats
            case 1: rdEvDur = 3.5; rdEvA = 36 + 14 * uni(); break;                                                        // the boom's pitch
            case 2: rdEvDur = 2.5 + 2.0 * uni(); rdEvA = 90 + 70 * uni(); rdEvB = 0.75 + 0.2 * uni(); rdPhan[4] = Phantom(); rdPhan[4].f0 = rdPhan[4].f0T = rdEvA; rdPhan[4].amp = 0; break;   // the moan: its pitch and where it sinks to
            case 3: rdEvN = 3 + (int)(uni() * 2.99); rdEvA = 0.35 + 0.35 * uni(); rdEvDur = rdEvN * rdEvA + 0.8; rdEvB = 90 + 60 * uni(); break;   // knocks: how many, how spaced, the wood's note
            case 4: rdEvDur = 1.5 + 1.0 * uni(); for (int i = 0; i < 3; i++) { rdPhan[i] = Phantom(); rdPhan[i].f0Base = v.murF0[i] * (0.9 + 0.2 * uni()); } break;   // a murmur
            case 5: rdEvDur = 3.2; break;                                                                                  // a breath drawn
            default: rdEvDur = 2.4 + 0.6 * uni(); rdEvA = 150 + 60 * uni(); rdPhan[4] = Phantom(); rdPhan[4].f0 = rdPhan[4].f0T = rdEvA; break;   // a scream, far away
        }
    }
    if (rdEvT >= 0) {
        double u = rdEvT / rdEvDur;
        if (u >= 1) { rdEvT = -1; rdEvNext = 12 + 28 * uni(); }
        else {
            switch (rdEvKind) {
                case 0: { double f = rdEvA * std::exp(-2.7 * u) + 180; rdEvPh += dt * f; ev += 0.09 * std::sin(PI_ * u) * std::sin(TAU_ * rdEvPh) * (0.7 + 0.3 * std::sin(TAU_ * rdEvB * rdEvT)); break; }
                case 1: { rdEvPh += dt * rdEvA; ev += 0.5 * std::exp(-rdEvT * 1.2) * std::sin(TAU_ * rdEvPh) + 4.0 * rdBrown2 * std::exp(-rdEvT) + 0.4 * n2 * std::exp(-rdEvT * 80); break; }
                case 2: { Phantom& p = rdPhan[4]; double env = std::pow(std::sin(PI_ * u), 0.7);
                          p.f0T = rdEvA * (1 - (1 - rdEvB) * u) * (1 + 0.02 * std::sin(TAU_ * 4.7 * rdEvT)); p.ampT = 0.8 * env;
                          double m = 0.5 + 0.5 * std::sin(TAU_ * 0.17 * rdEvT + 1.0); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[3][i] * (1 - m) + VOWELS[4][i] * m;   // o to u and back
                          ev += 0.55 * phantomSample(p, dt, 0, 0.45, 0.6); break; }
                case 3: { double tk = std::fmod(rdEvT, rdEvA); int k = (int)(rdEvT / rdEvA);
                          double x = k < rdEvN && tk < 0.012 ? n2 : 0;
                          ev += 1.4 * resonate(x, rdEvB, 25, dt, rdEvY1, rdEvY2) * (k < rdEvN ? std::exp(-tk * 18) : 0); break; }
                case 4: { double m = 0; for (int i = 0; i < 3; i++) m += phantomSample(rdPhan[i], dt, v.murRate * (i == 0 ? 1 : (i == 1 ? 1.12 : 0.9)), 0.25, 0.3);
                          rdEvLp += (m - rdEvLp) * 0.18; ev += 0.4 * std::sin(PI_ * u) * rdEvLp; break; }
                case 5: { rdEvLp += (n2 - rdEvLp) * 0.09; double env = u < 0.85 ? std::pow(u / 0.85, 2.0) : 0; ev += 0.7 * env * rdEvLp; break; }
                default: { Phantom& p = rdPhan[4]; double env = std::pow(std::sin(PI_ * u), 0.5), g = std::pow(u, 1.5);
                           p.f0T = rdEvA * (1 + 1.6 * g); p.ampT = 0.9 * env;
                           for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[0][i] * (1 - g) + VOWELS[2][i] * g;   // a to i
                           ev += 0.35 * phantomSample(p, dt, 0, 0.15, 1.2); break; }
            }
            rdEvT += dt;
        }
    }
    ev *= 1 - 0.7 * radarSigLvl;
    switch (st.radarKind) {
        case 0: {   // a people's transmission: a carrier that drifts under the voice, beating slowly; and the programme on the air
            rdCarrierF += (1200 + 40 * std::sin(lfo * 6.2831853 * 0.17) - rdCarrierF) * 0.001; rdCarrier += dt * rdCarrierF;
            sig += 0.04 * std::sin(6.2831853 * rdCarrier) * (0.6 + 0.4 * std::sin(lfo * 6.2831853 * 2.3));
            switch (st.radarVoice) {
                case 5: {   // a beacon: its pattern of tones, a rest, again; half of them sag a little, dying
                    double cyc = v.beacN * (v.beacDur + v.beacGap) + v.beacRest;
                    rdBeacT += dt; if (rdBeacT >= cyc) rdBeacT -= cyc;
                    int k = (int)(rdBeacT / (v.beacDur + v.beacGap)); double tk = rdBeacT - k * (v.beacDur + v.beacGap);
                    bool on = k < v.beacN && tk < v.beacDur;
                    rdBeacEnv += ((on ? 1.0 : 0.0) - rdBeacEnv) * (1 - std::exp(-dt / 0.008));
                    double f = v.beacF[std::min(k, 6)] * (v.beacSag ? 0.985 + 0.015 * std::cos(TAU_ * 0.03 * rdClock) : 1.0);
                    rdBeacPh += dt * f; double ph = rdBeacPh - std::floor(rdBeacPh);
                    sig += 0.24 * rdBeacEnv * ((1 - v.beacSq) * std::sin(TAU_ * ph) + v.beacSq * (ph < 0.5 ? 1 : -1));
                    break;
                }
                case 6: {   // data: bursts of two tones switched by a shift register, a buzz of the mains under them
                    rdDataT -= dt;
                    if (rdDataT <= 0) { rdDataOn = !rdDataOn; rdDataT = rdDataOn ? v.dataBurst * (0.7 + 0.6 * uni()) : v.dataGap * (0.6 + 0.8 * uni()); }
                    if (rdDataOn) {
                        rdDataBitT += dt * v.dataRate;
                        if (rdDataBitT >= 1) { rdDataBitT -= 1; uint32_t b = ((rdDataLfsr >> 0) ^ (rdDataLfsr >> 2) ^ (rdDataLfsr >> 3) ^ (rdDataLfsr >> 5)) & 1u; rdDataLfsr = (rdDataLfsr >> 1) | (b << 15); }
                        double want = (rdDataLfsr & 1u) ? v.dataF2 : v.dataF1; rdDataF += (want - rdDataF) * (1 - std::exp(-dt / 0.002));
                        rdDataPh += dt * rdDataF; double ph = rdDataPh - std::floor(rdDataPh);
                        sig += 0.17 * (0.55 * std::sin(TAU_ * ph) + 0.45 * (ph < 0.5 ? 1 : -1));
                        rdDataHumPh += dt * v.dataHum; double hp = rdDataHumPh - std::floor(rdDataHumPh);
                        sig += 0.09 * (2 * hp - 1) * (0.7 + 0.3 * std::sin(TAU_ * 0.9 * rdClock));
                    }
                    break;
                }
                case 7: {   // a bell tolling far off: six partials, the period a little irregular, now and then struck twice
                    rdBellNext -= dt;
                    if (rdBellNext <= 0) { rdBellT = 0; rdBellNext = uni() < 0.2 ? 0.6 + 0.4 * uni() : v.bellPeriod * (0.8 + 0.4 * uni()); }
                    if (rdBellT >= 0) {
                        double b = 0;
                        for (int i = 0; i < 6; i++) { rdBellPh[i] += dt * v.bellF * BELL_PARTIALS[i] * (i == 3 ? 1.003 : 1.0); b += std::sin(TAU_ * rdBellPh[i]) * std::exp(-rdBellT * (0.7 + 0.45 * i)) / (1 + 0.3 * i); }
                        b *= std::exp(-rdBellT / v.bellDecay) * (rdBellT < 0.004 ? rdBellT / 0.004 : 1.0);
                        rdBellLp += (b - rdBellLp) * v.bellFar;
                        sig += 0.55 * rdBellLp;
                        rdBellT += dt; if (rdBellT > v.bellDecay * 3) rdBellT = -1;
                    }
                    break;
                }
                case 8: {   // a siren still running: a slow glide or two tones, a sawtooth through a resonance, carried in and out as if on a wind
                    rdSirT += dt;
                    double f;
                    if (v.sirenTwo) f = ((int)(rdSirT / v.sirenCycle) % 2) ? v.sirenF * v.sirenRatio : v.sirenF;
                    else { double ph = std::fmod(rdSirT, 2 * v.sirenCycle) / v.sirenCycle; double tri = ph < 1 ? ph : 2 - ph; f = v.sirenF * std::pow(v.sirenRatio, tri); }
                    rdSirPh += dt * f; double saw = 2 * (rdSirPh - std::floor(rdSirPh)) - 1;
                    double o = resonate(saw, f * 3, f * 0.6, dt, rdSirY1, rdSirY2);
                    rdSirSubPh += dt * f * 0.5;
                    double wind = std::pow(0.5 + 0.5 * std::sin(TAU_ * rdClock / v.sirenSwell), 2.0);
                    sig += (0.5 * o + 0.1 * std::sin(TAU_ * rdSirSubPh)) * (0.2 + 0.8 * wind);
                    break;
                }
                case 9: {   // a murmur of voices behind a wall, one of them raised now and then
                    double m = 0;
                    for (int i = 0; i < 3; i++) { if (rdPhan[i].f0Base != v.murF0[i]) { rdPhan[i] = Phantom(); rdPhan[i].f0Base = v.murF0[i]; rdPhan[i].f0 = rdPhan[i].f0T = v.murF0[i]; } m += phantomSample(rdPhan[i], dt, v.murRate * (i == 0 ? 1 : (i == 1 ? 1.12 : 0.9)), 0.25, 0.3); }
                    rdShoutNext -= dt;
                    if (rdShoutNext <= 0 && rdShoutT < 0) { rdShoutT = 0; rdShoutNext = 6 + 9 * uni(); rdPhan[3] = Phantom(); rdPhan[3].f0Base = v.murF0[2] * 1.3; rdPhan[3].f0 = rdPhan[3].f0T = rdPhan[3].f0Base; }
                    if (rdShoutT >= 0) { double u = rdShoutT / 0.7; if (u >= 1) rdShoutT = -1; else { m += 1.6 * std::sin(PI_ * u) * phantomSample(rdPhan[3], dt, 6, 0.1, 0.8); rdShoutT += dt; } }
                    rdMurLp += (m - rdMurLp) * 0.18;
                    sig += 0.9 * rdMurLp;
                    break;
                }
                case 2: {   // a chant: a hum of three voices an octave, a unison and a fifth round the chanter's pitch, swelling with it
                    double p = st.voice ? st.voice->pitch : 110;
                    rdChantAmp += (spVoiceAmp - rdChantAmp) * (1 - std::exp(-dt / 0.6));
                    double vib = 1 + 0.008 * std::sin(TAU_ * 4.3 * rdClock);
                    rdChantPh[0] += dt * p * 0.5 * vib; rdChantPh[1] += dt * p * 1.003 * vib; rdChantPh[2] += dt * p * 1.5 * vib;
                    sig += 0.2 * rdChantAmp * (std::sin(TAU_ * rdChantPh[0]) + 0.6 * std::sin(TAU_ * rdChantPh[1]) + 0.4 * std::sin(TAU_ * rdChantPh[2]));
                    break;
                }
                case 1: {   // whispers: someone breathing close to the microphone
                    rdWhBr += (n2 - rdWhBr) * 0.06;
                    double env = std::pow(std::max(0.0, std::sin(TAU_ * 0.28 * rdClock)), 2.5);
                    sig += 0.5 * env * rdWhBr;
                    break;
                }
                case 3: {   // a numbers station: a carrier with the mains on it
                    rdNumPh += dt * 400; sig += 0.05 * std::sin(TAU_ * rdNumPh) * (0.75 + 0.25 * std::sin(TAU_ * 50 * rdClock));
                    break;
                }
                case 4: {   // a loop: the mechanism's whir while the phrase runs, tape hiss under
                    rdLoopPh += dt * 95; sig += (spOn ? 0.05 : 0.0) * (2 * (rdLoopPh - std::floor(rdLoopPh)) - 1) + 0.05 * rdN1;
                    break;
                }
                default: break;
            }
            // the cue at a speech's start: a numbers station's chime (three notes falling), a loop's relay and blip
            if (rdCueT >= 0) {
                if (st.radarVoice == 3) {
                    int k = (int)(rdCueT / 0.2); double tk = rdCueT - k * 0.2;
                    if (k < 3) { double f = k == 0 ? 1320 : (k == 1 ? 1100 : 880); rdCuePh += dt * f; sig += 0.2 * std::exp(-tk * 7) * std::sin(TAU_ * rdCuePh); }
                    else rdCueT = -1;
                } else if (st.radarVoice == 4) {
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
            double rate = std::max(0.2, st.radarPulseHz);
            double gate = 1;
            if (v.nulling) { rdNullT += dt; double cyc = v.nullOn + v.nullOff; if (std::fmod(rdNullT, cyc) >= v.nullOn) gate = 0; }
            rdPh += dt * rate;
            if (rdPh >= 1) { rdPh -= 1; rdHalf = false; if (gate > 0) rdT = 0; }
            if (v.interpulse > 0 && rdPh >= 0.5 && !rdHalf) { rdHalf = true; if (gate > 0) rdT2 = 0; }
            auto thump = [&](double& T, double& ph, double& gph, double amp) {
                if (T < 0) return;
                double env = std::exp(-T * 9), f = v.thumpF * std::exp(-T * 2.5) + 28;
                ph += dt * f; sig += 0.75 * amp * env * std::sin(TAU_ * ph);
                double fg = 160 * std::exp(-T * 1.5) + 50; gph += dt * fg; double saw = 2 * (gph - std::floor(gph)) - 1;
                sig += amp * (0.16 * v.growl * std::exp(-T * 4) * saw + 0.5 * std::exp(-T * 60) * n2);
                T += dt; if (T > 1.2) T = -1;
            };
            thump(rdT, rdThumpPh, rdGrowlPh, 1.0);
            thump(rdT2, rdThumpPh2, rdGrowlPh2, v.interpulse);
            if (v.breath && rate < 1.3) { double env = rdPh > 0.55 ? std::pow((rdPh - 0.55) / 0.45, 2.0) : 0; rdBreathLp += (n2 - rdBreathLp) * 0.08; sig += 0.9 * env * gate * rdBreathLp; }
            rdWhine += dt * (v.whineF + 300 * std::sin(lfo * 6.2831853 * 0.8));
            sig += 0.03 * std::sin(6.2831853 * rdWhine) * (0.5 + 0.5 * std::sin(lfo * 6.2831853 * 0.33));
            break;
        }
        case 2: {   // a comet: its song (a warble far down with a second voice an interval off, by its seed), geysers of hiss, ice cracking, a rumble; some glide, some moan
            double wob = 1 + 0.12 * std::sin(lfo * 6.2831853 * 1.9) + 0.05 * std::sin(lfo * 6.2831853 * 0.27);
            double gl = v.glide ? 1 + 0.2 * std::sin(TAU_ * 0.07 * rdClock) * std::sin(TAU_ * 0.031 * rdClock + 0.7) : 1;
            rdWarb += dt * v.songF * wob * gl; rdWarb2 += dt * v.songF * v.ratio * wob * gl;
            double songEnv = 0.5 + 0.5 * std::sin(lfo * 6.2831853 * 0.09);
            sig += 0.26 * songEnv * (std::sin(6.2831853 * rdWarb) + 0.6 * std::sin(6.2831853 * rdWarb2) + 0.3 * std::sin(6.2831853 * rdWarb * 2));
            rdGeyserNext -= dt;
            if (rdGeyserNext <= 0 && rdGeyser < 0) { rdGeyser = 0; rdGeyserNext = (2 + 4 * uni()) / v.geyserRate; }
            if (rdGeyser >= 0) { double u = rdGeyser / 1.4; if (u >= 1) rdGeyser = -1; else { sig += 0.7 * std::sin(3.14159 * u) * rdN1; rdGeyser += dt; } }
            sig += 2.5 * rdBrown2;
            if (rdCr < 0 && uni() < dt * 4 * v.crackle) rdCr = 0;
            if (rdCr >= 0) { sig += 0.7 * n2 * std::exp(-rdCr * 400); rdCr += dt; if (rdCr > 0.03) rdCr = -1; }
            if (v.moan) {
                Phantom& p = rdPhan[3];
                p.f0T = v.moanF * gl * (1 + 0.03 * std::sin(TAU_ * 0.13 * rdClock)); p.ampT = 0.35 + 0.35 * std::sin(TAU_ * 0.05 * rdClock);
                double m = 0.5 + 0.5 * std::sin(TAU_ * 0.09 * rdClock); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[3][i] * (1 - m) + VOWELS[0][i] * m;
                sig += 0.45 * phantomSample(p, dt, 0, 0.35, 0.5);
            }
            break;
        }
        case 3: {   // a magnetosphere: whistlers falling slowly from 2 kHz, bursts of chorus, a band of hiss that breathes, and a choir of three voices (by its seed quarter-tones, fifths, a cluster or octaves) that comes and goes; some crack with lightning (sferics and tweeks), some roar
            whNext -= dt;
            if (whNext <= 0 && whT < 0) { whT = 0; whNext = (2.0 + 4.0 * uni()) / v.whistlerRate; }
            if (whT >= 0) { double u = whT / 1.8; if (u >= 1) whT = -1; else { double f = 2200 * std::exp(-u * 2.6) + 150; whPh += dt * f; sig += 0.45 * std::sin(3.14159 * u) * std::sin(6.2831853 * whPh); whT += dt; } }
            chNext -= dt;
            if (chNext <= 0 && chT < 0) { chT = 0; chN = 2 + (int)(3.99 * uni()); chNext = (4 + 6.0 * uni()) / v.chorusRate; }
            if (chT >= 0) { const double per = 0.26; int kk = (int)(chT / per); if (kk >= chN) chT = -1; else { double u = chT / per - kk; double f = 700 * std::pow(2.5, u); chPh += dt * f; sig += 0.3 * std::sin(3.14159 * u) * std::sin(6.2831853 * chPh); chT += dt; } }
            sig += 0.4 * resonate(n2, v.hissF, 120, dt, rdHissY1, rdHissY2) * (0.5 + 0.5 * std::sin(lfo * 6.2831853 * 0.4));
            if (v.sferics) {
                if (rdSfT < 0 && uni() < dt * v.sfRate) { rdSfT = 0; rdSfPh = 0; }
                if (rdSfT >= 0) { double f = 2200 * std::exp(-rdSfT * 4) + 1500; rdSfPh += dt * f; sig += 0.9 * n2 * std::exp(-rdSfT * 600) + 0.25 * std::exp(-rdSfT * 30) * std::sin(TAU_ * rdSfPh); rdSfT += dt; if (rdSfT > 0.25) rdSfT = -1; }
            }
            if (v.roar) {
                rdRoarNext -= dt;
                if (rdRoarNext <= 0 && rdRoarT < 0) { rdRoarT = 0; rdRoarNext = 6 + 8 * uni(); }
                if (rdRoarT >= 0) { double u = rdRoarT / 1.6; if (u >= 1) rdRoarT = -1; else { sig += 0.6 * std::sin(PI_ * u) * resonate(n2, 300, 150, dt, rdRoarY1, rdRoarY2) * (0.6 + 0.4 * std::sin(TAU_ * 28 * rdRoarT)); rdRoarT += dt; } }
            }
            rdChoirNext -= dt;
            if (rdChoirNext <= 0 && rdChoirT < 0) { rdChoirT = 0; rdChoirNext = (8 + 6 * uni()) / v.choirRate; }
            if (rdChoirT >= 0) {
                double u = rdChoirT / 7.0;
                if (u >= 1) rdChoirT = -1;
                else {
                    double env = std::sin(3.14159 * u), vib = 1 + 0.01 * std::sin(6.2831853 * 5.3 * rdChoirT);
                    const double* c = CHOIRS[v.choirSet];
                    rdChoirPh[0] += dt * c[0] * vib; rdChoirPh[1] += dt * c[1] * vib; rdChoirPh[2] += dt * c[2] * vib;
                    sig += 0.13 * env * (std::sin(6.2831853 * rdChoirPh[0]) + std::sin(6.2831853 * rdChoirPh[1]) + std::sin(6.2831853 * rdChoirPh[2]));
                    rdChoirT += dt;
                }
            }
            break;
        }
        default: break;
    }
    double dry = bed + ev + 0.45 * sig * radarSigLvl + 0.8 * rdContent;
    // the echo: a third of a second behind, half of it back (more under a chant, whispers, a murmur, a bell), dulled; the void everything is heard in
    double fb = st.radarKind == 0 ? (st.radarVoice == 2 ? 0.72 : (st.radarVoice == 1 ? 0.7 : (st.radarVoice == 9 ? 0.72 : (st.radarVoice == 7 ? 0.65 : 0.5)))) : 0.5;
    if (rdEcho.size() != 8192) { rdEcho.assign(8192, 0.0f); rdEchoI = 0; }
    double e = rdEcho[rdEchoI]; rdEchoLp += (e - rdEchoLp) * 0.3;
    rdEcho[rdEchoI] = (float)(dry + rdEchoLp * fb); rdEchoI = (rdEchoI + 1) % rdEcho.size();
    return radarLvl * (dry + 0.55 * rdEchoLp);
}

// X-01: the probe's relay. The link is a soft hiss with a tick a second while the probe is in space; the air a rush whose
// brightness and level follow the stage; the entry a roar with a crackle; the chute a flutter on the rush; below the first deck a
// rumble deepening and, past half the depth, the hull's groans; a thunderclap muffled through the relay; at the end only the hiss
// of the lost signal. Everything through the relay's level (`prLvl`)
double AudioSynth::probeSample(double dt, AudioState& st) {
    if (st.probeThunder > 0) { prThunT = 0; prThunAmp = st.probeThunder; st.probeThunder = 0; }
    float n = noise();
    double s = 0;
    prHp += (n - prHp) * 0.35; double hiss = n - prHp;   // a high band of white noise
    if (st.probeLost) {
        prCrackle -= dt;
        double crack = 0;
        if (prCrackle <= 0) { crack = 0.5 * noise(); prCrackle = 0.02 + 0.3 * (0.5 + 0.5 * noise()); }
        return prLvl * (0.11 * hiss + 0.05 * crack);
    }
    double statik = 1 - st.probeSignal;
    s += (0.012 + 0.05 * statik * statik) * hiss;
    if (st.probeStage <= 1) {   // in space: the telemetry's tick
        prTick += dt;
        if (prTick >= 1.0) prTick -= 1.0;
        if (prTick < 0.008) s += 0.05 * std::sin(6.2831853 * 1600 * prTick);
    }
    double cut = 0.02 + 0.10 * st.probeWind * (1 - 0.6 * st.probeDepth);   // the air's rush, darker as it deepens
    prWindLp += (n - prWindLp) * cut; prWindLp2 += (prWindLp - prWindLp2) * cut;
    double gust = 0.75 + 0.25 * std::sin(lfo * 6.2831853 * 0.6) * std::sin(lfo * 6.2831853 * 0.17 + 1.1);
    double flutter = st.probeChute > 0 ? 0.55 + 0.45 * std::sin(prChutePh += dt * 6.2831853 * 7.3) : 1.0;
    s += st.probeWind * 0.55 * prWindLp2 * gust * flutter;
    if (st.probeHeat > 0.01) {   // the entry: a roar and a crackle
        prHeatLp1 += (n - prHeatLp1) * 0.03; prHeatLp2 += (prHeatLp1 - prHeatLp2) * 0.03;
        double crack = (0.5 + 0.5 * noise()) < 0.004 * st.probeHeat ? noise() * 0.6 : 0.0;
        s += st.probeHeat * (1.1 * prHeatLp2 + 0.12 * prHeatLp1 + crack);
    }
    if (st.probeDepth > 0.01) {   // the deep: a rumble, and the hull's groans past half the depth
        prRum1 += (n - prRum1) * 0.006; prRum2 += (prRum1 - prRum2) * 0.006;
        s += st.probeDepth * 1.6 * prRum2;
        if (st.probeDepth > 0.5) {
            prCreakNext -= dt;
            if (prCreakNext <= 0 && prCreakT < 0) { prCreakT = 0; prCreakF = 70 + 40 * (0.5 + 0.5 * noise()); prCreakNext = 2.5 + 6 * (0.5 + 0.5 * noise()) / st.probeDepth; }
            if (prCreakT >= 0) {
                double dur = 0.7;
                if (prCreakT < dur) {
                    double env = std::sin(prCreakT / dur * 3.14159) * (0.7 + 0.3 * std::sin(prCreakT * 90));
                    prCreakPh += dt * prCreakF * (1 - 0.35 * prCreakT / dur);
                    s += st.probeDepth * 0.09 * env * (std::sin(prCreakPh * 6.2831853) + 0.4 * std::sin(prCreakPh * 6.2831853 * 2.7));
                    prCreakT += dt;
                } else prCreakT = -1;
            }
        }
    }
    if (prThunT >= 0) {   // thunder through the relay: a band of the roar, a sharp front and a long tail
        if (prThunT < 2.2) {
            prThunLp1 += (n - prThunLp1) * 0.05; prThunLp2 += (prThunLp1 - prThunLp2) * 0.05;
            double env = std::exp(-prThunT * 1.8) * (prThunT < 0.03 ? prThunT / 0.03 : 1.0);
            s += prThunAmp * 2.2 * env * prThunLp2;
            prThunT += dt;
        } else prThunT = -1;
    }
    return prLvl * s;
}

void AudioSynth::render(float* out, int frames, int sr, AudioState& st) {
    double dt = 1.0 / sr;
    if (st.beep > 0) { beepT = 0; beepKind = st.beep; st.beep = 0; }
    if (st.thunder > 0) { thunderT = 0; thunderAmp = st.thunder; st.thunder = 0; }
    if (st.step > 0) { stepT = 0; stepKind = st.step; stepGainCur = st.stepGain; st.step = 0; }
    // C-04 the music: a start from the top; a null piece stops every voice at once
    if (st.pieceStart) { st.pieceStart = false; mPiece = st.piece; mNext = 0; mBeat = 0; mT = 0; for (MVoice& v : mv) v = MVoice(); mOn = mPiece && st.tradition && !mPiece->notes.empty(); st.pieceDone = !mOn; }
    if (!st.piece || !st.tradition || st.piece != mPiece) { if (mOn || mPiece) { for (MVoice& v : mv) v = MVoice(); mOn = false; mPiece = nullptr; } }
    // C-05 the voice: a start from the top; a null speech stops it at once, the resonators cleared
    if (st.speechStart && st.radio) { rdCueT = 0; rdCuePh = 0; }   // C-07: the receiver's cue at a programme's start
    if (st.speechStart) {
        st.speechStart = false; spSpeech = st.speech; spNext = 0; spSeg = -1; spT = 0; spVoiceAmp = spBreathAmp = spNzAmp = spNasal = 0; spPh = spSubPh = 0; spJit = spShim = 1; spLpB = 0;
        for (int i = 0; i < 6; i++) spY1[i] = spY2[i] = 0;
        spOn = spSpeech && st.voice && !spSpeech->segs.empty(); st.speechDone = !spOn;
    }
    if (!st.speech || !st.voice || st.speech != spSpeech) { if (spOn || spSpeech) { spOn = false; spSpeech = nullptr; spVoiceAmp = spBreathAmp = spNzAmp = 0; for (int i = 0; i < 6; i++) spY1[i] = spY2[i] = 0; } }
    double k = 1 - std::exp(-dt * 2.0);
    for (int i = 0; i < frames; i++) {
        humLvl += (st.hum - humLvl) * k;
        engLvl += (st.engine - engLvl) * k;
        windLvl += (st.wind - windLvl) * k;
        leavesLvl += (st.leaves - leavesLvl) * k;
        hoofLvl += (st.hoofs - hoofLvl) * k;
        rpmLvl += (st.engineRpm - rpmLvl) * k * 2;
        skidLvl += (st.skid - skidLvl) * k;
        rotorLvl += (st.rotor - rotorLvl) * k; rotorPitchLvl += (st.rotorPitch - rotorPitchLvl) * k;
        radarLvl += (st.radar - radarLvl) * k; radarSigLvl += (st.radarSignal - radarSigLvl) * k * 2;   // C-07
        if (st.thud > 0 && thudT < 0) { thudT = 0; thudAmp = st.thud; }
        st.thud = 0;
        insectLvl += (st.insects - insectLvl) * k;
        if (st.call > 0 && callT < 0) { callT = 0; callKind = st.call; callPitchCur = st.callPitch; }
        st.call = 0;
        rainLvl += (st.rain - rainLvl) * k;
        lavaLvl += (st.lava - lavaLvl) * k;
        breathLvl += (st.breath - breathLvl) * k;
        surfLvl += (st.surf - surfLvl) * k;
        birdLvl += (st.birds - birdLvl) * k;
        lfo += dt * 0.23;
        double s = 0;
        // hum: two low sines with slow beating
        phase1 += dt * 55.0; phase2 += dt * (110.0 + 0.7 * std::sin(lfo * 6.28));
        s += humLvl * 0.16 * (std::sin(phase1 * 6.2831853) + 0.5 * std::sin(phase2 * 6.2831853)) * (0.8 + 0.2 * std::sin(lfo * 6.28 * 0.5));
        // engine: filtered noise + rising tone
        float n = noise();
        lp1 += (n - lp1) * 0.08f;
        lp2 += (lp1 - lp2) * 0.08f;
        s += engLvl * (0.5 * lp2 + 0.12 * std::sin(phase1 * 6.2831853 * 3.0 * (1 + engLvl)));
        if (engLvl > 0.05 && rpmLvl > 0.001) s += engLvl * 0.08 * std::sin(phase1 * 6.2831853 * (9.0 + 12.0 * rpmLvl));   // N4-04 the gear's pitch on top of the drive
        if (skidLvl > 0.02) { float n4 = noise(); lpK += (n4 - lpK) * 0.25f; s += skidLvl * 0.16 * lpK; }   // N4-04 tyres sliding
        if (rotorLvl > 0.01) {   // R-403 the drone's pods: a blade-pass whine (a fundamental with two harmonics, beating between the four pods) over a low rush of air
            rotorPh += dt * (70.0 + 90.0 * rotorPitchLvl);
            double w = std::sin(rotorPh * 6.2831853) + 0.5 * std::sin(rotorPh * 6.2831853 * 2.01) + 0.25 * std::sin(rotorPh * 6.2831853 * 3.02);
            double beat = 0.8 + 0.2 * std::sin(lfo * 6.2831853 * 6.1) * std::sin(lfo * 6.2831853 * 4.3);
            lpR += (n - lpR) * 0.18;
            s += rotorLvl * (0.09 * w * beat + 0.2 * lpR);
        }
        if (thudT >= 0) { if (thudT < 0.25) { s += thudAmp * 0.35 * std::exp(-thudT * 18) * std::sin(6.2831853 * thudT * 55); thudT += dt; } else thudT = -1; }
        // wind: low-pass noise with gusty amplitude; the tone sets the family (sea: low and slow, mountain: a whistle)
        lp3 += (n - lp3) * (0.012f + 0.06f * (float)st.windTone + 0.03f * (float)windLvl);
        double gust = 0.6 + 0.4 * std::sin(lfo * 6.28 * (0.5 + 1.2 * st.windTone)) * std::sin(lfo * 6.28 * 0.37);
        s += windLvl * 0.9 * lp3 * gust;
        if (st.windTone > 0.85) s += windLvl * 0.06 * std::sin(phase1 * 6.2831853 * 14.0 * (1 + 0.3 * std::sin(lfo * 6.28 * 0.9)));   // the whistle
        if (callT >= 0) {   // N3-05 creature calls: chirp (a rising sine), low (a slow growl with vibrato), hoot (two soft notes), click (four noise ticks)
            double p = callPitchCur, v = 0;
            switch (callKind) {
                case 1: { double dur = 0.18; if (callT < dur) v = 0.14 * std::sin(callT / dur * 3.14159) * std::sin(6.2831853 * callT * (1800 + 1400 * callT / dur) * p); else callT = -2; break; }
                case 2: { double dur = 0.7; if (callT < dur) v = 0.16 * std::sin(callT / dur * 3.14159) * (std::sin(6.2831853 * callT * 110 * p * (1 + 0.04 * std::sin(callT * 40))) + 0.35 * std::sin(6.2831853 * callT * 220 * p) + 0.15 * std::sin(6.2831853 * callT * 330 * p)); else callT = -2; break; }
                case 3: { double dur = 0.55; if (callT < dur) { double f = callT < 0.27 ? 520 : 430; double e = std::sin(std::fmod(callT, 0.27) / 0.27 * 3.14159); v = 0.12 * e * std::sin(6.2831853 * callT * f * p); } else callT = -2; break; }
                default: { double dur = 0.34; if (callT < dur) { double ph = std::fmod(callT, 0.085); v = ph < 0.012 ? 0.2 * noise() : 0; } else callT = -2; break; }
            }
            if (callT >= 0) { s += v; callT += dt; } else callT = -1;
        }
        if (hoofLvl > 0.01) {   // N3-05 hoof and paw steps: soft thumps at a pace that quickens with the level
            hoofT += dt;
            double period = 0.42 / (0.5 + hoofLvl);
            if (hoofT > period) hoofT -= period;
            if (hoofT < 0.06) s += hoofLvl * 0.14 * std::exp(-hoofT * 45) * std::sin(6.2831853 * hoofT * 70);
        }
        if (insectLvl > 0.01) {   // N3-05 insects: a thin buzz, amplitude-modulated
            float n3 = noise();
            lpI += (n3 - lpI) * 0.6f; lpI2 += (lpI - lpI2) * 0.6f;
            s += insectLvl * 0.035 * (lpI - lpI2) * (0.6 + 0.4 * std::sin(6.2831853 * 48 * lfo));
        }
        if (leavesLvl > 0.001) {   // N2-06 leaves: a brighter, band-passed hiss that swells with the gusts
            float n2 = noise();
            lpL += (n2 - lpL) * 0.35f;
            hpL += (lpL - hpL) * 0.04f;
            double gust2 = 0.5 + 0.5 * std::sin(lfo * 6.28 * 0.8 + 1.7) * std::sin(lfo * 6.28 * 0.23);
            s += leavesLvl * 0.5 * (lpL - hpL) * (0.4 + 0.6 * gust2);
        }
        // surf: slow surges of noise
        if (surfLvl > 0.01) { double surge = std::pow(0.5 + 0.5 * std::sin(lfo * 6.28 * 0.14), 3.0); s += surfLvl * 0.7 * lp3 * surge; }
        // bird calls: short sine sweeps now and then
        if (birdLvl > 0.02) {
            nextChirp -= dt;
            if (nextChirp <= 0 && chirpT < 0) { chirpT = 0; chirpF = 1800 + 2200 * (0.5 + 0.5 * noise()); nextChirp = 0.4 + 3.0 * (0.5 + 0.5 * noise()) / birdLvl; }
            if (chirpT >= 0) {
                double dur = 0.09;
                if (chirpT < dur) { double env = std::sin(chirpT / dur * 3.14159); s += birdLvl * 0.12 * env * std::sin(chirpT * (chirpF + 1500 * chirpT / dur) * 6.2831853); chirpT += dt; }
                else chirpT = -1;
            }
        }
        // footsteps: a short filtered burst per stride
        if (stepT >= 0) {
            double dur = stepKind == 3 ? 0.16 : 0.05;
            if (stepT < dur) {
                double env = stepT < 0.005 ? stepT / 0.005 : std::exp(-(stepT - 0.005) * (stepKind == 3 ? 20 : 60));
                lpS += (n - lpS) * (stepKind == 1 ? 0.12f : 0.5f);
                hpS = n - hpS * 0.5;
                double v = stepKind == 1 ? lpS * 1.2 : (stepKind == 2 ? hpS * 0.5 : lpS * 0.9);
                s += stepGainCur * 0.35 * env * v;
                stepT += dt;
            } else stepT = -1;
        }
        // rain: high-passed noise crackle
        hp = n - hp * 0.6;
        s += rainLvl * 0.14 * hp;
        // lava rumble
        s += lavaLvl * 0.25 * lp2 * (0.5 + 0.5 * std::sin(lfo * 6.28 * 2.3));
        // breathing: slow noise pulses, faster when more winded
        if (breathLvl > 0.01) { breathPh += dt * (0.7 + 1.1 * breathLvl); double env = std::pow(std::max(0.0, std::sin(breathPh * 6.2831853)), 3.0); s += breathLvl * 0.22 * env * lp3; }
        {   // C-04 the music, C-05 the voice; C-07: through the radar's receiver when the radar owns them: a band of 250 Hz to 2.6 kHz,
            // a tape's wow (slight under a voice, heavy under a slowed piece), a slow fading, and only as much of it as the beam has
            double m = musicSample(dt, st) + speechSample(dt, st);
            rdContent = 0;
            if (st.radio) {   // and ghosted: a metallic ring under it, a slow fade, dropouts now and then; it joins the receiver's bed and echo below
                rdHp += (m - rdHp) * 0.069; double hp = m - rdHp;
                rdLp1 += (hp - rdLp1) * 0.52; rdLp2 += (rdLp1 - rdLp2) * 0.52;
                bool slowed = st.radarVoice == 10, whispers = st.radarVoice == 1;
                if (rdWow.size() != 4096) { rdWow.assign(4096, 0.0f); rdWowI = 0; }
                rdWow[rdWowI] = (float)rdLp2;
                rdWowT += dt;
                double d = 0.04 + (slowed ? 0.028 * (1 + std::sin(6.2831853 * 0.35 * rdWowT)) : 0.0025 * (1 + std::sin(6.2831853 * 0.8 * rdWowT))) + 0.0015 * std::sin(6.2831853 * 7.3 * rdWowT);
                auto tap = [&](double back) { double pos = (double)rdWowI - back * sr; while (pos < 0) pos += rdWow.size(); size_t i0 = (size_t)pos % rdWow.size(), i1 = (i0 + 1) % rdWow.size(); double fr = pos - std::floor(pos); return rdWow[i0] * (1 - fr) + rdWow[i1] * fr; };
                double x = tap(d);
                if (whispers) x = 0.75 * x + 0.5 * tap(d + 0.09);   // whispers, plural
                rdWowI = (rdWowI + 1) % rdWow.size();
                double fade = 0.72 + 0.28 * std::sin(lfo * 6.2831853 * 0.9) * std::sin(lfo * 6.2831853 * 1.7);
                rdRing += dt * (slowed ? 23.0 : 37.0);
                rdDropT -= dt;
                if (rdDropT <= 0) { bool gap = 0.5 + 0.5 * noise() < 0.22; rdDrop = gap ? 0.1 : 1.0; rdDropT = gap ? 0.12 + 0.35 * (0.5 + 0.5 * noise()) : 1.5 + 4.0 * (0.5 + 0.5 * noise()); }
                rdDropLvl += (rdDrop - rdDropLvl) * 0.003;
                rdContent = x * (0.78 + 0.22 * std::sin(6.2831853 * rdRing)) * (whispers ? 2.2 : 1.6) * radarSigLvl * fade * rdDropLvl;
                m = 0;
            }
            s += m;
        }
        prLvl += (st.probe - prLvl) * k * 2;   // X-01 the probe's relay
        if (prLvl > 0.003 || st.probeThunder > 0) s += probeSample(dt, st);
        if (radarLvl > 0.005) s += radarSample(dt, st);   // C-07 the receiver (the radar camera)
        else { rdContent = 0; rdEvT = -1; rdEvNext = 6 + 8 * (0.5 + 0.5 * noise()); rdClock = 0; }
        // beep
        if (beepT >= 0) {
            double f = beepKind == 1 ? 880 : (beepKind == 2 ? 660 : (beepKind == 3 ? 220 : 1320));
            double dur = beepKind == 2 ? 0.35 : 0.12;
            if (beepT < dur) {
                double env = std::sin(beepT / dur * 3.14159);
                s += 0.25 * env * std::sin(beepT * f * 6.2831853);
                beepT += dt;
            } else beepT = -1;
        }
        if (thunderT >= 0) {
            if (thunderT < 2.5) {
                double env = std::exp(-thunderT * 1.5) * (thunderT < 0.05 ? thunderT / 0.05 : 1.0);
                s += thunderAmp * 0.8 * env * lp2 * 3.0;
                thunderT += dt;
            } else thunderT = -1;
        }
        out[i] = (float)(std::max(-1.0, std::min(1.0, s)) * st.master);
    }
    st.pieceBeat = mOn ? mBeat : -1;
    st.speechT = spOn ? spT : -1;
}
