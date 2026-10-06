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
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// [[no_unique_address]] only for empty movable types: initializing a potentially-overlapping
// subobject from a prvalue is not a guaranteed elision, and operation states cannot be moved.
template <std::size_t _Ip, class _Tp, bool = std::is_empty_v<_Tp> && std::is_move_constructible_v<_Tp>>
struct __exec_leaf {
  _Tp __ycxx_v;
};
template <std::size_t _Ip, class _Tp>
struct __exec_leaf<_Ip, _Tp, true> {
  [[no_unique_address]] _Tp __ycxx_v;
};
template <class _Is, class... _Ts>
struct __exec_product;
template <std::size_t... _Is, class... _Ts>
struct __exec_product<std::index_sequence<_Is...>, _Ts...> : __exec_leaf<_Is, _Ts>... {
  template <std::size_t _Ip, class _Self>
  constexpr decltype(auto) get(this _Self&& __self) noexcept {
    using _Lp = __exec_leaf<_Ip, _Ts...[_Ip]>;
    using _LR = std::conditional_t<std::is_const_v<std::remove_reference_t<_Self>>, const _Lp&, _Lp&>;
    return std::forward_like<_Self>(static_cast<_LR>(__self).__ycxx_v);
  }
  template <class _Self, class _Fn>
    requires requires(_Self&& __self, _Fn&& __fn) { static_cast<_Fn&&>(__fn)(static_cast<_Self&&>(__self).template get<_Is>()...); }
  constexpr decltype(auto) apply(this _Self&& __self, _Fn&& __fn) noexcept(
      noexcept(static_cast<_Fn&&>(__fn)(static_cast<_Self&&>(__self).template get<_Is>()...))) {
    return static_cast<_Fn&&>(__fn)(static_cast<_Self&&>(__self).template get<_Is>()...);
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class... _Ts>
using __product_t = ::__ycxx::__adl_free::__exec_product<std::index_sequence_for<_Ts...>, _Ts...>;

template <class _Tp>
inline constexpr bool __is_product = false;
template <class _Is, class... _Ts>
inline constexpr bool __is_product<::__ycxx::__adl_free::__exec_product<_Is, _Ts...>> = true;

// product-type{ts...}: the elements decay-copied.
template <class... _Ts>
constexpr __product_t<std::decay_t<_Ts>...> __make_product(_Ts&&... __ts) noexcept(
    (std::is_nothrow_constructible_v<std::decay_t<_Ts>, _Ts> && ...)) {
  return {{static_cast<_Ts&&>(__ts)}...};
}
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Is, class... _Ts>
struct tuple_size<__ycxx::__adl_free::__exec_product<_Is, _Ts...>> : integral_constant<size_t, sizeof...(_Ts)> {};
template <size_t _Ip, class _Is, class... _Ts>
struct tuple_element<_Ip, __ycxx::__adl_free::__exec_product<_Is, _Ts...>> {
  using type = _Ts...[_Ip];
};
} // namespace std

// ---------------------------------------------------------------------------------------------
// impls-for, basic-sender and friends.
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

template <class _Tag>
struct __impls_for;

// data-type<Sndr> and child-type<Sndr, I>
template <class _Sndr>
using __data_type = decltype(std::declval<_Sndr>().template get<1>());
template <class _Sndr, std::size_t _Ip = 0>
using __child_type = decltype(std::declval<_Sndr>().template get<_Ip + 2>());
template <class _Sndr>
using __indices_for = typename std::remove_reference_t<_Sndr>::__ycxx_indices;

// allocator-aware-forward(obj, context) ([exec.snd.expos]/54)
template <class _Tp, class _Context>
constexpr decltype(auto) __allocator_aware_forward(_Tp&& __obj, _Context&& __context) {
  if constexpr (requires { std::get_allocator(std::execution::get_env(__context)); }) {
    auto __alloc = std::get_allocator(std::execution::get_env(__context));
    using _Pp = std::remove_cvref_t<_Tp>;
    if constexpr (__is_product<_Pp>) {
      return [&]<std::size_t... _Is>(std::index_sequence<_Is...>) {
        return _Pp{{std::make_obj_using_allocator<std::tuple_element_t<_Is, _Pp>>(__alloc, static_cast<_Tp&&>(__obj).template get<_Is>())}...};
      }(std::make_index_sequence<std::tuple_size_v<_Pp>>());
    } else {
      return std::make_obj_using_allocator<_Pp>(__alloc, static_cast<_Tp&&>(__obj));
    }
  } else {
    return static_cast<_Tp&&>(__obj);
  }
}

// The completion signatures of every child of Sndr, in the forwarded environment.
template <class _Sndr, class _Is, class... _Env>
struct __children_sigs;
template <class _Sndr, std::size_t... _Is, class... _Env>
struct __children_sigs<_Sndr, std::index_sequence<_Is...>, _Env...> {
  using type = __sigs_concat_t<__csigs_of_t<__child_type<_Sndr, _Is>, __fwd_env_t<_Env>...>...>;
};
template <class _Sndr, class... _Env>
using __children_sigs_t = typename __children_sigs<_Sndr, __indices_for<_Sndr>, _Env...>::type;

// default-impls ([exec.snd.expos]/35)
struct __default_impls {
  // The attributes ([exec.adapt.general]/3.2, 3.3): those of a single child (with its
  // completion queries; an adaptor whose completions change agents defines its own), else none.
  template <class _Data, class... _Child>
  static constexpr decltype(auto) __get_attrs(const _Data&, const _Child&... __child) noexcept {
    if constexpr (sizeof...(_Child) == 1)
      return ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(__child...[0]));
    else
      return std::execution::env<>();
  }
  template <class _Index, class _State, class _Rcvr>
  static constexpr decltype(auto) get_env(_Index, _State&, const _Rcvr& __rcvr) noexcept {
    return ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(__rcvr));
  }
  template <class _Sndr, class _Rcvr>
  static constexpr decltype(auto) __get_state(_Sndr&& __sndr, _Rcvr& __rcvr) noexcept {
    return ::__ycxx::__detail::__exec::__allocator_aware_forward(static_cast<_Sndr&&>(__sndr).template get<1>(), __rcvr);
  }
  template <class _State, class _Rcvr, class... _Ops>
  static constexpr void start(_State&, _Rcvr&, _Ops&... __ops) noexcept {
    (std::execution::start(__ops), ...);
  }
  template <class _Index, class _State, class _Rcvr, class _Tag, class... _Args>
    requires __callable<_Tag, _Rcvr, _Args...>
  static constexpr void complete(_Index, _State&, _Rcvr& __rcvr, _Tag, _Args&&... __args) noexcept {
    static_assert(_Index::value == 0);
    _Tag()(static_cast<_Rcvr&&>(__rcvr), static_cast<_Args&&>(__args)...);
  }
  // The signatures: those of the children, forwarded unchanged.
  template <class _Sndr, class... _Env>
  using __csigs = __children_sigs_t<_Sndr, _Env...>;
};

