#include "site.h"
#include "space/space_view.h"
#include <cmath>

void TerrainCache::init(const SurfaceSite* s, int cell, int logn) {
    site = s; cellSize = cell; logN = logn;
    v.assign((size_t)1 << (2 * logN), TerrainVertex());
}

const TerrainVertex& TerrainCache::at(int cx, int cz) {
    TerrainVertex& t = v[index(cx, cz)];
    if (t.cx != cx || t.cz != cz) {
        t = site->sampleAt((double)cx * cellSize, (double)cz * cellSize, cellSize);
        t.cx = cx; t.cz = cz;
    }
    return t;
}

TerrainVertex SurfaceSite::sampleAt(double x, double z, double detailM) const {
    Vec3 u = unitAt(x, z);
    SurfaceSample s = sampleSurface(gen, u, detailM);
    TerrainVertex t;
    t.h = (float)s.height;
    t.albedo = (float)s.albedo;
    t.veg = (float)s.veg;
    t.material = (uint8_t)s.material;
    t.glow = (uint8_t)(s.glow * 255);
    t.biome = (uint8_t)s.biome;
    t.water = (float)s.water;
    t.shore = (float)s.shore;
    t.scree = (uint8_t)clampi((int)(s.scree * 255 + 0.5), 0, 255);
    return t;
}

double vertexShore(TerrainCache& c, int cx, int cz) {
    const TerrainVertex& tv = c.at(cx, cz);
    if (tv.shore < 1e8f) return tv.shore;             // a river's or a lake's outline, on the bank beyond the plane too
    if (tv.water > -1e8f) return tv.h - tv.water;
    double sum = 0; int n = 0;   // no water field here: the height over the plane of the neighbours that have one
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dz) continue;
            const TerrainVertex& nv = c.at(cx + dx, cz + dz);
            if (nv.water > -1e8f) { sum += nv.water; n++; }
        }
    return n ? tv.h - sum / n : 1e9;
}

void shoreSplitTriangle(const double x[3], const double z[3], const double h[3], const double wl[3], const double sh[3], ShoreSplit& out) {
    out.nWet = out.nDry = 0;
    for (int e = 0; e < 3; e++) {
        int a = e, b = (e + 1) % 3;
        bool aw = sh[a] < 0, bw = sh[b] < 0;
        ShorePt pa{x[a], z[a], h[a], a, a, 0.0};
        if (aw) out.wet[out.nWet++] = pa; else out.dry[out.nDry++] = pa;
        if (aw != bw) {
            double t = sh[a] / (sh[a] - sh[b]);
            ShorePt pc{x[a] + (x[b] - x[a]) * t, z[a] + (z[b] - z[a]) * t, wl[a] + (wl[b] - wl[a]) * t - 0.03, a, b, t};
            out.wet[out.nWet++] = pc; out.dry[out.nDry++] = pc;
        }
    }
}

void SurfaceSite::init(const StarSystem* s, int bodyIndex, double lat, double lon, double t) {
    sys = s; body = bodyIndex; lat0 = lat; lon0 = lon;
    const Body& b = s->bodies[bodyIndex];
    season = s->seasonOf(bodyIndex, t);
    gen = BodyGen::make(b, season);
    R = b.radiusKm * 1000.0;
    gravity = b.gravity;
    up0 = StarSystem::bodyFromLatLon(lat0, lon0);
    Vec3 pole(0, 0, 1);
    east0 = normalize(cross(pole, up0));
    if (length(cross(pole, up0)) < 1e-6) east0 = Vec3(0, 1, 0);
    north0 = cross(up0, east0);
    atmosphere = PLANET_TYPES[b.type].atmosphere;
    hasWater = b.type == PT_FELISIAN || b.type == PT_OCEAN || gen.liquidLevel > -1e8;   // R-304: exotic seas are a sea too
    seaLevel = gen.liquidLevel > -1e8 ? gen.liquidLevel : 0;
    lavaLevel = b.type == PT_MOLTEN ? gen.lavaLevel : -1e9;
    lod0.init(this, 16, 7);
    lod1.init(this, 64, 7);
    lod2.init(this, 512, 6);
    lod3.init(this, 2048, 6);
    lodN.init(this, 4, 8);   // O6-02
    nearBlend = 0; nearR = 0;
    smallBody = R < 300e3;
    escapeVelocity = std::sqrt(2 * gravity * R);
    ringProf = b.rings ? s->ringProfileOf(bodyIndex) : std::vector<float>();
}

