// H-04 (2026-10-09): the records' texts (genres.h). The model the records' grammar reads: the schema (the facts, the entities, their
// paths and fields, the directives), a record's facts from the world, the people and the history at its date, the persons and places
// it names as entities in the writer's eyes, and the plan's directives that pick what it tells by salience.
#include "genres.h"
#include "genres_schema.h"

namespace {
double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

// a place's landmark (the first of these the place's hash ranks that its water and the record's age allow: need 0 none, 1 a coast, 2 a river)
struct Landmark { const char* text; int need; unsigned ages; };
constexpr unsigned LP = 1, LM = 2, LI = 4, LMO = 8, LF = 16, LFF = 32;
const Landmark LANDMARKS[] = {
    {"the old bridge", 2, LM | LI | LMO}, {"the harbour wall", 1, LM | LI | LMO}, {"the market cross", 0, LM | LI}, {"the long stair", 0, LM | LI | LMO | LF},
    {"the red rock", 0, LP | LM | LI | LMO | LF | LFF}, {"the great tree", 0, LP | LM | LI}, {"the north gate", 0, LM | LI}, {"the clock tower", 0, LI | LMO},
    {"the old mill", 2, LM | LI}, {"the three stones", 0, LP | LM}, {"the lookout hill", 0, LP | LM | LI | LMO | LF}, {"the ford", 2, LP | LM},
    {"the square", 0, LM | LI | LMO | LF}, {"the fountain", 0, LM | LI | LMO}, {"the bathhouse", 0, LM | LI}, {"the lighthouse", 1, LM | LI | LMO | LF},
    {"the dyeworks", 2, LI}, {"the gasworks", 0, LI | LMO}, {"the tram depot", 0, LMO}, {"the old wall", 0, LM | LI | LMO | LF},
    {"the castle mound", 0, LM | LI}, {"the barrow on the hill", 0, LP | LM}, {"the long house", 0, LP | LM}, {"the boat sheds", 1, LP | LM | LI},
    {"the salt pans", 1, LM | LI}, {"the quarry", 0, LP | LM | LI | LMO}, {"the pier", 1, LI | LMO}, {"the station", 0, LI | LMO},
    {"the tower blocks", 0, LMO | LF}, {"the dome", 0, LF | LFF}, {"the hanging gardens", 0, LM | LF | LFF}, {"the weir", 2, LM | LI | LMO},
    {"the reed beds", 2, LP | LM}, {"the shell mounds", 1, LP}, {"the painted cave", 0, LP}, {"the stone circle", 0, LP | LM}, {"the cattle pens", 0, LP | LM | LI},
    {"the ropewalk", 1, LM | LI}, {"the cistern", 0, LM | LI}, {"the drying racks", 0, LP | LM}, {"the beacon hill", 0, LM | LI}, {"the canal basin", 2, LI | LMO},
    {"the overpass", 0, LMO | LF}, {"the old reactor", 0, LF | LFF}, {"the glass gardens", 0, LF | LFF}, {"the tall mast", 0, LMO | LF}, {"the terraces", 0, LP | LM | LI},
    {"the twin towers", 0, LM | LI | LMO | LF}, {"the covered market", 0, LM | LI | LMO}, {"the hot springs", 0, LP | LM | LI | LMO | LF}};
constexpr int LANDMARK_N = (int)(sizeof(LANDMARKS) / sizeof(LANDMARKS[0]));
// a quarter of a place (the record's own: a town has many)
struct Quarter { const char* text; int need; unsigned ages; };
const Quarter QUARTERS[] = {
    {"the tanners' quarter", 2, LM | LI}, {"the potters' quarter", 0, LM | LI}, {"the weavers' quarter", 0, LM | LI}, {"the smiths' quarter", 0, LM | LI},
    {"the dyers' street", 2, LM | LI}, {"the fishers' quarter", 1, LM | LI | LMO}, {"the boatmen's quarter", 2, LM | LI}, {"the river quarter", 2, LM | LI | LMO | LF},
    {"the harbour quarter", 1, LM | LI | LMO | LF}, {"the upper town", 0, LM | LI | LMO}, {"the lower town", 0, LM | LI | LMO}, {"the old town", 0, LM | LI | LMO | LF},
    {"the new town", 0, LM | LI | LMO | LF}, {"the north side", 0, LM | LI | LMO | LF}, {"the south side", 0, LM | LI | LMO | LF}, {"the temple quarter", 0, LM | LI},
    {"the market quarter", 0, LM | LI | LMO}, {"the mill streets", 0, LI | LMO}, {"the station quarter", 0, LI | LMO}, {"the university quarter", 0, LI | LMO | LF},
    {"the east camp", 0, LP}, {"the lower huts", 0, LP}, {"the far fires", 0, LP}, {"the outer ring", 0, LMO | LF | LFF}, {"the inner ring", 0, LF | LFF},
    {"the garden levels", 0, LF | LFF}, {"the old blocks", 0, LMO | LF}, {"the docks", 1, LI | LMO | LF}};
constexpr int QUARTER_N = (int)(sizeof(QUARTERS) / sizeof(QUARTERS[0]));

int invIndex(const char* key) { for (int i = 0; i < INV_COUNT; i++) if (std::strcmp(INV_KEYS[i].key, key) == 0) return i; return 0; }
std::string num(int64_t v) { return std::to_string(v); }
bool closeKin(const std::string& k) { return k == "mother" || k == "father" || k == "husband" || k == "wife" || k == "grandmother" || k == "grandfather"; }
bool gendered(const std::string& k) { return !k.empty() && k != "cousin"; }
std::string capFirst(std::string s) { if (!s.empty()) s[0] = (char)std::toupper((unsigned char)s[0]); return s; }
// the history's pattern with its names filled in and marked (`{place}`, `{capital}`, `{river}`, `{sea}`, `{mountain}`, `{god}`, `{other}`)
std::string fillMarked(const History& H, std::string s, int place, int year) {
    auto rep = [&](const char* key, const std::string& v) { size_t at; while ((at = s.find(key)) != std::string::npos) s.replace(at, std::strlen(key), v); };
    rep("{place}", vgName(historyPlaceName(H, place >= 0 ? place : 0)));
    rep("{capital}", vgName(historyPlaceName(H, historySeat(H, year))));
    rep("{river}", vgName(H.lore.river)); rep("{sea}", vgName(H.lore.sea)); rep("{mountain}", vgName(H.lore.mountain)); rep("{god}", vgName(H.lore.god));
    rep("{other}", H.lore.other.empty() ? std::string("the enemy") : "the " + vgName(H.lore.other));
    return s;
}

// what the history tells of the world and the people that every record shares (built once a catalogue)
struct Timeline { std::vector<std::pair<int, int>> moves; int born = 0; };   // a person's homes by year (from their birth's place and moves)

// ---- the record model ---------------------------------------------------------------------------------------------------------------

enum RefType { R_PERSON = 0, R_EVENT, R_PLACE };
struct Ref { int type = R_PERSON; int idx = -1; };

struct RecordModel : VgModel {
    const History& H; const Lore& L; const VgSchema& S;
    std::vector<Timeline> lives;                  // each person's homes
    std::vector<int> deathOf;                     // each person's HE_DEATH event (or -1)
    std::vector<int> wayRail;                     // the years each rail way opened (sorted)
    std::vector<int> invYear;                     // each `INV_KEYS` invention's year in this history (0 never)
    std::vector<int> holyYear;                    // the years the faith's holy places were finished
    std::vector<std::vector<int>> kids;           // each person's children
    std::set<std::string> names;                  // every name the history and the lore hold (a war's or an era's words are marked by it)
    std::unordered_map<int, int> toldAll;         // the catalogue's: an event and how many records told it
    std::unordered_map<int, uint32_t> toldIn;     // the registers that told it (a bit a register)
    // the record
    int year = 1, season = 0, age = AGE_MEDIEVAL, medium = RM_STONE, home = 0, writer = -1, reader = -1, subject = -1;
    bool last = false, privateGenre = false;
    std::vector<int> toldHere;                    // the events this record told, in order
    std::vector<int> toldLegend;                  // H-06: what it told besides its directives' picks (the death an epitaph is about): the decoder's history page's, never the salience's
    std::vector<Ref> refs;                        // parallel to `entities`
    std::unordered_map<int, int> personEnt, eventEnt, placeEnt;
    Rng rng{1};
    int fixedYear = 0;                            // `@years`: the year a chronicle's paragraph is about (0 the record's)
    uint64_t recSeed = 0;                         // H-04's second pass: the record's (a place's quarter the record names)
    const Culture* culture0 = nullptr;            // the people's culture (its buildings by the record's age: `cultureFacts`)
    std::vector<int> foundingOf;                  // each place's founding event (-1 none)
    mutable std::unordered_map<int, std::string> epithets;   // a ruler's epithet, once worked out

    RecordModel(const History& h, const Lore& l) : H(h), L(l), S(schemaOf()) {
        facts.assign(S.facts.size(), std::string());
        bound.assign(S.entities.size(), -1);
        const int np = (int)H.persons.size();
        lives.resize(np); deathOf.assign(np, -1);
        for (int i = 0; i < (int)H.events.size(); i++) {
            const HistoryEvent& e = H.events[i];
            if (e.kind == HE_BIRTH && e.a >= 0 && e.a < np) { lives[e.a].born = e.year; if (e.place >= 0) lives[e.a].moves.push_back({e.year, e.place}); }
            else if (e.kind == HE_MOVE && e.a >= 0 && e.a < np && e.place2 >= 0) lives[e.a].moves.push_back({e.year, e.place2});
            else if (e.kind == HE_DEATH && e.a >= 0 && e.a < np) deathOf[e.a] = i;
            else if (e.kind == HE_WAY && e.out == NK_RAIL) wayRail.push_back(e.year);
            else if (e.kind == HE_BUILT && (e.thing == WK_HOLY || e.thing == WK_TEMPLE)) holyYear.push_back(e.year);
        }
        std::sort(wayRail.begin(), wayRail.end()); std::sort(holyYear.begin(), holyYear.end());
        kids.resize(np);
        for (int i = 0; i < np; i++) for (int par : {H.persons[i].mother, H.persons[i].father}) if (par >= 0 && par < np) kids[par].push_back(i);
        for (int p = 0; p < (int)H.places.size(); p++) names.insert(historyPlaceName(H, p));
        for (const std::string* n : {&L.people, &L.god, &L.river, &L.mountain, &L.sea, &L.city, &L.city2, &L.moon, &L.star, &L.festival, &L.other, &L.founder}) if (!n->empty()) names.insert(*n);
        foundingOf.assign(H.places.size(), -1);
        for (int i = 0; i < (int)H.events.size(); i++) if (H.events[i].kind == HE_FOUNDING && H.events[i].place >= 0 && H.events[i].place < (int)H.places.size() && foundingOf[H.events[i].place] < 0) foundingOf[H.events[i].place] = i;
        invYear.assign(INV_COUNT, 0);
        for (const HistoryInvention& I : H.inventions) {
            const std::string nm = historyInventionName(I.id);
            for (int k = 0; k < INV_COUNT; k++) if (nm == INV_KEYS[k].name && (invYear[k] == 0 || I.year < invYear[k])) invYear[k] = I.year;
        }
    }