template <class _Tag>
struct __impls_for : __default_impls {};

template <class _Sndr>
using __tag_t = typename std::remove_cvref_t<_Sndr>::__ycxx_tag;
template <class _Sndr>
using __impls_of = __impls_for<__tag_t<_Sndr>>;

template <class _Sndr, class _Rcvr>
using state_type = std::decay_t<decltype(__impls_of<_Sndr>::__get_state(std::declval<_Sndr>(), std::declval<_Rcvr&>()))>;
template <class _Index, class _Sndr, class _Rcvr>
using __env_type = decltype(__impls_of<_Sndr>::get_env(_Index(), std::declval<state_type<_Sndr, _Rcvr>&>(), std::declval<const _Rcvr&>()));

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// basic-state ([exec.snd.expos]/27)
template <class _Sndr, class _Rcvr>
struct __exec_basic_state {
  using __impls = ::__ycxx::__detail::__exec::__impls_of<_Sndr>;
  using __get_state_result = decltype(__impls::__get_state(std::declval<_Sndr>(), std::declval<_Rcvr&>()));
  using __state_t = ::__ycxx::__detail::__exec::state_type<_Sndr, _Rcvr>;

  // The state is computed from the stored receiver (the draft's parameter of the same name has
  // been moved from by then).
  constexpr __exec_basic_state(_Sndr&& __sndr, _Rcvr&& r) noexcept(
      std::is_nothrow_move_constructible_v<_Rcvr> && noexcept(__impls::__get_state(std::declval<_Sndr>(), std::declval<_Rcvr&>())) &&
      (std::is_same_v<__state_t, __get_state_result> || std::is_nothrow_constructible_v<__state_t, __get_state_result>))
      : __rcvr(static_cast<_Rcvr&&>(r)), state(__impls::__get_state(static_cast<_Sndr&&>(__sndr), __rcvr)) {}

