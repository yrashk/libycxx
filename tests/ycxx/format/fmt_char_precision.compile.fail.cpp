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
