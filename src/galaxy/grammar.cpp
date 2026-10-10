// H-03 (2026-10-09): the grammar engine (`grammar.h`): the `.vg` format's parser and link, the expansion, the realiser, the checks
// and the measures. The prototype H-01 measured (`RESEARCH-records.md`, section 8) in the tree's own terms: texts parsed once into
// nodes, references resolved against the model's schema at load, a record's attributes kept in reading order.
#include "grammar.h"
#include "../core/rng.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <unordered_set>

const char* const VG_AGES[6] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "farfuture"};
const char* const VG_CHECK_NAMES[VC_COUNT] = {"mark left", "diagnostic", "doubled word", "a or an", "article before a plural", "pronoun and verb",
                                              "capital", "spacing", "length", "anachronism"};

namespace {

// the catalogue's dryness: an alternative the people's earlier records used k times weighs 1 / (1 + VG_DRYNESS k) (H-03's 1.5; H-04: 4,
// so a people's records go through a part's alternatives before they come back to one, its weights still telling which first)
constexpr double VG_DRYNESS = 4.0;
// H-04's second pass: a people's dialect (an alternative's weight for a people times exp(sigma z), z a normal of the people's seed and the
// alternative's key; the genres' own `steady` root leans less), the words worn out (an alternative weighs 1 / (1 + VG_FATIGUE n), n the
// people's uses of its most used distinctive word: a word in VG_DISTINCT alternatives or fewer), a repeated sentence drawn again (a rule
// VG_RETRIES times at most, a record VG_RETRY_BUDGET draws undone in all)
constexpr double VG_DIALECT = 0.9, VG_DIALECT_STEADY = 0.45;
constexpr double VG_FATIGUE = 1.2;
constexpr int VG_DISTINCT = 24;
constexpr int VG_RETRIES = 6, VG_RETRY_BUDGET = 160;

double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
std::string lower(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }
std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
bool isVowelChar(char c) { c = (char)std::tolower((unsigned char)c); return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }
bool endsWith(const std::string& w, const char* x) { size_t n = std::strlen(x); return w.size() >= n && w.compare(w.size() - n, n, x) == 0; }
bool startsWith(const std::string& w, const char* x) { size_t n = std::strlen(x); return w.size() >= n && w.compare(0, n, x) == 0; }
int ageOf(const std::string& s) { for (int i = 0; i < 6; i++) if (s == VG_AGES[i]) return i; return -1; }
// a number in a value: digits ("212", "0.5") or an age's name (its order); false when it is neither
bool numberOf(const std::string& s, double& out) {
    if (s.empty()) return false;
    int a = ageOf(s); if (a >= 0) { out = a; return true; }
    char* e = nullptr; out = std::strtod(s.c_str(), &e);
    return e && *e == 0;
}
// a count written in words ("twenty", "a hundred", "three hundred") or digits: -1 neither
long countOf(const std::string& text) {
    std::string s = lower(trim(text));
    if (s.empty()) return -1;
    if (std::isdigit((unsigned char)s[0])) { long n = 0; for (char c : s) { if (c == ',') continue; if (!std::isdigit((unsigned char)c)) return -1; n = n * 10 + (c - '0'); } return n; }
    static const std::map<std::string, long> W = {{"no", 0}, {"none", 0}, {"zero", 0}, {"one", 1}, {"a", 1}, {"two", 2}, {"three", 3}, {"four", 4}, {"five", 5}, {"six", 6}, {"seven", 7},
        {"eight", 8}, {"nine", 9}, {"ten", 10}, {"eleven", 11}, {"twelve", 12}, {"thirteen", 13}, {"fourteen", 14}, {"fifteen", 15}, {"sixteen", 16}, {"seventeen", 17},
        {"eighteen", 18}, {"nineteen", 19}, {"twenty", 20}, {"thirty", 30}, {"forty", 40}, {"fifty", 50}, {"sixty", 60}, {"seventy", 70}, {"eighty", 80}, {"ninety", 90},
        {"hundred", 100}, {"thousand", 1000}, {"dozen", 12}, {"score", 20}};
    long total = 0, cur = 0; bool any = false;
    size_t p = 0;
    while (p < s.size()) {
        size_t q = s.find_first_of(" -", p); if (q == std::string::npos) q = s.size();
        std::string w = s.substr(p, q - p); p = q + 1;
        if (w.empty() || w == "and" || w == "some") continue;
        auto it = W.find(w); if (it == W.end()) return -1;
        any = true;
        if (it->second == 100) cur = (cur ? cur : 1) * 100;
        else if (it->second == 1000) { total += (cur ? cur : 1) * 1000; cur = 0; }
        else cur += it->second;
    }
    return any ? total + cur : -1;
}

const std::map<std::string, std::pair<const char*, const char*>>& irregularVerbs() {   // past, participle
    static const std::map<std::string, std::pair<const char*, const char*>> M = {
        {"be", {"was", "been"}}, {"have", {"had", "had"}}, {"do", {"did", "done"}}, {"go", {"went", "gone"}}, {"come", {"came", "come"}}, {"take", {"took", "taken"}},
        {"give", {"gave", "given"}}, {"make", {"made", "made"}}, {"say", {"said", "said"}}, {"see", {"saw", "seen"}}, {"find", {"found", "found"}}, {"bring", {"brought", "brought"}},
        {"buy", {"bought", "bought"}}, {"sell", {"sold", "sold"}}, {"build", {"built", "built"}}, {"fall", {"fell", "fallen"}}, {"rise", {"rose", "risen"}}, {"run", {"ran", "run"}},
        {"keep", {"kept", "kept"}}, {"leave", {"left", "left"}}, {"lose", {"lost", "lost"}}, {"send", {"sent", "sent"}}, {"set", {"set", "set"}}, {"sit", {"sat", "sat"}},
        {"stand", {"stood", "stood"}}, {"tell", {"told", "told"}}, {"think", {"thought", "thought"}}, {"write", {"wrote", "written"}}, {"read", {"read", "read"}},
        {"break", {"broke", "broken"}}, {"choose", {"chose", "chosen"}}, {"eat", {"ate", "eaten"}}, {"feed", {"fed", "fed"}}, {"fight", {"fought", "fought"}}, {"fly", {"flew", "flown"}},
        {"forget", {"forgot", "forgotten"}}, {"freeze", {"froze", "frozen"}}, {"get", {"got", "got"}}, {"grow", {"grew", "grown"}}, {"hang", {"hung", "hung"}}, {"hear", {"heard", "heard"}},
        {"hold", {"held", "held"}}, {"know", {"knew", "known"}}, {"lay", {"laid", "laid"}}, {"lead", {"led", "led"}}, {"lie", {"lay", "lain"}}, {"light", {"lit", "lit"}}, {"mean", {"meant", "meant"}},
        {"meet", {"met", "met"}}, {"pay", {"paid", "paid"}}, {"put", {"put", "put"}}, {"ride", {"rode", "ridden"}}, {"ring", {"rang", "rung"}}, {"seek", {"sought", "sought"}}, {"shake", {"shook", "shaken"}},
        {"shine", {"shone", "shone"}}, {"shut", {"shut", "shut"}}, {"sing", {"sang", "sung"}}, {"sink", {"sank", "sunk"}}, {"sleep", {"slept", "slept"}}, {"speak", {"spoke", "spoken"}},
        {"spend", {"spent", "spent"}}, {"steal", {"stole", "stolen"}}, {"strike", {"struck", "struck"}}, {"swear", {"swore", "sworn"}}, {"swim", {"swam", "swum"}}, {"teach", {"taught", "taught"}},
        {"tear", {"tore", "torn"}}, {"throw", {"threw", "thrown"}}, {"wake", {"woke", "woken"}}, {"wear", {"wore", "worn"}}, {"weep", {"wept", "wept"}}, {"win", {"won", "won"}},
        {"bear", {"bore", "borne"}}, {"begin", {"began", "begun"}}, {"blow", {"blew", "blown"}}, {"catch", {"caught", "caught"}}, {"cut", {"cut", "cut"}}, {"dig", {"dug", "dug"}},
        {"draw", {"drew", "drawn"}}, {"drink", {"drank", "drunk"}}, {"drive", {"drove", "driven"}}, {"feel", {"felt", "felt"}}, {"hide", {"hid", "hidden"}}, {"let", {"let", "let"}},
        {"sweep", {"swept", "swept"}}, {"shoot", {"shot", "shot"}}, {"bend", {"bent", "bent"}}, {"lend", {"lent", "lent"}}, {"wind", {"wound", "wound"}}, {"bite", {"bit", "bitten"}},
        {"bleed", {"bled", "bled"}}, {"breed", {"bred", "bred"}}, {"burn", {"burned", "burned"}}, {"cling", {"clung", "clung"}}, {"deal", {"dealt", "dealt"}}, {"dream", {"dreamt", "dreamt"}},
        {"flee", {"fled", "fled"}}, {"forgive", {"forgave", "forgiven"}}, {"grind", {"ground", "ground"}}, {"hit", {"hit", "hit"}}, {"hurt", {"hurt", "hurt"}}, {"kneel", {"knelt", "knelt"}},
        {"leap", {"leapt", "leapt"}}, {"quit", {"quit", "quit"}}, {"slay", {"slew", "slain"}}, {"slide", {"slid", "slid"}}, {"sow", {"sowed", "sown"}}, {"spin", {"spun", "spun"}},
        {"spit", {"spat", "spat"}}, {"split", {"split", "split"}}, {"spread", {"spread", "spread"}}, {"spring", {"sprang", "sprung"}}, {"sting", {"stung", "stung"}}, {"stride", {"strode", "stridden"}},
        {"string", {"strung", "strung"}}, {"swell", {"swelled", "swollen"}}, {"swing", {"swung", "swung"}}, {"tread", {"trod", "trodden"}}, {"understand", {"understood", "understood"}},
        {"weave", {"wove", "woven"}}, {"wring", {"wrung", "wrung"}}, {"arise", {"arose", "arisen"}}, {"awake", {"awoke", "awoken"}}, {"bid", {"bade", "bidden"}}, {"bind", {"bound", "bound"}},
        {"cast", {"cast", "cast"}}, {"cost", {"cost", "cost"}}, {"creep", {"crept", "crept"}}, {"forbid", {"forbade", "forbidden"}}, {"foresee", {"foresaw", "foreseen"}}, {"mistake", {"mistook", "mistaken"}},
        {"overcome", {"overcame", "overcome"}}, {"rid", {"rid", "rid"}}, {"sew", {"sewed", "sewn"}}, {"shed", {"shed", "shed"}}, {"shrink", {"shrank", "shrunk"}}, {"stick", {"stuck", "stuck"}},
        {"stink", {"stank", "stunk"}}, {"withdraw", {"withdrew", "withdrawn"}}};
    return M;
}
bool isModal(const std::string& w) { return w == "can" || w == "will" || w == "must" || w == "may" || w == "shall" || w == "should" || w == "would" || w == "could" || w == "might"; }
std::string pastOf(const std::string& w) {
    auto it = irregularVerbs().find(w); if (it != irregularVerbs().end()) return it->second.first;
    if (w == "can") return "could"; if (w == "will") return "would"; if (w == "may") return "might"; if (w == "shall") return "should";
    if (isModal(w)) return w;
    static const std::set<std::string> DOUBLE = {"stop", "plan", "drop", "step", "beg", "ship", "rob", "nod", "trip", "plot", "fit", "admit", "pin", "chop", "slip", "rot", "wed",
        "dim", "stir", "hug", "mop", "tap", "skip", "scrub", "trim", "grab", "drag", "tug", "hop", "clap", "flap", "wrap", "trap", "nag", "jog", "ban", "scan", "sob", "throb", "dip"};
    if (DOUBLE.count(w)) return w + w.back() + "ed";
    if (!w.empty() && w.back() == 'e') return w + "d";
    if (w.size() > 1 && w.back() == 'y' && !isVowelChar(w[w.size() - 2])) return w.substr(0, w.size() - 1) + "ied";
    return w + "ed";
}
std::string thirdOf(const std::string& w) {
    if (w == "have") return "has"; if (w == "be") return "is"; if (w == "do") return "does"; if (w == "go") return "goes";
    if (isModal(w)) return w;
    if (endsWith(w, "s") || endsWith(w, "x") || endsWith(w, "z") || endsWith(w, "ch") || endsWith(w, "sh") || endsWith(w, "o")) return w + "es";
    if (w.size() > 1 && w.back() == 'y' && !isVowelChar(w[w.size() - 2])) return w.substr(0, w.size() - 1) + "ies";
    return w + "s";
}

// the sound a word starts with, for "a" and "an": a vowel's, except the vowels that sound as a consonant (one, a use, a unit, a
// European) and the silent h (an hour, an heir); a numeral by how it is said (an 8, an 11, an 18)
bool vowelSound(const std::string& word) {
    std::string w = lower(word);
    while (!w.empty() && !std::isalnum((unsigned char)w[0])) w.erase(0, 1);
    if (w.empty()) return false;
    if (std::isdigit((unsigned char)w[0])) {
        if (w[0] == '8') return true;
        std::string d; for (char c : w) { if (std::isdigit((unsigned char)c)) d += c; else if (c != ',') break; }
        return (d.size() == 2 || d.size() == 5) && (startsWith(d, "11") || startsWith(d, "18"));
    }
    static const char* const CONSONANT[] = {"one", "once", "eu", "ewe", "uni", "use", "usu", "uti", "ura", "uri", "uro", "ubi", "uku", "ufo", "unanim", nullptr};
    static const char* const SILENT_H[] = {"hour", "honest", "honour", "honor", "heir", nullptr};
    for (int i = 0; SILENT_H[i]; i++) if (startsWith(w, SILENT_H[i])) return true;
    if (!isVowelChar(w[0])) return false;
    for (int i = 0; CONSONANT[i]; i++) if (startsWith(w, CONSONANT[i])) return false;
    return true;
}

}   // namespace