  _Rcvr __rcvr;
  __state_t state;
};

// basic-receiver ([exec.snd.expos]/29)
template <class _Sndr, class _Rcvr, class _Index>
  requires ::__ycxx::__detail::__exec::__valid_specialization<::__ycxx::__detail::__exec::__env_type, _Index, _Sndr, _Rcvr>
struct __exec_basic_receiver {
  using receiver_concept = std::execution::receiver_tag;
  using __impls = ::__ycxx::__detail::__exec::__impls_of<_Sndr>;
  using __state_t = ::__ycxx::__detail::__exec::state_type<_Sndr, _Rcvr>;

  template <class... _Args>
    requires requires(__state_t& s, _Rcvr& r, _Args&&... __args) {
      __impls::complete(_Index(), s, r, std::execution::set_value_t(), static_cast<_Args&&>(__args)...);
    }
  constexpr void set_value(_Args&&... __args) && noexcept {
    __impls::complete(_Index(), op->state, op->__rcvr, std::execution::set_value_t(), static_cast<_Args&&>(__args)...);
  }
  template <class _Error>
    requires requires(__state_t& s, _Rcvr& r, _Error&& e) { __impls::complete(_Index(), s, r, std::execution::set_error_t(), static_cast<_Error&&>(e)); }
  constexpr void set_error(_Error&& __err) && noexcept {
    __impls::complete(_Index(), op->state, op->__rcvr, std::execution::set_error_t(), static_cast<_Error&&>(__err));
  }
  constexpr void set_stopped() && noexcept
    requires requires(__state_t& s, _Rcvr& r) { __impls::complete(_Index(), s, r, std::execution::set_stopped_t()); }
  {
    __impls::complete(_Index(), op->state, op->__rcvr, std::execution::set_stopped_t());
  }
  constexpr auto get_env() const noexcept -> ::__ycxx::__detail::__exec::__env_type<_Index, _Sndr, _Rcvr> {
    return __impls::get_env(_Index(), op->state, op->__rcvr);
  }

  __exec_basic_state<_Sndr, _Rcvr>* op;
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// connect-all ([exec.snd.expos]/30)
template <class _Sndr, class _Rcvr, std::size_t _Ip>
using __child_receiver = ::__ycxx::__adl_free::__exec_basic_receiver<_Sndr, _Rcvr, std::integral_constant<std::size_t, _Ip>>;

template <class _Sndr, class _Rcvr, class _Is>
struct __connect_all_result_impl;
template <class _Sndr, class _Rcvr, std::size_t... _Is>
  requires(requires { typename std::execution::connect_result_t<__child_type<_Sndr, _Is>, __child_receiver<_Sndr, _Rcvr, _Is>>; } && ...)
struct __connect_all_result_impl<_Sndr, _Rcvr, std::index_sequence<_Is...>> {
  using type = __product_t<std::execution::connect_result_t<__child_type<_Sndr, _Is>, __child_receiver<_Sndr, _Rcvr, _Is>>...>;
  static constexpr bool nothrow =
      (std::is_nothrow_invocable_v<std::execution::connect_t, __child_type<_Sndr, _Is>, __child_receiver<_Sndr, _Rcvr, _Is>> && ...);
};
template <class _Sndr, class _Rcvr>
using __connect_all_result = typename __connect_all_result_impl<_Sndr, _Rcvr, __indices_for<_Sndr>>::type;

template <class _Sndr, class _Rcvr, std::size_t... _Is>
constexpr auto __connect_all(::__ycxx::__adl_free::__exec_basic_state<_Sndr, _Rcvr>* op, _Sndr&& __sndr, std::index_sequence<_Is...>) noexcept(
    __connect_all_result_impl<_Sndr, _Rcvr, std::index_sequence<_Is...>>::nothrow) -> __connect_all_result<_Sndr, _Rcvr> {
  return {{std::execution::connect(static_cast<_Sndr&&>(__sndr).template get<_Is + 2>(), __child_receiver<_Sndr, _Rcvr, _Is>{op})}...};
}
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// basic-operation ([exec.snd.expos]/33)
template <class _Sndr, class _Rcvr>
  requires ::__ycxx::__detail::__exec::__valid_specialization<::__ycxx::__detail::__exec::state_type, _Sndr, _Rcvr> &&
           ::__ycxx::__detail::__exec::__valid_specialization<::__ycxx::__detail::__exec::__connect_all_result, _Sndr, _Rcvr>
struct __exec_basic_operation : __exec_basic_state<_Sndr, _Rcvr> {
  using operation_state_concept = std::execution::operation_state_tag;
  using __impls = ::__ycxx::__detail::__exec::__impls_of<_Sndr>;
  using indices = ::__ycxx::__detail::__exec::__indices_for<_Sndr>;

