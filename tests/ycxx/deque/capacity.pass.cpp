// [deque.capacity]/1-4: resize(sz) erases the last size() - sz elements if sz < size(),
// otherwise appends sz - size() default-inserted elements; resize(sz, c) appends copies of c.
// /5-6: shrink_to_fit is a non-binding request to reduce memory use "but does not change the
// size of the sequence" (and leaves the values unchanged). The deque stays fully usable
// afterwards. Default-inserted int elements are value-initialized ([container.alloc.reqmts]
// default-insertion uses allocator_traits::construct(m, p), i.e. T()).
#include <deque>
#include <cstddef>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr bool test() {
  std::deque<T> d;
  for (int i = 0; i < 10; ++i) d.push_front(val<T>(i));
  d.resize(4);
  if (d.size() != 4 || !(d[0] == val<T>(9)) || !(d[3] == val<T>(6))) return false;
  d.resize(8);
  if (d.size() != 8 || !(d[3] == val<T>(6)) || !(d[4] == T()) || !(d[7] == T())) return false;
  d.resize(8);
  if (d.size() != 8) return false;
  d.resize(10, val<T>(5));
  if (d.size() != 10 || !(d[7] == T()) || !(d[8] == val<T>(5)) || !(d[9] == val<T>(5))) return false;
  d.resize(2, val<T>(1));
  if (d.size() != 2 || !(d[1] == val<T>(8))) return false;
  d.resize(0);
  if (!d.empty()) return false;
  d.resize(500, val<T>(3));
  if (d.size() != 500 || !(d[499] == val<T>(3))) return false;

  // shrink_to_fit keeps size and values
  std::deque<T> s;
  for (int i = 0; i < 800; ++i) s.push_back(val<T>(i % 80));
  for (int i = 0; i < 700; ++i) {
    if (i % 2) s.pop_back();
    else s.pop_front();
  }
  std::deque<T> copy = s;
  s.shrink_to_fit();
  if (s.size() != 100 || !(s == copy)) return false;
  s.push_front(val<T>(1));
  s.push_back(val<T>(2));
  if (s.size() != 102 || !(s.front() == val<T>(1)) || !(s.back() == val<T>(2))) return false;
  std::deque<T> e;
  e.shrink_to_fit();
  if (!e.empty()) return false;
  e.push_back(val<T>(4));
  e.shrink_to_fit();
  return e.size() == 1 && e[0] == val<T>(4);
}

static_assert(test<int>());
static_assert(test<Elem>());

int main() {
  CHECK(test<int>());
  CHECK(test<Elem>());
  CHECK(test<double>());
  std::deque<int> d(5);
  for (int x : d) CHECK(x == 0);
  return 0;
}
