// E-10: each age's texture (music_internal.h's `Bld`): the prehistoric's caller and chorus, the medieval's statement and answer,
// the industrial's melody over chords, the modern's song over a groove, the futuristic's phasing cells, the far future's tones
#include "music_internal.h"

namespace {
// a tempo map over the notes: `rate(b)` the tempo at beat b against the piece's (above one quicker); the notes' starts and lengths move
// to the clock's beats (a dance that quickens, a music box winding down, a song's last bars held back)
template <class F> void warp(Piece& P, double total, F rate) {
    const double step = 0.05; std::vector<double> clock(1, 0.0);
    for (double b = 0; b < total + 8; b += step) clock.push_back(clock.back() + step / std::max(0.2, rate(b + step * 0.5)));
    auto at = [&](double b) {
        if (b <= 0) return 0.0;
        const double x = b / step;
        const size_t i = std::min((size_t)x, clock.size() - 2);
        const double f = x - i;
        return clock[i] + (clock[i + 1] - clock[i]) * f;
    };
    for (Note& nt : P.notes) { const double s = at(nt.t), e = at(nt.t + nt.dur); nt.t = s; nt.dur = std::max(0.02, e - s); }
}
}   // namespace

// ---- the prehistoric: a caller and a chorus, a cell danced faster and faster, a voice alone ----

namespace {
// the drums of a cycle at `pos`: the low skin on the groups' first beats, the high on the other beats, the rattle on the subdivisions, the
// sticks on the beats, the log drum's two notes in a pattern of its own; `loud` the piece's level, `fill` the cycle's end crowded
void stoneDrums(Bld& b, double pos, double loud, bool fill, const std::vector<int>& logPat) {
    const int low = b.E.kitOf(KR_LOW), high = b.E.kitOf(KR_HIGH), shake = b.E.kitOf(KR_SHAKE), tick = b.E.kitOf(KR_TICK);
    int tuned[2] = {-1, -1}; { int k = 0; for (size_t i = 0; i < b.E.kitRole.size() && k < 2; i++) if (b.E.kitRole[i] == KR_TUNED) tuned[k++] = (int)i; }
    for (int u = 0; u < b.cu; u++) {
        const double t = pos + (double)u / b.sub;
        const bool g = b.isGS(u), beat = u % b.sub == 0;
        if (g) b.hit(t, low, loud * (u == 0 ? 1.0 : 0.85));
        else if (beat && high >= 0 && (fill || b.rng.chance(0.6))) b.hit(t, high, loud * 0.7);
        else if (fill && high >= 0 && b.rng.chance(0.5)) b.hit(t, high, loud * 0.55);
        if (shake >= 0) b.hit(t, shake, loud * (beat ? 0.55 : 0.35));
        if (tick >= 0 && beat && (g || b.rng.chance(0.35))) b.hit(t, tick, loud * 0.6);
        if (tuned[0] >= 0 && u < (int)logPat.size() && logPat[u] >= 0) b.hit(t, logPat[u] == 1 && tuned[1] >= 0 ? tuned[1] : tuned[0], loud * 0.75);
    }
}
}   // namespace

