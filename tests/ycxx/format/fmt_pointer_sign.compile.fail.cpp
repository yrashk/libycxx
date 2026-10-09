// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: the sign and \# options need an arithmetic presentation
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: the sign and \# options need an arithmetic presentation
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: the sign and \# options need an arithmetic presentation
// [format.string.std]/5: "The sign option is only valid for arithmetic types other than charT
// and bool or when an integer presentation type is specified"; pointer types (Table 111:
// none, p, P) are neither, so "{:+}" with nullptr is not a format string
// ([format.string.general]/5, [format.fmt.string]/3). The control uses 0, which /8 allows for
// pointer types.
#include <format>

int main() {
  (void)std::format("{:08}", nullptr);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:+}", nullptr);
#endif
}