  ::__ycxx::__detail::__exec::__connect_all_result<_Sndr, _Rcvr> __inner_ops;

  constexpr __exec_basic_operation(_Sndr&& __sndr, _Rcvr&& __rcvr) noexcept(
      std::is_nothrow_constructible_v<__exec_basic_state<_Sndr, _Rcvr>, _Sndr, _Rcvr> &&
      ::__ycxx::__detail::__exec::__connect_all_result_impl<_Sndr, _Rcvr, indices>::nothrow)
      : __exec_basic_state<_Sndr, _Rcvr>(static_cast<_Sndr&&>(__sndr), static_cast<_Rcvr&&>(__rcvr)),
        __inner_ops(::__ycxx::__detail::__exec::__connect_all(this, static_cast<_Sndr&&>(__sndr), indices())) {}
  __exec_basic_operation(__exec_basic_operation&&) = delete;

  constexpr void start() & noexcept {
    [this]<std::size_t... _Is>(std::index_sequence<_Is...>) {
      __impls::start(this->state, this->__rcvr, __inner_ops.template get<_Is>()...);
    }(indices());
  }
};

// basic-sender ([exec.snd.expos]/43)
template <class _Tag, class _Data, class... _Child>
struct __exec_basic_sender : __exec_product<std::index_sequence_for<_Tag, _Data, _Child...>, _Tag, _Data, _Child...> {
  using sender_concept = std::execution::sender_tag;
  using __ycxx_tag = _Tag;
  using __ycxx_indices = std::index_sequence_for<_Child...>;

  constexpr decltype(auto) get_env() const noexcept {
    return [this]<std::size_t... _Is>(std::index_sequence<_Is...>) -> decltype(auto) {
      return ::__ycxx::__detail::__exec::__impls_for<_Tag>::__get_attrs(this->template get<1>(), this->template get<_Is + 2>()...);
    }(__ycxx_indices());
  }

  template <::__ycxx::__detail::__exec::__decays_to<__exec_basic_sender> _Self, std::execution::receiver _Rcvr>
  constexpr auto connect(this _Self&& __self, _Rcvr __rcvr) noexcept(std::is_nothrow_constructible_v<__exec_basic_operation<_Self, _Rcvr>, _Self, _Rcvr>)
      -> __exec_basic_operation<_Self, _Rcvr> {
    return {static_cast<_Self&&>(__self), static_cast<_Rcvr&&>(__rcvr)};
  }

  template <class _Self, class... _Env>
  using __ycxx_csigs = typename ::__ycxx::__detail::__exec::__impls_for<_Tag>::template __csigs<_Self, _Env...>;

  // sndr.affine() ([exec.snd.expos]/1, [exec.affine]/6) for the senders that complete on the agent
  // that starts them.
  template <class _Self>
    requires requires { requires ::__ycxx::__detail::__exec::__impls_for<_Tag>::__ycxx_completes_inline; }
  constexpr std::remove_cvref_t<_Self> affine(this _Self&& __self) noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<_Self>, _Self>) {
    return static_cast<_Self&&>(__self);
  }