void buildPrehistoric(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng;
    const int caller = E.voiceOf(MR_LEAD), chorus = E.voiceOf(MR_CHORUS), flute = E.voiceOf(MR_ALT), bow = E.voiceOf(MR_SECOND), roar = E.voiceOf(MR_DRONE);
    const int n = b.n, cyc = b.beats;
    std::vector<int> logPat(b.cu, -1);   // the log drum's two notes: the tonic on the cycle's first beat, the rest on some of the other groups
    for (int u = 0; u < b.cu; u++) if (b.isGS(u)) logPat[u] = u == 0 ? 0 : (rng.chance(0.6) ? 1 : -1); else if (u % b.sub == 0 && rng.chance(0.25)) logPat[u] = rng.irange(2);
    std::vector<Ev> A, R, A2;
    double pos = 0;
    // the refrain: the people's own (the tradition's seed), so every chant of theirs answers alike
    Rng rr((uint64_t)T.refrain);
    auto refrain = [&](std::vector<Ev>& out) { Rng keep = b.rng; b.rng = rr; b.phrase(T.rest, 0, 0, 0.9, 0.04, b.cpp, out); b.rng = keep; };
    switch (form) {
        case PF_CHANT: case PF_WORK: case PF_HEALING: case PF_WARNING: case PF_HARVEST: case PF_FESTIVAL: {
            const bool healing = form == PF_HEALING, war = form == PF_WARNING, work = form == PF_WORK;
            const double loud = war ? 1.0 : (healing ? 0.6 : 0.82);
            const int intro = rng.chance(0.6) ? 1 : 2, pairs = b.bySeconds(rng.range(45, 70), 2.0 * b.cpp * cyc, 3, 10);
            for (int c = 0; c < intro * b.cpp; c++) { stoneDrums(b, pos, loud * 0.9, false, logPat); pos += cyc; }
            refrain(R);
            if (healing && roar >= 0) b.add(0, 1e3, roar, 0, 0.7);   // trimmed below to the piece's end
            for (int p = 0; p < pairs; p++) {
                if (p % 2 == 0) b.phrase(0, T.rest, 0, war ? 1.4 : 1.05, 0.06, b.cpp, A); else b.vary(A, T.rest, A2);
                const std::vector<Ev>& call = p % 2 == 0 ? A : A2;
                b.place(call, pos, caller, war ? 0.8 : 0.6, war ? 1.0 : 0.85, 0, true, war ? 0.6 : 1.0);
                if (rng.chance(0.3) && flute >= 0 && !war) b.place(call, pos, flute, 0.35, 0.5, n, false);   // the flute shadows the caller an octave up
                for (int c = 0; c < b.cpp; c++) stoneDrums(b, pos + c * cyc, loud * 0.85, false, logPat);
                pos += b.cpp * cyc;
                b.place(R, pos, chorus, war ? 0.85 : 0.65, war ? 1.0 : 0.85, 0, true, war ? 0.6 : 1.0);
                if (T.octaves) b.place(R, pos, chorus, 0.4, 0.55, -n, true, war ? 0.6 : 1.0);   // the low voices an octave under the answer
                if (healing && roar < 0) b.add(pos, b.cpp * cyc, chorus, -n, 0.35, -1000, 4);   // the chorus's hum under its answer
                for (int c = 0; c < b.cpp; c++) stoneDrums(b, pos + c * cyc, loud, c + 1 == b.cpp && p % 2 == 1, logPat);
                // the stroke
                if (work) for (int c = 0; c < b.cpp; c++) for (int gsu : b.gs) b.hit(pos + c * cyc + (double)gsu / b.sub, E.kitOf(KR_TICK) >= 0 ? E.kitOf(KR_TICK) : E.kitOf(KR_LOW), 0.9);
                pos += b.cpp * cyc;
            }
            b.add(pos, cyc, chorus, 0, 0.8, -1000, b.vowel());   // the chorus holds the tonic
            b.hit(pos, E.kitOf(KR_LOW), 1.0);
            pos += cyc;
            if (form == PF_FESTIVAL || form == PF_HARVEST) warp(b.P, pos, [&](double x) { return 1 + 0.18 * x / std::max(1.0, pos); });
            break;
        }
        case PF_RINGDANCE: case PF_DANCE: {   // a one-cycle cell over the drums, round and round, faster and faster
            const int intro = 2;
            for (int c = 0; c < intro; c++) { stoneDrums(b, pos, 0.85, false, logPat); pos += cyc; }
            const int lead = rng.chance(0.55) && flute >= 0 ? flute : chorus;
            b.phrase(0, T.rest, 0, 1.25, 0.05, 1, A);
            std::vector<Ev> B; b.phrase(T.rest, 0, 1, 1.3, 0.05, 1, B);
            const int rounds = b.bySeconds(rng.range(38, 60), cyc, 8, 28);
            for (int k = 0; k < rounds; k++) {
                const std::vector<Ev>* cell = &A;
                if (k % 4 == 2) cell = &B;
                if (k % 4 == 3) { b.vary(A, 0, A2); cell = &A2; }
                b.place(*cell, pos, lead, 0.62, 0.9, lead == flute ? n : 0, lead == chorus, 0.9);
                if (k >= rounds / 2 && bow >= 0) b.place(*cell, pos, bow, 0.4, 0.6, -n, false);
                else if (k >= rounds / 2 && lead == flute) b.place(*cell, pos, chorus, 0.4, 0.6, 0, true);
                stoneDrums(b, pos, 0.75 + 0.25 * k / rounds, k % 4 == 3, logPat);
                pos += cyc;
            }
            b.hit(pos, E.kitOf(KR_LOW), 1.0); b.add(pos, 0.5, chorus, n, 0.9, -1000, 0);   // the last stamp and the shout
            pos += 1;
            const double total = pos, accel = rng.range(0.2, 0.38);
            warp(b.P, total, [&](double x) { return 1 + accel * std::min(1.0, x / total); });
            break;
        }
        case PF_HUNT: {   // the flute (or the bow) alone, calls high and long, answers low; sticks or the log drum ticking softly
            const int lead = flute >= 0 ? flute : caller;
            const int phrases = b.bySeconds(rng.range(35, 60), (double)b.cpp * cyc, 3, 9);
            for (int p = 0; p < phrases; p++) {
                const bool high = p % 2 == 0;
                b.phrase(high ? n : 0, high ? n + T.rest : 0, high ? n : 0, 0.7, 0.12, b.cpp, A);
                b.place(A, pos, lead, 0.55, 0.85, 0, lead == caller, 1.0);
                if (!high && bow >= 0 && rng.chance(0.5)) b.add(pos, b.cpp * cyc, bow, -n, 0.4);
                for (int c = 0; c < b.cpp; c++) for (int u = 0; u < b.cu; u += b.sub) {
                    const int tk = E.kitOf(KR_TUNED) >= 0 ? E.kitOf(KR_TUNED) : E.kitOf(KR_TICK);
                    if (b.isGS(u) || rng.chance(0.3)) b.hit(pos + c * cyc + (double)u / b.sub, tk >= 0 ? tk : E.kitOf(KR_LOW), 0.45);
                }
                pos += b.cpp * cyc;
            }
            b.add(pos, cyc, lead, 0, 0.6);
            pos += cyc;
            break;
        }
        case PF_KEENING: case PF_LAMENT: case PF_LAST: {   // voices sliding down, the chorus humming the tonic, a slow heartbeat on the low skin or none
            const int phrases = b.bySeconds(rng.range(40, 65), (double)b.cpp * cyc, 3, 8);
            const bool beat = rng.chance(0.5) && form != PF_LAST;
            for (int p = 0; p < phrases; p++) {
                b.phrase(p == 0 ? n : n - 1 - rng.irange(2), p % 2 ? 0 : T.rest, n / 2, 0.55, 0.12, b.cpp, A);
                const int v = p % 2 == 0 ? caller : chorus;
                int vw = b.vowel();
                for (const Ev& e : A) { if (e.rest) continue; b.add(pos + (double)e.unit / b.sub, (double)e.len / b.sub, v, e.deg, rng.range(0.65, 0.9), e.deg + 1 + rng.irange(2), vw); }
                b.add(pos, b.cpp * cyc, chorus, -n, 0.5, -1000, 4);
                if (beat) for (int c = 0; c < b.cpp; c++) b.hit(pos + c * cyc, E.kitOf(KR_LOW), 0.5);
                pos += b.cpp * cyc;
            }
            b.add(pos, 2 * cyc, chorus, 0, 0.5, 1, 4);
            pos += 2 * cyc;
            break;
        }
        default: {   // a lullaby: the caller soft and high, rocking between two notes, a rattle shaken gently
            const int phrases = b.bySeconds(rng.range(35, 55), (double)b.cpp * cyc, 3, 10);
            const int other = rng.chance(0.5) ? 1 : T.rest;
            for (int p = 0; p < phrases; p++) {
                int vw = b.vowel(); int unit = 0;
                for (int c = 0; c < b.cpp; c++) for (size_t gi = 0; gi < b.T.groups.size(); gi++) {
                    const int len = b.T.groups[gi] * b.sub;
                    const bool last = c == b.cpp - 1 && gi + 1 == b.T.groups.size();
                    const int deg = last ? (p % 2 ? 0 : other) : ((gi + p) % 2 ? other : 0) + (rng.chance(0.2) ? 2 : 0);
                    const int l1 = len > b.sub && !last ? len - b.sub : len;
                    b.add(pos + (double)unit / b.sub, (double)l1 / b.sub, caller, n + deg, rng.range(0.38, 0.55), -1000, vw);
                    if (l1 < len) b.add(pos + (double)(unit + l1) / b.sub, (double)(len - l1) / b.sub, caller, n + deg + 1, 0.4, -1000, vw);
                    if (E.kitOf(KR_SHAKE) >= 0) b.hit(pos + (double)unit / b.sub, E.kitOf(KR_SHAKE), 0.25);
                    unit += len;
                }
                pos += b.cpp * cyc;
            }
            b.add(pos, cyc, caller, n, 0.4, -1000, 4);
            pos += cyc;
            const double total = pos;
            warp(b.P, total, [&](double x) { return x < total * 0.75 ? 1.0 : 1 - 0.35 * (x - total * 0.75) / (total * 0.25); });
            break;
        }
    }
    if (form != PF_RINGDANCE && form != PF_DANCE && form != PF_HUNT && form != PF_KEENING && form != PF_LAMENT && form != PF_LAST) b.ornament(caller, 0.8);
    else b.ornament(caller, 0.5);
}

// ---- the medieval: C-04's statement, variation, answer, return and close, its drone and its tabor; a choir in organum for a hymn ----

