// [simd.ctor]: broadcast, converting, generator (/10: invoked exactly once per index, in
// increasing order) and range constructors; [simd.overview]/1.6: value-initialization zeroes.
// [simd.subscr]: operator[]; [simd.iterator]: begin()/end() traverse the elements, iterator
// arithmetic and comparison with default_sentinel.
// [simd.unary]: ++ -- ! ~ + -; [simd.binary]: element-wise + - * / % & | ^ << >>, and << >> by a
// scalar simd-size-type; [simd.cassign]; [simd.comparison]: comparisons return mask_type.
// [simd.alg]: min, max, minmax, clamp, select (bool and mask).
// All members are constexpr.
// COUNTERPART: libcxx:experimental/simd/simd.class/.*
#include <simd>
#include <array>
#include <iterator>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<int, 5>;
using M = V::mask_type;

constexpr bool equal(const V& v, std::array<int, 5> a) {
  for (int i = 0; i < 5; ++i)
    if (v[i] != a[i]) return false;
  return true;
}
constexpr bool equal(const V& v, const V& w) {
  for (int i = 0; i < 5; ++i)
    if (v[i] != w[i]) return false;
  return true;
}
constexpr bool equal(const M& m, std::array<bool, 5> a) {
  for (int i = 0; i < 5; ++i)
    if (m[i] != a[i]) return false;
  return true;
}

