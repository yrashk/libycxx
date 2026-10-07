// The completion attributes of when_all, when_all_with_variant and let_value/let_error/
// let_stopped:
//   [exec.snd.general]/3: get_completion_domain<T>(get_env(sndr), env) is the COMMON-DOMAIN of
//     the domains of the agents that can evaluate the T completions, and ill-formed when there
//     are none; /4: get_completion_scheduler<T> only when all of them belong to one scheduler;
//   [exec.snd.expos]/8-9: COMMON-DOMAIN, and COMPL-DOMAIN (indeterminate_domain<> for a child
//     that reports no domain, given an environment); [exec.domain.indeterminate]/4: the common
//     type of indeterminate_domain<> and D is D;
//   [exec.when.all]/15-17: the operation completes in complete() of the last child to arrive:
//     with values only when every child completed with a value; with an error when a child
//     failed (its error, or an exception from decay-copying a value, TRY-EMPLACE-VALUE), on the
//     agent of whichever child arrives last; with stopped likewise (a later error turns the
//     disposition into error); [exec.when.all]/1, /19: when_all_with_variant is
//     when_all(into_variant(sndrs)...);
//   [exec.let]/2, /9-10, /16: let-cpo(sndr, f) completes with sndr's other completions, with the
//     exception of decay-copying the datums, calling f or connecting (on sndr's set-cpo agent),
//     and with the completions of the sender f returns, connected to receiver2 whose environment
//     is JOIN-ENV(let-env(sndr, env), FWD-ENV(env)) (let-env: SCHED-ENV of sndr's set-cpo
//     completion scheduler, else MAKE-ENV(get_domain, its completion domain));
//   [exec.adapt.general]/3.2-3.3: one child: its forwarding queries; several: none;
//   [exec.get.compl.domain]/3, [exec.get.compl.sched]/6: no answer for a tag the sender does not
//     complete with.
#include <execution>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
using ex::set_error_t;
using ex::set_stopped_t;
using ex::set_value_t;

template <int N>
struct dom {};

enum : unsigned { V = 1, E = 2, S = 4, ThrowingCopy = 8 };

struct throwing_copy {
  throwing_copy() = default;
  throwing_copy(const throwing_copy&) noexcept(false) {}
};

// A sender whose completions all run in domain dom<N> (attributes per tag, for the tags it has),
// with an optional completion scheduler for its value completions.
template <int N, unsigned Sigs = V, class Sch = void>
struct dsnd {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    using VS = std::conditional_t<(Sigs & ThrowingCopy) != 0, set_value_t(const throwing_copy&), set_value_t(int)>;
    if constexpr ((Sigs & E) && (Sigs & S))
      return ex::completion_signatures<VS, set_error_t(int), set_stopped_t()>();
    else if constexpr ((Sigs & E) != 0)
      return ex::completion_signatures<VS, set_error_t(int)>();
    else if constexpr ((Sigs & S) != 0)
      return ex::completion_signatures<VS, set_stopped_t()>();
    else
      return ex::completion_signatures<VS>();
  }
  struct attrs {
    Sch* sch = nullptr;
    template <class T, class... Env>
    dom<N> query(ex::get_completion_domain_t<T>, const Env&...) const noexcept {
      return {};
    }
    template <class... Env>
      requires(!std::is_void_v<Sch>)
    auto query(ex::get_completion_scheduler_t<set_value_t>, const Env&...) const noexcept {
      return *sch;
    }
  };
  Sch* sch = nullptr;
  attrs get_env() const noexcept { return {sch}; }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept {
      if constexpr ((Sigs & ThrowingCopy) != 0)
        ex::set_value(std::move(r), throwing_copy());
      else
        ex::set_value(std::move(r), N);
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

// A sender without attributes.
struct plain {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<set_value_t(int)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), 0); }
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

struct fwd_q_t : std::forwarding_query_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
template <class A>
concept answers_fwd = requires(const A& a) { fwd_q_t()(a); };
struct fwd_attr_sender : dsnd<1> {
  struct attrs : dsnd<1>::attrs {
    using dsnd<1>::attrs::query;
    int query(fwd_q_t) const noexcept { return 5; }
  };
  attrs get_env() const noexcept { return {}; }
};

