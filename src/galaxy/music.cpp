#include "music.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

const char* const TIMBRE_FAMILY_NAMES[TF_COUNT] = {"A PLUCKED STRING", "A PIPE", "A BOWED STRING", "A BELL", "A REED", "A DRUM"};
const char* const PIECE_FORM_NAMES[PF_COUNT] = {"A DANCE", "A LULLABY", "A WORK SONG", "A HYMN", "A HARVEST SONG", "A LOVE SONG", "A SONG OF THE FESTIVAL",
                                                "A LAMENT", "A MARCH", "A SONG OF WARNING", "THE LAST SONG"};

namespace {

double centsOf(double r) { return 1200.0 * std::log2(r); }

// a timbre of the family, with its own numbers: the partials, the envelope, the vibrato, the breath
Timbre makeTimbre(int family, Rng& rng) {
    Timbre t; t.family = family;
    switch (family) {
        case TF_PLUCKED: {   // a quick attack, a decay, slightly stretched partials
            double roll = rng.range(1.1, 1.9), inh = rng.range(0.0, 0.002);
            for (int k = 0; k < 6; k++) { t.ratio[k] = (k + 1) * (1 + inh * k * k); t.amp[k] = std::pow(1.0 / (k + 1), roll); }
            t.attack = 0.003; t.decay = rng.range(0.35, 1.1); t.sustain = 0; t.release = 0.4; t.gain = 0.34;
            break;
        }
        case TF_PIPE: {   // a soft attack, a hollow spectrum, breath and a gentle vibrato
            for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = k == 0 ? 1 : (k == 1 ? rng.range(0.15, 0.4) : rng.range(0.02, 0.12) / k); }
            t.attack = rng.range(0.03, 0.09); t.decay = 0.1; t.sustain = 0.85; t.release = rng.range(0.06, 0.14);
            t.vibRate = rng.range(4.0, 6.0); t.vibDepth = rng.range(0.002, 0.006); t.breath = rng.range(0.12, 0.3); t.gain = 0.3;
            break;
        }
        case TF_BOWED: {   // a slow attack, a saw's partials, a wider vibrato
            for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = 1.0 / (k + 1) * (k % 2 ? rng.range(0.6, 1.0) : 1.0); }
            t.attack = rng.range(0.08, 0.18); t.decay = 0.2; t.sustain = 0.9; t.release = rng.range(0.1, 0.2);
            t.vibRate = rng.range(4.5, 6.5); t.vibDepth = rng.range(0.003, 0.008); t.breath = rng.range(0.0, 0.05); t.gain = 0.2;
            break;
        }
        case TF_METAL: {   // a bell: inharmonic partials, a long ring
            const double R[6] = {1.0, 2.0, 3.0, 4.2, 5.4, 6.8};
            double stretch = rng.range(0.0, 0.25);
            for (int k = 0; k < 6; k++) { t.ratio[k] = R[k] + stretch * k * rng.range(0.3, 1.0); t.amp[k] = k == 0 ? 1 : rng.range(0.25, 0.6) / std::sqrt((double)k); }
            t.attack = 0.002; t.decay = rng.range(1.2, 3.0); t.sustain = 0; t.release = 1.2; t.gain = 0.26;
            break;
        }
        case TF_REED: {   // a reed: the odd partials strong, a nasal tone
            for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = (k % 2 == 0 ? 1.0 : rng.range(0.4, 0.8)) / (1 + 0.5 * k); }
            t.attack = rng.range(0.02, 0.05); t.decay = 0.15; t.sustain = 0.8; t.release = 0.08;
            t.vibRate = rng.range(5.0, 7.0); t.vibDepth = rng.range(0.002, 0.005); t.breath = rng.range(0.02, 0.08); t.gain = 0.22;
            break;
        }
        default: {   // the drum: a low skin and a high one (their pitches in Hz), their noise
            t.ratio[0] = rng.range(55, 90); t.ratio[1] = rng.range(160, 320); t.amp[0] = 1; t.amp[1] = rng.range(0.5, 1.0);
            t.attack = 0.001; t.decay = rng.range(0.12, 0.3); t.sustain = 0; t.release = 0.05; t.breath = rng.range(0.3, 0.8); t.gain = 0.5;
            break;
        }
    }
    return t;
}

}   // namespace

