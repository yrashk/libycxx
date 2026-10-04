// [format.string.std]/8: "The 0 option is valid for arithmetic types other than charT and bool,
// pointer types, or when an integer presentation type is specified." A string argument with 0 makes the format
// string invalid, and the call ill-formed ([format.fmt.string]/3). The control (0 with an int)
// is well-formed.
#include <format>
#include <string_view>

int main() {
  (void)std::format("{:05}", 12);  // control
  std::string_view s = "ab";
#ifndef YCXX_CONTROL
  (void)std::format("{:05}", s);
#endif
}
