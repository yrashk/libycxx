# libycxx

A clean-room C++26 standard library for the newest GCC (16.2) and Clang (23.1), with its own
Itanium C++ ABI runtime. It is layered: a freestanding core, a hosted layer, and a small
platform abstraction layer (PAL). See `STATUS.md` for what is implemented and `DECISIONS.md`
for the design rules.

## Quick start

From an empty directory to `import std;` with libycxx, with only git, CMake (3.28 or later),
Ninja, curl and the host's C toolchain installed (the Xcode Command Line Tools on macOS,
`build-essential` on Linux):

```sh
mkdir hello && cd hello
curl -fsSL --remote-name-all https://raw.githubusercontent.com/yrashk/libycxx/main/examples/quickstart/{CMakeLists.txt,hello.cpp}
cmake -B build -G Ninja
cmake --build build
./build/hello
```

The project (`examples/quickstart`) fetches libycxx with FetchContent and uses its toolchain file,
which finds Clang 23 or downloads it into `~/.local/share/ycxx/toolchains` (`-DYCXX_COMPILER=gcc`
for GCC 16). **`examples/quickstart/README.md`** explains each step, the first run's time and the
options. `tests/quickstart/run.sh` runs these commands as written on every CI run (`cmake` stage)
and, nightly, with Clang downloaded into an empty cache on Linux and macOS.

## Toolchains

Linux and macOS. `tools/toolchain/provision` finds GCC 16.2 and Clang 23.1 (with lld) or
installs them under `~/.local/share/ycxx/toolchains` (or `$YCXX_TOOLCHAINS`): Clang from the
LLVM release tarball, GCC built from source (on macOS against the SDK from `xcrun`). It also
picks up compilers you already have, including Homebrew's `gcc` and `llvm` kegs; `--use-brew`
installs those with Homebrew instead. Then load them into your shell:

```sh
source tools/toolchain/activate.sh            # bash, zsh
source tools/toolchain/activate.fish          # fish
source tools/toolchain/activate.sh --provision  # download/build whatever is missing first
ycxx-unload                                   # restore the previous environment
```

Activation exports `YCXX_GCC`, `YCXX_GXX`, `YCXX_CLANG`, `YCXX_CLANGXX`, `YCXX_LLD`,
`YCXX_LLVM_AR`, `YCXX_GCC_INSTALL_DIR` and `YCXX_ROOT` (on macOS also `SDKROOT`), and puts the
compilers and `tools/` on `PATH`. The repository's tools use these variables, falling back to
`g++-16` / `clang++-23`. Activation sets no `CXX`: the compilers on `PATH` use their own C++
library. `--use gcc|clang|<prefix>` also points `CXX`, `CC`, `CMAKE_TOOLCHAIN_FILE` and
`PKG_CONFIG_PATH` at a libycxx build or installation (below, "Building existing projects").

### From CMake alone