  template <::__ycxx::__detail::__exec::__decays_to<__exec_basic_sender> _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return ::__ycxx::__detail::__exec::__checked_sigs<__ycxx_csigs<_Self, _Env...>>();
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tag, class _Data, class... _Child>
struct tuple_size<__ycxx::__adl_free::__exec_basic_sender<_Tag, _Data, _Child...>> : integral_constant<size_t, sizeof...(_Child) + 2> {};
template <size_t _Ip, class _Tag, class _Data, class... _Child>
struct tuple_element<_Ip, __ycxx::__adl_free::__exec_basic_sender<_Tag, _Data, _Child...>> {
  using type = tuple_element_t<_Ip, __ycxx::__adl_free::__exec_product<index_sequence_for<_Tag, _Data, _Child...>, _Tag, _Data, _Child...>>;
};
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

template <class _Tag, class _Data, class... _Child>
using __basic_sender_t = ::__ycxx::__adl_free::__exec_basic_sender<_Tag, _Data, _Child...>;

// make-sender(tag, data, child...) ([exec.snd.expos]/23)
template <class _Tag, class _Data = __empty_data, class... _Child>
constexpr auto __make_sender(_Tag tag, _Data&& data, _Child&&... __child) noexcept(
    std::is_nothrow_constructible_v<std::decay_t<_Data>, _Data> && (std::is_nothrow_constructible_v<std::decay_t<_Child>, _Child> && ...)) {
  static_assert(std::semiregular<_Tag>);
  static_assert(__movable_value<_Data>);
  static_assert((std::execution::sender<_Child> && ...));
  using _Sp = __basic_sender_t<_Tag, std::decay_t<_Data>, std::decay_t<_Child>...>;
  static_assert(!__is_invalid_sigs<__csigs_of_t<_Sp>>,
                "the sender cannot complete in any environment (see the invalid_sigs<problem, ...> type in this diagnostic)");
  return _Sp{{{tag}, {static_cast<_Data&&>(data)}, {static_cast<_Child&&>(__child)}...}};
}
template <class _Tag>
constexpr auto __make_sender(_Tag tag) {
  return ::__ycxx::__detail::__exec::__make_sender(tag, __empty_data());
}

// The completion signatures of a basic-sender's children computed with and without the
// (forwarded) environment, and the failure checks the algorithms' check-types perform.
template <class _Sndr, class... _Env>
using __child_sigs_t = __csigs_of_t<__child_type<_Sndr>, __fwd_env_t<_Env>...>;

// decay-copyable-result-datums(cs)
template <class _Sig>
inline constexpr bool __decay_copyable_sig = false;
template <class _Tag, class... _Ts>
inline constexpr bool __decay_copyable_sig<_Tag(_Ts...)> = (std::is_constructible_v<std::decay_t<_Ts>, _Ts> && ...);
template <class _Sig>
inline constexpr bool __nothrow_decay_copy_sig = false;
template <class _Tag, class... _Ts>
inline constexpr bool __nothrow_decay_copy_sig<_Tag(_Ts...)> = (std::is_nothrow_constructible_v<std::decay_t<_Ts>, _Ts> && ...);
template <class _CS>
inline constexpr bool __decay_copyable_sigs = false;
template <class... _Sigs>
inline constexpr bool __decay_copyable_sigs<std::execution::completion_signatures<_Sigs...>> = (__decay_copyable_sig<_Sigs> && ...);
template <class _CS>
inline constexpr bool __nothrow_decay_copy_sigs = false;
template <class... _Sigs>
inline constexpr bool __nothrow_decay_copy_sigs<std::execution::completion_signatures<_Sigs...>> = (__nothrow_decay_copy_sig<_Sigs> && ...);

// A signature with its arguments decayed (what is sent after a decay-copy).
template <class _Sig>
struct __decayed_sig;
template <class _Tag, class... _Ts>
struct __decayed_sig<_Tag(_Ts...)> {
  using type = std::execution::completion_signatures<_Tag(std::decay_t<_Ts>...)>;
};
template <class _Sig>
using __decayed_sig_t = typename __decayed_sig<_Sig>::type;

using __eptr_sigs = std::execution::completion_signatures<std::execution::set_error_t(std::exception_ptr)>;
using __no_sigs = std::execution::completion_signatures<>;

// emplace-from ([exec.snd.expos]/15)
template <class _Fun>
  requires std::is_nothrow_move_constructible_v<_Fun>
struct __emplace_from {
  _Fun fun;
  using type = __call_result_t<_Fun>;
  constexpr operator type() && noexcept(__nothrow_callable<_Fun>) { return static_cast<_Fun&&>(fun)(); }
  constexpr type operator()() && noexcept(__nothrow_callable<_Fun>) { return static_cast<_Fun&&>(fun)(); }
};
template <class _Fun>
__emplace_from(_Fun) -> __emplace_from<_Fun>;

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// A variant of operation states (which cannot be moved): emplace constructs the alternative from
// the prvalue a function returns (guaranteed elision, which emplace-from cannot give through
// variant::emplace's direct-initialization). Index 0 is the empty state.
template <class... _Ts>
class __exec_op_variant {
  alignas(_Ts...) unsigned char __buf_[::__ycxx::__detail::__exec::max_size<_Ts...>];
  unsigned char __index_ = 0; // 0: empty, i + 1: Ts...[i]

