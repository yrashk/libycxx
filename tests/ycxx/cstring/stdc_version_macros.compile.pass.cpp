// The C++ headers for the C23 library headers define the C23 version macros (each synopsis):
//   [cinttypes.syn] #define __STDC_VERSION_INTTYPES_H__ 202311L
//   [cstdio.syn]    #define __STDC_VERSION_STDIO_H__ 202311L
//   [ctime.syn]     #define __STDC_VERSION_TIME_H__ 202311L
//   [cstring.syn]   #define __STDC_VERSION_STRING_H__ 202311L
//   [cuchar.syn]    #define __STDC_VERSION_UCHAR_H__ 202311L
// (<cfloat>, <cstdint>, <cwchar>: see cfloat/macros, cstdint/macros, cwchar/macros.)
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cuchar>

#if !defined(__STDC_VERSION_INTTYPES_H__) || __STDC_VERSION_INTTYPES_H__ != 202311L
#  error "<cinttypes>: __STDC_VERSION_INTTYPES_H__ must be 202311L"
#endif
#if !defined(__STDC_VERSION_STDIO_H__) || __STDC_VERSION_STDIO_H__ != 202311L
#  error "<cstdio>: __STDC_VERSION_STDIO_H__ must be 202311L"
#endif
#if !defined(__STDC_VERSION_TIME_H__) || __STDC_VERSION_TIME_H__ != 202311L
#  error "<ctime>: __STDC_VERSION_TIME_H__ must be 202311L"
#endif
#if !defined(__STDC_VERSION_STRING_H__) || __STDC_VERSION_STRING_H__ != 202311L
#  error "<cstring>: __STDC_VERSION_STRING_H__ must be 202311L"
#endif
#if !defined(__STDC_VERSION_UCHAR_H__) || __STDC_VERSION_UCHAR_H__ != 202311L
#  error "<cuchar>: __STDC_VERSION_UCHAR_H__ must be 202311L"
#endif