// ---------------------------------------------------------------------------
// the realiser's pieces
// ---------------------------------------------------------------------------

std::string vgInflect(const std::string& lemma, int person, bool plural, int tense) {
    size_t sp = lemma.find(' ');
    std::string w = lemma.substr(0, sp), rest = sp == std::string::npos ? "" : lemma.substr(sp);
    std::string f;
    bool singular3 = person == 3 && !plural, singular1 = person == 1 && !plural;
    if (tense == VG_PAST) {
        if (w == "be") f = (singular1 || singular3) ? "was" : "were";
        else f = pastOf(w);
    } else {
        if (w == "be") f = singular1 ? "am" : (singular3 ? "is" : "are");
        else f = singular3 ? thirdOf(w) : w;
    }
    return f + rest;
}

std::string vgPlural(const std::string& noun) {
    if (noun.empty()) return noun;
    size_t of = noun.find(" of ");
    if (of != std::string::npos) return vgPlural(noun.substr(0, of)) + noun.substr(of);   // "bed of the river" -> "beds of the river"
    size_t sp = noun.rfind(' ');
    std::string pre = sp == std::string::npos ? "" : noun.substr(0, sp + 1), w = sp == std::string::npos ? noun : noun.substr(sp + 1);
    static const std::map<std::string, std::string> IRR = {{"child", "children"}, {"man", "men"}, {"woman", "women"}, {"foot", "feet"}, {"tooth", "teeth"}, {"goose", "geese"},
        {"mouse", "mice"}, {"ox", "oxen"}, {"sheep", "sheep"}, {"fish", "fish"}, {"deer", "deer"}, {"person", "people"}, {"knife", "knives"}, {"wife", "wives"}, {"life", "lives"},
        {"leaf", "leaves"}, {"wolf", "wolves"}, {"half", "halves"}, {"loaf", "loaves"}, {"calf", "calves"}, {"shelf", "shelves"}, {"thief", "thieves"}, {"cattle", "cattle"},
        {"people", "people"}, {"barley", "barley"}, {"millet", "millet"}, {"salt", "salt"}};
    std::string lw = lower(w);
    for (const auto& kv : IRR) if (endsWith(lw, kv.first.c_str()) && (lw.size() == kv.first.size() || !std::isalpha((unsigned char)lw[lw.size() - kv.first.size() - 1]) || kv.first.size() >= 4))
        return pre + w.substr(0, w.size() - kv.first.size()) + kv.second;
    if (endsWith(lw, "s") || endsWith(lw, "x") || endsWith(lw, "z") || endsWith(lw, "ch") || endsWith(lw, "sh")) return pre + w + "es";
    if (lw.size() > 1 && lw.back() == 'y' && !isVowelChar(lw[lw.size() - 2])) return pre + w.substr(0, w.size() - 1) + "ies";
    return pre + w + "s";
}

std::string vgIrregularLemma(const std::string& word) {
    static const std::unordered_map<std::string, std::string> M = [] {
        std::unordered_map<std::string, std::string> m;
        for (const auto& kv : irregularVerbs()) { if (kv.first != kv.second.first) m[kv.second.first] = kv.first; if (kv.first != kv.second.second) m[kv.second.second] = kv.first; }
        m["is"] = "be"; m["am"] = "be"; m["are"] = "be"; m["were"] = "be"; m["was"] = "be"; m["has"] = "have"; m["does"] = "do";
        const char* const PL[][2] = {{"children", "child"}, {"men", "man"}, {"women", "woman"}, {"feet", "foot"}, {"teeth", "tooth"}, {"geese", "goose"}, {"mice", "mouse"},
                                     {"oxen", "ox"}, {"knives", "knife"}, {"wives", "wife"}, {"lives", "life"}, {"leaves", "leaf"}, {"wolves", "wolf"}, {"halves", "half"},
                                     {"loaves", "loaf"}, {"calves", "calf"}, {"shelves", "shelf"}, {"thieves", "thief"}};
        for (const auto& p : PL) m[p[0]] = p[1];
        return m;
    }();
    auto it = M.find(word);
    return it == M.end() ? std::string() : it->second;
}

