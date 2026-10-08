#!/usr/bin/env python3
"""Generate libycxx's hosted C-library wrapper headers (<cstdlib>, <cstring>, ...).

Each wrapper includes the C library's header and brings the C names into namespace std with
using-declarations. The data table below is the single source of truth; re-run after editing.
Types that libycxx already defines in std (size_t, ...) are not re-declared.
"""
import pathlib
import sys as _sys
_sys.path.insert(0, str(__import__("pathlib").Path(__file__).resolve().parent))
from check_visibility import fix as _visibility_fix
from uglify import uglify_text as _uglify


def hidden(text):
    """libycxx's namespaces get hidden visibility (DECISIONS §2), as tools/check_visibility.py
    requires of every header; the generated ones too."""
    return _visibility_fix(_uglify(text))[0]


HEADERS = {
    "cstdlib": ("stdlib.h", "div_t ldiv_t lldiv_t "
                "abort atexit at_quick_exit _Exit exit quick_exit getenv system malloc calloc realloc free "
                "aligned_alloc atof atoi atol atoll strtod strtof strtold strtol strtoll "
                "strtoul strtoull mblen mbtowc wctomb mbstowcs wcstombs qsort rand srand",
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
                "strcoll strncmp strxfrm strcspn strspn strtok memset strerror strlen",
                """// [cstring.syn]: the const-correct pairs of memchr, strchr, strpbrk, strrchr and strstr
// ([library.c]/2). The C library's declarations are renamed while its header is read (C_RENAMED),
// so these are the only ones on every C library; the compilers' builtins call the C functions.
inline const void* memchr(const void* s, int c, size_t n) noexcept { return __builtin_memchr(s, c, n); }
inline void* memchr(void* s, int c, size_t n) noexcept { return __builtin_memchr(s, c, n); }
inline const char* strchr(const char* s, int c) noexcept { return __builtin_strchr(s, c); }
inline char* strchr(char* s, int c) noexcept { return __builtin_strchr(s, c); }
inline const char* strpbrk(const char* s1, const char* s2) noexcept { return __builtin_strpbrk(s1, s2); }
inline char* strpbrk(char* s1, const char* s2) noexcept { return __builtin_strpbrk(s1, s2); }
inline const char* strrchr(const char* s, int c) noexcept { return __builtin_strrchr(s, c); }
inline char* strrchr(char* s, int c) noexcept { return __builtin_strrchr(s, c); }
inline const char* strstr(const char* s1, const char* s2) noexcept { return __builtin_strstr(s1, s2); }
inline char* strstr(char* s1, const char* s2) noexcept { return __builtin_strstr(s1, s2); }"""),
    "cstdio": ("stdio.h", "FILE fpos_t remove rename tmpfile tmpnam fclose fflush fopen freopen setbuf setvbuf "
               "fprintf fscanf printf scanf snprintf sprintf sscanf vfprintf vfscanf vprintf vscanf vsnprintf "
               "vsprintf vsscanf fgetc fgets fputc fputs getc getchar putc putchar puts ungetc fread fwrite "
               "fgetpos fseek fsetpos ftell rewind clearerr feof ferror perror", ""),
    "cctype": ("ctype.h", "isalnum isalpha isblank iscntrl isdigit isgraph islower isprint ispunct isspace "
               "isupper isxdigit tolower toupper", ""),
    "cwctype": ("wctype.h", "wctrans_t wctype_t iswalnum iswalpha iswblank iswcntrl iswdigit iswgraph "
                "iswlower iswprint iswpunct iswspace iswupper iswxdigit iswctype towctrans towlower towupper "
                "wctrans wctype", ""),
    "cwchar": ("wchar.h", "tm fwprintf fwscanf swprintf swscanf vfwprintf vfwscanf vswprintf vswscanf vwprintf vwscanf wprintf wscanf fgetwc fgetws fputwc fputws fwide getwc getwchar putwc putwchar ungetwc wcstod wcstof wcstold wcstol wcstoll wcstoul wcstoull wcscpy wcsncpy wmemcpy wmemmove wcscat wcsncat wcscmp wcscoll wcsncmp wcsxfrm wmemcmp wcscspn wcsspn wcstok wcslen wmemset wcsftime btowc wctob mbsinit mbrlen mbrtowc wcrtomb mbsrtowcs wcsrtombs", """\
// [library.c]: the const-correct pairs. The C library's own declarations of these five are
// renamed while its header is read (ycxx/hosted/c_wchar.hpp: some declare only
// `wchar_t* f(const wchar_t*, ...)`), and these call its functions through
// ycxx::detail::c_wchar. Templates, as <cstdlib>'s div: a C function of the same name that a
// program declares itself wins ties under `using namespace std;`.
template <class = void>
inline const wchar_t* wcschr(const wchar_t* s, wchar_t c) noexcept { return ::ycxx::detail::c_wchar::wcschr(s, c); }
template <class = void>
inline wchar_t* wcschr(wchar_t* s, wchar_t c) noexcept { return ::ycxx::detail::c_wchar::wcschr(s, c); }
template <class = void>
inline const wchar_t* wcspbrk(const wchar_t* s1, const wchar_t* s2) noexcept { return ::ycxx::detail::c_wchar::wcspbrk(s1, s2); }
template <class = void>
inline wchar_t* wcspbrk(wchar_t* s1, const wchar_t* s2) noexcept { return ::ycxx::detail::c_wchar::wcspbrk(s1, s2); }
template <class = void>
inline const wchar_t* wcsrchr(const wchar_t* s, wchar_t c) noexcept { return ::ycxx::detail::c_wchar::wcsrchr(s, c); }
template <class = void>
inline wchar_t* wcsrchr(wchar_t* s, wchar_t c) noexcept { return ::ycxx::detail::c_wchar::wcsrchr(s, c); }
template <class = void>
inline const wchar_t* wcsstr(const wchar_t* s1, const wchar_t* s2) noexcept { return ::ycxx::detail::c_wchar::wcsstr(s1, s2); }
template <class = void>
inline wchar_t* wcsstr(wchar_t* s1, const wchar_t* s2) noexcept { return ::ycxx::detail::c_wchar::wcsstr(s1, s2); }
template <class = void>
inline const wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n) noexcept { return ::ycxx::detail::c_wchar::wmemchr(s, c, n); }
template <class = void>
inline wchar_t* wmemchr(wchar_t* s, wchar_t c, size_t n) noexcept { return ::ycxx::detail::c_wchar::wmemchr(s, c, n); }"""),
    "cerrno": ("errno.h", "", ""),
    "csignal": ("signal.h", "sig_atomic_t signal raise", ""),
    "ctime": ("time.h", "clock_t time_t tm timespec clock difftime mktime timegm time timespec_get asctime ctime "
              "gmtime gmtime_r localtime localtime_r strftime", ""),
    "clocale": ("locale.h", "lconv setlocale localeconv", ""),
    "cinttypes": ("inttypes.h", "imaxdiv_t strtoimax strtoumax wcstoimax wcstoumax", """// imaxabs, imaxdiv are constexpr ([cinttypes.syn]), so they are not the C library's (as <cstdlib>'s
// div). Templates: under `using namespace std;` an unqualified call prefers the C library's.
// (The optional abs/div overloads for intmax_t exist only when it is an extended integer type.)
template <class = void>
constexpr intmax_t imaxabs(intmax_t j) noexcept {
  return j < 0 ? -j : j;
}
template <class = void>
constexpr imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom) noexcept {
  imaxdiv_t r{};
  r.quot = numer / denom;
  r.rem = numer % denom;
  return r;
}"""),
    "csetjmp": ("setjmp.h", "jmp_buf longjmp", ""),
    "cfenv": ("fenv.h", "fenv_t fexcept_t feclearexcept fegetexceptflag feraiseexcept fesetexceptflag "
              "fetestexcept fegetround fesetround fegetenv feholdexcept fesetenv feupdateenv", ""),
    "cuchar": ("uchar.h", "", ""),
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
CONDITIONAL["ctime"] = [("YCXX_C_HAS_TIMESPEC_GETRES", "timespec_getres", "",
    """// timespec_getres (C23 7.29.2.7), which this C library lacks: libycxx's own (src/hosted/ctime.cpp).
// A template, as strfromd is: should the C library gain it, its ::timespec_getres wins unqualified
// calls under `using namespace std;`.
template <class = void>
inline int timespec_getres(timespec* ts, int base) noexcept {
  return ycxx::detail::c_timespec_getres(ts, base);
}""")]
# <cuchar>'s six functions: name, parameters before the state, the arguments they pass on.
UCHAR_FUNCS = [("mbrtoc8", "char8_t* pc8, const char* s, size_t n", "pc8, s, n"),
               ("c8rtomb", "char* s, char8_t c8", "s, c8"),
               ("mbrtoc16", "char16_t* pc16, const char* s, size_t n", "pc16, s, n"),
               ("c16rtomb", "char* s, char16_t c16", "s, c16"),
               ("mbrtoc32", "char32_t* pc32, const char* s, size_t n", "pc32, s, n"),
               ("c32rtomb", "char* s, char32_t c32", "s, c32")]


def uchar_fallback(names, what):
    out = [f"// {what}:",
           "// libycxx's own, on the C library's mbrtowc/wcrtomb (src/hosted/uchar.cpp)."]
    for n, params, args in UCHAR_FUNCS:
        if n in names:
            out += [f"inline size_t {n}({params}, mbstate_t* ps) noexcept {{",
                    f"  return ycxx::detail::c_{n}({args}, ps, sizeof(mbstate_t));", "}"]
    return "\n".join(out)


UCHAR16_32 = "mbrtoc16 c16rtomb mbrtoc32 c32rtomb"
CONDITIONAL["cuchar"] = [
    ("YCXX_C_HAS_UCHAR_H", UCHAR16_32, "",
     uchar_fallback(UCHAR16_32.split(), "mbrtoc16, c16rtomb, mbrtoc32 and c32rtomb (C23 7.30.1), as this C library has no <uchar.h>")),
    ("YCXX_C_HAS_MBRTOC8", "mbrtoc8 c8rtomb", "",
     uchar_fallback(["mbrtoc8", "c8rtomb"], "mbrtoc8 and c8rtomb (C23 7.30.1.3-4), which this C library lacks"))]
# A C header that may be missing: included under the switch, else the given replacement.
OPTIONAL_CHEADER = {"cuchar": ("YCXX_C_HAS_UCHAR_H", "ycxx/hosted/c_wchar.hpp")}
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
// A template, so that a C library's ::memalignment wins ties instead of conflicting in the global
// namespace (<stdlib.h>).
template <class = void>
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
# C23's binary conversions PRIbN and SCNbN ([cinttypes.syn]/2: defined whenever the typedef is) for
# a C library whose <inttypes.h> predates them: Darwin's. Its types, from its <stdint.h>: int64_t
# is long long, the least and fast types are the exact-width ones, intmax_t and intptr_t are long
# (checked by the static_asserts). PRIBN stay undefined: they are defined only if fprintf
# supports the B conversion, which that C library's does not document.
def inttypes_binary():
    mods = {"8": "hh", "16": "h", "32": "", "64": "ll"}
    lines = ["#if YCXX_TARGET_DARWIN && !defined(PRIb8)", "namespace ycxx::detail {",
             "static_assert(is_any_of<std::uint8_t, unsigned char> && is_any_of<std::uint16_t, unsigned short> &&",
             "              is_any_of<std::uint32_t, unsigned> && is_any_of<std::uint64_t, unsigned long long> &&",
             "              is_any_of<std::uintptr_t, unsigned long>);"]
    lines += [f"static_assert(is_any_of<std::uint_least{n}_t, std::uint{n}_t> && is_any_of<std::uint_fast{n}_t, std::uint{n}_t>);"
              for n in mods]
    lines += ["} // namespace ycxx::detail"]
    for n, m in mods.items():
        for kind in ("", "LEAST", "FAST"):
            lines += [f'#  define PRIb{kind}{n} "{m}b"', f'#  define SCNb{kind}{n} "{m}b"']
    lines += ['#  define PRIbMAX "jb"', '#  define SCNbMAX "jb"', '#  define PRIbPTR "lb"', '#  define SCNbPTR "lb"',
              "#endif", ""]
    return lines
MACROS = {"cwchar": version_macro("WCHAR") + [
              "#if !YCXX_HOSTED", "#  define WEOF (static_cast<__WINT_TYPE__>(-1))", "#endif", ""],
          "cuchar": version_macro("UCHAR"), "cstring": version_macro("STRING"),
          "cstdio": version_macro("STDIO") + [
              "// _PRINTF_NAN_LEN_MAX (C23 7.23.1), for a C library that does not define it: the longest NaN",
              "// output of its printf, measured when libycxx is configured (cmake/ycxx-c-library.cmake).",
              "#if !defined(_PRINTF_NAN_LEN_MAX) && defined(YCXX_C_PRINTF_NAN_LEN_MAX)",
              "#  define _PRINTF_NAN_LEN_MAX YCXX_C_PRINTF_NAN_LEN_MAX", "#endif", ""], "ctime": version_macro("TIME"),
          "cinttypes": version_macro("INTTYPES") + inttypes_binary(), "csetjmp": version_macro("SETJMP") + [
              "// [csetjmp.syn]: setjmp is a macro. C leaves it unspecified whether setjmp is a macro or",
              "// an identifier with external linkage (C23 7.13), and Darwin's <setjmp.h> declares only the",
              "// function: the macro then names it.",
              "#ifndef setjmp", "#  define setjmp(env) setjmp(env)", "#endif", ""]}
# Global-scope redeclarations, emitted before namespace std.
GLOBAL = {"cstdlib": [
    "#if YCXX_HOSTED",
    "// atexit, at_quick_exit: noexcept ([support.start.term]), whether or not the C library's",
    "// declarations say so; theirs are renamed while its header is read (C_RENAMED). One declaration",
    "// serves both handler linkages (the compilers do not distinguish them in function types).",
    "extern \"C\" int atexit(void (*func)(void)) noexcept;",
    "extern \"C\" int at_quick_exit(void (*func)(void)) noexcept;",
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
    "// extern \"C\": <time.h> includes this header inside extern \"C++\".",
    'extern "C" {',
    '[[deprecated("asctime is deprecated ([depr.ctime]); use strftime or std::format")]] decltype(::asctime) asctime;',
    '[[deprecated("ctime is deprecated ([depr.ctime]); use strftime or std::format")]] decltype(::ctime) ctime;',
    '}',
    "",
    "// timespec_getres (C23 7.29.2.7) for C libraries without it, in the hosted runtime",
    "// (src/hosted/ctime.cpp): the resolution of TIME_UTC, from clock_getres(CLOCK_REALTIME).",
    "namespace ycxx::detail {",
    "int c_timespec_getres(::timespec* ts, int base) noexcept;",
    "} // namespace ycxx::detail",
    ""]}
EXTRA_INCLUDES = {"cstdlib": ["<ycxx/core/math_abs.hpp>", "<ycxx/core/c_bsearch.hpp>"], "cinttypes": ["<cstdint>", "<ycxx/core/prim_traits.hpp>"], "cwchar": ["<ycxx/core/char_traits.hpp>", "<ycxx/core/cstdint.hpp>"],
                  "cuchar": ["<ycxx/core/char_traits.hpp>"], "cwctype": ["<ycxx/core/char_traits.hpp>"]}

# The C library's declarations that would defeat the draft's C++ declarations of the same names
# in the global namespace ([support.c.headers.other]/1: <stdlib.h> and <inttypes.h> place
# <cstdlib>'s and <cinttypes>'s names there): a non-template C function `int abs(int)` or
# `div_t div(int, int)` is an exact match that wins over libycxx's constexpr templates, cannot be
# redeclared constexpr, and bsearch's single C signature conflicts with the const-correct pair.
# So they are renamed while the C library's header is read, and never used.
C_RENAMED = {"cstdlib": ["abs", "labs", "llabs", "div", "ldiv", "lldiv", "bsearch", "atexit", "at_quick_exit"],
             "cinttypes": ["imaxabs", "imaxdiv"],
             "cstring": ["memchr", "strchr", "strpbrk", "strrchr", "strstr"],
             "cwchar": ["wcschr", "wcspbrk", "wcsrchr", "wcsstr", "wmemchr"]}
# The C library's <wchar.h> is read by core's char_traits.hpp too (hosted, std::mbstate_t is its
# ::mbstate_t), and must be read with the renames whoever reads it first, so the reading lives
# in a header of its own (generated below) that <cwchar> and <cuchar> include as well.
C_READER = {"cwchar": "<ycxx/hosted/c_wchar.hpp>"}
# The C functions behind the renamed <wchar.h> declarations: name, parameters.
C_WCHAR_FUNCS = [("wcschr", "const wchar_t* s, wchar_t c"), ("wcspbrk", "const wchar_t* s1, const wchar_t* s2"),
                 ("wcsrchr", "const wchar_t* s, wchar_t c"), ("wcsstr", "const wchar_t* s1, const wchar_t* s2"),
                 ("wmemchr", "const wchar_t* s, wchar_t c, __SIZE_TYPE__ n")]
C_RENAMED_COMMENT = [
    "// The C library's declarations of the functions libycxx declares itself (constexpr, noexcept, or",
    "// the const-correct pairs) are renamed while its header is read, so that libycxx's can take the",
    "// names in the global namespace ([support.c.headers.other]/1; DECISIONS §3)."]
# The C headers libycxx wraps (include/<name>.h, generated below): in C++ the wrapper includes the
# <c...> header and adds to the global namespace the names that header declares itself.
H_WRAPPERS = {
    "stdlib.h": ("cstdlib", ["abs", "labs", "llabs", "div", "ldiv", "lldiv", "bsearch", "memalignment"],
                 [("YCXX_HOSTED", ["free_sized", "free_aligned_sized"]),
                  ("YCXX_HOSTED && !YCXX_C_HAS_STRFROM", ["strfromd", "strfromf", "strfroml"])]),
    "inttypes.h": ("cinttypes", ["imaxabs", "imaxdiv"], []),
    "string.h": ("cstring", ["memset_explicit"], []),
    "wchar.h": ("cwchar", [], [("YCXX_HOSTED", C_RENAMED["cwchar"])]),
    "time.h": ("ctime", [], [("YCXX_HOSTED && !YCXX_C_HAS_TIMESPEC_GETRES", ["timespec_getres"])]),
    "uchar.h": ("cuchar", [], [("!YCXX_C_HAS_UCHAR_H", ["mbrtoc16", "c16rtomb", "mbrtoc32", "c32rtomb"]),
                               ("!YCXX_C_HAS_MBRTOC8", ["mbrtoc8", "c8rtomb"])]),
}

root = pathlib.Path(__file__).resolve().parent.parent / "include"

# ycxx/hosted/c_wchar.hpp: the one place that reads the C library's <wchar.h> (see C_READER).
lines = ["// -*- C++ -*-  libycxx: the C library's <wchar.h>, as libycxx reads it   [hosted]  (generated by tools/gen_cheaders.py)",
         "//",
         "// Included by core's char_traits.hpp in hosted builds (std::mbstate_t is the C library's",
         "// ::mbstate_t, [support.c.headers.other]/1; DECISIONS §3), and by <cwchar> and <cuchar>.",
         "// #include_next finds the C library's header past libycxx's include directory, where this file",
         "// was found. Its declarations of the functions <cwchar> declares as const-correct pairs are",
         "// renamed while it is read ([library.c]; some C libraries declare only",
         "// `wchar_t* f(const wchar_t*, ...)`), so that <wchar.h> can place libycxx's pairs in the global",
         "// namespace; whichever libycxx header comes first reads it so. The renamed declarations are never",
         "// used: ycxx::detail::c_wchar declares the C functions again under their assembler names.",
         "#pragma once", "", "#include <ycxx/config.hpp>", ""]
lines += [f"#define {n} ycxx_c_{n}" for n in C_RENAMED["cwchar"]]
lines += ["#include_next <wchar.h>"] + [f"#undef {n}" for n in C_RENAMED["cwchar"]] + [""]
lines += ["// Core's freestanding std::mbstate_t (ycxx/core/mbstate.hpp) has this C library's layout.",
          "static_assert(sizeof(::mbstate_t) == ycxx::detail::cfg::mbstate_size &&",
          "                  alignof(::mbstate_t) == ycxx::detail::cfg::mbstate_align,",
          "              \"libycxx: cfg::mbstate_size/_align do not match this C library's ::mbstate_t\");",
          "",
          "namespace std {", "using ::mbstate_t;", "} // namespace std", "",
          "// An assembler name is the object-file symbol verbatim: Mach-O prefixes C symbols with '_'.",
          "// The functions are the C library's: default visibility (DECISIONS §2).",
          "namespace [[gnu::visibility(\"default\")]] ycxx { namespace detail::c_wchar {", "#if YCXX_TARGET_DARWIN"]
lines += [f"wchar_t* {n}({params}) noexcept __asm__(\"_{n}\");" for n, params in C_WCHAR_FUNCS]
lines += ["#else"]
lines += [f"wchar_t* {n}({params}) noexcept __asm__(\"{n}\");" for n, params in C_WCHAR_FUNCS]
lines += ["#endif", "}} // namespace ycxx::detail::c_wchar", ""]
(root / "ycxx/hosted/c_wchar.hpp").write_text(hidden("\n".join(lines)))
for hname, (cxx, names, cond) in H_WRAPPERS.items():
    lines = [f"// -*- C++ -*-  libycxx: <{hname}> ([support.c.headers.other])   [also usable from C]  (generated by tools/gen_cheaders.py)",
             "//",
             f"// The C library's <{hname}>; in C++ <{cxx}>, which reads the C library's header itself",
             f"// (#include_next), and the names <{cxx}> declares itself (not as the C library's), placed in",
             "// the global namespace ([support.c.headers.other]/1). extern \"C++\": a C header may include this",
             "// one inside an extern \"C\" block.",
             "#pragma once", "", "#ifdef __cplusplus", f"extern \"C++\" {{", f"#  include <{cxx}>", "}"]
    lines += [f"using std::{n};" for n in names]
    for switch, cnames in cond:
        lines += [f"#  if {switch}"] + [f"using std::{n};" for n in cnames] + ["#  endif"]
    lines += ["#else", f"#  include_next <{hname}>", "#endif", ""]
    (root / hname).write_text(hidden("\n".join(lines)))

# <complex.h> and <tgmath.h> "behave as if" they simply include <complex>, and <cmath> and
# <complex> ([complex.h.syn], [tgmath.h.syn]). In C++ the C library's must not be read: its
# `complex` and `I` macros and type-generic macros would break C++ code.
H_CXX_ONLY = {"complex.h": ("complex.h.syn", ["complex"]), "tgmath.h": ("tgmath.h.syn", ["cmath", "complex"])}
for hname, (stable, cxx) in H_CXX_ONLY.items():
    lines = [f"// -*- C++ -*-  libycxx: <{hname}> ([{stable}])   [also usable from C]  (generated by tools/gen_cheaders.py)",
             "//",
             f"// In C++ the header includes {' and '.join(f'<{h}>' for h in cxx)} and nothing else; in C, the C library's <{hname}>.",
             "#pragma once", "", "#ifdef __cplusplus", "extern \"C++\" {"]
    lines += [f"#  include <{h}>" for h in cxx]
    lines += ["}", "#else", f"#  include_next <{hname}>", "#endif", ""]
    (root / hname).write_text(hidden("\n".join(lines)))

for name, (cheader, names, extra) in HEADERS.items():
    fs = FREESTANDING.get(name)
    kind = "hosted; freestanding subset without the C library" if fs else "hosted"
    lines = [f"// -*- C++ -*-  libycxx: <{name}>   [{kind}]  (generated by tools/gen_cheaders.py)",
             "#pragma once", "", "#include <ycxx/config.hpp>", "#include <ycxx/core/version.hpp>",
             "#include <ycxx/core/cstddef.hpp>"]
    lines += [f"#include {h}" for h in EXTRA_INCLUDES.get(name, [])]
    # A header with a .h wrapper of libycxx's own reads the C library's past it (#include_next),
    # with the declarations libycxx replaces renamed (see C_RENAMED).
    if name in C_READER:
        inc = [f"#  include {C_READER[name]}"]
    elif cheader in H_WRAPPERS:
        renamed = C_RENAMED.get(name, [])
        inc = [f"#  include_next <{cheader}>"]
        if renamed:
            inc = C_RENAMED_COMMENT + [f"#  define {n} ycxx_c_{n}" for n in renamed] + inc + [
                f"#  undef {n}" for n in renamed]
    else:
        inc = [f"#  include <{cheader}>"]
    if fs:
        lines += ["#if YCXX_HOSTED"] + inc + ["#else", f"#  include {fs}", "#endif", ""]
    else:
        opt = OPTIONAL_CHEADER.get(name)
        if opt:
            lines += [f"#if {opt[0]}"] + inc + ["#else", f"#  include <{opt[1]}>", "#endif", ""]
        else:
            lines += [i.replace("#  ", "#", 1) for i in inc] + [""]
    lines += MACROS.get(name, [])
    lines += GLOBAL.get(name, [])
    if names or extra or name in CONDITIONAL:
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
    if name in C_RENAMED:
        # The C library's names stay usable in the global namespace, now as libycxx's functions.
        lines += ["", "// The renamed C functions' names in the global namespace, now naming libycxx's own."]
        if fs:
            lines.append("#if YCXX_HOSTED")
        lines += [f"using std::{n};" for n in C_RENAMED[name]]
        if fs:
            lines.append("#endif")
    (root / name).write_text(hidden("\n".join(lines) + "\n"))
print("generated", len(HEADERS), "headers and", len(H_WRAPPERS) + len(H_CXX_ONLY), ".h headers")
