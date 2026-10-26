# R-415: the peoples on the landing map

status: done 2026-10-11 (released in 1.3.0)
milestone: 1.3.0 (with R-414, the same message's last part)

## Request

"Final thing - on the capsule deployment map there are these dots for places were civilization ruins were found - they are too huge
and non-subtle. Please, make them turned of by-default and make it a separate button. Why? It would polute map if you find too many
cells with ruins/works. There should be some map like the one you used when you were adding cities layout, it just should be a button
toggle. Also It would be cool to have something like whole map autocompleted when you find all the shards for a civilization, as if you
have enough information to know where is what, where to find cities, capital, etc - this web of civilzation you generated while testing"

What the map drew: the explorer's marks of the world (R-408) as a glyph each with up to eight names in the zoom (the user's BUBAS holds
twelve marks, all of ruins, seven of them within a few kilometres in one town and three in another: a heap of glyphs and a name or two),
a point each on the world's map; and in the zoom, always, the old roads (C-09) and the plan's places' cores (T-01). "The map you used
when adding cities layout" is the harness's plan map (`plan <sx> <sz> <body>`: a people's land tinted to its frontier, its ways by kind, its places by
size).

## What was done

* **V, the peoples, off as the map opens** (`landPeoples`, `game/landing_map.cpp`): without it the map is the land, the clouds and the
  explorer's marks of the natural sights; the marks of ruins and works, the zoom's old roads and the places' cores (`zoomCivImg`: the
  cores over a copy, `zoomImg` the land alone) are the layer's.
* **The marks of ruins and works** in the layer: a small point each (a frame pixel's square at 1x, three at 2x, with a dark rim), the
  names of the three nearest the cursor listed beside it.
* **The places the explorer knows**: each record the ship holds was kept at a place of its people's plan (its home, T-04's
  `recordHomesOf`, read with the world's items as the drone bay reads them: `droneWorldAsk` from the map's opening, `landKnownOf`); the
  layer draws those places, a town a point in the people's colour (the first amber, the second cyan), a city paler and rimmed, the
  capital white and named `CAPITAL`. In the zoom a ring round each known place's core.
* **A people's whole map once all its records are held** ("whole map autocompleted"): its land tinted to its frontier, every place of
  its plan and every way between them, coloured by kind as the plan's maps (`NET_KIND_COLOURS`, `galaxy/plan.h`, now the harness's
  too); in the zoom every place of the window ringed.
* **The legend** at the map's top left while the layer is on: each people, its records held of its count (`THE JENGNE: 21 OF 147
  SHARDS`; `A PEOPLE` until a record of theirs is held, as the decoder names them), `THE JENGNE: ALL 147 SHARDS, THE WHOLE MAP` once
  they all are; `READING THE WORLD...` while the reading runs; `NO PEOPLE LIVED ON THIS WORLD`.
* **The hints** fit the screen now (they ran off both edges at 61 and 66 characters of 53): `Z ZOOM  R RANDOM  C CLOUDS  J SKY  V
  PEOPLES  ENTER`, zoomed `ARROWS 1 KM  Z WORLD  R RANDOM  V PEOPLES  ENTER`.

## Verified

* **`peoples map`** (the drone bay's world of two, Yeesheleemiola II-b, at 2x, the cursor on the first people's capital, two marks of
  ruins by it): as the map opens the layer is off and the marks hidden (`landpeoples_off`); `V` with nothing held draws the marks and the
  legend (`A PEOPLE: 0 OF 147 SHARDS`); a seventh of the Jengne's records held (21) shows 21 places, the capital among them
  (`landpeoples_some`); all 147 held, its whole map: 454 places, 847 ways, its land and frontier (5,222 pixels: `landpeoples_all`); the
  zoom over the capital with the layer, the roads, the cores, the ring and `CAPITAL` (`landpeoples_zoom`), and without it the land
  alone (`landpeoples_zoom_off`).
* **The unit's check** (the modern age world, the same): off at first and the mark hidden; on, the mark and no place with none held; 17
  records held, 17 places drawn of their 17 homes and no way; all 147, every one of the 454 places, 847 ways, the land, the capital
  named once; the zoom's ring and its capital; 81,997 pixels the layer's in the zoom.
* **The flow's** `roads_zoom` (Aieliaalas II) with `V`: 1,164 roads, the Dinaurlel's 13 held records known.
* `unit` 0 failures (313), `regress` unchanged (22 hashes), `flow` 35.0 s, `bench check` within every budget.
