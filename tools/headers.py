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
# Hosted: need an OS (through the PAL) or the C library.
HOSTED = [
    "any", "cctype", "cerrno", "cfenv", "cinttypes", "clocale", "csetjmp", "csignal", "cstdarg", "cstdio",
    "cstdlib", "cstring", "ctime", "cuchar", "cwchar", "cwctype", "system_error",
]
# <memory_resource>: memory_resource and polymorphic_allocator are core (ycxx/core/
# memory_resource.hpp, which <string> includes); the global resources, the pools and
# monotonic_buffer_resource are defined in the hosted runtime.
HOSTED += ["memory_resource"]
# Language-support headers whose *declarations* are core but which need the C++ ABI runtime
# (libycxx-abi) to be used with exceptions/RTTI enabled.
ABI = ["exception", "stdexcept", "typeinfo", "typeindex"]
