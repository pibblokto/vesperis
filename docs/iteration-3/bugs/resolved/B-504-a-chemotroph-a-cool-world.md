# B-504: a chemotroph is a cool world

status: resolved 2026-11-23 (J-16)
severity: S4
area: galaxy/kinds.cpp (the chemotroph's sources)

## Report

KI-436, found in Z-03 (2026-11-17): "A chemotroph world made from a hot tectonic world (Weesies II-b) reads 188 C on its ground, its
kind's frost and blizzards aside: the HUD's and the biota's mean (Z-01 drew its mats round 45 C). A generation change."

## Cause

The chemotroph is a made kind: `kindOf` remakes a tectonic or rocky world of a dead star (a white dwarf, a pulsar, a neutron star), or a
tectonic or rocky moon of a brown dwarf, by its three sources in `kinds.cpp`. All three took any warmth (0 to 1e9 K). A world of a dead
star can be hot: close to a white dwarf, or warmed by a brown dwarf's tides. Its mean on the ground is the drawn warmth's three quarters
plus the kind's own 30 C (`biotaMeanC`), so a world drawn at 484 K read 188 C under a kind made for dim cold worlds, with frost, blizzards
and mats drawn for about 45 C.

## Fix

Each of the three sources now takes a world under 300 K as drawn, so the mean stays under 50 C. A hotter world of a dead star or a brown
dwarf stays the kind it was drawn as. This changes what some worlds are, so it is a generation change. It rides generation 18 (the rule:
changes before the release ride the bump made), with its line in `generations.md`. The rule is in `rules-galaxy-worlds.md`.

## Verification (2026-11-23)

A new check in the `kinds` group, "a chemotroph a cool world warmed from within, its mean under 60 C", over the 424 locked and deep
worlds of the scan region:

- on the old code: FAIL, 11 of 51 chemotrophs over 60 C, the warmest (Iereriash V-a) at 254 C;
- on the new code: 0 of 39 over 60 C, the warmest (Feaneweilashan I-a) at 48 C. The group: 12 checks, 0 failures.

`survey 20000`: 65 chemotrophs of 20,004 bodies near the start before (44 from rocky worlds, 21 from tectonic ones), 48 after (33 and 15).
The other kinds' sources and shares are unchanged (the `kinds` group's share check passes).
