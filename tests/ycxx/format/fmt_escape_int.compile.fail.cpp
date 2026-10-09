// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid presentation type for the argument
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid presentation type for the argument
// [format.string.std] Table 107 (integral presentation types: b, B, c, d, o, x, X, none) has no
// '?'; '?' is only a string / character presentation type (Tables 106, 108). An invalid type
// for the argument makes the format string invalid and the call ill-formed
// ([format.fmt.string]/3). The control ('?' with a char) is well-formed.
#include <format>

int main() {
  (void)std::format("{:?}", 'c');  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:?}", 42);
#endif
}
