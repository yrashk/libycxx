// [format.string.std]/8: "The 0 option is valid for arithmetic types other than charT and bool,
// pointer types, or when an integer presentation type is specified." bool with the default (string)
// presentation and 0 is invalid; the control uses an integer presentation type ("{:05d}"),
// which is valid.
#include <format>

int main() {
  (void)std::format("{:05d}", true);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:05}", true);
#endif
}
