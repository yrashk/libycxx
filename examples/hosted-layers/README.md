# Hosted layers: a hosted C++ library without an operating system

"Hosted" does not have to mean "every OS primitive". libycxx's hosted library is split into
**layers of support** (DECISIONS.md §18). Each layer is a handful of C functions of
`include/ycxx/pal.h` (the platform layer), plus the library features they enable. You choose the
layers and supply their functions through **providers**: your own code, in your own CMake
targets. The program then gets exactly the hosted features it has primitives for.

Examples A and B run the same program (`common/demo.cpp`: `vector`, `map`,
`unordered_map`, `string`, `unique_ptr`, `shared_ptr`, `function`, `any`, `format`/`print`,
exceptions through a hierarchy of the program's own, `exception_ptr`, a function-local static, a
`thread_local`, both clocks) on four layers, `abort`, `memory`, `console` and `clock`, whose
primitives each example provides itself:

| | Example A: `host/` | Example B: `limine/` |
|---|---|---|
| Runs on | the host OS (Linux), as an ordinary program | bare x86_64 (QEMU), booted by Limine |
| `memory` | a free-list heap over an 8 MiB static arena | the same heap over the largest usable region of Limine's memory map |
| `console`, `abort` | `write(2)`; `abort()` | COM1 (polled UART); QEMU's isa-debug-exit device |
| `clock` | `clock_gettime` | the TSC calibrated against the PIT; the CMOS clock |
| C library | the host's, for the C runtime start-up and the providers; libycxx does not use it | none |

Example C (`files/`) shows a layer of a program's own beyond those four: the file streams over a
RAM disk.

## The layers

| Layer | A provider defines (`ycxx/pal.h`) | Enables | When absent |
|---|---|---|---|
| `abort` (always) | `ycxx_pal_abort`; `ycxx_pal_write` to `ycxx_pal_stderr` | `std::terminate` and its report, uncaught exceptions, the default `ycxx_error_handler` (`-fno-exceptions`, hardened checks), contract violations | required |
| `memory` | `ycxx_pal_allocate`, `ycxx_pal_deallocate` | the default `operator new`/`delete`: containers, strings, `function`, `any`, `shared_ptr`, exception objects | link error: `undefined reference to 'ycxx_pal_allocate'` |
| `console` | `ycxx_pal_write` to `ycxx_pal_stdout`, `ycxx_pal_read`, `ycxx_pal_is_terminal` | `std::print`/`println` to standard output (without `clib`) | compile error: a static_assert naming the layer |
| `clock` | `ycxx_pal_clock_now` | `system_clock`, `steady_clock`, `high_resolution_clock` | link error naming `ycxx_pal_clock_now` |
| `threads` | `ycxx_pal_wait`/`_wake_*`/`_wait_until`, `ycxx_pal_thread_*`, `ycxx_pal_sleep_until`, `ycxx_pal_thread_atexit`, `ycxx_pal_at_thread_end`, ... | `thread`, `jthread`, sleeping, timed waits, `<future>`, `<rcu>`, `<hazard_pointer>` | one thread of execution (fallbacks); `std::thread` fails to compile, the rest to link |
| `random` | `ycxx_pal_random_open`/`_read`/`_close` | `random_device` | link error naming `ycxx_pal_random_open` |
| `files` | `ycxx_pal_file_open`/`_close`/`_read`/`_write`/`_seek`/`_flush` | the file streams over your storage | with `clib`, C stdio's files; without, no file streams |
| `environment` | `ycxx_pal_error_message`, `ycxx_pal_environment_encoding` | error-category messages, `text_encoding::environment()` | fallbacks: "error N", unknown encoding |
| `debug` | `ycxx_pal_debugger_present`, `ycxx_pal_object_of`, ... | `is_debugger_present`, `<stacktrace>` | `is_debugger_present()` is false; no `<stacktrace>` |
| `clib` | the toolchain's C library | the C library headers, iostreams, locales, `print(FILE*, ...)`, `sto*`, `<cmath>`'s run-time functions, `<regex>`, chrono I/O; needed by `threads`, `files`, `debug` | everything is compiled freestanding; those headers stop with an `#error` naming the layer |