    // a person's place in a year: where they were born, then where each move took them; the history's last home else
    int homeAt(int p, int y) const {   // never a place not founded yet or left already (a figure's last home may be younger than the year)
        if (p < 0 || p >= (int)H.persons.size()) return -1;
        int h = -1;
        for (const auto& m : lives[p].moves) { if (m.first > y) break; h = m.second; }
        if (h < 0) h = H.persons[p].home;
        if (h >= 0 && h < (int)H.places.size() && !H.placeStands(h, y) && H.places[h].founded > y) h = -1;
        return h >= 0 ? h : historySeat(H, y);
    }
    bool alive(int p, int y) const { return H.alive(p, y); }
    int ageAt(int p, int y) const { return p >= 0 && p < (int)H.persons.size() ? y - H.persons[p].born : 0; }
    int theYear() const { return fixedYear ? fixedYear : year; }
    std::string kinOf(int p, int to) const { return to >= 0 && p >= 0 && p != to ? historyKinship(H, p, to) : std::string(); }

    // H-04's second pass: their own word for a thing of theirs (`{~coin}`)
    bool tongue(const std::string& key, std::string& out) override { out = tongueTerm(tongueOfSeed(L.seed), key); return !out.empty(); }

    // ---- the entities ----
    int addEntity(const VgEntity& e, Ref r) { entities.push_back(e); refs.push_back(r); return (int)entities.size() - 1; }
    // a ruler's epithet ("the Builder", "the Old"): their traits, their reign's length and their age at its start, the works and the
    // battles of their reign, the laws they gave; a plain one by their hash where nothing says more ("" none: some rulers have none)
    std::string epithetOf(int p) const {
        auto it = epithets.find(p); if (it != epithets.end()) return it->second;
        const HistoryPerson& P = H.persons[p];
        std::vector<std::pair<std::string, double>> c;
        if (P.role == HR_RULER) {
            int years = 0, startAge = 99, built = 0, won = 0, lost = 0; bool laws = false;
            for (const HistoryReign& R : H.reigns) if (R.ruler == p) { years += R.end - R.start + 1; startAge = std::min(startAge, R.start - P.born);
                int from = 0;
                for (int y = R.start; y <= R.end; y++) { const int n = H.eventsIn(y, from); for (int k = 0; k < n; k++) { const HistoryEvent& e = H.events[from + k];
                    if (e.kind == HE_BUILT) built++;
                    if (e.kind == HE_BATTLE) { std::string w; if (const_cast<RecordModel*>(this)->eventField(from + k, FD_WON, w, y)) { if (w == "1") won++; else if (w == "0") lost++; } }
                    if (e.kind == HE_DEED && e.thing == 2 && e.a == p) laws = true; } } }
            if (built >= 3) c.push_back({"the Builder", 2.0 + built * 0.3});
            if (won >= 2 && won > lost) c.push_back({won >= 4 ? "the Conqueror" : "the Victorious", 2.0 + won * 0.4});
            if (lost >= 2 && lost > won) c.push_back({"the Unlucky", 1.2});
            if (laws) c.push_back({"the Lawgiver", 3.0});
            if (years >= 40) c.push_back({"the Old", 1.5 + (years - 40) * 0.05});
            if (startAge < 16) c.push_back({startAge < 8 ? "the Child" : "the Young", 1.6});
            if (years <= 1) c.push_back({"the Brief", 1.2});
        }
        static const std::pair<uint32_t, const char*> TR[] = {{HT_BRAVE, "the Bold"}, {HT_CRUEL, "the Cruel"}, {HT_PIOUS, "the Pious"}, {HT_WISE, "the Wise"}, {HT_LEARNED, "the Learned"},
                                                              {HT_AMBITIOUS, "the Restless"}, {HT_KIND, "the Good"}, {HT_PROUD, "the Proud"}};
        for (const auto& t : TR) if (P.traits & t.first) c.push_back({t.second, 1.0});
        std::string out;
        const uint64_t h = mix64((uint64_t)p * 0x9E3779B97F4A7C15ULL ^ H.seed ^ 0xE91ULL);
        if (P.role == HR_RULER && (c.empty() || (h & 7) == 0)) {
            static const char* const PLAIN[] = {"the Tall", "the Short", "the Fat", "the Red", "the Fair", "the Dark", "the Lame", "the Silent", "the Quiet", "the Stern", "the Mild", "the Lucky", "the Elder", "the Bald", "the Fox"};
            c.push_back({PLAIN[(h >> 8) % (sizeof(PLAIN) / sizeof(PLAIN[0]))], 0.8});
        }
        if (!c.empty() && (P.role == HR_RULER || (h % 3) == 0)) {
            double tot = 0; for (const auto& x : c) tot += x.second;
            double u = unitFromHash(h >> 3) * tot;
            for (const auto& x : c) { u -= x.second; if (u < 0) { out = x.first; break; } }
            if (out.empty()) out = c.back().first;
        }
        epithets[p] = out;
        return out;
    }
    std::string placeTrade(int pl) const {   // what a place lived by: its founding's reason, its water, the people's ways, the record's age
        const HistoryPlace& Pl = H.places[pl];
        std::string why;
        if (pl < (int)foundingOf.size() && foundingOf[pl] >= 0) why = keyOf(FOUNDING_KEYS, H.events[foundingOf[pl]].thing);
        const int a = age;
        if (why == "harbour") return a <= AGE_MEDIEVAL ? "fishing" : "shipping";
        if (why == "mine") return "mining";
        if (why == "market") return "trade";
        if (why == "crossing") return Pl.river ? "the ferries" : "the roads";
        if (why == "shrine") return "the pilgrims";
        if (why == "fort") return "the garrison";
        if (why == "station") return "the railway";
        if (why == "work") return "the works";
        if (why == "capital") return "the rule";
        if (a >= AGE_MODERN) return Pl.sclass == SC_CITY || Pl.sclass == SC_CAPITAL ? "offices" : (Pl.coast ? "the port" : "the factories");
        if (a == AGE_INDUSTRIAL && (Pl.sclass == SC_CITY || Pl.sclass == SC_TOWN)) return Pl.river ? "the mills" : "the factories";
        if (Pl.coast) return "fishing";
        if (Pl.river) return "the boats";
        const int eco = H.profile.economy;
        return eco == ECO_HERDING ? "herding" : (eco == ECO_HUNTING ? "hunting" : (eco == ECO_TRADE ? "trade" : "farming"));
    }
    std::string landmarkOf(int pl, bool quarter) const {   // a place's landmark (its own), or a quarter of it (the record's)
        const HistoryPlace& Pl = H.places[pl];
        const unsigned bit = 1u << std::max(0, std::min(5, age));
        uint64_t h = mix64(H.seed ^ (uint64_t)(pl + 1) * 0x9E3779B97F4A7C15ULL ^ (quarter ? recSeed : 0x1A4DULL));
        const int n = quarter ? QUARTER_N : LANDMARK_N;
        for (int t = 0; t < 64; t++) {
            const int k = (int)(h % (uint64_t)n); h = mix64(h + 0x51);
            const int need = quarter ? QUARTERS[k].need : LANDMARKS[k].need; const unsigned ages = quarter ? QUARTERS[k].ages : LANDMARKS[k].ages;
            if (!(ages & bit) || (need == 1 && !Pl.coast) || (need == 2 && !Pl.river)) continue;
            if (quarter && (Pl.sclass == SC_HAMLET || Pl.sclass == SC_VILLAGE) && age > AGE_PREHISTORIC) continue;
            return quarter ? QUARTERS[k].text : LANDMARKS[k].text;
        }
        return "";
    }
    std::string rulerTitle(int p) const {   // "queen" while or after they ruled ("" none)
        if (p < 0 || p >= (int)H.persons.size() || H.persons[p].role != HR_RULER) return "";
        for (const HistoryReign& R : H.reigns) if (R.ruler == p && R.start <= theYear()) {   // H-04's second pass: the people's own title (a word of theirs marked)
            const std::string t = historyRulerTitle(H, R.government, H.persons[p].female);
            return historyTitleNative(H, R.government) ? vgName(t) : t;
        }
        return "";
    }
    int personEntity(int p) {
        if (p < 0 || p >= (int)H.persons.size()) return -1;
        auto it = personEnt.find(p); if (it != personEnt.end()) return it->second;
        const HistoryPerson& P = H.persons[p];
        VgEntity e; e.gender = P.female ? VG_SHE : VG_HE; e.ref = p;
        e.name = historyPersonName(H, p, false);
        const std::string nm = vgName(e.name);
        std::string given = e.name;   // the given name: the first word, unless the second is a regnal numeral ("Baliothoo II")
        {
            const size_t sp = given.find(' ');
            if (sp != std::string::npos) {
                const size_t sp2 = given.find(' ', sp + 1);
                const std::string w = given.substr(sp + 1, sp2 == std::string::npos ? std::string::npos : sp2 - sp - 1);
                if (w.empty() || w.find_first_not_of("IVXLC") != std::string::npos) given = given.substr(0, sp);
            }
        }
        if (p == writer) { e.person = 1; e.voc = vgName(given); }
        else if (p == reader) {
            e.person = 2;
            const std::string k = kinOf(p, writer);
            e.voc = closeKin(k) || k == "grandmother" || k == "grandfather" ? capFirst(k) : vgName(given);
        } else {
            const std::string kw = kinOf(p, writer), kr = kinOf(p, reader), title = rulerTitle(p);
            if (closeKin(kw)) { e.first = "my " + kw; e.later = e.first; e.voc = capFirst(kw); e.shown = true; }
            else if (!kw.empty()) { e.first = "my " + kw + " " + nm; e.later = nm; e.shown = gendered(kw); }
            else if (closeKin(kr)) { e.first = "your " + kr; e.later = e.first; e.shown = true; }
            else if (!kr.empty()) { e.first = "your " + kr + " " + nm; e.later = nm; e.shown = gendered(kr); }
            else if (!title.empty() && title != "the mind") {
                std::string t = title;   // "High Priestess": each word of the title capitalised before the name (a word of theirs inside its marks)
                if (t.compare(0, 4, "the ") != 0) for (size_t i = 0; i < t.size(); i++) if (i == 0 || t[i - 1] == ' ' || t[i - 1] == VG_NAME_BEGIN) t[i] = (char)std::toupper((unsigned char)t[i]);
                e.first = t + " " + nm; e.later = nm;
                e.shown = title == "king" || title == "queen" || title == "high priest" || title == "high priestess";
            } else if (privateGenre && P.trade >= 0) { e.first = nm + " the " + historyTradeName(P.trade); e.later = nm; }
            else { e.first = nm; e.later = nm; }
        }
        int id = addEntity(e, {R_PERSON, p});
        personEnt[p] = id;
        return id;
    }
    int eventEntity(int ev) {
        if (ev < 0 || ev >= (int)H.events.size()) return -1;
        auto it = eventEnt.find(ev); if (it != eventEnt.end()) return it->second;
        VgEntity e; e.gender = VG_IT; e.ref = ev;
        static const char* const NP[] = {"the coming", "the first days", "the stones", "the founding", "the new rule", "the end of the rule", "the regency",
            "the change", "the war", "the battle", "the siege", "the sack", "the peace", "the sickness", "the harvest", "the famine", "the flood", "the drought",
            "the earthquake", "the eruption", "the stone from the sky", "the flare", "the long winter", "the work", "the new way", "the festival", "the sign in the sky",
            "the invention", "the discovery", "the change in the faith", "the founding", "the deed", "the trouble", "the moving", "the leaving", "the meeting",
            "the treaty", "the news", "the sign", "the change", "the leaving", "the end", "the fair", "the fire", "the storm", "the plague", "the wonder", "the games",
            "the law", "the trial", "the embassy", "the disaster", "the price", "the herds", "the show", "the visit", "the sun", "the birth", "the wedding", "the moving", "the death",
            "the illness", "the hurt", "the apprenticeship", "the house", "the quarrel", "the journey", "the promise"};
        static_assert(sizeof(NP) / sizeof(NP[0]) == HE_COUNT, "a noun phrase an event kind");
        e.first = NP[std::max(0, std::min(HE_COUNT - 1, H.events[ev].kind))]; e.later = e.first; e.shown = true; e.named = true;
        int id = addEntity(e, {R_EVENT, ev});
        eventEnt[ev] = id;
        return id;
    }
    int placeEntity(int pl) {
        if (pl < 0 || pl >= (int)H.places.size()) return -1;
        auto it = placeEnt.find(pl); if (it != placeEnt.end()) return it->second;
        VgEntity e; e.gender = VG_IT; e.ref = pl; e.name = historyPlaceName(H, pl); e.first = e.name; e.later = e.name; e.shown = true; e.named = true;
        int id = addEntity(e, {R_PLACE, pl});
        placeEnt[pl] = id;
        return id;
    }

