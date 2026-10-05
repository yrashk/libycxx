// libycxx core: the call-wrapper adaptors of <functional>: not_fn ([func.not.fn]),
// bind_front/bind_back ([func.bind.partial]), mem_fn ([func.memfn]), bind and placeholders
// ([func.bind]).
//
// Perfect forwarding call wrappers have one explicit-object operator(), constrained on the call
// pattern's expression with the state entities forwarded like the wrapper (forward_like<Self>).
// Calling a const or rvalue wrapper therefore uses exactly the target's matching overload; a
// deleted overload is never bypassed. The wrapper types live in ycxx::adl_free (DECISIONS §2).
#pragma once

#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/tuple.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// f is a null (member) pointer: the Mandates of the NTTP forms.
template <auto f>
consteval bool is_null_pointer_constant() {
  if constexpr (std::is_pointer_v<decltype(f)> || std::is_member_pointer_v<decltype(f)>)
    return f == nullptr;
  else
    return false;
}
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The wrappers' state entities are direct-non-list-initialized ([func.not.fn]/1.3,
// [func.bind.partial]/1.3, [func.bind.bind]/1.3), so they have a tagged constructor rather than
// being aggregates.
struct wrapper_init_t {
  explicit wrapper_init_t() = default;
};
// The same, with a value-initialized target (the stateless constant_target of the NTTP forms).
struct wrapper_init_bound_t {
  explicit wrapper_init_bound_t() = default;
};

// The cast to the wrapper base is a constraint: it fails, rather than hard-errors, for a class
// with an ambiguous wrapper base. (A private base is fine for a C-style cast.)
template <class Self, class W>
concept wrapper_castable = requires(Self&& s) { (ycxx::detail::copy_cvref<Self&&, W>)s; };

// Stateless target for the NTTP forms (bind_front<f>, not_fn<f>): calls the constant f. (The
// draft's target object is a copy of cw<f>; its type is not observable through the wrapper.)
template <auto f>
struct constant_target {
  // f names a const lvalue (a template parameter object for class types).
  using FR = const decltype(f)&;
  template <class... A>
    requires std::is_invocable_v<FR, A...>
  static constexpr decltype(auto) operator()(A&&... a) noexcept(std::is_nothrow_invocable_v<FR, A...>) {
    return ::ycxx::detail::invoke(f, static_cast<A&&>(a)...);
  }
};

template <class FD>
struct not_fn_wrapper {
  [[no_unique_address]] FD fd;

  template <class F>
  constexpr not_fn_wrapper(wrapper_init_t, F&& f) : fd(static_cast<F&&>(f)) {}
  constexpr explicit not_fn_wrapper(wrapper_init_bound_t) : fd() {}

  template <class Self, class... A>
    requires(!std::is_volatile_v<std::remove_reference_t<Self>>) && wrapper_castable<Self, not_fn_wrapper> &&
            requires { !::ycxx::detail::invoke(std::declval<ycxx::detail::forward_like_t<Self, FD>>(), std::declval<A>()...); }
  constexpr decltype(auto) operator()(this Self&& self, A&&... a) noexcept(
      noexcept(!::ycxx::detail::invoke(std::forward_like<Self>(((ycxx::detail::copy_cvref<Self&&, not_fn_wrapper>)self).fd), static_cast<A&&>(a)...))) {
    return !::ycxx::detail::invoke(std::forward_like<Self>(((ycxx::detail::copy_cvref<Self&&, not_fn_wrapper>)self).fd), static_cast<A&&>(a)...);
  }
};

template <bool Front, class FD, class... Bound>
struct partial_wrapper {
  [[no_unique_address]] FD fd;
  [[no_unique_address]] std::tuple<Bound...> bound;

  template <class F, class... B>
  constexpr partial_wrapper(wrapper_init_t, F&& f, B&&... b)
      : fd(static_cast<F&&>(f)), bound(static_cast<B&&>(b)...) {}
  template <class... B>
  constexpr partial_wrapper(wrapper_init_bound_t, B&&... b) : fd(), bound(static_cast<B&&>(b)...) {}

