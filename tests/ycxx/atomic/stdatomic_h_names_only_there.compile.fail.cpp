// [stdatomic.h.syn]/3: "Neither the _Atomic macro, nor any of the non-macro global namespace
// declarations, are provided by any C++ standard library header other than <stdatomic.h>."
// <atomic> must therefore not declare ::atomic_int (the control on the first lines compiles).
#include <atomic>
#include <cstdint>
#include <memory>

std::atomic_int fine(0);  // control
std::memory_order also_fine = std::memory_order_relaxed;

::atomic_int not_declared(0);  // the error
