# R-420: auto-explore's tape

status: done 2026-10-19
milestone: 1.3.2 (the seventh of the patch's items)

## Request

After asking about four MP3s they had downloaded: "Ok, now what I want you to do add - add these for to
auto-explore mode as an optional feature - you can enable playlist with these 3 files with M button while in autoexplore and it will
cycle. Use some 80s-90s cassete insertion sound between tracks. Investigate authorship properly so that when track starts author and name
of the track are displayed for a few seconds on top left and then fade away. Also when music plays all other sounds are quiter"

"These for" and "these 3": the four downloaded, all four on the tape.

## Authorship

The files carried no tags, so the credits come from where the tracks are published: each one's YouTube upload, whose description names
the release:

| File (as downloaded) | Video, channel | Credit |
| --- | --- | --- |
| `Forwards.mp3` (2:35) | "Forwards", Jarrad Wright - Topic | Jarrad Wright, *Forwards*, from *Sassy the Sasquatch (Original Motion Picture Soundtrack)*, released 2022-10-05 through DistroKid |
| `CI2 soundtrack - It's time to wake up - by Jarrad Wright.mp3` (6:23) | THE BIG LEZ SHOW OFFICIAL, 2015-09-29 | Jarrad Wright, *It's time to wake up*, from the soundtrack of *Choomah Island 2* |
| `CI2 soundtrack - BLUE-MEAN-EYES - by Jarrad Wright.mp3` (3:13) | THE BIG LEZ SHOW OFFICIAL, 2015-09-29 | Jarrad Wright, *BLUE-MEAN-EYES*, the same |
| `Planet Caravan (2009 Remaster).mp3` (4:29) | Black Sabbath - Topic | Black Sabbath, *Planet Caravan*, from *Paranoid* (1970), the 2009 remaster, ℗ 2009 Sanctuary Records Group Ltd., a BMG company; written by Osbourne, Iommi, Butler and Ward |

Jarrad Wright created The Big Lez Show (with Tom Hollis) and writes and performs its music. *Choomah Island 2* is the show's special of just
under an hour, premiered on 21 August 2015; *Sassy the Sasquatch* is its spin-off series of 2022.

## What was done

* **M while auto-explore plays** (`game/tape.cpp`; a controller's `SELECT`, whose own key would end it) starts and stops a tape of the
  explorer's own music. The rest of auto-explore's rule holds: any other key, click, motion or button gives the controls back.
* **The tape** is the `music` folder beside the game (the working directory: the checkout's for `./vesperis`, `~/Documents/vesperis`
  for the macOS app). Its `playlist.txt` gives the order and the credits: a track a line, `file | author | title | gain in dB`, `#` for a
  comment, a missing file left out. Without one, the folder's MP3, OGG, WAV and QOA files play in the order of their names, an
  `Author - Title.mp3` name read as such. The tracks play round and round.
* **The deck** (`game/audio_deck.cpp`), synthesized, an 80s-90s front-loader:
  * before the first track a cassette is put in (the eject key, the door let go on its spring, the cassette slid in with its reels
    knocking, the door pushed shut onto its latch) and played: the key, the mechanism's ka-chunk as the heads and the pinch roller come
    up, the capstan motor's whir, the tape's hiss. The music begins about 2 s after `M`;
  * at a track's end the auto-stop throws the key up and the motor runs down; the cassette comes out, the next goes in, the door shuts,
    play. About 3.4 s from one track to the next;
  * the play key alone (0.55 s) when a cassette was left in; the stop key for `M` again, the music cut at its moment.
  * Its parts are struck modes on a burst of noise (a plastic part's resonances, a body's thump, a spring's ring), a slide of filtered
    noise sticking and slipping, the motor an oscillator, two early reflections for a room. Each sequence is varied by a seed (the parts'
    pitches a few per cent, the hands' pauses 15%), so no two changes sound alike.
* **The credit**: as a track begins, its author (amber) and title (white) fade in at the top left over 0.4 s, stay five seconds and fade
  out over 1.5, laid over the scene (`drawTextBlend`). The chooser and auto-explore's first seconds say `M STARTS AND STOPS THE MUSIC`
  when there is music to play; `M` with none says so where a title would be.
* **The rest quieter**: under the tape the game's own sounds keep 0.35 of their level (about 9 dB down), eased over half a second; the
  deck rides over them.
* **Kept**: when auto-explore ends the music fades over 1.5 s. The choice and the place in the tape stay in the settings (`auto_music`,
  `auto_track`, `auto_track_at`), and the tape is taken up again with the play key, where it was, as auto-explore next begins.
* **The platform** (`platform_raylib.cpp`) plays the file through raylib's music stream, in stereo at its own rate, beside the synth's
  mono stream at 22,050 Hz. A thread of its own feeds it every 5 ms and ends each track 30 ms short of its end. raylib, told not to loop,
  stops a track as it reads its last block, with that block and the one playing still unheard; and the game's frames can stall past
  the stream's buffers. The game asks and hears back through `AudioState`.
* **The user's four**, copied into `music/` (gitignored) under tidy names with a `playlist.txt` in the order downloaded, evened to
  -16 LUFS (EBU R128 by ffmpeg: *Forwards* -14.9 LUFS, gain -1.1 dB; *It's time to wake up* -26.5, +10.5; *BLUE-MEAN-EYES* -21.4, +5.4;
  *Planet Caravan* -15.7, -0.3). They are copyrighted, so they are neither committed nor shipped: the game plays a player's own folder
  and carries none.

## Verification (2026-10-19)

* `vesperis_test unit`, two new checks:
  * the tape played through with the platform's side stood in (`TapePump`). `M` puts a cassette in, and the track starts 2.10 s later
    (the deck's 2.12), at its playlist gain. The caption is 0.08 a frame in, whole at a second, gone at 7.5 s. The rest ducks to 0.35.
    At the end the next cassette goes in, 3.30 s to the next track (3.30), with its own credit and gain, and then round to the first.
    The stop key cuts the music with its place kept (4.0 s); the play key takes it up from there. The controller's `SELECT` works the
    same with auto-explore on. `G` ends auto-explore: the music fades (0.689 to 0.439 in half a second, then off), the choice is kept,
    and the next `autoStart` takes the tape up from its place. A file gone mid-tape is passed over to the next; an empty folder is said.
  * the deck's four sequences. Insert: the tape at 2.08 s of 3.83, 7 onsets. Change: 3.32 s of 5.07, 11 onsets. Play: 0.50 s of 2.25.
    Stop: 0.03 s of 0.73. Peaks 0.47 to 0.63. The play key's knock comes before the tape moves and the hiss after it. Each is alike for
    a seed and not for two. Through the synth the deck comes out as rendered (within 1.2e-8), the hum under it at 0.350 of itself
    block for block, and -1 hushes it.
* `vesperis_test tape` (new, `08-testing-and-tools.md`) reads the user's `music/` (the four, credits and gains as above), writes the
  deck's sequences to `shots/tests/tape_deck_*.wav`, and draws the caption: `tape_01`-`04` fading in, whole, fading out, gone; `tape_05`
  the next track's; `tape_00` the chooser's line; `tape_06` the empty folder's words.
* The platform's stream through raylib 5.5, a scratch build linking `platform_raylib.cpp` with no window and at a level of 0:
  * a 2 s tone ends after 2.005 s without going round;
  * the four tracks load with their lengths within 0.07 s of ffprobe's;
  * a place of 300 s sounds after 0.16 s and plays on from there;
  * a stall of the main thread for 0.8 s finds the music 0.83 s on (the thread fed it);
  * a place at the very end is over at once;
  * a missing file is reported.
* Not heard in the window yet: the deck's sounds and the balance of the music against the game (the tape's level 0.8, the duck 0.35)
  are set by measurement; the WAVs are there to listen to (KI-418).
