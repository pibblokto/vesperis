// U-02: the states' table (game.h's `StateRow`), a row a state in GameState's order: its name, traits and controller's context
// (E-06), what the frame runs for it (its update, its picture, an overlay's `covers` and its `away`) and, U-04, its controller's
// page (`padMenu`: its commands, its name, its own buttons). A new state appends its row; the compiler holds the order, and each
// row to its update and its picture.
#include "game.h"

namespace {
using S = GameState;
using G = Game;
constexpr int to(GameState s) { return (int)s; }   // a way back to a screen of its own
}

constexpr Game::StateRow Game::STATE_ROWS[STATE_COUNT] = {
    // the state, its name, traits and controller's context; its update, its picture, an overlay's covers, an away, its controller's page;
    // U-05: its enter (`openScreen`'s) and its way back (`closeScreen`'s); a screen with lines of its own in the save, its `save`,
    // `load` and `reset` after them (none yet)
    {S::TITLE, "TITLE", ST_STOWS | ST_TAKES_SYNTH | ST_BASE, PCX_TITLE,
     &G::updateTitle, &G::renderTitle, nullptr, nullptr, &G::titlePad, nullptr, BACK_NONE},
    {S::SPACE, "SPACE", ST_SIMULATES | ST_IN_PLAY | ST_MOUSE | ST_PHOTO | ST_BASE | ST_CLOCK, PCX_SHIP,
     &G::updateSpace, &G::renderSpaceScreen, nullptr, nullptr, &G::spacePad, nullptr, BACK_NONE},
    {S::LANDING_MAP, "LANDING_MAP", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_CLOCK, PCX_SCREEN,
     &G::updateLandingMap, &G::renderLandingScreen, nullptr, nullptr, &G::landingMapPad, &G::landingMapEnter, to(S::SPACE)},
    {S::DESCENT, "DESCENT", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_MOUSE | ST_GROUND, PCX_SCREEN,
     &G::updateDescentAscent, &G::renderCapsuleScreen, nullptr, nullptr, nullptr, nullptr, BACK_NONE},
    {S::SURFACE, "SURFACE", ST_SIMULATES | ST_STOWS | ST_IN_PLAY | ST_MOUSE | ST_PHOTO | ST_GROUND | ST_BASE | ST_CLOCK, PCX_FOOT,
     &G::updateSurface, &G::renderSurfaceScreen, nullptr, nullptr, &G::surfacePad, nullptr, BACK_NONE},
    {S::ASCENT, "ASCENT", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_MOUSE | ST_GROUND, PCX_SCREEN,
     &G::updateDescentAscent, &G::renderCapsuleScreen, nullptr, nullptr, nullptr, nullptr, BACK_NONE},
    {S::HELP, "HELP", ST_OVERLAY, PCX_SCREEN,
     &G::updateHelp, &G::renderHelp, nullptr, nullptr, &G::helpPad, &G::helpEnter, BACK_SCENE},
    {S::MENU, "MENU", ST_OVERLAY, PCX_SCREEN,
     &G::updateMenu, &G::renderMenu, nullptr, nullptr, nullptr, &G::menuEnter, BACK_SCENE},
    {S::SYSTEM_LIST, "SYSTEM_LIST", ST_OVERLAY, PCX_SCREEN,
     &G::updateSystemList, &G::renderSystemList, nullptr, nullptr, &G::systemListPad, nullptr, to(S::SPACE)},
    {S::DATA, "DATA", ST_OVERLAY, PCX_SCREEN,
     &G::updateDataSheet, &G::renderDataSheet, nullptr, nullptr, &G::dataSheetPad, nullptr, BACK_SCENE},
    {S::SETTINGS, "SETTINGS", ST_OVERLAY, PCX_SCREEN,
     &G::updateSettingsScreen, &G::renderSettings, nullptr, nullptr, nullptr, &G::settingsEnter, to(S::MENU)},
    {S::SLOTS, "SLOTS", ST_OVERLAY, PCX_SCREEN,
     &G::updateSlots, &G::renderSlots, nullptr, nullptr, nullptr, &G::slotsEnter, to(S::MENU)},
    {S::GUIDE, "GUIDE", ST_OVERLAY, PCX_SCREEN,
     &G::updateGuideMenu, &G::renderGuideMenu, nullptr, nullptr, nullptr, &G::guideEnter, BACK_SCENE},
    {S::TEXT_ENTRY, "TEXT_ENTRY", ST_OVERLAY | ST_TYPING, PCX_SCREEN,
     &G::updateTextEntry, &G::renderTextEntry, &G::textEntryCovers, nullptr, &G::textEntryPad, nullptr, BACK_OPENER},
    {S::STAR_MAP, "STAR_MAP", ST_OVERLAY | ST_MOUSE | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateStarMap, &G::renderStarMap, &G::starMapCovers, nullptr, &G::starMapPad, &G::starMapEnter, to(S::SPACE)},
    {S::LOG, "LOG", ST_OVERLAY, PCX_SCREEN,
     &G::updateLog, &G::renderLog, nullptr, nullptr, nullptr, &G::logEnter, BACK_SCENE},
    {S::STATS, "STATS", ST_OVERLAY, PCX_SCREEN,
     &G::updateStats, &G::renderStats, nullptr, nullptr, nullptr, nullptr, BACK_SCENE},
    {S::GALLERY, "GALLERY", ST_OVERLAY, PCX_SCREEN,
     &G::updateGallery, &G::renderGallery, nullptr, nullptr, nullptr, &G::galleryEnter, BACK_SCENE},
    {S::SHIPSCREEN, "SHIPSCREEN", ST_SIMULATES | ST_OVERLAY | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateShipScreen, &G::renderShipScreen, nullptr, nullptr, &G::shipScreenPad, &G::shipScreenEnter, to(S::SPACE)},
    {S::CONSOLE, "CONSOLE", ST_SIMULATES | ST_OVERLAY | ST_TYPING | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateConsole, &G::renderConsole, nullptr, nullptr, &G::consolePad, &G::consoleEnter, to(S::SPACE)},
    {S::SECTOR_MAP, "SECTOR_MAP", ST_OVERLAY | ST_STOWS | ST_MOUSE, PCX_SCREEN,
     &G::updateSectorMap, &G::renderSectorMap, nullptr, nullptr, &G::sectorMapPad, &G::sectorMapEnter, to(S::SURFACE)},
    {S::KEYS, "KEYS", ST_OVERLAY, PCX_SCREEN,
     &G::updateKeysScreen, &G::renderKeysScreen, nullptr, nullptr, nullptr, &G::keysEnter, BACK_OPENER},
    {S::SHARDS, "SHARDS", ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateShards, &G::renderShards, nullptr, &G::shardsAway, &G::shardsPad, &G::shardsEnter, to(S::SPACE)},
    {S::ALMANAC, "ALMANAC", 0, PCX_SCREEN,
     &G::updateAlmanac, &G::renderAlmanac, nullptr, nullptr, &G::almanacPad, &G::almanacEnter, BACK_OPENER},
    {S::PROBE, "PROBE", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_PHOTO, PCX_PROBE,
     &G::updateProbeScreen, &G::renderProbe, nullptr, nullptr, &G::probePad, nullptr, to(S::SPACE)},
    {S::RECORDING, "RECORDING", ST_TAKES_SYNTH | ST_PHOTO, PCX_RECORDING,
     &G::updateRecording, &G::renderRecording, nullptr, nullptr, &G::recordingPad, nullptr, to(S::GALLERY)},
    {S::INSTRUMENTS, "INSTRUMENTS", ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH | ST_SHIP_MOVES, PCX_INSTRUMENT,
     &G::updateInstruments, &G::renderInstruments, nullptr, &G::instrumentsAway, &G::instrumentsPad, &G::instrumentsEnter, to(S::SPACE)},
    {S::DRONES, "DRONES", ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH | ST_SHIP_MOVES, PCX_SCREEN,
     &G::updateDroneBay, &G::renderDroneBay, nullptr, nullptr, &G::droneBayPad, &G::droneBayEnter, to(S::SPACE)},
    {S::AUTO, "AUTO", ST_OVERLAY, PCX_SCREEN,
     &G::updateAutoScreen, &G::renderAutoScreen, nullptr, nullptr, nullptr, &G::autoEnter, BACK_OPENER},
};

