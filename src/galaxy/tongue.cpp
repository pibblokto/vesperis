// H-04's second pass (2026-10-10): a people's tongue (tongue.h): its sounds, its syllables, its spelling, its morphemes and its names.
#include "tongue.h"
#include "shards.h"
#include "ruins.h"
#include "../core/rng.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>

namespace {

// ---- the sounds -----------------------------------------------------------------------------------------------------------------
enum SoundKind { SK_STOP = 0, SK_FRIC, SK_NASAL, SK_LIQUID, SK_GLIDE, SK_AFFR };
// a consonant: its code, its kind and its spellings (the first the plainest, drawn most)
struct Cons { const char* code; int kind; bool back; const char* spell[4]; };
const Cons CONS[] = {
    {"p", SK_STOP, false, {"p", nullptr}}, {"b", SK_STOP, false, {"b", nullptr}}, {"t", SK_STOP, false, {"t", nullptr}}, {"d", SK_STOP, false, {"d", nullptr}},
    {"k", SK_STOP, true, {"k", "c", "k", nullptr}}, {"g", SK_STOP, true, {"g", nullptr}}, {"q", SK_STOP, true, {"q", "q", "qh", nullptr}},
    {"?", SK_STOP, true, {"'", "'", nullptr}}, {"kw", SK_STOP, true, {"kw", "qu", nullptr}},
    {"m", SK_NASAL, false, {"m", nullptr}}, {"n", SK_NASAL, false, {"n", nullptr}}, {"ng", SK_NASAL, true, {"ng", nullptr}}, {"ny", SK_NASAL, false, {"ny", "nj", nullptr}},
    {"f", SK_FRIC, false, {"f", "f", "ph", nullptr}}, {"v", SK_FRIC, false, {"v", "v", "w", nullptr}}, {"s", SK_FRIC, false, {"s", nullptr}}, {"z", SK_FRIC, false, {"z", nullptr}},
    {"sh", SK_FRIC, false, {"sh", "sch", "x", "sj"}}, {"zh", SK_FRIC, false, {"zh", "j", "zs", nullptr}}, {"th", SK_FRIC, false, {"th", nullptr}},
    {"dh", SK_FRIC, false, {"dh", "dd", "th", nullptr}}, {"x", SK_FRIC, true, {"kh", "ch", "x", nullptr}}, {"gh", SK_FRIC, true, {"gh", "gh", "g", nullptr}},
    {"h", SK_FRIC, true, {"h", nullptr}}, {"ts", SK_AFFR, false, {"ts", "tz", "z", "c"}}, {"dz", SK_AFFR, false, {"dz", "z", nullptr}},
    {"tsh", SK_AFFR, false, {"ch", "tch", "c", "tx"}}, {"dzh", SK_AFFR, false, {"j", "dj", "g", nullptr}}, {"hl", SK_LIQUID, false, {"hl", "lh", "tl", nullptr}},
    {"l", SK_LIQUID, false, {"l", nullptr}}, {"r", SK_LIQUID, false, {"r", nullptr}}, {"w", SK_GLIDE, false, {"w", nullptr}},
    {"j", SK_GLIDE, false, {"y", "y", "j", nullptr}},
};
constexpr int CONS_N = (int)(sizeof(CONS) / sizeof(CONS[0]));
int consIndex(const char* code) { for (int i = 0; i < CONS_N; i++) if (std::strcmp(CONS[i].code, code) == 0) return i; return -1; }

// the consonant sets a tongue begins from (each like a family of the world's languages, none of them one)
struct ConsSet { const char* name; const char* codes[24]; };
const ConsSet CSETS[] = {
    {"minimal", {"p", "t", "k", "m", "n", "l", "s", nullptr}},
    {"broad", {"p", "t", "k", "b", "d", "g", "m", "n", "l", "r", "s", "sh", "z", "zh", "tsh", nullptr}},
    {"sparse", {"p", "t", "k", "m", "n", "h", "s", nullptr}},
    {"island", {"h", "k", "l", "m", "n", "p", "w", "?", nullptr}},
    {"northern", {"p", "t", "k", "q", "v", "s", "g", "r", "m", "n", "ng", "l", "j", nullptr}},
    {"desert", {"t", "k", "s", "sh", "d", "b", "q", "gh", "x", "m", "n", "l", "r", "w", "j", "h", nullptr}},
    {"desert-lite", {"t", "k", "d", "g", "m", "n", "s", "sh", "r", nullptr}},
    {"plain", {"p", "t", "k", "b", "d", "g", "m", "n", "s", "z", "zh", "sh", "h", "j", "w", nullptr}},
    {"lake", {"p", "t", "k", "kw", "ts", "tsh", "hl", "m", "n", "s", "sh", "l", "w", "j", "?", nullptr}},
    {"mountain", {"p", "b", "t", "d", "k", "g", "q", "ts", "tsh", "dz", "dzh", "s", "z", "sh", "zh", "x", "gh", "h", "m", "n", "l", "r", "v", nullptr}},
    {"even", {"k", "g", "s", "z", "t", "d", "n", "h", "b", "p", "m", "r", "w", "j", nullptr}},
    {"fen", {"p", "t", "k", "d", "g", "h", "j", "l", "m", "n", "r", "s", "v", nullptr}},
    {"steppe", {"p", "b", "t", "d", "k", "g", "m", "n", "l", "r", "s", "z", "sh", "zh", "x", "ts", "tsh", "v", "j", nullptr}},
    {"western", {"b", "d", "g", "k", "m", "n", "l", "r", "s", "sh", "v", "x", "dh", "th", "w", nullptr}},
    {"savanna", {"p", "b", "t", "d", "k", "g", "m", "n", "ny", "ng", "s", "z", "f", "v", "l", "w", "j", nullptr}},
    {"jungle", {"p", "t", "k", "q", "?", "tsh", "ts", "s", "sh", "x", "h", "m", "n", "l", "r", "w", "j", nullptr}},
    {"ocean", {"p", "t", "k", "m", "n", "ng", "f", "v", "h", "r", "w", nullptr}},
    {"harsh", {"q", "?", "tsh", "dzh", "x", "gh", "sh", "b", "d", "p", "t", "k", "v", "m", "n", "l", "r", nullptr}},
    {"soft", {"l", "n", "r", "m", "th", "v", "s", "f", "d", "g", "k", "w", "j", nullptr}},
    {"river", {"t", "d", "k", "g", "n", "m", "r", "l", "s", "z", "dh", "th", "h", "w", nullptr}},
    {"forest", {"p", "b", "t", "d", "k", "g", "m", "n", "s", "x", "l", "r", "w", "j", "ts", nullptr}},
    {"clicking", {"t", "k", "q", "x", "?", "m", "n", "ng", "s", "h", "l", "w", "j", "tsh", nullptr}},
    {"hissing", {"s", "z", "sh", "zh", "ts", "dz", "tsh", "t", "k", "m", "n", "l", "r", "v", nullptr}},
    {"murmuring", {"m", "n", "ng", "ny", "l", "r", "w", "j", "h", "b", "d", "g", "v", "z", nullptr}},
};
constexpr int CSETS_N = (int)(sizeof(CSETS) / sizeof(CSETS[0]));

// a vowel: its code and its spellings
struct Vow { const char* code; const char* spell[3]; };
const Vow VOWS[] = {
    {"a", {"a", nullptr}}, {"e", {"e", nullptr}}, {"i", {"i", "i", "y"}}, {"o", {"o", nullptr}}, {"u", {"u", "u", "ou"}}, {"y", {"y", "ue", "eu"}},
    {"ae", {"ae", "ai", nullptr}}, {"oe", {"oe", "eu", nullptr}},
    {"A", {"aa", "ah", nullptr}}, {"E", {"ee", "eh", nullptr}}, {"I", {"ii", "iy", nullptr}}, {"O", {"oo", "oh", nullptr}}, {"U", {"uu", "uh", nullptr}},
    {"ai", {"ai", "ay", nullptr}}, {"au", {"au", "aw", nullptr}}, {"ei", {"ei", "ey", nullptr}}, {"ou", {"ou", "ow", nullptr}}, {"ia", {"ia", "ya", nullptr}},
    {"ie", {"ie", "ye", nullptr}}, {"oi", {"oi", "oy", nullptr}},
};
constexpr int VOWS_N = (int)(sizeof(VOWS) / sizeof(VOWS[0]));
const char* const VSETS[][9] = {
    {"a", "i", "u", nullptr}, {"a", "e", "i", "o", "u", nullptr}, {"a", "e", "i", "o", "u", "A", "I", "U", nullptr}, {"a", "i", "u", "A", "I", "U", nullptr},
    {"a", "e", "i", "o", "u", "y", nullptr}, {"a", "e", "i", "o", "u", "ai", "au", nullptr}, {"a", "o", "u", "ei", "ou", nullptr}, {"e", "i", "a", "ie", "ia", nullptr},
    {"a", "e", "o", "u", "oe", "ae", nullptr}, {"a", "e", "i", "o", "u", "ei", "oi", nullptr}, {"a", "o", "u", "O", "U", nullptr}, {"e", "i", "y", "E", "I", "a", nullptr},
    {"a", "e", "i", "o", nullptr}, {"i", "u", "a", "o", "ai", nullptr},
};
constexpr int VSETS_N = (int)(sizeof(VSETS) / sizeof(VSETS[0]));

// the syllables' shapes: C a consonant, V a vowel, L a liquid, S a sibilant, F an ending consonant; '?' after a part: it may be left out
const char* const SHAPES[] = {"CVC", "CVV?C", "CVVC?", "CVC?", "CV", "VC", "CVF", "C?VC", "CVF?", "CL?VC", "CL?VF", "S?CVC", "S?CVF", "S?CVC?",
                              "C?VF", "C?VC?", "C?VF?", "C?L?VC", "CVL?C?", "C?VL?C", "C?VLC?", "CV", "CVF?", "C?V"};
constexpr int SHAPES_N = (int)(sizeof(SHAPES) / sizeof(SHAPES[0]));

uint64_t strHashT(const std::string& s) { uint64_t h = 1469598103934665603ULL; for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; } return h; }
bool isVowelLetter(char c) { c = (char)std::tolower((unsigned char)c); return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y'; }
std::string capWord(std::string s) { if (!s.empty()) s[0] = (char)std::toupper((unsigned char)s[0]); return s; }
// a name that reads as a rude or silly word of ours is drawn again (anywhere in it for the worst, the whole name for the rest)
bool unfortunate(const std::string& name) {
    std::string w; for (char c : name) if (std::isalpha((unsigned char)c)) w += (char)std::tolower((unsigned char)c);
    static const char* const ANY[] = {"fuck", "fuk", "shit", "cunt", "piss", "cock", "dick", "slut", "rape", "nazi", "fag", "porn", "wank", "twat", "nigg", "kike", "spic", "turd", "puke", "poop", "jizz", "dildo", "anus", "penis", "vagin", "tits", "boob", "fart", "crap", "bitch", "whore", "pussy", nullptr};
    static const char* const WHOLE[] = {"poo", "pooh", "pee", "wee", "bum", "butt", "ass", "arse", "tit", "sex", "gay", "snot", "cum", "god", "jesus", "allah", "satan", "hell", "damn", "fat", "pus", "puss", "dung", "pis", "nob", "knob", "dong", "ho", "hoe", "lol", nullptr};
    for (int i = 0; ANY[i]; i++) if (w.find(ANY[i]) != std::string::npos) return true;
    for (int i = 0; WHOLE[i]; i++) if (w == WHOLE[i]) return true;
    // a part of a joined name that is a god or a name of ours ("Odin-Cid"), and a stutter thrice ("Sgisgisgis")
    static const char* const PART[] = {"odin", "thor", "zeus", "loki", "jesus", "christ", "allah", "buddha", "satan", "lucifer", "hitler", "stalin", "mao", "kali",
                                       "math", "ham", "bob", "tom", "jim", "joe", "ann", "dan", "max", nullptr};
    {
        std::string part; const std::string full = name + "-";
        for (char c : full) {
            if (std::isalpha((unsigned char)c)) { part += (char)std::tolower((unsigned char)c); continue; }
            for (int i = 0; PART[i]; i++) if (part == PART[i]) return true;
            part.clear();
        }
    }
    for (size_t len = 2; len <= 4; len++)
        for (size_t i = 0; i + 3 * len <= w.size(); i++)
            if (w.compare(i, len, w, i + len, len) == 0 && w.compare(i, len, w, i + 2 * len, len) == 0) return true;
    return false;
}

// a phoneme in a word: a consonant (its index into CONS) or a vowel (into VOWS)
struct Ph { bool vowel; int i; };

// the built tongue's sounds (the codes chosen and their spellings), kept beside the struct the callers read
struct Sounds {
    std::vector<int> cons, vows, liquids, sibs, finals;   // indices
    std::vector<int> clusterHeads, sibHeads;              // the consonants that may lead a cluster before a liquid, after a sibilant
    std::vector<std::string> consSp, vowSp;                // by index into CONS / VOWS: the spelling this tongue uses
    std::vector<std::string> shapes; std::vector<double> shapeW;
    double optional = 0.5;   // a '?' part's chance of being there
    bool noDoubleVowel = true;
};
std::mutex g_soundsMu;
std::map<uint64_t, std::shared_ptr<const Sounds>> g_sounds;

// a syllable of the shape, its parts drawn from the tongue's sets
void syllable(const Sounds& S, const std::string& shape, Rng& r, std::vector<Ph>& out) {
    for (size_t i = 0; i < shape.size(); i++) {
        const char c = shape[i];
        const bool opt = i + 1 < shape.size() && shape[i + 1] == '?';
        if (opt) i++;
        if (opt && !r.chance(S.optional)) continue;
        if (c == 'V' && !out.empty() && out.back().vowel && S.noDoubleVowel) continue;   // a second vowel where the tongue keeps them apart
        const std::vector<int>* pool = nullptr; bool vowel = false;
        const char next = i + 1 < shape.size() ? shape[i + 1] : 0;
        const bool prevS = !out.empty() && !out.back().vowel && c == 'C' && i > 0 && (shape[i - 1] == 'S' || (shape[i - 1] == '?' && i > 1 && shape[i - 2] == 'S'));
        switch (c) {
            case 'C':   // a cluster's head: a stop or a fricative before a liquid, a stop after a sibilant
                pool = &S.cons;
                if (next == 'L' && !S.clusterHeads.empty() && r.chance(S.optional)) { out.push_back({false, S.clusterHeads[r.irange((int)S.clusterHeads.size())]}); i++; if (i + 1 < shape.size() && shape[i + 1] == '?') i++; out.push_back({false, S.liquids[r.irange((int)S.liquids.size())]}); continue; }
                if (next == 'L') { i++; if (i + 1 < shape.size() && shape[i + 1] == '?') i++; }
                if (prevS && !S.sibHeads.empty()) { out.push_back({false, S.sibHeads[r.irange((int)S.sibHeads.size())]}); continue; }
                break;
            case 'V': pool = &S.vows; vowel = true; break;
            case 'L': pool = nullptr; break;   // only after a cluster's head (above)
            case 'S': pool = S.sibs.empty() ? nullptr : &S.sibs; break;
            case 'F': pool = S.finals.empty() ? &S.cons : &S.finals; break;
            default: break;
        }
        if (!pool || pool->empty()) continue;
        // the first consonant of a pool weighs most (a tongue's commonest sounds), as Zipf would have it
        const int n = (int)pool->size();
        int k = (int)(std::pow(r.uni(), 1.6) * n); if (k >= n) k = n - 1;
        out.push_back({vowel, (*pool)[k]});
    }
}
// a syllable with no vowel (a shape's optional parts all left out) gets one
void syllableV(const Sounds& S, const std::string& shape, Rng& r, std::vector<Ph>& out) {
    const size_t from = out.size();
    syllable(S, shape, r, out);
    for (size_t k = from; k < out.size(); k++) if (out[k].vowel) return;
    int k = (int)(std::pow(r.uni(), 1.6) * S.vows.size()); if (k >= (int)S.vows.size()) k = (int)S.vows.size() - 1;
    Ph v{true, S.vows[k]};
    if (out.size() > from) out.insert(out.begin() + (long)(from + 1 < out.size() ? from + 1 : out.size()), v); else out.push_back(v);
}
// the word's sounds spelt, with the tongue's doubling between vowels; a sound twice in a row, or two vowels where the tongue has none
// together, fail it (false: drawn again)
bool spell(const Sounds& S, const Tongue& T, const std::vector<Ph>& ph, Rng& r, std::string& out) {
    out.clear();
    for (size_t i = 0; i < ph.size(); i++) {
        if (i > 0 && ph[i].vowel == ph[i - 1].vowel && ph[i].i == ph[i - 1].i) return false;
        if (i > 0 && ph[i].vowel && ph[i - 1].vowel && S.noDoubleVowel) return false;
        if (i > 1 && !ph[i].vowel && !ph[i - 1].vowel && !ph[i - 2].vowel) {   // three consonants together: only a liquid or a sibilant among them
            int soft = 0;
            for (size_t k = i - 2; k <= i; k++) { const int kd = CONS[ph[k].i].kind; if (kd == SK_LIQUID || kd == SK_GLIDE || std::strcmp(CONS[ph[k].i].code, "s") == 0) soft++; }
            if (soft < 2) return false;
        }
        if (i > 0 && !ph[i].vowel && !ph[i - 1].vowel) {   // two consonants meeting: not two affricates, not a glottal stop or a glide before another
            const Cons& a = CONS[ph[i - 1].i]; const Cons& b = CONS[ph[i].i];
            if (a.kind == SK_AFFR && b.kind == SK_AFFR) return false;
            if (std::strcmp(a.code, "?") == 0 || std::strcmp(b.code, "?") == 0 || a.kind == SK_GLIDE || std::strcmp(a.code, "h") == 0) return false;
            if ((a.kind == SK_AFFR || a.kind == SK_FRIC) && (b.kind == SK_AFFR) ) return false;
            if (i == 1) {   // a word's first two: a stop or a fricative before l or r, s or sh before a stop
                const bool head = (a.kind == SK_STOP || std::strcmp(a.code, "f") == 0 || std::strcmp(a.code, "v") == 0 || std::strcmp(a.code, "th") == 0 || std::strcmp(a.code, "sh") == 0 || std::strcmp(a.code, "x") == 0)
                                  && (std::strcmp(b.code, "l") == 0 || std::strcmp(b.code, "r") == 0);
                const bool sib = (std::strcmp(a.code, "s") == 0 || std::strcmp(a.code, "sh") == 0) && b.kind == SK_STOP && std::strcmp(b.code, "?") != 0;
                if (!head && !sib) return false;
            }
        }
        const std::string& sp = ph[i].vowel ? S.vowSp[ph[i].i] : S.consSp[ph[i].i];
        const bool between = !ph[i].vowel && i > 0 && i + 1 < ph.size() && ph[i - 1].vowel && ph[i + 1].vowel;
        if (between && sp.size() == 1 && sp != "'" && sp != "h" && sp != "w" && sp != "y" && T.gem > 0 && r.chance(T.gem)) out += sp;
        out += sp;
    }
    // no three letters alike, no apostrophe at either end, no four consonant letters together, nothing unreadable
    for (size_t i = 2; i < out.size(); i++) if (out[i] == out[i - 1] && out[i] == out[i - 2]) return false;
    { int run = 0; for (char c : out) { run = (std::isalpha((unsigned char)c) && !isVowelLetter(c)) ? run + 1 : 0; if (run >= 4) return false; } }
    if (!out.empty() && (out[0] == '\'' || out.back() == '\'')) return false;
    if (out.find("''") != std::string::npos) return false;
    return !out.empty();
}
// a root of n syllables
std::string word(const Sounds& S, const Tongue& T, int syl, Rng& r) {
    std::string w;
    for (int tries = 0; tries < 24; tries++) {
        std::vector<Ph> ph;
        for (int s = 0; s < syl; s++) {
            const int k = r.pick(S.shapeW.data(), (int)S.shapeW.size());
            syllableV(S, S.shapes[k], r, ph);
        }
        if (ph.empty()) continue;
        bool hasV = false; for (const Ph& p : ph) if (p.vowel) hasV = true;
        if (!hasV) continue;
        if (spell(S, T, ph, r, w) && w.size() >= 2) return w;
    }
    w.clear();   // the shapes would not fit: plain syllables of a consonant and a vowel
    for (int s = 0; s < std::max(1, syl); s++) w += S.consSp[S.cons[r.irange((int)S.cons.size())]] + S.vowSp[S.vows[r.irange((int)S.vows.size())]];
    return w;
}
std::shared_ptr<const Sounds> soundsOf(const Tongue& T);

// two parts joined by the tongue's way ("Kal-Dor", "Kaldor", "Kal'dor"); a vowel meeting a vowel loses the first where the joiner is none
std::string join(const Tongue& T, std::string a, std::string b) {
    if (a.empty()) return b; if (b.empty()) return a;
    if (T.joiner.empty()) {
        if (isVowelLetter(a.back()) && isVowelLetter(b[0]) && a.size() > 2) a.pop_back();
        if (a.back() == b[0]) b.erase(0, 1);
        return a + b;
    }
    if (T.joiner == "-") return a + "-" + (T.capSecond ? capWord(b) : b);
    // an apostrophe joins where the sounds meet hard (two consonants or two vowels); else nothing
    const bool hard = isVowelLetter(a.back()) == isVowelLetter(b[0]);
    return a + (hard ? "'" : "") + b;
}
// a given name's ending by sex: a vowel ending meeting a vowel replaces it
std::string ending(std::string base, const std::string& end) {
    if (end.empty()) return base;
    if (isVowelLetter(base.back()) && isVowelLetter(end[0])) base.pop_back();
    return base + end;
}

}   // namespace