int main() {
  // --- when_all
  {
    using W = decltype(ex::when_all(dsnd<1>(), dsnd<2>()));
    static_assert(domain_is<W, set_value_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    static_assert(domain_is<W, set_value_t, ex::indeterminate_domain<dom<1>, dom<2>>>); // non-dependent children
    static_assert(!has_domain<set_error_t, W, E0> && !has_domain<set_stopped_t, W, E0>);
    using W1 = decltype(ex::when_all(dsnd<1>(), dsnd<1, V | E>()));
    static_assert(domain_is<W1, set_value_t, dom<1>, E0>);
    static_assert(domain_is<W1, set_error_t, dom<1>, E0>);
    static_assert(!has_domain<set_stopped_t, W1, E0>);
    // An error of one child, the last arrival another's value: the error completion can run on
    // either child's agents.
    using W2 = decltype(ex::when_all(dsnd<1>(), dsnd<2, V | E>()));
    static_assert(domain_is<W2, set_error_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    // A stop of one child, the last arrival another's value.
    using W3 = decltype(ex::when_all(dsnd<1>(), dsnd<2, V | S>()));
    static_assert(domain_is<W3, set_stopped_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    static_assert(!has_domain<set_error_t, W3, E0>);
    // The only child that can fail is the one that fails: its own agents.
    using W4 = decltype(ex::when_all(dsnd<1, V | E>()));
    static_assert(domain_is<W4, set_error_t, dom<1>, E0>);
    // Decay-copying the values can throw: an error on the value agent.
    using W5 = decltype(ex::when_all(dsnd<3, V | ThrowingCopy>(), dsnd<4>()));
    static_assert(domain_is<W5, set_error_t, ex::indeterminate_domain<dom<3>, dom<4>>, E0>);
    using W6 = decltype(ex::when_all(dsnd<3, V | ThrowingCopy>()));
    static_assert(domain_is<W6, set_error_t, dom<3>, E0>);
    // COMPL-DOMAIN: a child without a domain counts as indeterminate_domain<> given an
    // environment (the others' domain remains); without one, nothing is known.
    using W7 = decltype(ex::when_all(plain(), dsnd<5>()));
    static_assert(domain_is<W7, set_value_t, dom<5>, E0>);
    static_assert(!has_domain<set_value_t, W7>);
    using W8 = decltype(ex::when_all(plain(), plain()));
    static_assert(domain_is<W8, set_value_t, ex::indeterminate_domain<>, E0>);
  }
  // when_all's completion scheduler: one child's, when it is the only source.
  {
    ex::run_loop loop;
    auto sch = loop.get_scheduler();
    auto w = ex::when_all(ex::schedule(sch));
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(w), E0()) == sch);
    auto w2 = ex::when_all(ex::schedule(sch), ex::schedule(sch));
    static_assert(!has_sched<set_value_t, decltype(w2), E0>);
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_value_t>(ex::get_env(w2), E0())), ex::default_domain>);
    using Sch = decltype(sch);
    dsnd<1, V, Sch> d{&sch};
    auto w3 = ex::when_all(d);
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(w3), E0()) == sch);
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(w3)) == sch);
  }
  // /3.2-3.3: one child's forwarding queries; none with two.
  {
    auto w = ex::when_all(fwd_attr_sender());
    CHECK(fwd_q_t()(ex::get_env(w)) == 5);
    auto w2 = ex::when_all(fwd_attr_sender(), fwd_attr_sender());
    static_assert(!answers_fwd<decltype(ex::get_env(w2))>);
  }
  // when_all_with_variant: as when_all(into_variant(sndrs)...).
  {
    using W = decltype(ex::when_all_with_variant(dsnd<1>(), dsnd<2, V | E>()));
    static_assert(domain_is<W, set_value_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    static_assert(domain_is<W, set_error_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    using W1 = decltype(ex::when_all_with_variant(dsnd<1, V | S>()));
    static_assert(domain_is<W1, set_value_t, dom<1>, E0>);
    static_assert(domain_is<W1, set_stopped_t, dom<1>, E0>);
    static_assert(!has_domain<set_error_t, W1, E0>);
    // into_variant's decay-copy can throw: an error completion on the value agent.
    using W2 = decltype(ex::when_all_with_variant(dsnd<3, V | ThrowingCopy>()));
    static_assert(domain_is<W2, set_error_t, dom<3>, E0>);
  }

  // --- let_value
  {
    auto l = ex::let_value(dsnd<1, V | S>(), [](int) { return dsnd<2, V | E>(); });
    using L = decltype(l);
    static_assert(domain_is<L, set_value_t, dom<2>, E0>);
    // the child's values (f can throw), the returned sender's errors
    static_assert(domain_is<L, set_error_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    static_assert(domain_is<L, set_stopped_t, dom<1>, E0>);
    // The returned sender exists only once the child completes: no scheduler, no domain
    // without an environment.
    static_assert(!has_domain<set_value_t, L>);
    static_assert(!has_sched<set_value_t, L, E0>);
    // f, the decay-copy and connect cannot throw: the errors are the returned sender's only.
    auto l2 = ex::let_value(dsnd<1>(), [](int) noexcept { return dsnd<2, V | E>(); });
    static_assert(domain_is<decltype(l2), set_error_t, dom<2>, E0>);
    auto l3 = ex::let_value(dsnd<1>(), [](int) noexcept { return dsnd<2>(); });
    static_assert(!has_domain<set_error_t, decltype(l3), E0>);
    static_assert(!has_domain<set_stopped_t, decltype(l3), E0>);
    // The returned sender completes inline: its domain is the one let-env gives it, the child's
    // set_value completion domain ([exec.let]/2.2; inline-attrs, [exec.snd.expos]/61).
    auto l4 = ex::let_value(dsnd<7>(), [](int) noexcept { return ex::just(); });
    static_assert(domain_is<decltype(l4), set_value_t, dom<7>, E0>);
  }
  // let_error and let_stopped: the child's other completions keep their agents (and scheduler).
  {
    ex::run_loop loop;
    auto sch = loop.get_scheduler();
    auto le = ex::let_error(ex::schedule(sch), [](auto) noexcept { return ex::just(); });
    CHECK(ex::get_completion_scheduler<set_value_t>(ex::get_env(le), E0()) == sch);
    auto le2 = ex::let_error(dsnd<1, V | E>(), [](int) { return dsnd<2>(); });
    static_assert(domain_is<decltype(le2), set_value_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    static_assert(domain_is<decltype(le2), set_error_t, dom<1>, E0>); // f can throw, on the error agent
    auto ls = ex::let_stopped(dsnd<1, V | S>(), []() noexcept { return dsnd<2, V | E>(); });
    static_assert(domain_is<decltype(ls), set_value_t, ex::indeterminate_domain<dom<1>, dom<2>>, E0>);
    static_assert(domain_is<decltype(ls), set_error_t, dom<2>, E0>);
    static_assert(!has_domain<set_stopped_t, decltype(ls), E0>);
    // A child without set-cpo completions: the child's attributes, also without an environment.
    auto ls2 = ex::let_stopped(dsnd<3>(), []() noexcept { return dsnd<4>(); });
    static_assert(domain_is<decltype(ls2), set_value_t, dom<3>>);
    // One child: its forwarding queries.
    auto lf = ex::let_value(fwd_attr_sender(), [](int) { return ex::just(); });
    CHECK(fwd_q_t()(ex::get_env(lf)) == 5);
  }
  // The attributes describe what happens: run the senders.
  {
    auto [a, b] = std::this_thread::sync_wait(ex::when_all(dsnd<1>(), dsnd<2>())).value();
    CHECK(a == 1 && b == 2);
    auto [c] = std::this_thread::sync_wait(ex::let_value(dsnd<1, V | S>(), [](int x) { return ex::just(x + 10); })).value();
    CHECK(c == 11);
  }
  return 0;
}
