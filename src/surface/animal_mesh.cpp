#include "animal_mesh.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

namespace {
double bump(double x, double c, double w) { const double d = (x - c) / w; return std::fabs(d) >= 1 ? 0 : 0.5 + 0.5 * std::cos(PI * d); }

// the girth along the trunk (x 0 the rear, 1 the front) as shares of the widest ring's width and depth
void girthAt(int g, double x, int i, int last, double& w, double& h) {
    switch (g) {
        case GIRTH_CHEST: w = 0.55 + 0.45 * std::sin(PI * std::pow(x, 1.6)); h = 0.65 + 0.35 * std::sin(PI * std::pow(x, 1.5)); break;
        case GIRTH_HIPS: w = 0.55 + 0.45 * std::sin(PI * std::pow(x, 0.62)); h = 0.65 + 0.35 * std::sin(PI * std::pow(x, 0.7)); break;
        case GIRTH_WAIST: w = std::max(0.3, std::max(bump(x, 0.28, 0.34), 0.72 * bump(x, 0.8, 0.24))); h = w; break;
        case GIRTH_BEADS: w = (0.8 + 0.2 * std::sin(PI * x)) * ((i % 2) ? 0.8 : 1.0); h = w * 0.9; break;
        case GIRTH_TAPER: w = x < 0.62 ? 0.16 + 0.84 * smoothstep(0, 0.62, x) : 1 - 0.32 * smoothstep(0.62, 1, x); h = w; break;
        case GIRTH_ROUND: w = 0.14 + 0.86 * std::sqrt(std::max(0.0, 1 - (2 * x - 1) * (2 * x - 1))); h = w; break;
        default: w = 0.7 + 0.3 * std::sin(PI * x); h = 0.75 + 0.25 * std::sin(PI * x); break;
    }
    if (g != GIRTH_ROUND && g != GIRTH_TAPER && (i == 0 || i == last)) { w *= 0.82; h *= 0.85; }   // the ends rounded
}

// the knee of a two-bone limb from a to c, bent toward `bend`; stretched straight when c is out of its reach
Vec3 kneeOf(const Vec3& a, const Vec3& c, double l1, double l2, const Vec3& bend) {
    const Vec3 d = c - a;
    double L = length(d);
    if (L < 1e-9) return a + bend * l1;
    const Vec3 u = d / L;
    if (L >= l1 + l2) return a + u * l1;
    L = std::max(L, std::fabs(l1 - l2) + 1e-6);
    const double ca = clampd((l1 * l1 + L * L - l2 * l2) / (2 * l1 * L), -1, 1), sa = std::sqrt(1 - ca * ca);
    Vec3 v = bend - u * dot(bend, u);
    const double vl = length(v);
    v = vl > 1e-9 ? v / vl : normalize(cross(u, Vec3(0.3, 0.9, 0.2)));
    return a + u * (l1 * ca) + v * (l1 * sa);
}

// the gait's place in its cycle for a leg (pair p of n, side 0 the left, 1 the right)
double legOffset(int gait, int p, int n, int s) {
    switch (gait) {
        case GAIT_TROT: return ((p + s) % 2) * 0.5;
        case GAIT_GALLOP: return p == 0 ? 0.5 + s * 0.1 : s * 0.1;
        case GAIT_BOUND: return p == 0 ? 0.5 : 0.0;
        case GAIT_HOP: return 0;
        case GAIT_STRIDE: return s * 0.5;
        case GAIT_CRAWL: return ((p + s) % 2) * 0.5;   // an insect's tripods
        case GAIT_WAVE: return (double)(n - 1 - p) / std::max(1, n) * 0.8 + s * 0.5;   // a wave from the back to the front
        default: return n > 2 ? (double)(n - 1 - p) / n * 0.8 + s * 0.5 : (p == 0 ? 0.25 : 0.0) + s * 0.5;   // a walk's lateral sequence
    }
}
}

AnimalRig rigOf(const AnimalBody& b) {
    AnimalRig R;
    R.b = b;
    R.flies = b.plan == PLAN_FLYER || b.plan == PLAN_GLIDER;
    R.floats = b.plan == PLAN_FLOATER;
    R.swims = b.plan == PLAN_SWIMMER;
    int rings = clampi(b.segs + 1, 2, RIG_RINGS);
    if (b.girth == GIRTH_WAIST) rings = std::max(rings, 6);   // an abdomen, a waist and a thorax take rings enough
    if (b.girth == GIRTH_ROUND) rings = std::max(rings, 5);
    R.rings = rings;
    double maxTop = 0;
    const bool twoHumps = (b.seed >> 7) & 1;
    for (int i = 0; i < rings; i++) {
        const double x = (double)i / (rings - 1);
        double w, h;
        girthAt(b.girth, x, i, rings - 1, w, h);
        R.ringF[i] = b.length * (x - 0.5);
        R.ringW[i] = 0.5 * b.width * w;
        R.ringH[i] = 0.5 * b.depth * h;
        if (b.dorsal == DORSAL_HUMPS) {   // one hump or two over the middle: the ring taller and its middle raised as much
            const double hump = 0.5 * b.dorsalH * (twoHumps ? std::max(bump(x, 0.32, 0.22), bump(x, 0.7, 0.22)) : bump(x, 0.55, 0.3));
            R.ringH[i] += hump; R.ringLift[i] = hump;
        }
        maxTop = std::max(maxTop, R.ringH[i] + R.ringLift[i]);
    }
    const bool legless = b.pairs == 0;
    // a walker's back stands at its size; a flyer perched, a swimmer and a floater sit on legs of their own length
    const bool walker = !R.flies && !R.swims && !R.floats;
    R.spineY = legless ? 0.5 * b.depth : (walker ? std::max(b.size - maxTop, 0.6 * maxTop) : 0.75 * b.leg + 0.8 * maxTop);
    R.bones = clampi(b.joints, 2, 4);
    static const double SHARE[3][4] = {{0.5, 0.5, 0, 0}, {0.4, 0.38, 0.22, 0}, {0.36, 0.34, 0.2, 0.1}};
    for (int j = 0; j < 4; j++) R.boneShare[j] = SHARE[R.bones - 2][j];
    // the pairs along the trunk: a biped's under its hips, a hopper's at its rear, an insect's on its thorax, others front to back
    const int n = std::min(b.pairs, 6);
    for (int p = 0; p < n; p++) {
        double f;
        if (n == 1) f = b.plan == PLAN_HOPPER ? -0.3 : (R.swims ? 0.25 : -0.12);
        else if (b.girth == GIRTH_WAIST && b.plan != PLAN_OCTOPOD) f = 0.45 - 0.35 * p / (n - 1);
        else f = (b.plan == PLAN_CRAWLER ? 0.42 : 0.36) * (1 - 2.0 * p / (n - 1));
        R.legF[p] = f * b.length;
    }
    // each pair's hips over the ground (a pitched trunk's front pair stands higher, on longer legs), its length and a sprawl's reach out
    double meanReach = 0;
    for (int p = 0; p < n; p++) {
        double w = 0, h = 0, lift = 0;
        const double x = clampd(R.legF[p] / b.length + 0.5, 0, 1);
        const double fi = x * (rings - 1);
        const int i0 = std::min(rings - 2, (int)fi);
        const double t = fi - i0;
        w = R.ringW[i0] + (R.ringW[i0 + 1] - R.ringW[i0]) * t; h = R.ringH[i0] + (R.ringH[i0 + 1] - R.ringH[i0]) * t;
        lift = R.ringLift[i0] + (R.ringLift[i0 + 1] - R.ringLift[i0]) * t;
        (void)w;
        const double hip = R.spineY + lift + R.legF[p] * std::sin(b.pitch) - h * (b.stance == STANCE_SPRAWL ? 0.15 : 0.4);
        R.legReach[p] = std::max(0.02 * b.size, hip);
        meanReach += R.legReach[p] / n;
    }
    for (int p = 0; p < n; p++) {
        const double reach = R.legReach[p];
        if (!walker) { R.legLen[p] = b.leg; R.legSpread[p] = b.stance == STANCE_SPRAWL ? 0.45 * b.leg : 0; continue; }
        double L = b.leg * (meanReach > 0 ? reach / meanReach : 1);
        if (b.stance == STANCE_COLUMN) L = reach * 1.005;
        else if (b.stance == STANCE_SPRAWL) { L = std::max(L, reach / 0.8); R.legSpread[p] = std::sqrt(std::max(0.0, 0.85 * L * 0.85 * L - reach * reach)); }
        else L = std::max(L, reach / 0.93);
        R.legLen[p] = L;
    }
    return R;
}

