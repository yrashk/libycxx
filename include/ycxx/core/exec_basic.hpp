// libycxx core: the exposition-only machinery the sender algorithms of [exec] are specified
// with ([exec.snd.expos]): product-type, basic-sender, basic-state, basic-receiver,
// basic-operation, connect-all, default-impls and impls-for, make-sender, emplace-from,
// allocator-aware-forward, query-with-default, call-with-default, not-a-sender,
// not-a-scheduler; and the pipe syntax ([exec.adapt.obj]: sender_adaptor_closure).
//
// impls_for<Tag> holds static member functions in place of the draft's lambdas (get_attrs,
// get_env, get_state, start, complete) and an alias template csigs<Sndr, Env...> that computes
// the completion signatures as a type (exec_core.hpp). The algorithms specialize it.
#pragma once

#include <ycxx/core/exec_core.hpp>

// ---------------------------------------------------------------------------------------------
// product-type ([exec.snd.expos]/17): an aggregate of leaves, tuple-like through member get, so
// that structured bindings work (basic-sender derives from it).
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// [[no_unique_address]] only for empty movable types: initializing a potentially-overlapping
// subobject from a prvalue is not a guaranteed elision, and operation states cannot be moved.
template <std::size_t I, class T, bool = std::is_empty_v<T> && std::is_move_constructible_v<T>>
struct exec_leaf {
  T ycxx_v;
};
template <std::size_t I, class T>
struct exec_leaf<I, T, true> {
  [[no_unique_address]] T ycxx_v;
};
template <class Is, class... Ts>
struct exec_product;
template <std::size_t... Is, class... Ts>
struct exec_product<std::index_sequence<Is...>, Ts...> : exec_leaf<Is, Ts>... {
  template <std::size_t I, class Self>
  constexpr decltype(auto) get(this Self&& self) noexcept {
    using L = exec_leaf<I, Ts...[I]>;
    using LR = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const L&, L&>;
    return std::forward_like<Self>(static_cast<LR>(self).ycxx_v);
  }
  template <class Self, class Fn>
    requires requires(Self&& self, Fn&& fn) { static_cast<Fn&&>(fn)(static_cast<Self&&>(self).template get<Is>()...); }
  constexpr decltype(auto) apply(this Self&& self, Fn&& fn) noexcept(
      noexcept(static_cast<Fn&&>(fn)(static_cast<Self&&>(self).template get<Is>()...))) {
    return static_cast<Fn&&>(fn)(static_cast<Self&&>(self).template get<Is>()...);
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class... Ts>
using product_t = ::ycxx::adl_free::exec_product<std::index_sequence_for<Ts...>, Ts...>;

template <class T>
inline constexpr bool is_product = false;
template <class Is, class... Ts>
inline constexpr bool is_product<::ycxx::adl_free::exec_product<Is, Ts...>> = true;

// product-type{ts...}: the elements decay-copied.
template <class... Ts>
constexpr product_t<std::decay_t<Ts>...> make_product(Ts&&... ts) noexcept(
    (std::is_nothrow_constructible_v<std::decay_t<Ts>, Ts> && ...)) {
  return {{static_cast<Ts&&>(ts)}...};
}
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std {
template <class Is, class... Ts>
struct tuple_size<ycxx::adl_free::exec_product<Is, Ts...>> : integral_constant<size_t, sizeof...(Ts)> {};
template <size_t I, class Is, class... Ts>
struct tuple_element<I, ycxx::adl_free::exec_product<Is, Ts...>> {
  using type = Ts...[I];
};
} // namespace std

// ---------------------------------------------------------------------------------------------
// impls-for, basic-sender and friends.
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

template <class Tag>
struct impls_for;

// data-type<Sndr> and child-type<Sndr, I>
template <class Sndr>
using data_type = decltype(std::declval<Sndr>().template get<1>());
template <class Sndr, std::size_t I = 0>
using child_type = decltype(std::declval<Sndr>().template get<I + 2>());
template <class Sndr>
using indices_for = typename std::remove_reference_t<Sndr>::ycxx_indices;

// allocator-aware-forward(obj, context) ([exec.snd.expos]/54)
template <class T, class Context>
constexpr decltype(auto) allocator_aware_forward(T&& obj, Context&& context) {
  if constexpr (requires { std::get_allocator(std::execution::get_env(context)); }) {
    auto alloc = std::get_allocator(std::execution::get_env(context));
    using P = std::remove_cvref_t<T>;
    if constexpr (is_product<P>) {
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return P{{std::make_obj_using_allocator<std::tuple_element_t<Is, P>>(alloc, static_cast<T&&>(obj).template get<Is>())}...};
      }(std::make_index_sequence<std::tuple_size_v<P>>());
    } else {
      return std::make_obj_using_allocator<P>(alloc, static_cast<T&&>(obj));
    }
  } else {
    return static_cast<T&&>(obj);
  }
}

// The completion signatures of every child of Sndr, in the forwarded environment.
template <class Sndr, class Is, class... Env>
struct children_sigs;
template <class Sndr, std::size_t... Is, class... Env>
struct children_sigs<Sndr, std::index_sequence<Is...>, Env...> {
  using type = sigs_concat_t<csigs_of_t<child_type<Sndr, Is>, fwd_env_t<Env>...>...>;
};
template <class Sndr, class... Env>
using children_sigs_t = typename children_sigs<Sndr, indices_for<Sndr>, Env...>::type;

// default-impls ([exec.snd.expos]/35)
struct default_impls {
  // The attributes ([exec.adapt.general]/3.2, 3.3): those of a single child (with its
  // completion queries; an adaptor whose completions change agents defines its own), else none.
  template <class Data, class... Child>
  static constexpr decltype(auto) get_attrs(const Data&, const Child&... child) noexcept {
    if constexpr (sizeof...(Child) == 1)
      return ::ycxx::detail::exec::fwd_env(std::execution::get_env(child...[0]));
    else
      return std::execution::env<>();
  }
  template <class Index, class State, class Rcvr>
  static constexpr decltype(auto) get_env(Index, State&, const Rcvr& rcvr) noexcept {
    return ::ycxx::detail::exec::fwd_env(std::execution::get_env(rcvr));
  }
  template <class Sndr, class Rcvr>
  static constexpr decltype(auto) get_state(Sndr&& sndr, Rcvr& rcvr) noexcept {
    return ::ycxx::detail::exec::allocator_aware_forward(static_cast<Sndr&&>(sndr).template get<1>(), rcvr);
  }
  template <class State, class Rcvr, class... Ops>
  static constexpr void start(State&, Rcvr&, Ops&... ops) noexcept {
    (std::execution::start(ops), ...);
  }
  template <class Index, class State, class Rcvr, class Tag, class... Args>
    requires callable<Tag, Rcvr, Args...>
  static constexpr void complete(Index, State&, Rcvr& rcvr, Tag, Args&&... args) noexcept {
    static_assert(Index::value == 0);
    Tag()(static_cast<Rcvr&&>(rcvr), static_cast<Args&&>(args)...);
  }
  // The signatures: those of the children, forwarded unchanged.
  template <class Sndr, class... Env>
  using csigs = children_sigs_t<Sndr, Env...>;
};

template <class Tag>
struct impls_for : default_impls {};

template <class Sndr>
using tag_t = typename std::remove_cvref_t<Sndr>::ycxx_tag;
template <class Sndr>
using impls_of = impls_for<tag_t<Sndr>>;

template <class Sndr, class Rcvr>
using state_type = std::decay_t<decltype(impls_of<Sndr>::get_state(std::declval<Sndr>(), std::declval<Rcvr&>()))>;
template <class Index, class Sndr, class Rcvr>
using env_type = decltype(impls_of<Sndr>::get_env(Index(), std::declval<state_type<Sndr, Rcvr>&>(), std::declval<const Rcvr&>()));

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// basic-state ([exec.snd.expos]/27)
template <class Sndr, class Rcvr>
struct exec_basic_state {
  using impls = ::ycxx::detail::exec::impls_of<Sndr>;
  using get_state_result = decltype(impls::get_state(std::declval<Sndr>(), std::declval<Rcvr&>()));
  using state_t = ::ycxx::detail::exec::state_type<Sndr, Rcvr>;

