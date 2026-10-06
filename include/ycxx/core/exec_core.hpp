// libycxx core: the foundations of senders and receivers ([exec], <execution>): queries and
// environments ([exec.queryable], [exec.queries], [exec.envs]), receivers ([exec.recv]),
// operation states ([exec.opstate]), completion signatures ([exec.cmplsig]), the sender and
// scheduler concepts, domains, transform_sender/apply_sender, get_completion_signatures and
// connect ([exec.snd]), and the exposition-only machinery the algorithms are specified with
// (basic-sender, make-sender, product-type, FWD-ENV, JOIN-ENV, ...; [exec.snd.expos]).
//
// Completion signatures are computed as types. A library sender's member alias template
// __ycxx_csigs<Self, Env...> names its completion_signatures specialization, or one of two error
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
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

template <class... _Ts>
struct __tlist {};

template <class _Tp, class _List>
inline constexpr bool __in_tlist = false;
template <class _Tp, class... _Ts>
inline constexpr bool __in_tlist<_Tp, __tlist<_Ts...>> = (std::is_same_v<_Tp, _Ts> || ...);

// Appends each type not yet in the list (order of first appearance kept).
template <class _List, class... _Ts>
struct __tlist_add {
  using type = _List;
};
template <class... _Ls, class _Tp, class... _Ts>
struct __tlist_add<__tlist<_Ls...>, _Tp, _Ts...>
    : __tlist_add<std::conditional_t<__in_tlist<_Tp, __tlist<_Ls...>>, __tlist<_Ls...>, __tlist<_Ls..., _Tp>>, _Ts...> {};
template <class... _Ts>
using __unique_tlist = typename __tlist_add<__tlist<>, _Ts...>::type;

template <template <class...> class _Fp, class _List>
struct __tlist_apply;
template <template <class...> class _Fp, class... _Ts>
struct __tlist_apply<_Fp, __tlist<_Ts...>> {
  using type = _Fp<_Ts...>;
};
template <template <class...> class _Fp, class _List>
using __tlist_apply_t = typename __tlist_apply<_Fp, _List>::type;

// Template arguments with duplicates removed, as in "variant<...> except with duplicate types
// removed".
template <template <class...> class _Fp, class... _Ts>
using __apply_unique_t = __tlist_apply_t<_Fp, __unique_tlist<_Ts...>>;

template <class... _Lists>
struct __tlist_concat {
  using type = __tlist<>;
};
template <class... _Ts>
struct __tlist_concat<__tlist<_Ts...>> {
  using type = __tlist<_Ts...>;
};
template <class... _As, class... _Bs, class... _Rest>
struct __tlist_concat<__tlist<_As...>, __tlist<_Bs...>, _Rest...> : __tlist_concat<__tlist<_As..., _Bs...>, _Rest...> {};

template <bool... _Bs>
inline constexpr std::size_t __first_true = [] {
  constexpr bool __v[] = {_Bs..., true};
  std::size_t i = 0;
  while (!__v[i])
    ++i;
  return i;
}();

template <class... _Ts>
inline constexpr std::size_t max_size = [] {
  std::size_t m = 1;
  ((m = sizeof(_Ts) > m ? sizeof(_Ts) : m), ...);
  return m;
}();

// [exec.general]/6, [exec.snd.expos], [func.require] exposition-only concepts.
template <class _Tp>
concept __movable_value = std::move_constructible<std::decay_t<_Tp>> && std::constructible_from<std::decay_t<_Tp>, _Tp> &&
                        (!std::is_array_v<std::remove_reference_t<_Tp>>);
template <class _From, class _To>
concept __decays_to = std::same_as<std::decay_t<_From>, _To>;
template <class _Tp>
concept __class_type = __decays_to<_Tp, _Tp> && std::is_class_v<_Tp>;
template <class _Tp>
concept __queryable = std::destructible<_Tp>;
template <class _Fp, class... _As>
concept __callable = requires(_Fp&& __f, _As&&... __as) { static_cast<_Fp&&>(__f)(static_cast<_As&&>(__as)...); };
template <class _Fp, class... _As>
concept __nothrow_callable =
    __callable<_Fp, _As...> && requires(_Fp&& __f, _As&&... __as) {
      { static_cast<_Fp&&>(__f)(static_cast<_As&&>(__as)...) } noexcept;
    };
template <class _Fp, class... _As>
using __call_result_t = decltype(std::declval<_Fp>()(std::declval<_As>()...));
template <template <class...> class _Tp, class... _As>
concept __valid_specialization = requires { typename _Tp<_As...>; };

template <class _Tp>
[[__gnu__::__always_inline__]] constexpr const _Tp& __as_const_ref(const _Tp& __x) noexcept {
  return __x;
}

// T, made dependent on U (defers the completeness check of a type used before its definition).
template <class _Tp, class _Up>
struct __dependent_type {
  using type = _Tp;
};
template <class _Tp, class _Up>
using __dependent_t = typename __dependent_type<_Tp, _Up>::type;

template <auto _Vp>
struct __y_constant {
  static constexpr auto value = _Vp;
};

// The "unspecified empty trivially copyable class type that models semiregular" of make-sender's
// default Data (also the {} data of the adaptors that have none).
struct __empty_data {};

// decayed-typeof<cpo>
template <const auto& _Cpo>
using __decayed_typeof = std::decay_t<decltype(_Cpo)>;

}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// Completion functions, start, and the concept tags ([exec.recv], [exec.opstate]).
namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

struct scheduler_tag {};
struct receiver_tag {};
struct operation_state_tag {};
struct sender_tag {};

// [exec.set.value]
struct set_value_t {
  template <class _Rcvr, class... _Vs>
    requires(!is_lvalue_reference_v<_Rcvr> && !is_const_v<_Rcvr>) &&
            requires(_Rcvr&& r, _Vs&&... __vs) { static_cast<_Rcvr&&>(r).set_value(static_cast<_Vs&&>(__vs)...); }
  constexpr void operator()(_Rcvr&& __rcvr, _Vs&&... __vs) const noexcept {
    static_assert(noexcept(static_cast<_Rcvr&&>(__rcvr).set_value(static_cast<_Vs&&>(__vs)...)),
                  "set_value: the receiver's set_value must be noexcept");
    static_assert(is_void_v<decltype(static_cast<_Rcvr&&>(__rcvr).set_value(static_cast<_Vs&&>(__vs)...))>,
                  "set_value: the receiver's set_value must return void");
    static_cast<_Rcvr&&>(__rcvr).set_value(static_cast<_Vs&&>(__vs)...);
  }
};
// [exec.set.error]
struct set_error_t {
  template <class _Rcvr, class _Ep>
    requires(!is_lvalue_reference_v<_Rcvr> && !is_const_v<_Rcvr>) &&
            requires(_Rcvr&& r, _Ep&& e) { static_cast<_Rcvr&&>(r).set_error(static_cast<_Ep&&>(e)); }
  constexpr void operator()(_Rcvr&& __rcvr, _Ep&& __err) const noexcept {
    static_assert(noexcept(static_cast<_Rcvr&&>(__rcvr).set_error(static_cast<_Ep&&>(__err))),
                  "set_error: the receiver's set_error must be noexcept");
    static_assert(is_void_v<decltype(static_cast<_Rcvr&&>(__rcvr).set_error(static_cast<_Ep&&>(__err)))>,
                  "set_error: the receiver's set_error must return void");
    static_cast<_Rcvr&&>(__rcvr).set_error(static_cast<_Ep&&>(__err));
  }
};
// [exec.set.stopped]
struct set_stopped_t {
  template <class _Rcvr>
    requires(!is_lvalue_reference_v<_Rcvr> && !is_const_v<_Rcvr>) &&
            requires(_Rcvr&& r) { static_cast<_Rcvr&&>(r).set_stopped(); }
  constexpr void operator()(_Rcvr&& __rcvr) const noexcept {
    static_assert(noexcept(static_cast<_Rcvr&&>(__rcvr).set_stopped()), "set_stopped: the receiver's set_stopped must be noexcept");
    static_assert(is_void_v<decltype(static_cast<_Rcvr&&>(__rcvr).set_stopped())>,
                  "set_stopped: the receiver's set_stopped must return void");
    static_cast<_Rcvr&&>(__rcvr).set_stopped();
  }
};
inline constexpr set_value_t set_value{};
inline constexpr set_error_t set_error{};
inline constexpr set_stopped_t set_stopped{};

// [exec.opstate.start]
struct start_t {
  template <class _Op_>
    requires requires(_Op_& op) { op.start(); }
  constexpr void operator()(_Op_& op) const noexcept {
    static_assert(noexcept(op.start()), "start: the operation state's start must be noexcept");
    static_assert(is_void_v<decltype(op.start())>, "start: the operation state's start must return void");
    op.start();
  }
  template <class _Op_>
  void operator()(_Op_&& op) const = delete; // [exec.opstate.start]/1: ill-formed for an rvalue
};
inline constexpr start_t start{};

