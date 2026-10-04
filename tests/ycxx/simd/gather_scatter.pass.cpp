// [simd.permute.memory]: partial_gather_from(in[, mask], indices) returns
// mask[i] && indices[i] < size(in) ? static_cast<T>(data(in)[indices[i]]) : T() (/9), V defaulting
// to vec<range_value_t<R>, I::size()> (/10); unchecked_gather_from is the same with the
// precondition that the selected indices are in range. partial_scatter_to(v, out[, mask],
// indices) assigns data(out)[indices[i]] = v[i] for selected in-range indices (/18).
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
  // gather / scatter
  std::array<int, 6> src{100, 101, 102, 103, 104, 105};
  simd::vec<int, 4> gi([](int i) { return 5 - i; });  // 5 4 3 2
  auto gv = simd::unchecked_gather_from(src, gi);
  static_assert(std::is_same_v<decltype(gv), simd::vec<int, 4>>);
  CHECK(eq(gv, {105, 104, 103, 102}));
  simd::vec<int, 4> far([](int i) { return i * 3; });  // 0 3 6 9
  auto pg = simd::partial_gather_from(src, far);
  CHECK(eq(pg, {100, 103, 0, 0}));
  auto mg = simd::partial_gather_from(src, simd::mask<int, 4>(0b0101u), gi);
  CHECK(eq(mg, {105, 0, 103, 0}));
  auto conv = simd::unchecked_gather_from<simd::vec<long long, 4>>(src, gi);
  CHECK(conv[0] == 105);
  std::array<int, 6> dst{};
  simd::unchecked_scatter_to(simd::vec<int, 4>([](int i) { return i + 1; }), dst, gi);
  CHECK((dst == std::array<int, 6>{0, 0, 4, 3, 2, 1}));
  dst = {};
  simd::partial_scatter_to(simd::vec<int, 4>(7), dst, far);
  CHECK((dst == std::array<int, 6>{7, 0, 0, 7, 0, 0}));
  dst = {};
  simd::partial_scatter_to(simd::vec<int, 4>(8), dst, simd::mask<int, 4>(0b0010u), gi);
  CHECK((dst == std::array<int, 6>{0, 0, 0, 0, 8, 0}));

  return true;
}
static_assert(test());

int main() {
  test();
  return 0;
}
