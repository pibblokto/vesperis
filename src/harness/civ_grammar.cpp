// E-03: the records' grammar (H-03, H-04): `grammar`, `tongues`, the grammar's checks
#include "harness/harness.h"
#include "harness/test_worlds.h"

// H-03: the grammar engine. `grammarEngineCases`: a small grammar whose every rule must give one text (the realiser's pronouns, verbs,
// numbers, plurals, articles, attributes, `once`, `keep`, guards, a directive, a path, a person brought in, a letter's I and you, a kept
// word's agreement, the seams), the link's errors on a broken grammar, the checks on planted faults
namespace {
struct GrammarTestModel : VgModel {
    bool tongue(const std::string& key, std::string& out) override { out = key == "coin" ? "dashku" : ""; return !out.empty(); }   // H-04's second pass
    int follow(int e, int path) override { return path == 0 && e == 0 ? 5 : -1; }   // Aiwya's mother: Ilse
    bool field(int e, int f, std::string& out) override { if (f == 0) { out = e == 0 ? "weaver" : "smith"; return true; } return false; }
    bool directive(VgRun& run, int d, const std::string& args) override { if (d != 0) return false; run.bindEntity("$e", 6); return true; }
};
const char* const GRAMMAR_TEST = R"VG(
@root t.*
@lexicon industrial railway
t.pron
  {$a} came home. {$a} sat down, and {$b} gave {$a:obj} {$a:poss} cup. {$a} thanked {$b:obj}.
t.verb
  {$a} {walk:$a} to the well; we {go:we} with {$a:obj}; I {be:I} tired and they {have:they} bread; {$g} {be:$g} here.
t.verbpast
  (tense=past) {$a} {walk:$a} to the well; we {go:we} with {$a:obj}; I {be:I} tired and they {have:they} bread; {$g} {be:$g} here.
t.num
  {#one|year}, {#two|year}, {#twelve|child}, {#big|family}, {#big:about|family}, the {#big:ord} day, {#big:words}, {#t.nlist|goose}.
t.nlist
  three
t.plural
  {t.beast:pl} and {t.tree:pl} by the {t.bed:pl}.
t.beast
  goose
t.tree
  lamp tree
t.bed
  bed of the river
t.article
  a {t.owl}, a {t.goat}, an {t.house}, a hour, a one-eyed goat, a 8 and a unit.
t.owl
  owl
t.goat
  goat
t.house
  house
t.attr
  {t.attr.main}{t.attr.tail}
t.attr.main
  work is slow{=.slow}
t.attr.tail
  (.slow=1) , so I sit by the door.
  (!has:.slow) .
t.once
  {t.once.x} {t.once.x} {t.once.x}
t.once.x
  (once) one.
  (once) two.
  (once) three.
t.keep
  {t.crop}, {t.crop}, {t.crop}, {t.crop}
t.crop (mass keep)
  barley
  rye
  kale
  flax
t.guard
  (desert=1 age>=industrial) yes
  (desert=0|age<medieval) no
t.guard2
  (desert=0|age>=modern) no
  (desert=0|age>=industrial) yes
t.dir
  {@pick}{$e} came, and {$e} sat by the door.
t.path
  {$a.mother} loved {$a:obj}; {$a} was a {$a.trade}.
t.decl
  A woman{+$w:she|the woman} came in. {$w} sat by {$a} and {$w} wept.
t.letter
  {$me} {send:$me} {$you:obj} {$me:poss} love; {$you} {be:$you} kind, {$you:voc}.
t.word
  the {t.work>.w} {be:.w} mine and nobody will need {.w:obj}.
t.work
  (pl) nets
t.tidy
  \ \ hello ,world .  this is  it
t.plurals (pl)
  lizards
t.theirs
  three {~coin} for the {t.mine}.
t.mine
  salt
t.few (people=2)
  alpha
  beta
  gamma
  delta
  epsilon
  zeta
t.frame
  (3) the heavy sentence of the test is written here.
  the light sentence of the test is written here.
)VG";
const char* const GRAMMAR_BROKEN = R"VG(
@root b.top
b.top
  a {b.goats} and {b.nothing}
  (nofact=1) x
b.goats (pl)
  goats
b.lost
  never reached
)VG";
}   // namespace

static int grammarEngineCases(std::string& detail) {
    VgSchema S;
    S.facts = {"desert", "age", "one", "two", "twelve", "big"};
    S.entities = {"$a", "$b", "$g", "$me", "$you", "$e"};
    S.paths = {"mother"}; S.fields = {"trade"}; S.directives = {"pick"};
    VgGrammar G; vgParse(G, GRAMMAR_TEST, "test.vg"); vgLink(G, S);
    int bad = 0;
    for (const std::string& e : G.errors) { bad++; detail += "test grammar: " + e + "; "; }
    GrammarTestModel M;
    M.facts = {"1", "industrial", "1", "2", "12", "212"};
    auto person = [](const char* name, const char* first, int gender, bool shown, const char* later = "") { VgEntity e; e.name = name; e.first = first; e.gender = gender; e.shown = shown; e.later = later; return e; };
    M.entities = {person("Aiwya", "", VG_SHE, false), person("", "my father", VG_HE, true), person("", "the boatmen", VG_THEY, true), person("Kerro", "", VG_HE, false),
                  person("Taro", "", VG_SHE, false), person("Ilse", "", VG_SHE, false), person("", "a stranger", VG_HE, false, "the stranger"), person("", "my mother", VG_SHE, true)};
    M.entities[3].person = 1; M.entities[4].person = 2;
    auto bindAll = [&](int b) { M.bound.assign(S.entities.size(), -1); M.bound[0] = 0; M.bound[1] = b; M.bound[2] = 2; M.bound[3] = 3; M.bound[4] = 4; };
    struct Case { const char* rule; int b; const char* want; };
    const Case cases[] = {
        {"t.pron", 1, "Aiwya came home. She sat down, and my father gave her her cup. She thanked him."},
        {"t.pron", 7, "Aiwya came home. She sat down, and my mother gave Aiwya her cup. She thanked my mother."},
        {"t.verb", 1, "Aiwya walks to the well; we go with her; I am tired and they have bread; the boatmen are here."},
        {"t.verbpast", 1, "Aiwya walked to the well; we went with her; I was tired and they had bread; the boatmen were here."},
        {"t.num", 1, "One year, two years, twelve children, 212 families, some two hundred families, the 212th day, two hundred and twelve, three geese."},
        {"t.plural", 1, "Geese and lamp trees by the beds of the river."},
        {"t.article", 1, "An owl, a goat, a house, an hour, a one-eyed goat, an 8 and a unit."},
        {"t.attr", 1, "Work is slow, so I sit by the door."},
        {"t.guard", 1, "Yes."},
        {"t.guard2", 1, "Yes."},
        {"t.dir", 1, "A stranger came, and he sat by the door."},
        {"t.path", 1, "Ilse loved Aiwya; she was a weaver."},
        {"t.decl", 1, "A woman came in. She sat by Aiwya and the woman wept."},
        {"t.letter", 1, "I send you my love; you are kind, Taro."},
        {"t.word", 1, "The nets are mine and nobody will need them."},
        {"t.tidy", 1, "Hello, world. This is it."}};
    int n = 0;
    for (const Case& c : cases) {
        bindAll(c.b);
        VgRecord R; R.seed = 77 + n++;
        vgRecord(G, c.rule, M, nullptr, R);
        if (R.text != c.want || !R.diag.empty()) { bad++; detail += fmt("%s gave '%s'%s (want '%s'); ", c.rule, R.text.c_str(), R.diag.empty() ? "" : (" with " + R.diag[0]).c_str(), c.want); }
    }
    for (int s = 0; s < 6; s++) {   // `once`: three different; `keep`: one crop four times
        bindAll(1);
        VgRecord R; R.seed = 1000 + s; vgRecord(G, "t.once", M, nullptr, R);
        if (R.text.find("One.") == std::string::npos || R.text.find("Two.") == std::string::npos || R.text.find("Three.") == std::string::npos) { bad++; detail += "t.once gave '" + R.text + "'; "; }
        VgRecord K; K.seed = 2000 + s; vgRecord(G, "t.keep", M, nullptr, K);
        std::string first = K.text.substr(0, K.text.find(','));
        std::string lowFirst = first; for (char& ch : lowFirst) ch = (char)tolower((unsigned char)ch);
        if (K.text != first + ", " + lowFirst + ", " + lowFirst + ", " + lowFirst + ".") { bad++; detail += "t.keep gave '" + K.text + "'; "; }
    }
    {   // H-04's second pass: their own word (kept apart from the names), a people's own few (two of six, the same two every time, another
        // people's another two), a sentence said once (the frame memory draws the heavy alternative again when the light one is left)
        bindAll(1);
        VgRecord R; R.seed = 31; vgRecord(G, "t.theirs", M, nullptr, R);
        if (R.text != "Three dashku for the salt." || R.theirs.size() != 1 || R.theirs[0] != "dashku" || !R.names.empty()) { bad++; detail += "t.theirs gave '" + R.text + "'; "; }
        std::set<std::string> seenA, seenB;
        for (int s = 0; s < 40; s++) {
            M.culture = 0xA11CEULL; VgRecord A; A.seed = 500 + s; vgRecord(G, "t.few", M, nullptr, A); seenA.insert(A.text);
            M.culture = 0xB0BULL; VgRecord B; B.seed = 900 + s; vgRecord(G, "t.few", M, nullptr, B); seenB.insert(B.text);
        }
        M.culture = 0;
        if (seenA.size() != 2 || seenB.size() != 2) { bad++; detail += fmt("t.few: a people kept %zu and another %zu of six (want two each); ", seenA.size(), seenB.size()); }
        int same = 0;
        for (int s = 0; s < 20; s++) {
            VgMemory mem; VgRecord A, B; A.seed = 3000 + 2 * s; B.seed = 3001 + 2 * s;
            vgRecord(G, "t.frame", M, &mem, A); vgRecord(G, "t.frame", M, &mem, B);
            if (A.text == B.text) same++;
        }
        if (same) { bad++; detail += fmt("t.frame: the same sentence twice in %d of 20 catalogues; ", same); }
    }
    {   // the link on a broken grammar: "a" before a plural list, an unknown rule, an unknown fact in a guard, a rule no root reaches
        VgGrammar B; vgParse(B, GRAMMAR_BROKEN, "broken.vg"); vgLink(B, S);
        int found = 0;
        for (const char* want : {"\"a\" before {b.goats}", "unknown {b.nothing}", "unknown fact 'nofact'", "b.lost is reached from no root"})
            for (const std::string& e : B.errors) if (e.find(want) != std::string::npos && e.find("broken.vg:") == 0) { found++; break; }
        if (found != 4) { bad++; detail += fmt("the broken grammar's errors: %d of 4 found (%zu errors: %s); ", found, B.errors.size(), B.errors.empty() ? "-" : B.errors[0].c_str()); }
    }
    {   // the checks on planted faults: a doubled word, an article before a plural, a pronoun disagreeing with its verb, a word of a later age
        VgRecord R; R.text = "The the well is dry. A lizards came. He were here. We took the railway.";
        int counts[VC_COUNT] = {};
        vgCheck(G, R, 1, counts, nullptr);
        if (counts[VC_DOUBLED] != 1 || counts[VC_PLURAL] != 1 || counts[VC_AGREEMENT] != 1 || counts[VC_ANACHRONISM] != 1 || counts[VC_ARTICLE] != 0) {
            bad++; detail += fmt("the checks on planted faults: doubled %d, plural %d, agreement %d, anachronism %d, article %d (want 1 1 1 1 0); ", counts[VC_DOUBLED], counts[VC_PLURAL], counts[VC_AGREEMENT], counts[VC_ANACHRONISM], counts[VC_ARTICLE]);
        }
    }
    detail = fmt("%d cases, the broken grammar, the planted faults: %s", n, bad ? detail.c_str() : "all as written");
    return bad;
}

