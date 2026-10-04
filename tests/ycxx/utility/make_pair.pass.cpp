// [pairs.spec]/6: make_pair(T1&& x, T2&& y) "Returns: pair<unwrap_ref_decay_t<T1>,
// unwrap_ref_decay_t<T2>>(std::forward<T1>(x), std::forward<T2>(y))". [meta.trans.other]:
// unwrap_ref_decay_t<T> is unwrap_reference_t<decay_t<T>>, i.e. reference_wrapper<X> becomes
// X&, arrays and functions decay, and cv-qualifiers and references are dropped.
#include <utility>
#include <functional>
#include <type_traits>
#include "check.hpp"

struct MoveOnly {
  int v;
  constexpr explicit MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = 0; }
};
int fn(int x) { return x + 1; }

int main() {
  int i = 1;
  const long cl = 2;
  static_assert(std::is_same_v<decltype(std::make_pair(i, cl)), std::pair<int, long>>);
  static_assert(std::is_same_v<decltype(std::make_pair(std::ref(i), std::cref(cl))),
                               std::pair<int&, const long&>>);
  static_assert(std::is_same_v<decltype(std::make_pair("ab", fn)),
                               std::pair<const char*, int (*)(int)>>);
  static_assert(std::is_same_v<decltype(std::make_pair(1, MoveOnly(1))), std::pair<int, MoveOnly>>);
  constexpr auto cp = std::make_pair(3, 4.5);
  static_assert(cp.first == 3 && cp.second == 4.5);

  // reference_wrapper arguments produce reference members bound to the referents
  auto r = std::make_pair(std::ref(i), 0);
  r.first = 9;
  CHECK(i == 9);
  // rvalues are moved
  MoveOnly m(5);
  auto pm = std::make_pair(std::move(m), 1);
  CHECK(pm.first.v == 5 && m.v == 0);
  // decayed function pointer works
  auto pf = std::make_pair(fn, "x");
  CHECK(pf.first(1) == 2 && pf.second[0] == 'x');
  return 0;
}
