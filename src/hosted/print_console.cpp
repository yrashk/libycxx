// libycxx hosted runtime, without a C library (YCXX_PAL=none without the 'clib' layer, DECISIONS
// §18): the print functions to standard output ([print.fun]) over the 'console' hosted layer,
// ycxx_pal_write to ycxx_pal_stdout. Built instead of src/hosted/print.cpp's standard output
// functions, which use C's stdout. As there, the whole output is formatted first and then
// written, so a format_error writes nothing; a failed write throws system_error with the
// provider's error number (generic category: the PAL returns errno-style codes).
#include <print>
#include <system_error>
#include <ycxx/pal.h>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

namespace {

void write_stdout(const char* p, std::size_t n) {
  while (n != 0) {
    ycxx_pal_size w = 0;
    if (const int e = ::ycxx_pal_write(ycxx_pal_stdout, p, n, &w); e != 0)
      throw std::system_error(e, std::generic_category(), "std::print: writing to the console failed");
    if (w == 0)
      throw std::system_error(static_cast<int>(std::errc::io_error), std::generic_category(),
                              "std::print: the console accepted nothing");
    p += w;
    n -= w;
  }
}

} // namespace

void vprint_stdout(std::string_view fmt, std::format_args args, bool newline) {
  fmt_dynbuf<char> buf;
  ::ycxx::detail::fmt_vformat(buf, fmt, args, nullptr);
  if (newline)
    buf.push_back('\n');
  write_stdout(buf.data(), buf.size());
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// A console needs no native Unicode API: the UTF-8 goes out unchanged ([print.fun]/10.1).
void vprint_unicode(string_view fmt, format_args args) { ycxx::detail::vprint_stdout(fmt, args, false); }
void vprint_nonunicode(string_view fmt, format_args args) { ycxx::detail::vprint_stdout(fmt, args, false); }
void println() { ycxx::detail::write_stdout("\n", 1); }

} // namespace std
