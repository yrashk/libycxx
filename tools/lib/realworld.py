#!/usr/bin/env python3
"""libycxx test scripts: the real-world projects of tools/realworld.

    realworld.py plan REPO [NAME...]
        The projects to build, dependencies first: one line "NAME ROLE INSTALL" each, ROLE "test"
        (asked for: built and tested) or "dep" (only needed by one: built and installed), INSTALL
        "install" when another project of the plan needs it installed, else "-". No NAME: every
        project of tests/realworld.
    realworld.py get MANIFEST KEY [DEFAULT]
        A manifest value (tests/realworld/<name>/manifest: "key = value" lines, # comments).
    realworld.py select BUILD PROJECT_DIR OUT_DIR
        Which of the project's CTest tests run: every test but those tests/realworld/<name>/skip.txt
        names. Writes OUT_DIR/selection.json (every test with its number and, for a skipped or
        expected-failing one, xfail.txt's category and reason) and OUT_DIR/ctest-I.txt (the numbers to
        run, CTest's -I file; absent when every test runs). Prints "<to run> <skipped>".
    realworld.py build-check LOG PROJECT_DIR OUT_JSON
        After a failed build (ninja -k 0, whose output is LOG): exit 0 when only outputs that
        tests/realworld/<name>/build-skip.txt names failed (each with its category and reason,
        written to OUT_JSON for the report), else 1.
    realworld.py linkage REPO LIBDIR BUILD CMDLOG OUT_BASE [--expect-violations]
        The proof that a project was built against libycxx (see LINKAGE below). Writes
        OUT_BASE.txt (the evidence, for the report) and OUT_BASE.json. Exit 1 when anything is
        not libycxx's; with --expect-violations (the self-test: a program built with the
        toolchain's own C++ library), exit 1 unless every kind of check rejected it.
    realworld.py report REPO RESULTS_DIR OUT_BASE [KEY=VALUE...]
        The run's report: every project's build (its steps and their logs), linkage proof and
        tests, as build/test-logs/realworld-<config>.{html,md,tsv,data.json} (written by
        tools/lib/test_report.py, like the other suites' reports) and OUT_BASE.summary.md, one
        line per project. Exit 1 when anything failed.

LINKAGE: for each project and configuration, from the build tree and the records tools/ycxx-cxx
kept of every command it ran ($YCXX_CXX_LOG):
  - compile commands: every C++ translation unit of compile_commands.json was compiled by
    tools/ycxx-cxx, whose recorded command has -nostdinc++, -isystem <libycxx>/include, an
    effective -std=c++26 (or gnu++26), no -stdlib= and no include directory of libstdc++ or
    libc++ (".../include/c++/...", ".../c++/v1");
  - headers: the dependencies the compiler reported for each object (ninja -t deps) include no
    header of libstdc++ or libc++, and the C++ ones include libycxx's;
  - images (executables and shared libraries; on Linux ELF, on macOS Mach-O): no libstdc++ or
    libc++ among the needed libraries (readelf -d / otool -L); no libstdc++ or libc++ symbol,
    defined or undefined (std::__cxx11, __gnu_cxx::, std::__detail, any other std::__ name,
    which libycxx never uses, std::__1, GLIBCXX_/CXXABI_ symbol versions); and every image
    linked by the C++ driver defines libycxx's allocation table, __ycxx_allocation_functions,
    an exported symbol that only libycxx's runtime defines (DECISIONS §2);
  - static archives: no libstdc++ or libc++ symbol;
  - every test's executable is one of the checked images (or a script or a tool, named).
"""
import concurrent.futures, glob, html, json, os, re, shlex, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
CXX_EXT = ('.cpp', '.cc', '.cxx', '.c++', '.C', '.cppm', '.ixx', '.mpp', '.cu')
sys.path.insert(0, HERE)
from ycxx_linkage import (FORBIDDEN_DIR, FORBIDDEN_LIB, FORBIDDEN_DEMANGLED, FORBIDDEN_MANGLED,  # noqa: E402
                          MARKER, DARWIN, image_kind, run, needed_libs, symbols, sanitizer_runtime_symbols,
                          defines_marker, check_image, find_images)


# ---- manifests ----

def manifest(path):
    out = {}
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            if '=' not in line:
                raise SystemExit(f'{path}:{n}: expected "key = value"')
            k, v = line.split('=', 1)
            out[k.strip()] = v.strip()
    return out


def projects_dir(repo):
    return os.path.join(repo, 'tests', 'realworld')


def all_projects(repo):
    d = projects_dir(repo)
    return sorted(n for n in os.listdir(d) if os.path.isfile(os.path.join(d, n, 'manifest')))


