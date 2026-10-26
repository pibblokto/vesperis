#!/usr/bin/env python3
"""tools/docwrap.py (E-04): rewrap a Markdown file's long lines, changing only where the lines break.

    tools/docwrap.py [--max N] [--width W] [--check] <file.md> ...

Every line longer than N characters (2,000: the Read tool cuts such a line) is broken into lines of at most W (130), as the
file's own lines are: a paragraph's lines start at the margin, a list item's continue under its text, a quote's keep its
`>`. A break never falls inside a code span or a link's target, and no line it makes starts with what Markdown would read
as a new block (a list marker, `#`, `>`, `|`, a fence, `<`, a rule), so the page renders as before.

Not wrapped, only reported: headings, table rows (a table's row is one line: turn the table into a list by hand), lines in
fenced code. `--check` reports the long lines and changes nothing.

After wrapping, each file is compared with what it was: its words in order, its headings and its bold-led bullets must be
the same, or nothing is written. Exits 1 when a long line is left.
"""
import argparse
import re
import sys

LIST = re.compile(r'^(\s*)([*+-]|\d{1,9}[.)])(\s+)')
QUOTE = re.compile(r'^(\s*(?:>\s?)+)')
BLOCK_START = re.compile(r'^(?:[*+-]|\d{1,9}[.)]|#{1,6}|>.*|\|.*|`{3,}.*|~{3,}.*|<.*|=+|-{2,}.*|_{3,}.*|\*{3,}.*)$')


def tokens(text):
    """The words of a text, a code span or a link's `](target)` kept whole however many spaces it holds."""
    out, cur, i, n = [], '', 0, len(text)
    while i < n:
        c = text[i]
        if c == '`':
            j = i
            while j < n and text[j] == '`':
                j += 1
            ticks = text[i:j]
            k = text.find(ticks, j)
            while k != -1 and k + len(ticks) < n and text[k + len(ticks)] == '`':
                k = text.find(ticks, k + len(ticks) + 1)
            if k == -1:
                cur += ticks
                i = j
                continue
            cur += text[i:k + len(ticks)]
            i = k + len(ticks)
            continue
        if c == ']' and i + 1 < n and text[i + 1] == '(':
            k = text.find(')', i + 2)
            if k != -1:
                cur += text[i:k + 1]
                i = k + 1
                continue
        if c == ' ':
            if cur:
                out.append(cur)
                cur = ''
            i += 1
            continue
        cur += c
        i += 1
    if cur:
        out.append(cur)
    return out


def fill(words, first, rest, width):
    """Greedy lines of the words, the first after `first` and the others after `rest`; a word Markdown would take for the
    start of a block never begins a line."""
    lines, line, prefix = [], [], first
    for w in words:
        if line and len(prefix) + len(' '.join(line + [w])) > width:
            if BLOCK_START.match(w):
                # take the word up rather than start a line with it (or hand the line's last word down with it)
                if len(line) > 1 and not BLOCK_START.match(line[-1]):
                    carry = line.pop()
                    lines.append(prefix + ' '.join(line))
                    prefix, line = rest, [carry, w]
                    continue
                line.append(w)
                continue
            lines.append(prefix + ' '.join(line))
            prefix, line = rest, [w]
        else:
            line.append(w)
    if line:
        lines.append(prefix + ' '.join(line))
    return lines


def wrap_line(line, width):
    """The line broken into lines of at most `width`, or None when it is not a line to wrap."""
    if line.lstrip().startswith('#') and re.match(r'^\s*#{1,6}(\s|$)', line):
        return None
    if line.lstrip().startswith('|'):
        return None
    hard = line.endswith('  ')
    body = line.rstrip()
    m = QUOTE.match(body)
    if m:
        first = rest = m.group(1)
        text = body[len(first):]
        lm = LIST.match(text)
        if lm:
            first = first + text[:lm.end()]
            rest = rest + ' ' * len(text[:lm.end()])
            text = text[lm.end():]
    else:
        lm = LIST.match(body)
        if lm:
            first = body[:lm.end()]
            rest = ' ' * len(first)
            text = body[lm.end():]
        else:
            indent = body[:len(body) - len(body.lstrip())]
            first = rest = indent
            text = body.lstrip()
    out = fill(tokens(text), first, rest, width)
    if hard:
        out[-1] += '  '
    return out


def words(text):
    return text.split()


def headings(lines):
    return [l for l in lines if re.match(r'^#{1,6}\s', l)]


def bold_bullets(lines):
    return [l.split('**')[1] for l in lines if re.match(r'^\s*[*+-]\s+\*\*', l) and l.count('**') >= 2]


def process(path, maxlen, width, check):
    src = open(path, encoding='utf-8').read()
    lines = src.split('\n')
    out, left, fence = [], [], False
    for i, line in enumerate(lines, 1):
        if re.match(r'^\s*(```|~~~)', line):
            fence = not fence
        if len(line) <= maxlen:
            out.append(line)
            continue
        wrapped = None if (fence or check) else wrap_line(line, width)
        if wrapped is None:
            kind = 'in a fence' if fence else 'a heading' if line.lstrip().startswith('#') else \
                'a table row' if line.lstrip().startswith('|') else 'long'
            left.append('%s:%d: %s line of %d characters' % (path, i, kind, len(line)))
            out.append(line)
            continue
        out.extend(wrapped)
        if any(len(l) > maxlen for l in wrapped):
            left.append('%s:%d: a word of over %d characters' % (path, i, maxlen))
    new = '\n'.join(out)
    if new != src:
        assert words(new) == words(src), path + ': the words changed'
        assert headings(new.split('\n')) == headings(lines), path + ': the headings changed'
        assert bold_bullets(new.split('\n')) == bold_bullets(lines), path + ': the bold-led bullets changed'
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new)
        print('%s: %d lines, was %d' % (path, len(out), len(lines)))
    return left


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--max', type=int, default=2000)
    ap.add_argument('--width', type=int, default=130)
    ap.add_argument('--check', action='store_true')
    ap.add_argument('files', nargs='+')
    a = ap.parse_args()
    left = []
    for p in a.files:
        left += process(p, a.max, a.width, a.check)
    for l in left:
        print(l)
    return 1 if left else 0


if __name__ == '__main__':
    sys.exit(main())
