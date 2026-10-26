// The ship's readings of a system: the analyzer's list of its bodies (SYSTEM_LIST) and a body's data sheet (DATA) with its
// forecast
#include "game.h"
#include "peoples/plan.h"
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

void Game::renderSystemList() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    std::string head = fmt("SYSTEM ANALYZER - %s (%s)", trunc(upper(starNameOf(sys.star)), 16).c_str(), sys.classString().c_str());
    drawText(canvas, 8, 6, head.c_str(), HUD_AMBER);
    std::vector<std::string> desc = wrapText(upper(STAR_CLASSES[sys.star.cls].description), 50);   // S-02: the long rows wrap onto a second line (they were cut at 50 characters)
    for (size_t k = 0; k < desc.size() && k < 2; k++) drawText(canvas, 8, 15 + 8 * (int)k, desc[k].c_str(), HUD_DIM);
    int listY = desc.size() > 1 ? 32 : 28;
    int n = (int)sys.bodies.size();
    int rows = desc.size() > 1 ? 16 : 17;
    if (listSel < listScroll) listScroll = listSel;
    if (listSel >= listScroll + rows) listScroll = listSel - rows + 1;   // the belts count as rows after the bodies
    for (int r = 0; r < rows; r++) {
        int i = listScroll + r;
        if (i >= n) break;
        const Body& b = sys.bodies[i];
        double d = length(sys.bodyPos(i, t) - ship.pos);
        std::string line = fmt("%s%-18.18s %-10s %10s%s%s", b.parent >= 0 ? "  " : "", upper(bodyNameOf(i)).c_str(), shortType(b.type),
                               distanceString(d).c_str(), b.rings ? " R" : "", b.moonCount ? fmt(" %dM", b.moonCount).c_str() : "");
        if (b.type == PT_COMET) line += fmt(" %.0f KM/S", length(sys.bodyVel(i, t)));   // comets are the only bodies that visibly hurry
        uint32_t col = i == listSel ? HUD_WHITE : (i == ship.localTarget ? HUD_AMBER : (KINDS[b.type].landable ? HUD_GREEN : HUD_DIM));
        drawText(canvas, 14, listY + r * 9, line.c_str(), col);
        if (i == listSel) drawText(canvas, 6, listY + r * 9, ">", HUD_AMBER);
    }
    // belts after the bodies; O3: selectable like the bodies (Enter approaches, L targets)
    for (size_t k = 0; k < sys.belts.size(); k++) {
        int r = n - listScroll + (int)k;
        if (r < 0 || r >= rows) continue;
        const Belt& bl = sys.belts[k];
        uint32_t col = n + (int)k == listSel ? HUD_WHITE : (ship.targetBelt == (int)k ? HUD_AMBER : HUD_DIM);
        drawText(canvas, 14, listY + r * 9, fmt("%-18.18s %-10s %s - %s", upper(beltNameOf((int)k)).c_str(), "BELT", distanceString(bl.innerKm).c_str(), distanceString(bl.outerKm).c_str()).c_str(), col);
        if (n + (int)k == listSel) drawText(canvas, 6, listY + r * 9, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, UH - 10, "UP/DOWN SELECT  ENTER APPROACH  L TARGET  ESC CLOSE", HUD_DIM);
}

// W-03: the forecast at the feet
void Game::updateForecast() {
    if (!surf.valid || !sys.valid || surf.site.body < 0 || surf.site.body >= (int)sys.bodies.size()) return;
    const Body& b = sys.bodies[surf.site.body];
    if (!worldHasFronts(b)) { forecast.clear(); forecastBody = surf.site.body; forecastT = t; return; }
    if (forecastBody == surf.site.body && (std::fabs(t - forecastT) < 30 || realTime - forecastReal < 0.5)) return;   // thirty seconds of game time, and twice a second of ours under a warp
    frontsAhead(b, surf.site.unitAt(surf.player.x, surf.player.z), t, 5 * 86400.0, forecast);
    forecastBody = surf.site.body; forecastT = t; forecastReal = realTime;
}

