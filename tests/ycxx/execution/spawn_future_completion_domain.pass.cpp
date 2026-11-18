// The completion domains of the sender spawn_future returns ([exec.spawn.future]).
//   [exec.snd.general]/3: for a completion tag T, get_completion_domain<T>(get_env(sndr), env)
//     is COMMON-DOMAIN<DsT...> of the domains of the agents that can evaluate the operation's
//     T-completions ([exec.get.compl.domain]/1-2: the query object).
//   [exec.spawn.future]/10.2: when the spawned sender completes after the future operation was
//     started, the receiver is completed from complete(), on the agent where the spawned sender
//     completed; /11.3, /8: when the result is already there (or the association failed and the
//     state holds set_stopped), consume completes the receiver inline, on the agent that started
//     the future operation, whose domain is get_domain(env) for the receiver's environment env.
//     /5.2, /6: when decay-copying a datum can throw, the spawned sender's value completion can
//     become set_error(exception_ptr), on the spawned sender's agent.
//   So get_completion_domain<T>(get_env(future), env) is COMMON-DOMAIN of the spawned sender's
//     T-completion domain(s) and get_domain(env) (for an env without a domain, default_domain).
//   [exec.snd.expos]/8: COMMON-DOMAIN is common_type_t of the domains when that is well-formed
//     (a domain derived from default_domain and default_domain: default_domain), else
//     indeterminate_domain<Ds...> with the domains deduplicated ([exec.domain.indeterminate];
//     the order of Ds is not specified, so both orders are accepted).
//   Without an environment the starting agent is not known, so the query is ill-formed
//     ([exec.get.compl.domain]/2.5); checked through a concept.
//   In every case the future still delivers the spawned sender's value (sync_wait).
#include <execution>
#include <concepts>
#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

struct D : ex::default_domain {};  // a user domain derived from default_domain
struct D2 {};                      // a user domain unrelated to default_domain

template <class Dom>
struct dsched;

// A sender that completes inline with set_value(Vs...) on an agent of dsched<Dom>'s resource,
// whose domain is Dom.
template <class Dom, class... Vs>
struct dsender {
  using sender_concept = ex::sender_tag;
  std::tuple<Vs...> vals;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(Vs...)>();
  }
  struct attrs {
    dsched<Dom> query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
    Dom query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  };
  attrs get_env() const noexcept { return {}; }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    std::tuple<Vs...> vals;
    void start() & noexcept {
      std::apply([this](Vs&... v) { ex::set_value(std::move(r), std::move(v)...); }, vals);
    }
  };
  template <class R>
  op<R> connect(R r) && {
    return {std::move(r), std::move(vals)};
  }
  template <class R>
  op<R> connect(R r) const& {
    return {std::move(r), vals};
  }
};

template <class Dom>
struct dsched {
  using scheduler_concept = ex::scheduler_tag;
  dsender<Dom> schedule() const noexcept { return {}; }
  Dom query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const dsched&) const noexcept = default;
};
static_assert(ex::scheduler<dsched<D>> && ex::scheduler<dsched<D2>>);

// A value whose decay-copy can throw (it never does here).
struct MayThrow {
  int v = 0;
  MayThrow() = default;
  explicit MayThrow(int i) : v(i) {}
  MayThrow(const MayThrow& o) noexcept(false) : v(o.v) {}
  MayThrow(MayThrow&& o) noexcept(false) : v(o.v) {}
  MayThrow& operator=(const MayThrow&) = default;
};

template <class S, class Tag, class... Env>
concept has_domain = requires(const S& s, const Env&... e) { ex::get_completion_domain<Tag>(ex::get_env(s), e...); };

template <class S, class Tag, class... Env>
using domain_of = decltype(ex::get_completion_domain<Tag>(ex::get_env(std::declval<const S&>()), std::declval<const Env&>()...));

template <class T, class A, class B>
inline constexpr bool is_indeterminate_of =
    std::is_same_v<T, ex::indeterminate_domain<A, B>> || std::is_same_v<T, ex::indeterminate_domain<B, A>>;

using env_D = ex::prop<ex::get_domain_t, D>;
using env_D2 = ex::prop<ex::get_domain_t, D2>;

