#!/usr/bin/env python3
"""E-03: move whole top-level entities of a C++ file into other files, and prove that a move moved and nothing else.

  tools/mover.py list <file> [min-lines] [--flat] the file's top-level entities (with --flat, those inside namespaces
                                                  too): first line, lines, kind, name
  tools/mover.py move <manifest> [--dry]          apply a manifest; writes the step's record beside it (<manifest>.record)
  tools/mover.py verify <manifest>                reassemble the source as it was from the files the step wrote, byte for byte
  tools/mover.py xref <manifest>                  the files a manifest makes, their lines, and what each file uses of another's

A chunk is one entity with the comments and blank lines above it and the rest of its last line, so the chunks of a file,
joined, are the file. An entity is a function, a type, a variable or table, a declaration, a preprocessor line, or a whole
`namespace { ... }` block. The lexer knows comments, strings, characters and raw strings.

A manifest:
  source src/harness/main.cpp                     the file the chunks leave (it keeps the rest, in order)
  to src/harness/modes_galaxy.cpp                 where the next lines' chunks go (created with the prologue below if new;
                                                  appended to, after its last line, if it exists)
  prologue // E-03: the galaxy's modes            the new file's first lines, before the chunks (repeatable; \\n-free)
  prologue #include "harness/harness.h"
  keep #include "harness/harness.h"               (anywhere) a line put at the top of the source, before what it keeps
  runStars                                        one entity by name (`name@line` for one of several of the name, `@line`
                                                  for the entity whose code begins on that line)
  runSectors .. runFoo                            a run of entities in the file's order, both ends included
  # a comment                                     ('#' then a space; `#include <x>` names a preprocessor line)

verify takes the step's record (the source's text before the step, each target's text before it, the prologues) and checks:
every target is its old text, then its prologue if new, then chunks; the source is its `keep` lines, then chunks; the source's
remaining chunks and the targets' new chunks, merged in the source's old order, are the old source exactly.

E-07: a manifest whose first line is `flat` opens the anonymous namespaces: its entities are the ones inside a
`namespace { ... }` block too, each selected by name like any other, and the block's own lines are wrappers:
  open                                            (in a target's list) an anonymous namespace opens here ("namespace {")
  close                                           (in a target's list) and closes here ("}   // namespace")
  drop @30                                        (anywhere) the source loses a wrapper line of its own (a block emptied)
An entity selected without an `open` round it lands outside any namespace (a helper made global, or put in a header).
Its verify reads every file's entities the same way and sets the anonymous namespaces' open and close lines aside: the
entities, merged in the old order, are the old source's exactly; the wrappers in each file are balanced; and it lists the
entities that left an anonymous namespace or entered one.
E-09: a namespace line or its closing brace with comments above it (a section's banner) is a wrapper too, selected by its
line (`@138`), so a target's `close` or `open` can pair with it; its comments are an entity of their own, checked in order.
"""
import hashlib
import json
import os
import re
import sys

IDENT = re.compile(r'[A-Za-z_]\w*')


