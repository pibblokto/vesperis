#pragma once
// The text helpers every layer shares
#include <cctype>
#include <string>

inline std::string lowerOf(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }
