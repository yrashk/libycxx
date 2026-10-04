// [tab:char.traits.req]:
//   X::not_eof(e): "e if X::eq_int_type(e,X::eof()) is false, otherwise a value f such that
//     X::eq_int_type(f,X::eof()) is false."
//   X::to_char_type(e): "if for some c, X::eq_int_type(e,X::to_int_type(c)) is true, c".
//   X::eq_int_type(e,f): "for all c and d, X::eq(c,d) is equal to
//     X::eq_int_type(X::to_int_type(c), X::to_int_type(d)); otherwise, yields true if e and f
//     are both copies of X::eof(); otherwise, yields false if one of e and f is a copy of
//     X::eof() and the other is not".
// All of these are constexpr in every specialization ([char.traits.specializations]).
#include <string_view>
#include "check.hpp"

template <class C>
constexpr bool basic(C a, C b) {
  using T = std::char_traits<C>;
  const auto ea = T::to_int_type(a);
  if (T::to_char_type(ea) != a) return false;
  if (!T::eq_int_type(ea, T::to_int_type(a))) return false;
  if (T::eq_int_type(ea, T::to_int_type(b)) != T::eq(a, b)) return false;
  if (!T::eq_int_type(T::eof(), T::eof())) return false;
  if (T::eq_int_type(T::eof(), ea) || T::eq_int_type(ea, T::eof())) return false;
  if (!T::eq_int_type(T::not_eof(ea), ea)) return false;
  if (T::eq_int_type(T::not_eof(T::eof()), T::eof())) return false;
  return true;
}

static_assert(basic<char>('a', 'b') && basic<char>(static_cast<char>(0xFF), '\0'));
static_assert(basic<char>(static_cast<char>(0x80), static_cast<char>(0x7F)));
static_assert(basic<wchar_t>(L'x', L'y') && basic<wchar_t>(static_cast<wchar_t>(0x10FFFF), L'\0'));
static_assert(basic<char8_t>(u8'a', static_cast<char8_t>(0xFF)));
static_assert(basic<char16_t>(u'a', static_cast<char16_t>(0xD800)));
static_assert(basic<char16_t>(static_cast<char16_t>(0xFFFE), u'\0'));
static_assert(basic<char32_t>(U'\U0010FFFF', U'\0') && basic<char32_t>(U'a', static_cast<char32_t>(0xD800)));

// Exhaustive round trips for the byte-sized types: eq_int_type mirrors eq for every pair,
// and to_char_type inverts to_int_type.
template <class C>
bool exhaustive() {
  using T = std::char_traits<C>;
  for (int i = 0; i < 256; ++i) {
    C a = static_cast<C>(i);
    if (T::to_char_type(T::to_int_type(a)) != a) return false;
    if (T::eq_int_type(T::not_eof(T::to_int_type(a)), T::to_int_type(a)) == false) return false;
    for (int j = 0; j < 256; ++j) {
      C b = static_cast<C>(j);
      if (T::eq_int_type(T::to_int_type(a), T::to_int_type(b)) != T::eq(a, b)) return false;
    }
  }
  return true;
}

int main() {
  CHECK(exhaustive<char>());
  CHECK(exhaustive<char8_t>());
  CHECK(basic<char16_t>(u'z', u'y'));
  return 0;
}