std::string vgNumberWords(long n) {
    static const char* const S[] = {"no", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen",
                                    "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
    static const char* const T[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
    if (n < 0) return "minus " + vgNumberWords(-n);
    if (n < 20) return S[n];
    if (n < 100) return std::string(T[n / 10]) + (n % 10 ? std::string("-") + S[n % 10] : "");
    if (n < 1000) return (n / 100 == 1 ? std::string("a hundred") : vgNumberWords(n / 100) + " hundred") + (n % 100 ? " and " + vgNumberWords(n % 100) : "");
    if (n < 1000000) {
        long th = n / 1000, r = n % 1000;
        return (th == 1 ? std::string("a thousand") : vgNumberWords(th) + " thousand") + (r ? (r < 100 ? " and " : " ") + vgNumberWords(r) : "");
    }
    long mi = n / 1000000, r = n % 1000000;
    return (mi == 1 ? std::string("a million") : vgNumberWords(mi) + " million") + (r ? (r < 100 ? " and " : " ") + vgNumberWords(r) : "");
}
std::string vgOrdinal(long n) {
    static const char* const O[] = {"zeroth", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth", "eleventh", "twelfth"};
    if (n >= 0 && n <= 12) return O[n];
    long m = n % 100;
    const char* suf = (m >= 11 && m <= 13) ? "th" : (n % 10 == 1 ? "st" : (n % 10 == 2 ? "nd" : (n % 10 == 3 ? "rd" : "th")));
    return std::to_string(n) + suf;
}
namespace {
std::string aboutWords(long n) {   // "some three hundred": a round number (H-04: "some six million", "some forty thousand")
    if (n <= 12) return vgNumberWords(n);
    if (n < 100) return "some " + vgNumberWords((n + 5) / 10 * 10);
    if (n < 1000) { long r = (n + 50) / 100 * 100; return r == 100 ? std::string("a hundred") : (r == 1000 ? std::string("a thousand") : "some " + vgNumberWords(r)); }
    if (n < 1000000) {
        long step = n < 20000 ? 1000 : 10000, r = (n + step / 2) / step * step;
        if (r >= 1000000) return "a million";
        if (r == 1000) return "a thousand";
        const std::string w = vgNumberWords(r / 1000) + " thousand";
        return w.compare(0, 2, "a ") == 0 ? w : "some " + w;
    }
    long step = n < 20000000 ? 1000000 : (n < 200000000 ? 10000000 : 100000000), r = (n + step / 2) / step * step;
    const std::string w = vgNumberWords(r / 1000000) + " million";
    return w.compare(0, 2, "a ") == 0 ? w : "some " + w;   // "a hundred and ten million", not "some a hundred..."
}
}   // namespace

std::string vgName(const std::string& name) { return std::string(1, VG_NAME_BEGIN) + name + VG_NAME_END; }

// ---------------------------------------------------------------------------
// the format
// ---------------------------------------------------------------------------

std::string VgGrammar::where(int alt) const {
    if (alt < 0 || alt >= (int)alts.size()) return "?";
    const VgAlt& a = alts[alt];
    return (a.file >= 0 && a.file < (int)files.size() ? files[a.file] : std::string("?")) + ":" + std::to_string(a.line);
}

namespace {

struct Parser {
    VgGrammar& G; int file; int line = 0;
    Parser(VgGrammar& g, int f) : G(g), file(f) {}
    void error(const std::string& what) { G.errors.push_back(G.files[file] + ":" + std::to_string(line) + ": " + what); }

    bool parseMark(const std::string& m0, VgNode& n) {
        std::string m = trim(m0);
        if (m.empty()) { error("an empty mark {}"); return false; }
        char c = m[0];
        if (c == '~') {   // H-04: the people's own word for a thing
            n.type = VgNode::TONGUE; n.s = lower(trim(m.substr(1)));
            if (n.s.empty()) { error("a word of theirs needs a key: {~coin}"); return false; }
            G.tongueKeys.insert(n.s);
            return true;
        }
        if (c == '@') {
            n.type = VgNode::DIRECT; size_t sp = m.find(' ');
            n.s = m.substr(1, sp == std::string::npos ? std::string::npos : sp - 1); n.a = sp == std::string::npos ? "" : trim(m.substr(sp + 1));
            return true;
        }
        if (c == '=') {
            n.type = VgNode::ATTR; std::string r = m.substr(1); size_t col = r.find(':');
            n.s = col == std::string::npos ? r : r.substr(0, col); n.a = col == std::string::npos ? "1" : r.substr(col + 1);
            if (n.s.size() < 2 || n.s[0] != '.') { error("an attribute must start with a dot: {" + m + "}"); return false; }
            return true;
        }
        if (c == '#') {
            n.type = VgNode::NUM; std::string r = m.substr(1);
            size_t bar = r.find('|'); if (bar != std::string::npos) { n.b = trim(r.substr(bar + 1)); r = r.substr(0, bar); }
            size_t col = r.find(':'); if (col != std::string::npos) { n.a = r.substr(col + 1); r = r.substr(0, col); }
            if (!n.a.empty() && n.a != "ord" && n.a != "about" && n.a != "digits" && n.a != "words") { error("a number's form is ord, about, digits or words: {" + m + "}"); return false; }
            if (r.empty()) { error("a number without a key: {" + m + "}"); return false; }
            if (r[0] == '$') { size_t d = r.find('.'); n.s = r.substr(0, d); std::string rest = d == std::string::npos ? "" : r.substr(d + 1); while (!rest.empty()) { size_t e = rest.find('.'); n.path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); } }
            else n.s = r;
            return true;
        }
        if (c == '+') {
            n.type = VgNode::DECL; std::string r = m.substr(1);
            size_t bar = r.find('|'); if (bar != std::string::npos) { n.b = r.substr(bar + 1); r = r.substr(0, bar); }
            size_t col = r.find(':'); n.s = r.substr(0, col); n.a = col == std::string::npos ? "?" : r.substr(col + 1);
            if (n.s.size() < 2 || n.s[0] != '$' || (n.a != "he" && n.a != "she" && n.a != "they" && n.a != "it" && n.a != "?")) { error("a person brought in is {+$x:he|she|they|it|?}: {" + m + "}"); return false; }
            if (std::find(G.localEntities.begin(), G.localEntities.end(), n.s) == G.localEntities.end()) G.localEntities.push_back(n.s);
            return true;
        }
        if (c == '$') {
            n.type = VgNode::ENT; std::string r = m; size_t col = r.find(':');
            if (col != std::string::npos) { n.a = r.substr(col + 1); r = r.substr(0, col); }
            static const char* const FORMS[] = {"", "obj", "poss", "refl", "name", "full", "voc", "the", nullptr};
            bool ok = false; for (int i = 0; FORMS[i]; i++) if (n.a == FORMS[i]) ok = true;
            if (!ok) { error("an entity's form is obj, poss, refl, name, full, voc or the: {" + m + "}"); return false; }
            size_t d = r.find('.'); n.s = r.substr(0, d);
            std::string rest = d == std::string::npos ? "" : r.substr(d + 1);
            while (!rest.empty()) { size_t e = rest.find('.'); n.path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); }
            return true;
        }
        if (c == '.') {
            n.type = VgNode::LOCAL; size_t col = m.find(':');
            n.s = m.substr(0, col); n.a = col == std::string::npos ? "" : m.substr(col + 1);
            if (!n.a.empty() && n.a != "obj" && n.a != "subj") { error("a kept word's form is subj or obj: {" + m + "}"); return false; }
            return true;
        }
        size_t gt = m.find('>');
        if (gt != std::string::npos) {
            n.type = VgNode::RULE; n.s = trim(m.substr(0, gt)); n.b = trim(m.substr(gt + 1));
            if (n.b.size() < 2 || n.b[0] != '.') { error("a word is kept as a dotted name: {" + m + "}"); return false; }
            return true;
        }
        size_t col = m.find(':');
        if (col != std::string::npos) {
            std::string l = trim(m.substr(0, col)), r = trim(m.substr(col + 1));
            if (r == "pl") { n.type = VgNode::PLURAL; n.s = l; return true; }
            static const char* const PRON[] = {"I", "we", "you", "they", "he", "she", "it", nullptr};
            bool pron = false; for (int i = 0; PRON[i]; i++) if (r == PRON[i]) pron = true;
            if (!r.empty() && (r[0] == '$' || r[0] == '.' || pron)) {
                n.type = VgNode::VERB; n.s = l; n.a = r;
                if (r[0] == '$') {   // a subject's path: `{die:$e.a}`
                    size_t d = r.find('.'); n.a = r.substr(0, d);
                    std::string rest = d == std::string::npos ? "" : r.substr(d + 1);
                    while (!rest.empty()) { size_t e = rest.find('.'); n.path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); }
                }
                return true;
            }
            error("unknown mark {" + m + "}"); return false;
        }
        n.type = VgNode::RULE; n.s = m;
        return true;
    }

    // a text into nodes; `inChoice`: stop at '|' and ']' (the choice's own)
    void parseSeq(const std::string& t, size_t& i, bool inChoice, std::vector<VgNode>& out) {
        auto text = [&](const std::string& s) { if (!out.empty() && out.back().type == VgNode::TEXT) out.back().s += s; else { VgNode n; n.s = s; out.push_back(n); } };
        while (i < t.size()) {
            char c = t[i];
            if (inChoice && (c == '|' || c == ']')) return;
            if (c == '\\' && i + 1 < t.size()) {
                if (t[i + 1] == 'n') { VgNode n; n.type = VgNode::PAR; out.push_back(n); }
                else text(std::string(1, t[i + 1]));
                i += 2; continue;
            }
            if (c == '[') {
                i++;
                VgNode n; n.type = VgNode::CHOICE;
                for (;;) {
                    std::vector<VgNode> opt; parseSeq(t, i, true, opt); n.opts.push_back(opt);
                    if (i < t.size() && t[i] == '|') { i++; continue; }
                    if (i < t.size() && t[i] == ']') { i++; break; }
                    error("an unclosed [choice"); return;
                }
                out.push_back(n); continue;
            }
            if (c == '{') {
                size_t j = t.find('}', i);
                if (j == std::string::npos) { error("an unclosed {mark"); i = t.size(); return; }
                std::string inner = t.substr(i + 1, j - i - 1);
                if (inner.find('{') != std::string::npos) { error("a mark inside a mark: {" + inner + "}"); i = j + 1; continue; }
                VgNode n; if (parseMark(inner, n)) out.push_back(n);
                i = j + 1; continue;
            }
            if (c == ']' || c == '}') { error(std::string("a stray '") + c + "'"); i++; continue; }
            size_t j = i; while (j < t.size() && t[j] != '[' && t[j] != '{' && t[j] != '\\' && t[j] != ']' && t[j] != '}' && !(inChoice && t[j] == '|')) j++;
            if (j == i) j++;
            text(t.substr(i, j - i)); i = j;
        }
    }

    bool parseCond(const std::string& tok, VgCond& c) {
        if (startsWith(tok, "!has:")) { c.key = tok.substr(5); c.op = 7; return !c.key.empty(); }
        if (startsWith(tok, "has:")) { c.key = tok.substr(4); c.op = 6; return !c.key.empty(); }
        size_t p = tok.find_first_of("=!<>", 1);
        if (p == std::string::npos || p == 0) return false;
        c.key = tok.substr(0, p);
        std::string rest = tok.substr(p);
        if (startsWith(rest, ">=")) { c.op = 2; c.val = rest.substr(2); }
        else if (startsWith(rest, "<=")) { c.op = 3; c.val = rest.substr(2); }
        else if (startsWith(rest, "!=")) { c.op = 1; c.val = rest.substr(2); }
        else if (rest[0] == '=') { c.op = 0; c.val = rest.substr(1); }
        else if (rest[0] == '>') { c.op = 4; c.val = rest.substr(1); }
        else if (rest[0] == '<') { c.op = 5; c.val = rest.substr(1); }
        else return false;
        c.numeric = numberOf(c.val, c.num);
        if (c.op >= 2 && c.op <= 5 && !c.numeric) return false;   // an order needs a number or an age
        return true;
    }

    void parseLine(const std::string& raw, int& cur) {
        std::string ln = raw;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        std::string t = trim(ln);
        if (t.empty() || t[0] == '#') return;
        if (t == "R\"VG(" || t == ")VG\"") return;   // the raw string's wrapper (a file read from disk)
        if (ln[0] == '@') {
            std::vector<std::string> w; size_t p = 1;
            while (p < ln.size()) { size_t q = ln.find_first_of(" \t", p); if (q == std::string::npos) q = ln.size(); if (q > p) w.push_back(ln.substr(p, q - p)); p = q + 1; }
            if (w.empty()) { error("an empty @ line"); return; }
            if (w[0] == "root") { for (size_t i = 1; i < w.size(); i++) G.roots.push_back(w[i]); return; }
            if (w[0] == "lexicon") {
                if (w.size() < 3) { error("@lexicon <age|fact> words: the age one of prehistoric, medieval, industrial, modern, futuristic, farfuture"); return; }
                if (ageOf(w[1]) < 0) {   // H-04: a fact's words (linked against the schema)
                    for (size_t i = 2; i < w.size(); i++) G.lexFactNames[lower(w[i])] = w[1];
                    return;
                }
                int a = ageOf(w[1]);
                for (size_t i = 2; i < w.size(); i++) { std::string k = lower(w[i]); auto it = G.lexicon.find(k); if (it == G.lexicon.end() || it->second > a) G.lexicon[k] = a; }
                return;
            }
            error("unknown @" + w[0]); return;
        }
        if (ln[0] != ' ' && ln[0] != '\t') {   // a rule
            size_t par = t.find('(');
            std::string name = trim(par == std::string::npos ? t : t.substr(0, par));
            for (char ch : name) if (!std::isalnum((unsigned char)ch) && ch != '.' && ch != '-' && ch != '_') { error("a rule's name is letters, digits, '.', '-' and '_': " + name); cur = -1; return; }
            auto it = G.ruleIndex.find(name);
            if (it != G.ruleIndex.end()) { error("the rule " + name + " again (first at " + G.files[G.rules[it->second].file] + ":" + std::to_string(G.rules[it->second].line) + ")"); cur = -1; return; }
            VgRule r; r.name = name; r.file = file; r.line = line;
            if (par != std::string::npos) {
                size_t e = t.find(')', par);
                if (e == std::string::npos) { error("an unclosed rule option"); }
                else {
                    std::string opts = t.substr(par + 1, e - par - 1); size_t p = 0;
                    while (p < opts.size()) {
                        size_t q = opts.find(' ', p); if (q == std::string::npos) q = opts.size();
                        std::string o = opts.substr(p, q - p); p = q + 1;
                        if (o.empty()) continue;
                        if (o == "pl") r.number = VN_PLURAL; else if (o == "sing") r.number = VN_SING; else if (o == "mass") r.number = VN_MASS; else if (o == "keep") r.keep = true;
                        else if (o == "steady") r.steady = true;
                        else if (o == "formula") r.formula = true;
                        else if (startsWith(o, "people=")) { r.peopleK = std::atoi(o.c_str() + 7); if (r.peopleK < 1) error("people=K, K one or more"); }
                        else if (startsWith(o, "share=")) { r.share = std::atof(o.c_str() + 6); if (r.share <= 0 || r.share > 1) error("share=F, F above 0 and at most 1"); }
                        else if (startsWith(o, "words=")) { size_t dd = o.find(".."); if (dd == std::string::npos) error("words=lo..hi"); else { r.wordsLo = std::atoi(o.substr(6, dd - 6).c_str()); r.wordsHi = std::atoi(o.substr(dd + 2).c_str()); } }
                        else error("unknown rule option " + o);
                    }
                }
            }
            cur = (int)G.rules.size(); G.ruleIndex[name] = cur; G.rules.push_back(r);
            return;
        }
        if (cur < 0) { error("an alternative outside a rule"); return; }
        VgAlt a; a.rule = cur; a.file = file; a.line = line;
        {   // H-04: the alternative's key: its rule's name and its line (a people's dialect reads it; stable while other rules change)
            uint64_t h = 1469598103934665603ULL;
            for (unsigned char ch : G.rules[cur].name + "|" + t) { h ^= ch; h *= 1099511628211ULL; }
            a.key = mix64(h);
        }
        if (t[0] == '(') {
            size_t e = t.find(')');
            if (e == std::string::npos) { error("unclosed options"); return; }
            std::string opts = t.substr(1, e - 1); size_t p = 0;
            while (p < opts.size()) {
                size_t q = opts.find(' ', p); if (q == std::string::npos) q = opts.size();
                std::string o = opts.substr(p, q - p); p = q + 1;
                if (o.empty()) continue;
                if (o == "once") { a.once = true; continue; }
                if (o == "tense=past") { a.tense = VG_PAST; continue; }
                if (o == "tense=present") { a.tense = VG_PRESENT; continue; }
                if (o == "pl") { a.number = VN_PLURAL; continue; }
                if (o == "sing") { a.number = VN_SING; continue; }
                if (o == "mass") { a.number = VN_MASS; continue; }
                if (std::isdigit((unsigned char)o[0]) || (o[0] == '.' && o.size() > 1 && std::isdigit((unsigned char)o[1]))) { a.w = std::atof(o.c_str()); if (a.w <= 0) error("a weight above zero: " + o); continue; }
                VgCond c;
                if (o.find('|') != std::string::npos) {   // an OR of guards
                    size_t s = 0; bool ok = true;
                    while (s <= o.size()) { size_t b = o.find('|', s); if (b == std::string::npos) b = o.size(); VgCond x; if (!parseCond(o.substr(s, b - s), x)) ok = false; else c.alt.push_back(x); s = b + 1; }
                    if (!ok) { error("a guard: " + o); continue; }
                } else if (!parseCond(o, c)) { error("an option or a guard: " + o); continue; }
                a.conds.push_back(c);
            }
            t = trim(t.substr(e + 1));
        }
        size_t i = 0; parseSeq(t, i, false, a.body);
        G.rules[cur].alts.push_back((int)G.alts.size());
        G.alts.push_back(a);
    }
};

}   // namespace

bool vgParse(VgGrammar& G, const std::string& text, const std::string& file) {
    double t0 = nowMs();
    size_t errs = G.errors.size();
    Parser P(G, (int)G.files.size());
    G.files.push_back(file);
    int cur = -1; size_t p = 0;
    while (p <= text.size()) {
        size_t e = text.find('\n', p); if (e == std::string::npos) e = text.size();
        P.line++;
        P.parseLine(text.substr(p, e - p), cur);
        p = e + 1;
    }
    G.parseMs += nowMs() - t0;
    return G.errors.size() == errs;
}

