// [atomics.types.pointer]: atomic<T*> with difference_type ptrdiff_t; fetch_add / fetch_sub
// perform pointer arithmetic (in units of T) and return the old value; operator+=/-= return
// the new value; [atomics.types.memop]: ++/-- (pre: new value, post: old value).
#include <atomic>
#include <cstddef>
#include "check.hpp"

struct Big { char c[24]; };

int main() {
  Big arr[8];
  std::atomic<Big*> p(arr);
  CHECK(p.fetch_add(3) == arr);
  CHECK(p.load() == arr + 3);
  CHECK(p.fetch_sub(2, std::memory_order::acq_rel) == arr + 3);
  CHECK(p.load() == arr + 1);
  CHECK((p += 4) == arr + 5);
  CHECK((p -= 1) == arr + 4);
  CHECK(++p == arr + 5);
  CHECK(p++ == arr + 5);
  CHECK(p.load() == arr + 6);
  CHECK(--p == arr + 5);
  CHECK(p-- == arr + 5);
  CHECK(p.load() == arr + 4);
  CHECK(p.fetch_add(-4) == arr + 4);
  CHECK(p.load() == arr);

  std::atomic<const int*> c(nullptr);
  CHECK(c.load() == nullptr);
  const int ci[3] = {1, 2, 3};
  c = ci;
  CHECK(*++c == 2);
  CHECK(c.exchange(nullptr) == ci + 1);

  std::atomic<int*> d;  // value-initialized
  CHECK(d.load() == nullptr);
  int x = 0, y = 0;
  int* e = &x;
  d = &x;
  CHECK(d.compare_exchange_strong(e, &y));
  CHECK(d.load() == &y);
  CHECK(!d.compare_exchange_strong(e, &x));
  CHECK(e == &y);
  return 0;
}
