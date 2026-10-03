// C-02 (2026-10-02): the story grammar (`shards.h`). The texts are translations, so their words are ours; what is theirs is
// the names, the sky, the land and the water, read from the world. Templates use three marks: `{slot}` draws one entry of
// a slot (a lore name has one entry, a vocabulary slot several, a slot's entry may hold marks of its own), `[a|b|c]`
// draws one alternative. Everything is drawn from the shard's own `Rng`, so a shard is the same text every time.
#include "shards.h"
#include "charts.h"   // C-10: the charts' marks and the chart a caption names
#include "../core/rng.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>

const char* const SHARD_TONE_NAMES[ST_TONE_COUNT] = {"ordinary", "elegy", "warning", "the end"};

namespace {

constexpr double DEG_ = 3.14159265358979323846 / 180.0;

struct Grammar {
    std::map<std::string, std::vector<std::string>> rules;
    void set(const std::string& k, const std::vector<std::string>& v) { rules[k] = v; }
    void add(const std::string& k, const std::string& v) { rules[k].push_back(v); }
    std::string expand(const std::string& t, Rng& rng, int depth) const {
        std::string out;
        for (size_t i = 0; i < t.size();) {
            char c = t[i];
            if (c == '[') {
                int lvl = 0; size_t j = i, start = i + 1;
                std::vector<std::string> opts;
                for (; j < t.size(); j++) {
                    if (t[j] == '[') lvl++;
                    else if (t[j] == ']') { lvl--; if (lvl == 0) break; }
                    else if (t[j] == '|' && lvl == 1) { opts.push_back(t.substr(start, j - start)); start = j + 1; }
                }
                opts.push_back(t.substr(start, j - start));
                out += expand(opts[rng.irange((int)opts.size())], rng, depth + 1);
                i = j + 1;
            } else if (c == '{') {
                size_t j = t.find('}', i);
                if (j == std::string::npos) { out += t.substr(i); break; }
                std::string key = t.substr(i + 1, j - i - 1);
                auto it = rules.find(key);
                if (it == rules.end() || it->second.empty() || depth > 12) out += "{" + key + "}";   // left as a mark: the unit catches it
                else out += expand(it->second[rng.irange((int)it->second.size())], rng, depth + 1);
                i = j + 1;
            } else { out += c; i++; }
        }
        return out;
    }
};

bool vowel(char c) { c = (char)std::tolower((unsigned char)c); return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }

// the seams of the templates: one space between words, none before a stop, "a" before a vowel is "an", a sentence starts
// with a capital and the text ends with a stop
std::string tidy(const std::string& in) {
    std::string s;
    for (size_t i = 0; i < in.size(); i++) {
        char c = in[i];
        if (c == ' ' && (s.empty() || s.back() == ' ')) continue;
        if ((c == ',' || c == '.' || c == ';' || c == ':' || c == '?' || c == '!') && !s.empty() && s.back() == ' ') s.pop_back();
        s += c;
    }
    while (!s.empty() && s.back() == ' ') s.pop_back();
    std::string o;
    for (size_t i = 0; i < s.size(); i++) {
        bool wordStart = i == 0 || s[i - 1] == ' ';
        if (wordStart && (s[i] == 'a' || s[i] == 'A') && i + 2 < s.size() && s[i + 1] == ' ' && vowel(s[i + 2])) { o += s[i]; o += 'n'; continue; }
        o += s[i];
    }
    s = o;
    bool cap = true;
    for (size_t i = 0; i < s.size(); i++) {
        if (cap && std::isalpha((unsigned char)s[i])) { s[i] = (char)std::toupper((unsigned char)s[i]); cap = false; }
        if (s[i] == '.' || s[i] == '?' || s[i] == '!') cap = true;
    }
    if (!s.empty() && s.back() != '.' && s.back() != '?' && s.back() != '!') s += '.';
    return s;
}

// a kind's `until` (desert worlds only): the last age it is told at, for what speaks of living water
struct Kind { const char* name; int tone; int world; double w; double until; std::vector<const char*> tpl; bool other = false; };   // world 0 both, 1 felisian, 2 desert; C-13: `other` only where the world had a second people

const std::vector<Kind>& kinds() {
    static const std::vector<Kind> K = {
        {"harvest", ST_ORDINARY, 0, 1.0, 1, {
            "The {crop} came in before {weather}. {n} baskets from the terraces under {mountain}, more than last year. {kin} says {god} was kind to us. We keep the best for {festival}.",
            "{year}. The {crop} is in: {nbig} jars, the hall full to the beams. {child} carried the last basket {him}self and would not let anyone help.",
            "A thin year. The {crop} came small and late and {weather} took a third. We will eat, [{kin} says|the old ones say], but we will not sell to {city2}. {god} gives and {god} keeps.",
            "[First|Last] day of the {crop}. {sun} up before us, {skyone} still out. {child} fell asleep in the baskets and we carried {him} home with the rest.",
            "The {crop} is cut and the terraces are bare to the stone. {n} days of it and every back in {city} bent. Tonight the {crop} beer, tomorrow the {work}. {god} be thanked, and the rain."}},
        {"child", ST_ORDINARY, 0, 1.0, 1, {
            "{child} walked today, from the door to the well and back. {kin} laughed until {he} cried. I have cut the mark in the lintel under {founder}'s.",
            "{child} asked why {skyone} follows us home. I said it does not follow, it is only far. {he} did not believe me. Neither did I, at {his} age.",
            "{child} lost {his} first tooth and we put it under the stela for {god}. {he} wanted it back by evening. {kin} carved {him} a {beast} from the hearth wood instead.",
            "{child} can count to {n} now and counts everything: the {animal}, the jars, the stars over {mountain}. {kin} says {he} will keep the count one day. {he} says {he} will keep the {animal}.",
            "{child} was lost half a day and found asleep under the {tree} by {water}, with a {beast} {he} had caught. {he} did not understand why we were angry. Nor, by evening, did we."}},
        {"weather", ST_ORDINARY, 0, 1.0, 1, {
            "{weather} for {n} days. The roofs hold. {skyone} was hidden all week and the old ones said {god} had turned away, but it is only weather.",
            "No rain since {festival}. The terraces are dust to the knee. The old ones say a year like this comes [once in a life|twice in a hundred years|once in a long life]. I have seen [two|three|more].",
            "{weather} off {land} again, the {nth} time since {season}. We ate in the dark with the door shut and {kin} told the story of {founder} and the wall, the long way.",
            "The wind turned in the night and brought {weather} down off {mountain}. We lost {n} {animal} and a roof. {child} slept through it. {god} keeps the children, {kin} says, and leaves the roofs to us."}},
        {"the river runs", ST_ORDINARY, 0, 0.8, 0.5, {
            "{weather} broke at last and {river} runs brown. The {animal} are out. {child} stood in the doorway and shouted at {sun} until {kin} brought {him} in.",
            "{river} is up to the third step and the boats are tied to the gate. {n} days, the old ones say, then the {crop} goes in. {child} wants to swim. {child} always wants to swim."}},
        {"season", ST_ORDINARY, 0, 0.9, 0.5, {
            "Sing {season}, sing {river} full. Sing the {crop} green on the slope. Sing {god} who turns the year. Sing {child}, who will sing it after us.",
            "{season} again. The {tree} along {river} are [in leaf|bare|turning|loud with birds]. {kin} opened the north door for the first time since {weather}. {skyone} is early tonight.",
            "First day of {season} by {founder}'s stone: the shadow touches the gate at noon. The {animal} go up toward {mountain}. {child} went with them this year, for the first time."}},
        {"the turn of the year", ST_ORDINARY, 0, 0.6, 1, {
            "{season} again, by {founder}'s stone. The {tree} are [in leaf|bare|turning]. {kin} opened the north door for the first time since {weather}. {skyone} is early tonight.",
            "The year turned last night: {star} over the gate at first light, as {founder} marked it. We cut the {yearn}th mark in the lintel. {child} held the chisel."}},
        {"prayer", ST_ORDINARY, 0, 0.9, 1, {
            "{god}, keep {city} through {season}. Keep the walls, keep the well, keep {kin} on the road to {city2}. We have given {offering}. We ask for another year, nothing more.",
            "{god} of {mountain}, {god} of {river}, we set {offering} on the platform and we say the words {founder} said [over the first wall|at the founding|every year since]. If we say them wrong, [forgive us|be patient|say nothing]. We are the {people}; we are still here.",
            "A small prayer, for {child}, who is sick. {god}, you have {nbig} of us. Leave me this one.",
            "{god}, this is {kinv} speaking, from the roof, because the hall is full of people who want things. I want nothing. I wanted to say the {crop} is good and {child} is well. That is all."}},
        {"founding", ST_ORDINARY, 0, 0.8, 1, {
            "{founder} came over {mountain} with {n} families and found {water} and said: here. That was {yearn} years ago. The first wall is the one by the gate; {founder}'s mark is still on it.",
            "They say {founder} chose {city} because {skyone} stood over {mountain} [the night they came|the first night|the night of the founding], and the {people} took it for a sign. {kin} says it was the water. Both are true.",
            "Before {founder} the {people} had no walls and no stelae and did not count the years. Now we have {yearn} years cut into the hall's lintel. {child} can read them all."}},
        {"love", ST_ORDINARY, 0, 0.9, 1, {
            "I waited at {water} until {skyone} rose. {he} did not come. {kin} says the {work} will keep me from thinking. It does not.",
            "{he} brought me {trade} from {city2} and said nothing, and I said nothing, and now the whole of {city} knows[ by noon| already| and laughs|]. {god}, let {kin} say yes.",
            "We were married on the platform at {festival}, {skyone} up, the {animal} bawling in the pens. {he} laughed at the words[, and so did the priest| and the priest did not| and I did too|]. I will remember that longer than the words.",
            "{he} is from {city2} and says our {god} is a stranger and our {crop} is thin. {he} has stayed {n} years. {he} says it is for the {work}."}},
        {"lullaby", ST_ORDINARY, 0, 0.7, 1, {
            "Sleep, {child}, {skyone} is watching. Sleep, the {animal} are in. {god} holds the night and {river} holds the day. Sleep, and the {crop} will grow.",
            "Hush, the wind is only {weather}. Hush, the dark is only {mountain}. {founder} built the wall and the wall is strong. Sleep, {child}, sleep.",
            "The {beast} is in the pen and the lamp is in the niche. {sun} has gone to {city2} and {skyone} has come to us. Sleep, {child}; {god} counts the sleepers and {god} counts you."}},
        {"count", ST_ORDINARY, 0, 0.7, 1, {
            "{year}. {n} jars of {crop}, {n} of oil, {n} skins, {trade} for {city2}. The {animal}: {nbig}, {n} of them lame. Firewood to {season}. {kin} counted twice.",
            "The well: {n} spans to the water at the end of {season}. The cistern: full. The {crop}: {nbig} baskets. Births: {n}. Deaths: {n}. {god} keep the count."}},
        {"letter", ST_ORDINARY, 0, 0.9, 1, {
            "To {kin} in {city2}: the road over {mountain} is open again. Bring {trade}, and seed if they have it. {child} asks every day when you come. {god} keep you.",
            "{kinv}, {weather} has not reached us yet; if it comes your way, keep the {animal} in. The {crop} is fine. Do not worry about the roof. Come for {festival}.",
            "{kinv}: they are asking in {city2} what we want for the {trade}. Say {n} jars and no less; {founder} would have said {n}. {child} sends you a {beast} drawn in ash. {he} says it is you."}},
        {"sky", ST_ORDINARY, 0, 1.0, 1, {
            "{skyone} {skyverb} tonight. The old ones say it means {omen}. I do not know. I sat on the roof with {child} and we counted the stars we know: {star} first, then the rest.",
            "{sun} was red at its setting for {n} days. {kin} says it is the haze off {land}. {child} says {god} is tired. The {animal} did not mind either way.",
            "{bright} over {mountain} and the stars thick beside it. We put out the lamps [to see|and sat on the wall|and woke the children]. {founder} must have seen the same; [the stones do not change, only the people|the stones are the same and the people are not|the stones keep what the people lose].",
            "{star} is back over the gate at first light, as it is every year at the start of {season}. The old ones say the {people} set the gate by it. {child} says the star came after the gate, to look."}},
        {"work song", ST_ORDINARY, 0, 0.7, 1, {
            "Lift and set, lift and set. {founder} built the wall and we build the rest. Lift and set, {weather} is coming, {n} courses to the gate. {god} counts the stones and {god} counts us.",
            "Dig for {river}, dig for {god}, the well goes down and the bucket comes up. {kin} on the rope, {child} on the wall, {n} spans to water and {n} more to go."}},
        {"naming", ST_ORDINARY, 0, 0.7, 1, {
            "We named {him} {child}, after {kin}'s mother. {god} was given the first cup. The {animal} did not stop bawling all night; the old ones say that is a good sign.",
            "{child} is {his} name now, cut into the stela beside {founder}'s. {he} slept through the whole of it. {skyone} was up when we carried {him} home."}},
        {"the hunt", ST_ORDINARY, 0, 0.7, 0.5, {
            "The nets came up empty below {water} for {n} days, then full. {kin} says the fish follow {skyone}. {child} says they follow {kin}.",
            "{n} {animal} taken on the slopes of {mountain}, two lost to {land}. Enough until {season}. We left the first for {god} on the high stone."}},
        {"festival", ST_ORDINARY, 0, 0.9, 1, {
            "{festival} again. Fires on {mountain}, the {crop} beer, {child} dancing on the platform until {sun} came up. {kin} fell asleep against the stela. A good year.",
            "At {festival} we read the years from the lintel, all {yearn} of them, and {child} read the last one. Then the {animal} were let out and the children ran after them to {water}. {god} was pleased, I think.",
            "{festival}: {n} {animal} for the fire, {n} jars opened, the {work} shut for {n} days. {kin} danced, which {he} does once a year and badly. {child} has not stopped laughing."}},
        {"traveller", ST_ORDINARY, 0, 0.8, 1, {
            "A stranger came over {mountain} from {n} days beyond {city2}, speaking our words wrong. {he} said their {waterkind} had [gone|failed|turned]. We gave {him} bread and {he} went on toward {sea}.",
            "Traders from {city2} with {trade}: {n} days on the road, {weather} the whole way. They asked about {god}. They have one of their own, and {kin} says it is the same one with another name.",
            "A woman from {city2} stayed the winter and taught {child} to {skill}. {he} asked nothing for it. When {he} left, {he} took the road toward {sea} and not the road home."}},
        {"council", ST_ORDINARY, 1, 0.5, 1, {
            "The council sat until {skyone} set. {kin} wants the new terrace [above the gate|by the well|on the north slope]; the old ones want it [below|where it was|nowhere]. {founder} would have laughed at us. We will cut it where the water is."}},
        {"elegy", ST_ELEGY, 0, 1.0, 1, {
            "{kin} died in {season}, in the house by the gate. We laid {him} under the stela with {offering}. {he} taught me the {work}. The house is quiet. {child} asks where {he} is and I say: with {god}.",
            "The old one who kept the count is gone. {nbig} years cut by {his} hand, and the last mark is [thin|crooked|half cut]. [I will cut the next one|The next one is mine|{child} will cut the next one]. I do not have {his} hand.",
            "For {kin}, who went up {mountain} after the {herd} in {weather} and did not come down. We found the {herd}. {god} keep {him}, wherever {he} is walking.",
            "{kin} is dead and the {work} is mine now. {he} said I would never learn it. {he} was right for {n} years and wrong this morning. I wish {he} had seen it."}},
        {"a place", ST_ELEGY, 1, 0.5, 1, {
            "When I was small the lights of {city2} could be seen from {mountain} at night. Nobody has gone there in {n} years. The road is under the {tree}."}},
        {"the sea going", ST_WARNING, 2, 1.0, 1, {
            "The shore of {sea} is [a day's|half a day's|two days'] walk from the old quay now. The boats lie on the salt. {kin} remembers the water at the steps. The children do not believe {him}.",
            "{year}. {sea} has gone back another {n} spans by {founder}'s rod. Salt where the fish were. We say the prayer at the quay still. {god} does not come to the quay any more.",
            "{child} asked what the quay was for. I said: for the boats. {he} asked what the boats were for. {kin} walked out onto the salt and did not answer anyone until dark.",
            "We walked to the water: {n} days out across the salt, and it is a pond, warm and bitter, with the bones of the fish round it. The old ones say {sea} was deeper than {mountain} is high. We came back."}},
        {"the river", ST_WARNING, 2, 1.0, 1, {
            "{river} did not come down this year. We dug in the bed and found water at {n} spans. Next year it will be deeper. The old ones say {god} is thirsty too.",
            "The bed of {river} is stones and the stones are [hot|sharp|white]. {kin} says {his} mother swam in it. We walk down it now to the sea, and the sea is salt and sand.",
            "The {tree} along {river} are dying [from the top|from the roots|one by one]. {kin} cut one to see and the heart was dry. We will not cut another; a dead tree is still shade."}},
        {"the heat", ST_WARNING, 2, 0.9, 1, {
            "{n} days above the old marks. The {crop} burned on the stalk. We work at night now, by {sky}, and sleep in the cisterns. {child} does not know it was ever otherwise.",
            "{sun} is the same sun. {kin} says so, the old ones say so. But the shade is shorter and the {animal} die in it, and the well is {n} spans deeper than {founder}'s count.",
            "We have moved the {work} into the cistern and the {animal} into the hall. The sun is a weight on the head [by the second hour|before noon|from the first hour]. {kin} says the {people} are a night people now, and laughs."}},
        {"the dust", ST_WARNING, 2, 0.9, 1, {
            "The dust came again off {land}. {n} days of it. {child} coughs. The {animal} will not leave the walls. {kin} says it was not like this when {he} was small.",
            "We shut the north door [for good|with stones|with mud and the old door] and built the wall higher against the sand. The {tree} along {river} are dead and the sand takes them. {god} of the wind, enough.",
            "The road to {city2} is under the dunes past the [second|third|last] stela. We dug it out [twice|three times|every spring]. {kin} says a road is for going somewhere, and where is there."}},
        {"warning", ST_WARNING, 2, 1.0, 1, {
            "To whoever hears this: {water} is going and nobody will say it. We measured with {founder}'s rod: {n} spans in {n} years. Do not stay for {festival}. Go to {dest}. Go now.",
            "{year}. I say this for the record, since the council will not: the sea is going, the river is gone, and {city2} is already empty. {kin} laughed. They always laugh. Cut it on the stela when I am dead.",
            "{kinv}, if you hear this, do not come back for the jars. There is no water on the road past {mountain} and the well here is {n} spans and falling. Stay in {dest}. I will come to you if I can."}},
        {"council", ST_WARNING, 2, 0.8, 1, {
            "The council sat until {skyone} set. {kin} said the well is lower every year and we must [dig deeper|move to {mountain}|go north|go to {city2} while the road is open]. They laughed. They always laugh. {child} was asleep on my knee the whole time.",
            "They voted for a new cistern. {kin} asked what we would fill it with. {n} of us walked out, which has never happened at a council of the {people}. {god} saw. {god} said nothing, as usual."}},
        {"leaving", ST_WARNING, 2, 0.9, 1, {
            "We leave {city} at first light. {n} families; the rest will not come. {kin} will not leave the stela. I have put the jars in the cistern and shut the door. {god}, if you are still here, keep the house.",
            "The road to {city2} is {n} days without water now. We go anyway. {child} carries the lamp. If anyone of the {people} finds this: we went toward {dest}, in {season}, in {year}.",
            "{n} families left for {dest} this morning with the last of the {animal}. We stood at the gate until the dust took them. {kin} said: that is {city} now, walking. Then we went in and shut the gate, I do not know from what."}},
        {"the fading", ST_WARNING, 1, 1.0, 1, {
            "The young ones went to {city2} after {festival} and did not come back. The {crop} stands uncut on the upper terrace. {kin} says they will return for {season}. {kin} said that last year.",
            "{n} houses on the hill stand empty. We keep their roofs, I do not know why. {child} plays in them. {river} is full; the {tree} are in leaf; it is only us that are fewer.",
            "The sickness again, in {city2} first, then here. {n} since {season} began. {god}, we have given {offering}; tell us what else you want."}},
        {"the last", ST_END, 2, 1.0, 1, {
            "I am the last at {city}. The well is [dry|sand|dust] at {nbig} spans. I set this in the wall by {founder}'s mark so that someone knows the {people} were here. We were here. {god}, we were here.",
            "No one answers from {city2}. The {animal} are gone, the {crop} is gone, {sea} is a white floor to the edge of the sky. {child}'s lamp is lit. I will keep it lit as long as the oil lasts.",
            "{year}. The last count: {n} of us, {n} jars, {n} days to {dest}. {kin} says stay. I say go. {god} says nothing. We will decide at first light.",
            "The gate is open and there is nobody to shut it against. {skyone} comes up over the salt the same as over the sea. I have read all {yearn} years off the lintel to {child}, who is asleep. This is the last mark; I am cutting it now."}},
        {"the last", ST_END, 1, 1.0, 1, {
            "No child was born in {city} [this year, or last|this year, or the year before|in {n} years]. The {crop} is good; {river} is full; it is only us. The houses on the hill stand empty. I keep the gate open in case.",
            "The last of the {people}, {kin} says, and laughs, because what else. We cut the year in the lintel anyway, {yearn} and one. {skyone} is up. The {animal} do not know anything is wrong, and they are right.",
            "I am old and the {work} is done and nobody will need it. {god}, you kept {city} {yearn} years. Keep the {tree}, keep {river}, keep the {animal}. They were always more yours than ours."}},
        // C-13: the other people of a world of two (`Lore::other`): traders, a buried stranger, their words, their silence, their want, their end
        {"the other people", ST_ORDINARY, 0, 1.2, 1, {
            "Traders of the {other} came over {mountain} with {trade} and {n} of their {animal}. They speak as if through a wall, but {kin} can follow them. We gave them {offering} and the good hall.",
            "{child} asked why the {other} do not pray to {god}. I said they have their own, on the far side of the water. {he} asked if theirs is listening. I said ours is, and we went in.",
            "A boat of the {other} at {water}, the first since {festival}. They brought {trade} and a child who had never seen {skyone} so low. {kin} laughed and gave {him} bread.",
            "One of the {other} died on the road home and we buried {him} by our gate. Their stones face the other way; we set {his} so. {founder} would have done the same, {kin} says.",
            "The {other} count the year from a different stone and call {star} by another name. {child} wants to learn their words. I said learn ours first. {he} is learning both."}, true},
        {"the other people", ST_ELEGY, 0, 0.8, 1, {
            "No one has come from the {other} since {season} before last. {kin} says the road over {mountain} is grown shut. I say they are busy. Nobody says what we think.",
            "We lit the fire on the headland for the {other}, as every year at {festival}. No fire answered from their shore. {child} asked if they had forgotten. I said the night was thick."}, true},
        {"the other people", ST_WARNING, 0, 0.8, 1, {
            "Word from the {other}: their {waterkind} is failing too. They are coming here, or going to the mountains, nobody knows which. {god}, we cannot feed them. We cannot turn them away.",
            "The {other} have sent {n} families over {mountain} and want land by {river}. The council sat all night. {kin} says give it; the old ones say no. I say they will not be the last to ask."}, true},
        {"the last", ST_END, 0, 1.0, 1, {
            "{otherfate} I set this in the wall by {founder}'s mark so that someone knows the {people} were here. {god}, we were here."}, true},
    };
    return K;
}

std::string numberWord(int n) {
    static const char* small[] = {"no", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve"};
    if (n >= 0 && n <= 12) return small[n];
    return std::to_string(n);
}

// a name of at most `maxLen` letters: the generator's, retried, or cut at a syllable
std::string shortName(Rng& rng, int style, size_t maxLen) {
    std::string s;
    for (int tries = 0; tries < 12; tries++) { s = generateName(rng.next(), style); if (s.size() <= maxLen && s.find(' ') == std::string::npos) return s; }
    size_t sp = s.find(' '); if (sp != std::string::npos) s = s.substr(0, sp);
    if (s.size() > maxLen) { size_t cut = maxLen; while (cut > 3 && !vowel(s[cut - 1])) cut--; s = s.substr(0, cut); }
    return s;
}

// the vocabulary of one shard: the world's (the sun by the star's class, the sky by the moons, the rings, the lights, the
// second sun, the comets and the wanderers; the land by the traits; the water, crops, trees and seasons by the type, the
// tilt and the climate) and the shard's own (the kin and the pronoun, the child's name, the year)
void buildGrammar(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, double age, Rng& rng, Grammar& G) {
    bool desert = L.desert;
    G.set("people", {L.people}); G.set("god", {L.god}); G.set("river", {L.river}); G.set("mountain", {L.mountain}); G.set("sea", {L.sea});
    G.set("city", {L.city}); G.set("city2", {L.city2}); G.set("founder", {L.founder}); G.set("star", {L.star}); G.set("festival", {L.festival});
    // C-13: the other people of the world and their fate (the "other" kinds alone use them; a world of one people never draws those)
    G.set("other", {L.other.empty() ? std::string("the others") : L.other});
    G.set("otherfate", {L.otherFate < 0 ? "The fires of the {other} went out {n} years before ours. We went to see; their gate stood open. Now it is ours that stands open."
                        : (L.otherFate > 0 ? "The {other} still light their fires across the water. {kin} says they will come for what we leave. Let them. Someone should."
                                           : "The {other} are quiet on their side of the water. {kin} says they sent a boat; it did not come. We are the last on both shores.")});
    // the sun: the class of the star the world circles (a companion's world has the companion's)
    int cls = sys.star.cls;
    if (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION) cls = sys.bodies[b.parent].starClass;
    static const char* const SUNS[STAR_CLASS_COUNT][2] = {
        {"the sun", "the sun"}, {"the small sun", "the orange sun"}, {"the white sun", "the great white sun"}, {"the great red sun", "the swollen sun"},
        {"the small white sun", "the hard little sun"}, {"the beating star", "the star that beats"}, {"the red sun", "the dim red sun"}, {"the blue sun", "the hard blue sun"},
        {"the amber sun", "the great amber sun"}, {"the ember sun", "the sun in its soot"}, {"the white point", "the needle sun"}, {"the young sun", "the sun in its cloud"},
        {"the burning sun", "the sun in its ring"}, {"the dark star", "the hole in the sky"}};
    if (cls < 0 || cls >= STAR_CLASS_COUNT) cls = 0;
    G.set("sun", {SUNS[cls][0], SUNS[cls][1]});
    // the sky: `{sky}` is anything up there, `{skyone}` one thing a singular verb can follow (the stars and the lights are many)
    std::vector<std::string> sky = {"the stars", SUNS[cls][0]}, one = {SUNS[cls][0]};
    bool companion = false, comet = false, giant = false;
    for (const Body& o : sys.bodies) { if (o.type == PT_COMPANION) companion = true; if (o.type == PT_COMET) comet = true; if (o.type == PT_GASGIANT && o.parent < 0) giant = true; }
    if (L.moons > 0) { sky.push_back(L.moon); sky.push_back(L.moon); one.push_back(L.moon); one.push_back(L.moon); G.set("bright", {L.moon + " full"}); } else { one.push_back(L.star); G.set("bright", {L.star}); }
    if (L.moons > 1) sky.push_back(L.moons == 2 ? "the two moons" : "the moons");
    if (b.rings) { sky.push_back("the ring"); sky.push_back("the arch of the ring"); one.push_back("the ring"); }
    if (auroraPotentialAt(sys, b, 0.3) > 0.25) { sky.push_back("the lights in the north"); sky.push_back("the curtains of light"); }
    if (b.locked) { sky.push_back("the edge of the day"); sky.push_back("the line where the night begins"); }
    if (companion) { sky.push_back("the second sun"); sky.push_back("the lesser sun"); one.push_back("the second sun"); }
    if (comet) { sky.push_back("the hairy star"); one.push_back("the hairy star"); }
    if (giant) { sky.push_back("the wanderer"); one.push_back("the wanderer"); }
    if (L.dayHours > 2.5) sky.push_back("the long night");
    if (b.parent >= 0 && sys.bodies[b.parent].type != PT_COMPANION) { sky.push_back("the mother world"); sky.push_back("the great world overhead"); one.push_back("the mother world"); }
    G.set("sky", sky); G.set("skyone", one);
    G.set("skyverb", {"stood over {mountain}", "rose late and red", "was hidden by {weather}", "was bright enough to read by", "went out for a breath and came back", "stood still over the gate", "came up in the wrong place, {kin} swears"});
    // the land, by the traits
    std::vector<std::string> land;
    struct TL { int t; const char* s; };
    static const TL TLS[] = {{TR_CANYONS, "the canyons"}, {TR_MESAS, "the flat hills"}, {TR_KARST, "the stone towers"}, {TR_RIFT, "the great rift"}, {TR_ESCARPMENTS, "the scarps"},
        {TR_GLACIAL, "the ice valleys"}, {TR_INSELBERGS, "the lone hills"}, {TR_CINDER_FIELD, "the black fields"}, {TR_CHAOS, "the broken ground"}, {TR_POLYGONS, "the patterned ground"},
        {TR_YARDANGS, "the wind-cut ridges"}, {TR_ERG, "the sand sea"}, {TR_SALT_FLATS, "the salt flats"}, {TR_TRAPS, "the stair hills"}, {TR_GREAT_BASIN, "the great basin"},
        {TR_CORONAE, "the rings of hills"}, {TR_SPIRES, "the spires"}, {TR_GEYSERS, "the hot springs"}, {TR_ARCHIPELAGO, "the islands"}, {TR_PANGAEA, "the one land"},
        {TR_LAKELAND, "the lake country"}, {TR_SNOWBALL, "the ice"}, {TR_STORMS, "the storm country"}, {TR_HAZE, "the haze"}, {TR_GIANT_FLORA, "the great trees"},
        {TR_LUMINOUS_FLORA, "the glowing woods"}, {TR_DEAD_FOREST, "the dead forest"}, {TR_RED_SOIL, "the red earth"}, {TR_BLACK_SAND, "the black sand"}, {TR_CHALK, "the white cliffs"}};
    for (const TL& t : TLS) if (g.hasTrait(t.t)) land.push_back(t.s);
    if (desert) { land.push_back("the dry country"); land.push_back("the dunes"); }
    else { land.push_back("the hills"); land.push_back("the forest"); if (g.mountainAmp > 2000) land.push_back("the high mountains"); land.push_back("the marsh"); }
    G.set("land", land);
    // the water: a felisian world's rivers, lakes and sea; a desert's while they ran, then what is left
    if (!desert) G.set("water", {L.river, "the lake", "the sea", "the spring under {mountain}", "the shore"});
    else if (age < 0.5) G.set("water", {L.river, "the quay at " + L.sea, "the well", "the cistern", "the spring under {mountain}"});
    else G.set("water", {"the well", "the cistern", "the last spring", "the bed of " + L.river});
    G.set("waterkind", desert ? std::vector<std::string>{"well", "river", "spring", "sea"} : std::vector<std::string>{"river", "lake", "spring", "well"});
    G.set("dest", {L.city2, "the mountains", "the north", "the high country"});
    // crops, trees, animals
    double tempC = b.tempK + g.tempBias - 273.15;
    if (desert) G.set("crop", {"millet", "grain", "barley", "sesame"});
    else if (tempC < 5) G.set("crop", {"barley", "flax", "kale", "rye"});
    else G.set("crop", {"grain", "flax", "corn", "rye", "millet"});
    static const char* const TREES[10] = {"round trees", "needle trees", "flat-crowned trees", "great tiered trees", "stalk trees", "fern trees", "cap trees", "weeping trees", "candle trees", "spire trees"};
    std::vector<std::string> trees = {TREES[g.floraFamily >= 0 && g.floraFamily < 10 ? g.floraFamily : 0]};
    if (g.floraFamily2 >= 0 && g.floraFamily2 < 10 && g.floraFamily2 != g.floraFamily) trees.push_back(TREES[g.floraFamily2]);
    if (g.hasTrait(TR_LUMINOUS_FLORA)) trees.push_back("lamp trees");
    if (g.hasTrait(TR_GIANT_FLORA)) trees.push_back("great trees");
    if (desert) trees = {"thorn trees", "date palms"};
    G.set("tree", trees);
    G.set("animal", desert ? std::vector<std::string>{"goats", "sheep", "beasts"} : std::vector<std::string>{"goats", "geese", "sheep", "cattle"});
    G.set("beast", desert ? std::vector<std::string>{"goat", "lizard", "bird", "beetle"} : std::vector<std::string>{"goat", "bird", "fish", "hare"});
    G.set("herd", {desert ? (rng.chance(0.5) ? "goats" : "sheep") : (rng.chance(0.5) ? "goats" : "cattle")});
    G.set("skill", {"read the lintel", "mend a net", "cut a stela", "set a bone", "make the {crop} beer"});
    // seasons and weather
    double tilt = std::fabs(b.axialTilt) / DEG_;
    if (tilt > 8) G.set("season", {"spring", "summer", "autumn", "winter"});
    else if (desert) G.set("season", {"the cool months", "the hot months"});
    else G.set("season", {"the rains", "the dry months"});
    if (desert) G.set("weather", {"the dust", "the heat", "the wind", "the cold nights", "the sand"});
    else if (tempC < 5) G.set("weather", {"the snow", "the frost", "the fog", "the wind"});
    else G.set("weather", {"the rains", "the fog", "the heat", "the wind", "the storms"});
    G.set("work", {"loom", "kiln", "nets", "wall", "count", "well"});
    G.set("offering", {"the first cup", "bread and salt", "a goat", "the first of the {crop}", "oil and a lamp"});
    G.set("trade", {"salt", "iron", "cloth", "oil", "seed", "lamp oil"});
    G.set("omen", {"a good year", "a hard winter", "a birth", "a death in the council", "rain", "trouble from {city2}"});
    G.set("n", {"two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "twelve", "twenty"});
    G.set("nth", {"second", "third", "fourth", "fifth"});
    G.set("nbig", {"forty", "sixty", "a hundred", "two hundred", "three hundred"});
    // the shard's own: the kin and the pronoun that follows them (the child shares it), the child's name, the year
    static const char* const KIN[][5] = {{"my mother", "Mother", "she", "her", "her"}, {"my father", "Father", "he", "him", "his"}, {"my sister", "Sister", "she", "her", "her"}, {"my brother", "Brother", "he", "him", "his"},
        {"my daughter", "Daughter", "she", "her", "her"}, {"my son", "Son", "he", "him", "his"}, {"the old one", "Old one", "she", "her", "her"}, {"my wife", "Wife", "she", "her", "her"}, {"my husband", "Husband", "he", "him", "his"}};
    int k = rng.irange(9);
    G.set("kin", {KIN[k][0]}); G.set("kinv", {KIN[k][1]}); G.set("he", {KIN[k][2]}); G.set("him", {KIN[k][3]}); G.set("his", {KIN[k][4]});
    G.set("child", {shortName(rng, L.style, 7)});
    int yr = 1 + (int)std::lround(age * L.spanYears);
    G.set("year", {"year " + std::to_string(yr) + " of " + L.city});
    G.set("yearn", {yr <= 12 ? numberWord(yr) : std::to_string(yr)});
}

// C-12: the end's tone is the last recording's alone (`lastShardOf`); the late shards that drew it before are warnings on a
// desert and elegies on a felisian world, so one shard of a world closes its record (one draw either way: the stream stays)
int pickTone(bool desert, double age, Rng& rng) {
    double w[ST_TONE_COUNT];
    if (desert) {
        if (age > 0.85) { w[0] = 0.2; w[1] = 0.1; w[2] = 0.7; w[3] = 0; }
        else if (age >= 0.3) { w[0] = 0.37; w[1] = 0.08; w[2] = 0.55; w[3] = 0; }
        else { w[0] = 0.84; w[1] = 0.08; w[2] = 0.08; w[3] = 0; }
    } else {
        if (age > 0.9) { w[0] = 0.5; w[1] = 0.35; w[2] = 0.15; w[3] = 0; }
        else { w[0] = 0.83; w[1] = 0.12; w[2] = 0.05; w[3] = 0; }
    }
    return rng.pick(w, ST_TONE_COUNT);
}

}   // namespace

namespace {
// the names and the span: the lore's own draws (C-10 reads the people's name alone for the peoples a chart names)
void loreNames(const BodyGen& g, Lore& L) {
    Rng rng(g.seed ^ 0x10BE5ULL);
    double sw[3] = {0.4, 0.3, 0.3};
    L.style = rng.pick(sw, 3);
    std::vector<std::string> used;
    auto name = [&]() {
        for (int tries = 0; tries < 12; tries++) {
            std::string s = shortName(rng, L.style, 9);
            if (std::find(used.begin(), used.end(), s) == used.end()) { used.push_back(s); return s; }
        }
        return shortName(rng, L.style, 9);
    };
    L.people = name(); L.god = name(); L.river = name(); L.mountain = name(); L.sea = name(); L.city = name(); L.city2 = name();
    L.founder = name(); L.moon = name(); L.star = name(); L.festival = name();
    L.spanYears = 150 + (int)(450 * rng.uni());
}
}

// C-12: the calendar. The day is the world's turn and the year its orbit (`yearDays` their ratio); the month is the nearest
// moon's round in those days where a moon rounds in three days at least and twice within the year. A world whose day is a quarter
// of its year or more (a locked world: its day is its year) counts no days: moons alone when it has one to count by, else years
namespace { void calendarOf(const StarSystem& sys, const Body& b, Lore& L) {
    L.dayHours = std::fabs(b.rotPeriod) / 3600.0;
    L.yearDays = b.orbitPeriod / std::max(std::fabs(b.rotPeriod), 1.0);
    L.moons = b.moonCount;
    double nearest = 1e18;
    for (const Body& m : sys.bodies) if (m.parent == b.index && m.orbitPeriod > 0 && m.orbitPeriod < nearest) nearest = m.orbitPeriod;
    bool days = L.yearDays >= 4;
    L.monthDays = 0; L.months = 0;
    if (nearest < 1e17) {
        if (days) { int md = (int)std::lround(nearest / std::max(std::fabs(b.rotPeriod), 1.0)); if (md >= 3 && md * 2 <= L.yearDays) { L.monthDays = md; L.months = (int)std::floor(L.yearDays / md); } }
        else { int mo = (int)std::floor(b.orbitPeriod / nearest); if (mo >= 2) L.months = mo; }
    }
    L.calendar = days ? (L.months > 0 ? 1 : 0) : (L.months > 0 ? 2 : 3);
} }

Lore loreQuick(const StarSystem& sys, const Body& b, const BodyGen& g, int which) {
    Lore L;
    L.peoples = peoplesOf(g) > 1 ? 2 : 1;   // C-13: a world without the trait is read as one people (the harness's "what it would have said")
    L.which = which > 0 && L.peoples > 1 ? 1 : 0;
    const BodyGen pg = peopleGen(g, L.which);
    L.seed = pg.seed;
    loreNames(pg, L);
    calendarOf(sys, b, L);
    L.desert = b.type == PT_DESERT;
    L.last = lastShardOf(pg);
    if (L.peoples > 1) {   // C-13: the other people by name, and whether they ended before this people, with them or after
        L.other = peopleNameOf(peopleGen(g, 1 - L.which));
        if (!peoplesEndedTogether(g)) L.otherFate = cultureOf(g, 1 - L.which).ageYears > cultureOf(g, L.which).ageYears ? -1 : 1;
    }
    return L;
}

Lore loreOf(const StarSystem& sys, const Body& b, const BodyGen& g, int which) {
    Lore L = loreQuick(sys, b, g, which);
    BodyGen pg = g; pg.seed = L.seed;   // C-13: the people's draws
    chartMarksOf(sys, b, pg, L.marks);   // C-10: the stars their charts mark (the sky scanned once, a few milliseconds)
    return L;
}

std::string peopleNameOf(const BodyGen& g) { Lore L; loreNames(g, L); return L.people; }
int loreStyleOf(const BodyGen& g) { Rng rng(g.seed ^ 0x10BE5ULL); double sw[3] = {0.4, 0.3, 0.3}; return rng.pick(sw, 3); }   // C-12: `loreNames`' first draw
std::string personName(int style, uint64_t seed) { Rng rng(seed); return shortName(rng, style, 9); }   // C-12

std::string shardDate(const Lore& L, const Shard& s) {   // C-12
    std::string d = "year " + std::to_string(s.year) + " of " + L.city;
    if (L.calendar == 1) d += ", moon " + std::to_string(s.month) + ", day " + std::to_string(s.day);
    else if (L.calendar == 0) d += ", day " + std::to_string(s.day);
    else if (L.calendar == 2) d += ", moon " + std::to_string(s.month);
    return d;
}
std::string calendarLine(const Lore& L) {   // C-12: no unit of ours: their day is their own
    char buf[120];
    switch (L.calendar) {
        case 1: { int over = (int)std::lround(L.yearDays) - L.months * L.monthDays; if (over > 0) snprintf(buf, sizeof buf, "year: %d moons of %d days and %d over", L.months, L.monthDays, over); else snprintf(buf, sizeof buf, "year: %d moons of %d days", L.months, L.monthDays); break; }
        case 0: snprintf(buf, sizeof buf, "year: %.0f days   %s", L.yearDays, L.moons > 0 ? "the moon too quick to count" : "no moon to count by"); break;
        case 2: snprintf(buf, sizeof buf, "year: %d moons   the day is the year", L.months); break;
        default: snprintf(buf, sizeof buf, "the day is the year   no moon: years alone"); break;
    }
    return buf;
}

int shardKindCount() { return (int)kinds().size(); }
const char* shardKindName(int kind) { const auto& K = kinds(); return kind >= 0 && kind < (int)K.size() ? K[kind].name : "?"; }

int shardWords(const std::string& text) {
    int n = 0; bool in = false;
    for (char c : text) { if (c == ' ') in = false; else if (!in) { in = true; n++; } }
    return n;
}

// C-04: twelve to eighteen of a world's fifty are music, which ones by a shuffle of the fifty from the world's seed; C-10:
// the three to five after them in the same shuffle are star charts, so the music stayed where C-04 put it
namespace { void shardShuffle(const BodyGen& g, int* order) {
    for (int i = 0; i < SHARDS_PER_WORLD; i++) order[i] = i;
    Rng rng(mix64(g.seed ^ 0x3C04C1ULL));
    for (int i = SHARDS_PER_WORLD - 1; i > 0; i--) std::swap(order[i], order[rng.irange(i + 1)]);
} }
int musicShardsOf(const BodyGen& g) { return 12 + (int)(mix64(g.seed ^ 0x3C04C0ULL) % 7); }
bool shardIsMusic(const BodyGen& g, int index) {
    if (index < 0 || index >= SHARDS_PER_WORLD) return false;
    int order[SHARDS_PER_WORLD]; shardShuffle(g, order);
    int count = musicShardsOf(g);
    for (int i = 0; i < count; i++) if (order[i] == index) return true;
    return false;
}
int chartShardsOf(const BodyGen& g) { return 3 + (int)(mix64(g.seed ^ 0xC4A27ULL) % 3); }
int chartIndexOf(const BodyGen& g, int index) {
    if (index < 0 || index >= SHARDS_PER_WORLD) return -1;
    int order[SHARDS_PER_WORLD]; shardShuffle(g, order);
    int music = musicShardsOf(g), charts = chartShardsOf(g);
    for (int i = 0; i < charts; i++) if (order[music + i] == index) return i;
    return -1;
}
bool shardIsChart(const BodyGen& g, int index) { return chartIndexOf(g, index) >= 0; }
uint64_t chartSeedOf(const BodyGen& g, int index) { return mix64(g.seed ^ 0xC4A28ULL ^ ((uint64_t)(index + 1) * 0x9E3779B97F4A7C15ULL)); }

// C-12: the last recording: of the text shards (the music and the charts left out) the one whose age draw is the greatest
// (`shardOf`'s first draw of the shard's seed); `shardOf` sets its age to 1, so it is the latest of all fifty, and its tone the end
int lastShardOf(const BodyGen& g) {
    int order[SHARDS_PER_WORLD]; shardShuffle(g, order);
    bool skip[SHARDS_PER_WORLD] = {};
    int taken = musicShardsOf(g) + chartShardsOf(g);
    for (int i = 0; i < taken; i++) skip[order[i]] = true;
    int best = -1; double bestAge = -1;
    uint64_t base = mix64(g.seed ^ 0x5A4D5ULL);
    for (int i = 0; i < SHARDS_PER_WORLD; i++) {
        if (skip[i]) continue;
        Rng rng(base + (uint64_t)i * 0x9E3779B97F4A7C15ULL);
        double age = rng.uni();
        if (age > bestAge) { bestAge = age; best = i; }
    }
    return best;
}

Shard shardOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int index) {
    Shard s;
    BodyGen pg = g; pg.seed = loreSeed(g, L);   // C-13: the people's draws, whichever generator the caller holds
    s.seed = mix64(pg.seed ^ 0x5A4D5ULL) + (uint64_t)index * 0x9E3779B97F4A7C15ULL;
    Rng rng(s.seed);
    s.age = rng.uni();
    s.last = index == (L.last >= 0 ? L.last : lastShardOf(pg));   // C-12: the last recording: the latest of all, the end's tone
    if (s.last) s.age = 1.0;
    s.year = 1 + (int)std::lround(s.age * L.spanYears);
    {   // C-12: the date within the year, from the seed and not the stream (the texts stayed where C-02 left them)
        uint64_t h = mix64(s.seed ^ 0xCA1E0ULL);
        if (L.months > 0) s.month = 1 + (int)(h % (uint64_t)L.months);
        int days = L.calendar == 1 ? L.monthDays : (L.calendar == 0 ? std::max(1, (int)std::lround(L.yearDays)) : 0);
        if (days > 0) s.day = 1 + (int)((h >> 24) % (uint64_t)days);
    }
    Grammar G;
    buildGrammar(sys, b, g, L, s.age, rng, G);
    s.tone = pickTone(L.desert, s.age, rng);
    if (s.last) s.tone = ST_END;   // C-12
    s.music = shardIsMusic(pg, index);   // C-04: a piece of music (the tone stays, the piece's form follows it)
    if (s.music) return s;
    s.chart = shardIsChart(pg, index);   // C-10: a star chart: the caption names its figures and the star it marks, the tone colours the frame
    if (s.chart) {
        s.kind = -1;
        StarChart ch;
        if (chartOf(sys, b, pg, L, index, ch)) {
            std::vector<std::string> names; std::string inFig;
            for (const ChartFigure& f : ch.figures) { names.push_back(f.name); for (int k : f.stars) if (k == ch.markStar) inFig = ", in " + f.name; }
            G.set("figs", {names.empty() ? std::string("no figure we know") : chartFigureList(names)});
            G.set("other", {ch.mark.people.empty() ? std::string("others") : ch.mark.people});
            G.set("infig", {inFig});
            int named = 0; for (int j = 0; j < ch.which; j++) if (L.marks[j].how == 0) named++;   // the first star of no people is the one the lore names
            static const char* NAMED[4] = {"{star}, the brightest of our nights{infig}", "the star that rises before {festival}{infig}", "the star that stands over {mountain} at midwinter{infig}", "the star {founder} steered by{infig}"};
            G.set("mark", {ch.mark.how == 2 ? "the star the voices come from{infig}: the hearth of the {other}" : (ch.mark.how == 1 ? "the star of the {other}{infig}: they are there as we are here" : NAMED[std::min(named, 3)])});
            s.child = ch.mark.people;   // the other people's name: kept as it is by the decoder
            static const char* TPL[ST_TONE_COUNT] = {
                "The sky over {city} [at {festival}|on the longest night|in {season}]: {figs}. We marked {mark}.",
                "The stars {founder} knew over {city}: {figs}. [{kin} could name them all|The children learn them still]. We marked {mark}.",
                "[The sky as we drew it this year|What is still over {city} at night]: {figs}. We marked {mark}, so that it is not lost.",
                "The last sky we drew: {figs}. We marked {mark}, so that someone will know where we looked."};
            s.text = tidy(G.expand(TPL[s.tone], rng, 0));
        }
        return s;
    }
    const auto& K = kinds();
    std::vector<int> cand; std::vector<double> w;
    int world = L.desert ? 2 : 1;
    for (int k = 0; k < (int)K.size(); k++) if (K[k].tone == s.tone && (K[k].world == 0 || K[k].world == world) && (!L.desert || s.age <= K[k].until) && (!K[k].other || !L.other.empty())) { cand.push_back(k); w.push_back(K[k].w); }   // C-13: the "other" kinds where the world had two peoples
    if (cand.empty()) for (int k = 0; k < (int)K.size(); k++) if (K[k].tone == ST_ORDINARY) { cand.push_back(k); w.push_back(K[k].w); }
    s.kind = cand[rng.pick(w.data(), (int)w.size())];
    const Kind& kd = K[s.kind];
    s.text = tidy(G.expand(kd.tpl[rng.irange((int)kd.tpl.size())], rng, 0));
    auto ci = G.rules.find("child"); s.child = ci != G.rules.end() && !ci->second.empty() ? ci->second[0] : std::string();   // C-06: a name the decoder keeps
    return s;
}

void shardsOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int n, std::vector<Shard>& out) {
    out.clear();
    for (int i = 0; i < n; i++) out.push_back(shardOf(sys, b, g, L, i));
}

// ---------------------------------------------------------------------------
// C-03: the shards as objects in a settlement
// ---------------------------------------------------------------------------

const char* const SHARD_PLACE_NAMES[SHARD_PLACE_COUNT] = {"A HOUSE", "A HALL", "A TOWER", "A ROTUNDA", "THE FOOT OF A STELA"};

namespace {
// a point of the settlement lies within a piece's footprint widened by `margin` (the piece's own frame: across = (cos h, -sin h),
// along = (sin h, cos h), as `ruinElements` lays its boxes)
bool inPiece(double x, double z, const RuinElem& e, double margin) {
    double dx = x - e.x, dz = z - e.z;
    double across = dx * std::cos(e.heading) - dz * std::sin(e.heading), along = dx * std::sin(e.heading) + dz * std::cos(e.heading);
    return std::fabs(across) < e.hx + margin && std::fabs(along) < e.hz + margin;
}
// a building's frame to the settlement's metres
void toSettlement(const Building& b, double lx, double lz, double& x, double& z) {
    x = b.x + std::cos(b.heading) * lx + std::sin(b.heading) * lz;
    z = b.z - std::sin(b.heading) * lx + std::cos(b.heading) * lz;
}
}   // namespace

// A settlement's shards: the count by the class, then buildings drawn by a weight (a hall 2, a tower or a rotunda 1.5, a stela
// 1.2, a house 1) without repeating one, and for each a few candidate spots in its own frame tried in order: a room's far corners
// (from the door), the corners beside the door's wall, the back's middle; a hall's far end along the nave (the columns stand
// in two rows at half the width); a tower's far corners; a rotunda's side opposite its doorway; a stela's front, then its
// back. A spot must be 0.42 m clear of every piece of the building that stands on the ground (walls, the inner wall, the
// columns, the rubble; not the roof, the lintels or a dome) and 1.5 m from the doorway, so the explorer can stand at it;
// a building with no clear spot is skipped. The indices are distinct within the settlement
void shardSitesOf(const RuinSpec& r, const Culture& c, std::vector<ShardSite>& out) {
    out.clear();
    if (r.kind != RK_SETTLEMENT) return;
    Rng rng(r.seed ^ 0x5A4D53ULL);
    int want = 0;
    switch (r.sclass) {
        case SC_HAMLET: want = rng.chance(0.6) ? 1 : 0; break;
        case SC_VILLAGE: want = 1 + rng.irange(2); break;
        case SC_TOWN: want = 2 + rng.irange(3); break;
        default: want = rng.chance(0.7) ? 1 : 0; break;
    }
    if (want == 0) return;
    std::vector<RuinElem> el; ruinElements(r, c, 0, el);
    std::vector<int> cand; std::vector<double> wt;
    for (size_t i = 0; i < r.buildings.size(); i++) {
        int k = r.buildings[i].kind;
        double w = k == BK_HALL ? 2.0 : (k == BK_TOWER || k == BK_ROTUNDA ? 1.5 : (k == BK_HOUSE ? 1.0 : (k == BK_STELA ? 1.2 : 0)));   // a stela's foot is the one place in the open: a few, not none
        if (w > 0) { cand.push_back((int)i); wt.push_back(w); }
    }
    std::vector<int> used;
    while ((int)out.size() < want && !cand.empty()) {
        double total = 0; for (double w : wt) total += w;
        double u = rng.uni() * total; size_t pick = 0;
        for (size_t i = 0; i < cand.size(); i++) { u -= wt[i]; if (u <= 0) { pick = i; break; } pick = i; }
        int bi = cand[pick]; cand.erase(cand.begin() + pick); wt.erase(wt.begin() + pick);
        const Building& b = r.buildings[bi];
        double doorX = 0, doorZ = 0; bool hasDoor = buildingDoor(b, doorX, doorZ);
        struct Spot { double lx, lz; };
        std::vector<Spot> spots;
        int place = SHARD_IN_HOUSE;
        switch (b.kind) {
            case BK_HOUSE: case BK_TOWER: {
                place = b.kind == BK_HOUSE ? SHARD_IN_HOUSE : SHARD_IN_TOWER;
                // the door's frame: v toward the door's wall (+1 at it, -1 at the wall opposite), u along that wall; the far
                // corners first, then the corners beside the door's wall, then the back's middle
                const double uv[5][2] = {{-0.55, -0.55}, {0.55, -0.55}, {-0.55, 0.35}, {0.55, 0.35}, {0, -0.6}};
                for (int k = 0; k < 5; k++) {
                    double uu = uv[k][0], vv = uv[k][1], lx, lz;
                    switch (b.door) {
                        case 0: lx = uu * b.hw; lz = vv * b.hd; break;
                        case 2: lx = -uu * b.hw; lz = -vv * b.hd; break;
                        case 1: lx = vv * b.hw; lz = uu * b.hd; break;
                        default: lx = -vv * b.hw; lz = uu * b.hd; break;
                    }
                    spots.push_back({lx, lz});
                }
                break;
            }
            case BK_HALL: place = SHARD_IN_HALL; spots = {{0, -b.hd + 1.3}, {0, -b.hd * 0.3}, {0, 0.2}}; break;
            case BK_ROTUNDA: {
                place = SHARD_IN_ROTUNDA;
                double ax = doorX - b.x, az = doorZ - b.z, L = std::sqrt(ax * ax + az * az);
                if (L < 1e-6) { ax = 0; az = 1; L = 1; }
                // in the building's frame (its heading): the side opposite the door at 0.55 of the radius, then at 0.3
                double ox = -ax / L * b.hw * 0.55, oz = -az / L * b.hw * 0.55;   // settlement metres from the centre
                double lx = std::cos(b.heading) * ox - std::sin(b.heading) * oz, lz = std::sin(b.heading) * ox + std::cos(b.heading) * oz;
                spots = {{lx, lz}, {lx * 0.55, lz * 0.55}};
                break;
            }
            case BK_STELA: place = SHARD_AT_STELA; spots = {{0, 0.8}, {0, -0.8}}; break;
            default: break;
        }
        for (const Spot& sp : spots) {
            double x, z; toSettlement(b, sp.lx, sp.lz, x, z);
            if (hasDoor && std::hypot(x - doorX, z - doorZ) < 1.5) continue;
            bool clear = true;
            for (const RuinElem& e : el) {
                if (e.building != bi || e.shape != 0 || e.y0 > 1.0) continue;
                if (inPiece(x, z, e, 0.42)) { clear = false; break; }
            }
            if (!clear) continue;
            ShardSite s; s.building = bi; s.place = place; s.x = x; s.z = z; s.heading = b.heading + rng.sym(0.5);
            for (int tries = 0; tries < 50; tries++) {
                s.index = shardWorldIndex(r.people, rng.irange(SHARDS_PER_WORLD));   // C-13: from the settlement's people's fifty
                bool dup = false; for (const ShardSite& o : out) if (o.index == s.index) dup = true;
                if (!dup) break;
            }
            out.push_back(s);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// C-06: the decoding
// ---------------------------------------------------------------------------

namespace {
// the pools a people's tongue draws from, by the style of its names (`generateName`'s plain, hard and flowing)
const std::vector<std::string> T_ON[3] = {
    {"b", "d", "g", "k", "l", "m", "n", "p", "r", "s", "t", "v", "w", "y", "z", "h", "f", "sh", "th", "kl", "tr", "br", "st", "dr", "gr", "pl", "sk", "sl", "fr", "ch"},
    {"k", "g", "kr", "gr", "th", "z", "x", "sk", "d", "dr", "t", "tr", "kh", "gh", "br", "vr", "zh", "q", "kt", "rh", "ts", "gz", "st", "n", "r"},
    {"l", "ly", "m", "n", "s", "sh", "v", "f", "y", "r", "h", "w", "th", "ph", "sy", "ny", "ml", "vl", "hl", "sw", "fy", "lh", "el", "al"}};
const std::vector<std::string> T_NU[3] = {
    {"a", "e", "i", "o", "u", "ai", "ei", "ou", "ia", "ea", "oo", "au", "ie", "eo"},
    {"a", "o", "u", "ei", "aa", "au", "oa", "ui", "uu", "ao"},
    {"ia", "ea", "ai", "io", "ya", "e", "i", "a", "ie", "ei", "ee", "ue", "ae", "ii"}};
const std::vector<std::string> T_CO[3] = {
    {"n", "l", "r", "s", "m", "t", "k", "nd", "st", "sh", "th", "rn", "lt", "ng", "d", "ks"},
    {"k", "r", "n", "x", "th", "rk", "rt", "g", "d", "kh", "zd", "ng", "kt", "rg"},
    {"l", "n", "s", "la", "ne", "ri", "sh", "th", "m", "ya", "li", "ll", "ss", "nn"}};
uint64_t strHash(const std::string& s) { uint64_t h = 1469598103934665603ULL; for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; } return h; }
std::string lowerOf(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }
// a token of a text: the punctuation before, the word (letters, digits, hyphens, an inner apostrophe), the rest after; a possessive 's counts as after
void splitToken(const std::string& t, std::string& pre, std::string& core, std::string& post) {
    size_t i = 0, n = t.size();
    while (i < n && !std::isalnum((unsigned char)t[i])) i++;
    pre = t.substr(0, i);
    size_t j = i;
    while (j < n && (std::isalnum((unsigned char)t[j]) || t[j] == '-' || t[j] == '\'')) j++;
    core = t.substr(i, j - i); post = t.substr(j);
    if (core.size() > 2 && core[core.size() - 2] == '\'' && (core.back() == 's' || core.back() == 'S')) { post = core.substr(core.size() - 2) + post; core.erase(core.size() - 2); }
    while (!core.empty() && (core.back() == '\'' || core.back() == '-')) { post = core.back() + post; core.pop_back(); }
}
bool allDigits(const std::string& s) { if (s.empty()) return false; for (unsigned char c : s) if (!std::isdigit(c)) return false; return true; }
}   // namespace

Tongue tongueOf(const BodyGen& g, const Lore& L) {
    Tongue T;
    T.seed = mix64(loreSeed(g, L) ^ 0x70A6E5ULL);   // C-13: the people's
    Rng rng(T.seed);
    int st = L.style < 0 || L.style > 2 ? 0 : L.style;
    auto draw = [&](const std::vector<std::string>& pool, int n, std::vector<std::string>& out) {
        std::vector<std::string> p = pool;
        for (int i = 0; i < n && !p.empty(); i++) { int k = rng.irange((int)p.size()); out.push_back(p[k]); p.erase(p.begin() + k); }
    };
    draw(T_ON[st], 8 + rng.irange(5), T.onsets);
    draw(T_NU[st], 4 + rng.irange(3), T.nuclei);
    draw(T_CO[st], 3 + rng.irange(4), T.codas);
    T.codaChance = 0.3 + 0.4 * rng.uni();
    T.vowelStart = 0.05 + 0.3 * rng.uni();
    return T;
}

// one syllable for a word of three letters or fewer, two to six, three past that: the people's words run about as long as ours
std::string tongueWord(const Tongue& T, const std::string& word, int attempt) {
    Rng rng(mix64(T.seed ^ strHash(word)) + (uint64_t)attempt * 0x9E3779B97F4A7C15ULL);
    int len = (int)word.size();
    int syl = len <= 3 ? 1 : (len <= 6 ? 2 : (rng.chance(0.3) ? 2 : 3));
    if (attempt > 0 && rng.chance(0.5)) syl = std::min(3, syl + 1);
    std::string out;
    for (int s = 0; s < syl; s++) {
        if (s > 0 || !rng.chance(T.vowelStart)) out += T.onsets[rng.irange((int)T.onsets.size())];
        out += T.nuclei[rng.irange((int)T.nuclei.size())];
        if (rng.chance(s == syl - 1 ? T.codaChance : T.codaChance * 0.4)) out += T.codas[rng.irange((int)T.codas.size())];
    }
    if (out.size() < 2) out += T.codas[rng.irange((int)T.codas.size())];   // never a bare vowel alone: one letter reads as noise in capitals
    return out;
}

// the words of the fifty by how often they are used (ties by a hash of the word, so no alphabet shows through), each with
// its word in the people's tongue, drawn again when it would repeat one already given
void languageOf(const Lore& L, const std::vector<Shard>& fifty, const Tongue& T, Language& out) {
    out = Language();
    for (const std::string& nm : {L.people, L.god, L.river, L.mountain, L.sea, L.city, L.city2, L.founder, L.moon, L.star, L.festival}) out.names.insert(lowerOf(nm));
    if (!L.other.empty()) out.names.insert(lowerOf(L.other));   // C-13: the other people's name, kept as it is
    for (const Shard& s : fifty) if (!s.child.empty()) out.names.insert(lowerOf(s.child));
    std::map<std::string, int> count;
    for (const Shard& s : fifty) {
        size_t p = 0;
        while (p < s.text.size()) {
            size_t q = s.text.find(' ', p); if (q == std::string::npos) q = s.text.size();
            if (q > p) {
                std::string pre, core, post; splitToken(s.text.substr(p, q - p), pre, core, post);
                std::string key = lowerOf(core);
                if (!key.empty() && !allDigits(key) && !out.names.count(key)) count[key]++;
            }
            p = q + 1;
        }
    }
    std::vector<std::pair<std::string, int>> v(count.begin(), count.end());
    std::stable_sort(v.begin(), v.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) { return a.second != b.second ? a.second > b.second : mix64(strHash(a.first)) < mix64(strHash(b.first)); });
    std::set<std::string> given;
    for (const auto& kv : v) {
        out.index[kv.first] = (int)out.words.size(); out.words.push_back(kv.first); out.counts.push_back(kv.second); out.tokens += kv.second;
        std::string w;
        for (int attempt = 0; attempt < 8; attempt++) { w = tongueWord(T, kv.first, attempt); if (!given.count(w)) break; }
        given.insert(w); out.theirs.push_back(w);
    }
}

double languageShare(int shardsHeld) {
    if (shardsHeld <= 0) return 0;
    if (shardsHeld >= 10) return 1;
    return 0.3 + 0.7 * (shardsHeld - 1) / 9.0;
}

// the most used words down to where their uses reach the share of the fifty's words (the first word past it counts)
int languageKnown(const Language& L, int shardsHeld) {
    double share = languageShare(shardsHeld);
    if (share >= 1) return (int)L.words.size();
    double target = share * L.tokens; int cum = 0, k = 0;
    while (k < (int)L.words.size() && cum < target) { cum += L.counts[k]; k++; }
    return k;
}

int decodeShard(const Shard& s, const Language& L, const Tongue& T, int shardsHeld, std::vector<DecodedWord>& out) {
    out.clear();
    int known = languageKnown(L, shardsHeld), read = 0;
    size_t p = 0;
    while (p < s.text.size()) {
        size_t q = s.text.find(' ', p); if (q == std::string::npos) q = s.text.size();
        if (q > p) {
            DecodedWord w;
            splitToken(s.text.substr(p, q - p), w.pre, w.ours, w.post);
            std::string key = lowerOf(w.ours);
            if (w.ours.empty()) { w.known = true; w.theirs = w.ours; }
            else if (L.names.count(key)) { w.name = true; w.known = true; w.theirs = w.ours; }
            else if (allDigits(key)) {   // a numeral is read from the start; spoken digit by digit in the people's words for them (C-05)
                static const char* DIGITS[10] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
                w.known = true; w.theirs.clear();
                for (char c : key) { if (!w.theirs.empty()) w.theirs += '-'; w.theirs += tongueWord(T, DIGITS[c - '0']); }
            }
            else {
                auto it = L.index.find(key);
                w.known = it != L.index.end() && it->second < known;
                w.theirs = it != L.index.end() ? L.theirs[it->second] : tongueWord(T, key);
                if (std::isupper((unsigned char)w.ours[0]) && !w.theirs.empty()) w.theirs[0] = (char)std::toupper((unsigned char)w.theirs[0]);
            }
            if (w.known) read++;
            out.push_back(w);
        }
        p = q + 1;
    }
    return read;
}

std::string decodedText(const std::vector<DecodedWord>& w) {
    std::string out;
    for (const DecodedWord& d : w) { if (!out.empty()) out += ' '; out += d.pre + (d.known ? d.ours : d.theirs) + d.post; }
    return out;
}