void buildMedieval(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n, sub = b.sub, beats = b.beats, cycleUnits = b.cu, cpp = b.cpp;
    const FormP F = form < 11 ? C04_FP[form] : (form == PF_PROCESSION ? FormP{0.75, 0.7, 0.55, 0.8, 0.06, -1, 2, false} : FormP{0.9, 0.95, 0.5, 0.78, 0.08, 0, 1, true});   // a procession; a ballad
    const int reg = F.reg * (n / 2);
    const int nv = (int)T.voices.size();
    const int choir = E.voiceOf(MR_CHORUS), solo = E.voiceOf(MR_SINGER), drone = E.voiceOf(MR_DRONE), bell = E.voiceOf(MR_BELL) >= 0 ? E.voiceOf(MR_BELL) : -1;
    int bellV = bell; for (int i = 0; i < nv && bellV < 0; i++) if (T.voices[i].family == TF_METAL) bellV = i;
    // who sings: a hymn and a procession the choir, a love song, a ballad and a lullaby a voice now and then
    int leadV = 0;
    if ((form == PF_HYMN || form == PF_PROCESSION) && choir >= 0 && rng.chance(0.75)) leadV = choir;
    else if ((form == PF_LOVE || form == PF_BALLAD || form == PF_LULLABY || form == PF_LAMENT) && solo >= 0 && rng.chance(0.55)) leadV = solo;
    const bool sung = leadV == choir || leadV == solo;
    std::vector<Ev> A, A2, B, A3;
    b.phrase(0, T.rest, 0, F.density, F.restP, cpp, A);
    b.vary(A, 0, A2);
    b.phrase(rng.chance(0.5) ? n : T.rest, T.rest, 2, F.density * 1.1, F.restP, cpp, B);
    A3 = A; A3.back().deg = 0;
    const int answerVoice = sung ? (nv >= 1 ? 0 : leadV) : (T.second == 0 && nv >= 2 ? 1 : 0), variationVoice = sung ? leadV : (drone < 0 && nv >= 3 ? 2 : 0);
    struct Ph { const std::vector<Ev>* ev; int voice; };
    std::vector<Ph> order = {{&A, leadV}, {&A2, variationVoice}, {&B, answerVoice}, {&A3, leadV}};
    if (F.repeat) { order.push_back({&B, answerVoice}); order.push_back({&A3, leadV}); }
    const int phraseBeats = cpp * beats;
    // a short form comes round again (C-04's pieces ran 28-70 s): its statement varied anew, its answer moved, the return closing
    std::vector<Ev> A4, B2;
    {
        const double est = (order.size() * phraseBeats + beats) * 60.0 / std::max(40.0, P.bpm);
        if (est < 34) {
            b.vary(A, T.rest, A4); b.vary(B, T.rest, B2);
            const size_t first = order.size();
            order.push_back({&A4, leadV}); order.push_back({&B2, answerVoice}); order.push_back({&A3, leadV});
            if (est < 20) for (size_t k = 0; k < first; k++) order.push_back(order[k]);
        }
    }
    double pos = 0;
    int vw = b.vowel();
    for (const Ph& ph : order) {
        for (const Ev& e : *ph.ev) {
            if (e.rest) continue;
            const bool s = ph.voice == choir || ph.voice == solo;
            if (s && rng.chance(0.4)) vw = b.vowel();
            b.add(pos + (double)e.unit / sub, (double)e.len / sub, ph.voice, e.deg + reg, rng.range(F.vel0, F.vel1) * b.accent(e.unit), -1000, s ? vw : -1);
        }
        if (bellV >= 0 && (form == PF_HYMN || form == PF_PROCESSION || form == PF_LAMENT))
            b.add(pos + phraseBeats - (double)T.groups.back(), (double)T.groups.back(), bellV, reg + (rng.chance(0.5) ? 0 : T.rest), 0.55);   // a bell at the phrase's end
        pos += phraseBeats;
    }
    b.add(pos, beats, leadV, reg, F.vel1 * 0.9, -1000, sung ? b.vowel() : -1);   // the close: the tonic held a cycle
    pos += beats;
    P.beats = pos;
    b.ornament(leadV, sung ? 0.6 : 1.0);
    // the second voice: organum for a sung hymn (the choir again, the fifth's inversion below), else C-04's
    if (sung && leadV == choir && (form == PF_HYMN || form == PF_PROCESSION)) {
        std::vector<Note> lead = P.notes;
        for (const Note& l : lead) if (l.voice == choir) { Note v = l; v.degree = l.degree - (n - T.rest); v.vel *= 0.7; v.slideFrom = -1000; P.notes.push_back(v); }
    } else if (nv >= 2 && T.second != 0) {
        std::vector<Note> lead = P.notes;
        for (const Note& l : lead) {
            if (l.voice != leadV) continue;
            Note v = l; v.voice = sung ? 0 : 1; v.slideFrom = -1000; v.vowel = -1;
            if (T.second == 1) { v.degree = l.degree - (n - T.rest); v.vel *= 0.6; }
            else if (T.second == 2) { v.t = l.t + phraseBeats; if (v.t + v.dur > pos) continue; v.vel *= 0.55; }
            else { v.degree = l.degree - n; v.vel *= 0.5; v.t += rng.range(0.0, 0.08); if (v.t + v.dur > pos) v.dur = pos - v.t; }
            if (v.dur > 0.02) P.notes.push_back(v);
        }
    }
    // the drone: the tonic a period below a cycle at a time, on the drone if the people had one, else C-04's last timbre
    if (T.drone && (drone >= 0 || nv >= 2)) {
        const int dv = drone >= 0 ? drone : nv - 1;
        for (double t = 0; t < pos - 1e-6; t += beats) b.add(t, beats, dv, reg - n, 0.35);
    }
    // the tabor: C-04's pattern drawn once from the cycle's groups, the phrases' last groups filled, one low hit on the close; jingles in a dance
    if (F.drum == 2 || (F.drum == 1 && T.hasDrum)) {
        const int low = E.kitOf(KR_LOW), high = E.kitOf(KR_HIGH), jing = E.kitOf(KR_SHAKE);
        struct Hit { int unit, kind; double vel; };
        std::vector<Hit> pat;
        double pOff = F.density >= 1.4 ? 0.7 : 0.45, pSub = F.density >= 1.4 ? 0.35 : 0.15;
        for (int u = 0; u < cycleUnits; u++) {
            if (b.isGS(u)) pat.push_back({u, 0, u == 0 ? 1.0 : 0.85});
            else if (u % sub == 0) { if (rng.chance(pOff)) pat.push_back({u, 1, 0.6}); }
            else if (rng.chance(pSub)) pat.push_back({u, 1, 0.4});
        }
        const int cycles = (int)std::lround(pos / beats), lastCycle = cycles - 1, fillFrom = b.gs.back();
        for (int c = 0; c < lastCycle; c++) {
            bool fill = (c + 1) % cpp == 0;
            for (const Hit& h : pat) {
                if (fill && h.unit >= fillFrom) continue;
                b.hit(c * beats + (double)h.unit / sub, h.kind ? high : low, h.vel);
            }
            if (fill) for (int u = fillFrom; u < cycleUnits; u++) b.hit(c * beats + (double)u / sub, u == fillFrom ? low : high, 0.5 + 0.3 * (u - fillFrom) / std::max(1, cycleUnits - fillFrom));
            if (jing >= 0 && (form == PF_DANCE || form == PF_FESTIVAL || form == PF_HARVEST)) for (int u = 0; u < cycleUnits; u += sub) if (!b.isGS(u)) b.hit(c * beats + (double)u / sub, jing, 0.5);
        }
        b.hit(lastCycle * beats, low, 1.0);
    }
}

// ---- the industrial: a melody over chords of the people's scale, a bass, a band's drums; an organ, a music box, a parlour's keys ----