template <class _Op>
concept operation_state = derived_from<typename _Op::operation_state_concept, operation_state_tag> && requires(_Op& __o) { start(__o); };

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Tag>
concept __completion_tag =
    std::same_as<_Tag, std::execution::set_value_t> || std::same_as<_Tag, std::execution::set_error_t> ||
    std::same_as<_Tag, std::execution::set_stopped_t>;
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// Queries ([exec.queries]) and queryable utilities ([exec.envs]).
namespace [[__gnu__::__visibility__("hidden")]] std {

// [exec.fwd.env]
struct forwarding_query_t {
  template <class _Qp>
  constexpr bool operator()(_Qp __q) const noexcept {
    if constexpr (requires { __q.query(forwarding_query_t{}); }) {
      static_assert(noexcept(__q.query(forwarding_query_t{})), "forwarding_query: the query must be noexcept");
      static_assert(is_same_v<decltype(__q.query(forwarding_query_t{})), bool>, "forwarding_query: the query must return bool");
      return __q.query(forwarding_query_t{});
    } else {
      return derived_from<_Qp, forwarding_query_t>;
    }
  }
};
inline constexpr forwarding_query_t forwarding_query{};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// forwarding-query ([execution.syn]): forwarding_query(T{}) is true.
template <class _Qp>
concept __forwarding_query_c = requires { requires std::forwarding_query_t{}(_Qp{}); };

template <class _Env, class _Qp, class... _As>
concept __has_query = requires(const _Env& env, _As&&... __as) { env.query(_Qp(), static_cast<_As&&>(__as)...); };

// TRY-QUERY(q, tag, args...) ([exec.queries.expos]/2).
template <class _Qp, class _Tag, class... _As>
concept __try_queryable = requires(const _Qp& __q, _Tag tag, const _As&... __as) { __q.query(tag, __as...); } ||
                        requires(const _Qp& __q, _Tag tag) { __q.query(tag); };
template <class _Qp, class _Tag, class... _As>
  requires __try_queryable<_Qp, _Tag, _As...>
[[__gnu__::__always_inline__]] constexpr decltype(auto) try_query(const _Qp& __q, _Tag tag, const _As&... __as) noexcept {
  if constexpr (requires { __q.query(tag, __as...); })
    return __q.query(tag, __as...);
  else {
    ((void)__as, ...);
    return __q.query(tag);
  }
}
template <class _Qp, class _Tag, class... _As>
using __try_query_t = decltype(::__ycxx::__detail::__exec::try_query(std::declval<const _Qp&>(), _Tag(), std::declval<const _As&>()...));

// HIDE-SCHED(q) ([exec.queries.expos]/3): q with get_scheduler and get_domain removed.
template <class _Env>
struct __hide_sched_env;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std {

// [exec.get.allocator]
struct get_allocator_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Self, class _Env>
    requires requires(const _Env& env, const _Self& __q) { env.query(__q); }
  constexpr decltype(auto) operator()(this const _Self&, const _Env& env) noexcept {
    static_assert(noexcept(env.query(get_allocator_t{})), "get_allocator: the query must be noexcept");
    return env.query(get_allocator_t{});
  }
};
inline constexpr get_allocator_t get_allocator{};

// [exec.get.stop.token]
struct get_stop_token_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Env>
  constexpr decltype(auto) operator()(const _Env& env) const noexcept {
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

template <class _Tp>
using stop_token_of_t = remove_cvref_t<decltype(get_stop_token(declval<_Tp>()))>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <std::size_t _Ip, class _Ep>
struct __exec_env_leaf {
  [[no_unique_address]] _Ep __ycxx_env;
};
template <class _Is, class... _Es>
struct __exec_env_storage;
template <std::size_t... _Is, class... _Es>
struct __exec_env_storage<std::index_sequence<_Is...>, _Es...> : __exec_env_leaf<_Is, _Es>... {};
// A const empty member makes env and prop not assignable ([exec.prop]/4, [exec.env]/2) while
// keeping their implicit copy and move constructors (and their aggregate-ness).
struct __exec_not_assignable {};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

// [exec.prop]
template <class _QueryTag, class _ValueType>
struct prop {
  [[no_unique_address]] const _QueryTag __query_;
  _ValueType __value_;
  constexpr const _ValueType& query(_QueryTag, auto&&...) const noexcept { return __value_; }
};
template <class _QueryTag, class _ValueType>
prop(_QueryTag, _ValueType) -> prop<_QueryTag, unwrap_reference_t<_ValueType>>;

// [exec.env]
template <__ycxx::__detail::__exec::__queryable... _Envs>
struct env : __ycxx::__adl_free::__exec_env_storage<index_sequence_for<_Envs...>, _Envs...> {
  // A constructor rather than aggregate initialization, which would need brace elision into the
  // storage base (warned about by -Wmissing-braces at every env{...}).
  env() = default;
  template <class... _As>
    requires(sizeof...(_As) == sizeof...(_Envs) && sizeof...(_As) != 0 && (constructible_from<_Envs, _As> && ...) &&
             (!is_same_v<remove_cvref_t<_As>, env> && ...))
  constexpr env(_As&&... __as) noexcept((is_nothrow_constructible_v<_Envs, _As> && ...))
      : __ycxx::__adl_free::__exec_env_storage<index_sequence_for<_Envs...>, _Envs...>{{static_cast<_As&&>(__as)}...} {}
  env(const env&) = default;
  env(env&&) = default;
  env& operator=(const env&) = delete; // [exec.env]/2

  template <class _QueryTag, class... _Args>
    requires(__ycxx::__detail::__exec::__has_query<_Envs, _QueryTag, _Args...> || ...)
  constexpr decltype(auto) query(_QueryTag __q, _Args&&... __args) const
      noexcept(noexcept(this->template __ycxx_first<_QueryTag, _Args...>().query(__q, static_cast<_Args&&>(__args)...))) {
    return __ycxx_first<_QueryTag, _Args...>().query(__q, static_cast<_Args&&>(__args)...);
  }

  // The first element whose query is well-formed ([exec.env]/6).
  template <class _QueryTag, class... _Args>
  constexpr const auto& __ycxx_first() const noexcept {
    constexpr size_t i = __ycxx::__detail::__exec::__first_true<__ycxx::__detail::__exec::__has_query<_Envs, _QueryTag, _Args...>...>;
    return static_cast<const __ycxx::__adl_free::__exec_env_leaf<i, _Envs...[i]>&>(*this).__ycxx_env;
  }
};
template <class... _Envs>
env(_Envs...) -> env<unwrap_reference_t<_Envs>...>;

// [exec.get.env]
struct get_env_t {
  template <class _Tp>
  constexpr decltype(auto) operator()(const _Tp& __o) const noexcept {
    if constexpr (requires { __o.get_env(); }) {
      static_assert(noexcept(__o.get_env()), "get_env: the get_env member must be noexcept");
      static_assert(__ycxx::__detail::__exec::__queryable<remove_cvref_t<decltype(__o.get_env())>>);
      return __o.get_env();
    } else {
      return env<>{};
    }
  }
};
inline constexpr get_env_t get_env{};
template <class _Tp>
using env_of_t = decltype(get_env(declval<_Tp>()));

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// FWD-ENV(env) ([exec.snd.expos]/4): the forwarding queries of env. E is the environment type,
// or a const lvalue reference to it when the argument was an lvalue.
template <class _Ep>
struct __exec_fwd_env {
  _Ep __ycxx_env;
  template <::__ycxx::__detail::__exec::__forwarding_query_c _Qp, class... _As>
    requires ::__ycxx::__detail::__exec::__has_query<std::remove_cvref_t<_Ep>, _Qp, _As...>
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const
      noexcept(noexcept(::__ycxx::__detail::__exec::__as_const_ref(__ycxx_env).query(__q, static_cast<_As&&>(__as)...))) {
    return ::__ycxx::__detail::__exec::__as_const_ref(__ycxx_env).query(__q, static_cast<_As&&>(__as)...);
  }
};
// JOIN-ENV(env1, env2) ([exec.snd.expos]/6).
template <class _E1, class _E2>
struct __exec_join_env {
  _E1 __ycxx_env1;
  _E2 __ycxx_env2;
  template <class _Qp, class... _As>
    requires ::__ycxx::__detail::__exec::__has_query<std::remove_cvref_t<_E1>, _Qp, _As...>
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const
      noexcept(noexcept(::__ycxx::__detail::__exec::__as_const_ref(__ycxx_env1).query(__q, static_cast<_As&&>(__as)...))) {
    return ::__ycxx::__detail::__exec::__as_const_ref(__ycxx_env1).query(__q, static_cast<_As&&>(__as)...);
  }
  template <class _Qp, class... _As>
    requires(!::__ycxx::__detail::__exec::__has_query<std::remove_cvref_t<_E1>, _Qp, _As...> &&
             ::__ycxx::__detail::__exec::__has_query<std::remove_cvref_t<_E2>, _Qp, _As...>)
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const
      noexcept(noexcept(::__ycxx::__detail::__exec::__as_const_ref(__ycxx_env2).query(__q, static_cast<_As&&>(__as)...))) {
    return ::__ycxx::__detail::__exec::__as_const_ref(__ycxx_env2).query(__q, static_cast<_As&&>(__as)...);
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// An environment argument kept by reference when it is an lvalue, by value otherwise.
template <class _Ep>
using __env_member_t = std::conditional_t<std::is_lvalue_reference_v<_Ep>, const std::remove_reference_t<_Ep>&, std::remove_cvref_t<_Ep>>;

template <class _Ep>
constexpr auto __fwd_env(_Ep&& e) noexcept(std::is_nothrow_constructible_v<__env_member_t<_Ep>, _Ep>) {
  return ::__ycxx::__adl_free::__exec_fwd_env<__env_member_t<_Ep>>{static_cast<_Ep&&>(e)};
}
template <class _Ep>
using __fwd_env_t = decltype(::__ycxx::__detail::__exec::__fwd_env(std::declval<_Ep>()));

template <class _E1, class _E2>
constexpr auto __join_env(_E1&& __e1, _E2&& __e2) noexcept(std::is_nothrow_constructible_v<__env_member_t<_E1>, _E1> &&
                                                   std::is_nothrow_constructible_v<__env_member_t<_E2>, _E2>) {
  return ::__ycxx::__adl_free::__exec_join_env<__env_member_t<_E1>, __env_member_t<_E2>>{static_cast<_E1&&>(__e1), static_cast<_E2&&>(__e2)};
}
template <class _E1, class _E2>
using __join_env_t = decltype(::__ycxx::__detail::__exec::__join_env(std::declval<_E1>(), std::declval<_E2>()));
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// Awaitable helpers ([exec.awaitable]).
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

template <class _Tp>
inline constexpr bool __is_coroutine_handle = false;
template <class _Pp>
inline constexpr bool __is_coroutine_handle<std::coroutine_handle<_Pp>> = true;
template <class _Tp>
concept __await_suspend_result = std::is_void_v<_Tp> || std::is_same_v<_Tp, bool> || __is_coroutine_handle<_Tp>;

template <class _Ap, class... _Promise>
concept __is_awaiter = requires(_Ap& a, std::coroutine_handle<_Promise...> h) {
  a.await_ready() ? 1 : 0;
  { a.await_suspend(h) } -> __await_suspend_result;
  a.await_resume();
};

// GET-AWAITER(c, p): await_transform, then operator co_await (member or not), then the operand.
struct __none_such_promise {};

template <class _Cp>
[[__gnu__::__always_inline__]] constexpr decltype(auto) __get_awaiter_after_transform(_Cp&& c) noexcept {
  return static_cast<_Cp&&>(c);
}
template <class _Cp>
  requires requires(_Cp&& c) { static_cast<_Cp&&>(c).operator co_await(); }
constexpr decltype(auto) __get_awaiter_after_transform(_Cp&& c) noexcept(noexcept(static_cast<_Cp&&>(c).operator co_await())) {
  return static_cast<_Cp&&>(c).operator co_await();
}
template <class _Cp>
  requires(!requires(_Cp&& c) { static_cast<_Cp&&>(c).operator co_await(); }) &&
          requires(_Cp&& c) { operator co_await(static_cast<_Cp&&>(c)); }
constexpr decltype(auto) __get_awaiter_after_transform(_Cp&& c) noexcept(noexcept(operator co_await(static_cast<_Cp&&>(c)))) {
  return operator co_await(static_cast<_Cp&&>(c));
}

template <class _Cp, class _Promise>
constexpr decltype(auto) __get_awaiter(_Cp&& c, _Promise& p) {
  if constexpr (requires { p.await_transform(static_cast<_Cp&&>(c)); })
    return ::__ycxx::__detail::__exec::__get_awaiter_after_transform(p.await_transform(static_cast<_Cp&&>(c)));
  else
    return ::__ycxx::__detail::__exec::__get_awaiter_after_transform(static_cast<_Cp&&>(c));
}
// GET-AWAITER(c): with a promise that has no await_transform.
template <class _Cp>
constexpr decltype(auto) __get_awaiter(_Cp&& c) {
  return ::__ycxx::__detail::__exec::__get_awaiter_after_transform(static_cast<_Cp&&>(c));
}

template <class _Cp, class... _Promise>
concept __is_awaitable = requires(_Cp (*__fc)() noexcept, _Promise&... p) {
  { ::__ycxx::__detail::__exec::__get_awaiter(__fc(), p...) } -> __is_awaiter<_Promise...>;
};
template <class _Cp>
concept __is_awaitable_np = requires(_Cp (*__fc)() noexcept, __none_such_promise& p) {
  { ::__ycxx::__detail::__exec::__get_awaiter(__fc(), p) } -> __is_awaiter<>;
};

template <class _Cp, class... _Promise>
using __await_result_type =
    decltype(::__ycxx::__detail::__exec::__get_awaiter(std::declval<_Cp>(), std::declval<_Promise&>()...).await_resume());

// with-await-transform ([exec.awaitable]/5)
template <class _Tp, class _Promise>
concept __has_as_awaitable = requires(_Tp&& t, _Promise& p) {
  { static_cast<_Tp&&>(t).as_awaitable(p) } -> __is_awaitable<_Promise&>;
};

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Derived>
struct __exec_with_await_transform {
  template <class _Tp>
  _Tp&& await_transform(_Tp&& value) noexcept {
    return static_cast<_Tp&&>(value);
  }
  template <::__ycxx::__detail::__exec::__has_as_awaitable<_Derived> _Tp>
  auto await_transform(_Tp&& value) noexcept(noexcept(static_cast<_Tp&&>(value).as_awaitable(std::declval<_Derived&>())))
      -> decltype(static_cast<_Tp&&>(value).as_awaitable(std::declval<_Derived&>())) {
    return static_cast<_Tp&&>(value).as_awaitable(static_cast<_Derived&>(*this));
  }
};
// env-promise ([exec.awaitable]/6): used only for type computations.
template <class _Env>
struct __exec_env_promise : __exec_with_await_transform<__exec_env_promise<_Env>> {
  void get_return_object() noexcept;
  std::suspend_always initial_suspend() noexcept;
  std::suspend_always final_suspend() noexcept;
  void unhandled_exception() noexcept;
  void return_void() noexcept;
  std::coroutine_handle<> unhandled_stopped() noexcept;
  const _Env& get_env() const noexcept;
};
}} // namespace __ycxx::__adl_free

// ---------------------------------------------------------------------------------------------
// The sender concept, schedule and schedulers ([exec.snd.concepts], [exec.schedule], [exec.sched]).
namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <class _Sndr>
inline constexpr bool enable_sender =
    requires { requires derived_from<typename _Sndr::sender_concept, sender_tag>; } ||
    __ycxx::__detail::__exec::__is_awaitable<_Sndr, __ycxx::__adl_free::__exec_env_promise<env<>>>;

template <class _Sndr>
concept sender = enable_sender<remove_cvref_t<_Sndr>> && requires(const remove_cvref_t<_Sndr>& __sndr) {
  { get_env(__sndr) } -> __ycxx::__detail::__exec::__queryable;
} && move_constructible<remove_cvref_t<_Sndr>> && constructible_from<remove_cvref_t<_Sndr>, _Sndr>;

// [exec.schedule]
struct schedule_t {
  template <class _Sch>
    requires requires(_Sch&& __sch) { static_cast<_Sch&&>(__sch).schedule(); }
  constexpr decltype(auto) operator()(_Sch&& __sch) const noexcept(noexcept(static_cast<_Sch&&>(__sch).schedule())) {
    static_assert(sender<decltype(static_cast<_Sch&&>(__sch).schedule())>, "schedule: the result must be a sender");
    return static_cast<_Sch&&>(__sch).schedule();
  }
};
inline constexpr schedule_t schedule{};

enum class forward_progress_guarantee { concurrent, parallel, weakly_parallel };

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// The scheduler concept without its get_forward_progress_guarantee requirement, which that query
// itself requires of its argument.
template <class _Sch>
concept __scheduler_base = std::derived_from<typename std::remove_cvref_t<_Sch>::scheduler_concept, std::execution::scheduler_tag> &&
                         __queryable<_Sch> && requires(_Sch&& __sch) {
                           { std::execution::schedule(static_cast<_Sch&&>(__sch)) } -> std::execution::sender;
                         } && std::equality_comparable<std::remove_cvref_t<_Sch>> && std::copyable<std::remove_cvref_t<_Sch>>;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

// [exec.get.fwd.progress]
struct get_forward_progress_guarantee_t {
  template <class _Self, class _Sch>
    requires __ycxx::__detail::__exec::__scheduler_base<_Sch&> &&
             requires(const remove_cvref_t<_Sch>& s, const _Self& __q) { s.query(__q); }
  constexpr forward_progress_guarantee operator()(this const _Self&, _Sch&& __sch) noexcept {
    const auto& s = __sch;
    static_assert(noexcept(s.query(get_forward_progress_guarantee_t{})),
                  "get_forward_progress_guarantee: the query must be noexcept");
    static_assert(is_same_v<decltype(s.query(get_forward_progress_guarantee_t{})), forward_progress_guarantee>,
                  "get_forward_progress_guarantee: the query must return forward_progress_guarantee");
    return s.query(get_forward_progress_guarantee_t{});
  }
};
inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};

// [exec.sched]
template <class _Sch>
concept scheduler = __ycxx::__detail::__exec::__scheduler_base<_Sch> && requires(_Sch&& __sch) {
  { get_forward_progress_guarantee(__sch) } -> same_as<forward_progress_guarantee>;
};

template <scheduler _Sch>
using schedule_result_t = decltype(schedule(declval<_Sch>()));

// [exec.get.compl.sched]
template <class _CPO>
struct get_completion_scheduler_t;
template <class _CPO = void>
struct get_completion_domain_t;
struct default_domain;
template <class... _Domains>
struct indeterminate_domain;

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// RECURSE-QUERY(sch, envs...) ([exec.get.compl.sched]/4).
template <class _Sch, class... _Envs>
constexpr auto __recurse_query(_Sch __sch, const _Envs&... __envs) noexcept {
  using __gcs = std::execution::get_completion_scheduler_t<__dependent_t<std::execution::set_value_t, _Sch>>;
  if constexpr (__try_queryable<_Sch, __gcs, _Envs...>) {
    auto __sch2 = ::__ycxx::__detail::__exec::try_query(__sch, __gcs{}, __envs...);
    if constexpr (std::is_same_v<decltype(__sch2), _Sch>) {
      while (!(__sch2 == __sch)) {
        __sch = __sch2;
        if constexpr (std::is_same_v<decltype(::__ycxx::__detail::__exec::try_query(__sch, __gcs{}, __envs...)), _Sch>)
          __sch2 = ::__ycxx::__detail::__exec::try_query(__sch, __gcs{}, __envs...);
      }
      return __sch;
    } else {
      return ::__ycxx::__detail::__exec::__recurse_query(__sch2, __envs...);
    }
  } else {
    return __sch;
  }
}
template <class _Tag, class _Qp, class... _Envs>
concept __completion_scheduler_via_query =
    __try_queryable<_Qp, std::execution::get_completion_scheduler_t<_Tag>, _Envs...> &&
    requires(const _Qp& __q, const _Envs&... __envs) {
      ::__ycxx::__detail::__exec::__recurse_query(
          ::__ycxx::__detail::__exec::try_query(__q, std::execution::get_completion_scheduler_t<_Tag>{}, __envs...), __envs...);
    };
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <class _CPO>
struct get_completion_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Qp, class... _Envs>
    requires __ycxx::__detail::__exec::__completion_tag<_CPO> &&
             (__ycxx::__detail::__exec::__completion_scheduler_via_query<_CPO, _Qp, _Envs...> ||
              (sizeof...(_Envs) != 0 && scheduler<const _Qp&>))
  constexpr auto operator()(const _Qp& __q, const _Envs&... __envs) const noexcept {
    if constexpr (__ycxx::__detail::__exec::__completion_scheduler_via_query<_CPO, _Qp, _Envs...>) {
      auto s = __ycxx::__detail::__exec::__recurse_query(__ycxx::__detail::__exec::try_query(__q, *this, __envs...), __envs...);
      static_assert(scheduler<decltype(s)>, "get_completion_scheduler: the result must be a scheduler");
      return s;
    } else {
      ((void)__envs, ...);
      return __q;
    }
  }
};
template <class _CPO>
constexpr get_completion_scheduler_t<_CPO> get_completion_scheduler{};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// The D of get_completion_domain<Tag>(attrs, envs...) ([exec.get.compl.domain]/2); void when
// that expression is ill-formed.
template <class _Tag, class _Ap, class... _Envs>
struct __compl_domain {
  static auto __pick() {
    using std::execution::get_completion_domain_t;
    using std::execution::get_completion_scheduler_t;
    if constexpr (__try_queryable<_Ap, get_completion_domain_t<_Tag>, _Envs...>)
      return std::type_identity<std::remove_cvref_t<__try_query_t<_Ap, get_completion_domain_t<_Tag>, _Envs...>>>{};
    else if constexpr (std::is_void_v<_Tag>)
      return std::type_identity<typename __compl_domain<std::execution::set_value_t, _Ap, _Envs...>::type>{};
    else if constexpr (requires(const _Ap& a, const _Envs&... e) {
                         ::__ycxx::__detail::__exec::try_query(get_completion_scheduler_t<_Tag>{}(a, e...),
                                                         get_completion_domain_t<__dependent_t<std::execution::set_value_t, _Ap>>{}, e...);
                       })
      return std::type_identity<std::remove_cvref_t<decltype(::__ycxx::__detail::__exec::try_query(
          get_completion_scheduler_t<_Tag>{}(std::declval<const _Ap&>(), std::declval<const _Envs&>()...),
          get_completion_domain_t<__dependent_t<std::execution::set_value_t, _Ap>>{}, std::declval<const _Envs&>()...))>>{};
    else if constexpr (std::execution::scheduler<const _Ap&> && sizeof...(_Envs) != 0)
      return std::type_identity<std::execution::default_domain>{};
    else
      return std::type_identity<void>{};
  }
  using type = typename decltype(__pick())::type;
};
template <class _Tag, class _Ap, class... _Envs>
using __compl_domain_t = typename __compl_domain<_Tag, _Ap, _Envs...>::type;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

