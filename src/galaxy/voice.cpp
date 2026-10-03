// C-05: a people's voice (voice.h). The phone table, a world's voice from its seed, the speech of a decoded shard.
#include "voice.h"
#include "core/rng.h"
#include <cmath>
#include <cstring>
#include <cctype>
#include <algorithm>

namespace {

// the table's rows: a vowel (its formants, Hz, a man's; the voice's tract scales them), a diphthong (from one set to another), a
// stop (the locus its transitions head for, the burst's band and level), a fricative (the band and level of its noise; a centre of
// 0 is breath through the formants), a sonorant (a nasal, a liquid, a glide: formants of its own)
#define VOW(sym, a1, a2, a3, dur) {sym, PK_VOWEL, true, a1, a2, a3, a1, a2, a3, 0, 0, 0, dur}
#define DIP(sym, a1, a2, a3, b1, b2, b3) {sym, PK_VOWEL, true, a1, a2, a3, b1, b2, b3, 0, 0, 0, 0.15}
#define STOP(sym, voiced, locus, nf, nbw, namp) {sym, PK_STOP, voiced, 200, locus, 2400, 200, locus, 2400, nf, nbw, namp, voiced ? 0.045 : 0.055}
#define FRIC(sym, voiced, locus, nf, nbw, namp, dur) {sym, PK_FRIC, voiced, 250, locus, 2500, 250, locus, 2500, nf, nbw, namp, dur}
#define SON(sym, kind, a1, a2, a3, dur) {sym, kind, true, a1, a2, a3, a1, a2, a3, 0, 0, 0, dur}
uint64_t strHashV(const std::string& s) { uint64_t h = 1469598103934665603ULL; for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; } return h; }
}

// longest spellings first: the parse takes the first row that matches
const Phone PHONES[] = {
    DIP("ai", 730, 1090, 2440, 270, 2290, 3010), DIP("ei", 530, 1840, 2480, 270, 2290, 3010), DIP("ey", 530, 1840, 2480, 270, 2290, 3010),
    DIP("ou", 570, 840, 2410, 300, 870, 2240), DIP("au", 730, 1090, 2440, 300, 870, 2240), DIP("ao", 730, 1090, 2440, 570, 840, 2410),
    DIP("ia", 270, 2290, 3010, 730, 1090, 2440), DIP("ie", 270, 2290, 3010, 530, 1840, 2480), DIP("io", 270, 2290, 3010, 570, 840, 2410),
    DIP("ea", 530, 1840, 2480, 730, 1090, 2440), DIP("eo", 530, 1840, 2480, 570, 840, 2410), DIP("ae", 730, 1090, 2440, 530, 1840, 2480),
    DIP("oa", 570, 840, 2410, 730, 1090, 2440), DIP("ue", 300, 870, 2240, 530, 1840, 2480), DIP("ui", 300, 870, 2240, 270, 2290, 3010),
    VOW("aa", 730, 1090, 2440, 0.16), VOW("ee", 270, 2290, 3010, 0.15), VOW("ii", 270, 2290, 3010, 0.15), VOW("oo", 300, 870, 2240, 0.15), VOW("uu", 300, 870, 2240, 0.15),
    FRIC("sh", false, 1800, 3000, 1400, 0.6, 0.10), FRIC("zh", true, 1800, 3000, 1400, 0.4, 0.08), FRIC("th", false, 1600, 5200, 4500, 0.3, 0.09),
    FRIC("kh", false, 1900, 1500, 900, 0.5, 0.10), FRIC("gh", true, 1900, 1500, 900, 0.3, 0.08), FRIC("ph", false, 1100, 4000, 5000, 0.25, 0.09), FRIC("ss", false, 1700, 6500, 2500, 0.55, 0.14),
    STOP("ck", false, 2000, 2200, 1200, 0.6),
    SON("ng", PK_NASAL, 250, 2000, 2600, 0.08), SON("nn", PK_NASAL, 250, 1500, 2500, 0.10), SON("ll", PK_LIQUID, 360, 1100, 2600, 0.09), SON("rh", PK_LIQUID, 400, 1250, 1600, 0.07), SON("lh", PK_LIQUID, 360, 1100, 2600, 0.07),
    VOW("a", 730, 1090, 2440, 0.10), VOW("e", 530, 1840, 2480, 0.10), VOW("i", 270, 2290, 3010, 0.09), VOW("o", 570, 840, 2410, 0.10), VOW("u", 300, 870, 2240, 0.09),
    STOP("p", false, 1100, 1200, 2000, 0.5), STOP("b", true, 1100, 1200, 2000, 0.4), STOP("t", false, 1800, 4500, 3000, 0.6), STOP("d", true, 1800, 4500, 3000, 0.45),
    STOP("k", false, 2000, 2200, 1200, 0.6), STOP("g", true, 2000, 2200, 1200, 0.45), STOP("c", false, 2000, 2200, 1200, 0.6), STOP("q", false, 1200, 1300, 800, 0.6),
    FRIC("s", false, 1700, 6500, 2500, 0.55, 0.095), FRIC("z", true, 1700, 6500, 2500, 0.35, 0.075), FRIC("f", false, 1100, 4000, 5000, 0.25, 0.09), FRIC("v", true, 1100, 4000, 5000, 0.2, 0.07),
    FRIC("x", false, 1900, 1500, 900, 0.5, 0.10), FRIC("h", false, 1500, 0, 0, 0.45, 0.07), FRIC("j", true, 1800, 3000, 1400, 0.4, 0.08),
    SON("m", PK_NASAL, 250, 1000, 2200, 0.07), SON("n", PK_NASAL, 250, 1500, 2500, 0.07), SON("l", PK_LIQUID, 360, 1100, 2600, 0.06), SON("r", PK_LIQUID, 400, 1250, 1600, 0.06),
    SON("w", PK_GLIDE, 300, 650, 2200, 0.055), SON("y", PK_GLIDE, 280, 2200, 3000, 0.05),
};
int phoneCount() { return (int)(sizeof(PHONES) / sizeof(PHONES[0])); }
int phoneOf(const char* sym) { for (int i = 0; i < phoneCount(); i++) if (strcmp(PHONES[i].sym, sym) == 0) return i; return -1; }

