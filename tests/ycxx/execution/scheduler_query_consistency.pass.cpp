// [exec.sched]/5-6 for the library's schedulers: get_completion_domain<T>(sch, envs...) and
// get_completion_domain<T>(get_env(schedule(sch)), envs...) are both ill-formed or both
// well-formed with the same type; likewise get_completion_scheduler<T>, which also compares
// equal; get_completion_scheduler<set_value_t>(get_env(schedule(sch))) == sch when well-formed.
// Checked for the tags whose completions run on the scheduler ([exec.get.compl.sched]/6 and
// [exec.get.compl.domain]/3 make asking for others ill-formed): run_loop's set_value and
// set_stopped ([exec.run.loop.types]/5-6; a scheduler without a domain has default_domain given
// an environment, [exec.get.compl.domain]/2.4), and the set_value of inline_scheduler,
// parallel_scheduler and task_scheduler ([exec.inline.scheduler]/1-2, [exec.par.scheduler],
// [exec.task.scheduler]/1, /13). Only with an environment: without one,
// [exec.get.compl.sched]/5.2 gives a scheduler no completion scheduler while
// [exec.run.loop.types]/5 requires one of run_loop's schedule sender (STATUS.md, draft issues).
#include <execution>
#include <stop_token>
#include <type_traits>
#include "check.hpp"

namespace ex = std::execution;

template <class T, class... A>
concept has_domain = requires(A... a) { ex::get_completion_domain<T>(a...); };
template <class T, class... A>
concept has_sched = requires(A... a) { ex::get_completion_scheduler<T>(a...); };

template <class T, class Sch, class... Env>
void check(const Sch& sch, const Env&... env) {
  auto attrs = ex::get_env(ex::schedule(sch));
  using A = decltype(attrs);
  static_assert(has_domain<T, Sch, Env...> == has_domain<T, A, Env...>);
  if constexpr (has_domain<T, Sch, Env...>)
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<T>(sch, env...)), decltype(ex::get_completion_domain<T>(attrs, env...))>);
  static_assert(has_sched<T, Sch, Env...> == has_sched<T, A, Env...>);
  if constexpr (has_sched<T, Sch, Env...>) {
    static_assert(std::is_same_v<decltype(ex::get_completion_scheduler<T>(sch, env...)), decltype(ex::get_completion_scheduler<T>(attrs, env...))>);
    CHECK(ex::get_completion_scheduler<T>(sch, env...) == ex::get_completion_scheduler<T>(attrs, env...));
  }
}

template <class Sch>
void check_value(const Sch& sch) {
  using A = decltype(ex::get_env(ex::schedule(sch)));
  if constexpr (has_sched<ex::set_value_t, A>) // /5
    CHECK(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(ex::schedule(sch))) == sch);
  check<ex::set_value_t>(sch, ex::env<>());
  ex::run_loop other;
  check<ex::set_value_t>(sch, ex::env{ex::prop(ex::get_scheduler, other.get_scheduler())});
}

int main() {
  std::inplace_stop_source src;
  const ex::env<ex::prop<std::get_stop_token_t, std::inplace_stop_token>> stop_env{ex::prop(std::get_stop_token, src.get_token())};
  ex::run_loop loop;
  auto rl = loop.get_scheduler();
  check_value(rl);
  static_assert(std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(ex::get_env(ex::schedule(rl)), ex::env<>())), ex::default_domain>);
  check<ex::set_stopped_t>(rl, stop_env);
  check_value(ex::inline_scheduler());
  auto ps = ex::get_parallel_scheduler();
  check_value(ps);
  ex::task_scheduler ts(rl);
  check_value(ts);
  return 0;
}
