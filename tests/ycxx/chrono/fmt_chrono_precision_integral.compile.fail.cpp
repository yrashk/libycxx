// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: a precision is valid only for durations with a floating\-point representation
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: a precision is valid only for durations with a floating\-point representation
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: a precision is valid only for durations with a floating\-point representation
// [time.format]/1: "Giving a precision specification in the chrono-format-spec is valid only
// for types that are specializations of std::chrono::duration for which the nested
// typedef-name rep denotes a floating-point type. For all other types, an exception of type
// format_error is thrown if the chrono-format-spec contains a precision specification." The
// parse member throws for seconds (an integral rep), so the compile-time checked format string
// is ill-formed ([format.fmt.string]/3). The control uses duration<double>.
#include <chrono>
#include <format>

int main() {
  (void)std::format("{:.3%S}", std::chrono::duration<double>(1.5));  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:.3%S}", std::chrono::seconds(1));
#endif
}