def mask(src):
    """The text with comments blanked (newlines kept), string and character contents blanked, and preprocessor lines turned
    to '#'. Returns (masked, incomment): incomment[i] is True inside a comment, a string or a character literal."""
    out = list(src)
    inside = [False] * len(src)
    n = len(src)
    i = 0
    line_start = True

    def blank(a, b):
        for k in range(a, b):
            inside[k] = True
            if out[k] != '\n':
                out[k] = ' '

    while i < n:
        c = src[i]
        if c == '\n':
            line_start = True
            i += 1
            continue
        if c in ' \t\r\f\v':
            i += 1
            continue
        if line_start and c == '#':
            j = i
            while j < n and src[j] != '\n':
                if src[j] == '\\' and j + 1 < n and src[j + 1] == '\n':
                    j += 2
                    continue
                if src.startswith('/*', j):
                    e = src.find('*/', j + 2)
                    j = n if e < 0 else e + 2
                    continue
                j += 1
            for k in range(i, j):
                if out[k] != '\n':
                    out[k] = '#'
            i = j
            continue
        line_start = False
        if src.startswith('//', i):
            j = src.find('\n', i)
            j = n if j < 0 else j
            if src[j - 1] == '\\':
                raise SystemExit('a // comment continued by a backslash at offset %d' % i)
            blank(i, j)
            i = j
            continue
        if src.startswith('/*', i):
            j = src.find('*/', i + 2)
            j = n if j < 0 else j + 2
            blank(i, j)
            i = j
            continue
        m = re.match(r'(u8|u|U|L)?R"([^ ()\\\t\n]{0,16})\(', src[i:i + 40])
        if m and (i == 0 or not (src[i - 1].isalnum() or src[i - 1] == '_')):
            end = src.find(')' + m.group(2) + '"', i + m.end())
            if end < 0:
                raise SystemExit('an unterminated raw string at offset %d' % i)
            blank(i + m.end(), end)
            i = end + len(m.group(2)) + 2
            continue
        if c == '"' or c == "'":
            if c == "'" and i > 0 and (src[i - 1].isalnum() or src[i - 1] == '_'):
                # a digit separator (1'000) or a character literal's prefix (u8'a', L'a')
                k = i - 1
                while k > 0 and (src[k - 1].isalnum() or src[k - 1] in '_.'):
                    k -= 1
                if src[k].isdigit():
                    i += 1
                    continue
            j = i + 1
            while j < n and src[j] != c:
                if src[j] == '\\':
                    j += 2
                    continue
                if src[j] == '\n':
                    raise SystemExit('a newline inside a literal at offset %d' % i)
                j += 1
            blank(i + 1, j)
            i = j + 1
            continue
        if c.isalnum() or c == '_':
            j = i
            while j < n and (src[j].isalnum() or src[j] == '_'):
                j += 1
            i = j
            continue
        i += 1
    return ''.join(out), inside


class Chunk:
    def __init__(self, start, end, kind, name, first_line, last_line, ns):
        self.start, self.end, self.kind, self.name = start, end, kind, name
        self.first_line, self.last_line, self.ns = first_line, last_line, ns

    def text(self, src):
        return src[self.start:self.end]


def flat(h):
    """The head with its parenthesised and bracketed parts emptied (default arguments, array sizes)."""
    out, depth = [], 0
    for ch in h:
        if ch in '([':
            depth += 1
            if depth == 1:
                out.append(ch)
        elif ch in ')]':
            depth -= 1
            if depth == 0:
                out.append(ch)
        elif depth == 0:
            out.append(ch)
    return ''.join(out)


def head_name(head, kind):
    """The name an entity is called by, from its masked head (the text before its body or its ';')."""
    h = re.sub(r'\s+', ' ', head).strip()
    h = re.sub(r'^template ?<.*?> ?(?=[A-Za-z_])', '', h)
    if kind == 'pp':
        return h
    if kind == 'type':
        m = re.match(r'(?:typedef )?(?:struct|class|union|enum(?: class| struct)?) (?:alignas ?\([^)]*\) )?([A-Za-z_]\w*)', h)
        return m.group(1) if m else h[:40]
    if kind == 'ns-open':
        return h.rstrip('{ ').strip()
    if kind in ('using', 'typedef'):
        ids = IDENT.findall(h.split('=')[0])
        return ids[-1] if ids else h[:40]
    # functions and declarations: the name before the first top-level '('; variables: the last name before '=', '[', '{' or ';'
    depth = 0
    for k, ch in enumerate(h):
        if ch in '<':
            depth += 1
        elif ch == '>' and depth:
            depth -= 1
        elif ch == '(' and depth == 0:
            before = h[:k].rstrip()
            m = re.search(r'(operator\s*(?:\(\)|[^\w\s(]+|\w+)|~?[A-Za-z_]\w*(?:::~?[A-Za-z_]\w*)*)$', before)
            if m and kind in ('func', 'decl'):
                return m.group(1).replace(' ', '')
            break
        elif ch in '=[{;' and depth == 0:
            ids = IDENT.findall(h[:k])
            return ids[-1] if ids else h[:40]
    ids = IDENT.findall(h)
    return ids[-1] if ids else h[:40]


