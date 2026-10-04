// [list.overview]/2: list is an allocator-aware container ([container.alloc.reqmts]); [list.ops] remove / unique / merge / splice.
// The generic check support/reqs/allocator_construct_seq.hpp ([container.alloc.reqmts]/2 and
// its Note 2, [container.reqmts]/64): every element (and every temporary T, e.g. for emplace
// with an argument aliasing an element) is constructed with allocator_traits<A>::construct
// and destroyed with allocator_traits<A>::destroy, across the constructors, assignments,
// insertions, erasures, resize, erase/erase_if, swap and the list operations.
#include <list>
#include "reqs/allocator_construct_seq.hpp"
#include "check.hpp"

using namespace reqs::allocator_construct_seq;

int main() {
  CHECK(test<std::list<Tracked, ConstructAlloc<Tracked>>>());
  return 0;
}
