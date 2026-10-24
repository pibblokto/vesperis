#!/usr/bin/env python3
"""tools/png.py: the harness's pictures without numpy (E-02: D-05's pngcrop and P-07's montage made one tool).

  png.py stats <a.png> [x y w h]           the size, each channel's mean, min and max, the luminance's quartiles and the share of
                                           pixels darker than 16 (judge a dark frame by these numbers, not by eye)
  png.py crop <in> <out> x y w h [scale]   a rectangle, scaled up by whole pixels
  png.py montage <out> <cols> <a.png> ...  a grid of frames, each cut to the first's size, to Read one picture instead of many
  png.py diff <a> <b> [out]                the pixels that differ, the largest step and the rows they are in; out: the differing pixels
                                           in red over a darkened copy of a

Reads 8-bit RGB or RGBA non-interlaced PNGs (src/core/png writes RGB with filter 0) and writes RGB.
"""
import struct
import sys
import zlib


def read(path):
    """(width, height, rows): each row bytes of RGB triples."""
    data = open(path, 'rb').read()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError(path + ': not a PNG')
    i, idat, w, h, ct = 8, [], 0, 0, 2
    while i < len(data):
        n = struct.unpack('>I', data[i:i + 4])[0]
        kind, chunk = data[i + 4:i + 8], data[i + 8:i + 8 + n]
        i += 12 + n
        if kind == b'IHDR':
            w, h, depth, ct, _, _, interlace = struct.unpack('>IIBBBBB', chunk)
            if depth != 8 or interlace or ct not in (2, 6):
                raise ValueError(path + ': only 8-bit RGB or RGBA, not interlaced')
        elif kind == b'IDAT':
            idat.append(chunk)
    bpp = 3 if ct == 2 else 4
    raw, stride = zlib.decompress(b''.join(idat)), w * bpp
    rows, prev, k = [], bytearray(stride), 0
    for _ in range(h):
        f = raw[k]
        line = bytearray(raw[k + 1:k + 1 + stride])
        k += 1 + stride
        if f == 1:
            for x in range(bpp, stride): line[x] = (line[x] + line[x - bpp]) & 255
        elif f == 2:
            for x in range(stride): line[x] = (line[x] + prev[x]) & 255
        elif f == 3:
            for x in range(stride): line[x] = (line[x] + (((line[x - bpp] if x >= bpp else 0) + prev[x]) >> 1)) & 255
        elif f == 4:
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                b, c = prev[x], (prev[x - bpp] if x >= bpp else 0)
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else (b if pb <= pc else c))) & 255
        prev = line
        if bpp == 4:
            line = bytearray(b for j in range(0, stride, 4) for b in line[j:j + 3])
        rows.append(bytes(line))
    return w, h, rows


