# Idea: the gas giant probe

Kept from a discussion on 2026-09-30. Not scheduled or designed. Gas giants (and brown
dwarfs, and ice giants if they come) are not landable; a probe is the way to explore them,
and it fits the game's grammar: you do not conquer the place, you look at it, and then
the looking ends.

## The shape of it

A probe launched from orbit the way the capsule is, but you never leave the ship: the
cabin's screen shows the probe's camera and readouts, and the sound is the relay. The
descent is a one-way trip. It ends when the pressure crushes the probe; the last frame
stays on the screen with a loss-of-signal hiss. That ending is the feature: every giant
becomes a place you saw once, for twenty minutes, and cannot see again the same way.

## What there is to see, layer by layer

The descent is naturally staged, and each stage is a scene the renderer can draw in
some form:

1. **Above the clouds.** The ring arc from inside the ring plane, moons rising over the
   cloud deck, the planet's own shadow, the terminator as a wall of dark. The sky bodies
   of the space renderer.
2. **The upper haze.** The light going amber, the sun a soft disc, cirrus streaks racing
   past in the jet streams. The wind bands seen from orbit are the ones you fall through:
   band speeds and colours come from the function that painted the globe (the consistency
   principle again).
3. **The first cloud deck (ammonia).** A landscape: cloud tops drawn as terrain with a
   soft material, canyons between bands, thunderheads kilometres high, the great storm as
   a wall if you aimed at it. The terrain renderer with a cloud material instead of rock.
4. **Between decks.** Clear air, a floor of cloud below and a ceiling above, lightning
   flashing inside the ceiling, the light dim and red. The sound goes from wind to a low
   roar.
5. **The water deck and below.** Rain that never lands, darkness, the probe's own light
   on cloud, the readouts climbing, the hull creaking. Then the end.

## What the player does

Deliberately little: aim the probe before launch (a storm, a band boundary, the pole with
its aurorae, the night side for lightning); steer the camera during the fall; open a
parachute stage to spend longer in a layer; take photographs. Naming applies: you name the
storm you fell into, and it is on the guide's list with the depth reached. The data sheet
fills in as you go (pressure, temperature, wind, composition per layer), so the giant's
sheet after a probe is richer than before.

## What comes back out

The probe is lost; its record is not. Everything it relayed is kept on the ship:

* **The recording.** The whole descent as a video the ship stores (the frame recorder
  exists: `movies/`), replayable from the cabin's screen with the readouts overlaid, and
  scrubbable by depth. A recording is an item in the guide's gallery next to the
  screenshots, with the giant's name (or UNKNOWN), the date and the depth reached. The
  last frame is kept on its own as the probe's "final image".
* **The profile.** A depth chart of pressure, temperature, wind speed and direction,
  lightning rate and light level, drawn on the data sheet; the layers named by what the
  probe found (haze, ammonia deck, clear band, water deck). Two probes into the same giant
  at different places show different profiles, which is a reason to send a second.
* **Discoveries.** Anything named during the fall (the storm, a feature) and anything the
  probe crossed (a clear-air hole, a diamond hail on carbon-rich giants, an aurora) goes
  to the log as an event with the depth, and to the statistics ("probes sent", "deepest
  descent").
* **Sound.** The relay's audio is part of the recording: the wind, the thunder, the
  creaking, the hiss at the end. A shard of its own kind, sharable through the guide
  export like names, so a friend can watch your descent.

## Per-giant character

The generator gives each giant its bands and colours already. A descent adds a few
seeded numbers: how many decks and how deep the first is, storm frequency, lightning
rate, a hot deep glow (young giants) or a faint one, aurorae at the poles, and a rare
feature or two: a storm the size of a moon, a clear-air hole down to the water deck,
diamond hail. Brown dwarfs get their own glow from below; ice giants a different palette
and a quieter fall.

## What to resist

Life in the clouds. It is the obvious idea and it cheapens the descent; the register is
"indifferent and beautiful". At most one rare giant with something ambiguous seen once in
the deep and never explained.

## Cost

The cloud-deck rendering is the real work: cloud tops as terrain is a known trick here,
but being inside a deck with light coming through it is new. The rest reuses the capsule
flow, the sky renderer, the frame recorder, the data sheet and the gallery. A milestone
comparable to the drainage work, paying off on every gas giant in a galaxy with many.
