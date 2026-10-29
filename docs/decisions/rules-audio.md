# Rules: audio

The rules in force for the synth, the music, the voices and every sound, a line each with the entries behind them (`log/`, by release). A milestone that sets
or changes one edits its line here; a replaced one moves to `superseded.md` (`docs/CLAUDE.md`).

## The synth and its owners
- The synth renders on the main thread (`platformAudioPump`), no helper thread: a voice's cost is frame time [F-01], [F-08], [D-03]
- The synth reads a piece and a speech through `AudioState` pointers into their owner's copy; the owner clears them before it moves [C-04], [C-05], [C-11]
- The harness runs no synth: the game times a sound by its own clock (`pieceT`, `speechT`, `radarRepeatContent`); `speechDone` only ends one sooner [C-07]
- The decoder takes the synth: it, a flight, an arrival, the surface and a load switch the radar off; other screens pause the sweep [C-07]
- While the decoder, radar or gramophone plays, the ship's player is held, then resumes (`pieceFrom`, `speechFrom`): one piece at a time [C-11]
- `Game::darkRadio` plays a drifter on the cabin's and suit's radios only where no other owner has the synth: a new owner joins its list [D-03]
- `AudioState::surfaceOff()` zeroes every ground level (a new one joins it) each frame of descent, climb and space, and a load; ground menus keep them [B-413]

## Levels and the proof
- A change not meant to be heard keeps every WAV of the proof byte for byte (`AudioSynth::Coefs`: per-sample expressions) [F-07], [D-03]
- A sound's level is set from its render (`sounds`, `signals wav`) against the others' rms and peaks, not by ear [C-07], [C-16], [Q-03], [D-03]
- The mix clamps at 1 before the master's 0.8: a render peaking at exactly 0.80 has clipped, unseen by `RenderStats::clip` (the ground's bound: 0.75) [Q-03]
- Every receiver voice, overdriven too, keeps C-07's levels: an rms of 0.02-0.13, at most 20 samples clipped [C-07], [C-15], [D-03]
- A sound meant to be heard sits above what small speakers play: the quake's rumble at 80 Hz, not under 45 [Q-03]

## Voices and speech
- Voices are synthesized (`speechSample`), never sampled; a people's voice comes from its seed and tongue (`voiceOf`) [C-05], [C-13], [H-04 second pass]
- `PHONES` sounds every spelling the tongues write, longest first: a new spelling needs its row or goes unspoken [C-05]
- A text's speech is its decoded words one for one (`DecodedWord::theirs`, spans kept), whatever share of the language is read [C-05]
- On the air a record is its opening (`shardOpening`, 60 words), as the player replays a heard one; the decoder speaks the page shown [H-04], [C-11]
- A medium with a sound (`recordMediumSounds`) colours its record on the decoder, never on the air; else the ship renders it flat (`voiceRead`) [H-05], [R-414]

## Music and instruments
- A people's tradition comes from its seed and its lore's name style (`traditionOf`), a piece from its shard (`pieceOf`) [C-04], [C-13]
- `traditionOf` draws C-04's core first, bit-exact; the ages' layers and the instruments' makes draw on their own streams [H-05], [C-17]
- Scales: 5-9 notes, no neighbours under 75 cents, no hole over 450, mostly the octave: a tradition from elsewhere, never a noise experiment [C-04]
- Chords stack from the people's own scale (the degrees nearest a third and a fifth): a people without a third plays what it has [H-05]
- A found instrument sounds as its people's pieces, on their scale; its make never changes its timbre: case and decoder are one sound [C-11], [C-17]

## The radar and the drifters
- Signals and drifters sound of cosmic horror, never like music (the user's call): a piece on the air stays rare and slowed [C-07], [D-03]
- A signal sounds by its seed (`programmeOf`, `rvDerive`), a drifter by its seed and `RogueVoice`: the same twice, no two alike [C-07], [D-03]
- `rvDerive`'s indices are each kind's own (a drifter's from 161, `rogueDerive`): a new voice takes unread ones, `machineInto` adds in the old order [D-03]
- The 24 drifters nearest home have voice prints over 4.19 dB apart pairwise (two magnetospheres' median) [D-03]
- At x1 the receiver is C-07's (nothing under the signal), within 200 ly unworn (`radarWear`): a new receiver effect is 0 at x1 [C-15], [C-16]
- The gain lifts the noise, not the loudness: the bed's rms stays C-07's at every gain [C-16]
- The phantom word (x25, x32) is only from a recording in `Guide::heard`, in its own voice, and is never logged [C-16]
- Every signal kind renders with no extra data (a `SIG_ROGUE` without `RogueVoice`: a giant's voice); a new kind adds a synth case [D-02], [D-03]

## The tape and its deck
- The tape bypasses the synth (mono, 22,050 Hz): raylib's music stream plays it beside the synth's (MP3, OGG, WAV, QOA; no FLAC) [R-420]
- A platform thread pumps the tape every 5 ms; raylib loops it and the thread stops it 30 ms before its end (raylib's stop drops blocks) [R-420]
- The game drives the tape only through `AudioState`'s `music*` fields [R-420]
- The tape is the working directory's `music/` if it holds a track, else the executable's (`platformAppDir`) [R-420]
- A tape plays `playlist.txt` first (order, author and title by name only, a gain to -16 LUFS, none measured), then unlisted files [R-420]
- The cassette deck is synthesized, never sampled, each change seeded a few per cent off; the track starts as its tape moves (`deckTapeAt`) [R-420]
- While the tape is on, deck included, the synth's sounds duck to a fixed 0.35 over half a second: a level, not a compressor [R-420]

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
