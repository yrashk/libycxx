// [forward.list.modifiers]/1: the modifiers "do not affect the validity of iterators and
// references when inserting elements, and when erasing elements invalidate iterators and
// references to the erased elements only". before_begin() ([forward.list.iter]: "A
// non-dereferenceable iterator that, when incremented, is equal to the iterator returned by
// begin()") and end() are such iterators, also when they were taken from a forward_list that
// has never held an element. Every way of getting an empty forward_list is covered:
// default-initialized, value-initialized, from an allocator, forward_list(0),
// forward_list(0, t), from an empty iterator range, range or initializer list, a copy of,
// moved from or assigned from a never-filled one, swapped with one, and one that held elements
// and was cleared, erased, popped, resized or spliced ([forward.list.ops]) to empty or had them
// all removed (remove_if). On each, before_begin(), cbefore_begin(), begin(), end() and the
// iterator erase_after(before_begin(), end()) returned are saved, then the first element(s)
// are put in by each insertion member: push_front, emplace_front, insert_after,
// insert_range_after, emplace_after, prepend_range, and by splice_after ([forward.list.ops]/2-12:
// "Inserts the contents of x after position"), with the saved before_begin() as position
// where there is one. Afterwards the saved before_begin() is still the iterator before the
// first element (incrementing it gives begin()) and works as the position of insert_after,
// emplace_after, splice_after and erase_after; the saved end() is still end().
#include <forward_list>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

constexpr int n_starts = 19;

template <class T, class F>
constexpr bool from_start(int s, F f) {
  using L = std::forward_list<T>;
  switch (s) {
    case 0: {
      L c;
      return f(c);
    }
    case 1: {
      L c{};
      return f(c);
    }
    case 2: {
      L c = L();
      return f(c);
    }
    case 3: {
      L c(typename L::allocator_type{});
      return f(c);
    }
    case 4: {
      L c(typename L::size_type(0));
      return f(c);
    }
    case 5: {
      L c(typename L::size_type(0), val<T>(0));
      return f(c);
    }
    case 6: {
      T arr[1] = {val<T>(0)};
      L c(arr, arr);
      return f(c);
    }
    case 7: {
      L c(std::from_range, std::ranges::empty_view<T>{});
      return f(c);
    }
    case 8: {
      L c(std::initializer_list<T>{});
      return f(c);
    }
    case 9: {
      const L d;
      L c(d);
      return f(c);
    }
    case 10: {
      L d;
      L c(std::move(d));
      return f(c);
    }
    case 11: {
      const L d;
      L c{val<T>(0), val<T>(1)};
      c = d;
      return f(c);
    }
    case 12: {
      L d;
      L c{val<T>(0), val<T>(1)};
      c = std::move(d);
      return f(c);
    }
    case 13: {
      L d;
      L c{val<T>(0), val<T>(1)};
      c.swap(d);
      return f(c);
    }
    case 14: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      c.clear();
      return f(c);
    }
    case 15: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      c.erase_after(c.before_begin(), c.end());
      return f(c);
    }
    case 16: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      c.pop_front();
      c.erase_after(c.begin());
      c.erase_after(c.cbefore_begin());
      return f(c);
    }
    case 17: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      L d;
      d.splice_after(d.before_begin(), c);
      return f(c);
    }
    case 18: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      c.resize(0);
      c.remove_if([](const T&) { return true; });
      return f(c);
    }
  }
  return false;
}

constexpr int n_ops = 15;

// The first insertion into the empty c (values 0-3 only).
template <class T>
constexpr void first_insertion(std::forward_list<T>& c, typename std::forward_list<T>::iterator bb,
                               typename std::forward_list<T>::const_iterator cbb, int w) {
  switch (w) {
    case 0: {
      const T v = val<T>(1);
      c.push_front(v);
      break;
    }
    case 1:
      c.push_front(val<T>(1));
      break;
    case 2:
      c.emplace_front(val<T>(1));
      break;
    case 3: {
      const T v = val<T>(1);
      c.insert_after(bb, v);
      break;
    }
    case 4:
      c.insert_after(cbb, val<T>(1));
      break;
    case 5:
      c.insert_after(bb, 3, val<T>(1));
      break;
    case 6: {
      T arr[3] = {val<T>(0), val<T>(1), val<T>(2)};
      c.insert_after(cbb, arr, arr + 3);
      break;
    }
    case 7:
      c.insert_after(bb, {val<T>(0), val<T>(1), val<T>(2)});
      break;
    case 8: {
      T arr[3] = {val<T>(0), val<T>(1), val<T>(2)};
      c.insert_range_after(bb, arr);
      break;
    }
    case 9:
      c.emplace_after(cbb, val<T>(1));
      break;
    case 10: {
      T arr[2] = {val<T>(0), val<T>(1)};
      c.prepend_range(arr);
      break;
    }
    case 11: {
      std::forward_list<T> d{val<T>(0), val<T>(1), val<T>(2)};
      c.splice_after(bb, d);
      break;
    }
    case 12: {
      std::forward_list<T> d{val<T>(0), val<T>(1)};
      c.splice_after(cbb, std::move(d));
      break;
    }
    case 13: {
      std::forward_list<T> d{val<T>(0), val<T>(1), val<T>(2)};
      c.splice_after(bb, d, d.begin());
      break;
    }
    case 14: {
      std::forward_list<T> d{val<T>(0), val<T>(1), val<T>(2), val<T>(3)};
      c.splice_after(cbb, d, d.cbegin(), std::next(d.cbegin(), 3));
      break;
    }
  }
}

