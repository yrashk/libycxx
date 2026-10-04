// [reverse.iter.cmp]: operator== compares x.base() == y.base(); operator< is x.base() >
// y.base(); operator> is x.base() < y.base(); <= is x.base() >= y.base(); >= is x.base() <=
// y.base(); each "Constraints: x.base() OP y.base() is well-formed and convertible to bool."
// operator<=> (constrained on three_way_comparable_with) returns y.base() <=> x.base().
#include <iterator>
#include <compare>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

// Bidirectional iterator with only == (no ordering)
struct Bidi {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  int& operator*() const { return *p; }
  Bidi& operator++() { ++p; return *this; }
  Bidi operator++(int) { auto t = *this; ++p; return t; }
  Bidi& operator--() { --p; return *this; }
  Bidi operator--(int) { auto t = *this; --p; return t; }
  bool operator==(const Bidi&) const = default;
};

template <class A, class B>
concept less_comparable = requires(A a, B b) { a < b; };
template <class A, class B>
concept spaceship = requires(A a, B b) { a <=> b; };

using RB = std::reverse_iterator<Bidi>;
static_assert(!less_comparable<RB, RB>);
static_assert(!spaceship<RB, RB>);
static_assert(std::equality_comparable<RB>);
using R = std::reverse_iterator<int*>;
using CR = std::reverse_iterator<const int*>;
static_assert(std::is_same_v<decltype(R() <=> R()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(R() < CR()), bool>);

constexpr bool test() {
  int a[4] = {};
  R r1(a + 4), r2(a + 2);
  CR c2(a + 2);
  // r1 is "before" r2 in reverse order
  if (!(r1 < r2) || r2 < r1 || !(r2 > r1) || !(r1 <= r2) || !(r2 >= r1)) return false;
  if (!(r2 == c2) || r1 == c2 || !(r1 != c2)) return false;
  if (!(r1 < c2) || !(c2 > r1)) return false;
  if ((r1 <=> r2) != std::strong_ordering::less || (r2 <=> r1) != std::strong_ordering::greater) return false;
  if ((r2 <=> c2) != std::strong_ordering::equal) return false;
  if (c2 - r1 != 2) return false;  // heterogeneous difference: y.base() - x.base()
  Bidi b{a};
  RB rb1(b), rb2(b);
  if (!(rb1 == rb2)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
