// [depr.atomics.order]/1: "The memory_order enumeration contains an additional enumerator:
// consume = 1. The memory_order::consume enumerator is allowed wherever memory_order::acquire
// is allowed, and it has the same meaning." /2: kill_dependency(y) returns y.
// FLAGS: -Wno-deprecated-declarations -Wno-deprecated
#include <atomic>
#include "check.hpp"

static_assert(static_cast<int>(std::memory_order::consume) == 1);
static_assert(std::memory_order_consume == std::memory_order::consume);
static_assert(noexcept(std::kill_dependency(7)));

int main() {
  CHECK(std::kill_dependency(7) == 7);
  std::atomic<int> a(3);
  CHECK(a.load(std::memory_order::consume) == 3);
  int x = 0;
  std::atomic<int*> p(&x);
  int* q = p.load(std::memory_order_consume);
  CHECK(std::kill_dependency(q) == &x);
  std::atomic_flag f;
  CHECK(!f.test(std::memory_order::consume));
  return 0;
}
