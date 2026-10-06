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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Formats into a buffer, appends a newline if `__newline`, and writes the result to stream.
void __vprint_file(std::FILE* stream, std::string_view __fmt, std::format_args __args, bool __newline);
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

void vprint_unicode(FILE* stream, string_view __fmt, format_args __args);
void vprint_unicode_buffered(FILE* stream, string_view __fmt, format_args __args);
void vprint_nonunicode(FILE* stream, string_view __fmt, format_args __args);
void vprint_nonunicode_buffered(FILE* stream, string_view __fmt, format_args __args);
void vprint_unicode(string_view __fmt, format_args __args);
void vprint_nonunicode(string_view __fmt, format_args __args);

template <class... _Args>
void print(FILE* stream, format_string<_Args...> __fmt, _Args&&... __args) {
  // vprint_unicode, vprint_unicode_buffered, vprint_nonunicode and vprint_nonunicode_buffered
  // write the same bytes here (see above), whatever enable_nonlocking_formatter_optimization says.
  __ycxx::__detail::__vprint_file(stream, __fmt.get(), make_format_args(__args...), false);
}
template <class... _Args>
void print(format_string<_Args...> __fmt, _Args&&... __args) {
  __ycxx::__detail::__vprint_file(stdout, __fmt.get(), make_format_args(__args...), false);
}
template <class... _Args>
void println(FILE* stream, format_string<_Args...> __fmt, _Args&&... __args) {
  __ycxx::__detail::__vprint_file(stream, __fmt.get(), make_format_args(__args...), true);
}
template <class... _Args>
void println(format_string<_Args...> __fmt, _Args&&... __args) {
  __ycxx::__detail::__vprint_file(stdout, __fmt.get(), make_format_args(__args...), true);
}
void println(FILE* stream);
void println();

} // namespace std
