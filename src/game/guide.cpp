#include "guide.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cmath>

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

bool Guide::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    std::getline(f, line);
    if (line.rfind("vesperis-guide", 0) != 0) return false;
    while (std::getline(f, line)) {
        std::istringstream is(line);
        std::string key; is >> key;
        if (key == "name" || key == "inbox") {
            std::string k, rest; is >> k; std::getline(is, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            (key == "name" ? names : inbox)[k] = rest;
        } else if (key == "visited") { std::string k; is >> k; visited.insert(k); }
        else if (key == "landed") { std::string k; is >> k; landed.insert(k); }
        else if (key == "mark") {   // R-408: <body> <lat> <lon> <kind> <lent> <when> <name...>
            SurfaceMark m; int lentFlag = 0; std::string rest;
            is >> m.body >> m.lat >> m.lon >> m.kind >> lentFlag >> m.when;
            if (is.fail()) continue;
            std::getline(is, rest); if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            m.name = rest; m.lent = lentFlag != 0;
            if (m.kind < 0 || m.kind > MARK_PLACE) m.kind = MARK_PLACE;
            marks.push_back(m);
        }
        else if (key == "shard") { std::string k; is >> k; shards.insert(k); }   // C-03
        else if (key == "decoded") { std::string k; int n = 0; is >> k >> n; decoded[k] = n; }   // C-06
        else if (key == "signal") { std::string k; int n = 0; is >> k >> n; signals[k] = n; }   // C-07
        else if (key == "heard") { std::string k; is >> k; heard.insert(k); }   // C-07
        else if (key == "grave") { std::string k; is >> k; graves.insert(k); }   // C-12
        else if (key == "ended") { std::string k; is >> k; ended.insert(k); }   // C-12
        else if (key == "lent") { std::string k; is >> k; lent.insert(k); }   // C-14
        else if (key == "probe") {   // X-04
            ProbeRecord r; int lentFlag = 0; std::string storm;
            is >> r.id >> lentFlag >> r.body >> r.gen >> r.saved >> r.launchT >> r.aimLat >> r.aimLon >> r.seed >> r.startX >> r.startY >> r.startZ >> r.headYaw >> r.descYaw >> r.chuteOpen >> r.chuteClose >> r.crushBar
               >> r.endClock >> r.endBar >> r.endKm >> r.endK >> r.endCause >> storm;
            if (!is.fail()) { r.lent = lentFlag != 0; r.storm = storm == "-" ? std::string() : storm; probes.push_back(r); }
        } else if (key == "probecam" || key == "probetime" || key == "probeprofile" || key == "probelayers") {   // X-04: the record of that seed's
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
            else if (n == "probes") probesSent = (int)v;   // X-04
        }
    }
    return true;
}