// H-03: the worlds the grammar is read over: the C series' test worlds, the third (two peoples), the six age worlds, then the peoples
// nearest home
struct GrammarWorld { int64_t sx, sy, sz; int body, which; };
static std::vector<GrammarWorld> grammarWorlds(int nearHome) {
    std::vector<GrammarWorld> w = {{151, 0, 25, 1, 0}, {153, 0, 70, 2, 0}, {152, 0, 92, 0, 0}, {152, 0, 92, 0, 1}};
    for (int a = 0; a < AGE_COUNT; a++) w.push_back({AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, AGE_WORLDS[a].body, AGE_WORLDS[a].which});
    if (nearHome > 0)
        for (const NearPeople& p : peoplesNearHome(nearHome)) {
            bool dup = false; for (const GrammarWorld& x : w) if (x.sx == p.x && x.sy == p.y && x.sz == p.z && x.body == p.body && x.which == p.which) dup = true;
            if (!dup) w.push_back({p.x, p.y, p.z, p.body, p.which});
        }
    return w;
}
struct GrammarRun {
    int peoples = 0, records = 0, texts = 0, charts = 0; long words = 0; int minW = 1 << 30, maxW = 0;
    int counts[VC_COUNT] = {};
    int namesBad = 0, namesTold = 0;                 // H-04: the names written, and those not the history's at the record's year
    double usSum = 0, usMax = 0, msSum = 0, msMax = 0;
    std::vector<std::string> failures;
    std::vector<int> altUses, kindUses;
    double rep50 = 0, rep100 = 0, repAll = 0, dist50 = 0, d2sum = 0, d3sum = 0; int n50 = 0, n100 = 0;   // d2/d3: distinct 2/3-grams, summed over the peoples
    std::vector<std::string> sampleTexts;            // the first people's (the most repeated frames)
    std::vector<std::vector<std::string>> sampleForms;
    // H-04: by genre (its name): the records, their words, the shortest and the longest, and the texts (for the genre's own measures)
    struct Genre { int n = 0; long words = 0; int minW = 1 << 30, maxW = 0; std::vector<std::string> texts; std::vector<std::vector<std::string>> forms; std::vector<int> peopleOf; };
    std::map<std::string, Genre> genres;
    int byAge[AGE_COUNT] = {};                        // the texts of each age
    int sides = 0, sidesEvents = 0;                   // H-04: events told by two registers or more (one event from several sides)
    std::vector<std::string> sidesSample;             // the first such, its tellings
    // H-04's second pass: one people's sentences against the others': each people's long sentences' frames (its first hundred texts)
    std::vector<std::vector<std::string>> peopleFrames;
    int retries = 0, keptRepeats = 0;                 // the engine's draws undone for a repeated frame, and the repeats let stand
};
// a people's catalogue through the checks (and its texts to `out`, the first `print` to stdout)
static bool grammarPeople(const GrammarWorld& w, GrammarRun& R, FILE* out, int print) {
    Star st; if (!placeInSector(w.sx, w.sy, w.sz, st, true)) return false;   // D-05: a drifter's people too
    StarSystem sys; sys.generate(st);
    if (w.body >= (int)sys.bodies.size()) return false;
    const Body& b = sys.bodies[w.body];
    const BodyGen g = BodyGen::make(b);
    if (!worldHadCivilisation(g) || w.which >= std::max(1, peoplesOf(g))) return false;
    Lore L = loreQuick(sys, b, g, w.which);
    std::shared_ptr<const VgGrammar> G = shardGrammar();
    if (R.altUses.size() < G->alts.size()) R.altUses.resize(G->alts.size(), 0);
    if ((int)R.kindUses.size() < shardKindCount()) R.kindUses.resize(shardKindCount(), 0);
    std::shared_ptr<const ShardTexts> T = shardTextsOf(sys, b, g, L);
    std::shared_ptr<const History> H = historyOf(sys, b, g, w.which);
    R.peoples++; R.records += (int)T->items.size();
    R.msSum += T->buildMs; R.msMax = std::max(R.msMax, T->buildMs);
    const int age = peopleAgeOf(g, w.which).age;
    std::vector<std::string> texts; std::vector<std::vector<std::string>> forms;
    std::map<int, std::vector<std::pair<std::string, int>>> toldBy;   // an event and the genres (and records) that told it
    if (out) fprintf(out, "== %s%s (%s, %s): %zu records, the %s\n", b.name.c_str(), w.which ? " (second)" : "", AGE_NAMES[age], L.desert ? "desert" : "felisian", T->items.size(), L.people.c_str());
    for (size_t i = 0; i < T->items.size(); i++) {
        const ShardText& x = T->items[i];
        if (x.text.empty()) continue;
        R.texts++; texts.push_back(x.text); forms.push_back(x.rec.formulas);
        if (x.age >= 0 && x.age < AGE_COUNT) R.byAge[x.age]++;
        int nw = shardWords(x.text); R.words += nw; R.minW = std::min(R.minW, nw); R.maxW = std::max(R.maxW, nw);
        R.usSum += x.rec.us; R.usMax = std::max(R.usMax, x.rec.us);
        for (int a : x.rec.used) if (a >= 0 && a < (int)R.altUses.size()) R.altUses[a]++;
        if (x.kind >= 0 && x.kind < (int)R.kindUses.size()) R.kindUses[x.kind]++;
        const std::string gname = shardKindName(x.kind);
        GrammarRun::Genre& gr = R.genres[gname];
        gr.n++; gr.words += nw; gr.minW = std::min(gr.minW, nw); gr.maxW = std::max(gr.maxW, nw);
        if (gr.texts.size() < 400) { gr.texts.push_back(x.text); gr.forms.push_back(x.rec.formulas); gr.peopleOf.push_back(R.peoples - 1); }
        for (int ev : x.told) toldBy[ev].push_back({gname, (int)i});
        std::vector<std::string> why;
        if (vgCheck(*G, x.rec, x.age, R.counts, &why) && R.failures.size() < 5000)
            R.failures.push_back(fmt("%s S%zu (%s, %s): %s\n      %s", b.name.c_str(), i, VG_AGES[std::max(0, x.age)], gname.c_str(), why[0].c_str(), x.text.c_str()));
        if (H) for (const std::string& nm : x.rec.names) {   // H-04: every name the history's (or the lore's) at the record's year
            R.namesTold++;
            if (!recordNameKnown(*H, L, x.year, nm)) { R.namesBad++; if (R.failures.size() < 5000) R.failures.push_back(fmt("%s S%zu (%s): the name '%s' is not the history's in year %d", b.name.c_str(), i, gname.c_str(), nm.c_str(), x.year)); }
        }
        if (out) {
            std::string t = x.text; for (size_t k = 0; k < t.size(); k++) if (t[k] == '\n') t.replace(k, 1, "\n                                        ");
            fprintf(out, "%4zu  %-10s %-16s y%-5d %3dw  %s\n", i, VG_AGES[std::max(0, x.age)], gname.c_str(), x.year, nw, t.c_str());
            for (const std::string& f : x.rec.formulas) fprintf(out, "      ~f %s\n", f.c_str());   // the formulas' frames (the reading tools leave them out)
        }
        if (print < 0 || (int)texts.size() <= print) printf("  %s S%zu %s: %s\n", b.name.c_str(), i, gname.c_str(), x.text.c_str());
    }
    for (const auto& kv : toldBy) {   // H-04: one event from several sides: told in two genres or more
        std::set<std::string> gs; for (const auto& p : kv.second) gs.insert(p.first);
        if (gs.size() < 2) continue;
        R.sidesEvents++; R.sides += (int)kv.second.size();
        if (R.sidesSample.empty() && H && gs.size() >= 3) {
            R.sidesSample.push_back(fmt("%s, event %d (%s): %s", b.name.c_str(), kv.first, HISTORY_EVENT_NAMES[H->events[kv.first].kind], historyEventLine(*H, kv.first).c_str()));
            for (const auto& p : kv.second) R.sidesSample.push_back(fmt("[%s, S%d] %s", p.first.c_str(), p.second, T->items[p.second].text.c_str()));
        }
    }
    {   // the star charts' captions (C-10: written per chart, outside the catalogue; the full lore holds their marks)
        Lore full = loreOf(sys, b, g, w.which);
        for (size_t i = 0; i < T->items.size(); i++) {
            VgRecord rec;
            if (!shardChartRecord(sys, b, g, full, (int)i, rec)) continue;
            R.charts++;
            for (int a : rec.used) if (a >= 0 && a < (int)R.altUses.size()) R.altUses[a]++;
            std::vector<std::string> why;
            if (vgCheck(*G, rec, T->items[i].age, R.counts, &why) && R.failures.size() < 5000) R.failures.push_back(fmt("%s S%zu (chart): %s\n      %s", b.name.c_str(), i, why[0].c_str(), rec.text.c_str()));
            if (out) fprintf(out, "%4zu  %-10s %-16s y%-5d %3dw  %s\n", i, VG_AGES[std::max(0, T->items[i].age)], "star chart", T->items[i].year, shardWords(rec.text), rec.text.c_str());
        }
    }
    {   // H-04's second pass: the frames of the people's first hundred texts' long sentences (the formulas left out), for the peoples' overlap
        std::vector<std::string> fr;
        for (size_t k = 0; k < texts.size() && k < 100; k++) for (const std::string& sn : vgSentences(texts[k])) {
            if (vgWords(sn) < 6) continue;
            const std::string f = vgFrame(sn);
            if (std::find(forms[k].begin(), forms[k].end(), f) != forms[k].end()) continue;
            fr.push_back(f);
        }
        R.peopleFrames.push_back(fr);
        for (const ShardText& x : T->items) R.retries += x.rec.retries;
    }
    auto measure = [&](size_t n) { std::vector<std::string> t(texts.begin(), texts.begin() + std::min(n, texts.size())); std::vector<const std::vector<std::string>*> f; for (size_t k = 0; k < t.size(); k++) f.push_back(&forms[k]); return vgMeasure(t, &f); };
    if (texts.size() >= 50) { VgMeasure m = measure(50); R.rep50 += m.longS ? 100.0 * m.longRep / m.longS : 0; R.dist50 += m.longS ? 100.0 * m.distinct / m.longS : 0; R.n50++; }
    if (texts.size() >= 100) { VgMeasure m = measure(100); R.rep100 += m.longS ? 100.0 * m.longRep / m.longS : 0; R.n100++; }
    VgMeasure all = measure(texts.size()); R.repAll += all.longS ? 100.0 * all.longRep / all.longS : 0; R.d2sum += all.d2; R.d3sum += all.d3;
    if (R.sampleTexts.empty()) { R.sampleTexts = texts; R.sampleForms = forms; }
    if (getenv("VESPERIS_TOPFRAMES")) {   // H-04: a people's own most repeated frames (the tuning's)
        std::vector<const std::vector<std::string>*> f; for (const auto& x : forms) f.push_back(&x);
        std::vector<std::pair<int, std::string>> top; vgTopFrames(texts, atoi(getenv("VESPERIS_TOPFRAMES")), top, &f);
        printf("  -- %s's most repeated frames:\n", b.name.c_str());
        for (const auto& t : top) printf("       x%d  %s\n", t.first, t.second.c_str());
    }
    printf("  %-24s %-11s %3zu records, %3zu texts, %3d words a text (%d-%d), %5.1f ms; repeated frames %4.1f%% of %d sentences of six words or more\n",
           (b.name + (w.which ? " (2)" : "")).c_str(), AGE_NAMES[age], T->items.size(), texts.size(), all.texts ? all.words / all.texts : 0, all.minW, all.maxW, T->buildMs,
           all.longS ? 100.0 * all.longRep / all.longS : 0.0, all.longS);
    return true;
}

