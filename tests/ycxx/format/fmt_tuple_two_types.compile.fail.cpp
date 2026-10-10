// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::formatter: invalid tuple\-format\-spec
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::formatter: invalid tuple\-format\-spec
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::formatter: invalid tuple\-format\-spec
// [format.tuple]/2: "tuple-format-spec: tuple-fill-and-align_opt width_opt tuple-type_opt"
// with a single tuple-type, m or n; "{:nm}" (valid for a range of pairs,
// [format.range.formatter]/2) is not a format string for a pair ([format.string.general]/5,
// [format.fmt.string]/3). The control uses m alone.
#include <format>
#include <utility>
int main() {
  (void)std::format("{:m}", std::pair(1, 2));  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:nm}", std::pair(1, 2));
#endif
}
