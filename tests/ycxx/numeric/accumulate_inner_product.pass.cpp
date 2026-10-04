// [accumulate]: acc starts as init (so the result type is T, not the value type) and is
// updated in order with acc = std::move(acc) + *i or acc = binary_op(std::move(acc), *i).
// [inner.product]: acc = std::move(acc) + (*i1) * (*i2), or binary_op1(std::move(acc),
// binary_op2(*i1, *i2)), over [first1, last1) and the corresponding prefix of the second
// range, in order. Both are constexpr.
#include <numeric>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

// Records whether the accumulator was passed as an rvalue.
struct Acc {
  int sum = 0;
  int moved_in = 0;
  int copied_in = 0;
};
struct Op {
  constexpr Acc operator()(Acc&& a, int x) const {
    a.sum += x;
    ++a.moved_in;
    return std::move(a);
  }
  constexpr Acc operator()(const Acc& a, int x) const {
    Acc r = a;
    r.sum += x;
    ++r.copied_in;
    return r;
  }
};

constexpr bool test() {
  int a[] = {1, 2, 3, 4};
  if (std::accumulate(a, a + 4, 0) != 10) return false;
  if (std::accumulate(a, a, 42) != 42) return false;
  // T is the type of init: 0.5 + ints stays double, 0 + doubles truncates
  double ds[] = {0.5, 0.5, 0.5};
  static_assert(std::is_same_v<decltype(std::accumulate(ds, ds + 3, 0)), int>);
  if (std::accumulate(ds, ds + 3, 0) != 0) return false;  // 0 + 0.5 -> 0 each time
  if (std::accumulate(ds, ds + 3, 0.0) != 1.5) return false;
  if (std::accumulate(a, a + 4, 1, std::multiplies<>{}) != 24) return false;
  // order: acc on the left
  if (std::accumulate(a, a + 3, 100, std::minus<>{}) != 94) return false;
  std::string parts[] = {"x", "y", "z"};
  if (std::accumulate(parts, parts + 3, std::string(">")) != ">xyz") return false;
  if (std::accumulate(InputIter<int>(a), InputIter<int>(a + 4), 0) != 10) return false;
  Acc r = std::accumulate(a, a + 4, Acc{}, Op{});
  if (r.sum != 10 || r.moved_in != 4 || r.copied_in != 0) return false;

  int b[] = {5, 6, 7, 8};
  if (std::inner_product(a, a + 4, b, 0) != 5 + 12 + 21 + 32) return false;
  if (std::inner_product(a, a, b, 9) != 9) return false;
  if (std::inner_product(a, a + 2, b, 0) != 17) return false;  // only the first two of b
  // binary_op1 = acc combination, binary_op2 = elementwise
  if (std::inner_product(a, a + 4, b, 1, std::multiplies<>{}, std::plus<>{}) != 6 * 8 * 10 * 12) return false;
  if (std::inner_product(a, a + 3, b, 100, std::minus<>{}, std::multiplies<>{}) != 100 - 5 - 12 - 21) return false;
  int eq = std::inner_product(a, a + 4, a, 0, std::plus<>{}, std::equal_to<>{});
  if (eq != 4) return false;
  if (std::inner_product(InputIter<int>(a), InputIter<int>(a + 4), InputIter<int>(b), 0L) != 70L) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