void parsePhones(const std::string& word, std::vector<int>& out) {
    out.clear();
    size_t i = 0, n = word.size();
    while (i < n) {
        int hit = -1;
        for (int p = 0; p < phoneCount() && hit < 0; p++) { size_t l = strlen(PHONES[p].sym); if (i + l <= n && word.compare(i, l, PHONES[p].sym) == 0) hit = p; }
        if (hit < 0) { i++; continue; }   // a letter the tongue has no sound for (a digit, an apostrophe, a hyphen)
        out.push_back(hit); i += strlen(PHONES[hit].sym);
    }
}

// the voice from the body's seed, leaning with the names' style: hard names (1) sit lower and rougher and click and roll more,
// flowing names (2) run quicker and breathier and tremble more; the inventory is the sounds the tongue's pools spell
Voice voiceOf(const BodyGen& g, const Lore& L, const Tongue& T) {
    Voice V; V.seed = mix64(loreSeed(g, L) ^ 0x3C05A1ULL);   // C-13: the people's
    Rng rng(V.seed);
    int st = L.style < 0 || L.style > 2 ? 0 : L.style;
    double oct = rng.uni() * 1.75;                                   // 85 to 286 Hz
    if (st == 1) oct = std::max(0.0, oct - 0.25);
    V.pitch = 85 * std::pow(2.0, oct);
    V.tract = 1.22 - 0.38 * (oct / 1.75) + rng.sym(0.08);
    V.range = 0.5 + 1.1 * rng.uni();
    V.rate = 3.0 + 2.6 * rng.uni() + (st == 2 ? 0.3 : 0);
    V.breath = rng.chance(0.07) ? 0.95 : 0.05 + 0.5 * std::pow(rng.uni(), 1.5) + (st == 2 ? 0.08 : 0);
    V.rough = rng.chance(st == 1 ? 0.5 : 0.3) ? 0.2 + 0.6 * rng.uni() : 0.03 * rng.uni();
    V.nasal = rng.chance(0.3) ? 0.3 + 0.5 * rng.uni() : 0;
    V.tremor = rng.chance(st == 2 ? 0.4 : 0.22) ? 0.3 + 0.6 * rng.uni() : 0;
    if (rng.chance(0.12)) { V.sub = 0.3 + 0.4 * rng.uni(); V.subRatio = rng.chance(0.5) ? 0.5 : 2.0 / 3.0; }
    V.stress = rng.irange(4);
    V.contour = rng.irange(3);
    V.tonal = rng.chance(0.25);
    V.clicky = rng.chance(st == 1 ? 0.35 : 0.15);
    V.trill = rng.chance(st == 1 ? 0.5 : 0.25);
    std::vector<bool> seen(phoneCount(), false);
    for (const std::vector<std::string>* pool : {&T.onsets, &T.nuclei, &T.codas}) for (const std::string& s : *pool) { std::vector<int> ph; parsePhones(s, ph); for (int p : ph) seen[p] = true; }
    for (int p = 0; p < phoneCount(); p++) if (seen[p]) V.inventory.push_back(p);
    return V;
}

