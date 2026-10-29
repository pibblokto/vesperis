# B-432: the title forgot a landing

status: resolved 2026-10-29 (found 2026-10-26 in U-05's survey as KI-433; fixed on the user's word)
severity: S3
area: the title (`game/menus.cpp`, `updateTitle`), where the game is (`game/states.cpp`, `placeNow`), the save (`persistence.cpp`)

## Report

Found in U-05's survey (2026-10-26) and offered as KI-433: "A game saved on the ground and opened again: H (or C) then a key at
the title, and Enter resumes in space, not on the ground; the next save drops the landing. The title's H sets the scene to the
title." The user: "lets fix KI-423, KI-432, KI-433 and KI-434 one by one starting with 423".

## Cause

The title kept the game it resumes in `returnState`, the variable that also holds the scene a screen is drawn over. The load set
it (`returnState = state == SURFACE ? SURFACE : SPACE`), the title's Enter read it (the ground if the scene was SURFACE and the site
valid, else space), and a save at the title read it (`surface 1` when not in play and the scene the ground). The title's help and
credits are drawn over the title, so H and C set the scene to TITLE before they opened (`returnState = TITLE`): after either the
landing was forgotten. Enter went on in space, the site still loaded under the ship, and a save at the title (the one closing the
window makes) wrote `surface 0`.

The same variable could not hold the capsule: the load of a game saved on its way down or up (`phase 3` or `phase 5`) put the
state in the capsule, but the scene it set was SPACE. So such a game, opened at the start, went on in space after Enter, help or
no help, and the next save dropped the landing. A probe being watched (its save's flag) likewise went on in space.

## Fix

The title is opened from the game it resumes: the start (`Game()` and `loadFromDisk`) enters it by `openScreen(TITLE)`, so
`openedFrom[TITLE]` holds the new game's space or the load's place (the ground, the capsule's way down or up, the probe's screen,
space). Its help and credits leave that alone: Enter resumes it (a ground state while the site is valid, the probe while it
falls, else space).

Where the game is has one reader, `placeNow()` (states.cpp): the state in play, else the scene the screen is drawn over, and at
the title or over it the game the title resumes. The save writes its `surface`, site and `phase` lines and the probe's watching
flag by it, and the photo's caption (B-431) names the ground by it. No member was added.

## Verification (2026-10-29)

A unit check in the `states` group, "the title keeps the game's place through its help and credits: a save there, and Enter
(B-432)". A landing on the home system's first world, a save a second into the capsule's way down and another on the ground. Each
is opened as the game starts (`loadFromDisk`, the title). Then H and Esc, C and Esc, a save at the title (as closing the window
makes one), and Enter.
- Before the fix it failed: the ground's save at the title "surface 0" and Enter SPACE; the capsule's "surface 0", no phase, and
  Enter SPACE.
- After it, it passes: the ground's "surface 1" and Enter SURFACE; the capsule's "surface 1", "phase 3" and Enter DESCENT. The group
  has 5 checks with 0 failures, B-431's among them.

`tools/prove compare u06` (30 items, 1,793 files, 1,299 s): 28 items the same. The unit differs only by B-431's and B-432's checks
(409 checks with 0 failures against 407). The flow differs in B-431's two frames, pixel for pixel as with B-431 alone. game.h is
47,849 bytes (47.8 KB of the 48). `make check` passes.

The harness's `settings` opened the game from the flow's saves (`shots/tests/test_save`): run by hand after the flow, whose newest
save watches the probe, the title now went on to the probe's screen, its Esc to space, and the mode found no SETTINGS row. It
reads a save prefix of its own now (no save, as in the battery, where every item starts from an empty `shots/tests`): run after
the flow it changes the field of view again (85 to 90), and `tools/prove compare u06 settings flow` finds `settings` the same.

In the game: land, quit, start the game again; at the title press H and a key, C and a key, then Enter: the expedition goes on
on the ground. Quit during the capsule's way down and start again: Enter goes on down.
