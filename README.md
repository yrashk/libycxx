# libycxx

A clean-room C++26 standard library for the newest GCC (16.2) and Clang (23.1), with its own
Itanium C++ ABI runtime. It is layered: a freestanding core, a hosted layer, and a small
platform abstraction layer (PAL). See `STATUS.md` for what is implemented and `DECISIONS.md`
for the design rules.

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
`g++-16` / `clang++-23`.

### From CMake alone

`cmake/ycxx-toolchain.cmake` does the same from CMake, sharing the same cache (it reads and
updates the same `toolchains.env` and installations):

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<libycxx>/cmake/ycxx-toolchain.cmake -DYCXX_COMPILER=clang
```

It only downloads or builds a compiler when asked with `-DYCXX_PROVISION=ON`; otherwise a
missing or unsupported compiler stops the configuration with instructions.
`-DYCXX_GCC_VERSION=` / `-DYCXX_LLVM_VERSION=` select other versions.

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

Without CMake, `tools/ycxx-cxx gcc|clang <args>` compiles and links against the libycxx built in
`build/<compiler>`.

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
```

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

### Known failures and CI

Tests that libycxx does not pass yet are recorded per platform and compiler in
`tests/<suite>/baseline/<os>-<compiler>[-<sanitizer>].txt` (STATUS.md and the suites' TRIAGE.md
say why). `tools/test --baseline` (`YCXX_BASELINE=1`) fails a suite only on a test that did not
pass and is not listed, and names the listed tests that now pass. Without a baseline file every
failure counts. Each run writes its own list as `build/test-logs/<run>.baseline.txt`; copy it over
the baseline file to record or update one. A compiler gap that the test cannot avoid is an
expected failure in the test itself (`// XFAIL-COMPILER:`; `tests/<suite>/xfail.txt` for the
external suites) instead.

CI (`.github/workflows/ci.yml`), on every push, runs `tools/test --baseline policy build
freestanding cmake ycxx` on Linux (the `gcc:16` container, Clang 23 from apt.llvm.org) and
macOS (Apple Silicon, Homebrew's GCC 16, the provisioned Clang 23), plus a sample of the external
suites on Linux. `.github/workflows/full.yml`, nightly and on demand, runs libc++'s and
libstdc++'s whole suites on both compilers on both platforms, and the own suite under
ASan+UBSan, each against its baseline. Every job uploads its reports as an artifact.
