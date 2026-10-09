// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid format specification
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid format specification
// [format.string.std]/3: "For a format specification in UTF-8 ... the fill character
// corresponds to a single Unicode scalar value." e followed by U+0301 is two scalar values,
// so "{:e\u0301<5}" has no fill-and-align and is not a format string for a string argument
// ([format.string.general]/5, [format.fmt.string]/3). The control uses U+00E9, a single
// scalar value with a two-code-unit UTF-8 encoding.
#include <format>

int main() {
  (void)std::format("{:\u00e9<5}", "x");  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:e\u0301<5}", "x");
#endif
}
