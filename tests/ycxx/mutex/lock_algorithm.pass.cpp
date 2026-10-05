// [thread.lock.algorithm]/2-3: try_lock(l1, l2, ...) "Calls try_lock() for each argument in
// order beginning with the first until all arguments have been processed or a call to
// try_lock() fails, either by returning false or by throwing an exception. If a call to
// try_lock() fails, unlock() is called for all prior arguments with no further calls to
// try_lock()." Returns -1 or the zero-based index of the failing argument. /5: lock(...)
// locks all without deadlock; "If a call to lock() or try_lock() throws an exception, unlock()
// is called for any argument that had been locked by a call to lock() or try_lock()."
// FLAGS: -pthread
// REQUIRES: exceptions
#include <mutex>
#include <thread>
#include "check.hpp"

struct Spy {
  int held = 0, try_calls = 0;
  bool fail = false, throw_on_try = false, throw_on_lock = false;
  void lock() { if (throw_on_lock) throw 1; ++held; }
  bool try_lock() {
    ++try_calls;
    if (throw_on_try) throw 2;
    if (fail) return false;
    ++held;
    return true;
  }
  void unlock() { --held; }
};

int main() {
  Spy a, b, c;
  CHECK(std::try_lock(a, b, c) == -1);
  CHECK(a.held == 1 && b.held == 1 && c.held == 1);
  a.unlock(); b.unlock(); c.unlock();

  b.fail = true;
  int tries_c = c.try_calls;
  CHECK(std::try_lock(a, b, c) == 1);
  CHECK(a.held == 0 && b.held == 0 && c.held == 0);
  CHECK(c.try_calls == tries_c);  // no further calls
  b.fail = false;

  c.fail = true;
  CHECK(std::try_lock(a, b, c) == 2);
  CHECK(a.held == 0 && b.held == 0);
  c.fail = false;

  b.throw_on_try = true;
  bool caught = false;
  try {
    std::try_lock(a, b);
  } catch (int v) {
    caught = v == 2;
  }
  CHECK(caught && a.held == 0);
  b.throw_on_try = false;

  std::lock(a, b, c);
  CHECK(a.held == 1 && b.held == 1 && c.held == 1);
  a.unlock(); b.unlock(); c.unlock();

  // exception from lock() or try_lock(): everything locked so far is unlocked
  c.throw_on_try = true;
  c.throw_on_lock = true;
  caught = false;
  try {
    std::lock(a, b, c);
  } catch (int) {
    caught = true;
  }
  CHECK(caught);
  CHECK(a.held == 0 && b.held == 0 && c.held == 0);
  c.throw_on_try = c.throw_on_lock = false;

  // real mutexes, unique_locks, opposite orders, no deadlock
  std::mutex m1, m2;
  std::timed_mutex m3;
  long n = 0;
  std::thread t([&] {
    for (int i = 0; i < 3000; ++i) {
      std::unique_lock<std::mutex> l1(m1, std::defer_lock), l2(m2, std::defer_lock);
      std::lock(l1, m3, l2);
      ++n;
      m3.unlock();
    }
  });
  for (int i = 0; i < 3000; ++i) {
    std::lock(m2, m3, m1);
    ++n;
    m1.unlock(); m2.unlock(); m3.unlock();
  }
  t.join();
  CHECK(n == 6000);
  std::mutex x, y;
  x.lock();
  int r = 0;
  std::thread([&] { r = std::try_lock(y, x); }).join();
  CHECK(r == 1);  // x held elsewhere
  x.unlock();
  return 0;
}
