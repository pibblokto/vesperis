# Rules: the genes and the gene editor

Rules for changing a species' genes aboard (1.5.0, R-443: Y-02 to Y-04), one line per rule, linked to its log entry; replaced rules
go to `superseded.md`. Where life is: `rules-life.md`; incubator and collection: `rules-life-cultures.md`; sequencer, tree and
record: `rules-life-record.md`.

## The genes
- Genes are read from what a species is (values, form width and length) and its genesis, so nothing of generation 17 moves [Y-02]
- A genesis has one map for all its kin, 18 places: 3 vital, 3 quiet, warmth, salt, size, pigment, food, motion, 3 senses, rest, glow, shape [Y-02]
- A genesis code weights each letter (home's A 0, T 1, C 2, G 3) and has tables with each value at least once, on its own salts [Y-02]
- A species has a gene for each thing it uses, a gap where it lacks one; read by the genesis code, the wild letters give its values [Y-02]
- An unedited line is its species bit for bit: values, culture segments and slide kinds (the unit checks forty worlds) [Y-02]
- An edit moves a value from the parent's by the change in the code: warmth one step per weight, size doubling every three weights [Y-02]
- A cut gene loses its work: warmth only round the best, fresh water, half the size, no pigment, food, motion, sense, rest or glow, a sphere [Y-02]
- A line with a vital gene changed or cut does not live (its cells lyse in seconds, `GF_UNVIABLE`); a quiet gene changes nothing [Y-02]
- Edits never change how a species lives or its plan's family; a shape stays among a small cell's four, a motion among its plan's, values in limits [Y-02]
- A strain's genes are its parent's with the gene of its move stepped that way, so the sequencer reads which gene moved [Y-02]

## The editor and its lines
- The gene editor is its own machine (`EDITOR`, E at the bow bench); it takes a culture the sequencer read, in a flask or kept, no other [Y-02]
- An edited line is a strain of its species: the `strain` line holds SM_EDIT and the edits, numbered as the species' next, drawn E [Y-02]
- A line is read everywhere a strain is: in its flask (culture and form), the drop, the sequencer, the collection and the world's tree [Y-02]
- W asks for a folder (the parent's first), then a name (the parent's and the new number); writes in EDIT_S into an empty flask, kept there [Y-02] [R-445]
- Letters change by the arrows only (P and T are the game's own keys: a photograph, the warp); edits stay on the editor once written [Y-02]
- A gene the parent cut stays cut in its lines (to restore it, edit the parent); M can move a gene into it, or into a gap [Y-03]
- An edited line's answers on the slide belong to the line, never to its species' sheet [Y-02]

## Genes from others
- K puts a kin beside the line (a species or kept culture that was read); another genesis is refused for its alphabet, hand or map [Y-03]
- M moves the kin's gene at the chosen place in, with the kin's letters and value (range, band, width, share of pace, crowd, length) [Y-03]
- A moved gene keeps whose it was (its origin, not its carrier) and how far it came, down the lines; the donor is read again each time [Y-03]
- D copies a gene, with its letters, to the first free place of its kind (a gap, or one cut out), to be changed by hand [Y-03]
- A foreign gene slows growth by how far it came: 3% from its world, 6% its system, 10% another star (`burden`, of the best rate) [Y-03]
- A motion past the species' own pace costs 4% per doubling (5% if it was still); a foreign glow 4%, and lighting at a smaller crowd 2% per halving [Y-03]
- What a gene needs: a sense needs motion, a food a chemical-eater, a pigment's light a light-feeder, a shape one of the four small plans [Y-03]
- On the world's tree a moved gene is a dotted amber line from the donor's row to the line's, both on the page [Y-03]

## The switches
- S sets the chosen gene's switch (ALWAYS, NEVER, IN THE LIGHT, IN THE DARK, WHEN CROWDED, WHEN STARVING, WHEN COLD), kept down its lines [Y-04]
- A gene's own switch: a glow WHEN CROWDED, a rest WHEN STARVING (ALWAYS acts the same), others ALWAYS; a moved gene has its donor's, a copy its source's [Y-04]
- A gene switched off (its condition failing, or NEVER) works as if cut out and costs nothing; its letters and switch stay [Y-04]
- Conditions: lit, colder than the best, starving where the line as written cannot grow (cold aside), population over the quorum (the glow's, or 3%) [Y-04]
- A switched culture is laid out closed-form by state: light and cold fixed for a stretch, growth changing when the population passes the quorum [Y-04]
- A switched glow lights from the first cells while on, priced as at its least crowd; a switched size, shape, motion or sense shows only in the drop [Y-04]
- A drop is taken in the flask's state, and line and parent are compared in that state; a gene switched alone and on is never NOTHING SEEN [Y-04]
- A culture's lag comes from its species (Baranyi's h0, before the genes' price), so a line that pays nothing is exactly its species [Y-04]

## Learning what a gene does
- A gene is NOT KNOWN until seen; the guide keeps what was seen (`gene` lines, by genesis and number, so it holds for all kin) [Y-02]
- An edited line is compared with its parent: alone in a flask by where it grows, how it ends and the glow; in a drop by shape, size, colour, motion [Y-02]
- Also by its answers to light, food and the field where the parent's would differ, and by a vital gene's death at once [Y-02]
- Candidates are the changed genes that could show it (not known to do something else; NOTHING SEEN ones not for a drop): one is learned, more said [Y-02]
- Grown and looked at with nothing different, a line's unknown changed genes become NOTHING SEEN; when a strain is read, its move's gene is learned [Y-02]
- A finding is logged at once (GENE: what was seen, what it says) and said in turn, two lines of 51 characters at a time [Y-02]
- A gene that cannot work in its line is never NOTHING SEEN; when a known gene shows nothing, what it needs is said once per flask [Y-03]

[Y-02]: log/7-1.5.0.md
[Y-03]: log/7-1.5.0.md
[Y-04]: log/7-1.5.0.md
[R-445]: log/7-1.5.0.md
