// [alg.fold]: ranges::fold_left(r, init, f) folds from the left with accumulator type U =
// decay_t<invoke_result_t<F&, T, iter_reference_t<I>>> (returned as U even for an empty
// range); fold_left_first uses the first element as init and returns optional<U> (empty
// for an empty range); fold_right folds from the right with f(element, acc);
// fold_right_last uses the last element and returns optional<U>; fold_left_with_iter /
// fold_left_first_with_iter also return the end iterator (in_value_result). T defaults to
// the range's value type, so braced initial values work.
// COUNTERPART: libcxx:algorithms/alg.nonmodifying/alg.fold/ranges.fold_right_last.pass.cpp
#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 3, 4};
  if (std::ranges::fold_left(a, 0, std::plus<>{}) != 10) return false;
  if (std::ranges::fold_left(a, a + 4, 100, std::minus<>{}) != 90) return false;  // ((((100-1)-2)-3)-4)
  if (std::ranges::fold_right(a, 100, std::minus<>{}) != 98) return false;       // 1-(2-(3-(4-100)))
  // U from the invoke result: int + double -> double
  auto d = std::ranges::fold_left(a, 0, [](double acc, int x) { return acc + x / 2.0; });
  static_assert(std::is_same_v<decltype(d), double>);
  if (d != 5.0) return false;
  // empty range returns U(init)
  auto e = std::ranges::fold_left(a, a, 7L, std::plus<>{});
  static_assert(std::is_same_v<decltype(e), long>);
  if (e != 7) return false;
  // braced init: T defaults to range_value_t
  if (std::ranges::fold_left(a, {}, std::plus<>{}) != 10) return false;

  auto f = std::ranges::fold_left_first(a, std::multiplies<>{});
  static_assert(std::is_same_v<decltype(f), std::optional<int>>);
  if (!f || *f != 24) return false;
  if (std::ranges::fold_left_first(a, a, std::plus<>{}).has_value()) return false;
  auto g = std::ranges::fold_right_last(a, std::minus<>{});  // 1-(2-(3-4))
  if (!g || *g != -2) return false;
  if (std::ranges::fold_right_last(a + 2, a + 2, std::plus<>{}).has_value()) return false;

  auto w = std::ranges::fold_left_with_iter(a, 0, std::plus<>{});
  static_assert(std::is_same_v<decltype(w), std::ranges::fold_left_with_iter_result<int*, int>>);
  static_assert(std::is_same_v<std::ranges::fold_left_with_iter_result<int*, int>, std::ranges::in_value_result<int*, int>>);
  if (w.in != a + 4 || w.value != 10) return false;
  auto wf = std::ranges::fold_left_first_with_iter(a, a + 3, std::plus<>{});
  static_assert(std::is_same_v<decltype(wf), std::ranges::fold_left_first_with_iter_result<int*, std::optional<int>>>);
  if (wf.in != a + 3 || *wf.value != 6) return false;
  auto we = std::ranges::fold_left_first_with_iter(a, a, std::plus<>{});
  if (we.in != a || we.value.has_value()) return false;

  // single-pass input ranges and non-common sentinels for the left folds
  InputRange<int> in{a, a + 4};
  if (std::ranges::fold_left(in, 0, std::plus<>{}) != 10) return false;
  InputRange<int> in2{a, a + 4};
  auto wi = std::ranges::fold_left_with_iter(in2, 1, std::multiplies<>{});
  if (wi.in.p != a + 4 || wi.value != 24) return false;
  // order of operands is observable with a non-commutative operation
  std::string parts[] = {"a", "b", "c"};
  if (std::ranges::fold_left(parts, std::string("<"), std::plus<>{}) != "<abc") return false;
  if (std::ranges::fold_right(parts, std::string(">"), std::plus<>{}) != "abc>") return false;
  return true;
}
static_assert(test());

struct Owning {
  int v[2] = {1, 2};
  constexpr int* begin() { return v; }
  constexpr int* end() { return v + 2; }
};
static_assert(std::is_same_v<decltype(std::ranges::fold_left_with_iter(Owning{}, 0, std::plus<>{}).in),
                             std::ranges::dangling>);

int main() {
  CHECK(test());
  return 0;
}
