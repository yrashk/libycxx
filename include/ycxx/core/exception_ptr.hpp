// libycxx core: exception propagation ([propagation]) and nested_exception ([except.nested]).
//
// exception_ptr holds a counted reference to a primary exception object of libycxx's ABI runtime
// (src/abi). The runtime keeps the object alive while any exception_ptr refers to it, and
// rethrow_exception throws a dependent exception that refers to the same object, so no copy is
// ever made ([propagation]/9, /11 allow either).
//
// Constant evaluation: [propagation]/8 makes the members constexpr, and make_exception_ptr,
// rethrow_exception and exception_ptr_cast are constexpr. GCC 16 keeps the exceptions of a
// constant evaluation itself and has builtins for them (_YCXX_HAS_CONSTEXPR_EXCEPTION_PTR):
// __builtin_current_exception() makes an exception_ptr (p_) for the exception being handled, and
// __builtin_eh_ptr_adjust_ref counts the references; rethrowing is a throw of the same object
// through __cxa_throw, which GCC's evaluator implements. Elsewhere (Clang 23 cannot throw during
// constant evaluation) only null exception_ptrs exist there (STATUS: known limitations).
#pragma once

#include <ycxx/core/exception.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/typeinfo.hpp>

// Defined by the ABI runtime (src/abi/exception_ptr.cpp). `__object` is a primary exception's
// thrown object.
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __abi {
void __exception_ptr_retain(void* __object) noexcept;
void __exception_ptr_release(void* __object) noexcept;
// The currently handled exception's primary object with a new reference, or null.
void* __current_exception_object() noexcept;
[[noreturn]] void __rethrow_exception_object(void* __object);
// The object a handler of type `const _Tp&` (T given by its type_info) would bind to, or null.
const void* __exception_object_as(void* __object, const std::type_info& __handler) noexcept;
// Storage for a primary exception object of `size` bytes whose type is `type` and which
// `destroy` destroys (null: trivially destructible), with one reference, owned by the caller
// (an exception_ptr). The caller constructs the object before the reference is released. Never
// returns null (an allocation that cannot be served terminates, [ABI-EH] 2.4.2).
void* __exception_object_create(std::size_t size, const std::type_info* type, void (*destroy)(void*)) noexcept;
}} // namespace __ycxx::__abi

#if _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
// What GCC's evaluator implements for its own throw expressions; predeclared by GCC with this
// type once a throw expression is seen. Called during constant evaluation only.
extern "C" [[noreturn]] void __cxa_throw(void* __thrown, void* __tinfo, void (*destroy)(void*));
#endif

// The constant-evaluation half of exception_ptr (see above). The helpers are called inside
// `if consteval` only; without the builtins they are never reached with a non-null pointer.
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__cx_eh {
constexpr void __adjust_ref([[maybe_unused]] void* __object, [[maybe_unused]] int n) noexcept {
#if _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
  if consteval {
    __builtin_eh_ptr_adjust_ref(__object, n);
  }
#endif
}
[[noreturn]] constexpr void __rethrow([[maybe_unused]] void* __object) {
#if _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
  if consteval {
    __cxa_throw(__object, nullptr, nullptr);
  }
#endif
  __builtin_unreachable();
}
}} // namespace __ycxx::__detail::__cx_eh

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class exception_ptr;
exception_ptr current_exception() noexcept;
[[noreturn]] constexpr void rethrow_exception(exception_ptr p);

class exception_ptr {
  void* __p_ = nullptr;

  struct __adopt_t {};
  constexpr exception_ptr(__adopt_t, void* p) noexcept : __p_(p) {}

