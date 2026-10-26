#!/usr/bin/env python3
"""tools/check.py, run by `make check` (E-02): what the tree's layout promises, checked.

- No raylib include outside src/platform_raylib.cpp: src/platform.h is the boundary.
- The layering: src/core includes only src/core; src/galaxy (the worlds) only src/core and itself, but for the worlds' two
  readers of the peoples (E-09: landmarks.cpp, planet_bodygen.cpp); src/peoples only src/core, src/galaxy and itself;
  src/space and src/surface nothing of src/game, nor platform.h.
- No line in src over 2,000 characters (the Read tool cuts such a line).
- Every .cpp and .h is named in its directory's CLAUDE.md (src/'s own files in the root CLAUDE.md), by its name without the
  extension, so that the maps stay true.
- E-04: no line in docs/ over 2,000 characters either; the generated pages are what their sources give:
  docs/reference/harness/INDEX.md is `vesperis_test help md` (the Makefile builds the harness first),
  docs/reference/civilisations/grammar-files.md `vesperis_test grammar where md` (E-11) and
  docs/reference/harness/unit-checks.md `tools/unitchecks.py`'s page; every reference note's contents block is its
  headings (`tools/contents.py`).

Prints each problem and exits 1 on any.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src')
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
RAYLIB = re.compile(r'^\s*#\s*include\s*[<"](raylib|rlgl|raymath)\.h[>"]')
LOWER = {'core': {'core'}, 'galaxy': {'core', 'galaxy'}, 'peoples': {'core', 'galaxy', 'peoples'},
         'space': None, 'surface': None}   # None: anything but game/ and platform.h
WORLDS_READING_PEOPLES = {'galaxy/landmarks.cpp', 'galaxy/planet_bodygen.cpp'}   # E-09: the scanner's sights; a world's people
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


def docs_problems():
    problems = []
    for d, _, names in os.walk(os.path.join(ROOT, 'docs')):
        for n in sorted(names):
            if not n.endswith('.md'):
                continue
            path = os.path.join(d, n)
            with open(path, encoding='utf-8', errors='replace') as f:
                for i, line in enumerate(f.read().split('\n'), 1):
                    if len(line) > LONG:
                        problems.append('%s:%d: a line of %d characters (over %d; tools/docwrap.py wraps it)'
                                        % (os.path.relpath(path, ROOT), i, len(line), LONG))
    index = os.path.join(ROOT, 'docs', 'reference', 'harness', 'INDEX.md')
    binary = os.path.join(ROOT, 'vesperis_test')
    if not os.path.exists(binary):
        problems.append('vesperis_test is not built, so INDEX.md is not checked (make vesperis_test)')
    else:
        made = subprocess.run([binary, 'help', 'md'], cwd=ROOT, capture_output=True, text=True).stdout
        if not os.path.exists(index) or open(index, encoding='utf-8').read() != made:
            problems.append('docs/reference/harness/INDEX.md is not `vesperis_test help md`: '
                            './vesperis_test help md > docs/reference/harness/INDEX.md')
        # E-11: the grammar's files and rule prefixes, made from the tree
        page = os.path.join(ROOT, 'docs', 'reference', 'civilisations', 'grammar-files.md')
        made = subprocess.run([binary, 'grammar', 'where', 'md'], cwd=ROOT, capture_output=True, text=True).stdout
        if not os.path.exists(page) or open(page, encoding='utf-8').read() != made:
            problems.append('docs/reference/civilisations/grammar-files.md is not `vesperis_test grammar where md`: '
                            './vesperis_test grammar where md > docs/reference/civilisations/grammar-files.md')
    for tool in ('unitchecks.py', 'contents.py'):
        r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', tool), '--check'], cwd=ROOT,
                           capture_output=True, text=True)
        if r.returncode != 0:
            problems += r.stdout.strip().split('\n') if r.stdout.strip() else ['tools/%s --check failed' % tool]
    return problems


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
            if tdir == 'peoples' and rel.replace(os.sep, '/') in WORLDS_READING_PEOPLES:
                continue
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
    problems += docs_problems()
    for p in problems:
        print(p)
    print('make check: %s' % ('%d problems' % len(problems) if problems else 'the tree keeps its layout'))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
