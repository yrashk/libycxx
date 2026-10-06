// libycxx core: the foundations of senders and receivers ([exec], <execution>): queries and
// environments ([exec.queryable], [exec.queries], [exec.envs]), receivers ([exec.recv]),
// operation states ([exec.opstate]), completion signatures ([exec.cmplsig]), the sender and
// scheduler concepts, domains, transform_sender/apply_sender, get_completion_signatures and
// connect ([exec.snd]), and the exposition-only machinery the algorithms are specified with
// (basic-sender, make-sender, product-type, FWD-ENV, JOIN-ENV, ...; [exec.snd.expos]).
//
// Completion signatures are computed as types. A library sender's member alias template
// ycxx_csigs<Self, Env...> names its completion_signatures specialization, or one of two error
// types: dependent_sigs (no environment given and the signatures depend on it) or
// invalid_sigs<What, Info...> (a type error the draft reports by throwing from
// get_completion_signatures). The public consteval get_completion_signatures<Sndr, Env...>()
// returns the specialization or throws (dependent_sender_error, or an exception derived from
// std::exception naming the problem), as [exec.getcomplsigs] says; on a compiler that cannot
// throw during constant evaluation (Clang 23) the throw only makes the call non-constant, which
// is all sender_in observes. Only senders that are not the library's are asked through the
// consteval function (DECISIONS: <execution>).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/coroutine.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/exception_ptr.hpp>
#include <ycxx/core/integer_sequence.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/move.hpp>
#include <ycxx/core/stop_token.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/variant.hpp>

// ---------------------------------------------------------------------------------------------
// Type lists and small metafunctions.
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

template <class... Ts>
struct tlist {};

template <class T, class List>
inline constexpr bool in_tlist = false;
template <class T, class... Ts>
inline constexpr bool in_tlist<T, tlist<Ts...>> = (std::is_same_v<T, Ts> || ...);

// Appends each type not yet in the list (order of first appearance kept).
template <class List, class... Ts>
struct tlist_add {
  using type = List;
};
template <class... Ls, class T, class... Ts>
struct tlist_add<tlist<Ls...>, T, Ts...>
    : tlist_add<std::conditional_t<in_tlist<T, tlist<Ls...>>, tlist<Ls...>, tlist<Ls..., T>>, Ts...> {};
template <class... Ts>
using unique_tlist = typename tlist_add<tlist<>, Ts...>::type;

template <template <class...> class F, class List>
struct tlist_apply;
template <template <class...> class F, class... Ts>
struct tlist_apply<F, tlist<Ts...>> {
  using type = F<Ts...>;
};
template <template <class...> class F, class List>
using tlist_apply_t = typename tlist_apply<F, List>::type;

// Template arguments with duplicates removed, as in "variant<...> except with duplicate types
// removed".
template <template <class...> class F, class... Ts>
using apply_unique_t = tlist_apply_t<F, unique_tlist<Ts...>>;

template <class... Lists>
struct tlist_concat {
  using type = tlist<>;
};
template <class... Ts>
struct tlist_concat<tlist<Ts...>> {
  using type = tlist<Ts...>;
};
template <class... As, class... Bs, class... Rest>
struct tlist_concat<tlist<As...>, tlist<Bs...>, Rest...> : tlist_concat<tlist<As..., Bs...>, Rest...> {};

template <bool... Bs>
inline constexpr std::size_t first_true = [] {
  constexpr bool v[] = {Bs..., true};
  std::size_t i = 0;
  while (!v[i])
    ++i;
  return i;
}();

template <class... Ts>
inline constexpr std::size_t max_size = [] {
  std::size_t m = 1;
  ((m = sizeof(Ts) > m ? sizeof(Ts) : m), ...);
  return m;
}();

// [exec.general]/6, [exec.snd.expos], [func.require] exposition-only concepts.
template <class T>
concept movable_value = std::move_constructible<std::decay_t<T>> && std::constructible_from<std::decay_t<T>, T> &&
                        (!std::is_array_v<std::remove_reference_t<T>>);
template <class From, class To>
concept decays_to = std::same_as<std::decay_t<From>, To>;
template <class T>
concept class_type = decays_to<T, T> && std::is_class_v<T>;
template <class T>
concept queryable = std::destructible<T>;
template <class F, class... As>
concept callable = requires(F&& f, As&&... as) { static_cast<F&&>(f)(static_cast<As&&>(as)...); };
template <class F, class... As>
concept nothrow_callable =
    callable<F, As...> && requires(F&& f, As&&... as) {
      { static_cast<F&&>(f)(static_cast<As&&>(as)...) } noexcept;
    };
template <class F, class... As>
using call_result_t = decltype(std::declval<F>()(std::declval<As>()...));
template <template <class...> class T, class... As>
concept valid_specialization = requires { typename T<As...>; };

template <class T>
[[gnu::always_inline]] constexpr const T& as_const_ref(const T& x) noexcept {
  return x;
}

// T, made dependent on U (defers the completeness check of a type used before its definition).
template <class T, class U>
struct dependent_type {
  using type = T;
};
template <class T, class U>
using dependent_t = typename dependent_type<T, U>::type;

template <auto V>
struct constant {
  static constexpr auto value = V;
};

// The "unspecified empty trivially copyable class type that models semiregular" of make-sender's
// default Data (also the {} data of the adaptors that have none).
struct empty_data {};

