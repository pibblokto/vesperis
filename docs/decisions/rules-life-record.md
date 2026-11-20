# Rules: the sequencer, the tree and the record

Rules for reading microbe genomes aboard and for the record of what was found (1.5.0, R-425: Z-06, Z-07; B-443; R-444: Y-01), one line
per rule with its log entries (`log/`); a replaced rule moves to `superseded.md`. Search and microscope: `rules-life-lab.md`;
incubator, jars and collection: `rules-life-cultures.md`; where life is: `rules-life.md`.

## The sequencer and the tree
- The sequencer, a screen over the room (`SEQUENCER`, E at the bow bench), reads a species seen in a vial or grown in a flask, not a fossil or protocell [Z-06]
- A reading takes SEQUENCE_S on the lab's clock and has a save line (`seq`); its end is told as the clock reaches it, with the screen open or not [Z-06]
- A read species gets kin on its sheet (`SEEN_KIN`), a KIN log line (nearest kin read and their ancestor, or geneses met); a made one keeps its watermark [Z-06]
- The tree (`TREE`) has one page per tree: the ship's own, each world with a species read, each folder holding a culture; there is no tree of everything [Y-01]
- On a page each genesis is a clade, on the page's own time scale. Enter makes a kin's star the remote target; the nearest kin read anywhere is told [Y-01]
- Lab lines appear as twigs under their parents on their world's page as they arise (Backspace off, A back: `treeoff`); a folder's page is what it holds [Y-01]
- A watermark sits after its people's records in the decoder; the language count leaves it out and the history never reads it [Z-06]
- On the tree an unnamed species shows look and size, two alike on a world told apart. The sequencer uses the tree's words; long lines drop the world [B-443]

## The record
- The catalogue (a row of the guide) lists every species with a sheet, by world or by kin, a friend's lent ones in cyan; nothing of it is saved [Z-07]
- A sheet's form is drawn live in the catalogue's own framebuffer (with the banks of its way of seeing), in a way it was seen in [Z-07]
- Every count of microbes is the explorer's own, in the other screens' words (the catalogue's rows, the tree's geneses) [Z-07]
- A sample is counted and logged (SAMPLE) as it is taken, with its world; auto-explore samples at the water and springs (no lab screen opened) [Z-07]

[Z-06]: log/7-1.5.0.md
[Z-07]: log/7-1.5.0.md
[B-443]: log/7-1.5.0.md
[Y-01]: log/7-1.5.0.md
