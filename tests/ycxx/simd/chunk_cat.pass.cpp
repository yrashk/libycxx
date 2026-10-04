// [simd.creation]/1-5: chunk<T>(x) returns array<T, N> when T::size() divides x.size(),
// otherwise a tuple of N objects of type T and one resize_t<x.size() % T::size(), T>; chunk<N>(x)
// is chunk<resize_t<N, ...>>(x); masks likewise. /6: cat concatenates vecs (or masks) of any
// widths into resize_t<sum of widths, ...>.
#include <simd>
#include <array>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<int, 8>;
using M = V::mask_type;

template <class W>
constexpr bool eq(const W& w, std::initializer_list<typename W::value_type> l) {
  if (static_cast<std::size_t>(W::size()) != l.size()) return false;
  int i = 0;
  for (auto x : l)
    if (w[i++] != x) return false;
  return true;
}

constexpr bool test() {
  V v = V([](int i) { return 10 * i; });  // 0 10 ... 70
  M mk([](int i) { return i < 3; });
  // chunk / cat
  auto parts = simd::chunk<simd::vec<int, 4>>(v);
  static_assert(std::is_same_v<decltype(parts), std::array<simd::vec<int, 4>, 2>>);
  CHECK(eq(parts[0], {0, 10, 20, 30}) && eq(parts[1], {40, 50, 60, 70}));
  auto uneven = simd::chunk<3>(v);
  static_assert(std::is_same_v<decltype(uneven),
                               std::tuple<simd::vec<int, 3>, simd::vec<int, 3>, simd::vec<int, 2>>>);
  CHECK(eq(std::get<1>(uneven), {30, 40, 50}) && eq(std::get<2>(uneven), {60, 70}));
  auto mparts = simd::chunk<4>(mk);
  static_assert(std::is_same_v<decltype(mparts), std::array<simd::mask<int, 4>, 2>>);
  CHECK(mparts[0][2] && !mparts[0][3] && !mparts[1][0]);
  auto joined = simd::cat(std::get<2>(uneven), parts[0], simd::vec<int, 1>(5));
  static_assert(std::is_same_v<decltype(joined), simd::vec<int, 7>>);
  CHECK(eq(joined, {60, 70, 0, 10, 20, 30, 5}));
  auto mj = simd::cat(simd::mask<int, 2>(true), simd::mask<int, 3>(false));
  static_assert(std::is_same_v<decltype(mj), simd::mask<int, 5>>);
  CHECK(mj[1] && !mj[2]);
  return true;
}
static_assert(test());

int main() {
  test();
  return 0;
}
