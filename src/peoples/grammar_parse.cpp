// E-10: the `.vg` format: the parser (`vgParse`), the link against the model's schema (`vgLink`) and an alternative's place
#include "grammar_internal.h"

// ---------------------------------------------------------------------------
// the format
// ---------------------------------------------------------------------------

std::string VgGrammar::where(int alt) const {
    if (alt < 0 || alt >= (int)alts.size()) return "?";
    const VgAlt& a = alts[alt];
    return (a.file >= 0 && a.file < (int)files.size() ? files[a.file] : std::string("?")) + ":" + std::to_string(a.line);
}

namespace {

struct Parser {
    VgGrammar& G; int file; int line = 0;
    Parser(VgGrammar& g, int f) : G(g), file(f) {}
    void error(const std::string& what) { G.errors.push_back(G.files[file] + ":" + std::to_string(line) + ": " + what); }

    bool parseMark(const std::string& m0, VgNode& n) {
        std::string m = trim(m0);
        if (m.empty()) { error("an empty mark {}"); return false; }
        char c = m[0];
        if (c == '~') {   // H-04: the people's own word for a thing
            n.type = VgNode::TONGUE; n.s = lower(trim(m.substr(1)));
            if (n.s.empty()) { error("a word of theirs needs a key: {~coin}"); return false; }
            G.tongueKeys.insert(n.s);
            return true;
        }
        if (c == '@') {
            n.type = VgNode::DIRECT; size_t sp = m.find(' ');
            n.s = m.substr(1, sp == std::string::npos ? std::string::npos : sp - 1); n.a = sp == std::string::npos ? "" : trim(m.substr(sp + 1));
            return true;
        }
        if (c == '=') {
            n.type = VgNode::ATTR; std::string r = m.substr(1); size_t col = r.find(':');
            n.s = col == std::string::npos ? r : r.substr(0, col); n.a = col == std::string::npos ? "1" : r.substr(col + 1);
            if (n.s.size() < 2 || n.s[0] != '.') { error("an attribute must start with a dot: {" + m + "}"); return false; }
            return true;
        }
        if (c == '#') {
            n.type = VgNode::NUM; std::string r = m.substr(1);
            size_t bar = r.find('|'); if (bar != std::string::npos) { n.b = trim(r.substr(bar + 1)); r = r.substr(0, bar); }
            size_t col = r.find(':'); if (col != std::string::npos) { n.a = r.substr(col + 1); r = r.substr(0, col); }
            if (!n.a.empty() && n.a != "ord" && n.a != "about" && n.a != "digits" && n.a != "words") { error("a number's form is ord, about, digits or words: {" + m + "}"); return false; }
            if (r.empty()) { error("a number without a key: {" + m + "}"); return false; }
            if (r[0] == '$') { size_t d = r.find('.'); n.s = r.substr(0, d); std::string rest = d == std::string::npos ? "" : r.substr(d + 1); while (!rest.empty()) { size_t e = rest.find('.'); n.path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); } }
            else n.s = r;
            return true;
        }
        if (c == '+') {
            n.type = VgNode::DECL; std::string r = m.substr(1);
            size_t bar = r.find('|'); if (bar != std::string::npos) { n.b = r.substr(bar + 1); r = r.substr(0, bar); }
            size_t col = r.find(':'); n.s = r.substr(0, col); n.a = col == std::string::npos ? "?" : r.substr(col + 1);
            if (n.s.size() < 2 || n.s[0] != '$' || (n.a != "he" && n.a != "she" && n.a != "they" && n.a != "it" && n.a != "?")) { error("a person brought in is {+$x:he|she|they|it|?}: {" + m + "}"); return false; }
            if (std::find(G.localEntities.begin(), G.localEntities.end(), n.s) == G.localEntities.end()) G.localEntities.push_back(n.s);
            return true;
        }
        if (c == '$') {
            n.type = VgNode::ENT; std::string r = m; size_t col = r.find(':');
            if (col != std::string::npos) { n.a = r.substr(col + 1); r = r.substr(0, col); }
            static const char* const FORMS[] = {"", "obj", "poss", "refl", "name", "full", "voc", "the", nullptr};
            bool ok = false; for (int i = 0; FORMS[i]; i++) if (n.a == FORMS[i]) ok = true;
            if (!ok) { error("an entity's form is obj, poss, refl, name, full, voc or the: {" + m + "}"); return false; }
            size_t d = r.find('.'); n.s = r.substr(0, d);
            std::string rest = d == std::string::npos ? "" : r.substr(d + 1);
            while (!rest.empty()) { size_t e = rest.find('.'); n.path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); }
            return true;
        }
        if (c == '.') {
            n.type = VgNode::LOCAL; size_t col = m.find(':');
            n.s = m.substr(0, col); n.a = col == std::string::npos ? "" : m.substr(col + 1);
            if (!n.a.empty() && n.a != "obj" && n.a != "subj") { error("a kept word's form is subj or obj: {" + m + "}"); return false; }
            return true;
        }
        size_t gt = m.find('>');
        if (gt != std::string::npos) {
            n.type = VgNode::RULE; n.s = trim(m.substr(0, gt)); n.b = trim(m.substr(gt + 1));
            if (n.b.size() < 2 || n.b[0] != '.') { error("a word is kept as a dotted name: {" + m + "}"); return false; }
            return true;
        }
        size_t col = m.find(':');
        if (col != std::string::npos) {
            std::string l = trim(m.substr(0, col)), r = trim(m.substr(col + 1));
            if (r == "pl") { n.type = VgNode::PLURAL; n.s = l; return true; }
            static const char* const PRON[] = {"I", "we", "you", "they", "he", "she", "it", nullptr};
            bool pron = false; for (int i = 0; PRON[i]; i++) if (r == PRON[i]) pron = true;
            if (!r.empty() && (r[0] == '$' || r[0] == '.' || pron)) {
                n.type = VgNode::VERB; n.s = l; n.a = r;
                if (r[0] == '$') {   // a subject's path: `{die:$e.a}`
                    size_t d = r.find('.'); n.a = r.substr(0, d);
                    std::string rest = d == std::string::npos ? "" : r.substr(d + 1);
                    while (!rest.empty()) { size_t e = rest.find('.'); n.path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); }
                }
                return true;
            }
            error("unknown mark {" + m + "}"); return false;
        }
        n.type = VgNode::RULE; n.s = m;
        return true;
    }

    // a text into nodes; `inChoice`: stop at '|' and ']' (the choice's own)
    void parseSeq(const std::string& t, size_t& i, bool inChoice, std::vector<VgNode>& out) {
        auto text = [&](const std::string& s) { if (!out.empty() && out.back().type == VgNode::TEXT) out.back().s += s; else { VgNode n; n.s = s; out.push_back(n); } };
        while (i < t.size()) {
            char c = t[i];
            if (inChoice && (c == '|' || c == ']')) return;
            if (c == '\\' && i + 1 < t.size()) {
                if (t[i + 1] == 'n') { VgNode n; n.type = VgNode::PAR; out.push_back(n); }
                else text(std::string(1, t[i + 1]));
                i += 2; continue;
            }
            if (c == '[') {
                i++;
                VgNode n; n.type = VgNode::CHOICE;
                for (;;) {
                    std::vector<VgNode> opt; parseSeq(t, i, true, opt); n.opts.push_back(opt);
                    if (i < t.size() && t[i] == '|') { i++; continue; }
                    if (i < t.size() && t[i] == ']') { i++; break; }
                    error("an unclosed [choice"); return;
                }
                out.push_back(n); continue;
            }
            if (c == '{') {
                size_t j = t.find('}', i);
                if (j == std::string::npos) { error("an unclosed {mark"); i = t.size(); return; }
                std::string inner = t.substr(i + 1, j - i - 1);
                if (inner.find('{') != std::string::npos) { error("a mark inside a mark: {" + inner + "}"); i = j + 1; continue; }
                VgNode n; if (parseMark(inner, n)) out.push_back(n);
                i = j + 1; continue;
            }
            if (c == ']' || c == '}') { error(std::string("a stray '") + c + "'"); i++; continue; }
            size_t j = i; while (j < t.size() && t[j] != '[' && t[j] != '{' && t[j] != '\\' && t[j] != ']' && t[j] != '}' && !(inChoice && t[j] == '|')) j++;
            if (j == i) j++;
            text(t.substr(i, j - i)); i = j;
        }
    }

    bool parseCond(const std::string& tok, VgCond& c) {
        if (startsWith(tok, "!has:")) { c.key = tok.substr(5); c.op = 7; return !c.key.empty(); }
        if (startsWith(tok, "has:")) { c.key = tok.substr(4); c.op = 6; return !c.key.empty(); }
        size_t p = tok.find_first_of("=!<>", 1);
        if (p == std::string::npos || p == 0) return false;
        c.key = tok.substr(0, p);
        std::string rest = tok.substr(p);
        if (startsWith(rest, ">=")) { c.op = 2; c.val = rest.substr(2); }
        else if (startsWith(rest, "<=")) { c.op = 3; c.val = rest.substr(2); }
        else if (startsWith(rest, "!=")) { c.op = 1; c.val = rest.substr(2); }
        else if (rest[0] == '=') { c.op = 0; c.val = rest.substr(1); }
        else if (rest[0] == '>') { c.op = 4; c.val = rest.substr(1); }
        else if (rest[0] == '<') { c.op = 5; c.val = rest.substr(1); }
        else return false;
        c.numeric = numberOf(c.val, c.num);
        if (c.op >= 2 && c.op <= 5 && !c.numeric) return false;   // an order needs a number or an age
        return true;
    }

    void parseLine(const std::string& raw, int& cur) {
        std::string ln = raw;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        std::string t = trim(ln);
        if (t.empty() || t[0] == '#') return;
        if (t == "R\"VG(" || t == ")VG\"") return;   // the raw string's wrapper (a file read from disk)
        if (ln[0] == '@') {
            std::vector<std::string> w; size_t p = 1;
            while (p < ln.size()) { size_t q = ln.find_first_of(" \t", p); if (q == std::string::npos) q = ln.size(); if (q > p) w.push_back(ln.substr(p, q - p)); p = q + 1; }
            if (w.empty()) { error("an empty @ line"); return; }
            if (w[0] == "root") { for (size_t i = 1; i < w.size(); i++) G.roots.push_back(w[i]); return; }
            if (w[0] == "apart") {   // D-05
                if (w.size() == 1) { if ((int)G.apartFiles.size() <= file) G.apartFiles.resize(file + 1, 0); G.apartFiles[file] = 1; return; }
                for (size_t i = 1; i < w.size(); i++) {
                    const size_t q = w[i].find('=');
                    if (q == std::string::npos || q == 0 || q + 1 >= w[i].size()) { error("@apart, or @apart key=value"); continue; }
                    G.apartGuards.push_back({w[i].substr(0, q), w[i].substr(q + 1)});
                }
                return;
            }
            if (w[0] == "instead") {   // D-05
                if (w.size() != 4) { error("@instead <guard> <rule> <rule2>"); return; }
                VgInstead I;
                if (!parseCond(w[1], I.cond)) { error("@instead: a guard: " + w[1]); return; }
                I.from = w[2]; I.to = w[3]; I.file = file; I.line = line;
                G.insteads.push_back(I);
                return;
            }
            if (w[0] == "lexicon") {
                if (w.size() < 3) { error("@lexicon <age|fact> words: the age one of prehistoric, medieval, industrial, modern, futuristic, farfuture"); return; }
                if (ageOf(w[1]) < 0) {   // H-04: a fact's words (linked against the schema)
                    for (size_t i = 2; i < w.size(); i++) G.lexFactNames[lower(w[i])] = w[1];
                    return;
                }
                int a = ageOf(w[1]);
                for (size_t i = 2; i < w.size(); i++) { std::string k = lower(w[i]); auto it = G.lexicon.find(k); if (it == G.lexicon.end() || it->second > a) G.lexicon[k] = a; }
                return;
            }
            error("unknown @" + w[0]); return;
        }
        if (ln[0] != ' ' && ln[0] != '\t') {   // a rule
            size_t par = t.find('(');
            std::string name = trim(par == std::string::npos ? t : t.substr(0, par));
            for (char ch : name) if (!std::isalnum((unsigned char)ch) && ch != '.' && ch != '-' && ch != '_') { error("a rule's name is letters, digits, '.', '-' and '_': " + name); cur = -1; return; }
            auto it = G.ruleIndex.find(name);
            if (it != G.ruleIndex.end()) { error("the rule " + name + " again (first at " + G.files[G.rules[it->second].file] + ":" + std::to_string(G.rules[it->second].line) + ")"); cur = -1; return; }
            VgRule r; r.name = name; r.file = file; r.line = line;
            if (par != std::string::npos) {
                size_t e = t.find(')', par);
                if (e == std::string::npos) { error("an unclosed rule option"); }
                else {
                    std::string opts = t.substr(par + 1, e - par - 1); size_t p = 0;
                    while (p < opts.size()) {
                        size_t q = opts.find(' ', p); if (q == std::string::npos) q = opts.size();
                        std::string o = opts.substr(p, q - p); p = q + 1;
                        if (o.empty()) continue;
                        if (o == "pl") r.number = VN_PLURAL; else if (o == "sing") r.number = VN_SING; else if (o == "mass") r.number = VN_MASS; else if (o == "keep") r.keep = true;
                        else if (o == "steady") r.steady = true;
                        else if (o == "formula") r.formula = true;
                        else if (startsWith(o, "people=")) { r.peopleK = std::atoi(o.c_str() + 7); if (r.peopleK < 1) error("people=K, K one or more"); }
                        else if (startsWith(o, "share=")) { r.share = std::atof(o.c_str() + 6); if (r.share <= 0 || r.share > 1) error("share=F, F above 0 and at most 1"); }
                        else if (startsWith(o, "words=")) { size_t dd = o.find(".."); if (dd == std::string::npos) error("words=lo..hi"); else { r.wordsLo = std::atoi(o.substr(6, dd - 6).c_str()); r.wordsHi = std::atoi(o.substr(dd + 2).c_str()); } }
                        else error("unknown rule option " + o);
                    }
                }
            }
            cur = (int)G.rules.size(); G.ruleIndex[name] = cur; G.rules.push_back(r);
            return;
        }
        if (cur < 0) { error("an alternative outside a rule"); return; }
        VgAlt a; a.rule = cur; a.file = file; a.line = line;
        a.apart = (int)G.apartFiles.size() > file && G.apartFiles[file];   // D-05
        {   // H-04: the alternative's key: its rule's name and its line (a people's dialect reads it; stable while other rules change)
            uint64_t h = 1469598103934665603ULL;
            for (unsigned char ch : G.rules[cur].name + "|" + t) { h ^= ch; h *= 1099511628211ULL; }
            a.key = mix64(h);
        }
        if (t[0] == '(') {
            size_t e = t.find(')');
            if (e == std::string::npos) { error("unclosed options"); return; }
            std::string opts = t.substr(1, e - 1); size_t p = 0;
            while (p < opts.size()) {
                size_t q = opts.find(' ', p); if (q == std::string::npos) q = opts.size();
                std::string o = opts.substr(p, q - p); p = q + 1;
                if (o.empty()) continue;
                if (o == "once") { a.once = true; continue; }
                if (o == "tense=past") { a.tense = VG_PAST; continue; }
                if (o == "tense=present") { a.tense = VG_PRESENT; continue; }
                if (o == "pl") { a.number = VN_PLURAL; continue; }
                if (o == "sing") { a.number = VN_SING; continue; }
                if (o == "mass") { a.number = VN_MASS; continue; }
                if (std::isdigit((unsigned char)o[0]) || (o[0] == '.' && o.size() > 1 && std::isdigit((unsigned char)o[1]))) { a.w = std::atof(o.c_str()); if (a.w <= 0) error("a weight above zero: " + o); continue; }
                VgCond c;
                if (o.find('|') != std::string::npos) {   // an OR of guards
                    size_t s = 0; bool ok = true;
                    while (s <= o.size()) { size_t b = o.find('|', s); if (b == std::string::npos) b = o.size(); VgCond x; if (!parseCond(o.substr(s, b - s), x)) ok = false; else c.alt.push_back(x); s = b + 1; }
                    if (!ok) { error("a guard: " + o); continue; }
                } else if (!parseCond(o, c)) { error("an option or a guard: " + o); continue; }
                a.conds.push_back(c);
            }
            t = trim(t.substr(e + 1));
        }
        size_t i = 0; parseSeq(t, i, false, a.body);
        G.rules[cur].alts.push_back((int)G.alts.size());
        G.alts.push_back(a);
    }
};

}   // namespace

