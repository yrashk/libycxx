// libycxx core: the INVOKE protocol ([func.require]) and its traits.
#pragma once

#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/move.hpp>

namespace std {
template <class T>
class reference_wrapper;
} // namespace std

namespace ycxx::detail {

template <class T>
inline constexpr bool is_reference_wrapper = false;
template <class T>
inline constexpr bool is_reference_wrapper<std::reference_wrapper<T>> = true;

template <class M>
struct member_pointer_class;
template <class M, class C>
struct member_pointer_class<M C::*> {
  using type = C;
  using member = M;
};

// Dispatch tag computed from the callable and the first argument.
enum class invoke_kind { plain, mem_fn_ref, mem_fn_rw, mem_fn_ptr, mem_obj_ref, mem_obj_rw, mem_obj_ptr };

template <class F, class... Args>
consteval invoke_kind classify_invoke() {
  using FD = __remove_cvref(F);
  if constexpr (__is_member_pointer(FD) && sizeof...(Args) > 0) {
    using C = typename member_pointer_class<FD>::type;
    using T1 = __remove_cvref(Args...[0]);
    constexpr bool fn = __is_member_function_pointer(FD);
    if constexpr (__is_same(C, T1) || __is_base_of(C, T1))
      return fn ? invoke_kind::mem_fn_ref : invoke_kind::mem_obj_ref;
    else if constexpr (is_reference_wrapper<T1>)
      return fn ? invoke_kind::mem_fn_rw : invoke_kind::mem_obj_rw;
    else
      return fn ? invoke_kind::mem_fn_ptr : invoke_kind::mem_obj_ptr;
  } else {
    return invoke_kind::plain;
  }
}

template <invoke_kind K>
struct invoker;

template <>
struct invoker<invoke_kind::plain> {
  template <class F, class... Args>
  static constexpr auto call(F&& f, Args&&... args) noexcept(noexcept(static_cast<decltype(f)&&>(f)(static_cast<decltype(args)&&>(args)...)))
      -> decltype(static_cast<decltype(f)&&>(f)(static_cast<decltype(args)&&>(args)...)) {
    return static_cast<decltype(f)&&>(f)(static_cast<decltype(args)&&>(args)...);
  }
};
template <>
struct invoker<invoke_kind::mem_fn_ref> {
  template <class F, class T1, class... Args>
  static constexpr auto call(F f, T1&& t1, Args&&... args) noexcept(noexcept((static_cast<decltype(t1)&&>(t1).*f)(static_cast<decltype(args)&&>(args)...)))
      -> decltype((static_cast<decltype(t1)&&>(t1).*f)(static_cast<decltype(args)&&>(args)...)) {
    return (static_cast<decltype(t1)&&>(t1).*f)(static_cast<decltype(args)&&>(args)...);
  }
};
template <>
struct invoker<invoke_kind::mem_fn_rw> {
  template <class F, class T1, class... Args>
  static constexpr auto call(F f, T1&& t1, Args&&... args) noexcept(noexcept((t1.get().*f)(static_cast<decltype(args)&&>(args)...)))
      -> decltype((t1.get().*f)(static_cast<decltype(args)&&>(args)...)) {
    return (t1.get().*f)(static_cast<decltype(args)&&>(args)...);
  }
};
template <>
struct invoker<invoke_kind::mem_fn_ptr> {
  template <class F, class T1, class... Args>
  static constexpr auto call(F f, T1&& t1, Args&&... args) noexcept(noexcept(((*static_cast<decltype(t1)&&>(t1)).*f)(static_cast<decltype(args)&&>(args)...)))
      -> decltype(((*static_cast<decltype(t1)&&>(t1)).*f)(static_cast<decltype(args)&&>(args)...)) {
    return ((*static_cast<decltype(t1)&&>(t1)).*f)(static_cast<decltype(args)&&>(args)...);
  }
};
template <>
struct invoker<invoke_kind::mem_obj_ref> {
  template <class F, class T1>
  static constexpr auto call(F f, T1&& t1) noexcept -> decltype(static_cast<decltype(t1)&&>(t1).*f) {
    return static_cast<decltype(t1)&&>(t1).*f;
  }
};
template <>
struct invoker<invoke_kind::mem_obj_rw> {
  template <class F, class T1>
  static constexpr auto call(F f, T1&& t1) noexcept -> decltype(t1.get().*f) {
    return t1.get().*f;
  }
};
template <>
struct invoker<invoke_kind::mem_obj_ptr> {
  template <class F, class T1>
  static constexpr auto call(F f, T1&& t1) noexcept(noexcept((*static_cast<decltype(t1)&&>(t1)).*f)) -> decltype((*static_cast<decltype(t1)&&>(t1)).*f) {
    return (*static_cast<decltype(t1)&&>(t1)).*f;
  }
};

template <class F, class... Args>
using invoker_for = invoker<classify_invoke<F, Args...>()>;

// The INVOKE expression itself.
template <class F, class... Args>
[[gnu::always_inline]] constexpr auto invoke(F&& f, Args&&... args) noexcept(
    noexcept(invoker_for<F, Args...>::call(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...)))
    -> decltype(invoker_for<F, Args...>::call(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...)) {
  return invoker_for<F, Args...>::call(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...);
}

template <class F, class... Args>
concept invocable_ = requires(F&& f, Args&&... args) { ::ycxx::detail::invoke(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...); };

template <class F, class... Args>
concept nothrow_invocable_ = requires(F&& f, Args&&... args) {
  { ::ycxx::detail::invoke(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...) } noexcept;
};

template <class F, class... Args>
using invoke_result_t = decltype(::ycxx::detail::invoke(std::declval<F>(), std::declval<Args>()...));

// Implicit conversion test that also works with non-movable prvalues (guaranteed elision).
template <class T>
void implicitly_convert_to(T) noexcept;

template <class R, class F, class... Args>
consteval bool is_invocable_r_impl() {
  if constexpr (!invocable_<F, Args...>)
    return false;
  else if constexpr (::ycxx::detail::is_void_v<R>)
    return true;
  else
    return requires { implicitly_convert_to<R>(::ycxx::detail::invoke(std::declval<F>(), std::declval<Args>()...)); } &&
           !__reference_converts_from_temporary(R, invoke_result_t<F, Args...>);
}

template <class R, class F, class... Args>
consteval bool is_nothrow_invocable_r_impl() {
  if constexpr (!nothrow_invocable_<F, Args...>)
    return false;
  else if constexpr (::ycxx::detail::is_void_v<R>)
    return true;
  else
    return requires {
      { implicitly_convert_to<R>(::ycxx::detail::invoke(std::declval<F>(), std::declval<Args>()...)) } noexcept;
    } && !__reference_converts_from_temporary(R, invoke_result_t<F, Args...>);
}

// INVOKE<R>
template <class R, class F, class... Args>
[[gnu::always_inline]] constexpr R invoke_r(F&& f, Args&&... args) noexcept(is_nothrow_invocable_r_impl<R, F, Args...>()) {
  if constexpr (::ycxx::detail::is_void_v<R>)
    static_cast<void>(::ycxx::detail::invoke(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...));
  else
    return ::ycxx::detail::invoke(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...);
}

} // namespace ycxx::detail