def cmd_plan(repo, names):
    known = all_projects(repo)
    wanted = names or known
    for n in wanted:
        if n not in known:
            raise SystemExit(f'tools/realworld: no project "{n}" (tests/realworld/<name>/manifest); '
                             f'projects: {" ".join(known)}')
    order, state = [], {}

    def visit(n, chain):
        if state.get(n) == 'done':
            return
        if state.get(n) == 'visiting':
            raise SystemExit(f'tools/realworld: dependency cycle: {" -> ".join(chain + [n])}')
        state[n] = 'visiting'
        m = manifest(os.path.join(projects_dir(repo), n, 'manifest'))
        for d in m.get('deps', '').split():
            if d not in known:
                raise SystemExit(f'tools/realworld: {n} depends on unknown project "{d}"')
            visit(d, chain + [n])
        state[n] = 'done'
        order.append(n)

    for n in wanted:
        visit(n, [])
    needed = set()
    for n in order:
        needed.update(manifest(os.path.join(projects_dir(repo), n, 'manifest')).get('deps', '').split())
    for n in order:
        print(n, 'test' if n in wanted else 'dep', 'install' if n in needed else '-')


def conditions():
    """The configuration's conditions: the compiler, the platform, the sanitizers, shared
    ($YCXX_RW_CONDITIONS, set by tools/realworld: "gcc linux asan", "clang linux shared")."""
    return set(os.environ.get('YCXX_RW_CONDITIONS', '').split())


def substitute(v):
    """A manifest value's placeholders: @SRC@ (the project's sources), @SRC:<name>@ (one of its
    extra_sources), @PREFIX@ (where the projects it depends on are installed)."""
    src = os.environ.get('YCXX_RW_SRC', '')
    v = re.sub(r'@SRC:([A-Za-z0-9_.-]+)@', lambda m: f'{src}+{m.group(1)}', v)
    return v.replace('@SRC@', src).replace('@PREFIX@', os.environ.get('YCXX_RW_PREFIX', ''))


def cmd_get(path, key, default=None):
    m = manifest(path)
    # "key.gcc", "key.clang", "key.asan", ...: the value in that configuration.
    for c in sorted(conditions()):
        if f'{key}.{c}' in m:
            print(substitute(m[f'{key}.{c}']))
            return
    if key in m:
        print(substitute(m[key]))
    elif default is not None:
        print(default)
    else:
        raise SystemExit(f'{path}: no "{key}"')


# ---- test selection ----

CATEGORIES = {'libycxx-limitation', 'extension', 'implementation-specific', 'transitive-include',
              'pre-c++26', 'project-bug', 'flaky', 'compiler-bug', 'environment', 'infrastructure'}


def read_list(path):
    """skip.txt / xfail.txt / build-skip.txt: "<name regex> | <category> | <reason> [| <conditions>]"
    per line. The regex must match the whole name (a CTest test, or a build output); the entry
    applies only where every condition holds (gcc, clang, linux, darwin, asan, ubsan, tsan, shared
    for a run against libycxx's shared library, and no-ipv6 where the host cannot open an IPv6
    socket), and none of the ones written !name."""
    out = []
    if not os.path.isfile(path):
        return out
    cond = conditions()
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            s = line.strip()
            if not s or s.startswith('#'):
                continue
            parts = [p.strip() for p in s.split(' | ')]
            if len(parts) not in (3, 4) or not all(parts):
                raise SystemExit(f'{path}:{n}: expected "<regex> | <category> | <reason> [| <conditions>]"')
            if parts[1] not in CATEGORIES:
                raise SystemExit(f'{path}:{n}: unknown category "{parts[1]}" ({", ".join(sorted(CATEGORIES))})')
            if len(parts) == 4:
                want = parts[3].split()
                if not ({c for c in want if not c.startswith('!')} <= cond and
                        not {c[1:] for c in want if c.startswith('!')} & cond):
                    continue
            out.append({'re': re.compile(parts[0]), 'pattern': parts[0], 'category': parts[1],
                        'reason': parts[2], 'line': n, 'used': False})
    return out


FAILED = re.compile(r'^FAILED: (?:\[code=\d+\] )?(.*?)\s*$', re.M)


def cmd_build_check(log, pdir, out):
    """After a failed ninja -k 0: did only the outputs build-skip.txt names fail?"""
    skips = read_list(os.path.join(pdir, 'build-skip.txt'))
    with open(log, encoding='utf-8', errors='replace') as f:
        text = f.read()
    failed = [o for m in FAILED.finditer(text) for o in m.group(1).split()]
    if not failed:
        print('the build failed before building anything (see above)')
        return 1
    allowed, bad = [], []
    for o in failed:
        hit = next((s for s in skips if s['re'].fullmatch(o)), None)
        if hit:
            hit['used'] = True
            allowed.append({'output': o, 'category': hit['category'], 'reason': hit['reason']})
        else:
            bad.append(o)
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(allowed, f)
    for a in allowed:
        print(f'not built, as build-skip.txt says ({a["category"]}): {a["output"]}: {a["reason"]}')
    for o in bad:
        print(f'FAILED to build: {o}')
    return 1 if bad else 0