void poseAnimal(const AnimalRig& R, const AnimalPoseIn& in, AnimalPose& P) {
    const AnimalBody& b = R.b;
    const double k = in.scale;
    P.scale = k;
    const double hx = std::sin(in.heading), hz = std::cos(in.heading);
    const Vec3 up(0, 1, 0), fwd(hx, 0, hz), side(-hz, 0, hx);
    P.fwd = fwd; P.up = up; P.side = side;
    auto groundAt = [&](const Vec3& p) {   // the 3 by 3 heights' bilinear surface (held at its edge past it)
        const Vec3 d = p - in.at;
        const double u = clampd(dot(d, fwd) / in.gx, -1, 1) + 1, v = clampd(dot(d, side) / in.gz, -1, 1) + 1;
        const int i = std::min(1, (int)u), j = std::min(1, (int)v);
        const double fu = u - i, fv = v - j;
        const double a = in.ground[i][j] + (in.ground[i + 1][j] - in.ground[i][j]) * fu, c = in.ground[i][j + 1] + (in.ground[i + 1][j + 1] - in.ground[i][j + 1]) * fu;
        return a + (c - a) * fv;
    };
    const bool onGround = !in.aloft && !in.afloat;
    const bool legless = b.pairs == 0;
    const double phase = in.phase, TAUp = TAU * phase;
    // the trunk's tilt: its own (levelled lying down) and the ground's slope under it
    double pitch = b.pitch * (1 - in.lift);
    if (onGround && !legless) pitch += clampd(std::atan2(in.ground[2][1] - in.ground[0][1], 2 * in.gx), -0.35, 0.35);
    if (in.aloft && R.flies) pitch = -0.05 + 0.08 * in.flap;
    const Vec3 T0 = fwd * std::cos(pitch) + up * std::sin(pitch), U0 = up * std::cos(pitch) - fwd * std::sin(pitch);
    // the trunk's middle
    Vec3 base = in.at;
    double hop = 0;
    if (onGround) {
        const double reach0 = b.pairs ? R.legReach[0] : 0;
        const double bob = in.moving ? 0.04 * b.size * k * std::sin(TAUp * 2) : 0;
        hop = (in.gait == GAIT_HOP && in.moving) ? 0.5 * reach0 * k * std::max(0.0, std::sin(TAUp)) : 0;
        double belly = 0;   // lying, the belly rests on the ground
        for (int i = 0; i < R.rings; i++) belly = std::max(belly, R.ringH[i] - R.ringLift[i]);
        base.y = in.at.y + (legless ? 0 : std::max(R.spineY * (1 - 0.72 * in.lift), belly) * k) + bob + hop;
    }
    // the rings, a serpent's and a swimmer's waved sideways, a slug's rippled
    const int n = R.rings;
    P.rings = n;
    double waveA = 0, waves = 1.5, wavePh = phase;
    if (b.plan == PLAN_SERPENT) waveA = (in.moving ? 0.09 : 0.05) * b.length * k;
    if (R.swims && (b.variant == SWIM_FISH || b.variant == SWIM_EEL)) { waveA = (b.variant == SWIM_EEL ? 0.08 : 0.07) * b.length * k; waves = b.variant == SWIM_EEL ? 1.6 : 0.8; wavePh = in.t * 1.2; }
    for (int i = 0; i < n; i++) {
        const double x = (double)i / (n - 1);
        double s = 0;
        if (waveA > 0) {
            const double env = R.swims && b.variant == SWIM_FISH ? std::pow(1 - x, 1.5) : 0.8;
            s = waveA * env * std::sin(TAU * (waves * x - wavePh));
        }
        Vec3 c = base + T0 * (R.ringF[i] * k) + side * s + U0 * (R.ringLift[i] * k);
        if (onGround && legless) c.y = groundAt(c) + R.ringH[i] * k + (b.plan == PLAN_SLUG ? 0.12 * R.ringH[i] * k * std::sin(TAU * (3 * x - phase)) : 0);
        P.ringC[i] = c;
        P.ringW[i] = R.ringW[i] * k; P.ringH[i] = R.ringH[i] * k;
    }
    for (int i = 0; i < n; i++) {
        const Vec3 t = normalize(P.ringC[std::min(n - 1, i + 1)] - P.ringC[std::max(0, i - 1)]);
        Vec3 s = cross(t, U0);
        s = length(s) > 1e-6 ? normalize(s) : side;
        P.ringT[i] = t; P.ringS[i] = s; P.ringU[i] = cross(s, t);
    }
    const Vec3 cFront = P.ringC[n - 1], tFront = P.ringT[n - 1], uFront = P.ringU[n - 1];
    const double hFront = P.ringH[n - 1];
    Vec3 flatFwd(tFront.x, 0, tFront.z);
    flatFwd = length(flatFwd) > 1e-6 ? normalize(flatFwd) : fwd;
    // the neck: up from the front ring at its rise, lowered to the ground grazing, an arched one curving over
    P.neckN = 0;
    Vec3 neckDir = flatFwd;
    if (b.neck > 0.03 * b.size) {
        const int nn = b.neck > b.size ? 3 : 2;
        P.neckN = nn;
        const double np = b.neckUp - in.headPitch * (b.neckUp + 0.73);
        const double a0 = b.arched ? np + 0.35 * (1 - in.headPitch) : np, a1 = b.arched ? np - 0.35 : np;
        Vec3 p = cFront + tFront * (0.2 * hFront) + uFront * (0.25 * hFront);
        P.neckP[0] = p; P.neckR[0] = 0.5 * b.neckThick * k;
        for (int j = 1; j <= nn; j++) {
            const double a = a0 + (a1 - a0) * (j - 0.5) / nn;
            neckDir = flatFwd * std::cos(a) + up * std::sin(a);
            p = p + neckDir * (b.neck * k / nn);
            if (onGround && in.headPitch > 0.3) p.y = std::max(p.y, groundAt(p) + 0.6 * b.headH * k);
            P.neckP[j] = p; P.neckR[j] = 0.5 * b.neckThick * k * (1 - 0.35 * j / nn);
        }
        P.head0 = p;
        P.headDir = normalize(neckDir * 0.6 + flatFwd * 0.5 - up * (0.25 + 0.4 * in.headPitch));
    } else {
        P.head0 = cFront + tFront * (0.3 * hFront);
        P.headDir = normalize(tFront - up * (0.12 + 0.5 * in.headPitch));
    }
    if (R.swims && b.variant == SWIM_LONGNECK) P.headDir = normalize(neckDir * 0.3 + flatFwd * 0.7);
    {
        Vec3 hs = cross(P.headDir, up);
        P.headSide = length(hs) > 1e-6 ? normalize(hs) : P.ringS[n - 1];
        P.headUp = cross(P.headSide, P.headDir);
    }
    P.headLen = b.headLen * k; P.headW = b.headW * k; P.headH = b.headH * k;
    if (onGround) {   // a muzzle (a grazing one, a serpent's going down a slope) stops on the ground
        const Vec3 tip = P.head0 + P.headDir * P.headLen;
        const double g = groundAt(tip) + 0.3 * P.headH;
        if (tip.y < g) { P.head0.y += g - tip.y; }
    }
    // the tail: back from the rear ring, each kind its own curve, wagging
    P.tailN = 0;
    if (b.tail != TAIL_NONE && b.tailLen > 0.02 * b.size) {
        const int nt = b.tail == TAIL_CURL ? 6 : (b.tailLen > b.size ? 4 : 3);
        P.tailN = nt;
        double a0 = 0.15, a1 = -0.9;
        switch (b.tail) {
            case TAIL_THICK: a0 = 0.05; a1 = -0.3; break;
            case TAIL_CLUB: a0 = 0.1; a1 = -0.15; break;
            case TAIL_FAN: a0 = 0.35; a1 = 0.15; break;
            case TAIL_FIN: a0 = 0; a1 = 0; break;
            case TAIL_STINGER: a0 = 0.8; a1 = 2.8; break;
            case TAIL_CURL: a0 = 0.5; a1 = 4.4; break;
            default: break;
        }
        if (in.aloft) { a0 = 0.02; a1 = -0.1; }
        if (b.plan == PLAN_SERPENT || (R.swims && b.variant == SWIM_EEL)) { a0 = 0; a1 = 0; }
        const double wag = R.swims ? 0.45 * std::sin(in.t * 4.0 + (in.seed % 7)) : std::sin(in.t * 3 + (in.seed % 7)) * (in.moving ? 0.35 : 0.12);
        const Vec3 tb = -P.ringT[0], tu = P.ringU[0], ts = P.ringS[0];
        Vec3 p = P.ringC[0] + tb * (0.2 * P.ringH[0]) + tu * (0.15 * P.ringH[0]);
        if (b.plan == PLAN_SERPENT) p = P.ringC[0];
        const double r0 = std::min(0.5 * b.tailThick * k, 0.85 * P.ringH[0]);
        P.tailP[0] = p; P.tailU[0] = tu; P.tailR[0] = r0;
        for (int j = 1; j <= nt; j++) {
            const double s = (j - 0.5) / nt, a = a0 + (a1 - a0) * s, w = wag * j / nt;
            Vec3 d = tb * std::cos(a) + tu * std::sin(a);
            d = normalize(d * std::cos(w) + ts * std::sin(w));
            p = p + d * (b.tailLen * k / nt);
            if (onGround && (b.plan == PLAN_SERPENT)) p.y = groundAt(p) + r0 * (1 - 0.8 * j / nt);
            else if (onGround) p.y = std::max(p.y, groundAt(p) + 0.3 * r0);   // a tail lies on the ground, never in it
            P.tailP[j] = p;
            P.tailU[j] = normalize(cross(cross(d, tu), d) * -1.0);
            P.tailR[j] = r0 * (b.tail == TAIL_WHIP ? 1 - 0.85 * j / nt : 1 - 0.7 * j / nt);
        }
        if (b.tail == TAIL_CLUB) P.tailR[nt] = r0 * 1.8;
    }
    // the legs
    const int pairs = std::min(b.pairs, 6);
    P.legs = 0; P.joints = R.bones + 1; P.legR = 0.5 * b.legThick * k;
    auto frameAt = [&](double f, Vec3& c, Vec3& s, Vec3& u, Vec3& t, double& w, double& h) {   // the trunk's frame at f along it (metres)
        const double x = clampd(f / (b.length * k) + 0.5, 0, 1) * (n - 1);
        const int i0 = std::min(n - 2, (int)x);
        const double q = x - i0;
        c = P.ringC[i0] + (P.ringC[i0 + 1] - P.ringC[i0]) * q;
        s = normalize(P.ringS[i0] + (P.ringS[i0 + 1] - P.ringS[i0]) * q);
        u = normalize(P.ringU[i0] + (P.ringU[i0 + 1] - P.ringU[i0]) * q);
        t = normalize(P.ringT[i0] + (P.ringT[i0 + 1] - P.ringT[i0]) * q);
        w = P.ringW[i0] + (P.ringW[i0 + 1] - P.ringW[i0]) * q;
        h = P.ringH[i0] + (P.ringH[i0 + 1] - P.ringH[i0]) * q;
    };
    if (R.floats) {   // a floater's strands hang from under its bag and sway
        const int ns = std::min(RIG_LEGS, 2 * pairs);
        const int nj = std::min(RIG_JOINTS - 1, R.bones + 1);
        P.legs = ns; P.joints = nj + 1; P.legR = 0.5 * b.legThick * k;
        const Vec3 cm = P.ringC[n / 2];
        for (int q = 0; q < ns; q++) {
            const double a = TAU * q / ns;
            Vec3 p = cm + P.ringS[n / 2] * (std::cos(a) * P.ringW[n / 2] * 0.55) + P.ringT[n / 2] * (std::sin(a) * b.length * k * 0.22) - up * (P.ringH[n / 2] * 0.8);
            P.leg[q][0] = p;
            const double seg = b.leg * k / nj;
            for (int j = 1; j <= nj; j++) {
                const double sw = 0.3 * j / nj;
                p = p - up * seg + (fwd * std::sin(in.t * 0.6 + q * 1.7 + j * 0.4) + side * std::cos(in.t * 0.5 + q * 2.3 + j * 0.5)) * (seg * sw);
                P.leg[q][j] = p;
            }
        }
    } else for (int p = 0; p < pairs; p++) {
        for (int s = 0; s < 2; s++) {
            const double sg = s ? 1.0 : -1.0;
            Vec3 c, S, U, T; double w, h;
            frameAt(R.legF[p] * k, c, S, U, T, w, h);
            const bool sprawl = b.stance == STANCE_SPRAWL;
            const Vec3 hip = c + S * (sg * w * (sprawl ? 0.92 : 0.55)) - U * (h * (sprawl ? 0.15 : 0.4));
            const double L = R.legLen[p] * k;
            const double l0 = L * R.boneShare[0], l1 = L * R.boneShare[1], l2 = L * R.boneShare[2], l3 = L * R.boneShare[3];
            Vec3* J = P.leg[P.legs];
            for (int j = 0; j < RIG_JOINTS; j++) J[j] = hip;
            Vec3 foot;
            if (b.foot == FOOT_FLIPPER) {   // a swimmer's flipper, paddling
                const double pad = std::sin(in.t * (b.variant == SWIM_LONGNECK ? 1.3 : 2.2) + p * PI + s);
                foot = hip + S * (sg * L * 0.7) - T * (L * 0.5) + U * (pad * L * 0.35);
            } else if (in.aloft) {   // a flyer's tucked back under it, a glider's spread to its skin's corners
                foot = b.wings == WINGS_SKIN ? hip + S * (sg * L * 0.85) + T * ((p == 0 ? 0.35 : -0.35) * L) : hip - U * (L * 0.3) - T * (L * 0.6);
            } else if (in.afloat) {
                foot = hip - U * (L * 0.5) - T * (L * 0.5);
            } else {
                const double off = legOffset(in.gait, p, pairs, s);
                double swing = in.moving ? std::sin(TAU * (phase + off)) : 0, liftA = in.moving ? std::max(0.0, std::cos(TAU * (phase + off))) : 0;
                if (in.gait == GAIT_HOP && in.moving) { swing = -0.3 + 0.6 * std::max(0.0, std::sin(TAUp)); liftA = 0; }
                const double stride = R.legReach[p] * k * (in.running ? 1.1 : 0.6);
                if (in.lift > 0.5) foot = hip + S * (sg * R.legSpread[p] * k * 0.7) - fwd * (L * 0.12);   // folded under it
                else foot = hip + S * (sg * R.legSpread[p] * k) + fwd * (swing * stride);
                foot.y = groundAt(foot) + liftA * 0.25 * R.legReach[p] * k;
            }
            Vec3 bend = b.kneeBack ? -T : T;
            if (sprawl) bend = normalize(up * 0.85 + S * (sg * 0.5));
            if (in.lift > 0.5 && !sprawl) bend = normalize(up * 0.8 + bend * 0.6);   // lying: the knees fold up beside it
            if (in.aloft || b.foot == FOOT_FLIPPER) bend = -T;
            if (R.bones == 2) {
                J[1] = kneeOf(hip, foot, l0, l1, bend);
                J[2] = foot;
                if (length(foot - hip) > l0 + l1) J[2] = J[1] + normalize(foot - J[1]) * l1;
            } else {
                Vec3 ball = foot, toe = foot;
                const Vec3 out = sprawl ? S * sg : fwd;
                if (R.bones == 4) toe = ball + out * l3;
                const Vec3 ankle = sprawl ? ball + (up * 0.95 + S * (sg * 0.3)) * l2 : ball + (up * 0.94 - fwd * 0.34) * l2;
                J[1] = kneeOf(hip, ankle, l0, l1, bend);
                J[2] = length(ankle - hip) > l0 + l1 ? J[1] + normalize(ankle - J[1]) * l1 : ankle;
                J[3] = ball;
                J[4] = toe;
                if (R.bones == 3) J[4] = ball;
            }
            if (onGround) for (int j = 1; j < RIG_JOINTS; j++) J[j].y = std::max(J[j].y, groundAt(J[j]) + 0.2 * P.legR);   // no joint in the ground
            P.legs++;
        }
    }
    // the arms: off the ground at the front, swinging against the legs
    P.arms = b.arms > 0;
    if (P.arms) {
        for (int s = 0; s < 2; s++) {
            const double sg = s ? 1.0 : -1.0;
            const Vec3 sh = P.ringC[n - 1] - tFront * (0.25 * b.length * k / (n - 1)) + P.ringS[n - 1] * (sg * P.ringW[n - 1] * 0.8) - uFront * (0.15 * hFront);
            const double A = b.arms * k, sw = in.moving ? 0.35 * std::sin(TAU * (phase + 0.5 * s)) : 0.1;
            const Vec3 hand = sh + flatFwd * (A * (0.35 + sw)) - up * (A * 0.62);
            P.arm[s][0] = sh; P.arm[s][1] = kneeOf(sh, hand, A * 0.5, A * 0.5, -flatFwd); P.arm[s][2] = hand;
            if (onGround) for (int j = 1; j < 3; j++) P.arm[s][j].y = std::max(P.arm[s][j].y, groundAt(P.arm[s][j]) + 0.02 * A);   // lying, the hands on the ground
        }
    }
    // the wings
    P.wings = 0; P.wingPts = 0;
    if (b.wings != WINGS_NONE && b.span > 0) {
        const double half = 0.5 * b.span * k;
        Vec3 c, S, U, T; double w, h;
        frameAt(0.1 * b.length * k, c, S, U, T, w, h);
        if (b.wings == WINGS_SKIN && R.swims) {   // a ray's wings: its flanks out to their tips, flapping slowly
            P.wingPts = 4;
            const double fl = std::sin(in.t * 1.6) * 0.35;
            for (int s = 0; s < 2; s++) {
                const double sg = s ? 1.0 : -1.0;
                Vec3 cf, cr, Sx, Ux, Tx; double wf, hf, wr, hr;
                frameAt(0.38 * b.length * k, cf, Sx, Ux, Tx, wf, hf);
                frameAt(-0.4 * b.length * k, cr, Sx, Ux, Tx, wr, hr);
                Vec3* Wp = P.wing[s];
                Wp[0] = cf + S * (sg * wf * 0.7);
                Wp[1] = c + S * (sg * half) + U * (std::sin(fl) * half) + T * (-0.05 * half);
                Wp[2] = cr + S * (sg * wr * 0.7);
                Wp[3] = c + S * (sg * w * 0.9);
            }
            P.wings = 2;
        } else if (b.wings == WINGS_SKIN) {   // a glider's skin between its legs: spread aloft
            if (in.aloft && P.legs >= 4) {
                P.wingPts = 4;
                for (int s = 0; s < 2; s++) {
                    Vec3* Wp = P.wing[s];
                    const Vec3* fl = P.leg[s];            // the front pair's leg on this side
                    const Vec3* rl = P.leg[2 + s];        // the rear pair's
                    Wp[0] = fl[0]; Wp[1] = fl[P.joints - 1]; Wp[2] = rl[P.joints - 1]; Wp[3] = rl[0];
                }
                P.wings = 2;
            }
        } else if (b.wings == WINGS_INSECT) {   // two pairs, fore and hind, beating out of step
            P.wingPts = 4;
            for (int q = 0; q < 4; q++) {
                const int s = q & 1, fore = q < 2;
                const double sg = s ? 1.0 : -1.0;
                const double th = in.folded ? -0.2 : (fore ? 1 : -1) * in.flap * 0.9;
                const Vec3 root = c + T * ((fore ? 0.12 : -0.05) * b.length * k) + U * (h * 0.6) + S * (sg * w * 0.4);
                const Vec3 out = in.folded ? normalize(-T * 0.95 + S * (sg * 0.25)) : normalize(S * (sg * std::cos(th)) + U * std::sin(th) - T * 0.25);
                const double len = half * (fore ? 1.0 : 0.85), chord = half * 0.24;
                Vec3* Wp = P.wing[q];
                Wp[0] = root; Wp[1] = root + out * len; Wp[2] = root + out * (len * 0.85) - T * chord; Wp[3] = root - T * (chord * 0.5);
            }
            P.wings = 4;
        } else {   // feathers or a membrane: an arm and a hand, flapping about the body's axis; folded along the flanks when perched
            P.wingPts = 6;
            const double chord = b.span * k * (b.wings == WINGS_MEMBRANE ? 0.17 : 0.13);
            const double th = in.folded ? -0.15 : in.flap * 0.85, th2 = in.folded ? -0.25 : in.flap * 1.15;   // folded: down along the flanks
            const double span = in.folded ? 0.35 : 1.0;
            for (int s = 0; s < 2; s++) {
                const double sg = s ? 1.0 : -1.0;
                const Vec3 out = S * sg;
                const Vec3 root = c + out * (w * 0.7) + U * (h * 0.5);
                const Vec3 elbow = root + (out * std::cos(th) + U * std::sin(th)) * (half * 0.45 * span) - T * (half * 0.04);
                const Vec3 tip = elbow + (out * std::cos(th2) + U * std::sin(th2)) * (half * 0.55 * span) - T * (half * (in.folded ? 0.5 : 0.15));
                Vec3* Wp = P.wing[s];
                Wp[0] = root; Wp[1] = elbow; Wp[2] = tip;
                Wp[3] = tip - T * (chord * 0.35);
                Wp[4] = elbow - T * chord;
                Wp[5] = b.wings == WINGS_MEMBRANE ? P.ringC[0] + P.ringS[0] * (sg * P.ringW[0] * 0.8) : root - T * (chord * 0.9);
            }
            P.wings = 2;
        }
        if (onGround)   // perched: the wings' ends above the ground they stand on
            for (int w = 0; w < P.wings; w++) for (int j = 0; j < P.wingPts; j++) P.wing[w][j].y = std::max(P.wing[w][j].y, groundAt(P.wing[w][j]) + 0.01);
    }
}

