// [set.overview]/2, [multiset.overview]/2: the container, reversible container and
// allocator-aware container requirements. The generic checks from tests/ycxx/containers
// (support/reqs) that apply to an ordered container: == ([container.reqmts]/42-47), <=>
// ([container.opt.reqmts]), size / max_size / empty, rbegin / rend, the allocator-aware
// constructors and propagate_on_container_* behaviour, and move construction / move
// assignment / swap keeping the elements in place. Plus copy semantics and noexcept members
// from the synopsis.
#include <set>
#include <compare>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_allocators.hpp"
#include "reqs/container_equality.hpp"
#include "reqs/container_three_way.hpp"
#include "reqs/container_size_empty.hpp"
#include "reqs/reversible_container.hpp"
#include "reqs/allocator_aware.hpp"
#include "reqs/move_swap_stability.hpp"
#include "check.hpp"

template <class X>
constexpr X make_n(int n) {
  X x;
  for (int i = 0; i < n; ++i) x.insert(val<typename X::value_type>((i * 7) % n));
  return x;
}

template <class X>
constexpr bool all() {
  return reqs::container_equality::generic<X>() && reqs::container_size_empty::test<X>() &&
         reqs::reversible_container::test<X>() &&
         reqs::move_swap_stability::test<X>(make_n<X>);
}

template <class A>
using SetInt = std::set<int, std::less<int>, typename std::allocator_traits<A>::template rebind_alloc<int>>;
template <class A>
using MSetElem = std::multiset<Elem, std::less<Elem>, typename std::allocator_traits<A>::template rebind_alloc<Elem>>;

template <template <class> class R>
constexpr bool allocs() {
  using namespace reqs::allocator_aware;
  return test<R, false, false, false>() && test<R, true, false, false>() && test<R, false, true, false>() &&
         test<R, false, false, true>() && test<R, true, true, true>();
}

using S = std::set<int>;
S& s() noexcept;
const S& cs() noexcept;
static_assert(noexcept(s().begin()) && noexcept(cs().end()) && noexcept(s().rbegin()) && noexcept(cs().crend()));
static_assert(noexcept(cs().size()) && noexcept(cs().empty()) && noexcept(cs().max_size()) && noexcept(s().clear()));
static_assert(std::is_nothrow_move_assignable_v<S> && noexcept(s().swap(s())) && noexcept(swap(s(), s())));
static_assert(std::is_nothrow_move_assignable_v<std::multiset<int>>);

static_assert(all<std::set<int>>() && all<std::set<Elem>>() && all<std::multiset<int>>());
static_assert(reqs::container_three_way::test<std::set<int>, std::strong_ordering>());
static_assert(reqs::container_three_way::test<std::multiset<double>, std::partial_ordering>());
static_assert(allocs<SetInt>() && allocs<MSetElem>());

int main() {
  CHECK(all<std::set<int>>());
  CHECK(all<std::set<Elem>>());
  CHECK(all<std::multiset<int>>());
  CHECK(all<std::multiset<Elem>>());
  CHECK((reqs::container_three_way::test<std::set<Elem>, std::strong_ordering>()));
  CHECK((reqs::container_three_way::test<std::multiset<double>, std::partial_ordering>()));
  CHECK(allocs<SetInt>());
  CHECK(allocs<MSetElem>());
  std::multiset<int> m{2, 1, 2};
  std::multiset<int> m2(m);
  CHECK(m2 == m && m2.count(2) == 2);
  return 0;
}
