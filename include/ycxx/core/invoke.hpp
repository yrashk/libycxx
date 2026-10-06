// libycxx core: the INVOKE protocol ([func.require]) and its traits.
#pragma once

#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/move.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
class reference_wrapper;
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_reference_wrapper = false;
template <class _Tp>
inline constexpr bool __is_reference_wrapper<std::reference_wrapper<_Tp>> = true;

template <class _Mp>
struct __member_pointer_class;
template <class _Mp, class _Cp>
struct __member_pointer_class<_Mp _Cp::*> {
  using type = _Cp;
  using __member = _Mp;
};

// Dispatch tag computed from the callable and the first argument.
enum class __invoke_kind { __plain, __mem_fn_ref, __mem_fn_rw, __mem_fn_ptr, __mem_obj_ref, __mem_obj_rw, __mem_obj_ptr };

template <class _Fp, class... _Args>
consteval __invoke_kind __classify_invoke() {
  using _FD = __remove_cvref(_Fp);
  if constexpr (__is_member_pointer(_FD) && sizeof...(_Args) > 0) {
    using _Cp = typename __member_pointer_class<_FD>::type;
    using _T1 = __remove_cvref(_Args...[0]);
    constexpr bool __fn = __is_member_function_pointer(_FD);
    if constexpr (__is_same(_Cp, _T1) || __is_base_of(_Cp, _T1))
      return __fn ? __invoke_kind::__mem_fn_ref : __invoke_kind::__mem_obj_ref;
    else if constexpr (__is_reference_wrapper<_T1>)
      return __fn ? __invoke_kind::__mem_fn_rw : __invoke_kind::__mem_obj_rw;
    else
      return __fn ? __invoke_kind::__mem_fn_ptr : __invoke_kind::__mem_obj_ptr;
  } else {
    return __invoke_kind::__plain;
  }
}

template <__invoke_kind _Kp>
struct __invoker;

template <>
struct __invoker<__invoke_kind::__plain> {
  template <class _Fp, class... _Args>
  static constexpr auto __call(_Fp&& __f, _Args&&... __args) noexcept(noexcept(static_cast<decltype(__f)&&>(__f)(static_cast<decltype(__args)&&>(__args)...)))
      -> decltype(static_cast<decltype(__f)&&>(__f)(static_cast<decltype(__args)&&>(__args)...)) {
    return static_cast<decltype(__f)&&>(__f)(static_cast<decltype(__args)&&>(__args)...);
  }
};
template <>
struct __invoker<__invoke_kind::__mem_fn_ref> {
  template <class _Fp, class _T1, class... _Args>
  static constexpr auto __call(_Fp __f, _T1&& __t1, _Args&&... __args) noexcept(noexcept((static_cast<decltype(__t1)&&>(__t1).*__f)(static_cast<decltype(__args)&&>(__args)...)))
      -> decltype((static_cast<decltype(__t1)&&>(__t1).*__f)(static_cast<decltype(__args)&&>(__args)...)) {
    return (static_cast<decltype(__t1)&&>(__t1).*__f)(static_cast<decltype(__args)&&>(__args)...);
  }
};
template <>
struct __invoker<__invoke_kind::__mem_fn_rw> {
  template <class _Fp, class _T1, class... _Args>
  static constexpr auto __call(_Fp __f, _T1&& __t1, _Args&&... __args) noexcept(noexcept((__t1.get().*__f)(static_cast<decltype(__args)&&>(__args)...)))
      -> decltype((__t1.get().*__f)(static_cast<decltype(__args)&&>(__args)...)) {
    return (__t1.get().*__f)(static_cast<decltype(__args)&&>(__args)...);
  }
};
template <>
struct __invoker<__invoke_kind::__mem_fn_ptr> {
  template <class _Fp, class _T1, class... _Args>
  static constexpr auto __call(_Fp __f, _T1&& __t1, _Args&&... __args) noexcept(noexcept(((*static_cast<decltype(__t1)&&>(__t1)).*__f)(static_cast<decltype(__args)&&>(__args)...)))
      -> decltype(((*static_cast<decltype(__t1)&&>(__t1)).*__f)(static_cast<decltype(__args)&&>(__args)...)) {
    return ((*static_cast<decltype(__t1)&&>(__t1)).*__f)(static_cast<decltype(__args)&&>(__args)...);
  }
};
template <>
struct __invoker<__invoke_kind::__mem_obj_ref> {
  template <class _Fp, class _T1>
  static constexpr auto __call(_Fp __f, _T1&& __t1) noexcept -> decltype(static_cast<decltype(__t1)&&>(__t1).*__f) {
    return static_cast<decltype(__t1)&&>(__t1).*__f;
  }
};
template <>
struct __invoker<__invoke_kind::__mem_obj_rw> {
  template <class _Fp, class _T1>
  static constexpr auto __call(_Fp __f, _T1&& __t1) noexcept -> decltype(__t1.get().*__f) {
    return __t1.get().*__f;
  }
};
template <>
struct __invoker<__invoke_kind::__mem_obj_ptr> {
  template <class _Fp, class _T1>
  static constexpr auto __call(_Fp __f, _T1&& __t1) noexcept(noexcept((*static_cast<decltype(__t1)&&>(__t1)).*__f)) -> decltype((*static_cast<decltype(__t1)&&>(__t1)).*__f) {
    return (*static_cast<decltype(__t1)&&>(__t1)).*__f;
  }
};

template <class _Fp, class... _Args>
using __invoker_for = __invoker<__classify_invoke<_Fp, _Args...>()>;

