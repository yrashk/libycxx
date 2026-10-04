// [list.ops]/15-19: remove(value) / remove_if(pred) erase every element with *i == value /
// pred(*i) != false, return the number of elements erased (size_type), invalidate only the
// erased elements, apply the predicate exactly size() times and are stable. /20-25:
// unique() / unique(binary_pred) erase all but the first element of every consecutive group
// of equivalent elements, return the number erased, apply the predicate exactly size() - 1
// times (none when empty) and invalidate only the erased elements. remove(value) with value
// referring to an element of the list itself is a valid call (no precondition excludes it).
#include <list>
#include <iterator>
#include <memory>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr std::list<T> mk(std::initializer_list<int> idx) {
  std::list<T> l;
  for (int i : idx) l.push_back(val<T>(i));
  return l;
}

template <class T>
constexpr bool test() {
  using S = typename std::list<T>::size_type;
  static_assert(std::is_same_v<decltype(std::declval<std::list<T>&>().remove(val<T>(1))), S>);
  static_assert(std::is_same_v<decltype(std::declval<std::list<T>&>().unique()), S>);
  auto l = mk<T>({1, 2, 1, 3, 1, 1, 4});
  auto i2 = std::next(l.begin());
  auto i4 = std::prev(l.end());
  const T* p3 = std::addressof(*std::next(l.begin(), 3));
  S n = l.remove(val<T>(1));
  if (n != 4 || !holds(l, {2, 3, 4}) || l.size() != 3) return false;
  if (i2 != l.begin() || std::next(i2, 2) != i4 || std::addressof(*std::next(i2)) != p3) return false;
  if (l.remove(val<T>(9)) != 0 || l.size() != 3) return false;

  int calls = 0;
  l = mk<T>({5, 6, 7, 8, 9, 10});
  n = l.remove_if([&](const T& x) {
    ++calls;
    return x == val<T>(6) || x == val<T>(9) || x == val<T>(10);
  });
  if (n != 3 || calls != 6 || !holds(l, {5, 7, 8})) return false;

  // value aliases an element that gets erased
  l = mk<T>({3, 1, 3, 2, 3});
  n = l.remove(l.front());
  if (n != 3 || !holds(l, {1, 2})) return false;
  l = mk<T>({1, 3, 2, 3});
  n = l.remove(*std::next(l.begin()));
  if (n != 2 || !holds(l, {1, 2})) return false;

  l = mk<T>({1, 1, 2, 2, 2, 1, 3, 3, 1});
  auto first2 = std::next(l.begin(), 2);
  n = l.unique();
  if (n != 4 || !holds(l, {1, 2, 1, 3, 1}) || l.size() != 5) return false;
  if (first2 != std::next(l.begin())) return false;
  calls = 0;
  l = mk<T>({1, 2, 3, 4, 5, 6, 7});
  // "equivalent" when both are below 4 or both are at least 4 (an equivalence relation)
  n = l.unique([&](const T& a, const T& b) {
    ++calls;
    return (a < val<T>(4)) == (b < val<T>(4));
  });
  if (n != 5 || calls != 6 || !holds(l, {1, 4})) return false;
  std::list<T> e;
  calls = 0;
  if (e.unique([&](const T&, const T&) { return ++calls, true; }) != 0 || calls != 0) return false;
  if (e.remove(val<T>(1)) != 0) return false;
  return true;
}

static_assert(test<int>());
static_assert(test<Elem>());

int main() {
  CHECK(test<int>());
  CHECK(test<Elem>());
  return 0;
}
