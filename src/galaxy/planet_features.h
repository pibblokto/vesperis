#pragma once
// E-07: the planet function's finders (planet_features.cpp): what stands near a point of a world, from the features its ground
// builds, for the surface, the scanner, the globe and the harness: the ember fields, the vents of the geyser basins and the hot
// springs, the steam world's and the chemotroph's vents, the plumes and the spiders, the great shield, the landforms' point
// features, the tiger stripes and their curtains, the great cliff, a giant's storm cells
#include "core/types.h"
#include <cstdint>
#include <vector>
struct BodyGen;
// D-04 (TR_EMBERS on a world with no lava of its own): its ember fields, where its lava lakes are (the globe's spots of light), and the
// fields' strength at a point (km on the sphere), 0 outside
struct EmberField { Vec3 unit; double radiusKm = 0, strength = 0; };
void emberFieldsOf(const BodyGen& g, std::vector<EmberField>& out);
double emberFieldAt(const BodyGen& g, const Vec3& p);
// R-307: the vents of the geyser basins (TR_GEYSERS) near a point: the centres of the sinter mounds (unit vectors) and their
// radii in km, the same features the landform builds, so a surface can put an erupting geyser at each
struct GeyserVent { Vec3 unit; double radiusKm; uint64_t id; };
void geyserVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);
void hotSpringsNear(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);   // Q-02 (TR_HOT_SPRINGS): the springs' vents within 2.5 km, their pools' radii
// P-01: the steam world's vents near a point (EV_STEAM_BURSTS): the centres of the sinter mounds of `groundSteam` (where the mound lies
// in a field of vents on the high ground: a surface finds its sinter under the vent)
void steamVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);
// P-03: the chemotroph's hot springs near a point (EV_SPRINGS): the pools' centres and radii (km), the same features its ground builds; a
// surface keeps the ones whose centre lies on the pool's own ground (MAT_BRINE)
void springVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);
// P-05: the cryovolcanic world's plume vents near a point (EV_PLUMES): the vents on its cap whose dark streaks its ground lays downwind (the
// unit, `radiusKm` the streak's length, the id); the dry ice world's spiders near a point (EV_SPRING_JETS): their centres in the band round
// its caps and their reach (km), where its jets break through the seasonal frost; the prevailing wind at a point (a tangent unit vector:
// the direction the streaks, the fans and the plumes' flags run)
void plumeVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);
void springJets(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);
Vec3 prevailingWind(const BodyGen& g, const Vec3& unit);
Vec3 loneMountainOf(const BodyGen& g);   // P-05: the brine dwarf's lone mountain of ice (its centre, a unit vector)
Vec3 nitrogenBasinOf(const BodyGen& g);  // P-05: the nitrogen world's great plain (the middle of its basin, a unit vector)
// P-06: the strange moons' features. The two-faced moon's apex (the middle of the face it leads with: `BodyGen::apexDir`, or on a body the
// harness staged a hashed point of its equator). The tiger stripes: four rifts in the south polar terrain, each a curve in the pole's
// tangent plane (`off` km across it, bowed by `bow` at its ends, `len` long about `mid`), `stripeDistanceKm` a point's distance from the
// nearest (km; 1e9 away from the south polar terrain); the curtains' vents along them near a point (EV_CURTAINS: a vent every kilometre, its
// strength in `radiusKm`). The patchwork's great cliff: the middle of its foot and the unit direction up across it (false: none)
Vec3 twoFacedApex(const BodyGen& g);
// Q-01: the great shield (TR_GREAT_SHIELD): its summit's middle, the radius of its cliff ring (km), its height over the plain (m), its caldera's
// two to four nested pits (their middles, radii in km and depths in m)
struct GreatShield { Vec3 c; double radKm = 0, heightM = 0; int nPits = 0; Vec3 pit[4]; double pitKm[4] = {0, 0, 0, 0}, pitDepthM[4] = {0, 0, 0, 0}; };
GreatShield greatShieldOf(const BodyGen& g);
// Q-01: a new landform's point features round a point (their middles within `halfKm` of it each way): a great shield's caldera pits, the
// chains' craters, the mud volcanoes, the pingos, the maars (their middles, radii in km, heights or depths in m, an id); the scanner keeps the
// ones its landform stands at (their ground carries its mark: the province, the cold or the land may hold none there)
struct LandFeature { Vec3 unit; double radiusKm = 0, heightM = 0; uint64_t id = 0; };
void landformFeatures(const BodyGen& g, int trait, const Vec3& unit, double halfKm, std::vector<LandFeature>& out);
struct TigerStripes { Vec3 e1, e2; double off[4], bow[4], len[4], mid[4], ph[4]; };
TigerStripes tigerStripesOf(const BodyGen& g);
double stripeYKm(const TigerStripes& T, double R, int k, double along);   // the stripe's line across the plane at `along` km from its middle
Vec3 stripePoint(const TigerStripes& T, double R, int k, double along);   // the point of stripe k `along` km from its middle (a unit vector)
double stripeDistanceKm(const TigerStripes& T, double R, const Vec3& unit, int* which = nullptr, double* along = nullptr);
void curtainVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);
bool greatCliffOf(const BodyGen& g, Vec3& foot, Vec3& up);
// X-04: a giant's storm cell at a point of its globe (body frame, unit): the storm term its branch of `sampleSurface` paints
// (0 where none) and the cell's id (`id`: 0 where none), by which a storm the probe fell into keeps the explorer's name
double giantStormCell(const BodyGen& g, const Vec3& unit, uint64_t* id = nullptr);
