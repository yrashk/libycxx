// [ptr.align]/2-3: align(alignment, size, ptr, space): "If it is possible to fit size bytes of
// storage aligned by alignment into the buffer pointed to by ptr with length space, the
// function updates ptr to represent the first possible address of such storage and decreases
// space by the number of bytes used for alignment. Otherwise, the function does nothing."
// Returns null if it does not fit, otherwise the adjusted ptr.
#include <memory>
#include <cstddef>
#include <cstdint>
#include "check.hpp"

int main() {
  alignas(32) unsigned char buf[64];
  // size 0 fits anywhere the alignment can be reached
  void* ptr = buf + 3;
  std::size_t space = 13;
  void* r = std::align(8, 0, ptr, space);
  CHECK(r == buf + 8 && ptr == buf + 8 && space == 8);
  // alignment 1 never adjusts
  ptr = buf + 5;
  space = 10;
  r = std::align(1, 10, ptr, space);
  CHECK(r == buf + 5 && space == 10);
  // one byte too many: no change
  ptr = buf + 5;
  space = 10;
  r = std::align(1, 11, ptr, space);
  CHECK(r == nullptr && ptr == buf + 5 && space == 10);
  // space 0 with size 0 and an aligned pointer fits
  ptr = buf;
  space = 0;
  r = std::align(16, 0, ptr, space);
  CHECK(r == buf && space == 0);
  // space smaller than the padding needed: no change even for size 0
  ptr = buf + 1;
  space = 6;
  r = std::align(8, 0, ptr, space);
  CHECK(r == nullptr && ptr == buf + 1 && space == 6);
  // padding exactly consumes the space and size is 0
  ptr = buf + 1;
  space = 7;
  r = std::align(8, 0, ptr, space);
  CHECK(r == buf + 8 && space == 0);
  // repeated calls carve the buffer
  ptr = buf + 1;
  space = 63;
  void* a1 = std::align(4, 4, ptr, space);
  CHECK(a1 == buf + 4 && space == 60);
  ptr = static_cast<unsigned char*>(ptr) + 4;
  space -= 4;
  void* a2 = std::align(16, 8, ptr, space);
  CHECK(a2 == buf + 16 && space == 48);
  CHECK(reinterpret_cast<std::uintptr_t>(a2) % 16 == 0);
  return 0;
}
