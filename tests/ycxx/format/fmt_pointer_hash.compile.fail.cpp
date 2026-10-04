// [format.string.std]/7: the # option "is valid for arithmetic types other than charT and
// bool or when an integer presentation type is specified, and not otherwise." A void*
// argument with # is not a format string ([format.string.general]/5, [format.fmt.string]/3).
// The control uses the P type.
#include <format>

int main() {
  (void)std::format("{:P}", static_cast<const void*>(nullptr));  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:#}", static_cast<const void*>(nullptr));
#endif
}
