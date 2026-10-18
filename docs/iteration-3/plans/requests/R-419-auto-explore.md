# R-419: auto-explore

status: done 2026-10-18
milestone: 1.3.2 (the fourth of the patch's items)

## Request

"Next thing I would like you to include in this patch  - autoexploration. Some user requested it so game plays itself. What I suggest -
severeal profiles (like focus on space, on planets, on surface of planets, on civilizations, gas giants, random - you choose and give
ideas). Also I suggest no HUD while it's on and make sure movements are soft, non-robotic, everything is in chilling manner. So it feels
like real person is playing"

## What was done

* **Where it starts** (`game/autoexplore.cpp`): the game menu's second row, AUTO-EXPLORE (the controller's commands too, on their
  general page), opens a chooser of six profiles, a line each, on the one chosen last (`auto_profile` in the settings file). Enter
  begins it from the scene the menu was opened over, in space or on the ground. For its first five seconds a line at the foot says what
  plays and how to stop it (`AUTO-EXPLORE: SURVEYOR - ANY KEY ENDS IT`, `ANY BUTTON` with a controller in use), fading out; then nothing
  is written on the screen.
* **The profiles** (the user's six, named; each a taste the same mind plays with):
  * **WANDERER** (the user's "random"): a new whim at each star, one of the other five, the system's own leading (its giants, a people's
    world, ground to walk on).
  * **STARGAZER** (space): stars by their class, the strange first (a black hole, a Wolf-Rayet star, a protostar, a pulsar or a neutron
    star, the giants; a red dwarf seldom), a long jump of 60 to 3,000 ly a third of the time, none to two worlds of a system, and at
    them the telescope, the roof deck and the glass hull often; it hardly lands.
  * **PLANETFALL** (planets, from orbit): three to six worlds of each system, ringed worlds, worlds with moons and living worlds first; a
    lap round each (the parked ship carried round its world, R-405's), the telescope on it or on a moon; down for a short look now and
    then.
  * **SURVEYOR** (the surface): down nearly everywhere it can (living worlds and deserts first, a grass or forest site on a living
    one), four to eight minutes on the ground: a rise within a few hundred metres and the view from it (reachable on foot: no stretch of
    the way steeper than a grade of 0.45), the water's edge, the animals walked up to 18 to 30 m off and watched (birds overhead
    otherwise), a sight the scanner hears (a canyon's or a crater's rim, the geysers, the crystals, near a peak's summit, a lake's
    shore), a drive in the buggy down the longest dry, gentle run, a flight in the drone with a ring round a sight, the sky.
  * **ARCHAEOLOGIST** (civilisations): the peoples' worlds alone. A neighbour star with one, else the radar swept for a people's
    transmission (the camera's picture alone, the beam run past the signal and back as the meter rises and falls until it locks, a
    while listening to what the people sent, then Enter flies to it), else a long jump. Down beside a place of the world's plan in the
    light (a capital not visited yet seven times in ten, where its people's records gather; else a city half the time), the settlement
    with the most records left for its distance, its records walked to one after another, the nearest first (looked at a moment, taken
    with `E`; an instrument lying nearer is taken the same way; a look round the ruins after the first), three to eight a visit; a record
    more than 400 m off (across a city two kilometres wide) reached by the drone, a ring over the place and set down near it; the works
    the scanner hears. A system without a people's world is passed through.
  * **GIANTS**: systems with gas giants (and brown dwarfs), each giant from orbit, its bands through the telescope, a probe dropped and
    its camera watched to the end (the fall and the descent in stretches at Shift's eight times with breaks at their own pace, the chute
    once in the air, the eyes round the clouds, the last frame a while, then back to the cabin), a giant's moon visited with the giant
    20 to 50 degrees from the zenith.
* **The mind**: a short plan of steps for the scene it is in (the cabin and the ship, the ground, the probe's camera), made again when it
  runs out or the scene changes; each step runs every frame until it is done or given up after its time. The space plans: to the front
  window, the arrival (the star, then the worlds in the window), a world turned to and approached (the nose eased onto it, a moment,
  Enter on the local target), the parking (the eyes over the disc and its limb, the glass hull, a lap, the roof deck, the telescope a
  few powers closer at a time), the landing, the probe, the move to the next star (turned to, targeted, the Vimana; a long jump plotted
  first, its star aimed at, then flown, as the galaxy chart's two Enters do). The ground's: a look round, the outings by the taste (not
  the same twice running), and home (out of a vehicle, the capsule called when it is far, a last look, aboard with `Q`); night falling
  sends it home sooner. The landing map is never shown: the site is chosen in its head (a lit, dry place by the world's map, the sun
  well up and low a fifth of the time; a place of the plan; a giant's moon) and the map takes it with Enter on its first frame.
* **The hands** (`autoHands`): the step of the moment says where to look, where to go and which keys; the hands make the frame's input
  from it as a hand gives it, and the game reads it as it reads the player's, so every rule holds (the walls, the slopes, the water, the
  vehicles' handling, the reach of a record).
  * The gaze: a critically damped spring onto the target in the look's own frame (the head in the cabin, the ship's nose in the cockpit
    view and the radar's camera, the explorer's look, a vehicle's camera on its gimbal within its pan and tilt, the probe's camera), at
    most 43 degrees a second and 80 a second squared at a walk's pace, a third of that for a long look; at rest it sways three slow
    waves of a few tenths of a degree. A turn of the game's own (the approach's aim, a camera's limit) is taken up as the new rest, not
    fought.
  * The stick: pushed and let back (from rest to full in half a second), never a key's on and off; on foot along the way relative to
    where the eyes point (slowed in a sharp turn and before the stop; the sprint on long legs while the breath allows; with the toggle
    setting, a press when it should change); the buggy's throttle eased toward the speed the way allows and its steering held to a calm
    turn (about 35 degrees a second at any speed: at low speed its full lock turns it a hundred); the drone's at most half the stick (35
    degrees a second, not its 70).
  * The ship: its nose eased onto an aim (26 degrees a second; the arrows turn it at a key's pace and `X` snaps); a lap round the
    parked world eased in and out over a second; the clock's pace changed a decade at a time over a second and a half (x100 at most, to
    watch a sky wheel from the ground).
  * Before the game turns the view itself it looks the same way, so the turn is small: the head to the front before the Vimana and
    the telescope (both face the front window at once), along a vehicle's heading before its seat (the seat faces it), the camera to the
    front before stepping out.
  * **The fine approach** (the game's own, a player's too): its harness found the one fast turn left at every parking, the straight
    line to a parking beside the world with the nose on the world's centre swooping the view 60 to 350 degrees a second in its last tenth
    of a second. The approach is now taken from the world as the probe's fall is (the distance closed by equal ratios, the direction
    swung onto the parking's in its first seven tenths, the last stretch straight in): the same start, end, duration and aim; the world a
    speck a little longer, then growing steadily and holding still as the ship arrives.
  * In the cabin it stands at the front window between its bars (they stand at 0 and 1.25 m: in the middle of the window one stood in
    the middle of the view), at a side window for a while on a long flight, walks round the capsule's cage to it and presses `E` facing
    it to land or to drop a probe.
* **The walk's way** (`autoWay`): in legs of up to 24 m, a grid of 40 cm cells round the leg; the colliders standing at a body's height
  marked a body's width wide (a settlement's walls are rows of discs: a doorway stays open), a step steeper than the walk climbs blocked,
  the deep water dear; A* to the goal or the reachable cell nearest it, the path pulled straight where its cells see one another. No
  progress for three seconds: a jump with the jetpack over it (not on the lightest worlds), the leg planned again; given up after five,
  and a walk's time is its length at a metre and a half a second.
* **The vehicles**: the buggy unfolded at the capsule (called to the explorer with `K` when it is far), watched as it unfolds, driven
  round water and slopes too steep ahead (the goal's heading or the nearest clear one either side, looked at twice a second), a stall
  backed out of; the drone up to 50-130 m over the highest ground of the way ahead, a ring round a sight with its camera on it (its 70
  degree pan keeps the sight in the frame from the ring), set down on dry, gentle ground. The chase camera now and then.
* **No HUD**: the space HUD, the ground's, the capsule's caption, the probe's frame and the radar's are not drawn while it plays, nor
  the status line or the controller's menu; the cabin's own screens are (they are the room). The landing map's frame draws the cabin
  still.
* **The controls given back**: any key, click or turn of the wheel, a hand on the mouse (six pixels at once or fourteen within a moment;
  none in a scene's first half second, when the window takes the mouse) or the controller's buttons and sticks end it; that press is not
  acted on (`E` takes nothing, Esc opens no menu). The warp goes back to x1, what its steps queued on the status line is dropped, and
  the status says `AUTO-EXPLORE OFF - THE CONTROLS ARE YOURS`; the HUD is back.
* **What it does to the expedition**: it plays the expedition, so its arrivals, landings, roads and graves passed, sightings and the
  records and instruments it takes go into the log and the guide as the player's would, and the game's own autosaves run (every five
  minutes and at a landing or a launch). It names nothing, marks nothing, saves into no slot, never opens a screen (one opened by chance
  is closed with Esc) and leaves the ship's player alone.

## Ideas (not built)

* **Begin when idle**: a setting to start the last profile after some minutes untouched, as a screensaver does (off by default).
* **A photographer**: a profile, or a switch, that takes a photograph at its best moments (a giant over its moon, a sunset from a
  rise, a settlement from the drone's ring) into `shots/`.
* **Nightfall**: dusk and night on the ground, the almanac's events run to (an eclipse, a meteor shower's peak, an aurora's night).
* **Captions as an option**: a documentary's line when it arrives at a star or lands (the name or UNKNOWN, the type), fading.
* **Readings**: the archaeologist taking its records to the decoder in the cabin and reading one aloud between worlds; the drone bay
  sent down at a people's world; an instrument it found played at the case.
* **A journey's end**: a time limit after which it flies home (or to the system it began in) and parks.

## Verified

* `vesperis_test auto <profile> <minutes> <seed> [cockpit]` (new, `08-testing-and-tools.md`): a fresh expedition from home played for
  that many minutes of frames, its steps as they change, a frame every 20 s (`shots/tests/auto_<profile>_NNN.png`; a step's twice a
  second with `VESPERIS_AUTOSHOT`), what it did and how the eye moved. The last round, twelve minutes each (seed 31): the Wanderer four
  stars and nine worlds (no landing this time); the Stargazer nine stars and seven worlds, long jumps, the telescope, the roof deck, the
  glass hull; Planetfall four worlds, three landings and seven outings; the Surveyor two landings and five outings (two drone flights
  with their rings, a drive, two rises); the Archaeologist two people's worlds, three settlements, thirteen records (a capital 2.3 km
  across reached by drone, an archive's row emptied); the Giants profile three stars, five worlds, a landing and a probe (an earlier
  run watched one to its end, two and a half minutes); Planetfall in the cockpit view five worlds and a landing in eight. Outside the cuts (a scene's change and the
  game's own snaps, a turn over 12 degrees in a frame) the eye's fastest turn was 43 to 56 degrees a second in every run and no frame
  turned faster than 60 (before the approach was smoothed, its end alone reached 60 to 350 in every run with parkings).
* Tried and changed on the way: the window's middle bar stood in the middle of the view (it stands between them now); a rise up a steep
  airless slope took three minutes of the explorer's face against the ground (a rise is reachable on foot now, a walk timed by its
  length); the archaeologist landed by villages of two records and flew kilometres for one (a capital first now, the settlement by its
  records); a capital's middle a kilometre into its streets cost a walk of six minutes (the records first, the drone past 400 m); the
  Vimana's and the telescope's snap of the head to the front, a vehicle's seat (looked toward first now); the drone's turn at full lock
  and the buggy's at low speed (a calm turn's share of the stick).
* `unit` 338 checks, 0 failures (one new: begun from the menu's second row on the profile chosen last; twenty seconds from home's parking
  walk to the window and look at the world, the fastest turn 37 degrees a second; G gives the controls back and opens no guide);
  `regress` 24 of 24 unchanged; `flow` passes (its settings shot had pressed into LOAD since the new row: fixed, and the chooser shot
  added, `flow_auto.png`); `input` passes (26 lines, the commands' general page a line longer).
