// [alg.min.max]: min(a, b) "Returns: The smaller value. Returns the first argument when the
// arguments are equivalent."; max(a, b) "Returns: The larger value. Returns the first
// argument when the arguments are equivalent." Both return const T& to an argument and do
// "Exactly one comparison and two applications of the projection, if any". "An invocation
// may explicitly specify an argument for the template parameter T of the overloads in
// namespace std." ranges::min / ranges::max (const T&, const T&, comp, proj) likewise.
#include <algorithm>
#include <functional>
#include <type_traits>
#include "sort_support.hpp"
#include "check.hpp"

constexpr bool test() {
  int a = 1, b = 2;
  static_assert(std::is_same_v<decltype(std::min(a, b)), const int&>);
  static_assert(std::is_same_v<decltype(std::max(a, b, std::less<>{})), const int&>);
  static_assert(std::is_same_v<decltype(std::ranges::min(a, b)), const int&>);
  static_assert(std::is_same_v<decltype(std::ranges::max(a, b)), const int&>);
  if (&std::min(a, b) != &a || &std::min(b, a) != &a) return false;
  if (&std::max(a, b) != &b || &std::max(b, a) != &b) return false;
  // equivalent: the first argument, for both min and max
  int c = 5, d = 5;
  if (&std::min(c, d) != &c || &std::max(c, d) != &c) return false;
  if (&std::ranges::min(c, d) != &c || &std::ranges::max(c, d) != &c) return false;
  KV x{1, 0}, y{1, 1};
  if (std::min(x, y).id != 0 || std::max(x, y).id != 0) return false;
  if (std::min(y, x).id != 1 || std::max(y, x).id != 1) return false;
  // comparator
  if (&std::min(a, b, std::greater<>{}) != &b || &std::max(a, b, std::greater<>{}) != &a) return false;
  // equivalence under the comparator, not equality
  int t1 = 11, t2 = 19;
  auto tens = [](int p, int q) { return p / 10 < q / 10; };
  if (&std::min(t1, t2, tens) != &t1 || &std::max(t1, t2, tens) != &t1) return false;
  if (&std::min(t2, t1, tens) != &t2 || &std::max(t2, t1, tens) != &t2) return false;
  // explicit template argument with mixed argument types
  if (std::min<long>(3, 2L) != 2 || std::max<long>(3, 4L) != 4) return false;
  if (std::max<double>(1, 2.5) != 2.5) return false;
  // ranges with projection
  KV p{3, 0}, q{1, 1};
  if (&std::ranges::min(p, q, {}, &KV::key) != &q || &std::ranges::max(p, q, {}, &KV::key) != &p) return false;
  if (&std::ranges::min(p, q, {}, &KV::id) != &p) return false;
  if (&std::ranges::max(p, q, std::ranges::greater{}, &KV::key) != &q) return false;
  KV r{1, 7};
  if (&std::ranges::min(q, r, {}, &KV::key) != &q || &std::ranges::max(q, r, {}, &KV::key) != &q) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  int a = 4, b = 2;
  int comps = 0, projs = 0;
  (void)std::min(a, b, CountingLess{&comps});
  CHECK(comps == 1);
  comps = 0;
  (void)std::max(a, b, CountingLess{&comps});
  CHECK(comps == 1);
  comps = 0;
  (void)std::ranges::min(a, b, CountingLess{&comps}, CountingProj{&projs});
  CHECK(comps == 1 && projs == 2);
  comps = projs = 0;
  (void)std::ranges::max(a, b, CountingLess{&comps}, CountingProj{&projs});
  CHECK(comps == 1 && projs == 2);
  return 0;
}
