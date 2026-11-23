# R-443: the genes

status: done 2026-11-20 (1.5.0, before its release: the Y series, Y-02 to Y-04)
milestone: Y-02, Y-03, Y-04

## Request

The user, 2026-11-19, after asking what the sequencer is for: "Would be cool if there was a mechanic for changing genes so that it can
have different characteristics, behaviour, etc. [...] As for modification of genes - make it smart but not boring so that you can somehow
understand and learn what affects what, can experiemnt, changes behaviours, etc". Talked through (a sketch of genes read, cut, moved
between kin and switched, the editor's screen to scale, five calls), they answered: "Good, proceed, regarding decisions though: 1. New
machine for editing 2. Found versions and saved cultures (flasks). Also there should be ability to save modified organisms back to flasks
as new cultures 3. Relatives can give a gene AND you can manually change it/modify so you can create something that never existed
completely (and so that you don't need any relatives to modify and create new organisms, so that you can just get one organism and create
tons of variations you like from it) [...] 5. It's all 1.5.0, we are extending it".

## Plan

The design is `docs/archive/ideas/IDEAS-genes.md` section 1; the milestones and their checks `docs/archive/ideas/ITERATION-1.5.0.md`,
Phase 5.

- **Y-02, the gene editor and a species' own genes:** genes read from what each species is (nothing generated moves), a genesis's map
  and code, the working parts changed by hand and genes cut on a machine of its own, the edited line written into a flask as a new
  culture, read everywhere a strain is; what a gene does learned by seeing an edit against its parent, a vital gene, a strain read.
- **Y-03, genes from others:** a gene moved in from a species read or a culture kept of the same genesis (refused across geneses), its
  price by the donor's distance, a gene duplicated, what a gene needs; the donors' lines on the trees.
- **Y-04, the switches:** when a gene is on (always, never, in the light or the dark, crowded, starving, cold), in the culture's
  segments and the drop; a gene off costs nothing.

## Done

- **Y-02, the gene editor and a species' own genes** (2026-11-20): a species' genes read from what it is (`life/genes`: a genesis's map of
  eighteen places and its code, every kin's; a gene for each thing it uses; its wild letters its own values; nothing generated moves, an
  unedited line its species bit for bit); the gene editor in the laboratory (`EDITOR`, on the bow bench past the sequencer): a culture read
  put on, its genes on the map NOT KNOWN till seen, a gene's letters changed by hand or the gene cut, W writing the line in two minutes into
  an empty flask, a strain of its species read everywhere a strain is (E1 where a strain is S1); what a gene does learned as an edited line
  shows it against its parent (its flask's warmth, salt, food, light, end and glow; its drop's shape, size, colour, motion and answers; a
  vital gene's death; nothing seen) and as a strain is read. Proof and records: `docs/decisions/log/7-1.5.0.md`, Y-02;
  `docs/decisions/rules-life-genes.md`; the unit's `genes` group; `lab genes` (`shots/tests/lab_genes.png`).
- **Y-03, genes from others** (2026-11-20): K puts a kin beside the culture on the editor (a species read or a culture kept; one of
  another genesis refused with its words), the map counting where their letters differ; M moves the kin's gene in (its letters and its
  value the kin's), over its own gene, into a gap or a gene cut out; D copies a gene to a free place of its kind, to be changed by hand. A
  gene not its own slows its line by how far it came (its world's kin 3%, its system's 6%, another star's 10%; a faster motion and a glow
  cost too): its doubling shows it, beside its parent it loses. What a gene needs (a sense motion, a food or a pigment's light its way of
  living, a shape the small cells' plans) is said when seen of a gene known; the tree draws a moved gene's line from its donor. Proof and
  records: `docs/decisions/log/7-1.5.0.md`, Y-03; `docs/decisions/rules-life-genes.md`; the unit's `genes` group; `lab genes`
  (`shots/tests/lab_genes_kin.png`).
- **Y-04, the switches** (2026-11-20): S on the editor sets a gene's switch (ALWAYS, NEVER, IN THE LIGHT, IN THE DARK, WHEN CROWDED, WHEN
  STARVING, WHEN COLD; a glow's own WHEN CROWDED, a rest's WHEN STARVING). Off, a gene loses its work as one cut and pays nothing; the
  culture's laws change where the condition does (its light, the cold against its best, starving, its crowd's share), its drop as its
  flask has it: a glow IN THE DARK lights a dark flask from its first cells, a rest IN THE LIGHT rests lit and dies in the dark, a vital
  gene IN THE LIGHT is a kill switch. With it B-444 (a culture held at its crowd for good). Proof and records:
  `docs/decisions/log/7-1.5.0.md`, Y-04; `docs/decisions/rules-life-genes.md`; the unit's `genes` group; `lab cultures`, `lab genes`
  (`shots/tests/lab_genes_switch.png`).
