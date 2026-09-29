#include "guide.h"
#include <fstream>
#include <sstream>
#include <algorithm>

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
        else if (key == "landmark") { std::string k; is >> k; landmarksSeen.insert(k); }
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
            else if (n == "walked") totalWalkedM = v; else if (n == "driven") totalDrivenM = v; else if (n == "screenshots") screenshots = (int)v;
            else if (n == "creatures") creaturesSeen = (int)v; else if (n == "topspeed") topSpeedKmh = v; else if (n == "longestdrive") longestDriveM = v;
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
    for (auto& k : landmarksSeen) f << "landmark " << k << "\n";
    for (int c : classesSeen) f << "class " << c << "\n";
    for (int c : typesSeen) f << "type " << c << "\n";
    for (auto& k : history) f << "history " << k << "\n";
    f << "stat furthest " << furthestFromHomeLY << "\nstat walk " << longestWalkM << "\nstat highest " << highestPointM
      << "\nstat walked " << totalWalkedM << "\nstat driven " << totalDrivenM << "\nstat screenshots " << screenshots << "\nstat creatures " << creaturesSeen << "\nstat topspeed " << topSpeedKmh << "\nstat longestdrive " << longestDriveM << "\n";
    for (auto& e : log) f << "log " << e.t << " " << e.kind << " " << e.text << "\n";
    return true;
}

int Guide::importInbox(const std::string& path) {
    Guide other;
    if (!other.load(path)) return -1;
    int n = 0;
    for (auto& kv : other.names) {
        if (names.count(kv.first)) continue;   // yours win
        if (inbox[kv.first] != kv.second) { inbox[kv.first] = kv.second; n++; }
    }
    return n;
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