void buildIndustrial(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n, sub = b.sub, cyc = b.beats;
    int lead = E.voiceOf(MR_LEAD), second = E.voiceOf(MR_SECOND), chords = E.voiceOf(MR_CHORDS), bass = E.voiceOf(MR_BASS);
    const int organV = E.voiceOf(MR_ORGAN), box = E.voiceOf(MR_BOX), singerV = E.voiceOf(MR_SINGER), keysV = E.voiceOf(MR_KEYS) >= 0 ? E.voiceOf(MR_KEYS) : (E.band == 1 ? E.voiceOf(MR_CHORDS) : -1);
    const int bd = E.kitOf(KR_LOW), sn = E.kitOf(KR_BACK), cy = E.kitOf(KR_CRASH);
    bool march = form == PF_MARCH || form == PF_FUNERAL || form == PF_WARNING || form == PF_OVERTURE || form == PF_FESTIVAL || form == PF_WORK;
    const bool turning = form == PF_TURNING || form == PF_DANCE || form == PF_HARVEST || form == PF_LOVE;
    const bool anthem = form == PF_ANTHEM, hymn = form == PF_HYMN || form == PF_LAMENT || form == PF_LAST, parlour = form == PF_PARLOUR || form == PF_LULLABY || form == PF_BALLAD;
    const bool musicbox = form == PF_MUSICBOX;
    if (hymn && organV >= 0) { lead = organV; second = organV; chords = organV; bass = organV; }
    if (parlour) { if (singerV >= 0 && rng.chance(0.6)) lead = singerV; if (keysV >= 0) { chords = keysV; bass = keysV; } }
    if (musicbox && box >= 0) { lead = box; second = -1; chords = box; bass = box; }
    const bool sung = lead == singerV;
    // the strains: a phrase of one or two cycles, each with its chords (one a cycle, two in a long cycle); A A B B A, a coda
    // a strain of about eleven seconds (two phrases), the piece five or six of them: a minute or so whatever the cycle and the tempo
    const int strainCycles = b.bySeconds(anthem || form == PF_OVERTURE ? 8.5 : 11.0, cyc, 1, 8), phraseCycles = std::max(1, strainCycles / 2), strainPhrases = strainCycles / phraseCycles;
    const bool twoChords = cyc >= 6;
    Rng fav(mix64(T.seed ^ 0x1D05C0ULL));   // the people's own turns, which their tunes come back to
    std::vector<int> progA =
            rng.chance(0.6) ? b.progression(phraseCycles * strainPhrases * (twoChords ? 2 : 1), false, fav) : b.progression(phraseCycles * strainPhrases * (twoChords ? 2 : 1), false, rng);
    std::vector<int> progA2 = progA; progA2.back() = 0;
    std::vector<int> progB = b.progression((int)progA.size(), false, rng); progB[0] = b.roots.size() > 1 ? b.roots[1] : T.rest;
    struct Strain { const std::vector<int>* prog; int centre; bool closed; };
    std::vector<Strain> strains = {{&progA, 0, false}, {&progA2, 0, true}, {&progB, 2, false}, {&progA2, 0, true}};
    if (form == PF_OVERTURE || anthem) strains.push_back({&progB, 2, false}), strains.push_back({&progA2, 0, true});
    if (musicbox || parlour) strains.resize(3), strains[2] = {&progA2, 0, true};
    const double tempoVel = form == PF_FUNERAL ? 0.75 : 1.0;
    double pos = 0;
    const int chordsPerCycle = twoChords ? 2 : 1;
    const int half = twoChords ? b.midGroup() : b.cu;
    std::vector<Ev> mel, mel0;
    int vw = b.vowel();
    // an introduction: the last strain's chords, a cycle
    if (!musicbox) {
        int c3[3]; b.chord(T.rest, c3);
        for (int k = 0; k < 3; k++) b.add(0, cyc, chords, c3[k] - n / 2, 0.45 * tempoVel);
        b.add(0, cyc, bass, T.rest - 2 * n + (hymn ? n : 0), 0.6);
        if (march && sn >= 0) for (int u = 0; u < b.cu; u++) b.hit((double)u / sub, sn, 0.35 + 0.4 * u / b.cu);
        pos += cyc;
    }
    for (size_t si = 0; si < strains.size(); si++) {
        const Strain& S = strains[si];
        const int cycles = phraseCycles * strainPhrases;
        // the tune: two phrases over the strain (the first open, the second as the strain ends), its notes on the groups' first beats moved
        // onto the chord sounding; the strains that return vary the first
        {
            const double dens = (musicbox ? 1.25 : (anthem || hymn ? 0.6 : 1.0)) * T.density;
            std::vector<Ev> p1, p2;
            b.phrase(S.centre == 0 ? 0 : n, T.rest, S.centre, dens, 0.06, phraseCycles, p1);
            b.phrase(T.rest, S.closed ? 0 : T.rest, S.centre, dens, 0.06, phraseCycles * (strainPhrases - 1) > 0 ? phraseCycles * (strainPhrases - 1) : phraseCycles, p2);
            mel = p1; const int off = phraseCycles * b.cu; if (strainPhrases > 1) for (Ev e : p2) { e.unit += off; mel.push_back(e); }
        }
        if ((si == 1 || si == 3) && !mel0.empty()) b.vary(mel0, S.closed ? 0 : T.rest, mel);
        if (si == 0) mel0 = mel;
        for (Ev& e : mel) {
            const int cIdx = std::min((int)S.prog->size() - 1, (e.unit / b.cu) * chordsPerCycle + (twoChords && (e.unit % b.cu) >= half ? 1 : 0));
            if (b.isGS(e.unit) && !e.rest) e.deg = b.nearestChordTone(e.deg, (*S.prog)[cIdx]);
        }
        const int reg = musicbox ? n : (hymn ? 0 : (sung ? 0 : n / 2));
        for (const Ev& e : mel) {
            if (e.rest) continue;
            if (sung && rng.chance(0.45)) vw = b.vowel();
            b.add(pos + (double)e.unit / sub, (double)e.len / sub * (march && !anthem ? 0.85 : 1.0), lead, e.deg + reg, rng.range(0.62, 0.9) * b.accent(e.unit) * tempoVel, -1000, sung ? vw : -1);
            if (second >= 0 && second != lead && !musicbox && (b.isGS(e.unit) || e.len >= 2 * sub) && rng.chance(anthem ? 0.9 : 0.6)) {   // the second: a chord tone under the tune
                const int cIdx = std::min((int)S.prog->size() - 1, (e.unit / b.cu) * chordsPerCycle + (twoChords && (e.unit % b.cu) >= half ? 1 : 0));
                int c3[3]; b.chord((*S.prog)[cIdx], c3);
                int under = c3[0]; for (int k = 0; k < 3; k++) { int d = c3[k]; while (d >= e.deg) d -= n; while (d < e.deg - n) d += n; if (d > under || under >= e.deg) under = d; }
                b.add(pos + (double)e.unit / sub, (double)e.len / sub, second, under + reg, 0.5 * tempoVel);
            }
        }
        // the chords and the bass, chord by chord
        for (int c = 0; c < cycles; c++)
            for (int h = 0; h < chordsPerCycle; h++) {
                const int root = (*S.prog)[std::min((int)S.prog->size() - 1, c * chordsPerCycle + h)];
                int c3[3]; b.chord(root, c3);
                const int u0 = h == 0 ? 0 : half, u1 = h + 1 < chordsPerCycle ? half : b.cu;
                const double t0 = pos + c * cyc;
                const int creg = musicbox ? 0 : (hymn ? -n / 2 : -n / 2), breg = musicbox ? -n : -2 * n + (hymn ? n : 0);
                if (march) {   // the bass on the groups' first beats, root and fifth by turns; the chords on the other beats
                    bool alt = false;
                    for (int u = u0; u < u1; u += sub) {
                        if (b.isGS(u)) { b.add(t0 + (double)u / sub, 0.9, bass, (alt ? c3[2] : c3[0]) + breg, 0.75 * tempoVel); alt = !alt; }
                        else for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, 0.6, chords, c3[k] + creg, 0.4 * tempoVel);
                    }
                } else if (turning) {   // the bass once a cycle, the chords on the other groups' first beats, held
                    for (int u = u0; u < u1; u++) {
                        if (!b.isGS(u)) continue;
                        if (u == 0) b.add(t0, 1.0, bass, c3[0] + breg, 0.7);
                        else for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, 0.9, chords, c3[k] + creg, 0.38);
                    }
                } else if (parlour || musicbox) {   // the bass on the chord's first beat, the chord broken in subdivisions
                    b.add(t0 + (double)u0 / sub, std::max(1.0, (double)(u1 - u0) / sub * 0.5), bass, c3[0] + breg, 0.55);
                    const int pat[4] = {0, 2, 1, 2};
                    for (int u = u0 + (musicbox ? sub : 1); u < u1; u += musicbox ? sub : 1)
                        b.add(t0 + (double)u / sub, musicbox ? 0.5 : (double)1 / sub * 1.6, chords, c3[pat[(u - u0) % 4]] + creg, musicbox ? 0.32 : 0.3);
                } else {   // an anthem, a hymn: the chord held on each group, the bass with it
                    for (int u = u0; u < u1; u++) {
                        if (!b.isGS(u)) continue;
                        int ge = u + 1; while (ge < u1 && !b.isGS(ge)) ge++;
                        const double len = (double)(ge - u) / sub;
                        for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, len, chords, c3[k] + creg, (anthem ? 0.5 : 0.42) * tempoVel);
                        b.add(t0 + (double)u / sub, len, bass, c3[0] + breg, (anthem ? 0.7 : 0.55) * tempoVel);
                    }
                }
            }
        // the drums: the bass drum on the groups' first beats, the side drum on the other beats and a roll into the next strain, a crash at its start
        if (march || anthem || (turning && bd >= 0 && rng.chance(0.4))) {
            for (int c = 0; c < cycles; c++) {
                const double t0 = pos + c * cyc;
                for (int u = 0; u < b.cu; u++) {
                    const bool g = b.isGS(u), beat = u % sub == 0;
                    if (g) b.hit(t0 + (double)u / sub, bd, (u == 0 ? 0.95 : 0.8) * tempoVel);
                    else if (march && beat && sn >= 0 && !anthem) b.hit(t0 + (double)u / sub, sn, 0.6 * tempoVel);
                    if (march && sn >= 0 && c == cycles - 1 && u >= b.gs.back()) b.hit(t0 + (double)u / sub, sn, 0.35 + 0.5 * (u - b.gs.back()) / std::max(1, b.cu - b.gs.back()));
                }
                if (c == 0 && cy >= 0) b.hit(t0, cy, anthem ? 0.9 : 0.7);
            }
        }
        pos += cycles * cyc;
    }
    // the close: the tonic chord held a cycle, the bass under it, the bass drum and a crash
    {
        int c3[3]; b.chord(0, c3);
        const int creg = musicbox ? 0 : -n / 2, breg = musicbox ? -n : -2 * n + (hymn ? n : 0);
        b.add(pos, cyc, lead, (musicbox ? n : (hymn ? 0 : n / 2)) + (sung ? -n / 2 : 0), 0.8, -1000, sung ? b.vowel() : -1);
        if (!musicbox) for (int k = 0; k < 3; k++) b.add(pos, cyc, chords, c3[k] + creg, 0.5);
        b.add(pos, cyc, bass, c3[0] + breg, 0.7);
        if (march || anthem) { b.hit(pos, bd, 1.0); b.hit(pos, cy, 0.8); }
        pos += cyc;
    }
    b.ornament(lead, sung ? 0.5 : 0.35);
    const double total = pos;
    if (musicbox) {   // the spring runs down: the last third slows by half, and the box may stop before the tune is done
        const double fall = rng.range(0.4, 0.6);
        warp(P, total, [&](double x) { return x < total * 0.66 ? 1.0 : 1 - fall * (x - total * 0.66) / (total * 0.34); });
        if (rng.chance(0.5)) { std::vector<Note> keep; const double cut = P.notes.empty() ? 0 : total * 0.93; for (const Note& nt : P.notes) if (nt.t < cut) keep.push_back(nt); P.notes.swap(keep); }
    } else if (parlour || hymn || anthem) warp(P, total, [&](double x) { return x < total - cyc * 1.5 ? 1.0 : 1 - 0.3 * (x - (total - cyc * 1.5)) / (cyc * 1.5); });
}

