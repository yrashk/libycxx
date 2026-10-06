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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::c_str {
// The elements as their unsigned type, as the C comparison functions compare them ("unsigned
// char" for the byte functions; wchar_t values compare as wchar_t).
template <class _Cp>
using __cmp_t = std::conditional_t<std::is_same_v<_Cp, char>, unsigned char, _Cp>;

template <class _Cp>
constexpr std::size_t __len(const _Cp* s) noexcept {
  std::size_t n = 0;
  while (s[n] != _Cp())
    ++n;
  return n;
}
template <class _Cp>
constexpr int __cmp(const _Cp* a, const _Cp* b, std::size_t n) noexcept {
  for (std::size_t i = 0; i != n; ++i) {
    const __cmp_t<_Cp> __x = static_cast<__cmp_t<_Cp>>(a[i]), y = static_cast<__cmp_t<_Cp>>(b[i]);
    if (__x != y)
      return __x < y ? -1 : 1;
    if (__x == 0)
      return 0;
  }
  return 0;
}
template <class _Cp>
constexpr int __mem_cmp(const _Cp* a, const _Cp* b, std::size_t n) noexcept {
  for (std::size_t i = 0; i != n; ++i)
    if (a[i] != b[i])
      return static_cast<__cmp_t<_Cp>>(a[i]) < static_cast<__cmp_t<_Cp>>(b[i]) ? -1 : 1;
  return 0;
}
template <class _Cp>
constexpr _Cp* copy(_Cp* d, const _Cp* s, std::size_t n) noexcept {
  std::size_t i = 0;
  for (; i != n && s[i] != _Cp(); ++i)
    d[i] = s[i];
  for (; i != n; ++i) // strncpy pads with null characters
    d[i] = _Cp();
  return d;
}
template <class _Cp>
constexpr _Cp* cat(_Cp* d, const _Cp* s, std::size_t n) noexcept {
  _Cp* e = d + ::__ycxx::__detail::c_str::__len(d);
  std::size_t i = 0;
  for (; i != n && s[i] != _Cp(); ++i)
    e[i] = s[i];
  e[i] = _Cp();
  return d;
}
template <class _Cp>
constexpr const _Cp* __chr(const _Cp* s, _Cp c) noexcept {
  for (;; ++s) {
    if (*s == c)
      return s;
    if (*s == _Cp())
      return nullptr;
  }
}
template <class _Cp>
constexpr const _Cp* __rchr(const _Cp* s, _Cp c) noexcept {
  const _Cp* r = nullptr;
  for (;; ++s) {
    if (*s == c)
      r = s;
    if (*s == _Cp())
      return r;
  }
}
template <class _Cp>
constexpr const _Cp* __mem_chr(const _Cp* s, _Cp c, std::size_t n) noexcept {
  for (std::size_t i = 0; i != n; ++i)
    if (s[i] == c)
      return s + i;
  return nullptr;
}
// The length of the initial segment of s whose characters are (in == true) or are not in set.
template <class _Cp>
constexpr std::size_t span(const _Cp* s, const _Cp* set, bool in) noexcept {
  std::size_t n = 0;
  for (; s[n] != _Cp(); ++n)
    if ((::__ycxx::__detail::c_str::__chr(set, s[n]) != nullptr) != in)
      break;
  return n;
}
template <class _Cp>
constexpr const _Cp* __pbrk(const _Cp* s, const _Cp* set) noexcept {
  s += ::__ycxx::__detail::c_str::span(s, set, false);
  return *s != _Cp() ? s : nullptr;
}
template <class _Cp>
constexpr const _Cp* str(const _Cp* s, const _Cp* t) noexcept {
  const std::size_t m = ::__ycxx::__detail::c_str::__len(t);
  for (;; ++s) {
    if (::__ycxx::__detail::c_str::__mem_cmp(s, t, m) == 0)
      return s;
    if (*s == _Cp())
      return nullptr;
  }
}
template <class _Cp>
constexpr _Cp* __tok(_Cp* s, const _Cp* __sep, _Cp** save) noexcept {
  if (s == nullptr)
    s = *save;
  if (s == nullptr)
    return nullptr;
  s += ::__ycxx::__detail::c_str::span(s, __sep, true);
  if (*s == _Cp()) {
    *save = nullptr;
    return nullptr;
  }
  _Cp* e = s + ::__ycxx::__detail::c_str::span(s, __sep, false);
  if (*e == _Cp()) {
    *save = nullptr;
  } else {
    *e = _Cp();
    *save = e + 1;
  }
  return s;
}
}} // namespace __ycxx::__detail::c_str

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- <cstring> ----
template <class = void>
inline void* memcpy(void* __s1, const void* __s2, size_t n) noexcept {
  return __builtin_memcpy(__s1, __s2, n);
}
template <class = void>
inline void* memmove(void* __s1, const void* __s2, size_t n) noexcept {
  return __builtin_memmove(__s1, __s2, n);
}
template <class = void>
inline void* memset(void* s, int c, size_t n) noexcept {
  return __builtin_memset(s, c, n);
}
template <class = void>
inline int memcmp(const void* __s1, const void* __s2, size_t n) noexcept {
  return __builtin_memcmp(__s1, __s2, n);
}
template <class = void>
inline void* memccpy(void* __s1, const void* __s2, int c, size_t n) noexcept {
  auto* d = static_cast<unsigned char*>(__s1);
  const auto* s = static_cast<const unsigned char*>(__s2);
  for (size_t i = 0; i != n; ++i) {
    d[i] = s[i];
    if (s[i] == static_cast<unsigned char>(c))
      return d + i + 1;
  }
  return nullptr;
}
template <class = void>
inline char* strcpy(char* __s1, const char* __s2) noexcept {
  return static_cast<char*>(__builtin_memcpy(__s1, __s2, ::__ycxx::__detail::c_str::__len(__s2) + 1));
}
template <class = void>
inline char* strncpy(char* __s1, const char* __s2, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::copy(__s1, __s2, n);
}
template <class = void>
inline char* strcat(char* __s1, const char* __s2) noexcept {
  return ::__ycxx::__detail::c_str::cat(__s1, __s2, static_cast<size_t>(-1));
}
template <class = void>
inline char* strncat(char* __s1, const char* __s2, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::cat(__s1, __s2, n);
}
template <class = void>
inline int strcmp(const char* __s1, const char* __s2) noexcept {
  return ::__ycxx::__detail::c_str::__cmp(__s1, __s2, static_cast<size_t>(-1));
}
template <class = void>
inline int strncmp(const char* __s1, const char* __s2, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::__cmp(__s1, __s2, n);
}
template <class = void>
inline const void* memchr(const void* s, int c, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::__mem_chr(static_cast<const unsigned char*>(s), static_cast<unsigned char>(c), n);
}
template <class = void>
inline void* memchr(void* s, int c, size_t n) noexcept {
  return const_cast<unsigned char*>(
      ::__ycxx::__detail::c_str::__mem_chr(static_cast<const unsigned char*>(s), static_cast<unsigned char>(c), n));
}
template <class = void>
inline const char* strchr(const char* s, int c) noexcept {
  return ::__ycxx::__detail::c_str::__chr(s, static_cast<char>(c));
}
template <class = void>
inline char* strchr(char* s, int c) noexcept {
  return const_cast<char*>(::__ycxx::__detail::c_str::__chr(static_cast<const char*>(s), static_cast<char>(c)));
}
template <class = void>
inline size_t strcspn(const char* __s1, const char* __s2) noexcept {
  return ::__ycxx::__detail::c_str::span(__s1, __s2, false);
}
template <class = void>
inline const char* strpbrk(const char* __s1, const char* __s2) noexcept {
  return ::__ycxx::__detail::c_str::__pbrk(__s1, __s2);
}
template <class = void>
inline char* strpbrk(char* __s1, const char* __s2) noexcept {
  return const_cast<char*>(::__ycxx::__detail::c_str::__pbrk(static_cast<const char*>(__s1), __s2));
}
template <class = void>
inline const char* strrchr(const char* s, int c) noexcept {
  return ::__ycxx::__detail::c_str::__rchr(s, static_cast<char>(c));
}
template <class = void>
inline char* strrchr(char* s, int c) noexcept {
  return const_cast<char*>(::__ycxx::__detail::c_str::__rchr(static_cast<const char*>(s), static_cast<char>(c)));
}
template <class = void>
inline size_t strspn(const char* __s1, const char* __s2) noexcept {
  return ::__ycxx::__detail::c_str::span(__s1, __s2, true);
}
template <class = void>
inline const char* strstr(const char* __s1, const char* __s2) noexcept {
  return ::__ycxx::__detail::c_str::str(__s1, __s2);
}
template <class = void>
inline char* strstr(char* __s1, const char* __s2) noexcept {
  return const_cast<char*>(::__ycxx::__detail::c_str::str(static_cast<const char*>(__s1), __s2));
}
template <class = void>
inline size_t strlen(const char* s) noexcept {
  return ::__ycxx::__detail::c_str::__len(s);
}

