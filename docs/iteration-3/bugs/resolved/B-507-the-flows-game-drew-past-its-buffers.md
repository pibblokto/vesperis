# B-507: the flow's game drew past its buffers after the reload's games

status: resolved 2026-11-23 (J-16)
severity: S4
area: the harness's flow (`harness/modes_game.cpp`: `Flow::own`, `run`, `press`, `shot`); the frame's size, which is the process's
(`core/framebuffer`: `FBW`, `FBH`, `FB_SCALE` and the mush's kernel, set by `Game::applySettings`)

## Report

KI-450, found in Y-02 (2026-11-20): "The flow's game, run on after the reload made and destroyed games of their own, broke its heap
(5 of 5 runs, its cabin's frames alone; freed at its end). Its last stage now runs before them; one game alone (the game's) never met
it."

## Cause

The frame's size belongs to the process, not to a game. `FBW`, `FBH`, `FB_SCALE` and the mush's kernel are globals, set by whichever
game last applied its settings (`Game::applySettings`), and a game makes its buffers at the size set then. The game itself holds one
`Game`, so its size and its buffers always agree.

The flow holds two more beside its own. Its game runs at the test scale, 1x: its framebuffer is 320 x 200 pixels. Its last two stages
make games of their own (auto-explore's samples; the reload's two loads), and a new game applies its default settings in its
constructor: 4x, 1280 x 800. When they end, the process stays at 4x, and a frame of the flow's game after them drew a 4x frame into its
1x buffers.

An address-sanitised build (the default build's flags with `-fsanitize=address`, in an object directory of its own) ran the flow with
the gene editor's and the seeding's stages after the reload, the order Y-02 first had. It stopped at the gene editor's first frame:
"heap-buffer-overflow", a read 400 bytes past the 256,000 bytes of the flow's game's depth buffer (`Framebuffer::invz`, 64,000 floats,
made by `Flow::title` at 1x), in `rasterProjected` (`core/raster.cpp:105`) under `ShapeList::fill` on the pool, drawing the cabin
(`Game::drawCabin`). In the flow's order since Y-02 nothing of the flow's game is drawn after those stages, so nothing broke.

## Fix

The flow's game takes its own size back before its frames. `Flow::own` applies its settings again when the process's frame size is not
its buffers' (`game.fb.idx.size() != FBW * FBH`), and `run`, `press` and `shot` call it first. In the flow's order it never applies
anything (the size is already its own), so no frame moves; in any order its frames are drawn at its own size. The stage order Y-02 set
stays, as its frames are the battery's.

Not done: a size of each game's own. The globals are read all over the drawing code, and only the harness ever holds two games.

The rule (`rules-harness.md`): a game made beside the flow's sets the process's frame size; the flow's game takes its own back before
its frames, so a stage may follow one.

## Verification (2026-11-23)

- **The sanitised build, the flow with its gene editor's and seeding's stages after the reload** (a switch of the scratch build, taken
  out since). On the old code: "AddressSanitizer: heap-buffer-overflow ... raster.cpp:105 in rasterProjected", exit 1. With the fix:
  the flow ran to its end in that order, the gene editor writing its line into flask 2 and the seeding taking (RED SEAS), 197
  pictures, no report, exit 0.
- **The sanitised build, the flow in its own order:** no report and exit 0, before the fix and after.
- **The default build, the flow in its own order, the old binary against the new:** its 197 pictures and the gallery's three the same
  byte for byte, its printout the same but for its times. `make check`.
