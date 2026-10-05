// libycxx core: the freestanding functions of <cstring> and <cwchar> ([cstring.syn],
// [cwchar.syn]) for a freestanding implementation, which has no C library to take them from.
//
// Only the freestanding <cstring>/<cwchar> include this; hosted, they are the C library's
// functions. The byte functions memcpy, memmove, memset and memcmp go through the compilers'
// builtins, which may call the functions of the same name: both GCC and Clang require a
// freestanding environment to provide those four. Everything else is written out here. As in
// <cmath>, every function is a template with a defaulted parameter, so a C function of the same
// name that a program declares itself wins ties under `using namespace std;`.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/char_traits.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::c_str {
// The elements as their unsigned type, as the C comparison functions compare them ("unsigned
// char" for the byte functions; wchar_t values compare as wchar_t).
template <class C>
using cmp_t = std::conditional_t<std::is_same_v<C, char>, unsigned char, C>;

template <class C>
constexpr std::size_t len(const C* s) noexcept {
  std::size_t n = 0;
  while (s[n] != C())
    ++n;
  return n;
}
template <class C>
constexpr int cmp(const C* a, const C* b, std::size_t n) noexcept {
  for (std::size_t i = 0; i != n; ++i) {
    const cmp_t<C> x = static_cast<cmp_t<C>>(a[i]), y = static_cast<cmp_t<C>>(b[i]);
    if (x != y)
      return x < y ? -1 : 1;
    if (x == 0)
      return 0;
  }
  return 0;
}
template <class C>
constexpr int mem_cmp(const C* a, const C* b, std::size_t n) noexcept {
  for (std::size_t i = 0; i != n; ++i)
    if (a[i] != b[i])
      return static_cast<cmp_t<C>>(a[i]) < static_cast<cmp_t<C>>(b[i]) ? -1 : 1;
  return 0;
}
template <class C>
constexpr C* copy(C* d, const C* s, std::size_t n) noexcept {
  std::size_t i = 0;
  for (; i != n && s[i] != C(); ++i)
    d[i] = s[i];
  for (; i != n; ++i) // strncpy pads with null characters
    d[i] = C();
  return d;
}
template <class C>
constexpr C* cat(C* d, const C* s, std::size_t n) noexcept {
  C* e = d + ::ycxx::detail::c_str::len(d);
  std::size_t i = 0;
  for (; i != n && s[i] != C(); ++i)
    e[i] = s[i];
  e[i] = C();
  return d;
}
template <class C>
constexpr const C* chr(const C* s, C c) noexcept {
  for (;; ++s) {
    if (*s == c)
      return s;
    if (*s == C())
      return nullptr;
  }
}
template <class C>
constexpr const C* rchr(const C* s, C c) noexcept {
  const C* r = nullptr;
  for (;; ++s) {
    if (*s == c)
      r = s;
    if (*s == C())
      return r;
  }
}
template <class C>
constexpr const C* mem_chr(const C* s, C c, std::size_t n) noexcept {
  for (std::size_t i = 0; i != n; ++i)
    if (s[i] == c)
      return s + i;
  return nullptr;
}
// The length of the initial segment of s whose characters are (in == true) or are not in set.
template <class C>
constexpr std::size_t span(const C* s, const C* set, bool in) noexcept {
  std::size_t n = 0;
  for (; s[n] != C(); ++n)
    if ((::ycxx::detail::c_str::chr(set, s[n]) != nullptr) != in)
      break;
  return n;
}
template <class C>
constexpr const C* pbrk(const C* s, const C* set) noexcept {
  s += ::ycxx::detail::c_str::span(s, set, false);
  return *s != C() ? s : nullptr;
}
template <class C>
constexpr const C* str(const C* s, const C* t) noexcept {
  const std::size_t m = ::ycxx::detail::c_str::len(t);
  for (;; ++s) {
    if (::ycxx::detail::c_str::mem_cmp(s, t, m) == 0)
      return s;
    if (*s == C())
      return nullptr;
  }
}
template <class C>
constexpr C* tok(C* s, const C* sep, C** save) noexcept {
  if (s == nullptr)
    s = *save;
  if (s == nullptr)
    return nullptr;
  s += ::ycxx::detail::c_str::span(s, sep, true);
  if (*s == C()) {
    *save = nullptr;
    return nullptr;
  }
  C* e = s + ::ycxx::detail::c_str::span(s, sep, false);
  if (*e == C()) {
    *save = nullptr;
  } else {
    *e = C();
    *save = e + 1;
  }
  return s;
}
}} // namespace ycxx::detail::c_str

