# R-448: seeded worlds

status: planned 2026-11-20 (1.5.0: the J series, J-01 to J-12); J-01 and J-02 done 2026-11-20, J-03 to J-09 2026-11-21, J-10 to J-12 2026-11-22; done
milestone: J-01 to J-12

## Request

The user, 2026-11-20, after R-447: "Ok, next one will be huge but we are including it in 1.5.0 nonetheless. At this point investigate
and prepare milestones since it can't be implemented in one run. So the description of set if features:

You can send cultivated (genetically modified with feature you implemented recently) microbs/cultures to certain planets and if they
survive they can change planet in a different unpredictable way, sometimes beneficial (making it livable, adding atmosphere, affong
vegetation), sometimes neutral, sometimes harmful (needs more planet traits, maybe new classes of planets that will be a byproduct)

For this one there should be more genes that you can tweak with BUT don't make it ridiculous "growing extensive mushrooms" genes or
"making thick atmosphere" gene - it should be more nuanced. For instance, microb with tweaked gene can produce more co2 which will
transform planet into venusian type of planet. Another gene may cause water turn red, another make luminescent clouds, another add smoke
on the water, etc. Sometimes microb can change one aspect of a planet, sometimes several. Come up with new original and tons of
traits/aspects

There should be more traits for planets, in a kind of way we added in 1.4.0 but now they should be of artificial type (like flesh
covered plabet, more on that later). Sometimes It's traits you wouldn't encountering the wild

About harmful aspects - sometimes you may kill or mutate life in different ways (I want you to make it bio horror like) - for instance,
animals may die out and you will see their remains (I want you to model them after animals that are actually living on the planet). As
for mutations they may become disgusting creatures with ribs being seen, flesh melting, etc - I want you to make it feel like you
actions don't necessarily make a world you touched a better place- sometimes it's statistically up horror. Other ideas for harmful
transformation is upt to you and they are welcome, some of them can be a all the vegetation dying out

As for flesh covered planet - maybe some of the organisms you send grow tissue indefinitely after some gene modification which makes the
whole planet covered in flesh. Thos one is also quite big because it shouldn't be mere texture change - ruins may also be covered in
flesh after that, aome new landscape structures may appear and so on

There should be a variety even if traits is technically the same - when it's animal mutation it can be any kind of mutation or at least
preselected from big list of possible mutations (and not only biohorror!). If it's flesh growing it can be a flesh like tissue of
different types. It can be red flesh, eegulat flesh, etc. Or not fesh, maybe mushrooms! Or It's vegetation of different colors. It can
be anything!

Finally it's time to extend animals variation drastically. And still procedural"

## Plan

The design is `docs/ideas/IDEAS-seeded-worlds.md`; the milestones and their checks are `docs/ideas/ITERATION-1.5.0.md`, Phase 6. It
reverses 1.5.0's "the worlds stay as generated" for the worlds the explorer seeds; a world nobody seeds stays as generated.

- **J-01, world genes:** fourteen new genes on every species (gas, dye, lift, frost, slime, shell, stop, bind, rot, poison, carrier,
  host, dry, shield), seen in the lab first.
- **J-02, the world now:** a seeded world's layer over the world as generated, proven by changing no picture.
- **J-03, sending:** the drone bay's seed trip, whether the culture takes, the spread over the guide's clock, the record; the waters'
  colours as the first change.
- **J-04 and J-05:** air and sky; water, ground and plants.
- **J-06 to J-08, the animals:** bodies of parts in sixteen plans (generation 18, plates judged first), their ways and the explorer's
  record, then their fates: die-offs with remains of the world's own animals, sickness, changed young.
- **J-09 and J-10, the overgrowth:** flesh, fungus and eight more types over the ground with structures (plates judged first), then the
  ruins overgrown and the sounds.
- **J-11 and J-12:** made kinds (a hothouse, a snowball, a flesh world and the rest), then the record and the close.

## Done

- **J-01, world genes (2026-11-20):** the fourteen genes at places 19 to 32 on every species, read from what each species is; what they
  do in a flask (an overgrowth, ice, the UV light, a poison, genes passed) and how the lab shows them (bubbles, dyed water, a scum, slime,
  a sediment, clumps, tissue); learned against the parent; the editor's two pages (Page Down and Page Up). Log: `log/7-1.5.0.md`.
- **J-02, the world now (2026-11-20):** the layer a seeded world is drawn through (`galaxy/made.h`, `life/seeding`), carried in the
  world's generator and read last by the planet function, the globe, the telescope's plate, the maps and the ground; the guide's
  `seeded` and `clock` lines; `seed test` shows a test tint and cover in every view and the views as before once it is off. Nothing
  drawn changes on a world nobody seeded. Log: `log/7-1.5.0.md`.
- **J-03, sending a culture down (2026-11-21):** S on the drone bay: a flask that shows FULL, a place on the world's map, drone 1 sent;
  the culture let go as it arrives, its report the verdict (the furnace, the liquid, the medium as the incubator judges it, oxygen, pH,
  the dose, the land, the natives), its drift in the wild; the spread through the waters, land and warmth its line lives in, at its pace;
  the waters' colours from DYE and GLOWING SEAS on the ground, the maps and the globe; the line in vials where it spread, the data sheet's
  SEEDED and MADE lines, the LANDING log's change. Log: `log/7-1.5.0.md`.
