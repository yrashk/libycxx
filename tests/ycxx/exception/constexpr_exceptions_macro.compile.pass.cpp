// [version.syn]: __cpp_lib_constexpr_exceptions is 202502L and is defined by <version>,
// <exception>, <stdexcept>, <expected>, <optional>, <variant> and <format> (P3068, P3378).
// [support.limits.general]/3: a macro is defined only when the implementation supports the
// feature; it covers throwing during constant evaluation, which Clang 23 cannot do.
// XFAIL: clang Clang 23 cannot throw during constant evaluation, so the feature and its macro are absent
// REQUIRES: exceptions
#include <exception>

#if !defined(__cpp_lib_constexpr_exceptions) || __cpp_lib_constexpr_exceptions != 202502L
#  error "<exception>: __cpp_lib_constexpr_exceptions != 202502L"
#endif

#include <stdexcept>
#include <expected>
#include <optional>
#include <variant>
#include <format>
#include <version>

#if !defined(__cpp_lib_constexpr_exceptions) || __cpp_lib_constexpr_exceptions != 202502L
#  error "__cpp_lib_constexpr_exceptions != 202502L"
#endif

int main() { return 0; }