Tradition traditionOf(const BodyGen& g, const Lore& L) {
    Tradition T; T.seed = mix64(g.seed ^ 0x3C04A1ULL);
    Rng rng(T.seed);
    // the period: the octave, now and then a stretched or a narrow one, rarely the twelfth
    double pr = rng.uni();
    T.period = pr < 0.82 ? 2.0 : (pr < 0.95 ? rng.range(1.93, 2.08) : 3.0);
    bool twelfth = T.period > 2.5;
    double perCents = centsOf(T.period);
    static const double NW[5] = {2, 2, 3, 1.2, 0.8};   // five to nine notes
    int n = 5 + rng.pick(NW, 5);
    if (twelfth) n = std::max(n, 7);
    const double minGap = 75, maxGap = 450;   // cents between neighbours: enough to tell them apart, no hole wider than a third and a half
    for (int attempt = 0; attempt < 60 && T.scale.empty(); attempt++) {
        std::vector<double> cents;
        T.tuning = rng.chance(0.55) ? 0 : 1;
        if (T.tuning == 0) {
            static const int EDO[13] = {5, 7, 9, 10, 12, 13, 14, 15, 16, 17, 19, 22, 24};
            static const double EW[13] = {2, 3, 1, 1, 4, 1.5, 1, 1, 1, 1.5, 2, 1, 1};
            T.division = twelfth ? 13 : EDO[rng.pick(EW, 13)];
            if (T.division < n + 1) continue;
            std::vector<int> steps; for (int i = 1; i < T.division; i++) steps.push_back(i);
            for (int i = (int)steps.size() - 1; i > 0; i--) std::swap(steps[i], steps[rng.irange(i + 1)]);
            cents.push_back(0);
            for (int i = 0; i < n - 1; i++) cents.push_back(steps[i] * perCents / T.division);
        } else {
            static const double JI[19] = {16.0 / 15, 10.0 / 9, 9.0 / 8, 8.0 / 7, 7.0 / 6, 6.0 / 5, 5.0 / 4, 9.0 / 7, 4.0 / 3, 11.0 / 8, 7.0 / 5, 3.0 / 2, 14.0 / 9, 8.0 / 5, 5.0 / 3, 7.0 / 4, 16.0 / 9, 9.0 / 5, 15.0 / 8};
            std::vector<double> pool;
            for (double r : JI) if (r < T.period * 0.98) pool.push_back(r);
            if (twelfth) { const double extra[8] = {2.0, 9.0 / 4, 7.0 / 3, 5.0 / 2, 18.0 / 7, 8.0 / 3, 11.0 / 4, 25.0 / 9}; for (double r : extra) pool.push_back(r); }
            for (int i = (int)pool.size() - 1; i > 0; i--) std::swap(pool[i], pool[rng.irange(i + 1)]);
            cents.push_back(0);
            for (int i = 0; i < n - 1 && i < (int)pool.size(); i++) cents.push_back(centsOf(pool[i]));
        }
        std::sort(cents.begin(), cents.end());
        bool ok = (int)cents.size() == n;
        for (size_t i = 1; i < cents.size() && ok; i++) if (cents[i] - cents[i - 1] < minGap || cents[i] - cents[i - 1] > maxGap) ok = false;
        if (ok && (perCents - cents.back() < minGap || perCents - cents.back() > maxGap)) ok = false;
        if (!ok) continue;
        for (double c : cents) T.scale.push_back(std::pow(2.0, c / 1200.0));
    }
    if (T.scale.empty()) { T.tuning = 0; T.division = 12; T.period = 2.0; const double P5[5] = {0, 200, 400, 700, 900}; for (double c : P5) T.scale.push_back(std::pow(2.0, c / 1200.0)); }
    n = (int)T.scale.size();
    // the resting degree: the one nearest a fifth, else a fourth, else the middle
    {
        int best = -1; double bd = 60;
        for (int d = 1; d < n; d++) { double c = centsOf(T.scale[d]); if (std::fabs(c - 702) < bd) { bd = std::fabs(c - 702); best = d; } }
        if (best < 0) { bd = 60; for (int d = 1; d < n; d++) { double c = centsOf(T.scale[d]); if (std::fabs(c - 498) < bd) { bd = std::fabs(c - 498); best = d; } } }
        T.rest = best < 0 ? n / 2 : best;
    }
    T.base = 165.0 * std::pow(2.0, rng.uni());   // 165..330 Hz
    // the cycle
    static const int BEATS[10] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    static const double BW[10] = {0.5, 1.5, 2, 1.5, 3, 2, 1.5, 1, 0.8, 0.8};
    T.beats = BEATS[rng.pick(BW, 10)];
    for (int rem = T.beats; rem > 0;) {
        int gch;
        if (rem <= 3) gch = rem;
        else if (rem == 4) gch = rng.chance(0.3) ? 4 : 2;
        else { double w[3] = {1, 1, rem >= 6 ? 0.25 : 0}; gch = 2 + rng.pick(w, 3); if (rem - gch == 1) gch = gch == 2 ? 3 : 2; }
        T.groups.push_back(gch); rem -= gch;
    }
    T.sub = rng.chance(0.75) ? 2 : 3;
    T.bpm = rng.range(66, 144);
    T.drift = rng.range(0.01, 0.05);
    // the timbres, by the style of the names: hard names lean to struck and plucked things, flowing ones to pipes and bows
    {
        double fw[5] = {3, 2.5, 1.5, 1.5, 1.5};   // plucked, pipe, bowed, metal, reed
        if (L.style == 1) { fw[0] += 1.5; fw[3] += 1.5; }
        if (L.style == 2) { fw[1] += 1.5; fw[2] += 1.5; }
        int count = rng.chance(0.55) ? 2 : 3;
        for (int i = 0; i < count; i++) { int f = rng.pick(fw, 5); fw[f] = 0; T.voices.push_back(makeTimbre(f, rng)); }
    }
    T.drum = makeTimbre(TF_DRUM, rng);
    T.hasDrum = rng.chance(0.75) || L.style == 1;
    T.drone = rng.chance(0.4);
    { double sw[4] = {1.5, 1.5, 1.5, 1}; T.second = rng.pick(sw, 4); }
    { double ow[5] = {1.5, 1.5, 1, 1.2, 1}; T.ornament = rng.pick(ow, 5); }
    T.ornamentChance = rng.range(0.2, 0.5);
    return T;
}