// [exec.get.compl.domain]
template <class _CPO>
struct get_completion_domain_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Ap, class... _Envs>
    requires(is_void_v<_CPO> || __ycxx::__detail::__exec::__completion_tag<_CPO>) &&
            (!is_void_v<__ycxx::__detail::__exec::__compl_domain_t<_CPO, _Ap, _Envs...>>)
  constexpr auto operator()(const _Ap&, const _Envs&...) const noexcept {
    using _Dp = __ycxx::__detail::__exec::__compl_domain_t<_CPO, _Ap, _Envs...>;
    static_assert(noexcept(_Dp()), "get_completion_domain: constructing the domain must not throw");
    return _Dp();
  }
};
template <class _CPO = void>
constexpr get_completion_domain_t<_CPO> get_completion_domain{};

// [exec.get.scheduler]
struct get_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Self, class _Env>
    requires requires(const _Env& env, const _Self& __q, const __ycxx::__detail::__exec::__hide_sched_env<_Env>& h) {
      get_completion_scheduler_t<set_value_t>{}(env.query(__q), h);
    }
  constexpr auto operator()(this const _Self&, const _Env& env) noexcept {
    static_assert(noexcept(env.query(get_scheduler_t{})), "get_scheduler: the query must be noexcept");
    auto s = get_completion_scheduler_t<set_value_t>{}(env.query(get_scheduler_t{}), __ycxx::__detail::__exec::__hide_sched_env<_Env>{env});
    static_assert(scheduler<decltype(s)>, "get_scheduler: the result must be a scheduler");
    return s;
  }
};
inline constexpr get_scheduler_t get_scheduler{};

