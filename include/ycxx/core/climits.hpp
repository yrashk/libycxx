// libycxx core: <climits> macros from compiler-predefined macros only.
#pragma once

#ifndef CHAR_BIT
#  define CHAR_BIT __CHAR_BIT__
#endif
#ifndef SCHAR_MAX
#  define SCHAR_MIN (-__SCHAR_MAX__ - 1)
#  define SCHAR_MAX __SCHAR_MAX__
#  define UCHAR_MAX (__SCHAR_MAX__ * 2 + 1)
#  ifdef __CHAR_UNSIGNED__
#    define CHAR_MIN 0
#    define CHAR_MAX UCHAR_MAX
#  else
#    define CHAR_MIN SCHAR_MIN
#    define CHAR_MAX __SCHAR_MAX__
#  endif
#  define SHRT_MIN (-__SHRT_MAX__ - 1)
#  define SHRT_MAX __SHRT_MAX__
#  define USHRT_MAX (__SHRT_MAX__ * 2 + 1)
#  define INT_MIN (-__INT_MAX__ - 1)
#  define INT_MAX __INT_MAX__
#  define UINT_MAX (__INT_MAX__ * 2U + 1U)
#  define LONG_MIN (-__LONG_MAX__ - 1L)
#  define LONG_MAX __LONG_MAX__
#  define ULONG_MAX (__LONG_MAX__ * 2UL + 1UL)
#  define LLONG_MIN (-__LONG_LONG_MAX__ - 1LL)
#  define LLONG_MAX __LONG_LONG_MAX__
#  define ULLONG_MAX (__LONG_LONG_MAX__ * 2ULL + 1ULL)
#endif
#ifndef MB_LEN_MAX
#  define MB_LEN_MAX 16
#endif
#ifndef BOOL_WIDTH
#  define BOOL_WIDTH 1
#  define CHAR_WIDTH __CHAR_BIT__
#  define SCHAR_WIDTH __SCHAR_WIDTH__
#  define UCHAR_WIDTH __SCHAR_WIDTH__
#  define SHRT_WIDTH __SHRT_WIDTH__
#  define USHRT_WIDTH __SHRT_WIDTH__
#  define INT_WIDTH __INT_WIDTH__
#  define UINT_WIDTH __INT_WIDTH__
#  define LONG_WIDTH __LONG_WIDTH__
#  define ULONG_WIDTH __LONG_WIDTH__
#  define LLONG_WIDTH __LLONG_WIDTH__
#  define ULLONG_WIDTH __LLONG_WIDTH__
#endif
#if defined(__BITINT_MAXWIDTH__) && !defined(BITINT_MAXWIDTH)
#  define BITINT_MAXWIDTH __BITINT_MAXWIDTH__
#endif

