# Civilisations: the peoples and their ages

One of the five notes on the dead peoples (`peoples-and-ages.md`, `built-world.md`, `records-and-grammar.md`, `sound.md`,
`radar.md`), split by topic from `12-civilisations.md` in E-04; its sections are as they were, in their order.

The C series of 1.1.0 (`docs/archive/ideas/IDEAS-ruins-shards-signals.md`, `docs/ideas/MILESTONES.md`). A people lived on one
felisian or desert world in eight and is gone; what it left is settlements of ruins (C-01, `06-surface.md` and
`05-planet-function.md` for the trait and a dead desert's old seas), from C-02 on its words, from C-04 its music and from C-05
its voice. One such world in five had two peoples (C-13), on either side of a great circle, each with a culture, a lore, a
tongue, a voice, a music and fifty recordings of its own. 1.3.0's expansion (`docs/archive/ideas/ITERATION-1.3.0.md`) gives every
people the age it had reached when it collapsed and a profile the later milestones read (A-01, generation 13: "The ages",
below).

<!-- contents -->
Contents:
- The trait (C-01)
- The settlements (C-01)
- Graves and names, the calendar and the last recording (C-12)
- Two peoples on a world (C-13)
- The ages (A-01)
- The lights left on (D-04, for D-05's peoples)
- The people who lost their sun (D-05)
- The history (H-02)
- One age a world (B-414)
- Two peoples of one age (R-413)
- What the later milestones take from here
<!-- /contents -->

## The trait (C-01)

`TR_CIVILISATION` is never drawn by the trait draw: `BodyGen::make` gives it to a felisian or desert world when
`mix64(seed ^ 0xC1D1)` falls under 0.125, first among its traits (after the glass), so no other world's traits move.
A trait desert keeps the level its seas stood at (`BodyGen::oldSeaM`, -120..100 m) and a river density of 0.8 at
least: dry seabeds with salt in the deeps, old channels, a shore band. Every other desert is a desert and nothing more.
`worldHadCivilisation(g)` (`peoples/ruins.h`) is the one question the ruins, the landmarks, the harness and the shards ask. Since R-408 (2026-10-07) the surface scanner's RUINS mode hears every settlement within 40 km (`ruinsNear`; B-417: M4-07's monoliths of the old ones, on every felisian, quartz and ocean world a people there or not, are gone, and a world without a people offers no RUINS mode) and points to the nearest one the explorer has not marked; the explorer marks and names what they find (`07-game-flow-and-controls.md`).

## The settlements (C-01)

`peoples/ruins.*`: hamlets, villages, towns and lone monuments (a desert's; B-417: every people's world's) on the 2 km ruin grid, in a `Culture` the whole
world shares (`cultureOf`; C-13: the whole people, a settlement's being its people's, `RuinSpec::people`), each building drawn from its own seed with a doorway, a window, an inner wall, a roof where
the culture built stone ones, rubble at the fallen walls. `06-surface.md` has the layouts, the pieces, the levels of
detail and the colliders (B-405: each spans its own height); `08-testing-and-tools.md` the scenes, the `ruins` mode and
the unit's checks.

## Graves and names, the calendar and the last recording (C-12)

Three small things of the ideas note's third section, built together on 2026-10-03 because they share the people's own
count of years: a burial ground at every settlement whose stones carry names and years, a calendar the shards are dated
in, and one shard a world that is the last and closes its timeline on the decoder.

*The calendar* (`loreQuick`, `calendarOf` in `peoples/shards.cpp`). The day is the world's turn and the year its orbit
(`Lore::yearDays` their ratio, as C-02 read it); the month is the nearest moon's round in those days where a moon rounds
in three days at least and twice within the year (`monthDays`, `months`, the days over at the year's end kept apart). A
world whose day is a quarter of its year or more (a locked world: its day is its year) counts no days: moons alone when
it has one to count by, else years alone (`Lore::calendar` 0 days and years, 1 moons, days and years, 2 moons and years,
3 years alone). No unit of ours appears anywhere: the game's days are minutes of play, so an hour of theirs would be a
lie; the calendar is theirs and the shards are dated in it. A shard's `month` and `day` come from a hash of its seed, not
from its stream, so every text C-02 wrote is where it was; `shardDate` reads "year 212 of Aiyin, moon 69, day 5" and
`calendarLine` "year: 101 moons of 5 days and 1 over" (Aieliaalas II, whose one moon rounds in five of its days), "year:
93 moons of 4 days and 1 over" (Leileashphail III), "year: 25 moons   the day is the year" (a locked desert world near
home). The decoder shows the date on every shard's screen (a piece's too) and the calendar line under the world's list.

*The last recording* (`lastShardOf`). Of the text shards (the music and the charts left out) the one whose first draw,
the age, is the greatest is the world's last: `shardOf` sets its age to 1, so its year is the span's end and later than
every other shard's, and its tone the end; `pickTone` gives the end to no other shard (the late draws that were the end
before are warnings on a desert, elegies on a felisian world), so one shard a world says "I am the last". Finding it is
not signposted: it lies in a ruin like any shard, the taking says what every taking says, the list shows it among the
others until it is read. Read on the decoder (the speech's end or Enter), `markShardRead` writes a `SHARD` log line
("THE LAST RECORDING OF THE RAUZHIA READ: THE RECORD OF <world> ENDS IN YEAR 344 OF AIYIN"), the guide keeps the world
in `Guide::ended`, the shard's screen is headed THE LAST RECORDING, its row's year turns amber, and the timeline closes.

*The timeline* (`renderShards`, the world's list). Under the sixteen rows a rule runs from YEAR 1 across the screen: a
tick per shard held (read green, unread dim, the one under the cursor white; the last, once read, amber), the rule drawn
to the latest year held and dotted beyond it while the record is open, solid to an amber cap with THE RECORD ENDS, YEAR
N once the last recording is read. The span is not told before that: the dotted tail is all the explorer knows of how
much is left.

*The graves* (`peoples/graves.*`). A settlement's burial ground lies outside its edge (beyond the wall and every
building's reach, two metres on), on a heading away from the shore where it has one within 600 m, else the seed's: rows
of up to three to five stones 2.2 m apart, the rows 2.6 m apart, facing the settlement. A hamlet has one to three, a
village three to six, a town six to twelve, a lone monument one in three times; a stone whose place is inside a fallen
stretch of wall or a feature is skipped after its draws, so the rest stay where they were. Each stone is a slab 0.64 m
wide, 0.16 m thick and 0.7-1.2 m tall with a line of glyphs on its face, or the same lying flat (fallen with a chance of
0.6 times the culture's decay: nearly all on the twenty-two-millennia world, few on the young desert), drawn and collided
among the settlement's pieces (`graveElements` appends them to the cell's lod 0). Each carries a name in the people's
style (`personName`, nine letters at most, distinct within the settlement) and the years it covers in the people's count
(died within the span, a life of 30-90 of their years; born before the founding reads "to year 41 of Aiyin"). Within 2.2 m
of a stone on foot the HUD reads `A GRAVE: THAELU, 140-212` in amber, and the first time the guide records it like a
landmark (`Guide::graves`, a `grave` line; the status `A GRAVE: THAELU, YEARS 140 TO 212 OF AIYIN`, a `GRAVE` log line
naming the settlement's class and the world); the statistics count the graves read and the records ended on a dim line
under the shards. Nothing else happens, as the note asked.

*The harness.* `vesperis_test graves <sx> <sz> <body> [latDeg lonDeg] [cells]` prints the calendar, the span, the last
recording with its date and text, and the graves of the settlements round a point; the scenes `civilisation_graves` and
`civilisation_graves_desert` stand two metres before a burial ground; the flow reads Aieliaalas II's last recording on
the decoder (`shards_last`, `shards_timeline`); four unit checks (the last and the dates, the graves' placement, the
town's stones in the view, the explorer told once on the ground).

## Two peoples on a world (C-13)

"A world with ruins may have had one culture or, rarely, two, one on each continent, with their own languages and
traditions, whose shards speak of the other. Their catastrophe may be shared or not" (`docs/archive/ideas/IDEAS-ruins-shards-signals.md`,
section 4). None, one or two: a felisian or desert world without the trait has no people (seven in eight, unchanged), a
trait world has one, and one trait world in five has two (`peoplesOf`, `peoples/ruins.h`: `mix64(seed ^ 0xC13)` under 0.2; one
desert or felisian world in forty). The two test worlds are worlds of one; the third, Eleinewai I (sector 152 0 92, body 0,
a felisian world of a red dwarf), has the Ghotokhor (smooth adobe walls, paved roads, no roofs left) and the Khokurt
(striated rock, beaten tracks, stone roofs), who ended together 1005 years ago.

**The second people is a second seed.** `peopleGen(g, k)` is the world's generator with its seed mixed for the second people
(`mix64(seed ^ (0xC13A5 + k * golden))`) and the world's own for the first, so every draw of C-01..C-12 that read the body's
seed (the culture, the names, the span, the tongue, the voice, the tradition, the shuffle of the fifty, the charts' figures,
the last recording) is the people's without a line of those modules changing its draws; the first people's draws are the
world's as before, so no one-people world moved. The generator is never handed to the planet function: the terrain is the
world's. The convention: a `Lore` is a people (`Lore::which`, `peoples`, `seed`), the functions that take one (`shardOf`,
`tongueOf`, `voiceOf`, `traditionOf`, `chartOf`) read `loreSeed(g, L)` and never the generator's seed, so they take the
world's generator and give the lore's people; the seed-only functions (`shardIsMusic`, `chartShardsOf`, `lastShardOf`,
`peopleNameOf`, `chartMarksOf`...) take the people's generator. `cultureOf(g, people)` and `loreQuick`/`loreOf(sys, b, g,
which)` take the world's and the index.

**The divide.** The two lived on either side of a great circle (`peopleDivide`): twelve poles drawn from the seed, forty-eight
points along each sampled at 2 km (the sea, the inland water and a dead desert's old sea count as water), the one crossing the
least land kept, so where the world has two continents the divide runs through the sea between them, and on the test world
it crosses land at 23 of 48 points. It is found once a world and kept (a seed-keyed cache under a mutex, like the landmark
cells'). `peopleAt(g, lat, lon)` is the side of a point; `ruinOfCell` sets `RuinSpec::people` from the settlement's centre
and lays it out in its people's culture. The frontier of Eleinewai I (0.00 N 47.33 E, `vesperis_test peoples 152 92 0`) has
23 settlements of each people in the 13 x 13 cells round it.

**A people's fifty.** Each people has its own fifty (`SHARDS_PER_WORLD` is a people's count), so a world of two holds a
hundred: the second people's take the indices from fifty up in the guide's keys (`<body key>/S57`) and the view's sites
(`shardSitesOf` draws from `shardWorldIndex(spec.people, local)`), and `shardPeopleOf` / `shardLocalOf` split a world index
into the people and its place in that people's fifty, which every catalogue (`shardsOf`, `Lore::last`, the decoder's lists)
counts from zero. The taking says `N OF 100 ON THIS WORLD` there (`shardsOfWorld`); the log line is unchanged otherwise.
The decoder (`game/shards_screen.cpp`) lists a world once a people whose shards are held, each entry with the people's name
after the sector (`THE KHOKURT`; the world's system is generated once at the opening, so a world of two is named as such
from the first shard of either), with its own count, language share and read count; a people's list holds its shards
alone (`ShardWorld::which`, `base`), its header names the people (`UNKNOWN - THE KHOKURT - 4 OF 50 - 53% LEARNT`), a
shard's and a piece's header likewise, the language is learnt from that people's shards alone, and the second people's
last recording closes its own timeline under its own key (`Guide::ended` `<body key>/P1`; the first's stays `<body key>`).
A one-people world's screens are byte for byte what they were.

**They speak of each other.** `Lore::other` is the other people's name and `Lore::otherFate` their end against this
people's (-1 they ended first, 0 together, 1 they went on): the grammar's "other" kinds (`Kind::other`, appended to the
table so the one-people worlds' draws are untouched; they are candidates only where `other` is set) tell of their
traders over the mountain, a buried stranger whose stone faces the other way, their words and their god, the fire on the
headland no fire answered, their river failing or their families asking for land, and an end-tone kind whose first
sentence is the fate (`{otherfate}`: their fires went out years before ours; they are quiet on their side of the water; they
still light their fires and will come for what we leave). A few of each people's fifty name the other (three and four on
the test world; since H-04 where their histories met, and R-413 made Eleinewai I's two peoples prehistoric, their lands apart and
neither sailing: they never met and name none), and the name is kept as it is in the decoding (`languageOf` adds it to the names). Whether they ended
together is a coin of the world's (`peoplesEndedTogether`): the second people's `ageYears` is then the first's (its decay,
its drifts and its road wear follow from its own wet), else its own log-uniform draw; the calendar is the world's for both
(the day, the moon and the year are physical), the spans and the counts of years are each people's own.

**On the ground.** The view holds a culture and a lore a people (`SurfaceView::cultures`, `lores`, `peoples`) and builds
each cell in its settlement's (`ruinCell`: the pieces, the shard sites, the graves and their names, the bank and grain the
forms are drawn in: A-02's `FORM_BANKS` of the people), so the two peoples' ruins stand in their own styles and their stones carry their own names and counts. The
roads: a `RoadNode` and a `Road` carry their people; a road between two peoples' settlements is built as the bigger
settlement's people built (`roadPeopleOf`: the first people's when the two are of a class), so its width, its paving and
its wear are that culture's, and the site keeps each road's paving and wear on the road itself (`SiteRoad::paved`, `wear`;
`roadCultures[2]`) rather than one setting a site. The frontier of the test world has five roads between the peoples among
sixty-one. The radar (C-07) hears both: on a world of two the slot's people is a coin of the slot's (`radarStartContent`),
the world built again for the other people when the air changes hands, the `heard` key the world index.

**The harness.** `vesperis_test peoples [count]` scans the civilisation worlds near home until `count` of two peoples are
found (eight in the first fifty-eight: 14%) and prints each with its peoples, their ends, the divide's pole and land
crossings, a land point deep on each side and the frontier; `peoples <sx> <sz> <body index> [latDeg lonDeg]` prints one
world's peoples in full (names, culture, calendar, span, last, voice, music, the shards of each that speak of the other)
and the settlements of the 13 x 13 cells round the frontier (or the point) by people with the roads between the two. The
word `second` anywhere in the arguments of `shards`, `music`, `voice`, `charts` and `graves` asks for the second people;
`ruins`, `graves` and `roads` print each settlement's and road's people on a world of two. The scene
`civilisation_second_people` stands outside the gate of the nearest settlement of the second people from a land point deep
on their side of the scan's first world of two whose light allows (Thyaveielea II's walled town). The unit's four checks
(143) are in `08-testing-and-tools.md`; the flow's `shards_peoples` and `shards_second_people` frames show the decoder's
list with a world of two and the second people's list.

**Not done.** A star chart of another people that marks a world of two peoples names the first people alone (C-10's
`peoplesWorldOf`), and the signal's source line reads the world as one (KI-354). The data sheet, the landmarks and the HUD
say nothing of two peoples: the explorer learns it from the ruins' styles, the stones' names, the decoder's entries and the
shards, which is as it should be.

## The ages (A-01)

The expansion of 1.3.0 (`docs/archive/ideas/IDEAS-civilisation-expansion.md`, section 1) starts here: a people is found at the age
it had reached when it collapsed, one of the user's six (prehistoric, medieval, industrial, modern, futuristic, far
future), and has a profile every later milestone reads, built to the fields of H-01's data model
(`docs/archive/ideas/RESEARCH-records.md`, section 6). `peoples/peoples.*`; generation 13.

* **The age** is drawn from the people's generator (`peopleGen`) by the decided shares, 28, 32, 18, 13, 7 and 2 per cent,
  whatever the world (within 25 ly of home 28.2, 33.0, 17.0, 12.5, 7.1 and 2.1 of 1,501 peoples). R-413: a world's second
  people reached its first's age ("Two peoples of one age (R-413)", below).
* **The eras** are the ages passed, one each, with their years in the people's own count: year 1 is the first towns'
  founding (the medieval age's start) or a prehistoric people's first village, and the ages before it are counted back (a
  later people's prehistoric age 3,000-20,000 years before year 1). A whole age runs 1,000-4,500 years (medieval), 120-400
  (industrial), 70-300 (modern) or 250-3,000 (futuristic); the age reached is cut by the end (a prehistoric people's
  villages 400-4,000 years, a far-future people 800-150,000). Each era carries the government it closed with, by its age.
* **Radio** comes 55-85% into a whole industrial age: an industrial people had it if it got that far (four in ten), every
  later people had it, 29% of all.
* **The end** is C-08's draw, log-uniform over the age's span: 200-40,000 years for the first two ages (C-08's own),
  200-60,000 industrial, 250-100,000 modern, 400-250,000 futuristic and 1,000-2,000,000 far future, since the late ages' works
  outlast them. The culture's decay reads the years over the age's durability (1, 1, 1.5, 2.5, 6 and 40:
  `Culture::weatherYears`), so a late people's ruins at a given age look like a medieval people's at a fraction of it; A-03
  gives each material its own. Two peoples that ended together share the first's years (C-13).
* **The collapse.** Its cause is one of 37: the ideas note's section 8 by age, each at 0.7-1.3 in its own age and a little in
  the next (D-05: a 38th, the loss of the sun, outside the pick and given to a drifter's people alone: below). The world's disasters weigh in at every age:
  * the drying: ×6 on a desert (the world dried with its people: C-01's old seas), ×0.15 on a living world;
  * a stellar flare: ×6 round a red dwarf;
  * a volcanic winter: by the world's volcanic traits;
  * an impact: by the system's comets and belts;
  * the cold centuries: by an eccentric orbit or a cold world;
  * a flood: by the world's water.

  Where the other people of a world went on after this one, conquest and war come from them. Two peoples that ended together
  share one cause that can end a world, drawn by their age (R-413: one age; drawn over both ages before). The collapse takes years by its cause (an impact 1-25,
  the drying 60-600), within the last era. Its four stages (the warning, the onset, the crisis, the end) are shares of it,
  set by the cause's speed.
* **The extent:**
  * the share of the world that was land in their time: 64 points at 2 km, a dead desert's old seas counted as water;
  * the share of that land they held at their height, by the age: on average 14% prehistoric, 33 medieval, 49 industrial,
    63 modern, 68 futuristic, 48 far future (half that on a world of two);
  * how many they were, by a density for the age over that land: medieval peoples mostly 100-700 million, modern ones a
    few billion.
* **The rest:**
  * the government at the end;
  * what they lived by, a first and a second: hunting, herding, farming, trade, industry or information, by the age (on a
    desert herding before farming, on a world of islands trade);
  * what they believed, by the age: the ancestors, the spirits of the land, the sky, many gods, one god, a philosophy, the
    machine; the rite and the holy place follow the faith;
  * five values (war, piety, trade, learning, art), tilted by the age, the government, the economy and the faith.

  Each is a name the grammar will read: `AGE_NAMES`, `CAUSE_NAMES`, `GOVERNMENT_NAMES` with the ruler's title as a man and
  as a woman, `ECONOMY_NAMES`, `FAITH_NAMES`, `RITE_NAMES`, `HOLY_NAMES`, `STAGE_NAMES`. The lore's names stay C-02's.
* **Two functions:**
  * `peopleAgeOf(g, people)` reads the generator alone: the age, the eras, the end and radio, which is what a ruin cell, the
    culture and the radar need;
  * `peopleProfileOf(sys, b, g, people)` builds the whole profile in 0.25 ms, most of it the land's 64 samples.
* **Shown, not told** (decided with the order). No screen names an age: the ruins will show it (A-02, A-03), the records
  will tell it (H-04), and the decoder's history page names it once read (H-06). So far in the game an age changes the ruins
  (A-03: what the people built and how it weathered) and who is on the air.
* **Only the radio ages on the air.** A transmitter sector's star sends `starSignal`, and the local signal, only when one of
  the peoples of its first people's world had radio (`peoplesOnAir`). A world of two broadcasts only the peoples that had it
  (`slotPeople`, with C-13's coin where both did).

  Three peoples in ten had radio, so the cells hold 3.4 times as many candidates: a mean of 0.384 a cube (one with
  probability 0.306, two 0.034, three 0.0034). The mean heard within 200 ly on the arm stays 1.85.

  The salt was pinned again from sixty tried (nine gave home nothing). Home hears three peoples of three ages: a modern one
  at 90 ly, a futuristic one at 184 and an industrial one at 197. Each test world hears four, a far-future people among
  them. C-15 kept this salt and pinned the transmitters' powers with it ("The radar's gain", below).

  A chart marks a transmission as heard (C-10's `how` 2) only where the people charting had radio and the marked star's
  people is on the air.
* **A test world of each age:** the nearest to home on the harness's plane (sector y 0), pinned in the harness as
  `AGE_WORLDS`:

  | Age | Sector, body | World | Distance |
  | --- | --- | --- | --- |
  | Prehistoric | 198 55 body 10 | Navio V-a, felisian | 9.5 ly |
  | Medieval | 200 41 body 1 | Faiyiari II, desert | 7.1 ly |
  | Industrial | 189 45 body 2 | Meihimeeri III, felisian | 6.1 ly |
  | Modern | 208 51 body 5 | Yeesheleemiola II-b, desert | 13.9 ly |
  | Futuristic | 210 33 body 0 | Iamaielyari I, desert | 19.8 ly |
  | Far future | 202 26 body 2 | Seanesaila III, desert | 21.2 ly |

  The C series' test worlds:
  * Aieliaalas II's Rauzhia are industrial, without radio, and ended 31,594 years ago; weathered as 21,063 years, their
    decay is 0.91 as before.
  * Leileashphail III's Feloya are medieval and ended 209 years ago by the drying, over 401 years.
  * Eleinewai I's two peoples, both prehistoric (R-413; the second was medieval before), ended together by a stellar flare
    1,005 years ago.

  R-413 moved the modern one: Rieneseirela VI-a's second people (11 ly) took its first's age, medieval, and the nearest modern
  people on the plane is Yeesheleemiola II-b's first (13.9 ly, the modern scene's world, two modern peoples).
* **The harness:** `vesperis_test peoples ages [count]`, and the profile in `peoples <sx> <sz> <body>`
  (`08-testing-and-tools.md`).

## The lights left on (D-04, for D-05's peoples)

A dead people of the late ages may have left its lights burning (`TR_LIGHTS_LEFT_ON`, a trait of the drifters' own: `05-planet-function.md`,
`06-surface.md`). D-04 builds what it shows and gives it to no world: D-05 gives it to the drifters' peoples of the late ages. From orbit a
point of light at each of the people's places on the night side (`drawDarkLights` from the world's plan: a capital or a city brighter than a
town); on the ground a third of the forms' window cells lit (a people whose light glows in its own bank, every other in the lamps' warm one,
bank 15), far off a scatter of lit points as many as the lit windows' share and a point for every eight metres up a silhouette, and street
lamps every 30 m along the towns' streets, fewer where the years left less of the street. `rogues dark` stages it on a modern people's
capital at night (`rogue_lights_left_off.png` against `rogue_lights_left_on.png`).

## The people who lost their sun (D-05)

One frozen felisian or desert drifter in four kept a people (`drifterHadPeople`: a hash of the drifter's own, `DRIFTER_PEOPLE_SHARE`; 103
of the 406 within 9,000 ly of home), and its story is the lost sun's, no other world's. 17 peoples on 13 drifters within 3,000 ly, 127
within 9,000 (prehistoric 27, medieval 48, industrial 20, modern 22, futuristic 9, far future 1). `docs/decisions/log/6-1.4.0.md` (D-05) has the reasons.

* **The then-world, with a system** (`formerSystemOf`, `peoplesWorldOf`, `galaxy/system.h`). The star it came from is D-01's real star,
  chosen near its way's end among the living suns (yellow stars, orange and red dwarfs) to fit the way it was lost (`drifterSunWayOf`
  first: a system with a giant for the wanderer, a companion for the dance), with its own system as `generate` makes it; the drifter
  stands at its former orbit in that system's temperate band (`Body::former`: its year, its eccentricity, the moons that came with it), a
  desert drifter warm then (270-320 K). The profile, the lore and the calendar, the history, the records' world facts, the charts and the
  instruments read that system (`peopleProfileOf`, `loreQuick`, `buildHistory`, `writeRecordTexts`, `recordWorldFacts`, `chartMarksOf`,
  `chartOf` swap it in); the ground (the towns, the works, the records' homes) reads the planet function's then-world as P-04's dead
  worlds do. A people's history is the same built from the decoder (its system) and from the ground (the planet function alone).
* **The cause** (`CAUSE_SUN_LOST`, "the loss of the sun"): the 38th, outside the old pick (`CAUSE_DRAWN` 37, every other people's draw as
  it was), given to a drifter's people. Its way (`SunWay`): the wanderer (a giant of their own system grew nearer each year, then the
  summers came late and short; nine of the 17), the intruder (a star brighter each generation until there were two suns, then neither),
  the dark (the stars bent round a point where nothing shone), the dance (a companion stretching each year until one let go). Its timeline
  is the generator's alone (`sunLossOf`, the profile, the culture and the age read it alike): its length by the age (a prehistoric
  people's a few generations, a futuristic people's centuries below ground, the far future's over a thousand years), the warning's omens,
  the onset's short summers, the crisis while the sun went, its last sunrise (`PeopleProfile::lastSunrise`), the long dark below; two
  peoples on one drifter end together. The sun's light in a year is `1/(1+9p²)²` of the way's progress (`sunLightOf`); the dark's stage
  (`sunDarkAt`): 0 as ever, 1 dimming (under 0.9), 2 going (under 0.25), 3 gone (after the last sunrise).
* **The history** (H-02): a SCRIPTS row (the long cold from the onset, the harvests falling, the deaths of the cold, the warmest ground
  left last: `OR_WARM`, a field of embers and the deep valleys), the sunlight kept in the history (`History::sunlight`) and its own event
  (`HE_SUN`: the sun smaller, the year that did not end, the sea frozen from the shore, the air fallen as snow, the last sunrise, the
  stars at noon), the omens of the way (the wanderer growing, the new star, the bent stars, the dance), the whither below (`WH_UNDER`), no
  flood or drought once the sun was going. They went on inventing while it dimmed (their radio among them) to the last sunrise, and laid
  their plan's railways after the railway.
* **The records** (H-04): the year's facts `dark` (0-3), `darkyears` (the years since the last sunrise) and `sunway`; eight genres of their
  own for every age from the prehistoric (`grammar/peoples/sunlost/`): the omens and the last harvest (`omens`), the myth of the sun walking
  away and the fires lit on the hills to call it back (`sunmyth`), the observatory's table of the sun's disc falling year by year
  (`sunmeasure`, the `@suntable` directive), the letters to the sun (`sunletter`), the count of the days without a dawn (`daycount`), the
  rules and the broadcasts of the halls below (`deephalls`), the lesson of what the sun was for the children born in the dark (`sunlesson`),
  and a radio people's last broadcast (`lastbroadcast`, its last recording); the loss told in the ordinary registers (`<register>.sun`); the
  cause's rules given theirs (`cause.what`, `cause.noun`, `last.cause`, `em.cause`, `mod.cause.noun`); the sun's, the seasons' and the
  weather's words darkened by `dark` (`@instead`). Their tones darken toward the end (`Lore::dark`). Every line of theirs is `@apart`: their
  words make no other people's word common, so no other people's records moved (a unit check pins the age and scene worlds' as D-04 left
  them).
* **Charts and music** (H-05): the sun's measure (`CHS_SUNMEASURE`: shrinking suns along a cave's wall, rings drawn with the compass, the
  observatory's printed curve), its caption read from the chart; their star charts mark the star that was their sun (`ChartMark::how` 4:
  Enter flies there); the dawn song (`PF_DAWN`), the last sunrise's (`PF_LASTSUN`) and the last song (`PF_LAST`) in the dark years.
* **The works of the end** (T-03, the table above): the last shelter by the warmest ground (a medieval people's halls under the mountain:
  its mouth in the rock, a gatehouse, walls), a shelter by each of the two places left last after the last sunrise (the last records of
  their own years in them; the last recording in the last shelter alone), a dome over the warm ground by each of those places with its
  glasshouses, the graves of the last generations on a slope facing the last place. The last city is the last place; the late ages' lights
  are left on (`TR_LIGHTS_LEFT_ON`, D-04's, given to the futuristic and far-future peoples). Their works weathered only until the cold took
  the world (60-250 years: no rain, no wind, no thaw since), so the glass is gone and the iron stands.
* **The beacon** (`drifterBeaconOf`, `peoples/signals.h`): a radio people whose history invented the radio before the last sunrise left its
  last broadcast as its last recording, on the air ever since (the first people with one, where both had). Under the drifter's own voice a
  machine of C-16's by the people's age (an industrial people's bell or tones, a modern people's tones, siren or data, a futuristic
  people's data or tones, the far future's mathematics), in the cabin's and the suit's radios too (D-03's slot); on the radar camera the
  message itself, the recording's opening in the people's voice slowed and worn, three times and a long gap (`beaconProgramme`). Locked, it
  is heard (`Guide::heard`): the last recording found in the last shelter is the one the radar caught, read as their language is learnt.
  32 beacons on the 40 radio peoples within 9,000 ly. The decoder's history page closes ENDED BY THE LOSS OF THE SUN once a record read has
  told the cause.

| Mode | What |
| --- | --- |
| `peoples rogues [ly]` | the drifters within ly of home (3,000) that kept a people: the star it left, its former orbit, each people's age, radio, way, years and records, the beacon |
| `shards drifters [n] [held]` | the two test drifters' records (`TEST_DRIFTERS`: Balyaphebist's medieval people, Vreikheid's two modern ones), fifty each, in full (or as decoded with `held` shards) |
| `shards rogue <k>\|age=<age> [n] ...` | the k-th drifter's people nearest home, or the nearest of an age within 12,000 ly |
| `grammar rogues[=ly]` | the grammar's checks and measures with the drifters' peoples within ly (3,000) besides |
| `charts rogues` | the sun's measure and a star chart in each age's way (`shots/tests/chart_rogue_<age>_measure.png`, `_dark.png`) |
| `works rogue <k> [ly]` | the k-th drifter's people's works (the works of the end among them) |
| `rogues voices` | (besides D-03's) the test drifters' beacons through the receiver, as the radar plays them locked (`rogue_voice_13_beacon_vreikheid.wav`) |
| `ruins kinds` | the genome page holds the domes and the glasshouses of each age |

## The history (H-02)

A people had a profile (A-01) and a map of its world (T-01), but nothing happened in its years. Now each people has a history
(`peoples/history.*`, `docs/archive/ideas/IDEAS-civilisation-expansion.md` section 8), simulated before its towns, its works and its records so
they can read it (Dwarf Fortress's order: the world's history first, then the sites and the legends that tell it), to the data model of
`docs/archive/ideas/RESEARCH-records.md` section 6. Deterministic from the profile, the plan and the world; a year at a time from the capital's
founding (year 1, the reckoning's start) to the end; 0.9-6.5 ms on the six age worlds, the world's plan built first (`historyOf` keeps
the last eight; `buildHistory` builds one). T-02's towns read it (their old cores, a place left early: `historyOfWorld`, the approach's
plan thread building it). Generation 13 holds.

* **The eras**: one per age passed (A-01's), each with the people's own name for it (the Old Time, the Gathering, the Kingdom of Kauza,
  the Temple Years, the Age of Smoke, the Engine Years, the Years of Light, the Years of the Habitats, the Stillness), the government it
  began with and the one it closed with (changed once within it: a kingdom founded by the chief who ruled, a revolution after a short
  war, the priests taking the rule, the companies buying it, the machine given it), its faith (a prophet preaches a new one where the
  era's is not the last's) and what it lived by. The old time before year 1 is a few round-year legends: the people's coming into the
  land (from beyond the mountain, over the sea, down the river, across the dry country), the first herds, fields, boats and pots, a
  great flood or drought or winter remembered, the stones raised where the capital would be, the first village there.
* **The places** are the plan's, founded in its order, each in the era whose ground suited it: a modern people's old cities in its
  medieval age (the plan's ground scored as the medieval age would, over the territory: `planScores`), its mill and station towns in
  the industrial, the rest in its own age (a last age too short before its collapse founds nothing); the land filling slowly, then
  fast, then slowly. Each has its founder (a figure, the settlers' place), the age of its old core, and its end: a few left early (after
  a sack, a sickness, a flood, an earthquake, an eruption, a stone from the sky; a mine worked out, a band moved on, a river's new
  course, the soil spent; at most one place in seven), the rest left in the collapse in the cause's order (the frontier first in a
  conquest, the coasts in a flood, the cold in an ice age, the dry in a drying, the crowded in a sickness, the small in a leaving), the
  least exposed the last place. A futuristic or far-future people's older cities, which its plan no longer draws, are places too
  (12-26, on the territory's best medieval ground), founded in the old ages and cleared in its own; a far-future people's rule sat in
  its old capital until it was cleared (`historySeat`).
* **The rule**: chiefs, elders and high priests chosen for life; a monarchy's houses (the heir by the people's rule of succession: the
  eldest son, the eldest child or the eldest daughter first, then a grandchild, a sibling, a sibling's child; a child under a regent,
  its other parent or a lord, until sixteen; with no heir the lords choose a new house, or two claimants fight for the rule), a house
  founded with its founder's household (a spouse and the children living); consuls and speakers elected for terms of three to seven
  years (re-elected once at most), directors appointed; the machine's rule a reign of no person; terms longer in the later ages (two to
  twelve times: they lived longer); a ruler deposed or murdered now and then; the rule ends in the collapse with its last ruler.
  Regnal numbers where a name came again (a prince named after a grandparent: Klaunaith I and II).
* **The figures**: founders, generals, rebels, regents, prophets, makers, inventors, explorers, envoys, artists and the last, each a
  person with a life (born, died, home, trade, traits); a fifth of them, in the families' years, from the families followed.
* **The families**: ten households followed through the last 600 years (from year 1 on a short history): births (at the age's rate,
  named after a grandparent one time in five), marriages (now and then into another family followed; the couple living where the
  people's custom says), moves (the young to the cities, most in the industrial age), deaths by the age's mortality (a medieval child's
  first year one in five, a far-future person past two hundred) and by the year's harm where they live (a sickness, a famine, a battle
  for those of fighting age, a flood, the collapse), each with its cause in its cause's season; past eight living only the family's main
  line marries in (the others' branches let go, alive as kin until a death drawn alone); a family that dies out is replaced by another
  household; a ruling house followed while it rules. Surnames from the industrial age (`historyPersonName`); kinship between any two
  (`historyKinship`: mother, son-in-law, cousin).
* **The events** (`HistoryEvent`, 46 kinds: the slots of the research's model, a cause, a season, public or the families'):
  * war: a band's feud, a war between cities, a rising against the rule, a war of succession, a revolution, a civil war; their
    battles (a battle's dead by the people's number, at most an age's: 300 a band's, 30,000 a medieval battle's, 600,000 a modern
    one's), sieges and sacks, the peace and who won; more by what they valued, the government and a cruel or ambitious ruler;
  * sicknesses in waves, named in the people's way (the grey fever, the sweating pox), coming back within two centuries; harvests (a
    year's from the climate and the cause's trend: a failed one told, a poor one where a famine followed) and famines; the world's own
    disasters: floods by its water, droughts on a desert, earthquakes and eruptions by its volcanoes, stones from the sky by its comets
    and belts, flares round a red dwarf, long winters by its cold and its orbit (`readWorld`);
  * works by what they valued and could build (temples and the faith's holy place, walls, fortresses, palaces, bridges, harbours,
    libraries, observatories, theatres, mills; stations, factories, schools, hospitals, masts; towers, stadiums, dams, launch sites;
    arcologies, habitats, elevators to orbit, rings; the genome's tombs, monuments and focal work; the far future's works finished);
    the plan's ways opened (between two great places, every railway, canal and maglev; rail after the railway);
  * inventions at their share of each age (the steam engine before the industrial age, radio at A-01's year, a landing on the moon
    where the world has one), discoveries (the far shore before its first place over the water, a river's source, the shape of the
    world, the far side, the poles, the other worlds), faiths (a prophet, a schism, the rites reformed), institutions (the priesthood,
    the guilds, the Academy, the Company, the Gazette, the University, the Assembly, the Broadcast, the Network, the Mind), deeds,
    unrest (strikes, risings, riots over bread), the countryside moving to the cities, the capital's centuries, omens (the system's
    comet returning by its period, eclipses by the moons, aurorae, falling stars, a new star).
* **The other people** (C-13): where their years overlap (in years before the present: each people's end and count) and their lands
  touch or one of them sailed, they meet; treaties and wars follow by what both valued, with battles at the frontier's places, and word
  of the other's end reaches the one that went on. One list for both, so both histories hold the same years: on Eleinewai I nine
  shared events, on Meareafionris III thirteen (R-413: of one age, neither pair meets now; the check reads Leanmya V-a's 42 and
  Hiameshseine I-a's 47, its conquest among them). Where a people's end is a conquest and the other went on, the other conquers it (its
  battles inward to the capital), meeting it before if they had not met.
* **The collapse**: the profile's cause in its four stages, each marked; the signs of each stage at the places most exposed (the
  wells of Tsearis sank; the fields of Tsearis turned to dust; the caravans no longer reached Tsearis; the last well at Kauza was
  guarded); the onset's harm (an impact, a flare, a great sickness, the war, the drought, the long cold, the fire), repeated through the
  crisis; the places left in the cause's order with the reason (its wells failed, the ice came, the heat and the sea), their people
  leaving for the last places or over the sea, into the sky, elsewhere, into the machines; the rule ending; the end at the last place
  with the last person (the eldest of the families there).
* **Causes kept**: a famine names its harvest, a battle its war, a peace the last battle, a death the sickness, the battle, the famine
  or the end that caused it (in its season), an accession the death or the term that opened the rule, a regency its accession, a
  place's end the harm that ended it, a sign the sign before it; within a year an effect is never told before its cause.
* **What it gives the later milestones**: the events of a year (`eventsIn`), a person's (`eventsOf`), the reign of a
  year (`reignAt`), its harvest (`harvestAt`), the inventions made by then
  (`inventionYear`), the seat (`historySeat`), the sentences of a legends print (`historyEventLine`).

Where to see it:

| Mode | What |
| --- | --- |
| `history <sx> <sz> <body> [second]` | a people's history as Dwarf Fortress's legends: the head (the span, the height, the end, what was told by kind, the build time), the eras, the rulers by house, the places, the wars, the sicknesses, the inventions and the institutions, the figures, the families, one family's years, the chronicle, the collapse to the last person (`shots/tests/history_<world>[_2].txt`; the screen leaves out the chronicle and the towns) |
| `history ages` | the six age worlds' peoples |
| `history check` | the unit's H-02 checks alone |

## One age a world (B-414)

The user found a disc in what read as a medieval village and a lattice mast over its ruins (a modern people's world: its villages a
medieval core in rings of industrial rows and modern suburbs, its records over its eras, its modern houses in its medieval forms) and
chose that a world show only the age its people reached. The history (H-02) keeps every era; the ground is the last age's.

* **The age's first year** (`ageFromOf`, peoples/peoples.h; `Lore::ageFrom`): the start of the last era in the people's count (1 at
  least). A modern people's last age runs 30-300 years, an industrial people's 30-400, so its records and its dead fall in those.
* **Towns** stand in the last age's way, one layer (`placeHist`, `countrysideHist`); a place of the plan the history left before that
  age is gone (`TownHist::gone`: `cellSettlement` returns nothing for its cell, its works go with it: `WB::gone`), one left within it
  weathers from when it was left. Over the six age worlds 40 places are gone and 39 test towns are all their people's age.
* **Works** (`peoples/works.cpp`): `placeAge` and the history's works give the people's age (`eraAge` is gone): a modern people's dam,
  mine, observatory or lighthouse its history finished in an earlier era stands as a modern one. Gone: a battle of an earlier age
  (`e.year < ageFrom`), a later people's henges (a prehistoric people's own stay), a modern or later people's aqueduct (a medieval and
  an industrial people's in their age's way). A necropolis of barrows raises its age's stelae (menhirs a prehistoric people's).
* **Records**: every record made in the last age's years; the medium, the category's share, the genre, the music, the chart's way and the
  instrument beside a piece follow. **Histories** (`peoples/grammar/core/history.vg`): an industrial people's on printed pages or a codex, a
  modern people's on printed pages or a drive, a futuristic people's on a drive or a crystal, the far future's on a crystal (`has:past`:
  an era before the record's since year 1). `@past <n> <rule>` draws one of the earlier eras (the nearer the likelier) and a year in it,
  takes a run of up to n years with something in them back from it (forward too in a quiet stretch), binds each year's facts and ruler
  (its era's name, government, age and inventions) and tells it by `<rule>` (`history.year`: C-02's chronicle heading and the annal's
  register), the gaps by `<rule>.gap`, `<rule>.open` and `<rule>.close` with the first and the last year's facts; the record's own come
  back after. About one record in twenty: SLEEPING's modern people 7 of 150, the futuristic age world 7 of 175.
* **Graves**: a death in the age's years while the place stood (`diedIn`).
* **The machine ages' buildings their age's** (`ageRoof`, `Layout::formOf`, `culturePalette`): an industrial or a modern people's villas,
  suburban houses and blocks on a rectangle, an L or a court; their roofs the people's where the age built so (flat, pitched, hipped, a
  row's crow steps in the industrial age; flat, pitched, hipped, a saddle in the modern), else hipped slate (industrial) or flat (modern;
  B-415: the age's alone, below);
  their windows the age's (slot 0 the dwellings', 1 the public and the tall buildings'): an industrial people's sash windows, round-headed
  where its openings were arched, round, pointed or keyholed, a modern people's wide panes and bands. Slot 2 keeps the people's openings
  for its temples, palaces and focal accent (the suburbs, the mills, the malls and the lighthouses left it); slot 3 (the age before's, for
  the old cores) is gone. The temple keeps the people's plan and roof in every age.
* **A mill's doorway is in its front** and its chimney in its back end: all-industrial towns stood mills side by side, and a side doorway
  met a neighbour's wall or chimney.
* **A guide saved before** (`records 2`) keeps its found shards by their index, their contents new, said once (`records 3`).

Where to see it: `towns check` (the one-age check), `genome pair <sx sy sz body people> x 2` (a people's town in every age, the machine
ages' houses now their age's), `records <sx> <sz> <body>` (a people's records by their years' age and medium: one age), `works ages`.

## Two peoples of one age (R-413)

The user: "it won't make sense to have futuristic society next to prehistoric [...] it would be cool if both societes were of the same
age". A world's second people reached its first's age (`peopleAgeOf`): its own draw from its generator is taken and set aside
(`ageOfPeople(pg, age)`), so the lengths of its eras, its end's years over the age's span and its radio's year stay its own draws within
the shared age. Before, each people's age was its own (of 600 worlds of two, 22% of one age, 36% a stage apart, 42% two or more); now
every world of two is of one age, and 466 of those 600 (78%) have a new second people, the rest and every first people as they were.

* **A people's own generator.** The charts and the records' categories read a people's age from its own generator
  (`peopleAgeOf(pg, 0)`); the generator keeps its world's seed (`BodyGen::worldSeed`, set by `make` and kept by `peopleGen` and the copies
  with a people's seed), so a second people's generator reads as that people.
* **One age, not one time.** Ended apart (half the worlds of two), each people's end is its own draw over the age's span: two peoples
  of one age may have ended far apart (Rieneseirela VI-a's 34,293 and 427 years ago), the one's ruins worn and the other's fresh, and
  never met (H-02 meets them where their years overlap and their lands touch or one sailed: 13 of the 35 worlds of two nearest home
  met). Ended together, they share the first's years and one cause, drawn by their age.
* **On the air.** A world of two transmits where one of its peoples had radio: 33.5% of them (47.8% before), all civilised worlds 31.3%
  (34.2%). The weak transmitters' powers were pinned again so every step of the gain adds one to three peoples from home and the three
  C-series worlds (`SALT_POWER` 0xC15A0404; home 3, +2, +3, +2, +1, +1); the cells and the strong transmitters are as they were.
* **The test worlds.** The modern age world is Yeesheleemiola II-b's first people (208 51 body 5; Rieneseirela VI-a's second, medieval
  now); H-02's worlds of two are Leanmya V-a (179 57 body 18, they met: 42 shared events) and Hiameshseine I-a (202 9 body 5, a
  conquest: 47); the works' checks read Shioleesein I-a too (196 66 body 7, two dams holding their water).

## What the later milestones take from here

C-03 placed them, C-06 reads them, C-04 plays them, C-05 speaks them, C-07 hears them from afar, C-08 sets their ruins in the terrain, C-10 draws their skies and C-09 joins their ruins by road (above); C-12 dated them in the people's calendar, drew their timeline and
marked the last, and set their graves outside the ruins (above); C-13 gave one world in five a second people, a second seed
with everything of its own (above); C-14 lent the shards found and read to a friend's decoder (above); C-16 put each age's
broadcasts on the air (above); A-02 gave the renderer the forms the ages will build with (above); A-03 built each age's buildings
from them and weathered them by their materials (above); A-04 gave every people its architecture genome, its own through its ages
(above); T-01 laid out each people's world, its territory, its ranked places and its networks (above); H-02 told each people's
history over the plan's places in the order of their founding (above); T-02 laid its villages, towns and cities as their ages laid
them, their streets, districts, layers and edges, the old cores the history founded them in and C-08's reading of the site by age
(above); T-03 built its works outside the towns by the land's rules and the history's (a battlefield where its war was fought, a
dam of its era, the last shelter by the last place; above), the bridges and the locks its ways need left for later (KI-379); T-04
counted each people's records, dated them in its history, made them in their years' way and laid them in its places, archives and
works, the last recording where the history ended (above); H-03 built the grammar engine, its format, its realiser and its checks,
and rebuilt C-02's kinds on it (above); H-04 wrote the records from the history: the genres of each age, the collapse told from
many sides, the language by roots and the decoder's pages (above); H-05 filled the catalogue's three categories by age: the music
on each age's instruments, the charts in each age's way, the media heard and drawn (above); H-06 read the people's history off the
records read: the decoder's records by kind, the timeline of the eras, the history page of the events learnt, the persons as
named, the people's ways, the age once told and the last recording closing it (above); C-11 played the pieces and the recordings
on the ship away from the decoder and put each people's instruments of its ages in its ruins and in the cabin's case (above); C-17
made those instruments by each people's own way of making, from parts, of what its world gave and in its genome's style (above).
None of them needed a new
generation: the shards are a function of what the galaxy already is.