Vec3 SurfaceSite::unitAt(double x, double z) const {
    double d = std::sqrt(x * x + z * z);
    if (d < 1e-9) return up0;
    double ang = d / R;
    Vec3 dir = (east0 * x + north0 * z) / d;
    return normalize(up0 * std::cos(ang) + dir * std::sin(ang));
}

void SurfaceSite::localAt(const Vec3& unit, double& x, double& z) const {
    double c = clampd(dot(unit, up0), -1, 1);
    double ang = std::acos(c);
    Vec3 tang = unit - up0 * c;
    double tl = length(tang);
    if (tl < 1e-12 || ang < 1e-12) { x = 0; z = 0; return; }
    tang = tang / tl;
    x = R * ang * dot(tang, east0);
    z = R * ang * dot(tang, north0);
}

void SurfaceSite::latLonAt(double x, double z, double& lat, double& lon) const {
    StarSystem::latLonFromBody(unitAt(x, z), lat, lon);
}

Mat3 SurfaceSite::localFrame(double t) const {
    Mat3 bodyT = sys->bodyFrame(body, t).transposed();   // body -> world
    Vec3 e = bodyT * east0, u = bodyT * up0, n = bodyT * north0;
    return Mat3::fromRows(e, u, n);
}

Vec3 SurfaceSite::worldPos(double t, double x, double z, double alt) const {
    Mat3 bodyT = sys->bodyFrame(body, t).transposed();
    Vec3 u = bodyT * unitAt(x, z);
    return sys->bodyPos(body, t) + u * ((R + alt) / 1000.0);
}

SunInfo SurfaceSite::sun(double t) const {
    SunInfo si;
    Vec3 bp = sys->bodyPos(body, t);
    Vec3 dW = normalize(sys->star.pos - bp);
    Mat3 L = localFrame(t);
    si.dirLocal = L * dW;
    si.altitude = std::asin(clampd(si.dirLocal.y, -1, 1));
    si.azimuth = wrap2pi(std::atan2(si.dirLocal.x, si.dirLocal.z));
    double dist = length(sys->star.pos - bp);
    double flare = starFlare(sys->star, t);   // S-01: a red dwarf's flare doubles its light for a minute and whitens it: a quarter more light past the exposure clamp (its near worlds sit at the clamp already)
    si.lightFactor = std::max(0.62, SpaceRenderer::lightFactor(sys->star.luminosity, dist)) * (1 + 0.25 * flare);
    si.angularRadius = std::asin(clampd(sys->star.radiusKm / dist, 0, 1));
    // M1-08: other bodies between the site and the star cover part of its disc
    si.eclipse = 0;
    {
        Vec3 sitePos = worldPos(t, 0, 0, 0);
        for (int j = 0; j < (int)sys->bodies.size(); j++) {
            if (j == body) continue;
            Vec3 rel = sys->bodyPos(j, t) - sitePos;
            double d = length(rel);
            if (d <= 0) continue;
            double rb = std::asin(clampd(sys->bodies[j].radiusKm / d, 0, 1));
            double theta = std::acos(clampd(dot(rel / d, dW), -1, 1));
            double rs = si.angularRadius;
            double cov = 0;
            if (theta >= rs + rb) cov = 0;
            else if (theta <= std::fabs(rb - rs)) cov = rb >= rs ? 1.0 : (rb * rb) / (rs * rs);
            else cov = clampd((rs + rb - theta) / (2 * std::min(rs, rb)), 0, 1) * std::min(1.0, (rb * rb) / (rs * rs));
            si.eclipse = std::max(si.eclipse, cov);
        }
    }
    si.lightFactor *= (1 - 0.97 * si.eclipse);
    // O0-01: the world's own ring between the site and the sun: a dusky band on the winter side of the equator
    si.ringShadow = 0;
    if (!ringProf.empty()) {
        const Body& b = sys->bodies[body];
        Vec3 P = worldPos(t, 0, 0, 0) - bp;   // km from the centre
        double denom = dot(dW, b.spinAxis);
        if (std::fabs(denom) > 1e-9) {
            double s = -dot(P, b.spinAxis) / denom;
            if (s > 0) {
                double rr = length(P + dW * s);
                double r0 = b.ringInner * b.radiusKm, r1 = b.ringOuter * b.radiusKm;
                if (rr >= r0 && rr <= r1) si.ringShadow = 0.85 * ringProf[clampi((int)((rr - r0) / (r1 - r0) * 255), 0, 255)];
            }
        }
        si.lightFactor *= (1 - 0.8 * si.ringShadow);
    }
    // local time from the hour angle: sub-solar longitude in body frame
    Vec3 sunBody = sys->bodyFrame(body, t) * dW;
    double subLon = std::atan2(sunBody.y, sunBody.x);
    double ha = wrapAngle(lon0 - subLon);   // hour angle, 0 = noon
    si.dayFraction = wrap2pi(ha + PI) / TAU;
    si.color = flare > 0 ? lerp(sys->star.color, RGB(1, 1, 1), (float)(0.35 * std::min(1.0, flare))) : sys->star.color;
    return si;
}

