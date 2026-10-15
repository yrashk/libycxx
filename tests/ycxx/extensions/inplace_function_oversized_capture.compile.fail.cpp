// An actual oversized lambda construction must fail, rather than silently use the heap.
// EXPECT-ERROR: constraints not satisfied
#include <ycxx/functional.hpp>
#include <array>

void test() {
  auto closure = [values = std::array<void*, 5>{}] { return values[0]; };
  static_assert(sizeof(closure) > 4 * sizeof(void*));
  ycxx::inplace_function<void*()> callback(closure);
}