`cmake/ycxx-toolchain.cmake` does the same from CMake, sharing the same cache (it reads and
updates the same `toolchains.env` and installations):

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<libycxx>/cmake/ycxx-toolchain.cmake -DYCXX_COMPILER=clang
```

It only downloads or builds a compiler when asked with `-DYCXX_PROVISION=ON`; otherwise a
missing or unsupported compiler stops the configuration with instructions.
`-DYCXX_GCC_VERSION=` / `-DYCXX_LLVM_VERSION=` select other versions.

This file selects the compilers only: a project configured with it alone still uses the
toolchain's C++ library, unless it links `ycxx::ycxx` (below).

## Building existing projects against libycxx

**`docs/BUILDING_PROJECTS.md`** is the guide: which projects qualify, CMake, Meson, make,
autotools, Bazel, Conan and vcpkg, and how to check the result. In short, an installation
(`cmake --install`) has what a project that knows nothing about libycxx needs:

```sh
cmake -S proj -B build -DCMAKE_TOOLCHAIN_FILE=<prefix>/lib/cmake/libycxx/toolchain.cmake   # CMake
make CXX=<prefix>/bin/ycxx-c++ CC=<prefix>/bin/ycxx-cc                                    # make, autotools
meson setup build --native-file <prefix>/share/libycxx/meson-native.ini                   # Meson
pkg-config --cflags --libs libycxx                                                         # the plain compiler's flags
source tools/toolchain/activate.sh --use <prefix>       # exports CXX, CC, CMAKE_TOOLCHAIN_FILE, PKG_CONFIG_PATH
<prefix>/bin/ycxx-check-binary build                    # built against libycxx and nothing else?
```

`ycxx-c++` is the compiler libycxx was built with plus libycxx's headers, C++26 (an older `-std=`
is raised) and libycxx's archives; the build tree has the same files (`build/<cc>/bin/ycxx-c++`,
`build/<cc>/toolchain.cmake`, ...). `tests/integration/run.sh` builds sample projects each of
these ways.

## Using libycxx from CMake

```cmake
find_package(libycxx CONFIG REQUIRED)   # or: add_subdirectory(libycxx) / FetchContent
target_link_libraries(app PRIVATE ycxx::ycxx)
```

`ycxx::ycxx` compiles against libycxx's headers instead of the toolchain's (`-nostdinc++`,
C++26) and links libycxx and its ABI runtime instead of libstdc++/libc++ (`-nostdlib++`). Link
it into every C++ target of a program. The project must use GCC >= 16.2 or Clang >= 23.

Build and install:

```sh
cmake -S . -B build/gcc -G Ninja -DCMAKE_C_COMPILER=$YCXX_GCC -DCMAKE_CXX_COMPILER=$YCXX_GXX
cmake --build build/gcc
cmake --install build/gcc --prefix /opt/libycxx
```

Working examples: `examples/find_package` and `examples/add_subdirectory` (both build
`examples/demo.cpp`).

With Clang on Linux, `ycxx::ycxx` passes `--gcc-install-dir=<GCC 16's installation>` (for the C
runtime startup files and libgcc, as `tools/ycxx-cxx` and the toolchain file do); Clang would
otherwise pick the newest GCC it finds in the system's standard places. The directory is found
when libycxx is configured (`YCXX_GCC_INSTALL_DIR`, a cache variable: by default
`$YCXX_GCC_INSTALL_DIR`, the GCC building libycxx, `$YCXX_GXX` or `g++-16`), recorded in the
installed package, and can be overridden by a consumer's `YCXX_GCC_INSTALL_DIR`.

`-DYCXX_FREESTANDING_RUNTIME=ON` also builds and installs `libycxx-freestanding.a`, the runtime of
freestanding programs, as `ycxx::freestanding` (libycxx's headers and `-ffreestanding`; link it
instead of `ycxx::ycxx`): the default allocation functions of a heap-less program (replace them
to have a heap), `std::nothrow`, floating-point `<charconv>`, the `<atomic>` lock and wait tables,
`<debugging>` and the default contract-violation handler. It is built for the compiler's target
with `-ffreestanding -nostdinc -fno-exceptions -fno-rtti` (and the compiler's own header
directory, for `<stddef.h>`); the program provides `memcpy`,
`memmove`, `memset`, `memcmp` and its entry point (`tests/freestanding/rt.c` is an example).
`tools/check_freestanding.sh` builds the same archive for bare-metal targets.

### Hosted layers: hosted without an operating system

The hosted library is split into layers of support: `abort`, `memory`, `console`, `clock`,
`threads`, `random`, `files`, `environment`, `debug`, and the C library itself (`clib`). Each
layer is a few C primitives of `include/ycxx/pal.h`. `-DYCXX_PAL=none
"-DYCXX_HOSTED_LAYERS=memory;console;clock"` builds libycxx without its POSIX platform layer, and
your own providers supply the selected layers:

```cmake
set(YCXX_PAL none)
set(YCXX_HOSTED_LAYERS memory console clock)
add_subdirectory(libycxx)
ycxx_add_hosted_layer(memory PROVIDER my_heap)    # or SOURCES heap.c, or -DYCXX_PAL_MEMORY_PROVIDER=...
```

A feature whose layer is absent fails to compile or link, naming the layer or its primitive.
`examples/hosted-layers` has three working examples:

