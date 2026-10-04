// [indirect.relops]: indirect == indirect<U, AA> compares the owned objects; with a valueless
// operand it is lhs.valueless_after_move() == rhs.valueless_after_move(); <=> uses
// synth-three-way, valueless ordering below any value. [indirect.comp.with.t]: indirect == U
// is false for a valueless lhs, otherwise *lhs == rhs; <=> with U gives strong_ordering::less
// for a valueless lhs. [indirect.hash]: hash<indirect<T>> is enabled iff hash<T> is, and for a
// non-valueless i gives hash<T>()(*i).
#include <memory>
#include <compare>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct NoHash {};
static_assert(std::is_default_constructible_v<std::hash<std::indirect<int>>>);
static_assert(!std::is_default_constructible_v<std::hash<std::indirect<NoHash>>>);

struct OnlyLess {  // synth-three-way falls back to <
  int v;
  friend bool operator<(const OnlyLess& a, const OnlyLess& b) { return a.v < b.v; }
};

int main() {
  std::indirect<int> a(1), b(2), a2(1);
  CHECK(a == a2 && a != b && a < b && b > a && a <= a2 && b >= a);
  CHECK((a <=> b) == std::strong_ordering::less);
  static_assert(std::is_same_v<decltype(a <=> b), std::strong_ordering>);
  std::indirect<long> l(1);
  CHECK(a == l);  // different value types
  std::indirect<double> d(1.5);
  CHECK((a <=> d) == std::partial_ordering::less);
  static_assert(std::is_same_v<decltype(a <=> d), std::partial_ordering>);

  // With T.
  CHECK(a == 1 && 1 == a && a != 2 && a < 2 && 0 < a);
  CHECK((b <=> 2) == std::strong_ordering::equal);

  // Valueless operands.
  std::indirect<int> v1(5), v2(6);
  std::indirect<int> t1(std::move(v1)), t2(std::move(v2));
  CHECK(v1.valueless_after_move() && v2.valueless_after_move());
  CHECK(v1 == v2);       // both valueless
  CHECK(!(v1 == a) && !(a == v1));
  CHECK(v1 < a && a > v1 && (v1 <=> v2) == 0);
  CHECK(!(v1 == 5));
  CHECK((v1 <=> 5) == std::strong_ordering::less);

  std::indirect<OnlyLess> o1(OnlyLess{1}), o2(OnlyLess{2});
  CHECK((o1 <=> o2) == std::weak_ordering::less);
  static_assert(std::is_same_v<decltype(o1 <=> o2), std::weak_ordering>);

  std::indirect<std::string> s(std::string("key"));
  CHECK(std::hash<std::indirect<std::string>>()(s) == std::hash<std::string>()("key"));
  CHECK(std::hash<std::indirect<int>>()(a) == std::hash<int>()(1));
  (void)std::hash<std::indirect<int>>()(v1);  // valueless: an implementation-defined value
  return 0;
}
