// [pairs.spec]/1-2: operator==(const pair<T1, T2>& x, const pair<U1, U2>& y) "Returns: x.first
// == y.first && x.second == y.second." /3: operator<=> returns
// common_comparison_category_t<synth-three-way-result<T1, U1>, synth-three-way-result<T2, U2>>
// and is "Equivalent to: if (auto c = synth-three-way(x.first, y.first); c != 0) return c;
// return synth-three-way(x.second, y.second);" [expr.rel] / [over.match.oper]: <, >, <=, >=
// and != are rewritten from <=> and ==. [expos.only.entity]: synth-three-way uses <=> when
// three_way_comparable_with, otherwise derives a weak_ordering from operator<.
#include <utility>
#include <compare>
#include <limits>
#include <type_traits>
#include "check.hpp"

struct LessOnly {  // no <=>: synth-three-way falls back to < and yields weak_ordering
  int v;
  friend constexpr bool operator<(const LessOnly& a, const LessOnly& b) { return a.v < b.v; }
  friend constexpr bool operator==(const LessOnly&, const LessOnly&) = default;
};
struct Counting {
  static inline int seconds = 0;
  int v;
  friend bool operator==(const Counting& a, const Counting& b) {
    ++seconds;
    return a.v == b.v;
  }
  friend std::strong_ordering operator<=>(const Counting& a, const Counting& b) {
    ++seconds;
    return a.v <=> b.v;
  }
};

using P = std::pair<int, double>;
static_assert(std::is_same_v<decltype(P() <=> P()), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::pair<int, long>() <=> std::pair<int, long>()),
                             std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::pair<int, LessOnly>() <=> std::pair<int, LessOnly>()),
                             std::weak_ordering>);
static_assert(std::is_same_v<decltype(P() == P()), bool>);

constexpr bool test() {
  P a(1, 2.0), b(1, 3.0), c(2, 0.0);
  if (!(a == a) || a == b || !(a != b)) return false;
  if (!(a < b) || !(b < c) || !(a < c) || c < a) return false;
  if (!(a <= a) || !(b >= a) || !(c > b)) return false;
  if ((a <=> b) != std::partial_ordering::less) return false;
  if ((c <=> a) != std::partial_ordering::greater) return false;
  if ((a <=> a) != std::partial_ordering::equivalent) return false;
  // heterogeneous element types
  std::pair<long, float> h(1L, 2.0f);
  if (!(a == h) || !(h == a)) return false;
  if ((std::pair<int, int>(1, 2) <=> std::pair<long, short>(1L, short(3))) >= 0) return false;
  // first decides when it differs, regardless of second
  if (!(std::pair<int, int>(0, 100) < std::pair<int, int>(1, 0))) return false;
  // fallback to operator<
  std::pair<int, LessOnly> l1(1, {1}), l2(1, {2});
  if ((l1 <=> l2) != std::weak_ordering::less || !(l1 < l2) || l1 >= l2) return false;
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
  // NaN: unordered
  double nan = std::numeric_limits<double>::quiet_NaN();
  P n(1, nan);
  CHECK((n <=> n) == std::partial_ordering::unordered);
  CHECK(!(n == n) && !(n < n) && !(n > n));
  // a different first element short-circuits before second is compared
  std::pair<int, Counting> x(1, {5}), y(2, {5});
  Counting::seconds = 0;
  CHECK(x < y);
  CHECK(Counting::seconds == 0);
  CHECK(!(x == y));
  CHECK(Counting::seconds == 0);
  return 0;
}
