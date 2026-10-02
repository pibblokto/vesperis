# 12. The civilisations: their worlds, their ruins, their shards, their music, their voices

The C series of 1.1.0 (`docs/ideas/IDEAS-ruins-shards-signals.md`, `docs/ideas/MILESTONES.md`). A people lived on one
felisian or desert world in eight and is gone; what it left is settlements of ruins (C-01, `06-surface.md` and
`05-planet-function.md` for the trait and a dead desert's old seas), from C-02 on its words, from C-04 its music and from C-05
its voice.

## The trait (C-01)

`TR_CIVILISATION` is never drawn by the trait draw: `BodyGen::make` gives it to a felisian or desert world when
`mix64(seed ^ 0xC1D1)` falls under 0.125, first among its traits (after the glass), so no other world's traits move.
A trait desert keeps the level its seas stood at (`BodyGen::oldSeaM`, -120..100 m) and a river density of 0.8 at
least: dry seabeds with salt in the deeps, old channels, a shore band. Every other desert is a desert and nothing more.
`worldHadCivilisation(g)` (`galaxy/ruins.h`) is the one question the ruins, the landmarks, the harness and the shards ask.

## The settlements (C-01)

`galaxy/ruins.*`: hamlets, villages, towns and a desert's lone monuments on the 2 km ruin grid, in a `Culture` the whole
world shares (`cultureOf`), each building drawn from its own seed with a doorway, a window, an inner wall, a roof where
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
warning; from 0.3 to 0.85 55% warning, 37% ordinary; past 0.85 75% the end, 15% warning. Overall about half
ordinary, a third warnings, one in ten the end. A felisian world: 83% ordinary, 12% elegy, 5% warning,
and past 0.9 of the span half the shards are the end. So the desert's fifty read as a timeline that darkens and
closes, the felisian's as a life that went on with fewer people in it (the agreed tone, ideas note section 2).

*Text.* `tidy` closes the seams: one space between words, no space before a stop, "a" before a vowel is "an", a capital
after every stop, a stop at the end. The texts are translations, so their nouns are ours (goats, barley, a quay); on
the ship's screen (C-06) they are in capitals, the game's font having no lowercase. What a shard withholds is the
decoder's doing (below), not damage: C-02's `damage` and `Shard::fragment` went with C-06. `Shard::child` keeps the name
the kind gave its child, for the decoder.

*Numbers.* Fifty shards of Aieliaalas II (the first felisian world with the trait near home): 45 ordinary, 1 elegy, 3
warnings, 1 the end (the tones count the pieces of music too, which keep theirs), 13 of them music, 31 words a text; of
Leileashphail III (the first dead desert): 27 ordinary, 3 elegies, 19 warnings, 1 the end, 12 music, 34 words.
`vesperis_test shards 151 25 1 50 full` and `shards 153 70 2 50 full` print them.

## The shards in the ruins (C-03)

A settlement holds a few of the world's fifty (`shardSitesOf`, `galaxy/shards.*`, from the settlement's seed): a hamlet one
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

## What the later milestones take from here

C-03 placed them, C-06 reads them, C-04 plays them, C-05 speaks them and C-07 hears them from afar (above); C-11 (the instruments) is where the
pieces and the recordings found should become playable from the cabin, away from the decoder screen (KI-347); C-12 sorts
them by `Shard::year` into the world's timeline and marks the last; C-10 adds the star charts; C-13 a second culture on a
world (a second `Lore` by a second seed, a second `Tradition`, a second `Voice`). None of them should need a new
generation: the shards are a function of what the galaxy already is.
