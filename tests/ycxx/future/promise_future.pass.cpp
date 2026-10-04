// [futures.promise]: promise creates a shared state; get_future() shares it; set_value /
// set_exception store the result and make the state ready; [futures.unique.future]: get()
// waits, returns std::move(v) (or the reference for R&, nothing for void), throws the stored
// exception, and leaves valid() == false; a default future has valid() == false; move leaves
// the source invalid. Values cross threads (/9: setting synchronizes with detecting ready).
// FLAGS: -pthread
#include <future>
#include <thread>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::future<int>>);
static_assert(std::is_nothrow_move_constructible_v<std::future<int>>);
static_assert(!std::is_copy_constructible_v<std::future<int>>);
static_assert(!std::is_copy_constructible_v<std::promise<int>>);
static_assert(std::is_nothrow_move_constructible_v<std::promise<int>>);
static_assert(std::is_same_v<decltype(std::declval<std::future<int>&>().get()), int>);
static_assert(std::is_same_v<decltype(std::declval<std::future<int&>&>().get()), int&>);
static_assert(std::is_same_v<decltype(std::declval<std::future<void>&>().get()), void>);
static_assert(noexcept(std::declval<const std::future<int>&>().valid()));

int main() {
  std::future<int> none;
  CHECK(!none.valid());

  std::promise<int> p;
  std::future<int> f = p.get_future();
  CHECK(f.valid());
  CHECK(f.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
  p.set_value(42);
  CHECK(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
  f.wait();
  CHECK(f.get() == 42);
  CHECK(!f.valid());

  // move-only value: get returns std::move(v)
  std::promise<std::unique_ptr<int>> pu;
  auto fu = pu.get_future();
  pu.set_value(std::make_unique<int>(5));
  std::unique_ptr<int> got = fu.get();
  CHECK(*got == 5);

  // lvalue set_value copies
  std::promise<std::string> ps;
  auto fs = ps.get_future();
  std::string s = "hello";
  ps.set_value(s);
  CHECK(s == "hello" && fs.get() == "hello");

  // references
  int x = 1;
  std::promise<int&> pr;
  auto fr = pr.get_future();
  pr.set_value(x);
  int& rx = fr.get();
  CHECK(&rx == &x);

  // void
  std::promise<void> pv;
  auto fv = pv.get_future();
  pv.set_value();
  fv.get();
  CHECK(!fv.valid());

  // exception
  std::promise<int> pe;
  auto fe = pe.get_future();
  pe.set_exception(std::make_exception_ptr(std::runtime_error("bad")));
  bool caught = false;
  try {
    fe.get();
  } catch (const std::runtime_error& e) {
    caught = std::string(e.what()) == "bad";
  }
  CHECK(caught && !fe.valid());

  // across threads
  std::promise<int> pt;
  std::future<int> ft = pt.get_future();
  std::thread t([p = std::move(pt)]() mutable { p.set_value(7); });
  CHECK(ft.get() == 7);
  t.join();

  // move
  std::promise<int> pm;
  std::future<int> a = pm.get_future();
  std::future<int> b(std::move(a));
  CHECK(!a.valid() && b.valid());
  a = std::move(b);
  CHECK(a.valid() && !b.valid());
  std::promise<int> pm2(std::move(pm));
  pm2.set_value(3);
  CHECK(a.get() == 3);
  std::promise<int> q1, q2;
  auto fq1 = q1.get_future();
  q1.swap(q2);
  q2.set_value(9);  // q2 now holds fq1's state
  CHECK(fq1.get() == 9);
  swap(q1, q2);
  return 0;
}
