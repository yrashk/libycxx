# Quickstart: `import std;` with libycxx

From an empty directory to a program that does `import std;` with libycxx, on Linux or macOS.
You need git, CMake 3.28 or later, Ninja, curl, and the host's C toolchain: the Xcode Command
Line Tools on macOS, `build-essential` (or your distribution's equivalent) on Linux. Nothing else
has to be installed first: the compiler is downloaded if you do not have it.

```sh
mkdir hello && cd hello
curl -fsSL --remote-name-all https://raw.githubusercontent.com/yrashk/libycxx/main/examples/quickstart/{CMakeLists.txt,hello.cpp}
cmake -B build -G Ninja
cmake --build build
./build/hello
```

It prints `["import", "modules", "std"]`. The two files are [`CMakeLists.txt`](CMakeLists.txt)
and [`hello.cpp`](hello.cpp) in this directory.

## What the commands do

1. `curl` downloads the two files of this directory.
2. `cmake -B build -G Ninja` configures the project. Before `project()`, `CMakeLists.txt` fetches
   libycxx with FetchContent (a shallow git clone of `main`) and selects libycxx's toolchain file,
   `cmake/ycxx-toolchain.cmake`. That file looks for Clang 23.1: in its toolchain cache
   (`~/.local/share/ycxx/toolchains`, or `$YCXX_TOOLCHAINS`), then `clang++-23`, `clang++` and
   Homebrew's `llvm` keg. When there is none, it downloads LLVM's 23.1.2 release (a 2 GB download;
   Linux x86_64 and arm64, macOS on Apple silicon) into the cache, where later projects and
   `tools/toolchain/provision` find it.
3. `cmake --build build` builds what `hello` needs from libycxx (`libycxx.a`, its ABI runtime and
   the `std` and `std.compat` modules, compiled with the project's flags), then `hello`.
4. `./build/hello` runs it. It links no libstdc++ or libc++.

The first run took 3 minutes on a Linux x86_64 test machine with a fast connection: 15 seconds
to download Clang, 2.5 minutes to extract it (0.8 GB of the release is kept), 20 seconds to build
with 2 jobs. Later runs, and other projects, reuse the downloaded Clang.

## GCC instead of Clang

```sh
cmake -B build -G Ninja -DYCXX_COMPILER=gcc
```

uses GCC 16.2 (`g++-16`, `g++` of that version, Homebrew's `gcc`). Without one, it builds GCC
from source into the cache, once, which takes about an hour on 4 cores (as
`tools/toolchain/provision --gcc-only` does); on macOS, `brew install gcc` first is quicker.

## Options

- `-DYCXX_PROVISION=OFF`: never download or build a compiler; stop with a message instead.
- `YCXX_TOOLCHAINS=<dir>` (environment): another toolchain cache.
- `-DFETCHCONTENT_SOURCE_DIR_LIBYCXX=<checkout>`: use a libycxx checkout instead of cloning it.
- `GIT_TAG`: `main` until libycxx has a release; pin a commit for reproducible builds.

Other ways to use libycxx (an installation with `find_package`, a project that knows nothing
about libycxx, builds without CMake) are in the [README](../../README.md) and
[docs/BUILDING_PROJECTS.md](../../docs/BUILDING_PROJECTS.md).

## Tested

`tests/quickstart/run.sh` runs the commands above literally, from an empty directory, against
the checkout it belongs to, and checks the output: with the compilers already installed in the
`cmake` stage of `tools/test` (both compilers, every CI run), and with Clang downloaded into an
empty cache (`--provision`) in the nightly workflow, on Linux and macOS.
