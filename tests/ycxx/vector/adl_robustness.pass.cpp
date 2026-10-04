// [vector.overview]: vector is a sequence container whose element requirements are [container.alloc.reqmts]/2.
// The generic check support/reqs/adl_robustness_seq.hpp ([contents]/3, [container.alloc.reqmts]/2,
// [iterator.requirements]): every constructor, assignment, insertion, erasure, resize, swap,
// comparison and erase/erase_if, reserve and shrink_to_fit works when the element type's associated namespace
// (evil, or the global namespace for GVal) declares unconstrained function templates named
// move, copy, addressof, distance, fill, ... that fail to compile when instantiated, and when the
// element type and the iterators passed in delete unary & and the comma operator.
#include <vector>
#include "reqs/adl_robustness_seq.hpp"
#include "check.hpp"

template <class T>
using C = std::vector<T>;

int main() {
  CHECK(reqs::adl_robustness_seq::values<C<evil::Val>>());
  CHECK(reqs::adl_robustness_seq::values<C<GVal>>());
  return 0;
}
