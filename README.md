<picture>
  <source media="(prefers-color-scheme: dark)" srcset="site/assets/logo-dark.svg">
  <img src="site/assets/logo.svg" alt="libycxx" width="266" height="80">
</picture>

# An independent C++26 standard library

libycxx implements the C++26 standard library from the working draft, with its own
Itanium C++ ABI runtime. Use it with GCC 16.2 or Clang 23.1 on Linux and macOS, or use
its freestanding core on bare metal.

It replaces the compiler's usual C++ library when you explicitly select it for a project.
You can use standard headers or `import std;`.

**Early development (0.1.0):** the ABI is not stable. Rebuild your program and all C++
dependencies when libycxx changes. Compiler gaps and remaining conformance issues are
recorded in [STATUS.md](STATUS.md).

[Website](https://libycxx.org/) · [API reference](https://libycxx.org/api/) ·
[Implementation status](STATUS.md) · [Design decisions](DECISIONS.md)

## Quick start

Build a small program that uses `import std;`. On Linux or macOS, you need:

- git, curl, CMake **3.28+**, and Ninja;
- the host's C toolchain: Xcode Command Line Tools on macOS, or `build-essential`
  (or your distribution's equivalent) on Linux.

Run these commands from a directory where you want to create the example:

```sh
mkdir hello && cd hello
curl -fsSL --remote-name-all https://raw.githubusercontent.com/yrashk/libycxx/main/examples/quickstart/{CMakeLists.txt,hello.cpp}
cmake -B build -G Ninja
cmake --build build
./build/hello
```

The output is `["import", "modules", "std"]`.

The example fetches libycxx, selects Clang 23, and builds the library and its modules.
If Clang is missing, it downloads it into `~/.local/share/ycxx/toolchains`; the download
is about 2 GB, and later builds reuse it. The example follows `main` until a release
is available; pin a commit for reproducible builds.

To use GCC instead, add `-DYCXX_COMPILER=gcc` to the configure command. Missing GCC is
built from source, which can take about an hour; using an installed GCC 16.2 is quicker.

The [quick-start guide](examples/quickstart/README.md) explains the files, compiler
selection, cache options, and first-build timing. These commands are tested as written in CI.

## What you get

| Part | Purpose |
| --- | --- |
| C++26 library | Every standard header, including containers, ranges, formatting, concurrency, and the newer C++26 facilities. Compiler-dependent features have [known gaps](STATUS.md). |
| Freestanding core | Library facilities for programs without an operating system or C library. An optional runtime supplies allocation, floating-point character conversion, and atomic support. |
| Hosted library | Files, streams, threads, clocks, and locales, connected to the system through a small C platform abstraction layer (PAL). You can supply individual hosted layers yourself. |
| ABI runtime | libycxx's own exceptions, RTTI, `dynamic_cast`, and static-local guards; stack unwinding comes from the toolchain. |
| Standard modules | `import std;` and `import std.compat;`, compiled with your project's flags. |
| Static or shared | Static archives by default; a shared library, `libycxx.so.0.<minor>` (`libycxx.0.<minor>.dylib`), on request, chosen per program or library ([details](docs/BUILDING_PROJECTS.md#static-and-shared-libycxx)). |

Header availability does not mean every feature works with every compiler or platform.
See [spec coverage](docs/SPEC_COVERAGE.md) and [status](STATUS.md) for the evidence and limitations.

## Using libycxx from CMake

With libycxx built and installed, configure your project with GCC 16.2+ or Clang 23+,
and make the installation discoverable through `CMAKE_PREFIX_PATH`:

```cmake
find_package(libycxx CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE ycxx::ycxx)
```

Link `ycxx::ycxx` into **every C++ target** in the program. It selects libycxx's headers
and runtime in place of the compiler's usual C++ library.

For `import std;`, link `ycxx::modules` instead. Modules need CMake 3.28+, Ninja, and,
with Clang, `clang-scan-deps`. You can also bring libycxx into a project with
`add_subdirectory` or FetchContent.

`ycxx::ycxx` links libycxx's static archives. When libycxx was built with `-DYCXX_SHARED=ON`,
`ycxx::shared` links its shared library instead (and `ycxx::static` the archives, when they were
built too); one target may not link both.

Working examples: [installed package](examples/find_package),
[add_subdirectory](examples/add_subdirectory), and [modules](examples/modules).
The [usage guide](docs/USAGE.md) covers module flags, compiler selection, and advanced setup.

## Building existing projects against libycxx

An installation includes compiler wrappers and build-system configuration files, so
existing projects can select libycxx without changing their source:

```sh
# CMake: use the toolchain file from an installation.
cmake -S proj -B build -DCMAKE_TOOLCHAIN_FILE=<prefix>/lib/cmake/libycxx/toolchain.cmake

# make: select the installed compiler wrappers.
make CXX=<prefix>/bin/ycxx-c++ CC=<prefix>/bin/ycxx-cc

# Verify the resulting binaries.
<prefix>/bin/ycxx-check-binary build
```

Replace `<prefix>` with your installation directory. **Build every C++ dependency
from source against the same libycxx installation.** Prebuilt libstdc++/libc++ dependencies
have a different ABI; C libraries are fine.

[Building projects](docs/BUILDING_PROJECTS.md) covers CMake, Meson, make, autotools,
Bazel, Conan, vcpkg, and how to verify a build.

## Building libycxx

From a checkout, provision the supported compilers and load them into your shell:

```sh
source tools/toolchain/activate.sh --provision
cmake -S . -B build/gcc -G Ninja -DCMAKE_C_COMPILER="$YCXX_GCC" -DCMAKE_CXX_COMPILER="$YCXX_GXX"
cmake --build build/gcc
cmake --install build/gcc --prefix /opt/libycxx
```

The install step may need permission to write to `/opt`; choose a writable prefix if needed.
For Clang, use `build/clang`, `$YCXX_CLANG`, and `$YCXX_CLANGXX`. Keep separate builds
and installations for each compiler. Add `-DYCXX_SHARED=ON` to build the shared library as well
(`-DYCXX_STATIC=OFF` for the shared library alone).

The [usage guide](docs/USAGE.md) explains provisioning, shell activation, freestanding
builds, custom hosted layers, and transitive includes.

## Tests

The test driver builds the library, runs the selected suites, and writes HTML and
Markdown reports with commands and failure transcripts in `build/test-logs/`.

```sh
tools/test                         # policy, builds, freestanding checks, and own tests
tools/test all                     # also CMake integration and external test suites
tools/test -c clang -f format ycxx  # focus on one compiler and test area
tools/test --help                  # stages, sanitizers, and other options
```

The library is checked with spec-derived tests, unmodified libc++ and libstdc++ test
suites, and real-world projects built against libycxx. External tests are run only,
never copied into this repository. Remaining failures and unsupported cases are documented;
see [STATUS.md](STATUS.md) for results.

The [testing guide](docs/TESTING.md) covers reports, real-world projects, writing tests,
sanitizers, failure categories, and CI.

## Documentation

| I want to… | Read |
| --- | --- |
| Try my first program | [Quick start](examples/quickstart/README.md) |
| Use libycxx in an existing project | [Building projects](docs/BUILDING_PROJECTS.md) |
| Configure compilers, modules, or custom platforms | [Usage guide](docs/USAGE.md) |
| Look up a library declaration | [API reference](https://libycxx.org/api/) |
| Understand what works and what is still open | [Status](STATUS.md) and [spec coverage](docs/SPEC_COVERAGE.md) |
| Understand implementation choices | [Design decisions](DECISIONS.md) |
| Run or extend the test suites | [Testing guide](docs/TESTING.md) |
| Compare custom standard-library workflows | [Custom standard library guide](docs/CUSTOM_STDLIB.md) |

## License

libycxx is licensed under the **Apache License 2.0 with LLVM Exceptions**
(`Apache-2.0 WITH LLVM-exception`). See [LICENSE](LICENSE) for the full terms.

Third-party material retains its own licenses, including the website fonts
([site/fonts/README.txt](site/fonts/README.txt)) and the reference material
attributed in the generated documentation and similarity analysis.

## How it was written

libycxx was implemented by AI agents (Claude, directed by the author) from the C++ working
draft, WG21 papers, POSIX and the Itanium C++ ABI. During development the agents had no access
to the sources or headers of other standard libraries or C++ runtimes (libstdc++, libc++, the
MSVC STL, libsupc++, libc++abi, libcxxrt) or of glibc, and did not read their generated code.
The libc++ and libstdc++ test suites are run unmodified as external oracles: their *test* files
were read to triage failures, and none is copied into this repository.

This is not a clean-room claim in the legal sense. The models the agents run on were trained on
public code that very likely includes those libraries, so no separation from them can be shown,
only that no implementation source was consulted while libycxx was written. A similarity
analysis compares libycxx with libstdc++, libc++ and the MSVC STL, and those three with each
other, by token and structure metrics and fingerprints, against known derived code as a positive
control; it is regenerated from pinned sources on every build of
[libycxx.org/similarity](https://libycxx.org/similarity/). Its method, thresholds and curated
judgments are in [docs/similarity/METHOD.md](docs/similarity/METHOD.md). Agents implementing
libycxx must not read the rendered pages or the Pages workflow's artifacts, which quote the
other implementations.
