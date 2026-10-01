# Coordinates, frames and units

## World axes and camera

* World is right-handed with **y up**. Galactic positions are in kilometres (doubles).
* Heading (yaw): 0 = +z ("north"), increasing clockwise seen from above, so yaw 90 deg = +x ("east"). Pitch positive looks up.
* `cameraBasis(yaw, pitch)` returns rows (right, up, forward): forward = (sin yaw cos pitch, sin pitch, cos yaw cos pitch), right = (cos yaw, 0, -sin yaw), up = forward x right. `Mat3 * v` maps world to view.

## Sectors

`SECTOR_KM = 1.5e10`. Sector index of a coordinate: `floor(km / SECTOR_KM)`. The HUD shows one sector as one "LY" (`distanceString` prints km, Mkm or LY). `StarNeighborhood` scans a cube of radius 10 sectors around the ship whenever the ship's sector changes.

The galactic centre is not the origin (G-01, 2026-10-01): sector coordinates are heliocentric like the real galactic frame, and the centre sits at sector (12000, 0, -5500), 13,050 ly from the home sectors at yaw 115 degrees; `galaxyTerms` measures the radius and the arm phase from there, so every scan and pin of the harness and the new-game search kept their coordinates when the galaxy grew to 200 billion stars (`04-galaxy-and-systems.md`).

**Surface sectors (O2, R-301)** are the 1 x 1 degree cells of a world's latitude/longitude grid, named `LON:LAT` with the longitude index 0..359 counted from 180 W and the latitude index 0..179 from 90 S (`sectorIndexOf`, `sectorName` in `game/ui.h`; on a 6000 km world a sector is 105 km square at the equator). The landing map, its zoom (`Z`), the surface HUD and the sector map all use these names; crossing into another sector on the ground is announced and logged. The old landing-map readout (360 x 120 cells) is gone.

## Body frames

Each `Body` has a spin axis `a` (unit), and two perpendicular reference vectors `ref0`, `ref1`. At game time `t`:

```
theta(t) = rotPhase0 + 2*pi*t / rotPeriod        (rotPeriod < 0 = retrograde)
E0 = cos(theta) ref0 + sin(theta) ref1
E1 = -sin(theta) ref0 + cos(theta) ref1
bodyFrame(i, t) = rows(E0, E1, a)                  world -> body-frame vector
```

Body-frame unit vector of a surface point: `(cos lat cos lon, cos lat sin lon, sin lat)`; inverse: `lat = asin(z)`, `lon = atan2(y, x)`. `surfacePointWorld(i, t, lat, lon, altKm)` gives the world position.

Spin axis construction: `a = normalize(y cos tilt + (cos az, 0, sin az) sin tilt)`; `ref0 = normalize(cross(any, a))`, `ref1 = cross(a, ref0)`.

## Orbits

Circular. `u = (cos node, 0, sin node)`, `v0 = cross(y, u)`, `v = v0 cos incl + y sin incl`; position relative to the parent `= r (u cos phi + v sin phi)` with `phi = orbitPhase0 + 2*pi*t / orbitPeriod`. Parent is the star for planets, the planet for moons. `bodyPos(i, t)` is absolute (recursion through the parent).

Periods: planets `T = ORBIT_K * r^1.5 / sqrt(massFactor)` with `ORBIT_K = 8e-6` (r in km, T in s): a 1.4e7 km orbit takes about 116 h; moons `T = MOON_K * r^1.5 / sqrt(pmass)` with `MOON_K = 1.26e-3` and `pmass = (Rp/6371)^2.5` (x0.6 for gas giants): 5 planet radii around an Earth-sized planet is about 2 h.

## Landing site frame (`SurfaceSite`)

* Site basis in the **body frame**: `up0` = unit vector of (lat0, lon0); `east0 = normalize(cross((0,0,1), up0))`; `north0 = cross(up0, east0)`.
* Local coordinates: `x` east, `z` north, metres from the site origin; heights `y` in metres above the reference level (sea level 0 on felisian worlds).
* `unitAt(x, z)`: azimuthal equidistant mapping, `ang = sqrt(x^2+z^2) / R`, `unit = up0 cos(ang) + dir sin(ang)` with `dir = (east0 x + north0 z)/d`. Exact geodesics, so no distortion at the poles; consistent everywhere.
* `localFrame(t)` = rows (east, up, north) expressed in world coordinates, i.e. it maps world vectors to local `(x=E, y=U, z=N)`. This matches the camera convention, so `camLocal = cameraBasis(yaw, pitch)` composes directly: `cam = camLocal * localFrame(t)` maps world to view.
* Everything on the surface uses the site basis, not the player's position; the site re-anchors at 50 km on a planet and at 0.35 radii on a small body (O4), so the azimuthal-equidistant scale error stays under 2%.
* Curvature (O1): `SurfaceView::toView` lowers every point by `d^2 / 2R` with its horizontal distance `d` from the camera (planets), so the ground, the water, rocks, trees, the capsule and the buggy all curve away from the camera; on a small body (`SurfaceSite::smallBody`, R under 300 km) it places the point on the exact sphere seen from the camera's own vertical: `horizontal = (R + h) sin(d/R)`, `vertical = (R + h) cos(d/R) - (R + eye)`, so the far side curves under and positions foreshorten with the angle. The camera's local vertical is therefore always the true one; only the (x, z) coordinates carry the projection's distortion, which the re-anchoring bounds.

## Sun and time of day (`SurfaceSite::sun(t)`)

```
dW   = normalize(star.pos - bodyPos)          world direction to the star
dL   = localFrame(t) * dW                     local (E, U, N)
altitude = asin(dL.y)     azimuth = atan2(dL.x, dL.z)   (0 = north, clockwise)
sunBody  = bodyFrame(t) * dW;  subLon = atan2(sunBody.y, sunBody.x)
hourAngle = wrapAngle(lon0 - subLon);  dayFraction = wrap2pi(hourAngle + pi) / 2pi   (0.5 = noon)
lightFactor = max(0.62, clamp((L * (AU/d)^2)^0.25, 0.35, 1.15))
angularRadius = asin(starRadius / d)
```

`AU_GAME_KM = 2.2e7` is the reference orbit for temperature: `Teq = 278 K * L^0.25 * sqrt(AU / r)`.

## Units summary

| Quantity | Unit | Notes |
| --- | --- | --- |
| Galactic positions, radii, orbits | km | doubles; 1 m resolution at galactic scale |
| Surface positions, heights | m | site-relative |
| Time `t` | s | game time; starts at 3.6e6; warp x1..x10000 |
| EPOC display | - | `6011 + t/1e9 : (t/1e6 mod 1000).(t/1e3 mod 1000)` |
| Temperature | K internally, C on the HUD | |
| Wind | knots | |
| Pressure | atm | |

## Ship and transitions

Parking distance: 3.5 body radii (or `ringOuter + 1.3` radii for ringed bodies). Fine approach lasts `4 + 2 log10(1 + d/1e5)` real seconds with smoothstep easing and a smooth turn toward the body. Vimana flight lasts `7 + 2 sqrt(LY)` real seconds and ends at `max(25 R, 0.55 * first orbit)` from the star on the side you came from. Descent and ascent take 7 real seconds from/to 1800 m altitude. Orbit mode rotates the parking direction around the body's spin axis with a period of `600 + 200 (R/6000)` game seconds.
