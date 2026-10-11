# Research: grammars and histories for the records (H-01)

Written 2026-10-08 for H-01 of `ITERATION-1.3.0.md`, on the user's request in R-411 ("investigation on free-context grammar
is required ... This context-free grammar stuff would also need investigation and research on your side"). It answers the
seven questions of the ideas note's section 11 (`IDEAS-civilisation-expansion.md`), records the formalism and the authoring
format chosen and why, and fixes the shape of the data the peoples' milestones lay down for the records to read (A-01's
profile, H-02's history, T-04's records). A scratch prototype tried the candidates on C-02's world inputs for the two test
worlds; its fifty records sit beside C-02's texts in `RESEARCH-records-samples.md`. The prototype is out of the tree, as
CLAUDE.md's rule for experiments asks; what it showed is here.

## The answer in short

* **The depth comes from a history, told by a grammar.** A people gets a simulated history (H-02): places, rulers,
  families followed year by year, public and private events with their causes, the collapse in stages. A record is planned
  from it: a genre's schema says what parts a letter, a chronicle or an epitaph has, and salience picks which events fill
  them. The grammar writes those parts in the people's voice. This is how Dwarf Fortress and Caves of Qud get their depth,
  and Ryan's work on emergent narrative says the same: simulate first, then curate.
* **The formalism is a small attribute grammar over a realiser.** It has weighted alternatives, guards on the inputs, and
  entities bound for the whole record (the writer, the reader, the event and its people). Attributes pass down the tree
  (the tense, the record's facts) and up it (a part chosen early tells the later parts what it said: Knuth's synthesized
  attributes). A record does not use the same alternative twice while others are left. The grammar never writes "he",
  "his", "a" or "an", verb endings or number words itself. It writes entity and verb references, and the realiser does the
  English: the first mention in full, kinship seen from the writer, pronouns only where clear, verbs by tense and
  subject, numbers with their nouns, articles and punctuation. So C-02's "him mother" and "a lizards" cannot happen.
* **Frequent parts are composed from smaller pieces; strong moments are whole sentences.** Measured on 100 chronicles: a
  chronicle entry built from three pieces (time, main clause, consequence) cut the share of repeated sentence frames from
  77% to 30%, with 1.7 times the authored lines. Spreading choices by weight alone ("dryness") barely moved the numbers. A
  small pool of choices repeats however they are drawn.
* **The authoring format is plain text grammar files** (`.vg`, the format below). They are compiled into the binary by
  wrapping each file in a C++ raw string and `#include`-ing it, so neither the Makefile nor CMakeLists.txt changes and both
  track the dependency. The harness reads the same files from disk, so a rule can be edited and its output reprinted
  without a rebuild.
* **Cost:** a record's text takes about 28 microseconds and a world's history 1.5 to 3 ms. A people's whole catalogue
  (80 to 200 records) is about 10 ms, built once on a thread with the history and cached.
* **Variety, measured** (sentences of six words or more whose frame, with names and numbers masked, recurs in the
  sample; the genre's formulas of greeting, year heading and blessing left out; C-02's texts have none):

  | records a world | prototype, Aieliaalas II | prototype, Leileashphail III | C-02, Aieliaalas II | C-02, Leileashphail III |
  | --- | --- | --- | --- | --- |
  | 50 | 12% | 12% | 20% (34 texts) | 11% (35 texts) |
  | 100 | 21% | 22% | 38% | 34% |
  | 200 | 33% | 35% | 51% | 50% |

  The prototype's records are half as long again (about 47 words without their frame against C-02's 31). They carry the
  history, and C-02's carry none.

## 1. What C-02 does, and where it stops

