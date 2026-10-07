#!/usr/bin/env python3
"""Compile the spec-coverage probes of an audit part and report every check that fails.

    tools/spec_audit/run_probes.py --part part2 [-c gcc|clang]... [-j N] [--only SUBSTR] [--show]

Each probe (tools/spec_audit/<part>/probes/*.cpp) is compiled with -fsyntax-only against
libycxx's headers (tools/ycxx-cxx; *.freestanding.cpp with -ffreestanding -nostdinc as
tools/check_freestanding.sh does). Every diagnostic is attributed to the probe line it is
about: the error's own line, or the line of the probe that the instantiation context or the
"required from here" note names. A line ends in `// @E<n> <aspect>`, so a failure is a check of
an entity (tools/spec_audit/<part>/entities.tsv). Known gaps (tools/spec_audit/<part>/gaps.tsv:
id, aspect, compiler (gcc, clang or any), class (1-7), reason) are reported as XFAIL; an
unlisted failure is FAIL, a listed check that passes is XPASS (also a failure). Writes
build/spec_audit/<part>-results.tsv; exit status 1 when anything FAILed or XPASSed.
"""
import argparse, collections, concurrent.futures, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MARK = re.compile(r'//\s*@(E\d+)\s+(\w+)\s*$')
LOC = re.compile(r'^(?P<file>[^:\s][^:]*):(?P<line>\d+):(?:\d+:)?\s*(?P<what>.*)$')


def marks(path):
    m = {}
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            x = MARK.search(line)
            if x:
                m[n] = (x.group(1), x.group(2))
    return m


def compile_probe(cc, path, libdir):
    if path.endswith('.freestanding.cpp'):
        cxx = os.environ.get('YCXX_GXX', 'g++-16') if cc == 'gcc' else os.environ.get('YCXX_CLANGXX', 'clang++-23')
        cmd = [cxx, '-std=c++26', '-ffreestanding', '-nostdinc', '-nostdinc++', '-isystem', os.path.join(ROOT, 'include'),
               '-fno-exceptions', '-fno-rtti', '-fsyntax-only', path]
    else:
        cmd = [os.path.join(ROOT, 'tools', 'ycxx-cxx'), cc, f'--libdir={libdir}', '-fsyntax-only', path]
        if '.hardened.' in os.path.basename(path):
            cmd.append('-DYCXX_HARDENED=1')
    cmd.append('-fmax-errors=0' if cc == 'gcc' else '-ferror-limit=0')
    if cc == 'clang':
        cmd.append('-fno-color-diagnostics')
    else:
        cmd.append('-fdiagnostics-color=never')
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def attribute(out, path, cc):
    """{probe line: [error messages]} for the errors of one compile."""
    base = os.path.basename(path)
    res = collections.defaultdict(list)
    lines = out.splitlines()
    ctx = []         # GCC: the probe lines of the instantiation context before an error
    i = 0
    while i < len(lines):
        l = lines[i]
        m = LOC.match(l)
        if not m:
            if l.startswith('In file included from') or l.lstrip().startswith('from '):
                pass
            i += 1
            continue
        f, ln, what = m.group('file'), int(m.group('line')), m.group('what')
        mine = os.path.basename(f) == base
        is_err = what.startswith(('error', 'fatal error')) or ' error:' in what[:20]
        if not is_err:
            if mine and cc == 'gcc' and ('required from' in what or 'required by' in what or 'required here' in what or what.startswith('note:') or 'in ' in what[:4]):
                ctx.append(ln)
            elif mine:
                ctx.append(ln)
            i += 1
            continue
        target = None
        if mine:
            target = ln
        elif cc == 'gcc' and ctx:
            target = ctx[-1]
        else:
            # Clang: the notes after the error name the probe line
            j = i + 1
            while j < len(lines):
                m2 = LOC.match(lines[j])
                if m2 and m2.group('what').startswith(('error', 'fatal error')):
                    break
                if m2 and os.path.basename(m2.group('file')) == base:
                    target = int(m2.group('line'))
                    break
                j += 1
            if target is None and ctx:
                target = ctx[-1]
        ctx = []
        res[target].append(what.strip())
        i += 1
    return res


