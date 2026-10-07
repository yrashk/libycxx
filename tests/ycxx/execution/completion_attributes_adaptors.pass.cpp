// The completion attributes of the single-child adaptors, per completion tag:
//   [exec.snd.general]/3-4: the domain is the COMMON-DOMAIN of the agents that can evaluate the
//     tag's completions; a scheduler only when one scheduler's agents evaluate them all.
//     Examples 1-2: then(sndr, f) has sndr's set_value domain and scheduler; its set_error ones are
//     sndr's when f cannot throw, else they also include sndr's value agents (no scheduler);
//   [exec.then]/3 (upon_error, upon_stopped: the other channel), [exec.bulk]/7, [exec.into.variant]
//     /5, [exec.stopped.opt]/4, [exec.stopped.err]/3: the completions each adaptor turns a child
//     completion into, an exception included;
//   [exec.write.env]/1, [exec.unstoppable]/1-2: the child is connected in the written
//     environment, so it is asked there;
//   [exec.continues.on]/9-12: every completion arrives through the schedule sender's value
//     completion (also the exception of decay-copying the child's datums), plus the schedule
//     sender's own error and stopped completions ([exec.run.loop.types]/5-6: run_loop's
//     schedule sender completes with set_value and, when stop is possible, set_stopped, both
//     on the loop);
//   [exec.starts.on]/4: as its let_value form ([exec.let]/2, /9: the child's domain in the
//     environment the let-state gives it);
//   [exec.on]/6-7, [exec.affine]/5, /7: the completions run where the environment's start
//     scheduler is (through continues_on);
//   [exec.associate]/11: the wrapped sender's completions, and set_stopped on the starting agent
//     when the association fails; [exec.read.env]/3: inline (TRY-SET-VALUE);
//   [exec.get.compl.domain]/3, [exec.get.compl.sched]/6: none for a tag the sender does not
//     complete with.
#include <execution>
#include <stop_token>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
using ex::set_error_t;
using ex::set_stopped_t;
using ex::set_value_t;

template <int N>
struct dom {};

// A sender with value, error and (optionally) stopped completions, all in dom<N>, all on the
// scheduler it carries (when it carries one).
template <int N, bool Stopped = false, class Sch = void>
struct ssnd {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (Stopped)
      return ex::completion_signatures<set_value_t(int), set_error_t(int), set_stopped_t()>();
    else
      return ex::completion_signatures<set_value_t(int), set_error_t(int)>();
  }
  struct attrs {
    const Sch* sch;
    template <class T, class... Env>
    dom<N> query(ex::get_completion_domain_t<T>, const Env&...) const noexcept {
      return {};
    }
    template <class T, class... Env>
      requires(!std::is_void_v<Sch>)
    Sch query(ex::get_completion_scheduler_t<T>, const Env&...) const noexcept {
      return *sch;
    }
  };
  const Sch* sch = nullptr;
  attrs get_env() const noexcept { return {sch}; }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), N); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

template <class T, class Sndr, class... Env>
using domain_of = decltype(ex::get_completion_domain<T>(ex::get_env(std::declval<const Sndr&>()), std::declval<const Env&>()...));
template <class T, class Sndr, class... Env>
concept has_domain = requires { typename domain_of<T, Sndr, Env...>; };
template <class T, class Sndr, class... Env>
concept has_sched = requires(const Sndr& s, const Env&... e) { ex::get_completion_scheduler<T>(ex::get_env(s), e...); };
template <class Sndr, class T, class D, class... Env>
constexpr bool domain_is = std::is_same_v<domain_of<T, Sndr, Env...>, D>;

using E0 = ex::env<>;

struct throwing_q_t {
  template <class Env>
  int operator()(const Env&) const {
    return 3;
  }
};

