# B-494: a disc's made clouds before its looks

status: resolved 2026-11-23 (J-16)
severity: S4
area: space/space_view.cpp (`setupPalette`: each disc's made banks)

## Report

KI-502, found in B-458 (2026-11-22): "A disc shows five made colours from orbit at most (six on the first): past them a colour takes its
kind's nearest line's bank, and a made cloud waits behind each class's latest look."

## Cause

Each disc deals its made colours from a pool of banks, five or six. The order was: the overgrowth's stain, the waters' tint, each class's
latest look (the land's, the plants' and the snow's), then the made clouds. A world with all three looks filled the second disc's five
banks before its clouds, so its dyed or glowing clouds, which cover the whole disc, lost their colour.

## Fix

The made clouds are now dealt after the waters and before the class looks. A look past the pool takes its kind's nearest line's bank, as
any colour past the pool does.

The pool's size stays. The 32 banks also hold the stars, the sky, the rings and each disc's own family colours. A disc's sixth colour or
more borrows the nearest one, which reads the same at a disc's size. Proposed to close as a limit: KNOWN-ISSUES says so.

## Verification (2026-11-23)

A new check in the `seeding` group, the J-12 record world's two lines with the land's, the plants' and the snow's looks added and red
clouds, drawn as the second disc:

- On the old code: FAIL. Its five banks went to the stain (26), the waters (28) and the three looks (30, 22, 24); the clouds had none (-1).
- On the new code: the stain 26, the waters 28, the clouds 30, the land's look 22 and the plants' 24. The snow's look takes its nearest's.
