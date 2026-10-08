// libycxx core: the error hooks through which every library "throw" goes (DECISIONS §4).
//
// With exceptions enabled, raise_with(kind, what, make) throws make() from the header (exception
// classes defined inline in core headers; also works in constant evaluation), and
// raise(kind, what) throws a <stdexcept> class through the out-of-line __ycxx::__detail::__throw_std in
// the hosted runtime. The throw_length_error, throw_out_of_range, ... helpers use raise() at run
// time and throw the <stdexcept> class from the header during constant evaluation (raise_std).
//
// With -fno-exceptions the user-replaceable C function `ycxx_error_handler` is called. Its
// default (weak) definition calls __builtin_trap(). Provide a strong definition to override:
//
//   extern "C" [[noreturn]] void ycxx_error_handler(ycxx_error_kind kind, const char* what) noexcept { ... }
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/stdexcept.hpp>
#include <ycxx/pal.h>

extern "C" {

enum ycxx_error_kind : int {
  ycxx_error_assertion = 0, // hardened precondition violated (always via the handler)
  ycxx_error_logic_error,
  ycxx_error_domain_error,
  ycxx_error_invalid_argument,
  ycxx_error_length_error,
  ycxx_error_out_of_range,
  ycxx_error_runtime_error,
  ycxx_error_range_error,
  ycxx_error_overflow_error,
  ycxx_error_underflow_error,
  ycxx_error_bad_alloc,
  ycxx_error_bad_array_new_length,
  ycxx_error_bad_optional_access,
  ycxx_error_bad_variant_access,
  ycxx_error_bad_expected_access,
  ycxx_error_bad_function_call,
  ycxx_error_bad_any_cast,
  ycxx_error_bad_weak_ptr,
  ycxx_error_format_error,
  ycxx_error_system_error,
  ycxx_error_bad_cast,
  ycxx_error_future_error,
  ycxx_error_regex_error,
  ycxx_error_filesystem_error,
  ycxx_error_nonexistent_local_time,
  ycxx_error_ambiguous_local_time,
};

// Weak default: a strong definition anywhere in the program replaces it. Builds with the PAL's
// 'abort' layer (every hosted build, and a freestanding one whose integrator provides that layer,
// DECISIONS §18) report the message through the PAL and abort; other freestanding builds trap.
// (The PAL call sits in a discarded `if constexpr` branch otherwise, so no PAL symbol is
// referenced.)
[[noreturn, __gnu__::__weak__, __gnu__::__cold__, __gnu__::__noinline__, __gnu__::__visibility__("hidden")]] void ycxx_error_handler(ycxx_error_kind, const char* what) noexcept {
  if constexpr (__ycxx::__detail::__cfg::__layer::abort)
    ycxx_pal_abort(what);
  else
    __builtin_trap();
}

} // extern "C"

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

[[noreturn]] [[__gnu__::__cold__]] inline void __assertion_failed(const char* __msg) noexcept {
   ::ycxx_error_handler(ycxx_error_assertion, __msg);
}

// Defined in the hosted runtime (built with exceptions); throws the <stdexcept> class of `kind`.
[[noreturn]] void __throw_std(ycxx_error_kind kind, const char* what);

// The run-time hook for the <stdexcept> classes. `kind` selects the standard exception type.
[[noreturn]] [[__gnu__::__cold__]] inline void raise(ycxx_error_kind kind, const char* what) {
  if constexpr (__cfg::exceptions)
    __throw_std(kind, what);
  else
     ::ycxx_error_handler(kind, what);
}

// The same hook for exception types that cannot cross the C-linkage runtime boundary (class
// templates such as bad_expected_access<E>): `__make()` builds the exception object, which is
// thrown here when exceptions are enabled; otherwise `kind` goes to the handler like raise() and
// make() is never called.
template <class _Make>
[[noreturn]] [[__gnu__::__cold__]] constexpr void __raise_with(ycxx_error_kind kind, const char* what, _Make&& __make) {
  if constexpr (__cfg::exceptions)
    throw static_cast<_Make&&>(__make)();
  else
     ::ycxx_error_handler(kind, what);
}

// Library precondition check: active when YCXX_HARDENED=1. During constant evaluation a
// violated precondition is always a compile-time error.
[[__gnu__::__always_inline__]] constexpr void __precondition(bool ok, const char* __msg) noexcept {
  if consteval {
    if (!ok)
      __assertion_failed(__msg);
  } else {
    if constexpr (__cfg::__hardened) {
      if (!ok) [[unlikely]]
        __assertion_failed(__msg);
    }
  }
}

// The <stdexcept> classes are thrown from the header during constant evaluation (their
// constructors are constexpr there), and at run time through raise(), whose out-of-line
// throw_std keeps the many call sites small.
template <class _Ep>
[[noreturn]] [[__gnu__::__cold__]] constexpr void __raise_std(ycxx_error_kind kind, const char* what) {
  if consteval {
    ::__ycxx::__detail::__raise_with(kind, what, [what] { return _Ep(what); });
  } else {
    ::__ycxx::__detail::raise(kind, what);
  }
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_length_error(const char* __w) {
  ::__ycxx::__detail::__raise_std<std::length_error>(ycxx_error_length_error, __w);
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_out_of_range(const char* __w) {
  ::__ycxx::__detail::__raise_std<std::out_of_range>(ycxx_error_out_of_range, __w);
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_invalid_argument(const char* __w) {
  ::__ycxx::__detail::__raise_std<std::invalid_argument>(ycxx_error_invalid_argument, __w);
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_overflow_error(const char* __w) {
  ::__ycxx::__detail::__raise_std<std::overflow_error>(ycxx_error_overflow_error, __w);
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_range_error(const char* __w) {
  ::__ycxx::__detail::__raise_std<std::range_error>(ycxx_error_range_error, __w);
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_runtime_error(const char* __w) {
  ::__ycxx::__detail::__raise_std<std::runtime_error>(ycxx_error_runtime_error, __w);
}
// Other exception classes defined inline in core headers are thrown from the header through
// raise_with, which also works in constant evaluation (P3068). The throw_bad_* helpers for
// them live next to those classes.
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_bad_alloc() {
  ::__ycxx::__detail::__raise_with(ycxx_error_bad_alloc, "std::bad_alloc", [] { return std::bad_alloc(); });
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_bad_array_new_length() {
  ::__ycxx::__detail::__raise_with(ycxx_error_bad_array_new_length, "std::bad_array_new_length", [] { return std::bad_array_new_length(); });
}

}} // namespace __ycxx::__detail

