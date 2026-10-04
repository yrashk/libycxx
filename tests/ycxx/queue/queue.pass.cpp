// [queue.defn], [queue.cons]: queue<T, Container> (Container = deque<T> by default) has
// member types from Container and the protected member c; constructors from a container
// (copy or move), an iterator range and a range; front() / back() are c.front() / c.back();
// push / emplace / pop are push_back / emplace_back / pop_front, emplace returning what
// emplace_back returns (decltype(auto)); swap swaps the containers; [queue.ops]: the
// comparisons compare the containers, <=> when Container is three_way_comparable. Works with
// deque and list. Every member is constexpr.
#include <queue>
#include <compare>
#include <deque>
#include <list>
#include <ranges>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

template <class C>
struct Peek : std::queue<typename C::value_type, C> {
  using Base = std::queue<typename C::value_type, C>;
  using Base::Base;
  constexpr const C& cont() const { return this->c; }
};

template <class C>
constexpr bool test() {
  using Q = std::queue<int, C>;
  static_assert(std::is_same_v<typename Q::value_type, int> && std::is_same_v<typename Q::container_type, C>);
  static_assert(std::is_same_v<typename Q::const_reference, typename C::const_reference>);
  static_assert(std::is_same_v<decltype(std::declval<Q&>().emplace(1)), int&>);
  static_assert(std::is_same_v<decltype(std::declval<const Q&>().front()), const int&>);
  static_assert(std::is_same_v<decltype(std::declval<Q&>().back()), int&>);
  Q q;
  if (!q.empty() || q.size() != 0) return false;
  q.push(1);
  const int two = 2;
  q.push(two);
  int& r = q.emplace(3);
  if (&r != &q.back() || q.front() != 1 || q.back() != 3 || q.size() != 3) return false;
  q.front() = 10;
  q.pop();
  if (q.front() != 2 || q.size() != 2) return false;
  C c{1, 2, 3};
  Peek<C> p(c);
  if (!(p.cont() == c) || p.front() != 1 || p.back() != 3) return false;
  Peek<C> pm(std::move(c));
  if (pm.size() != 3) return false;
  int arr[] = {4, 5, 6};
  Peek<C> pi(arr, arr + 3);
  if (pi.front() != 4 || pi.back() != 6) return false;
  Peek<C> pin(InputIter<int>(arr), InputIter<int>(arr + 2));
  if (pin.back() != 5) return false;
  Peek<C> pr(std::from_range, InputRange<int>{arr, arr + 3});
  if (!(pr.cont() == C{4, 5, 6})) return false;
  Q a(C{1, 2}), b(C{1, 3}), a2(C{1, 2});
  if (!(a == a2) || a != a2 || !(a < b) || !(b > a) || !(a <= a2) || !(b >= a)) return false;
  if constexpr (std::three_way_comparable<C>) {
    if ((a <=> b) >= 0 || (b <=> a) <= 0) return false;
  }
  a.swap(b);
  if (a.back() != 3) return false;
  swap(a, b);
  return a.back() == 2;
}

static_assert(test<std::deque<int>>());
static_assert(test<std::list<int>>());
static_assert(std::is_same_v<std::queue<int>::container_type, std::deque<int>>);

int main() {
  CHECK(test<std::deque<int>>());
  CHECK(test<std::list<int>>());
  return 0;
}
