// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::out_of_range';[^\n]*basic_string_view::substr: pos > size\(\)
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*basic_string_view::substr: pos > size\(\)
// EXPECT-ERROR-CLANG: error: constexpr variable 'v' must be initialized by a constant expression
// EXPECT-ERROR-CLANG: note: in call to[^\n]*\.substr\(3
// [string.view.ops]: substr(pos, n) "Throws: out_of_range if pos > size()." An exception
// that escapes a constant expression makes it not a core constant expression
// ([expr.const]), so this constexpr variable is ill-formed.
#include <string_view>

constexpr std::string_view v = std::string_view("ab").substr(3);