- containers, `std::print` and exceptions on a host OS with the program's own heap and console;
- the same on bare x86_64, booted by Limine in QEMU;
- the file streams over a RAM disk of the program's own.

The design is DECISIONS §18.

Without CMake, `tools/ycxx-cxx gcc|clang <args>` compiles and links against the libycxx built in
`build/<compiler>`.

### Transitive includes: `YCXX_NO_TRANSITIVE_INCLUDES`

Which other headers a standard header includes is unspecified ([res.on.headers]/1), yet much code
uses `std::min` after including only `<string>`, or `errno` after `<mutex>`, because libstdc++ and
libc++ happen to provide them. By default libycxx's headers provide the same: each public header
also includes the headers that both libraries provide with it (measured by compiling against
them, `tools/probe_transitive.py`), plus a few that real-world code is known to rely on; the list
is `tools/data/transitive-includes.txt`. Define `YCXX_NO_TRANSITIVE_INCLUDES` (on the command line:
`-DYCXX_NO_TRANSITIVE_INCLUDES`, any value) and each header includes only what the draft and the
implementation need: faster to compile, and a check that a program includes what it uses. The
design is DECISIONS §19.

`docs/CUSTOM_STDLIB.md`: building, using and testing a custom standard library, compared with libc++, libstdc++ and the MSVC STL.

## Modules: `import std;` and `import std.compat;`