// The INVOKE expression itself.
template <class _Fp, class... _Args>
[[__gnu__::__always_inline__]] constexpr auto invoke(_Fp&& __f, _Args&&... __args) noexcept(
    noexcept(__invoker_for<_Fp, _Args...>::__call(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...)))
    -> decltype(__invoker_for<_Fp, _Args...>::__call(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...)) {
  return __invoker_for<_Fp, _Args...>::__call(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...);
}

template <class _Fp, class... _Args>
concept __invocable_ = requires(_Fp&& __f, _Args&&... __args) { ::__ycxx::__detail::invoke(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...); };

template <class _Fp, class... _Args>
concept __nothrow_invocable_ = requires(_Fp&& __f, _Args&&... __args) {
  { ::__ycxx::__detail::invoke(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...) } noexcept;
};

template <class _Fp, class... _Args>
using invoke_result_t = decltype(::__ycxx::__detail::invoke(std::declval<_Fp>(), std::declval<_Args>()...));

// Implicit conversion test that also works with non-movable prvalues (guaranteed elision).
template <class _Tp>
void __implicitly_convert_to(_Tp) noexcept;

template <class _Rp, class _Fp, class... _Args>
consteval bool __is_invocable_r_impl() {
  if constexpr (!__invocable_<_Fp, _Args...>)
    return false;
  else if constexpr (::__ycxx::__detail::is_void_v<_Rp>)
    return true;
  else
    return requires { ::__ycxx::__detail::__implicitly_convert_to<_Rp>(::__ycxx::__detail::invoke(std::declval<_Fp>(), std::declval<_Args>()...)); } &&
           !__reference_converts_from_temporary(_Rp, invoke_result_t<_Fp, _Args...>);
}

template <class _Rp, class _Fp, class... _Args>
consteval bool __is_nothrow_invocable_r_impl() {
  if constexpr (!__nothrow_invocable_<_Fp, _Args...>)
    return false;
  else if constexpr (::__ycxx::__detail::is_void_v<_Rp>)
    return true;
  else
    return requires {
      { ::__ycxx::__detail::__implicitly_convert_to<_Rp>(::__ycxx::__detail::invoke(std::declval<_Fp>(), std::declval<_Args>()...)) } noexcept;
    } && !__reference_converts_from_temporary(_Rp, invoke_result_t<_Fp, _Args...>);
}

// INVOKE<R>
template <class _Rp, class _Fp, class... _Args>
[[__gnu__::__always_inline__]] constexpr _Rp invoke_r(_Fp&& __f, _Args&&... __args) noexcept(__is_nothrow_invocable_r_impl<_Rp, _Fp, _Args...>()) {
  if constexpr (::__ycxx::__detail::is_void_v<_Rp>)
    static_cast<void>(::__ycxx::__detail::invoke(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...));
  else
    return ::__ycxx::__detail::invoke(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Fp, class... _Args>
struct invoke_result {};
template <class _Fp, class... _Args>
  requires __ycxx::__detail::__invocable_<_Fp, _Args...>
struct invoke_result<_Fp, _Args...> {
  using type = __ycxx::__detail::invoke_result_t<_Fp, _Args...>;
};
template <class _Fp, class... _Args>
using invoke_result_t = typename invoke_result<_Fp, _Args...>::type;

template <class _Fp, class... _Args>
struct is_invocable : bool_constant<__ycxx::__detail::__invocable_<_Fp, _Args...>> {};
template <class _Fp, class... _Args>
inline constexpr bool is_invocable_v = __ycxx::__detail::__invocable_<_Fp, _Args...>;

template <class _Rp, class _Fp, class... _Args>
struct is_invocable_r : bool_constant<__ycxx::__detail::__is_invocable_r_impl<_Rp, _Fp, _Args...>()> {};
template <class _Rp, class _Fp, class... _Args>
inline constexpr bool is_invocable_r_v = __ycxx::__detail::__is_invocable_r_impl<_Rp, _Fp, _Args...>();

template <class _Fp, class... _Args>
struct is_nothrow_invocable : bool_constant<__ycxx::__detail::__nothrow_invocable_<_Fp, _Args...>> {};
template <class _Fp, class... _Args>
inline constexpr bool is_nothrow_invocable_v = __ycxx::__detail::__nothrow_invocable_<_Fp, _Args...>;

template <class _Rp, class _Fp, class... _Args>
struct is_nothrow_invocable_r : bool_constant<__ycxx::__detail::__is_nothrow_invocable_r_impl<_Rp, _Fp, _Args...>()> {};
template <class _Rp, class _Fp, class... _Args>
inline constexpr bool is_nothrow_invocable_r_v = __ycxx::__detail::__is_nothrow_invocable_r_impl<_Rp, _Fp, _Args...>();

template <class _Fp, class... _Args>
  requires is_invocable_v<_Fp, _Args...>
constexpr invoke_result_t<_Fp, _Args...> invoke(_Fp&& __f, _Args&&... __args) noexcept(is_nothrow_invocable_v<_Fp, _Args...>) {
  return __ycxx::__detail::invoke(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...);
}

template <class _Rp, class _Fp, class... _Args>
  requires is_invocable_r_v<_Rp, _Fp, _Args...>
constexpr _Rp invoke_r(_Fp&& __f, _Args&&... __args) noexcept(is_nothrow_invocable_r_v<_Rp, _Fp, _Args...>) {
  return __ycxx::__detail::invoke_r<_Rp>(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...);
}

} // namespace std

