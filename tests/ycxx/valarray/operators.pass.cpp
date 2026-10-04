// [valarray.unary], [valarray.cassign], [valarray.binary], [valarray.comparison],
// [valarray.transcend]: element-wise unary operators (!, returning valarray<bool>), compound
// assignment with a scalar and with a valarray, binary operators valarray op valarray,
// valarray op T and T op valarray (T deduced from the valarray: "type_identity_t<T>" scalar
// parameter), comparisons returning valarray<bool>, logical && and ||, and abs, exp, sqrt, pow,
// atan2, ... element-wise.
#include <valarray>
#include <cmath>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

// [valarray.syn]/3: results may be replacement types offering valarray's const members, so the
// helper only uses size() and operator[].
template <class V, class T>
bool eq(const V& v, std::initializer_list<T> il) {
  if (v.size() != il.size()) return false;
  std::size_t i = 0;
  for (T x : il)
    if (v[i++] != x) return false;
  return true;
}

int main() {
  std::valarray<int> a{1, 2, 3, 4}, b{4, 3, 2, 1};
  CHECK(eq(+a, {1, 2, 3, 4}) && eq(-a, {-1, -2, -3, -4}) && eq(~a, {~1, ~2, ~3, ~4}));
  std::valarray<bool> n = !std::valarray<int>{0, 1, 0};
  CHECK(eq(n, {true, false, true}));

  CHECK(eq(a + b, {5, 5, 5, 5}) && eq(a - b, {-3, -1, 1, 3}) && eq(a * b, {4, 6, 6, 4}));
  CHECK(eq(b / a, {4, 1, 0, 0}) && eq(b % a, {0, 1, 2, 1}));
  CHECK(eq(a & b, {0, 2, 2, 0}) && eq(a | b, {5, 3, 3, 5}) && eq(a ^ b, {5, 1, 1, 5}));
  CHECK(eq(a << std::valarray<int>{1, 1, 2, 2}, {2, 4, 12, 16}) && eq(a >> 1, {0, 1, 1, 2}));
  CHECK(eq(a + 10, {11, 12, 13, 14}) && eq(10 - a, {9, 8, 7, 6}) && eq(2 * a, {2, 4, 6, 8}));
  CHECK(eq(12 / a, {12, 6, 4, 3}) && eq(a % 2, {1, 0, 1, 0}) && eq(1 << a, {2, 4, 8, 16}));

  // The scalar is not used for deduction (it converts to T).
  std::valarray<double> d{1.0, 2.0};
  CHECK(eq(d * 2, {2.0, 4.0}) && eq(1 + d, {2.0, 3.0}));

  std::valarray<int> c = a;
  c += 1;
  CHECK(eq(c, {2, 3, 4, 5}));
  c -= b;
  CHECK(eq(c, {-2, 0, 2, 4}));
  c *= 3;
  CHECK(eq(c, {-6, 0, 6, 12}));
  c /= std::valarray<int>{2, 1, 3, 4};
  CHECK(eq(c, {-3, 0, 2, 3}));
  c %= 2;
  CHECK(eq(c, {-1, 0, 0, 1}));
  c |= 6;
  CHECK(eq(c, {-1, 6, 6, 7}));
  c &= std::valarray<int>{1, 2, 4, 8};
  CHECK(eq(c, {1, 2, 4, 0}));
  c ^= 1;
  CHECK(eq(c, {0, 3, 5, 1}));
  c <<= 2;
  CHECK(eq(c, {0, 12, 20, 4}));
  c >>= std::valarray<int>{0, 1, 2, 2};
  CHECK(eq(c, {0, 6, 5, 1}));
  static_assert(std::is_same_v<decltype(c += 1), std::valarray<int>&>);

  // Comparisons and logical operators give valarray<bool>.
  std::valarray<bool> cmp = a == b;  // convertible to valarray<bool>
  CHECK(cmp.size() == 4);
  CHECK(eq(a == b, {false, false, false, false}) && eq(a != b, {true, true, true, true}));
  CHECK(eq(a < b, {true, true, false, false}) && eq(a >= b, {false, false, true, true}));
  CHECK(eq(a <= 2, {true, true, false, false}) && eq(3 > a, {true, true, false, false}));
  CHECK(eq(a == 3, {false, false, true, false}) && eq(2 != a, {true, false, true, true}));
  std::valarray<int> z{0, 1, 0, 1};
  CHECK(eq(z && a, {false, true, false, true}) && eq(z || std::valarray<int>{0, 0, 1, 1}, {false, true, true, true}));
  CHECK(eq(z && 1, {false, true, false, true}) && eq(0 || z, {false, true, false, true}));

  // Transcendentals.
  std::valarray<double> x{-4.0, 9.0};
  CHECK(eq(std::abs(x), {4.0, 9.0}));
  std::valarray<double> s = std::sqrt(std::valarray<double>{4.0, 9.0});
  CHECK(eq(s, {2.0, 3.0}));
  CHECK(eq(std::pow(std::valarray<double>{2.0, 3.0}, 2.0), {4.0, 9.0}));
  CHECK(eq(std::pow(2.0, std::valarray<double>{1.0, 3.0}), {2.0, 8.0}));
  CHECK(eq(std::pow(std::valarray<double>{2.0, 4.0}, std::valarray<double>{3.0, 0.5}), {8.0, 2.0}));
  std::valarray<double> at = std::atan2(std::valarray<double>{0.0, 1.0}, 1.0);
  CHECK(at[0] == 0.0 && at[1] == std::atan2(1.0, 1.0));
  std::valarray<double> ex = std::exp(std::valarray<double>{0.0});
  CHECK(ex[0] == 1.0);
  CHECK(std::log(std::valarray<double>{1.0})[0] == 0.0 && std::cos(std::valarray<double>{0.0})[0] == 1.0);
  CHECK(std::sin(x)[1] == std::sin(9.0) && std::tanh(x)[0] == std::tanh(-4.0));
  CHECK(std::log10(std::valarray<double>{100.0})[0] == 2.0);
  return 0;
}
