// [valarray.cons], [valarray.assign], [valarray.access], [valarray.members]: construction from
// a size (value-initialized), a value and a count, a pointer and a count, an initializer list,
// copy/move; the deduction guide valarray(const T(&)[cnt], size_t) -> valarray<T>; assignment
// from a valarray, an initializer list and a scalar (fills); size, operator[], swap, sum, min,
// max, and resize(sz, c) ("Changes the length ... and then assigns to each element the value
// of the second argument").
#include <valarray>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<std::valarray<int>::value_type, int>);
static_assert(std::is_nothrow_move_constructible_v<std::valarray<int>>);
static_assert(std::is_nothrow_move_assignable_v<std::valarray<int>>);
static_assert(!std::is_convertible_v<std::size_t, std::valarray<int>>);  // explicit valarray(size_t)
static_assert(noexcept(std::declval<std::valarray<int>&>().swap(std::declval<std::valarray<int>&>())));

int main() {
  std::valarray<int> e;
  CHECK(e.size() == 0);
  std::valarray<double> z(4);
  CHECK(z.size() == 4 && z[0] == 0 && z[3] == 0);
  std::valarray<int> f(7, 3);
  CHECK(f.size() == 3 && f[0] == 7 && f[2] == 7);
  const int raw[] = {5, 1, 4, 2, 3};
  std::valarray<int> p(raw, 4);
  CHECK(p.size() == 4 && p[0] == 5 && p[3] == 2);
  std::valarray g(raw, 5);  // deduction guide
  static_assert(std::is_same_v<decltype(g), std::valarray<int>>);
  CHECK(g.size() == 5 && g[4] == 3);
  std::valarray<int> il{1, 2, 3};
  CHECK(il.size() == 3 && il[1] == 2);
  std::valarray<int> c(il);
  CHECK(c.size() == 3 && c[2] == 3);
  std::valarray<int> m(std::move(c));
  CHECK(m.size() == 3 && m[0] == 1);

  // sum/min/max
  CHECK(g.sum() == 15 && g.min() == 1 && g.max() == 5);
  std::valarray<double> one{2.5};
  CHECK(one.sum() == 2.5 && one.min() == 2.5 && one.max() == 2.5);

  // Assignment.
  std::valarray<int> a(3);
  a = {9, 8, 7};
  CHECK(a.size() == 3 && a[0] == 9 && a[2] == 7);
  a = 4;  // operator=(const T&)
  CHECK(a.size() == 3 && a[0] == 4 && a[1] == 4 && a[2] == 4);
  std::valarray<int> b(3);
  b = a;
  CHECK(b[1] == 4);
  b = std::valarray<int>{1, 2, 3};
  CHECK(b.size() == 3 && b[2] == 3);
  std::valarray<int> d;
  d = std::valarray<int>{6, 6};  // move assignment may change the size
  CHECK(d.size() == 2 && d[1] == 6);

  // Element access returns references.
  a[1] = 42;
  CHECK(a[1] == 42);
  const std::valarray<int>& ca = a;
  static_assert(std::is_same_v<decltype(ca[0]), const int&>);
  static_assert(std::is_same_v<decltype(a[0]), int&>);
  CHECK(&a[1] == &a[0] + 1);

  // swap
  std::valarray<int> s1{1, 2}, s2{3, 4, 5};
  s1.swap(s2);
  CHECK(s1.size() == 3 && s1[2] == 5 && s2.size() == 2 && s2[0] == 1);
  swap(s1, s2);  // non-member swap
  CHECK(s1.size() == 2 && s2.size() == 3);

  // resize assigns c to every element, including the old ones.
  std::valarray<int> r{1, 2, 3};
  r.resize(5, 9);
  CHECK(r.size() == 5 && r[0] == 9 && r[2] == 9 && r[4] == 9);
  r.resize(2);
  CHECK(r.size() == 2 && r[0] == 0 && r[1] == 0);
  r.resize(0);
  CHECK(r.size() == 0);
  return 0;
}