bool poseWithin(const AnimalPose& P, const Vec3& m, double box, double floorY) {
    bool ok = true;
    auto in = [&](const Vec3& p) {
        if (!(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z))) { ok = false; return; }
        if (std::fabs(p.x - m.x) > box || std::fabs(p.y - m.y) > box || std::fabs(p.z - m.z) > box || p.y < floorY) ok = false;
    };
    for (int i = 0; i < P.rings; i++) in(P.ringC[i]);
    for (int j = 0; j <= P.neckN; j++) if (P.neckN) in(P.neckP[j]);
    in(P.head0); in(P.head0 + P.headDir * P.headLen);
    for (int j = 0; j <= P.tailN; j++) if (P.tailN) in(P.tailP[j]);
    for (int l = 0; l < P.legs; l++) for (int j = 0; j < P.joints; j++) in(P.leg[l][j]);
    if (P.arms) for (int s = 0; s < 2; s++) for (int j = 0; j < 3; j++) in(P.arm[s][j]);
    for (int w = 0; w < P.wings; w++) for (int j = 0; j < P.wingPts; j++) in(P.wing[w][j]);
    return ok;
}

int animalDetail(double pixels, double fbScale) {
    const double px1 = pixels / std::max(1.0, fbScale);   // as at 1x, with some more at the finer scales
    const double k = 1 + 0.35 * (fbScale - 1);
    if (px1 * k < 7) return 0;
    if (px1 * k < 24) return 1;
    return 2;
}

