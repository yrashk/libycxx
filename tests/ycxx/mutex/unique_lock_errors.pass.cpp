// [thread.lock.unique.locking]: lock / try_lock / try_lock_for / try_lock_until throw
// system_error with "operation_not_permitted — if pm is nullptr" and
// "resource_deadlock_would_occur — if on entry owns is true"; unlock(): "operation_not_permitted
// — if on entry owns is false". [thread.lock.shared.locking]: the same for shared_lock.
// REQUIRES: exceptions
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <system_error>
#include "check.hpp"

template<class F>
static bool throws(F f, std::errc ec) {
  try {
    f();
  } catch (const std::system_error& e) {
    return e.code() == ec;
  }
  return false;
}

int main() {
  using std::errc;
  using ms = std::chrono::milliseconds;
  std::unique_lock<std::timed_mutex> none;
  CHECK(throws([&] { none.lock(); }, errc::operation_not_permitted));
  CHECK(throws([&] { none.try_lock(); }, errc::operation_not_permitted));
  CHECK(throws([&] { none.try_lock_for(ms(1)); }, errc::operation_not_permitted));
  CHECK(throws([&] { none.try_lock_until(std::chrono::steady_clock::now()); }, errc::operation_not_permitted));
  CHECK(throws([&] { none.unlock(); }, errc::operation_not_permitted));

  std::timed_mutex m;
  std::unique_lock<std::timed_mutex> l(m);
  CHECK(throws([&] { l.lock(); }, errc::resource_deadlock_would_occur));
  CHECK(throws([&] { l.try_lock(); }, errc::resource_deadlock_would_occur));
  CHECK(throws([&] { l.try_lock_for(ms(1)); }, errc::resource_deadlock_would_occur));
  CHECK(throws([&] { l.try_lock_until(std::chrono::steady_clock::now()); }, errc::resource_deadlock_would_occur));
  l.unlock();
  CHECK(throws([&] { l.unlock(); }, errc::operation_not_permitted));

  std::shared_lock<std::shared_timed_mutex> snone;
  CHECK(throws([&] { snone.lock(); }, errc::operation_not_permitted));
  CHECK(throws([&] { snone.try_lock(); }, errc::operation_not_permitted));
  CHECK(throws([&] { snone.try_lock_for(ms(1)); }, errc::operation_not_permitted));
  CHECK(throws([&] { snone.unlock(); }, errc::operation_not_permitted));
  std::shared_timed_mutex sm;
  std::shared_lock<std::shared_timed_mutex> sl(sm);
  CHECK(throws([&] { sl.lock(); }, errc::resource_deadlock_would_occur));
  CHECK(throws([&] { sl.try_lock(); }, errc::resource_deadlock_would_occur));
  CHECK(throws([&] { sl.try_lock_until(std::chrono::steady_clock::now()); }, errc::resource_deadlock_would_occur));
  sl.unlock();
  CHECK(throws([&] { sl.unlock(); }, errc::operation_not_permitted));
  return 0;
}
