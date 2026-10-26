// U-02: the states' table (game.h's `StateRow`), a row a state in GameState's order: its name, traits and controller's context
// (E-06), and what the frame runs for it: its update, its picture, an overlay's `covers` and its `away`. A new state appends its
// row; the compiler holds the order, and each row to its update and its picture.
#include "game.h"

namespace {
using S = GameState;
using G = Game;
}

constexpr Game::StateRow Game::STATE_ROWS[STATE_COUNT] = {
    // the state, its name, traits and controller's context; its update, its picture, then an overlay's covers and an away
    {S::TITLE, "TITLE", ST_STOWS | ST_TAKES_SYNTH, PCX_TITLE,
     &G::updateTitle, &G::renderTitle, nullptr, nullptr},
    {S::SPACE, "SPACE", ST_SIMULATES | ST_IN_PLAY | ST_MOUSE | ST_PHOTO, PCX_SHIP,
     &G::updateSpace, &G::renderSpaceScreen, nullptr, nullptr},
    {S::LANDING_MAP, "LANDING_MAP", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY, PCX_SCREEN,
     &G::updateLandingMap, &G::renderLandingScreen, nullptr, nullptr},
    {S::DESCENT, "DESCENT", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_MOUSE | ST_GROUND, PCX_SCREEN,
     &G::updateDescentAscent, &G::renderCapsuleScreen, nullptr, nullptr},
    {S::SURFACE, "SURFACE", ST_SIMULATES | ST_STOWS | ST_IN_PLAY | ST_MOUSE | ST_PHOTO | ST_GROUND, PCX_FOOT,
     &G::updateSurface, &G::renderSurfaceScreen, nullptr, nullptr},
    {S::ASCENT, "ASCENT", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_MOUSE | ST_GROUND, PCX_SCREEN,
     &G::updateDescentAscent, &G::renderCapsuleScreen, nullptr, nullptr},
    {S::HELP, "HELP", ST_OVERLAY, PCX_SCREEN,
     &G::updateHelp, &G::renderHelp, nullptr, nullptr},
    {S::MENU, "MENU", ST_OVERLAY, PCX_SCREEN,
     &G::updateMenu, &G::renderMenu, nullptr, nullptr},
    {S::SYSTEM_LIST, "SYSTEM_LIST", ST_OVERLAY, PCX_SCREEN,
     &G::updateSystemList, &G::renderSystemList, nullptr, nullptr},
    {S::DATA, "DATA", ST_OVERLAY, PCX_SCREEN,
     &G::updateDataSheet, &G::renderDataSheet, nullptr, nullptr},
    {S::SETTINGS, "SETTINGS", ST_OVERLAY, PCX_SCREEN,
     &G::updateSettingsScreen, &G::renderSettings, nullptr, nullptr},
    {S::SLOTS, "SLOTS", ST_OVERLAY, PCX_SCREEN,
     &G::updateSlots, &G::renderSlots, nullptr, nullptr},
    {S::GUIDE, "GUIDE", ST_OVERLAY, PCX_SCREEN,
     &G::updateGuideMenu, &G::renderGuideMenu, nullptr, nullptr},
    {S::TEXT_ENTRY, "TEXT_ENTRY", ST_OVERLAY | ST_TYPING, PCX_SCREEN,
     &G::updateTextEntry, &G::renderTextEntry, &G::textEntryCovers, nullptr},
    {S::STAR_MAP, "STAR_MAP", ST_OVERLAY | ST_MOUSE | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateStarMap, &G::renderStarMap, &G::starMapCovers, nullptr},
    {S::LOG, "LOG", ST_OVERLAY, PCX_SCREEN,
     &G::updateLog, &G::renderLog, nullptr, nullptr},
    {S::STATS, "STATS", ST_OVERLAY, PCX_SCREEN,
     &G::updateStats, &G::renderStats, nullptr, nullptr},
    {S::GALLERY, "GALLERY", ST_OVERLAY, PCX_SCREEN,
     &G::updateGallery, &G::renderGallery, nullptr, nullptr},
    {S::SHIPSCREEN, "SHIPSCREEN", ST_SIMULATES | ST_OVERLAY | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateShipScreen, &G::renderShipScreen, nullptr, nullptr},
    {S::CONSOLE, "CONSOLE", ST_SIMULATES | ST_OVERLAY | ST_TYPING | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateConsole, &G::renderConsole, nullptr, nullptr},
    {S::SECTOR_MAP, "SECTOR_MAP", ST_OVERLAY | ST_STOWS | ST_MOUSE, PCX_SCREEN,
     &G::updateSectorMap, &G::renderSectorMap, nullptr, nullptr},
    {S::KEYS, "KEYS", ST_OVERLAY, PCX_SCREEN,
     &G::updateKeysScreen, &G::renderKeysScreen, nullptr, nullptr},
    {S::SHARDS, "SHARDS", ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateShards, &G::renderShards, nullptr, &G::shardsAway},
    {S::ALMANAC, "ALMANAC", 0, PCX_SCREEN,
     &G::updateAlmanac, &G::renderAlmanac, nullptr, nullptr},
    {S::PROBE, "PROBE", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_PHOTO, PCX_PROBE,
     &G::updateProbeScreen, &G::renderProbe, nullptr, nullptr},
    {S::RECORDING, "RECORDING", ST_TAKES_SYNTH | ST_PHOTO, PCX_RECORDING,
     &G::updateRecording, &G::renderRecording, nullptr, nullptr},
    {S::INSTRUMENTS, "INSTRUMENTS", ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH | ST_SHIP_MOVES, PCX_INSTRUMENT,
     &G::updateInstruments, &G::renderInstruments, nullptr, &G::instrumentsAway},
    {S::DRONES, "DRONES", ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateDroneBay, &G::renderDroneBay, nullptr, nullptr},
    {S::AUTO, "AUTO", ST_OVERLAY, PCX_SCREEN,
     &G::updateAutoScreen, &G::renderAutoScreen, nullptr, nullptr},
};

namespace {
// each row in its state's place, named, with its update and its picture; only an overlay covers the frame
constexpr bool stateRowsWhole() {
    for (int i = 0; i < STATE_COUNT; i++) {
        const Game::StateRow& r = Game::STATE_ROWS[i];
        if ((int)r.state != i || !r.name || !r.update || !r.render || (r.covers && !(r.traits & ST_OVERLAY))) return false;
    }
    return true;
}
static_assert(stateRowsWhole(), "a row a state in GameState's order, each with its update and its picture");
}

// read here, out of line, so this file holds the table's one definition whichever way a compiler reads its `constexpr`
bool stateHas(GameState s, unsigned trait) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT && (Game::STATE_ROWS[i].traits & trait); }
int statePad(GameState s) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT ? Game::STATE_ROWS[i].pad : (int)PCX_SCREEN; }
const char* stateName(GameState s) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT ? Game::STATE_ROWS[i].name : "?"; }
