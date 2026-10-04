// [string.view.ops]: substr(pos, n) "Throws: out_of_range if pos > size()." An exception
// that escapes a constant expression makes it not a core constant expression
// ([expr.const]), so this constexpr variable is ill-formed.
#include <string_view>

constexpr std::string_view v = std::string_view("ab").substr(3);
