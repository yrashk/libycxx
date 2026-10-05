# Building, using and testing a custom C++ standard library

A method for building a replacement C++ standard library, delivering it to users, keeping it out
of the way of the platform's own C++ runtime, and testing it. It is derived from how libc++,
libstdc++, the MSVC STL and a few other projects do these things, compared stage by stage with
what libycxx does.

Sources were read on 2026-10-05; each claim about another project cites its source inline, and
[Sources](#9-sources) lists them all. Only documentation, build configuration, CI scripts and test
configuration were read, never the source or headers of another standard library or C++ runtime.
Every command and claim about libycxx was checked in a worktree on Linux (x86_64, GCC 16.2,
Clang 23.1) the same day; [Validation](#validation-record) records what was run. Nothing here was
run on macOS.

## 1. Purpose and audience

For people who maintain libycxx or a library like it: a standard library that is not the
toolchain's default, that programs select explicitly, and that is tested against the standard
rather than against itself. It assumes familiarity with `README.md`, `DECISIONS.md` and
`STATUS.md`, and does not repeat their rationale.

## 2. The method in brief

1. **Layer the library**: a freestanding core with no OS or C library, a hosted layer on top, and a
   small C-linkage platform layer between the hosted layer and the OS.
2. **Ship your own ABI runtime**, or pick one explicitly; never depend by accident on the
   toolchain's runtime (`libsupc++`, `libc++abi`).
3. **Build static archives with hidden visibility**, so that a program or shared object exports
   nothing of the library. Hide what the compiler keeps default (the implicit allocation
   functions, predeclared ABI entry points).
4. **Select the library with the generic driver flags**: `-nostdinc++` plus `-isystem <headers>`,
   and `-nostdlib++` plus the archives and their system dependencies. Do not rely on `-stdlib=`.
5. **Deliver the same flags three ways**, generated from one place where possible: a CMake package
   (an interface target), a wrapper script for everything else, and a toolchain file or
   provisioning script that finds the supported compilers.
6. **Test against the specification, not the implementation**: an own suite written from the
   working draft, plus other implementations' suites run unmodified, never copied into the
   repository.
7. **Make known failures explicit and machine-checked**: per-configuration baselines for "not done
   yet", expected-failure annotations for compiler gaps, skip lists with a category and a reason
   for tests that do not apply. An unexpected pass must fail the run.
8. **Run the suite in several configurations**: each compiler, each OS, sanitizers, and each
   library mode a user can select (hardening, no exceptions, no RTTI). Check the configurations
   that are not run.
9. **Validate the tests themselves** by running the own suite against another implementation, and
   triage every difference.
10. **Make every run explainable**: record the commands, exit statuses and output of passing tests
    as well as failing ones, and keep the reports as CI artifacts.
11. **Split CI**: a fast gate on every push, the whole external suites and the sanitizer runs
    nightly, each failing only on tests outside its baseline.

## 3. Building

### Layout

libycxx (`DECISIONS.md` §3) puts the freestanding core in `include/ycxx/core/**` (no OS, no libc
headers, no heap unless an allocator is supplied), the hosted layer in `include/ycxx/hosted/**`
and `src/hosted`, and the platform layer behind `include/ycxx/pal.h` (C-linkage hooks such as
`ycxx_pal_allocate`, `ycxx_pal_wait`; `src/pal/posix`). The public headers have the standard names
(`include/vector`, ...). `tools/check_includes.py` enforces the layering and
`tools/check_freestanding.sh` compiles every core and freestanding header with
`-ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti` and links a smoke program for bare
metal (x86_64 and riscv64 with Clang, x86_64 with GCC).

Others: libc++ is built from the LLVM monorepo's `runtimes` directory, with libc++abi and
libunwind as sibling projects (`-DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind"`), and
installs headers under `<prefix>/include/c++/v1` [libcxx-vendor]. libstdc++ is built only as part
of GCC; a freestanding subset is a configure option (`--disable-hosted-libstdcxx`)
[libstdcxx-configure]. The MSVC STL builds with CMake presets (`cmake --preset x64`) [msvc-readme].
None of these documents describes a platform-abstraction layer comparable to libycxx's PAL; not
confirmed either way.

### Freestanding and hosted

libycxx builds two hosted archives with CMake (`libycxx.a`, `libycxx-abi.a`) and a third,
freestanding runtime archive (allocation functions without a heap, `std::nothrow`, floating-point
`<charconv>`, the atomic lock tables, replaceable hooks). Every replaceable function is alone in
an archive member, so a program can replace any subset (DECISIONS §3). The freestanding archive is
built only by `tools/check_freestanding.sh` (`build_fsrt`), not by CMake: it is neither installed
nor exported by the package (see [Gaps](#8-gaps-and-proposals), item 8).

libstdc++'s freestanding mode is a separate configuration of the whole library
[libstdcxx-configure]; libc++ has feature switches such as `LIBCXX_ENABLE_FILESYSTEM` and
`LIBCXX_ENABLE_EXCEPTIONS` [libcxx-vendor] and CMake caches such as
`Generic-no-localization.cmake` [libcxx-caches]. libycxx's split is by layer, so one build serves
both modes; the price is that the freestanding archive needs its own build path.

### The ABI runtime

libycxx has its own Itanium ABI runtime (`src/abi`, `libycxx-abi.a`: exceptions, RTTI,
`__dynamic_cast`, guards) and takes only stack unwinding from the toolchain (`libgcc_s`; on
Darwin libSystem's unwinder) (DECISIONS §4). libc++ instead selects an ABI library at
configure time: `LIBCXX_CXX_ABI` is one of `none`, `libcxxabi`, `system-libcxxabi`, `libcxxrt`,
`libstdc++`, `libsupc++`, `vcruntime`, and `LIBCXX_ENABLE_STATIC_ABI_LIBRARY` links it statically
[libcxx-vendor]. Owning the runtime is what lets libycxx hide it per image (section 5); borrowing
one is far less work but ties the library to that runtime's exported symbols.

### Archives versus shared libraries

libycxx builds static archives only, position-independent so they can go into shared objects
(`CMakeLists.txt`). The two archives refer to each other; CMake links the pair repeatedly
(`LINK_INTERFACE_MULTIPLICITY 3`), and `tools/ycxx-cxx` wraps them in
`-Wl,--start-group ... --end-group` on ELF.

libc++ builds both by default (`LIBCXX_ENABLE_SHARED`, `LIBCXX_ENABLE_STATIC`, both ON)
[libcxx-vendor]. Shipping a shared library makes the exported symbol set an ABI: libstdc++
versions it with a linker version script (`GLIBCXX_3.4.x`, `CXXABI_1.3.x`, `--enable-symvers`) and
checks it against a baseline with `make check-abi` [libstdcxx-abi]; libc++ marks exported entities
with visibility macros (`_LIBCPP_EXPORTED_FROM_ABI`, `_LIBCPP_HIDE_FROM_ABI`) [libcxx-visibility]
and has `LIBCXX_ABI_VERSION`/`LIBCXX_ABI_NAMESPACE` [libcxx-vendor]. Android ships libc++ as both
`libc++_shared.so` and `libc++_static.a` and allows the static one only when an app has exactly
one shared library [android-cpp].

Recommendation: while the library has no stable ABI, ship static archives only, with every
symbol hidden (below). A shared library is worth its cost (a versioned export list, a symbol
baseline check, an inline ABI namespace) only once ABI stability is promised.

### Visibility

libycxx hides everything (DECISIONS §2): every file-scope opening of `std` and `ycxx` is
`namespace [[gnu::visibility("hidden")]] std {` (enforced by `tools/check_visibility.py`), the
archives are built with `-fvisibility=hidden`, and what the compilers keep default is hidden with
assembler directives: the implicitly declared allocation functions, GCC's predeclared `__cxa_*`
entry points, and on ELF GCC's fundamental type_info objects.

Others: libc++ offers `LIBCXX_HERMETIC_STATIC_LIBRARY`, "Do not export any symbols from the static
libc++ library" [libcxx-vendor]. Clang has `-fvisibility-global-new-delete=` (`force-default`,
`force-protected`, `force-hidden`, `source`) [clang-ref]; it was added (as
`-fvisibility-global-new-delete-hidden`) because the implicit declarations of `operator new` and
`delete` always had default visibility, citing libFuzzer's internal libc++ and Fuchsia [D53787].
Chromium passes it when building its static libc++ on Apple platforms with Clang, and on other
platforms deliberately keeps the allocation functions default because "elf visibility rules
require that linkers use the least visible form when merging" and Chromium's own allocator must
intercept allocations from other shared libraries [chromium-libcxx-gn]. Checked here: GCC 16.2 does
not accept `-fvisibility-global-new-delete=` (`unrecognized command-line option`), which is why
libycxx uses assembler directives instead.

## 4. Using

### Compiler flags

The generic recipe is the same for every replacement library: drop the toolchain's C++ include
directories and add yours, drop the toolchain's C++ library and add yours. libc++ documents it for
a custom installation as
`clang++ -nostdinc++ -isystem <install>/include/c++/v1 -nostdlib++ -L <install>/lib -lc++ -Wl,-rpath,<install>/lib`
[libcxx-vendor].

What each flag does, checked with GCC 16.2 and Clang 23.1 on this machine:

| Flag | GCC 16.2 | Clang 23.1 |
|---|---|---|
| `-nostdinc++` | drops `include/c++/16.2.0`, its target and `backward` directories; keeps the compiler's `include`, `include-fixed` and the system directories. `#include <vector>` then fails ("No such file or directory"). | drops the selected GCC installation's libstdc++ directories (here GCC 13's, since no `--gcc-install-dir` was given); keeps the resource directory and the system directories |
| `-isystem <libycxx>/include` | searched before every standard directory | the same: before Clang's resource directory |
| `-nostdlib++` | drops `-lstdc++` **and `-lm`**; keeps `-lgcc_s -lgcc -latomic_asneeded -lc` | drops `-lstdc++` only; `-lm` stays |
| `-stdlib=` | rejected (`unrecognized command-line option`): GCC accepts it only "when G++ is configured to support this option" [gcc-dialect] | `libc++`, `libstdc++` or `platform` [clang-ref]; none of them names a third library |
| `-stdlib++-isystem <dir>` | rejected | replaces the C++ library include path [clang-ref]. Checked: without `-nostdinc++` the directory takes the place of the libstdc++ directories (before the resource directory); with `-nostdinc++` it is dropped as well |
| `--gcc-install-dir=<dir>` | n/a | selects the GCC installation for crt files, `libgcc` and (with `-stdlib=libstdc++`) libstdc++; "executables (e.g. ld) used by the compiler are not overridden" [clang-ref] |
| `-fvisibility-global-new-delete=` | rejected | accepted [clang-ref] |

So the portable choice for both compilers is `-nostdinc++ -isystem` and `-nostdlib++`, plus the
libraries the driver no longer adds (`-lm` for GCC). GCC's manual describes `-nostdinc++` as "used
when building the C++ library" [gcc-dialect]; `-isystem` directories are scanned after `-I` ones
and before the standard system directories, and are treated as system headers [gcc-dirs].

libycxx's link line on Linux, from `tools/ycxx-cxx clang -###`:
`... -nostdlib++ --start-group build/clang/libycxx.a build/clang/libycxx-abi.a --end-group -lm -shared-libgcc`
plus `-Wl,-rpath` to GCC 16's `libgcc_s`; for Clang also `--gcc-install-dir=` GCC 16's
installation. The resulting program needs only `libm.so.6`, `libgcc_s.so.1`, `libc.so.6` and the
dynamic loader (checked with `readelf -d`), and its dynamic symbol table holds no `_Z*`, `__cxa_*`
or `__gxx_*` symbol. `-shared-libgcc` is there because glibc's `pthread_exit`/`pthread_cancel`
unwind through `libgcc_s.so`, and a second, static unwinder in the program would abort
(`CMakeLists.txt`); GCC's manual also recommends the shared libgcc when exceptions cross shared
libraries [gcc-link].

GCC warns (`-Wattributes`) about program classes with a member of a library class type:
`struct S { std::string s; };` gives "'S' declared with greater visibility than the type of its
field 'S::s'" with GCC 16.2, nothing with Clang 23.1 (checked). The package and the wrapper pass
`-Wno-attributes` to GCC; other build systems must add it (STATUS, Known limitations).

### The wrapper

`tools/ycxx-cxx gcc|clang <args>` compiles and links against `build/<compiler>` (or
`$YCXX_LIBDIR`). It adds `-std=c++26 -nostdinc++ -isystem <repo>/include`, the archives only when
linking (not with `-c`, `-S`, `-E`, `-fsyntax-only`, `-M`, `-MM`), and the platform specifics:
Apple's linker without `--start-group`, `-static-libgcc` for GCC on Darwin, `-latomic` dropped where
the toolchain has none. `tools/ref-cxx` is its counterpart for the toolchain's libstdc++ (reference
runs, section 6). Checked:

```sh
tools/ycxx-cxx gcc   -O2 examples/demo.cpp -o demo-gcc   && ./demo-gcc     # libycxx example: ok
tools/ycxx-cxx clang -O2 examples/demo.cpp -o demo-clang && ./demo-clang   # libycxx example: ok
```

Others: the MSVC STL generates `set_environment.bat`, which puts the built `inc` and `lib`
directories first in `INCLUDE`, `LIB` and `PATH` [msvc-readme]; libc++ provides
`libcxx/utils/libcxx-lit` as a test wrapper but documents the raw flags for users [libcxx-testing,
libcxx-vendor]. A wrapper is the right tool for scripts and test harnesses; the flags themselves
should also be documented, as libc++ does, for build systems that cannot call a wrapper.

### CMake package

`find_package(libycxx CONFIG REQUIRED)` and `target_link_libraries(app PRIVATE ycxx::ycxx)`.
`ycxx::headers` carries `-nostdinc++`, the C++26 flag, the include directory as `SYSTEM`, and
`-Wno-attributes` for GCC; `ycxx::ycxx` adds the archives, `-lm` (not on Apple), `-nostdlib++` and
on Linux `-shared-libgcc`. The package configuration rejects a consumer compiler older than GCC 16.2
or Clang 23 with a message. Checked in the generated `build.ninja` of `examples/find_package`
(GCC): `FLAGS = -O3 -DNDEBUG -nostdinc++ -std=c++26 -Wno-attributes`,
`LINK_FLAGS = -nostdlib++ -shared-libgcc`, the archive pair three times, `-lm`.
`tests/cmake/run.sh` builds, installs and uses the package with both compilers (all checks
passed; see [Validation](#validation-record)).

The package does not pass `--gcc-install-dir` for Clang, while `tools/ycxx-cxx` and the toolchain
file do. A Clang consumer that uses the package without the toolchain file links the crt files and
`libgcc` of whatever GCC installation Clang finds (here GCC 13's). It works, but the three
delivery paths differ (Gaps, item 10).

### Toolchain file and provisioning

`tools/toolchain/provision` finds GCC 16.2 and Clang 23.1 (in its cache, on `PATH`, in Homebrew's
kegs) or installs them (Clang from the LLVM release tarball, GCC built from source), writing
`toolchains.env`; `activate.sh`/`activate.fish` export `YCXX_*` variables; and
`cmake/ycxx-toolchain.cmake` does the same from CMake against the same cache, downloading only with
`-DYCXX_PROVISION=ON`. It adds `--gcc-install-dir` to Clang's initial flags and propagates its
settings to `try_compile` projects through `CMAKE_TRY_COMPILE_PLATFORM_VARIABLES`; CMake loads a
toolchain file "early to set values for the compilers", and the file is used again in other
contexts such as `try_compile()` [cmake-toolchains].

Others: libc++ and libstdc++ are built by and for the toolchain they come with; the bootstrapping
build of libc++ builds Clang first [libcxx-vendor]. Chromium pins its own libc++
(`use_custom_libcxx = true`) and warns that "Bringing your own C++ standard library is deprecated"
[chromium-cxx-gni]. A library that needs the newest compilers, as libycxx does, needs this
provisioning step; it is not a feature others document.

### macOS

On Darwin the archives are linked into the executable and bound there (two-level namespace);
Apple's linker searches archives repeatedly, so no group is needed; Clang's Apple arm64 ABI marks
type_info objects that may be duplicated across images by setting bit 63 of the name pointer,
which the runtime clears (DECISIONS §4, STATUS "macOS"). STATUS says that nothing of the macOS port
had run on macOS when it was written; CI runs a `macos` job. Not checked here.

## 5. Coexisting with another C++ runtime in the process

A replacement library rarely owns the process. On Darwin, libSystem loads Apple's libc++ and
libc++abi into every process (DECISIONS §2); on Linux a plugin or system library may bring
libstdc++. Three mechanisms decide who binds to what:

- **ELF symbol interposition.** A shared object's exported symbols can be pre-empted by the
  program's or another object's. `-Bsymbolic` binds a shared library's references to its own
  definitions "if any" [ld-options]; `--exclude-libs` treats an archive's symbols as hidden
  [ld-options]; a version script can reduce symbols to `local:` scope [ld-version]. All three act at
  link time of each image, so every consumer must remember them.
- **Darwin weak-definition coalescing.** "if weak symbols are not hidden, the dynamic loader needs
  to make them unique at runtime"; since OS X 10.6 dyld binds by two-level namespace first, then
  coalesces duplicate weak symbols in a separate pass [dyld-relnotes]. A developer reports that weak
  definitions such as template type_info are overridden across images despite two-level namespace
  [apple-forum-693914]. libycxx observed exactly this: `std::current_exception` and, with GCC, the
  exception classes' type_info bound to libc++'s, so `catch (const std::exception&)` stopped
  matching (DECISIONS §2).
- **One runtime per process, by rule.** Android requires one C++ runtime per app: with the static
  libc++ in two shared libraries, "the runtime behavior of this application is undefined, and in
  practice crashes are very common", including exceptions that go uncaught across libraries and
  memory freed in the wrong library [android-cpp].

libc++ and Chromium separate their names with an ABI namespace instead: `LIBCXX_ABI_NAMESPACE`
[libcxx-vendor], `std::__Cr` in Chromium's build [pdfium-issue-176]; a static Chromium libc++ gets
`-fvisibility-global-new-delete=force-hidden` on Apple platforms [chromium-libcxx-gn]. libstdc++'s
inline `__cxx11` namespace serves its dual ABI, not coexistence [libstdcxx-abi].

libycxx's model (DECISIONS §2) is hidden visibility for everything, including the ABI runtime, so
each image that links libycxx has its own runtime, bound inside it, and exports nothing. Exceptions
still cross between libycxx images (type_info compared by name, the uncaught count kept by the
runtime that added the exception), and libycxx and libstdc++ or libc++ coexist with separate
runtimes. What is not shared is documented: each image has its own `uncaught_exceptions()` count,
error categories, default memory resources and replacement `operator new`. `tests/cmake/run.sh`
checks it: a program and a shared library built with libycxx, each in one process with a shared
library built with the toolchain's library ("mine 3 other 3"), a program catching a libycxx shared
library's exceptions ("caught 15 uncaught 0 0"), and no exported libycxx symbol in any image (all
passed with both compilers; ELF only here).

Compared with the alternatives: an ABI namespace alone keeps names apart but still exports them
(and their weak definitions coalesce within one library's own copies); `-Bsymbolic` and version
scripts protect only images whose link line remembers them; Android's rule forbids the case.
Hiding at the declaration, as libycxx does, protects every image by default. Its costs are real
and documented: GCC's `-Wattributes` warning, nothing shareable by plugins, and per-image
singletons.

### Replacement allocation functions and sanitizers

The library's default `operator new`/`delete` live in their own archive members, so a program's
replacement is linked instead ([replacement.functions]; DECISIONS §3). A sanitizer runtime is a
second replacement: AddressSanitizer's run-time library "replaces the malloc and free functions"
[asan-algorithm] and checks allocation/deallocation pairing (`alloc_dealloc_mismatch`,
`new_delete_type_mismatch`) [asan-flags]. Checked here: in a program linked with
`tools/ycxx-cxx clang -fsanitize=address`, the linker map shows `operator new(unsigned long)` taken
from `libclang_rt.asan_cxx-x86_64.a(asan_new_delete.cpp.o)`, not from `libycxx.a`; the program
runs. libycxx's own suite marks the tests this changes (`new/*` forwarding and new_handler tests,
`linkage/*` export tests) `// UNSUPPORTED-SANITIZER: asan` with the reason. GCC 16.2 here has no
ASan runtime (`cannot find -lasan`), so sanitizer runs are Clang-only (STATUS).

The rule that follows: test allocation-function behaviour without sanitizers, and mark, not
delete, the tests a sanitizer runtime invalidates. The MSVC STL does the same in its expected
results, under an "ASAN FAILURES" section [msvc-expected].

## 6. Testing

### The own suite, derived from the specification

libycxx's own suite (`tests/ycxx`, 2328 tests as lit counts them at this commit) is written by an author who may
read only the working draft and cppreference.com, never another implementation's tests or any
implementation; a failing test is a library bug until the draft shows otherwise, and tests are
never weakened (DECISIONS §6). The lit format (`tests/ycxxlit/ycxx_format.py`) knows
`*.pass.cpp` (compile, link, run), `*.compile.pass.cpp` (must compile) and `*.compile.fail.cpp`
(must not compile, for a reason other than a missing header), with directives `FLAGS`, `FILES`,
`ARCHIVE`, `SHARED` (multi-image tests), `UNSUPPORTED-SANITIZER` and `XFAIL-COMPILER`. Run it as:

```sh
tools/test -c clang -f optional ycxx     # one directory, one compiler
tools/run-conformance ycxx gcc optional  # the same stage, directly
```

Others: libc++'s test kinds are richer: `.pass.cpp`, `.compile.pass.cpp`, `.compile.fail.cpp`,
`.verify.cpp` (diagnostics checked with clang `-verify`, "automatically marked as UNSUPPORTED if
the compiler does not support clang-verify"), `.link.pass.cpp`, `.link.fail.cpp`, `.sh.cpp`
(lit shell), `.gen.cpp` (generated tests), `.bench.cpp`; directives `ADDITIONAL_COMPILE_FLAGS`,
`FILE_DEPENDENCIES`, `MODULE_DEPENDENCIES`, and lit's `XFAIL`/`UNSUPPORTED`/`REQUIRES` against
features such as `std-at-least-c++26` [libcxx-testing]. Lit evaluates those directives against a
suite's `available_features` [lit]. libstdc++ uses DejaGnu directives in comments: `dg-do
run|compile|link`, `dg-options`, `dg-additional-options`, `dg-require-effective-target`,
`dg-error`/`dg-warning` with a message and line, `dg-xfail-run-if`, `dg-timeout-factor`
[libstdcxx-test]. The MSVC STL has its own `tests/std` and `tests/tr1` and runs libc++'s tests
[msvc-readme].

What libycxx's own suite lacks against these: a diagnostic-matching kind (`.verify.cpp`,
`dg-error "message"`): a `.compile.fail.cpp` passes on any error that is not a missing header, so a
test can pass for the wrong reason; and feature-based constraints: its directives name a compiler
or sanitizer directly, where lit features would let a test say `REQUIRES: hardened` or
`UNSUPPORTED: darwin, no-exceptions`. Gaps, items 2 and 4.

### External suites: run only, never vendored

`tools/fetch-suites` downloads libc++'s `libcxx/test/std` and `libcxx/test/support` from LLVM
23.1.2 (sparse, shallow clone) and libstdc++'s `testsuite` from the GCC 16.2.0 tarball (only that
directory is extracted) into `~/.local/share/ycxx/suites`; `tools/test` fetches on demand. The
suites are interpreted by libycxx's own lit formats (`tests/ycxxlit/libcxx_format.py`,
`libstdcxx_format.py`), which implement the subset that matters for conformance: for libc++ the
file kinds, `ADDITIONAL_COMPILE_FLAGS` (dropping `-D_LIBCPP*`), `FILE_DEPENDENCIES`, and
`REQUIRES`/`UNSUPPORTED` against a feature set in `tests/libcxx/lit.cfg.py`; tests with `RUN:` lines,
`.sh.cpp` and generated tests are UNSUPPORTED; a `.verify.cpp` must fail to compile if it has an
active `expected-error`, and is UNSUPPORTED otherwise (warning-only). For libstdc++: `dg-do`,
`dg-options`, effective targets, `dg-error` (any applicable one means compilation must fail;
messages are not matched), `dg-xfail-run-if`.

Others: the MSVC STL runs libc++'s suite from `llvm-project\libcxx\test` in its checkout (cloned
with `--recurse-submodules`), through `stl-lit.py` [msvc-readme]. libc++'s own harness can test another library: `--param stdlib=` takes
`llvm-libc++`, `apple-libc++`, `libstdc++` or `msvc` [libcxx-params], and
`libcxx/test/configs/stdlib-libstdc++.cfg.in` runs the suite against an installed libstdc++
(`--param libstdcxx_install_prefix=... libstdcxx_version=... libstdcxx_triple=...`)
[libcxx-stdlib-libstdcxx-cfg]. libstdc++'s suite can run against an installed library with
`runtest --tool libstdc++ --srcdir=...` [libstdcxx-test]. Neither documents running against a
third library; libycxx's formats are its own, not confirmed equivalents.

Recommendation: keep external suites out of the repository and pin their versions; interpret their
directives with your own small format rather than their full harness (whose substitutions assume
their library), and report every construct you do not interpret as UNSUPPORTED with the reason, so
that the count of what was not run stays visible. Checked:

```sh
tools/test --baseline -c gcc -f numerics/numbers libcxx
#   4 passed, 1 failed (numerics/numbers/value.pass.cpp, listed in tests/libcxx/baseline/linux-gcc.txt)
#   "baseline .../linux-gcc.txt: 200 known; this run: 1 did not pass, 0 outside the baseline,
#    0 of the baseline now pass" -> exit 0
```

### Triage and known failures

libycxx separates four kinds of "does not pass", each in its own file with a reason:

| Kind | Where | Effect |
|---|---|---|
| Does not apply (removed feature, divergence from the draft, extension, implementation-specific, infrastructure) | `tests/<suite>/skip.txt`, `tests/common/skip.txt`; categories in `tests/SKIPPED.md` | UNSUPPORTED with the reason |
| Compiler gap | `// XFAIL-COMPILER:` in the own test; `tests/<suite>/xfail.txt` for external suites | XFAIL; XPASS fails the run |
| Not passing yet, with a known cause | `tests/<suite>/baseline/<os>-<compiler>[-<sanitizer>].txt`, explained in `TRIAGE.md` (categories A-F) and STATUS | with `--baseline`, a failure outside the list fails the run; listed tests that now pass are named |
| Reference-run difference | `tests/ycxx/REFERENCE.md` | documentation only |

The baseline is produced, not written: each run writes `build/test-logs/<run>.baseline.txt`, and
copying it over the baseline file records or updates it (`tools/lib/baseline.py`).
`tools/triage.py` groups failures by missing header or first error message.

Others: the MSVC STL keeps one hand-maintained `expected_results.txt` per suite, lines
`<test>[:<configuration>] FAIL|SKIPPED`, grouped by cause (issues reported upstream, slow tests,
ASan failures, missing STL features, CRT bugs, likely bogus tests, likely STL bugs, not yet
analyzed) [msvc-expected]; an XPASS requires updating it [msvc-readme]. lit has `--xfail` /
`LIT_XFAIL` for marking tests as expected failures without editing them [lit]. libstdc++ expresses
expected failures in the test (`xfail` selectors, `dg-xfail-run-if`) [libstdcxx-test].

libycxx does better: its baseline is a list of known failures per OS, compiler and sanitizer,
generated from a run, which never counts as "pass"; a recorded failure that starts passing is
reported (not failed), and a missing baseline means every failure counts. The MSVC file does
better at keeping the cause next to each entry; libycxx keeps causes in `TRIAGE.md`, which can
drift from the list.

At this commit the baselines are incomplete: `tests/libcxx/baseline` has `darwin-clang.txt` and
`linux-gcc.txt` only, and `tests/ycxx/baseline` has Linux files only. CI runs
`--baseline` for libc++ on Linux with Clang (`full.yml`) and for the own suite on macOS
(`ci.yml`), where with no file every failure counts, including the documented ones
(STATUS: `except/handler_*` are expected to fail on macOS). Gaps, item 1.

### Sanitizers

`tools/test -s asan,ubsan` (or `SANITIZER=asan,ubsan tools/run-conformance ...`) adds
`-fsanitize=... -fno-sanitize-recover=all -g` to the tests, and `tsan` with a library built with
`-fsanitize=thread` (`YCXX_LIBDIR=build/clang-tsan`). Lit features `asan`/`ubsan` are set for the
libc++ suite. Nightly CI runs the own suite with Clang under ASan+UBSan against its own baseline
(`linux-clang-asan-ubsan.txt`). Checked: `tools/test -c clang -s asan,ubsan -f optional ycxx`,
37 passed.

Others: libc++ has `--param use_sanitizer=` (`Address`, `HWAddress`, `Undefined`, `Memory`,
`MemoryWithOrigins`, `Thread`, `DataFlow`, `Leaks`, ...) [libcxx-params] and CMake caches
`Generic-asan.cmake`, `Generic-ubsan.cmake`, `Generic-tsan.cmake`, `Generic-msan.cmake`
[libcxx-caches]. libc++ also annotates its containers for ASan (STATUS notes that 16 libc++ tests
fail under ASan only because libycxx has no such annotations; ASan's `detect_container_overflow`
flag honours them [asan-flags]).

### Library-mode configurations (hardening, exceptions, RTTI)

libc++ tests each mode a user can select: `--param hardening_mode=none|fast|extensive|debug`
[libcxx-params], CMake caches `Generic-hardening-mode-{fast,extensive,debug}.cmake`,
`Generic-no-exceptions.cmake`, `Generic-no-rtti.cmake`, `Generic-static.cmake`, `Generic-cxx26.cmake`
[libcxx-caches], and death tests for hardening assertions (`assert.*.pass.cpp`,
`TEST_LIBCPP_ASSERT_FAILURE`, features `can-test-hardening-assertions*`) [libcxx-testing]. The
hardening modes are `none`, `fast`, `extensive`, `debug`, chosen with `_LIBCPP_HARDENING_MODE` by
users or `LIBCXX_HARDENING_MODE` by vendors, with assertion semantics `ignore`, `observe`,
`quick-enforce`, `enforce` [libcxx-hardening]. libstdc++ runs its suite with extra flags per board
(`RUNTESTFLAGS=--target_board=unix/-O1/-D_GLIBCXX_ASSERTIONS`), has `make check-debug` for its debug
mode, and runs the whole suite in several `-std` modes (`GLIBCXX_TESTSUITE_STDS=11,17,23`)
[libstdcxx-test]. The MSVC STL runs each test in a matrix of compiler configurations
(`tests/libcxx/usual_matrix.lst`: a cross list of compiler options, one line with
`-fsanitize=address`, one with clang-cl) [msvc-matrix], and reports each result as
`{Result Code}: {Test Suite Name} :: {Test Name}:{Configuration Number}` [msvc-readme]. libstdc++'s
debug mode changes container sizes, so debug and non-debug code can be linked only if no container
instantiation passes between them [libstdcxx-debug]: a mode that changes layout needs its own
configuration, not a per-test flag.

libycxx has `YCXX_HARDENED=1` (run-time precondition checks through `ycxx::detail::precondition`),
`-fno-exceptions` and `-fno-rtti` support (DECISIONS §1, §4), but no suite configuration runs
them: no own test sets `YCXX_HARDENED` (the libstdc++ format adds it only when a libstdc++ test
asks for `_GLIBCXX_ASSERTIONS`), `-fno-exceptions` appears in two own tests, and there is no lit
parameter to add flags to a whole run. A precondition check that is never compiled in a test run is
untested code. Gaps, items 3 and 6.

### Reference runs against another library

`YCXX_STDLIB=libstdcxx tools/run-conformance ycxx gcc|clang` builds the own suite against the
toolchain's libstdc++ (`tools/ref-cxx`), and `tests/ycxx/REFERENCE.md` explains every failure (bugs
or missing C++26 parts in libstdc++, compiler differences, ABI limits); STATUS records that no
failure was traced to a defect in a test. Checked: `optional` against libstdc++ with GCC, 36
passed, 1 failed (`optional/nullopt_compare.compile.pass.cpp`, listed in REFERENCE.md).

This is the check that the tests test the standard: a spec-derived suite needs an independent
implementation to catch tests that encode a misreading. libc++ supports the same direction
(running its suite against libstdc++) [libcxx-stdlib-libstdcxx-cfg]. Keep the reference runs
manual or nightly; they produce triage work, not a gate.

### Reports

Every lit format records each command a test ran, with exit status, duration and output, for
passing tests too (`tests/ycxxlit/transcript.py`). `tools/run-conformance` writes
`build/test-logs/<suite>-<compiler>[-<sanitizer>][-ref].{log,html,md,tsv,json}` with the run's
provenance (commit, compiler version, command, host), and `tools/test` adds `run.html`/`run.md`,
the composite report. CI uploads them as artifacts.

Others: lit writes JSON (`-o`) and xUnit XML (`--xunit-xml-output`) and can print the slowest tests
(`--time-tests`) [lit]; libc++ consolidates benchmark results and compares them
(`consolidate-benchmarks`, `compare-benchmarks`) [libcxx-testing]; libstdc++'s DejaGnu run produces
`libstdc++.sum` and `libstdc++.log` [libstdcxx-test]. libycxx's reports are more complete (a
passing test shows evidence, not just a verdict); xUnit output would let CI systems display
per-test results natively.

### CI layout

| When | libycxx | Others |
|---|---|---|
| Every push / PR | `ci.yml`: `tools/test --baseline policy build freestanding cmake ycxx` on Linux (gcc:16 container, Clang 23 from apt.llvm.org) and macOS 15 arm64; a sample of both external suites on Linux | libc++: CI configurations defined in `libcxx/utils/ci/Dockerfile` and run by `libcxx/utils/ci/run-buildbot`, reproducible locally with `run-buildbot-container` [libcxx-testing] |
| Nightly / on demand | `full.yml`: both external suites, both compilers, Linux and macOS (one job per suite and compiler, up to 300-340 minutes), and the own suite with ASan+UBSan, each against its baseline | libc++: continuous fuzzing on OSS-Fuzz (`libcxx/utils/ci/oss-fuzz.sh`) [libcxx-oss-fuzz] |

lit can split a run into shards (`--num-shards M --run-shard N`, or `LIT_NUM_SHARDS`), "for
parallel execution on separate machines" [lit]; libycxx's nightly jobs do not shard (Gaps, item 5).

### Fuzzing

libc++ keeps fuzz targets as ordinary tests (`libcxx/test/libcxx/fuzzing/*.pass.cpp`); the OSS-Fuzz
script compiles each with `-DLIBCPP_OSS_FUZZ` and the fuzzing engine, using
`-nostdinc++ -cxx-isystem <install>/include/c++/v1` [libcxx-oss-fuzz]. libycxx has no fuzzing
(Gaps, item 7). Its parsers are natural targets: `<regex>` compilation, format strings, `from_chars`,
`<chrono>` parsing, the Itanium demangler, TZif reading.

## 7. Comparison by stage

| Stage | libycxx | libc++ | libstdc++ | MSVC STL |
|---|---|---|---|---|
| Build system | CMake; library built with itself | CMake `runtimes` build, CMake caches per configuration [libcxx-vendor, libcxx-caches] | GCC's configure/make, built with GCC [libstdcxx-configure] | CMake presets [msvc-readme] |
| Freestanding | core layer, one build; separate runtime archive (script-built) | feature switches [libcxx-vendor] | `--disable-hosted-libstdcxx` [libstdcxx-configure] | not confirmed |
| ABI runtime | own (Itanium), unwinder from the toolchain | selectable: libc++abi, libcxxrt, libsupc++, ... [libcxx-vendor] | libsupc++ | not confirmed |
| Artifacts | static archives, PIC | shared and static [libcxx-vendor] | shared, versioned [libstdcxx-abi]; static not confirmed | not confirmed |
| Symbol policy | everything hidden, per-image runtime | exported ABI with visibility macros, inline ABI namespace; hermetic static option [libcxx-visibility, libcxx-vendor] | version script, `check-abi` baseline [libstdcxx-abi] | not confirmed |
| Selection | `-nostdinc++ -isystem`, `-nostdlib++`; wrapper; CMake package; toolchain file | `-stdlib=libc++` or the same generic flags [libcxx-user, libcxx-vendor] | default for GCC | `INCLUDE`/`LIB` via `set_environment.bat` [msvc-readme] |
| Own tests | spec-only author; `.pass`/`.compile.pass`/`.compile.fail` | rich kinds incl. `.verify.cpp`, `.sh.cpp`, `.gen.cpp` [libcxx-testing] | DejaGnu `dg-*` with message matching [libstdcxx-test] | `tests/std`, `tests/tr1` [msvc-readme] |
| External suites | libc++'s and libstdc++'s, run only, fetched and pinned | can run against libstdc++ [libcxx-stdlib-libstdcxx-cfg] | not confirmed | libc++'s, from its llvm-project checkout [msvc-readme] |
| Known failures | per OS/compiler/sanitizer baselines (generated), xfail and skip lists with reasons | `XFAIL`/`UNSUPPORTED` in tests [libcxx-testing] | `xfail` selectors in tests [libstdcxx-test] | `expected_results.txt` by cause, per configuration [msvc-expected] |
| Configurations | 2 compilers x 2 OSes; ASan+UBSan (Clang); TSan manual | hardening modes, no-exceptions, no-rtti, sanitizers, std modes, modules [libcxx-caches, libcxx-params] | `-std` list, debug mode, board flags [libstdcxx-test] | matrix files [msvc-matrix] |
| Reference runs | own suite against libstdc++ | suite against libstdc++ [libcxx-stdlib-libstdcxx-cfg] | not confirmed | not confirmed |
| Reports | per-test transcripts, HTML/Markdown, provenance | lit output | `.sum`/`.log` [libstdcxx-test] | lit output [msvc-readme] |
| Fuzzing | none | OSS-Fuzz [libcxx-oss-fuzz] | not confirmed | not confirmed |

**What libycxx does better.** Hermetic by default (no opt-in, no consumer link flags, tested with a
second runtime in the process); one generated set of flags behind three delivery paths, with
compiler provisioning; a clean-room own suite written from the draft alone, validated by reference
runs; external suites never vendored, pinned and fetched; baselines generated from runs, per OS,
compiler and sanitizer, with XPASS failing the run; reports that show the evidence for passing
tests.

**What others do better.** Configuration coverage (hardening, no-exceptions, no-rtti, TSan, MSan as
standing configurations); diagnostic-matching tests; feature-based test constraints; sharding;
fuzzing; container annotations for ASan; documented raw flags for consumers; keeping the cause next
to each expected failure.

## 8. Gaps and proposals

Ranked by value for effort. None of these is implemented by this document.

1. **Complete the baselines CI already depends on.** `full.yml` runs libc++ on Linux with Clang and
   macOS with GCC, and `ci.yml` the own suite on macOS, all with `--baseline`, but
   `tests/libcxx/baseline/linux-clang.txt`, `darwin-gcc.txt` and `tests/ycxx/baseline/darwin-*.txt`
   do not exist, so every failure counts there, documented ones included. Record them from the next
   runs' `<run>.baseline.txt` artifacts. Cheap; without it those jobs cannot be green.
2. **A diagnostic-matching test kind.** Add `// EXPECT-ERROR: <regex>` (one or more) to
   `*.compile.fail.cpp`: the test passes only if every pattern matches an error line, on both
   compilers; optionally map libc++'s `expected-error` and libstdc++'s `dg-error "msg"` to it where
   the message is the standard's (a `static_assert` text the library controls). Today a
   compile-fail test passes on any error but a missing header. libc++'s `.verify.cpp` uses clang
   `-verify` [libcxx-testing], which GCC lacks, hence a regex of one's own.
3. **A hardened configuration.** Run the own suite with `-DYCXX_HARDENED=1` nightly, and add death
   tests for precondition violations (a `.pass.cpp` that forks or expects an abnormal exit, under a
   `hardened` feature), as libc++'s `assert.*.pass.cpp` [libcxx-testing]. Today the hardened code
   paths are compiled by no test run.
4. **Lit features instead of bespoke directives.** Set `available_features` in
   `tests/ycxx/lit.cfg.py` (`gcc`, `clang`, `linux`, `darwin`, `asan`, `ubsan`, `tsan`, `hardened`,
   `no-exceptions`, `no-rtti`) and let tests use lit's `REQUIRES:`/`UNSUPPORTED:`/`XFAIL:` with
   boolean expressions [lit]; keep `XFAIL-COMPILER` as an alias. Item 3 and 6 then need no new
   directive kinds.
5. **Shard the nightly suites.** Split each `full.yml` suite job with lit's
   `--num-shards`/`--run-shard` [lit] across matrix entries and merge the `.tsv` files before
   `baseline.py`; record test times (`--time-tests`) to order slow tests first. The jobs now take
   up to 300-340 minutes.
6. **A configuration parameter and a nightly matrix.** Add a lit parameter (and `tools/test`
   option) for extra compile flags, then run nightly: `-fno-exceptions`, `-fno-rtti`, hardened, `-O0`
   and `-O2`, and TSan with `build/clang-tsan`, each with its own baseline (the baseline file name
   already carries a configuration suffix). Models: libc++'s CMake caches [libcxx-caches],
   libstdc++'s board flags [libstdcxx-test], the MSVC STL's matrix files [msvc-matrix].
7. **Fuzz targets that are also tests.** Write libFuzzer entry points for `<regex>`, format strings,
   `from_chars`, `<chrono>` parsing, the demangler and TZif parsing as `.pass.cpp` files that replay
   a small corpus in the normal suite and build as fuzzers with `-fsanitize=fuzzer` under a
   parameter, as libc++ does for OSS-Fuzz [libcxx-oss-fuzz]. Run them for a fixed time nightly.
8. **The freestanding runtime archive as a CMake target.** Build `libycxx-freestanding.a` in
   `CMakeLists.txt` (`ycxx::freestanding`), install and export it, and let
   `tools/check_freestanding.sh` use it. Today only the script builds it, so a freestanding consumer
   has no supported way to get it.
9. **ASan container annotations** for `vector`, `deque` and `basic_string` (STATUS, Known
   limitations; 16 libc++ tests fail under ASan only for that reason), honoured by ASan's
   `detect_container_overflow` [asan-flags].
10. **One flag set for Clang everywhere.** Pass `--gcc-install-dir` (or record it in the package
    from the build) for Clang consumers of the CMake package, as `tools/ycxx-cxx` and the toolchain
    file do, or document that the package uses Clang's default GCC installation for crt files and
    libgcc.
11. **Document the raw flags for other build systems** (Meson, Bazel, plain Make): the table of
    section 4 and the exact link order, as libc++ documents its custom-installation command
    [libcxx-vendor]. Today README points to the wrapper and the package only.
12. **Keep the cause next to each baseline entry.** Allow `test  # <TRIAGE category>: <reason>` in
    baseline files (`baseline.py` would strip the comment), as `expected_results.txt` groups entries
    by cause [msvc-expected], so the list and `TRIAGE.md` cannot drift.
13. **xUnit output in CI** (`--xunit-xml-output` [lit]) so that GitHub's test summaries show
    per-test results next to the HTML reports.
14. **`-stdlib++-isystem` for Clang: not recommended.** Clang drops it under `-nostdinc++` (checked),
    GCC rejects it, and `-nostdinc++ -isystem` already places libycxx's directory first on both
    compilers. It would help only if libycxx had to coexist with a `-stdlib=` choice, which it does
    not.
15. **Symbol version scripts: not applicable** while libycxx ships no shared library and exports
    nothing; `tests/ycxx/linkage/no_exported_library_symbols` and `tests/cmake/run.sh` already check
    the export set. Revisit with a shared library (`check-abi`-style baseline [libstdcxx-abi]).

## Validation record

Run on 2026-10-05 in a worktree at `6f13096`, Linux x86_64, GCC 16.2.0 (`/opt/gcc-16`), Clang 23.1
(apt.llvm.org), lit 23.1.2 through `uvx`.

| What | Command | Result |
|---|---|---|
| Build, both compilers | `cmake -S . -B build/<cc> -G Ninja -DCMAKE_C_COMPILER=... -DCMAKE_CXX_COMPILER=...` and `ninja -C build/<cc>` | `libycxx.a`, `libycxx-abi.a` for each |
| Wrapper | `tools/ycxx-cxx gcc|clang -O2 examples/demo.cpp -o demo && ./demo` | "libycxx example: ok", exit 0, both compilers; NEEDED `libm`, `libgcc_s`, `libc`, loader; no exported `_Z*`/`__cxa_*`/`__gxx_*` |
| CMake package, add_subdirectory, toolchain file, coexistence | `tests/cmake/run.sh gcc clang` | all checks passed (install, find_package, add_subdirectory, no toolchain C++ library, no exports, libycxx with libstdc++ in one process, catcher, old-compiler rejection, toolchain file, missing-version message) |
| Own suite, one directory | `tools/test -j4 -f optional ycxx` | 37/37 passed, GCC and Clang |
| Own suite, sanitizers | `tools/test -j2 -c clang -s asan,ubsan -f optional ycxx` | 37/37 passed |
| Reference run | `YCXX_STDLIB=libstdcxx tools/run-conformance ycxx gcc optional` | 36 passed, 1 failed (`optional/nullopt_compare`, in REFERENCE.md) |
| External suite with baseline | `tools/test --baseline -c gcc -f numerics/bit libcxx`; `... -f numerics/numbers libcxx` | 16 passed + 1 unsupported, exit 0; 4 passed + 1 failed (in the baseline), "0 outside the baseline", exit 0 |
| Freestanding | `tools/check_freestanding.sh` | 73 headers compile and the smoke program links: clang x86_64, clang riscv64, gcc x86_64 |
| Driver flags | `-E -v`, `-###` with and without `-nostdinc++`, `-nostdlib++`, `-stdlib++-isystem`, `-stdlib=`, `-fvisibility-global-new-delete=` on `g++-16` and `clang++-23` | as in the table of section 4 |
| ASan allocation functions | `tools/ycxx-cxx clang -fsanitize=address ... -Wl,-Map,...` | `operator new` from `libclang_rt.asan_cxx-x86_64.a(asan_new_delete.cpp.o)`; GCC: `cannot find -lasan` |
| GCC `-Wattributes` | `g++-16 -nostdinc++ -isystem include -c` on `struct S { std::string s; };` | the warning; Clang: none |

## 9. Sources

All read on 2026-10-05. Only documentation, build configuration, CI scripts and test configuration.

- [libcxx-user] Using libc++: https://libcxx.llvm.org/UserDocumentation.html
- [libcxx-vendor] Building libc++ (vendor documentation): https://libcxx.llvm.org/VendorDocumentation.html
- [libcxx-testing] Testing libc++: https://libcxx.llvm.org/TestingLibcxx.html
- [libcxx-hardening] Hardening modes: https://libcxx.llvm.org/Hardening.html
- [libcxx-visibility] Symbol visibility macros: https://libcxx.llvm.org/DesignDocs/VisibilityMacros.html
- [libcxx-params] lit parameters (test infrastructure, read for parameter names only):
  https://github.com/llvm/llvm-project/blob/llvmorg-23.1.2/libcxx/utils/libcxx/test/params.py
- [libcxx-stdlib-libstdcxx-cfg] Test configuration for libstdc++:
  https://github.com/llvm/llvm-project/blob/llvmorg-23.1.2/libcxx/test/configs/stdlib-libstdc++.cfg.in
- [libcxx-caches] CMake caches (file names; `Generic-hardening-mode-fast.cmake` read):
  https://github.com/llvm/llvm-project/tree/llvmorg-23.1.2/libcxx/cmake/caches
- [libcxx-oss-fuzz] OSS-Fuzz build script:
  https://github.com/llvm/llvm-project/blob/main/libcxx/utils/ci/oss-fuzz.sh
- [D53787] Clang review "Provide -fvisibility-global-new-delete-hidden option": https://reviews.llvm.org/D53787
- [clang-ref] Clang command line reference: https://clang.llvm.org/docs/ClangCommandLineReference.html
- [lit] lit command guide: https://llvm.org/docs/CommandGuide/lit.html
- [libstdcxx-test] libstdc++ manual, Testing: https://gcc.gnu.org/onlinedocs/libstdc++/manual/test.html
- [libstdcxx-abi] libstdc++ manual, ABI Policy and Guidelines: https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html
- [libstdcxx-configure] libstdc++ manual, Configure: https://gcc.gnu.org/onlinedocs/libstdc++/manual/configure.html
- [libstdcxx-debug] libstdc++ manual, Using the debug mode: https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode_using.html
- [gcc-dialect] GCC, C++ Dialect Options: https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html
- [gcc-dirs] GCC, Directory Options: https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html
- [gcc-link] GCC, Link Options: https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html
- [msvc-readme] MSVC STL README: https://github.com/microsoft/STL/blob/main/README.md
- [msvc-expected] MSVC STL, libc++ expected results: https://github.com/microsoft/STL/blob/main/tests/libcxx/expected_results.txt
- [msvc-matrix] MSVC STL, libc++ test matrix (test configuration): https://github.com/microsoft/STL/blob/main/tests/libcxx/usual_matrix.lst
- [chromium-cxx-gni] Chromium `build/config/c++/c++.gni`:
  https://chromium.googlesource.com/chromium/src/+/main/build/config/c++/c++.gni
- [chromium-libcxx-gn] Chromium `buildtools/third_party/libc++/BUILD.gn`:
  https://chromium.googlesource.com/chromium/src/+/main/buildtools/third_party/libc++/BUILD.gn
- [pdfium-issue-176] pdfium-lib issue on Chromium's `std::__Cr` symbols: https://github.com/paulocoutinhox/pdfium-lib/issues/176
- [android-cpp] Android NDK, C++ library support: https://developer.android.com/ndk/guides/cpp-support
- [ld-options] GNU ld, Command-line Options: https://sourceware.org/binutils/docs/ld/Options.html
- [ld-version] GNU ld, VERSION command: https://sourceware.org/binutils/docs/ld/VERSION.html
- [dyld-relnotes] Apple, Dynamic Loader Release Notes: https://developer.apple.com/releasenotes/DeveloperTools/RN-dyld/index.html
- [apple-forum-693914] Apple Developer Forums, "Two-level namespace and coalesced typeinfos": https://developer.apple.com/forums/thread/693914
- [asan-algorithm] AddressSanitizer algorithm: https://github.com/google/sanitizers/wiki/AddressSanitizerAlgorithm
- [asan-flags] AddressSanitizer flags: https://github.com/google/sanitizers/wiki/AddressSanitizerFlags
- [cmake-toolchains] CMake, cmake-toolchains(7): https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html
- [eastl] EASTL README, CONTRIBUTING and FAQ: https://github.com/electronicarts/EASTL,
  https://github.com/electronicarts/EASTL/blob/master/CONTRIBUTING.md,
  https://github.com/electronicarts/EASTL/blob/master/doc/FAQ.md
- [etl] Embedded Template Library README: https://github.com/ETLCPP/etl
- [stdcxx-attic] Apache stdcxx in the Attic: https://attic.apache.org/projects/stdcxx.html

### Other libraries, for scope

These are not replacements for `std` and were consulted only to place libycxx's choices. EASTL
lives in its own namespace, builds its tests with `-DEASTL_BUILD_TESTS:BOOL=ON` and runs them with
`ctest` [eastl], and by default requires the program to define `operator new[]` overloads taking a
name, flags and debug flags [eastl]. The Embedded Template Library is "not intended as a full
replacement" for the standard library, never uses the heap, can work without it (`ETL_NO_STL`), and
runs its unit tests in GitHub Actions [etl]. Apache stdcxx, a full standard library, was retired in
July 2013 and moved to the Attic in 2014 [stdcxx-attic]. uSTL and Fuchsia's libc++ configuration
were not consulted; nothing is claimed about them.
