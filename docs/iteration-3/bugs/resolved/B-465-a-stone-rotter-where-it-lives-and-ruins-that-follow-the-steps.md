# B-465: a stone rotter where it lives, and ruins that follow the steps

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/overgrowth_ruins.cpp`: `stoneRotOf`, `stoneRotsOf`, `layRuinPieces`, `regrowRuins`; `surface_places.cpp`;
`surface_view.cpp`: `update`); peoples (`buildings.cpp`: `buildingDecay`, `Building::stoneRot`); game (`game.cpp`: `madeTick`)

## Report

KI-480, found in J-10 (2026-11-22): "A stone rotter crumbles every settlement its spread reached alike, whatever its line lives on there,
and a settlement in view keeps its state till the next landing." The user listed it with the others to fix (2026-11-22): "Now let's handle
all of these".

## Cause

`stoneRotOf` read a stone rotter's spread share and its steps alone, at the settlement's middle: never whether its line can live there
(its band of warmth), and one value for every building of the settlement (`Culture::stoneRot`). The ground takes the world's layer once,
at the landing (`site.gen.made`); the cells' cover and crumble were made from it, so while the explorer stood in a town nothing moved,
however many steps passed.

## Fix

Each building has its own share (`Building::stoneRot`, read by `buildingDecay` beside its culture's): the rotter's share at the
building's place, none where its line cannot live (its band of warmth at the building's place, the ground's height as generated: the
stone of a building is the rock its look lies on). `layRuinPieces` lays a cell's pieces out with them.

The ruins now read the world now (`ruinsMade`), which the game hands the ground every frame (`setMadeNow`, from `madeTick`); the
terrain and the structures keep the landing's layer. When its key changes (a step passed), `update` grows every cell's cover again and
lays out again the pieces of a cell where a building's share moved (`regrowRuins`, the ahead worker joined, torn membranes kept).

## Verification (2026-11-22)

- The unit's `seeding` group, new check "the ruins where the rotter lives and as the steps pass": the scene's town of 90 buildings at
  21.6 C. A stone rotter living from 32 to 62 C eats none of them, one from -3 to 47 C all 90. A flesh over the town climbed 450 m in all
  at its 11th step and 1,015 m once its 14th was handed on, the ground's layer the landing's. Without the fix: the rotter out of its band
  eats all 90, and the climb stays 450 m.
- `overgrowth ruins`: the city crumbled by a rotter with no band, as before (6,719 pieces near, walls 2.39 m on average).
