// libycxx core: char_traits ([char.traits]) and the freestanding types it names: mbstate_t and
// wint_t ([cwchar.syn], freestanding), streamoff and the fpos declaration ([iosfwd.syn]).
//
// mbstate_t (DECISIONS §3): hosted, the C library's ::mbstate_t ([support.c.headers.other]/1),
// read with its <wchar.h>; freestanding, core's own type with the same layout
// (ycxx/core/mbstate.hpp).
#pragma once

#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/cstdint.hpp>
#if YCXX_HOSTED
#  include <ycxx/hosted/c_wchar.hpp>
#else
#  include <ycxx/core/mbstate.hpp>
#endif

namespace [[gnu::visibility("hidden")]] std {

using wint_t = __WINT_TYPE__;

using streamoff = long long;
template <class stateT>
class fpos; // defined with the iostreams
using streampos = fpos<mbstate_t>;
using wstreampos = fpos<mbstate_t>;
using u8streampos = fpos<mbstate_t>;
using u16streampos = fpos<mbstate_t>;
using u32streampos = fpos<mbstate_t>;

template <class charT>
struct char_traits; // only the specializations below are defined

} // namespace std

// In ycxx::adl_free (DECISIONS §2): char_traits<C> is a template argument of basic_string_view,
// so this base's namespace is an associated namespace for ADL on every string view.
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The operations shared by all five specializations. CharT is the character type, IntT its
// int_type, U the type whose built-in < defines lt() (unsigned char for char).
template <class CharT, class IntT, class U, IntT Eof>
struct char_traits_base {
  using char_type = CharT;
  using int_type = IntT;
  using off_type = std::streamoff;
  using pos_type = std::fpos<std::mbstate_t>;
  using state_type = std::mbstate_t;
  using comparison_category = std::strong_ordering;

  static constexpr void assign(char_type& c1, const char_type& c2) noexcept { c1 = c2; }
  static constexpr bool eq(char_type c1, char_type c2) noexcept { return c1 == c2; }
  static constexpr bool lt(char_type c1, char_type c2) noexcept {
    return static_cast<U>(c1) < static_cast<U>(c2);
  }

  static constexpr int compare(const char_type* s1, const char_type* s2, std::size_t n) {
    if constexpr (sizeof(char_type) == 1) {
      if !consteval {
        return __builtin_memcmp(s1, s2, n); // compares as unsigned char, like lt()
      }
    }
    for (std::size_t i = 0; i < n; ++i) {
      if (lt(s1[i], s2[i]))
        return -1;
      if (lt(s2[i], s1[i]))
        return 1;
    }
    return 0;
  }
  static constexpr std::size_t length(const char_type* s) noexcept {
    // strlen/memchr are libc functions: only hosted builds may call them (freestanding builds
    // provide just memcpy/memmove/memset/memcmp).
    if constexpr (sizeof(char_type) == 1 && ycxx::detail::cfg::hosted) {
      if !consteval {
        return __builtin_strlen(reinterpret_cast<const char*>(s));
      }
    }
    std::size_t n = 0;
    while (!eq(s[n], char_type()))
      ++n;
    return n;
  }
  static constexpr const char_type* find(const char_type* s, std::size_t n, const char_type& a) {
    if constexpr (sizeof(char_type) == 1 && ycxx::detail::cfg::hosted) {
      if !consteval {
        return static_cast<const char_type*>(__builtin_memchr(s, static_cast<unsigned char>(a), n));
      }
    }
    for (std::size_t i = 0; i < n; ++i)
      if (eq(s[i], a))
        return s + i;
    return nullptr;
  }
  static constexpr char_type* move(char_type* s1, const char_type* s2, std::size_t n) {
    if (n == 0)
      return s1;
    if consteval {
      // Pointers into unrelated arrays cannot be ordered here, so go through a temporary.
      char_type* tmp = new char_type[n];
      for (std::size_t i = 0; i < n; ++i)
        tmp[i] = s2[i];
      for (std::size_t i = 0; i < n; ++i)
        s1[i] = tmp[i];
      delete[] tmp;
    } else {
      __builtin_memmove(s1, s2, n * sizeof(char_type));
    }
    return s1;
  }
  static constexpr char_type* copy(char_type* s1, const char_type* s2, std::size_t n) {
    if consteval {
      for (std::size_t i = 0; i < n; ++i)
        s1[i] = s2[i];
    } else {
      if (n != 0)
        __builtin_memcpy(s1, s2, n * sizeof(char_type));
    }
    return s1;
  }
  static constexpr char_type* assign(char_type* s, std::size_t n, char_type a) {
    for (std::size_t i = 0; i < n; ++i)
      s[i] = a;
    return s;
  }

  static constexpr int_type eof() noexcept { return Eof; }
  static constexpr int_type not_eof(int_type c) noexcept { return c == Eof ? int_type() : c; }
  static constexpr char_type to_char_type(int_type c) noexcept { return static_cast<char_type>(c); }
  static constexpr int_type to_int_type(char_type c) noexcept { return static_cast<int_type>(static_cast<U>(c)); }
  static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept { return c1 == c2; }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

// [char.traits.specializations]. eof() values: EOF (-1) for char; for the others a value that is
// not a valid code unit / code point (all bits set), and WEOF for wchar_t.
template <>
struct char_traits<char> : ycxx::adl_free::char_traits_base<char, int, unsigned char, -1> {};
template <>
struct char_traits<char8_t> : ycxx::adl_free::char_traits_base<char8_t, unsigned int, char8_t, 0xFFFF'FFFFu> {};
template <>
struct char_traits<char16_t>
    : ycxx::adl_free::char_traits_base<char16_t, uint_least16_t, char16_t, uint_least16_t(0xFFFF)> {};
template <>
struct char_traits<char32_t>
    : ycxx::adl_free::char_traits_base<char32_t, uint_least32_t, char32_t, uint_least32_t(0xFFFF'FFFFu)> {};
template <>
struct char_traits<wchar_t> : ycxx::adl_free::char_traits_base<wchar_t, wint_t, wchar_t, static_cast<wint_t>(-1)> {};

} // namespace std
