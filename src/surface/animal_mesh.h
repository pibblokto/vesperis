// J-06 (R-448): an animal drawn. Its rig is built once a species from its body (`rigOf`: the rings along the trunk, a leg's bones, the hips'
// height). Each frame poses it (`poseAnimal`: the rings, the neck, the head, the tail, every leg's joints on the ground under it, the arms, the
// wings, a floater's strands) and makes its shapes into a sink (`emitAnimal`) by how big it shows: a few lines far away, a coarse mesh, the
// whole mesh near. Pure: a pose reads only its inputs, so the frames pose their animals on the pool
#pragma once
#include "animal_body.h"
#include "core/raster.h"

constexpr int RIG_RINGS = 13, RIG_LEGS = 12, RIG_JOINTS = 5, RIG_CHAIN = 6;

struct AnimalRig {
    AnimalBody b;
    int rings = 4;
    double ringF[RIG_RINGS] = {}, ringW[RIG_RINGS] = {}, ringH[RIG_RINGS] = {};   // along the trunk from the rear (metres), half widths and heights
    double ringLift[RIG_RINGS] = {};   // a hump raises its rings' middles
    double spineY = 0.5;           // the spine's height over the ground standing (lying: its depth's half)
    int bones = 2;
    double boneShare[4] = {};      // a leg's bones from the hip, shares of its length
    double legF[6] = {}, legReach[6] = {}, legLen[6] = {}, legSpread[6] = {};   // each pair: along the trunk, its hips' height, its length, a sprawl's foot out
    bool flies = false, floats = false, swims = false;
};
AnimalRig rigOf(const AnimalBody& b);

// what a frame poses an animal by
struct AnimalPoseIn {
    Vec3 at;                       // on the ground: the trunk's middle at the ground's height there; aloft or afloat: the body's middle
    double heading = 0;            // radians: forward is (sin, 0, cos)
    double scale = 1;              // this animal's size over its species'
    double phase = 0;              // gait cycles walked
    bool moving = false, running = false;
    int gait = 0;                  // `Gait`
    double lift = 0, headPitch = 0;   // lying down 0..1; the head to the ground 0..1
    double t = 0;
    uint64_t seed = 0;
    // the ground's heights under it: [forward -gx, 0, gx][side -gz, 0, gz] round `at`
    double ground[3][3] = {};
    double gx = 1, gz = 1;
    bool aloft = false;            // a flyer in the air, a floater
    double flap = 0;               // the wings' angle share -1..1
    bool folded = false;           // perched: the wings along the body
    bool afloat = false;           // a swimmer
};

// the pose: every point drawn, local metres
struct AnimalPose {
    Vec3 fwd, up, side;
    int rings = 0;
    Vec3 ringC[RIG_RINGS], ringS[RIG_RINGS], ringU[RIG_RINGS], ringT[RIG_RINGS];   // each ring's middle, its side, up and along axes
    double ringW[RIG_RINGS] = {}, ringH[RIG_RINGS] = {};
    int neckN = 0;
    Vec3 neckP[RIG_CHAIN + 1];
    double neckR[RIG_CHAIN + 1] = {};
    Vec3 head0, headDir, headUp, headSide;
    double headLen = 0, headW = 0, headH = 0;
    int tailN = 0;
    Vec3 tailP[RIG_CHAIN + 1], tailU[RIG_CHAIN + 1];
    double tailR[RIG_CHAIN + 1] = {};
    int legs = 0, joints = 0;      // legs (a floater's strands among them), points each
    Vec3 leg[RIG_LEGS][RIG_JOINTS];
    double legR = 0;
    bool arms = false;
    Vec3 arm[2][3];
    int wings = 0, wingPts = 0;    // wing outlines (an insect's four)
    Vec3 wing[4][6];
    double scale = 1;
};
void poseAnimal(const AnimalRig& R, const AnimalPoseIn& in, AnimalPose& out);
// every point of a pose within `box` metres of `middle`, none under `floorY`, none a NaN (the unit's bounds)
bool poseWithin(const AnimalPose& P, const Vec3& middle, double box, double floorY);

// how an animal is drawn: the surface's view and light, the world's banks, how big it shows
struct AnimalView {
    virtual Vec3 view(const Vec3& p) const = 0;                    // local metres to view space
    virtual double light(const Vec3& p, const Vec3& n) const = 0;  // the shade a white hide takes at p facing n (the sun, the sky, the lamps)
    double fog = 0;                // the fog's share where the animal stands
    Vec3 eye;                      // the camera, local metres
    int hide[2] = {3, 0};          // the world's two hide banks
    int bone = 29, dark = 30, glint = 8;
    double pxPerM = 1;             // framebuffer pixels a metre where the animal stands
    double fbScale = 1;            // framebuffer pixels a 1x pixel (a line's thickness)
protected:
    ~AnimalView() = default;
};
// how much of an animal to draw by its height in framebuffer pixels: 0 lines, 1 a coarse mesh, 2 the whole
int animalDetail(double pixels, double fbScale);
void emitAnimal(const AnimalRig& R, const AnimalPose& P, const AnimalView& V, int detail, ShapeSink& out);
