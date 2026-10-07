# Iteration 1.2.0: time and witness

The subset of `MILESTONES.md` chosen for 1.2.0 on 2026-10-06. The C series gave the explorer
reasons to go *somewhere* (signal, ruin, shard, chart, the next world). This iteration gives
reasons to be somewhere *at a time*: events you can see coming, plan for, travel to and
witness, and the one irreversible act in the game, the probe into a giant.

Left out on purpose: the V series (parked until the rendering direction is settled; nothing
here depends on it but the ground look of W-03), W-02, W-05, C-11, S-07, the P, L and K
series. They stay in `MILESTONES.md` as they are.

## Order

| # | Id | Item | Size | Status |
| --- | --- | --- | --- | --- |
| 1 | W-01 | The almanac: the time-lapse as a menu of upcoming events, run-to-it | M | done 2026-10-06 |
| 2 | W-06 | The telescope on the ship: zoom, reticle, drift, photo mode | M | done 2026-10-06; reworked the same day on review (the plate, the set, R-404), then R-405 (the stabiliser, the orbit keys) |
| 3 | W-03 | Weather that arrives: fronts with position, heading and speed | L | done 2026-10-06 |
| 4 | W-04 | Nights: meteor showers, zodiacal light, ring and moon shadows, earthshine | M | done 2026-10-07 (with B-408 first) |
| 5 | X-01 | The probe: launch and relay from orbit, the cabin screen, aiming, the parachute, the end | M | done 2026-10-07 (unlimited probes, the user's answer) |
| 6 | X-02 | The descent's stages: above the clouds, haze, the ammonia deck, between decks, the water deck | XL | done 2026-10-07 (with R-406: Shift runs the probe's clock x8) |
| 7 | X-03 | Per-giant character: decks, storms, lightning, glow, rare features; brown dwarfs, ice giants | M | done 2026-10-07 |
| 8 | X-04 | What comes back: the recording, the final image, the depth profile, log, statistics, export | M | done 2026-10-07 (with B-410 between) |

Each row's `Status` moves to `done` (with the date) when its milestone is finished, and the
matching row of `MILESTONES.md` gets `1.2.0`.

After the eight, on the user's play of X-04 (2026-10-07): R-407, a jump of any length along the
aim (with the aim's markers: no row of UNKNOWN, VISITED), and R-408, the explorer's own marks on
the sector map and a surface scanner in place of O6-06's landmarks; both done 2026-10-07
(`docs/iteration-3/plans/requests/`). Released as 1.2.0 on 2026-10-07.

## The milestones

Sources: `IDEAS-world-and-sky.md` for the W items, `IDEAS-gas-giant-probe.md` for the X items.
Read the source section before starting; it holds the intent and the register.

### W-01: the almanac

`T`'s warp steps and `Ctrl+T`'s 25-second time-lapse at x600 (M5-06, `Game` in `game.cpp`)
cannot catch a moment. `Ctrl+T` opens a small menu instead (also reached from the data sheet):
the next events at this place with countdowns, and "run to it", where the warp slows over the
last seconds so the moment is seen and not skipped. Under the real-time clock setting the menu
still lists the events but cannot run to them.

* Events on the ground: sunrise, sunset, noon, the end of twilight; moonrise and moonset per
  moon, the parent's rise on a moon, full and new phases; the next eclipse (moon over sun,
  parent over sun, ring shadow); "until dark", "until light", and a free "in N min/h/d".
* The same menu on the ship for orbital events: a transit, a conjunction, a comet's periapsis.
* Built as an event registry other milestones add to: W-03 adds "front arrives", W-04 adds
  "meteor shower peak". Every event is computed from the geometry, so the list is exact.
* Reuse what exists: the eclipse code in `surface/site.*` and `surface_view.*`, the warp in
  `game/game.*`, the moons' and the parent's positions the sky already uses.
* Done when: the menu lists correct events on a moon of a ringed giant, a locked world and a
  plain world (a `unit` check per event kind: the time it gives is the time the geometry
  reaches it); run-to-it stops within a second of game time; the flow uses it once; one new
  scene or harness mode prints a place's almanac.

### W-06: the telescope

A magnified view from orbit or deep space: a planet from across the system, a moon's craters,
a companion star's disc, a cluster resolved, a comet's tail. The space renderer draws at any
scale, so this is a field of view, a reticle, a slow drift and the screenshot path.

