// [string.syn]: basic_string is provided for char, char8_t, char16_t, char32_t and wchar_t
// (string, u8string, u16string, u32string, wstring); the full interface works for each,
// using char_traits<charT> (length stops at charT()).
#include <string>
#include "check.hpp"

template <class C>
constexpr bool test(const C* lit) {  // lit is "abc" in the given character type
  std::basic_string<C> s = lit;
  if (s.size() != 3 || s[0] != C('a')) return false;
  s += lit;
  s.insert(s.begin(), C('<'));
  s.push_back(C('>'));
  if (s.size() != 8 || s.front() != C('<') || s.back() != C('>')) return false;
  if (s.find(lit, 2) != 4 || s.rfind(C('a')) != 4) return false;
  s.replace(1, 3, 2, C('x'));
  if (s.size() != 7 || s[1] != C('x') || s[3] != C('a')) return false;
  if (s.substr(3, 3) != lit) return false;
  if (!(s.substr(3, 3) == std::basic_string<C>(lit))) return false;
  s.resize(10, C('z'));
  if (s[9] != C('z') || s.c_str()[10] != C()) return false;
  std::basic_string<C> big(100, C('b'));
  big.append(s);
  if (big.size() != 110 || big.compare(0, 2, std::basic_string<C>(2, C('b'))) != 0) return false;
  return true;
}

static_assert(test("abc"));
static_assert(test(L"abc"));
static_assert(test(u8"abc"));
static_assert(test(u"abc"));
static_assert(test(U"abc"));

int main() {
  CHECK(test("abc"));
  CHECK(test(L"abc"));
  CHECK(test(u8"abc"));
  CHECK(test(u"abc"));
  CHECK(test(U"abc"));
  return 0;
}
