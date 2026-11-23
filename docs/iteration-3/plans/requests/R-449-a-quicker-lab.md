# R-449: a quicker lab

status: done 2026-11-22 (1.5.0, before its release)
milestone: 1.5.0

## Request

The user, 2026-11-22, in a list of play notes: "Speed up modified organism creation", "Faster population growth" and "Select of culture
on drone doesn't work. Also remove charging of drones". Then: "Make genes reading in sequencer and genes modification running for 10
secods. For seeding with drone no cooldown at all (but current time for waiting for drone to plant and watch is fine)." On the plan
(faster growth in a flask and on a world, the drones never charging): "Proceed", and "make it so that flask stays full indefinitely, I
don't like that you have a window for culture to be seeded - once it's grown you should be able to seed it (and yes agree - make growing
itself way faster, that's too long)".

## Done

- **The sequencer** reads a genome in 10 seconds (it took 90), and **the gene editor** writes a strain in 10 seconds (it took two
  minutes; its status says TEN SECONDS).
- **The drones never charge.** They can be sent again the moment they dock, after a search or a seed trip. The charge's code is gone:
  its five minutes, the CHARGING header and refusal, the countdown on the flight computer's devices page, the amber lamps and the
  breathing cells while charging, and the save's `bayback` line (an older save's is dropped). A seed trip's fall, watch and climb are as
  before. A drone back aboard while the others are still away shows an amber lamp, as before.
- **A flask grows five times as fast.** Every time in a flask (its lag, its rise to its crowd, its spores' waking) is a fifth of what it
  was (`FLASK_QUICK`). At its best a culture is full in 2 to 4.5 minutes (it took 8 to 20); a grazer alone in broth, which needs prey, in
  about 12. Every rate keeps its ratio to the others, so which culture wins in a flask, and every seeding's verdict and pace, are as
  before.
- **A full flask stays full.** The incubator feeds it: once at its crowd a culture stays FULL until its medium is changed, it is
  emptied, or another culture in the flask beats it. With nothing it can eat it still starves, holds a few minutes and then rests or
  dies, so the spores, cysts and fruiting bodies can still be seen.
- **A seeded world changes four times as fast.** A step of a line's spread is 75 seconds (it was five minutes): a line covers its world
  in about half an hour (it took two hours), and everything that goes by the steps follows (the air, the waters and ground, the
  overgrowth, the ruins taken, the animals' fates, a native dying out).
- **Verified:** the unit's checks with their new numbers (see the log entry R-449).
- **Left:** a seeded world in a save made before this jumps ahead to the step its clock now gives.