// ---- <cwchar> ----
template <class = void>
inline wchar_t* wcscpy(wchar_t* __s1, const wchar_t* __s2) noexcept {
  return static_cast<wchar_t*>(__builtin_memcpy(__s1, __s2, (::__ycxx::__detail::c_str::__len(__s2) + 1) * sizeof(wchar_t)));
}
template <class = void>
inline wchar_t* wcsncpy(wchar_t* __s1, const wchar_t* __s2, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::copy(__s1, __s2, n);
}
template <class = void>
inline wchar_t* wmemcpy(wchar_t* __s1, const wchar_t* __s2, size_t n) noexcept {
  return static_cast<wchar_t*>(__builtin_memcpy(__s1, __s2, n * sizeof(wchar_t)));
}
template <class = void>
inline wchar_t* wmemmove(wchar_t* __s1, const wchar_t* __s2, size_t n) noexcept {
  return static_cast<wchar_t*>(__builtin_memmove(__s1, __s2, n * sizeof(wchar_t)));
}
template <class = void>
inline wchar_t* wcscat(wchar_t* __s1, const wchar_t* __s2) noexcept {
  return ::__ycxx::__detail::c_str::cat(__s1, __s2, static_cast<size_t>(-1));
}
template <class = void>
inline wchar_t* wcsncat(wchar_t* __s1, const wchar_t* __s2, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::cat(__s1, __s2, n);
}
template <class = void>
inline int wcscmp(const wchar_t* __s1, const wchar_t* __s2) noexcept {
  return ::__ycxx::__detail::c_str::__cmp(__s1, __s2, static_cast<size_t>(-1));
}
template <class = void>
inline int wcsncmp(const wchar_t* __s1, const wchar_t* __s2, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::__cmp(__s1, __s2, n);
}
template <class = void>
inline int wmemcmp(const wchar_t* __s1, const wchar_t* __s2, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::__mem_cmp(__s1, __s2, n);
}
template <class = void>
inline const wchar_t* wcschr(const wchar_t* s, wchar_t c) noexcept {
  return ::__ycxx::__detail::c_str::__chr(s, c);
}
template <class = void>
inline wchar_t* wcschr(wchar_t* s, wchar_t c) noexcept {
  return const_cast<wchar_t*>(::__ycxx::__detail::c_str::__chr(static_cast<const wchar_t*>(s), c));
}
template <class = void>
inline size_t wcscspn(const wchar_t* __s1, const wchar_t* __s2) noexcept {
  return ::__ycxx::__detail::c_str::span(__s1, __s2, false);
}
template <class = void>
inline const wchar_t* wcspbrk(const wchar_t* __s1, const wchar_t* __s2) noexcept {
  return ::__ycxx::__detail::c_str::__pbrk(__s1, __s2);
}
template <class = void>
inline wchar_t* wcspbrk(wchar_t* __s1, const wchar_t* __s2) noexcept {
  return const_cast<wchar_t*>(::__ycxx::__detail::c_str::__pbrk(static_cast<const wchar_t*>(__s1), __s2));
}
template <class = void>
inline const wchar_t* wcsrchr(const wchar_t* s, wchar_t c) noexcept {
  return ::__ycxx::__detail::c_str::__rchr(s, c);
}
template <class = void>
inline wchar_t* wcsrchr(wchar_t* s, wchar_t c) noexcept {
  return const_cast<wchar_t*>(::__ycxx::__detail::c_str::__rchr(static_cast<const wchar_t*>(s), c));
}
template <class = void>
inline size_t wcsspn(const wchar_t* __s1, const wchar_t* __s2) noexcept {
  return ::__ycxx::__detail::c_str::span(__s1, __s2, true);
}
template <class = void>
inline const wchar_t* wcsstr(const wchar_t* __s1, const wchar_t* __s2) noexcept {
  return ::__ycxx::__detail::c_str::str(__s1, __s2);
}
template <class = void>
inline wchar_t* wcsstr(wchar_t* __s1, const wchar_t* __s2) noexcept {
  return const_cast<wchar_t*>(::__ycxx::__detail::c_str::str(static_cast<const wchar_t*>(__s1), __s2));
}
template <class = void>
inline wchar_t* wcstok(wchar_t* __s1, const wchar_t* __s2, wchar_t** ptr) noexcept {
  return ::__ycxx::__detail::c_str::__tok(__s1, __s2, ptr);
}
template <class = void>
inline const wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n) noexcept {
  return ::__ycxx::__detail::c_str::__mem_chr(s, c, n);
}
template <class = void>
inline wchar_t* wmemchr(wchar_t* s, wchar_t c, size_t n) noexcept {
  return const_cast<wchar_t*>(::__ycxx::__detail::c_str::__mem_chr(static_cast<const wchar_t*>(s), c, n));
}
template <class = void>
inline size_t wcslen(const wchar_t* s) noexcept {
  return ::__ycxx::__detail::c_str::__len(s);
}
template <class = void>
inline wchar_t* wmemset(wchar_t* s, wchar_t c, size_t n) noexcept {
  for (size_t i = 0; i != n; ++i)
    s[i] = c;
  return s;
}

} // namespace std