  // The state is computed from the stored receiver (the draft's parameter of the same name has
  // been moved from by then).
  constexpr exec_basic_state(Sndr&& sndr, Rcvr&& r) noexcept(
      std::is_nothrow_move_constructible_v<Rcvr> && noexcept(impls::get_state(std::declval<Sndr>(), std::declval<Rcvr&>())) &&
      (std::is_same_v<state_t, get_state_result> || std::is_nothrow_constructible_v<state_t, get_state_result>))
      : rcvr(static_cast<Rcvr&&>(r)), state(impls::get_state(static_cast<Sndr&&>(sndr), rcvr)) {}

  Rcvr rcvr;
  state_t state;
};

// basic-receiver ([exec.snd.expos]/29)
template <class Sndr, class Rcvr, class Index>
  requires ::ycxx::detail::exec::valid_specialization<::ycxx::detail::exec::env_type, Index, Sndr, Rcvr>
struct exec_basic_receiver {
  using receiver_concept = std::execution::receiver_tag;
  using impls = ::ycxx::detail::exec::impls_of<Sndr>;
  using state_t = ::ycxx::detail::exec::state_type<Sndr, Rcvr>;

  template <class... Args>
    requires requires(state_t& s, Rcvr& r, Args&&... args) {
      impls::complete(Index(), s, r, std::execution::set_value_t(), static_cast<Args&&>(args)...);
    }
  constexpr void set_value(Args&&... args) && noexcept {
    impls::complete(Index(), op->state, op->rcvr, std::execution::set_value_t(), static_cast<Args&&>(args)...);
  }
  template <class Error>
    requires requires(state_t& s, Rcvr& r, Error&& e) { impls::complete(Index(), s, r, std::execution::set_error_t(), static_cast<Error&&>(e)); }
  constexpr void set_error(Error&& err) && noexcept {
    impls::complete(Index(), op->state, op->rcvr, std::execution::set_error_t(), static_cast<Error&&>(err));
  }
  constexpr void set_stopped() && noexcept
    requires requires(state_t& s, Rcvr& r) { impls::complete(Index(), s, r, std::execution::set_stopped_t()); }
  {
    impls::complete(Index(), op->state, op->rcvr, std::execution::set_stopped_t());
  }
  constexpr auto get_env() const noexcept -> ::ycxx::detail::exec::env_type<Index, Sndr, Rcvr> {
    return impls::get_env(Index(), op->state, op->rcvr);
  }

