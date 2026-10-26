// The grammar engine (H-03; the format and the engine: docs/reference/civilisations/records-and-grammar.md). A record's text is a small
// attribute grammar over a realiser: the `.vg` files (peoples/grammar/, compiled in, read from disk by the harness) hold the rules, each a
// name and its indented alternatives (a weight, guards, `once`, a tense, a text of marks); the expansion draws by weight from the
// alternatives whose guards hold, the unused first and the ones the people's other records used less (`VgMemory`: a world's records are one
// catalogue), with each people's dialect, its sentences said once and its distinctive words worn out (VG_DIALECT, VG_RETRIES, VG_FATIGUE);
// the realiser writes the English (a first mention in full, a pronoun only where it is clear, the verbs by tense, person and number, the
// numbers with their nouns, "a" or "an", the spaces and the capitals), so the grammar never writes "he" or a verb's ending for an entity.
// The checks (`vgCheck`) read a realised record for what the construction cannot rule out; the measures (`vgMeasure`) count the repeated
// frames. A record is a function of its seed, its model and the catalogue's memory (drawn in index order); a name inside a fact's words is
// marked (VG_NAME_BEGIN .. VG_NAME_END) and never translated.
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// a word list's number, for its articles and its verbs: a plural ("goats") and a mass noun ("barley") take no "a"
enum VgNumber { VN_NONE = 0, VN_SING, VN_PLURAL, VN_MASS };
enum VgTense { VG_PAST = 0, VG_PRESENT };
enum VgGender { VG_HE = 0, VG_SHE, VG_THEY, VG_IT };
extern const char* const VG_AGES[6];   // the ages as the guards and the lexicon name them: "prehistoric" .. "farfuture" (A-01's order)
// H-04: a proper name within a model's words ("the War of \1Kauza\2"): written as it is and kept with the record's names
constexpr char VG_NAME_BEGIN = '\x01', VG_NAME_END = '\x02';
std::string vgName(const std::string& name);   // the name between the marks

// what a grammar may read, named once by its model: `vgLink` checks every reference against it (a typo is an error at load)
struct VgSchema {
    std::vector<std::string> facts;        // by index: the model's `facts` ("desert", "tone", "river" ...)
    std::vector<std::string> names;        // the facts that are proper names (the decoder keeps them as they are)
    std::vector<std::string> entities;     // the entities a model binds ("$kin", "$child" ...)
    std::vector<std::string> paths;        // an entity's fields that are entities ("a", "place", "mother": `$e.a`, `$x.mother`)
    std::vector<std::string> fields;       // an entity's fields that are words ("kind", "season", "trade": guards and `{$e.season}`)
    std::vector<std::string> directives;   // the plan's (`{@news private}`)
};

// an entity a record speaks of: a person, a group, a place, a thing. The model writes its forms; the realiser chooses among them
struct VgEntity {
    int gender = VG_HE;       // its pronouns (he, she, they, it)
    int person = 3;           // 1 the writer (I, me, my), 2 the reader (you, your), 3 another
    std::string name;         // what it is called ("Aiwya", "Kethra"); "" none (a relative known by the relation)
    std::string first;        // its first mention in full ("my mother", "Queen Felugocea", "Aurai the weaver"); "" the name
    std::string later;        // a later mention where a pronoun would not be clear ("the queen"); "" the name, else the first
    std::string voc;          // as it is addressed ("Mother"); "" the name, else the first
    bool shown = false;       // its first mention shows its gender ("my mother", "the woman", "Queen X"; a name does not until a pronoun has)
    bool named = false;       // H-04: always by its name or its later form, never a pronoun (a place, an event: "it" would be anything)
    int ref = -1;             // the model's own (a history's person or event)
};

