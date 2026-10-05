// [futures.task.members]: packaged_task() has no shared state (valid() false); packaged_task(f)
// stores the task and creates a shared state; get_future() shares it ("future_already_retrieved"
// the second time, "no_state" without one); operator() runs INVOKE<R>(f, args...) and stores
// the result or the exception, making the state ready; a second call throws
// promise_already_satisfied; reset() gives a fresh shared state (the old one abandoned:
// broken_promise); move leaves the source without a shared state; ~packaged_task abandons.
// The deduction guide deduces R(A...).
// REQUIRES: exceptions
#include <future>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

template<class F>
static bool throws(F f, std::future_errc e) {
  try {
    f();
  } catch (const std::future_error& fe) {
    return fe.code() == std::make_error_code(e);
  }
  return false;
}

static int add(int a, int b) { return a + b; }

static_assert(std::is_nothrow_default_constructible_v<std::packaged_task<int()>>);
static_assert(!std::is_copy_constructible_v<std::packaged_task<int()>>);
static_assert(std::is_nothrow_move_constructible_v<std::packaged_task<int()>>);

int main() {
  using E = std::future_errc;
  std::packaged_task<int()> none;
  CHECK(!none.valid());
  CHECK(throws([&] { none.get_future(); }, E::no_state));
  CHECK(throws([&] { none(); }, E::no_state));
  CHECK(throws([&] { none.reset(); }, E::no_state));

  std::packaged_task<int(int, int)> t(add);
  CHECK(t.valid());
  auto f = t.get_future();
  CHECK(throws([&] { t.get_future(); }, E::future_already_retrieved));
  CHECK(f.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
  t(2, 3);
  CHECK(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
  CHECK(f.get() == 5);
  CHECK(throws([&] { t(1, 1); }, E::promise_already_satisfied));

  // reset: new shared state, same task
  t.reset();
  CHECK(t.valid());
  auto f2 = t.get_future();
  t(10, 20);
  CHECK(f2.get() == 30);

  // reset abandons a state that was never made ready
  std::packaged_task<int(int, int)> u(add);
  auto old = u.get_future();
  u.reset();
  CHECK(throws([&] { old.get(); }, E::broken_promise));

  // exceptions are stored
  std::packaged_task<void()> thrower([] { throw std::domain_error("d"); });
  auto fe = thrower.get_future();
  thrower();
  bool caught = false;
  try { fe.get(); } catch (const std::domain_error&) { caught = true; }
  CHECK(caught);

  // INVOKE<R>: the result converts to R
  std::packaged_task<long(short)> conv([](int x) { return x * 2; });
  auto fc = conv.get_future();
  conv(21);
  CHECK(fc.get() == 42L);

  // move and swap
  std::packaged_task<int()> a([] { return 1; });
  std::packaged_task<int()> b(std::move(a));
  CHECK(!a.valid() && b.valid());
  std::packaged_task<int()> c([] { return 2; });
  b.swap(c);
  auto fb = b.get_future();
  b();
  CHECK(fb.get() == 2);
  swap(b, c);
  auto fc2 = b.get_future();
  b();
  CHECK(fc2.get() == 1);

  // destruction without invocation abandons
  std::future<int> fd;
  {
    std::packaged_task<int()> d([] { return 0; });
    fd = d.get_future();
  }
  CHECK(throws([&] { fd.get(); }, E::broken_promise));

  // deduction guides
  std::packaged_task g1(add);
  static_assert(std::is_same_v<decltype(g1), std::packaged_task<int(int, int)>>);
  std::packaged_task g2([](double) noexcept -> char { return 'a'; });
  static_assert(std::is_same_v<decltype(g2), std::packaged_task<char(double)>>);
  return 0;
}
