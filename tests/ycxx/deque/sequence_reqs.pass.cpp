// [deque.overview]/2: a deque meets the requirements "of a sequence container, including the
// optional sequence container requirements ([sequence.reqmts])". The generic checks from
// tests/ycxx/containers (support/reqs): X(n, t), X(i, j), X(from_range, rg), X(il);
// assignment from il, assign, assign_range; insert / insert_range / emplace (positions,
// return values, arguments aliasing elements); erase / clear; integral arguments selecting
// the (size_type, const T&) overloads; single-pass dereferencing; front/back, push_back,
// pop_back, operator[], at (and its out_of_range), append_range; and the front operations
// emplace_front, push_front, prepend_range, pop_front. Also in constant expressions, since
// every member is constexpr.
// REQUIRES: exceptions
#include <deque>
#include "container_values.hpp"
#include "reqs/sequence_construct.hpp"
#include "reqs/sequence_assign.hpp"
#include "reqs/sequence_insert.hpp"
#include "reqs/sequence_emplace.hpp"
#include "reqs/sequence_erase_clear.hpp"
#include "reqs/sequence_integral_dispatch.hpp"
#include "reqs/sequence_single_pass.hpp"
#include "reqs/sequence_optional_ops.hpp"
#include "reqs/sequence_front_ops.hpp"
#include "check.hpp"

template <class X>
constexpr bool all() {
  return reqs::sequence_construct::test<X>() && reqs::sequence_assign::test<X>() &&
         reqs::sequence_insert::test<X>() && reqs::sequence_emplace::generic<X>() &&
         reqs::sequence_erase_clear::test<X>() && reqs::sequence_single_pass::test<X>() &&
         reqs::sequence_optional_ops::test<X>() && reqs::sequence_front_ops::test<X>();
}

static_assert(all<std::deque<int>>());
static_assert(all<std::deque<Elem>>());
static_assert(reqs::sequence_integral_dispatch::test<std::deque<int>, int>());
static_assert(reqs::sequence_integral_dispatch::test<std::deque<char>, char>());
static_assert(reqs::sequence_integral_dispatch::test<std::deque<double>, int>());

int main() {
  CHECK(all<std::deque<int>>());
  CHECK(all<std::deque<Elem>>());
  CHECK((reqs::sequence_integral_dispatch::test<std::deque<int>, int>()));
  CHECK((reqs::sequence_integral_dispatch::test<std::deque<unsigned long>, unsigned long>()));
  CHECK((reqs::sequence_integral_dispatch::test<std::deque<long long>, short>()));
  CHECK(reqs::sequence_optional_ops::at_throws<std::deque<int>>());
  CHECK(reqs::sequence_optional_ops::at_throws<std::deque<Elem>>());
  return 0;
}