struct VgRun;
// what a record reads. The model fills `facts` (by the schema's index) and `entities`, binds the schema's entities (`bind`), and
// answers what the grammar asks of its history: an entity's paths and fields, the plan's directives
struct VgModel {
    std::vector<std::string> facts;
    std::vector<VgEntity> entities;
    std::vector<int> bound;   // by the schema's entity index: the entity it is in this record, -1 none
    virtual ~VgModel() = default;
    void bind(const VgSchema& S, const std::string& name, int entity);
    void set(const VgSchema& S, const std::string& fact, const std::string& value);
    virtual int follow(int entity, int path) { return -1; }                         // `$x.path`: an entity (added to `entities` as needed), -1 none
    virtual int lazy(int schemaEntity) { return -1; }                               // H-04: a schema entity not bound yet, bound when a text first names it (-1 none)
    virtual bool field(int entity, int field, std::string& out) { return false; }   // `$x.field`: a word ("spring", "1")
    virtual bool directive(VgRun& run, int directive, const std::string& args) { return false; }
    // H-04's second pass: the people's seed (0 none: no dialect, every alternative) and their own word for a thing (`{~key}`)
    uint64_t culture = 0;
    virtual bool tongue(const std::string& key, std::string& out) { return false; }
};

// a parsed text
struct VgNode {
    enum Type : uint8_t { TEXT = 0, PAR, CHOICE, RULE, FACT, LOCAL, ENT, VERB, NUM, ATTR, DIRECT, DECL, PLURAL, TONGUE };
    Type type = TEXT;
    std::string s;                // TEXT the text; RULE/FACT/PLURAL the key; LOCAL/ATTR the attribute; ENT/DECL the entity; VERB the lemma; NUM the key; DIRECT the name; TONGUE the thing's key
    std::string a, b;             // ENT the form; VERB the subject; NUM the form and the noun; ATTR the value; DIRECT the arguments; DECL the gender and the later form; RULE the local it binds (`>.x`); LOCAL the form
    std::vector<std::string> path;          // ENT, NUM: the fields after the entity (`$e.a.mother` -> a, mother)
    std::vector<std::vector<VgNode>> opts;  // CHOICE
    int ref = -1;                 // linked: the rule, the fact, the schema's entity, the directive (-1 a local entity or attribute)
    std::vector<int> pathRef;     // linked: the schema's paths and the last a field (-1)
    int fieldRef = -1;
};
struct VgCond {   // a guard: key op value; `alt` an OR (`a=1|b<2`)
    std::string key, val;
    int op = 0;          // 0 =, 1 !=, 2 >=, 3 <=, 4 >, 5 <, 6 has, 7 !has
    int kind = 0;        // linked: 0 a fact, 1 a local attribute, 2 an entity, 3 an entity's field
    int ref = -1;        // the fact, the schema's entity
    std::vector<int> pathRef; int fieldRef = -1;
    double num = 0; bool numeric = false;
    std::vector<VgCond> alt;
};
struct VgAlt {
    double w = 1;
    bool once = false;
    int tense = -1;           // -1 inherited
    int number = VN_NONE;     // the word's number (else the rule's)
    std::vector<VgCond> conds;
    std::vector<VgNode> body;
    int rule = -1, file = 0, line = 0;
    uint64_t key = 0;         // its rule's name and its text hashed: a people's dialect and choices read it (stable when other rules change)
    std::vector<int> words;   // its distinctive words (`VgGrammar::words`): the fatigue's
    bool apart = false;       // D-05: its file's or its guard's `@apart`: left out of the counts that make a word distinctive (no other people's draws move)
};
// D-05: `@instead <guard> <rule> <rule2>`: where the guard holds, a reference to `rule` expands `rule2` (a population's own words for an ordinary
// rule's, the rule's lines unchanged: a line's key is its text, which every people's dialect and choices read)
struct VgInstead { VgCond cond; std::string from, to; int fromRule = -1, toRule = -1, file = 0, line = 0; };
struct VgRule {
    std::string name;
    int number = VN_NONE;     // a word list's: `(pl)`, `(mass)`, `(sing)`
    bool keep = false;        // `(keep)`: the first draw holds for the whole record
    bool steady = false;      // `(steady)`: its weights hold across a people's records (no catalogue dryness: C-02's kinds keep their shares)
    bool formula = false;     // `(formula)`: a genre's formula (a greeting, a heading, a blessing): the measures leave its sentences out
    int wordsLo = 0, wordsHi = 0;   // `(words=8..70)`: a record it writes is that long (the check)
    int peopleK = 0;          // `(people=K)`: a people keeps K of its alternatives (0 all)
    double share = 0;         // `(share=F)`: a people keeps that share of them (0 all)
    std::vector<int> alts;    // into `VgGrammar::alts`
    int file = 0, line = 0;
    bool used = false;        // reachable from a root (the link)
    bool pure = true;         // no directive under it (the link): it may be drawn again
    bool stops = false;       // it may write a sentence's end (the link)
};
struct VgGrammar {
    std::vector<VgRule> rules;
    std::vector<VgAlt> alts;
    std::unordered_map<std::string, int> ruleIndex;
    std::vector<std::string> files;                          // the sources by index (the diagnostics' "file:line")
    std::vector<std::string> roots;                          // `@root`: the rules a model expands
    std::unordered_map<std::string, int> lexicon;            // `@lexicon <age> words`: a word and the first age that had it
    std::unordered_map<std::string, std::string> lexFactNames;   // `@lexicon <fact> words` (H-04): a word and the fact it needs, as written
    std::unordered_map<std::string, int> lexFact;            // the same, linked: the fact's index
    std::vector<std::string> errors;                         // "file:line: what"
    std::vector<std::string> localEntities;                  // the `{+$x}` the texts bring in (the link knows them)
    std::unordered_set<std::string> plurals;                 // the plural words of the `(pl)` lists (the check: no "a" or "one" before them)
    std::vector<std::string> words;                          // the distinctive words (in few alternatives): the fatigue's, by index
    std::unordered_set<std::string> tongueKeys;              // the `{~key}` the texts ask the model for
    // a population's own lines kept apart from the others' (`@apart` alone: a file's every alternative; `@apart key=value`: every
    // alternative with that guard, in any file): a word of theirs never makes a word of the others' common, so no other people's records move;
    // and the rules a guard swaps (`@instead`)
    std::vector<char> apartFiles;
    std::vector<std::pair<std::string, std::string>> apartGuards;
    std::vector<VgInstead> insteads;
    const VgSchema* schema = nullptr;
    double parseMs = 0;
    int rule(const std::string& name) const { auto it = ruleIndex.find(name); return it == ruleIndex.end() ? -1 : it->second; }
    std::string where(int alt) const;                        // "registers/annal.vg:18"
};
// a source's text into the grammar (a raw string's `R"VG(` and `)VG"` lines are skipped, so a file and its embedded copy read alike)
bool vgParse(VgGrammar& G, const std::string& text, const std::string& file);
// resolves every reference against the model's schema; errors: a rule or a fact unknown, a guard's key unknown, a rule no root
// reaches, "a" before a plural word list
bool vgLink(VgGrammar& G, const VgSchema& S);

