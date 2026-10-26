// E-10: the realiser's English words (a verb's past and third person, a plural, the numbers and ordinals in words, a name's
// marks) and the checks and measures of the texts (`vgCheck`, `vgMeasure`, `vgTopFrames`)
#include "grammar_internal.h"

const char* const VG_CHECK_NAMES[VC_COUNT] = {"mark left", "diagnostic", "doubled word", "a or an", "article before a plural", "pronoun and verb",
                                              "capital", "spacing", "length", "anachronism"};

namespace {
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

std::string vgName(const std::string& name) { return std::string(1, VG_NAME_BEGIN) + name + VG_NAME_END; }

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
