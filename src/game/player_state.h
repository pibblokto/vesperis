// The ship's player (game/player.cpp): its list and the item playing
#pragma once
#include "shard_world.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

struct PlayerItem {
    std::string world; int which = 0, local = 0;   // the guide's world key, the people, its record
    bool music = false, heard = false;             // a piece; caught by the radar (and not held as a recording)
    int medium = -1, year = 0, form = -1;          // what it was made on; its year; a piece's form
    double seconds = 0;                            // a piece's length (a recording's once built)
    std::string people;                            // "THE KHOSA"
};
struct ShipPlayer {
    std::vector<PlayerItem> items; int sel = 0, top = 0;
    int playing = -1;                              // the item playing (of `items`), -1 none
    bool paused = false, held = false, waiting = false;   // paused by the explorer; held while another has the synth; its world's records still being written
    ShardWorld world;                              // the playing item's world: the synth's piece, tradition, speech and voice point into it
    double t = 0, secs = 0, syncT = -1;            // the game's clock into it (synced to the synth's while it runs), its length
    std::string title, people;                     // the HUD's ("A LULLABY", "THE KHOSA": the people left out where the line has no room)
    int finished = 0;                              // items played to their end since the game began (the harness)
    int guideVersion = -1;                         // the list's (rebuilt when the guide's shards change)
    std::map<std::string, std::pair<int, double>> pieceInfo;   // a piece's form and length by its key, worked out once
};
