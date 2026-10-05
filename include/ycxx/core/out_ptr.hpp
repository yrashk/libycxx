// libycxx core: smart pointer adaptors for output-pointer parameters ([smartptr.adapt]):
// out_ptr_t / out_ptr and inout_ptr_t / inout_ptr.
//
// operator void**() returns the address of the stored Pointer reinterpreted as void**, the
// strategy [out.ptr.t] Note 3 mentions; it is valid because Pointer is then an object pointer
// type with the same representation as void* on the supported targets.
#pragma once

#include <ycxx/core/tuple.hpp>
#include <ycxx/core/shared_ptr.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// POINTER_OF(T) and POINTER_OF_OR(T, U) ([memory.syn]/2-3). pointer_of_or<T, void> is void when
// POINTER_OF(T) is not valid.
template <class T, class U>
consteval auto pointer_of_or_impl() {
  if constexpr (requires { typename T::pointer; })
    return std::type_identity<typename T::pointer>();
  else if constexpr (requires { typename T::element_type; })
    return std::type_identity<typename T::element_type*>();
  else if constexpr (requires { typename std::pointer_traits<T>::element_type; })
    return std::type_identity<typename std::pointer_traits<T>::element_type*>();
  else
    return std::type_identity<U>();
}
template <class T, class U>
using pointer_of_or = typename decltype(pointer_of_or_impl<T, U>())::type;

// The P of [out.ptr]/1 and [inout.ptr]/1: Pointer, or POINTER_OF(Smart) when Pointer is void.
template <class Pointer, class Smart>
using out_ptr_pointer = std::conditional_t<std::is_void_v<Pointer>, pointer_of_or<Smart, void>, Pointer>;

template <class T>
inline constexpr bool is_shared_ptr = false;
template <class T>
inline constexpr bool is_shared_ptr<std::shared_ptr<T>> = true;

// The destructors' final step: s.reset(static_cast<SP>(p), args...) if that is well-formed,
// otherwise s = Smart(static_cast<SP>(p), args...).
template <class Smart, class SP, class Pointer, class Tuple, std::size_t... I>
constexpr void out_ptr_store(Smart& s, Pointer& p, Tuple& a, std::index_sequence<I...>) {
  if constexpr (requires { s.reset(static_cast<SP>(p), std::get<I>(static_cast<Tuple&&>(a))...); })
    s.reset(static_cast<SP>(p), std::get<I>(static_cast<Tuple&&>(a))...);
  else if constexpr (std::is_constructible_v<Smart, SP, std::tuple_element_t<I, Tuple>...>)
    s = Smart(static_cast<SP>(p), std::get<I>(static_cast<Tuple&&>(a))...);
  else
    static_assert(always_false<Smart>, "out_ptr/inout_ptr: Smart cannot be reset from the pointer and arguments");
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [out.ptr.t]
template <class Smart, class Pointer, class... Args>
class out_ptr_t {
  static_assert(!(ycxx::detail::is_shared_ptr<Smart> && sizeof...(Args) == 0),
                "std::out_ptr: a shared_ptr needs a deleter argument");

  Smart& s_;
  tuple<Args...> a_;
  Pointer p_;

public:
  constexpr explicit out_ptr_t(Smart& smart, Args... args)
      : s_(smart), a_(static_cast<Args&&>(args)...), p_() {
    if constexpr (requires { s_.reset(); })
      s_.reset();
    else if constexpr (is_constructible_v<Smart>)
      s_ = Smart();
    else
      static_assert(ycxx::detail::always_false<Smart>, "std::out_ptr: Smart can be neither reset nor value-initialized");
  }
  out_ptr_t(const out_ptr_t&) = delete;

  constexpr ~out_ptr_t() {
    if (p_)
      ycxx::detail::out_ptr_store<Smart, ycxx::detail::pointer_of_or<Smart, Pointer>>(s_, p_, a_,
                                                                                       index_sequence_for<Args...>());
  }

  constexpr operator Pointer*() const noexcept { return __builtin_addressof(const_cast<Pointer&>(p_)); }
  operator void**() const noexcept
    requires(!is_same_v<Pointer, void*>)
  {
    static_assert(is_pointer_v<Pointer>, "std::out_ptr_t::operator void**: Pointer must be a pointer type");
    return reinterpret_cast<void**>(__builtin_addressof(const_cast<Pointer&>(p_)));
  }
};

// [out.ptr]
template <class Pointer = void, class Smart, class... Args>
constexpr auto out_ptr(Smart& s, Args&&... args) {
  using P = ycxx::detail::out_ptr_pointer<Pointer, Smart>;
  static_assert(!is_void_v<P>, "out_ptr/inout_ptr: POINTER_OF(Smart) is not valid; name the Pointer type");
  return out_ptr_t<Smart, P, Args&&...>(s, static_cast<Args&&>(args)...);
}

// [inout.ptr.t]
template <class Smart, class Pointer, class... Args>
class inout_ptr_t {
  static_assert(!ycxx::detail::is_shared_ptr<Smart>, "std::inout_ptr: shared_ptr cannot release ownership");

  Smart& s_;
  tuple<Args...> a_;
  Pointer p_;

  static constexpr Pointer initial(Smart& smart) {
    if constexpr (is_pointer_v<Smart>)
      return smart;
    else
      return smart.get();
  }

public:
  constexpr explicit inout_ptr_t(Smart& smart, Args... args)
      : s_(smart), a_(static_cast<Args&&>(args)...), p_(initial(smart)) {}
  inout_ptr_t(const inout_ptr_t&) = delete;

  // s.release() runs here (the release-statement of [inout.ptr.t]/10), not in the constructor.
  constexpr ~inout_ptr_t() {
    using SP = ycxx::detail::pointer_of_or<Smart, Pointer>;
    if constexpr (is_pointer_v<Smart>) {
      [&]<size_t... I>(index_sequence<I...>) {
        s_ = Smart(static_cast<SP>(p_), std::get<I>(static_cast<tuple<Args...>&&>(a_))...);
      }(index_sequence_for<Args...>());
    } else {
      (void)s_.release();
      if (p_)
        ycxx::detail::out_ptr_store<Smart, SP>(s_, p_, a_, index_sequence_for<Args...>());
    }
  }

  constexpr operator Pointer*() const noexcept { return __builtin_addressof(const_cast<Pointer&>(p_)); }
  operator void**() const noexcept
    requires(!is_same_v<Pointer, void*>)
  {
    static_assert(is_pointer_v<Pointer>, "std::inout_ptr_t::operator void**: Pointer must be a pointer type");
    return reinterpret_cast<void**>(__builtin_addressof(const_cast<Pointer&>(p_)));
  }
};

// [inout.ptr]
template <class Pointer = void, class Smart, class... Args>
constexpr auto inout_ptr(Smart& s, Args&&... args) {
  using P = ycxx::detail::out_ptr_pointer<Pointer, Smart>;
  static_assert(!is_void_v<P>, "out_ptr/inout_ptr: POINTER_OF(Smart) is not valid; name the Pointer type");
  return inout_ptr_t<Smart, P, Args&&...>(s, static_cast<Args&&>(args)...);
}

} // namespace std
