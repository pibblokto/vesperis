# B-460: a made crust's forms, and lush forests

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/textures.cpp`: the crust's tile; `surface_terrain.cpp`: `growthTexture`; `flora.cpp`: `forTrees`, the bushes,
`canopyDensityAt`, `lushK`)

## Report

KI-463, found in J-05 (2026-11-21): "A crust's three forms (polygons, ripples, rosettes) are kept but drawn with the one crust texture;
LUSH deepens the plants' colours but grows no more trees." The user listed it with the others to fix (2026-11-22): "Now let's handle all
of these".

## Cause

A line of the land whose cells bind lays crusts, and `looksOf` keeps the crust's form in `MadeLook::variant` (0 polygons, 1 ripples, 2
rosettes). Nothing drew it. The ground's crust is drawn with `MAT_CRUST`'s detail tile, which is built from the material's own hashed
variant, and `MAT_CRUST` has one look: the lichen rosettes. LUSH (a new ozone or an endless rain) deepened the plants' colours through
`madePlantColour`, but nothing on the ground read `MadeWorld::lush`, so a lush forest had as many trees as before.

## Fix

The crust's tile now has three forms: polygons (a dried soil crust cracked into plates, each rim curled up dark), ripples (a rugose
crust's low ridges, bent and broken, with dark pits in the troughs) and the rosettes as before. `growthTexture` hands the tile a made
crust's form, so a crust line's ground is drawn in its own form. A world's own crust keeps the rosettes bit for bit (no form is handed
in). On a world with one crust line's look, the world's own crust takes that form too: one tile a landing.

`lushK` is 1 + 0.6 × `MadeWorld::lush` on a world with plants, and exactly 1 elsewhere. It multiplies the trees in a cell (`forTrees`,
a full forest's too, so a lush forest can hold more than any world's own), the bushes, and the canopy's density up to 1
(`canopyDensityAt`), which the forest floor's darkening, the fallen logs and the undergrowth read. The far ring's average darkening
follows it.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a crust's forms and a lush forest": the crust's tiles for polygons and ripples 66,732 apart
  (summed texel differences), ripples and rosettes 78,279, polygons and rosettes 83,799, a world's own crust and the rosettes 0. A crust
  line's ground on a steppe's forest edge drawn in its form. 1,450 trees round the edge plain, 2,302 under a full LUSH (1.59 times).
  Without the fix: every tile 0 apart, the same 1,450 trees under LUSH.
- `seed looks` draws the crust's and LUSH's rows on the ground; a world nobody seeded is drawn as before (every factor is 1 there).
