"""Header manifest: the layer of every public header. Single source of truth for the
include-graph check, the freestanding check and STATUS.md."""

CORE = [
    "array", "bit", "cassert", "charconv", "stdfloat", "functional", "iterator", "tuple", "cfloat", "climits", "compare", "concepts", "coroutine", "cstddef", "cstdint",
    "initializer_list", "limits", "memory", "new", "optional", "source_location", "variant", "expected", "span", "string_view", "bitset", "type_traits", "utility",
    "version",
    # <string>: the sto* functions and floating-point to_string are declared in core and
    # defined in the hosted runtime (libycxx.a), like the <stdexcept> members.
    "string",
]
CORE += ["algorithm", "numeric", "execution", "ranges"]
CORE += ["scoped_allocator"]
CORE += ["vector", "inplace_vector"]
CORE += ["deque", "list", "forward_list", "stack", "queue"]
CORE += ["map", "set", "flat_map", "flat_set"]
CORE += ["unordered_map", "unordered_set", "hive"]
CORE += ["mdspan", "linalg"]
# <simd>: the mathematical functions need libm at run time, like <cmath>.
CORE += ["simd"]
# Numerics (<cmath>: see DECISIONS §3; the run-time calls of its functions need libm).
CORE += ["ratio", "numbers", "cmath", "complex", "valarray"]
# <atomic>: operations that are not lock-free and the waits use the runtime archive's tables
# (libycxx.a and the freestanding archive).
CORE += ["atomic", "stdatomic.h"]
# <debugging>: defined in the runtime archives (is_debugger_present asks the PAL).
CORE += ["debugging"]
# <contracts>: the default contract-violation handler is in the runtime archives.
CORE += ["contracts"]
# <random>: random_device is declared in core and defined in the hosted runtime.
CORE += ["random"]
# <meta>: reflection needs the compiler's support (GCC 16 -freflection); empty without it.
CORE += ["meta"]
# Hosted: need an OS (through the PAL) or the C library.
HOSTED = [
    "any", "cctype", "cfenv", "cinttypes", "clocale", "csetjmp", "csignal", "cstdio",
    "ctime", "cuchar", "cwctype",
]
# <cstdarg>: compiler builtins only. <stdbit.h>: on <bit>. <stdckdint.h>: overflow builtins.
CORE += ["cstdarg", "stdbit.h", "stdckdint.h"]
# Hosted headers with a freestanding subset ([compliance]): with YCXX_HOSTED 0 (-ffreestanding)
# they include core headers instead of the C library's (the freestanding parts of <cstdlib>,
# <cstring>, <cwchar>: ycxx/core/c_stdlib.hpp, c_string.hpp; <cerrno>'s macros; <system_error>'s
# errc and classes, whose categories are in the hosted runtime).
FREESTANDING_SUBSET = ["cstdlib", "cstring", "cwchar", "cerrno", "system_error"]
# <memory_resource>: memory_resource and polymorphic_allocator are core (ycxx/core/
# memory_resource.hpp, which <string> includes); the global resources, the pools and
# monotonic_buffer_resource are defined in the hosted runtime.
HOSTED += ["memory_resource"]
# <chrono>: the arithmetic is core (ycxx/core/chrono_base.hpp), the clocks need the OS.
HOSTED += ["chrono"]
# The thread support library: threads, mutexes and condition variables need the OS (PAL).
HOSTED += ["thread", "stop_token", "mutex", "shared_mutex", "condition_variable", "semaphore", "latch", "barrier",
           "future", "rcu", "hazard_pointer"]
# <math.h>: the C library's header plus <cmath>'s names in the global namespace.
HOSTED += ["math.h"]
# <stdlib.h>, <inttypes.h>, <string.h>: the C library's plus the names <cstdlib>, <cinttypes>,
# <cstring> declare themselves; <complex.h>, <tgmath.h>: <complex> (and <cmath>) in C++.
HOSTED += ["stdlib.h", "inttypes.h", "string.h", "complex.h", "tgmath.h"]
# Iostreams and localization (Phase 4): the non-template parts are in the hosted runtime.
HOSTED += ["iosfwd", "ios", "streambuf", "istream", "ostream", "iostream", "sstream", "spanstream", "fstream",
           "syncstream", "iomanip", "locale"]
# Formatting: the machinery and the formatters are core headers (ycxx/core/format_*.hpp), the
# locale-dependent parts and the print functions are in the hosted runtime.
HOSTED += ["format", "print"]
# <filesystem> (POSIX): the operations are in the hosted runtime.
HOSTED += ["filesystem"]
# <regex>: the name tables and regex_error's members are in the hosted runtime.
HOSTED += ["regex"]
# Language-support headers whose *declarations* are core but which need the C++ ABI runtime
# (libycxx-abi) to be used with exceptions/RTTI enabled.
ABI = ["exception", "stdexcept", "typeinfo", "typeindex"]
# <generator>: core code, but a generator's promise stores and rethrows exceptions
# (current_exception/rethrow_exception) through the ABI runtime.
ABI += ["generator"]
# <text_encoding>: the class is constexpr core code (ycxx/core/text_encoding.hpp); environment()
# and locale::encoding() are in the hosted runtime.
HOSTED += ["text_encoding"]
# <stacktrace>: capture and symbolization are in the hosted runtime (unwinder, PAL).
HOSTED += ["stacktrace"]

# [compliance] Table 27: the headers a freestanding implementation provides at least. The
# freestanding check compiles each of them (with CORE and FREESTANDING_SUBSET).
FREESTANDING_REQUIRED = [
    "cstddef", "cstdlib", "cfloat", "climits", "limits", "version", "cstdint", "new", "typeinfo",
    "source_location", "exception", "contracts", "initializer_list", "compare", "coroutine", "cstdarg",
    "concepts", "cerrno", "system_error", "debugging", "memory", "type_traits", "ratio", "utility", "tuple",
    "optional", "variant", "expected", "functional", "bit", "stdbit.h", "array", "inplace_vector", "span",
    "mdspan", "iterator", "ranges", "algorithm", "numeric", "execution", "string_view", "string", "cstring",
    "cwchar", "charconv", "random", "cmath", "atomic",
]
