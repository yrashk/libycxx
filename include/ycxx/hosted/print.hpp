// libycxx hosted: the print functions of <print> ([print.fun]).
//
// The output is formatted completely before anything is written (into a local buffer that grows
// on the heap only for long results), then written with one fwrite, which holds the stream's lock
// for the whole write. So a format_error leaves the stream untouched, and the buffered and the
// unbuffered (P3107 "nonlocking") variants behave alike. On POSIX systems no terminal needs a
// native Unicode API ([print.fun]/10.1), so vprint_unicode writes the UTF-8 unchanged, as
// vprint_nonunicode does. A failed write throws system_error with the stream's errno.
// The functions are defined in the hosted runtime (src/hosted/print.cpp).
#pragma once

#include <ycxx/core/format_base.hpp>
#include <cstdio>

namespace ycxx::detail {
// Formats into a buffer, appends a newline if `newline`, and writes the result to stream.
void vprint_file(std::FILE* stream, std::string_view fmt, std::format_args args, bool newline);
} // namespace ycxx::detail

namespace std {

void vprint_unicode(FILE* stream, string_view fmt, format_args args);
void vprint_unicode_buffered(FILE* stream, string_view fmt, format_args args);
void vprint_nonunicode(FILE* stream, string_view fmt, format_args args);
void vprint_nonunicode_buffered(FILE* stream, string_view fmt, format_args args);
void vprint_unicode(string_view fmt, format_args args);
void vprint_nonunicode(string_view fmt, format_args args);

template <class... Args>
void print(FILE* stream, format_string<Args...> fmt, Args&&... args) {
  // vprint_unicode, vprint_unicode_buffered, vprint_nonunicode and vprint_nonunicode_buffered
  // write the same bytes here (see above), whatever enable_nonlocking_formatter_optimization says.
  ycxx::detail::vprint_file(stream, fmt.get(), make_format_args(args...), false);
}
template <class... Args>
void print(format_string<Args...> fmt, Args&&... args) {
  ycxx::detail::vprint_file(stdout, fmt.get(), make_format_args(args...), false);
}
template <class... Args>
void println(FILE* stream, format_string<Args...> fmt, Args&&... args) {
  ycxx::detail::vprint_file(stream, fmt.get(), make_format_args(args...), true);
}
template <class... Args>
void println(format_string<Args...> fmt, Args&&... args) {
  ycxx::detail::vprint_file(stdout, fmt.get(), make_format_args(args...), true);
}
void println(FILE* stream);
void println();

} // namespace std