def chunks(src):
    m, inside = mask(src)
    n = len(m)
    line_of = []
    ln = 1
    for ch in src:
        line_of.append(ln)
        if ch == '\n':
            ln += 1
    line_of.append(ln)

    res = []
    pos = 0          # where the current chunk began (the previous one's end)
    ns = []          # the namespaces we are in (their heads)
    i = 0

    def close_at(e, kind, name, ent_start):
        """End the chunk after index e (the entity's last character) and the rest of its line, if only blanks or comments."""
        nonlocal pos
        j = e + 1
        while j < n and m[j] in ' \t\r\f\v':
            j += 1
        if j < n and m[j] == '\n':
            if inside[j - 1] and j - 1 > e and src[j - 1] != '\n':
                # a comment runs on to this line's end: it is part of this chunk, unless it is a block comment going on
                pass
            end = j + 1
        elif j >= n:
            end = n
        else:
            end = e + 1
        if inside[end - 1] and end < n and inside[end] and src[end - 1] != '\n':
            raise SystemExit('a chunk would end inside a comment at line %d' % line_of[end])
        res.append(Chunk(pos, end, kind, name, line_of[ent_start], line_of[e], tuple(ns)))
        pos = end

    while i < n:
        c = m[i]
        if c in ' \t\r\f\v\n':
            i += 1
            continue
        if c == '#':
            j = i
            while j < n and m[j] == '#':
                j += 1
            # a directive's '#' run ends at its line's end (continuations are '#' too, and their newlines kept)
            while j < n and m[j] != '\n':
                j += 1
            k = i
            while k < j and m[k] == '#':
                k += 1
            close_at(k - 1, 'pp', head_name(src[i:k], 'pp'), i)
            i = pos
            continue
        if c == '}' and ns:
            close_at(i, 'ns-close', '}', i)
            ns.pop()
            i = pos
            continue
        # an entity: scan to its end
        ent_start = i
        depth = paren = 0
        head_end = None
        j = i
        while j < n:
            ch = m[j]
            if ch in '([':
                paren += 1
            elif ch in ')]':
                paren -= 1
            elif ch == '{':
                if depth == 0 and paren == 0 and head_end is None:
                    head_end = j
                    head = m[ent_start:j]
                    if re.match(r'\s*(inline\s+)?namespace\b', head) or re.match(r'\s*extern\s+"', src[ent_start:j]):
                        break
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0 and paren == 0:
                    head = m[ent_start:head_end]
                    h = re.sub(r'\s+', ' ', head).strip()
                    hh = re.sub(r'^template ?<.*?> ?(?=[A-Za-z_])', '', h)
                    is_init = re.search(r'(?<![=!<>+\-*/%&|^])=(?!=)', re.sub(r'operator\s*\S+', '', flat(hh))) is not None
                    is_type = re.match(r'(?:(?:static|inline|constexpr|typedef|const|thread_local)\s+)*(struct|class|union|enum)\b', hh)
                    k = j + 1
                    while k < n and m[k] in ' \t\r\f\v\n':
                        k += 1
                    nxt = m[k] if k < n else ''
                    if is_init or is_type or not head.strip():
                        pass   # go on to the ';'
                    elif nxt and nxt in ',{':
                        pass   # a constructor's member initialiser
                    else:
                        kind = 'func'
                        close_at(j, kind, head_name(head, kind), ent_start)
                        break
            elif ch == ';' and depth == 0 and paren == 0:
                head = m[ent_start:j] if head_end is None else m[ent_start:head_end]
                h = re.sub(r'\s+', ' ', head).strip()
                hh = re.sub(r'^template ?<.*?> ?(?=[A-Za-z_])', '', h)
                if head_end is not None and re.match(r'(?:(?:static|inline|constexpr|typedef|const)\s+)*(struct|class|union|enum)\b', hh):
                    kind = 'type'
                elif re.match(r'using\b', hh):
                    kind = 'using'
                elif re.match(r'typedef\b', hh):
                    kind = 'typedef'
                elif re.match(r'static_assert\b', hh):
                    kind = 'assert'
                elif head_end is None and re.match(r'(?:struct|class|union|enum)\b(?:\s+class)?\s+[A-Za-z_]\w*$', hh):
                    kind = 'type'   # a forward declaration of a type
                elif head_end is None and '(' in hh and not re.search(r'(?<![=!<>])=(?!=)', hh.split('(')[0]):
                    kind = 'decl'
                else:
                    kind = 'var'
                close_at(j, kind, head_name(head, kind), ent_start)
                break
            j += 1
        else:
            raise SystemExit('an entity runs off the end of the file from line %d' % line_of[ent_start])
        if head_end is not None and j == head_end:
            close_at(j, 'ns-open', head_name(m[ent_start:j], 'ns-open'), ent_start)
            ns.append(m[ent_start:j].strip())
        i = pos
    if pos < n:
        res.append(Chunk(pos, n, 'tail', '', line_of[pos], line_of[n - 1], tuple(ns)))
    assert ''.join(c.text(src) for c in res) == src
    return res


