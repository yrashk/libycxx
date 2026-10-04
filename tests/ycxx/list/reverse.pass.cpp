// [list.ops]/31-32: reverse() is noexcept, reverses the order of the elements and "Does not
// affect the validity of iterators and references": each saved iterator still refers to the
// same element, now at the mirrored position, and is still an iterator into the list.
#include <list>
#include <iterator>
#include <memory>
#include "container_values.hpp"
#include "check.hpp"

static_assert(noexcept(std::declval<std::list<int>&>().reverse()));

template <class T>
constexpr bool test(int n) {
  std::list<T> l;
  for (int i = 0; i < n; ++i) l.push_back(val<T>(i % 80));
  typename std::list<T>::iterator its[100];
  int k = 0;
  for (auto it = l.begin(); it != l.end(); ++it) its[k++] = it;
  l.reverse();
  if (l.size() != static_cast<std::size_t>(n)) return false;
  k = n;
  for (auto it = l.begin(); it != l.end(); ++it) {
    --k;
    if (it != its[k] || !(*it == val<T>(k % 80))) return false;
  }
  if (n > 0 && (its[n - 1] != l.begin() || std::next(its[0]) != l.end())) return false;
  l.reverse();
  k = 0;
  for (auto it = l.begin(); it != l.end(); ++it, ++k)
    if (it != its[k]) return false;
  return true;
}

static_assert(test<int>(0) && test<int>(1) && test<int>(2) && test<int>(7) && test<int>(100));
static_assert(test<Elem>(10));

int main() {
  for (int n : {0, 1, 2, 3, 50, 100}) {
    CHECK(test<int>(n));
    CHECK(test<Elem>(n));
  }
  return 0;
}
