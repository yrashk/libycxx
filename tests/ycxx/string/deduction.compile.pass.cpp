// [string.cons]/26-27, [basic.string.general] deduction guides:
//   basic_string(InputIterator, InputIterator, Allocator = Allocator())
//     -> basic_string<iter_value_t, char_traits<iter_value_t>, Allocator>
//   basic_string(from_range_t, R&&, Allocator = Allocator())
//   explicit basic_string(basic_string_view<charT, traits>, const Allocator& = Allocator())
//   basic_string(basic_string_view<charT, traits>, size_type, size_type, const Allocator& = ...)
// [sequence.reqmts]/69.3: a guide does not participate if a non-allocator is deduced for
// Allocator. [string.cons]/16,18: the const charT* and (n, c) constructors are constrained on
// Allocator qualifying as an allocator (this affects CTAD).
#include <string>
#include <string_view>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"

void f() {
  const char* p = "abc";
  std::basic_string a(p, p + 3);
  static_assert(std::is_same_v<decltype(a), std::string>);
  const wchar_t* w = L"abc";
  std::basic_string b(w, w + 3, std::allocator<wchar_t>());
  static_assert(std::is_same_v<decltype(b), std::wstring>);
  std::basic_string c(p, p + 3, MinimalAlloc<char>());
  static_assert(std::is_same_v<decltype(c),
                               std::basic_string<char, std::char_traits<char>, MinimalAlloc<char>>>);

  std::basic_string d(std::from_range, std::u16string_view(u"xy"));
  static_assert(std::is_same_v<decltype(d), std::u16string>);
  std::basic_string e(std::from_range, std::views::iota(U'a', U'c'), MinimalAlloc<char32_t>());
  static_assert(std::is_same_v<decltype(e),
                               std::basic_string<char32_t, std::char_traits<char32_t>,
                                                 MinimalAlloc<char32_t>>>);

  std::basic_string g(std::string_view("sv"));
  static_assert(std::is_same_v<decltype(g), std::string>);
  std::basic_string h(std::u8string_view(u8"sv"), std::allocator<char8_t>());
  static_assert(std::is_same_v<decltype(h), std::u8string>);
  std::basic_string i(std::string_view("hello"), 1, 2);
  static_assert(std::is_same_v<decltype(i), std::string>);
  std::basic_string j(std::wstring_view(L"hello"), 1u, 2u, std::allocator<wchar_t>());
  static_assert(std::is_same_v<decltype(j), std::wstring>);

  std::basic_string k("lit");
  static_assert(std::is_same_v<decltype(k), std::string>);
  std::basic_string l(3, 'x');
  static_assert(std::is_same_v<decltype(l), std::string>);
  std::basic_string m(U"lit", std::allocator<char32_t>());
  static_assert(std::is_same_v<decltype(m), std::u32string>);
  std::basic_string n{std::string("copy")};
  static_assert(std::is_same_v<decltype(n), std::string>);
}

// Deduction is SFINAE-friendly: with an argument that is not an allocator, no guide applies.
template <class... Args>
concept deducible = requires(Args... args) { std::basic_string(args...); };
static_assert(deducible<const char*, const char*>);
static_assert(!deducible<const char*, const char*, int>);
static_assert(!deducible<std::string_view, int>);
