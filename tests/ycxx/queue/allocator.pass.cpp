// [queue.cons.alloc]: the allocator-extended constructors initialize c with the allocator
// (and the container / other queue / iterator range / range) and take part in overload
// resolution only if uses_allocator_v<container_type, Alloc> is true. [queue.syn]:
// uses_allocator<queue<T, Container>, Alloc> derives from uses_allocator<Container, Alloc>.
// REQUIRES: exceptions
#include <queue>
#include <deque>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"

using A = IdAlloc<int>;
using C = std::deque<int, A>;
struct Peek : std::queue<int, C> {
  using std::queue<int, C>::queue;
  int id() const { return this->c.get_allocator().id; }
};

static_assert(std::uses_allocator_v<std::queue<int, C>, A>);
static_assert(!std::uses_allocator_v<std::queue<int, C>, std::allocator<int>>);
static_assert(std::is_constructible_v<std::queue<int, C>, const A&>);
static_assert(!std::is_constructible_v<std::queue<int, C>, const std::allocator<int>&>);
static_assert(!std::is_constructible_v<std::queue<int, C>, const C&, const std::allocator<int>&>);

int main() {
  Peek a(A(1));
  C cont({1, 2}, A(9));
  Peek b(cont, A(2));
  Peek c(C({3}, A(9)), A(3));
  Peek d(b, A(4));
  Peek e(std::move(d), A(5));
  int arr[] = {7, 8};
  Peek f(arr, arr + 2, A(6));
  Peek g(std::from_range, arr, A(7));
  CHECK(a.id() == 1 && b.id() == 2 && c.id() == 3 && e.id() == 5 && f.id() == 6 && g.id() == 7);
  CHECK(b.back() == 2 && c.front() == 3 && e.size() == 2 && f.front() == 7 && g.back() == 8);
  return 0;
}
