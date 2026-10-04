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
