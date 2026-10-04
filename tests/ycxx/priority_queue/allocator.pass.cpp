// [priqueue.cons.alloc]: the allocator-extended constructors initialize c with the allocator
// (value-initializing or copying comp) and call make_heap where a container or elements are
// given; they take part only if uses_allocator_v<container_type, Alloc> is true.
// [priqueue.overview]: uses_allocator<priority_queue<...>, Alloc> derives from
// uses_allocator<Container, Alloc>.
#include <queue>
#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"

using A = IdAlloc<int>;
using C = std::vector<int, A>;
using PQ = std::priority_queue<int, C>;
struct Peek : PQ {
  using PQ::PQ;
  int id() const { return this->c.get_allocator().id; }
  bool heap() const { return std::is_heap(this->c.begin(), this->c.end(), this->comp); }
};

static_assert(std::uses_allocator_v<PQ, A> && !std::uses_allocator_v<PQ, std::allocator<int>>);
static_assert(std::is_constructible_v<PQ, const A&>);
static_assert(!std::is_constructible_v<PQ, const std::allocator<int>&>);

int main() {
  int arr[] = {2, 8, 5};
  Peek a(A(1));
  Peek b(std::less<int>(), A(2));
  C cont({1, 7, 3}, A(9));
  Peek c(std::less<int>(), cont, A(3));
  Peek d(std::less<int>(), C({4, 6}, A(9)), A(4));
  Peek e(c, A(5));
  Peek f(std::move(e), A(6));
  Peek g(arr, arr + 3, A(7));
  Peek h(arr, arr + 3, std::less<int>(), A(8));
  Peek i(arr, arr + 3, std::less<int>(), cont, A(10));
  Peek j(std::from_range, arr, std::less<int>(), A(11));
  Peek k(std::from_range, arr, A(12));
  CHECK(a.id() == 1 && b.id() == 2 && c.id() == 3 && d.id() == 4 && f.id() == 6 && g.id() == 7);
  CHECK(h.id() == 8 && i.id() == 10 && j.id() == 11 && k.id() == 12);
  CHECK(c.heap() && c.top() == 7 && d.top() == 6 && f.top() == 7 && g.top() == 8 && h.heap());
  CHECK(i.size() == 6 && i.top() == 8 && i.heap() && j.top() == 8 && k.heap());
  return 0;
}
