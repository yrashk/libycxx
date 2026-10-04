// [deque.modifiers]/1: "An insertion at either end of the deque invalidates all the iterators
// to the deque, but has no effect on the validity of references to elements of the deque."
// /4: "An erase operation that erases the last element of a deque invalidates only the
// past-the-end iterator and all iterators and references to the erased elements. An erase
// operation that erases the first element of a deque but not the last element invalidates
// only iterators and references to the erased elements." pop_front and pop_back are erase
// operations (Note 1). So references (and pointers) survive push/emplace/prepend/append at
// the ends, and iterators to surviving elements stay usable after erasing at either end.
#include <deque>
#include <memory>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr bool refs_survive_end_insertion() {
  std::deque<T> d;
  for (int i = 0; i < 5; ++i) d.push_back(val<T>(i));
  T* p[5];
  for (int i = 0; i < 5; ++i) p[i] = std::addressof(d[static_cast<std::size_t>(i)]);
  T arr[3] = {val<T>(70), val<T>(71), val<T>(72)};
  for (int round = 0; round < 300; ++round) {
    switch (round % 6) {
      case 0: d.push_back(val<T>(round % 60)); break;
      case 1: d.push_front(val<T>(round % 60)); break;
      case 2: d.emplace_back(val<T>(round % 60)); break;
      case 3: d.emplace_front(val<T>(round % 60)); break;
      case 4: d.append_range(arr); break;
      case 5: d.prepend_range(arr); break;
    }
    for (int i = 0; i < 5; ++i)
      if (!(*p[i] == val<T>(i))) return false;
  }
  // insert(begin()/end(), ...) are insertions at the ends as well
  d.insert(d.begin(), val<T>(1));
  d.insert(d.end(), 3, val<T>(2));
  d.insert(d.cend(), arr, arr + 3);
  d.insert(d.cbegin(), {val<T>(5), val<T>(6)});
  for (int i = 0; i < 5; ++i)
    if (!(*p[i] == val<T>(i))) return false;
  // the referenced objects are still the elements of the deque
  int found = 0;
  for (auto& x : d)
    for (int i = 0; i < 5; ++i)
      if (std::addressof(x) == p[i]) ++found;
  return found == 5;
}

template <class T>
constexpr bool iterators_survive_end_erasure() {
  std::deque<T> d;
  for (int i = 0; i < 60; ++i) d.push_back(val<T>(i));
  auto it10 = d.begin() + 10;
  auto it30 = d.begin() + 30;
  T* p30 = std::addressof(*it30);
  for (int i = 0; i < 10; ++i) d.pop_front();
  if (it10 != d.begin() || !(*it10 == val<T>(10))) return false;
  for (int i = 0; i < 20; ++i) d.pop_back();
  // it10 and it30 still refer to their elements and are still iterators into d
  if (d.end() - it10 != 30 || it30 - it10 != 20 || std::addressof(*it30) != p30) return false;
  if (!(*it30 == val<T>(30)) || !(it10[5] == val<T>(15))) return false;
  d.erase(d.begin());  // the first element, not the last
  d.erase(d.end() - 1);  // the last element
  d.erase(d.begin(), d.begin() + 2);
  d.erase(d.end() - 3, d.end());
  if (!(*it30 == val<T>(30)) || std::addressof(*it30) != p30) return false;
  if (d.begin() + 17 != it30 || d.front() != val<T>(13) || d.back() != val<T>(35)) return false;
  int n = 0;
  for (auto it = d.begin(); it != it30; ++it) ++n;
  return n == 17;
}

static_assert(refs_survive_end_insertion<int>());
static_assert(refs_survive_end_insertion<Elem>());
static_assert(iterators_survive_end_erasure<int>());
static_assert(iterators_survive_end_erasure<Elem>());

int main() {
  CHECK(refs_survive_end_insertion<int>());
  CHECK(refs_survive_end_insertion<Elem>());
  CHECK(refs_survive_end_insertion<long double>());
  CHECK(iterators_survive_end_erasure<int>());
  CHECK(iterators_survive_end_erasure<Elem>());
  return 0;
}
