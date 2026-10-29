// E-10: the records' schema (`schemaOf`; its tables in genres_schema.h) and the facts every record of a world shares
// (`recordWorldFacts`: the world's, the people's, the lore's)
#include "genres_schema.h"

namespace {
constexpr double DEG_ = 3.14159265358979323846 / 180.0;
}   // namespace

const VgSchema& schemaOf() {
    static const VgSchema S = [] {
        VgSchema s;
        for (int i = 0; i < F_FIXED_COUNT; i++) s.facts.push_back(FACT_NAMES[i]);
        for (int i = 0; i < INV_COUNT; i++) s.facts.push_back(std::string("inv.") + INV_KEYS[i].key);
        for (int i = 0; i < HI_COUNT; i++) s.facts.push_back(INST_KEYS[i]);
        for (int i = 0; i < TRAIT_COUNT; i++) s.facts.push_back(TRAIT_FACTS[i].fact);
        for (int i = 0; i < NAME_FACTS; i++) s.names.push_back(FACT_NAMES[i]);
        for (const char* e : ENTITY_NAMES) s.entities.push_back(e);
        for (const char* p : PATH_NAMES) s.paths.push_back(p);
        for (const char* f : FIELD_NAMES) s.fields.push_back(f);
        for (const char* d : DIRECTIVE_NAMES) s.directives.push_back(d);
        return s;
    }();
    return S;
}

// ---- the public functions ------------------------------------------------------------------------------------------------------------

const VgSchema& recordSchema() { return schemaOf(); }

namespace {
template <size_t N> void addKeys(std::vector<RecordKeyTable>& k, const char* name, const char* const (&t)[N]) {
    k.push_back({name, std::vector<std::string>(t, t + N)});
}
}   // namespace
const std::vector<RecordKeyTable>& recordSchemaKeys() {
    static const std::vector<RecordKeyTable> K = [] {
        std::vector<RecordKeyTable> k;
        // the facts (`age=medieval`): the people's and the world's (`recordWorldFacts`), the record's and its year's (genres.cpp)
        addKeys(k, "lastage", AGE_KEYS); addKeys(k, "faith", FAITH_KEYS); addKeys(k, "rite", RITE_KEYS); addKeys(k, "holy", HOLY_KEYS);
        addKeys(k, "economy", ECONOMY_KEYS); addKeys(k, "economy2", ECONOMY_KEYS); addKeys(k, "value", VALUE_KEYS); addKeys(k, "value2", VALUE_KEYS);
        addKeys(k, "focus", FOCUS_KEYS); addKeys(k, "centre", CENTRE_KEYS); addKeys(k, "deadway", DEAD_KEYS); addKeys(k, "monuments", MONUMENT_KEYS);
        addKeys(k, "orient", ORIENT_KEYS); addKeys(k, "cause", CAUSE_KEYS); addKeys(k, "age", AGE_KEYS); addKeys(k, "medium", MEDIUM_KEYS);
        addKeys(k, "gov", GOV_KEYS); addKeys(k, "temper", TEMPER_KEYS); addKeys(k, "tone", TONE_KEYS); addKeys(k, "homerank", RANK_KEYS);
        addKeys(k, "plan", PLAN_KEYS); addKeys(k, "roof", ROOF_KEYS); addKeys(k, "ornament", ORNAMENT_KEYS); addKeys(k, "motif", MOTIF_KEYS);
        addKeys(k, "tops", TOPS_KEYS); addKeys(k, "accent", ACCENT_KEYS); addKeys(k, "land", LAND_KEYS); addKeys(k, "streets", STREETS_KEYS);
        addKeys(k, "public", PUBLIC_KEYS); addKeys(k, "edge", EDGE_KEYS); addKeys(k, "links", LINKS_KEYS); addKeys(k, "light", LIGHT_KEYS);
        addKeys(k, "sunway", SUN_WAY_KEYS);
        // the fields (`$e.kind=war`, `$x.role=ruler`): an event's, a person's, a place's (genres.cpp's `field`)
        addKeys(k, ".kind", KIND_KEYS); addKeys(k, ".warkind", WAR_KEYS); addKeys(k, ".workkey", WORK_KEYS);
        std::vector<std::string> inv; for (const InvKey& x : INV_KEYS) inv.push_back(x.key);
        k.push_back({".inv", inv});
        addKeys(k, ".omen", OMEN_KEYS); addKeys(k, ".how", DEATH_KEYS); addKeys(k, ".gov", GOV_KEYS); addKeys(k, ".govold", GOV_KEYS);
        addKeys(k, ".govhow", GOVHOW_KEYS); addKeys(k, ".accession", ACCESSION_KEYS); addKeys(k, ".ruleend", RULEEND_KEYS); addKeys(k, ".institution", INST_KEYS);
        addKeys(k, ".discovery", DISCOVERY_KEYS); addKeys(k, ".deed", DEED_KEYS); addKeys(k, ".unrest", UNREST_KEYS); addKeys(k, ".whither", WHITHER_KEYS);
        addKeys(k, ".left", LEFT_KEYS); addKeys(k, ".faith", FAITHEV_KEYS); addKeys(k, ".treaty", TREATY_KEYS); addKeys(k, ".founding", FOUNDING_KEYS);
        addKeys(k, ".whence", WHENCE_KEYS); addKeys(k, ".first", FIRST_KEYS); addKeys(k, ".role", ROLE_KEYS); addKeys(k, ".rank", RANK_KEYS);
        addKeys(k, ".verdict", VERDICT_KEYS); addKeys(k, ".sun", SUN_KEYS);
        return k;
    }();
    return K;
}

