// [allocator.members]/4: allocate(n) "Throws: bad_array_new_length if
// numeric_limits<size_t>::max() / sizeof(T) < n, or bad_alloc if the storage cannot be
// obtained." [allocator.globals]: operator== returns true for any two allocators.
// [default.allocator.general]: converting constructor from allocator<U> is noexcept.
// UNSUPPORTED-SANITIZER: asan  ASan replaces the global allocation functions: its operator new neither calls the new_handler nor throws for impossible sizes, and its other forms do not forward to a program's replacement ([new.delete])
#include <memory>
#include <cstddef>
#include <limits>
#include <new>
#include "check.hpp"

struct Big {
  char data[64];
};

int main() {
  std::allocator<Big> a;
  constexpr std::size_t too_many = std::numeric_limits<std::size_t>::max() / sizeof(Big) + 1;
  int stage = 0;
  try {
    (void)a.allocate(too_many);
  } catch (const std::bad_array_new_length&) {
    stage = 1;
  }
  CHECK(stage == 1);
  try {
    (void)a.allocate(std::numeric_limits<std::size_t>::max());
  } catch (const std::bad_alloc&) {  // bad_array_new_length derives from bad_alloc
    stage = 2;
  }
  CHECK(stage == 2);
  // exactly at the limit is not a length error (it may still fail as bad_alloc)
  std::allocator<char> c;
  try {
    (void)c.allocate(std::numeric_limits<std::size_t>::max());
    stage = 3;  // unexpected, but not a conformance failure
  } catch (const std::bad_array_new_length&) {
    stage = -1;
  } catch (const std::bad_alloc&) {
    stage = 3;
  }
  CHECK(stage == 3);

  // round trip through a converted allocator
  std::allocator<int> ai(a);
  int* p = ai.allocate(3);
  CHECK(p != nullptr);
  ai.deallocate(p, 3);
  CHECK(ai == a && !(ai != a));
  return 0;
}
