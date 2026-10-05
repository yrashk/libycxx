// [deque.capacity]/5-6: shrink_to_fit (Preconditions: T is Cpp17MoveInsertable) is a
// non-binding request that "does not change the size of the sequence"; "If the size is equal to
// the old capacity, or if an exception is thrown other than by the move constructor of a
// non-Cpp17CopyInsertable T, then there are no effects." For a copyable T whose copy and move
// constructors can throw, an exception escaping shrink_to_fit must therefore leave the deque
// exactly as it was (same size, same values, in order). Checked after the deque has acquired
// spare room at both ends (push at both ends, then erasure at both ends), with the throw armed
// at every possible element operation.
// REQUIRES: exceptions
#include <deque>
#include "check.hpp"

struct T {
  static inline int countdown = -1;  // throw when it reaches 0; -1: never
  int v;
  static void tick() {
    if (countdown > 0 && --countdown == 0) throw 42;
  }
  T(int x) : v(x) {}
  T(const T& o) : v(o.v) { tick(); }
  T(T&& o) noexcept(false) : v(o.v) { tick(); }
  T& operator=(const T& o) {
    tick();
    v = o.v;
    return *this;
  }
  T& operator=(T&& o) noexcept(false) {
    tick();
    v = o.v;
    return *this;
  }
};

static std::deque<T> make() {
  std::deque<T> d;
  for (int i = 0; i < 1500; ++i) d.push_back(T(i));
  for (int i = 0; i < 1500; ++i) d.push_front(T(-i - 1));
  d.erase(d.begin(), d.begin() + 1400);
  d.erase(d.end() - 1400, d.end());
  return d;  // 200 elements: -100 .. 99
}

static bool intact(const std::deque<T>& d) {
  if (d.size() != 200) return false;
  int expect = -100;
  for (const T& t : d)
    if (t.v != expect++) return false;
  return true;
}

int main() {
  int threw = 0;
  for (int k = 1; k < 260; ++k) {
    T::countdown = -1;
    std::deque<T> d = make();
    CHECK(intact(d));
    T::countdown = k;
    try {
      d.shrink_to_fit();
    } catch (int) {
      ++threw;
    }
    T::countdown = -1;
    CHECK(intact(d));  // either the request succeeded or there were no effects
    d.push_back(T(100));
    d.push_front(T(-101));
    CHECK(d.size() == 202 && d.front().v == -101 && d.back().v == 100);
  }
  (void)threw;  // the request may be a no-op that never touches the elements
  return 0;
}
