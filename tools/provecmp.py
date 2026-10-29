#!/usr/bin/env python3
"""tools/provecmp.py: tools/prove's comparisons (E-02). An item's run is a directory: `stdout`, `stderr`, `exit`, `seconds`, and
`shots/`, every file the item wrote; a snapshot's item may hold `NOISE`, what moved between its own two runs.

  provecmp.py report <snapshot items> <compared items> <items file> [item...]   the report, at most 30 lines; exit 1 on a difference
  provecmp.py noise <run 1> <run 2> [item items-file]                        write run 1's NOISE: what differs between the two runs
  provecmp.py expect <run> <item> <items file>                                the item's outputs counted against its `expect`

Files are compared by kind: `fingerprint_*.txt` key by key (tools/keydiff.py), other .txt files and the printouts line by line
after the masks below, .png pixel by pixel (tools/png.py), the rest byte for byte.
"""
import collections
import fnmatch
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import keydiff  # noqa: E402
import png      # noqa: E402

# What moves between two runs of the same binary: timings, and the saves' wall clock. An item adds its own with `mask=<regex>`
# in its options (tools/prove.items); nothing else is masked.
MASKS = [
    (re.compile(r'(?:\d+(?:\.\d+)?(?:/| \+ ))*\d+(?:\.\d+)? ?(?:ms|us)\b'), '# ms'),
    (re.compile(r'\b1[789]\d{8}\b'), '#unix'),   # a unix time (2023-2033): a save's, a probe record's
    (re.compile(r'\d+(?:\.\d+)?% of (?:real time|a core)'), '#%'),   # a render's cost against the sound's length or a core
    (re.compile(r'\(\d+(?:\.\d+)? s\)'), '(# s)'),
    (re.compile(r'\bin \d+(?:\.\d+)? s\b'), 'in # s'),
    (re.compile(r'^saved \d+$'), 'saved #'),
]


def items(path):
    """name -> (command, expect, options), in the file's order: the options are the rest of the line, words, `mask=<regex>`
    among them (a regex may hold a |, a command may not)."""
    out = collections.OrderedDict()
    for line in open(path):
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        f = [x.strip() for x in line.rstrip('\n').split('|', 3)]
        f += [''] * (4 - len(f))
        out[f[0]] = (f[1], f[2], f[3].split())
    return out


SPACES = re.compile(r'[ \t]+')


def item_masks(items_file, name):
    opts = items(items_file).get(name, ('', '', []))[2] if items_file else []
    return [(re.compile(o[len('mask='):]), '#') for o in opts if o.startswith('mask=')]


def item_noise(items_file, name):
    """The item's files known to move between runs (`noise=<glob>`, relative to shots/), skipped like its NOISE."""
    opts = items(items_file).get(name, ('', '', []))[2] if items_file else []
    return [o[len('noise='):] for o in opts if o.startswith('noise=')]


def masked(path, extra=()):
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            lines = f.read().split('\n')
    except OSError:
        return None
    out = []
    for line in lines:
        for rx, to in MASKS + list(extra):
            line = rx.sub(to, line)
        out.append(SPACES.sub(' ', line))   # a column's padding moves with a masked number's width
    return out


def clip(s, n=60):
    s = s.strip()
    return s if len(s) <= n else s[:n - 3] + '...'


def text_diff(a, b, extra=()):
    la, lb = masked(a, extra), masked(b, extra)
    if la is None or lb is None:
        return None if la == lb else 'missing'
    if la == lb:
        return None
    first = next((i for i, (x, y) in enumerate(zip(la, lb)) if x != y), min(len(la), len(lb)))
    moved = max(sum((collections.Counter(la) - collections.Counter(lb)).values()),
                sum((collections.Counter(lb) - collections.Counter(la)).values()))
    x = la[first] if first < len(la) else '(end)'
    y = lb[first] if first < len(lb) else '(end)'
    return '%d lines differ, the first at %d: "%s" -> "%s"' % (moved, first + 1, clip(x), clip(y))


def file_diff(a, b, extra=()):
    name = os.path.basename(a)
    if name.startswith('fingerprint_') and name.endswith('.txt'):
        r = keydiff.Result(a, b)
        return None if r.same() else r.summary()
    if name.endswith('.txt') or name.endswith('.log'):
        return text_diff(a, b, extra)
    with open(a, 'rb') as fa, open(b, 'rb') as fb:
        da, db = fa.read(), fb.read()
    if da == db:
        return None
    if name.endswith('.png'):
        try:
            n, step, r0, r1 = png.diff(a, b, quiet=True)
            if n < 0:
                return 'its size differs'
            if n == 0:
                return None   # the same pixels, written differently
            return '%d px differ, the largest step %d, rows %d-%d' % (n, step, r0, r1)
        except (ValueError, OSError) as e:
            return 'differs (%s)' % e
    return 'differs (%d bytes against %d)' % (len(da), len(db))


def files(run):
    root = os.path.join(run, 'shots')
    out = set()
    for d, _, names in os.walk(root):
        for n in names:
            out.add(os.path.relpath(os.path.join(d, n), root))
    return out


def read(path, default=''):
    try:
        return open(path).read().strip()
    except OSError:
        return default


