// [thread.lock.scoped]/1: with one mutex type, mutex_type names it; otherwise "there is no
// member mutex_type". /2: with zero mutexes no effects; with one m.lock(); otherwise lock(m...)
// (deadlock-avoiding). /3-5: the adopt_lock_t constructor does not lock. /6: the destructor
// unlocks each. Two threads taking the same pair of mutexes in opposite argument orders, many
// times, must not deadlock ([thread.lock.algorithm]/5: "The sequence of calls does not result
// in deadlock").
// FLAGS: -pthread
#include <mutex>
#include <thread>
#include <type_traits>
#include "check.hpp"

struct Spy {
  int held = 0;
  void lock() { ++held; }
  bool try_lock() { ++held; return true; }
  void unlock() { --held; }
};

template<class T> concept has_mutex_type = requires { typename T::mutex_type; };
static_assert(has_mutex_type<std::scoped_lock<std::mutex>>);
static_assert(std::is_same_v<std::scoped_lock<std::mutex>::mutex_type, std::mutex>);
static_assert(!has_mutex_type<std::scoped_lock<>>);
static_assert(!has_mutex_type<std::scoped_lock<std::mutex, std::mutex>>);
static_assert(!std::is_copy_constructible_v<std::scoped_lock<std::mutex>>);
static_assert(!std::is_convertible_v<std::mutex&, std::scoped_lock<std::mutex>>);

int main() {
  { std::scoped_lock<> none; }
  std::scoped_lock empty;  // CTAD with no arguments
  static_assert(std::is_same_v<decltype(empty), std::scoped_lock<>>);

  Spy a, b, c;
  {
    std::scoped_lock l(a);
    static_assert(std::is_same_v<decltype(l), std::scoped_lock<Spy>>);
    CHECK(a.held == 1);
  }
  CHECK(a.held == 0);
  {
    std::scoped_lock l(a, b, c);
    CHECK(a.held == 1 && b.held == 1 && c.held == 1);
  }
  CHECK(a.held == 0 && b.held == 0 && c.held == 0);
  a.lock();
  b.lock();
  {
    std::scoped_lock l(std::adopt_lock, a, b);
    CHECK(a.held == 1 && b.held == 1);
  }
  CHECK(a.held == 0 && b.held == 0);

  std::mutex m1, m2;
  long counter = 0;
  std::thread t1([&] {
    for (int i = 0; i < 5000; ++i) { std::scoped_lock l(m1, m2); ++counter; }
  });
  std::thread t2([&] {
    for (int i = 0; i < 5000; ++i) { std::scoped_lock l(m2, m1); ++counter; }
  });
  t1.join();
  t2.join();
  CHECK(counter == 10000);
  return 0;
}