namespace {
struct Linker {
    VgGrammar& G; const VgSchema& S;
    Linker(VgGrammar& g, const VgSchema& s) : G(g), S(s) {}
    std::unordered_map<std::string, int> facts, ents, paths, fields, dirs;
    int file = 0, line = 0;
    void error(const std::string& w) { G.errors.push_back(G.files[file] + ":" + std::to_string(line) + ": " + w); }
    bool localEnt(const std::string& n) const { return std::find(G.localEntities.begin(), G.localEntities.end(), n) != G.localEntities.end(); }
    // `$x` and its path: the schema's entity (or a person a text brings in), its paths, and the last a field when `field` may end it
    bool entityPath(const std::string& head, const std::vector<std::string>& path, int& ref, std::vector<int>& pathRef, int& fieldRef, bool fieldOk) {
        auto it = ents.find(head);
        if (it != ents.end()) ref = it->second; else if (localEnt(head)) ref = -1; else { error("unknown entity " + head); return false; }
        pathRef.clear(); fieldRef = -1;
        for (size_t i = 0; i < path.size(); i++) {
            auto pt = paths.find(path[i]);
            if (pt != paths.end()) { pathRef.push_back(pt->second); continue; }
            if (i + 1 == path.size() && fieldOk) {
                auto ft = fields.find(path[i]);
                if (ft != fields.end()) { fieldRef = ft->second; return true; }
                if (path[i] == "gender") { fieldRef = -2; return true; }
                if (path[i] == "name") { fieldRef = -3; return true; }
                if (path[i] == "person") { fieldRef = -4; return true; }
            }
            error("unknown field ." + path[i] + " of " + head); return false;
        }
        return true;
    }
    void cond(VgCond& c) {
        if (!c.alt.empty()) { for (VgCond& x : c.alt) cond(x); return; }
        if (c.key[0] == '.') { c.kind = 1; return; }
        if (c.key[0] == '$') {
            size_t d = c.key.find('.');
            std::string head = c.key.substr(0, d); std::vector<std::string> path; std::string rest = d == std::string::npos ? "" : c.key.substr(d + 1);
            while (!rest.empty()) { size_t e = rest.find('.'); path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); }
            c.kind = path.empty() ? 2 : 3;
            entityPath(head, path, c.ref, c.pathRef, c.fieldRef, true);
            return;
        }
        auto it = facts.find(c.key);
        if (it == facts.end()) { error("unknown fact '" + c.key + "' in a guard"); return; }
        c.kind = 0; c.ref = it->second;
    }
    void node(VgNode& n) {
        switch (n.type) {
            case VgNode::CHOICE: for (auto& o : n.opts) for (VgNode& x : o) node(x); break;
            case VgNode::RULE: {
                int r = G.rule(n.s);
                if (r >= 0) { n.ref = r; break; }
                auto it = facts.find(n.s);
                if (it != facts.end() && n.b.empty()) { n.type = VgNode::FACT; n.ref = it->second; break; }
                error("unknown {" + n.s + "}: no rule and no fact"); break;
            }
            case VgNode::PLURAL: {
                int r = G.rule(n.s); if (r >= 0) { n.ref = r; break; }
                auto it = facts.find(n.s); if (it != facts.end()) { n.fieldRef = it->second; break; }
                error("unknown {" + n.s + ":pl}"); break;
            }
            case VgNode::ENT: entityPath(n.s, n.path, n.ref, n.pathRef, n.fieldRef, true); break;
            case VgNode::VERB:
                if (n.a[0] == '$') entityPath(n.a, n.path, n.ref, n.pathRef, n.fieldRef, false);
                break;
            case VgNode::NUM: {
                if (n.s[0] == '$') { entityPath(n.s, n.path, n.ref, n.pathRef, n.fieldRef, true); if (n.fieldRef == -1) error("a number from an entity needs a field: {#" + n.s + "}"); break; }
                if (n.s[0] == '.') break;
                int r = G.rule(n.s); if (r >= 0) { n.ref = r; n.fieldRef = -1; break; }
                auto it = facts.find(n.s); if (it != facts.end()) { n.ref = -1; n.fieldRef = it->second; break; }
                error("unknown {#" + n.s + "}"); break;
            }
            case VgNode::DIRECT: { auto it = dirs.find(n.s); if (it == dirs.end()) error("unknown directive @" + n.s); else n.ref = it->second; break; }
            default: break;
        }
    }
    void reach(int r) {
        if (r < 0 || G.rules[r].used) return;
        G.rules[r].used = true;
        std::vector<int> stack = {r};
        while (!stack.empty()) {
            int x = stack.back(); stack.pop_back();
            for (int ai : G.rules[x].alts) walk(G.alts[ai].body, stack);
        }
    }
    void walk(const std::vector<VgNode>& body, std::vector<int>& stack) {
        for (const VgNode& n : body) {
            if (n.type == VgNode::CHOICE) for (const auto& o : n.opts) walk(o, stack);
            int r = -1;
            if (n.type == VgNode::RULE || n.type == VgNode::PLURAL) r = n.ref;
            if (n.type == VgNode::NUM && n.s[0] != '$' && n.s[0] != '.' && n.fieldRef < 0) r = n.ref;
            if (r >= 0 && !G.rules[r].used) { G.rules[r].used = true; stack.push_back(r); }
        }
    }
    int numberOfRule(int r) const {   // a list's number when all its words have one (a rule's own, else its alternatives' alike)
        const VgRule& R = G.rules[r];
        if (R.number != VN_NONE) return R.number;
        int n = -1;
        for (int ai : R.alts) { int x = G.alts[ai].number; if (n < 0) n = x; else if (n != x) return VN_NONE; }
        return n < 0 ? VN_NONE : n;
    }
    // "a {goats}": an article written before a plural or a mass list
    void articles(const std::vector<VgNode>& body) {
        for (size_t i = 0; i < body.size(); i++) {
            const VgNode& n = body[i];
            if (n.type == VgNode::CHOICE) for (const auto& o : n.opts) articles(o);
            if (n.type != VgNode::TEXT || i + 1 >= body.size() || body[i + 1].type != VgNode::RULE || body[i + 1].ref < 0) continue;
            std::string t = lower(n.s);
            bool art = endsWith(t, " a ") || endsWith(t, " an ") || t == "a " || t == "an ";
            int num = numberOfRule(body[i + 1].ref);
            if (art && (num == VN_PLURAL || num == VN_MASS)) error("\"a\" before {" + body[i + 1].s + "}, a " + (num == VN_PLURAL ? "plural" : "mass") + " list");
        }
    }
};
}   // namespace

bool vgLink(VgGrammar& G, const VgSchema& S) {
    size_t errs = G.errors.size();
    Linker L(G, S);
    for (size_t i = 0; i < S.facts.size(); i++) L.facts[S.facts[i]] = (int)i;
    for (size_t i = 0; i < S.entities.size(); i++) L.ents[S.entities[i]] = (int)i;
    for (size_t i = 0; i < S.paths.size(); i++) L.paths[S.paths[i]] = (int)i;
    for (size_t i = 0; i < S.fields.size(); i++) L.fields[S.fields[i]] = (int)i;
    for (size_t i = 0; i < S.directives.size(); i++) L.dirs[S.directives[i]] = (int)i;
    G.schema = &S;
    for (VgRule& r : G.rules) r.used = false;
    for (VgAlt& a : G.alts) {
        L.file = a.file; L.line = a.line;
        for (VgCond& c : a.conds) L.cond(c);
        for (VgNode& n : a.body) L.node(n);
    }
    for (VgAlt& a : G.alts) { L.file = a.file; L.line = a.line; L.articles(a.body); }
    G.lexFact.clear();   // H-04: the words a fact allows
    for (const auto& kv : G.lexFactNames) {
        auto it = L.facts.find(kv.second);
        if (it == L.facts.end()) G.errors.push_back("@lexicon " + kv.second + ": no such fact (for '" + kv.first + "')");
        else G.lexFact[kv.first] = it->second;
    }
    // the roots (`@root name`, `@root prefix.*`) and what they reach
    for (const std::string& root : G.roots) {
        if (endsWith(root, ".*")) {
            std::string pre = root.substr(0, root.size() - 1); bool any = false;
            for (int r = 0; r < (int)G.rules.size(); r++) if (startsWith(G.rules[r].name, pre.c_str())) { L.reach(r); any = true; }
            if (!any) G.errors.push_back("@root " + root + ": no rule");
            continue;
        }
        int r = G.rule(root);
        if (r < 0) G.errors.push_back("@root " + root + ": no rule"); else L.reach(r);
    }
    for (const VgRule& r : G.rules) {
        if (!r.used) G.errors.push_back(G.files[r.file] + ":" + std::to_string(r.line) + ": the rule " + r.name + " is reached from no root");
        if (r.alts.empty()) G.errors.push_back(G.files[r.file] + ":" + std::to_string(r.line) + ": the rule " + r.name + " has no alternative");
    }
    // the plural words of the plural lists (the check: no "a" or "one" before them)
    G.plurals.clear();
    static const char* const IRR[] = {"geese", "cattle", "children", "men", "women", "feet", "teeth", "mice", "oxen", "people", nullptr};
    for (int i = 0; IRR[i]; i++) G.plurals.insert(IRR[i]);
    for (const VgRule& r : G.rules) for (int ai : r.alts) {
        const VgAlt& a = G.alts[ai];
        if ((a.number != VN_NONE ? a.number : r.number) != VN_PLURAL || a.body.size() != 1 || a.body[0].type != VgNode::TEXT) continue;
        std::string t = lower(trim(a.body[0].s)); size_t sp = t.rfind(' '); std::string w = sp == std::string::npos ? t : t.substr(sp + 1);
        if (w.size() > 2 && w.back() == 's' && !endsWith(w, "ss")) G.plurals.insert(w);
    }
    // H-04's second pass: which rules may be drawn again (no directive under them) and which may end a sentence; the distinctive words
    {
        const int nr = (int)G.rules.size();
        std::vector<std::vector<int>> refs(nr);
        std::vector<char> direct(nr, 0), stop(nr, 0);
        std::function<void(const std::vector<VgNode>&, int)> scan = [&](const std::vector<VgNode>& body, int r) {
            for (const VgNode& n : body) {
                if (n.type == VgNode::CHOICE) for (const auto& o : n.opts) scan(o, r);
                else if (n.type == VgNode::DIRECT) direct[r] = 1;
                else if (n.type == VgNode::PAR) stop[r] = 1;
                else if (n.type == VgNode::TEXT) { if (n.s.find_first_of(".?!") != std::string::npos) stop[r] = 1; }
                else if ((n.type == VgNode::RULE || n.type == VgNode::PLURAL) && n.ref >= 0) refs[r].push_back(n.ref);
                else if (n.type == VgNode::NUM && n.s[0] != '$' && n.s[0] != '.' && n.fieldRef < 0 && n.ref >= 0) refs[r].push_back(n.ref);
            }
        };
        for (int r = 0; r < nr; r++) for (int ai : G.rules[r].alts) scan(G.alts[ai].body, r);
        for (int r = 0; r < nr; r++) { G.rules[r].pure = !direct[r]; G.rules[r].stops = stop[r] != 0; }
        for (bool changed = true; changed;) {
            changed = false;
            for (int r = 0; r < nr; r++) for (int x : refs[r]) {
                if (G.rules[r].pure && !G.rules[x].pure) { G.rules[r].pure = false; changed = true; }
                if (!G.rules[r].stops && G.rules[x].stops) { G.rules[r].stops = true; changed = true; }
            }
        }
        // the words of each alternative's own text (always written when it is drawn), four letters or more, in few alternatives
        static const std::set<std::string> COMMON = {"that", "this", "with", "from", "have", "were", "they", "their", "there", "them", "then", "than", "what",
            "when", "where", "which", "will", "would", "your", "yours", "been", "into", "only", "over", "some", "more", "most", "much", "very", "just", "like",
            "each", "every", "other", "after", "before", "about", "again", "still", "even", "also", "here", "said", "says", "know", "knew", "come", "came",
            "went", "goes", "gone", "made", "make", "take", "took", "give", "gave", "once", "ever", "never", "nothing", "something", "everyone", "nobody",
            "anyone", "someone", "these", "those", "whose", "until", "while", "because", "though", "through", "upon", "under", "back", "down", "well",
            "year", "years", "days", "time", "first", "last", "long", "good", "great", "little", "many", "left", "right", "same", "such", "both", "keep"};
        std::unordered_map<std::string, int> df;
        std::vector<std::vector<std::string>> altWords(G.alts.size());
        for (size_t ai = 0; ai < G.alts.size(); ai++) {
            std::set<std::string> ws;
            for (const VgNode& n : G.alts[ai].body) {
                if (n.type != VgNode::TEXT) continue;
                std::string w;
                for (size_t i = 0; i <= n.s.size(); i++) {
                    char c = i < n.s.size() ? n.s[i] : ' ';
                    if (std::isalpha((unsigned char)c)) { w += (char)std::tolower((unsigned char)c); continue; }
                    if (w.size() >= 4 && !COMMON.count(w)) ws.insert(w);
                    w.clear();
                }
            }
            for (const std::string& w : ws) { df[w]++; altWords[ai].push_back(w); }
        }
        G.words.clear();
        std::unordered_map<std::string, int> idx;
        for (size_t ai = 0; ai < G.alts.size(); ai++) {
            G.alts[ai].words.clear();
            for (const std::string& w : altWords[ai]) {
                if (df[w] > VG_DISTINCT) continue;
                auto it = idx.find(w);
                int id = it != idx.end() ? it->second : (int)G.words.size();
                if (it == idx.end()) { idx[w] = id; G.words.push_back(w); }
                G.alts[ai].words.push_back(id);
            }
        }
    }
    return G.errors.size() == errs;
}

// ---------------------------------------------------------------------------
// the expansion and the realiser
// ---------------------------------------------------------------------------

namespace {

enum TokType : uint8_t { T_TEXT = 0, T_PAR, T_ENT, T_VERB, T_MENTION, T_NUMBER, T_NAME, T_FBEGIN, T_FEND, T_WORD };   // T_WORD: a word of theirs (`{~key}`)
// a model's words with their names marked: the text and the names apart (`out` gets the text, `names` each name)
std::string unmarkNames(const std::string& s, std::vector<std::string>* names) {
    if (s.find(VG_NAME_BEGIN) == std::string::npos && s.find(VG_NAME_END) == std::string::npos) return s;
    std::string out; size_t i = 0;
    while (i < s.size()) {
        if (s[i] == VG_NAME_BEGIN) {
            size_t e = s.find(VG_NAME_END, i + 1); if (e == std::string::npos) e = s.size();
            std::string n = s.substr(i + 1, e - i - 1);
            out += n; if (names && !n.empty()) names->push_back(n);
            i = e + 1; continue;
        }
        if (s[i] != VG_NAME_END) out += s[i];
        i++;
    }
    return out;
}
enum Form : uint8_t { F_SUBJ = 0, F_OBJ, F_POSS, F_REFL, F_NAME, F_FULL, F_VOC, F_THE };
struct Tok {
    uint8_t t = T_TEXT, form = F_SUBJ, number = VN_NONE;
    int8_t tense = VG_PRESENT, person = 3;
    bool plural = false;
    int ent = -1;
    std::string s;
};
uint8_t formOf(const std::string& f) {
    if (f == "obj") return F_OBJ; if (f == "poss") return F_POSS; if (f == "refl") return F_REFL; if (f == "name") return F_NAME;
    if (f == "full") return F_FULL; if (f == "voc") return F_VOC; if (f == "the") return F_THE; return F_SUBJ;
}

struct Local { std::string name, value; int number = VN_NONE; };
struct Scope { std::vector<std::pair<std::string, int>> ents; int tense = VG_PRESENT; };

struct Expander : VgRun {
    const VgGrammar& G; VgModel& M; VgMemory* mem; VgRecord& R;
    Rng rng;
    std::vector<Tok> out;
    std::vector<Local> locals;                         // the record's attributes and kept words, in reading order
    std::vector<std::pair<std::string, int>> declared; // the persons the texts brought in
    struct Kept { std::string text; int number = VN_NONE; std::vector<Tok> toks; };
    std::unordered_map<int, Kept> kept;   // a `keep` rule's draw: its words, and its tokens (H-04's second pass: a person in it is still that person)
    std::vector<int> usedHere;
    std::vector<int> onceUsed;
    std::vector<int> candIds; std::vector<double> candW;
    Scope* cur = nullptr;
    int depth = 0, rootRule = -1;
    Expander(const VgGrammar& g, VgModel& m, VgMemory* me, VgRecord& r) : G(g), M(m), mem(me), R(r), rng(r.seed) {}

