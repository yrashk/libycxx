// [thread.jthread.cons]: jthread() does not represent a thread and "ssource.stop_possible() is
// false" (/2); the constructor invokes f with get_stop_token() first if that is well-formed,
// otherwise without it (/6); afterwards get_id() != id() and the stop source is engaged (/8).
// [thread.jthread.stop]: get_stop_source / get_stop_token / request_stop.
// FLAGS: -pthread
#include <thread>
#include <stop_token>
#include <atomic>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::jthread>);
static_assert(std::is_nothrow_move_constructible_v<std::jthread>);
static_assert(std::is_nothrow_move_assignable_v<std::jthread>);
static_assert(!std::is_copy_constructible_v<std::jthread>);

int main() {
  std::jthread e;
  CHECK(!e.joinable());
  CHECK(e.get_id() == std::jthread::id());
  CHECK(!e.get_stop_source().stop_possible());
  CHECK(!e.get_stop_token().stop_possible());
  CHECK(!e.request_stop());

  // without a stop_token parameter
  int v = 0;
  {
    std::jthread t([&](int a, int b) { v = a * b; }, 6, 7);
    CHECK(t.joinable());
    CHECK(t.get_stop_source().stop_possible());
    CHECK(!t.get_stop_token().stop_requested());
  }
  CHECK(v == 42);

  // with a stop_token as first parameter: the token is the jthread's own
  std::atomic<bool> started(false);
  std::jthread t([&](std::stop_token st, int step) {
    started = true;
    started.notify_one();
    while (!st.stop_requested()) std::this_thread::yield();
    v += step;
  }, 8);
  started.wait(false);
  std::stop_token tok = t.get_stop_token();
  CHECK(tok.stop_possible() && !tok.stop_requested());
  CHECK(tok == t.get_stop_source().get_token());
  CHECK(t.request_stop());
  CHECK(!t.request_stop());  // already requested
  CHECK(tok.stop_requested());
  CHECK(t.get_stop_source().stop_requested());
  t.join();
  CHECK(v == 50);
  // after join the stop state is still there
  CHECK(t.get_stop_token().stop_requested());
  return 0;
}
