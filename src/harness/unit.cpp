// E-03: the unit: `unit [list] [-q] [group ...]` (testUnit) runs UNIT_GROUPS, its groups in the unit's order (their bodies in
// unit_*.cpp, the peoples' *Checks in the civ_ files), each from the scale and mush it starts at in the full run
#include "harness/harness.h"
#if defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>   // E-03: `unit -q`'s /dev/null
#endif

// E-03: `unit -q`: stdout goes to /dev/null while the groups run, so only the failures (written to the stdout saved) and the count
// show; where there is no dup2 (Windows), only the checks' own lines are held back
QuietStdout::QuietStdout(bool on) {
    if (!on) return;
    fflush(stdout);
#if defined(__APPLE__) || defined(__linux__)
    fd = dup(1);
    const int devnull = open("/dev/null", O_WRONLY);
    if (fd >= 0 && devnull >= 0 && dup2(devnull, 1) >= 0) {
        close(devnull);
        saved = fdopen(dup(fd), "w");
        return;
    }
    if (devnull >= 0) close(devnull);
    if (fd >= 0) close(fd);
    fd = -1;
#endif
    saved = stdout;
}

void QuietStdout::restore() {
    if (!saved) return;
    fflush(stdout);
#if defined(__APPLE__) || defined(__linux__)
    if (fd >= 0) {
        fclose(saved);
        dup2(fd, 1);
        close(fd);
        fd = -1;
    }
#endif
    saved = nullptr;
}

// E-03: the unit's groups in the unit's order: `unit` runs them all (its printout the one testUnit made in one function),
// `unit <group> ...` those, `unit list` names them, `-q` prints only the failures and the count. A group starts at the render
// scale and mush the groups before it leave in the full run (a scripted game leaves its own, 4x): set before it runs, so that a
// group alone draws as it does in the full run
const UnitGroup UNIT_GROUPS[] = {
    {"noise", "unit_basics.cpp", 1, 2, unitNoise},
    {"kinds", "unit_kinds.cpp", 1, 2, unitKinds},
    {"dead", "unit_kinds.cpp", 1, 2, unitDead},
    {"cold", "unit_kinds.cpp", 1, 2, unitCold},
    {"moons", "unit_kinds.cpp", 1, 2, unitMoons},
    {"giants", "unit_kinds.cpp", 1, 2, unitGiants},
    {"traits", "unit_kinds.cpp", 1, 2, unitTraits},
    {"drifters", "unit_drifters.cpp", 1, 2, unitDrifters},
    {"basics", "unit_basics.cpp", 4, 3, unitBasics},
    {"signals", "unit_signals.cpp", 4, 3, unitSignals},
    {"forms", "civ_forms.cpp", 4, 3, [](const Check& c) { formsChecks(c); }},
    {"ages", "civ_peoples.cpp", 1, 2, [](const Check& c) { agesChecks(c); }},
    {"genome", "civ_genome.cpp", 1, 2, [](const Check& c) { genomeChecks(c); }},
    {"plan", "civ_plan.cpp", 1, 2, [](const Check& c) { planChecks(c); }},
    {"history", "civ_peoples.cpp", 4, 3, [](const Check& c) { historyChecks(c); }},
    {"towns", "civ_plan.cpp", 4, 3, [](const Check& c) { townChecks(c); }},
    {"works", "civ_works.cpp", 4, 3, [](const Check& c) { worksChecks(c); }},
    {"records", "civ_records.cpp", 4, 3, [](const Check& c) { recordsChecks(c); }},
    {"walls", "civ_walls.cpp", 1, 2, [](const Check& c) { wallsChecks(c); }},
    {"grammar", "civ_grammar.cpp", 1, 2, [](const Check& c) { grammarChecks(c); }},
    {"media", "civ_records.cpp", 1, 2, [](const Check& c) { mediaChecks(c); }},
    {"pages", "civ_records.cpp", 4, 3, [](const Check& c) { historyPageChecks(c); }},
    {"instruments", "civ_instruments.cpp", 4, 3, [](const Check& c) { instrumentChecks(c); }},
    {"makers", "civ_instruments.cpp", 1, 2, [](const Check& c) { makerChecks(c); }},
    {"player", "civ_instruments.cpp", 1, 2, [](const Check& c) { playerChecks(c); }},
    {"drones", "modes_drones.cpp", 1, 2, [](const Check& c) { droneChecks(c); }},
    {"navigation", "unit_navigation.cpp", 1, 2, unitNavigation},
    {"sights", "unit_navigation.cpp", 4, 3, unitSights},
    {"stars", "unit_sky.cpp", 4, 3, unitStars},
    {"civilisations", "unit_civ.cpp", 4, 3, unitCivilisations},
    {"ruins", "unit_civ.cpp", 4, 3, unitRuins},
    {"charts", "unit_civ.cpp", 4, 3, unitCharts},
    {"roads", "unit_civ.cpp", 4, 3, unitRoads},
    {"graves", "unit_civ.cpp", 4, 3, unitGraves},
    {"cultures", "unit_peoples.cpp", 4, 3, unitCultures},
    {"profiles", "unit_peoples.cpp", 4, 3, unitProfiles},
    {"story", "unit_peoples.cpp", 4, 3, unitStory},
    {"sky", "unit_sky.cpp", 4, 3, unitSky},
    {"probe", "unit_probe.cpp", 1, 2, unitProbe},
    {"perf", "unit_proof.cpp", 4, 3, unitPerf},
    {"auto", "unit_navigation.cpp", 1, 2, unitAuto},
    {"states", "unit_navigation.cpp", 1, 2, unitStates},
};

int testUnit(int argc, char** argv) {
    bool quiet = false, list = false;
    std::vector<std::string> names;
    for (int i = 2; i < argc; i++) {
        const std::string a = argv[i];
        if (a == "-q") quiet = true; else if (a == "list") list = true; else names.push_back(a);
    }
    if (list) { for (const UnitGroup& g : UNIT_GROUPS) printf("%-14s %s\n", g.name, g.file); return 0; }
    for (const std::string& n : names)
        if (std::none_of(std::begin(UNIT_GROUPS), std::end(UNIT_GROUPS), [&](const UnitGroup& g) { return n == g.name; })) {
            printf("unit: no group %s (`unit list` names them)\n", n.c_str());
            return 2;
        }
    int fails = 0;
    int checks = 0;   // F-01: the count the reports give
    Check check{&checks, &fails};
    QuietStdout q(quiet);   // -q: stdout silenced, the failures written to the one saved
    check.failsTo = q.saved;
    for (const UnitGroup& g : UNIT_GROUPS)
        if (names.empty() || std::find(names.begin(), names.end(), g.name) != names.end()) {
            setFramebufferScale(g.scale);
            g_mushKernel = g.mush;
            g.run(check);
        }
    q.restore();
    printf("unit: %d checks, %d failures\n", checks, fails);
    return fails;
}