def group_namespaces(cs):
    """Each top-level `namespace { ... }` block as one chunk (its inner chunks kept in .inner)."""
    out = []
    stack = []
    for c in cs:
        if c.kind == 'ns-open':
            stack.append([c])
        elif c.kind == 'ns-close':
            block = stack.pop()
            block.append(c)
            whole = Chunk(block[0].start, c.end, 'namespace', block[0].name or 'namespace', block[0].first_line, c.last_line, ())
            whole.inner = block
            if stack:
                stack[-1].append(whole)
            else:
                out.append(whole)
        elif stack:
            stack[-1].append(c)
        else:
            out.append(c)
    assert not stack
    return out


def top_chunks(src):
    return group_namespaces(chunks(src))


WRAP_OPEN = '\nnamespace {\n'
WRAP_CLOSE = '}   // namespace\n'


def wrappers(cs, src):
    """The indices of the chunks that open or close an anonymous namespace (`namespace {`, or its `}` with or without a
    `// namespace`, with blank lines or `//` comments above it); a named namespace's lines are entities."""
    out, stack = set(), []
    for k, c in enumerate(cs):
        if c.kind == 'ns-open':
            anon = re.fullmatch(r'(?:[ \t]*//[^\n]*\n|\n)*namespace \{\n', c.text(src)) is not None
            stack.append((k, anon))
        elif c.kind == 'ns-close':
            if not stack:
                raise SystemExit('a namespace closes at line %d that never opened' % c.first_line)
            ko, anon = stack.pop()
            if anon:
                if not re.fullmatch(r'(?:[ \t]*//[^\n]*\n|\n)*\}(?:[ \t]*// namespace)?\n', c.text(src)):
                    raise SystemExit('the anonymous namespace of line %d closes with more than its "}" at line %d'
                                     % (cs[ko].first_line, c.first_line))
                out |= {ko, k}
    if stack:
        raise SystemExit('a namespace opened at line %d never closes' % cs[stack[-1][0]].first_line)
    return out


def anon_depth(c):
    return sum(1 for h in c.ns if h == 'namespace')


def cmd_list(path, min_lines=0, opened=False):
    src = open(path).read()
    for c in (chunks(src) if opened else top_chunks(src)):
        lines = c.last_line - c.first_line + 1
        if lines >= min_lines:
            print('%6d %5d %-9s %s%s' % (c.first_line, lines, c.kind, '  ' * len(c.ns), c.name[:100]))


