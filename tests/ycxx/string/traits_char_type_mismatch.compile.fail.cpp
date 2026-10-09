// EXPECT-ERROR: error: static assertion failed[^\n]*std::basic_string: traits::char_type must be charT \(\[string\.require\]\)
// [string.require]/3 note 2: "The program is ill-formed if traits::char_type is not the same
// type as charT."
#include <string>

std::basic_string<char, std::char_traits<wchar_t>> s;