// ---- the modern: a song's sections over a groove of the people's cycle; a dance record that builds and breaks; a ballad, a jingle ----

namespace {
void groove(Bld& b, double t0, int cycles, double level, int kind, bool fillEnd, bool crashStart) {
    // kind: 0 a song's (the kick on the groups' first beats, the back on the middle group, the hats on the subdivisions); 1 the floor's (the kick
    // on every beat, the open hat between); 2 half (the kick on the cycle's first beat, the back once); 3 the hats alone
    const Ensemble& E = b.E; const int sub = b.sub, cu = b.cu;
    const int kick = E.kitOf(KR_LOW), back = E.kitOf(KR_BACK), hat = E.kitOf(KR_TICK), open = E.kitOf(KR_OPEN), crash = E.kitOf(KR_CRASH);
    const int mid = b.midGroup();
    for (int c = 0; c < cycles; c++) {
        const double tc = t0 + c * b.beats;
        if (crashStart && c == 0) b.hit(tc, crash, 0.75 * level);
        for (int u = 0; u < cu; u++) {
            const double t = tc + (double)u / sub; const bool beat = u % sub == 0, g = b.isGS(u);
            const bool fill = fillEnd && c == cycles - 1 && u >= b.gs.back();
            if (kind == 0) {
                if (g && u != mid) b.hit(t, kick, (u == 0 ? 1.0 : 0.85) * level);
                else if (u + 1 == mid && b.rng.chance(0.3)) b.hit(t, kick, 0.6 * level);
                if (u == mid || (b.gs.size() > 3 && u == b.gs.back())) b.hit(t, back, 0.85 * level);
                if (fill) b.hit(t, back, (0.4 + 0.4 * (u - b.gs.back()) / std::max(1, cu - b.gs.back())) * level);
                else b.hit(t, hat, (beat ? 0.7 : 0.45) * level);
            } else if (kind == 1) {
                if (beat) b.hit(t, kick, (u == 0 ? 1.0 : 0.9) * level);
                if (!beat && sub == 2) b.hit(t, open, 0.55 * level); else if (!beat) b.hit(t, hat, 0.45 * level);
                if (u == mid) b.hit(t, back, 0.8 * level);
            } else if (kind == 2) {
                if (u == 0) b.hit(t, kick, 0.9 * level);
                if (u == mid) b.hit(t, back, 0.7 * level);
                if (beat) b.hit(t, hat, 0.4 * level);
            } else if (beat) b.hit(t, hat, (g ? 0.6 : 0.4) * level);
        }
    }
}
}   // namespace

