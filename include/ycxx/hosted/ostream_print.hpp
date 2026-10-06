// libycxx hosted: print, println, vprint_unicode and vprint_nonunicode for ostream
// ([ostream.formatted.print]). The non-template parts are in the hosted runtime
// (src/hosted/print.cpp). As for FILE* streams, vprint_unicode writes UTF-8 unchanged: no
// POSIX terminal needs a native Unicode API.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/hosted/ostream.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// [ostream.formatted.print]/4: a formatted output function writing vformat(os.getloc(), fmt,
// args), followed by a newline if `__newline`.
void __vprint_ostream(std::ostream& __os, std::string_view __fmt, std::format_args __args, bool __newline);
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

void vprint_unicode(ostream& __os, string_view __fmt, format_args __args);
void vprint_nonunicode(ostream& __os, string_view __fmt, format_args __args);

template <class... _Args>
void print(ostream& __os, format_string<_Args...> __fmt, _Args&&... __args) {
  __ycxx::__detail::__vprint_ostream(__os, __fmt.get(), make_format_args(__args...), false);
}
template <class... _Args>
void println(ostream& __os, format_string<_Args...> __fmt, _Args&&... __args) {
  __ycxx::__detail::__vprint_ostream(__os, __fmt.get(), make_format_args(__args...), true);
}
void println(ostream& __os);

} // namespace std