// decayed-typeof<cpo>
template <const auto& Cpo>
using decayed_typeof = std::decay_t<decltype(Cpo)>;

}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// Completion functions, start, and the concept tags ([exec.recv], [exec.opstate]).
namespace [[gnu::visibility("hidden")]] std { namespace execution {

struct scheduler_tag {};
struct receiver_tag {};
struct operation_state_tag {};
struct sender_tag {};

// [exec.set.value]
struct set_value_t {
  template <class Rcvr, class... Vs>
    requires(!is_lvalue_reference_v<Rcvr> && !is_const_v<Rcvr>) &&
            requires(Rcvr&& r, Vs&&... vs) { static_cast<Rcvr&&>(r).set_value(static_cast<Vs&&>(vs)...); }
  constexpr void operator()(Rcvr&& rcvr, Vs&&... vs) const noexcept {
    static_assert(noexcept(static_cast<Rcvr&&>(rcvr).set_value(static_cast<Vs&&>(vs)...)),
                  "set_value: the receiver's set_value must be noexcept");
    static_assert(is_void_v<decltype(static_cast<Rcvr&&>(rcvr).set_value(static_cast<Vs&&>(vs)...))>,
                  "set_value: the receiver's set_value must return void");
    static_cast<Rcvr&&>(rcvr).set_value(static_cast<Vs&&>(vs)...);
  }
};
// [exec.set.error]
struct set_error_t {
  template <class Rcvr, class E>
    requires(!is_lvalue_reference_v<Rcvr> && !is_const_v<Rcvr>) &&
            requires(Rcvr&& r, E&& e) { static_cast<Rcvr&&>(r).set_error(static_cast<E&&>(e)); }
  constexpr void operator()(Rcvr&& rcvr, E&& err) const noexcept {
    static_assert(noexcept(static_cast<Rcvr&&>(rcvr).set_error(static_cast<E&&>(err))),
                  "set_error: the receiver's set_error must be noexcept");
    static_assert(is_void_v<decltype(static_cast<Rcvr&&>(rcvr).set_error(static_cast<E&&>(err)))>,
                  "set_error: the receiver's set_error must return void");
    static_cast<Rcvr&&>(rcvr).set_error(static_cast<E&&>(err));
  }
};
// [exec.set.stopped]
struct set_stopped_t {
  template <class Rcvr>
    requires(!is_lvalue_reference_v<Rcvr> && !is_const_v<Rcvr>) &&
            requires(Rcvr&& r) { static_cast<Rcvr&&>(r).set_stopped(); }
  constexpr void operator()(Rcvr&& rcvr) const noexcept {
    static_assert(noexcept(static_cast<Rcvr&&>(rcvr).set_stopped()), "set_stopped: the receiver's set_stopped must be noexcept");
    static_assert(is_void_v<decltype(static_cast<Rcvr&&>(rcvr).set_stopped())>,
                  "set_stopped: the receiver's set_stopped must return void");
    static_cast<Rcvr&&>(rcvr).set_stopped();
  }
};
inline constexpr set_value_t set_value{};
inline constexpr set_error_t set_error{};
inline constexpr set_stopped_t set_stopped{};

// [exec.opstate.start]
struct start_t {
  template <class Op>
    requires requires(Op& op) { op.start(); }
  constexpr void operator()(Op& op) const noexcept {
    static_assert(noexcept(op.start()), "start: the operation state's start must be noexcept");
    static_assert(is_void_v<decltype(op.start())>, "start: the operation state's start must return void");
    op.start();
  }
  template <class Op>
  void operator()(Op&& op) const = delete; // [exec.opstate.start]/1: ill-formed for an rvalue
};
inline constexpr start_t start{};

template <class O>
concept operation_state = derived_from<typename O::operation_state_concept, operation_state_tag> && requires(O& o) { start(o); };

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Tag>
concept completion_tag =
    std::same_as<Tag, std::execution::set_value_t> || std::same_as<Tag, std::execution::set_error_t> ||
    std::same_as<Tag, std::execution::set_stopped_t>;
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// Queries ([exec.queries]) and queryable utilities ([exec.envs]).
namespace [[gnu::visibility("hidden")]] std {

// [exec.fwd.env]
struct forwarding_query_t {
  template <class Q>
  constexpr bool operator()(Q q) const noexcept {
    if constexpr (requires { q.query(forwarding_query_t{}); }) {
      static_assert(noexcept(q.query(forwarding_query_t{})), "forwarding_query: the query must be noexcept");
      static_assert(is_same_v<decltype(q.query(forwarding_query_t{})), bool>, "forwarding_query: the query must return bool");
      return q.query(forwarding_query_t{});
    } else {
      return derived_from<Q, forwarding_query_t>;
    }
  }
};
inline constexpr forwarding_query_t forwarding_query{};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// forwarding-query ([execution.syn]): forwarding_query(T{}) is true.
template <class Q>
concept forwarding_query_c = requires { requires std::forwarding_query_t{}(Q{}); };

template <class Env, class Q, class... As>
concept has_query = requires(const Env& env, As&&... as) { env.query(Q(), static_cast<As&&>(as)...); };

// TRY-QUERY(q, tag, args...) ([exec.queries.expos]/2).
template <class Q, class Tag, class... As>
concept try_queryable = requires(const Q& q, Tag tag, const As&... as) { q.query(tag, as...); } ||
                        requires(const Q& q, Tag tag) { q.query(tag); };
template <class Q, class Tag, class... As>
  requires try_queryable<Q, Tag, As...>
[[gnu::always_inline]] constexpr decltype(auto) try_query(const Q& q, Tag tag, const As&... as) noexcept {
  if constexpr (requires { q.query(tag, as...); })
    return q.query(tag, as...);
  else {
    ((void)as, ...);
    return q.query(tag);
  }
}
template <class Q, class Tag, class... As>
using try_query_t = decltype(::ycxx::detail::exec::try_query(std::declval<const Q&>(), Tag(), std::declval<const As&>()...));

// HIDE-SCHED(q) ([exec.queries.expos]/3): q with get_scheduler and get_domain removed.
template <class Env>
struct hide_sched_env;
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std {

// [exec.get.allocator]
struct get_allocator_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Self, class Env>
    requires requires(const Env& env, const Self& q) { env.query(q); }
  constexpr decltype(auto) operator()(this const Self&, const Env& env) noexcept {
    static_assert(noexcept(env.query(get_allocator_t{})), "get_allocator: the query must be noexcept");
    return env.query(get_allocator_t{});
  }
};
inline constexpr get_allocator_t get_allocator{};

// [exec.get.stop.token]
struct get_stop_token_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Env>
  constexpr decltype(auto) operator()(const Env& env) const noexcept {
    if constexpr (requires { env.query(get_stop_token_t{}); }) {
      static_assert(noexcept(env.query(get_stop_token_t{})), "get_stop_token: the query must be noexcept");
      static_assert(stoppable_token<remove_cvref_t<decltype(env.query(get_stop_token_t{}))>>,
                    "get_stop_token: the query must return a stoppable_token");
      return env.query(get_stop_token_t{});
    } else {
      return never_stop_token{};
    }
  }
};
inline constexpr get_stop_token_t get_stop_token{};

template <class T>
using stop_token_of_t = remove_cvref_t<decltype(get_stop_token(declval<T>()))>;

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <std::size_t I, class E>
struct exec_env_leaf {
  [[no_unique_address]] E ycxx_env;
};
template <class Is, class... Es>
struct exec_env_storage;
template <std::size_t... Is, class... Es>
struct exec_env_storage<std::index_sequence<Is...>, Es...> : exec_env_leaf<Is, Es>... {};
// A const empty member makes env and prop not assignable ([exec.prop]/4, [exec.env]/2) while
// keeping their implicit copy and move constructors (and their aggregate-ness).
struct exec_not_assignable {};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {

// [exec.prop]
template <class QueryTag, class ValueType>
struct prop {
  [[no_unique_address]] const QueryTag query_;
  ValueType value_;
  constexpr const ValueType& query(QueryTag, auto&&...) const noexcept { return value_; }
};
template <class QueryTag, class ValueType>
prop(QueryTag, ValueType) -> prop<QueryTag, unwrap_reference_t<ValueType>>;

// [exec.env]
template <ycxx::detail::exec::queryable... Envs>
struct env : ycxx::adl_free::exec_env_storage<index_sequence_for<Envs...>, Envs...> {
  // A constructor rather than aggregate initialization, which would need brace elision into the
  // storage base (warned about by -Wmissing-braces at every env{...}).
  env() = default;
  template <class... As>
    requires(sizeof...(As) == sizeof...(Envs) && sizeof...(As) != 0 && (constructible_from<Envs, As> && ...) &&
             (!is_same_v<remove_cvref_t<As>, env> && ...))
  constexpr env(As&&... as) noexcept((is_nothrow_constructible_v<Envs, As> && ...))
      : ycxx::adl_free::exec_env_storage<index_sequence_for<Envs...>, Envs...>{{static_cast<As&&>(as)}...} {}
  env(const env&) = default;
  env(env&&) = default;
  env& operator=(const env&) = delete; // [exec.env]/2

  template <class QueryTag, class... Args>
    requires(ycxx::detail::exec::has_query<Envs, QueryTag, Args...> || ...)
  constexpr decltype(auto) query(QueryTag q, Args&&... args) const
      noexcept(noexcept(this->template ycxx_first<QueryTag, Args...>().query(q, static_cast<Args&&>(args)...))) {
    return ycxx_first<QueryTag, Args...>().query(q, static_cast<Args&&>(args)...);
  }

