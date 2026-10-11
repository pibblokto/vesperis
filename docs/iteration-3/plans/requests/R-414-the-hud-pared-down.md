# R-414: the HUD and the screens pared down

status: done 2026-10-11 (uncommitted)
milestone: 1.3.0 (after R-413; the same message's last part, the landing map's peoples, is R-415)

## Request

"Now let's refactor all the HUD/UI we have since it's overblown at times. First, surface:

1. Remove all indicators of how far capsule/drone/buggy are - there is no need for that if you can always call capsule in 1 button and
   buggy/drone in 2
2. All the indicators of elevation, longitude, local tme, elevation, temp, grav, wind, rain and what not should be hidden in a separate
   menu you can open by some key - maybe there it is already, then just remove it from the screen. If it's not there - add this key and
   remove it from the screen. Sector number from the top left should also go there
3. Eopoc, name of the planet with coordinates, scanner status and this compass stay, stamina also

Second, drone:

1. turn indicator at the bottom stays, camera indicator at the top (where it's facing) stays, REC. stays. compass with scanner indicator
   (scanning this, how far) also stays as when you are on foot
2. Remove epoc when in drone
3. local time, hdg, alt, vs, trip and other things (pan, tils, cam 02 drone) - gone, descent and climb also. Only keep speed

third buggy: the same as drone

Not UIs:

First, remove these lines that show what routes you traveled - they seem to be a good idea but they confuse you a lot. You can find them
with button N and also in galaxy map that shows where you gone (green lines, attached screenshot)

Shards - there is this overexplanation, like "The Reel: Rough voice that clicks" - no need please, just say what kind of medium this is,
"The Real", or printed pages "read by the ship" - again no need. For all the shards just explanation of what it is, it would be super
enough along with pretty representations of what shard was that you added (good work on that btw, it's pretty)

when you are selecting an instrument you first go to name of the planet. On the right you again see that it's "From <planet>" and
coordinates. No need for that - you already know where it is from when you open a name of the planet/civilization in instruments case

For drones rack there all the findings should be cleared after you took a look at them. You don't want this list of findings to stick
around, no need - it will appear in decoder/instruments case. So drones are back, you open, see what they found, close rack and that's it"

The screenshot was the galaxy chart with two green legs from HOME to a star by the core and on to YOU.

## What was done

* **On foot** (`renderSurfaceHUD`, `game/hud.cpp`): the world's name and the place (top left), the epoch (top right), the compass with
  the scanner's mark, the scanner's two lines (moved up a row, under the name) and the stamina with the pulse. Over the stamina, what E
  or Q would do here, stacked from the bottom: `CAPSULE HERE - Q`, `BUGGY HERE - E` or `DRONE HERE - E` (new: the distances are gone,
  so the vehicle at hand says so), an instrument's or a record's `- E`, `A GRAVE: ...`. The jetpack keeps its heat (`JET HEAT 45%`), the
  one gauge besides the stamina. Gone from the view: the capsule's, the buggy's and the drone's distances, the sector, the sun's line,
  the local time, the elevation, the temperature, the pressure, the gravity, the wind and the weather, the life sensor, the old road,
  the rangefinder and the mark under the crosshair, SWIMMING, the jetpack's height and climb, AUTOWALK and the vision's name.
* **The readings** (`Tab`, `drawReadings`): a panel at the right under the epoch's lines (rows of 23 characters at most, two readings to
  a row where they fit, so it stops short of the crosshair) with all of that: the sector, the local time and the sun, the elevation and the temperature, the gravity and the pressure (a comet: its escape speed), the wind and the weather,
  the life sensor, the old road, `AHEAD 287 M 68 S` and the mark the crosshair rests on, and the walker's own (swimming, the
  jetpack's height and climb, the autowalk, the vision). In a vehicle the panel holds its numbers too: the drone's heading, trip,
  height, climb and state (`LANDED`, `AT ITS CEILING`, `LIGHTS ON` ...), the buggy's heading, trip, gear and state. Tab again puts it
  away; it stays as left for the session. Help page 2 and the key list (`LIST / READINGS`) say so.
* **The drone and the buggy** (the nose camera, one branch for both): the brackets, the blinking `REC` (where `CAM 02 DRONE` was), the
  pan gauge under the compass, the crosshair, the speed (`KM/H BACK` reversing, the buggy's too now) and the turn bar under the picture.
  Gone: the epoch, `CAM 01 NOSE`/`CAM 02 DRONE`, the pan and tilt, the height and the climb, the trip, the gear, the heading, the state
  (CLIMB, DESCENT ...), the capsule and the local time. The compass and the scanner's lines stay as on foot.
* **The ways travelled** (G-05's route, M4-02's trail): the galaxy chart draws the stars visited and no line between them; the guide
  keeps no `route` lines (an older guide's are skipped at its load and gone at its next save); the N map draws no trail of where the
  explorer walked (`SurfaceView::trail` is gone).
* **The decoder's medium line** (`mediumLine`, `game/medium_draw.cpp`): the object alone (`PRINTED PAGES`, `A REEL`, `A WAX CYLINDER`);
  a piece on a medium without a sound of its own says what it is (`A BONE FLUTE`, `NOTATION ON A SCROLL`, `PRINTED MUSIC`). Gone: the
  voice (`A REEL: A DEEP, ROUGH VOICE, SLOW, WITH CLICKS`), `READ BY THE SHIP`, `PLAYED BY THE SHIP`, `THROUGH ITS HORN`. The same line
  on the history's page. The ship still reads a record without a voice in its flat voice; the line no longer says so.
* **The instruments' case** (`renderInstruments`): the open folder's title names the world and its people on a world of one too
  (`UNKNOWN - THE JENGNE`); an instrument's line is its name alone (C-11's `OF THE JENGNE` and C-17's `FROM UNKNOWN (+208 +0 +51 / 5)`
  are gone), its tradition's line and the register's word a row higher.
* **The drone rack** (`droneFindsSeen`): closed after the drones are back aboard, the rack keeps no list: the finds, the drones'
  counts and the places searched on its map go (the guide holds the finds: the decoder and the case show them). Closed while a trip is
  still on, it keeps them.

## Verified

* **The frames** (`flow`): `surface2` the walker's view (the name, the epoch, the compass, the stamina), `surface_readings` with `Tab`
  (`SECTOR 357:048`, `LOCAL 10:20`, `SUN +51 AZ 041`, the elevation and the temperature, the gravity and the pressure, the wind, `LIFE
  71 M SW (FLYERS)`, `AHEAD 287 M 68 S`; the first draft's panel at the left, 29 characters wide, hid a vehicle's crosshair), `scan_hud` (the scanner's lines under the name, `DRONE HERE - E` over `CAPSULE HERE - Q`),
  `drone_flight` and `buggy_drive` (the frame, REC, the pan gauge, the speed, the turn bar), `drone_readings` and `buggy_readings`,
  `sectormap` without the trail; `drones frames`: `drones_case_world` (`UNKNOWN - THE JENGNE`, `BOWS ON A POT`), `drones_screen_seen`
  (the rack after its screen closed: no list, no places); the decoder's `shards_speaking` (`PRINTED PAGES`).
* **The checks**: the galaxy chart's (three arrivals visited, none of the route's green on the chart, no `route` line in the guide),
  B-303's (the waypoint kept across a re-anchor and cleared by a landing), the rack's (12 finds listed, none on the rack once closed, the
  guide's unchanged).
* `unit` 0 failures (313), `regress` unchanged (22 hashes: they hold no HUD), `flow` 35.0 s with no failure, `drones frames` 0
  failures, `bench check` within every budget.
