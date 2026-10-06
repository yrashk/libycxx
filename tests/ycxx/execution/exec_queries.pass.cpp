// Queries and queryable utilities ([exec.queryable], [exec.queries], [exec.envs]):
//   [exec.fwd.env]: forwarding_query(q) is q.query(forwarding_query) if valid, else whether Q is
//     derived from forwarding_query_t. The standard queries are forwarding queries; get_env is
//     not a query.
//   [exec.get.allocator], [exec.get.stop.token]: get_stop_token(env) is never_stop_token{} when
//     env has no such query.
//   [exec.get.env]: get_env(o) is o.get_env(), or env<>{}.
//   [exec.prop]: prop(q, v).query(q) is v; prop is not assignable.
//   [exec.env]: env{e0, e1, ...} queries its elements in order; not assignable; CTAD unwraps
//     reference_wrapper.
//   [exec.get.scheduler], [exec.get.delegation.scheduler], [exec.get.start.scheduler].
//   [exec.get.compl.sched]/5.2: get_completion_scheduler<set_value_t>(sch, env) is sch for a
//     scheduler without the query.
//   [exec.get.domain]/2.3: default_domain when nothing says otherwise.
//   [exec.get.fwd.progress].
#include <execution>
#include <functional>
#include <memory>
#include <stop_token>
#include <type_traits>
#include "check.hpp"

namespace ex = std::execution;

struct my_query_t : std::forwarding_query_t {
  template <class Env>
  constexpr decltype(auto) operator()(const Env& e) const noexcept {
    return e.query(*this);
  }
};
inline constexpr my_query_t my_query{};
struct local_query_t {
  static constexpr bool query(std::forwarding_query_t) noexcept { return false; }
  template <class Env>
  constexpr decltype(auto) operator()(const Env& e) const noexcept {
    return e.query(*this);
  }
};
inline constexpr local_query_t local_query{};
struct plain_t {};

template <class E, class Q>
concept has_query = requires(const E& e, Q q) { e.query(q); };
template <class Tag, class S, class... Envs>
concept has_completion_scheduler = requires(const S& s, const Envs&... envs) { ex::get_completion_scheduler<Tag>(s, envs...); };

static_assert(std::forwarding_query(std::get_allocator));
static_assert(std::forwarding_query(std::get_stop_token));
static_assert(std::forwarding_query(ex::get_scheduler));
static_assert(std::forwarding_query(ex::get_delegation_scheduler));
static_assert(std::forwarding_query(ex::get_start_scheduler));
static_assert(std::forwarding_query(ex::get_domain));
static_assert(std::forwarding_query(ex::get_completion_scheduler<ex::set_value_t>));
static_assert(std::forwarding_query(ex::get_completion_domain<ex::set_error_t>));
static_assert(std::forwarding_query(ex::get_await_completion_adaptor));
static_assert(std::forwarding_query(my_query));
static_assert(!std::forwarding_query(local_query));
static_assert(!std::forwarding_query(plain_t{}));
static_assert(noexcept(std::forwarding_query(my_query)));

// get_env of an object without a get_env member is env<>.
struct no_env {};
static_assert(std::is_same_v<decltype(ex::get_env(no_env{})), ex::env<>>);
static_assert(std::is_same_v<ex::env_of_t<no_env>, ex::env<>>);

// get_stop_token of an environment without one is never_stop_token.
static_assert(std::is_same_v<std::stop_token_of_t<ex::env<>>, std::never_stop_token>);

// The draft's own example ([exec.env]/4), with values.
struct sched_t {
  using scheduler_concept = ex::scheduler_tag;
  int id = 0;
  ex::inline_scheduler inner{};
  auto schedule() const noexcept { return inner.schedule(); }
  bool operator==(const sched_t&) const = default;
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::concurrent;
  }
};
static_assert(ex::scheduler<sched_t>);