  // The first element whose query is well-formed ([exec.env]/6).
  template <class QueryTag, class... Args>
  constexpr const auto& ycxx_first() const noexcept {
    constexpr size_t i = ycxx::detail::exec::first_true<ycxx::detail::exec::has_query<Envs, QueryTag, Args...>...>;
    return static_cast<const ycxx::adl_free::exec_env_leaf<i, Envs...[i]>&>(*this).ycxx_env;
  }
};
template <class... Envs>
env(Envs...) -> env<unwrap_reference_t<Envs>...>;

// [exec.get.env]
struct get_env_t {
  template <class T>
  constexpr decltype(auto) operator()(const T& o) const noexcept {
    if constexpr (requires { o.get_env(); }) {
      static_assert(noexcept(o.get_env()), "get_env: the get_env member must be noexcept");
      static_assert(ycxx::detail::exec::queryable<remove_cvref_t<decltype(o.get_env())>>);
      return o.get_env();
    } else {
      return env<>{};
    }
  }
};
inline constexpr get_env_t get_env{};
template <class T>
using env_of_t = decltype(get_env(declval<T>()));

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// FWD-ENV(env) ([exec.snd.expos]/4): the forwarding queries of env. E is the environment type,
// or a const lvalue reference to it when the argument was an lvalue.
template <class E>
struct exec_fwd_env {
  E ycxx_env;
  template <::ycxx::detail::exec::forwarding_query_c Q, class... As>
    requires ::ycxx::detail::exec::has_query<std::remove_cvref_t<E>, Q, As...>
  constexpr decltype(auto) query(Q q, As&&... as) const
      noexcept(noexcept(::ycxx::detail::exec::as_const_ref(ycxx_env).query(q, static_cast<As&&>(as)...))) {
    return ::ycxx::detail::exec::as_const_ref(ycxx_env).query(q, static_cast<As&&>(as)...);
  }
};
// JOIN-ENV(env1, env2) ([exec.snd.expos]/6).
template <class E1, class E2>
struct exec_join_env {
  E1 ycxx_env1;
  E2 ycxx_env2;
  template <class Q, class... As>
    requires ::ycxx::detail::exec::has_query<std::remove_cvref_t<E1>, Q, As...>
  constexpr decltype(auto) query(Q q, As&&... as) const
      noexcept(noexcept(::ycxx::detail::exec::as_const_ref(ycxx_env1).query(q, static_cast<As&&>(as)...))) {
    return ::ycxx::detail::exec::as_const_ref(ycxx_env1).query(q, static_cast<As&&>(as)...);
  }
  template <class Q, class... As>
    requires(!::ycxx::detail::exec::has_query<std::remove_cvref_t<E1>, Q, As...> &&
             ::ycxx::detail::exec::has_query<std::remove_cvref_t<E2>, Q, As...>)
  constexpr decltype(auto) query(Q q, As&&... as) const
      noexcept(noexcept(::ycxx::detail::exec::as_const_ref(ycxx_env2).query(q, static_cast<As&&>(as)...))) {
    return ::ycxx::detail::exec::as_const_ref(ycxx_env2).query(q, static_cast<As&&>(as)...);
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// An environment argument kept by reference when it is an lvalue, by value otherwise.
template <class E>
using env_member_t = std::conditional_t<std::is_lvalue_reference_v<E>, const std::remove_reference_t<E>&, std::remove_cvref_t<E>>;

template <class E>
constexpr auto fwd_env(E&& e) noexcept(std::is_nothrow_constructible_v<env_member_t<E>, E>) {
  return ::ycxx::adl_free::exec_fwd_env<env_member_t<E>>{static_cast<E&&>(e)};
}
template <class E>
using fwd_env_t = decltype(::ycxx::detail::exec::fwd_env(std::declval<E>()));

template <class E1, class E2>
constexpr auto join_env(E1&& e1, E2&& e2) noexcept(std::is_nothrow_constructible_v<env_member_t<E1>, E1> &&
                                                   std::is_nothrow_constructible_v<env_member_t<E2>, E2>) {
  return ::ycxx::adl_free::exec_join_env<env_member_t<E1>, env_member_t<E2>>{static_cast<E1&&>(e1), static_cast<E2&&>(e2)};
}
template <class E1, class E2>
using join_env_t = decltype(::ycxx::detail::exec::join_env(std::declval<E1>(), std::declval<E2>()));
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// Awaitable helpers ([exec.awaitable]).
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

template <class T>
inline constexpr bool is_coroutine_handle = false;
template <class P>
inline constexpr bool is_coroutine_handle<std::coroutine_handle<P>> = true;
template <class T>
concept await_suspend_result = std::is_void_v<T> || std::is_same_v<T, bool> || is_coroutine_handle<T>;

template <class A, class... Promise>
concept is_awaiter = requires(A& a, std::coroutine_handle<Promise...> h) {
  a.await_ready() ? 1 : 0;
  { a.await_suspend(h) } -> await_suspend_result;
  a.await_resume();
};

// GET-AWAITER(c, p): await_transform, then operator co_await (member or not), then the operand.
struct none_such_promise {};

template <class C>
[[gnu::always_inline]] constexpr decltype(auto) get_awaiter_after_transform(C&& c) noexcept {
  return static_cast<C&&>(c);
}
template <class C>
  requires requires(C&& c) { static_cast<C&&>(c).operator co_await(); }
constexpr decltype(auto) get_awaiter_after_transform(C&& c) noexcept(noexcept(static_cast<C&&>(c).operator co_await())) {
  return static_cast<C&&>(c).operator co_await();
}
template <class C>
  requires(!requires(C&& c) { static_cast<C&&>(c).operator co_await(); }) &&
          requires(C&& c) { operator co_await(static_cast<C&&>(c)); }
constexpr decltype(auto) get_awaiter_after_transform(C&& c) noexcept(noexcept(operator co_await(static_cast<C&&>(c)))) {
  return operator co_await(static_cast<C&&>(c));
}

template <class C, class Promise>
constexpr decltype(auto) get_awaiter(C&& c, Promise& p) {
  if constexpr (requires { p.await_transform(static_cast<C&&>(c)); })
    return ::ycxx::detail::exec::get_awaiter_after_transform(p.await_transform(static_cast<C&&>(c)));
  else
    return ::ycxx::detail::exec::get_awaiter_after_transform(static_cast<C&&>(c));
}
// GET-AWAITER(c): with a promise that has no await_transform.
template <class C>
constexpr decltype(auto) get_awaiter(C&& c) {
  return ::ycxx::detail::exec::get_awaiter_after_transform(static_cast<C&&>(c));
}

template <class C, class... Promise>
concept is_awaitable = requires(C (*fc)() noexcept, Promise&... p) {
  { ::ycxx::detail::exec::get_awaiter(fc(), p...) } -> is_awaiter<Promise...>;
};
template <class C>
concept is_awaitable_np = requires(C (*fc)() noexcept, none_such_promise& p) {
  { ::ycxx::detail::exec::get_awaiter(fc(), p) } -> is_awaiter<>;
};

template <class C, class... Promise>
using await_result_type =
    decltype(::ycxx::detail::exec::get_awaiter(std::declval<C>(), std::declval<Promise&>()...).await_resume());

// with-await-transform ([exec.awaitable]/5)
template <class T, class Promise>
concept has_as_awaitable = requires(T&& t, Promise& p) {
  { static_cast<T&&>(t).as_awaitable(p) } -> is_awaitable<Promise&>;
};

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Derived>
struct exec_with_await_transform {
  template <class T>
  T&& await_transform(T&& value) noexcept {
    return static_cast<T&&>(value);
  }
  template <::ycxx::detail::exec::has_as_awaitable<Derived> T>
  auto await_transform(T&& value) noexcept(noexcept(static_cast<T&&>(value).as_awaitable(std::declval<Derived&>())))
      -> decltype(static_cast<T&&>(value).as_awaitable(std::declval<Derived&>())) {
    return static_cast<T&&>(value).as_awaitable(static_cast<Derived&>(*this));
  }
};
// env-promise ([exec.awaitable]/6): used only for type computations.
template <class Env>
struct exec_env_promise : exec_with_await_transform<exec_env_promise<Env>> {
  void get_return_object() noexcept;
  std::suspend_always initial_suspend() noexcept;
  std::suspend_always final_suspend() noexcept;
  void unhandled_exception() noexcept;
  void return_void() noexcept;
  std::coroutine_handle<> unhandled_stopped() noexcept;
  const Env& get_env() const noexcept;
};
}} // namespace ycxx::adl_free

// ---------------------------------------------------------------------------------------------
// The sender concept, schedule and schedulers ([exec.snd.concepts], [exec.schedule], [exec.sched]).
namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <class Sndr>
inline constexpr bool enable_sender =
    requires { requires derived_from<typename Sndr::sender_concept, sender_tag>; } ||
    ycxx::detail::exec::is_awaitable<Sndr, ycxx::adl_free::exec_env_promise<env<>>>;

template <class Sndr>
concept sender = enable_sender<remove_cvref_t<Sndr>> && requires(const remove_cvref_t<Sndr>& sndr) {
  { get_env(sndr) } -> ycxx::detail::exec::queryable;
} && move_constructible<remove_cvref_t<Sndr>> && constructible_from<remove_cvref_t<Sndr>, Sndr>;

// [exec.schedule]
struct schedule_t {
  template <class Sch>
    requires requires(Sch&& sch) { static_cast<Sch&&>(sch).schedule(); }
  constexpr decltype(auto) operator()(Sch&& sch) const noexcept(noexcept(static_cast<Sch&&>(sch).schedule())) {
    static_assert(sender<decltype(static_cast<Sch&&>(sch).schedule())>, "schedule: the result must be a sender");
    return static_cast<Sch&&>(sch).schedule();
  }
};
inline constexpr schedule_t schedule{};

enum class forward_progress_guarantee { concurrent, parallel, weakly_parallel };

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// The scheduler concept without its get_forward_progress_guarantee requirement, which that query
// itself requires of its argument.
template <class Sch>
concept scheduler_base = std::derived_from<typename std::remove_cvref_t<Sch>::scheduler_concept, std::execution::scheduler_tag> &&
                         queryable<Sch> && requires(Sch&& sch) {
                           { std::execution::schedule(static_cast<Sch&&>(sch)) } -> std::execution::sender;
                         } && std::equality_comparable<std::remove_cvref_t<Sch>> && std::copyable<std::remove_cvref_t<Sch>>;
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

// [exec.get.fwd.progress]
struct get_forward_progress_guarantee_t {
  template <class Self, class Sch>
    requires ycxx::detail::exec::scheduler_base<Sch&> &&
             requires(const remove_cvref_t<Sch>& s, const Self& q) { s.query(q); }
  constexpr forward_progress_guarantee operator()(this const Self&, Sch&& sch) noexcept {
    const auto& s = sch;
    static_assert(noexcept(s.query(get_forward_progress_guarantee_t{})),
                  "get_forward_progress_guarantee: the query must be noexcept");
    static_assert(is_same_v<decltype(s.query(get_forward_progress_guarantee_t{})), forward_progress_guarantee>,
                  "get_forward_progress_guarantee: the query must return forward_progress_guarantee");
    return s.query(get_forward_progress_guarantee_t{});
  }
};
inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};

// [exec.sched]
template <class Sch>
concept scheduler = ycxx::detail::exec::scheduler_base<Sch> && requires(Sch&& sch) {
  { get_forward_progress_guarantee(sch) } -> same_as<forward_progress_guarantee>;
};

template <scheduler Sch>
using schedule_result_t = decltype(schedule(declval<Sch>()));

// [exec.get.compl.sched]
template <class CPO>
struct get_completion_scheduler_t;
template <class CPO = void>
struct get_completion_domain_t;
struct default_domain;
template <class... Domains>
struct indeterminate_domain;

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// RECURSE-QUERY(sch, envs...) ([exec.get.compl.sched]/4).
template <class Sch, class... Envs>
constexpr auto recurse_query(Sch sch, const Envs&... envs) noexcept {
  using gcs = std::execution::get_completion_scheduler_t<dependent_t<std::execution::set_value_t, Sch>>;
  if constexpr (try_queryable<Sch, gcs, Envs...>) {
    auto sch2 = ::ycxx::detail::exec::try_query(sch, gcs{}, envs...);
    if constexpr (std::is_same_v<decltype(sch2), Sch>) {
      while (!(sch2 == sch)) {
        sch = sch2;
        if constexpr (std::is_same_v<decltype(::ycxx::detail::exec::try_query(sch, gcs{}, envs...)), Sch>)
          sch2 = ::ycxx::detail::exec::try_query(sch, gcs{}, envs...);
      }
      return sch;
    } else {
      return ::ycxx::detail::exec::recurse_query(sch2, envs...);
    }
  } else {
    return sch;
  }
}
template <class Tag, class Q, class... Envs>
concept completion_scheduler_via_query =
    try_queryable<Q, std::execution::get_completion_scheduler_t<Tag>, Envs...> &&
    requires(const Q& q, const Envs&... envs) {
      ::ycxx::detail::exec::recurse_query(
          ::ycxx::detail::exec::try_query(q, std::execution::get_completion_scheduler_t<Tag>{}, envs...), envs...);
    };
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <class CPO>
struct get_completion_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Q, class... Envs>
    requires ycxx::detail::exec::completion_tag<CPO> &&
             (ycxx::detail::exec::completion_scheduler_via_query<CPO, Q, Envs...> ||
              (sizeof...(Envs) != 0 && scheduler<const Q&>))
  constexpr auto operator()(const Q& q, const Envs&... envs) const noexcept {
    if constexpr (ycxx::detail::exec::completion_scheduler_via_query<CPO, Q, Envs...>) {
      auto s = ycxx::detail::exec::recurse_query(ycxx::detail::exec::try_query(q, *this, envs...), envs...);
      static_assert(scheduler<decltype(s)>, "get_completion_scheduler: the result must be a scheduler");
      return s;
    } else {
      ((void)envs, ...);
      return q;
    }
  }
};
template <class CPO>
constexpr get_completion_scheduler_t<CPO> get_completion_scheduler{};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// The D of get_completion_domain<Tag>(attrs, envs...) ([exec.get.compl.domain]/2); void when
// that expression is ill-formed.
template <class Tag, class A, class... Envs>
struct compl_domain {
  static auto pick() {
    using std::execution::get_completion_domain_t;
    using std::execution::get_completion_scheduler_t;
    if constexpr (try_queryable<A, get_completion_domain_t<Tag>, Envs...>)
      return std::type_identity<std::remove_cvref_t<try_query_t<A, get_completion_domain_t<Tag>, Envs...>>>{};
    else if constexpr (std::is_void_v<Tag>)
      return std::type_identity<typename compl_domain<std::execution::set_value_t, A, Envs...>::type>{};
    else if constexpr (requires(const A& a, const Envs&... e) {
                         ::ycxx::detail::exec::try_query(get_completion_scheduler_t<Tag>{}(a, e...),
                                                         get_completion_domain_t<dependent_t<std::execution::set_value_t, A>>{}, e...);
                       })
      return std::type_identity<std::remove_cvref_t<decltype(::ycxx::detail::exec::try_query(
          get_completion_scheduler_t<Tag>{}(std::declval<const A&>(), std::declval<const Envs&>()...),
          get_completion_domain_t<dependent_t<std::execution::set_value_t, A>>{}, std::declval<const Envs&>()...))>>{};
    else if constexpr (std::execution::scheduler<const A&> && sizeof...(Envs) != 0)
      return std::type_identity<std::execution::default_domain>{};
    else
      return std::type_identity<void>{};
  }
  using type = typename decltype(pick())::type;
};
template <class Tag, class A, class... Envs>
using compl_domain_t = typename compl_domain<Tag, A, Envs...>::type;
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

// [exec.get.compl.domain]
template <class CPO>
struct get_completion_domain_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class A, class... Envs>
    requires(is_void_v<CPO> || ycxx::detail::exec::completion_tag<CPO>) &&
            (!is_void_v<ycxx::detail::exec::compl_domain_t<CPO, A, Envs...>>)
  constexpr auto operator()(const A&, const Envs&...) const noexcept {
    using D = ycxx::detail::exec::compl_domain_t<CPO, A, Envs...>;
    static_assert(noexcept(D()), "get_completion_domain: constructing the domain must not throw");
    return D();
  }
};
template <class CPO = void>
constexpr get_completion_domain_t<CPO> get_completion_domain{};

