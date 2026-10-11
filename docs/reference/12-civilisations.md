# 12. The civilisations: their worlds, their ruins, their shards, their music, their voices

The C series of 1.1.0 (`docs/ideas/IDEAS-ruins-shards-signals.md`, `docs/ideas/MILESTONES.md`). A people lived on one
felisian or desert world in eight and is gone; what it left is settlements of ruins (C-01, `06-surface.md` and
`05-planet-function.md` for the trait and a dead desert's old seas), from C-02 on its words, from C-04 its music and from C-05
its voice. One such world in five had two peoples (C-13), on either side of a great circle, each with a culture, a lore, a
tongue, a voice, a music and fifty recordings of its own. 1.3.0's expansion (`docs/ideas/ITERATION-1.3.0.md`) gives every
people the age it had reached when it collapsed and a profile the later milestones read (A-01, generation 13: "The ages",
below).

## The trait (C-01)

`TR_CIVILISATION` is never drawn by the trait draw: `BodyGen::make` gives it to a felisian or desert world when
`mix64(seed ^ 0xC1D1)` falls under 0.125, first among its traits (after the glass), so no other world's traits move.
A trait desert keeps the level its seas stood at (`BodyGen::oldSeaM`, -120..100 m) and a river density of 0.8 at
least: dry seabeds with salt in the deeps, old channels, a shore band. Every other desert is a desert and nothing more.
`worldHadCivilisation(g)` (`galaxy/ruins.h`) is the one question the ruins, the landmarks, the harness and the shards ask. Since R-408 (2026-10-07) the surface scanner's RUINS mode hears every settlement within 40 km (`ruinsNear`; B-417: M4-07's monoliths of the old ones, on every felisian, quartz and ocean world a people there or not, are gone, and a world without a people offers no RUINS mode) and points to the nearest one the explorer has not marked; the explorer marks and names what they find (`07-game-flow-and-controls.md`).

## The settlements (C-01)

