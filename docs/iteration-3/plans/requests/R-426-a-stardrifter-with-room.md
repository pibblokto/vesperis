# R-426: a Stardrifter with room, not utilitarian

status: done 2026-11-17 (planned 2026-11-16 as block I of `docs/archive/ideas/ITERATION-1.5.0.md`: I-01 to I-04; the design in
`docs/archive/ideas/IDEAS-small-and-spacious.md` section 2); I-01 done 2026-11-16 (the ship as rows, `game/ship_plan`); I-02 done
2026-11-16 (the new Stardrifter: three decks and eight spaces); I-03 done 2026-11-16 (light, materials and views); I-04 done
2026-11-17 (life aboard)
milestone: 1.5.0

## Request

From the user's request of 2026-11-16 (quoted whole in R-425): "Also another thing I want you do include is expanding stardridter so
that it's way way more specious - right now it's "utilitarian" as one of the players described."

What exists: one room of 5 x 5 x 2.4 m (`game/cabin.cpp`, M2 of the first roadmap). It has a window band on every wall and the roof,
a roof deck reached by a lifter (PgUp, PgDn), the capsule in a cage in the middle, and eleven devices against the walls, each
screen in its own colour (R-404). It is drawn over the space view as about 150 quads a band, in one grey hull bank under one light;
looking out, the space view and the cabin take all 32 banks. The harness, auto-explore and the drone rack hold its coordinates by
hand.

## Plan

`docs/archive/ideas/IDEAS-small-and-spacious.md` section 2 holds the plan of the new ship (three decks, eight spaces, about 820 square
metres), its walk, its light, its views, life aboard and how it is built. The milestones come after Z-01 and Z-02 (R-425):

* **I-01, the ship as rows:** rooms, openings, fixtures and devices as rows, a mesh built once, each polygon projected once a frame,
  the banks dealt from the rows; the harness and auto-explore read the rows. Nothing drawn changes: the 1.4.0 cabin is the first
  plan, proven bit for bit.
* **I-02, the new Stardrifter:** the bridge with its great window, the two-deck hall with the spiral stair and the garden, the
  library, the laboratory, the quarters, the hangar with the capsule and the drones, the engine room, the observatory under a glass
  dome. The devices move to their rooms; the space view is drawn only where a window is in view; nothing needs walking.
* **I-03, light, materials and views:** warm panels, fabric, brass and plants; each room's lamps; the star's light through the
  windows, moving as the ship turns; the hull seen outside (the radar's mast, the drive's ring); the depolarised hull.
* **I-04, life aboard:** the bunk and the almanac, the explorer's photographs on the quarters' wall, the garden, the library's
  shelves of shards, the capsule's hatch, the drive's glow, sound by room.

## Done

* **I-01 (2026-11-16), the ship as rows:** `game/ship_plan` (`docs/decisions/rules-ship.md`): the Stardrifter as rooms, the lifter
  between them, fixtures, colliders, lamps and views, the 1.4.0 cabin its first plan piece for piece; the mesh built once, each frame
  projecting it once and every band filling it; the walk, the facing, auto-explore's places and the harness's views from the rows;
  nothing drawn, played or saved changing (`docs/decisions/log/7-1.5.0.md`).
* **I-02 (2026-11-16), the new Stardrifter:** three decks and eight spaces on the plan's rows (the bridge with its great window,
  the hall two decks high with the spiral stair, the library, the laboratory, the quarters, the hangar with the capsule's cradle and
  the drone rack, the engine room, the observatory under its glass dome), the arches, the stair walked round, the lift; the devices
  in their rooms; the rooms seen through the arches drawn in their boxes of the screen, the space view skipped with no window in
  view, the banks dealt by nearness; auto-explore's routes through the arches and round the stair; `ship rooms` and `ship walk`
  (`docs/decisions/log/7-1.5.0.md`).
* **I-03 (2026-11-16), light, materials and views:** every surface in a finish (warm panels, a darker floor, fabric, brass, plants,
  glass, the hull's grey) dealt its bank by the frame; each room's lamps as pools on its surfaces (the bridge dim, the hall warm,
  the library's reading lamps, the lab bright, the quarters low, the hangar's work lights, the dome dark), U dimming them; the sun
  through every window, the great window's panes and the dome cast as patches on the floors, walls and tables, the boxes' shadows
  in them, moving as the ship turns; the parked world's light on the walls facing it; the hull outside through the windows and the
  dome (the roofs, the prow, the drive's ring astern, the radar's mast turning, the drones' door), lit by the sun; `ship light`
  (`docs/decisions/log/7-1.5.0.md`).
* **I-04 (2026-11-17), life aboard:** the quarters' bunk (`E`: the almanac, a rest watched from the quarters), its desk (the log)
  and its wall of the gallery's last twelve photographs printed in one tone; the hall's garden of the home world's flora, grown by
  the days played and swaying in the draught, and the drive's core under the floor glass, brightening with a flight as the drive's
  ring and the orb do; the library's shelves of shards aglow by how many of each world's are held, the instruments against the
  case's glass, the player's deck (`E`: the player's page); the capsule as the surface draws it in its cradle, its drop through the
  hatch when boarded at the capsule; U's night keeping the reading lamps; each room's sound by the plan's rows (the hum and the
  drive by room, the lab's bubbling, the garden's water, the music fuller in the library); `ship life`
  (`docs/decisions/log/7-1.5.0.md`).
