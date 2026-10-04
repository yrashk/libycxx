// [forward.list.overview], [forward.list.ops]: forward_list element requirements are [container.alloc.reqmts]/2.
// The generic check support/reqs/adl_robustness_seq.hpp ([contents]/3, [container.alloc.reqmts]/2,
// [iterator.requirements]): every constructor, assignment, insertion, erasure, resize, swap,
// comparison and erase/erase_if, the *_after members, and sort, merge, unique, remove, reverse and splice_after works when the element type's associated namespace
// (evil, or the global namespace for GVal) declares unconstrained function templates named
// move, copy, addressof, distance, fill, ... that fail to compile when instantiated, when the
// element type and the iterators passed in delete unary & and the comma operator, and when the
// elements are pointers to a class template specialization that must not be instantiated.
#include <forward_list>
#include "reqs/adl_robustness_seq.hpp"
#include "check.hpp"

template <class T>
using C = std::forward_list<T>;

int main() {
  CHECK(reqs::adl_robustness_seq::values<C<evil::Val>>());
  CHECK(reqs::adl_robustness_seq::values<C<GVal>>());
  CHECK(reqs::adl_robustness_seq::pointers<C<evil::Holder<evil::Incomplete>*>>());
  return 0;
}
