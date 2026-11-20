# Rules: the living small

The rules in force for the microorganisms of 1.5.0 (`src/life/`, R-425): where life is (Z-01), what it looks like (Z-02), its genomes
(Z-06); a line each with its entries (`log/`); a replaced one moves to `superseded.md`. GEN_VERSION, the draws and the salts:
`rules-galaxy.md`; the search and the lab: `rules-life-lab.md`; the incubator and the collection: `rules-life-cultures.md`; the sequencer,
the tree and the record: `rules-life-record.md`; the genes and the gene editor: `rules-life-genes.md`.

## Where life is
- Every landable kind has a row of `BIOTA` (built at compile time, held against `KINDS` by the unit); a giant, a brown dwarf and the companion none [Z-01]
- The biota draws on salts of its own after every draw of generation 16 (17 is it): no star, system, body, map or ground moves [Z-01]
- A world shows its stage and at most a second: fossils in old rock, or the protocells under spores carried to it [Z-01]
- The furnaces (molten, volcanic, tectonic, magma ocean, chthonian, glass, steam) hold nothing and keep nothing carried [Z-01]
- No life in a giant's clouds (X-03); none fed by light in the dark or under ice; the venusian's ground none, its clouds' life [Z-01]
- The active stages need the chemistry's liquid at the world's mean, unless its life is seen on its ground (flora, fauna, mats, vents) [Z-01]
- A world with flora is colonial; a people's world reached large cells at least; a dead world sleeps over its fossils [Z-01]
- A world whose life shows on its ground (flora, fauna, mats, vents) lives, glassed or not; another glassed world has none of its own [Z-01]
- "Any planet": every landable kind but the furnaces shows something somewhere (the unit over 1,500 systems and 40 drifters) [Z-01]

## Species
- A living world has 3-16 living species, a dead world 1-4 spores, a prebiotic world 2-4 kinds of protocell (no genome, no genesis) [Z-01]
- A species draws from its own stream (its world's seed, `SALT_SPECIES`, its index): a field added at a stream's end moves no other [Z-01]
- A world's list keeps its indices: its own, then its fossils, then the spores carried to it; the enums append (saved keys from Z-04) [Z-01]
- The first species take each livable habitat once in a hashed order; the rest by weight; a living world keeps one spore former [Z-01]
- A pigment follows the sun its world lives by (Kiang 2007) and its niche (the halophiles', the snow algae's, the mats') [Z-01]
- Nothing is named here: the names are the explorer's (Z-04); no count of a world's species is ever shown [Z-01]

## Geneses and kin
- A system's living worlds of one chemistry share a genesis; one system in five its water life its kin cell's (64 sectors) [Z-01]
- A cell holds one or two geneses, its first the oldest; the other chemistries are each a system's own [Z-01]
- A drifter's life is the star's it left (`drifterPast`), a wanderer's its own; a drifter's airless and icy bodies its spores one in three [Z-01]
- Home's genesis is pinned (`HOME_GENESIS_ID`: its cell's oldest, ours in hand, four letters); the ship's species are of it [Z-01]
- A genesis's handedness, letters, style and age come from its id; mirrored is against home's [Z-01]

## Carried spores
- Impacts carry a living world's spore formers to its neighbours that keep them (airless or icy, no life of their own, no wanderer) [Z-01]
- Neighbours are one family (a world and its moons) or worlds of one sun with none of its order between them [Z-01]
- A carried spore is its source's species (seed, plan), dormant, with `fromBody` and `fromIndex`; its world takes the source's genesis [Z-01]

## The cache
- `lifeOf` draws a system's biotas together and keeps sixteen, keyed by the star and every body's seed, kind, parent and glassing [Z-01]

## Forms
- A form is a pure function of its species and, on a world, of the forms before it; nothing of it is saved; each group its own stream [Z-02]
- No two of a world's forms within `MORPH_ALIKE` (2.5, the pictures' evidence); a carried spore is its source's form, as drawn there [Z-02]
- A genesis's style (symmetry, mineral, flagellum, hand) is its kin's; a mirrored genesis's helices and spirals turn the other way [Z-02]
- Under 1.5% of 1,000 species' pictures alike (C-17's measure, 96 px in a dark field); the battery's `life_look` keeps the measure [Z-02]
- Every plan's variants (75 forms) in every act stay within `specimenExtentUm` at every zoom (the unit) [Z-02]

## The slide
- A slide is drawn in a framebuffer of its own: every bank the slide's, the ramps its way of seeing's; the mush the caller's [Z-02]
- A slide is a pure function of its view, its specimens and the clock: no state between frames, the same at any thread count [Z-02]
- A way of seeing keeps to its banks (`seeingBanks`, 26 at most); its ramps meet the field's at the field's shade [Z-02]
- Thin things are sized in screen pixels (a strand `g_mushKernel + 1` wide, a dot that radius) and fade as they widen [Z-02]
- The ways show what the real ones do: no flagella in a bright field, DNA blue, plastids red, spores phase-bright, the gel no electron's [Z-02]
- Fossils are carbon outlines in the section's rock, which goes to its mean where its grains are small on the screen [Z-02]

## Genomes and the tree
- A genome is its species' and genesis's alone: the alphabet and the hand the genesis's, the marker changed down its lineage, salts of its own [Z-06]
- The lineage: deep branches, a kin cell's boxes (16 and 4 sectors), a star's line (a drifter's: its old star's), the last exchange, a comb [Z-06]
- Within a deep branch the distances rank as the lineage (a world's, a system's, a cell's); none across geneses; a carried spore its source's leaf [Z-06]
- The tree's order keeps every clade a run of neighbours (`treeBefore`): two leaves' common ancestor the oldest join between them [Z-06]
- A late people's world's made species is one of its own by the world's hash (`peoples/made`, the peoples' one reader of life); its text a record [Z-06]

[Z-01]: log/7-1.5.0.md
[Z-02]: log/7-1.5.0.md
[Z-06]: log/7-1.5.0.md
