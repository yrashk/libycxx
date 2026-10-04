// [cwchar.syn]: the freestanding functions wcscpy, wcsncpy, wmemcpy, wmemmove, wcscat,
// wcsncat, wcscmp, wcsncmp, wmemcmp, wcschr, wcscspn, wcspbrk, wcsrchr, wcsspn, wcsstr,
// wcstok, wmemchr, wcslen, wmemset. [library.c]: wcschr, wcspbrk, wcsrchr, wcsstr and wmemchr
// come as const/non-const overload pairs: "const wchar_t* wcschr(const wchar_t* s, wchar_t c);
// wchar_t* wcschr(wchar_t* s, wchar_t c);" (and likewise for the others); behaviour as in C.
#include <cwchar>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

using CP = const wchar_t*;
using P = wchar_t*;
static_assert(std::is_same_v<decltype(std::wcschr(std::declval<CP>(), L'a')), CP>);
static_assert(std::is_same_v<decltype(std::wcschr(std::declval<P>(), L'a')), P>);
static_assert(std::is_same_v<decltype(std::wcspbrk(std::declval<CP>(), std::declval<CP>())), CP>);
static_assert(std::is_same_v<decltype(std::wcspbrk(std::declval<P>(), std::declval<CP>())), P>);
static_assert(std::is_same_v<decltype(std::wcsrchr(std::declval<CP>(), L'a')), CP>);
static_assert(std::is_same_v<decltype(std::wcsrchr(std::declval<P>(), L'a')), P>);
static_assert(std::is_same_v<decltype(std::wcsstr(std::declval<CP>(), std::declval<CP>())), CP>);
static_assert(std::is_same_v<decltype(std::wcsstr(std::declval<P>(), std::declval<CP>())), P>);
static_assert(std::is_same_v<decltype(std::wmemchr(std::declval<CP>(), L'a', 1)), CP>);
static_assert(std::is_same_v<decltype(std::wmemchr(std::declval<P>(), L'a', 1)), P>);
static_assert(std::is_same_v<decltype(std::wcslen(L"")), std::size_t>);

int main() {
  wchar_t buf[16] = {};
  CHECK(std::wcscpy(buf, L"abc") == buf && std::wcslen(buf) == 3);
  CHECK(std::wcscat(buf, L"de") == buf && std::wcscmp(buf, L"abcde") == 0);
  CHECK(std::wcsncat(buf, L"fgh", 1) == buf && std::wcscmp(buf, L"abcdef") == 0);
  CHECK(std::wcsncmp(buf, L"abX", 2) == 0 && std::wcsncmp(buf, L"abX", 3) > 0); // L'c' (0x63) > L'X' (0x58)
  wchar_t n[6];
  CHECK(std::wcsncpy(n, L"xy", 5) == n && n[1] == L'y' && n[2] == 0 && n[4] == 0);  // pads
  CHECK(std::wmemcpy(n, L"pqrst", 5) == n && std::wmemcmp(n, L"pqrst", 5) == 0);
  CHECK(std::wmemmove(n + 1, n, 4) == n + 1 && std::wmemcmp(n, L"ppqrs", 5) == 0);
  CHECK(std::wmemset(n, L'z', 2) == n && n[0] == L'z' && n[1] == L'z' && n[2] == L'q');
  CHECK(std::wmemcmp(L"ab", L"ac", 2) < 0 && std::wmemcmp(L"ab", L"ac", 1) == 0);

  const wchar_t* s = L"hello world";
  CHECK(std::wcschr(s, L'o') == s + 4 && std::wcschr(s, L'q') == nullptr);
  CHECK(std::wcschr(s, L'\0') == s + 11);
  CHECK(std::wcsrchr(s, L'o') == s + 7);
  CHECK(std::wcspbrk(s, L"wr") == s + 6 && std::wcspbrk(s, L"xyz") == nullptr);
  CHECK(std::wcsstr(s, L"wor") == s + 6 && std::wcsstr(s, L"") == s && std::wcsstr(s, L"wx") == nullptr);
  CHECK(std::wmemchr(s, L'w', 11) == s + 6 && std::wmemchr(s, L'w', 6) == nullptr);
  CHECK(std::wcsspn(s, L"leh") == 4 && std::wcscspn(s, L" ") == 5);
  wchar_t* mut = buf;
  CHECK(std::wcschr(mut, L'c') == buf + 2);  // non-const overload

  wchar_t tok[] = L"a,b,,c";
  wchar_t* state = nullptr;
  wchar_t* t1 = std::wcstok(tok, L",", &state);
  wchar_t* t2 = std::wcstok(nullptr, L",", &state);
  wchar_t* t3 = std::wcstok(nullptr, L",", &state);
  wchar_t* t4 = std::wcstok(nullptr, L",", &state);
  CHECK(t1 && std::wcscmp(t1, L"a") == 0 && t2 && std::wcscmp(t2, L"b") == 0);
  CHECK(t3 && std::wcscmp(t3, L"c") == 0 && t4 == nullptr);
  return 0;
}
