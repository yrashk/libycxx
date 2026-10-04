// emplace(position, args) and insert(position, x) with position == begin() or end() are
// insertions at an end of the deque. [deque.modifiers]/1: "An insertion at either end of the
// deque invalidates all the iterators to the deque, but has no effect on the validity of
// references to elements of the deque." /2: "Inserting a single element at either the
// beginning or end of a deque always takes constant time and causes a single call to a
// constructor of T." (so no element is moved, copied or assigned).
// [sequence.reqmts]/23: emplace returns an iterator that points to the new element; /27 the same
// for insert(p, t). insert_range(begin()/end(), rg) is a (multi-element) insertion at an end, so
// references survive it too.
#include <deque>
#include <memory>
#include "check.hpp"

struct T {
  static inline int ctors = 0, assigns = 0;
  int a, b;
  T(int x, int y) : a(x), b(y) { ++ctors; }
  T(const T& o) : a(o.a), b(o.b) { ++ctors; }
  T(T&& o) noexcept : a(o.a), b(o.b) { ++ctors; }
  T& operator=(const T& o) {
    ++assigns;
    a = o.a;
    b = o.b;
    return *this;
  }
  T& operator=(T&& o) noexcept {
    ++assigns;
    a = o.a;
    b = o.b;
    return *this;
  }
};

int main() {
  std::deque<T> d;
  for (int i = 0; i < 10; ++i) d.emplace_back(i, -i);
  const T* p[10];
  for (int i = 0; i < 10; ++i) p[i] = std::addressof(d[static_cast<std::size_t>(i)]);
  int front_count = 0;
  for (int round = 0; round < 2000; ++round) {
    T::ctors = T::assigns = 0;
    std::deque<T>::iterator it;
    switch (round % 4) {
      case 0:
        it = d.emplace(d.cbegin(), round, 1);
        ++front_count;
        CHECK(it == d.begin());
        break;
      case 1:
        it = d.emplace(d.cend(), round, 2);
        CHECK(it == d.end() - 1);
        break;
      case 2: {
        const T t(round, 3);
        T::ctors = 0;
        it = d.insert(d.cbegin(), t);
        ++front_count;
        CHECK(it == d.begin());
        break;
      }
      case 3:
        it = d.insert(d.cend(), T(round, 4));
        T::ctors -= 1;  // the temporary argument
        CHECK(it == d.end() - 1);
        break;
    }
    CHECK(T::ctors == 1);
    CHECK(T::assigns == 0);
    CHECK(it->a == round);
    for (int i = 0; i < 10; ++i) CHECK(p[i]->a == i && p[i]->b == -i);
    CHECK(std::addressof(d[static_cast<std::size_t>(front_count)]) == p[0]);
  }
  // multi-element insertions at the ends keep references too
  T arr[3] = {T(7, 7), T(8, 8), T(9, 9)};
  for (int round = 0; round < 200; ++round) {
    d.insert_range(round % 2 ? d.cend() : d.cbegin(), arr);
    d.insert(round % 2 ? d.cbegin() : d.cend(), 5, arr[0]);
    for (int i = 0; i < 10; ++i) CHECK(p[i]->a == i && p[i]->b == -i);
  }
  return 0;
}