    // ---- what a record reads of the history ----
    int rulerAt(int y) const { int r = H.reignAt(y); return r >= 0 ? H.reigns[r].ruler : -1; }
    int childOf(int p, int y) const {   // a child alive in the year, the eldest
        int best = -1;
        if (p < 0 || p >= (int)kids.size()) return -1;
        for (int i : kids[p]) if (alive(i, y) && (best < 0 || H.persons[i].born < H.persons[best].born)) best = i;
        return best;
    }
    int spouseAt(int p, int y) const { if (p < 0) return -1; int s = H.persons[p].spouse; return s >= 0 && alive(s, y) ? s : -1; }

    int lazy(int se) override {
        const int y = theYear();
        switch (se) {
            case E_RULER: { int r = rulerAt(y); return r >= 0 ? personEntity(r) : -1; }
            case E_HOME: return placeEntity(home);
            case E_SEAT: return placeEntity(historySeat(H, y));
            case E_FOUNDER: { int f = !H.places.empty() ? H.places[0].founder : -1; return personEntity(f); }
            case E_LAST: return personEntity(H.lastPerson);
            case E_CHILD: { int w = writer >= 0 ? writer : subject; int c = w >= 0 ? childOf(w, y) : -1; return c >= 0 && c != reader ? personEntity(c) : -1; }
            case E_SPOUSE: { int w = writer >= 0 ? writer : subject; int s = spouseAt(w, y); return s >= 0 && s != reader ? personEntity(s) : -1; }
            case E_KIN: {   // a relative of the writer's alive (not the reader), the closest first
                if (writer < 0) return -1;
                int best = -1, bestRank = 99;
                for (int c : {H.persons[writer].mother, H.persons[writer].father, spouseAt(writer, y), childOf(writer, y)}) {
                    if (c < 0 || c == reader || !alive(c, y)) continue;
                    int rk = 1; if (rk < bestRank) { best = c; bestRank = rk; }
                }
                return best >= 0 ? personEntity(best) : -1;
            }
            case E_F: {   // a figure alive: the most notable of the year (a maker, a prophet, a general, an inventor)
                int best = -1; for (int i = 0; i < (int)H.persons.size(); i++) {
                    const HistoryPerson& P = H.persons[i];
                    if (P.role == HR_NONE || P.role == HR_RULER || P.role == HR_REGENT || !alive(i, y) || y - P.born < 18) continue;
                    if (best < 0 || P.born < H.persons[best].born) best = i;
                }
                return best >= 0 ? personEntity(best) : -1;
            }
            default: return -1;
        }
    }
    int follow(int ent, int path) override {
        if (ent < 0 || ent >= (int)refs.size()) return -1;
        const Ref r = refs[ent];
        const int y = theYear();
        if (r.type == R_EVENT) {
            const HistoryEvent& e = H.events[r.idx];
            switch (path) {
                case P_A: return personEntity(e.a);
                case P_B: return personEntity(e.b);
                case P_C: return personEntity(e.c);
                case P_PLACE: return placeEntity(e.place);
                case P_PLACE2: return placeEntity(e.place2);
                case P_CAUSE: return eventEntity(e.cause);
                case P_RULER: { int ru = rulerAt(e.year); return ru >= 0 ? personEntity(ru) : -1; }
                default: return -1;
            }
        }
        if (r.type == R_PERSON) {
            const HistoryPerson& P = H.persons[r.idx];
            switch (path) {
                case P_MOTHER: return personEntity(P.mother);
                case P_FATHER: return personEntity(P.father);
                case P_SPOUSE: { int s = P.spouse; return s >= 0 ? personEntity(s) : -1; }
                case P_CHILD: { int c = childOf(r.idx, std::max(y, P.born)); return c >= 0 ? personEntity(c) : -1; }
                case P_HOME: { int h = homeAt(r.idx, std::min(y, P.died)); return placeEntity(h); }
                default: return -1;
            }
        }
        if (r.type == R_PLACE) {
            const HistoryPlace& Pl = H.places[r.idx];
            switch (path) {
                case P_FOUNDER: return personEntity(Pl.founder);
                case P_FROM: return placeEntity(Pl.from);
                default: return -1;
            }
        }
        return -1;
    }
    bool field(int ent, int f, std::string& out) override {
        if (ent < 0 || ent >= (int)refs.size()) return false;
        const Ref r = refs[ent];
        const int y = theYear();
        if (r.type == R_EVENT) return eventField(r.idx, f, out, y);
        if (r.type == R_PERSON) {
            const HistoryPerson& P = H.persons[r.idx];
            switch (f) {
                case FD_AGE: { int at = std::min(y, P.died); out = num(std::max(0, at - P.born)); return true; }
                case FD_BORN: out = num(P.born); return true;
                case FD_DIED: if (P.died > y) return false; out = num(P.died); return true;
                case FD_TRADE: out = historyTradeName(P.trade); return true;
                case FD_TRADES: out = historyTradePlural(P.trade); return true;
                case FD_TITLE: out = rulerTitle(r.idx); return !out.empty();
                case FD_ROLE: out = keyOf(ROLE_KEYS, P.role); return true;
                case FD_ALIVE: out = alive(r.idx, y) ? "1" : "0"; return true;
                case FD_DEAD: { bool d = P.fate == HF_DIED && (P.died < y || (P.died == y && deathOf[r.idx] >= 0 && H.events[deathOf[r.idx]].season <= season)); out = d ? "1" : "0"; return true; }
                case FD_KIN: out = kinOf(r.idx, writer); return !out.empty();
                case FD_YOURKIN: out = kinOf(r.idx, reader); return !out.empty();
                case FD_FAMILY: if (P.family < 0 || P.family >= (int)H.families.size()) return false; out = vgName(historyFamilyName(H, P.family)); return true;
                case FD_REIGNED: { for (const HistoryReign& R : H.reigns) if (R.ruler == r.idx) { out = num(std::max(1, std::min(y, R.end) - R.start + 1)); return true; } return false; }
                case FD_CHILDREN: { int n = 0; for (const HistoryPerson& c : H.persons) if ((c.mother == r.idx || c.father == r.idx) && c.born <= y) n++; out = num(n); return true; }
                case FD_FAR: { int h = homeAt(r.idx, std::min(y, P.died)); out = h >= 0 && h != home ? "1" : "0"; return true; }
                case FD_EPITHET: out = epithetOf(r.idx); return !out.empty();
                case FD_TRAIT: {
                    static const std::pair<uint32_t, const char*> TR[] = {{HT_BRAVE, "brave"}, {HT_CRUEL, "cruel"}, {HT_PIOUS, "pious"}, {HT_WISE, "wise"}, {HT_LEARNED, "learned"},
                                                                          {HT_AMBITIOUS, "ambitious"}, {HT_KIND, "kind"}, {HT_PROUD, "proud"}};
                    std::vector<const char*> c; for (const auto& t : TR) if (P.traits & t.first) c.push_back(t.second);
                    if (c.empty()) return false;
                    out = c[mix64((uint64_t)r.idx ^ H.seed ^ 0x7A17ULL) % c.size()]; return true;
                }
                default: return false;
            }
        }
        if (r.type == R_PLACE) {
            const HistoryPlace& Pl = H.places[r.idx];
            switch (f) {
                case FD_RANK: out = keyOf(RANK_KEYS, Pl.sclass); return true;
                case FD_COAST: out = Pl.coast ? "1" : "0"; return true;
                case FD_RIVER: out = Pl.river ? "1" : "0"; return true;
                case FD_FOUNDED: out = num(Pl.founded); return true;
                case FD_YEARS: out = num(std::max(0, y - Pl.founded)); return true;
                case FD_STANDS: out = H.placeStands(r.idx, y) ? "1" : "0"; return true;
                case FD_GONE: out = Pl.ended < y ? "1" : "0"; return true;
                case FD_HERE: out = r.idx == home ? "1" : "0"; return true;
                case FD_PTRADE: out = placeTrade(r.idx); return !out.empty();
                case FD_LANDMARK: out = landmarkOf(r.idx, false); return !out.empty();
                case FD_QUARTER: out = landmarkOf(r.idx, true); return !out.empty();
                default: return false;
            }
        }
        return false;
    }
    bool eventField(int ev, int f, std::string& out, int y) const {
        const HistoryEvent& e = H.events[ev];
        switch (f) {
            case FD_KIND: out = KIND_KEYS[std::max(0, std::min(HE_COUNT - 1, e.kind))]; return true;
            case FD_SEASON: if (e.season < 0) return false; out = historySeason(H, e.season); return !out.empty();
            case FD_YEAR: out = num(e.year); return true;
            case FD_AGO: out = num(y - e.year); return true;
            case FD_VALUE: out = num(std::max<int64_t>(0, e.value)); return true;
            case FD_OUT: out = num(e.out); return true;
            case FD_HERE: out = e.place >= 0 && e.place == home ? "1" : "0"; return true;
            case FD_PUB: out = e.pub ? "1" : "0"; return true;
            case FD_WAR: {
                int w = -1;
                if (e.kind == HE_WAR || e.kind == HE_BATTLE || e.kind == HE_SIEGE || e.kind == HE_SACK || e.kind == HE_PEACE) w = e.thing;
                if (w < 0 || w >= (int)H.wars.size()) return false;
                out = markWords(historyWarName(H, w)); return true;
            }
            case FD_WARKIND: {
                int w = (e.kind == HE_WAR || e.kind == HE_BATTLE || e.kind == HE_SIEGE || e.kind == HE_SACK || e.kind == HE_PEACE) ? e.thing : -1;
                if (w < 0 || w >= (int)H.wars.size()) return false;
                out = keyOf(WAR_KEYS, H.wars[w].kind); return true;
            }
            case FD_SICKNESS: if (e.kind != HE_SICKNESS) { if (e.kind == HE_DEATH && e.cause >= 0 && H.events[e.cause].kind == HE_SICKNESS) { out = markWords(historySicknessName(H, H.events[e.cause].thing)); return true; } return false; } out = markWords(historySicknessName(H, e.thing)); return true;
            case FD_WAVE: if (e.kind != HE_SICKNESS) return false; out = num(std::max(1, e.out)); return true;
            case FD_WORK: if (e.kind != HE_BUILT) return false; out = historyWorkName(H, e.thing); return !out.empty();
            case FD_WORKKEY: if (e.kind != HE_BUILT) return false; out = keyOf(WORK_KEYS, e.thing); return true;
            case FD_INVENTION: { if (e.kind != HE_INVENTION) return false; std::string nm = historyInventionName(e.thing); if (nm == "a landing on the moon") nm = L.moons > 0 ? "a landing on " + vgName(L.moon) : "the first craft to another world"; out = nm; return true; }
            case FD_INV: { if (e.kind != HE_INVENTION) return false; const std::string nm = historyInventionName(e.thing); for (const InvKey& k : INV_KEYS) if (nm == k.name) { out = k.key; return true; } return false; }
            case FD_OMEN: if (e.kind != HE_OMEN) return false; out = keyOf(OMEN_KEYS, e.thing); return true;
            case FD_SUN: if (e.kind != HE_SUN) return false; out = keyOf(SUN_KEYS, e.thing); return true;   // D-05
            case FD_HARVEST: if (e.kind != HE_HARVEST) return false; out = e.out == 0 ? "failed" : (e.out == 1 ? "poor" : "rich"); return true;
            case FD_FLOOD: if (e.kind != HE_FLOOD) return false; out = e.thing == 1 ? "sea" : "river"; return true;
            case FD_IMPACT: if (e.kind != HE_IMPACT) return false; out = e.out == 2 ? "world" : (e.out == 0 ? "town" : "near"); return true;
            case FD_HOW: if (e.kind != HE_DEATH) return false; out = keyOf(DEATH_KEYS, e.thing); return true;
            case FD_GOV: if (e.kind != HE_GOVERNMENT) return false; out = keyOf(GOV_KEYS, e.thing); return true;
            case FD_GOVOLD: if (e.kind != HE_GOVERNMENT) return false; out = keyOf(GOV_KEYS, (int)e.value); return true;
            case FD_GOVHOW: if (e.kind != HE_GOVERNMENT) return false; out = keyOf(GOVHOW_KEYS, e.out); return true;
            case FD_ACCESSION: if (e.kind != HE_ACCESSION) return false; out = keyOf(ACCESSION_KEYS, e.thing); return true;
            case FD_RULEEND: if (e.kind != HE_RULE_END) return false; out = keyOf(RULEEND_KEYS, e.thing); return true;
            case FD_INSTITUTION: if (e.kind != HE_INSTITUTION || e.thing < 0 || e.thing >= (int)H.institutions.size()) return false; out = INST_KEYS[std::max(0, std::min(HI_COUNT - 1, H.institutions[e.thing].kind))]; return true;
            case FD_INSTNAME: if (e.kind != HE_INSTITUTION || e.thing < 0 || e.thing >= (int)H.institutions.size()) return false; out = institutionName(e.thing); return true;
            case FD_DISCOVERY: if (e.kind != HE_DISCOVERY) return false; out = keyOf(DISCOVERY_KEYS, e.thing); return true;
            case FD_DEED: if (e.kind != HE_DEED) return false; out = keyOf(DEED_KEYS, e.thing); return true;
            case FD_UNREST: if (e.kind != HE_UNREST) return false; out = keyOf(UNREST_KEYS, e.thing); return true;
            case FD_WHITHER: if (e.kind != HE_EXODUS) return false; out = keyOf(WHITHER_KEYS, e.thing); return true;
            case FD_SIGN: if (e.kind != HE_SIGN) return false; out = fillMarked(H, historySignPattern(H.cause, e.thing / 2, e.thing & 1), e.place, e.year); return true;
            case FD_STAGE: if (e.kind == HE_SIGN) { out = num(std::min(3, e.thing / 2)); return true; } if (e.kind == HE_STAGE) { out = num(e.out); return true; } return false;
            case FD_LEFT: if (e.kind != HE_ABANDONED) return false; out = keyOf(LEFT_KEYS, e.thing); return true;
            case FD_FAITH: if (e.kind != HE_FAITH) return false; out = keyOf(FAITHEV_KEYS, e.thing); return true;
            case FD_TREATY: if (e.kind != HE_TREATY) return false; out = keyOf(TREATY_KEYS, e.thing); return true;
            case FD_FOUNDING: if (e.kind != HE_FOUNDING) return false; out = keyOf(FOUNDING_KEYS, e.thing); return true;
            case FD_WHENCE: if (e.kind != HE_COMING) return false; out = keyOf(WHENCE_KEYS, e.thing); return true;
            case FD_FIRST: if (e.kind != HE_FIRST) return false; out = keyOf(FIRST_KEYS, e.thing); return true;
            case FD_WHAT:   // H-04's second pass: an everyday kind's word (a family's too; an apprenticeship's trade)
                if (e.kind == HE_APPRENTICE) { out = historyTradeName(e.thing); return true; }
                if ((e.kind < HE_FAIR || e.kind > HE_VISIT) && (e.kind < HE_ILLNESS || e.kind > HE_JOURNEY)) return false;
                out = historyEverydayKey(e.kind, e.thing); return !out.empty();
            case FD_VERDICT: if (e.kind != HE_TRIAL) return false; out = keyOf(VERDICT_KEYS, e.out); return true;
            case FD_WON: {   // "1" when the record's own side won (a battle, a war's end): ours is the seat's, or the people against the other people
                if (e.kind != HE_BATTLE && e.kind != HE_PEACE) return false;
                if (e.out == 2) { out = "draw"; return true; }
                int w = e.thing; if (w < 0 || w >= (int)H.wars.size()) return false;
                const HistoryWar& W = H.wars[w];
                bool usA = W.sideB == -2 || W.sideA < 0 || W.sideA == historySeat(H, e.year) || W.kind == HW_REBELLION || W.kind == HW_REVOLUTION || W.kind == HW_CIVIL;
                out = (e.out == 0) == usA ? "1" : "0"; return true;
            }
            default: return false;
        }
    }
    std::string institutionName(int i) const {
        const HistoryInstitution& I = H.institutions[i];
        static const char* const NM[HI_COUNT] = {"the Council of {place}", "the priesthood of {god}", "the guilds of {place}", "the Academy at {place}", "the Company of {place}",
            "the {place} Gazette", "the University of {place}", "the Assembly", "the Broadcast of {place}", "the Network", "the Mind", "the Army of {place}"};
        return fillMarked(H, NM[std::max(0, std::min(HI_COUNT - 1, I.kind))], I.place, I.founded);
    }
    // a name of several words marked where its words are the history's names (a war's "the \1Shiodrirn\2 War")
    std::string markWords(const std::string& s) const {
        std::string out, w;
        for (size_t i = 0; i <= s.size(); i++) {
            char c = i < s.size() ? s[i] : ' ';
            if (std::isalnum((unsigned char)c) || c == '\'' || c == '-') { w += c; continue; }
            if (!w.empty()) { out += names.count(w) ? vgName(w) : w; w.clear(); }
            if (i < s.size()) out += c;
        }
        return out;
    }

