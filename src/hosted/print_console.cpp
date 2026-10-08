// libycxx hosted runtime, without a C library (YCXX_PAL=none without the 'clib' layer, DECISIONS
// §18): the print functions to standard output ([print.fun]) over the 'console' hosted layer,
// ycxx_pal_write to ycxx_pal_stdout. Built instead of src/hosted/print.cpp's standard output
// functions, which use C's stdout. As there, the whole output is formatted first and then
// written, so a format_error writes nothing; a failed write throws system_error with the
// provider's error number (generic category: the PAL returns errno-style codes).
#include <print>
#include <system_error>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

namespace {

void write_stdout(const char* p, std::size_t n) {
  while (n != 0) {
    ycxx_pal_size __w = 0;
    if (const int e = ::ycxx_pal_write(ycxx_pal_stdout, p, n, &__w); e != 0)
      throw std::system_error(e, std::generic_category(), "std::print: writing to the console failed");
    if (__w == 0)
      throw std::system_error(static_cast<int>(std::errc::io_error), std::generic_category(),
                              "std::print: the console accepted nothing");
    p += __w;
    n -= __w;
  }
}

} // namespace

void __vprint_stdout(std::string_view __fmt, std::format_args __args, bool __newline) {
  __fmt_dynbuf<char> __buf;
  ::__ycxx::__detail::__fmt_vformat(__buf, __fmt, __args, nullptr);
  if (__newline)
    __buf.push_back('\n');
  write_stdout(__buf.data(), __buf.size());
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// A console needs no native Unicode API: the UTF-8 goes out unchanged ([print.fun]/10.1).
void vprint_unicode(string_view __fmt, format_args __args) { __ycxx::__detail::__vprint_stdout(__fmt, __args, false); }
void vprint_nonunicode(string_view __fmt, format_args __args) { __ycxx::__detail::__vprint_stdout(__fmt, __args, false); }
void println() { __ycxx::__detail::write_stdout("\n", 1); }

}} // namespace std
