// [list.erasure]: erase(c, value) is equivalent to
//   return erase_if(c, [&](const auto& elem) -> bool { return elem == value; });
// and erase_if(c, pred) to return c.remove_if(pred); both return list::size_type, U defaults
// to T (so a braced initializer works), and the survivors keep their order and addresses.
#include <list>
#include <iterator>
#include <memory>
#include <type_traits>
#include "check.hpp"

struct P {
  int a, b;
  constexpr bool operator==(const P&) const = default;
};

constexpr bool test() {
  std::list<int> l;
  for (int i = 0; i < 70; ++i) l.push_back(i % 7);
  static_assert(std::is_same_v<decltype(std::erase(l, 3)), std::list<int>::size_type>);
  static_assert(std::is_same_v<decltype(std::erase_if(l, [](int) { return true; })), std::list<int>::size_type>);
  const int* p1 = std::addressof(*std::next(l.begin()));
  if (std::erase(l, 3) != 10 || l.size() != 60) return false;
  if (std::erase_if(l, [](int x) { return x != 1 && x != 5; }) != 40 || l.size() != 20) return false;
  if (std::addressof(l.front()) != p1) return false;
  int k = 0;
  for (int x : l)
    if (x != (k++ % 2 == 0 ? 1 : 5)) return false;
  std::list<P> ps{{1, 2}, {3, 4}, {1, 2}};
  if (std::erase(ps, {1, 2}) != 2 || ps.size() != 1) return false;
  std::list<long> longs{1, 2, 3, 2};
  return std::erase(longs, 2) == 2 && longs.size() == 2;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
