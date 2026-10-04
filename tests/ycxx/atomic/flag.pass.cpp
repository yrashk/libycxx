// [atomics.flag]: atomic_flag "has two states, set and clear"; the default constructor
// "Initializes *this to the clear state" (/4); test returns the value (/8); test_and_set
// sets to true and returns the previous value (/9-10); clear sets to false (/12);
// ATOMIC_FLAG_INIT initializes to the clear state (/21); /3: standard-layout, trivial
// destructor; non-copyable. The non-member functions.
#include <atomic>
#include <type_traits>
#include "check.hpp"

static std::atomic_flag global = ATOMIC_FLAG_INIT;

static_assert(std::is_standard_layout_v<std::atomic_flag>);
static_assert(std::is_trivially_destructible_v<std::atomic_flag>);
static_assert(!std::is_copy_constructible_v<std::atomic_flag>);
static_assert(!std::is_copy_assignable_v<std::atomic_flag>);
static_assert(std::is_nothrow_default_constructible_v<std::atomic_flag>);

int main() {
  CHECK(!global.test());
  std::atomic_flag f;
  CHECK(!f.test());
  CHECK(!f.test_and_set());
  CHECK(f.test());
  CHECK(f.test(std::memory_order::acquire));
  CHECK(f.test_and_set(std::memory_order::relaxed));
  f.clear();
  CHECK(!f.test(std::memory_order::relaxed));
  CHECK(!f.test_and_set(std::memory_order::acq_rel));
  f.clear(std::memory_order::release);
  CHECK(!f.test());

  CHECK(!std::atomic_flag_test_and_set(&f));
  CHECK(std::atomic_flag_test(&f));
  CHECK(std::atomic_flag_test_explicit(&f, std::memory_order::seq_cst));
  CHECK(std::atomic_flag_test_and_set_explicit(&f, std::memory_order::seq_cst));
  std::atomic_flag_clear(&f);
  CHECK(!std::atomic_flag_test(&f));
  std::atomic_flag_test_and_set(&f);
  std::atomic_flag_clear_explicit(&f, std::memory_order::relaxed);
  CHECK(!f.test());

  volatile std::atomic_flag vf;
  CHECK(!vf.test_and_set());
  CHECK(vf.test());
  vf.clear();
  CHECK(!std::atomic_flag_test(&vf));

  const std::atomic_flag& cf = f;
  CHECK(!cf.test());
  CHECK(!std::atomic_flag_test(&cf));

  // wait returns at once when test() != old
  f.wait(true);
  std::atomic_flag_wait(&f, true);
  std::atomic_flag_wait_explicit(&f, true, std::memory_order::acquire);
  f.notify_one();
  f.notify_all();
  std::atomic_flag_notify_one(&f);
  std::atomic_flag_notify_all(&f);
  return 0;
}
