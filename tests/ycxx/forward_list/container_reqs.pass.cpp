// [forward.list.overview]/2: forward_list meets the container requirements except for size()
// (and operator== is linear); every member is constexpr. The generic checks from
// tests/ycxx/containers (support/reqs) that need no size(), insert(p, t) or reverse
// iteration: construction and assignment, ==, begin/end/cbegin/cend, swap (contents and
// iterators following the elements), <=> ([container.opt.reqmts]), move construction /
// move assignment / swap keeping the elements in place; and [sequence.reqmts] X(n, t),
// X(i, j), X(from_range, rg), X(il), assignment from il, assign, assign_range ("In addition,
// a forward_list provides the assign member functions") and the front operations
// emplace_front, push_front, prepend_range, pop_front.
#include <forward_list>
#include <compare>
#include "container_values.hpp"
#include "reqs/container_construct_assign.hpp"
#include "reqs/container_equality.hpp"
#include "reqs/container_iterators.hpp"
#include "reqs/container_swap.hpp"
#include "reqs/container_three_way.hpp"
#include "reqs/move_swap_stability.hpp"
#include "reqs/sequence_construct.hpp"
#include "reqs/sequence_assign.hpp"
#include "reqs/sequence_front_ops.hpp"
#include "check.hpp"

template <class T>
constexpr std::forward_list<T> make_n(int n) {
  std::forward_list<T> l;
  for (int i = 0; i < n; ++i) l.push_front(val<T>(i));
  return l;
}

template <class X>
constexpr bool all() {
  return reqs::container_construct_assign::test<X>() && reqs::container_equality::generic<X>() &&
         reqs::container_iterators::test<X>() && reqs::container_swap::contents<X>() &&
         reqs::container_swap::iterators_follow<X>() && reqs::sequence_construct::test<X>() &&
         reqs::sequence_assign::test<X>() && reqs::sequence_front_ops::test<X>() &&
         reqs::move_swap_stability::test<X>(make_n<typename X::value_type>);
}

static_assert(all<std::forward_list<int>>());
static_assert(all<std::forward_list<Elem>>());
static_assert(reqs::container_three_way::test<std::forward_list<int>, std::strong_ordering>());
static_assert(reqs::container_three_way::test<std::forward_list<double>, std::partial_ordering>());

int main() {
  CHECK(all<std::forward_list<int>>());
  CHECK(all<std::forward_list<Elem>>());
  CHECK(all<std::forward_list<char>>());
  CHECK((reqs::container_three_way::test<std::forward_list<int>, std::strong_ordering>()));
  CHECK((reqs::container_three_way::test<std::forward_list<Elem>, std::strong_ordering>()));
  CHECK((reqs::container_three_way::test<std::forward_list<double>, std::partial_ordering>()));
  return 0;
}
