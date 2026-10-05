// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>

[[gnu::weak]] void* operator new(std::size_t) { ycxx::detail::throw_bad_alloc(); }
// Marks that this heap-less default is the operator new linked into the program (see
// try_or_null.hpp).
extern "C" const char ycxx_fs_default_new = 0;