namespace std {

template <class F, class... Args>
struct invoke_result {};
template <class F, class... Args>
  requires ycxx::detail::invocable_<F, Args...>
struct invoke_result<F, Args...> {
  using type = ycxx::detail::invoke_result_t<F, Args...>;
};
template <class F, class... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;

template <class F, class... Args>
struct is_invocable : bool_constant<ycxx::detail::invocable_<F, Args...>> {};
template <class F, class... Args>
inline constexpr bool is_invocable_v = ycxx::detail::invocable_<F, Args...>;

template <class R, class F, class... Args>
struct is_invocable_r : bool_constant<ycxx::detail::is_invocable_r_impl<R, F, Args...>()> {};
template <class R, class F, class... Args>
inline constexpr bool is_invocable_r_v = ycxx::detail::is_invocable_r_impl<R, F, Args...>();

template <class F, class... Args>
struct is_nothrow_invocable : bool_constant<ycxx::detail::nothrow_invocable_<F, Args...>> {};
template <class F, class... Args>
inline constexpr bool is_nothrow_invocable_v = ycxx::detail::nothrow_invocable_<F, Args...>;

template <class R, class F, class... Args>
struct is_nothrow_invocable_r : bool_constant<ycxx::detail::is_nothrow_invocable_r_impl<R, F, Args...>()> {};
template <class R, class F, class... Args>
inline constexpr bool is_nothrow_invocable_r_v = ycxx::detail::is_nothrow_invocable_r_impl<R, F, Args...>();

template <class F, class... Args>
  requires is_invocable_v<F, Args...>
constexpr invoke_result_t<F, Args...> invoke(F&& f, Args&&... args) noexcept(is_nothrow_invocable_v<F, Args...>) {
  return ycxx::detail::invoke(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...);
}

template <class R, class F, class... Args>
  requires is_invocable_r_v<R, F, Args...>
constexpr R invoke_r(F&& f, Args&&... args) noexcept(is_nothrow_invocable_r_v<R, F, Args...>) {
  return ycxx::detail::invoke_r<R>(static_cast<decltype(f)&&>(f), static_cast<decltype(args)&&>(args)...);
}

} // namespace std

