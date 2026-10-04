// [priqueue.overview], [priqueue.cons], [priqueue.members]: priority_queue<T, Container,
// Compare> (vector<T> and less<> by default) keeps c as a heap with respect to the
// protected member comp: the constructors from (comp, container), from an iterator range
// (optionally appended to a given container) and from a range call make_heap; push /
// emplace are push_back / emplace_back followed by push_heap, pop is pop_heap then
// pop_back; top() is c.front(), so the elements come out largest first under comp (smallest
// first with greater<>). No equality is provided. Works with vector and deque.
#include <queue>
#include <algorithm>
#include <deque>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include "test_iterators.hpp"
#include "check.hpp"

template <class C, class Cmp>
struct Peek : std::priority_queue<int, C, Cmp> {
  using Base = std::priority_queue<int, C, Cmp>;
  using Base::Base;
  constexpr bool heap() const { return std::is_heap(this->c.begin(), this->c.end(), this->comp); }
  constexpr const C& cont() const { return this->c; }
};

template <class C, class Cmp>
constexpr bool drains_in_order(Peek<C, Cmp>& p, std::initializer_list<int> expect) {
  for (int e : expect) {
    if (p.empty() || p.top() != e || !p.heap()) return false;
    p.pop();
  }
  return p.empty();
}

template <class C>
constexpr bool test() {
  using P = Peek<C, std::less<int>>;
  using G = Peek<C, std::greater<int>>;
  static_assert(std::is_same_v<typename P::value_compare, std::less<int>>);
  static_assert(std::is_same_v<decltype(std::declval<const P&>().top()), const int&>);
  static_assert(std::is_same_v<decltype(std::declval<P&>().emplace(1)), void>);
  P p;
  for (int x : {3, 1, 4, 1, 5, 9, 2, 6}) p.push(x);
  if (p.size() != 8 || !p.heap()) return false;
  if (!drains_in_order(p, {9, 6, 5, 4, 3, 2, 1, 1})) return false;
  G g;
  const int seven = 7;
  g.push(seven);
  g.emplace(2);
  g.push(5);
  if (!drains_in_order(g, {2, 5, 7})) return false;
  C c{5, 1, 8};
  P from_cont(std::less<int>(), c);  // make_heap on a copy
  if (!from_cont.heap() || from_cont.top() != 8 || from_cont.size() != 3) return false;
  P from_moved(std::less<int>(), C{2, 7});
  if (from_moved.top() != 7) return false;
  int arr[] = {4, 9, 2};
  P it(arr, arr + 3);
  if (!it.heap() || it.top() != 9) return false;
  P it_in(InputIter<int>(arr), InputIter<int>(arr + 3), std::less<int>());
  if (it_in.top() != 9) return false;
  // iterator range appended to a given container
  P app(arr, arr + 3, std::less<int>(), C{10, 0});
  if (app.size() != 5 || app.top() != 10 || !app.heap()) return false;
  P app2(arr, arr + 3, std::less<int>(), c);
  if (app2.size() != 6 || app2.top() != 9) return false;
  G r(std::from_range, InputRange<int>{arr, arr + 3}, std::greater<int>());
  if (!drains_in_order(r, {2, 4, 9})) return false;
  P r2(std::from_range, arr);
  if (r2.top() != 9) return false;
  // swap exchanges containers and comparators
  P s1(std::less<int>(), C{1});
  P s2(std::less<int>(), C{2, 3});
  s1.swap(s2);
  if (s1.size() != 2 || s1.top() != 3 || s2.top() != 1) return false;
  swap(s1, s2);
  return s1.top() == 1;
}

template <class Q>
concept has_equality = requires(const Q& a) { a == a; };
static_assert(!has_equality<std::priority_queue<int>>);
static_assert(std::is_same_v<std::priority_queue<int>::container_type, std::vector<int>>);
static_assert(std::is_same_v<std::priority_queue<int>::value_compare, std::less<int>>);

static_assert(test<std::vector<int>>());
static_assert(test<std::deque<int>>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::deque<int>>());
  // stateful comparator: kept and used
  struct ByMod {
    int m;
    bool operator()(int a, int b) const { return a % m < b % m; }
  };
  std::priority_queue<int, std::vector<int>, ByMod> q(ByMod{10});
  for (int x : {19, 21, 35, 8}) q.push(x);
  CHECK(q.top() == 19);
  q.pop();
  CHECK(q.top() == 8);
  return 0;
}
