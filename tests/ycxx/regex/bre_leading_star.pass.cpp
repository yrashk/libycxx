// [re.synopt] Table 118: basic selects the POSIX BRE grammar of IEEE 1003.1 (XBD 9.3.3): "The
// asterisk is special except when used: in a bracket expression; as the first character of an
// entire BRE (after an initial '^', if any); as the first character of a subexpression (after
// an initial '^', if any)". There it stands for itself.
// REQUIRES: exceptions
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

bool full(const std::string& s, const char* re) {
  try {
    return std::regex_match(s, std::regex(re, rc::basic));
  } catch (const std::regex_error&) {
    return false;
  }
}

int main() {
  CHECK(full("*a", "*a") && !full("a", "*a"));
  CHECK(full("*a", "^*a"));
  CHECK(full("*b", "\\(*b\\)"));
  CHECK(full("a*", "a[*]") && full("aaa", "a*"));
  // grep is BRE-based.
  CHECK(std::regex_match(std::string("*x"), std::regex("*x", rc::grep)));
  return 0;
}
