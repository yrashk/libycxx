// libycxx core: smart pointer adaptors for output-pointer parameters ([smartptr.adapt]):
// out_ptr_t / out_ptr and inout_ptr_t / inout_ptr.
//
// operator void**() returns the address of the stored Pointer reinterpreted as void**, the
// strategy [out.ptr.t] Note 3 mentions; it is valid because Pointer is then an object pointer
// type with the same representation as void* on the supported targets.
#pragma once

#include <ycxx/core/tuple.hpp>
#include <ycxx/core/shared_ptr.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// POINTER_OF(T) and POINTER_OF_OR(T, U) ([memory.syn]/2-3). pointer_of_or<T, void> is void when
// POINTER_OF(T) is not valid.
template <class _Tp, class _Up>
consteval auto __pointer_of_or_impl() {
  if constexpr (requires { typename _Tp::pointer; })
    return std::type_identity<typename _Tp::pointer>();
  else if constexpr (requires { typename _Tp::element_type; })
    return std::type_identity<typename _Tp::element_type*>();
  else if constexpr (requires { typename std::pointer_traits<_Tp>::element_type; })
    return std::type_identity<typename std::pointer_traits<_Tp>::element_type*>();
  else
    return std::type_identity<_Up>();
}
template <class _Tp, class _Up>
using __pointer_of_or = typename decltype(__pointer_of_or_impl<_Tp, _Up>())::type;

// The P of [out.ptr]/1 and [inout.ptr]/1: Pointer, or POINTER_OF(Smart) when Pointer is void.
template <class _Pointer, class _Smart>
using __out_ptr_pointer = std::conditional_t<std::is_void_v<_Pointer>, __pointer_of_or<_Smart, void>, _Pointer>;

template <class _Tp>
inline constexpr bool __is_shared_ptr = false;
template <class _Tp>
inline constexpr bool __is_shared_ptr<std::shared_ptr<_Tp>> = true;

