// [queue.mod]/1: push_range(rg) is c.append_range(std::forward<R>(rg)) if that is a valid
// expression, otherwise ranges::copy(rg, back_inserter(c)). The elements are appended in
// order behind the existing ones.
#include <queue>
#include <list>
#include <vector>
#include "mini_sequence.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class C>
struct Peek : std::queue<int, C> {
  constexpr C& cont() { return this->c; }
};

constexpr bool test() {
  int arr[] = {1, 2, 3};
  Peek<MiniSeq<int, true>> with;
  with.push(0);
  with.push_range(arr);
  if (with.cont().append_calls != 1 || with.size() != 4 || with.front() != 0 || with.back() != 3) return false;
  Peek<MiniSeq<int, false>> without;
  without.push_range(InputRange<int>{arr, arr + 3});
  if (without.size() != 3 || without.front() != 1 || without.back() != 3) return false;
  std::queue<int, std::list<int>> l;
  l.push_range(ForwardRange<int>{arr, arr + 2});
  l.push_range(std::vector<int>{7});
  if (l.size() != 3 || l.front() != 1 || l.back() != 7) return false;
  l.pop();
  return l.front() == 2;
}

static_assert(test());

int main() {
  CHECK(test());
  std::queue<int> d;
  int arr[] = {5, 6};
  d.push_range(arr);
  CHECK(d.front() == 5 && d.back() == 6);
  return 0;
}
