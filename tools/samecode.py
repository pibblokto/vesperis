#!/usr/bin/env python3
"""E-03: prove that moving functions between files changed no function: every function's unoptimised IR, before and after.

  tools/samecode.py [-I<dir>] [--rename <old>=<new> ...] <old files> -- <new files>
                                                   each a .cpp (compiled here at -O0) or a .ll; `rev:<path>` reads a file
                                                   from git HEAD (`rev=<commit>:<path>` from another commit); -I<dir> is
                                                   searched first for the old files' includes (a saved tree's own headers:
                                                   <dir>/harness/harness.h); --rename (E-05): a C++ name that changed, e.g.
                                                   `Game::RadarState=RadarState` for a type moved out of its class

Each side's .cpp files are compiled to LLVM IR at -O0 (no inlining) and without exceptions, so a function's IR is its own
body and the declarations it saw. Then every function definition is normalised:
  - linkage dropped (a static function made extern, or an inline one in a header, is the same function), and a static
    variable's internal-linkage mangling read as its plain name;
  - the compiler's own counters (a lambda's `$_N`, a string constant's `.str.N`, a type's `.N`) replaced by what they name:
    a lambda by its enclosing function, a private constant by its contents, a type by its name;
  - attribute groups by their contents (less `nounwind`, which clang infers differently once a function is extern),
    metadata and comments dropped.
and the two sides compared per function name, as sets of bodies (an inline function compiled in several files is one).
It prints the counts, the functions missing or added, and the first lines that differ in each differing function.

With --rename, both sides' C++ names are read demangled (the Xcode `c++filt`) and the renames applied to them and to the
types' IR names (`%"struct.Game::RadarState"` is `%struct.RadarState`), so a moved type's functions compare under their new
names; a lambda's counter is dropped there too.
"""
import hashlib
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

CXX = '/usr/bin/clang++'
# without exceptions: the harness throws and catches nothing, and with them whether a call is an `invoke` (and a function
# `nounwind`) follows the order clang emits functions in, which a move changes
FLAGS = ['-std=c++17', '-O0', '-fno-exceptions', '-pthread', '-Isrc', '-S', '-emit-llvm', '-w']

NAME = re.compile(r'@("(?:[^"\\]|\\.)*"|[-\w$.]+)')
ATTR = re.compile(r'(?<=\s)#(\d+)\b')
META = re.compile(r',?\s*!(?:[A-Za-z_][\w.]*)?\s*!\d+|!\d+')
TYPE_SUFFIX = re.compile(r'(%"?(?:struct|class|union)\.[^\s,()*"\[\]{}]*?)\.\d+(?="?(?:[\s,()*\]}]|$))')
LAMBDA = re.compile(r'\d+\$_\d+')


def norm_name(raw):
    """A global's name as both sides can agree on it."""
    q = raw.startswith('"')
    s = raw[1:-1] if q else raw
    s = LAMBDA.sub('$_', s)
    s = re.sub(r'(?<=_Z)L(?=\d)|(?<=Z)L(?=\d)', '', s)
    m = re.match(r'^_Z(\d+)([A-Za-z_]\w*)$', s)
    if m and len(m.group(2)) == int(m.group(1)):
        s = m.group(2)   # a variable at namespace scope: mangled only while it was static
    s = re.sub(r'^__cxx_global_var_init\.\d+$', '__cxx_global_var_init', s)
    s = re.sub(r'^_GLOBAL__sub_I_.*$', '_GLOBAL__sub_I', s)
    return s


def strip_comment(line):
    out = []
    inq = False
    for k, ch in enumerate(line):
        if ch == '"':
            inq = not inq
        elif ch == ';' and not inq:
            return ''.join(out).rstrip()
        out.append(ch)
    return line


RENAMES = []   # (pattern, new): --rename's


def renamed(s):
    for pat, new in RENAMES:
        s = pat.sub(new, s)
    return re.sub(r'\$_\d+', '$_', s)


HOMEBREW_CXXFILT = '/opt/homebrew/opt/llvm/bin/llvm-cxxfilt'
UNREAD = set()   # names the demangler could not read (kept mangled: a rename inside one is not seen)


