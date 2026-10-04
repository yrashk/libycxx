// [forward.list.overview]/2: forward_list is an allocator-aware container.
// The generic checks support/reqs/allocator_select_move_only.hpp ([container.reqmts]/64,
// [container.alloc.reqmts]/13-29): copy construction uses the allocator returned by
// select_on_container_copy_construction (which here differs from the source's), while the
// allocator-extended copy, move construction and copy assignment do not; and move assignment
// with unequal, non-propagating allocators and the allocator-extended move constructor work
// with a move-only element type (they need only Cpp17MoveInsertable / Cpp17MoveAssignable).
#include <forward_list>
#include "reqs/allocator_select_move_only.hpp"
#include "check.hpp"

template <class A>
using C = std::forward_list<typename A::value_type, A>;

using namespace reqs::allocator_select_move_only;

int main() {
  CHECK((copy_select<C, int>()));
  CHECK((copy_select<C, Elem>()));
  CHECK(move_only<C>());
  return 0;
}