// [exec.get.domain]
struct get_domain_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Env>
  static constexpr auto __ycxx_pick() noexcept {
    if constexpr (requires(const _Env& env) { auto(env.query(get_domain_t{})); })
      return type_identity<decltype(auto(declval<const _Env&>().query(get_domain_t{})))>{};
    else if constexpr (requires(const _Env& env, const __ycxx::__detail::__exec::__hide_sched_env<_Env>& h) {
                         get_completion_domain_t<set_value_t>{}(get_scheduler_t{}(env), h);
                       })
      return type_identity<decltype(get_completion_domain_t<set_value_t>{}(
          get_scheduler_t{}(declval<const _Env&>()), declval<const __ycxx::__detail::__exec::__hide_sched_env<_Env>&>()))>{};
    else
      return type_identity<default_domain>{};
  }
  template <class _Env>
  constexpr auto operator()(const _Env&) const noexcept {
    using _Dp = typename decltype(__ycxx_pick<_Env>())::type;
    static_assert(noexcept(_Dp()), "get_domain: constructing the domain must not throw");
    return _Dp();
  }
};
inline constexpr get_domain_t get_domain{};

// [exec.get.start.scheduler]
struct get_start_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Self, class _Env>
    requires requires(const _Env& env, const _Self& __q) { env.query(__q); }
  constexpr decltype(auto) operator()(this const _Self&, const _Env& env) noexcept {
    static_assert(noexcept(env.query(get_start_scheduler_t{})), "get_start_scheduler: the query must be noexcept");
    static_assert(scheduler<decltype(env.query(get_start_scheduler_t{}))>, "get_start_scheduler: the result must be a scheduler");
    return env.query(get_start_scheduler_t{});
  }
};
inline constexpr get_start_scheduler_t get_start_scheduler{};

// [exec.get.delegation.scheduler]
struct get_delegation_scheduler_t {
  static constexpr bool query(forwarding_query_t) noexcept { return true; }
  template <class _Self, class _Env>
    requires requires(const _Env& env, const _Self& __q) { env.query(__q); }
  constexpr decltype(auto) operator()(this const _Self&, const _Env& env) noexcept {
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
  template <class _Self, class _Env>
    requires requires(const _Env& env, const _Self& __q) { env.query(__q); }
  constexpr decltype(auto) operator()(this const _Self&, const _Env& env) noexcept {
    static_assert(noexcept(env.query(get_await_completion_adaptor_t{})), "get_await_completion_adaptor: the query must be noexcept");
    return env.query(get_await_completion_adaptor_t{});
  }
};
inline constexpr get_await_completion_adaptor_t get_await_completion_adaptor{};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Env>
struct __hide_sched_env {
  const _Env& env;
  template <class _Qp, class... _As>
    requires(!std::is_same_v<_Qp, std::execution::get_scheduler_t> && !std::is_same_v<_Qp, std::execution::get_domain_t>) &&
            __has_query<_Env, _Qp, _As...>
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const noexcept(noexcept(env.query(__q, static_cast<_As&&>(__as)...))) {
    return env.query(__q, static_cast<_As&&>(__as)...);
  }
};

// inline-attrs<Tag> ([exec.snd.expos]/59): the attributes of a sender that completes with Tag on
// the agent that starts it.
template <class _Tag>
struct __inline_attrs {
  template <class _Env>
    requires requires(const _Env& env) { std::execution::get_scheduler(env); }
  constexpr auto query(std::execution::get_completion_scheduler_t<_Tag>, const _Env& env) const noexcept {
    return std::execution::get_scheduler(env);
  }
  template <class _Env>
  constexpr auto query(std::execution::get_completion_domain_t<_Tag>, const _Env& env) const noexcept {
    return std::execution::get_domain(env);
  }
};

// COMMON-DOMAIN(domains...) ([exec.snd.expos]/8), as a type.
template <class... _Ds>
struct __common_domain {
  static auto __pick() {
    if constexpr (requires { typename std::common_type_t<_Ds...>; requires (sizeof...(_Ds) != 0); })
      return std::type_identity<std::common_type_t<_Ds...>>{};
    else
      return std::type_identity<__apply_unique_t<std::execution::indeterminate_domain, _Ds...>>{};
  }
  using type = typename decltype(__pick())::type;
};
template <class... _Ds>
using __common_domain_t = typename __common_domain<_Ds...>::type;

// COMPL-DOMAIN(Tag, sndr, envs) ([exec.snd.expos]/9), as a type.
template <class _Tag, class _Sndr, class... _Envs>
using __compl_domain_of_t = std::conditional_t<
    !std::is_void_v<__compl_domain_t<_Tag, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<_Sndr>()))>, _Envs...>> ||
        sizeof...(_Envs) == 0,
    __compl_domain_t<_Tag, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<_Sndr>()))>, _Envs...>,
    std::execution::indeterminate_domain<>>;
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// Completion signatures ([exec.cmplsig]).
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Fn>
inline constexpr bool __is_completion_signature = false;
template <class... _Vs>
inline constexpr bool __is_completion_signature<std::execution::set_value_t(_Vs...)> =
    ((std::is_object_v<_Vs> || std::is_reference_v<_Vs>) && ...);
template <class _Ep>
inline constexpr bool __is_completion_signature<std::execution::set_error_t(_Ep)> = std::is_object_v<_Ep> || std::is_reference_v<_Ep>;
template <>
inline constexpr bool __is_completion_signature<std::execution::set_stopped_t()> = true;
template <class _Fn>
concept __completion_signature = __is_completion_signature<_Fn>;