    // ---- the record's facts -------------------------------------------------------------------------------------------------------
    void setFlag(int f, bool v) { facts[f] = v ? "1" : "0"; }
    void yearFacts(int y) {   // what the history held in a year (a chronicle's paragraphs read their own)
        const int ei = H.eraAt(y);
        const HistoryEra* era = ei >= 0 && ei < (int)H.eras.size() ? &H.eras[ei] : nullptr;
        int gov = era ? (era->govChange && y < era->govChange ? era->govFrom : era->government) : GOV_CHIEFS;
        const int ri = H.reignAt(y);
        if (ri >= 0) gov = H.reigns[ri].government;
        facts[F_GOV] = keyOf(GOV_KEYS, gov);
        facts[F_ERA] = era ? markWords(era->name) : std::string();
        setFlag(F_RULED, ri >= 0 && H.reigns[ri].ruler >= 0);
        facts[F_REIGN] = ri >= 0 ? num(y - H.reigns[ri].start + 1) : std::string();
        facts[F_HARVEST] = num(H.harvestAt(y));
        facts[F_LASTHARVEST] = num(H.harvestAt(y - 1));
        for (int k = 0; k < INV_COUNT; k++) facts[F_INV0 + k] = invYear[k] > 0 && invYear[k] <= y ? "1" : "";
        for (int k = 0; k < HI_COUNT; k++) facts[F_INST0 + k].clear();
        for (int i = 0; i < (int)H.institutions.size(); i++) {
            const HistoryInstitution& I = H.institutions[i];
            if (I.founded <= y && (I.ended <= 0 || I.ended >= y)) facts[F_INST0 + std::max(0, std::min(HI_COUNT - 1, I.kind))] = institutionName(i);
        }
        bool war = false; for (const HistoryWar& W : H.wars) if (W.start <= y && W.end >= y) war = true;
        setFlag(F_WAR, war);
        bool sick = false, famine = false; int from = 0;
        for (int yy = y - 1; yy <= y; yy++) { int n = H.eventsIn(yy, from); for (int k = 0; k < n; k++) { int kind = H.events[from + k].kind; if (kind == HE_SICKNESS) sick = true; if (kind == HE_FAMINE) famine = true; } }
        setFlag(F_SICK, sick); setFlag(F_FAMINE, famine);
        const int st = H.profile.stageAt(y);
        facts[F_STAGE] = num(st);
        setFlag(F_ENDING, st >= STAGE_CRISIS);
        {   // D-05: the sun of a drifter's people in the year
            const double light = H.sunlightAt(y);
            const int dark = H.sunlight.empty() ? 0 : sunDarkOf(light, y, H.profile.lastSunrise);
            facts[F_DARK] = num(dark);
            facts[F_DARKYEARS] = num(dark == 3 ? y - H.profile.lastSunrise : 0);
        }
        bool contact = false, otherWar = false, otherGone = false;
        for (const HistoryEvent& e : H.events) {
            if (e.year > y) break;
            if (e.kind == HE_CONTACT) contact = true;
            if (e.kind == HE_OTHER_END) otherGone = true;
        }
        for (const HistoryWar& W : H.wars) if ((W.kind == HW_OTHER || W.kind == HW_CONQUEST) && W.start <= y && W.end >= y) otherWar = true;
        setFlag(F_CONTACT, contact && !L.other.empty()); setFlag(F_OTHERWAR, otherWar); setFlag(F_OTHERGONE, otherGone);
        facts[F_AGE] = AGE_KEYS[std::max(0, std::min(AGE_COUNT - 1, era ? era->age : age))];
    }

