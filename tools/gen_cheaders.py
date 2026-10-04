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
                "aligned_alloc atof atoi atol atoll strtod strfromd strfromf strfroml strtof strtold strtol strtoll "
                "strtoul strtoull mblen mbtowc wctomb mbstowcs wcstombs bsearch qsort rand srand",
                """// free_sized, free_aligned_sized (C23 7.24.3.4-5): free with the allocation's size (and
// alignment), which this C library does not take. Templates, as <cmath>'s functions are: a later C
// library's ::free_sized then wins unqualified calls under `using namespace std;`.
template <class = void>
inline void free_sized(void* ptr, size_t) noexcept {
  ::free(ptr);
}
template <class = void>
inline void free_aligned_sized(void* ptr, size_t, size_t) noexcept {
  ::free(ptr);
}"""),
    "cstring": ("string.h", "memcpy memccpy memmove strcpy strncpy strdup strndup strcat strncat memcmp strcmp "
                "strcoll strncmp strxfrm memchr strchr strcspn strpbrk strrchr strspn strstr strtok memset strerror "
                "strlen", ""),
    "cstdio": ("stdio.h", "FILE fpos_t remove rename tmpfile tmpnam fclose fflush fopen freopen setbuf setvbuf "
               "fprintf fscanf printf scanf snprintf sprintf sscanf vfprintf vfscanf vprintf vscanf vsnprintf "
               "vsprintf vsscanf fgetc fgets fputc fputs getc getchar putc putchar puts ungetc fread fwrite "
               "fgetpos fseek fsetpos ftell rewind clearerr feof ferror perror", ""),
    "cctype": ("ctype.h", "isalnum isalpha isblank iscntrl isdigit isgraph islower isprint ispunct isspace "
               "isupper isxdigit tolower toupper", ""),
    "cwctype": ("wctype.h", "wctrans_t wctype_t iswalnum iswalpha iswblank iswcntrl iswdigit iswgraph "
                "iswlower iswprint iswpunct iswspace iswupper iswxdigit iswctype towctrans towlower towupper "
                "wctrans wctype", ""),
    "cwchar": ("wchar.h", "tm fwprintf fwscanf swprintf swscanf vfwprintf vfwscanf vswprintf vswscanf vwprintf vwscanf wprintf wscanf fgetwc fgetws fputwc fputws fwide getwc getwchar putwc putwchar ungetwc wcstod wcstof wcstold wcstol wcstoll wcstoul wcstoull wcscpy wcsncpy wmemcpy wmemmove wcscat wcsncat wcscmp wcscoll wcsncmp wcsxfrm wmemcmp wcscspn wcsspn wcstok wcslen wmemset wcsftime btowc wctob mbsinit mbrlen mbrtowc wcrtomb mbsrtowcs wcsrtombs", MBSTATE_CHECK + """
// Additions, all templates (template <class = void>): a non-template C function then wins every
// tie, so unqualified calls under `using namespace std;` stay unambiguous (null state pointers
// too), while qualified std:: calls find these where the C function does not fit.
// - Overloads for std::mbstate_t (core's freestanding type, DECISIONS §3), forwarding to the C
//   functions.
template <class = void>
inline int mbsinit(const mbstate_t* ps) noexcept { return ::mbsinit(reinterpret_cast<const ::mbstate_t*>(ps)); }
template <class = void>
inline size_t mbrlen(const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrlen(s, n, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbrtowc(wchar_t* pwc, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtowc(pwc, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t wcrtomb(char* s, wchar_t wc, mbstate_t* ps) noexcept {
  return ::wcrtomb(s, wc, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbsrtowcs(wchar_t* dst, const char** src, size_t len, mbstate_t* ps) noexcept {
  return ::mbsrtowcs(dst, src, len, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t wcsrtombs(char* dst, const wchar_t** src, size_t len, mbstate_t* ps) noexcept {
  return ::wcsrtombs(dst, src, len, reinterpret_cast<::mbstate_t*>(ps));
}

// - [library.c]: the const-correct pairs, for C libraries that declare only
//   `wchar_t* f(const wchar_t*, ...)`.
template <class = void>
inline const wchar_t* wcschr(const wchar_t* s, wchar_t c) noexcept { return ::wcschr(s, c); }
template <class = void>
inline wchar_t* wcschr(wchar_t* s, wchar_t c) noexcept { return const_cast<wchar_t*>(::wcschr(s, c)); }
template <class = void>
inline const wchar_t* wcspbrk(const wchar_t* s1, const wchar_t* s2) noexcept { return ::wcspbrk(s1, s2); }
template <class = void>
inline wchar_t* wcspbrk(wchar_t* s1, const wchar_t* s2) noexcept { return const_cast<wchar_t*>(::wcspbrk(s1, s2)); }
template <class = void>
inline const wchar_t* wcsrchr(const wchar_t* s, wchar_t c) noexcept { return ::wcsrchr(s, c); }
template <class = void>
inline wchar_t* wcsrchr(wchar_t* s, wchar_t c) noexcept { return const_cast<wchar_t*>(::wcsrchr(s, c)); }
template <class = void>
inline const wchar_t* wcsstr(const wchar_t* s1, const wchar_t* s2) noexcept { return ::wcsstr(s1, s2); }
template <class = void>
inline wchar_t* wcsstr(wchar_t* s1, const wchar_t* s2) noexcept { return const_cast<wchar_t*>(::wcsstr(s1, s2)); }
template <class = void>
inline const wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n) noexcept { return ::wmemchr(s, c, n); }
template <class = void>
inline wchar_t* wmemchr(wchar_t* s, wchar_t c, size_t n) noexcept { return const_cast<wchar_t*>(::wmemchr(s, c, n)); }"""),
    "cerrno": ("errno.h", "", ""),
    "csignal": ("signal.h", "sig_atomic_t signal raise", ""),
    "ctime": ("time.h", "clock_t time_t tm timespec clock difftime mktime time timespec_get asctime ctime "
              "gmtime localtime strftime", ""),
    "clocale": ("locale.h", "lconv setlocale localeconv", ""),
    "cinttypes": ("inttypes.h", "imaxdiv_t imaxabs imaxdiv strtoimax strtoumax wcstoimax wcstoumax", ""),
    "csetjmp": ("setjmp.h", "jmp_buf longjmp", ""),
    "cfenv": ("fenv.h", "fenv_t fexcept_t feclearexcept fegetexceptflag feraiseexcept fesetexceptflag "
              "fetestexcept fegetround fesetround fegetenv feholdexcept fesetenv feupdateenv", ""),
    "cuchar": ("uchar.h", "mbrtoc8 c8rtomb mbrtoc16 c16rtomb mbrtoc32 c32rtomb", MBSTATE_CHECK + """
// Overloads for std::mbstate_t, as templates; see <cwchar>.
template <class = void>
inline size_t mbrtoc8(char8_t* pc8, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtoc8(pc8, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t c8rtomb(char* s, char8_t c8, mbstate_t* ps) noexcept {
  return ::c8rtomb(s, c8, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbrtoc16(char16_t* pc16, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtoc16(pc16, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t c16rtomb(char* s, char16_t c16, mbstate_t* ps) noexcept {
  return ::c16rtomb(s, c16, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t mbrtoc32(char32_t* pc32, const char* s, size_t n, mbstate_t* ps) noexcept {
  return ::mbrtoc32(pc32, s, n, reinterpret_cast<::mbstate_t*>(ps));
}
template <class = void>
inline size_t c32rtomb(char* s, char32_t c32, mbstate_t* ps) noexcept {
  return ::c32rtomb(s, c32, reinterpret_cast<::mbstate_t*>(ps));
}"""),
}
# Headers with a freestanding subset ([compliance]): without a C library (YCXX_HOSTED 0) they
# include the core header given here instead of the C library's, and declare none of the names
# above. COMMON holds what both modes share.
FREESTANDING = {"cstdlib": "<ycxx/core/c_stdlib.hpp>", "cstring": "<ycxx/core/c_string.hpp>",
                "cwchar": "<ycxx/core/c_string.hpp>", "cerrno": "<ycxx/core/cerrno_macros.hpp>"}
