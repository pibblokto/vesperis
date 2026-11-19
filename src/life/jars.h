#pragma once
// Z-05 (2026-11-17): a world in a jar (R-425; IDEAS-small-and-spacious.md 1.8: Winogradsky's column, Rundell et al. 2014). A vial's wet silt and
// its water stood in a column and lit from within: the silt settles in its first minutes, and over twenty its living species come up in bands by
// what each lives on: the light-feeders that make oxygen at the top, the purple and green that live without it under them, the iron-eaters'
// rust, the sulphide's white, the deep's grey, the black of the heterotrophs that live without air at the foot; each its pigment's colour where it
// has one, as thick as its crowd. A pure function of the sample and the time since the jar was made
#include "life/samples.h"
#include "core/types.h"
#include <vector>

// a band's tone: its pigment's (1-7: green .. black), else the iron's rust, the sulphur's white, the deep's grey, the foot's black
enum JarTone { JT_RUST = PG_COUNT, JT_SULPHUR, JT_GREY, JT_BLACK, JT_COUNT };
// R-439: a band's place, top to bottom, by what its species lives on: the light's oxygen-makers; the air's edge (the iron's and every other eater
// of a chemical that needs air); the light without air; the sulphide's; the deep (a chemical without air); the foot (the dead, without air).
// The guide's `jarband` lines keep it: append-only
enum JarLayer { JL_OXYGEN = 0, JL_EDGE, JL_DARK_LIGHT, JL_SULPHIDE, JL_DEEP, JL_FOOT, JL_COUNT };
// a band: where it lies in the silt (0 its top .. 1 its foot), its colour and its tone, when it is full (s from the jar's making), its species,
// its place (R-439)
struct JarBand { double top = 0, bottom = 0; RGB col; int tone = JT_GREY; double fullS = 600; int species = -1; int layer = JL_DEEP, substrate = SU_NONE; };
struct JarRecipe {
    RGB water, silt;            // the liquid's colour (tinted by what floats in it), the silt's
    double waterShare = 0.28;   // the water's share of the column, over the silt
    std::vector<JarBand> bands;
};
JarRecipe jarRecipe(const Biota& B, const Sample& s, const Microhabitat& h);
// the column at a time since it was made: the silt still in the water (1 all through it .. 0 settled), each band's strength (0..1)
struct JarNow { double cloud = 1; std::vector<double> strength; };
JarNow jarAt(const JarRecipe& r, double t);
constexpr double JAR_SETTLED_S = 1200;   // twenty minutes: every band in
// R-439: a species' band's place in a jar (-1 none: an eater of what breathes air, a grazer)
int jarLayerOf(const Microbe& m);
// whether a niche's sample makes a jar (wet silt, a mat, soil)
inline bool jarNiche(int niche) { return niche == NI_SILT || niche == NI_MAT || niche == NI_SOIL; }
