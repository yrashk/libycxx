// [map.modifiers]/1-2: template<class P> insert(P&& x) (and insert(position, P&&)) take part
// only if is_constructible_v<value_type, P&&> and are emplace(std::forward<P>(x)) /
// emplace_hint(position, std::forward<P>(x)). So a pair<int, long> or pair<long, int>
// inserts with conversion, a braced {k, v} picks insert(value_type&&), and emplace forwards
// (k, v) or (piecewise_construct, tuple, tuple) to pair's constructors. Same for multimap
// ([multimap.modifiers]).
#include <map>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Two {
  int a, b;
  constexpr Two(int x, int y) : a(x), b(y) {}
};

template <class M>
constexpr bool test() {
  M m;
  std::pair<long, int> pl(3, 30);
  m.insert(pl);  // P = pair<long, int>&
  m.insert(std::pair<int, long>(1, 10L));
  m.insert({2, 20});  // value_type&&
  m.insert(m.cend(), std::pair<short, int>(4, 40));
  m.insert(m.cbegin(), {0, 0});
  m.emplace(5, 50);
  m.emplace(std::piecewise_construct, std::forward_as_tuple(6), std::forward_as_tuple(60));
  m.emplace_hint(m.cend(), 7, 70);
  if (m.size() != 8) return false;
  int k = 0;
  for (auto& [key, v] : m) {
    if (key != k || v != 10 * k) return false;
    ++k;
  }
  return true;
}

template <class M>
constexpr bool piecewise() {
  M m;
  m.emplace(std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple(2, 3));
  m.emplace(std::piecewise_construct, std::forward_as_tuple(0), std::forward_as_tuple(4, 5));
  return m.size() == 2 && m.begin()->second.a == 4 && std::next(m.begin())->second.b == 3;
}

template <class M>
concept can_insert = requires(M& m, std::pair<int, int*> p) { m.insert(p); };
static_assert(!can_insert<std::map<int, int>>);  // value_type is not constructible from it
static_assert(!can_insert<std::multimap<int, int>>);

static_assert(test<std::map<int, long>>() && test<std::multimap<int, long>>());
static_assert(piecewise<std::map<int, Two>>() && piecewise<std::multimap<int, Two>>());

int main() {
  CHECK(test<std::map<int, long>>());
  CHECK(test<std::multimap<int, long>>());
  CHECK(piecewise<std::map<int, Two>>());
  CHECK(piecewise<std::multimap<int, Two>>());
  return 0;
}