// what a world's records share: each alternative's uses so far (an alternative used k times weighs 1 / (1 + VG_DRYNESS k)); H-04's
// second pass: the distinctive words' uses (the fatigue) and the frames of the sentences written (each with the order it came in)
struct VgMemory {
    std::vector<int> uses;
    std::vector<int> wordUses;
    std::vector<float> dialect;   // the people's weight of each alternative (0 not computed yet)
    std::unordered_map<uint64_t, int> frames;
    int frameSeq = 0;
    int retries = 0, kept = 0;   // the draws undone for a repeated frame, and the repeats let stand (the retries or the budget spent)
};

// a record: what it is written with and what came out
struct VgRecord {
    uint64_t seed = 0;
    int tense = VG_PRESENT;   // the verbs' unless an alternative says
    bool inscription = false; // the paragraphs keep the pronouns' reach (an epitaph's lines read as one text)
    std::string text;
    int root = -1;            // the root's alternative drawn (C-02's kind), its index in the rule
    int words = 0, lo = 0, hi = 0;   // the words and the length its rules declared (0 none)
    std::vector<int> used;    // the alternatives drawn, in order
    std::vector<std::string> names;   // the proper names written (the decoder keeps them)
    std::vector<std::string> theirs;  // the people's own words written (`{~key}`; the decoder keeps them as names)
    std::vector<std::string> formulas;   // the frames of the sentences its formula rules wrote (`vgFrame`; the measures leave them out)
    std::vector<std::string> diag;    // what could not be written ("no alternative of x", "an a before a plural") and the realiser's notes
    double us = 0;            // the microseconds it took
    int retries = 0;          // the rule draws undone for a repeated frame
};
// expands the root with the model's facts and entities, then realises it; false (with `diag`) when it could not
bool vgRecord(const VgGrammar& G, const std::string& root, VgModel& M, VgMemory* mem, VgRecord& R);

