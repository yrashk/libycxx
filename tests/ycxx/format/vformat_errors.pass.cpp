// [format.err.report]/1: "Formatting functions throw format_error if an argument fmt is
// passed that is not a format string for args." Checked through vformat for unmatched
// braces, a missing argument, mixed automatic and manual indexing
// ([format.string.general]/4), invalid format-specs for the argument type, and the dynamic
// width/precision rules of [format.string.std]/10 (non-integer argument; negative value).
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

template <class... Args>
bool throws(std::string_view fmt, Args&&... args) {
  try {
    (void)std::vformat(fmt, std::make_format_args(args...));
  } catch (const std::format_error&) {
    return true;
  }
  return false;
}

int main() {
  int i = 1;
  double d = 1.5;
  const char* s = "s";
  char c = 'c';
  bool b = true;
  int neg = -1;
  CHECK(!throws("{}", i));
  CHECK(throws("{", i));
  CHECK(throws("}", i));
  CHECK(throws("{0", i));
  CHECK(throws("x}y", i));
  CHECK(throws("{}{}", i));       // missing argument
  CHECK(throws("{1}", i));
  CHECK(throws("{0}{}", i, i));   // manual then automatic
  CHECK(throws("{}{0}", i, i));   // automatic then manual
  CHECK(throws("{:{}}", i, i) == false);
  CHECK(throws("{0:{}}", i, i));  // mixing inside a format-spec
  CHECK(throws("{:s}", i));       // invalid type for int
  CHECK(throws("{:.2}", i));      // precision is invalid for integers
  CHECK(throws("{:f}", s));
  CHECK(throws("{:+}", s));       // sign invalid for strings
  CHECK(throws("{:#}", s));
  CHECK(throws("{:0}", s));
  CHECK(throws("{:+}", c));       // sign invalid for charT without integer type
  CHECK(!throws("{:+d}", c));
  CHECK(throws("{:#}", b));
  CHECK(throws("{:+}", b));
  CHECK(throws("{:x}", d));
  CHECK(throws("{:d}", d));
  CHECK(throws("{:c}", d));
  CHECK(throws("{:p}", i));
  CHECK(throws("{:L}", s));       // L only for arithmetic types
  CHECK(throws("{:<<<}", i));
  CHECK(throws("{:{}}", i, d));   // dynamic width must be an integer
  CHECK(throws("{:.{}}", d, s));
  CHECK(throws("{:{}}", i, neg)); // negative dynamic width
  CHECK(throws("{:.{}}", d, neg));
  CHECK(throws("{:{}}", i, c));   // a char is not a standard integer type
  CHECK(throws("{:{}}", i, b));
  CHECK(throws("{:0}", i) == false);
  CHECK(throws("{:00}", i));      // width is a positive-integer: the second 0 is not valid
  CHECK(throws("{:{", i));
  CHECK(throws("{:?}", i));       // ? is not an integer presentation type
  CHECK(throws("{:{}}", i));      // dynamic width refers to a missing argument
  CHECK(throws("{: }", s));
  // format_error is a runtime_error.
  try {
    (void)std::vformat("{:s}", std::make_format_args(i));
    CHECK(false);
  } catch (const std::runtime_error& e) {
    CHECK(e.what() != nullptr);
  }
}