// the unit's H-03 checks
void grammarChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    {
        std::string detail; int bad = grammarEngineCases(detail);
        check("grammar engine: pronouns where clear, verbs by tense and person, numbers with their nouns, plurals, a and an, attributes passed up, once, keep, guards, a directive, a path, a person brought in, I and you (H-03)", bad == 0, detail);
    }
    {   // the compiled-in grammar: parsed and linked without an error, the tree's files the same text (E-09: each one read, so a
        // folder moved fails rather than comparing nothing)
        std::shared_ptr<const VgGrammar> G = shardGrammar();
        std::vector<std::string> names; int nf = shardGrammarFiles(&names);
        int same = 0, read = 0; size_t biggest = 0; std::string biggestName;
        for (int i = 0; i < nf; i++) {
            std::ifstream f("src/peoples/grammar/" + names[i]); if (!f) continue;
            read++;
            std::stringstream ss; ss << f.rdbuf(); std::string t = ss.str();
            size_t a = t.find("R\"VG("), e = t.rfind(")VG\"");
            if (a != std::string::npos && e != std::string::npos && t.substr(a + 5, e - a - 5) == shardGrammarSource(i)) same++;
            if (strlen(shardGrammarSource(i)) > biggest) { biggest = strlen(shardGrammarSource(i)); biggestName = names[i]; }
        }
        int alts = (int)G->alts.size(), rules = (int)G->rules.size();
        check("grammar: the records' genres as .vg files, compiled in, linked without an error, the tree's copy the same, each under 16 KB (H-03, H-04)", G->errors.empty() && read == nf && same == read && shardKindCount() >= 60 && biggest < 16000,
              fmt("%d files, %d rules, %d alternatives, %d genres' alternatives; %zu errors%s; %d of %d files in the tree the same as compiled in; the largest %s, %zu bytes", nf, rules, alts, shardKindCount(), G->errors.size(),
                  G->errors.empty() ? "" : (": " + G->errors[0]).c_str(), same, read, biggestName.c_str(), biggest));
    }
    {   // H-04's second pass: every .vg in the tree compiled in (the harness reads a new file from disk before the build lists it);
        // E-09: and every one compiled in found there, so an empty or missing folder fails; E-11: the loader's list, its folders walked
        std::vector<std::string> names; shardGrammarFiles(&names);
        const std::vector<std::string> tree = shardGrammarTree("src/peoples/grammar");
        int missing = 0; std::string first;
        for (const std::string& n : tree) if (std::find(names.begin(), names.end(), n) == names.end()) { missing++; if (first.empty()) first = n; }
        check("grammar: every .vg file in the tree compiled in (H-04's second pass: the harness reads a new one from disk before the build lists it)", missing == 0 && tree.size() == names.size(),
              fmt("%zu files in the tree, %zu compiled in, %d not%s", tree.size(), names.size(), missing, first.empty() ? "" : (" (" + first + ")").c_str()));
    }
    {   // H-04's second pass: each people's names in its own tongue: a held-out name told its people from its letters, every name readable
        const std::vector<NearPeople> near = peoplesNearHome(24);
        std::vector<std::vector<std::string>> all;
        int bad = 0, total = 0; std::string firstBad;
        for (const NearPeople& np : near) {
            Star st; starInSector(np.x, np.y, np.z, st, true); StarSystem sys; sys.generate(st);
            const Body& b = sys.bodies[np.body]; const BodyGen g = BodyGen::make(b);
            const Lore L = loreQuick(sys, b, g, np.which);
            const Tongue& T = tongueOfSeed(L.seed);
            std::vector<std::string> mine;
            for (int i = 0; i < 40; i++) mine.push_back(tongueName(T, mix64(L.seed ^ (0x77A0ULL + i)), TN_PERSON, i & 1));
            for (int i = 0; i < 20; i++) mine.push_back(tongueName(T, mix64(L.seed ^ (0x77B0ULL + i)), TN_HAMLET + i % 5));
            for (const std::string* n : {&L.people, &L.god, &L.river, &L.mountain, &L.sea, &L.city, &L.city2, &L.founder, &L.moon, &L.star, &L.festival}) mine.push_back(*n);
            for (size_t k = 0; k < mine.size(); k++) {
                total++;
                const size_t cap = k < 40 ? 9 : 12;
                if (!tongueReadable(mine[k]) || mine[k].size() < 2 || mine[k].size() > cap) { bad++; if (firstBad.empty()) firstBad = mine[k]; }
            }
            all.push_back(mine);
        }
        const double acc = nameClassify(all);
        check("tongues: each people's names in its own tongue, a held-out name told its people from its letters, every name readable (H-04's second pass)",
              acc >= 65 && bad == 0, fmt("%zu peoples: %.1f%% of held-out names told their people (chance %.1f%%); %d of %d names unreadable or too long%s",
                                         near.size(), acc, 100.0 / std::max<size_t>(1, near.size()), bad, total, firstBad.empty() ? "" : (" (" + firstBad + ")").c_str()));
    }
    {   // H-04's second pass: the everyday in the histories: the fourteen kinds over the test worlds' peoples, each at a place standing, its person alive
        int kinds[HE_COUNT] = {}; int bad = 0, total = 0, events = 0; std::string firstBad;
        for (const GrammarWorld& w : grammarWorlds(8)) {
            Star st; if (!starInSector(w.sx, w.sy, w.sz, st, true)) continue;
            StarSystem sys; sys.generate(st);
            if (w.body >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[w.body]; const BodyGen g = BodyGen::make(b);
            if (!worldHadCivilisation(g) || w.which >= std::max(1, peoplesOf(g))) continue;
            std::shared_ptr<const History> H = historyOf(sys, b, g, w.which);
            if (!H) continue;
            events += (int)H->events.size();
            for (size_t i = 0; i < H->events.size(); i++) {
                const HistoryEvent& e = H->events[i];
                const bool open = e.kind >= HE_FAIR && e.kind <= HE_VISIT, family = e.kind >= HE_ILLNESS && e.kind <= HE_BETROTHAL;
                if (!open && !family) continue;
                kinds[e.kind]++; total++;
                const bool placeOk = e.place < 0 || H->placeStands(e.place, e.year);
                const bool personOk = (e.a < 0 && open) || H->alive(e.a, e.year);
                const bool otherOk = e.b < 0 || H->alive(e.b, e.year);
                if (!placeOk || !personOk || !otherOk || e.pub != open) { bad++; if (firstBad.empty()) firstBad = historyEventLine(*H, (int)i); }
            }
        }
        int present = 0; std::string list;
        for (int k = HE_FAIR; k <= HE_VISIT; k++) { if (kinds[k]) present++; list += fmt("%s %d, ", HISTORY_EVENT_NAMES[k], kinds[k]); }
        for (int k = HE_ILLNESS; k <= HE_BETROTHAL; k++) { if (kinds[k]) present++; list += fmt("%s %d, ", HISTORY_EVENT_NAMES[k], kinds[k]); }
        check("history: the everyday of the quiet years and of the families, the twenty-one kinds over the test worlds' peoples, each at a place standing, its persons alive, the families' private (H-04's second pass)",
              present == (HE_VISIT - HE_FAIR + 1) + (HE_BETROTHAL - HE_ILLNESS + 1) && bad == 0, fmt("%d everyday events of %d (%s); %d out of place%s", total, events, list.c_str(), bad, firstBad.empty() ? "" : (": " + firstBad).c_str()));
    }
    {   // H-04: the records of the test worlds' peoples (the C series' two, the world of two peoples, the six age worlds): every check at zero, every name
        // the history's at the record's year, each age's genres told, one event from several sides, the same twice and alone, the cost
        GrammarRun R;
        std::vector<GrammarWorld> W = grammarWorlds(0);
        for (const GrammarWorld& w : W) grammarPeople(w, R, nullptr, 0);
        int fails = 0; std::string counts;
        for (int k = 0; k < VC_COUNT; k++) { fails += R.counts[k]; if (R.counts[k]) counts += fmt("%s %d, ", VG_CHECK_NAMES[k], R.counts[k]); }
        bool determ = true, alone = true;
        {   // the same twice (a reload of the cache) and one record alone the same as from its catalogue
            Star st; starInSector(151, 0, 25, st, true); StarSystem sys; sys.generate(st); const Body& b = sys.bodies[1]; const BodyGen g = BodyGen::make(b);
            Lore L = loreOf(sys, b, g);
            std::vector<Shard> a, c; shardsOf(sys, b, g, L, L.records, a);
            std::string report; shardGrammarFromDisk("src/peoples/grammar", report);   // a reload: the generation moves on, the texts are written again
            shardsOf(sys, b, g, L, L.records, c);
            for (size_t i = 0; i < a.size(); i++) if (a[i].text != c[i].text) determ = false;
            for (int i : {0, 7, 30, 60, 110}) if (i < (int)a.size()) { Shard x = shardOf(sys, b, g, L, i); if (x.text != a[i].text || x.kind != a[i].kind) alone = false; }
        }
        check("records: every check at zero over every record of the test worlds' peoples, every name the history's at its year (H-04: no anachronism, no unknown place or figure)",
              fails == 0 && R.namesBad == 0 && R.texts >= 600 && determ && alone,
              fmt("%d peoples, %d records, %d texts; %s; %d names written, %d not the history's; %s; %s", R.peoples, R.records, R.texts, fails ? counts.c_str() : "no mark left, no diagnostic, no doubled word, no wrong article, none before a plural, no pronoun against its verb, every sentence capitalised, the spaces clean, every length within its genre's, no anachronism",
                  R.namesTold, R.namesBad, determ ? "the same twice" : "NOT the same twice", alone ? "a record alone the same as in its catalogue" : "a record alone DIFFERS"));
        if (!R.failures.empty()) check("records: the first failure", false, R.failures[0]);
        int ages = 0; for (int a = 0; a < AGE_COUNT; a++) if (R.byAge[a] > 0) ages++;
        int genres = 0; for (const auto& kv : R.genres) if (kv.second.n > 0) genres++;
        double usMean = R.texts ? R.usSum / R.texts : 0;
        check("records: the genres of every age told, one event from several sides, the cost (H-04: a text under 100 us)", ages == AGE_COUNT && genres >= 50 && R.sidesEvents >= 10 && usMean < 100,
              fmt("texts of %d ages, %d genres told on the %d peoples; %d events told in two genres or more; a text %.1f us on average (%.0f at most), a people's texts %.1f ms on average with its history (%.1f at most); %d words a text (%d-%d)",
                  ages, genres, R.peoples, R.sidesEvents, usMean, R.usMax, R.peoples ? R.msSum / R.peoples : 0.0, R.msMax, R.texts ? (int)(R.words / R.texts) : 0, R.minW, R.maxW));
    }
    {   // D-05: the test drifters' peoples' records (the Reimempel, medieval; the Ghaiwuwutxu and the Ainyiny, modern): every check at zero, every name
        // the history's at its year, their own genres told
        GrammarRun R;
        for (const AgeWorld& d : TEST_DRIFTERS) {
            Star st; if (!placeInSector(d.sx, d.sy, d.sz, st, true)) continue;
            StarSystem sys; sys.generate(st); const BodyGen g = BodyGen::make(sys.bodies[0]);
            for (int k = 0; k < std::max(1, peoplesOf(g)); k++) grammarPeople(GrammarWorld{d.sx, d.sy, d.sz, 0, k}, R, nullptr, 0);
        }
        int fails = 0; std::string counts;
        for (int k = 0; k < VC_COUNT; k++) { fails += R.counts[k]; if (R.counts[k]) counts += fmt("%s %d, ", VG_CHECK_NAMES[k], R.counts[k]); }
        static const char* const OWN[] = {"omens", "sunmyth", "sunmeasure", "sunletter", "daycount", "deephalls", "sunlesson", "lastbroadcast"};
        int told = 0; std::string list;
        for (const char* gname : OWN) { auto it = R.genres.find(gname); const int n = it == R.genres.end() ? 0 : it->second.n; if (n) told++; list += fmt("%s%s %d", list.empty() ? "" : ", ", gname, n); }
        check("records of the lost sun: the test drifters' peoples, every check at zero, every name the history's, their own genres told (D-05)",
              R.peoples == 3 && fails == 0 && R.namesBad == 0 && told == (int)(sizeof(OWN) / sizeof(OWN[0])),
              fmt("%d peoples, %d records, %d texts; %s; %d names, %d not the history's; their genres: %s", R.peoples, R.records, R.texts, fails ? counts.c_str() : "every check at zero", R.namesTold, R.namesBad, list.c_str()));
        if (!R.failures.empty()) check("records of the lost sun: the first failure", false, R.failures[0]);
    }
    {   // D-05: no other people's records moved: the age and scene worlds' peoples (their histories, record catalogues and homes, every text, their
        // charts' marks) as D-04 left them (the hash pinned: `build/d05/recdump` showed them the same, byte for byte)
        uint64_t h = 0xD05;
        auto str = [](const std::string& t) { uint64_t x = 1469598103934665603ULL; for (unsigned char c : t) { x ^= c; x *= 1099511628211ULL; } return x; };
        std::set<std::string> seen;
        std::vector<AgeWorld> ws(AGE_WORLDS, AGE_WORLDS + AGE_COUNT); ws.insert(ws.end(), AGE_SCENE_WORLDS, AGE_SCENE_WORLDS + AGE_COUNT);
        int peoples = 0, texts = 0;
        for (const AgeWorld& aw : ws) {
            if (!seen.insert(fmt("%lld %lld %d", (long long)aw.sx, (long long)aw.sz, aw.body)).second) continue;
            Star st; if (!starInSector(aw.sx, 0, aw.sz, st, true)) continue;
            StarSystem sys; sys.generate(st);
            if (aw.body >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[aw.body]; const BodyGen g = BodyGen::make(b);
            if (!worldHadCivilisation(g)) continue;
            for (int k = 0; k < std::max(1, peoplesOf(g)); k++) {
                peoples++;
                std::shared_ptr<const History> H = historyOf(sys, b, g, k);
                if (H) { HistoryEvent* none = nullptr; (void)none; uint64_t hh = 0xD05A15ULL;   // D-04's event numbers (HE_SUN came before the families' kinds)
                         for (const HistoryEvent& e : H->events) hh = mix64(hh ^ (uint64_t)(e.kind > HE_SUN ? e.kind - 1 : e.kind) ^ ((uint64_t)e.year << 20) ^ ((uint64_t)(e.place + 1) << 40) ^ ((uint64_t)(e.a + 1) << 52));
                         h = hashCombine(h, hh); }
                std::shared_ptr<const RecordCatalogue> C = recordsOf(g, k);
                if (C) for (const RecordEntry& e : C->entries) h = hashCombine(h, mix64((uint64_t)e.year * 31 + (uint64_t)e.age * 7 + (uint64_t)e.category * 3 + (uint64_t)e.medium + (uint64_t)e.last * 1000003ULL));
                std::shared_ptr<const RecordHomes> RH = recordHomesOf(g, k);
                if (RH) for (int r : RH->rank) h = hashCombine(h, (uint64_t)(r + 7));
                const Lore L = loreOf(sys, b, g, k);
                for (const ChartMark& m : L.marks) h = hashCombine(h, mix64((uint64_t)m.star.sx * 73856093ULL ^ (uint64_t)m.star.sy * 19349663ULL ^ (uint64_t)m.star.sz * 83492791ULL ^ (uint64_t)(m.body + 2) ^ ((uint64_t)m.how << 40)));
                std::shared_ptr<const ShardTexts> T = shardTextsOf(sys, b, g, L);
                if (T) for (const ShardText& t : T->items) { h = hashCombine(h, str(t.text)); texts++; }
            }
        }
        const uint64_t PINNED = 0x13212ef73adea507ULL;   // D-04's (D-05 left them as they were)
        check("records: no other people's moved by the lost sun, the age and scene worlds' peoples as D-04 left them (D-05)", h == PINNED,
              fmt("%d peoples, %d texts: the hash %016llx (pinned %016llx)", peoples, texts, (unsigned long long)h, (unsigned long long)PINNED));
    }
}

// H-04: fifty records of one people per age, for reading: the age worlds' peoples' first fifty texts (by index, the music and the charts
// left out), each with its year, its genre, its medium, its length, the persons it was cast with, into shots/tests/records_<age>.txt
static void printAgeRecords() {
    std::shared_ptr<const VgGrammar> G = shardGrammar();
    for (int a = 0; a < AGE_COUNT; a++) {
        const AgeWorld& aw = AGE_WORLDS[a];
        Star st; if (!starInSector(aw.sx, 0, aw.sz, st, true)) continue;
        StarSystem sys; sys.generate(st);
        if (aw.body >= (int)sys.bodies.size()) continue;
        const Body& b = sys.bodies[aw.body]; const BodyGen g = BodyGen::make(b);
        Lore L = loreQuick(sys, b, g, aw.which);
        std::shared_ptr<const ShardTexts> T = shardTextsOf(sys, b, g, L);
        std::shared_ptr<const History> H = historyOf(sys, b, g, aw.which);
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, aw.which);
        std::string agename = AGE_NAMES[a]; for (char& c : agename) if (c == ' ') c = '_';
        const std::string path = "shots/tests/records_" + agename + ".txt";
        FILE* f = fopen(path.c_str(), "w"); if (!f) continue;
        fprintf(f, "THE RECORDS OF THE %s (%s), %s\n", L.people.c_str(), AGE_NAMES[a], b.name.c_str());
        if (H) fprintf(f, "%d records in all, their years 1 to %d; the collapse (%s) from year %d. The first fifty texts by their index:\n\n", L.records, H->last, CAUSE_NAMES[std::max(0, std::min(CAUSE_COUNT - 1, H->cause))], H->stageYear[0]);
        int n = 0, words = 0; std::map<std::string, int> byGenre;
        for (size_t i = 0; i < T->items.size() && n < 50; i++) {
            const ShardText& x = T->items[i];
            if (x.text.empty()) continue;
            n++;
            const std::string gname = shardKindName(x.kind);
            byGenre[gname]++;
            const int nw = shardWords(x.text); words += nw;
            std::string who;
            if (H && x.writer >= 0) who += " by " + historyPersonName(*H, x.writer, true);
            if (H && x.reader >= 0) who += " to " + historyPersonName(*H, x.reader, true);
            if (H && x.subject >= 0) who += " of " + historyPersonName(*H, x.subject, true);
            const int medium = C && (int)i < C->count ? C->entries[i].medium : -1;
            fprintf(f, "%d. S%zu, year %d (%s), %s on %s, %d words%s\n", n, i, x.year, AGE_NAMES[std::max(0, std::min(AGE_COUNT - 1, x.age))], gname.c_str(),
                    medium >= 0 ? RECORD_MEDIUM_NAMES[medium] : "?", nw, who.c_str());
            std::string t = x.text; size_t at = 0;
            while ((at = t.find('\n', at)) != std::string::npos) { t.insert(at + 1, "   "); at += 4; }
            fprintf(f, "   %s\n\n", t.c_str());
        }
        fprintf(f, "%d texts, %d words (%d a text); by genre:", n, words, n ? words / n : 0);
        for (const auto& kv : byGenre) fprintf(f, " %s %d,", kv.first.c_str(), kv.second);
        fprintf(f, "\n");
        fclose(f);
        printf("  %-12s the %s of %s: %d texts, %d words a text -> %s\n", AGE_NAMES[a], L.people.c_str(), b.name.c_str(), n, n ? words / n : 0, path.c_str());
    }
}

