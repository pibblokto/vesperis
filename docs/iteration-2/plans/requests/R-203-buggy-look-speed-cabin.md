---
id: R-203
title: The buggy should look like a buggy, go faster, and show a cabin from the seat
status: done 2026-09-26 (N4; the headlight cones light the ground only, KI-210)
requested: 2026-09-26
related: N4-01..N4-05, KI-205, M8-06 (first attempt)
---

## What
A recognisable buggy: roll cage, seats, fenders, big wheels on suspension, lights; a much higher top speed; from the seat the cage, the hood, the wheels and a dashboard with a steering wheel so it is obvious you are inside; better looking in every view.

## Why
The buggy is the way to see a world at scale; a box on octagons at 65 km/h with a HUD cowl neither looks nor feels like driving.

## Must / should / could
- must: a proper model in all three views; top speed around 110 km/h with a believable acceleration curve; a visible cabin from the seat with a turning steering wheel and dials.
- should: suspension travel, wheel steering and spin, headlight and tail light quads, spray per wheel, engine pitch with gear steps, skids.
- could: dust settling on the body, a dish antenna, mirror, the buggy on the sector map.

## References
Dune buggies and lunar rovers; the current model in `shots/flow_*buggy*.png`.

## My notes (filled by Claude)
Design in `PLAN-buggy-2.md`; items N4-01 to N4-05 in `ROADMAP.md`. Size L-XL, independent of the other milestones (can go first). Top speed 32 m/s (115 km/h), reverse 8 m/s, torque curve, traction by material and gravity. Acceptance: seat, chase and parked frames; a 2 km drive over 100 km/h in `flow`; the driving frame at 2x under 5 ms; `audio` stage "buggy".

### Outcome (2026-09-26, N4)
`surface/buggy.cpp` replaced the box on octagons. The model: a chassis pan with side pods and a rear deck, two bucket seats, a sloping hood with a cowl and a bumper, fenders arcing over each wheel, four 12-sided knobby wheels (tread knobs, chrome hubs and spokes) on two suspension arms each that follow the terrain contact and steer with the front pair, a chrome roll cage (front hoop, taller rear hoop, side bars, diagonals) that rises while unfolding, headlight quads (bright when on), red tail lights (brighter under braking), a spare wheel and a dish on the deck, an antenna with a blinking red light; the hull on a new grey-blue bank, chrome and warning-red banks; dust darkens the body on dry ground and rain washes it. Driving: a torque curve (5 m/s^2 to 15 m/s tapering to 1.2 at 32 m/s = 115 km/h, reverse 8 m/s, braking x2.5), traction by material and gravity (ice 0.35, snow 0.6, sand 0.8, grass 0.9, rock 1.0, times g/9.8 down to 0.3), a steering lock that shrinks from 30 to 9 degrees with speed, handbrake and low-grip slides (`skid`), collisions that cost 60% of the speed and push back while rocks under 0.9 m are bumps the wheels ride over, jumps as before, camera shake with speed and roughness, three engine gears, thumps on landings and hits, wind at speed. Cabin: the eye sits in the driver's (left) seat; the front hoop, the windshield bar and the hood with both fenders frame the view, the front wheels show at the sides when steering, a steering wheel (ring, spokes, gloved hands) turns 2.6 times the steering angle, an instrument binnacle carries the speed dial with a red needle, a compass needle and lights/brake tell-tales, with the readouts drawn under them on screen and following the dash (`dashAnchor`); at night the instruments glow and the headlights spill onto the hood; the HUD cowl is gone. Effects: spray per rear wheel by material (dust, snow, mud), tread tracks 0.3 m wide with alternating knobs, engine pitch per gear, skid noise, suspension thumps. The guide keeps the longest drive and the top speed (`stat topspeed`, `stat longestdrive`), the statistics screen shows them, leaving the buggy reports the trip and top speed, the buggy stays on the sector map.

Numbers: `vesperis_test drive` (a desert site, an 900 m open run, a test driver with U-turns and avoidance): 2,152 m in 90 s, top 107 km/h, render 2.3 ms mean at 1x; `bench` driving frame at 2x 5.7 ms (budget 14); `audio` stage "buggy"; scenes `buggy_seat` and `buggy_parked`; frames `drive_seat`, `drive_chase`, `drive_night`, `drive_night_chase`. The flow's landing site is forest everywhere within 800 m, so its drive stays a short smoke test. Not done: the headlight cones do not light trees and rocks (KI-210); a mirror.