    // what the history offers at the record's date, for the genres' guards: the candidates of each cast
    std::vector<int> readers, writersAny, deadNear, testators, notables, households;
    std::vector<char> readerHere;   // a reader lives where the record lies
    void offers() {
        const int y = year;
        readers.clear(); readerHere.clear(); writersAny.clear(); deadNear.clear(); testators.clear(); notables.clear(); households.clear();
        bool pair = false;
        for (int p = 0; p < (int)H.persons.size(); p++) {
            const HistoryPerson& P = H.persons[p];
            if (P.died >= y - 3 && P.died <= y && P.fate == HF_DIED && deathOf[p] >= 0) {
                const HistoryEvent& d = H.events[deathOf[p]];
                if (d.year < y || d.season <= season || d.season < 0) { deadNear.push_back(p); if (d.pub && d.year >= y - 1) notables.push_back(p); }
            }
            if (!alive(p, y)) continue;
            const int a = y - P.born;
            if (a >= 16) writersAny.push_back(p);
            if (a >= 45 && P.died > y && P.died <= y + 6 && P.fate == HF_DIED && (spouseAt(p, y) >= 0 || childOf(p, y) >= 0)) testators.push_back(p);
            if (a >= 12) {   // a reader: anyone alive with kin to write to them; the ones living where the record lies first (it was kept there)
                const bool here = homeAt(p, y) == home;
                bool kin = false;
                for (int k : {P.mother, P.father, spouseAt(p, y)}) if (k >= 0 && alive(k, y) && y - H.persons[k].born >= 16) kin = true;
                if (!kin) for (int c : kids[p]) if (alive(c, y) && y - H.persons[c].born >= 16) { kin = true; break; }
                if (kin) { readers.push_back(p); pair = true; readerHere.push_back(here); }
                if (here && a >= 20 && spouseAt(p, y) >= 0 && childOf(p, y) >= 0) households.push_back(p);
            }
        }
        setFlag(F_PAIR, pair);
        setFlag(F_WRITER, !writersAny.empty());
        setFlag(F_DEAD, !deadNear.empty());
        setFlag(F_WILL, !testators.empty());
        setFlag(F_NOTABLE, !notables.empty());
        setFlag(F_HOUSEHOLD, !households.empty());
        bool event = false, battle = false, built = false, invented = false, sign = false, figure = false;
        int from = 0;
        for (int yy = std::max(H.first, y - 20); yy <= y; yy++) {
            int n = H.eventsIn(yy, from);
            for (int k = 0; k < n; k++) {
                const HistoryEvent& e = H.events[from + k];
                if (!e.pub || (yy == y && e.season > season)) continue;
                if (yy >= y - 15 && e.salience >= 0.5f) event = true;
                if (e.kind == HE_BATTLE || e.kind == HE_SACK || e.kind == HE_SIEGE) battle = true;
                if (yy >= y - 10 && e.kind == HE_BUILT) built = true;
                if (yy >= y - 15 && e.kind == HE_INVENTION) invented = true;
                if (yy >= y - 6 && (e.kind == HE_SIGN || e.kind == HE_EXODUS || e.kind == HE_ABANDONED) && H.profile.stageAt(yy) >= 0) sign = true;
            }
        }
        for (int p = 0; p < (int)H.persons.size() && !figure; p++) { const HistoryPerson& P = H.persons[p]; if (P.role != HR_NONE && P.role != HR_RULER && P.role != HR_REGENT && alive(p, y) && y - P.born >= 18) figure = true; }
        setFlag(F_EVENT, event); setFlag(F_BATTLE, battle); setFlag(F_BUILT, built); setFlag(F_INVENTED, invented); setFlag(F_SIGN, sign); setFlag(F_FIGURE, figure);
        setFlag(F_HOLYPLACE, !holyYear.empty() && holyYear.front() <= y);
        setFlag(F_RAILWAY, !wayRail.empty() && wayRail.front() <= y && facts[F_INV0 + invIndex("railway")] == "1");
    }

    void beginRecord(int y, int seas, int ag, int med, int hm, bool isLast, int tone, uint64_t seed, int month, int day) {
        year = y; season = seas; age = ag; medium = med; home = hm; last = isLast;
        writer = reader = subject = -1; privateGenre = false; fixedYear = 0;
        entities.clear(); refs.clear(); personEnt.clear(); eventEnt.clear(); placeEnt.clear(); toldHere.clear(); toldLegend.clear();
        bound.assign(S.entities.size(), -1);
        rng = Rng(seed ^ 0x4E0A5ULL);
        recSeed = mix64(seed ^ 0x9EC5EEDULL);
        if (culture0) cultureFacts(ag);
        yearFacts(y);
        facts[F_YEAR] = num(y);
        facts[F_SEASON] = num(seas);
        facts[F_MEDIUM] = MEDIUM_KEYS[std::max(0, std::min(RM_COUNT - 1, med))];
        facts[F_AGE] = AGE_KEYS[std::max(0, std::min(AGE_COUNT - 1, ag))];
        setFlag(F_LAST, isLast);
        facts[F_TONE] = keyOf(TONE_KEYS, tone);
        char buf[32]; snprintf(buf, sizeof buf, "%.4f", H.last > 1 ? (y - 1) / (double)(H.last - 1) : 1.0); facts[F_SHARE] = buf;
        facts[F_MONTH] = month > 0 ? num(month) : std::string(); facts[F_DAY] = day > 0 ? num(day) : std::string();
        facts[F_HOMERANK] = hm >= 0 && hm < (int)H.places.size() ? keyOf(RANK_KEYS, H.places[hm].sclass) : std::string("town");
        facts[F_TEMPER] = TEMPER_KEYS[rng.irange(5)];
        facts[F_FAR] = "0";
        facts[F_Y] = num(y); facts[F_Y0].clear(); facts[F_Y1].clear(); facts[F_GAP].clear();
        { const int own = H.eraAt(y); bool past = false; for (int k = 0; k < own && k < (int)H.eras.size(); k++) if (H.eras[k].end >= std::max(1, H.first)) past = true; setFlag(F_PAST, past); }   // B-414
        offers();
    }

    // H-04's second pass: the people's buildings in the record's age (the genome's ways through the age, as the ruins show them)
    Culture cultAge[AGE_COUNT]; bool cultHave[AGE_COUNT] = {false, false, false, false, false, false};
    void cultureFacts(int ag) {
        ag = std::max(0, std::min(AGE_COUNT - 1, ag));
        if (!cultHave[ag]) { cultAge[ag] = ag == culture0->age ? *culture0 : cultureAtAge(*culture0, ag); cultHave[ag] = true; }
        const Culture& c = cultAge[ag];
        const Genome& G = c.genome;
        facts[F_PLAN] = keyOf(PLAN_KEYS, G.of(GV_PLAN)); facts[F_ROOF] = keyOf(ROOF_KEYS, c.roof); facts[F_ORNAMENT] = keyOf(ORNAMENT_KEYS, G.of(GV_ORNAMENT));
        facts[F_MOTIF] = keyOf(MOTIF_KEYS, G.of(GV_MOTIF)); facts[F_TOPS] = keyOf(TOPS_KEYS, G.of(GV_TOPS)); facts[F_ACCENT] = keyOf(ACCENT_KEYS, G.accentColour());
        facts[F_LAND] = keyOf(LAND_KEYS, G.of(GV_LAND)); facts[F_STREETS] = keyOf(STREETS_KEYS, G.of(GV_STREETS)); facts[F_PUBLIC] = keyOf(PUBLIC_KEYS, G.of(GV_PUBLIC));
        facts[F_EDGE] = keyOf(EDGE_KEYS, G.of(GV_EDGE)); facts[F_LINKS] = keyOf(LINKS_KEYS, G.of(GV_LINKS)); facts[F_LIGHT] = keyOf(LIGHT_KEYS, G.of(GV_LIGHT));
        facts[F_WALLS] = c.family == 1 ? "mudbrick" : "stone"; facts[F_HUTS] = c.hut == 1 ? "stone" : "wattle"; setFlag(F_TOWERS, c.towers);
    }

    // ---- salience (`RESEARCH-records.md`, section 3) --------------------------------------------------------------------------------
    // an event's worth to this record now: its weight, how near it is in time, its place, the writer's stake; nothing told twice in a
    // record; told elsewhere in the catalogue half (a great event told in another register more: one event from several sides)
    double salience(int ev, int scope, int reg, double jitter) const {
        const HistoryEvent& e = H.events[ev];
        const int ago = year - e.year;
        if (std::find(toldHere.begin(), toldHere.end(), ev) != toldHere.end()) return 0;
        if (ago == 0 && e.season > season) return 0;   // later this year: not yet
        double s = std::max(0.05, (double)e.salience);
        if (e.pub) {
            if (scope == 1) return 0;   // private only
            if (ago < 0 || ago > 4) return 0;
            s *= ago <= 1 ? 1.0 : 0.35;
            if (e.place >= 0 && e.place == home) s *= 1.6;
            if (writer >= 0 && e.place >= 0 && e.place == homeAt(writer, year)) s *= 1.6;
        } else {
            if (scope == 2) return 0;   // public only
            if (ago < 0 || ago > 2) return 0;
            bool mine = false;
            for (int r : {e.a, e.b, e.c}) if (r >= 0 && (r == writer || r == reader || !kinOf(r, writer).empty() || (reader >= 0 && !kinOf(r, reader).empty()))) mine = true;
            if (!mine) return 0;
            if (e.kind == HE_DEATH && (e.a == reader || e.a == writer)) return 0;
            if (e.kind == HE_BIRTH && (e.a == writer || e.a == reader || e.b == reader || e.c == reader)) return 0;   // the reader knows their own child
            if ((e.kind == HE_MARRIAGE || e.kind == HE_MOVE) && (e.a == reader || e.b == reader)) return 0;   // and their own wedding
            if (e.kind >= HE_ILLNESS && (e.a == reader || (e.kind == HE_BETROTHAL && e.b == reader))) return 0;   // H-04's second pass: and their own fever, house, journey
            if (e.kind == HE_BIRTH && e.a >= 0 && !alive(e.a, year)) s *= 0.5;   // a child born and dead within the reach: the death tells it
            s *= 3.0 * (ago == 0 ? 1.0 : (ago == 1 ? 0.6 : 0.25));
        }
        auto it = toldAll.find(ev);
        if (it != toldAll.end()) {
            const bool great = e.salience >= 0.7f;
            auto in = toldIn.find(ev);
            const bool otherReg = in != toldIn.end() && !(in->second & (1u << reg));
            s *= great && otherReg && it->second < 3 ? 1.3 : 0.5;
        }
        for (int t : toldHere) if (H.events[t].kind == e.kind) { s *= 0.25; break; }
        return s * jitter;
    }
    void told(int ev, int reg) { toldHere.push_back(ev); }

    // ---- the directives ---------------------------------------------------------------------------------------------------------
    static int registerOf(const std::string& r) {
        static const char* const R[] = {"annal", "letter", "news", "song", "log", "picture", "diary", "sermon", "broadcast", "minutes"};
        for (int i = 0; i < (int)(sizeof(R) / sizeof(R[0])); i++) if (r == R[i]) return i;
        return 15;
    }
    bool directive(VgRun& run, int d, const std::string& args) override {
        std::vector<std::string> a; { size_t p = 0; while (p < args.size()) { size_t q = args.find(' ', p); if (q == std::string::npos) q = args.size(); if (q > p) a.push_back(args.substr(p, q - p)); p = q + 1; } }
        auto arg = [&](size_t i, const char* def) { return i < a.size() ? a[i] : std::string(def); };
        switch (d) {
            case D_CAST: return cast(run, arg(0, "letter"));
            case D_NEWS: return news(run, arg(0, "any"), arg(1, "letter"));
            case D_ENTRIES: return entries(run, arg(0, "annal"), std::atoi(arg(1, "3").c_str()));
            case D_YEARS: return years(run, std::atoi(arg(0, "3").c_str()), arg(1, "chronicle.year"));
            case D_DEATH: return death(run, arg(0, "epitaph.by"));
            case D_DEED: return deed(run, arg(0, "epitaph.deed"));
            case D_LIFE: return life(run, arg(0, "life"), std::atoi(arg(1, "3").c_str()));
            case D_EVENT: return event(run, arg(0, "any"), arg(1, "song"), std::atoi(arg(2, "15").c_str()));
            case D_DIGEST: return digest(run, arg(0, "public"), arg(1, "news"), std::atoi(arg(2, "3").c_str()));
            case D_PAST: return past(run, std::atoi(arg(0, "4").c_str()), arg(1, "history.year"));
            case D_SUNTABLE: return suntable(run, std::atoi(arg(0, "5").c_str()), arg(1, "suntable.row"));   // D-05
            default: return false;
        }
    }