def ctest_tests(build):
    ctest = shlex.split(os.environ.get('YCXX_RW_CTEST', 'ctest'))
    r = subprocess.run(ctest + ['--test-dir', build, '--show-only=json-v1'], capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(f'ctest --show-only failed in {build}:\n{r.stderr}')
    # CTest prints "Could not find executable ..." on standard output for a test whose program
    # was not built (build-skip.txt) before the JSON document.
    out = r.stdout
    start = out.find('\n{') + 1 if not out.startswith('{') else 0
    return json.JSONDecoder().raw_decode(out[start:])[0].get('tests', [])


def cmd_select(build, pdir, out):
    os.makedirs(out, exist_ok=True)
    skips = read_list(os.path.join(pdir, 'skip.txt'))
    xfails = read_list(os.path.join(pdir, 'xfail.txt'))
    sel, run = [], []
    for i, t in enumerate(ctest_tests(build), 1):
        e = {'number': i, 'name': t['name'], 'command': t.get('command', [])}
        for kind, lst in (('skip', skips), ('xfail', xfails)):
            for s in lst:
                if s['re'].fullmatch(t['name']):
                    s['used'] = True
                    e[kind] = {'category': s['category'], 'reason': s['reason']}
                    break
        if 'skip' not in e:
            run.append(i)
        sel.append(e)
    # An entry that matches no test is stale (renamed or removed upstream): say so, loudly.
    stale = [f'{k}.txt:{s["line"]}: "{s["pattern"]}" matches no test'
             for k, lst in (('skip', skips), ('xfail', xfails)) for s in lst if not s['used']]
    with open(os.path.join(out, 'selection.json'), 'w', encoding='utf-8') as f:
        json.dump({'tests': sel, 'stale': stale}, f)
    ifile = os.path.join(out, 'ctest-I.txt')
    if os.path.exists(ifile):
        os.remove(ifile)
    if len(run) != len(sel):
        with open(ifile, 'w', encoding='utf-8') as f:
            f.write('0,0,0,' + ','.join(map(str, run)) + '\n')
    for s in stale:
        print(f'warning: {s}', file=sys.stderr)
    print(len(run), len(sel) - len(run))
    return 1 if stale else 0


# ---- linkage proof ----

def read_cmdlog(cmdlog):
    """tools/ycxx-cxx's records: (cwd, argv) per command it ran."""
    out = []
    # Oldest first: an incremental build's newer record of an output replaces the older one.
    for p in sorted(glob.glob(os.path.join(cmdlog, 'cmd.*')), key=os.path.getmtime):
        with open(p, 'rb') as f:
            parts = f.read().split(b'\0')
        if parts and parts[-1] == b'':
            parts.pop()
        if len(parts) >= 2:
            out.append((parts[0].decode(errors='replace'), [a.decode(errors='replace') for a in parts[1:]]))
    return out


def output_of(cwd, argv):
    for i, a in enumerate(argv):
        if a == '-o' and i + 1 < len(argv):
            return os.path.normpath(os.path.join(cwd, argv[i + 1]))
    return None


def include_dirs(argv):
    dirs = []
    flags = ('-I', '-isystem', '-idirafter', '-iquote', '-cxx-isystem', '-stdlib++-isystem')
    for i, a in enumerate(argv):
        for fl in flags:
            if a == fl and i + 1 < len(argv):
                dirs.append(argv[i + 1])
            elif a.startswith(fl) and len(a) > len(fl) and fl == '-I':
                dirs.append(a[2:])
    return dirs


def check_compile_record(argv, include):
    """Why a recorded compile command is not one against libycxx ([] when it is)."""
    bad = []
    if '-nostdinc++' not in argv:
        bad.append('no -nostdinc++')
    if not any(argv[i] == '-isystem' and os.path.normpath(argv[i + 1]) == include for i in range(len(argv) - 1)):
        bad.append(f'no -isystem {include}')
    stds = [a[5:] for a in argv if a.startswith('-std=')]
    if not stds or stds[-1] not in ('c++26', 'gnu++26', 'c++2c', 'gnu++2c'):
        bad.append(f'effective -std={stds[-1] if stds else "(none)"}, not C++26')
    if any(a.startswith('-stdlib=') or a.startswith('-stdlib++') for a in argv):
        bad.append('selects a C++ library: ' + ' '.join(a for a in argv if a.startswith('-stdlib')))
    for d in include_dirs(argv):
        if FORBIDDEN_DIR.search(d):
            bad.append(f'include directory of another C++ library: {d}')
    return bad


def tu_entries(build):
    path = os.path.join(build, 'compile_commands.json')
    if not os.path.isfile(path):
        return None
    with open(path, encoding='utf-8') as f:
        entries = json.load(f)
    out = []
    for e in entries:
        src = e.get('file', '')
        argv = e.get('arguments') or shlex.split(e.get('command', ''))
        if not src.endswith(CXX_EXT) and not any(a in ('-xc++', 'c++') for a in argv):
            continue
        d = e.get('directory', build)
        o = e.get('output') or output_of(d, argv)
        out.append({'file': os.path.normpath(os.path.join(d, src)),
                    'output': os.path.normpath(os.path.join(d, o)) if o else None,
                    'compiler': argv[0] if argv else ''})
    return out


def ninja_deps(build):
    """{object: [headers]} as the compiler reported them (ninja -t deps), or None without ninja."""
    if not os.path.isfile(os.path.join(build, 'build.ninja')):
        return None
    r = subprocess.run(['ninja', '-C', build, '-t', 'deps'], capture_output=True, text=True)
    if r.returncode != 0:
        return None
    deps, cur = {}, None
    for line in r.stdout.splitlines():
        if not line.strip():
            continue
        if not line.startswith(' '):
            cur = os.path.normpath(os.path.join(build, line.split(':', 1)[0]))
            deps[cur] = []
        elif cur is not None:
            deps[cur].append(os.path.normpath(os.path.join(build, line.strip())))
    return deps


def cmd_linkage(repo, libdir, build, cmdlog, out_base, expect_violations=False):
    include = os.path.normpath(os.path.join(repo, 'include'))
    report, problems, kinds = [], [], set()

    def problem(kind, text):
        kinds.add(kind)
        problems.append(text)

    records = read_cmdlog(cmdlog)
    compiles = {}
    links = set()
    for cwd, argv in records:
        o = output_of(cwd, argv)
        if not o:
            continue
        if any(a in ('-c', '-S') for a in argv):
            compiles[o] = argv
        elif not any(a in ('-E', '-M', '-MM', '-fsyntax-only') for a in argv):
            links.add(o)
    report.append(f'tools/ycxx-cxx ran {len(records)} commands: {len(compiles)} compilations, {len(links)} links '
                  f'(records in {cmdlog})')

    # 1. Compile commands.
    tus = tu_entries(build)
    if tus is None:
        problem('compile', f'no compile_commands.json in {build}')
    else:
        bad, unbuilt = [], 0
        for t in tus:
            argv = compiles.get(t['output'])
            if argv is None and t['output'] and not os.path.exists(t['output']):
                # Never compiled: a target outside the build (EXCLUDE_FROM_ALL), or a test that
                # must not compile and was not tried; nothing of it is in the build.
                unbuilt += 1
                continue
            if argv is None:
                bad.append(f'{t["file"]}: not compiled by tools/ycxx-cxx (compiler {t["compiler"]}, output {t["output"]})')
                continue
            why = check_compile_record(argv, include)
            if why:
                bad.append(f'{t["file"]}: ' + '; '.join(why))
        if bad:
            problem('compile', f'{len(bad)} of {len(tus)} C++ translation units not compiled against libycxx:\n  ' +
                    '\n  '.join(bad[:20]) + ('\n  ...' if len(bad) > 20 else ''))
        report.append(f'compile commands: {len(tus) - len(bad) - unbuilt} of {len(tus)} C++ translation units of '
                      f'compile_commands.json compiled by tools/ycxx-cxx with -nostdinc++ -isystem {include} '
                      f'-std=c++26 and no other C++ library\'s include directory' +
                      (f'; {unbuilt} not built (outside the build, no object)' if unbuilt else ''))

    # 2. Headers the compiler reported.
    deps = ninja_deps(build)
    if deps is None:
        report.append('headers: no ninja dependency information')
    else:
        cxx_objs = [o for o in deps if o in compiles or any(h.endswith(CXX_EXT) for h in deps[o][:1])]
        bad, with_ycxx = [], 0
        for o in deps:
            hs = deps[o]
            forb = [h for h in hs if FORBIDDEN_DIR.search(h)]
            if forb:
                bad.append(f'{o}: {forb[0]}' + (f' (+{len(forb) - 1})' if len(forb) > 1 else ''))
            if any(h.startswith(include + os.sep) for h in hs):
                with_ycxx += 1
        if bad:
            problem('headers', f'{len(bad)} objects include another C++ library\'s headers:\n  ' + '\n  '.join(bad[:20]))
        report.append(f'headers: {len(deps)} objects\' dependencies (ninja -t deps): {len(bad)} with a header of '
                      f'libstdc++ or libc++; {with_ycxx} of {len(cxx_objs)} C++ objects include libycxx headers '
                      f'from {include}')

    # 3. Images.
    images = find_images(build)
    results = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        futs = {pool.submit(check_image, p, k, os.path.normpath(p) in links): (p, k) for p, k in images}
        for fut in concurrent.futures.as_completed(futs):
            p, k = futs[fut]
            results[p] = (k,) + fut.result()
    n = {'exe': 0, 'shared': 0, 'archive': 0}
    marked, nlinked, cimages, local = 0, 0, 0, []
    for p in sorted(results):
        k, probs, ev = results[p]
        n[k] += 1
        if k != 'archive':
            if ev.get('marker'):
                marked += 1
            if ev.get('marker') == 'local':
                local.append(os.path.relpath(p, build))
            if os.path.normpath(p) in links:
                nlinked += 1
            elif not ev['cxx_symbols']:
                cimages += 1
        for pr in probs:
            kind = 'needed' if pr.startswith('needs') else 'symbols' if 'symbols' in pr else 'marker'
            problem(kind, f'{os.path.relpath(p, build)}: {pr}')
    sample = [p for p in sorted(results) if results[p][0] == 'exe' and os.path.normpath(p) in links][:1]
    report.append(f'images: {n["exe"]} executables, {n["shared"]} shared libraries, {n["archive"]} archives; '
                  f'{nlinked} linked by tools/ycxx-cxx, {marked} define {MARKER}, {cimages} without C++ code; '
                  f'none needs libstdc++/libc++ or has their symbols' if not (kinds & {'needed', 'symbols', 'marker'})
                  else f'images: {n["exe"]} executables, {n["shared"]} shared libraries, {n["archive"]} archives: '
                  'problems below')
    if local:
        report.append(f'  ({len(local)} of them keep the table local, by their version script or export list: '
                      f'{", ".join(local[:5])}{" ..." if len(local) > 5 else ""}; such a library uses its own '
                      'default allocation functions)')
    for p in sample:
        k, probs, ev = results[p]
        report.append(f'  e.g. {os.path.relpath(p, build)}: needs {", ".join(ev.get("needed", [])) or "nothing"}; '
                      f'defines {MARKER}: {ev.get("marker") or "no"}; {ev["ycxx_symbols"]} symbols of '
                      f'libycxx (__ycxx::)')

    # 4. Test executables.
    try:
        tests = ctest_tests(build) if os.path.isfile(os.path.join(build, 'CTestTestfile.cmake')) else []
    except SystemExit:
        tests = []
    checked = {os.path.realpath(p) for p in results}
    exes, others = set(), set()
    for t in tests:
        cmd = t.get('command') or []
        if not cmd:
            continue
        exe = cmd[0]
        if not os.path.isabs(exe):
            for prop in t.get('properties', []):
                if prop.get('name') == 'WORKING_DIRECTORY':
                    exe = os.path.join(prop['value'], exe)
        if os.path.realpath(exe) in checked:
            exes.add(exe)
        else:
            # A script, an interpreter or a tool (cmake -P, python, sh): it runs the project's
            # programs, which are among the images checked above.
            others.add(os.path.basename(exe))
    if tests:
        report.append(f'tests: {len(tests)} CTest tests run {len(exes)} distinct checked executables' +
                      (f'; others run {", ".join(sorted(others))}' if others else ''))
    status = 'FAIL' if problems else 'PASS'
    if problems:
        report.append('')
        report.append('PROBLEMS:')
        report += problems
    with open(out_base + '.txt', 'w', encoding='utf-8') as f:
        f.write('\n'.join(report) + '\n')
    with open(out_base + '.json', 'w', encoding='utf-8') as f:
        json.dump({'status': status, 'kinds': sorted(kinds), 'images': len(results), 'linked': nlinked,
                   'marked': marked, 'tus': len(tus or []), 'tests': len(tests), 'test_exes': len(exes)}, f)
    print('\n'.join(report))
    if expect_violations:
        want = {'compile', 'headers', 'needed', 'symbols', 'marker'}
        missing = want - kinds
        if missing:
            print(f'SELF-TEST FAILED: the checker did not reject: {", ".join(sorted(missing))}')
            return 1
        print(f'self-test: the checker rejected the program built with the toolchain\'s C++ library on '
              f'every count ({", ".join(sorted(want))})')
        return 0
    return 1 if problems else 0


# ---- results and report ----

def junit_tests(path):
    import xml.etree.ElementTree as ET
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError):
        return {}
    out = {}
    for tc in root.iter('testcase'):
        so = tc.find('system-out')
        fail = tc.find('failure')
        out[tc.get('name')] = {'status': tc.get('status', ''), 'time': float(tc.get('time') or 0),
                               'output': so.text if so is not None and so.text else '',
                               'message': fail.get('message', '') if fail is not None else ''}
    return out


