// [ostream.formatted.print]: print(os, fmt, args...) formats with vformat(os.getloc(), ...)
// and inserts the result as a formatted output function; println(os, fmt, args...) appends
// '\n'; println(os) is print(os, "\n"). An exception thrown by formatting propagates without
// setting badbit (/4.2). A failing insertion sets badbit (/4.4).
#include <print>
#include <format>
#include <sstream>
#include <ostream>
#include <string>
#include "check.hpp"

struct Full : std::streambuf {
  int_type overflow(int_type) override { return traits_type::eof(); }
};

int main() {
  std::ostringstream os;
  std::print(os, "{} + {} = {}", 1, 2, 3);
  CHECK(os.str() == "1 + 2 = 3");
  std::println(os, "!");
  CHECK(os.str() == "1 + 2 = 3!\n");
  std::println(os);
  CHECK(os.str() == "1 + 2 = 3!\n\n");
  std::print(os, "{:>5}|{:<4}|{:^5}", "ab", 7, 'c');
  CHECK(os.str() == "1 + 2 = 3!\n\n   ab|7   |  c  ");

  std::ostringstream ex;
  bool threw = false;
  try {
    std::string fmt = "{:d}";
    std::vprint_nonunicode(ex, fmt, std::make_format_args(fmt));  // a string with 'd': format_error
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  CHECK(ex.good());  // no badbit for a formatting exception

  std::ostringstream vu;
  int n = 5;
  std::vprint_unicode(vu, "<{}>", std::make_format_args(n));
  CHECK(vu.str() == "<5>");

  Full full;
  std::ostream f(&full);
  std::print(f, "{}", 123);
  CHECK(f.bad());

  std::ostream null(nullptr);  // the sentry fails: nothing happens
  std::print(null, "{}", 1);
  CHECK(null.bad());
  return 0;
}