    // `@cast <what>`: the persons a genre needs, from the history alive at the date (bound for the rest of the record)
    bool cast(VgRun& run, const std::string& what) {
        const int y = year;
        auto pick = [&](const std::vector<int>& c, auto weight) {
            double total = 0; std::vector<double> w(c.size());
            for (size_t i = 0; i < c.size(); i++) { w[i] = weight(c[i]); total += w[i]; }
            if (total <= 0) return -1;
            double u = run.uni() * total;
            for (size_t i = 0; i < c.size(); i++) { u -= w[i]; if (u < 0) return c[i]; }
            return c.back();
        };
        auto bindPerson = [&](int se, int p) { if (p < 0) return; bound[se] = personEntity(p); };
        if (what == "letter" || what == "message") {
            privateGenre = true;
            // the reader lives where the record lies (it was kept there); the writer is kin, mostly away
            std::vector<std::pair<int, int>> pairs; std::vector<double> w;
            for (size_t ri = 0; ri < readers.size(); ri++) {
                const int r = readers[ri];
                const HistoryPerson& R = H.persons[r];
                const double hereW = readerHere[ri] ? 5.0 : 1.0;
                std::vector<std::pair<int, double>> kin = {{R.mother, 1.2}, {R.father, 1.2}, {spouseAt(r, y), 0.5}};   // a parent, a spouse away, a child, a brother or sister
                for (int c : kids[r]) kin.push_back({c, 1.2});
                for (int par : {R.mother, R.father}) if (par >= 0) for (int sib : kids[par]) if (sib != r) kin.push_back({sib, 1.0});
                for (const auto& kw : kin) {
                    const int k = kw.first;
                    if (k < 0 || !alive(k, y) || y - H.persons[k].born < 16) continue;
                    bool dup = false; for (const auto& pr : pairs) if (pr.first == k && pr.second == r) dup = true;
                    if (dup) continue;
                    const bool away = homeAt(k, y) != homeAt(r, y);
                    pairs.push_back({k, r}); w.push_back(hereW * kw.second * (away ? 4.0 : 0.6));
                }
            }
            if (pairs.empty()) return false;
            double total = 0; for (double x : w) total += x;
            double u = run.uni() * total; size_t i = 0;
            for (; i + 1 < pairs.size(); i++) { u -= w[i]; if (u < 0) break; }
            writer = pairs[i].first; reader = pairs[i].second;
            setFlag(F_FAR, homeAt(writer, y) != homeAt(reader, y));
            bindPerson(E_ME, writer); bindPerson(E_YOU, reader);
            return true;
        }
        if (what == "diary" || what == "archive" || what == "writer") {
            privateGenre = what != "writer";
            std::vector<int> c; for (size_t ri = 0; ri < readers.size(); ri++) if (readerHere[ri] && y - H.persons[readers[ri]].born >= (what == "archive" ? 40 : 16)) c.push_back(readers[ri]);
            if (c.empty()) for (int p : writersAny) if (y - H.persons[p].born >= (what == "archive" ? 40 : 16)) c.push_back(p);
            if (c.empty() && what == "archive" && H.cause == CAUSE_SUN_LOST) {   // D-05: none forty in the dark: the oldest of them
                int old = -1; for (int p : writersAny) if (old < 0 || H.persons[p].born < H.persons[old].born) old = p;
                if (old >= 0 && y - H.persons[old].born >= 16) c.push_back(old);
            }
            writer = pick(c, [&](int) { return 1.0; });
            if (writer < 0) return false;
            bindPerson(E_ME, writer);
            return true;
        }
        if (what == "will") {
            privateGenre = true;
            writer = pick(testators, [&](int p) { return homeAt(p, y) == home ? 3.0 : 1.0; });
            if (writer < 0) return false;
            bindPerson(E_ME, writer);
            return true;
        }
        if (what == "dead" || what == "epitaph") {
            subject = pick(deadNear, [&](int p) { return (homeAt(p, H.persons[p].died) == home ? 3.0 : 1.0) * (H.persons[p].role != HR_NONE ? 2.0 : 1.0); });
            if (subject < 0) return false;
            bindPerson(E_X, subject);
            toldDeathOf(subject);
            return true;
        }
        if (what == "notable" || what == "obituary") {
            subject = pick(notables.empty() ? deadNear : notables, [&](int p) { return H.persons[p].role != HR_NONE ? 3.0 : 1.0; });
            if (subject < 0) return false;
            bindPerson(E_X, subject);
            toldDeathOf(subject);
            return true;
        }
        if (what == "household" || what == "census") {
            subject = pick(households, [&](int) { return 1.0; });
            if (subject < 0) return false;
            bindPerson(E_X, subject);
            return true;
        }
        if (what == "last") {   // the last person, at the last place (none told: the record speaks without a name)
            writer = H.lastPerson >= 0 && H.lastPerson < (int)H.persons.size() ? H.lastPerson : -1;
            if (writer >= 0) bindPerson(E_ME, writer);
            return true;
        }
        return false;
    }