  template <class _Tp>
  static constexpr unsigned char __index_of = static_cast<unsigned char>(::__ycxx::__detail::__exec::__first_true<std::is_same_v<_Tp, _Ts>...> + 1);

  void destroy() noexcept {
    if (__index_ != 0) {
      [this]<std::size_t... _Is>(std::index_sequence<_Is...>) {
        (void)((__index_ == _Is + 1 ? (std::launder(reinterpret_cast<_Ts...[_Is]*>(__buf_))->~_Ts...[_Is](), true) : false) || ...);
      }(std::index_sequence_for<_Ts...>());
      __index_ = 0;
    }
  }

public:
  __exec_op_variant() noexcept = default;
  __exec_op_variant(__exec_op_variant&&) = delete;
  ~__exec_op_variant() { destroy(); }

  void reset() noexcept { destroy(); }
  template <class _Tp, class _Fp>
  _Tp& __emplace_from_fn(_Fp&& __f) noexcept(noexcept(static_cast<_Fp&&>(__f)())) {
    destroy();
    _Tp* p = ::new (static_cast<void*>(__buf_)) _Tp(static_cast<_Fp&&>(__f)());
    __index_ = __index_of<_Tp>;
    return *p;
  }
  template <class _Tp>
  _Tp& get() noexcept {
    return *std::launder(reinterpret_cast<_Tp*>(__buf_));
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

// query-with-default(tag, env, value) ([exec.snd.expos]/11)
template <class _Tag, class _Env, class _Default>
consteval bool __query_with_default_nothrow() {
  if constexpr (requires(const _Env& env) { _Tag()(env); })
    return noexcept(_Tag()(std::declval<const _Env&>()));
  else
    return noexcept(static_cast<_Default>(std::declval<_Default>()));
}
template <class _Tag, class _Env, class _Default>
constexpr decltype(auto) __query_with_default(_Tag, const _Env& env, _Default&& value) noexcept(__query_with_default_nothrow<_Tag, _Env, _Default>()) {
  if constexpr (requires { _Tag()(env); })
    return _Tag()(env);
  else
    return static_cast<_Default>(static_cast<_Default&&>(value));
}

// call-with-default(fn, value, args...) ([exec.snd.expos]/55)
template <class _Fn, class _Default, class... _Args>
constexpr decltype(auto) __call_with_default(_Fn&& __fn, _Default&& value, _Args&&... __args) {
  if constexpr (requires { static_cast<_Fn&&>(__fn)(static_cast<_Args&&>(__args)...); })
    return static_cast<_Fn&&>(__fn)(static_cast<_Args&&>(__args)...);
  else
    return static_cast<_Default>(static_cast<_Default&&>(value));
}

// SET-VALUE(rcvr, expr), TRY-EVAL, TRY-SET-VALUE ([exec.snd.expos]/11): the callable form
// takes a function computing expr.
template <class _Rcvr, class _Fp>
constexpr void __set_value_of(_Rcvr& __rcvr, _Fp&& __f) noexcept(noexcept(static_cast<_Fp&&>(__f)())) {
  if constexpr (std::is_void_v<decltype(static_cast<_Fp&&>(__f)())>) {
    static_cast<_Fp&&>(__f)();
    std::execution::set_value(static_cast<_Rcvr&&>(__rcvr));
  } else {
    std::execution::set_value(static_cast<_Rcvr&&>(__rcvr), static_cast<_Fp&&>(__f)());
  }
}
template <class _Rcvr, class _Fp>
constexpr void __try_eval(_Rcvr& __rcvr, _Fp&& __f) noexcept {
  if constexpr (noexcept(static_cast<_Fp&&>(__f)())) {
    static_cast<_Fp&&>(__f)();
  } else if constexpr (__cfg::exceptions) {
    try {
      static_cast<_Fp&&>(__f)();
    } catch (...) {
      std::execution::set_error(static_cast<_Rcvr&&>(__rcvr), std::current_exception());
    }
  } else {
    static_cast<_Fp&&>(__f)();
  }
}
template <class _Rcvr, class _Fp>
constexpr void __try_set_value(_Rcvr& __rcvr, _Fp&& __f) noexcept {
  ::__ycxx::__detail::__exec::__try_eval(__rcvr, [&]() noexcept(noexcept(::__ycxx::__detail::__exec::__set_value_of(__rcvr, static_cast<_Fp&&>(__f)))) {
    ::__ycxx::__detail::__exec::__set_value_of(__rcvr, static_cast<_Fp&&>(__f));
  });
}

// overload-set ([exec.snd.expos]/50)
template <class... _Fns>
struct __overload_set : _Fns... {
  using _Fns::operator()...;
};
template <class... _Fns>
__overload_set(_Fns...) -> __overload_set<_Fns...>;

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// not-a-sender ([exec.snd.expos]/51): its completion signatures are an error.
struct __exec_not_a_sender {
  using sender_concept = std::execution::sender_tag;
  template <class _Self, class... _Env>
  using __ycxx_csigs = ::__ycxx::__detail::__exec::__invalid_sigs<::__ycxx::__detail::__exec::__not_a_sender_for_this_environment, _Self, _Env...>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return ::__ycxx::__detail::__exec::__checked_sigs<__ycxx_csigs<_Self, _Env...>>();
  }
};
// not-a-scheduler ([exec.snd.expos]/52)
struct __exec_not_a_scheduler {
  using scheduler_concept = std::execution::scheduler_tag;
  constexpr auto schedule() const noexcept { return __exec_not_a_sender(); }
};
}} // namespace __ycxx::__adl_free

