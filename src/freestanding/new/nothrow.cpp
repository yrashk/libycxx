// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>

// [new.syn]: the object std::nothrow (hosted builds get it from the ABI runtime).
namespace [[__gnu__::__visibility__("hidden")]] std {
extern const nothrow_t nothrow;
const nothrow_t nothrow{};
} // namespace std