    // H-06: a record about one of the dead tells their death (the history page's: `toldLegend`)
    void toldDeathOf(int p) {
        const int dv = p >= 0 && p < (int)deathOf.size() ? deathOf[p] : -1;
        if (dv >= 0 && H.events[dv].year <= year && std::find(toldLegend.begin(), toldLegend.end(), dv) == toldLegend.end()) toldLegend.push_back(dv);
    }
    // a register's rule for an event's kind (the special forms of a few: a ruler's death, a figure's)
    std::string ruleFor(const VgRun& run, const std::string& reg, int ev) const {
        const HistoryEvent& e = H.events[ev];
        if (e.kind == HE_DEATH && e.pub && e.a >= 0 && H.persons[e.a].role == HR_RULER && run.has(reg + ".rulerdeath")) return reg + ".rulerdeath";
        std::string r = reg + "." + KIND_KEYS[std::max(0, std::min(HE_COUNT - 1, e.kind))];
        return run.has(r) ? r : std::string();
    }
    // `@news <private|public|any|near> <register>`: the most salient event the record has not told, in the register's rule for its kind;
    // the register's `.none` when there is none
    bool news(VgRun& run, const std::string& scopeS, const std::string& reg) {
        const int scope = scopeS == "private" ? 1 : (scopeS == "public" ? 2 : (scopeS == "near" ? 3 : 0));
        const int ri = registerOf(reg);
        int best = -1; double bs = 0; int from = 0;
        for (int yy = std::max(H.first, year - 4); yy <= year; yy++) {
            int n = H.eventsIn(yy, from);
            for (int k = 0; k < n; k++) {
                const int ev = from + k;
                const HistoryEvent& e = H.events[ev];
                if (scope == 3 && (!e.pub || e.place != home)) continue;
                if (ruleFor(run, reg, ev).empty()) continue;
                double s = salience(ev, scope == 3 ? 2 : scope, ri, 0.7 + 0.6 * run.uni());
                if (s > bs) { bs = s; best = ev; }
            }
        }
        if (best < 0) { if (run.has(reg + ".none")) return run.expand(reg + ".none"); return true; }
        return tell(run, reg, best);
    }
    // H-04's second pass: `@digest <private|public|any|near> <register> <n>`: the n most salient events `@news` would pick, told in the order
    // they happened (a newspaper's items, a letter's news: a war declared before its first battle), a paragraph between them (the register's
    // `.sep` instead where it has one); the register's `.none` when there is none
    bool digest(VgRun& run, const std::string& scopeS, const std::string& reg, int n) {
        const int scope = scopeS == "private" ? 1 : (scopeS == "public" ? 2 : (scopeS == "near" ? 3 : 0));
        const int ri = registerOf(reg);
        std::vector<std::pair<double, int>> cand; int from = 0;
        for (int yy = std::max(H.first, year - 4); yy <= year; yy++) {
            const int k = H.eventsIn(yy, from);
            for (int j = 0; j < k; j++) {
                const int ev = from + j; const HistoryEvent& e = H.events[ev];
                if (scope == 3 && (!e.pub || e.place != home)) continue;
                if (ruleFor(run, reg, ev).empty()) continue;
                const double sc = salience(ev, scope == 3 ? 2 : scope, ri, 0.7 + 0.6 * run.uni());
                if (sc > 0) cand.push_back({-sc, ev});
            }
        }
        std::sort(cand.begin(), cand.end());
        std::vector<int> pick;
        for (const auto& c : cand) {
            if ((int)pick.size() >= n) break;
            bool sameKind = false; for (int q : pick) if (H.events[q].kind == H.events[c.second].kind) sameKind = true;
            if (!sameKind) pick.push_back(c.second);
        }
        if (pick.empty()) { if (run.has(reg + ".none")) return run.expand(reg + ".none"); return true; }
        std::sort(pick.begin(), pick.end(), [&](int a, int b) { const HistoryEvent& x = H.events[a]; const HistoryEvent& z = H.events[b];
            return x.year != z.year ? x.year < z.year : (x.season != z.season ? x.season < z.season : a < b); });
        for (size_t i = 0; i < pick.size(); i++) {
            if (i > 0) { if (run.has(reg + ".sep")) run.expand(reg + ".sep"); else run.paragraph(); }
            tell(run, reg, pick[i]);
        }
        return true;
    }
    bool tell(VgRun& run, const std::string& reg, int ev) {
        const std::string r = ruleFor(run, reg, ev);
        if (r.empty()) return false;
        toldHere.push_back(ev);
        toldAll[ev]++; toldIn[ev] |= 1u << registerOf(reg);
        run.bindEntity("$e", eventEntity(ev));
        return run.expand(r);
    }
    // `@entries <register> <n>`: a year's events (the chronicle's year, else the record's), the most salient n, told in their seasons'
    // order (a war declared before its battle); the register's `.quiet` when the year had nothing
    bool entries(VgRun& run, const std::string& reg, int n) {
        const int y = theYear();
        int from = 0, cnt = H.eventsIn(y, from);
        std::vector<std::pair<double, int>> ev; int harvests = 0, deaths = 0;
        for (int k = 0; k < cnt; k++) {
            const int i = from + k; const HistoryEvent& e = H.events[i];
            if (!e.pub || (y == year && e.season > season)) continue;
            if (std::find(toldHere.begin(), toldHere.end(), i) != toldHere.end()) continue;
            if (e.kind == HE_HARVEST && harvests++) continue;
            if (e.kind == HE_DEATH && deaths++ >= 1) continue;
            if (std::find_if(ev.begin(), ev.end(), [&](const std::pair<double, int>& q) { return H.events[q.second].kind == e.kind && e.kind != HE_BATTLE; }) != ev.end()) continue;   // one of a kind a year
            if (ruleFor(run, reg, i).empty()) continue;
            double s = e.salience * (0.8 + 0.4 * run.uni());
            if (toldAll.count(i)) s *= 0.7;
            ev.push_back({-s, i});
        }
        std::sort(ev.begin(), ev.end());
        std::vector<int> chosen;
        for (const auto& p : ev) { if ((int)chosen.size() >= n) break; chosen.push_back(p.second); }
        std::stable_sort(chosen.begin(), chosen.end(), [&](int a, int b) { const HistoryEvent& x = H.events[a]; const HistoryEvent& z = H.events[b]; return x.season != z.season ? x.season < z.season : a < b; });
        if (chosen.empty()) { if (run.has(reg + ".quiet")) return run.expand(reg + ".quiet"); return true; }
        for (int i : chosen) tell(run, reg, i);
        return true;
    }
    // a year a chronicle tells: one with events its register has
    bool notable(const VgRun& run, int y, const std::string& reg) const {
        int from = 0, n = H.eventsIn(y, from);
        for (int k = 0; k < n; k++) {
            const HistoryEvent& e = H.events[from + k];
            if (!e.pub || e.kind == HE_HARVEST || e.kind == HE_FESTIVAL || (y == year && e.season > season)) continue;
            if (!ruleFor(run, reg, from + k).empty()) return true;
        }
        return false;
    }
    // `@years <n> <rule>`: a chronicle: up to n years with something in them, the last the record's own or before it, within its era, in
    // their order; each told by `<rule>` with the year's facts and ruler bound, the empty years between by `<rule>.gap`
    // D-05: `@suntable <n> <rule>`: a drifter's people's observatory's table of the sun's disc, up to n rows from the last year of the whole sun
    // (the year before the onset, or the record's year, if earlier) to the record's year, evenly between, each told by `<rule>` with its year (`y`) and the disc then (`sunsize`) in the
    // people's own parts of its first measure (`sunfrom`: 60, 100, 120, 32 or 24 by the people; the disc as the distance grew, the square root
    // of its light); nothing for a people that kept its sun
    bool suntable(VgRun& run, int n, const std::string& rule) {
        if (H.sunlight.empty() || n < 1) return false;
        static const int UNITS[5] = {60, 100, 120, 32, 24};
        const int units = UNITS[mix64(H.seed ^ 0xD055AULL) % 5];
        const int from = std::min(year, H.stageYear[STAGE_ONSET] - 1), to = std::max(from, year);   // from the last year of the whole sun
        const std::string saveY = facts[F_Y], saveS = facts[F_SUNSIZE], saveF = facts[F_SUNFROM];
        facts[F_SUNFROM] = num(units);
        const int rows = std::min(n, to - from + 1);
        int last = -1;
        for (int i = 0; i < rows; i++) {
            const int y = rows == 1 ? to : from + (int)std::lround((double)(to - from) * i / (rows - 1));
            if (y == last) continue;
            last = y;
            const double size = units * std::sqrt(std::max(0.0, H.sunlightAt(y)));
            char buf[32]; if (size >= 10) snprintf(buf, sizeof buf, "%d", (int)std::lround(size)); else snprintf(buf, sizeof buf, "%.1f", size);
            facts[F_Y] = num(y); facts[F_SUNSIZE] = buf;
            if (i > 0) run.paragraph();
            run.expand(rule);
        }
        facts[F_Y] = saveY; facts[F_SUNSIZE] = saveS; facts[F_SUNFROM] = saveF;
        return true;
    }
    bool years(VgRun& run, int n, const std::string& rule) {
        const int ei = H.eraAt(year);
        const int eraStart = ei >= 0 && ei < (int)H.eras.size() ? H.eras[ei].start : 1;
        const int lo = std::max({1, H.first, eraStart, year - 60});
        std::vector<int> ys;
        for (int y = year; y >= lo && (int)ys.size() < n; y--) if (notable(run, y, "annal")) ys.push_back(y);
        if ((int)ys.size() < std::min(n, 2)) {   // H-04's second pass: a sparse age (the far future's long quiet) reaches further back for its years
            ys.clear();
            const int lo2 = std::max({1, H.first, eraStart, year - 1500});
            for (int y = year; y >= lo2 && (int)ys.size() < n; y--) if (notable(run, y, "annal")) ys.push_back(y);
        }
        if (ys.empty()) ys.push_back(year);
        std::reverse(ys.begin(), ys.end());
        const std::vector<std::string> saved = facts;
        for (size_t i = 0; i < ys.size(); i++) {
            const int y = ys[i];
            if (i > 0 && y - ys[i - 1] > 1) {
                fixedYear = y; yearFacts(y);
                facts[F_Y0] = num(ys[i - 1] + 1); facts[F_Y1] = num(y - 1); facts[F_GAP] = num(y - ys[i - 1] - 1);
                run.paragraph();
                if (run.has(rule + ".gap")) run.expand(rule + ".gap");
            }
            if (i > 0) run.paragraph();
            fixedYear = y; yearFacts(y);
            facts[F_Y] = num(y);
            bound[E_RULER] = -1;   // the year's own ruler
            run.expand(rule);
        }
        // the facts as at the chronicle's last year (inventions only grow, so its words are allowed), the rest the record's
        const std::vector<std::string> lastYear = facts;
        facts = saved;
        for (int k = 0; k < INV_COUNT; k++) facts[F_INV0 + k] = lastYear[F_INV0 + k];
        fixedYear = 0; bound[E_RULER] = -1;
        return true;
    }
    // B-414: `@past <n> <rule>`: a late people's history of an age before the record's (a world shows only the age its people reached, so its
    // earlier ages are told by its own histories): one of its earlier eras since the reckoning began (year 1: the old time before it is the
    // chroniclers' round years, no heading counts them), the record's draw, the nearer the likelier; a run of up to n of
    // its years with something in them counting back from a year drawn in it, told in their order by `<rule>` with each year's facts and ruler
    // bound (the era's name, its government, its age, its inventions), the empty years between by `<rule>.gap`; `<rule>.open` before them and
    // `<rule>.close` after, with the first year's and the last year's facts. The record's own facts come back after
    bool past(VgRun& run, int n, const std::string& rule) {
        const int own = H.eraAt(year);
        std::vector<int> eras; double total = 0;
        for (int k = 0; k < own && k < (int)H.eras.size(); k++) if (H.eras[k].end >= std::max(1, H.first)) { eras.push_back(k); total += k + 1; }
        if (eras.empty()) return false;
        double u = run.uni() * total; int pick = eras.back();
        for (int k : eras) { u -= k + 1; if (u < 0) { pick = k; break; } }
        const HistoryEra& E = H.eras[pick];
        const int lo = std::max({1, H.first, E.start}), hi = std::min(E.end, year - 1);
        if (hi < lo) return false;
        const int anchor = lo + std::min(hi - lo, (int)(run.uni() * (hi - lo + 1)));
        std::vector<int> ys;
        for (int y = anchor; y >= std::max(lo, anchor - 60) && (int)ys.size() < n; y--) if (notable(run, y, "annal")) ys.push_back(y);
        if ((int)ys.size() < std::min(n, 2)) {   // a quiet stretch: the era's notable years further back, then forward
            ys.clear();
            for (int y = anchor; y >= lo && (int)ys.size() < n; y--) if (notable(run, y, "annal")) ys.push_back(y);
            for (int y = anchor + 1; y <= hi && (int)ys.size() < n; y++) if (notable(run, y, "annal")) ys.insert(ys.begin(), y);
        }
        if (ys.empty()) return false;
        std::sort(ys.begin(), ys.end());
        const std::vector<std::string> saved = facts;
        auto bindYear = [&](int y) { fixedYear = y; yearFacts(y); facts[F_Y] = num(y); bound[E_RULER] = -1; };
        bindYear(ys.front());
        if (run.has(rule + ".open")) { run.expand(rule + ".open"); run.paragraph(); }
        for (size_t i = 0; i < ys.size(); i++) {
            const int y = ys[i];
            if (i > 0 && y - ys[i - 1] > 1) {
                bindYear(y);
                facts[F_Y0] = num(ys[i - 1] + 1); facts[F_Y1] = num(y - 1); facts[F_GAP] = num(y - ys[i - 1] - 1);
                run.paragraph();
                if (run.has(rule + ".gap")) run.expand(rule + ".gap");
            }
            if (i > 0) run.paragraph();
            bindYear(y);
            run.expand(rule);
        }
        if (run.has(rule + ".close")) { bindYear(ys.back()); run.paragraph(); run.expand(rule + ".close"); }
        facts = saved;
        fixedYear = 0; bound[E_RULER] = -1;
        return true;
    }
    // `@death <prefix>`: the subject's death, by its cause (`<prefix>.<cause kind>`: the sickness, the battle) or its way (`<prefix>.old`,
    // `.young`, `.childbirth`), `<prefix>.any` else; the death bound as $e and its cause as $c
    bool death(VgRun& run, const std::string& prefix) {
        const int x = subject >= 0 ? subject : writer;
        if (x < 0) return false;
        const int dv = deathOf[x];
        if (dv >= 0) {
            const HistoryEvent& d = H.events[dv];
            toldDeathOf(x);   // H-06
            run.bindEntity("$e", eventEntity(dv));
            if (d.cause >= 0 && d.cause < (int)H.events.size()) {
                run.bindEntity("$c", eventEntity(d.cause));
                const std::string r = prefix + "." + KIND_KEYS[std::max(0, std::min(HE_COUNT - 1, H.events[d.cause].kind))];
                if (run.has(r)) { toldHere.push_back(d.cause); return run.expand(r); }
            }
            const int age = d.year - H.persons[x].born;
            std::string how = keyOf(DEATH_KEYS, d.thing);
            if (how == "old") how = age < 12 ? "young" : (age >= 60 ? "old" : "illness");
            if (run.has(prefix + "." + how)) return run.expand(prefix + "." + how);
        }
        if (run.has(prefix + ".any")) return run.expand(prefix + ".any");
        return true;
    }
    // `@deed <prefix>`: something of the subject's life the history holds (a work they made or saw finished at their home, a battle of
    // their town in their fighting years, their town's founding, their own deed or invention), `<prefix>.<kind>`; `<prefix>.trade` else
    bool deed(VgRun& run, const std::string& prefix) {
        const int x = subject >= 0 ? subject : writer;
        if (x < 0) return false;
        const HistoryPerson& P = H.persons[x];
        std::vector<int> cand;
        for (int ev : H.eventsOf(x)) {
            const HistoryEvent& e = H.events[ev];
            if ((e.kind == HE_BUILT || e.kind == HE_DEED || e.kind == HE_INVENTION || e.kind == HE_DISCOVERY || e.kind == HE_FOUNDING || e.kind == HE_BATTLE) && e.a == x) cand.push_back(ev);
        }
        if (cand.empty()) {
            const int hm = homeAt(x, P.died);
            int from = 0;
            for (int y = std::max(H.first, P.born + 18); y <= std::min(P.died, H.last) && y <= year; y++) {
                int n = H.eventsIn(y, from);
                for (int k = 0; k < n; k++) {
                    const HistoryEvent& e = H.events[from + k];
                    if ((e.kind == HE_BUILT && e.place == hm) || (e.kind == HE_BATTLE && e.place == hm && y - P.born <= 50) || (e.kind == HE_FOUNDING && e.place == hm)) cand.push_back(from + k);
                }
            }
        }
        if (!cand.empty() && run.uni() < 0.8) {
            const int ev = cand[std::min((int)cand.size() - 1, (int)(run.uni() * cand.size()))];
            const std::string r = prefix + "." + KIND_KEYS[std::max(0, std::min(HE_COUNT - 1, H.events[ev].kind))];
            if (run.has(r)) { run.bindEntity("$d", eventEntity(ev)); toldHere.push_back(ev); return run.expand(r); }
        }
        if (run.has(prefix + ".trade")) return run.expand(prefix + ".trade");
        return true;
    }
    // `@life <prefix> <n>`: up to n of the subject's own events in their order (a marriage, a child, a move, a deed), `<prefix>.<kind>`
    bool life(VgRun& run, const std::string& prefix, int n) {
        const int x = subject >= 0 ? subject : writer;
        if (x < 0) return false;
        std::vector<int> evs;
        for (int ev : H.eventsOf(x)) {
            const HistoryEvent& e = H.events[ev];
            if (e.year > year || (e.kind == HE_BIRTH && e.a == x) || e.kind == HE_DEATH) continue;
            if (e.kind == HE_BIRTH && e.b != x && e.c != x) continue;
            if (!run.has(prefix + "." + KIND_KEYS[std::max(0, std::min(HE_COUNT - 1, e.kind))])) continue;
            evs.push_back(ev);
        }
        if ((int)evs.size() > n) {   // keep n, in order: the first, the last and between
            std::vector<int> keep;
            for (int i = 0; i < n; i++) keep.push_back(evs[(size_t)((double)i * (evs.size() - 1) / std::max(1, n - 1))]);
            keep.erase(std::unique(keep.begin(), keep.end()), keep.end());
            evs = keep;
        }
        for (int ev : evs) { toldHere.push_back(ev); run.bindEntity("$e", eventEntity(ev)); run.expand(prefix + "." + KIND_KEYS[std::max(0, std::min(HE_COUNT - 1, H.events[ev].kind))]); }
        if (evs.empty() && run.has(prefix + ".none")) return run.expand(prefix + ".none");
        return true;
    }
    // `@event <kinds|any> <register> <years>`: the most salient public event of those kinds in the years before the record (a ballad's
    // battle, a painted flood, a hymn's sickness), in `<register>.<kind>`; `<register>.none` else
    bool event(VgRun& run, const std::string& kinds, const std::string& reg, int span) {
        std::set<std::string> want; { size_t p = 0; while (p < kinds.size()) { size_t q = kinds.find(',', p); if (q == std::string::npos) q = kinds.size(); want.insert(kinds.substr(p, q - p)); p = q + 1; } }
        const bool any = want.count("any") > 0;
        const int ri = registerOf(reg);
        int best = -1; double bs = 0; int from = 0;
        for (int yy = std::max(H.first, year - span); yy <= year; yy++) {
            int n = H.eventsIn(yy, from);
            for (int k = 0; k < n; k++) {
                const int ev = from + k; const HistoryEvent& e = H.events[ev];
                if (!e.pub || (yy == year && e.season > season)) continue;
                if (!any && !want.count(KIND_KEYS[std::max(0, std::min(HE_COUNT - 1, e.kind))])) continue;
                if (ruleFor(run, reg, ev).empty()) continue;
                if (std::find(toldHere.begin(), toldHere.end(), ev) != toldHere.end()) continue;
                double s = std::max(0.05, (double)e.salience) * (1.0 - 0.5 * (year - yy) / std::max(1, span)) * (0.7 + 0.6 * run.uni());
                if (e.place == home) s *= 1.4;
                auto it = toldAll.find(ev);
                if (it != toldAll.end()) { auto in = toldIn.find(ev); s *= e.salience >= 0.7f && in != toldIn.end() && !(in->second & (1u << ri)) && it->second < 3 ? 1.3 : 0.5; }
                if (s > bs) { bs = s; best = ev; }
            }
        }
        if (best < 0) { if (run.has(reg + ".none")) return run.expand(reg + ".none"); return true; }
        return tell(run, reg, best);
    }
};

}   // namespace

