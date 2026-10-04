// [vector.overview], [vector.bool.pspc], [vector.data]: vector and vector<bool> with a fancy-pointer allocator, also in constant evaluation (every member is constexpr).
// The generic check support/reqs/fancy_alloc_seq.hpp ([container.reqmts]/64 and its Note 2,
// [allocator.requirements.general]): the container works with an allocator whose pointer type
// is a class (FancyPtr), and its pointer / const_pointer members are the allocator's.
#include <vector>
#include "reqs/fancy_alloc_seq.hpp"
#include "check.hpp"

using X_int = std::vector<int, FancyAlloc<int>>;
using X_Elem = std::vector<Elem, FancyAlloc<Elem>>;
using X_bool = std::vector<bool, FancyAlloc<bool>>;

static_assert(reqs::fancy_alloc_seq::test<X_int>());
static_assert(reqs::fancy_alloc_seq::test<X_Elem>());
static_assert(reqs::fancy_alloc_seq::test<X_bool>());

int main() {
  CHECK(reqs::fancy_alloc_seq::test<X_int>());
  CHECK(reqs::fancy_alloc_seq::test<X_Elem>());
  CHECK(reqs::fancy_alloc_seq::test<X_bool>());
  return 0;
}
