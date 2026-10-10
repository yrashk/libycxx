// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::formatter: invalid tuple\-format\-spec
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::formatter: invalid tuple\-format\-spec
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::formatter: invalid tuple\-format\-spec
// [format.tuple]/2: "tuple-fill: any character other than { or } or :", and a
// tuple-format-spec has no underlying spec, so "{::>8}" is not a format string for a pair
// ([format.string.general]/5, [format.fmt.string]/3), although it is for an int (where : is
// a valid fill) and for a range (an underlying spec, [format.range.formatter]/2). The control
// uses * as the tuple-fill.
#include <format>
#include <utility>
int main() {
  (void)std::format("{:*>8}{::>8}", std::pair(1, 2), 3);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{::>8}", std::pair(1, 2));
#endif
}
