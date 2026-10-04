// [tuple.rel]: operator== and operator<=> between tuples of different element types and
// between a tuple and another tuple-like type (pair). The result type of <=> is
// common_comparison_category_t<synth-three-way-result<TTypes, UTypes>...>; comparisons are
// lexicographic and short-circuit ([tuple.rel]/4.1, Note 2).
#include <compare>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct OnlyLess {    // synth-three-way falls back to < and yields weak_ordering
  int v;
  friend constexpr bool operator<(OnlyLess a, OnlyLess b) { return a.v < b.v; }
  friend constexpr bool operator==(OnlyLess a, OnlyLess b) { return a.v == b.v; }
};
struct Counted {
  int v;
  int* count;
  friend constexpr bool operator==(const Counted& a, const Counted& b) { ++*a.count; return a.v == b.v; }
  friend constexpr std::strong_ordering operator<=>(const Counted& a, const Counted& b) {
    ++*a.count;
    return a.v <=> b.v;
  }
};

template <class T, class U> concept EqComparable = requires(const T& t, const U& u) { t == u; };
template <class T, class U> concept ThreeWay = requires(const T& t, const U& u) { t <=> u; };

static_assert(std::is_same_v<decltype(std::tuple<int>{} <=> std::tuple<long>{}), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::tuple<int, double>{} <=> std::tuple<long, float>{}), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::tuple<int, OnlyLess>{} <=> std::tuple<long, OnlyLess>{}), std::weak_ordering>);
static_assert(std::is_same_v<decltype(std::tuple<>{} <=> std::tuple<>{}), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::tuple<int, int>{} <=> std::pair<long, short>{}), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::tuple<int, double>{} <=> std::pair<int, double>{}), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::tuple<int>{} == std::tuple<long>{}), bool>);
// Sizes must match.
static_assert(!EqComparable<std::tuple<int>, std::tuple<int, int>>);
static_assert(!ThreeWay<std::tuple<int>, std::tuple<int, int>>);
static_assert(!EqComparable<std::tuple<int, int, int>, std::pair<int, int>>);
// Elements must be comparable.
static_assert(!EqComparable<std::tuple<int>, std::tuple<int*>>);
static_assert(EqComparable<std::tuple<int, int>, std::pair<long, char>>);

constexpr bool test() {
  if (!(std::tuple<int, long>(1, 2) == std::tuple<long, short>(1, 2))) return false;
  if (std::tuple<int, long>(1, 2) != std::tuple<long, short>(1, 2)) return false;
  if (!(std::tuple<int, long>(1, 2) != std::tuple<long, short>(1, 3))) return false;
  if (!(std::tuple<int, long>(1, 2) < std::tuple<long, short>(1, 3))) return false;
  if (!(std::tuple<int, long>(2, 0) > std::tuple<long, short>(1, 9))) return false;
  if ((std::tuple<int, long>(1, 2) <=> std::tuple<long, short>(1, 2)) != std::strong_ordering::equal) return false;
  if ((std::tuple<char, unsigned>('a', 5u) <=> std::tuple<int, unsigned long>(97, 6ul)) != std::strong_ordering::less) return false;
  // Against pair (found by ADL).
  if (!(std::tuple<int, int>(1, 2) == std::pair<long, short>(1, 2))) return false;
  if (!(std::tuple<int, int>(1, 2) < std::pair<long, short>(1, 3))) return false;
  if ((std::tuple<int, int>(3, 0) <=> std::pair<long, short>(1, 9)) != std::strong_ordering::greater) return false;
  if (!(std::pair<long, short>(1, 2) == std::tuple<int, int>(1, 2))) return false;   // reversed candidate
  if (!(std::pair<long, short>(1, 1) < std::tuple<int, int>(1, 2))) return false;
  // Empty tuples.
  if (!(std::tuple<>{} == std::tuple<>{}) || (std::tuple<>{} <=> std::tuple<>{}) != 0) return false;
  // partial ordering propagates NaN as unordered.
  const double nan = std::numeric_limits<double>::quiet_NaN();
  if ((std::tuple<int, double>(1, nan) <=> std::tuple<long, double>(1, 0.0)) != std::partial_ordering::unordered) return false;
  if ((std::tuple<int, double>(0, nan) <=> std::tuple<long, double>(1, 0.0)) != std::partial_ordering::less) return false;
  if (std::tuple<double>(nan) == std::tuple<double>(nan)) return false;
  // weak ordering via operator<.
  if ((std::tuple<OnlyLess>(OnlyLess{1}) <=> std::tuple<OnlyLess>(OnlyLess{2})) != std::weak_ordering::less) return false;
  if ((std::tuple<OnlyLess>(OnlyLess{2}) <=> std::tuple<OnlyLess>(OnlyLess{2})) != std::weak_ordering::equivalent) return false;
  // Short-circuit: later elements are not compared once the result is known.
  int count = 0;
  std::tuple<int, Counted> a(1, Counted{0, &count}), b(2, Counted{0, &count});
  if (a == b) return false;
  if (count != 0) return false;
  if ((a <=> b) != std::strong_ordering::less) return false;
  if (count != 0) return false;
  std::tuple<int, Counted> c(1, Counted{0, &count});
  if (!(a == c) || count != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
