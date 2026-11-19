#include "guide.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <cstdlib>

std::string Guide::starKey(int64_t sx, int64_t sy, int64_t sz) {
    return std::to_string(sx) + "," + std::to_string(sy) + "," + std::to_string(sz);
}
std::string Guide::bodyKey(int64_t sx, int64_t sy, int64_t sz, int body) { return starKey(sx, sy, sz) + "/" + std::to_string(body); }

bool Guide::parseStarKey(const std::string& key, int64_t& sx, int64_t& sy, int64_t& sz) {
    std::string k = key.substr(0, key.find('/'));
    for (char& c : k) if (c == ',') c = ' ';
    std::istringstream is(k);
    return (bool)(is >> sx >> sy >> sz);
}

namespace {
// T-04: a key of a shard of a world's second people as C-13 kept them ("<body>/S57") at the records' stride ("<body>/S1007"); any other unchanged
std::string recordsKey(const std::string& k) {
    const size_t p = k.rfind("/S");
    if (p == std::string::npos || p + 2 >= k.size()) return k;
    for (size_t i = p + 2; i < k.size(); i++) if (k[i] < '0' || k[i] > '9') return k;
    const int i = atoi(k.c_str() + p + 2);
    if (i < 50 || i >= 100) return k;
    return k.substr(0, p + 2) + std::to_string(1000 + (i - 50));
}
template <class M> void rekeyMap(M& m) { M n; for (auto& kv : m) n[recordsKey(kv.first)] = kv.second; m.swap(n); }
void rekeySet(std::set<std::string>& s) { std::set<std::string> n; for (const std::string& k : s) n.insert(recordsKey(k)); s.swap(n); }
}