namespace {
// the shapes of one animal: its faces lit and coated, culled where they face away
struct Emitter {
    const AnimalRig& R; const AnimalPose& P; const AnimalView& V; ShapeSink& out; int detail;
    const AnimalBody& b;
    Emitter(const AnimalRig& r, const AnimalPose& p, const AnimalView& v, ShapeSink& o, int d) : R(r), P(p), V(v), out(o), detail(d), b(r.b) {}

    double shadeOf(double light, double tone) const { const double s = light * tone; return s + (63 - s) * V.fog; }
    RVert vert(const Vec3& w, double shade) const { const Vec3 v = V.view(w); RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = clampd(shade, 0, 63); return q; }
    int thick(double metres) const { return clampi((int)std::lround(metres * V.pxPerM / V.fbScale), 1, 4); }
    int hideBank() const { return V.hide[b.bank & 1]; }
    int coatBank() const { return V.hide[b.coatBank & 1]; }
    // a face's bank and tone by the coat: part 0 the trunk, 1 the neck, 2 the head, 3 the tail; i its segment, x along the trunk, nu the up share of its normal
    void coat(int part, int i, double x, double nu, int k, int& bank, double& tone) const {
        bank = hideBank(); tone = b.tone;
        bool c = false;
        switch (b.coat) {
            case COAT_STRIPES: c = part != 2 && (i % 2) == 1; break;
            case COAT_BANDS: c = part == 0 && (i % 3) == 1; break;
            case COAT_SADDLE: c = part == 0 && x > 0.28 && x < 0.72 && nu > 0.2; break;
            case COAT_BACKSTRIPE: c = part != 2 && nu > 0.8; break;
            case COAT_COUNTERSHADE: c = nu > 0.35; if (nu < -0.35) tone = std::min(1.0, tone * 1.22); break;
            case COAT_MOTTLED: c = hash01(mix64(b.seed + part * 977 + i * 131 + k * 17)) < 0.3; break;
            default: break;
        }
        if (b.hide == HIDE_WET) tone = std::min(1.0, tone * 1.08);
        if (c) { bank = coatBank(); tone = b.coatTone; }
    }
    // a convex polygon (world corners) facing n: left out facing away unless two-sided, its shade by the light at its middle (`light` <= 0: given)
    void polygon(const Vec3* w, int m, Vec3 n, int bank, double tone, bool twoSided) const {
        Vec3 c(0, 0, 0);
        for (int i = 0; i < m; i++) c += w[i];
        c = c / m;
        if (dot(n, c - V.eye) > 0) { if (!twoSided) return; n = -n; }
        const double s = shadeOf(V.light(c, n), tone);
        RVert q[8];
        for (int i = 0; i < m && i < 8; i++) q[i] = vert(w[i], s);
        RasterParams rp; rp.bank = bank;
        out.polygon(q, std::min(m, 8), rp);
    }
    void line(const Vec3& a, const Vec3& c, int bank, double tone, int th) const {
        Vec3 n = V.eye - (a + c) * 0.5;
        n = normalize(n);
        const double s = shadeOf(V.light((a + c) * 0.5, normalize(n + Vec3(0, 0.6, 0))), tone);
        out.line(vert(a, s), vert(c, s), bank, true, th);
    }
    void point(const Vec3& p, int bank, double shade, int size) const { out.point(vert(p, shade), bank, true, size); }