def defined_names(c, src, m):
    """The names an entity defines: a function's or a type's, each declarator's of a variable."""
    if c.kind in ('func', 'decl', 'type', 'using', 'typedef'):
        return {c.name.split('::')[-1]}
    if c.kind == 'var':
        # the declarators: the text with every bracketed part emptied, split at its top-level commas, each part up to its '='
        text = m[c.start:c.end]
        out, depth = [], 0
        for ch in text:
            if ch in '([{':
                depth += 1
            elif ch in ')]}':
                depth -= 1
            elif depth == 0:
                out.append(ch)
        names = set()
        for pt in ''.join(out).rstrip().rstrip(';').split(','):
            pt = pt.split('=')[0]
            ids = IDENT.findall(pt)
            if ids:
                names.add(ids[-1])
        return names
    if c.kind == 'namespace':
        out = set()
        for ic in c.inner[1:-1]:
            out |= defined_names(ic, src, m)
        return out
    return set()


def xref(src, opened=False):
    """For each top-level chunk (each entity, namespaces opened, when `opened`): the names it defines and the other chunks'
    names it uses (lexically, an over-estimate)."""
    m, _ = mask(src)
    cs = chunks(src) if opened else top_chunks(src)
    defs = [defined_names(c, src, m) for c in cs]
    owner = {}
    for k, d in enumerate(defs):
        for nm in d:
            owner.setdefault(nm, []).append(k)
    uses = []
    for k, c in enumerate(cs):
        text = m[c.start:c.end]
        ids = set()
        for mm in re.finditer(r'[A-Za-z_]\w*', text):
            a = mm.start()
            if a and (text[a - 1].isalnum() or text[a - 1] == '_'):
                continue
            j = a - 1
            while j >= 0 and text[j] in ' \t\n':
                j -= 1
            if j >= 0 and (text[j] == '.' or text[j - 1:j + 1] in ('->', '::')):
                continue
            ids.add(mm.group(0))
        uses.append({nm for nm in ids if nm in owner})
    return cs, defs, owner, uses


def layout_of(manifest, cs):
    st = parse_manifest(manifest)
    where = {}
    for t in st['targets']:
        for sel in t['select']:
            if st['flat'] and sel in ('open', 'close'):
                continue
            for k in resolve(cs, sel, manifest):
                if k in where:
                    raise SystemExit('%s: line %d (%s) is placed twice' % (manifest, cs[k].first_line, cs[k].name))
                where[k] = t['path']
    return st, where


def cmd_xref(manifest, verbose=False):
    st = parse_manifest(manifest)
    src = open(st['source']).read()
    cs, defs, owner, uses = xref(src, st['flat'])
    _, where = layout_of(manifest, cs)
    home = lambda k: where.get(k, st['source'])
    sizes = {}
    for k, c in enumerate(cs):
        sizes[home(k)] = sizes.get(home(k), 0) + c.text(src).count('\n')
    print('the files:')
    for f in sorted(sizes, key=lambda f: -sizes[f]):
        print('  %6d  %s' % (sizes[f], f))
    shared = {}
    for k, c in enumerate(cs):
        for nm in uses[k]:
            for d in owner[nm]:
                if d != k and home(d) != home(k) and cs[d].kind not in ('decl',):
                    shared.setdefault(d, set()).add(home(k))
    print('used from another file (%d):' % len(shared))
    for d in sorted(shared):
        c = cs[d]
        print('  %-9s %-34s %-28s <- %s' % (c.kind, c.name[:34], os.path.basename(home(d)), ' '.join(sorted(os.path.basename(f) for f in shared[d]))[:150]))


# ---- manifests

def parse_manifest(path):
    steps = {'source': None, 'targets': [], 'flat': False, 'drop': []}
    cur = None
    for raw in open(path):
        line = raw.rstrip('\n')
        if not line.strip() or re.match(r'\s*#(?![A-Za-z])', line):
            continue   # a comment ('#' then a space); '#include ...' names a preprocessor line
        word, _, rest = line.strip().partition(' ')
        if word == 'flat' and not rest.strip():
            if steps['source'] or steps['targets']:
                raise SystemExit('%s: `flat` is the first line' % path)
            steps['flat'] = True
        elif word == 'drop':
            steps['drop'].append(rest.strip())
        elif word == 'source':
            steps['source'] = rest.strip()
        elif word == 'keep':
            steps.setdefault('keep', []).append(rest)
        elif word == 'to':
            cur = {'path': rest.strip(), 'prologue': [], 'select': []}
            steps['targets'].append(cur)
        elif word == 'prologue':
            cur['prologue'].append(rest)
        else:
            sel = line.strip() if line.strip().startswith('#') else re.sub(r'\s+#\s.*$', '', line).strip()
            cur['select'].append(sel)
    return steps


