// [valarray.members]/12-15: shift(n): element I is (*this)[I + n] if I + n is in [0, size()),
// otherwise T(); cshift(n): circular shift, left for non-negative n and right for negative n;
// apply(func) for T func(T) and T func(const T&) applies func to every element.
#include <valarray>
#include <initializer_list>
#include "check.hpp"

template <class V, class T>
bool eq(const V& v, std::initializer_list<T> il) {
  if (v.size() != il.size()) return false;
  std::size_t i = 0;
  for (T x : il)
    if (v[i++] != x) return false;
  return true;
}

int twice(int x) { return 2 * x; }
int negate(const int& x) { return -x; }

int main() {
  const std::valarray<int> v{1, 2, 3, 4, 5};
  CHECK(eq(v.shift(0), {1, 2, 3, 4, 5}));
  CHECK(eq(v.shift(2), {3, 4, 5, 0, 0}));
  CHECK(eq(v.shift(-2), {0, 0, 1, 2, 3}));
  CHECK(eq(v.shift(5), {0, 0, 0, 0, 0}));
  CHECK(eq(v.shift(-7), {0, 0, 0, 0, 0}));
  CHECK(eq(v.cshift(0), {1, 2, 3, 4, 5}));
  CHECK(eq(v.cshift(2), {3, 4, 5, 1, 2}));
  CHECK(eq(v.cshift(-2), {4, 5, 1, 2, 3}));
  CHECK(eq(v.cshift(5), {1, 2, 3, 4, 5}));
  CHECK(eq(v.cshift(7), {3, 4, 5, 1, 2}));
  CHECK(eq(v.cshift(-6), {5, 1, 2, 3, 4}));
  CHECK(std::valarray<int>().shift(3).size() == 0);
  CHECK(std::valarray<int>().cshift(-3).size() == 0);
  std::valarray<double> d{1.5, 2.5};
  CHECK(eq(d.shift(1), {2.5, 0.0}));

  CHECK(eq(v.apply(twice), {2, 4, 6, 8, 10}));
  CHECK(eq(v.apply(negate), {-1, -2, -3, -4, -5}));
  CHECK(eq(v.apply([](int x) { return x * x; }), {1, 4, 9, 16, 25}));  // converts to int(*)(int)
  CHECK(eq(v, {1, 2, 3, 4, 5}));  // const members leave *this unchanged
  return 0;
}
