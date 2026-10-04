// [simd.mask.conv]/1-2: basic_mask<Bytes, Abi> converts to basic_vec<U, A> of the same width,
// explicitly when sizeof(U) != Bytes, with elements static_cast<U>(k[i]).
#include <simd>
#include <cstdint>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using M = simd::mask<int, 6>;

constexpr bool test() {
  M g([](int i) { return i % 3 == 0; });
  simd::vec<short, 6> s(g);
  CHECK(s[0] == 1 && s[1] == 0 && s[3] == 1);
  simd::vec<double, 6> d(g);
  CHECK(d[3] == 1.0 && d[4] == 0.0);
  simd::vec<std::uint8_t, 6> u = static_cast<simd::vec<std::uint8_t, 6>>(g);
  CHECK(u[0] == 1 && u[5] == 0);
  simd::mask<char, 6> c(g);
  simd::vec<long long, 6> l(c);
  CHECK(l[0] == 1 && l[2] == 0);
  return true;
}
static_assert(test());
static_assert(!std::is_convertible_v<M, simd::vec<short, 6>> && !std::is_convertible_v<M, simd::vec<double, 6>>);

int main() {
  test();
  return 0;
}
