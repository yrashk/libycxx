// [thread.lock.shared]: shared_lock(m) calls m.lock_shared(); defer_lock / try_to_lock /
// adopt_lock / timed constructors; the destructor calls unlock_shared() if owns; move leaves
// the source empty; swap; release() returns pm without unlocking; owns_lock / operator bool /
// mutex(). CTAD.
// FLAGS: -pthread
#include <thread>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Spy {
  int shared = 0, tries = 0;
  bool allow = true;
  void lock_shared() { ++shared; }
  bool try_lock_shared() { ++tries; if (allow) ++shared; return allow; }
  template<class R, class P> bool try_lock_shared_for(const std::chrono::duration<R, P>&) { return try_lock_shared(); }
  template<class C, class D> bool try_lock_shared_until(const std::chrono::time_point<C, D>&) { return try_lock_shared(); }
  void unlock_shared() { --shared; }
};

using SL = std::shared_lock<Spy>;
static_assert(std::is_same_v<SL::mutex_type, Spy>);
static_assert(std::is_nothrow_default_constructible_v<SL>);
static_assert(std::is_nothrow_move_constructible_v<SL>);
static_assert(!std::is_copy_constructible_v<SL>);
static_assert(!std::is_convertible_v<SL, bool>);

int main() {
  SL e;
  CHECK(!e && !e.owns_lock() && e.mutex() == nullptr);
  Spy s;
  {
    SL l(s);
    CHECK(l.owns_lock() && s.shared == 1 && l.mutex() == &s);
    SL l2(s, std::defer_lock);
    CHECK(!l2.owns_lock() && s.shared == 1);
    l2.lock();
    CHECK(s.shared == 2);
    l2.unlock();
    CHECK(s.shared == 1 && !l2);
  }
  CHECK(s.shared == 0);
  s.allow = false;
  {
    SL l(s, std::try_to_lock);
    CHECK(!l.owns_lock());
    SL l2(s, std::chrono::milliseconds(1));
    CHECK(!l2.owns_lock());
    SL l3(s, std::chrono::steady_clock::now());
    CHECK(!l3.owns_lock());
    CHECK(s.tries == 3);
  }
  CHECK(s.shared == 0);
  s.allow = true;
  s.lock_shared();
  {
    SL l(s, std::adopt_lock);
    CHECK(l.owns_lock() && s.shared == 1);
    SL m(std::move(l));
    CHECK(!l.owns_lock() && l.mutex() == nullptr && m.owns_lock());
    SL n;
    n = std::move(m);
    CHECK(n.owns_lock() && !m.owns_lock() && s.shared == 1);
    SL o;
    o.swap(n);
    CHECK(o.owns_lock() && !n.owns_lock());
    swap(o, n);
    CHECK(n.owns_lock());
    Spy* p = n.release();
    CHECK(p == &s && s.shared == 1 && !n.owns_lock());
  }
  CHECK(s.shared == 1);
  s.unlock_shared();

  std::shared_mutex sm;
  {
    std::shared_lock a(sm);
    static_assert(std::is_same_v<decltype(a), std::shared_lock<std::shared_mutex>>);
    std::thread([&] {
      std::shared_lock<std::shared_mutex> b(sm, std::try_to_lock);
      if (!b.owns_lock()) b.lock();  // try may fail spuriously
      CHECK(b.owns_lock() && b.mutex() == &sm);
    }).join();
    CHECK(a.owns_lock());
  }
  sm.lock();
  sm.unlock();
  return 0;
}
