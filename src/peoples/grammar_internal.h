#pragma once
// E-10: grammar.cpp's constants and small helpers, for its parts alone (grammar, grammar_parse, grammar_words): the catalogue's
// dryness, dialect, fatigue and retries, the string and word tests, an age's or a number's value, a word's vowel sound
#include "grammar.h"
#include "../core/rng.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <unordered_set>

// the catalogue's dryness: an alternative the people's earlier records used k times weighs 1 / (1 + VG_DRYNESS k) (H-03's 1.5; H-04: 4,
// so a people's records go through a part's alternatives before they come back to one, its weights still telling which first)
constexpr double VG_DRYNESS = 4.0;
// H-04's second pass: a people's dialect (an alternative's weight for a people times exp(sigma z), z a normal of the people's seed and the
// alternative's key; the genres' own `steady` root leans less), the words worn out (an alternative weighs 1 / (1 + VG_FATIGUE n), n the
// people's uses of its most used distinctive word: a word in VG_DISTINCT alternatives or fewer), a repeated sentence drawn again (a rule
// VG_RETRIES times at most, a record VG_RETRY_BUDGET draws undone in all)
constexpr double VG_DIALECT = 0.9, VG_DIALECT_STEADY = 0.45;
constexpr double VG_FATIGUE = 1.2;
constexpr int VG_DISTINCT = 24;
constexpr int VG_RETRIES = 6, VG_RETRY_BUDGET = 160;

inline double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline std::string lower(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }
inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
inline bool isVowelChar(char c) { c = (char)std::tolower((unsigned char)c); return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }
inline bool endsWith(const std::string& w, const char* x) { size_t n = std::strlen(x); return w.size() >= n && w.compare(w.size() - n, n, x) == 0; }
inline bool startsWith(const std::string& w, const char* x) { size_t n = std::strlen(x); return w.size() >= n && w.compare(0, n, x) == 0; }
inline int ageOf(const std::string& s) { for (int i = 0; i < 6; i++) if (s == VG_AGES[i]) return i; return -1; }
// a number in a value: digits ("212", "0.5") or an age's name (its order); false when it is neither
inline bool numberOf(const std::string& s, double& out) {
    if (s.empty()) return false;
    int a = ageOf(s); if (a >= 0) { out = a; return true; }
    char* e = nullptr; out = std::strtod(s.c_str(), &e);
    return e && *e == 0;
}

// the sound a word starts with, for "a" and "an": a vowel's, except the vowels that sound as a consonant (one, a use, a unit, a
// European) and the silent h (an hour, an heir); a numeral by how it is said (an 8, an 11, an 18)
inline bool vowelSound(const std::string& word) {
    std::string w = lower(word);
    while (!w.empty() && !std::isalnum((unsigned char)w[0])) w.erase(0, 1);
    if (w.empty()) return false;
    if (std::isdigit((unsigned char)w[0])) {
        if (w[0] == '8') return true;
        std::string d; for (char c : w) { if (std::isdigit((unsigned char)c)) d += c; else if (c != ',') break; }
        return (d.size() == 2 || d.size() == 5) && (startsWith(d, "11") || startsWith(d, "18"));
    }
    static const char* const CONSONANT[] = {"one", "once", "eu", "ewe", "uni", "use", "usu", "uti", "ura", "uri", "uro", "ubi", "uku", "ufo", "unanim", nullptr};
    static const char* const SILENT_H[] = {"hour", "honest", "honour", "honor", "heir", nullptr};
    for (int i = 0; SILENT_H[i]; i++) if (startsWith(w, SILENT_H[i])) return true;
    if (!isVowelChar(w[0])) return false;
    for (int i = 0; CONSONANT[i]; i++) if (startsWith(w, CONSONANT[i])) return false;
    return true;
}

inline bool isPunct(char c) { return c == ',' || c == '.' || c == ';' || c == ':' || c == '?' || c == '!'; }
