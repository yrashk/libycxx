// The completion-scheduler and completion-domain queries, and the queries built on them:
//   [exec.queries.expos]/2: TRY-QUERY(q, tag, envs...) passes the environments to q's query when
//     it accepts them, else asks without them.
//   [exec.get.compl.sched]/4-5: get_completion_scheduler<Tag>(q, envs...) follows the scheduler's
//     own set_value completion scheduler (RECURSE-QUERY) until there is none or it is the same;
//     a scheduler without the query is its own completion scheduler only when envs is not
//     empty (/5.2); otherwise the query is ill-formed (/5.3).
//   [exec.get.compl.domain]/2: get_completion_domain<Tag>(attrs, envs...) is attrs' answer (2.1);
//     for Tag void, the set_value one (2.2); else the set_value domain of the completion
//     scheduler (2.3); else default_domain for a scheduler with envs (2.4); else ill-formed (2.5).
//   [exec.get.scheduler]/2: get_scheduler(env) is the completion scheduler of env's scheduler.
//   [exec.get.domain]/2: get_domain(env) is env's answer (2.1), else the completion domain of
//     get_scheduler(env) (2.2), else default_domain (2.3).
#include <execution>
#include <type_traits>
#include "check.hpp"

namespace ex = std::execution;

struct dom_d {};

// A scheduler whose agents belong to dom_d.
struct sch_d {
  using scheduler_concept = ex::scheduler_tag;
  int id = 0;
  struct attrs {
    int id;
    sch_d query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {id}; }
    dom_d query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  };
  struct sender {
    using sender_concept = ex::sender_tag;
    int id;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>();
    }
    attrs get_env() const noexcept { return {id}; }
  };
  sender schedule() const noexcept { return {id}; }
  dom_d query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const sch_d&) const = default;
};
static_assert(ex::scheduler<sch_d>);

// A scheduler without completion queries.
struct sch_b {
  using scheduler_concept = ex::scheduler_tag;
  int id = 0;
  struct sender {
    using sender_concept = ex::sender_tag;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>();
    }
  };
  sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const sch_b&) const = default;
};
static_assert(ex::scheduler<sch_b>);

// An object whose set_value completion scheduler is an sch_b.
struct to_b {
  int id;
  sch_b query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {id + 100}; }
};

// A query, and attributes whose answer depends on the environment.
struct q_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr q_t q{};
struct env_attrs {
  template <class Env>
  sch_b query(ex::get_completion_scheduler_t<ex::set_value_t>, const Env& e) const noexcept {
    return {q(e)};
  }
  sch_b query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {-1}; }
};

template <class Tag, class... A>
concept has_compl_sched = requires(A... a) { ex::get_completion_scheduler<Tag>(a...); };
template <class Tag, class... A>
concept has_compl_domain = requires(A... a) { ex::get_completion_domain<Tag>(a...); };

int main() {
  using ex::set_error_t, ex::set_value_t, ex::set_stopped_t;
  // RECURSE-QUERY: through to_b to the sch_b it names.
  {
    auto attrs = ex::env{ex::prop(ex::get_completion_scheduler<set_value_t>, to_b{1})};
    auto s = ex::get_completion_scheduler<set_value_t>(attrs);
    static_assert(std::is_same_v<decltype(s), sch_b>);
    CHECK(s.id == 101);
    // For set_error too: the recursion asks for the set_value completion scheduler.
    auto attrs_e = ex::env{ex::prop(ex::get_completion_scheduler<set_error_t>, to_b{2})};
    auto se = ex::get_completion_scheduler<set_error_t>(attrs_e);
    static_assert(std::is_same_v<decltype(se), sch_b>);
    CHECK(se.id == 102);
    static_assert(!has_compl_sched<set_value_t, decltype(attrs_e)>);
    // get_scheduler(env) follows the same recursion.
    auto env = ex::env{ex::prop(ex::get_scheduler, to_b{3})};
    auto gs = ex::get_scheduler(env);
    static_assert(std::is_same_v<decltype(gs), sch_b>);
    CHECK(gs.id == 103);
  }
  // TRY-QUERY passes the environment when the query takes it.
  {
    auto e = ex::env{ex::prop(q, 5)};
    CHECK(ex::get_completion_scheduler<set_value_t>(env_attrs{}, e).id == 5);
    CHECK(ex::get_completion_scheduler<set_value_t>(env_attrs{}).id == -1);
  }
  // /5.2-5.3: a scheduler without the query.
  {
    CHECK(ex::get_completion_scheduler<set_value_t>(sch_b{9}, ex::env<>()).id == 9);
    CHECK(ex::get_completion_scheduler<set_stopped_t>(sch_b{8}, ex::env<>()).id == 8);
    static_assert(!has_compl_sched<set_value_t, sch_b>);
    static_assert(!has_compl_sched<set_value_t, ex::env<>>);
    static_assert(!has_compl_sched<set_value_t, ex::env<>, ex::env<>>);
  }
  // get_completion_domain
  {
    // 2.1
    auto a1 = ex::env{ex::prop(ex::get_completion_domain<set_error_t>, dom_d())};
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_error_t>(a1)), dom_d>);
    static_assert(!has_compl_domain<set_value_t, decltype(a1)>);
    // 2.2: void asks for set_value's.
    auto a2 = ex::env{ex::prop(ex::get_completion_domain<set_value_t>, dom_d())};
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<void>(a2)), dom_d>);
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<>(a2)), dom_d>);
    // 2.3: through the completion scheduler, for any tag.
    auto a3 = ex::env{ex::prop(ex::get_completion_scheduler<set_stopped_t>, sch_d{1})};
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_stopped_t>(a3)), dom_d>);
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_value_t>(sch_d{})), dom_d>);
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_value_t>(ex::get_env(ex::schedule(sch_d{})))), dom_d>);
    // 2.4: a scheduler without a domain, given an environment.
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_value_t>(sch_b{}, ex::env<>())), ex::default_domain>);
    static_assert(!has_compl_domain<set_value_t, sch_b>);
    // ... but not for attributes naming such a scheduler: 2.3 asks the scheduler itself
    // (TRY-QUERY, no default), and 2.4 needs attrs to be a scheduler.
    auto a4 = ex::env{ex::prop(ex::get_completion_scheduler<set_value_t>, sch_b{})};
    static_assert(!has_compl_domain<set_value_t, decltype(a4), ex::env<>>);
    // 2.5
    static_assert(!has_compl_domain<set_value_t, ex::env<>>);
    static_assert(!has_compl_domain<set_value_t, ex::env<>, ex::env<>>);
    static_assert(!has_compl_domain<void, ex::env<>>);
  }
  // get_domain
  {
    struct other_dom {};
    // 2.1 first.
    auto e1 = ex::env{ex::prop(ex::get_domain, other_dom()), ex::prop(ex::get_scheduler, sch_d{})};
    static_assert(std::is_same_v<decltype(ex::get_domain(e1)), other_dom>);
    // 2.2: the scheduler's completion domain.
    auto e2 = ex::env{ex::prop(ex::get_scheduler, sch_d{})};
    static_assert(std::is_same_v<decltype(ex::get_domain(e2)), dom_d>);
    // ... default_domain for a scheduler without one (2.2 through [exec.get.compl.domain]/2.4).
    auto e3 = ex::env{ex::prop(ex::get_scheduler, sch_b{})};
    static_assert(std::is_same_v<decltype(ex::get_domain(e3)), ex::default_domain>);
    // 2.3
    static_assert(std::is_same_v<decltype(ex::get_domain(ex::env<>())), ex::default_domain>);
  }
  return 0;
}
