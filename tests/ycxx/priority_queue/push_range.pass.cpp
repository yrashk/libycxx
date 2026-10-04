// [priqueue.members]/3-4: push_range(rg) inserts all elements of rg into c via
// c.append_range(std::forward<R>(rg)) if valid, or ranges::copy(rg, back_inserter(c))
// otherwise, then restores the heap property as if by make_heap; postcondition:
// is_heap(c.begin(), c.end(), comp).
#include <queue>
#include <algorithm>
#include <functional>
#include <vector>
#include "mini_sequence.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class C>
struct Peek : std::priority_queue<int, C> {
  constexpr C& cont() { return this->c; }
  constexpr bool heap() { return std::is_heap(this->c.begin(), this->c.end(), this->comp); }
};

constexpr bool test() {
  int arr[] = {3, 9, 1, 7};
  Peek<MiniSeq<int, true>> with;
  with.push(5);
  with.push_range(arr);
  if (with.cont().append_calls != 1 || with.size() != 5 || with.top() != 9 || !with.heap()) return false;
  Peek<MiniSeq<int, false>> without;
  without.push(5);
  without.push_range(InputRange<int>{arr, arr + 4});
  if (without.size() != 5 || without.top() != 9 || !without.heap()) return false;
  Peek<std::vector<int>> v;
  v.push_range(ForwardRange<int>{arr, arr + 3});
  v.push_range(std::vector<int>{20, 0});
  if (!v.heap() || v.top() != 20 || v.size() != 5) return false;
  v.pop();
  return v.top() == 9 && v.heap();
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
