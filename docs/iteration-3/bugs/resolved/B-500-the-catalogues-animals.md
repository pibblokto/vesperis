# B-500: the catalogue's animals

status: resolved 2026-11-23 (J-16)
severity: S4
area: game (game/catalogue.cpp: `catalogueWorld`, `animalFates`, `catalogueBuild`, `catalogueAnimalMarks`, `renderCatalogueAnimal`,
`renderCatalogueBeast`; lab_state.h: `CatalogueAnimal`, `CatalogueWorld::genesis`)

## Report

KI-488, found in J-12 (2026-11-22): "The catalogue's animals are listed by world alone (by kin lists the microbes), their sheets are
words with no picture, and their marks are what the ground showed: a fate never seen there marks nothing."

## Cause

J-12 added the animals seen to the catalogue's list by world. Its page by kin was made from the sequenced microbes' lineages and left
the animals out. An animal's sheet (`renderCatalogueAnimal`) was its lines alone. Its marks (`catalogueAnimalMarks`) were the guide's
record of what the ground showed: a species killed off by a line while the explorer was away read nothing, though the world's layer
held its fate, as it holds the microbes that died out (DIED OUT is read from the record, not from what was seen).

## Fix

- **Their fates as the record holds them:** each animal's fate over its world at the guide's clock (`animalFates`): on 128 points
  spread over the world, where it lives (the land, the air over all of it, the waters as generated), the ground's own reading of a
  species' fate at a point (`madeFateAt`, `animalFateOf`, its changes by the world's steering). GONE where it died out at every point,
  DYING where any died, CHANGED where young were born changed. These join the marks the ground showed. The dim rows, the sheet's FATE
  and the LIFE page's count of animals gone read them too. A world nobody seeded marks nothing.
- **By kin:** each animal follows its world's genesis, after that genesis's species, once one of the world's own species of it has
  been read (the explorer then knows its kin). Until then the animals are listed with those NOT YET SEQUENCED. By kin a row says its
  world where it has room, as the species' rows do.
- **Drawn:** an animal's sheet draws it as the ground does, its body posed and made into shapes by the ground's own code (`rigOf`,
  `poseAnimal`, `emitAnimal`), three quarters on and turning a little; a flyer on the wing, a swimmer afloat. It is drawn in its
  world's two hide colours on a grey ground, smoothed as the ground's frames are, in the box a microbe's form takes, with a scale bar
  in metres. Its lines stand beside it.
- `seed record` draws the catalogue by kin too (`record/catalogue_kin.png`).

## Verification (2026-11-23)

- The unit's `incubator` group, new check "the catalogue's animals": `seed record`'s world (a nerve poison's line at its 64th step),
  every animal of its bestiary seen and none marked on the ground.
  - By the old code: of 8 animals none read a fate, none was listed by kin, no sheet drew one.
  - With the fix: by world all 8 read a fate ("STRIDER - DYING"); with the guide's clock back at the line's letting go, none; by kin 8
    rows listed with those not yet sequenced (none under the world's genesis), then, once one of the world's own species was read, all 8
    under its genesis; 8 sheets each drawing its animal (the fewest 358 of the picture's pixels in the hides' banks).
- `seed record`: the catalogue (`record/catalogue.png`: STRIDER GONE, the others DYING), an animal's sheet (`record/animal_gone.png`)
  and the page by kin (`record/catalogue_kin.png`).
- Left: KI-NNN, the fate's points read the world's waters as generated (a runaway's dried seas still count as its swimmers' waters).
