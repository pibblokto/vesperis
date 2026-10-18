# B-428: moons crowding their worlds from the ship

status: resolved 2026-10-18 (1.3.2, the sixth of the patch's items)
severity: S3
area: the moons' orbits (`galaxy/system.cpp`, `StarSystem::generate`; generation 15)

## Report

The user, after B-427: "quite often planets/moons are super close to each other when you are on startdrifter. Don't get me wrong, I
love when I am on the surface and you seen a big beatiful moon or planet but when you are on stardrifter and they are all within your
visibility it's strange - there is no sense of it being an actual system with distance and stuff, makes it feel more pocket".

## Cause

* **The moons, by design.** A system's worlds lie millions of kilometres apart, points of light to one another. Its moons did not:
  every world's first moon orbited at 4.5-6 of its radii (R-001 and M5-09 drew them in for discs in the sky, B-314 set the range), a
  solid world's moon may be half its size (M5-09), and a twin planet stood 5-9 radii off. The ship parks at 3.5 radii, inside the first
  moon's orbit, so a moon swung past it one to three radii away, wider than from the ground. Parked near home (`parkings`, below): a
  rocky world's moon 17 and 19 degrees wide 15,000-20,000 km off, a twin 11 degrees, a giant 19-26 degrees wide behind its inner moons.
* **How often** (`survey 6000`: 595 systems round home, every world parked at twice): another world wider than 10 degrees in the ship's
  sky at 44% of the parkings. At a solid world's moon 96% (its world 19-26 degrees across), at a twin 80%, at a solid world 21% (its
  moon), at a giant's moon 34% (the giant). Another world wider than 6 degrees in the view of the world: 17%.
* **The ground's discs are the same distance.** A moon 12 degrees wide from the ground is about as wide from a ship parked by its world.
  From the ship both worlds are seen at once, and the gap between them, a few of their own diameters, reads as a pocket. So spreading
  the moons trades the ground's biggest discs. How far was the user's call: one question, three directions with their numbers (the solid
  worlds' moons and the twins; the giants' too; a gentle spread to 5-20 radii). The user chose the first.

## Fix

* **A solid world's first moon at 5-40 radii,** even in the logarithm: `5 x 8^u`, on the draw `u` that gave `4.5 + 1.5 u`, so the
  rest of each system holds. Most land near 14 radii, one in eleven inside 6, and each stays outside the rings by a third of their
  radius. Its later moons keep their spacing from it. Real moons spread so: Charon at 16.5 of Pluto's radii, the Moon at 60 of Earth's.
* **A twin at 12-20 radii** (5-9), as Charon is from Pluto.
* **A giant's moons stay** (its first at 4.5-6 radii): the giant over its inner moons, as Jupiter over Io (20 degrees), the biggest
  discs in the game's skies.
* **What follows from the geometry.**
  * **Periods:** they follow by Kepler's law. A locked moon's day is its month (seven moons in ten are locked), so a solid world's first
    moon has a day of 7.7 hours at the median (1.9 before), over 30 hours for one in ten and 49 at most. A twin pair shares a day of
    6-12 hours (2-3.5 before).
  * **Eclipses:** a moon far out is often smaller than its sun, so its eclipses are transits more often than total.
  * **Companions and comets:** where a world's reach grew, a companion's floor (B-425) can move, and so can the comets keyed to the last
    body (KI-416).
  * **Version:** `GEN_VERSION` 15; saves from 14 and before get the regeneration notice once.
* **The almanac's check** (W-01) read every eclipse's listed time as the peak of the share covered. That fails for a transit (a disc wholly
  inside the sun's) or a total phase, where the share is the discs' ratio, flat but for the body's distance. The almanac lists the middle,
  where the centres pass nearest, and the check now reads that. One of its test worlds, the first planet of Seapheeseeri, now sees its moon
  (1.9 degrees) cross a sun 20 degrees wide. The share there rises from 0.8818% to 0.8883% over four minutes as the site turns toward the
  moon, so a peak half a minute either side was never there.

## Verification (2026-10-18)

* `vesperis_test survey` has a new line, the parkings (same 595 systems, before and after):

  | Parked at | World wider than 10 degrees in the sky | Wider than 6 degrees in the view |
  | --- | --- | --- |
  | Every world | 44% → 21% | 17% → 8% |
  | Solid worlds | 21% → 4% | 14% → 2% |
  | Their moons | 96% → 39% | 30% → 15% |
  | Twins | 80% → 5% | 29% → 8% |
  | Giants' moons | 34% → 34% | 11% → 11% |

  The ground, at the survey's 11,298 sites (R-001's measure, by radius): a disc wider than 2 degrees at 31% of the sites (33), wider
  than 6 degrees at 17% (27), wider than 20 degrees at 3% (10), mostly a giant from its inner moons.
* `vesperis_test parkings` (new): twelve parkings near home, each world parked at and then the eyes on the biggest other disc, the same
  places before and after:
  * the venusian world's moon was 17 degrees wide 20,000 km off, now 2.4 degrees 145,000 km off (21 radii);
  * the rocky world's moon was 18.7 degrees, now 4.6 degrees (13 radii);
  * the twins were 10.9 and 10.1 degrees, now 6.9 and 6.2 degrees;
  * the giants' moons are as they were.
* `vesperis_test unit` has a new check (B-428) over 2,500 worlds of the survey's sectors:
  * solid worlds' first moons beyond 10 radii in 333 of 513 systems, none outside 5-40;
  * giants' first moons all within 4.5-6, twins all within 12-20;
  * from the parkings, another world wider than 10 degrees in the sky at 21% (solid worlds 5%, twins 10%);
  * from the ground, a disc wider than 2 degrees at 33% of the sites and wider than 20 degrees at 2.4%.

  Run on the generator with only these lines reverted, it fails on every spacing count: no first moon beyond 10 radii (97 under 5),
  34 of 34 twins outside 12-20, and 43% of the parkings (solid worlds 21%, twins 78%).
* `regress`: 8 of the 24 hashes moved and were blessed again; the old generator linked with the same tree changes none.
  * The five systems.
  * `cratered_noon`: 71 pixels by one step, the moonlight.
  * `molten_night`: the far ground under the horizon a touch dimmer, its moon farther.
  * The cabin at 1x: a dark moon's edge in the right window gone.

  The frames are the same drawn by one thread.
* `unit` 340 checks, 0 failures (one new); `flow` passes.