// ---- the tongue's draw ------------------------------------------------------------------------------------------------------------

namespace {
Tongue buildTongue(uint64_t peopleSeed) {
    Tongue T;
    T.people = peopleSeed;
    T.seed = mix64(peopleSeed ^ 0x70A6E5ULL);   // C-06's salt: the people's
    Rng r(mix64(T.seed ^ 0x2B0A5ULL));
    auto S = std::make_shared<Sounds>();
    // the consonants: a set, a few dropped, one or two added
    const ConsSet& cs = CSETS[r.irange(CSETS_N)];
    T.inventory = cs.name;
    std::vector<int> cons;
    for (int i = 0; cs.codes[i]; i++) { const int k = consIndex(cs.codes[i]); if (k >= 0) cons.push_back(k); }
    for (size_t i = 0; i < cons.size() && cons.size() > 6;) { if (r.chance(0.12)) cons.erase(cons.begin() + i); else i++; }
    for (int add = r.irange(3); add > 0; add--) { const int k = r.irange(CONS_N); if (std::find(cons.begin(), cons.end(), k) == cons.end()) cons.push_back(k); }
    for (size_t i = cons.size(); i > 1; i--) std::swap(cons[i - 1], cons[r.irange((int)i)]);   // shuffled: the first drawn most
    S->cons = cons;
    // the vowels
    const char* const* vs = VSETS[r.irange(VSETS_N)];
    for (int i = 0; vs[i]; i++) for (int k = 0; k < VOWS_N; k++) if (std::strcmp(VOWS[k].code, vs[i]) == 0) S->vows.push_back(k);
    for (size_t i = S->vows.size(); i > 1; i--) std::swap(S->vows[i - 1], S->vows[r.irange((int)i)]);
    // the spelling: each sound's letters, never two sounds alike
    S->consSp.assign(CONS_N, std::string()); S->vowSp.assign(VOWS_N, std::string());
    std::vector<std::string> taken;
    auto choose = [&](const char* const* sp, int n, const char* code) {
        std::vector<std::string> opts; for (int i = 0; i < n && sp[i]; i++) opts.push_back(sp[i]);
        if (opts.empty()) opts.push_back(code);
        const int first = r.chance(0.55) ? 0 : r.irange((int)opts.size());
        for (int k = 0; k < (int)opts.size(); k++) {
            const std::string& o = opts[(first + k) % opts.size()];
            if (std::find(taken.begin(), taken.end(), o) == taken.end()) { taken.push_back(o); return o; }
        }
        taken.push_back(std::string(code) + "h"); return taken.back();
    };
    for (int k : S->cons) S->consSp[k] = choose(CONS[k].spell, 4, CONS[k].code);
    for (int k : S->vows) S->vowSp[k] = choose(VOWS[k].spell, 3, VOWS[k].code);
    // the liquids and the sibilants that may join a cluster; the consonants that may end a syllable
    for (int k : S->cons) {
        const std::string c = CONS[k].code;
        if (c == "l" || c == "r") S->liquids.push_back(k);
        if (c == "s" || c == "sh") S->sibs.push_back(k);
        if (CONS[k].kind == SK_STOP && c != "?" && c != "q" && c != "kw") { S->clusterHeads.push_back(k); S->sibHeads.push_back(k); }
        if (c == "f" || c == "v" || c == "th" || c == "sh" || c == "x") S->clusterHeads.push_back(k);
    }
    if (S->liquids.empty()) S->clusterHeads.clear();
    {
        const int fin = r.irange(4);   // 0 any, 1 the nasals and the liquids, 2 those and s, 3 a few at random
        for (int k : S->cons) {
            const int kd = CONS[k].kind; const std::string c = CONS[k].code;
            bool ok = fin == 0 || (fin == 1 && (kd == SK_NASAL || kd == SK_LIQUID)) || (fin == 2 && (kd == SK_NASAL || kd == SK_LIQUID || c == "s" || c == "sh")) || (fin == 3 && r.chance(0.45));
            if (c == "?" || c == "w" || c == "j" || c == "h" || kd == SK_AFFR || c == "kw" || c == "hl" || c == "ny" || c == "zh" || c == "gh" || c == "dh") ok = false;
            if (ok) S->finals.push_back(k);
        }
        if (S->finals.empty()) for (int k : S->cons) if (CONS[k].kind == SK_NASAL) S->finals.push_back(k);
    }
    // the shapes: one, two or three, the first the commonest
    {
        const int n = 1 + r.irange(3);
        for (int i = 0; i < n; i++) {
            std::string sh = SHAPES[r.irange(SHAPES_N)];
            if ((sh.find('L') != std::string::npos && S->liquids.empty()) || (sh.find('S') != std::string::npos && S->sibs.empty())) sh = "CVC?";
            if (std::find(S->shapes.begin(), S->shapes.end(), sh) != S->shapes.end()) continue;
            S->shapes.push_back(sh); S->shapeW.push_back(i == 0 ? 3.0 : 1.0);
        }
    }
    S->optional = 0.3 + 0.45 * r.uni();
    S->noDoubleVowel = !r.chance(0.3);
    T.gem = r.chance(0.3) ? 0.15 + 0.35 * r.uni() : 0;
    T.minSyl = 1 + (r.chance(0.25) ? 1 : 0);
    T.maxSyl = T.minSyl + (r.chance(0.6) ? 1 : 0) + (r.chance(0.2) ? 1 : 0);
    // the callers' lists, spelt
    for (int k : S->cons) T.cons.push_back(S->consSp[k]);
    for (int k : S->vows) T.vows.push_back(S->vowSp[k]);
    for (int k : S->liquids) T.liquids.push_back(S->consSp[k]);
    for (int k : S->sibs) T.sibs.push_back(S->consSp[k]);
    for (int k : S->finals) T.finals.push_back(S->consSp[k]);
    T.shapes = S->shapes;
    T.onsets = T.cons; T.nuclei = T.vows; T.codas = T.finals;
    {   // C-06's two numbers, from the shapes
        double coda = 0, bare = 0;
        for (const std::string& sh : S->shapes) { if (sh.find('F') != std::string::npos || (sh.size() > 1 && sh.back() == 'C') || sh.find("C?") == sh.size() - 2) coda += 1; if (sh[0] == 'V' || (sh.size() > 1 && sh[1] == '?')) bare += 1; }
        T.codaChance = 0.2 + 0.6 * coda / S->shapes.size(); T.vowelStart = 0.05 + 0.3 * bare / S->shapes.size();
    }
    {   // how it sounds: the stops and the back fricatives against the liquids, the glides and the nasals
        double hard = 0, soft = 0;
        for (int k : S->cons) { const int kd = CONS[k].kind; if (kd == SK_STOP || kd == SK_AFFR || CONS[k].back) hard += 1; if (kd == SK_LIQUID || kd == SK_GLIDE || kd == SK_NASAL) soft += 1; }
        const double ratio = hard / std::max(1.0, soft);
        T.style = ratio > 1.6 ? 1 : (ratio < 0.8 ? 2 : 0);
    }
    {
        std::lock_guard<std::mutex> lk(g_soundsMu);
        if (g_sounds.size() > 4096) g_sounds.clear();   // a long session's worlds: drawn again when asked (the same draws)
        g_sounds[T.seed] = S;
    }
    // the morphemes: one or two syllables each, all different
    Rng m(mix64(T.seed ^ 0x4D0E5ULL));
    std::vector<std::string> made;
    auto fresh = [&](int syl) {
        for (int t = 0; t < 30; t++) { std::string w = word(*S, T, syl, m); if (w.size() <= 6 && std::find(made.begin(), made.end(), w) == made.end()) { made.push_back(w); return w; } }
        return word(*S, T, syl, m);
    };
    for (int i = 0; i < TM_COUNT; i++) T.morph[i] = fresh(m.chance(0.8) ? 1 : 2);
    const int nRoots = 28 + m.irange(20);
    for (int i = 0; i < nRoots; i++) {
        const int syl = T.minSyl + m.irange(std::max(1, T.maxSyl - T.minSyl + 1));
        T.roots.push_back(fresh(std::min(2, syl)));
    }
    {   // the joining, the order, the persons' endings, the families'
        const double j = m.uni();
        T.joiner = j < 0.55 ? "" : (j < 0.8 ? "-" : "'");
        T.genericFirst = m.chance(0.3);
        T.capSecond = m.chance(0.5);
        if (m.chance(0.55)) {   // the tongue marks a given name's sex: a vowel each (and a consonant after it now and then)
            const std::vector<std::string>& sv = T.vows;
            auto plain = [&]() { for (int t = 0; t < 8; t++) { const std::string& v = sv[m.irange((int)sv.size())]; if (v.size() == 1) return v; } return sv[0]; };
            T.femEnd = plain();
            for (int t = 0; t < 8 && (T.mascEnd.empty() || T.mascEnd == T.femEnd); t++) T.mascEnd = plain();
            if (m.chance(0.35) && !T.finals.empty()) T.mascEnd += T.finals[m.irange((int)T.finals.size())];
            else if (m.chance(0.25) && !T.finals.empty()) T.femEnd += T.finals[m.irange((int)T.finals.size())];
            if (T.femEnd == T.mascEnd) T.mascEnd.clear();
        }
        T.family = m.irange(3);
        T.famAffix = fresh(1);
    }
    {   // the endings for the decoder (a coda for "-s", a syllable each for "-ed" and "-ing")
        Rng er(mix64(T.seed ^ 0xE4D1ULL));
        auto pick = [&](const std::vector<std::string>& v) { return v.empty() ? std::string("a") : v[er.irange((int)v.size())]; };
        T.endS = pick(T.codas);
        for (int k = 0; k < 6 && (T.endPast.empty() || T.endPast == T.endS); k++) T.endPast = pick(T.nuclei) + pick(T.codas);
        for (int k = 0; k < 6 && (T.endIng.empty() || T.endIng == T.endPast || T.endIng == T.endS); k++) T.endIng = pick(T.nuclei) + pick(T.codas);
    }
    return T;
}

std::mutex g_tongueMu;
std::map<uint64_t, std::shared_ptr<const Tongue>> g_tongues;

std::shared_ptr<const Sounds> soundsOf(const Tongue& T) {
    {
        std::lock_guard<std::mutex> lk(g_soundsMu);
        auto it = g_sounds.find(T.seed);
        if (it != g_sounds.end()) return it->second;
    }
    // the sounds dropped from their cache since: the tongue drawn again (the same draws put them back)
    (void)buildTongue(T.people);
    std::lock_guard<std::mutex> lk(g_soundsMu);
    auto it = g_sounds.find(T.seed);
    return it != g_sounds.end() ? it->second : std::shared_ptr<const Sounds>();
}
}   // namespace

