// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid chrono conversion specifier
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: invalid chrono conversion specifier
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid chrono conversion specifier
// [time.format]/1: the parse member functions of the chrono formatters interpret the format
// specification as a chrono-format-spec, whose conversion-spec is "% modifier_opt type" with
// type one of a A b B c C d D e F g G h H I j m M n p q Q r R S t T u U V w W x X y Y z Z %;
// "%K" matches no production, so the format string is invalid for the argument and the call
// with a compile-time checked format string is ill-formed ([format.fmt.string]/3).
// The control ("%H") is well-formed.
#include <chrono>
#include <format>

int main() {
  using namespace std::chrono_literals;
  (void)std::format("{:%H}", 5s);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:%K}", 5s);
#endif
}
