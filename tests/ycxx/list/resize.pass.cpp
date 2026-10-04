// [list.capacity]: resize(sz) appends sz - size() default-inserted elements if size() < sz,
// otherwise erases the elements from position sz onwards; resize(sz, c) is
// "insert(end(), sz-size(), c)" when growing, "erase(begin() + sz, end())" when shrinking,
// and does nothing when sz == size(). Elements before position sz are untouched (same
// addresses, so iterators to them stay valid).
#include <list>
#include <iterator>
#include <memory>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr bool test() {
  std::list<T> l;
  for (int i = 0; i < 6; ++i) l.push_back(val<T>(i));
  const T* p1 = std::addressof(*std::next(l.begin()));
  l.resize(3);
  if (!holds(l, {0, 1, 2}) || l.size() != 3 || std::addressof(*std::next(l.begin())) != p1) return false;
  l.resize(5);
  if (l.size() != 5 || !(*std::next(l.begin(), 3) == T()) || !(l.back() == T())) return false;
  if (std::addressof(*std::next(l.begin())) != p1) return false;
  l.resize(5, val<T>(9));
  if (l.size() != 5 || !(l.back() == T())) return false;
  l.resize(7, val<T>(9));
  if (l.size() != 7 || !(l.back() == val<T>(9)) || !(*std::next(l.begin(), 5) == val<T>(9))) return false;
  l.resize(1, val<T>(9));
  if (!holds(l, {0})) return false;
  l.resize(0);
  if (!l.empty()) return false;
  l.resize(3, l.empty() ? val<T>(4) : val<T>(5));
  return holds(l, {4, 4, 4});
}

static_assert(test<int>());
static_assert(test<Elem>());

int main() {
  CHECK(test<int>());
  CHECK(test<Elem>());
  std::list<int> z(4);
  for (int x : z) CHECK(x == 0);
  return 0;
}