    // a tube along a chain of rings (middles, side and up axes, half widths and heights), `sides` faces round, each vertex lit by its normal
    void tube(const Vec3* C, const Vec3* S, const Vec3* U, const double* W, const double* H, int n, int sides, int part, double x0, double x1, bool capEnd) const {
        if (n < 2) return;
        constexpr int MAXR = 2 * RIG_RINGS + 2;
        Vec3 wv[MAXR][8], nv[MAXR][8];
        RVert vv[MAXR][8];
        double lt[MAXR][8];
        n = std::min(n, MAXR); sides = std::min(sides, 8);
        const double a0 = sides == 4 ? PI / 4 : 0;
        for (int i = 0; i < n; i++)
            for (int k = 0; k < sides; k++) {
                const double a = a0 + TAU * k / sides, ca = std::cos(a), sa = std::sin(a);
                wv[i][k] = C[i] + S[i] * (W[i] * ca) + U[i] * (H[i] * sa);
                nv[i][k] = normalize(S[i] * (ca * std::max(H[i], 1e-6)) + U[i] * (sa * std::max(W[i], 1e-6)));
                lt[i][k] = V.light(wv[i][k], nv[i][k]);
                const Vec3 v = V.view(wv[i][k]);
                vv[i][k].x = v.x; vv[i][k].y = v.y; vv[i][k].z = v.z;
            }
        for (int i = 0; i + 1 < n; i++)
            for (int k = 0; k < sides; k++) {
                const int k1 = (k + 1) % sides;
                const Vec3 fc = (wv[i][k] + wv[i + 1][k] + wv[i + 1][k1] + wv[i][k1]) * 0.25;
                const Vec3 fn = normalize(nv[i][k] + nv[i + 1][k] + nv[i + 1][k1] + nv[i][k1]);
                if (dot(fn, fc - V.eye) > 0.02 * length(fc - V.eye)) continue;
                int bank; double tone;
                coat(part, i, x0 + (x1 - x0) * (i + 0.5) / std::max(1, n - 1), fn.y, k, bank, tone);
                RVert q[4] = {vv[i][k], vv[i + 1][k], vv[i + 1][k1], vv[i][k1]};
                q[0].shade = clampd(shadeOf(lt[i][k], tone), 0, 63); q[1].shade = clampd(shadeOf(lt[i + 1][k], tone), 0, 63);
                q[2].shade = clampd(shadeOf(lt[i + 1][k1], tone), 0, 63); q[3].shade = clampd(shadeOf(lt[i][k1], tone), 0, 63);
                RasterParams rp; rp.bank = bank;
                out.polygon(q, 4, rp);
            }
        if (capEnd && n >= 2) {   // the last ring closed
            Vec3 w[8];
            for (int k = 0; k < sides; k++) w[k] = wv[n - 1][k];
            int bank; double tone;
            coat(part, n - 1, x1, 0, 0, bank, tone);
            polygon(w, sides, normalize(C[n - 1] - C[n - 2]), bank, tone, false);
        }
    }

    // the trunk's rings rounded off at both ends (a ring a third in, beyond each end ring) and, near, each segment halved on a smooth curve
    void trunk() const {
        const int n = P.rings;
        if (detail == 0) {
            const Vec3 a = P.ringC[0], c = P.ringC[n - 1];
            line(a, c, hideBank(), b.tone, thick(2 * P.ringH[n / 2]));
            return;
        }
        const int sides = detail == 2 ? 8 : (P.ringH[n / 2] * 2 * V.pxPerM > 30 ? 6 : 4);
        constexpr int MAXR = 2 * RIG_RINGS + 2;
        Vec3 C[MAXR], S[MAXR], U[MAXR]; double W[MAXR], H[MAXR];
        int m = 0;
        auto put = [&](const Vec3& c, const Vec3& sd, const Vec3& u, double w, double h) { if (m < MAXR) { C[m] = c; S[m] = sd; U[m] = u; W[m] = w; H[m] = h; m++; } };
        const int step = detail == 1 && n > 7 ? 2 : 1;   // a long trunk's every other ring
        put(P.ringC[0] - P.ringT[0] * (0.45 * P.ringH[0]), P.ringS[0], P.ringU[0], 0.55 * P.ringW[0], 0.55 * P.ringH[0]);
        for (int i = 0; i < n; i += step) {
            put(P.ringC[i], P.ringS[i], P.ringU[i], P.ringW[i], P.ringH[i]);
            if (detail == 2 && i + 1 < n) {   // halfway on the curve through the rings round it (Catmull-Rom), the radii between
                const Vec3 p0 = P.ringC[std::max(0, i - 1)], p1 = P.ringC[i], p2 = P.ringC[i + 1], p3 = P.ringC[std::min(n - 1, i + 2)];
                const Vec3 mid = (p1 + p2) * 0.5625 - (p0 + p3) * 0.0625;
                put(mid, normalize(P.ringS[i] + P.ringS[i + 1]), normalize(P.ringU[i] + P.ringU[i + 1]), 0.5 * (P.ringW[i] + P.ringW[i + 1]) * 1.02, 0.5 * (P.ringH[i] + P.ringH[i + 1]) * 1.02);
            }
        }
        if ((n - 1) % step) put(P.ringC[n - 1], P.ringS[n - 1], P.ringU[n - 1], P.ringW[n - 1], P.ringH[n - 1]);
        put(P.ringC[n - 1] + P.ringT[n - 1] * (0.4 * P.ringH[n - 1]), P.ringS[n - 1], P.ringU[n - 1], 0.6 * P.ringW[n - 1], 0.6 * P.ringH[n - 1]);
        tube(C, S, U, W, H, m, sides, 0, -0.08, 1.08, true);
        // the rear closed
        Vec3 w[8];
        const double a0 = sides == 4 ? PI / 4 : 0;
        for (int k = 0; k < sides; k++) { const double a = a0 + TAU * (sides - 1 - k) / sides; w[k] = C[0] + S[0] * (W[0] * std::cos(a)) + U[0] * (H[0] * std::sin(a)); }
        int bank; double tone; coat(0, 0, 0, 0, 0, bank, tone);
        polygon(w, sides, -P.ringT[0], bank, tone, false);
    }

    void neck() const {
        if (P.neckN == 0) return;
        const int n = P.neckN + 1;
        if (detail == 0) { for (int j = 0; j < P.neckN; j++) line(P.neckP[j], P.neckP[j + 1], hideBank(), b.tone, thick(2 * P.neckR[j])); return; }
        Vec3 S[RIG_CHAIN + 1], U[RIG_CHAIN + 1];
        for (int j = 0; j < n; j++) {
            const Vec3 t = normalize(P.neckP[std::min(n - 1, j + 1)] - P.neckP[std::max(0, j - 1)]);
            Vec3 s = cross(t, P.up);
            s = length(s) > 1e-6 ? normalize(s) : P.side;
            S[j] = s; U[j] = cross(s, t);
        }
        double R[RIG_CHAIN + 1];
        for (int j = 0; j < n; j++) R[j] = P.neckR[j];
        R[0] = std::max(R[0], 0.75 * std::min(P.ringW[P.rings - 1], P.ringH[P.rings - 1]));
        if (n > 2) R[1] = std::max(R[1], 0.5 * (R[0] + R[2]));
        tube(P.neckP, S, U, R, R, n, detail == 2 ? 6 : 4, 1, 1, 1, false);
    }