// "RAIN FROM THE SW IN 2 H 05 MIN", "IN A FRONT, CLEARING IN 1 H 05 MIN", "NO FRONT WITHIN 5 DAYS"; "" on a world without fronts
std::string Game::forecastLine() const {
    if (!surf.valid || !sys.valid || surf.site.body < 0 || surf.site.body >= (int)sys.bodies.size()) return "";
    const Body& b = sys.bodies[surf.site.body];
    if (!worldHasFronts(b)) return "";
    for (const FrontForecast& f : forecast) {
        if (f.tClear <= t) continue;
        if (f.tArrive <= t) return fmt("IN A %sFRONT, CLEARING IN %s", f.dust ? "DUST " : "", countdownString(f.tClear - t).c_str());
        const char* what = f.dust ? "DUST" : (!kindHas(b.type, KT_FRONT_RAIN) ? "A FRONT" : (surf.env.temperatureC < -1 ? "SNOW" : "RAIN"));
        return fmt("%s FROM THE %s IN %s", what, frontCompass(f.fromBearing), countdownString(f.tArrive - t).c_str());
    }
    return "NO FRONT WITHIN 5 DAYS";
}

void Game::renderDataSheet() {
    if (dataPage >= dataSheetPages) {   // X-04: a probe's profile (Q-01: after the sheet's pages)
        const std::vector<int> pr = dataProbes();
        if (dataPage - dataSheetPages < (int)pr.size()) { renderProbeProfile(pr[dataPage - dataSheetPages]); return; }
        dataPage = 0;
    }
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    int bi = state == GameState::DATA && surf.valid && returnState == GameState::SURFACE ? surf.site.body : ship.localTarget;
    int y = 10;
    auto line = [&](const std::string& s, uint32_t c) { drawText(canvas, 10, y, s.c_str(), c); y += 9; };
    drawText(canvas, UW - 10 - textWidth(epocFullString().c_str()), y, epocFullString().c_str(), HUD_DIM);
    if (bi < 0 && ship.targetBelt >= 0 && ship.targetBelt < (int)sys.belts.size() && !(surf.valid && returnState == GameState::SURFACE)) {
        // the belt's sheet
        const Belt& bl = sys.belts[ship.targetBelt];
        line(upper(beltNameOf(ship.targetBelt)), HUD_AMBER);
        line("ASTEROID BELT", HUD_GREEN);
        for (const std::string& w : wrapText("A BAND OF ROCKS AND DUST LEFT IN THE GAP BETWEEN TWO ORBITS. MOST OF ITS STONES ARE UNDER A KILOMETRE; A FEW ARE MOUNTAINS TWENTY KILOMETRES ACROSS. THE INNER ONES OUTRUN THE OUTER, SO THE FIELD SHEARS SLOWLY.", 50)) line(w, HUD_DIM);
        y += 3;
        double w = bl.outerKm - bl.innerKm, mid = 0.5 * (bl.innerKm + bl.outerKm);
        line(fmt("INNER EDGE  %s FROM THE STAR", distanceString(bl.innerKm).c_str()), HUD_GREEN);
        line(fmt("OUTER EDGE  %s   WIDTH %s", distanceString(bl.outerKm).c_str(), distanceString(w).c_str()), HUD_GREEN);
        line(fmt("PERIOD      %.1f H AT THE INNER EDGE, %.1f H AT THE OUTER", bl.period / 3600, TAU / bl.rateAt(bl.outerKm) / 3600), HUD_GREEN);
        double cells = (w / BELT_CELL_KM) * (TAU * mid / BELT_CELL_KM) * std::max(1.0, 2 * 0.015 * mid / BELT_CELL_KM);
        double rocks = cells * 1.1;
        line(fmt("ROCKS       ABOUT %s, THE SPARKLE BAND IS THE SUNLIT DUST", rocks > 1e6 ? fmt("%.1f MILLION", rocks / 1e6).c_str() : fmt("%.0f THOUSAND", rocks / 1e3).c_str()), HUD_GREEN);
        line("LANDABLE    NO - THE CAPSULE NEEDS A WORLD UNDER IT", HUD_RED);
        BeltRock rk;
        if (beltRockNow(rk)) line(fmt("%s     A ROCK %.1f KM ACROSS, %s OFF, TUMBLING SLOWLY", ship.parkedBelt >= 0 ? "PARKED " : "TARGET ", 2 * rk.radiusKm, distanceString(length(rk.pos - ship.pos)).c_str()), HUD_CYAN);
        y += 3;
        line(fmt("STAR        %s, %s", trunc(upper(starNameOf(sys.star)), 18).c_str(), sys.classString().c_str()), HUD_CYAN);
        line(fmt("            L=%.2f  R=%.0f KM", sys.star.luminosity, sys.star.radiusKm), HUD_CYAN);
        for (const std::string& ww : wrapText(upper(describeStar(sys.star)), 50)) line(ww, HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "T THE ALMANAC  ANY OTHER KEY CLOSES", HUD_DIM);
        return;
    }
    if (bi < 0 || bi >= (int)sys.bodies.size()) { drawTextCentered(canvas, UW / 2, 90, "NO LOCAL TARGET", HUD_DIM); return; }
    const Body& b = sys.bodies[bi];
    // the body's sheet in sections (the world, its numbers, the place, the star) laid over pages (RIGHT): a section kept whole on a
    // page where it fits a page, a sheet that fits one drawn as it always was (the sentence, six traits and the facts ran past the footer
    // on half the worlds; a fifth already had)
    struct SheetLine { std::string s; uint32_t c; int sec; bool gap; };
    std::vector<SheetLine> sheet; int sec = 0; bool gap = false;
    auto put = [&](const std::string& s, uint32_t c) { sheet.push_back({s, c, sec, gap}); gap = false; };
    auto section = [&](bool withGap) { sec++; gap = withGap; };
    put(fmt("%s", upper(bodyNameOf(bi)).c_str()), HUD_AMBER);
    put(fmt("%s%s", KINDS[b.type].name, b.parent >= 0 ? fmt(" - MOON OF %s", trunc(upper(bodyNameOf(b.parent)), 20).c_str()).c_str() : ""), HUD_GREEN);
    for (const std::string& w : wrapText(upper(KINDS[b.type].description), 50)) put(w, HUD_DIM);
    for (const std::string& w : wrapText(upper(describeBody(b, spaceR.genFor(b))), 50)) put(w, HUD_CYAN);
    {   // the traits (Q-01: six slots, on two lines), and the facts read from the orbit and the spin on one
        const std::string tl = traitList(spaceR.genFor(b)), fl = factList(sys, bi);
        bool first = true;
        if (!tl.empty()) for (const std::string& w : wrapText(tl, 38)) { put(fmt("%s%s", first ? "TRAITS      " : "            ", w.c_str()), HUD_AMBER); first = false; }
        first = true;
        if (!fl.empty()) for (const std::string& w : wrapText(fl, 38)) { put(fmt("%s%s", first ? "FACTS       " : "            ", w.c_str()), HUD_AMBER); first = false; }
    }
    if (surf.valid && returnState == GameState::SURFACE && bi == surf.site.body) {
        int bio = surf.site.lod0.at((int)std::floor(surf.player.x / 16), (int)std::floor(surf.player.z / 16)).biome;
        std::string sn = sys.sunless() ? std::string() : seasonName(surf.site.season, surf.site.lat0, b.axialTilt);   // no sun, no season
        if (bio || !sn.empty()) put(fmt("HERE        %s%s%s", bio ? BIOME_NAMES[bio] : "", (bio && !sn.empty()) ? ", " : "", sn.c_str()), HUD_AMBER);
    }
    section(true);
    put(fmt("RADIUS      %.0f KM   MASS %.3f EARTHS   TILT %.1f%c", b.radiusKm, b.massEarths, b.axialTilt / DEG, CH_DEGREE), HUD_GREEN);
    if (sys.sunless()) {   // a drifter's worlds have no equilibrium (their warmth is their own) and the drifter no orbit
        put(fmt("GRAVITY     %.2f G (%.1f M/S2)   OWN HEAT %.0f K (%+.0f C)", b.gravity / 9.8, b.gravity, b.tempK, b.tempK - 273.15), HUD_GREEN);
        if (b.parent < 0) put("ORBIT       NONE - ADRIFT BETWEEN THE STARS", HUD_GREEN);
        else put(fmt("ORBIT       %s FROM ITS PLANET%s", distanceString(b.orbitRadiusKm).c_str(), b.ecc > 0.05 ? fmt(", ECC %.2f", b.ecc).c_str() : ""), HUD_GREEN);
        if (b.parent < 0) put(fmt("PERIOD      NONE   ROTATION %.2f H%s", std::fabs(b.rotPeriod) / 3600, b.rotPeriod < 0 ? " (RETROGRADE)" : ""), HUD_GREEN);
        else put(fmt("PERIOD      %.1f H   ROTATION %.2f H%s", std::fabs(b.orbitPeriod) / 3600, std::fabs(b.rotPeriod) / 3600, b.rotPeriod < 0 ? " (RETROGRADE)" : ""), HUD_GREEN);
    } else {
    put(fmt("GRAVITY     %.2f G (%.1f M/S2)   EQUILIBRIUM %.0f K (%+.0f C)", b.gravity / 9.8, b.gravity, b.tempK, b.tempK - 273.15), HUD_GREEN);
    put(fmt("ORBIT       %s FROM %s%s", distanceString(b.orbitRadiusKm).c_str(), b.parent >= 0 ? (sys.bodies[b.parent].type == PT_COMPANION ? "ITS SUN" : "ITS PLANET") : "THE STAR", b.ecc > 0.05 ? fmt(", ECC %.2f", b.ecc).c_str() : ""), HUD_GREEN);
    put(fmt("PERIOD      %.1f H   ROTATION %.2f H%s", std::fabs(b.orbitPeriod) / 3600, std::fabs(b.rotPeriod) / 3600, b.rotPeriod < 0 ? " (RETROGRADE)" : (b.locked ? " (LOCKED)" : "")), HUD_GREEN);
    }
    if (b.type == PT_COMET) {   // where it is now on its plunge
        put(fmt("NOW         %s FROM THE STAR", distanceString(length(sys.bodyPos(bi, t) - sys.star.pos)).c_str()), HUD_CYAN);
        put(fmt("            %s", cometMotionString(bi).c_str()), HUD_CYAN);
    }
    put(fmt("ATMOSPHERE  %s   MOONS %d%s   LANDABLE %s", KINDS[b.type].atmosphere ? (airFrozen(b) ? "FROZEN" : "YES") : "NONE", b.moonCount, b.rings ? "   RINGS" : "", KINDS[b.type].landable ? "YES" : "NO"), KINDS[b.type].landable ? HUD_GREEN : HUD_RED);
    {   // the magnetic field and what it means for the nights (the star's class counts too)
        int mc = magneticClass(magneticField(b));
        double quiet = auroraPotentialAt(sys, b, 0), peak = auroraPotentialAt(sys, b, 1);
        const char* hint = quiet > 0.15 ? "   AURORAE ON MOST NIGHTS" : (peak > 0.3 ? "   AURORAE IN STORMS" : (peak > 0.12 ? "   A FAINT AURORA IN A GREAT STORM" : ""));
        put(fmt("MAGNETIC    %s%s", MAGNETIC_CLASS_NAMES[mc], hint), HUD_GREEN);
    }
    section(true);
    // seasons readout, M5-09 what is up in the sky (surface only)
    if (surf.valid && returnState == GameState::SURFACE && bi == surf.site.body) {
        double lat = surf.site.lat0;
        double sinDec = sys.seasonOf(bi, t) * std::sin(b.axialTilt) / std::max(1e-9, std::sin(std::max(b.axialTilt, 1e-6)));
        sinDec = clampd(sys.seasonOf(bi, t), -1, 1);   // dot(spin axis, to the star) = sin of the declination
        double dec = std::asin(sinDec);
        double cosH0 = -std::tan(lat) * std::tan(dec);
        std::string dayLen = cosH0 <= -1 ? "POLAR DAY" : (cosH0 >= 1 ? "POLAR NIGHT" : fmt("DAY %.1f H OF %.1f", std::fabs(b.rotPeriod) / 3600.0 * std::acos(cosH0) / PI, std::fabs(b.rotPeriod) / 3600.0));
        // next equinox and solstice by scanning the orbit
        double P = std::fabs(b.orbitPeriod), toEq = -1, toSol = -1, prev = sinDec, prevD = 0;
        for (int k = 1; k <= 144 && (toEq < 0 || toSol < 0); k++) {
            double v = sys.seasonOf(bi, t + P * k / 144.0);
            if (toEq < 0 && ((prev < 0) != (v < 0))) toEq = P * k / 144.0;
            double d = v - prev;
            if (toSol < 0 && k > 1 && ((prevD > 0) != (d > 0))) toSol = P * (k - 1) / 144.0;
            prev = v; prevD = d;
        }
        auto hours = [&](double secs) { return secs < 0 ? std::string("-") : (secs > 48 * 3600 ? fmt("%.0f D", secs / 86400) : fmt("%.0f H", secs / 3600)); };
        if (sys.sunless()) put("SEASON      NONE - NO SUN, NO DAY", HUD_AMBER);
        else if (b.axialTilt > 0.5 * DEG) put(fmt("SEASON      DECL %+.1f%c  %s  EQUINOX IN %s  SOLSTICE IN %s", dec / DEG, CH_DEGREE, dayLen.c_str(), hours(toEq).c_str(), hours(toSol).c_str()), HUD_AMBER);
        else line(fmt("SEASON      NONE (NO TILT)  %s", dayLen.c_str()), HUD_AMBER);
        bool forecastShown = false;
        { std::string fc = forecastLine(); if (!fc.empty()) { put(fmt("FORECAST    %s", fc.c_str()), HUD_AMBER); forecastShown = true; } }
        std::string sky;
        int nsky = 0;
        Mat3 L = surf.site.localFrame(t);
        Vec3 sitePos = surf.site.worldPos(t, surf.player.x, surf.player.z, 0.002);
        std::vector<std::pair<double, std::string>> up;
        for (int j = 0; j < (int)sys.bodies.size(); j++) {
            if (j == bi) continue;
            Vec3 rel = sys.bodyPos(j, t) - sitePos;
            double d = length(rel);
            Vec3 dl = L * (rel / d);
            if (dl.y < 0) continue;
            double ang = std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1)) / DEG;
            up.push_back({ang, fmt("%s %.0f%c%s", trunc(upper(bodyNameOf(j)), 18).c_str(), std::asin(dl.y) / DEG, CH_DEGREE, ang > 0.25 ? fmt(" (%.1f%c WIDE)", 2 * ang, CH_DEGREE).c_str() : "")});
        }
        std::sort(up.begin(), up.end(), [](auto& a, auto& b2) { return a.first > b2.first; });
        for (auto& u : up) { if (nsky++ >= 2) break; sky += (sky.empty() ? "" : ", ") + u.second; }
        if (up.size() > 2) sky += fmt(" +%d", (int)up.size() - 2);
        put(fmt("IN THE SKY  %s", sky.empty() ? "NOTHING ABOVE THE HORIZON" : sky.c_str()), HUD_AMBER);
        if (surf.env.hasSun2) put(fmt("SECOND SUN  %s AT %+.0f%c", upper(bodyNameOf(sys.companion)).c_str(), surf.env.sun2.altitude / DEG, CH_DEGREE), HUD_AMBER);
        if (!surf.bestiary.species.empty()) {   // the world's bestiary, sighted species marked
            std::string life;
            for (int i = 0; i < (int)surf.bestiary.species.size(); i++) {
                const Species& sp = surf.bestiary.species[i];
                life += (life.empty() ? "" : ", ") + upper(sp.name) + (surf.seenSpecies.count(i) ? "*" : "") + " (" + shortPlan(sp.plan) + ")";
            }
            std::vector<std::string> ws = wrapText("LIFE        " + life, 52);
            for (size_t w = 0; w < ws.size() && w < (forecastShown ? 2u : 3u); w++) put(ws[w], HUD_AMBER);   // the forecast takes the third life line's row (the sheet was already past the frame on a world of many species)
        }
    }
    section(false);
    if (settings.clockMode == 1) put("CLOCK       REAL TIME (UTC)", HUD_DIM);
    if (sys.sunless()) put("STAR        NONE", HUD_CYAN);   // the drifter's place is no star (its name is the world's)
    else {
        put(fmt("STAR        %s, %s", trunc(upper(starNameOf(sys.star)), 18).c_str(), sys.classString().c_str()), HUD_CYAN);
        put(fmt("            L=%.2f  R=%.0f KM", sys.star.luminosity, sys.star.radiusKm), HUD_CYAN);
    }
    for (const std::string& w : wrapText(upper(describeStar(sys.star)), 50)) put(w, HUD_DIM);
    {   // the pages: a line's top at UH - 18 at most (its glyphs clear of the footer at UH - 10); a page after the first under the world's name
        // again, its number left of the clock
        const int top = y, last = UH - 18;
        std::vector<int> ys(sheet.size()), pg(sheet.size());
        int page = 0, yy = top;
        for (size_t i = 0; i < sheet.size(); i++) {
            const int first = page == 0 ? top : top + 9;   // the page's first line under its title
            const bool starts = i > 0 && sheet[i].sec != sheet[i - 1].sec;
            if (starts && yy > first) {
                size_t j = i; while (j < sheet.size() && sheet[j].sec == sheet[i].sec) j++;
                const int h = 9 * (int)(j - i) - 9;   // the section's last line's top, from its first's
                if (yy + (sheet[i].gap ? 3 : 0) + h > last && top + 9 + h <= last) { page++; yy = top + 9; }
                else if (sheet[i].gap) yy += 3;
            }
            if (yy > last) { page++; yy = top + 9; }
            ys[i] = yy; pg[i] = page; yy += 9;
        }
        dataSheetPages = page + 1;
        const int show = std::min(dataPage, page);
        dataSheetBottom = top;
        for (size_t i = 0; i < sheet.size(); i++) {
            dataSheetBottom = std::max(dataSheetBottom, ys[i] + 9);
            if (pg[i] == show) drawText(canvas, 10, ys[i], sheet[i].s.c_str(), sheet[i].c);
        }
        if (show > 0) drawText(canvas, 10, top, sheet[0].s.c_str(), sheet[0].c);
        if (dataSheetPages > 1) { const std::string pn = fmt("%d/%d", show + 1, dataSheetPages); drawText(canvas, UW - 10 - textWidth(epocFullString().c_str()) - 12 - textWidth(pn.c_str()), top, pn.c_str(), HUD_DIM); }
    }
    const char* foot = dataPage == 0 ? (dataSheetPages > 1 ? "T THE ALMANAC  RIGHT MORE  ANY OTHER KEY CLOSES" : (dataProbes().empty() ? "T THE ALMANAC  ANY OTHER KEY CLOSES" : "T THE ALMANAC  RIGHT PROFILES  ANY OTHER KEY CLOSES"))
                                     : "RIGHT MORE  LEFT BACK  ANY OTHER KEY CLOSES";   // a giant's probes; Q-01: the sheet's pages
    drawTextCentered(canvas, UW / 2, UH - 10, foot, HUD_DIM);
}
