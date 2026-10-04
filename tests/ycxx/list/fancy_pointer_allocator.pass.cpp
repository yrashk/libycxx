// [list.overview], [list.ops]: list with a fancy-pointer allocator.
// The generic check support/reqs/fancy_alloc_seq.hpp ([container.reqmts]/64 and its Note 2,
// [allocator.requirements.general]): the container works with an allocator whose pointer type
// is a class (FancyPtr), and its pointer / const_pointer members are the allocator's.
#include <list>
#include "reqs/fancy_alloc_seq.hpp"
#include "check.hpp"

using X_int = std::list<int, FancyAlloc<int>>;
using X_Elem = std::list<Elem, FancyAlloc<Elem>>;

int main() {
  CHECK(reqs::fancy_alloc_seq::test<X_int>());
  CHECK(reqs::fancy_alloc_seq::test<X_Elem>());
  return 0;
}