def read_text(path, tail=None):
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            lines = f.read().splitlines()
    except OSError:
        return ''
    if tail and len(lines) > tail:
        return f'[... {len(lines) - tail} lines before]\n' + '\n'.join(lines[-tail:])
    return '\n'.join(lines)


def project_results(repo, pres, name):
    """The report's entries for one project: its build (steps), its linkage proof, its tests."""
    tests = []
    steps = []
    stepfile = os.path.join(pres, 'steps.tsv')
    if os.path.isfile(stepfile):
        with open(stepfile, encoding='utf-8') as f:
            for line in f:
                p = line.rstrip('\n').split('\t')
                if len(p) >= 5:
                    steps.append({'step': p[0], 'status': p[1], 'secs': float(p[2] or 0), 'log': p[3], 'command': p[4]})
    m = manifest(os.path.join(projects_dir(repo), name, 'manifest'))
    info = {'name': name, 'ref': m.get('ref', ''), 'commit': m.get('commit', '')[:12], 'steps': steps,
            'build': 'not run', 'linkage': 'not run', 'counts': {}, 'notes': m.get('notes', '')}
    # The build: fetch, configure, build, install.
    out, code, secs = [], 'PASS', 0.0
    built = False
    for s in steps:
        if s['step'] in ('test', 'linkage'):
            continue
        secs += s['secs']
        out.append(f'$ {s["command"]}')
        st = 'exit 0' if s['status'] == 'ok' else ('exit 130' if s['status'] == 'interrupted' else 'exit 1')
        out.append(f'[{s["step"]}: {st}, {s["secs"]:.1f}s]')
        out.append(read_text(s['log'], 25 if s['status'] == 'ok' else 300))
        if s['status'] != 'ok':
            code = 'FAIL'
        if s['step'] == 'build' and s['status'] == 'ok':
            built = True
    try:
        with open(os.path.join(pres, 'build-skips.json'), encoding='utf-8') as f:
            for a in json.load(f):
                tests.append({'name': f'realworld :: {name}/(build) {a["output"]}', 'code': 'UNSUPPORTED',
                              'elapsed': 0, 'output': f'skipped ({a["category"]}): not built: {a["reason"]}'})
    except (OSError, ValueError):
        pass
    if steps:
        tests.append({'name': f'realworld :: {name}/(build)', 'code': code, 'elapsed': secs,
                      'output': f'{name} {info["ref"]} ({m.get("url", "")} at {m.get("commit", "")})\n' + '\n'.join(out)})
        info['build'] = 'ok' if code == 'PASS' and built else ('ok (dependency)' if code == 'PASS' else 'FAIL')
    # The linkage proof.
    lk = [s for s in steps if s['step'] == 'linkage']
    if lk:
        s = lk[-1]
        ev = read_text(os.path.join(pres, 'linkage.txt')) or read_text(s['log'])
        code = 'PASS' if s['status'] == 'ok' else 'FAIL'
        tests.append({'name': f'realworld :: {name}/(linkage)', 'code': code, 'elapsed': s['secs'],
                      'output': f'$ {s["command"]}\n[linkage: exit {0 if code == "PASS" else 1}, {s["secs"]:.1f}s]\n{ev}'})
        info['linkage'] = 'ok' if code == 'PASS' else 'FAIL'
        try:
            with open(os.path.join(pres, 'linkage.json'), encoding='utf-8') as f:
                info['proof'] = json.load(f)
        except (OSError, ValueError):
            pass
    # The tests.
    selfile = os.path.join(pres, 'selection.json')
    ts = [s for s in steps if s['step'] == 'test']
    if os.path.isfile(selfile):
        with open(selfile, encoding='utf-8') as f:
            sel = json.load(f)
        ju = junit_tests(os.path.join(pres, 'junit.xml'))
        ran = bool(ts) and ts[-1]['status'] in ('ok', 'FAIL')
        for e in sel['tests']:
            tname = f'realworld :: {name}/{e["name"]}'
            cmd = ' '.join(shlex.quote(a) for a in e.get('command', []))
            if 'skip' in e:
                tests.append({'name': tname, 'code': 'UNSUPPORTED', 'elapsed': 0,
                              'output': f'skipped ({e["skip"]["category"]}): {e["skip"]["reason"]}'})
                continue
            j = ju.get(e['name'])
            if j is None:
                if ran or ts:
                    tests.append({'name': tname, 'code': 'UNRESOLVED', 'elapsed': 0,
                                  'output': 'not run: CTest reported no result for it' +
                                  (' (interrupted)' if ts and ts[-1]['status'] == 'interrupted' else '')})
                continue
            st = j['status']
            if st == 'disabled' or j['message'].startswith('Disabled'):
                tests.append({'name': tname, 'code': 'UNSUPPORTED', 'elapsed': 0,
                              'output': 'disabled by the project (the DISABLED test property)'})
                continue
            if st == 'notrun':
                code = 'UNRESOLVED'
            elif st == 'run':
                code = 'XPASS' if 'xfail' in e else 'PASS'
            elif j['message'] == 'Timeout':
                code = 'TIMEOUT'
            else:
                code = 'XFAIL' if 'xfail' in e else 'FAIL'
            why = f'expected to fail ({e["xfail"]["category"]}): {e["xfail"]["reason"]}\n' if 'xfail' in e else ''
            tests.append({'name': tname, 'code': code, 'elapsed': j['time'],
                          'output': f'{why}$ {cmd}\n[run: {"exit 0" if st == "run" else (j["message"] or st)}, '
                                    f'{j["time"]:.2f}s]\n{j["output"]}'})
        for s in sel.get('stale', []):
            tests.append({'name': f'realworld :: {name}/(stale skip or xfail entry)', 'code': 'FAIL', 'elapsed': 0,
                          'output': s + ': remove or update the entry'})
    counts = {}
    for t in tests:
        if t['name'].endswith(('/(build)', '/(linkage)')):
            continue
        counts[t['code']] = counts.get(t['code'], 0) + 1
    info['counts'] = counts
    return tests, info