def compare_run(a, b, extra=(), known=()):
    """(problems, noisy files, files compared): a the snapshot's run, b the new one; extra: the item's own masks; known: the globs
    of its files that move between runs."""
    noise = set(read(os.path.join(a, 'NOISE')).split('\n')) - {''}
    noise |= {f for f in files(a) | files(b) if any(fnmatch.fnmatch(f, g) for g in known)}
    problems = []
    ea, eb = read(os.path.join(a, 'exit')), read(os.path.join(b, 'exit'))
    if ea != eb:
        problems.append('exit %s -> %s' % (ea, eb))
    for stream in ('stdout', 'stderr'):
        if stream in noise:
            continue
        d = text_diff(os.path.join(a, stream), os.path.join(b, stream), extra)
        if d:
            problems.append('%s: %s' % (stream, d))
    fa, fb = files(a), files(b)
    for f in sorted(fa - fb):
        problems.append('%s: missing' % f)
    for f in sorted(fb - fa):
        problems.append('%s: new' % f)
    noisy = 0
    for f in sorted(fa & fb):
        if f in noise:
            noisy += 1
            continue
        d = file_diff(os.path.join(a, 'shots', f), os.path.join(b, 'shots', f), extra)
        if d:
            problems.append('%s: %s' % (f, d))
    return problems, noisy, len(fa)


def expect(run, spec):
    """The problems with the item's expected outputs: `glob=count ...`, globs relative to the scratch directory."""
    out = []
    for term in spec.split():
        g, _, n = term.rpartition('=')
        got = len([p for p in glob.glob(os.path.join(run, g)) if os.path.isfile(p)])
        if not g or not n.isdigit():
            out.append('bad expect "%s"' % term)
        elif got != int(n):
            out.append('%s: %d, expected %s' % (g, got, n))
    return out


def report(snap, new, items_file, names):
    battery = items(items_file)
    names = names or [n for n in battery if os.path.isdir(os.path.join(snap, n))]
    lines, same, noisy_items, differ, total = [], [], [], 0, 0
    for name in names:
        a, b = os.path.join(snap, name), os.path.join(new, name)
        if not os.path.isdir(a):
            lines.append('NOT IN THE SNAPSHOT %s' % name)
            differ += 1
            continue
        if not os.path.isdir(b):
            lines.append('NOT RUN %s' % name)
            differ += 1
            continue
        problems, noisy, n = compare_run(a, b, item_masks(items_file, name), item_noise(items_file, name))
        problems += expect(b, battery.get(name, ('', '', []))[1])
        total += n
        if noisy:
            noisy_items.append('%s (%d)' % (name, noisy))
        if not problems:
            same.append(name)
            continue
        differ += 1
        cap = 3 if len(names) > 1 else 25   # an item asked for alone shows more
        lines.append('DIFFERS %s: %d problems' % (name, len(problems)))
        lines += ['  ' + clip(p, 150) for p in problems[:cap]]
        if len(problems) > cap:
            lines.append('  and %d more (compare this item alone)' % (len(problems) - cap))
    head = ['prove compare: %d items, %d files' % (len(names), total)]
    if same:
        words, line = same, '  the same:'
        for w in words:
            if len(line) + len(w) > 136:
                head.append(line)
                line = '   '
            line += ' ' + w
        head.append(line)
    if noisy_items:
        head.append('  noisy as in the snapshot\'s own two runs (files skipped): ' + ', '.join(noisy_items))
    tail = ['everything the same' if not differ else '%d of %d items differ' % (differ, len(names))]
    body = head + lines
    if len(body) > 29:
        body = body[:28] + ['  ... (%d lines cut)' % (len(body) - 28)]
    print('\n'.join(body + tail))
    return 1 if differ else 0


def noise(a, b, extra=()):
    moved = []
    for stream in ('stdout', 'stderr'):
        if text_diff(os.path.join(a, stream), os.path.join(b, stream), extra):
            moved.append(stream)
    fa, fb = files(a), files(b)
    moved += sorted(fa ^ fb)
    moved += [f for f in sorted(fa & fb) if file_diff(os.path.join(a, 'shots', f), os.path.join(b, 'shots', f), extra)]
    with open(os.path.join(a, 'NOISE'), 'w') as out:
        out.write(''.join(m + '\n' for m in moved))
    print('%s: %d of %d outputs moved between its two runs%s' % (os.path.basename(a.rstrip('/')), len(moved), len(fa) + 2,
          (': ' + ', '.join(moved[:6])) if moved else ''))
    return 0


def main(argv):
    if len(argv) >= 5 and argv[1] == 'report':
        return report(argv[2], argv[3], argv[4], argv[5:])
    if len(argv) in (4, 6) and argv[1] == 'noise':
        return noise(argv[2], argv[3], item_masks(argv[5], argv[4]) if len(argv) == 6 else ())
    if len(argv) == 5 and argv[1] == 'expect':
        spec = items(argv[4]).get(argv[3], ('', '', []))[1]
        problems = expect(argv[2], spec)
        for p in problems:
            print('%s: %s' % (argv[3], p))
        return 1 if problems else 0
    print(__doc__.strip())
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
