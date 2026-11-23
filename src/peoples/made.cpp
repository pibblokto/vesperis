// Z-06: the made organisms (peoples/made.h): the peoples' one file that reads src/life
#include "made.h"
#include "genres.h"
#include "peoples.h"
#include "records.h"
#include "ruins.h"
#include "life/biota.h"
#include "../core/rng.h"
#include <algorithm>

namespace {
constexpr uint64_t SALT_MADE = 0x3ADE0C0000000001ULL;   // which species, which people, the year
constexpr uint64_t SALT_MARK = 0x3ADE0C0000000002ULL;   // the watermark's text
}   // namespace

MadeSpecies madeSpeciesOf(const StarSystem& sys, int body) {
    MadeSpecies m;
    if (body < 0 || body >= (int)sys.bodies.size()) return m;
    const Body& b = sys.bodies[body];
    if (!KINDS[b.type].landable) return m;
    const BodyGen g = BodyGen::make(b);
    if (!worldHadCivilisation(g)) return m;
    Rng r(mix64(b.seed ^ SALT_MADE));
    const double pick = r.uni(), who = r.uni(), when = r.uni();
    const int peoples = std::max(1, peoplesOf(g));
    const int people = peoples > 1 && who < 0.5 ? 1 : 0;
    const PeopleAge pa = peopleAgeOf(g, people);
    if (pa.age < AGE_FUTURISTIC) return m;
    // one of the world's own, alive or asleep (a lost sun's world keeps its spores), never a protocell's chemistry, a fossil or a spore carried there
    auto L = lifeOf(sys);
    const Biota& B = L->bodies[body];
    std::vector<int> own;
    for (const Microbe& s : B.species) if ((s.state == SS_LIVING || s.state == SS_DORMANT) && s.fromBody < 0 && !protocellPlan(s.plan)) own.push_back(s.index);
    if (own.empty()) return m;
    m.species = own[std::min((int)own.size() - 1, (int)(pick * own.size()))];
    m.people = people;
    // made in their last age, in the first five sixths of its years (the made genes a modern people's, so before it)
    const int from = ageFromOf(pa), to = std::max(from, pa.lastYear - 1);
    m.year = from + (int)(when * (to - from) * 5 / 6);
    return m;
}

bool madeWatermarkOf(const StarSystem& sys, int body, const Lore& L, const MadeSpecies& m, Shard& out, VgRecord* rec) {
    if (m.species < 0 || body < 0 || body >= (int)sys.bodies.size()) return false;
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    auto life = lifeOf(sys);
    const Biota& B = life->bodies[body];
    if (m.species >= (int)B.species.size()) return false;
    const Microbe& s = B.species[m.species];
    const int way = s.way == LW_LIGHT ? 0 : (s.way == LW_CHEMICAL ? 1 : (s.way == LW_DEAD ? 2 : (s.way == LW_GRAZER ? 3 : 4)));
    int where = 0;   // water, soil, rock, ice, salt, a spring
    switch (s.niche) {
        case NI_SOIL: case NI_MAT: case NI_CRUST: case NI_RUINS: where = 1; break;
        case NI_ROCK: case NI_DUST: where = 2; break;
        case NI_ICE: case NI_SNOW: where = 3; break;
        case NI_SALT: where = 4; break;
        case NI_SPRING: case NI_VENT: where = 5; break;
        default: where = 0; break;
    }
    const uint64_t seed = mix64(b.seed ^ SALT_MARK ^ ((uint64_t)m.species << 8));
    VgRecord R;
    if (!writeWatermark(sys, b, g, L, *shardGrammar(), m.year, way, where, s.glows, seed, R)) return false;
    out = Shard();
    out.watermark = true; out.kind = -1; out.year = m.year; out.seed = seed;
    out.text = R.text;
    out.names = R.names; out.names.insert(out.names.end(), R.theirs.begin(), R.theirs.end());   // the names and their own words, kept as they are
    out.recordAge = recordAgeAt(peopleAgeOf(g, m.people), m.year);
    if (rec) *rec = R;
    return true;
}
