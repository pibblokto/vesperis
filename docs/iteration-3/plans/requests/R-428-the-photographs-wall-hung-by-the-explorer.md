# R-428: the photographs' wall hung by the explorer

status: done 2026-11-18 (1.5.0, before its release; one of five requests aboard, R-427 to R-431)
milestone: 1.5.0

## Request

The user, 2026-11-18: "Photos updating via in-game screenshots", then: "For photos - there was a screenshot of a wall with photos,
don't know how to get them but what I suggest is to let you configure photos using in-game photos you make."

What was there (I-04): the quarters' wall printed the gallery's twelve newest photographs in one tone, by itself, and nothing in the
game said so.

## Done

- E at the wall (`FACE_PRINTS`) opens its screen (`WALL`): the twelve frames as the wall holds them, each print's thumbnail in its
  colours; the arrows choose a frame, Enter opens the photographs (the newest first, one shown large with its caption) and hangs
  the one chosen, Backspace takes the frame's down.
- Until the first change the wall is I-04's, the newest twelve; from then on it is the explorer's, and photographs taken after no
  longer move it (`ShipWall`, the save's `wall` line). The prints stay in one tone: the ship gives each surface one hue.
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-428; `ship life`'s `wall_screen`, `wall_pick`, `wall_hung`; the unit's
  `ship` group.
