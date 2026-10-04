// [forward.list.erasure]: erase(c, value) is equivalent to
//   return erase_if(c, [&](const auto& elem) -> bool { return elem == value; });
// and erase_if(c, pred) to return c.remove_if(pred); both return forward_list::size_type, and
// U defaults to T so a braced initializer can be passed.
#include <forward_list>
#include <type_traits>
#include "check.hpp"

struct P {
  int a, b;
  constexpr bool operator==(const P&) const = default;
};

constexpr bool test() {
  std::forward_list<int> l{1, 2, 3, 2, 5, 2};
  static_assert(std::is_same_v<decltype(std::erase(l, 2)), std::forward_list<int>::size_type>);
  static_assert(std::is_same_v<decltype(std::erase_if(l, [](int) { return true; })), std::forward_list<int>::size_type>);
  if (std::erase(l, 2) != 3 || l != std::forward_list<int>{1, 3, 5}) return false;
  if (std::erase_if(l, [](int x) { return x > 2; }) != 2 || l != std::forward_list<int>{1}) return false;
  std::forward_list<P> ps{{1, 2}, {3, 4}, {1, 2}};
  if (std::erase(ps, {1, 2}) != 2 || ps.front() != P{3, 4}) return false;
  std::forward_list<int> e;
  return std::erase(e, 0) == 0;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
