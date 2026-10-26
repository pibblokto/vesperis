// E-10: the history's names: the event kinds', roles', wars' and institutions' (history.h's tables), a place's, a person's, a
// ruler's, a war's, a sickness's, a year's, a trade's and a work's, and each event's line (`historyEventLine`)
#include "history_tables.h"
#include "tongue.h"

const char* const HISTORY_EVENT_NAMES[] = {"coming", "first", "stones", "founding", "accession", "rule end", "regency", "government", "war", "battle", "siege",
    "sack", "peace", "sickness", "harvest", "famine", "flood", "drought", "quake", "eruption", "impact", "flare", "winter", "built", "way", "festival", "omen",
    "invention", "discovery", "faith", "institution", "deed", "unrest", "migration", "abandoned", "contact", "treaty", "other end", "sign", "stage", "exodus",
    "end", "fair", "fire", "storm", "pest", "wonder", "games", "law", "trial", "embassy", "accident", "price", "beasts", "show", "visit", "sun", "birth", "marriage", "move", "death",
    "illness", "injury", "apprentice", "house", "quarrel", "journey", "betrothal"};
static_assert(sizeof(HISTORY_EVENT_NAMES) / sizeof(HISTORY_EVENT_NAMES[0]) == HE_COUNT, "a name an event kind");
const char* const HISTORY_ROLE_NAMES[HR_COUNT] = {"", "ruler", "regent", "founder", "general", "prophet", "maker", "inventor", "explorer", "rebel", "envoy", "artist", "the last"};
const char* const HISTORY_WAR_KINDS[HW_COUNT] = {"a war between bands", "a war between cities", "a rising", "a war of succession", "a revolution", "a civil war",
                                                 "a war with the other people", "a conquest"};
const char* const HISTORY_INSTITUTION_KINDS[HI_COUNT] = {"the council", "the priesthood", "the guilds", "the academy", "the company", "the gazette", "the university",
                                                         "the assembly", "the broadcaster", "the network", "the mind", "the army"};

namespace {
std::string withCommas(int64_t n) {
    std::string s = std::to_string(n < 0 ? -n : n), out;
    for (size_t i = 0; i < s.size(); i++) { if (i > 0 && (s.size() - i) % 3 == 0) out += ','; out += s[i]; }
    return n < 0 ? "-" + out : out;
}
std::string ordinal(int n) {
    const int m100 = n % 100, m10 = n % 10;
    const char* suf = (m100 >= 11 && m100 <= 13) ? "th" : (m10 == 1 ? "st" : (m10 == 2 ? "nd" : (m10 == 3 ? "rd" : "th")));
    return withCommas(n) + suf;
}
std::string fillText(const History& H, std::string s, int place, int year = 1 << 30) {
    auto rep = [&](const char* key, const std::string& v) { size_t at; while ((at = s.find(key)) != std::string::npos) s.replace(at, std::string(key).size(), v); };
    rep("{place}", place >= 0 ? historyPlaceName(H, place) : historyPlaceName(H, 0));
    rep("{capital}", H.places.empty() ? H.lore.city : historyPlaceName(H, historySeat(H, year)));
    rep("{river}", H.lore.river); rep("{sea}", H.lore.sea); rep("{mountain}", H.lore.mountain); rep("{god}", H.lore.god);
    rep("{other}", H.lore.other.empty() ? std::string("the enemy") : "the " + H.lore.other);
    return s;
}
}   // namespace

std::string historyInventionName(int id) { return id >= 0 && id < INVENTION_COUNT ? INVENTIONS[id].name : "something"; }
int historyInventionIndex(const char* name) { return inventionIndex(name); }

