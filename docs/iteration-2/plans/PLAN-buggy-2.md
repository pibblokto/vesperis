# Plan: the buggy II (N4)

## Where it stands

`drawBuggy` draws a low box (chassis) on four octagonal wheels, a roll bar and an antenna light, tracks and dust puffs; from the seat the HUD draws a dark cowl with a dial (`hud.cpp`, "M8-06 dashboard"); the chase camera (`V`) sits 4.5 m behind. Driving: 4 m/s^2 forward, 2.5 reverse, drag 0.02 v^2, rolling resistance by material, speed clamped to -5..18 m/s (65 km/h), steering to 30 degrees, four-wheel terrain contact with pitch and roll, airborne over crests, collisions with rocks, trees and ruins. It works; it does not look like anything.

## Design

**Model (N4-01).** Built from quads and line strips in the buggy's frame (heading, pitch, roll), scaled 1.0 = a 3.2 m long, 1.9 m wide vehicle:

* a tubular roll cage (six tubes as thin quads with a bright edge line), front hoop, rear hoop, side bars;
* two bucket seats, a steering column;
* body panels: a hood sloping to the front bumper, side pods, rear deck with the spare wheel, fenders over each wheel (curved strips);
* four knobby wheels (12-sided, tread as a dark/bright alternating rim band) on visible suspension arms (two lines per wheel to the chassis) that travel with the terrain contact (the existing four-corner sampling gives the travel), steering wheels turned by `steer`, spin by `wheelSpin`;
* headlights (two bright quads plus the existing ground cones), tail lights (red bank when braking), the antenna with a blinking light, a small dish;
* colours: the ship's hull grey on bank 3, a chrome bright bank for tubes and rims, dark rubber, amber lights; dust settles on it after long drives (a darkening factor).

The same model serves the parked view, the chase view and the seat view (the parts in front of the camera). Unfolding (2 s) animates the cage rising and the wheels dropping.

**Driving (N4-02).** Top speed 32 m/s forward (115 km/h), 8 m/s reverse; a torque curve: 5 m/s^2 up to 15 m/s, tapering to 0.6 m/s^2 at the top; traction by material (ice 0.35, snow 0.6, sand 0.8, grass 0.9, rock 1.0) and gravity (below 4 m/s^2 the wheels spin and jumps grow); handbrake skids with lateral slide; the slope stall at 30 degrees stays; collisions cost 60% of speed and push back; camera shake grows with speed and roughness; a top-speed wind roar.

**Cabin (N4-03).** From the seat: the front hoop and the windshield edge frame the top of the view, the hood with both fenders fills the lower third, the front wheels visible at the sides when steering, a steering wheel (a ring and spokes) that turns with `steer`, gloved hands on it, a dashboard with dials: speed (needle), heading (a compass tape), odometer, temperature, lights on/off; the whole cabin pitches and rolls with the chassis and vibrates with the terrain. The HUD readouts stay for the environment; the cowl goes.

**Effects (N4-04).** Spray per wheel by material (dust, sand, snow, mud in wetlands), tracks matched to the wheels' width and tread, engine pitch with three gear steps, suspension thumps on landings, skid noise, headlight cones lighting terrain, rocks and trees at night.

## Tests

`flow`: unfold, get in (seat frame), drive 2 km at over 100 km/h across hills (a frame at speed, one airborne, one at night with the lights), chase frame, park and reload; `bench`: the driving frame at 2x under 5 ms; `audio` stage "buggy".