namespace [[gnu::visibility("hidden")]] std {

// ---- <cstring> ----
template <class = void>
inline void* memcpy(void* s1, const void* s2, size_t n) noexcept {
  return __builtin_memcpy(s1, s2, n);
}
template <class = void>
inline void* memmove(void* s1, const void* s2, size_t n) noexcept {
  return __builtin_memmove(s1, s2, n);
}
template <class = void>
inline void* memset(void* s, int c, size_t n) noexcept {
  return __builtin_memset(s, c, n);
}
template <class = void>
inline int memcmp(const void* s1, const void* s2, size_t n) noexcept {
  return __builtin_memcmp(s1, s2, n);
}
template <class = void>
inline void* memccpy(void* s1, const void* s2, int c, size_t n) noexcept {
  auto* d = static_cast<unsigned char*>(s1);
  const auto* s = static_cast<const unsigned char*>(s2);
  for (size_t i = 0; i != n; ++i) {
    d[i] = s[i];
    if (s[i] == static_cast<unsigned char>(c))
      return d + i + 1;
  }
  return nullptr;
}
template <class = void>
inline char* strcpy(char* s1, const char* s2) noexcept {
  return static_cast<char*>(__builtin_memcpy(s1, s2, ::ycxx::detail::c_str::len(s2) + 1));
}
template <class = void>
inline char* strncpy(char* s1, const char* s2, size_t n) noexcept {
  return ::ycxx::detail::c_str::copy(s1, s2, n);
}
template <class = void>
inline char* strcat(char* s1, const char* s2) noexcept {
  return ::ycxx::detail::c_str::cat(s1, s2, static_cast<size_t>(-1));
}
template <class = void>
inline char* strncat(char* s1, const char* s2, size_t n) noexcept {
  return ::ycxx::detail::c_str::cat(s1, s2, n);
}
template <class = void>
inline int strcmp(const char* s1, const char* s2) noexcept {
  return ::ycxx::detail::c_str::cmp(s1, s2, static_cast<size_t>(-1));
}
template <class = void>
inline int strncmp(const char* s1, const char* s2, size_t n) noexcept {
  return ::ycxx::detail::c_str::cmp(s1, s2, n);
}
template <class = void>
inline const void* memchr(const void* s, int c, size_t n) noexcept {
  return ::ycxx::detail::c_str::mem_chr(static_cast<const unsigned char*>(s), static_cast<unsigned char>(c), n);
}
template <class = void>
inline void* memchr(void* s, int c, size_t n) noexcept {
  return const_cast<unsigned char*>(
      ::ycxx::detail::c_str::mem_chr(static_cast<const unsigned char*>(s), static_cast<unsigned char>(c), n));
}
template <class = void>
inline const char* strchr(const char* s, int c) noexcept {
  return ::ycxx::detail::c_str::chr(s, static_cast<char>(c));
}
template <class = void>
inline char* strchr(char* s, int c) noexcept {
  return const_cast<char*>(::ycxx::detail::c_str::chr(static_cast<const char*>(s), static_cast<char>(c)));
}
template <class = void>
inline size_t strcspn(const char* s1, const char* s2) noexcept {
  return ::ycxx::detail::c_str::span(s1, s2, false);
}
template <class = void>
inline const char* strpbrk(const char* s1, const char* s2) noexcept {
  return ::ycxx::detail::c_str::pbrk(s1, s2);
}
template <class = void>
inline char* strpbrk(char* s1, const char* s2) noexcept {
  return const_cast<char*>(::ycxx::detail::c_str::pbrk(static_cast<const char*>(s1), s2));
}
template <class = void>
inline const char* strrchr(const char* s, int c) noexcept {
  return ::ycxx::detail::c_str::rchr(s, static_cast<char>(c));
}
template <class = void>
inline char* strrchr(char* s, int c) noexcept {
  return const_cast<char*>(::ycxx::detail::c_str::rchr(static_cast<const char*>(s), static_cast<char>(c)));
}
template <class = void>
inline size_t strspn(const char* s1, const char* s2) noexcept {
  return ::ycxx::detail::c_str::span(s1, s2, true);
}
template <class = void>
inline const char* strstr(const char* s1, const char* s2) noexcept {
  return ::ycxx::detail::c_str::str(s1, s2);
}
template <class = void>
inline char* strstr(char* s1, const char* s2) noexcept {
  return const_cast<char*>(::ycxx::detail::c_str::str(static_cast<const char*>(s1), s2));
}
template <class = void>
inline size_t strlen(const char* s) noexcept {
  return ::ycxx::detail::c_str::len(s);
}

// ---- <cwchar> ----
template <class = void>
inline wchar_t* wcscpy(wchar_t* s1, const wchar_t* s2) noexcept {
  return static_cast<wchar_t*>(__builtin_memcpy(s1, s2, (::ycxx::detail::c_str::len(s2) + 1) * sizeof(wchar_t)));
}
template <class = void>
inline wchar_t* wcsncpy(wchar_t* s1, const wchar_t* s2, size_t n) noexcept {
  return ::ycxx::detail::c_str::copy(s1, s2, n);
}
template <class = void>
inline wchar_t* wmemcpy(wchar_t* s1, const wchar_t* s2, size_t n) noexcept {
  return static_cast<wchar_t*>(__builtin_memcpy(s1, s2, n * sizeof(wchar_t)));
}
template <class = void>
inline wchar_t* wmemmove(wchar_t* s1, const wchar_t* s2, size_t n) noexcept {
  return static_cast<wchar_t*>(__builtin_memmove(s1, s2, n * sizeof(wchar_t)));
}
template <class = void>
inline wchar_t* wcscat(wchar_t* s1, const wchar_t* s2) noexcept {
  return ::ycxx::detail::c_str::cat(s1, s2, static_cast<size_t>(-1));
}
template <class = void>
inline wchar_t* wcsncat(wchar_t* s1, const wchar_t* s2, size_t n) noexcept {
  return ::ycxx::detail::c_str::cat(s1, s2, n);
}
template <class = void>
inline int wcscmp(const wchar_t* s1, const wchar_t* s2) noexcept {
  return ::ycxx::detail::c_str::cmp(s1, s2, static_cast<size_t>(-1));
}
template <class = void>
inline int wcsncmp(const wchar_t* s1, const wchar_t* s2, size_t n) noexcept {
  return ::ycxx::detail::c_str::cmp(s1, s2, n);
}
template <class = void>
inline int wmemcmp(const wchar_t* s1, const wchar_t* s2, size_t n) noexcept {
  return ::ycxx::detail::c_str::mem_cmp(s1, s2, n);
}
template <class = void>
inline const wchar_t* wcschr(const wchar_t* s, wchar_t c) noexcept {
  return ::ycxx::detail::c_str::chr(s, c);
}
template <class = void>
inline wchar_t* wcschr(wchar_t* s, wchar_t c) noexcept {
  return const_cast<wchar_t*>(::ycxx::detail::c_str::chr(static_cast<const wchar_t*>(s), c));
}
template <class = void>
inline size_t wcscspn(const wchar_t* s1, const wchar_t* s2) noexcept {
  return ::ycxx::detail::c_str::span(s1, s2, false);
}
template <class = void>
inline const wchar_t* wcspbrk(const wchar_t* s1, const wchar_t* s2) noexcept {
  return ::ycxx::detail::c_str::pbrk(s1, s2);
}
template <class = void>
inline wchar_t* wcspbrk(wchar_t* s1, const wchar_t* s2) noexcept {
  return const_cast<wchar_t*>(::ycxx::detail::c_str::pbrk(static_cast<const wchar_t*>(s1), s2));
}
template <class = void>
inline const wchar_t* wcsrchr(const wchar_t* s, wchar_t c) noexcept {
  return ::ycxx::detail::c_str::rchr(s, c);
}
template <class = void>
inline wchar_t* wcsrchr(wchar_t* s, wchar_t c) noexcept {
  return const_cast<wchar_t*>(::ycxx::detail::c_str::rchr(static_cast<const wchar_t*>(s), c));
}
template <class = void>
inline size_t wcsspn(const wchar_t* s1, const wchar_t* s2) noexcept {
  return ::ycxx::detail::c_str::span(s1, s2, true);
}
template <class = void>
inline const wchar_t* wcsstr(const wchar_t* s1, const wchar_t* s2) noexcept {
  return ::ycxx::detail::c_str::str(s1, s2);
}
template <class = void>
inline wchar_t* wcsstr(wchar_t* s1, const wchar_t* s2) noexcept {
  return const_cast<wchar_t*>(::ycxx::detail::c_str::str(static_cast<const wchar_t*>(s1), s2));
}
template <class = void>
inline wchar_t* wcstok(wchar_t* s1, const wchar_t* s2, wchar_t** ptr) noexcept {
  return ::ycxx::detail::c_str::tok(s1, s2, ptr);
}
template <class = void>
inline const wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n) noexcept {
  return ::ycxx::detail::c_str::mem_chr(s, c, n);
}
template <class = void>
inline wchar_t* wmemchr(wchar_t* s, wchar_t c, size_t n) noexcept {
  return const_cast<wchar_t*>(::ycxx::detail::c_str::mem_chr(static_cast<const wchar_t*>(s), c, n));
}
template <class = void>
inline size_t wcslen(const wchar_t* s) noexcept {
  return ::ycxx::detail::c_str::len(s);
}
template <class = void>
inline wchar_t* wmemset(wchar_t* s, wchar_t c, size_t n) noexcept {
  for (size_t i = 0; i != n; ++i)
    s[i] = c;
  return s;
}

} // namespace std
