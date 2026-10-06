// [exec.starts.on]/4: starts_on.transform_sender(set_value, out_sndr, env) is
// let_value(continues_on(just(), sch), [sndr]() { return std::move(sndr); }), so the
// completions of scheduling onto sch are among starts_on's; /5: if scheduling onto sch fails,
// the receiver gets an error completion (and sndr is not started); a stopped schedule operation
// likewise completes the starts_on operation with stopped. /2: starts_on(sch, sndr) needs a
// scheduler and a sender.
#include <execution>
#include <exception>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// A scheduler whose schedule operation completes as told: 0 value, 1 error 99, 2 stopped.
struct flaky_scheduler {
  using scheduler_concept = ex::scheduler_tag;
  int mode = 0;
  struct sender {
    using sender_concept = ex::sender_tag;
    int mode;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(int), ex::set_stopped_t()>();
    }
    template <class R>
    struct op {
      using operation_state_concept = ex::operation_state_tag;
      R r;
      int mode;
      void start() & noexcept {
        if (mode == 0)
          ex::set_value(std::move(r));
        else if (mode == 1)
          ex::set_error(std::move(r), 99);
        else
          ex::set_stopped(std::move(r));
      }
    };
    template <class R>
    op<R> connect(R r) const noexcept {
      return {std::move(r), mode};
    }
  };
  sender schedule() const noexcept { return {mode}; }
  constexpr ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const flaky_scheduler&) const = default;
};
static_assert(ex::scheduler<flaky_scheduler>);

template <class S>
auto tag_of_sender(S& s) {
  auto&& [tag, data, child] = s;
  return tag;
}

int main() {
  using EP = std::exception_ptr;
  using S = decltype(ex::starts_on(flaky_scheduler{}, ex::just(1)));
  using CS = ex::completion_signatures_of_t<S, ex::env<>>;
  static_assert(sigs_subset<ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(int), ex::set_stopped_t()>, CS>);
  static_assert(sigs_subset<CS, ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(int), ex::set_stopped_t(), ex::set_error_t(EP)>>);

  // /4
  {
    auto s = ex::starts_on(flaky_scheduler{}, ex::just(1));
    auto t = ex::starts_on.transform_sender(ex::set_value, std::move(s), ex::env<>());
    static_assert(std::is_same_v<decltype(tag_of_sender(t)), ex::let_value_t>);
  }
  for (int mode = 0; mode < 3; ++mode) {
    bool started = false;
    record<int, int> rec;
    run(ex::starts_on(flaky_scheduler{mode}, ex::just(1) | ex::then([&](int x) noexcept {
                                               started = true;
                                               return x + 1;
                                             })),
        receiver_for(rec));
    CHECK(rec.calls == 1);
    if (mode == 0) {
      CHECK(started && rec.how == done::value && std::get<0>(*rec.values) == 2);
    } else if (mode == 1) {
      CHECK(!started && rec.how == done::error && *rec.error == 99);
    } else {
      CHECK(!started && rec.how == done::stopped);
    }
  }
  // /2
  static_assert(!std::is_invocable_v<ex::starts_on_t, int, decltype(ex::just())>);
  static_assert(!std::is_invocable_v<ex::starts_on_t, flaky_scheduler, int>);
  static_assert(std::is_invocable_v<ex::starts_on_t, flaky_scheduler, decltype(ex::just())>);
  return 0;
}
