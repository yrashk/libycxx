// [futures.promise]/14-15: get_future() twice: future_already_retrieved; on a moved-from promise
// (no shared state): no_state. /17-18, /21-22: set_value / set_exception when a result is
// already stored: promise_already_satisfied; no shared state: no_state. [futures.state]/7 and
// [futures.promise]/7: a promise destroyed without a result abandons its state, which stores a
// future_error with broken_promise.
#include <future>
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

int main() {
  using E = std::future_errc;
  std::promise<int> p;
  auto f = p.get_future();
  CHECK(throws([&] { p.get_future(); }, E::future_already_retrieved));
  p.set_value(1);
  CHECK(throws([&] { p.set_value(2); }, E::promise_already_satisfied));
  CHECK(throws([&] { p.set_exception(std::make_exception_ptr(1)); }, E::promise_already_satisfied));
  CHECK(throws([&] { p.set_value_at_thread_exit(3); }, E::promise_already_satisfied));
  CHECK(f.get() == 1);

  std::promise<void> pv;
  pv.set_exception(std::make_exception_ptr(5));
  CHECK(throws([&] { pv.set_value(); }, E::promise_already_satisfied));

  std::promise<int> src;
  std::promise<int> dst(std::move(src));
  CHECK(throws([&] { src.get_future(); }, E::no_state));
  CHECK(throws([&] { src.set_value(1); }, E::no_state));
  CHECK(throws([&] { src.set_exception(std::make_exception_ptr(1)); }, E::no_state));

  // broken promise
  std::future<int> fb;
  {
    std::promise<int> pb;
    fb = pb.get_future();
  }
  CHECK(fb.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
  CHECK(throws([&] { fb.get(); }, E::broken_promise));

  std::future<void> fbv;
  {
    std::promise<void> pb;
    fbv = pb.get_future();
    std::promise<void> other;
    pb = std::move(other);  // the old state is abandoned
  }
  CHECK(throws([&] { fbv.get(); }, E::broken_promise));

  // a result that is set is not broken by destruction
  std::future<int> fok;
  {
    std::promise<int> pk;
    fok = pk.get_future();
    pk.set_value(8);
  }
  CHECK(fok.get() == 8);
  return 0;
}