    size_t mergeFloor = 0;   // a capture's start: text written after it never joins a token before it
    void text(const std::string& s) { if (s.empty()) return; if (!out.empty() && out.back().t == T_TEXT && out.size() > mergeFloor) out.back().s += s; else { Tok k; k.s = s; out.push_back(k); } }
    size_t capture() { size_t f = mergeFloor; mergeFloor = out.size(); return f; }
    void note(const std::string& w) override { R.diag.push_back(w); }
    double uni() override { return rng.uni(); }
    void paragraph() override { Tok k; k.t = T_PAR; out.push_back(k); }
    void bindEntity(const std::string& name, int entity) override { if (cur) cur->ents.push_back({name, entity}); }
    bool expand(const std::string& rule) override { int r = G.rule(rule); if (r < 0) { note("no rule " + rule); return false; } Scope s = cur ? *cur : Scope(); return expandRule(r, s, std::string()); }
    bool has(const std::string& rule) const override { return G.rule(rule) >= 0; }
    // H-04: a model's words, the names in them as names
    void marked(const std::string& s) {
        if (s.find(VG_NAME_BEGIN) == std::string::npos) { text(unmarkNames(s, nullptr)); return; }
        size_t i = 0;
        while (i < s.size()) {
            size_t b = s.find(VG_NAME_BEGIN, i);
            if (b == std::string::npos) { text(unmarkNames(s.substr(i), nullptr)); break; }
            if (b > i) text(s.substr(i, b - i));
            size_t e = s.find(VG_NAME_END, b + 1); if (e == std::string::npos) e = s.size();
            if (e > b + 1) {   // a name; a word of theirs kept and written again is one in lowercase (the history's names are capitalised)
                Tok t; t.t = T_NAME; t.s = s.substr(b + 1, e - b - 1);
                if (std::islower((unsigned char)t.s[0])) t.t = T_WORD;
                out.push_back(t);
            }
            i = e + 1;
        }
    }

    Local* local(const std::string& n) { for (size_t i = locals.size(); i-- > 0;) if (locals[i].name == n) return &locals[i]; return nullptr; }
    void setLocal(const std::string& n, const std::string& v, int number) { Local* l = local(n); if (l) { l->value = v; l->number = number; } else locals.push_back({n, v, number}); }

    int entityHead(int ref, const std::string& name, const Scope& sc) {
        for (size_t i = sc.ents.size(); i-- > 0;) if (sc.ents[i].first == name) return sc.ents[i].second;
        for (size_t i = declared.size(); i-- > 0;) if (declared[i].first == name) return declared[i].second;
        if (ref >= 0 && ref < (int)M.bound.size()) { if (M.bound[ref] < 0) M.bound[ref] = M.lazy(ref); return M.bound[ref]; }   // H-04: bound when first named
        return -1;
    }
    int entityAt(int ref, const std::string& name, const std::vector<int>& pathRef, const Scope& sc) {
        int e = entityHead(ref, name, sc);
        for (int p : pathRef) { if (e < 0) return -1; e = M.follow(e, p); }
        return e;
    }
    bool fieldOf(int e, int fieldRef, std::string& out) {
        if (e < 0 || e >= (int)M.entities.size()) return false;
        const VgEntity& x = M.entities[e];
        if (fieldRef == -2) { static const char* const G4[] = {"he", "she", "they", "it"}; out = G4[x.gender & 3]; return true; }
        if (fieldRef == -3) { out = x.name; return !out.empty(); }
        if (fieldRef == -4) { out = std::to_string(x.person); return true; }
        return M.field(e, fieldRef, out);
    }
    bool valueOf(const VgCond& c, const Scope& sc, std::string& v) {
        switch (c.kind) {
            case 0: if (c.ref < 0 || c.ref >= (int)M.facts.size()) return false; v = M.facts[c.ref]; return !v.empty();
            case 1: { Local* l = local(c.key); if (!l) return false; v = l->value; return true; }
            case 2: { size_t d = c.key.find('.'); int e = entityHead(c.ref, c.key.substr(0, d), sc); if (e < 0) return false; v = "1"; return true; }
            default: {
                size_t d = c.key.find('.'); int e = entityAt(c.ref, c.key.substr(0, d), c.pathRef, sc);
                if (e < 0) return false;
                if (c.fieldRef == -1) { v = "1"; return true; }   // a path's end: the entity is there
                return fieldOf(e, c.fieldRef, v);
            }
        }
    }
    bool holds(const VgCond& c, const Scope& sc) {
        if (!c.alt.empty()) { for (const VgCond& x : c.alt) if (holds(x, sc)) return true; return false; }
        std::string v; bool have = valueOf(c, sc, v);
        if (c.op == 6) return have && !v.empty() && v != "0";
        if (c.op == 7) return !(have && !v.empty() && v != "0");
        if (!have) return c.op == 1;
        double a;
        if (c.numeric && numberOf(v, a)) {
            switch (c.op) { case 0: return a == c.num; case 1: return a != c.num; case 2: return a >= c.num; case 3: return a <= c.num; case 4: return a > c.num; default: return a < c.num; }
        }
        if (c.op == 0) return v == c.val;
        if (c.op == 1) return v != c.val;
        return false;
    }

    std::string textFrom(size_t start) const {   // the names kept marked (a kept word written again is still a name)
        std::string s;
        for (size_t i = start; i < out.size(); i++) { if (out[i].t == T_TEXT) s += out[i].s; else if (out[i].t == T_NAME || out[i].t == T_WORD) s += vgName(out[i].s); }
        return trim(s);
    }

    // ---- H-04's second pass: the undo journal, a rule drawn again, the frames of the sentences written ----
    struct Undo { uint8_t kind; int a; uint64_t h; };   // 0 an alternative's use, 1 a word's use, 2 a frame kept, 3 a kept rule's draw, 4 a repeat let stand
    std::unordered_set<uint64_t> standing;              // the repeats an inner rule let stand (its draws spent): an outer rule does not draw again for them (writer F's)
    std::vector<Undo> journal;
    int budget = VG_RETRY_BUDGET;
    struct Snap { size_t out, lastLen, used, here, once, declared, diag, journal, mergeFloor; int root, lo, hi; std::vector<Local> locals; };
    Snap snapshot() const {   // `lastLen`: the text the draw may join to the last token (`text` merges into it)
        Snap s; s.out = out.size(); s.lastLen = out.empty() ? 0 : out.back().s.size(); s.used = R.used.size(); s.here = usedHere.size(); s.once = onceUsed.size(); s.declared = declared.size();
        s.diag = R.diag.size(); s.journal = journal.size(); s.mergeFloor = mergeFloor; s.root = R.root; s.lo = R.lo; s.hi = R.hi; s.locals = locals;
        return s;
    }
    void restore(const Snap& s) {
        out.resize(s.out); if (!out.empty() && out.back().t == T_TEXT) out.back().s.resize(s.lastLen);
        R.used.resize(s.used); usedHere.resize(s.here); onceUsed.resize(s.once); declared.resize(s.declared); R.diag.resize(s.diag);
        mergeFloor = s.mergeFloor; R.root = s.root; R.lo = s.lo; R.hi = s.hi; locals = s.locals;
        while (journal.size() > s.journal) {
            const Undo u = journal.back(); journal.pop_back();
            switch (u.kind) {
                case 0: if (mem && u.a < (int)mem->uses.size()) mem->uses[u.a]--; break;
                case 1: if (mem && u.a < (int)mem->wordUses.size()) mem->wordUses[u.a]--; break;
                case 2: if (mem) mem->frames.erase(u.h); break;
                case 3: kept.erase(u.a); break;
                case 4: standing.erase(u.h); break;
            }
        }
    }
    double dialect(const VgAlt& a, bool steady) {   // the people's leaning to an alternative (1 with no people)
        if (!M.culture) return 1;
        const int ai = (int)(&a - G.alts.data());
        if (mem && ai >= 0 && ai < (int)mem->dialect.size() && mem->dialect[ai] > 0) return steady ? std::pow((double)mem->dialect[ai], VG_DIALECT_STEADY / VG_DIALECT) : mem->dialect[ai];
        uint64_t h = mix64(M.culture ^ a.key ^ 0xD1A1EC7ULL);
        double z = 0; for (int i = 0; i < 4; i++) { z += (double)(h & 0xFFFF) / 65535.0; h >>= 16; }
        z = (z - 2.0) * 1.7320508;   // four uniforms: a normal near enough
        const double w = std::exp(VG_DIALECT * z);
        if (mem && ai >= 0 && ai < (int)mem->dialect.size()) mem->dialect[ai] = (float)w;
        return steady ? std::pow(w, VG_DIALECT_STEADY / VG_DIALECT) : w;   // the root's sigma (exp(VG_DIALECT_STEADY z))
    }
    static bool numberWordF(const std::string& w) {
        static const std::set<std::string> NUMW = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "twenty", "thirty", "forty",
                                                   "fifty", "sixty", "hundred", "thousand", "first", "second", "third", "fourth", "fifth", "no"};
        return NUMW.count(w) > 0;
    }
    // the sentences that end within [from, out.size()): each one's words from its start (which may lie before `from`), the names, the
    // persons, the digits and the number words masked (the measures' frame, near enough); one of six words or more on a frame the people
    // wrote before this draw (kept before `seq0`) is a repeat; the new ones are kept when `insert`. The genres' formulas are left out
    std::vector<uint64_t> lastRepeats;   // the frames the last check found repeated
    int frames(size_t from, int seq0, bool insert) {
        if (!mem) return 0;
        lastRepeats.clear();
        size_t i = from, c0 = 0;
        for (; i > 0; i--) {
            const Tok& k = out[i - 1];
            if (k.t == T_PAR) break;
            if (k.t == T_TEXT) { size_t p = k.s.find_last_of(".?!"); if (p != std::string::npos) { i--; c0 = p + 1; break; } }
        }
        int fdepth = 0;
        for (size_t j = 0; j < i; j++) { if (out[j].t == T_FBEGIN) fdepth++; else if (out[j].t == T_FEND) fdepth--; }
        std::string sig; int words = 0, repeats = 0; bool inFormula = fdepth > 0;
        auto word = [&](const std::string& w) { if (!sig.empty()) sig += ' '; sig += w; words++; };
        auto finish = [&]() {
            if (words >= 6 && !inFormula) {
                uint64_t h = 1469598103934665603ULL; for (unsigned char ch : sig) { h ^= ch; h *= 1099511628211ULL; }
                auto it = mem->frames.find(h);
                if (it != mem->frames.end()) { if (it->second < seq0 && !standing.count(h)) { repeats++; lastRepeats.push_back(h); } }
                else if (insert) { mem->frames[h] = mem->frameSeq++; journal.push_back({2, 0, h}); }
            }
            sig.clear(); words = 0; inFormula = fdepth > 0;
        };
        for (size_t j = i; j < out.size(); j++) {
            const Tok& k = out[j];
            switch (k.t) {
                case T_TEXT: {
                    std::string w;
                    for (size_t c = (j == i ? c0 : 0); c <= k.s.size(); c++) {
                        const char ch = c < k.s.size() ? k.s[c] : ' ';
                        if (std::isalnum((unsigned char)ch) || ch == '\'') { w += ch; continue; }
                        if (!w.empty()) {
                            if (std::isdigit((unsigned char)w[0])) word("#");
                            else if (std::isupper((unsigned char)w[0]) && w != "I") word("N");
                            else { std::string l = lower(w); word(numberWordF(l) ? "#" : l); }
                            w.clear();
                        }
                        if (ch == '.' || ch == '?' || ch == '!') finish();
                    }
                    break;
                }
                case T_PAR: finish(); break;
                case T_NAME: word("N"); break;
                case T_WORD: word(lower(k.s)); break;
                case T_ENT: { const int p = k.ent >= 0 && k.ent < (int)M.entities.size() ? M.entities[k.ent].person : 3; word(p == 1 ? "I" : (p == 2 ? "you" : "E")); break; }
                case T_VERB: word(k.s); break;
                case T_FBEGIN: fdepth++; inFormula = true; break;
                case T_FEND: fdepth--; if (words > 0) inFormula = true; break;   // the sentence after a formula that ended at a stop is checked (writer A's find)
                default: break;
            }
        }
        return repeats;
    }

