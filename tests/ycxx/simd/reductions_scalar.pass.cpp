// [simd.reductions]/10-14: reduce(const T& x, op) for a vectorizable T returns x;
// reduce(x, bool mask, op, identity) returns identity when mask is false (the default identity
// is T() for plus<>, bit_or<>, bit_xor<>, T(1) for multiplies<>, T(~T()) for bit_and<>), x
// otherwise. /23-25: reduce_min/reduce_max(x[, mask]) return x, or numeric_limits<T>::max() /
// lowest() when mask is false.
#include <simd>
#include <functional>
#include <limits>
#include "check.hpp"

namespace simd = std::simd;

struct Max {
  template <class T>
  constexpr T operator()(const T& a, const T& b) const {
    return simd::select(a < b, b, a);
  }
};

constexpr bool test() {
  // scalar overloads
  CHECK(simd::reduce(5) == 5 && simd::reduce(5, std::multiplies<>()) == 5);
  CHECK(simd::reduce(5, false) == 0 && simd::reduce(5, false, std::multiplies<>()) == 1);
  CHECK(simd::reduce(5, true, std::bit_and<>()) == 5 && simd::reduce(5, false, std::bit_and<>()) == ~0);
  CHECK(simd::reduce(5, false, Max(), -1) == -1 && simd::reduce(5, true, Max(), -1) == 5);
  CHECK(simd::reduce_min(3) == 3 && simd::reduce_max(3, true) == 3);
  CHECK(simd::reduce_min(3, false) == std::numeric_limits<int>::max());
  CHECK(simd::reduce_max(3.0, false) == std::numeric_limits<double>::lowest());

  return true;
}
static_assert(test());

int main() {
  test();
  return 0;
}
