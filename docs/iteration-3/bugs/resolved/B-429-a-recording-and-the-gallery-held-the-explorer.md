# B-429: a recording and the gallery held the explorer

status: resolved 2026-10-26 (found the same day planning U-05 as KI-427; fixed on the user's word)
severity: S2
area: the recording's screen (`game/probe_records.cpp`, `updateRecording`) and the gallery's way back (`guide_screens.cpp`)

## Report

Found while planning U-05 (`docs/archive/ideas/PLAN-screens.md`, section 9) and offered as KI-427: naming a storm in a probe's recording
(N) leaves `guideReturn` at RECORDING. From then on Esc or Enter goes from the recording to the gallery, and from the gallery back
to the recording. The explorer is held there until they quit. Since X-04 (1.2.0). The user: "Delete old snapshots from e02 to e12,
fix KI-427."

## Cause

The gallery has no way back of its own. Its Esc and Enter go to `guideReturn` (`updateGallery`), which the guide menu sets as it
opens: SPACE, or SURFACE on the ground (`openGuide`). A text entry goes back by the same variable (`updateTextEntry`), so a screen
that names something sets `guideReturn` to itself first. The probe's screen, the decoder, the star map's chart and the sector map
do so, and each of them leaves for a fixed state (SPACE or SURFACE), so the value they leave behind is never read.

The recording is the one naming screen opened from a screen that goes back by `guideReturn`. Its N (X-04) set the variable to
RECORDING for the storm's entry, and nothing set it back. So the recording's Esc went to the gallery, and the gallery's Esc went to
RECORDING. A cancelled entry (Esc) held the explorer as surely as a name did.

## Fix

The recording sets the gallery's way back again as it returns to it, on Esc, on Enter, or when its record is gone:
`guideReturn = GameState::SPACE`. That is the ship's, because the gallery opens a recording only there. On the ground it refuses
("THE RECORDINGS PLAY ON THE SHIP'S SCREEN"), so a gallery under a recording was always opened over SPACE.

No variable was added. U-05 gives every screen its own way back (`openScreen`, the row's `back`) and folds `guideReturn` into it.
Before that fold, `guideReturn` again holds only the guide's scene, or a naming screen's own state while its entry is open.

## Verification (2026-10-26)

A unit check in the probe group, "a storm renamed in its recording, then Esc to the gallery and to the ship (B-429)":
- It opens the gallery, plays the recording of the probe that fell into the great storm, and presses N.
- It clears RED EYE, types OLD RED, and presses Esc twice.
- Before the fix it failed (`RECORDING, N TEXT_ENTRY, renamed 1, RECORDING; Esc GALLERY, Esc RECORDING`).
- After it, it passes (`... Esc GALLERY, Esc SPACE`), and the probe group has 15 checks with 0 failures.

`tools/prove compare e13` found 27 of its 28 items the same. The unit differs only where the new check writes: its line, the
count (405 checks and 0 failures, against 404), and its own changes to `test_x04/guide.txt` (the storm's new name and the
`NAMED A STORM OLD RED` line). `make check` passes. `tools/prove snapshot b429` is the U series' base.

In the game: on the ship, G, GALLERY, then a recording of a probe that fell into a storm. N names the storm; Enter. Esc returns to
the gallery, and Esc again to the cabin.