bool Guide::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    std::getline(f, line);
    if (line.rfind("vesperis-guide", 0) != 0) return false;
    int recordsScheme = 1;   // T-04: the shards' keys of a guide without the line are C-13's
    while (std::getline(f, line)) {
        std::istringstream is(line);
        std::string key; is >> key;
        if (key == "name" || key == "inbox") {
            std::string k, rest; is >> k; std::getline(is, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            (key == "name" ? names : inbox)[k] = rest;
        } else if (key == "visited") { std::string k; is >> k; visited.insert(k); }
        else if (key == "landed") { std::string k; is >> k; landed.insert(k); }
        else if (key == "mark") {   // <body> <lat> <lon> <kind> <lent> <when> <name...>
            SurfaceMark m; int lentFlag = 0; std::string rest;
            is >> m.body >> m.lat >> m.lon >> m.kind >> lentFlag >> m.when;
            if (is.fail()) continue;
            std::getline(is, rest); if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            m.name = rest; m.lent = lentFlag != 0;
            m.kind = markKindFromFile(m.kind);   // the file's 8 a place, 9 a work
            if (m.kind < 0 || m.kind > MARK_PLACE) m.kind = MARK_PLACE;
            marks.push_back(m);
        }
        else if (key == "shownmark") is >> shownMark;   // after the marks it counts in
        else if (key == "gmark") {   // <x> <y> <z> <lent> <when> <name...>
            GalaxyMark m; int lentFlag = 0; std::string rest;
            is >> m.x >> m.y >> m.z >> lentFlag >> m.when;
            if (is.fail()) continue;
            std::getline(is, rest); if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            m.name = rest; m.lent = lentFlag != 0;
            galaxyMarks.push_back(m);
        }
        else if (key == "shard") { std::string k; is >> k; shards.insert(k); }
        else if (key == "decoded") { std::string k; int n = 0; is >> k >> n; decoded[k] = n; }
        else if (key == "signal") { std::string k; int n = 0; is >> k >> n; signals[k] = n; }
        else if (key == "heard") { std::string k; is >> k; heard.insert(k); }
        else if (key == "grave") { std::string k; is >> k; graves.insert(k); }
        else if (key == "ended") { std::string k; is >> k; ended.insert(k); }
        else if (key == "lent") { std::string k; is >> k; lent.insert(k); }
        else if (key == "instrument") { std::string k; is >> k; instruments.insert(k); }
        else if (key == "microbe" || key == "lentmicrobe") {   // Z-04: <key> <seen> <forms>; Z-05: [<stages> <doubling> <cell> <tempC> <light> <energy> <salt>
            std::string k; MicrobeSheet m; is >> k >> m.seen >> m.forms;   // [<slept years>]] (Z-07); Z-07: a friend's lent alike
            if (is.fail()) continue;
            MicrobeSheet g = m;
            is >> g.stages >> g.doublingS >> g.cell >> g.tempC >> g.light >> g.energy >> g.salt;
            if (!is.fail()) { double slept = 0; is >> slept; if (!is.fail()) g.sleptYears = slept; else is.clear(); }
            (key == "microbe" ? microbes : lentMicrobes)[k] = is.fail() ? m : g;
        }
        else if (key == "answers" || key == "lentanswers") {   // R-437: <key> <answers> <movingLo> <movingHi> <stillCold> <stillHot> <stressC> <stressAct>
            std::string k; MicrobeSheet a;   // (after its sheet's line: the sheet's fields its own)
            is >> k >> a.answers >> a.movingLo >> a.movingHi >> a.stillCold >> a.stillHot >> a.stressC >> a.stressAct;
            if (is.fail()) continue;
            MicrobeSheet& m = (key == "answers" ? microbes : lentMicrobes)[k];
            m.answers = a.answers; m.movingLo = a.movingLo; m.movingHi = a.movingHi; m.stillCold = a.stillCold; m.stillHot = a.stillHot;
            m.stressC = a.stressC; m.stressAct = a.stressAct;
        }
        else if (key == "jarband" || key == "lentjarband") {   // R-439: <key> <layer> (after its sheet's line)
            std::string k; int l = -1; is >> k >> l;
            if (!is.fail() && l >= 0) (key == "jarband" ? microbes : lentMicrobes)[k].jarLayer = l;
        }
        else if (key == "sampled") { std::string k; is >> k; if (!is.fail()) sampled.insert(k); }   // Z-07: a world a sample was taken on
        else if (key == "strain") {   // Z-05: <key> <parent> <what> <value>
            std::string k; StrainRecord r; is >> k >> r.parent >> r.what >> r.value;
            if (!is.fail()) strains[k] = r;
        }
        else if (key == "watermark") { std::string k; is >> k; if (!is.fail()) watermarks.insert(k); }   // Z-06: a made species sequenced
        else if (key == "probe") {
            ProbeRecord r; int lentFlag = 0; std::string storm;
            is >> r.id >> lentFlag >> r.body >> r.gen >> r.saved >> r.launchT >> r.aimLat >> r.aimLon >> r.seed >> r.startX >> r.startY >> r.startZ >> r.headYaw >> r.descYaw >> r.chuteOpen >>
                    r.chuteClose >> r.crushBar >> r.endClock >> r.endBar >> r.endKm >> r.endK >> r.endCause >> storm;
            if (!is.fail()) { r.lent = lentFlag != 0; r.storm = storm == "-" ? std::string() : storm; probes.push_back(r); }
        } else if (key == "probecam" || key == "probetime" || key == "probeprofile" || key == "probelayers") {   // the record of that seed's
            uint64_t seed = 0; size_t n = 0; is >> seed >> n;
            int at = !probes.empty() && probes.back().seed == seed ? (int)probes.size() - 1 : probeIndexBySeed(seed);
            if (at < 0 || is.fail() || n > 200000) continue;
            ProbeRecord& r = probes[at];
            if (key == "probeprofile") {
                r.profile.clear();
                for (size_t i = 0; i < n; i++) { ProbeSample q; is >> q.clock >> q.bar >> q.tempK >> q.windE >> q.light >> q.layer >> q.flashes; if (is.fail()) break; r.profile.push_back(q); }
            } else {
                std::vector<double>& v = key == "probecam" ? r.cam : (key == "probetime" ? r.time : r.layers);
                v.clear();
                for (size_t i = 0; i < n; i++) { double x; is >> x; if (is.fail()) break; v.push_back(x); }
            }
        }
        else if (key == "class") { int c; is >> c; classesSeen.insert(c); }
        else if (key == "type") { int c; is >> c; typesSeen.insert(c); }
        else if (key == "history") { std::string k; is >> k; history.push_back(k); }
        else if (key == "home") is >> home;
        else if (key == "gen") is >> genVersion;
        else if (key == "records") is >> recordsScheme;
        else if (key == "log") {
            LogEntry e; std::string rest; is >> e.t >> e.kind; std::getline(is, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            e.text = rest; log.push_back(e);
        }
        else if (key == "stat") {
            std::string n; double v; is >> n >> v;
            if (n == "furthest") furthestFromHomeLY = v; else if (n == "walk") longestWalkM = v; else if (n == "highest") highestPointM = v;
            else if (n == "walked") totalWalkedM = v; else if (n == "driven") totalDrivenM = v; else if (n == "flown") totalFlownM = v; else if (n == "screenshots") screenshots = (int)v;
            else if (n == "creatures") creaturesSeen = (int)v; else if (n == "topspeed") topSpeedKmh = v; else if (n == "longestdrive") longestDriveM = v;
            else if (n == "probes") probesSent = (int)v;
            else if (n == "drones") droneTrips = (int)v;
            else if (n == "dronefinds") droneFinds = (int)v;
            else if (n == "samples") samplesTaken = (int)v;   // Z-07
        }
    }
    if (shownMark < -1 || shownMark >= (int)marks.size()) shownMark = -1;
    for (ProbeRecord& r : probes) r.lines = probeLines(r);   // once the whole file is read (a record's lines follow its `probe` line)
    if (recordsScheme < 3) {   // (2): the records' contents new; B-414 (3): a people's records all of the age it reached
        migratedShards = 0;
        for (const std::string& k : shards) if (k.find("/S") != std::string::npos) migratedShards++;
    }
    // the second people's shards at the records' stride, everywhere a shard's key is kept
    if (recordsScheme < 2) { rekeySet(shards); rekeySet(heard); rekeySet(lent); rekeyMap(decoded); rekeyMap(names); rekeyMap(inbox); }
    return true;
}

std::string probeLines(const ProbeRecord& r) {
    std::ostringstream f;
    f.precision(12);   // the guide's file
    auto list = [&](const char* k, const std::vector<double>& v) { f << k << " " << r.seed << " " << v.size(); for (double x : v) f << " " << x; f << "\n"; };
    list("probecam", r.cam); list("probetime", r.time);
    f << "probeprofile " << r.seed << " " << r.profile.size();
    for (const ProbeSample& q : r.profile) f << " " << q.clock << " " << q.bar << " " << q.tempK << " " << q.windE << " " << q.light << " " << q.layer << " " << q.flashes;
    f << "\n";
    list("probelayers", r.layers);
    return f.str();
}

bool Guide::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f.precision(12);
    f << "vesperis-guide 1\n";
    f << "gen " << genVersion << "\n";
    f << "records 3\n";   // T-04: the shards' keys at the records' stride (people x 1000 + index); B-414: the records of the age reached
    f << "microbes 1\n";  // Z-04: the species' keys (`<body>/M<index>`) of generation 17's lists
    if (!home.empty()) f << "home " << home << "\n";
    for (auto& kv : names) f << "name " << kv.first << " " << kv.second << "\n";
    for (auto& kv : inbox) f << "inbox " << kv.first << " " << kv.second << "\n";
    for (auto& k : visited) f << "visited " << k << "\n";
    for (auto& k : landed) f << "landed " << k << "\n";
    for (const SurfaceMark& m : marks)
        f << "mark " << m.body << " " << m.lat << " " << m.lon << " " << markKindToFile(m.kind) << " " << (m.lent ? 1 : 0) << " " << m.when << (m.name.empty() ? "" : " ") << m.name
          << "\n";   // (T-03: R-408's numbers kept)
    if (shownMark >= 0 && shownMark < (int)marks.size()) f << "shownmark " << shownMark << "\n";
    for (const GalaxyMark& m : galaxyMarks) f << "gmark " << m.x << " " << m.y << " " << m.z << " " << (m.lent ? 1 : 0) << " " << m.when << (m.name.empty() ? "" : " ") << m.name << "\n";
    for (auto& k : shards) f << "shard " << k << "\n";
    for (auto& kv : decoded) f << "decoded " << kv.first << " " << kv.second << "\n";
    for (auto& kv : signals) f << "signal " << kv.first << " " << kv.second << "\n";
    for (auto& k : heard) f << "heard " << k << "\n";
    for (auto& k : graves) f << "grave " << k << "\n";
    for (auto& k : ended) f << "ended " << k << "\n";
    for (auto& k : lent) f << "lent " << k << "\n";
    for (auto& k : instruments) f << "instrument " << k << "\n";
    auto sheet = [&](const char* kind, const std::string& k, const MicrobeSheet& m) {
        f << kind << " " << k << " " << m.seen << " " << m.forms;
        if (m.stages || m.doublingS > 0 || m.sleptYears > 0) {
            f << " " << m.stages << " " << m.doublingS << " " << m.cell << " " << m.tempC << " " << m.light << " " << m.energy << " " << m.salt;   // Z-05
            if (m.sleptYears > 0) f << " " << m.sleptYears;   // Z-07
        }
        f << "\n";
    };
    for (auto& kv : microbes) sheet("microbe", kv.first, kv.second);
    for (auto& kv : lentMicrobes) sheet("lentmicrobe", kv.first, kv.second);   // Z-07
    auto answers = [&](const char* kind, const std::string& k, const MicrobeSheet& m) {   // R-437: after the sheets
        if (m.answered()) f << kind << " " << k << " " << m.answers << " " << m.movingLo << " " << m.movingHi << " " << m.stillCold << " " << m.stillHot << " " << m.stressC
                            << " " << m.stressAct << "\n";
    };
    for (auto& kv : microbes) answers("answers", kv.first, kv.second);
    for (auto& kv : lentMicrobes) answers("lentanswers", kv.first, kv.second);
    for (auto& kv : microbes) if (kv.second.jarLayer >= 0) f << "jarband " << kv.first << " " << kv.second.jarLayer << "\n";   // R-439
    for (auto& kv : lentMicrobes) if (kv.second.jarLayer >= 0) f << "lentjarband " << kv.first << " " << kv.second.jarLayer << "\n";
    for (auto& k : sampled) f << "sampled " << k << "\n";
    for (auto& kv : strains) f << "strain " << kv.first << " " << kv.second.parent << " " << kv.second.what << " " << kv.second.value << "\n";
    for (auto& k : watermarks) f << "watermark " << k << "\n";   // Z-06
    for (const ProbeRecord& r : probes) {
        f << "probe " << r.id << " " << (r.lent ? 1 : 0) << " " << r.body << " " << r.gen << " " << r.saved << " " << r.launchT << " " << r.aimLat << " " << r.aimLon << " " << r.seed << " "
          << r.startX << " " << r.startY << " " << r.startZ << " " << r.headYaw << " " << r.descYaw << " " << r.chuteOpen << " " << r.chuteClose << " " << r.crushBar << " " << r.endClock << " "
          << r.endBar << " " << r.endKm << " " << r.endK << " " << r.endCause << " " << (r.storm.empty() ? "-" : r.storm) << "\n";
        if (r.lines.empty()) r.lines = probeLines(r);   // a record made by a path that did not make them
        f << r.lines;
    }
    for (int c : classesSeen) f << "class " << c << "\n";
    for (int c : typesSeen) f << "type " << c << "\n";
    for (auto& k : history) f << "history " << k << "\n";
    f << "stat furthest " << furthestFromHomeLY << "\nstat walk " << longestWalkM << "\nstat highest " << highestPointM << "\nstat walked " << totalWalkedM << "\nstat driven " << totalDrivenM
      << "\nstat flown " << totalFlownM << "\nstat screenshots " << screenshots << "\nstat creatures " << creaturesSeen << "\nstat topspeed " << topSpeedKmh << "\nstat longestdrive " << longestDriveM
      << "\nstat probes " << probesSent << "\nstat drones " << droneTrips << "\nstat dronefinds " << droneFinds << "\nstat samples " << samplesTaken << "\n";
    for (auto& e : log) f << "log " << e.t << " " << e.kind << " " << e.text << "\n";
    return true;
}

