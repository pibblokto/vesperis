// A people's world as the decoder, the radar and the player hold it (game/shards_screen.cpp's `buildShardWorld`): its
// records, language, voice, music and charts
#pragma once
#include "peoples/charts.h"
#include "peoples/history.h"
#include "peoples/music.h"
#include "peoples/shards.h"
#include "galaxy/system.h"
#include "peoples/tongue.h"
#include "peoples/voice.h"
#include <memory>
#include <string>
#include <vector>

struct ShardWorld { std::string key; bool valid = false; StarSystem sys; int body = -1; Lore lore; Tongue tongue; Language lang; std::vector<Shard> shards;
                    // C-13: which of the world's peoples this is, how many the world had, the guide index of this people's first shard (its
                    // records count from zero here; T-04: `lore.records` of them)
                    int which = 0, peoples = 1, base = 0;
                    Tradition tradition; std::vector<int> pieceForm; std::vector<double> pieceSeconds; Piece piece;    // its music, the forms of its pieces, the piece on the screen
                    Voice voice; Speech speech;                                                                      // its voice and the shard on the screen spoken
                    Voice readVoice;                                                                                 // the ship reading a record with no voice of its own (`voiceRead`)
                    StarChart chart;                                                                                 // the chart on the screen
                    bool pending = false;                                                                            // its records are being written on a thread (the radar does not wait)
                    std::shared_ptr<const History> history; std::shared_ptr<const ShardTexts> texts; };              // the decoder's: its history and its records' texts (what each told)
