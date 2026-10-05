// [inplace.vector.overview]/2: inplace_vector meets the sequence container requirements
// "including most of the optional sequence container requirements"; push_front,
// prepend_range, pop_front and emplace_front are not provided. The generic checks from
// tests/ycxx/containers (support/reqs): X(n, t), X(i, j), X(from_range, rg), X(il); assign
// forms; insert / insert_range / emplace; erase / clear; integral arguments selecting the
// (size_type, const T&) overloads; single-pass dereferencing; front/back, push_back,
// pop_back, operator[], at, append_range. Also in constant expressions.
// REQUIRES: exceptions
// COUNTERPART: libstdcxx:23_containers/inplace_vector/(cons/1|copy|move).cc
#include <inplace_vector>
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
#include "check.hpp"

template <class T>
using IV = std::inplace_vector<T, 128>;

template <class X>
constexpr bool all() {
  return reqs::sequence_construct::test<X>() && reqs::sequence_assign::test<X>() &&
         reqs::sequence_insert::test<X>() && reqs::sequence_emplace::generic<X>() &&
         reqs::sequence_erase_clear::test<X>() && reqs::sequence_single_pass::test<X>() &&
         reqs::sequence_optional_ops::test<X, typename X::reference>();
}

template <class X>
concept has_front_ops = requires(X& x) { x.push_front(x.front()); } || requires(X& x) { x.pop_front(); } ||
                        requires(X& x) { x.emplace_front(); } || requires(X& x, int (&r)[1]) { x.prepend_range(r); };
static_assert(!has_front_ops<IV<int>>);

static_assert(all<IV<int>>());
static_assert(all<IV<Elem>>());
static_assert(reqs::sequence_integral_dispatch::test<IV<int>, int>());
static_assert(reqs::sequence_integral_dispatch::test<IV<double>, int>());

int main() {
  CHECK(all<IV<int>>());
  CHECK(all<IV<Elem>>());
  CHECK((reqs::sequence_integral_dispatch::test<IV<char>, char>()));
  CHECK(reqs::sequence_optional_ops::at_throws<IV<int>>());
  CHECK(reqs::sequence_optional_ops::at_throws<IV<Elem>>());
  return 0;
}
