// [optional.relops]: x == y, x != y, x < y, ... with both engaged return *x op *y for the same
// op ("Mandates: The expression *x op *y is well-formed and its result is convertible to
// bool"); [optional.comp.with.t] likewise against a T. operator<=> (/19): "If x && y,
// *x <=> *y; otherwise x.has_value() <=> y.has_value()" (result compare_three_way_result_t<T, U>).
// With NaN: unordered, and comparisons are false (except !=). An empty optional is less than
// any engaged one, NaN included. Non-bool results of the value type's operators are converted.
#include <cmath>
#include <compare>
#include <optional>
#include <type_traits>
#include "check.hpp"

struct Weird {
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
  std::optional<double> on(nan), o1(1.0), oe;
  std::optional<int> oi(1);
  static_assert(std::is_same_v<decltype(on <=> o1), std::partial_ordering>);
  CHECK((on <=> o1) == std::partial_ordering::unordered);
  CHECK((on <=> on) == std::partial_ordering::unordered && !(on == on) && (on != on));
  CHECK((oe <=> on) < 0 && oe < on && !(on < oe));
  CHECK((on <=> 1.0) == std::partial_ordering::unordered && !(on == nan) && !(on < 1.0) && !(on >= 1.0));
  CHECK((oi <=> o1) == std::partial_ordering::equivalent && oi == o1);

  std::optional<Weird> w1(Weird{1}), w2(Weird{2});
  static_assert(std::is_same_v<decltype(w1 == w2), bool> && std::is_same_v<decltype(w1 < Weird{}), bool>);
  CHECK(w1 == w2 && !(w1 != w2) && w1 < w2 && !(w2 < w1) && !(w1 > w2) && w1 <= w2 && w1 >= w2);
  CHECK(w1 == Weird{5} && !(w1 != Weird{5}) && w1 <= Weird{0} && !(Weird{0} > w1));
  CHECK(Weird{9} < w1 == false && Weird{0} < w1);
  return 0;
}
