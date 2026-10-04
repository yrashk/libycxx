// [print.fun]/10: vprint_unicode(stream, fmt, args): "Let out denote the character
// representation of formatting arguments provided by args formatted according to
// specifications given in fmt", then "writes out to stream unchanged" (/10.2); /11: "Throws:
// Any exception thrown by the call to vformat". /16-17 likewise for vprint_nonunicode. The text
// written is the complete formatted result: when formatting fails (a format_error from
// vformat), no prefix of the output has been written. (Interpretive: the effects write `out`,
// which does not exist when vformat throws.) [ostream.formatted.print]/4 states the same for
// ostreams explicitly ("string out = vformat(...)").
#include <print>
#include <format>
#include <cstdio>
#include <sstream>
#include <string>
#include "fs_tmpdir.hpp"
#include "check.hpp"

int main() {
  TmpDir dir;
  const std::string p = dir / "f";
  FILE* f = std::fopen(p.c_str(), "w");
  CHECK(f != nullptr);
  int v = 1;
  int caught = 0;
  try {
    std::vprint_unicode(f, "ok {} {}", std::make_format_args(v));
  } catch (const std::format_error&) {
    ++caught;
  }
  try {
    std::vprint_nonunicode(f, "ok {} {:d}", std::make_format_args(v, "s"));
  } catch (const std::format_error&) {
    ++caught;
  }
  std::fclose(f);
  CHECK(caught == 2);
  CHECK(read_file(p).empty());

  std::ostringstream os;
  try {
    std::vprint_unicode(os, "ok {} {}", std::make_format_args(v));
  } catch (const std::format_error&) {
    ++caught;
  }
  CHECK(caught == 3 && os.str().empty());
  return 0;
}
