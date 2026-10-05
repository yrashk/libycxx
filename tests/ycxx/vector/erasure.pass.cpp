// [vector.erasure]: erase(c, value) and erase_if(c, pred) remove the matching elements
// (stable for the rest) and return the number removed; U defaults to T so a
// braced-init-list can be passed as the value.
// COUNTERPART: libstdcxx:23_containers/vector/debug/erase.cc
#include <vector>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct P {
  int x, y;
  constexpr bool operator==(const P&) const = default;
};

static_assert(std::is_same_v<decltype(std::erase(std::declval<std::vector<int>&>(), 1)),
                             std::vector<int>::size_type>);

constexpr bool test() {
  std::vector<int> v{1, 2, 1, 3, 1, 4};
  if (std::erase(v, 1) != 3 || v != std::vector<int>{2, 3, 4}) return false;
  if (std::erase(v, 9) != 0 || v.size() != 3) return false;
  if (std::erase(v, 3L) != 1 || v != std::vector<int>{2, 4}) return false;  // U = long
  if (std::erase_if(v, [](int x) { return x % 2 == 0; }) != 2 || !v.empty()) return false;
  if (std::erase_if(v, [](int) { return true; }) != 0) return false;
  std::vector<P> p{{1, 2}, {3, 4}, {1, 2}};
  if (std::erase(p, {1, 2}) != 2 || p.size() != 1 || !(p[0] == P{3, 4})) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