// [exec.get.scheduler]
struct get_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Self, class Env>
    requires requires(const Env& env, const Self& q, const ycxx::detail::exec::hide_sched_env<Env>& h) {
      get_completion_scheduler_t<set_value_t>{}(env.query(q), h);
    }
  constexpr auto operator()(this const Self&, const Env& env) noexcept {
    static_assert(noexcept(env.query(get_scheduler_t{})), "get_scheduler: the query must be noexcept");
    auto s = get_completion_scheduler_t<set_value_t>{}(env.query(get_scheduler_t{}), ycxx::detail::exec::hide_sched_env<Env>{env});
    static_assert(scheduler<decltype(s)>, "get_scheduler: the result must be a scheduler");
    return s;
  }
};
inline constexpr get_scheduler_t get_scheduler{};

// [exec.get.domain]
struct get_domain_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Env>
  static constexpr auto ycxx_pick() noexcept {
    if constexpr (requires(const Env& env) { auto(env.query(get_domain_t{})); })
      return type_identity<decltype(auto(declval<const Env&>().query(get_domain_t{})))>{};
    else if constexpr (requires(const Env& env, const ycxx::detail::exec::hide_sched_env<Env>& h) {
                         get_completion_domain_t<set_value_t>{}(get_scheduler_t{}(env), h);
                       })
      return type_identity<decltype(get_completion_domain_t<set_value_t>{}(
          get_scheduler_t{}(declval<const Env&>()), declval<const ycxx::detail::exec::hide_sched_env<Env>&>()))>{};
    else
      return type_identity<default_domain>{};
  }
  template <class Env>
  constexpr auto operator()(const Env&) const noexcept {
    using D = typename decltype(ycxx_pick<Env>())::type;
    static_assert(noexcept(D()), "get_domain: constructing the domain must not throw");
    return D();
  }
};
inline constexpr get_domain_t get_domain{};

// [exec.get.start.scheduler]
struct get_start_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Self, class Env>
    requires requires(const Env& env, const Self& q) { env.query(q); }
  constexpr decltype(auto) operator()(this const Self&, const Env& env) noexcept {
    static_assert(noexcept(env.query(get_start_scheduler_t{})), "get_start_scheduler: the query must be noexcept");
    static_assert(scheduler<decltype(env.query(get_start_scheduler_t{}))>, "get_start_scheduler: the result must be a scheduler");
    return env.query(get_start_scheduler_t{});
  }
};
inline constexpr get_start_scheduler_t get_start_scheduler{};

// [exec.get.delegation.scheduler]
struct get_delegation_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Self, class Env>
    requires requires(const Env& env, const Self& q) { env.query(q); }
  constexpr decltype(auto) operator()(this const Self&, const Env& env) noexcept {
    static_assert(noexcept(env.query(get_delegation_scheduler_t{})), "get_delegation_scheduler: the query must be noexcept");
    static_assert(scheduler<decltype(env.query(get_delegation_scheduler_t{}))>,
                  "get_delegation_scheduler: the result must be a scheduler");
    return env.query(get_delegation_scheduler_t{});
  }
};
inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};

// [exec.get.await.adapt]
struct get_await_completion_adaptor_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class Self, class Env>
    requires requires(const Env& env, const Self& q) { env.query(q); }
  constexpr decltype(auto) operator()(this const Self&, const Env& env) noexcept {
    static_assert(noexcept(env.query(get_await_completion_adaptor_t{})), "get_await_completion_adaptor: the query must be noexcept");
    return env.query(get_await_completion_adaptor_t{});
  }
};
inline constexpr get_await_completion_adaptor_t get_await_completion_adaptor{};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Env>
struct hide_sched_env {
  const Env& env;
  template <class Q, class... As>
    requires(!std::is_same_v<Q, std::execution::get_scheduler_t> && !std::is_same_v<Q, std::execution::get_domain_t>) &&
            has_query<Env, Q, As...>
  constexpr decltype(auto) query(Q q, As&&... as) const noexcept(noexcept(env.query(q, static_cast<As&&>(as)...))) {
    return env.query(q, static_cast<As&&>(as)...);
  }
};

// inline-attrs<Tag> ([exec.snd.expos]/59): the attributes of a sender that completes with Tag on
// the agent that starts it.
template <class Tag>
struct inline_attrs {
  template <class Env>
    requires requires(const Env& env) { std::execution::get_scheduler(env); }
  constexpr auto query(std::execution::get_completion_scheduler_t<Tag>, const Env& env) const noexcept {
    return std::execution::get_scheduler(env);
  }
  template <class Env>
  constexpr auto query(std::execution::get_completion_domain_t<Tag>, const Env& env) const noexcept {
    return std::execution::get_domain(env);
  }
};

// COMMON-DOMAIN(domains...) ([exec.snd.expos]/8), as a type.
template <class... Ds>
struct common_domain {
  static auto pick() {
    if constexpr (requires { typename std::common_type_t<Ds...>; requires (sizeof...(Ds) != 0); })
      return std::type_identity<std::common_type_t<Ds...>>{};
    else
      return std::type_identity<apply_unique_t<std::execution::indeterminate_domain, Ds...>>{};
  }
  using type = typename decltype(pick())::type;
};
template <class... Ds>
using common_domain_t = typename common_domain<Ds...>::type;

// COMPL-DOMAIN(Tag, sndr, envs) ([exec.snd.expos]/9), as a type.
template <class Tag, class Sndr, class... Envs>
using compl_domain_of_t = std::conditional_t<
    !std::is_void_v<compl_domain_t<Tag, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<Sndr>()))>, Envs...>> ||
        sizeof...(Envs) == 0,
    compl_domain_t<Tag, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<Sndr>()))>, Envs...>,
    std::execution::indeterminate_domain<>>;
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// Completion signatures ([exec.cmplsig]).
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Fn>
inline constexpr bool is_completion_signature = false;
template <class... Vs>
inline constexpr bool is_completion_signature<std::execution::set_value_t(Vs...)> =
    ((std::is_object_v<Vs> || std::is_reference_v<Vs>) && ...);
template <class E>
inline constexpr bool is_completion_signature<std::execution::set_error_t(E)> = std::is_object_v<E> || std::is_reference_v<E>;
template <>
inline constexpr bool is_completion_signature<std::execution::set_stopped_t()> = true;
template <class Fn>
concept completion_signature = is_completion_signature<Fn>;

