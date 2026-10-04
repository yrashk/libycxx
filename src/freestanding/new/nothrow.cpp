// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). Without exceptions the nothrow forms cannot
// detect a failure of a forwarded call, so they return a null pointer themselves; a program that
// replaces operator new should replace the nothrow forms too.
#include <new>
#include <ycxx/core/error.hpp>

// [new.syn]: the object std::nothrow (hosted builds get it from libsupc++).
namespace std {
extern const nothrow_t nothrow;
const nothrow_t nothrow{};
} // namespace std