  friend exception_ptr current_exception() noexcept;
  // current-exception ([exception.syn]): current_exception, also during constant evaluation.
  static constexpr exception_ptr current() noexcept;
  friend constexpr void rethrow_exception(exception_ptr);
  template <class _Ep>
  friend constexpr optional<const _Ep&> exception_ptr_cast(const exception_ptr&) noexcept;
  template <class _Ep>
  friend constexpr exception_ptr make_exception_ptr(_Ep) noexcept;

public:
  constexpr exception_ptr() noexcept = default;
  constexpr exception_ptr(nullptr_t) noexcept {}
  constexpr exception_ptr(const exception_ptr& __o) noexcept : __p_(__o.__p_) {
    if (__p_) {
      if consteval {
        ::__ycxx::__detail::__cx_eh::__adjust_ref(__p_, 1);
      } else {
        ::__ycxx::__abi::__exception_ptr_retain(__p_);
      }
    }
  }
  constexpr exception_ptr(exception_ptr&& __o) noexcept : __p_(__o.__p_) { __o.__p_ = nullptr; }
  constexpr exception_ptr& operator=(const exception_ptr& __o) noexcept {
    exception_ptr(__o).swap(*this);
    return *this;
  }
  constexpr exception_ptr& operator=(exception_ptr&& __o) noexcept {
    exception_ptr(static_cast<exception_ptr&&>(__o)).swap(*this);
    return *this;
  }
  constexpr ~exception_ptr() {
    if (__p_) {
      if consteval {
        ::__ycxx::__detail::__cx_eh::__adjust_ref(__p_, -1);
      } else {
        ::__ycxx::__abi::__exception_ptr_release(__p_);
      }
    }
  }

  constexpr void swap(exception_ptr& __o) noexcept {
    void* t = __p_;
    __p_ = __o.__p_;
    __o.__p_ = t;
  }
  constexpr explicit operator bool() const noexcept { return __p_ != nullptr; }

  friend constexpr bool operator==(const exception_ptr& a, const exception_ptr& b) noexcept { return a.__p_ == b.__p_; }
  friend constexpr bool operator==(const exception_ptr& a, nullptr_t) noexcept { return a.__p_ == nullptr; }
  friend constexpr void swap(exception_ptr& a, exception_ptr& b) noexcept { a.swap(b); }
};

inline exception_ptr current_exception() noexcept {
  return exception_ptr(exception_ptr::__adopt_t{}, ::__ycxx::__abi::__current_exception_object());
}

constexpr exception_ptr exception_ptr::current() noexcept {
  if consteval {
#if _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
    return __builtin_current_exception();
#else
    return exception_ptr();
#endif
  } else {
    return current_exception();
  }
}

[[noreturn]] constexpr void rethrow_exception(exception_ptr p) {
  ::__ycxx::__detail::__precondition(p.__p_ != nullptr, "std::rethrow_exception: null exception_ptr");
  if consteval {
    ::__ycxx::__detail::__cx_eh::__rethrow(p.__p_);
  } else {
    ::__ycxx::__abi::__rethrow_exception_object(p.__p_);
  }
}

template <class _Ep>
constexpr exception_ptr make_exception_ptr(_Ep e) noexcept {
  if constexpr (::__ycxx::__detail::__cfg::exceptions) {
    try {
      throw e;
    } catch (...) {
      return exception_ptr::current();
    }
  } else if constexpr (::__ycxx::__detail::__cfg::__rtti) {
    // Without exceptions there is no throw to copy e, so the runtime's object is made directly:
    // the same primary exception a `throw e` would create, which exception_ptr_cast observes
    // and rethrow_exception throws (in code built with exceptions). E's copy constructor cannot
    // throw here.
    void (*destroy)(void*) = nullptr;
    if constexpr (!is_trivially_destructible_v<_Ep>)
      destroy = [](void* p) noexcept { static_cast<_Ep*>(p)->~_Ep(); };
    void* __obj = ::__ycxx::__abi::__exception_object_create(sizeof(_Ep), ::__ycxx::__detail::__type_id<_Ep>, destroy);
    ::new (__obj) _Ep(e);
    return exception_ptr(exception_ptr::__adopt_t{}, __obj);
  } else {
    // Without exceptions and without RTTI the exception object's type cannot be recorded (its
    // type_info cannot be named), and nothing in such a program could match it: null.
    return exception_ptr();
  }
}