int Guide::importInbox(const std::string& path, int* lentOut, int* probesOut, int* marksOut, int* galaxyOut, int* microbesOut) {
    Guide other;
    if (!other.load(path)) return -1;
    int n = 0;
    for (auto& kv : other.names) {
        if (names.count(kv.first)) continue;   // yours win
        if (inbox[kv.first] != kv.second) { inbox[kv.first] = kv.second; n++; }
    }
    int l = 0;   // C-14: the shards the friend found and read are lent (what they were lent themselves does not travel on, like the inbox's names)
    for (const auto& kv : other.decoded) {
        if (!other.shards.count(kv.first) || shards.count(kv.first)) continue;
        if (lent.insert(kv.first).second) l++;
    }
    if (lentOut) *lentOut = l;
    int pn = 0;   // the friend's own recordings, lent (theirs lent to them do not travel on); dated by the lending for the gallery's order
    for (const ProbeRecord& r : other.probes) {
        if (r.lent || probeIndexBySeed(r.seed) >= 0) continue;
        ProbeRecord c = r; c.lent = true; c.saved = (long long)std::time(nullptr);
        probes.push_back(c); pn++;
    }
    if (probesOut) *probesOut = pn;
    int mn = 0;   // the friend's own marks, lent (one where a mark already stands, within about 30 m, is the same place)
    for (const SurfaceMark& m : other.marks) {
        if (m.lent) continue;
        bool there = false;
        for (const SurfaceMark& o : marks) if (o.body == m.body && std::fabs(o.lat - m.lat) < 5e-6 && std::fabs(o.lon - m.lon) < 5e-6) { there = true; break; }
        if (there) continue;
        SurfaceMark c = m; c.lent = true; c.when = (long long)std::time(nullptr);
        marks.push_back(c); mn++;
    }
    if (marksOut) *marksOut = mn;
    int gn = 0;   // the friend's own marks of the galaxy, lent (one within a light year of a mark already there is the same place)
    for (const GalaxyMark& m : other.galaxyMarks) {
        if (m.lent) continue;
        bool there = false;
        for (const GalaxyMark& o : galaxyMarks) if (std::fabs(o.x - m.x) < 1 && std::fabs(o.y - m.y) < 1 && std::fabs(o.z - m.z) < 1) { there = true; break; }
        if (there) continue;
        GalaxyMark c = m; c.lent = true; c.when = (long long)std::time(nullptr);
        galaxyMarks.push_back(c); gn++;
    }
    if (galaxyOut) *galaxyOut = gn;
    int un = 0;   // Z-07: the sheets of the species the friend saw (not those lent to them; the explorer's own win), an earlier loan's added to
    for (const auto& kv : other.microbes) {
        if (microbes.count(kv.first)) continue;
        const auto it = lentMicrobes.find(kv.first);
        if (it == lentMicrobes.end()) { lentMicrobes[kv.first] = kv.second; un++; continue; }
        MicrobeSheet& m = it->second; const MicrobeSheet& o = kv.second;
        m.seen |= o.seen; m.forms |= o.forms; m.stages |= o.stages;
        if (o.doublingS > 0 && (m.doublingS <= 0 || o.doublingS < m.doublingS)) { m.doublingS = o.doublingS; m.cell = o.cell; m.tempC = o.tempC; m.light = o.light; m.energy = o.energy; m.salt = o.salt; }
        m.sleptYears = std::max(m.sleptYears, o.sleptYears);
        m.answers |= o.answers;   // R-437: what the friend saw it answer too
        m.movingLo = std::min(m.movingLo, o.movingLo); m.movingHi = std::max(m.movingHi, o.movingHi);
        m.stillCold = std::max(m.stillCold, o.stillCold); m.stillHot = std::min(m.stillHot, o.stillHot);
        if (m.stressAct < 0 && o.stressAct >= 0) { m.stressAct = o.stressAct; m.stressC = o.stressC; }
        if (m.jarLayer < 0) m.jarLayer = o.jarLayer;   // R-439
    }
    if (microbesOut) *microbesOut = un;
    return n;
}