std::string voiceLine(const Voice& V) {
    std::string s = "A ";
    if (V.pitch < 110) s += "DEEP "; else if (V.pitch < 150) s += "LOW "; else if (V.pitch >= 210) s += "HIGH ";
    if (V.breath >= 0.9) s += "WHISPERING"; else if (V.sub > 0) s += "TWO-TONED"; else if (V.rough > 0.3) s += "ROUGH"; else if (V.breath > 0.35) s += "BREATHY";
    else if (V.nasal > 0.3) s += "NASAL"; else if (V.tremor > 0.4) s += "TREMBLING"; else s += "CLEAR";
    s += " VOICE";
    if (V.rate < 3.2) s += ", SLOW"; else if (V.rate > 4.6) s += ", QUICK";
    if (V.clicky) s += ", WITH CLICKS"; else if (V.tonal) s += ", WITH TONES"; else if (V.trill) s += ", ROLLING ITS R'S";
    return s;
}

namespace {
// the sentence's melody at u of its syllables (0 the first, 1 past the last), as a ratio to the voice's base
double contourAt(const Voice& V, double u) {
    switch (V.contour) {
        case 1: { double hump = u < 0.3 ? u / 0.3 : 1 - (u - 0.3) / 0.7; return 1 + V.range * (0.18 * hump - 0.06 - 0.08 * u); }
        case 2: return 1 + V.range * (0.04 - (u > 0.85 ? 0.16 * (u - 0.85) / 0.15 : 0));
        default: return 1 + V.range * (0.10 - 0.22 * u);
    }
}
}