    void head() const {
        const Vec3 h0 = P.head0, d = P.headDir, s = P.headSide, u = P.headUp;
        const double L = P.headLen, W = 0.5 * P.headW, H = 0.5 * P.headH;
        const Vec3 tip = h0 + d * L;
        if (detail == 0) { line(h0, tip, hideBank(), b.tone, thick(2 * H)); return; }
        // the skull: three rings to the muzzle, a beak's and a trunk's apart
        double tipW = 0.55, tipH = 0.55;
        switch (b.head) {
            case HEAD_SNOUT: tipW = 0.35; tipH = 0.4; break;
            case HEAD_BLUNT: tipW = 0.8; tipH = 0.75; break;
            case HEAD_BEAK: tipW = 0.7; tipH = 0.7; break;
            case HEAD_TRUNK: tipW = 0.75; tipH = 0.75; break;
            case HEAD_MANDIBLES: tipW = 0.7; tipH = 0.6; break;
            case HEAD_SHOVEL: tipW = 1.15; tipH = 0.35; break;
            default: break;
        }
        const double skull = b.head == HEAD_BEAK || b.head == HEAD_TRUNK ? 0.7 : 1.0;
        Vec3 C[3] = {h0 - d * (0.1 * L), h0 + d * (0.45 * L * skull), h0 + d * (L * skull)};
        Vec3 S[3] = {s, s, s}, U[3] = {u, u, u};
        double Wr[3] = {W * 0.85, W, W * tipW}, Hr[3] = {H * 0.85, H, H * tipH};
        tube(C, S, U, Wr, Hr, 3, detail == 2 ? 6 : 4, 2, 1, 1, true);
        const int boneBank = V.bone, darkBank = V.dark;
        const Vec3 muzzle = h0 + d * (L * skull);
        if (b.head == HEAD_BEAK) {   // a beak: a cone off the muzzle, in bone or dark horn
            const Vec3 bt = muzzle + d * (L * 0.55) - u * (H * 0.25);
            const Vec3 w0 = muzzle + s * (W * tipW * 0.8), w1 = muzzle - s * (W * tipW * 0.8), w2 = muzzle + u * (H * tipH * 0.7), w3 = muzzle - u * (H * tipH * 0.7);
            const int bk = (b.seed & 1) ? boneBank : darkBank;
            const Vec3 f1[3] = {w2, w0, bt}, f2[3] = {w1, w2, bt}, f3[3] = {w0, w3, bt}, f4[3] = {w3, w1, bt};
            polygon(f1, 3, normalize(cross(w0 - w2, bt - w2)), bk, 0.8, true);
            polygon(f2, 3, normalize(cross(w2 - w1, bt - w1)), bk, 0.8, true);
            polygon(f3, 3, normalize(cross(w3 - w0, bt - w0)), bk, 0.7, true);
            polygon(f4, 3, normalize(cross(w1 - w3, bt - w3)), bk, 0.7, true);
        } else if (b.head == HEAD_TRUNK) {   // a trunk: down off the muzzle and curling forward at its end
            Vec3 p0 = muzzle, p1 = p0 + d * (L * 0.25) - P.up * (L * 0.6), p2 = p1 - P.up * (L * 0.55) + d * (L * 0.1), p3 = p2 + d * (L * 0.2) - P.up * (L * 0.1);
            const double r = std::max(0.25 * W * tipW, 0.01);
            if (detail == 2) {
                Vec3 Ct[4] = {p0, p1, p2, p3}, St[4] = {s, s, s, s}, Ut[4];
                for (int j = 0; j < 4; j++) { const Vec3 t = normalize(Ct[std::min(3, j + 1)] - Ct[std::max(0, j - 1)]); Ut[j] = normalize(cross(s, t)); }
                double R4[4] = {r * 1.2, r, r * 0.85, r * 0.7};
                tube(Ct, St, Ut, R4, R4, 4, 4, 2, 1, 1, false);
            } else { line(p0, p1, hideBank(), b.tone, thick(2 * r)); line(p1, p2, hideBank(), b.tone, thick(1.7 * r)); line(p2, p3, hideBank(), b.tone, thick(1.4 * r)); }
        } else if (b.head == HEAD_MANDIBLES && detail == 2) {   // two pincers closing in front
            for (int e = -1; e <= 1; e += 2) {
                const Vec3 m0 = muzzle + s * (e * W * tipW * 0.7), m1 = m0 + d * (L * 0.45) + s * (e * W * 0.35), m2 = m1 + d * (L * 0.25) - s * (e * W * 0.55);
                line(m0, m1, darkBank, 0.6, thick(0.4 * H)); line(m1, m2, darkBank, 0.6, std::max(1, thick(0.3 * H)));
            }
        }
        if (detail < 2) return;
        // the eyes: glints in a pair, a row, a ring or on stalks
        if (b.eyes > 0) {
            const int ne = std::min(b.eyes, 8);
            const int sz = W * 2 * V.pxPerM > 40 ? 2 : 1;
            for (int e = 0; e < ne; e++) {
                Vec3 p;
                switch (b.eyeLayout) {
                    case EYES_ROW: { const double q = ne > 1 ? (double)e / (ne - 1) * 2 - 1 : 0; p = h0 + d * (L * 0.6) + s * (q * W * 0.8) + u * (H * 0.75); break; }
                    case EYES_RING: { const double a = TAU * e / ne; p = h0 + d * (L * 0.5) + s * (std::cos(a) * W * 1.02) + u * (std::sin(a) * H * 1.02); break; }
                    case EYES_STALKS: {
                        const double q = ne > 1 ? (double)e / (ne - 1) * 2 - 1 : 0;
                        const Vec3 b0 = h0 + d * (L * 0.5) + s * (q * W * 0.6) + u * (H * 0.8);
                        p = b0 + u * (L * 0.9) + d * (L * 0.3) + s * (q * W * 0.4);
                        line(b0, p, hideBank(), b.tone, 1);
                        break;
                    }
                    default: {
                        const int pr = e / 2;
                        const double sd = (e & 1) ? 1 : -1;
                        p = h0 + d * (L * (0.55 - 0.18 * pr)) + s * (sd * W * 0.92) + u * (H * (0.35 + 0.22 * pr));
                        break;
                    }
                }
                point(p, V.glint, 50 + 8 * (1 - V.fog), sz);
            }
        }
        ornament();
    }

    void ornament() const {
        const Vec3 h0 = P.head0, d = P.headDir, s = P.headSide, u = P.headUp;
        const double L = P.headLen, W = 0.5 * P.headW, H = 0.5 * P.headH, O = b.ornamentLen * P.scale;
        const int bone = V.bone;
        const int th = thick(0.18 * O);
        switch (b.ornament) {
            case ORN_JAWS: {   // a heavy lower jaw, open a little
                const Vec3 j0 = h0 - u * (H * 0.8), j1 = h0 + d * (L * 1.05) - u * (H * 1.1);
                line(j0, j1, hideBank(), b.tone * 0.8, thick(0.6 * H));
                break;
            }
            case ORN_TUSKS:
                for (int e = -1; e <= 1; e += 2) {
                    const Vec3 t0 = h0 + d * (L * 0.8) + s * (e * W * 0.5) - u * (H * 0.4), t1 = t0 + d * (O * 0.5) - P.up * (O * 0.45), t2 = t1 + d * (O * 0.35) + P.up * (O * 0.3);
                    line(t0, t1, bone, 0.85, th); line(t1, t2, bone, 0.85, std::max(1, th - 1));
                }
                break;
            case ORN_HORNS: case ORN_CURVED: case ORN_SPIRAL:
                for (int e = -1; e <= 1; e += 2) {
                    const Vec3 r0 = h0 + d * (L * 0.2) + s * (e * W * 0.55) + u * (H * 0.8);
                    if (b.ornament == ORN_HORNS) { line(r0, r0 + P.up * (O * 0.8) - d * (O * 0.35) + s * (e * O * 0.2), bone, 0.85, th); continue; }
                    if (b.ornament == ORN_CURVED) {   // up, back and down round to the front
                        const Vec3 c1 = r0 + s * (e * O * 0.35) + P.up * (O * 0.35) - d * (O * 0.2), c2 = c1 + s * (e * O * 0.25) - d * (O * 0.35) - P.up * (O * 0.15),
                                   c3 = c2 - P.up * (O * 0.35) + d * (O * 0.25);
                        line(r0, c1, bone, 0.85, th); line(c1, c2, bone, 0.85, th); line(c2, c3, bone, 0.85, std::max(1, th - 1));
                        continue;
                    }
                    Vec3 p = r0;   // a spiral: twisting up and out
                    for (int j = 1; j <= 4; j++) {
                        const Vec3 q = r0 + (P.up * 0.8 + s * (e * 0.45) - d * 0.25) * (O * j / 4) + (s * std::cos(j * 2.1) + d * std::sin(j * 2.1)) * (O * 0.08);
                        line(p, q, bone, 0.85, std::max(1, th - (j > 2)));
                        p = q;
                    }
                }
                break;
            case ORN_ANTLERS:
                for (int e = -1; e <= 1; e += 2) {
                    const Vec3 r0 = h0 + d * (L * 0.15) + s * (e * W * 0.5) + u * (H * 0.85);
                    const Vec3 beam = r0 + P.up * (O * 0.9) + s * (e * O * 0.45) - d * (O * 0.3);
                    line(r0, beam, bone, 0.8, th);
                    for (int j = 1; j <= 3; j++) {
                        const Vec3 at = r0 + (beam - r0) * (0.3 + 0.22 * j);
                        line(at, at + P.up * (O * 0.3) + d * (O * 0.18) - s * (e * O * 0.05), bone, 0.8, 1);
                    }
                }
                break;
            case ORN_NOSE_HORN: {
                const Vec3 n0 = h0 + d * (L * 0.8) + u * (H * 0.6);
                line(n0, n0 + P.up * (O * 0.55) + d * (O * 0.3), bone, 0.85, std::max(1, thick(0.25 * O)));
                break;
            }
            case ORN_EARS:
                for (int e = -1; e <= 1; e += 2) {
                    const Vec3 e0 = h0 + d * (L * 0.05) + s * (e * W * 0.6) + u * (H * 0.6), e1 = e0 + d * (O * 0.18), e2 = e0 + P.up * (O * 0.45) + s * (e * O * 0.2) - d * (O * 0.08);
                    const Vec3 tri[3] = {e0, e1, e2};
                    polygon(tri, 3, normalize(d * -1.0 + s * (e * 0.2)), hideBank(), b.tone * 0.9, true);
                }
                break;
            case ORN_FRILL: {   // a fan round the back of the skull
                Vec3 f[7];
                const Vec3 c = h0 + d * (L * 0.1);
                f[0] = c;
                for (int j = 0; j < 6; j++) { const double a = -1.7 + 3.4 * j / 5; f[j + 1] = c + (u * std::cos(a) + s * std::sin(a)) * (O * 0.75) - d * (O * 0.15); }
                polygon(f, 7, -d, coatBank(), b.coatTone, true);
                break;
            }
            case ORN_FEELERS:
                for (int e = -1; e <= 1; e += 2) {
                    const Vec3 a0 = h0 + d * (L * 0.85) + s * (e * W * 0.4) + u * (H * 0.6), a1 = a0 + d * (O * 0.5) + P.up * (O * 0.45) + s * (e * O * 0.2),
                               a2 = a1 + d * (O * 0.45) + s * (e * O * 0.15) - P.up * (O * 0.05);
                    line(a0, a1, V.dark, 0.7, 1); line(a1, a2, V.dark, 0.7, 1);
                }
                break;
            default: break;
        }
    }

