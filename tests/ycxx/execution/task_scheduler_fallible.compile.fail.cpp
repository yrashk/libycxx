// [exec.task.scheduler]/2: task_scheduler(sch) mandates infallible-scheduler<Sch, env<>>
// ([exec.sched]/8): in env<> (an unstoppable token) the schedule sender's completion signatures
// must be completion_signatures<set_value_t()>. A scheduler whose schedule sender can complete
// with an error is rejected.
// EXPECT-ERROR: static assert.*task_scheduler.*infallible
#include <execution>

namespace ex = std::execution;

struct fallible_scheduler {
  using scheduler_concept = ex::scheduler_tag;
  struct sender {
    using sender_concept = ex::sender_tag;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(int)>();
    }
    template <class R>
    struct op {
      using operation_state_concept = ex::operation_state_tag;
      R r;
      void start() & noexcept { ex::set_value(static_cast<R&&>(r)); }
    };
    template <class R>
    op<R> connect(R r) const noexcept {
      return {static_cast<R&&>(r)};
    }
  };
  sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const fallible_scheduler&) const = default;
};
static_assert(ex::scheduler<fallible_scheduler>);

void f() { ex::task_scheduler ts(fallible_scheduler{}); }
