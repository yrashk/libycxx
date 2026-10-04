// [time.format]/1: the parse member functions of the chrono formatters interpret the format
// specification as a chrono-format-spec, whose conversion-spec is "% modifier_opt type" with
// type one of a A b B c C d D e F g G h H I j m M n p q Q r R S t T u U V w W x X y Y z Z %;
// "%K" matches no production, so the format string is invalid for the argument and the call
// with a compile-time checked format string is ill-formed ([format.fmt.string]/3).
// The control ("%H") is well-formed.
#include <chrono>
#include <format>

int main() {
  using namespace std::chrono_literals;
  (void)std::format("{:%H}", 5s);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:%K}", 5s);
#endif
}
