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
