// [print.fun]/8 and /14: vprint_unicode_buffered(stream, fmt, args) and
// vprint_nonunicode_buffered are "Equivalent to: string out = vformat(fmt, args);
// vprint_...(stream, "{}", make_format_args(out));", so when vformat throws format_error
// nothing has been written. The unbuffered FILE* overloads (/10, /16: "While holding the lock on
// stream, writes the character representation of formatting arguments ...") deliberately allow
// formatting directly into the stream (P3107R5: "with the direct method, the output
// written to the stream before the exception occurred is preserved"), so for them only the
// exception and an output that is a prefix of the intended text are checked.
// [ostream.formatted.print]/4: vprint_unicode(ostream&, ...) initializes
// "string out = vformat(os.getloc(), fmt, args);" before writing, so nothing is written there.
// REQUIRES: exceptions
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
    std::vprint_unicode_buffered(f, "ok {} {}", std::make_format_args(v));
  } catch (const std::format_error&) {
    ++caught;
  }
  try {
    std::vprint_nonunicode_buffered(f, "ok {} {:d}", std::make_format_args(v, "s"));
  } catch (const std::format_error&) {
    ++caught;
  }
  std::fclose(f);
  CHECK(caught == 2);
  CHECK(read_file(p).empty());

  // unbuffered: the exception propagates; partial output is permitted
  f = std::fopen(p.c_str(), "w");
  CHECK(f != nullptr);
  caught = 0;
  try {
    std::vprint_unicode(f, "ok {} {}", std::make_format_args(v));
  } catch (const std::format_error&) {
    ++caught;
  }
  std::fclose(f);
  CHECK(caught == 1);
  const std::string partial = read_file(p);
  CHECK(std::string("ok 1 ").starts_with(partial));

  std::ostringstream os;
  try {
    std::vprint_unicode(os, "ok {} {}", std::make_format_args(v));
  } catch (const std::format_error&) {
    ++caught;
  }
  CHECK(caught == 2 && os.str().empty());
  return 0;
}
