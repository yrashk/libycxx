// [deque.overview]/2: a deque meets the requirements of an allocator-aware container
// ([container.alloc.reqmts]). The generic check from tests/ycxx/containers (support/reqs):
// get_allocator, the allocator-extended constructors, and propagation on copy assignment,
// move assignment and swap according to propagate_on_container_*, for every combination of
// the three traits, with a stateful allocator.
// REQUIRES: exceptions
#include <deque>
#include <memory>
#include "container_values.hpp"
#include "test_allocators.hpp"
#include "reqs/allocator_aware.hpp"
#include "check.hpp"

template <class A>
using DeqInt = std::deque<int, typename std::allocator_traits<A>::template rebind_alloc<int>>;
template <class A>
using DeqElem = std::deque<Elem, typename std::allocator_traits<A>::template rebind_alloc<Elem>>;

template <template <class> class R>
constexpr bool all() {
  using namespace reqs::allocator_aware;
  return test<R, false, false, false>() && test<R, true, false, false>() &&
         test<R, false, true, false>() && test<R, false, false, true>() && test<R, true, true, true>();
}

static_assert(all<DeqInt>());
static_assert(all<DeqElem>());

int main() {
  CHECK(all<DeqInt>());
  CHECK(all<DeqElem>());
  std::deque<int, MinimalAlloc<int>> m{1, 2, 3};
  m.push_front(0);
  m.insert(m.begin() + 2, 9);
  CHECK(m.size() == 5 && m[0] == 0 && m[2] == 9 && m[4] == 3);
  return 0;
}
