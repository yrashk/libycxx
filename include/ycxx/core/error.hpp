// libycxx core: the error hooks through which every library "throw" goes (DECISIONS §4).
//
// With exceptions enabled, raise_with(kind, what, make) throws make() from the header (exception
// classes defined inline in core headers; also works in constant evaluation), and
// raise(kind, what) throws a <stdexcept> class through the out-of-line ycxx::detail::throw_std in
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

// Weak default: a strong definition anywhere in the program replaces it. Hosted builds report
// the message through the PAL and abort; freestanding builds trap. (The PAL call sits in a
// discarded `if constexpr` branch when freestanding, so no PAL symbol is referenced.)
[[noreturn, gnu::weak, gnu::cold, gnu::noinline]] void ycxx_error_handler(ycxx_error_kind, const char* what) noexcept {
  if constexpr (ycxx::detail::cfg::hosted)
    ycxx_pal_abort(what);
  else
    __builtin_trap();
}

} // extern "C"

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

[[noreturn]] [[gnu::cold]] inline void assertion_failed(const char* msg) noexcept {
  ::ycxx_error_handler(ycxx_error_assertion, msg);
}

// Defined in the hosted runtime (built with exceptions); throws the <stdexcept> class of `kind`.
[[noreturn]] void throw_std(ycxx_error_kind kind, const char* what);

// The run-time hook for the <stdexcept> classes. `kind` selects the standard exception type.
[[noreturn]] [[gnu::cold]] inline void raise(ycxx_error_kind kind, const char* what) {
  if constexpr (cfg::exceptions)
    throw_std(kind, what);
  else
    ::ycxx_error_handler(kind, what);
}

// The same hook for exception types that cannot cross the C-linkage runtime boundary (class
// templates such as bad_expected_access<E>): `make()` builds the exception object, which is
// thrown here when exceptions are enabled; otherwise `kind` goes to the handler like raise() and
// make() is never called.
template <class Make>
[[noreturn]] [[gnu::cold]] constexpr void raise_with(ycxx_error_kind kind, const char* what, Make&& make) {
  if constexpr (cfg::exceptions)
    throw static_cast<Make&&>(make)();
  else
    ::ycxx_error_handler(kind, what);
}

// Library precondition check: active when YCXX_HARDENED=1. During constant evaluation a
// violated precondition is always a compile-time error.
[[gnu::always_inline]] constexpr void precondition(bool ok, const char* msg) noexcept {
  if consteval {
    if (!ok)
      assertion_failed(msg);
  } else {
    if constexpr (cfg::hardened) {
      if (!ok) [[unlikely]]
        assertion_failed(msg);
    }
  }
}

// The <stdexcept> classes are thrown from the header during constant evaluation (their
// constructors are constexpr there), and at run time through raise(), whose out-of-line
// throw_std keeps the many call sites small.
template <class E>
[[noreturn]] [[gnu::cold]] constexpr void raise_std(ycxx_error_kind kind, const char* what) {
  if consteval {
    ::ycxx::detail::raise_with(kind, what, [what] { return E(what); });
  } else {
    ::ycxx::detail::raise(kind, what);
  }
}
[[noreturn]] [[gnu::cold]] constexpr void throw_length_error(const char* w) {
  ::ycxx::detail::raise_std<std::length_error>(ycxx_error_length_error, w);
}
[[noreturn]] [[gnu::cold]] constexpr void throw_out_of_range(const char* w) {
  ::ycxx::detail::raise_std<std::out_of_range>(ycxx_error_out_of_range, w);
}
[[noreturn]] [[gnu::cold]] constexpr void throw_invalid_argument(const char* w) {
  ::ycxx::detail::raise_std<std::invalid_argument>(ycxx_error_invalid_argument, w);
}
[[noreturn]] [[gnu::cold]] constexpr void throw_overflow_error(const char* w) {
  ::ycxx::detail::raise_std<std::overflow_error>(ycxx_error_overflow_error, w);
}
[[noreturn]] [[gnu::cold]] constexpr void throw_range_error(const char* w) {
  ::ycxx::detail::raise_std<std::range_error>(ycxx_error_range_error, w);
}
[[noreturn]] [[gnu::cold]] constexpr void throw_runtime_error(const char* w) {
  ::ycxx::detail::raise_std<std::runtime_error>(ycxx_error_runtime_error, w);
}
// Other exception classes defined inline in core headers are thrown from the header through
// raise_with, which also works in constant evaluation (P3068). The throw_bad_* helpers for
// them live next to those classes.
[[noreturn]] [[gnu::cold]] constexpr void throw_bad_alloc() {
  ::ycxx::detail::raise_with(ycxx_error_bad_alloc, "std::bad_alloc", [] { return std::bad_alloc(); });
}
[[noreturn]] [[gnu::cold]] constexpr void throw_bad_array_new_length() {
  ::ycxx::detail::raise_with(ycxx_error_bad_array_new_length, "std::bad_array_new_length", [] { return std::bad_array_new_length(); });
}

}} // namespace ycxx::detail

