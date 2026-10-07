# R-406: Shift runs the probe's clock faster

status: done 2026-10-07
milestone: 1.2.0 (with X-02, on the user's play of X-01)

## Request

"Damn, really good work, I played and really liked it, just wish there was more to look at which is exactly the point of
X-02 and also so there is an acceleration button like shift so you can speed up probe. Proceed"

The relay's clock is real time (X-01's decision: the warp turns the sky, not the fall), so a descent takes seven or eight
minutes free and about four more under the chute, and nothing on the probe's screen made it go faster.

## Done

* **Shift held on the probe's screen** runs the relay's clock eight times as fast (`Game::ProbeState::fast`, set by
  `updateProbeScreen` from the key each frame, read by `updateProbe`): the fall, the entry, the stages and the chute go by
  eight times as fast, the lightning comes eight times as often; `FAST` in amber after the stage's name on the camera's
  frame. Let go, away from the screen (the cabin, the ship's other screens) or at the end it runs at one again. A hold
  rather than a toggle: the descent's moments are worth slowing for, and a toggle left on would rush them; eight, so a free
  descent passes in under a minute.
* The screen's last line: `MOUSE LOOKS  SHIFT FAST  P PHOTO  ESC CABIN` (`X` still centres the camera).
* Harness: the unit's probe check holds Shift for a second of the game (the clock 8.0 s on against 1.0 s plain, back to
  one a second after); the flow holds it a second on the probe's screen (`probe_fast`).
* Docs: 07 (the probe bullet), 08 (the unit check, the flow step), 10-decisions (a section after X-02's).
