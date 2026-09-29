# R-401: nothing has a name until you give one

status: done 2026-09-29
milestone: release

## Request

Every object you approach shows a generated name; in the original everything was unknown until the explorer named it. Only the name: every other attribute (type, size, orbit, climate) stays known.

## Done

Stars, worlds (planets, moons, comets, companions, brown dwarfs), belts and landmarks display `UNKNOWN` (landmarks `UNNAMED <KIND>`) until a name is given through the guide (`NAME THIS STAR`, `NAME THIS WORLD`, `NAME THE NEAREST LANDMARK`; `RENAME ...` once named) or comes from a friend's inbox file. The generated names still exist inside the generator (keys, landmark ids, the harness's scene finders use them) and are never shown. The text entry starts empty for an unnamed object. The save slot's `name` line is the display name.
