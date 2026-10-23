// Basic math types shared by the whole game. Everything is double precision:
// galactic coordinates span ~1e12 km and surface detail is ~1 m.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

constexpr double PI  = 3.14159265358979323846;
constexpr double TAU = 2.0 * PI;
constexpr double DEG = PI / 180.0;

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3() {}
    Vec3(double X, double Y, double Z) : x(X), y(Y), z(Z) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }
};
inline Vec3 operator*(double s, const Vec3& v) { return v * s; }
inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double length(const Vec3& v) { return std::sqrt(dot(v, v)); }
inline double length2(const Vec3& v) { return dot(v, v); }
inline Vec3 normalize(const Vec3& v) {
    double l = length(v);
    return l > 0 ? v / l : Vec3(0, 0, 1);
}
inline Vec3 lerp(const Vec3& a, const Vec3& b, double t) { return a + (b - a) * t; }

// Row-major 3x3 matrix. Used for camera bases and body rotations.
struct Mat3 {
    double m[3][3];
    static Mat3 identity() {
        Mat3 r;
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r.m[i][j] = (i == j) ? 1.0 : 0.0;
        return r;
    }
    // Rows are the basis vectors: transforming p yields (dot(r0,p), dot(r1,p), dot(r2,p)).
    static Mat3 fromRows(const Vec3& r0, const Vec3& r1, const Vec3& r2) {
        Mat3 r;
        r.m[0][0] = r0.x; r.m[0][1] = r0.y; r.m[0][2] = r0.z;
        r.m[1][0] = r1.x; r.m[1][1] = r1.y; r.m[1][2] = r1.z;
        r.m[2][0] = r2.x; r.m[2][1] = r2.y; r.m[2][2] = r2.z;
        return r;
    }
    static Mat3 fromColumns(const Vec3& c0, const Vec3& c1, const Vec3& c2) {
        return fromRows(c0, c1, c2).transposed();
    }
    static Mat3 axisAngle(const Vec3& axisIn, double a) {
        Vec3 u = normalize(axisIn);
        double c = std::cos(a), s = std::sin(a), t = 1 - c;
        Mat3 r;
        r.m[0][0] = t * u.x * u.x + c;       r.m[0][1] = t * u.x * u.y - s * u.z; r.m[0][2] = t * u.x * u.z + s * u.y;
        r.m[1][0] = t * u.x * u.y + s * u.z; r.m[1][1] = t * u.y * u.y + c;       r.m[1][2] = t * u.y * u.z - s * u.x;
        r.m[2][0] = t * u.x * u.z - s * u.y; r.m[2][1] = t * u.y * u.z + s * u.x; r.m[2][2] = t * u.z * u.z + c;
        return r;
    }
    Vec3 operator*(const Vec3& v) const {
        return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
    }
    Mat3 operator*(const Mat3& o) const {
        Mat3 r;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j];
        return r;
    }
    Mat3 transposed() const {
        Mat3 r;
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r.m[i][j] = m[j][i];
        return r;
    }
    Vec3 row(int i) const { return {m[i][0], m[i][1], m[i][2]}; }
};

// Camera basis from a compass heading (yaw, 0 = +z "north", 90deg = +x "east")
// and a pitch (positive looks up). Rows: right, up, forward.
inline Mat3 cameraBasis(double yaw, double pitch) {
    Vec3 f(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
    Vec3 r(std::cos(yaw), 0, -std::sin(yaw));
    Vec3 u = cross(f, r);   // right-handed: right x up = forward
    return Mat3::fromRows(r, u, f);
}

struct RGB {
    float r = 0, g = 0, b = 0;
    constexpr RGB() {}
    constexpr RGB(float R, float G, float B) : r(R), g(G), b(B) {}   // P-00: constexpr, for the kind table
    RGB operator*(float s) const { return {r * s, g * s, b * s}; }
    RGB operator*(const RGB& o) const { return {r * o.r, g * o.g, b * o.b}; }
    RGB operator+(const RGB& o) const { return {r + o.r, g + o.g, b + o.b}; }
};
inline RGB lerp(const RGB& a, const RGB& b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}
inline RGB clampRGB(const RGB& c) {
    return {std::min(1.f, std::max(0.f, c.r)), std::min(1.f, std::max(0.f, c.g)), std::min(1.f, std::max(0.f, c.b))};
}

inline double clampd(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline double lerpd(double a, double b, double t) { return a + (b - a) * t; }
inline double smoothstep(double e0, double e1, double x) {
    if (e1 == e0) return x < e0 ? 0.0 : 1.0;   // B-409: equal edges are a step (0/0 was a NaN, a black picture)
    double t = clampd((x - e0) / (e1 - e0), 0.0, 1.0);
    return t * t * (3 - 2 * t);
}
// F-07: floor to an integer. `ifloorTrunc`: the truncation, less one where it rounded a negative non-integer up: the same integer as
// std::floor for every input in range, without the library call baseline x86-64 (the Linux and Windows zips) makes of std::floor;
// where the target rounds in one instruction (arm64, x86-64 from SSE4.1: the Mac's) std::floor stays (the truncation's is dearer there)
inline int64_t ifloorTrunc(double v) { const int64_t i = (int64_t)v; return i - (v < (double)i); }
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__SSE4_1__)
inline int64_t ifloor64(double v) { return (int64_t)std::floor(v); }
#else
inline int64_t ifloor64(double v) { return ifloorTrunc(v); }
#endif
inline int ifloor(double v) { return (int)ifloor64(v); }
inline double wrapAngle(double a) {  // to [-PI, PI)
    a = std::fmod(a + PI, TAU);
    if (a < 0) a += TAU;
    return a - PI;
}
inline double wrap2pi(double a) {  // to [0, 2PI)
    // F-07: within a turn of the range, the value fmod gives without the call (fmod returns [0, TAU) as it is, a - TAU over [TAU,
    // 2 TAU) is exact by Sterbenz, (-TAU, 0) has TAU added either way); -0.0 returns itself as before; NaN and the infinities fall through
    if (a >= 0 && a < TAU) return a;
    if (a >= TAU && a < 2 * TAU) return a - TAU;
    if (a < 0 && a > -TAU) return a + TAU;
    a = std::fmod(a, TAU);
    if (a < 0) a += TAU;
    return a;
}