template <class T>
constexpr std::ptrdiff_t length(const std::forward_list<T>& c) {
  return std::distance(c.begin(), c.end());
}

// bb / cbb are the before-begin iterator and e the past-the-end iterator of the non-empty c.
template <class T>
constexpr bool before_begin_usable(std::forward_list<T>& c, typename std::forward_list<T>::iterator bb,
                                   typename std::forward_list<T>::const_iterator cbb,
                                   typename std::forward_list<T>::iterator e) {
  using It = typename std::forward_list<T>::iterator;
  if (c.empty()) return false;
  if (bb != c.before_begin() || cbb != c.cbefore_begin() || bb != c.cbefore_begin() || cbb != c.before_begin() ||
      std::as_const(c).before_begin() != cbb)
    return false;
  if (std::next(bb) != c.begin() || std::next(cbb) != c.cbegin() || e != c.end()) return false;
  if (std::addressof(*std::next(bb)) != std::addressof(c.front())) return false;
  std::ptrdiff_t n = 0;
  It last = bb;
  for (It it = std::next(bb); it != e; ++it, ++n) last = it;
  if (n != length(c) || n == 0) return false;
  const T* last_addr = std::addressof(*last);
  // as insertion positions: the new elements go first
  It h = c.insert_after(bb, val<T>(50));
  if (!(*h == val<T>(50)) || std::next(bb) != h || c.begin() != h) return false;
  It h2 = c.emplace_after(cbb, val<T>(51));
  if (!(*h2 == val<T>(51)) || std::next(bb) != h2 || std::next(h2) != h) return false;
  c.push_front(val<T>(52));
  if (!(*std::next(bb) == val<T>(52)) || std::next(bb, 2) != h2) return false;
  std::forward_list<T> d{val<T>(53)};
  c.splice_after(bb, d);
  if (!(*std::next(bb) == val<T>(53)) || std::next(bb, 3) != h2 || !d.empty()) return false;
  // as erase_after positions
  if (c.erase_after(bb) != c.begin() || !(c.front() == val<T>(52))) return false;
  if (c.erase_after(cbb) != h2 || c.begin() != h2) return false;
  if (std::addressof(*last) != last_addr || std::next(last) != e) return false;
  if (c.erase_after(bb, e) != e || !c.empty() || c.begin() != e || std::next(bb) != e || c.end() != e) return false;
  // and once more after emptying
  It h3 = c.insert_after(cbb, val<T>(5));
  if (c.begin() != h3 || std::next(bb) != h3 || std::next(h3) != e || c.before_begin() != bb) return false;
  c.erase_after(bb);
  return c.empty() && c.before_begin() == bb && c.end() == e;
}

template <class T>
constexpr bool one(int s, int w) {
  return from_start<T>(s, [w](std::forward_list<T>& c) {
    using It = typename std::forward_list<T>::iterator;
    using CIt = typename std::forward_list<T>::const_iterator;
    if (!c.empty() || c.begin() != c.end()) return false;
    const It bb = c.before_begin();
    const CIt cbb = c.cbefore_begin();
    const It b = c.begin();
    const It e = c.end();
    const It er = c.erase_after(c.before_begin(), c.end());
    if (std::next(bb) != c.end()) return false;
    first_insertion(c, bb, cbb, w);
    if (c.empty()) return false;
    // b, e and er were all past-the-end; they still are
    if (b != c.end() || e != c.end() || er != c.end()) return false;
    return before_begin_usable(c, bb, cbb, e);
  });
}

template <class T>
constexpr bool test() {
  for (int s = 0; s < n_starts; ++s)
    for (int w = 0; w < n_ops; ++w)
      if (!one<T>(s, w)) return false;
  return true;
}

static_assert(test<int>());
static_assert(test<Elem>());

// A forward_list with static storage duration (possibly constant-initialized), filled for
// the first time in main.
std::forward_list<int> global;

template <class T>
void run(const char* name) {
  for (int s = 0; s < n_starts; ++s)
    for (int w = 0; w < n_ops; ++w) CHECK_SAY(one<T>(s, w), "%s: start state %d, insertion %d", name, s, w);
}

int main() {
  run<int>("forward_list<int>");
  run<Elem>("forward_list<Elem>");
  {
    const auto bb = global.before_begin();
    const auto e = global.end();
    global.push_front(1);
    CHECK(bb == global.before_begin());
    CHECK(std::next(bb) == global.begin());
    CHECK(e == global.end());
    global.insert_after(bb, 2);
    CHECK(global.front() == 2);
    CHECK(std::next(bb, 3) == e);
    CHECK(global.erase_after(bb, e) == e);
    CHECK(global.empty());
    CHECK(global.before_begin() == bb);
  }
  return 0;
}
