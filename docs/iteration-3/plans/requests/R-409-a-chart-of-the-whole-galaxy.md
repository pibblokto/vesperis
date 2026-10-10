# R-409: a chart of the whole galaxy, and the next jump chosen on it

status: done 2026-10-08 (G-05, G-06 and G-07; `docs/ideas/ITERATION-1.3.0.md`)
milestone: 1.3.0

## Request

"I would like you to add a feature that would desplay whole galaxy map and show a point where you are + density of the stars
and the ability to select on that map where to jump next, it should be in this release."

The star map (`M`) shows the ten light years round the ship (`nb.stars`, rings at 2, 5 and 10 ly), by the design of
`PLAN-galaxy-scale.md`; nothing in the game draws the galaxy's shape (the harness's `galaxy` mode writes a face-on map for
the docs only). A jump is chosen in the aim by a direction and a number of light years (R-407), so a far place is reached by
turning the ship toward where it is believed to lie. A galaxy page of the star map was offered on 2026-10-02, when the user
asked whether the galaxy really holds 200 billion stars, and not scheduled until now.

## Plan

* **G-05, the galaxy chart:** a second page of the star map (`Tab`): the disc face-on by its stars' density, the edge-on strip,
  the ship, home, the route, the remote target, the named stars, the radar's locked signals; zoom from the whole disc to about
  500 ly across; also in a Vimana flight.
* **G-06, the next jump chosen on it:** a cursor with what lies under it (distance, way, region, density, flight time), Enter
  plots the jump to the star nearest the point (R-407's `jumpTarget`), Enter again flies; a look at the neighbourhood of the
  plotted end first; `N` steps through what the explorer has on the chart.
* **G-07, the crossing** (right after G-06): the galaxy drawn turning round the ship in a long flight.

## Done

* **G-05 (2026-10-08):** `Tab` in the star map turns to THE GALAXY (`game/galaxy_chart.cpp`): the whole disc face-on in the light
  of its stars (the closed-form column `galaxyFaceOnColumn`, coloured by the populations: the core warm, the arms blue-white) and
  edge-on under it, nine levels from the whole disc (110,000 ly tall) to 430 ly, the globulars, and from level 6 the open clusters
  and the star-forming complexes; the ship (`YOU`), home, the stars visited and the way between them (`Guide::route`), the remote
  target and its line, the signals the radar locked, the named stars; where the ship is (`13,040 LY FROM THE CORE, IN THE PLANE`,
  the region). `M` opens it in a Vimana flight too. Decisions in `10-decisions.md` (G-05), what stays in KI-366.
* **G-06 (2026-10-08):** the next jump chosen on the chart. A cursor (the arrows, the mouse; the view moves with it at the face's edge,
  the zoom keeps its pixel, `PgUp`/`PgDn` its height over the plane) and what lies under it: the region, the stars there (`ONE STAR IN 7
  SECTORS`), its height, the way and the flight from the ship (`5,000 LY COREWARD - 61 S BY VIMANA`), what the explorer has there.
  `Enter` plots the jump to its point by R-407's line (the star nearest the end, the void sending it back to the first stars),
  `Enter` again flies it, `Esc` keeps the target for `V`; on a plotted jump `Tab` looks at the stars round its end, where another can
  be taken; `N` steps through the locked signals, home, the named stars, the marks and the systems visited; `L` names a place of the
  galaxy (`gmark` lines, lent by an export). Decisions in `10-decisions.md` (G-06), what stays in KI-366.
* **G-07 (2026-10-08):** the crossing. The galaxy's band is drawn in a Vimana flight from where the ship is along its line (KI-364: it
  was not drawn in a flight): keyframes built ahead of the ship a share a frame and blended (`SpaceRenderer::flightBand`), the first the
  band the ship was seeing, the last handed to the parked band at the arrival. A jump coreward shows the bulge widening until it fills the
  windows, a jump north the disc dropping away under the side windows; the star-forming regions' patches fade with the flight's speed (at
  a crossing's pace they came and went within a frame). `vesperis_test crossing`, `stability flight`, the bench's `vimana 2x`. Decisions in
  `10-decisions.md` (G-07), what stays in KI-367.
