// [optional.relops]: optional<T> vs optional<U> comparisons (each uses the same operator of
// the contained types, constrained on validity), <=> with three_way_comparable_with.
// [optional.nullops]: == and <=> with nullopt, noexcept, returning bool / strong_ordering.
// [optional.comp.with.t]: comparisons with a value; constraints exclude U that is an optional;
// <=> requires !is-derived-from-optional<U>.
#include <optional>
#include <compare>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Weird {
  int v;
  constexpr friend bool operator==(Weird, Weird) { return true; }
  constexpr friend bool operator!=(Weird, Weird) { return true; }
  constexpr friend bool operator<(Weird, Weird) { return true; }
  constexpr friend bool operator>(Weird, Weird) { return true; }
  constexpr friend bool operator<=(Weird, Weird) { return false; }
  constexpr friend bool operator>=(Weird, Weird) { return false; }
};
struct OnlyEq { constexpr friend bool operator==(OnlyEq, OnlyEq) { return true; } };

template <class A, class B> concept eq = requires(const A& a, const B& b) { a == b; };
template <class A, class B> concept lt = requires(const A& a, const B& b) { a < b; };
template <class A, class B> concept ss = requires(const A& a, const B& b) { a <=> b; };

static_assert(eq<std::optional<OnlyEq>, std::optional<OnlyEq>>);
static_assert(!lt<std::optional<OnlyEq>, std::optional<OnlyEq>>);
static_assert(!ss<std::optional<OnlyEq>, std::optional<OnlyEq>>);
static_assert(!lt<std::optional<OnlyEq>, OnlyEq>);
static_assert(eq<std::optional<OnlyEq>, OnlyEq>);
static_assert(ss<std::optional<OnlyEq>, std::nullopt_t>);
static_assert(std::is_same_v<decltype(std::optional<int>() <=> std::optional<long>()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::optional<int>() <=> std::optional<double>()), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::optional<double>() <=> 1), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::optional<OnlyEq>() <=> std::nullopt), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::optional<OnlyEq>() == std::nullopt), bool>);
static_assert(noexcept(std::optional<OnlyEq>() == std::nullopt));
static_assert(noexcept(std::optional<OnlyEq>() <=> std::nullopt));
static_assert(noexcept(std::nullopt == std::optional<OnlyEq>()));

// A type derived from optional is compared as an optional, not as a "value"
struct DerivedOpt : std::optional<int> { using std::optional<int>::optional; };

constexpr bool test() {
  std::optional<int> e, one(1), two(2);
  std::optional<long> lone(1);
  // optional vs optional
  if (!(e == e) || e != e || e == one || !(one == lone) || one != lone) return false;
  if (!(e < one) || one < e || e < e || !(one < two)) return false;
  if (!(one > e) || e > one || !(two > one)) return false;
  if (!(e <= e) || !(e <= one) || one <= e) return false;
  if (!(e >= e) || !(one >= e) || e >= one) return false;
  if ((e <=> one) != std::strong_ordering::less || (one <=> e) != std::strong_ordering::greater) return false;
  if ((e <=> e) != std::strong_ordering::equal || (two <=> lone) != std::strong_ordering::greater) return false;
  // nullopt
  if (!(e == std::nullopt) || one == std::nullopt || !(std::nullopt == e) || !(one != std::nullopt)) return false;
  if ((one <=> std::nullopt) != std::strong_ordering::greater) return false;
  if ((e <=> std::nullopt) != std::strong_ordering::equal) return false;
  if (!(std::nullopt < one) || one < std::nullopt || !(e <= std::nullopt) || !(std::nullopt >= e)) return false;
  // value
  if (!(one == 1) || !(1 == one) || e == 1 || 1 == e || !(e != 1) || !(1 != e)) return false;
  if (!(e < 1) || 1 < e || !(one < 2) || !(0 < one)) return false;
  if (e > 1 || !(1 > e) || !(two > 1) || !(3 > two)) return false;
  if (!(e <= 1) || 1 <= e || !(one <= 1) || !(1 <= one)) return false;
  if (e >= 1 || !(1 >= e) || !(one >= 1) || !(1 >= one)) return false;
  if ((e <=> 5) != std::strong_ordering::less || (two <=> 1L) != std::strong_ordering::greater) return false;
  if ((5 <=> e) != std::strong_ordering::greater) return false;  // synthesized reverse
  // same operator is used, not synthesized from another
  std::optional<Weird> w1(Weird{1}), w2(Weird{2}), we;
  if (!(w1 == w2) || !(w1 != w2) || !(w1 < w2) || !(w1 > w2) || w1 <= w2 || w1 >= w2) return false;
  if (!(w1 == Weird{3}) || !(w1 != Weird{3}) || !(w1 < Weird{3}) || !(Weird{3} > w1)) return false;
  if (w1 <= Weird{3} || Weird{3} >= w1) return false;
  if (we == w1 || !(we != w1) || !(we < w1) || we > w1) return false;
  // derived-from-optional on the right-hand side of <=> uses the optional/optional overload
  DerivedOpt d(1);
  if ((one <=> d) != std::strong_ordering::equal) return false;
  if (!(one == d)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
