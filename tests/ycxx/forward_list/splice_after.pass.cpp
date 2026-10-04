// [forward.list.ops]/2-12: splice_after(position, x) inserts the contents of x after
// position and x becomes empty; splice_after(position, x, i) moves the element following i
// ("The result is unchanged if position == i or position == ++i"); splice_after(position, x,
// first, last) moves the open range (first, last). Lvalue and rvalue x overloads exist, and
// the single-element and range forms also work within one list. "Pointers and references to
// the moved elements of x now refer to those same elements but as members of *this.
// Iterators referring to the moved elements will continue to refer to their elements, but
// they now behave as iterators into *this, not into x."
#include <forward_list>
#include <iterator>
#include <memory>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr std::forward_list<T> mk(std::initializer_list<int> idx) {
  return make<std::forward_list<T>>(idx);
}

template <class T>
constexpr bool test() {
  using L = std::forward_list<T>;
  L a = mk<T>({1, 2, 3});
  L b = mk<T>({4, 5});
  auto i4 = b.begin();
  const T* p5 = std::addressof(*std::next(b.begin()));
  a.splice_after(a.cbegin(), b);
  if (!holds(a, {1, 4, 5, 2, 3}) || !b.empty()) return false;
  if (std::next(i4, 4) != a.end() || std::addressof(*std::next(i4)) != p5) return false;
  a.splice_after(a.cbefore_begin(), mk<T>({6}));
  if (!holds(a, {6, 1, 4, 5, 2, 3})) return false;
  L empty;
  a.splice_after(a.cbegin(), empty);
  if (!holds(a, {6, 1, 4, 5, 2, 3})) return false;

  // single element
  L c = mk<T>({7, 8, 9});
  auto i8 = std::next(c.begin());
  a.splice_after(a.cbefore_begin(), c, c.cbegin());  // moves 8
  if (!holds(a, {8, 6, 1, 4, 5, 2, 3}) || !holds(c, {7, 9}) || i8 != a.begin()) return false;
  a.splice_after(a.cbefore_begin(), std::move(c), c.cbefore_begin());  // moves 7, rvalue
  if (!holds(a, {7, 8, 6, 1, 4, 5, 2, 3}) || !holds(c, {9})) return false;
  // unchanged when position == i or position == ++i
  a.splice_after(a.cbegin(), a, a.cbegin());
  if (!holds(a, {7, 8, 6, 1, 4, 5, 2, 3})) return false;
  a.splice_after(std::next(a.cbegin()), a, a.cbegin());
  if (!holds(a, {7, 8, 6, 1, 4, 5, 2, 3})) return false;
  // within one list
  a.splice_after(a.cbefore_begin(), a, std::next(a.cbegin(), 6));  // moves 3 to the front
  if (!holds(a, {3, 7, 8, 6, 1, 4, 5, 2})) return false;

  // open range (first, last)
  L d = mk<T>({10, 11, 12, 13, 14});
  auto first = d.cbegin();                 // 10
  auto last = std::next(d.cbegin(), 4);    // 14
  auto i11 = std::next(d.begin());
  const T* p12 = std::addressof(*std::next(d.begin(), 2));
  a.splice_after(a.cbefore_begin(), d, first, last);  // moves 11, 12, 13
  if (!holds(a, {11, 12, 13, 3, 7, 8, 6, 1, 4, 5, 2}) || !holds(d, {10, 14})) return false;
  if (i11 != a.begin() || std::addressof(*std::next(i11)) != p12) return false;
  a.splice_after(a.cbegin(), d, d.cbegin(), std::next(d.cbegin()));  // empty open range
  if (!holds(d, {10, 14}) || count_elems(a) != 11) return false;
  a.splice_after(a.cbegin(), std::move(d), d.cbefore_begin(), d.cend());  // all of d
  if (!holds(a, {11, 10, 14, 12, 13, 3, 7, 8, 6, 1, 4, 5, 2}) || !d.empty()) return false;
  // within one list: move (12, 7) i.e. 13, 3 to the front
  auto f = std::next(a.cbegin(), 3);
  auto l = std::next(a.cbegin(), 6);
  a.splice_after(a.cbefore_begin(), a, f, l);
  return holds(a, {13, 3, 11, 10, 14, 12, 7, 8, 6, 1, 4, 5, 2});
}

static_assert(test<int>());
static_assert(test<Elem>());

int main() {
  CHECK(test<int>());
  CHECK(test<Elem>());
  return 0;
}