`<filesystem>` and the time zone database still call POSIX directly and exist with
`YCXX_PAL=posix` only. The file streams, though, are a layer of their own (`files`): a provider
can put them over its own block device, a RAM disk or a network protocol.

## Choosing layers in CMake

```cmake
set(YCXX_PAL none)                               # no platform layer from libycxx
set(YCXX_HOSTED_LAYERS memory console clock)     # 'abort' is implied
add_subdirectory(path/to/libycxx libycxx EXCLUDE_FROM_ALL)

add_library(my_providers STATIC providers.c)     # defines the primitives
target_include_directories(my_providers PRIVATE path/to/libycxx/include)   # ycxx/pal.h
foreach(layer IN ITEMS abort memory console clock)
  ycxx_add_hosted_layer(${layer} PROVIDER my_providers)
endforeach()
# or: ycxx_add_hosted_layer(console SOURCES uart.c)   (a static library made for you)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE ycxx::ycxx)    # brings libycxx and the providers
```

- `YCXX_PAL=posix` (the default) is the usual build: every layer, from `src/pal/posix`.
- From the command line: `-DYCXX_PAL=none "-DYCXX_HOSTED_LAYERS=memory;console;clock"
  -DYCXX_PAL_MEMORY_PROVIDER=my_heap ...` (one `YCXX_PAL_<LAYER>_PROVIDER` per layer).
- The set of layers is fixed when libycxx is configured. `ycxx_add_hosted_layer` on a layer you
  did not select stops with a message saying how to select it. libycxx validates the list
  (`threads`, `files` and `debug` need `clib`) and, at the end of the configuration, prints each
  layer with its provider:

  ```
  -- libycxx: YCXX_PAL=none, hosted layers: abort memory console clock
  -- libycxx: hosted layer memory: provided by host_providers
  -- libycxx: hosted layers absent: threads random files environment debug clib filesystem tzdb
  ```
- Without `clib`, `ycxx::headers` compiles C++ freestanding (`-ffreestanding -nostdinc` and the
  compiler's own header directory). C sources are left alone: a provider may use whatever C
  library it has (Example A's uses the host's).
- After `cmake --install`, `find_package(libycxx)` defines `ycxx_add_hosted_layer` as well, and
  `libycxx_HOSTED_LAYERS` lists the layers of that build.

## Writing a provider

A provider is plain C (or C++ with C linkage) against `ycxx/pal.h`. The conventions:

- functions returning `int` return 0 or a positive errno-style number;
- nothing throws or unwinds; `ycxx_pal_abort` never returns;
- the declarations in `pal.h` carry hidden visibility, which the definitions inherit;
- a provider must not need libycxx itself, or it links `ycxx::ycxx` too, so that CMake orders
  the archives.

A console over a UART, for instance:

```c
#include <ycxx/pal.h>

int ycxx_pal_write(ycxx_pal_handle fd, const void* data, ycxx_pal_size n, ycxx_pal_size* written) {
  if (fd != ycxx_pal_stdout && fd != ycxx_pal_stderr) { *written = 0; return 9; /* EBADF */ }
  for (ycxx_pal_size i = 0; i < n; ++i) uart_put(((const char*)data)[i]);
  *written = n;
  return 0;
}
```

A file layer over your own storage implements the six `ycxx_pal_file_*` functions: `open` gets
the flags of fopen's modes (`ycxx_pal_file_read_access`, `_write_access`, `_append`, `_create`,
`_truncate`, `_exclusive`) and stores a nonzero handle; `read`, `write` and `seek` work on byte
positions. `std::ofstream`, `std::ifstream` and `std::fstream` then run on it (this layer needs
`clib` for the iostreams). Example C (`files/`) is one: a RAM disk in about 150 lines of C.

## Absent layers fail when the program is built

Example A's `absent/` programs use layers that build does not have (`tests/cmake/run.sh` checks
these diagnostics):

```
$ cmake --build build --target absent_thread           # std::thread
include/ycxx/hosted/thread.hpp: error: static assertion failed: std::thread needs the 'threads'
hosted layer: libycxx was configured without it (YCXX_HOSTED_LAYERS, DECISIONS §18), so there is
one thread of execution
$ cmake --build build --target absent_sleep            # this_thread::sleep_for
sleep.cpp: undefined reference to `ycxx_pal_sleep_until'
$ cmake --build build --target absent_random_device    # std::random_device
random.cpp: undefined reference to `ycxx_pal_random_open'
$ cmake --build build --target absent_fstream          # <fstream>
include/fstream:14:4: error: #error "libycxx: <fstream> needs the C library (the 'clib' hosted
layer, DECISIONS §18); this translation unit is compiled freestanding"
```

