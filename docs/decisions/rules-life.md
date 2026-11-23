# Rules: the living small

Rules in force for 1.5.0's microbes (`src/life/`, R-425): where life is (Z-01), what it looks like (Z-02) and its genomes
(Z-06), one line per rule with its log entries (`log/`); a replaced rule moves to `superseded.md`. GEN_VERSION, draws and salts:
`rules-galaxy.md`; search and lab: `rules-life-lab.md`; incubator and collection: `rules-life-cultures.md`; sequencer, tree and
record: `rules-life-record.md`; genes and gene editor: `rules-life-genes.md`.

## Where life is
- Every landable kind has a `BIOTA` row (built at compile time, held against `KINDS` by the unit); giants, brown dwarfs and the companion have none [Z-01]
- The biota draws on its own salts after every draw of generation 16 (it made 17), so no star, system, body, map or ground moves [Z-01]
- A world shows its stage and at most one more: fossils in old rock, or protocells under spores carried to it [Z-01]
- The furnaces (molten, volcanic, tectonic, magma ocean, chthonian, glass, steam) hold nothing and keep nothing carried [Z-01]
- No life in a giant's clouds (X-03), none fed by light in the dark or under ice; a venusian world's life is in its clouds, not on its ground [Z-01]
- The active stages need the chemistry's liquid at the world's mean, unless life is seen on the ground (flora, fauna, mats, vents) [Z-01]
- A world with flora is colonial, a people's world reached large cells at least, and a dead world sleeps over its fossils [Z-01]
- A world whose life shows on the ground (flora, fauna, mats, vents) lives, glassed or not; any other glassed world has no life of its own [Z-01]
- "Any planet": every landable kind but the furnaces shows something somewhere (the unit over 1,500 systems and 40 drifters) [Z-01]

## Species
- A living world has 3-16 living species, a dead world 1-4 spores, a prebiotic world 2-4 kinds of protocell (no genome, no genesis) [Z-01]
- A species draws from its own stream (world seed, `SALT_SPECIES`, index), so a field added at a stream's end moves no other [Z-01]
- A world's species keep their indices: its own, then its fossils, then spores carried to it; the enums only append (saved keys from Z-04) [Z-01]
- The first species take each livable habitat once, in a hashed order, the rest by weight; a living world keeps one spore former [Z-01]
- A pigment follows the sun its world lives by (Kiang 2007) and its niche (halophiles, snow algae, mats) [Z-01]
- Nothing is named here: the names are the explorer's (Z-04); no count of a world's species is ever shown [Z-01]

## Geneses and kin
- A system's living worlds of one chemistry share a genesis; in one system in five, water life takes its kin cell's genesis (64 sectors) [Z-01]
- A cell holds one or two geneses, the first the oldest; other chemistries each have a system's own [Z-01]
- A drifter's life comes from the star it left (`drifterPast`), a wanderer's is its own; one in three airless or icy drifter bodies holds its spores [Z-01]
- Home's genesis is pinned (`HOME_GENESIS_ID`: its cell's oldest, our handedness, four letters); the ship's species belong to it [Z-01]
- A genesis's handedness, letters, style and age come from its id; mirrored means mirrored against home's [Z-01]

## Carried spores
- Impacts carry a living world's spore formers to its neighbours that keep them (airless or icy, no life of their own, no wanderer) [Z-01]
- Neighbours are one family (a world and its moons) or worlds of one sun with no other of their order between them [Z-01]
- A carried spore is its source's species (seed, plan), dormant, with `fromBody` and `fromIndex`; the world it reaches takes the source's genesis [Z-01]

## The cache
- `lifeOf` draws a system's biotas together and caches sixteen systems, keyed by the star and every body's seed, kind, parent and glassing [Z-01]

## Forms
- A form is a pure function of its species and, on a world, of the forms before it; nothing is saved, and each group has its own stream [Z-02]
- No two forms of a world lie within `MORPH_ALIKE` (2.5, set from the pictures); a carried spore keeps its source's form, as drawn there [Z-02]
- A genesis's style (symmetry, mineral, flagellum, hand) is shared by its kin; a mirrored genesis turns helices and spirals the other way [Z-02]
- Under 1.5% of 1,000 species' pictures look alike (C-17's measure, 96 px in a dark field); the battery's `life_look` keeps the measure [Z-02]
- Every plan's variants (75 forms) in every act stay within `specimenExtentUm` at every zoom (the unit) [Z-02]

## The slide
- A slide is drawn in its own framebuffer: every bank is the slide's, the ramps are its way of seeing's, and the caller applies the mush [Z-02]
- A slide is a pure function of view, specimens and clock: no state between frames, the same at any thread count [Z-02]
- A way of seeing keeps to its banks (`seeingBanks`, 26 at most), and its ramps meet the field's at the field's shade [Z-02]
- Thin things are sized in screen pixels (a strand `g_mushKernel + 1` wide, a dot that radius) and fade as they widen [Z-02]
- The ways of seeing show what real ones do: no flagella in a bright field, DNA blue, plastids red, spores phase-bright, no gel under the electron [Z-02]
- Fossils are carbon outlines in the section's rock, which fades to its mean where its grains are small on the screen [Z-02]

## Genomes and the tree
- A genome depends only on its species and genesis: the genesis gives alphabet and hand, the marker changes down the lineage, on its own salts [Z-06]
- The lineage: deep branches, a kin cell's boxes (16 and 4 sectors), a star's line (for a drifter its old star's), the last exchange, a comb [Z-06]
- Within a deep branch distances rank as the lineage does (world, system, cell), never across geneses; a carried spore is its source's leaf [Z-06]
- The tree's order keeps every clade a run of neighbours (`treeBefore`): the common ancestor of two leaves is the oldest join between them [Z-06]
- A late people's made species is one of their world's own, by the world's hash (`peoples/made`, the peoples' one reader of life); its text is a record [Z-06]

[Z-01]: log/7-1.5.0.md
[Z-02]: log/7-1.5.0.md
[Z-06]: log/7-1.5.0.md
