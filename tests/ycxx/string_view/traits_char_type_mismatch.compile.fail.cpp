// [char.traits.general]/3: "If X::char_type is not the same type as C, the program is
// ill-formed." [string.view.template.general]/1, Note 1: "The program is ill-formed if
// traits::char_type is not the same type as charT."
#include <string_view>

std::basic_string_view<char, std::char_traits<wchar_t>> v;