libycxx provides the standard library modules ([std.modules]) for Clang 23 and GCC 16
(`-fmodules`): `std` exports every declaration in namespace `std` of the importable library
headers and the C++ headers for C library facilities, and the global `operator new`/`delete`;
`std.compat` also exports the C library's names in the global namespace (`::printf`, `::size_t`,
`<stdbit.h>`, `<stdckdint.h>`). No macro is exported (`assert`, `errno`, `EOF`, `INT_MAX`,
`__cpp_lib_*`: #include `<cassert>`, `<version>`, ... for those). The interface units are
`modules/std.cppm` and `modules/std.compat.cppm`, generated from the headers by
`tools/gen_std_module.py` (DECISIONS §16). A module's compiled interface is only valid with the
compiler options it was built with, so libycxx ships the sources and each project builds them.

From CMake (>= 3.28, a Ninja generator; Clang needs `clang-scan-deps`, which LLVM installs next
to `clang++`):

```cmake
find_package(libycxx CONFIG REQUIRED)        # or add_subdirectory / FetchContent
add_executable(app main.cpp)                  # main.cpp: import std;
target_link_libraries(app PRIVATE ycxx::modules)   # brings ycxx::ycxx
```

CMake compiles the interfaces with the project's flags and links `libycxx-modules.a` (the
modules' initializers). `ycxx::modules` exists when libycxx was configured with a Ninja generator
(`-DYCXX_MODULES=ON|OFF` decides explicitly). `examples/modules` is a complete project. CMake's
own `CMAKE_CXX_MODULE_STD` (CMake >= 3.30) is not supported: it would build the toolchain's C++
library's module, not libycxx's.

Without CMake, build the modules once per compiler and set of flags, then compile and link with
`--std-modules`:

```sh
tools/ycxx-modules clang                          # -> build/clang/modules (BMIs, libycxx-modules.a)
tools/ycxx-cxx clang --std-modules hello.cpp -o hello
tools/ycxx-modules gcc -o build/gcc/modules-O2 -O2     # other flags: another directory
tools/ycxx-cxx gcc --std-modules=build/gcc/modules-O2 -O2 hello.cpp -o hello
```

With a compiler directly: Clang needs `-fmodule-file=std=<dir>/std.pcm` (and
`-fmodule-file=std.compat=<dir>/std.compat.pcm`), GCC `-fmodules -fmodule-mapper=<dir>/module.map`
(the file `tools/ycxx-modules` writes), plus `<dir>/libycxx-modules.a` when linking, on top of
the flags `tools/ycxx-cxx` passes for any libycxx program (`-std=c++26 -nostdinc++ -isystem
include`, `-nostdlib++` and the archives).

Limitations: GCC 16 cannot mix `#include` of a standard header and `import std;` in one
translation unit (its own module bugs; STATUS.md, known compiler gaps); different translation
units of one program may freely use either. Clang 23 handles both orders. Only Linux is tested.

## Tests

One driver runs everything. It prints each command before running it, then live progress
(elapsed time and the latest output line for builds; pass/fail counts and ETA for suites).
Every test is listed as it finishes, passing ones too, with the steps it ran and what they
returned (`compile exit 0 0.21s · run exit 0 0.00s`; for a test that must not compile, the
compiler's first error). Failures show their whole transcript: the exact commands, their exit
statuses and their output. A summary closes the run. Each suite run also writes
`build/test-logs/<suite>-<compiler>.html`, a report of every test with its transcript and how
the run was made (commit, compiler version, command), and a `.tsv` with one line per test.
`tools/test` adds `build/test-logs/run.html`, the composite report of the whole run, made to
be shared. It covers every stage with its command; every failure in full (a failed stage's log
tail, such as a build error, and a failed test's transcript); and every suite run: own,
libc++ and libstdc++, per compiler. For each suite it gives the counts, why tests were
unsupported, an example of how a test runs, and every test with its steps. Its "Copy for an
agent" button copies all of that except the list of every test. The reports have copy
buttons (one failure, all failures, the whole report) that put self-contained Markdown on the
clipboard, ready to paste to a person or an AI assistant. Each report also embeds the whole of
itself as Markdown and writes it next to itself as `.md`, so it can be read without a browser. Colour is on for terminals and GitHub Actions; set
`NO_COLOR=1` or `YCXX_COLOR=never` to turn it off. Full logs are in `build/test-logs/`. Works
on Linux and macOS. lit runs through [uv](https://docs.astral.sh/uv/)'s `uvx`, pinned to the
LLVM release of the libc++ tests, so nothing needs installing besides uv (`YCXX_LIT=lit` uses a
lit already installed).

```sh
tools/test                            # policy checks, library builds, freestanding, own suite (both compilers)
tools/test all                        # also the CMake package test and the libc++/libstdc++ suites
                                      #   (fetched on first use: tools/fetch-suites)
tools/test -c clang -f format ycxx    # one compiler, one directory of the own suite
tools/test --help                     # stages and options (-j, -s asan, --fail-fast, -v, -q)
tools/check-all                       # the fast gate: tools/test --fail-fast policy build freestanding
tools/test realworld                  # real-world projects with their own tests (not a default stage)
```

### Real-world projects

`tools/realworld [-c gcc|clang] [-s asan|tsan] [project...]` (the `realworld` stage of
`tools/test`) builds open-source C++ projects against libycxx and runs their own test suites:
GoogleTest, Catch2, doctest, nlohmann/json, {fmt}, spdlog (with `std::format`), CLI11,
magic_enum, glaze, simdjson, Taskflow, EnTT, Google Benchmark, Microsoft GSL, oneTBB, range-v3,
libcoro, yaml-cpp and Abseil, each pinned to a release (`tests/realworld/<name>/manifest`). It fetches
them (into `build/realworld/src`), builds them with a C++ compiler that is `tools/ycxx-cxx`,
runs their CTest suites, and proves for every project that it was built against libycxx and
nothing else: every translation unit's recorded command, every object's headers, every image's
needed libraries and symbols, and libycxx's allocation table in every image (a self-test checks
that a build with the toolchain's own library is rejected). Patches of the projects' own
non-standard code and skipped tests carry their category and reason. Reports:
`build/test-logs/realworld-<cc>.html`; method: `docs/CUSTOM_STDLIB.md` ("Real-world projects");
results: STATUS.md.

The stages can also be run directly:

```sh
tests/cmake/run.sh                    # CMake package and toolchain file (YCXX_TEST_PROVISION=1:
                                      #   also download Clang through the toolchain file)
tools/run-conformance ycxx clang      # libycxx's own spec-derived suite (tests/ycxx)
tools/run-conformance libcxx gcc <dirs>     # libc++'s tests (run only)
tools/run-conformance libstdcxx gcc <dirs>  # libstdc++'s testsuite (run only)
```

The libc++ and libstdc++ suites are run only, never copied into this repository.
`tools/fetch-suites` downloads the pinned versions into `~/.local/share/ycxx/suites`
(`$YCXX_SUITES`). It takes `libcxx/test/std` and `libcxx/test/support` from LLVM 23.1.2 (a
sparse, shallow clone), and `libstdc++-v3/testsuite` from the GCC 16.2.0 release tarball,
unpacking nothing else. `tools/test` runs it on demand when a suite stage finds its suite
missing; `--no-fetch` turns that off. `LIBCXX_TESTS` and `LIBSTDCXX_TESTS` point at other
copies.

`tools/run-conformance` options go to lit after `--`. `YCXX_QUIET=1` (`tools/test -q`) lists
only the tests that did not pass, `YCXX_VERBOSE=1` (`-v`) prints every test's transcript,
`YCXX_FAIL_DETAILS=N` shows the transcripts of the first N failures (default 10), and
`YCXX_RAW=1` prints lit's own output. CI keeps the reports as the `test-reports` artifact.

### Own tests

A test in `tests/ycxx` is `*.pass.cpp` (compiled, linked and run; passes on exit status 0),
`*.compile.pass.cpp` (must compile) or `*.compile.fail.cpp` (must not compile, for a reason other
than a missing header). Directives in `//` comments adjust a test; `tests/ycxxlit/ycxx_format.py`
documents them all. A `*.compile.fail.cpp` should say why it must fail: with
`// EXPECT-ERROR: <regex>` (repeatable; `EXPECT-ERROR-GCC:` / `EXPECT-ERROR-CLANG:` for one
compiler's wording) it passes only if the compiler's diagnostics match every regex, and its
transcript names each regex that did not match, so a test cannot pass on an unrelated error:

```cpp
// EXPECT-ERROR: static assertion failed.*std::expected::value: E must be copy constructible
// EXPECT-ERROR-GCC: use of deleted function .*basic_string\(nullptr_t\)
// EXPECT-ERROR-CLANG: call to deleted constructor of 'std::string'
```

`// REQUIRES: <features>` runs a test only when a boolean expression of lit features holds (else
it is UNSUPPORTED): `gcc`, `clang`, `linux`, `darwin`, `asan`, `ubsan`, `tsan`, `hardened`,
`exceptions`, `rtti`. Tests that throw or catch say `// REQUIRES: exceptions`.
`// MODULES: std` (or `std.compat`) compiles a test that imports the standard library modules
(`tests/ycxx/modules`): they are built for the compiler and the test's flags (cached under the
run's build directory) and passed with `--std-modules`; UNSUPPORTED with the compiler's reason
when it cannot build a module at all. libc++'s `MODULE_DEPENDENCIES:` works the same way.
`// EXPECT-TERMINATE[: <regex>]` makes a `*.pass.cpp` a death test: the program must be killed by
SIGABRT, SIGTRAP or SIGILL. `tests/ycxx/precondition` holds such tests, one per hardened
precondition ([structure.specifications]/3.5: `vector::operator[]` out of range, `front()` of an
empty container, `*` of a disengaged `optional`, `span` and `mdspan` indexing,
`string_view::remove_prefix` beyond the size, ...), which run only in the hardened configuration.

The own suite also runs in other configurations, each with its own logs and reports (the run
name gets the suffix):

```sh
tools/test --hardened -c gcc ycxx                     # -DYCXX_HARDENED=1: ycxx-gcc-hardened
tools/test --cxxflags=-fno-exceptions --config-name=noexcept ycxx   # ycxx-<cc>-noexcept
tools/test --cxxflags=-DYCXX_NO_TRANSITIVE_INCLUDES --config-name=strict-includes ycxx   # no transitive includes
YCXX_HARDENED=1 tools/run-conformance ycxx clang precondition       # the same, directly
YCXX_CXXFLAGS=-O2 YCXX_CONFIG_NAME=O2 tools/run-conformance ycxx gcc
```

`--cxxflags` (`YCXX_CXXFLAGS`) appends flags to every test's; without `--config-name`
(`YCXX_CONFIG_NAME`) the name is made from the flags. `-fno-exceptions` and `-fno-rtti` remove the
`exceptions` and `rtti` features.

### Failures and CI

A test reported FAIL fails the run, and CI: there are no lists of known failures. A test fails
for one of three reasons, each handled where it is decided:

- a libycxx bug: fixed (STATUS.md lists what is open);
- a test that does not apply to libycxx (an external test of another library's internals,
  extensions or older rules): skipped with its category and reason (`tests/<suite>/skip.txt`;
  reported UNSUPPORTED, with the libycxx test that covers its subject, if any);
- a cause outside the test and the library (a compiler bug, an ABI limit, a draft defect, a
  feature not implemented yet): an expected failure with its reason, in the own test
  (`// XFAIL: gcc|clang|any <reason>`) or the external suite's `tests/<suite>/xfail.txt`.
  Reported XFAIL with the reason; a pass is XPASS, which fails the run, so the mark goes
  when the cause does.

A libc++ test that cannot apply in one configuration only, such as a permission-error test when
the run is as root (CI's Linux containers), is listed in `tests/libcxx/unsupported.txt` with the
lit feature naming that configuration (`root`), and reported UNSUPPORTED only there. A libstdc++
test that relies on one compiler's implementation-defined behaviour or extensions (GCC's
`source_location` columns or predefined macros, an optional `std::float32_t`) is listed the same
way in `tests/libstdcxx/unsupported.txt`, with that compiler's name.

Sanitizer runs (`tools/test -s asan,ubsan`, `-s tsan`; `SANITIZER=` for `tools/run-conformance`)
compile the tests with the sanitizers and link them with a libycxx built with the same ones,
`build/<cc>-<sanitizers>` (`build/clang-tsan`), which `tools/run-conformance` configures and
brings up to date itself (CMake option `YCXX_SANITIZE`; DECISIONS §6.8). A sanitizer sees only
instrumented code: with an uninstrumented library ThreadSanitizer reports every hand-off through
libycxx's own mutexes, queues and reference counts. All three suites run under sanitizers. A
ThreadSanitizer report is a race to fix, in the library or the test (an external suite's test
with a race is skipped with its reason), or a false positive suppressed in the suite's
`tests/<suite>/tsan.supp` with its reason (each program gets it in `TSAN_OPTIONS`, shown in its
transcript). GCC needs its sanitizer runtimes (`libasan`, `libtsan`), which some GCC builds lack:
`tools/toolchain/provision --with-sanitizers` builds GCC 16.2 with them.

CI (`.github/workflows/ci.yml`), on every push, runs `tools/test policy build freestanding cmake
ycxx` on Linux (the `gcc:16` container, Clang 23 from apt.llvm.org) and macOS (Apple Silicon,
Homebrew's GCC 16, the provisioned Clang 23), plus a sample of the external suites on Linux.
`.github/workflows/full.yml`, nightly and on demand, runs libc++'s and libstdc++'s whole suites on
both compilers on both platforms, the own suite under ASan+UBSan (Clang), all three suites under
ThreadSanitizer on both compilers (libycxx instrumented too; a job of its own on the bare runner,
with GCC 16.2 built with libsanitizer by `tools/toolchain/provision` and cached), and the own
suite on both compilers hardened, with `-fno-exceptions`, with `-O2` and without transitive
includes (`-DYCXX_NO_TRANSITIVE_INCLUDES`), and the benchmarks of `bench/` against their stored
baseline of ratios to libstdc++ (`bench/check`: a FAIL is a regression that repeated in two
confirmation runs; DECISIONS §15). Every job uploads its reports as an
artifact.
Tests that need a named locale (libstdc++'s `dg-require-namedlocale`, libc++'s `locale.<name>`
features) run when the C library has it (`tests/ycxxlit/locales.py`); `tools/ci/gen-locales`
generates every locale the suites name (the nightly Linux jobs do), and `YCXX_LONG_TESTS=1` runs
libc++'s long tests.
