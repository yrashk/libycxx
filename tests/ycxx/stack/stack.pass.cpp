// [stack.defn], [stack.cons]: stack<T, Container> (Container = deque<T> by default) has
// member types from Container, the protected member c, constructors from a container (copy
// or move), from an iterator range and from a range (ranges::to<Container>); top() is
// c.back(); push / emplace / pop are push_back / emplace_back / pop_back, and emplace
// returns what emplace_back returns (decltype(auto), so a reference for the standard
// containers); swap swaps the containers. Works with vector, deque and list. [stack.ops]:
// ==, !=, <, >, <=, >= compare the containers, and <=> exists when Container is
// three_way_comparable. Every member is constexpr.
#include <stack>
#include <compare>
#include <deque>
#include <list>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include "test_iterators.hpp"
#include "check.hpp"

template <class C>
struct Peek : std::stack<typename C::value_type, C> {
  using Base = std::stack<typename C::value_type, C>;
  using Base::Base;
  constexpr const C& cont() const { return this->c; }
};

template <class C>
constexpr bool test() {
  using S = std::stack<int, C>;
  static_assert(std::is_same_v<typename S::value_type, int> && std::is_same_v<typename S::container_type, C>);
  static_assert(std::is_same_v<typename S::reference, typename C::reference>);
  static_assert(std::is_same_v<typename S::size_type, typename C::size_type>);
  static_assert(std::is_same_v<decltype(std::declval<S&>().emplace(1)), int&>);
  static_assert(std::is_same_v<decltype(std::declval<const S&>().top()), const int&>);
  S s;
  if (!s.empty() || s.size() != 0) return false;
  s.push(1);
  const int two = 2;
  s.push(two);
  int& r = s.emplace(3);
  if (&r != &s.top() || s.top() != 3 || s.size() != 3) return false;
  s.top() = 30;
  s.pop();
  if (s.top() != 2) return false;
  C c{1, 2, 3};
  Peek<C> p(c);
  if (!(p.cont() == c) || p.top() != 3) return false;
  Peek<C> pm(std::move(c));
  if (pm.size() != 3 || pm.top() != 3) return false;
  int arr[] = {4, 5, 6};
  Peek<C> pi(arr, arr + 3);
  if (pi.top() != 6 || !(pi.cont() == C{4, 5, 6})) return false;
  Peek<C> pin(InputIter<int>(arr), InputIter<int>(arr + 2));
  if (pin.top() != 5) return false;
  Peek<C> pr(std::from_range, arr);
  if (!(pr.cont() == C{4, 5, 6})) return false;
  Peek<C> pr2(std::from_range, InputRange<int>{arr, arr + 3});
  if (pr2.size() != 3) return false;
  S a(C{1, 2}), b(C{1, 3}), a2(C{1, 2}), shorter(C{1});
  if (!(a == a2) || a != a2 || !(a < b) || !(b > a) || !(a <= a2) || !(a >= a2) || !(shorter < a)) return false;
  if constexpr (std::three_way_comparable<C>) {
    if ((a <=> b) >= 0 || (a <=> a2) != 0) return false;
  }
  a.swap(b);
  if (a.top() != 3 || b.top() != 2) return false;
  swap(a, b);
  return a.top() == 2 && noexcept(a.swap(b)) == std::is_nothrow_swappable_v<C>;
}

static_assert(test<std::vector<int>>());
static_assert(test<std::deque<int>>());
static_assert(test<std::list<int>>());
static_assert(std::is_same_v<std::stack<int>::container_type, std::deque<int>>);

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::deque<int>>());
  CHECK(test<std::list<int>>());
  return 0;
}
