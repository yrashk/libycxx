// [re.badexp], [re.err], [re.regex.construct]: an invalid regular expression makes the
// basic_regex constructor throw regex_error (derived from runtime_error) whose code()
// identifies the error: error_paren (mismatched parentheses), error_brack (mismatched [ ]),
// error_brace (mismatched { }), error_badbrace (invalid range in {}), error_badrepeat (a
// repeat not preceded by a valid expression), error_escape (invalid or trailing escape),
// error_backref (invalid back reference), error_range (invalid character range), error_ctype
// (invalid class name, [re.grammar]/11).
// REQUIRES: exceptions
#include <regex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"

namespace rc = std::regex_constants;
static_assert(std::is_base_of_v<std::runtime_error, std::regex_error>);

bool fails_with(const char* re, rc::error_type code, rc::syntax_option_type f = rc::ECMAScript) {
  try {
    std::regex r(re, f);
  } catch (const std::regex_error& e) {
    return e.code() == code;
  }
  return false;
}

int main() {
  CHECK(fails_with("(a", rc::error_paren));
  CHECK(fails_with("a)", rc::error_paren));
  CHECK(fails_with("[ab", rc::error_brack));
  CHECK(fails_with("a{1", rc::error_brace));
  CHECK(fails_with("a{2,1}", rc::error_badbrace));
  CHECK(fails_with("*a", rc::error_badrepeat));
  CHECK(fails_with("a**", rc::error_badrepeat));
  CHECK(fails_with("a\\", rc::error_escape));
  CHECK(fails_with("(a)\\2", rc::error_backref));
  CHECK(fails_with("[z-a]", rc::error_range));
  CHECK(fails_with("[[:nonsense:]]", rc::error_ctype));
  CHECK(fails_with("\\(a", rc::error_paren, rc::basic));

  std::regex_error e(rc::error_space);
  CHECK(e.code() == rc::error_space && e.what() != nullptr);
  static_assert(std::is_same_v<decltype(e.code()), rc::error_type>);
  return 0;
}