// ---- the names and the lines ----------------------------------------------------------------------------------------------------
int historySeat(const History& H, int year) {
    for (int p = 0; p < (int)H.places.size(); p++) if (H.places[p].sclass == SC_CAPITAL && H.placeStands(p, year)) return p;
    for (int p = 0; p < (int)H.places.size(); p++) if (H.placeStands(p, year)) return p;
    for (int p = 0; p < (int)H.places.size(); p++) if (H.places[p].sclass == SC_CAPITAL) return p;   // before the founding: the capital to be
    return H.places.empty() ? -1 : 0;
}
std::string historyPlaceName(const History& H, int place) {
    if (place < 0 || place >= (int)H.places.size()) return "the land";
    const uint64_t s = H.places[place].name;
    if (s == NAME_CITY) return H.lore.city;
    if (s == NAME_CITY2) return H.lore.city2;
    return tongueName(tongueOfSeed(H.lore.seed), s, tongueNameKindOfRank(H.places[place].sclass));   // H-04's second pass: the tongue's
}
std::string historyPersonName(const History& H, int person, bool withTitle) {
    if (person < 0 || person >= (int)H.persons.size()) return "someone";
    const HistoryPerson& P = H.persons[person];
    std::string nm = P.name == NAME_FOUNDER ? H.lore.founder : tongueName(tongueOfSeed(H.lore.seed), P.name, TN_PERSON, P.female);
    if (P.ordinal > 0) nm += " " + romanNumeral(P.ordinal);
    // a surname from the industrial age, when the registers keep them (a ruling house is named for its founder instead)
    if (P.family >= 0 && P.family < (int)H.families.size() && !H.families[P.family].house && P.born + 20 >= 1) {
        const int e = H.eraAt(P.born + 20);
        if (e >= 0 && H.eras[e].age >= AGE_INDUSTRIAL) nm += " " + historyFamilyName(H, P.family);
    }
    if (withTitle && P.role == HR_RULER) {
        for (const HistoryReign& R : H.reigns) if (R.ruler == person) {
            std::string t = historyRulerTitle(H, R.government, P.female);
            if (!t.empty() && t.compare(0, 4, "the ") != 0) for (size_t i = 0; i < t.size(); i++) if (i == 0 || t[i - 1] == ' ') t[i] = (char)toupper(t[i]);
            return t + " " + nm;
        }
    }
    return nm;
}
// H-04's second pass: a people's title for whoever held the rule (each people its own of the government's: a headman, a war-leader, an
// archon; a monarchy's ruler often by a word of their own tongue, as ours keep a khan or a pharaoh)
std::string historyRulerTitle(const History& H, int gov, bool female) {
    struct TitleSet { const char* m; const char* f; };
    static const TitleSet T[GOV_COUNT][6] = {
        {{"chief", "chief"}, {"headman", "headwoman"}, {"elder", "elder"}, {"speaker", "speaker"}, {"war-leader", "war-leader"}, {nullptr, nullptr}},
        {{"king", "queen"}, {"high king", "high queen"}, {"prince", "princess"}, {"lord", "lady"}, {"emperor", "empress"}, {nullptr, nullptr}},
        {{"first elder", "first elder"}, {"first councillor", "first councillor"}, {"archon", "archon"}, {"warden", "warden"}, {"provost", "provost"}, {nullptr, nullptr}},
        {{"high priest", "high priestess"}, {"oracle", "oracle"}, {"holy father", "holy mother"}, {"keeper", "keeper"}, {"prophet", "prophetess"}, {nullptr, nullptr}},
        {{"consul", "consul"}, {"president", "president"}, {"first citizen", "first citizen"}, {"chancellor", "chancellor"}, {"magistrate", "magistrate"}, {nullptr, nullptr}},
        {{"speaker", "speaker"}, {"chair", "chair"}, {"first delegate", "first delegate"}, {"convener", "convener"}, {nullptr, nullptr}, {nullptr, nullptr}},
        {{"director", "director"}, {"chairman", "chairwoman"}, {"chief executive", "chief executive"}, {"president", "president"}, {nullptr, nullptr}, {nullptr, nullptr}},
        {{"the mind", "the mind"}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}}};
    gov = clampi(gov, 0, GOV_COUNT - 1);
    const uint64_t h = mix64(H.lore.seed ^ 0x717E5ULL ^ (uint64_t)(gov + 1) * 0x9E3779B97F4A7C15ULL);
    const bool native = (gov == GOV_MONARCHY || gov == GOV_CHIEFS || gov == GOV_THEOCRACY) && unitFromHash(h) < (gov == GOV_MONARCHY ? 0.45 : 0.25);
    if (native) {   // their own word (the decoder keeps it as a name)
        const Tongue& TG = tongueOfSeed(H.lore.seed);
        std::string w = tongueTerm(TG, gov == GOV_MONARCHY ? (female ? "queen" : "king") : (gov == GOV_CHIEFS ? "chief" : (female ? "priestess" : "priest")));
        if (female && gov != GOV_MONARCHY && !TG.femEnd.empty() && TG.femEnd.size() <= 2) w += TG.femEnd;
        return w;
    }
    int n = 0; while (n < 6 && T[gov][n].m) n++;
    const int k = (int)((h >> 8) % (uint64_t)std::max(1, n));
    return female ? T[gov][k].f : T[gov][k].m;
}
bool historyTitleNative(const History& H, int gov) {
    gov = clampi(gov, 0, GOV_COUNT - 1);
    const uint64_t h = mix64(H.lore.seed ^ 0x717E5ULL ^ (uint64_t)(gov + 1) * 0x9E3779B97F4A7C15ULL);
    return (gov == GOV_MONARCHY || gov == GOV_CHIEFS || gov == GOV_THEOCRACY) && unitFromHash(h) < (gov == GOV_MONARCHY ? 0.45 : 0.25);
}
std::string historyFamilyName(const History& H, int family) {
    if (family < 0 || family >= (int)H.families.size()) return "";
    return tongueName(tongueOfSeed(H.lore.seed), H.families[family].name, TN_FAMILY);
}
std::string historyWarName(const History& H, int war) {
    if (war < 0 || war >= (int)H.wars.size()) return "the war";
    const HistoryWar& w = H.wars[war];
    const int pick = (int)(w.name % (sizeof(WAR_NAMES) / sizeof(WAR_NAMES[0])));
    switch (w.kind) {
        case HW_REBELLION: return "the Rising of " + historyPlaceName(H, w.sideB);
        case HW_REVOLUTION: return "the Revolution of " + std::to_string(w.start);
        case HW_SUCCESSION: return "the War of the Two Claimants";
        case HW_OTHER: return "the War with the " + (H.lore.other.empty() ? std::string("Others") : H.lore.other);
        case HW_CONQUEST: return "the Conquest";
        case HW_BANDS: return "the Feud of " + historyPlaceName(H, w.sideA) + " and " + historyPlaceName(H, w.sideB);
        default: break;
    }
    if (w.name % 3 == 0 && w.sideB >= 0) return "the " + historyPlaceName(H, w.sideB) + " War";
    {   // H-04's second pass: each people its own way with the rest (its own third of the names, by its seed)
        const int nw = (int)(sizeof(WAR_NAMES) / sizeof(WAR_NAMES[0]));
        const uint64_t ps = mix64(H.lore.seed ^ 0x3A2ULL);
        return WAR_NAMES[(int)((ps + (uint64_t)pick * 3) % (uint64_t)nw)];
    }
}
std::string historySicknessName(const History& H, int sickness) {
    if (sickness < 0 || sickness >= (int)H.sicknesses.size()) return "a sickness";
    const HistorySickness& S = H.sicknesses[sickness];
    if (S.made) return "the made sickness";
    const uint64_t s = S.name;
    // H-04's second pass: a people's own looks and kinds (four looks and three kinds of the tables, by its seed), and a place's or a river's
    // name for one sickness in four (where it first struck, as ours name a flu)
    const uint64_t ps = mix64(H.lore.seed ^ 0x51C4ULL);
    const int nl = (int)(sizeof(SICK_LOOK) / sizeof(SICK_LOOK[0])), nk = (int)(sizeof(SICK_KIND) / sizeof(SICK_KIND[0]));
    const int look = (int)((ps + (s % 4) * 7) % (uint64_t)nl), kind = (int)(((ps >> 16) + ((s >> 8) % 3) * 5) % (uint64_t)nk);
    if (((s >> 24) & 3) == 0) {
        int at = -1;
        for (const HistoryEvent& e : H.events) if (e.kind == HE_SICKNESS && e.thing == sickness && e.place >= 0) { at = e.place; break; }
        if (at >= 0) return "the " + historyPlaceName(H, at) + " " + SICK_KIND[kind];
        return "the " + H.lore.river + " " + SICK_KIND[kind];
    }
    return std::string("the ") + SICK_LOOK[look] + " " + SICK_KIND[kind];
}
std::string historyYear(const History& H, int year) {
    (void)H;
    if (year >= 1) return "year " + withCommas(year);
    if (year == 0) return "the year before the first";
    return "about " + withCommas(-year) + " years before the first";
}
std::string historySeason(const History& H, int season) {
    if (season < 0) return "";
    if (H.seasons == 4) return SEASONS4[season & 3];
    return SEASONS2[H.lore.desert ? 1 : 0][season & 1];
}
std::string historyTradeName(int trade) { return TRADES[clampi(trade / 10, 0, AGE_COUNT - 1)][clampi(trade % 10, 0, 9)]; }
std::string historyTradePlural(int trade) {
    std::string t = historyTradeName(trade);
    if (t.size() > 3 && t.compare(t.size() - 3, 3, "man") == 0) return t.substr(0, t.size() - 3) + "men";
    return t + "s";
}

