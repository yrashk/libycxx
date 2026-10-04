// [stack.mod]/1: push_range(rg) is c.append_range(std::forward<R>(rg)) if that is a valid
// expression, otherwise ranges::copy(rg, back_inserter(c)). Checked with a program-defined
// container with and without append_range, and with the standard containers; the pushed
// elements keep their order, so the last element of rg ends up on top.
#include <stack>
#include <deque>
#include <vector>
#include "mini_sequence.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class C>
struct Peek : std::stack<int, C> {
  constexpr C& cont() { return this->c; }
};

constexpr bool test() {
  int arr[] = {1, 2, 3};
  Peek<MiniSeq<int, true>> with;
  with.push(0);
  with.push_range(arr);
  if (with.cont().append_calls != 1 || with.size() != 4 || with.top() != 3) return false;
  Peek<MiniSeq<int, false>> without;
  without.push_range(InputRange<int>{arr, arr + 3});
  if (without.size() != 3 || without.top() != 3 || without.cont().data[0] != 1) return false;
  std::stack<int, std::vector<int>> v;
  v.push_range(ForwardRange<int>{arr, arr + 2});
  v.push_range(std::vector<int>{7});
  if (v.size() != 3 || v.top() != 7) return false;
  v.pop();
  return v.top() == 2;
}

static_assert(test());

int main() {
  CHECK(test());
  std::stack<int> d;
  int arr[] = {5, 6};
  d.push_range(arr);
  CHECK(d.top() == 6 && d.size() == 2);
  return 0;
}