// ---------------------------------------------------------------------------------------------
// Sender adaptor closure objects ([exec.adapt.obj]).
namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
template <__ycxx::__detail::__exec::__class_type _Dp>
struct sender_adaptor_closure {};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Dp>
void __closure_base_probe(const std::execution::sender_adaptor_closure<_Dp>&);
// A pipeable sender adaptor closure object ([exec.adapt.obj]/2): derived from exactly one
// sender_adaptor_closure specialization, that of its own type, and not a sender.
template <class _Tp>
concept __pipeable_closure = requires(const std::remove_cvref_t<_Tp>& t) { ::__ycxx::__detail::__exec::__closure_base_probe(t); } &&
                           std::derived_from<std::remove_cvref_t<_Tp>, std::execution::sender_adaptor_closure<std::remove_cvref_t<_Tp>>> &&
                           (!std::execution::sender<std::remove_cvref_t<_Tp>>);
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// c | d: a perfect forwarding call wrapper calling d2(c2(arg)).
template <class _Cp, class _Dp>
struct __exec_composed_closure : std::execution::sender_adaptor_closure<__exec_composed_closure<_Cp, _Dp>> {
  [[no_unique_address]] _Dp __d2;
  [[no_unique_address]] _Cp __c2;
  template <class _Self, class _Sndr>
  constexpr auto operator()(this _Self&& __self, _Sndr&& s) noexcept(
      noexcept(std::forward_like<_Self>(__self.__d2)(std::forward_like<_Self>(__self.__c2)(static_cast<_Sndr&&>(s)))))
      -> decltype(std::forward_like<_Self>(__self.__d2)(std::forward_like<_Self>(__self.__c2)(static_cast<_Sndr&&>(s)))) {
    return std::forward_like<_Self>(__self.__d2)(std::forward_like<_Self>(__self.__c2)(static_cast<_Sndr&&>(s)));
  }
};
// adaptor(args...): a perfect forwarding call wrapper calling adaptor(sndr, bound_args...).
template <class _Adaptor, class... _Bound>
struct __exec_bound_closure : std::execution::sender_adaptor_closure<__exec_bound_closure<_Adaptor, _Bound...>> {
  [[no_unique_address]] _Adaptor __adaptor;
  ::__ycxx::__detail::__exec::__product_t<_Bound...> __y_bound;

