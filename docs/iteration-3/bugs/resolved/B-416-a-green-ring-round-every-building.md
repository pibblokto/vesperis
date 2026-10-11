# B-416: a green ring round every building

status: resolved 2026-10-11
severity: S3
area: the civilisations (what a building stands on)

## Report

The user, with B-415's screenshots of BUBAS (the drone over the MUSICAL CITY: a pale ring round each block, the rings crossing; from the
street, bright green banks at the walls' feet): "there are these round green things around each building - I noticed similar on outher
planets around other buildings as well but in different color - please remove them, I am not sure what they are and what for but they
overlap with each other and other buildings a lot."

## Cause

A-04's foundations (`foundation`, galaxy/buildings.cpp): a people whose genome's land is MOUNDS (BUBAS's Az-Spyzh) banked earth against
every dwelling's walls, a people of PITS sank its dwellings in an earth rim, each drawn as a round bank of the ground's own soil (`FK_SWEEP`
in part 8: the grass of a living world, a desert's sand, the "different color" of the other planets) of the building's longest half side
and 1.6 m in radius: a terraced row 40 m long stood in a ring 43 m across, over its neighbours, the street and the next row. A
prehistoric people's pit huts had the same rim (`genHut`). One people in seven builds on mounds, one in seven in pits.

## Fix

The banks and the rims are gone. A people of mounds shows it in where it sited its towns (T-01's plan), a people of pits in its sunk
floors; the pits, a cut in the rock round a house and a terrace's kerb (a rectangle of the walls' own stone 1.1-1.6 m out) are the
hand-made ages' alone: an industrial row and a modern block stand on the street (`Layout::foundOf`). A plinth of the walls' own stone in
the banks' place was tried and left out: once a ruin's walls fell, it read from the drone as the flat slab of a modern foundation.

## Verified

BUBAS's two towns at the user's marks (`spot`, by day): the CITY from its streets and from 35 m, the MUSICAL CITY from 32 m (the drone's
place in the user's screenshot), no ring and nothing in the rings' place, the roofless rows on the grass; `ruins check` (A-03): every piece
resting on something, every doorway open; `unit` 0 failures (294 checks), `flow` exit 0, the regress unchanged.
