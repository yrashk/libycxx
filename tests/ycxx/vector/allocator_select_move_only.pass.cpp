// [vector.overview]/2: vector (and vector<bool>, [vector.bool.pspc]) is an allocator-aware container; every member is constexpr.
// The generic checks support/reqs/allocator_select_move_only.hpp ([container.reqmts]/64,
// [container.alloc.reqmts]/13-29): copy construction uses the allocator returned by
// select_on_container_copy_construction (which here differs from the source's), while the
// allocator-extended copy, move construction and copy assignment do not; and move assignment
// with unequal, non-propagating allocators and the allocator-extended move constructor work
// with a move-only element type (they need only Cpp17MoveInsertable / Cpp17MoveAssignable).
#include <vector>
#include "reqs/allocator_select_move_only.hpp"
#include "check.hpp"

template <class A>
using C = std::vector<typename A::value_type, A>;

using namespace reqs::allocator_select_move_only;

static_assert(copy_select<C, int>());
static_assert(copy_select<C, Elem>());
static_assert(copy_select<C, bool>());
static_assert(move_only<C>());

int main() {
  CHECK((copy_select<C, int>()));
  CHECK((copy_select<C, Elem>()));
  CHECK((copy_select<C, bool>()));
  CHECK(move_only<C>());
  return 0;
}
