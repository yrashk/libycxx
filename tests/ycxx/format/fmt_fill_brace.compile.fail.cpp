// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid fill character
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: invalid fill character
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid fill character
// [format.string.std]/1: "fill: any character other than { or }". "{:{<5}" has no valid
// fill-and-align, and "{<5}" is not a width ("{ arg-id_opt }" needs a "}" after the
// optional arg-id), so the string is not a format string for the arguments
// ([format.string.general]/5) and the consteval constructor of format_string makes the call
// ill-formed ([format.fmt.string]/3). The control uses * as the fill.
#include <format>

int main() {
  (void)std::format("{:*<5}", 1);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:{<5}", 1);
#endif
}
