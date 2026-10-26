// H-03 (2026-10-09): the grammar engine (`grammar.h`): the `.vg` format's parser and link, the expansion, the realiser, the checks
// and the measures. The prototype H-01 measured (`RESEARCH-records.md`, section 8) in the tree's own terms: texts parsed once into
// nodes, references resolved against the model's schema at load, a record's attributes kept in reading order.
#include "grammar.h"
#include "grammar_internal.h"

const char* const VG_AGES[6] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "farfuture"};

namespace {
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
        for (const VgInstead& I : G.insteads) if (I.fromRule == ri && I.toRule >= 0 && holds(I.cond, sc)) { ri = I.toRule; break; }   // D-05
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
