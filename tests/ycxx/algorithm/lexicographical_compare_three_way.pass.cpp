// [alg.three.way]: lexicographical_compare_three_way(b1, e1, b2, e2, comp) returns "E(i),
// where i is the smallest integer in [0, N) such that E(i) != 0 is true, or (e1 - b1) <=>
// (e2 - b2) if no such integer exists", E(n) = comp(*(b1 + n), *(b2 + n)); the return type
// is decltype(comp(*b1, *b2)). "At most N applications of comp." The comp-less overload
// uses compare_three_way(). Input iterators suffice.
#include <algorithm>
#include <compare>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

struct Weak {
  constexpr std::weak_ordering operator()(int a, int b) const { return (a / 10) <=> (b / 10); }
};

constexpr bool test() {
  int a[] = {1, 2, 3};
  int b[] = {1, 2, 4};
  int c[] = {1, 2};
  static_assert(std::is_same_v<decltype(std::lexicographical_compare_three_way(a, a + 3, b, b + 3)), std::strong_ordering>);
  if (std::lexicographical_compare_three_way(a, a + 3, b, b + 3) != std::strong_ordering::less) return false;
  if (std::lexicographical_compare_three_way(b, b + 3, a, a + 3) != std::strong_ordering::greater) return false;
  if (std::lexicographical_compare_three_way(a, a + 3, a, a + 3) != std::strong_ordering::equal) return false;
  if (std::lexicographical_compare_three_way(c, c + 2, a, a + 3) != std::strong_ordering::less) return false;
  if (std::lexicographical_compare_three_way(a, a + 3, c, c + 2) != std::strong_ordering::greater) return false;
  if (std::lexicographical_compare_three_way(a, a, b, b) != std::strong_ordering::equal) return false;
  // the comparison category of comp is the return type
  int t1[] = {11, 25};
  int t2[] = {19, 21};
  auto w = std::lexicographical_compare_three_way(t1, t1 + 2, t2, t2 + 2, Weak{});
  static_assert(std::is_same_v<decltype(w), std::weak_ordering>);
  if (w != std::weak_ordering::equivalent) return false;
  // equal prefix under comp, then lengths decide (as strong_ordering converted to the category)
  int t3[] = {12};
  if (std::lexicographical_compare_three_way(t3, t3 + 1, t1, t1 + 2, Weak{}) != std::weak_ordering::less) return false;
  // partial ordering from floating point
  double d1[] = {1.0, 2.0};
  double d2[] = {1.0, 3.0};
  auto p = std::lexicographical_compare_three_way(d1, d1 + 2, d2, d2 + 2);
  static_assert(std::is_same_v<decltype(p), std::partial_ordering>);
  if (p != std::partial_ordering::less) return false;
  double nan = __builtin_nan("");
  double d3[] = {nan, 0.0};
  double d4[] = {nan, 5.0};
  if (std::lexicographical_compare_three_way(d3, d3 + 2, d4, d4 + 2) != std::partial_ordering::unordered) return false;
  // a heterogeneous custom comparator returning strong_ordering
  auto rev = [](int x, int y) { return y <=> x; };
  if (std::lexicographical_compare_three_way(a, a + 3, b, b + 3, rev) != std::strong_ordering::greater) return false;
  // input iterators
  if (std::lexicographical_compare_three_way(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b),
                                             InputIter<int>(b + 3)) != std::strong_ordering::less)
    return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  int x[50], y[50];
  for (int i = 0; i < 50; ++i) x[i] = y[i] = i;
  for (int n1 : {0, 10, 50})
    for (int n2 : {0, 10, 50}) {
      int calls = 0;
      auto cmp = [&calls](int p, int q) {
        ++calls;
        return p <=> q;
      };
      auto r = std::lexicographical_compare_three_way(x, x + n1, y, y + n2, cmp);
      CHECK(calls <= (n1 < n2 ? n1 : n2));
      CHECK(r == (n1 <=> n2));
    }
  return 0;
}
