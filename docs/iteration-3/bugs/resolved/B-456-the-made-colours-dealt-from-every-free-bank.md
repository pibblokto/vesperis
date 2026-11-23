# B-456: the made colours dealt from every free bank

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/surface_palette.cpp`: `setupPalette`'s pool; `surface_effects.cpp`: the dyed rain and its puddles)

## Report

Four known issues of one cause, the user asking for them fixed (2026-11-22, with the other known issues of the J series):

- KI-459, found in J-04: "On a world of two peoples, or with red snow and strata, a dyed rain falls in the sky's colour and leaves no
  puddles: no bank is lent for its colour (J-02's lent banks)."
- KI-464, found in J-05: "The made looks take the four lent banks (28 to 25): where the traits or a second people hold them, a look past
  the free ones is drawn in its family's bank."
- KI-465, found in J-06: "On a world of a people with a second sun, the two hides share one bank (13) or fall back on the forest's and
  the rock's: 3 of the 64 plate worlds, their marks then darker shades of the hide."
- KI-474, found in J-09: "An overgrowth's cover needs a lent bank: on a world of two peoples (or a second people's forms staged) it and
  its structures are drawn in the rock's colour, its ruins bare."

## Cause

The ground has 32 banks, each a ramp of 64 shades. A seeding's colours (the waters' tint, the land's, plants' and snow's looks, the
overgrowth, the foam) could only take the four the second people's palette lends, 25 to 28, and only where no second people stood and
the traits left them. The waters' tint tried 28 and 27 alone. The dyed rain fell in the waters' tint's bank or none. The hides had a
longer list (24 and 20 where nobody built, 13 with no glowing body in the sky, 12 with no second sun, then the lent four), but a world of
a people with a second sun had at most 13 left.

## Fix

`setupPalette` deals every such colour from one pool, the banks the world leaves free this frame, each handed out once in the order its
use asks:

- the lent four where no second people's forms stand and the traits leave them;
- the first people's 24 and 20 where nobody built;
- 13 with no glowing body in the sky, 12 with no second sun;
- the aurorae's 11 and 21 where none can show: no air, an opaque deck, or under 36 degrees of latitude without great aurorae.

The made colours take the pool lent banks first, so a world that had its colours keeps the same banks. The hides keep J-06's order with
the aurorae's two last, so a world nobody seeded draws its animals as before. The waters' tint, the looks and the overgrowth now take a
bank on a world of two peoples too. A dyed rain falls in the dyed waters' bank, else a bank of its own, else its dyed clouds' (bank 23),
and leaves puddles in it. KI-473's colours a line (B-457) draw from the same pool.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "the made colours dealt from every free bank": `seed record`'s superhabitable world with an
  overgrowing red line, a blue one and a dyed rain, a second people's settlement staged on it (its forms in the lent four). The
  overgrowth, the waters' tint, the two hides and the dyed rain each in a bank of its own, none shared. By the old code the overgrowth,
  the waters' tint and the rain had none there (the lent banks held), and the hides 13 and 12 at best.
- A world nobody seeded: its banks as before (the battery's frames of worlds nobody seeded unchanged).
