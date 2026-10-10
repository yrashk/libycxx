# Building existing projects against libycxx

How to build a C++ project that was written for libstdc++ or libc++, and knows nothing about
libycxx, against libycxx: which projects can be built, how to do it with CMake, Meson, make,
autotools, Bazel and package managers, and how to check the result. Every method below was run
on Linux x86_64 with GCC 16.2 and Clang 23.1; [Verification](#verification) lists the commands,
and `tests/integration/run.sh` (part 12 of `tests/cmake/run.sh`, the `cmake` stage of
`tools/test`) repeats most of them on every CI run. Nothing here was run on macOS by hand; the
macOS code paths are those of `tools/ycxx-cxx`, which CI exercises there.

## In short

1. **Which projects qualify.** Projects that build as C++26 with GCC 16.2 or Clang 23.1 on Linux
   or macOS, and whose C++ dependencies can be built from source too: libycxx has its own ABI, so
   no prebuilt C++ library (a distribution's Boost, Qt, protobuf, GoogleTest) can be linked in. C
   libraries are fine. A project pinned to C++17/20/23 is built as C++26 (libycxx is C++26 only),
   so features C++26 removed (`auto_ptr`, `random_shuffle`, `<strstream>`, `bind1st`, ...) fail
   to compile; so do libstdc++/libc++ extensions (`__gnu_cxx::`, `<ext/...>`). Expect to add a
   few `#include`s that libstdc++ and libc++ happened to provide; see
   [the checklist](#checklist).
2. **CMake.** Pass libycxx's *installed* toolchain file:
   `-DCMAKE_TOOLCHAIN_FILE=<prefix>/lib/cmake/libycxx/toolchain.cmake`. That alone builds the
   whole project against libycxx: its targets, its `try_compile` checks, FetchContent
   dependencies, shared libraries, installed packages. (`cmake/ycxx-toolchain.cmake` in the
   source tree is a different file: it only *selects GCC 16 or Clang 23*, and a project built
   with it alone still uses libstdc++.)
3. **Not CMake.** `source tools/toolchain/activate.sh` alone is not enough: it puts the compilers
   on `PATH` but sets no `CXX`, so builds use the toolchain's own library. `activate.sh --use
   <prefix>` (or `--use gcc|clang` for this checkout's build) also exports `CXX`/`CC` (libycxx's
   compiler wrappers `ycxx-c++`/`ycxx-cc`), `CMAKE_TOOLCHAIN_FILE` and `PKG_CONFIG_PATH`. Without
   the checkout: `CXX=<prefix>/bin/ycxx-c++ CC=<prefix>/bin/ycxx-cc` for make, autotools and most
   build systems, `--native-file <prefix>/share/libycxx/meson-native.ini` for Meson, and
   `pkg-config libycxx` for a plain compiler.
4. **Anything else.** Check the result with `ycxx-check-binary`; know that shared libraries built
   with GCC export only what they mark for export, that each image has its own runtime state, and
   that libycxx has no stable ABI yet. Sections [4](#4-anything-else) and
   [Common errors](#common-errors-and-their-fixes) cover modules, IDEs, ccache, sanitizers and the
   errors met in real projects.

## Contents

- [Getting libycxx](#getting-libycxx)
- [1. Which projects qualify](#1-which-projects-qualify)
- [2. CMake projects](#2-cmake-projects)
- [3. Projects that do not use CMake](#3-projects-that-do-not-use-cmake)
- [4. Anything else](#4-anything-else)
- [Common errors and their fixes](#common-errors-and-their-fixes)
- [Verification](#verification)

## Getting libycxx

A new project written for libycxx needs no installation: `examples/quickstart` fetches libycxx with
FetchContent and its toolchain file provides the compiler (the README's quick start). This guide
is about projects that know nothing about libycxx, which are built against an installation.

Build and install it once per compiler (an installation is for the compiler it was built with):

```sh
source tools/toolchain/activate.sh          # GCC 16.2 and Clang 23.1 (tools/toolchain/provision)
cmake -S . -B build/gcc -G Ninja -DCMAKE_C_COMPILER=$YCXX_GCC -DCMAKE_CXX_COMPILER=$YCXX_GXX \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build/gcc
cmake --install build/gcc --prefix /opt/libycxx-gcc
```

Besides the headers, archives and CMake package (`find_package(libycxx)`, README), the
installation has what a project that knows nothing about libycxx needs
(`cmake/ycxx-consumer.cmake`):

| File | What it is |
|---|---|
| `bin/ycxx-c++` | The C++ compiler: runs the compiler libycxx was built with, adding libycxx's headers (`-nostdinc++ -isystem <prefix>/include/libycxx`), `-std=c++26` (an older `-std=` is raised, below) and, when linking, `-nostdlib++`, libycxx's archives and the [link options](#linker-details) |
| `bin/ycxx-cc` | The matching C compiler (with Clang on Linux, GCC 16's installation for the startup files) |
| `bin/ycxx-clang-scan-deps` | Clang builds only: the compiler's `clang-scan-deps`, where CMake looks for it next to `ycxx-c++` (module scanning of C++20 targets) |
| `bin/ycxx-check-binary` | [Is a binary built against libycxx alone?](#is-my-binary-using-libycxx) |
| `lib/cmake/libycxx/toolchain.cmake` | CMake toolchain file naming `ycxx-c++` and `ycxx-cc` |
| `lib/pkgconfig/libycxx.pc` | Compile and link flags for the plain compiler (`g++-16`, `clang++-23`) |
| `share/libycxx/meson-native.ini` | Meson native file naming `ycxx-c++` and `ycxx-cc` |

Each finds the installation relative to itself, so the prefix can be moved (the compiler's own
path is recorded as found when libycxx was configured). The build tree has the same files with
absolute paths: `build/gcc/bin/ycxx-c++`, `build/gcc/toolchain.cmake`,
`build/gcc/pkgconfig/libycxx.pc`, `build/gcc/meson-native.ini`; everything below works with
those too. They are made for the default platform layer (`YCXX_PAL=posix`); a
[hosted-layers](#freestanding-and-hosted-layers) build links the program's own providers and is
used through CMake.

`$YCXX_CXX` (and `$YCXX_CC`) make the wrappers run another compiler binary, e.g. one at a new
path; it must be the same compiler and version libycxx was built with.

## 1. Which projects qualify

### Compilers and language standard

GCC 16.2 or later in the 16 series, or Clang 23. libycxx's headers stop older compilers and
older modes with an error:

| `-std=` | Result against libycxx's headers (GCC 16.2 and Clang 23.1, checked) |
|---|---|
| `c++17`, `c++20`, `c++23`, `gnu++23`, any older | `error: "libycxx requires C++26 (-std=c++26 or -std=c++2c)"` |
| `c++2c`, `c++26`, `gnu++26` | builds |

Projects pin a standard (`CMAKE_CXX_STANDARD 17`, Meson's `cpp_std=c++17`, `-std=c++17` in a
Makefile), so the supported ways of building raise it: `ycxx-c++` replaces `-std=c++NN` and
`-std=gnu++NN` older than 26 by `-std=c++26` / `-std=gnu++26` wherever it appears, and
`ycxx::ycxx` adds `-std=c++26` after the project's flag (CMake puts a target's standard flag
first). `pkg-config --cflags libycxx` gives `-std=c++26` too; put it after the project's own
`-std=` (later wins). So a project pinned to C++17 is compiled as C++26. Most C++17/20 code is
also C++26 code; what is not is listed next.

CMake before 3.30 knows no C++26 flag: `CMAKE_CXX_STANDARD 26` (or a Conan profile's
`compiler.cppstd=26`) stops CMake 3.28 with "requires the language dialect CXX26 ... CMake does
not know the flags". Leave the standard at 23 or lower (it is raised) or use CMake 3.30+.

### Removed and deprecated features

libycxx implements C++26 and nothing the standard removed (STATUS, "Deliberate omissions"). Each
row was checked with `ycxx-c++ -fsyntax-only` (GCC 16.2):

| Feature | Removed in | With libycxx |
|---|---|---|
| `std::auto_ptr` | C++17 | error: "'auto_ptr' in namespace 'std' does not name a template type" |
| `std::random_shuffle` | C++17 | error: not a member of `std` (use `std::shuffle`) |
| `std::bind1st`, `bind2nd`, `ptr_fun`, `mem_fun`, `mem_fun_ref`, `unary_function`, `binary_function` | C++17 | error (use lambdas, `std::bind`, `std::mem_fn`) |
| `std::not1`, `not2` | C++20 | error (use `std::not_fn`) |
| `std::result_of`, `is_literal_type` | C++20 | error (use `std::invoke_result`) |
| `std::uncaught_exception()` | C++20 | error (use `std::uncaught_exceptions()`) |
| `std::get_temporary_buffer`, `raw_storage_iterator` | C++20 | error |
| `allocator<T>::construct`, `destroy`, `address`, `max_size` | C++20 | error (use `std::allocator_traits`) |
| `<ciso646>`, `<cstdbool>`, `<ctgmath>`, `<ccomplex>`, `<cstdalign>` | C++20 | error: no such file (`<ciso646>` was a common way to detect libc++: include `<version>`) |
| `std::atomic_load(shared_ptr*)` and the other `shared_ptr` atomic functions | C++26 | error (use `std::atomic<std::shared_ptr<T>>`) |
| `<strstream>` | C++26 | error: no such file (use `<spanstream>` or `<sstream>`) |
| `<codecvt>`, `std::wstring_convert`, `wbuffer_convert` | C++26 | error |
| `basic_string::reserve()` without an argument | C++26 | error (use `shrink_to_fit()`) |

Deprecated features (Annex D) are provided with a `[[deprecated]]` warning, which `-Werror`
turns into an error: `std::iterator`, `is_pod`, `is_trivial`, `aligned_storage`,
`aligned_union`, `filesystem::u8path`, `memory_order_consume`, the `unsigned char`/`signed char`
stream inserters, and the rest of STATUS, "Annex D". A project that builds with `-Werror` may
need `-Wno-deprecated-declarations` or a fix.

Language changes of C++23/26 can matter too (`std::optional` is a range, so formatting or
printing helpers that test "is it a range" change; `std::map` is constexpr; implicit moves in
`return` (P2266); comparisons of arrays are removed). The real-world projects met each of these;
see [Common errors](#common-errors-and-their-fixes).

### Platforms

Linux (glibc; x86_64 is what is tested) and macOS (Apple Silicon first; Homebrew's GCC 16 and
Clang 23 against Apple's SDK; CI runs the CMake and integration tests there). Windows is not
supported.

### ABI: every C++ dependency must be built against libycxx

libycxx's types have their own layouts and names (`std::__y1::string`, not `std::__cxx11` or
`std::__1`; DECISIONS §20.4), its own exception runtime and its own allocation functions. Code compiled against libstdc++ or libc++
cannot be linked with code compiled against libycxx in one image: the mangled names differ, so
the link fails (an undefined reference to `f(std::string)` that the library defines as
`f(std::__cxx11::basic_string<...>)`), and where it does not fail the objects disagree on
layouts. So:

- **Every C++ library a program links must be built from source against libycxx**: GoogleTest,
  {fmt}, Boost's compiled libraries, Qt, protobuf, gRPC, Abseil. Prebuilt C++ packages
  (`libboost-*-dev`, `qt6-base-dev`, `libprotobuf-dev`, `libgtest-dev`, Homebrew's bottles) do not
  work. [Dependencies](#dependencies) shows how to build them: a prefix, FetchContent, Conan, vcpkg.
- **Header-only C++ libraries** work from wherever they are installed (a distribution's
  `nlohmann-json3-dev`, Boost's header-only parts, Eigen), as long as nothing links one of their
  compiled parts and they do not detect the library by its macros (below).
- **C libraries are fine** (zlib, OpenSSL, libcurl, SQLite, the C library itself), and so are C
  APIs of C++ libraries (`extern "C"` interfaces that pass no C++ types), even when the library
  is built with libstdc++: it is a separate image with its own runtime.
- **Plugins and hosts**: a program built with libycxx can load a shared library built with
  libstdc++ or libc++ and the other way round; each keeps its own runtime
  (`tests/cmake/visibility` checks it). Only C interfaces may cross between them.

### Shared libraries

libycxx comes in two kinds (DECISIONS §20.2; [Static and shared libycxx](#static-and-shared-libycxx)
says how to choose). Each image (executable or shared library) links one of them; a process may
mix images of both kinds, and images built with libstdc++ or libc++. What a project with shared
libraries sees depends on the kind:

- **Static libycxx** (the default). Each image linking the archives gets its own copy, with
  everything hidden (DECISIONS §2); the images share only libycxx's allocation table, so that an
  object allocated in one image and freed in another uses one `operator new`/`delete` (the
  program's replacement, if any). Consequences (STATUS, "Known limitations"):
  - **With GCC, a function whose signature names a standard library type is hidden** unless the
    function itself is declared with default visibility: GCC constrains a declaration's
    visibility by its types', and libycxx's types are hidden. A shared library that exports
    `std::string greet(const std::vector<std::string>&)` without marking it gives "undefined
    reference to `greet(...)`" when a program links it (checked). Libraries with export macros
    (`GTEST_API_`, `FMT_API`, Qt's `Q_DECL_EXPORT`, CMake's `GenerateExportHeader`) work, since
    those expand to `__attribute__((visibility("default")))` on ELF when the library is built as
    shared; a project relying on everything being exported does not. Clang has no such rule. Mark
    the exported functions (`[[gnu::visibility("default")]]`), build the library static, use
    Clang, or use the shared libycxx.
  - **Per-image runtime state**: `std::set_terminate`, `std::set_new_handler`, `throw;` and
    `std::current_exception()` in another image than the handler's, `std::uncaught_exceptions()`,
    `std::error_category` objects (`generic_category()` compares unequal across images),
    `locate_zone`, the default memory resources, the standard streams' objects: each image has
    its own. Tests that install a handler in the program and expect a shared library to call it
    (doctest's exception translators, oneTBB's `terminate_on_exception`) do not work.
- **Shared libycxx.** Every image needs `libycxx.so.0.<minor>` (`libycxx.0.<minor>.dylib`) and
  links its small per-image part, `libycxx_nonshared.a`. The library and its runtime exist once in
  the process: handlers, the current exception, `uncaught_exceptions()`, the error categories, the
  streams and the memory resources are shared by every shared-mode image, and GCC's visibility
  rule above does not apply (the library's types have default visibility). Each image still has its
  own hidden Itanium entry points, `std::nothrow` and default allocation functions, bound to the
  program's through the allocation table, so no name another C++ runtime defines is exported.
- **Either kind**:
  - **Exceptions cross** between libycxx images (a shared library throws, the program catches), and
    the standard library types cross (`std::string` built in one, destroyed in another), static
    and shared images alike: the names (`std::__y1`, DECISIONS §20.4) and layouts are the same.
    Between a static-mode image and a shared-mode one the runtime state is per image, as between
    two static ones.
  - **Exceptions from code built against another library** (libstdc++, libc++, Apple's libc++abi)
    are foreign to libycxx's runtime: `catch (...)` catches them, `catch (const std::exception&)`
    does not, and the other way round. Do not let exceptions cross a C++ boundary between
    libraries built against different standard libraries.
  - **One libycxx version per process**: every image must be built against the same libycxx
    (layouts are not stable during 0.x, DECISIONS §20.7).

### Libraries that detect the standard library by its macros

libycxx defines neither `__GLIBCXX__`/`_GLIBCXX_RELEASE` nor `_LIBCPP_VERSION`, and no macro of
its own meant for detection. Code that decides "GCC means libstdc++" or "not libc++ means
libstdc++" takes a wrong branch:

- `#if defined(__GLIBCXX__)` around a workaround or an extension: the workaround is skipped.
  nlohmann/json's tests guarded `std::char_traits<std::byte>` with it (an extension libycxx does
  not provide); glaze spelled type names as libstdc++ does.
- `#ifdef _LIBCPP_VERSION ... #else` assuming libstdc++ otherwise: Catch2 and doctest included
  `<ciso646>` to tell (a removed header); range-v3 recognises libstdc++ and libc++ internals.
- Feature detection by library: GoogleTest uses `<cxxabi.h>` (type names, demangling) only when it
  recognises the library; libycxx provides `<cxxabi.h>` and `abi::__cxa_demangle`, so set
  `-DGTEST_HAS_CXXABI_H_=1`.
- Declaring standard templates instead of including their headers ([namespace.std]/1: undefined
  behavior). libycxx defines them in the inline namespace `std::__y1`, so a declaration of
  `std::tuple` or `std::basic_ostream` in `namespace std` declares another template and makes the
  name ambiguous ("reference to 'tuple' is ambiguous"). doctest does this unless it finds libc++:
  define `DOCTEST_CONFIG_USE_STD_HEADERS`.

Use the feature-test macros (`__cpp_lib_*` from `<version>`) and `__has_include` instead.

### Transitive includes

Which standard headers a standard header includes is unspecified ([res.on.headers]). Much code
uses `std::min` after including only `<string>`, `errno` after `<mutex>` or `std::abort` after
`<memory>`, because libstdc++ and libc++ happen to provide them. In this version libycxx's
headers include only what the standard and the implementation need, so such code fails with
"'abort' is not a member of 'std'", "'errno' was not declared in this scope" and the like: the
most common error in real projects (STATUS, "Real-world projects", lists 14 projects' cases). The
fix is the missing `#include`; without touching the project, force-include the header, e.g.
`CXXFLAGS="-include cstdlib -include cerrno"` (how toml++'s vendored Catch2 was built below).

Work in progress (another branch) makes each libycxx header also include what both libstdc++ and
libc++ provide with it, by default, with `-DYCXX_NO_TRANSITIVE_INCLUDES` to turn it off; once it
lands, most of these errors go away by default. Check `README.md` of your checkout for
`YCXX_NO_TRANSITIVE_INCLUDES`.

### libstdc++ and libc++ extensions

Not provided: `__gnu_cxx::` (`stdio_filebuf`, `__normal_iterator`, policy-based data
structures), the `<ext/...>` and `<tr1/...>` headers, `<bits/...>` internals,
`std::char_traits<unsigned char>`/`<std::byte>` (`std::basic_string<unsigned char>` is an
"incomplete type" error), libc++'s `std::__1::` names and `_LIBCPP_*` configuration macros, and
libstdc++'s debug mode (`_GLIBCXX_DEBUG`, `_GLIBCXX_ASSERTIONS`: defining them does nothing; use
[`YCXX_HARDENED`](#sanitizers-and-hardened-mode)). `<bits/stdc++.h>` is provided (every standard
header), and so is `<cxxabi.h>` (`abi::__cxa_demangle`, `__cxa_current_exception_type`).

A compiler header that includes libstdc++ itself fails too: GCC 16's `<omp.h>` includes
libstdc++'s `<bits/new_throw.h>`, so OpenMP with GCC does not build (oneTBB's `test_openmp`).

### Checklist

- [ ] GCC 16.2+ or Clang 23 on Linux or macOS.
- [ ] The project builds as C++26 (no removed features; deprecated ones under `-Werror`).
- [ ] Every C++ dependency can be built from source (or is header-only); no prebuilt C++ package.
- [ ] No `__gnu_cxx::`, `<ext/...>`, `<tr1/...>`, `std::__1`, `char_traits<unsigned char>`.
- [ ] Library detection by `__GLIBCXX__`/`_LIBCPP_VERSION` reviewed.
- [ ] Shared libraries: exported functions marked for export (GCC), no handler or
      `current_exception` expected to cross images, no exceptions to or from code built against
      another library.
- [ ] Missing `#include`s added (or force-included) where the compiler says so.

### Compatibility smoke test

Three steps, a few minutes for most projects:

```sh
# 1. Look for what libycxx does not provide (each hit is a place to review, not always an error:
#    {fmt} tests _LIBCPP_VERSION and __GLIBCXX__ only to pick a path that also works without them).
grep -rnE 'auto_ptr|random_shuffle|bind1st|bind2nd|ptr_fun|mem_fun|unary_function|binary_function|<strstream>|<ciso646>|<codecvt>|wstring_convert|result_of<|__gnu_cxx|<ext/|<tr1/|_LIBCPP_VERSION|__GLIBCXX__|std::__1|__cxx11' \
  --include='*.h' --include='*.hpp' --include='*.cc' --include='*.cpp' --include='*.cxx' src include

# 2. Build everything you can, collecting every error rather than stopping at the first.
cmake -S . -B build-ycxx -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/libycxx-gcc/lib/cmake/libycxx/toolchain.cmake
cmake --build build-ycxx -- -k 0 2>&1 | grep -E 'error|undefined reference' | sort | uniq -c | sort -rn | head -40

# 3. Check that what was built is libycxx's alone.
/opt/libycxx-gcc/bin/ycxx-check-binary build-ycxx
```

The errors of step 2 sort into the categories of [Common errors](#common-errors-and-their-fixes).

## 2. CMake projects

### The two toolchain files

| File | What it does | Builds against |
|---|---|---|
| `<libycxx>/cmake/ycxx-toolchain.cmake` (source tree) | finds or provisions GCC 16.2 / Clang 23.1 and makes them the project's compilers; for building libycxx itself, and for projects that use `find_package(libycxx)` | the toolchain's own library, unless the project links `ycxx::ycxx` |
| `<prefix>/lib/cmake/libycxx/toolchain.cmake` (installed; `build/<cc>/toolchain.cmake` in a build tree) | makes `ycxx-c++` and `ycxx-cc` the project's compilers | libycxx, for everything the project compiles |

Checked: a project with `CMAKE_CXX_STANDARD 17` configured with `cmake/ycxx-toolchain.cmake`
alone compiles with `g++-16 -std=gnu++17` (no `-nostdinc++`), and its program needs
`libstdc++.so.6`.

### Recommended: the installed toolchain file

```sh
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/libycxx-gcc/lib/cmake/libycxx/toolchain.cmake
cmake --build build
ctest --test-dir build
/opt/libycxx-gcc/bin/ycxx-check-binary build
```

The C++ compiler is the wrapper, so libycxx is in every command CMake runs, whatever the project
does with its flags. Checked with `tests/integration/cmake-project` (GCC and Clang, Ninja and
Unix Makefiles) and with real projects ([Verification](#verification)):

| What | Result |
|---|---|
| `CMAKE_CXX_STANDARD 17` (`-std=gnu++17` in the commands) | compiled as C++26 |
| `check_cxx_source_compiles`, `check_include_file_cxx`, any `try_compile` | run with libycxx (the toolchain file is read again in each `try_compile` project) |
| `FetchContent` / `add_subdirectory` dependencies | built with libycxx (same configuration) |
| `ExternalProject` | built with libycxx **only if** the project passes the toolchain on (`CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}`), or with `CXX`/`CC` [in the environment](#alternatives) of the build; otherwise it is built against the toolchain's library and the link fails (checked) |
| `find_package` of C++ dependencies | finds what `CMAKE_PREFIX_PATH` names; they must have been built against libycxx ([Dependencies](#dependencies)) |
| Shared libraries, RPATH | work; see [Shared libraries](#shared-libraries). On Linux the wrapper adds `-Wl,-rpath,<GCC 16's lib64>` (its `libgcc_s.so.1`); `cmake --install` keeps it next to the project's own `INSTALL_RPATH` (`$ORIGIN/../lib:/opt/gcc-16/lib64`) |
| `install(EXPORT)` and a consumer with `find_package` | work (the consumer is configured with the same toolchain file) |
| Ninja, Unix Makefiles | both |
| C++20 module scanning (CMake 3.28+, a target at C++20 or later, Ninja) | with Clang, CMake runs `clang-scan-deps`, which it looks for next to the compiler: the toolchain file of a Clang build names the compiler's own (`CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS`), and `bin/ycxx-clang-scan-deps` is where CMake finds it without the toolchain file (`CC`/`CXX`). It reads the command as written, without the wrapper's flags, which is enough to find module imports; for `import std;` use `ycxx::modules` ([Modules](#modules-import-std)) |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | the commands name `ycxx-c++`: see [IDEs](#ides-compile_commandsjson-and-clangd) |
| `CMAKE_CXX_COMPILER_LAUNCHER=ccache`, `target_precompile_headers` | work |

### Alternatives

| Method | Command | Covers | Does not cover |
|---|---|---|---|
| Installed toolchain file (recommended) | `-DCMAKE_TOOLCHAIN_FILE=<prefix>/lib/cmake/libycxx/toolchain.cmake` | everything above | ExternalProject that does not pass the toolchain on |
| Compilers in the environment | `CXX=<prefix>/bin/ycxx-c++ CC=<prefix>/bin/ycxx-cc cmake ...`, and the same variables while building | everything, ExternalProject too (it runs CMake at build time and inherits the environment) | a build started without the variables: only the first configure reads them |
| Activation | `source tools/toolchain/activate.sh --use <prefix>` | exports `CXX`, `CC` and `CMAKE_TOOLCHAIN_FILE` (CMake 3.21+ reads it from the environment), so plain `cmake -S . -B build` works, ExternalProject too | shells other than bash, zsh and fish |
| The compiler on the command line | `-DCMAKE_CXX_COMPILER=<prefix>/bin/ycxx-c++ -DCMAKE_C_COMPILER=<prefix>/bin/ycxx-cc` | like the toolchain file | ExternalProject unless passed on (checked: its archive is built against libstdc++) |
| `find_package(libycxx)` + `ycxx::ycxx` in the project | `target_link_libraries(app PRIVATE ycxx::ycxx)` for every C++ target, plain `g++-16`/`clang++-23` | targets that link it (README, "Using libycxx from CMake"); `compile_commands.json` carries the flags, so IDEs see libycxx | `try_compile` checks, targets that forget it, ExternalProject |
| Injected into an unmodified project | `-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=inject.cmake` with `find_package(libycxx CONFIG REQUIRED)` and `link_libraries(ycxx::ycxx)` in it, plus `-DCMAKE_PREFIX_PATH=<prefix>` | the project's targets | `try_compile` checks (they ran against libstdc++, checked), ExternalProject (link failed, checked), imported targets; not recommended |

`tools/realworld` uses the first method's idea: its generated toolchain file names a two-line
`c++` that runs `tools/ycxx-cxx` (the source tree's wrapper, which `ycxx-c++` mirrors), plus a
record of every command for the linkage proof. That is why the 19 real-world projects build
without changes to their build files (their patches are to source code: STATUS).

### Dependencies

Build every C++ dependency against the same libycxx installation, then point the project at it.

**A dependency prefix.** Install each dependency with the toolchain file into one prefix, and
pass it with `CMAKE_PREFIX_PATH`. Checked with GoogleTest v1.18.0 and EnTT v4.0.0:

```sh
TC=/opt/libycxx-gcc/lib/cmake/libycxx/toolchain.cmake
cmake -S googletest -B build/gtest -G Ninja -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_INSTALL_PREFIX=$HOME/ycxx-deps \
      -DCMAKE_CXX_FLAGS=-DGTEST_HAS_CXXABI_H_=1
cmake --build build/gtest && cmake --install build/gtest
cmake -S entt -B build/entt -G Ninja -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_PREFIX_PATH=$HOME/ycxx-deps \
      -DENTT_BUILD_TESTING=ON -DENTT_FIND_GTEST_PACKAGE=ON
cmake --build build/entt && ctest --test-dir build/entt
```

Keep this prefix separate from any prefix with libstdc++ builds, and from the system's
(`/usr/lib/cmake/GTest` would be found otherwise when the prefix lacks a package:
`-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF` does not exclude `/usr`; check with
`ycxx-check-binary`, which flags a libstdc++ archive).

**FetchContent** builds dependencies in the project's configuration, so they get libycxx as
long as the project is configured with the toolchain file (checked, `tests/integration`).
`FETCHCONTENT_TRY_FIND_PACKAGE_MODE` / `FIND_PACKAGE_ARGS` may prefer an installed package: make
sure it is a libycxx build.

**Conan 2.** `examples/package-managers/conan`: `settings_user.yml` adds `compiler.libcxx=libycxx`
(so libycxx packages get their own package IDs and are never mixed with libstdc++ ones) and Clang
23; the profile `ycxx` sets that, chains libycxx's toolchain file into Conan's CMake toolchain
(`tools.cmake.cmaketoolchain:user_toolchain`), and names `ycxx-c++`/`ycxx-cc` for other build
systems. Checked with {fmt} 12.2.0 from ConanCenter:

```sh
cp examples/package-managers/conan/settings_user.yml "$(conan config home)/"
YCXX_PREFIX=/opt/libycxx-gcc conan install examples/package-managers/conan --output-folder=build/conan \
    -pr:h examples/package-managers/conan/ycxx -pr:b default --build=missing
cmake -S examples/package-managers/app -B build/conan/build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/conan/build
```

The profile uses `compiler.cppstd=23` (the wrapper raises it): 26 needs CMake 3.30+ in the
recipes' builds. The build profile (`-pr:b default`) stays the toolchain's: tools built for the
build machine do not need libycxx. ConanCenter's {fmt} 11.1.4 does not build (it calls `malloc`
without `<cstdlib>`, a transitive include); 12.2.0 does.

**vcpkg.** `examples/package-managers/vcpkg/x64-linux-ycxx.cmake`, an overlay triplet that
chainloads libycxx's toolchain file (`VCPKG_CHAINLOAD_TOOLCHAIN_FILE`) for every port, static
libraries (`VCPKG_LIBRARY_LINKAGE static`), and its own triplet name, so its binary cache is
separate. The project itself also chainloads it:

```sh
YCXX_PREFIX=/opt/libycxx-gcc cmake -S examples/package-managers/app -B build/vcpkg -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_OVERLAY_TRIPLETS=examples/package-managers/vcpkg -DVCPKG_TARGET_TRIPLET=x64-linux-ycxx \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/opt/libycxx-gcc/lib/cmake/libycxx/toolchain.cmake
```

Checked with a local overlay port (`tests/integration/vcpkg`: the port builds with libycxx's
compilers and the program links it, `ycxx-check-binary` accepts both); ports from the registry
could not be downloaded in the sandbox this was written in (its egress policy denies GitHub
archive downloads), so {fmt} through vcpkg was not run. For aarch64 or macOS, copy the triplet
and change `VCPKG_TARGET_ARCHITECTURE`/`VCPKG_CMAKE_SYSTEM_NAME`.

### Sanitizers and hardened mode

**Sanitizers.** A program compiled with `-fsanitize=...` links against an uninstrumented libycxx
fine for AddressSanitizer and UBSan (the sanitizer sees the program's code; libycxx's allocation
functions give way to the sanitizer's, CUSTOM_STDLIB §5). For ThreadSanitizer, and to see inside
the library, build an instrumented libycxx and use *its* wrapper or toolchain file:

```sh
cmake -S . -B build/clang-tsan -G Ninja -DCMAKE_CXX_COMPILER=clang++-23 -DCMAKE_C_COMPILER=clang-23 \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DYCXX_SANITIZE=thread
cmake --build build/clang-tsan && cmake --install build/clang-tsan --prefix /opt/libycxx-clang-tsan
cmake -S proj -B build-tsan -DCMAKE_TOOLCHAIN_FILE=/opt/libycxx-clang-tsan/lib/cmake/libycxx/toolchain.cmake \
      "-DCMAKE_CXX_FLAGS=-fsanitize=thread" "-DCMAKE_C_FLAGS=-fsanitize=thread"
```

`YCXX_SANITIZE` takes `address`, `undefined`, `thread` (comma-separated); such a build's
`ycxx-c++`, `libycxx.pc` and `ycxx::ycxx` add `-fsanitize=<list>` to every link (and, for Clang's
TSan, `-fno-sanitize-link-c++-runtime`); the project adds it to its compilations. GCC needs its
sanitizer runtimes (`tools/toolchain/provision --with-sanitizers`). `tools/realworld -s asan`
builds all 19 projects this way.

**Hardened mode.** `-DYCXX_HARDENED=1` (on the command line, for every translation unit) checks
the library's preconditions at run time (`vector::operator[]` out of range, `front()` of an empty
container, `*` of an empty `optional`, ...) and stops the program. It is a header switch: no
other libycxx build is needed. Define it the same way in all of a program's translation units
(mixing is an ODR violation). libstdc++'s `_GLIBCXX_ASSERTIONS` and libc++'s
`_LIBCPP_HARDENING_MODE` do nothing.

### Static and shared libycxx

libycxx builds static archives (`libycxx.a`, `libycxx-abi.a`, position-independent), a shared
library (`libycxx.so.0.<minor>` with `libycxx_nonshared.a`; on macOS `libycxx.0.<minor>.dylib`), or
both (DECISIONS §20.2). Configure with

| | `-DYCXX_STATIC` | `-DYCXX_SHARED` | what the unqualified consumers link |
|---|---|---|---|
| default | `ON` | `OFF` | the archives |
| both | `ON` | `ON` | the archives; the shared library on request |
| shared only | `OFF` | `ON` | the shared library |

The shared library needs `YCXX_PAL=posix` and no `YCXX_SANITIZE` (instrumented builds stay
static for now).

**Choosing per image.** Every translation unit of an image is compiled in the mode the image is
linked in: shared mode compiles with `YCXX_SHARED` defined. A mismatch fails to link, naming
`__ycxx_linkage_static_v1` or `__ycxx_linkage_shared_v1` (each translation unit refers to its mode's
marker). The ways of linking each kind:

| | static | shared |
|---|---|---|
| CMake package | `ycxx::static` | `ycxx::shared` (CMake stops a target whose dependencies link both) |
| `ycxx::ycxx` | the default kind | |
| modules | `ycxx::modules` (default kind) | `ycxx::shared_modules` (when both are built) |
| `ycxx-c++` | `YCXX_LINKAGE=static` | `YCXX_LINKAGE=shared` (adds a run path; `YCXX_NO_RPATH=1` drops it) |
| pkg-config | `libycxx-static.pc` | `libycxx-shared.pc` (`libycxx.pc`: the default kind) |
| Meson | `meson-native.ini` | `meson-native-shared.ini` (both built), or `meson-native.ini` of a shared-only build |
| `activate.sh` | `--use DIR` | `--use DIR --shared` (`--use gcc --shared`: `build/gcc-shared`) |
| `tools/ycxx-cxx` (source tree) | default | `--shared` (`build/<compiler>-shared`) |

**Static.** Every executable and shared library linking libycxx contains the parts it uses,
hidden. What the images share is the allocation table (`__ycxx_allocation_functions`, exported
from each image and bound by the dynamic linker to the program's), so that one
`operator new`/`delete` serves all of them: a `std::string` created in a shared library and
destroyed in the program works, and a program's replacement `operator new` serves its libycxx
shared libraries. The link options that keep the table (below) are in every supported way of
linking; a hand-written link line must add them.

**Shared.** Programs need the library at run time: the consumers above record a run path to the
library's directory (CMake's own `RPATH` handling for `ycxx::shared`), and an installation moved
elsewhere needs `LD_LIBRARY_PATH` or a new run path. On ELF, the installed `libycxx.so` is a linker
script naming `libycxx.so.0.<minor>` and `libycxx_nonshared.a`, so `-lycxx` links both (GNU ld and
lld; other linkers take the two files by name, as the build tree does). The library exports only
`std::__y1`, `__ycxx`, the runtime's entry points under its own names (`__ycxx_abi_*`) and the
allocation table, with the ELF symbol version `YCXX_0.<minor>`. Hand-written link lines: compile
with `-DYCXX_SHARED`, link `-nostdlib++ <libdir>/libycxx.so.0.1 <libdir>/libycxx_nonshared.a` (macOS:
`libycxx.0.1.dylib`) with the allocation table's options and `-lm`.

## 3. Projects that do not use CMake

### The compiler wrapper: make, raw compiler invocations, most build systems

```sh
make CXX=/opt/libycxx-gcc/bin/ycxx-c++ CC=/opt/libycxx-gcc/bin/ycxx-cc
/opt/libycxx-gcc/bin/ycxx-c++ -O2 main.cpp util.cpp -o app      # compile and link
```

`ycxx-c++` takes everything the compiler takes, and adds libycxx's flags (the archives only when
linking: not with `-c`, `-S`, `-E`, `-M`, `-MM`, `-fsyntax-only`). Queries (`--version`,
`-dumpversion`, `-print-*`) go to the compiler unchanged, so build systems identify it as GCC or
Clang. Checked with `tests/integration/make-project` (threads, `<format>`, C sources) and toml++.

`tools/ycxx-cxx gcc|clang` is the same for this checkout's `build/<compiler>` (and
`--libdir=`, `--std-modules`); `ycxx-c++` is what an installation has.

### pkg-config, for the plain compiler

```make
CXX = g++-16
CXXFLAGS += $(shell pkg-config --cflags libycxx)     # after the project's own -std=
LDLIBS   += $(shell pkg-config --libs libycxx)       # after the objects
```

with `PKG_CONFIG_PATH=/opt/libycxx-gcc/lib/pkgconfig`. The flags, for a GCC build on Linux:

```
Cflags: -std=c++26 -nostdinc++ -isystem ${includedir} -Wno-attributes
Libs: -nostdlib++ -Wl,-u,__ycxx_allocation_table_anchor -Wl,--export-dynamic-symbol=__ycxx_allocation_functions
      -Wl,--start-group ${libdir}/libycxx.a ${libdir}/libycxx-abi.a -Wl,--end-group -lm -shared-libgcc
      -Wl,-rpath,/opt/gcc-16/lib64
```

(a Clang build adds `--gcc-install-dir=<GCC 16's>` to both). The `.pc` file is for the
compiler libycxx was built with (its description names it). Checked with
`tests/integration/make-project` and both compilers. Meson's `dependency('libycxx')` reads the
same file, but the native file below is simpler and also covers Meson's own checks.

### Meson

```sh
meson setup build --native-file /opt/libycxx-gcc/share/libycxx/meson-native.ini
meson compile -C build && meson test -C build
```

The native file names `ycxx-c++` and `ycxx-cc` (as paths relative to itself, Meson's
`@DIRNAME@`, so the installation can move); `cpp_std=c++17` in a project's `default_options` is
raised. Checked with toml++ v3.4.0 and its Meson-only test suite (9 test runs, 46888 assertions,
both compilers) with Meson 1.12.1:

```sh
CXXFLAGS="-include cstdlib" meson setup build tomlplusplus -Dbuild_tests=true \
    --native-file /opt/libycxx-gcc/share/libycxx/meson-native.ini
meson compile -C build && meson test -C build
```

(`-include cstdlib`: toml++'s vendored Catch2 calls `std::abort` without `<cstdlib>`.)

### autotools

```sh
./configure CXX=/opt/libycxx-gcc/bin/ycxx-c++ CC=/opt/libycxx-gcc/bin/ycxx-cc
make && make check
```

`AC_PROG_CXX`, `AC_CHECK_HEADERS` and `AC_COMPILE_IFELSE` run through the wrapper, so configure's
answers are libycxx's; `AX_CXX_COMPILE_STDCXX`'s `-std=c++17` is raised. Checked with
`tests/integration/autotools-project`.

### Bazel

Not run here (no Bazel in the environment this was written in); what is needed:

- Bazel's auto-configured C++ toolchain (`@bazel_tools//tools/cpp:cc_configure`) takes one
  compiler driver from `CC` for C and C++, and its default link flags name libstdc++ (the
  `BAZEL_LINKLIBS`/`BAZEL_LINKOPTS` defaults). So give it the plain compiler and libycxx's flags
  through its documented variables, replacing the defaults: `BAZEL_CXXOPTS` with
  `pkg-config --cflags libycxx` and `BAZEL_LINKOPTS`/`BAZEL_LINKLIBS` with
  `pkg-config --libs libycxx` (colon-separated lists), passed as `--repo_env=...`. An explicit
  `-lstdc++` that remains on the link line defeats `-nostdlib++`.
- The robust form is a hand-written `cc_toolchain` (`cc_toolchain_config`) whose C++ compile and
  link actions run `<prefix>/bin/ycxx-c++` (and C compile actions `ycxx-cc`), with no
  `-lstdc++` in its link flags; registered with `--extra_toolchains`.
- Every C++ dependency (`http_archive`, Bazel Central Registry modules) is then compiled with that
  toolchain, as Bazel builds everything from source.

Check the result with `ycxx-check-binary bazel-bin/...`.

### Is `activate.sh` enough?

`source tools/toolchain/activate.sh` (or `activate.fish`) exports `YCXX_ROOT`, `YCXX_GCC`,
`YCXX_GXX`, `YCXX_GCC_INSTALL_DIR`, `YCXX_CLANG`, `YCXX_CLANGXX`, `YCXX_LLD`, `YCXX_LLVM_AR` (and
`SDKROOT` on macOS) and puts the compilers' directories and `tools/` on `PATH`. It sets no `CXX`
or `CC`, and `PATH` gets the raw `g++-16`/`clang++-23`: a build in that shell uses the
toolchain's own library. That is what building libycxx and running its tests needs.

To build other projects against libycxx in a shell:

```sh
source tools/toolchain/activate.sh --use gcc            # this checkout's build/gcc
source tools/toolchain/activate.sh --use /opt/libycxx-gcc   # an installation (or a build tree)
ycxx-unload                                             # restores everything
```

`--use` also exports `CXX` and `CC` (the wrappers), `CMAKE_TOOLCHAIN_FILE`, `PKG_CONFIG_PATH`
(with `libycxx.pc`) and `YCXX_USE`, and puts the installation's `bin/` on `PATH`. Then `make`,
`./configure`, `cmake -S . -B build` (CMake 3.21+ reads `CMAKE_TOOLCHAIN_FILE` from the
environment) and `meson setup` (it reads `CC`/`CXX`) build against libycxx. Without the checkout,
the same in three lines:

```sh
export CXX=/opt/libycxx-gcc/bin/ycxx-c++ CC=/opt/libycxx-gcc/bin/ycxx-cc
export CMAKE_TOOLCHAIN_FILE=/opt/libycxx-gcc/lib/cmake/libycxx/toolchain.cmake
export PKG_CONFIG_PATH=/opt/libycxx-gcc/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
```

### Linker details

What every link of a libycxx program or shared library needs (the wrapper, `libycxx.pc` and
`ycxx::ycxx` all pass it; a hand-written link line must):

| Option | Why |
|---|---|
| `-nostdlib++` | no libstdc++/libc++. With GCC it also drops `-lm`, hence `-lm` |
| `libycxx.a libycxx-abi.a` in `-Wl,--start-group ... --end-group` (ELF) | the library and its ABI runtime refer to each other; Apple's linker needs no group |
| `-Wl,-u,__ycxx_allocation_table_anchor` | keeps the allocation table in the program even when the program replaces every allocation function (`_` prefix on Mach-O: the build records the right spelling) |
| `-Wl,--export-dynamic-symbol=__ycxx_allocation_functions` (ELF, when the linker has it) | exports the table, so that shared libraries loaded later bind to the program's |
| `-shared-libgcc` (Linux) | one unwinder per process: glibc's `pthread_exit`/`pthread_cancel` unwind through `libgcc_s.so.1`, and a second, static unwinder would abort |
| `-Wl,-rpath,<GCC 16's libgcc_s directory>` (Linux) | run with GCC 16's `libgcc_s.so.1`, not an older system copy |
| `--gcc-install-dir=<GCC 16's>` (Clang, Linux) | Clang otherwise takes the startup files and libgcc of the newest GCC it finds, which may be older |
| `-static-libgcc` (GCC, macOS) | GCC's Darwin libgcc; unwinding is libSystem's |
| `-Wno-attributes` (GCC, compiling) | silences GCC's warning about program classes with members of hidden library types (STATUS) |

The exact options of a build are in `<build>/ycxx-link-options` and in `pkg-config --libs
libycxx`. On macOS there is no `-rpath`, `--export-dynamic-symbol` or group (the archives are
bound in the executable, two-level namespace; DECISIONS §2, STATUS "macOS").

## 4. Anything else

### Modules: `import std;`

From CMake (3.28+, Ninja): `find_package(libycxx)` and `target_link_libraries(app PRIVATE
ycxx::modules)`, with the plain compiler; CMake builds the interfaces of `std` and `std.compat`
for the project (README, "Modules"; `examples/modules`). CMake's own `CMAKE_CXX_MODULE_STD` is not
supported (it would build the toolchain library's module). The installed `ycxx-c++` does not
build or find modules; outside CMake use `tools/ycxx-modules` and `tools/ycxx-cxx --std-modules`
from the source tree. GCC 16 cannot mix `#include <...>` of a standard header and `import std;`
in one translation unit.

### Freestanding and hosted layers

A freestanding program uses `ycxx::freestanding` (`-DYCXX_FREESTANDING_RUNTIME=ON`; README). A
hosted program without POSIX (your own heap, console, files: `-DYCXX_PAL=none
-DYCXX_HOSTED_LAYERS=...`) links its providers through `ycxx_add_hosted_layer` from CMake;
`examples/hosted-layers/README.md` has three working examples, DECISIONS §18 the design. Such a
build has no `ycxx-c++`, `libycxx.pc` or Meson file: its link needs the program's providers.

### Mixing C and C++

C needs nothing of libycxx: compile C with the plain C compiler (`ycxx-cc` is that, with Clang's
`--gcc-install-dir` on Linux), and link the program with the C++ driver (`ycxx-c++`), as CMake
does for a target with C and C++ sources (checked in `tests/integration/cmake-project` and the
make, Meson and autotools samples). A C++ library used from C exposes `extern "C"` functions;
with GCC, mark them for export if their parameters name library types (they would be hidden).
Linking a C++ target with the C driver (`gcc ... -lfoo`) misses libycxx: let the C++ driver link.

### Precompiled headers

`target_precompile_headers` works with the wrapper (both compilers, checked), as does
`-include`/`.gch` by hand, as long as the header is compiled with the same compiler and flags as
the sources. A precompiled header made with the plain compiler holds libstdc++'s headers: rebuild
it with `ycxx-c++`.

### ccache

`-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` (and `_C_`) with CMake, `make CXX="ccache
/opt/libycxx-gcc/bin/ycxx-c++"` with make: ccache 4.9 hashes the wrapper (its default `compiler_check=mtime`, so reinstalling
libycxx invalidates the cache) and the headers it includes; a second build of
`tests/integration/cmake-project` was all hits. The `ccache` symlink farm
(`/usr/lib/ccache/g++`) does not help: it wraps `g++`, not `ycxx-c++`.

### IDEs: `compile_commands.json` and clangd

With the wrapper, `compile_commands.json` holds `ycxx-c++ ... -std=gnu++17 ...`: the include
directory and `-std=c++26` are added when the wrapper runs, so a tool that reads the database
without running the compiler does not see them. clangd (checked with clangd 22.1):

- without help it parses with its own default library (libstdc++, C++17) and reports no error,
  which is misleading;
- with `--query-driver=/opt/libycxx-gcc/bin/ycxx-*` it asks the wrapper for its include
  directories (it gets libycxx's and drops libstdc++'s) but keeps `-std=gnu++17`, so libycxx's
  headers stop with "libycxx requires C++26";
- with a `.clangd` file in the project it works (0 errors):

```yaml
CompileFlags:
  Remove: [-std=*]
  Add: [-std=c++26, -nostdinc++, -isystem/opt/libycxx-gcc/include/libycxx]
```

With `find_package(libycxx)` and `ycxx::ycxx`, the flags are in the database itself and clangd
needs nothing. Use a clangd at least as new as the Clang libycxx supports (23); an older one may
not parse every header (clangd 22 parsed the samples).

### Debugging

No GDB or LLDB pretty printers exist yet: `std::string`, `std::vector` and the other containers
show as their members (`__y_...` fields). Debug information and stack traces work as usual
(`-g`); `<stacktrace>` names frames on Linux (ELF and DWARF).

### Is my binary using libycxx?

```sh
/opt/libycxx-gcc/bin/ycxx-check-binary build/app build/libfoo.so build/libbar.a   # or a directory
tools/ycxx-check-binary build                                                    # from the checkout
```

For each executable, shared library, archive or object (a directory is searched), it checks what
`tools/realworld` proves for every real-world project (`tools/lib/ycxx_linkage.py`): no
libstdc++/libc++ among the needed libraries; no libstdc++/libc++ symbol, defined or undefined
(`std::__cxx11`, `__gnu_cxx::`, `std::__1`, any other `std::__` name but libycxx's own
`std::__y1`, `GLIBCXX_`/`CXXABI_`
versions), which an object compiled with the toolchain's headers has; and, for executables and
shared libraries with C++ code, libycxx's allocation table. One line per file, exit status 1 if
any fails:

```
OK   build/app: exe: libycxx (allocation table exported; needs libgreet.so.1, libm.so.6, libgcc_s.so.1, libc.so.6, ...)
OK   build/libsub.a: archive: no libstdc++/libc++ symbol (10 C++ symbols)
FAIL build/ep-prefix/lib/libsub.a: archive: 1 symbols of libstdc++ or libc++, e.g. U std::__throw_out_of_range(char const*)
C    build/cprog: exe: no C++ code (needs libc.so.6)
```

`readelf -d app | grep NEEDED` (`otool -L` on macOS) is the quick look: no `libstdc++.so` or
`libc++` line.

### Versioning and ABI

libycxx is at version 0.1.0 (`find_package(libycxx 0.1)`, `libycxx.pc`'s `Version`). There is no
stable ABI: the layouts of library types and the archives' contents change between commits.
Rebuild everything (dependencies included) when libycxx changes, keep one installation per
compiler (GCC and Clang builds are separate prefixes), and do not mix objects compiled against
two libycxx versions. Since 2026-10-08 the standard entities are in the inline namespace
`std::__y1` (DECISIONS §20.4): every mangled name changed then, so objects built against an
earlier libycxx fail to link with "undefined reference" errors and must be rebuilt. Shipped
binaries are self-contained (static libycxx); they need only the C library, `libm` and
`libgcc_s`.

### CI recipes

GitHub Actions on Linux, with the compilers from the `gcc:16` container and apt.llvm.org as in
libycxx's own CI, or provisioned:

```yaml
- name: libycxx
  run: |
    git clone --depth 1 https://github.com/<you>/libycxx "$RUNNER_TEMP/libycxx"
    cd "$RUNNER_TEMP/libycxx"
    . tools/toolchain/activate.sh --provision     # finds GCC 16 / Clang 23, or installs them
    cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER="$YCXX_GCC" -DCMAKE_CXX_COMPILER="$YCXX_GXX" -DCMAKE_BUILD_TYPE=Release
    cmake --build build && cmake --install build --prefix "$RUNNER_TEMP/ycxx"
- name: build and test against libycxx
  run: |
    cmake -S . -B build-ycxx -G Ninja -DCMAKE_TOOLCHAIN_FILE="$RUNNER_TEMP/ycxx/lib/cmake/libycxx/toolchain.cmake"
    cmake --build build-ycxx && ctest --test-dir build-ycxx --output-on-failure
    "$RUNNER_TEMP/ycxx/bin/ycxx-check-binary" build-ycxx
```

Cache the libycxx prefix keyed on the libycxx commit and the compiler version, and the dependency
prefix keyed on that plus the dependencies' versions. Run `ycxx-check-binary` in CI: a
dependency that silently came from the system is the usual way a build stops being libycxx's.

## Common errors and their fixes

From the 19 real-world projects (`tests/realworld/*/patches`, each with its category and reason)
and the projects built for this guide:

| Error | Cause | Fix |
|---|---|---|
| `'abort' is not a member of 'std'`, `'errno' was not declared`, `'isspace' was not declared`, `'min' is not a member of 'std'`, `'ostream' in namespace 'std' does not name a type`, `'malloc' was not declared` | a transitive include of libstdc++/libc++ (`<cstdlib>` from `<string>`, `<cerrno>` from `<mutex>`, `<cctype>` from `<iostream>`, `<algorithm>` from `<vector>`, `<ostream>` from `<string>`) | include the header; without editing, `-include cstdlib` etc. in `CXXFLAGS` |
| `PATH_MAX` / `pthread_*` / `cpu_set_t` not declared | POSIX headers that libstdc++ includes from `<climits>`, `<thread>`, `<atomic>` | include `<limits.h>`, `<pthread.h>`, `<sched.h>` |
| `ciso646: No such file or directory` | removed in C++20; included to detect libc++ | include `<version>` |
| `'auto_ptr'`/`'random_shuffle'`/`'bind1st'` ... `is not a member of 'std'` | removed features | the replacements in [the table](#removed-and-deprecated-features) |
| `invalid use of incomplete type 'struct std::char_traits<unsigned char>'` | a libstdc++/libc++ extension (`basic_string<unsigned char>`, `<std::byte>`) | `std::vector<unsigned char>`, `std::basic_string<char>` with casts, or traits of your own |
| `... is deprecated` under `-Werror` | Annex D features (`std::iterator`, `is_pod`, `unsigned char` stream insertion) | fix, or `-Wno-error=deprecated-declarations` |
| a test of `std::optional` printing or formatting fails | `std::optional` is a range in C++26 (P3168): range formatting is chosen | adapt the expectation |
| `undefined reference to 'f(std::string ...)'` when linking a shared library's user, GCC only | GCC hides functions whose signature names a libycxx type | `[[gnu::visibility("default")]]` (or the project's export macro), a static library, or Clang |
| `undefined reference to 'g(std::__cxx11::basic_string...)'` or to `std::__throw_...`, `__cxa_...@CXXABI` | a dependency built against libstdc++ (a system package, an ExternalProject without the toolchain) | build it against libycxx; `ycxx-check-binary` names it |
| `libycxx requires C++26` | a compile that did not go through the wrapper or `ycxx::ycxx`'s flag order (an IDE, a hand-written rule with `-std=c++17` after `pkg-config --cflags`) | the wrapper, or put `-std=c++26` last |
| CMake: "requires the language dialect CXX26 ... CMake does not know the flags" | `CMAKE_CXX_STANDARD 26` with CMake < 3.30 | 23 or lower (raised), or CMake 3.30+ |
| a terminate or new handler set in the program is not called by a shared library | per-image runtime state | set it in the image that calls it |
| `catch (const std::exception&)` does not catch an exception from a library | the library was built against another standard library | catch `...`, or build it against libycxx |
| type names in output differ (`std::__cxx11::basic_string<char>` expected) | tests that compare the library's spelled type names | adapt the expectation |
| `GTEST_HAS_CXXABI_H_` / demangled names missing | GoogleTest recognises `<cxxabi.h>` by library | `-DGTEST_HAS_CXXABI_H_=1` |
| GCC: `<omp.h>` errors about `bits/new_throw.h` | GCC's `<omp.h>` includes libstdc++ | no OpenMP with GCC; Clang's `<omp.h>` |

## Verification

Run on Linux x86_64 (Ubuntu 24.04 userland, GCC 16.2.0, Clang 23.1.2, CMake 3.28.3, Ninja,
GNU make 4.3, Meson 1.12.1 through `uvx`, autoconf 2.71/automake 1.16, Conan 2.30.0, vcpkg
2026-09-26, ccache 4.9.1, clangd 22.1.1 through `uvx`), libycxx installed from its Release build
of each compiler.

Automated, both compilers (`tests/integration/run.sh gcc|clang <prefix>`, run by
`tests/cmake/run.sh`, so by `tools/test cmake` and CI): the installed files; `ycxx-check-binary`
rejecting a libstdc++ build; `tests/integration/cmake-project` (pinned C++17, `try_compile`
checks, shared library with C sources, FetchContent and ExternalProject dependencies,
`install(EXPORT)`, a `find_package` consumer) with the toolchain file alone under Ninja and Unix
Makefiles, and with `CC`/`CXX` in the environment; `make-project` with `CXX=ycxx-c++` and with
the plain compiler and `pkg-config`; a moved installation; `activate.sh --use`; `meson-project`
with the native file; `autotools-project` with `./configure CXX=ycxx-c++`. Every build is then
checked with `ycxx-check-binary`.

By hand (commands as in the sections above):

| Project | How | GCC | Clang |
|---|---|---|---|
| {fmt} 12.2.0, `FMT_TEST=ON` | installed toolchain file only | 21/21 tests pass; 26 binaries OK | 21/21 pass; 26 binaries OK |
| GoogleTest v1.18.0 installed to a prefix, then EnTT v4.0.0 (`ENTT_FIND_GTEST_PACKAGE=ON`, `GTest_DIR` found in the prefix) | toolchain file, `CMAKE_PREFIX_PATH` | 15/15 tests pass; 19 binaries OK | 15/15 pass; 19 binaries OK (after the toolchain file learned to name `clang-scan-deps`: EnTT is C++20 and was scanned) |
| toml++ v3.4.0, Meson tests | Meson native file, `CXXFLAGS="-include cstdlib"` | 9/9 test runs pass | 9/9 pass |
| {fmt} 12.2.0 from ConanCenter + `examples/package-managers/app` | Conan profile `ycxx` | builds, runs, `ycxx-check-binary` OK | not run |
| local overlay port + app | vcpkg triplet `x64-linux-ycxx` | builds, runs, OK | not run |
| `cmake/ycxx-toolchain.cmake` alone | (baseline) | program needs `libstdc++.so.6` | |
| `CMAKE_PROJECT_TOP_LEVEL_INCLUDES` injection | `tests/integration/cmake-project` | checks ran against libstdc++; ExternalProject link fails | |
| ccache, `target_precompile_headers` | toolchain file | second build all hits; PCH OK | PCH OK |
| clangd 22.1 `--check` | `ycxx::ycxx` database (GCC build) / wrapper database with the `.clangd` above (Clang build) | 0 errors | 0 errors |
| ASan+UBSan with the uninstrumented libycxx | `ycxx-c++ -fsanitize=address,undefined` | | heap overflow reported |
| `tests/integration/cmake-project` against libycxx built with `-DYCXX_SANITIZE=thread` | that installation's toolchain file, `-fsanitize=thread` | | runs clean; a racy program's race is reported |
| `-DYCXX_HARDENED=1` | `ycxx-c++` | | `vector::operator[]` out of range aborts (134) |
| Meson reading `CC`/`CXX` (no native file) | `meson-project` | | builds, `ycxx-check-binary` OK |

The 19 projects of `tools/realworld` (STATUS, "Real-world projects") are built the same way, with
the source tree's wrapper.