constexpr bool test() {
  V zero{};
  CHECK(equal(zero, {0, 0, 0, 0, 0}));
  V b = 7;
  CHECK(equal(b, {7, 7, 7, 7, 7}));
  V g([](int i) { return i * 10; });
  CHECK(equal(g, {0, 10, 20, 30, 40}));
  V gi([](auto i) { return decltype(i)::value + 1; });  // integral_constant argument
  CHECK(equal(gi, {1, 2, 3, 4, 5}));
  static_assert(std::is_same_v<decltype(g[0]), int>);

  // iterators
  int sum = 0;
  for (int x : g) sum += x;
  CHECK(sum == 100);
  auto it = g.begin();
  CHECK(*it == 0 && it[3] == 30 && *(it + 2) == 20 && *(2 + it) == 20);
  ++it;
  it += 2;
  CHECK(*it == 30 && it - g.begin() == 3 && g.end() - it == 2 && it - g.end() == -2);
  CHECK(it > g.begin() && (it <=> g.begin()) > 0);
  it += 2;
  CHECK(it == g.end());
  --it;
  CHECK(*it-- == 40 && *it == 30);
  V::const_iterator cit = g.begin();
  CHECK(*cit == 0 && cit == g.cbegin());
  CHECK(std::ranges::distance(g) == 5);

  // unary
  V u = g;
  CHECK(equal(++u, {1, 11, 21, 31, 41}));
  CHECK(equal(u++, {1, 11, 21, 31, 41}) && equal(u, {2, 12, 22, 32, 42}));
  CHECK(equal(--u, {1, 11, 21, 31, 41}));
  CHECK(equal(u--, {1, 11, 21, 31, 41}) && equal(u, g));
  CHECK(equal(-g, {0, -10, -20, -30, -40}) && equal(+g, g));
  CHECK(equal(~V(0), {-1, -1, -1, -1, -1}));
  CHECK(equal(!g, {true, false, false, false, false}));

  // binary
  V a([](int i) { return i + 1; });  // 1 2 3 4 5
  CHECK(equal(a + g, {1, 12, 23, 34, 45}));
  CHECK(equal(g - a, {-1, 8, 17, 26, 35}));
  CHECK(equal(a * a, {1, 4, 9, 16, 25}));
  CHECK(equal(g / a, {0, 5, 6, 7, 8}));
  CHECK(equal(g % a, {0, 0, 2, 2, 0}));
  CHECK(equal(a & 6, {0, 2, 2, 4, 4}) && equal(a | 8, {9, 10, 11, 12, 13}) && equal(a ^ 1, {0, 3, 2, 5, 4}));
  CHECK(equal(a << a, {2, 8, 24, 64, 160}) && equal(g >> a, {0, 2, 2, 1, 1}));
  CHECK(equal(a << 2, {4, 8, 12, 16, 20}) && equal(g >> 1, {0, 5, 10, 15, 20}));
  CHECK(equal(2 + a, {3, 4, 5, 6, 7}) && equal(a * 3, {3, 6, 9, 12, 15}));  // broadcast operands

  // compound assignment
  V c = a;
  c += a;
  CHECK(equal(c, {2, 4, 6, 8, 10}));
  c -= 1;
  c *= 2;
  CHECK(equal(c, {2, 6, 10, 14, 18}));
  c /= a;
  CHECK(equal(c, {2, 3, 3, 3, 3}));
  c %= 2;
  CHECK(equal(c, {0, 1, 1, 1, 1}));
  c |= 4;
  c &= 5;
  c ^= 1;
  CHECK(equal(c, {5, 4, 4, 4, 4}));
  c <<= 1;
  CHECK(equal(c, {10, 8, 8, 8, 8}));
  c >>= a;
  CHECK(equal(c, {5, 2, 1, 0, 0}));
  CHECK(&(c += 1) == &c);

  // comparisons
  static_assert(std::is_same_v<decltype(a == g), M>);
  CHECK(equal(a == V(3), {false, false, true, false, false}));
  CHECK(equal(a != 3, {true, true, false, true, true}));
  CHECK(equal(a < 3, {true, true, false, false, false}) && equal(a <= 3, {true, true, true, false, false}));
  CHECK(equal(a > 3, {false, false, false, true, true}) && equal(a >= 3, {false, false, true, true, true}));

  // algorithms
  V r([](int i) { return 4 - i * 2; });  // 4 2 0 -2 -4
  CHECK(equal(simd::min(a, r), {1, 2, 0, -2, -4}) && equal(simd::max(a, r), {4, 2, 3, 4, 5}));
  auto [mn, mx] = simd::minmax(a, r);
  CHECK(equal(mn, {1, 2, 0, -2, -4}) && equal(mx, {4, 2, 3, 4, 5}));
  CHECK(equal(simd::clamp(r, V(-1), V(3)), {3, 2, 0, -1, -1}));
  CHECK(equal(std::min(a, r), {1, 2, 0, -2, -4}));  // also found as std::min
  CHECK(equal(simd::select(a > 2, a, r), {4, 2, 3, 4, 5}));
  CHECK(equal(simd::select(a > 2, 100, r), {4, 2, 100, 100, 100}));
  CHECK(simd::select(true, 1, 2) == 1 && simd::select(false, 1, 2.5) == 2.5);
  return true;
}

static_assert(test());

int main() {
  test();
  // The generator is invoked once per index, in increasing order.
  int order[8] = {};
  int calls = 0;
  simd::vec<float, 8> v([&](int i) {
    order[calls++] = i;
    return float(i);
  });
  CHECK(calls == 8);
  for (int i = 0; i < 8; ++i) CHECK(order[i] == i && v[i] == float(i));

  // converting constructor and widths other than the native one
  simd::vec<short, 5> s([](int i) { return short(i - 2); });
  V w = s;
  CHECK(w[0] == -2 && w[4] == 2);
  simd::vec<short, 5> back(w);
  CHECK(back[1] == -1);
  simd::vec<double, 5> d = V(3);
  CHECK(d[2] == 3.0);
  simd::vec<float, 5> f(d);  // explicit narrowing
  CHECK(f[4] == 3.0f);

  // range constructor
  std::array<int, 5> arr{5, 4, 3, 2, 1};
  V fromr(arr);
  CHECK(fromr[0] == 5 && fromr[4] == 1);
  std::array<double, 5> darr{1.5, 2.5, 3.5, 4.5, 5.5};
  V conv(darr, simd::flag_convert);
  CHECK(conv[0] == 1 && conv[4] == 5);
  return 0;
}
