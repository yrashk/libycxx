// [futures.async]/3.1: with launch::async the function runs "as if in a new thread of
// execution represented by a thread object"; the result or exception is stored; /4.1: a waiting
// function blocks until the associated thread has completed. The default policy is
// async | deferred. Return type future<invoke_result_t<decay_t<F>, decay_t<Args>...>>; INVOKE
// with a member pointer; reference results.
// FLAGS: -pthread
// COUNTERPART: libstdcxx:30_threads/async/except.cc
#include <future>
#include <thread>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

struct Obj {
  int v = 3;
  int twice() const { return v * 2; }
};
static int global = 0;
static int& ref() { return global; }

int main() {
  std::thread::id main_id = std::this_thread::get_id();
  auto f = std::async(std::launch::async, [main_id] { return std::this_thread::get_id() != main_id; });
  CHECK(f.get());

  auto e = std::async(std::launch::async, [] { throw std::invalid_argument("x"); });
  static_assert(std::is_same_v<decltype(e), std::future<void>>);
  bool caught = false;
  try { e.get(); } catch (const std::invalid_argument&) { caught = true; }
  CHECK(caught);

  Obj o;
  auto m = std::async(&Obj::twice, &o);
  CHECK(m.get() == 6);
  auto d = std::async(std::launch::async | std::launch::deferred, [](int a) { return a; }, 4);
  CHECK(d.get() == 4);

  auto r = std::async(std::launch::async, ref);
  static_assert(std::is_same_v<decltype(r), std::future<int&>>);
  CHECK(&r.get() == &global);

  // waiting with a timeout on a running async task
  std::promise<void> gate;
  std::shared_future<void> open = gate.get_future().share();
  auto slow = std::async(std::launch::async, [open] { open.wait(); return 1; });
  CHECK(slow.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout);
  gate.set_value();
  CHECK(slow.wait_for(std::chrono::seconds(30)) == std::future_status::ready);
  CHECK(slow.get() == 1);
  return 0;
}