// H-04: C-02's texts (H-03's `grammar print all` of the same peoples, saved before H-04: one text a line under each people's heading)
// measured as the records are: the repeated frames at a people's first fifty and hundred texts and over all, the sentences written once
struct OldPeople { std::string name; std::vector<std::string> texts; };
static std::vector<OldPeople> readOldTexts(const std::string& path) {
    std::vector<OldPeople> out; std::ifstream f(path); std::string line;
    while (std::getline(f, line)) {
        if (line.compare(0, 3, "== ") == 0) { size_t e = line.find(" ("); out.push_back({line.substr(3, e == std::string::npos ? std::string::npos : e - 3), {}}); continue; }
        if (out.empty() || line.size() < 37 || !std::isdigit((unsigned char)line[3])) continue;
        std::string kind = line.substr(17, 18); if (kind.find("star chart") != std::string::npos) continue;
        out.back().texts.push_back(line.substr(36));
    }
    return out;
}

// H-04's second pass: the peoples' tongues side by side (their sounds, their shapes, their names), and how well a name tells its people:
// each people's persons' and places' names as letter trigrams; a name held out is given to the people whose other names make it likeliest
// (add-one smoothing); the share right against C-02's one generator in three styles, measured the same way
static std::string oldStyleName(int style, uint64_t seed) {   // C-02's `personName` (the generator in three styles, nine letters, one word)
    Rng rng(seed); std::string s;
    for (int t = 0; t < 12; t++) { s = generateName(rng.next(), style); if (s.size() <= 9 && s.find(' ') == std::string::npos) return s; }
    size_t sp = s.find(' '); if (sp != std::string::npos) s = s.substr(0, sp);
    if (s.size() > 9) s = s.substr(0, 9);
    return s;
}
double nameClassify(const std::vector<std::vector<std::string>>& names) {
    const int P = (int)names.size();
    std::vector<std::map<std::string, int>> tri(P); std::vector<int> total(P, 0);
    auto grams = [](const std::string& n) { std::string w = "^" + n + "$"; for (char& c : w) c = (char)std::tolower((unsigned char)c); std::vector<std::string> g; for (size_t i = 0; i + 3 <= w.size(); i++) g.push_back(w.substr(i, 3)); return g; };
    for (int p = 0; p < P; p++) for (const std::string& n : names[p]) for (const std::string& g : grams(n)) { tri[p][g]++; total[p]++; }
    int right = 0, all = 0;
    for (int p = 0; p < P; p++) for (const std::string& n : names[p]) {
        const std::vector<std::string> gs = grams(n);
        int best = -1; double bl = -1e300;
        for (int q = 0; q < P; q++) {
            double l = 0; const int tot = total[q] - (q == p ? (int)gs.size() : 0);
            for (const std::string& g : gs) { auto it = tri[q].find(g); int c = it == tri[q].end() ? 0 : it->second; if (q == p) c--; l += std::log((c + 0.1) / (tot + 0.1 * 4000.0)); }
            if (l > bl) { bl = l; best = q; }
        }
        all++; if (best == p) right++;
    }
    return all ? 100.0 * right / all : 0;
}
int runTongues(int argc, char** argv) {
    const int want = argc > 2 ? atoi(argv[2]) : 24;
    auto join = [](const std::vector<std::string>& v) { std::string r; for (const std::string& x : v) { if (!r.empty()) r += " "; r += x; } return r; };
    const std::vector<NearPeople> near = peoplesNearHome(want);
    std::vector<std::vector<std::string>> fresh, old;
    printf("the tongues of the %zu peoples nearest home (the people, its god, river, mountain, sea, capital, second city, founder, moon, star, festival; then persons, families, places):\n", near.size());
    for (const NearPeople& np : near) {
        Star st; starInSector(np.x, np.y, np.z, st, true); StarSystem sys; sys.generate(st);
        const Body& b = sys.bodies[np.body]; const BodyGen g = BodyGen::make(b);
        const Lore L = loreQuick(sys, b, g, np.which);
        const Tongue& T = tongueOfSeed(L.seed);
        std::vector<std::string> mine, theirs;
        printf("\n%s (%s): the %s; %s consonants [%s], vowels [%s], shapes [%s]%s%s, joined '%s'%s\n", np.world.c_str(), AGE_NAMES[std::max(0, std::min(AGE_COUNT - 1, np.age))], L.people.c_str(),
               T.inventory.c_str(), join(T.cons).c_str(), join(T.vows).c_str(), join(T.shapes).c_str(), T.gem > 0 ? ", doubling" : "",
               T.femEnd.empty() ? "" : (", -" + T.femEnd + " her, -" + (T.mascEnd.empty() ? std::string("") : T.mascEnd) + " him").c_str(), T.joiner.c_str(), T.genericFirst ? ", the generic first" : "");
        printf("  %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s\n", L.people.c_str(), L.god.c_str(), L.river.c_str(), L.mountain.c_str(), L.sea.c_str(), L.city.c_str(), L.city2.c_str(), L.founder.c_str(), L.moon.c_str(), L.star.c_str(), L.festival.c_str());
        std::string persons, families, places;
        for (int i = 0; i < 60; i++) {
            const std::string n = tongueName(T, mix64(L.seed ^ (0x77A0ULL + i)), TN_PERSON, i & 1);
            mine.push_back(n); if (i < 12) persons += (i ? ", " : "") + n;
            theirs.push_back(oldStyleName(L.style, mix64(L.seed ^ (0x77A0ULL + i))));
        }
        for (int i = 0; i < 30; i++) {
            const std::string n = tongueName(T, mix64(L.seed ^ (0x77B0ULL + i)), TN_HAMLET + i % 5);
            mine.push_back(n); if (i < 10) places += (i ? ", " : "") + n;
            theirs.push_back(oldStyleName(L.style, mix64(L.seed ^ (0x77B0ULL + i))));
        }
        for (int i = 0; i < 6; i++) families += (i ? ", " : "") + tongueName(T, mix64(L.seed ^ (0x77C0ULL + i)), TN_FAMILY);
        printf("  persons: %s\n  families: %s\n  places: %s\n", persons.c_str(), families.c_str(), places.c_str());
        printf("  their words: coin %s, bread %s, beast %s, feast %s; the decoder's for \"river\" %s, \"remember\" %s\n", tongueTerm(T, "coin").c_str(), tongueTerm(T, "bread").c_str(),
               tongueTerm(T, "beast").c_str(), tongueTerm(T, "feast").c_str(), tongueWord(T, "river").c_str(), tongueWord(T, "remember").c_str());
        fresh.push_back(mine); old.push_back(theirs);
        {   // the decoder's words: none without a vowel
            static const char* const W[] = {"write", "written", "the", "and", "river", "remember", "year", "king", "house", "mother", "bread", "water", "go", "come", "a", "of", "to", "in", "it", "is"};
            int bad = 0; std::string ex;
            for (const char* w : W) { const std::string t = tongueWord(T, w); bool v = false; for (char c : t) if (strchr("aeiouy", c)) v = true; if (!v) { bad++; ex += " " + std::string(w) + "=" + t; } }
            if (bad) printf("  DECODER WORDS WITHOUT A VOWEL: %d:%s\n", bad, ex.c_str());
        }
    }
    printf("\na held-out name told its people (of %zu) from its letters: %.1f%% (C-02's names, one generator in three styles: %.1f%%; chance %.1f%%)\n", near.size(), nameClassify(fresh), nameClassify(old), 100.0 / std::max<size_t>(1, near.size()));
    return 0;
}

