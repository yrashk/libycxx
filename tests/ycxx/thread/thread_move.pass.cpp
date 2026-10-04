// [thread.thread.constr]/11: thread(thread&& x): "x.get_id() == id() and get_id() returns the
// value of x.get_id() prior to the start of construction." [thread.thread.assign]: move
// assignment "If joinable(), invokes terminate. Otherwise, assigns the state of x to *this and
// sets x to a default constructed state." Postconditions: x.get_id() == id() and get_id()
// returns the value of x.get_id() prior to the assignment. [thread.thread.member]/1 swap.
// [thread.thread.algorithm]: swap(x, y).
// FLAGS: -pthread
#include <thread>
#include <utility>
#include "check.hpp"

int main() {
  std::thread a([] {});
  auto id = a.get_id();
  std::thread b(std::move(a));
  CHECK(a.get_id() == std::thread::id());
  CHECK(!a.joinable());
  CHECK(b.get_id() == id);

  std::thread c;
  c = std::move(b);
  CHECK(b.get_id() == std::thread::id());
  CHECK(c.get_id() == id);

  std::thread d([] {});
  auto id2 = d.get_id();
  c.swap(d);
  CHECK(c.get_id() == id2 && d.get_id() == id);
  swap(c, d);
  CHECK(c.get_id() == id && d.get_id() == id2);
  std::swap(c, d);
  CHECK(c.get_id() == id2);
  c.join();
  d.join();
  return 0;
}