def write(path, w, h, rows):
    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body) & 0xffffffff)
    raw = b''.join(b'\x00' + r for r in rows)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                           + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def stats(path, box=None):
    w, h, rows = read(path)
    x0, y0, bw, bh = box if box else (0, 0, w, h)
    bw, bh = min(bw, w - x0), min(bh, h - y0)
    sums, lows, highs, lum, n = [0, 0, 0], [255] * 3, [0] * 3, [], 0
    for y in range(y0, y0 + bh):
        r = rows[y]
        for x in range(x0, x0 + bw):
            p = r[3 * x:3 * x + 3]
            for c in range(3):
                sums[c] += p[c]
                lows[c] = min(lows[c], p[c])
                highs[c] = max(highs[c], p[c])
            lum.append((299 * p[0] + 587 * p[1] + 114 * p[2]) // 1000)
            n += 1
    lum.sort()
    q = lambda f: lum[min(n - 1, int(f * n))]
    dark = sum(1 for v in lum if v < 16)
    print('%s: %dx%d%s' % (path, w, h, ' box %d,%d %dx%d' % (x0, y0, bw, bh) if box else ''))
    for c, name in enumerate('RGB'):
        print('  %s mean %.1f min %d max %d' % (name, sums[c] / n, lows[c], highs[c]))
    print('  luminance quartiles %d %d %d %d %d (min, 25%%, median, 75%%, max); %.1f%% darker than 16'
          % (lum[0], q(0.25), q(0.5), q(0.75), lum[-1], 100.0 * dark / n))


def crop(src, dst, x0, y0, cw, chh, scale=1):
    w, h, rows = read(src)
    cw, chh = min(cw, w - x0), min(chh, h - y0)
    out = []
    for y in range(y0, y0 + chh):
        r = rows[y][3 * x0:3 * (x0 + cw)]
        if scale > 1:
            r = b''.join(r[i:i + 3] * scale for i in range(0, len(r), 3))
        out.extend([r] * scale)
    write(dst, cw * scale, chh * scale, out)
    print('%dx%d -> %dx%d %s' % (w, h, cw * scale, chh * scale, dst))


def montage(dst, cols, paths):
    frames = [read(p) for p in paths]
    fw, fh = frames[0][0], frames[0][1]
    gap = 2
    rws = (len(frames) + cols - 1) // cols
    W, H = cols * fw + (cols - 1) * gap, rws * fh + (rws - 1) * gap
    out = [bytearray(b'\x40' * (3 * W)) for _ in range(H)]
    for k, (w, h, rows) in enumerate(frames):
        ox, oy = (k % cols) * (fw + gap), (k // cols) * (fh + gap)
        for y in range(min(fh, h)):
            seg = rows[y][:3 * min(fw, w)]
            out[oy + y][3 * ox:3 * ox + len(seg)] = seg
    write(dst, W, H, [bytes(r) for r in out])
    print('%d frames of %dx%d -> %dx%d %s' % (len(frames), fw, fh, W, H, dst))


def diff(a, b, dst=None, quiet=False):
    """(differing pixels, largest step, first differing row, last): (0, 0, -1, -1) when the same."""
    wa, ha, ra = read(a)
    wb, hb, rb = read(b)
    if (wa, ha) != (wb, hb):
        if not quiet: print('%s: %dx%d, %s: %dx%d' % (a, wa, ha, b, wb, hb))
        return (-1, -1, -1, -1)
    count, step, first, last = 0, 0, -1, -1
    out = [] if dst else None
    for y in range(ha):
        if ra[y] == rb[y]:
            if out is not None: out.append(bytes(v // 3 for v in ra[y]))
            continue
        line = bytearray(v // 3 for v in ra[y]) if out is not None else None
        for x in range(wa):
            p, q = ra[y][3 * x:3 * x + 3], rb[y][3 * x:3 * x + 3]
            if p != q:
                count += 1
                step = max(step, max(abs(p[c] - q[c]) for c in range(3)))
                first = y if first < 0 else first
                last = y
                if line is not None: line[3 * x:3 * x + 3] = b'\xff\x00\x00'
        if out is not None: out.append(bytes(line))
    if dst: write(dst, wa, ha, out)
    if not quiet:
        print('%s: %d px differ (%.3f%%), the largest step %d%s' % (b, count, 100.0 * count / (wa * ha), step,
              ', rows %d-%d' % (first, last) if count else ''))
    return (count, step, first, last)


def main(argv):
    if len(argv) < 3:
        print(__doc__.strip())
        return 2
    cmd = argv[1]
    if cmd == 'stats':
        stats(argv[2], tuple(map(int, argv[3:7])) if len(argv) >= 7 else None)
    elif cmd == 'crop':
        crop(argv[2], argv[3], *map(int, argv[4:8]), scale=int(argv[8]) if len(argv) > 8 else 1)
    elif cmd == 'montage':
        montage(argv[2], int(argv[3]), argv[4:])
    elif cmd == 'diff':
        return 1 if diff(argv[2], argv[3], argv[4] if len(argv) > 4 else None)[0] else 0
    else:
        print(__doc__.strip())
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
