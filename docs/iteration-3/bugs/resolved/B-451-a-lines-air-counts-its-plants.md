# B-451: a line's air counts its plants

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: life and game (`life/seeding.cpp`: `seedHabitatOf`, `seedingLivesIn`; game/game.cpp: `vialLines`)

## Report

KI-462, found in J-05 (2026-11-21): "A line's air answers to its waters and land alone: a host of plants or a wood rotter living in the
forests gives its gas as if it lived in its waters only (`seedHabitatOf`)." The user asked for it to be fixed (2026-11-22, with the other
known issues of the J series).

## Cause

A line's share of its world counted the sample points whose waters or land it lives on, never its plants, though its plants' look lay
there. A vial's lines were read by a habitat of their own that left out the plants and a stone rotter's rock.

## Fix

The share counts a dry point of plants (forest or grass, on a world with plants) within the line's band of warmth where it lives in the
plants (a host of plants, a wood rotter). A vial finds a line by the same habitat (`seedingLivesIn`): its waters by their salt, its land,
a stone rotter's rock, and the soil under the plants it lives in.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a line's air counts its plants": fourteen hosts of plants on twenty worlds with plants. Before:
  0 of 14 on more of their world than their own line alone. After: 13 of 14.
