// [hive.modifiers]: emplace / insert return an iterator to the new element and construct
// exactly one T; insertion invalidates only the past-the-end iterator (/5), so pointers,
// references and iterators to other elements stay valid; erase invalidates only the erased
// elements (and end() when the last element goes) (/17) and returns the iterator following
// the erased element(s); insert(n, x), insert(i, j), insert(il) and insert_range insert
// copies, dereferencing each iterator once (/8). [hive.overview]/1, /3: insertion may
// reuse the memory of erased elements, and erasure does not relocate elements.
// [hive.operations]/17-18: get_iterator(p) returns an iterator to the element p points to.
#include <hive>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>
#include "test_iterators.hpp"
#include "check.hpp"

struct Counted {
  static inline int ctors = 0;
  int v;
  Counted(int x) : v(x) { ++ctors; }
  Counted(const Counted& o) : v(o.v) { ++ctors; }
  Counted(Counted&& o) noexcept : v(o.v) { ++ctors; }
  Counted& operator=(const Counted&) = default;
  Counted& operator=(Counted&&) = default;
};

int main() {
  std::hive<int> h;
  std::vector<std::hive<int>::iterator> its;
  std::vector<int*> ptrs;
  for (int i = 0; i < 1000; ++i) {
    auto it = h.insert(i);
    CHECK(*it == i);
    its.push_back(it);
    ptrs.push_back(&*it);
  }
  CHECK(h.size() == 1000 && !h.empty());
  // every element is still where it was, reachable through its iterator and get_iterator
  for (int i = 0; i < 1000; ++i) {
    CHECK(*its[static_cast<std::size_t>(i)] == i && &*its[static_cast<std::size_t>(i)] == ptrs[static_cast<std::size_t>(i)]);
    CHECK(h.get_iterator(ptrs[static_cast<std::size_t>(i)]) == its[static_cast<std::size_t>(i)]);
  }
  // iteration visits every element once
  std::vector<int> seen(h.begin(), h.end());
  std::sort(seen.begin(), seen.end());
  for (int i = 0; i < 1000; ++i) CHECK(seen[static_cast<std::size_t>(i)] == i);
  // erase every odd element; the even ones do not move
  for (int i = 1; i < 1000; i += 2) {
    auto next = std::next(its[static_cast<std::size_t>(i)]);
    auto r = h.erase(its[static_cast<std::size_t>(i)]);
    CHECK(r == next);
  }
  CHECK(h.size() == 500);
  for (int i = 0; i < 1000; i += 2) CHECK(&*its[static_cast<std::size_t>(i)] == ptrs[static_cast<std::size_t>(i)] && *its[static_cast<std::size_t>(i)] == i);
  // new insertions keep the survivors in place
  for (int i = 0; i < 300; ++i) h.emplace(5000 + i);
  CHECK(h.size() == 800);
  for (int i = 0; i < 1000; i += 2) CHECK(&*its[static_cast<std::size_t>(i)] == ptrs[static_cast<std::size_t>(i)]);
  // erase a range: returns its end
  auto first = h.begin();
  auto last = std::next(first, 10);
  auto r = h.erase(first, last);
  CHECK(r == last && h.size() == 790);
  CHECK(h.erase(h.cbegin(), h.cbegin()) == h.begin());
  // bulk insertion forms
  std::hive<int> b;
  b.insert(std::size_t(5), 7);
  b.insert({1, 2});
  int arr[] = {8, 9, 10};
  int derefs = 0;
  b.insert_range(InputRange<int>{arr, arr + 3, &derefs});
  CHECK(derefs == 3);
  b.insert(arr, arr + 2);
  b.insert(InputIter<int>(arr), InputIter<int>(arr + 1));
  CHECK(b.size() == 13 && std::count(b.begin(), b.end(), 7) == 5 && std::count(b.begin(), b.end(), 8) == 3);
  auto hint = b.insert(b.cbegin(), 42);
  CHECK(*hint == 42 && *b.emplace_hint(b.cend(), 43) == 43);
  // exactly one T constructed per inserted element
  std::hive<Counted> c;
  Counted::ctors = 0;
  c.emplace(1);
  CHECK(Counted::ctors == 1);
  const Counted x(2);
  Counted::ctors = 0;
  c.insert(x);
  c.insert(std::size_t(4), x);
  Counted arr2[] = {Counted(1), Counted(2)};
  Counted::ctors = 0;
  c.insert_range(arr2);
  CHECK(Counted::ctors == 2 && c.size() == 8);
  // emplace with an argument referring to an element of the hive
  std::hive<int> s{1, 2, 3};
  for (int i = 0; i < 100; ++i) s.emplace(*s.begin());
  CHECK(s.size() == 103);
  // clear
  s.clear();
  CHECK(s.empty() && s.begin() == s.end() && s.size() == 0);
  return 0;
}