    bool expandRule(int ri, Scope& sc, const std::string& bindLocal) {
        if (++depth > 48) { note("too deep at " + G.rules[ri].name); depth--; return false; }
        const VgRule& rule = G.rules[ri];
        if (rule.keep) {
            auto k = kept.find(ri);
            if (k != kept.end()) {   // written again as it was drawn (its persons, verbs and names as tokens)
                for (const Tok& t : k->second.toks) if (t.t != T_FBEGIN && t.t != T_FEND) out.push_back(t);
                if (!bindLocal.empty()) setLocal(bindLocal, k->second.text, k->second.number);
                depth--; return true;
            }
        }
        // H-04's second pass: a rule that writes a whole sentence and asks nothing of the model is drawn again while a sentence it ended
        // is on a frame the people wrote before (the draws and their marks undone; the last draw stands)
        const bool redraw = mem && rule.pure && rule.stops && !rule.formula && !rule.keep && !rule.steady;
        if (!redraw) { const bool ok = drawRule(ri, sc, bindLocal, nullptr, nullptr); depth--; return ok; }
        const Snap snap = snapshot();
        std::vector<int> tried;
        bool ok = false;
        for (int attempt = 0;; attempt++) {
            // the draw opens a token of its own (its text never joins the one before), so the check finds where its sentences began (writer F's
            // find: a sentence merged into the previous token was never checked)
            const size_t floorSave = mergeFloor;
            mergeFloor = out.size();
            const size_t start = out.size(); const int seq0 = mem->frameSeq;
            int drawn = -1;
            ok = drawRule(ri, sc, bindLocal, &tried, &drawn);
            mergeFloor = floorSave;
            if (!ok) break;
            const int rep = frames(start, seq0, true);
            if (rep == 0) break;
            if (attempt + 1 >= VG_RETRIES || budget <= 0) {   // let stand: the rules round it do not draw again for it
                mem->kept += rep;
                for (uint64_t h : lastRepeats) if (standing.insert(h).second) journal.push_back({4, 0, h});
                break;
            }
            budget--; R.retries++; mem->retries++;
            if (drawn >= 0) tried.push_back(drawn);
            restore(snap);
        }
        depth--;
        return ok;
    }
    // a rule's draw: its candidates by weight (the guards, the once, the people's dialect and choice, the catalogue's dryness, the words worn
    // out, an alternative this call tried already), the fresh ones first, the alternative expanded
    bool drawRule(int ri, Scope& sc, const std::string& bindLocal, const std::vector<int>* tried, int* drawn) {
        const VgRule& rule = G.rules[ri];
        // the candidates (scratch shared by the recursion: the draw is made before the alternative is expanded)
        std::vector<int>& ids = candIds; std::vector<double>& w = candW;
        ids.clear(); w.clear();
        const bool many = rule.alts.size() > 1;
        for (int ai : rule.alts) {
            const VgAlt& a = G.alts[ai];
            if (a.once && std::find(onceUsed.begin(), onceUsed.end(), ai) != onceUsed.end()) continue;
            bool ok = true; for (const VgCond& c : a.conds) if (!holds(c, sc)) { ok = false; break; }
            if (!ok) continue;
            double wt = a.w;
            if (many && rule.steady) wt *= dialect(a, true);   // the people's own lean among the genres (their prayers, their letters)
            if (mem && !rule.steady && ai < (int)mem->uses.size()) wt /= 1.0 + VG_DRYNESS * mem->uses[ai];
            if (mem && many && !a.words.empty()) { int mx = 0; for (int wd : a.words) if (wd < (int)mem->wordUses.size()) mx = std::max(mx, mem->wordUses[wd]); wt /= 1.0 + VG_FATIGUE * mx; }
            if (tried && std::find(tried->begin(), tried->end(), ai) != tried->end()) wt *= 0.1;
            ids.push_back(ai); w.push_back(wt);
        }
        int n = (int)ids.size();
        if (n == 0) { note("no alternative of " + rule.name); return false; }
        if (M.culture && (rule.peopleK > 0 || rule.share > 0) && n > 1) {   // the people's own few: the seed's first K of those that hold
            const int keepN = rule.peopleK > 0 ? rule.peopleK : std::max(1, (int)std::lround(rule.share * n));
            if (keepN < n) {
                std::vector<std::pair<uint64_t, int>> rank(n);
                for (int i = 0; i < n; i++) rank[i] = {mix64(M.culture ^ G.alts[ids[i]].key ^ 0x9E0F1EULL), i};
                std::sort(rank.begin(), rank.end());
                std::vector<int> ki(keepN); std::vector<double> kw(keepN);
                for (int i = 0; i < keepN; i++) { ki[i] = ids[rank[i].second]; kw[i] = w[rank[i].second]; }
                std::copy(ki.begin(), ki.end(), ids.begin()); std::copy(kw.begin(), kw.end(), w.begin());
                ids.resize(keepN); w.resize(keepN); n = keepN;
            }
        }
        {   // the alternatives this record has not used yet, while there are any (compacted in place; none fresh leaves all)
            int m = 0;
            for (int i = 0; i < n; i++) if (std::find(usedHere.begin(), usedHere.end(), ids[i]) == usedHere.end()) { ids[m] = ids[i]; w[m] = w[i]; m++; }
            if (m > 0) { ids.resize(m); w.resize(m); n = m; }
        }
        int k = rng.pick(w.data(), n), ai = ids[k];
        if (drawn) *drawn = ai;
        const VgAlt& a = G.alts[ai];
        usedHere.push_back(ai); R.used.push_back(ai);
        if (a.once) onceUsed.push_back(ai);
        if (mem && !rule.steady && many && ai < (int)mem->uses.size()) { mem->uses[ai]++; journal.push_back({0, ai, 0}); }
        if (mem && many) for (int wd : a.words) if (wd < (int)mem->wordUses.size()) { mem->wordUses[wd]++; journal.push_back({1, wd, 0}); }
        if (ri == rootRule && R.root < 0) R.root = (int)(std::find(rule.alts.begin(), rule.alts.end(), ai) - rule.alts.begin());
        if (rule.wordsHi > 0 && R.hi == 0) { R.lo = rule.wordsLo; R.hi = rule.wordsHi; }
        int number = a.number != VN_NONE ? a.number : rule.number;
        size_t start = out.size();
        const bool capturing = rule.keep || !bindLocal.empty();
        size_t floor0 = capturing ? capture() : mergeFloor;
        if (number != VN_NONE) { Tok t; t.t = T_NUMBER; t.number = (uint8_t)number; out.push_back(t); }
        if (rule.formula) { Tok t; t.t = T_FBEGIN; out.push_back(t); }
        Scope child = sc; if (a.tense >= 0) child.tense = a.tense;
        Scope* prev = cur; cur = &child;
        run(a.body, child);
        cur = prev;
        if (rule.formula) { Tok t; t.t = T_FEND; out.push_back(t); }
        mergeFloor = floor0;
        if (capturing) {
            std::string t = textFrom(start);
            if (rule.keep) { Kept K; K.text = t; K.number = number; K.toks.assign(out.begin() + (long)start, out.end()); kept[ri] = K; journal.push_back({3, ri, 0}); }
            if (!bindLocal.empty()) setLocal(bindLocal, t, number);
        }
        return true;
    }

    void run(const std::vector<VgNode>& body, Scope& sc) {
        for (const VgNode& n : body) {
            switch (n.type) {
                case VgNode::TEXT: text(n.s); break;
                case VgNode::PAR: paragraph(); break;
                case VgNode::CHOICE: run(n.opts[rng.irange((int)n.opts.size())], sc); break;
                case VgNode::RULE: if (n.ref >= 0) expandRule(n.ref, sc, n.b); else note("unlinked {" + n.s + "}"); break;
                case VgNode::FACT: {
                    const std::string& v = n.ref >= 0 && n.ref < (int)M.facts.size() ? M.facts[n.ref] : std::string();
                    if (v.empty()) { note("the fact " + n.s + " is not set"); break; }
                    bool name = false;
                    if (G.schema) for (const std::string& nm : G.schema->names) if (nm == n.s) name = true;
                    if (name) { Tok t; t.t = T_NAME; t.s = unmarkNames(v, nullptr); out.push_back(t); } else marked(v);
                    break;
                }
                case VgNode::LOCAL: {
                    Local* l = local(n.s);
                    if (!l) { note("the word " + n.s + " is not kept"); break; }
                    if (n.a.empty()) { if (l->number != VN_NONE) { Tok t; t.t = T_NUMBER; t.number = (uint8_t)l->number; out.push_back(t); } marked(l->value); }
                    else text(l->number == VN_PLURAL ? (n.a == "obj" ? "them" : "they") : "it");
                    break;
                }
                case VgNode::ENT: {
                    int e = entityAt(n.ref, n.s, n.pathRef, sc);
                    if (e < 0) { std::string p = n.s; for (const std::string& x : n.path) p += "." + x; note("unbound " + p); break; }
                    if (n.fieldRef != -1) { std::string v; if (fieldOf(e, n.fieldRef, v)) marked(v); else note("no field of " + n.s); break; }
                    Tok t; t.t = T_ENT; t.ent = e; t.form = formOf(n.a); out.push_back(t);
                    break;
                }
                case VgNode::VERB: {
                    Tok t; t.t = T_VERB; t.s = n.s; t.tense = (int8_t)sc.tense;
                    const std::string& s = n.a;
                    if (s[0] == '$') { t.ent = entityAt(n.ref, s, n.pathRef, sc); if (t.ent < 0) { note("the verb's subject " + s + " is unbound"); t.ent = -1; } }
                    else if (s[0] == '.') { Local* l = local(s); t.plural = l && l->number == VN_PLURAL; if (!l) note("the verb's subject " + s + " is not kept"); }
                    else if (s == "I") t.person = 1; else if (s == "we") { t.person = 1; t.plural = true; } else if (s == "you") t.person = 2; else if (s == "they") t.plural = true;
                    out.push_back(t);
                    break;
                }
                case VgNode::NUM: number(n, sc); break;
                case VgNode::ATTR: setLocal(n.s, n.a, VN_NONE); break;
                case VgNode::DIRECT: {
                    Scope* prev = cur; cur = &sc;
                    if (!M.directive(*this, n.ref, n.a)) note("the directive @" + n.s + " wrote nothing");
                    cur = prev;
                    break;
                }
                case VgNode::DECL: {
                    VgEntity x;
                    x.gender = n.a == "she" ? VG_SHE : (n.a == "they" ? VG_THEY : (n.a == "it" ? VG_IT : (n.a == "he" ? VG_HE : (rng.chance(0.5) ? VG_SHE : VG_HE))));
                    x.later = n.b;
                    x.shown = n.a != "?";
                    M.entities.push_back(x);
                    int id = (int)M.entities.size() - 1;
                    declared.push_back({n.s, id});
                    Tok t; t.t = T_MENTION; t.ent = id; out.push_back(t);
                    break;
                }
                case VgNode::TONGUE: {   // H-04: their own word for a thing
                    std::string wd;
                    if (M.tongue(n.s, wd) && !wd.empty()) { Tok t; t.t = T_WORD; t.s = wd; out.push_back(t); }
                    else note("no word of theirs for {~" + n.s + "}");
                    break;
                }
                case VgNode::PLURAL: {
                    if (n.ref >= 0) { size_t start = out.size(), f = capture(); expandRule(n.ref, sc, std::string()); std::string t = unmarkNames(textFrom(start), nullptr); out.resize(start); mergeFloor = f; text(vgPlural(t)); }
                    else if (n.fieldRef >= 0 && n.fieldRef < (int)M.facts.size()) text(vgPlural(unmarkNames(M.facts[n.fieldRef], nullptr)));
                    break;
                }
            }
        }
    }

