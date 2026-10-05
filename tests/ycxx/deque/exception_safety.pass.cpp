// [deque.modifiers]/3: "If an exception is thrown other than by the copy constructor, move
// constructor, assignment operator, or move assignment operator of T, there are no effects.
// If an exception is thrown while inserting a single element at either end, there are no
// effects." Checked for push_back / push_front / emplace_back / emplace_front and
// insert(begin()/end(), x) with a throwing copy or converting constructor, and for a large
// insertion in the middle whose allocation throws (an exception not thrown by T).
// REQUIRES: exceptions
#include <deque>
#include <new>
#include <cstddef>
#include "test_allocators.hpp"
#include "check.hpp"

struct Bomb {};
inline int copies_left = -1;  // copy constructor throws when this reaches 0

struct T {
  int v;
  T(int x) : v(x) {}
  T(int x, bool boom) : v(x) {
    if (boom) throw Bomb{};
  }
  T(const T& o) : v(o.v) {
    if (copies_left == 0) throw Bomb{};
    if (copies_left > 0) --copies_left;
  }
  T(T&&) noexcept = default;
  T& operator=(const T&) = default;
  T& operator=(T&&) noexcept = default;
};

template <class D>
bool unchanged(const D& d, int n, int first) {
  if (d.size() != static_cast<std::size_t>(n)) return false;
  for (int i = 0; i < n; ++i)
    if (d[static_cast<std::size_t>(i)].v != first + i) return false;
  return true;
}

template <class D>
bool ends(int n) {
  D d;
  for (int i = 0; i < n; ++i) d.emplace_back(i);
  const T t(99);
  int thrown = 0;
  copies_left = 0;
  try { d.push_back(t); } catch (Bomb) { ++thrown; }
  if (!unchanged(d, n, 0)) return false;
  try { d.push_front(t); } catch (Bomb) { ++thrown; }
  if (!unchanged(d, n, 0)) return false;
  try { d.insert(d.end(), t); } catch (Bomb) { ++thrown; }
  if (!unchanged(d, n, 0)) return false;
  try { d.insert(d.begin(), t); } catch (Bomb) { ++thrown; }
  if (!unchanged(d, n, 0)) return false;
  copies_left = -1;
  try { d.emplace_back(5, true); } catch (Bomb) { ++thrown; }
  if (!unchanged(d, n, 0)) return false;
  try { d.emplace_front(5, true); } catch (Bomb) { ++thrown; }
  if (!unchanged(d, n, 0)) return false;
  // still usable
  d.push_back(t);
  d.push_front(t);
  return thrown == 6 && d.size() == static_cast<std::size_t>(n + 2) && d.front().v == 99 && d.back().v == 99;
}

bool allocation_failure_in_middle() {
  using D = std::deque<T, CountingAlloc<T>>;
  D d;
  for (int i = 0; i < 40; ++i) d.emplace_back(i);
  std::deque<T> big;
  for (int i = 0; i < 5000; ++i) big.emplace_back(1000 + i);
  alloc_counters.fail_after = 0;
  bool threw = false;
  try {
    d.insert(d.begin() + 20, big.begin(), big.end());
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  alloc_counters.fail_after = -1;
  if (!threw) return false;  // 5000 new elements cannot fit without allocating
  return unchanged(d, 40, 0);
}

int main() {
  for (int n : {0, 1, 7, 64, 511, 512, 513, 1000, 4096}) {
    CHECK(ends<std::deque<T>>(n));
    CHECK(ends<std::deque<T, CountingAlloc<T>>>(n));
  }
  CHECK(allocation_failure_in_middle());
  CHECK(alloc_counters.outstanding == 0);
  return 0;
}
