// The sender factories, a few adaptors, and this_thread::sync_wait ([exec]):
//   [exec.just]: just(ts...), just_error(e), just_stopped() complete synchronously in start with
//     set_value(ts...), set_error(e), set_stopped().
//   [exec.sync.wait]: sync_wait mandates a single value completion signature (/1, /5.2); it
//     returns optional<tuple<decayed values...>>: "For a value completion, the result datums
//     are returned in a tuple in an engaged optional object. For an error completion, an
//     exception is thrown" - the error rethrown as AS-EXCEPT-PTR(err) ([exec.general]/8: an
//     exception_ptr as itself).
//   [exec.then]: then / upon_error / upon_stopped invoke f on the value / error / stopped
//     completion and send its result as a value; other completions pass through; an exception
//     from f becomes set_error(current_exception()) (TRY-SET-VALUE, [exec.snd.expos]).
//   [exec.let]: let_value(sndr, f): the sender f returns, called with the values, is connected
//     and started; its completions are the result's.
//   [exec.when.all]: when_all(sndrs...) completes with the values of all, concatenated, once
//     all complete; an error from any becomes the result's error.
//   [exec.stopped.opt]: stopped_as_optional maps a value v to optional(v) and stopped to
//     nullopt.
//   [exec.run.loop]: run() processes the work scheduled on get_scheduler() on the calling
//     thread until finish() has been called and the queue is empty.
// FLAGS: -pthread
// REQUIRES: exceptions
// XFAIL: any  not implemented yet: the senders/receivers part of <execution> (STATUS)
#include <exception>
#include <execution>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

template <class F>
bool throws_int(F f, int v) {
  try {
    f();
  } catch (int e) {
    return e == v;
  } catch (...) {
  }
  return false;
}

void factories() {
  auto r = tt::sync_wait(ex::just(1, 'c', std::string("s")));
  static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<int, char, std::string>>>);
  CHECK(r && std::get<0>(*r) == 1 && std::get<1>(*r) == 'c' && std::get<2>(*r) == "s");

  auto e = tt::sync_wait(ex::just());
  static_assert(std::is_same_v<decltype(e), std::optional<std::tuple<>>>);
  CHECK(e.has_value());

  // sync_wait mandates exactly one value completion signature ([exec.sync.wait]/1, /5.2), so
  // just_error / just_stopped are observed through adaptors below. An exception thrown on the
  // way is the error (an exception_ptr) and is rethrown by sync_wait.
  CHECK(throws_int([] { (void)tt::sync_wait(ex::just(1) | ex::then([](int x) -> int { throw x + 41; })); }, 42));
  try {
    (void)tt::sync_wait(ex::just() | ex::then([]() -> int { throw std::runtime_error("boom"); }));
    CHECK(false);
  } catch (const std::runtime_error& x) {
    CHECK(std::string(x.what()) == "boom");
  }

  // A move-only value is moved through.
  struct MoveOnly {
    int v;
    explicit MoveOnly(int x) : v(x) {}
    MoveOnly(MoveOnly&&) = default;
  };
  auto m = tt::sync_wait(ex::just(MoveOnly(5)));
  CHECK(m && std::get<0>(*m).v == 5);
}

void adaptors() {
  auto r = tt::sync_wait(ex::just(2) | ex::then([](int x) { return x * 10; }) |
                         ex::then([](int x) { return std::to_string(x); }));
  CHECK(r && std::get<0>(*r) == "20");

  // then on a void-returning function: an empty value completion.
  auto v = tt::sync_wait(ex::just(1) | ex::then([](int) {}));
  static_assert(std::is_same_v<decltype(v), std::optional<std::tuple<>>>);
  CHECK(v.has_value());


  // upon_error / upon_stopped turn the error / stopped completion into a value; other
  // completions pass through.
  auto ue = tt::sync_wait(ex::just_error(5) | ex::upon_error([](int e) { return e * 2; }));
  CHECK(ue && std::get<0>(*ue) == 10);
  auto us = tt::sync_wait(ex::just_stopped() | ex::upon_stopped([] { return 7; }));
  CHECK(us && std::get<0>(*us) == 7);
  auto pass = tt::sync_wait(ex::just(3) | ex::upon_error([](auto) { return 0; }));
  CHECK(pass && std::get<0>(*pass) == 3);
  auto ue2 = tt::sync_wait(ex::just_error(std::make_error_code(std::errc::invalid_argument)) |
                          ex::upon_error([](std::error_code ec) { return ec.value(); }));
  CHECK(ue2 && std::get<0>(*ue2) == static_cast<int>(std::errc::invalid_argument));

  // let_value: the returned sender's completion is the result.
  auto lv = tt::sync_wait(ex::just(3) | ex::let_value([](int x) { return ex::just(x, x * 2); }));
  CHECK(lv && std::get<0>(*lv) == 3 && std::get<1>(*lv) == 6);
  auto le = tt::sync_wait(ex::just_error(4) | ex::let_error([](int e) { return ex::just(e + 1); }));
  CHECK(le && std::get<0>(*le) == 5);

  // when_all: values concatenated; an error wins.
  auto wa = tt::sync_wait(ex::when_all(ex::just(1), ex::just(), ex::just('a', 2.5)));
  static_assert(std::is_same_v<decltype(wa), std::optional<std::tuple<int, char, double>>>);
  CHECK(wa && std::get<0>(*wa) == 1 && std::get<1>(*wa) == 'a' && std::get<2>(*wa) == 2.5);
  CHECK(throws_int(
      [] {
        (void)tt::sync_wait(ex::when_all(ex::just(1), ex::just(2) | ex::then([](int x) -> int { throw x + 7; })));
      },
      9));

  // stopped_as_optional.
  auto so = tt::sync_wait(ex::just(4) | ex::stopped_as_optional());
  CHECK(so && std::get<0>(*so) == std::optional<int>(4));
}

void loop() {
  ex::run_loop rl;
  std::thread::id ran_on;
  std::thread worker([&] {
    ran_on = std::this_thread::get_id();
    rl.run();
  });
  auto sch = rl.get_scheduler();
  static_assert(ex::scheduler<decltype(sch)>);
  CHECK(sch == rl.get_scheduler());
  auto r = tt::sync_wait(ex::schedule(sch) | ex::then([] { return std::this_thread::get_id(); }));
  auto w = tt::sync_wait(ex::starts_on(sch, ex::just(5) | ex::then([](int x) { return x + 1; })));
  rl.finish();
  worker.join();
  CHECK(r && std::get<0>(*r) == ran_on && ran_on != std::this_thread::get_id());
  CHECK(w && std::get<0>(*w) == 6);
}

int main() {
  factories();
  adaptors();
  loop();
  return 0;
}