  template <class Self, class... A>
  static constexpr bool callable = [] {
    if constexpr (Front)
      return std::is_invocable_v<ycxx::detail::forward_like_t<Self, FD>, ycxx::detail::forward_like_t<Self, Bound>...,
                                 A...>;
    else
      return std::is_invocable_v<ycxx::detail::forward_like_t<Self, FD>, A...,
                                 ycxx::detail::forward_like_t<Self, Bound>...>;
  }();
  template <class Self, class... A>
  static constexpr bool nothrow = [] {
    if constexpr (Front)
      return std::is_nothrow_invocable_v<ycxx::detail::forward_like_t<Self, FD>,
                                         ycxx::detail::forward_like_t<Self, Bound>..., A...>;
    else
      return std::is_nothrow_invocable_v<ycxx::detail::forward_like_t<Self, FD>, A...,
                                         ycxx::detail::forward_like_t<Self, Bound>...>;
  }();

  template <class Self, class... A>
    requires(!std::is_volatile_v<std::remove_reference_t<Self>>) && wrapper_castable<Self, partial_wrapper> &&
            callable<Self, A...>
  constexpr decltype(auto) operator()(this Self&& self, A&&... a) noexcept(nothrow<Self, A...>) {
    // Through the wrapper type: Self may be a class derived from it, even privately.
    auto&& w = (ycxx::detail::copy_cvref<Self&&, partial_wrapper>)self;
    return [&]<std::size_t... I>(std::index_sequence<I...>) -> decltype(auto) {
      if constexpr (Front)
        return ::ycxx::detail::invoke(std::forward_like<Self>(w.fd), std::forward_like<Self>(std::get<I>(w.bound))...,
                                      static_cast<A&&>(a)...);
      else
        return ::ycxx::detail::invoke(std::forward_like<Self>(w.fd), static_cast<A&&>(a)...,
                                      std::forward_like<Self>(std::get<I>(w.bound))...);
    }(std::index_sequence_for<Bound...>{});
  }
};

