#!/usr/bin/env python3
"""tools/wav.py: the harness's sounds without numpy (E-02: Q-03's wavinfo made a tool).

  wav.py stats <a.wav>                  the length, the rate, the peak, the rms, the samples at the ceiling, the loudest second
  wav.py events <a.wav> [threshold]     windows of 20 ms over the threshold (0.02 by default) grouped into events: each one's start,
                                        length, peak, rms and its zero-crossing pitch at the start, the middle and the end
  wav.py pitch <a.wav> [from to]        the autocorrelation pitch of the loudest 50 ms window (of the span in seconds)

Reads 16-bit PCM, mono or the first channel (the harness writes 22,050 Hz mono).
"""
import array
import math
import sys
import wave


def read(path):
    w = wave.open(path)
    rate, n, ch, width = w.getframerate(), w.getnframes(), w.getnchannels(), w.getsampwidth()
    if width != 2:
        raise ValueError(path + ': only 16-bit PCM')
    a = array.array('h', w.readframes(n))
    return rate, [a[i] / 32767.0 for i in range(0, len(a), ch)]


def stats(path):
    rate, x = read(path)
    n = len(x)
    peak = max((abs(v) for v in x), default=0.0)
    rms = math.sqrt(sum(v * v for v in x) / n) if n else 0.0
    ceiling = sum(1 for v in x if abs(v) >= 0.999)
    best, at = 0.0, 0
    for k in range(0, max(1, n - rate), max(1, rate // 4)):
        seg = x[k:k + rate]
        e = sum(v * v for v in seg)
        if e > best:
            best, at = e, k
    print('%s: %.2f s at %d Hz, peak %.3f, rms %.3f, %d samples at the ceiling, the loudest second from %.2f s (rms %.3f)'
          % (path, n / rate, rate, peak, rms, ceiling, at / rate, math.sqrt(best / min(rate, n)) if n else 0.0))


def crossings(seg, rate):
    c = sum(1 for i in range(1, len(seg)) if (seg[i - 1] < 0) != (seg[i] < 0))
    return c / 2 / (len(seg) / rate) if seg else 0.0


def events(path, threshold=0.02):
    rate, x = read(path)
    w = int(0.02 * rate)
    found, cur = [], None
    for k in range(0, len(x) - w, w):
        seg = x[k:k + w]
        pk = max(abs(v) for v in seg)
        if pk > threshold:
            if cur is None:
                cur = [k, k + w, pk, [seg]]
            else:
                cur[1], cur[2] = k + w, max(cur[2], pk)
                cur[3].append(seg)
        elif cur is not None:
            found.append(cur)
            cur = None
    if cur:
        found.append(cur)
    for e in found:
        segs = e[3]
        allx = [v for s in segs for v in s]
        r = math.sqrt(sum(v * v for v in allx) / len(allx))
        print('%6.2f s  %5.0f ms  peak %.3f rms %.3f  pitch %5.0f -> %5.0f -> %5.0f Hz' % (
            e[0] / rate, (e[1] - e[0]) / rate * 1000, e[2], r, crossings(segs[0], rate), crossings(segs[len(segs) // 2], rate),
            crossings(segs[-1], rate)))
    print('%d events over %.3f' % (len(found), threshold))


def pitch(path, t0=None, t1=None):
    rate, x = read(path)
    a, b = int((t0 or 0) * rate), int(t1 * rate) if t1 else len(x)
    w = int(0.05 * rate)
    best, at = -1.0, a
    for k in range(a, max(a + 1, b - w), w // 2):
        e = sum(v * v for v in x[k:k + w])
        if e > best:
            best, at = e, k
    seg = x[at:at + w]
    lo, hi = int(rate / 1000), int(rate / 50)   # 50 Hz to 1 kHz
    r0 = sum(v * v for v in seg) or 1.0
    scores = [(sum(seg[i] * seg[i + lag] for i in range(len(seg) - lag)) / r0, lag) for lag in range(lo, min(hi, len(seg) - 1))]
    peak = max(s for s, _ in scores)
    lag = next(lag for s, lag in scores if s >= 0.9 * peak)   # the shortest strong period
    print('%s: the loudest 50 ms from %.2f s, pitch %.1f Hz (the autocorrelation %.2f at %d samples)' % (path, at / rate, rate / lag, peak, lag))


def main(argv):
    if len(argv) < 3:
        print(__doc__.strip())
        return 2
    if argv[1] == 'stats':
        for p in argv[2:]:
            stats(p)
    elif argv[1] == 'events':
        events(argv[2], float(argv[3]) if len(argv) > 3 else 0.02)
    elif argv[1] == 'pitch':
        pitch(argv[2], float(argv[3]) if len(argv) > 3 else None, float(argv[4]) if len(argv) > 4 else None)
    else:
        print(__doc__.strip())
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
