// [list.modifiers]/2: insert, emplace, push/emplace at either end, prepend_range and
// append_range "Does not affect the validity of iterators and references." /3: erase,
// pop_front, pop_back and clear "Invalidates only the iterators and references to the erased
// elements." So iterators saved before any of these keep referring to their (surviving)
// elements and can still be used to traverse the list, including end().
#include <list>
#include <iterator>
#include <memory>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr bool test() {
  std::list<T> l;
  for (int i = 0; i < 5; ++i) l.push_back(val<T>(i));
  using It = typename std::list<T>::iterator;
  It its[5];
  const T* addr[5];
  {
    int k = 0;
    for (auto it = l.begin(); it != l.end(); ++it, ++k) {
      its[k] = it;
      addr[k] = std::addressof(*it);
    }
  }
  const It end = l.end();
  T arr[3] = {val<T>(70), val<T>(71), val<T>(72)};
  l.insert(its[2], val<T>(50));
  l.insert(its[0], 3, val<T>(51));
  l.insert(l.end(), arr, arr + 3);
  l.insert_range(its[4], arr);
  l.emplace(its[1], val<T>(52));
  l.emplace_front(val<T>(53));
  l.emplace_back(val<T>(54));
  l.push_front(val<T>(55));
  l.push_back(val<T>(56));
  l.prepend_range(arr);
  l.append_range(arr);
  l.insert(its[3], {val<T>(57), val<T>(58)});
  for (int k = 0; k < 5; ++k)
    if (std::addressof(*its[k]) != addr[k] || !(*its[k] == val<T>(k))) return false;
  if (end != l.end()) return false;
  // walk from a saved iterator to the saved end
  int n = 0;
  for (It it = its[0]; it != end; ++it) ++n;
  if (n != 20) return false;
  // erase everything except the saved elements
  for (auto it = l.begin(); it != l.end();) {
    bool saved = false;
    for (int k = 0; k < 5; ++k) saved = saved || it == its[k];
    if (saved) ++it;
    else it = l.erase(it);
  }
  if (l.size() != 5 || !holds(l, {0, 1, 2, 3, 4})) return false;
  for (int k = 0; k < 5; ++k)
    if (std::addressof(*its[k]) != addr[k]) return false;
  l.pop_front();
  l.pop_back();
  l.erase(its[2]);
  if (std::next(its[1]) != its[3] || std::next(its[3]) != end || its[1] != l.begin()) return false;
  l.erase(its[1], its[3]);
  if (l.begin() != its[3] || l.size() != 1) return false;
  l.clear();
  return l.end() == end && l.empty();
}

static_assert(test<int>());
static_assert(test<Elem>());

int main() {
  CHECK(test<int>());
  CHECK(test<Elem>());
  return 0;
}
