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
// The forms without a stream write standard output through ycxx::detail::vprint_stdout: C's
// stdout when there is a C library (src/hosted/print.cpp), and otherwise the console hosted
// layer, ycxx_pal_write to ycxx_pal_stdout (src/hosted/print_console.cpp; DECISIONS §18). The
// FILE* forms exist only with a C library, which is what defines FILE.
#pragma once

#include <ycxx/core/format_base.hpp>
#if YCXX_HOSTED
#  include <cstdio>
#endif

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
#if YCXX_HOSTED
// Formats into a buffer, appends a newline if `newline`, and writes the result to stream.
void vprint_file(std::FILE* stream, std::string_view fmt, std::format_args args, bool newline);
#endif
// The same for standard output.
void vprint_stdout(std::string_view fmt, std::format_args args, bool newline);

// Standard output is C's stdout or the console layer's (DECISIONS §18). Dependent on Args, so
// that only a program that prints fails without either.
template <class... Args>
inline constexpr bool has_stdout = cfg::hosted || cfg::layer::console;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

#if YCXX_HOSTED
void vprint_unicode(FILE* stream, string_view fmt, format_args args);
void vprint_unicode_buffered(FILE* stream, string_view fmt, format_args args);
void vprint_nonunicode(FILE* stream, string_view fmt, format_args args);
void vprint_nonunicode_buffered(FILE* stream, string_view fmt, format_args args);
#endif
void vprint_unicode(string_view fmt, format_args args);
void vprint_nonunicode(string_view fmt, format_args args);

#if YCXX_HOSTED
template <class... Args>
void print(FILE* stream, format_string<Args...> fmt, Args&&... args) {
  // vprint_unicode, vprint_unicode_buffered, vprint_nonunicode and vprint_nonunicode_buffered
  // write the same bytes here (see above), whatever enable_nonlocking_formatter_optimization says.
  ycxx::detail::vprint_file(stream, fmt.get(), make_format_args(args...), false);
}
#endif
template <class... Args>
void print(format_string<Args...> fmt, Args&&... args) {
  static_assert(ycxx::detail::has_stdout<Args...>,
                "std::print to standard output needs the 'console' hosted layer (or the C library, "
                "'clib'): libycxx was configured without either (YCXX_HOSTED_LAYERS, DECISIONS §18)");
  ycxx::detail::vprint_stdout(fmt.get(), make_format_args(args...), false);
}
#if YCXX_HOSTED
template <class... Args>
void println(FILE* stream, format_string<Args...> fmt, Args&&... args) {
  ycxx::detail::vprint_file(stream, fmt.get(), make_format_args(args...), true);
}
#endif
template <class... Args>
void println(format_string<Args...> fmt, Args&&... args) {
  static_assert(ycxx::detail::has_stdout<Args...>,
                "std::println to standard output needs the 'console' hosted layer (or the C library, "
                "'clib'): libycxx was configured without either (YCXX_HOSTED_LAYERS, DECISIONS §18)");
  ycxx::detail::vprint_stdout(fmt.get(), make_format_args(args...), true);
}
#if YCXX_HOSTED
void println(FILE* stream);
#endif
void println();

} // namespace std