// the words spoken in order: each the people's word (a name as it is), its sounds timed by the voice's rate, the stressed
// syllable longer and louder, the last before a pause longer still, the pitch by the sentence's melody and the syllable's
// tone, a pause by the punctuation; a stop is a closure and a burst (a click in a clicking voice)
void speechOf(const Voice& V, const std::vector<DecodedWord>& words, uint64_t seed, Speech& out) {
    out = Speech(); out.seed = mix64(seed ^ 0x3C05ULL);
    Rng rng(out.seed);
    int nw = (int)words.size();
    out.wordStart.assign(nw, 0.0); out.wordEnd.assign(nw, 0.0); out.phones.assign(nw, std::string());
    double k = (1.0 / V.rate) / 0.28;   // a syllable's sounds (stress and the ends of phrases included) sum to about 0.28 s at the base rate
    struct W { std::vector<int> ph; int syl = 0, brk = 0; std::string spoken; };
    std::vector<W> ws(nw);
    for (int i = 0; i < nw; i++) {
        const DecodedWord& d = words[i];
        W& x = ws[i];
        x.spoken = d.name ? d.ours : d.theirs;
        for (char& c : x.spoken) c = (char)std::tolower((unsigned char)c);
        parsePhones(x.spoken, x.ph);
        for (int p : x.ph) if (PHONES[p].kind == PK_VOWEL) x.syl++;
        if (!x.ph.empty() && x.syl == 0) x.syl = 1;
        for (int p : x.ph) { if (!out.phones[i].empty()) out.phones[i] += '-'; out.phones[i] += PHONES[p].sym; }
        const std::string& post = d.post;
        if (post.find('?') != std::string::npos) x.brk = 4; else if (post.find('!') != std::string::npos) x.brk = 5; else if (post.find('.') != std::string::npos) x.brk = 3;
        else if (post.find(';') != std::string::npos || post.find(':') != std::string::npos) x.brk = 2; else if (post.find(',') != std::string::npos) x.brk = 1;
        if (i == nw - 1 && x.brk < 2) x.brk = 3;
    }
    double t = 0;
    int i = 0;
    while (i < nw) {
        int j = i, N = 0;   // the sentence: words i..j-1, N syllables
        while (j < nw) { N += ws[j].syl; j++; if (ws[j - 1].brk >= 2) break; }
        int endBrk = ws[j - 1].brk, sylIdx = 0;
        for (int w = i; w < j; w++) {
            W& x = ws[w];
            out.wordStart[w] = t;
            if (x.ph.empty()) { out.wordEnd[w] = t; }
            else {
                int n = x.syl, np = (int)x.ph.size();
                int stressAt = V.stress == 0 ? 0 : (V.stress == 1 ? n - 1 : (V.stress == 2 ? std::max(0, n - 2) : -1));
                bool lastWord = w == j - 1;
                std::vector<int> sylOf(np); { int seen = 0; for (int p = 0; p < np; p++) { if (PHONES[x.ph[p]].kind == PK_VOWEL) { sylOf[p] = seen; seen++; } else sylOf[p] = std::min(seen, n - 1); } }
                // each sound's length and loudness, then the syllables' spans
                std::vector<double> durs(np), amps(np), closure(np, 0.0);
                std::vector<double> sylDur(n, 0.0);
                for (int p = 0; p < np; p++) {
                    const Phone& ph = PHONES[x.ph[p]];
                    int s = sylOf[p]; bool stressed = s == stressAt, lastSyl = s == n - 1 && (x.brk > 0 || lastWord);
                    double dur = ph.dur * k;
                    if (ph.kind == PK_VOWEL) dur *= stressed ? 1.3 : (V.stress == 3 ? 1.0 : 0.85);
                    if (lastSyl) dur *= ph.kind == PK_VOWEL ? 1.35 : 1.15;
                    if (V.trill && ph.kind == PK_LIQUID && ph.sym[0] == 'r') dur *= 1.5;
                    dur *= 1 + rng.sym(0.06);
                    if (ph.kind == PK_STOP) { bool click = V.clicky && !ph.voiced; closure[p] = click ? 0.03 * k : dur; dur = closure[p] + (click ? 0.012 : (ph.voiced ? 0.018 : 0.045) * std::sqrt(k)); }
                    durs[p] = dur; amps[p] = (stressed ? 1.2 : 1.0) * (lastSyl && endBrk == 3 ? 0.85 : 1.0);
                    sylDur[s] += dur;
                }
                // the syllables' pitch, start to end
                std::vector<double> p0(n), p1(n);
                for (int s = 0; s < n; s++) {
                    int g = sylIdx + s;
                    double u0 = (double)g / std::max(1, N), u1 = (double)(g + 1) / std::max(1, N);
                    double a = contourAt(V, u0), b = contourAt(V, u1);
                    if (V.tonal) {
                        int tone = (int)(mix64(strHashV(x.spoken) ^ ((uint64_t)s * 0x9E3779B97F4A7C15ULL)) & 3);
                        double r = V.range;
                        if (tone == 0) { a *= 1 + 0.10 * r; b *= 1 + 0.10 * r; } else if (tone == 1) { a *= 1 - 0.08 * r; b *= 1 - 0.08 * r; }
                        else if (tone == 2) { a *= 1 - 0.06 * r; b *= 1 + 0.14 * r; } else { a *= 1 + 0.14 * r; b *= 1 - 0.10 * r; }
                    } else if (s == stressAt) { a += 0.12 * V.range; b += 0.06 * V.range; }
                    if (s == n - 1) {
                        if (x.brk == 1) b *= 1 + 0.08 * V.range; else if (x.brk == 2) b *= 1 - 0.08 * V.range; else if (x.brk == 3) b *= 1 - 0.16 * V.range;
                        else if (x.brk == 4) { a *= 1 + 0.1 * V.range; b *= 1 + 0.3 * V.range; } else if (x.brk == 5) a *= 1 + 0.15 * V.range;
                    }
                    p0[s] = a; p1[s] = b;
                }
                std::vector<double> sylAt(n, 0.0);   // the time into each syllable so far
                for (int p = 0; p < np; p++) {
                    const Phone& ph = PHONES[x.ph[p]];
                    int s = sylOf[p];
                    double span = std::max(sylDur[s], 1e-6), ua = sylAt[s] / span, ub = (sylAt[s] + durs[p]) / span;
                    double pa = p0[s] + (p1[s] - p0[s]) * ua, pb = p0[s] + (p1[s] - p0[s]) * ub;
                    Segment sg; sg.phone = x.ph[p]; sg.t = t; sg.dur = durs[p]; sg.pitch = pa; sg.pitchEnd = pb; sg.amp = amps[p]; sg.word = w; sg.syllable = s; sg.stressed = s == stressAt;
                    sg.trill = V.trill && ph.kind == PK_LIQUID && ph.sym[0] == 'r';
                    if (ph.kind == PK_STOP) {   // the closure, then the burst (or the click)
                        bool click = V.clicky && !ph.voiced;
                        double pm = p0[s] + (p1[s] - p0[s]) * ((sylAt[s] + closure[p]) / span);
                        sg.dur = closure[p]; sg.pitchEnd = pm; out.segs.push_back(sg);
                        sg.t = t + closure[p]; sg.dur = durs[p] - closure[p]; sg.pitch = pm; sg.pitchEnd = pb; sg.part = click ? 2 : 1; out.segs.push_back(sg);
                    } else out.segs.push_back(sg);
                    sylAt[s] += durs[p]; t += durs[p];
                }
                out.wordEnd[w] = t;
                out.syllables += n;
            }
            double gap = x.brk == 0 ? 0.02 * k : (x.brk == 1 ? 0.18 * k : (x.brk == 2 ? 0.28 * k : (w == nw - 1 ? 0.25 * k : 0.45 * k)));
            Segment ps; ps.phone = -1; ps.t = t; ps.dur = gap; ps.amp = 0; ps.word = -1; out.segs.push_back(ps); t += gap;
            sylIdx += x.syl;
        }
        i = j;
    }
    out.seconds = t;
}

int speechWordAt(const Speech& S, double t) {
    int w = -1;
    for (size_t i = 0; i < S.wordStart.size(); i++) { if (S.wordStart[i] > t) break; if (S.wordEnd[i] > S.wordStart[i]) w = (int)i; }
    return w;
}