  template <class _Self, class _Sndr, std::size_t... _Is>
  static constexpr auto __ycxx_call(_Self&& __self, _Sndr&& s, std::index_sequence<_Is...>) noexcept(
      noexcept(std::forward_like<_Self>(__self.__adaptor)(static_cast<_Sndr&&>(s), static_cast<_Self&&>(__self).__y_bound.template get<_Is>()...)))
      -> decltype(std::forward_like<_Self>(__self.__adaptor)(static_cast<_Sndr&&>(s), static_cast<_Self&&>(__self).__y_bound.template get<_Is>()...)) {
    return std::forward_like<_Self>(__self.__adaptor)(static_cast<_Sndr&&>(s), static_cast<_Self&&>(__self).__y_bound.template get<_Is>()...);
  }
  template <class _Self, class _Sndr>
  constexpr auto operator()(this _Self&& __self, _Sndr&& s) noexcept(
      noexcept(__ycxx_call(static_cast<_Self&&>(__self), static_cast<_Sndr&&>(s), std::index_sequence_for<_Bound...>())))
      -> decltype(__ycxx_call(static_cast<_Self&&>(__self), static_cast<_Sndr&&>(s), std::index_sequence_for<_Bound...>())) {
    return __ycxx_call(static_cast<_Self&&>(__self), static_cast<_Sndr&&>(s), std::index_sequence_for<_Bound...>());
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Adaptor, class... _Args>
constexpr auto __bind_closure(_Adaptor __adaptor, _Args&&... __args) {
  return ::__ycxx::__adl_free::__exec_bound_closure<_Adaptor, std::decay_t<_Args>...>{{}, __adaptor, {{static_cast<_Args&&>(__args)}...}};
}
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
template <class _Sndr, class _Closure>
  requires sender<_Sndr> && __ycxx::__detail::__exec::__pipeable_closure<_Closure> && __ycxx::__detail::__exec::__callable<_Closure, _Sndr>
constexpr decltype(auto) operator|(_Sndr&& __sndr, _Closure&& c) noexcept(__ycxx::__detail::__exec::__nothrow_callable<_Closure, _Sndr>) {
  return static_cast<_Closure&&>(c)(static_cast<_Sndr&&>(__sndr));
}
template <class _Cp, class _Dp>
  requires __ycxx::__detail::__exec::__pipeable_closure<_Cp> && __ycxx::__detail::__exec::__pipeable_closure<_Dp> &&
           constructible_from<decay_t<_Cp>, _Cp> && constructible_from<decay_t<_Dp>, _Dp>
constexpr auto operator|(_Cp&& c, _Dp&& d) noexcept(is_nothrow_constructible_v<decay_t<_Cp>, _Cp> && is_nothrow_constructible_v<decay_t<_Dp>, _Dp>) {
  return __ycxx::__adl_free::__exec_composed_closure<decay_t<_Cp>, decay_t<_Dp>>{{}, static_cast<_Dp&&>(d), static_cast<_Cp&&>(c)};
}
}} // namespace std::execution