// the tongues stay (a few kilobytes each: a session reads a few hundred peoples), so a reference handed out stays good
const Tongue& tongueOfSeed(uint64_t peopleSeed) {
    thread_local uint64_t lastSeed = 0; thread_local std::shared_ptr<const Tongue> last;
    if (last && lastSeed == peopleSeed) return *last;
    std::shared_ptr<const Tongue> t;
    {
        std::lock_guard<std::mutex> lk(g_tongueMu);
        auto it = g_tongues.find(peopleSeed);
        if (it != g_tongues.end()) t = it->second;
    }
    if (!t) {
        t = std::make_shared<const Tongue>(buildTongue(peopleSeed));
        std::lock_guard<std::mutex> lk(g_tongueMu);
        auto it = g_tongues.find(peopleSeed);
        if (it == g_tongues.end()) g_tongues[peopleSeed] = t; else t = it->second;
    }
    lastSeed = peopleSeed; last = t;
    return *t;
}

Tongue tongueOf(const BodyGen& g, const Lore& L) { return tongueOfSeed(loreSeed(g, L)); }

// one syllable for a word of three letters or fewer, two to six, three past that: the people's words run about as long as ours
std::string tongueWord(const Tongue& T, const std::string& w, int attempt) {
    std::shared_ptr<const Sounds> S = soundsOf(T);
    Rng rng(mix64(T.seed ^ strHashT(w)) + (uint64_t)attempt * 0x9E3779B97F4A7C15ULL);
    const int len = (int)w.size();
    int syl = len <= 3 ? 1 : (len <= 6 ? 2 : (rng.chance(0.3) ? 2 : 3));
    if (attempt > 0 && rng.chance(0.5)) syl = std::min(3, syl + 1);
    if (!S) return w;
    std::string out = word(*S, T, syl, rng);
    if (out.size() < 2) out += T.vows.empty() ? "a" : T.vows[0];
    if (unfortunate(out) && attempt < 40) return tongueWord(T, w, attempt + 7);   // drawn again (an attempt past the decoder's own)
    return out;
}

