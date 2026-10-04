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