GOOD = {'PASS', 'XFAIL'}
SKIP = {'UNSUPPORTED'}


def console_style():
    """(colour, unicode) for the terminal, decided as tools/lib/ui.sh decides: YCXX_COLOR=always|never|auto
    (tools/test passes always or never to the commands it runs), NO_COLOR, a terminal or GitHub
    Actions; box-drawing characters when the locale is UTF-8."""
    mode = os.environ.get('YCXX_COLOR', 'auto')
    if mode == 'always':
        colour = True
    elif mode == 'never':
        colour = False
    else:
        colour = not os.environ.get('NO_COLOR') and (sys.stdout.isatty() or os.environ.get('GITHUB_ACTIONS') == 'true'
                                                    or bool(os.environ.get('FORCE_COLOR')))
    # tools/realworld passes ui.sh's choice (YCXX_UNICODE): Python's own locale coercion (PEP 538) would
    # make a C locale look like UTF-8 here.
    if os.environ.get('YCXX_UNICODE') in ('0', '1'):
        return colour, os.environ['YCXX_UNICODE'] == '1'
    loc = os.environ.get('LC_ALL') or os.environ.get('LC_CTYPE') or os.environ.get('LANG') or ''
    return colour, 'utf-8' in loc.lower() or 'utf8' in loc.lower()