def demangled_ir(text):
    """The IR with every C++ global's name demangled and renamed, and the types' names renamed (--rename)."""
    keys = sorted({m.group(1).strip('"') for m in NAME.finditer(text) if m.group(1).strip('"').startswith('_Z')})
    # Homebrew's llvm-cxxfilt reads every name clang makes; the Xcode c++filt (a Mach-O symbol, one more leading underscore)
    # leaves the newer forms (a `u` vendor type, a decltype) mangled
    if os.path.exists(HOMEBREW_CXXFILT):
        cmd, pre = [HOMEBREW_CXXFILT, '-n'], ''
    else:
        cmd, pre = ['/usr/bin/c++filt'], '_'
    out = subprocess.run(cmd, input=''.join(pre + k + '\n' for k in keys), capture_output=True, text=True,
                         check=True).stdout.split('\n')
    dem = {k: out[i] for i, k in enumerate(keys) if out[i] != pre + k}
    UNREAD.update(k for k in keys if k not in dem)

    def name(m):
        key = m.group(1).strip('"')
        return '@"' + renamed(dem[key]) + '"' if key in dem else m.group(0)
    text = NAME.sub(name, text)
    text = re.sub(r'%"((?:struct|class|union)\.[^"]*)"', lambda m: '%"' + renamed(m.group(1)) + '"', text)
    text = re.sub(r'%((?:struct|class|union)\.[\w.]+)', lambda m: '%' + renamed(m.group(1)), text)
    return re.sub(r'%"((?:struct|class|union)\.[\w.]+)"', r'%\1', text)


class Module:
    def __init__(self, text, label):
        if RENAMES:
            text = demangled_ir(text)
        self.label = label
        self.funcs = {}      # name -> set of normalised bodies
        self.globals = {}    # name -> set of normalised definitions (non-private)
        self.attrs = {}
        self.private = {}    # raw name -> raw rhs
        self.private_norm = {}
        lines = text.split('\n')
        defs = []
        for i, l in enumerate(lines):
            if l.startswith('attributes #'):
                k, v = l[len('attributes #'):].split(' = ', 1)
                self.attrs[k] = re.sub(r'\bnounwind ', '', v)   # inferred, and it follows a lambda's enclosing linkage
            elif l.startswith('@') and ' = ' in l:
                name, rhs = l.split(' = ', 1)
                raw = name[1:]
                if rhs.startswith('private '):
                    self.private[raw] = rhs
                else:
                    defs.append((raw, rhs))
        self._lines = lines
        for raw, rhs in defs:
            if rhs.startswith('external ') or ' external ' in rhs.split('[')[0].split('{')[0]:
                continue
            if raw.strip('"').startswith('_ZTS'):   # a type's RTTI name holds its lambdas' counters and its functions' linkage
                rhs = re.sub(r'(?<=Z)L(?=\d)', '', LAMBDA.sub('$_', rhs))
                rhs = re.sub(r'\[\d+ x i8\]', '[n x i8]', rhs)
            self.globals.setdefault(norm_name(raw), set()).add(self.norm(self.drop_linkage(rhs)))
        i = 0
        while i < len(lines):
            l = lines[i]
            if l.startswith('define '):
                j = i + 1
                while lines[j] != '}':
                    j += 1
                m = NAME.search(l[len('define '):])
                name = norm_name(m.group(1))
                head = self.norm(self.drop_linkage(l[len('define '):]))
                body = [self.norm(strip_comment(b)) for b in lines[i + 1:j]]
                self.funcs.setdefault(name, set()).add('\n'.join([head] + [b for b in body if b.strip()]))
                i = j + 1
                continue
            i += 1
        del self._lines

    @staticmethod
    def drop_linkage(s):
        return re.sub(r'\b(internal|private|linkonce_odr|weak_odr|linkonce|weak|available_externally|hidden|dso_local|'
                      r'local_unnamed_addr|unnamed_addr|comdat(?:\([^)]*\))?)\s+', '', s + ' ').rstrip()

    def private_ref(self, raw):
        if raw not in self.private_norm:
            self.private_norm[raw] = '<pending>'
            rhs = self.drop_linkage(self.private[raw])
            n = self.norm(rhs)
            short = re.sub(r'\s+', ' ', n)
            self.private_norm[raw] = '@P{' + (short if len(short) <= 80 else short[:60] + '..' +
                                              hashlib.sha1(n.encode()).hexdigest()[:10]) + '}'
        return self.private_norm[raw]

    def norm(self, s):
        def name(m):
            raw = m.group(1)
            key = raw[1:-1] if raw.startswith('"') else raw
            if key in self.private:
                return self.private_ref(key)
            return '@' + norm_name(raw)
        s = NAME.sub(name, s)
        s = ATTR.sub(lambda m: '#{' + self.attrs.get(m.group(1), '?') + '}', s)
        s = META.sub('', s)
        s = TYPE_SUFFIX.sub(r'\1', s)
        s = re.sub(r'%(class|struct|union)\.anon\b', r'%\1.anon', s)
        return s


OLD_INC = []


