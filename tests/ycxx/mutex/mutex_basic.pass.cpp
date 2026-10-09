// [thread.mutex.class]: mutex has a constexpr noexcept default constructor and is neither
// copyable nor movable; standard-layout. [thread.mutex.requirements.mutex.general]: lock()
// blocks until ownership is obtained; try_lock() "attempts to obtain ownership ... without
// blocking. If ownership is not obtained, there is no effect and try_lock() immediately
// returns. An implementation may fail to obtain the lock even if it is not held by any other
// thread."; unlock() releases ownership.
// FLAGS: -pthread
#include <mutex>
#include <thread>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::mutex>);
static_assert(!std::is_copy_constructible_v<std::mutex>);
static_assert(!std::is_move_constructible_v<std::mutex>);
static_assert(!std::is_copy_assignable_v<std::mutex>);
static_assert(std::is_standard_layout_v<std::mutex>);
static_assert(std::is_same_v<decltype(std::declval<std::mutex&>().try_lock()), bool>);

constinit std::mutex global;  // constexpr mutex() noexcept

int main() {
  std::mutex m;
  m.lock();
  bool other = true;
  std::thread([&] { other = m.try_lock(); }).join();
  CHECK(!other);  // held by main: never obtained
  m.unlock();
  bool got = false;
  std::thread([&] { m.lock(); got = true; m.unlock(); }).join();
  CHECK(got);
  if (m.try_lock()) m.unlock();  // a spurious failure is permitted
  m.lock();
  m.unlock();
  global.lock();
  global.unlock();
  return 0;
}
