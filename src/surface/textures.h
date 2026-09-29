// Material meso textures (M10-05): tileable 128x128 signed-offset tiles, one per material,
// generated per planet seed. One texel is a quarter metre, so a tile repeats every 32 m.
// They are added to the shade in the rasteriser (world-mapped, perspective-correct) on
// top of the micro grain; intensity only, never hue. B-311: every material has two or three
// looks (`mesoVariant`, hashed per planet and material) and a strength of its own, so two
// worlds of the same type do not share one ground; `buildMacroTile` makes the broad tile of
// slow tone changes (a texel is two metres, patches of 15-85 m) that breaks up flat plates.
#pragma once
#include "core/noise.h"
#include "galaxy/planetmap.h"

void buildMesoTile(GrainTexture& g, int material, uint64_t seed);
// true for materials that have a meso tile at all
bool materialHasMesoTile(int material);
// the look of a material on this planet (0..2), and its name for the data sheets
int mesoVariant(int material, uint64_t seed);
const char* mesoVariantName(int material, int variant);
void buildMacroTile(GrainTexture& g, uint64_t seed);
// B-315: the vegetation's tiles, per planet: the leaf tile (lumps of light and shade the size of a leaf cluster's leaves, a
// texel is 8 cm at 12 texels a metre), the leaf edge (signed, the cut of a cluster's ragged rim) and the bark (streaks
// when the trunk maps it 8 texels a metre across and 2 along)
void buildLeafTiles(GrainTexture& leaf, GrainTexture& edge, uint64_t seed);
void buildBarkTile(GrainTexture& g, uint64_t seed);