template <class _Ep>
constexpr optional<const _Ep&> exception_ptr_cast(const exception_ptr& p) noexcept {
  static_assert(is_object_v<_Ep> && !is_const_v<_Ep> && !is_volatile_v<_Ep>,
                "std::exception_ptr_cast: Mandates: E is a cv-unqualified complete object type");
  static_assert(!is_array_v<_Ep>, "std::exception_ptr_cast: Mandates: E is not an array type");
  static_assert(!is_pointer_v<_Ep> && !is_member_pointer_v<_Ep>,
                "std::exception_ptr_cast: Mandates: E is not a pointer or pointer-to-member type");
  static_assert(sizeof(_Ep) > 0, "std::exception_ptr_cast: Mandates: E is complete");
  if (!p.__p_)
    return nullopt;
  if consteval {
    // A handler decides; the object stays alive while p refers to it, so the reference the
    // handler binds outlives the handler.
    if constexpr (::__ycxx::__detail::__cfg::__constexpr_exception_ptr) {
      try {
        ::__ycxx::__detail::__cx_eh::__rethrow(p.__p_);
      } catch (const _Ep& e) {
        return optional<const _Ep&>(e);
      } catch (...) {
      }
    }
    return nullopt;
  }
  if constexpr (::__ycxx::__detail::__cfg::__rtti) {
    const void* __obj = ::__ycxx::__abi::__exception_object_as(p.__p_, *::__ycxx::__detail::__type_id<_Ep>);
    if (!__obj)
      return nullopt;
    return optional<const _Ep&>(*static_cast<const _Ep*>(__obj));
  } else if constexpr (::__ycxx::__detail::__cfg::exceptions) {
    // Without RTTI, let a handler decide; the rethrown exception refers to the same object,
    // which p keeps alive.
    try {
      rethrow_exception(p);
    } catch (const _Ep& e) {
      return optional<const _Ep&>(e);
    } catch (...) {
    }
    return nullopt;
  } else {
    return nullopt;
  }
}
template <class _Ep>
void exception_ptr_cast(const exception_ptr&&) = delete;

// ---- [except.nested] ----
class nested_exception {
  exception_ptr __nested_;

public:
  nested_exception() noexcept : __nested_(current_exception()) {}
  nested_exception(const nested_exception&) noexcept = default;
  nested_exception& operator=(const nested_exception&) noexcept = default;
  virtual ~nested_exception() = default;

  [[noreturn]] void rethrow_nested() const {
    if (!__nested_)
      terminate();
    rethrow_exception(__nested_);
  }
  exception_ptr nested_ptr() const noexcept { return __nested_; }
};

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
// The exception type throw_with_nested throws for a class U.
template <class _Up>
struct __nested_wrapper : _Up, std::nested_exception {
  template <class _Tp>
  explicit __nested_wrapper(_Tp&& t) : _Up(static_cast<_Tp&&>(t)) {}
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
[[noreturn]] void throw_with_nested(_Tp&& t) {
  using _Up = decay_t<_Tp>;
  if constexpr (is_class_v<_Up> && !is_final_v<_Up> && !is_base_of_v<nested_exception, _Up>)
    throw ::__ycxx::__adl_free::__nested_wrapper<_Up>(static_cast<_Tp&&>(t));
  else
    throw static_cast<_Tp&&>(t);
}

template <class _Ep>
void rethrow_if_nested(const _Ep& e) {
  if constexpr (is_polymorphic_v<_Ep>) {
    if constexpr (is_base_of_v<nested_exception, _Ep>) {
      // An accessible, unambiguous base: the conversion is static. (Otherwise: no effect.)
      if constexpr (is_convertible_v<const _Ep*, const nested_exception*>)
        static_cast<const nested_exception*>(__builtin_addressof(e))->rethrow_nested();
    } else if constexpr (::__ycxx::__detail::__cfg::__rtti) {
      // (A dynamic_cast in a discarded branch of a template is not diagnosed under -fno-rtti.)
      if (auto p = dynamic_cast<const nested_exception*>(__builtin_addressof(e)))
        p->rethrow_nested();
    }
  }
}

}} // namespace std
