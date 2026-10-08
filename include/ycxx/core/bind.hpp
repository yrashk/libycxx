// libycxx core: the call-wrapper adaptors of <functional>: not_fn ([func.not.fn]),
// bind_front/bind_back ([func.bind.partial]), mem_fn ([func.memfn]), bind and placeholders
// ([func.bind]).
//
// Perfect forwarding call wrappers have one explicit-object operator(), constrained on the call
// pattern's expression with the state entities forwarded like the wrapper (forward_like<Self>).
// Calling a const or rvalue wrapper therefore uses exactly the target's matching overload; a
// deleted overload is never bypassed. The wrapper types live in __ycxx::__adl_free (DECISIONS §2).
#pragma once

#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/tuple.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// f is a null (member) pointer: the Mandates of the NTTP forms.
template <auto __f>
consteval bool __is_null_pointer_constant() {
  if constexpr (std::is_pointer_v<decltype(__f)> || std::is_member_pointer_v<decltype(__f)>)
    return __f == nullptr;
  else
    return false;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

// The wrappers' state entities are direct-non-list-initialized ([func.not.fn]/1.3,
// [func.bind.partial]/1.3, [func.bind.bind]/1.3), so they have a tagged constructor rather than
// being aggregates.
struct __wrapper_init_t {
  explicit __wrapper_init_t() = default;
};
// The same, with a value-initialized target (the stateless constant_target of the NTTP forms).
struct __wrapper_init_bound_t {
  explicit __wrapper_init_bound_t() = default;
};

// The cast to the wrapper base is a constraint: it fails, rather than hard-errors, for a class
// with an ambiguous wrapper base. (A private base is fine for a C-style cast.)
template <class _Self, class _Wp>
concept __wrapper_castable = requires(_Self&& s) { (__ycxx::__detail::__copy_cvref<_Self&&, _Wp>)s; };

// Stateless target for the NTTP forms (bind_front<f>, not_fn<f>): calls the constant f. (The
// draft's target object is a copy of cw<f>; its type is not observable through the wrapper.)
template <auto __f>
struct __constant_target {
  // f names a const lvalue (a template parameter object for class types).
  using _FR = const decltype(__f)&;
  template <class... _Ap>
    requires std::is_invocable_v<_FR, _Ap...>
  static constexpr decltype(auto) operator()(_Ap&&... a) noexcept(std::is_nothrow_invocable_v<_FR, _Ap...>) {
    return ::__ycxx::__detail::invoke(__f, static_cast<_Ap&&>(a)...);
  }
};

template <class _FD>
struct __not_fn_wrapper {
  [[no_unique_address]] _FD __fd;

  template <class _Fp>
  constexpr __not_fn_wrapper(__wrapper_init_t, _Fp&& __f) : __fd(static_cast<_Fp&&>(__f)) {}
  constexpr explicit __not_fn_wrapper(__wrapper_init_bound_t) : __fd() {}

  template <class _Self, class... _Ap>
    requires(!std::is_volatile_v<std::remove_reference_t<_Self>>) && __wrapper_castable<_Self, __not_fn_wrapper> &&
            requires { !::__ycxx::__detail::invoke(std::declval<__ycxx::__detail::__forward_like_t<_Self, _FD>>(), std::declval<_Ap>()...); }
  constexpr decltype(auto) operator()(this _Self&& __self, _Ap&&... a) noexcept(
      noexcept(!::__ycxx::__detail::invoke(std::forward_like<_Self>(((__ycxx::__detail::__copy_cvref<_Self&&, __not_fn_wrapper>)__self).__fd), static_cast<_Ap&&>(a)...))) {
    return !::__ycxx::__detail::invoke(std::forward_like<_Self>(((__ycxx::__detail::__copy_cvref<_Self&&, __not_fn_wrapper>)__self).__fd), static_cast<_Ap&&>(a)...);
  }
};

template <bool _Front, class _FD, class... _Bound>
struct __partial_wrapper {
  [[no_unique_address]] _FD __fd;
  [[no_unique_address]] std::tuple<_Bound...> __y_bound;

  template <class _Fp, class... _Bp>
  constexpr __partial_wrapper(__wrapper_init_t, _Fp&& __f, _Bp&&... b)
      : __fd(static_cast<_Fp&&>(__f)), __y_bound(static_cast<_Bp&&>(b)...) {}
  template <class... _Bp>
  constexpr __partial_wrapper(__wrapper_init_bound_t, _Bp&&... b) : __fd(), __y_bound(static_cast<_Bp&&>(b)...) {}

  template <class _Self, class... _Ap>
  static constexpr bool __callable = [] {
    if constexpr (_Front)
      return std::is_invocable_v<__ycxx::__detail::__forward_like_t<_Self, _FD>, __ycxx::__detail::__forward_like_t<_Self, _Bound>...,
                                 _Ap...>;
    else
      return std::is_invocable_v<__ycxx::__detail::__forward_like_t<_Self, _FD>, _Ap...,
                                 __ycxx::__detail::__forward_like_t<_Self, _Bound>...>;
  }();
  template <class _Self, class... _Ap>
  static constexpr bool nothrow = [] {
    if constexpr (_Front)
      return std::is_nothrow_invocable_v<__ycxx::__detail::__forward_like_t<_Self, _FD>,
                                         __ycxx::__detail::__forward_like_t<_Self, _Bound>..., _Ap...>;
    else
      return std::is_nothrow_invocable_v<__ycxx::__detail::__forward_like_t<_Self, _FD>, _Ap...,
                                         __ycxx::__detail::__forward_like_t<_Self, _Bound>...>;
  }();

  template <class _Self, class... _Ap>
    requires(!std::is_volatile_v<std::remove_reference_t<_Self>>) && __wrapper_castable<_Self, __partial_wrapper> &&
            __callable<_Self, _Ap...>
  constexpr decltype(auto) operator()(this _Self&& __self, _Ap&&... a) noexcept(nothrow<_Self, _Ap...>) {
    // Through the wrapper type: Self may be a class derived from it, even privately.
    auto&& __w = (__ycxx::__detail::__copy_cvref<_Self&&, __partial_wrapper>)__self;
    return [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) -> decltype(auto) {
      if constexpr (_Front)
        return ::__ycxx::__detail::invoke(std::forward_like<_Self>(__w.__fd), std::forward_like<_Self>(std::get<_Ip>(__w.__y_bound))...,
                                      static_cast<_Ap&&>(a)...);
      else
        return ::__ycxx::__detail::invoke(std::forward_like<_Self>(__w.__fd), static_cast<_Ap&&>(a)...,
                                      std::forward_like<_Self>(std::get<_Ip>(__w.__y_bound))...);
    }(std::index_sequence_for<_Bound...>{});
  }
};

// mem_fn: a simple call wrapper.
template <class _PM>
struct __mem_fn_wrapper {
  _PM __pm;
  template <class... _Ap>
    requires std::is_invocable_v<const _PM&, _Ap...>
  constexpr std::invoke_result_t<const _PM&, _Ap...> operator()(_Ap&&... a) const
      noexcept(std::is_nothrow_invocable_v<const _PM&, _Ap...>) {
    return ::__ycxx::__detail::invoke(__pm, static_cast<_Ap&&>(a)...);
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// ---- [func.not.fn] ----
// Each factory checks its Mandates first and constructs only when they hold, so a violation
// produces the one static_assert message.
template <class _Fp>
constexpr auto not_fn(_Fp&& __f) noexcept(is_nothrow_constructible_v<decay_t<_Fp>, _Fp>) {
  using _FD = decay_t<_Fp>;
  if constexpr (!(is_constructible_v<_FD, _Fp> && is_move_constructible_v<_FD>))
    static_assert(false, "std::not_fn: decay_t<F> must be constructible from F and move constructible");
  else
    return __ycxx::__adl_free::__not_fn_wrapper<_FD>(__ycxx::__adl_free::__wrapper_init_t{}, static_cast<_Fp&&>(__f));
}
template <auto __f>
constexpr auto not_fn() noexcept {
  if constexpr (__ycxx::__detail::__is_null_pointer_constant<__f>())
    static_assert(false, "std::not_fn<f>: f must not be a null pointer");
  else
    return __ycxx::__adl_free::__not_fn_wrapper<__ycxx::__adl_free::__constant_target<__f>>(__ycxx::__adl_free::__wrapper_init_bound_t{});
}

// ---- [func.bind.partial] ----
// The factories are noexcept when initialising the state entities cannot throw (a permitted
// strengthening of "Throws: any exception thrown by the initialization").
template <class _Fp, class... _Args>
constexpr auto bind_front(_Fp&& __f, _Args&&... __args) noexcept(is_nothrow_constructible_v<decay_t<_Fp>, _Fp> && (is_nothrow_constructible_v<decay_t<_Args>, _Args> && ...)) {
  using _FD = decay_t<_Fp>;
  if constexpr (!(is_constructible_v<_FD, _Fp> && is_move_constructible_v<_FD> &&
                  (is_constructible_v<decay_t<_Args>, _Args> && ...) && (is_move_constructible_v<decay_t<_Args>> && ...)))
    static_assert(false, "std::bind_front: the target and bound arguments must be constructible and move constructible");
  else
    return __ycxx::__adl_free::__partial_wrapper<true, _FD, decay_t<_Args>...>(__ycxx::__adl_free::__wrapper_init_t{}, static_cast<_Fp&&>(__f),
                                                                       static_cast<_Args&&>(__args)...);
}
template <class _Fp, class... _Args>
constexpr auto bind_back(_Fp&& __f, _Args&&... __args) noexcept(is_nothrow_constructible_v<decay_t<_Fp>, _Fp> && (is_nothrow_constructible_v<decay_t<_Args>, _Args> && ...)) {
  using _FD = decay_t<_Fp>;
  if constexpr (!(is_constructible_v<_FD, _Fp> && is_move_constructible_v<_FD> &&
                  (is_constructible_v<decay_t<_Args>, _Args> && ...) && (is_move_constructible_v<decay_t<_Args>> && ...)))
    static_assert(false, "std::bind_back: the target and bound arguments must be constructible and move constructible");
  else
    return __ycxx::__adl_free::__partial_wrapper<false, _FD, decay_t<_Args>...>(__ycxx::__adl_free::__wrapper_init_t{}, static_cast<_Fp&&>(__f),
                                                                        static_cast<_Args&&>(__args)...);
}
template <auto __f, class... _Args>
constexpr auto bind_front(_Args&&... __args) noexcept((is_nothrow_constructible_v<decay_t<_Args>, _Args> && ...)) {
  if constexpr (!((is_constructible_v<decay_t<_Args>, _Args> && ...) && (is_move_constructible_v<decay_t<_Args>> && ...)))
    static_assert(false, "std::bind_front<f>: the bound arguments must be constructible and move constructible");
  else if constexpr (__ycxx::__detail::__is_null_pointer_constant<__f>())
    static_assert(false, "std::bind_front<f>: f must not be a null pointer");
  else
    return __ycxx::__adl_free::__partial_wrapper<true, __ycxx::__adl_free::__constant_target<__f>, decay_t<_Args>...>(
        __ycxx::__adl_free::__wrapper_init_bound_t{}, static_cast<_Args&&>(__args)...);
}
template <auto __f, class... _Args>
constexpr auto bind_back(_Args&&... __args) noexcept((is_nothrow_constructible_v<decay_t<_Args>, _Args> && ...)) {
  if constexpr (!((is_constructible_v<decay_t<_Args>, _Args> && ...) && (is_move_constructible_v<decay_t<_Args>> && ...)))
    static_assert(false, "std::bind_back<f>: the bound arguments must be constructible and move constructible");
  else if constexpr (__ycxx::__detail::__is_null_pointer_constant<__f>())
    static_assert(false, "std::bind_back<f>: f must not be a null pointer");
  else
    return __ycxx::__adl_free::__partial_wrapper<false, __ycxx::__adl_free::__constant_target<__f>, decay_t<_Args>...>(
        __ycxx::__adl_free::__wrapper_init_bound_t{}, static_cast<_Args&&>(__args)...);
}

// ---- [func.memfn] ----
template <class _Rp, class _Tp>
constexpr auto mem_fn(_Rp _Tp::* __pm) noexcept {
  return __ycxx::__adl_free::__mem_fn_wrapper<_Rp _Tp::*>{__pm};
}

// ---- [func.bind.isbind], [func.bind.isplace] ----
template <class _Tp>
struct is_bind_expression : false_type {};
template <class _Tp>
constexpr bool is_bind_expression_v = is_bind_expression<_Tp>::value;
template <class _Tp>
struct is_placeholder : integral_constant<int, 0> {};
template <class _Tp>
constexpr int is_placeholder_v = is_placeholder<_Tp>::value;

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

template <int _Jp>
struct __placeholder {
  constexpr __placeholder() noexcept = default;
  constexpr __placeholder(const __placeholder&) noexcept = default;
  constexpr __placeholder& operator=(const __placeholder&) noexcept = default;
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// The J-th argument, forwarded. (A function, not u...[J] in place: GCC evaluates a pack index
// in a discarded branch, and fails when the pack is empty.)
template <std::size_t _Jp, class... _Up>
constexpr _Up...[_Jp]&& __nth_arg(_Up&&... __u) noexcept {
  return static_cast<_Up...[_Jp]&&>(__u...[_Jp]);
}

// The value of a bound argument (one full-expression with the call, so temporaries of nested
// bind expressions live long enough).
template <class _CvTD, class... _Up>
constexpr decltype(auto) __bind_value(_CvTD __td, _Up&&... __u) {
  using _TD = std::remove_cvref_t<_CvTD>;
  if constexpr (__ycxx::__detail::__is_reference_wrapper<_TD>)
    return __td.get();
  else if constexpr (std::is_bind_expression_v<_TD>)
    return __td(static_cast<_Up&&>(__u)...);
  else if constexpr (std::is_placeholder_v<_TD> > 0)
    return ::__ycxx::__detail::__nth_arg<std::is_placeholder_v<_TD> - 1>(static_cast<_Up&&>(__u)...);
  else
    return static_cast<_CvTD>(__td);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

// The type V_i of a bound argument ([func.bind.bind]/7), for a wrapper of constness `__cv` (CvTD
// is cv TD&) called with arguments U&&...
template <class _CvTD, class... _Up>
struct __bind_arg {
  using type = _CvTD; // (7.4)
};
template <class _CvTD, class... _Up>
  requires __ycxx::__detail::__is_reference_wrapper<std::remove_cvref_t<_CvTD>>
struct __bind_arg<_CvTD, _Up...> { // (7.1)
  using type = typename std::remove_cvref_t<_CvTD>::type&;
};
template <class _CvTD, class... _Up>
  requires(!__ycxx::__detail::__is_reference_wrapper<std::remove_cvref_t<_CvTD>>) &&
          std::is_bind_expression_v<std::remove_cvref_t<_CvTD>> && std::is_invocable_v<_CvTD, _Up...> &&
          (!std::is_void_v<std::invoke_result_t<_CvTD, _Up...>>)
struct __bind_arg<_CvTD, _Up...> { // (7.2)
  using type = std::invoke_result_t<_CvTD, _Up...>&&;
};
template <class _CvTD, class... _Up>
  requires(!__ycxx::__detail::__is_reference_wrapper<std::remove_cvref_t<_CvTD>>) &&
          (!std::is_bind_expression_v<std::remove_cvref_t<_CvTD>>) && (std::is_placeholder_v<std::remove_cvref_t<_CvTD>> > 0) &&
          (std::is_placeholder_v<std::remove_cvref_t<_CvTD>> <= static_cast<int>(sizeof...(_Up)))
struct __bind_arg<_CvTD, _Up...> { // (7.3)
  using type = _Up...[std::is_placeholder_v<std::remove_cvref_t<_CvTD>> - 1]&&;
};
// A bind expression that cannot be called with U... (or returns void: V_i would be void&&), or a
// placeholder beyond the arguments: no type, so the wrapper's operator() is not viable.
template <class _CvTD, class... _Up>
  requires(!__ycxx::__detail::__is_reference_wrapper<std::remove_cvref_t<_CvTD>>) &&
          ((std::is_bind_expression_v<std::remove_cvref_t<_CvTD>> &&
            (!std::is_invocable_v<_CvTD, _Up...> || std::is_void_v<std::invoke_result_t<_CvTD, _Up...>>)) ||
           (!std::is_bind_expression_v<std::remove_cvref_t<_CvTD>> &&
            std::is_placeholder_v<std::remove_cvref_t<_CvTD>> > static_cast<int>(sizeof...(_Up))))
struct __bind_arg<_CvTD, _Up...> {};

template <class _CvTD, class... _Up>
using __bind_arg_t = typename __bind_arg<_CvTD, _Up...>::type;
template <class _CvTD, class... _Up>
concept __has_bind_arg = requires { typename __bind_arg<_CvTD, _Up...>::type; };

struct __bind_no_r {};

template <class _Rp, class _FD, class... _TD>
struct __binder {
  _FD __fd;
  std::tuple<_TD...> __y_bound;

  template <class _Fp, class... _Bp>
  constexpr __binder(__wrapper_init_t, _Fp&& __f, _Bp&&... b) : __fd(static_cast<_Fp&&>(__f)), __y_bound(static_cast<_Bp&&>(b)...) {}

  template <class _Self>
  using __cv = std::conditional_t<std::is_const_v<std::remove_reference_t<_Self>>, const int, int>;
  template <class _Self, class _Tp>
  using __cv_ref = __ycxx::__detail::__copy_cv<__cv<_Self>, _Tp>&;

  template <class _Self, class... _Up>
  static constexpr bool __callable = [] {
    if constexpr ((__has_bind_arg<__cv_ref<_Self, _TD>, _Up...> && ...)) {
      if constexpr (std::is_same_v<_Rp, __bind_no_r>)
        return std::is_invocable_v<__cv_ref<_Self, _FD>, __bind_arg_t<__cv_ref<_Self, _TD>, _Up...>...>;
      else
        return std::is_invocable_r_v<_Rp, __cv_ref<_Self, _FD>, __bind_arg_t<__cv_ref<_Self, _TD>, _Up...>...>;
    } else {
      return false;
    }
  }();

  // The call is expression-equivalent to INVOKE (or INVOKE<R>) on the bound values, so it is
  // noexcept when that call and every nested bind call (7.2) are.
  template <class _CvTD, class... _Up>
  static constexpr bool __nested_nothrow = [] {
    if constexpr (!__ycxx::__detail::__is_reference_wrapper<std::remove_cvref_t<_CvTD>> &&
                  std::is_bind_expression_v<std::remove_cvref_t<_CvTD>>)
      return std::is_nothrow_invocable_v<_CvTD, _Up...>;
    else
      return true;
  }();
  template <class _Self, class... _Up>
  static constexpr bool nothrow = [] {
    if constexpr (__callable<_Self, _Up...>) {
      if constexpr (std::is_same_v<_Rp, __bind_no_r>)
        return std::is_nothrow_invocable_v<__cv_ref<_Self, _FD>, __bind_arg_t<__cv_ref<_Self, _TD>, _Up...>...> &&
               (__nested_nothrow<__cv_ref<_Self, _TD>, _Up...> && ...);
      else
        return std::is_nothrow_invocable_r_v<_Rp, __cv_ref<_Self, _FD>, __bind_arg_t<__cv_ref<_Self, _TD>, _Up...>...> &&
               (__nested_nothrow<__cv_ref<_Self, _TD>, _Up...> && ...);
    } else {
      return false;
    }
  }();

  template <class _Self, class... _Up>
    requires(!std::is_volatile_v<std::remove_reference_t<_Self>>) && __wrapper_castable<_Self&, __binder> &&
            __callable<_Self, _Up...>
  constexpr decltype(auto) operator()(this _Self&& __selfd, _Up&&... __u) noexcept(nothrow<_Self, _Up...>) {
    auto& __self = (__ycxx::__detail::__copy_cvref<_Self&, __binder>)__selfd; // see partial_wrapper
    // COMPILER-BUG(gcc): GCC 16 diagnoses TD...[I] inside an expansion over an empty I pack
    // ("cannot index an empty pack"), so the element types come from tuple_element_t.
    return [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) -> decltype(auto) {
      if constexpr (std::is_same_v<_Rp, __bind_no_r>)
        return ::__ycxx::__detail::invoke(static_cast<__cv_ref<_Self, _FD>>(__self.__fd),
                                      static_cast<__bind_arg_t<__cv_ref<_Self, std::tuple_element_t<_Ip, std::tuple<_TD...>>>, _Up...>>(
                                          ::__ycxx::__detail::__bind_value<__cv_ref<_Self, std::tuple_element_t<_Ip, std::tuple<_TD...>>>>(std::get<_Ip>(__self.__y_bound), static_cast<_Up&&>(__u)...))...);
      else
        return ::__ycxx::__detail::invoke_r<_Rp>(static_cast<__cv_ref<_Self, _FD>>(__self.__fd),
                                           static_cast<__bind_arg_t<__cv_ref<_Self, std::tuple_element_t<_Ip, std::tuple<_TD...>>>, _Up...>>(::__ycxx::__detail::__bind_value<__cv_ref<_Self, std::tuple_element_t<_Ip, std::tuple<_TD...>>>>(
                                               std::get<_Ip>(__self.__y_bound), static_cast<_Up&&>(__u)...))...);
    }(std::index_sequence_for<_TD...>{});
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Rp, class _FD, class... _TD>
struct is_bind_expression<__ycxx::__adl_free::__binder<_Rp, _FD, _TD...>> : true_type {};
template <int _Jp>
struct is_placeholder<__ycxx::__adl_free::__placeholder<_Jp>> : integral_constant<int, _Jp> {};
// decltype(placeholders::_J) is const-qualified, and "the type of _J" must be recognised; the
// same for const bind expressions.
template <class _Tp>
struct is_placeholder<const _Tp> : is_placeholder<_Tp> {};
template <class _Tp>
struct is_bind_expression<const _Tp> : is_bind_expression<_Tp> {};
template <class _Tp>
struct is_bind_expression<volatile _Tp> : is_bind_expression<_Tp> {};
template <class _Tp>
struct is_bind_expression<const volatile _Tp> : is_bind_expression<_Tp> {};

namespace placeholders {
inline constexpr __ycxx::__adl_free::__placeholder<1> _1{};
inline constexpr __ycxx::__adl_free::__placeholder<2> _2{};
inline constexpr __ycxx::__adl_free::__placeholder<3> _3{};
inline constexpr __ycxx::__adl_free::__placeholder<4> _4{};
inline constexpr __ycxx::__adl_free::__placeholder<5> _5{};
inline constexpr __ycxx::__adl_free::__placeholder<6> _6{};
inline constexpr __ycxx::__adl_free::__placeholder<7> _7{};
inline constexpr __ycxx::__adl_free::__placeholder<8> _8{};
inline constexpr __ycxx::__adl_free::__placeholder<9> _9{};
inline constexpr __ycxx::__adl_free::__placeholder<10> _10{};
} // namespace placeholders

// ---- [func.bind.bind] ----
template <class _Fp, class... _BoundArgs>
constexpr auto bind(_Fp&& __f, _BoundArgs&&... __bound_args) {
  if constexpr (!(is_constructible_v<decay_t<_Fp>, _Fp> && (is_constructible_v<decay_t<_BoundArgs>, _BoundArgs> && ...)))
    static_assert(false, "std::bind: the target and bound arguments must be constructible from the arguments");
  else
    return __ycxx::__adl_free::__binder<__ycxx::__adl_free::__bind_no_r, decay_t<_Fp>, decay_t<_BoundArgs>...>(
        __ycxx::__adl_free::__wrapper_init_t{}, static_cast<_Fp&&>(__f), static_cast<_BoundArgs&&>(__bound_args)...);
}
template <class _Rp, class _Fp, class... _BoundArgs>
constexpr auto bind(_Fp&& __f, _BoundArgs&&... __bound_args) {
  if constexpr (!(is_constructible_v<decay_t<_Fp>, _Fp> && (is_constructible_v<decay_t<_BoundArgs>, _BoundArgs> && ...)))
    static_assert(false, "std::bind: the target and bound arguments must be constructible from the arguments");
  else
    return __ycxx::__adl_free::__binder<_Rp, decay_t<_Fp>, decay_t<_BoundArgs>...>(__ycxx::__adl_free::__wrapper_init_t{}, static_cast<_Fp&&>(__f),
                                                                        static_cast<_BoundArgs&&>(__bound_args)...);
}

}} // namespace std
