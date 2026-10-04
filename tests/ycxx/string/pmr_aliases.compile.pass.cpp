// [string.syn]: namespace pmr { template<class charT, class traits = char_traits<charT>>
// using basic_string = std::basic_string<charT, traits, polymorphic_allocator<charT>>;
// using string = basic_string<char>; u8string, u16string, u32string, wstring likewise }.
// <string> alone must make the aliases (and so polymorphic_allocator) nameable.
#include <string>
#include <type_traits>

template <class C>
using PA = std::pmr::polymorphic_allocator<C>;

static_assert(std::is_same_v<std::pmr::basic_string<char>, std::basic_string<char, std::char_traits<char>, PA<char>>>);
static_assert(std::is_same_v<std::pmr::string, std::basic_string<char, std::char_traits<char>, PA<char>>>);
static_assert(std::is_same_v<std::pmr::u8string, std::basic_string<char8_t, std::char_traits<char8_t>, PA<char8_t>>>);
static_assert(std::is_same_v<std::pmr::u16string, std::basic_string<char16_t, std::char_traits<char16_t>, PA<char16_t>>>);
static_assert(std::is_same_v<std::pmr::u32string, std::basic_string<char32_t, std::char_traits<char32_t>, PA<char32_t>>>);
static_assert(std::is_same_v<std::pmr::wstring, std::basic_string<wchar_t, std::char_traits<wchar_t>, PA<wchar_t>>>);

struct MyTraits : std::char_traits<char> {};
static_assert(std::is_same_v<std::pmr::basic_string<char, MyTraits>, std::basic_string<char, MyTraits, PA<char>>>);
static_assert(std::is_same_v<std::pmr::string::allocator_type, PA<char>>);

int main() {}
