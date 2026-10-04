// Freestanding smoke test: exercises core headers with no OS, no libc, no exceptions, no RTTI.
#include <bit>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace {
struct Pt {
  int x, y;
  auto operator<=>(const Pt&) const = default;
};
} // namespace

extern "C" int ycxx_freestanding_main() {
  int r = 0;
  std::pair<int, long> p{1, 2};
  auto [a, b] = p;
  r += a + static_cast<int>(b);
  r += std::popcount(0xffu) + std::bit_width(5u);
  r += std::numeric_limits<std::int8_t>::max() > 100;
  r += (Pt{1, 2} < Pt{1, 3});
  alignas(int) unsigned char buf[sizeof(int)];
  int* ip = std::construct_at(reinterpret_cast<int*>(buf), 5);
  r += *ip;
  std::destroy_at(ip);
  assert(r > 0);
  return r;
}
