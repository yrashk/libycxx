// [inplace.vector.overview]/1-3: inplace_vector is a contiguous container meeting the
// container and reversible container requirements, with constexpr iterators. The generic
// checks from tests/ycxx/containers (support/reqs): member types, iterator copy/move not
// throwing, construction and assignment, ==, iterator operations including random access,
// size/max_size/empty, swap of contents ([container.reqmts]/65 excludes inplace_vector from
// the "iterators follow the elements" rule), <=>, rbegin/rend, no spurious invalidation by
// observers, and the contiguous-container properties. N is large enough for every check.
#include <inplace_vector>
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
#include "reqs/contiguous_container.hpp"
#include "check.hpp"

template <class T>
using IV = std::inplace_vector<T, 64>;

static_assert(reqs::container_types::container_types<IV<int>, int, false>());
static_assert(reqs::container_types::container_types<IV<Elem>, Elem, false>());
static_assert(reqs::iterator_nothrow::check<IV<int>>() && reqs::iterator_nothrow::check<IV<Elem>>());
static_assert(std::is_same_v<IV<int>::size_type, std::size_t> && std::is_same_v<IV<int>::pointer, int*>);

template <class X>
constexpr bool all() {
  return reqs::container_construct_assign::test<X>() && reqs::container_equality::generic<X>() &&
         reqs::container_iterators::test<X>() && reqs::container_size_empty::test<X>() &&
         reqs::container_swap::contents<X>() && reqs::reversible_container::test<X>() &&
         reqs::no_spurious_invalidation::test<X>() && reqs::contiguous_container::test<X>();
}

static_assert(all<IV<int>>());
static_assert(all<IV<Elem>>());
static_assert(reqs::container_three_way::test<IV<int>, std::strong_ordering>());
static_assert(reqs::container_three_way::test<IV<double>, std::partial_ordering>());

int main() {
  CHECK(all<IV<int>>());
  CHECK(all<IV<Elem>>());
  CHECK(all<IV<char>>());
  CHECK((reqs::container_three_way::test<IV<Elem>, std::strong_ordering>()));
  return 0;
}
