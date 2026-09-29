# Reference notes

Read in order the first time; afterwards jump to the topic you need.

1. `01-architecture.md` — modules, dependency rules, per-frame data flow.
2. `02-rendering.md` — framebuffer, palette banks, mush filter, rasteriser, per-scene bank assignments, draw order.
3. `03-coordinates-and-units.md` — axes, camera, body frames, local frames, sun position, units, constants.
4. `04-galaxy-and-systems.md` — star field, star classes, system generation, planet type selection, names.
5. `05-planet-function.md` — the shared planet function, LOD gating, feature fields, materials, planet maps.
6. `06-surface.md` — landing site, terrain caches, player physics, environment model, lighting, objects, life, weather.
7. `07-game-flow-and-controls.md` — states, ship autopilot, landing map, HUD, save format, audio state.
8. `08-testing-and-tools.md` — headless harness, screenshots, benchmarks, how to add tests.
9. `09-cookbook.md` — step-by-step recipes for common changes.
10. `10-decisions.md` — decision log with rationale.
11. `11-original-noctis-notes.md` — what the original did (from its source and manuals), what was replicated.

Keep these in sync with the code: when a constant or a rule changes, update the note in the same change.
