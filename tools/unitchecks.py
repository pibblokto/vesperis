#!/usr/bin/env python3
"""tools/unitchecks.py (E-04): docs/reference/harness/unit-checks.md, the unit's checks by group, read from the source.

    tools/unitchecks.py            write the page
    tools/unitchecks.py --check    exit 1 when the page differs from what the source gives (`make check` runs this)

The groups are `UNIT_GROUPS` (`src/harness/unit.cpp`) in the unit's order; a group's checks are the `check(...)` calls in
its function's body, in the order they stand there, by their name (the first argument: its literal, or the expression that
makes it). A check in a loop is one line here however often it runs; one that a helper calls is not seen.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HARNESS = os.path.join(ROOT, 'src', 'harness')
PAGE = os.path.join(ROOT, 'docs', 'reference', 'harness', 'unit-checks.md')
ROW = re.compile(r'\{"(\w+)", "([\w.]+)", (\d+), (\d+), (?:(\w+)|\[\]\(const Check& c\) \{ (\w+)\(c\); \})\}')


def code_only(src):
    """The source with comments, string and character literals' insides blanked (same length), so braces and calls can be
    found by position."""
    out, i, n = list(src), 0, len(src)
    while i < n:
        c = src[i]
        if src.startswith('//', i):
            j = src.find('\n', i)
            j = n if j == -1 else j
            for k in range(i, j):
                out[k] = ' '
            i = j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            j = n if j == -1 else j + 2
            for k in range(i, j):
                if out[k] != '\n':
                    out[k] = ' '
            i = j
        elif c == '"' or c == "'":
            if c == '"' and i > 0 and src[i - 1] == 'R':
                m = re.match(r'R"([^(]*)\(', src[i - 1:])
                end = src.find(')' + m.group(1) + '"', i)
                for k in range(i + 1, end + len(m.group(1)) + 1):
                    if out[k] != '\n':
                        out[k] = ' '
                i = end + len(m.group(1)) + 2
                continue
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == '\\' else 1
            for k in range(i + 1, j):
                out[k] = ' '
            i = j + 1
        else:
            i += 1
    return ''.join(out)


def body_of(src, code, fn):
    m = re.search(r'^(?:static\s+)?void\s+' + re.escape(fn) + r'\s*\(', code, re.M)
    assert m, fn
    i = code.index('{', m.end())
    depth = 0
    for j in range(i, len(code)):
        if code[j] == '{':
            depth += 1
        elif code[j] == '}':
            depth -= 1
            if depth == 0:
                return i, j
    raise AssertionError(fn)


def unquote(lits):
    out = ''
    for lit in lits:
        s = lit[1:-1]
        out += re.sub(r'\\(.)', lambda m: {'n': ' ', 't': ' '}.get(m.group(1), m.group(1)), s)
    return out


def checks(src, code, a, b):
    names = []
    for m in re.finditer(r'(?<![\w.>])check\s*\(', code[a:b]):
        i = a + m.end()
        depth, j = 0, i
        while j < b:
            ch = code[j]
            if ch in '([{':
                depth += 1
            elif ch in ')]}':
                if depth == 0:
                    break
                depth -= 1
            elif ch == ',' and depth == 0:
                break
            j += 1
        arg = src[i:j].strip()
        lits = re.findall(r'"(?:[^"\\]|\\.)*"', arg)
        if lits and re.fullmatch(r'(?:"(?:[^"\\]|\\.)*"\s*)+', arg):
            names.append(unquote(lits))
        else:
            names.append('(made by `%s`)' % ' '.join(arg.split()))
    return names


def page():
    unit = open(os.path.join(HARNESS, 'unit.cpp'), encoding='utf-8').read()
    rows = ROW.findall(unit[unit.index('UNIT_GROUPS[]'):])
    assert rows, 'no UNIT_GROUPS rows'
    out = ['# The unit\'s checks', '',
           'Written by `tools/unitchecks.py` from `UNIT_GROUPS` (`src/harness/unit.cpp`) and its groups\' functions: run it again',
           'rather than edit this page (`make check` compares). `vesperis_test unit list` names the groups;',
           '`vesperis_test unit <group> -q` runs one and prints its failures. A check in a loop is one line here.', '']
    total = 0
    cache = {}
    for name, fname, scale, mush, fn, fn2 in rows:
        fn = fn or fn2
        if fname not in cache:
            src = open(os.path.join(HARNESS, fname), encoding='utf-8').read()
            cache[fname] = (src, code_only(src))
        src, code = cache[fname]
        a, b = body_of(src, code, fn)
        names = checks(src, code, a, b)
        total += len(names)
        out.append('## %s (`%s`, `%s`, %dx): %d' % (name, fname, fn, int(scale), len(names)))
        out.append('')
        out += ['- %s' % n for n in names]
        out.append('')
    out.insert(6, '%d groups, %d checks in their order; a group\'s heading gives its file, its function, the render scale it' % (len(rows), total))
    out.insert(7, 'starts at and its count.')
    out.insert(8, '')
    return '\n'.join(out).rstrip('\n') + '\n'


def main():
    text = page()
    if '--check' in sys.argv[1:]:
        old = open(PAGE, encoding='utf-8').read() if os.path.exists(PAGE) else ''
        if old != text:
            print('%s differs from the source: run tools/unitchecks.py' % os.path.relpath(PAGE, ROOT))
            return 1
        return 0
    os.makedirs(os.path.dirname(PAGE), exist_ok=True)
    with open(PAGE, 'w', encoding='utf-8') as f:
        f.write(text)
    print('%s: %d lines' % (os.path.relpath(PAGE, ROOT), text.count('\n')))
    return 0


if __name__ == '__main__':
    sys.exit(main())
