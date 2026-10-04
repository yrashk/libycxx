// [istream.unformatted]/13: get(sb, delim) extracts and inserts characters until "(13.4) an
// exception occurs (in which case, the exception is caught but not rethrown)"; /14: "If the
// function inserts no characters, ios_base::failbit is set in the input function's local error
// state before setstate is called." The exception comes from the output buffer sb, and the
// paragraph says it is not rethrown, even when badbit is set in exceptions().
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include "check.hpp"

struct ThrowingSink : std::streambuf {
  int_type overflow(int_type) override { throw 5; }
};

// inserts one character, then throws
struct OneThenThrow : std::streambuf {
  std::string got;
  int_type overflow(int_type c) override {
    if (!got.empty()) throw 6;
    got.push_back(traits_type::to_char_type(c));
    return c;
  }
};

int main() {
  {
    std::istringstream is("cd\nx");
    ThrowingSink ts;
    is.get(ts);
    CHECK(is.fail());
    CHECK(is.gcount() == 0);
  }
  {
    std::istringstream is("cd\nx");
    is.exceptions(std::ios_base::badbit);
    ThrowingSink ts;
    bool escaped = false;
    try {
      is.get(ts);
    } catch (...) {
      escaped = true;
    }
    CHECK(!escaped);
    CHECK(is.fail());
  }
  {
    // one character inserted before the exception: no failbit, gcount() == 1
    std::istringstream is("cd\nx");
    is.exceptions(std::ios_base::badbit);
    OneThenThrow sb;
    bool escaped = false;
    try {
      is.get(sb);
    } catch (...) {
      escaped = true;
    }
    CHECK(!escaped);
    CHECK(sb.got == "c");
    CHECK(is.gcount() == 1);
  }
  return 0;
}
