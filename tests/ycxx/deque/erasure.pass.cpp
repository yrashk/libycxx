// [deque.erasure]: erase(c, value) is equivalent to
//   auto it = remove(c.begin(), c.end(), value); auto r = distance(it, c.end());
//   c.erase(it, c.end()); return r;
// and erase_if(c, pred) likewise with remove_if; both return deque::size_type. U defaults to
// T, so a braced initializer can be passed as the value. The relative order of the remaining
// elements is kept ([alg.remove] is stable).
// COUNTERPART: libstdcxx:23_containers/deque/debug/erase.cc
#include <deque>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

struct P {
  int a, b;
  constexpr bool operator==(const P&) const = default;
};

constexpr bool test() {
  std::deque<int> d;
  for (int i = 0; i < 300; ++i) d.push_back(i % 7);
  static_assert(std::is_same_v<decltype(std::erase(d, 3)), std::deque<int>::size_type>);
  static_assert(std::is_same_v<decltype(std::erase_if(d, [](int) { return true; })), std::deque<int>::size_type>);
  auto n = std::erase(d, 3);
  if (n != 43 || d.size() != 257) return false;
  for (int x : d)
    if (x == 3) return false;
  n = std::erase_if(d, [](int x) { return x % 2 == 0; });
  if (n != 171 || d.size() != 86) return false;
  for (std::size_t i = 0; i < d.size(); ++i)
    if (d[i] != (i % 2 == 0 ? 1 : 5)) return false;
  if (std::erase(d, 42) != 0 || d.size() != 86) return false;
  std::deque<P> ps{{1, 2}, {3, 4}, {1, 2}};
  if (std::erase(ps, {1, 2}) != 2 || ps.size() != 1 || !(ps[0] == P{3, 4})) return false;
  std::deque<long> longs{1, 2, 3};
  if (std::erase(longs, 2) != 1) return false;  // U = int
  std::deque<int> e;
  return std::erase_if(e, [](int) { return true; }) == 0;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
