// [container.reqmts]/15-16, /50, /65 and [container.alloc.reqmts]/17,20 for list: move
// construction (also with an equal allocator), move assignment with std::allocator and swap
// do not copy or move individual elements; the elements stay where they are and are owned
// by the destination, and after swap iterators refer to the same elements in the other
// container. ([container.reqmts]/66 Note 4: end() may be invalidated by swap; it is not
// used.)
#include <list>
#include "container_values.hpp"
#include "reqs/move_swap_stability.hpp"
#include "check.hpp"

template <class T>
constexpr std::list<T> make_n(int n) {
  std::list<T> d;
  for (int i = 0; i < n; ++i) d.push_back(val<T>(i));
  return d;
}

static_assert(reqs::move_swap_stability::test<std::list<int>>(make_n<int>));
static_assert(reqs::move_swap_stability::test<std::list<Elem>>(make_n<Elem>));

int main() {
  CHECK(reqs::move_swap_stability::test<std::list<int>>(make_n<int>));
  CHECK(reqs::move_swap_stability::test<std::list<Elem>>(make_n<Elem>));
  return 0;
}
