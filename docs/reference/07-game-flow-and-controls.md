# Game flow, controls, persistence

<!-- contents -->
Contents:
- States (`GameState`)
- The frame (`Game::frame`, game.cpp)
- Ship (`ShipState`)
- Landing map (`LANDING_MAP`)
- Surface controls
- HUD
- The almanac (W-01, 2026-10-06)
- The cabin (M2)
- The GUIDE (`guide.txt`, M3)
- Settings (`vesperis_settings.txt`, M0-02)
- Key map (M6-02)
- The controller (R-418, 2026-10-17; M6-02's fixed table replaced)
- Photo tools (M6-05)
- Auto-explore (R-419, 2026-10-18)
- Title, attract mode and credits (M6-06)
- Saves (M0-04)
- Audio state
<!-- /contents -->

## States (`GameState`)

```
TITLE -> SPACE (Enter, or SURFACE when a loaded save was on the surface) ; N starts a new expedition (G-02: parked at the first living world of the pinned home star, `HOME_SX/SY/SZ` in `galaxy/starfield.h`, Heilya at sector 195 0 46; the search of sectors 176..196 x 36..56 for a yellow or orange star with a living world is the fallback when a generation change takes it away)
SPACE -> LANDING_MAP (C, parked at a landable body) -> DESCENT (Enter) -> SURFACE -> ASCENT (Q/E at the capsule) -> SPACE
SPACE/SURFACE -> HELP (H, ?, F1), MENU (Esc), DATA (I, F2); SPACE -> SYSTEM_LIST (Tab); MENU -> SETTINGS, SLOTS (save/load pickers)
SPACE -> SHARDS (C-06: the shard decoder, from the cabin's back wall, the flight computer's preferences page or the guide menu; the menu refuses it on the surface; C-04: a piece of music plays there and `N` names it through TEXT_ENTRY, which returns to the decoder; C-05: a text shard speaks there; C-11: `Tab` at its worlds turns to the ship's player)
SPACE -> INSTRUMENTS (C-11: the instruments' case on the cabin's left wall, or the flight computer's devices page: the instruments taken from the ruins, played there; C-18: it opens on its worlds, a folder each people of each, Enter opens one, Esc back to the worlds, Esc again returns to SPACE)
SPACE -> DRONES (C-18: the drone bay's screen, from its rack on the cabin's left wall by the front window or the flight computer's devices page; Enter sends the drones and returns to SPACE, Esc or E returns to SPACE)
SPACE/SURFACE/LANDING_MAP -> ALMANAC (W-01: Ctrl+T; DATA -> ALMANAC with T; the flight computer's devices page); Esc returns where it came from, Enter starts a run to the row and returns to the scene
SPACE -> PROBE (X-01: C parked at a giant launches a probe and shows its camera; C, E at the landing screen or the cage watch a probe that falls; Esc, E or C go back to the cabin, the probe falling on; at its end Enter or Esc let it go)
GALLERY -> RECORDING (X-04: Enter on a probe's recording plays it on the ship's screen; Esc or Enter back to the gallery; on the ground the gallery says the recordings play on the ship)
MENU -> AUTO (R-419: AUTO-EXPLORE, the menu's second row; the controller's commands' general page too) -> SPACE or SURFACE playing on its own (Enter); Esc back; any key, click, mouse motion or button gives the controls back (R-420: but `M`, a controller's `SELECT`, starts and stops its tape)
```

`returnState` remembers where an overlay came from and is set explicitly by every overlay entry (B-006). Only SPACE, SURFACE, DESCENT, ASCENT and LANDING_MAP advance game time (and the ship's screens, the decoder and, since X-01, the probe's camera).

## The frame (`Game::frame`, game.cpp)

`padFrame` → auto-explore's input → `tapeUpdate` → `handleGlobalKeys` (not while `ST_TYPING`) → screens' sounds stopped off
their states, the cameras off on `ST_STOWS` → `noteVisit` → the clock (`ST_SIMULATES`: time warp, the almanac) → autosave →
`updateProbe`, `updateDrones` → the update `switch (state)` → `probeAudio`, `darkRadio`, `updatePlayer` → the render `switch`
(a scene's case; `default`, the `ST_OVERLAY` states, draws `returnState`'s scene first) → `autoRender` or `renderPad`. Then the
platform pumps the synth (`platformAudioPump`), which reads the `AudioState` pointers on the main thread.

## Ship (`ShipState`)

Modes: `STANDBY` (free in a system or deep space), `VIMANA` (interstellar flight), `APPROACH` (fine approach), `PARKED` (following a body, optionally orbiting).

* Remote target: `R` toggles crosshair mode; the star nearest to the crosshair within 14 px is named live; Enter/click locks it (`lockRemoteTarget`). The current star is refused. `N` turns the ship to the next nearest star and (G-04) faces the cabin view forward as `X` does, so the lock finds it: in the cabin the view kept its own yaw and `Enter` missed the star the ship pointed at. While aiming the star field is amplified x1.5, the six nearest stars carry diamond markers with their distance (`4.2 LY`; R-407: the name only where the star has one, so no row of `UNKNOWN`s, and `VISITED` in amber where the ship has been: `aimStarLabel`), and `N` cycles the crosshair through them (`cycleTargetStar`); the star under the crosshair, the remote target's line and the radar's lock say `VISITED` too, and a key line `N NEXT STAR  ENTER LOCKS  DIGITS JUMP  R CANCELS` holds the bottom (the local target's label is left out meanwhile). **The jump** (R-407, 2026-10-07): the digits typed in the aim are a jump's light years (`jumpDigits`, 1 to 100,000; Backspace takes one back); a panel shows `JUMP 400 LY COREWARD` (`galacticHeading`: COREWARD, RIMWARD, ALONG THE DISC, NORTH or SOUTH OUT OF THE DISC), where the line's end lies (`END: SPIRAL ARM - 13104 LY FROM THE CORE`) and `ENTER JUMPS  BACKSPACE  R CANCELS`; Enter (`startJump`) makes the star nearest the end of the line along the crosshair (`aimDirection`, the view's forward) the remote target (`jumpTarget` in `galaxy/starfield.h`, `04-galaxy-and-systems.md`) and flies at once (`VIMANA JUMP OF 400 LY: A STAR 402 LY AWAY`; where the end lies in the void, `NO STARS OUT THERE - THE JUMP ENDS 580 LY SHORT`), logging `JUMP 400 LY COREWARD FROM <star> TO PARSIS ..., SPIRAL ARM`. `F` toggles the field amplificator at any time (M1-10); arrival from a Vimana flight flashes the palette (`arrivalFlash`).
* Vimana: `V` starts a flight (`flightFrom`, `flightTo`, real-time duration `vimanaSeconds`: `7 + 2 sqrt(LY)` to 100 LY, then 6 s more for every doubling since R-407: 400 LY in 39 s, 10,000 in 67, 100,000 in 87); the system is unloaded (`sys.valid = false`) during flight; arrival regenerates the target system and parks the ship in standby facing the star. `V` again aborts and reloads the nearest star's system. **The crossing** (G-07, 2026-10-08): the galaxy's band is drawn in the flight, from where the ship is along its line (the bulge swells ahead on a jump coreward until it fills the windows; on a jump north the disc drops away below the side windows), the streaks fading into it; the star-forming regions' patches fade with the flight's speed (half at 15 ly a second), so they show as the ship sets off and arrives and not as one-frame flashes on the way. A flight within 0.6 degrees of straight out of the disc has no heading, so the ship turns its right side to the galactic centre (the floor, the one side without a window, faced it by chance before). How the band is built ahead of the ship: `04-galaxy-and-systems.md` (G-07).
* Local target: `L` picks the body under the crosshair (within 18 px of its disc) or cycles; click also picks; `Tab` opens the list (`Enter` approaches, `L` targets). `X` centres the view on it.
* Approach: `startApproach(body)` chooses a parking direction on the lit side, offset sideways and slightly above the orbital plane; `updateShipMotion` moves with smoothstep easing and turns the view. R-419 (2026-10-18): the path is taken from the world (`ShipState::approachDir0/approachD0`, the start's direction and distance from it): the distance closed by equal ratios and the direction swung from the start's onto the parking's in the approach's first seven tenths (a slerp; straight across the world, round it by its spin axis's side), so the last stretch runs straight in and the world holds still in the window as the ship arrives; the straight line to a parking beside the world had swooped the view through 60 to 350 degrees a second in its last tenth of a second. The start, the end, the duration and the nose's aim are as before; the world's own motion is carried (a comet's). A belt's rock the same.
* Parked: position = body position + `parkDir * parkDist`; `O` toggles orbiting (slow rotation of `parkDir` about the spin axis). R-405 (2026-10-06): with `Shift` held the arrows carry the parked ship round its world (`Game::orbitShip`: Left/Right east and west along its parallel, about the spin axis; Up/Down north and south along its meridian, within 85 degrees of the poles; twenty degrees a second, a lap in eighteen seconds), the ship's attitude turning with it so the world stays where it was in the window and its ground streams past, the status reading the ground under the ship (`OVER 12.3° N  134.5° E`); the lap of the orbit mode goes on meanwhile. While the eye is at the telescope the stabiliser holds the parking in the body's frame (the telescope bullet below). `parkDist` comes from `Game::parkDistanceFor`: 3.5 radii, outside the rings for ringed worlds, 12 nucleus radii for a comet (N0-01: the nucleus is 76 px across at 2x with its jets around it).
* Belts (O3, R-302): `ShipState::targetBelt` is the local target when it is a belt (`localTarget` is then -1), `parkedBelt` the belt the ship is parked in, and `rockIr/rockIa/rockIy/rockM` + `rockDist` name the rock it parks beside (`04-galaxy-and-systems.md`). `L` cycles the bodies and then the belts, the analyzer lists the belts after the bodies (selectable), `Enter` calls `startApproachBelt`: `pickBeltRock` keeps the ship's angle round the star, clamps its radius into the belt and takes the biggest rock within two cells of that anchor; the parking point is six rock radii (at least 2 km) off on the sunlit side, a little above (`beltParkPos`), and the parked ship follows it (mode name `IN THE BELT`). `X` aims at the rock from this frame's parking point (a parked ship moves a hundred kilometres per frame at x10). Arrival logs a `BELT` entry. `C` in a belt says there is no world under the ship. `localTargetLabel()` names either kind of target; a body's approach (`startApproach`) leaves the belt, both its target and its parking (B-410: the belt's parking won at the approach's end and put the ship back beside its rock), and the flight computer's next body clears the belt target. Saved as `belt <target> <parked> <ir> <ia> <iy> <m> <rockDist>` (a save with a body's and a belt's parking loads at the body, B-410).
* Signal radar (C-07, 2026-10-02, `game/radar.cpp`, `peoples/signals.h`, `civilisations/radar.md`): `B` in space (the radar set beside
  the decoder on the cabin's back wall, `facing` 8, or the flight computer's devices page) enters the radar camera, the ship's own
  instrument seen through like the vehicles' nose cameras: the cabin is left behind (the head faces forward, `drawCabin` is
  skipped), the mouse turns the ship (`updateCabin` is not run), the picture is the camera's (`radarCameraFeed`: a cold blue-green
  phosphor, scanlines, a rolling band, a vignette, static on every pixel and snow in flecks that thin with the clarity), and
  `B`/`Esc` leave it. It listens where the camera looks (`viewBasis()`). The far signals are scanned once per sector of the ship
  (`signalsNear`: the transmitter sectors within 200 ly and the loud pulsars within 25, under a millisecond, the current star's
  own left to the local list), the system's own every frame (`localSignals`: a comet within 1.5 AU of its sun, a giant's or a
  strong big world's magnetosphere, and the people's world of a transmitter system). The meter reads the sum of the beam's gains
  over the static (`beamGain`: a wide lobe within forty degrees, a middle within ten, a narrow within three, times the signal's
  strength by its distance; the static a slow wander plus a jitter of about 0.07); a hold within three degrees of the strongest
  for `2.5 + 3 (1 - strength)` seconds locks (`radarLock`: a `SIGNAL` log entry for a new source naming only the system (the star,
  its class code, the distance and age, the sector; a body's name for a signal of this system), `Guide::signals`, a beep), the
  lock holds while the beam stays within eight degrees (lost after 1.5 s outside, `LOCK LOST`), and `Enter` on a lock locks the
  star and flies (`radarAccept`: `setRemoteStar` then `toggleVimana`, which leaves the camera; a body of this system: the camera
  left and `startApproach`). What sent the signal is never told: it is heard, and found on arrival. A people's world within the
  middle lobe is heard before the lock: its slot on the air (`transmittedShard`: the fifty walked in a stride of the world's own,
  one a slot of 75 s of game time; the next when it ends; C-13: on a world of two peoples the slot's people is a coin of the
  slot's, the world built again when the air changes hands) is built as a `ShardWorld` of the radar's own (`buildShardWorld`,
  shared with the decoder) and put on the synth through the receiver (`AudioState::radio`) as the slot's programme
  (`programmeFor`: the recording spoken, whispered, chanted or looped, a numbers station, a machine of the world's, or rarely its
  piece slowed; a loop's and a numbers station's repeats run on the game's clock, `radarRepeatContent`); what is heard while
  locked goes into `Guide::heard` when the programme carried the recording, and a shard taken later that the radar caught says so.
  The radar is off during a Vimana flight, at an arrival (the scan is fresh there), on the decoder (which takes the synth), on the
  surface and at a load; the other screens pause the sweep and keep the sound. `testAimAtSignal`, `testRadarInfo` for the harness.
  **The gain** (C-15, 2026-10-08, `civilisations/radar.md`): `+`/`-`, the wheel and `PgUp`/`PgDn` (the telescope's keys; the two are
  never out together) step the gain x1, x5, x11, x18, x25, x32 (`radarGainStep`, `RECEIVER GAIN X11` on the status, `GAIN X11` by
  the scope, the key line `+/- GAIN  B / ESC LEAVES THE CAMERA` at the telescope's place); the dial is kept while the radar is
  off, x1 again with a new game or a load. Over x1 the far scan of the ship's sector runs once, 40,000 cells a frame
  (`radarDeepStep`, `SignalScan`; `LISTENING DEEPER...` on the readout), and the list then holds what the gain carries from beyond
  200 ly (`needGain` within the dial: a step is a filter). The meter's floor climbs with the gain (`0.07 + 0.30 t` with t from 0
  at x1 to 1 at x32, the wander and the jitter wider) and its top is soft over x1; a near or a local signal reads its strength
  times `G^0.1` and drives the receiver by `G^0.4` (`signalLevel`, `signalDrive`): with the hold's signal driving it at 1 or over
  the meter pins red, `OVERLOAD` by the scope and `OVERLOADED - TURN THE GAIN DOWN`, no hold, and `AudioState::radarDrive`
  overdrives the sound. The hold is on the loudest signal within the lock cone and lasts
  `(2.5 + 3 (1 - strength)) (1 + 0.25 log2 G)` seconds; the lock cone is `3 / (1 + 0.1 log2 G)` degrees and the keep cone
  `8 / (1 + 0.2 log2 G)`; a gain turned down under a locked signal loses it (`LOCK LOST - TOO FAINT AT THIS GAIN`). The readout
  writes thousands of light years with their commas, the sector line dropping `THE SIGNAL IS` where the line would pass 52
  characters. **What the gain lets in** (C-16, 2026-10-09, `civilisations/radar.md`): the receiver is told the dial
  (`AudioState::radarGainT`), the band's brightness round the beam (`radarSkyAt`, `radarSky`: the floor 0.12 higher toward the
  core at x32, a far signal's clarity a third lower there), how worn the signal heard is (`radarWear`, by its distance beyond 200
  ly) and the age on the air (`radarAge`); above x1 the loudest other signal within forty degrees is heard under the one centred
  (`RadarState::under`, `radarKind2` and its level, the heterodyne `radarHetHz`/`radarHet` between two peoples' carriers and the
  intermodulation `radarIntermod` at the top steps). At x25 and x32 the static's stranger events join, a word of a recording in
  `Guide::heard` comes back while nothing is on the air (`radarWordStep`, `radarGhostWord`, never logged), and false rises come
  8-30 degrees off the beam every 12-45 s (`radarRiseStep`): held, the hold fills part way and empties, `NOTHING THERE` on the
  readout for 2.5 s and `SWEEP` by the scope, never a lock. The camera's static and snow thicken with the gain, the band glows
  through the phosphor, the stars ghost a double, the rolling band runs faster (`RadarState::rollPh`) and the picture tears now
  and then. Nothing is printed of any of it.
* Telescope (W-06, 2026-10-06, `game/telescope.cpp`): `Z` in space (the devices page too) puts the eye to the telescope: the view
  is the ship's own attitude magnified by a power of two, x2 to x1024 (`Game::telescopeProj`: the setting's focal length
  multiplied; the field `2 atan(tan(fov / 2) / power)`, 38.6 deg at x2 and 0.08 at x1024 for the 70 deg setting), the cabin left
  behind as in the radar camera (`cabin.yaw/pitch` zeroed, `drawCabin` skipped, the stars gathered x1.5 as the field amplificator
  does); `+`/`-`, the wheel and PgUp/PgDn step the power; the mouse and the arrows steer the ship at the rate divided by the
  power. The reticle names what it rests on (`telescopeFindTarget`, every frame from the attitude: a body or the sun within 18 px
  of its disc, else a star of the neighbourhood within 14 px, the local target's and the aim's own picks, so the pick shrinks in
  angle as the power grows): `<name>  <type>  <range>  <width>` (the explorer's name or UNKNOWN, `ROCKY MOON`, the range to the
  surface, the apparent width in degrees, minutes or seconds of arc; the sun by its class, a star with its light years). Enter
  tracks the body under the reticle (`telescopeAim`: the ship re-aimed at it every frame, the mouse moving the view's offset from
  its centre) and lets go again; from an orbiting parking everything but the parked body sweeps the field in a second, so the
  telescope tracks what it is pointed at from the moment it opens (`TRACKING <name>`). `N` names the target through the guide's
  text entry (`NAME THIS WORLD` / `NAME THIS MOON` / `NAME THE COMET` / `NAME THE SECOND SUN`, kind 2, the status and the log
  naming the kind; `NAME THIS STAR`, kind 1; a star of the neighbourhood `NAME THE STAR`, kind 8 through `textStar`); `P`
  photographs it through the usual screenshot path with `THROUGH THE TELESCOPE X64: <the reticle's line> - IN ORBIT OF ...` as the
  caption; `Z` or Esc stow it. It is stowed on a load (not saved), when a Vimana flight starts, off the window (the surface, the
  landing map, the decoder, the sector map) and when the radar comes on (one instrument at the window at a time; the telescope
  switches the radar off likewise). The telescope's target (tracked, else under the reticle) takes palette bank 2 whatever its
  size (`choosePaletteBodies`), so a planet across the system is drawn in its colours; Enter does not start an approach while the
  telescope is out; the local target's bracket is not drawn in the eyepiece (the reticle names things), its line stays; the flight
  computer's devices page shows `X64` or `STOWED` beside `THE TELESCOPE`. **The plate** (the user's review of 2026-10-06, "a poor
  zoom in and that's it": `DetailPlate` in `space/space_view.h`, `02-rendering.md`): the eyepiece's body (tracked, else under the
  reticle: `telescopePlateBody`) is drawn from the planet function sampled at the eyepiece's own footprint, not from the world
  map's 512 texels: a body-fixed patch round the aim point, built in blocks of 8, 4, 2 and 1 texels within 8000 samples a frame
  (`TELE_PLATE_BUDGET`: a few milliseconds on the workers; coarse at once, whole within a second or two), read by the globe
  wherever a cell is filled with the map standing in elsewhere, the previous plate kept as the fallback while a zoom step or a
  drift begins a new one; the rivers' and lakes' tiles are fetched in the background for the steps under 2 km and the plate begun
  again when they are in. The frame says `EXPOSING 46%` while it builds; `P` reads it out whole before the photo
  (`telePlateFinish`, a short stall at most). **The stabiliser** (R-405, the user's review of 2026-10-06, "planet still moves and
  it creates shaking ... it should feel like it's not moving at all": `tele.frameBody`, `parkBF`, `viewBF`,
  `Game::telescopeHoldFrame`/`telescopeHoldView`/`telescopeStabilised`): while the eye is at the eyepiece a ship parked at a body
  holds its parking direction and the view's axis in the body's frame (both taken when the telescope opens, or when the ship
  arrives at a parking with it out; the view's axis taken again after every input), so the lap of the orbit mode is held off and
  the world's turn carries the ship with it: at any power the ground under the reticle stays where it is until the mouse or the
  arrows move it, and the plate is never begun again by a drift. Opening the telescope keeps the attitude (the side faced is the
  side held: `STABILISED ON <name>` on the frame and the status), Enter on the parked body brings its centre under the reticle and
  holds again; another body or the sun is tracked on its centre as before (`TRACKING <name>`: a moon moves by its own orbit,
  slowly); off a parking (standby, the approach) nothing is held. Stowing lets the lap go on from where the ship is. (It replaced
  the rework's ground hold of a deep power the same day.) The telescope's own set stands on the cabin's back wall (`facing` 9, x
  1.0..2.1, the radar's twin; `E` looks through it, `TELESCOPE - E STOWS IT` while it is out) and the fuel orb moved to the
  front-left corner.
* The probe (X-01, 2026-10-07, `game/probe.cpp`, `galaxy/probe.h`, `space/probe_view.h`): parked at a gas giant or a brown dwarf,
  `C` (the landing screen on the right wall and the capsule cage, `PROBE SCREEN - E TO LAUNCH A PROBE` / `PROBE BAY - ...` there;
  the flight computer's `DEPLOY THE CAPSULE OR A PROBE`) sends a probe to the ground under the telescope's reticle when the
  telescope is out (`C SENDS A PROBE TO 5.1° N 97.8° W` on its frame), else to the ground under the ship
  (`PROBE AWAY, UNDER THE SHIP (THE TELESCOPE AIMS IT)`), and the screen becomes its camera (`GameState::PROBE`): the fall from
  the ship (36 s, the giant growing, the camera on the aim point and never steeper than a little under the limb), the entry (14 s
  of plasma, the speed and the g falling), then the descent through the stages of `galaxy/probe.h` (above the clouds, the upper
  haze, then the giant's one to three decks named by their clouds with the clear bands between them, the deep) to the end, the
  hull's limit (18-32 bar, per probe) or the heat (750 K), seven or eight minutes falling free. The relay's clock is real time:
  the warp turns the sky (the sun crosses the deck) and not the fall; `Shift` held on the probe's screen runs it eight times as
  fast (R-406, `FAST` in amber after the stage's name; let go, or away from the screen, it runs at one again). The mouse and the
  arrows turn the camera from its default (toward the aim in the fall, then a little aside of the sun's azimuth, down at the deck
  from high over it (22 degrees), less steeply in the haze, level in the decks and between them), `X` centres it; `Space` opens
  the chute once after the entry (the pressure grows four and a half times under it at a quarter of the pace: a layer to linger
  in, about four minutes; `Space` again cuts it), `P` photographs
  (`THE PROBE IN <giant>: THE UPPER HAZE, 0.32 ATM, ALT 53.4 KM - ...`), `Ctrl+R` records the feed, `Ctrl+S` saves; `Esc`, `E` or
  `C` go back to the cabin with the probe falling on (`PROBE: THE UPPER HAZE  0.32 ATM - C WATCHES` on the space HUD, the landing
  screen breathing, the relay murmuring), `C` or `E` at the landing screen or the cage come back to it. At the end the last frame
  is held under the loss-of-signal hiss (`SIGNAL LOST`, the depth, the pressure, the temperature, the minutes since the launch,
  `THE HULL GAVE WAY` or `THE HEAT TOOK IT`); away from the screen the status says
  `PROBE LOST - ITS LAST FRAME IS ON THE SCREEN (C)` and the frame waits until it is seen; `Enter` or `Esc` let it go. The ship
  holds its parking meanwhile: leaving it (an approach, a Vimana flight, another star) cuts the relay
  (`THE PROBE'S RELAY IS CUT: THE SHIP LEFT ITS PARKING`). Unlimited probes (the user's answer at X-01's start), one falling at a
  time (`C` at the giant watches the falling one). Log: `PROBE` at the launch (`INTO <giant> AT <lat lon>`), at the end
  (`LOST IN <giant>: 285 KM DOWN, 23.6 ATM, +477 C, 11 MIN - THE HEAT TOOK IT`) and at a cut
  (`RELAY TO THE PROBE IN <giant> CUT AT 0.32 ATM: THE SHIP LEFT ITS PARKING`), each within the log screen's two wrapped lines of
  44 with a name of twenty. X-02 renders the stages (`space/probe_view.h`, 02-rendering): over the clouds the ammonia deck to its
  curved horizon under a dark sky with the thin bright limb, the stars, the sun, the moons and the rings' arc over it, its towers
  casting their shadows, the terminator crossing it at dusk; the upper haze's amber with the cirrus racing along the jet in the
  band's colour and the towers rising round the probe; inside the ammonia deck the fog with the light coming down through it, its
  caverns and layers; between the decks the ceiling's pouches over the water deck's towers, the lightning lighting a tower from
  within (now and then a channel up through the clear air) and the whole band for an instant; the water deck's dark with the rain
  and the lamp's cone; the deep's motes in the lamp and its dull red heat near the end. X-03 gives each giant its own
  (`galaxy/probe.h`'s character, 04 and the decisions log): its decks by condensation, named on the screen by their clouds
  (`THE METHANE DECK`, `THE SULPHIDE DECK`, `THE AMMONIA DECK`, `THE HYDROSULPHIDE DECK`, `THE WATER DECK`, `THE SALT DECK`), the
  clear band coming twice under three decks; its storms, lightning (in the convective decks; an ice giant's fall the quieter, its
  wind's sound softer too), the deep's glow (a young giant's dull red, a brown dwarf's clouds lit from under them, on the night
  side too), the polar aurora's curtains over the clouds, a great storm's wall, a clear-air hole, diamond hail in the lamp, and on
  one rare giant something seen once in the deep that the screen never names. The camera's default turns to what was aimed at: a
  great storm's wall when the aim is within 2,500 km of its rim, else the aurora's curtains within 2,800 km of the oval; falling
  into a clear-air hole it looks down into it and further down the shaft. The probe's HUD names the layer its picture shows
  (inside a great storm's dome the deck's name though the pressure says the haze; `A CLEAR-AIR HOLE` in one); the cabin's line and
  the photograph's caption name the pressure's. X-04 keeps the record. The screen's last line:
  `MOUSE LOOKS  SHIFT FAST  P PHOTO  ESC CABIN` (`X` still centres).
* What a probe sends back (X-04, 2026-10-07, `game/probe_records.cpp`, `game/guide.*`, `galaxy/probe.*`). Every probe has its number
  (`Guide::probesSent`, `PROBE 3` in its lines). At its end, or when the relay is cut, its record goes into the guide
  (`ProbeRecord` in `Guide::probes`): the launch's numbers (the giant's key, the aim, the seed, the ship's place, the headings,
  the chute's clocks, the hull's limit), the explorer's turns of its camera and the game's clock along the fall as keys (a turn of
  half a degree, a drift of half a second off the pace), the end (its clock, pressure, depth, temperature, cause: 1 the hull, 2
  the heat, 3 the relay cut), the storm it fell into, and the profile it measured: a sample every quarter of an e-fold of the
  pressure from the clouds' top (the clock, the pressure, the temperature, the wind as the screen read it, the light: the sun's at
  the depth by its height over the aim's horizon and the deep's glow, the layer found there, the flashes since the sample over it)
  and the layers it found with their bounds (`giantFoundLayer` on `GiantDecks::layersAt`, the view's own: a deck by its cloud, a
  clear band, the deep, `A CLEAR-AIR HOLE`). Its final image is drawn then (the frame the screen holds) and written as
  `shots/probe_<n>.png` with a `.txt` caption. A probe ended again after a load replaces its own record (the seed). The log keeps
  what it crossed, once each with the depth: `PROBE 3 FELL INTO THE GREAT STORM OF <giant> AT 0.21 ATM, 35 KM UP` (or `A STORM`,
  or the storm's name), `SAW THE WALL OF THE GREAT STORM ..., 799 KM OFF`, `SAW THE AURORA'S CURTAINS OVER ..., 1200 KM OFF`,
  `FELL THROUGH A CLEAR-AIR HOLE IN <giant>'S AMMONIA DECK, ...`, `MET DIAMOND HAIL IN ... AT 9.86 ATM, 182 KM DOWN`; the rare
  sight is never logged. `N` on the probe's screen names the storm it falls into once it is in the clouds (text entry kind 9: the
  great storm's key `<body>/TG`, a storm cell's `<body>/T<id>` from the globe's own cell, `giantStormCell`, at the aim and the
  bands' time); the name shows under the giant's at the screen's right (`THE GREAT STORM` / `A STORM` unnamed) and the keys' line
  reads `N NAMES THE STORM  SHIFT FAST  P PHOTO  ESC CABIN`. The gallery holds the recordings beside the photographs in the order
  they came (a photograph by its file's time, a recording by its end's), framed amber (a lent one cyan, its last frame drawn when
  it is shown); `ENTER PLAYS`. The recording (`GameState::RECORDING`, not simulating: the game's clock and a falling probe wait)
  is the descent drawn again from the record by the probe's own functions (its probe, and another star's system and sky, swapped
  in for the call): `PROBE 3  T+04:31` with `PLAYING` or `PAUSED`, the stage and the readouts as the live screen's, the storm, and
  a depth bar on the right (the fall and the entry over its first tenth, then the pressure's logarithm, the layers found in their
  colours, the playhead); Space pauses (at the end plays again), Left/Right go up and down it (a press a fiftieth, held a quarter
  of it a second, Shift four times as far), Up/Down and Page Up/Down to the layers' tops, Home/End, Shift held plays it eight
  times as fast, the mouse looks round the recording's camera (`X` back to it), `N` names the storm, Esc or Enter back to the
  gallery; at its end the last frame under the hiss, `SIGNAL LOST` (or `RELAY CUT`). The giant's data sheet gains a page per probe
  (Right/Left, `T THE ALMANAC  RIGHT PROFILES  ANY OTHER KEY CLOSES`): `PROBE 3 INTO <giant>`, the aim or the storm and the date,
  the end; the pressure down the page from 0.001 atm to the deepest end of the giant's probes, the layers found (a strip in their
  colours, named where there is room), the temperature, the wind (east right of the dotted zero), the light (its logarithm, `DARK`
  to `SUN`) and the lightning's rate (ticks right of it); the giant's other probes dim under it on the same scales. The
  statistics: `PROBES SENT 3   DEEPEST DESCENT 412 KM` under the creatures' line.
* A drifter (D-01, 2026-10-23, `galaxy/drifters.h`, `04-galaxy-and-systems.md`): a place as a star is, reached by the same paths. Every key
  that names a place goes through one resolver, `placeInSector` (the sector's star, else its drifter): the save's `star` and `remote`
  lines, targeting by name and by sector coordinates, the previous star and home, the radar's lock and accept, the arrival, the history's
  regeneration, the probe's replay, the shards', the player's and the instruments' screens, the galaxy chart's keys; the star field, the
  neighbourhood (the aim, the star map, `N`, the console's `SL`), the jumps and the new game stay the stars' (`starInSector`), so a drifter
  is never seen, only reached. The remote's line reads `REMOTE TARGET: UNKNOWN - NO STAR - 416.40 LY - V TO FLY` (no class code). The
  Vimana flight ends, and the arrival puts the ship, 40-80 of the drifter's radii off its centre (`drifterStandOffKm`, by its seed: the
  stars' rule, a first orbit or 25 radii, would put it at the centre), facing it; the arrival reads `ARRIVED AT UNKNOWN - NO STAR, ONE WORLD,
  1 MOON` and logs `UNKNOWN, NO STAR, ONE WORLD 1 MOON, SECTOR ...`. The analyzer's head and the data sheet read `NO STAR`
  (`StarSystem::classString`), the sheet's star section `STAR NONE` and the class's sentence (`describeStar`: no star, the world adrifts
  alone, warmed by nothing but its own heat), the drifter's lines `OWN HEAT 36 K (-237 C)` for the equilibrium, `ORBIT NONE - ADRIFT
  BETWEEN THE STARS`, `PERIOD NONE`, its moons' orbits from their planet, and on its ground `SEASON NONE - NO SUN, NO DAY` (no season's
  name, there or on the landing map); the classes seen leave it out (`STAR CLASSES SEEN n/14`); no radiation warning (no class of its
  own in the table). D-02 (2026-10-23, `civilisations/radar.md`, the drifters on the radar): the radar hears it above x1, by a law of its own;
  a lock reads `NO STAR AT THE SOURCE`, its distance and `ENTER FLIES THERE` (no name, no class, no sector), the status `A SIGNAL LOCKED -
  NO STAR THERE - ENTER FLIES`, the log `A SIGNAL WITH NO STAR AT ITS SOURCE, 437 LY (437 YEARS OLD), SECTOR ...`, and the lock's key is the
  sector's, so the galaxy chart's red ring and its `A SIGNAL` stop stand at the place. On the chart `Enter` plots the jump to a drifter the
  explorer knows (a lock, a visit, a name, home) within four pixels of the cursor (`chartDrifterAt`: a jump's line ends at the star nearest
  its end and never sees a drifter); the flight's status reads `VIMANA JUMP OF 437 LY: NO STAR AT ITS END`, `N`'s stop for a visited one
  `VISITED - NO STAR`. The arrival makes the drifter the local target (the HUD's brackets on the black disc, `LOCAL: UNKNOWN 3.42 MKM`); the
  HUD's remote line reads `NO STAR` for a class; the guide's second item there is `NAME THIS PLACE` (`THE PLACE IS NOW CALLED ...`, the log's
  `NAMED THE PLACE WITH NO STAR ...`), so it is a place in the guide as a system is, the explorer's to name. D-03 (`civilisations/radar.md`, the
  drifters' voices): its voice on the radar, on the cabin's radio while the ship is at it and on the suit's on its ground (the audio below).
* `choosePaletteBodies` assigns palette banks 2/3 to the two bodies with the largest apparent size.

## Landing map (`LANDING_MAP`)

F-02 (2026-10-13, the user's choice): on a world whose map is not made yet (a first opening after an arrival; the map is made on its
thread, 150-200 ms) the screen opens at once and reads the world, its frame empty with `READING THE WORLD...`, until the map's thread
has made it; the keys work meanwhile as they did after the wait it replaced (`Enter` lands at the cursor), `R` waits for the map. The
drone bay's map likewise (`THE MAP IS BEING DRAWN`).

Shows the body's `PlanetMap` through `buildLandingMapBase` (N1-04:
`materialRampColor(ramp[material], exposureStop(albedo, light factor, atmosphere) x hillshade)` with the body's `matColor`, the
star tint and the body's noon sky light, so the map is the ground seen from above at noon; north-west hillshade normalised by the
map's RMS gradient; lava emissive; clouds at 55% on felisian/thin-atmosphere/quartz worlds, an opaque deck in the body's cloud
colour on venusian worlds) times the day/night term from the current sub-solar direction and, since O0-04 (B-306), the shadows on
the day side: the moons and the parent between a texel and the star and the world's own rings (`Game::buildShadowMap`, the globe's
own rule, 90% dark inside the umbra; rebuilt for another body, every 20 s of game time and at most twice a second of real time;
the zoom window gets the same), the cursor (arrows, Shift for 6 deg steps), `R` for a random lit land site, `C` toggles the cloud
overlay (W-03: the fronts' bands with it, from the globe's own overlay `SpaceRenderer::frontsFor`, on the map and in the zoom
window: a band is whitened at 60% of its cloud like the pattern's), `Enter` to land, `Esc` back. The readout (five lines at 8 px
pitch since N1): name, coordinates and sector; `MATERIAL +height RELIEF` with a `GROUND` swatch showing the noon ground colour at
the cursor from the same ramp (N1-04) and the relief class from `Game::siteSlope` (the mean grade over 150 m and 600 m around the
probe scaled to a gradient, O6-02; the steepest rise before; `reliefClass`: FLAT under 0.05, HILLS under 0.2, else MOUNTAINS,
N1-06); biome and season (M9-05/09) with the sun's altitude, azimuth and day/twilight/night; local time, day length, gravity,
atmosphere; `SKY: ...`. The data sheet adds a generated description of the body and the star (M9-14) and, on the surface, the
biome and season here (Q-01: `TRAITS` on up to two lines, the six slots' names, and `FACTS` on one, what the orbit and the spin
say: `SIDEWAYS SPIN, RINGED`; a sheet longer than the screen goes on over pages, sections kept whole where they fit, the world's
name and `2/2` over each, `RIGHT MORE`, then a giant's probes' profiles); the analyzer lists asteroid belts after the bodies; the
space HUD names the galaxy region. The readout uses a cached `SurfaceSite` probe at the cursor (F-02: `SurfaceSite::initFrame`,
the site without its five terrain caches, which it never read and which were filled at every step of the cursor): exact terrain
material and elevation at 64 m detail, sun altitude/azimuth, day/twilight/night, local time, day length (or "tidally locked"),
gravity, atmosphere. Sector numbers follow the original's 360 x 120 convention.

The survey (D-02): a world with no sun has no light to map by (the map went black at a light of 0), so the landing map, its zoom, the
sector map and the drone bay's map draw it as the ship's altimeter reads it (`mapColor`'s `survey`): the material's colour at the noon stop,
two parts in three the radar camera's cold phosphor, its brightness by the height (from the map's 2nd percentile, dark, to its 98th, pale:
`surveyRange`) times the hillshade, lava glowing; no night side, no clouds (`C` is not offered). The title reads `SURFACE CAPSULE - NO LIGHT:
THE ALTIMETER'S SURVEY`; the readout `NO SUN - THE DARK` for the sun's line, `TURNS IN 0.3 H` for the local time and the day, a sky body
`UNLIT` for its phase, and the `GROUND` swatch the noon colour (what a lamp will show). D-04: an air fallen as snow reads `AIR FROZEN` for
the atmosphere (the data sheet's `ATMOSPHERE  FROZEN`); the capsule comes down with its floodlights lit under 500 m by night (a wide cone
down and ahead), and lands `LANDED IN THE DARK - U THE LAMP, Q AT THE CAPSULE TO RETURN` on a drifter's world while the lamp is off.

`J` (M5-09) moves the cursor under the biggest body in this world's sky (the sub-body point, where it stands at the zenith); the fifth readout line says what hangs over the cursor now (`SKY: name, altitude, width, phase, +N PTS`); on a ringed world it starts with the ring's own arc (`RINGS 62 deg UP`, `RINGS EDGE-ON OVERHEAD` within 1.5 deg of the equator, `RINGS BELOW THE HORIZON`) and a ringed sky body reads `RINGED` in place of its phase (O0-01).

Sector zoom (O2, R-301): `Z` zooms into the cursor's sector (`Game::buildLandingZoom`): a window one sector (one degree of latitude) tall and two sectors' worth of ground wide (`zoomHalfLon = zoomHalfLat x 2 / cos(lat)`, the frame being 2:1), 256 x 128 texels sampled from the planet function at `clamp(0.6 texel, 64, 512)` m of detail on all threads (`parallelFor`), coloured like the whole-world map (`mapRampsFor`, `mapColor`: the body's material ramps under the star's light, `exposureStop`, a north-west hillshade normalised over the window and capped at a 3% grade) and lit by the day side per frame; the one-degree grid is drawn over it with the sectors' names at their centres, a scale bar (50, 20, 5 or 1 km) sits at the bottom right, the cursor steps a hundredth of a degree (five with Shift) and the window re-centres when the cursor passes 75% of its half-size; `Z` again returns to the world; worlds under 100 km of radius have no zoom. The first readout line prints the sector as `SECTOR LON:LAT` (`sectorName`, `03-coordinates-and-units.md`); the title says `SECTOR 319:062 - PICK THE SITE` while zoomed. The landing map also prints a comet's gravity in four decimals. C-09: on a world that had a civilisation the window's old roads are drawn over it as hairlines in the ground's own colour under the texel's light (`Game::zoomRoads`: the villages' and the towns' roads of the network of `peoples/roads.*` that the ground carries, the lanes between hamlets left out, walked for the window on the worker pool at the build), whether or not any of it has been walked; R-415: with the peoples (`V`), as the plan's cores (`zoomCivImg`), not on the land alone.

The peoples (R-415, 2026-10-11, `V`, off each time the map opens; `drawLandPeoples`): without them the map is the land, the clouds and the explorer's marks of the natural sights. With them: the explorer's marks of ruins and works as small points (a frame pixel's square at 1x, three at 2x, a dark rim; the names of the three nearest the cursor, within ten pixels, listed beside it), in the zoom the old roads and the plan's cores; the places the records the ship holds were kept at (each record's home, T-04's `recordHomesOf`, read with the world's items as the drone bay reads them, `droneWorldAsk` from the map's opening; `landKnownOf`): a town a point in the people's colour (the first amber, the second cyan), a city paler and rimmed, the capital white and named `CAPITAL`, in the zoom a ring round each known place's core; a people whose records are all held, its whole map: its land tinted to its frontier (`landTerr`, from the plan's territory per grid cell), every place of its plan and every way between them coloured by kind (`NET_KIND_COLOURS`: a path brown, a road pale, rail red, a canal blue, a highway orange, maglev magenta), in the zoom every place of the window ringed. The legend at the map's top left: each people with its records held of its count (`THE JENGNE: 21 OF 147 SHARDS`; `A PEOPLE` and `ANOTHER PEOPLE` until a record of theirs is held, as the decoder names them; `THE JENGNE: ALL 147 SHARDS, THE WHOLE MAP`), `READING THE WORLD...`, `NO PEOPLE LIVED ON THIS WORLD`. The hints fit the 53 characters now (R-415: they ran off both edges): `Z ZOOM  R RANDOM  C CLOUDS  J SKY  V PEOPLES  ENTER` (no `C CLOUDS` without an atmosphere: `... ENTER LAND`), zoomed `ARROWS 1 KM  Z WORLD  R RANDOM  V PEOPLES  ENTER` (Shift still steps five).

## Surface controls

Mouse look, `WASD` (arrows also turn/pitch), `Shift` sprint (stamina), `Space` jump and, held in the air, jetpack, `1`-`9`/`0`
autowalk, `C` crouch, `Z` stand tall, `Ctrl` dive while swimming, `B` unfold a buggy at the capsule (a new one each time; any
buggy already out is scrapped, R-204), `E` enter/leave the buggy, take a shard of the old people lying within 2.2 m (C-03:
`A SHARD HERE - E` on the HUD, T-04: the object, `A CODEX - E`, `A PAINTED WALL - E`, a painted wall recorded and left on its
wall; C-11: an instrument, `A LUTE - E`, the nearer of it and a record taken first, into the cabin's case; the guide, the log and
the statistics record it; it is read on the ship's decoder, C-06; `civilisations/`), or board the capsule (leaving reports
the trip and the top speed); inside the buggy the mouse pans the nose camera +-70 deg and tilts it -35..+25 (never backwards,
N4-06), `V` the outside chase view, `F` unfold a drone at the capsule (R-403, 2026-10-02: a new one each time, the old one
scrapped, like the buggy; none on a comet), `E` within 3.5 m gets in and, on the ground, out (the kilometres flown and the top
speed reported; `LAND FIRST - HOLD SHIFT TO COME DOWN` in the air); in the drone `Space` lifts off and climbs, `Shift` descends
and lands, `W`/`S` thrust fore and aft to 320 km/h, `A`/`D` turn, the mouse pans the nose camera +-70 deg and tilts it -60..+25,
`V` the chase view, a ceiling of 400 m over the ground, `M` waypoint (O1: on the spot under the crosshair when the rangefinder has
one, `WAYPOINT SET 8.4 KM AWAY`, else where you stand; `M` again clears), `Ctrl` in the air thrusts down on light worlds (O4),
`Q`/`E` board the capsule within 7 m (otherwise the status says how far it is and where), `Tab` the readings (R-414: the place,
the sky, the air, the ground and the vehicle, below under HUD), `K` recalls the capsule next to you (always; the status names a
buggy left over 50 m away, B-205), `N` sector map (M4-02/O2/O1-04: `+`/`-` or the wheel zoom through 2, 8, 32, 128, 512 and 2048
km wide (`Game::sectorMaxLevel`: a level is offered while its half-width stays within 1.2 radii, so a comet has two and a planet
six; the image is 100 x 80 samples of the planet function at 16 m to 16 km of detail, always centred on you, so walking never
leaves it and the neighbouring sectors are simply there), coloured like the landing map (`mapRampsFor`), the sector grid on the
levels from 32 km up (Q-02: on a world of great tides its flats wet sand whatever the tide, the high water's line pale and the low
water's dark: the landing map's zoom draws them too) (B-402: a step of 1, 2, 5, 10, 15 or 30 degrees keeping the lines 24 px
apart; the one-degree cells named where they are at least 46 px wide, the coarser lines labelled with their latitude and longitude
along the edges); R-408: a cursor (amber) moved by the arrows (Shift faster) and the mouse within the frame, `C` or `Space` back
to you; `Enter` (or a click) marks the place under it (the name's entry, below; on a mark it renames it), `Delete` (or Backspace)
twice removes the mark under it (`REMOVE <NAME>? DELETE AGAIN`, the mark blinking), `M` puts the waypoint at the cursor (on the
waypoint it clears it), `Tab` goes from mark to mark in the frame, `Shift+Enter` (or a click with `Shift`) on a mark shows it over
the view in place of any other and on the shown one hides it (B-412: `SHOWN OVER THE VIEW: <NAME>`, `NO MARK SHOWN OVER THE VIEW`;
on no mark `NO MARK UNDER THE CURSOR`), `R`/`Shift+R` work the scanner; the explorer's marks drawn with the glyph of what they
mark (a place a small diamond; a friend's in cyan), the names of the nearest twelve where they do not run into one another (B-412:
the shown mark boxed in its colour, its name first); the scanner's echoes of its mode as dots where they are heard (the nearest
30, a village or a town a bigger dot, the nearest boxed; the marked left out unless taken in); the panel: the width and
`+/- ZOOM N CLOSE`, the cursor's distance and way from you (`HERE`), its latitude and longitude, its sector and the mark (B-412:
`SHIFT+ENTER SHOWS` or `SHIFT+ENTER HIDES` under it; the scanner's lines a row lower) or the waypoint under it, the scanner's
mode, nearest echo and what it is (`A TOWN`, `1240 M, 380 OVER`, `2.3 KM ACROSS`) with `MARKED LEFT OUT` or `MARKED HEARD (*)`,
your elevation and the next front; `ENTER MARK  DEL REMOVE  M WAYPOINT  TAB NEXT  C HOME` under the map; `Esc`/`N` close (the
image is rebuilt when you move a tenth of the width, change the level, land or re-anchor: `siteEpoch`, B-303)), **the scanner**
(R-408, `game/marks.cpp`): `R` on the ground (in a vehicle too) cycles it through the kinds of sights the world can hold
(`sightKindsOf`): RUINS, WORKS (T-03: a people's works outside its towns, the nearest named by its kind, `A DAM`, `A BATTLEFIELD`,
`LINES ON THE GROUND`, `SOMETHING MADE`), LAKES, PEAKS, CRATERS, CANYONS, MESAS, GEYSERS, CRYSTALS, and on a world with Q-01's
landform CALDERAS, TESSERAE, LABYRINTHS, HOLLOWS, CHAINS, PENITENTES, COLONNADES, MUD VOLCANOS, RIDGES, PINGOS or MAARS (their
echoes `2524 M DEEP`, `41 M HIGH`, `BLADES OF ICE`, `BRIGHT PITS`, `1.9 KM ACROSS`; auto-explore stands on a caldera's, a chain
crater's or a maar's rim and near a cone's or a pingo's top), and on a world with Q-02's meteorite fields METEORITES
(`IRON STONES, 4.3 KM`: a strewn field), off; it hears every sight of the mode within 40 km (`06-surface.md`) and names the
nearest at the top left (`SCAN RUINS` / `1.6 KM NW`, `SCANNING...`, `NONE IN 40 KM`, `NONE ON THIS WORLD`, `HERE` within its
extent) with a diamond on the compass strip at its bearing (an arrow at the strip's end beyond sixty degrees); an echo is heard
off its place by up to 8% of its distance, 3 km at most, in a direction of its own (`echoHeardAt`), so the way comes first and the
place as it nears; `Shift+R` leaves out the sights the explorer marked (the default: one of the explorer's own marks within the
sight's extent, 150 m at least, and 250 m more) or takes them in (`*` after the mode), **marks** (R-408): `L` marks where the
crosshair rests (the rangefinder's spot, else the feet), the sector map's `Enter` the place under its cursor; the name's entry
(`NAME THE MARK`, text kind 6) shows what it marks (`MARKS # RUINS - TAB CHANGES`: the sight the scanner hears there, else
`A PLACE`), Enter places it (an empty name places an unnamed mark), Esc places nothing; a mark is a `Guide::marks` entry (`mark`
lines, below) shown on the sector map, the landing map (a dot) and its zoom (the glyph, eight names; R-415: a mark of ruins or
works only with the peoples, `V`, a small point and its name by the cursor), over the view the one the explorer shows from the
sector map (B-412, `Guide::shownMark`, a `shownmark` line in the guide; none by default, a new mark is not shown: a diamond over
the ground with the name and the distance wherever it stands on the world, and a hollow diamond on the compass strip at its
bearing, an arrow at the strip's end beyond sixty degrees, `drawShownMark`; R-408 drew the eight nearest within 25 km, their names
over one another when they stood in a line) and in the readings under the rangefinder when the crosshair rests within 150 m of one
(R-414; B-422: a mark's ground is read with the rivers only where their drainage tiles are built, a far one's without them until
the explorer's way builds them, `marksHere`: reading them built a tile of a degree a mark on the main thread, the readings', the
scanner's or a shown mark's first look after a landing frozen for 8 s on a world of nineteen marks) (O6-06's `UNNAMED PEAK` went
with the landmarks), crossing into another sector on foot or in the buggy shows `ENTERING SECTOR 226:113 - GRASS, GRASSLAND` and
logs a `SECTOR` entry (`lastSecX/Y`, set silently at landing and after a load), `V` on foot cycles the vision modes (M4-10:
radiation visor, supervision, infrared, plant vision), `U` on foot the explorer's helmet lamp (D-04, 2026-10-23:
`LAMP ON`/`LAMP OFF`; a cone of light along the view by night, on every night side as in a drifter's dark, nothing while the sun
is up; the pad's commands `LIGHT THE LAMP`/`PUT THE LAMP OUT`; kept on or off for the session; `06-surface.md`), `X` highlights
nearby creatures and flocks for four seconds with the species name and its state (M4-05, N3-06), `I`/`F2` data sheet, `T` time
warp (refused when the clock setting is real time; it stops an almanac run), `Ctrl+T` the almanac (W-01, 2026-10-06: the section
below; M5-06's 25 s time-lapse at x600 went with it, the almanac's free row runs for any span), `Ctrl+S`/`F5` save, `Ctrl+L`/`F9`
load, `H`/`F1` help, `Esc` menu, `P`/`F12` screenshot, `Ctrl+K`/`F10` scanlines, `Ctrl+F`/`Alt+Enter`/`F11` fullscreen. Movement
keys are ignored while `Ctrl` is held.

Key bindings (N5-05): the settings' last item, `CONTROLS` (`KEY BINDINGS` until R-418), opens the `KEYS` state on its keyboard's page (on the controller's while a controller is in use; `Tab` between them; the commands' `CONTROLLER SETUP` opens it from anywhere and `Esc` goes back there): 28 actions in two columns (R-403: `E` is `USE / VEHICLE`, `F` `FIELD AMP. / DRONE`; C-07: `B` `BUGGY / SIGNAL RADAR`; D-04: `U` `SHIP LIGHT / LAMP`) with the physical keys that act as them; arrows select, Enter captures the next raw key (`Game::rawKey`, handed by the platform layer before the key map is applied) and maps it to the action's logical key (`KeyMap::map[raw] = logical`; the action's own key keeps working), Backspace resets the action, Esc writes `vesperis_keys.txt` (`KeyMap::save`, only the pairs that differ) and returns; the file is still read at start-up as before. Global keys are ignored while capturing.

Key twins exist because Mac laptops route the top row to hardware functions unless `Fn` is held (B-002); `Game::macHints` (set by the platform layer on macOS) shows a one-time reminder after the title screen. The helpers `helpKey`, `dataKey`, `saveKey`, `loadKey`, `enterKey` in `game.cpp` define the pairs; `enterKey` excludes `Alt+Enter` (fullscreen).

## HUD

Space: system name and class, sector coordinates, EPOC, mode (standby / vimana / approach / orbit / chase / in the belt), remote
target line and marker box, local target line and bracket (label flips to the left near the edge), off-screen arrow, radiation
warning, crosshair, status line, time warp indicator (W-01: during an almanac run it reads `TO SUNRISE  2 H 05 MIN  X600`, the
event, what is left of the wait and the warp the run is at). With a belt as the local target (O3):
`LOCAL: <belt> <distance to the rock>`, `ROCK 24.3 KM ACROSS` (`- NO GROUND FOR THE CAPSULE` when parked) and a bracket sized to
the rock; the data sheet (`I`) shows the belt's sheet: edges, width, periods at both edges, a rock estimate, `LANDABLE NO`, the
target or parked rock. When the local target is a comet a second line gives `Game::cometMotionString`:
`216 KM/S, PERIAPSIS IN 6.5 H` or `... RECEDING, APOAPSIS IN 14 H` (N0-01); the data sheet adds `NOW <distance> FROM THE STAR`
plus that line, the analyzer appends the speed to comet rows, and the help says `T TIME WARP (ORBITS MOVE)`. In the radar camera
(C-07, `renderRadarCamera`, in place of the space HUD): the vehicles' frame (the visor and the inner brackets, `CAM 03 RADAR` with
a blinking `REC`, `AZ 123  EL +12` the ship's attitude, the EPOC and the system's name at the right), `RECEIVER` and
`SWEEP`/`LOCKING`/`LOCKED` under a scope of the meter's last four seconds (x 80-240, y 36-56, the trace in green) with the meter
beside it (amber when locked); the crosshair and the lock cone of three degrees round it (dim while sweeping, cyan with a white
arc filling while the hold lasts, amber when locked); the source as a smudge of static within the middle lobe (nothing outside ten
degrees) and as an amber diamond once locked (an edge `+` when it is behind); three readout lines at y 146, 154 and 162 (the hint
`SWEEP THE SKY - A RISE ON THE METER IS A SIGNAL`, `A RISE - CENTRE IT AND HOLD TO LOCK`, `LOCKING 62% - HOLD STEADY`, or the
lock, the system only: `A SIGNAL FROM UNKNOWN (S00)  79 LY` / `SECTOR 149 56 78 - THE SIGNAL IS 79 YEARS OLD` /
`ENTER LOCKS THE STAR AND FLIES THERE`; of this system: `A SIGNAL FROM THIS SYSTEM` / the body's name / `ENTER APPROACHES IT`);
`B / ESC LEAVES THE CAMERA` at the bottom left. In the telescope (W-06, `renderTelescopeHUD` in place of the space HUD, as the
radar camera's; the user's review of 2026-10-06 made it the camera's frame): the vehicles' frame (the visor and the inner
brackets, `CAM 04 TELESCOPE` with a blinking `REC`, `X16  FIELD 2.4°` under it in cyan, the EPOC and the system's name at the
right, `EXPOSING 46%` with a bar under them while the plate builds), a reticle at the centre (a circle open at the four points and
a cross clear of the centre, cyan while tracking), the target's line under it (`UNKNOWN  ROCKY MOON  64397 KM  6.4°`, 52
characters at most, cyan when it is the tracked body), a scale bar of forty pixels at the bottom right with what it spans at the
target's range (`120 KM`), `STABILISED ON <name>` (the parked world under the stabiliser, R-405; `TRACKING <name>` for another
body; else `ENTER TRACKS THE TARGET`) at the bottom left and `+/- ZOOM  N NAMES  P PHOTO  Z / ESC STOWS` under it; parked at a
giant, `C SENDS A PROBE TO <lat lon>` over them (X-01). The probe's camera (X-01, `renderProbeHUD`): the vehicles' frame,
`CAM 05 PROBE  T+04:31` with a blinking `REC`, the stage in cyan under it (`FALLING TO <giant>`, `ENTRY`, `ABOVE THE CLOUDS` ...),
the EPOC and the giant at the right and a `SIGNAL` bar under them (green, amber, red as the link weakens with the depth; static
and snow on the picture grow with it); three readouts at the bottom left: in the fall `94438 KM TO THE CLOUD TOPS` /
`ENTRY IN 34 S` / `AIMED AT 5.0° N 100.3° W`, in the entry `ENTRY 19.9 KM/S  206 G` / `ALT 473 KM  0.0000 ATM` /
`THE HEAT SHIELD TAKES IT`, in the descent `PRESS 1.30 ATM  DEPTH 4.2 KM` (`ALT` over the one-bar level) /
`TEMP -63 C  WIND 290 KT W` (toward the west, as the surface's) / the chute's state; `MOUSE LOOKS  X CENTRES  P PHOTO  ESC CABIN`
under them.

Surface (R-414, 2026-10-11: pared down on the user's "it's overblown at times"): on foot the body's name and latitude/longitude,
the scanner's two lines under it while it is on (R-408: `SCAN RUINS` and `1.6 KM NW`) with its diamond on the compass strip, EPOC,
the compass strip with the heading, the mark the explorer shows over the view (B-412: one, chosen on the sector map, none by
default; a diamond and `CITY OF ASH 1.7 KM`, and a hollow white diamond on the compass strip at its bearing beside the scanner's
filled cyan one; R-408 drew the eight nearest within 25 km), the stamina bar with the pulse (M8-02), `JET HEAT 45%` while the
jetpack flies (`CTRL DESCENDS` on a comet), and over the stamina what `E` or `Q` would do here, stacked from the bottom:
`CAPSULE HERE - Q`, `BUGGY HERE - E` or `DRONE HERE - E` within 3.5 m, the record's object or the instrument within reach
(`A CODEX - E`, `A PAINTED WALL - E`, `A LUTE - E`; C-03's `A SHARD HERE - E`), `A GRAVE: THAELU, 140-212` while a grave's stone
is within 2.2 m on foot (C-12: the name and the years in the people's count; the first time at each stone a status line
`A GRAVE: THAELU, YEARS 140 TO 212 OF AIYIN`, a `GRAVE` entry in the log and a `grave` line in the guide). The capsule's, the
buggy's and the drone's distances are not shown (`K` calls the capsule, `B`/`F` at it unfold a new buggy or drone; `Q` or `E` far
from the capsule says in the status how far it is; the sector map marks all three). The readings (R-414, `Tab` on the ground and
in the vehicles, `drawReadings`): a panel at the right under the epoch's lines (rows of 23 characters at most, two readings to a
row where they fit, so it stops short of the crosshair; twelve rows at most), `Tab` again puts it away (kept as left for the
session): `SECTOR LON:LAT` (O2), the local time, the sun's altitude and azimuth (D-04: `NO SUN` on a drifter's ground), the
elevation and the temperature (D-04: `LAMP ON` beside them while it is lit), the gravity and the pressure
(`GRAV 0.91 G  0.65 ATM`; `GRAV 0.0005 G`, `ESCAPE 9.2 M/S` on a comet, O4), the wind and the weather (`RAIN 40%`, `SNOW`, `HAIL`,
`DUST 30%`, `FOG`, `AURORA` or `AURORA STORM`, `RING SHADOW` O0-01, `VENTING 40%` O4; a second row when there are many), the life
sensor (B-321: `LIFE 300 M NE (FLYERS)`, the nearest herd or flock within 1.5 km), the old road underfoot (C-09: `OLD ROAD NE-SW`,
the way it runs; T-01: its kind, `OLD PATH`, `OLD RAILWAY`, `OLD CANAL`, `OLD HIGHWAY`; the first step onto each road of a landing
is a status line, `AN OLD ROAD RUNS HERE, NE TO SW`, and a `ROAD` entry in the log), the rangefinder (O1: `AHEAD 8.4 KM 33 MIN`,
the time at a walk, a drive or a flight at 80 m/s as the explorer goes; `WATER` when the crosshair rests on water; nothing on the
sky) with the mark the crosshair rests on under it (R-408), `SWIMMING` or `DIVING`, the jetpack's height and climb, `AUTOWALK 3`
and the vision mode. In the buggy (N4-06) and the drone (R-403) the picture is the nose camera's (`SurfaceView::cameraFeed`) and
the HUD is its frame: the inner brackets, a blinking `REC`, the name and the coordinates, the scanner's lines under the REC, the
compass, a pan gauge under it showing the +-70 deg range, a crosshair, the speed in double-size digits (`KM/H BACK` in reverse)
and the turn bar under the picture (`STEER`, `TURN`: the marker moves right when turning right); no epoch, no stamina; R-414 took
`CAM 01 NOSE`/`CAM 02 DRONE`, the pan and tilt numbers, the trip, the gear, the height and the climb, the heading, the state, the
capsule and the local time off the view. Their readings add the heading and the trip, the buggy's gear (`REVERSE`) and state
(`AIRBORNE`, `SKIDDING`, `BRAKING`, `LIGHTS ON`) and the old road under its wheels, the drone's height over the ground and its
climb (amber at the 400 m ceiling) and state (`LANDED`, `LIFTING`, `AT ITS CEILING`, `CLIMBING`, `DESCENDING`, `HOVERING`,
`LIGHTS ON`). The chase view (`V` in a vehicle) shows the walker's HUD without the stamina. N3: the data sheet on the surface
lists the world's bestiary (`LIFE`, sighted species starred), a first sighting writes a `SIGHTING` entry to the expedition log and
a status line and counts in `CREATURES SIGHTED` on the statistics screen (`Guide::creaturesSeen`, `stat creatures` in the guide
file); N4-05 adds `LONGEST DRIVE` and `TOP SPEED` (`stat longestdrive`, `stat topspeed`); the landing map's biome line ends with
`LIFE: <species>`, the land species most likely met in that biome. W-03: the sector map (`N`) draws the nearest front's rain line
across it (cyan; amber for a dust front) with its band hatched behind the line, and the legend reads `RAIN FRONT` /
`IN 2 H 05 MIN` (or `ENDS 1 H 05 MIN` under one); the readings' own (`WIND`, `RAIN`, `DUST`, `TEMP`) are where the front shows on
the ground.

## The almanac (W-01, 2026-10-06)

`Ctrl+T` in space, on the surface and on the landing map (also `T` on the data sheet, whose footer says so, and `THE ALMANAC` on
the flight computer's devices page) opens the almanac (`ALMANAC`, `game/almanac.cpp`): the next events at the place with their
countdowns and a free row, drawn over the scene it was opened from (the landing map keeps its picture). The place is the
explorer's feet on the surface, the cursor's site on the landing map (a sunrise at the site chosen can be waited for before
landing) and the ship's parking in space (the lap of the orbit mode followed). The events come from the finders of
`galaxy/almanac.*` (`almanacOf`; a registry W-03's fronts and W-04's showers add to with `addAlmanacFinder`), every one a zero
crossing or an extremum of the geometry the sky draws, scanned at a step small against the shortest period it moves with and
refined by bisection or a golden section: on the ground `SUNRISE`/`SUNSET` (the upper limb at the horizon: the centre at minus its
angular radius), `NOON` (the transit, the hour angle through zero), `FIRST LIGHT`/`END OF TWILIGHT` (the sine of the sun's
altitude through -0.12, where the sky model's glow begins and ends; worlds with an atmosphere only), for every body with a disc of
a quarter degree or more in the sky and for the companion sun `<NAME> RISES`/`SETS` (`SECOND SUN RISES`), `<NAME> FULL`/`NEW` (the
lit share's extrema, read from the world's centre), `<NAME> OVER THE SUN 85%` (the deepest of the next crossing, M1-08's cover
from the place itself), `INTO`/`OUT OF THE RING SHADOW` (O0-01's rule); from the ship `<MOON> ACROSS`/`BEHIND <BODY>` (first
contact with the parked body's disc, in front of it or behind), `SHADOW OF <MOON> ON <BODY>` and `<MOON> ECLIPSED BY <BODY>` (the
landing map's shadow rule: the centre against the line from the star), `<A> AND <B> 0.4° APART` (two planets within two degrees at
their closest), `<NAME> ACROSS THE SUN` (first contact); from both `<COMET> AT PERIAPSIS`/`APOAPSIS`; W-03 (on the ground and at
the landing map's cursor) `A FRONT FROM THE W` / `A DUST FRONT FROM THE SW` (the front's line reaches the place: the rain or the
dust begins, the bearing it comes from) and `THE FRONT CLEARS` (its rain ends), every front within the window. Names are the
explorer's (UNKNOWN otherwise). The window is five days on the ground and thirty from the ship, the first event of each kind per
body, sorted by time; the header reads the state (`DAY  DARK IN 2 H 05 MIN`, `NIGHT  LIGHT IN 3 H 10 MIN`,
`THE SUN STANDS STILL HERE` on a locked world, `12 EVENTS WITHIN 30 DAYS, FROM THE PARKING`). `Enter` runs the clock to the row:
the warp is `(remaining / 3 s)^1.5` clamped to x1..x10000 and set again every frame (`almanacWarpFor`), so the last minute of game
time takes about five seconds of ours and the last three seconds run at x1; the last frame's step is cut to the remainder and the
clock lands on the event (`SUNRISE - X1` on the status line, a beep). `T` stops a run (`RUN STOPPED - X1`), and the HUD's warp
corner reads `TO SUNRISE  2 H 05 MIN  X600` meanwhile. The free row (`RUN FOR 1 H`, Left/Right from a minute to five days) runs
for a span, as M5-06's time-lapse did. Under the real-time clock setting the list is shown but nothing can be run to (the footer
says so). A run in progress is not saved (the save's `warp` line reads 1), and a load or a new expedition ends it. Harness:
`vesperis_test almanac` (`08-testing-and-tools.md`), `testAlmanacInfo`, `testAlmanacSelect`, `testAlmanacRunning`,
`testAlmanacTarget`, `testTimeWarp`.

## The cabin (M2)

With the SHIP VIEW setting on CABIN (default) the space state puts you inside the Stardrifter (`Cabin`, `game/cabin.cpp`): a 5 x 5
x 2.4 m room in ship-local metres (+z forward) drawn over the space view with depth, so the outside shows through the window band
(0.95-2.05 m) of every wall and the round roof window. Mouse looks (`cabin.yaw/pitch`), `W A S D` walk (walls, the capsule cage of
radius 1 m, the GOES console on the left wall the front desk block and, since C-06, the decoder on the back wall), the arrow keys
turn the ship itself; `viewBasis()` = head look x ship attitude is used for the space camera, the crosshair and the HUD markers.
`E` uses what you face within reach (`cabin.facing`): the GOES console (left wall), the flight computer (front desk), the three
right-wall screens (star map, landing map = deploy the capsule, target data), the capsule cage (deploy) and, since C-06, the shard
decoder on the back wall (`facing` 7: a desk like the console's whose screen glows and breathes while a shard waits to be read)
and, since C-07, the signal radar beside it on the same wall (`facing` 8, x -2.1..-1.0: a narrower set whose screen brightens and
pulses while the receiver is on; `E` enters the radar camera) and, since W-06's rework, the telescope's set at the wall's other
end (`facing` 9, x 1.0..2.1, its screen glowing while the eye is at the eyepiece; `E` looks through it). R-404 (2026-10-06): every
computer's screen glows in a colour of its own (`SCREENS` in `cabin.cpp`: the console amber, the flight computer green, the star
map blue, the landing map pink, the target data cyan, the decoder violet, the radar red, the telescope ice-white; banks 9 and
21-27) and the `E` prompt is written in it, so the one wanted is found from across the room. `U` toggles the cabin light (the hull
ramp follows `lightLevel`), `Y` depolarises the hull (B-411: the hull alone, its walls, floor and ceiling, as tinted glass that
catches the cabin light as a faint sheen and clears with the light off; what stands in the room stays solid and lit; `U` with the
hull glass says `Y POLARISES THE GLASS HULL`), `PgUp`/`PgDn` ride the lifter to the observation deck on the roof. Banks 8 (hull),
9 (screens), 10 (light, the fuel orb and the capsule) are set by `cabinPalette`. The cabin state is saved (`cabin` line).

The flight computer (`SHIPSCREEN`) has three pages: Flight control (targeting, Vimana, local target, analyzer, approach, centre, orbit, deploy, home, previous; R-407: `VIMANA JUMP: LIGHT YEARS ALONG THE AIM` last, which opens the aim along the nose for the digits), Onboard devices (light, hull, lifter, field amplificator, time warp, scanlines, the signal radar: C-07, the almanac: W-01, `RUNNING` beside it during a run, the telescope: W-06, `X64` or `STOWED` beside it) and Preferences (settings, guide, log, the shards, save); Left/Right change the page, Up/Down and Enter act, `E`/Esc leave. Every action shares code with the key commands (`toggleVimana`, `deployCapsule`, `startApproach`, ...).

The GOES console (`CONSOLE`): a text terminal with a 200-line scrollback and typed commands: `SL [LY]` lists stars within a range (name, class, distance, sector), `DL` lists the bodies and belts of the system, `PAR NAME` / `WHERE NAME` print a star's sector, class and distance (`WHERE` alone prints the ship's sector and region), `ST NAME` sets the remote target, `CAST TEXT` writes a note, `CAT` reads the last notes, `HOME` and `PREV` target the home or the previous star, `JUMP LY` jumps that far along the ship's nose (R-407), `HELP`. Esc leaves. Renamed stars are found by their new names.

The shard decoder (`SHARDS`, C-06, `game/shards_screen.cpp`): the shards taken from the ruins (C-03) are read on the ship, at the
decoder on the cabin's back wall (`E` facing it), from the flight computer's preferences page or from the guide menu's
`THE SHARDS` (on the surface the menu says they are read on the ship). Three levels: the worlds the guide holds shards of (the
name or UNKNOWN with the sector and the body, `N OF 50 SHARDS`, `THE LANGUAGE P% LEARNT`, how many are read; C-18: `LANGUAGE P%`
where the row would run off the screen with three-figure counts; the world under the ship is selected first; C-18: the lists made
again when the guide holds more while the decoder is open, the drones' finds listed as they dock; C-13: a world of two peoples is
listed once a people whose shards are held, the people's name after the sector (`THE KHOKURT`), each entry's count, share and
reads its own, and the people named in the list's and the shard's headers; a shard taken on such a world says
`N OF 100 ON THIS WORLD`; C-14: a world with lent shards shows `N OF 50 SHARDS (M LENT)`, a lent row in cyan, `LENT` in cyan after
the shard's header, a friend's name for a piece in cyan, and a friend's name for the world in cyan), a world's shards in the order
of their years (each row the shard's first words as the computer last read them, an unread shard all in the people's tongue and
dim, `*` on one not yet read at the language's current share), and a shard: `YEAR N OF <town>` and the text laid out word by word,
a word read in white, a name in amber, a word not learnt yet dim in the people's own tongue (`civilisations/`). The language
is learnt from the shards held of that world (`languageShare`: 30% of the words with one, all at the people's full count, 12% of
its records between 10 and 24 (H-04: C-06's was all with ten); the computer learns roots, the most used first, and reads their
forms). A text shard is a recording that speaks (C-05, `peoples/voice.h`, `AudioState::speech`): opening it plays the people's
words in the people's voice (the voice named under the year: `A DEEP BREATHY VOICE, SLOW`), 12-25 s for a text of thirty words,
and the word being spoken is underlined. H-04: a long record is laid out in pages (`PAGE n/m` at the top right; a page starts at a
paragraph, or at a sentence within its last forty words); the voice speaks the page shown, the opening first, and the rest on
request: `UP`/`DOWN` or `PGUP`/`PGDN` turn the pages (the voice stops; a first reading is read at once), `ENTER` speaks the page
shown, or stops the voice, or ends a first reading's wait (`ENTER READS THE PAGE  UP/DOWN PAGE  LEFT/RIGHT`); on a record of one
page `UP`/`DOWN` move to the world's other shards as before. H-04's second pass: the people's own words a record keeps (their
coin, their dish, a native title: `{~key}`) are read from the start like a name, in amber, and every name is the people's own
tongue's. Reading a shard the computer has not read at the current share resolves the words as they are spoken, the one being
spoken flickering, with `DECODING P%` and a bar below (`ENTER` skips to the end, `SPACE` pauses, `ESC` leaves it unread, a beep
when it is done); then the guide records the count it was read with (`decoded`), so the next reading shows the whole text at once
(the recording speaking over it, `PLAYING P%`; `ENTER` skips it, or plays it again when it has ended) until more of that world's
shards are found and it decodes again with fewer gaps. `LEFT`/`RIGHT` move to the world's other shards (the voice stops), `ESC`
goes back a level. The voice, like the music, plays on the decoder screen only (`Game::frame` clears `audio.speech` off it) and
ducks the ship's hum. Game time runs on the screen as it does at the flight computer. C-12: every shard's screen is dated in the
people's own calendar (`YEAR 212 OF AIYIN, MOON 69, DAY 5`; `shardDate`, `civilisations/peoples-and-ages.md`), the world's list carries the
calendar line (`YEAR: 101 MOONS OF 5 DAYS AND 1 OVER`) and a timeline under its rows (a rule from YEAR 1, a tick a shard held:
read green, unread dim, the cursor's white; dotted past the latest held while the record is open), and the world's last recording,
once read, heads its screen THE LAST RECORDING, turns its row's year amber, closes the timeline with THE RECORD ENDS, YEAR N in
amber and writes a `SHARD` log line; the guide keeps the world in `ended`. H-05: under a shard's date its medium is named (R-414:
the object alone, `A CLAY TABLET`, `A WAX CYLINDER`, `A REEL`, a piece on a medium without a sound of its own as what it is,
`A BONE FLUTE`, `NOTATION ON A SCROLL`, `PRINTED MUSIC`; H-05 said who was heard, `A CLAY TABLET, READ BY THE SHIP`,
`A WAX CYLINDER: A ROUGH VOICE`, `A RECORDING IN WAX, THROUGH ITS HORN`) and the object is drawn at the right of the date (40 x 26
of the HUD: a bone, a tablet, a scroll on its rods, a codex, a wax cylinder turning while it plays, a disc spinning, two spools, a
drive blinking, a crystal glowing, the humming thing's rings; `game/medium_draw.cpp`); a recording (a wax cylinder, a reel, a
disc, a drive, a crystal, something that hums) is heard through its medium (`AudioState::medium`: the cylinder's horn, crackle and
turning, the disc's surface, the reel's hiss and wow, the drive's rot, the crystal's shimmer, the hum's drone), a record with no
sound of its own is read by the ship in the people's voice made flatter or its notation played dry; a piece's page names its age's
instruments (`VOICES, A BONE FLUTE AND SKIN DRUMS`; the far future's `NO BEAT: 12 TONES OVER 5 OCTAVES`) and its roll colours the
parts by role (the tune white, the second voices green, chords dark cyan, the bass dim amber, the kit in rows of ticks); a chart
is drawn in its age's way (a cave's ceiling, a carved bone, a painted vault, an astrolabe's plate, a codex page, an engraved
atlas, a photograph on glass, a survey, a navigation chart of routes, the far future's chart of places where nothing is), its
header naming the way (`A STAR CHART OF THE MURUSAC, ON A CAVE'S CEILING`; `game/chart_draw.cpp`); `civilisations/`, "The
three categories by age (H-05)". H-06: a world's records are on three pages, `TAB` between them (`SHIFT+TAB` back; the strip under
the title names them, the one shown white): RECORDS (the list: each row's kind as a glyph, a page, a note or a star, then the year
in the people's count and the first words; `LEFT`/`RIGHT` show the texts, the music, the star charts or all, the strip counting
each kind; `UP`/`DOWN` and a record's own `LEFT`/`RIGHT` keep to the kind shown; R-417: `UP`/`DOWN` held with `SHIFT` run as the
timeline's do, through the kind shown; the hint's kind keys are the font's two arrows,
`ENTER READ  ↔ KIND  SHIFT FAST  TAB PAGE  ESC BACK`), TIMELINE (the people's eras across the width, a band and its name once a
record of the era is read, the records held stacked by their years in their kinds' colours, the events learnt as ticks under the
axis, the record under the cursor below with its date, medium and first words; `LEFT`/`RIGHT` along the records (R-416: held with
`SHIFT` they run, faster the longer they are held, stopping at the ends), `ENTER` reads one) and HISTORY (THE AGE THEY REACHED
once three of its records are read or the last recording, the people's ways as the records told them, the events the read records
told in order under their eras, a person named only once a record wrote their name, the words read as the records are, the last
recording closing it; `UP`/`DOWN` along the events (`SHIFT` held: they run), `ENTER` opens the record that told the one under the
cursor, `ESC` back to the page); `civilisations/records-and-grammar.md`, "The decoder reads a history (H-06)".

A piece of music (C-04, `peoples/music.h`): twelve to eighteen of a world's fifty shards are music, not text. In the world's list a piece's row is its form and length (`A LULLABY, 52 S`) or, once named, `'THE RIVER AT NIGHT' - A LULLABY`; `*` marks one not heard yet. Opening it plays it through the synth (`AudioState::piece`): the screen names the piece (its form "OF THE <people>", or its name) and the tradition in three lines (`A SCALE OF 7 NOTES TO THE OCTAVE`, `A CYCLE OF 2+2+3 AT 112 BEATS A MINUTE`, the timbres: `A PLUCKED STRING, A PIPE AND A DRUM`), and draws the notes as a roll across the piece's length, a bar per note at its degree's height (the lead white and two rows thick, the second voice green, a third cyan, the drum amber ticks under them), the cycles marked, an amber playhead; what the playhead has passed is in colour, the rest dim. The footer is `PLAYING P%` with a bar, then `THE PIECE ENDED`. Keys: `SPACE` pauses and resumes, `N` names the piece (TEXT_ENTRY over the screen, the music going on; the name goes into the guide's `names` under the shard's key and the log gets a note), `ENTER` plays an ended piece again, `LEFT`/`RIGHT` move on (the music stops), `ESC` goes back. A piece is heard whole from the first (the language has no part in it); hearing it records it in the guide as read at the world's full count, so it never shows `*` again. The music stops the moment the decoder screen is left by any road (`Game::frame` clears `audio.piece` when the state is neither SHARDS nor its TEXT_ENTRY); nothing plays it elsewhere on the ship yet (C-11, the instruments, is where that belongs).

A star chart (C-10, 2026-10-03, `peoples/charts.h`, `civilisations/records-and-grammar.md`): three to five of a world's fifty shards are charts of the people's sky round a star they marked (the world's list says `A CHART:` and the caption). Opening one draws it beside its caption: a disc of the hemisphere round the marked star with the stars as dots by their brightness, the people's figures joined by lines and named at their brightest star (dim in the people's words until every word is learnt), the marked star's amber diamond, and at the right the caption word by word (coloured as a text's) and `THEY MARKED`: the star as the explorer knows it (or UNKNOWN), its class, its distance from here and its sector (`- THE REMOTE TARGET` in cyan once it is). A first reading at the language's current share draws the chart over four seconds, the stars first, then the lines, then the names flickering into place (`DECODING P%`; `ENTER` skips the wait, `ESC` leaves it unread); on a chart read `ENTER` sets the remote target on the marked star (`REMOTE TARGET: ... - V TO FLY`, the chain's next link), `O` holds the chart up to the sky and puts it down again (`HELD UP TO THE SKY` under the star), `LEFT`/`RIGHT` move on, `ESC` goes back. Held up, the chart's figures are drawn on the space view (not during a Vimana flight or in the radar camera) and on the surface at night between the real stars from wherever the ship is, on the sky alone (a line's pixel is skipped where the frame has depth: the cabin's walls, a globe, the ground, a mountain), with the names at their brightest star, the marked star's diamond and `THEY MARKED THIS STAR` (an amber `+` at the edge when it is behind the view) and `A CHART OF THE <PEOPLE> HELD UP` at the top right; it stays up across screens and landings until put down, and is not saved. A chart read logs a `CHART` entry naming the marked star's sector and class only. No chart has a voice: a map, not a sound.

## The GUIDE (`guide.txt`, M3)

`Guide` (`game/guide.h`) holds your names for stars (`sx,sy,sz`) and bodies (`sx,sy,sz/index`), names received from other
explorers (`inbox`, shown in cyan), the expedition log, visited stars, landed bodies, star classes and world types seen, the
shards taken (`shard <body key>/S<index>`, C-03: `Game::shardKey`; `Guide::shardWorlds` counts the worlds they come from; T-04: a
people's records at people x 1000 + index, the file's `records 2` line saying so: a guide without it had C-13's second people at
50 + index and is moved when loaded, `Guide::migratedShards`), the shards read on the decoder (`decoded <body key>/S<index> <n>`,
the count of that world's shards the language had then, C-06), the signals the radar locked (`signal <key> <kind>`: a world's key
or a pulsar's star key, C-07) and the recordings heard on it (`heard <body key>/S<index>`, C-07), the explorer's marks on the
worlds (`mark <body key> <lat> <lon> <kind> <lent> <when> <name>`: radians, the kind a `LandmarkKind` or 8 for a place (T-03: 9 a
work's; in memory `LM_WORK` is 8 and a place 9, `markKindToFile`), the name last and possibly empty, R-408), the probes' records
(`probe <id> <lent> <body key> <gen> <saved> <launchT> <aimLat> <aimLon> <seed> <start xyz> <headYaw> <descYaw> <chuteOpen> <chuteClose> <crushBar> <endClock> <endBar> <endKm> <endK> <endCause> <storm key|->`,
each followed by `probecam <seed> <n> ...`, `probetime <seed> <n> ...`,
`probeprofile <seed> <n> <clock bar K wind light layer flashes> ...` and `probelayers <seed> <n> <layer from to> ...`;
`stat probes`, X-04; F-02: the four lines are made once, at the load or a record's first save, and kept in `ProbeRecord::lines`,
so a save writes them as they are: 0.9 ms for the player's 731,810 bytes, 7.0 before), the travel history (last 20 stars) (R-414:
G-05's route, `route <star key>` lines of every arrival in order for the galaxy chart's way between the stars visited, is gone; an
older guide's lines are skipped at its load and gone at its next save), the explorer's marks of the galaxy
(`gmark <x> <y> <z> <lent> <when> <name>`: light years in the sector frame, `y` over the plane, the name last and possibly empty;
lent by an export like the marks of the worlds, one within a light year of a mark already there being the same place; G-06), the
home star and the statistics. Text file, header `vesperis-guide 1`, saved whenever it changes and with every game save.
`Game::starNameOf`/`bodyNameOf` resolve display names everywhere (HUD, lists, sheets, landing map, statuses); a renamed star
renames the generated names hanging off it ("HOME OF CATS II", "HOME OF CATS II-a").

Keys: `G` opens the guide menu in space and on the surface (star map, rename this star / this world, write a note, log, gallery, statistics, the shards (C-06: the decoder, from the ship; C-04: `N` on a piece names it), target by name, target by coordinates, return to the previous star, target the home star, set home here, export, import); `M` in space opens the star map directly (G-05: in a Vimana flight too, on the galaxy page); `J` the log. Text entry (`TEXT_ENTRY`) takes letters, digits, space, `-` and `.`; Enter confirms, Esc cancels.

Log entries are written on arrival (class, counts, sector), when parked in orbit, on landing (place, temperature, pressure, gravity, day or night), on launch (longest walk, distance driven), when a shard is taken (`SHARD`: the place, the settlement's class, the world and the count of the world's fifty, C-03) when the radar locks a source it had not (`SIGNAL`: the kind, what is heard, the source and its distance and age, the sector, C-07), at a jump (`JUMP`, R-407), at a mark placed (`MARK`: its name, what it marks, the world, the place, R-408) and for notes; the log screen pages newest first. Statistics: systems visited, worlds landed on, classes and types seen (S-02: the classes' codes on their own line, two past twelve classes), furthest from home (LY), longest walk from the capsule, highest point, distance on foot and driven (one line since C-03), shards taken and the worlds they come from (C-03) and how many are decoded (C-06), under it a dim line with the graves read and the records ended (C-12), names given, photographs, creatures sighted and (C-07) the signals heard on the same line.

Star map (`STAR_MAP`), two pages turned with `Tab` (G-05; the map opens on the page it was left on, on the galaxy in a Vimana
flight, where `Tab` is refused with `IN FLIGHT THE MAP SHOWS THE GALAXY`). **The neighbourhood** (titled
`THE NEIGHBOURHOOD  1419 STARS IN 10 LY  ZOOM 1.0X`, the keys `WHEEL ZOOM  C/X/0 CLASS  ENTER TARGET  TAB GALAXY`): the 10 LY
around the ship, turned with the mouse or arrows, zoomed with the wheel or `+`/`-`; rings at 2, 5 and 10 LY on the galactic plane;
stars coloured by class and dimmed with depth; visited stars boxed, the current star crossed, the remote target framed, the home
star a diamond (R-407: a legend beside the class filter says so, an unnamed visited star carries the name of a world the explorer
named there in brackets and its card lists them: `YOUR WORLDS THERE: XOHORT`); the class filter cycles with `C` (forward) and `X`
(back) through ALL and every class of the table, whatever their number (S-02: the keys `1`-`9` toggled S00-S08 and ran out at
S-01's tenth class), `0` shows all; the legend reads `CLASSES: ALL` or, in amber, `CLASS: S10 NEUTRON STAR`, and the title counts
the stars shown; the star nearest the crosshair is described and `Enter` (or a click) makes it the remote target. **The galaxy**
(`game/galaxy_chart.cpp`, G-05, R-409): the whole disc by its stars over the frame's width (`Game::CHART_FACE_Y0` 22, 128 rows)
and an edge-on strip under it (rows 152-173): the face-on column of the stars through the disc (`galaxyFaceOnColumn`,
`04-galaxy-and-systems.md`) toned like the harness's map and coloured by the light of its populations (the bulge warm, the disc
between the arms a pale gold, the arms blue-white and weighted brighter), the strip the field's mean density along the view's
depth; the globulars as faint warm points at every level, the open clusters (blue-white knots) and the star-forming complexes
(faint red glows) from level 6; nothing named. Nine levels by powers of two (`+`/`-`, the wheel): level 0 is the whole disc,
110,000 ly tall (860 ly a pixel), centred on the galactic centre; from level 1 the view is centred on the ship and follows it (a
flight too) until the cursor takes it away; level 8 is 430 ly tall (3.4 ly a pixel). **The cursor** (G-06): white corners round a
point, on the ship (and moving with it) until the arrows (80 px a second, Shift three times faster) or the mouse move it (a nudge
under 2 counts leaves it there); the view moves with it within 16 px of the face's sides and 12 of its top and bottom (level 0
holds it: the whole disc), its reach 54,000 ly from the galactic centre (`Game::CHART_REACH_LY`); the zoom keeps its pixel; `C` or
`Space` put it and the view back on the ship, and so does an arrival. Let go of the ship it lies in the plane (`y` 0, where the
stars are) unless `PgUp`/`PgDn` moved its height (two seconds from the plane to the strip's edge, Shift faster, to 30,000 ly over
or under), shown on the strip as a small x with a dotted line from the ship. Over the chart: the ship (a white crosshair open in
the middle, `YOU`; an arrow at the edge when it is off the face), home (the amber diamond, `HOME`), the stars visited (green dots;
R-414: G-05's green way between them in the order of the arrivals is gone, the user: "they seem to be a good idea but they confuse
you a lot"), the remote target (a cyan square, a dotted line from the ship, on the strip too; in a flight the line flown, the ship
moving along it), the signals the radar locked (red rings), the stars the explorer named at levels 6-8, the explorer's marks of
the galaxy at every level (a pennant with its name, a friend's in cyan; G-06), the cursor's dotted line from the ship and, beside
it, what the explorer has within four pixels of it (a mark's name, `A SIGNAL`, `HOME`, a star's name, `VISITED`). The labels are
kept apart, the ship's first, then what the cursor rests on and home's; a name with no room is left out. The rows: the title
`THE GALAXY` and the region at the cursor (the ship's while it sits on it); under it the ship's place in green
(`13,040 LY FROM THE CORE, IN THE PLANE`, `2,000 LY NORTH OF THE PLANE`), the cursor's in white
(`ONE STAR IN 7 SECTORS, IN THE PLANE`: `galaxyDensity` at the point as one star in so many sectors to two figures,
`A STAR IN EVERY SECTOR`, `NO STARS` past one in a million; ` OF THE PLANE` dropped past 52 characters), or, plotted, in cyan
`PLOTTED: A STAR 1 LY FROM THE CURSOR` (its name once it has one; in the void `PLOTTED: A STAR 16,795 LY SHORT, THE VOID`); the
scale bar with the zoom (`10,000 LY  X4`) in the face's corner; under the strip the jump's line
(`5,000 LY COREWARD - 61 S BY VIMANA`: `galacticHeading`, `vimanaSeconds`; to the plotted star in cyan) or, the cursor on the
ship, the legend (`YOU HOME VISITED SIGNAL TARGET MARK`); the last row the status while one shows (G-06: the map pages drew none,
so a refusal there was silent), else the keys of the moment: `ARROWS MOVE  WHEEL ZOOM  N STOPS  TAB NEIGHBOURHOOD` (in a flight
`... N STOPS  L MARK`), the cursor away `ENTER PLOT  N STOPS  L MARK  PGUP/PGDN HEIGHT  C YOU` (on a mark
`ENTER PLOT  L RENAME  DEL TWICE REMOVES  C YOU`), plotted `ENTER FLY  TAB LOOK THERE  ESC KEEP THE TARGET`.

**Plot, then fly** (G-06): `Enter` (or a click) plots the jump to the cursor's point by R-407's line (`Game::plotJump`,
`jumpTarget`: the star nearest its end, in the void back along it to the first stars), the star made the remote target
(`PLOTTED - ENTER FLIES, ESC KEEPS IT FOR V`); `Enter` again flies it (the map closes; the Vimana; the log's
`JUMP 5000 LY COREWARD FROM ... TO PARSIS ..., THE DISK - PLOTTED ON THE CHART`, written as the flight starts, so `V` flying a
plotted target writes it too); `Esc` keeps the target for `V`; a move of the cursor lets the plot go (the target stays, a new
`Enter` plots again). Refused in a flight (`CANNOT JUMP DURING VIMANA FLIGHT`) and with the cursor on the ship
(`MOVE THE CURSOR TO WHERE THE JUMP GOES`). The aim's jump and the console's `JUMP` are the same two steps at once (`startJump`).
**The look before the leap**: on a plotted jump `Tab` opens the neighbourhood page round the plotted star
(`AT THE PLOTTED END  1215 STARS IN 10 LY  ZOOM 1.0X`; a `StarNeighborhood` there, `Game::mapStars`/`mapCentre`), the plotted star
in the middle left out of the pick as the ship's own star is on the ship's page and the card's distance from it; `Enter` takes the
star the crosshair rests on as the plotted end and goes back to the chart still plotted, `Tab` goes back without one, `Esc` closes
the map (it opens on the chart again). **Stops**: `N` (Shift+`N` back) puts the cursor on the explorer's things, the view centred
there: the signals the radar locked, home, the named stars (the inbox's too), the marks of the galaxy, the systems visited,
nearest the ship first within each kind, a star once, the ship's own star left out (`A SIGNAL - 1 OF 5`). **Marks of the galaxy**:
`L` names the place under the cursor (a void, a cluster, a stretch of arm; the ship's place when the cursor sits on it): the entry
`NAME THE PLACE` over the chart with the place's region and distance from the core, an empty name placing an unnamed mark, on a
mark `RENAME THE PLACE`; `Delete` twice removes the mark under the cursor; logged
(`MARK THE HIGH VOID IN THE GALAXY: THE HALO, 8,040 LY FROM THE CORE, 1,500 LY NORTH OF THE PLANE`). No scene is rendered under
the chart. Its images are built at the logical resolution when the view changes (2-7 ms on the worker pool), a frame without a
change costs under a millisecond at 4x.

Gallery: screenshots taken with `P`/`F12` get a sidecar `.txt` with the place and EPOC (`Game::screenshotCaption`); the gallery lists `shots/screenshot_*.png`, decodes them with `readPNG` (the game's own PNG format) and shows them with their captions. W-06: a photo through the telescope carries `THROUGH THE TELESCOPE X16: UNKNOWN  ROCKY MOON  64397 KM  6.4 DEG - IN ORBIT OF UNKNOWN  EPOC ...` (the glyph written as `DEG` in the text file). X-04: the probes' recordings are listed among them (their final images, `shots/probe_<n>.png`), merged by time; Enter plays one; a photo of a recording carries `THE RECORDING OF PROBE 3 INTO UNKNOWN AT T+04:31, 0.62 ATM - ...`.

Sharing: "export the guide" writes `guide_export.txt`; "import an inbox file" reads `guide_inbox.txt` (a friend's export placed next to the game) into the inbox; your own names always win. C-14: the import also lends the shards the friend found and read (their `shard` keys with a `decoded` line; what they were lent themselves does not travel on, like the inbox's names): a lent shard is listed and read on the decoder in cyan, counts in its world's language like one of your own, plays or speaks like one, and is never taken (the ruins still hold it: taking it there makes it your own). The status says `N NEW NAMES AND M LENT SHARDS FROM THE INBOX`, an `INBOX` log line counts the worlds, and the statistics count the lent (`Guide::lent`, `lent` lines). X-04: it lends the friend's own probe recordings too (not those lent to them; a seed already held is skipped): a lent recording is marked (`lent` in its `probe` line), framed and titled in cyan in the gallery and on the giant's profile pages (`LENT`), plays like one's own and counts in no statistic; an `INBOX` log line counts them. R-408: and the friend's own marks (not those lent to them; one standing within about 30 m of a mark already held is the same place): drawn in cyan on the maps and over the view, heard by the scanner like any sight (only the explorer's own marks leave a sight out), the explorer's own once renamed. The status reads `INBOX: N NAMES, M SHARDS, K RECORDINGS, J MARKS` (the parts there are).

## Settings (`vesperis_settings.txt`, M0-02)

`Settings` (`game/settings.h`): `mouse_sensitivity` 0.2-3.0, `invert_y`, `fov` 50-100 (horizontal degrees), `master_volume` 0-1, `scanlines`, `window_scale` 2-6 (window = 320*s x 200*s), `render_scale` 1-4 (framebuffer = 320*s x 200*s, default 4 since B-310, 2 before), `mush` 0 classic / 1 soft / 2 sharp / 3 fine (box size scale+1 / scale+2 / max(2, scale) / max(2, scale-1); fine is the default since B-310), `shading` 0 flat cells / 1 stepped gradient / 2 smooth (the default since B-311), `crt` and `bloom` (optional post shader in the platform layer, off), `sprint_toggle` (hold or toggle) and `sprint_speed` 2.5-4.0 (M8-10); R-418: `pad_look_speed` 0.2-3.0, `pad_invert_y`, `pad_dead_zone` 0.05-0.40 (set on the controller page). Text file with a `vesperis-settings 2` header; unknown keys ignored, values clamped on load; a version-1 file (from before B-310) keeps its other settings but takes the new look once (render scale 4, the fine mush, smooth shading) and is saved as version 2. `Game::applySettings` pushes them into the projections (`spaceR.proj`, `SurfaceView::setFov` rebuilds the direction table), the mouse factors, and `AudioState::master`. The Settings screen (menu item 4, state `SETTINGS`) edits them with up/down and left/right (Enter also advances); Esc or BACK saves the file. `Ctrl+K`/`F10` toggles scanlines directly and saves. A window-size change sets `Game::wantsWindowScale`, which the platform layer applies with `platformSetWindowSize` (ignored in fullscreen). The `Game` constructor touches no files: `loadFromDisk()` reads the settings and the save (the window build calls it; tests set `settingsPath`/`savePath` under `shots/`). R-419: `auto_profile` 0-5, the auto-explore profile chosen last. R-420: `auto_music` 0/1 (auto-explore's tape on), `auto_track` (the track in the deck), `auto_track_at` (the second it was left at; -1 no cassette in yet).

`clock` (M5-05): 0 game time (warpable, saved), 1 real time: every simulated frame sets `t = 3.6e6 + (unix time - 2026-01-01 00:00 UTC)`, so the sky follows the wall clock as in the original and positions stay consistent when switching (`t` is the only state). The data sheet shows `EPOC:SINISTER.MEDIUS.DEXTER` (10^9, 10^6, 10^3, 1 s groups) top right and a `CLOCK REAL TIME` line in that mode.

The data sheet on the surface (M5-06/09; W-01: `T` on the sheet opens the almanac of the place, in space as on the surface) adds `SEASON` (declination, day length or polar day/night, hours or days to the next equinox and solstice, scanned over one orbit), W-03 `FORECAST` on worlds with fronts (`RAIN FROM THE SW IN 2 H 05 MIN`, `SNOW` below -1 C now, `DUST` on desert and thin-atmosphere worlds, `A FRONT` on a quartz world; `IN A FRONT, CLEARING IN 1 H 05 MIN` under one; `NO FRONT WITHIN 5 DAYS`; `Game::updateForecast` lists the next fronts at the feet every thirty seconds of game time, `forecastLine` reads them), `IN THE SKY` (the two largest bodies above the horizon with altitude and width) and `SECOND SUN` when the system is multiple. The HUD, the analyzer and the arrival log use `StarSystem::classString()` (`S00+S01 MULTIPLE`); the analyzer's class description wraps onto a second line when it needs one (S-02: the long rows were cut at 50 characters) and the list gives up a row.

`aspect` (M6-01): 0 square pixels, 1 the original's 4:3 (the platform layer draws the picture 1.2 times taller); `dither` (ordered 2x2 dither over the 64 palette stops instead of the smooth 256-entry interpolation; the pattern is applied per logical pixel at every render scale); `hud_color` 0 green, 1 amber, 2 white (`setHudScheme` rewrites the `HUD_*` colours, so every screen follows).

## Key map (M6-02)

`vesperis_keys.txt` next to the game (see `vesperis_keys.example.txt`) holds `PHYSICAL LOGICAL` pairs: the front end applies `KeyMap::apply` after the platform filled the `Input`, so pressing the physical key acts as the logical one everywhere (swap both ways with two lines). Names: letters, digits, `SPACE ENTER ESC TAB BACKSPACE UP DOWN LEFT RIGHT PGUP PGDN HOME END INSERT DELETE LSHIFT RSHIFT LCTRL RCTRL LALT RALT F1..F12` and the punctuation names in the example file.

## The controller (R-418, 2026-10-17; M6-02's fixed table replaced)

**Read whole** (`platform_raylib.cpp`): GLFW's joystick calls (raylib's own gamepad calls read only the devices GLFW has a mapping for,
and only through it) give every button, axis and hat of the first controller: the first GLFW knows the layout of, else the first with
four buttons or more (a pad's motion sensor shows up as a joystick of axes alone). `PadState` (`core/input.h`) carries the raw device
always and the standard layout when GLFW's mapping knows it (`mapped`: 15 buttons and 6 axes by the Xbox names and places, a
PlayStation pad's cross being A). `gamecontrollerdb.txt` beside the game (SDL's controller database) is read once at the start
(`SetGamepadMappings`) and teaches GLFW the layouts its built-in list lacks. GLFW sits inside raylib's static library: the release
scripts link raylib statically and the dev Makefile links Homebrew's `libraylib.a` (Homebrew's shared library keeps GLFW hidden). The
browser build reads the standard mapping through raylib (no raw device there).

**Roles** (`PadMap` in `core/input.*`): the game reads sixteen buttons (`PR_A` .. `PR_RIGHT`: A B X Y by place, LB RB LT RT, SELECT,
START, L3, R3, the d-pad) and two sticks (`PR_WALK_X/Y`, `PR_LOOK_X/Y`, each axis bound to the half read as right or down). A role is
bound to a physical control, a number (`PADC_*`): a standard button (0-14), a standard axis's half (16 + 2 axis + 1 positive), a raw
button (32 + n), a raw axis's half (64 + 2 axis + 1 positive), a raw hat's direction (96 + 4 hat + 0 up, 1 right, 2 down, 3 left), a
raw axis read whole as a trigger from its rest at -1 (112 + axis) or +1 (128 + axis); the file's names `A`, `LT+`, `B7`, `A2-`, `H0U`,
`T5`, `T5R`, the screen's `A`, `D-PAD UP`, `L STICK UP`, `BUTTON 7`, `AXIS 2-`, `HAT UP`, `AXIS 5`. Each device starts from its
defaults, taken as it arrives: a known one its standard layout; a raw one a guess (buttons 0-3 the face, 4-5 the bumpers, 6-7 SELECT
and START, the thumbs after them past a guide button when there are eleven or more; the first hat the d-pad; axes 0-1 the left stick;
with six axes whose third and sixth rest at one end the Linux order, LT and RT there and the right stick on axes 3-4, else the right
stick on 2-3 and the triggers on 4-5 when those rest at one end), binding no stick axis that rests off-centre and no trigger that rests
in the middle (either would act by itself). A button reads pressed past 0.4 of its throw; the sticks have a round dead zone
(`pad_dead_zone`, 18%), the rest rescaled to reach the rim. What the explorer binds is kept by device in `vesperis_pad.txt` (`pad
<S: or R:><GUID, else the name> <name>` and `ROLE CONTROL` lines under it, only the roles that differ from the device's defaults),
read by `loadFromDisk`.

**The layouts** (`game/pad.cpp`, `Game::padFrame`, the first thing a frame does: the controller presses the game's own keys into the
frame's copy of the `Input`, so every action keeps its one code path). Everywhere: Y the commands, START Esc, SELECT Tab, the d-pad the
arrows, the right stick the mouse (the look: a curve `0.25 v + 0.75 v^3`, 900 mouse pixels a second at full throw times
`pad_look_speed`, the mouse's sensitivity and, outside the screens, its inversion taken back out; `pad_invert_y` its own). By context
(`padContext`, `padBindings`, which also give the menu's legend and its hints):

| Context | A | B | X | LB / RB | LT / RT | L3 / R3 | SELECT | left stick |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| screens (menus, lists, the decoder, ...) | Enter | Esc | Space | PgUp / PgDn | `-` / `=` | Shift / - | Tab | the arrows, repeating |
| the landing map | Enter (land) | Esc | - | - | `Z` out / in (the sector's zoom) | Shift (6-degree steps) / - | - | the arrows, repeating |
| the sector map | Enter (mark) | Esc | Space (the cursor home) | - | `-` / `=` | Shift (faster) / - | Tab (the next mark) | the arrows (the cursor) |
| the star map | Enter (target) | Esc | - | - | `-` / `=` | - | Tab (the galaxy) | the arrows (it turns) |
| the galaxy chart | Enter (plot, fly) | Esc | Space (the cursor to the ship) | PgUp / PgDn (the height) | `-` / `=` | Shift (faster) / - | Tab (the neighbourhood) | the arrows (the cursor) |
| the title | Enter (START too) | - | - | | | | | the arrows |
| the ship (the cabin, the cockpit, the aim, the telescope, the radar) | Enter | Esc while aiming, at the telescope or in the radar | E | PgUp / PgDn | L (N while aiming) / Shift | R / X | Tab | walks the cabin |
| on foot | Space | C (Ctrl swimming, or in the air of a world under 1 m/s2) | E | N / R | Z / Shift | Shift / V | Tab | walks |
| the buggy | Space | V | E | N / R | S / W | - / V | Tab | throttle and steering |
| the drone | Space | Shift | E | N / R | Shift / Space | - / V | Tab | thrust and turn |
| the probe's camera | Space (Enter at its end) | Esc | X | | - / Shift | | | the arrows, held |
| a recording | Space | Esc | X | PgUp / PgDn | - / Shift | | | the arrows, held |
| an instrument (playing) | 1 | Esc | 2 | 3 / 4 | 5 / 6 | 7 / 8 | | the arrows |

The d-pad's arrows repeat in the screens (after 0.4 s, every 0.09 s; the left stick past half its throw counts as an arrow there), and
are held in the scene contexts (the ship turns, the explorer walks, the vehicles drive). In the screens the right stick runs at 400
mouse pixels a second at full throw (a cursor or a map's turn, against 900 for a head): the sector map's cursor (at half a throw about
three seconds across the frame), the galaxy chart's, the star map's turn, and the landing map's cursor, which reads the mouse's motion
since R-418 (about 60 degrees a second at full throw, half a degree zoomed in; the mouse is not held there, so only the stick moves it)
while the d-pad steps it a degree (a hundredth zoomed in). The maps' rows above list what their buttons do; the buttons doing nothing
on a map press nothing there, and the menu's legend says what the sticks do. Photo mode on the ground: A up, B down, RT
fast. RB's press on foot or in the drone goes without the pad's own Shift (RT's sprint, the drone's descent), so it is the scanner and
not `Shift+R`. The stick's axes each give way to the keys alone (`updateWalking`, the free camera, `updateBuggy`, `updateDrone`; the
cabin reads `moveX`/`moveY` now too).

**The commands** (Y; `padItems`, `padMenuUpdate`, `renderPad`): a panel over the frame (the game runs on beneath it, the controller
all the menu's) with three pages, LB/RB or Left/Right between them: the context's, named for it (`THE CABIN`, `THE AIM`, `THE
TELESCOPE`, `THE RADAR`, `ON FOOT`, `THE BUGGY`, `THE DRONE`, `PHOTO MODE`, `LANDING MAP`, `SECTOR MAP`, `STAR MAP`, `THE GALAXY`,
`THE DECODER`, `THE CASE`, `THE DRONE BAY`, `THE PROBE`, `THE RECORDING`, `THE ALMANAC`, `DATA SHEET`, `THE ANALYZER`, `FLIGHT
COMPUTER`, `GOES CONSOLE`, `THE ENTRY`, `HELP`, `VESPERIS` at the title; no page where a screen's own keys are its arrows, Enter and
Esc), every action of the screen in words with its state (`USE THE FLIGHT COMPUTER` by `cabin.facing`, `VIMANA FLIGHT TO THE TARGET`
with a remote target, `FIELD AMPLIFICATOR ON`, `TAKE THE RECORD`, `GET IN THE BUGGY`, `AUTOWALK 6`, `THE SCANNER: MARKED SIGHTS IN`,
`FLY THE PLOTTED JUMP`, `PUT A CHART DOWN` ...); `GENERAL`: the guide, the star map, the log, the data sheet, the warp (`TIME WARP X1
TO X10`), the almanac, a photograph, photo mode, the panorama, the recorder, save and load (the slot), help, the scanlines, fullscreen,
`CONTROLLER SETUP`, `AUTO-EXPLORE` (R-419, in space and on the ground), the game menu (each where its key works); `KEYS`: the keyboard below with Shift, Ctrl and Alt latched for the next
key (every key the game reads: the arrows are the d-pad's and every F key has its twin, B-002). A line runs as its keys (`padRunItem`: a key with its modifiers into a queue delivered one a frame; a mark's removal pressed
twice; the console's commands typed letter by letter with Enter: `SL: THE STARS NEAR`, `DL`, `WHERE`, `HOME`, `PREV`, `CAT`, `HELP`)
and closes the menu; A or X runs, B, Y or START close. Each line's right names the button that does it at once in the context (cyan)
or the key it presses (`(CTRL+T)`); the panel's foot lists the context's buttons in words (`A JUMP  B CROUCH  X USE ...`).

**The keyboard** (`padKeyboardUpdate`, `renderPadKeyboard`): seven rows of ten units (`A`-`T`, `U`-`Z - . / =`, the digits, `SPACE
BKSP DEL ENTER`, `SHIFT CTRL ALT TAB ESC`, `PGUP PGDN HOME END`); the arrows move over it (up and down keep the place across rows of
other widths), A presses the key under the cursor with the latches. It comes up by itself at a text entry and the console while the
controller is the device in use (`padInUse`: the last input came from it; a key or the mouse takes that back), docked at the bottom
(the text entry's box moves up to 18-78; the console keeps its last eight lines and the prompt over it), the cursor on `A`: X deletes,
Y puts a space, START is Enter, B Esc, SELECT opens the commands over it (the console's commands). The aim's light years take it from
the commands (`JUMP: TYPE THE LIGHT YEARS`, `kbFor` 1): docked at the top, clear of the crosshair and the jump's line, the cursor on
`1`; START jumps and closes it, B closes it and leaves the aim up; it goes when the aim does.

**The controller page** (the CONTROLS screen's second page, below; `updatePadScreen`, `renderPadScreen`): the device (`TEST PAD -
STANDARD LAYOUT`, `... - NOT RECOGNISED, READ RAW`, `NO CONTROLLER CONNECTED`), the sixteen buttons in two columns of eight with
their controls (a button's name lit while it is held: which is which), `WALK STICK` and `LOOK STICK` with their axes' names and live
values, `LOOK SPEED` 0.2-3.0, `INVERT LOOK`, `DEAD ZONE` 5-40%, `RESET THIS CONTROLLER`. Up and Down go down a column and on to the
rows under them, Left and Right cross the columns or change a setting. Enter on a button waits six seconds for the next press of
anything (`padCapture`: a button or a hat's direction as it goes down, never one held before; an axis's half 0.6 from where it rested
when the wait began, a trigger resting at one end read whole; the standard names first) and binds it; a control another button had
takes this one's old control (two roles never share one: `A IS NOW Y, Y TOOK A`). A stick takes two pushes, up (its Y axis: the
other half is down) then right, each after the sticks are back at rest. Six seconds, Esc or the controller's going leave it as it was
(`NOTHING SET`); the next frame takes what is held as held (`resync`), so the press that set a button does not act. Esc saves
the settings and the key map, and `vesperis_pad.txt` when a binding changed (`PadMap::dirty`: an explorer's file is not rewritten for
nothing, and none is made with nothing to keep), and goes back where the screen was opened from (`keysReturn`).

A new controller is announced once where the status shows (`CONTROLLER: <NAME> - Y OPENS THE COMMANDS`; read raw: `<NAME>: NOT
RECOGNISED - SETTINGS > CONTROLS SETS ITS BUTTONS`). The title reads `A: CONTINUE      Y: NEW EXPEDITION, HELP, QUIT` while a
controller is in use. The help's third page lists the buttons by context (Space goes through the ship's page, the surface's and the
controller's). Harness: `vesperis_test input` (08).

## Photo tools (M6-05)

`Ctrl+P` toggles photo mode in space and on the surface: the HUD goes away and on the surface the camera detaches (`SurfaceView::freeCam`): W A S D fly along the view, Space and C rise and sink, Shift is five times faster, never below the ground and no higher than 400 m above it; the explorer stays where they were and normal play resumes on the second `Ctrl+P`. `Ctrl+Shift+P` writes a panorama, three views at yaw -fov, 0, +fov side by side (`Game::renderPanorama`, `shots/panorama_N.png`, 960x200 at 1x). `Ctrl+R` starts and stops the recorder: every presented frame goes to `movies/take_NNN/frame_NNNNN.png` (a blinking REC on the HUD) for a video tool to assemble. W-06: `P` in the telescope is the telescope's photo (its caption names the target), and `Ctrl+P` hides the reticle with the rest of the HUD.

## Auto-explore (R-419, 2026-10-18)

The game menu's second row, `AUTO-EXPLORE` (the controller's commands, general page: `AUTO-EXPLORE`), opens the chooser (`GameState::AUTO`,
`game/autoexplore.cpp`): six profiles with a line each, `UP`/`DOWN`, `ENTER` begins, `ESC` back; it opens on the profile chosen last
(`auto_profile` in the settings file). WANDERER (a new whim at each star), STARGAZER (stars, skies, long jumps, the telescope and the roof
deck), PLANETFALL (worlds from orbit, a lap round each), SURVEYOR (the ground: rises, water, animals, sights, the buggy, the drone),
ARCHAEOLOGIST (the peoples' worlds: the radar after a transmission, settlements and their records), GIANTS (gas giants, probes dropped
and watched to the end, their moons). The explorer then plays on its own from the scene the menu was opened over: its mind keeps a
plan of steps for the scene and its hands make the frame's input (the keys, the mouse's motion, the stick) eased as a hand's, which
the game reads as the player's (`Game::autoInput` after `padFrame`). No HUD meanwhile (nor the status line or the controller's menu; the
landing map is never shown, its site chosen in its head; the cabin's own screens are drawn), except a fading line in the first five
seconds (`AUTO-EXPLORE: SURVEYOR - ANY KEY ENDS IT`). Any key, click, wheel, a mouse's motion past a few pixels or a controller's button
or stick gives the controls back (`autoTakeover`): that press is not acted on, the warp goes back to x1, the status says
`AUTO-EXPLORE OFF - THE CONTROLS ARE YOURS`. What it does is the expedition's (the log, the guide, the records it takes, the game's
autosaves); it names nothing, marks nothing and saves into no slot. `docs/iteration-3/plans/requests/R-419-auto-explore.md`.

R-420 (2026-10-19): auto-explore's tape. `M` while auto-explore plays (a controller's `SELECT`, whose own key would end it) starts and
stops a tape: the `music` folder in the working directory when it holds a track (the checkout's for `./vesperis`, `~/Documents/vesperis`
for the macOS app; `Game::musicDir`), else the game's own beside the executable (`Game::musicDirsGame` from `platformAppDir`: `music`
beside the binary, the app's `Contents/Resources/music`, a `Vesperis.app` beside a bare binary), four tracks: Jarrad Wright's *Forwards*,
*It's time to wake up* and *BLUE-MEAN-EYES*, Black Sabbath's *Planet Caravan*. A folder's tape is its `playlist.txt`'s tracks (a line
each, `file | author | title | gain in dB`, `#` for a comment; a file listed but missing is left out), then its other MP3, OGG, WAV and
QOA files in the order of their names (`Author - Title.mp3` read as such). The tracks play round and round through a cassette deck's sounds (`game/audio_deck.cpp`): before
the first a cassette put in (the eject key, the door on its spring, the cassette slid in, the door shut on its latch) and the play key
(its ka-chunk, the motor coming up, the tape's hiss), the music about 2 s after `M`; at a track's end the auto-stop, the cassette out
and the next in, about 3.4 s between two tracks; the play key alone (0.55 s) when a cassette was left in; the stop key for `M` again, the music
cut at it. As a track begins its author (amber) and title (white) fade in at the top left over 0.4 s, stay five seconds and fade out
over 1.5. Under the tape the game's own sounds keep 0.35 of their level (about 9 dB down; the deck itself is not ducked). The chooser
and the first seconds' line say `M STARTS AND STOPS THE MUSIC` (`SELECT ...` with a controller) when there is music to play; with none,
`M` says so where a title would be. When auto-explore ends the music fades over 1.5 s; the choice (`auto_music`) and the place in the
tape (`auto_track`, `auto_track_at`) are kept in the settings, and the tape is taken up again, with the play key, as auto-explore next
begins. `docs/iteration-3/plans/requests/R-420-auto-explores-tape.md`.

## Title, attract mode and credits (M6-06)

`C` on the title opens the credits (help page 2). After 20 s without a key or mouse motion the title camera leaves the parked ship and tours the current system, fourteen seconds per body in order, circling at four radii (rings included), with the body in palette bank 2; any input returns to the parked view.

## Saves (M0-04)

Three slots and an autosave, named from `Game::savePrefix` (default `vesperis_save`): `vesperis_save_1.txt` .. `_3.txt` and `vesperis_save_auto.txt`; a legacy `vesperis_save.txt` (version 1, no timestamp) is still read. `Ctrl+S`/`F5` saves to `currentSlot` (also allowed during descent and ascent), `Ctrl+L`/`F9` loads it; the menu's Save/Load open the slot picker (`SLOTS` state, `openSlots`/`updateSlots`/`renderSlots`) whose rows show `saveSummary`: star name, EPOC and where (in space, on the surface, descending, ascending). Autosave (`autosave()`) runs every 5 minutes of real time in a simulating state, on every landing (Enter on the landing map), on every launch (boarding the capsule) and on quit. Continue on the title screen (`loadFromDisk` -> `loadNewest`) takes the file with the newest `saved` stamp, slots first on a tie.

```
vesperis-save 2
saved <unix seconds>
name <star name>
t <game seconds>
warp <time warp>
star <sx> <sy> <sz> <valid 0|1>
ship <x> <y> <z> <yaw> <pitch>
mode <mode> <parkedBody> <localTarget> <orbiting>
park <dirx> <diry> <dirz> <distKm>
belt <targetBelt> <parkedBelt> <rockIr> <rockIa> <rockIy> <rockM> <rockDistKm>   (O3; -1 -1 when no belt is involved)
remote <hasRemote> <sx> <sy> <sz>
surface <0|1>
site <body> <lat> <lon>            (only when surface 1)
player <x> <z> <yaw> <pitch>       (only when surface 1)
capsule <x> <z>                    (only when surface 1)
phase <state> <transT>             (only when saved during DESCENT (3) or ASCENT (5))
buggy <deployed> <x> <z> <heading> <odometer> <in>   (surface only, M8-06)
drone <deployed> <x> <z> <heading> <odometer> <in>   (surface only, R-403; loads landed on the ground where it was, the explorer back in it if they were; a save without the line has no drone)
waypoint <has> <x> <z>             (surface only)
stamina <0..100>                   (surface only)
probe <body> <starSeed> <aimLat> <aimLon> <clock> <launchT> <startRel xyz> <chuteOpen> <chuteClose> <crushBar> <seed> <headYaw> <descYaw> <camYaw> <camPitch> <torn> <watching>   (X-01: a falling probe; a last frame held is not saved; `watching` 1 loads onto its screen)
... <id> <notes> <stormKind> <stormKey|->     (X-04, at the probe line's end: its number, the crossings logged (bits), its storm; a save before X-04 lacks them)
probetrack <n> <clock yaw pitch ...> <m> <clock t ...>   (X-04: the tracks so far, so the recording goes on after a load)
```

Load regenerates the star (D-01: or the drifter, `placeInSector`; the `remote` line likewise) and the system from the sector, restores the ship, drops VIMANA/APPROACH back to STANDBY, and rebuilds the surface site if `surface 1`; a `phase` line resumes the descent or ascent at the same `transT` (the camera path is a function of it). Unknown keys are ignored; the header prefix must match (versions 1 and 2 are read).

The ship's player (C-11, 2026-10-10, `game/player.cpp`): `Tab` at the decoder's worlds turns to THE PLAYER (and back), the page the decoder opens on next. It lists every piece held, every recording held (a text record made on something that sounds: a wax cylinder, a disc, a reel, a drive, a crystal, something that hums) and every recording the radar caught (`Guide::heard`, `AIR:` in cyan: the record's opening as it was on the air), by their worlds and years, a piece by its name or form with its length, a recording by what it was made on, the people at the right; the row under the cursor names its world, what it was made on (or `CAUGHT BY THE RADAR`) and its year. `ENTER` plays one and the list on from it, `SPACE` pauses, `BACKSPACE` stops, `TAB` back to the worlds, `ESC` closes the decoder and it plays on. It plays wherever the ship goes, the cabin, the window, the maps, a Vimana flight: a line under the system's two lines on the space HUD (a note or a page glyph, `A SONG - THE NULO  0:42/1:21`, dim while paused or held); the hum falls to 0.3 and the drive to 0.55 under it. A landing stops it (the descent, the surface, the title). It is held, its place kept, while the decoder plays a record of its own, the radar listens or the gramophone turns, and takes up where it was after (`AudioState::pieceFrom`, `speechFrom`); the synth's piece and speech point into the player's own copy of the world, so nothing the decoder opens moves them.

The drone bay (`DRONES`, C-18, `game/drones.cpp`, `game/drone_draw.cpp`, `civilisations/records-and-grammar.md`): a rack of three shelves against
the cabin's left wall by the front window (`facing` 11, `DRONE BAY - E TO SEND THE DRONES`, `E FOLLOWS THE DRONES` while they are
out, `THE DRONES ARE CHARGING` after; pale yellow; walking stops 0.26 m before its front), the bay's screen in three cells across
its head and a lamp at each shelf (green aboard and ready, amber charging, red blinking while its drone is out); also the flight
computer's devices page (`THE DRONE BAY`, `READY`, `AWAY` or the cooldown's `3:40` beside it). The screen: each drone in lines
with its number, its finds of what it wanted (or its height while it falls or climbs) and where it is; the world's map with the
ground under the ship (a cross), the places searched, the ways flown and the drones; what the ship holds of the world's, in the
decoder's words, `ABOARD: 58 OF ITS 262 SHARDS` and `AND 7 OF ITS 9 INSTRUMENTS` (its peoples together; `READING THE WORLD...`
while it is read); the finds, the newest first, four rows (dim until aboard; `UP`/`DOWN` scroll, arrows when there are more;
R-414: the screen closed once the drones are back aboard, the finds, the drones' counts and the places searched go from it, the
decoder and the case holding them); the header `READY`, `BACK IN 1:52` or `CHARGING 3:40`. `ENTER` sends the drones (refused, in
the status over the footer: `THE DRONES ARE CHARGING - READY IN 3:40`, `THE DRONES GO DOWN FROM AN ORBIT: PARK AT A WORLD`,
`NO PEOPLE LIVED DOWN THERE: NOTHING TO BRING`, `THE SHIP HOLDS ALL THEY LEFT DOWN THERE`) and returns to the cabin to see them go
(`THE DRONES ARE AWAY - E AT THEIR RACK FOLLOWS THEM`); `ESC` or `E` back. While they are out the space HUD has
`DRONES BACK IN 1:52 - 5 FOUND` (and `, 2 ABOARD`) at the probe's line; at the end `THE DRONES ARE BACK WITH 14 FINDS` then their
kinds, each on the decoder at once (an open decoder lists them as they dock); leaving the parking calls them back
(`THE DRONES CAME BACK AS THE SHIP LEFT: 5 FINDS`). Two minutes searching (about 2:35 from the launch to the last dock), then five
minutes charging from their return, on real time.

The instruments' case (`INSTRUMENTS`, C-11, `game/instruments_screen.cpp`): a cabinet on the cabin's left wall behind the GOES console (`facing` 10, `INSTRUMENT CASE - E TO PLAY`, lime, its three shelves brighter with what it holds), also the flight computer's devices page (`THE INSTRUMENTS`, the count beside it). C-18: it opens on its worlds, a folder each (`UNKNOWN (+189 +0 +45 / 2)`, `1 INSTRUMENT`; the world under the ship first; `UP`/`DOWN`, `ENTER` opens one, `ESC` closes the case), a world of two peoples a folder each people, named on its line as the decoder's rows name them (`UNKNOWN (+208 +0 +51 / 5)  THE VASANTHUL`; the head `12 IN THE CASE FROM 2 WORLDS`; the open folder's `UNKNOWN - THE VASANTHUL`). A folder's instruments in a column, the one under the cursor drawn large from its parts as it lies, seen from above, with a ruler of its size (C-17, `game/instrument_draw.cpp`), `A NECKED GOURD` (its make's name; R-414: its name alone, C-11's `OF THE <PEOPLE>` and C-17's `FROM <world> (<sector> / <body>)` gone, the open folder's title naming the world and its people on a world of one too, `UNKNOWN - THE JENGNE`), its scale's line (the register's word at its right), and under it ten bars, one a key, as high as each degree's pitch over the register's first note. `1` to `9` and `0` play the scale's degrees from the register's first note up (past the scale's last, the next period's), held while the key is down; `LEFT`/`RIGHT` move the register a period down or up (`LOWER`/`HIGHER`); `UP`/`DOWN` take another of the folder's; `ESC` back to the worlds. A drum machine's keys are its pads. The machine that played the wax cylinders (C-11's gramophone; C-17: a talking machine, a cylinder box or a listening box, where the people made one) plays its cylinder instead: `ENTER` winds it (a piece of its people's age through its horn, `TURNING 0:12 OF 0:44`), again stops it. The ship's player plays on under an instrument (one may play along); only the machine holds it.

## Audio state

`Game::audio` levels per state: space hum 0.6, engine 0.9 during Vimana and 0.35 during approach (C-11: 0.3, 0.55 and 0.22 while
the ship's player plays); surface hum 0.05, wind from knots and pressure with a tone per place (M4-09: sea 0.25, desert 0.6,
mountain 1.0), surf near water, bird calls by day, one footstep per stride by material, rain, lava rumble and eruptions on molten
worlds, breath while sprinting; beeps 1 lock, 2 arrive, 3 error, 4 UI; thunder with lightning. B-413: the ground's levels are the
surface's alone: `AudioState::surfaceOff` (every level `updateSurface` sets: the wind, the rain, the lava, the breath, the surf,
the birds, the leaves, the hooves, the insects, the buggy's gear and skid, the drone's pods) runs every frame of the capsule's
descent and climb, every frame of the space view and at a load, so nothing of a world is heard in the capsule or aboard (the space
view silenced only the wind, the rain and the lava before, and the rest went on aboard until another landing).
`AudioSynth::render` mixes sines, filtered noise and envelopes at 22050 Hz mono. C-07: `radar` is 1 while the radar camera is on
(the space hum drops to 0.25 under it): the bed of the void (a rumble far down that breathes, a drone of two tones a quarter-tone
apart beating with their octave, a wisp of resonant static wandering between 300 and 1500 Hz, the hiss), thinned as `radarSignal`
(the beam's clarity on the strongest signal, 0.85 and over when locked) rises; `radarKind` names the signal in the beam for its
own sound (a people's: a carrier at 1.2 kHz drifting under the voice; a pulsar: a thump far down at `radarPulseHz` with a growl
falling away after each and a thin whine that circles; a comet: its song, a warble near 92 Hz with a second voice a semitone off,
geysers of hiss, ice cracking, a rumble; a magnetosphere: whistlers falling slowly from 2 kHz, bursts of chorus, a band of hiss
that breathes, a choir of three voices a quarter-tone apart that comes and goes); `radio` marks the piece or the speech on the
synth as the radar's, ghosted by the receiver (the band 250 Hz to 2.6 kHz, a metallic ring at 37 Hz under it, a slow fade,
dropouts now and then) at the clarity, and keeps `Game::frame` from stopping them off the decoder; everything the receiver gives
goes through an echo a third of a second behind with half of it back. The user's second call ("MORE sounds for signals, greater
variety in that cosmic horror direction") added `radarVoice` (the programme a people's transmission carries, `RP_*` of
`peoples/signals.h`: the recording as a voice, whispers, a chant, a numbers station, a loop, or the world's machines: a beacon,
data bursts, a bell tolling, a siren, a murmur of phantom voices, or rarely the piece slowed by `pieceSpeed`; the synth adds the
machines, the hum under a chant, the breath under whispers, the chime or the relay click at a loop's or a numbers station's start,
a tape wow on the content), `radarSeed` (the signal's seed, from which `AudioSynth::rvDerive` draws every natural kind's own
parameters: no two pulsars thump alike) and the bed's events (every twelve to forty seconds something in the static: a carrier
passing, a boom, a moan, knocking, a murmur, a breath, a scream far away). `vesperis_test signals wav` renders the bed over thirty
seconds, three seeds of each kind and every programme to `shots/tests/radar_*.wav` (twenty-one files); `civilisations/` has
the whole list. X-01: `probe` is 1 on the probe's screen and 0.12 in the cabin while one falls (the relay murmuring from the
landing screen): the link's hiss (more as `probeSignal` falls with the depth) and in space a tick a second, the air's rush by
`probeWind` (darker as it deepens, fluttering at 7 Hz under the chute: `probeChute`), the entry's roar and crackle (`probeHeat`),
the deep's rumble and past half the depth the hull's groans (`probeDepth`), a thunderclap through the relay at a flash
(`probeThunder`, one-shot) and at the end only the hiss of the lost signal (`probeLost`); `vesperis_test probe wav` writes each
stage's sound to `shots/tests/probe_*.wav` (rms 0.06-0.10, the entry's peak 0.73). Q-03 (2026-10-22): the
sounds of the ground the world's traits give (`groundSoundsTo` sets them from the surface's cues, `AudioSynth::groundSample` sings them), ground
levels that `surfaceOff` clears: `dunes` at `duneHz`, the singing dunes' hum (the note and four harmonics, pure and wavering, buzzing with the
grains' collisions, over the sand's hiss: 70-100 Hz under an Earth's gravity, 30-50 on a Titan or a Mars; a slide's hold rms 0.10, twice the
full wind's); `ice`, the singing ice (a near crack and its flexural wave's chirp falling as 1/t^2 from 4-7 kHz to under 1 kHz in a tenth to a
fifth of a second, a far crack's duller chirp over a second, the sheet's low whoop, the snow settling: a sound every 0.4-3.6 s, oftener with
the level: 18 in twenty seconds at dusk, 8 at noon); `quake` and `quakeHit` (the P-wave's thump at the onset, then the rumble under a slow
heave, the air's roar over it and the stones' rattle: peak 0.53; on an airless world the rumble alone, through the boots); `air`, how much of
them the air carries (the wind's rule: whole at a third of a bar, 0.15 in the thinnest). `vesperis_test sounds` renders each to
`shots/tests/sound_*.wav` with its levels; the scenes `singing_dunes`, `singing_ice` and `quakes` hear them on the ground.

D-03 (2026-10-23): a drifter's voice (`civilisations/radar.md`, the drifters' voices; `game/audio_rogue.cpp`). `radarKind` `SIG_ROGUE` plays it
from `radarSeed` and `radarRogue` (`RogueVoice`: its class by the body, the period of its turning and of its innermost moon's orbit, its
power; `rogueVoiceFor` reads them off its signal), the one under it from `radarRogue2`; `radarVoice` with a drifter is the beacon under its
voice (D-05's slot: a programme of C-16's machines, slowed and darkened; -1 none). `radarSet` is where the receiver is heard: the radar
camera (`RADIO_CAMERA`), the cabin's radio (`RADIO_CABIN`) or the suit's (`RADIO_SUIT`: the whole receiver through a small set's band of
300 Hz to 3 kHz). `Game::darkRadio` runs each frame after the state's update: while the ship is at a drifter (its system, out of a flight)
and the camera is off, the cabin's radio carries the drifter at `radar` 0.45 (`RADIO_CABIN_LEVEL`; 0.35 of it under the ship's player),
clarity 0.9, x1; on its ground (the state the surface's, or a sheet or a menu opened there) the suit's at 0.4 (`RADIO_SUIT_LEVEL`); nothing on
the title, the landing map, the capsule's descent and climb, the decoder, the instruments, the probe's screen and its recordings, the drone
bay. The voice costs the synth about 7 ms a second of sound (a magnetosphere's 2.8, the bed's 2.1). `vesperis_test rogues voices` writes
twelve of them to `shots/tests/rogue_voice_*.wav`.

T-01 (2026-10-09): the surface scanner's RUINS mode hears a people's world's plan by its places' sizes (`ruinsNear`): beside the ruins of
the cells within 40 km, a capital from 400 km, a city from 200, a town of the plan from 80, a band's seat from 40, a futuristic people's
great places from 300 and the lesser from 120, a far-future work from 1,500; the readout names the class (`A CITY`, `A GREAT CITY` for
the capital), the nearest still the one shown. A world's plan is read on a thread of its own when the ship starts its approach (and at
a parking, the landing map and a load), so the landing finds it built; the landing zoom (`Z`) draws its cores and its ways with the
peoples (`V`, R-415).