template <class Sig>
struct sig_tag;
template <class Tag, class... As>
struct sig_tag<Tag(As...)> {
  using type = Tag;
  using args = tlist<As...>;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <ycxx::detail::exec::completion_signature... Fns>
struct completion_signatures {
  // count-of(tag) and for-each(fn) ([exec.cmplsig]/8), exposition-only in the draft.
  template <class Tag>
  static constexpr size_t ycxx_count_of(Tag) {
    return (size_t{0} + ... + size_t{is_same_v<typename ycxx::detail::exec::sig_tag<Fns>::type, decay_t<Tag>>});
  }
  template <class Fn>
  static constexpr void ycxx_for_each(Fn&& fn) {
    (fn(static_cast<Fns*>(nullptr)), ...);
  }
};

struct dependent_sender_error : exception {
  constexpr const char* what() const noexcept override { return "std::execution::dependent_sender_error"; }
};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

template <class T>
inline constexpr bool is_csigs = false;
template <class... Fns>
inline constexpr bool is_csigs<std::execution::completion_signatures<Fns...>> = true;
template <class T>
concept valid_completion_signatures = is_csigs<T>;

// The error results of a completion-signature computation.
struct dependent_sigs {};
template <class What, class... Info>
struct invalid_sigs {};
template <class T>
inline constexpr bool is_invalid_sigs = false;
template <class What, class... Info>
inline constexpr bool is_invalid_sigs<invalid_sigs<What, Info...>> = true;

// Problems reported by invalid_sigs (the names show in diagnostics).
struct sender_has_no_completion_signatures;
struct not_a_sender_for_this_environment;
struct function_not_invocable_with_these_arguments;
struct result_datums_not_decay_copyable;
struct let_function_must_return_a_sender;
struct when_all_child_has_more_than_one_value_completion;
struct sender_has_not_exactly_one_value_completion;
struct environment_has_no_start_scheduler;
struct start_scheduler_is_not_infallible;
struct read_env_query_ill_formed_or_void;
struct transform_sender_ill_formed;

// Sets of completion signatures with duplicates removed; an error operand gives the error
// (the first invalid one, else dependent).
template <class... CS>
struct sigs_concat;
template <>
struct sigs_concat<> {
  using type = std::execution::completion_signatures<>;
};
template <class... Fns>
struct sigs_concat<std::execution::completion_signatures<Fns...>> {
  using type = apply_unique_t<std::execution::completion_signatures, Fns...>;
};
template <class... As, class... Bs, class... Rest>
struct sigs_concat<std::execution::completion_signatures<As...>, std::execution::completion_signatures<Bs...>, Rest...>
    : sigs_concat<std::execution::completion_signatures<As..., Bs...>, Rest...> {};
template <class First, class... Rest>
  requires(!is_csigs<First>)
struct sigs_concat<First, Rest...> {
  static auto pick() {
    if constexpr (is_invalid_sigs<First>)
      return std::type_identity<First>{};
    else if constexpr (requires { typename sigs_concat<Rest...>::type; requires is_invalid_sigs<typename sigs_concat<Rest...>::type>; })
      return std::type_identity<typename sigs_concat<Rest...>::type>{};
    else
      return std::type_identity<First>{};
  }
  using type = typename decltype(pick())::type;
};
template <class CS, class... Rest>
  requires is_csigs<CS> && (sizeof...(Rest) != 0) && (!is_csigs<Rest> || ...)
struct sigs_concat<CS, Rest...> {
  static auto pick() {
    using R = typename sigs_concat<Rest...>::type;
    if constexpr (is_csigs<R>)
      return std::type_identity<typename sigs_concat<CS, R>::type>{};
    else
      return std::type_identity<R>{};
  }
  using type = typename decltype(pick())::type;
};
template <class... CS>
using sigs_concat_t = typename sigs_concat<CS...>::type;

// Maps each signature of CS through F<Sig> (a completion_signatures or an error) and joins the
// results; an error CS is the result.
template <class CS, template <class> class F>
struct sigs_map {
  using type = CS;
};
template <class... Sigs, template <class> class F>
struct sigs_map<std::execution::completion_signatures<Sigs...>, F> {
  using type = sigs_concat_t<F<Sigs>...>;
};
template <class CS, template <class> class F>
using sigs_map_t = typename sigs_map<CS, F>::type;

// The signatures with the given tag, as a tlist of the argument tlists.
template <class Tag, class CS>
struct sigs_args;
template <class Tag, class... Sigs>
struct sigs_args<Tag, std::execution::completion_signatures<Sigs...>> {
  using type = typename tlist_concat<
      std::conditional_t<std::is_same_v<typename sig_tag<Sigs>::type, Tag>, tlist<typename sig_tag<Sigs>::args>, tlist<>>...>::type;
};
template <class Tag, class CS>
using sigs_args_t = typename sigs_args<Tag, CS>::type;

template <class Tag, class CS>
inline constexpr std::size_t sigs_count = 0;
template <class Tag, class... Sigs>
inline constexpr std::size_t sigs_count<Tag, std::execution::completion_signatures<Sigs...>> =
    (std::size_t{0} + ... + std::size_t{std::is_same_v<typename sig_tag<Sigs>::type, Tag>});

// META-APPLY ([exec.cmplsig]/6).
template <bool>
struct indirect_meta_apply {
  template <template <class...> class T, class... As>
  using meta_apply = T<As...>;
};
template <class...>
concept always_true = true;

// Alias templates throughout, so that a Tuple or Variant of the wrong arity is a substitution
// failure (a nested class template would make it a hard error).
template <class Args>
struct gather_tuple;
template <class... As>
struct gather_tuple<tlist<As...>> {
  template <template <class...> class Tuple>
  using with = typename indirect_meta_apply<always_true<As...>>::template meta_apply<Tuple, As...>;
};
template <class ArgLists>
struct gather_variant;
template <class... ArgLists>
struct gather_variant<tlist<ArgLists...>> {
  template <template <class...> class Tuple, template <class...> class Variant>
  using with = typename indirect_meta_apply<always_true<ArgLists...>>::template meta_apply<
      Variant, typename gather_tuple<ArgLists>::template with<Tuple>...>;
};
// gather-signatures<Tag, Completions, Tuple, Variant>
template <class Tag, valid_completion_signatures Completions, template <class...> class Tuple, template <class...> class Variant>
using gather_signatures = typename gather_variant<sigs_args_t<Tag, Completions>>::template with<Tuple, Variant>;

template <class... Ts>
using decayed_tuple = std::tuple<std::decay_t<Ts>...>;

struct empty_variant {
  empty_variant() = delete;
};
template <class... Ts>
struct variant_or_empty_impl {
  using type = apply_unique_t<std::variant, std::decay_t<Ts>...>;
};
template <>
struct variant_or_empty_impl<> {
  using type = empty_variant;
};
template <class... Ts>
using variant_or_empty = typename variant_or_empty_impl<Ts...>::type;

template <class... Ts>
struct type_list_tl {};

// MATCHING-SIG(F1, F2) ([exec.general]/7)
template <class F1, class F2>
inline constexpr bool matching_sig = false;
template <class R1, class... A1, class R2, class... A2>
inline constexpr bool matching_sig<R1(A1...), R2(A2...)> = std::is_same_v<R1(A1 && ...), R2(A2 && ...)>;

// SET-VALUE-SIG(T)
template <class T>
struct set_value_sig {
  using type = std::execution::set_value_t(T);
};
template <class T>
  requires std::is_void_v<T>
struct set_value_sig<T> {
  using type = std::execution::set_value_t();
};
template <class T>
using set_value_sig_t = typename set_value_sig<T>::type;

}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// Domains, transform_sender, apply_sender ([exec.domain.indeterminate], [exec.domain.default],
// [exec.snd.transform], [exec.snd.apply]); tag_of_t ([exec.snd.concepts]/6).
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// tag_of_t: the type of the first element of a tuple-like sender with at least two elements
// (the library's senders). Aggregates of other forms are not recognised (DECISIONS).
template <class Sndr>
concept has_sender_tag = requires(Sndr&& s) {
  requires std::tuple_size<std::remove_cvref_t<Sndr>>::value >= 2;
  static_cast<Sndr&&>(s).template get<0>();
};
template <class Sndr>
struct tag_of {};
template <class Sndr>
  requires has_sender_tag<Sndr>
