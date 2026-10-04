// [list.ops]/3-14: splice(position, x) inserts the contents of x before position and x becomes
// empty; splice(position, x, i) moves the element *i before position ("The result is
// unchanged if position == i or position == ++i"); splice(position, x, first, last) moves
// [first, last), also within the same list (addressof(x) == this, position outside the
// range). In every case "Pointers and references to the moved elements of x now refer to
// those same elements but as members of *this. Iterators referring to the moved elements
// will continue to refer to their elements, but they now behave as iterators into *this".
// Lvalue and rvalue x overloads both exist. size() follows the moved elements.
#include <list>
#include <iterator>
#include <memory>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr std::list<T> mk(std::initializer_list<int> idx) {
  std::list<T> l;
  for (int i : idx) l.push_back(val<T>(i));
  return l;
}

template <class T>
constexpr bool whole() {
  auto a = mk<T>({1, 2, 3});
  auto b = mk<T>({4, 5});
  auto i4 = b.begin();
  T* p5 = std::addressof(*std::next(b.begin()));
  a.splice(std::next(a.cbegin()), b);
  if (!holds(a, {1, 4, 5, 2, 3}) || !b.empty() || b.size() != 0 || a.size() != 5) return false;
  // i4 is now an iterator into a: it can be walked to a.end()
  if (std::next(i4, 4) != a.end() || std::addressof(*std::next(i4)) != p5) return false;
  if (std::prev(i4) != a.begin()) return false;
  a.splice(a.cend(), mk<T>({6, 7}));  // rvalue
  if (!holds(a, {1, 4, 5, 2, 3, 6, 7}) || a.size() != 7) return false;
  std::list<T> e;
  a.splice(a.cbegin(), e);
  if (a.size() != 7 || !holds(a, {1, 4, 5, 2, 3, 6, 7})) return false;
  e.splice(e.cend(), a);
  return a.empty() && e.size() == 7 && holds(e, {1, 4, 5, 2, 3, 6, 7}) && std::next(i4, 6) == e.end();
}

template <class T>
constexpr bool single() {
  auto a = mk<T>({1, 2, 3});
  auto b = mk<T>({4, 5, 6});
  auto i5 = std::next(b.begin());
  T* p5 = std::addressof(*i5);
  a.splice(a.cbegin(), b, i5);
  if (!holds(a, {5, 1, 2, 3}) || !holds(b, {4, 6}) || a.size() != 4 || b.size() != 2) return false;
  if (i5 != a.begin() || std::addressof(a.front()) != p5) return false;
  // within the same list: position == i and position == ++i leave it unchanged
  a.splice(a.cbegin(), a, a.cbegin());
  if (!holds(a, {5, 1, 2, 3})) return false;
  a.splice(std::next(a.cbegin()), a, a.cbegin());
  if (!holds(a, {5, 1, 2, 3}) || a.size() != 4) return false;
  // within the same list: a real move, last element to front and first to end
  auto i3 = std::prev(a.end());
  a.splice(a.cbegin(), a, i3);
  if (!holds(a, {3, 5, 1, 2}) || i3 != a.begin() || a.size() != 4) return false;
  a.splice(a.cend(), a, a.cbegin());
  if (!holds(a, {5, 1, 2, 3}) || std::next(i3) != a.end()) return false;
  // rvalue overload
  a.splice(a.cend(), std::move(b), b.cbegin());
  return holds(a, {5, 1, 2, 3, 4}) && holds(b, {6}) && b.size() == 1;
}

template <class T>
constexpr bool range() {
  auto a = mk<T>({1, 2, 3});
  auto b = mk<T>({4, 5, 6, 7, 8});
  auto first = std::next(b.begin());
  auto last = std::next(b.begin(), 4);
  T* p5 = std::addressof(*first);
  a.splice(std::next(a.cbegin(), 2), b, first, last);
  if (!holds(a, {1, 2, 5, 6, 7, 3}) || !holds(b, {4, 8}) || a.size() != 6 || b.size() != 2) return false;
  if (std::addressof(*first) != p5 || std::next(first, 3) != std::prev(a.end())) return false;
  if (*last != val<T>(8) || std::next(last) != b.end()) return false;
  // empty range
  a.splice(a.cbegin(), b, b.cbegin(), b.cbegin());
  if (a.size() != 6 || b.size() != 2) return false;
  // whole of b as a range, rvalue overload
  a.splice(a.cbegin(), std::move(b), b.cbegin(), b.cend());
  if (!holds(a, {4, 8, 1, 2, 5, 6, 7, 3}) || !b.empty() || a.size() != 8) return false;
  // within the same list (position outside [first, last))
  a.splice(a.cbegin(), a, std::next(a.cbegin(), 5), a.cend());
  if (!holds(a, {6, 7, 3, 4, 8, 1, 2, 5}) || a.size() != 8) return false;
  a.splice(a.cend(), a, a.cbegin(), std::next(a.cbegin(), 3));
  if (!holds(a, {4, 8, 1, 2, 5, 6, 7, 3}) || a.size() != 8) return false;
  a.splice(std::next(a.cbegin(), 4), a, std::next(a.cbegin()), std::next(a.cbegin(), 3));
  return holds(a, {4, 2, 8, 1, 5, 6, 7, 3}) && a.size() == 8;
}

static_assert(whole<int>() && single<int>() && range<int>());
static_assert(whole<Elem>() && single<Elem>() && range<Elem>());

int main() {
  CHECK(whole<int>() && single<int>() && range<int>());
  CHECK(whole<Elem>() && single<Elem>() && range<Elem>());
  return 0;
}
