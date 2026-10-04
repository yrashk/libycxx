"""Header manifest: the layer of every public header. Single source of truth for the
include-graph check, the freestanding check and STATUS.md."""

CORE = [
    "array", "bit", "cassert", "stdfloat", "functional", "iterator", "tuple", "cfloat", "climits", "compare", "concepts", "coroutine", "cstddef", "cstdint",
    "initializer_list", "limits", "memory", "new", "optional", "source_location", "variant", "expected", "span", "string_view", "bitset", "type_traits", "utility",
    "version",
]
# Hosted: need an OS (through the PAL) or the C library.
HOSTED = [
    "any", "cctype", "cerrno", "cfenv", "cinttypes", "clocale", "csetjmp", "csignal", "cstdarg", "cstdio",
    "cstdlib", "cstring", "ctime", "cuchar", "cwchar", "cwctype", "system_error",
]
# Language-support headers whose *declarations* are core but which need the C++ ABI runtime
# (libycxx-abi) to be used with exceptions/RTTI enabled.
ABI = ["exception", "stdexcept", "typeinfo", "typeindex"]
