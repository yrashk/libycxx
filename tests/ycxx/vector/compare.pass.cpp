// [vector.syn], [container.reqmts]/42-47: operator== is equal(c.begin(), c.end(),
// b.begin(), b.end()); != is !(==). operator<=> returns synth-three-way-result<T> and
// compares lexicographically, using T's <=> if it has one and otherwise synthesizing from
// < ([expos.only.entity] synth-three-way). <, >, <=, >= are rewritten from <=>.
#include <vector>
#include <compare>
#include <type_traits>
#include "check.hpp"

struct OnlyLess {
  int v;
  constexpr bool operator<(const OnlyLess& o) const { return v < o.v; }
  constexpr bool operator==(const OnlyLess& o) const { return v == o.v; }
};

static_assert(std::is_same_v<decltype(std::vector<int>() <=> std::vector<int>()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::vector<double>() <=> std::vector<double>()), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::vector<OnlyLess>() <=> std::vector<OnlyLess>()), std::weak_ordering>);
static_assert(std::is_same_v<decltype(std::vector<int>() == std::vector<int>()), bool>);

constexpr bool test() {
  std::vector<int> a{1, 2, 3}, b{1, 2, 4}, c{1, 2}, d{1, 2, 3};
  if (!(a == d) || a == b || !(a != b) || a == c) return false;
  if (!(a < b) || !(c < a) || !(b > c) || !(a <= d) || !(a >= d)) return false;
  if ((a <=> b) != std::strong_ordering::less || (a <=> d) != std::strong_ordering::equal) return false;
  if ((b <=> c) != std::strong_ordering::greater) return false;
  if (!(std::vector<int>() < c) || !(std::vector<int>() == std::vector<int>())) return false;
  std::vector<OnlyLess> x{{1}, {5}}, y{{1}, {6}};
  if ((x <=> y) != std::weak_ordering::less || !(x < y) || (y <=> y) != 0) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::vector<double> n{1.0, 0.0 / 0.0}, m{1.0, 2.0};
  CHECK((n <=> m) == std::partial_ordering::unordered);
  CHECK(!(n == n));
  return 0;
}
