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
(elapsed time and the latest output line for builds; tests done, pass/fail counts, ETA and the
current test for suites). Failures are shown as they happen, with the commands that ran and
their output. A summary closes the run. Colour is on for terminals and GitHub Actions; set
`NO_COLOR=1` or `YCXX_COLOR=never` to turn it off. Full logs are in `build/test-logs/`. Works
on Linux and macOS. lit runs through [uv](https://docs.astral.sh/uv/)'s `uvx`, pinned to the
LLVM release of the libc++ tests, so nothing needs installing besides uv (`YCXX_LIT=lit` uses a
lit already installed).

```sh
tools/test                            # policy checks, library builds, freestanding, own suite (both compilers)
tools/test all                        # also the CMake package test and the libc++/libstdc++ suites
tools/test -c clang -f format ycxx    # one compiler, one directory of the own suite
tools/test --help                     # stages and options (-j, -s asan, --fail-fast, -v)
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

`tools/run-conformance` options go to lit after `--`. `YCXX_VERBOSE=1` lists every test,
`YCXX_FAIL_DETAILS=N` shows the output of the first N failures (default 10), and `YCXX_RAW=1`
prints lit's own output.