void buildModern(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n, sub = b.sub, cyc = b.beats;
    int lead = E.voiceOf(MR_LEAD);
    const int second = E.voiceOf(MR_SECOND), chords = E.voiceOf(MR_CHORDS), bass = E.voiceOf(MR_BASS), padV = E.voiceOf(MR_ALT) >= 0 ? E.voiceOf(MR_ALT) : chords,
              singerV = E.voiceOf(MR_SINGER) >= 0 ? E.voiceOf(MR_SINGER) : (E.voices[lead].family == TF_VOICE ? lead : -1);
    if ((form == PF_BALLAD || form == PF_PROTEST || form == PF_REQUIEM || form == PF_LAMENT || form == PF_LULLABY) && singerV >= 0 && rng.chance(0.75)) lead = singerV;
    const bool sung = E.voices[lead].family == TF_VOICE;
    const bool dance = form == PF_DANCEREC || form == PF_FESTIVAL || form == PF_DANCE, slow = form == PF_BALLAD || form == PF_REQUIEM || form == PF_LAMENT || form == PF_LULLABY || form == PF_LAST;
    const bool jingle = form == PF_JINGLE, theme = form == PF_THEME, anthem = form == PF_ANTHEM;
    // the sections, in cycles: about twelve seconds each (two to eight cycles), so a song runs a minute and a half whatever its cycle and tempo
    const int S = b.bySeconds(12.0, cyc, 1, 8);
    enum { INTRO, VERSE, CHORUS, BRIDGE, OUTRO, BUILD, DROP, BREAK, HOOK };
    struct Sec { int kind, cycles; };
    std::vector<Sec> secs;
    const int H = std::max(1, S / 2), I2 = std::min(2, S);
    if (jingle) { const int J = b.bySeconds(7.0, cyc, 1, 8); secs = {{HOOK, J}, {HOOK, J}, {OUTRO, 1}}; }
    else if (theme) secs = {{INTRO, I2}, {HOOK, S}, {CHORUS, H}, {OUTRO, 1}};
    else if (dance) secs = {{INTRO, H}, {BUILD, S}, {DROP, S}, {BREAK, H}, {BUILD, H}, {DROP, S}, {OUTRO, H}};
    else if (slow) secs = {{INTRO, I2}, {VERSE, S}, {CHORUS, S}, {VERSE, S}, {CHORUS, S}, {OUTRO, I2}};
    else secs = {{INTRO, H}, {VERSE, S}, {CHORUS, S}, {VERSE, S}, {CHORUS, S}, {BRIDGE, H}, {CHORUS, S}, {OUTRO, H}};
    if (!jingle && !theme && !dance && !slow && rng.chance(0.35)) secs.erase(secs.begin() + 5);   // no bridge
    Rng fav(mix64(T.seed ^ 0x1D05C1ULL));
    const int plen = std::min(4, S);
    std::vector<int> progV = rng.chance(0.55) ? b.progression(plen, false, fav) : b.progression(plen, false, rng);
    std::vector<int> progC = b.progression(plen, true, rng); if (progC.size() > 1 && rng.chance(0.5)) progC[0] = b.roots.size() > 1 ? b.roots[1] : T.rest;
    std::vector<int> progB = b.progression(plen, false, rng); progB[0] = b.roots.size() > 2 ? b.roots[2] : T.rest;
    // the hook: the chorus's motif, a cycle long, sung or played twice and varied
    std::vector<Ev> hook, verseA, tmp; b.phrase(n, 0, n, 1.15 * T.density, 0.05, 1, hook);
    const int vc = std::max(1, std::min(2, S / 2));
    b.phrase(0, T.rest, 0, 0.95 * T.density, 0.08, vc, verseA);
    std::vector<Ev> riff; b.phrase(0, 0, 0, 1.6, 0.15, 1, riff);   // the dance record's riff, a cycle of subdivisions
    double pos = 0;
    int vw = b.vowel();
    const double vol = slow ? 0.75 : 1.0;
    for (size_t si = 0; si < secs.size(); si++) {
        const Sec& sc = secs[si];
        const std::vector<int>& prog = sc.kind == CHORUS || sc.kind == HOOK || sc.kind == DROP ? progC : (sc.kind == BRIDGE ? progB : progV);
        const bool full = sc.kind == CHORUS || sc.kind == DROP || sc.kind == HOOK || (sc.kind == BRIDGE && !slow);
        // the drums
        if (sc.kind == INTRO) { if (dance || theme) groove(b, pos, sc.cycles, 0.7, dance ? 3 : 0, true, false); else if (!slow) groove(b, pos, sc.cycles, 0.6, 3, false, false); }
        else if (sc.kind == BREAK || (slow && sc.kind == VERSE && si < 2)) { if (dance) groove(b, pos, sc.cycles, 0.5, 3, false, false); }
        else if (sc.kind == BUILD) groove(b, pos, sc.cycles, 0.8, 0, true, false);
        else if (sc.kind == OUTRO) { if (!slow) groove(b, pos, std::max(1, sc.cycles - 1), 0.6, dance ? 1 : 2, false, false); }
        else groove(b, pos, sc.cycles, vol * (full ? 1.0 : 0.85), dance ? 1 : (slow || (anthem && !full) ? 2 : 0), true, full);   // an anthem's verses at half time
        // the chords and the bass: a chord a cycle, the bass on the cycle's beats with a step into the next root
        for (int c = 0; c < sc.cycles; c++) {
            const int root = prog[c % prog.size()], next = prog[(c + 1) % prog.size()];
            int c3[3]; b.chord(root, c3);
            const double t0 = pos + c * cyc;
            const int chordFam = chords >= 0 ? E.voices[chords].family : TF_PAD;
            if (sc.kind != BREAK || chordFam == TF_PAD) {
                if (chordFam == TF_PAD || (chordFam == TF_ORGAN && E.comp != 1)) for (int k = 0; k < 3; k++) b.add(t0, cyc, chords, c3[k], (full ? 0.6 : 0.48) * vol);
                else if (E.comp == 0 || slow) for (size_t gi = 0; gi < b.gs.size(); gi++) {   // a chord on each group, held to the next
                    const int gsu = b.gs[gi], ge = gi + 1 < b.gs.size() ? b.gs[gi + 1] : b.cu;
                    for (int k = 0; k < 3; k++) b.add(t0 + (double)gsu / sub, (double)(ge - gsu) / sub * 0.9, chords, c3[k], (full ? 0.55 : 0.45) * vol * (gsu == 0 ? 1.0 : 0.85));
                } else for (int u = 0; u < b.cu; u++) {   // 1 on every beat, 2 between the beats, 3 pushed ahead of the groups
                    const bool on = E.comp == 1 ? u % sub == 0 : (E.comp == 2 ? (sub > 1 && u % sub == sub - 1) : (b.isGS((u + 1) % b.cu) || u == 0));
                    if (!on) continue;
                    for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, E.comp == 1 ? 0.8 : 0.45, chords, c3[k], (full ? 0.5 : 0.42) * vol * (u == 0 ? 1.0 : 0.85));
                }
            }
            if (padV >= 0 && padV != chords && (full || slow) && sc.kind != INTRO) for (int k = 0; k < 2; k++) b.add(t0, cyc, padV, c3[k] + n, 0.4 * vol);
            if (sc.kind != INTRO && sc.kind != BREAK && bass >= 0) {
                for (int u = 0; u < b.cu; u++) {
                    const double t = t0 + (double)u / sub;
                    if (dance) { if (u % sub == (sub > 1 ? 1 : 0)) b.add(t, 1.0 / sub * 0.9, bass, c3[0] - n, 0.7); continue; }
                    if (u == 0) b.add(t, slow ? cyc * 0.9 : 0.9, bass, c3[0] - n, 0.8 * vol);
                    else if (!slow && (b.isGS(u) || u == b.midGroup())) b.add(t, 0.8, bass, (rng.chance(0.25) ? c3[2] : c3[0]) - n + (rng.chance(0.15) ? n : 0), 0.7 * vol);
                    else if (!slow && u == b.cu - 1 && root != next) b.add(t, 1.0 / sub, bass, next - n + (next > root ? -1 : 1), 0.55 * vol);   // the step into the next root
                }
            }
        }
        // the lead: the verse's line low, the chorus's hook high and twice, the bridge a new line; the riff in the dance record's drops
        if (sc.kind == VERSE) {
            for (int c = 0; c + vc <= sc.cycles; c += vc) {
                if (c > 0 && rng.chance(0.6)) { b.vary(verseA, c + 2 >= sc.cycles ? T.rest : 0, tmp); b.place(tmp, pos + c * cyc, lead, 0.55, 0.8, 0, sung); }
                else b.place(verseA, pos + c * cyc, lead, 0.55, 0.8, 0, sung);
            }
        } else if (sc.kind == CHORUS || sc.kind == HOOK) {
            for (int c = 0; c < sc.cycles; c++) {
                if (c % 2 == 1) { b.vary(hook, c + 1 == sc.cycles ? 0 : T.rest, tmp); b.place(tmp, pos + c * cyc, lead, 0.65, 0.95, 0, sung); }
                else b.place(hook, pos + c * cyc, lead, 0.65, 0.95, 0, sung);
                if (second >= 0 && second != lead && c % 2 == 0) b.place(hook, pos + c * cyc, second, 0.3, 0.45, -n, false, 0.7);
            }
        } else if (sc.kind == BRIDGE) {
            b.phrase(T.rest, T.rest, n / 2, 0.8, 0.1, sc.cycles, tmp); b.place(tmp, pos, lead, 0.55, 0.8, 0, sung);
        } else if (sc.kind == DROP || sc.kind == BUILD || sc.kind == BREAK) {
            const int rv = second >= 0 ? second : lead;
            for (int c = 0; c < sc.cycles; c++)
                b.place(riff, pos + c * cyc, rv, sc.kind == BUILD ? 0.45 + 0.4 * c / sc.cycles : 0.7, sc.kind == BUILD ? 0.55 + 0.4 * c / sc.cycles : 0.9, 0, false, 0.8);
            if (sc.kind == DROP && sung) for (int c = 0; c < sc.cycles; c += 2) b.place(hook, pos + c * cyc, lead, 0.6, 0.85, 0, true);
        } else if (sc.kind == INTRO && (theme || jingle)) {
            b.phrase(n, n + T.rest, n, 1.0, 0.0, 1, tmp); b.place(tmp, pos, second >= 0 ? second : lead, 0.7, 0.95, 0, false);
        }
        if (sung) for (Note& nt : P.notes) if (nt.voice == lead && nt.vowel < 0) { if (rng.chance(0.4)) vw = b.vowel(); nt.vowel = vw; }
        pos += sc.cycles * cyc;
    }
    // the end: the tonic chord rung out, a crash (a jingle's sting)
    {
        int c3[3];
        b.chord(0, c3);
        for (int k = 0; k < 3; k++) b.add(pos, cyc, chords, c3[k], 0.55);
        if (bass >= 0) b.add(pos, cyc, bass, -n, 0.75);
        if (!slow) {
            b.hit(pos, E.kitOf(KR_LOW), 0.9);
            b.hit(pos, E.kitOf(KR_CRASH), 0.8);
        }
        pos += cyc;
    }
    if (sung) b.ornament(lead, 0.3);
    if (slow) { const double total = pos; warp(P, total, [&](double x) { return x < total - 2 * cyc ? 1.0 : 1 - 0.3 * (x - (total - 2 * cyc)) / (2 * cyc); }); }
}

