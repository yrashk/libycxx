// [forward.list.iter]: before_begin() is not dereferenceable, ++before_begin() == begin(), and
// before_begin() != end(). [forward.list.modifiers]: insert_after(p, x) / (p, rv) /
// emplace_after(p, args) return an iterator to the new element; insert_after(p, n, x),
// insert_after(p, first, last), insert_after(p, il) and insert_range_after(p, rg) return an
// iterator to the last inserted element, or p if nothing was inserted; erase_after(p)
// returns an iterator to the element after the erased one (or end()); erase_after(p, last)
// erases (p, last) and returns last; resize appends default-inserted elements or copies of c
// at the end, or erases the trailing elements; clear() erases everything and "Does not
// invalidate past-the-end iterators". Insertion does not affect the validity of iterators
// and references (/1).
#include <forward_list>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class T>
constexpr bool test() {
  using L = std::forward_list<T>;
  L l;
  if (l.before_begin() == l.end() || l.cbefore_begin() == l.cend()) return false;
  if (std::next(l.before_begin()) != l.begin() || std::next(l.cbefore_begin()) != l.cbegin()) return false;
  const T x = val<T>(1);
  auto it = l.insert_after(l.cbefore_begin(), x);
  if (it != l.begin() || !holds(l, {1})) return false;
  it = l.insert_after(it, val<T>(2));
  if (std::next(l.begin()) != it || !holds(l, {1, 2})) return false;
  it = l.emplace_after(l.cbefore_begin(), val<T>(0));
  if (it != l.begin() || !holds(l, {0, 1, 2})) return false;
  if (std::next(l.before_begin()) != l.begin()) return false;
  const T* p1 = std::addressof(*std::next(l.begin()));

  auto p = std::next(l.cbegin());  // at 1
  it = l.insert_after(p, std::size_t(3), val<T>(5));
  if (!holds(l, {0, 1, 5, 5, 5, 2}) || it != std::next(l.begin(), 4)) return false;
  it = l.insert_after(p, std::size_t(0), val<T>(5));
  if (it != p) return false;
  T arr[] = {val<T>(7), val<T>(8), val<T>(9)};
  it = l.insert_after(l.cbefore_begin(), arr, arr + 3);
  if (!holds(l, {7, 8, 9, 0, 1, 5, 5, 5, 2}) || it != std::next(l.begin(), 2)) return false;
  it = l.insert_after(p, InputIter<T>(arr), InputIter<T>(arr + 2));
  if (!holds(l, {7, 8, 9, 0, 1, 7, 8, 5, 5, 5, 2}) || *it != val<T>(8) || std::next(it) == l.end()) return false;
  it = l.insert_after(p, arr, arr);
  if (it != p) return false;
  it = l.insert_after(l.cbefore_begin(), {val<T>(3), val<T>(4)});
  if (!holds(l, {3, 4, 7, 8, 9, 0, 1, 7, 8, 5, 5, 5, 2}) || it != std::next(l.begin())) return false;
  it = l.insert_range_after(p, InputRange<T>{arr + 2, arr + 3});
  if (!holds(l, {3, 4, 7, 8, 9, 0, 1, 9, 7, 8, 5, 5, 5, 2}) || std::next(p) != it) return false;
  it = l.insert_range_after(p, ForwardRange<T>{arr, arr + 2});
  if (!holds(l, {3, 4, 7, 8, 9, 0, 1, 7, 8, 9, 7, 8, 5, 5, 5, 2}) || std::next(p, 2) != it) return false;
  it = l.insert_range_after(p, std::ranges::subrange(arr, arr));
  if (it != p) return false;
  if (std::addressof(*p) != p1) return false;  // insertion keeps references valid

  // erase_after
  L e{val<T>(1), val<T>(2), val<T>(3), val<T>(4), val<T>(5)};
  auto r = e.erase_after(e.cbegin());
  if (!holds(e, {1, 3, 4, 5}) || r != std::next(e.begin()) || *r != val<T>(3)) return false;
  r = e.erase_after(e.cbefore_begin());
  if (!holds(e, {3, 4, 5}) || r != e.begin()) return false;
  r = e.erase_after(std::next(e.cbegin()));
  if (!holds(e, {3, 4}) || r != e.end()) return false;
  e.push_front(val<T>(2));
  e.push_front(val<T>(1));
  auto last = std::next(e.cbegin(), 3);  // at 4
  r = e.erase_after(e.cbegin(), last);
  if (!holds(e, {1, 4}) || r != last) return false;
  r = e.erase_after(e.cbegin(), std::next(e.cbegin()));  // empty range
  if (!holds(e, {1, 4}) || r != std::next(e.begin())) return false;
  r = e.erase_after(e.cbefore_begin(), e.cend());
  if (e.begin() != e.end() || r != e.end()) return false;

  // resize
  L z{val<T>(1), val<T>(2), val<T>(3)};
  z.resize(5);
  if (count_elems(z) != 5 || *std::next(z.begin(), 4) != T() || *std::next(z.begin(), 2) != val<T>(3)) return false;
  z.resize(2);
  if (!holds(z, {1, 2})) return false;
  z.resize(4, val<T>(6));
  if (!holds(z, {1, 2, 6, 6})) return false;
  z.resize(4, val<T>(7));
  if (!holds(z, {1, 2, 6, 6})) return false;
  z.resize(0);
  if (!z.empty()) return false;

  // clear keeps end() valid
  auto end = l.end();
  l.clear();
  return l.empty() && end == l.end() && l.begin() == end;
}

static_assert(test<int>());
static_assert(test<Elem>());

int main() {
  CHECK(test<int>());
  CHECK(test<Elem>());
  return 0;
}
