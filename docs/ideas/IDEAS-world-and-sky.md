# Ideas: the world and the sky

Kept from a discussion on 2026-09-30. Not scheduled or designed; each is a few hundred
lines on geometry and models the game already computes.

## Weather that arrives

Storm fronts as moving features on the landing map: visible from orbit as cloud, then on
the ground as a wall of rain or dust coming over the ridge with the wind rising ahead of
it, the light dropping, the temperature falling behind the front. The wind, rain, dust
and lightning models exist; giving a front a position, a heading and a speed turns them
into events you see coming. The data sheet can say "rain in 40 minutes from the west";
the time warp lets you wait for it.

## Tides

Ocean worlds with a large moon (or a close sun): the shoreline moves over hours, floes
ground and float, the capsule left on the sand is in the water when you return. The
water level is one number per body; making it follow the moon's position (and the sun's,
for spring and neap tides) is cheap. Tidal range from the moon's mass and distance and
the world's radius, so a world with a huge close moon has tides of many metres and a
tidal flat kilometres wide. The sector map and the rangefinder should mark the tide line.

## A night that means something

All derivable from the geometry and the system, all quiet:

* **Aurorae** near the poles of worlds with a magnetic field (a body property) under a
  bright or active star: curtains of green and red in the sky bank, brighter after a
  flare.
* **Meteor showers** when a world crosses the orbit of one of the system's comets (the
  comets exist and their orbits are known): a night of streaks with a radiant in the
  right constellation, a few per minute, more at the peak.
* **Zodiacal light** in the dust of young systems (and near belts): a faint cone along
  the ecliptic after sunset.
* The **shadow of a ring** or a moon crossing the ground at night, the **earthshine** on
  a moon's dark side seen from its parent, and the parent's glow lighting a moon's night.

## Fossils and strata

Canyon walls and cliff bands already come in layers. A world's history (an old sea, a
lava flow, an impact, an ice age) can be read in them: the layer sequence derives from
the world's traits and the terrain's own record (drainage, lava fields, crater ages), a
landmark kind marks a good exposure, and the data sheet says what the layers show
("a shoreline 300 m above the present sea"). On worlds with life, fossils in the right
layers: a bestiary of the past, drawn as impressions in the rock. Ties into the landform
and drainage work already done.

## A telescope on the ship

From orbit or deep space, a magnified view on one of the cabin's screens or as a key: a
planet's globe from across the system, a moon's craters, the companion star's disc, a
distant cluster resolved into stars, a comet's tail. The space renderer already draws
at any scale, so this is a zoom, a reticle and a slow drift; the photo mode and the
gallery apply, so a telescope frame can be kept and its target named.

## The time-lapse as a menu, not a toggle

At x1000 the right moment is impossible to catch with a toggle. Replace the toggle with a
small menu (on `Ctrl+T`, or from the data sheet): a list of the next events at this place,
each with a countdown, and "run to it":

* sunrise, sunset, noon, the end of twilight;
* moonrise and moonset of each moon, the parent's rise on a moon, full and new phases;
* the next eclipse (a moon over the sun; the parent over the sun; a ring shadow);
* the next tide (once tides exist), the next storm front, the next shower;
* "in N minutes / hours / days" as a free entry, and "until dark" / "until light".

Choosing one runs the warp and stops exactly at the event (the warp slows in the last
seconds so the moment is seen, not skipped). The same menu on the ship for orbital
events: a transit, a conjunction, the comet's periapsis. Because every event is computed
from the geometry, the list is exact, and it doubles as the almanac of the place.