struct tag_of<Sndr> {
  using type = std::decay_t<decltype(std::declval<Sndr>().template get<0>())>;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <sender Sndr>
using tag_of_t = typename ycxx::detail::exec::tag_of<Sndr>::type;

// [exec.domain.default]
struct default_domain {
  template <class Tag, sender Sndr, ycxx::detail::exec::queryable Env>
  static constexpr decltype(auto) transform_sender(Tag, Sndr&& sndr, const Env& env) noexcept(
      noexcept(ycxx_transform(Tag(), static_cast<Sndr&&>(sndr), env))) {
    return ycxx_transform(Tag(), static_cast<Sndr&&>(sndr), env);
  }
  template <class Tag, sender Sndr, class... Args>
    requires requires(Sndr&& sndr, Args&&... args) { Tag().apply_sender(static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...); }
  static constexpr decltype(auto) apply_sender(Tag, Sndr&& sndr, Args&&... args) noexcept(
      noexcept(Tag().apply_sender(static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...))) {
    return Tag().apply_sender(static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...);
  }

private:
  template <class Tag, class Sndr, class Env>
  static constexpr decltype(auto) ycxx_transform(Tag, Sndr&& sndr, const Env& env) noexcept(
      noexcept(tag_of_t<Sndr>().transform_sender(Tag(), static_cast<Sndr&&>(sndr), env)))
    requires requires { tag_of_t<Sndr>().transform_sender(Tag(), static_cast<Sndr&&>(sndr), env); }
  {
    return tag_of_t<Sndr>().transform_sender(Tag(), static_cast<Sndr&&>(sndr), env);
  }
  template <class Tag, class Sndr, class Env>
  static constexpr Sndr ycxx_transform(Tag, Sndr&& sndr, const Env&) noexcept {
    return static_cast<Sndr>(static_cast<Sndr&&>(sndr));
  }
};

// [exec.domain.indeterminate]
template <class... Domains>
struct indeterminate_domain {
  indeterminate_domain() = default;
  constexpr indeterminate_domain(auto&&) noexcept {}
  template <class Tag, sender Sndr, ycxx::detail::exec::queryable Env>
  static constexpr decltype(auto) transform_sender(Tag, Sndr&& sndr, const Env& env) noexcept(
      noexcept(default_domain().transform_sender(Tag(), static_cast<Sndr&&>(sndr), env))) {
    using R = decay_t<decltype(default_domain().transform_sender(Tag(), static_cast<Sndr&&>(sndr), env))>;
    static_assert((ycxx_agrees<Domains, R, Tag, Sndr, Env> && ...),
                  "indeterminate_domain: the possible domains transform the sender differently");
    return default_domain().transform_sender(Tag(), static_cast<Sndr&&>(sndr), env);
  }

private:
  template <class D, class R, class Tag, class Sndr, class Env>
  static constexpr bool ycxx_agrees = [] {
    if constexpr (requires { D().transform_sender(Tag(), declval<Sndr>(), declval<const Env&>()); })
      return is_same_v<decay_t<decltype(D().transform_sender(Tag(), declval<Sndr>(), declval<const Env&>()))>, R>;
    else
      return true;
  }();
};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] std {
// [exec.domain.indeterminate]/4
template <class... Ds, class... Es>
struct common_type<execution::indeterminate_domain<Ds...>, execution::indeterminate_domain<Es...>> {
  using type = ycxx::detail::exec::apply_unique_t<execution::indeterminate_domain, Ds..., Es...>;
};
template <class... Ds, class D>
struct common_type<execution::indeterminate_domain<Ds...>, D> {
  using type = conditional_t<sizeof...(Ds) == 0, D, ycxx::detail::exec::apply_unique_t<execution::indeterminate_domain, Ds..., D>>;
};
template <class D, class... Ds>
struct common_type<D, execution::indeterminate_domain<Ds...>> {
  using type = conditional_t<sizeof...(Ds) == 0, D, ycxx::detail::exec::apply_unique_t<execution::indeterminate_domain, Ds..., D>>;
};
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// transformed-sndr(dom, tag, s) and transform-recurse ([exec.snd.transform]/3).
template <class Dom, class Tag, class Sndr, class Env>
constexpr decltype(auto) transformed_sndr(Dom dom, Tag tag, Sndr&& s, const Env& env) noexcept(
    noexcept(std::execution::default_domain().transform_sender(tag, static_cast<Sndr&&>(s), env))) {
  if constexpr (requires { dom.transform_sender(tag, static_cast<Sndr&&>(s), env); })
    return dom.transform_sender(tag, static_cast<Sndr&&>(s), env);
  else
    return std::execution::default_domain().transform_sender(tag, static_cast<Sndr&&>(s), env);
}

template <class S, class Env>
using completion_domain_for = std::conditional_t<
    std::is_void_v<compl_domain_t<void, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<S>()))>, Env>>,
    std::execution::default_domain,
    compl_domain_t<void, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<S>()))>, Env>>;
template <class Env>
using start_domain_for = decltype(std::execution::get_domain(std::declval<const Env&>()));

template <class Dom, class Tag, class Sndr, class Env>
constexpr decltype(auto) transform_recurse(Dom dom, Tag tag, Sndr&& s, const Env& env) {
  using S2 = decltype(::ycxx::detail::exec::transformed_sndr(dom, tag, static_cast<Sndr&&>(s), env));
  if constexpr (std::is_same_v<std::remove_cvref_t<S2>, std::remove_cvref_t<Sndr>>) {
    return ::ycxx::detail::exec::transformed_sndr(dom, tag, static_cast<Sndr&&>(s), env);
  } else {
    using Dom2 = std::conditional_t<std::is_same_v<Tag, std::execution::start_t>, start_domain_for<Env>, completion_domain_for<S2, Env>>;
    return ::ycxx::detail::exec::transform_recurse(Dom2(), tag, ::ycxx::detail::exec::transformed_sndr(dom, tag, static_cast<Sndr&&>(s), env), env);
  }
}
// Whether a transformation may throw: the noexcept of every step.
template <class Dom, class Tag, class Sndr, class Env>
consteval bool transform_recurse_nothrow() {
  using S2 = decltype(::ycxx::detail::exec::transformed_sndr(Dom(), Tag(), std::declval<Sndr>(), std::declval<const Env&>()));
  constexpr bool here = noexcept(::ycxx::detail::exec::transformed_sndr(Dom(), Tag(), std::declval<Sndr>(), std::declval<const Env&>()));
  if constexpr (std::is_same_v<std::remove_cvref_t<S2>, std::remove_cvref_t<Sndr>>) {
    return here;
  } else {
    using Dom2 = std::conditional_t<std::is_same_v<Tag, std::execution::start_t>, start_domain_for<Env>, completion_domain_for<S2, Env>>;
    return here && ::ycxx::detail::exec::transform_recurse_nothrow<Dom2, Tag, S2, Env>();
  }
}
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

// [exec.snd.transform]
template <sender Sndr, ycxx::detail::exec::queryable Env>
constexpr decltype(auto) transform_sender(Sndr&& sndr, const Env& env) noexcept(
    ycxx::detail::exec::transform_recurse_nothrow<ycxx::detail::exec::completion_domain_for<Sndr, Env>, set_value_t, Sndr, Env>() &&
    ycxx::detail::exec::transform_recurse_nothrow<
        ycxx::detail::exec::start_domain_for<Env>, start_t,
        decltype(ycxx::detail::exec::transform_recurse(ycxx::detail::exec::completion_domain_for<Sndr, Env>(), set_value_t(),
                                                       declval<Sndr>(), declval<const Env&>())),
        Env>()) {
  using namespace ycxx::detail::exec;
  return transform_recurse(start_domain_for<Env>(), start_t(),
                           transform_recurse(completion_domain_for<Sndr, Env>(), set_value_t(), static_cast<Sndr&&>(sndr), env), env);
}

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Domain, class Tag, class Sndr, class... Args>
consteval bool apply_nothrow() {
  if constexpr (requires { std::declval<Domain&>().apply_sender(Tag(), std::declval<Sndr>(), std::declval<Args>()...); })
    return noexcept(std::declval<Domain&>().apply_sender(Tag(), std::declval<Sndr>(), std::declval<Args>()...));
  else
    return noexcept(std::execution::default_domain().apply_sender(Tag(), std::declval<Sndr>(), std::declval<Args>()...));
}
template <class Domain, class Tag, class Sndr, class... Args>
constexpr decltype(auto) apply_dispatch(Domain dom, Tag, Sndr&& sndr, Args&&... args) noexcept(apply_nothrow<Domain, Tag, Sndr, Args...>()) {
  if constexpr (requires { dom.apply_sender(Tag(), static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...); })
    return dom.apply_sender(Tag(), static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...);
  else
    return std::execution::default_domain().apply_sender(Tag(), static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...);
}
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
template <class Domain, class Tag, sender Sndr, class... Args>
  requires requires(Domain dom, Sndr&& sndr, Args&&... args) {
    dom.apply_sender(Tag(), static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...);
  } || requires(Sndr&& sndr, Args&&... args) {
    default_domain().apply_sender(Tag(), static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...);
  }
constexpr decltype(auto) apply_sender(Domain dom, Tag, Sndr&& sndr, Args&&... args) noexcept(
    ycxx::detail::exec::apply_nothrow<Domain, Tag, Sndr, Args...>()) {
  return ycxx::detail::exec::apply_dispatch(dom, Tag(), static_cast<Sndr&&>(sndr), static_cast<Args&&>(args)...);
}
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// get_completion_signatures ([exec.getcomplsigs]) and the concepts built on it.
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

// The exception get_completion_signatures throws for an invalid sender (the "unspecified-
// exception" of [exec.snd.general]/6 and the "except" of [exec.getcomplsigs]/1).
template <class What, class... Info>
struct completion_signatures_error : std::exception {
  constexpr const char* what() const noexcept override { return "std::execution: invalid sender for the environment"; }
};
template <class... Info>
struct completion_signatures_error<dependent_sigs, Info...> : std::execution::dependent_sender_error {};

void sender_type_error_without_exceptions() noexcept; // never defined: makes the call non-constant

template <class What, class... Info>
[[noreturn]] consteval void report_sigs_error() {
  if constexpr (cfg::exceptions)
    throw completion_signatures_error<What, Info...>();
  else
    ::ycxx::detail::exec::sender_type_error_without_exceptions();
}

// CHECKED-COMPLSIGS: the completion_signatures of a computation, or the throw for its error.
template <class R>
consteval auto checked_sigs() {
  if constexpr (is_csigs<R>) {
    return R();
  } else if constexpr (std::is_same_v<R, dependent_sigs>) {
    ::ycxx::detail::exec::report_sigs_error<dependent_sigs>();
    return std::execution::completion_signatures<>();
  } else {
    []<class W, class... I>(invalid_sigs<W, I...>*) { ::ycxx::detail::exec::report_sigs_error<W, I...>(); }(static_cast<R*>(nullptr));
    return std::execution::completion_signatures<>();
  }
}

