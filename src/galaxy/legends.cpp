// H-06: what a people's read records told of its history (see legends.h)
#include "legends.h"
#include <algorithm>
#include <map>

const char* const AGE_TOLD_NAMES[AGE_COUNT] = {"PREHISTORIC", "MEDIEVAL", "INDUSTRIAL", "MODERN", "FUTURISTIC", "FAR FUTURE"};

namespace {
std::string lowerOf(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }
void addWords(const std::string& name, std::set<std::string>& out) {
    size_t p = 0;
    while (p <= name.size()) {
        size_t q = name.find(' ', p); if (q == std::string::npos) q = name.size();
        std::string w = name.substr(p, q - p);
        while (!w.empty() && !std::isalnum((unsigned char)w.back()) && w.back() != '-') w.pop_back();
        if (!w.empty()) out.insert(lowerOf(w));
        p = q + 1;
    }
}
// the given name a record calls a person by: the first word, unless the second is a ruler's numeral ("Baliothoo II"), as the records' model has it
std::string givenName(const std::string& full) {
    const size_t sp = full.find(' ');
    if (sp == std::string::npos) return full;
    const size_t sp2 = full.find(' ', sp + 1);
    const std::string w = full.substr(sp + 1, sp2 == std::string::npos ? std::string::npos : sp2 - sp - 1);
    if (!w.empty() && w.find_first_not_of("IVXLC") == std::string::npos) return sp2 == std::string::npos ? full : full.substr(0, sp2);
    return full.substr(0, sp);
}
}   // namespace

void toldHistoryOf(const History& H, const ShardTexts& texts, const std::vector<int>& read, int lastIndex, ToldHistory& out) {
    out = ToldHistory();
    out.age = H.eras.empty() ? H.age : H.eras.back().age;
    out.eraRead.assign(H.eras.size(), 0);
    std::map<int, std::vector<int>> by;   // an event and the records that told it
    for (int i : read) {
        if (i < 0 || i >= (int)texts.items.size()) continue;
        const ShardText& t = texts.items[i];
        out.read++;
        if (t.age >= 0 && t.age < AGE_COUNT) out.ageRead[t.age]++;
        const int era = H.eraAt(t.year);
        if (era >= 0 && era < (int)out.eraRead.size()) out.eraRead[era]++;
        if (out.first == 0 || t.year < out.first) out.first = t.year;
        if (out.last == 0 || t.year > out.last) out.last = t.year;
        if (i == lastIndex) out.ended = true;
        if (t.text.empty()) continue;   // a piece, a chart: it dates its era and its age, it tells no event
        out.texts++;
        for (int ev : t.told) if (ev >= 0 && ev < (int)H.events.size()) { std::vector<int>& v = by[ev]; if (std::find(v.begin(), v.end(), i) == v.end()) v.push_back(i); }
        for (const std::string& n : t.rec.names) out.names.insert(n);
        for (const auto& w : t.ways) if (!out.ways.count(w.first)) out.ways[w.first] = {w.second, i};
    }
    for (auto& kv : by) {
        std::vector<int> v = kv.second;
        std::sort(v.begin(), v.end(), [&](int a, int b) { const int ya = texts.items[a].year, yb = texts.items[b].year; return ya != yb ? ya < yb : a < b; });
        out.events.push_back({kv.first, v});
    }
    out.ageTold = out.ended || out.ageRead[std::max(0, std::min(AGE_COUNT - 1, out.age))] >= AGE_TOLD_RECORDS;
}

bool toldPersonNamed(const History& H, const ToldHistory& T, int person) {
    if (person < 0 || person >= (int)H.persons.size()) return false;
    const std::string full = historyPersonName(H, person, false);
    return T.names.count(full) > 0 || T.names.count(givenName(full)) > 0;
}