* A key in space (and the cabin screen if it fits the cabin's layout): zoom steps, steering at a
  rate that falls with the zoom, the target's name (or UNKNOWN) and range on the reticle.
* Photo mode: the frame saved like F12's; naming the target through the guide as elsewhere.
* Pairs with W-03 (a front seen from orbit) and W-04 (a shower's radiant), and with X-01's
  aiming (the telescope is how you choose where the probe goes).
* Done when: `space` renders telescope frames of a moon, a ringed giant, a companion star and a
  comet; the HUD stays within the 52-character line; `bench check` holds at the deepest zoom.

### W-03: weather that arrives

Fronts as moving features: a position, a heading and a speed per front, hashed from the body
and the time so they are deterministic like everything else. Seen from orbit as cloud, on the
landing map and the sector map as a band, on the ground as a wall of rain or dust coming over
the ridge, the wind rising ahead of it, the light dropping, the temperature falling behind it.

* The wind, rain, dust and lightning models exist; a front drives them by distance to the
  front line instead of (or on top of) the local hash.
* The data sheet forecasts ("RAIN IN 40 MIN FROM THE W"); the almanac lists "front arrives".
* The globe's cloud from orbit must agree with the fronts on the ground (the consistency
  principle): the same function draws both.
* Generation: if fronts come from a new hash stream, nothing old moves and no bump is needed;
  if the existing weather draws change, bump `GEN_VERSION` and record it.
* Done when: a scene shows a front approaching over open ground at three times (far, arriving,
  through); `unit` checks the forecast's arrival time against the front reaching the site; the
  landing map shows the band; orbit and ground agree on a pinned world.

### W-04: nights

The aurorae are done (B-403, R-402). The rest of the nights, all derived from the system:

* Meteor showers when a world crosses a comet's orbit: streaks with a radiant fixed in the
  sky, a few a minute, more at the peak; the almanac lists the peak.
* Zodiacal light in young systems and near belts: a faint cone along the ecliptic after sunset.
* The shadow of a ring or a moon crossing the ground at night (the landing map already carries
  shadows: `buildShadowMap`), earthshine on a moon's dark side seen from its parent, the
  parent's glow lighting a moon's night.
* Measure dark frames by printed RGB or bank/shade, not by eye (the viewer stretches them).
* Done when: a scene per effect (shower at the peak, zodiacal cone, earthshine); `unit` checks
  the shower dates against the comet's orbit crossing; `stability` on the shower scene; regress
  unchanged unless a pinned frame's night legitimately changes.

### X-01: the probe, launch to end

The flow, before the hard rendering: from orbit around a gas giant, aim (with the telescope),
launch, and watch on the cabin's screen while the ship stays where it is. The relay's sound,
the readouts (depth, pressure, temperature, wind), a parachute stage that spends longer in a
layer, photographs, and the end: the last frame held under a loss-of-signal hiss.

* Reuse the capsule flow, the cabin screens and the frame recorder (`movies/`).
* The descent can render placeholder stages here; X-02 makes them real.
* How many probes the ship carries (unlimited, a few, restocked) is a question for the user at
  the start of X-01.
* Done when: the flow launches a probe into a giant, falls to the end and returns control; the
  save survives a launch mid-descent (or the descent is not saveable, by decision).

### X-02: the descent's stages

The real work: above the clouds (the ring from inside its plane, moons over the deck, the
terminator), the upper haze (amber light, cirrus in the jet streams whose speeds and colours
are the globe's bands), the ammonia deck as terrain with a cloud material, the clear air
between decks (a floor below, a ceiling above, lightning inside it), the water deck and the
dark. Being *inside* a deck with light coming through it is new to the renderer.

* Bands, colours and speeds come from the function that paints the globe.
* Done when: a harness mode renders each stage on two giants; `bench check` gains a descent
  frame within budget; `stability` on the deck stage shows no flicker.

### X-03: per-giant character

A few seeded numbers per giant: decks and the first's depth, storm frequency, lightning rate,
the deep glow (young giants), polar aurorae, and a rare feature or two (a storm the size of a
moon, a clear-air hole to the water deck, diamond hail on carbon-rich giants). Brown dwarfs
glow from below; ice giants get their own palette and a quieter fall. No life in the clouds
(the source's "What to resist"): at most one rare, ambiguous sight, never explained.

* Done when: a harness mode prints the character of the first twenty giants of the scan, and
  renders a hot young giant, an ice giant and a brown dwarf at the same stage.

### X-04: what comes back

The probe is lost; its record is kept: the recording in the guide's gallery (giant's name or
UNKNOWN, date, depth), scrubbable by depth on the cabin screen with the readouts; the final
image on its own; the depth profile (pressure, temperature, wind, lightning, light) on the
giant's data sheet, layers named by what was found; log events with depths; statistics
("PROBES SENT", "DEEPEST DESCENT"); a named storm on the guide's list; the recording sharable
through the guide export like C-14's lending.

* Done when: `unit` round-trips a profile and a recording entry through save and export; the
  flow sends a probe and finds it in the gallery and the statistics; two probes into one giant
  at different places give different profiles.

## Working rules for this iteration

* One milestone at a time, in the order above, and stop after each for the user's review and
  "proceed". The user may interleave bug reports and requests; handle those as B-4NN/R-4NN.
* Start each milestone by reading its source section, `docs/reference/10-decisions.md` and the
  reference notes it touches; if the design has an open question the user should answer (the
  probe count, a key binding that collides, a register question), ask before building.
* Per milestone: `make -j8`, `unit`, `flow`, `regress` (bless only for a deliberate change of
  what is drawn or generated), `bench check`, plus the milestone's own checks above; look at
  the frames in `shots/tests/`.
* Docs per milestone: a dated bullet in `docs/iteration-3/plans/PROGRESS.md`, the reference
  notes it changes (02 rendering, 06 surface, 07 controls, 08 testing, 09 cookbook as relevant),
  `10-decisions.md` for decisions and any `GEN_VERSION` bump, `KNOWN-ISSUES.md` for what stays,
  the status here and in `MILESTONES.md`.
* Nothing is committed unless the user asks; commits carry a randomised author.