bool SurfaceSite::sun2(double t, SunInfo& si) const {
    if (sys->companion < 0) return false;
    const Body& k = sys->bodies[sys->companion];
    Vec3 bp = sys->bodyPos(body, t);
    Vec3 kp = sys->bodyPos(sys->companion, t);
    double dist = length(kp - bp);
    Vec3 dW = (kp - bp) / std::max(dist, 1.0);
    Mat3 L = localFrame(t);
    si.dirLocal = L * dW;
    si.altitude = std::asin(clampd(si.dirLocal.y, -1, 1));
    si.azimuth = wrap2pi(std::atan2(si.dirLocal.x, si.dirLocal.z));
    si.lightFactor = SpaceRenderer::lightFactor(k.luminosity, dist);
    si.angularRadius = std::asin(clampd(k.radiusKm / dist, 0, 1));
    si.eclipse = 0;
    si.dayFraction = 0;
    si.color = k.color;
    return true;
}

double SurfaceSite::waterAt(double x, double z) {   // B-320: bilinear over the wet corners (the level the sheet is drawn at); it was the highest corner
    double fx = x / lod0.cellSize, fz = z / lod0.cellSize;
    int cx = (int)std::floor(fx), cz = (int)std::floor(fz);
    fx -= cx; fz -= cz;
    double ws[4] = {lod0.at(cx, cz).water, lod0.at(cx + 1, cz).water, lod0.at(cx, cz + 1).water, lod0.at(cx + 1, cz + 1).water};
    double wgt[4] = {(1 - fx) * (1 - fz), fx * (1 - fz), (1 - fx) * fz, fx * fz};
    double sum = 0, wsum = 0;
    for (int k = 0; k < 4; k++) if (ws[k] > -1e8) { sum += ws[k] * wgt[k]; wsum += wgt[k]; }
    return wsum > 1e-6 ? sum / wsum : -1e9;
}

namespace {
// the height of a point inside a triangle piece (fan from [0]), by barycentric interpolation of the fan triangle that holds it
double pieceHeight(const ShorePt* p, int n, double x, double z) {
    double best = p[0].h, bestErr = 1e18;
    for (int k = 1; k + 1 < n; k++) {
        const ShorePt& A = p[0]; const ShorePt& B = p[k]; const ShorePt& C = p[k + 1];
        double det = (B.x - A.x) * (C.z - A.z) - (C.x - A.x) * (B.z - A.z);
        if (std::fabs(det) < 1e-12) continue;
        double u = ((x - A.x) * (C.z - A.z) - (C.x - A.x) * (z - A.z)) / det;
        double v = ((B.x - A.x) * (z - A.z) - (x - A.x) * (B.z - A.z)) / det;
        double err = std::max(0.0, -u) + std::max(0.0, -v) + std::max(0.0, u + v - 1);   // 0 inside
        if (err < bestErr) { bestErr = err; best = A.h + (B.h - A.h) * u + (C.h - A.h) * v; }
    }
    return best;
}

}

