# R-431: the decoder and the player apart

status: done 2026-11-18 (1.5.0, before its release; one of five requests aboard, R-427 to R-431)
milestone: 1.5.0

## Request

The user, 2026-11-18: "Finally - in shards decoder room you can access either player or decoder but once you access one you can just
switch with tab - no need to do that, make decoder and player (or how is it called) two distinct interfaces and categorize by planet in
the player please BUT make it only display music from planets please"

What was there (C-11, I-04): the player was a page of the decoder's screen, Tab at the decoder's worlds turning to it (and the deck
opening the decoder on it); its list held every piece, every recording held and what the radar caught, by world and year in one list.

## Done

- The player is its own screen (`PLAYER`), at its deck in the library; the decoder keeps its worlds and records, no Tab between.
- It opens on its worlds, a folder each (the world playing, else the one under the ship), each with its pieces' count and its
  peoples; Enter opens a world's music, Enter plays a piece and the list on from it, Space pauses, Backspace stops, Esc goes back to the
  worlds and from them closes (the player plays on).
- Music alone: the pieces held and the ones the radar caught (played as kept off the air); the recordings and texts stay the
  decoder's.
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-431; the unit's `player` group; the flow's `aboard_deck`; the tour.
