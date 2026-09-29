// Tiny portable file-system helpers (M6-03: the Windows build has no POSIX mkdir).
#pragma once
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

inline bool makeDir(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}
inline bool fileExists(const std::string& path) { struct stat st; return stat(path.c_str(), &st) == 0; }