// the run a directive is handed: it may bind an entity for the rest of the alternative, expand a rule, start a paragraph, draw
struct VgRun {
    virtual ~VgRun() = default;
    virtual void bindEntity(const std::string& name, int entity) = 0;   // "$e": for the rest of the calling alternative
    virtual bool expand(const std::string& rule) = 0;                   // false: no alternative held (nothing written)
    virtual bool has(const std::string& rule) const = 0;                // the rule exists (H-04: a directive tells an event only in a register that has its kind)
    virtual void paragraph() = 0;
    virtual double uni() = 0;                                           // the record's own stream
    virtual void note(const std::string& what) = 0;                     // a diagnostic
};

// the realiser's pieces (the unit's cases): a verb's form, a noun's plural, a number in words
std::string vgInflect(const std::string& lemma, int person, bool plural, int tense);
std::string vgPlural(const std::string& noun);
std::string vgNumberWords(long n);   // "forty-two", "a hundred", "two thousand and six"
// H-04: the root word of an irregular form the realiser writes ("went" -> "go", "children" -> "child"); "" when it is not one
std::string vgIrregularLemma(const std::string& word);
std::string vgOrdinal(long n);       // "fifth", "212th"

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do

// the checks on a realised record (the harness and the unit run them over thousands): counts by kind, the reasons appended
enum VgCheckKind { VC_MARK = 0, VC_DIAG, VC_DOUBLED, VC_ARTICLE, VC_PLURAL, VC_AGREEMENT, VC_CAPITAL, VC_SPACING, VC_LENGTH, VC_ANACHRONISM, VC_COUNT };
extern const char* const VG_CHECK_NAMES[VC_COUNT];   // "mark left", "diagnostic", "doubled word", ...
int vgCheck(const VgGrammar& G, const VgRecord& R, int age, int counts[VC_COUNT], std::vector<std::string>* why);

// the measures (`RESEARCH-records.md`, section 7): a sentence's frame is its words with the names, the digits and the number words
// masked; a sentence of six words or more is on a repeated frame when its frame occurs twice or more in the sample
std::vector<std::string> vgSentences(const std::string& text);
std::string vgFrame(const std::string& sentence);
int vgWords(const std::string& text);
// H-04: `formulas` (per text, its record's `formulas`, or null) leaves the genres' formulas out; `distinct` counts the sentences of
// six words or more written once in the sample, word for word
struct VgMeasure { int texts = 0, words = 0, minW = 0, maxW = 0, sentences = 0, longS = 0, longRep = 0, formulaS = 0, distinct = 0; double d2 = 0, d3 = 0; };
VgMeasure vgMeasure(const std::vector<std::string>& texts, const std::vector<const std::vector<std::string>*>* formulas = nullptr);
void vgTopFrames(const std::vector<std::string>& texts, int n, std::vector<std::pair<int, std::string>>& out, const std::vector<const std::vector<std::string>*>* formulas = nullptr);