// The destructors' final step: s.reset(static_cast<SP>(p), args...) if that is well-formed,
// otherwise s = Smart(static_cast<SP>(p), args...).
template <class _Smart, class _SP, class _Pointer, class _Tuple, std::size_t... _Ip>
constexpr void __out_ptr_store(_Smart& s, _Pointer& p, _Tuple& a, std::index_sequence<_Ip...>) {
  if constexpr (requires { s.reset(static_cast<_SP>(p), std::get<_Ip>(static_cast<_Tuple&&>(a))...); })
    s.reset(static_cast<_SP>(p), std::get<_Ip>(static_cast<_Tuple&&>(a))...);
  else if constexpr (std::is_constructible_v<_Smart, _SP, std::tuple_element_t<_Ip, _Tuple>...>)
    s = _Smart(static_cast<_SP>(p), std::get<_Ip>(static_cast<_Tuple&&>(a))...);
  else
    static_assert(__always_false<_Smart>, "out_ptr/inout_ptr: Smart cannot be reset from the pointer and arguments");
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [out.ptr.t]
template <class _Smart, class _Pointer, class... _Args>
class out_ptr_t {
  static_assert(!(__ycxx::__detail::__is_shared_ptr<_Smart> && sizeof...(_Args) == 0),
                "std::out_ptr: a shared_ptr needs a deleter argument");

  _Smart& __s_;
  tuple<_Args...> __a_;
  _Pointer __p_;

public:
  constexpr explicit out_ptr_t(_Smart& __smart, _Args... __args)
      : __s_(__smart), __a_(static_cast<_Args&&>(__args)...), __p_() {
    if constexpr (requires { __s_.reset(); })
      __s_.reset();
    else if constexpr (is_constructible_v<_Smart>)
      __s_ = _Smart();
    else
      static_assert(__ycxx::__detail::__always_false<_Smart>, "std::out_ptr: Smart can be neither reset nor value-initialized");
  }
  out_ptr_t(const out_ptr_t&) = delete;

  constexpr ~out_ptr_t() {
    if (__p_)
      __ycxx::__detail::__out_ptr_store<_Smart, __ycxx::__detail::__pointer_of_or<_Smart, _Pointer>>(__s_, __p_, __a_,
                                                                                       index_sequence_for<_Args...>());
  }

  constexpr operator _Pointer*() const noexcept { return __builtin_addressof(const_cast<_Pointer&>(__p_)); }
  operator void**() const noexcept
    requires(!is_same_v<_Pointer, void*>)
  {
    static_assert(is_pointer_v<_Pointer>, "std::out_ptr_t::operator void**: Pointer must be a pointer type");
    return reinterpret_cast<void**>(__builtin_addressof(const_cast<_Pointer&>(__p_)));
  }
};

// [out.ptr]
template <class _Pointer = void, class _Smart, class... _Args>
constexpr auto out_ptr(_Smart& s, _Args&&... __args) {
  using _Pp = __ycxx::__detail::__out_ptr_pointer<_Pointer, _Smart>;
  static_assert(!is_void_v<_Pp>, "out_ptr/inout_ptr: POINTER_OF(Smart) is not valid; name the Pointer type");
  return out_ptr_t<_Smart, _Pp, _Args&&...>(s, static_cast<_Args&&>(__args)...);
}

// [inout.ptr.t]
template <class _Smart, class _Pointer, class... _Args>
class inout_ptr_t {
  static_assert(!__ycxx::__detail::__is_shared_ptr<_Smart>, "std::inout_ptr: shared_ptr cannot release ownership");

  _Smart& __s_;
  tuple<_Args...> __a_;
  _Pointer __p_;

  static constexpr _Pointer __initial(_Smart& __smart) {
    if constexpr (is_pointer_v<_Smart>)
      return __smart;
    else
      return __smart.get();
  }

public:
  constexpr explicit inout_ptr_t(_Smart& __smart, _Args... __args)
      : __s_(__smart), __a_(static_cast<_Args&&>(__args)...), __p_(__initial(__smart)) {}
  inout_ptr_t(const inout_ptr_t&) = delete;

  // s.release() runs here (the release-statement of [inout.ptr.t]/10), not in the constructor.
  constexpr ~inout_ptr_t() {
    using _SP = __ycxx::__detail::__pointer_of_or<_Smart, _Pointer>;
    if constexpr (is_pointer_v<_Smart>) {
      [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        __s_ = _Smart(static_cast<_SP>(__p_), std::get<_Ip>(static_cast<tuple<_Args...>&&>(__a_))...);
      }(index_sequence_for<_Args...>());
    } else {
      (void)__s_.release();
      if (__p_)
        __ycxx::__detail::__out_ptr_store<_Smart, _SP>(__s_, __p_, __a_, index_sequence_for<_Args...>());
    }
  }

  constexpr operator _Pointer*() const noexcept { return __builtin_addressof(const_cast<_Pointer&>(__p_)); }
  operator void**() const noexcept
    requires(!is_same_v<_Pointer, void*>)
  {
    static_assert(is_pointer_v<_Pointer>, "std::inout_ptr_t::operator void**: Pointer must be a pointer type");
    return reinterpret_cast<void**>(__builtin_addressof(const_cast<_Pointer&>(__p_)));
  }
};

// [inout.ptr]
template <class _Pointer = void, class _Smart, class... _Args>
constexpr auto inout_ptr(_Smart& s, _Args&&... __args) {
  using _Pp = __ycxx::__detail::__out_ptr_pointer<_Pointer, _Smart>;
  static_assert(!is_void_v<_Pp>, "out_ptr/inout_ptr: POINTER_OF(Smart) is not valid; name the Pointer type");
  return inout_ptr_t<_Smart, _Pp, _Args&&...>(s, static_cast<_Args&&>(__args)...);
}

} // namespace std
