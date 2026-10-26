# R-421: thirty to fifty new kinds of world, scientific enough

status: done 2026-10-23 (block P of `docs/ideas/ITERATION-1.4.0.md`: P-00 to P-07, and the dark ocean in D-04); P-00 done 2026-10-21, P-01 (the
six furnace worlds, generation 16) done 2026-10-21, P-02 (the six living worlds) done 2026-10-21, P-03 (the locked and the deep) done
2026-10-21, P-04 (the dead worlds and their peoples) done 2026-10-21, P-05 (the far cold) done 2026-10-22, P-06 (the strange moons) done 2026-10-22,
P-07 (the giants) done 2026-10-22: 58 kinds of body, 50 landable (the dark ocean, D-04's, the 59th)
milestone: 1.4.0

## Request

"Fine, now let's prepare for release v1.4.0 (prepare a branch). This one will be focused on improved planets generation
(there should be 30-50 new planet types, not too ridiculous so it's still scientific enough), each planet should have way
more unique modifiers (again they should make sense) - 20 to 30 more unique modifiers in addition to what we have now, also
greater variery of modifiers in general. One interesting request/idea for planets - consider rogue planets that lost their
system and just fly in space. They shoulbe be able to be any kind of available planets and the only way to find them is by
radio signal (they should be very unqiues, comsic horror like sounds). If rogue planet is civilized desert (it shouldn't be
felisian since it would be cold already but you can add dead felisian world also among these 30-50 new types) then
civilization shards should tell how they planet lost the system, sun disappeared and they died (should be unique for rogue
planets). For everything here as milestones for 1.4.0, investigate, propose. I will tell you when to proceed"

The kinds are R-421, the modifiers R-422 (the traits), the rogue planets R-423.

What exists: twenty-one kinds of body (`PlanetType`), nineteen landable, each with its own branch of the planet function, its
palette, sky, weights by zone and star class, sentence and (six of them, R-307) an event on the ground.

## Plan

`docs/ideas/IDEAS-worlds-and-wanderers.md` section 1 holds the thirty-eight kinds (what each is, its source in the literature,
where it forms, its ground, its sky, its event); the milestones are in the iteration file:

* **P-00, the kind table:** every kind a row, the planet function's shared skeleton, accent hues for worlds without flora,
  traits by tags; nothing generated or drawn changes.
* **P-01 to P-07, the families:** the furnace worlds (magma ocean, chthonian, glass, steam, salt, rust; generation 16, the
  new kinds drawn from the old draws by hashes of their own), the living worlds (archean, hothouse, superhabitable, steppe,
  tundra, lichen), the locked and the deep (eyeball, terminator, hycean, ammonia, snowball, chemotroph), the dead worlds and
  their peoples (frozen felisian, scorched felisian: the user's dead felisian world), the far cold (nitrogen, cryovolcanic,
  dry ice, wanderer, brine dwarf, frost dwarf), the strange moons (grooved ice, two-faced, tiger-striped, patchwork, sponge),
  the giants (water-cloud, azure, alkali, silicate, gas dwarf, super-puff).
* **D-04:** the drifters' own kind, the dark ocean.
