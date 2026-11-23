# src/galaxy: the worlds, drawn from the hashes

Generators: the same input always gives the same output, and the harness pins much of it. Of the world code only `landmarks.cpp` and
`planet_bodygen.cpp` read `src/peoples/`. Over 50 KB (read by range): kinds, system, drainage, probe, planet_landforms, planet_grounds,
planet_grounds2, starfield.

## Files
- `starfield` (the star field, the galaxy's shape: `starInSector`, `jumpTarget`), `system` (orbits, spin, light, tides, `GEN_VERSION`:
  `StarSystem::generate`, `kindOf`), `kinds` (`KINDS`, a row per `PlanetType`, `TRAITS`, `kindHas`)
- `planetmap`: the planet function's entry (`sampleSurface`, `PlanetMap`, `typeSky`, clouds). Its parts share `planet_internal.h`
  (`GroundCtx`, the lattice): `planet_bodygen` (`BodyGen::make`, ramps, climate), `planet_features` (relief, cliffs, and the finders in
  `planet_features.h`), `planet_landforms` (`landformsAt`), `planet_felisian` (the living worlds, `applyDrainage`), `planet_grounds` and
  `planet_grounds2` (each kind's ground)
- `drainage` (rivers, valleys and lakes in cached tiles: `drainageAt`), `landmarks` (the scanner's sights: `sightsNear`), `describe.cpp`
  (`describeBody`, declared in planetmap.h)
- `made` (J-02: what a seeded world was made into at a step, `MadeWorld`, carried in `BodyGen::made`: the spreads (`madeReachAt`, a
  share that never fades with the detail), a cover and a tint over the land, `madeKey`). The planet function's last pass reads it.
  J-03: each spread's habitat (the waters by their salt, the land, a band of warmth: `madeLives`, `madeSpreadAt`), a tint over the
  waters with a glow at night. `placeMeanC` (planet_bodygen.cpp, moved from the surface's sampling): a place's mean warmth.
  J-04: the made air's fields (`warmC`, `pressureK`, the traits, the looks; `ground()` what the last pass draws, with a runaway's dry
  seas), `madeCapShare`; in planetmap `madeWetAt`, `madeSaltAt`, `madeMeanAt`, `madeMatColour`; `typeSky` and `sampleCloudPattern` read
  the air; the living ground reads the made warmth (`felisianSample`: the caps, the snow line, the ice), its heights but the ice's never.
  J-05: the made looks by class (`MadeClass`: the waters, the land, the snow and ice, the plants; `MadeLook`, a spread's `makes`, its
  `landOn`, `lookWaters`, cover), the waters', ground's and plants' traits; in planetmap `madeClassOf`, `madeGlobeClass`, `madeLifeTint`,
  `madeColourAt`, `madeMatColour` for a sulphide's waters, a rusted ground, lush and bleached plants; the made pass's shares by class.
  J-08: the animals' fates (`MadeFate`: the air's in `MadeWorld::fate`, a line's in its spread's; `fauna`, `fateAge`, `rotFast`,
  `radiusKm`), `madeFateAt` (a point's: the air's and each line's by its share there, its age from the spread's arrival).
  J-09: the overgrowth (`MadeGrowth` in a spread: its type, looks, age, reach and cover; `growths`, `growthLine`), `madeGrowthAt` (a
  point's share, its edge fixed to the place); in planetmap the cover's pass (`madeGrowth`, last), `madeGrowthMaterial`, and
  `madeGlobeGrowth` (the globe's made bank to its stain). The three cover materials are TISSUE, THREADS and HARD (`kinds.h`).
  J-10: its steps on past 24 (`MadeGrowth::steps`), `madeGrowthSince` (a point's steps under it), a stone rotter's steps
  (`MadeSpread::pits`), `MADE_WET_GROUND` and `madeGrowthBreathes` (the traits WET GROUND and BREATHING).
  J-11: the made kind (`MadeKind`, `MadeWorld::kind`, the snowball's `freeze`, `deadCover`) and its words (`madeKindName`,
  `madeKindSentence`, `madeKindPhrase`); planetmap.h's `deadCoverOf` and `deadGroundOf`; `felisianSample` lays a made kind's dead cover.
  B-447 to B-458: `madeKindShort`; each line's own colours (`MadeSpread::waterTint`, `look`, `glow`; its plants' glow `plantsGlow`) and
  the line at a point (`SurfaceSample::madeLine`, `PlanetMap::mline`, `madeColourAt` and `madeMatColour` by line); every kind's caps and
  frosts by the made warmth (planet_internal.h's `madeCapDeg`, `madeSnowM`, `madeFrostLess`); an ocean's dried floor (`driedSeaFloor`); a
  made snowball's ice in the grass family (`matFamilyOnGen`). J-15: `madeGreens` (B-482: any bare world with air, no plants or seas of
  its own), `MADE_GREEN_AFTER` and `MADE_GREEN_THICK`, `madeGreenAt` (a carpet's grass behind its edge, read with the edge's
  own noise), `MDK_GREENED_DESERT`; the cover's pass lays grass there over any land look, and `madeMatColour` gives the grass its line's
  pigment's colour (B-482: drawn in its line's own growth bank, never the world's grass bank). J-16: `MGT_COLOURED_SEEPS` and a deep
  line's `landsGlow` (B-479); `madeLives` lets a rock line's look lie on its bare rock
- Sky: `almanac` (`almanacOf`, the finders' `registry()`), `nights` (showers, planetshine), `fronts` (`frontsOf`)
- `probe` (giant atmospheres, the probe's fall: `probeStageAt`), `drifters` (sunless worlds: `drifterInSector`, and `placeInSector`,
  which resolves every saved place)

## Rules and traps
- Protected worlds: a people's world and the home world keep their kind and take no ground-moving trait. Heilya II stays felisian. The
  regress and bench worlds don't move.
- Never insert into a saved or drawn list: kind and trait ids are saved keys and set the draw order (append before `PT_COUNT`,
  `TR_COUNT`). Picks draw only below `*_DRAWN_COUNT`, and entries after it are given, not drawn.
- A new decision draws from its own salt or stream, after the old draws. Pinned salts keep home and the regress worlds as drawn
  (kinds.cpp's `// pinned:` rows, signals.cpp's cells).
- A catalogue's share is a seeded shuffle with a count, not a coin per item. A hash chain steps by a number coprime with its catalogue's
  size. What shows from afar is hashed per cell of sectors, never per world. A new star class is a variety (`mix64(seed ^ c)`), and a
  special world a forced trait (its own hash, put first).
- The view, the finders and the harness read the body, the system and `BodyGen`, never a site. A material never depends on a faded value.
- J-02: a generator from `make` has no made layer; only the game puts one in (`SpaceRenderer::genFor`, `SurfaceSite::initFrame`). The
  peoples, the sights and the drainage read the world as generated: `unseasoned` and `thenWorld` take the layer off, and `buildTile`
  does for a living world. Every read of `made` does nothing when its key is 0.
- `Rng` goes by reference (a copy forks the stream). Moved code keeps each expression's shape (clang fuses a multiply into an add within
  one expression). The ~120 expressions that draw twice stay as written.
- kinds.cpp's tables are built at compile time: every row's arrays of non-zero defaults are set first, in order, and the checks read a
  table built for them (B-430: GCC 9.4, the Linux zips' compiler, lost 55 kinds' rules and crashed on `BUILT_KINDS`).
- Each file keeps its state (`thread_local` guards and caches). `g_reaper` is defined after drainage's tile cache, so it joins the
  prefetches before the cache goes.

More, with the numbers: `docs/reference/lessons.md`, "Generation: the worlds". The rules: `rules-galaxy*.md`.
