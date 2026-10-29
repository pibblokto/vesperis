// The records' grammar's files, GRAMMAR_FILES' rows (shards.cpp): three lines a file, its name (its path under grammar/),
// its text, `},`. Append-only: a new file's three lines go at the end and no file changes its place (the parse follows this
// order). The harness reads the same names from disk (`vesperis_test grammar`).
{"core/records.vg",
#include "core/records.vg"
},
{"core/world.vg",
#include "core/world.vg"
},
{"core/people.vg",
#include "core/people.vg"
},
{"registers/annal.vg",
#include "registers/annal.vg"
},
{"registers/annal_war_land.vg",
#include "registers/annal_war_land.vg"
},
{"registers/letter_events.vg",
#include "registers/letter_events.vg"
},
{"registers/letter_events_war.vg",
#include "registers/letter_events_war.vg"
},
{"registers/news.vg",
#include "registers/news.vg"
},
{"registers/song.vg",
#include "registers/song.vg"
},
{"registers/log.vg",
#include "registers/log.vg"
},
{"genres/letters.vg",
#include "genres/letters.vg"
},
{"ages/prehistoric/marks.vg",
#include "ages/prehistoric/marks.vg"
},
{"ages/prehistoric/herd_dead.vg",
#include "ages/prehistoric/herd_dead.vg"
},
{"ages/medieval/chronicle.vg",
#include "ages/medieval/chronicle.vg"
},
{"ages/medieval/will_epitaph.vg",
#include "ages/medieval/will_epitaph.vg"
},
{"ages/medieval/sermon.vg",
#include "ages/medieval/sermon.vg"
},
{"ages/industrial/newspaper.vg",
#include "ages/industrial/newspaper.vg"
},
{"ages/industrial/census_obituary.vg",
#include "ages/industrial/census_obituary.vg"
},
{"ages/modern/broadcast.vg",
#include "ages/modern/broadcast.vg"
},
{"ages/modern/propaganda_emergency.vg",
#include "ages/modern/propaganda_emergency.vg"
},
{"ages/futuristic/machine_log.vg",
#include "ages/futuristic/machine_log.vg"
},
{"ages/futuristic/lesson.vg",
#include "ages/futuristic/lesson.vg"
},
{"ages/futuristic/mission_charter.vg",
#include "ages/futuristic/mission_charter.vg"
},
{"ages/farfuture/untranslatable_mathematics.vg",
#include "ages/farfuture/untranslatable_mathematics.vg"
},
{"genres/ending_warning.vg",
#include "genres/ending_warning.vg"
},
{"genres/ending_whoever.vg",
#include "genres/ending_whoever.vg"
},
{"core/charts.vg",
#include "core/charts.vg"
},
{"core/lexicon.vg",
#include "core/lexicon.vg"
},
{"core/culture_goods.vg",
#include "core/culture_goods.vg"
},
{"core/culture_ways.vg",
#include "core/culture_ways.vg"
},
{"core/culture_speech.vg",
#include "core/culture_speech.vg"
},
{"registers/annal_works.vg",
#include "registers/annal_works.vg"
},
{"registers/annal_everyday.vg",
#include "registers/annal_everyday.vg"
},
{"registers/annal_collapse.vg",
#include "registers/annal_collapse.vg"
},
{"registers/annal_everyday2.vg",
#include "registers/annal_everyday2.vg"
},
{"genres/diary.vg",
#include "genres/diary.vg"
},
{"genres/ending_orders.vg",
#include "genres/ending_orders.vg"
},
{"genres/ending_last.vg",
#include "genres/ending_last.vg"
},
{"ages/industrial/columns.vg",
#include "ages/industrial/columns.vg"
},
{"ages/industrial/columns_adverts.vg",
#include "ages/industrial/columns_adverts.vg"
},
{"ages/industrial/ledger_report.vg",
#include "ages/industrial/ledger_report.vg"
},
{"ages/industrial/voice.vg",
#include "ages/industrial/voice.vg"
},
{"ages/industrial/telegram_rules.vg",
#include "ages/industrial/telegram_rules.vg"
},
{"registers/letter_events_land.vg",
#include "registers/letter_events_land.vg"
},
{"registers/letter_events_everyday.vg",
#include "registers/letter_events_everyday.vg"
},
{"registers/letter_events_everyday2.vg",
#include "registers/letter_events_everyday2.vg"
},
{"registers/letter_events_private.vg",
#include "registers/letter_events_private.vg"
},
{"genres/letters_life.vg",
#include "genres/letters_life.vg"
},
{"genres/letters_life2.vg",
#include "genres/letters_life2.vg"
},
{"genres/letters_life3.vg",
#include "genres/letters_life3.vg"
},
{"registers/log_everyday.vg",
#include "registers/log_everyday.vg"
},
{"ages/modern/station_log.vg",
#include "ages/modern/station_log.vg"
},
{"ages/modern/weather.vg",
#include "ages/modern/weather.vg"
},
{"ages/modern/messages.vg",
#include "ages/modern/messages.vg"
},
{"ages/modern/reports.vg",
#include "ages/modern/reports.vg"
},
{"ages/modern/minutes.vg",
#include "ages/modern/minutes.vg"
},
{"ages/modern/lyrics.vg",
#include "ages/modern/lyrics.vg"
},
{"ages/modern/manual.vg",
#include "ages/modern/manual.vg"
},
{"ages/modern/paper.vg",
#include "ages/modern/paper.vg"
},
{"registers/news_land.vg",
#include "registers/news_land.vg"
},
{"registers/news_collapse.vg",
#include "registers/news_collapse.vg"
},
{"registers/news_everyday.vg",
#include "registers/news_everyday.vg"
},
{"registers/news_everyday2.vg",
#include "registers/news_everyday2.vg"
},
{"registers/news_sickness.vg",
#include "registers/news_sickness.vg"
},
{"registers/news_rule.vg",
#include "registers/news_rule.vg"
},
{"ages/prehistoric/tally.vg",
#include "ages/prehistoric/tally.vg"
},
{"ages/prehistoric/hunt.vg",
#include "ages/prehistoric/hunt.vg"
},
{"ages/prehistoric/pre_song.vg",
#include "ages/prehistoric/pre_song.vg"
},
{"ages/prehistoric/pre_picture.vg",
#include "ages/prehistoric/pre_picture.vg"
},
{"ages/prehistoric/moon.vg",
#include "ages/prehistoric/moon.vg"
},
{"ages/prehistoric/pre_song_events.vg",
#include "ages/prehistoric/pre_song_events.vg"
},
{"ages/prehistoric/hunt_events.vg",
#include "ages/prehistoric/hunt_events.vg"
},
{"registers/song_land.vg",
#include "registers/song_land.vg"
},
{"registers/picture.vg",
#include "registers/picture.vg"
},
{"ages/farfuture/instructions.vg",
#include "ages/farfuture/instructions.vg"
},
{"ages/farfuture/reflection.vg",
#include "ages/farfuture/reflection.vg"
},
{"ages/farfuture/instruction_reasons.vg",
#include "ages/farfuture/instruction_reasons.vg"
},
{"ages/futuristic/archive_events.vg",
#include "ages/futuristic/archive_events.vg"
},
{"ages/futuristic/archive.vg",
#include "ages/futuristic/archive.vg"
},
{"ages/futuristic/habitat.vg",
#include "ages/futuristic/habitat.vg"
},
{"ages/futuristic/notes.vg",
#include "ages/futuristic/notes.vg"
},
{"ages/futuristic/vote_law.vg",
#include "ages/futuristic/vote_law.vg"
},
{"ages/futuristic/lesson_events.vg",
#include "ages/futuristic/lesson_events.vg"
},
{"ages/futuristic/medical_memorial.vg",
#include "ages/futuristic/medical_memorial.vg"
},
{"ages/medieval/prayer_events.vg",
#include "ages/medieval/prayer_events.vg"
},
{"ages/medieval/ballad.vg",
#include "ages/medieval/ballad.vg"
},
{"ages/medieval/chronicle_asides.vg",
#include "ages/medieval/chronicle_asides.vg"
},
{"ages/medieval/sermon_everyday2.vg",
#include "ages/medieval/sermon_everyday2.vg"
},
{"ages/medieval/decree.vg",
#include "ages/medieval/decree.vg"
},
{"ages/medieval/sermon_news.vg",
#include "ages/medieval/sermon_news.vg"
},
{"ages/medieval/sermon_everyday.vg",
#include "ages/medieval/sermon_everyday.vg"
},
{"ages/medieval/prayer.vg",
#include "ages/medieval/prayer.vg"
},
{"ages/medieval/account_herbal.vg",
#include "ages/medieval/account_herbal.vg"
},
{"ages/medieval/pilgrim.vg",
#include "ages/medieval/pilgrim.vg"
},
{"core/history.vg",
#include "core/history.vg"
},
{"peoples/sunlost/sunlost.vg",
#include "peoples/sunlost/sunlost.vg"
},
{"peoples/sunlost/omens.vg",
#include "peoples/sunlost/omens.vg"
},
{"peoples/sunlost/sunmyth.vg",
#include "peoples/sunlost/sunmyth.vg"
},
{"peoples/sunlost/sunmeasure.vg",
#include "peoples/sunlost/sunmeasure.vg"
},
{"peoples/sunlost/sunletter.vg",
#include "peoples/sunlost/sunletter.vg"
},
{"peoples/sunlost/daycount.vg",
#include "peoples/sunlost/daycount.vg"
},
{"peoples/sunlost/deephalls.vg",
#include "peoples/sunlost/deephalls.vg"
},
{"peoples/sunlost/sunlesson.vg",
#include "peoples/sunlost/sunlesson.vg"
},
{"peoples/sunlost/lastbroadcast.vg",
#include "peoples/sunlost/lastbroadcast.vg"
},
{"peoples/sunlost/sun_events.vg",
#include "peoples/sunlost/sun_events.vg"
},
{"peoples/sunlost/daycount2.vg",
#include "peoples/sunlost/daycount2.vg"
},
{"peoples/sunlost/deephalls2.vg",
#include "peoples/sunlost/deephalls2.vg"
},
