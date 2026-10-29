#!/usr/bin/env python3
"""tools/clash.py (E-09): the names a split's internal header makes global, checked against the program.

  tools/clash.py <internal header>      e.g. src/peoples/buildings_internal.h (its family: build/*/buildings*.o)

A split that takes a file-local helper or type into its internal header gives it external linkage: an inline function, or a
type's member, of the same mangled name defined in another object would be merged by the linker with it, silently. This reads
the names the header defines or declares outside an anonymous namespace (its functions, types and variables), then every
global symbol the build's objects define, and lists each symbol of one of those names that an object outside the family (the
header's name up to its first `_`) defines too. Run it after `make`; it exits 1 on any.
"""
import collections
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import mover   # noqa: E402

DIRS = ('core', 'galaxy', 'peoples', 'space', 'surface', 'game', 'harness')


def header_names(path):
    src = open(path).read()
    m, _ = mover.mask(src)
    names = set()
    for c in mover.chunks(src):
        if c.kind in ('func', 'decl', 'type', 'var', 'using', 'typedef') and not mover.anon_depth(c):
            names |= set(mover.defined_names(c, src, m))
    return {n.split('::')[0] for n in names}


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    header = sys.argv[1]
    fam = os.path.basename(header).split('_')[0].split('.')[0]
    names = header_names(header)
    objs = [o for d in DIRS for o in glob.glob(os.path.join(ROOT, 'build', d, '*.o'))] + glob.glob(os.path.join(ROOT, 'build', '*.o'))
    defs = collections.defaultdict(set)
    for o in objs:
        out = subprocess.run(['nm', '-g', '--defined-only', o], capture_output=True, text=True).stdout
        for line in out.splitlines():
            p = line.split()
            if len(p) >= 3:
                defs[p[2]].add(os.path.basename(o))
    syms = sorted(defs)
    dem = dict(zip(syms, subprocess.run(['c++filt'], input='\n'.join(syms), capture_output=True, text=True).stdout.splitlines()))
    lead = re.compile(r'^(?:[\w:<>, ]*? )?([A-Za-z_]\w*)(?:::|\(|$)')   # a symbol's first name (after a return type, if any)
    mine, clash = 0, []
    for s in syms:
        objs_ = defs[s]
        if not any(o.startswith(fam) for o in objs_):
            continue
        k = lead.match(dem[s])
        if not k or k.group(1) not in names:
            continue
        mine += 1
        others = sorted(o for o in objs_ if not o.startswith(fam))
        if others:
            clash.append((dem[s], others))
    print('clash: %s: %d names; %d symbols of them its objects define, %d defined elsewhere too'
          % (os.path.relpath(header, ROOT), len(names), mine, len(clash)))
    for n, others in clash:
        print('  %s: %s' % (n[:110], ' '.join(others[:6])))
    return 1 if clash else 0


if __name__ == '__main__':
    sys.exit(main())