  exec_basic_state<Sndr, Rcvr>* op;
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// connect-all ([exec.snd.expos]/30)
template <class Sndr, class Rcvr, std::size_t I>
using child_receiver = ::ycxx::adl_free::exec_basic_receiver<Sndr, Rcvr, std::integral_constant<std::size_t, I>>;

template <class Sndr, class Rcvr, class Is>
struct connect_all_result_impl;
template <class Sndr, class Rcvr, std::size_t... Is>
  requires(requires { typename std::execution::connect_result_t<child_type<Sndr, Is>, child_receiver<Sndr, Rcvr, Is>>; } && ...)
struct connect_all_result_impl<Sndr, Rcvr, std::index_sequence<Is...>> {
  using type = product_t<std::execution::connect_result_t<child_type<Sndr, Is>, child_receiver<Sndr, Rcvr, Is>>...>;
  static constexpr bool nothrow =
      (std::is_nothrow_invocable_v<std::execution::connect_t, child_type<Sndr, Is>, child_receiver<Sndr, Rcvr, Is>> && ...);
};
template <class Sndr, class Rcvr>
using connect_all_result = typename connect_all_result_impl<Sndr, Rcvr, indices_for<Sndr>>::type;

template <class Sndr, class Rcvr, std::size_t... Is>
constexpr auto connect_all(::ycxx::adl_free::exec_basic_state<Sndr, Rcvr>* op, Sndr&& sndr, std::index_sequence<Is...>) noexcept(
    connect_all_result_impl<Sndr, Rcvr, std::index_sequence<Is...>>::nothrow) -> connect_all_result<Sndr, Rcvr> {
  return {{std::execution::connect(static_cast<Sndr&&>(sndr).template get<Is + 2>(), child_receiver<Sndr, Rcvr, Is>{op})}...};
}
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// basic-operation ([exec.snd.expos]/33)
template <class Sndr, class Rcvr>
  requires ::ycxx::detail::exec::valid_specialization<::ycxx::detail::exec::state_type, Sndr, Rcvr> &&
           ::ycxx::detail::exec::valid_specialization<::ycxx::detail::exec::connect_all_result, Sndr, Rcvr>
struct exec_basic_operation : exec_basic_state<Sndr, Rcvr> {
  using operation_state_concept = std::execution::operation_state_tag;
  using impls = ::ycxx::detail::exec::impls_of<Sndr>;
  using indices = ::ycxx::detail::exec::indices_for<Sndr>;

