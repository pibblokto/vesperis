---
id: R-307
title: More planet types, with their own events
status: done 2026-09-28 (O11)
requested: 2026-09-28 (verbal)
related: R-304, R-305
---

## What
"Please also add more planet types like venusian, quartz, rocky, gas, etc. - come up with more, maybe some seismically active
like volcanic but with huge lava geysers, maybe something like Europa with underground oceans which causes huge water geysers
... on top of my idea put your ideas for more variants and make sure recent modifications and terrain changes work with new
kinds as well."

## My notes (Claude)
Six new landable types (`PT_EUROPAN`, `PT_TECTONIC`, `PT_DESERT`, `PT_HYDROCARBON`, `PT_BOMBARDED`, `PT_ACIDIC`; 19 landable
types of 22), each with its own planet function, palette, sky, weather, temperature and pressure, description, trait list,
map and globe, and an event of its own on the surface:

* **Europan** (small, airless, the cold zones and a giant's inner moons): an ice shell over a hidden ocean. Lineae at three
  scales (double ridges 120/45/12 m tall with a trough between, on the boundaries of 0.25 R, 40 km and 6 km cells), the great
  ones staining a band 6-16 km wide red-brown (`MAT_DUST`, what a globe shows), chaos terrain where the shell broke into
  rafts, a few craters. **Water geysers** (`drawGeysers`, `drawJet`): vents hashed on a 300 m grid where the 16 m ring is
  stained, in three classes (160, 400 and 800 m high), a third of them blowing steadily and the rest in cycles of one to
  three minutes that start with a burst; each a dense bright column cut ragged at its sides, a spray crown and a veil of
  ice falling back in the weak gravity (the user asked for them "way stronger, dense, hitting hard, varying in size, some
  constant and some periodic": the first version was a scatter of points).
* **Tectonic** (medium, a thin sulphurous sky, the warm zones and a giant's inner moons): rift valleys 20 km wide and 800 m
  deep with flat floors on the boundaries of 0.3 R cells, fissures on the floors (trenches on 5 km cells) running with lava
  100-180 m wide, fault-block ranges, ash plains, sulphur crusts round the vents, volcanoes. **Lava fountains**: every
  15-40 s a great fountain rises from a lava cell within 600 m, 250-450 m high for half a minute (eruption kind 1, the same
  dense jet as the geysers in the lava bank), while steady fountains of 30-90 m play on the lava and fumaroles steam from
  the sulphur crusts, one in four a periodic hot spring; **quakes** every one to three minutes: the camera heaves and
  trembles (`quake`), the rumble (`audio.lava`) swells. The lava bank, the ground glow and the infrared view treat it as a
  lava world (`isLavaWorld`).
* **Desert** (medium, 0.3-1 atm, the middle zones): hamada plains, mesas where the dry flats terrace, canyons, ergs of
  10-32 m dunes in the basins with salt pans at their floors, hot days and cold nights (a 32 degree swing), an ochre sky,
  dust storms (rarer than a thin-air world's). **Dust devils** (`devils`): up to four within 500 m, born every 20-50 s,
  30-110 m tall, walking with the wind for two to five minutes; a thin-air world in a dust storm gets them too.
* **Hydrocarbon** (medium, 1.4-1.7 atm at -180 C, the cold zones): an opaque orange haze deck (`hasOpaqueDeck`: the globe
  and the sky are the deck, no sun disc, no cast shadows, the picture blurred like a venusian's), rounded ice-rock hills,
  dark tholin dunes in the low latitudes, channels, cryovolcanic domes, and seas of methane at the reference level
  (`liquidLevel`; a quarter of the surface, more toward the poles) drawn with the sea's ramp and its Fresnel; a methane
  drizzle now and then.
* **Bombarded** (small, airless, near the star and round dead stars): craters at six scales with sharp rims and bright rays
  over a dark glassy regolith, ejecta boulders. **Meteorite strikes** (eruption kind 2): every 20-60 s within 1.2 km a white
  flash, then eight seconds of dark ejecta with glowing chips on ballistic arcs, a shake and a thud.
* **Acidic** (medium, 2-5 atm at +50 C, the warm zones): bleached rock eaten into karst (sinkholes on 300 m cells, towers in
  the wettest basins), sulphur crusts along the shores, seas of dilute acid at the reference level (a third of the surface), a
  yellow-green sky that rains more often than not.

Shared: `typeSky` gives every atmospheric type one zenith/horizon pair used by the surface, the globe's hemisphere light and
the landing map (the three copies of that table are gone); the new types are eligible for the traits that fit them (canyons,
mesas, karst, rifts, escarpments, inselbergs, cinder fields, chaos, patterned ground, yardangs, ergs, salt flats, traps, great
basins, coronae, spires, geysers, storms, haze, the colour traits); the survey over 2,014 bodies: europan 7.2%, bombarded 4.4%,
hydrocarbon 3.2%, desert 2.2%, tectonic 1.8%, acidic 1.3%. Scenes `europan_crack` (on the ice beside a narrow linea, facing
it), `tectonic_fissure`, `desert_erg`, `hydrocarbon_shore`, `bombarded_plain`, `acidic_shore`; `scene <name> [scale] [warm]`
runs the world that many seconds first and stages the type's event in view (a fountain 160-420 m ahead, a strike 220 m
ahead, a devil 130 m ahead). The type table changes every system (GEN 9 with B-322).
