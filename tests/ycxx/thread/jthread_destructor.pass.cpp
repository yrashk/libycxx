// [thread.jthread.cons]/12: ~jthread(): "If joinable() is true, calls request_stop() and then
// join()." /13: move assignment: "if joinable() is true, calls request_stop() and then join(),
// then assigns the state of x to *this and sets x to a default constructed state." /11: move
// construction transfers the stop source; "x.ssource.stop_possible() is false".
// FLAGS: -pthread
#include <thread>
#include <stop_token>
#include <atomic>
#include <utility>
#include "check.hpp"

static void until_stopped(std::stop_token st, std::atomic<int>* out) {
  while (!st.stop_requested()) std::this_thread::yield();
  out->fetch_add(1);
}

int main() {
  std::atomic<int> finished(0);
  {
    std::jthread t(until_stopped, &finished);
  }  // would never return without the stop request
  CHECK(finished.load() == 1);

  std::jthread a(until_stopped, &finished);
  auto id = a.get_id();
  std::stop_source src = a.get_stop_source();
  std::jthread b(std::move(a));
  CHECK(!a.joinable());
  CHECK(!a.get_stop_source().stop_possible());
  CHECK(b.get_id() == id);
  CHECK(b.get_stop_source() == src);

  std::jthread c(until_stopped, &finished);
  std::stop_token ctok = c.get_stop_token();
  c = std::move(b);  // stops and joins the thread c ran
  CHECK(ctok.stop_requested());
  CHECK(finished.load() == 2);
  CHECK(c.get_id() == id);
  CHECK(!b.joinable());
  CHECK(!b.get_stop_source().stop_possible());

  c = std::move(c);  // self-move: no effects
  CHECK(c.get_id() == id && c.joinable());

  std::jthread d;
  d.swap(c);
  CHECK(d.get_id() == id && !c.joinable());
  swap(c, d);
  CHECK(c.get_id() == id);
  return 0;  // c's destructor stops and joins
}
