#pragma once
// E-10: music.cpp's piece builder, for its parts alone (music, music_ages): C-04's forms' parameters, a cell's note, and `Bld`,
// the builder every age's texture writes through (in the class, so inline); the ages' builders declared at the end
#include "music.h"
#include "instruments.h"
#include "voice.h"
#include "tongue.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cctype>

struct FormP { double tempo, density, vel0, vel1, restP; int reg, drum; bool repeat; };   // drum: 0 never, 1 the tradition's, 2 always; repeat: the answer and the return once more
const FormP C04_FP[11] = {
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

struct Ev { int unit, len, deg; bool rest; };

// the builder every texture writes through: the tradition's cycle in subdivision units, C-04's walk and phrases, chords stacked from the
// people's own scale, notes added in beats
struct Bld {
    const Tradition& T; const Ensemble& E; Piece& P; Rng& rng;
    int n, sub, beats, cu, cpp;
    std::vector<int> gs;
    int third[16], fifth[16];
    std::vector<int> roots;   // the scale's degrees by how well a chord stands on them (the tonic first)
    Bld(const Tradition& t, const Ensemble& e, Piece& p, Rng& r) : T(t), E(e), P(p), rng(r) {
        n = std::max(1, (int)T.scale.size()); sub = T.sub; beats = T.beats; cu = beats * sub; cpp = beats <= 3 ? 4 : (beats <= 6 ? 2 : 1);
        int u = 0; for (int g : T.groups) { gs.push_back(u); u += g * sub; }
        // the chord on each degree: the degree nearest a third above it (230-470 cents) and the one nearest a fifth, in the people's tuning
        std::vector<std::pair<double, int>> q;
        for (int r0 = 0; r0 < n && r0 < 16; r0++) {
            double c0 = traditionCents(T, r0), best3 = 1e9, best5 = 1e9; int d3 = r0 + std::min(2, n - 1), d5 = r0 + std::min(4, n - 1);
            for (int d = r0 + 1; d < r0 + n; d++) {
                const double c = traditionCents(T, d) - c0;
                if (c >= 230 && c <= 470 && std::fabs(c - 350) < best3) { best3 = std::fabs(c - 350); d3 = d; }
                if (std::fabs(c - 702) < best5) { best5 = std::fabs(c - 702); d5 = d; }
            }
            third[r0] = d3; fifth[r0] = d5;
            q.push_back({best5 + 0.5 * std::min(best3, 200.0) + (r0 == 0 ? -1000 : 0), r0});
        }
        std::sort(q.begin(), q.end());
        for (auto& x : q) roots.push_back(x.second);
    }
    bool isGS(int u) const { u = ((u % cu) + cu) % cu; for (int g : gs) if (g == u) return true; return false; }
    // how many units of `beatsEach` beats make about `secs` seconds at the piece's tempo (a long cycle or a slow tempo takes fewer)
    int bySeconds(double secs, double beatsEach, int lo, int hi) const { return std::max(lo, std::min(hi, (int)std::lround(secs * P.bpm / 60.0 / std::max(1.0, beatsEach)))); }
    int midGroup() const { int best = gs.size() > 1 ? gs[1] : cu / 2; for (int g : gs) if (g > 0 && std::abs(g - cu / 2) < std::abs(best - cu / 2)) best = g; return best; }
    void add(double t, double dur, int voice, int deg, double vel, int slide = -1000, int vowel = -1, double bend = 0) {
        if (voice < 0 || voice >= (int)E.voices.size() || dur <= 1e-6 || t < 0) return;
        Note nt; nt.t = t; nt.dur = dur; nt.voice = voice; nt.degree = clampi(deg, -3 * n, 3 * n); nt.vel = clampd(vel, 0.05, 1.0); nt.slideFrom = slide; nt.vowel = vowel; nt.bend = bend;
        P.notes.push_back(nt);
    }
    void hit(double t, int piece, double vel) {
        if (piece < 0 || piece >= (int)E.kit.size() || t < 0) return;
        Note nt; nt.t = t; nt.dur = 0.25; nt.voice = -1; nt.degree = piece; nt.vel = clampd(vel, 0.05, 1.0); P.notes.push_back(nt);
    }
    int vowel() { return T.vowels.empty() ? 0 : T.vowels[rng.irange((int)T.vowels.size())]; }
    double accent(int unit) const { const int uc = ((unit % cu) + cu) % cu; return uc == 0 ? 1.0 : (isGS(uc) ? 0.92 : 0.82); }
    // C-04's: a group's rhythm, the walk, a phrase and a variation
    void groupRhythm(int gb, double density, bool closing, std::vector<int>& out) {
        out.clear(); int units = gb * sub;
        if (closing && gb <= 2) { if (rng.chance(0.6)) out.push_back(units); else { out.push_back(sub); out.push_back(units - sub); } return; }
        if (closing) units -= 2 * sub;   // a long group closes on its last two beats held, the beats before it moving (a cycle of one group still sings)
        for (int rem = units; rem > 0;) {
            int lens[4] = {1, sub, 2 * sub, 3 * sub};
            double wl[4] = {density * 0.9, 1.2, 0.7 / density, 0.3 / density};
            for (int k = 0; k < 4; k++) if (lens[k] > rem) wl[k] = 0;
            int len = lens[rng.pick(wl, 4)];
            if (len == 1) { int k = std::min(rem, sub); for (int i = 0; i < k; i++) out.push_back(1); rem -= k; }
            else { out.push_back(len); rem -= len; }
        }
        if (closing) out.push_back(2 * sub);
    }
    int nextDegree(int cur, int lastStep, int centre) {
        double wd[3] = {0.5, 0.15, 0.35};
        if (std::abs(lastStep) >= 2) { wd[0] = 0.8; wd[1] = 0.05; wd[2] = 0.15; }
        int c = rng.pick(wd, 3), d = 0;
        if (c == 0) d = std::abs(lastStep) >= 2 ? (lastStep > 0 ? -1 : 1) : (rng.chance(0.5) ? 1 : -1);
        else if (c == 2) { d = 2 + rng.irange(3); if (rng.chance(cur > centre ? 0.7 : 0.3)) d = -d; }
        int nd = cur + d;
        if (nd < centre - n / 2 - 1) nd = cur + std::abs(d);
        if (nd > centre + n + 1) nd = cur - std::abs(d);
        return nd;
    }
    void phrase(int startDeg, int endDeg, int centre, double density, double restP, int cycles, std::vector<Ev>& ev) {
        ev.clear(); int unit = 0, cur = startDeg, lastStep = 0; bool first = true;
        for (int c = 0; c < cycles; c++)
            for (size_t gi = 0; gi < T.groups.size(); gi++) {
                bool closing = c == cycles - 1 && gi + 1 == T.groups.size();
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
    }
    void vary(const std::vector<Ev>& a, int endDeg, std::vector<Ev>& out) {
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
    }
    // a phrase's events as notes of a voice from `pos` (beats); `sung` a vowel to each note (a new one now and then)
    void place(const std::vector<Ev>& ev, double pos, int voice, double vel0, double vel1, int reg, bool sung, double legato = 1.0) {
        int vw = sung ? vowel() : -1;
        for (const Ev& e : ev) {
            if (e.rest) continue;
            if (sung && rng.chance(0.45)) vw = vowel();
            add(pos + (double)e.unit / sub, (double)e.len / sub * legato, voice, e.deg + reg, rng.range(vel0, vel1) * accent(e.unit), -1000, vw);
        }
    }
    // the chord on a root: the root, its third, its fifth (degrees, the root's period)
    void chord(int root, int out[3]) const { const int r = ((root % n) + n) % n, oct = root - r; out[0] = root; out[1] = third[r] + oct; out[2] = fifth[r] + oct; }
    bool inChord(int deg, int root) const { int c[3]; chord(root, c); for (int k = 0; k < 3; k++) if ((((deg - c[k]) % n) + n) % n == 0) return true; return false; }
    int nearestChordTone(int deg, int root) const {
        for (int d = 0; d <= n; d++) { if (inChord(deg - d, root)) return deg - d; if (inChord(deg + d, root)) return deg + d; }
        return deg;
    }
    // a progression of `len` roots: the tonic first; open on the resting degree's chord, closed on the tonic; between them the scale's best
    // roots (the people's own turns: `fav` the progression its songs come back to, drawn once from the tradition)
    std::vector<int> progression(int len, bool closed, Rng& r) const {
        std::vector<int> p(std::max(1, len), 0);
        std::vector<int> mid; for (int x : roots) if (x != 0 && x != T.rest && (int)mid.size() < 3) mid.push_back(x);
        if (mid.empty()) mid.push_back(T.rest);
        for (int i = 1; i < len; i++) {
            if (i == len - 1) { p[i] = closed ? 0 : T.rest; break; }
            int x = mid[r.irange((int)mid.size())];
            if (x == p[i - 1] && mid.size() > 1) x = mid[(r.irange((int)mid.size() - 1) + 1) % mid.size()];
            p[i] = x;
        }
        if (len >= 2 && closed && p[len - 2] == 0) p[len - 2] = T.rest;
        return p;
    }
    // the people's ornament rule on a voice's notes (C-04's), at a share of its chance
    void ornament(int voice, double share) {
        if (T.ornament == 0 || share <= 0) return;
        std::vector<Note> all; all.swap(P.notes);
        int prevDeg = -1000; double prevEnd = -1;
        for (size_t i = 0; i < all.size(); i++) {
            Note nt = all[i];
            if (nt.voice != voice) { P.notes.push_back(nt); continue; }
            bool adjacent = prevDeg != -1000 && std::fabs(nt.t - prevEnd) < 1e-6;
            int step = prevDeg == -1000 ? 0 : nt.degree - prevDeg;
            bool orn = i + 1 < all.size() && rng.chance(T.ornamentChance * share);
            if (orn && T.ornament == 1 && std::abs(step) >= 2 && nt.dur >= 0.5) {
                Note gr = nt; gr.dur = 0.12; gr.degree = nt.degree + (step > 0 ? -1 : 1); gr.vel *= 0.7; P.notes.push_back(gr);
                nt.t += 0.12; nt.dur -= 0.12; P.notes.push_back(nt);
            } else if (orn && T.ornament == 2 && nt.dur >= 1.5) {
                double tt = nt.t, end = nt.t + nt.dur; int k = 0;
                while (end - tt > 1e-6) { Note tr = nt; tr.t = tt; tr.dur = end - tt < 0.5 ? end - tt : 0.25; tr.degree = nt.degree + (k % 2); tr.vel *= k == 0 ? 1.0 : 0.8; P.notes.push_back(tr); tt += tr.dur; k++; }
            } else if (orn && T.ornament == 3 && std::abs(step) == 1 && adjacent) {
                nt.slideFrom = prevDeg; P.notes.push_back(nt);
            } else if (orn && T.ornament == 4 && nt.dur >= 1.0) {
                Note m1 = nt; m1.dur = 0.15; P.notes.push_back(m1);
                Note m2 = nt; m2.t += 0.15; m2.dur = 0.15; m2.degree = nt.degree - 1; m2.vel *= 0.8; P.notes.push_back(m2);
                nt.t += 0.3; nt.dur -= 0.3; P.notes.push_back(nt);
            } else P.notes.push_back(nt);
            prevDeg = all[i].degree; prevEnd = all[i].t + all[i].dur;
        }
    }
};

// ---- each age's texture (music_ages.cpp), that `pieceOf` (music.cpp) builds a piece with
void buildPrehistoric(Bld& b, int form);
void buildMedieval(Bld& b, int form);
void buildIndustrial(Bld& b, int form);
void buildModern(Bld& b, int form);
void buildFuturistic(Bld& b, int form);
void buildFarFuture(Bld& b, int form);
