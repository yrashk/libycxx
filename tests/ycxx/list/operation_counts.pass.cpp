// [list.modifiers]/1: "Insertion of a single element into a list takes constant time and
// exactly one call to a constructor of T. Insertion of multiple elements into a list is
// linear in the number of elements inserted, and the number of calls to the copy constructor
// or move constructor of T is exactly equal to the number of elements inserted." /2: "If an
// exception is thrown, there are no effects." /5: erasing a single element calls the
// destructor of T once, and erasing a range calls it exactly the size of the range times.
// No assignment operator of T is ever needed.
// REQUIRES: exceptions
#include <list>
#include <cstddef>
#include <iterator>
#include "check.hpp"

struct Bomb {};
struct Counts {
  int ctors = 0, copies_moves = 0, dtors = 0, assigns = 0;
  int throw_after = -1;  // the copy constructor throws once this many copies were made
};
inline Counts counts;

struct T {
  int v;
  T(int x) : v(x) { ++counts.ctors; }
  T(const T& o) : v(o.v) {
    if (counts.throw_after == 0) throw Bomb{};
    if (counts.throw_after > 0) --counts.throw_after;
    ++counts.ctors;
    ++counts.copies_moves;
  }
  T(T&& o) noexcept : v(o.v) {
    ++counts.ctors;
    ++counts.copies_moves;
  }
  T& operator=(const T& o) { v = o.v; ++counts.assigns; return *this; }
  T& operator=(T&& o) noexcept { v = o.v; ++counts.assigns; return *this; }
  ~T() { ++counts.dtors; }
};

bool values(const std::list<T>& l, std::initializer_list<int> want) {
  if (l.size() != want.size()) return false;
  auto w = want.begin();
  for (const T& t : l)
    if (t.v != *w++) return false;
  return true;
}

int main() {
  std::list<T> l;
  for (int i = 0; i < 4; ++i) l.emplace_back(i);
  const T t(9);
  T arr[5] = {T(20), T(21), T(22), T(23), T(24)};
  auto mid = std::next(l.begin(), 2);

  counts = {};
  l.insert(mid, t);
  l.push_front(t);
  l.push_back(t);
  l.emplace(mid, 7);
  l.emplace_front(7);
  l.emplace_back(7);
  CHECK(counts.ctors == 6 && counts.assigns == 0 && counts.dtors == 0);

  counts = {};
  l.insert(mid, arr, arr + 5);
  l.insert(mid, std::size_t(3), t);
  l.insert_range(mid, arr);
  l.append_range(arr);
  l.prepend_range(arr);
  CHECK(counts.copies_moves == 23 && counts.ctors == 23 && counts.assigns == 0 && counts.dtors == 0);

  // erasure: one destructor call per erased element
  std::size_t before = l.size();
  counts = {};
  l.erase(mid);
  l.pop_front();
  l.pop_back();
  CHECK(counts.dtors == 3 && counts.assigns == 0);
  counts = {};
  auto last = l.begin();
  std::advance(last, 10);
  l.erase(l.begin(), last);
  CHECK(counts.dtors == 10 && l.size() == before - 13);
  counts = {};
  std::size_t rest = l.size();
  l.clear();
  CHECK(counts.dtors == static_cast<int>(rest) && l.empty());

  // exceptions: no effects
  l.clear();
  for (int i = 0; i < 4; ++i) l.emplace_back(i);
  mid = std::next(l.begin(), 2);
  counts = {};
  counts.throw_after = 0;
  int thrown = 0;
  try { l.push_back(t); } catch (Bomb) { ++thrown; }
  try { l.push_front(t); } catch (Bomb) { ++thrown; }
  try { l.insert(mid, t); } catch (Bomb) { ++thrown; }
  counts.throw_after = 2;
  try { l.insert(mid, arr, arr + 5); } catch (Bomb) { ++thrown; }
  CHECK(values(l, {0, 1, 2, 3}));
  counts.throw_after = 3;
  try { l.insert(mid, std::size_t(5), t); } catch (Bomb) { ++thrown; }
  CHECK(values(l, {0, 1, 2, 3}));
  counts.throw_after = 1;
  try { l.append_range(arr); } catch (Bomb) { ++thrown; }
  counts.throw_after = 4;
  try { l.prepend_range(arr); } catch (Bomb) { ++thrown; }
  counts.throw_after = 2;
  try { l.insert_range(mid, arr); } catch (Bomb) { ++thrown; }
  CHECK(thrown == 8);
  CHECK(values(l, {0, 1, 2, 3}));
  // the partially constructed elements were destroyed again: constructions == destructions
  CHECK(counts.ctors == counts.dtors);
  counts.throw_after = -1;
  l.insert(mid, arr, arr + 2);
  CHECK(values(l, {0, 1, 20, 21, 2, 3}));
  return 0;
}
