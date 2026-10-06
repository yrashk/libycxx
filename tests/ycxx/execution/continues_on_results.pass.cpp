// [exec.continues.on]/8, /11: continues_on(sndr, sch) decay-copies sndr's result into its state
// before it schedules onto sch (a datum sent by reference is copied: later changes to the
// referenced object are not seen), and a decay-copy that throws becomes set_error(
// exception_ptr), which is then among the completion signatures only when some decay-copy can
// throw. /9: when the schedule operation itself completes with an error or with stopped, that
// completion is forwarded to the receiver instead of the stored result. /6: the signatures
// include those of the scheduler's schedule sender.
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::done;

template <class...>
struct types {};

int shared = 1;
// Sends shared by reference.
struct ref_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int&)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), shared); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

struct ThrowsOnCopy {
  ThrowsOnCopy() = default;
  ThrowsOnCopy(const ThrowsOnCopy&) { throw std::runtime_error("copy"); }
};
struct lvalue_thrower {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(const ThrowsOnCopy&)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    ThrowsOnCopy t;
    void start() & noexcept { ex::set_value(std::move(r), std::as_const(t)); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), {}};
  }
};

// A scheduler whose schedule operation completes with an error, or with stopped.
template <bool Stop>
struct failing_sched {
  using scheduler_concept = ex::scheduler_tag;
  struct sndr {
    using sender_concept = ex::sender_tag;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      if constexpr (Stop)
        return ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>();
      else
        return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(long)>();
    }
    template <class R>
    struct op {
      using operation_state_concept = ex::operation_state_tag;
      R r;
      void start() & noexcept {
        if constexpr (Stop)
          ex::set_stopped(std::move(r));
        else
          ex::set_error(std::move(r), 42L);
      }
    };
    template <class R>
    op<R> connect(R r) const noexcept {
      return {std::move(r)};
    }
  };
  sndr schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const failing_sched&) const = default;
};
static_assert(ex::scheduler<failing_sched<false>>);

int main() {
  // The datum is copied before scheduling.
  {
    ex::run_loop loop;
    exec_test::record<int, int> rec;
    auto op = ex::connect(ex::continues_on(ref_sender{}, loop.get_scheduler()), exec_test::receiver_for(rec));
    ex::start(op);
    shared = 99;
    loop.finish();
    loop.run();
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 1);
    using S = decltype(ex::continues_on(ref_sender{}, loop.get_scheduler()));
    // Copying an int cannot throw: no set_error(exception_ptr).
    static_assert(std::is_same_v<ex::error_types_of_t<S, ex::env<>, types>, types<>>);
  }
  // A throwing decay-copy: set_error(exception_ptr), after scheduling.
  {
    ex::run_loop loop;
    exec_test::record<std::exception_ptr, ThrowsOnCopy*> rec;
    using S = decltype(ex::continues_on(lvalue_thrower{}, loop.get_scheduler()));
    static_assert(std::is_same_v<ex::error_types_of_t<S, ex::env<>>, std::variant<std::exception_ptr>>);
    auto op = ex::connect(ex::continues_on(lvalue_thrower{}, loop.get_scheduler()), exec_test::receiver_for(rec));
    ex::start(op);
    CHECK(rec.how == done::none);
    loop.finish();
    loop.run();
    CHECK(rec.how == done::error && *rec.error);
  }
  // The schedule operation fails: its error, or stopped, reaches the receiver.
  {
    using S = decltype(ex::continues_on(ex::just(1), failing_sched<false>{}));
    static_assert(std::is_same_v<ex::error_types_of_t<S, ex::env<>>, std::variant<long>>);
    exec_test::record<long, int> rec;
    exec_test::run(ex::continues_on(ex::just(1), failing_sched<false>{}), exec_test::receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 42L);
    using T = decltype(ex::continues_on(ex::just(1), failing_sched<true>{}));
    static_assert(ex::sends_stopped<T, ex::env<>>);
    exec_test::record<long, int> rec2;
    exec_test::run(ex::continues_on(ex::just(1), failing_sched<true>{}), exec_test::receiver_for(rec2));
    CHECK(rec2.how == done::stopped);
  }
  return 0;
}
