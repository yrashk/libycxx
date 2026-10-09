// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::format: precision is valid only for floating\-point and string types
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: precision is valid only for floating\-point and string types
// [format.string.std]/15: "The precision option is valid for floating-point and string
// types." charT is neither ([format.string.std] Table 108), so "{:.2}" with a char argument
// is not a format string ([format.string.general]/5, [format.fmt.string]/3). The control
// gives a width.
#include <format>

int main() {
  (void)std::format("{:2}", 'c');  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:.2}", 'c');
#endif
}
