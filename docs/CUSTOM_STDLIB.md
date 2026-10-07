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
   nothing of the library. Hide what the compiler keeps default (predeclared ABI entry points),
   including the default allocation functions; let the images that link the library share those
   through a table under a name of the library's own, so that one replacement (the program's, or
   a sanitizer's) serves every such image and objects can change owner between them, while
   another C++ runtime in the process keeps its own.
   **Discover target facts, do not write them down**: what the C library declares, which
   type_info objects the compiler emits, and the like are tested by the preprocessor where it can
   see them (`#ifndef`, `__has_include_next`) and otherwise probed against the real toolchain
   when the library is configured, the answers generated into the build tree.
4. **Select the library with the generic driver flags**: `-nostdinc++` plus `-isystem <headers>`,
   and `-nostdlib++` plus the archives and their system dependencies. Do not rely on `-stdlib=`.
5. **Deliver the same flags three ways**, generated from one place where possible: a CMake package
   (an interface target), a wrapper script for everything else, and a toolchain file or
   provisioning script that finds the supported compilers.
6. **Test against the specification, not the implementation**: an own suite written from the
   working draft, plus other implementations' suites run unmodified, never copied into the
   repository.
7. **Every failure fails the run, unless it carries its cause**: fix library bugs; skip a test
   that does not apply with a category and a reason; mark an expected failure, with its reason,
   only for a cause outside the test and the library (a compiler bug, an ABI limit, a draft
   defect, a feature not implemented yet). No anonymous list of known failures. An unexpected
   pass must fail the run.
8. **Run the suite in several configurations**: each compiler, each OS, sanitizers, and each
   library mode a user can select (hardening, no exceptions, no RTTI). Check the configurations
   that are not run.
9. **Validate the tests themselves** by running the own suite against another implementation, and
   triage every difference.
10. **Make every run explainable**: record the commands, exit statuses and output of passing tests
    as well as failing ones, and keep the reports as CI artifacts.
11. **Split CI**: a fast gate on every push, the whole external suites and the sanitizer runs
    nightly, each failing on every FAIL and XPASS.

## 3. Building

### Layout

libycxx (`DECISIONS.md` §3) puts the freestanding core in `include/ycxx/core/**` (no OS, no libc
headers, no heap unless an allocator is supplied), the hosted layer in `include/ycxx/hosted/**`
and `src/hosted`, and the platform layer behind `include/ycxx/pal.h` (C-linkage hooks such as
`ycxx_pal_allocate`, `ycxx_pal_wait`; `src/pal/posix`). The public headers have the standard names
(`include/vector`, ...). `tools/check_includes.py` enforces the layering and
`tools/check_freestanding.sh` compiles every core and freestanding header with
`-ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti` (plus the compiler's own header
directory, for its `<stddef.h>`) and links a smoke program for bare
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
an archive member, so a program can replace any subset (DECISIONS §3). CMake builds the
freestanding archive for the compiler's target with `-DYCXX_FREESTANDING_RUNTIME=ON` (off by
default), and installs and exports it as `ycxx::freestanding` (libycxx's headers and
`-ffreestanding`); `tools/check_freestanding.sh` (`build_fsrt`) builds the same sources for
bare-metal targets (Gaps, item 8: done).

libstdc++'s freestanding mode is a separate configuration of the whole library
[libstdcxx-configure]; libc++ has feature switches such as `LIBCXX_ENABLE_FILESYSTEM` and
`LIBCXX_ENABLE_EXCEPTIONS` [libcxx-vendor] and CMake caches such as
`Generic-no-localization.cmake` [libcxx-caches]. libycxx's split is by layer, so one build serves
both modes; the price is that the freestanding archive needs its own build path.

Between the two modes, libycxx's hosted library is split into **hosted layers** (DECISIONS §18):

- Each layer is a set of PAL primitives: memory, console, clock, threads, files, and the C library
  itself, among others.
- `-DYCXX_PAL=none` builds only what the selected layers enable, and the integrator's providers
  supply their primitives (`ycxx_add_hosted_layer`).
- A use of an absent layer fails at compile or link time, naming the layer.
- `examples/hosted-layers` runs containers, `std::print` and exceptions on a bare-metal x86_64
  kernel booted by Limine.

