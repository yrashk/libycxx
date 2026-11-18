// [list.modifiers]/2: insert, emplace, emplace_front, emplace_back, prepend_range,
// append_range, push_front, push_back and insert_range "Does not affect the validity of
// iterators and references"; /3: erase, pop_front, pop_back and clear "Invalidates only the
// iterators and references to the erased elements". The past-the-end iterator
// ([container.reqmts]/31: end() returns "An iterator which is the past-the-end value for the
// container") is one of those iterators, also when it was taken from a list that has never
// held an element. Every way of getting an empty list is covered: default-initialized,
// value-initialized, from an allocator, list(0), list(0, t), from an empty iterator range,
// range or initializer list, a copy of, moved from or assigned from a never-filled one,
// swapped with one, and one that held elements and was cleared, erased, popped, resized
// ([list.capacity]) or spliced ([list.ops]) to empty or had them all removed ([list.ops]
// remove_if). On each, end(), cend(), begin(), rbegin() and the iterator erase(begin(), end())
// returned are saved, then the first element(s) are put in by each insertion member, and by
// splice ([list.ops]/2-12: "Inserts the contents of x before position"), with the saved end()
// as position where there is one. Afterwards the saved iterators are still the past-the-end
// iterator: equal to end(), std::prev of them is the last element, reverse iteration from
// reverse_iterator(e) visits every element, they work as insert / emplace / splice positions
// (the new element goes last) and as the end of erase(begin(), e) ([sequence.reqmts]: erase(q1,
// q2) returns q2).
#include <list>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

constexpr int n_starts = 20;

template <class T, class F>
constexpr bool from_start(int s, F f) {
  using L = std::list<T>;
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
      c.erase(c.begin(), c.end());
      return f(c);
    }
    case 16: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      c.pop_front();
      c.pop_back();
      c.erase(c.begin());
      return f(c);
    }
    case 17: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      L d;
      d.splice(d.end(), c);
      return f(c);
    }
    case 18: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      c.resize(0);
      return f(c);
    }
    case 19: {
      L c{val<T>(0), val<T>(1), val<T>(2)};
      c.remove_if([](const T&) { return true; });
      return f(c);
    }
  }
  return false;
}

constexpr int n_ops = 18;

// The first insertion into the empty c (values 0-3 only).
template <class T>
constexpr void first_insertion(std::list<T>& c, typename std::list<T>::iterator e,
                               typename std::list<T>::const_iterator ce, int w) {
  switch (w) {
    case 0: {
      const T v = val<T>(1);
      c.push_back(v);
      break;
    }
    case 1:
      c.push_back(val<T>(1));
      break;
    case 2:
      c.push_front(val<T>(1));
      break;
    case 3:
      c.emplace_back(val<T>(1));
      break;
    case 4:
      c.emplace_front(val<T>(1));
      break;
    case 5: {
      const T v = val<T>(1);
      c.insert(e, v);
      break;
    }
    case 6:
      c.insert(ce, val<T>(1));
      break;
    case 7:
      c.insert(e, 3, val<T>(1));
      break;
    case 8: {
      T arr[3] = {val<T>(0), val<T>(1), val<T>(2)};
      c.insert(ce, arr, arr + 3);
      break;
    }
    case 9:
      c.insert(e, {val<T>(0), val<T>(1), val<T>(2)});
      break;
    case 10: {
      T arr[3] = {val<T>(0), val<T>(1), val<T>(2)};
      c.insert_range(e, arr);
      break;
    }
    case 11:
      c.emplace(ce, val<T>(1));
      break;
    case 12: {
      T arr[2] = {val<T>(0), val<T>(1)};
      c.append_range(arr);
      break;
    }
    case 13: {
      T arr[2] = {val<T>(0), val<T>(1)};
      c.prepend_range(arr);
      break;
    }
    case 14: {
      std::list<T> d{val<T>(0), val<T>(1), val<T>(2)};
      c.splice(e, d);
      break;
    }
    case 15: {
      std::list<T> d{val<T>(0), val<T>(1)};
      c.splice(ce, std::move(d));
      break;
    }
    case 16: {
      std::list<T> d{val<T>(0), val<T>(1), val<T>(2)};
      c.splice(e, d, std::next(d.begin()));
      break;
    }
    case 17: {
      std::list<T> d{val<T>(0), val<T>(1), val<T>(2), val<T>(3)};
      c.splice(ce, d, std::next(d.cbegin()), std::prev(d.cend()));
      break;
    }
  }
}

