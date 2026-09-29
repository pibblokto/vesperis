---
id: R-304
title: A far greater variety of planets, with modifiers
status: done 2026-09-28 (O10)
requested: 2026-09-28 (verbal)
related: R-305, O6-04, O6-05
---

## What
"More planets variations, modificators, etc. Regarding planets - it has to be FAR greater variety of planets, tons of
modifications to the planet (don't limit your imagination)."

## My notes (Claude)
* **Traits** (`Trait`, `BodyGen::traits[3]`, `traitEligible`, `TRAIT_NAMES`, `traitPhrase`, `traitList`): every landable
  body draws up to three traits from its type's list, hashed from its seed (a fifth of the bodies have none, two fifths
  one, a bit over a quarter two, the rest three; the landform traits and the world traits are drawn from separate lists
  so the living worlds' hydrology and flora traits are not crowded out by the eighteen landforms; one colour trait at
  most; pangaea and archipelago never together, nor giant and dead flora). The data sheet (`I`) prints `TRAITS`, the
  description sentence carries a phrase per trait (", cut by canyons", ", flora that glows at night").
* **Eighteen landform traits** (R-305): canyon lands, badlands, karst towers, great rift, escarpments, glaciated,
  inselbergs, cinder fields, chaos terrain, patterned ground, yardangs, dune seas, salt flats, trap terraces, great
  basin, coronae, spire fields, geyser basins.
* **Thirteen world traits:** archipelago (the sea level up 0.3, five times the volcanic islands), pangaea (the
  continent field's wavelength 1.8x), lake country (lakes on 85% of the 7 km cells, rivers on 90% of the network),
  snowball (the ice cap from 28-40 degrees, the snow line halved, six degrees colder), exotic seas (a liquid that is
  not water at a hashed level: methane on thin-air worlds, tar on carbon worlds, brine on icy ones; a sea to the
  surface code, `SurfaceSite::hasWater/seaLevel`, coloured by `matColor[MAT_WATER]`), storm world (cloud cover
  +0.35), haze (the haze distance a third, the sky denser), giant flora (trees 1.9x), luminous flora (the leaf banks
  and the canopies glow at night), dead forests (60% of the trees dead skeletons, the flora colour greyed), red soils,
  black sands, chalk lands (the ground family tinted in every view through the material palette).
* Over 300 landable bodies: none 59, one 115, two 89, three 37; the commonest are escarpments, trap terraces, the
  great basin and exotic seas (36-42 each), the rarest the felisian-only ones (1-5 each). `vesperis_test traits` renders
  a sheet per landform trait (`shots/tests/trait_*.png`: a hillshade round the strongest spot, a standing frame and an
  aerial frame) and prints the distribution.
* Generation version 8 (with B-320 and B-321): every solid surface changes; systems and star positions do not.
