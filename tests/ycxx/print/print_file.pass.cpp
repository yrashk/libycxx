// [print.fun]: print(FILE* stream, fmt, args...) writes the formatted output to stream;
// println(stream, fmt, args...) appends '\n' (/5); println(stream) is print(stream, "\n")
// (/6); vprint_unicode / vprint_nonunicode(stream, fmt, args) write "the character
// representation of formatting arguments ... formatted according to specifications given in
// fmt to stream" (/10.2, /16). Checked by reading a temporary file back.
#include <print>
#include <format>
#include <cstdio>
#include <string>
#include "check.hpp"

static std::string slurp(std::FILE* f) {
  std::fflush(f);
  std::rewind(f);
  std::string r;
  int c;
  while ((c = std::fgetc(f)) != EOF) r += static_cast<char>(c);
  return r;
}

int main() {
  std::FILE* f = std::tmpfile();
  CHECK(f != nullptr);
  std::print(f, "{}-{}", "a", 1);
  std::println(f, " {:03}", 7);
  std::println(f);
  int v = 42;
  std::vprint_unicode(f, "u{}", std::make_format_args(v));
  std::vprint_nonunicode(f, "n{}", std::make_format_args(v));
  std::print(f, "{:x} {:.2f} {}", 255, 1.0, true);
  CHECK(slurp(f) == "a-1 007\n\nu42n42ff 1.00 true");
  std::fclose(f);

  bool threw = false;
  std::FILE* g = std::tmpfile();
  try {
    std::string fmt = "{:d}";
    std::vprint_nonunicode(g, fmt, std::make_format_args(fmt));
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  CHECK(slurp(g).empty());
  std::fclose(g);
  return 0;
}
