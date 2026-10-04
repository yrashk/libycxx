// [deque.modifiers]/2: "Inserting a single element at either the beginning or end of a deque
// always takes constant time and causes a single call to a constructor of T." /6: for erase,
// pop_front and pop_back, "The number of calls to the destructor of T is the same as the
// number of elements erased, but the number of calls to the assignment operator of T is no
// more than the lesser of the number of elements before the erased elements and the number
// of elements after the erased elements."
#include <deque>
#include <cstddef>
#include "check.hpp"

struct Counts {
  int ctors = 0, dtors = 0, assigns = 0;
};
inline Counts counts;

struct T {
  int v;
  T() : v(0) { ++counts.ctors; }
  T(int x) : v(x) { ++counts.ctors; }
  T(const T& o) : v(o.v) { ++counts.ctors; }
  T(T&& o) noexcept : v(o.v) { ++counts.ctors; }
  T& operator=(const T& o) { v = o.v; ++counts.assigns; return *this; }
  T& operator=(T&& o) noexcept { v = o.v; ++counts.assigns; return *this; }
  ~T() { ++counts.dtors; }
};

std::deque<T> make(int n) {
  std::deque<T> d;
  for (int i = 0; i < n; ++i) d.emplace_back(i);
  return d;
}

bool single_end_insertions() {
  std::deque<T> d;
  const T t(7);
  for (int i = 0; i < 3000; ++i) {
    counts = {};
    switch (i % 6) {
      case 0: d.push_back(t); break;
      case 1: d.push_front(t); break;
      case 2: d.push_back(T(i)); break;  // the temporary is one more constructor call
      case 3: d.emplace_front(i); break;
      case 4: d.emplace_back(i); break;
      case 5: d.insert(d.begin(), t); break;
    }
    int expected = (i % 6 == 2) ? 2 : 1;
    if (counts.ctors != expected || counts.assigns != 0) return false;
    if (counts.dtors != expected - 1) return false;
  }
  return d.size() == 3000;
}

bool erasures(int n) {
  for (int first = 0; first < n; first += (n > 40 ? 7 : 1)) {
    for (int len = 1; first + len <= n; len += (n > 40 ? 5 : 1)) {
      std::deque<T> d = make(n);
      counts = {};
      d.erase(d.begin() + first, d.begin() + first + len);
      int before = first, after = n - first - len;
      if (counts.dtors != len) return false;
      if (counts.assigns > (before < after ? before : after)) return false;
      if (d.size() != static_cast<std::size_t>(n - len)) return false;
      for (int k = 0; k < n - len; ++k)
        if (d[static_cast<std::size_t>(k)].v != (k < first ? k : k + len)) return false;
    }
  }
  if (n >= 2) {
    std::deque<T> d = make(n);
    counts = {};
    d.pop_front();
    d.pop_back();
    if (counts.dtors != 2 || counts.assigns != 0) return false;
  }
  return true;
}

int main() {
  CHECK(single_end_insertions());
  CHECK(erasures(1));
  CHECK(erasures(12));
  CHECK(erasures(300));
  return 0;
}
