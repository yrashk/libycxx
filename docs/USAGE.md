# Using libycxx

Compiler setup, CMake integration, freestanding and hosted layers, and standard library modules.
For a first program, start with the [quick start](../examples/quickstart/README.md).
For an existing project, see [Building projects](BUILDING_PROJECTS.md).

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
