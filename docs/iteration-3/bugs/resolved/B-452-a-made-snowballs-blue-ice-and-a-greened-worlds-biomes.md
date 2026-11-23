# B-452: a made snowball's blue ice, and a greened world's biomes

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: galaxy (`galaxy/planetmap.cpp`: `deadCoverColour`; `planet_bodygen.cpp`: `matFamilyOnGen`; `planet_felisian.cpp`: the biome
table); surface (`surface_terrain.cpp`: `materialLook`); space (`space_globe.cpp`: the globe's material banks)

## Report

KI-484, found in J-11 (2026-11-22): "A made snowball's sea ice is drawn white like its snow (the frozen felisian's ice is blue in a bank of
its own), and a greened world's biomes stay where they were." The user asked for it to be fixed (2026-11-22, with the other known issues
of the J series).

## Cause

A made kind's dead cover gave the whole snow family the frozen felisian's snow colour, its sea ice with it, and the ice stayed in the
snow's bank: the frozen felisian draws its ice blue only because its kind's row puts ice in the grass bank, and a made snowball is still a
felisian by its row. A greened world moved its biomes only across the ice's edge (J-04's rule).

## Fix

- A made snowball's sea ice takes the frozen felisian's blue (`deadCoverColour`), drawn in the grass family's bank, which holds the blue
  under the frost (no grass is left there): `matFamilyOnGen` on the ground, and the globe's material banks, the maps' ramps reading the
  colour.
- A greened world's biomes follow its made warmth (a cold world greened) or its rain (a dry one), in the biome table alone; its rivers, its
  heights and the alpine belt stay as generated.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a made snowball's ice and a greened world's biomes": a tundra's made snowball and a tundra's
  greened world among `seed climate`'s twenty. Before: its sea ice white, 0 of 648 points' biomes moved beyond the ice's edge. After: the
  ice blue in the grass bank, 75 of 648 points' biomes moved beyond the ice's edge, none of its land's heights.