namespace {
// each row in its state's place, named, with its update and its picture; only an overlay covers the frame; an overlay has a way back,
// a scene's state or another screen's, never itself
constexpr bool stateRowsWhole() {
    for (int i = 0; i < STATE_COUNT; i++) {
        const Game::StateRow& r = Game::STATE_ROWS[i];
        if ((int)r.state != i || !r.name || !r.update || !r.render || (r.covers && !(r.traits & ST_OVERLAY))) return false;
        if (r.back < BACK_OPENER || r.back >= STATE_COUNT || r.back == i || ((r.traits & ST_OVERLAY) && r.back == BACK_NONE)) return false;
    }
    return true;
}
static_assert(stateRowsWhole(), "a row a state in GameState's order, each with its update, its picture and a way back");
}

// U-05: the one way in: where the screen was opened from (its way back when its row's is its opener), its row's enter (its fields,
// its devices stowed; `state` still the screen it opens from), then the state. A screen that closes as it opens the next one
// closes first (`closeScreen`): its own way back is then the next one's opener. B-431: a screen opened from a base has it as its
// scene (`returnState`: drawn under it, a save's place), so no scene the explorer left (the title, the ground) stays under it
void Game::openScreen(GameState s) {
    const StateRow& r = STATE_ROWS[(int)s];
    if (stateHas(state, ST_BASE)) returnState = state;
    openedFrom[(int)s] = state;
    if (r.enter) (this->*r.enter)();
    state = s;
}

// U-05: the one way back, the row's: its scene, its opener, or a screen of its own (Esc, and an exit that goes where Esc goes)
void Game::closeScreen() {
    const int b = STATE_ROWS[(int)state].back;
    if (b == BACK_SCENE) state = returnState;
    else if (b == BACK_OPENER) state = openedFrom[(int)state];
    else if (b >= 0) state = (GameState)b;
}

// B-432: where the game is, which a save writes and a photo names: the state in play, else the scene the screen is drawn over;
// at the title, or over it, the game it resumes (where the title was opened from: a load's place, or space)
GameState Game::placeNow() const {
    const GameState s = stateHas(state, ST_IN_PLAY) ? state : returnState;
    return state == GameState::TITLE || s == GameState::TITLE ? openedFrom[(int)GameState::TITLE] : s;
}

// read here, out of line, so this file holds the table's one definition whichever way a compiler reads its `constexpr`
bool stateHas(GameState s, unsigned trait) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT && (Game::STATE_ROWS[i].traits & trait); }
int statePad(GameState s) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT ? Game::STATE_ROWS[i].pad : (int)PCX_SCREEN; }
const char* stateName(GameState s) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT ? Game::STATE_ROWS[i].name : "?"; }