// ---- the futuristic: a cell phasing against itself, swarms gliding between chords, a synthetic voice over an arpeggiator ----

namespace {
double microBend(const Bld& b, int deg, Rng& rng) {   // a microtone: half the way to the next note of the scale, up or down
    const double up = traditionCents(b.T, deg + 1) - traditionCents(b.T, deg), dn = traditionCents(b.T, deg) - traditionCents(b.T, deg - 1);
    return rng.chance(0.5) ? up * 0.5 : -dn * 0.5;
}
}   // namespace

void buildFuturistic(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng;
    const int n = b.n, sub = b.sub, cyc = b.beats;
    const int voiceV = E.voiceOf(MR_LEAD), glass = E.voiceOf(MR_SECOND), swarmV = E.voiceOf(MR_SWARM), padV = E.voiceOf(MR_CHORDS), sub0 = E.voiceOf(MR_BASS), alt = E.voiceOf(MR_ALT);
    const int tick = E.kitOf(KR_TICK), tick2 = E.kitOf(KR_HIGH), pulse = E.kitOf(KR_SUB);
    const bool sungSwarm = swarmV >= 0 && E.voices[swarmV].family == TF_SYNVOICE;   // a choir of synthetic voices in the swarm's place
    double pos = 0;
    if (form == PF_PROCESS || form == PF_SIGNAL || form == PF_WORK || form == PF_DANCE || form == PF_FESTIVAL || form == PF_HARVEST || form == PF_MARCH) {
        // the cell: m notes over a cycle's subdivisions; the first voice loops it, the second loops it a subdivision longer (they drift apart and
        // meet again), the voice enters halfway with the cell's notes held long; or the cell is built note by note and taken apart again
        const bool signal = form == PF_SIGNAL || form == PF_WARNING || form == PF_MARCH;
        const int units = b.cu, m = signal ? 2 : 4 + rng.irange(4);
        std::vector<int> at, deg;
        if (signal) { at = {0, units / 2}; const int iv = 1 + rng.irange(2); deg = {n + iv, n};
        } else {
            std::vector<int> slots(units);
            for (int i = 0; i < units; i++) slots[i] = i;
            for (int i = units - 1; i > 0; i--) std::swap(slots[i], slots[rng.irange(i + 1)]);
            at.assign(slots.begin(), slots.begin() + std::min(m, units));
            std::sort(at.begin(), at.end());
            for (size_t i = 0; i < at.size(); i++) deg.push_back(rng.irange(n + 2) + (i == 0 ? 0 : 0));
            deg[0] = 0;
        }
        const bool additive = !signal && rng.chance(0.4);
        double posB = 0; const double cellA = (double)units / sub, cellB = (double)(units + 1) / sub;
        const int loops = std::max(8, (int)std::lround(rng.range(signal ? 45 : 55, signal ? 65 : 85) * b.P.bpm / 60.0 / cellA));
        for (int k = 0; k < loops; k++) {
            const int shown = additive ? std::min((int)at.size(), k < loops / 2 ? k + 1 : loops - k) : (int)at.size();
            for (int i = 0; i < shown; i++) b.add(pos + (double)at[i] / sub, signal ? (double)units / 2 / sub * 0.8 : 0.4, glass, deg[i] + n / 2, rng.range(0.55, 0.8));
            if (!signal && k >= 2 && alt >= 0) { for (int i = 0; i < (int)at.size(); i++) b.add(posB + (double)at[i] / sub, 0.4, alt, deg[i] + n / 2 + (k % 4 == 3 ? n : 0), 0.5); posB += cellB; }
            else posB = pos + cellA;
            if (k >= loops / 2 && k % 2 == 0 && voiceV >= 0) b.add(pos, cellA * 2, voiceV, deg[(k / 2) % deg.size()] + (signal ? 0 : -n / 2 + n / 2), 0.55, -1000, b.vowel());
            for (int u = 0; u < units; u++) {
                if (u % sub == 0 && tick >= 0 && rng.chance(signal ? 1.0 : 0.5)) b.hit(pos + (double)u / sub, tick, 0.5);
                else if (signal && tick2 >= 0) b.hit(pos + (double)u / sub, tick2, 0.35);
            }
            if (k % 2 == 0) b.hit(pos, pulse, 0.8);
            if (k % 4 == 0 && swarmV >= 0 && !signal) { int c3[3]; b.chord(0, c3); for (int q = 0; q < 2; q++) b.add(pos, cellA * 4, swarmV, c3[q] - n / 2, 0.35, -1000, sungSwarm ? b.vowel() : -1); }
            pos += cellA;
        }
        pos = std::max(pos, posB);
        b.add(pos, 2.0 * cyc / 2, glass, 0, 0.6); b.hit(pos, pulse, 0.9);
        pos += cyc;
    } else if (form == PF_SYNSONG || form == PF_LOVE || form == PF_HYMN || form == PF_BALLAD) {
        // the voice's phrases over an arpeggiator (the chord's notes up and down on every subdivision), the sub tone on the cycle, clicks on the beats
        Rng fav(mix64(T.seed ^ 0x1D05C2ULL));
        std::vector<int> prog = rng.chance(0.6) ? b.progression(4, true, fav) : b.progression(4, true, rng);
        const int phrases = b.bySeconds(rng.range(45, 70), (double)b.cpp * cyc, 4, 14);
        std::vector<Ev> A, A2;
        b.phrase(0, T.rest, n / 2, 0.85, 0.08, b.cpp, A);
        for (int p = 0; p < phrases; p++) {
            if (p % 2 == 1) b.vary(A, p + 1 == phrases ? 0 : T.rest, A2); else if (p > 0 && rng.chance(0.4)) b.phrase(n / 2, T.rest, n / 2, 0.85, 0.08, b.cpp, A);
            b.place(p % 2 ? A2 : A, pos, voiceV, 0.6, 0.85, 0, true);
            for (int c = 0; c < b.cpp; c++) {
                const int root = prog[(p * b.cpp + c) % prog.size()]; int c3[3]; b.chord(root, c3);
                const double t0 = pos + c * cyc;
                const int arp[6] = {0, 1, 2, 1, 2, 0};
                for (int u = 0; u < b.cu; u++) b.add(t0 + (double)u / sub, 1.0 / sub * 0.9, glass, c3[arp[u % 6] % 3] + n / 2 + (u % 6 >= 3 ? n : 0), 0.32 + 0.12 * (u == 0));
                b.add(t0, cyc, sub0, root - n, 0.7);
                for (int u = 0; u < b.cu; u += sub) b.hit(t0 + (double)u / sub, tick, b.isGS(u) ? 0.55 : 0.35);
                if (padV >= 0) for (int k = 0; k < 3; k++) b.add(t0, cyc, padV, c3[k], 0.3);
            }
            pos += b.cpp * cyc;
        }
        b.add(pos, cyc, voiceV, 0, 0.6, -1000, b.vowel()); b.add(pos, cyc, sub0, -n, 0.6);
        pos += cyc;
    } else {
        // a swarm, a drift, an elegy: chords of three to five tones, some between the people's notes, each held a few cycles and gliding into the next
        const bool elegy = form == PF_ELEGY || form == PF_LAMENT || form == PF_LAST, drift = form == PF_DRIFT || form == PF_LULLABY;
        const int chordsN = 6 + rng.irange(3);
        std::vector<int> prev;
        for (int c = 0; c < chordsN; c++) {
            const int root = c == 0 || c == chordsN - 1 ? 0 : b.roots[1 + rng.irange(std::max(1, std::min(3, (int)b.roots.size() - 1)))];
            int c3[3]; b.chord(root, c3);
            std::vector<int> tones = {c3[0] - n, c3[1], c3[2]};
            if (rng.chance(0.6)) tones.push_back(c3[0] + n);
            if (rng.chance(0.4)) tones.push_back(c3[1] + n);
            const double len = rng.range(5.5, 11.0) * b.P.bpm / 60.0;   // five to eleven seconds a chord
            for (size_t k = 0; k < tones.size(); k++) {
                const double bend = !drift && rng.chance(0.35) ? microBend(b, tones[k], rng) : 0;
                const int from = k < prev.size() && c > 0 ? prev[k] : -1000;
                const int v = drift && padV >= 0 ? padV : swarmV;
                b.add(pos, len + cyc * 0.5, v, tones[k], rng.range(0.4, 0.6), from, v == swarmV && sungSwarm ? b.vowel() : -1, bend);
            }
            if (drift && glass >= 0) for (int u = 0; u < (int)(len * sub); u += sub * (1 + rng.irange(3))) b.add(pos + (double)u / sub, 0.5, glass, tones[rng.irange((int)tones.size())] + n, 0.35);
            if (elegy && voiceV >= 0 && c % 2 == 1) b.add(pos + cyc * 0.5, len - cyc * 0.5, voiceV, tones[std::min((size_t)2, tones.size() - 1)], 0.5, -1000, b.vowel());
            if (rng.chance(0.5)) b.hit(pos, pulse, 0.6);
            prev = tones;
            pos += len;
        }
        pos += cyc;
    }
    b.P.beats = pos;
}