def resolve(cs, sel, where):
    def find(name):
        nm, _, at = name.rpartition('@') if re.search(r'@\d+$', name) else (name, '', '')
        hits = [k for k, c in enumerate(cs) if (c.name == nm or not nm) and (not at or c.first_line == int(at))]
        if len(hits) != 1:
            raise SystemExit('%s: "%s" names %d entities%s' % (where, name, len(hits),
                             (' (at lines ' + ', '.join(str(cs[k].first_line) for k in hits) + ')') if hits else ''))
        return hits[0]
    if ' .. ' in sel:
        a, b = (s.strip() for s in sel.split(' .. '))
        ia, ib = find(a), find(b)
        if ib < ia:
            raise SystemExit('%s: the run "%s" goes backwards' % (where, sel))
        return list(range(ia, ib + 1))
    return [find(sel)]


def sha(s):
    return hashlib.sha256(s.encode()).hexdigest()[:16]


def cmd_move_flat(manifest, st, dry):
    """A `flat` manifest's move: the entities inside anonymous namespaces selectable, `open` and `close` written as asked."""
    src_path = st['source']
    src = open(src_path).read()
    cs = chunks(src)
    wr = wrappers(cs, src)
    taken = {}
    plans = []
    for t in st['targets']:
        items = []
        for sel in t['select']:
            if sel in ('open', 'close'):
                items.append(WRAP_OPEN if sel == 'open' else WRAP_CLOSE)
                continue
            for k in resolve(cs, sel, manifest):
                if k in taken:
                    raise SystemExit('%s: line %d (%s) is taken twice' % (manifest, cs[k].first_line, cs[k].name))
                taken[k] = t['path']
                items.append(k)
        ks = [x for x in items if isinstance(x, int)]
        if ks != sorted(ks):
            raise SystemExit('%s: %s takes its entities out of the source\'s order' % (manifest, t['path']))
        plans.append((t, items))
    dropped = set()
    for sel in st['drop']:
        for k in resolve(cs, sel, manifest):
            if k not in wr or k in taken:
                raise SystemExit('%s: drop %s names line %d (%s), not a wrapper the source keeps' % (manifest, sel, cs[k].first_line, cs[k].kind))
            dropped.add(k)
    record = {'flat': True, 'source': src_path, 'source_before': src, 'targets': []}
    writes = {}
    for t, items in plans:
        before = open(t['path']).read() if os.path.exists(t['path']) else None
        if before is not None and t['prologue']:
            raise SystemExit('%s: %s exists, so it takes no prologue' % (manifest, t['path']))
        if before is not None and before and not before.endswith('\n'):
            raise SystemExit('%s does not end with a newline' % t['path'])
        pro = ''.join(p + '\n' for p in t['prologue'])
        body = ''.join(cs[x].text(src) if isinstance(x, int) else x for x in items)
        writes[t['path']] = (before or '') + (pro if before is None else '') + body
        n = sum(1 for x in items if isinstance(x, int) and x not in wr)
        record['targets'].append({'path': t['path'], 'before': before, 'prologue': pro, 'chunks': n})
        print('%-44s %4d entities, %6d lines%s' % (t['path'], n, body.count('\n'), '' if before is not None else ' (new)'))
    keep = ''.join(p + '\n' for p in st.get('keep', []))
    rest = keep + ''.join(c.text(src) for k, c in enumerate(cs) if k not in taken and k not in dropped)
    writes[src_path] = rest
    record['keep'] = keep
    kept = sum(1 for k in range(len(cs)) if k not in taken and k not in dropped and k not in wr)
    print('%-44s keeps %d entities, %d lines' % (src_path, kept, rest.count('\n')))
    if dry:
        return
    for p, s in writes.items():
        os.makedirs(os.path.dirname(p) or '.', exist_ok=True)
        open(p, 'w').write(s)
    record['after'] = {p: sha(s) for p, s in writes.items()}
    json.dump(record, open(manifest + '.record', 'w'))
    verify(manifest)


