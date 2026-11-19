# R-430: the ship quiet but its hum

status: done 2026-11-18 (1.5.0, before its release; one of five requests aboard, R-427 to R-431)
milestone: 1.5.0

## Request

The user, 2026-11-18: "Also please remove all sounds you added except inital humming/buzzing we had from the very begginig."

The sounds added (I-04): each room's own share of the hum and the drive (the engine room's the loudest, the dome's the quietest), the
laboratory's bubbling, the garden's water, the player's music fuller in the library.

## Done

- Aboard, the ship sounds as 1.4.0 did in every room: the hum (lower under the player, the receiver's own under the radar) and the
  drive in a flight, both there from the first version. `audio.h`, `audio.cpp` and the `sounds` mode are 1.4.0's again, line for
  line; the plan's sound rows and `Game::aboardSounds` are gone.
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-430; the `sounds` item.
