// libycxx core: exception propagation ([propagation]) and nested_exception ([except.nested]).
//
// exception_ptr holds a counted reference to a primary exception object of libycxx's ABI runtime
// (src/abi). The runtime keeps the object alive while any exception_ptr refers to it, and
// rethrow_exception throws a dependent exception that refers to the same object, so no copy is
// ever made ([propagation]/9, /11 allow either).
//
// Constant evaluation: [propagation]/8 makes the members constexpr, and make_exception_ptr,
// rethrow_exception and exception_ptr_cast are constexpr. GCC 16 keeps the exceptions of a
// constant evaluation itself and has builtins for them (YCXX_HAS_CONSTEXPR_EXCEPTION_PTR):
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

// Defined by the ABI runtime (src/abi/exception_ptr.cpp). `object` is a primary exception's
// thrown object.
namespace [[gnu::visibility("hidden")]] ycxx { namespace abi {
void exception_ptr_retain(void* object) noexcept;
void exception_ptr_release(void* object) noexcept;
// The currently handled exception's primary object with a new reference, or null.
void* current_exception_object() noexcept;
[[noreturn]] void rethrow_exception_object(void* object);
// The object a handler of type `const T&` (T given by its type_info) would bind to, or null.
const void* exception_object_as(void* object, const std::type_info& handler) noexcept;
// Storage for a primary exception object of `size` bytes whose type is `type` and which
// `destroy` destroys (null: trivially destructible), with one reference, owned by the caller
// (an exception_ptr). The caller constructs the object before the reference is released. Never
// returns null (an allocation that cannot be served terminates, [ABI-EH] 2.4.2).
void* exception_object_create(std::size_t size, const std::type_info* type, void (*destroy)(void*)) noexcept;
}} // namespace ycxx::abi

#if YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
// What GCC's evaluator implements for its own throw expressions; predeclared by GCC with this
// type once a throw expression is seen. Called during constant evaluation only.
extern "C" [[noreturn]] void __cxa_throw(void* thrown, void* tinfo, void (*destroy)(void*));
#endif

// The constant-evaluation half of exception_ptr (see above). The helpers are called inside
// `if consteval` only; without the builtins they are never reached with a non-null pointer.
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::cx_eh {
constexpr void adjust_ref([[maybe_unused]] void* object, [[maybe_unused]] int n) noexcept {
#if YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
  if consteval {
    __builtin_eh_ptr_adjust_ref(object, n);
  }
#endif
}
[[noreturn]] constexpr void rethrow([[maybe_unused]] void* object) {
#if YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
  if consteval {
    __cxa_throw(object, nullptr, nullptr);
  }
#endif
  __builtin_unreachable();
}
}} // namespace ycxx::detail::cx_eh

namespace [[gnu::visibility("hidden")]] std {

class exception_ptr;
exception_ptr current_exception() noexcept;
[[noreturn]] constexpr void rethrow_exception(exception_ptr p);

class exception_ptr {
  void* p_ = nullptr;

  struct adopt_t {};
  constexpr exception_ptr(adopt_t, void* p) noexcept : p_(p) {}