def compile_ir(path, out_dir, inc=()):
    out = os.path.join(out_dir, re.sub(r'[^\w.]', '_', path) + '.ll')
    if path.startswith('rev:') or path.startswith('rev='):
        rev, _, p = (path[4:].partition(':') if path.startswith('rev=') else ('HEAD', ':', path[4:]))
        src = subprocess.run(['git', 'show', '%s:%s' % (rev, p)], capture_output=True, text=True, check=True).stdout
        tmp = os.path.join(out_dir, 'rev_' + re.sub(r'[^\w.]', '_', p))
        # compiled from beside the original, so its own quoted includes resolve the same
        tmp = os.path.join(os.path.dirname(p), '.samecode_rev_' + os.path.basename(p))
        open(tmp, 'w').write(src)
        try:
            subprocess.run([CXX] + list(inc) + FLAGS + ['-o', out, tmp], check=True)
        finally:
            os.remove(tmp)
        return out
    if path.endswith('.ll'):
        return path
    subprocess.run([CXX] + list(inc) + FLAGS + ['-o', out, path], check=True)
    return out


def load(paths, out_dir, label, inc=()):
    with ThreadPoolExecutor(8) as ex:
        lls = list(ex.map(lambda p: compile_ir(p, out_dir, inc), paths))
    funcs, globs = {}, {}
    for ll in lls:
        m = Module(open(ll).read(), label)
        for k, v in m.funcs.items():
            funcs.setdefault(k, set()).update(v)
        for k, v in m.globals.items():
            globs.setdefault(k, set()).update(v)
    return funcs, globs


def first_diff(a, b, n=6):
    al, bl = a.split('\n'), b.split('\n')
    for k in range(max(len(al), len(bl))):
        x = al[k] if k < len(al) else '<end>'
        y = bl[k] if k < len(bl) else '<end>'
        if x != y:
            return ['    old: ' + x[:200], '    new: ' + y[:200], '    (line %d of %d / %d)' % (k + 1, len(al), len(bl))]
    return ['    (same lines)']


def main(argv):
    if '--' not in argv:
        raise SystemExit(__doc__)
    argv = list(argv)
    while '--rename' in argv:
        i = argv.index('--rename')
        old, _, new = argv[i + 1].partition('=')
        RENAMES.append((re.compile(r'(?<![\w:])' + re.escape(old) + r'(?!\w)'), new))
        del argv[i:i + 2]
    k = argv.index('--')
    old_paths, new_paths = argv[:k], argv[k + 1:]
    inc = [a for a in old_paths if a.startswith('-I')]
    old_paths = [a for a in old_paths if not a.startswith('-I')]
    with tempfile.TemporaryDirectory(dir=os.environ.get('SAMECODE_TMP')) as d:
        od, nd = os.path.join(d, 'old'), os.path.join(d, 'new')
        os.makedirs(od)
        os.makedirs(nd)
        of, og = load(old_paths, od, 'old', inc)
        nf, ng = load(new_paths, nd, 'new')
    init = {'__cxx_global_var_init', '_GLOBAL__sub_I'}
    missing = sorted(set(of) - set(nf) - init)
    added = sorted(set(nf) - set(of) - init)
    differ = sorted(k for k in set(of) & set(nf) if of[k] != nf[k] and k not in init)
    gmissing = sorted(set(og) - set(ng))
    gadded = sorted(set(ng) - set(og))
    gdiffer = sorted(k for k in set(og) & set(ng) if og[k] != ng[k])
    same = len(set(of) & set(nf)) - len(differ)
    print('samecode: %d functions the same, %d differ, %d missing, %d added; globals: %d differ, %d missing, %d added'
          % (same, len(differ), len(missing), len(added), len(gdiffer), len(gmissing), len(gadded)))
    if UNREAD:
        print('  (%d names the demangler could not read, compared mangled)' % len(UNREAD))
    for k in sorted(init):
        if of.get(k) != nf.get(k):
            print('  (start-up code differs: %s, %d old bodies, %d new)' % (k, len(of.get(k, ())), len(nf.get(k, ()))))
    shown = 0
    for k in differ:
        if shown >= 12:
            print('  ... and %d more' % (len(differ) - shown))
            break
        print('  differs: %s (%d old bodies, %d new)' % (k[:160], len(of[k]), len(nf[k])))
        a = sorted(of[k] - nf[k])
        b = sorted(nf[k] - of[k])
        if a and b:
            for line in first_diff(a[0], b[0]):
                print(line)
        shown += 1
    for lab, items in (('missing', missing), ('added', added), ('global differs', gdiffer), ('global missing', gmissing),
                       ('global added', gadded)):
        for k in items[:12]:
            print('  %s: %s' % (lab, k[:160]))
        if len(items) > 12:
            print('  ... and %d more %s' % (len(items) - 12, lab))
    return 0 if not (missing or added or differ or gmissing or gadded or gdiffer) else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
