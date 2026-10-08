// libycxx hosted runtime: the print functions of <print> ([print.fun]) and of <ostream>
// ([ostream.formatted.print]). See ycxx/hosted/print.hpp for the design.
#include <print>
#include <ostream>
#include <system_error>
#include <cerrno>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

namespace {

// Writes [p, p + n) to stream with one fwrite (which holds the stream's lock throughout).
void write_file(std::FILE* stream, const char* p, std::size_t n) {
  errno = 0;
  if (std::fwrite(p, 1, n, stream) != n) {
    const int e = errno != 0 ? errno : EIO;
    throw std::system_error(e, std::system_category(), "std::print: writing to the stream failed");
  }
}

void format_to_file(std::FILE* stream, std::string_view __fmt, std::format_args __args, bool __newline) {
  __fmt_dynbuf<char> __buf;
  ::__ycxx::__detail::__fmt_vformat(__buf, __fmt, __args, nullptr);
  if (__newline)
    __buf.push_back('\n');
  write_file(stream, __buf.data(), __buf.size());
}

} // namespace

void __vprint_file(std::FILE* stream, std::string_view __fmt, std::format_args __args, bool __newline) {
  format_to_file(stream, __fmt, __args, __newline);
}

// Standard output is C's stdout ([print.fun]/3: print(fmt, args) is print(stdout, fmt, args)).
// Without a C library, src/hosted/print_console.cpp defines these instead (DECISIONS §18).
void __vprint_stdout(std::string_view __fmt, std::format_args __args, bool __newline) {
  format_to_file(stdout, __fmt, __args, __newline);
}

void __vprint_ostream(std::ostream& __os, std::string_view __fmt, std::format_args __args, bool __newline) {
  std::ios_base::iostate __err = std::ios_base::goodbit;
  if (std::ostream::sentry ok{__os}) {
    // An exception from vformat propagates as is: no badbit, whatever exceptions() says (/4.2).
    bool formatting = true;
    ::__ycxx::__detail::__guarded_io(
        __os,
        [&] {
          std::string out = std::vformat(__os.getloc(), __fmt, __args);
          formatting = false;
          if (__newline)
            out.push_back('\n');
          const std::streamsize n = static_cast<std::streamsize>(out.size());
          if (__os.rdbuf()->sputn(out.data(), n) != n)
            __err |= std::ios_base::badbit;
        },
        formatting);
  }
  if (__err)
    __os.setstate(__err);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

void vprint_unicode(FILE* stream, string_view __fmt, format_args __args) {
  __ycxx::__detail::format_to_file(stream, __fmt, __args, false);
}
void vprint_unicode_buffered(FILE* stream, string_view __fmt, format_args __args) {
  __ycxx::__detail::format_to_file(stream, __fmt, __args, false);
}
void vprint_nonunicode(FILE* stream, string_view __fmt, format_args __args) {
  __ycxx::__detail::format_to_file(stream, __fmt, __args, false);
}
void vprint_nonunicode_buffered(FILE* stream, string_view __fmt, format_args __args) {
  __ycxx::__detail::format_to_file(stream, __fmt, __args, false);
}
void vprint_unicode(string_view __fmt, format_args __args) {
  __ycxx::__detail::format_to_file(stdout, __fmt, __args, false);
}
void vprint_nonunicode(string_view __fmt, format_args __args) {
  __ycxx::__detail::format_to_file(stdout, __fmt, __args, false);
}
void println(FILE* stream) {
  __ycxx::__detail::write_file(stream, "\n", 1);
}
void println() {
  __ycxx::__detail::write_file(stdout, "\n", 1);
}

void vprint_unicode(ostream& __os, string_view __fmt, format_args __args) {
  __ycxx::__detail::__vprint_ostream(__os, __fmt, __args, false);
}
void vprint_nonunicode(ostream& __os, string_view __fmt, format_args __args) {
  __ycxx::__detail::__vprint_ostream(__os, __fmt, __args, false);
}
void println(ostream& __os) {
  __ycxx::__detail::__vprint_ostream(__os, "\n", format_args(make_format_args()), false);
}

}} // namespace std
