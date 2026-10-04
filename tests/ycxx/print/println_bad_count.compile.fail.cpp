// [format.fmt.string]/2: the format string is checked at compile time: a replacement field
// referring to a missing argument makes the call ill-formed.
#include <print>
#include <cstdio>

void f() {
  std::println(stdout, "{} {}", 1);
}