COMMON = {
    "cstdlib": """// abs, labs, llabs ([c.math.abs]): constexpr, shared with <cmath> (ycxx/core/math_abs.hpp).
// div, ldiv, lldiv are constexpr ([cstdlib.syn]), so they are not the C library's. Templates, as
// abs is: under `using namespace std;` an unqualified call prefers the C library's ::div.
template <class = void>
constexpr div_t div(int numer, int denom) noexcept {
  div_t r{};
  r.quot = numer / denom;
  r.rem = numer % denom;
  return r;
}
template <class = void>
constexpr ldiv_t div(long numer, long denom) noexcept {
  ldiv_t r{};
  r.quot = numer / denom;
  r.rem = numer % denom;
  return r;
}
template <class = void>
constexpr lldiv_t div(long long numer, long long denom) noexcept {
  lldiv_t r{};
  r.quot = numer / denom;
  r.rem = numer % denom;
  return r;
}
template <class = void>
constexpr ldiv_t ldiv(long numer, long denom) noexcept {
  return std::div<>(numer, denom);
}
template <class = void>
constexpr lldiv_t lldiv(long long numer, long long denom) noexcept {
  return std::div<>(numer, denom);
}
// memalignment (C23 7.24.3.1): the largest power of two dividing the address; 0 for a null pointer.
inline size_t memalignment(const void* p) noexcept {
  auto v = reinterpret_cast<__UINTPTR_TYPE__>(p);
  return static_cast<size_t>(v & (~v + 1));
}""",
    "cstring": """// memset_explicit (C23 7.26.6.2): memset whose stores are kept even when the object is never read
// again: the empty asm statement claims to read all memory through s. A template, as <cmath>'s
// functions are, so a C library's ::memset_explicit wins unqualified calls.
template <class = void>
inline void* memset_explicit(void* s, int c, size_t n) noexcept {
  __builtin_memset(s, c, n);
  asm volatile("" : : "r"(s) : "memory");
  return s;
}""",
}
# Standard-mandated macros the C library header may lack (each synopsis; WCHAR_WIDTH comes from
# core's cstdint.hpp, the version macros of <cfloat>/<cstdint>/<cstdarg> from core too).
def version_macro(header):
    m = f"__STDC_VERSION_{header}_H__"
    return [f"#ifndef {m}", f"#  define {m} 202311L", "#endif", ""]