    void number(const VgNode& n, Scope& sc) {
        std::string v; bool words = false;   // `words`: the value is written in words already (a list's "twenty"): kept as it is
        if (n.s[0] == '$') { int e = entityAt(n.ref, n.s, n.pathRef, sc); if (e < 0 || !fieldOf(e, n.fieldRef, v)) { note("no number at " + n.s); return; } }
        else if (n.s[0] == '.') { Local* l = local(n.s); if (!l) { note("no number " + n.s); return; } v = l->value; }
        else if (n.fieldRef >= 0) { if (n.fieldRef < (int)M.facts.size()) v = M.facts[n.fieldRef]; }
        else if (n.ref >= 0) { size_t start = out.size(), f = capture(); expandRule(n.ref, sc, std::string()); v = textFrom(start); out.resize(start); mergeFloor = f; }
        v = unmarkNames(v, nullptr);
        long x = countOf(v);
        if (x < 0) { note("not a number: {#" + n.s + "} = '" + v + "'"); return; }
        words = !v.empty() && !std::isdigit((unsigned char)trim(v)[0]);
        std::string s;
        if (n.a == "ord") s = vgOrdinal(x);
        else if (n.a == "about") s = aboutWords(x);
        else if (n.a == "digits") s = std::to_string(x);
        else if (n.a == "words") s = vgNumberWords(x);
        else s = words ? trim(v) : (x <= 12 ? vgNumberWords(x) : std::to_string(x));
        if (!n.b.empty()) s += " " + (x == 1 ? n.b : vgPlural(n.b));
        text(s);
    }
};

// the realiser: the referring expressions, the verbs, the articles' notes; then the seams
struct Realiser {
    const std::vector<VgEntity>& E; bool inscription; VgRecord& R;
    int sentence = 0, order = 0;
    std::vector<int> lastSent, seq;   // each entity's last mention: its sentence, its place in the order of mentions
    std::vector<char> shown;          // the reader knows its gender (its first form said it, or a pronoun has)
    int lastOf[4] = {-1, -1, -1, -1};
    std::string s;
    std::vector<size_t> formulaAt;          // H-04: where an open formula began in `s`
    std::vector<std::string> formulaRaw;    // the formulas' raw words (the record's `formulas` are their sentences' frames)
    Realiser(const std::vector<VgEntity>& e, bool ins, VgRecord& r) : E(e), inscription(ins), R(r), lastSent(e.size(), -1000), seq(e.size(), -1), shown(e.size(), 0) {
        for (size_t i = 0; i < e.size(); i++) shown[i] = e[i].shown;
    }
    // a person mentioned since `id` was whose gender the reader does not know: "Cekrast says they follow him" is not clear when
    // Cekrast is a girl, since nothing has said so
    bool unshownSince(int id) const {
        for (size_t y = 0; y < E.size(); y++) if ((int)y != id && seq[y] > seq[id] && !shown[y] && E[y].person == 3) return true;
        return false;
    }

    static std::string pronoun(int g, uint8_t form) {
        static const char* const SUBJ[] = {"he", "she", "they", "it"}, *const OBJ[] = {"him", "her", "them", "it"}, *const POSS[] = {"his", "her", "their", "its"},
                          *const REFL[] = {"himself", "herself", "themselves", "itself"};
        g &= 3;
        return form == F_OBJ ? OBJ[g] : (form == F_POSS ? POSS[g] : (form == F_REFL ? REFL[g] : SUBJ[g]));
    }
    void mention(int id) { if (id >= 0 && id < (int)lastSent.size()) { lastSent[id] = sentence; seq[id] = ++order; lastOf[E[id].gender & 3] = id; } }
    void name(const std::string& n) { if (!n.empty() && std::find(R.names.begin(), R.names.end(), n) == R.names.end()) R.names.push_back(n); }
    std::string refer(int id, uint8_t form) {
        if (id < 0 || id >= (int)E.size()) return "";
        const VgEntity& e = E[id];
        if (e.person == 1 || e.person == 2) {
            bool me = e.person == 1;
            if (form == F_NAME || form == F_FULL || form == F_VOC) {
                std::string n = form == F_VOC && !e.voc.empty() ? e.voc : e.name;
                std::vector<std::string> marks; n = unmarkNames(n, &marks); for (const std::string& m : marks) name(m);   // H-04: a marked name
                name(unmarkNames(e.name, nullptr));
                return n.empty() ? (me ? "I" : "you") : n;
            }
            if (form == F_OBJ) return me ? "me" : "you";
            if (form == F_POSS) return me ? "my" : "your";
            if (form == F_REFL) return me ? "myself" : "yourself";
            return me ? "I" : "you";
        }
        bool seen = lastSent[id] > -1000;
        bool clear = seen && lastOf[e.gender & 3] == id && sentence - lastSent[id] <= 1 && !unshownSince(id);
        bool bare = e.name.empty() && e.first.empty() && e.later.empty();   // a person a text brought in without a word for them: always a pronoun
        std::string out;
        const std::string& firstForm = !e.first.empty() ? e.first : e.name;
        const std::string& laterForm = !e.later.empty() ? e.later : (!e.name.empty() ? e.name : e.first);
        switch (form) {
            case F_NAME: out = !e.name.empty() ? e.name : firstForm; break;
            case F_FULL: out = firstForm; break;
            case F_VOC: out = !e.voc.empty() ? e.voc : (!e.name.empty() ? e.name : firstForm); break;
            case F_THE: out = laterForm; break;
            case F_REFL: out = pronoun(e.gender, F_REFL); break;
            default:
                if (!e.named && (bare || (seen && clear))) { out = pronoun(e.gender, form); shown[id] = 1; }
                else { out = seen ? laterForm : firstForm; if (form == F_POSS) out += "'s"; }
                break;
        }
        if (out.empty()) { out = pronoun(e.gender, form); shown[id] = 1; }
        if (form == F_REFL) shown[id] = 1;
        if (e.named && form == F_POSS && !seen && out.size() > 2 && out.compare(out.size() - 2, 2, "'s") != 0) out += "'s";
        { std::vector<std::string> marks; out = unmarkNames(out, &marks); for (const std::string& m : marks) name(m); }   // H-04: names within a form
        if (!e.name.empty() && out.find(e.name) != std::string::npos) name(e.name);
        mention(id);
        return out;
    }
    std::string lastWord() const {   // the last word written (lowercase)
        size_t e = s.size();
        while (e > 0 && !std::isalnum((unsigned char)s[e - 1])) { if (s[e - 1] != ' ') return ""; e--; }
        size_t b = e; while (b > 0 && (std::isalnum((unsigned char)s[b - 1]) || s[b - 1] == '\'')) b--;
        return lower(s.substr(b, e - b));
    }
    void run(const std::vector<Tok>& toks) {
        for (const Tok& k : toks) {
            switch (k.t) {
                case T_TEXT: s += k.s; for (char c : k.s) if (c == '.' || c == '!' || c == '?') sentence++; break;
                case T_NAME: s += k.s; name(k.s); break;
                case T_WORD: s += k.s; if (std::find(R.theirs.begin(), R.theirs.end(), k.s) == R.theirs.end()) R.theirs.push_back(k.s); break;
                case T_FBEGIN: formulaAt.push_back(s.size()); break;
                case T_FEND: if (!formulaAt.empty()) { formulaRaw.push_back(s.substr(formulaAt.back())); formulaAt.pop_back(); } break;
                case T_PAR: s += "\n"; if (!inscription) { sentence += 2; for (int& l : lastOf) l = -1; } break;
                case T_ENT: s += refer(k.ent, k.form); break;
                case T_MENTION: mention(k.ent); break;
                case T_NUMBER: { std::string w = lastWord(); if ((w == "a" || w == "an") && (k.number == VN_PLURAL || k.number == VN_MASS)) R.diag.push_back(std::string("plural: \"a\" before a ") + (k.number == VN_PLURAL ? "plural" : "mass") + " word"); break; }
                case T_VERB: {
                    int person = k.person; bool plural = k.plural;
                    if (k.ent >= 0 && k.ent < (int)E.size()) { person = E[k.ent].person; plural = E[k.ent].gender == VG_THEY && person == 3; }
                    s += vgInflect(k.s, person, plural, k.tense);
                    break;
                }
            }
        }
    }
};

bool isPunct(char c) { return c == ',' || c == '.' || c == ';' || c == ':' || c == '?' || c == '!'; }

// the seams: one space between words, none before a stop or at a line's ends, one after a comma before a word; "a" or "an" by the
// next word's sound; a capital at a sentence's start; a stop at the end
std::string tidy(const std::string& in) {
    std::string s; s.reserve(in.size() + 8);
    for (char c : in) {
        if (c == '\t') c = ' ';
        if (c == ' ') { if (s.empty() || s.back() == ' ' || s.back() == '\n') continue; s += c; continue; }
        if (c == '\n') { while (!s.empty() && s.back() == ' ') s.pop_back(); if (s.empty() || s.back() == '\n') continue; s += c; continue; }
        if (isPunct(c)) {
            while (!s.empty() && s.back() == ' ') s.pop_back();
            if (!s.empty() && s.back() == '\n') { s.pop_back(); while (!s.empty() && s.back() == ' ') s.pop_back(); }
            if (!s.empty() && isPunct(s.back())) {
                char p = s.back();
                if (c == ',' || c == ';' || c == ':') continue;             // a comma after a stop or a comma: the first holds
                if (c == '.' && (p == '.' || p == '?' || p == '!')) continue;
                if (c == '.' && (p == ',' || p == ';' || p == ':')) { s.back() = '.'; continue; }   // "the end, ." -> "the end."
            }
            if (s.empty()) continue;
            s += c; continue;
        }
        if (!s.empty() && (s.back() == ',' || s.back() == ';' || s.back() == ':' || s.back() == '.' || s.back() == '?' || s.back() == '!') && (std::isalnum((unsigned char)c) || c == '\'' || c == '"')) {
            // a space after a comma joining two parts; a stop between digits is a decimal, a colon between them a time (H-04: "06:10")
            if (!((s.back() == '.' || s.back() == ':') && s.size() >= 2 && std::isdigit((unsigned char)s[s.size() - 2]) && std::isdigit((unsigned char)c))) s += ' ';
        }
        s += c;
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\n')) s.pop_back();
    // "a" and "an"
    std::string o; o.reserve(s.size() + 8);
    for (size_t i = 0; i < s.size(); i++) {
        bool wordStart = i == 0 || s[i - 1] == ' ' || s[i - 1] == '\n';
        if (wordStart && (s[i] == 'a' || s[i] == 'A')) {
            bool an = i + 1 < s.size() && s[i + 1] == 'n' && i + 2 < s.size() && s[i + 2] == ' ';
            bool a = i + 1 < s.size() && s[i + 1] == ' ';
            if (a || an) {
                size_t w = i + (an ? 3 : 2), we = s.find_first_of(" \n", w);
                std::string next = s.substr(w, we == std::string::npos ? std::string::npos : we - w);
                if (!next.empty()) { o += s[i]; if (vowelSound(next)) o += 'n'; i += an ? 1 : 0; continue; }
            }
        }
        o += s[i];
    }
    s.swap(o);
    bool cap = true;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (cap && std::isalpha((unsigned char)c)) { s[i] = (char)std::toupper((unsigned char)c); cap = false; }
        else if (cap && std::isdigit((unsigned char)c)) cap = false;
        if (c == '.' || c == '?' || c == '!' || c == '\n') cap = true;
    }
    if (!s.empty() && s.back() != '.' && s.back() != '?' && s.back() != '!') {
        if (s.back() == ',' || s.back() == ';' || s.back() == ':') s.back() = '.'; else s += '.';
    }
    return s;
}

}   // namespace

void VgModel::bind(const VgSchema& S, const std::string& name, int entity) {
    if (bound.size() < S.entities.size()) bound.resize(S.entities.size(), -1);
    for (size_t i = 0; i < S.entities.size(); i++) if (S.entities[i] == name) { bound[i] = entity; return; }
}
void VgModel::set(const VgSchema& S, const std::string& fact, const std::string& value) {
    if (facts.size() < S.facts.size()) facts.resize(S.facts.size());
    for (size_t i = 0; i < S.facts.size(); i++) if (S.facts[i] == fact) { facts[i] = value; return; }
}