## What the environment still provides

Layers are what libycxx needs from the platform. A C++ program also needs, from its environment:

- the functions every compiler calls: `memcpy`, `memmove`, `memset`, `memcmp` (and `strlen`,
  `strcmp`, `strncmp`, which libycxx's ABI runtime calls);
- its start: the static constructors (`.init_array`), and `__cxa_atexit`/`__dso_handle` for the
  static destructors (without `threads`, libycxx registers `thread_local` destructors there
  too);
- the thread-local storage of its thread, when it uses `thread_local` or exceptions (libycxx's
  runtime keeps the current exceptions in `thread_local` objects);
- for exceptions, the unwinder. GCC's `libgcc_eh.a` is built for GNU/Linux: it finds the unwind
  tables through glibc's `_dl_find_object` (Example B returns its `.eh_frame_hdr`, which the
  linker makes with `--eh-frame-hdr`), and refers to `abort`, `malloc`, `free` and a few
  `pthread_*` functions, of which only `pthread_once` is called in a single-threaded program.

On a host (Example A) the C runtime provides all of that. Example B's `support.c` does it in
about 150 lines.

## Example A: on the host OS, without the POSIX platform layer

```sh
cmake -S examples/hosted-layers/host -B build/ex-host -G Ninja \
      -DCMAKE_C_COMPILER=gcc-16 -DCMAKE_CXX_COMPILER=g++-16     # or clang-23 / clang++-23
cmake --build build/ex-host
build/ex-host/hosted_layers_host
```

```
hosted-layers demo on the host OS: libycxx with YCXX_PAL=none
vector: 1000 squares, sum 332833500, first five [0, 1, 4, 9, 16]
map: {"clock": 1, "console": 1, "layer": 3, "memory": 1}
string: "hosted doesn't have to mean every OS primitive" (46 characters)
unique_ptr<shape>: circle with area 3.1416
unique_ptr<shape>: square with area 4.0000
function 42, any "an any", optional 2.5, variant "a variant"
format: [   right] [left    ] [  mid   ] [0003.142] [0xff] [6.022141e+23] [true]
caught parse_error (code 42): not a digit: 'x'
caught out_of_range: std::vector::at: index out of range
caught bad_any_cast: bad any_cast
rethrown: kept in an exception_ptr
caught int 7 after 2 destructors ran
hello from a function-local static; thread_local storage works
clock: the demo took 151 us; system_clock says 1791268102 s since 1970
hosted-layers demo: ok
heap (providers.c's arena): 8388608 bytes, peak in use 6176, 45 allocations, 2 blocks still allocated
```

- `CMakeLists.txt` selects the layers, builds `providers.c` and the heap into `host_providers`,
  and attaches it to all four layers.
- `providers.c` defines the eleven functions of those layers. `nm` shows none of libycxx's POSIX
  layer in the program, and no `malloc`: every allocation, exceptions included, comes from the
  arena.
- `main.cpp` declares `main` `extern "C"`. Compiled freestanding, Clang gives `main` no special
  linkage, and the host's C runtime calls the C symbol.

## Example C: your own file layer

`files/` keeps the host's C library as a layer (`YCXX_HOSTED_LAYERS=clib;memory;files`), so
iostreams and locales work, but supplies the `files` layer itself: `ramdisk.c`, a table of named
files whose contents grow on the program's heap. `std::ofstream`, `std::ifstream` and
`std::fstream` (text, append, binary, seeking) run on it, and the host's file system never sees the
files (`fopen` of the same name fails):

```sh
cmake -S examples/hosted-layers/files -B build/ex-files -G Ninja -DCMAKE_C_COMPILER=gcc-16 -DCMAKE_CXX_COMPILER=g++-16
cmake --build build/ex-files && build/ex-files/hosted_layers_files
```

