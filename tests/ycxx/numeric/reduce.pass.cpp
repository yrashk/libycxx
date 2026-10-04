// [reduce]: reduce(first, last) is reduce(first, last, value_type{}); reduce(first, last,
// init) is reduce(first, last, init, plus<>()); the result is GENERALIZED_SUM(binary_op,
// init, *i...), so for an associative and commutative op it equals the plain sum.
// [transform.reduce]: transform_reduce(first1, last1, first2, init) is (init, plus<>,
// multiplies<>); the binary form GENERALIZED_SUM(op1, init, op2(*i, *(first2 + ...)));
// the unary form GENERALIZED_SUM(op, init, unary_op(*i)). All are constexpr; the return
// type is T (or the value type for the init-less reduce).
#include <numeric>
#include <functional>
#include <string>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 3, 4, 5};
  if (std::reduce(a, a + 5) != 15) return false;
  static_assert(std::is_same_v<decltype(std::reduce(a, a + 5)), int>);
  if (std::reduce(a, a) != 0) return false;
  if (std::reduce(a, a + 5, 10) != 25) return false;
  if (std::reduce(a, a + 5, 1, std::multiplies<>{}) != 120) return false;
  static_assert(std::is_same_v<decltype(std::reduce(a, a + 5, 0L)), long>);
  double d[] = {0.25, 0.5};
  if (std::reduce(d, d + 2) != 0.75) return false;
  if (std::reduce(InputIter<int>(a), InputIter<int>(a + 5), 0) != 15) return false;
  // max is associative and commutative too
  if (std::reduce(a, a + 5, 0, [](int x, int y) { return x > y ? x : y; }) != 5) return false;

  int b[] = {2, 2, 2, 2, 2};
  if (std::transform_reduce(a, a + 5, b, 0) != 30) return false;
  if (std::transform_reduce(a, a + 5, b, 0, std::plus<>{}, std::minus<>{}) != 5) return false;  // sum(a-b)
  if (std::transform_reduce(a, a + 5, 0, std::plus<>{}, [](int x) { return x * x; }) != 55) return false;
  if (std::transform_reduce(a, a, 7, std::plus<>{}, [](int x) { return x; }) != 7) return false;
  if (std::transform_reduce(InputIter<int>(a), InputIter<int>(a + 5), InputIter<int>(b), 0L) != 30) return false;
  std::string s[] = {"a", "bb", "ccc"};
  auto len = std::transform_reduce(s, s + 3, std::size_t{0}, std::plus<>{}, [](const std::string& x) { return x.size(); });
  if (len != 6) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
