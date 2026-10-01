# Plan: a galaxy of two hundred billion stars

Status: proposed 2026-09-30. **G-01 done 2026-10-01 (generation 11)**: the constants live in
`galaxy/starfield.h`; the implemented values differ from the table below (disc peak 0.68, four
logarithmic arms of 12 degrees pitch instead of two Archimedean ones, and the centre at sector
(12000, 0, -5500) with home kept at its sectors, so the harness scans and pins kept their
coordinates). **G-02 done 2026-10-01**: 145 globulars in a halo distribution and open clusters along
the arms replace the knots, the bulge region reaches 2,600 ly, the home star is pinned (Heilya, sector
195 0 46) with the search as the fallback. **G-03 done 2026-10-01**: the band integrates the starlight behind
the dust on a fixed exposure (a 128 x 64 map, the rift at the core, the night sky on the ground shows it in its own
bank) and the star-forming regions are complexes 42 ly apart on average, none in home's sky. **G-04 done 2026-10-01**:
the mountain site re-pinned, the scene finders get the light they ask for (seasons, binaries, locked worlds), the flow
flies again (the lock in the cabin, a land site, a living world), `regress` re-blessed. The star classes
(`PLAN-star-classes.md`) share the bump.

## Target

**About 200 billion stars**, the size of the Milky Way. 100 billion was the floor asked
for; 200 costs nothing more (the count is an integral of a density function, nothing is
stored) and it lets the disc have real proportions: a 50,000 light year radius, a thin
disc a few hundred light years thick, a bulge, arms. Vesperis today has about 8 million
stars in a disc 1,500 light years across.

The 64-bit sector coordinates allow it many times over: one sector is one light year,
and the grid reaches 9e18 sectors in each direction.

## What stays the same

* **One star per sector at most**, decided by a hash against the density at that point.
* **Local density near the player.** The galaxy grows, the neighbourhood does not: the
  disc is tuned so that in the arms a sector in ten holds a star (nearest star 2 to 3
  light years) and in the outer disc a sector in forty (nearest 3 to 4), which is what
  the Vimana flights, the remote-target cycling and the star map (10 light years) are
  built for. Today's density near the player is about one in four; the new disc is a
  little sparser mid-way out and a little denser in the arms, close to the original's
  feel.
* The star map, targeting, the guide, saves: nothing in them assumes a size.

## The density function

`galaxyDensity(sx, sy, sz)` in `galaxy/starfield.cpp` keeps its shape with new scales
(numbers to be tuned at implementation; these give ~260 billion, so the target is a
touch smaller):

| Term | Today | Proposed |
| --- | --- | --- |
| disc scale length | 230 ly | ~8,000 ly |
| disc thickness `h` | 10 + 45 e^(-r/130) | ~250 + 1,200 e^(-r/4,000) |
| bulge radius | 70 ly | ~2,500 ly |
| arm period | r/45 | r/1,500 (two arms winding ~4 times over the disc) |
| cap per sector | 0.97 | 0.97 (the core is nearly solid stars, as now) |

Regions (`galaxyRegion`: core, bulge, arm, disk, halo, cluster, nebula) scale with the
same factors, so the class weights per region keep working. Globular clusters: about 150
of them, 30 to 100 light years across, in the halo (today they are knots every few
hundred sectors; the spacing scales up, the size does not).

## What has to change with it

* **The galaxy background** (`galaxyLook`, `nebulaGlow`): the ray integration takes 28
  steps over a fixed distance; the step and range scale so the Milky Way band is still
  drawn from the density (a longer range, coarser steps, or a two-scale integration:
  fine near, coarse far).
* **Region names on the console and in the log** (`REGION_NAMES`): unchanged, but the
  thresholds scale.
* **The home star and the start position.** The expedition starts at a fixed sector; it
  moves to a comparable place in the new disc (an arm, mid-radius, a few thousand light
  years from the core, so the core is a visible glow and a lifetime's journey).
* **The scene finders and pinned test sites** (`spot`, `scene`, the flow's
  "Skeatoltdos" check): re-found after the bump; `regress bless`.
* **Distances shown as light years** already go to hundreds of thousands without
  formatting trouble; the star map's ring labels (2, 5, 10 ly) stay.
* **Coordinates as text** (the guide's `SECTOR X Y Z` targeting, keys in `guide.txt`)
  are 64-bit already.

## Feel

The player cannot cross this galaxy; that is the point. What changes in play: the core
becomes a distant glow rather than a place you reach in an evening; the halo, the
clusters and the outer disc become regions with a character, days apart; a signal from
a civilisation (`IDEAS-ruins-shards-signals.md`) can come from far enough away that
the age of the signal means something.

## Order of work

1. New constants, the start sector, the background integration. `survey` prints the
   density profile and the count estimate.
2. Regions and clusters; class weights checked with `survey`.
3. Scene finders and pinned sites re-found; `regress bless`; the flow passes.
4. A play session: the neighbourhood must feel like today's.