// the height of the two triangles of a cell (the split the terrain mesh draws), on any cache; B-322: a cell cut by a
// water's edge is read as the drawn pieces (`shoreSplitTriangle`)
double cacheHeightAt(TerrainCache& c, double x, double z) {
    double cs = c.cellSize;
    double fx = x / cs, fz = z / cs;
    int cx = (int)std::floor(fx), cz = (int)std::floor(fz);
    double tx = fx - cx, tz = fz - cz;
    const TerrainVertex& t1 = c.at(cx, cz); const TerrainVertex& t2 = c.at(cx + 1, cz); const TerrainVertex& t3 = c.at(cx + 1, cz + 1); const TerrainVertex& t4 = c.at(cx, cz + 1);
    double h1 = t1.h, h2 = t2.h, h3 = t3.h, h4 = t4.h;
    bool lower = tx + tz < 1;
    double plain = lower ? h1 + (h2 - h1) * tx + (h4 - h1) * tz : h3 + (h4 - h3) * (1 - tx) + (h2 - h3) * (1 - tz);
    const TerrainVertex* tv[3] = {lower ? &t1 : &t2, lower ? &t2 : &t3, lower ? &t4 : &t4};   // the mesh's triangles: (00, 10, 01) and (10, 11, 01)
    double wMean = 0; int wN = 0;
    for (const TerrainVertex* v : {&t1, &t2, &t3, &t4}) if (v->water > -1e8f) { wMean += v->water; wN++; }
    if (!wN) return plain;
    wMean /= wN;
    double sh[3], xs[3], zs[3], hs[3], wl[3];
    int ox[3] = {lower ? 0 : 1, lower ? 1 : 1, 0}, oz[3] = {0, lower ? 0 : 1, 1};
    bool anyWet = false, anyDry = false;
    for (int k = 0; k < 3; k++) {
        sh[k] = vertexShore(c, cx + ox[k], cz + oz[k]);
        xs[k] = (cx + ox[k]) * cs; zs[k] = (cz + oz[k]) * cs; hs[k] = tv[k]->h;
        wl[k] = tv[k]->water > -1e8f ? tv[k]->water : wMean;
        if (sh[k] < 0) anyWet = true; else anyDry = true;
    }
    if (!anyWet || !anyDry) return plain;
    ShoreSplit sp;
    shoreSplitTriangle(xs, zs, hs, wl, sh, sp);
    // which side of the edge the point lies on: the shore distance is linear over the triangle
    double det = (xs[1] - xs[0]) * (zs[2] - zs[0]) - (xs[2] - xs[0]) * (zs[1] - zs[0]);
    double u = ((x - xs[0]) * (zs[2] - zs[0]) - (xs[2] - xs[0]) * (z - zs[0])) / det;
    double v = ((xs[1] - xs[0]) * (z - zs[0]) - (x - xs[0]) * (zs[1] - zs[0])) / det;
    double shHere = sh[0] + (sh[1] - sh[0]) * u + (sh[2] - sh[0]) * v;
    if (shHere < 0) return sp.nWet >= 3 ? pieceHeight(sp.wet, sp.nWet, x, z) : plain;
    if (sp.nDry < 3) return plain;
    ShorePt dry[4];   // O6-03: the dry piece as drawn: its edge on the cut stands SHORE_LIP over the water
    for (int k = 0; k < sp.nDry; k++) { dry[k] = sp.dry[k]; if (dry[k].a != dry[k].b) dry[k].h += SHORE_LIP; }
    return pieceHeight(dry, sp.nDry, x, z);
}

double SurfaceSite::groundHeightCoarse(double x, double z) { return cacheHeightAt(lod0, x, z); }

double SurfaceSite::groundHeight(double x, double z) {
    double h0 = cacheHeightAt(lod0, x, z);
    if (lod0R > 0 && lod0Band > 0) {   // B-319: across lod0's outer band the drawn ground slides onto lod1's (the ring's edge vertices sit on it)
        double dx = (x - camX) / lod0.cellSize, dz = (z - camZ) / lod0.cellSize;
        double m = smoothstep(lod0R - lod0Band, lod0R - 0.5, std::sqrt(dx * dx + dz * dz));
        if (m > 0) h0 += (cacheHeightAt(lod1, x, z) - h0) * m;
    }
    if (!inNearRing(x, z)) return h0;
    return h0 + (cacheHeightAt(lodN, x, z) - h0) * (1 - nearMorph(x, z));   // O6-02: the near ring's ground, as drawn (B-313: morphed onto lod0 toward its edge)
}