// The answer of a sender that is not the library's: its static member function
// get_completion_signatures<Sndr, Env...>() when that is a constant expression; an error
// otherwise (dependent if it throws dependent_sender_error; where a constant evaluation cannot
// catch, dependent when no environment was given).
template <class S, class... Env>
concept has_member_complsigs = requires { std::remove_reference_t<S>::template get_completion_signatures<S, Env...>(); };
template <class S, class... Env>
concept constant_member_complsigs =
    has_member_complsigs<S, Env...> &&
    requires { typename constant<std::remove_reference_t<S>::template get_completion_signatures<S, Env...>()>; };

template <class S, class... Env>
consteval int classify_member_complsigs_error() {
  if constexpr (cfg::constexpr_exceptions) {
    try {
      (void)std::remove_reference_t<S>::template get_completion_signatures<S, Env...>();
      return 0;
    } catch (std::execution::dependent_sender_error&) {
      return 1;
    } catch (...) {
      return 2;
    }
  } else {
    return 2;
  }
}
template <class S, class... Env>
struct member_complsigs {
  static auto pick() {
    if constexpr (constant_member_complsigs<S, Env...>) {
      using R = std::remove_cvref_t<decltype(std::remove_reference_t<S>::template get_completion_signatures<S, Env...>())>;
      if constexpr (is_csigs<R>)
        return std::type_identity<R>{};
      else
        return std::type_identity<invalid_sigs<sender_has_no_completion_signatures, S, Env...>>{};
    } else if constexpr (cfg::constexpr_exceptions &&
                         requires { typename constant<::ycxx::detail::exec::classify_member_complsigs_error<S, Env...>()>; }) {
      if constexpr (::ycxx::detail::exec::classify_member_complsigs_error<S, Env...>() == 1)
        return std::type_identity<dependent_sigs>{};
      else
        return std::type_identity<invalid_sigs<sender_has_no_completion_signatures, S, Env...>>{};
    } else if constexpr (sizeof...(Env) == 0) {
      return std::type_identity<dependent_sigs>{};
    } else {
      return std::type_identity<invalid_sigs<sender_has_no_completion_signatures, S, Env...>>{};
    }
  }
  using type = typename decltype(pick())::type;
};

template <class S, class... Env>
concept library_sender = requires { typename std::remove_reference_t<S>::template ycxx_csigs<S, Env...>; };

// The type NewSndr of [exec.getcomplsigs]/1.
template <class Sndr, class... Env>
struct new_sndr {
  using type = Sndr;
};
template <class Sndr, class Env>
struct new_sndr<Sndr, Env> {
  using type = decltype(std::execution::transform_sender(std::declval<Sndr>(), std::declval<Env>()));
};

template <class Sndr, class... Env>
struct csigs_of_impl {
  static auto pick() {
    if constexpr (sizeof...(Env) > 1) {
      return std::type_identity<invalid_sigs<not_a_sender_for_this_environment, Sndr, Env...>>{};
    } else if constexpr (!requires { typename new_sndr<Sndr, Env...>::type; }) {
      return std::type_identity<invalid_sigs<transform_sender_ill_formed, Sndr, Env...>>{};
    } else {
      using NS = typename new_sndr<Sndr, Env...>::type;
      using X = std::remove_reference_t<NS>;
      if constexpr (library_sender<NS, Env...>)
        return std::type_identity<typename X::template ycxx_csigs<NS, Env...>>{};
      else if constexpr (has_member_complsigs<NS, Env...>)
        return std::type_identity<typename member_complsigs<NS, Env...>::type>{};
      else if constexpr (has_member_complsigs<NS>)
        return std::type_identity<typename member_complsigs<NS>::type>{};
      else if constexpr (requires { typename X::completion_signatures; requires is_csigs<typename X::completion_signatures>; })
        // The form of [exec.cmplsig]'s example (a member type), which [exec.getcomplsigs] does
        // not list (DECISIONS: <execution>).
        return std::type_identity<typename X::completion_signatures>{};
      else if constexpr (is_awaitable<NS, ::ycxx::adl_free::exec_env_promise<Env>...>)
        return std::type_identity<std::execution::completion_signatures<
            set_value_sig_t<await_result_type<NS, ::ycxx::adl_free::exec_env_promise<Env>...>>,
            std::execution::set_error_t(std::exception_ptr), std::execution::set_stopped_t()>>{};
      else if constexpr (sizeof...(Env) == 0)
        return std::type_identity<dependent_sigs>{};
      else
        return std::type_identity<invalid_sigs<not_a_sender_for_this_environment, Sndr, Env...>>{};
    }
  }
  using type = typename decltype(pick())::type;
};
// The completion signatures of Sndr in Env... (or an error type).
template <class Sndr, class... Env>
using csigs_of_t = typename csigs_of_impl<Sndr, Env...>::type;

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <class Sndr, class... Env>
  requires(sizeof...(Env) <= 1)
consteval auto get_completion_signatures() -> ycxx::detail::exec::valid_completion_signatures auto {
  return ycxx::detail::exec::checked_sigs<ycxx::detail::exec::csigs_of_t<Sndr, Env...>>();
}

template <class Sndr, class... Env>
concept sender_in = sender<Sndr> && (sizeof...(Env) <= 1) && (ycxx::detail::exec::queryable<Env> && ...) &&
                    ycxx::detail::exec::is_csigs<ycxx::detail::exec::csigs_of_t<Sndr, Env...>>;

template <class Sndr>
concept dependent_sender = sender<Sndr> && is_same_v<ycxx::detail::exec::csigs_of_t<Sndr>, ycxx::detail::exec::dependent_sigs>;

template <class Sndr, class... Env>
  requires sender_in<Sndr, Env...>
using completion_signatures_of_t = ycxx::detail::exec::csigs_of_t<Sndr, Env...>;

template <class Sndr, class Env = env<>, template <class...> class Tuple = ycxx::detail::exec::decayed_tuple,
          template <class...> class Variant = ycxx::detail::exec::variant_or_empty>
  requires sender_in<Sndr, Env>
using value_types_of_t = ycxx::detail::exec::gather_signatures<set_value_t, completion_signatures_of_t<Sndr, Env>, Tuple, Variant>;

template <class Sndr, class Env = env<>, template <class...> class Variant = ycxx::detail::exec::variant_or_empty>
  requires sender_in<Sndr, Env>
using error_types_of_t = ycxx::detail::exec::gather_signatures<set_error_t, completion_signatures_of_t<Sndr, Env>, type_identity_t, Variant>;

template <class Sndr, class Env = env<>>
  requires sender_in<Sndr, Env>
constexpr bool sends_stopped = ycxx::detail::exec::sigs_count<set_stopped_t, completion_signatures_of_t<Sndr, Env>> != 0;

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// single-sender-value-type<Sndr, Env...> ([execution.syn]/2): from the value completions' argument
// lists, decay_t<T> for one completion with one datum, void for none or one without datums,
// decayed-tuple<Ts...> for one with several, and nothing otherwise.
template <class ArgLists>
struct single_value_of {};
template <>
struct single_value_of<tlist<>> {
  using type = void;
};
template <>
struct single_value_of<tlist<tlist<>>> {
  using type = void;
};
template <class T>
struct single_value_of<tlist<tlist<T>>> {
  using type = std::decay_t<T>;
};
template <class T0, class T1, class... Ts>
struct single_value_of<tlist<tlist<T0, T1, Ts...>>> {
  using type = decayed_tuple<T0, T1, Ts...>;
};
template <class Sndr, class... Env>
struct single_sender_value : single_value_of<sigs_args_t<std::execution::set_value_t, std::execution::completion_signatures_of_t<Sndr, Env...>>> {};
template <class Sndr, class... Env>
  requires std::execution::sender_in<Sndr, Env...>
using single_sender_value_type = typename single_sender_value<Sndr, Env...>::type;
// single-sender-value-type, or void where it is ill-formed.
template <class Sndr, class... Env>
struct single_value_or_void {
  using type = void;
};
template <class Sndr, class... Env>
  requires requires { typename single_sender_value<Sndr, Env...>::type; }
struct single_value_or_void<Sndr, Env...> {
  using type = typename single_sender_value<Sndr, Env...>::type;
};
template <class Sndr, class... Env>
using single_sender_value_or_void = typename single_value_or_void<Sndr, Env...>::type;
template <class Sndr, class... Env>
concept single_sender = std::execution::sender_in<Sndr, Env...> && requires { typename single_sender_value_type<Sndr, Env...>; };

// sender-in-of / sender-of ([exec.snd.concepts]/5)
template <class... As>
using value_signature = std::execution::set_value_t(As...);
template <class Sndr, class SetValue, class... Env>
concept sender_in_of_impl =
    std::execution::sender_in<Sndr, Env...> &&
    matching_sig<SetValue, gather_signatures<std::execution::set_value_t, std::execution::completion_signatures_of_t<Sndr, Env...>,
                                             value_signature, std::type_identity_t>>;
template <class Sndr, class Env, class... Values>
concept sender_in_of = sender_in_of_impl<Sndr, std::execution::set_value_t(Values...), Env>;
template <class Sndr, class... Values>
concept sender_of = sender_in_of_impl<Sndr, std::execution::set_value_t(Values...)>;
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// Receiver concepts ([exec.recv.concepts]).
namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <class Rcvr>
concept receiver = derived_from<typename remove_cvref_t<Rcvr>::receiver_concept, receiver_tag> &&
                   requires(const remove_cvref_t<Rcvr>& rcvr) {
                     { get_env(rcvr) } -> ycxx::detail::exec::queryable;
                   } && move_constructible<remove_cvref_t<Rcvr>> && constructible_from<remove_cvref_t<Rcvr>, Rcvr> &&
                   is_nothrow_move_constructible_v<remove_cvref_t<Rcvr>>;