template <class _Sig>
struct __sig_tag;
template <class _Tag, class... _As>
struct __sig_tag<_Tag(_As...)> {
  using type = _Tag;
  using __args = __tlist<_As...>;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <__ycxx::__detail::__exec::__completion_signature... _Fns>
struct completion_signatures {
  // count-of(tag) and for-each(fn) ([exec.cmplsig]/8), exposition-only in the draft.
  template <class _Tag>
  static constexpr size_t __ycxx_count_of(_Tag) {
    return (size_t{0} + ... + size_t{is_same_v<typename __ycxx::__detail::__exec::__sig_tag<_Fns>::type, decay_t<_Tag>>});
  }
  template <class _Fn>
  static constexpr void __ycxx_for_each(_Fn&& __fn) {
    (__fn(static_cast<_Fns*>(nullptr)), ...);
  }
};

struct dependent_sender_error : exception {
  constexpr const char* what() const noexcept override { return "std::execution::dependent_sender_error"; }
};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

template <class _Tp>
inline constexpr bool __is_csigs = false;
template <class... _Fns>
inline constexpr bool __is_csigs<std::execution::completion_signatures<_Fns...>> = true;
template <class _Tp>
concept __valid_completion_signatures = __is_csigs<_Tp>;

// The error results of a completion-signature computation.
struct __dependent_sigs {};
template <class _What, class... _Info>
struct __invalid_sigs {};
template <class _Tp>
inline constexpr bool __is_invalid_sigs = false;
template <class _What, class... _Info>
inline constexpr bool __is_invalid_sigs<__invalid_sigs<_What, _Info...>> = true;

// Problems reported by invalid_sigs (the names show in diagnostics).
struct __sender_has_no_completion_signatures;
struct __not_a_sender_for_this_environment;
struct __function_not_invocable_with_these_arguments;
struct __result_datums_not_decay_copyable;
struct __let_function_must_return_a_sender;
struct __when_all_child_has_more_than_one_value_completion;
struct __sender_has_not_exactly_one_value_completion;
struct __environment_has_no_start_scheduler;
struct __start_scheduler_is_not_infallible;
struct __read_env_query_ill_formed_or_void;
struct __transform_sender_ill_formed;

// Sets of completion signatures with duplicates removed; an error operand gives the error
// (the first invalid one, else dependent).
template <class... _CS>
struct __sigs_concat;
template <>
struct __sigs_concat<> {
  using type = std::execution::completion_signatures<>;
};
template <class... _Fns>
struct __sigs_concat<std::execution::completion_signatures<_Fns...>> {
  using type = __apply_unique_t<std::execution::completion_signatures, _Fns...>;
};
template <class... _As, class... _Bs, class... _Rest>
struct __sigs_concat<std::execution::completion_signatures<_As...>, std::execution::completion_signatures<_Bs...>, _Rest...>
    : __sigs_concat<std::execution::completion_signatures<_As..., _Bs...>, _Rest...> {};
template <class _First, class... _Rest>
  requires(!__is_csigs<_First>)
struct __sigs_concat<_First, _Rest...> {
  static auto __pick() {
    if constexpr (__is_invalid_sigs<_First>)
      return std::type_identity<_First>{};
    else if constexpr (requires { typename __sigs_concat<_Rest...>::type; requires __is_invalid_sigs<typename __sigs_concat<_Rest...>::type>; })
      return std::type_identity<typename __sigs_concat<_Rest...>::type>{};
    else
      return std::type_identity<_First>{};
  }
  using type = typename decltype(__pick())::type;
};
template <class _CS, class... _Rest>
  requires __is_csigs<_CS> && (sizeof...(_Rest) != 0) && (!__is_csigs<_Rest> || ...)
struct __sigs_concat<_CS, _Rest...> {
  static auto __pick() {
    using _Rp = typename __sigs_concat<_Rest...>::type;
    if constexpr (__is_csigs<_Rp>)
      return std::type_identity<typename __sigs_concat<_CS, _Rp>::type>{};
    else
      return std::type_identity<_Rp>{};
  }
  using type = typename decltype(__pick())::type;
};
template <class... _CS>
using __sigs_concat_t = typename __sigs_concat<_CS...>::type;

// Maps each signature of CS through F<Sig> (a completion_signatures or an error) and joins the
// results; an error CS is the result.
template <class _CS, template <class> class _Fp>
struct __sigs_map {
  using type = _CS;
};
template <class... _Sigs, template <class> class _Fp>
struct __sigs_map<std::execution::completion_signatures<_Sigs...>, _Fp> {
  using type = __sigs_concat_t<_Fp<_Sigs>...>;
};
template <class _CS, template <class> class _Fp>
using __sigs_map_t = typename __sigs_map<_CS, _Fp>::type;

// The signatures with the given tag, as a tlist of the argument tlists.
template <class _Tag, class _CS>
struct __sigs_args;
template <class _Tag, class... _Sigs>
struct __sigs_args<_Tag, std::execution::completion_signatures<_Sigs...>> {
  using type = typename __tlist_concat<
      std::conditional_t<std::is_same_v<typename __sig_tag<_Sigs>::type, _Tag>, __tlist<typename __sig_tag<_Sigs>::__args>, __tlist<>>...>::type;
};
template <class _Tag, class _CS>
using __sigs_args_t = typename __sigs_args<_Tag, _CS>::type;

template <class _Tag, class _CS>
inline constexpr std::size_t __sigs_count = 0;
template <class _Tag, class... _Sigs>
inline constexpr std::size_t __sigs_count<_Tag, std::execution::completion_signatures<_Sigs...>> =
    (std::size_t{0} + ... + std::size_t{std::is_same_v<typename __sig_tag<_Sigs>::type, _Tag>});

// META-APPLY ([exec.cmplsig]/6).
template <bool>
struct __indirect_meta_apply {
  template <template <class...> class _Tp, class... _As>
  using __meta_apply = _Tp<_As...>;
};
template <class...>
concept __always_true = true;

// Alias templates throughout, so that a Tuple or Variant of the wrong arity is a substitution
// failure (a nested class template would make it a hard error).
template <class _Args>
struct __gather_tuple;
template <class... _As>
struct __gather_tuple<__tlist<_As...>> {
  template <template <class...> class _Tuple>
  using __with = typename __indirect_meta_apply<__always_true<_As...>>::template __meta_apply<_Tuple, _As...>;
};
template <class _ArgLists>
struct __gather_variant;
template <class... _ArgLists>
struct __gather_variant<__tlist<_ArgLists...>> {
  template <template <class...> class _Tuple, template <class...> class _Variant>
  using __with = typename __indirect_meta_apply<__always_true<_ArgLists...>>::template __meta_apply<
      _Variant, typename __gather_tuple<_ArgLists>::template __with<_Tuple>...>;
};
// gather-signatures<Tag, Completions, Tuple, Variant>
template <class _Tag, __valid_completion_signatures _Completions, template <class...> class _Tuple, template <class...> class _Variant>
using __gather_signatures = typename __gather_variant<__sigs_args_t<_Tag, _Completions>>::template __with<_Tuple, _Variant>;

template <class... _Ts>
using __decayed_tuple = std::tuple<std::decay_t<_Ts>...>;

struct __empty_variant {
  __empty_variant() = delete;
};
template <class... _Ts>
struct __variant_or_empty_impl {
  using type = __apply_unique_t<std::variant, std::decay_t<_Ts>...>;
};
template <>
struct __variant_or_empty_impl<> {
  using type = __empty_variant;
};
template <class... _Ts>
using __variant_or_empty = typename __variant_or_empty_impl<_Ts...>::type;

template <class... _Ts>
struct __type_list_tl {};

// MATCHING-SIG(F1, F2) ([exec.general]/7)
template <class _F1, class _F2>
inline constexpr bool __matching_sig = false;
template <class _R1, class... _A1, class _R2, class... _A2>
inline constexpr bool __matching_sig<_R1(_A1...), _R2(_A2...)> = std::is_same_v<_R1(_A1 && ...), _R2(_A2 && ...)>;

// SET-VALUE-SIG(T)
template <class _Tp>
struct __set_value_sig {
  using type = std::execution::set_value_t(_Tp);
};
template <class _Tp>
  requires std::is_void_v<_Tp>
struct __set_value_sig<_Tp> {
  using type = std::execution::set_value_t();
};
template <class _Tp>
using __set_value_sig_t = typename __set_value_sig<_Tp>::type;

}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// Domains, transform_sender, apply_sender ([exec.domain.indeterminate], [exec.domain.default],
// [exec.snd.transform], [exec.snd.apply]); tag_of_t ([exec.snd.concepts]/6).
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// tag_of_t: the type of the first element of a tuple-like sender with at least two elements
// (the library's senders). Aggregates of other forms are not recognised (DECISIONS).
template <class _Sndr>
concept __has_sender_tag = requires(_Sndr&& s) {
  requires std::tuple_size<std::remove_cvref_t<_Sndr>>::value >= 2;
  static_cast<_Sndr&&>(s).template get<0>();
};
template <class _Sndr>
struct __tag_of {};
template <class _Sndr>
  requires __has_sender_tag<_Sndr>
