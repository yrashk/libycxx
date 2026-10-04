// [algorithms.results]: in_fun_result, in_in_result, in_out_result, in_in_out_result,
// in_out_out_result, min_max_result, in_found_result, in_value_result, out_value_result are
// aggregates with the documented member names (declared with [[no_unique_address]]), and
// each converts to another specialization of the same template when the members are
// convertible (from const& by copying, from && by moving).
#include <algorithm>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct MoveOnly {
  int v = 0;
  constexpr MoveOnly(int x) : v(x) {}
  MoveOnly(const MoveOnly&) = delete;
  constexpr MoveOnly(MoveOnly&& o) : v(o.v) { o.v = -1; }
};
struct FromMoveOnly {
  int v;
  constexpr FromMoveOnly(MoveOnly&& m) : v(m.v) { m.v = -1; }
};

static_assert(std::is_aggregate_v<std::ranges::in_fun_result<int*, int>>);
static_assert(std::is_aggregate_v<std::ranges::in_in_result<int*, int*>>);
static_assert(std::is_aggregate_v<std::ranges::in_value_result<int*, int>>);
static_assert(std::is_aggregate_v<std::ranges::min_max_result<int>>);
static_assert(std::is_convertible_v<const std::ranges::in_fun_result<int*, int>&, std::ranges::in_fun_result<const int*, long>>);
static_assert(!std::is_convertible_v<const std::ranges::in_fun_result<const int*, int>&, std::ranges::in_fun_result<int*, int>>);
static_assert(!std::is_convertible_v<const std::ranges::in_fun_result<int*, MoveOnly>&, std::ranges::in_fun_result<int*, FromMoveOnly>>);
static_assert(std::is_convertible_v<std::ranges::in_fun_result<int*, MoveOnly>&&, std::ranges::in_fun_result<int*, FromMoveOnly>>);

constexpr bool test() {
  int a[3] = {};
  std::ranges::in_fun_result<int*, int> f{a + 1, 5};
  if (f.in != a + 1 || f.fun != 5) return false;
  auto [i, fn] = f;
  if (i != a + 1 || fn != 5) return false;
  std::ranges::in_fun_result<const int*, long> g = f;
  if (g.in != a + 1 || g.fun != 5) return false;
  std::ranges::in_in_result<int*, int*> ii{a, a + 2};
  if (ii.in1 != a || ii.in2 != a + 2) return false;
  std::ranges::in_out_result<int*, int*> io{a, a + 1};
  if (io.in != a || io.out != a + 1) return false;
  std::ranges::in_in_out_result<int*, int*, int*> iio{a, a + 1, a + 2};
  if (iio.in1 != a || iio.in2 != a + 1 || iio.out != a + 2) return false;
  std::ranges::in_out_out_result<int*, int*, int*> ioo{a, a + 1, a + 2};
  if (ioo.out1 != a + 1 || ioo.out2 != a + 2) return false;
  std::ranges::min_max_result<int> mm{1, 9};
  if (mm.min != 1 || mm.max != 9) return false;
  std::ranges::in_found_result<int*> fr{a, true};
  if (!fr.found) return false;
  std::ranges::in_value_result<int*, int> iv{a, 3};
  if (iv.value != 3) return false;
  std::ranges::out_value_result<int*, int> ov{a, 4};
  if (ov.out != a || ov.value != 4) return false;
  std::ranges::in_fun_result<int*, MoveOnly> m{a, MoveOnly(7)};
  std::ranges::in_fun_result<int*, FromMoveOnly> n = std::move(m);
  if (n.fun.v != 7 || m.fun.v != -1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
