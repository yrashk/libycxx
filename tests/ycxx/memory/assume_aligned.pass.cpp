// [ptr.align]/5-8: template<size_t N, class T> constexpr T* assume_aligned(T* ptr); "Mandates:
// N is a power of two." "Preconditions: ptr points to an object X of a type similar to T,
// where X has alignment N." "Returns: ptr." "Throws: Nothing." It is constexpr, and T may be
// cv-qualified (the result keeps the qualification).
#include <memory>
#include <type_traits>
#include "check.hpp"

struct alignas(32) Wide {
  int v[8];
};
alignas(64) int buffer[16] = {1, 2, 3};

static_assert(std::is_same_v<decltype(std::assume_aligned<64>(buffer)), int*>);
static_assert(std::is_same_v<decltype(std::assume_aligned<4>(static_cast<const int*>(buffer))), const int*>);
static_assert(std::is_same_v<decltype(std::assume_aligned<32>(static_cast<volatile Wide*>(nullptr))),
                             volatile Wide*>);

constexpr int in_constexpr() {
  int local[4] = {4, 5, 6, 7};
  int* p = std::assume_aligned<alignof(int)>(local);
  return p == local ? p[2] : -1;
}
static_assert(in_constexpr() == 6);

int main() {
  CHECK(std::assume_aligned<64>(buffer) == buffer);
  CHECK(std::assume_aligned<64>(buffer)[2] == 3);
  CHECK(std::assume_aligned<1>(buffer + 1) == buffer + 1);
  CHECK(std::assume_aligned<16>(buffer + 4) == buffer + 4);  // 64-aligned + 16 bytes
  Wide w{};
  w.v[3] = 9;
  const Wide* cw = std::assume_aligned<32>(static_cast<const Wide*>(&w));
  CHECK(cw == &w && cw->v[3] == 9);
  CHECK(std::assume_aligned<alignof(Wide)>(&w) == &w);
  return 0;
}