using Token = decltype(std::declval<ex::counting_scope&>().get_token());
template <class S>
using future_of = decltype(ex::spawn_future(std::declval<S>(), std::declval<Token>()));

// A spawned just(...): default_domain for set_value and set_stopped.
using FJ = future_of<decltype(ex::just(1))>;
static_assert(std::is_same_v<domain_of<FJ, ex::set_value_t, ex::env<>>, ex::default_domain>);
static_assert(std::is_same_v<domain_of<FJ, ex::set_stopped_t, ex::env<>>, ex::default_domain>);
static_assert(!has_domain<FJ, ex::set_value_t> && !has_domain<FJ, ex::set_stopped_t>);

// Spawned on D (derived from default_domain).
using FD = future_of<dsender<D, int>>;
static_assert(std::is_same_v<domain_of<FD, ex::set_value_t, env_D>, D>);
static_assert(std::is_same_v<domain_of<FD, ex::set_value_t, ex::env<>>, ex::default_domain>);  // common_type
static_assert(!has_domain<FD, ex::set_value_t>);

// Spawned on D2 (not derived from default_domain).
using FD2 = future_of<dsender<D2, int>>;
static_assert(is_indeterminate_of<domain_of<FD2, ex::set_value_t, ex::env<>>, D2, ex::default_domain>);
static_assert(std::is_same_v<domain_of<FD2, ex::set_value_t, env_D2>, D2>);
static_assert(!has_domain<FD2, ex::set_value_t>);
// The indeterminate domain collects both: common_type with either one adds nothing.
static_assert(is_indeterminate_of<std::common_type_t<domain_of<FD2, ex::set_value_t, ex::env<>>, D2>, D2, ex::default_domain>);
static_assert(is_indeterminate_of<std::common_type_t<domain_of<FD2, ex::set_value_t, ex::env<>>, ex::default_domain>, D2,
                                  ex::default_domain>);

// A datum whose decay-copy can throw (/5.2, /6): set_error also happens on the spawned sender's
// agent, or inline on the starting agent.
using FT = future_of<dsender<D2, MayThrow>>;
static_assert(is_indeterminate_of<domain_of<FT, ex::set_value_t, ex::env<>>, D2, ex::default_domain>);
static_assert(is_indeterminate_of<domain_of<FT, ex::set_error_t, ex::env<>>, D2, ex::default_domain>);
static_assert(std::is_same_v<domain_of<FT, ex::set_error_t, env_D2>, D2>);
static_assert(std::is_same_v<domain_of<FT, ex::set_value_t, env_D2>, D2>);
static_assert(!has_domain<FT, ex::set_error_t>);

int main() {
  ex::counting_scope scope;
  {
    auto f = ex::spawn_future(ex::just(1), scope.get_token());
    CHECK((std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(ex::get_env(f), ex::env<>())), ex::default_domain>));
    auto r = tt::sync_wait(std::move(f));
    CHECK(r && std::get<0>(*r) == 1);
  }
  {
    auto f = ex::spawn_future(dsender<D, int>{{2}}, scope.get_token());
    CHECK((std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(ex::get_env(f), env_D(ex::get_domain, D()))), D>));
    auto r = tt::sync_wait(std::move(f));
    CHECK(r && std::get<0>(*r) == 2);
  }
  {
    auto f = ex::spawn_future(dsender<D2, int>{{3}}, scope.get_token());
    CHECK((std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(ex::get_env(f), env_D2(ex::get_domain, D2()))), D2>));
    auto r = tt::sync_wait(std::move(f));
    CHECK(r && std::get<0>(*r) == 3);
  }
  {
    auto r = tt::sync_wait(ex::spawn_future(ex::schedule(dsched<D2>{}) | ex::then([] { return 4; }), scope.get_token()));
    CHECK(r && std::get<0>(*r) == 4);
  }
  {
    auto r = tt::sync_wait(ex::spawn_future(dsender<D2, MayThrow>{std::tuple<MayThrow>(MayThrow(5))}, scope.get_token()));
    CHECK(r && std::get<0>(*r).v == 5);
  }
  tt::sync_wait(scope.join());
  return 0;
}