// ---- the public functions ------------------------------------------------------------------------------------------------------------

std::vector<std::string> recordGenreNames(const VgGrammar& G) {   // "genre.letter.far" -> "letter"
    std::vector<std::string> out;
    int r = G.rule("record"); if (r < 0) return out;
    for (int ai : G.rules[r].alts) {
        std::string n = "?";
        for (const VgNode& x : G.alts[ai].body) if (x.type == VgNode::RULE) { n = x.s; break; }
        if (n.rfind("genre.", 0) == 0) n = n.substr(6);
        size_t d = n.find('.'); if (d != std::string::npos) n = n.substr(0, d);
        for (char& c : n) if (c == '-') c = ' ';
        out.push_back(n);
    }
    return out;
}

bool recordNameKnown(const History& H, const Lore& L, int year, const std::string& name) {
    for (const std::string* s : {&L.people, &L.god, &L.river, &L.mountain, &L.sea, &L.city, &L.city2, &L.moon, &L.star, &L.festival, &L.other, &L.founder})
        if (!s->empty() && *s == name) return true;
    for (int gov = 0; gov < GOV_COUNT; gov++) if (historyTitleNative(H, gov)) for (bool f : {false, true}) {   // H-04's second pass: a ruler's title in their tongue
        std::string t = historyRulerTitle(H, gov, f); if (!t.empty()) t[0] = (char)std::toupper((unsigned char)t[0]);
        if (t == name) return true;
    }
    for (int p = 0; p < (int)H.places.size(); p++) if (H.places[p].founded <= year && historyPlaceName(H, p) == name) return true;
    for (int p = 0; p < (int)H.persons.size(); p++) {   // a person by the whole name, or by the given name a letter calls them by
        if (H.persons[p].born > year) continue;
        const std::string full = historyPersonName(H, p, false);
        if (full == name || (full.size() > name.size() && full.compare(0, name.size(), name) == 0 && full[name.size()] == ' ')) return true;
    }
    if (!H.places.empty() && historyPlaceName(H, 0) == name) return true;   // the capital's name before its founding (the old time's "where Kauza would be")
    return false;
}

void writeRecordTexts(const StarSystem& sys0, const Body& b0, const BodyGen& g, const Lore& L, const VgGrammar& G, ShardTexts& out) {
    const PeoplesWorld PW = peoplesWorldOf(sys0, b0);   // D-05: a drifter's people's world at its former orbit
    const StarSystem& sys = *PW.sys; const Body& b = *PW.b;
    out.items.assign(std::max(0, L.records), ShardText());
    const double t0 = nowMs();
    std::shared_ptr<const History> Hp = historyOf(sys, b, g, L.which);
    if (!Hp) return;
    const double tHist = nowMs();
    const History& H = *Hp;
    std::shared_ptr<const RecordCatalogue> C = recordsOf(g, L.which);
    std::shared_ptr<const RecordHomes> homes = recordHomesOf(g, L.which);
    RecordModel M(H, L);
    recordWorldFacts(sys, b, g, L, &H, M);
    const Culture cult = cultureOf(g, L.which);   // H-04's second pass: the people's buildings, its dialect and its own words
    M.culture0 = &cult;
    M.culture = mix64(L.seed ^ 0xC0170E5ULL);
    std::vector<int> placeOfRank;   // the plan's rank -> the history's place
    for (int p = 0; p < (int)H.places.size(); p++) { int r = H.places[p].plan; if (r >= 0) { if (r >= (int)placeOfRank.size()) placeOfRank.resize(r + 1, -1); placeOfRank[r] = p; } }
    VgMemory mem;
    std::set<std::string> told;   // a world never repeats a text whole
    for (int i = 0; i < (int)out.items.size(); i++) {
        ShardText& it = out.items[i];
        Shard s = shardDrawOf(g, L, i);
        it.age = C && i < C->count ? C->entries[i].age : H.profile.age;
        it.year = s.year;
        if (s.music || s.chart) continue;
        const int medium = C && i < C->count ? C->entries[i].medium : RM_STONE;
        int rank = homes && i < (int)homes->rank.size() ? homes->rank[i] : -1;
        int home = rank >= 0 && rank < (int)placeOfRank.size() ? placeOfRank[rank] : -1;
        if (home < 0 || !H.placeStands(home, s.year)) home = historySeat(H, s.year);
        int season = 0;
        if (L.months > 0 && s.month > 0) season = std::min(H.seasons - 1, (s.month - 1) * H.seasons / std::max(1, L.months));
        else season = (int)(mix64(s.seed ^ 0x5EA50ULL) % (uint64_t)std::max(1, H.seasons));
        for (int attempt = 0; attempt < 8; attempt++) {
            VgMemory before = mem; auto toldAll = M.toldAll; auto toldIn = M.toldIn;
            M.beginRecord(s.year, season, it.age, medium, home, s.last, s.tone, s.seed + (uint64_t)attempt * 0x9E3779B97F4A7C15ULL, s.month, s.day);
            it.rec = VgRecord();
            it.rec.seed = mix64(s.seed ^ 0x40E04ULL) + (uint64_t)attempt * 0x9E3779B97F4A7C15ULL;
            it.rec.tense = VG_PRESENT;
            vgRecord(G, "record", M, &mem, it.rec);
            if (!told.count(it.rec.text) || attempt == 7) break;
            mem = before; M.toldAll = toldAll; M.toldIn = toldIn;   // a draw thrown away leaves no mark
        }
        told.insert(it.rec.text);
        it.text = it.rec.text;
        it.kind = it.rec.root;
        it.told = M.toldHere;
        for (int ev : M.toldLegend) if (std::find(it.told.begin(), it.told.end(), ev) == it.told.end()) it.told.push_back(ev);   // H-06: a death an epitaph is about
        if (s.last) for (int ev = (int)H.events.size() - 1; ev >= 0; ev--) if (H.events[ev].kind == HE_END) { if (std::find(it.told.begin(), it.told.end(), ev) == it.told.end()) it.told.push_back(ev); break; }   // H-06: the last recording tells the end
        it.writer = M.writer; it.reader = M.reader; it.subject = M.subject;
        for (int a : it.rec.used) {   // H-06: the people's ways it expressed (its drawn alternatives' guards on them, as the model holds them)
            if (a < 0 || a >= (int)G.alts.size()) continue;
            for (const VgCond& c : G.alts[a].conds) {
                auto take = [&](const VgCond& k) {
                    if (k.op != 0 || k.kind != 0 || k.ref < 0 || k.ref >= (int)M.facts.size() || k.val != M.facts[k.ref]) return;
                    if (k.ref != F_FAITH && k.ref != F_RITE && k.ref != F_HOLY && k.ref != F_ECONOMY && k.ref != F_ECONOMY2 && k.ref != F_VALUE && k.ref != F_VALUE2 && k.ref != F_DEADWAY && k.ref != F_CAUSE) return;
                    const std::pair<std::string, std::string> w(FACT_NAMES[k.ref], k.val);
                    if (std::find(it.ways.begin(), it.ways.end(), w) == it.ways.end()) it.ways.push_back(w);
                };
                take(c); for (const VgCond& o : c.alt) take(o);
            }
        }
    }
    if (getenv("VESPERIS_TRACE")) fprintf(stderr, "records: history %.1f ms, texts %.1f ms (%d)\n", tHist - t0, nowMs() - tHist, (int)out.items.size());
}
