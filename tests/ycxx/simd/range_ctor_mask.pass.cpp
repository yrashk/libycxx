// [simd.ctor]/11-15: basic_vec(R&& r, const mask_type& mask, flags<Flags...> = {}) initializes
// the ith element with mask[i] ? static_cast<T>(ranges::data(r)[i]) : T().
#include <simd>
#include <array>
#include <span>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<int, 5>;

int main() {
  std::array<int, 5> arr{5, 4, 3, 2, 1};
  V masked(arr, V::mask_type([](int i) { return i % 2 == 0; }));
  CHECK(masked[0] == 5 && masked[1] == 0 && masked[2] == 3 && masked[3] == 0 && masked[4] == 1);
  const double d[5] = {1.5, 2.5, 3.5, 4.5, 5.5};
  V conv(std::span<const double, 5>(d), V::mask_type(true), simd::flag_convert);
  CHECK(conv[0] == 1 && conv[4] == 5);
  V none(arr, V::mask_type(false));
  CHECK(simd::none_of(none != 0));
  return 0;
}