def load_gaps(part):
    gaps = {}
    p = os.path.join(HERE, part, 'gaps.tsv')
    if os.path.exists(p):
        with open(p, encoding='utf-8') as f:
            for line in f:
                if line.startswith('#') or not line.strip():
                    continue
                c = line.rstrip('\n').split('\t')
                gaps[(c[0], c[1], c[2])] = (c[3], c[4] if len(c) > 4 else '')
    return gaps


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--part', required=True)
    ap.add_argument('-c', '--compiler', action='append')
    ap.add_argument('-j', '--jobs', type=int, default=2)
    ap.add_argument('--only')
    ap.add_argument('--show', action='store_true', help='print each failure with its first diagnostic')
    ap.add_argument('--libdir-root', default=os.environ.get('YCXX_BUILD', os.path.join(ROOT, 'build')))
    a = ap.parse_args()
    ccs = a.compiler or ['gcc', 'clang']
    pdir = os.path.join(HERE, a.part, 'probes')
    probes = sorted(os.path.join(pdir, f) for f in os.listdir(pdir) if f.endswith('.cpp') and (not a.only or a.only in f))
    gaps = load_gaps(a.part)
    jobs = [(cc, p) for cc in ccs for p in probes]
    results = []      # (cc, probe, id, aspect, status, message)
    unattributed = []
    checks = 0

    def one(job):
        cc, p = job
        libdir = os.path.join(a.libdir_root, cc)
        rc, out = compile_probe(cc, p, libdir)
        return cc, p, rc, out

    with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        for cc, p, rc, out in ex.map(one, jobs):
            mk = marks(p)
            errs = attribute(out, p, cc) if rc else {}
            if rc and not errs:
                unattributed.append((cc, p, out[:2000]))
            failed = {}
            for ln, msgs in errs.items():
                if ln in mk:
                    failed.setdefault(mk[ln], msgs[0])
                else:
                    unattributed.append((cc, p, f'line {ln}: ' + msgs[0]))
            for eid, asp in dict.fromkeys(mk.values()):
                checks += 1
                key = None
                for k in ((eid, asp, cc), (eid, asp, 'any')):
                    if k in gaps:
                        key = k
                if (eid, asp) in failed:
                    st = 'XFAIL' if key else 'FAIL'
                    results.append((cc, os.path.basename(p), eid, asp, st, failed[(eid, asp)] if not key else f'[{gaps[key][0]}] {gaps[key][1]}'))
                elif key:
                    results.append((cc, os.path.basename(p), eid, asp, 'XPASS', gaps[key][1]))
    outdir = os.path.join(ROOT, 'build', 'spec_audit')
    os.makedirs(outdir, exist_ok=True)
    with open(os.path.join(outdir, f'{a.part}-results.tsv'), 'w', encoding='utf-8') as o:
        for r in results:
            o.write('\t'.join(r) + '\n')
    cnt = collections.Counter(r[4] for r in results)
    for r in results:
        if r[4] != 'XFAIL' or a.show:
            print(f'{r[4]}: {r[0]} {r[1]} @{r[2]} {r[3]}: {r[5][:300]}' if a.show or r[4] != 'XFAIL' else '')
    for cc, p, msg in unattributed:
        print(f'FAIL: {cc} {os.path.basename(p)}: diagnostic outside any check: {msg[:500]}')
    print(f'{a.part}: {checks} checks over {len(probes)} probes x {len(ccs)} compilers: '
          f'{checks - len(results)} pass, {cnt["FAIL"] + len(unattributed)} FAIL, {cnt["XFAIL"]} XFAIL, {cnt["XPASS"]} XPASS')
    return 1 if cnt['FAIL'] or cnt['XPASS'] or unattributed else 0


if __name__ == '__main__':
    sys.exit(main())
