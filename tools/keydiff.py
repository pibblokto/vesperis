#!/usr/bin/env python3
"""tools/keydiff.py: two fingerprint files compared key by key (E-02's `fingerprint` modes).

A line is `<what> <key...> <field>=<value> ...`. Lines are matched by their key (the words before the first field; a key that
repeats, by its occurrence) and compared field by field. A field that only one side has is a column added or dropped: it is
reported apart from the differences, so a step that adds a field still proves every other one. Lines without fields are
compared as text.

  keydiff.py <a> <b> [--show N]   the counts, then the first N differing keys with their fields (8 by default)

Exits 0 when no shared field differs and no key is missing or new.
"""
import collections
import sys


def parse(path):
    keyed, loose, seen = collections.OrderedDict(), collections.Counter(), collections.Counter()
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            words = line.split()
            if not words:
                continue
            first = next((i for i, w in enumerate(words) if '=' in w), None)
            if first is None:
                loose[line.rstrip('\n')] += 1
                continue
            key = ' '.join(words[:first])
            seen[key] += 1
            if seen[key] > 1:
                key += ' #%d' % seen[key]
            keyed[key] = dict(w.split('=', 1) for w in words[first:] if '=' in w)
    return keyed, loose


class Result:
    def __init__(self, a, b):
        ka, la = parse(a)
        kb, lb = parse(b)
        self.keys = len(ka)
        self.missing = [k for k in ka if k not in kb]
        self.new = [k for k in kb if k not in ka]
        self.differ = []          # (key, [(field, a, b)])
        self.fields = collections.Counter()
        added, dropped = set(), set()
        for k, fa in ka.items():
            fb = kb.get(k)
            if fb is None:
                continue
            moved = [(n, v, fb[n]) for n, v in fa.items() if n in fb and fb[n] != v]
            added.update(n for n in fb if n not in fa)
            dropped.update(n for n in fa if n not in fb)
            if moved:
                self.differ.append((k, moved))
                self.fields.update(n for n, _, _ in moved)
        self.added, self.dropped = sorted(added), sorted(dropped)
        self.loose_only_a = sum((la - lb).values())
        self.loose_only_b = sum((lb - la).values())

    def same(self):
        return not (self.missing or self.new or self.differ or self.loose_only_a or self.loose_only_b)

    def summary(self):
        """One line."""
        if self.same():
            s = '%d keys the same' % self.keys
        else:
            parts = []
            if self.differ:
                parts.append('%d keys differ (%s)' % (len(self.differ), ', '.join('%s %d' % f for f in self.fields.most_common(4))))
            if self.missing:
                parts.append('%d keys missing' % len(self.missing))
            if self.new:
                parts.append('%d keys new' % len(self.new))
            if self.loose_only_a or self.loose_only_b:
                parts.append('%d text lines differ' % max(self.loose_only_a, self.loose_only_b))
            s = '; '.join(parts) + ' of %d' % self.keys
        if self.added:
            s += '; columns added: ' + ' '.join(self.added)
        if self.dropped:
            s += '; columns dropped: ' + ' '.join(self.dropped)
        return s

    def examples(self, n):
        out = []
        for k, moved in self.differ[:n]:
            out.append('%s: %s' % (k, ' '.join('%s %s->%s' % m for m in moved[:4])))
        out += ['missing: ' + k for k in self.missing[:max(0, n - len(out))]]
        out += ['new: ' + k for k in self.new[:max(0, n - len(out))]]
        return out


def main(argv):
    if len(argv) < 3:
        print(__doc__.strip())
        return 2
    show = int(argv[argv.index('--show') + 1]) if '--show' in argv else 8
    r = Result(argv[1], argv[2])
    print(r.summary())
    for line in r.examples(show):
        print('  ' + line)
    return 0 if r.same() else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
