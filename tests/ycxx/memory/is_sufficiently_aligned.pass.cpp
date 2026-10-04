// [ptr.align]/10-13: template<size_t Alignment, class T> bool is_sufficiently_aligned(T* ptr);
// "Mandates: Alignment is a power of two." "Returns: true if X has alignment at least
// Alignment, otherwise false." "Throws: Nothing."
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::is_sufficiently_aligned<4>(std::declval<int*>())), bool>);

int main() {
  alignas(64) unsigned char buf[256];
  CHECK(std::is_sufficiently_aligned<1>(buf));
  CHECK(std::is_sufficiently_aligned<2>(buf));
  CHECK(std::is_sufficiently_aligned<16>(buf));
  CHECK(std::is_sufficiently_aligned<64>(buf));
  CHECK(std::is_sufficiently_aligned<1>(buf + 1));
  CHECK(!std::is_sufficiently_aligned<2>(buf + 1));
  CHECK(std::is_sufficiently_aligned<32>(buf + 32));
  CHECK(!std::is_sufficiently_aligned<64>(buf + 32));
  CHECK(std::is_sufficiently_aligned<8>(buf + 8));
  CHECK(!std::is_sufficiently_aligned<16>(buf + 8));
  CHECK(std::is_sufficiently_aligned<128>(buf + 128) == (reinterpret_cast<std::uintptr_t>(buf + 128) % 128 == 0));
  // Pointers to cv-qualified objects.
  const unsigned char* cp = buf + 4;
  CHECK(std::is_sufficiently_aligned<4>(cp));
  CHECK(!std::is_sufficiently_aligned<8>(cp));
  volatile unsigned char* vp = buf + 16;
  CHECK(std::is_sufficiently_aligned<16>(vp));
  // Every object is aligned to at least its type's alignment.
  long long ll = 0;
  CHECK(std::is_sufficiently_aligned<alignof(long long)>(&ll));
  struct alignas(32) Over { char c; } over;
  CHECK(std::is_sufficiently_aligned<32>(&over));
  double d[4] = {};
  CHECK(std::is_sufficiently_aligned<alignof(double)>(&d[3]));
  return 0;
}