bool Guide::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f.precision(12);
    f << "vesperis-guide 1\n";
    f << "gen " << genVersion << "\n";
    if (!home.empty()) f << "home " << home << "\n";
    for (auto& kv : names) f << "name " << kv.first << " " << kv.second << "\n";
    for (auto& kv : inbox) f << "inbox " << kv.first << " " << kv.second << "\n";
    for (auto& k : visited) f << "visited " << k << "\n";
    for (auto& k : landed) f << "landed " << k << "\n";
    for (const SurfaceMark& m : marks) f << "mark " << m.body << " " << m.lat << " " << m.lon << " " << m.kind << " " << (m.lent ? 1 : 0) << " " << m.when << (m.name.empty() ? "" : " ") << m.name << "\n";   // R-408
    for (auto& k : shards) f << "shard " << k << "\n";   // C-03
    for (auto& kv : decoded) f << "decoded " << kv.first << " " << kv.second << "\n";   // C-06
    for (auto& kv : signals) f << "signal " << kv.first << " " << kv.second << "\n";   // C-07
    for (auto& k : heard) f << "heard " << k << "\n";   // C-07
    for (auto& k : graves) f << "grave " << k << "\n";   // C-12
    for (auto& k : ended) f << "ended " << k << "\n";   // C-12
    for (auto& k : lent) f << "lent " << k << "\n";   // C-14
    for (const ProbeRecord& r : probes) {   // X-04
        f << "probe " << r.id << " " << (r.lent ? 1 : 0) << " " << r.body << " " << r.gen << " " << r.saved << " " << r.launchT << " " << r.aimLat << " " << r.aimLon << " " << r.seed << " " << r.startX << " " << r.startY << " " << r.startZ
          << " " << r.headYaw << " " << r.descYaw << " " << r.chuteOpen << " " << r.chuteClose << " " << r.crushBar << " " << r.endClock << " " << r.endBar << " " << r.endKm << " " << r.endK << " " << r.endCause << " " << (r.storm.empty() ? "-" : r.storm) << "\n";
        auto list = [&](const char* k, const std::vector<double>& v) { f << k << " " << r.seed << " " << v.size(); for (double x : v) f << " " << x; f << "\n"; };
        list("probecam", r.cam); list("probetime", r.time);
        f << "probeprofile " << r.seed << " " << r.profile.size();
        for (const ProbeSample& q : r.profile) f << " " << q.clock << " " << q.bar << " " << q.tempK << " " << q.windE << " " << q.light << " " << q.layer << " " << q.flashes;
        f << "\n";
        list("probelayers", r.layers);
    }
    for (int c : classesSeen) f << "class " << c << "\n";
    for (int c : typesSeen) f << "type " << c << "\n";
    for (auto& k : history) f << "history " << k << "\n";
    f << "stat furthest " << furthestFromHomeLY << "\nstat walk " << longestWalkM << "\nstat highest " << highestPointM
      << "\nstat walked " << totalWalkedM << "\nstat driven " << totalDrivenM << "\nstat flown " << totalFlownM << "\nstat screenshots " << screenshots << "\nstat creatures " << creaturesSeen << "\nstat topspeed " << topSpeedKmh << "\nstat longestdrive " << longestDriveM << "\nstat probes " << probesSent << "\n";
    for (auto& e : log) f << "log " << e.t << " " << e.kind << " " << e.text << "\n";
    return true;
}

int Guide::importInbox(const std::string& path, int* lentOut, int* probesOut, int* marksOut) {
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
    int pn = 0;   // X-04: the friend's own recordings, lent (theirs lent to them do not travel on); dated by the lending for the gallery's order
    for (const ProbeRecord& r : other.probes) {
        if (r.lent || probeIndexBySeed(r.seed) >= 0) continue;
        ProbeRecord c = r; c.lent = true; c.saved = (long long)std::time(nullptr);
        probes.push_back(c); pn++;
    }
    if (probesOut) *probesOut = pn;
    int mn = 0;   // R-408: the friend's own marks, lent (one where a mark already stands, within about 30 m, is the same place)
    for (const SurfaceMark& m : other.marks) {
        if (m.lent) continue;
        bool there = false;
        for (const SurfaceMark& o : marks) if (o.body == m.body && std::fabs(o.lat - m.lat) < 5e-6 && std::fabs(o.lon - m.lon) < 5e-6) { there = true; break; }
        if (there) continue;
        SurfaceMark c = m; c.lent = true; c.when = (long long)std::time(nullptr);
        marks.push_back(c); mn++;
    }
    if (marksOut) *marksOut = mn;
    return n;
}

double Guide::deepestProbeKm() const {   // X-04
    double d = 0;
    for (const ProbeRecord& r : probes) if (!r.lent) d = std::max(d, r.endKm);
    return d;
}

int Guide::probeIndexBySeed(uint64_t seed) const {
    for (int i = (int)probes.size() - 1; i >= 0; i--) if (probes[i].seed == seed) return i;
    return -1;
}

int Guide::lentCount() const {   // C-14
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