// ---- E-11: `grammar where` (where a rule is defined, its alternatives, what names it, the directive calls the C++ builds its name
// from, its `@instead` lines, the roots that reach it) and `grammar schema` (what a guard, a mark and a directive may name)
namespace {
// the names the C++ builds and expands: genres.cpp's directives, each from its call's argument R (the directive's default when the call
// names none) and a suffix: `-` R itself, `<kind>` an event's key (`.kind`'s words), `<how>` a death's way (`.how`'s, and `illness`);
// `vgRecord` expands `record` (genres.cpp) and `chart` (shards.cpp)
struct BuiltNames { const char* directive; const char* args; int arg; const char* def; const char* suffixes; const char* function; };
const BuiltNames BUILT_NAMES[] = {
    {"news", "<scope> R", 1, "letter", "<kind> rulerdeath none", "news, ruleFor"},
    {"digest", "<scope> R <n>", 1, "news", "<kind> rulerdeath none sep", "digest, ruleFor"},
    {"entries", "R <n>", 0, "annal", "<kind> rulerdeath quiet", "entries, ruleFor"},
    {"event", "<kinds> R <years>", 1, "song", "<kind> rulerdeath none", "event, ruleFor"},
    {"years", "<n> R", 1, "chronicle.year", "- gap", "years"},
    {"past", "<n> R", 1, "history.year", "- open gap close", "past"},
    {"suntable", "<n> R", 1, "suntable.row", "-", "suntable"},
    {"death", "R", 0, "epitaph.by", "<kind> <how> any", "death"},
    {"deed", "R", 0, "epitaph.deed", "<kind> trade", "deed"},
    {"life", "R <n>", 0, "life", "<kind> none", "life"},
};
constexpr int BUILT_COUNT = (int)(sizeof(BUILT_NAMES) / sizeof(BUILT_NAMES[0]));
const char* const CXX_ROOTS[][2] = {{"record", "genres.cpp: a record"}, {"chart", "shards.cpp: a chart's caption"}};

std::vector<std::string> wordsOf(const std::string& s) { std::vector<std::string> w; std::istringstream in(s); std::string x; while (in >> x) w.push_back(x); return w; }
const std::vector<std::string>& schemaWords(const char* name) {
    static const std::vector<std::string> none;
    for (const RecordKeyTable& t : recordSchemaKeys()) if (std::strcmp(t.name, name) == 0) return t.keys;
    return none;
}
std::string patternOf(const BuiltNames& B) {
    std::string p; for (const std::string& x : wordsOf(B.suffixes)) p += (p.empty() ? "" : " ") + (x == "-" ? std::string("R") : "R." + x);
    return p;
}
bool rootCovers(const std::string& root, const std::string& rule) {
    if (root.size() > 2 && root.compare(root.size() - 2, 2, ".*") == 0) return rule.compare(0, root.size() - 1, root, 0, root.size() - 1) == 0;
    return root == rule;
}
// a list printed in lines of at most 140 characters, its first after `head`, the rest indented
void printList(const std::string& head, const std::vector<std::string>& items) {
    std::string line = head;
    for (const std::string& s : items) {
        if (line.size() + 1 + s.size() > 140 && line.size() > 4) { printf("%s\n", line.c_str()); line = "    "; }
        line += (line == "    " || line.empty() ? "" : " ") + s;
    }
    printf("%s\n", line.c_str());
}

// the grammar read once and indexed: each file's lines, and each rule's alternatives that name it, directive calls that build its name,
// `@instead` lines and the roots that reach it (through every name, built name and stand-in)
struct GrammarIndex {
    std::shared_ptr<const VgGrammar> G;
    std::vector<std::vector<std::string>> lines;      // each file's lines, numbered as the parse numbers them
    struct Build { int alt, conv; std::string call; };
    std::vector<std::vector<int>> callers, insteads;
    std::vector<std::vector<Build>> builds;
    std::vector<std::string> roots, rootAt;           // the `@root` entries in the parse's order, each one's file:line
    std::vector<std::vector<int>> reachedFrom;        // a rule's roots, by index
    std::vector<int> emptyCalls;                      // the alternatives with a directive call that builds no rule
    std::string at(int file, int line) const { return (file >= 0 && file < (int)G->files.size() ? G->files[file] : std::string("?")) + ":" + std::to_string(line); }
    std::string text(int file, int line) const {
        if (file < 0 || file >= (int)lines.size() || line < 1 || line > (int)lines[file].size()) return "";
        const std::string& s = lines[file][line - 1]; const size_t a = s.find_first_not_of(" \t");
        return a == std::string::npos ? "" : s.substr(a);
    }
};
std::vector<int> builtRules(const VgGrammar& G, const VgNode& n, int& conv) {
    std::vector<int> out; conv = -1;
    for (int c = 0; c < BUILT_COUNT; c++) if (n.s == BUILT_NAMES[c].directive) conv = c;
    if (conv < 0) return out;
    const BuiltNames& B = BUILT_NAMES[conv];
    const std::vector<std::string> a = wordsOf(n.a);
    const std::string R = (int)a.size() > B.arg ? a[B.arg] : std::string(B.def);
    std::vector<std::string> names;
    for (const std::string& s : wordsOf(B.suffixes)) {
        if (s == "-") names.push_back(R);
        else if (s == "<kind>") { for (const std::string& k : schemaWords(".kind")) names.push_back(R + "." + k); }
        else if (s == "<how>") { for (const std::string& k : schemaWords(".how")) names.push_back(R + "." + k); names.push_back(R + ".illness"); }
        else names.push_back(R + "." + s);
    }
    for (const std::string& nm : names) { const int r = G.rule(nm); if (r >= 0 && std::find(out.begin(), out.end(), r) == out.end()) out.push_back(r); }
    return out;
}
GrammarIndex grammarIndex(const std::string& dir, bool embedded) {
    GrammarIndex X;
    X.G = shardGrammar();
    const VgGrammar& G = *X.G;
    for (size_t f = 0; f < G.files.size(); f++) {
        std::string t;
        if (embedded) t = shardGrammarSource((int)f);
        else { std::ifstream in(dir + "/" + G.files[f]); std::stringstream ss; ss << in.rdbuf(); t = ss.str(); }
        std::vector<std::string> L; size_t p = 0;
        while (p <= t.size()) {
            size_t q = t.find('\n', p); if (q == std::string::npos) q = t.size();
            std::string s = t.substr(p, q - p); if (!s.empty() && s.back() == '\r') s.pop_back();
            L.push_back(s); p = q + 1;
        }
        X.lines.push_back(L);
        for (size_t i = 0; i < L.size(); i++) {
            if (L[i].compare(0, 6, "@root ") != 0 && L[i].compare(0, 6, "@root\t") != 0) continue;
            const std::vector<std::string> w = wordsOf(L[i].substr(1));
            for (size_t k = 1; k < w.size(); k++) { X.roots.push_back(w[k]); X.rootAt.push_back(X.at((int)f, (int)i + 1)); }
        }
    }
    const int nr = (int)G.rules.size();
    X.callers.assign(nr, {}); X.insteads.assign(nr, {}); X.builds.assign(nr, {}); X.reachedFrom.assign(nr, {});
    std::vector<std::vector<int>> next(nr);   // a rule's: the rules its alternatives name or build, its stand-ins
    std::function<void(const std::vector<VgNode>&, int)> walk = [&](const std::vector<VgNode>& body, int ai) {
        const int from = G.alts[ai].rule;
        for (const VgNode& n : body) {
            if (n.type == VgNode::CHOICE) for (const auto& o : n.opts) walk(o, ai);
            int r = -1;
            if (n.type == VgNode::RULE || n.type == VgNode::PLURAL) r = n.ref;
            if (n.type == VgNode::NUM && !n.s.empty() && n.s[0] != '$' && n.s[0] != '.' && n.fieldRef < 0) r = n.ref;
            if (r >= 0) { if (X.callers[r].empty() || X.callers[r].back() != ai) X.callers[r].push_back(ai); next[from].push_back(r); }
            if (n.type == VgNode::DIRECT) {
                int conv = -1; const std::vector<int> b = builtRules(G, n, conv);
                if (conv >= 0 && b.empty()) X.emptyCalls.push_back(ai);
                for (int x : b) {
                    bool dup = false; for (const GrammarIndex::Build& o : X.builds[x]) if (o.alt == ai && o.conv == conv) dup = true;
                    if (!dup) X.builds[x].push_back({ai, conv, "{@" + n.s + (n.a.empty() ? "" : " " + n.a) + "}"});
                    next[from].push_back(x);
                }
            }
        }
    };
    for (int ai = 0; ai < (int)G.alts.size(); ai++) walk(G.alts[ai].body, ai);
    for (int i = 0; i < (int)G.insteads.size(); i++) {
        const VgInstead& I = G.insteads[i];
        if (I.fromRule >= 0) X.insteads[I.fromRule].push_back(i);
        if (I.toRule >= 0 && I.toRule != I.fromRule) X.insteads[I.toRule].push_back(i);
        if (I.fromRule >= 0 && I.toRule >= 0) next[I.fromRule].push_back(I.toRule);
    }
    for (int k = 0; k < (int)X.roots.size(); k++) {
        std::vector<char> seen(nr, 0); std::vector<int> stack;
        for (int r = 0; r < nr; r++) if (rootCovers(X.roots[k], G.rules[r].name)) { seen[r] = 1; stack.push_back(r); }
        while (!stack.empty()) { const int x = stack.back(); stack.pop_back(); for (int y : next[x]) if (!seen[y]) { seen[y] = 1; stack.push_back(y); } }
        for (int r = 0; r < nr; r++) if (seen[r]) X.reachedFrom[r].push_back(k);
    }
    return X;
}

void whereRule(const GrammarIndex& X, int r) {
    const VgGrammar& G = *X.G; const VgRule& R = G.rules[r];
    std::string opts;
    if (R.number == VN_SING) opts += " sing"; else if (R.number == VN_PLURAL) opts += " pl"; else if (R.number == VN_MASS) opts += " mass";
    if (R.keep) opts += " keep";
    if (R.steady) opts += " steady";
    if (R.formula) opts += " formula";
    if (R.wordsHi > 0) opts += fmt(" words=%d..%d", R.wordsLo, R.wordsHi);
    if (R.peopleK > 0) opts += fmt(" people=%d", R.peopleK);
    if (R.share > 0) opts += fmt(" share=%g", R.share);
    printf("%s  %s%s  %zu alternative%s\n", R.name.c_str(), X.at(R.file, R.line).c_str(), opts.empty() ? "" : ("  (" + opts.substr(1) + ")").c_str(), R.alts.size(), R.alts.size() == 1 ? "" : "s");
    for (int ai : R.alts) printf("  %s  %s\n", X.at(G.alts[ai].file, G.alts[ai].line).c_str(), X.text(G.alts[ai].file, G.alts[ai].line).c_str());
    printf("named by %zu alternative%s%s\n", X.callers[r].size(), X.callers[r].size() == 1 ? "" : "s", X.callers[r].empty() ? "" : ":");
    for (int ai : X.callers[r]) printf("  %s  in %s\n", X.at(G.alts[ai].file, G.alts[ai].line).c_str(), G.rules[G.alts[ai].rule].name.c_str());
    if (!X.builds[r].empty()) {
        printf("its name built by the C++ (genres.cpp) from %zu directive call%s:\n", X.builds[r].size(), X.builds[r].size() == 1 ? "" : "s");
        for (const GrammarIndex::Build& b : X.builds[r]) {
            const BuiltNames& B = BUILT_NAMES[b.conv];
            printf("  %s  in %s: %s  (%s: %s)\n", X.at(G.alts[b.alt].file, G.alts[b.alt].line).c_str(), G.rules[G.alts[b.alt].rule].name.c_str(), b.call.c_str(), B.function,
                   patternOf(B).c_str());
        }
    }
    for (int i : X.insteads[r]) {
        const VgInstead& I = G.insteads[i];
        printf("@instead  %s  %s\n", X.at(I.file, I.line).c_str(), X.text(I.file, I.line).c_str());
    }
    for (int k = 0; k < (int)X.roots.size(); k++) if (rootCovers(X.roots[k], R.name)) printf("a root: @root %s  %s\n", X.roots[k].c_str(), X.rootAt[k].c_str());
    for (const auto& c : CXX_ROOTS) if (R.name == c[0]) printf("expanded by the C++: %s (`vgRecord`)\n", c[1]);
    std::vector<std::string> from;   // a root declared in two files (`med.chr.*`) once
    for (int k : X.reachedFrom[r]) if (std::find(from.begin(), from.end(), X.roots[k]) == from.end()) from.push_back(X.roots[k]);
    printList(fmt("reached from %zu root%s:", from.size(), from.size() == 1 ? "" : "s"), from);
}

void wherePrefix(const GrammarIndex& X, const std::string& query) {
    const VgGrammar& G = *X.G;
    const std::string pre = query.substr(0, query.size() - 1);   // `ann.sg.*`: the rules whose names start `ann.sg.`
    std::vector<int> hit; std::map<std::string, int> files; std::vector<std::string> fileOrder;
    for (int r = 0; r < (int)G.rules.size(); r++) {
        if (G.rules[r].name.compare(0, pre.size(), pre) != 0) continue;
        hit.push_back(r);
        if (!files.count(G.files[G.rules[r].file])) fileOrder.push_back(G.files[G.rules[r].file]);
        files[G.files[G.rules[r].file]]++;
    }
    std::vector<std::string> fl; for (const std::string& f : fileOrder) fl.push_back(fmt("%s (%d)", f.c_str(), files[f]));
    printList(fmt("%s: %zu rule%s in", query.c_str(), hit.size(), hit.size() == 1 ? "" : "s"), fl);
    std::map<std::string, int> calls;
    for (int r : hit) {
        int other = 0; for (int ai : X.callers[r]) if (G.alts[ai].file != G.rules[r].file) other++;
        printf("  %-28s %s  %zu alternative%s; named by %zu (%d from another file)%s\n", G.rules[r].name.c_str(), X.at(G.rules[r].file, G.rules[r].line).c_str(),
               G.rules[r].alts.size(), G.rules[r].alts.size() == 1 ? "" : "s", X.callers[r].size(), other, X.builds[r].empty() ? "" : fmt("; built by %zu calls", X.builds[r].size()).c_str());
        for (const GrammarIndex::Build& b : X.builds[r]) calls[b.call]++;
    }
    std::vector<std::string> cl; for (const auto& kv : calls) cl.push_back(fmt("%s (%d)", kv.first.c_str(), kv.second));
    if (!cl.empty()) printList("the directive calls whose names the C++ builds here (genres.cpp; `grammar where` for their patterns):", cl);
    std::vector<std::string> rl; for (int k = 0; k < (int)X.roots.size(); k++) for (int r : hit) if (rootCovers(X.roots[k], G.rules[r].name)) { rl.push_back("@root " + X.roots[k] + " " + X.rootAt[k]); break; }
    if (!rl.empty()) printList("its roots:", rl);
}

void whereConventions() {
    printf("the names the C++ builds (genres.cpp's directives, from a call's R: its argument, the default when it names none; <kind> an event's\n"
           "key, <how> a death's way: `grammar schema`'s .kind and .how):\n");
    for (const BuiltNames& B : BUILT_NAMES)
        printf("  {@%s %s}%*s%s   (R by default %s; %s)\n", B.directive, B.args, std::max(1, 26 - (int)strlen(B.directive) - (int)strlen(B.args)), "", patternOf(B).c_str(), B.def, B.function);
    for (const auto& c : CXX_ROOTS) printf("  vgRecord: %s (%s)\n", c[0], c[1]);
}

// the rules a known issue owns (O6-01: printed FAIL(<id>), not counted), by their names' start
const char* const WHERE_OWNED[][2] = {{"picture.", "KI-424"}};   // the picture register: no call expands it
int whereCheck(const GrammarIndex& X) {
    const VgGrammar& G = *X.G; const int nr = (int)G.rules.size();
    int byMark = 0, cross = 0, built = 0, bad = 0; std::string first; std::map<std::string, std::vector<std::string>> owned;
    for (int r = 0; r < nr; r++) {
        bool mark = false, other = false;
        for (int ai : X.callers[r]) if (G.alts[ai].file != G.rules[r].file) mark = true;
        for (int i : X.insteads[r]) if (G.insteads[i].file != G.rules[r].file) other = true;
        byMark += mark; cross += mark || other;
        bool root = false; for (const auto& c : CXX_ROOTS) if (G.rules[r].name == c[0]) root = true;
        if (!X.builds[r].empty() || root) built++;
        bool stand = false; for (int i : X.insteads[r]) if (G.insteads[i].toRule == r) stand = true;
        if ((X.callers[r].empty() && X.builds[r].empty() && !stand && !root) || X.reachedFrom[r].empty()) {
            const char* id = nullptr; for (const auto& o : WHERE_OWNED) if (G.rules[r].name.compare(0, strlen(o[0]), o[0]) == 0) id = o[1];
            if (id) { owned[id].push_back(G.rules[r].name); continue; }
            bad++; if (first.size() < 200) first += " " + G.rules[r].name;
        }
    }
    std::string empties; for (int ai : X.emptyCalls) if (empties.size() < 200) empties += " " + X.at(G.alts[ai].file, G.alts[ai].line);
    printf("grammar where check: %zu files, %d rules; %d named from another file than their own (%d by a mark, the rest by an @instead line), "
           "%d built by the C++ (the %d directives' names and vgRecord's roots), each found with its place and what names or builds it; "
           "%d that nothing names, builds or stands in for%s; %zu directive calls that build no rule%s\n",
           G.files.size(), nr, cross, byMark, built, BUILT_COUNT, bad, first.c_str(), X.emptyCalls.size(), empties.c_str());
    for (const auto& kv : owned) printList(fmt("  FAIL(%s) %zu rules nothing names or builds, not counted:", kv.first.c_str(), kv.second.size()), kv.second);
    return bad || !X.emptyCalls.empty() ? 1 : 0;
}

// the page `docs/reference/civilisations/grammar-files.md` (`make check` holds it to `grammar where md`)
void whereMd(const GrammarIndex& X) {
    const VgGrammar& G = *X.G;
    std::vector<std::string> shelves; std::map<std::string, std::vector<std::string>> byShelf;
    std::vector<std::set<std::string>> genres(G.files.size()), prefixes(G.files.size()), words(G.files.size());
    std::map<std::string, std::vector<int>> prefixFiles;
    for (const VgRule& R : G.rules) {
        const size_t d = R.name.find('.');
        if (d == std::string::npos) { words[R.file].insert(R.name); continue; }
        const std::string p = R.name.substr(0, d);
        if (p == "genre") { genres[R.file].insert(R.name.substr(d + 1)); continue; }
        prefixes[R.file].insert(p);
        std::vector<int>& pf = prefixFiles[p]; if (std::find(pf.begin(), pf.end(), R.file) == pf.end()) pf.push_back(R.file);
    }
    for (const std::string& n : G.files) {
        const size_t s = n.rfind('/'); const std::string shelf = s == std::string::npos ? "" : n.substr(0, s + 1);
        if (!byShelf.count(shelf)) shelves.push_back(shelf);
        byShelf[shelf].push_back(n);
    }
    auto codes = [](const std::set<std::string>& s) { std::string o; for (const std::string& x : s) o += (o.empty() ? "`" : ", `") + x + "`"; return o; };
    printf("# The records' grammar: its files and rule prefixes\n\n"
           "Generated by `vesperis_test grammar where md` from `src/peoples/grammar/` (`make check` holds it to the tree). The layout and the\n"
           "rules are that folder's `CLAUDE.md`; a rule's place, its alternatives, what names it and what builds its name: `vesperis_test\n"
           "grammar where <rule|prefix.*>`. A rule's prefix is its name before the first dot; `genre.<name>` is a genre, and a name without a\n"
           "dot is a rule of its own (a word list: `crop`).\n\n## The files by shelf\n\n");
    for (const std::string& shelf : shelves) {
        std::vector<std::string> fs = byShelf[shelf]; std::sort(fs.begin(), fs.end());
        printf("### `%s` (%zu)\n\n| File | Genres | Prefixes | Rules without a dot |\n| --- | --- | --- | --- |\n", shelf.empty() ? "./" : shelf.c_str(), fs.size());
        for (const std::string& n : fs) {
            const int f = (int)(std::find(G.files.begin(), G.files.end(), n) - G.files.begin());
            printf("| `%s` | %s | %s | %s |\n", n.substr(shelf.size()).c_str(), codes(genres[f]).c_str(), codes(prefixes[f]).c_str(), codes(words[f]).c_str());
        }
        printf("\n");
    }
    printf("## The prefixes\n\n| Prefix | Files |\n| --- | --- |\n");
    for (const auto& kv : prefixFiles) {
        std::string o; for (int f : kv.second) o += (o.empty() ? "`" : ", `") + G.files[f] + "`";
        printf("| `%s` | %s |\n", kv.first.c_str(), o.c_str());
    }
}
}   // namespace