  ::ycxx::detail::exec::connect_all_result<Sndr, Rcvr> inner_ops;

  constexpr exec_basic_operation(Sndr&& sndr, Rcvr&& rcvr) noexcept(
      std::is_nothrow_constructible_v<exec_basic_state<Sndr, Rcvr>, Sndr, Rcvr> &&
      ::ycxx::detail::exec::connect_all_result_impl<Sndr, Rcvr, indices>::nothrow)
      : exec_basic_state<Sndr, Rcvr>(static_cast<Sndr&&>(sndr), static_cast<Rcvr&&>(rcvr)),
        inner_ops(::ycxx::detail::exec::connect_all(this, static_cast<Sndr&&>(sndr), indices())) {}
  exec_basic_operation(exec_basic_operation&&) = delete;

  constexpr void start() & noexcept {
    [this]<std::size_t... Is>(std::index_sequence<Is...>) {
      impls::start(this->state, this->rcvr, inner_ops.template get<Is>()...);
    }(indices());
  }
};

// basic-sender ([exec.snd.expos]/43)
template <class Tag, class Data, class... Child>
struct exec_basic_sender : exec_product<std::index_sequence_for<Tag, Data, Child...>, Tag, Data, Child...> {
  using sender_concept = std::execution::sender_tag;
  using ycxx_tag = Tag;
  using ycxx_indices = std::index_sequence_for<Child...>;

  constexpr decltype(auto) get_env() const noexcept {
    return [this]<std::size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
      return ::ycxx::detail::exec::impls_for<Tag>::get_attrs(this->template get<1>(), this->template get<Is + 2>()...);
    }(ycxx_indices());
  }

  template <::ycxx::detail::exec::decays_to<exec_basic_sender> Self, std::execution::receiver Rcvr>
  constexpr auto connect(this Self&& self, Rcvr rcvr) noexcept(std::is_nothrow_constructible_v<exec_basic_operation<Self, Rcvr>, Self, Rcvr>)
      -> exec_basic_operation<Self, Rcvr> {
    return {static_cast<Self&&>(self), static_cast<Rcvr&&>(rcvr)};
  }

  template <class Self, class... Env>
  using ycxx_csigs = typename ::ycxx::detail::exec::impls_for<Tag>::template csigs<Self, Env...>;

  // sndr.affine() ([exec.snd.expos]/1, [exec.affine]/6) for the senders that complete on the agent
  // that starts them.
  template <class Self>
    requires requires { requires ::ycxx::detail::exec::impls_for<Tag>::ycxx_completes_inline; }
  constexpr std::remove_cvref_t<Self> affine(this Self&& self) noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<Self>, Self>) {
    return static_cast<Self&&>(self);
  }

