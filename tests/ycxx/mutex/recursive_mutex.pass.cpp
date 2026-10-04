// [thread.mutex.recursive]: "A thread that owns a recursive_mutex object may acquire additional
// levels of ownership by calling lock() or try_lock() on that object. ... A thread shall call
// unlock() once for each level of ownership acquired by calls to lock() and try_lock(). Only
// when all levels of ownership have been released may ownership be acquired by another thread."
// [thread.timedmutex.recursive] likewise for recursive_timed_mutex (also try_lock_for).
// FLAGS: -pthread
#include <mutex>
#include <thread>
#include <chrono>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_copy_constructible_v<std::recursive_mutex>);
static_assert(std::is_standard_layout_v<std::recursive_mutex>);

template<class M>
static bool other_thread_try(M& m) {
  bool r = true;
  std::thread([&] { r = m.try_lock(); if (r) m.unlock(); }).join();
  return r;
}

template<class M>
static void run() {
  M m;
  m.lock();
  m.lock();
  bool t = false;
  for (int i = 0; i < 1000 && !t; ++i) t = m.try_lock();
  CHECK(t);
  CHECK(!other_thread_try(m));
  m.unlock();
  CHECK(!other_thread_try(m));
  m.unlock();
  CHECK(!other_thread_try(m));
  m.unlock();
  bool ok = false;
  for (int i = 0; i < 1000 && !ok; ++i) ok = other_thread_try(m);
  CHECK(ok);
}

int main() {
  run<std::recursive_mutex>();
  run<std::recursive_timed_mutex>();
  std::recursive_timed_mutex rt;
  CHECK(rt.try_lock_for(std::chrono::milliseconds(1)));
  CHECK(rt.try_lock_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(1)));
  rt.unlock();
  rt.unlock();
  return 0;
}
