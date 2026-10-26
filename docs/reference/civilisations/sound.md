# Civilisations: music, voices and instruments

One of the five notes on the dead peoples (`peoples-and-ages.md`, `built-world.md`, `records-and-grammar.md`, `sound.md`,
`radar.md`), split by topic from `12-civilisations.md` in E-04; its sections are as they were, in their order.

<!-- contents -->
Contents:
- The music (C-04)
- The voice (C-05)
- The ship's player and the instruments of the ages (C-11)
- The instruments' makers (C-17)
<!-- /contents -->

## The music (C-04)

Twelve to eighteen of a world's fifty shards are pieces of music (`Shard::music`, `shardIsMusic`: a shuffle of the fifty
from the world's seed, the count from the seed too), with no text; they keep the tone a text would have had, and the
piece's form follows it. `peoples/music.*` is the module, `game/audio.cpp` the player, `game/shards_screen.cpp` the screen
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
for the digits. `peoples/voice.*` is the module, `game/audio.cpp` the speaker, `game/shards_screen.cpp` the screen
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

*The instruments* (`peoples/instruments.*`; C-17 below replaced the kinds named here with a constructor: what follows was C-11's). A
people's instruments are the voices of its ensembles (H-05's `Tradition::ages`), one of each
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

C-17 (2026-10-10, `peoples/instruments.*`, `peoples/instrument_plans.cpp`, `game/instrument_draw.cpp`; the user's request after C-11: "make
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
