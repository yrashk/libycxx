// [exec.sync.wait]/2: sync-wait-env answers get_scheduler, get_start_scheduler and
// get_delegation_scheduler with the same run_loop's scheduler (so they compare equal,
// [exec.run.loop.types]); /10, /11.1: the run_loop is driven by the calling thread, so work
// another thread schedules on the delegation scheduler runs on the thread blocked in sync_wait.
// /7: the receiver's set_value emplaces the result inside try; an exception from decay-copying
// a datum is stored and rethrown by sync_wait (/10).
// [exec.sync.wait.var]/3, /4.2: sync_wait_with_variant returns an engaged optional with a
// variant of tuples (the alternative of the value completion that happened), a disengaged one
// for stopped, and throws for an error.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <variant>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

bool copy_throws = false;
struct thrower {
  int v = 0;
  thrower(int x) : v(x) {}
  thrower(const thrower& o) : v(o.v) {
    if (copy_throws)
      throw 42;
  }
  thrower(thrower&&) noexcept = default;
};

// Sends an lvalue of *obj.
struct ref_sender {
  using sender_concept = ex::sender_tag;
  thrower* obj;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(thrower&)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    thrower* obj;
    void start() & noexcept { ex::set_value(std::move(r), *obj); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), obj};
  }
};

// Completes as told: 0 an int, 1 a string, 2 stopped, 3 an error.
struct either {
  using sender_concept = ex::sender_tag;
  int how;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(std::string), ex::set_stopped_t(), ex::set_error_t(int)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    int how;
    void start() & noexcept {
      switch (how) {
      case 0: ex::set_value(std::move(r), 5); break;
      case 1: ex::set_value(std::move(r), std::string("five")); break;
      case 2: ex::set_stopped(std::move(r)); break;
      default: ex::set_error(std::move(r), 55);
      }
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), how};
  }
};

int main() {
  // /2: the three queries give equal schedulers.
  {
    auto r = tt::sync_wait(ex::when_all(ex::read_env(ex::get_scheduler), ex::read_env(ex::get_start_scheduler),
                                        ex::read_env(ex::get_delegation_scheduler)));
    CHECK(r.has_value());
    auto& [a, b, c] = *r;
    CHECK(a == b && b == c);
  }
  // /11.1: work scheduled on the delegation scheduler from another thread runs here.
  {
    ex::run_loop other;
    std::thread t([&] { other.run(); });
    const auto me = tt::get_id();
    auto r = tt::sync_wait(ex::read_env(ex::get_delegation_scheduler) | ex::let_value([&](auto d) {
                             return ex::schedule(other.get_scheduler()) | ex::then([] { return tt::get_id(); }) | ex::continues_on(d) |
                                    ex::then([](std::thread::id there) { return std::tuple(there, tt::get_id()); });
                           }));
    other.finish();
    const auto tid = t.get_id();
    t.join();
    CHECK(r && std::get<0>(std::get<0>(*r)) == tid && std::get<1>(std::get<0>(*r)) == me);
  }
  // /7: a throwing decay-copy of a datum.
  {
    thrower x{3};
    auto r = tt::sync_wait(ref_sender{&x});
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<thrower>>>);
    CHECK(r && std::get<0>(*r).v == 3);
    copy_throws = true;
    bool caught = false;
    try {
      (void)tt::sync_wait(ref_sender{&x});
    } catch (int e) {
      caught = e == 42;
    }
    copy_throws = false;
    CHECK(caught);
  }
  // sync_wait_with_variant
  {
    using R = decltype(tt::sync_wait_with_variant(either{0}));
    static_assert(std::is_same_v<R, std::optional<std::variant<std::tuple<int>, std::tuple<std::string>>>>);
    auto r0 = tt::sync_wait_with_variant(either{0});
    CHECK(r0 && r0->index() == 0 && std::get<0>(std::get<0>(*r0)) == 5);
    auto r1 = tt::sync_wait_with_variant(either{1});
    CHECK(r1 && r1->index() == 1 && std::get<0>(std::get<1>(*r1)) == "five");
    auto r2 = tt::sync_wait_with_variant(either{2});
    CHECK(!r2);
    bool caught = false;
    try {
      (void)tt::sync_wait_with_variant(either{3});
    } catch (int e) {
      caught = e == 55;
    }
    CHECK(caught);
  }
  return 0;
}
