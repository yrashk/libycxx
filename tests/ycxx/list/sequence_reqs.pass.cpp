// [list.overview]/2: a list meets the requirements "of a sequence container, including most
// of the optional sequence container requirements ([sequence.reqmts]). The exceptions are
// the operator[] and at member functions, which are not provided." The generic checks from
// tests/ycxx/containers (support/reqs): X(n, t), X(i, j), X(from_range, rg), X(il);
// assignment from il, assign, assign_range; insert / insert_range / emplace (positions,
// return values, arguments aliasing elements); erase / clear; integral arguments selecting
// the (size_type, const T&) overloads; single-pass dereferencing; front/back, push_back,
// pop_back, append_range; and the front operations emplace_front, push_front,
// prepend_range, pop_front. Also in constant expressions, since every member used is
// constexpr. operator[] and at are absent.
// REQUIRES: exceptions
#include <list>
#include <cstddef>
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

template <class X>
concept has_index = requires(X& x) { x[std::size_t(0)]; };
template <class X>
concept has_at = requires(X& x) { x.at(std::size_t(0)); };
static_assert(!has_index<std::list<int>> && !has_at<std::list<int>>);
static_assert(!has_index<const std::list<int>> && !has_at<const std::list<int>>);

static_assert(all<std::list<int>>());
static_assert(all<std::list<Elem>>());
static_assert(reqs::sequence_integral_dispatch::test<std::list<int>, int>());
static_assert(reqs::sequence_integral_dispatch::test<std::list<double>, int>());

int main() {
  CHECK(all<std::list<int>>());
  CHECK(all<std::list<Elem>>());
  CHECK((reqs::sequence_integral_dispatch::test<std::list<int>, int>()));
  CHECK((reqs::sequence_integral_dispatch::test<std::list<char>, char>()));
  CHECK((reqs::sequence_integral_dispatch::test<std::list<long long>, short>()));
  return 0;
}
