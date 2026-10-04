// libycxx hosted runtime: the print functions of <print> ([print.fun]) and of <ostream>
// ([ostream.formatted.print]). See ycxx/hosted/print.hpp for the design.
#include <print>
#include <ostream>
#include <system_error>
#include <cerrno>

namespace ycxx::detail {

namespace {

// Writes [p, p + n) to stream with one fwrite (which holds the stream's lock throughout).
void write_file(std::FILE* stream, const char* p, std::size_t n) {
  errno = 0;
  if (std::fwrite(p, 1, n, stream) != n) {
    const int e = errno != 0 ? errno : EIO;
    throw std::system_error(e, std::system_category(), "std::print: writing to the stream failed");
  }
}

void format_to_file(std::FILE* stream, std::string_view fmt, std::format_args args, bool newline) {
  fmt_dynbuf<char> buf;
  ::ycxx::detail::fmt_vformat(buf, fmt, args, nullptr);
  if (newline)
    buf.push_back('\n');
  write_file(stream, buf.data(), buf.size());
}

} // namespace

void vprint_file(std::FILE* stream, std::string_view fmt, std::format_args args, bool newline) {
  format_to_file(stream, fmt, args, newline);
}

void vprint_ostream(std::ostream& os, std::string_view fmt, std::format_args args, bool newline) {
  std::ios_base::iostate err = std::ios_base::goodbit;
  if (std::ostream::sentry ok{os}) {
    // An exception from vformat propagates as is: no badbit, whatever exceptions() says (/4.2).
    bool formatting = true;
    ::ycxx::detail::guarded_io(
        os,
        [&] {
          std::string out = std::vformat(os.getloc(), fmt, args);
          formatting = false;
          if (newline)
            out.push_back('\n');
          const std::streamsize n = static_cast<std::streamsize>(out.size());
          if (os.rdbuf()->sputn(out.data(), n) != n)
            err |= std::ios_base::badbit;
        },
        formatting);
  }
  if (err)
    os.setstate(err);
}

} // namespace ycxx::detail

namespace std {

void vprint_unicode(FILE* stream, string_view fmt, format_args args) {
  ycxx::detail::format_to_file(stream, fmt, args, false);
}
void vprint_unicode_buffered(FILE* stream, string_view fmt, format_args args) {
  ycxx::detail::format_to_file(stream, fmt, args, false);
}
void vprint_nonunicode(FILE* stream, string_view fmt, format_args args) {
  ycxx::detail::format_to_file(stream, fmt, args, false);
}
void vprint_nonunicode_buffered(FILE* stream, string_view fmt, format_args args) {
  ycxx::detail::format_to_file(stream, fmt, args, false);
}
void vprint_unicode(string_view fmt, format_args args) {
  ycxx::detail::format_to_file(stdout, fmt, args, false);
}
void vprint_nonunicode(string_view fmt, format_args args) {
  ycxx::detail::format_to_file(stdout, fmt, args, false);
}
void println(FILE* stream) {
  ycxx::detail::write_file(stream, "\n", 1);
}
void println() {
  ycxx::detail::write_file(stdout, "\n", 1);
}

void vprint_unicode(ostream& os, string_view fmt, format_args args) {
  ycxx::detail::vprint_ostream(os, fmt, args, false);
}
void vprint_nonunicode(ostream& os, string_view fmt, format_args args) {
  ycxx::detail::vprint_ostream(os, fmt, args, false);
}
void println(ostream& os) {
  ycxx::detail::vprint_ostream(os, "\n", format_args(make_format_args()), false);
}

} // namespace std