struct __tag_of<_Sndr> {
  using type = std::decay_t<decltype(std::declval<_Sndr>().template get<0>())>;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <sender _Sndr>
using tag_of_t = typename __ycxx::__detail::__exec::__tag_of<_Sndr>::type;

// [exec.domain.default]
struct default_domain {
  template <class _Tag, sender _Sndr, __ycxx::__detail::__exec::__queryable _Env>
  static constexpr decltype(auto) transform_sender(_Tag, _Sndr&& __sndr, const _Env& env) noexcept(
      noexcept(__ycxx_transform(_Tag(), static_cast<_Sndr&&>(__sndr), env))) {
    return __ycxx_transform(_Tag(), static_cast<_Sndr&&>(__sndr), env);
  }
  template <class _Tag, sender _Sndr, class... _Args>
    requires requires(_Sndr&& __sndr, _Args&&... __args) { _Tag().apply_sender(static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...); }
  static constexpr decltype(auto) apply_sender(_Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(
      noexcept(_Tag().apply_sender(static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...))) {
    return _Tag().apply_sender(static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...);
  }

private:
  template <class _Tag, class _Sndr, class _Env>
  static constexpr decltype(auto) __ycxx_transform(_Tag, _Sndr&& __sndr, const _Env& env) noexcept(
      noexcept(tag_of_t<_Sndr>().transform_sender(_Tag(), static_cast<_Sndr&&>(__sndr), env)))
    requires requires { tag_of_t<_Sndr>().transform_sender(_Tag(), static_cast<_Sndr&&>(__sndr), env); }
  {
    return tag_of_t<_Sndr>().transform_sender(_Tag(), static_cast<_Sndr&&>(__sndr), env);
  }
  template <class _Tag, class _Sndr, class _Env>
  static constexpr _Sndr __ycxx_transform(_Tag, _Sndr&& __sndr, const _Env&) noexcept {
    return static_cast<_Sndr>(static_cast<_Sndr&&>(__sndr));
  }
};

// [exec.domain.indeterminate]
template <class... _Domains>
struct indeterminate_domain {
  indeterminate_domain() = default;
  constexpr indeterminate_domain(auto&&) noexcept {}
  template <class _Tag, sender _Sndr, __ycxx::__detail::__exec::__queryable _Env>
  static constexpr decltype(auto) transform_sender(_Tag, _Sndr&& __sndr, const _Env& env) noexcept(
      noexcept(default_domain().transform_sender(_Tag(), static_cast<_Sndr&&>(__sndr), env))) {
    using _Rp = decay_t<decltype(default_domain().transform_sender(_Tag(), static_cast<_Sndr&&>(__sndr), env))>;
    static_assert((__ycxx_agrees<_Domains, _Rp, _Tag, _Sndr, _Env> && ...),
                  "indeterminate_domain: the possible domains transform the sender differently");
    return default_domain().transform_sender(_Tag(), static_cast<_Sndr&&>(__sndr), env);
  }

private:
  template <class _Dp, class _Rp, class _Tag, class _Sndr, class _Env>
  static constexpr bool __ycxx_agrees = [] {
    if constexpr (requires { _Dp().transform_sender(_Tag(), declval<_Sndr>(), declval<const _Env&>()); })
      return is_same_v<decay_t<decltype(_Dp().transform_sender(_Tag(), declval<_Sndr>(), declval<const _Env&>()))>, _Rp>;
    else
      return true;
  }();
};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] std {
// [exec.domain.indeterminate]/4
template <class... _Ds, class... _Es>
struct common_type<execution::indeterminate_domain<_Ds...>, execution::indeterminate_domain<_Es...>> {
  using type = __ycxx::__detail::__exec::__apply_unique_t<execution::indeterminate_domain, _Ds..., _Es...>;
};
template <class... _Ds, class _Dp>
struct common_type<execution::indeterminate_domain<_Ds...>, _Dp> {
  using type = conditional_t<sizeof...(_Ds) == 0, _Dp, __ycxx::__detail::__exec::__apply_unique_t<execution::indeterminate_domain, _Ds..., _Dp>>;
};
template <class _Dp, class... _Ds>
struct common_type<_Dp, execution::indeterminate_domain<_Ds...>> {
  using type = conditional_t<sizeof...(_Ds) == 0, _Dp, __ycxx::__detail::__exec::__apply_unique_t<execution::indeterminate_domain, _Ds..., _Dp>>;
};
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// transformed-sndr(dom, tag, s) and transform-recurse ([exec.snd.transform]/3). Its exception
// specification is that of the transform_sender it calls: the domain's, else default_domain's.
template <class _Dom, class _Tag, class _Sndr, class _Env>
consteval bool __transformed_sndr_nothrow() {
  if constexpr (requires { std::declval<_Dom&>().transform_sender(_Tag(), std::declval<_Sndr>(), std::declval<const _Env&>()); })
    return noexcept(std::declval<_Dom&>().transform_sender(_Tag(), std::declval<_Sndr>(), std::declval<const _Env&>()));
  else
    return noexcept(std::execution::default_domain().transform_sender(_Tag(), std::declval<_Sndr>(), std::declval<const _Env&>()));
}
template <class _Dom, class _Tag, class _Sndr, class _Env>
constexpr decltype(auto) __transformed_sndr(_Dom __dom, _Tag tag, _Sndr&& s, const _Env& env) noexcept(
    ::__ycxx::__detail::__exec::__transformed_sndr_nothrow<_Dom, _Tag, _Sndr, _Env>()) {
  if constexpr (requires { __dom.transform_sender(tag, static_cast<_Sndr&&>(s), env); })
    return __dom.transform_sender(tag, static_cast<_Sndr&&>(s), env);
  else
    return std::execution::default_domain().transform_sender(tag, static_cast<_Sndr&&>(s), env);
}

template <class _Sp, class _Env>
using __completion_domain_for = std::conditional_t<
    std::is_void_v<__compl_domain_t<void, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<_Sp>()))>, _Env>>,
    std::execution::default_domain,
    __compl_domain_t<void, std::remove_cvref_t<decltype(std::execution::get_env(std::declval<_Sp>()))>, _Env>>;
template <class _Env>
using __start_domain_for = decltype(std::execution::get_domain(std::declval<const _Env&>()));

template <class _Dom, class _Tag, class _Sndr, class _Env>
constexpr decltype(auto) __transform_recurse(_Dom __dom, _Tag tag, _Sndr&& s, const _Env& env) {
  using _S2 = decltype(::__ycxx::__detail::__exec::__transformed_sndr(__dom, tag, static_cast<_Sndr&&>(s), env));
  if constexpr (std::is_same_v<std::remove_cvref_t<_S2>, std::remove_cvref_t<_Sndr>>) {
    return ::__ycxx::__detail::__exec::__transformed_sndr(__dom, tag, static_cast<_Sndr&&>(s), env);
  } else {
    using _Dom2 = std::conditional_t<std::is_same_v<_Tag, std::execution::start_t>, __start_domain_for<_Env>, __completion_domain_for<_S2, _Env>>;
    return ::__ycxx::__detail::__exec::__transform_recurse(_Dom2(), tag, ::__ycxx::__detail::__exec::__transformed_sndr(__dom, tag, static_cast<_Sndr&&>(s), env), env);
  }
}
// Whether a transformation may throw: the noexcept of every step.
template <class _Dom, class _Tag, class _Sndr, class _Env>
consteval bool __transform_recurse_nothrow() {
  using _S2 = decltype(::__ycxx::__detail::__exec::__transformed_sndr(_Dom(), _Tag(), std::declval<_Sndr>(), std::declval<const _Env&>()));
  constexpr bool __here = noexcept(::__ycxx::__detail::__exec::__transformed_sndr(_Dom(), _Tag(), std::declval<_Sndr>(), std::declval<const _Env&>()));
  if constexpr (std::is_same_v<std::remove_cvref_t<_S2>, std::remove_cvref_t<_Sndr>>) {
    return __here;
  } else {
    using _Dom2 = std::conditional_t<std::is_same_v<_Tag, std::execution::start_t>, __start_domain_for<_Env>, __completion_domain_for<_S2, _Env>>;
    return __here && ::__ycxx::__detail::__exec::__transform_recurse_nothrow<_Dom2, _Tag, _S2, _Env>();
  }
}
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