    void tail() const {
        if (P.tailN == 0) return;
        const int n = P.tailN + 1;
        const bool thin = detail == 0 || b.tail == TAIL_WHIP || b.tail == TAIL_CURL || b.tail == TAIL_STINGER || P.tailR[0] * V.pxPerM < 2.5;
        if (thin) {
            for (int j = 0; j + 1 < n; j++) {
                int bank; double tone;
                coat(3, j, 0, 1, 0, bank, tone);
                line(P.tailP[j], P.tailP[j + 1], bank, tone, thick(2 * P.tailR[j]));
            }
        } else {
            Vec3 S[RIG_CHAIN + 1], U[RIG_CHAIN + 1];
            for (int j = 0; j < n; j++) {
                const Vec3 t = normalize(P.tailP[std::min(n - 1, j + 1)] - P.tailP[std::max(0, j - 1)]);
                Vec3 sd = cross(t, P.tailU[j]);
                sd = length(sd) > 1e-6 ? normalize(sd) : P.side;
                S[j] = sd; U[j] = cross(sd, t);
            }
            tube(P.tailP, S, U, P.tailR, P.tailR, n, 4, 3, 0, 0, b.tail == TAIL_CLUB);
        }
        if (detail == 0) return;
        const Vec3 end = P.tailP[n - 1], prev = P.tailP[n - 2];
        const Vec3 t = normalize(end - prev);
        const double big = std::max(b.tailLen * P.scale * 0.4, 0.02);
        if (b.tail == TAIL_FAN) {   // a fan spread flat
            Vec3 s = cross(t, P.up); s = length(s) > 1e-6 ? normalize(s) : P.side;
            Vec3 f[6] = {prev, end + s * (big * 0.8) - t * (big * 0.2), end + s * (big * 0.5) + t * (big * 0.6), end + t * (big * 0.75), end - s * (big * 0.5) + t * (big * 0.6),
                         end - s * (big * 0.8) - t * (big * 0.2)};
            polygon(f, 6, P.up, coatBank(), b.coatTone, true);
        } else if (b.tail == TAIL_FIN) {   // a fin standing up
            const Vec3 u = P.tailU[n - 1];
            Vec3 f[4] = {prev, end + u * (big * 0.9) + t * (big * 0.35), end + t * (big * 0.15), end - u * (big * 0.9) + t * (big * 0.35)};
            polygon(f, 4, normalize(cross(t, u)), hideBank(), b.tone * 0.9, true);
        } else if (b.tail == TAIL_STINGER) {
            line(end, end + t * (big * 0.5) - P.up * (big * 0.2), V.dark, 0.6, std::max(1, thick(0.3 * big)));
        }
    }

    void legs() const {
        if (P.legs == 0) return;
        const bool tubes = detail >= 1 && P.legR * 2 * V.pxPerM >= 2.0 * V.fbScale;
        const int th = thick(2 * P.legR);
        for (int l = 0; l < P.legs; l++) {
            const Vec3* J = P.leg[l];
            const int nj = R.floats ? P.joints : std::min(P.joints, R.bones + 1);
            if (R.floats) {   // strands
                for (int j = 0; j + 1 < nj; j++) line(J[j], J[j + 1], coatBank(), b.coatTone, std::max(1, th - (j > 1)));
                continue;
            }
            if (b.foot == FOOT_FLIPPER) {   // a paddle
                const Vec3 a = J[0], c = J[nj - 1];
                Vec3 t = normalize(c - a), s = cross(t, P.up);
                s = length(s) > 1e-6 ? normalize(s) * (std::max(P.legR * 2.5, 0.02)) : P.side * P.legR;
                const Vec3 f[4] = {a + P.fwd * (P.legR * 1.5), c + P.fwd * (P.legR * 0.8), c - P.fwd * (P.legR * 0.8), a - P.fwd * (P.legR * 1.5)};
                polygon(f, 4, P.up, hideBank(), b.tone * 0.9, true);
                continue;
            }
            if (detail == 0) { line(J[0], J[nj - 1], hideBank(), b.tone * 0.85, 1); continue; }
            const bool sprawl = b.stance == STANCE_SPRAWL, column = b.stance == STANCE_COLUMN;
            for (int j = 0; j + 1 < nj; j++) {
                // a thigh thick at the hip (a sprawler's and an insect's thin), the shank thinner, the foot's bones thin
                const double r0 = P.legR * (j == 0 ? (sprawl ? 1.1 : (column ? 1.15 : 1.7)) : (j == 1 ? 0.95 : 0.6));
                const double r1 = P.legR * (j == 0 ? (column ? 1.05 : 1.0) : (j == 1 ? 0.7 : 0.45));
                if (tubes && j < 2) {
                    const Vec3 t = normalize(J[j + 1] - J[j]);
                    Vec3 s = cross(t, P.fwd);
                    s = length(s) > 1e-6 ? normalize(s) : P.side;
                    const Vec3 u = cross(s, t);
                    const Vec3 C[2] = {J[j] - t * (j == 0 ? r0 * 0.5 : 0.0), J[j + 1] + t * (r1 * 0.3)}, S[2] = {s, s}, U[2] = {u, u};
                    const double Wr[2] = {r0, r1};
                    tube(C, S, U, Wr, Wr, 2, detail == 2 ? 6 : 4, 4, 0, 0, false);
                } else line(J[j], J[j + 1], hideBank(), b.tone * (j ? 0.8 : 0.9), std::max(1, thick(2 * r0)));
            }
            // the feet
            const Vec3 foot = J[nj - 1];
            const double fr = std::max(P.legR * 1.2, 0.01);
            switch (b.foot) {
                case FOOT_HOOF: line(foot, foot + P.fwd * (fr * 1.3), V.dark, 0.5, std::max(1, th)); break;
                case FOOT_PAD: line(foot - P.fwd * (fr * 0.6), foot + P.fwd * (fr * 1.2), hideBank(), b.tone * 0.7, std::max(1, th)); break;
                case FOOT_CLAW:
                    if (detail == 2) for (int c = -1; c <= 1; c++) line(foot, foot + P.fwd * (fr * 1.6) + P.side * (c * fr * 0.7), V.bone, 0.8, 1);
                    break;
                case FOOT_FLAT: {
                    const Vec3 f[4] = {foot + P.fwd * (fr * 1.5) + P.side * fr, foot + P.fwd * (fr * 1.5) - P.side * fr, foot - P.fwd * fr - P.side * fr, foot - P.fwd * fr + P.side * fr};
                    polygon(f, 4, P.up, hideBank(), b.tone * 0.7, true);
                    break;
                }
                default: break;
            }
        }
        if (P.arms) for (int s = 0; s < 2; s++) { line(P.arm[s][0], P.arm[s][1], hideBank(), b.tone * 0.9, th); line(P.arm[s][1], P.arm[s][2], hideBank(), b.tone * 0.85, std::max(1, th - 1)); }
    }

    void wings() const {
        if (P.wings == 0) return;
        for (int w = 0; w < P.wings; w++) {
            const Vec3* Wp = P.wing[w];
            if (detail == 0) { line(Wp[0], Wp[P.wingPts == 6 ? 2 : 1], hideBank(), b.tone, 1); continue; }
            const int bank = b.wings == WINGS_INSECT ? V.glint : (b.wings == WINGS_FEATHERS ? coatBank() : hideBank());
            const double tone = b.wings == WINGS_INSECT ? 0.55 : (b.wings == WINGS_FEATHERS ? b.coatTone : b.tone * 0.85);
            if (P.wingPts == 6) {   // the arm's sheet and the hand's: two convex pieces
                const Vec3 inner[4] = {Wp[0], Wp[1], Wp[4], Wp[5]}, outer[4] = {Wp[1], Wp[2], Wp[3], Wp[4]};
                polygon(inner, 4, normalize(cross(Wp[1] - Wp[0], Wp[5] - Wp[0])), bank, tone, true);
                polygon(outer, 4, normalize(cross(Wp[2] - Wp[1], Wp[4] - Wp[1])), bank, tone, true);
                if (detail == 2 && b.wings == WINGS_MEMBRANE) {   // its fingers
                    line(Wp[1], Wp[2], V.dark, 0.6, 1); line(Wp[1], Wp[3], V.dark, 0.6, 1); line(Wp[1], Wp[4], V.dark, 0.6, 1);
                }
                if (detail == 2 && b.wings == WINGS_FEATHERS) line(Wp[2], Wp[3], V.dark, 0.5, 1);   // the primaries' fingers
            } else {
                polygon(Wp, 4, normalize(cross(Wp[1] - Wp[0], Wp[3] - Wp[0])), bank, tone, true);
            }
        }
    }