// `grammar where [<rule> | <prefix>.* | check | md]`: the tree's grammar (or the compiled-in one, `embedded`) indexed
static int grammarWhere(int argc, char** argv, const std::string& dir, bool embedded) {
    if (!embedded) {
        std::string report;
        if (!shardGrammarFromDisk(dir, report)) { printf("the grammar from %s/: ERRORS\n%s", dir.c_str(), report.c_str()); return 1; }
    }
    const GrammarIndex X = grammarIndex(dir, embedded);
    const VgGrammar& G = *X.G;
    const std::string q = argc > 0 ? argv[0] : "";
    if (q == "md") { whereMd(X); return 0; }
    if (q == "check") return whereCheck(X);
    if (q.empty()) {
        printf("grammar where <rule> | <prefix>.* | check | md: a rule's definition and alternatives, the alternatives that name it, the directive\n"
               "calls whose names the C++ builds into it, its @instead lines and the roots that reach it; a prefix's rules; the check that\n"
               "every rule is named, built or stood in for; docs/reference/civilisations/grammar-files.md\n");
        whereConventions();
        return 0;
    }
    if (q.size() > 2 && q.compare(q.size() - 2, 2, ".*") == 0) { wherePrefix(X, q); return 0; }
    const int r = G.rule(q);
    if (r < 0) {
        std::vector<std::string> near; for (const VgRule& R : G.rules) if (R.name.find(q) != std::string::npos && near.size() < 24) near.push_back(R.name);
        printList(fmt("no rule %s%s", q.c_str(), near.empty() ? "" : "; rules with it in their names:"), near);
        return 1;
    }
    whereRule(X, r);
    return 0;
}

