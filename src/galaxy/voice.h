#pragma once
// C-05 (2026-10-02): a people's voice. A text shard is a recording of someone speaking the people's tongue (`Tongue`, C-06):
// the spoken words are the written ones, the people's word for each of ours, so the speech and the decoded text agree word for
// word. A world's voice (`voiceOf`, from the body's seed) is a pitch, a range, a rate, the length of the tract (the formants'
// scale), a quality (breath, roughness, a nasal murmur, a tremor, a second tone under the first, a whisper), a stress rule, a
// sentence melody and the marks of its sounds (clicks for the voiceless stops, rolled r's, tones on the syllables). The
// inventory is the sounds the tongue's pools spell (`PHONES`: a vowel's formants, a consonant's manner, place and noise);
// `speechOf` turns a decoded shard into segments (a phone each, with its time, its pitch and its loudness) by the voice's
// prosody, and keeps where each word starts and ends, which is what the decoder screen aligns the text to. `AudioSynth`
// speaks the segments (`game/audio.h`: a glottal source and four formants in cascade, noise for the fricatives and the bursts).
#include "shards.h"
#include <string>
#include <vector>

enum PhoneKind { PK_VOWEL = 0, PK_STOP, PK_FRIC, PK_NASAL, PK_LIQUID, PK_GLIDE, PK_PAUSE, PK_COUNT };

struct Phone {                  // one sound as the tongue spells it
    const char* sym;            // the spelling ("a", "ai", "sh"; a cluster is several phones)
    int kind;
    bool voiced;
    double f1, f2, f3;          // a vowel's formants, Hz (a diphthong ends at f1e..f3e); a consonant's locus for the transitions
    double f1e, f2e, f3e;
    double noiseF, noiseBw;     // frication or a stop's burst: the band's centre and width (0: breath through the formants)
    double noiseAmp;            // its level (0: none)
    double dur;                 // seconds at the base rate
};
extern const Phone PHONES[];    // the whole table, longest spellings first
int phoneCount();
int phoneOf(const char* sym);   // the phone spelt so, -1 if none

struct Voice {
    double pitch = 160;         // the voice's base, Hz
    double range = 1.0;         // the melody's span (0.5 flat .. 1.6 lively)
    double rate = 3.8;          // syllables a second
    double tract = 1.0;         // the formants' scale (a short tract high, a long one deep)
    double breath = 0.2;        // aspiration under the voicing (0.9 and over: a whisper)
    double rough = 0.0;         // the period's wobble
    double nasal = 0.0;         // a nasal murmur in the vowels
    double tremor = 0.0;        // a slow vibrato on the long vowels
    double sub = 0.0;           // a second tone under the first (a share; its pitch `subRatio`)
    double subRatio = 0.5;
    int stress = 0;             // 0 the first syllable, 1 the last, 2 the last but one, 3 none (all even)
    int contour = 0;            // the sentence's melody: 0 falling through, 1 rising to a peak then falling, 2 level with a drop at the end
    bool tonal = false;         // a tone on each syllable (level high or low, rising, falling) from the word
    bool clicky = false;        // the voiceless stops are clicks
    bool trill = false;         // the r's are rolled
    std::vector<int> inventory; // the phones the tongue's pools use, for the print
    uint64_t seed = 0;
};
Voice voiceOf(const BodyGen& g, const Lore& L, const Tongue& T);
std::string voiceLine(const Voice& V);   // "A DEEP, ROUGH VOICE, SLOW, WITH CLICKS" (52 characters at most)

struct Segment {
    int phone = -1;             // index into PHONES; -1 a pause
    int part = 0;               // a stop: 0 the closure, 1 the burst (and its aspiration); a click: 1 the click
    double t = 0, dur = 0;      // seconds from the speech's start
    double pitch = 1, pitchEnd = 1;   // relative to the voice's base, at the segment's start and its end
    double amp = 1;             // loudness
    int word = -1;              // the decoded word it belongs to (-1 a pause between)
    int syllable = 0;           // its syllable in the word
    bool stressed = false;
    bool trill = false;         // rolled
};
struct Speech {
    std::vector<Segment> segs;              // in time
    std::vector<double> wordStart, wordEnd; // per decoded word, seconds (a word without sounds: both the time it would be at)
    std::vector<std::string> phones;        // per decoded word, its sounds spelt with '-' between ("tr-ie") for the harness
    double seconds = 0;
    int syllables = 0;
    uint64_t seed = 0;
};
void speechOf(const Voice& V, const std::vector<DecodedWord>& words, uint64_t seed, Speech& out);
int speechWordAt(const Speech& S, double t);   // the word being spoken at that time: the last begun (-1 before the first)
void parsePhones(const std::string& word, std::vector<int>& out);   // a word's letters to phones, longest spelling first
