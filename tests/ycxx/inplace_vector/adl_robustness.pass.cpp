// [inplace.vector.overview]: inplace_vector is a sequence container; its elements need what [container.alloc.reqmts]/2 defines for allocator<T>.
// The generic check support/reqs/adl_robustness_seq.hpp ([contents]/3, [container.alloc.reqmts]/2,
// [iterator.requirements]): every constructor, assignment, insertion, erasure, resize, swap,
// comparison and erase/erase_if works when the element type's associated namespace
// (evil, or the global namespace for GVal) declares unconstrained function templates named
// move, copy, addressof, distance, fill, ... that fail to compile when instantiated, when the
// element type and the iterators passed in delete unary & and the comma operator, and when the
// elements are pointers to a class template specialization that must not be instantiated.
#include <inplace_vector>
#include "reqs/adl_robustness_seq.hpp"
#include "check.hpp"

template <class T>
using C = std::inplace_vector<T, 128>;

int main() {
  CHECK(reqs::adl_robustness_seq::values<C<evil::Val>>());
  CHECK(reqs::adl_robustness_seq::values<C<GVal>>());
  CHECK(reqs::adl_robustness_seq::pointers<C<evil::Holder<evil::Incomplete>*>>());
  return 0;
}