`galaxy/ruins.*`: hamlets, villages, towns and lone monuments (a desert's; B-417: every people's world's) on the 2 km ruin grid, in a `Culture` the whole
world shares (`cultureOf`; C-13: the whole people, a settlement's being its people's, `RuinSpec::people`), each building drawn from its own seed with a doorway, a window, an inner wall, a roof where
the culture built stone ones, rubble at the fallen walls. `06-surface.md` has the layouts, the pieces, the levels of
detail and the colliders (B-405: each spans its own height); `08-testing-and-tools.md` the scenes, the `ruins` mode and
the unit's checks.

## The shards (C-02): the story grammar

A shard is a recording the people left; decoded, it is a short text or (C-04, below) a piece of music. The texts are
generated without a language model, deterministically from the body's seed and the shard's index, so the same shard
reads the same on every machine and can be found and shared like a landmark (`galaxy/shards.*`).

(H-03 rebuilt this grammar on the engine: the vocabulary below and the kinds are `src/galaxy/grammar/*.vg` now, the slots facts
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
grammar (`docs/ideas/RESEARCH-records.md`).

## The shards in the ruins (C-03)

(T-04 replaced the placement: "Where the records lie" below; `shardSitesOf` is `recordSitesOf`, the fifty are 80 to 200.) A settlement held a few of its people's fifty (`shardSitesOf`, `galaxy/shards.*`, from the settlement's seed; C-13: the second people's at the indices from fifty up): a hamlet one
three times in five, a village one or two, a town two to four, a lone monument one seven times in ten; distinct within the
settlement, copies across the world (10-decisions: fifty a world, in copies). The buildings are drawn by a weight (a hall 2,
a tower or a rotunda 1.5, a stela 1.2, a house 1) and each tries its spots in order: a house's far corners from the door,
then the corners beside the door's wall, then the back's middle; a hall's far end along the nave (the columns stand in two
rows at half the width); a tower's far corners; a rotunda's side opposite its doorway at 0.55 of the radius; a stela's front,
then its back. A spot must be 0.42 m clear of every piece of the building that stands on the ground and 1.5 m from the
doorway (`buildingDoor`, `galaxy/ruins.h`), so the explorer can stand at it; `ShardSite` is the index, the building, the
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
the decoder on the cabin's back wall, the worlds, a world's shards by their years, a shard's text); `galaxy/shards.*`
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

## The music (C-04)

Twelve to eighteen of a world's fifty shards are pieces of music (`Shard::music`, `shardIsMusic`: a shuffle of the fifty
from the world's seed, the count from the seed too), with no text; they keep the tone a text would have had, and the
piece's form follows it. `galaxy/music.*` is the module, `game/audio.cpp` the player, `game/shards_screen.cpp` the screen
(`07-game-flow-and-controls.md`).

*The tradition* (`traditionOf(gen, lore)`, from the body's seed and the names' style): a period (the octave at 82%, a
stretched or narrow one of 1.93-2.08 at 13%, the twelfth at 5%); a scale of five to nine notes within it, from an equal
division of the period into 5-24 steps (55%) or from just ratios (16/15 to 15/8, and past the octave for the twelfth),
redrawn until no two neighbours are under 75 cents apart or over 450; a resting degree (the one nearest a fifth, else a
fourth, else the middle) the open phrases end on; a tonic of 165-330 Hz; a cycle of 3-12 beats in groups of two and three
(a four now and then), each beat in two (75%) or three; a beat of 66-144 a minute that wanders by 1-5%; two or three
timbres of five families (`makeTimbre`: a plucked string, a pipe, a bowed string, a bell with stretched partials, a reed;
six partials, an envelope, vibrato, breath), hard names leaning the draw to the bell and the plucked string and keeping
the drum, flowing names to the pipe and the bow; a drum of two skins; a drone (40%); a second voice (in parallel below the
lead, answering a phrase later, or doubling a period below; or none, in which case the second timbre plays the answers);
an ornament rule (grace notes before a leap, trills on long notes, slides between neighbours, mordents, or none) and its
chance (20-50%). Aieliaalas II: seven notes of twelve equal steps (0 100 300 400 700 1000 1100 cents), a cycle of 3+3+3 at
89, a bell and a pipe, trills; Leileashphail III: seven notes of thirteen steps (0 185 277 369 462 738 1108), 3+2+2+2 at
69, a plucked and a bowed string, mordents. Two worlds never share a tradition; after two pieces of one the listener
knows it.

*A piece* (`pieceOf(tradition, shard)`, from the shard's seed): its form by the tone (ordinary: a dance, a lullaby, a work
song, a hymn, a harvest song, a love song, a song of the festival; an elegy: a lament or a hymn; a warning: a march, a song
of warning or a hymn; the end: the last song), each form a row of tempo factor, note density, velocities, rest chance,
register (a lullaby high, a hymn or a lament low), drum (a dance, a march, a work song, a festival always; a lullaby, a hymn,
a lament, the last song never; the rest as the tradition has it) and whether the answer and the return come twice. Phrases
of one cycle (two when the cycle is six beats or under): a rhythm per group (subdivisions, beats, two and three beats, by
the density; the closing group one long note), a walk of degrees (a step at 50%, a repeat at 15%, a leap of two to four at
35% answered by a step back, held within a range round the phrase's centre), a rest of a beat at most. The structure is
statement (resting open), variation (some degrees moved, some notes split, closing on the tonic), answer (higher, open),
return (the statement closed), the answer and the return once more for the quick forms, and a close of the tonic held a
cycle: 28-70 s. Then the ornaments on the lead, the second voice, the drone (the tonic a period below, a cycle at a time
on the last timbre) and the drum (a pattern per cycle from the groups' first beats, filled on a phrase's last group, one
low hit on the close). A `Note` is a start and a length in beats, a voice, a degree and a velocity; the tempo's drift is
the player's.

*The player* (`AudioSynth::musicSample`): ten voices, each a note of a timbre (the partials summed, the velocity opening
the upper ones, attack-decay-sustain-release, vibrato after 0.3 s, breath; a drum a thump whose pitch falls, and noise),
the clock in beats wandering by the drift; `AudioState::piece` and `tradition` are set by the decoder screen, a null piece
stops everything at once, `pieceBeat` and `pieceDone` come back for the screen. The ship's hum is ducked to 0.2 under a
piece. Rendered to a WAV through the harness, the dominant pitch of a march's windows sits within 5 cents of the
tradition's scale; a dance peaks at 0.8, a lullaby at 0.3.

*On the ship.* A piece's row in the world's list is its form and length, or its name; opening it plays it from the top
with the notes drawn as a roll the playhead crosses; `N` names it (the guide's `names`, keyed like the shard; a note in the
log) and the name shows in the list and on the piece. A piece is heard whole from the first (the language has no part in
it) and recorded as read at the world's full count. `vesperis_test music 151 25 1` prints the tradition, the thirteen pieces
and the first as notation, `... 1 wav` renders it.

## The voice (C-05)

(H-04: the voice speaks the page shown, the opening first, and the rest on request; "The records (H-04)" below.)

A text shard is a recording of someone speaking the people's tongue: the spoken words are the written ones, the
people's word for each of ours (`DecodedWord::theirs`), a name as it is, a numeral digit by digit in the people's words
for the digits. `galaxy/voice.*` is the module, `game/audio.cpp` the speaker, `game/shards_screen.cpp` the screen
(`07-game-flow-and-controls.md`).

*The sounds.* `PHONES` is one table for every world: a row per spelling the tongue's pools (`T_ON`, `T_NU`, `T_CO`) and
the name generator use, longest spellings first (`sh`, `th`, `kh`, `ng`, the diphthongs `ai`, `ou`, `ia`... before the
single letters): a vowel's three formants (a man's; a diphthong's two sets), a stop's locus and the band and level of its
burst, a fricative's noise band (`h` is breath through the formants), a nasal's, a liquid's and a glide's formants, each
with a length. `parsePhones` reads a word by the first row that matches at each letter, so `thovriss` is th-o-v-r-i-ss and
a word's syllables are its vowels. A world's inventory (`Voice::inventory`) is the sounds its pools spell: Aieliaalas II
17 (ou ia ie ea th a o p b t d g z f l r w), Leileashphail III 14.

*The voice* (`voiceOf(gen, lore, tongue)`, from the body's seed and the names' style): a pitch of 85-286 Hz with the
tract (the formants' scale, 0.8-1.3) longer for the deeper voices; a range for the melody; a rate of 3.0-5.6 syllables a
second; breath (7% of worlds whisper), roughness (jitter and shimmer on the period), a nasal murmur, a tremor, a second
tone an octave or a fifth below (12%); the stress rule (the first syllable, the last, the last but one, or none); the
sentence's melody (falling through, rising to a peak then falling, level with a drop at the end); tones on the syllables
(25%), clicks for the voiceless stops (15%, 35% with hard names), rolled r's (25%, 50% with hard names). Hard names sit
lower and rougher, flowing names run quicker and breathier. `voiceLine` is the sentence under the year on the screen (`A
DEEP BREATHY VOICE, SLOW`, `A LOW NASAL VOICE`, `A HIGH CLEAR VOICE, QUICK, WITH CLICKS`). Aieliaalas II: a deep breathy
voice at 100 Hz, 3.2 syllables a second, the stress on the last syllable but one, the melody falling; Leileashphail III: a
low nasal voice at 136 Hz, 4.1 a second, rising to a peak then falling.

*The speech* (`speechOf(voice, words, seed)`): the words in order, each a run of segments (a sound each with its time, its
pitch at its start and end relative to the base, its loudness; a stop a closure then a burst, a click in a clicking
voice), the syllable's sounds summing to 0.28 s at the base rate, the stressed syllable's vowel longer and the syllable
louder, the last before a pause longer still, the pitch by the sentence's melody and the stress or the tone, a fall at a
full stop, a rise before a comma or at a question; a pause of 0.02 s between words, 0.18 at a comma, 0.28 at a semicolon,
0.45 at a full stop (all at the base rate, longer in a slow voice). The speech keeps every word's span (`wordStart`,
`wordEnd`): a text of thirty words speaks in 12-25 s. The speech does not depend on the share read, so the alignment holds
whether the word shows as ours or as theirs.

*The speaker* (`AudioSynth::speechSample`): a glottal pulse (the derivative of a Rosenberg flow) at the segment's pitch
with the roughness' jitter and shimmer, the tremor and the second tone, breath as dulled noise; three formants moving at a
20 ms time constant toward the segment's targets and a fourth fixed, in cascade; a nasal resonance beside; the fricatives'
and bursts' noise through a band of its own; a rolled r modulated at 26 Hz. A text peaks near 0.5 and sits at an rms of
0.13-0.15 (the music's dance peaks at 0.8); the ship's hum is ducked under it.

*On the ship.* Opening a text shard speaks it; on a first reading at the current share the words resolve as they are
spoken (the one being spoken flickering, `DECODING P%`), and the end of the recording reads the shard (Enter skips to the
end, Esc leaves it unread); a read shard is spoken again on opening with the whole text and the spoken word underlined
(`PLAYING P%`; Enter skips, Space pauses, Enter plays an ended recording again). `vesperis_test voice 151 25 1` prints the
voice, the sounds and the first text as it is spoken, word by word with its span; `... wav` renders it.

## The signal radar (C-07)

A few of the dead peoples' worlds still have a transmission on its way out through the galaxy, and the Stardrifter's
receiver catches it (`galaxy/signals.*` the sources, `game/radar.cpp` the instrument, `07-game-flow-and-controls.md` the
keys and the HUD). Which worlds transmit is a property of the galaxy, not of the sweep (ideas note, section 2): the
transmitters are hashed sectors, a few or none in each cube of 32 (`transmitterCell`: one with probability 0.09, two with
0.01, three with 0.001, a mean of 0.113 a cube, about one sector in 290,000), and a transmitter sector's star transmits
when it has a people's world (`starSignal`: the first felisian or desert world with the trait; a pulsar there transmits
its pulses). On the arm's density that is 1.8 peoples' transmissions within the radar's reach of 200 ly on average, so
two explorers sweeping the same sky hear the same ones and a signal's star can be shared by its sector. The cells' salt
is pinned like the home star (G-02): of forty tried, the one kept gives home three (a desert world of a yellow star at
79 ly, a felisian one of another at 85, a third at 179) and the two test worlds three each; fifteen of the forty gave
home none. The signals are rare and far (ideas note: "the journey is the price"): the distance in light years is the
age of what is heard, and a Vimana flight of 79 ly takes 25 s. Since A-01 (generation 13) only a people that had radio
transmits, the cells hold 3.4 times as many candidates and the salt is pinned again: home hears a modern people at 90 ly, a
futuristic one at 184 and an industrial one at 197 ("The ages", below). Since C-15 a gain turned up hears farther, as far
as each transmitter's power carries it ("The radar's gain", below).

*The sweep.* The radar is a camera entered (the user's call, 2026-10-02: "a camera mode you enter for this signal search
minigame, like you are viewing through radar camera"): the ship's own instrument seen through like the vehicles' nose
cameras, the cabin left behind, the mouse turning the ship, the picture cold and full of static that thins as a signal
comes through. The meter reads the beam's gains over the static (`beamGain`: a wide lobe within forty degrees, a middle
one within ten, a narrow one within three; a signal's strength falls with its distance, `signalStrengthAt`: 1 near, 0.48
at 79 ly, 0.18 at the reach), so a sweep finds a rise and turning onto it sharpens it; the scope keeps the last seconds
of the meter so a rise passed can be turned back to, and within the middle lobe the source shows as a smudge of static
to centre on. A hold within three degrees for a few seconds (longer for a faint one) locks: the readout names only the
system the signal comes from (the star, its sector, the distance: the age), never the world or what sent it ("you don't
know if it's felisian, or desert or anything"); the lock holds within eight degrees, and Enter locks the star and flies
there at once (a signal of this system: the fine approach to its body).

*What is heard.* A people's world within the middle lobe is heard before the lock, through the static: its recording on
the air, one of its fifty (`transmittedShard`: the fifty walked in a stride of the world's own, coprime with fifty so every
recording comes round before any repeats, one a slot of 75 s of the game's time; a lock held hears the next in the walk),
a text spoken in the world's voice (C-05) or a piece of its music (C-04), built from the same `ShardWorld` the decoder
uses and ghosted by the receiver (the band 250 Hz to 2.6 kHz, a metallic ring under it, a slow fade, dropouts, the echo
of the void; the static thinned as the clarity rises). The nature of a signal is told by its sound alone, and the sounds
are made to unsettle (the user's call: "some deep cosmic-horror like feeling"): the bed is a rumble that breathes, a
drone of two tones a quarter-tone apart beating, a wisp of resonant static that wanders, through an echo a third of a
second behind; a pulsar is a thump far down with a growl falling away after each and a thin whine circling; a comet
sings, a warble far down with a second voice a semitone off, between geysers of hiss and ice cracking; a magnetosphere
has whistlers falling slowly, bursts of chorus, a breathing band of hiss and a choir of three voices a quarter-tone
apart that comes and goes; a people's voice comes with a carrier drifting under it (`07-game-flow-and-controls.md`,
the audio state; `vesperis_test signals wav` renders them).

*More of it, and no two alike* (the user's second call, 2026-10-02, after playing: "the sounds for signal are PERFECT
... echoed voices, buzzing, pulsating were all perfect ... the only thing I didn't like too much were pieces that were
similar to music. Please, give MORE sounds for signals, greater variety in that cosmic horror direction"). A people's
broadcast is no longer the fifty recordings read out one after another: each slot of the walk is a *programme*
(`programmeOf`, `Programme`, `programmeFor` in `galaxy/signals.h`), hashed from the world and the slot, and each world
has a character of its own (two of the five machines and two of the four ways of speaking are its, from the world's
seed). A text slot is spoken as it is (`A VOICE`, eleven slots in twenty), whispered close to the microphone with
someone breathing under it (`WHISPERS`: the world's voice at a breath of 0.95, the content doubled a tenth of a second
behind), intoned deep and slow over a hum of three voices round the chanter's pitch in a long echo (`A CHANT`: the pitch
at 0.72, the rate halved, level, trembling, an octave under), read as a numbers station (`A NUMBERS STATION`: digit
groups in the people's words for the digits, the first the shard's year, a chime of three falling notes before each,
five to eight groups with a second or two between, a carrier with the mains on it) or looped (`A LOOP`: the first three
to six words again and again, four to seven times with three to eight seconds between, a relay's click and a blip at
each start, the mechanism's whir while it runs); a music slot is the piece three times in ten (`A PIECE, SLOWED`: at
half to three quarters of its speed through a heavy tape wow and a deeper ring, so the music is what the user heard
least of) and otherwise one of the world's machines, as are the rest of the text slots: a beacon (`A BEACON`: three to
seven tones of its own in a pattern, a rest, again; half of them sag, dying), data (`DATA BURSTS`: two tones switched by
a shift register at 12-60 bits a second in bursts, the mains buzzing under them), a bell (`A BELL TOLLING`: six partials,
far off, the period a little irregular, now and then struck twice), a siren (`A SIREN`: a slow glide or two tones, a
sawtooth through a resonance, carried in and out as if on a wind) or a murmur (`A MURMUR OF VOICES`: three *phantoms*,
voices that are no one's, a glottal pulse through three formants babbling vowels at a syllable rate behind a wall, one
of them raised now and then). Yethlyeahei II-c, the first heard from home: 20 voices, 12 whispers, 1 numbers station,
5 data, 10 bells, 2 pieces over its fifty. The natural kinds take their parameters from the signal's seed
(`AudioSynth::rvDerive`): a pulsar's thump sits between 45 and 95 Hz with its growl and whine its own, half of them have
an interpulse half a period on, three in ten fall silent for spells (a nulling pulsar), and the slow ones may draw a
breath before each thump; a comet's song is 60-130 Hz with its second voice a semitone, a quarter-tone, a fifth, an
octave under or a tritone off, some glide, some moan (a phantom riding the song), the geysers and the ice at rates of
their own; a magnetosphere's whistlers, chorus and choir come at their own rates, the choir in quarter-tones, fifths, a
cluster or octaves, half of them crack with lightning (sferics and tweeks), a third roar. And the static itself is not
empty: every twelve to forty seconds something is in it (quieter while a signal is clear): a carrier passing, a boom far
off, a moan, knocking, a murmur, a long breath drawn, or, rarely, a scream far away. The recording counts as heard
(`Guide::heard`) when its words or its piece were on the air, not for a numbers station or a machine.
`vesperis_test signals wav` renders the bed over thirty seconds, three seeds of each natural kind and every programme of
the first people's world, twenty-one WAVs.
"A signal could even be a shard heard from afar, so that on arrival you recognise the voice or the music" (ideas note):
what the radar heard while locked is in the guide (`heard`), and the shard taken later from the ruins that the radar
caught is logged as such. The language has no part in it: the voice is heard in the people's own tongue, the decoding is
the decoder's.

*Not every lock is a story.* The radar also catches the pulsars (every pulsar in a transmitter sector, and one in 320 of
those within 25 ly), a comet of the system within 1.5 AU of its sun outgassing and the magnetosphere of every gas giant
and of every big world with a field of the strongest; `localSignals` reads them from the system every frame. The readout
does not say which is which; the sound does. A pulsar's lock flies to it like a people's; a comet's or a magnetosphere's
approaches the body.

## Ruins that read the world (C-08)

The ideas note (section 3) asked that "a ruin's layout follows what the people had: a coastal culture leaves harbours on
the old shoreline, a mountain one terraces and cairns on the ridges, a desert one cisterns and long walls against the
wind. The terrain already knows all this, so the buildings come from the same data", and section 4 that the ruins weather
by the world's own climate "so the ruin tells you how long ago without a number". C-08 (2026-10-02) builds both on
C-01's settlements (`galaxy/ruins.*`; `06-surface.md` has the pieces and the drawing).

*The terrain read.* A settlement reads the ground round it once, when its layout is made (`readSite`, kept in
`RuinSpec::site`): the grade across it and the heading it falls toward, the highest ground, the planet function's
relief at the centre, the nearest water or old sea within 500 m of its edge (the shoreline bisected to a few metres) and
the prevailing wind's heading (the dune fields' rule: the world's wind angle, flipped per hemisphere, turned per
30-degree band). From that it gets its features (`addFeatures`, `featureList`): a harbour on a shore (a quay along the
shoreline, a mole out into the water, a warehouse behind; on a dead desert world the quay and the mole stand over the dry
bed, which is the old shore made visible), terraces on a slope (retaining walls along the contour between the houses),
on a desert a cistern with its channel at the upslope edge and a crescent of wall against the wind, and cairns out along
the ridge in mountain country. Nothing is placed that the terrain did not ask for: a settlement on a flat inland plain
is as C-01 left it. On the desert test world's region 317 settlements are sited, 19 terraced, 123 with a cistern, 134
with a wind wall; the harbours are on the coasts (the scenes find them 60 km from the types' spots).

*Weathering by age.* The culture's decay was a draw; it is the age's now (`decayOf`): the years since the end are
hashed from the world's seed, 200 to 40,000 log-uniform, and a wet world (rain) decays a fifth more, so the two test
worlds sit at the ends: Leileashphail III ended 209 years ago and stands to its lintels (decay 0.19), Aieliaalas II
22,045 years ago and is foundations (0.92). The walls sink the deeper the older, and on a desert the sand has drifted
against every wall the wind meets (a wedge in the sand's colour). The plans, the roofs and the towers are the draws
they were (C-01's stream is kept), so a known settlement keeps its buildings. The age is not told anywhere: the
description line still says "the ruins of a people that is gone"; the shards' own years (`Shard::year`) count from
the founding and are a different clock (C-12's timeline).

*The explorer among them.* No rock is drawn or stands inside a room and no fallen log lies in one (KI-345's half that
could be done without flattening ground), and the capsule sets down outside a settlement's walls
(`settlementClearance`, `THE CAPSULE KEEPS OUTSIDE THE RUINS`). What stays by design is in KI-345 (the walls step at
their joints on a slope: there is no plinth and no ground is flattened, the terraces are walls on the slope) and KI-350.
The age also exposed two C-01 defects, fixed here (`10-decisions.md`): the doorway's midpoint was mirrored on two of the
four walls (rubble could lie in a real doorway, a shard could sit in one), and a town wall could run through a house.

## Star charts (C-10)

The ideas note (section 3) asked for charts: "Some shards are a map, not a sound: the culture's own sky, drawn from their
world with their constellations and names. Decoding it on the ship overlays their sky on yours, so you can find the star
they named, and sometimes it points at another dead world. Cultures that knew each other become a chain to follow." C-10
(2026-10-03) builds it in `galaxy/charts.*` on C-02's shards and C-06's decoder (`07-game-flow-and-controls.md` for the
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

## The roads (C-09)

The ideas note (section 4) asked for roads that still go somewhere: "a dead civilisation's roads survive as faint lines in
the terrain (compacted ground, a different material), readable from the landing zoom. Following one on foot or by buggy
leads between the ruins of a world: a network to trace rather than points to find". C-09 (2026-10-03) builds them in
`galaxy/roads.*` on C-01's settlements and the planet function, so the surface, the landing zoom and the harness read one
network.

*The network.* The nodes are the sited settlements of the ruin grid (`roadNodeOfCell`: C-01's cell draw without the
layout and its site test without the drainage, as the view sites them). Two settlements within two cells and six and a
half kilometres of each other are joined when no third settlement lies closer to both than they are to each other (the
relative neighbourhood graph, `roadJoined`): every settlement reaches its nearest neighbour, a town gathers three or four
roads, the graph is planar and the same from either end, and a pair is judged by the union of its two ends' blocks of
cells so the answer never depends on where it was asked from. On the desert test world's region 45 settlements have 66
roads (209 km together), on the felisian's land region 58 have 89; none is without a road, the degrees run one to four.

*The way.* A road runs from one settlement's edge to the other's (`roadWay`): the end of the street's axis that faces
the other settlement where the plan is a street or a grid, a town's gate at 1.04 radii, the edge toward it otherwise.
Between them it is a walk in fifty-metre steps that takes the gentlest of five headings each step (straight at the
goal, 22 and 45 degrees either side: the grade squared, the turn and the way off the goal weighed together, and the
straight step taken without looking further where the ground rises under six per cent), never stepping into the sea, a
lake or a dead desert's old sea, which stood when the roads were used (a pair whose every way is wet has no road); the
kinks are averaged out and a waver of a few metres over a few hundred keeps a road over a plain from being a ruled
line. A road is about a fifth of a millisecond to walk, so the view walks its block at a landing and the zoom its window
on the worker pool.

*The road on the ground.* The culture's roads (`Culture::roadHalf`, `paved`, `roadWear`) are five to eight metres
wide (a metre more between towns), paved with the rock family's material or beaten (the ground's own, the bare earth of
the sand family through grass and forest), and worn by the age: a noise along the way under the culture's wear leaves
gaps of forty to a hundred metres (`roadLeft`), a tenth of a young desert people's length and half of a
twenty-millennia people's on a wet world (Leileashphail III keeps 94 per cent of its roads, Aieliaalas II 51). The
site grows its network as the explorer moves (`SurfaceSite::ensureRoads`: the cells within five of the explorer's,
walked once each, again two cells on; a probe site builds none) and every terrain vertex within a hundred metres of
detail reads it (`sampleAt`): the road's share of the vertex's footprint is `TerrainVertex::road`; on the 4 and 16 m
rings a vertex on the bed takes the paving as its material, loses its vegetation and darkens a tenth (a beaten track a
fifth), so the two-material contour of B-311 draws the road's edge through the cells; on the 64 m ring the tone alone
darkens, a faint line from the air; the 512 m ring sees nothing. No tree, log or rock stands on the bed
(`roadCover`), and the buggy rolls on rock where the road is paved. Under the feet the readings say `OLD ROAD NE-SW` (R-414: `Tab`;
the HUD's own line before), the first step onto each road of a landing is a status line and a `ROAD` entry in the log, and on the
landing map the zoom draws the window's roads as faint lines in the ground's own colour (`Game::zoomRoads`: the villages' and the
towns' roads, since the whole web of lanes between hamlets was a honeycomb over the land), the one map of a world that shows something
not yet found: the roads are terrain, as the rivers are. R-415: with the peoples (`V`), off as the map opens.

*The peoples on the landing map* (R-415, 2026-10-11, `game/landing_map.cpp`; the user: "make them turned of by-default and make it a
separate button ... something like whole map autocompleted when you find all the shards for a civilization"). `V` shows the layer: the
explorer's marks of ruins and works (small points; the names of the three nearest the cursor beside it), the zoom's old roads and the
plan's cores (below), the places the records held were kept at (each record's home, `recordHomesOf`: of the places standing in its
year, by their size; read with the world's items, `droneWorldAsk`, the drone bay's reading, from the map's opening) and, of a people
whose records are all held, its whole plan: its land tinted to its frontier, every place (a town a point in the people's colour, the
first amber and the second cyan, a city paler and rimmed, the capital white and named) and every way, coloured by kind as the plan's
maps (`NET_KIND_COLOURS`). The homes spread over the land (the capital weighs ten, a city four, a town one and a half): on the drone
bay's world of two, 21 of the first people's 147 records showed 21 places and its capital among them, all of them its 454 places and
847 ways. The legend counts each people's records held of its count, its name once a record of theirs is held.

*The harness.* `vesperis_test roads <sx> <sz> <body> [latDeg lonDeg]` prints the culture's road style, the region's
network (the nodes, the pairs, each road's length against the straight line, its bends and steepest step, the share
left after the wear, the cost) and a sketch; `vesperis_test zoom <sx> <sz> <body> <latDeg> <lonDeg> [scale]` renders
the landing map's zoom over a point with the roads drawn (at 4x the roads are hairlines in the ground's own colour,
which is the game's default; at 1x, the flow's scale, a pixel each, so the test frame reads denser than the game); the
scenes `civilisation_road` and `civilisation_road_desert` stand on a road looking along it (the print gives the road
under the camera and twenty metres ahead against the ground as it is); the unit checks the network and the ground
(`08-testing-and-tools.md`); the flow's `roads_zoom` frame shows the zoom. What stays by design is in KI-352.

## Graves and names, the calendar and the last recording (C-12)

Three small things of the ideas note's third section, built together on 2026-10-03 because they share the people's own
count of years: a burial ground at every settlement whose stones carry names and years, a calendar the shards are dated
in, and one shard a world that is the last and closes its timeline on the decoder.

*The calendar* (`loreQuick`, `calendarOf` in `galaxy/shards.cpp`). The day is the world's turn and the year its orbit
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

*The graves* (`galaxy/graves.*`). A settlement's burial ground lies outside its edge (beyond the wall and every
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
traditions, whose shards speak of the other. Their catastrophe may be shared or not" (`docs/ideas/IDEAS-ruins-shards-signals.md`,
section 4). None, one or two: a felisian or desert world without the trait has no people (seven in eight, unchanged), a
trait world has one, and one trait world in five has two (`peoplesOf`, `galaxy/ruins.h`: `mix64(seed ^ 0xC13)` under 0.2; one
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

## Lending (C-14)

"The guide export already carries names to a friend. Decoded shards travel the same way: a friend hears the music you
found without flying there, credited in cyan like their names are" (`docs/ideas/IDEAS-ruins-shards-signals.md`, section 3).
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

## The ages (A-01)

The expansion of 1.3.0 (`docs/ideas/IDEAS-civilisation-expansion.md`, section 1) starts here: a people is found at the age
it had reached when it collapsed, one of the user's six (prehistoric, medieval, industrial, modern, futuristic, far
future), and has a profile every later milestone reads, built to the fields of H-01's data model
(`docs/ideas/RESEARCH-records.md`, section 6). `galaxy/peoples.*`; generation 13.

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
  the next. The world's disasters weigh in at every age:
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

## The radar's gain (C-15)

Turned up, the radar hears farther (R-410: "a way to increase sensitivity of signals radar so you can find further signals
but it should give some radio noise, creepy mixed sounds when you are trying to get really far signal"). C-15 is the dial
and what it hears; the noise and the creepy mixed sounds are C-16's (below). `10-decisions.md` (C-15) has the reasons.

* **The dial.** In the radar camera `+`/`-`, the wheel and `PgUp`/`PgDn` step the gain through x1, x5, x11, x18, x25 and
  x32 (`SIGNAL_GAINS`), written by the scope (`GAIN X11`). The dial stays where it was left while the radar is off; a new
  game or a load sets it back to x1.
* **A power per transmitter.** C-07's transmitter sectors are the weak (`transmitterPower`: 0.005-0.05, log-uniform from the
  sector). A second, sparse population of strong transmitters is hashed in cells of 256 sectors (`deepCell`: a Poisson count
  of mean 1.4, powers 1-40, log-uniform): one in 12 million sectors against C-07's one in 85,000. A pulsar in a transmitter
  sector carries a third of its sector's power; the loud near pulsars keep their 25 ly at every gain.
* **How far.** Every transmitter within 200 ly is heard at every gain, at C-07's strength (so x1 hears exactly A-01's
  lists). Beyond, a transmitter of power p is heard at gain G within 200 sqrt(1 + (G - 1) p) ly (`signalReachAtGain`), and
  its strength there is its margin over the floor read as C-07 reads the distance (0.18 at the edge of its reach). The
  weak are carried to 319 ly at x32, the strongest to 7,045. Each signal keeps the least gain that hears it (`needGain`).
* **Only what was on the air in the light that reaches the ship.** A far people is heard only where its distance is
  within the years since its world's first broadcast (`Signal::airYears`: its end's years plus its radio's): from farther,
  the light left before its radio. So what the gain finds is nearly always a people's machines, the light having left
  after its end (as C-07's near signals always are, the ends running from 200 years); a people heard alive, its end's years
  under the distance, is rare (none from home).
* **What each step adds.** The steps are even in G, so each adds about the same area of the disc: about two peoples a
  step on the arm (2.3, 2.2, 2.2, 2.0 and 1.7 on average from home over 300 layouts of the salts). The salts are pinned
  so that every step adds one to three from home and from all nine test worlds. From home:

  | Gain | Peoples | Added (age, distance) | Pulsars |
  | --- | --- | --- | --- |
  | x1 | 3 | the Ghutreig (modern, 90 ly), the Visie (futuristic, 184), the Otobrun (industrial, 197) | 5 |
  | x5 | 6 | Eeneseel I (industrial, 202 ly), Venepheinene II (industrial, 216), Elailyeen II-a (modern, 349) | 6 |
  | x11 | 7 | Ifelangrisk B III (modern, 3,085 ly) | 6 |
  | x18 | 8 | Phirihal I (futuristic, 3,469 ly) | 8 |
  | x25 | 10 | Nisyionila II (far future, 211 ly), Eelelaiselash B II (futuristic, 4,548) | 8 |
  | x32 | 12 | Phiostraisia I (far future, 4,500 ly), Esamiala I (modern, 5,524) | 10, the farthest 3,841 ly |

  Every one of them is heard as its machines (the light left after the end). The test worlds hear eleven to thirteen
  peoples at x32, the farthest 5,477-5,540 ly. The scan from home at x32 takes about 10 ms.
* **The price.**
  * The static's floor rises with the gain (0.07 at x1, 0.37 at x32), wanders wider and fills the scope with grass, so a
    far signal is a small rise over it. Above x1 the meter's top is soft, so a full meter is an overload alone.
  * A near or a local signal drives the receiver harder with the gain (`signalDrive`: its strength times G^0.4). With the
    beam on one whose drive is 1 or over the meter pins red, the hold is refused (`OVERLOADED - TURN THE GAIN DOWN`) and
    the sound is overdriven (`AudioState::radarDrive`). At x32 that is the peoples within about 165 ly and most of a
    system's own: the Ghutreig at 90 ly overloads from x11, so it is locked at a lower gain.
  * The hold grows with the gain (x2.25 at x32: eleven seconds for a faint far signal, nine for the Ghutreig against four
    at x1), the lock cone narrows from 3 degrees to 2 and the keep cone from 8 to 4. The hold is on the loudest signal
    within the lock cone. Turning the gain down under a locked signal loses it (`LOCK LOST - TOO FAINT AT THIS GAIN`).
* **The far scan** runs when the gain first goes over x1 in a sector, 40,000 cells a frame (`SignalScan`, five frames from
  home), with `LISTENING DEEPER...` on the readout meanwhile; it finds the whole dial's list at once, so a step is a filter.
* **The readout is C-07's:** the system, its sector, the distance and the age, thousands of light years with their commas
  (`A SIGNAL FROM UNKNOWN (S06)  5,523 LY` from the flow's point at home, the sector line dropping `THE SIGNAL IS` where it would pass 52 characters).
  Enter locks the star and flies there. A far signal locked is a red ring on the galaxy chart (G-05) and one of `N`'s first
  stops there (G-06).
* **The harness:** `vesperis_test signals` prints the six steps from home (what each adds, the nearest and the farthest,
  the pulsars, the cost) and from the test worlds; `signals <sx> <sy> <sz> x32` lists what that gain hears there, each
  signal with its power and its step (`08-testing-and-tools.md`).

## What the gain lets in, and each age on the air (C-16)

The receiver is C-07's. Turned up, it lets more of the void through: R-410's "radio noise, creepy mixed sounds when you are
trying to get really far signal". Nothing of it is printed; the sound and the picture tell. `10-decisions.md` (C-16) has the
reasons and the numbers.

* **The floor.** The static rises with the gain:
  * the hiss is louder and a crackle of clicks comes in;
  * the rumble breathes deeper and quicker;
  * the receiver's level holds, so the noise comes up and the loudness does not.
  At x1 the bed is C-07's.
* **The galaxy's own noise.** The band's brightness round the beam (`radarSkyAt`) is a rush in the static that scintillates:
  * loud toward the band and the core (from home the core is at AZ 115 EL 0), faint toward the poles;
  * it lifts the meter's floor up to 0.12 at x32;
  * it takes up to a third of a far signal's clarity.
  A far signal toward the core is the hardest to pull out.
* **Two at once.** Above x1 the loudest other signal within forty degrees bleeds in under the one heard (`RadarState::under`).
  * You hear two broadcasts, one through the other, or a pulsar's thump under a voice. A people under another is heard as a
    machine of its age or the shape of a voice.
  * Two peoples' carriers beat into a whistle that slides as the beam turns.
  * At the top steps the two ring against each other, a metallic intermodulation.
  * From home at x5 the Ghutreig (AZ 253 EL -17) has the Visie 4.6 degrees from it.
* **Far voices worn.** Beyond 200 ly a signal comes worn by its distance (none at 200 ly, half at about 1,000, all at
  5,500): a narrower band, its pitch drifting, deep slow fades, dropouts, and three echoes of itself. A voice from 5,000
  years ago is the shape of a voice under the noise.
* **The static's events.**
  * C-07's seven (a carrier passing, a boom, a moan, knocking, a murmur, a breath, a scream) come oftener with the gain: every
    12-40 s at x1, 4-13 s at x32.
  * From x25 five stranger ones join: speech running backwards, counting whispered in no one's language, a breath close to
    the microphone, a knock answering a knock, and a choir swelling and stopping. The near ones are heard dry.
  * At x25 and x32, with nothing on the air, a word of a recording the explorer has heard (`Guide::heard`) comes back out of
    the static in its world's voice, as from very far. Never one not heard; none of it is logged.
* **False rises.** At x25 and x32 the noise makes rises where nothing is: a rise 8-30 degrees off the beam that lives ten to
  sixteen seconds. It has a smudge like a source's, and the static gathers into almost a tone there. Held, the hold fills
  part way and empties, the readout says `NOTHING THERE` and the rise fades. It never locks.
* **Each age on the air.** What a slot of a people's broadcast carries is by the age that people reached (`programmeOf`). The
  recording heard counts as heard when its words or its piece were on the air: everything but the beacons, the data, the
  machines and the mathematics.

  | Age | A text slot | A music slot | Under it |
  | --- | --- | --- | --- |
  | industrial | a phonograph, six in ten (the voice through a horn, over the record's hiss, crackle and tick); otherwise Morse by spark | the phonograph's piece, one in three; otherwise Morse | the spark's rasp |
  | modern | C-07's: the voice, whispers, a chant, a numbers station, a loop, the world's machines | the piece, slowed, three in ten; otherwise the machines | C-07's carrier |
  | futuristic | a synthetic voice, over half; data or a beacon | data or a beacon; the piece, one in five | a thin whine that flutters |
  | far future | a beacon of mathematics, three in four; otherwise a synthetic voice | the beacon of mathematics | a pure tone that never moves (1,420 Hz) |

  * The Morse is the text keyed in a code of the people's own: each letter one to four dots and dashes by the world's hash,
    after the station's call twice.
  * The beacon of mathematics sends pure pips, a group a term. By the world it counts the primes, the squares, Fibonacci's
    numbers, the powers of two or the triangular numbers, or sends the harmonic series as pitches, a count in binary, or the
    digits of pi.
  * The age test worlds' fifty slots:
    * the industrial: Morse 23, a phonograph 27;
    * the futuristic: a beacon 20, data 13, a synthetic voice 14, the piece 3;
    * the far future: the mathematics 44, a synthetic voice 6.
* **Where to listen from home.** `vesperis_test signals x32` prints each signal's AZ and EL (the radar camera's) and its
  people's age. A slot changes every 75 s, so what a world sends varies.

  | Gain | AZ, EL | What |
  | --- | --- | --- |
  | x1 | 056, +11 | the Otobrun (industrial, 197 ly): Morse or a phonograph |
  | x1 | 252, -13 | the Visie (futuristic, 184 ly): a synthetic voice, data or a beacon |
  | x5 | 253, -17 | the Ghutreig (modern, 90 ly) with the Visie under it: two broadcasts and the whistle |
  | x25 | 290, +34 | Nisyionila II (far future, 211 ly): the beacon of mathematics, clear |
  | x32 | 118, +01 | Phiostraisia I (far future, 4,500 ly) three degrees from the core: the hardest to pull out |
  | x32 | 275, +00 | Esamiala I (modern, 5,524 ly): a far voice worn |
  | x32 | EL +90 | an empty pole: the floor, the stranger events, false rises and a word heard before |

* **The picture.** With the gain:
  * the camera's static and snow thicken;
  * the band glows through the phosphor (the radio sky the receiver hears);
  * the star field ghosts a faint double;
  * the rolling band runs faster;
  * the picture tears now and then.
* **The harness.** `signals wav` adds the renders of C-16 with their rms and peaks (`08-testing-and-tools.md`):
  * the bed at three gains and toward a pole and the core;
  * two minutes of the top step's static;
  * a near voice and a far voice at three gains;
  * two signals mixed;
  * each age's programmes.
  All of them are within C-07's levels.

## The forms the renderer needs (A-02)

C-01's ruins were boxes and domes. The ages need far more, and A-02 is the renderer's half of it: the shapes, their broken
states, how they stand on the ground, how they block the explorer, how far off they are drawn and in what colours. Who builds
what is A-03's (the ages' buildings) and A-04's (the genome); where is T-01's and T-02's. Until A-03 the game's settlements are
C-01's and look exactly as they did (the nine civilisation scenes are pixel-identical); what A-02 adds is seen in the harness's
sheet, test city and skyline.

* **The forms** (`galaxy/forms.*`, `Form` replaces C-01's `RuinElem`: its `shape` became `kind`, and C-01's boxes and
  hemispheres keep their own arithmetic, so they draw to the bit). Twelve kinds, most with variants:
  * the box (C-01's walls, columns, slabs and blocks) and the dome (C-01's hemisphere, a saucer, a pointed dome, an onion);
  * turned forms: a cylinder, a frustum or a cone, a spire whose flank bows in to a needle, a cooling tower's waist (a
    hyperboloid), a tank with domed ends, a chimney with its lip; round, or a prism of three to eight flat faces (a square
    or an octagonal tower), on an oval plan if asked;
  * the pyramid and the stepped mass: smooth, a ziggurat's tiers, a setback tower's upright tiers;
  * vaults (round, pointed, parabolic) springing from walls or from the ground, open or closed at their ends (a hall, a
    hangar);
  * arches: a gate, a free arch, an arcade of any number of openings (an aqueduct, a viaduct), round, pointed or flat-headed;
  * thin shells: a saddle (a hyperbolic paraboloid) and a dish on its mast, aimed;
  * trusses: a lattice mast or pylon, a girder;
  * cables hung between two points;
  * rings: lying on the ground or standing on their rim;
  * sweeps: a section along an arc or a line (a curved wall, an amphitheatre's tiers, a battered dam or embankment);
  * grown forms: a lumpy body and its lobes.
  * plates (C-17): a flat body of a drawn outline (a superellipse with a taper, a waist, a lean and a dome, or a polygon), an instrument's
    body; its outline one function the mesh and the case's picture share (`formPlateOutline`).
* **Broken.** Every kind has its broken state, from a share (`broken`, 0 whole to 1) and a seed for its jags: a wall's top
  ragged in steps; a dome caved in, its rim jagged and its inside showing; a tower snapped, open, holed under the break (and
  leaning by `tilt`, which also lays a form on its side); a pyramid's top tiers gone and its corners worn back; a stretch of
  a vault fallen; an arch's keystone gone and a pier of an arcade snapped (the arches it carried with it); a shell holed and
  sagging; a mast bent over above its buckle with members gone, a girder sagging; a cable snapped, both ends hanging; a
  ring with a sector gone; a sweep with gaps and a ragged top; a grown form cracked open. What falls (the rubble, the
  fallen top of a tower) is a building's, A-03's.
* **Levels of detail.** Each building is drawn at its own level by its distance: whole within 350 m (a round form 12-28
  faces), simpler to 1.3 km (8-14), coarse to 4 km (6), and beyond that as its silhouette where it stands a pixel high: a
  flat outline across the line of sight with the form's own profile up its height (a dome's curve, a spire's taper, a
  setback's steps), at least a pixel of the framebuffer wide (the mush averages it into its neighbours), a lattice's
  outline half as wide. That is the skyline: a city of towers stands on the horizon as far as the air lets it be seen.
  C-01 drew a whole settlement at one level by the distance to its edge; a city two kilometres across needs each building's
  own.
* **On the ground.** A small piece stands on the ground at its middle, sunk a metre (C-01's rule). A big one (over 6 m
  across) stands on the highest ground under its footprint and reaches down to the lowest, so an aqueduct's deck stays
  level while its piers go down into the valley, and a block on a slope stands on its plinth. Near, the ground is the one
  drawn this frame; farther, the coarse ring's surface that draws the ground there, read once while that ring does (the
  64 m samples without the rivers that C-01 used for its far towns floated a mast 22 m over a wadi's floor).
* **Colliders** (`formColliders`): discs with the span of heights they stand over (B-405), C-01's rows along a wall kept:
  slices up a tower, a cone and a pyramid by their profile (prisms and pyramids along their outline), a dome's rim where it
  springs from the ground (a roof leaves it to its walls), an arch's and an arcade's piers with the openings free, a vault's
  flanks and closed ends with the hall free, a shell's low corners, a mast's legs, a ring's tube (standing, its foot), a
  sweep's two edges and its ends. A broken form's missing parts have none (a ring's lost sector, a vault's fallen stretch,
  a sweep's gaps). The unit walks the explorer into every sample, whole and broken: it touches each and never enters one,
  and goes through a gate, an arcade, a hall open at its ends, between masts and a lattice's legs and under a saddle.
* **A people's colours** (`FormPalette`, in `Culture::palette`, A-04's genome's since): four ramps, the structure, the accent,
  metal and glass, and light, in the surface's free banks (the first people's 18, 19, 20 and 24, the second's 25-28,
  `FORM_BANKS`). A slot can be the ground's own material: C-01's walls are the rock (or the sand of adobe), and their bank's
  ramp is that bank's, so they look as they did. The light's ramp glows whatever the sun. The striated rock's grain stays
  on walls cut from the rock; painted and built materials are smooth.
* **Windows** (`WindowSpec`, four a people) are a pattern on a face by the face's own metres (along it, up it), like the
  grain: a bay, a storey, a share of each, a shape (a rectangle, an arched head, a slit, round, a band of glass along the
  storey), how dark, lit or not, and from what height. A tower of thirty storeys costs what a blank one does. Where a bay
  spans fewer than two pixels the pattern fades to its mean, so a far tower does not shimmer. They go on boxes, turned
  forms, a setback's upright tiers and a sweep's outer face.
* **What it costs.** The forms are gathered building by building, built, lit and fogged in chunks on the workers, and drawn
  in bands of rows that every thread walks in the same order, so the frame is the serial loop's (`VESPERIS_THREADS=1` gives
  the same hash). The test city (1,348 buildings, 1,555 forms) costs about 2.5 ms of a frame at 2x from its edge.

Where to see it (the harness, nothing in the game until A-03):

| Mode | What |
| --- | --- |
| `forms sheet [scale]` | 33 samples, each whole and broken near, its middle and far meshes up close, and at 600 m and 2.5 km (magnified twice): `shots/tests/forms_<scale>x_<n>.png` |
| `forms city [scale] [desert]` | the test city from 300 m outside its edge on Phiyiesh II (desert): `forms_city_desert_<scale>x.png` |
| `forms skyline [scale] [desert\|felisian\|airless] [km]` | the test city's skyline from a summit, at 30 km by default |
| `stability skyline [scale]` | the skyline while walking toward it: the desert at 15 km, an airless world at 30 km |

The air decides how far a city is seen. The forms take the terrain's fog (one formula, so a tower and the hill beside it
fade alike), and on the peoples' worlds that is 9-12 km: at 15 km the test city is a pale cluster on a desert's horizon, at
30 km a ghost, where the airless world's 45 km shows its towers clearly. KI-373.

## What each age builds (A-03)

A people's settlements are built as its age built (`docs/ideas/IDEAS-civilisation-expansion.md`, section 2), from A-02's forms, and
weather as their materials do. `galaxy/buildings.*` holds the generators, the materials' decay and the rooms; `galaxy/ruins.*` lays
the buildings out in C-01's plans (T-02 laid towns by age since: below). Every civilised world's settlements change with it (the medieval
peoples' least: C-01's buildings are theirs, roofed); generation 13 (A-01's) holds, nothing else moved.

* **The kinds** (`BuildingKind`, a generator each, `buildingForms`), by the age that built them (`buildingAge`):
  * prehistoric: round huts of wattle on posts or of dry stone under a cone of thatch, with a hearth of four stones; longhouses of
    turf or planks under a ridge of thatch on two rows of posts; tent rings; stone circles (a lintel over a pair here and there, an
    altar stone, a bank round the big ones); standing stones alone or in rows, the tallest at the head; dolmens (a chamber of slabs
    under its capstone, the ring of its mound); palisades on their banks;
  * medieval: C-01's houses, halls, keeps (merlons, slits), rotundas, platforms, colonnades (an arcade for some), stelae, rubble and
    town walls, now roofed by the culture (a gable or a hipped roof of timber, or C-01's slab, or a stone barrel vault); the temple at a
    town's centre (a basilica with its tower and spire, a hall under a dome with its minarets, or a stepped mass with its shrine and
    a house at the stair's foot); round towers on the town wall; gatehouses (an arch between two towers);
  * industrial: terraced rows of brick with sash windows, a slate gable and stacks on the ridge; tenements; villas under hipped roofs;
    the church with its spire, the town hall with its clock tower, the station (a front block and its iron shed), the mill with its
    chimney, the gasometer, the water tower, the headframe with its engine house and spoil heap, the observatory;
  * modern: slabs of flats in bands of windows; suburban houses; towers of 100-500 m (a slab, setbacks, a cylinder or a prism, the
    people's form in three of five) over a lobby; the stadium; masts on their guys; the power station's cooling towers and stack;
    tanks and silos;
  * futuristic: the arcology (a stepped pyramid, a cone, or tapering towers grown together), the habitat dome of diamond glass on its
    ring, spires of a vertical town with their collars, the ring, the maglev line on its pylons, pods, a wreck fallen from orbit; a
    gate hall before each great structure;
  * the far future: one structure: a spike, a ring standing on its rim, a row of pillars, the mouth of a shaft, a solid (a cube, a
    tetrahedron, a sphere half sunk).

  C-08's features are built in the age's way where the age built them (a prehistoric people the cairns; an industrial one no
  cairns; a modern one its harbours and terraces in mass concrete; the futuristic and far-future ages none). The culture says how
  its age built (`Culture::roof`, `temple`, `hut`, `towerForm`, `great`: A-03's hashes, the genome's since A-04, below), and
  `Building::style` carries the choice into the building.
* **The plans by age** (`Layout`): C-01's plans hold the age's buildings, each place asking the age for a dwelling, a centre, a hall,
  a tower, a stela or a monument; the spacing and the radius scale with the age (0.85 a camp's, 1 a medieval town's, 1.85 an
  industrial town's, 2.4 a modern one's). A prehistoric people has no streets (camps, rings of huts round a stone circle or a
  longhouse, a "town" of two rings in a palisade); a medieval town's plaza holds its temple (C-01's platform, colonnade and stelae in
  one town of three); an industrial town has its station beside the town hall or the church and up to four mills and towers toward
  its edge; a modern town its towers in the middle, lower outward, and a stadium; a futuristic people lays out a few great structures
  (`SP_GREAT`: an arcology, two or three spires, a ring and a maglev line in a town of 300-450 m; a dome, a spire and pods in a
  village; pods in a hamlet); a lone monument is the age's (a stone circle, a dolmen, a row of stones; C-01's; an observatory, a
  gasometer, a water tower, a headframe; a mast, a power station, tanks, a tower; a ring, a wreck, a spire; the far future's one
  structure), drawn with the class's size so its reach is the radius the finders read without the layout.
* **How many** (`ruinOfCell`): the share of the 2 km cells holding a settlement follows the age (the lone monuments in the draw's first
  twentieth; B-417: on every people's world, the old ones' monoliths gone):

  | Age | Hamlets | Villages | Towns |
  | --- | --- | --- | --- |
  | Prehistoric (camps days apart) | 9% | 3% | 1% |
  | Medieval (C-01's) | 16% | 9% | 5% |
  | Industrial | 13% | 10% | 7% |
  | Modern | 10% | 10% | 10% |
  | Futuristic (a few great places) | 3% | 2% | 1.5% |
  | Far future | one structure in about seven hundred cells | | |

  A far-future world's "one to a few" structures and a modern people's megacities wait for the plan of a world (T-01).
* **The decay is the material's** (`materialDecay`: 0 whole, 1 gone, a log-logistic in the years; `buildingDecay` spreads it by the
  building's luck, C-01's quarter where the material is half gone, nothing where it is whole or gone):

  | Material | Half gone (a middling rain) | A tenth to nine tenths | The rain | At 100 / 1,000 / 10,000 / 100,000 years |
  | --- | --- | --- | --- | --- |
  | Timber, thatch, hide | 80 years | 1.2 decades | 0.8 decades faster wet | 0.56 / 1 / 1 / 1 |
  | Earth (mud brick, turf) | 400 | 2 | 1.6 | 0.26 / 0.66 / 1 / 1 |
  | Dry stone, megaliths | 20,000 | 2.5 | 0.4 | 0 / 0.08 / 0.40 / 0.72 |
  | Masonry, brick | 1,166 (C-08's `decayOf`) | 2.7 | 0.67 | 0.18 / 0.48 / 0.78 / 1 |
  | Iron, steel | 400 | 2 | 0.8 | 0.26 / 0.66 / 1 / 1 |
  | Reinforced concrete | 500 | 2 | 0.6 | 0.22 / 0.62 / 1 / 1 |
  | Mass concrete | 8,000 | 2 | 0.5 | 0 / 0.14 / 0.54 / 0.94 |
  | Glass | 5 | 1 | none | 1 / 1 / 1 / 1 |
  | Composite, ceramic, grown stone | 15,000 | 1.5 | 0.4 | 0 / 0 / 0.41 / 0.94 |
  | The far future's | never | | | 0 |

  A building reads each of its materials: a medieval house's walls stand for centuries while its timber roof is gone in one; an
  industrial row's brick stands roofless; a station's iron shed stands whole only while its glass would, then as its ribs, then
  nothing. Low walls stand to C-01's share; tall things (towers, chimneys, spires, masts, walls over 8 m) by a share that keeps them
  whole through their first decay and falls to a stump, their tops ragged; what fell of a tower lies across the ground away from the
  town's middle, broken, and a heap of rubble with tumbled blocks lies at its foot (the fallen top breaks up with its material; the
  heap stays). A roof, a dome, a lintel, a spire never outlast what holds them up. Posts rot to stumps and then to their holes (a
  dark ring of soil at the nearest level); dry stone leans, then lies where it fell; mounds and banks of earth last ten times a wall.
  The A-01 stopgap (the years over the age's durability) is the roads' and the sand's alone now; the buildings read the real years.
* **Rooms** (`buildingRoom`): every building with a room says where its floor is, its doorway (its midpoint and the wall's line) and
  the spots where a record can lie, the best first: a hut's and a rotunda's side opposite the doorway, a house's far corners, a
  hall's, a temple's and a church's nave, a tower's lobby, a station's front block, a mill's floor, an engine house, the gate hall
  before a great structure; the foot of a standing stone, a stela, a ring or a far-future structure in the open. C-03's shards lie
  there now (`shardSitesOf`, a spot 0.42 m clear of every collider of the building's pieces and 1.5 m from the doorway), and their
  place is the building's kind (`buildingPlaceName`: "A SHARD OF THE OLD PEOPLE TAKEN FROM A HUT", "... FROM THE FOOT OF A
  STANDING STONE"). T-04 lays the records' places by age in them.
* **Colours and windows** (A-03's `agePalette`; the genome's `culturePalette` since A-04, below): the prehistoric and medieval walls the ground's own rock (C-01's), timber and
  thatch, the roofs' tiles, slates, shingles or thatch (an onion of copper or gilt); brick or the dressed rock, slate and rusted iron;
  concrete, an accent, steel; composite, an accent, diamond glass; the far future's one material (black, white, bronze or blue-black)
  with its accent. The windows by the age: arched and slits, sash and tall arched and a mill's grid, bands and a curtain's empty frames
  and a grid, the futuristic age's thin bands and round lights; none before the medieval age nor in the far future. Earth (a mound, a
  bank, a spoil heap, a wreck's furrow: `Form::part` 8) draws in the ground's own soil: the grass on a living world, the sand on a
  desert.
* **The forest takes them back** (`forTrees`): no tree stands in a building; a settlement's yards keep a twentieth of their trees two
  centuries after the end, a third after ten thousand years (C-01's one in three), so a young ruin is a clearing.
* **Every age's buildings are generators**: `cultureAs(g, people, age, years)` builds a people's culture as if it had reached an age and
  ended so long ago, `settlementOf` lays out a settlement of a class in it, and a building keeps the age whose way built it
  (`Building::age`). B-414: a world's buildings are all its people's last age's (T-02's old cores and the old time's megaliths went with
  their ages: "One age a world", below); a work's pieces keep their work's.

Where to see it:

| Mode | What |
| --- | --- |
| `ruins ages [scale] [town\|village\|hamlet\|monument] [desert\|felisian] [wide\|near] [years ...]` | a settlement of each age (rows) at 100, 1,000, 10,000 and 100,000 years since the end (columns), from outside its edge (`near`: among its first dwellings): `shots/tests/ruins_ages_<class>[_near]_<scale>x.png` |
| `ruins kinds [scale] [years ...]` | every kind of building alone, a page an age, at 1, 300, 3,000 and 30,000 years since the end: `shots/tests/ruins_kinds_<age>_<scale>x.png` |
| `scene civilisation_<age>` | a settlement of each age on a people that ended lately (`AGE_SCENE_WORLDS`, found by `peoples fresh`), from a vantage on dry open ground with the light from the side |
| `ruins check` | the unit's four checks of the ages alone |

## The architecture genome (A-04)

A people's buildings are its own (`docs/ideas/IDEAS-civilisation-expansion.md`, section 3): every people draws thirty-five deep
variables (R-412: fifty-one, below) from its seed (`galaxy/genome.*`: `genomeOf`, one hash a variable, so a variable added later moves none of the others) and
keeps them through its ages, and its culture reads them through the age it reached (`cultureWays`, galaxy/buildings.h): a people that
loved domes builds them in dry stone, in masonry, in iron and glass, in concrete and in diamond glass, and a people of blue spires
raises one over its huts, its church and its tower block. Every civilised world's settlements change with it; generation 13 (A-01's)
holds. A variable is a number in [0, 1): a category its share of the range (`Genome::of`), a measure the number itself.

| Group | Variable (as printed) | Values | What it does | Ages |
| --- | --- | --- | --- | --- |
| Mass | MASSING | compact, sprawling, vertical, buried, stepped | the dwellings' footprints and heights (compact: smaller and taller, the town tighter; sprawling: wider, one storey, the town wider, the futuristic age's habitats spread; vertical: tower houses; buried: sunk 40% with their doorways kept; stepped: an upper storey set back on the walls, the towers' setbacks) | all but the far future |
| | PROPORTION | squat .. slender | every building's height (C-01's `tall`, 0.75 .. 1.3), the roofs' pitch, the standing stones, the domes' rise | all |
| | AMBITION | modest .. colossal | the centre's and the focal accent's size (0.75 .. 1.55, heights to 1.7), the lone monuments, the far future's structure | all |
| | HEIGHT | low .. lofty | how many towers and how tall (keeps, chimneys, spires, minarets, highrises, futuristic spires), the modern downtown's share of towers; C-01's `towers` over 0.2 | all but the far future |
| Form | PLAN | rectangle, circle, polygon, ellipse, free | the houses', huts', villas', blocks' and keeps' plan (a ring of the people's number of sides, an oval, an L), the towers' sections, the round temple, the arcology's sides, the far future's solid (a cube, a prism, an ellipsoid, a tetrahedron, a sphere) | all |
| | ROOF | flat, pitched, hipped, domed, conical, vaulted, saddle, stepped, open | every roof in its age's material (a corbelled beehive of dry stone, a turf mound, thatch, tiles, a masonry dome, an iron sheet, a concrete shell, diamond glass), the temple's choice, the towers' tops, the habitats' domes | all but the far future |
| | PROFILE | upright, battered, bulging, leaning out, stepped back | walls and rings battered on a thicker foot, bulging with rounded buttresses at the corners, an upper storey jettied out or set back (a plinth for a low wall), towers as frustums, barrels, corbelled tops or tiers, the futuristic spires' flanks | all but the far future |
| | SYMMETRY | bilateral, radial, rotational, none | doorways in the middle (bilateral, radial), the temple's and the church's towers (a pair at the front, one over the crossing, two at opposite corners, one at a corner), the minarets round a dome, the futuristic spires' placement | medieval to futuristic |
| | NUMBER | three .. nine | a polygon's sides, the stepped temple's and the setbacks' tiers, the platform's steps, the colonnade's columns, the stone circles' stones, the gasometer's columns, the cooling towers, the tanks, the grown arcology's towers, the spokes of a radial town, the far future's pillars and facets | all |
| | MODULES | one-offs, cells | a house and a row roofed cell by cell (a row of gables), crow steps on the gables, a mill's gables across it; a modern block's lift house a module (B-415) | medieval to modern |
| Surface | OPENINGS | square, arched, round, slit, triangular, keyhole | the windows' shape (`FacePattern` 0, 1, 3, 2 and A-04's 5 and 6; B-415: the industrial age's sash square or round-headed, the modern age's bands, portholes and slits on its temples, the futuristic age's portholes for arches and keyholes), the gates', arcades' and aqueducts' arches (round, pointed, flat; B-415: pointed in the hand-made ages alone), the domes' profile (B-415: an industrial people's hemispheres, a modern people's saucers) | medieval to futuristic |
| | RHYTHM | regular, paired, grouped, scattered | the windows' lights a bay (a pair, three or four split by mullions) or cells left blank by their hash (B-415: the industrial age's a pair at the most, never scattered) | medieval to futuristic |
| | GLAZING | few .. wide | the windows' share of the wall within its age's (B-415: a medieval window a third of a bay at the most, an industrial sash a third to a half, a modern pane half to the whole; bands in a modern people's frame) | medieval to futuristic |
| | ORNAMENT | bare, banded, carved, encrusted | bands of the accent at the walls' tops and storeys, a cornice, glyphs round every side, the door's surround, capitals | medieval to futuristic |
| | MOTIF | lines, circles, steps, spirals, glyph bands, figures, stars | on the great buildings: ribs up the walls, spheres on the corners, steps along the top, a ramp climbing round, glyph bands, statues at the front, the points of a star round the foot; the domes' onion profile (spirals, figures), the spires' collars | medieval to futuristic |
| | STRUCTURE | heavy masonry, expressed frame, smooth monolith | thick walls on the rock's striated grain (C-01's `style`), small windows; posts and beams on the walls (half timbering of daub between timber; B-415: an industrial people's brick piers between the windows, a modern people's concrete grid of columns and floor slabs; a futuristic exoskeleton); smooth walls, fewer windows | medieval to futuristic |
| | TOPS | finials, crenels, masts, statues, nothing | what tops the roofs, walls and towers (finials, spheres on a people of circles; merlons; poles; figures). B-415: crenels the hand-made ages' alone; an industrial people's everyday buildings topped by their chimneys, a modern people's by a mast or nothing; no statues after the modern age | medieval to futuristic |
| Colour | BASE | sixteen colours | the structure's colour: the ground's rock tinted toward it, or painted in it; the brick it fires (B-415: `BRICK`, one a colour, from a gault and a cream stock to reds, a blue, a dark engineering brick and a plum); its concrete, its composite, the far future's material at the colour's extreme | all |
| | ACCENT | seven colours on bands, roofs, doors or tops | slot 1's colour and the pieces that take it (bands always) | all |
| | FINISH | its own, painted | the structure the material's own (the rock tinted a quarter toward the base, concrete toward the base) or painted the base (B-415: an industrial people's brick, painted or not) | prehistoric, medieval, modern |
| | TINT | clear, bronze, green, blue, gold, smoke | metal and glass (slot 2) of the futuristic age: the frames, the domes of diamond glass (B-415: a modern ruin's glass is gone within years, its metal roofs went with its flat ones) | futuristic |
| | LIGHT | none, lamps, lines | a far-future structure's lights: glowing lines up it, lamps scattered on it (a dead city is dark) | far future |
| Place | LAND | ridges, valleys, terraced, cliffs, stilts, mounds, pits | a high place toward the ridge (a stepped platform and its shrine; B-415: an industrial people's bandstand, a modern people's plinth and mast), an aqueduct down from the high ground (a leat before the medieval age), terraces on any slope and a kerb round each dwelling, a cut in the rock round each, houses on stilts (the records under their floor; a modern people's blocks on pilotis), dwellings sunk in pits (B-416: the kerbs, the cuts and the pits the hand-made ages' alone, the mounds' earth banks and the pits' rims gone); T-01's plan sites the towns by it | prehistoric to modern |
| | FACING | sunrise, a star, the moon, the river, the wind, none | the settlement's axis (`orientedHeading`: east; a bearing of the people's own; the moon's rising; the ground's fall; the wind's; the cell's draw); the roads read it | all |
| | STREETS | grid, radial, concentric, organic, a line, contour, branching | the plan: C-01's grid; spokes from the centre (the people's number); rings; houses scattered round the centre; a street; a street across the fall; a street with side streets; T-02's street plans in each age's way (towns by age, below) | medieval to modern |
| | PUBLIC | plazas, courts, gardens, none | a plaza kept open round the centre; houses, villas and blocks round inward courts; walled gardens with their pools; none (denser) | medieval to modern |
| | GROUPING | compounds, rows, detached | houses in compounds of two or three round a court with low walls, in rows of three joined, or apart | prehistoric to modern |
| | EDGE | walls, ditches, nothing | the town's edge: walls with their towers and gates (a palisade before the medieval age), a bank and its ditch, nothing; towns and a village in three (T-02: an industrial town has none, its medieval core keeps its own) | prehistoric, medieval |
| | LINKS | arcades, bridges, tunnels, colonnades, none | arcaded fronts round the plaza and along the street, two towers joined by a bridge (keeps, brick towers, tower blocks, spires), tunnels' mouths, colonnades along the main street | medieval to futuristic |
| Meaning | FOCUS | spire, dome, gate, tower, colossus, mast, ring, obelisk, pool, great tree | the focal accent before the centre (`BK_FOCUS`, where the centre does not carry it), in its age's way (a menhir, a great mound, a trilithon, a broch, a carved stone, a pole; a lattice tower; a guyed mast); the temple's choice; the far future's structure (a spike, a halo, pillars, a shaft, a solid) | all |
| | CENTRE | temple, palace, assembly, market, tomb, machine | what stands at the centre: a stone circle, a great longhouse, a wood henge, an avenue of stones, a passage tomb, an alignment; a temple, a palace round its court with corner towers, an amphitheatre, a market hall, a stepped or domed tomb, a windmill; a church, a town hall, its domed assembly, an iron market hall, a mausoleum, a mill; a modern temple, the tallest tower, a stadium, a mall, a mausoleum, a power station; a spire, the arcology, a dome, a ring | all but the far future |
| | MONUMENTS | stelae, obelisks, colossi, arches, pillars | the monuments before the centre and on the plaza, and the lone monuments (standing stones, a trilithon before the medieval age) | prehistoric to modern |
| | THE DEAD | barrows, tombs, cemeteries, towers of silence, nothing | round C-12's stones: a barrow behind each, a small tomb, a low wall round the cemetery, a tower of silence (a ring of high wall), the stones alone | all but the far future |
| | HOUSES | alike .. varied | the dwellings' sizes' spread, another plan or roof now and then | all but the far future |
| | BUILT | light .. to last | its works' lifetimes over the age's materials' (`Culture::life`, half to twice), a light people's houses of timber, its huts of wattle (to last: dry stone) | prehistoric to modern |

* **The age's own** (`techniqueOf`): the technique a people used in an age, drawn among the age's by the deep variables' leanings:
  post and lintel (the prehistoric age's only), or arch and vault (domes, vaults, arched openings lean to it) in the medieval age; arch
  and vault or frame (iron) in the industrial; frame, cantilever (a modern people's towers stacked and slid off their core), tension
  (masts and cables) or shell (a tower's hyperbolic waist, a mall's shell) in the modern; shell, tension, printed (the stepped arcology)
  or grown (the grown arcology) in the futuristic. The materials, heights and spans are A-03's, scaled by the deep variables.
* **The palette** (`culturePalette`): slot 0 the structure, 1 the accent, 2 the roofing (thatch, tiles, slate, metal sheet) or the
  glass in its tint, 3 timber and iron (dark, rust) where the age has no light, the light's ramp in the futuristic and the far future's
  (`FormPalette::glow`); a slot of the ground's own is tinted toward its colour (`FormPalette::tint`). A piece's slot is what it is
  (`Gen::roofSlot`, `topSlot`, `doorSlot`, `timberSlot`): the accent goes where the people put it. A pool's water draws in the water's
  bank, a great tree in the bark's and the leaves' (`Form::part` 9, 10, 11).
* **The genome's works** (`BK_FOCUS` with its focus as its style, `BK_WINDMILL`, `BK_AQUEDUCT`, `BK_TWIN`, `BK_PORTAL`) and the kinds'
  new styles (a market hall and a palace of `BK_HALL`, a round temple, an amphitheatre of `BK_STADIUM` in stone, a town hall's dome or
  spire, an iron market hall, a mall, the monuments of `BK_STELA`), each with its room or the spots at its foot. Every building keeps
  what its room reads of the genome (`Building::form`, `sides`, `found`, `centred`), so a room is still known without the culture.
* **Found on the way**: C-01's rotundas and A-03's dry-stone huts, cisterns, observatories and stone circles laid their ring segments
  across the ring, as fins (and the stone circles' lintels radially); they lie along it now. A tower's or a roof's tops stand only on
  a roof that stands (a stone finial outlived its timber roof). A sunk house keeps its doorway 2.1 m over the ground outside.
* **The uniqueness is measured** (`genomeDistance`): a weighted distance on the strongest variables (the base colour 2, the roof 1.5,
  the plan and the massing 1.2, the accent, the proportion and the focus 1, the profile 0.8, the height, the openings and the structure
  0.6, the centre, the tops and the streets 0.5, the ornament 0.4: 13.4 at most; colours by a distance the eye weighs, measures a third
  of their range apart counting as different). Two peoples are alike under a quarter of it (`GENOME_ALIKE`, 3.4): sharing all the
  strongest variables but the weaker few. Of the hundred peoples nearest home (4,950 pairs) the nearest pair is 4.93 apart; of 200,000
  random pairs none is under 3.5 and five in a million under 4.0.

Where to see it:

| Mode | What |
| --- | --- |
| `genome sheet [scale] [years] [town\|village\|hamlet] [desert\|felisian]` | the architecture sheet, a page an age (`shots/tests/genome_<age>_<scale>x.png`): the twelve peoples nearest home as if each had reached the page's age, a town at the same cell under the same light 30 years after the end (unless given), its genome printed under it |
| `genome ages [scale] [peoples] [years]` | the same peoples (rows, six unless given) in every age (columns): does a people read as itself? (`genome_ages_<scale>x.png`) |
| `genome distinct [n]` | the distinctness of the n peoples nearest home (100): the pairs by distance, the nearest eight and what they share, the random pairs' tail, and the command for the nearest pair |
| `genome pair <sx> <sy> <sz> <body> <people>` (twice) | two peoples in every age, side by side (`genome_pair_<scale>x.png`, `scale=N`) |
| `genome print <sx> <sz> <body> [people]` | a people's genome and its techniques by age |
| `genome reach` | every variable's values set in turn in three peoples' genomes, in every age it acts in: which reach the drawing |
| `genome street [scale] [peoples] [years] [desert\|felisian] [drone]` | B-415: the same peoples (rows, six unless given) in every age but the far future's, each town seen from inside it (a street half way in, at a walker's height; `drone` 35 m up at its edge): does every age read as itself? (`genome_street[_drone]_<scale>x.png`) |
| `ruins kinds [scale]` | its last page (`ruins_kinds_genome_<scale>x.png`): the focal accents, the monuments, the windmill, the market halls, the palace, the round temple, the amphitheatre, the aqueduct, the joined towers, the tunnel, the town halls' dome and spire, the mall, a modern temple |

## A people's map of its world (T-01)

Until T-01 every 2 km cell of a civilised world was hashed on its own: a third of them held a settlement, the whole world over. A
people's world has a plan now (`galaxy/plan.*`, `docs/ideas/IDEAS-civilisation-expansion.md` section 5), built once a world on a
thread of its own while the ship comes in (`worldPlanPrefetch` at the approach, the parking, the landing map and a load; `worldPlanOf`
waits for it or builds it; four worlds kept). Generation 13 (A-01's) holds.

* **The reading** (`PlanGrid`): the planet function on a grid of half-degree rows, each row's cells as wide as it is tall (165,018
  cells, 33-65 km across on the peoples' worlds), at half a cell's detail without the rivers' tiles: the land of the people's time (no
  sea, lake or old sea), the height, the roughness, the vegetation, the climate, the fertility by the biome, the coast's distance,
  coal and ore (a field of the world's own at the mountains' feet and in them), the side of C-13's divide, and the rivers of a flow
  over the grid (every pit filled from the sea, each cell gathering the rain upstream: a dead desert's rivers ran into its seas).
* **The score** (`scoreOf`, by the age): water and good land always; the hills for defence in the medieval age; the rivers, the coal
  and the ore in the industrial; the coasts in the modern; a choice of the people's own in the futuristic and the far future's (a
  mountain to ring or a plain to make flat); the genome's relation to the land on top (KI-375's: a people of ridges on the ridges, of
  valleys by the rivers, of cliffs in the rough, of stilts by the water, of terraces on the hills); the climate (a world too hot or
  cold everywhere was the people's own) and the height under it.
* **The territory**: grown from the capital (the side's best ground, with a little of the people's own choice) over the land it could
  reach, the dearer the poorer, the rougher or across the water (the age's boats: a prehistoric people's coast is its end), until it
  holds its profile's share of the world's land (`PeopleExtent::held`, A-01's draw, now readable without the system); two peoples grow
  on their own sides of the divide and meet only where both reach it.
* **The places**, by rank (`PlanPlace`): the capital (`SC_CAPITAL`), the cities (`SC_CITY`) on the age's best ground (its top half; a
  modern people's top third: its coasts) and the towns (`SC_TOWN`) on any but the poorest, spaced as the age travelled (cities 260 km
  apart in the medieval age, 170 in the industrial, 140 in the modern; towns 70, 45, 45), their counts by the territory's size and
  capped (40, 80, 120 cities; 400, 700, 900 towns: a great territory's stand farther apart); a prehistoric people's gathering place
  and its bands' seats (`SC_VILLAGE`, 90 km apart, 40 at most); a futuristic people's capital, its great places (`SC_TOWN` in
  `SP_GREAT`, 500 km apart, 24) and the lesser (`SC_VILLAGE`, 250 km, 60); the far future's one to three works (`SC_MONUMENT`, 1,500 km
  apart). Each on the fine ground of its cell: dry where it stands and round its core (C-01's site test at 64 m and, for a capital or
  a city, two rings more), the gentlest of a lattice of spots, by the water where the age wanted it. Each has its rank in the founding
  (the capital first, then the near and the good before the far and the poor, a city before a town: H-02's order), its population at
  the height (the age's share of the people in its places, 5% to all of them, the capital the most) and the core the ground draws:
  a capital 110, 470, 820 and 1,150 m across its radius in the four ages to the modern (480 m a futuristic one), a city 270, 500 and
  700 m, holding C-01's town's density at the age's spacing (1,100 to 1,400 buildings in a capital, 350 to 550 in a city); a town,
  a seat, a great place and a work their layout's own draw.
* **The countryside** stays the cells' own (`planCountryside`): C-01's hamlets, villages and market towns, a lone monument in the draw's
  first twentieth (a desert's; B-417: every people's world's), by a density the plan gives each grid cell (the territory's ground, thinning toward the frontier, half a
  cell out beyond it) and the nearness of the people's places; at full density a medieval people's cells hold 16% hamlets, 10%
  villages and 2% towns (C-01's third, a little thinner: its towns are the plan's), the prehistoric 5% and 3%, the industrial 12, 10
  and 4, the modern 8, 8 and 4; a futuristic people's pods round its great places alone; the far future none. No settlement in the
  land no people held (round a medieval capital about one cell in six is settled), none within a core and 250 m round it but the city.
* **The networks** (`PlanEdge`): a greedy spanner over each place's eight nearest (the cheapest first, a link kept where the ways laid
  go round by more than the age's detour: 3.0 a prehistoric people's paths, 1.7, 1.6 and 1.5 to the modern, 2.2 the futuristic),
  dearer over water so a crossing is laid only where the land does not join; the components left apart joined by their cheapest pair;
  the trunk the network's routes between the capital and the cities along the spanning tree of their costs. The kinds: paths; roads
  (the trunk paved and broader); an industrial people's rail on its trunk and canals between two river places in flat country (two in
  five); a modern people's highways on its trunk; a futuristic people's maglev; none in the far future; a way over water a crossing
  (three or more of the grid's water in a row, or over half its length), not drawn. Every place reaches the capital.
* **The ways on the ground** (`planEdgeWay`, `roadLegWalk`): waypoints every 3 km on dry ground (the line's point or the nearest dry one
  across it, within 2.4 km), made when first asked and kept; the legs between them walked as C-09 walked its roads, by the kind's
  weights (a path wanders and climbs, rail and a highway keep their grades low and turn wide, a canal keeps level), drawn into the
  terrain by their kind (C-09's paving or beaten earth, a railway's dark ballast, a canal's silted bed, a highway's pale slabs, a path's
  worn line; `SiteRoad::kind`, `netHalfWidth`), worn as C-09's are. The HUD names the kind underfoot (`OLD RAILWAY NE-SW`); a maglev
  runs on its pylons, A-03's spans of about 150 m in every cell it crosses (with its settlement, or a cell of works alone, `SC_WORKS`:
  no shards, no graves).
* **The cells read the plan** (`ruinOfCell`): a place's own 2 km cell holds it (its layout from C-01's cell seed: a capital or a city
  `SC_CAPITAL`/`SC_CITY` in the town's plans at the plan's radius and count, a people of the line's on a grid, the gathering place in
  rings; an industrial or modern city's middle built up, its tenements, blocks and towers by the people's appetite for height; the wall
  in stretches of a town's); the rest the countryside's. The view reaches a core from every cell it spans and draws the plan's places to
  24 km (`SurfaceView::planNear`, `PLAN_NEAR_M`): a far city's towers on the skyline; the colliders, the rocks, the flora and the shards
  read the core wherever the explorer stands in it. A landing inside a core is moved out past its edge.
* **What is seen from orbit is what is walked on.** The landing zoom draws the plan's ways by their kind and the places' cores as
  discs of built ground (a core's share of a texel, less what the years took); the telescope's plate draws the same over its cells
  (the cores' built ground; the ways where a cell is under 300 m); the whole-world map shows nothing at its scale (a core is a
  thousandth of a texel). The scanner hears the plan's places by their size (a capital from 400 km, a city from 200, a town of the
  plan from 80, a band's seat from 40, a futuristic great place from 300, a far-future work from 1,500: `A CITY`, `A GREAT CITY`).
* **Found on the way.** C-09's blocks of cells stepped by the same longitude index in every row, but rows have their own steps: at 50
  degrees and an index of 5,000 the next row's cell of that index lies two and a half cells off, so a hamlet's neighbour 1.8 km away
  was outside its block (`ruinCellLonIn` now: the roads' blocks, `roadsNear`, the site's road growth, the landing's clearance).
* **What moved.** Every civilised world's settlements (where they stand, how many, the cities), its roads (the plan's ways, and C-09's
  lanes in the countryside only), its shards' and graves' places, the civilisation scenes (from a town of the plan; the ages' from the
  capital). C-01's checks read the countryside round a town of the plan; C-13's frontier is asked for only where the two territories
  meet (Eleinewai I's do not: a prehistoric people's and a medieval people's lands far apart; R-413: two prehistoric peoples' now).

Where to see it:

| Mode | What |
| --- | --- |
| `plan <sx> <sz> <body> [second]` | the world's plan over its map for each age (the people as if it had reached it, over one reading of the world: `shots/tests/plan_<world>_<age>.png`, the six on `plan_<world>_ages.png`), the territory tinted and outlined, the capital white, the cities red, the towns yellow, the ways by kind (paths brown, roads pale, rail red, canals blue, highways orange, maglev magenta, crossings dotted); its places, ways, held share, people and the capital's site printed, each plan's time; then the plan the game builds on one thread |
| `plan ages` | the six age worlds' plans at their own ages |
| `plan view <sx> <sz> <body> [second]` | the people's capital through the telescope from above it under a high sun at x64, x256 and x1024 (`plan_view_<world>_x<power>.png`, `_map.png` the map alone) |
| `zoom <sx> <sz> <body> <latDeg> <lonDeg> [scale]` | the landing zoom: the cores and the ways with C-09's lanes |
| `scene civilisation_<age>` | the capital of the age's scene world |

## The history (H-02)

A people had a profile (A-01) and a map of its world (T-01), but nothing happened in its years. Now each people has a history
(`galaxy/history.*`, `docs/ideas/IDEAS-civilisation-expansion.md` section 8), simulated before its towns, its works and its records so
they can read it (Dwarf Fortress's order: the world's history first, then the sites and the legends that tell it), to the data model of
`docs/ideas/RESEARCH-records.md` section 6. Deterministic from the profile, the plan and the world; a year at a time from the capital's
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
* **What it gives the later milestones**: the events of a year (`eventsIn`), a person's and a place's (`eventsOf`, `eventsAt`), the
  reign of a year (`reignAt`), its harvest and the people's number (`harvestAt`, `populationAt`), the inventions made by then
  (`inventionYear`), the seat (`historySeat`), the sentences of a legends print (`historyEventLine`).

Where to see it:

| Mode | What |
| --- | --- |
| `history <sx> <sz> <body> [second]` | a people's history as Dwarf Fortress's legends: the head (the span, the height, the end, what was told by kind, the build time), the eras, the rulers by house, the places, the wars, the sicknesses, the inventions and the institutions, the figures, the families, one family's years, the chronicle, the collapse to the last person (`shots/tests/history_<world>[_2].txt`; the screen leaves out the chronicle and the towns) |
| `history ages` | the six age worlds' peoples |
| `history check` | the unit's H-02 checks alone |

## Towns by age (T-02)

A settlement used to be C-01's plans (a cluster, a street, rings, a grid of 17 by 15 m blocks) scaled by the age, its streets the gaps
between its houses. A village, a town and a city are laid out now as their ages laid them (`galaxy/towns.*` and `layoutTown` in
`galaxy/ruins.cpp`, `docs/ideas/IDEAS-civilisation-expansion.md` section 6): streets first, then the buildings on them, by district, in
layers by the history's eras. Every civilised world's villages, towns and cities change (and with them the shards' and graves' places);
generation 13 holds. A camp and a hamlet keep C-01's and A-03's plans; a futuristic people keeps its few great places in the land left wild;
the far future has no towns.

* **The streets** (`Street`: a segment, its half width, its rank: a lane, a street, a main street, an avenue, a ring road; the age that
  laid it; paved or beaten), in the people's pattern (`GV_STREETS`), each age in its own way:

  | Pattern | Medieval (crooked, narrow) | Industrial (straight) | Modern (blocks of about 100 m) |
  | --- | --- | --- | --- |
  | grid | a grid of uneven lines, some broken off | a grid with a back lane down each block | a downtown grid |
  | radial | spokes and rings, more spokes outward | spokes and ring boulevards | spokes and rings |
  | concentric | rings close together, joined from ring to ring | the same, straight | the same |
  | organic | wandering main ways from the square, lanes off them | quarters, each its grid at its own angle | a grid at an angle |
  | a line, the contour | one main street with lanes behind (a city of the line: streets side by side, crossed rarely; on the contour, stairs between them) | long terraced streets | the same |
  | branching | a main street with side streets ending short | the same, the side streets' own branches | the same |

  A modern town's ring road (a city's on its older rings' edge) and beyond it the suburbs' collector streets and cul-de-sacs, each with its
  turning circle. The main ways out run from the centre's square along the axis and toward the plan's places the ways go to. A village is a
  street or two, a green, or lanes. A medieval layer's lesser streets end inside its wall, a street runs round inside it, the main streets
  go out by its gates. The streets end before the water (and the harbour's room); what a clip cut off from the rest is dropped (the web is
  one). The blocks widen where a town's buildings could not line its streets (a city keeps its class's count: the frame's budget).
* **The layers** (`TownLayer`): a place of the plan reads its history (`historyOfWorld`: the world's system found again from its star's
  sector). B-414: one layer, its people's last age, whatever age founded it (T-02 drew the ages from its founding to its end, a ring each:
  a modern capital's medieval core to 470 m, its industrial ring to 820, its modern to 1,150); a place the history left before that age
  began is not on the ground (`TownHist::gone`), one left early within it is built weathered from when it was left. The countryside's
  villages, towns and lone monuments are their people's age (T-02 gave a later people's an older core by the history's shares, and its
  lone monuments and its capital's fields the megaliths of its old time: gone with that age).
* **The centre and the districts** (`District`, on each building): the centre's square with its centre in the oldest layer's way (B-414: the
  town's one), its monuments and its focal accent; the old core (none since B-414); the houses, built up densest near the centre and
  along the main streets (a medieval town's main streets a row of joined houses whatever the people's grouping); the works downwind (an
  industrial ring's mills, warehouses, water towers, gasworks and workers' rows; a modern one's tanks, works halls and masts); the harbour
  toward the water; a modern city's towers round its old core and its suburbs beyond its ring road, thinning into the fields; the station on
  a main street of an industrial ring; a medieval town's market hall by the square; a modern city's stadium and power station beyond its
  edge. The houses' grouping is the people's (rows joined along the street, compounds round a court, detached), their heights falling from
  a city's middle.
* **The edge**: a medieval layer's wall with its towers and its gates where the main streets go through (a bank and its ditch for a people
  of ditches); an industrial town has none (its old core keeps its own); a modern city its ring road and suburbs; nothing.
* **C-08's reading of the site by age**: the harbour a quay along the shore and its mole (medieval), a dock between two piers with its jib
  cranes and its warehouse (industrial: `BK_CRANE` style 0), a container port with its gantries and stacks of containers (modern: style 1,
  `BK_CONTAINERS`), a city's berths side by side along its shore; terraces on a slope; a desert's cistern and wind wall (medieval), a
  reservoir upslope (industrial: a basin of masonry in its bank; modern: a round tank of concrete; `BK_RESERVOIR`), on a living world a
  town's on a slope; the fields round it (`FieldPatch`): a camp's plots, a medieval town's strips in furlongs (walled fields on a desert),
  the industrial age's enclosures, a modern people's great fields and, on flat dry ground, its irrigation circles, terraced fields below a
  town on a slope.
* **In the ground**: a town's streets, squares and fields are drawn into the terrain (`SurfaceSite::addTownMarks`, `applyMarks`) on the 4,
  16 and 64 m rings as the town comes within two cells: the paving (setts, a modern street's asphalt, a lane's beaten earth) and on the air
  the streets' tone, a field's strips or tone and its edge (a hedge on a living world, a wall or a bank on a dry one), an irrigation
  circle's darker soil, faded by the years; the trees and the rocks keep off the streets.
* **Kept off the streets**: every building is tested against every street's corridor as its rectangle and, where its pieces reach past it,
  by its own colliders (set back by as much once, else not built); the unit checks the streets walkable along every line.

Where to see it:

| Mode | What |
| --- | --- |
| `towns <sx> <sz> <body> [second]` | a people's capital, its first city, its oldest town, a place it left early, a town and a village of its countryside: each printed (its layers, its buildings by district and age, its streets by rank and how joined, its fields, its works, the time to lay it out) and drawn from above (`shots/tests/towns_<world>_<what>.png`: the streets by rank, the squares, the layers' rings, the buildings by district, the wall red, the fields) |
| `towns ages` | the six age worlds' |
| `towns view [<sx> <sz> <body> [second]] [scale=N]` | the ages' scene worlds' capitals through the renderer: from the air over the edge, low over the old core, along a street of the middle (`towns_view_<world>_<air\|core\|street>.png`); then the site's reading by age, a harbour, a reservoir or cistern and the fields of each age from the air (`towns_feature_<age>_<what>.png`) |
| `towns check` | the unit's T-02 checks alone |
| `scene civilisation_<age>` | the capital of the age's scene world, from its edge |

## The works (T-03)

Everything else a people made outside its towns (`galaxy/works.*`, the expansion note's section 6: "it can be virtually anything"), each
placed by a rule of the terrain or where the history (H-02) says it was made, and built in the way of the age that made it (`Work::age`,
weathered from when it was left, `before`). A world's works are made once (`worldWorksOf`, the last four kept) on the approach's plan thread
after the peoples' histories, in 44 to 350 ms; the surface takes them when they are built (a render never waits). Twenty-two kinds (B-418:
the fort); generation 13 holds (the works are a layer over the ruin cells: no settlement, shard, grave or road moved). B-418 made a people's
everyday works its land's (the land's works, below): the rules marked "the land's" place one in a cell of 8 km where its ground offers it.

| Work | Where its rule puts it | Its age's way |
| --- | --- | --- |
| quarry | the land's: where a cell's rock shows, a face where the hill stands steep, a pit in flat rock (a stone-built people's); a medieval or later people's pit on any dry ground, the forest grown over it since (clay, marl, gravel, sand: an adobe people's only these); a prehistoric people's flint pits on any ground but snow | flint pits and their spoil rings (prehistoric); a face or a stone pit of three-metre steps, cut blocks, a timber derrick, a hut (medieval); benches, an iron derrick, a shed and a spoil tip (industrial); great benches or an open pit, its crusher's hall, tailings and trucks (modern) |
| mine | the land's where the plan read coal and ore; the history's by the places it sank one | adits along a hillside, their spoil tipped below (medieval); headframes with their engine houses and stacks, a great tip, the miners' houses (industrial); an open pit in benches, its headframe, mill, tailings and trucks (modern) |
| dam | where the history built one (a river town's): the narrowest cross-section of a valley near the town or up its river, its crest at the lower shoulder (the age's height at most), never in a basin the drainage fills; the reservoir behind it as high as it holds | a masonry gravity dam and its buttresses (industrial); a concrete gravity dam with its spillway, intake tower and power house, or an arch bowed into the reservoir (modern); a wall of composite, its light along the crest (futuristic). Breached when its material is half gone, the reservoir drained to its silt; a dead desert's an old pale bed |
| aqueduct | where the history built one: from a spring on higher ground 3 to 13 km off to its town's edge, the channel falling all the way, on arches where the ground falls under it | arcades of one to three tiers standing to the channel's line, the spring's house, the castellum; its conduit's line across the ground |
| observatory | the highest summit within 25 km of the town that built one | a platform, a tower and its ring of horizon stones (medieval); domes (industrial); dishes and a dome (modern); a ring of dishes round a dome (futuristic) |
| shrine, the pilgrims' way | the people's holy place (`PeopleProfile::holy`): a summit, a spring's hollow, a cliff (a cave's mouth), a grove (in its clearing), the open sky; a beaten track from its town with a cairn or a stela every 600 m. The land's: the lesser shrines where a cell's ground is its holy place's kind (no way to them) | stones and a cairn, a temple, a chapel, a monument |
| necropolis | the land's: the slope that looks at the greatest settlement in or round its cell (a village or more; a prehistoric people's any) across the low ground (its river's where it has one); the genome's way with its dead | barrows and a long barrow (a people of barrows, the old time), tombs cut into the slope over houses of the dead and rows of graves (a people of tombs), a walled garden of graves with its tombs, obelisks and chapel (industrial), a grid of graves and a columbarium (modern), a field of memorial pillars (futuristic), towers of silence |
| lines on a plain | the flattest dry plains of a prehistoric or medieval people's land | a creature drawn in cleared bands ten metres wide, rays from a cairn, cleared trapezoids: read from the air (the drone) |
| henge, standing stones | the land's: open ground on a slight rise wherever a prehistoric people lived | a circle in its ditch and bank with its avenue, rows of standing stones, a great circle with its trilithons, a field of dolmens and barrows |
| fortified line | along the frontier with the other people, or the border nearest the capital facing land not held; in stretches of 2.5 km | a wall with its towers and forts (medieval); a ditch, a rampart and redoubts (industrial); an anti-tank ditch, dragon's teeth, pillboxes and a blockhouse (modern) |
| battlefield | each war's greatest battle (two of a great war): between the battle's place and the place the other side came from, on the line between them, on open ground | cairns (prehistoric); barrows and a memorial stone (medieval); trenches facing each other, shell craters, an obelisk and war graves (industrial); craters, wrecked tanks, an aircraft, pillboxes, war graves (modern); glassed craters and fallen machines (futuristic); its dead's graves all of the battle's year |
| launch site | where the history built one: the flattest dry ground of the land nearest the equator, the sea to the east | pads with their towers and aprons, the crawlerways, the assembly hall, tanks, a blockhouse (modern); a mass driver rising east (futuristic); an elevator's anchor and its ribbon to the sky |
| antenna field | a radio people's high plain away from its towns | a row of tall masts (industrial); a field of dishes or of masts (modern); a ring of dishes (futuristic) |
| wreck | the land's: a ship on a cell's shore; a modern people's aircraft on open flat ground; a futuristic people's wreckage fallen from orbit in its crater | a hull's ribs and keel, an iron steamer listing, a freighter broken in two; an aircraft; a hull in its furrow |
| colossus | a people of colossi (the genome's focus or monuments): its capital's harbour mouth, else the height over it | its figure on its plinth, an arm raised; fallen along the ground, its head off |
| terraformed basin | a low basin of a futuristic people's land | the ground levelled in its rim, channels across it, pumping stations |
| the last shelter | the steep rock nearest the place the collapse ended in (H-02's last place) | a rock shelter's mouth (prehistoric), a citadel on the height (medieval), a casemate (industrial), a bunker's door with its vents, masts and guardhouse (modern), a vault under its dome (futuristic) |
| something of the far future | anywhere on its land, two to four | a plain made flat and glassy, pillars to the horizon, a mountain cut into terraces, a lattice over a valley, a sphere on its point |
| wind farm | the land's: a modern people's cell's highest ridge | rows of turbines into the wind |
| lighthouse | where the history lit one: the headland nearest its harbour; the land's on a cell's headland (the water round a third of it at 400 m) by a settlement | its tower and lantern, the keeper's house |
| fort | the land's (B-418): the cell's height, a crest it can stand on (a prehistoric people's on a hill; a later people's anywhere, likelier on a hill), its gate toward the settlement it guarded | a hill fort's one to three rings of bank in their ditches round the huts of its last people (prehistoric); a castle of five to eight sides, a tower at each corner, the gatehouse, the keep, behind a moat on the flat (medieval); a redoubt, its ring bank round a casemate in its ditch (industrial); pillboxes round a blockhouse, dragon's teeth across the way up (modern); its towers and walls in the people's genome |

* **The pieces** are buildings of the works' kinds (`BK_MOUND`, `BK_DAM`, `BK_DISH`, `BK_TOMB`, `BK_FORT`, `BK_BUNKER`, `BK_TEETH`,
  `BK_VEHICLE`, `BK_AIRCRAFT`, `BK_SHIP`, `BK_LAUNCHPAD`, `BK_HANGAR`, `BK_COLOSSUS`, `BK_TURBINE`, `BK_LIGHTHOUSE`, `BK_LATTICE`, `BK_DERRICK`,
  `BK_BLOCKS`, `BK_SPHERE`, `BK_TETHER`, `BK_TRILITHON`, `BK_CONVEYOR`) and of the ages' (a headframe, a mast, a stone circle, a portal),
  laid out by `workSpec` as a settlement's (`RuinSpec::work`, `sclass` `SC_WORKS`) and drawn, decayed and walked into as one. A dam's wall,
  an aqueduct's arches and the far future's lattice stand at heights over the reference level (`Building::base`, `Form::absolute`): they
  span their valley whatever the ground under each piece does. A necropolis's and a battlefield's graves are C-12's stones in their plots,
  named, found and logged like a town's.
* **The ground** (`GroundEdit`, drawn by `SurfaceSite::addWorkEdits` and `applyEdits` as the work comes within 6 km): a face's floor and
  its benches, a pit's, a ditch silting up with its bank, a crater filling (scorched, or glassed), a plain made flat (paved, glassy, or a
  basin levelled), the reservoir's water at its level over the cells it filled (or its old bed), lines (cleared and paler, beaten and darker,
  paved), a mountain's terraces; the trees and the rocks keep off a fresh cut, a line and a plain made flat; the landing capsule is set
  down outside a compact work.
* **Heard and marked**: the scanner's WORKS mode, after RUINS (`worksHeardNear`): the nearest work's kind (`A DAM`, `A BATTLEFIELD`,
  `LINES ON THE GROUND`, `SOMETHING MADE`); a line's lesser stretches and a pilgrims' way are not heard. A mark's kind is `LM_WORK` (`%` on the
  maps); the guide's file keeps R-408's numbers (8 a place, 9 a work).

Where to see it:

| Mode | What |
| --- | --- |
| `works <sx> <sz> <body>` | a world's works: each kind's count by people, what its history finished, and every work's line (its age, its place, its rule's numbers: a dam's height, length and reservoir; a quarry's face or pit; a battlefield's war and year); B-418: the land's round each people's first ten places and at forty points of its land, and what the scanner hears from those points |
| `works ages` | the six age worlds' and the six scene worlds' |
| `works gallery [scale=N]` | each kind of work of each age's scene world through the renderer, where its rule put it (`shots/tests/works_<age>_<kind>.png`, a sheet `works_<age>_sheet.png`) |
| `works map` | the same works on insets of the ground with the place each serves (`works_map_<age>.png`: the terrain, the cores, the ways, its reach, a battlefield's line between its sides, the rule in a line) |
| `works view <sx> <sz> <body> <kind's word> [index]` | one work through the renderer (`WORK_D`, `WORK_H`, `WORK_A` set the camera) and its pieces printed; the world's then the land's, a kind's whole name matched whole (`"a fort"`) |
| `works check` | the unit's T-03 checks alone (B-418: and the land's, and the scanner's from the peoples' land) |

## Where the records lie (T-04)

A people left between 80 and 200 records (`recordCountOf`, `galaxy/records.*`, the expansion note's section 7), by its age and by how many
they were at their height: the literate and the many left more (the age worlds: the prehistoric Taarkhaur 86, the medieval Othleas 101,
the industrial Xoocu 135, the modern Veathiash 143, the futuristic Balaxiorn 175, the far-future Xoguskax 178). The count is fixed here,
before H-04 writes the texts: C-02's fifty are the first fifty and keep their texts, their music and their charts; the rest are drawn the
same way, music and star charts at the people's own shares of the fifty (H-05 sorts the three categories by the age). `Lore::records`
carries it: the decoder's lists, the taking and the radar's world read it (`shardsOfWorld` both peoples').

* **Dated in the history** (`recordsOf`, a catalogue a people, the last eight kept). Each record is dated in the people's own count
  (`recordYear`). B-414: in the years of the age the people reached (`ageFromOf` to the end), weighed by what the years left (e every 1,500
  years before the end); T-04 spread them over the eras by what each age wrote a year, so a modern people's records were a fifth medieval
  books and scrolls, a fifth industrial print and wax. Its earlier ages are told by its own histories (`genre.history`, "One age a world",
  below). C-02's draw of a shard's share of the years is the record's share, so the order is C-02's and the last recording is the end's year. The shards' years
  (`Shard::year`, the `{year}` in a text, the decoder's dates and timeline, the graves' years: the lore's span is the end's year less one)
  are the history's now: A-01 had kept C-02's span of 150 to 600 years until H-04, but the medium follows the date.
* **Made in its year's way** (`RecordMedium`, `mediaOf`; B-414: the age reached's): carved bone (a tally, a flute), carved stone and a painted wall (prehistoric); clay
  tablets early, scrolls, codices late, carved stones, an engraved plate for a chart (medieval); printed pages, codices, wax cylinders (a
  voice, a piece), a glass plate of the sky (industrial); printed pages and reels early, discs, drives late, a glass plate (modern); drives
  and crystals (futuristic); crystals and something that hums (the far future).
* **Each its home** (`recordHomesOf`): a place of the people's plan standing in the record's year, drawn by its size (the capital and a far
  future structure ten, a city four, a town one and a half, a village one): where it was kept. The home always holds it; copies lie
  anywhere else (C-03's rule kept: a copy taken is gone everywhere), so every record lies somewhere on the world: the medieval Othleas'
  101 at 1,656 sites of their plan's places, works and the countryside round their capital (the commonest at 27), the far future's 178
  at its two structures.
* **Where by age** (`recordSitesOf`): a settlement holds C-03's few by its class (one a building, in the buildings an age kept its writings
  in: a chief's longhouse, a stone chamber, a temple, a hall, a church, a town hall, a station, offices, a station's hut at a mast's foot,
  a gate hall, a structure's foot, and the houses), a place of the plan its own records on top; the capital an archive of 8 to 14 and
  each city one of 5 to 8, some towns (one in three) one of 3 to 5 (a prehistoric capital 4 to 6 in its chief's longhouse): a row of
  records across the middle of the room its age kept them in (a temple's, a palace's hall, a town hall, a tower's ground floor, a spire's
  gate hall), the place's own first; the far future's structures keep theirs in rings round their feet (the archive that keeps itself).
  The works that kept records hold a few (an observatory its charts, a necropolis its dead's, a shrine, standing stones, an antenna
  field's mast, a launch site's blockhouse, a lighthouse). A place left early holds nothing written after it was left; the old time's
  stones, left before year 1, nothing.
* **The last recording** lies where the history ended: at its last place, in the building its age would keep its last words in (a chief's
  longhouse, the temple or the palace's hall, the town hall or a station, the capital's mast, a spire's gate hall, the far future's
  structure), and in the last shelter by it (T-03); nowhere else.
* **A painted wall** is painted on the face of the room's wall away from its doorway (`paint`: the first wall box the line meets, the
  painting centred on that face and leaning with it): creatures, hands, a sky of points joined by lines, in ochre and white; recorded
  (`E`), it stays painted. Where no wall stands it is painted on a slab on the ground.

In the view (`06-surface.md`) each record is its object, drawn with the settlement's forms within 40 m (`gatherRecords`, the records' banks
29 to 31, clay the cut wood's 17, metal the chrome's 7, a stone and a sleeve in the people's colours, a crystal and a sphere in its light),
with C-03's point of light over it to 250 m; the HUD names it (`A CODEX - E`, `A PAINTED WALL - E`, a row over the sun's line), the taking
says what and where (`A CODEX OF THE OLD PEOPLE - 3 OF 147 ON THIS WORLD`; the log: `... TAKEN FROM AN ARCHIVE IN A TEMPLE IN THE RUINS OF A
CITY ON <WORLD>`, a painted wall `RECORDED`, a work's `AT A SHELTER`).

*The guide across the change.* A people's records are at `people x 1000 + index` in the world's indices (`shardWorldIndex`; C-13's second
people was at 50 + index): a guide saved before (no `records 2` line) is moved when it is loaded (`Guide::load`: the shards, decoded, heard,
lent, and the pieces' names), so the shards found before generation 13 are kept by their index, the counts and the language kept, the
contents the records' now; the game says once `RECORDS REGENERATED - 23 SHARDS KEPT, CONTENTS NEW` and logs it, and saves the guide.

Where to see it:

| Mode | What |
| --- | --- |
| `records <sx> <sz> <body> [second]` | a people's records: the count, by the age of their years and by medium, the last recording and the last place; where they lie over the plan's places, the works and the countryside round the capital (archives, painted walls, by place), the copies, and the sites of the capital, the first cities, a place left early, the last place, the works and a few villages |
| `records ages` | the six age worlds' |
| `records scenes [age]` | through the game, landed by each age's scene world's capital: an archive and each common medium's object, a frame before it, `E`, the frame after (`shots/tests/records_<age>_*.png`, a sheet `records_<age>_sheet.png`) |
| `records check` | the unit's T-04 checks alone |

## The grammar engine (H-03)

The records are written by a small attribute grammar over a realiser (`galaxy/grammar.*`; the formalism H-01 chose,
`docs/ideas/RESEARCH-records.md` sections 4 and 5). The grammar is plain text files with the extension `.vg` in
`src/galaxy/grammar/`, compiled into the game and read from disk by the harness; the engine draws from them with the record's
model (its facts and its persons), and the realiser writes the English. C-02's kinds were rebuilt on it first (below), from the
same inputs, before H-04 writes new content.

*The files.* Each file is wrapped once in a raw string, so a C++ source includes it where a string goes, with no build step and
no generated header (Make's `-MMD` and CMake's scanning both see the include, so editing a grammar file rebuilds what reads it):

```
R"VG(
# a comment
...the rules...
)VG"
```

```
static const char* const SRC_SHARDS =
#include "grammar/shards.vg"
;
```

The loader skips the wrapper's two lines, so a file read from disk and its compiled-in copy parse alike and give the same line
numbers. A file is kept under 16 KB (MSVC's limit on a string literal; the release builds use clang). `shardGrammarFromDisk`
replaces the compiled-in grammar with the tree's files (the harness: an edited rule is reprinted without a rebuild, the texts
built again); the unit checks the tree's files are the text compiled in.

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
the water by the type and the span, the crops, trees and beasts, the seasons and weather; the counts) and `lexicon.vg`. The
shard model (`galaxy/shards.cpp`) sets the world's facts (the names, the type and climate, the sky, the traits), the record's
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
| `grammar` | the grammar read from `src/galaxy/grammar/` (`dir D` another, `embedded` the compiled-in), its errors; every people's texts of the test worlds, the age worlds and the 40 nearest home (`near N`) through every check, each world's line (records, texts, words, the time, its repeated frames), the failures with their texts, the measures and the most repeated frames, the rule use (the most drawn and the never drawn by file and line, the kinds told), the cost; every text in `shots/tests/grammar_texts.txt`, two a people on the screen (`print N`, `print all`) |
| `grammar check` | the unit's H-03 checks alone (the engine's cases, the files compiled in, the test worlds' texts) |

## The records (H-04)

H-04 writes each people's records from its history (`galaxy/genres.*`, the genres in `src/galaxy/grammar/*.vg`). T-04 fixed how
many there are, the year of each, the medium it was made in and where it lies; the engine above draws each record's genre and text
from the record model, and C-02's kinds are gone from the text shards (the star charts' captions and the music keep C-02's draws).

*The genres by age.* The root (`records.vg`, `record (steady)`) picks a genre by the age of the record's year and its medium, weighed
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

*A tongue for every people* (`galaxy/tongue.*`; after Martin O'Leary's naming languages and Mark Rosenfelder's kit). A people's seed
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

*The people's things in the texts.* The culture's pools (`culture.vg`, `culture2.vg`, `culture3.vg`) are lists from which each people
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

*The counts* (`galaxy/records.*`). C-02's fifty keep their categories (C-04's twelve to eighteen pieces, C-10's three to five charts).
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

*The music by age* (`galaxy/music.*`). A people's music changes with its ages and stays its own. `traditionOf` draws C-04's core as before, to the bit (the scale and its
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

*The star charts by age* (`galaxy/charts.*`, `game/chart_draw.cpp`). A chart is drawn in the way of its record's age and medium (`chartStyleOf(age, medium)`, `StarChart::style`, galaxy/charts.*): the
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

*The caption* (charts.vg) is the way's: an opening of its object ("On the cave's roof, over the fire:", "Turn the rete and the stars
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

*The history* (`galaxy/legends.*`, `game/history_screen.cpp`): THE AGE THEY REACHED once the records have told it (`AGE_TOLD_RECORDS`,
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

## The ship's player and the instruments of the ages (C-11)

C-11 (2026-10-10), the last of 1.3.0, closes KI-347: the pieces and the recordings play on the ship, away from the decoder, and the peoples'
instruments lie in their ruins and are played from the cabin.

*The ship's player* (`game/player.cpp`). The decoder's worlds turn with `TAB` to THE PLAYER: every piece held, every recording held (a text
record made on something that sounds, H-05's media: a wax cylinder, a disc, a reel, a drive, a crystal, something that hums; a tablet or a
page is read on the decoder, not played) and every recording the radar caught (`Guide::heard`, `AIR:`: the record's opening as it was on
the air, heard through `MEDIUM_AIR`, the receiver's band, the air's hiss and its slow fading), by their worlds and years. A piece is listed
by its form and length from its draws alone (`shardDrawOf` and the catalogue's age: no people's texts written for the list), a recording by
what it was made on. `ENTER` plays one and the list on from it, `SPACE` pauses, `BACKSPACE` stops. It plays wherever the ship goes, a line
on the space HUD under the system's two (`A SONG - THE NULO  0:42/1:21`; the people left out where the line has no room), the hum and the
drive lower under it, and stops at a landing. The synth's piece and speech point into the player's own copy of the world
(`ShipPlayer::world`), so nothing the decoder opens moves them; one sound at a time: while the decoder plays a record of its own, the radar
listens or the gramophone turns, the player is held and lets go of the synth, then takes up where it was (`AudioState::pieceFrom` in beats,
`speechFrom` in seconds: the synth starts there, the notes begun before not struck again). The game keeps its own clock into it and takes
the synth's when the synth reports (the harness runs none).

*The instruments* (`galaxy/instruments.*`; C-17 below replaced the kinds named here with a constructor: what follows was C-11's). A people's
instruments are the voices of its ensembles (H-05's `Tradition::ages`), one of each
kind an age had, of the kinds a hand can carry: a bone flute, panpipes, a double pipe, a musical bow, a lute, a long-necked lute, a lyre, a
harp, a zither, a fiddle, a spike fiddle, a viol, a bass fiddle, a shawm, a reed pipe, a squeezebox, hand bells, a gong, chimes, a
bullroarer (the prehistoric and medieval ages, by the people's coin among the kinds of a family); a violin, a cornet, a horn, a bass horn, a
music box (the industrial); an electric guitar and bass, an electric piano and organ, a synthesiser (the modern); a tone panel, a pattern
engine, glass rods, tuned bars, a swarm engine (the futuristic); a ring that sings, a hollow ring, a bell stone, a deep stone, a stone that
hums, a glass star, a choir stone (the far future); and the kits': a skin drum and a slit drum of the prehistoric kit, a medieval tabor, an
industrial kettle or frame drum, a modern drum machine (its keys its pads); the industrial age's gramophone besides, which plays a disc of
theirs (a piece of the age's forms from its own seed) through a cylinder's horn and crackle. The singers, the drones, the church organs, the
pianos and a band's drum kit stay where they stood. A people has 14 to 24 over its ages (the age worlds' 19, 22, 16, 20, 17, 24). Each is
named for what it is, never for its people's age (A-01). Its timbre is the ensemble's voice itself, so the lute found sounds as the lute of
their pieces (the unit compares the case's 112 timbres, read through the lore the view reads, with the decoder's).

*Where they lie* (`recordSitesOf`'s settlement branch, its own stream: the records lie where they did, the unit compares 233 places without
and with). Beside a third of the music records (`INSTRUMENT_SHARE`, by the record: every copy of it has it), of the record's age, in its room,
0.75 to 1.45 m from it clear of the pieces, the records and the doorway (`InstrumentSite`); the view resolves which of the age's instruments
(`instrumentIdOf`, the pick), keeps one of each in a settlement, draws its object with the records' (`instrumentForms`: the records' banks,
bone pale, wood ochre, leather and lacquer dark, brass and steel, the people's accent and light) and a glint over it that pulses twice (a
record's once). `E` takes the nearer of a record and an instrument, the HUD names it (`A LUTE - E`); taken, every copy on the world is gone
(`Guide::instruments`, `instrument <body>/I<id>` lines, the id `people x 1000 + age x 100 + slot`). Over the age worlds' places of the plan and
the countryside round the capital: the Witniikh 12 sites in 9 of 83 places, the Ta-Zhozo 50 in 46 of 385, the Aachidblukh 156 in 115 of
404, the Nulo 338 in 226 of 577, the Irrur 19 in 14 of 110, the Glaak 14 in 2 of 13.

*The case* (`game/instruments_screen.cpp`, `game/instrument_draw.cpp`). A cabinet on the cabin's left wall behind the GOES console (`facing`
10, lime, bank 28: R-404's colours stay at least 0.3 apart), also the flight computer's devices page. The one on the screen is drawn from
its own numbers: a flute's holes and a panpipe's tubes are its scale (the tubes' lengths the degrees' ratios inverted), a lute's and a
guitar's frets its tuning's steps on the neck (an equal division's steps, or a just scale's notes), a harp's strings two periods of the scale
with the period's first in red, bars, bells and glass rods by the degrees' ratios, a keyboard's keys the scale's degrees with a dark key where
it leaves a gap wider than a step; its proportions and ornament from its seed, a body painted in its people's accent two times in five. The
keys `1` to `9` and `0` play the scale's degrees from the register's first note up (held while down: `AudioState::instrumentHits`, the
synth's twelve instrument voices with a limiter of their own, the voices set up by `voiceShape` as a piece's), `LEFT`/`RIGHT` the register
a period down or up from the instrument's own (a bass one period down, a flute, bells, a music box one up), the course just played glowing
and a string shivering; ten bars under the picture stand as high as each degree's pitch, the scale's own steps. The player plays on under
it (one may play along).

*The levels* (`vesperis_test instruments wav`, a phrase up a period and back and a held note; a far future's slow tones three at a time): the
age worlds' 18 of their own age peak at 0.13 to 0.65 and stand at an rms of 0.034 to 0.14 (the gramophone's disc 0.14 through its horn),
the 7 pitched ones' first notes heard within 2% of their degree's pitch (the autocorrelation's shortest strong period).

*The harness*: `vesperis_test instruments [ages]` lists each age world's instruments by age and their sites over its places; `instruments
<sx> <sz> <body> [second]` a world's; `instruments wav` writes `shots/tests/instrument_<age>_<slot>_<name>.wav`; `instruments scenes [age]`
lands at each age's scene world's capital (its cities after), stands before an instrument of the world's own age where nothing else to take
lies nearer, takes it and plays it in the case (`shots/tests/instruments_<age>_<found|taken|case>.png`: a slit drum, a tabor, a gramophone
turning, a synthesiser, tuned bars, a deep stone); `instruments check` runs the unit's C-11 checks alone (the instruments from the ensembles,
beside the music records, their sound, one of each age found and played; the player's list, through a flight to a landing, held and taken
up, off the air).

## The instruments' makers (C-17)

C-17 (2026-10-10, `galaxy/instruments.*`, `galaxy/instrument_plans.cpp`, `game/instrument_draw.cpp`; the user's request after C-11: "make
shape and form of instruments procedural as well - civilizations are different, numerous, alien - yes, there are industrial or prehistoric
societies but it doesn't mean everyone was carving bones or building gramophones. It should be sort of a constructor that would place randomly
generated instruments"). C-11's 49 kinds chosen by the age (every prehistoric people's bone flute, every industrial people's gramophone and
violin, each drawn by a hand-written picture and object) are gone: every instrument is made by its people's own way of making, from parts.

*What sounds in it* (`InstrumentClass`, `instrumentClassOf`): its voice's family, as H-05 left it: an edge-blown pipe (`TF_PIPE`), a reed,
free reeds (a reed of two or more copies: a bellows), lips (brass), a plucked string, a string plucked in the mouth (C-11's musical bow's
timbre), a bowed string, struck metal or stone, a whirled blade, the far future's hum, a comb plucked by pins (the music box), an electric
string, electric keys and the electric organ, struck glass, a synthesiser or a pad, a swarm, a field; of the kits a skin, a log's tongues, a
rattle, jingles, an industrial band's cymbals, a modern drum machine's pads; and the industrial age's machine that played the wax cylinders.
The singers, the synthetic voices, the pianos, a church's or a barrel's organ, the drones and a drum kit stay where they stood. The sound is
the voice's as H-05 made it (its timbre, every number): the music is the same to the byte (its 25 WAVs identical to C-11's); the make is the
thing that made it.

*The plans* (`InstrumentPlan`, `INSTRUMENT_PLANS`: 97 over the 24 classes, each a few lines of `instrument_plans.cpp` over the shared pieces):
a straight pipe, a side-blown pipe, a vessel, a raft of pipes, a ring of pipes, twin pipes; a reed pipe and its bell (or a hook curling back,
or a bulb), a double reed pipe, a bag and its pipes, a wind chamber and its pipes; a bellows box, a round bellows; a coiled horn (an oval
where the people's bodies are boxes), a folded horn, a long horn, a spiral horn, a forked horn, a great upright horn (the bass); a necked
body, a frame (a lyre, its arms straight and splayed or curving like horns), a harp (arched, angled or framed: the neck through the strings'
tops, their lengths the scale's ratios), a board of strings (fixed or movable bridges at the strings' ratios), a strung tube, a ring of
strings (its hub off the middle: the spokes graded), plucked tongues; a strung bow, a tongue in a frame, bows on a box; a bowed necked body,
a spiked body, a bowed tube, a bowed board, a wheel and its keys, a bowed horn; bars on rails, hung tubes in a frame, a set of bells, a tree
of bells, a row of bowls, gongs (one in its frame, a row, two rows, a ring), ringing slabs, a wheel of bars; a blade on a cord, a disc on a
cord, a ridged tube; an orb in its cradle, a grooved ring; a pinned drum and its comb, a pinned disc; a solid body, a bare frame, a lap slab;
a keyed slab, keys on an arc, a ring of keys; glass rods (a fan, a row, a grove, a frame), glass bars, bowls on an axle; a panel of keys and
knobs, a ribbon, a grid of pads, an orb of keys, strips of light (the futuristic); a field of rods (an urchin, a feather, a pinwheel, tufts),
a lattice, a cloud of beads; rings (nested, eccentric, broken, polygonal), a monolith, a star of rods (plain, forked, ringed, curving), an
orb and its moons, nested shells; a frame drum, a barrel, a goblet, a pot drum, an hourglass, a kettle, a carapace drum, a pair; a slit log
(or a slit gourd, pot, stone), a tongue box, slit canes; a vessel rattle, a strung rattle (a ring to wear, a bunch on a stick, a string
between two handles), a rattle stick, a rain tube; a jingle fork, a jingle hoop, a jingle stick; a pair of cymbals, small cymbals on cords,
a cymbal and its striker; a box of pads, a ring of pads; a horned machine, a cabinet, a box of listening tubes. A plan has its ages (a bag
from the medieval age, a bowed horn from the industrial, strips of light from the futuristic) and its leanings (`PL_RADIAL`, round, open,
long; a strung tube needs cane or wood, a carapace drum chitin or shell; a great upright horn is a bass's only).

*Each people's way of making* (`Organology`, from the world's generator and the lore's people's seed: hashed streams of their own, so the
tradition's music draws stay):

* *What its world gave*: wood where its flora's families grow trees (domes, cones, umbrellas, giants, weeping, candelabra, spires), stalks of
  cane from fibrous stalks and fern trees (and lake country), gourds from domes, umbrellas and candelabra's pods, pith from mushroom trees;
  a desert's a third of it and gourds; its animals by the world's hash: bone, horn, hide and gut (most worlds), chitin (two in five), shell
  and coral from its seas (a desert's old seas count); ice where it is cold, obsidian by its volcanoes and black sands, crystal on spires,
  salt flats and fused glass; stone and clay everywhere (a desert's the more). The metals from the medieval age (bronze, copper, iron,
  silver), brass and steel from the industrial, an alloy from the modern; glass from the medieval age a little (by the volcanoes' black glass
  before); lacquer and paint; plastic in the modern age; composite, ceramic, crystal and light in the futuristic; the void's dark glass in the
  far future. Each material by the age: what the early ages made of bone and wood the later ones still make now and then.
* *What it loved*: three of the natural materials its world gave (five times, two and a half, one and seven tenths the rest), a metal, a
  made material; and its plans: a log-normal leaning a plan, pushed by its genome (a radial people four times to its rings of pipes and
  strings, its wheels of bars and rings of keys; a people of round plans to its vessels, bowls and gongs; of open roofs to its frames and
  harps; of tall massing to its long things). An age's second instrument of a class leans away from the plan of its first (a band's horns
  differ).
* *Its style*, its architecture genome's (A-04): its bodies' outline (`FK_PLATE`'s numbers) from its plan (a rectangle, a circle, a polygon
  of the people's number, an ellipse, a free hand leaning sideways, a court), its walls' profile (a taper, a bulge, a waist) and its roof (a
  dome over a body); its holes and sound holes its windows' shape (round, square, arched, a slit, a triangle, a keyhole); its heads its
  buildings' tops (a knob, a notched head, a spike, a carved figure, a plain end); its ornament (bands, carving in its motif, studs round a
  rim) and paint in its accent where it paints (its accent's place: the bands, the face, round the holes, the head); its symmetry (a
  radial people's pipes, bells, bowls and gongs in rings, a turning people's in spirals and pinwheels); its number (drones, bands, rods,
  moons, gongs in a ring); the rhythm of its holes (even, in pairs, in threes, by the scale's own steps); its proportion; the light of its late
  ages (lamps or lines).

*The make* (`InstrumentMake`, `Tradition::makes`, `Timbre::make`; `traditionMakes`, called last by `traditionOf`): each voice and kit piece
that is a thing, keyed in its age by its class, family, kind and a bass's own (the strings' lead and second one fiddle, free reeds' tune,
chords and bass one bellows, a kit's low and high skins one pair of drums, the drum machine's pieces its pads); its plan, its materials (a
body, a second for the neck or the frame or the rails, fittings, strings, a skin, an ornament's, and what its world gave for a stick, a vessel
and a tongue), its style with a little of its own, its size (a bass's half again), a variant for the plan's own choices. Its name is what it
is made of and how, seventeen letters at the most (the case's list), its material's word left out where it would not fit: `A SHELL PIPE`,
`CANE PIPES`, `A NECKED GOURD`, `A HORN FRAME`, `A STRUNG RING`, `BOWS ON A POT`, `A SPIKED CARAPACE`, `BRONZE BOWLS`, `RINGING STONES`, `A
WHIRLED BONE`, `A TALKING MACHINE`, `AN ELECTRIC WEDGE`, `A KEYED TWIN`, `A TONE ORB`, `A FOREST OF RODS`, `NESTED SHELLS`, `A SINGING ORB`.
The timbre takes the name (a kit piece the plural: `POT DRUMS`), so the decoder names a piece's instruments as the ruins show them (`VOICES,
A VOICE, A RING OF PIPES AND PAIRED DRUMS`). The industrial age's machine for the wax cylinders is made where the people made one (a third
to three quarters by the people: 62 of 100 near home), a horned machine, a cabinet or a box of listening tubes; Enter winds it as C-11's
gramophone.

*The parts* (`InstrumentPart`, `instrumentParts`): a tube lying (a cone, a bell's flare in four lengths, an arc's or a spiral's lengths), an
outlined body (`FK_PLATE`: a superellipse with a taper, a waist, a lean, a dome, or a polygon; A-02's newest kind, its outline one function
the mesh and the picture share), a disc standing (a bowl open), a ball, a box, a ring lying or round a tube, a string, a mark (a hole, a
fret, an inlay, a seam: round, a slit, a square, a triangle, a keyhole, an arch, a star). In its own metres as it lies, u along it, v across,
w up; centred. Every part that sounds carries its course (a hole, a string, a bar, a key, a pad, a bell; a drum's skin): it glows as its key
is played. A pipe's holes by the people's rhythm, a raft's tubes and a harp's strings by the scale's ratios inverted, a lute's frets by its
tuning's steps, a board's movable bridges where the ratios fall, bars and rods by the ratios' square roots, a keyboard's dark keys over the
scale's wide steps. A plan's pieces: a body in the people's outline and its ornament, holes along a run, a sound hole and its rosette by the
motif, a head by the tops, pegs, frets, a keyboard, knobs, the late ages' lights, a bow beside a bowed one, a beater beside a struck one;
a part lying on a dome sits on its surface (`surfaceAt`).

*Two pictures of one thing*: the object in the ruins is the parts as A-02's forms (`instrumentPartsToForms`: a tube a lathe lying, a body a
`FK_PLATE`, a disc or a bowl a lathe, a ball a tank, a box, a torus, a string a cable a millimetre thick in its material's bank, a mark a thin
prism; in the records' banks or the people's: bone, shell, ice and ceramic pale; horn, chitin, darkwood, iron, obsidian and the void dark;
wood, clay, copper and coral ochre; hide, gourd, cane, bronze and brass the cut wood's bank; steel, silver and the alloy metal; stone and
composite the people's structure; glass and crystal their glass; paint, lacquer and plastic their accent; light their light); beyond 7 m its
strings and its smallest parts are left out. The case's picture is the same parts as it lies seen from above (`drawInstrumentParts`): on a
felt between the dark and the pale, each part over its shadow, lit from the upper left (a tube's light side and shadow, a body's bevel and
its dome's sheen, a ball's sheen, a bowl's dark inside, an incised hole's lit lower edge), the lowest drawn first and what lies on a dome's
slope after it, turned a quarter where it lies tall in the wide box, a ruler under it (2 cm to 1 m). An instrument is 29 forms on the mean
(22 the median, 95% under 71, the heaviest 144: small cymbals).

*How far apart* (`vesperis_test instruments distinct [peoples]`, the unit's check over 48): for each age and class, each people's first
make; pairs sharing the plan (26%), the plan and the body's material (5.0%); each drawn small as the case draws it (144 x 80) and pairs
whose pictures differ by under 6 on the mean of the channels counted alike: 0.87% of the 72,440 pairs of 100 peoples near home (the closest
0.4, the median 31.5; `shots/tests/instruments_closest.png` sets the closest sixteen side by side). Prehistoric pipes of bone 13 of 100 (C-11:
every one); 326 names; no class's plans fewer than three where three were open to it and five peoples or more made it.

*The harness*: `instruments sheet [age] [peoples] [scale]` draws each age's instruments of peoples near home, a people a row
(`shots/tests/instruments_sheet_<age>.png`); `instruments plans [peoples] [scale]` every plan made by a few peoples at its class's age
(`instruments_plans_<n>.png`); `instruments distinct [peoples]` the measure above, the forms an instrument takes, the commonest names
(`VESPERIS_MAKERS=1` every name, and the unit check's every finding); the lists (`instruments ages`) print each one's plan and materials.

## One age a world (B-414)

The user found a disc in what read as a medieval village and a lattice mast over its ruins (a modern people's world: its villages a
medieval core in rings of industrial rows and modern suburbs, its records over its eras, its modern houses in its medieval forms) and
chose that a world show only the age its people reached. The history (H-02) keeps every era; the ground is the last age's.

* **The age's first year** (`ageFromOf`, galaxy/peoples.h; `Lore::ageFrom`): the start of the last era in the people's count (1 at
  least). A modern people's last age runs 30-300 years, an industrial people's 30-400, so its records and its dead fall in those.
* **Towns** stand in the last age's way, one layer (`placeHist`, `countrysideHist`); a place of the plan the history left before that
  age is gone (`TownHist::gone`: `cellSettlement` returns nothing for its cell, its works go with it: `WB::gone`), one left within it
  weathers from when it was left. Over the six age worlds 40 places are gone and 39 test towns are all their people's age.
* **Works** (`galaxy/works.cpp`): `placeAge` and the history's works give the people's age (`eraAge` is gone): a modern people's dam,
  mine, observatory or lighthouse its history finished in an earlier era stands as a modern one. Gone: a battle of an earlier age
  (`e.year < ageFrom`), a later people's henges (a prehistoric people's own stay), a modern or later people's aqueduct (a medieval and
  an industrial people's in their age's way). A necropolis of barrows raises its age's stelae (menhirs a prehistoric people's).
* **Records**: every record made in the last age's years; the medium, the category's share, the genre, the music, the chart's way and the
  instrument beside a piece follow. **Histories** (`galaxy/grammar/history.vg`): an industrial people's on printed pages or a codex, a
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

## Each age its own vocabulary (B-415)

The user found an industrial town that read as modern (white stucco, flat roofs, ribbon windows) and as medieval (colonnades of stone,
white chimneys standing like columns) and asked that every age be clear, the genome kept. **The age speaks first, the people second**: each
age has its vocabulary and the genome chooses within it; the genome's variables stay the people's own through its ages.

* **A building's role** (`greatKind`): the people's great buildings (temples, churches, town halls, palaces, keeps and towers, the focal
  accent, monuments, gates, platforms, rotundas, joined towers, aqueducts, tunnels), where its genome speaks fully, and its everyday ones
  (houses, rows, tenements, villas, blocks, halls, mills, works), where the age speaks first.
* **The genome read through the age** (`ageExpressed`, galaxy/buildings.h; the generators read it through `Gen::gv`; `archSub`, `ageDome`):

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

## No one's ruins, the land's works, and not the season's (B-417, B-418, B-419)

**A world's ruins are its people's (B-417).** M4-07's monoliths of the old ones (a ring of eight columns, a cube, a dome, three walls, a
rare 40 m cube, in one 2 km cell in twenty of every felisian, quartz and ocean world, a people there or not) were nobody's: no age, no
genome, no record, drawn by code of their own. They are gone. `worldHadCivilisation` is the one question (`worldHasRuins` is gone); a world
without a people (seven felisian worlds in eight, every quartz and ocean world) has no ruins and its scanner no RUINS mode. On a people's
world the draw's first twentieth of a cell goes to the countryside's lone monuments, its people's in its age and genome, as on a dead
desert world before. A ruin's echo carries its settlement's class (`Landmark::sub`).

**What is made on a world is not its season's (B-419).** The plan, the people's extent, its histories, its settlements, roads and works
and the scanner's sights read the planet at the year's mean (`unseasoned`, galaxy/planetmap.h); the game's generators carry the season of
the moment (a winter's sea ice had counted as land, and nearly every place of a plan moved with the season of a session's first approach).

**A people's everyday works are its land's (B-418).** T-03's rules made a handful of each kind a people (24 quarries by its towns, four
necropolises by its capital and cities, four wrecks, three henges...), and T-01's plan gives a people hundreds of places over a third of its
world or more: over 41 people's worlds near home a tenth of the plan's places and one point of the peoples' land in two hundred had a work
within the scanner's 40 km. The world's list keeps the history's works and the world's great ones (the battlefields, the dams, the
aqueducts, the observatories, the lighthouses the history lit, the launch sites, the holy place and its way, the colossus, the fortified
line, the antenna field, the terraformed basins, the far future's, the lines on the plains, the last shelter, the mines the history sank);
the everyday ones are the land's:

* **The grid** (`landCellOf`, `landCellsAround`): rows of `LAND_CELL_M` (8 km) of latitude, each row's cells at least as wide; a cell's
  people the side of the divide its middle lies on.
* **Whether a cell holds one**: its hash under the countryside's density at its middle (`planCountryside`: the people's land, thicker near
  its places, none beyond its frontier) times its age's appetite (`LAND_RATE`: 0.34 prehistoric and medieval, 0.36 industrial and modern,
  0.5 futuristic round its great places, none in the far future, whose works are the world's).
* **What its ground offers** (`landWork`: a 5 x 5 reading of the cell and the plan's coarse grid at its middle): rock, open ground, forest,
  snow, a shore (water that is not an old sea), coal and ore (the plan's `res`), a hill (`relief`), the greatest settlement in or round it
  (the plan's places reaching in, the countryside's in its 2 km cells, not a lone monument). A prehistoric people's flint pits (on any
  ground but snow), standing stones (open ground), a hill fort (a hill); a medieval to modern people's quarry (rock for a stone-built
  people, else a pit on any dry ground), a mine (coal and ore), a wreck (a shore; a modern people's aircraft on open flat ground), a
  lighthouse (a shore by a settlement), a wind farm (a modern people's ridges), a fort (anywhere, likelier on a hill); a futuristic people's
  wreckage fallen from orbit; for every people but the far future's a necropolis by a village or more (a prehistoric people's by any
  settlement) where its genome buries its dead, and a shrine where the cell's ground is its holy place's kind. One is drawn by their
  weights; one that finds no site gives way to the next.
* **Where in it** (the kind's rule over a jittered grid of the cell, `landPoints`): its reach and 60 m inside the cell (no two of the land's
  touch: `landInside`), clear of the 2 km cells' settlements (their radius and 60 m), the cities' cores (150 m) and the world's works (40 m),
  as T-03's rules kept clear of each other (`WB::clearOf`, the world's works its `world`).
* **Made when asked** (`landWorkOf`): a cell's work (or none) made the first time it is asked for, in a few milliseconds, and kept (the last
  two worlds' cells, a world's cleared at 40,000); the world's works waited for first (the land's keep clear of them). `worksAround` gives
  the world's and the land's within a reach (the scanner's `worksHeardNear`, the landing's `settlementClearance`, the harness);
  `worksAroundIfReady` the view's, which never waits (its `WorkNear` holds each as a shared `Work`). `Work::land` marks the land's.
* **What the explorer meets**: from every point of a prehistoric to a modern people's land sampled by the unit (216) and every place (108)
  the scanner hears a work within 40 km, eleven on the whole; a scan's first hearing of a region takes 5 to 10 ms on the scanner's thread.
  A futuristic people's land is wild between its great places (T-01's countryside), and so are its works (KI-396).
* **The fort** (`WO_FORT`, `A FORT`, B-418's new kind): a prehistoric people's hill fort (one to three rings of bank, `BK_MOUND` style 4,
  inside their ditches, the huts of its last people within), a medieval people's castle (a curtain wall of five to eight sides, a wall tower
  at each corner, the gatehouse in the side toward the settlement it guarded, a keep back from it, a moat on the flat), an industrial
  people's redoubt (`BK_FORT`: its ring bank round a casemate, its ditch), a modern people's strongpoint (pillboxes round a blockhouse,
  dragon's teeth across the way up); one record in its keep or casemate (`recordSitesOf`).

## The genome within each age, the walls and the records (R-412, B-420, B-421)

R-412 (2026-10-11): B-415 let the age speak first; within an age a people's everyday buildings drew from fewer variables, and towns of one
age looked more alike. Sixteen more deep variables (`galaxy/genome.*`, fifty-one in all; each its own hash, so A-04's thirty-five did not
move) act on what the eye picks out along a street, each read through the age as B-415 reads the rest (its expression the age's, its
choice the people's). `docs/iteration-3/plans/requests/R-412-the-genome-grows-within-each-age.md` has the table; in short:

* **Roofs**: PITCH (every pitched, hipped and conical roof and gable, a hut's cone: `Gen::pitchK`), EAVES (the overhang, a bellcast
  skirt, a modern slab's lip, deep slab or cornice: `Gen::eave`, `slabEave`), ROOFLINE (`rooflineOf`: dormers, a cross gable, cresting,
  a bell-cote or a ventilator; a dome's or a cone's lantern, a vault's crest, a cell roof's crest and lantern; on a hand-made people's
  flat roof a pavilion, a stair house, a balustrade, a cupola; on a modern people's a pergola, a water tank, rails, a penthouse), GABLES
  (`gables`: crow steps, a curved neck and its cap, a pediment's cornice and raking frame, a half hip), ROOFING (palette slot 2 in the
  age's material: `genomeRoofing`, eight colours by age).
* **Walls**: POLYCHROMY (a dado and courses through the windows' pattern, `WindowSpec::dado`, `stripe`; each house its own shade and a
  third in the accent, `Gen::tone`, `painted`, `Form::tone`; a lighthouse's bands), TRIM (`WindowSpec::frame`, `shutter`), PROJECTIONS
  (`projectionsOf`: balconies, galleries and a one-storey house's verandah, bays, oriels, each on the front as `walls4x` leans or jetties
  it; `ringDress` for a round house; `podDress` for a pod), ENTRANCES (`entranceAt`: porch, stoop, portico, hood or canopy; `hutEntrance`;
  a pod's arch, step, pylons, shell), CORNERS (`cornersOf`: quoins, pilasters, corbelled turrets or a low house's corner tower; a modern
  people's glazed corners, fins, rounded corners).
* **Town**: STOREYS (`Layout::heightK`), FRONTAGE (`Layout::frontage`; an industrial row a gable a house), SETBACK (the lots' setback,
  `Building::yard`), BOUNDARIES (`yardOf`, `yardRun`, `hutRing`: each under C-01's 0.9 m, stepped over), FURNITURE (`BK_FURNITURE`,
  `genFurniture`, `placeFurniture`: laid last in a town by a stream of its own, round a futuristic people's great structure), POLES
  (`hutPole`).
* **A new form**: `FK_FENCE` (pales, bars, wattle, a balustrade; its pales one by one near, its stretches as bands far; its colliders discs
  where its pales stand). A building in a row or round a compound's court is `Building::joined` (no turrets at its sides, no garden behind).
* **The works**: every pitched roof of a work takes the roofs' variables; a lighthouse its section, taper, bands, lantern and rail, a
  headframe its structure (a lattice, a brick tower, an A-frame), a turbine a lattice tower, a rounded nacelle and its blades by number,
  an observatory, a pillbox and a casemate the people's section, a tomb its gables and doorway, a hangar its roof; a church, a town
  hall, a station and a mill their doorways, corners and roofline.
* **The uniqueness** (`genomeDistance`): R-412's own among the strongest variables (the roofing 1, the pitch, the polychromy, the
  projections and the storeys 0.6, the eaves 0.5, the roofline, the corners, the frontage and the boundaries 0.4, the gables and the
  trim 0.3, the entrances 0.3): 19.8 at most, alike under a quarter of it (`GENOME_ALIKE` 4.9). The hundred peoples nearest home are
  10.0 apart at the nearest.

**B-420, the walls the explorer meets are the walls drawn** (`docs/iteration-3/bugs/resolved/B-420-*.md`): every kind's colliders follow
its drawn mesh (`formColliders`: a lathe's slices by their narrowest outline, a lying piece's section where it lies, a vault's flanks,
an arch's piers, a truss's legs, a sweep's faces band by band); a big piece stands on the highest ground under it with its colliders
(`pieceFootNear`); a heap, a bank, a barrow or a tier is stood on (`formWalkable`, `formSupportAt`); the rocks drawn are the rocks
collided with (`forRocks`). **B-421, the records lie where they can be reached**: off the drawn pieces at the floor and the heaps'
slopes (`Placer::clear`), out of the water (the view lays a place's records again with its drawn water within 600 m: `waterRecords`),
off the earthworks' banks, and not behind a doorway something fell across (`Placer::doorOpen`).

## The drone bay (C-18)

The user's request (2026-10-11): "add three drones with robotic hands [...] that you can send from stardrifter to felisian/desert planets
with civilizations. When you send them each can bring 2-8 items [...] cooldown is 5 minutes. They can bring both shards (all three kinds)
and instruments", and the instruments' case "separated by planet/world as shards are".

**What a world holds for them** (`galaxy/drones.*`, `droneWorldOf`): every record of its peoples (both, on a world of two: T-04's 80 to 200
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
with everything of its own (above); C-14 lent the shards found and read to a friend's decoder (above); C-16 put each age's broadcasts on the air (above); A-02 gave the renderer the forms the ages will build with (above); A-03 built each age's buildings from them and weathered them by their materials (above); A-04 gave every people its architecture genome, its own through its ages (above); T-01 laid out each people's world, its territory, its ranked places and its networks (above); H-02 told each people's history over the plan's places in the order of their founding (above); T-02 laid its villages, towns and cities as their ages laid them, their streets, districts, layers and edges, the old cores the history founded them in and C-08's reading of the site by age (above); T-03 built its works outside the towns by the land's rules and the history's (a battlefield where its war was fought, a dam of its era, the last shelter by the last place; above), the bridges and the locks its ways need left for later (KI-379); T-04 counted each people's records, dated them in its history, made them in their years' way and laid them in its places, archives and works, the last recording where the history ended (above); H-03 built the grammar engine, its format, its realiser and its checks, and rebuilt C-02's kinds on it (above); H-04 wrote the records from the history: the genres of each age, the collapse told from many sides, the language by roots and the decoder's pages (above); H-05 filled the catalogue's three categories by age: the music on each age's instruments, the charts in each age's way, the media heard and drawn (above); H-06 read the people's history off the records read: the decoder's records by kind, the timeline of the eras, the history page of the events learnt, the persons as named, the people's ways, the age once told and the last recording closing it (above); C-11 played the pieces and the recordings on the ship away from the decoder and put each people's instruments of its ages in its ruins and in the cabin's case (above); C-17 made those instruments by each people's own way of making, from parts, of what its world gave and in its genome's style (above). None of them needed a new
generation: the shards are a function of what the galaxy already is.
