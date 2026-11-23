# Rules: the ship: its plan, its rooms, the light, the walk, the sight, life aboard

Rules for the Stardrifter's interior, one line per rule with its log entries (`log/`); a replaced rule moves to `superseded.md`.
Devices: `rules-game-devices.md`; a device's row: `rules-game-screens.md`.

## The plan
- The ship is its plan's rows (`game/ship_plan`): rooms, ways, windows, fixtures, lamps, views; a device is a `cabinDevices` row in a room [I-01], [I-02]
- Three decks and eight spaces round the hall, the stair a room of its own; `ShipRoomId` is saved: a new room goes before `SHIP_ROOMS` [I-02]
- The mesh is built once from the rows, in ship metres (a piece: room, material, bank, device, shade factors); the frame adds light and screens [I-01]
- Every surface has a finish (hull grey, panels, a dark floor, fabric, brass, plants, glass, the outside); each but the hull's is dealt a bank [I-03]
- A piece faces where it is seen from (normal toward its room's middle, a box's outside); one turned away from the eye is left out [I-03]
- The constant rows are checked whole at compile time (`planWhole`), fixtures and colliders by the unit's `ship` group [I-01], [I-02]
- Walls between rooms are slabs (no two rooms' faces share a plane); a curve's points are shared by its pieces (the well's sixteen) [I-02]

## The frame
- The sight is taken before the space view (`cabinSight`): the eye's room, then rooms seen through its ways, each with its box on screen [I-02]
- A seen room's pieces are projected once a frame, in order (`ShapeList`), left out off its box and filled only in its rows [I-01], [I-02]
- A window's glass is written to depth only (no room behind it drawn through it); lamp glows after the bands [I-01], [I-02]
- No window, oculus or dome in view and the hull polarised: no space view (the frame cleared, bodies' places kept) [I-02]
- Skipping and clipping leave the same pixels (`ship rooms` draws each unclipped: none differ) [I-02]
- Banks are dealt once the outside is drawn (`cabinDeal`): nearest asks first, to their homes, else free banks; the rest dim or grey [I-03], [B-435]
- A device behind the eye asks no bank; each device has its own home (a home asked twice keeps one colour: 30 is the drones') [Z-06]
- The pool: every bank but 0, 8, 10 and, looking out, 7 (the outside in the glass's box, in its own palette); no bank holds two hues [I-03], [B-437]
- Aboard, a target is marked only where the glass shows its point (the outside's bank there, the cabin drawn) [B-501]

## The light
- A room's lamps light its vertices once with the mesh (surfaces and boxes cut to a 1 m grid give pools); the frame scales them by U's level [I-03]
- A lamp's glow on a surface is its pool, never `glowDisc` (it lands only on empty pixels); the lamp is a point in the light's bank [I-03]
- The sun's patch is each glass's outline cast along the light onto surface pieces facing it (round arches, windows, wells), shadows cut out [I-03], [B-434]
- A patch adds shades in the surface's own bank on its plane only (`BLEND_ADD`, a hair nearer, unwritten); none sunless, in flight or eclipsed [I-03]
- The parked world's light (`bodyShineAt`, by its phase) falls on faces turned to it in rooms whose glass sees it; depolarised, in every room [I-03]
- The hull outside (roofs, the prow, the drive's ring, the mast, the drones' door) is drawn first within the glass in view, lit by the sun [I-03]
- Y depolarises every outer wall: the skin is glass (not drawn), what stands outside stays, the sun on every face toward it, no patch [B-411], [I-03]

## The walk and the places
- The walk keeps to a room's floors and its arches' tunnels; crossing an arch's middle changes the room, then its colliders apply in order [U-03], [I-02]
- The colliders are the fixtures' footprints (a box kept to the side it was met from), the cradle's circle, the stair holes' rails [I-02]
- The stair is walked by the angle round it (`stairA`): flat on its landings, a ramp between, entered and left only through a landing's way [I-02]
- Floor height changes continuously by place (the bridge's steps, the stair); Shift runs at 3.2 m/s, walking is 1.6 [I-02]
- The explorer faces only the devices of the room stood in, at the spot most ahead within reach [U-03], [I-01]
- The lift: Page Up from any room to the observatory, Page Down to the bridge (the flight computer's row too) [I-02]
- No place is held outside the plan: auto-explore's windows and routes, the harness's views and the devices' stands are its rows [I-01], [I-02]
- Auto-explore walks the plan's routes a way at a time (`shipNextWay`), first clearing a rail (outward when right beside it) [I-02], [I-04]
- The save keeps the `cabin` line and adds `cabin_place` (the room, the stair's angle); one without it starts at the bridge [I-01], [I-02]

## Life aboard
- The explorer's things are pieces the frame fills in their room's order (`cabin_life.cpp`), each its fixture's, lit like the vertices [I-04]
- The wall prints the newest twelve photographs in one tone, or the explorer's own once one is hung; on a thread, never awaited [I-04], [R-428]
- The garden is the home world's flora, a bed grown by the time played since E trimmed it (full in an hour), swaying in the draught [I-04], [R-429]
- U's night keeps the glows' share (`SHIP_NIGHT_GLOW`); the drive's level in a flight lights the core and the ring outside [I-04], [R-427]
- A descent boarded at the capsule opens on the hatch's drop, seen from the hangar; C and the landing screen go straight down [I-04]
- Aboard, one hum and the drive in a flight, alike in every room (1.4.0's): no room has its own sound [R-430]
- The maintenance room's buggy and drone use the ground's shapes (`vehicle_shape.h`) in the bay drones' primitives [R-427]

[B-411]: log/3-1.3.0.md
[U-03]: log/6-1.4.0.md
[I-01]: log/7-1.5.0.md
[I-02]: log/7-1.5.0.md
[I-03]: log/7-1.5.0.md
[I-04]: log/7-1.5.0.md
[Z-06]: log/7-1.5.0.md
[B-434]: log/7-1.5.0.md
[B-435]: log/7-1.5.0.md
[B-437]: log/7-1.5.0.md
[B-501]: log/7-1.5.0.md
[R-427]: log/7-1.5.0.md
[R-428]: log/7-1.5.0.md
[R-429]: log/7-1.5.0.md
[R-430]: log/7-1.5.0.md
