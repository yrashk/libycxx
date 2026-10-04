// [stdatomic.h.syn]/3: "Neither the _Atomic macro, nor any of the non-macro global namespace
// declarations, are provided by any C++ standard library header other than <stdatomic.h>."
// Including the concurrency headers (and others that may use atomics internally) defines no
// _Atomic macro and declares no global memory_order or atomic_flag.
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <future>
#include <stop_token>
#include <latch>
#include <barrier>
#include <semaphore>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#ifdef _Atomic
#error "_Atomic is defined without <stdatomic.h>"
#endif

namespace probe {
struct memory_order {};
struct atomic_flag {};
struct atomic_bool {};
}
using namespace probe;
// unambiguous only if the global namespace does not declare these names too
memory_order mo;
atomic_flag af;
atomic_bool ab;
int main() {}