MACROS = {"cwchar": version_macro("WCHAR") + [
              "#if !YCXX_HOSTED", "#  define WEOF (static_cast<__WINT_TYPE__>(-1))", "#endif", ""],
          "cuchar": version_macro("UCHAR"), "cstring": version_macro("STRING"),
          "cstdio": version_macro("STDIO"), "ctime": version_macro("TIME"),
          "cinttypes": version_macro("INTTYPES"), "csetjmp": version_macro("SETJMP")}
EXTRA_INCLUDES = {"cstdlib": ["<ycxx/core/math_abs.hpp>"], "cinttypes": ["<cstdint>"], "cwchar": ["<ycxx/core/char_traits.hpp>", "<ycxx/core/cstdint.hpp>"],
                  "cuchar": ["<ycxx/core/char_traits.hpp>"], "cwctype": ["<ycxx/core/char_traits.hpp>"]}

root = pathlib.Path(__file__).resolve().parent.parent / "include"
for name, (cheader, names, extra) in HEADERS.items():
    fs = FREESTANDING.get(name)
    kind = "hosted; freestanding subset without the C library" if fs else "hosted"
    lines = [f"// -*- C++ -*-  libycxx: <{name}>   [{kind}]  (generated by tools/gen_cheaders.py)",
             "#pragma once", "", "#include <ycxx/config.hpp>", "#include <ycxx/core/version.hpp>",
             "#include <ycxx/core/cstddef.hpp>"]
    lines += [f"#include {h}" for h in EXTRA_INCLUDES.get(name, [])]
    if fs:
        lines += ["#if YCXX_HOSTED", f"#  include <{cheader}>", "#else", f"#  include {fs}", "#endif", ""]
    else:
        lines += [f"#include <{cheader}>", ""]
    lines += MACROS.get(name, [])
    if names or extra:
        if fs:
            lines.append("#if YCXX_HOSTED")
        lines.append("namespace std {")
        lines += [f"using ::{n};" for n in names.split()]
        if extra:
            lines += ["", extra]
        lines.append("} // namespace std")
        if fs:
            lines.append("#endif")
    if name in COMMON:
        lines += ["", "namespace std {", COMMON[name], "} // namespace std"]
    (root / name).write_text("\n".join(lines) + "\n")
print("generated", len(HEADERS), "headers")
