#!/usr/bin/env python3
"""Generate libycxx's hosted C-library wrapper headers (<cstdlib>, <cstring>, ...).

Each wrapper includes the C library's header and brings the C names into namespace std with
using-declarations. The data table below is the single source of truth; re-run after editing.
Types that libycxx already defines in std (size_t, ...) are not re-declared.
"""
import pathlib

# std::mbstate_t and std::wint_t come from core (char_traits.hpp); the C library's ::mbstate_t is
# a different type with the same layout, checked here.
MBSTATE_CHECK = """static_assert(sizeof(mbstate_t) == sizeof(::mbstate_t) && alignof(mbstate_t) == alignof(::mbstate_t),
              "libycxx: std::mbstate_t does not match this C library's ::mbstate_t layout");
"""

HEADERS = {
    "cstdlib": ("stdlib.h", "div_t ldiv_t lldiv_t "
                "abort atexit at_quick_exit _Exit exit quick_exit getenv system malloc calloc realloc free "
                "aligned_alloc atof atoi atol atoll strtod strtof strtold strtol strtoll strtoul strtoull "
                "mblen mbtowc wctomb mbstowcs wcstombs bsearch qsort rand srand abs labs llabs div ldiv lldiv",
                """inline long abs(long x) noexcept { return x < 0 ? -x : x; }
inline long long abs(long long x) noexcept { return x < 0 ? -x : x; }
inline ldiv_t div(long a, long b) noexcept { return ::ldiv(a, b); }
inline lldiv_t div(long long a, long long b) noexcept { return ::lldiv(a, b); }
inline float abs(float x) noexcept { return __builtin_fabsf(x); }
inline double abs(double x) noexcept { return __builtin_fabs(x); }
inline long double abs(long double x) noexcept { return __builtin_fabsl(x); }"""),
    "cstring": ("string.h", "memcpy memmove strcpy strncpy strcat strncat memcmp strcmp strcoll strncmp strxfrm "
                "memchr strchr strcspn strpbrk strrchr strspn strstr strtok memset strerror strlen", ""),
    "cstdio": ("stdio.h", "FILE fpos_t remove rename tmpfile tmpnam fclose fflush fopen freopen setbuf setvbuf "
               "fprintf fscanf printf scanf snprintf sprintf sscanf vfprintf vfscanf vprintf vscanf vsnprintf "
               "vsprintf vsscanf fgetc fgets fputc fputs getc getchar putc putchar puts ungetc fread fwrite "
               "fgetpos fseek fsetpos ftell rewind clearerr feof ferror perror", ""),
    "cctype": ("ctype.h", "isalnum isalpha isblank iscntrl isdigit isgraph islower isprint ispunct isspace "
               "isupper isxdigit tolower toupper", ""),
    "cwctype": ("wctype.h", "wctrans_t wctype_t iswalnum iswalpha iswblank iswcntrl iswdigit iswgraph "
                "iswlower iswprint iswpunct iswspace iswupper iswxdigit iswctype towctrans towlower towupper "
                "wctrans wctype", ""),
    "cwchar": ("wchar.h", "tm fwprintf fwscanf swprintf swscanf vfwprintf vfwscanf vswprintf vswscanf vwprintf vwscanf wprintf wscanf fgetwc fgetws fputwc fputws fwide getwc getwchar putwc putwchar ungetwc wcstod wcstof wcstold wcstol wcstoll wcstoul wcstoull wcscpy wcsncpy wmemcpy wmemmove wcscat wcsncat wcscmp wcscoll wcsncmp wcsxfrm wmemcmp wcscspn wcsspn wcstok wcslen wmemset wcsftime btowc wctob", MBSTATE_CHECK + """
// The conversion functions taking a state. The overloads for std::mbstate_t (core's freestanding
// type, DECISIONS §3) forward to the C functions; those for the C library's ::mbstate_t are
// templates, so that a null state pointer (nullptr, 0) prefers the non-template overload instead
// of being ambiguous.
inline int mbsinit(const mbstate_t* ps) noexcept { return ::mbsinit(reinterpret_cast<const ::mbstate_t*>(ps)); }
template <class = void>
inline int mbsinit(const ::mbstate_t* ps) noexcept { return ::mbsinit(ps); }
inline size_t mbrlen(const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrlen(s, n, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbrlen(const char* s, size_t n, ::mbstate_t* ps) noexcept { return ::mbrlen(s, n, ps); }
inline size_t mbrtowc(wchar_t* pwc, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtowc(pwc, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbrtowc(wchar_t* pwc, const char* s, size_t n, ::mbstate_t* ps) noexcept { return ::mbrtowc(pwc, s, n, ps); }
inline size_t wcrtomb(char* s, wchar_t wc, mbstate_t* ps) noexcept {
  return ::wcrtomb(s, wc, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t wcrtomb(char* s, wchar_t wc, ::mbstate_t* ps) noexcept { return ::wcrtomb(s, wc, ps); }
inline size_t mbsrtowcs(wchar_t* dst, const char** src, size_t len, mbstate_t* ps) noexcept {
  return ::mbsrtowcs(dst, src, len, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbsrtowcs(wchar_t* dst, const char** src, size_t len, ::mbstate_t* ps) noexcept {
  return ::mbsrtowcs(dst, src, len, ps);
}
inline size_t wcsrtombs(char* dst, const wchar_t** src, size_t len, mbstate_t* ps) noexcept {
  return ::wcsrtombs(dst, src, len, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t wcsrtombs(char* dst, const wchar_t** src, size_t len, ::mbstate_t* ps) noexcept {
  return ::wcsrtombs(dst, src, len, ps);
}

// [library.c]: the const-correct overload pairs.
inline const wchar_t* wcschr(const wchar_t* s, wchar_t c) noexcept { return ::wcschr(s, c); }
inline wchar_t* wcschr(wchar_t* s, wchar_t c) noexcept { return ::wcschr(s, c); }
inline const wchar_t* wcspbrk(const wchar_t* s1, const wchar_t* s2) noexcept { return ::wcspbrk(s1, s2); }
inline wchar_t* wcspbrk(wchar_t* s1, const wchar_t* s2) noexcept { return ::wcspbrk(s1, s2); }
inline const wchar_t* wcsrchr(const wchar_t* s, wchar_t c) noexcept { return ::wcsrchr(s, c); }
inline wchar_t* wcsrchr(wchar_t* s, wchar_t c) noexcept { return ::wcsrchr(s, c); }
inline const wchar_t* wcsstr(const wchar_t* s1, const wchar_t* s2) noexcept { return ::wcsstr(s1, s2); }
inline wchar_t* wcsstr(wchar_t* s1, const wchar_t* s2) noexcept { return ::wcsstr(s1, s2); }
inline const wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n) noexcept { return ::wmemchr(s, c, n); }
inline wchar_t* wmemchr(wchar_t* s, wchar_t c, size_t n) noexcept { return ::wmemchr(s, c, n); }"""),
    "cerrno": ("errno.h", "", ""),
    "csignal": ("signal.h", "sig_atomic_t signal raise", ""),
    "ctime": ("time.h", "clock_t time_t tm timespec clock difftime mktime time timespec_get asctime ctime "
              "gmtime localtime strftime", ""),
    "clocale": ("locale.h", "lconv setlocale localeconv", ""),
    "cinttypes": ("inttypes.h", "imaxdiv_t imaxabs imaxdiv strtoimax strtoumax wcstoimax wcstoumax", ""),
    "csetjmp": ("setjmp.h", "jmp_buf longjmp", ""),
    "cstdarg": ("stdarg.h", "va_list", ""),
    "cfenv": ("fenv.h", "fenv_t fexcept_t feclearexcept fegetexceptflag feraiseexcept fesetexceptflag "
              "fetestexcept fegetround fesetround fegetenv feholdexcept fesetenv feupdateenv", ""),
    "cuchar": ("uchar.h", "", MBSTATE_CHECK + """
// Overloads for std::mbstate_t and (as templates) ::mbstate_t; see <cwchar>.
inline size_t mbrtoc8(char8_t* pc8, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtoc8(pc8, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
inline size_t c8rtomb(char* s, char8_t c8, mbstate_t* ps) noexcept {
  return ::c8rtomb(s, c8, reinterpret_cast<::mbstate_t*>(ps));
}
inline size_t mbrtoc16(char16_t* pc16, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtoc16(pc16, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
inline size_t c16rtomb(char* s, char16_t c16, mbstate_t* ps) noexcept {
  return ::c16rtomb(s, c16, reinterpret_cast<::mbstate_t*>(ps));
}
inline size_t mbrtoc32(char32_t* pc32, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtoc32(pc32, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
inline size_t c32rtomb(char* s, char32_t c32, mbstate_t* ps) noexcept {
  return ::c32rtomb(s, c32, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbrtoc8(char8_t* pc8, const char* s, size_t n, ::mbstate_t* ps) noexcept { return ::mbrtoc8(pc8, s, n, ps); }
template <class = void>
inline size_t c8rtomb(char* s, char8_t c8, ::mbstate_t* ps) noexcept { return ::c8rtomb(s, c8, ps); }
template <class = void>
inline size_t mbrtoc16(char16_t* pc16, const char* s, size_t n, ::mbstate_t* ps) noexcept { return ::mbrtoc16(pc16, s, n, ps); }
template <class = void>
inline size_t c16rtomb(char* s, char16_t c16, ::mbstate_t* ps) noexcept { return ::c16rtomb(s, c16, ps); }
template <class = void>
inline size_t mbrtoc32(char32_t* pc32, const char* s, size_t n, ::mbstate_t* ps) noexcept { return ::mbrtoc32(pc32, s, n, ps); }
template <class = void>
inline size_t c32rtomb(char* s, char32_t c32, ::mbstate_t* ps) noexcept { return ::c32rtomb(s, c32, ps); }"""),
}
# Standard-mandated macros the C library header may lack ([cwchar.syn]; WCHAR_WIDTH comes from
# core's cstdint.hpp).
MACROS = {"cwchar": ["#ifndef __STDC_VERSION_WCHAR_H__", "#  define __STDC_VERSION_WCHAR_H__ 202311L", "#endif", ""]}
EXTRA_INCLUDES = {"cinttypes": ["<cstdint>"], "cwchar": ["<ycxx/core/char_traits.hpp>", "<ycxx/core/cstdint.hpp>"],
                  "cuchar": ["<ycxx/core/char_traits.hpp>"], "cwctype": ["<ycxx/core/char_traits.hpp>"]}

root = pathlib.Path(__file__).resolve().parent.parent / "include"
for name, (cheader, names, extra) in HEADERS.items():
    lines = [f"// -*- C++ -*-  libycxx: <{name}>   [hosted]  (generated by tools/gen_cheaders.py)",
             "#pragma once", "", "#include <ycxx/config.hpp>", "#include <ycxx/core/version.hpp>",
             "#include <ycxx/core/cstddef.hpp>"]
    lines += [f"#include {h}" for h in EXTRA_INCLUDES.get(name, [])]
    lines += [f"#include <{cheader}>", ""]
    lines += MACROS.get(name, [])
    if names or extra:
        lines.append("namespace std {")
        lines += [f"using ::{n};" for n in names.split()]
        if extra:
            lines += ["", extra]
        lines.append("} // namespace std")
    (root / name).write_text("\n".join(lines) + "\n")
print("generated", len(HEADERS), "headers")
