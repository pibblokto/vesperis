#!/usr/bin/env python3
"""tools/contents.py (E-04): the contents block of each reference note, made from its headings.

    tools/contents.py [--check] [file.md ...]

A note's block stands before its first `## ` heading, between `<!-- contents -->` and `<!-- /contents -->`: a `Contents:`
line, then the note's `## ` headings in order, a `### ` one indented under its `## `. Without files: every note under
docs/reference/ that has a `## ` heading, but the generated pages (harness/INDEX.md, harness/unit-checks.md,
civilisations/grammar-files.md) and the README.
`--check` changes nothing and exits 1 when a block is missing or stale (`make check` runs it).
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(ROOT, 'docs', 'reference')
SKIP = {'harness/INDEX.md', 'harness/unit-checks.md', 'civilisations/grammar-files.md', 'README.md'}
OPEN, CLOSE = '<!-- contents -->', '<!-- /contents -->'


def notes():
    for d, _, names in os.walk(REF):
        for n in sorted(names):
            p = os.path.join(d, n)
            if n.endswith('.md') and os.path.relpath(p, REF) not in SKIP:
                yield p


def headings(lines):
    out, fence = [], False
    for l in lines:
        if re.match(r'^\s*(```|~~~)', l):
            fence = not fence
        if fence:
            continue
        if l.startswith('## '):
            out.append('- ' + l[3:].strip())
        elif l.startswith('### '):
            out.append('  - ' + l[4:].strip())
    return out


def with_block(text):
    lines = text.split('\n')
    if OPEN in lines:
        a = lines.index(OPEN)
        b = lines.index(CLOSE, a)
        del lines[a:b + 1]
        if a < len(lines) and lines[a] == '':
            del lines[a]
    heads = headings(lines)
    if not heads:
        return text
    first = next(i for i, l in enumerate(lines) if l.startswith('## '))
    block = [OPEN, 'Contents:'] + heads + [CLOSE, '']
    lines[first:first] = block
    return '\n'.join(lines)


def main():
    args = [a for a in sys.argv[1:] if a != '--check']
    check = '--check' in sys.argv[1:]
    files = args or list(notes())
    stale = []
    for p in files:
        text = open(p, encoding='utf-8').read()
        new = with_block(text)
        if new == text:
            continue
        rel = os.path.relpath(p, ROOT)
        if check:
            stale.append('%s: its contents block is missing or stale (tools/contents.py writes it)' % rel)
        else:
            open(p, 'w', encoding='utf-8').write(new)
            print('%s: contents written' % rel)
    for s in stale:
        print(s)
    return 1 if stale else 0


if __name__ == '__main__':
    sys.exit(main())