// e / ce are the past-the-end iterator of the non-empty c.
template <class T>
constexpr bool end_usable(std::list<T>& c, typename std::list<T>::iterator e, typename std::list<T>::const_iterator ce) {
  using It = typename std::list<T>::iterator;
  using RIt = typename std::list<T>::reverse_iterator;
  if (c.empty()) return false;
  if (e != c.end() || ce != c.cend() || e != c.cend() || ce != c.end() || std::as_const(c).end() != ce) return false;
  std::size_t n = 0;
  It last = e;
  for (It it = c.begin(); it != e; ++it, ++n) last = it;
  if (n != c.size() || std::prev(e) != last || std::prev(ce) != last) return false;
  if (std::addressof(*std::prev(e)) != std::addressof(c.back())) return false;
  n = 0;
  for (It it = e; it != c.begin(); ++n) --it;
  if (n != c.size()) return false;
  RIt r(e);
  if (r != c.rbegin() || std::addressof(*r) != std::addressof(c.back())) return false;
  n = 0;
  for (; r != c.rend(); ++r) ++n;
  if (n != c.size()) return false;
  // as insertion positions: the new elements go last
  const T* first_addr = std::addressof(c.front());
  It h = c.insert(e, val<T>(50));
  if (!(*h == val<T>(50)) || std::next(h) != e || e != c.end() || std::addressof(c.back()) != std::addressof(*h))
    return false;
  It h2 = c.emplace(ce, val<T>(51));
  if (!(*h2 == val<T>(51)) || std::next(h2) != e || std::prev(h2) != h || ce != c.cend()) return false;
  c.push_back(val<T>(52));
  if (!(*std::prev(e) == val<T>(52)) || std::prev(e, 2) != h2) return false;
  std::list<T> d{val<T>(53)};
  c.splice(e, d);
  if (!(*std::prev(e) == val<T>(53)) || std::prev(e, 3) != h2 || e != c.end()) return false;
  if (std::addressof(c.front()) != first_addr) return false;
  // as the end of an erased range
  if (c.erase(c.begin(), e) != e || !c.empty() || c.begin() != e || c.end() != e || c.cend() != ce) return false;
  // and once more after emptying
  c.push_front(val<T>(5));
  if (c.end() != e || std::prev(e) != c.begin() || !(*std::prev(e) == val<T>(5))) return false;
  c.insert(e, val<T>(6));
  if (!(*std::prev(e) == val<T>(6)) || std::prev(e, 2) != c.begin()) return false;
  c.pop_back();
  c.pop_back();
  return c.empty() && c.end() == e;
}

template <class T>
constexpr bool one(int s, int w) {
  return from_start<T>(s, [w](std::list<T>& c) {
    using It = typename std::list<T>::iterator;
    using CIt = typename std::list<T>::const_iterator;
    if (!c.empty() || c.begin() != c.end()) return false;
    const It e = c.end();
    const CIt ce = c.cend();
    const It b = c.begin();
    const CIt cb = std::as_const(c).begin();
    const It er = c.erase(c.begin(), c.end());
    const typename std::list<T>::reverse_iterator rb = c.rbegin();
    first_insertion(c, e, ce, w);
    if (c.empty()) return false;
    // every iterator saved from the never-filled list is still the past-the-end one
    if (b != c.end() || cb != c.cend() || er != c.end()) return false;
    if (rb != c.rbegin() || std::addressof(*rb) != std::addressof(c.back())) return false;
    return end_usable(c, e, ce);
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

// A list with static storage duration (possibly constant-initialized), filled for the first
// time in main.
std::list<int> global;

template <class T>
void run(const char* name) {
  for (int s = 0; s < n_starts; ++s)
    for (int w = 0; w < n_ops; ++w) CHECK_SAY(one<T>(s, w), "%s: start state %d, insertion %d", name, s, w);
}

int main() {
  run<int>("list<int>");
  run<Elem>("list<Elem>");
  {
    const auto e = global.end();
    const auto ce = global.cend();
    global.push_back(1);
    CHECK(e == global.end());
    CHECK(ce == global.cend());
    CHECK(std::prev(e) == global.begin());
    CHECK(*std::prev(e) == 1);
    global.insert(e, 2);
    CHECK(global.back() == 2);
    CHECK(std::next(global.begin(), 2) == e);
    CHECK(global.erase(global.begin(), e) == e);
    CHECK(global.empty());
    CHECK(global.end() == e);
  }
  return 0;
}
