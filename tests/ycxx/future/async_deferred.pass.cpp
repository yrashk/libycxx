// [futures.async]/3.2: with launch::deferred the function and arguments are stored ("auto(...)"
// copies) and "The first call to a non-timed waiting function on an asynchronous return object
// referring to this shared state invokes the deferred function in the thread that called the
// waiting function." Exceptions are stored. [futures.unique.future]/22-23: wait_for / wait_until
// have no effect and return future_status::deferred while the function is deferred.
// FLAGS: -pthread
#include <future>
#include <thread>
#include <chrono>
#include <memory>
#include <stdexcept>
#include "check.hpp"

int main() {
  int calls = 0;
  std::thread::id ran_on;
  auto f = std::async(std::launch::deferred, [&](int a, int b) {
    ++calls;
    ran_on = std::this_thread::get_id();
    return a + b;
  }, 20, 22);
  static_assert(std::is_same_v<decltype(f), std::future<int>>);
  CHECK(calls == 0);
  CHECK(f.wait_for(std::chrono::seconds(0)) == std::future_status::deferred);
  CHECK(f.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(1)) == std::future_status::deferred);
  CHECK(calls == 0);
  CHECK(f.get() == 42);
  CHECK(calls == 1);
  CHECK(ran_on == std::this_thread::get_id());

  // wait() runs it; get() afterwards does not run it again
  auto g = std::async(std::launch::deferred, [&] { ++calls; });
  g.wait();
  CHECK(calls == 2);
  CHECK(g.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
  g.get();
  CHECK(calls == 2);

  // stored exception
  auto h = std::async(std::launch::deferred, [] { throw std::out_of_range("r"); return 1; });
  bool caught = false;
  try { h.get(); } catch (const std::out_of_range&) { caught = true; }
  CHECK(caught);

  // never waited on: never called
  {
    auto n = std::async(std::launch::deferred, [&] { ++calls; });
  }
  CHECK(calls == 2);

  // arguments are copied at the call: later changes are not seen; move-only arguments work
  int v = 1;
  auto c = std::async(std::launch::deferred, [](int x) { return x; }, v);
  v = 2;
  CHECK(c.get() == 1);
  auto m = std::async(std::launch::deferred, [](std::unique_ptr<int> p) { return *p; }, std::make_unique<int>(6));
  CHECK(m.get() == 6);

  // a shared_future runs it on the first waiting thread only
  auto sf = std::async(std::launch::deferred, [&] { return ++calls; }).share();
  std::thread t([sf] { (void)sf.get(); });
  t.join();
  CHECK(sf.get() == 3 && calls == 3);
  return 0;
}