// `grammar schema`: what the records' grammar may name (genres_schema.h): the facts, the entities, their paths and fields, the directives
// and the names they build, and the words each fact and field takes (`recordSchemaKeys`)
static int grammarSchema() {
    const VgSchema& S = shardSchema();
    std::vector<std::string> facts, inv, rest;   // the schema's facts: its own, then the inventions', then the institutions' and the traits'
    for (const std::string& f : S.facts) {
        const bool name = std::find(S.names.begin(), S.names.end(), f) != S.names.end();
        if (f.compare(0, 4, "inv.") == 0) inv.push_back(f.substr(4));
        else if (!inv.empty()) rest.push_back(f);
        else facts.push_back(name ? f + "*" : f);
    }
    printList(fmt("facts (%zu; a guard's key, a mark's {fact}; * a name the decoder keeps):", facts.size()), facts);
    printList(fmt("the inventions (%zu, each a fact while it exists: inv.railway):", inv.size()), inv);
    printList("the institutions and the world's traits (each a fact):", rest);
    printList(fmt("entities (%zu):", S.entities.size()), S.entities);
    printList(fmt("paths (%zu: an entity's entities, $e.a, $x.mother):", S.paths.size()), S.paths);
    printList(fmt("fields (%zu: an entity's words, $e.kind, $x.role):", S.fields.size()), S.fields);
    printList(fmt("directives (%zu: {@news public news}):", S.directives.size()), S.directives);
    whereConventions();
    printf("the words a fact or a field takes:\n");
    for (const RecordKeyTable& t : recordSchemaKeys()) printList(fmt("  %s:", t.name), t.keys);
    return 0;
}

