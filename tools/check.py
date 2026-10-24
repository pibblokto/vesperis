#!/usr/bin/env python3
"""tools/check.py, run by `make check` (E-02): what the tree's layout promises, checked.

- No raylib include outside src/platform_raylib.cpp: src/platform.h is the boundary.
- The layering: src/core includes only src/core; src/galaxy, src/space and src/surface include nothing of src/game, nor platform.h.
- No line in src over 2,000 characters (the Read tool cuts such a line).
- Every .cpp and .h is named in its directory's CLAUDE.md (src/'s own files in the root CLAUDE.md), by its name without the
  extension, so that the maps stay true.

Prints each problem and exits 1 on any.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src')
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
RAYLIB = re.compile(r'^\s*#\s*include\s*[<"](raylib|rlgl|raymath)\.h[>"]')
LOWER = {'core': {'core'}, 'galaxy': None, 'space': None, 'surface': None}   # None: anything but game/ and platform.h
LONG = 2000


def sources():
    for d, _, names in os.walk(SRC):
        for n in sorted(names):
            if n.endswith(('.cpp', '.h')):
                yield os.path.join(d, n)


def resolve(path, inc):
    for base in (os.path.dirname(path), SRC):
        p = os.path.normpath(os.path.join(base, inc))
        if os.path.exists(p):
            return p
    return None


def main():
    problems = []
    notes = {}
    for path in sources():
        rel = os.path.relpath(path, SRC)
        top = rel.split(os.sep)[0] if os.sep in rel else ''
        with open(path, encoding='utf-8', errors='replace') as f:
            lines = f.read().split('\n')
        for i, line in enumerate(lines, 1):
            if len(line) > LONG:
                problems.append('src/%s:%d: a line of %d characters (over %d)' % (rel, i, len(line), LONG))
            if RAYLIB.match(line) and rel != 'platform_raylib.cpp':
                problems.append('src/%s:%d: includes raylib (only platform_raylib.cpp may)' % (rel, i))
            m = INCLUDE.match(line)
            if not m or top not in LOWER:
                continue
            target = resolve(path, m.group(1))
            if target is None:
                continue
            trel = os.path.relpath(target, SRC)
            tdir = trel.split(os.sep)[0] if os.sep in trel else ''
            allowed = LOWER[top]
            if (allowed is not None and tdir not in allowed) or (allowed is None and (tdir == 'game' or trel == 'platform.h')):
                problems.append('src/%s:%d: includes %s (src/%s may not)' % (rel, i, trel, top))
        note = os.path.join(os.path.dirname(path), 'CLAUDE.md') if top else os.path.join(ROOT, 'CLAUDE.md')
        if note not in notes:
            notes[note] = open(note, encoding='utf-8').read() if os.path.exists(note) else None
        stem = os.path.splitext(os.path.basename(path))[0]
        if notes[note] is None:
            problems.append('src/%s: no %s to name it' % (rel, os.path.relpath(note, ROOT)))
        elif not re.search(r'(?<![\w.])' + re.escape(stem) + r'(?!\w)', notes[note]):
            problems.append('src/%s: not named in %s' % (rel, os.path.relpath(note, ROOT)))
    for p in problems:
        print(p)
    print('make check: %s' % ('%d problems' % len(problems) if problems else 'the tree keeps its layout'))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
