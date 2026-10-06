// this_thread::sync_wait and sync_wait_with_variant ([exec.sync.wait], [exec.sync.wait.var]):
//   sync_wait(sndr) blocks until sndr completes and returns optional<tuple<decayed values>>:
//   engaged for a value completion, disengaged for stopped; an error is thrown as
//   AS-EXCEPT-PTR(err) ([exec.general]/8): an exception_ptr is rethrown, an error_code becomes
//   system_error, any other error is thrown as itself. The receiver's environment has the
//   run_loop's scheduler as get_scheduler, get_start_scheduler and get_delegation_scheduler.
//   sync_wait_with_variant(sndr) returns optional<variant<tuple<...>...>>.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <variant>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

struct my_error {
  int code;
};

// A sender with one value completion that completes with the error e.
template <class E>
struct fails_with {
  using sender_concept = ex::sender_tag;
  E e;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(E)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    E e;
    void start() & noexcept { ex::set_error(std::move(r), std::move(e)); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), e};
  }
};

int main() {
  // Result types.
  {
    auto r = tt::sync_wait(ex::just(1, std::string("a")));
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<int, std::string>>>);
    CHECK(r && std::get<0>(*r) == 1 && std::get<1>(*r) == "a");
    const int c = 5;
    auto rr = tt::sync_wait(ex::just() | ex::then([&]() noexcept -> const int& { return c; }));
    static_assert(std::is_same_v<decltype(rr), std::optional<std::tuple<int>>>); // decayed
    CHECK(rr && std::get<0>(*rr) == 5);
    auto e = tt::sync_wait(ex::just());
    static_assert(std::is_same_v<decltype(e), std::optional<std::tuple<>>>);
    CHECK(e.has_value());
  }
  // Stopped: a disengaged optional.
  {
    auto r = tt::sync_wait(ex::just(1) | ex::let_value([](int) { return ex::just_stopped(); }) | ex::upon_error([](auto) noexcept {}));
    CHECK(!r);
  }
  // Errors.
  {
    bool caught = false;
    try {
      (void)tt::sync_wait(fails_with<my_error>{my_error{3}});
    } catch (const my_error& e) {
      caught = e.code == 3;
    }
    CHECK(caught);
    caught = false;
    try {
      (void)tt::sync_wait(fails_with<std::error_code>{std::make_error_code(std::errc::timed_out)});
    } catch (const std::system_error& e) {
      caught = e.code() == std::errc::timed_out;
    }
    CHECK(caught);
    caught = false;
    try {
      (void)tt::sync_wait(fails_with<std::exception_ptr>{std::make_exception_ptr(std::out_of_range("r"))});
    } catch (const std::out_of_range&) {
      caught = true;
    }
    CHECK(caught);
  }
  // The environment: the run_loop's scheduler, on this thread.
  {
    auto me = tt::get_id();
    auto r = tt::sync_wait(ex::read_env(ex::get_scheduler) | ex::let_value([](auto sch) { return ex::schedule(sch); }) |
                           ex::then([] { return tt::get_id(); }));
    CHECK(r && std::get<0>(*r) == me);
    auto d = tt::sync_wait(ex::read_env(ex::get_delegation_scheduler));
    auto s = tt::sync_wait(ex::read_env(ex::get_start_scheduler));
    CHECK(d && s);
  }
  // Completion on another thread: sync_wait blocks until it.
  {
    ex::run_loop loop;
    std::thread t([&] { loop.run(); });
    const auto tid = t.get_id();
    auto r = tt::sync_wait(ex::schedule(loop.get_scheduler()) | ex::then([] { return tt::get_id(); }));
    loop.finish();
    t.join();
    CHECK(r && std::get<0>(*r) == tid);
  }
  // sync_wait_with_variant
  {
    auto r = tt::sync_wait_with_variant(ex::just(2.5));
    static_assert(std::is_same_v<decltype(r), std::optional<std::variant<std::tuple<double>>>>);
    CHECK(r && std::get<0>(std::get<0>(*r)) == 2.5);
  }
  return 0;
}
