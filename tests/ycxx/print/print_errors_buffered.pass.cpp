// [print.fun]/11, /17: vprint_unicode / vprint_nonunicode throw "Any exception thrown by the
// call to vformat" and "system_error if writing to the terminal or stream fails"; print /
// println forward to them (/2, /5). /8, /14 (P3107, P3235): vprint_unicode_buffered(stream,
// fmt, args) is "string out = vformat(fmt, args); vprint_unicode(stream, "{}",
// make_format_args(out));" (likewise nonunicode). /10.2: for a stream that is not a terminal
// the output is written "unchanged", including invalid UTF-8 code units.
// [format.formatter.locking]: enable_nonlocking_formatter_optimization is true for the
// standard arithmetic, character and string formatters, false by default for user types.
// REQUIRES: exceptions
#include <print>
#include <format>
#include <cstdio>
#include <string>
#include <system_error>
#include "fs_tmpdir.hpp"
#include "check.hpp"

struct User {};
template <>
struct std::formatter<User> : std::formatter<int> {
  auto format(User, std::format_context& ctx) const { return std::formatter<int>::format(5, ctx); }
};

static_assert(std::enable_nonlocking_formatter_optimization<int>);
static_assert(std::enable_nonlocking_formatter_optimization<double>);
static_assert(std::enable_nonlocking_formatter_optimization<char>);
static_assert(std::enable_nonlocking_formatter_optimization<const char*>);
static_assert(std::enable_nonlocking_formatter_optimization<std::string>);
static_assert(std::enable_nonlocking_formatter_optimization<std::string_view>);
static_assert(!std::enable_nonlocking_formatter_optimization<User>);

int main() {
  TmpDir dir;
  const std::string p = dir / "f";
  write_file(p, "");
  {
    // writing to a stream opened for reading fails: system_error
    FILE* f = std::fopen(p.c_str(), "r");
    CHECK(f != nullptr);
    int thrown = 0;
    try {
      std::print(f, "{}", 12345);
      std::fflush(f);
    } catch (const std::system_error&) {
      ++thrown;
    }
    try {
      std::vprint_nonunicode(f, "abc{}", std::make_format_args("x"));
    } catch (const std::system_error&) {
      ++thrown;
    }
    try {
      int v = 1;
      std::vprint_unicode(f, "abc{}", std::make_format_args(v));
    } catch (const std::system_error&) {
      ++thrown;
    }
    try {
      std::println(f, "{}", User{});
    } catch (const std::system_error&) {
      ++thrown;
    }
    CHECK(thrown == 4);
    std::fclose(f);
  }
  {
    // format errors propagate as format_error
    FILE* f = std::fopen(p.c_str(), "w");
    int v = 1;
    bool caught = false;
    try {
      std::vprint_unicode(f, "ok {} {}", std::make_format_args(v));
    } catch (const std::format_error&) {
      caught = true;
    }
    CHECK(caught);
    caught = false;
    try {
      std::vprint_nonunicode_buffered(f, "{:d}", std::make_format_args("str"));
    } catch (const std::format_error&) {
      caught = true;
    }
    CHECK(caught);
    std::fclose(f);
  }
  {
    // the buffered variants write the formatted text; invalid UTF-8 is written unchanged
    FILE* f = std::fopen(p.c_str(), "w");
    int v = 42;
    std::vprint_unicode_buffered(f, "[{:>4}]", std::make_format_args(v));
    std::vprint_nonunicode_buffered(f, "<{}>", std::make_format_args(v));
    std::string bad = "\xff\xfe";
    std::vprint_unicode(f, "{}", std::make_format_args(bad));
    std::print(f, "{}|{}", User{}, 'c');  // a user type: the buffered path
    std::fclose(f);
    CHECK(read_file(p) == "[  42]<42>\xff\xfe" "5|c");
  }
  return 0;
}