double traditionHz(const Tradition& T, int degree) {
    int n = (int)T.scale.size(); if (n == 0) return T.base;
    int oct = degree >= 0 ? degree / n : -((-degree + n - 1) / n);
    return T.base * std::pow(T.period, oct) * T.scale[degree - oct * n];
}
double traditionCents(const Tradition& T, int degree) { return centsOf(traditionHz(T, degree) / T.base); }

bool pieceHasDrum(const Piece& P) { for (const Note& n : P.notes) if (n.voice < 0) return true; return false; }

std::string traditionLine(const Tradition& T) {
    char buf[96];
    const char* per = T.period > 2.5 ? "THE TWELFTH" : (T.period > 2.01 ? "A STRETCHED OCTAVE" : (T.period < 1.99 ? "A NARROW OCTAVE" : "THE OCTAVE"));
    snprintf(buf, sizeof buf, "A SCALE OF %d NOTES TO %s", (int)T.scale.size(), per);
    return buf;
}
std::string cycleLine(const Tradition& T, double bpm) {
    std::string g; for (size_t i = 0; i < T.groups.size(); i++) { if (i) g += "+"; g += std::to_string(T.groups[i]); }
    char buf[96]; snprintf(buf, sizeof buf, "A CYCLE OF %s AT %d BEATS A MINUTE", g.c_str(), (int)std::lround(bpm));
    return buf;
}
std::string timbresLine(const Tradition& T, bool drum) {
    std::vector<std::string> parts;
    for (const Timbre& t : T.voices) parts.push_back(TIMBRE_FAMILY_NAMES[t.family]);
    if (drum) parts.push_back(TIMBRE_FAMILY_NAMES[TF_DRUM]);
    std::string s;
    for (size_t i = 0; i < parts.size(); i++) { if (i) s += i + 1 == parts.size() ? " AND " : ", "; s += parts[i]; }
    if (T.drone && s.size() + 9 <= 52) s += ", A DRONE";
    return s;
}

