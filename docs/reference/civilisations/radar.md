# Civilisations: what the radar hears

One of the five notes on the dead peoples (`peoples-and-ages.md`, `built-world.md`, `records-and-grammar.md`, `sound.md`,
`radar.md`), split by topic from `12-civilisations.md` in E-04; its sections are as they were, in their order.

<!-- contents -->
Contents:
- The signal radar (C-07)
- The radar's gain (C-15)
- What the gain lets in, and each age on the air (C-16)
- The drifters on the radar (D-02)
- The drifters' voices (D-03)
<!-- /contents -->

## The signal radar (C-07)

A few of the dead peoples' worlds still have a transmission on its way out through the galaxy, and the Stardrifter's
receiver catches it (`peoples/signals.*` the sources, `game/radar.cpp` the instrument, `07-game-flow-and-controls.md` the
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
(`programmeOf`, `Programme`, `programmeFor` in `peoples/signals.h`), hashed from the world and the slot, and each world
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

## The radar's gain (C-15)

Turned up, the radar hears farther (R-410: "a way to increase sensitivity of signals radar so you can find further signals
but it should give some radio noise, creepy mixed sounds when you are trying to get really far signal"). C-15 is the dial
and what it hears; the noise and the creepy mixed sounds are C-16's (below). `docs/decisions/log/3-1.3.0.md` (C-15) has the reasons.

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
trying to get really far signal". Nothing of it is printed; the sound and the picture tell. `docs/decisions/log/3-1.3.0.md` (C-16) has the
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

## The drifters on the radar (D-02)

The worlds with no sun (D-01, `04-galaxy-and-systems.md`) are found by the radar alone, and only through the gain: SIMP 0136 was
found by its radio aurorae. `docs/decisions/log/6-1.4.0.md` (D-02) has the reasons.

* **A law of their own.** A drifter of power p is heard at gain G within 180 sqrt((G - 1) p) ly (`rogueReachAtGain`; the least
  gain that hears it, `rogueNeedGain`, is 1 + (d / 180)^2 / p, over 1 for any distance), so none is heard at x1: x1 stays C-07's
  list by rule. Its strength is its margin over the floor, read as a far people's (0.18 at the edge of its reach).
* **The power** (`drifterPower`) by what it is: a brown dwarf 1, a giant 0.6, a young tectonic or molten world 0.12 (its dynamo),
  a metal world 0.1 (its iron core), a rock, a desert, a europan or a frozen living world 0.05, a small icy world 0.02; times its
  radius over its kind's middle, times a spread of its own, 0.5-2 log-uniform (its spin and its field). So the giants and the brown
  dwarfs are what the radar finds; a small world is heard only within about 250 ly at x32.
* **What each step hears.** From home, and within one of it from the three C-series worlds and the six age worlds:

  | Gain | Drifters | Added (kind, distance, the gain it needs) |
  | --- | --- | --- |
  | x1, x5 | 0 | |
  | x11 | 1 | a brown dwarf, 437 ly (x8.7) |
  | x18 | 2 | a brown dwarf, 921 ly (x15.7) |
  | x25 | 4 | a giant, 776 ly (x20.6); a brown dwarf, 983 ly (x21.6) |
  | x32 | 6 | a brown dwarf, 728 ly (x26.8); a giant, 1,197 ly (x27.1) |

* **The scan.** `SignalScan` gains two layers after C-15's: the drifters' cells within the loudest's reach at the dial's top
  (`drifterPowerMax`), then the globular clusters' own; each candidate by its own law (`considerDrifter`). Nothing of either runs
  at x1. A drifter's own system holds it as a local `SIG_ROGUE`, whatever its kind (its moons by C-07's rule).
* **`SIG_ROGUE`** is the fifth kind. D-02 played it as a magnetosphere by its own seed; D-03 gives it a voice of its own (the drifters'
  voices, below).
* **The readout after a lock:** `NO STAR AT THE SOURCE`, the distance (`VISITED` once it has been), `ENTER FLIES THERE`. There is
  no name, no class and no sector: the sound tells, the readout never does. The status says
  `A SIGNAL LOCKED - NO STAR THERE - ENTER FLIES`. The log says `A SIGNAL WITH NO STAR AT ITS SOURCE, 437 LY (437 YEARS OLD), SECTOR ...`
  (and its name, once the explorer gave one). The lock's key is the sector's (`Guide::starKey`, as a pulsar's), so the chart's red
  ring and its `A SIGNAL` stop are at the place. Enter flies there through D-01's resolver.
* **The chart.** A jump's line ends at the star nearest its end and never sees a drifter, so Enter on the chart plots the jump to
  the drifter itself when one the explorer knows (a lock, a visit, a name, home) lies within four pixels of the cursor
  (`chartDrifterAt`, `plotJump`'s `place`).

## The drifters' voices (D-03)

Every drifter has a voice of its own, from its seed and its body (`game/audio_rogue.cpp`; `docs/decisions/log/6-1.4.0.md`, D-03, has the reasons). The
radar plays it as `channelSample`'s `SIG_ROGUE`, on parameters of its own (`AudioSynth::rogueDerive`: indices no other signal reads), so
no other signal's sound moved. It grows from what receivers hear of the planets and from C-16's static. What the synth needs besides the
seed is `RogueVoice` (the class by the body, `rogueVoiceClass`; the period of its turning and of its innermost moon's orbit, `Signal::turnS`
and `moonS`, which the scan and `localSignals` read off its system; its power).

* **The breath of its turning.** The beam of its aurorae sweeps past once a turning: a breath every 20-90 s by its spin (the system's
  0.33-3.7 h), a main lobe and, by its seed, the other pole's weaker one half a turning on; silence between, its drones a breath of
  themselves under it. A voice first heard comes in on the rising lobe.
* **The body by what it is.** A giant's decametric storm: the L-bursts' slow swells of surf in a band of its own, the S-bursts' trains of
  millisecond tones each falling, a fluttering roar (two giants in five also howl). A brown dwarf's storm is thicker, with a cyclotron howl
  rising and falling as its beam comes round, once a turning (SIMP 0136's pulses). A small world's is the Earth's: chorus (flocks of rising
  chirps), whistlers falling from 2.5-5 kHz, a breathing band of hiss, the kilometric howl wandering up and down its range. A young world's
  is a small world's over the rumble of its dynamo, its drones a fourth lower; a metal world's has its iron core ringing now and then (a
  free bar's partials).
* **Its moon's flux tube.** The innermost moon's orbit (1-28 hours, most 3.5-8) pulses it: an arc swept up and down again and the body
  pumped every 0.9-16 s (0.9 s times the orbit's hours to the 0.85), fainter while the beam is away.
* **Its motif, its signature.** A figure of four to six whistles on a scale of its own (7, 9, 11, 13, 17 or 19 steps to the octave), each
  its own pitch, spanning three quarters of an octave or more, with glides, a vibrato and a timbre of its own (pure, hollow, breathy or
  metallic), as the beam's centre passes. The other pole answers it backwards, softer and in a key of its own (a quarter to a little over
  half an octave off). Each whistle hangs on in the plasma after it, dying over its own 10-18 s.
* **Plasma vowels.** A moan with no language in it (a voice's source at 45-150 Hz through three formants sweeping through three vowels of
  its own, sagging as the beam leaves) and the plasma's breath (noise through two resonances wandering between the same vowels).
* **Detuned drones.** A chord of three or four notes on its scale from a root of 45-180 Hz, each beating against a twin 0.12-2.4 Hz off,
  six partials by a tilt and an even share of its own.
* **Its balance and its plasma.** Each part is weighed by the seed (one storm mostly crackle, another mostly surf), and the whole goes
  through the plasma's two resonances (160-740 Hz and 0.9-3.6 kHz, 5-12 dB at their centres), its loudness kept.

*Where it is heard.* On the radar from afar, worn by the distance (C-16's `radarWear`: besides the worn bus's fades, dropouts and echoes the
voice loses its top and its bottom), and mixed under what else is in the beam (`radarRogue2`, the one under it). Whole when the ship is
close: the drifter's own system holds it as a local signal, with no wear. With the camera off, while the ship is at a drifter, the cabin's
radio carries it under the ship's hum (`Game::darkRadio`, `RADIO_CABIN` at 0.45, lower under the ship's player). On its ground (the
drifter's or a moon's: walking, in the buggy or the drone, a sheet or the guide opened there) the suit's radio carries it, fainter and through
a small set's band (`RADIO_SUIT` at 0.4, 300 Hz to 3 kHz). Nothing is heard where the synth is another's: the decoder, the instruments, the
probe's relay and its recordings, the drone bay, the landing, the title. *The beacon's slot*: `radarVoice` with a drifter is a programme of
C-16's machines played under the voice, slowed to 0.7 and darkened; D-05's beacon sets it (below).

*No two alike.* The hundred drifters nearest home (10 brown dwarfs, 36 giants, 3 young worlds, 3 metal worlds, 48 small worlds), each over a
breath through the receiver: their voice prints (the long spectrum in twelfths of an octave from 30 Hz, its level taken out) lie at least
4.55 dB apart pair by pair (the median 7.2 dB), where two magnetospheres of seeds of their own lie 4.19 dB apart at the median (two comets
3.65, two pulsars 3.08). `vesperis_test rogues voices` prints each heard drifter's voice in words and numbers, writes twelve WAVs with
their levels and prints the distinctness. D-04: a dark ocean, made from a rocky drifter, keeps that drifter's voice (its class, turning and
power the same; its moons the rocky drifter's, a moon pushed out past three of the dark ocean's radii pulsing slower).
