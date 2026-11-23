# R-427: the maintenance room

status: done 2026-11-18 (1.5.0, before its release; one of five requests aboard, R-427 to R-431)
milestone: 1.5.0

## Request

The user, 2026-11-18, after walking the new ship: "Buggy and drone on lower level instead of weird portal", then: "Regarding buggy and
drone - there is a portal on the lowest part of the ship where there is a portal. I suggest to remove it and put buggy and drone models
on some maintenace platforms like it's maintenance room. Purely for display, would be cool."

The portal was the Vimana drive's ring standing upright on its plinth in the engine room (I-02, lit by I-04's drive and its fuel orb),
on the lower deck astern of the hangar.

## Done

- The engine room is the maintenance room (`ROOM_MAINTENANCE`, its number and place the engine room's, so a save stood in it loads
  there): the ring, its plinth and the fuel orb are gone (the orb's light with them, `LAMP_ORB`); the drive's ring outside the hull
  and the core under the hall's floor glass keep the flight's glow.
- The buggy and the drone the capsule carries stand on two platforms under a work lamp each, the buggy turned to the walk between
  them, the drone's nose to the arch; a workbench against the stern wall. For display: E does nothing there.
- Their models are the ground's shapes: the stations, wheels, pods and skids moved to `surface/vehicle_shape.h`, which buggy.cpp and
  drone.cpp read as before and `game/vehicles_draw.cpp` assembles as they do (at rest, unfolded, the lamps out) in the bay drones'
  primitives, drawn and lit as the rack's drones are, in their paint.
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-427; `ship life`'s `maintenance` and `maintenance_platforms`.
