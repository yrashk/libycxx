// [simd.loadstore]:
// - partial_load returns mask[i] && i < ranges::size(r) ? static_cast<T>(data(r)[i]) : T()
//   (/9); the range may be given as a range, (first, n) or (first, last); the default V is
//   basic_vec<range_value_t<R>> (/10); unchecked_load is partial_load with the precondition
//   size(r) >= V::size() (/4);
// - partial_store assigns data(r)[i] = static_cast<range_value_t<R>>(v[i]) iff mask[i] &&
//   i < ranges::size(r) (/19); unchecked_store is partial_store (/14);
// - flag_convert permits non-value-preserving conversions; flag_aligned / flag_overaligned<N>
//   only add alignment preconditions (alignment_v<V, U>).
// COUNTERPART: libcxx:experimental/simd/simd.class/.*
// COUNTERPART: libcxx:experimental/simd/simd.traits/(memory_alignment|simd_size).*
#include <simd>
#include <array>
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<int, 4>;
using M = V::mask_type;

constexpr bool test() {
  std::array<int, 6> a{1, 2, 3, 4, 5, 6};
  V v = simd::unchecked_load<V>(a);
  CHECK(v[0] == 1 && v[3] == 4);
  V p = simd::partial_load<V>(a.data(), 2);
  CHECK(p[0] == 1 && p[1] == 2 && p[2] == 0 && p[3] == 0);
  V q = simd::partial_load<V>(a.begin() + 3, a.end());
  CHECK(q[0] == 4 && q[1] == 5 && q[2] == 6 && q[3] == 0);
  V u = simd::unchecked_load<V>(a.begin() + 2, 4);
  CHECK(u[0] == 3 && u[3] == 6);
  V u2 = simd::unchecked_load<V>(a.begin() + 1, a.begin() + 5);
  CHECK(u2[0] == 2 && u2[3] == 5);
  M even([](int i) { return i % 2 == 0; });
  V pm = simd::partial_load<V>(a, even);
  CHECK(pm[0] == 1 && pm[1] == 0 && pm[2] == 3 && pm[3] == 0);
  V pm2 = simd::partial_load<V>(a.data(), 1, even);
  CHECK(pm2[0] == 1 && pm2[1] == 0 && pm2[2] == 0);
  V empty = simd::partial_load<V>(a.data(), 0);
  CHECK(simd::all_of(empty == 0));

  // default V
  auto d = simd::unchecked_load(std::vector<float>(simd::vec<float>::size(), 2.5f));
  static_assert(std::is_same_v<decltype(d), simd::vec<float>>);
  CHECK(d[0] == 2.5f);
  auto dp = simd::partial_load(a.data(), 3);
  static_assert(std::is_same_v<decltype(dp), simd::vec<int>>);
  CHECK(dp[0] == 1 && dp[2] == 3 && (dp.size() < 4 || dp[3] == 0));

  // conversions
  std::array<double, 4> dd{1.75, -2.25, 3.0, 4.5};
  V c = simd::unchecked_load<V>(dd, simd::flag_convert);
  CHECK(c[0] == 1 && c[1] == -2 && c[3] == 4);
  std::array<short, 4> ss{-1, 2, -3, 4};
  V widened = simd::unchecked_load<V>(ss);  // value-preserving: no flag needed
  CHECK(widened[0] == -1 && widened[2] == -3);

  // stores
  std::array<int, 6> out{};
  simd::unchecked_store(v, out);
  CHECK(out[0] == 1 && out[3] == 4 && out[4] == 0);
  out = {};
  simd::partial_store(v, out.data(), 2);
  CHECK(out[0] == 1 && out[1] == 2 && out[2] == 0);
  out = {};
  simd::partial_store(v, out.begin() + 4, out.end());
  CHECK(out[4] == 1 && out[5] == 2 && out[3] == 0);
  out = {};
  simd::unchecked_store(v, out, even);
  CHECK(out[0] == 1 && out[1] == 0 && out[2] == 3 && out[3] == 0);
  out = {};
  simd::partial_store(v, out.data(), 3, !even);
  CHECK(out[0] == 0 && out[1] == 2 && out[2] == 0 && out[3] == 0);
  out = {};
  simd::unchecked_store(v, out.begin() + 1, 4);
  CHECK(out[0] == 0 && out[1] == 1 && out[4] == 4);
  std::array<long long, 4> wide{};
  simd::unchecked_store(v, wide);  // int -> long long is value-preserving
  CHECK(wide[3] == 4);
  std::array<short, 4> narrow{};
  simd::unchecked_store(V(-7), narrow, simd::flag_convert);
  CHECK(narrow[0] == -7 && narrow[3] == -7);
  return true;
}
static_assert(test());

int main() {
  test();
  // aligned loads and stores
  alignas(simd::alignment_v<V>) int buf[4] = {9, 8, 7, 6};
  V al = simd::unchecked_load<V>(buf, simd::flag_aligned);
  CHECK(al[0] == 9 && al[3] == 6);
  alignas(64) int buf64[4] = {1, 1, 2, 2};
  V ol = simd::unchecked_load<V>(buf64, simd::flag_overaligned<64>);
  CHECK(ol[2] == 2);
  alignas(simd::alignment_v<V, short>) short sbuf[4] = {1, 2, 3, 4};
  V cs = simd::unchecked_load<V>(sbuf, simd::flag_aligned);
  CHECK(cs[3] == 4);
  alignas(simd::alignment_v<V>) int obuf[4] = {};
  simd::unchecked_store(al, obuf, simd::flag_aligned | simd::flag_convert);
  CHECK(obuf[0] == 9 && obuf[3] == 6);
  // spans of dynamic extent, partial at run time
  std::vector<int> vec{1, 2, 3};
  V fromspan = simd::partial_load<V>(std::span<const int>(vec));
  CHECK(fromspan[2] == 3 && fromspan[3] == 0);
  return 0;
}
