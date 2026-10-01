# Idea: caves

Status: discussed 2026-09-30 and 2026-10-01, not designed, not scheduled. The terrain plan
(`docs/iteration-3/plans/PLAN-terrain-3.md`, sections 6 and 9) left caves out of Terrain
III because the planet function is a height field: one height per point on the planet,
which is what lets the orbital map, the landing map and the walking terrain come from the
same function. A cave is an overhang, and a height field cannot express one. Nothing in
Terrain III blocks caves; the pit chains and cave mouths of O6-05/O6-06 are surface
features a cave milestone can open up.

## The framing

**Caves are the underground of the drainage system.** The drainage tiles already flood the
ground per one-degree tile and route rivers to lakes and seas. On karst worlds (the karst
trait exists) some rivers should vanish: a river meets a sinkhole and goes underground,
and comes out somewhere lower as a spring. The cave is the path between them. You can
follow a river into the hill and out the other side. That gives caves a reason to be
where they are, an entrance and an exit, and a shape (a stream passage) without any
hand design.

Other kinds, each with a cause in the terrain:

* **Lava tubes** on volcanic and tectonic worlds, under the lava flows: long, smooth,
  with skylights where the roof fell in.
* **Sea caves** at the foot of exposed coasts (coasts are shaped by exposure already):
  short, loud, flooded at high tide once tides exist.
* **Ice caves** under glaciers on cold worlds: blue light through the roof, meltwater.
* **Collapse pits** opening into chambers under crater terraces and canyon walls.
* **Cut galleries** in the ruins of civilisations (`IDEAS-ruins-shards-signals.md`): the
  deeper shards lie in them.

## How it could be built

* **Placement.** A cave is a landmark kind (`LM_CAVE`) found through the existing landmark
  grid (one feature per ~14 km cell, deterministic by hash), so the maps, the rangefinder,
  the log and the guide handle it like a peak or a crater. The mouth is placed where the
  terrain allows it: a sinkhole of the drainage, the end of a lava flow, a cliff band, a
  coast.
* **Geometry.** A carved volume near the surface: a polyline of passages and chambers
  (radius, height, roughness per segment) generated from the landmark's seed and bent to
  follow the ground between entrance and exit (down from the sink, out at the spring). A
  **hole is cut** in the terrain rings around each mouth (the shore cut of B-322 is the
  model: a contour cut into the mesh) and a **separate tunnel mesh** is drawn behind it,
  with its own collision. The height field stays untouched everywhere else.
* **Rendering.** The tunnel is rock material on the inside, lit by the mouth's daylight
  falling off with distance, then by the explorer's own lamp (a light the game does not
  have yet: a cone from the helmet, needed here and useful at night). Water on the floor
  where the stream runs, drawn as the surface water is. Skylights as holes in the roof
  with a shaft of light. Sound: drips, the stream, a long reverb.
* **Scale.** Hundreds of metres to a few kilometres; a passage you can walk in ten
  minutes. Deep systems are not the point; the way through is.
* **Consistency.** Everything comes from the body's seed and the drainage tile, so the
  orbital zoom can mark the sinkhole and the spring before you land, and two explorers
  find the same cave.

## Open questions (the design to settle first)

* How a cave is found: only on foot through the landmark log, or marked on the landing
  zoom like other sights once seen?
* How dark: a lamp with a range, or caves always lit by skylights so the look stays
  readable at 320x200? The mush filter and the palette banks favour a lit cave with deep
  shadow over true darkness.
* Does anything live there? The bestiary could give cold worlds a cave species, but the
  register says: mostly nothing, water and stone.
* Collision and the buggy: caves are on foot only; the buggy stays at the mouth.
* How the sector map draws a place under the ground (a dotted passage between the two
  mouths, once walked).

## Size

A milestone of its own, comparable to the drainage work: the cut and the tunnel mesh are
the new machinery, the lamp is new, the placement reuses landmarks and drainage. Worth
doing after the generation bump of `PLAN-galaxy-scale.md` and `PLAN-star-classes.md`, so
caves land on the final galaxy.