  template <::ycxx::detail::exec::decays_to<exec_basic_sender> Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ::ycxx::detail::exec::checked_sigs<ycxx_csigs<Self, Env...>>();
  }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {
template <class Tag, class Data, class... Child>
struct tuple_size<ycxx::adl_free::exec_basic_sender<Tag, Data, Child...>> : integral_constant<size_t, sizeof...(Child) + 2> {};
template <size_t I, class Tag, class Data, class... Child>
struct tuple_element<I, ycxx::adl_free::exec_basic_sender<Tag, Data, Child...>> {
  using type = tuple_element_t<I, ycxx::adl_free::exec_product<index_sequence_for<Tag, Data, Child...>, Tag, Data, Child...>>;
};
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

template <class Tag, class Data, class... Child>
using basic_sender_t = ::ycxx::adl_free::exec_basic_sender<Tag, Data, Child...>;

// make-sender(tag, data, child...) ([exec.snd.expos]/23)
template <class Tag, class Data = empty_data, class... Child>
constexpr auto make_sender(Tag tag, Data&& data, Child&&... child) noexcept(
    std::is_nothrow_constructible_v<std::decay_t<Data>, Data> && (std::is_nothrow_constructible_v<std::decay_t<Child>, Child> && ...)) {
  static_assert(std::semiregular<Tag>);
  static_assert(movable_value<Data>);
  static_assert((std::execution::sender<Child> && ...));
  using S = basic_sender_t<Tag, std::decay_t<Data>, std::decay_t<Child>...>;
  static_assert(!is_invalid_sigs<csigs_of_t<S>>,
                "the sender cannot complete in any environment (see the invalid_sigs<problem, ...> type in this diagnostic)");
  return S{{{tag}, {static_cast<Data&&>(data)}, {static_cast<Child&&>(child)}...}};
}
template <class Tag>
constexpr auto make_sender(Tag tag) {
  return ::ycxx::detail::exec::make_sender(tag, empty_data());
}

// The completion signatures of a basic-sender's children computed with and without the
// (forwarded) environment, and the failure checks the algorithms' check-types perform.
template <class Sndr, class... Env>
using child_sigs_t = csigs_of_t<child_type<Sndr>, fwd_env_t<Env>...>;

// decay-copyable-result-datums(cs)
template <class Sig>
inline constexpr bool decay_copyable_sig = false;
template <class Tag, class... Ts>
inline constexpr bool decay_copyable_sig<Tag(Ts...)> = (std::is_constructible_v<std::decay_t<Ts>, Ts> && ...);
template <class Sig>
inline constexpr bool nothrow_decay_copy_sig = false;
template <class Tag, class... Ts>
inline constexpr bool nothrow_decay_copy_sig<Tag(Ts...)> = (std::is_nothrow_constructible_v<std::decay_t<Ts>, Ts> && ...);
template <class CS>
inline constexpr bool decay_copyable_sigs = false;
template <class... Sigs>
inline constexpr bool decay_copyable_sigs<std::execution::completion_signatures<Sigs...>> = (decay_copyable_sig<Sigs> && ...);
template <class CS>
inline constexpr bool nothrow_decay_copy_sigs = false;
template <class... Sigs>
inline constexpr bool nothrow_decay_copy_sigs<std::execution::completion_signatures<Sigs...>> = (nothrow_decay_copy_sig<Sigs> && ...);

// A signature with its arguments decayed (what is sent after a decay-copy).
template <class Sig>
struct decayed_sig;
template <class Tag, class... Ts>
struct decayed_sig<Tag(Ts...)> {
  using type = std::execution::completion_signatures<Tag(std::decay_t<Ts>...)>;
};
template <class Sig>
using decayed_sig_t = typename decayed_sig<Sig>::type;

using eptr_sigs = std::execution::completion_signatures<std::execution::set_error_t(std::exception_ptr)>;
using no_sigs = std::execution::completion_signatures<>;

// emplace-from ([exec.snd.expos]/15)
template <class Fun>
  requires std::is_nothrow_move_constructible_v<Fun>
struct emplace_from {
  Fun fun;
  using type = call_result_t<Fun>;
  constexpr operator type() && noexcept(nothrow_callable<Fun>) { return static_cast<Fun&&>(fun)(); }
  constexpr type operator()() && noexcept(nothrow_callable<Fun>) { return static_cast<Fun&&>(fun)(); }
};
template <class Fun>
emplace_from(Fun) -> emplace_from<Fun>;

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// A variant of operation states (which cannot be moved): emplace constructs the alternative from
// the prvalue a function returns (guaranteed elision, which emplace-from cannot give through
// variant::emplace's direct-initialization). Index 0 is the empty state.
template <class... Ts>
class exec_op_variant {
  alignas(Ts...) unsigned char buf_[::ycxx::detail::exec::max_size<Ts...>];
  unsigned char index_ = 0; // 0: empty, i + 1: Ts...[i]

  template <class T>
  static constexpr unsigned char index_of = static_cast<unsigned char>(::ycxx::detail::exec::first_true<std::is_same_v<T, Ts>...> + 1);

  void destroy() noexcept {
    if (index_ != 0) {
      [this]<std::size_t... Is>(std::index_sequence<Is...>) {
        (void)((index_ == Is + 1 ? (std::launder(reinterpret_cast<Ts...[Is]*>(buf_))->~Ts...[Is](), true) : false) || ...);
      }(std::index_sequence_for<Ts...>());
      index_ = 0;
    }
  }

public:
  exec_op_variant() noexcept = default;
  exec_op_variant(exec_op_variant&&) = delete;
  ~exec_op_variant() { destroy(); }

  void reset() noexcept { destroy(); }
  template <class T, class F>
  T& emplace_from_fn(F&& f) noexcept(noexcept(static_cast<F&&>(f)())) {
    destroy();
    T* p = ::new (static_cast<void*>(buf_)) T(static_cast<F&&>(f)());
    index_ = index_of<T>;
    return *p;
  }
  template <class T>
  T& get() noexcept {
    return *std::launder(reinterpret_cast<T*>(buf_));
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

// query-with-default(tag, env, value) ([exec.snd.expos]/11)
template <class Tag, class Env, class Default>
consteval bool query_with_default_nothrow() {
  if constexpr (requires(const Env& env) { Tag()(env); })
    return noexcept(Tag()(std::declval<const Env&>()));
  else
    return noexcept(static_cast<Default>(std::declval<Default>()));
}
template <class Tag, class Env, class Default>
constexpr decltype(auto) query_with_default(Tag, const Env& env, Default&& value) noexcept(query_with_default_nothrow<Tag, Env, Default>()) {
  if constexpr (requires { Tag()(env); })
    return Tag()(env);
  else
    return static_cast<Default>(static_cast<Default&&>(value));
}

// call-with-default(fn, value, args...) ([exec.snd.expos]/55)
template <class Fn, class Default, class... Args>
constexpr decltype(auto) call_with_default(Fn&& fn, Default&& value, Args&&... args) {
  if constexpr (requires { static_cast<Fn&&>(fn)(static_cast<Args&&>(args)...); })
    return static_cast<Fn&&>(fn)(static_cast<Args&&>(args)...);
  else
    return static_cast<Default>(static_cast<Default&&>(value));
}

// SET-VALUE(rcvr, expr), TRY-EVAL, TRY-SET-VALUE ([exec.snd.expos]/11): the callable form
// takes a function computing expr.
template <class Rcvr, class F>
constexpr void set_value_of(Rcvr& rcvr, F&& f) noexcept(noexcept(static_cast<F&&>(f)())) {
  if constexpr (std::is_void_v<decltype(static_cast<F&&>(f)())>) {
    static_cast<F&&>(f)();
    std::execution::set_value(static_cast<Rcvr&&>(rcvr));
  } else {
    std::execution::set_value(static_cast<Rcvr&&>(rcvr), static_cast<F&&>(f)());
  }
}
template <class Rcvr, class F>
constexpr void try_eval(Rcvr& rcvr, F&& f) noexcept {
  if constexpr (noexcept(static_cast<F&&>(f)())) {
    static_cast<F&&>(f)();
  } else if constexpr (cfg::exceptions) {
    try {
      static_cast<F&&>(f)();
    } catch (...) {
      std::execution::set_error(static_cast<Rcvr&&>(rcvr), std::current_exception());
    }
  } else {
    static_cast<F&&>(f)();
  }
}
template <class Rcvr, class F>
constexpr void try_set_value(Rcvr& rcvr, F&& f) noexcept {
  ::ycxx::detail::exec::try_eval(rcvr, [&]() noexcept(noexcept(::ycxx::detail::exec::set_value_of(rcvr, static_cast<F&&>(f)))) {
    ::ycxx::detail::exec::set_value_of(rcvr, static_cast<F&&>(f));
  });
}

// overload-set ([exec.snd.expos]/50)
template <class... Fns>
struct overload_set : Fns... {
  using Fns::operator()...;
};
template <class... Fns>
overload_set(Fns...) -> overload_set<Fns...>;

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// not-a-sender ([exec.snd.expos]/51): its completion signatures are an error.
struct exec_not_a_sender {
  using sender_concept = std::execution::sender_tag;
  template <class Self, class... Env>
  using ycxx_csigs = ::ycxx::detail::exec::invalid_sigs<::ycxx::detail::exec::not_a_sender_for_this_environment, Self, Env...>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ::ycxx::detail::exec::checked_sigs<ycxx_csigs<Self, Env...>>();
  }
};
// not-a-scheduler ([exec.snd.expos]/52)
struct exec_not_a_scheduler {
  using scheduler_concept = std::execution::scheduler_tag;
  constexpr auto schedule() const noexcept { return exec_not_a_sender(); }
};
}} // namespace ycxx::adl_free