// ---- the far future: tones of the scale across several periods that come and go, a deep tone under them, a rare glint ----

void buildFarFuture(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n;
    P.bpm = 60;   // a beat a second: the field's times are seconds
    const int a = E.voiceOf(MR_LEAD), a2 = E.voiceOf(MR_SECOND), deep = E.voiceOf(MR_BASS), glint = E.voiceOf(MR_BELL);
    const double total = rng.range(70, 115);
    std::vector<int> tones;   // the field's degrees: across three periods, the tonic and the resting degree first
    tones.push_back(0); tones.push_back(T.rest);
    for (int k = 0; k < 4 + rng.irange(4); k++) tones.push_back(rng.irange(n) + (rng.irange(3) - 1) * n);
    std::sort(tones.begin(), tones.end()); tones.erase(std::unique(tones.begin(), tones.end()), tones.end());
    // the deep tone under all of it, swelling twice or three times
    for (double t = 0; t < total - 5;) { const double len = std::min(total - t, rng.range(25, 45)); b.add(t, len, deep, -2 * n, rng.range(0.55, 0.75)); t += len - rng.range(2, 6); }
    if (form == PF_TURNFIELD) {   // the tones enter in turn, low to high, at the cycle's groups stretched to seconds: a slow wheel, twice or three times round
        const double turn = rng.range(2.5, 4.5);
        int k = 0;
        for (double t = 2; t < total - 10; t += turn) {
            const int deg = tones[k % tones.size()]; k++;
            b.add(t, turn * rng.range(2.5, 4.0), k % 2 ? a : a2, deg, rng.range(0.45, 0.7));
            if (k % (int)tones.size() == 0 && rng.chance(0.5)) std::reverse(tones.begin(), tones.end());
        }
    } else if (form == PF_STANDING || form == PF_ELEGY || form == PF_LAMENT || form == PF_LAST) {   // one chord held, a tone of it moved every twenty seconds or so
        std::vector<int> held(tones.begin(), tones.begin() + std::min((size_t)4, tones.size()));
        std::vector<double> since(held.size(), 1.0);
        for (size_t k = 0; k < held.size(); k++) since[k] = 1 + k * 1.5;
        double t = 18;
        while (t < total - 8) {
            const size_t k = rng.irange((int)held.size());
            b.add(since[k], t - since[k] + 3, k % 2 ? a : a2, held[k], 0.6);
            held[k] = tones[rng.irange((int)tones.size())] + (rng.chance(0.3) ? n : 0); since[k] = t;
            t += rng.range(14, 24);
        }
        for (size_t k = 0; k < held.size(); k++) b.add(since[k], total - since[k], k % 2 ? a : a2, held[k], 0.6);
    } else {   // a field: each tone comes and goes on its own, overlapping; a field of bells holds fewer tones and many glints
        const bool bells = form == PF_BELLFIELD;
        for (size_t k = 0; k < tones.size(); k++) {
            if (bells && k % 2) continue;
            for (double t = rng.range(0, 12); t < total - 6;) {
                const double len = std::min(total - t, rng.range(8, 20));
                b.add(t, len, k % 2 ? a : a2, tones[k], rng.range(0.4, 0.65));
                t += len + rng.range(2, 9);
            }
        }
    }
    // the glints: a high bell now and then (often in a field of bells); a far choir's long swells instead where the people heard one
    const bool choir = glint >= 0 && E.voices[glint].family == TF_SWARM;
    for (double t = rng.range(3, 8); t < total - 4; t += choir ? rng.range(10, 22) : (form == PF_BELLFIELD ? rng.range(1.5, 4.5) : rng.range(6, 15))) {
        const int d = tones[rng.irange((int)tones.size())];
        if (choir) b.add(t, std::min(total - 1 - t, rng.range(6, 12)), glint, ((d % n) + n) % n + n, rng.range(0.45, 0.65));
        else b.add(t, 0.5, glint, ((d % n) + n) % n + 2 * n - (rng.chance(0.4) ? n : 0), rng.range(0.3, 0.6));
    }
    b.P.beats = total + 6;   // the last tones' long fade inside the piece
}
