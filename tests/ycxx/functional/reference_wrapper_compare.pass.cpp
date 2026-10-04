// [refwrap.comparisons] (C++26, P2944): friend operator== and operator<=> taking
// (reference_wrapper, reference_wrapper), (reference_wrapper, const T&) and
// (reference_wrapper, reference_wrapper<const T>), returning x.get() == y.get() and
// synth-three-way(x.get(), y.get()); each constrained on the underlying expression.
#include <functional>
#include <compare>
#include <type_traits>
#include "check.hpp"

struct NoEq {};
struct OnlyLess {
  int v;
  friend constexpr bool operator<(OnlyLess a, OnlyLess b) { return a.v < b.v; }
  friend constexpr bool operator==(OnlyLess a, OnlyLess b) { return a.v == b.v; }
};

template <class A, class B>
concept eq_comparable = requires(A a, B b) { a == b; };
template <class A, class B>
concept three_way = requires(A a, B b) { a <=> b; };

using RI = std::reference_wrapper<int>;
using RCI = std::reference_wrapper<const int>;
static_assert(eq_comparable<RI, RI>);
static_assert(eq_comparable<RI, int>);
static_assert(eq_comparable<RI, RCI>);
static_assert(eq_comparable<RCI, RI>);
static_assert(!eq_comparable<std::reference_wrapper<NoEq>, std::reference_wrapper<NoEq>>);
static_assert(!eq_comparable<std::reference_wrapper<NoEq>, NoEq>);
static_assert(!three_way<std::reference_wrapper<NoEq>, std::reference_wrapper<NoEq>>);
static_assert(std::is_same_v<decltype(std::declval<RI>() <=> std::declval<RI>()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::reference_wrapper<double>>() <=> 1.0), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::reference_wrapper<OnlyLess>>() <=>
                                      std::declval<std::reference_wrapper<OnlyLess>>()),
                             std::weak_ordering>);
static_assert(std::is_same_v<decltype(std::declval<RI>() == std::declval<RI>()), bool>);

constexpr bool test() {
  int a = 1, b = 2, a2 = 1;
  const int cb = 2;
  RI ra(a), rb(b), ra2(a2);
  RCI rcb(cb);
  if (!(ra == ra2) || ra == rb || !(ra != rb)) return false;  // compares values, not addresses
  if (!(ra == 1) || !(1 == ra) || ra == 2) return false;
  if (!(rb == rcb) || !(rcb == rb) || ra == rcb) return false;
  if ((ra <=> rb) != std::strong_ordering::less || !(rb > ra) || !(ra <= ra2)) return false;
  if ((ra <=> 0) != std::strong_ordering::greater || !(0 < ra)) return false;
  if ((ra <=> rcb) != std::strong_ordering::less || !(rcb > ra)) return false;
  OnlyLess x{1}, y{2};
  std::reference_wrapper rx(x), ry(y);
  if ((rx <=> ry) != std::weak_ordering::less || !(rx < ry)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