bool vgRecord(const VgGrammar& G, const std::string& root, VgModel& M, VgMemory* mem, VgRecord& R) {
    double t0 = nowMs();
    R.text.clear(); R.root = -1; R.words = 0; R.lo = R.hi = 0; R.used.clear(); R.names.clear(); R.theirs.clear(); R.diag.clear(); R.retries = 0;
    if (G.schema) { if (M.facts.size() < G.schema->facts.size()) M.facts.resize(G.schema->facts.size()); if (M.bound.size() < G.schema->entities.size()) M.bound.resize(G.schema->entities.size(), -1); }
    if (mem && mem->uses.size() < G.alts.size()) mem->uses.resize(G.alts.size(), 0);
    if (mem && mem->wordUses.size() < G.words.size()) mem->wordUses.resize(G.words.size(), 0);
    if (mem && mem->dialect.size() < G.alts.size()) mem->dialect.resize(G.alts.size(), 0.0f);
    int r = G.rule(root);
    if (r < 0) { R.diag.push_back("no rule " + root); return false; }
    size_t ents0 = M.entities.size();
    Expander X(G, M, mem, R);
    X.rootRule = r;
    Scope sc; sc.tense = R.tense;
    X.cur = &sc;
    bool ok = X.expandRule(r, sc, std::string());
    if (mem) X.frames(0, mem->frameSeq, true);   // H-04: every sentence of the record kept (those the genres' own text joined too)
    Realiser Z(M.entities, R.inscription, R);
    Z.run(X.out);
    R.text = tidy(Z.s);
    R.words = vgWords(R.text);
    R.formulas.clear();   // H-04: the formulas' sentences, as frames
    for (const std::string& raw : Z.formulaRaw) for (const std::string& sn : vgSentences(tidy(raw))) if (vgWords(sn) >= 6) R.formulas.push_back(vgFrame(sn));
    if (!G.lexFact.empty()) {   // H-04: a word its fact does not allow (the railway before it opened)
        std::string w; std::set<std::string> told;
        for (size_t i = 0; i <= R.text.size(); i++) {
            char c = i < R.text.size() ? R.text[i] : ' ';
            if (std::isalpha((unsigned char)c)) { w += (char)std::tolower((unsigned char)c); continue; }
            if (!w.empty()) {
                auto it = G.lexFact.find(w);
                if (it != G.lexFact.end() && !told.count(w)) {
                    const std::string& v = it->second >= 0 && it->second < (int)M.facts.size() ? M.facts[it->second] : std::string();
                    if (v.empty() || v == "0") { R.diag.push_back("anachronism: '" + w + "' before " + G.schema->facts[it->second]); told.insert(w); }
                }
                w.clear();
            }
        }
    }
    M.entities.resize(ents0);   // the persons the text brought in are its own
    R.us = 1000.0 * (nowMs() - t0);
    return ok && R.diag.empty();
}

// ---------------------------------------------------------------------------
// the checks and the measures
// ---------------------------------------------------------------------------

int vgWords(const std::string& text) {
    int n = 0; bool in = false;
    for (char c : text) { if (c == ' ' || c == '\n') in = false; else if (!in) { in = true; n++; } }
    return n;
}

std::vector<std::string> vgSentences(const std::string& t) {
    std::vector<std::string> out; std::string cur;
    for (char c : t) {
        cur += c;
        if (c == '.' || c == '?' || c == '!' || c == '\n') { size_t p = cur.find_first_not_of(" \n"); if (p != std::string::npos) { std::string x = cur.substr(p); while (!x.empty() && x.back() == '\n') x.pop_back(); if (!x.empty()) out.push_back(x); } cur.clear(); }
    }
    size_t p = cur.find_first_not_of(" \n"); if (p != std::string::npos) out.push_back(cur.substr(p));
    return out;
}

std::string vgFrame(const std::string& s) {
    static const std::set<std::string> NUMW = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "twenty", "thirty", "forty",
                                               "fifty", "sixty", "hundred", "thousand", "first", "second", "third", "fourth", "fifth", "no"};
    std::string out; bool first = true; size_t p = 0;
    while (p < s.size()) {
        size_t q = s.find_first_of(" \n", p); if (q == std::string::npos) q = s.size();
        std::string w = s.substr(p, q - p); p = q + 1;
        if (w.empty()) continue;
        std::string core; for (char c : w) if (std::isalnum((unsigned char)c) || c == '\'') core += c;
        std::string low = lower(core);
        if (!core.empty() && std::isdigit((unsigned char)core[0])) out += "# ";
        else if (NUMW.count(low)) out += "# ";
        else if (!first && !core.empty() && std::isupper((unsigned char)core[0]) && low != "i") out += "N ";
        else out += low + " ";
        first = false;
    }
    return out;
}

namespace {
// a text's sentences of six words or more that are not its formulas' (H-04: a greeting, a heading, a blessing recur by design)
void longSentences(const std::string& t, const std::vector<std::string>* formulas, std::vector<std::string>& out, int* formulaS) {
    out.clear();
    for (const std::string& s : vgSentences(t)) {
        if (vgWords(s) < 6) continue;
        if (formulas && !formulas->empty()) { std::string f = vgFrame(s); if (std::find(formulas->begin(), formulas->end(), f) != formulas->end()) { if (formulaS) (*formulaS)++; continue; } }
        out.push_back(s);
    }
}
}

VgMeasure vgMeasure(const std::vector<std::string>& texts, const std::vector<const std::vector<std::string>*>* formulas) {
    VgMeasure M; M.minW = 1 << 30;
    std::unordered_map<std::string, int> frames, exact;
    std::vector<std::string> toks, longs;
    for (size_t ti = 0; ti < texts.size(); ti++) {
        const std::string& t = texts[ti];
        const std::vector<std::string>* f = formulas && ti < formulas->size() ? (*formulas)[ti] : nullptr;
        M.texts++; int w = vgWords(t); M.words += w; M.minW = std::min(M.minW, w); M.maxW = std::max(M.maxW, w);
        M.sentences += (int)vgSentences(t).size();
        longSentences(t, f, longs, &M.formulaS);
        for (const std::string& s : longs) { frames[vgFrame(s)]++; exact[s]++; }
        size_t p = 0;
        while (p < t.size()) { size_t q = t.find_first_of(" \n", p); if (q == std::string::npos) q = t.size(); std::string l; for (size_t i = p; i < q; i++) if (std::isalpha((unsigned char)t[i])) l += (char)std::tolower((unsigned char)t[i]); if (!l.empty()) toks.push_back(l); p = q + 1; }
    }
    for (size_t ti = 0; ti < texts.size(); ti++) {
        const std::vector<std::string>* f = formulas && ti < formulas->size() ? (*formulas)[ti] : nullptr;
        longSentences(texts[ti], f, longs, nullptr);
        for (const std::string& s : longs) { M.longS++; if (frames[vgFrame(s)] > 1) M.longRep++; if (exact[s] == 1) M.distinct++; }
    }
    std::set<std::string> b2, b3; size_t T = std::min<size_t>(toks.size(), 1500), n2 = 0, n3 = 0;
    for (size_t i = 0; i + 1 < T; i++) { b2.insert(toks[i] + " " + toks[i + 1]); n2++; }
    for (size_t i = 0; i + 2 < T; i++) { b3.insert(toks[i] + " " + toks[i + 1] + " " + toks[i + 2]); n3++; }
    M.d2 = n2 ? (double)b2.size() / n2 : 0; M.d3 = n3 ? (double)b3.size() / n3 : 0;
    if (M.texts == 0) M.minW = 0;
    return M;
}

void vgTopFrames(const std::vector<std::string>& texts, int n, std::vector<std::pair<int, std::string>>& out, const std::vector<const std::vector<std::string>*>* formulas) {
    std::unordered_map<std::string, int> f;
    std::vector<std::string> longs;
    for (size_t ti = 0; ti < texts.size(); ti++) {
        longSentences(texts[ti], formulas && ti < formulas->size() ? (*formulas)[ti] : nullptr, longs, nullptr);
        for (const std::string& s : longs) f[vgFrame(s)]++;
    }
    out.clear();
    for (const auto& kv : f) if (kv.second > 1) out.push_back({kv.second, kv.first});
    std::sort(out.begin(), out.end(), [](const std::pair<int, std::string>& a, const std::pair<int, std::string>& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
    if ((int)out.size() > n) out.resize(n);
}

int vgCheck(const VgGrammar& G, const VgRecord& R, int age, int counts[VC_COUNT], std::vector<std::string>* why) {
    int bad = 0;
    auto fail = [&](int kind, const std::string& w) { counts[kind]++; bad++; if (why) why->push_back(std::string(VG_CHECK_NAMES[kind]) + ": " + w); };
    const std::string& t = R.text;
    for (const char* m : {"{", "}", "[", "]", "$", "@", "#", "|", "\\", "\x01", "\x02"}) if (t.find(m) != std::string::npos) fail(VC_MARK, m[0] == '\x01' ? "a name's opening mark" : (m[0] == '\x02' ? "a name's closing mark" : m));
    for (const std::string& d : R.diag) { if (startsWith(d, "plural:")) fail(VC_PLURAL, d.substr(8)); else if (startsWith(d, "anachronism:")) fail(VC_ANACHRONISM, d.substr(13)); else fail(VC_DIAG, d); }
    if (t.empty()) { fail(VC_DIAG, "empty"); return bad; }
    // the words, sentence by sentence (a stop, a comma or a colon breaks the adjacency of two words)
    struct W { std::string w, raw; bool afterPunct; size_t pos; bool cap; };   // raw: as written ("mother's mother" is no doubled word); cap: a capital
    std::vector<W> words; bool punct = true; size_t i = 0;
    while (i < t.size()) {
        if (std::isalnum((unsigned char)t[i])) {
            size_t j = i; while (j < t.size() && (std::isalnum((unsigned char)t[j]) || t[j] == '\'' || t[j] == '-')) j++;
            std::string raw = lower(t.substr(i, j - i)), w = raw;
            if (endsWith(w, "'s")) w.resize(w.size() - 2);
            words.push_back({w, raw, punct, i, std::isupper((unsigned char)t[i]) != 0}); punct = false; i = j; continue;
        }
        if (isPunct(t[i]) || t[i] == '\n') punct = true;
        i++;
    }
    static const std::set<std::pair<std::string, std::string>> BAD = {{"he", "are"}, {"she", "are"}, {"it", "are"}, {"he", "were"}, {"she", "were"}, {"it", "were"}, {"he", "am"},
        {"she", "am"}, {"it", "am"}, {"they", "is"}, {"they", "was"}, {"they", "am"}, {"we", "is"}, {"we", "was"}, {"we", "am"}, {"you", "is"}, {"you", "was"}, {"you", "am"},
        {"i", "is"}, {"i", "are"}, {"he", "have"}, {"she", "have"}, {"it", "have"}, {"they", "has"}, {"we", "has"}, {"i", "has"}, {"you", "has"}, {"he", "do"}, {"she", "do"},
        {"they", "does"}, {"we", "does"}, {"i", "does"}};
    for (size_t k = 0; k < words.size(); k++) {
        const W& x = words[k];
        if (k == 0 || x.afterPunct) continue;
        const W& p = words[k - 1];
        if (p.raw == x.raw && x.w != "had" && x.w != "that" && !std::isdigit((unsigned char)x.w[0])) fail(VC_DOUBLED, "'" + x.raw + " " + x.raw + "'");
        if (p.w == "a" || p.w == "an") {
            bool v = vowelSound(x.w);
            if ((p.w == "a" && v) || (p.w == "an" && !v)) fail(VC_ARTICLE, "'" + p.w + " " + x.w + "'");
        }
        if ((p.w == "a" || p.w == "an" || p.w == "one") && G.plurals.count(x.w)) fail(VC_PLURAL, "'" + p.w + " " + x.w + "'");
        if ((p.w == "a" || p.w == "an" || p.w == "the") && (x.w == "a" || x.w == "an" || x.w == "the") && p.raw != x.raw) fail(VC_ARTICLE, "'" + p.w + " " + x.w + "'");   // H-04's second pass: "a the rains" (a list that carries its article)
        const bool regnal = p.w == "i" && k >= 2 && !p.afterPunct && words[k - 2].cap;   // H-04: "King Nilzhos I is dead" (a name before the numeral)
        // H-04's second pass: the pronoun is not the verb's subject after a preposition or in a wish ("to you is", "as if it were")
        static const std::set<std::string> NOT_SUBJECT = {"if", "as", "to", "for", "with", "of", "at", "by", "from", "about", "than", "like", "on", "into", "without"};
        const bool notSubject = k >= 2 && !p.afterPunct && NOT_SUBJECT.count(words[k - 2].w) > 0;
        if (BAD.count({p.w, x.w}) && !regnal && !notSubject) fail(VC_AGREEMENT, "'" + p.w + " " + x.w + "'");
    }
    // a sentence's first letter a capital
    bool start = true;
    for (size_t k = 0; k < t.size(); k++) {
        char c = t[k];
        if (start && std::isalpha((unsigned char)c)) { if (!std::isupper((unsigned char)c)) fail(VC_CAPITAL, "'" + t.substr(k, 12) + "'"); start = false; }
        else if (start && std::isdigit((unsigned char)c)) start = false;
        if (c == '.' || c == '?' || c == '!' || c == '\n') start = true;
    }
    for (const char* sp : {"  ", " ,", " .", " ;", " :", " ?", " !", ",,", "..", ",.", "\n\n", " \n", "\n "}) if (t.find(sp) != std::string::npos) fail(VC_SPACING, std::string("'") + sp + "'");
    for (size_t k = 0; k + 1 < t.size(); k++) if ((t[k] == ',' || t[k] == ';') && t[k + 1] != ' ' && t[k + 1] != '\n') fail(VC_SPACING, "no space after '" + std::string(1, t[k]) + "'");
    if (R.hi > 0 && (R.words < R.lo || R.words > R.hi)) fail(VC_LENGTH, std::to_string(R.words) + " words of " + std::to_string(R.lo) + ".." + std::to_string(R.hi));
    if (age >= 0) for (const W& x : words) { auto it = G.lexicon.find(x.w); if (it != G.lexicon.end() && it->second > age) fail(VC_ANACHRONISM, "'" + x.w + "' (" + VG_AGES[it->second] + ") in a " + VG_AGES[std::min(age, 5)] + " record"); }
    return bad;
}
