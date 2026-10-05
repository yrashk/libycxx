// [char.traits.specializations.char]/3: "The member eof() returns EOF."
// [char.traits.specializations.wchar.t]/2: "The member eof() returns WEOF."
// [char.traits.specializations.char8.t]/2: eof() "cannot appear as a valid UTF-8 code unit";
// [char.traits.specializations.char16.t]/2: "... cannot appear as a valid UTF-16 code unit";
// [char.traits.specializations.char32.t]/2: "... cannot appear as a Unicode code point".
// [tab:char.traits.req]: X::eof() returns "a value e such that X::eq_int_type(e,
// X::to_int_type(c)) is false for all values c." Checked exhaustively for the 8- and
// 16-bit character types and over the Unicode code space for char32_t.
// XFAIL: any  draft defect: char_traits<char16_t>::int_type is uint_least16_t (16 bits), so no eof() value differs from every code unit as [char.traits.require] asks (STATUS, Deliberate divergences)
// COUNTERPART: libcxx:strings/char.traits/char.traits.specializations/char.traits.specializations.(char|wchar_t)/eof.pass.cpp
#include <string_view>
#include <cstdio>
#include <cwchar>
#include "check.hpp"

template <class C>
bool eof_distinct_from(unsigned long first, unsigned long last) {
  using T = std::char_traits<C>;
  for (unsigned long v = first;; ++v) {
    if (T::eq_int_type(T::eof(), T::to_int_type(static_cast<C>(v)))) return false;
    if (v == last) break;
  }
  return true;
}

static_assert(std::char_traits<char>::eof() == EOF);
static_assert(std::char_traits<wchar_t>::eof() == WEOF);
static_assert(std::char_traits<char>::eq_int_type(std::char_traits<char>::eof(), EOF));
static_assert(std::char_traits<char32_t>::eof() > 0x10FFFF);  // not a code point

int main() {
  CHECK(eof_distinct_from<char>(0, 255));
  CHECK(eof_distinct_from<char8_t>(0, 255));
  CHECK(eof_distinct_from<char16_t>(0, 0xFFFF));
  CHECK(eof_distinct_from<char32_t>(0, 0x10FFFF));
  CHECK(eof_distinct_from<wchar_t>(0, 0xFFFF));
  // char: the byte 0xFF is a character, not end-of-file
  CHECK(std::char_traits<char>::to_int_type(static_cast<char>(0xFF)) != EOF);
  CHECK(std::char_traits<wchar_t>::to_int_type(L'a') != WEOF);
  return 0;
}
