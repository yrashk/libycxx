// [container.alloc.reqmts]/2 and its Note 2 ("A container calls
// allocator_traits<A>::construct(m, p, args) to construct an element at p using args"): every
// object of the element type an allocator-aware sequence container makes, temporaries
// included, is constructed and destroyed through its allocator. This covers the insertions of
// single-pass ranges (input iterators, input ranges), which cannot be counted in advance, at the
// front, in the middle and at the end: [sequence.reqmts] prepend_range ("the order of elements
// in rg is not reversed"; for deque T is Cpp17MoveInsertable, MoveAssignable and Swappable),
// insert(p, i, j), insert_range, append_range, assign_range and the constructors.
#include <deque>
#include <list>
#include <vector>
#include <forward_list>
#include "reqs/allocator_construct_seq.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

using namespace reqs::allocator_construct_seq;

template <class X>
bool values(const X& x, std::initializer_list<int> want) {
  auto it = x.begin();
  for (int v : want) {
    if (it == x.end() || it->v != v) return false;
    ++it;
  }
  return it == x.end();
}

template <class X>
bool run() {
  using A = typename X::allocator_type;
  Val arr[4] = {{1}, {2}, {3}, {4}};
  Val more[3] = {{7}, {8}, {9}};
  X x(InputIter<Val>(arr), InputIter<Val>(arr + 4), A(1));
  if (!values(x, {1, 2, 3, 4})) return false;
  if constexpr (requires { x.prepend_range(InputRange<Val>{more, more + 3}); }) {
    x.prepend_range(InputRange<Val>{more, more + 3});
    if (!values(x, {7, 8, 9, 1, 2, 3, 4})) return false;
    x.prepend_range(InputRange<Val>{arr, arr + 2});
    if (!values(x, {1, 2, 7, 8, 9, 1, 2, 3, 4})) return false;
    x.assign(InputIter<Val>(arr), InputIter<Val>(arr + 4));
  }
  if constexpr (requires { x.insert(x.cbegin(), InputIter<Val>(), InputIter<Val>()); }) {
    x.insert(x.cbegin(), InputIter<Val>(more), InputIter<Val>(more + 3));
    if (!values(x, {7, 8, 9, 1, 2, 3, 4})) return false;
    x.insert(std::next(x.cbegin(), 5), InputIter<Val>(arr), InputIter<Val>(arr + 3));
    if (!values(x, {7, 8, 9, 1, 2, 1, 2, 3, 3, 4})) return false;
    x.insert_range(std::next(x.cbegin(), 2), InputRange<Val>{more, more + 2});
    if (!values(x, {7, 8, 7, 8, 9, 1, 2, 1, 2, 3, 3, 4})) return false;
    x.append_range(InputRange<Val>{arr, arr + 1});
    if (!values(x, {7, 8, 7, 8, 9, 1, 2, 1, 2, 3, 3, 4, 1})) return false;
  } else {
    x.insert_after(x.cbefore_begin(), InputIter<Val>(more), InputIter<Val>(more + 3));
    if (!values(x, {7, 8, 9, 1, 2, 3, 4})) return false;
    x.insert_range_after(std::next(x.cbefore_begin(), 2), InputRange<Val>{arr, arr + 2});
    if (!values(x, {7, 8, 1, 2, 9, 1, 2, 3, 4})) return false;
  }
  x.assign_range(InputRange<Val>{arr, arr + 2});
  if (!values(x, {1, 2})) return false;
  x.assign(InputIter<Val>(more), InputIter<Val>(more + 3));
  if (!values(x, {7, 8, 9})) return false;
  X y(std::from_range, InputRange<Val>{arr, arr + 4}, A(1));
  return values(y, {1, 2, 3, 4});
}

template <class X>
bool test_single_pass() {
  counters = Counters();
  bool ok = run<X>();
  return ok && counters.outside_constructs == 0 && counters.outside_destroys == 0 && counters.live == 0 &&
         counters.constructs == counters.destroys;
}

int main() {
  CHECK(test_single_pass<std::deque<Tracked, ConstructAlloc<Tracked>>>());
  CHECK(test_single_pass<std::vector<Tracked, ConstructAlloc<Tracked>>>());
  CHECK(test_single_pass<std::list<Tracked, ConstructAlloc<Tracked>>>());
  CHECK(test_single_pass<std::forward_list<Tracked, ConstructAlloc<Tracked>>>());
  return 0;
}
