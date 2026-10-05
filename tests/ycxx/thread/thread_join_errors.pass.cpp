// [thread.thread.member]/6-7: join() "Throws: system_error when an exception is required";
// "invalid_argument — if the thread is not joinable"; "resource_deadlock_would_occur — if
// deadlock is detected or get_id() == this_thread::get_id()". /10-11: detach() on a
// non-joinable thread: invalid_argument. [thread.req.exception]: the error_code's condition
// is the errc value.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <thread>
#include <system_error>
#include <atomic>
#include "check.hpp"

template<class F>
static std::errc error_of(F f) {
  try {
    f();
  } catch (const std::system_error& e) {
    if (e.code() == std::errc::invalid_argument) return std::errc::invalid_argument;
    if (e.code() == std::errc::resource_deadlock_would_occur) return std::errc::resource_deadlock_would_occur;
    if (e.code() == std::errc::no_such_process) return std::errc::no_such_process;
    return std::errc::io_error;
  }
  return std::errc{};
}

int main() {
  std::thread t;
  CHECK(error_of([&] { t.join(); }) == std::errc::invalid_argument);
  CHECK(error_of([&] { t.detach(); }) == std::errc::invalid_argument);

  std::thread done([] {});
  done.join();
  CHECK(error_of([&] { done.join(); }) == std::errc::invalid_argument);
  CHECK(error_of([&] { done.detach(); }) == std::errc::invalid_argument);

  // joining oneself (main touches s again only after the thread is done with it:
  // "Operations on *this are not synchronized")
  std::atomic<std::thread*> self(nullptr);
  std::atomic<bool> joined(false);
  std::errc inner{};
  std::thread s([&] {
    self.wait(nullptr);
    inner = error_of([&] { self.load()->join(); });
    joined = true;
    joined.notify_one();
  });
  self = &s;
  self.notify_one();
  joined.wait(false);
  s.join();
  CHECK(inner == std::errc::resource_deadlock_would_occur);

  std::jthread j;
  CHECK(error_of([&] { j.join(); }) == std::errc::invalid_argument);
  CHECK(error_of([&] { j.detach(); }) == std::errc::invalid_argument);
  return 0;
}
