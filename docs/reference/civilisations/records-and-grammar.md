# Civilisations: the records and the grammar

One of the five notes on the dead peoples (`peoples-and-ages.md`, `built-world.md`, `records-and-grammar.md`, `sound.md`,
`radar.md`), split by topic from `12-civilisations.md` in E-04; its sections are as they were, in their order.

<!-- contents -->
Contents:
- The shards (C-02): the story grammar
- The shards in the ruins (C-03)
- The decoding on the ship (C-06)
- Star charts (C-10)
- Lending (C-14)
- The grammar engine (H-03)
- The records (H-04)
- Each people its own (H-04's second pass)
- The three categories by age (H-05)
- The decoder reads a history (H-06)
- Each age its own vocabulary (B-415)
- The drone bay (C-18)
<!-- /contents -->

## The shards (C-02): the story grammar

A shard is a recording the people left; decoded, it is a short text or (C-04, below) a piece of music. The texts are
generated without a language model, deterministically from the body's seed and the shard's index, so the same shard
reads the same on every machine and can be found and shared like a landmark (`peoples/shards.*`).

(H-03 rebuilt this grammar on the engine: the vocabulary below and the kinds are `src/peoples/grammar/`'s files now, the slots facts
and rules, `{kin}`, `{child}` and C-02's `{he}`, `{him}`, `{his}` persons the realiser refers to, `tidy` the realiser's seams. See
"The grammar engine (H-03)" below. H-04 replaced the text kinds with the records' genres, written from the history: "The records
(H-04)" below; the star charts' captions and the music keep C-02's draws.)

*Lore.* `loreOf(sys, body, gen)` draws the proper nouns a world's shards share from the body's seed: the people's name,
their god, their river, mountain and sea, two towns, the founder, the moon and the star they named, the festival; all
through `generateName` in one of its three phonotactic styles (the style drawn per world), one word of nine letters at
most, the eleven distinct. It also fixes the span of years the shards cover (150-600) and reads the world's day and
year. A shard's `age` (0..1, drawn from its own seed) places it in that span: `year N of <town>` is its date.

*Vocabulary.* `buildGrammar` fills the slots a template may draw from, from the system and the planet function:

| slot | from |
| --- | --- |
| `{sun}` | the star's class (a companion's world: the companion's): "the sun", "the small sun", "the great red sun", "the beating star", "the ember sun", "the dark star"... |
| `{sky}`, `{skyone}` | the stars and the sun; the named moon (twice, it is the thing most looked at), "the two moons"; the ring; the lights in the north (`auroraPotentialAt` over 0.25); the edge of the day (locked); the second sun (a companion in the system); the hairy star (a comet); the wanderer (a gas giant); the long night (a slow spinner); the mother world (a moon's sky). `{skyone}` holds only what a singular verb can follow. |
| `{land}` | the traits (`TR_CANYONS` "the canyons", `TR_ERG` "the sand sea", `TR_LUMINOUS_FLORA` "the glowing woods", ...) and the type's own (hills, forest, high mountains, marsh; the dry country, the dunes) |
| `{water}` | a felisian world's river, lake, sea, spring, shore; a desert's river, quay and well while the age is under 0.5, then the well, the cistern, the last spring, the bed of the river |
| `{crop}`, `{tree}`, `{animal}`, `{beast}`, `{herd}` | by type and temperature (mass nouns: grain, flax, barley, millet...); the flora families' silhouettes (round trees, needle trees, cap trees, weeping trees...), lamp trees and great trees by trait, thorn trees and date palms on a desert; goats, geese, sheep, cattle; `{herd}` one pick per shard where a text needs the same animal twice |
| `{season}`, `{weather}` | four seasons over 8 degrees of tilt, else the rains and the dry months (a desert: the cool and the hot months); rain, fog, heat, wind, storms; snow and frost on a cold world; dust, heat, wind, cold nights and sand on a desert |
| `{work}`, `{offering}`, `{trade}`, `{omen}`, `{skill}`, `{n}`, `{nbig}` | plain lists |
| `{kin}`, `{kinv}`, `{he}`, `{him}`, `{his}` | one relative per shard with the pronoun that follows them (the child in the text shares it); `{kinv}` the vocative |
| `{child}` | a name of seven letters at most, one per shard |
| `{year}`, `{yearn}` | "year N of <town>" and the number alone |

*Grammar.* `kinds()` is the table of shard kinds: a name, a tone (`ST_ORDINARY`, `ST_ELEGY`, `ST_WARNING`, `ST_END`), the
world it belongs to (both, felisian, desert), a weight, the last age it is told at on a desert (what speaks of living
water stops at 0.5) and its templates. A template is prose with `{slot}` (one entry of the slot; an entry may hold marks
of its own) and `[a|b|c]` (one alternative). Thirty-one kinds, a hundred templates: harvests, children, weather, the
river in flood, seasons, prayers, the founding, love, lullabies, counts, letters, the sky, work songs, namings, the
hunt, festivals, travellers, the council; elegies for a person and (felisian) for a place; on a desert the sea going, the
river, the heat, the dust, warnings to whoever hears, the council that laughs, the leaving; on a felisian world the
fading (the young gone to the other town, empty houses, a sickness); the last recording of each world type.

*Tone by world type.* `pickTone` by the world and the age. A desert: under 0.3 of the span 84% ordinary, 8% elegy, 8%
warning; from 0.3 to 0.85 55% warning, 37% ordinary; past 0.85 70% warning, 20% ordinary, 10% elegy. Overall about
half ordinary and a third warnings. A felisian world: 83% ordinary, 12% elegy, 5% warning, and past 0.9 of the span
half ordinary, a third elegies. The end's tone is one shard's alone since C-12: the world's last recording (below),
the latest of the fifty. So the desert's fifty read as a timeline that darkens and closes, the felisian's as a life
that went on with fewer people in it (the agreed tone, ideas note section 2).

*Text.* `tidy` closes the seams: one space between words, no space before a stop, "a" before a vowel is "an", a capital
after every stop, a stop at the end. The texts are translations, so their nouns are ours (goats, barley, a quay); on
the ship's screen (C-06) they are in capitals, the game's font having no lowercase. What a shard withholds is the
decoder's doing (below), not damage: C-02's `damage` and `Shard::fragment` went with C-06. `Shard::child` keeps the name
the kind gave its child, for the decoder.

*Numbers.* Fifty shards of Aieliaalas II (the first felisian world with the trait near home): 44 ordinary, 2 elegies, 3
warnings, 1 the end (the tones count the pieces of music too, which keep theirs), 13 of them music, 31 words a text; of
Leileashphail III (the first dead desert): 28 ordinary, 4 elegies, 17 warnings, 1 the end, 12 music, 33 words (the
counts since C-12, which made the end one shard's). `vesperis_test shards 151 25 1 50 full` and `shards 153 70 2 50 full`
print them. H-01 (2026-10-08) measured what they repeat (KI-368: over 100 shard indices about a third of the longer
sentences recur on one frame, over 200 half) and chose what replaces them in H-03 and H-04: a history told by a record
grammar (`docs/archive/ideas/RESEARCH-records.md`).

## The shards in the ruins (C-03)

(T-04 replaced the placement: "Where the records lie" below; `shardSitesOf` is `recordSitesOf`, the fifty are 80 to 200.) A settlement held a few of its people's fifty (`shardSitesOf`, `peoples/shards.*`, from the settlement's seed; C-13: the second people's at the indices from fifty up): a hamlet one
three times in five, a village one or two, a town two to four, a lone monument one seven times in ten; distinct within the
settlement, copies across the world (the decisions log: fifty a world, in copies). The buildings are drawn by a weight (a hall 2,
a tower or a rotunda 1.5, a stela 1.2, a house 1) and each tries its spots in order: a house's far corners from the door,
then the corners beside the door's wall, then the back's middle; a hall's far end along the nave (the columns stand in two
rows at half the width); a tower's far corners; a rotunda's side opposite its doorway at 0.55 of the radius; a stela's front,
then its back. A spot must be 0.42 m clear of every piece of the building that stands on the ground and 1.5 m from the
doorway (`buildingDoor`, `peoples/ruins.h`), so the explorer can stand at it; `ShardSite` is the index, the building, the
place (`SHARD_PLACE_NAMES`: a house, a hall, a tower, a rotunda, the foot of a stela), the spot in the settlement's metres.

In the view (`06-surface.md`): a slab of dark glass on the floor with a point of light over it that pulses, drawn behind the
walls' depth, within 250 m; `findNearShard` offers the nearest within 2.2 m on the explorer's floor and `E` takes it
(`Game::takeShard`): the guide's `shards` set (`shard <body key>/S<index>`), a `SHARD` log entry ("A SHARD OF THE OLD PEOPLE
TAKEN FROM A HOUSE IN THE RUINS OF A TOWN ON <WORLD> (3 OF 50 FOUND THERE)"), the status line, and `SHARDS TAKEN` on the
statistics screen. `Game::syncShards` refills `SurfaceView::shardsFound` from the guide at a landing, a load and a taking,
and the view draws and offers no copy of an index in the set. The text stays unread until C-06.

`vesperis_test ruins` lists each settlement's shards; `scene civilisation_shard` stands in the room of one; the unit checks
the sites over 961 cells and the town in the view (`08-testing-and-tools.md`).

## The decoding on the ship (C-06)

(H-04 re-tuned it for the records: the language is learnt by roots, the share reaches all the words at the people's full count
instead of ten shards, and a long text is read in pages; "The records (H-04)" below. The numbers here are C-06's.)

A shard is in its people's own tongue; the ship's computer learns the language from the shards the explorer holds of
that world and reads each shard as far as it can. `game/shards_screen.cpp` is the screen (`07-game-flow-and-controls.md`:
the decoder on the cabin's back wall, the worlds, a world's shards by their years, a shard's text); `peoples/shards.*`
the rules:

*The tongue.* `tongueOf(gen, lore)` draws a people's syllable parts (eight to twelve onsets, four to six nuclei, three to
six codas) from three pools by the style of their names (plain, hard, flowing), with a chance of a coda and of a bare
first vowel; `tongueWord(tongue, word)` hashes the world and one of our words into one syllable for a word of three
letters or fewer, two to six, three past that, never under two letters. So a word of ours is one word of theirs wherever
it recurs on that world, two worlds never share a tongue, and the people's words run about as long as ours.

*The language.* `languageOf(lore, fifty, tongue)` counts the words of the world's texts (the pieces of music have none;
lowercase, a possessive 's set aside, the names and the numerals left out), ranks them by their count (ties by a hash, so
no alphabet shows through) and gives each its word in the tongue, re-drawn when it would repeat one already given.
Aieliaalas II's 37 texts use 294 words 1050 times, Leileashphail III's 38 use 338 words 1210 times. `languageShare(held)`
is the share of those uses the computer reads with that many shards of the world held (a piece counts as a shard held: the
computer learns from every recording): 30% with one, 8% more per shard, all with ten; `languageKnown` takes words from the
most used down until the share is covered (eleven words with one shard, twenty-eight with three, sixty-one with five, all
294 with ten on Aieliaalas II). The names (the lore's eleven and the shards' children) and the numerals are read from the
start.

*A reading.* `decodeShard(shard, language, tongue, held)` splits the text into words with their punctuation and gives
each as ours (read), as itself (a name), or as the people's word, capitalised as ours is; `decodedText` is the line the
harness prints. The screen draws a word read in white, a name in amber, a word not learnt dim, and a reading the
computer has not done at the current share resolves the words as the recording speaks them (C-05, below; it took 0.11 s a
word before); the guide keeps the count a shard was read with (`decoded` lines), so a shard re-read after more of its
world were found decodes again with fewer gaps and the whole text is what ten shards give. On the two worlds the first reading gives 37% and 35% of the words
(the articles, the pronouns, the names), three shards half, five two thirds, ten all (`08-testing-and-tools.md` for the unit's
numbers). `vesperis_test shards 151 25 1 12 3` prints a reading with three held.

## Star charts (C-10)

The ideas note (section 3) asked for charts: "Some shards are a map, not a sound: the culture's own sky, drawn from their
world with their constellations and names. Decoding it on the ship overlays their sky on yours, so you can find the star
they named, and sometimes it points at another dead world. Cultures that knew each other become a chain to follow." C-10
(2026-10-03) builds it in `peoples/charts.*` on C-02's shards and C-06's decoder (`07-game-flow-and-controls.md` for the
screen and the keys).

*Which shards.* Three to five of a world's fifty are charts (`chartShardsOf`, `shardIsChart`: the next of the shuffle that
picks the music, so every piece stayed where C-04 put it). A chart is a text shard without a voice (`Shard::chart`): its
caption names the figures on it and the star it marks, in the grammar's style and by the shard's tone ("The sky over
Aiyin on the longest night: the bridge, the gate, the mother, the crown, the tree and the well. We marked Beysknoon, the
brightest of our nights, in the bridge."; a desert's last: "The last sky we drew: ... We marked ..., so that someone will
know where we looked."), so the figures' names are words of the world's language that resolve as it is learnt, and the
other people's name is kept as a name from the start (`Shard::child`).

*The sky.* A chart is the people's sky as the game draws it: every star of the cube of ten sectors round their star
(`chartSkyOf`, `StarNeighborhood`'s radius, so the same stars are in the sky when the ship is at their star), the
brightest hundred by the star field's own measure (the luminosity over the squared distance). The people's figures are
one thing over their whole sky (`skyFiguresOf`: ten to fourteen, each grown from the brightest star left that keeps
twelve degrees from every star taken, by the nearest star left within thirty degrees of any of its stars, three to six
stars each joined to the star it was nearest to, named from the world's draw of twenty-four names: the hunter, the boat,
the two sisters, the plough, the serpent...). A chart is the hemisphere round the star it marks (an azimuthal equidistant
disc, up toward the galaxy's north) with the figures that fit whole within it, three to six as a rule.

*The marks.* A people's world is at one star in eight near home, so a chart that marks one is a choice, not a chance
(`chartMarksOf`, kept in `Lore::marks`, one per chart, about a millisecond a world): the first chart marks a people the
world knew, the nearest it heard (C-07's transmitters within 200 ly of it: "the star the voices come from: the hearth of
the Xoogreyss"), else the brightest people's world among the sixty brightest stars of its sky ("the star of the Dugukhox:
they are there as we are here"); the second the brightest star of its nights, the one the lore names (`Lore::star`:
"Beysknoon, the brightest of our nights"); the rest a people or a bright star by the world's coin (a later bright star by
a phrase: "the star that rises before Kackcoss"). No star twice. Over forty civilised worlds near home 43% of the charts
mark a bright star, 47% a people heard, 10% a near people. Aieliaalas II's three charts mark the Urghokug (69 ly off),
Beysknoon (a red giant 1.5 ly off, the brightest in its sky) and the Xoogreyss (77 ly); Leileashphail III's the Xoogreyss
(57 ly) and two bright stars. A people heard is a transmitter, so the chain the charts make is the radar's: what a world
marked, the receiver hears from its orbit.

*On the ship.* The decoder (`game/shards_screen.cpp`, `renderChart`) draws a chart beside its caption: the disc with the
stars as dots by their brightness, the figures' lines, their names at their brightest star (white when every word is
learnt, else dim in the people's words), the mark's amber diamond; and names the star marked as the explorer knows it (or
UNKNOWN) with its class, its sector and its distance from here. A first reading at the language's current share draws it
over four seconds (the stars appear, then the lines, then the names), Enter skips the wait, Esc leaves it unread; a chart
read goes into the guide (`decoded`) and the log (`CHART`: the sector and the class of the star marked, never what is
there). On a read chart Enter sets the remote target on the marked star (the chain's next link; `V` flies) and `O` holds
the chart up to the sky: in space and on the surface at night the figures' lines are drawn between the real stars from
wherever the ship is (`Game::drawChartOverlay`: on the sky alone, never over the cabin's walls, a globe or the ground),
the names at their brightest star, the mark's diamond with `THEY MARKED THIS STAR` (an edge mark when it is behind),
until `O` on the chart puts it down. From the people's own star the figures are as the chart draws them; from anywhere
else they are the same stars seen from another place, which is the overlay's point.

*What stays* (KI-351): a chart holds the stars within ten light years of its world, so a brilliant far star of their sky
is not on it; the chart held up is not saved; the overlay is not drawn in the radar camera; a people's marks never reach
beyond the radio's 200 ly; the figures' names are ours in translation (no alien drawing of a hunter), and the other
people's name is the only name a chart carries besides the lore's.

*The harness.* `vesperis_test charts <sx> <sz> <body> [index]` prints a world's marks, each chart's figures with their
stars and its caption, and sketches one chart in text; the unit has two checks (`08-testing-and-tools.md`); the flow's
`shards_chart_reading`, `shards_chart`, `chart_overlay` and `chart_overlay_figure` frames show the screen and the overlay.

## Lending (C-14)

"The guide export already carries names to a friend. Decoded shards travel the same way: a friend hears the music you
found without flying there, credited in cyan like their names are" (`docs/archive/ideas/IDEAS-ruins-shards-signals.md`, section 3).
The export is the guide file as it is; the import ("import an inbox file", `Game::importInboxFile`, `Guide::importInbox`)
takes the friend's names into the inbox as before and now lends their shards: every `shard` key of theirs with a `decoded`
line, that is what they found in the ruins and read on their decoder, goes into `Guide::lent` (`lent` lines) unless it is
already the explorer's own. What the friend was lent themselves does not travel on, like the inbox's names, so a shard is
credited to the one who found it. The status says `N NEW NAMES AND M LENT SHARDS FROM THE INBOX`, an `INBOX` log line
counts the worlds, and the statistics' `LENT` counts what is lent and not taken since.

On the decoder (`game/shards_screen.cpp`) a lent shard is one of the world's held: its world is listed (`N OF 50 SHARDS
(M LENT)`), its row is cyan, `LENT` stands in cyan after the shard's header, it counts in the world's language share like
an own shard (the friend's computer read it, and the recording is on the ship), it is read, spoken or played like one and
its reading goes into `decoded`; a piece the friend named carries their name in cyan (`shardNameOf`: the explorer's own
name wins), and a world the friend named is in cyan too. A lent shard is never taken: the ruins still hold it (the view's
`shardsFound` is the own shards alone), so a world a friend read is still worth flying to, and taking a lent shard there
makes it the explorer's own (`Guide::isLent` is lent and not taken). The harness: the unit's lending check and the flow's
`shards_lent` and `shards_lent_piece` frames (`08-testing-and-tools.md`). One lesson of the harness: a second `Game` alive
beside the first corrupts the first (the flow imports into a fresh guide in the same game and puts the guide back).

## The grammar engine (H-03)

The records are written by a small attribute grammar over a realiser (`peoples/grammar.*`; the formalism H-01 chose,
`docs/archive/ideas/RESEARCH-records.md` sections 4 and 5). The grammar is plain text files with the extension `.vg` on
`src/peoples/grammar/`'s shelves (its note), compiled into the game and read from disk by the harness; the engine draws from them with the
record's model (its facts and its persons), and the realiser writes the English. C-02's kinds were rebuilt on it first (below), from the
same inputs, before H-04 writes new content.

*The files.* Each file is wrapped once in a raw string, so a C++ source includes it where a string goes, with no build step and
no generated header (Make's `-MMD` and CMake's scanning both see the include, so editing a grammar file rebuilds what reads it).
E-11's manifest `grammar/files.inc` is `GRAMMAR_FILES`' rows (`shards.cpp`), three lines a file in the parse's order:

```
R"VG(
# a comment
...the rules...
)VG"
```

```
{"core/records.vg",
#include "core/records.vg"
},
```

The loader skips the wrapper's two lines, so a file read from disk and its compiled-in copy parse alike and give the same line
numbers. A file is kept under 16 KB (MSVC's limit on a string literal; the release builds use clang). `shardGrammarFromDisk`
replaces the compiled-in grammar with the tree's files, its shelves walked (the harness: an edited rule is reprinted without a
rebuild, the texts built again); the unit checks the tree's files are the text compiled in, every one listed.

*A file* is lines of four kinds:

| Line | What |
| --- | --- |
| `# ...` | a comment (a line whose first character that is not a space is `#`) |
| `@root shard chart` | the rules the code expands; `@root letter.*` every rule of that prefix (a directive's); every other rule must be reached from them |
| `@lexicon industrial railway railways` | words of an age, as written (not stems): a record of an earlier age that has one fails the anachronism check; or of a fact (H-04: `@lexicon inv.railway train trains`, a word whose fact is not set at the record's year) |
| `name (options)` | a rule: a name at the line's start (letters, digits, `.`, `-`, `_`); options `pl`, `sing`, `mass` (the number of the words it gives: no "a" before a plural or a mass list, its verbs agree), `keep` (the first draw holds for the whole record: one crop), `steady` (its weights hold across a people's records: no catalogue dryness), `words=8..70` (a record it writes is that long: the length check), `formula` (H-04: a genre's fixed heading or closing, left out of the measures), `people=K` and `share=F` (H-04's second pass: a people keeps K of its alternatives, or that share of them, ranked by its seed among those whose guards hold) |
| `  (options) text` | an alternative of the rule above (indented); options first, in parentheses: a weight (`2`, `0.8`), guards, `once`, `tense=past` or `tense=present`, `pl`/`sing`/`mass` (its own number) |

*Guards* compare a key with a value: `key=v`, `!=`, `>=`, `<=`, `>`, `<` (an order needs a number or an age: `age>=industrial`,
the ages in A-01's order, `prehistoric` to `farfuture`), `has:key` and `!has:key` (set and not "0"); `a=1|b<0.5` is an OR. A key
is a fact of the model (`desert`, `tone`, `moons`), a local attribute (`.slow`), an entity (`has:$e`) or an entity's field
(`$e.kind=war`, `$x.gender=she`). All of an alternative's guards must hold.

*The text's marks:*

| Mark | What |
| --- | --- |
| `{rule}` | expands a rule (the joins are spaces: a parent writes `{lead} {main} {tail}` and the realiser closes the seams) |
| `{fact}` | a fact's value (a name stays as it is, and the decoder keeps it) |
| `[a\|b\|c]` | one of these, drawn evenly (nested choices and marks allowed) |
| `{$x}` `{$x:obj}` `{$x:poss}` `{$x:refl}` | an entity as the realiser refers to it: its first mention in full, then a pronoun where it is clear, else its later form; the case of each |
| `{$x:name}` `{$x:full}` `{$x:the}` `{$x:voc}` | its name, its first form, its later form, as it is addressed ("Mother"), whatever came before |
| `{$e.a}` `{$x.mother}` `{$e.season}` | an entity reached from another (the model's paths) or a field of it written as a word |
| `{walk:$x}` `{be:I}` `{have:they}` `{be:.w}` | a verb in the inherited tense, agreeing with its subject: an entity (the writer is I, the reader you, a group they), a pronoun, a kept word |
| `{#key}` `{#key\|year}` | a number: a fact's, a field's, a kept word's or a rule's (a rule's written in words stays in its words: "twenty"); a word to twelve, then digits; with `\|noun` the noun agrees ("one year", "two years", "twelve children") |
| `{#key:ord}` `{#key:about}` `{#key:words}` `{#key:digits}` | "the 212th", "some two hundred", "two hundred and twelve", "212" |
| `{rule>.w}` then `{.w}` `{.w:obj}` | a rule's word kept as `.w` with its number: written again, as it or them, a verb's subject (`the {work>.w} {be:.w} mine`: "the nets are mine") |
| `{=.x}` `{=.x:v}` | sets an attribute (a dot: it cannot shadow a fact); the record's attributes are read in reading order, so a later part's guard sees what an earlier part chose (Knuth's synthesized attributes) |
| `{+$w:she\|the woman}` `{+$s:?\|the stranger}` | a person the text brings in, here (`?`: he or she drawn): its pronoun after, its later form where the pronoun would not be clear |
| `{key:pl}` | a rule's or a fact's word in the plural ("goose" geese, "bed of the river" beds of the river) |
| `{@news private}` | a directive: the model's plan (H-04's: the event picked by salience, bound for the rest of the alternative) |
| `{~coin}` | H-04's second pass: the people's own word for a thing of theirs that has none of ours (their coin, their dish, their dance), from the model's tongue (`VgModel::tongue`); written in lowercase, kept by the decoder as it keeps a name (`VgRecord::theirs`) |
| `\n`, `\x` | a paragraph; `x` itself (`\(` an alternative that starts with a parenthesis) |

*H-04's second pass* (the section "Each people its own" below): the draw also weighs a distinctive word down by the people's uses of
it (the fatigue), keeps a `people=K` rule's few for the people, leans the genres' root by the people's seed, and draws a rule again
when a sentence it finished repeats a frame the people wrote (the frame memory).

*The draw.* Of a rule's alternatives whose guards hold, those this record has not used yet come first while there are any; one
is drawn by weight, the weight divided by 1 + 4 times (H-03: 1.5) the uses the people's earlier records made of it (`VgMemory`: a people's
records are one catalogue, so a world does not tell one template twice while others are left; a `steady` rule's weights hold, so
C-02's kinds keep their shares and only the templates within a kind are spread); `once` never twice in a record; a `keep` rule's
first draw holds. Everything is drawn from the record's own seed.

*The realiser* writes the tokens out. A person's first mention is its first form ("my mother", "Aiwya", "a stranger"); later, a
pronoun where it is clear, its later form else ("the child", "my mother", "the woman"). Clear means the entity is the last of
its gender mentioned, within this sentence or the one before, and no person was mentioned since whose gender the reader cannot
know: the peoples' names do not say it, so "Cekrast says they follow him" is not written when Cekrast is a girl and "him" her
father; the father is named again ("my husband"). A kin word, a stated gender or a pronoun already used shows a person's
gender. A paragraph starts afresh (an inscription's lines do not). Verbs by tense, person and number (a hundred and forty
irregular verbs; modals kept); numbers in words to twelve, ordinals, round numbers; plurals with their irregulars; "a" or "an" by
the next word's sound (an hour, a one-eyed goat, an 8, a unit); the possessive "'s"; the seams (one space, none before a stop,
one after a comma, no doubled stops), a capital at each sentence and a stop at the end. The grammar never writes "he", "his"
or a verb's ending for an entity, so C-02's "him mother" cannot be written, nor "a lizards" (a plural list after "a" is a load
error, and the realiser notes one that comes another way). A name keeps its marks through the realiser (H-04: `vgName`), so the
checks and the decoder know every name a text wrote; places and events are never written as "it" (`VgEntity::named`); a model can
bind an entity on its first use (`VgModel::lazy`).

*The link* (`vgLink`, at load, every error with its file and line): a rule or a fact unknown, a guard's key unknown, an entity
or a field unknown, a directive unknown, a rule no root reaches, an alternative-less rule, "a" written before a plural or a mass
list. The model's schema names its facts, entities, paths, fields and directives (`VgSchema`).

*The checks* (`vgCheck`, on every realised text): a mark left; a diagnostic of the expansion (no alternative held, an unbound
entity, an unset fact); a doubled word in a sentence ("had had" and "that that" allowed); a wrong "a" or "an"; an article or
"one" before a plural; a pronoun against its verb ("he were", "they was"); a sentence without its capital; the spacing; a length
out of its rule's range; a lexicon word of a later age than the record's year (`@lexicon`).

*The measures* (`vgMeasure`): a sentence's frame is its words with the names, the digits and the number words masked; the share
of the sentences of six words or more whose frame recurs in the sample (H-04's targets: under 15% at a people's first 50 records,
25% at 100, 35% at 200); the distinct 2- and 3-grams over the first 1,500 words; the rule use by file and line.

*C-02's kinds on it.* `shards.vg` (the kinds by tone, the world, the span and the other people, C-02's weights, and the
ordinary year's), `shards_end.vg` (the elegies, the warnings, the last recordings, the other people's, the star charts'
captions), `shards_world.vg` (the vocabulary `buildGrammar` held: the sun by the star's class, the sky, the land by the traits,
the water by the type and the span, the crops, trees and beasts, the seasons and weather; the counts) and `lexicon.vg`: H-03's
files, which H-04 replaced (their words are `core/world.vg`'s, `core/culture_*.vg`'s and `core/lexicon.vg`'s). The
shard model (`peoples/shards.cpp`) sets the world's facts (the names, the type and climate, the sky, the traits), the record's
(its tone, its share of the span, its year and its year's age, the herd), and three persons: the kin (C-02's nine, "my mother"
to "the old one", addressed "Mother"), the child (its name and now its own pronoun, from its seed; later "the child") and the
founder; a text brings in its own (the beloved, the stranger, the woman from the other town, the old one who kept the count).
C-02's draws stay in their order (the herd, the kin, the child's name), so every shard's tone is C-02's and C-04's pieces are the
same pieces. A text the same as an earlier record's of the people is drawn again (up to eight times, the memory untouched by the
draws thrown away): a world never repeats a text whole. The texts read as C-02's with their seams mended: "My mother says her mother swam in it", "Iapya stood in the
doorway and shouted at the sun until my sister brought the child in", "We cut the 4234th mark", "That was one year ago", one
crop and one herd animal all through a text, "the nets are mine now". A people's texts are one catalogue (`shardTextsOf`, the
last eight kept), built in index order; `shardOf` reads it. A trade of iron waits for the medieval age.

*Numbers.* 4 files, 66 rules, 391 alternatives, 35 kinds; read and linked in about a millisecond. Over the 48 peoples of the test
worlds, the age worlds and the forty nearest home: 5,553 records, 3,479 texts and 444 star charts' captions, 113,377 words, every
check at zero; a text 3.9 us on average (18 at most), a people's texts 0.5 ms. C-02's repetition stays C-02's (its kinds have
one to five templates each, their shares kept): 20% of the longer sentences on a repeated frame at a people's first 50 texts, 37% at
100; H-04's genres are written to the targets.

| Mode | What |
| --- | --- |
| `grammar` | the grammar read from `src/peoples/grammar/` (`dir D` another, `embedded` the compiled-in), its errors; every people's texts of the test worlds, the age worlds and the 40 nearest home (`near N`) through every check, each world's line (records, texts, words, the time, its repeated frames), the failures with their texts, the measures and the most repeated frames, the rule use (the most drawn and the never drawn by file and line, the kinds told), the cost; every text in `shots/tests/grammar_texts.txt`, two a people on the screen (`print N`, `print all`) |
| `grammar check` | the unit's H-03 checks alone (the engine's cases, the files compiled in, the test worlds' texts) |

## The records (H-04)

H-04 writes each people's records from its history (`peoples/genres.*`, the genres on `src/peoples/grammar/`). T-04 fixed how
many there are, the year of each, the medium it was made in and where it lies; the engine above draws each record's genre and text
from the record model, and C-02's kinds are gone from the text shards (the star charts' captions and the music keep C-02's draws).

*The genres by age.* The root (`core/records.vg`, `record (steady)`) picks a genre by the age of the record's year and its medium, weighed
by what the history offers at its date: `pair` (two of a family alive, one of them where the record lies: a letter), `writer`
(someone alive to write), `dead` (a death in the three years before), `will` (someone old who dies within six), `notable` (a death
the chronicle told), `event`, `battle`, `built`, `invented` and `sign` (such an event in the years before), `holyplace`, `railway`,
`household`, `figure`. The weights hold across a people's records (`steady`): what a people wrote most stays so.

| Age | Genres (the medium) |
| --- | --- |
| prehistoric | the tally (bone, stone), the moon count (bone), the hunt (painted, stone, bone), the herd, the dead, the song, the picture of an event, the boundary stone |
| medieval | the annal (tablet, scroll), the chronicle (codex, scroll), the letter, the decree, the prayer, the account, the will, the epitaph (stone), the ballad, the sermon, the pilgrim's guide, the herbal, the inscription of a work (stone) |
| industrial | the newspaper, the report, the patent, the telegram (when the telegraph is invented), the rules, the census, the timetable (with a railway), the obituary (printed); the letter, the diary, the ledger, the chronicle (codex); the voice, the sermon, the ballad (wax cylinder); the epitaph |
| modern | the newspaper, the report, the manual, the minutes, the propaganda, the paper, the obituary (printed); the lyrics, the broadcast, the weather (disc, reel); the diary, the station's log, the messages, the voice (reel, drive, disc) |
| futuristic | the machine's log, the messages, the medical log, the mission record, the diary, the machine's notes (drive); the personal archive, the charter, the law, the memorial, the chronicle, the lesson, the vote (crystal, drive) |
| far future | the untranslatable, the instructions, the mathematics, the whoever-comes, the machine's log (crystal, the thing that hums); the reflection, the archive, the memorial, the chronicle, the lesson (crystal) |
| the collapse | the warning (from its first year, `stage` 0), the denial (0 to 1), the orders (1 on), the inventory and the whoever-comes (2 on), the emergency broadcast (modern and futuristic, 1 on), each on its age's media; the last recording (`last=1`, where the history ended) |

81 alternatives at the root, 63 genres; each genre's `words=` range is what it writes, from a tablet's line of seven words to a
chronicle of 257 (the letters, the diaries and the archives run to 140-160).

*The model* (`RecordModel`, a `VgModel`) reads the world (C-02's facts with the seasons and the coast), the people (its last age,
faith, rite, holy place, economies, values, sacred number, monuments, the orientation, its centre, how it keeps its dead, the other
people's fate, the cause of its collapse and its speed), the record (its age, medium, year, season, month and day; the year's
government, era (marked), ruler and reign, harvest, war, sickness and famine, the collapse's stage and ending, the contact, the
tone, the share of the span; the inventions at the year as `inv.<key>`, forty-seven of them, and the institutions by kind with their
names), the genome's thirty traits and the plan's places. Its entities are the writer (`$me`), the reader (`$you`), the subject
(`$x`), the event told (`$e`), the ruler, the home, the seat, the founder, a kin, a child, a spouse, the last person, a figure
(lazily bound on their first use); its paths reach an event's persons and places (`$e.a`, `$e.place2`), a person's kin and home;
its fields are an event's (`kind`, `season`, `war`, `sickness`, `wave`, `work`, `won` for the record's own side, `out`: a way's kind),
a person's (`age`, `born`, `died`, `trade`, `title`, `role`, `kin`, `children`) and a place's (`rank`, `coast`, `river`, `founded`, `stands`).

*The directives* are the model's plan, written in the genres where a record needs the history:

| Directive | What |
| --- | --- |
| `@cast letter\|diary\|archive\|writer\|will\|dead\|epitaph\|notable\|obituary\|household\|census\|last` | binds the record's persons: a letter's writer and reader (two of a family alive, the reader any with kin, those at home five times as likely; parents and children weigh 1.2, siblings 1, a spouse 0.5, four times as much when they live apart), a diary's writer, the dead an epitaph names, the old one who leaves a will |
| `@news private\|public\|any\|near <register>` | the most salient event of the record's year and the four before that the record has not told, in the register's rule for its kind; the register's `.none` when there is none |
| `@entries <register> n` | a year's events (the chronicle's year, else the record's), the most salient n, one of a kind, in their seasons' order |
| `@years n rule` | a chronicle: up to n years with something in them, within the era and sixty years, the empty years between told by `rule.gap`, each year's facts and ruler bound |
| `@death prefix`, `@deed prefix`, `@life prefix n`, `@event kinds register span` | a death by its cause or how it came (old, young, illness), a person's deed, a life's events in order (an archive), an event of the kinds within the span |

*Salience*: the event's weight times its nearness (the years since) times its place (1.6 at the record's home) times the writer's stake
(three for a private event of the writer's family); an event told by another record counts half, or 1.3 when it is a great one told in
another register (the annal, the letter and the song of one event: perspectives); a second event of a kind in one record a quarter;
nothing is told twice in a record. A register is the way a kind of record tells an event: `annal` (the chronicle's line), `letter`
(the family's news and the people's, past tense), `news` (a headline and its body), `song` (verses), `picture` (a painted event),
`sermon` (the lesson drawn from it) and `log` (the facts, short).

*The register kept.* Most records are the ordinary life; the collapse's genres come only in its years (`stage`), the catastrophe late,
so a people's last century is told from many sides (the warning nobody heeded, the denial, the orders, the inventory, the emergency,
the whoever-comes, the last recording) and every earlier record is of its time. On the test worlds 93% and 95% of the texts are of the
ordinary genres, none of the collapse's before it began.

*The persons' forms.* Close kin as "my mother", other kin as "my sister Utsior", the reader's as "your father"; a ruler with the title,
each word capitalised ("Queen Vroghart", "High Priestess Keyklan"); "Gheigon the farmer" in the private genres; a letter greets by the
given name alone (the second word of a name is a surname unless it is a regnal numeral). Places and events are never "it" (`named`).

*Names.* Every place and figure is the history's at the record's year: places founded by then, persons born by then (by the whole name
or the given name), the lore's names; the unit checks every name the records write (`recordNameKnown`).

*Anachronism.* `@lexicon` lists words by age and by invention (`@lexicon inv.railway train trains railway`): a word whose fact is not
set at the record's year fails the check ("the computer" in a prehistoric text was "the ship"; "broadcast" waits for the radio, "steam"
for the steam engine; the history's coal-out signs say "the engines stopped").

*Repetition.* A slot a people draws a hundred times needs many frames, so the parts compose within a sentence (`[a|b|c]` inside a line,
a part with the tails that fit it); the catalogue dryness is 4 (an alternative's weight over 1 + 4 times its uses by the people's
earlier records). A text the same as an earlier record's is drawn again (up to eight times).

*Built on a thread.* A people's texts with its history and plan take about a third of a second (`shardTextsOf`); the radar and the
decoder do not wait: `shardTextsIfReady` returns them when built and starts a thread when not (`shardTextsPrefetch`); the radar asks
again every quarter second, the decoder builds the world under the cursor as it moves down its list.

*The language by roots* (C-06 re-tuned): the computer learns roots, not forms: `rootOf` finds a word's root among the words the
records wrote (the realiser's irregular forms, -s, -es, -ies, -ed, -ing), and the tongue gives each root a word and three endings of
its own (`endS`, `endPast`, `endIng`), so "walked" reads once "walk" is known. `languageShare(held, records)` is 30% of the uses with one
shard and all at the people's full count (`languageFull`: 12% of its records, 10 to 24); the names read from the start are every name
the records wrote (a regnal numeral left out, an ordinal in digits read as a numeral). On the test worlds 38/45/54/75/100% of the words
read with 1/3/5/10/16 shards (felisian, 698 roots) and 39/48/60/89/100% with 1/3/5/10/12 (desert, 535).

*The decoder's pages.* A text is laid out from the words as they read at this share, in pages of the screen's height; a new page
starts at a paragraph, or at a sentence within the last forty words; `PAGE n/m` at the top right. The voice speaks the page shown (the
opening first, `speakShardPage`), the rest on request: `UP`/`DOWN` (`PGUP`/`PGDN`) turn the pages, `ENTER` speaks the page (or stops
the voice, or ends a first reading's wait). `07-game-flow-and-controls.md` has the keys. The radar's voice reads a record's opening
(`shardOpening`, sixty words at most).

*Numbers.* 28 files, 914 rules, 3,640 alternatives, 141 lexicon words, the largest file 14.6 KB. Over the 48 peoples of the test worlds,
the age worlds and the forty nearest home: 5,553 records, 3,479 texts (prehistoric 872, medieval 1,324, industrial 512, modern 389,
futuristic 276, far future 106) and 444 captions, 216,497 words (62 a text; C-02's 33), every check at zero and every name the
history's (9,166 of them); a text 24 us on average (342 at most), a people's texts with its history 0.32 s. The repeated frames
(sentences of six words or more, names and numbers masked, the genres' formulas left out) at a people's first 50 texts 10.6% (91.8%
of the sentences written once), at 100 34.6%, over a people's all 15.8%, distinct 2- and 3-grams 0.74 and 0.89; C-02's texts of the
same peoples 20.4% (84.7%), 36.4%, 27.6%, 0.68 and 0.83. 59 events told by two genres or more. Fifty records of one people per age are
printed for reading by `vesperis_test grammar ages` (`shots/tests/records_<age>.txt`). The second pass (below, "Each people its own")
took these to 0.2%, 0.9% and 0.6% with every people's records its own.

| Mode | What |
| --- | --- |
| `grammar ages` | with the run, fifty records of one people of each age, each with its year, genre, medium, words, writer and reader (`shots/tests/records_<age>.txt`) |
| `grammar compare <file>` | with the run, C-02's texts of the same peoples (H-03's `grammar print all`, saved) measured the same way |

## Each people its own (H-04's second pass)

H-04's records told each people's history, but in one voice: a held-out name told its people 53% of the time over 48 peoples (C-02's
one generator in three styles), 66% of a people's longer sentences were on a frame another people also wrote, the same few nouns
came back on every world ("jars", "the well", "the chief"), and a people's hundredth text still repeated a third of its frames. The
second pass (2026-10-10, the user's ask: 5% at a hundred texts, nothing repeating, every civilisation unique) changed four things.

*A tongue for every people* (`peoples/tongue.*`; after Martin O'Leary's naming languages and Mark Rosenfelder's kit). A people's seed
draws its consonants from one of twenty-four sets (a small island set, a Caucasian one rich in affricates, a savanna set with its
nasals, a hissing one...) with a few dropped and added, its vowels from fourteen sets (three to eight, long ones, diphthongs), one to
three syllable shapes (`CVC`, `C?L?VC`, `S?CVF`...), the clusters it allows (a stop or a fricative before l or r, s before a stop), the
consonants that may end a syllable, its spelling (each sound's letters among its ways: sh, sch, x or sj; kh, ch or x; never two sounds
alike), a doubling between vowels in some tongues, the morphemes of thirty-three meanings (town, river, hill, holy, old, the people...)
and whether they come before the root or after it, joined, hyphened or with an apostrophe, a pool of thirty to fifty name roots that
come back in its names, a given name's ending by sex where the tongue marks it, and its families' affix. Every name is the tongue's
(`tongueName`): the lore's (the people, the god, the river, the mountain, the sea, the two cities, the founder, the moon, the star, the
festival), the history's persons, families and places (a place by its rank: a hamlet's, a capital's), the graves'; the decoder's
words for what is not learnt yet (`tongueWord`) and the people's own words (`tongueTerm`, `{~coin}`) too, and the voice speaks the
tongue's sounds and leans with its hardness. A name that reads as a rude or famous word of ours, or stutters a syllable thrice, is
drawn again. A held-out name told its people 74% of the time over the 60 peoples nearest home (C-02's: 51%; chance 1.7%), 83% over 24
(`vesperis_test tongues [n]` prints them side by side: "Pirtima, Pirstousi, Stara", "Zhida, Tsatsu, Bubita", "Moopuu, Suumu, Kuupo").

*The people's things in the texts.* The culture's pools (`core/culture_goods.vg`, `_ways.vg`, `_speech.vg`) are lists from which each people
keeps its own few (`people=K`): its three crops of twenty-two by its climate, two herds, its fruit, roots, fish, birds and pests, its
food, drink and vessels, where its water came from and what it burnt, its trees (named its own way by the world's flora families),
its materials, crafts, tools, garments, instruments and pastimes, its feasts' customs, the hours of its day, its measures and its coin
(a word of its tongue), its proverbs (eight of forty-three), virtues and vices, blessings, curses, oaths and cries, how it says a life
is, where its dead went and are, how the years pass, its signs of luck, what it calls a child and a fool, and how it speaks of its
rulers. The ruins are in the texts too: the genome read through the record's age gives the facts `plan`, `roof`, `walls`, `huts`,
`ornament`, `motif`, `tops`, `accent`, `land`, `streets`, `public`, `edge`, `links`, `light` and `towers`, so a text says "under the
domed roofs", "the spirals over the doors", "houses on stilts" where the explorer finds them. A place has a trade (`$p.ptrade`: what its
founding and its water made it), a landmark of its own (`$p.landmark`) and quarters (`$p.quarter`, the record's); a ruler an epithet
(`$x.epithet`: "the Builder" for three works in the reign, "the Conqueror", "the Lawgiver", "the Old" for forty years, "the Child", a
plain one by the hash for some); a person a trait. A government's ruler has the people's own title (`historyRulerTitle`: a headman, a
war-leader, an archon, an oracle, a first citizen; in nearly half the monarchies a word of the tongue, kept as a name), and the wars',
sicknesses' and eras' names come from larger tables by the people's seed (a place's or the river's name in some: "the Kokotkas sweat").

*The everyday* (the history). Fourteen public kinds of the quiet years, each from a stream of its own a year (`Builder::everyday`), so
H-02's history stands as it was: a fair (what it was for, how many came), a fire (what burnt, the houses), a storm (wind, hail, snow,
dust, the sea, lightning, rain), the fields' or the herds' plague, a wonder told about (a calf with two heads, a great fish, a hoard, a
red rain, a bell that rang by itself, an older people's ruin found), games (their kind, the winner), a law (on measures, a tax, the
water, a curfew, the coin, dogs...), a trial (the charge, the accused, the verdict), envoys, a works' disaster (a bridge, a mine, a ship,
a train, a dam, a reactor, a habitat), a price that rose or fell, the herds and the game (late, early, none, a great one taken, one that
killed), a show (a play, a song, pictures, a film) and the ruler's visit; and seven private kinds of the families followed, from each
person's own stream after the years are drawn (`Builder::family`): an illness got over, a hurt, a child apprenticed, the house built,
sold, burnt or mended, a quarrel with a neighbour, a journey and back, a betrothal the year before a wedding. None of them harms
anyone or makes a person; all are low in salience, so they fill the years the great events leave empty (75,845 of 184,665 events on
the test worlds' peoples). `$e.what` gives an everyday event's word, `$e.verdict` a trial's. A reader is never told their own news.

*The engine.* Four additions (the format above): the frame memory (a people's records keep the frame of every sentence they wrote; a
rule that writes whole sentences and holds no directive is drawn again, up to six times, while a sentence it finished repeats one, its
draws and their marks undone), the fatigue (a distinctive word, in 24 alternatives or fewer, weighs its alternatives down by the
people's uses of it), a people's own few (`people=K`, `share=F`) and their own words (`{~key}`). A weight on every alternative by the
people's seed (a dialect) was tried and left only on the genres' root: it changed which alternatives a people used first, not which it
used, so it left the sharing between peoples where it was (65.4% against 66.0%) and raised the repetition within one (4.8% to 7.3%);
on the root it gives each people its own mix of genres. `@digest <scope> <register> <n>` tells a record's several news items in the
order they happened (a war declared before its first battle).

*The measures.* Beside H-04's, the harness prints the share of a people's longer sentences (its first hundred texts) on a frame another
people also wrote, the frames the most peoples wrote, and per genre the share between peoples and within one; `grammar late N` adds the
N nearest peoples of the modern age and later (a hundred texts and more each) to the sample, so the measure at a hundred texts reads
seventeen peoples, not six; `grammar dir D out F` lets several writers work side by side on copies of the tree.

*The writing.* The genres were rewritten by seven writers side by side (the prehistoric genres; the medieval; the letters and diaries
with the letter register; the industrial documents with the newspaper's register; the modern genres with the log's; the futuristic and
the far-future; the annals, the songs and the collapse), each owning its files and a rule prefix (`pre.`, `med.`, `let.`, `ind.`,
`mod.`, `fut.`, `ann.`). Their sentences are built from parts in the people's own words: a tally counts in the people's own marks and
goods, a hunt is of its game with its weapons by its water, a letter's everyday lines (seventeen topics: the children, the spouse, the
work by the writer's trade, the weather, the sky, the house, the market, the table, health, faith, memories, sayings, the neighbours,
gifts, the town, the feasts, the other people) and its news in several voices for every kind, the private ones included; a newspaper's
masthead, motto and price in the people's coin, its columns composed, its front page in the order things happened (`@digest`); a
broadcast, a manual for the people's own machines and crafts, minutes of the people's own quarrels; a machine's log with each people's
tag, a charter by what the people valued, the lessons of its own history, far-future instructions and mathematics that differ in their
whole conception; annals dated each people's way (its herds and harvests, its feasts, its months in its tongue, its rulers' epithets),
songs with each people's refrains, a collapse told in each world's colour, motifs, herds and burial custom.

*The numbers.* 94 files, 2,864 rules, 16,808 alternatives (H-04: 28, 914, 3,640), every file under 16 KB. Over the 48 peoples H-04 measured
(5,553 records, 3,479 texts and 444 captions, 287,211 words, 83 a text): every check at zero and every one of 9,546 names the history's;
the repeated frames 0.2% at a people's first 50 texts (99.9% of the sentences written once), 0.9% at 100, 0.6% over all (H-04: 10.6%,
34.6%, 15.8%), distinct 2/3-grams 0.79 and 0.94; 17.1% of a people's sentences on a frame another people wrote (65.9%). With the twenty
nearest peoples of the modern age and later (`grammar late 20`: 68 peoples, 8,828 records, 5,567 texts, 17 at a hundred texts): 0.3%,
1.6%, 0.9%, sharing 22.2%, every one of 14,868 names the history's. The genres are under 3% within one people (most at 0) and between 2%
and 45% between peoples (the far future's instructions and the charters the most, the letters, the diaries, the annals, the wills and the
herbals under 13%); the frame the most peoples wrote is in 13 of the 68. A text 67 us (501 at most), a people's texts with its history
0.35 s; the engine draws a rule again about 1,800 times over the 68 peoples.

## The three categories by age (H-05)

H-05 (2026-10-10) fills T-04's catalogue in its three categories, each in the way of the age the record was made in: a modern people's
records reach back into its medieval and industrial years, so its music holds a hymn from a codex, a march cut in wax and a song pressed
on a disc, and its charts an astrolabe's plate beside a survey plate.

*The counts* (`peoples/records.*`). C-02's fifty keep their categories (C-04's twelve to eighteen pieces, C-10's three to five charts).
Past them a record is music or a chart by the age of its year (`recordIsMusic`, `recordChartIndex`): the age's share of its records
(`recordMusicShare`: 36% prehistoric, 22% medieval, 25% industrial, 32% modern, 24% futuristic, 26% far future; `recordChartShare`: 9%,
8%, 7%, 6%, 12%, 15%), leaned by the people's own fifty (its pieces against fifteen, its charts against four), taken within each age by a
hash of each record so the count is exact; the charts are numbered C-10's first, then by the index (`recordChartRecords` gives each chart
its record, so a chart knows its age). Every `Shard` carries `recordAge` (the age of its year) and `medium` (what it was made on). On the
age worlds: the Witniikh 86 records (51 texts, 26 music, 9 charts), the Ta-Zhozo 101 (74, 21, 6), the Aachidblukh 135 (97, 30, 8:
medieval 53/16/3 and industrial 44/14/5), the Nulo 143 (86, 44, 13 over three ages), the Irrur 175 (110, 52, 13), the Glaak 178 (101,
48, 29). Over the 129 peoples nearest home by the age each reached: a prehistoric people's records 60% texts, 32% music, 8.5% charts; a
far-future people's 57%, 29%, 14% (`vesperis_test decoder counts`).

*What a record was made on, heard* (`game/audio_medium.cpp`, `AudioState::medium`). A record made on a medium with a sound of its own
(`recordMediumSounds`: a wax cylinder, a reel, a disc, a drive, a crystal, something that hums) is the recording itself, heard through
its medium on the decoder: a wax cylinder through a band of 320 Hz to 2.9 kHz and a horn's hump at 1.25 kHz, a little saturated, its
surface hissing and swishing round at the cylinder's turn (2.2 a second), its needle crackling (eight ticks a second, a pop now and
then) and its pitch wavering with the turn; a disc through a wider band with a quiet surface, a pop now and then and the turntable's
slow wow; a reel with its hiss, its wow and its capstan's flutter, a little saturation and a dropout where the oxide wore; a drive
clean but rotted (every six to twenty seconds a scrap read again two to five times, a moment gone coarse at a quarter of the rate, or a
gap); a crystal with a faint second reading a few milliseconds behind; something that hums with a drone of 55 Hz under it and the voice
riding on it. The level of the content is kept (the speech's rms within a tenth either way); the surface in the speech's pauses is
0.012-0.016 on a cylinder, 0.002-0.003 on a disc, 0.005 on a reel, 0.0005 on a drive, 0.013 the hum (`vesperis_test decoder wav`). The
radar's programmes are not coloured by it (the air has its own, C-16).

*Read or played by the ship.* A record with no sound of its own (a carved bone, a stone, a painted wall, a tablet, a scroll, a codex, an
engraved plate, a glass plate, printed pages) is the ship's rendering: a text is read in the people's tongue in the people's voice as the
computer rebuilds it, flatter (`voiceRead`: the melody and the tones a third of the voice's, an even pace a little slower, no tremor,
no second tone, little breath; the vowels' pitch spread 30% of the voice's own), and a piece's notation (a bone flute's holes, a scroll's
or a codex's marks, printed music) is played dry, as written.

*On the decoder* (`game/medium_draw.cpp`). Under a shard's date its medium is named, the object alone (R-414, the user: "just say what
kind of medium this is"): `A CLAY TABLET`, `A WAX CYLINDER`, `A REEL`, `SOMETHING THAT HUMS`; a piece on a medium without a sound of its
own as what it is, `A BONE FLUTE`, `NOTATION ON A SCROLL`, `NOTATION IN A CODEX`, `PRINTED MUSIC` (H-05 named who was heard, `A CLAY
TABLET, READ BY THE SHIP`, `A WAX CYLINDER: A ROUGH VOICE`, `A RECORDING IN WAX, THROUGH ITS HORN`: the sound says it); at the right of
the date the object is drawn (40 x 26 of the HUD's 320 x 200): a bone with its cuts (a
flute's holes, a sky's dots), a slab with its signs, a patch of painted rock, a clay tablet's wedges, a scroll on its rods, an open codex
with its red initial, a brass plate, a wax cylinder on its mandrel with its grooves turning by while it plays, a glass plate, printed
pages, two spools turning, a disc spinning with its label, a drive blinking, a crystal glowing, and an ovoid whose rings pulse out.

*The music by age* (`peoples/music.*`). A people's music changes with its ages and stays its own. `traditionOf` draws C-04's core as before, to the bit (the scale and its
tuning, the tonic, the cycle, the beat and its drift, the people's own two or three timbres, its drum, drone, second voice and
ornament), then from a stream of its own what holds through every age (`Tradition::swing`, a lilt of 8-28% on the second of two
subdivisions for 45% of the peoples whose beat divides in two; `density`; `refrain`, the seed of the phrase every chant and chorus of
theirs answers with; `octaves`, a chorus that sings in octaves for a third of them; `vowels`, the vowels its singers sing, the first
letters of its own tongue's vowels) and, for each of the six ages, an `Ensemble` from a stream of its own (`Tradition::ages`: its
melodic parts by role, `MR_LEAD` ... `MR_SWARM`, its kit by role, `KR_LOW` ... `KR_SUB`, the age's tempo against the tradition's, the
drift as played: none from a machine). The people's own C-04 timbres lean every age's draw (a people of reeds gets free reeds in its
industrial years more often). The singers are the people's speakers: `voiceOf`'s tract, breath, roughness and nasal murmur.

A piece is played on the ensemble of its record's age (`Shard::recordAge`, `Piece::age`: a modern people's medieval hymn is
medieval) in a form chosen by the shard's tone and the age (`PIECE_FORM_NAMES`, forty), in that age's texture, its length set by the
clock (about a minute; a jingle a quarter of that, a field up to two):

* *The prehistoric* (voices, a bone flute that breathes and wavers, a musical bow for a quarter of the peoples, a bullroarer for three
  in ten; the people's own skin drum, low and high, a tuned log drum's two notes, rattles, sticks or clapping hands): a chant (a
  caller's phrase, varied, and the chorus's refrain, the same in every chant of the people, over the drums' cycle), a work song (the
  stroke on every group), a healing song (over the bullroarer or the chorus's hum), a ring dance (a cell of one cycle round and round,
  20-38% faster by the end, a stamp and a shout), a hunting song (the flute alone, calls high and long, a soft tick), a keening
  (voices sliding down onto each note, the chorus humming the tonic, a slow heartbeat or none), a lullaby (rocking between two notes,
  slowing at the end).
* *The medieval*: C-04's statement, variation, answer, return and close on C-04's own timbres, plus a choir that sings a hymn or a
  procession in organum (a second line at the resting degree's inversion below), a voice for the love songs, ballads, lullabies and
  laments, a drone (a bag's reed or a wheel's bow) for the peoples that drone, a bell at a hymn's phrase ends, a tabor and jingles under
  the dances; a short form comes round again.
* *The industrial*: one of five bands (a brass band; strings and keys; an orchestra of a woodwind lead, a horn, a section of strings and
  a bowed bass; a barrel organ that plays tune, chords, bass, drum and cymbal itself; free reeds in a box of bellows), plus an organ
  for the hymns, a music box, a singer and keys. Homophony: chords stacked from the people's own scale (`Bld::chord`: the root, the
  degree nearest a third above it within 230-470 cents, the one nearest a fifth: in a 22-step scale a narrow fifth of 655 cents) in
  progressions that open on the resting degree and close on the tonic, the people's favourite turns (`fav`, one draw a people) six
  times in ten; a melody of two phrases a strain, its notes on the groups' first beats moved onto the chord sounding; marches (the bass
  on the groups' first beats, root and fifth by turns, the chords on the other beats, the side drum's roll into the next strain), turning
  dances, anthems, an overture, a funeral march, parlour songs (the chord broken in subdivisions), hymns on the organ, and the music box
  (high, its spring running down: the last third slows by half, and half the time it stops before the tune is done).
* *The modern* (a drum machine for six peoples in ten, else a kit; a synth bass or an electric bass; chords on electric keys, a pad,
  a strummed electric string or an electric organ, played held, on every beat, between the beats or pushed ahead of the groups; the
  lead a singer, a synthesiser or an overdriven electric string): songs of intro, verse, chorus, verse, chorus, bridge (two times in
  three), chorus, outro over a groove read from the people's cycle (the kick on its groups, the back on the group nearest its middle,
  the hats on the subdivisions, a crash at a chorus, a fill into the next section); dance records that build, drop and break (the kick
  on every beat, the riff); ballads, requiems and lullabies at half time; anthems, a broadcast's theme, a jingle of fifteen to twenty-five
  seconds, protest songs.
* *The futuristic* (a synthetic voice, its samples held on a coarse clock and a ring under it, or a pure tone; struck glass, a pluck or
  struck metal for the cells; a swarm of ten detuned tones swelling on their own, or a choir of synthetic voices; a pad, a sub tone;
  clicks and a sub pulse): a process (a cell of four to seven notes looped by one voice and, a subdivision longer each time, by another,
  so the two drift apart and meet again; or built note by note and taken apart; the voice entering halfway), a signal (two notes
  alternating over clicks), a synthetic song over an arpeggiator, swarms, drifts and elegies (chords of three to five tones, some
  half-way between the people's own notes, each held five to eleven seconds and gliding into the next).
* *The far future*: a field of sound rather than a piece (two fields of long tones, full, hollow or bell-like, beating by a few tenths of
  a hertz; five to nine tones of the scale over three periods, each coming and going on its own; a deep tone or a hum under them;
  glints or a far choir over them). A field turns (the tones entering in turn, low to high, a slow wheel), holds a standing tone (one
  chord, a tone moved every fifteen to twenty-five seconds) or rings with bells. `cycleLine` says "NO BEAT: 11 TONES OVER 5 OCTAVES".

*The player* (`game/audio_music.cpp`, `AudioSynth::musicTrigger/musicVoice/musicSample`, moved out of audio.cpp): twenty-eight voices of
up to thirty-two partials on a 4,096-step sine table; each family's partials laid out at the note's start (a sung vowel's harmonics
under its three formants scaled by the singers' tract; brass whose upper partials come in as the note swells, renormalised so the
loudness is the envelope's; struck strings stiff, two a key, dying high first; an organ's stops and chiff, a barrel organ's tremolo; a
music box's tines at 1 : 6.3 : 17.6; an electric string overdriven then rolled off; two-operator electric keys and glass; a synth's saw
or square partials under a low-pass that falls and resonates; pads; swarms; fields) and moved at a control rate of 32 samples; the kits'
pieces from sines, resonators and noise. A limiter on the music's bus (instant down, a quarter of a second back up) keeps stacked
chords under 0.82. The pieces need at most 22 voices at once; the dearest (a modern song) costs 315 ms of one core a minute (0.5%).

*Levels* (`vesperis_test music ages wav`, the game's master level): every age's pieces peak under 0.8 with an rms of 0.061-0.152 (the
speech sits at 0.13-0.15); `music ages frames` puts a piece of each age on the decoder (`shots/tests/music_decoder_<age>.png`: the tune
white and three rows, second voices and choruses green, chords and swarms dark cyan in one row, bass and drones dim amber, the kit's
pieces in rows of ticks below; the far future without the cycle's grid).

*The harness*: `music <sx> <sz> <body> [index] [wav] [age=N] [tone=N] [second]` prints the core, the six ensembles and every piece of the
people's records by year with its age, form, length and instruments, then one as notation (a field as each tone's spans in seconds);
`age=` and `tone=` play the records as if of that age or tone. `music ages [wav|frames]`: each age world's core and ensemble, three
pieces of its age with their levels (`shots/tests/music_age_<age>_<form>.wav`), the modern world's pieces of three ages
(`music_people_<age>.wav`), two peoples of one age (`music_pair_<age>_<people>.wav`), the synth's cost.

*The star charts by age* (`peoples/charts.*`, `game/chart_draw.cpp`). A chart is drawn in the way of its record's age and medium (`chartStyleOf(age, medium)`, `StarChart::style`, peoples/charts.*): the
ideas note's "from a painted ceiling to a navigation chart of routes". The figures stay one set over the people's sky (C-10's lesson:
a property of the sky, not of a chart), so a people's charts of different ages show the same figures in different hands.

| Age | Medium | The way (`CHS_*`) | What it shows |
|---|---|---|---|
| prehistoric | painted wall | a cave's ceiling | ochre and soot on lamp-lit rock with its cracks and older paint; the bright stars red daubs, the middling chalk, the faint soot; the figures' animals painted large under them; the moon's count in a row of soot strokes (a white disc at the full moon); the marked star ringed in soot with a hand blown in red beside it |
| prehistoric | carved bone | a carved bone | a long bone with its knobs, the brightest 45 scratched on it (crosses and points), the animals scratched small, the moon's nights notched along its lower edge (worn notches where the people counted none), the mark ringed |
| medieval | painted wall | a painted vault | a lunette of ultramarine over a red border dotted with gold, the stars gold (eight-pointed for the bright), the mark silver, the figures' lines in thin gold, their creatures beside their names |
| medieval | engraved plate | an astrolabe's plate | a brass disc in its graduated limb (five-degree ticks, longer every fifteen) under a throne, their equator, tropics (at the world's own tilt) and the ecliptic engraved, the brightest sixteen as the rete's pointers, the wanderers' ways dotted, the rule across the face |
| medieval | codex | a page of a codex | parchment with its gutter, stains and deckle, a double rule in red, an initial in red and gold (the people's first letter), the stars in brown ink, the sun's road as a red band and the wanderers' ways on it, the creatures in red beside their names |
| industrial | printed pages | an engraved atlas | paper in a double border, the grid of their coordinates (their pole the world's spin axis, their hour circles from its own meridian) numbered in their units (a circle of 240-600 of their degrees, a day of 10-36 hours, the people's own), the brightest 24 numbered in their catalogue's order, the figures dotted, the plate's number (TAB. n) |
| industrial | glass plate | a photograph on glass | a negative: dark blurs on the pale emulsion, sized by brightness, the reseau, a corner chipped off, the mark ringed in ink by hand, the plate's number on a paper label; the figures' lines only as the computer's faint reading |
| modern | glass, printed pages, a drive | a survey | the stars sized by magnitude in their own steps (a magnitude 2-4 times the next: `magStep`), the key in the corner, the field's number; the peoples heard by radio ringed with the radio's arcs (the lore's marks of `how` 2 within the field); on glass a negative, printed black on white with a ticked border, on a drive a dark screen with the stars in their colours |
| futuristic | crystal, drive | a navigation chart | a map of their neighbourhood from above (the galaxy's heart up, rings at 2, 5 and 10 ly, CHART_MAP_LY 12), a star's height a tick up or down; the routes from their star to the star the chart marks, to the worlds of the peoples they knew and to the nearest stars, by a star on the way past 7 ly, flown (solid) or only meant (dashed), a far end an arrow at the map's edge with its distance |
| far future | crystal, something that hums | a chart of far places | a violet dark with rings round the mark out of true, their own sky faint, three to six places where nothing is (rings with a sign of their own, `sign` stars thousands to 150,000 ly off, no star), each joined to the mark |

*The names* (`chartFigureNames`, `figureName`): a people whose records began in the oldest age names animals (28: the elk, the
crane, the salmon, the adder...; plain, with a word of colour or of what it does, its calf or tracks or skull, a pair or three, a
person's: "the lame reindeer", "the fox's skull", "the two bison", "the drummer's swan"), every other people creatures, heroes and
things (51, C-10's twenty-four among them; "the drowned king", "the smith's hammer", "the golden harp", "the two weavers"), no two
figures of a people on one word; each name its glyph (`CHART_GLYPH_ROWS`, seven by six: twelve animals, twelve creatures and
things). A navigation chart's routes are named from their ways ("the slow crossing", "the dark run"), the far future's places of
nothing from theirs ("the cold door", "the nameless field").

*What each way reads*: the moon's count is the calendar's (`Lore::monthDays` with a month, `months` without days; none without a moon to
count by: C-12's), the ecliptic the world's orbit (a moon's: its planet's), the tropics its tilt, the wanderers the system's other planets
seen from it over 0.6 of a synodic period round the time each stands nearest the field's middle (their loops where an outer one turns back:
`bodyPos`), the atlas's grid the world's spin axis and `ref0`, the survey's ringed peoples the lore's heard marks.

*The far future's marks* (`farMarkOf`, `ChartMark::how` 3): a chart of the far future marks a real star thousands of light years off,
toward the galaxy's heart (30-92% of the way), in a cluster (an open one 200-1,500 ly off, else the nearest globular), out of the disc
(250-600 ly up or down) or along it (1,500-6,000 ly), found by a cube of 2-7 sectors round the point and cached a people; the sky's charts
keep C-10's order among themselves. Enter targets it like any mark ("finds much later": the galaxy chart's jumps reach it). A chart
drawn before the people's radio (its record's year before `PeopleAge::radioYear`) marks no voice: a near people that transmits is a world
among their bright stars to it (`how` 1), a far one nothing (C-10's marks were the people's, whatever the chart's year: a medieval
astrolabe could mark "the star the voices come from"); near home 15 of 341 charts now mark a people heard (33 before).

*The caption* (`core/charts.vg`) is the way's: an opening of its object ("On the cave's roof, over the fire:", "Turn the rete and the stars
rise:", "Field 178 of the survey of Techaenoe:", "Places where nothing is:"), the figures, what the way adds (the moon's count, the
wanderers, the numbers, the voices heard, the routes flown and meant), the mark; the chart's facts in the schema: `chartstyle`, `tally`,
`tallykind`, `wanderers`, `numbered`, `plate`, `maglimit`, `heard`, `routes`, `reached`, `planned`, `farly`, `farkind`, `signs` with the
record's `age` and `medium`; the people's own words (`{~key}`) through a model of the chart's own.

*On the ship*: the decoder's chart page (game/chart_draw.cpp, moved from shards_screen.cpp) draws the object in the chart's box (x 4-164,
y 31-176), the stars and lines and names over it as a first reading reveals them; the names the computer reads, five at most, beside their
stars wherever they keep clear of each other, the mark and the object's own writing (outlined on a pale or busy object); the header says
the way ("A STAR CHART OF THE MURUSAC, ON A CAVE'S CEILING"); the caption fills the column down to the mark's block at its foot and ends
in "..." where it is longer. Held up (O), a navigation chart's routes are lines between the real stars from their star, the far future's
places of nothing rings on the sky.

*The harness*: `vesperis_test charts <sx> <sz> <body> [second]` prints every chart of the people's records with its way and what it
shows; `charts ages` finds a chart of each of the twelve looks among the age worlds (and the worlds near home for the oldest with a moon's
count) and draws each on the decoder at 2x (`shots/tests/chart_age_<way>.png`, four held up: `..._held.png`); `charts check` is the unit's
check alone (0.75 s). Unit: "star charts by age" (78 charts of the six age worlds: ceiling 3, bone 6, vault 1, astrolabe 10, page 3,
atlas 6, photograph 3, survey 5, navigation 12, far 29; the lore 1.2 ms at most); the decoder's page 3.4-4.1 ms a whole frame at 2x.

## The decoder reads a history (H-06)

H-06 (2026-10-10) gives the decoder a people's history as its records read so far tell it. A world's records are on three pages, `TAB`
between them (`SHIFT+TAB` back): the records, the timeline of the eras, the history.

*The records* (`game/shards_screen.cpp`): C-06's list under the pages' strip, each row's kind as a glyph before it (a page, a note, a star),
the year in the people's count and the record's first words as the computer last read them; `LEFT`/`RIGHT` show one kind (the texts, the
music, the star charts) or all, the strip counting each kind held (the one shown white); `UP`/`DOWN` and a record's own `LEFT`/`RIGHT` keep
to the kind shown (R-417: held with `SHIFT` they run as the timeline's do, stopping at the ends). C-12's calendar and timeline stay
under the rows.

*The timeline* (`game/history_screen.cpp`, C-12's grown): the people's eras across the width, each its share by the people's records
written in it (an era of few or none a sliver, linear in its years within; B-414: a late people's earlier eras slivers); an era is placed, a band, once a record written in it (B-414: or a history telling it) is read
(the computer dates its records), and its name is shown over the band or, where that row is taken, under it, read through the language as
the records are (THE YEARS OF THE SOHONG until "bells" is learnt); an era no read record is from is a row of dots. The records held stand
on the axis by their years, stacked, in their kinds' colours (a text green, a piece blue, a chart violet; held but not read dim; a
friend's cyan; the last recording amber once read; the one under the cursor white with a mark over it); the axis is solid to the latest
held and dotted on until the last recording is read, when it closes in amber (THE RECORD ENDS, YEAR 3,314). Under it the events learnt are
ticks at their years (those the record under the cursor told white), and under them that record: its date, its medium, its first words.
`LEFT`/`RIGHT` (`UP`/`DOWN`, `PGUP`/`PGDN` by ten) move along the records held, `ENTER` reads one. R-416: held with `SHIFT`
the arrows run (a step as they go down, then from a fifth of a second 12 a second, doubling every half second to 100: a hundred records in
about two seconds), stopping at the first and the last where an arrow alone goes round (`ARROWS  SHIFT FAST  ENTER READ  TAB PAGE  ESC
BACK`).

*The history* (`peoples/legends.*`, `game/history_screen.cpp`): THE AGE THEY REACHED once the records have told it (`AGE_TOLD_RECORDS`,
three records written in that age read, or the last recording: PREHISTORIC .. FAR FUTURE; before, NOT TOLD YET: A-01's rule that the age
is shown in what they built and named only when its records tell it), the events learnt and the records read; then the people as their
records told them (the people's name as a heading, then FAITH with their god once a record named it, RITE, HOLY, LIVED, VALUED, DEAD, ENDED:
`legendWays`), then the events the read records told in the history's order, under the eras they fell in (an era's name and years; one no
read record is from AN ERA NO RECORD READ IS FROM), each a legend's line with its year in their count; the last recording closes it (THE
LAST RECORDING: THE RECORD ENDS IN YEAR 4,414, after THE END OF THE NULO: A MAN WAS THE LAST OF THEM, AT TECHAENOE). `UP`/`DOWN`
(`PGUP`/`PGDN` by eight; R-416: held with `SHIFT` they run as the timeline's do) move along the events, `ENTER` opens the record that told the one under the cursor (the earliest of them; TOLD BY
N at the right when several did) and `ESC` comes back to the page.

*What a record tells* (`ShardText::told`, `ShardText::ways`). The events its genre's directives picked and wrote (H-04: a chronicle's
years, a letter's news, a ballad's battle, an epitaph's death's cause), and since H-06 the death an epitaph or an obituary is about and the
end the last recording tells (kept apart from the salience, `RecordModel::toldLegend`, so no text moved: the grammar's 3,564 texts are H-05's
byte for byte; "one event from several sides" counts 75 events where it counted 71). And the people's ways its drawn alternatives were
guarded by (`faith`, `rite`, `holy`, `economy`, `economy2`, `value`, `value2`, `deadway`, `cause` held equal: a line written for a people of
many gods, for one that buried its dead in barrows, for the drying that ended it). Of the age worlds' texts about half tell an event (from
two in five of the prehistoric and a fifth of the far future's, whose tallies, paintings, instructions, mathematics and messages to
whoever comes tell none, to seven in ten of the industrial: 0.4 to 1.7 events a text), and a fifth to two fifths express one of its ways.

*Only what was told* (`legends.h`): the page holds the events the read records told and nothing an unread one told; a person is named only
once a read record wrote their name (the whole name, or the given name a letter calls them by), until then by what they were at the time
(a woman, a man, a child, a ruler: "A CHILD WAS BORN TO A WOMAN AND A MAN AT KETHRA", `historyEventLine` taking a namer); the lines, the
eras' and the people's names are read through the language as the records are (`decodeLine`: a name kept, a word by its root as the
shards held read it, a word the records never wrote read only once the whole tongue is), so the explorer's legends of a people clear as its
tongue is learnt (three records read of the Nulo: "A MOEWO AT CACUCHAE (HAHA; 17,124 CHOKHAEOEKH)"). The page is built when the world's
pages are opened and again after a reading (`Game::ensureTold`).

*The age worlds* with every third record held and four in five read (`vesperis_test decoder pages`): the Witniikh 24 read (12 texts) tell 7
events, the god, what they lived by and their barrows; the Ta-Zhozo 28 (20) 13; the Aachidblukh 36 (28) 31 under two of their three eras;
the Nulo 39 (23) 26 under three eras, closed by the last recording; the Irrur 48 (33) 19 under four eras; the Glaak 49 (25) 8, the
Stillness alone placed of six eras, their end by an experiment with physics. Of the persons in those lines 0 to 8 are named by what they
were.

*The harness*: `vesperis_test decoder pages` writes each age world's pages (`shots/tests/decoder_pages_<age>_<records|timeline|history|
history_end>.png`), the modern people's music alone (`..._modern_records_music.png`) and its timeline and history after three of its texts
read (`..._modern_early_...`), every history page's rows to `shots/tests/decoder_history_<age>.txt` (a name `{in braces}`, a word not
learnt yet `~theirs`), then runs the unit's history checks. Unit: the industrial world with every fourth record held, those of its earlier
ages read and two texts of its last: the page holds the 41 events they told and none of the 7 only the unread told, no person named
without a record (24 of 36 named), the eras placed by the records read, the age not told yet; a third record of the age read on the decoder
adds what it told (41 to 43) and tells the age; `ENTER` on an event opens a record that told it; the records page kept to the music.

## Each age its own vocabulary (B-415)

The user found an industrial town that read as modern (white stucco, flat roofs, ribbon windows) and as medieval (colonnades of stone,
white chimneys standing like columns) and asked that every age be clear, the genome kept. **The age speaks first, the people second**: each
age has its vocabulary and the genome chooses within it; the genome's variables stay the people's own through its ages.

* **A building's role** (`greatKind`): the people's great buildings (temples, churches, town halls, palaces, keeps and towers, the focal
  accent, monuments, gates, platforms, rotundas, joined towers, aqueducts, tunnels), where its genome speaks fully, and its everyday ones
  (houses, rows, tenements, villas, blocks, halls, mills, works), where the age speaks first.
* **The genome read through the age** (`ageExpressed`, peoples/buildings.h; the generators read it through `Gen::gv`; `archSub`, `ageDome`):

| | Prehistoric | Medieval | Industrial | Modern | Futuristic, far future |
| --- | --- | --- | --- | --- | --- |
| Walls | dry stone, wattle, turf (the rock tinted, or daubed) | the rock tinted toward the base, or painted (adobe a desert people's) | brick, painted or not (`BRICK` by the base) | concrete toward the base, or rendered in it | composite in a light of the base; the far future's one material |
| Everyday roofs | thatch cones, turf, beehives | the people's (any) | slate, pitched or hipped (`ageRoof`); a people of flat roofs behind a parapet (`parapetRoof`); stone, brick or adobe gables standing after the roof (`gables`) | flat | domes and shells |
| Windows | none | small: a third of a bay at the most, lancets in pairs or groups, scattered where the people scattered them | tall sash one to a bay (a pair at most, never scattered), square or round-headed | wide panes or bands; on a temple its own shape, wide (bands for arches and keyholes, slits for triangles) | bands of glow; portholes for arches and keyholes |
| Tops | (none) | the people's: finials, crenels, masts, statues | chimney stacks with their pots on the everyday buildings; the great ones' finials, masts, statues; a tower's slate pyramid, spire or dome; no crenels | a mast or nothing on the everyday buildings; needles, saucers or masts on its towers; no crenels | masts, finials; no crenels or statues |
| Walls' profile | battered huts | the people's (battered, bulging, jettied, set back) | upright (a great building's set back) | upright (a great building's battered, jettied or set back) | the spires' flanks |
| An expressed frame | | half timbering | brick piers between the windows | a concrete grid: columns between the windows, a slab every storey | an exoskeleton |
| Arches, domes | | pointed, round, flat; every dome | round or flat; hemispheres | round or flat; saucers | the people's |
| Colonnades | | columns, arcades | a covered walk of cast-iron columns under iron and glass | pilotis under a slab | |
| The platform | | steps and a shrine | a bandstand | a plinth and its mast | a plinth |
| The great buildings | henges, longhouses, dolmens | the people's own forms | a church's own tall windows and gables; a town hall hipped; a round temple a brick rotunda under an iron saucer (no peristyle) | the temple of its age: a low mausoleum of three broad setbacks, a drum under a saucer or a cone, a saucer with needles for minarets, a basilica under a concrete shell (a steep gable, a vault, a saddle, a saucer, a tent) with needle-topped towers; the focal tower's pod | the people's |

* **The everyday buildings' own ages**: an industrial warehouse's iron columns and tall windows, a mill's chimney sooted at its lip and
  its roof the age's (a row of gables across it for a people of cells), a tenement's four stacks, a villa's chimney at each end; a
  modern block's lift house a module for a people of cells.
* **What a building stands on** (B-416): the mounds' earth banks and the pits' rims are gone (each building stood in a ring of the
  ground's soil as wide as its longest side, over its neighbours); the pits, the cuts in the rock and the kerbs are the hand-made ages'.
* **The reach**: the finish acts in the prehistoric, medieval and modern ages (an industrial people's brick), the tint in the futuristic
  alone, the modules in the modern through the lift houses (`genome reach`).

Where to see it: `genome street [scale] [peoples] [years] [desert|felisian] [drone]` (the same peoples in every age, from the street or
the drone), `genome ages`, `ruins kinds` (every kind of each age at its years), `spot` at a people's place.

## The drone bay (C-18)

The user's request (2026-10-11): "add three drones with robotic hands [...] that you can send from stardrifter to felisian/desert planets
with civilizations. When you send them each can bring 2-8 items [...] cooldown is 5 minutes. They can bring both shards (all three kinds)
and instruments", and the instruments' case "separated by planet/world as shards are".

**What a world holds for them** (`peoples/drones.*`, `droneWorldOf`): every record of its peoples (both, on a world of two: T-04's 80 to 200
each, H-05's texts, music and star charts) and every instrument lying beside their music records (C-11's third by the record: an id once a
world), each at its home: the place of its people's plan that always holds it (`recordHomesOf`; an instrument at the home of the first
music record it lies beside), with the place's latitude, longitude and class. It reads the plan and the histories (blocking on the
approach's prefetch), so the game asks for it on a thread of its own (`Game::droneWorldAsk`) when the bay's screen opens or the drones go;
the modern age world's 250 items in 0.75 s from nothing built, a few milliseconds once the plan is.

**A trip's haul** (`droneHaulOf`): each of the three wants two to eight (`DRONE_MIN_FINDS`, `DRONE_MAX_FINDS`; six to twenty-four a trip:
the request's "max 32" would need eleven a drone). A drone goes to a place drawn by how much of its own the explorer lacks there (the count
to the power 0.75; a place another drone searches at 0.15 of its weight), takes three to five there, the place's own first, then copies
kept there of its people's other records (T-04: a record's copies lie in many places; an instrument beside each copy of its record), and
flies on to a second and a third place for the rest; every find drawn with the rarer kinds leaned toward (a text 1, music 1.4, a star chart
2.2, an instrument 4: a world holds a few instruments to its hundreds of records), and at one stop in three an instrument of the place's people
looked for first. Over two hundred hauls from the modern age world (250 items: 143 texts, 80 music, 18 star charts, 9 instruments) the
finds were 17% instruments (3.6% of the items: two or three a trip) and 9.5% star charts (7.2%), 47% a place's own. A drone brings fewer only when the world has no more; a world whose every item the ship holds sends none.

**The trip** (`game/drones.cpp`, `DroneBay`, `DroneRun`): the drones leave the rack 0.7 s apart (2.2 s each through its port), fall 14 s
to their first place, search for two minutes (`DRONE_SEARCH_S`, the user's after the first trips: from a drone's first place to its last,
the flights of 4 to 10 s between its places within them, the rest shared by its places, a share each and one a find, its finds spread
through each), climb 14 s and dock (2.2 s back through the port): about 2:35 from the launch to the last dock. Each drone's finds go into
the guide as it docks (`shard` and `instrument` lines: the decoder's and the case's, a copy taken is gone from the whole world,
`syncShards` when the explorer is down there; the decoder, open, lists them at once); the log has a line at the launch and one at the end
(`DRONES`), the status the count and its kinds. The bay's clock is real time, in the simulating states (the warp does not hurry it, as the
probe's relay; a menu stops it); the bay charges for five minutes from the drones' return (`DRONE_COOLDOWN_S` from `DroneBay::back`, the
last dock or the recall).
Leaving the parking (an approach, a Vimana flight, a jump) calls them back: what each had found comes aboard, the rest stays down there.
A trip is saved and resumed (`bay`, `bayback`, `bayrun`, `baystop`, `bayfind` lines in the save); `stat drones` and `stat dronefinds` in the guide.

**The drones** (`game/drone_draw.cpp`, `droneModel`): the surface drone's family, not its copy: a short octagonal hull in a light paint
(bank 30) with the dark band of its eye on the nose's bevel and three cyan lenses in it, the bay's stripe down its deck, its number in
lamps by its tail, a beacon on a mast; four ducted fans on struts; two skids; a hold under its back; and on the nose's flanks two arms in
the bay's yellow, a shoulder, an elbow and a wrist of metal, each ending in a hand of three fingers of two joints (folded, the hands held
beside the nose like a crab's claws; at a place, reaching down under it, a hand closing on each find and opening after). On the rack (three
shelves against the left wall by the front window, the bay's screen across its head in three cells, a lamp at each shelf: green aboard
and ready, amber charging, red blinking while its drone is out) they stand facing the room; launched, each lifts, its fans blur, and it backs
through its port into the hull; outside it flies past the windows lit by the sun, then dwindles toward the world as a strobe; coming back,
the same in reverse.

**The screen** (`GameState::DRONES`, the rack's E or the devices page): the bay's state (`READY`, `BACK IN 1:52` while out, `CHARGING
4:10`); each drone in lines (its visible faces' edges; turning on its shelf, its face three-quarters on while out), its number, its finds of
what it wanted or its height while it falls or climbs, where it is (`IN A CITY`, `FLYING TO A TOWN`, `CLIMBING`, `ABOARD, CHARGING`); the
world's map (its ground under the screen's lines, the ground under the ship, the places searched and the ways flown, the drones blinking);
what the ship holds of what the world left, in the decoder's words (`ABOARD: 58 OF ITS 262 SHARDS` / `AND 7 OF ITS 9 INSTRUMENTS`: the
world's peoples together, where the decoder lists each people's on a row of its own, `32 OF 129` and `26 OF 133`); the finds, the newest
first, dim until aboard. The peoples are named only once the ship holds a record of theirs (`TWO PEOPLES LIVED THERE`). The cabin's line
while they are out: `DRONES BACK IN 1:52 - 4 FOUND`.

**The case by its worlds**: the instruments' case opens on its worlds, a folder each as the decoder's (the world's name and where it is,
how many it holds; the world under the ship first); Enter opens one (its instruments, the C-11 screen within it), Esc goes back to the
worlds and Esc again to the cabin. A world of two peoples is a folder each people, as the decoder lists a row each (the user, the same day:
"in instruments case split instruments by civilization as a decoder does"): the people named on the folder's line after the world
(`UNKNOWN (+208 +0 +51 / 5)  THE VASANTHUL`), the open folder headed `UNKNOWN - THE VASANTHUL` and holding that people's alone (an
instrument's id carries its people: people x 1000 + age x 100 + slot).