double Guide::deepestProbeKm() const {
    double d = 0;
    for (const ProbeRecord& r : probes) if (!r.lent) d = std::max(d, r.endKm);
    return d;
}

int Guide::probeIndexBySeed(uint64_t seed) const {
    for (int i = (int)probes.size() - 1; i >= 0; i--) if (probes[i].seed == seed) return i;
    return -1;
}

int Guide::lentCount() const {
    int n = 0;
    for (const std::string& k : lent) if (!shards.count(k)) n++;
    return n;
}

int Guide::shardWorlds() const {   // C-03: the distinct body keys before "/S"
    std::set<std::string> worlds;
    for (const std::string& k : shards) { size_t p = k.rfind("/S"); if (p != std::string::npos) worlds.insert(k.substr(0, p)); }
    return (int)worlds.size();
}

void Guide::addLog(double t, const std::string& kind, const std::string& text) {
    LogEntry e; e.t = t; e.kind = kind; e.text = text;
    log.push_back(e);
    if (log.size() > 2000) log.erase(log.begin(), log.begin() + 200);
}

void Guide::pushHistory(const std::string& key) {
    if (key.empty()) return;
    if (!history.empty() && history.back() == key) return;
    history.push_back(key);
    if (history.size() > 20) history.erase(history.begin());
}