```
hosted-layers-ramdisk-notes.txt on the RAM disk: 25 bytes, lines ["line one", "42 3.5", "appended"]
iostreams and locales work as usual, through the C library: 1.25
hosted-layers files demo: ok
```

`ycxx_add_hosted_layer(files SOURCES ramdisk.c)` makes the provider from sources; the abort and
memory layers reuse Example A's `providers.c`.

## Example B: bare x86_64 under QEMU, booted by Limine

```sh
examples/hosted-layers/limine/run.sh gcc      # or clang
```

`run.sh` builds the kernel with CMake and `toolchain.cmake`. It fetches Limine's binary release
(the `v11.4.1-binary` tag of github.com/limine-bootloader/limine) into
`build/limine-v11.4.1-binary` and builds its `limine` tool, then makes a hybrid BIOS/UEFI ISO
(`xorriso`, `limine bios-install`). Finally it boots the ISO in QEMU with no display, COM1 on
standard output and the isa-debug-exit device, under a timeout. It passes when QEMU exits with
the kernel's success status (33) and the serial output has the success line. It needs cmake,
ninja, git, a C compiler, `xorriso` and `qemu-system-x86_64` (`apt-get install qemu-system-x86
xorriso`). Set `LIMINE_DIR` to a copy of the release to work offline.

```
== booting in QEMU (timeout 120s); serial output follows
kernel: booted by Limine, base revision 6
kernel: heap of 64 MiB from the memory map; TSC at 2101 MHz
hosted-layers demo on bare-metal x86_64, booted by Limine: libycxx with YCXX_PAL=none
vector: 1000 squares, sum 332833500, first five [0, 1, 4, 9, 16]
...
caught parse_error (code 42): not a digit: 'x'
caught out_of_range: std::vector::at: index out of range
...
hosted-layers demo: ok
heap (a region of the Limine memory map): 67108864 bytes, peak in use 6176, 45 allocations, 2 blocks still allocated
kernel: done, exiting QEMU (success)
== QEMU exited with status 33
== PASS: the kernel ran the demonstration and reported success
```

The pieces:

- `toolchain.cmake`: `CMAKE_SYSTEM_NAME Generic`, the host's GCC 16.2 or Clang 23.1 with
  `-mno-red-zone -fno-stack-protector -fno-stack-check -fcf-protection=none -march=x86-64`, and
  position-independent code. This is the small code model, linked at `0xffffffff80000000`;
  libycxx's archives are PIC anyway, so `-mcmodel=kernel` is not needed.
- `linker.ld`: the higher-half image. Limine's requests come first, then text, then rodata with
  `.eh_frame_hdr` and `.eh_frame` (and a `PT_GNU_EH_FRAME` header), then data with
  `.init_array`/`.fini_array`, then the TLS segment and its layout for `boot.c`.
- `boot.c`: the Limine requests (base revision 6, HHDM, memory map) between the request
  delimiters, and the entry point. The entry point switches to its own 256 KiB stack and enables
  SSE: Limine clears `CR4.OSFXSR`, and the code is compiled for x86-64 with SSE2. `kernel_boot`
  then sets up COM1 and the TLS block below `%fs` (ELF TLS variant II). It takes the heap from
  the memory map and calibrates the clocks. It runs the constructors, `kernel_main`
  (`main.cpp`), and the destructors, then exits QEMU through isa-debug-exit.
- `providers.c`, `hw.c`: the four layers on the hardware.
- `support.c`: what the environment provides (above), including `_dl_find_object` for GCC's
  unwinder.
- `limine_protocol.h`: the few protocol structures used, written from Limine's `PROTOCOL.md`.

Exceptions work on bare metal: libycxx's own ABI runtime (personality routine, exception
allocation through the `memory` layer, `exception_ptr`) runs on GCC 16's `libgcc_eh.a`, for both
compilers.

## Not done yet

- `<filesystem>` and the time zone database have no primitives; they need `YCXX_PAL=posix`.
- `threads`, `files` and `debug` need the C library: `src/hosted/thread.cpp` copies the
  floating-point environment with `<cfenv>`, and iostreams and `<stacktrace>` use the C library.
  A bare-metal `threads` provider is therefore not possible yet.
- These builds are tested on Linux, with both compilers; on macOS they are not tested yet.
