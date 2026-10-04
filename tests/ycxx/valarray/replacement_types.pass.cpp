// [valarray.syn]/3: "Any function returning a valarray<T> is permitted to return an object of
// another type, provided all the const member functions of valarray<T> other than begin and end
// are also applicable to this type." /4: "for every function taking a const valarray<T>&,
// identical functions taking the replacement types shall be added; for every function taking
// two const valarray<T>& arguments, identical functions taking every combination of const
// valarray<T>& and replacement types shall be added." /5: "an implementation shall allow a
// valarray<T> to be constructed from such replacement types and shall allow assignments and
// compound assignments of such types to valarray<T>, slice_array<T>, gslice_array<T>,
// mask_array<T> and indirect_array<T> objects." So the results of valarray expressions (written
// here without naming their type) support every const member function, can be passed to every
// valarray function and operator, can be used as masks / index arrays and can be assigned.
#include <valarray>
#include <cstddef>
#include <cmath>
#include "check.hpp"

static int twice(int x) { return 2 * x; }

static bool same(const std::valarray<int>& v, std::initializer_list<int> il) {
  if (v.size() != il.size()) return false;
  std::size_t i = 0;
  for (int x : il)
    if (v[i++] != x) return false;
  return true;
}

int main() {
  const std::valarray<int> a{1, 2, 3, 4};
  const std::valarray<int> b{10, 20, 30, 40};

  // const member functions on expression results
  CHECK((a + b).size() == 4);
  CHECK((a + b)[2] == 33);
  CHECK((a * b).sum() == 10 + 40 + 90 + 160);
  CHECK((b - a).min() == 9 && (b - a).max() == 36);
  CHECK(same((a + b).shift(1), {22, 33, 44, 0}));
  CHECK(same((a + b).cshift(-1), {44, 11, 22, 33}));
  CHECK(same((a + b).apply(twice), {22, 44, 66, 88}));
  CHECK(same((a + b)[std::slice(1, 2, 2)], {22, 44}));
  CHECK(same((a + b)[std::gslice(0, std::valarray<std::size_t>{2}, std::valarray<std::size_t>{3})], {11, 44}));
  CHECK(same((-a)[std::valarray<bool>{true, false, false, true}], {-1, -4}));
  CHECK(same((a * 2)[std::valarray<std::size_t>{3, 0}], {8, 2}));
  CHECK((+a)[0] == 1 && (~a)[0] == ~1);
  CHECK((!(a - a))[1]);

  // functions and operators taking every combination of valarray and replacement types
  CHECK(same((a + b) + (b - a), {20, 40, 60, 80}));
  CHECK(same(a * (b / 10), {1, 4, 9, 16}));
  CHECK(same((b / 10) * a, {1, 4, 9, 16}));
  CHECK(same((a + a) % 3, {2, 1, 0, 2}));
  CHECK(same(100 - (a * 10), {90, 80, 70, 60}));
  CHECK(same((a << 1) | (a & 1), {3, 4, 7, 8}));
  CHECK(same(std::abs(a - b), {9, 18, 27, 36}));
  std::valarray<bool> eq = (a + b) == (b + a);
  CHECK(eq.min() && eq.size() == 4);
  CHECK(((a * 2) > a).min());
  CHECK((a < (a + 1)).min() && ((a + 1) != a).min() && ((a * 1) <= a).min() && (a >= (a - 1)).min());
  CHECK(((a > 1) && (a < 4))[1] && !((a > 1) && (a < 4))[3]);
  CHECK(((a < 2) || (a > 3))[3]);

  const std::valarray<double> x{0.25, 1.0, 4.0};
  CHECK(std::sqrt(x * 4.0)[2] == 4.0);
  CHECK(std::pow(x + x, x - x)[1] == 1.0);  // pow(replacement, replacement)
  CHECK(std::pow(x * 1.0, 2.0)[2] == 16.0);
  CHECK(std::pow(2.0, x * 2.0)[1] == 4.0);
  CHECK(std::atan2(x - x, x)[0] == 0.0);
  CHECK(std::exp(x - x)[0] == 1.0 && std::log(x / x)[1] == 0.0);
  CHECK(std::abs(-(x * 2.0))[2] == 8.0);

  // construction and assignment from replacement types
  std::valarray<int> v(a + b);
  CHECK(same(v, {11, 22, 33, 44}));
  std::valarray<int> w = a * 3;
  CHECK(same(w, {3, 6, 9, 12}));
  w = a + a;
  CHECK(same(w, {2, 4, 6, 8}));
  w += a * 2;
  CHECK(same(w, {4, 8, 12, 16}));
  w -= a + a;
  w *= a - a + 1;
  w /= a / a;
  w %= a + 100;
  w ^= a - a;
  w &= a - a - 1;  // all bits set
  w |= a - a;
  w <<= a - a + 1;
  w >>= a - a + 1;
  CHECK(same(w, {2, 4, 6, 8}));

  std::valarray<int> big(0, 8);
  big[std::slice(0, 4, 2)] = a + b;
  CHECK(same(big, {11, 0, 22, 0, 33, 0, 44, 0}));
  big[std::slice(0, 4, 2)] += a * 0 + 1;
  CHECK(big[0] == 12 && big[6] == 45);
  big[std::gslice(1, std::valarray<std::size_t>{4}, std::valarray<std::size_t>{2})] = -a;
  CHECK(same(big, {12, -1, 23, -2, 34, -3, 45, -4}));
  big[std::gslice(1, std::valarray<std::size_t>{4}, std::valarray<std::size_t>{2})] *= a - a + 2;
  CHECK(big[7] == -8);
  big[big < 0] = a * 0;  // a mask that is itself a replacement-type result
  CHECK(same(big, {12, 0, 23, 0, 34, 0, 45, 0}));
  big[big > 0] -= a + 1;
  CHECK(same(big, {10, 0, 20, 0, 30, 0, 40, 0}));
  std::valarray<std::size_t> idx{7, 5, 3, 1};
  big[idx] = b + a;
  CHECK(same(big, {10, 44, 20, 33, 30, 22, 40, 11}));
  big[idx] /= a - a + 11;
  CHECK(same(big, {10, 4, 20, 3, 30, 2, 40, 1}));
  return 0;
}
