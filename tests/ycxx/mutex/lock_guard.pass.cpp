// [thread.lock.guard]: lock_guard(m) calls m.lock(); lock_guard(m, adopt_lock) does not; the
// destructor calls pm.unlock(); mutex_type; not copyable; class template argument deduction.
// [thread.lock]: defer_lock, try_to_lock, adopt_lock are inline constexpr objects of the
// tag types, which have explicit default constructors.
#include <mutex>
#include <type_traits>
#include "check.hpp"

struct Spy {
  int locks = 0, unlocks = 0;
  void lock() { ++locks; }
  void unlock() { ++unlocks; }
};

static_assert(std::is_same_v<std::lock_guard<Spy>::mutex_type, Spy>);
static_assert(!std::is_copy_constructible_v<std::lock_guard<Spy>>);
static_assert(!std::is_copy_assignable_v<std::lock_guard<Spy>>);
static_assert(!std::is_convertible_v<Spy&, std::lock_guard<Spy>>);  // explicit
static_assert(std::is_same_v<decltype(std::defer_lock), const std::defer_lock_t>);
static_assert(std::is_same_v<decltype(std::try_to_lock), const std::try_to_lock_t>);
static_assert(std::is_same_v<decltype(std::adopt_lock), const std::adopt_lock_t>);
static_assert(std::is_default_constructible_v<std::adopt_lock_t>);
template<class T> concept implicit_default = requires { [](T) {}({}); };
static_assert(!implicit_default<std::defer_lock_t>);
static_assert(!implicit_default<std::try_to_lock_t>);
static_assert(!implicit_default<std::adopt_lock_t>);

int main() {
  Spy s;
  {
    std::lock_guard g(s);
    static_assert(std::is_same_v<decltype(g), std::lock_guard<Spy>>);
    CHECK(s.locks == 1 && s.unlocks == 0);
  }
  CHECK(s.locks == 1 && s.unlocks == 1);
  {
    std::lock_guard<Spy> g(s, std::adopt_lock);
    CHECK(s.locks == 1);
  }
  CHECK(s.unlocks == 2);

  std::mutex m;
  {
    std::lock_guard g(m);
  }
  m.lock();
  { std::lock_guard g(m, std::adopt_lock); }
  CHECK(m.try_lock() || m.try_lock() || m.try_lock());
  m.unlock();
  return 0;
}