C-02's story grammar (`galaxy/shards.*`, `12-civilisations.md`) is a weighted grammar of whole texts. It has thirty-one
kinds and a hundred templates, a single text each of about 31 words, with `{slot}` and `[a|b]` marks over some twenty slots
read from the world, and `tidy` closes the seams. It is a Tracery-like grammar written in C++ tables. Its voice is its
strength: the templates are small stories with a turn at the end ("He was right for twenty years and wrong this
morning"), concrete and dry. Every new genre should keep that register.

What it cannot do, from its own print on the two test worlds (`vesperis_test shards 151 25 1 50 10` and `153 70 2 50 10`;
the full lists are in the samples file):

* **The same template twice on one world.**
  * Aieliaalas II: "A woman from Xostoor stayed the winter and taught … to mend a net" (years 5 and 43); "A stranger came
    over Zeyzox from … days beyond Xostoor" (293 and 308); "Sing spring, sing Cretsax full" (29 and 162); "The young ones
    went to Xostoor after Aiwya" (51 and 225); "Year … of Aiyin. Five jars of barley …" three times.
  * Leileashphail III: "A woman from Thamwe stayed the winter" (35 and 124); "… if you hear this, do not come back for the
    jars" (246 and 287); "The … along Yiokul are dying one by one" (280 and 388).
  * Each world's fifty is drawn from the same hundred templates, and two hundred texts repeat half their sentence frames
    (the table above).
* **No memory.**
  * A slot is drawn again in every text. One world grows kale, flax, barley and rye; "The barley is cut … Tonight the rye
    beer".
  * The child's name, the relative and their pronoun are new in every text, so no person recurs and a "father" can die
    in two texts.
  * Nets come up "below the well" on a desert.
* **No history.** The founder and two towns are all a people has: no ruler, no war, no sickness with a date, no building,
  and no cause for anything. The dead desert's decline is told by tone (warnings grow late in the span), but nothing in
  one text knows what another said.
* **One length.** Every text is 19 to 50 words.
* **Agreement fixed by hand.** C-02's first print had "him mother", "the the well" and "a lizards". They were fixed in the
  templates one at a time, not prevented: `{he}`/`{him}`/`{his}` are slots, `{skyone}` exists only so that a singular verb
  can follow it.

## 2. The sources

Each: what it does, what it would give Vesperis, what it costs.

**Tracery** (Kate Compton, Ben Kybartas, Michael Mateas, "Tracery: An Author-Focused Generative Text Tool", ICIDS 2015).

* *What it does.* A context-free grammar in JSON: a symbol maps to a list of expansions, `#symbol#` expands one, and
  modifiers mend the surface (`.capitalize`, `.s`, `.a`, `.ed`). Actions push a choice onto a symbol for the rest of the
  expansion (`[hero:#name#]`, then `#hero#` reads it back), so a story can keep its hero. Weights are made by repeating
  an entry.
* *What it gives.* The authoring ease the expansion needs: plain text, no code, an author reads it as prose.
* *What it costs.* No conditions on a world model, no agreement beyond the modifiers, and no memory across texts. C-02 is
  this design, and section 1 is what it costs at a few hundred texts a world.

**Kate Compton, "So you want to build a generator…"** (2016, an essay on her blog).

* *What it does.* It starts from the artifact and the qualities wanted, not the method. Its "10,000 bowls of oatmeal" names
  the trap: content that is unique in its numbers and the same to the eye.
* *What it gives.* The measure in section 7: what the reader perceives as repeated (a sentence's frame), not whether two
  strings differ.
* *What it costs.* Nothing; it is a way of judging.

**Caves of Qud's sultans** (Jason Grinblat and C. Brian Bucklew, "Subverting Historical Cause & Effect: Generation of
Mythic Biographies in Caves of Qud", FDG 2017; Grinblat's GDC 2018 talk "Procedurally Generating History in Caves of Qud").

* *What it does.* Five sultans per game (a sixth is fixed) each get about a dozen events: an origin, eight drawn from
  seventeen kinds, an ascension, a death. Each event changes the sultan's state (themes such as glass or stars, a
  significant colour, a profession, cognomens, the factions that love or hate them), and later events and names read that
  state. The game does not simulate causes. It generates an event, then writes a reason for it from the state, after the
  fact. Every event is told twice, as a gospel in the journal and as a tomb mural in a more worshipful tone. The
  vocabulary changes with the era (the early sultanates' "star, cosmic, reactor", the late ones' "sand, salt, hearth").
* *What it gives.*
  * A history can be cheap and still read as deep, as long as state accumulates and the telling reads it.
  * One event can be told in two registers (the plan's "the annal, the letter and the song of one event").
  * The era can be a vocabulary axis, which is our age.
* *What it costs.* Rationalising after the fact suits myth. Vesperis's records are documents of ordinary life (the ideas
  note's register), where a cause the reader can check (a famine after two bad harvests) is worth more. H-02 should
  simulate the few causes that matter and rationalise the rest.

**Dwarf Fortress's histories and legends** (Tarn Adams; Tanya X. Short and Tarn Adams, eds., *Procedural Generation in
Game Design*, 2017, and *Procedural Storytelling in Game Design*, 2019, with Adams's chapter "Emergent Narrative in Dwarf
Fortress").

* *What it does.* World generation runs a history (civilisations, sites, historical figures with relationships, artifacts
  and events) before play. Legends mode prints its events as plain dated sentences, grouped into collections such as wars
  and battles. Engravings and artifacts in play depict historical events, which Adams describes as one of the strongest
  prompts for players' own stories.
* *What it gives.* The order the plan already took: the history before the sites (T-01 and H-02 before T-02 to T-04), and
  objects that depict events (our shards and charts). The depth is in the volume and the cross-references, not in the
  sentences, which are templates.
* *What it costs.* A full simulation with thousands of figures is far beyond a record's needs. The prototype's stub, with
  ten families, rulers and public events, is about 2 ms a world.

**Expressionist** (James Ryan, Ethan Seither, Michael Mateas, Noah Wardrip-Fruin, "Expressionist: An Authoring Tool for
In-Game Text Generation", ICIDS 2016) and **Ryan's *Curating Simulated Storyworlds*** (PhD dissertation, UC Santa Cruz,
2018).

* *What it does.*
  * Expressionist is a context-free grammar whose symbols carry the author's markup (tags), so every generated text
    arrives with the markup of all the symbols that made it; a game can then ask for content that expresses given tags.
  * The dissertation argues that emergent narrative needs curation: a simulation produces many events, and stories
    exist only where someone, human or program ("story sifting"), picks and frames them.
* *What it gives.*
  * Markup turns a grammar into something a game can query, which suits H-05's categories, the radar's programmes and
    the decoder's history page.
  * Sifting is what the record's plan does when salience picks the events a letter tells.
* *What it costs.* A separate tool and a runtime search over the grammar. The prototype's guards and directives do a
  narrower version (the plan picks the event, the grammar reads its kind) with nothing to search.

**Ehud Reiter and Robert Dale, *Building Natural Language Generation Systems*** (Cambridge University Press, 2000).

* *What it does.* The standard pipeline:
  * document planning: content determination and document structure;
  * microplanning: lexical choice, referring expressions, aggregation;
  * surface realisation.

  Dale and Reiter's incremental algorithm for referring expressions is the source of "the first mention in full, then the
  shortest form that is still clear".
* *What it gives.* The prototype's layers are this pipeline in small: the history, the record's plan, the grammar, the
  realiser. Its realiser's referring rule is the incremental algorithm cut down to names, kinship, titles and pronouns.
* *What it costs.* Nothing to adopt. A full NLG system is far more than the records need.

**SimpleNLG** (Albert Gatt and Ehud Reiter, "SimpleNLG: A Realisation Engine for Practical Applications", ENLG 2009).

* *What it does.* A realiser library (Java): give it a clause's parts and features (subject, verb, object, tense, number,
  negation, passive) and it writes the inflection, the word order, the agreement and the punctuation.
* *What it gives.* The list of what a realiser must own so that the grammar never does: the prototype's realiser is a
  C++ subset (verbs by tense and person, about a hundred irregular verbs, plurals with irregulars, articles, numbers,
  pronouns by case, possessives, capitals and stops).
* *What it costs.* The library itself is Java and far larger than needed. We take its division of labour, not its code.

**Attribute grammars** (Donald E. Knuth, "Semantics of Context-Free Languages", *Mathematical Systems Theory* 2(2), 1968).

* *What it does.* Each grammar symbol carries attributes, with rules per production. Inherited attributes flow down from
  the parent (the context); synthesized attributes flow up from the children (what was built).
* *What it gives.* The vocabulary for exactly the two things a plain context-free grammar lacks:
  * the tense and the bound people of a record, inherited down;
  * what an early part chose, synthesized up (the prototype's `{=.x}`). A letter's "work is slow" then leads to "so I
    sit by the door", never to "so I am up before the light".
* *What it costs.* In full generality, evaluating attributes needs dependency analysis. Vesperis needs only top-down
  inheritance and one-level-up synthesis in reading order, which the expander does as it goes.

**Grammatical Framework** (Aarne Ranta, *Grammatical Framework: Programming with Multilingual Grammars*, CSLI, 2011).

* *What it does.* An abstract syntax (trees of meaning) with one concrete syntax per language. Each concrete syntax writes
  the trees out with its own parameters (number, gender, case) and word order, and a resource library covers dozens of
  languages.
* *What it gives.* The shape of the language layer, question 7. A record is meaning first (the realiser's tokens: an
  entity, a verb lemma, a number), so one record could be written out in English (the decoded text) and in the people's
  tongue, which has its own word order and endings.
* *What it costs.* GF itself (a Haskell toolchain and its runtime) is too heavy to embed. We take the idea, not the tool.

**Improv** (Bruno Dias, a JavaScript library, 2016–2017, written for his game *Voyageur*, 2017).

* *What it does.* Snippets in groups carry tags, and a world model holds the facts as tags. Filters score each group's
  salience against the model:
  * a mismatch excludes the group;
  * a partial or full match adds a bonus;
  * dryness drops phrases already used.

  With reincorporation on, the tags of what was said are added to the model, so later text agrees with earlier choices.
  *Voyageur* used it for paragraph descriptions of planets from their ecology, ideology and economy.
* *What it gives.* Three of the prototype's mechanisms come from here:
  * guards on a model;
  * dryness, both within a record and across a world's catalogue;
  * reincorporation, which is the synthesized attribute again.

  Its descriptions of planets are close to our worlds' records in size and purpose.
* *What it costs.* Nothing beyond the prototype's own version.

**Emily Short on procedural text and salience** ("Beyond Branching: Quality-Based, Salience-Based, and Waypoint
Narrative Structures", 2016; "Bowls of Oatmeal and Text Generation", 2016; *The Annals of the Parrigues*, 2015).

* *What it does.* In salience-based narrative, the content whose conditions best match the current state is chosen. It
  is the model of Valve's dynamic dialogue (Elan Ruskin, "AI-driven Dynamic Dialog through Fuzzy Pattern Matching", GDC
  2012), where the most specific matching rule wins. Her oatmeal essay argues that generated text matters when it shows a
  state the player can perceive and has changed, as small layered consequences, and that procedural generation is a way
  of designing content as laborious as any other.
* *What it gives.* The record's plan is salience by construction. The shards reflect what the explorer can find (the
  ruins, the graves, the places on the plan), which is the "mechanical connection" she asks for.
* *What it costs.* Authoring volume, which the measures confirm.

**Mark R. Johnson, Ultima Ratio Regum** ("Procedural Generation of Linguistics, Dialects, Naming Conventions and Spoken
Sentences", DiGRA–FDG 2016).

* *What it does.* What he calls "qualitative procedural generation": cultures with norms, beliefs and practices are
  generated first. Each culture's speech then follows from them: a set of references drawn from its geography, climate,
  ideology and religion, its own greetings, insults, farewells and compliments, and the complexity of its sentences.
* *What it gives.* The people's own turns of phrase as an input: the prototype's `simile` ("as patient as Cropheyng",
  "as dry as the quay") and the world's sky in the blessings. A-04's genome and H-02's institutions should feed the
  grammar's register the same way: a pious people swears by its god, a mercantile one counts.
* *What it costs.* Each such input is a few lines per rule. The cost is in deciding which ones matter.

**Story grammars** (David E. Rumelhart, "Notes on a Schema for Stories", in Bobrow and Collins, eds., *Representation and
Understanding*, 1975; John B. Black and Robert Wilensky, "An Evaluation of Story Grammars", *Cognitive Science* 3(3),
1979) and **Propp's *Morphology of the Folktale*** (1928).

* *What it does.* A story as a grammar of parts (a setting and episodes; Propp's thirty-one functions in a fixed order
  over seven roles). Black and Wilensky's objection is that such a grammar describes the form and not what makes it a
  story.
* *What it gives.* Genre schemas: a letter is a greeting, news, feeling, a request and a farewell; an epitaph is a name, a
  life, a death and who set the stone. These are the plan's top level. The objection is answered by filling the schema
  from a history, not by the schema itself.
* *What it costs.* Nothing.

**Set aside.**

* *Markov chains*: no structure and no control (the ideas note).
* *Neural language models*: models of hundreds of megabytes to gigabytes, slow on a CPU for a catalogue of thousands of
  texts, not bit-for-bit reproducible across machines, and unable to promise that a text matches its history. The game's
  texts are its authored voice.

## 3. The seven questions

**1. Plain weighted grammar, attribute grammar, or grammar over a realiser?** A small attribute grammar over a realiser,
with the content chosen outside the grammar.

* A plain weighted grammar is C-02: it cannot keep a crop, a person or a pronoun. Its agreement is hand-fixed.
* An attribute grammar alone could carry agreement as features, but every rule would then thread number and person by
  hand.
* With a realiser, the grammar names *who* and *which verb*, and the realiser writes the English. The attributes left to
  the grammar are the ones that are about content: the tense, the bound entities and the facts passed down, and a part's
  choice passed up.
* Content selection (which events) is a directive. The grammar asks the plan for "the most salient untold private event
  near the date" and the plan binds it. It is not a production rule, because it reads the history, not the text.

**2. Where does the depth come from?** The history first, the grammar second, as the ideas note supposed and the
prototype confirms.

* What the reader remembers from the samples is the history: Queen Felugocea dying in the tenth year of her reign,
  Queen Seystault taking the levy against Ryanu over the border at Cretsax, a father's letter about his daughter dead of
  the sickness the chronicle records that spring, a stone for a woman who "stood at the field below Zeyzox".
* None of C-02's texts can say any of that.
* The grammar gives the voice and the variety of the telling. The same plague is "the sickness that was in Aiyin" in a
  letter, "sickness came to Aiyin from the hill farms; some three hundred dead before summer" in the chronicle, and "She
  died in the sickness of the year 42" on a stone.

**3. How does a long text hold together, and what picks the events?** By genre schemas, salience and two kinds of
memory.

* **The schema is the grammar's top rules.**
  * A letter: an opening; one to three news items as paragraphs; a turn to the reader (a question, a feeling, a
    request); a closing.
  * A chronicle: two to four years that have something in them; a run of empty years between them told as one line or
    passed over; a year's events chosen by salience and then told in the order of their seasons, a war's declaration
    before its battle.
  * An epitaph: a name line, a deed or the trade, the death by its cause, who set the stone.
  * H-04's other genres are more schemas of the same kind.
* **Salience** is the event's weight times its nearness in time times its place times the reader's stake, with novelty on
  top:
  * nearness in time: a letter tells this year's and last year's, a rumour of a war or a sickness coming, older things
    only weakly;
  * the place: the writer's town counts 1.6 times;
  * the stake: the writer's or the reader's family three times;
  * novelty: an event already told in this record is out, one told by another record of the world counts half, and a
    second event of a kind already told counts a quarter.
* **Two memories.**
  * Within a record, entities, attributes and what was told keep the text consistent, and no alternative is used twice
    while another is left.
  * Across a world's records, a catalogue keeps what was told and which alternatives were used. So a people's records
    are built as one catalogue (deterministic, about 10 ms) and a shard's text is looked up from it.

**4. The authoring format.** Text files, embedded at build time without a build step and reloaded by the harness:
section 5.

**5. Determinism and cost.** Everything is drawn from seeded streams: a world's history from its people's seed, a record
from the catalogue's seed and its index. The same world gives the same catalogue on every machine (seeded `Rng`
draws and plain double arithmetic, no threads inside a catalogue, as everywhere in the game). Measured on the development
Mac, one thread, `-O2`:

| | Aieliaalas II | Leileashphail III |
| --- | --- | --- |
| the grammar read and parsed (579 alternatives) | 0.7 ms | (once) |
| a world's history (884 / 1,709 entities; 1,951 / 3,763 events) | 1.5–2.7 ms | 2.5–2.9 ms |
| a record's text (3,000 records) | 28.4 µs | 28.3 µs |

A catalogue of 200 records is about 5 ms on top of its history. The plan's budget was "a text in well under a millisecond,
a world's catalogue on a thread", which is met with room for H-04's longer genres.

**6. Evaluation.** Automatic checks, variety measures and the reading: section 7.

**7. The language layer.**

* **The tongue reads lemmas, not word forms.**
  * C-06's decoding maps each of our words to one of theirs, so "walk", "walks" and "walked" are three words of theirs.
    A record of 400 words would have a vocabulary that grows with every inflection.
  * The realiser knows each word's lemma and features, so the tongue should take one word per lemma and its own few
    endings, drawn from the people's seed (a plural, a past, a possessive).
  * The decoder then shows "Kriweir-ak" before "of Kriweir" is learnt, and the language share (C-06's 30% at one shard,
    all at ten) is counted over lemmas.
  * H-04 re-tunes it: H-05's catalogue is 80 to 200 records, and the share at ten shards must still feel like learning.
* **The tongue can have its own word order** (GF's lesson): the abstract record is what both languages write out. That
  is H-04's choice; the decoder's word-by-word display assumes English order today.
* **The voice** (C-05) reads a record's opening paragraph and the rest on request. At C-05's 3–5.6 syllables a second, a
  record of 300 words is over two minutes.
* **The radar's programmes** (C-07, C-16) take whole sentences from the records of a radio age: a news item, a
  weather report, a name list, a last message. The catalogue's tags say which records are broadcasts.

## 4. The formalism

A **record grammar** is a set of rules. A rule is a name and its alternatives; an alternative is a weight, a set of
guards, flags, and a text. Expanding a rule:

1. **Guards** filter the alternatives. A guard compares a key with a value (`=`, `!=`, `>=`, `<=`, `>`, `<`), or tests
   that a key is set or unset (`has:`, `!has:`). Keys are:
   * the record's facts (`era`, `stage`, `temper`, `rel`, …);
   * the world's facts (`desert`, `moons`, `comet`, …);
   * fields of bound entities and events (`$e.kind`, `$e.when`, `$e.a.age`, `$x.spouse.pron`, …);
   * attributes passed up (`.slow`).

   Ages compare in order (`era>=industrial`).
2. **Dryness.**
   * `once` excludes an alternative already used in this record.
   * Without `once`, alternatives this record has not used come first while there are any.
   * Across the catalogue, an alternative's weight is divided by 1 + 1.5 × its uses so far. The measures say this
     matters little; it is kept because it costs nothing.
3. **The draw** is weighted, from the record's stream.
4. **The text** is expanded in reading order, with the parent's bindings (inherited) and the alternative's own
   `tense=past` or `tense=present` (inherited by everything under it). Its marks:
   * `{rule}` expands another rule, a world word list (`{crop}`) or a fact (`{festival}`); `{key:pl}` is a word's plural.
   * `[a|b|c]` picks one inline.
   * `{$x}` refers to an entity. Its forms are `:obj`, `:poss`, `:name`, `:full` and `:given`; field paths such as
     `{$e.a}`, `{$e.place}` and `{$x.spouse}` reach the entities an event or a person holds, and `{$e.cause}` is an event
     told as a noun phrase by `np.<kind>`.
   * `{$e.thing}` is an event's own text, expanded with that event bound; scalar fields such as `{$e.season}` and
     `{$x.trade}` give words.
   * `{lemma:$x}` is a verb in the inherited tense, agreeing with its subject (also `{lemma:I}`, `{lemma:they}`, …).
   * `{#key}` is a number: a word to twelve, then digits. Its forms are `:ord` for an ordinal, `:about` for a round
     number ("some three hundred") and `:digits`; `{#key|noun}` makes the noun agree ("one year", "two years").
   * `{=.key:value}` sets a synthesized attribute. It is visible to the rest of the alternative and passes up to the
     rule that called it, so the parent's later parts see it. The name starts with a dot so that it cannot shadow a
     fact: the prototype's first `{=comet}` collided with the world fact `comet` and told a letter "they have said so
     for years" about the moon.
   * `{@directive}` is the record's plan: `@news private|public|any`, `@years`, `@entries`, `@death`, `@deed`. These are
     the C++ functions that pick events by salience and bind them before expanding `<genre>.<kind>`.
   * `\n` is a paragraph.
5. **The realiser** turns the token stream into text. It does the following:
   * The first mention of a person is in full. That means kinship from the writer ("my daughter Piazuwiol"), or the
     reader's close kinship where the writer's is distant ("your brother Mauklo", not "my nephew"); else a title
     ("Queen Felugocea", "Chancellor Taibi Owolt"); else name and trade ("Aurai the weaver").
   * Later mentions are a pronoun when the entity was the last one of its gender and is in this or the last sentence;
     else a short form ("the queen", the name).
   * Paragraphs reset pronouns, except an epitaph's lines.
   * The writer is I and the reader you, in a letter.
   * Verbs inflect by tense, person and number (about a hundred irregular verbs); numbers, ordinals and round numbers;
     plurals with irregulars.
   * Possessives take "'s", "a" becomes "an" before a vowel, and a capital follows a stop or a paragraph (not a colon).
   * Spaces: none before punctuation, one after a comma or semicolon. This is the safety net for joins.
   * Over 3,000 records of Aieliaalas II the realiser made 3,360 first mentions of persons (995 of them kinship),
     4,004 pronouns and 395 later short forms.

Everything a plain context-free grammar cannot do is one of the following:

* the bound entities and the facts passed down;
* the attributes passed up;
* the record's and the catalogue's memory;
* the plan's directives;
* the realiser.

Each was needed by something the prototype printed wrong first:

* the attribute namespace by a line about the moon given the comet's tail ("they have said so for years");
* season order within a chronicle's year by a battle told before its war was declared;
* the reader's kinship by "my nephew" for the reader's brother;
* number agreement by "one years";
* a chronicle's facts taken year by year, by grain sent "by the railway" twenty-two years before the railway opened;
* the deaths' seasons by "died this spring, of the sickness" that the chronicle put in autumn.

## 5. The authoring format

**Files.** The grammar lives in text files with the extension `.vg`, a few per genre group, in one directory of the
tree (H-03 picks the place, say `src/galaxy/records/`). A file is rules:

```
# a comment
letter.ambient
  (once has:$k) {$k:full} {kid.main} {kid.tail}
  (once) {work.main} {work.tail}

work.main
  work is slow {=.slow}
  there is more work than I can do {=.busy}
  nobody wants a {trade} in {thisseason} {=.slow}
  (era=industrial) the [works|mill|yards] are on short time {=.slow}

work.tail
  (2) .
  (.slow=1) , so I sit by the door and watch the road.
  (.busy=1) , so I am up before the light.
  ; the {herd} do not care either way.

chronicle.plague
  {lead} {plague.main} {plague.tail}

plague.main
  ($e.thing!=great) sickness came to {$e.place} [with the boats|by the south road|from the hill farms]
  ($e.thing=great) {sickness} was at {$e.place} for the {#waves:ord} time
```

* A rule is a name at the start of a line, and its alternatives are the indented lines under it. Options in
  parentheses come first: a weight, guards, `once`, a tense.
* **The joins are spaces.** A parent writes its parts with spaces between them (`{lead} {main} {tail}`), and the
  realiser removes a space before punctuation and collapses doubles. A tail that starts with a comma or a stop joins
  cleanly. A trailing space in a file is invisible, and the prototype's first composed rules ran words together
  ("autumn,the", "diedin") until this convention was made.
* **Local attributes start with a dot.**
* **A count takes its noun through `{#n|noun}`.**

**Embedding.** Each file is wrapped once in a raw string:

```
R"VG(
...the rules...
)VG"
```

A C++ source includes it where a string goes:

```
static const char* const RECORDS_LETTER =
#include "records/letter.vg"
;
```

* This needs no build step, no `xxd` and no generated header. Make's `-MMD` and CMake's dependency scanning both see the
  include, so editing a grammar file rebuilds what reads it.
* The files stay readable: the wrapper is two lines.
* MSVC limits one string literal to about 16 KB. The release's Windows builds use clang (llvm-mingw), which has no such
  limit, but files kept under 16 KB stay safe for an MSVC build of CMakeLists.txt.
* The loader strips the wrapper, so the same parser reads the embedded text and a file from disk.

**Reloading.** `vesperis_test grammar` reads the `.vg` files from the tree, not the embedded copies. So an author edits a
rule and reruns the harness without rebuilding: the prototype's parse took under 2 ms for 579 alternatives. The harness
prints:

* the catalogue of a world, or a genre's records;
* the checks;
* the measures, with the most repeated frames and the most used alternatives (`rule:line xN`);
* one event from several sides.

When the format moves from the prototype to H-03 it gains:

* a check that every rule referenced exists, and every rule is reachable;
* a list of the facts and fields a guard may read, with a typo caught at load;
* per-file line numbers in every diagnostic.

**Why not C++ tables** (C-02's way):

* a template in a C++ string literal is escaped, comma-separated and quoted, so prose reads worse;
* every change is a rebuild;
* the guards would be C++ expressions mixed with prose.

**Why not JSON** (Tracery's way): quoting and commas on every line, no comments, and conditions as strings anyway.

## 6. The pipeline and the data model

The layers, each a function of the one before it, all in `galaxy/` (the records are read by the decoder, the radar, the
harness and the voice alike):

1. **The people's profile** (A-01, `galaxy/peoples.*` or within `ruins.*`). The fields:
   * `seed`; `age` (the age reached: prehistoric, medieval, industrial, modern, futuristic, far future); `eras` (the ages
     passed, each with its start and end years in the people's count);
   * `cause` of the collapse (by the age and the world, from the ideas note's section 8 lists), its `onset` year and its
     `stages` (warning, onset, crisis, end, as fractions of the collapse);
   * `endedYearsAgo` (C-08's age, longer for the late ages); `peak` population and `held` share of the land;
   * `government` (chiefs, monarchy, council, theocracy, republic, assembly, corporate, machine), `faith` (the god and
     its rites, `Lore::god`), `economy` (hunting, herding, farming, trade, industry, information); `values` (war, piety,
     trade, learning, art: five weights);
   * the genome's meanings (A-04: the sacred number, the focal monument, the dead's custom);
   * `peoples`/`which` (C-13);
   * the Lore names (C-02's, kept).

   A-01 builds the profile to these fields; the grammar reads them as facts.
2. **The history** (H-02, `galaxy/history.*`), deterministic from the profile, the plan (T-01) and the world:
   * **Entities:**
     * places: the plan's capital, cities, towns, villages, with founding and abandonment years;
     * the lore's river, mountain and sea; buildings and works (T-03), each with its place and years;
     * persons: name (a surname from the age when registers keep them), gender (one pronoun for life), born, died,
       `deathCause` (an event), home and moves, `mother`/`father`/`spouse`/`kids`, trade, role (ruler, priest, maker,
       the last), traits;
     * groups: the people, a town's levy, the council, the Company, the other people;
     * institutions with their changes; the god.
   * **Events:** `kind`, `year`, `season`, `place`, role slots (`a`, `b`, `c`: who did it, to whom, with whom), `cause`
     (an event), `value` (the dead, the houses, the spans, the years of work), `out` (a battle's result, a sign's
     stage), `thing` (an event's own text: what was built, fought over, agreed), base salience, public or private.
     * The prototype's kinds:
       * founding, accession, a ruler's death, reform;
       * war, battle, peace;
       * plague, famine, good and bad harvest, flood, drought, earthquake;
       * a building finished, a festival (the city's hundredth year), an omen (the comet's return, an eclipse, aurorae,
         falling stars);
       * an invention, a strike or riot, a town abandoned, a sign of the collapse, an exodus;
       * private: birth, marriage, move, death.
     * H-02 adds:
       * migrations and discoveries;
       * disasters read from the world (an impact, an eruption, a flare, a long winter);
       * the other people's wars;
       * figures' deeds;
       * the collapse's causes from many sides.
   * **Indices:** the events of each year, each person's life events, each place's events.
   * **Per year:** the ruler at that year and their reign's start (the chronicle's heading), the harvest's quality.
   * **Causes kept** (Ryan's "causal bookkeeping"):
     * a famine names the harvests that failed;
     * a battle and a peace name their war;
     * a death names the event that caused it, and takes its season;
     * an exodus names the sign that drove it.

     A cause is what lets a text say "after the second bad harvest" and be right.
3. **The records** (T-04 sizes them, H-04 writes them, built as one catalogue per people). A record has:
   * `genre`, `year`, `season`, and the date in the people's calendar (C-12's `shardDate`);
   * `writer` (a person, or none for an annal or a stone);
   * `reader` (a person, for a letter);
   * `subject` (a stone's dead, a report's matter);
   * `place` (where it lies: T-04's building and object);
   * `medium` (the age's: a stone, a tablet, a codex, a print, a cylinder, a tape, a crystal);
   * `told` (the events, with their order);
   * `tags` (H-05's category, broadcast or not, the stage of the collapse);
   * `seed`;
   * the text, and its tokens for the tongue (lemma and features per word).
4. **The inputs** the grammar reads (the plan's "dozens", as facts, word lists and bound fields):
   * *The world*:
     * its type and climate;
     * seasons (the names and the tilt), the day and the year (C-12);
     * the sky: the sun by its class, the moons and the named moon, the ring, the second sun, the wanderer, the comet
       and its period, aurorae, a locked day's edge;
     * the land's traits, the water (river, sea, old sea), the crops (one main and one second a world), trees,
       animals, weather (by season, which the prototype lacked: "the frost took a part of the kale" in summer);
     * the people's similes drawn from these.
   * *The people*:
     * age and era at the record's date; the stage of the collapse and its cause;
     * the names: the people, the god, the places, the festival, the star, the moon;
     * government and its titles (King/Queen, Chancellor); faith; economy; values; the genome's meanings;
     * C-13's other people and its fate.
   * *The history at the date*:
     * the ruler and the reign's year;
     * the events of the year and the year before, rumours of what is coming, the inventions that exist ("the
       railway" only after it opened), the waves of the great sickness so far;
     * a person's kin, age, trade, spouse, children, and the living and the dead among them.
   * *The record*: genre, medium, writer's temper (plain, wry, anxious, pious: the same news told differently), the
     reader's relation, whether the reader is far, the season of writing.
   * *The language*: the lemma and features of each word (for the tongue), the voice's reading length.

## 7. The evaluation plan

**Checks by construction** (the realiser and the plan cannot produce these):

* a pronoun for an entity that is not the last of its gender in reach;
* a verb that disagrees with its subject;
* "a" before a plural;
* a number disagreeing with its noun;
* a person writing or written to after their death;
* an event told after the record's date (only rumours of a few kinds, a year ahead);
* the same event twice in a record;
* a reader told their own wedding or their own child's birth;
* a battle before its war in a chronicle's year.

**Checks by the harness, over thousands of records a world** (the prototype ran these over 3,000 a world: no failures
but lengths at the edges):

* marks left unexpanded (`{`, `}`, `[`, `]`, `$`, `@`, `#`);
* a doubled word within a sentence;
* "a" or "an" before a known plural;
* an anachronism: a word of a later age in a record of an earlier one.

  The prototype checked surface words ("railway", "telegraph", "gas" in a medieval text), and "bore" (the verb) falsely
  matched "bores" (the Board's wells). H-03 should tag the lexicon by age instead, so the check reads which entries
  were used, not the words.
* a word of a later invention within an age: the prototype's first print sent grain "by the railway" in year 350, and
  the railway opened in 372. A chronicle reads the world as it was in each of its years, not its first, and the history's
  inventions gate their words the way the ages do;
* a record's length outside its genre's range;
* every place and person named exists in the history at the date (H-04's "no unknown place or figure");
* no event before its cause.

**Variety measures** (the harness prints them for a world's catalogue):

* **The share of sentences on a repeated frame.**
  * A sentence's frame is its words with names, digits and number words masked. A sentence counts if its frame occurs
    twice or more in the sample; only sentences of six words or more count.
  * The genre's formulas are left out: a letter's greeting and closing, a chronicle year's heading, a stone's name and
    blessing. A reader expects those to recur.
  * It is measured at 50, 100 and 200 records a world, with the most repeated frames listed (that list is where the
    next alternatives go).
* Distinct bigrams and trigrams over the sample's first 1,500 words, comparable between samples of different lengths.
* **Rule use:** how many alternatives were used and the most used ones by file and line.

**Targets for H-03 and H-04** (the prototype's numbers in brackets):

* repeated frames of six words or more under 15% at 50 records a people (12%), 25% at 100 (21–22%) and 35% at 200
  (33–35%);
* every automatic check at zero;
* a record's text under 50 µs and a people's history under 10 ms;
* no genre under 8 alternatives on any rule used more than once a record;
* frequent parts composed (lead, main, tail), with whole hand-written sentences for the strong moments (the last
  recording, the turn at the end of a letter).

**The reading.** Fifty records of a world read before judging (C-02's lesson). The user reads the samples of each age as
H-04 writes them, beside the previous print of the same world.

## 8. The prototype

**What it was.** About 1,500 lines of C++ in the scratchpad, linked against `build/galaxy/*.o` and `build/core/*.o`
(C-02's `loreOf`, `cultureOf`, `personName` and the world's inputs), with a grammar file of 167 rules and 579
alternatives read at run time. It had five parts:

* a history stub;
* the plan's directives;
* the grammar engine with guards, bindings, `once`, dryness and synthesized attributes;
* the realiser;
* the checks and measures.

It printed:

* each world's history;
* twenty-five records of three genres (letters, chronicles, epitaphs) per world;
* one plague from three sides (the chronicle of its year, a letter of a family it struck, a stone);
* the measures over 3,000 records a world, beside C-02's on the same worlds.

**The worlds.** Aieliaalas II's Rauzhia were set medieval, and their collapse is a sickness in waves from year 281.
Leileashphail III's Feloya were set industrial from year 309 of 542, and their collapse is the drying from year 454. Both
are the C series' test worlds, so the samples read beside C-02's. The ages are the prototype's choice; A-01 draws them.

**What it showed** (the numbers above, and these lessons):

* **The history is what reads as depth.** The kinship ("my daughter-in-law Crini had a son in the night"), the reigns,
  the war with Ryanu over the border at Cretsax, the stone of a woman who stood at the field below Zeyzox: each is one
  sentence that C-02 cannot write.
* **Composition is what reads as variety.** The whole-sentence grammar (414 alternatives) repeated more than C-02 at 100
  records (43–46% against 34–38%). The composed one (579) halved that.
* **Weighting the draw is not variety.** Catalogue dryness at 0, 1.5, 3 and 6 moved the repeats at 100 records by three
  points at most, no more than a different seed does: the repeats come from small pools, not from bad draws.
* **Every composed join needs its convention**: the spaces, the attribute namespace, the number's noun, the tail that
  fits every main (or the attribute that says which mains it fits).
* **Order within a year matters as much as which events.** A year's events are picked by salience and told by season.
* **The register holds** when most news is private and ordinary: in the samples the collapse is late and lands harder
  for it.
* **Same-town letters** are odd ("I am writing because you will not come to the door") and the plan keeps them to one
  in ten.
* **The industrial era reads as itself.** The Board, the Gazette, the Company's shop, the railway's two days and the
  riots over the water came from a dozen guarded alternatives and six invention events.

**What it left for H-03 and H-04.**

* Letters are the most repetitive genre: 83% of frames recur over 1,000 letters, against 74–80% for chronicles,
  because the letters' private news is still whole sentences.
* Weather is not by season ("the frost took a part of the kale" in summer).
* The chronicle's run of empty years is a formula with three wordings.
* An epitaph's deed is one of three kinds.
* A people's whole catalogue was not built as T-04 will size it.

## 9. What the later milestones take from here

* **A-01** builds the profile to section 6's fields. The ages, the cause and its stages, the eras with their years,
  government, faith and economy are facts the grammar will read, so they need names, not only numbers.
* **C-16** gives each radio age's programmes sentences from the records. The catalogue's tags say which records are
  broadcasts.
* **H-02** builds the history to section 6:
  * families and persons with their kinship and causes of death;
  * the per-year index; causes kept for every event that has one;
  * deaths in their cause's season; a war's battles after its declaration;
  * at least the prototype's kinds; and the catalogue's deterministic build.
* **T-04** sizes the catalogue per people and gives each record its place and medium.
* **H-03** builds the engine (done 2026-10-09: `galaxy/grammar.*`, `src/galaxy/grammar/*.vg`; the format as built, with what it
  gained, is in `docs/reference/12-civilisations.md`, "The grammar engine"):
  * the format, its loader and the embedding (section 5);
  * the guards, `once` and dryness, inherited and synthesized attributes, the directives, the realiser;
  * the checks and the measures in `vesperis_test grammar`.

  It then rebuilds C-02's thirty-one kinds on it, from the same inputs, before any new content (the plan's proof).
  It also tags the lexicon by age for the anachronism check.
* **H-04** writes the genres of each age to section 7's targets. It composes the frequent parts, re-tunes C-06's
  language share for lemmas, and has the voice read the opening. (Done 2026-10-10: `12-civilisations.md`, "The records (H-04)";
  the tongue learns roots found among the records' own words rather than a dictionary's lemmas; the targets met at 50 records
  a people, not at 100: 34.6% against 25%, KI-381.)
* **H-05** uses the records' tags for the three categories.
* **H-06** reads the history's events from the shards read (the record's `told` list), which the catalogue already
  keeps.

## References

Black, J. B., and Wilensky, R. (1979). An Evaluation of Story Grammars. *Cognitive Science* 3(3).

Compton, K. (2016). So you want to build a generator… (essay, blog).

Compton, K., Kybartas, B., and Mateas, M. (2015). Tracery: An Author-Focused Generative Text Tool. *ICIDS 2015*.

Dias, B. Improv (JavaScript library, documentation at improv.readthedocs.io); *Voyageur* (2017).

Gatt, A., and Reiter, E. (2009). SimpleNLG: A Realisation Engine for Practical Applications. *ENLG 2009*.

Grinblat, J., and Bucklew, C. B. (2017). Subverting Historical Cause & Effect: Generation of Mythic Biographies in Caves
of Qud. *FDG 2017*. Grinblat, J. (2018). Procedurally Generating History in Caves of Qud. *GDC 2018*.

Johnson, M. R. (2016). Procedural Generation of Linguistics, Dialects, Naming Conventions and Spoken Sentences. *DiGRA–FDG
2016*.

Knuth, D. E. (1968). Semantics of Context-Free Languages. *Mathematical Systems Theory* 2(2).

Propp, V. (1928). *Morphology of the Folktale*.

Ranta, A. (2011). *Grammatical Framework: Programming with Multilingual Grammars*. CSLI.

Reiter, E., and Dale, R. (2000). *Building Natural Language Generation Systems*. Cambridge University Press.

Rumelhart, D. E. (1975). Notes on a Schema for Stories. In Bobrow and Collins, eds., *Representation and Understanding*.

Ruskin, E. (2012). AI-driven Dynamic Dialog through Fuzzy Pattern Matching. *GDC 2012*.

Ryan, J. (2018). *Curating Simulated Storyworlds*. PhD dissertation, UC Santa Cruz.

Ryan, J., Seither, E., Mateas, M., and Wardrip-Fruin, N. (2016). Expressionist: An Authoring Tool for In-Game Text
Generation. *ICIDS 2016*.

Short, E. (2015). *The Annals of the Parrigues*. (2016) Beyond Branching: Quality-Based, Salience-Based, and Waypoint
Narrative Structures; Bowls of Oatmeal and Text Generation (blog, emshort.blog).

Short, T. X., and Adams, T., eds. (2017). *Procedural Generation in Game Design*; (2019) *Procedural Storytelling in Game
Design* (Adams, "Emergent Narrative in Dwarf Fortress").
