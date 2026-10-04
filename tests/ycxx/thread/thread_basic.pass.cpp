// [thread.thread.constr]: thread() "does not represent a thread of execution",
// get_id() == id(); the constructor runs invoke(auto(f), auto(args)...) in the new thread with
// the decayed copies materialized in the constructing thread (/6); return value ignored;
// get_id() != id() afterwards (/8). [thread.thread.member]: joinable() is get_id() != id();
// join() blocks until completion and synchronizes with it, then get_id() == id() (/4-5);
// get_id() is the new thread's this_thread::get_id() (/12).
// FLAGS: -pthread
#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::thread>);
static_assert(std::is_nothrow_move_constructible_v<std::thread>);
static_assert(std::is_nothrow_move_assignable_v<std::thread>);
static_assert(!std::is_copy_constructible_v<std::thread>);
static_assert(!std::is_copy_assignable_v<std::thread>);
static_assert(!std::is_convertible_v<void (*)(), std::thread>);  // explicit

struct Counter {
  int calls = 0;
  void add(int n) { calls += n; }
};

static int free_fn(int a, int b) { return a + b; }

int main() {
  std::thread empty;
  CHECK(!empty.joinable());
  CHECK(empty.get_id() == std::thread::id());

  int result = 0;
  std::thread::id seen;
  std::thread t([&](int a, std::string s) {
    result = a + static_cast<int>(s.size());
    seen = std::this_thread::get_id();
  }, 40, std::string("ab"));
  CHECK(t.joinable());
  CHECK(t.get_id() != std::thread::id());
  CHECK(t.get_id() != std::this_thread::get_id());
  std::thread::id tid = t.get_id();
  t.join();
  CHECK(!t.joinable());
  CHECK(t.get_id() == std::thread::id());
  CHECK(result == 42);
  CHECK(seen == tid);

  // arguments are copied: the thread sees its own copy, not a reference
  int x = 1;
  std::thread t2([](int& r) { r = 5; }, std::ref(x));
  t2.join();
  CHECK(x == 5);
  int y = 1;
  std::thread t3([](int v) { v = 7; (void)v; }, y);
  t3.join();
  CHECK(y == 1);

  // pointer to member function, return value ignored
  Counter c;
  std::thread t4(&Counter::add, &c, 3);
  t4.join();
  CHECK(c.calls == 3);
  std::thread t5(free_fn, 1, 2);
  t5.join();

  // move-only arguments and callables
  auto up = std::make_unique<int>(9);
  int got = 0;
  std::thread t6([&got](std::unique_ptr<int> p) { got = *p; }, std::move(up));
  t6.join();
  CHECK(got == 9 && up == nullptr);
  return 0;
}