std::string legendLine(const History& H, const ToldHistory& T, int event) {
    if (event < 0 || event >= (int)H.events.size()) return "";
    const int year = H.events[event].year;
    const HistoryPersonNamer namer = [&](int p, bool withTitle) -> std::string {
        if (p < 0 || p >= (int)H.persons.size()) return "someone";
        if (toldPersonNamed(H, T, p)) return historyPersonName(H, p, withTitle);
        const HistoryPerson& P = H.persons[p];
        if (withTitle && P.role == HR_RULER) return "a ruler";
        if (year - P.born < 14) return "a child";
        return P.female ? "a woman" : "a man";
    };
    return historyEventLine(H, event, &namer);
}

void legendWays(const History& H, const ToldHistory& T, std::vector<std::pair<std::string, std::string>>& out) {
    out.clear();
    const PeopleProfile& P = H.profile;
    auto told = [&](const char* k) { return T.ways.count(k) > 0; };
    auto value = [&](const char* k) { auto it = T.ways.find(k); return it == T.ways.end() ? std::string() : it->second.first; };
    const bool godNamed = !H.lore.god.empty() && T.names.count(H.lore.god) > 0;
    if (told("faith")) {
        std::string f = FAITH_NAMES[std::max(0, std::min(FAITH_COUNT - 1, P.faith))];
        if (godNamed && P.faith == FAITH_ONE_GOD) f += ", " + H.lore.god;
        else if (godNamed && (P.faith == FAITH_MANY_GODS || P.faith == FAITH_SKY)) f += ", " + H.lore.god + " among them";
        out.push_back({"FAITH", f});
    } else if (godNamed) out.push_back({"GOD", H.lore.god});
    if (told("rite")) out.push_back({"RITE", RITE_NAMES[std::max(0, std::min(RITE_COUNT - 1, P.rite))]});
    if (told("holy")) out.push_back({"HOLY", HOLY_NAMES[std::max(0, std::min(HOLY_COUNT - 1, P.holy))]});
    if (told("economy") || told("economy2")) {
        std::string l = "by ";
        if (told("economy")) l += ECONOMY_NAMES[std::max(0, std::min(ECO_COUNT - 1, P.economy))];
        if (told("economy2") && P.economy2 != P.economy) l += std::string(told("economy") ? " and " : "") + ECONOMY_NAMES[std::max(0, std::min(ECO_COUNT - 1, P.economy2))];
        if (l != "by ") out.push_back({"LIVED", l});
    }
    if (told("value") || told("value2")) {   // the records' value words are A-01's ("war", "piety", "trade", "learning", "art")
        std::string l = told("value") ? value("value") : value("value2");
        if (told("value") && told("value2") && value("value2") != value("value")) l += " and " + value("value2");
        out.push_back({"VALUED", l});
    }
    if (told("deadway")) {
        static const char* const KEYS[5] = {"barrows", "tombs", "cemeteries", "towers", "none"};
        static const char* const DEAD[5] = {"buried in barrows", "laid in tombs", "buried in cemeteries", "given to towers", "kept no graves"};
        int k = 0; for (int i = 0; i < 5; i++) if (value("deadway") == KEYS[i]) k = i;
        out.push_back({"DEAD", DEAD[k]});
    }
    if (told("cause")) out.push_back({"ENDED", std::string("by ") + CAUSE_NAMES[std::max(0, std::min(CAUSE_COUNT - 1, H.cause))]});
}

void legendNameWords(const History& H, const ToldHistory& T, std::set<std::string>& out) {
    for (int p = 0; p < (int)H.places.size(); p++) addWords(historyPlaceName(H, p), out);
    for (const std::string& n : T.names) addWords(n, out);
    for (int g = 0; g < GOV_COUNT; g++) if (historyTitleNative(H, g)) { addWords(historyRulerTitle(H, g, false), out); addWords(historyRulerTitle(H, g, true), out); }
    for (const std::string* n : {&H.lore.people, &H.lore.god, &H.lore.river, &H.lore.mountain, &H.lore.sea, &H.lore.city, &H.lore.city2, &H.lore.founder, &H.lore.moon, &H.lore.star, &H.lore.festival, &H.lore.other})
        if (!n->empty()) addWords(*n, out);
}
