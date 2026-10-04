"""Header manifest: the layer of every public header. Single source of truth for the
include-graph check, the freestanding check and STATUS.md."""

CORE = [
    "bit", "cassert", "functional", "iterator", "cfloat", "climits", "compare", "concepts", "coroutine", "cstddef", "cstdint",
    "initializer_list", "limits", "memory", "new", "source_location", "type_traits", "utility",
    "version",
]
# Hosted: need an OS (through the PAL) or the C library.
HOSTED = [
    "cctype", "cerrno", "cfenv", "cinttypes", "clocale", "csetjmp", "csignal", "cstdarg", "cstdio",
    "cstdlib", "cstring", "ctime", "cuchar", "cwchar", "cwctype",
]
# Language-support headers whose *declarations* are core but which need the C++ ABI runtime
# (libsupc++) to be used with exceptions/RTTI enabled.
ABI = ["exception", "stdexcept", "typeinfo"]
