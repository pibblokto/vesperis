# B-447: a made kind named on every screen

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: game (`game/names.cpp`: `kindNameNow`, `kindShortNow`, `kindWordsNow`; galaxy/made.cpp: `madeKindShort`)

## Report

KI-482, found in J-11 (2026-11-22): "A made kind is named on the data sheet and the catalogue's head alone (J-12): the analyzer's list,
the IN ORBIT line and the log, and the telescope name the kind as generated." The user asked for it to be fixed (2026-11-22, with the
other known issues of the J series).

## Cause

Each screen read the body's kind from its row (`KINDS[b.type]`). Only the data sheet and the catalogue read the world's layer.

## Fix

Three words for a body's kind as it is now, read from the layer the space view holds for it:

- `kindNameNow`: the made kind's name ("FLESH WORLD"), else the kind's. The IN ORBIT line, the local target's label, a screenshot's
  caption and the console's body list use it.
- `kindShortNow`: ten characters at most ("FLESH", "HOTHOUSE", `madeKindShort`). The analyzer's list and the telescope's line use it.
- `kindWordsNow`: the log's words ("FLESH WORLD, ONCE SUPERHABITABLE"). The ORBIT and LANDING log lines use it.

The tree of kin's world pages name an unnamed world's made kind too ("UNKNOWN - HAIR WORLD"), as the catalogue's heads do. A world
nobody seeded reads as before on every screen. The kinds seen on the statistics page stay the kinds generated.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a made kind named": `seed record`'s HAIR WORLD, once superhabitable. The target's label reads
  "UNKNOWN (HAIR WORLD)", the telescope "UNKNOWN  HAIR  ...", the log "HAIR WORLD, ONCE SUPERHABITABLE", the tree's page "UNKNOWN - HAIR
  WORLD"; every made kind's short name is ten characters at most. Before, each read SUPERHABITABLE (the helpers did not exist).
