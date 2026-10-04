// [deque.overview]/2: "A deque meets all of the requirements of a container
// ([container.reqmts]), of a reversible container ([container.rev.reqmts]), ..." /3: its
// iterators meet the constexpr iterator requirements, and every member is constexpr, so the
// checks run in constant expressions too. The generic checks from tests/ycxx/containers
// (support/reqs): member types, iterator copy/move not throwing, construction and assignment,
// ==, begin/end/cbegin/cend and random-access iterator comparisons, size/max_size/empty,
// swap (contents and iterators following the elements), <=> ([container.opt.reqmts]),
// rbegin/rend, and no spurious invalidation by observers ([container.reqmts]/67).
#include <deque>
#include <compare>
#include "container_values.hpp"
#include "reqs/container_types.hpp"
#include "reqs/iterator_nothrow.hpp"
#include "reqs/container_construct_assign.hpp"
#include "reqs/container_equality.hpp"
#include "reqs/container_iterators.hpp"
#include "reqs/container_size_empty.hpp"
#include "reqs/container_swap.hpp"
#include "reqs/container_three_way.hpp"
#include "reqs/reversible_container.hpp"
#include "reqs/no_spurious_invalidation.hpp"
#include "check.hpp"

static_assert(std::is_same_v<std::deque<int>::allocator_type, std::allocator<int>>);
static_assert(reqs::container_types::container_types<std::deque<int>, int>());
static_assert(reqs::container_types::container_types<std::deque<Elem>, Elem>());
static_assert(reqs::container_types::container_types<std::deque<const int*>, const int*>());
static_assert(reqs::iterator_nothrow::check<std::deque<int>>());
static_assert(reqs::iterator_nothrow::check<std::deque<Elem>>());

template <class X>
constexpr bool all() {
  return reqs::container_construct_assign::test<X>() && reqs::container_equality::generic<X>() &&
         reqs::container_iterators::test<X>() && reqs::container_size_empty::test<X>() &&
         reqs::container_swap::contents<X>() && reqs::container_swap::iterators_follow<X>() &&
         reqs::reversible_container::test<X>() && reqs::no_spurious_invalidation::test<X>();
}

static_assert(all<std::deque<int>>());
static_assert(all<std::deque<Elem>>());
static_assert(reqs::container_three_way::test<std::deque<int>, std::strong_ordering>());
static_assert(reqs::container_three_way::test<std::deque<Elem>, std::strong_ordering>());
static_assert(reqs::container_three_way::test<std::deque<double>, std::partial_ordering>());

int main() {
  CHECK(all<std::deque<int>>());
  CHECK(all<std::deque<Elem>>());
  CHECK(all<std::deque<char>>());
  CHECK((reqs::container_three_way::test<std::deque<int>, std::strong_ordering>()));
  CHECK((reqs::container_three_way::test<std::deque<Elem>, std::strong_ordering>()));
  CHECK((reqs::container_three_way::test<std::deque<double>, std::partial_ordering>()));
  return 0;
}
