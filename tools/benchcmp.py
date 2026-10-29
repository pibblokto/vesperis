#!/usr/bin/env python3
"""tools/benchcmp.py: `bench check` in pairs, binary B against binary A (E-02; P-06's and Q-03's scratch tools made one).

  benchcmp.py <dir>   reads <dir>/a_<i>.txt and <dir>/b_<i>.txt (tools/prove bench writes them) and prints each budget row's
                      best-of ratio (B's fastest run over A's) and the median of its pairs' ratios, the runs over a budget, and the
                      verdict: "level" when every row's best-of ratio is within 0.97-1.03 (or its bests are one printed
                      step of 0.1 ms apart)

Rows known to be noisy are marked (the buggy's 2x moves by itself between runs); a row outside the band is named either way.
Exits 0 when level and every run of B is within its budgets.
"""
import os
import re
import statistics
import sys

ROW = re.compile(r'budget (.+?)\s+([0-9.]+) ms \(limit ([0-9.]+)\) (\w+)')
NOISY = {'buggy 2x'}
BAND = (0.97, 1.03)
STEP = 0.1   # the printed resolution in ms: a row a step apart is level, whatever its ratio


def rows(path):
    out = {}
    for line in open(path, errors='replace'):
        m = ROW.match(line)
        if m:
            out[m.group(1).strip()] = (float(m.group(2)), float(m.group(3)), m.group(4))
    return out


def main(argv):
    if len(argv) < 2:
        print(__doc__.strip())
        return 2
    d = argv[1]
    pairs = []
    for i in range(1, 100):
        a, b = os.path.join(d, 'a_%d.txt' % i), os.path.join(d, 'b_%d.txt' % i)
        if not (os.path.exists(a) and os.path.exists(b)):
            break
        pairs.append((rows(a), rows(b)))
    if not pairs:
        print('no pairs in %s' % d)
        return 2
    names = list(pairs[0][0].keys())
    info = open(os.path.join(d, 'info')).read().strip().replace('\n', '; ') if os.path.exists(os.path.join(d, 'info')) else d
    print('bench pairs: %d (%s); per row: B/A best-of, the median of the pairs, A and B best in ms' % (len(pairs), info))
    outside, over_a, over_b = [], 0, 0
    for n in names:
        av = [p[0][n][0] for p in pairs if n in p[0]]
        bv = [p[1][n][0] for p in pairs if n in p[1]]
        if not av or not bv:
            print('%-22s missing in a run' % n)
            outside.append(n)
            continue
        best = min(bv) / min(av) if min(av) > 0 else 1.0
        med = statistics.median(p[1][n][0] / p[0][n][0] for p in pairs if n in p[0] and n in p[1] and p[0][n][0] > 0)
        oa = sum(1 for p in pairs if n in p[0] and p[0][n][2] != 'ok')
        ob = sum(1 for p in pairs if n in p[1] and p[1][n][2] != 'ok')
        over_a += oa
        over_b += ob
        flag = '' if BAND[0] <= best <= BAND[1] or abs(min(bv) - min(av)) <= STEP + 1e-9 else ' OUTSIDE'
        if flag:
            outside.append(n)
        print('%-22s %.3f  %.3f  %7.2f %7.2f%s%s%s' % (n, best, med, min(av), min(bv), flag, ' (noisy)' if n in NOISY else '',
              (' over budget: A %d, B %d' % (oa, ob)) if oa or ob else ''))
    real = [n for n in outside if n not in NOISY]
    print('%d rows; %s; runs over a budget: A %d, B %d' % (
        len(names), 'level' if not real else 'outside the band: ' + ', '.join(outside),
        over_a, over_b) + (' (noisy rows outside: %s)' % ', '.join(n for n in outside if n in NOISY) if outside and not real else ''))
    return 0 if not real and not over_b else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
