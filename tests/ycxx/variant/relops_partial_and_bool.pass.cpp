// [variant.relops]: each relational operator returns false/true by index first and otherwise
// "GET<i>(v) op GET<i>(w)" for the same op ("Mandates: GET<i>(v) op GET<i>(w) is a valid
// expression that is convertible to bool"), so the alternatives' own operators are used, with
// whatever (non-bool) result they return converted to bool, and != is not derived from ==.
// operator<=> (/13): return type common_comparison_category_t<compare_three_way_result_t<
// Types>...>; "if (auto c = v.index() <=> w.index(); c != 0) return c; return GET<i>(v) <=>
// GET<i>(w)" -- so a NaN double alternative gives partial_ordering::unordered (also against
// itself), and different indices order even when the values would be unordered.
#include <cmath>
#include <compare>
#include <type_traits>
#include <variant>
#include "check.hpp"

struct Weird {  // every operator returns a non-bool, and they are mutually inconsistent
  int v;
  friend int operator==(Weird, Weird) { return 2; }
  friend int operator!=(Weird, Weird) { return 0; }
  friend int operator<(Weird a, Weird b) { return a.v < b.v ? 3 : 0; }
  friend int operator>(Weird, Weird) { return 0; }
  friend int operator<=(Weird, Weird) { return 5; }
  friend int operator>=(Weird, Weird) { return 6; }
};

int main() {
  const double nan = std::nan("");
  using V = std::variant<double, int>;
  V a(nan), b(1.0), c(2), d(1.0);
  static_assert(std::is_same_v<decltype(a <=> b), std::partial_ordering>);
  CHECK((a <=> b) == std::partial_ordering::unordered);
  CHECK(!(a < b) && !(a > b) && !(a == b) && (a != b) && !(a <= b) && !(a >= b));
  CHECK((a <=> a) == std::partial_ordering::unordered && !(a == a) && (a != a));
  CHECK((a <=> c) == std::partial_ordering::less && a < c && c > a);  // by index
  CHECK((b <=> d) == std::partial_ordering::equivalent && b == d);
  static_assert(std::is_same_v<decltype(std::variant<int, long>() <=> std::variant<int, long>()), std::strong_ordering>);

  std::variant<Weird> w1(Weird{1}), w2(Weird{2});
  static_assert(std::is_same_v<decltype(w1 == w2), bool> && std::is_same_v<decltype(w1 < w2), bool>);
  CHECK(w1 == w2);
  CHECK(!(w1 != w2));
  CHECK(w1 < w2 && !(w2 < w1));
  CHECK(!(w1 > w2));
  CHECK(w1 <= w2 && w2 <= w1);
  CHECK(w1 >= w2);
  return 0;
}
