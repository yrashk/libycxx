// [string.cons]: basic_string(const charT* s) copies [s, s + traits::length(s)). libycxx copies a
// short string with fixed-size, possibly overlapping word copies guarded by the length. GCC, with
// -fsanitize=address weakening its range analysis, inlined that copy with s one of two short
// literals and the length unknown, and warned (-Warray-bounds) about the word copies that the
// guards keep from running: a false positive that -Werror turned into a build failure (of
// libycxx's own instrumented build, in filesystem::path::lexically_normal). This program is that
// pattern, built the same way.
// REQUIRES: gcc
// UNSUPPORTED-SANITIZER: tsan  -fsanitize=address cannot be combined with ThreadSanitizer
// FLAGS: -O2 -fsanitize=address -Werror=array-bounds
#include <string>
#include "check.hpp"

[[__gnu__::__noinline__]] std::string pick(bool root) {
  std::string r = root ? "/" : "";
  return r;
}

int main() {
  volatile bool root = true;
  CHECK(pick(root) == "/");
  root = false;
  CHECK(pick(root).empty());
}