namespace {
std::string workName(const History& H, int k) {
    const Genome G = genomeOf(H.seed);
    switch (k) {
        case WK_HOLY: { static const char* const HOLY[HOLY_COUNT] = {"a temple", "a sacred grove", "a high place", "a shrine at the spring", "a cave shrine", "a house of the god", "the great tombs", "an open altar", "a hall of the faith"}; return HOLY[clampi(H.profile.holy, 0, HOLY_COUNT - 1)]; }
        case WK_TOMB: { static const char* const DEAD[5] = {"a barrow", "a tomb", "a cemetery", "a tower of the dead", "a tomb"}; return DEAD[clampi(G.of(GV_DEAD), 0, 4)]; }
        case WK_MONUMENT: { static const char* const MN[5] = {"a row of stelae", "an obelisk", "a colossus", "a triumphal arch", "a pillar"}; return MN[clampi(G.of(GV_MONUMENTS), 0, 4)]; }
        case WK_FOCUS: { static const char* const FO[10] = {"a great spire", "a great dome", "a great gate", "a great tower", "a colossus", "a great mast", "a great ring", "a great obelisk", "a sacred pool", "a planted tree"}; return FO[clampi(G.of(GV_FOCUS), 0, 9)]; }
        default: return WORKS[clampi(k, 0, WK_COUNT - 1)].name;
    }
}
std::string cap1(std::string s) { if (!s.empty()) s[0] = (char)toupper(s[0]); return s; }
std::string people(const History& H) { return "the " + H.lore.people; }
std::string other(const History& H) { return "the " + (H.lore.other.empty() ? std::string("others") : H.lore.other); }
std::string dead(int64_t n) { return n <= 0 ? std::string() : "; " + withCommas(n) + " died"; }
const char* ofKind(int kind) {
    switch (kind) {
        case NK_PATH: return "path"; case NK_ROAD: return "road"; case NK_RAIL: return "railway"; case NK_CANAL: return "canal";
        case NK_HIGHWAY: return "highway"; case NK_MAGLEV: return "maglev"; default: return "crossing";
    }
}
}

std::string historyWorkName(const History& H, int work) { return workName(H, work); }   // H-04
const char* historyEverydayKey(int kind, int thing) {   // H-04's second pass
    for (int k = 0; k < EVERYDAY_N; k++) if (EVERYDAY[k].kind == kind) return thing >= 0 && thing < EVERYDAY[k].n ? EVERYDAY[k].words[thing].key : EVERYDAY[k].words[0].key;
    switch (kind) {
        case HE_ILLNESS: return wordAt(PRIVATE_ILLNESS, thing);
        case HE_INJURY: return wordAt(PRIVATE_INJURY, thing);
        case HE_HOUSE: return wordAt(PRIVATE_HOUSE, thing);
        case HE_QUARREL: return wordAt(PRIVATE_QUARREL, thing);
        case HE_JOURNEY: return wordAt(PRIVATE_JOURNEY, thing);
        default: return "";
    }
}
const char* historySignPattern(int cause, int stage, int alt) { return SCRIPTS[clampi(cause, 0, CAUSE_COUNT - 1)].signs[clampi(stage, 0, STAGE_COUNT - 1)][alt & 1]; }
const char* historyLeftWhy(int cause) { return LEFT_BY_CAUSE[clampi(cause, 0, CAUSE_COUNT - 1)]; }