int main() {
  // prop
  constexpr auto p = ex::prop(my_query, 42);
  static_assert(my_query(p) == 42);
  static_assert(std::is_same_v<decltype(p.query(my_query)), const int&>);
  static_assert(!std::is_copy_assignable_v<ex::prop<my_query_t, int>>);
  static_assert(std::is_copy_constructible_v<ex::prop<my_query_t, int>>);
  int x = 7;
  auto pr = ex::prop(my_query, std::ref(x));
  static_assert(std::is_same_v<decltype(pr), ex::prop<my_query_t, int&>>);
  CHECK(&my_query(pr) == &x);

  // env: queries in order; the first element with the query answers.
  ex::env e{ex::prop(my_query, 1), ex::prop(my_query, 2), ex::prop(local_query, 3)};
  CHECK(my_query(e) == 1);
  CHECK(local_query(e) == 3);
  static_assert(!std::is_copy_assignable_v<decltype(e)>);
  static_assert(noexcept(e.query(my_query)));
  ex::env<> empty;
  (void)empty;
  static_assert(!has_query<decltype(empty), my_query_t>);
  // CTAD unwraps reference_wrapper.
  auto er = ex::env{std::ref(e)};
  static_assert(std::is_same_v<decltype(er), ex::env<decltype(e)&>>);
  CHECK(my_query(er) == 1);
  // A copy of an env.
  auto e2 = e;
  CHECK(local_query(e2) == 3);

  // get_allocator, get_stop_token through an env.
  std::allocator<int> a;
  auto ea = ex::env{ex::prop(std::get_allocator, a)};
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get_allocator(ea))>, std::allocator<int>>);
  std::inplace_stop_source src;
  auto es = ex::env{ex::prop(std::get_stop_token, src.get_token())};
  CHECK(std::get_stop_token(es) == src.get_token());
  CHECK(!std::get_stop_token(es).stop_requested());
  src.request_stop();
  CHECK(std::get_stop_token(es).stop_requested());
  static_assert(std::is_same_v<std::stop_token_of_t<decltype(es)>, std::inplace_stop_token>);

  // Schedulers in environments.
  sched_t s{3};
  auto env_s = ex::env{ex::prop(ex::get_scheduler, s), ex::prop(ex::get_delegation_scheduler, sched_t{4}),
                       ex::prop(ex::get_start_scheduler, sched_t{5})};
  CHECK(ex::get_scheduler(env_s).id == 3);
  CHECK(ex::get_delegation_scheduler(env_s).id == 4);
  CHECK(ex::get_start_scheduler(env_s).id == 5);
  CHECK(ex::get_forward_progress_guarantee(s) == ex::forward_progress_guarantee::concurrent);
  static_assert(noexcept(ex::get_forward_progress_guarantee(s)));
  // get_completion_scheduler<set_value_t>(sch, env) is sch ([exec.get.compl.sched]/5.2).
  CHECK(ex::get_completion_scheduler<ex::set_value_t>(s, ex::env<>()).id == 3);
  static_assert(!has_completion_scheduler<ex::set_value_t, sched_t>);
  static_assert(has_completion_scheduler<ex::set_value_t, sched_t, ex::env<>>);
  static_assert(!has_completion_scheduler<int, sched_t, ex::env<>>);

  // get_domain: default_domain without a domain or scheduler.
  static_assert(std::is_same_v<decltype(ex::get_domain(ex::env<>())), ex::default_domain>);
  struct my_domain {};
  auto ed = ex::env{ex::prop(ex::get_domain, my_domain{})};
  static_assert(std::is_same_v<decltype(ex::get_domain(ed)), my_domain>);
  // For a scheduler without a domain, the domain of its environment is default_domain.
  static_assert(std::is_same_v<decltype(ex::get_domain(env_s)), ex::default_domain>);

  // forward_progress_guarantee enumerators.
  static_assert(ex::forward_progress_guarantee::concurrent != ex::forward_progress_guarantee::parallel);
  static_assert(ex::forward_progress_guarantee::parallel != ex::forward_progress_guarantee::weakly_parallel);
  return 0;
}