// mem_fn: a simple call wrapper.
template <class PM>
struct mem_fn_wrapper {
  PM pm;
  template <class... A>
    requires std::is_invocable_v<const PM&, A...>
  constexpr std::invoke_result_t<const PM&, A...> operator()(A&&... a) const
      noexcept(std::is_nothrow_invocable_v<const PM&, A...>) {
    return ::ycxx::detail::invoke(pm, static_cast<A&&>(a)...);
  }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

// ---- [func.not.fn] ----
// Each factory checks its Mandates first and constructs only when they hold, so a violation
// produces the one static_assert message.
template <class F>
constexpr auto not_fn(F&& f) noexcept(is_nothrow_constructible_v<decay_t<F>, F>) {
  using FD = decay_t<F>;
  if constexpr (!(is_constructible_v<FD, F> && is_move_constructible_v<FD>))
    static_assert(false, "std::not_fn: decay_t<F> must be constructible from F and move constructible");
  else
    return ycxx::adl_free::not_fn_wrapper<FD>(ycxx::adl_free::wrapper_init_t{}, static_cast<F&&>(f));
}
template <auto f>
constexpr auto not_fn() noexcept {
  if constexpr (ycxx::detail::is_null_pointer_constant<f>())
    static_assert(false, "std::not_fn<f>: f must not be a null pointer");
  else
    return ycxx::adl_free::not_fn_wrapper<ycxx::adl_free::constant_target<f>>(ycxx::adl_free::wrapper_init_bound_t{});
}

// ---- [func.bind.partial] ----
// The factories are noexcept when initialising the state entities cannot throw (a permitted
// strengthening of "Throws: any exception thrown by the initialization").
template <class F, class... Args>
constexpr auto bind_front(F&& f, Args&&... args) noexcept(is_nothrow_constructible_v<decay_t<F>, F> && (is_nothrow_constructible_v<decay_t<Args>, Args> && ...)) {
  using FD = decay_t<F>;
  if constexpr (!(is_constructible_v<FD, F> && is_move_constructible_v<FD> &&
                  (is_constructible_v<decay_t<Args>, Args> && ...) && (is_move_constructible_v<decay_t<Args>> && ...)))
    static_assert(false, "std::bind_front: the target and bound arguments must be constructible and move constructible");
  else
    return ycxx::adl_free::partial_wrapper<true, FD, decay_t<Args>...>(ycxx::adl_free::wrapper_init_t{}, static_cast<F&&>(f),
                                                                       static_cast<Args&&>(args)...);
}
template <class F, class... Args>
constexpr auto bind_back(F&& f, Args&&... args) noexcept(is_nothrow_constructible_v<decay_t<F>, F> && (is_nothrow_constructible_v<decay_t<Args>, Args> && ...)) {
  using FD = decay_t<F>;
  if constexpr (!(is_constructible_v<FD, F> && is_move_constructible_v<FD> &&
                  (is_constructible_v<decay_t<Args>, Args> && ...) && (is_move_constructible_v<decay_t<Args>> && ...)))
    static_assert(false, "std::bind_back: the target and bound arguments must be constructible and move constructible");
  else
    return ycxx::adl_free::partial_wrapper<false, FD, decay_t<Args>...>(ycxx::adl_free::wrapper_init_t{}, static_cast<F&&>(f),
                                                                        static_cast<Args&&>(args)...);
}
template <auto f, class... Args>
constexpr auto bind_front(Args&&... args) noexcept((is_nothrow_constructible_v<decay_t<Args>, Args> && ...)) {
  if constexpr (!((is_constructible_v<decay_t<Args>, Args> && ...) && (is_move_constructible_v<decay_t<Args>> && ...)))
    static_assert(false, "std::bind_front<f>: the bound arguments must be constructible and move constructible");
  else if constexpr (ycxx::detail::is_null_pointer_constant<f>())
    static_assert(false, "std::bind_front<f>: f must not be a null pointer");
  else
    return ycxx::adl_free::partial_wrapper<true, ycxx::adl_free::constant_target<f>, decay_t<Args>...>(
        ycxx::adl_free::wrapper_init_bound_t{}, static_cast<Args&&>(args)...);
}
template <auto f, class... Args>
constexpr auto bind_back(Args&&... args) noexcept((is_nothrow_constructible_v<decay_t<Args>, Args> && ...)) {
  if constexpr (!((is_constructible_v<decay_t<Args>, Args> && ...) && (is_move_constructible_v<decay_t<Args>> && ...)))
    static_assert(false, "std::bind_back<f>: the bound arguments must be constructible and move constructible");
  else if constexpr (ycxx::detail::is_null_pointer_constant<f>())
    static_assert(false, "std::bind_back<f>: f must not be a null pointer");
  else
    return ycxx::adl_free::partial_wrapper<false, ycxx::adl_free::constant_target<f>, decay_t<Args>...>(
        ycxx::adl_free::wrapper_init_bound_t{}, static_cast<Args&&>(args)...);
}

// ---- [func.memfn] ----
template <class R, class T>
constexpr auto mem_fn(R T::* pm) noexcept {
  return ycxx::adl_free::mem_fn_wrapper<R T::*>{pm};
}

// ---- [func.bind.isbind], [func.bind.isplace] ----
template <class T>
struct is_bind_expression : false_type {};
template <class T>
constexpr bool is_bind_expression_v = is_bind_expression<T>::value;
template <class T>
struct is_placeholder : integral_constant<int, 0> {};
template <class T>
constexpr int is_placeholder_v = is_placeholder<T>::value;

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

template <int J>
struct placeholder {
  constexpr placeholder() noexcept = default;
  constexpr placeholder(const placeholder&) noexcept = default;
  constexpr placeholder& operator=(const placeholder&) noexcept = default;
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// The J-th argument, forwarded. (A function, not u...[J] in place: GCC evaluates a pack index
// in a discarded branch, and fails when the pack is empty.)
template <std::size_t J, class... U>
constexpr U...[J]&& nth_arg(U&&... u) noexcept {
  return static_cast<U...[J]&&>(u...[J]);
}

// The value of a bound argument (one full-expression with the call, so temporaries of nested
// bind expressions live long enough).
template <class CvTD, class... U>
constexpr decltype(auto) bind_value(CvTD td, U&&... u) {
  using TD = std::remove_cvref_t<CvTD>;
  if constexpr (ycxx::detail::is_reference_wrapper<TD>)
    return td.get();
  else if constexpr (std::is_bind_expression_v<TD>)
    return td(static_cast<U&&>(u)...);
  else if constexpr (std::is_placeholder_v<TD> > 0)
    return ::ycxx::detail::nth_arg<std::is_placeholder_v<TD> - 1>(static_cast<U&&>(u)...);
  else
    return static_cast<CvTD>(td);
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The type V_i of a bound argument ([func.bind.bind]/7), for a wrapper of constness `cv` (CvTD
// is cv TD&) called with arguments U&&...
template <class CvTD, class... U>
struct bind_arg {
  using type = CvTD; // (7.4)
};
template <class CvTD, class... U>
  requires ycxx::detail::is_reference_wrapper<std::remove_cvref_t<CvTD>>
struct bind_arg<CvTD, U...> { // (7.1)
  using type = typename std::remove_cvref_t<CvTD>::type&;
};
template <class CvTD, class... U>
  requires(!ycxx::detail::is_reference_wrapper<std::remove_cvref_t<CvTD>>) &&
          std::is_bind_expression_v<std::remove_cvref_t<CvTD>> && std::is_invocable_v<CvTD, U...> &&
          (!std::is_void_v<std::invoke_result_t<CvTD, U...>>)
struct bind_arg<CvTD, U...> { // (7.2)
  using type = std::invoke_result_t<CvTD, U...>&&;
};
template <class CvTD, class... U>
  requires(!ycxx::detail::is_reference_wrapper<std::remove_cvref_t<CvTD>>) &&
          (!std::is_bind_expression_v<std::remove_cvref_t<CvTD>>) && (std::is_placeholder_v<std::remove_cvref_t<CvTD>> > 0) &&
          (std::is_placeholder_v<std::remove_cvref_t<CvTD>> <= static_cast<int>(sizeof...(U)))
struct bind_arg<CvTD, U...> { // (7.3)
  using type = U...[std::is_placeholder_v<std::remove_cvref_t<CvTD>> - 1]&&;
};
// A bind expression that cannot be called with U... (or returns void: V_i would be void&&), or a
// placeholder beyond the arguments: no type, so the wrapper's operator() is not viable.
template <class CvTD, class... U>
  requires(!ycxx::detail::is_reference_wrapper<std::remove_cvref_t<CvTD>>) &&
          ((std::is_bind_expression_v<std::remove_cvref_t<CvTD>> &&
            (!std::is_invocable_v<CvTD, U...> || std::is_void_v<std::invoke_result_t<CvTD, U...>>)) ||
           (!std::is_bind_expression_v<std::remove_cvref_t<CvTD>> &&
            std::is_placeholder_v<std::remove_cvref_t<CvTD>> > static_cast<int>(sizeof...(U))))
struct bind_arg<CvTD, U...> {};

template <class CvTD, class... U>
using bind_arg_t = typename bind_arg<CvTD, U...>::type;
template <class CvTD, class... U>
concept has_bind_arg = requires { typename bind_arg<CvTD, U...>::type; };

struct bind_no_r {};

template <class R, class FD, class... TD>
struct binder {
  FD fd;
  std::tuple<TD...> bound;

  template <class F, class... B>
  constexpr binder(wrapper_init_t, F&& f, B&&... b) : fd(static_cast<F&&>(f)), bound(static_cast<B&&>(b)...) {}

  template <class Self>
  using cv = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const int, int>;
  template <class Self, class T>
  using cv_ref = ycxx::detail::copy_cv<cv<Self>, T>&;

  template <class Self, class... U>
  static constexpr bool callable = [] {
    if constexpr ((has_bind_arg<cv_ref<Self, TD>, U...> && ...)) {
      if constexpr (std::is_same_v<R, bind_no_r>)
        return std::is_invocable_v<cv_ref<Self, FD>, bind_arg_t<cv_ref<Self, TD>, U...>...>;
      else
        return std::is_invocable_r_v<R, cv_ref<Self, FD>, bind_arg_t<cv_ref<Self, TD>, U...>...>;
    } else {
      return false;
    }
  }();

  // The call is expression-equivalent to INVOKE (or INVOKE<R>) on the bound values, so it is
  // noexcept when that call and every nested bind call (7.2) are.
  template <class CvTD, class... U>
  static constexpr bool nested_nothrow = [] {
    if constexpr (!ycxx::detail::is_reference_wrapper<std::remove_cvref_t<CvTD>> &&
                  std::is_bind_expression_v<std::remove_cvref_t<CvTD>>)
      return std::is_nothrow_invocable_v<CvTD, U...>;
    else
      return true;
  }();
  template <class Self, class... U>
  static constexpr bool nothrow = [] {
    if constexpr (callable<Self, U...>) {
      if constexpr (std::is_same_v<R, bind_no_r>)
        return std::is_nothrow_invocable_v<cv_ref<Self, FD>, bind_arg_t<cv_ref<Self, TD>, U...>...> &&
               (nested_nothrow<cv_ref<Self, TD>, U...> && ...);
      else
        return std::is_nothrow_invocable_r_v<R, cv_ref<Self, FD>, bind_arg_t<cv_ref<Self, TD>, U...>...> &&
               (nested_nothrow<cv_ref<Self, TD>, U...> && ...);
    } else {
      return false;
    }
  }();

  template <class Self, class... U>
    requires(!std::is_volatile_v<std::remove_reference_t<Self>>) && wrapper_castable<Self&, binder> &&
            callable<Self, U...>
  constexpr decltype(auto) operator()(this Self&& selfd, U&&... u) noexcept(nothrow<Self, U...>) {
    auto& self = (ycxx::detail::copy_cvref<Self&, binder>)selfd; // see partial_wrapper
    // COMPILER-BUG(gcc): GCC 16 diagnoses TD...[I] inside an expansion over an empty I pack
    // ("cannot index an empty pack"), so the element types come from tuple_element_t.
    return [&]<std::size_t... I>(std::index_sequence<I...>) -> decltype(auto) {
      if constexpr (std::is_same_v<R, bind_no_r>)
        return ::ycxx::detail::invoke(static_cast<cv_ref<Self, FD>>(self.fd),
                                      static_cast<bind_arg_t<cv_ref<Self, std::tuple_element_t<I, std::tuple<TD...>>>, U...>>(
                                          ::ycxx::detail::bind_value<cv_ref<Self, std::tuple_element_t<I, std::tuple<TD...>>>>(std::get<I>(self.bound), static_cast<U&&>(u)...))...);
      else
        return ::ycxx::detail::invoke_r<R>(static_cast<cv_ref<Self, FD>>(self.fd),
                                           static_cast<bind_arg_t<cv_ref<Self, std::tuple_element_t<I, std::tuple<TD...>>>, U...>>(::ycxx::detail::bind_value<cv_ref<Self, std::tuple_element_t<I, std::tuple<TD...>>>>(
                                               std::get<I>(self.bound), static_cast<U&&>(u)...))...);
    }(std::index_sequence_for<TD...>{});
  }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

template <class R, class FD, class... TD>
struct is_bind_expression<ycxx::adl_free::binder<R, FD, TD...>> : true_type {};
template <int J>
struct is_placeholder<ycxx::adl_free::placeholder<J>> : integral_constant<int, J> {};
// decltype(placeholders::_J) is const-qualified, and "the type of _J" must be recognised; the
// same for const bind expressions.
template <class T>
struct is_placeholder<const T> : is_placeholder<T> {};
template <class T>
struct is_bind_expression<const T> : is_bind_expression<T> {};
template <class T>
struct is_bind_expression<volatile T> : is_bind_expression<T> {};
template <class T>
struct is_bind_expression<const volatile T> : is_bind_expression<T> {};

namespace placeholders {
inline constexpr ycxx::adl_free::placeholder<1> _1{};
inline constexpr ycxx::adl_free::placeholder<2> _2{};
inline constexpr ycxx::adl_free::placeholder<3> _3{};
inline constexpr ycxx::adl_free::placeholder<4> _4{};
inline constexpr ycxx::adl_free::placeholder<5> _5{};
inline constexpr ycxx::adl_free::placeholder<6> _6{};
inline constexpr ycxx::adl_free::placeholder<7> _7{};
inline constexpr ycxx::adl_free::placeholder<8> _8{};
inline constexpr ycxx::adl_free::placeholder<9> _9{};
inline constexpr ycxx::adl_free::placeholder<10> _10{};
} // namespace placeholders

// ---- [func.bind.bind] ----
template <class F, class... BoundArgs>
constexpr auto bind(F&& f, BoundArgs&&... bound_args) {
  if constexpr (!(is_constructible_v<decay_t<F>, F> && (is_constructible_v<decay_t<BoundArgs>, BoundArgs> && ...)))
    static_assert(false, "std::bind: the target and bound arguments must be constructible from the arguments");
  else
    return ycxx::adl_free::binder<ycxx::adl_free::bind_no_r, decay_t<F>, decay_t<BoundArgs>...>(
        ycxx::adl_free::wrapper_init_t{}, static_cast<F&&>(f), static_cast<BoundArgs&&>(bound_args)...);
}
template <class R, class F, class... BoundArgs>
constexpr auto bind(F&& f, BoundArgs&&... bound_args) {
  if constexpr (!(is_constructible_v<decay_t<F>, F> && (is_constructible_v<decay_t<BoundArgs>, BoundArgs> && ...)))
    static_assert(false, "std::bind: the target and bound arguments must be constructible from the arguments");
  else
    return ycxx::adl_free::binder<R, decay_t<F>, decay_t<BoundArgs>...>(ycxx::adl_free::wrapper_init_t{}, static_cast<F&&>(f),
                                                                        static_cast<BoundArgs&&>(bound_args)...);
}

} // namespace std