def console_table(infos):
    """The summary as a table for the terminal: box-drawn (UTF-8) or ASCII, coloured when colour is on,
    numbers right-aligned, a totals row. The Markdown summary (.summary.md) stays as it is."""
    colour, uni = console_style()
    sgr = (lambda code, t: f'\033[{code}m{t}\033[0m') if colour else (lambda code, t: t)
    head = ['project', 'ref', 'build', 'linkage', 'passed', 'skipped', 'failed', 'proof']
    right = {4, 5, 6}
    rows, tot = [], [0, 0, 0]
    for i in infos:
        c = i['counts']
        passed = c.get('PASS', 0) + c.get('XFAIL', 0)
        skipped = c.get('UNSUPPORTED', 0)
        failed = sum(v for k, v in c.items() if k not in GOOD | SKIP)
        tot = [tot[0] + passed, tot[1] + skipped, tot[2] + failed]
        pr = i.get('proof', {})
        proof = f'{pr.get("tus", 0)} TUs, {pr.get("linked", 0)} images, {pr.get("marked", 0)} marked' if pr else ''
        rows.append([i['name'], i['ref'], i['build'], i['linkage'], str(passed), str(skipped), str(failed), proof])

    def style(col, text, row):
        if col in (2, 3):
            return sgr('32', text) if text == 'ok' else sgr('1;31', text)
        if col == 4:
            return sgr('32', text) if text != '0' else sgr('2', text)
        if col == 5:
            return sgr('33', text) if text != '0' else sgr('2', text)
        if col == 6:
            return sgr('1;31', text) if text != '0' else sgr('2', text)
        if col == 0:
            return sgr('1', text)
        if col == 7:
            return sgr('2', text)
        return text

    total = ['total', '', '', '', str(tot[0]), str(tot[1]), str(tot[2]), f'{len(rows)} project' + ('' if len(rows) == 1 else 's')]
    widths = [max(len(r[k]) for r in [head, total] + rows) for k in range(len(head))]
    if uni:
        h, v, tl, tm, tr, ml, mm, mr, bl, bm, br = '─', '│', '┌', '┬', '┐', '├', '┼', '┤', '└', '┴', '┘'
    else:
        h, v, tl, tm, tr, ml, mm, mr, bl, bm, br = '-', '|', '+', '+', '+', '+', '+', '+', '+', '+', '+'

    def rule(left, mid, rgt):
        return sgr('2', left + mid.join(h * (w + 2) for w in widths) + rgt)

    def line(cells, styler):
        out = []
        for k, (cell, w) in enumerate(zip(cells, widths)):
            pad = ' ' * (w - len(cell))
            text = styler(k, cell)
            out.append(' ' + (pad + text if k in right else text + pad) + ' ')
        bar = sgr('2', v)
        return bar + bar.join(out) + bar

    out = [rule(tl, tm, tr), line(head, lambda k, t: sgr('1', t)), rule(ml, mm, mr)]
    out += [line(r, lambda k, t, r=r: style(k, t, r)) for r in rows]
    out += [rule(ml, mm, mr), line(total, lambda k, t: style(k, t, total) if k in (4, 5, 6) else sgr('1', t)),
            rule(bl, bm, br)]
    return '\n'.join('  ' + l for l in out)