    // plates, spines, a sail, a shell or a fin along the back
    void dorsal() const {
        if (detail == 0 || b.dorsal == DORSAL_NONE || b.dorsal == DORSAL_HUMPS) return;
        const int n = P.rings;
        const double H = b.dorsalH * P.scale;
        auto top = [&](int i) { return P.ringC[i] + P.ringU[i] * (P.ringH[i] * 0.95); };
        switch (b.dorsal) {
            case DORSAL_PLATES:
                for (int i = 1; i + 1 < n || (n <= 2 && i < n); i++) {
                    const double x = (double)i / (n - 1), hh = H * (0.5 + 0.5 * std::sin(PI * x));
                    const Vec3 t0 = top(i), a = t0 - P.ringT[i] * (b.length * P.scale / (n - 1) * 0.35), c = t0 + P.ringT[i] * (b.length * P.scale / (n - 1) * 0.35);
                    const Vec3 tri[3] = {a, c, t0 + P.ringU[i] * hh + P.ringS[i] * ((i % 2 ? 1 : -1) * hh * 0.12)};
                    polygon(tri, 3, P.ringS[i], coatBank(), b.coatTone, true);
                }
                break;
            case DORSAL_SPINES:
                for (int i = 0; i < n; i++) {
                    const double x = (double)i / std::max(1, n - 1);
                    const Vec3 t0 = top(i);
                    line(t0, t0 + P.ringU[i] * (H * (0.4 + 0.6 * std::sin(PI * x))) - P.ringT[i] * (H * 0.25), V.bone, 0.8, 1);
                }
                break;
            case DORSAL_SAIL:
                for (int i = 0; i + 1 < n; i++) {
                    const double x0 = (double)i / (n - 1), x1 = (double)(i + 1) / (n - 1);
                    const double h0 = H * std::pow(std::sin(PI * x0), 0.8), h1 = H * std::pow(std::sin(PI * x1), 0.8);
                    const Vec3 q[4] = {top(i), top(i + 1), top(i + 1) + P.ringU[i + 1] * h1, top(i) + P.ringU[i] * h0};
                    polygon(q, 4, P.ringS[i], coatBank(), b.coatTone, true);
                }
                break;
            case DORSAL_FIN: {
                const int i0 = n / 3, i1 = std::max(i0 + 1, (2 * n) / 3);
                const Vec3 q[3] = {top(i0), top(std::min(n - 1, i1)), top(std::min(n - 1, i1)) + P.ringU[i1 < n ? i1 : n - 1] * H - P.ringT[i0] * (H * 0.5)};
                polygon(q, 3, P.ringS[i0], hideBank(), b.tone * 0.85, true);
                break;
            }
            case DORSAL_SHELL: {   // a dome over the middle rings (a slug's: a coil on its back)
                Vec3 C[RIG_RINGS], S[RIG_RINGS], U[RIG_RINGS]; double W[RIG_RINGS], Hh[RIG_RINGS];
                int m = 0;
                if (b.plan == PLAN_SLUG) {   // sunk a third into the back it rides on
                    const double r = 0.5 * H;
                    const Vec3 c0 = top(n / 2) + P.ringT[n / 2] * (-0.1 * b.length * P.scale) - P.ringU[n / 2] * (0.6 * r);
                    for (int j = 0; j < 5; j++) {
                        const double a = -PI / 2 + PI * j / 4;
                        C[m] = c0 + P.ringU[n / 2] * (r + r * std::sin(a)); S[m] = P.ringS[n / 2]; U[m] = -P.ringT[n / 2];
                        W[m] = Hh[m] = std::max(r * std::cos(a), r * 0.05); m++;
                    }
                } else {
                    for (int i = 0; i < n; i++) {
                        const double x = (double)i / (n - 1);
                        if (x < 0.12 || x > 0.88) continue;
                        C[m] = P.ringC[i] + P.ringU[i] * (H * 0.25); S[m] = P.ringS[i]; U[m] = P.ringU[i];
                        W[m] = P.ringW[i] * 1.22; Hh[m] = P.ringH[i] * 1.1 + H * 0.4 * std::sin(PI * x); m++;
                    }
                }
                if (m >= 2) tube(C, S, U, W, Hh, m, detail == 2 ? 6 : 4, 0, 0.12, 0.88, b.plan == PLAN_SLUG);
                break;
            }
            default: break;
        }
    }

    // spots, rosettes, eyespots, warts, quills and long hair over the hide (near)
    void marks() const {
        if (detail < 2) return;
        const int n = P.rings;
        auto onHide = [&](double x, double a, double lift) {
            const double fi = clampd(x, 0, 1) * (n - 1);
            const int i0 = std::min(n - 2, (int)fi);
            const double q = fi - i0;
            const Vec3 c = P.ringC[i0] + (P.ringC[i0 + 1] - P.ringC[i0]) * q, s = P.ringS[i0], u = P.ringU[i0];
            const double w = P.ringW[i0] + (P.ringW[i0 + 1] - P.ringW[i0]) * q, h = P.ringH[i0] + (P.ringH[i0 + 1] - P.ringH[i0]) * q;
            return c + s * (w * lift * std::cos(a)) + u * (h * lift * std::sin(a));
        };
        auto normalAt = [&](double x, double a) { const int i = clampi((int)(x * (n - 1)), 0, n - 1); return normalize(P.ringS[i] * std::cos(a) + P.ringU[i] * std::sin(a)); };
        const int sz = P.ringH[n / 2] * 2 * V.pxPerM > 60 ? 2 : 1;
        if (b.coat == COAT_SPOTS || b.coat == COAT_ROSETTES || b.coat == COAT_EYESPOTS) {
            const int count = b.coat == COAT_EYESPOTS ? 4 : (b.coat == COAT_ROSETTES ? 6 : 12);
            for (int j = 0; j < count; j++) {
                const uint64_t h = mix64(b.seed + 300 + j);
                const double x = 0.12 + 0.76 * hash01(h);
                const double a = (j & 1 ? 0 : PI) + (hash01(mix64(h + 1)) - 0.5) * 1.6;
                const Vec3 p = onHide(x, a, 1.04), nn = normalAt(x, a);
                if (dot(nn, p - V.eye) > 0) continue;
                const double lt = V.light(p, nn);
                if (b.coat == COAT_ROSETTES) {
                    for (int r = 0; r < 4; r++) point(onHide(x + 0.03 * std::cos(r * 1.57), a + 0.18 * std::sin(r * 1.57), 1.05), coatBank(), shadeOf(lt, b.coatTone), 1);
                } else if (b.coat == COAT_EYESPOTS) {
                    point(p, coatBank(), shadeOf(lt, std::min(1.0, b.coatTone * 1.4)), sz + 1);
                    point(onHide(x, a, 1.07), hideBank(), shadeOf(lt, b.tone * 0.3), 1);
                } else point(p, coatBank(), shadeOf(lt, b.coatTone), sz);
            }
        }
        if (b.hide == HIDE_WARTS) {
            for (int j = 0; j < 14; j++) {
                const uint64_t h = mix64(b.seed + 700 + j);
                const double x = 0.05 + 0.9 * hash01(h), a = hash01(mix64(h + 1)) * TAU;
                const Vec3 p = onHide(x, a, 1.03), nn = normalAt(x, a);
                if (dot(nn, p - V.eye) > 0) continue;
                point(p, hideBank(), shadeOf(V.light(p, nn), b.tone * 0.6), 1);
            }
        }
        if (b.hide == HIDE_QUILLS) {
            for (int j = 0; j < 12; j++) {
                const double x = 0.1 + 0.8 * j / 11.0, a = PI / 2 + (j % 3 - 1) * 0.6;
                const Vec3 p = onHide(x, a, 1.0), nn = normalAt(x, a);
                line(p, p + nn * (b.depth * P.scale * 0.35) - P.ringT[clampi((int)(x * (n - 1)), 0, n - 1)] * (b.depth * P.scale * 0.25), coatBank(), b.coatTone, 1);
            }
        }
        if (b.hide == HIDE_HAIR) {   // long hair hanging off the flanks and the belly
            for (int j = 0; j < 10; j++) {
                const double x = 0.08 + 0.84 * j / 9.0, a = (j & 1 ? -0.4 : PI + 0.4);
                const Vec3 p = onHide(x, a, 1.0);
                line(p, p - P.up * (b.depth * P.scale * 0.45), hideBank(), b.tone * 0.8, thick(0.05 * b.depth * P.scale));
            }
        }
        if (b.hide == HIDE_PLATES) {   // the seams between a plated trunk's segments
            for (int i = 1; i + 1 < n; i++) {
                const Vec3 a = onHide((double)i / (n - 1), 0.2, 1.02), c = onHide((double)i / (n - 1), PI - 0.2, 1.02), tp = onHide((double)i / (n - 1), PI / 2, 1.02);
                line(a, tp, V.dark, 0.5, 1); line(tp, c, V.dark, 0.5, 1);
            }
        }
    }
};
}

void emitAnimal(const AnimalRig& R, const AnimalPose& P, const AnimalView& V, int detail, ShapeSink& out) {
    const Emitter E(R, P, V, out, detail);
    E.legs();
    E.trunk();
    E.dorsal();
    E.neck();
    E.head();
    E.tail();
    E.wings();
    E.marks();
}
