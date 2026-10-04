// [thread.lock.unique.cons]: unique_lock() has pm == nullptr, owns == false; (m) locks;
// (m, defer_lock) does not; (m, try_to_lock) owns == result of try_lock; (m, adopt_lock)
// owns; timed constructors call try_lock_for / try_lock_until; move construction leaves the
// source empty; move assignment is unique_lock(std::move(u)).swap(*this); the destructor
// unlocks if owns. [thread.lock.unique.mod]: swap, release() returns pm without unlocking.
// [thread.lock.unique.obs]: owns_lock, explicit operator bool, mutex().
#include <mutex>
#include <chrono>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Spy {
  int locks = 0, unlocks = 0, tries = 0;
  bool allow = true;
  void lock() { ++locks; }
  bool try_lock() { ++tries; if (allow) ++locks; return allow; }
  template<class R, class P> bool try_lock_for(const std::chrono::duration<R, P>&) { return try_lock(); }
  template<class C, class D> bool try_lock_until(const std::chrono::time_point<C, D>&) { return try_lock(); }
  void unlock() { ++unlocks; }
  int held() const { return locks - unlocks; }
};

using UL = std::unique_lock<Spy>;
static_assert(std::is_same_v<UL::mutex_type, Spy>);
static_assert(std::is_nothrow_default_constructible_v<UL>);
static_assert(std::is_nothrow_move_constructible_v<UL>);
static_assert(std::is_nothrow_move_assignable_v<UL>);
static_assert(!std::is_copy_constructible_v<UL>);
static_assert(!std::is_convertible_v<UL, bool>);
static_assert(std::is_constructible_v<bool, UL>);
static_assert(std::is_nothrow_constructible_v<UL, Spy&, std::defer_lock_t>);

int main() {
  UL e;
  CHECK(!e.owns_lock() && !e && e.mutex() == nullptr);

  Spy s;
  {
    UL l(s);
    CHECK(l.owns_lock() && bool(l) && l.mutex() == &s && s.held() == 1);
  }
  CHECK(s.held() == 0);
  {
    UL l(s, std::defer_lock);
    CHECK(!l.owns_lock() && l.mutex() == &s && s.held() == 0);
    l.lock();
    CHECK(l.owns_lock() && s.held() == 1);
    l.unlock();
    CHECK(!l.owns_lock() && s.held() == 0);
    CHECK(l.try_lock() && l.owns_lock());
  }
  CHECK(s.held() == 0);
  s.allow = false;
  const int unlocks_before = s.unlocks;
  {
    UL l(s, std::try_to_lock);
    CHECK(!l.owns_lock() && s.tries == 2);
    UL t1(s, std::chrono::milliseconds(1));
    CHECK(!t1.owns_lock());
    UL t2(s, std::chrono::steady_clock::now());
    CHECK(!t2.owns_lock());
  }
  CHECK(s.unlocks == unlocks_before);  // nothing unlocked: none of them owned
  s.allow = true;
  {
    UL l(s, std::try_to_lock);
    CHECK(l.owns_lock());
    UL t1(s, std::chrono::milliseconds(1));
    CHECK(t1.owns_lock());
  }
  CHECK(s.held() == 0);

  s.lock();
  {
    UL l(s, std::adopt_lock);
    CHECK(l.owns_lock());
  }
  CHECK(s.held() == 0);

  // move
  UL a(s);
  UL b(std::move(a));
  CHECK(!a.owns_lock() && a.mutex() == nullptr);
  CHECK(b.owns_lock() && b.mutex() == &s);
  Spy s2;
  UL c(s2);
  c = std::move(b);  // releases s2
  CHECK(s2.held() == 0);
  CHECK(c.mutex() == &s && c.owns_lock() && !b.owns_lock() && b.mutex() == nullptr);

  // swap
  UL d(s2, std::defer_lock);
  c.swap(d);
  CHECK(c.mutex() == &s2 && !c.owns_lock() && d.mutex() == &s && d.owns_lock());
  swap(c, d);
  CHECK(c.mutex() == &s && c.owns_lock());

  // release: no unlock
  int before = s.unlocks;
  Spy* p = c.release();
  CHECK(p == &s && c.mutex() == nullptr && !c.owns_lock());
  CHECK(s.unlocks == before && s.held() == 1);
  s.unlock();

  // CTAD with a real mutex
  std::mutex m;
  std::unique_lock ml(m);
  static_assert(std::is_same_v<decltype(ml), std::unique_lock<std::mutex>>);
  CHECK(ml.owns_lock());
  return 0;
}