def flat_entities(cs, text, w):
    """A file's entities for a flat verify: every chunk but the wrappers, and a wrapper's comments above its namespace line
    (E-09: a section's banner), each with its chunk."""
    out = []
    for k, c in enumerate(cs):
        t = c.text(text)
        if k in w:
            head = t[:t.rstrip('\n').rfind('\n') + 1]   # all but its namespace line or its brace
            if head.strip():
                out.append((head, c))
            continue
        out.append((t, c))
    return out


def verify_flat(rec):
    """A `flat` move's proof: every file's entities, the anonymous namespaces' lines set aside, merged in the old order are
    the old source's entities, byte for byte; each file's wrappers balance; the entities whose namespace changed, listed."""
    old = rec['source_before']
    old_cs = chunks(old)
    old_w = wrappers(old_cs, old)
    queues = {}

    def load(path, text):
        cs = chunks(text)
        w = wrappers(cs, text)
        for k in sorted(w):
            if cs[k].kind == 'ns-open' and k + 1 in w and cs[k + 1].kind == 'ns-close':
                print('verify: note: %s has an empty anonymous namespace at its line %d' % (path, cs[k].first_line))
        queues[path] = [(t, anon_depth(c)) for t, c in flat_entities(cs, text, w)]
    for t in rec['targets']:
        now = open(t['path']).read()
        lead = (t['before'] or '') + (t['prologue'] if t['before'] is None else '')
        if not now.startswith(lead):
            raise SystemExit('verify: %s does not begin with its old text and prologue' % t['path'])
        load(t['path'], now[len(lead):])
    src_now = open(rec['source']).read()
    keep = rec.get('keep', '')
    if not src_now.startswith(keep):
        raise SystemExit('verify: %s does not begin with its new lines' % rec['source'])
    load(rec['source'], src_now[len(keep):])
    heads = {p: 0 for p in queues}
    left, entered, n = [], [], 0
    for want, c in flat_entities(old_cs, old, old_w):
        hit = [p for p in queues if heads[p] < len(queues[p]) and queues[p][heads[p]][0] == want]
        if not hit:
            raise SystemExit('verify: the old line %d (%s %s) is in no file at its place' % (c.first_line, c.kind, c.name))
        p = hit[0]
        was, now_d = anon_depth(c), queues[p][heads[p]][1]
        heads[p] += 1
        n += 1
        if was and not now_d:
            left.append(c.name)
        elif now_d and not was:
            entered.append(c.name)
    for p in queues:
        if heads[p] != len(queues[p]):
            raise SystemExit('verify: %s holds %d entities that were not in the source' % (p, len(queues[p]) - heads[p]))
    print('verify: the old %s\'s %d entities (%d lines) in %d files, byte for byte and in their order; the anonymous '
          'namespaces balanced' % (rec['source'], n, old.count('\n'), len(queues)))
    if left:
        print('  left an anonymous namespace (%d): %s' % (len(left), ' '.join(left)))
    if entered:
        print('  entered one (%d): %s' % (len(entered), ' '.join(entered)))


