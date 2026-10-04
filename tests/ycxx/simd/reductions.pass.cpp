// [simd.reductions]:
// - reduce(x, op = plus<>) is GENERALIZED_SUM of the elements (/3);
// - reduce(x, mask, op, identity) returns identity if none_of(mask), otherwise the sum of the
//   selected elements (/7); the default identity is T() for plus<>, bit_or<>, bit_xor<>, T(1)
//   for multiplies<>, T(~T()) for bit_and<> (/9); other operations need an explicit identity
//   (/5.2);
// - reduce_min/reduce_max (/16, /20); with a mask, numeric_limits<T>::max() / lowest() when no
//   element is selected (/18, /22);
// [simd.mask.reductions]: all_of, any_of, none_of, reduce_count, reduce_min_index,
// reduce_max_index, and their bool overloads.
#include <simd>
#include <functional>
#include <limits>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<int, 6>;
using M = V::mask_type;

struct Max {
  template <class T>
  constexpr T operator()(const T& a, const T& b) const {
    return simd::select(a < b, b, a);
  }
};

template <class... A>
concept can_reduce = requires(A... a) { simd::reduce(a...); };

constexpr bool test() {
  V v([](int i) { return i + 1; });  // 1..6
  CHECK(simd::reduce(v) == 21);
  CHECK(simd::reduce(v, std::plus<>()) == 21);
  CHECK(simd::reduce(v, std::multiplies<>()) == 720);
  CHECK(simd::reduce(v, std::bit_or<>()) == 7 && simd::reduce(v, std::bit_and<>()) == 0);
  CHECK(simd::reduce(v, std::bit_xor<>()) == (1 ^ 2 ^ 3 ^ 4 ^ 5 ^ 6));
  CHECK(simd::reduce(v, Max()) == 6);
  static_assert(std::is_same_v<decltype(simd::reduce(v)), int>);

  M odd([](int i) { return i % 2 == 0; });  // elements 1, 3, 5
  M none(false);
  CHECK(simd::reduce(v, odd) == 9);
  CHECK(simd::reduce(v, odd, std::multiplies<>()) == 15);
  CHECK(simd::reduce(v, none) == 0);
  CHECK(simd::reduce(v, none, std::multiplies<>()) == 1);
  CHECK(simd::reduce(v, none, std::bit_and<>()) == ~0);
  CHECK(simd::reduce(v, none, std::bit_or<>()) == 0 && simd::reduce(v, none, std::bit_xor<>()) == 0);
  CHECK(simd::reduce(v, odd, Max(), std::numeric_limits<int>::lowest()) == 5);
  CHECK(simd::reduce(v, none, Max(), -100) == -100);
  static_assert(can_reduce<V, M, Max, int> && !can_reduce<V, M, Max>);

  CHECK(simd::reduce_min(v) == 1 && simd::reduce_max(v) == 6);
  CHECK(simd::reduce_min(v, !odd) == 2 && simd::reduce_max(v, odd) == 5);
  CHECK(simd::reduce_min(v, none) == std::numeric_limits<int>::max());
  CHECK(simd::reduce_max(v, none) == std::numeric_limits<int>::lowest());
  simd::vec<float, 3> f([](int i) { return 1.5f - float(i); });
  CHECK(simd::reduce_max(f, simd::vec<float, 3>::mask_type(false)) == std::numeric_limits<float>::lowest());
  CHECK(simd::reduce_min(f) == -0.5f && simd::reduce(f) == 1.5f);

  // mask reductions
  M all(true);
  M some([](int i) { return i == 2 || i == 4; });
  CHECK(simd::all_of(all) && simd::any_of(all) && !simd::none_of(all) && simd::reduce_count(all) == 6);
  CHECK(!simd::all_of(none) && !simd::any_of(none) && simd::none_of(none) && simd::reduce_count(none) == 0);
  CHECK(!simd::all_of(some) && simd::any_of(some) && !simd::none_of(some) && simd::reduce_count(some) == 2);
  CHECK(simd::reduce_min_index(some) == 2 && simd::reduce_max_index(some) == 4);
  CHECK(simd::reduce_min_index(all) == 0 && simd::reduce_max_index(all) == 5);
  static_assert(std::is_signed_v<decltype(simd::reduce_count(all))>);
  CHECK(simd::all_of(true) && !simd::all_of(false) && simd::any_of(true) && simd::none_of(false) && !simd::none_of(true));
  CHECK(simd::reduce_count(true) == 1 && simd::reduce_count(false) == 0);
  CHECK(simd::reduce_min_index(true) == 0 && simd::reduce_max_index(true) == 0);
  return true;
}
static_assert(test());

int main() {
  test();
  simd::vec<unsigned char, 64> big((unsigned char)1);
  CHECK(simd::reduce(big) == 64);  // unsigned char arithmetic, result converted to T
  simd::vec<double, 7> d([](int i) { return 0.5 * i; });
  CHECK(simd::reduce(d) == 10.5);
  return 0;
}