bool vgParse(VgGrammar& G, const std::string& text, const std::string& file) {
    double t0 = nowMs();
    size_t errs = G.errors.size();
    Parser P(G, (int)G.files.size());
    G.files.push_back(file);
    int cur = -1; size_t p = 0;
    while (p <= text.size()) {
        size_t e = text.find('\n', p); if (e == std::string::npos) e = text.size();
        P.line++;
        P.parseLine(text.substr(p, e - p), cur);
        p = e + 1;
    }
    G.parseMs += nowMs() - t0;
    return G.errors.size() == errs;
}

namespace {
struct Linker {
    VgGrammar& G; const VgSchema& S;
    Linker(VgGrammar& g, const VgSchema& s) : G(g), S(s) {}
    std::unordered_map<std::string, int> facts, ents, paths, fields, dirs;
    int file = 0, line = 0;
    void error(const std::string& w) { G.errors.push_back(G.files[file] + ":" + std::to_string(line) + ": " + w); }
    bool localEnt(const std::string& n) const { return std::find(G.localEntities.begin(), G.localEntities.end(), n) != G.localEntities.end(); }
    // `$x` and its path: the schema's entity (or a person a text brings in), its paths, and the last a field when `field` may end it
    bool entityPath(const std::string& head, const std::vector<std::string>& path, int& ref, std::vector<int>& pathRef, int& fieldRef, bool fieldOk) {
        auto it = ents.find(head);
        if (it != ents.end()) ref = it->second; else if (localEnt(head)) ref = -1; else { error("unknown entity " + head); return false; }
        pathRef.clear(); fieldRef = -1;
        for (size_t i = 0; i < path.size(); i++) {
            auto pt = paths.find(path[i]);
            if (pt != paths.end()) { pathRef.push_back(pt->second); continue; }
            if (i + 1 == path.size() && fieldOk) {
                auto ft = fields.find(path[i]);
                if (ft != fields.end()) { fieldRef = ft->second; return true; }
                if (path[i] == "gender") { fieldRef = -2; return true; }
                if (path[i] == "name") { fieldRef = -3; return true; }
                if (path[i] == "person") { fieldRef = -4; return true; }
            }
            error("unknown field ." + path[i] + " of " + head); return false;
        }
        return true;
    }
    void cond(VgCond& c) {
        if (!c.alt.empty()) { for (VgCond& x : c.alt) cond(x); return; }
        if (c.key[0] == '.') { c.kind = 1; return; }
        if (c.key[0] == '$') {
            size_t d = c.key.find('.');
            std::string head = c.key.substr(0, d); std::vector<std::string> path; std::string rest = d == std::string::npos ? "" : c.key.substr(d + 1);
            while (!rest.empty()) { size_t e = rest.find('.'); path.push_back(rest.substr(0, e)); rest = e == std::string::npos ? "" : rest.substr(e + 1); }
            c.kind = path.empty() ? 2 : 3;
            entityPath(head, path, c.ref, c.pathRef, c.fieldRef, true);
            return;
        }
        auto it = facts.find(c.key);
        if (it == facts.end()) { error("unknown fact '" + c.key + "' in a guard"); return; }
        c.kind = 0; c.ref = it->second;
    }
    void node(VgNode& n) {
        switch (n.type) {
            case VgNode::CHOICE: for (auto& o : n.opts) for (VgNode& x : o) node(x); break;
            case VgNode::RULE: {
                int r = G.rule(n.s);
                if (r >= 0) { n.ref = r; break; }
                auto it = facts.find(n.s);
                if (it != facts.end() && n.b.empty()) { n.type = VgNode::FACT; n.ref = it->second; break; }
                error("unknown {" + n.s + "}: no rule and no fact"); break;
            }
            case VgNode::PLURAL: {
                int r = G.rule(n.s); if (r >= 0) { n.ref = r; break; }
                auto it = facts.find(n.s); if (it != facts.end()) { n.fieldRef = it->second; break; }
                error("unknown {" + n.s + ":pl}"); break;
            }
            case VgNode::ENT: entityPath(n.s, n.path, n.ref, n.pathRef, n.fieldRef, true); break;
            case VgNode::VERB:
                if (n.a[0] == '$') entityPath(n.a, n.path, n.ref, n.pathRef, n.fieldRef, false);
                break;
            case VgNode::NUM: {
                if (n.s[0] == '$') { entityPath(n.s, n.path, n.ref, n.pathRef, n.fieldRef, true); if (n.fieldRef == -1) error("a number from an entity needs a field: {#" + n.s + "}"); break; }
                if (n.s[0] == '.') break;
                int r = G.rule(n.s); if (r >= 0) { n.ref = r; n.fieldRef = -1; break; }
                auto it = facts.find(n.s); if (it != facts.end()) { n.ref = -1; n.fieldRef = it->second; break; }
                error("unknown {#" + n.s + "}"); break;
            }
            case VgNode::DIRECT: { auto it = dirs.find(n.s); if (it == dirs.end()) error("unknown directive @" + n.s); else n.ref = it->second; break; }
            default: break;
        }
    }
    void reach(int r) {
        if (r < 0 || G.rules[r].used) return;
        G.rules[r].used = true;
        std::vector<int> stack = {r};
        while (!stack.empty()) {
            int x = stack.back(); stack.pop_back();
            for (int ai : G.rules[x].alts) walk(G.alts[ai].body, stack);
        }
    }
    void walk(const std::vector<VgNode>& body, std::vector<int>& stack) {
        for (const VgNode& n : body) {
            if (n.type == VgNode::CHOICE) for (const auto& o : n.opts) walk(o, stack);
            int r = -1;
            if (n.type == VgNode::RULE || n.type == VgNode::PLURAL) r = n.ref;
            if (n.type == VgNode::NUM && n.s[0] != '$' && n.s[0] != '.' && n.fieldRef < 0) r = n.ref;
            if (r >= 0 && !G.rules[r].used) { G.rules[r].used = true; stack.push_back(r); }
        }
    }
    int numberOfRule(int r) const {   // a list's number when all its words have one (a rule's own, else its alternatives' alike)
        const VgRule& R = G.rules[r];
        if (R.number != VN_NONE) return R.number;
        int n = -1;
        for (int ai : R.alts) { int x = G.alts[ai].number; if (n < 0) n = x; else if (n != x) return VN_NONE; }
        return n < 0 ? VN_NONE : n;
    }
    // "a {goats}": an article written before a plural or a mass list
    void articles(const std::vector<VgNode>& body) {
        for (size_t i = 0; i < body.size(); i++) {
            const VgNode& n = body[i];
            if (n.type == VgNode::CHOICE) for (const auto& o : n.opts) articles(o);
            if (n.type != VgNode::TEXT || i + 1 >= body.size() || body[i + 1].type != VgNode::RULE || body[i + 1].ref < 0) continue;
            std::string t = lower(n.s);
            bool art = endsWith(t, " a ") || endsWith(t, " an ") || t == "a " || t == "an ";
            int num = numberOfRule(body[i + 1].ref);
            if (art && (num == VN_PLURAL || num == VN_MASS)) error("\"a\" before {" + body[i + 1].s + "}, a " + (num == VN_PLURAL ? "plural" : "mass") + " list");
        }
    }
};
}   // namespace

