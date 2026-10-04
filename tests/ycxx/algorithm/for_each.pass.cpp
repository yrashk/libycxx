// [alg.foreach]: for_each(first, last, f) applies f to every *i in order and returns f;
// ranges::for_each calls invoke(f, invoke(proj, *i)) and returns {last, std::move(f)}
// (for_each_result<I, Fun> = in_fun_result); for_each_n(first, n, f) applies f to the first n
// elements and returns first + n (for n <= 0 nothing, returns first); ranges::for_each_n
// returns {first + n, std::move(f)}. f may modify the elements through a mutable iterator.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

struct Sum {
  int total = 0;
  int calls = 0;
  constexpr void operator()(int x) {
    total += x;
    ++calls;
  }
};
struct P {
  int x;
  int y;
};

constexpr bool test() {
  int a[] = {1, 2, 3, 4};
  Sum s = std::for_each(a, a + 4, Sum{});
  if (s.total != 10 || s.calls != 4) return false;
  std::for_each(a, a + 4, [](int& x) { x *= 2; });
  if (a[0] != 2 || a[3] != 8) return false;
  // order
  int seen[4] = {};
  int k = 0;
  std::for_each(InputIter<int>(a), InputIter<int>(a + 4), [&](int x) { seen[k++] = x; });
  if (seen[0] != 2 || seen[1] != 4 || seen[2] != 6 || seen[3] != 8) return false;

  // ranges::for_each
  auto r1 = std::ranges::for_each(a, a + 4, Sum{});
  static_assert(std::is_same_v<decltype(r1), std::ranges::for_each_result<int*, Sum>>);
  static_assert(std::is_same_v<std::ranges::for_each_result<int*, Sum>, std::ranges::in_fun_result<int*, Sum>>);
  if (r1.in != a + 4 || r1.fun.total != 20) return false;
  P ps[] = {{1, 10}, {2, 20}};
  auto r2 = std::ranges::for_each(ps, Sum{}, &P::y);
  if (r2.in != ps + 2 || r2.fun.total != 30) return false;
  ForwardRange<int> fr{a, a + 2};
  auto r3 = std::ranges::for_each(fr, Sum{});
  if (r3.in.p != a + 2 || r3.fun.total != 6) return false;

  // for_each_n
  int b[] = {1, 1, 1, 1, 1};
  int* e = std::for_each_n(b, 3, [](int& x) { x = 0; });
  if (e != b + 3 || b[2] != 0 || b[3] != 1) return false;
  if (std::for_each_n(b, 0, [](int&) {}) != b) return false;
  auto r4 = std::ranges::for_each_n(b + 1, 4, Sum{});
  static_assert(std::is_same_v<decltype(r4), std::ranges::for_each_n_result<int*, Sum>>);
  if (r4.in != b + 5 || r4.fun.total != 2 || r4.fun.calls != 4) return false;
  auto r5 = std::ranges::for_each_n(ps, 1, Sum{}, &P::x);
  if (r5.in != ps + 1 || r5.fun.total != 1) return false;
  auto r6 = std::ranges::for_each_n(InputIter<int>(b), 2, Sum{});
  if (r6.in.p != b + 2 || r6.fun.calls != 2) return false;
  return true;
}
static_assert(test());

// The ranges overload on an rvalue non-borrowed range returns dangling.
struct Owning {
  int v[2] = {1, 2};
  constexpr int* begin() { return v; }
  constexpr int* end() { return v + 2; }
};
static_assert(std::is_same_v<decltype(std::ranges::for_each(Owning{}, Sum{}).in), std::ranges::dangling>);

int main() {
  CHECK(test());
  return 0;
}