std::string tongueTerm(const Tongue& T, const std::string& key) {
    std::shared_ptr<const Sounds> S = soundsOf(T);
    if (!S) return key;
    Rng rng(mix64(T.seed ^ strHashT(key) ^ 0x7E4A11ULL));
    const int syl = rng.chance(0.55) ? 2 : (rng.chance(0.6) ? 1 : 3);
    std::string out;
    for (int t = 0; t < 8; t++) {
        out = rng.chance(0.35) && !T.roots.empty() ? T.roots[rng.irange((int)T.roots.size())] : word(*S, T, syl, rng);
        if (out.size() >= 3 && out.size() <= 9 && out != tongueWord(T, key) && !unfortunate(out)) break;
    }
    for (char& c : out) c = (char)std::tolower((unsigned char)c);
    return out;
}

bool tongueReadable(const std::string& n) {
    if (n.size() < 2 || unfortunate(n) || !std::isalpha((unsigned char)n[0]) || !std::isalpha((unsigned char)n.back())) return false;
    for (size_t i = 0; i < n.size(); i++) {
        const char c = n[i];
        if (!std::isalpha((unsigned char)c) && c != '-' && c != '\'') return false;
        if (i >= 2 && std::tolower((unsigned char)c) == std::tolower((unsigned char)n[i - 1]) && std::tolower((unsigned char)c) == std::tolower((unsigned char)n[i - 2])) return false;
    }
    return true;
}