bool vgLink(VgGrammar& G, const VgSchema& S) {
    size_t errs = G.errors.size();
    Linker L(G, S);
    for (size_t i = 0; i < S.facts.size(); i++) L.facts[S.facts[i]] = (int)i;
    for (size_t i = 0; i < S.entities.size(); i++) L.ents[S.entities[i]] = (int)i;
    for (size_t i = 0; i < S.paths.size(); i++) L.paths[S.paths[i]] = (int)i;
    for (size_t i = 0; i < S.fields.size(); i++) L.fields[S.fields[i]] = (int)i;
    for (size_t i = 0; i < S.directives.size(); i++) L.dirs[S.directives[i]] = (int)i;
    G.schema = &S;
    for (VgRule& r : G.rules) r.used = false;
    for (VgAlt& a : G.alts) {
        L.file = a.file; L.line = a.line;
        for (VgCond& c : a.conds) L.cond(c);
        for (VgNode& n : a.body) L.node(n);
    }
    for (VgAlt& a : G.alts) { L.file = a.file; L.line = a.line; L.articles(a.body); }
    // D-05: the alternatives under an `@apart` guard; the swapped rules
    for (VgAlt& a : G.alts) {
        if (a.apart) continue;
        std::function<bool(const VgCond&)> apartCond = [&](const VgCond& c) {
            if (!c.alt.empty()) { for (const VgCond& x : c.alt) if (apartCond(x)) return true; return false; }
            for (const auto& kv : G.apartGuards) if (c.op == 0 && c.key == kv.first && c.val == kv.second) return true;
            return false;
        };
        for (const VgCond& c : a.conds) if (apartCond(c)) a.apart = true;
    }
    for (VgInstead& I : G.insteads) {
        L.file = I.file; L.line = I.line;
        L.cond(I.cond);
        I.fromRule = G.rule(I.from); I.toRule = G.rule(I.to);
        if (I.fromRule < 0) L.error("@instead: no rule " + I.from);
        if (I.toRule < 0) L.error("@instead: no rule " + I.to);
    }
    G.lexFact.clear();   // H-04: the words a fact allows
    for (const auto& kv : G.lexFactNames) {
        auto it = L.facts.find(kv.second);
        if (it == L.facts.end()) G.errors.push_back("@lexicon " + kv.second + ": no such fact (for '" + kv.first + "')");
        else G.lexFact[kv.first] = it->second;
    }
    // the roots (`@root name`, `@root prefix.*`) and what they reach
    for (const std::string& root : G.roots) {
        if (endsWith(root, ".*")) {
            std::string pre = root.substr(0, root.size() - 1); bool any = false;
            for (int r = 0; r < (int)G.rules.size(); r++) if (startsWith(G.rules[r].name, pre.c_str())) { L.reach(r); any = true; }
            if (!any) G.errors.push_back("@root " + root + ": no rule");
            continue;
        }
        int r = G.rule(root);
        if (r < 0) G.errors.push_back("@root " + root + ": no rule"); else L.reach(r);
    }
    for (bool more = true; more;) {   // D-05: a swapped rule's stand-in is reached where the rule is
        more = false;
        for (const VgInstead& I : G.insteads) if (I.fromRule >= 0 && I.toRule >= 0 && G.rules[I.fromRule].used && !G.rules[I.toRule].used) { L.reach(I.toRule); more = true; }
    }
    for (const VgRule& r : G.rules) {
        if (!r.used) G.errors.push_back(G.files[r.file] + ":" + std::to_string(r.line) + ": the rule " + r.name + " is reached from no root");
        if (r.alts.empty()) G.errors.push_back(G.files[r.file] + ":" + std::to_string(r.line) + ": the rule " + r.name + " has no alternative");
    }
    // the plural words of the plural lists (the check: no "a" or "one" before them)
    G.plurals.clear();
    static const char* const IRR[] = {"geese", "cattle", "children", "men", "women", "feet", "teeth", "mice", "oxen", "people", nullptr};
    for (int i = 0; IRR[i]; i++) G.plurals.insert(IRR[i]);
    for (const VgRule& r : G.rules) for (int ai : r.alts) {
        const VgAlt& a = G.alts[ai];
        if ((a.number != VN_NONE ? a.number : r.number) != VN_PLURAL || a.body.size() != 1 || a.body[0].type != VgNode::TEXT) continue;
        std::string t = lower(trim(a.body[0].s)); size_t sp = t.rfind(' '); std::string w = sp == std::string::npos ? t : t.substr(sp + 1);
        if (w.size() > 2 && w.back() == 's' && !endsWith(w, "ss")) G.plurals.insert(w);
    }
    // H-04's second pass: which rules may be drawn again (no directive under them) and which may end a sentence; the distinctive words
    {
        const int nr = (int)G.rules.size();
        std::vector<std::vector<int>> refs(nr);
        std::vector<char> direct(nr, 0), stop(nr, 0);
        std::function<void(const std::vector<VgNode>&, int)> scan = [&](const std::vector<VgNode>& body, int r) {
            for (const VgNode& n : body) {
                if (n.type == VgNode::CHOICE) for (const auto& o : n.opts) scan(o, r);
                else if (n.type == VgNode::DIRECT) direct[r] = 1;
                else if (n.type == VgNode::PAR) stop[r] = 1;
                else if (n.type == VgNode::TEXT) { if (n.s.find_first_of(".?!") != std::string::npos) stop[r] = 1; }
                else if ((n.type == VgNode::RULE || n.type == VgNode::PLURAL) && n.ref >= 0) refs[r].push_back(n.ref);
                else if (n.type == VgNode::NUM && n.s[0] != '$' && n.s[0] != '.' && n.fieldRef < 0 && n.ref >= 0) refs[r].push_back(n.ref);
            }
        };
        for (int r = 0; r < nr; r++) for (int ai : G.rules[r].alts) scan(G.alts[ai].body, r);
        for (int r = 0; r < nr; r++) { G.rules[r].pure = !direct[r]; G.rules[r].stops = stop[r] != 0; }
        for (bool changed = true; changed;) {
            changed = false;
            for (int r = 0; r < nr; r++) for (int x : refs[r]) {
                if (G.rules[r].pure && !G.rules[x].pure) { G.rules[r].pure = false; changed = true; }
                if (!G.rules[r].stops && G.rules[x].stops) { G.rules[r].stops = true; changed = true; }
            }
        }
        // the words of each alternative's own text (always written when it is drawn), four letters or more, in few alternatives
        static const std::set<std::string> COMMON = {"that", "this", "with", "from", "have", "were", "they", "their", "there", "them", "then", "than", "what",
            "when", "where", "which", "will", "would", "your", "yours", "been", "into", "only", "over", "some", "more", "most", "much", "very", "just", "like",
            "each", "every", "other", "after", "before", "about", "again", "still", "even", "also", "here", "said", "says", "know", "knew", "come", "came",
            "went", "goes", "gone", "made", "make", "take", "took", "give", "gave", "once", "ever", "never", "nothing", "something", "everyone", "nobody",
            "anyone", "someone", "these", "those", "whose", "until", "while", "because", "though", "through", "upon", "under", "back", "down", "well",
            "year", "years", "days", "time", "first", "last", "long", "good", "great", "little", "many", "left", "right", "same", "such", "both", "keep"};
        std::unordered_map<std::string, int> df;
        std::vector<std::vector<std::string>> altWords(G.alts.size());
        for (size_t ai = 0; ai < G.alts.size(); ai++) {
            std::set<std::string> ws;
            const bool apart = G.alts[ai].apart;   // D-05: its words its own, counted for none (the others' distinctive words stay theirs)
            for (const VgNode& n : G.alts[ai].body) {
                if (n.type != VgNode::TEXT) continue;
                std::string w;
                for (size_t i = 0; i <= n.s.size(); i++) {
                    char c = i < n.s.size() ? n.s[i] : ' ';
                    if (std::isalpha((unsigned char)c)) { w += (char)std::tolower((unsigned char)c); continue; }
                    if (w.size() >= 4 && !COMMON.count(w)) ws.insert(w);
                    w.clear();
                }
            }
            for (const std::string& w : ws) { if (!apart) df[w]++; altWords[ai].push_back(w); }
        }
        G.words.clear();
        std::unordered_map<std::string, int> idx;
        for (size_t ai = 0; ai < G.alts.size(); ai++) {
            G.alts[ai].words.clear();
            for (const std::string& w : altWords[ai]) {
                if (df[w] > VG_DISTINCT) continue;
                auto it = idx.find(w);
                int id = it != idx.end() ? it->second : (int)G.words.size();
                if (it == idx.end()) { idx[w] = id; G.words.push_back(w); }
                G.alts[ai].words.push_back(id);
            }
        }
    }
    return G.errors.size() == errs;
}
