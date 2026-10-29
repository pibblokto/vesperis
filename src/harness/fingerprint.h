#pragma once
// E-02: the fingerprint modes (fingerprint.cpp, fingerprint_modules.cpp)
#include <cstdint>
#include <cstdio>
#include <string>

#if defined(__GNUC__)
#define FP_PRINTF __attribute__((format(printf, 2, 3)))
#else
#define FP_PRINTF
#endif

int runFingerprint(int argc, char** argv);                    // `fingerprint bodies|stream|peoples|modules [n] [out] [full]`
int fingerprintModules(FILE* out, int every, bool full);      // fingerprint_modules.cpp
uint64_t fpHash(const std::string& bytes);                    // FNV-1a over the bytes (fp5's)
void fpPut(std::string& s, const char* fmt, ...) FP_PRINTF;   // printf onto the end of s