// ---------------------------------------------------------------------------------------------
// Sender adaptor closure objects ([exec.adapt.obj]).
namespace [[gnu::visibility("hidden")]] std { namespace execution {
template <ycxx::detail::exec::class_type D>
struct sender_adaptor_closure {};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class D>
void closure_base_probe(const std::execution::sender_adaptor_closure<D>&);
// A pipeable sender adaptor closure object ([exec.adapt.obj]/2): derived from exactly one
// sender_adaptor_closure specialization, that of its own type, and not a sender.
template <class T>
concept pipeable_closure = requires(const std::remove_cvref_t<T>& t) { ::ycxx::detail::exec::closure_base_probe(t); } &&
                           std::derived_from<std::remove_cvref_t<T>, std::execution::sender_adaptor_closure<std::remove_cvref_t<T>>> &&
                           (!std::execution::sender<std::remove_cvref_t<T>>);
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// c | d: a perfect forwarding call wrapper calling d2(c2(arg)).
template <class C, class D>
struct exec_composed_closure : std::execution::sender_adaptor_closure<exec_composed_closure<C, D>> {
  [[no_unique_address]] D d2;
  [[no_unique_address]] C c2;
  template <class Self, class Sndr>
  constexpr auto operator()(this Self&& self, Sndr&& s) noexcept(
      noexcept(std::forward_like<Self>(self.d2)(std::forward_like<Self>(self.c2)(static_cast<Sndr&&>(s)))))
      -> decltype(std::forward_like<Self>(self.d2)(std::forward_like<Self>(self.c2)(static_cast<Sndr&&>(s)))) {
    return std::forward_like<Self>(self.d2)(std::forward_like<Self>(self.c2)(static_cast<Sndr&&>(s)));
  }
};
// adaptor(args...): a perfect forwarding call wrapper calling adaptor(sndr, bound_args...).
template <class Adaptor, class... Bound>
struct exec_bound_closure : std::execution::sender_adaptor_closure<exec_bound_closure<Adaptor, Bound...>> {
  [[no_unique_address]] Adaptor adaptor;
  ::ycxx::detail::exec::product_t<Bound...> bound;

