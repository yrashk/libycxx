// libycxx hosted: print, println, vprint_unicode and vprint_nonunicode for ostream
// ([ostream.formatted.print]). The non-template parts are in the hosted runtime
// (src/hosted/print.cpp). As for FILE* streams, vprint_unicode writes UTF-8 unchanged: no
// POSIX terminal needs a native Unicode API.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/hosted/ostream.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// [ostream.formatted.print]/4: a formatted output function writing vformat(os.getloc(), fmt,
// args), followed by a newline if `newline`.
void vprint_ostream(std::ostream& os, std::string_view fmt, std::format_args args, bool newline);
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

void vprint_unicode(ostream& os, string_view fmt, format_args args);
void vprint_nonunicode(ostream& os, string_view fmt, format_args args);

template <class... Args>
void print(ostream& os, format_string<Args...> fmt, Args&&... args) {
  ycxx::detail::vprint_ostream(os, fmt.get(), make_format_args(args...), false);
}
template <class... Args>
void println(ostream& os, format_string<Args...> fmt, Args&&... args) {
  ycxx::detail::vprint_ostream(os, fmt.get(), make_format_args(args...), true);
}
void println(ostream& os);

} // namespace std