// [exec.snd.transform]
template <sender _Sndr, __ycxx::__detail::__exec::__queryable _Env>
constexpr decltype(auto) transform_sender(_Sndr&& __sndr, const _Env& env) noexcept(
    __ycxx::__detail::__exec::__transform_recurse_nothrow<__ycxx::__detail::__exec::__completion_domain_for<_Sndr, _Env>, set_value_t, _Sndr, _Env>() &&
    __ycxx::__detail::__exec::__transform_recurse_nothrow<
        __ycxx::__detail::__exec::__start_domain_for<_Env>, start_t,
        decltype(__ycxx::__detail::__exec::__transform_recurse(__ycxx::__detail::__exec::__completion_domain_for<_Sndr, _Env>(), set_value_t(),
                                                       declval<_Sndr>(), declval<const _Env&>())),
        _Env>()) {
  using namespace __ycxx::__detail::__exec;
  return __transform_recurse(__start_domain_for<_Env>(), start_t(),
                           __transform_recurse(__completion_domain_for<_Sndr, _Env>(), set_value_t(), static_cast<_Sndr&&>(__sndr), env), env);
}

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Domain, class _Tag, class _Sndr, class... _Args>
consteval bool __apply_nothrow() {
  if constexpr (requires { std::declval<_Domain&>().apply_sender(_Tag(), std::declval<_Sndr>(), std::declval<_Args>()...); })
    return noexcept(std::declval<_Domain&>().apply_sender(_Tag(), std::declval<_Sndr>(), std::declval<_Args>()...));
  else
    return noexcept(std::execution::default_domain().apply_sender(_Tag(), std::declval<_Sndr>(), std::declval<_Args>()...));
}
template <class _Domain, class _Tag, class _Sndr, class... _Args>
constexpr decltype(auto) __apply_dispatch(_Domain __dom, _Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(__apply_nothrow<_Domain, _Tag, _Sndr, _Args...>()) {
  if constexpr (requires { __dom.apply_sender(_Tag(), static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...); })
    return __dom.apply_sender(_Tag(), static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...);
  else
    return std::execution::default_domain().apply_sender(_Tag(), static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...);
}
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
template <class _Domain, class _Tag, sender _Sndr, class... _Args>
  requires requires(_Domain __dom, _Sndr&& __sndr, _Args&&... __args) {
    __dom.apply_sender(_Tag(), static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...);
  } || requires(_Sndr&& __sndr, _Args&&... __args) {
    default_domain().apply_sender(_Tag(), static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...);
  }
constexpr decltype(auto) apply_sender(_Domain __dom, _Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(
    __ycxx::__detail::__exec::__apply_nothrow<_Domain, _Tag, _Sndr, _Args...>()) {
  return __ycxx::__detail::__exec::__apply_dispatch(__dom, _Tag(), static_cast<_Sndr&&>(__sndr), static_cast<_Args&&>(__args)...);
}
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// get_completion_signatures ([exec.getcomplsigs]) and the concepts built on it.
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

// The exception get_completion_signatures throws for an invalid sender (the "unspecified-
// exception" of [exec.snd.general]/6 and the "except" of [exec.getcomplsigs]/1).
template <class _What, class... _Info>
struct __completion_signatures_error : std::exception {
  constexpr const char* what() const noexcept override { return "std::execution: invalid sender for the environment"; }
};
template <class... _Info>
struct __completion_signatures_error<__dependent_sigs, _Info...> : std::execution::dependent_sender_error {};

void __sender_type_error_without_exceptions() noexcept; // never defined: makes the call non-constant

template <class _What, class... _Info>
[[noreturn]] consteval void __report_sigs_error() {
  if constexpr (__cfg::exceptions)
    throw __completion_signatures_error<_What, _Info...>();
  else
    ::__ycxx::__detail::__exec::__sender_type_error_without_exceptions();
}

// CHECKED-COMPLSIGS: the completion_signatures of a computation, or the throw for its error.
template <class _Rp>
consteval auto __checked_sigs() {
  if constexpr (__is_csigs<_Rp>) {
    return _Rp();
  } else if constexpr (std::is_same_v<_Rp, __dependent_sigs>) {
    ::__ycxx::__detail::__exec::__report_sigs_error<__dependent_sigs>();
    return std::execution::completion_signatures<>();
  } else {
    []<class _Wp, class... _Ip>(__invalid_sigs<_Wp, _Ip...>*) { ::__ycxx::__detail::__exec::__report_sigs_error<_Wp, _Ip...>(); }(static_cast<_Rp*>(nullptr));
    return std::execution::completion_signatures<>();
  }
}

// The answer of a sender that is not the library's: its static member function
// get_completion_signatures<Sndr, Env...>() when that is a constant expression; an error
// otherwise (dependent if it throws dependent_sender_error; where a constant evaluation cannot
// catch, dependent when no environment was given).
template <class _Sp, class... _Env>
concept __has_member_complsigs = requires { std::remove_reference_t<_Sp>::template get_completion_signatures<_Sp, _Env...>(); };
template <class _Sp, class... _Env>
concept __constant_member_complsigs =
    __has_member_complsigs<_Sp, _Env...> &&
    requires { typename __y_constant<std::remove_reference_t<_Sp>::template get_completion_signatures<_Sp, _Env...>()>; };

template <class _Sp, class... _Env>
consteval int __classify_member_complsigs_error() {
  if constexpr (__cfg::__constexpr_exceptions) {
    try {
      (void)std::remove_reference_t<_Sp>::template get_completion_signatures<_Sp, _Env...>();
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
template <class _Sp, class... _Env>
struct __member_complsigs {
  static auto __pick() {
    if constexpr (__constant_member_complsigs<_Sp, _Env...>) {
      using _Rp = std::remove_cvref_t<decltype(std::remove_reference_t<_Sp>::template get_completion_signatures<_Sp, _Env...>())>;
      if constexpr (__is_csigs<_Rp>)
        return std::type_identity<_Rp>{};
      else
        return std::type_identity<__invalid_sigs<__sender_has_no_completion_signatures, _Sp, _Env...>>{};
    } else if constexpr (__cfg::__constexpr_exceptions &&
                         requires { typename __y_constant<::__ycxx::__detail::__exec::__classify_member_complsigs_error<_Sp, _Env...>()>; }) {
      if constexpr (::__ycxx::__detail::__exec::__classify_member_complsigs_error<_Sp, _Env...>() == 1)
        return std::type_identity<__dependent_sigs>{};
      else
        return std::type_identity<__invalid_sigs<__sender_has_no_completion_signatures, _Sp, _Env...>>{};
    } else if constexpr (sizeof...(_Env) == 0) {
      return std::type_identity<__dependent_sigs>{};
    } else {
      return std::type_identity<__invalid_sigs<__sender_has_no_completion_signatures, _Sp, _Env...>>{};
    }
  }
  using type = typename decltype(__pick())::type;
};

template <class _Sp, class... _Env>
concept __library_sender = requires { typename std::remove_reference_t<_Sp>::template __ycxx_csigs<_Sp, _Env...>; };

// The type NewSndr of [exec.getcomplsigs]/1.
template <class _Sndr, class... _Env>
struct __new_sndr {
  using type = _Sndr;
};
template <class _Sndr, class _Env>
struct __new_sndr<_Sndr, _Env> {
  using type = decltype(std::execution::transform_sender(std::declval<_Sndr>(), std::declval<_Env>()));
};

template <class _Sndr, class... _Env>
struct __csigs_of_impl {
  static auto __pick() {
    if constexpr (sizeof...(_Env) > 1) {
      return std::type_identity<__invalid_sigs<__not_a_sender_for_this_environment, _Sndr, _Env...>>{};
    } else if constexpr (!requires { typename __new_sndr<_Sndr, _Env...>::type; }) {
      return std::type_identity<__invalid_sigs<__transform_sender_ill_formed, _Sndr, _Env...>>{};
    } else {
      using _NS = typename __new_sndr<_Sndr, _Env...>::type;
      using _Xp = std::remove_reference_t<_NS>;
      if constexpr (__library_sender<_NS, _Env...>)
        return std::type_identity<typename _Xp::template __ycxx_csigs<_NS, _Env...>>{};
      else if constexpr (__has_member_complsigs<_NS, _Env...>)
        return std::type_identity<typename __member_complsigs<_NS, _Env...>::type>{};
      else if constexpr (__has_member_complsigs<_NS>)
        return std::type_identity<typename __member_complsigs<_NS>::type>{};
      else if constexpr (requires { typename _Xp::completion_signatures; requires __is_csigs<typename _Xp::completion_signatures>; })
        // The form of [exec.cmplsig]'s example (a member type), which [exec.getcomplsigs] does
        // not list (DECISIONS: <execution>).
        return std::type_identity<typename _Xp::completion_signatures>{};
      else if constexpr (__is_awaitable<_NS, ::__ycxx::__adl_free::__exec_env_promise<_Env>...>)
        return std::type_identity<std::execution::completion_signatures<
            __set_value_sig_t<__await_result_type<_NS, ::__ycxx::__adl_free::__exec_env_promise<_Env>...>>,
            std::execution::set_error_t(std::exception_ptr), std::execution::set_stopped_t()>>{};
      else if constexpr (sizeof...(_Env) == 0)
        return std::type_identity<__dependent_sigs>{};
      else
        return std::type_identity<__invalid_sigs<__not_a_sender_for_this_environment, _Sndr, _Env...>>{};
    }
  }
  using type = typename decltype(__pick())::type;
};
// The completion signatures of Sndr in Env... (or an error type).
template <class _Sndr, class... _Env>
using __csigs_of_t = typename __csigs_of_impl<_Sndr, _Env...>::type;

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <class _Sndr, class... _Env>
  requires(sizeof...(_Env) <= 1)
consteval auto get_completion_signatures() -> __ycxx::__detail::__exec::__valid_completion_signatures auto {
  return __ycxx::__detail::__exec::__checked_sigs<__ycxx::__detail::__exec::__csigs_of_t<_Sndr, _Env...>>();
}

template <class _Sndr, class... _Env>
concept sender_in = sender<_Sndr> && (sizeof...(_Env) <= 1) && (__ycxx::__detail::__exec::__queryable<_Env> && ...) &&
                    __ycxx::__detail::__exec::__is_csigs<__ycxx::__detail::__exec::__csigs_of_t<_Sndr, _Env...>>;

template <class _Sndr>
concept dependent_sender = sender<_Sndr> && is_same_v<__ycxx::__detail::__exec::__csigs_of_t<_Sndr>, __ycxx::__detail::__exec::__dependent_sigs>;

template <class _Sndr, class... _Env>
  requires sender_in<_Sndr, _Env...>
using completion_signatures_of_t = __ycxx::__detail::__exec::__csigs_of_t<_Sndr, _Env...>;

template <class _Sndr, class _Env = env<>, template <class...> class _Tuple = __ycxx::__detail::__exec::__decayed_tuple,
          template <class...> class _Variant = __ycxx::__detail::__exec::__variant_or_empty>
  requires sender_in<_Sndr, _Env>
using value_types_of_t = __ycxx::__detail::__exec::__gather_signatures<set_value_t, completion_signatures_of_t<_Sndr, _Env>, _Tuple, _Variant>;

template <class _Sndr, class _Env = env<>, template <class...> class _Variant = __ycxx::__detail::__exec::__variant_or_empty>
  requires sender_in<_Sndr, _Env>
using error_types_of_t = __ycxx::__detail::__exec::__gather_signatures<set_error_t, completion_signatures_of_t<_Sndr, _Env>, type_identity_t, _Variant>;

template <class _Sndr, class _Env = env<>>
  requires sender_in<_Sndr, _Env>
constexpr bool sends_stopped = __ycxx::__detail::__exec::__sigs_count<set_stopped_t, completion_signatures_of_t<_Sndr, _Env>> != 0;

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// single-sender-value-type<Sndr, Env...> ([execution.syn]/2): from the value completions' argument
// lists, decay_t<T> for one completion with one datum, void for none or one without datums,
// decayed-tuple<Ts...> for one with several, and nothing otherwise.
template <class _ArgLists>
struct __single_value_of {};
template <>
struct __single_value_of<__tlist<>> {
  using type = void;
};
template <>
struct __single_value_of<__tlist<__tlist<>>> {
  using type = void;
};
template <class _Tp>
struct __single_value_of<__tlist<__tlist<_Tp>>> {
  using type = std::decay_t<_Tp>;
};
template <class _T0, class _T1, class... _Ts>
struct __single_value_of<__tlist<__tlist<_T0, _T1, _Ts...>>> {
  using type = __decayed_tuple<_T0, _T1, _Ts...>;
};
template <class _Sndr, class... _Env>
struct __single_sender_value : __single_value_of<__sigs_args_t<std::execution::set_value_t, std::execution::completion_signatures_of_t<_Sndr, _Env...>>> {};
template <class _Sndr, class... _Env>
  requires std::execution::sender_in<_Sndr, _Env...>
using __single_sender_value_type = typename __single_sender_value<_Sndr, _Env...>::type;
// single-sender-value-type, or void where it is ill-formed.
template <class _Sndr, class... _Env>
struct __single_value_or_void {
  using type = void;
};
template <class _Sndr, class... _Env>
  requires requires { typename __single_sender_value<_Sndr, _Env...>::type; }
struct __single_value_or_void<_Sndr, _Env...> {
  using type = typename __single_sender_value<_Sndr, _Env...>::type;
};
template <class _Sndr, class... _Env>
using __single_sender_value_or_void = typename __single_value_or_void<_Sndr, _Env...>::type;
template <class _Sndr, class... _Env>
concept __single_sender = std::execution::sender_in<_Sndr, _Env...> && requires { typename __single_sender_value_type<_Sndr, _Env...>; };

// sender-in-of / sender-of ([exec.snd.concepts]/5)
template <class... _As>
using __value_signature = std::execution::set_value_t(_As...);
template <class _Sndr, class _SetValue, class... _Env>
concept __sender_in_of_impl =
    std::execution::sender_in<_Sndr, _Env...> &&
    __matching_sig<_SetValue, __gather_signatures<std::execution::set_value_t, std::execution::completion_signatures_of_t<_Sndr, _Env...>,
                                             __value_signature, std::type_identity_t>>;
template <class _Sndr, class _Env, class... _Values>
concept __sender_in_of = __sender_in_of_impl<_Sndr, std::execution::set_value_t(_Values...), _Env>;
template <class _Sndr, class... _Values>
concept __sender_of = __sender_in_of_impl<_Sndr, std::execution::set_value_t(_Values...)>;
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// Receiver concepts ([exec.recv.concepts]).
namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <class _Rcvr>
concept receiver = derived_from<typename remove_cvref_t<_Rcvr>::receiver_concept, receiver_tag> &&
                   requires(const remove_cvref_t<_Rcvr>& __rcvr) {
                     { get_env(__rcvr) } -> __ycxx::__detail::__exec::__queryable;
                   } && move_constructible<remove_cvref_t<_Rcvr>> && constructible_from<remove_cvref_t<_Rcvr>, _Rcvr> &&
                   is_nothrow_move_constructible_v<remove_cvref_t<_Rcvr>>;

template <class _Rcvr, class _ChildOp>
concept inlinable_receiver = receiver<_Rcvr> && requires(_ChildOp* __child) {
  { remove_cvref_t<_Rcvr>::__make_receiver_for(__child) } noexcept -> same_as<remove_cvref_t<_Rcvr>>;
};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Rcvr, class _Sig>
inline constexpr bool __valid_completion_for = false;
template <class _Rcvr, class _Tag, class... _As>
inline constexpr bool __valid_completion_for<_Rcvr, _Tag(_As...)> = __callable<_Tag, std::remove_cvref_t<_Rcvr>, _As...>;
template <class _Rcvr, class _CS>
inline constexpr bool __has_completions = false;
template <class _Rcvr, class... _Sigs>
inline constexpr bool __has_completions<_Rcvr, std::execution::completion_signatures<_Sigs...>> = (__valid_completion_for<_Rcvr, _Sigs> && ...);
template <class _Rcvr, class _Completions>
concept __receiver_of = std::execution::receiver<_Rcvr> && __has_completions<_Rcvr, _Completions>;
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// connect ([exec.connect]).
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _DS, class _DR>
struct __exec_connect_awaitable_promise;

template <class _DS, class _DR>
struct __exec_operation_state_task {
  using operation_state_concept = std::execution::operation_state_tag;
  using promise_type = __exec_connect_awaitable_promise<_DS, _DR>;
  explicit __exec_operation_state_task(std::coroutine_handle<> h) noexcept : __coro(h) {}
  __exec_operation_state_task(__exec_operation_state_task&&) = delete;
  ~__exec_operation_state_task() { __coro.destroy(); }
  void start() & noexcept { __coro.resume(); }

private:
  std::coroutine_handle<> __coro;
};

template <class _DS, class _DR>
struct __exec_connect_awaitable_promise : __exec_with_await_transform<__exec_connect_awaitable_promise<_DS, _DR>> {
  __exec_connect_awaitable_promise(_DS&, _DR& r) noexcept : __rcvr(r) {}
  std::suspend_always initial_suspend() noexcept { return {}; }
  [[noreturn]] std::suspend_always final_suspend() noexcept { __builtin_trap(); }
  [[noreturn]] void unhandled_exception() noexcept { __builtin_trap(); }
  [[noreturn]] void return_void() noexcept { __builtin_trap(); }
  std::coroutine_handle<> unhandled_stopped() noexcept {
    std::execution::set_stopped(static_cast<_DR&&>(__rcvr));
    return std::noop_coroutine();
  }
  __exec_operation_state_task<_DS, _DR> get_return_object() noexcept {
    return __exec_operation_state_task<_DS, _DR>{std::coroutine_handle<__exec_connect_awaitable_promise>::from_promise(*this)};
  }
  std::execution::env_of_t<_DR> get_env() const noexcept { return std::execution::get_env(__rcvr); }

private:
  _DR& __rcvr;
};

// suspend-complete(fun, as...)
template <class _Fn>
struct __exec_suspend_complete {
  _Fn __fn;
  static constexpr bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) noexcept { __fn(); }
  [[noreturn]] void await_resume() noexcept { __builtin_unreachable(); }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Fun, class... _Ts>
auto __suspend_complete(_Fun fun, _Ts&&... __as) noexcept {
  auto __fn = [&, fun]() noexcept { fun(static_cast<_Ts&&>(__as)...); };
  return ::__ycxx::__adl_free::__exec_suspend_complete<decltype(__fn)>{__fn};
}

template <class _DS, class _DR>
using __connect_awaitable_value = __await_result_type<_DS, ::__ycxx::__adl_free::__exec_connect_awaitable_promise<_DS, _DR>>;

template <class _DS, class _DR>
using __connect_awaitable_sigs =
    std::execution::completion_signatures<__set_value_sig_t<__connect_awaitable_value<_DS, _DR>>,
                                          std::execution::set_error_t(std::exception_ptr), std::execution::set_stopped_t()>;

// connect-awaitable(sndr, rcvr) ([exec.connect]/5). Its frame is allocated (only awaitables that
// are not senders of their own come here).
template <class _DS, class _DR>
  requires(__receiver_of<_DR, __connect_awaitable_sigs<_DS, _DR>>)
::__ycxx::__adl_free::__exec_operation_state_task<_DS, _DR> __connect_awaitable(_DS __sndr, _DR __rcvr) {
  std::exception_ptr __ep;
  if constexpr (__cfg::exceptions) {
    try {
      if constexpr (std::is_void_v<__connect_awaitable_value<_DS, _DR>>) {
        co_await static_cast<_DS&&>(__sndr);
        co_await ::__ycxx::__detail::__exec::__suspend_complete(std::execution::set_value, static_cast<_DR&&>(__rcvr));
      } else {
        co_await ::__ycxx::__detail::__exec::__suspend_complete(std::execution::set_value, static_cast<_DR&&>(__rcvr),
                                                         co_await static_cast<_DS&&>(__sndr));
      }
    } catch (...) {
      __ep = std::current_exception();
    }
  } else {
    if constexpr (std::is_void_v<__connect_awaitable_value<_DS, _DR>>) {
      co_await static_cast<_DS&&>(__sndr);
      co_await ::__ycxx::__detail::__exec::__suspend_complete(std::execution::set_value, static_cast<_DR&&>(__rcvr));
    } else {
      co_await ::__ycxx::__detail::__exec::__suspend_complete(std::execution::set_value, static_cast<_DR&&>(__rcvr),
                                                       co_await static_cast<_DS&&>(__sndr));
    }
  }
  co_await ::__ycxx::__detail::__exec::__suspend_complete(std::execution::set_error, static_cast<_DR&&>(__rcvr), static_cast<std::exception_ptr&&>(__ep));
}

template <class _Sndr, class _Rcvr>
using __connect_new_sndr_t =
    decltype(std::execution::transform_sender(std::declval<_Sndr>(), std::execution::get_env(std::declval<const std::remove_cvref_t<_Rcvr>&>())));

template <class _Sndr, class _Rcvr>
concept __connect_via_member = requires(_Sndr&& s, _Rcvr&& r) {
  std::execution::transform_sender(static_cast<_Sndr&&>(s), std::execution::get_env(r));
  std::execution::transform_sender(static_cast<_Sndr&&>(s), std::execution::get_env(r)).connect(static_cast<_Rcvr&&>(r));
};
template <class _Sndr, class _Rcvr>
concept __connect_via_awaitable = requires(_Sndr&& s, _Rcvr&& r) {
  std::execution::transform_sender(static_cast<_Sndr&&>(s), std::execution::get_env(r));
  ::__ycxx::__detail::__exec::__connect_awaitable<std::decay_t<__connect_new_sndr_t<_Sndr, _Rcvr>>, std::decay_t<_Rcvr>>(
      std::execution::transform_sender(static_cast<_Sndr&&>(s), std::execution::get_env(r)), static_cast<_Rcvr&&>(r));
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

struct connect_t {
  template <class _Sndr, class _Rcvr>
    requires __ycxx::__detail::__exec::__connect_via_member<_Sndr, _Rcvr> || __ycxx::__detail::__exec::__connect_via_awaitable<_Sndr, _Rcvr>
  constexpr auto operator()(_Sndr&& __sndr, _Rcvr&& __rcvr) const noexcept(__ycxx_nothrow<_Sndr, _Rcvr>()) {
    static_assert(sender_in<_Sndr, env_of_t<_Rcvr>>, "connect: the sender has no completion signatures in the receiver's environment");
    static_assert(__ycxx::__detail::__exec::__receiver_of<_Rcvr, __ycxx::__detail::__exec::__csigs_of_t<_Sndr, env_of_t<_Rcvr>>>,
                  "connect: the receiver cannot accept every completion of the sender");
    if constexpr (__ycxx::__detail::__exec::__connect_via_member<_Sndr, _Rcvr>) {
      using _Rp = decltype(transform_sender(static_cast<_Sndr&&>(__sndr), get_env(__rcvr)).connect(static_cast<_Rcvr&&>(__rcvr)));
      static_assert(operation_state<_Rp>, "connect: the result must be an operation state");
      return transform_sender(static_cast<_Sndr&&>(__sndr), get_env(__rcvr)).connect(static_cast<_Rcvr&&>(__rcvr));
    } else {
      using _DS = decay_t<__ycxx::__detail::__exec::__connect_new_sndr_t<_Sndr, _Rcvr>>;
      return __ycxx::__detail::__exec::__connect_awaitable<_DS, decay_t<_Rcvr>>(transform_sender(static_cast<_Sndr&&>(__sndr), get_env(__rcvr)),
                                                                     static_cast<_Rcvr&&>(__rcvr));
    }
  }

private:
  template <class _Sndr, class _Rcvr>
  static consteval bool __ycxx_nothrow() {
    if constexpr (__ycxx::__detail::__exec::__connect_via_member<_Sndr, _Rcvr>)
      return noexcept(transform_sender(declval<_Sndr>(), get_env(declval<_Rcvr&>())).connect(declval<_Rcvr>()));
    else
      return false;
  }
};
inline constexpr connect_t connect{};

template <class _Sndr, class _Rcvr>
using connect_result_t = decltype(connect(declval<_Sndr>(), declval<_Rcvr>()));

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// sender-to ([exec.snd.concepts])
template <class _Sndr, class _Rcvr>
concept __sender_to = std::execution::sender_in<_Sndr, std::execution::env_of_t<_Rcvr>> &&
                    __receiver_of<_Rcvr, std::execution::completion_signatures_of_t<_Sndr, std::execution::env_of_t<_Rcvr>>> &&
                    requires(_Sndr&& __sndr, _Rcvr&& __rcvr) { std::execution::connect(static_cast<_Sndr&&>(__sndr), static_cast<_Rcvr&&>(__rcvr)); };
}}} // namespace __ycxx::__detail::__exec