  template <class Self, class Sndr, std::size_t... Is>
  static constexpr auto ycxx_call(Self&& self, Sndr&& s, std::index_sequence<Is...>) noexcept(
      noexcept(std::forward_like<Self>(self.adaptor)(static_cast<Sndr&&>(s), static_cast<Self&&>(self).bound.template get<Is>()...)))
      -> decltype(std::forward_like<Self>(self.adaptor)(static_cast<Sndr&&>(s), static_cast<Self&&>(self).bound.template get<Is>()...)) {
    return std::forward_like<Self>(self.adaptor)(static_cast<Sndr&&>(s), static_cast<Self&&>(self).bound.template get<Is>()...);
  }
  template <class Self, class Sndr>
  constexpr auto operator()(this Self&& self, Sndr&& s) noexcept(
      noexcept(ycxx_call(static_cast<Self&&>(self), static_cast<Sndr&&>(s), std::index_sequence_for<Bound...>())))
      -> decltype(ycxx_call(static_cast<Self&&>(self), static_cast<Sndr&&>(s), std::index_sequence_for<Bound...>())) {
    return ycxx_call(static_cast<Self&&>(self), static_cast<Sndr&&>(s), std::index_sequence_for<Bound...>());
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Adaptor, class... Args>
constexpr auto bind_closure(Adaptor adaptor, Args&&... args) {
  return ::ycxx::adl_free::exec_bound_closure<Adaptor, std::decay_t<Args>...>{{}, adaptor, {{static_cast<Args&&>(args)}...}};
}
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
template <class Sndr, class Closure>
  requires sender<Sndr> && ycxx::detail::exec::pipeable_closure<Closure> && ycxx::detail::exec::callable<Closure, Sndr>
constexpr decltype(auto) operator|(Sndr&& sndr, Closure&& c) noexcept(ycxx::detail::exec::nothrow_callable<Closure, Sndr>) {
  return static_cast<Closure&&>(c)(static_cast<Sndr&&>(sndr));
}
template <class C, class D>
  requires ycxx::detail::exec::pipeable_closure<C> && ycxx::detail::exec::pipeable_closure<D> &&
           constructible_from<decay_t<C>, C> && constructible_from<decay_t<D>, D>
constexpr auto operator|(C&& c, D&& d) noexcept(is_nothrow_constructible_v<decay_t<C>, C> && is_nothrow_constructible_v<decay_t<D>, D>) {
  return ycxx::adl_free::exec_composed_closure<decay_t<C>, decay_t<D>>{{}, static_cast<D&&>(d), static_cast<C&&>(c)};
}
}} // namespace std::execution
