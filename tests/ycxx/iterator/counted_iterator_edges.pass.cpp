// [counted.iter.nav]: operator+(n), operator-(n), +=, -= with negative n move both the
// iterator and the count ("counted_iterator(current + n, length - n)" /6, "current -= n;
// length += n;" /16). /11-12: x - y for counted_iterator<I> and counted_iterator<I2>
// (common_with) "return y.length - x.length" with type iter_difference_t<I2>; /13-14:
// x - default_sentinel is -x.length, default_sentinel - x is x.length, both noexcept.
// [counted.iter.cmp]/2,5: == and <=> between counted_iterator<int*> and
// counted_iterator<const int*> compare the lengths (reversed for <=>, strong_ordering).
// [counted.iter.elem]/5: operator[](n) is current[n]. /4 (nav): operator++(int) for a
// non-forward I is "--length; try { return current++; } catch(...) { ++length; throw; }" --
// so the result type is decltype(current++) and a throwing increment leaves count()
// unchanged. [range.iter.op.advance]/4.2: ranges::next(ci, default_sentinel) uses the sized
// sentinel to jump straight to count 0; [range.iter.op.distance]/3: distance is the count.
// REQUIRES: exceptions
#include <iterator>
#include <compare>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

using CI = std::counted_iterator<int*>;
using CCI = std::counted_iterator<const int*>;

constexpr bool arith() {
  int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  CI it(a + 5, 3);
  CI back = it + (-2);
  if (back.base() != a + 3 || back.count() != 5) return false;
  CI fwd = it - (-2);
  if (fwd.base() != a + 7 || fwd.count() != 1) return false;
  CI x = it;
  x -= -3;
  if (x.base() != a + 8 || x.count() != 0 || x != std::default_sentinel) return false;
  x += -4;
  if (x.base() != a + 4 || x.count() != 4) return false;
  CI y = (-1) + it;
  if (y.base() != a + 4 || y.count() != 4 || y != x) return false;
  if (it[-5] != 0 || it[2] != 7) return false;
  if (x - it != -1 || it - x != 1) return false;  // x is one before it
  if (it - std::default_sentinel != -3 || std::default_sentinel - it != 3) return false;
  // Mixed with counted_iterator<const int*>.
  CCI c(static_cast<const int*>(a + 6), 2);
  if (!(c - it == 1) || !(it - c == -1)) return false;
  static_assert(std::is_same_v<decltype(it - c), std::iter_difference_t<const int*>>);
  if (it == c || !(it < c) || !(c > it) || (it <=> c) != std::strong_ordering::less) return false;
  CCI c2(static_cast<const int*>(a + 5), 3);
  if (!(it == c2) || (c2 <=> it) != 0) return false;
  // Decrement before the original start is fine (the count grows).
  CI z(a + 2, 0);
  --z;
  --z;
  if (z.base() != a || z.count() != 2 || *z != 0) return false;
  return true;
}

constexpr bool cpo() {
  int a[8] = {};
  CI it(a + 2, 5);
  CI e = std::ranges::next(it, std::default_sentinel);
  if (e.base() != a + 7 || e.count() != 0) return false;
  if (std::ranges::distance(it, std::default_sentinel) != 5) return false;
  CI f = it;
  if (std::ranges::advance(f, 9, std::default_sentinel) != 4 || f.count() != 0) return false;
  // n < 0 with bound of the same type before i: [range.iter.op.advance]/6.1.
  CI g = e;
  if (std::ranges::advance(g, -2, it) != 0 || g.base() != a + 5 || g.count() != 2) return false;
  g = e;
  if (std::ranges::advance(g, -9, it) != -4 || g.base() != a + 2 || g.count() != 5) return false;
  return true;
}
static_assert(arith());
static_assert(cpo());
static_assert(noexcept(std::declval<const CI&>() - std::default_sentinel));
static_assert(noexcept(std::default_sentinel - std::declval<const CI&>()));
static_assert(std::is_same_v<decltype(std::declval<CI&>() <=> std::declval<CCI&>()), std::strong_ordering>);

// Non-forward iterator whose post-increment may throw and returns a proxy type.
struct Token { int v; };
struct ThrowIn {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  bool* armed;
  int& operator*() const { return *p; }
  ThrowIn& operator++() { ++p; return *this; }
  Token operator++(int) {
    if (*armed) throw std::runtime_error("increment");
    return Token{*p++};
  }
};
static_assert(std::input_iterator<ThrowIn> && !std::forward_iterator<ThrowIn>);

int main() {
  CHECK(arith() && cpo());
  int a[4] = {1, 2, 3, 4};
  bool armed = false;
  std::counted_iterator<ThrowIn> it(ThrowIn{a, &armed}, 4);
  static_assert(std::is_same_v<decltype(it++), Token>);
  Token t = it++;
  CHECK(t.v == 1 && it.count() == 3 && *it == 2);
  armed = true;
  bool threw = false;
  try {
    (void)it++;
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CHECK(threw && it.count() == 3);
  armed = false;
  (void)it++;
  CHECK(it.count() == 2 && *it == 3);
  return 0;
}
