// The frame's two scenes, drawn under every overlay: space (the stars, the system, the cabin or the radar's feed, the
// title's tour) and the ground (the surface, the vehicles' nose camera, the vision modes)
#include "game.h"
#include "peoples/plan.h"
#include <array>
#include "ui.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cmath>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

void Game::renderSpace() {
    spaceR.proj = telescopeProj();   // W-06: the telescope's magnification (the setting's field alone when it is stowed)
    int a, b;
    choosePaletteBodies(a, b);
    if (ship.mode == ShipState::VIMANA) { a = -1; b = -1; }
    spaceR.setupPalette(fb, sys.valid ? &sys : nullptr, a, b, fade);
    SpaceContext c;
    c.sys = sys.valid ? &sys : nullptr;
    c.stars = &nb.stars;
    c.t = t;
    c.shipPos = ship.pos;
    bool titleCam = state == GameState::TITLE || (state == GameState::HELP && returnState == GameState::TITLE);
    c.cam = titleCam ? cameraBasis(titleYaw, 0.05) : viewBasis();
    if (titleCam && attractActive()) {
        // attract mode: a slow tour of the system, fourteen seconds per body, circling each at four radii
        int nb = (int)sys.bodies.size();
        int bi = (int)(attractT / 14.0) % nb;
        double u = std::fmod(attractT, 14.0) / 14.0;
        const Body& bd = sys.bodies[bi];
        Vec3 bp = sys.bodyPos(bi, t);
        double ang = u * 1.6 + bi;
        double R = bd.radiusKm * (bd.rings ? std::max(4.0, bd.ringOuter + 1.5) : 4.0);
        Vec3 toStar = normalize(sys.star.pos - bp);
        Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
        Vec3 dir = normalize(toStar * (0.5 + 0.4 * std::cos(ang)) + side * std::sin(ang) + Vec3(0, 0.25, 0));
        c.shipPos = bp + dir * R;
        Vec3 fwd = normalize(bp - c.shipPos);
        c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
        a = bi; b = -1;
        spaceR.setupPalette(fb, &sys, a, b, fade);
    }
    c.vimana = ship.mode == ShipState::VIMANA;
    if (c.vimana) {
        double u = clampd(ship.flightT / ship.flightDur, 0, 1);
        c.vimanaSpeed = std::sin(u * PI);
        c.vimanaDirView = c.cam * normalize(ship.flightTo - ship.flightFrom);
        c.flightFrom = ship.flightFrom; c.flightTo = ship.flightTo; c.flightT = ship.flightT; c.flightDur = ship.flightDur;   // the band built ahead along the line
    }
    c.bankBodyA = a; c.bankBodyB = b;
    c.starIntensity = (ship.targeting || fieldAmp || radar.on || tele.on) ? 1.5 : 1.0;   // field amplification while aiming or by choice; C-07: the radar camera sees the stars amplified; W-06: the telescope gathers light
    c.detailBody = titleCam ? -1 : telescopePlateBody();   // the eyepiece's body gets the planet function at its footprint (the plate)
    c.detailBudget = (wantsScreenshot || telePlateFinish) ? 0 : TELE_PLATE_BUDGET;   // a photo reads the plate out whole
    telePlateFinish = false;
    spaceR.render(fb, c);
    if (settings.cabin && !titleCam && (state != GameState::LANDING_MAP || ax.on) && !radar.on && !tele.on) { cabinPalette(); drawDroneLights(); drawCabin(); }   // auto-explore's landing map is the cabin still   // M2; C-07: the radar camera is outside the cabin; W-06: the eye is at the eyepiece; C-18: the drones outside the windows
    if (radar.on && !titleCam) radarCameraFeed(fb);
    fb.mush(2);
    fb.toRGB(rgbBuf.data(), 1.0 + 1.3 * std::max(0.0, arrivalFlash), settings.dither);   // arrival flash
}

// G-07: the galaxy page in a flight draws no scene, so the band's keyframes are kept building here (a share a frame), or closing the
// page would build one whole at once
void Game::keepFlightBand() {
    if (ship.mode != ShipState::VIMANA) return;
    SpaceContext c;
    c.vimana = true; c.flightFrom = ship.flightFrom; c.flightTo = ship.flightTo; c.flightT = ship.flightT; c.flightDur = ship.flightDur;
    spaceR.flightBand(c);
}

void Game::renderSurfaceScene() {
    surf.floodlights = state == GameState::DESCENT || state == GameState::ASCENT;   // D-04: the capsule's, on its way down and up
    surf.render(fb, t, nb.stars, spaceR, fade);
    fb.mush(surf.valid && hasOpaqueDeck(surf.site.gen.type) ? 3 : 2);   // the thick air blurs everything
    bool feed = surf.inVehicle() && !surf.chaseCam && !photoMode && surf.cameraOverrideAlt < 0;   // driving is seen through the nose camera; R-403: flying too
    if (feed) surf.cameraFeed(fb, realTime);
    else if (visionMode != 0) applyVision();
    fb.toRGB(rgbBuf.data(), visionMode == 1 ? 1.25 : 1.0, settings.dither);
}

// M4-10: vision modes are palette remaps applied after the mush, before the RGB conversion.
void Game::applyVision() {
    uint8_t* pal = fb.pal;
    for (int b = 0; b < BANKS; b++)
        for (int i = 0; i < 64; i++) {
            uint8_t* c = pal + (b * 64 + i) * 3;
            double lum = (0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2]);
            switch (visionMode) {
                case 1: {   // radiation visor: brighter, greyer, grainy
                    double g = std::min(255.0, lum * 1.6 + 20);
                    c[0] = (uint8_t)(g * 0.95); c[1] = (uint8_t)g; c[2] = (uint8_t)(g * 0.9);
                    break;
                }
                case 2: {   // supervision: blue scale
                    c[0] = (uint8_t)(lum * 0.25); c[1] = (uint8_t)(lum * 0.55); c[2] = (uint8_t)std::min(255.0, lum * 1.2 + 25);
                    break;
                }
                case 3: {   // infrared: heat shows, the rest is dark; lava and creatures are hot
                    bool hot = (b == 2 && surf.lavaGlow) || b == 3;
                    double v = hot ? std::min(255.0, i * 4.5 + 60) : lum * 0.35;
                    c[0] = (uint8_t)std::min(255.0, v); c[1] = (uint8_t)(hot ? v * 0.6 : v * 0.5); c[2] = (uint8_t)(hot ? v * 0.15 : v * 0.7);
                    break;
                }
                case 4: {   // plant vision: vegetation magenta, everything else grey
                    bool plant = b == 3 && kindHas(surf.site.gen.type, KT_FLORA);
                    if (plant) { c[0] = (uint8_t)std::min(255.0, i * 4.0 + 40); c[1] = (uint8_t)(i * 0.8); c[2] = (uint8_t)std::min(255.0, i * 3.5 + 30); }
                    else { c[0] = c[1] = c[2] = (uint8_t)(lum * 0.6); }
                    break;
                }
            }
        }
    if (visionMode == 1) {
        // grain: a little noise on the intensities
        Rng r((uint64_t)(realTime * 60));
        for (size_t i = 0; i < fb.idx.size(); i += 3) fb.idx[i] = pixI(bankOf(fb.idx[i]), intenOf(fb.idx[i]) + (int)(r.sym(80)));   // +-2.5 shades
    }
}