int main() {
  ex::run_loop loop, other_loop;
  auto sch = loop.get_scheduler();
  auto other = other_loop.get_scheduler();
  using Sch = decltype(sch);
  std::inplace_stop_source stop_src;
  const ex::env<ex::prop<std::get_stop_token_t, std::inplace_stop_token>> stop_env{ex::prop(std::get_stop_token, stop_src.get_token())};
  using SE = std::remove_cvref_t<decltype(stop_env)>;
  const ssnd<1, false, Sch> child{&sch};
  using C = ssnd<1, false, Sch>;

  // then: Example 2.
  {
    auto nothrow_then = child | ex::then([](int x) noexcept { return x; });
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(nothrow_then), E0()) == sch);
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(nothrow_then), E0()) == sch);
    static_assert(domain_is<decltype(nothrow_then), set_error_t, dom<1>, E0>);
    auto throwing_then = child | ex::then([](int x) { return x; });
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(throwing_then), E0()) == sch);
    static_assert(!has_sched<set_error_t, decltype(throwing_then), E0>);
    static_assert(domain_is<decltype(throwing_then), set_error_t, dom<1>, E0>);
    static_assert(!has_domain<set_stopped_t, decltype(throwing_then), E0>);
    // Without errors in the child and a throwing f: the value agents only.
    auto t2 = ex::schedule(sch) | ex::then([] { return 1; });
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(t2), E0()) == sch);
  }
  // upon_error, upon_stopped
  {
    auto ue = child | ex::upon_error([](int) noexcept { return 0; });
    static_assert(!has_sched<set_value_t, decltype(ue), E0>); // values and errors
    static_assert(domain_is<decltype(ue), set_value_t, dom<1>, E0>);
    static_assert(!has_domain<set_error_t, decltype(ue), E0>); // f cannot throw
    auto ue2 = child | ex::upon_error([](int) { return 0; });
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(ue2), E0()) == sch);
    auto us = ssnd<2, true, Sch>{&sch} | ex::upon_stopped([]() noexcept { return 0; });
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(us), E0()) == sch);
    static_assert(!has_sched<set_value_t, decltype(us), E0>);
    static_assert(!has_domain<set_stopped_t, decltype(us), E0>);
  }
  // into_variant, stopped_as_optional, stopped_as_error, bulk
  {
    auto iv = ex::into_variant(child);
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(iv), E0()) == sch);
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(iv), E0()) == sch); // int's decay-copy cannot throw
    auto so = ex::stopped_as_optional(ssnd<2, true>());
    static_assert(domain_is<decltype(so), set_value_t, dom<2>, E0>);
    static_assert(domain_is<decltype(so), set_error_t, dom<2>, E0>);
    static_assert(!has_domain<set_stopped_t, decltype(so), E0>);
    auto se = ex::stopped_as_error(ssnd<2, true, Sch>{&sch}, 5);
    static_assert(!has_domain<set_stopped_t, decltype(se), E0>);
    static_assert(!has_sched<set_error_t, decltype(se), E0>); // errors and stopped: two sources
    static_assert(domain_is<decltype(se), set_error_t, dom<2>, E0>);
    auto b = ex::bulk(child, ex::par, 4, [](int, int) noexcept {});
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(b), E0()) == sch);
    auto b2 = ex::bulk(child, ex::par, 4, [](int, int) {});
    static_assert(!has_sched<set_error_t, decltype(b2), E0>);
    static_assert(domain_is<decltype(b2), set_error_t, dom<1>, E0>);
  }
  // unstoppable: the child is asked in the environment it is connected in.
  {
    static_assert(has_domain<set_stopped_t, decltype(ex::schedule(sch)), SE>);
    auto u = ex::unstoppable(ex::schedule(sch));
    static_assert(!has_domain<set_stopped_t, decltype(u), SE>);
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(u), stop_env) == sch);
  }
  // continues_on
  {
    auto c = ex::continues_on(child, sch);
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(c), E0()) == sch);
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(c)) == sch); // the schedule sender's ([exec.run.loop.types]/5)
    // The child's error arrives through the schedule sender's value completion.
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(c), E0()) == sch);
    static_assert(domain_is<decltype(c), set_error_t, ex::default_domain, E0>);
    static_assert(!has_domain<set_stopped_t, decltype(c), E0>);
    // Stoppable: the schedule sender's own stopped completion.
    CHECK(ex::get_completion_scheduler<set_stopped_t>(ex::get_env(c), stop_env) == sch);
    // The child's stopped completion and the schedule sender's: two sources.
    auto c2 = ex::continues_on(ssnd<2, true>(), sch);
    static_assert(!has_sched<set_stopped_t, decltype(c2), SE>);
    static_assert(domain_is<decltype(c2), set_stopped_t, ex::default_domain, SE>);
    CHECK(ex::get_completion_scheduler<set_stopped_t>(ex::get_env(c2), E0()) == sch);
  }
  // starts_on: the child's domains; the schedule sender's stopped completion.
  {
    auto s = ex::starts_on(sch, ssnd<3>());
    static_assert(domain_is<decltype(s), set_value_t, dom<3>, E0>);
    static_assert(domain_is<decltype(s), set_error_t, dom<3>, E0>);
    static_assert(!has_domain<set_stopped_t, decltype(s), E0>);
    CHECK(ex::get_completion_scheduler<set_stopped_t>(ex::get_env(s), stop_env) == sch);
    static_assert(!has_domain<set_value_t, decltype(s)>);
  }
  // on, affine: the environment's start scheduler.
  {
    const auto start_env = ex::env{ex::prop(ex::get_start_scheduler, other)};
    using SS = std::remove_cvref_t<decltype(start_env)>;
    auto o = ex::on(sch, ex::just(1));
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(o), start_env) == other);
    static_assert(domain_is<decltype(o), set_value_t, ex::default_domain, SS>);
    static_assert(!has_domain<set_value_t, decltype(o), E0>); // no start scheduler: no completions
    static_assert(!has_domain<set_value_t, decltype(o)>);
    auto o2 = ex::on(ex::schedule(other), sch, ex::then([] { return 2; }));
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(o2), E0()) == other);
    auto a = ex::affine(ssnd<4>());
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(a), start_env) == other);
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(a), start_env) == other);
    static_assert(!has_domain<set_value_t, decltype(a), E0>);
  }
  // associate: the wrapped sender's domains; a failed association stops on the starting agent.
  {
    ex::simple_counting_scope scope;
    auto as = ex::associate(ssnd<5>(), scope.get_token());
    static_assert(domain_is<decltype(as), set_value_t, dom<5>, E0>);
    static_assert(domain_is<decltype(as), set_error_t, dom<5>, E0>);
    static_assert(domain_is<decltype(as), set_stopped_t, ex::default_domain, E0>);
    static_assert(domain_is<decltype(as), set_stopped_t, dom<6>, ex::env<ex::prop<ex::get_domain_t, dom<6>>>>);
    static_assert(!has_sched<set_value_t, decltype(as), E0>);
    auto [v] = std::this_thread::sync_wait(std::move(as)).value();
    CHECK(v == 5);
    std::this_thread::sync_wait(scope.join());
  }
  // read_env: inline, its error too when the query can throw.
  {
    const auto sch_env = ex::env{ex::prop(ex::get_scheduler, other)};
    using SC = std::remove_cvref_t<decltype(sch_env)>;
    auto r = ex::read_env(ex::get_scheduler);
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(r), sch_env) == other);
    static_assert(!has_domain<set_error_t, decltype(r), SC>);
    auto r2 = ex::read_env(throwing_q_t());
    CHECK(ex::get_completion_scheduler<set_error_t>(ex::get_env(r2), sch_env) == other);
    static_assert(domain_is<decltype(r2), set_error_t, ex::default_domain, SC>);
  }
  return 0;
}
