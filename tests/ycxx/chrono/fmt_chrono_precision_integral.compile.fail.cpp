// [time.format]/1: "Giving a precision specification in the chrono-format-spec is valid only
// for types that are specializations of std::chrono::duration for which the nested
// typedef-name rep denotes a floating-point type. For all other types, an exception of type
// format_error is thrown if the chrono-format-spec contains a precision specification." The
// parse member throws for seconds (an integral rep), so the compile-time checked format string
// is ill-formed ([format.fmt.string]/3). The control uses duration<double>.
#include <chrono>
#include <format>

int main() {
  (void)std::format("{:.3%S}", std::chrono::duration<double>(1.5));  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:.3%S}", std::chrono::seconds(1));
#endif
}
