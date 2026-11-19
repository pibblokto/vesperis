#pragma once
// Z-03 (2026-11-17): what a vial holds (R-425; docs/ideas/IDEAS-small-and-spacious.md section 1.7). The explorer chooses where life would
// be: the ground at the crosshair (`surface/sampling` reads its microhabitat) or the clouds' temperate height (the capsule's scoop). What a
// sample holds is a pure function of its microhabitat and its world's biota, the microhabitat a function of its square metre: the same place
// gives the same sample, whenever and from whichever landing it is taken. Its activity is read on the spot as the Viking landers' labelled
// release read metabolism (Levin and Straat 1976): a hint that can mislead (spores read none, a protocell's chemistry reads faint)
#include "life/biota.h"
#include "core/types.h"
#include <cstdint>
#include <vector>

// a place's microhabitat: its niche, its own mean temperature (a spring's band, a vent's, else the ground's year round), its salt, whether the
// sun reaches it, whether its rock is old enough to keep fossils, and the square metre it is (`placeKey`)
struct Microhabitat {
    int niche = NI_DUST;
    double meanC = 0;
    int salt = 0;            // 0 fresh, 1 the sea's, 2 brine
    bool shade = false;      // out of the sun the year round (a slope turned from it, a cliff's foot, under trees)
    bool oldRock = false;    // a world's old rock (its fossils, where its life ended or began long ago)
    bool bloom = false;      // its life seen crowded there (red snow, a spring's mats in their band): ten to thirty times its niche's crowd
    uint64_t place = 0;
};

// the activity a vial reads on the spot
enum VialActivity { VA_NONE = 0, VA_FAINT, VA_STRONG, VA_COUNT };
extern const char* const VIAL_ACTIVITY_NAMES[VA_COUNT];   // "NONE", "FAINT", "STRONG"

// one species in a vial: its index in its world's list, what it is there (a cell alive, a spore, a fossil, a protocell), how many in a
// millilitre (a fossil: how many a section shows), and whether it grows at the place's temperature
struct Held {
    int species = -1;
    int state = SS_LIVING;
    double cells = 0;
    bool active = false;
    // Z-05: a culture's (a flask's drop): the form its cells are in (-1 its state's), the share found dividing (-1 a sample's few)
    int form = -1;
    double dividing = -1;
    // R-438: a culture's verdict whether its cells live (1: waiting for their warmth, hungry or in the dark) or die (0: too hot, the wrong salt
    // or solvent); -1 a sample's, its drop's salt deciding (the slide's stage sets their warmth)
    int alive = -1;
};
struct Sample {
    std::vector<Held> held;   // in the world's order
    int activity = VA_NONE;
    bool organics = false;    // a sterile world's: organics fallen from space, cooked (the furnaces' and the dead rocks' grains alone)
    double activeLog = -1;    // R-434: the crowd at work, log10 cells a millilitre (-1 none at work): what the activity's words are read from
    int living() const { int n = 0; for (const Held& h : held) n += h.state == SS_LIVING; return n; }
};

// the square metre: the body-fixed point (a unit vector on the body) rounded to the metre on its radius, hashed with the body's seed
uint64_t placeKey(uint64_t bodySeed, const Vec3& unit, double radiusKm);
// what a vial of the microhabitat holds on a world with this biota (a pure function of the two)
Sample sampleOf(const Biota& B, const Microhabitat& h);
// the living films' density in a niche, 0..1 (the lamp's ultraviolet: their glow by it): the living cells' shares there, none where nothing
// lives or only spores, fossils and protocells are
double filmDensity(const Biota& B, int niche);
// the colour a world's films glow under the ultraviolet by the pigment most of its living cells carry: chlorophyll's deep red (Kiang 2007's
// green and blue-green), rhodopsin's and the carotenoids' orange, a chemotroph's flavins' blue-green, the dark pigments' dim blue-white
Vec3 filmGlowColour(const Biota& B);
