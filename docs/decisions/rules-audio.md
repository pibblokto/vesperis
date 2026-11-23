# Rules: audio

Rules in force for the synth, music, voices and all sounds, one line per rule with its log entries (`log/`, by release). A
milestone that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md` (`docs/CLAUDE.md`).

## The synth and its owners
- The synth renders on the main thread (`platformAudioPump`), never a helper, so each voice costs frame time [F-01], [F-08], [D-03]
- The synth reads a piece or speech through `AudioState` pointers into its owner's copy; the owner clears them before moving it [C-04], [C-05], [C-11]
- The harness runs no synth: the game times sounds by its own clock (`pieceT`, `speechT`, `radarRepeatContent`); `speechDone` only ends one early [C-07]
- The radar goes off for the decoder (which takes the synth), flights, arrivals, the surface and loads; other screens pause the sweep [C-07]
- While the decoder, radar or gramophone plays, the ship's player pauses, then resumes (`pieceFrom`, `speechFrom`): one piece at a time [C-11]
- `Game::darkRadio` plays a drifter on the cabin and suit radios only when no other owner has the synth; new owners join its list [D-03]
- `AudioState::surfaceOff()` zeroes all ground levels (a new one joins it) each frame of descent, climb and space and on a load; ground menus keep them [B-413]

## Levels and the proof
- A change not meant to be heard keeps every proof WAV byte for byte (`AudioSynth::Coefs`: per-sample expressions) [F-07], [D-03]
- Levels are set from renders (`sounds`, `signals wav`) against other sounds' rms and peaks, not by ear [C-07], [C-16], [Q-03], [D-03]
- The mix clamps at 1 before the 0.8 master gain, so a render peaking at exactly 0.80 has clipped unseen by `RenderStats::clip` (ground limit: 0.75) [Q-03]
- Every receiver voice, overdriven too, keeps C-07's levels: rms 0.02-0.13 and at most 20 clipped samples [C-07], [C-15], [D-03]
- A sound meant to be heard stays in small speakers' range: the quake rumbles at 80 Hz, not under 45 [Q-03]

## Voices and speech
- Voices are synthesized (`speechSample`), never sampled; a people's voice comes from their seed and tongue (`voiceOf`) [C-05], [C-13], [H-04 second pass]
- `PHONES` has a sound for every spelling the tongues use, longest first; a new spelling needs a row or goes unspoken [C-05]
- Speech says a text's decoded words one for one (`DecodedWord::theirs`, spans kept), however much of the language is read [C-05]
- On the air a record plays its opening (`shardOpening`, 60 words), as the player replays a heard one; the decoder speaks the page shown [H-04], [C-11]
- A medium's sound (`recordMediumSounds`) colours its record on the decoder, never on the air; otherwise the record is read flat (`voiceRead`) [H-05], [R-414]

## Music and instruments
- A people's tradition comes from their seed and their lore's name style (`traditionOf`), a piece from its shard (`pieceOf`) [C-04], [C-13]
- `traditionOf` draws C-04's core first, bit-exact; each age's layers and the instrument makes draw on their own streams [H-05], [C-17]
- Scales: 5-9 notes, none under 75 cents apart, no gap over 450, the period mostly an octave: a tradition from elsewhere, not a noise experiment [C-04]
- Chords stack from the people's own scale (the degrees nearest a third and a fifth); a people with no third plays what they have [H-05]
- A found instrument plays like its people's pieces, on their scale; the make never changes the timbre, so case and decoder sound alike [C-11], [C-17]

## The radar and the drifters
- Signals and drifters sound like cosmic horror, never like music (the user's call); a piece on the air stays rare and slowed [C-07], [D-03]
- A signal's sound is set by its seed (`programmeOf`, `rvDerive`), a drifter's by seed and `RogueVoice`: the same twice, no two alike [C-07], [D-03]
- Each kind has its own `rvDerive` indices (a drifter's from 161, `rogueDerive`); a new voice takes unused ones, `machineInto` adds in the old order [D-03]
- The 24 drifters nearest home have voice prints over 4.19 dB apart in every pair (two magnetospheres' median) [D-03]
- At x1 the receiver is C-07's (nothing under the signal) and unworn within 200 ly (`radarWear`); a new receiver effect is 0 at x1 [C-15], [C-16]
- Gain raises the noise, not the loudness: the noise bed keeps C-07's rms at every gain [C-16]
- The phantom word (x25, x32) comes only from a recording in `Guide::heard`, in its voice, and is never logged [C-16]
- Every signal kind renders without extra data (a `SIG_ROGUE` without `RogueVoice` gets a giant's voice); a new kind adds a synth case [D-02], [D-03]

## The tape and its deck
- The tape bypasses the synth (mono, 22,050 Hz): raylib's music stream plays it beside the synth's (MP3, OGG, WAV, QOA; no FLAC) [R-420]
- A platform thread pumps the tape every 5 ms; raylib loops the stream, and the thread stops it 30 ms before the end (a raylib stop drops blocks) [R-420]
- The game drives the tape only through `AudioState`'s `music*` fields [R-420]
- The tape is `music/` in the working directory if that holds a track, else beside the executable (`platformAppDir`) [R-420]
- A tape plays `playlist.txt` first (order, author and title by name only, a gain to -16 LUFS, none measured), then unlisted files [R-420]
- The cassette deck is synthesized, never sampled, each change seeded a few per cent off; the track starts as the tape moves (`deckTapeAt`) [R-420]
- While the tape or deck sequences play, synth sounds duck to a fixed 0.35 over half a second: a level, not a compressor [R-420]

[C-04]: log/1-1.1.0.md
[C-05]: log/1-1.1.0.md
[C-07]: log/1-1.1.0.md
[C-13]: log/1-1.1.0.md
[C-11]: log/3-1.3.0.md
[C-15]: log/3-1.3.0.md
[C-16]: log/3-1.3.0.md
[C-17]: log/3-1.3.0.md
[H-04]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[H-05]: log/3-1.3.0.md
[B-413]: log/3-1.3.0.md
[R-414]: log/3-1.3.0.md
[F-01]: log/4-1.3.1.md
[F-07]: log/4-1.3.1.md
[F-08]: log/4-1.3.1.md
[R-420]: log/5-1.3.2.md
[Q-03]: log/6-1.4.0.md
[D-02]: log/6-1.4.0.md
[D-03]: log/6-1.4.0.md