// `grammar [dir D] [embedded] [near N] [print N|all]`: the records' grammar read from the tree (peoples/grammar/: an edited rule is
// reprinted without a rebuild), every people's texts of the test worlds and the peoples nearest home through every check, the texts
// in `shots/tests/grammar_<world>.txt`, the measures, the rule use and the cost; `check`, `where`, `schema` (above)
int runGrammar(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    std::string dir = "src/peoples/grammar", compareWith, textsOut = "shots/tests/grammar_texts.txt"; bool embedded = false, ages = false, rogues = false; int near = 40, print = 2, late = 0;
    double roguesLy = 3000;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "dir" && i + 1 < argc) dir = argv[++i];
        else if (a == "embedded") embedded = true;
        else if (a == "near" && i + 1 < argc) near = atoi(argv[++i]);
        else if (a == "print" && i + 1 < argc) { std::string v = argv[++i]; print = v == "all" ? -1 : atoi(v.c_str()); }
        else if (a == "ages") ages = true;
        else if (a == "rogues") rogues = true;   // D-05: the drifters' peoples within 3,000 ly of home besides
        else if (a.compare(0, 7, "rogues=") == 0) { rogues = true; roguesLy = atof(a.c_str() + 7); }   // D-05: within that many
        else if (a == "out" && i + 1 < argc) textsOut = argv[++i];   // H-04's second pass: the texts' file (writers working side by side)
        else if (a == "late" && i + 1 < argc) late = atoi(argv[++i]);   // H-04's second pass: the nearest peoples of the modern age and later besides (the measure at 100 texts)
        else if (a == "compare" && i + 1 < argc) compareWith = argv[++i];
        else if (a == "check") {   // the unit's H-03 checks alone
            int failures = 0;
            grammarChecks(modeCheck(failures));
            printf("grammar check: %d failure%s\n", failures, failures == 1 ? "" : "s");
            return failures ? 1 : 0;
        }
        else if (a == "where") return grammarWhere(argc - i - 1, argv + i + 1, dir, embedded);   // E-11
        else if (a == "schema") return grammarSchema();
    }
    if (!embedded) {
        std::string report;
        bool ok = shardGrammarFromDisk(dir, report);
        printf("the grammar from %s/: %s\n%s", dir.c_str(), ok ? "read and linked" : "ERRORS", report.c_str());
    } else printf("the grammar compiled in\n");
    std::shared_ptr<const VgGrammar> G = shardGrammar();
    {
        int used = 0; for (const VgRule& r : G->rules) if (r.used) used++;
        printf("%zu files, %zu rules (%d reached), %zu alternatives, %d kinds, %zu lexicon words; parsed in %.2f ms; %zu errors\n", G->files.size(), G->rules.size(), used, G->alts.size(),
               shardKindCount(), G->lexicon.size(), G->parseMs, G->errors.size());
    }
    GrammarRun R;
    double t0 = nowSec();
    std::vector<GrammarWorld> W = grammarWorlds(near);
    if (late > 0) {   // H-04's second pass: the late peoples (a hundred texts and more each) nearest home besides
        int added = 0;
        for (const NearPeople& p : peoplesNearHome(4000)) {
            if (added >= late) break;
            if (p.age < AGE_MODERN) continue;
            bool dup = false; for (const GrammarWorld& x : W) if (x.sx == p.x && x.sy == p.y && x.sz == p.z && x.body == p.body && x.which == p.which) dup = true;
            if (!dup) { W.push_back({p.x, p.y, p.z, p.body, p.which}); added++; }
        }
    }
    if (rogues) for (const Star& d : drifterPeoplesNearHome(roguesLy)) {   // D-05
        StarSystem ds; ds.generate(d);
        const int np = std::max(1, peoplesOf(BodyGen::make(ds.bodies[0])));
        for (int k = 0; k < np; k++) W.push_back({d.sx, d.sy, d.sz, 0, k});
    }
    printf("the peoples (%zu: the test worlds, the age worlds, the nearest %d to home%s%s):\n", W.size(), near, late > 0 ? fmt(", the nearest %d of the modern age and later", late).c_str() : "", rogues ? ", the drifters' peoples" : "");
    FILE* out = fopen(textsOut.c_str(), "w");
    for (const GrammarWorld& w : W) grammarPeople(w, R, out, print);
    if (out) fclose(out);
    int fails = 0; for (int k = 0; k < VC_COUNT; k++) fails += R.counts[k];
    fails += R.namesBad;
    printf("%d peoples, %d records, %d texts and %d star charts' captions, %ld words (every text in %s) in %.1f s\n", R.peoples, R.records, R.texts, R.charts, R.words, textsOut.c_str(), nowSec() - t0);
    printf("  the texts by age:"); for (int a = 0; a < AGE_COUNT; a++) printf(" %s %d%s", AGE_NAMES[a], R.byAge[a], a + 1 < AGE_COUNT ? "," : "\n");
    printf("the checks over the %d texts and captions:", R.texts + R.charts);
    for (int k = 0; k < VC_COUNT; k++) printf("%s %s %d", k ? "," : "", VG_CHECK_NAMES[k], R.counts[k]);
    printf("; names not the history's at the record's year %d of %d\n", R.namesBad, R.namesTold);
    for (const std::string& f : R.failures) printf("  FAIL %s\n", f.c_str());
    printf("the measures (the genres' formulas left out): sentences of six words or more on a repeated frame (names and numbers masked): %.1f%% at a people's first 50 texts (%d peoples; %.1f%% of them written once word for word), %.1f%% at 100 (%d), %.1f%% over a people's all; distinct 2/3-grams %.2f %.2f\n",
           R.n50 ? R.rep50 / R.n50 : 0.0, R.n50, R.n50 ? R.dist50 / R.n50 : 0.0, R.n100 ? R.rep100 / R.n100 : 0.0, R.n100, R.peoples ? R.repAll / R.peoples : 0.0,
           R.peoples ? R.d2sum / R.peoples : 0.0, R.peoples ? R.d3sum / R.peoples : 0.0);
    {   // H-04's second pass: how much of a people's writing another people wrote too
        std::unordered_map<std::string, std::set<int>> owners;
        for (size_t p = 0; p < R.peopleFrames.size(); p++) for (const std::string& f : R.peopleFrames[p]) owners[f].insert((int)p);
        double shared = 0; int n = 0; long allS = 0, allShared = 0;
        for (size_t p = 0; p < R.peopleFrames.size(); p++) {
            if (R.peopleFrames[p].empty()) continue;
            int sh = 0; for (const std::string& f : R.peopleFrames[p]) if (owners[f].size() > 1) sh++;
            shared += 100.0 * sh / R.peopleFrames[p].size(); n++; allS += (long)R.peopleFrames[p].size(); allShared += sh;
        }
        printf("one people against the others: %.1f%% of a people's sentences of six words or more (its first hundred texts) are on a frame another people wrote too (%ld of %ld); the engine drew %d rules again for a repeated frame\n",
               n ? shared / n : 0.0, allShared, allS, R.retries);
        std::vector<std::pair<int, std::string>> wide;
        for (const auto& kv : owners) if (kv.second.size() >= 3) wide.push_back({(int)kv.second.size(), kv.first});
        std::sort(wide.begin(), wide.end(), [](const std::pair<int, std::string>& a, const std::pair<int, std::string>& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
        printf("  the frames the most peoples wrote (of %d):\n", (int)R.peopleFrames.size());
        for (size_t i = 0; i < wide.size() && i < 24; i++) printf("    %2d peoples  %s\n", wide[i].first, wide[i].second.c_str());
    }
    {
        std::vector<const std::vector<std::string>*> f; for (const auto& x : R.sampleForms) f.push_back(&x);
        VgMeasure m = vgMeasure(R.sampleTexts, &f);
        printf("  the first people's (%d texts): distinct 2/3-grams over the first 1,500 words %.2f %.2f; the most repeated frames:\n", m.texts, m.d2, m.d3);
        std::vector<std::pair<int, std::string>> top; vgTopFrames(R.sampleTexts, 8, top, &f);
        for (const auto& t : top) printf("    x%d  %s\n", t.first, t.second.c_str());
    }
    {   // H-04: the genres: how many, their lengths, their own repetition over every people's
        printf("the genres (records, words: mean, shortest, longest; repeated frames within the genre over every people's; H-04's second pass: the share of its\n"
               "sentences on a frame two peoples or more wrote in it, and within one people the share on a frame that people wrote twice):\n");
        for (const auto& kv : R.genres) {
            const GrammarRun::Genre& gr = kv.second;
            std::vector<const std::vector<std::string>*> f; for (const auto& x : gr.forms) f.push_back(&x);
            VgMeasure m = vgMeasure(gr.texts, &f);
            std::unordered_map<std::string, std::set<int>> own; std::map<std::pair<int, std::string>, int> perPeople; int longs = 0, sharedS = 0, twice = 0;
            for (size_t t = 0; t < gr.texts.size(); t++) for (const std::string& sn : vgSentences(gr.texts[t])) {
                if (vgWords(sn) < 6) continue; const std::string fr = vgFrame(sn);
                if (std::find(gr.forms[t].begin(), gr.forms[t].end(), fr) != gr.forms[t].end()) continue;
                own[fr].insert(gr.peopleOf[t]); perPeople[{gr.peopleOf[t], fr}]++;
            }
            for (size_t t = 0; t < gr.texts.size(); t++) for (const std::string& sn : vgSentences(gr.texts[t])) {
                if (vgWords(sn) < 6) continue; const std::string fr = vgFrame(sn);
                if (std::find(gr.forms[t].begin(), gr.forms[t].end(), fr) != gr.forms[t].end()) continue;
                longs++; if (own[fr].size() > 1) sharedS++; if (perPeople[{gr.peopleOf[t], fr}] > 1) twice++;
            }
            printf("  %-16s %5d  %3ld words (%d-%d)  %5.1f%% of %d; between peoples %5.1f%%, within one %5.1f%%\n", kv.first.c_str(), gr.n, gr.n ? gr.words / gr.n : 0, gr.minW, gr.maxW,
                   m.longS ? 100.0 * m.longRep / m.longS : 0.0, m.longS, longs ? 100.0 * sharedS / longs : 0.0, longs ? 100.0 * twice / longs : 0.0);
        }
    }
    printf("one event from several sides: %d events told in two genres or more (%d tellings)\n", R.sidesEvents, R.sides);
    for (const std::string& x : R.sidesSample) printf("  %s\n", x.c_str());
    {
        int drawn = 0; std::vector<std::pair<int, int>> order;
        for (size_t a = 0; a < R.altUses.size(); a++) { if (R.altUses[a]) drawn++; order.push_back({R.altUses[a], (int)a}); }
        std::sort(order.begin(), order.end(), [](const std::pair<int, int>& x, const std::pair<int, int>& y) { return x.first != y.first ? x.first > y.first : x.second < y.second; });
        printf("the rule use: %d of %zu alternatives drawn; the most drawn:", drawn, G->alts.size());
        for (int i = 0; i < 8 && i < (int)order.size(); i++) printf(" %s x%d", G->where(order[i].second).c_str(), order[i].first);
        printf("\n  never drawn:");
        int shown = 0; for (size_t a = 0; a < R.altUses.size() && shown < 16; a++) if (!R.altUses[a]) { printf(" %s", G->where((int)a).c_str()); shown++; }
        printf("%s\n", shown == 0 ? " none" : "");
    }
    if (ages) { printf("fifty records of one people per age, for reading:\n"); printAgeRecords(); }
    if (!compareWith.empty()) {   // H-04: against C-02's texts of the same peoples
        std::vector<OldPeople> old = readOldTexts(compareWith);
        double r50 = 0, r100 = 0, rAll = 0, d50 = 0, w = 0, g2 = 0, g3 = 0; int n50 = 0, n100 = 0, nAll = 0, texts = 0;
        for (const OldPeople& p : old) {
            if (p.texts.empty()) continue;
            auto m = [&](size_t n) { std::vector<std::string> t(p.texts.begin(), p.texts.begin() + std::min(n, p.texts.size())); return vgMeasure(t); };
            if (p.texts.size() >= 50) { VgMeasure x = m(50); r50 += x.longS ? 100.0 * x.longRep / x.longS : 0; d50 += x.longS ? 100.0 * x.distinct / x.longS : 0; n50++; }
            if (p.texts.size() >= 100) { VgMeasure x = m(100); r100 += x.longS ? 100.0 * x.longRep / x.longS : 0; n100++; }
            VgMeasure x = m(p.texts.size()); rAll += x.longS ? 100.0 * x.longRep / x.longS : 0; nAll++; texts += x.texts; w += x.words; g2 += x.d2; g3 += x.d3;
        }
        printf("C-02's texts (%s): %d peoples, %d texts, %.0f words a text; repeated frames %.1f%% at 50 (%d peoples; %.1f%% written once word for word), %.1f%% at 100 (%d), %.1f%% over all; distinct 2/3-grams %.2f %.2f\n",
               compareWith.c_str(), nAll, texts, texts ? w / texts : 0.0, n50 ? r50 / n50 : 0.0, n50, n50 ? d50 / n50 : 0.0, n100 ? r100 / n100 : 0.0, n100, nAll ? rAll / nAll : 0.0, nAll ? g2 / nAll : 0.0, nAll ? g3 / nAll : 0.0);
    }
    printf("the cost: a text %.1f us on average (%.0f at most), a people's texts %.1f ms on average (%.1f at most), the grammar read in %.2f ms\n",
           R.texts ? R.usSum / R.texts : 0.0, R.usMax, R.peoples ? R.msSum / R.peoples : 0.0, R.msMax, G->parseMs);
    printf("grammar: %d failure%s\n", fails + (int)G->errors.size(), fails + G->errors.size() == 1 ? "" : "s");
    return fails || !G->errors.empty() ? 1 : 0;
}