def cmd_move(manifest, dry=False):
    st = parse_manifest(manifest)
    if st['flat']:
        return cmd_move_flat(manifest, st, dry)
    if st['drop'] or any(s in ('open', 'close') for t in st['targets'] for s in t['select']):
        raise SystemExit('%s: open, close and drop are a flat manifest\'s' % manifest)
    src_path = st['source']
    src = open(src_path).read()
    cs = top_chunks(src)
    taken = {}
    for t in st['targets']:
        for sel in t['select']:
            for k in resolve(cs, sel, manifest):
                if k in taken:
                    raise SystemExit('%s: line %d (%s) is taken twice' % (manifest, cs[k].first_line, cs[k].name))
                taken[k] = t['path']
    record = {'source': src_path, 'source_before': src, 'targets': []}
    writes = {}
    for t in st['targets']:
        mine = sorted(k for k, p in taken.items() if p == t['path'])
        before = open(t['path']).read() if os.path.exists(t['path']) else None
        if before is not None and t['prologue']:
            raise SystemExit('%s: %s exists, so it takes no prologue' % (manifest, t['path']))
        pro = ''.join(p + '\n' for p in t['prologue'])
        body = ''.join(cs[k].text(src) for k in mine)
        if before is not None and before and not before.endswith('\n'):
            raise SystemExit('%s does not end with a newline' % t['path'])
        writes[t['path']] = (before or '') + (pro if before is None else '') + body
        record['targets'].append({'path': t['path'], 'before': before, 'prologue': pro, 'chunks': len(mine)})
        print('%-44s %4d entities, %6d lines%s' % (t['path'], len(mine), body.count('\n'), '' if before is not None else ' (new)'))
    keep = ''.join(p + '\n' for p in st.get('keep', []))
    rest = keep + ''.join(c.text(src) for k, c in enumerate(cs) if k not in taken)
    writes[src_path] = rest
    record['keep'] = keep
    print('%-44s keeps %d entities, %d lines' % (src_path, len(cs) - len(taken), rest.count('\n')))
    if dry:
        return
    for p, s in writes.items():
        os.makedirs(os.path.dirname(p) or '.', exist_ok=True)
        open(p, 'w').write(s)
    record['after'] = {p: sha(s) for p, s in writes.items()}
    json.dump(record, open(manifest + '.record', 'w'))
    verify(manifest)


def verify(manifest):
    rec = json.load(open(manifest + '.record'))
    if rec.get('flat'):
        return verify_flat(rec)
    old = rec['source_before']
    old_cs = top_chunks(old)
    queues = {}
    for t in rec['targets']:
        now = open(t['path']).read()
        lead = (t['before'] or '') + (t['prologue'] if t['before'] is None else '')
        if not now.startswith(lead):
            raise SystemExit('verify: %s does not begin with its old text and prologue' % t['path'])
        rest = now[len(lead):]
        q = top_chunks(rest)
        queues[t['path']] = [c.text(rest) for c in q]
    src_now = open(rec['source']).read()
    keep = rec.get('keep', '')
    if not src_now.startswith(keep):
        raise SystemExit('verify: %s does not begin with its new lines' % rec['source'])
    src_now = src_now[len(keep):]
    queues[rec['source']] = [c.text(src_now) for c in top_chunks(src_now)]
    heads = {p: 0 for p in queues}
    out = []
    for c in old_cs:
        want = c.text(old)
        hit = [p for p in queues if heads[p] < len(queues[p]) and queues[p][heads[p]] == want]
        if not hit:
            raise SystemExit('verify: the old line %d (%s %s) is in no file at its place' % (c.first_line, c.kind, c.name))
        p = hit[0]
        heads[p] += 1
        out.append(want)
    for p in queues:
        if heads[p] != len(queues[p]):
            raise SystemExit('verify: %s holds %d chunks that were not in the source' % (p, len(queues[p]) - heads[p]))
    if ''.join(out) != old:
        raise SystemExit('verify: the reassembled source differs')
    print('verify: the old %s (%d lines) reassembled from %d files, byte for byte' % (rec['source'], old.count('\n'), len(queues)))


if __name__ == '__main__':
    a = sys.argv[1:]
    if not a:
        print(__doc__)
    elif a[0] == 'list':
        opened = '--flat' in a
        a = [x for x in a if x != '--flat']
        cmd_list(a[1], int(a[2]) if len(a) > 2 else 0, opened)
    elif a[0] == 'move':
        cmd_move(a[1], '--dry' in a)
    elif a[0] == 'verify':
        verify(a[1])
    elif a[0] == 'xref':
        cmd_xref(a[1])
    else:
        raise SystemExit(__doc__)