libc++'s closest equivalents are its feature switches (no filesystem, no localization, no
threads). Those remove features; they do not let the integrator supply them.

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
`namespace [[gnu::visibility("hidden")]] std {` in the headers and the runtime's sources alike
(enforced by `tools/check_visibility.py`), the runtime's C-linkage entry points carry
`[[gnu::visibility("hidden")]]` on their declarations (no `-fvisibility=hidden`: the sources say
what is hidden, whoever builds them), and what the compilers keep default is hidden with
assembler directives: GCC's predeclared `__cxa_*` entry points, and GCC's fundamental type_info
objects. Which fundamental type_info objects a compiler emits depends on the target (GCC 16.2 for
aarch64-apple-darwin emits 300 symbols, with the SVE, `__bf16` and `__mfp8` types; 150 of them are
not in Apple's libc++abi), so the list is not written down: CMake compiles a probe defining
`__fundamental_type_info`'s key function at configure time and lists its symbols with `nm`
(DECISIONS §2). The default allocation functions are hidden as well, and shared among the images
that link libycxx through an allocation table (Replacement allocation functions, below).

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
libycxx hides its defaults with assembler directives.

### Facts about the target

A library that forwards to the C library, or hides what a compiler emits, depends on facts about
the target. libycxx does not encode them as knowledge about a platform (DECISIONS §1, rule 8):

- What the preprocessor can see is tested where it is used: `#ifndef _PRINTF_NAN_LEN_MAX`,
  `!defined(PRIb8)`, `__has_include_next(<uchar.h>)` (the `_next` form skips the library's own
  `<uchar.h>`).
- The rest is probed when the library is configured, against the real toolchain, on every target:
  `cmake/ycxx-c-library.cmake` compiles C probes against the C library (does `<stdlib.h>` declare
  `strfromd`, `<uchar.h>` `mbrtoc8`, `<time.h>` `timespec_getres`; and, by running a probe, the
  longest NaN its `printf` writes) and writes `<ycxx/generated/c_library.hpp>` into the build tree,
  which `ycxx/config.hpp` includes when it is found (`__has_include`; without it a C23 C library is
  assumed, so a gap is a compile error naming the function). The fundamental type_info probe
  (Visibility, above) writes the symbols the ABI runtime hides.
- Where the C library's declaration is wrong for C++ ([cstring.syn]'s const/non-const `strchr`
  pairs, [support.start.term]'s `noexcept` `atexit`), the wrapper renames the C library's
  declaration while reading its header and declares the C++ one itself, on every C library, so
  nothing depends on which C library it is (DECISIONS §3).

Checked on macOS 26 (arm64, GCC 16.2 and Clang 23.1): the probes find no `strfrom*`, `mbrtoc8`,
`timespec_getres` or `<uchar.h>`, and measure `_PRINTF_NAN_LEN_MAX` as 3 (every NaN prints as
`nan`).

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

An installation has the same wrapper as `<prefix>/bin/ycxx-c++` (generated from
`cmake/ycxx-c++.in` for the compiler libycxx was built with, relocatable, raising an older
`-std=` to C++26), with a toolchain file naming it, `libycxx.pc` and a Meson native file:
`docs/BUILDING_PROJECTS.md` covers using them with existing projects.

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

On Linux the package passes `--gcc-install-dir=<GCC 16's installation>` to Clang consumers, as
`tools/ycxx-cxx` and the toolchain file do; without it, Clang links the crt files and `libgcc` of
whatever GCC installation it finds (here GCC 13's). The directory is the cache variable
`YCXX_GCC_INSTALL_DIR`, found when libycxx is configured (`$YCXX_GCC_INSTALL_DIR`, a
`--gcc-install-dir` already in `CMAKE_CXX_FLAGS`, else the libgcc directory of the GCC building
libycxx, of `$YCXX_GXX` or of `g++-16`), recorded in `libycxxConfig.cmake`, and overridable by the
consumer's `YCXX_GCC_INSTALL_DIR`. `tests/cmake/run.sh` checks in the link map that both example
programs load GCC 16's `crtbeginS.o` (Gaps, item 10: done).

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
which the runtime clears (DECISIONS §4, STATUS "macOS"). Checked on macOS 26 arm64 with Homebrew
GCC 16.2 and Clang 23.1 (`tools/test policy build cmake ycxx`): the platform-specific findings were
GCC's target-dependent fundamental type_info objects (above), C library gaps (above; also no `%b`
in `printf`, a C library gap the library cannot fill), `$TMPDIR` behind a symbolic link
(`/var` -> `/private/var`: a path under `PATH_MAX` can still fail with `ENAMETOOLONG` once resolved),
and a Clang code generation bug on Mach-O: the TLS initialization function of an
`inline thread_local` variable with hidden visibility is emitted as a strong symbol, so two
translation units defining the variable fail to link (STATUS, with a repro without the library).

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
error categories and default memory resources (but one set of allocation functions, see below). `tests/cmake/run.sh`
checks it: a program and a shared library built with libycxx, each in one process with a shared
library built with the toolchain's library ("mine 3 other 3"), a program catching a libycxx shared
library's exceptions ("caught 15 uncaught 0 0"), and no exported libycxx symbol in any image other
than the allocation table (all passed with both compilers, on Linux and on macOS 26).

Compared with the alternatives: an ABI namespace alone keeps names apart but still exports them
(and their weak definitions coalesce within one library's own copies); `-Bsymbolic` and version
scripts protect only images whose link line remembers them; Android's rule forbids the case.
Hiding at the declaration, as libycxx does, protects every image by default. Its costs are real
and documented: GCC's `-Wattributes` warning, nothing shareable by plugins, and per-image
singletons.

### Replacement allocation functions and sanitizers

The global allocation functions are the one part of the library an image must share with the
others: a `std::string` built in a shared library and destroyed in the program is allocated by
one image's `operator new` and freed by the other's `operator delete`. With per-image (hidden)
defaults that pairs only while both reach `malloc`/`free`; it breaks when the program replaces
`operator delete`, and AddressSanitizer reports it as `alloc-dealloc-mismatch`. The usual answer
is to export the defaults, as libstdc++ and libc++ do, and as Chromium keeps them on ELF so that its
allocator sees every image's allocations [chromium-libcxx-gn]; libycxx did so briefly.

Exporting the defaults has a cost where another C++ runtime shares the process. On Darwin, dyld
takes each allocation function from the first image in load order that defines it (observed with
`DYLD_PRINT_BINDINGS` on macOS 26), coalescing with libc++abi's by name: a libycxx program's default
then served Apple's libc++, whose allocation failure threw libycxx's `bad_alloc` into code built
with libc++abi; and a libycxx shared library in a host without libycxx (a C program, a plugin host
built with Apple's libc++) got libc++abi's `operator new`, with libc++abi's `bad_alloc` (foreign to
libycxx's runtime) and libycxx's `new_handler` never called. ELF interposition does the same when
libycxx and libstdc++ meet. libycxx therefore keeps its defaults hidden and lets its own images
share them through `ycxx_allocation_functions`, a weak, exported table of function pointers under a
name no other runtime defines: the dynamic linker binds every image to the first image's table
(the program's, kept there by link options the package adds), whose entries call that image's
`operator new` and friends, the program's replacements included; each default forwards to the
process's entry when it is another image's (DECISIONS §2). Checked on macOS 26 with both
compilers: a program's replacement serves its libycxx shared libraries
(`linkage/shared_library_replaced_new`), objects cross images under AddressSanitizer
(`linkage/shared_library_allocation_exchange`), and a libycxx shared library and one built with
Apple's libc++ each keep their own allocation functions, `new_handler` and `bad_alloc`, in a
libycxx program and in a program built with Apple's libc++ (`tests/cmake/visibility`, "mine 7
other 7"; the exported defaults gave "other 3"). A program's own replacement is exported and serves
the other runtime too, the platform's ordinary rule.


The library's default `operator new`/`delete` live in their own archive members, so a program's
replacement is linked instead ([replacement.functions]; DECISIONS §3). A sanitizer runtime is a
second replacement: AddressSanitizer's run-time library "replaces the malloc and free functions"
[asan-algorithm] and checks allocation/deallocation pairing (`alloc_dealloc_mismatch`,
`new_delete_type_mismatch`) [asan-flags]. Checked here: in a program linked with
`tools/ycxx-cxx clang -fsanitize=address`, the linker map shows `operator new(unsigned long)` taken
from `libclang_rt.asan_cxx-x86_64.a(asan_new_delete.cpp.o)`, not from `libycxx.a`; the program
runs. libycxx's own suite marks the tests this changes (`new/*` forwarding and new_handler tests,
`linkage/*` export and replacement tests) `// UNSUPPORTED-SANITIZER: asan` with the reason; the
tests that exchange objects between images (`linkage/shared_library_exceptions`,
`linkage/shared_library_allocation_exchange`) run under ASan, clean on macOS 26 with both compilers. ThreadSanitizer's
runtimes define the allocation functions too, Clang's in an archive linked whole and GCC's in the
shared `libtsan.so`, linked ahead of every input; libycxx keeps its own there (Clang:
`-fno-sanitize-link-c++-runtime`; both: weak defaults, each linked through an anchor that a TSan
build's link options name as undefined, so that the program's definitions win over the shared
runtime's; DECISIONS §6.8), and the `new/*` tests run under TSan. A GCC configured with
`--disable-libsanitizer` has no sanitizer runtimes (`cannot find -ltsan`);
`tools/toolchain/provision --with-sanitizers` builds GCC 16.2 with them.

The rule that follows: test allocation-function behaviour without sanitizers, and mark, not
delete, the tests a sanitizer runtime invalidates. The MSVC STL does the same in its expected
results, under an "ASAN FAILURES" section [msvc-expected].

## 6. Testing

### The own suite, derived from the specification

libycxx's own suite (`tests/ycxx`, 2381 tests as lit counts them at this commit) is written by an author who may
read only the working draft and cppreference.com, never another implementation's tests or any
implementation; a failing test is a library bug until the draft shows otherwise, and tests are
never weakened (DECISIONS §6). The lit format (`tests/ycxxlit/ycxx_format.py`) knows
`*.pass.cpp` (compile, link, run), `*.compile.pass.cpp` (must compile) and `*.compile.fail.cpp`
(must not compile, for a reason other than a missing header), with directives `FLAGS`, `FILES`,
`ARCHIVE`, `SHARED` (multi-image tests), `UNSUPPORTED-SANITIZER`,
`XFAIL: gcc|clang|any[-linux|-darwin] <reason>` (`XFAIL-COMPILER` accepted; the OS suffix limits the
mark to that OS, for a compiler bug of one object format),
`EXPECT-ERROR[-GCC|-CLANG]: <regex>` (a compile-fail test's diagnostics must match),
`REQUIRES: <features>` (lit features `gcc`, `clang`, `linux`, `darwin`, `asan`, `ubsan`, `tsan`,
`hardened`, `exceptions`, `rtti`) and `EXPECT-TERMINATE` (a death test: killed by SIGABRT,
SIGTRAP or SIGILL). Run it as:

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

What libycxx's own suite lacked against these, now added: diagnostic matching
(`EXPECT-ERROR`, Gaps item 2; a `.compile.fail.cpp` without it still passes on any error that is
not a missing header, so only the tests that state their diagnostic, 13 so far, are protected
against passing for the wrong reason), and feature-based constraints (`REQUIRES:`, Gaps item 4,
in part: there is no `UNSUPPORTED:`/`XFAIL:` on features yet, and the older `XFAIL-COMPILER` and
`UNSUPPORTED-SANITIZER` directives remain).

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
tools/test -c gcc -f numerics/bit -f numerics/numbers libcxx
#   20 passed, 1 unsupported, 1 failed (numerics/numbers/value.pass.cpp) -> exit 1
```

The failure fails the run: `value.pass` compares `e_v<long double>` with the `double` value in a
constant expression (STATUS, numerics), and until it is fixed, skipped or marked expected with its
reason, nothing makes that run pass.

### Triage and known failures

In libycxx a test reported FAIL fails the run and CI; there is no list of known failures. A
failure has one of three causes, each handled where it is decided, with its reason:

| Cause | Where | Effect |
|---|---|---|
| A libycxx bug | fixed; STATUS lists what is open | FAIL until fixed, and CI is red |
| The test does not apply (removed feature, divergence from the draft, extension, implementation-specific, infrastructure) | `tests/<suite>/skip.txt`, `tests/common/skip.txt`, a line `<test regex> \| <category> \| <reason>`; categories in `tests/SKIPPED.md` | UNSUPPORTED with the reason, and the libycxx test that covers the subject, if any |
| A cause outside the test and the library (a compiler bug, an ABI limit, a draft defect, a feature not implemented yet) | `// XFAIL: gcc\|clang\|any <reason>` in the own test (`XFAIL-COMPILER` still accepted); `tests/<suite>/xfail.txt`, `<test regex> \| <compiler> \| <reason>`, for external suites | XFAIL with the reason; XPASS fails the run, so the mark goes when the cause does |
| Reference-run difference | `tests/ycxx/REFERENCE.md` | documentation only |

`TRIAGE.md` (categories A-F) records the classification of a full external run; each classified
failure must end in one of the rows above, and until it does it fails the run.
`tools/triage.py` groups failures by missing header or first error message.

Why not a baseline, a generated list of the tests that failed last time (which libycxx had until
commit `49a167c`): such a list says that a test fails, not why. A regression in a listed test
stays hidden among the "known failures"; CI is green while tests fail, so the red state that
should prompt a fix never comes; and the cause lives elsewhere (here `TRIAGE.md`), drifts from the
list, and is lost when the list is regenerated. With the rule above every non-passing result
names its cause in the place that decides it, a bug cannot be recorded as expected, and an
expected failure whose cause is gone shows up as XPASS.

Others: the MSVC STL keeps one hand-maintained `expected_results.txt` per suite, lines
`<test>[:<configuration>] FAIL|SKIPPED`, grouped by cause (issues reported upstream, slow tests,
ASan failures, missing STL features, CRT bugs, likely bogus tests, likely STL bugs, not yet
analyzed) [msvc-expected]; an XPASS requires updating it [msvc-readme]. lit has `--xfail` /
`LIT_XFAIL` for marking tests as expected failures without editing them [lit]. libstdc++ expresses
expected failures in the test (`xfail` selectors, `dg-xfail-run-if`) [libstdcxx-test].

libycxx's rule is closest to libc++'s and libstdc++'s: the reason sits with the test (in the own
test, or one line per external test with its reason, since external tests are never edited). The
MSVC file is a list, but keeps a cause per entry, grouped by cause, which is what an anonymous
baseline loses; libycxx goes further in two ways: a library bug cannot be listed at all (it is
fixed, and fails CI until then), and every skip and expected failure states its reason in the
run's output, not only in the file.

At this commit CI fails where tests fail: the own suite's expected failures are each marked in the
test with the STATUS cause (`char_traits/eof`, draft defect; `except/handler_pointer_reference*`,
Itanium ABI; two `except/handler_*` and `exception/exception_ptr_constexpr` and
`contracts/observe` on GCC 16; `execution/senders_basic`, not implemented yet), and three tests
still FAIL as libycxx divergences being fixed (`cstddef/stddef_global{,_reverse}`,
`cwchar/mbstate_global`; `cwchar/wchar_h_global_names` now passes on both compilers with the
library's own `<wchar.h>`). The full external runs fail
until every failure of `TRIAGE.md` is fixed, skipped or marked (Gaps, item 1).

### Sanitizers

`tools/test -s asan,ubsan` (or `SANITIZER=asan,ubsan tools/run-conformance ...`; also `tsan`) adds
`-fsanitize=... -fno-sanitize-recover=all -g` to the tests and links them with libycxx built with
the same sanitizers, `build/<cc>-<sanitizers>`, which `tools/run-conformance` configures
(`-DYCXX_SANITIZE=...`) and builds itself: a sanitizer sees only instrumented code, and with an
uninstrumented library ThreadSanitizer reported every hand-off through libycxx's own futex
mutexes, thread-pool queue and reference counts (DECISIONS §6.8, which also covers the sanitizer
runtimes' allocation functions and static-local guards). Lit features `asan`/`ubsan`/`tsan` are
set for the libc++ suite (and `tsan` can name a libstdc++ test in its `unsupported.txt`).
Every program of a TSan run gets `TSAN_OPTIONS` on its command line: the suite's
`tests/<suite>/tsan.supp`, false positives each with its reason, and
`allocator_may_return_null=1`; a test program's time limit is three times the plain one
(`tests/ycxxlit/sanitizers.py`). Nightly CI runs the own suite with Clang under ASan+UBSan, and
all three suites under TSan with both compilers in a job of its own on the bare runner (GCC 16.2
provisioned with libsanitizer and cached), where every failure fails the job (a test that cannot
run under a sanitizer says so in the test or in the suite's lists). Checked: `tools/test -c clang -s asan,ubsan -f optional ycxx`,
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
`-fno-exceptions` and `-fno-rtti` support (DECISIONS §1, §4). The own suite runs in those
configurations through lit parameters: `hardened=1` (`tools/test --hardened`, `YCXX_HARDENED=1`)
compiles every test with `-DYCXX_HARDENED=1` and enables the death tests of
`tests/ycxx/precondition` (one per hardened precondition, `REQUIRES: hardened` and
`EXPECT-TERMINATE`); `cxxflags=` with a configuration name (`tools/test --cxxflags=...
--config-name=...`, `YCXX_CXXFLAGS`/`YCXX_CONFIG_NAME`) appends flags to every test, and
`-fno-exceptions`/`-fno-rtti` there remove the `exceptions`/`rtti` features, which the 430 tests
that throw or catch require. Each configuration has its own exec root, logs and reports
(`ycxx-<cc>-hardened`, `ycxx-<cc>-<name>`), and fails on every FAIL like the default run. Nightly CI runs hardened, `-fno-exceptions` and `-O2`
on both compilers, and all three suites under TSan (Gaps, items 3 and 6: done, except `-fno-rtti`).

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

### Real-world projects

A conformance suite checks what the standard says; real projects check what programs do. libycxx
builds open-source projects with their own test suites against itself (`tools/realworld`, the
`realworld` stage of `tools/test`; nightly in `full.yml`). Each project is a manifest,
`tests/realworld/<name>/manifest`: the git URL and the pinned commit (with its release tag), the
CMake options that build its tests, its dependencies (other projects, built and installed first
with libycxx: a test framework is never the system's), repositories its build would download, and
the CMake version it needs. Beside it: `patches/` (each patch starts with its category and
reason; patched is only the project's own non-standard code, never something to hide a libycxx
bug), `skip.txt` (CTest tests that do not apply, `<regex> | <category> | <reason> [| <conditions>]`,
with conditions such as `clang asan` or `gcc !tsan`; reported UNSUPPORTED), `xfail.txt` (expected failures; an XPASS fails the run) and
`build-skip.txt` (build outputs that cannot be built, with the same fields). An entry that matches
nothing is reported as a failure, so the lists cannot go stale.

How a project is built against libycxx: a generated toolchain file names a two-line C++ compiler,
`build/realworld/<config>/bin/c++`, that runs `tools/ycxx-cxx <cc> --libdir=<build>`. That is
robust where flags in `CMAKE_CXX_FLAGS` are not: a project that resets its flags, its
`try_compile` checks, its nested CMake projects and its FetchContent dependencies all get libycxx's
include directory, `-nostdinc++`, `-nostdlib++` and archives. libycxx is C++26 only, so a
project's `-std=c++NN` (`gnu++NN`) becomes `-std=c++26` (`gnu++26`); projects are built with
`-O1 -g1` and assertions on.

The harness then proves that the build is libycxx's, for every project and configuration, and
fails the project otherwise (`tools/lib/realworld.py`, LINKAGE):

- every C++ translation unit in `compile_commands.json` has a record of the command
  `tools/ycxx-cxx` ran for its object (`$YCXX_CXX_LOG`), with `-nostdinc++`,
  `-isystem <libycxx>/include` and an effective `-std=c++26`, and no `-stdlib=` or include
  directory of libstdc++ or libc++;
- the headers the compiler reported for every object (`ninja -t deps`) include none of
  libstdc++'s or libc++'s (`.../include/c++/...`, `.../c++/v1/...`);
- no executable or shared library needs `libstdc++`/`libc++` (`readelf -d`, `otool -L`) or
  holds one of their symbols, defined or undefined (`std::__cxx11`, `__gnu_cxx::`, any
  `std::__` name, which libycxx never uses, `std::__1`, `GLIBCXX_`/`CXXABI_` versions); static
  archives are checked for the symbols;
- every image linked by the C++ driver defines `__ycxx_allocation_functions`, the exported
  allocation table that only libycxx's runtime defines (DECISIONS §2) and that `tools/ycxx-cxx`'s
  link options keep in every image;
- every CTest test runs one of the checked executables (or names the tool or script it runs).

A self-test runs first: `tests/realworld/selftest`, built with the toolchain's own C++ library,
must be rejected on all five counts, and the same program built against libycxx accepted.

`tools/realworld [-c gcc|clang] [-s asan|tsan|...] [project...]` writes
`build/test-logs/realworld-<config>.{html,md,tsv}` in the suites' report format (every project's
build with its steps and logs, its linkage evidence, every test with its output) and
`.summary.md`, one line per project; STATUS.md ("Real-world projects") records the results.
Sanitizer runs link the instrumented libycxx (`build/<cc>-<sanitizers>`), as the suites do.

What the projects found is mostly not about the standard: code that relies on the headers
libstdc++ and libc++ include from one another (`errno` after `<string>` or `<mutex>`,
`std::abort` after `<memory>`, `std::ostream` after `<string>`, `isspace` after `<iostream>`),
on their extensions (`std::char_traits<std::byte>`), or on older language rules than C++26
(`std::optional` became a range). STATUS.md lists them, with the libycxx bugs found.

Others: libc++ and libstdc++ rely on the distributions that build thousands of packages with them;
Chromium builds itself with its own libc++ [chromium-cxx-gni]. A replacement library has no such
users yet, so it has to bring the projects itself.

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
| Every push / PR | `ci.yml`: `tools/test policy build freestanding cmake ycxx`, failing on every FAIL and XPASS, on Linux (gcc:16 container, Clang 23 from apt.llvm.org) and macOS 15 arm64; a sample of both external suites on Linux | libc++: CI configurations defined in `libcxx/utils/ci/Dockerfile` and run by `libcxx/utils/ci/run-buildbot`, reproducible locally with `run-buildbot-container` [libcxx-testing] |
| Nightly / on demand | `full.yml`: both external suites, both compilers, Linux and macOS (one job per suite and compiler, up to 300-340 minutes), the own suite with ASan+UBSan, the own suite hardened, with `-fno-exceptions` and with `-O2` (both compilers, Linux), the real-world projects (`tools/test realworld`, both compilers, Linux), and the benchmarks against their baseline of ratios to libstdc++ (`bench/check`, both compilers, Linux); no job tolerates a FAIL | libc++: continuous fuzzing on OSS-Fuzz (`libcxx/utils/ci/oss-fuzz.sh`) [libcxx-oss-fuzz] |

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
| Freestanding | core layer, one build; separate runtime archive (CMake option, installed) | feature switches [libcxx-vendor] | `--disable-hosted-libstdcxx` [libstdcxx-configure] | not confirmed |
| ABI runtime | own (Itanium), unwinder from the toolchain | selectable: libc++abi, libcxxrt, libsupc++, ... [libcxx-vendor] | libsupc++ | not confirmed |
| Artifacts | static archives, PIC | shared and static [libcxx-vendor] | shared, versioned [libstdcxx-abi]; static not confirmed | not confirmed |
| Symbol policy | everything hidden, per-image runtime | exported ABI with visibility macros, inline ABI namespace; hermetic static option [libcxx-visibility, libcxx-vendor] | version script, `check-abi` baseline [libstdcxx-abi] | not confirmed |
| Selection | `-nostdinc++ -isystem`, `-nostdlib++`; wrapper; CMake package; toolchain file | `-stdlib=libc++` or the same generic flags [libcxx-user, libcxx-vendor] | default for GCC | `INCLUDE`/`LIB` via `set_environment.bat` [msvc-readme] |
| Own tests | spec-only author; `.pass`/`.compile.pass`/`.compile.fail`, diagnostic regexes, death tests | rich kinds incl. `.verify.cpp`, `.sh.cpp`, `.gen.cpp` [libcxx-testing] | DejaGnu `dg-*` with message matching [libstdcxx-test] | `tests/std`, `tests/tr1` [msvc-readme] |
| External suites | libc++'s and libstdc++'s, run only, fetched and pinned | can run against libstdc++ [libcxx-stdlib-libstdcxx-cfg] | not confirmed | libc++'s, from its llvm-project checkout [msvc-readme] |
| Known failures | none: FAIL fails CI; skip lists with category and reason, `XFAIL` with reason in own tests, `xfail.txt` with reason for external suites | `XFAIL`/`UNSUPPORTED` in tests [libcxx-testing] | `xfail` selectors in tests [libstdcxx-test] | `expected_results.txt` by cause, per configuration [msvc-expected] |
| Configurations | 2 compilers x 2 OSes; ASan+UBSan (Clang); hardened, `-fno-exceptions`, `-O2`; TSan on all suites, both compilers (Linux, nightly) | hardening modes, no-exceptions, no-rtti, sanitizers, std modes, modules [libcxx-caches, libcxx-params] | `-std` list, debug mode, board flags [libstdcxx-test] | matrix files [msvc-matrix] |
| Reference runs | own suite against libstdc++ | suite against libstdc++ [libcxx-stdlib-libstdcxx-cfg] | not confirmed | not confirmed |
| Reports | per-test transcripts, HTML/Markdown, provenance | lit output | `.sum`/`.log` [libstdcxx-test] | lit output [msvc-readme] |
| Fuzzing | none | OSS-Fuzz [libcxx-oss-fuzz] | not confirmed | not confirmed |

**What libycxx does better.** Hermetic by default (no opt-in, no consumer link flags, tested with a
second runtime in the process); one generated set of flags behind three delivery paths, with
compiler provisioning; a clean-room own suite written from the draft alone, validated by reference
runs; external suites never vendored, pinned and fetched; no list of known failures: every FAIL
fails CI and every skip or expected failure carries its reason into the output, with XPASS
failing the run; reports that show the evidence for passing tests.

**What others do better.** Configuration coverage (no-rtti, MSan as standing configurations;
hardening has one mode, not libc++'s four); diagnostic matching as a test kind of its own (libycxx
matches only where a test states a regex); feature-based test constraints beyond `REQUIRES`;
sharding;
fuzzing; container annotations for ASan; documented raw flags for consumers. libc++'s
`XFAIL`/`UNSUPPORTED` take boolean feature expressions, where libycxx's take a compiler name.

## 8. Gaps and proposals

Ranked by value for effort. Items 2, 3, 6 (in part), 8 and 10 are now done, and 4 in part; each
says what was done.

1. **Every external failure needs a reason.** The full runs of `full.yml` fail on every FAIL, and
   the last full libc++ run (`tests/libcxx/TRIAGE.md`) left about 240 failures per compiler,
   classified but not resolved (libstdc++'s in `tests/libstdcxx/TRIAGE.md`). Work through them: fix the A (library bug) entries; skip the
   tests that do not apply with their category and reason (`skip.txt`, naming the libycxx test that
   covers the subject); mark the compiler, ABI, draft-defect and not-yet-implemented ones in
   `xfail.txt` with their reason. Then a red nightly means a new failure, not an old one. Never
   restore a list of failures without causes to get there sooner.
2. **A diagnostic-matching test kind.** Add `// EXPECT-ERROR: <regex>` (one or more) to
   `*.compile.fail.cpp`: the test passes only if every pattern matches an error line, on both
   compilers; optionally map libc++'s `expected-error` and libstdc++'s `dg-error "msg"` to it where
   the message is the standard's (a `static_assert` text the library controls). Today a
   compile-fail test passes on any error but a missing header. libc++'s `.verify.cpp` uses clang
   `-verify` [libcxx-testing], which GCC lacks, hence a regex of one's own.
   **Done:** `// EXPECT-ERROR: <regex>` (repeatable), `EXPECT-ERROR-GCC:`/`EXPECT-ERROR-CLANG:` for
   one compiler's wording, matched against the compiler's output (`tests/ycxxlit/ycxx_format.py`);
   a test that fails without a match names the regexes. 13 tests use it (Mandates
   `static_assert`s, deleted functions, constraint failures). Not done: mapping the external
   suites' `expected-error`/`dg-error` messages to it.
3. **A hardened configuration.** Run the own suite with `-DYCXX_HARDENED=1` nightly, and add death
   tests for precondition violations (a `.pass.cpp` that forks or expects an abnormal exit, under a
   `hardened` feature), as libc++'s `assert.*.pass.cpp` [libcxx-testing]. Today the hardened code
   paths are compiled by no test run.
   **Done:** `tools/test --hardened` (lit param `hardened=1`, `YCXX_HARDENED=1`), run nightly on
   both compilers; `tests/ycxx/precondition` has 53 death tests (`REQUIRES: hardened`,
   `EXPECT-TERMINATE: about to violate`), one per hardened precondition of the sequence
   containers, `basic_string`, `string_view`, `span`, `optional` (and `optional<T&>`), `expected`,
   `mdspan`, `bitset`, `valarray`, `view_interface` and `shared_ptr<T[]>`. With the checks compiled
   out (`--cxxflags=-UYCXX_HARDENED`) all 53 fail, three of them by SIGSEGV, which
   `EXPECT-TERMINATE` rejects. The first hardened runs of the whole suite found one failure beyond
   the default run's: a precondition check inside `submdspan` (STATUS, Own-suite configurations).
4. **Lit features instead of bespoke directives.** Set `available_features` in
   `tests/ycxx/lit.cfg.py` (`gcc`, `clang`, `linux`, `darwin`, `asan`, `ubsan`, `tsan`, `hardened`,
   `no-exceptions`, `no-rtti`) and let tests use lit's `REQUIRES:`/`UNSUPPORTED:`/`XFAIL:` with
   boolean expressions [lit]; keep `XFAIL-COMPILER` as an alias. Item 3 and 6 then need no new
   directive kinds.
   **In part:** the features are set (`config.available_features`) and `// REQUIRES:` evaluates
   them with lit's own boolean-expression parser (a comma means `&&`). Not yet: `UNSUPPORTED:`
   and `XFAIL:` on features, replacing `XFAIL-COMPILER`/`UNSUPPORTED-SANITIZER`.
5. **Shard the nightly suites.** Split each `full.yml` suite job with lit's
   `--num-shards`/`--run-shard` [lit] across matrix entries and merge the `.tsv` files for the
   report; record test times (`--time-tests`) to order slow tests first. The jobs now take
   up to 300-340 minutes.
6. **A configuration parameter and a nightly matrix.** Add a lit parameter (and `tools/test`
   option) for extra compile flags, then run nightly: `-fno-exceptions`, `-fno-rtti`, hardened, `-O0`
   and `-O2`, and TSan with `build/clang-tsan`, each failing on every FAIL (a test that cannot run
   in a configuration says so with `REQUIRES:` or `XFAIL:` and its reason). Models: libc++'s CMake caches [libcxx-caches],
   libstdc++'s board flags [libstdcxx-test], the MSVC STL's matrix files [msvc-matrix].
   **Done, in part:** lit param `cxxflags=` with `config=<name>` (`tools/test --cxxflags=...
   --config-name=...`, `YCXX_CXXFLAGS`/`YCXX_CONFIG_NAME`; the name defaults to one made from the
   flags), and a nightly `linux-configurations` job matrix in `full.yml`: hardened,
   `-fno-exceptions` and `-O2` on both compilers, each failing on every FAIL, and the own suite
   under TSan (Clang, with libycxx instrumented: `build/clang-tsan`). No `-O0`
   job: the default own-suite run is already unoptimized. Not yet: `-fno-rtti` (tests that use
   `typeid`/`dynamic_cast` would need `REQUIRES: rtti`).
7. **Fuzz targets that are also tests.** Write libFuzzer entry points for `<regex>`, format strings,
   `from_chars`, `<chrono>` parsing, the demangler and TZif parsing as `.pass.cpp` files that replay
   a small corpus in the normal suite and build as fuzzers with `-fsanitize=fuzzer` under a
   parameter, as libc++ does for OSS-Fuzz [libcxx-oss-fuzz]. Run them for a fixed time nightly.
8. **The freestanding runtime archive as a CMake target.** Build `libycxx-freestanding.a` in
   `CMakeLists.txt` (`ycxx::freestanding`), install and export it, and let
   `tools/check_freestanding.sh` use it. Today only the script builds it, so a freestanding consumer
   has no supported way to get it.
   **Done:** `-DYCXX_FREESTANDING_RUNTIME=ON` builds, installs and exports it as
   `ycxx::freestanding`; `tests/cmake/run.sh` links the freestanding smoke program with the
   installed archive and no C library. The script keeps its own build, since it targets bare-metal
   triples the CMake build is not configured for.
9. **ASan container annotations** for `vector`, `deque` and `basic_string` (STATUS, Known
   limitations; 16 libc++ tests fail under ASan only for that reason), honoured by ASan's
   `detect_container_overflow` [asan-flags].
10. **One flag set for Clang everywhere.** Pass `--gcc-install-dir` (or record it in the package
    from the build) for Clang consumers of the CMake package, as `tools/ycxx-cxx` and the toolchain
    file do, or document that the package uses Clang's default GCC installation for crt files and
    libgcc.
    **Done:** recorded in the package from the build (`YCXX_GCC_INSTALL_DIR`; section 4, CMake
    package), checked by `tests/cmake/run.sh` in the examples' link maps.
11. **Document the raw flags for other build systems** (Meson, Bazel, plain Make): the table of
    section 4 and the exact link order, as libc++ documents its custom-installation command
    [libcxx-vendor]. Today README points to the wrapper and the package only.
12. **xUnit output in CI** (`--xunit-xml-output` [lit]) so that GitHub's test summaries show
    per-test results next to the HTML reports.
13. **`-stdlib++-isystem` for Clang: not recommended.** Clang drops it under `-nostdinc++` (checked),
    GCC rejects it, and `-nostdinc++ -isystem` already places libycxx's directory first on both
    compilers. It would help only if libycxx had to coexist with a `-stdlib=` choice, which it does
    not.
14. **Symbol version scripts: not applicable** while libycxx ships no shared library and exports
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
| External suite (later, after the baselines were removed) | `tools/test -c gcc -f numerics/bit -f numerics/numbers build libcxx` | 20 passed, 1 unsupported, 1 failed (`numerics/numbers/value.pass.cpp`: `e_v<long double>` compared with the `double` value in a constant expression, STATUS), exit 1 |
| Freestanding | `tools/check_freestanding.sh` | 73 headers compile and the smoke program links: clang x86_64, clang riscv64, gcc x86_64 |
| Driver flags | `-E -v`, `-###` with and without `-nostdinc++`, `-nostdlib++`, `-stdlib++-isystem`, `-stdlib=`, `-fvisibility-global-new-delete=` on `g++-16` and `clang++-23` | as in the table of section 4 |
| ASan allocation functions | `tools/ycxx-cxx clang -fsanitize=address ... -Wl,-Map,...` | `operator new` from `libclang_rt.asan_cxx-x86_64.a(asan_new_delete.cpp.o)`; GCC: `cannot find -lasan` |
| GCC `-Wattributes` | `g++-16 -nostdinc++ -isystem include -c` on `struct S { std::string s; };` | the warning; Clang: none |
| Diagnostic matching (later) | `tools/run-conformance ycxx gcc|clang <the 13 tests with EXPECT-ERROR>`; one with a wrong regex added | 13/13 pass on both compilers; the wrong regex fails with "no match for: EXPECT-ERROR: ..." |
| Hardened configuration (later) | `tools/test --hardened -c gcc|clang ycxx` (as `YCXX_HARDENED=1 tools/run-conformance ...`) | GCC 2363 pass / 13 fail, Clang 2355 / 10: all 53 `precondition/` death tests pass; one failure beyond the default run's (`mdspan/submdspan_exhaustive_oracle`, STATUS) |
| Death tests without the checks (later) | `YCXX_HARDENED=1 YCXX_CXXFLAGS=-UYCXX_HARDENED ... clang precondition` | 53/53 fail: 50 exit 0, 3 SIGSEGV |
| `-fno-exceptions` (later) | `YCXX_CXXFLAGS=-fno-exceptions YCXX_CONFIG_NAME=noexcept tools/run-conformance ycxx gcc|clang` | 483 UNSUPPORTED (430 `REQUIRES: exceptions`, 53 `REQUIRES: hardened`); failures only those of the default run at the time (GCC 5, Clang 6) |
| CMake, later | `tests/cmake/run.sh gcc clang` | all checks passed, including GCC 16's `crtbeginS.o` in the Clang examples' link maps (GCC 13's with `-DYCXX_GCC_INSTALL_DIR=`) and the freestanding smoke link with the installed `libycxx-freestanding.a` |

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
