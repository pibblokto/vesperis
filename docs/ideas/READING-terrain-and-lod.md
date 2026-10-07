# Reading list: terrain meshing, LOD and noise

**STATUS: TO INVESTIGATE LATER. Nothing here is decided or scheduled.** Links the user handed
over on 2026-10-04 while the direction of the surface rendering (the parked V series of
`MILESTONES.md`, `IDEAS-surface-picture.md`; or GPU rendering through raylib) is still open. Read
them before that discussion; each may feed it, none is a commitment. The one-line summaries are
from a first look, not a study.

| Link | What it is | Why it may matter here |
| --- | --- | --- |
| [Using quadtrees for level of detail in voxel generation](https://medium.com/@danieljackson97123/using-quadtrees-for-level-of-detail-in-voxel-generation-517f98f3bf50) | A quadtree over the terrain, subdivided toward the camera, each leaf a chunk at its own resolution; the seams between neighbours of different depth and how they are stitched | The alternative to our nested rings (4 m to 2048 m cells with morph bands): a quadtree picks the detail per chunk by distance and screen error rather than by fixed radii, which is what V-01 (continuous LOD) would want |
| [Perlin noise: the algorithm behind the universes of No Man's Sky](https://medium.com/@pratyaksh.notebook/perlin-noise-the-evolving-algorithm-behind-the-diverse-universes-of-no-mans-sky-f2cc8ddacd52) | An overview of gradient noise, octaves, domain warping and how a planet-scale game layers them | Mostly what the planet function already does (`galaxy/planetmap.*`: the relief spectrum, erosion and hybrid weights, Worley and value noise); worth a skim for the uber-noise ideas (altitude erosion, ridged and billowy variants, sharpness by slope) we do not have |
| [Dual contouring tutorial](https://www.boristhebrave.com/2018/04/15/dual-contouring-tutorial/) | Boris the Brave's walk through dual contouring: a signed distance field sampled on a grid, one vertex per cell placed by the field's gradient (QEF), sharp features kept | Relevant only if the ground ever becomes a volume (caves, overhangs, arches: the K series), which the heightfield rings cannot draw. Dual contouring is the method that keeps a cliff edge sharp |
| [Smooth voxel mapping: surface nets and texturing](https://bonsairobo.medium.com/smooth-voxel-mapping-a-technical-deep-dive-on-real-time-surface-nets-and-texturing-ef06d0f8ca14) | Naive surface nets (the cheaper cousin of dual contouring) in real time, chunked, with LOD and triplanar texturing | Same case as above, with the practical side: chunk sizes, LOD transitions between chunks, how to texture a mesh without UVs (triplanar: our material tiles projected along three axes would be the analogue) |
| [Cube sphere (Catlike Coding)](https://catlikecoding.com/unity/tutorials/procedural-meshes/cube-sphere/) | A sphere built from a cube's six faces, each a grid pushed onto the sphere with a distortion-reducing mapping, and the mesh code for it | The classic way to mesh a whole planet with no pole singularity, for a GPU renderer that draws the globe and the ground as one mesh. Our ground is a local tangent-plane frame (`site.localFrame`) with curvature in `toView`; a cube sphere would be the base of a quadtree LOD over the whole globe |

## Questions to answer when reading

1. Does a quadtree over a cube sphere (links 1 and 5) give continuous LOD cheaper than our rings
   on the CPU, or only on a GPU? The rings exist because the CPU draws every triangle.
2. Could the planet function stay as it is (a height and a material per point, deterministic) under
   any of these schemes? It must: the saves, the maps and the globe read it.
3. Volumes (links 3 and 4) only with the caves (K): what would a heightfield plus local volumes at
   the cave mouths cost, against a volume everywhere?
