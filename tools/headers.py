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
# Numerics (<cmath>: see DECISIONS §3; the run-time calls of its functions need libm).
CORE += ["ratio", "numbers", "cmath", "complex", "valarray"]
# <atomic>: operations that are not lock-free and the waits use the runtime archive's tables
# (libycxx.a and the freestanding archive).
CORE += ["atomic", "stdatomic.h"]
# Hosted: need an OS (through the PAL) or the C library.
HOSTED = [
    "any", "cctype", "cerrno", "cfenv", "cinttypes", "clocale", "csetjmp", "csignal", "cstdarg", "cstdio",
    "cstdlib", "cstring", "ctime", "cuchar", "cwchar", "cwctype", "system_error",
]
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
# Iostreams and localization (Phase 4): the non-template parts are in the hosted runtime.
HOSTED += ["iosfwd", "ios", "streambuf", "istream", "ostream", "iostream", "sstream", "spanstream", "fstream",
           "syncstream", "iomanip", "locale"]
# Language-support headers whose *declarations* are core but which need the C++ ABI runtime
# (libycxx-abi) to be used with exceptions/RTTI enabled.
ABI = ["exception", "stdexcept", "typeinfo", "typeindex"]
