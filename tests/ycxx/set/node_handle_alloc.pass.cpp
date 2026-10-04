// [container.node.overview]/2, [container.node.cons]/1-5, [container.node.dtor],
// [container.node.observers], [container.node.modifiers]/1-2 for the node handles of
// set and multiset, with a stateful allocator
// (with and without propagate_on_container_move_assignment / _swap): a non-empty handle holds
// an allocator equal to the container's; moving a handle moves the allocator; move
// assignment into an empty handle takes the allocator, into a non-empty one destroys and
// deallocates the old element through the allocator and takes the allocator only when
// propagating; swap exchanges the allocators when one handle is empty or when propagating, and
// is noexcept when propagate_on_container_swap holds; the
// destructor destroys and deallocates; extract / insert(nh) construct, destroy, allocate and
// deallocate nothing ([associative.reqmts.general]/84-99, [unord.req.general]). The generic
// check support/reqs/node_handle_alloc.hpp.
#include <set>
#include <functional>
#include <utility>
#include "reqs/node_handle_alloc.hpp"
#include "check.hpp"

using reqs::node_handle_alloc::NAlloc;
using reqs::node_handle_alloc::test;

int main() {
  CHECK((test<std::set<int, std::less<int>, NAlloc<int, false, false>>>()));
  CHECK((test<std::set<int, std::less<int>, NAlloc<int, true, true>>>()));
  CHECK((test<std::set<int, std::less<int>, NAlloc<int, false, true>>>()));
  CHECK((test<std::multiset<int, std::less<int>, NAlloc<int, false, false>>>()));
  CHECK((test<std::multiset<int, std::less<int>, NAlloc<int, true, true>>>()));
  CHECK((test<std::multiset<int, std::less<int>, NAlloc<int, false, true>>>()));
  return 0;
}
