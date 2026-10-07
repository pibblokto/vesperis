# R-405: the stabiliser, and the ship carried round its world

status: done 2026-10-06
milestone: 1.2.0 (on the user's review of W-06's rework)

## Request

"When you are in telescope planet still moves and it creates shaking. How I suggest you to fix it - when you enter
telescope mode planet completely locks on the side you are located on - it should feel like it's not moving at all
while in this mode (lore-wise it should be something like comprehensive stardrifter stabilization system). Also another
thing I would like you to add is an ability to rotate stardrifter around the planet. But then you can just enter
telescope and you will lock on the side you are facing."

What moved: from a parking the ship lapped its world in ten to twenty minutes (70 km/s of ground), and the world turned
under it besides; the rework's ground hold kept one point under the reticle at deep powers while everything round it
slid, and the lower powers tracked the centre with the ground streaming across it.

## Done

* **The stabiliser** (`Game::telescopeHoldFrame`/`telescopeHoldView`/`telescopeStabilised`, `tele.frameBody`, `parkBF`,
  `viewBF`): while the eye is at the eyepiece a ship parked at a body holds its parking direction and the view's axis in
  the body's frame, so the lap is held off and the world's turn carries the ship with it: at any power nothing moves but
  the light and the clouds. Opening the telescope keeps the attitude (the side faced is the side held; `STABILISED ON
  <name>` on the frame); Enter on the parked body centres it and holds again; another body or the sun is tracked on its
  centre as before. Stowing lets the lap go on from where the ship is. The ground hold is gone.
* **Round the world** (`Game::orbitShip`): with `Shift` held the arrows carry the stowed, parked ship round its world,
  Left/Right east and west along its parallel (about the spin axis), Up/Down north and south along its meridian within
  85 degrees of the poles, twenty degrees a second (a lap in eighteen seconds); the attitude turns with the ship so the
  world stays where it was in the window and its ground streams past; the status reads `OVER 12.3° N  134.5° E`. The
  help's flight page: `SHIFT+ARROWS  ROUND THE WORLD`.
* Harness: the unit's telescope checks rewritten round the stabiliser (the ground under the reticle within 0.1 px and
  the ship within 1e-6 degrees of its ground after two seconds at x16, no plate begun again; Enter lets go and the view
  is still held; stowed, the lap goes on; a second of Shift+Right turns the parking twenty degrees east with the world
  kept in the window, a second of Shift+Up raises the latitude twenty degrees; opened after the turn the telescope holds
  the side faced), the flow's `orbit_turn`.
* Docs: 07 (the parking, the telescope bullet, the HUD line), 08, 10-decisions (a section of its own; the ground hold's
  bullet marked superseded), KI-356.
