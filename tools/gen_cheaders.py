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
                "aligned_alloc atof atoi atol atoll strtod strtof strtold strtol strtoll "
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
    "cuchar": ("uchar.h", "", MBSTATE_CHECK),
}
# Names a C library may lack: brought in with using-declarations under the YCXX_* switch of
# config.hpp that says the C library declares them (followed by the text given for that case),
# and otherwise replaced by libycxx's own.
CONDITIONAL = {"cstdlib": [("YCXX_C_HAS_STRFROM", "strfromd strfromf strfroml", "",
    """// strfromd, strfromf, strfroml (C23 7.24.1.3), which this C library lacks: libycxx's own, on the
// C library's snprintf (ycxx::detail::strfrom, src/hosted/strfrom.cpp). Templates, as free_sized
// is: should the C library gain them, its ::strfromd wins unqualified calls under
// `using namespace std;`.
template <class = void>
inline int strfromd(char* s, size_t n, const char* format, double fp) noexcept {
  return ycxx::detail::strfrom(s, n, format, fp);
}
template <class = void>
inline int strfromf(char* s, size_t n, const char* format, float fp) noexcept {
  return ycxx::detail::strfrom(s, n, format, static_cast<double>(fp));
}
template <class = void>
inline int strfroml(char* s, size_t n, const char* format, long double fp) noexcept {
  return ycxx::detail::strfrom(s, n, format, fp);
}""")],
}
# <cuchar>'s six functions: name, parameters before the state, the arguments they pass on.
UCHAR_FUNCS = [("mbrtoc8", "char8_t* pc8, const char* s, size_t n", "pc8, s, n"),
               ("c8rtomb", "char* s, char8_t c8", "s, c8"),
               ("mbrtoc16", "char16_t* pc16, const char* s, size_t n", "pc16, s, n"),
               ("c16rtomb", "char* s, char16_t c16", "s, c16"),
               ("mbrtoc32", "char32_t* pc32, const char* s, size_t n", "pc32, s, n"),
               ("c32rtomb", "char* s, char32_t c32", "s, c32")]


def uchar_present(names):
    out = ["// Overloads for std::mbstate_t, as templates; see <cwchar>."]
    for n, params, args in UCHAR_FUNCS:
        if n in names:
            out += ["template <class = void>", f"inline size_t {n}({params}, mbstate_t* ps) noexcept {{",
                    f"  return ::{n}({args}, reinterpret_cast<::mbstate_t*>(ps));", "}"]
    return "\n".join(out)


def uchar_fallback(names, what):
    out = [f"// {what}:",
           "// libycxx's own, on the C library's mbrtowc/wcrtomb (src/hosted/uchar.cpp). The forms taking",
           "// ::mbstate_t* are plain functions, as the C library's would be, so a null state pointer",
           "// selects them; those taking std::mbstate_t* are templates, as above."]
    for n, params, args in UCHAR_FUNCS:
        if n in names:
            out += [f"inline size_t {n}({params}, ::mbstate_t* ps) noexcept {{",
                    f"  return ycxx::detail::c_{n}({args}, ps, sizeof(::mbstate_t));", "}"]
    for n, params, args in UCHAR_FUNCS:
        if n in names:
            out += ["template <class = void>", f"inline size_t {n}({params}, mbstate_t* ps) noexcept {{",
                    f"  return ycxx::detail::c_{n}({args}, ps, sizeof(mbstate_t));", "}"]
    return "\n".join(out)


UCHAR16_32 = "mbrtoc16 c16rtomb mbrtoc32 c32rtomb"
CONDITIONAL["cuchar"] = [
    ("YCXX_C_HAS_UCHAR_H", UCHAR16_32, uchar_present(UCHAR16_32.split()),
     uchar_fallback(UCHAR16_32.split(), "mbrtoc16, c16rtomb, mbrtoc32 and c32rtomb (C23 7.30.1), as this C library has no <uchar.h>")),
    ("YCXX_C_HAS_MBRTOC8", "mbrtoc8 c8rtomb", uchar_present(["mbrtoc8", "c8rtomb"]),
     uchar_fallback(["mbrtoc8", "c8rtomb"], "mbrtoc8 and c8rtomb (C23 7.30.1.3-4), which this C library lacks"))]
# A C header that may be missing: included under the switch, else the given replacement.
OPTIONAL_CHEADER = {"cuchar": ("YCXX_C_HAS_UCHAR_H", "wchar.h")}
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
# Global-scope redeclarations, emitted before namespace std.
GLOBAL = {"cstdlib": [
    "#if YCXX_HOSTED",
    "// strfromd/strfromf/strfroml (C23 7.24.1.3) for C libraries without them, in the hosted runtime",
    "// (src/hosted/strfrom.cpp): snprintf with the format checked and rebuilt.",
    "namespace ycxx::detail {",
    "int strfrom(char* s, __SIZE_TYPE__ n, const char* format, double fp) noexcept;",
    "int strfrom(char* s, __SIZE_TYPE__ n, const char* format, long double fp) noexcept;",
    "} // namespace ycxx::detail",
    "#endif",
    ""],
    "cuchar": [
    "// The <cuchar> functions for C libraries without them, in the hosted runtime",
    "// (src/hosted/uchar.cpp). state_size is the size of the mbstate_t object at ps (null: an",
    "// internal one): the C library's mbrtowc/wcrtomb state occupies its start, the code units still",
    "// to deliver or to complete a character its last 16 bytes.",
    "namespace ycxx::detail {"] + [
    f"__SIZE_TYPE__ c_{n}({params.replace('size_t', '__SIZE_TYPE__')}, void* ps, __SIZE_TYPE__ state_size) noexcept;"
    for n, params, _ in UCHAR_FUNCS] + [
    "} // namespace ycxx::detail",
    ""],
    "ctime": [
    "// [depr.ctime] (Annex D; also deprecated in C23): the C library's declarations, redeclared",
    "// [[deprecated]] (decltype keeps their exact type, noexcept included); std:: names them below.",
    '[[deprecated("asctime is deprecated ([depr.ctime]); use strftime or std::format")]] decltype(::asctime) asctime;',
    '[[deprecated("ctime is deprecated ([depr.ctime]); use strftime or std::format")]] decltype(::ctime) ctime;',
    ""]}
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
        opt = OPTIONAL_CHEADER.get(name)
        if opt:
            lines += [f"#if {opt[0]}", f"#  include <{cheader}>", "#else", f"#  include <{opt[1]}>", "#endif", ""]
        else:
            lines += [f"#include <{cheader}>", ""]
    lines += MACROS.get(name, [])
    lines += GLOBAL.get(name, [])
    if names or extra:
        if fs:
            lines.append("#if YCXX_HOSTED")
        lines.append("namespace std {")
        lines += [f"using ::{n};" for n in names.split()]
        for switch, cnames, present, fallback in CONDITIONAL.get(name, []):
            lines.append(f"#if {switch}")
            lines += [f"using ::{n};" for n in cnames.split()]
            if present:
                lines.append(present)
            lines += ["#else", fallback, "#endif"]
        if extra:
            lines += ["", extra]
        lines.append("} // namespace std")
        if fs:
            lines.append("#endif")
    if name in COMMON:
        lines += ["", "namespace std {", COMMON[name], "} // namespace std"]
    (root / name).write_text("\n".join(lines) + "\n")
print("generated", len(HEADERS), "headers")
