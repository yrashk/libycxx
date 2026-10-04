// [format.arg.store]/1, [format.syn]: `make_format_args(Args&... fmt_args)` (P2905R2) takes
// lvalue references only, so an rvalue argument cannot bind (the stored basic_format_args
// would otherwise refer to a temporary that is gone at the end of the full-expression).
// The control passes an lvalue; format(fmt, args...) itself still accepts rvalues
// ([format.functions]/2: Args&&...).
#include <format>
#include <string>

int main() {
  int i = 42;
  std::string s = std::vformat("{}", std::make_format_args(i));  // control
  s += std::format("{}", 42);                                     // control
#ifndef YCXX_CONTROL
  s += std::vformat("{}", std::make_format_args(42));
#endif
  return static_cast<int>(s.size());
}
