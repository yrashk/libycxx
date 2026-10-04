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

```sh
tools/check-all                       # policy checks, both library builds, freestanding check
tests/cmake/run.sh                    # CMake package and toolchain file (YCXX_TEST_PROVISION=1:
                                      #   also download Clang through the toolchain file)
tools/run-conformance ycxx clang      # libycxx's own spec-derived suite (tests/ycxx)
tools/run-conformance libcxx gcc <dirs>     # libc++'s tests (run only)
tools/run-conformance libstdcxx gcc <dirs>  # libstdc++'s testsuite (run only)
```
