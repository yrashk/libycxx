// [support.c.headers.other]/1: <wchar.h> places in the global namespace each name <cwchar>
// places in std. [cwchar.syn] and [library.c] make wcschr, wcspbrk, wcsrchr, wcsstr and wmemchr
// const-correct overload pairs ("const wchar_t* wcschr(const wchar_t* s, wchar_t c); wchar_t*
// wcschr(wchar_t* s, wchar_t c);"), so the global names are overloaded the same way.
// Only <wchar.h> is included (mbstate_t: cwchar/mbstate_global).
#include <wchar.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::btowc;
using ::fgetwc;
using ::fgetws;
using ::fputwc;
using ::fputws;
using ::fwide;
using ::fwprintf;
using ::fwscanf;
using ::getwc;
using ::getwchar;
using ::mbrlen;
using ::mbrtowc;
using ::mbsinit;
using ::mbsrtowcs;
using ::mbstate_t;
using ::putwc;
using ::putwchar;
using ::size_t;
using ::swprintf;
using ::swscanf;
using ::tm;
using ::ungetwc;
using ::vfwprintf;
using ::vfwscanf;
using ::vswprintf;
using ::vswscanf;
using ::vwprintf;
using ::vwscanf;
using ::wcrtomb;
using ::wcscat;
using ::wcscmp;
using ::wcscoll;
using ::wcscpy;
using ::wcscspn;
using ::wcsftime;
using ::wcslen;
using ::wcsncat;
using ::wcsncmp;
using ::wcsncpy;
using ::wcsrtombs;
using ::wcsspn;
using ::wcstod;
using ::wcstof;
using ::wcstok;
using ::wcstol;
using ::wcstold;
using ::wcstoll;
using ::wcstoul;
using ::wcstoull;
using ::wcsxfrm;
using ::wctob;
using ::wint_t;
using ::wmemcmp;
using ::wmemcpy;
using ::wmemmove;
using ::wmemset;
using ::wprintf;
using ::wscanf;

#ifndef WEOF
#error WEOF
#endif
static_assert(WCHAR_MIN <= 0 && WCHAR_MAX > 0);

const wchar_t* cs = L"";
wchar_t* s = nullptr;
static_assert(same<decltype(::wcschr(cs, L'a')), const wchar_t*>);
static_assert(same<decltype(::wcschr(s, L'a')), wchar_t*>);
static_assert(same<decltype(::wcspbrk(cs, L"")), const wchar_t*>);
static_assert(same<decltype(::wcspbrk(s, L"")), wchar_t*>);
static_assert(same<decltype(::wcsrchr(cs, L'a')), const wchar_t*>);
static_assert(same<decltype(::wcsrchr(s, L'a')), wchar_t*>);
static_assert(same<decltype(::wcsstr(cs, L"")), const wchar_t*>);
static_assert(same<decltype(::wcsstr(s, L"")), wchar_t*>);
static_assert(same<decltype(::wmemchr(cs, L'a', 0)), const wchar_t*>);
static_assert(same<decltype(::wmemchr(s, L'a', 0)), wchar_t*>);

int main() { return 0; }
