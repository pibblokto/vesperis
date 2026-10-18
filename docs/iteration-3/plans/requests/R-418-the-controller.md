# R-418: the controller

status: done 2026-10-17
milestone: 1.3.2 (the first of the patch's items)

## Request

"I want you to revamp gamepad/controller support - I tested it and it somehow works but many bindings for all the features are
missing - some guy with AYN Thor reached me and said he managed to run it but bindings were fucked up. What I suggest - move tons of
binding to some menu that will be gamepad/controller specific, like you open menu and have options you select from instead of keys -
this way we can be sure game is playable if bindings are wrong (however, configuring bindings should still be an option, I am just not
sure how it can work out of controller can be literally anything, I tested it with dualsense, someone with Thor and another person will
do it with xbox controller)."

M6-02's pad reached a handful of keys through one fixed table: `E` (use, take, board, every console of the cabin) sat on the guide
button, which the systems keep for themselves (the PS button, the Xbox button, Android's home); `M` on a sixteenth button no pad has;
the left stick walked on the ground but not in the cabin; the right stick's look ran with the frame rate and the mouse's sensitivity
twice over; and the rest of the game (the aim, the Vimana, the targets, the maps, the scanner, the marks, the decoder, the instruments,
the names) had no button at all. raylib reads only the controllers GLFW has a mapping for, and only through it: a device nobody mapped
read as nothing.

## What was done

* **The controller read whole** (`platform_raylib.cpp`): GLFW's joystick calls (inside raylib's static library: the dev Makefile now
  links Homebrew's `libraylib.a`, as the release scripts link theirs) give every button, axis and hat of the first controller (the
  first GLFW knows the layout of, else the first with four buttons: a pad's motion sensor shows up as a joystick of axes alone), and
  its standard layout when GLFW's mapping knows it (`PadState`: the Xbox names by place, a PlayStation pad's cross being A). A
  `gamecontrollerdb.txt` beside the game (SDL's controller database, the community's file or a line of one's own) is read at the
  start and teaches GLFW the layouts its own list lacks.
* **Roles, bound to anything** (`core/input.*`, `PadMap`): the game reads sixteen buttons (A B X Y by place, LB RB LT RT, SELECT,
  START, L3, R3, the d-pad) and two sticks, each bound to any physical control: a standard button or axis half, a raw button, a raw
  axis's half, a raw axis read whole as a trigger from its rest, a hat's direction. A known device starts from its standard layout; a
  raw one from a guess (buttons 0-3 the face, the bumpers, SELECT and START, the thumbs; the first hat the d-pad; axes 0 and 1 the left
  stick; with six axes whose third and sixth rest at one end the Linux order, triggers there and the right stick on the fourth and
  fifth, else the right stick on the third and fourth), which binds no stick axis resting off-centre and no trigger resting in the
  middle (it would walk, or hold a button, by itself). What the explorer binds is kept by device (`vesperis_pad.txt`, a `pad
  <device> <name>` line and `ROLE CONTROL` lines under it).
* **Each context's buttons** (`game/pad.cpp`, `Game::padBindings`): the controller presses the game's own keys, so every action keeps
  one code path. Everywhere Y opens the commands, START is Esc (the menu, back), SELECT is Tab, the d-pad the arrows and the right
  stick looks (its own speed, curve and inversion, the mouse's taken back out; per second now, not per frame). The screens: A Enter,
  B back, X Space, LB/RB the pages, LT/RT the zoom, L3 Shift, the left stick the arrows, both repeating after a moment held. The maps
  each their own: the landing map A lands, LT/RT its sector zoom (`Z`), L3 the 6-degree steps, and the right stick moves its cursor
  smoothly (it read no mouse; the d-pad steps a degree); the sector map A marks, X brings the cursor home, SELECT the next mark,
  LT/RT zoom, L3 faster; the star map A targets, LT/RT zoom, SELECT the galaxy, the right stick turns it; the galaxy chart A plots and
  flies, X the cursor to the ship, LB/RB its height, LT/RT its levels, SELECT the neighbourhood. On the maps the right stick runs at
  under half a head's pace (a cursor's). The title: A or START begin (B quits nothing). The ship: A Enter (approach, lock the aimed star, track, fly to the
  radar's lock), B leaves the aim, the radar or the telescope, X uses what the explorer faces, LB/RB the roof deck (the telescope's
  zoom, the radar's gain), LT the local target (the next star in the aim), RT Shift (with the d-pad round the parked world), L3 the
  aim, R3 the centring; the left stick walks the cabin. On foot: A jump (the jetpack held), B crouch (dive in the water, down in the
  air of the lightest worlds), X use, LB the sector map, RB the scanner, LT stand tall, RT and L3 sprint, R3 the vision modes, SELECT
  the readings. The buggy: RT/LT throttle and reverse with the stick's steering, A brake, B and R3 the view, X out. The drone: A and RT
  climb, B and LT descend, X out, R3 the view. The probe: A the chute (let go at its end), B the cabin, X centre, RT fast. A recording:
  A play/pause, LB/RB the layers, X centre, RT fast. An instrument: A X LB RB LT RT L3 R3 its first eight degrees, the d-pad the
  register and the next one. The left stick's two axes now each give way to the keys alone (a trigger's throttle with the stick's
  steering), and walk the cabin as they walk the ground.
* **The commands** (Y): a panel over the frame with three pages, LB/RB (or left/right) between them. The context's (named by it: THE
  CABIN, THE AIM, THE TELESCOPE, THE RADAR, ON FOOT, THE BUGGY, LANDING MAP, SECTOR MAP, THE GALAXY, THE DECODER, THE CASE, THE PROBE,
  GOES CONSOLE, VESPERIS at the title ...): every action of the screen in words with its state (`FIELD AMPLIFICATOR ON`, `USE THE
  FLIGHT COMPUTER`, `TAKE THE RECORD`, `AUTOWALK 6`), run as its keys; the console's commands typed and run (`SL`, `DL`, `WHERE` ...);
  a mark's removal pressed twice. The general page: the guide, the star map, the log, the data sheet, the warp, the almanac, the photo
  tools, save and load, help, the scanlines, fullscreen, CONTROLLER SETUP, the game menu. The keys page: a keyboard of every key the
  game reads (letters, digits, `- . / =`, Space, Backspace, Delete, Enter, Tab, Esc, PgUp/PgDn, Home/End; the arrows are the d-pad's
  and every F key has a letter's twin, B-002) with Shift, Ctrl and Alt latched for the next key: nothing a key does is out of a
  controller's reach, whatever the bindings. Each line names the button that
  does it at once here (`LT`, `SELECT`) or the key it presses (`(CTRL+T)`); the panel's foot lists the context's buttons in words.
* **The keyboard**: at a text entry and the console it comes up by itself while the controller is the device in use (a key or the
  mouse takes it back): A types the key under the cursor, X deletes, Y a space, START is Enter, B Esc, SELECT the commands over it.
  The aim's light years take it from the commands (`JUMP: TYPE THE LIGHT YEARS`), at the top of the frame clear of the crosshair and
  the jump's line; START jumps. The text entry's box moves up over it; the console keeps its last lines and its prompt above it.
* **The CONTROLS screen** (the settings' `KEY BINDINGS` row renamed; `Tab` between its pages; the commands' CONTROLLER SETUP opens it
  from anywhere and it goes back there): the keyboard's page as before, and the controller's: the device's name and whether it is
  known or read raw, the sixteen buttons in two columns with the control each is bound to (lit while held: which button is which),
  the two sticks with their axes and live values, the look speed, its inversion, the dead zone (`pad_look_speed`, `pad_invert_y`,
  `pad_dead_zone` in the settings file), and the reset. Enter on a button takes the next press of anything (a button, a trigger, a
  hat, an axis's half); a control another button had changes places with this one's; a stick takes two pushes, up then right; six
  seconds or Esc leaves it as it was. Esc saves the settings, and `vesperis_pad.txt` when a binding changed (none is made with nothing
  to keep).
* **The rest**: a new controller is announced once (`CONTROLLER: DUALSENSE WIRELESS CONTROLLER - Y OPENS THE COMMANDS`; one read raw:
  `... NOT RECOGNISED - SETTINGS > CONTROLS SETS ITS BUTTONS`); the title says `A: CONTINUE  Y: NEW EXPEDITION, HELP, QUIT` while a
  controller is in use; a third help page lists the buttons by context; the keyboard page's key capture is cancelled by a
  controller's B too (it waited for a key of the keyboard); RB's scanner while RT sprints is the scanner, not `Shift+R`. The
  example key file named the old game (`noctis_keys.txt`): `vesperis_keys.txt` now.

## Verified

* **`vesperis_test input`**, 24 new lines, all PASS: the standard layout (A, a trigger from its rest, the round dead zone); a raw
  device's guess; the guess reading the axes at rest (an off-centre axis unbound; a DirectInput order); the capture (a button by its
  press and not one held, a stick's half, a hat, a trigger resting at one end, the standard names first); every control's name read
  back; a set for each controller in the file (the default left out, another device untouched, the reset). Then the game driven by a
  controller alone: A begins at the title; the announcement; the left stick walks the cabin and the right stick turns the head; X uses
  the flight computer and B leaves it; the commands' pages and `FIELD AMPLIFICATOR ON` run from them; `F` from the keys page; the
  aim's light years typed (`100`), the keyboard closed and the aim left; a star named `AB` through the guide and the keyboard; the
  console's `WHERE` run from the commands over its keyboard; the controller page (A taken from Y, Y swapped to A and then pressing
  Enter, the walk stick from two pushes, the reset); the help's third page; the star map (the right stick's turn of 0.28 rad in half a second at 0.7 of a throw, RT's zoom, the menu's class
  filter and back) and the galaxy chart (SELECT to it, the left stick's cursor off the ship, RT a level in, a place marked through the
  keyboard and removed from the menu, X back to the ship); the landing map's cursor (60 degrees east in a second of the stick, the
  d-pad a degree) and its triggers zooming; a landing by A, a jump
  of 1.5 m, a sprint (stamina 100 to 90), RB's scanner under the sprint, the sector map (LB opens it, half a throw of the right stick moves the cursor 758 m of the
  8 km frame in two thirds of a second, RT and LT zoom, A and START place a mark, the menu removes it, X brings the cursor home), the
  commands' buggy, X into it and RT driving it (6-8 m/s after two seconds).
  The overlays as drawn: `shots/tests/pad_*.png` (the commands in the cabin, their general and keys pages, on foot and in the buggy, over the landing map, the sector map, the star
  map and the galaxy chart;
  the keyboards of the aim, a text entry and the console; the controller page; the help).
* `unit` 332 checks, 0 failures; `regress` 24 of 24 unchanged (no frame without a controller moved).
* Not tried on a real controller here: the DualSense, the Xbox pad and the AYN Thor are the user's and their testers' (KI-412).