  friend exception_ptr current_exception() noexcept;
  // current-exception ([exception.syn]): current_exception, also during constant evaluation.
  static constexpr exception_ptr current() noexcept;
  friend constexpr void rethrow_exception(exception_ptr);
  template <class E>
  friend constexpr optional<const E&> exception_ptr_cast(const exception_ptr&) noexcept;
  template <class E>
  friend constexpr exception_ptr make_exception_ptr(E) noexcept;

public:
  constexpr exception_ptr() noexcept = default;
  constexpr exception_ptr(nullptr_t) noexcept {}
  constexpr exception_ptr(const exception_ptr& o) noexcept : p_(o.p_) {
    if (p_) {
      if consteval {
        ::ycxx::detail::cx_eh::adjust_ref(p_, 1);
      } else {
        ::ycxx::abi::exception_ptr_retain(p_);
      }
    }
  }
  constexpr exception_ptr(exception_ptr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
  constexpr exception_ptr& operator=(const exception_ptr& o) noexcept {
    exception_ptr(o).swap(*this);
    return *this;
  }
  constexpr exception_ptr& operator=(exception_ptr&& o) noexcept {
    exception_ptr(static_cast<exception_ptr&&>(o)).swap(*this);
    return *this;
  }
  constexpr ~exception_ptr() {
    if (p_) {
      if consteval {
        ::ycxx::detail::cx_eh::adjust_ref(p_, -1);
      } else {
        ::ycxx::abi::exception_ptr_release(p_);
      }
    }
  }

  constexpr void swap(exception_ptr& o) noexcept {
    void* t = p_;
    p_ = o.p_;
    o.p_ = t;
  }
  constexpr explicit operator bool() const noexcept { return p_ != nullptr; }

  friend constexpr bool operator==(const exception_ptr& a, const exception_ptr& b) noexcept { return a.p_ == b.p_; }
  friend constexpr bool operator==(const exception_ptr& a, nullptr_t) noexcept { return a.p_ == nullptr; }
  friend constexpr void swap(exception_ptr& a, exception_ptr& b) noexcept { a.swap(b); }
};

inline exception_ptr current_exception() noexcept {
  return exception_ptr(exception_ptr::adopt_t{}, ::ycxx::abi::current_exception_object());
}

constexpr exception_ptr exception_ptr::current() noexcept {
  if consteval {
#if YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
    return __builtin_current_exception();
#else
    return exception_ptr();
#endif
  } else {
    return current_exception();
  }
}

[[noreturn]] constexpr void rethrow_exception(exception_ptr p) {
  ::ycxx::detail::precondition(p.p_ != nullptr, "std::rethrow_exception: null exception_ptr");
  if consteval {
    ::ycxx::detail::cx_eh::rethrow(p.p_);
  } else {
    ::ycxx::abi::rethrow_exception_object(p.p_);
  }
}

template <class E>
constexpr exception_ptr make_exception_ptr(E e) noexcept {
  if constexpr (::ycxx::detail::cfg::exceptions) {
    try {
      throw e;
    } catch (...) {
      return exception_ptr::current();
    }
  } else if constexpr (::ycxx::detail::cfg::rtti) {
    // Without exceptions there is no throw to copy e, so the runtime's object is made directly:
    // the same primary exception a `throw e` would create, which exception_ptr_cast observes
    // and rethrow_exception throws (in code built with exceptions). E's copy constructor cannot
    // throw here.
    void (*destroy)(void*) = nullptr;
    if constexpr (!is_trivially_destructible_v<E>)
      destroy = [](void* p) noexcept { static_cast<E*>(p)->~E(); };
    void* obj = ::ycxx::abi::exception_object_create(sizeof(E), ::ycxx::detail::type_id<E>, destroy);
    ::new (obj) E(e);
    return exception_ptr(exception_ptr::adopt_t{}, obj);
  } else {
    // Without exceptions and without RTTI the exception object's type cannot be recorded (its
    // type_info cannot be named), and nothing in such a program could match it: null.
    return exception_ptr();
  }
}

template <class E>
constexpr optional<const E&> exception_ptr_cast(const exception_ptr& p) noexcept {
  static_assert(is_object_v<E> && !is_const_v<E> && !is_volatile_v<E>,
                "std::exception_ptr_cast: Mandates: E is a cv-unqualified complete object type");
  static_assert(!is_array_v<E>, "std::exception_ptr_cast: Mandates: E is not an array type");
  static_assert(!is_pointer_v<E> && !is_member_pointer_v<E>,
                "std::exception_ptr_cast: Mandates: E is not a pointer or pointer-to-member type");
  static_assert(sizeof(E) > 0, "std::exception_ptr_cast: Mandates: E is complete");
  if (!p.p_)
    return nullopt;
  if consteval {
    // A handler decides; the object stays alive while p refers to it, so the reference the
    // handler binds outlives the handler.
    if constexpr (::ycxx::detail::cfg::constexpr_exception_ptr) {
      try {
        ::ycxx::detail::cx_eh::rethrow(p.p_);
      } catch (const E& e) {
        return optional<const E&>(e);
      } catch (...) {
      }
    }
    return nullopt;
  }
  if constexpr (::ycxx::detail::cfg::rtti) {
    const void* obj = ::ycxx::abi::exception_object_as(p.p_, *::ycxx::detail::type_id<E>);
    if (!obj)
      return nullopt;
    return optional<const E&>(*static_cast<const E*>(obj));
  } else if constexpr (::ycxx::detail::cfg::exceptions) {
    // Without RTTI, let a handler decide; the rethrown exception refers to the same object,
    // which p keeps alive.
    try {
      rethrow_exception(p);
    } catch (const E& e) {
      return optional<const E&>(e);
    } catch (...) {
    }
    return nullopt;
  } else {
    return nullopt;
  }
}
template <class E>
void exception_ptr_cast(const exception_ptr&&) = delete;

// ---- [except.nested] ----
class nested_exception {
  exception_ptr nested_;

public:
  nested_exception() noexcept : nested_(current_exception()) {}
  nested_exception(const nested_exception&) noexcept = default;
  nested_exception& operator=(const nested_exception&) noexcept = default;
  virtual ~nested_exception() = default;

  [[noreturn]] void rethrow_nested() const {
    if (!nested_)
      terminate();
    rethrow_exception(nested_);
  }
  exception_ptr nested_ptr() const noexcept { return nested_; }
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// The exception type throw_with_nested throws for a class U.
template <class U>
struct nested_wrapper : U, std::nested_exception {
  template <class T>
  explicit nested_wrapper(T&& t) : U(static_cast<T&&>(t)) {}
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

template <class T>
[[noreturn]] void throw_with_nested(T&& t) {
  using U = decay_t<T>;
  if constexpr (is_class_v<U> && !is_final_v<U> && !is_base_of_v<nested_exception, U>)
    throw ::ycxx::adl_free::nested_wrapper<U>(static_cast<T&&>(t));
  else
    throw static_cast<T&&>(t);
}

template <class E>
void rethrow_if_nested(const E& e) {
  if constexpr (is_polymorphic_v<E>) {
    if constexpr (is_base_of_v<nested_exception, E>) {
      // An accessible, unambiguous base: the conversion is static. (Otherwise: no effect.)
      if constexpr (is_convertible_v<const E*, const nested_exception*>)
        static_cast<const nested_exception*>(__builtin_addressof(e))->rethrow_nested();
    } else if constexpr (::ycxx::detail::cfg::rtti) {
      // (A dynamic_cast in a discarded branch of a template is not diagnosed under -fno-rtti.)
      if (auto p = dynamic_cast<const nested_exception*>(__builtin_addressof(e)))
        p->rethrow_nested();
    }
  }
}

} // namespace std