template <class Rcvr, class ChildOp>
concept inlinable_receiver = receiver<Rcvr> && requires(ChildOp* child) {
  { remove_cvref_t<Rcvr>::make_receiver_for(child) } noexcept -> same_as<remove_cvref_t<Rcvr>>;
};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Rcvr, class Sig>
inline constexpr bool valid_completion_for = false;
template <class Rcvr, class Tag, class... As>
inline constexpr bool valid_completion_for<Rcvr, Tag(As...)> = callable<Tag, std::remove_cvref_t<Rcvr>, As...>;
template <class Rcvr, class CS>
inline constexpr bool has_completions = false;
template <class Rcvr, class... Sigs>
inline constexpr bool has_completions<Rcvr, std::execution::completion_signatures<Sigs...>> = (valid_completion_for<Rcvr, Sigs> && ...);
template <class Rcvr, class Completions>
concept receiver_of = std::execution::receiver<Rcvr> && has_completions<Rcvr, Completions>;
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// connect ([exec.connect]).
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class DS, class DR>
struct exec_connect_awaitable_promise;

template <class DS, class DR>
struct exec_operation_state_task {
  using operation_state_concept = std::execution::operation_state_tag;
  using promise_type = exec_connect_awaitable_promise<DS, DR>;
  explicit exec_operation_state_task(std::coroutine_handle<> h) noexcept : coro(h) {}
  exec_operation_state_task(exec_operation_state_task&&) = delete;
  ~exec_operation_state_task() { coro.destroy(); }
  void start() & noexcept { coro.resume(); }

private:
  std::coroutine_handle<> coro;
};

template <class DS, class DR>
struct exec_connect_awaitable_promise : exec_with_await_transform<exec_connect_awaitable_promise<DS, DR>> {
  exec_connect_awaitable_promise(DS&, DR& r) noexcept : rcvr(r) {}
  std::suspend_always initial_suspend() noexcept { return {}; }
  [[noreturn]] std::suspend_always final_suspend() noexcept { __builtin_trap(); }
  [[noreturn]] void unhandled_exception() noexcept { __builtin_trap(); }
  [[noreturn]] void return_void() noexcept { __builtin_trap(); }
  std::coroutine_handle<> unhandled_stopped() noexcept {
    std::execution::set_stopped(static_cast<DR&&>(rcvr));
    return std::noop_coroutine();
  }
  exec_operation_state_task<DS, DR> get_return_object() noexcept {
    return exec_operation_state_task<DS, DR>{std::coroutine_handle<exec_connect_awaitable_promise>::from_promise(*this)};
  }
  std::execution::env_of_t<DR> get_env() const noexcept { return std::execution::get_env(rcvr); }

private:
  DR& rcvr;
};

// suspend-complete(fun, as...)
template <class Fn>
struct exec_suspend_complete {
  Fn fn;
  static constexpr bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) noexcept { fn(); }
  [[noreturn]] void await_resume() noexcept { __builtin_unreachable(); }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Fun, class... Ts>
auto suspend_complete(Fun fun, Ts&&... as) noexcept {
  auto fn = [&, fun]() noexcept { fun(static_cast<Ts&&>(as)...); };
  return ::ycxx::adl_free::exec_suspend_complete<decltype(fn)>{fn};
}

template <class DS, class DR>
using connect_awaitable_value = await_result_type<DS, ::ycxx::adl_free::exec_connect_awaitable_promise<DS, DR>>;

template <class DS, class DR>
using connect_awaitable_sigs =
    std::execution::completion_signatures<set_value_sig_t<connect_awaitable_value<DS, DR>>,
                                          std::execution::set_error_t(std::exception_ptr), std::execution::set_stopped_t()>;

// connect-awaitable(sndr, rcvr) ([exec.connect]/5). Its frame is allocated (only awaitables that
// are not senders of their own come here).
template <class DS, class DR>
  requires(receiver_of<DR, connect_awaitable_sigs<DS, DR>>)
::ycxx::adl_free::exec_operation_state_task<DS, DR> connect_awaitable(DS sndr, DR rcvr) {
  std::exception_ptr ep;
  if constexpr (cfg::exceptions) {
    try {
      if constexpr (std::is_void_v<connect_awaitable_value<DS, DR>>) {
        co_await static_cast<DS&&>(sndr);
        co_await ::ycxx::detail::exec::suspend_complete(std::execution::set_value, static_cast<DR&&>(rcvr));
      } else {
        co_await ::ycxx::detail::exec::suspend_complete(std::execution::set_value, static_cast<DR&&>(rcvr),
                                                         co_await static_cast<DS&&>(sndr));
      }
    } catch (...) {
      ep = std::current_exception();
    }
  } else {
    if constexpr (std::is_void_v<connect_awaitable_value<DS, DR>>) {
      co_await static_cast<DS&&>(sndr);
      co_await ::ycxx::detail::exec::suspend_complete(std::execution::set_value, static_cast<DR&&>(rcvr));
    } else {
      co_await ::ycxx::detail::exec::suspend_complete(std::execution::set_value, static_cast<DR&&>(rcvr),
                                                       co_await static_cast<DS&&>(sndr));
    }
  }
  co_await ::ycxx::detail::exec::suspend_complete(std::execution::set_error, static_cast<DR&&>(rcvr), static_cast<std::exception_ptr&&>(ep));
}

template <class Sndr, class Rcvr>
using connect_new_sndr_t =
    decltype(std::execution::transform_sender(std::declval<Sndr>(), std::execution::get_env(std::declval<const std::remove_cvref_t<Rcvr>&>())));

template <class Sndr, class Rcvr>
concept connect_via_member = requires(Sndr&& s, Rcvr&& r) {
  std::execution::transform_sender(static_cast<Sndr&&>(s), std::execution::get_env(r));
  std::execution::transform_sender(static_cast<Sndr&&>(s), std::execution::get_env(r)).connect(static_cast<Rcvr&&>(r));
};
template <class Sndr, class Rcvr>
concept connect_via_awaitable = requires(Sndr&& s, Rcvr&& r) {
  std::execution::transform_sender(static_cast<Sndr&&>(s), std::execution::get_env(r));
  ::ycxx::detail::exec::connect_awaitable<std::decay_t<connect_new_sndr_t<Sndr, Rcvr>>, std::decay_t<Rcvr>>(
      std::execution::transform_sender(static_cast<Sndr&&>(s), std::execution::get_env(r)), static_cast<Rcvr&&>(r));
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

struct connect_t {
  template <class Sndr, class Rcvr>
    requires ycxx::detail::exec::connect_via_member<Sndr, Rcvr> || ycxx::detail::exec::connect_via_awaitable<Sndr, Rcvr>
  constexpr auto operator()(Sndr&& sndr, Rcvr&& rcvr) const noexcept(ycxx_nothrow<Sndr, Rcvr>()) {
    static_assert(sender_in<Sndr, env_of_t<Rcvr>>, "connect: the sender has no completion signatures in the receiver's environment");
    static_assert(ycxx::detail::exec::receiver_of<Rcvr, ycxx::detail::exec::csigs_of_t<Sndr, env_of_t<Rcvr>>>,
                  "connect: the receiver cannot accept every completion of the sender");
    if constexpr (ycxx::detail::exec::connect_via_member<Sndr, Rcvr>) {
      using R = decltype(transform_sender(static_cast<Sndr&&>(sndr), get_env(rcvr)).connect(static_cast<Rcvr&&>(rcvr)));
      static_assert(operation_state<R>, "connect: the result must be an operation state");
      return transform_sender(static_cast<Sndr&&>(sndr), get_env(rcvr)).connect(static_cast<Rcvr&&>(rcvr));
    } else {
      using DS = decay_t<ycxx::detail::exec::connect_new_sndr_t<Sndr, Rcvr>>;
      return ycxx::detail::exec::connect_awaitable<DS, decay_t<Rcvr>>(transform_sender(static_cast<Sndr&&>(sndr), get_env(rcvr)),
                                                                     static_cast<Rcvr&&>(rcvr));
    }
  }

private:
  template <class Sndr, class Rcvr>
  static consteval bool ycxx_nothrow() {
    if constexpr (ycxx::detail::exec::connect_via_member<Sndr, Rcvr>)
      return noexcept(transform_sender(declval<Sndr>(), get_env(declval<Rcvr&>())).connect(declval<Rcvr>()));
    else
      return false;
  }
};
inline constexpr connect_t connect{};

template <class Sndr, class Rcvr>
using connect_result_t = decltype(connect(declval<Sndr>(), declval<Rcvr>()));

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// sender-to ([exec.snd.concepts])
template <class Sndr, class Rcvr>
concept sender_to = std::execution::sender_in<Sndr, std::execution::env_of_t<Rcvr>> &&
                    receiver_of<Rcvr, std::execution::completion_signatures_of_t<Sndr, std::execution::env_of_t<Rcvr>>> &&
                    requires(Sndr&& sndr, Rcvr&& rcvr) { std::execution::connect(static_cast<Sndr&&>(sndr), static_cast<Rcvr&&>(rcvr)); };
}}} // namespace ycxx::detail::exec