int tongueNameKindOfRank(int sclass) {
    switch (sclass) {
        case SC_HAMLET: return TN_HAMLET;
        case SC_VILLAGE: return TN_VILLAGE;
        case SC_CITY: return TN_CITY;
        case SC_CAPITAL: return TN_CAPITAL;
        default: return TN_TOWN;
    }
}

std::string tongueName(const Tongue& T, uint64_t seed, int kind, bool female) {
    std::shared_ptr<const Sounds> S = soundsOf(T);
    if (!S || T.roots.empty()) return "Unnamed";
    Rng r(mix64(T.seed ^ seed ^ ((uint64_t)(kind + 1) * 0x9E3779B97F4A7C15ULL)));
    const size_t maxLen = kind == TN_PERSON ? 9 : (kind == TN_FAMILY ? 10 : 12);
    auto root = [&](int sylMax) {   // a pool root most of the time (the names' family look), a fresh one else
        if (r.chance(0.72)) return T.roots[r.irange((int)T.roots.size())];
        return word(*S, T, 1 + r.irange(std::max(1, sylMax)), r);
    };
    std::string best;
    for (int tries = 0; tries < 20; tries++) {
        std::string n;
        switch (kind) {
            case TN_PERSON: {   // a root, or two joined plainly (a person's name has no hyphen), the ending of their sex
                n = r.chance(0.5) ? word(*S, T, 1 + r.irange(2), r) : root(2);
                if (n.size() <= 5 && r.chance(0.55)) { std::string b = root(1); if (isVowelLetter(n.back()) && isVowelLetter(b[0])) b.erase(0, 1); if (!b.empty() && n.back() != b[0]) n += b; }
                const std::string& end = female ? T.femEnd : T.mascEnd;
                if (!end.empty() && r.chance(0.85)) n = ending(n, end);
                if (n.size() < 4 && r.chance(0.7)) continue;
                break;
            }
            case TN_FAMILY:
                if (T.family == 0) n = ending(root(2), T.famAffix);
                else if (T.family == 1) n = T.famAffix + (T.joiner == "-" ? "-" : "") + root(2);
                else n = join(T, root(1), root(1));
                break;
            case TN_HAMLET: case TN_VILLAGE: case TN_TOWN: case TN_CITY: case TN_CAPITAL: {
                static const int GEN[5][4] = {{TM_HOME, TM_FIELD, TM_FORD, TM_VILLAGE}, {TM_VILLAGE, TM_HOME, TM_FIELD, TM_FORD}, {TM_TOWN, TM_FORD, TM_GATE, TM_HILL},
                                              {TM_CITY, TM_FORT, TM_TOWN, TM_GATE}, {TM_CITY, TM_GREAT, TM_FORT, TM_HOLY}};
                static const int ADJ[10] = {TM_NEW, TM_OLD, TM_GREAT, TM_SMALL, TM_RED, TM_BLACK, TM_WHITE, TM_NORTH, TM_SOUTH, TM_HOLY};
                const int row = kind - TN_HAMLET;
                const double p = r.uni();
                if (p < 0.22) n = root(2);
                else if (p < 0.72) { const std::string gw = T.morph[GEN[row][r.irange(4)]]; n = T.genericFirst ? join(T, gw, root(2)) : join(T, root(2), gw); }
                else if (p < 0.92) n = join(T, T.morph[ADJ[r.irange(10)]], root(2));
                else n = join(T, root(1), root(1));
                break;
            }
            case TN_RIVER: case TN_MOUNTAIN: case TN_SEA: {
                const int gm = kind == TN_RIVER ? (r.chance(0.7) ? TM_RIVER : TM_WATER) : (kind == TN_MOUNTAIN ? (r.chance(0.7) ? TM_MOUNT : TM_HILL) : (r.chance(0.6) ? TM_SEA : TM_LAKE));
                if (r.chance(0.6)) n = T.genericFirst ? join(T, T.morph[gm], root(2)) : join(T, root(2), T.morph[gm]);
                else n = root(2);
                break;
            }
            case TN_PEOPLE: n = r.chance(0.7) ? ending(root(2), T.morph[TM_PEOPLE]) : root(2); break;
            case TN_GOD: n = r.chance(0.25) ? join(T, root(1), T.morph[TM_GOD]) : (r.chance(0.5) ? join(T, root(1), root(1)) : root(2)); break;
            case TN_FESTIVAL: n = r.chance(0.4) ? join(T, root(2), T.morph[TM_FEAST]) : root(2); break;
            case TN_MOON: n = r.chance(0.3) ? join(T, root(1), T.morph[TM_MOON]) : root(2); break;
            case TN_STAR: n = r.chance(0.3) ? join(T, root(1), T.morph[TM_STAR]) : root(2); break;
            default: n = root(2); break;
        }
        if (n.size() < 3) n = join(T, n, root(1));
        if (n.empty() || n[0] == '\'' || n.back() == '\'' || n.back() == '-' || unfortunate(n)) continue;
        if (n.size() <= maxLen) { best = n; break; }
        if (best.empty() || n.size() < best.size()) best = n;
    }
    {   // no letter thrice
        std::string c; for (char ch : best) { if (c.size() >= 2 && std::tolower((unsigned char)c[c.size() - 1]) == std::tolower((unsigned char)ch) && std::tolower((unsigned char)c[c.size() - 2]) == std::tolower((unsigned char)ch)) continue; c += ch; }
        best = c;
    }
    if (best.size() > maxLen) {   // cut at a syllable's end
        size_t cut = maxLen; while (cut > 3 && !isVowelLetter(best[cut - 1])) cut--;
        best = best.substr(0, cut);
        while (!best.empty() && (best.back() == '\'' || best.back() == '-')) best.pop_back();
    }
    return capWord(best);
}