def cmd_report(repo, results_dir, out_base, meta):
    tests, infos = [], []
    order_file = os.path.join(results_dir, 'order.txt')
    names = []
    if os.path.isfile(order_file):
        with open(order_file, encoding='utf-8') as f:
            names = [l.split()[0] for l in f if l.strip()]
    selftest = os.path.join(results_dir, 'selftest.txt')
    if os.path.isfile(selftest):
        with open(selftest, encoding='utf-8') as f:
            body = f.read()
        ok = body.rstrip().endswith('SELF-TEST PASSED')
        tests.append({'name': 'realworld :: (harness)/linkage checker self-test', 'code': 'PASS' if ok else 'FAIL',
                      'elapsed': 0, 'output': body})
    for n in names:
        pres = os.path.join(results_dir, n)
        if not os.path.isdir(pres):
            continue
        t, info = project_results(repo, pres, n)
        tests += t
        infos.append(info)
    elapsed = 0.0
    try:
        with open(os.path.join(results_dir, 'elapsed'), encoding='utf-8') as f:
            elapsed = float(f.read().strip() or 0)
    except (OSError, ValueError):
        pass
    data = {'tests': tests, 'elapsed': elapsed}
    with open(out_base + '.json', 'w', encoding='utf-8') as f:
        json.dump(data, f)
    # The summary: one line per project.
    lines = [f'# Real-world projects against libycxx: {meta.get("configuration", "")}', '',
             '| project | ref | build | linkage | tests passed | skipped | failed | proof |',
             '|---|---|---|---|---:|---:|---:|---|']
    bad = any(t['code'] not in GOOD | SKIP for t in tests)
    for i in infos:
        c = i['counts']
        passed = c.get('PASS', 0) + c.get('XFAIL', 0)
        skipped = c.get('UNSUPPORTED', 0)
        failed = sum(v for k, v in c.items() if k not in GOOD | SKIP)
        pr = i.get('proof', {})
        proof = (f'{pr.get("tus", 0)} TUs, {pr.get("linked", 0)} images linked, {pr.get("marked", 0)} with '
                 f'the allocation table' if pr else '')
        lines.append(f'| {i["name"]} | {i["ref"]} | {i["build"]} | {i["linkage"]} | {passed} | {skipped} | '
                     f'{failed} | {proof} |')
    with open(out_base + '.summary.md', 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    print(console_table(infos))
    args = [sys.executable, os.path.join(HERE, 'test_report.py'), out_base + '.json', out_base]
    args += [f'{k}={v}' for k, v in meta.items()]
    subprocess.run(args, check=False)
    return 1 if bad else 0


def main():
    a = sys.argv[1:]
    if not a:
        print(__doc__)
        return 2
    c = a[0]
    if c == 'plan':
        cmd_plan(a[1], a[2:])
        return 0
    if c == 'get':
        cmd_get(a[1], a[2], a[3] if len(a) > 3 else None)
        return 0
    if c == 'select':
        return cmd_select(a[1], a[2], a[3])
    if c == 'build-check':
        return cmd_build_check(a[1], a[2], a[3])
    if c == 'linkage':
        return cmd_linkage(a[1], a[2], a[3], a[4], a[5], '--expect-violations' in a[6:])
    if c == 'report':
        meta = dict(x.split('=', 1) for x in a[4:])
        return cmd_report(a[1], a[2], a[3], meta)
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main())
