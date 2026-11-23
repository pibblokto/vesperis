# B-458: the globe's made banks

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: space (`space/space_view.cpp`: `setupPalette`'s `discMade`; `space_globe.cpp`: `drawGlobe`)

## Report

Two known issues of one cause, the user asking for them fixed (2026-11-22, with the other known issues of the J series):

- KI-461, found in J-05: "The globe gives its one made bank to one class, the waters' first: a world with a waters' look and a land's or
  plants' shows only the waters' colour from orbit (the maps and the ground show all)."
- KI-475, found in J-09: "Under an overgrowth's stain the globe has one made bank a disc: the second disc's seeded waters lose their tint,
  and a made cloud's colour shows on neither."

## Cause

The space view gave each disc one made bank (25 for the first, 26 for the second; 31 the first's waters under a stain where no trait held
it). A stain, the waters' tint, a look and the made clouds took turns for it. Banks 21 to 24 and 27 to 30 stood unused in the space view.

## Fix

Each disc deals its made colours into banks of their own (`DiscMade`): the first disc from 25, 27, 29, 21, 23 and 31 (where no water trait
holds it), the second from 26, 28, 30, 22 and 24. In turn: its overgrowth's stain (its veins lit at night where it glows), its waters' tint
(a glowing line's light at night) or the test layer's land, each class's latest look (land, plants, snow; a glowing forest's light at
night), its made clouds (a dye's or the spores' colour, a glow at night), then every other line's own colours (B-457). Two of one colour
share a bank; past the banks a colour takes its kind's nearest line's. A pixel takes its bank by its class and the line whose look lies
on its texel. A made air's veil covers each disc's dealt banks.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "each line's banks on the ground and the globe": `seed record`'s overgrowing world with a red line
  and a blue one, drawn as the first disc and as the second. Each line's stain and waters in banks of their own on both (four banks a
  disc), the clouds too where it has them. By the old code each disc had one bank: under the stain the second disc's waters kept their own
  colour, and the clouds lost theirs.
- The battery's `seed traits`: the glowing clouds' overcast is drawn in the clouds' own bank and ramp, its night glow as faint as J-04
  made it, where J-12 drew it in the waters' tint's bank (the land under it in the waters' colour at night).
