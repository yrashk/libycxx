// [cwchar.syn], [support.c.headers.other]/1, [library.c]: <wchar.h> and <cwchar> declare
// wcschr, wcspbrk, wcsrchr, wcsstr and wmemchr as const-correct pairs ("const T* f(const T*, ...)"
// and "T* f(T*, ...)"), in the global namespace and in std, and they find what the C functions
// find. Also with <string> included first, and under `using namespace std;`.
#include <string>
#include <wchar.h>
#include <cwchar>
#include <type_traits>

#include "check.hpp"

using namespace std;

int main() {
  const wchar_t* cs = L"hello, world";
  wchar_t buf[] = L"hello, world";
  wchar_t* s = buf;

  static_assert(is_same_v<decltype(::wcschr(cs, L'o')), const wchar_t*>);
  static_assert(is_same_v<decltype(::wcschr(s, L'o')), wchar_t*>);
  static_assert(is_same_v<decltype(wcschr(cs, L'o')), const wchar_t*>);
  static_assert(is_same_v<decltype(std::wcspbrk(cs, L",")), const wchar_t*>);
  static_assert(is_same_v<decltype(wcspbrk(s, L",")), wchar_t*>);
  static_assert(is_same_v<decltype(::wcsrchr(cs, L'o')), const wchar_t*>);
  static_assert(is_same_v<decltype(::wcsstr(cs, L"wo")), const wchar_t*>);
  static_assert(is_same_v<decltype(wcsstr(s, L"wo")), wchar_t*>);
  static_assert(is_same_v<decltype(::wmemchr(cs, L'w', 12)), const wchar_t*>);
  static_assert(is_same_v<decltype(std::wmemchr(s, L'w', 12)), wchar_t*>);

  CHECK(::wcschr(cs, L'o') == cs + 4);
  CHECK(wcschr(s, L'o') == s + 4);
  CHECK(std::wcschr(cs, L'z') == nullptr);
  CHECK(::wcschr(cs, L'\0') == cs + 12);
  CHECK(::wcspbrk(cs, L" ,") == cs + 5);
  CHECK(std::wcspbrk(s, L"xyz") == nullptr);
  CHECK(::wcsrchr(cs, L'o') == cs + 8);
  CHECK(wcsrchr(s, L'h') == s);
  CHECK(::wcsstr(cs, L"world") == cs + 7);
  CHECK(std::wcsstr(s, L"") == s);
  CHECK(wcsstr(cs, L"worlds") == nullptr);
  CHECK(::wmemchr(cs, L'w', 12) == cs + 7);
  CHECK(std::wmemchr(s, L'w', 7) == nullptr);
  *wcschr(s, L'h') = L'j';
  CHECK(buf[0] == L'j');
  return 0;
}
