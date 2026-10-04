// [ptr.align]: void* align(size_t alignment, size_t size, void*& ptr, size_t& space);
// updates ptr to the first suitably aligned address and decreases space by the bytes used
// for alignment, returning the adjusted ptr; if the request does not fit it does nothing and
// returns a null pointer. assume_aligned<N>(ptr) is constexpr and returns ptr.
#include <memory>
#include <cstddef>
#include <cstdint>
#include "check.hpp"

constexpr bool test_assume() {
  alignas(16) int a[4] = {1, 2, 3, 4};
  int* p = std::assume_aligned<16>(a);
  if (p != a || p[3] != 4) return false;
  const int* cp = std::assume_aligned<4>(static_cast<const int*>(a + 1));
  return cp == a + 1;
}
static_assert(test_assume());

int main() {
  CHECK(test_assume());
  alignas(64) unsigned char buf[128];
  void* ptr = buf + 1;
  std::size_t space = 100;
  void* r = std::align(16, 8, ptr, space);
  CHECK(r == buf + 16);
  CHECK(ptr == buf + 16);
  CHECK(space == 100 - 15);
  // already aligned: no change
  r = std::align(16, 8, ptr, space);
  CHECK(r == buf + 16 && space == 85);
  // does not fit: nothing changes, null returned
  void* before = ptr;
  r = std::align(64, 80, ptr, space);  // needs 48 bytes of padding + 80 > 85
  CHECK(r == nullptr);
  CHECK(ptr == before && space == 85);
  // fits exactly
  ptr = buf + 1;
  space = 63 + 1;
  r = std::align(64, 1, ptr, space);
  CHECK(r == buf + 64 && space == 1);
  return 0;
}
