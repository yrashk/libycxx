// libycxx core: the single error hook through which every library "throw" goes.
//
// With exceptions enabled, ycxx::detail::throw_xxx(...) throws the standard exception type.
// The exception classes are defined by the hosted runtime (src/hosted/exceptions.cpp); core
// headers only declare the out-of-line throwing functions, so they never depend on
// <exception> or <stdexcept>.
//
// With -fno-exceptions the user-replaceable C function `ycxx_error_handler` is called. Its
// default (weak) definition calls __builtin_trap(). Provide a strong definition to override:
//
//   extern "C" [[noreturn]] void ycxx_error_handler(ycxx_error_kind kind, const char* what) noexcept { ... }
#ifndef YCXX_CORE_ERROR_HPP
#define YCXX_CORE_ERROR_HPP

#include <ycxx/config.hpp>

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

#if YCXX_COMPILER_GCC
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#endif
// Weak default: a strong definition anywhere in the program replaces it.
[[noreturn, gnu::weak, gnu::cold, gnu::noinline]] void ycxx_error_handler(ycxx_error_kind, const char*) noexcept {
  __builtin_trap();
}
#if YCXX_COMPILER_GCC
#  pragma GCC diagnostic pop
#endif

} // extern "C"

namespace ycxx::detail {

[[noreturn]] YCXX_COLD inline void assertion_failed(const char* msg) noexcept {
  ::ycxx_error_handler(ycxx_error_assertion, msg);
}

#if YCXX_HAS_EXCEPTIONS
// Defined in the hosted runtime; each throws the corresponding std:: exception.
[[noreturn]] void throw_std(ycxx_error_kind kind, const char* what);
#endif

// The one hook. `kind` selects the standard exception type.
[[noreturn]] YCXX_COLD inline void raise(ycxx_error_kind kind, const char* what) {
#if YCXX_HAS_EXCEPTIONS
  throw_std(kind, what);
#else
  ::ycxx_error_handler(kind, what);
#endif
}

[[noreturn]] YCXX_COLD inline void throw_length_error(const char* w) { raise(ycxx_error_length_error, w); }
[[noreturn]] YCXX_COLD inline void throw_out_of_range(const char* w) { raise(ycxx_error_out_of_range, w); }
[[noreturn]] YCXX_COLD inline void throw_invalid_argument(const char* w) { raise(ycxx_error_invalid_argument, w); }
[[noreturn]] YCXX_COLD inline void throw_overflow_error(const char* w) { raise(ycxx_error_overflow_error, w); }
[[noreturn]] YCXX_COLD inline void throw_range_error(const char* w) { raise(ycxx_error_range_error, w); }
[[noreturn]] YCXX_COLD inline void throw_runtime_error(const char* w) { raise(ycxx_error_runtime_error, w); }
[[noreturn]] YCXX_COLD inline void throw_bad_alloc() { raise(ycxx_error_bad_alloc, "std::bad_alloc"); }
[[noreturn]] YCXX_COLD inline void throw_bad_array_new_length() {
  raise(ycxx_error_bad_array_new_length, "std::bad_array_new_length");
}
[[noreturn]] YCXX_COLD inline void throw_bad_optional_access() {
  raise(ycxx_error_bad_optional_access, "std::bad_optional_access");
}
[[noreturn]] YCXX_COLD inline void throw_bad_variant_access() {
  raise(ycxx_error_bad_variant_access, "std::bad_variant_access");
}
[[noreturn]] YCXX_COLD inline void throw_bad_function_call() {
  raise(ycxx_error_bad_function_call, "std::bad_function_call");
}
[[noreturn]] YCXX_COLD inline void throw_bad_any_cast() { raise(ycxx_error_bad_any_cast, "std::bad_any_cast"); }
[[noreturn]] YCXX_COLD inline void throw_bad_weak_ptr() { raise(ycxx_error_bad_weak_ptr, "std::bad_weak_ptr"); }

} // namespace ycxx::detail

#endif // YCXX_CORE_ERROR_HPP
