// libycxx hosted: the print functions of <print> ([print.fun]).
//
// The output is formatted completely before anything is written (into a local buffer that grows
// on the heap only for long results), then written with one fwrite, which holds the stream's lock
// for the whole write. So a format_error leaves the stream untouched, and the buffered and the
// unbuffered (P3107 "nonlocking") variants behave alike. On POSIX systems no terminal needs a
// native Unicode API ([print.fun]/10.1), so vprint_unicode writes the UTF-8 unchanged, as
// vprint_nonunicode does. A failed write throws system_error with the stream's errno.
// The functions are defined in the hosted runtime (src/hosted/print.cpp).
//
// The forms without a stream write standard output through __ycxx::__detail::__vprint_stdout: C's
// stdout when there is a C library (src/hosted/print.cpp), and otherwise the console hosted
// layer, ycxx_pal_write to ycxx_pal_stdout (src/hosted/print_console.cpp; DECISIONS §18). The
// FILE* forms exist only with a C library, which is what defines FILE.
#pragma once

#include <ycxx/core/format_base.hpp>
#if _YCXX_HOSTED
#  include <cstdio>
#endif

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
#if _YCXX_HOSTED
// Formats into a buffer, appends a newline if `__newline`, and writes the result to stream.
void __vprint_file(std::FILE* stream, std::string_view __fmt, std::format_args __args, bool __newline);
#endif
// The same for standard output.
void __vprint_stdout(std::string_view __fmt, std::format_args __args, bool __newline);

// Standard output is C's stdout or the console layer's (DECISIONS §18). Dependent on Args, so
// that only a program that prints fails without either.
template <class... _Args>
inline constexpr bool __has_stdout = __cfg::__hosted || __cfg::__layer::__console;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

#if _YCXX_HOSTED
void vprint_unicode(FILE* stream, string_view __fmt, format_args __args);
void vprint_unicode_buffered(FILE* stream, string_view __fmt, format_args __args);
void vprint_nonunicode(FILE* stream, string_view __fmt, format_args __args);
void vprint_nonunicode_buffered(FILE* stream, string_view __fmt, format_args __args);
#endif
void vprint_unicode(string_view __fmt, format_args __args);
void vprint_nonunicode(string_view __fmt, format_args __args);

#if _YCXX_HOSTED
template <class... _Args>
void print(FILE* stream, format_string<_Args...> __fmt, _Args&&... __args) {
  // vprint_unicode, vprint_unicode_buffered, vprint_nonunicode and vprint_nonunicode_buffered
  // write the same bytes here (see above), whatever enable_nonlocking_formatter_optimization says.
  __ycxx::__detail::__vprint_file(stream, __fmt.get(), make_format_args(__args...), false);
}
#endif
template <class... _Args>
void print(format_string<_Args...> __fmt, _Args&&... __args) {
  static_assert(__ycxx::__detail::__has_stdout<_Args...>,
                "std::print to standard output needs the 'console' hosted layer (or the C library, "
                "'clib'): libycxx was configured without either (YCXX_HOSTED_LAYERS, DECISIONS §18)");
  __ycxx::__detail::__vprint_stdout(__fmt.get(), make_format_args(__args...), false);
}
#if _YCXX_HOSTED
template <class... _Args>
void println(FILE* stream, format_string<_Args...> __fmt, _Args&&... __args) {
  __ycxx::__detail::__vprint_file(stream, __fmt.get(), make_format_args(__args...), true);
}
#endif
template <class... _Args>
void println(format_string<_Args...> __fmt, _Args&&... __args) {
  static_assert(__ycxx::__detail::__has_stdout<_Args...>,
                "std::println to standard output needs the 'console' hosted layer (or the C library, "
                "'clib'): libycxx was configured without either (YCXX_HOSTED_LAYERS, DECISIONS §18)");
  __ycxx::__detail::__vprint_stdout(__fmt.get(), make_format_args(__args...), true);
}
#if _YCXX_HOSTED
void println(FILE* stream);
#endif
void println();

}} // namespace std
