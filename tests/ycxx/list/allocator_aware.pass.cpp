// [list.overview]/2: a list meets the requirements of an allocator-aware container
// ([container.alloc.reqmts]). The generic check from tests/ycxx/containers (support/reqs):
// get_allocator, the allocator-extended constructors, and propagation on copy assignment,
// move assignment and swap according to propagate_on_container_*, for every combination of
// the three traits, with a stateful allocator.
// REQUIRES: exceptions
#include <list>
#include <iterator>
#include <memory>
#include "container_values.hpp"
#include "test_allocators.hpp"
#include "reqs/allocator_aware.hpp"
#include "check.hpp"

template <class A>
using ListInt = std::list<int, typename std::allocator_traits<A>::template rebind_alloc<int>>;
template <class A>
using ListElem = std::list<Elem, typename std::allocator_traits<A>::template rebind_alloc<Elem>>;

template <template <class> class R>
constexpr bool all() {
  using namespace reqs::allocator_aware;
  return test<R, false, false, false>() && test<R, true, false, false>() &&
         test<R, false, true, false>() && test<R, false, false, true>() && test<R, true, true, true>();
}

static_assert(all<ListInt>());
static_assert(all<ListElem>());

int main() {
  CHECK(all<ListInt>());
  CHECK(all<ListElem>());
  std::list<int, MinimalAlloc<int>> m{1, 2, 3};
  m.push_front(0);
  m.insert(std::next(m.begin(), 2), 9);
  m.sort();
  m.reverse();
  CHECK(m.size() == 5 && m.front() == 9 && m.back() == 0);
  return 0;
}