void recordWorldFacts(const StarSystem& sys0, const Body& b0, const BodyGen& g, const Lore& L, const History* H, VgModel& M) {
    const PeoplesWorld PW = peoplesWorldOf(sys0, b0);   // D-05: a drifter's people's world at its former orbit
    const StarSystem& sys = *PW.sys; const Body& b = *PW.b;
    const VgSchema& S = schemaOf();
    if (M.facts.size() < S.facts.size()) M.facts.resize(S.facts.size());
    auto set = [&](int f, const std::string& v) { M.facts[f] = v; };
    auto flag = [&](int f, bool v) { M.facts[f] = v ? "1" : "0"; };
    set(F_PEOPLE, L.people); set(F_GOD, L.god); set(F_RIVER, L.river); set(F_MOUNTAIN, L.mountain); set(F_SEA, L.sea); set(F_CITY, L.city); set(F_CITY2, L.city2);
    set(F_MOON, L.moon); set(F_STAR, L.star); set(F_FESTIVAL, L.festival); set(F_OTHER, L.other); set(F_FOUNDERNAME, L.founder);
    set(F_OTHERFATE, L.other.empty() ? std::string() : std::to_string(L.otherFate));
    flag(F_DESERT, L.desert);
    flag(F_TILTED, std::fabs(b.axialTilt) / DEG_ > 8);
    const double tempC = (b.thenTempK > 0 ? b.thenTempK : b.tempK) + g.tempBias - 273.15;   // a dead world's in their time
    flag(F_COLD, tempC < 5);
    set(F_MOONS, std::to_string(L.moons));
    flag(F_RINGS, b.rings);
    flag(F_AURORA, auroraPotentialAt(sys, b, 0.3) > 0.25);
    flag(F_LOCKED, b.locked);
    bool companion = false, comet = false, giant = false;
    for (const Body& o : sys.bodies) { if (o.type == PT_COMPANION) companion = true; if (o.type == PT_COMET) comet = true; if (kindHas(o.type, KT_GAS_GIANT) && o.parent < 0) giant = true; }
    flag(F_COMPANION, companion); flag(F_COMET, comet); flag(F_GIANT, giant);
    flag(F_LONGNIGHT, L.dayHours > 2.5);
    flag(F_MOTHER, b.parent >= 0 && sys.bodies[b.parent].type != PT_COMPANION);
    flag(F_HIGHMOUNTAINS, g.mountainAmp > 2000);
    int cls = sys.star.cls;   // the sun: the class of the star the world circles (a companion's world has the companion's)
    if (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION) cls = sys.bodies[b.parent].starClass;
    if (cls < 0 || cls >= STAR_DRAWN_COUNT) cls = 0;   // a drifter's world has no class of sun (D-05 reads the star it left)
    set(F_SUNCLASS, std::to_string(cls));
    const int f1 = g.floraFamily >= 0 && g.floraFamily < 10 ? g.floraFamily : 0;
    const int f2 = g.floraFamily2 >= 0 && g.floraFamily2 < 10 && g.floraFamily2 != f1 ? g.floraFamily2 : -1;
    set(F_FLORA, std::to_string(f1)); set(F_FLORA2, std::to_string(f2));
    for (int i = 0; i < TRAIT_COUNT; i++) flag(F_TRAIT0 + i, g.hasTrait(TRAIT_FACTS[i].trait));
    flag(F_LAST, false);
    set(F_MARKHOW, "0"); set(F_NAMED, "0");
    set(F_TONE, "ordinary");
    // the people (the history's profile when there is one)
    const int seasons = H ? H->seasons : (std::fabs(b.axialTilt) / DEG_ > 8 ? 4 : 2);
    set(F_SEASONS, std::to_string(seasons));
    set(F_PEOPLES, std::to_string(L.peoples));
    if (H) {
        const PeopleProfile& P = H->profile;
        bool coast = false; for (const HistoryPlace& p : H->places) if (p.coast) { coast = true; break; }
        flag(F_COAST, coast);
        set(F_LASTAGE, AGE_KEYS[std::max(0, std::min(AGE_COUNT - 1, P.age))]);
        set(F_FAITH, keyOf(FAITH_KEYS, P.faith)); set(F_RITE, keyOf(RITE_KEYS, P.rite)); set(F_HOLY, keyOf(HOLY_KEYS, P.holy));
        set(F_ECONOMY, keyOf(ECONOMY_KEYS, P.economy)); set(F_ECONOMY2, keyOf(ECONOMY_KEYS, P.economy2));
        int v1 = 0, v2 = 1;
        for (int v = 0; v < VAL_COUNT; v++) if (P.values[v] > P.values[v1]) v1 = v;
        v2 = v1 == 0 ? 1 : 0; for (int v = 0; v < VAL_COUNT; v++) if (v != v1 && P.values[v] > P.values[v2]) v2 = v;
        set(F_VALUE, VALUE_KEYS[v1]); set(F_VALUE2, VALUE_KEYS[v2]);
        const Genome Gn = genomeOf(P.seed);
        set(F_NUMBER, std::to_string(Gn.number()));
        set(F_FOCUS, keyOf(FOCUS_KEYS, Gn.of(GV_FOCUS))); set(F_CENTRE, keyOf(CENTRE_KEYS, Gn.of(GV_CENTRE))); set(F_DEADWAY, keyOf(DEAD_KEYS, Gn.of(GV_DEAD)));
        set(F_MONUMENTS, keyOf(MONUMENT_KEYS, Gn.of(GV_MONUMENTS))); set(F_ORIENT, keyOf(ORIENT_KEYS, Gn.of(GV_ORIENT)));
        set(F_CAUSE, keyOf(CAUSE_KEYS, H->cause)); set(F_SPEED, std::to_string(causeSpeed(H->cause)));
        set(F_SUNWAY, H->profile.sunWay >= 0 ? SUN_WAY_KEYS[std::min(SW_COUNT - 1, H->profile.sunWay)] : "");
        if (!H->places.empty()) set(F_CITY, historyPlaceName(*H, 0));
    }
}
