// [forward.list.modifiers]/1: "Inserting n elements into a forward_list is linear in n, and
// the number of calls to the copy or move constructor of T is exactly equal to n. Erasing n
// elements from a forward_list is linear in n and the number of calls to the destructor of
// type T is exactly equal to n." and "If an exception is thrown by any of these member
// functions there is no effect on the container."
// REQUIRES: exceptions
#include <forward_list>
#include <cstddef>
#include <iterator>
#include "check.hpp"

struct Bomb {};
struct Counts {
  int copies_moves = 0, dtors = 0, assigns = 0;
  int throw_after = -1;
};
inline Counts counts;

struct T {
  int v;
  T(int x) : v(x) {}
  T(const T& o) : v(o.v) {
    if (counts.throw_after == 0) throw Bomb{};
    if (counts.throw_after > 0) --counts.throw_after;
    ++counts.copies_moves;
  }
  T(T&& o) noexcept : v(o.v) { ++counts.copies_moves; }
  T& operator=(const T& o) { v = o.v; ++counts.assigns; return *this; }
  T& operator=(T&& o) noexcept { v = o.v; ++counts.assigns; return *this; }
  ~T() { ++counts.dtors; }
};

bool values(const std::forward_list<T>& l, std::initializer_list<int> want) {
  auto w = want.begin();
  for (const T& t : l)
    if (w == want.end() || t.v != *w++) return false;
  return w == want.end();
}

int main() {
  std::forward_list<T> l;
  for (int i = 3; i >= 0; --i) l.emplace_front(i);
  T arr[5] = {T(20), T(21), T(22), T(23), T(24)};
  const T t(9);
  auto p = std::next(l.cbegin());
  counts = {};
  l.insert_after(p, t);
  l.push_front(t);
  l.insert_after(p, std::size_t(3), t);
  l.insert_after(p, arr, arr + 5);
  l.insert_range_after(p, arr);
  l.prepend_range(arr);
  l.insert_after(p, T(5));  // one move of the temporary
  CHECK(counts.copies_moves == 1 + 1 + 3 + 5 + 5 + 5 + 1 && counts.assigns == 0);
  int n = 0;
  for (auto& x : l) (void)x, ++n;
  counts = {};
  l.erase_after(p);
  l.pop_front();
  l.erase_after(l.cbefore_begin(), std::next(l.cbegin(), 6));
  CHECK(counts.dtors == 1 + 1 + 6 && counts.assigns == 0);
  counts = {};
  l.clear();
  CHECK(counts.dtors == n - 8 && l.empty());

  for (int i = 3; i >= 0; --i) l.emplace_front(i);
  p = std::next(l.cbegin());
  int thrown = 0;
  counts.throw_after = 0;
  try { l.push_front(t); } catch (Bomb) { ++thrown; }
  try { l.insert_after(p, t); } catch (Bomb) { ++thrown; }
  counts.throw_after = 2;
  try { l.insert_after(p, arr, arr + 5); } catch (Bomb) { ++thrown; }
  counts.throw_after = 1;
  try { l.insert_after(p, std::size_t(4), t); } catch (Bomb) { ++thrown; }
  counts.throw_after = 3;
  try { l.insert_range_after(l.cbefore_begin(), arr); } catch (Bomb) { ++thrown; }
  counts.throw_after = 4;
  try { l.prepend_range(arr); } catch (Bomb) { ++thrown; }
  counts.throw_after = 2;
  try { l.resize(10, t); } catch (Bomb) { ++thrown; }
  CHECK(thrown == 7);
  CHECK(values(l, {0, 1, 2, 3}));
  counts.throw_after = -1;
  l.insert_after(p, arr, arr + 2);
  CHECK(values(l, {0, 1, 20, 21, 2, 3}));
  return 0;
}