std::string historyKinship(const History& H, int from, int to) {
    if (from < 0 || to < 0 || from >= (int)H.persons.size() || to >= (int)H.persons.size() || from == to) return "";
    const HistoryPerson& A = H.persons[from]; const HistoryPerson& B = H.persons[to];
    const bool f = A.female;
    auto parentsOf = [&](int p) { return std::pair<int, int>(H.persons[p].mother, H.persons[p].father); };
    auto isParent = [&](int p, int c) { return c >= 0 && (H.persons[c].mother == p || H.persons[c].father == p); };
    auto siblings = [&](int a, int b) { const auto pa = parentsOf(a), pb = parentsOf(b); return (pa.first >= 0 && pa.first == pb.first) || (pa.second >= 0 && pa.second == pb.second); };
    if (A.spouse == to || B.spouse == from) return f ? "wife" : "husband";
    if (isParent(from, to)) return f ? "mother" : "father";
    if (isParent(to, from)) return f ? "daughter" : "son";
    if (siblings(from, to)) return f ? "sister" : "brother";
    const auto pb = parentsOf(to), pa = parentsOf(from);
    for (int p : {pb.first, pb.second}) if (p >= 0 && isParent(from, p)) return f ? "grandmother" : "grandfather";
    for (int p : {pa.first, pa.second}) if (p >= 0 && isParent(to, p)) return f ? "granddaughter" : "grandson";
    for (int p : {pb.first, pb.second}) if (p >= 0 && siblings(from, p)) return f ? "aunt" : "uncle";
    for (int p : {pa.first, pa.second}) if (p >= 0 && siblings(to, p)) return f ? "niece" : "nephew";
    for (int p : {pa.first, pa.second}) for (int q : {pb.first, pb.second}) if (p >= 0 && q >= 0 && siblings(p, q)) return "cousin";
    if (B.spouse >= 0 && isParent(from, B.spouse)) return f ? "mother-in-law" : "father-in-law";
    if (A.spouse >= 0 && isParent(to, A.spouse)) return f ? "daughter-in-law" : "son-in-law";
    if (B.spouse >= 0 && siblings(from, B.spouse)) return f ? "sister-in-law" : "brother-in-law";
    if (A.spouse >= 0 && siblings(to, A.spouse)) return f ? "sister-in-law" : "brother-in-law";
    return "";
}

