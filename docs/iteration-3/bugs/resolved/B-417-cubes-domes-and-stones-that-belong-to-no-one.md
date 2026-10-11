# B-417: cubes, domes and stones that belong to no one

status: resolved 2026-10-11
severity: S2
area: the civilisations (what a world's ruins are)

## Report

The user, with four screenshots of the scanner's RUINS mode saying HERE (a grey cube with strokes cut into its face on an ice sheet,
three long low walls in a savanna, a ring of tall red stones in a desert's rain, grey pillars in a jungle): "there are still all these
old-versions repetetive ruins like cube, half-sphere, old stonehendge, there burried walls, etc - they should be removed, they don't
follow the genome, never have records on anything, they should be removed. Also most of them on Felisian planets as I noticed, I don't
know why (insted of them should be new ruins/works which brings us to the next point)."

## Cause

M4-07's monoliths of the old ones (1.0.0, before there were peoples): one cell in twenty of the 2 km ruin grid of every felisian, quartz
and ocean world held a ring of eight columns, a cube, a dome, three walls or (one in thirty) a 40 m cube, 6-14 m, smooth, striated or with
glowing edges, the star's name's low bits cut into a cube's face as strokes. C-01 kept them to the bit when the peoples came ("so nothing
moved on the worlds that had no civilisation"), and on a felisian world that had a people too, outside its cities' cores: nobody's, of no
age and no genome, holding no record, drawn by code of their own (`drawRuins`' boxes and domes, not A-02's forms). Seven felisian worlds in
eight have no people (`TR_CIVILISATION`: one felisian or desert world in eight), and all quartz and ocean worlds: there they were the only
ruins, sixty within the scanner's 40 km of any point; on a people's felisian world they were most of what it heard between the towns.

## Fix

They are gone: a world's ruins are its people's. `worldHadCivilisation` is the one question (`worldHasRuins` is gone); a world without a
people offers no RUINS mode. On a people's world the draw's first twentieth of a cell goes to the countryside's lone monuments, as it did on
a dead desert world (C-01's rule): a monument of its people's age built in its genome's way (a stone circle, a dolmen or a row of stones; a
platform with its shrine, a tower, a rotunda or a pair of stelae; an observatory, a gasometer, a water tower or a headframe; a mast, a power
station, tanks or a tower; a ring, a wreck or a spire), with C-03's record seven times in ten. `RuinKind`, `RuinSpec::kind` and `style`, `Ruin::kind` and `style`, the monoliths' drawing and colliders and the
`felisian_ruin` scene are gone; a ruin's echo is its settlement's class (`Landmark::sub`: the `+ 1` left room for the monolith's 0). The
second half of the report, what should be found instead, is B-418 (the works).

## Verified

* The first 34 felisian, quartz and ocean worlds without a people near home: none of 57,154 cells holds a ruin (`ruinOfCell`), none
  offers a RUINS or a WORKS mode (`sightKindsOf`).
* Aieliaalas II (a people's felisian world, the C-01 test world), the unit's settlements check: 31 x 31 cells round a town of its plan hold
  85 hamlets, 63 villages, 30 towns and 3 lone monuments (where monoliths stood), no monolith.
* The scanner (R-408's check, landing now 5 km off a town of the people's plan: the old landing point lay outside its land, where only
  the monoliths stood) hears 203 of the people's settlements; T-01's check hears a modern capital from 150 km by its class.
* `unit` 0 failures (297 checks), `flow` exit 0 (35.2 s), the regress unchanged (22 hashes: no monolith stood in the felisian shore's
  frame), `bench check` within every budget (with B-418 and B-419).