void pieceOf(const Tradition& T, const Shard& s, Piece& P) {
    P = Piece(); P.seed = mix64(s.seed ^ 0x3C04ULL);
    Rng rng(P.seed);
    const int n = (int)T.scale.size(), sub = T.sub, beats = T.beats;
    // the form by the shard's tone
    double w[PF_COUNT] = {0};
    switch (s.tone) {
        case ST_ELEGY: w[PF_LAMENT] = 3; w[PF_HYMN] = 1; break;
        case ST_WARNING: w[PF_MARCH] = 2; w[PF_WARNING] = 2; w[PF_HYMN] = 1; break;
        case ST_END: w[PF_LAST] = 1; break;
        default: w[PF_DANCE] = 3; w[PF_LULLABY] = 2; w[PF_WORK] = 2; w[PF_HYMN] = 2; w[PF_HARVEST] = 1.2; w[PF_LOVE] = 1.2; w[PF_FESTIVAL] = 1; break;
    }
    P.form = rng.pick(w, PF_COUNT);
    struct FormP { double tempo, density, vel0, vel1, restP; int reg, drum; bool repeat; };   // drum: 0 never, 1 the tradition's, 2 always; repeat: the answer and the return once more
    static const FormP FP[PF_COUNT] = {
        {1.25, 1.5, 0.65, 0.95, 0.06, 0, 2, true},     // a dance
        {0.62, 0.8, 0.42, 0.62, 0.1, 1, 0, false},     // a lullaby: slow, soft, high
        {1.0, 1.1, 0.6, 0.85, 0.08, 0, 2, true},       // a work song
        {0.7, 0.55, 0.5, 0.75, 0.08, -1, 0, false},    // a hymn: long notes, low
        {1.1, 1.2, 0.6, 0.9, 0.08, 0, 1, true},        // a harvest song
        {0.82, 0.9, 0.5, 0.72, 0.1, 0, 1, false},      // a love song
        {1.35, 1.6, 0.7, 1.0, 0.05, 0, 2, true},       // a song of the festival: quick, loud, drummed
        {0.55, 0.65, 0.45, 0.65, 0.15, -1, 0, false},  // a lament
        {1.0, 1.0, 0.65, 0.95, 0.05, 0, 2, true},      // a march
        {0.9, 0.9, 0.55, 0.85, 0.1, -1, 1, false},     // a song of warning
        {0.5, 0.45, 0.4, 0.6, 0.25, -1, 0, false},     // the last song: slow, sparse, low
    };
    const FormP& F = FP[P.form];
    P.bpm = std::min(190.0, std::max(44.0, T.bpm * F.tempo * rng.range(0.92, 1.08)));
    const int reg = F.reg * (n / 2);
    const int cpp = beats <= 6 ? 2 : 1;   // cycles per phrase
    const int cycleUnits = beats * sub;
    std::vector<int> groupStart; { int u = 0; for (int gb : T.groups) { groupStart.push_back(u); u += gb * sub; } }
    auto isGroupStart = [&](int unitInCycle) { for (int gs : groupStart) if (gs == unitInCycle) return true; return false; };

    struct Ev { int unit, len, deg; bool rest; };
    // a group's rhythm: lengths in subdivision units that fill it; the phrase's closing group holds one note (or a short one and a long)
    auto groupRhythm = [&](int gb, double density, bool closing, std::vector<int>& out) {
        out.clear(); int units = gb * sub;
        if (closing) { if (rng.chance(0.6)) out.push_back(units); else { out.push_back(sub); out.push_back(units - sub); } return; }
        for (int rem = units; rem > 0;) {
            int lens[4] = {1, sub, 2 * sub, 3 * sub};
            double wl[4] = {density * 0.9, 1.2, 0.7 / density, 0.3 / density};
            for (int k = 0; k < 4; k++) if (lens[k] > rem) wl[k] = 0;
            int len = lens[rng.pick(wl, 4)];
            if (len == 1) { int k = std::min(rem, sub); for (int i = 0; i < k; i++) out.push_back(1); rem -= k; }
            else { out.push_back(len); rem -= len; }
        }
    };
    // the melody's walk: mostly steps, a leap now and then answered by a step back, held within a range round the centre
    auto nextDegree = [&](int cur, int lastStep, int centre) {
        double wd[3] = {0.5, 0.15, 0.35};
        if (std::abs(lastStep) >= 2) { wd[0] = 0.8; wd[1] = 0.05; wd[2] = 0.15; }
        int c = rng.pick(wd, 3), d = 0;
        if (c == 0) d = std::abs(lastStep) >= 2 ? (lastStep > 0 ? -1 : 1) : (rng.chance(0.5) ? 1 : -1);
        else if (c == 2) { d = 2 + rng.irange(3); if (rng.chance(cur > centre ? 0.7 : 0.3)) d = -d; }
        int nd = cur + d;
        if (nd < centre - n / 2 - 1) nd = cur + std::abs(d);
        if (nd > centre + n + 1) nd = cur - std::abs(d);
        return nd;
    };
    auto makePhrase = [&](int startDeg, int endDeg, int centre, double density, double restP, std::vector<Ev>& ev) {
        ev.clear(); int unit = 0, cur = startDeg, lastStep = 0; bool first = true;
        for (int c = 0; c < cpp; c++)
            for (size_t gi = 0; gi < T.groups.size(); gi++) {
                bool closing = c == cpp - 1 && gi + 1 == T.groups.size();
                std::vector<int> r; groupRhythm(T.groups[gi], density, closing, r);
                for (size_t k = 0; k < r.size(); k++) {
                    bool last = closing && k + 1 == r.size();
                    int deg = first ? startDeg : (last ? endDeg : nextDegree(cur, lastStep, centre));
                    bool rest = !first && !last && r[k] <= sub && rng.chance(restP);   // a rest takes a beat at most
                    ev.push_back({unit, r[k], deg, rest});
                    if (!rest) { lastStep = deg - cur; cur = deg; }
                    first = false; unit += r[k];
                }
            }
    };
    // a variation: some degrees moved by one, some notes split in two with a neighbour, the ending as asked
    auto vary = [&](const std::vector<Ev>& a, int endDeg, std::vector<Ev>& out) {
        out.clear();
        for (size_t i = 0; i < a.size(); i++) {
            Ev e = a[i];
            if (i > 0 && i + 1 < a.size() && !e.rest && rng.chance(0.3)) e.deg += rng.chance(0.5) ? 1 : -1;
            if (i + 1 == a.size()) e.deg = endDeg;
            if (!e.rest && e.len >= 2 && e.len % 2 == 0 && i + 1 < a.size() && rng.chance(0.2)) {
                out.push_back({e.unit, e.len / 2, e.deg, false});
                out.push_back({e.unit + e.len / 2, e.len / 2, e.deg + (rng.chance(0.5) ? 1 : -1), false});
            } else out.push_back(e);
        }
    };
    std::vector<Ev> A, A2, B, A3;
    makePhrase(0, T.rest, 0, F.density, F.restP, A);                                        // the statement, resting open
    vary(A, 0, A2);                                                                          // its variation, closing on the tonic
    makePhrase(rng.chance(0.5) ? n : T.rest, T.rest, 2, F.density * 1.1, F.restP, B);       // the answer, higher
    A3 = A; A3.back().deg = 0;                                                               // the return, closed
    const int nv = (int)T.voices.size();
    const int answerVoice = T.second == 0 && nv >= 2 ? 1 : 0, variationVoice = !T.drone && nv >= 3 ? 2 : 0;   // the timbres take turns when they have no part of their own
    struct Ph { const std::vector<Ev>* ev; int voice; };
    std::vector<Ph> order = {{&A, 0}, {&A2, variationVoice}, {&B, answerVoice}, {&A3, 0}};
    if (F.repeat) { order.push_back({&B, answerVoice}); order.push_back({&A3, 0}); }
    const int phraseBeats = cpp * beats;
    double pos = 0;
    for (const Ph& ph : order) {
        for (const Ev& e : *ph.ev) {
            if (e.rest) continue;
            Note nt; nt.t = pos + (double)e.unit / sub; nt.dur = (double)e.len / sub; nt.voice = ph.voice; nt.degree = e.deg + reg;
            int uc = e.unit % cycleUnits;
            nt.vel = rng.range(F.vel0, F.vel1) * (uc == 0 ? 1.0 : (isGroupStart(uc) ? 0.92 : 0.82));
            P.notes.push_back(nt);
        }
        pos += phraseBeats;
    }
    { Note nt; nt.t = pos; nt.dur = beats; nt.voice = 0; nt.degree = reg; nt.vel = F.vel1 * 0.9; P.notes.push_back(nt); }   // the close: the tonic held a cycle
    pos += beats;
    P.beats = pos; P.cycles = (int)std::lround(pos / beats);
    // the ornaments, on the lead by the tradition's rule
    if (T.ornament != 0) {
        std::vector<Note> lead; lead.swap(P.notes);
        int prevDeg = -1000; double prevEnd = -1;
        for (size_t i = 0; i < lead.size(); i++) {
            Note nt = lead[i];
            bool adjacent = prevDeg != -1000 && std::fabs(nt.t - prevEnd) < 1e-6;
            int step = prevDeg == -1000 ? 0 : nt.degree - prevDeg;
            bool orn = nt.voice == 0 && i + 1 < lead.size() && rng.chance(T.ornamentChance);
            if (orn && T.ornament == 1 && std::abs(step) >= 2 && nt.dur >= 0.5) {   // a grace note from the side the leap came from
                Note gr = nt; gr.dur = 0.12; gr.degree = nt.degree + (step > 0 ? -1 : 1); gr.vel *= 0.7; P.notes.push_back(gr);
                nt.t += 0.12; nt.dur -= 0.12; P.notes.push_back(nt);
            } else if (orn && T.ornament == 2 && nt.dur >= 1.5) {   // a trill with the upper neighbour by quarter beats
                double tt = nt.t, end = nt.t + nt.dur; int k = 0;
                while (end - tt > 1e-6) { Note tr = nt; tr.t = tt; tr.dur = end - tt < 0.5 ? end - tt : 0.25; tr.degree = nt.degree + (k % 2); tr.vel *= k == 0 ? 1.0 : 0.8; P.notes.push_back(tr); tt += tr.dur; k++; }
            } else if (orn && T.ornament == 3 && std::abs(step) == 1 && adjacent) {   // a slide from the neighbour
                nt.slideFrom = prevDeg; P.notes.push_back(nt);
            } else if (orn && T.ornament == 4 && nt.dur >= 1.0) {   // a mordent: the note, the one below, the note
                Note m1 = nt; m1.dur = 0.15; P.notes.push_back(m1);
                Note m2 = nt; m2.t += 0.15; m2.dur = 0.15; m2.degree = nt.degree - 1; m2.vel *= 0.8; P.notes.push_back(m2);
                nt.t += 0.3; nt.dur -= 0.3; P.notes.push_back(nt);
            } else P.notes.push_back(nt);
            prevDeg = lead[i].degree; prevEnd = lead[i].t + lead[i].dur;
        }
    }
    // the second voice
    if (nv >= 2 && T.second != 0) {
        std::vector<Note> lead = P.notes;
        for (const Note& l : lead) {
            if (l.voice != 0) continue;
            Note v = l; v.voice = 1; v.slideFrom = -1000;
            if (T.second == 1) { v.degree = l.degree - (n - T.rest); v.vel *= 0.6; }                             // in parallel below, at the fifth's inversion
            else if (T.second == 2) { v.t = l.t + phraseBeats; if (v.t + v.dur > P.beats) continue; v.vel *= 0.55; }   // the answer: the lead a phrase later
            else { v.degree = l.degree - n; v.vel *= 0.5; v.t += rng.range(0.0, 0.08); if (v.t + v.dur > P.beats) v.dur = P.beats - v.t; }   // doubling a period below, a little late
            P.notes.push_back(v);
        }
    }
    // the drone: the tonic a period below, held a cycle at a time on the last timbre
    if (T.drone && nv >= 2) {
        int dv = nv - 1;
        for (double t = 0; t < P.beats - 1e-6; t += beats) { Note d; d.t = t; d.dur = beats; d.voice = dv; d.degree = reg - n; d.vel = 0.35; P.notes.push_back(d); }
    }
    // the drum: a pattern drawn once from the cycle's groups, the phrases' last groups filled, one low hit on the close
    if (F.drum == 2 || (F.drum == 1 && T.hasDrum)) {
        struct Hit { int unit, kind; double vel; };
        std::vector<Hit> pat;
        double pOff = F.density >= 1.4 ? 0.7 : 0.45, pSub = F.density >= 1.4 ? 0.35 : 0.15;
        for (int u = 0; u < cycleUnits; u++) {
            if (isGroupStart(u)) pat.push_back({u, 0, u == 0 ? 1.0 : 0.85});
            else if (u % sub == 0) { if (rng.chance(pOff)) pat.push_back({u, 1, 0.6}); }
            else if (rng.chance(pSub)) pat.push_back({u, 1, 0.4});
        }
        int lastCycle = P.cycles - 1, fillFrom = groupStart.back();
        for (int c = 0; c < lastCycle; c++) {
            bool fill = (c + 1) % cpp == 0;
            for (const Hit& h : pat) {
                if (fill && h.unit >= fillFrom) continue;
                Note d; d.t = c * beats + (double)h.unit / sub; d.dur = 0.25; d.voice = -1; d.degree = h.kind; d.vel = h.vel; P.notes.push_back(d);
            }
            if (fill) for (int u = fillFrom; u < cycleUnits; u++) { Note d; d.t = c * beats + (double)u / sub; d.dur = 0.25; d.voice = -1; d.degree = u == fillFrom ? 0 : 1; d.vel = 0.5 + 0.3 * (u - fillFrom) / std::max(1, cycleUnits - fillFrom); P.notes.push_back(d); }
        }
        Note d; d.t = lastCycle * beats; d.dur = 0.25; d.voice = -1; d.degree = 0; d.vel = 1.0; P.notes.push_back(d);
    }
    std::stable_sort(P.notes.begin(), P.notes.end(), [](const Note& a, const Note& b) { return a.t < b.t; });
    P.seconds = P.beats * 60.0 / P.bpm;
}