std::string historyEventLine(const History& H, int i, const HistoryPersonNamer* namer) {
    if (i < 0 || i >= (int)H.events.size()) return "";
    const HistoryEvent& e = H.events[i];
    // H-06: a person by the namer where one is given (the decoder's history page: only the persons the records named), else by their name
    auto P = [&](int p) { return namer ? (*namer)(p, true) : historyPersonName(H, p, true); };
    auto N = [&](int p) { return namer ? (*namer)(p, false) : historyPersonName(H, p, false); };
    auto L = [&](int p) { return historyPlaceName(H, p); };
    const std::string at = e.place >= 0 ? " at " + L(e.place) : "";
    std::string s;
    switch (e.kind) {
        case HE_COMING: {
            static const char* const WH[8] = {"from beyond {mountain}", "over {sea}", "out of the north", "out of the south", "out of the east", "out of the west", "down {river}", "across the dry country"};
            s = people(H) + " came into the land " + fillText(H, WH[clampi(e.thing, 0, 7)], -1, e.year); break;
        }
        case HE_FIRST: {
            static const char* const FI[6] = {"the first fields were sown", "the first herds were kept", "the first boats were made", "the first pots were fired", "the first copper was worked", "the first village stood where {capital} would be"};
            s = fillText(H, FI[clampi(e.thing, 0, 5)], -1, e.year); break;
        }
        case HE_STONES: s = "the stones were raised where " + L(0) + " would be" + (e.a >= 0 ? ", by " + P(e.a) : ""); break;
        case HE_FOUNDING: {
            static const char* const WHY[13] = {"", " as a fort", " as a harbour", " at a crossing of the river", " at a mine", " round a market", " round a shrine", " round a station", " as the gathering place", " as a band's winter seat", " as a great place", "", ""};
            if (e.thing == 11) { s = "the work at " + L(e.place) + " was begun" + (e.a >= 0 ? " by " + P(e.a) : ""); break; }
            if (e.thing == 12 || (e.place == 0 && e.year == 1)) { s = P(e.a) + " founded " + L(e.place) + (e.year == 1 ? ", and the years were counted from it" : ""); break; }
            s = P(e.a) + " founded " + L(e.place) + WHY[clampi(e.thing, 0, 12)] + (e.place2 >= 0 ? ", with settlers from " + L(e.place2) : ""); break;
        }
        case HE_ACCESSION: {
            const int g = e.value >= 0 && e.value < (int)H.reigns.size() ? H.reigns[e.value].government : GOV_CHIEFS;
            const std::string t = e.a >= 0 ? historyRulerTitle(H, g, H.persons[e.a].female) : "the mind";
            const std::string nm = N(e.a);
            switch (e.thing) {
                case 0: s = nm + " ruled as " + t; break;
                case 1: {
                    const std::string kin = e.b >= 0 ? historyKinship(H, e.b, e.a) : "";
                    s = nm + " became " + t + (e.b >= 0 ? " after " + N(e.b) + (kin.empty() ? "" : std::string(H.persons[e.a].female ? ", her " : ", his ") + kin) : "");
                    break;
                }
                case 2: s = nm + " was chosen " + t; break;
                case 3: s = nm + " was elected " + t; break;
                case 4: s = nm + " seized the rule as " + t; break;
                case 5: s = nm + " was appointed " + t; break;
                case 6: s = nm + " won the rule in the war and ruled as " + t; break;
                case 7: s = nm + " came of age and ruled alone" + (e.b >= 0 ? ", after the regency of " + N(e.b) : ""); break;
                case 8: s = "the rule was given to the mind"; break;
                case 9: s = nm + " was raised by the revolution as " + t; break;
                default: s = nm + " was elected " + t + " again"; break;
            }
            break;
        }
        case HE_RULE_END: {
            const std::string nm = P(e.a);
            switch (e.thing) {
                case 1: s = nm + " was deposed"; break;
                case 2: s = nm + " stepped down"; break;
                case 3: s = nm + "'s term ended"; break;
                case 4: s = nm + " was overthrown"; break;
                case 5: s = nm + " fled"; break;
                case 6: s = e.a >= 0 ? "the rule ended with " + nm + ", the last to hold it" : "the rule ended"; break;
                default: s = nm + " gave the rule over"; break;
            }
            break;
        }
        case HE_REGENCY: s = N(e.a) + " ruled for the child " + P(e.b); break;
        case HE_GOVERNMENT: {
            static const char* const HOW[9] = {" by a reform", " by a revolution", ": a kingdom was founded", ": the priests took the rule", ": the companies bought the rule", ": the machine was given the rule", " by a coup", ": the elders took the rule", ": the assembly was called"};
            s = "the rule passed from " + std::string(GOVERNMENT_NAMES[clampi((int)e.value, 0, GOV_COUNT - 1)]) + " to " + GOVERNMENT_NAMES[clampi(e.thing, 0, GOV_COUNT - 1)] + HOW[clampi(e.out, 0, 8)] + (e.a >= 0 ? ", led by " + N(e.a) : "");
            break;
        }
        case HE_WAR: {
            const HistoryWar& w = H.wars[clampi(e.thing, 0, (int)H.wars.size() - 1)];
            const std::string nm = historyWarName(H, e.thing);
            switch (w.kind) {
                case HW_REBELLION: s = L(w.sideB) + " rose against " + (H.reigns.empty() ? std::string("the rule") : "the rule of " + L(historySeat(H, e.year))) + " (" + nm + ")"; break;
                case HW_CIVIL: s = "civil war: " + L(w.sideB) + " against " + L(historySeat(H, e.year)) + " (" + nm + ")"; break;
                case HW_SUCCESSION: s = "two claimants fought for the rule: " + N(w.leaderA) + " and " + N(w.leaderB) + " (" + nm + ")"; break;
                case HW_REVOLUTION: s = "the revolution began" + at + (e.b >= 0 ? ", led by " + N(e.b) : ""); break;
                case HW_OTHER: s = "war with " + other(H) + (e.out == 0 ? ", begun by " + people(H) : ", begun by " + other(H)) + at; break;
                case HW_CONQUEST: s = other(H) + " came to conquer" + at; break;
                case HW_BANDS: s = "a feud between the bands of " + L(w.sideA) + " and " + L(w.sideB); break;
                default: s = nm + " began between " + L(w.sideA) + " and " + L(w.sideB); break;
            }
            break;
        }
        case HE_BATTLE: {
            const std::string nm = historyWarName(H, e.thing);
            const HistoryWar& w = H.wars[clampi(e.thing, 0, (int)H.wars.size() - 1)];
            std::string won;
            if (w.sideB == -2) won = e.out == 0 ? people(H) + " won" : other(H) + " won";
            else {
                const std::string sideA = w.sideA >= 0 ? L(w.sideA) : (w.kind == HW_REVOLUTION ? "the old rule" : "the rule of " + L(historySeat(H, e.year)));
                const std::string sideB = w.kind == HW_REVOLUTION ? "the revolution" : (w.kind == HW_SUCCESSION ? N(w.leaderB) : L(w.sideB));
                won = (e.out == 0 ? sideA : sideB) + " won";
            }
            const std::string under = e.a >= 0 ? N(e.a) : "";
            s = "the battle of " + L(e.place) + " in " + nm + ": " + won + (!under.empty() && won.compare(0, under.size(), under) != 0 ? " under " + under : "") + dead(e.value);
            break;
        }
        case HE_SIEGE: s = L(e.place) + " was besieged in " + historyWarName(H, e.thing) + (e.out ? " and fell" : " and held"); break;
        case HE_SACK: s = L(e.place) + " was sacked in " + historyWarName(H, e.thing) + dead(e.value); break;
        case HE_PEACE: {
            const HistoryWar& w = H.wars[clampi(e.thing, 0, (int)H.wars.size() - 1)];
            const std::string nm = historyWarName(H, e.thing);
            std::string res;
            if (e.out == 2) res = "neither side won";
            else switch (w.kind) {
                case HW_REBELLION: res = e.out == 0 ? L(w.sideB) + " submitted" : L(w.sideB) + " kept its own law"; break;
                case HW_REVOLUTION: res = e.out == 0 ? "the revolution was put down" : "the revolution won"; break;
                case HW_SUCCESSION: res = N(e.out == 0 ? w.leaderA : w.leaderB) + " won"; break;
                case HW_OTHER: case HW_CONQUEST: res = e.out == 0 ? people(H) + " won" : other(H) + " won"; break;
                default: res = (e.out == 0 ? (w.sideA >= 0 ? L(w.sideA) : L(historySeat(H, e.year))) : L(w.sideB)) + " won"; break;
            }
            s = nm + " ended: " + res + (w.dead > 0 ? " (" + withCommas(w.dead) + " dead in it)" : "");
            break;
        }
        case HE_SICKNESS: s = (e.out <= 1 ? historySicknessName(H, e.thing) + " came to " : historySicknessName(H, e.thing) + " came back to ") + L(e.place) + (e.out > 1 ? " (its " + ordinal(e.out) + " wave)" : "") + dead(e.value); break;
        case HE_HARVEST: {
            const int eco = H.eras[H.eraAt(e.year)].economy;
            const char* what = eco == ECO_HERDING ? "the herds" : (eco == ECO_HUNTING ? "the hunt" : "the harvest");
            if (e.out == 0) s = std::string(what) + (eco == ECO_HERDING ? " failed" : " failed");
            else if (e.out == 1) s = std::string("a poor year for ") + what;
            else s = std::string("a rich year for ") + what;
            break;
        }
        case HE_FAMINE: {
            const int eco = H.eras[H.eraAt(e.year)].economy;
            const char* after = eco == ECO_HERDING ? " after the herds failed" : (eco == ECO_HUNTING ? " after the hunt failed" : " after the harvest failed");
            s = "famine" + (e.place >= 0 ? " at " + L(e.place) + " and round it" : std::string(" in the land")) + (e.cause >= 0 && H.events[e.cause].kind == HE_HARVEST ? after : "") + dead(e.value);
            break;
        }
        case HE_FLOOD: s = e.year < 1 ? "the great flood, remembered ever after" : ((e.thing == 1 ? "the sea came over " : (e.place == 0 ? H.lore.river + " flooded " : "the river flooded ")) + L(e.place) + dead(e.value)); break;
        case HE_DROUGHT: s = e.year < 1 ? "the long drought, remembered ever after" : "drought for " + withCommas(e.value) + (e.value == 1 ? " year" : " years"); break;
        case HE_QUAKE: s = "an earthquake at " + L(e.place) + dead(e.value); break;
        case HE_ERUPTION: s = H.lore.mountain + " broke open near " + L(e.place) + dead(e.value); break;
        case HE_IMPACT: s = e.out == 2 ? "a great stone fell from the sky and darkened the world" + dead(e.value) : (e.out == 0 ? "a stone fell from the sky and ruined " + L(e.place) + dead(e.value) : "a stone fell from the sky near " + L(e.place) + dead(e.value)); break;
        case HE_FLARE: s = "the sun flared and burned the day side" + dead(e.value); break;
        case HE_WINTER:
            if (e.year < 1) s = "the long winter, remembered ever after";
            else if (e.value >= 25) s = "the long cold began, and lasted to the end";
            else s = "a winter of " + withCommas(e.value) + (e.value == 1 ? " year" : " years") + (e.cause >= 0 ? (H.events[e.cause].kind == HE_ERUPTION ? ", after the eruption" : ", after the stone from the sky") : "");
            break;
        case HE_BUILT:
            if (e.thing == WK_GREAT) { s = "the work at " + L(e.place) + " was finished after " + withCommas(e.value) + " years" + (e.a >= 0 ? ", in the keeping of " + P(e.a) : ""); break; }
            s = cap1(workName(H, e.thing)) + (e.thing == WK_WALLS || (e.thing == WK_HOLY && H.profile.holy == HOLY_TOMBS) ? " were finished at " : " was finished at ") + L(e.place) + " after " + withCommas(e.value) + (e.value == 1 ? " year" : " years") + (e.a >= 0 ? ", the work of " + P(e.a) : ""); break;
        case HE_WAY: s = std::string("the ") + ofKind(e.out) + " between " + L(e.place) + " and " + L(e.place2) + " was opened"; break;
        case HE_FESTIVAL: s = L(e.place) + " kept the " + ordinal((int)e.value) + " year of its founding"; break;
        case HE_OMEN: {
            static const char* const OM[10] = {"the comet returned", "the sun went dark at noon", "lights filled the night sky", "stars fell all night", "a new star shone for a season", "the moon turned red",
                                               "the wanderer grew brighter in the sky, year after year", "a new star grew brighter each year", "the stars bent round a place where nothing shone",
                                               "the second sun drew further off each year"};   // D-05: the lost sun's four
            s = OM[clampi(e.thing, 0, 9)] + (e.thing != 0 && e.thing != 4 && e.thing < 6 ? std::string(" over ") + L(e.place) : std::string());
            break;
        }
        case HE_INVENTION: {
            std::string nm = historyInventionName(e.thing);
            if (e.thing == INV_MOON) nm = H.lore.moons > 0 ? "a landing on " + H.lore.moon : "the first craft to another world";
            s = P(e.a) + " of " + L(e.place) + " made " + nm; break;
        }
        case HE_DISCOVERY: {
            static const char* const DI[9] = {"the far shore", "the source of {river}", "the pass over {mountain}", "that the world is round", "the far side of the world", "the poles", "the way to orbit", "the moon", "the other worlds of the sun"};
            s = P(e.a) + " found " + fillText(H, DI[clampi(e.thing, 0, 8)], e.place, e.year) + (e.thing == 0 && e.value >= 0 ? ", where " + L((int)e.value) + " would be founded" : ""); break;
        }
        case HE_FAITH: {
            switch (e.thing) {
                case 0: s = P(e.a) + " preached " + (e.value == FAITH_ONE_GOD ? "the faith of " + H.lore.god + ", the one god," : std::string("the faith of ") + FAITH_NAMES[clampi((int)e.value, 0, FAITH_COUNT - 1)]) + at; break;
                case 1: s = "a schism split the faith" + at; break;
                case 2: s = P(e.a) + " reformed the rites" + at; break;
                case 3: s = "the temples were closed"; break;
                default: s = "a persecution" + at; break;
            }
            break;
        }
        case HE_INSTITUTION: {
            const HistoryInstitution& I = H.institutions[clampi(e.thing, 0, (int)H.institutions.size() - 1)];
            static const char* const NM[HI_COUNT] = {"the Council of {place}", "the priesthood of {god}", "the guilds of {place}", "the Academy at {place}", "the Company of {place}", "the {place} Gazette",
                                                     "the University of {place}", "the Assembly", "the Broadcast of {place}", "the Network", "the Mind", "the Army of {place}"};
            s = cap1(fillText(H, NM[clampi(I.kind, 0, HI_COUNT - 1)], e.place, e.year)) + (I.kind == HI_GUILDS ? " were founded" : " was founded") + (e.a >= 0 ? " by " + P(e.a) : "");
            break;
        }
        case HE_DEED: {
            static const char* const DE[13] = {"made a song that outlived its maker", "wrote the chronicle of {capital}", "gave the laws", "drew the map of the land", "led the great hunt",
                                               "sailed beyond the known coasts", "healed the sick of {place}", "carved the great frieze at {place}", "taught at {place}", "wrote a book that everyone read",
                                               "made a theory of the world", "made the pictures that move", "made the gardens of {place}"};
            s = P(e.a) + " " + fillText(H, DE[clampi(e.thing, 0, 12)], e.place, e.year); break;
        }
        case HE_UNREST: {
            static const char* const UN[4] = {"a riot over bread", "a strike", "a rising", "a riot"};
            const bool early = H.eras[H.eraAt(e.year)].age == AGE_PREHISTORIC;
            s = (early ? (e.thing == 0 ? std::string("a fight over the food") : std::string("a quarrel between the bands")) : std::string(UN[clampi(e.thing, 0, 3)])) + at + dead(e.value); break;
        }
        case HE_MIGRATION: s = withCommas(e.value) + " moved from " + (e.place >= 0 ? L(e.place) : std::string("the countryside")) + " to " + (e.place2 >= 0 ? L(e.place2) : std::string("the cities")); break;
        case HE_ABANDONED: {
            static const char* const WHY[10] = {"", ": the river changed its course", ": the wells failed", ": the mine was worked out", ": the soil was spent", ": its people went to the cities",
                                                ": the band moved on", ": it was cleared and given back to the land", "", ": it was taken"};
            s = L(e.place) + " was left";
            if (e.thing == 8 || (e.thing == 0 && e.cause >= 0)) {
                // the harm that struck it, where it struck there; else the collapse's own reason
                std::string after;
                if (e.cause >= 0) {
                    const HistoryEvent& c = H.events[e.cause];
                    const bool here = c.place == e.place || c.place < 0;
                    switch (c.kind) {
                        case HE_SACK: if (c.place == e.place) after = " after the sack"; break;
                        case HE_SICKNESS: if (here) after = " after " + historySicknessName(H, c.thing); break;
                        case HE_FLOOD: if (c.place == e.place) after = " after the flood"; break;
                        case HE_QUAKE: if (c.place == e.place) after = " after the earthquake"; break;
                        case HE_ERUPTION: if (c.place == e.place) after = " after the eruption"; break;
                        case HE_IMPACT: if (here) after = " after the stone from the sky"; break;
                        case HE_FAMINE: if (c.place == e.place) after = " in the famine"; break;
                        default: break;
                    }
                }
                if (!after.empty()) s += after;
                else if (e.thing == 8) { const std::string why = LEFT_BY_CAUSE[clampi(H.cause, 0, CAUSE_COUNT - 1)]; s += why.empty() ? std::string(" in the end") : ": " + why; }
            } else s += WHY[clampi(e.thing, 0, 9)];
            break;
        }
        case HE_CONTACT: s = people(H) + " met " + other(H) + at + (e.a >= 0 ? "; " + P(e.a) + " went to them" : ""); break;
        case HE_TREATY: { static const char* const TR[3] = {"a pact of friendship", "a treaty of trade", "a border agreed"}; s = std::string(TR[clampi(e.thing, 0, 2)]) + " with " + other(H) + at; break; }
        case HE_OTHER_END: s = "word came that " + other(H) + " were gone"; break;
        case HE_SIGN: {
            const CauseScript& CS = SCRIPTS[clampi(H.cause, 0, CAUSE_COUNT - 1)];
            const int st = clampi(e.thing / 2, 0, STAGE_COUNT - 1), alt = e.thing & 1;
            s = fillText(H, CS.signs[st][alt], e.place, e.year) + (e.value > 0 ? dead(e.value) : "");
            break;
        }
        case HE_STAGE: s = std::string("the collapse: ") + STAGE_TITLES[clampi(e.out, 0, STAGE_COUNT - 1)] + " (" + CAUSE_NAMES[clampi(H.cause, 0, CAUSE_COUNT - 1)] + ")"; break;
        case HE_EXODUS: {
            static const char* const WHI[9] = {"for ", "for the mountains", "over the sea", "for the south", "for the north", "into the sky", "elsewhere", "into the machines", "for the halls below "};
            const int w = clampi(e.thing, 0, 8);
            s = withCommas(e.value) + " left " + L(e.place) + " " + (w == 0 ? std::string("for ") + (e.place2 >= 0 ? L(e.place2) : std::string("the last places"))
                                                                          : (w == 8 ? std::string(WHI[w]) + (e.place2 >= 0 ? L(e.place2) : std::string("the warm ground")) : std::string(WHI[w])));
            break;
        }
        case HE_SUN: {   // D-05: the sun going, as it was seen
            static const char* const SU[6] = {"the sun had grown smaller", "the year did not end: the summer did not come back", "the sea froze from the shore at {place}",
                                              "the air itself fell as snow", "the sun rose for the last time", "the stars stood in the sky at noon"};
            s = fillText(H, SU[clampi(e.thing, 0, 5)], e.place, e.year) + (e.thing == 4 && e.place >= 0 ? std::string(" over ") + L(e.place) : std::string());
            break;
        }
        case HE_END: s = "the end of " + people(H) + ": " + N(e.a) + " was the last of them, at " + L(e.place); break;
        // H-04's second pass: the everyday (the legends' plain line; the records tell them in their registers)
        case HE_FAIR: s = std::string(std::strcmp(historyEverydayKey(e.kind, e.thing), "gathering") == 0 ? "the bands gathered" : "a fair") + at + " (" + historyEverydayKey(e.kind, e.thing) + "; " + withCommas(e.value) + " came)"; break;
        case HE_FIRE: s = std::string("a fire") + at + " (" + historyEverydayKey(e.kind, e.thing) + "; " + withCommas(e.value) + " houses)"; break;
        case HE_STORM: s = std::string("a storm of ") + historyEverydayKey(e.kind, e.thing) + at + dead(e.value); break;
        case HE_PEST: s = std::string(historyEverydayKey(e.kind, e.thing)) + at; break;
        case HE_WONDER: s = std::string("a wonder: ") + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_GAMES: s = std::string("games of ") + historyEverydayKey(e.kind, e.thing) + at + (e.a >= 0 ? ", won by " + P(e.a) : ""); break;
        case HE_LAW: s = std::string("a law on ") + historyEverydayKey(e.kind, e.thing) + (e.a >= 0 ? " by " + P(e.a) : ""); break;
        case HE_TRIAL: { static const char* const V[4] = {"acquitted", "condemned", "exiled", "fined"}; s = (e.a >= 0 ? P(e.a) : std::string("someone")) + " tried for " + historyEverydayKey(e.kind, e.thing) + at + ", " + V[clampi(e.out, 0, 3)]; break; }
        case HE_EMBASSY: s = e.thing == 0 ? "envoys from " + L(e.place2) + " came to " + L(e.place) : (e.thing == 1 ? "envoys of " + other(H) + " came to " + L(e.place) : "envoys went to " + other(H)) + (e.a >= 0 ? " (" + P(e.a) + ")" : ""); break;
        case HE_ACCIDENT: s = std::string("a disaster: ") + historyEverydayKey(e.kind, e.thing) + at + dead(e.value); break;
        case HE_PRICE: s = std::string("the price of ") + historyEverydayKey(e.kind, e.thing) + (e.out ? " rose " : " fell ") + std::to_string(e.value) + "%"; break;
        case HE_BEASTS: s = std::string("the herds: ") + historyEverydayKey(e.kind, e.thing) + at + (e.value > 0 ? dead(e.value) : ""); break;
        case HE_SHOW: s = std::string("a new ") + historyEverydayKey(e.kind, e.thing) + at + (e.a >= 0 ? " by " + P(e.a) : ""); break;
        case HE_VISIT: s = P(e.a) + " came to " + L(e.place); break;
        case HE_BIRTH: s = N(e.a) + " was born to " + N(e.b) + " and " + N(e.c) + at; break;
        case HE_MARRIAGE: s = N(e.a) + " married " + N(e.b) + at; break;
        case HE_MOVE: s = N(e.a) + " moved from " + L(e.place) + " to " + L(e.place2); break;
        case HE_ILLNESS: s = N(e.a) + " was ill (" + historyEverydayKey(e.kind, e.thing) + ") and got well" + at; break;
        case HE_INJURY: s = N(e.a) + " hurt " + std::string(H.persons[e.a].female ? "her " : "his ") + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_APPRENTICE: s = N(e.a) + " was apprenticed to a " + historyTradeName(e.thing) + at; break;
        case HE_HOUSE: s = N(e.a) + "'s house: " + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_QUARREL: s = N(e.a) + " quarrelled" + (e.b >= 0 ? " with " + N(e.b) : std::string()) + " over the " + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_JOURNEY: s = N(e.a) + " went to " + L(e.place2) + " (" + historyEverydayKey(e.kind, e.thing) + ") and came back"; break;
        case HE_BETROTHAL: s = N(e.a) + " and " + N(e.b) + " were promised" + at; break;
        case HE_DEATH: {
            static const char* const HOW[16] = {"", " of a sickness", " in childbirth", " in battle", " in the famine", " in the flood", " in the earthquake", " in an accident", "", "",
                                               " in the end", " young", " in the fire from the sky", " of the cold", " at sea", " in the hard years"};
            const HistoryPerson& Q = H.persons[clampi(e.a, 0, (int)H.persons.size() - 1)];
            const int years = e.year - Q.born;
            std::string how = HOW[clampi(e.thing, 0, 15)];
            if (e.thing == 1 && e.cause >= 0 && H.events[e.cause].kind == HE_SICKNESS) how = " of " + historySicknessName(H, H.events[e.cause].thing);
            if (e.thing == 3 && e.cause >= 0 && H.events[e.cause].kind == HE_BATTLE) how = " at the battle of " + L(H.events[e.cause].place);
            std::string who = P(e.a);
            if (e.pub) {   // who they were
                switch (Q.role) {
                    case HR_FOUNDER: for (int q = 0; q < (int)H.places.size(); q++) if (H.places[q].founder == e.a) { who += ", who founded " + L(q) + ","; break; } break;
                    case HR_GENERAL: who = "the general " + who; break;
                    case HR_PROPHET: who = "the prophet " + who; break;
                    case HR_EXPLORER: who = "the explorer " + who; break;
                    case HR_REBEL: who = "the rebel " + who; break;
                    case HR_REGENT: who = "the regent " + who; break;
                    case HR_INVENTOR: for (const HistoryInvention& I : H.inventions) if (I.person == e.a) { who += ", who made " + historyInventionName(I.id) + ","; break; } break;
                    default: break;
                }
            }
            const char* verb = e.thing == 8 ? " was murdered" : (e.thing == 9 ? " was put to death" : " died");
            s = who + verb + how + (e.place >= 0 && e.thing != 3 ? " at " + L(e.place) : "") + (e.thing == 11 || years < 1 ? "" : ", aged " + std::to_string(years));
            break;
        }
        default: s = HISTORY_EVENT_NAMES[clampi(e.kind, 0, HE_COUNT - 1)]; break;
    }
    return cap1(s);
}
