# 12. The civilisations: their worlds, their ruins, their shards, their music, their voices

The C series of 1.1.0 (`docs/ideas/IDEAS-ruins-shards-signals.md`, `docs/ideas/MILESTONES.md`). A people lived on one
felisian or desert world in eight and is gone; what it left is settlements of ruins (C-01, `06-surface.md` and
`05-planet-function.md` for the trait and a dead desert's old seas), from C-02 on its words, from C-04 its music and from C-05
its voice. One such world in five had two peoples (C-13), on either side of a great circle, each with a culture, a lore, a
tongue, a voice, a music and fifty recordings of its own.

## The trait (C-01)

`TR_CIVILISATION` is never drawn by the trait draw: `BodyGen::make` gives it to a felisian or desert world when
`mix64(seed ^ 0xC1D1)` falls under 0.125, first among its traits (after the glass), so no other world's traits move.
A trait desert keeps the level its seas stood at (`BodyGen::oldSeaM`, -120..100 m) and a river density of 0.8 at
least: dry seabeds with salt in the deeps, old channels, a shore band. Every other desert is a desert and nothing more.
`worldHadCivilisation(g)` (`galaxy/ruins.h`) is the one question the ruins, the landmarks, the harness and the shards ask.

## The settlements (C-01)

`galaxy/ruins.*`: hamlets, villages, towns and a desert's lone monuments on the 2 km ruin grid, in a `Culture` the whole
world shares (`cultureOf`; C-13: the whole people, a settlement's being its people's, `RuinSpec::people`), each building drawn from its own seed with a doorway, a window, an inner wall, a roof where
the culture built stone ones, rubble at the fallen walls. `06-surface.md` has the layouts, the pieces, the levels of
detail and the colliders (B-405: each spans its own height); `08-testing-and-tools.md` the scenes, the `ruins` mode and
the unit's checks.

## The shards (C-02): the story grammar

A shard is a recording the people left; decoded, it is a short text or (C-04, below) a piece of music. The texts are
generated without a language model, deterministically from the body's seed and the shard's index, so the same shard
reads the same on every machine and can be found and shared like a landmark (`galaxy/shards.*`).

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
print them.

## The shards in the ruins (C-03)

A settlement holds a few of its people's fifty (`shardSitesOf`, `galaxy/shards.*`, from the settlement's seed; C-13: the second people's at the indices from fifty up): a hamlet one
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
age of what is heard, and a Vimana flight of 79 ly takes 25 s.

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
(`roadCover`), and the buggy rolls on rock where the road is paved. Under the feet the HUD reads `OLD ROAD NE-SW`,
the first step onto each road of a landing is a status line and a `ROAD` entry in the log, and on the landing map the
zoom draws the window's roads as faint lines in the ground's own colour (`Game::zoomRoads`: the villages' and the towns'
roads, since the whole web of lanes between hamlets was a honeycomb over the land), the one map of a world that shows
something not yet found: the roads are terrain, as the rivers are.

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
the test world), and the name is kept as it is in the decoding (`languageOf` adds it to the names). Whether they ended
together is a coin of the world's (`peoplesEndedTogether`): the second people's `ageYears` is then the first's (its decay,
its drifts and its road wear follow from its own wet), else its own log-uniform draw; the calendar is the world's for both
(the day, the moon and the year are physical), the spans and the counts of years are each people's own.

**On the ground.** The view holds a culture and a lore a people (`SurfaceView::cultures`, `lores`, `peoples`) and builds
each cell in its settlement's (`ruinCell`: the pieces, the shard sites, the graves and their names, `drawSettlement`'s bank
and grain), so the two peoples' ruins stand in their own styles and their stones carry their own names and counts. The
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

## What the later milestones take from here

C-03 placed them, C-06 reads them, C-04 plays them, C-05 speaks them, C-07 hears them from afar, C-08 sets their ruins in the terrain, C-10 draws their skies and C-09 joins their ruins by road (above); C-12 dated them in the people's calendar, drew their timeline and
marked the last, and set their graves outside the ruins (above); C-13 gave one world in five a second people, a second seed
with everything of its own (above); C-14 lent the shards found and read to a friend's decoder (above); C-11 (the instruments) is where the pieces and the
recordings found should become playable from the cabin, away from the decoder screen (KI-347). None of them should need a new
generation: the shards are a function of what the galaxy already is.
