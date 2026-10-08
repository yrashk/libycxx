// libycxx hosted: the text representation of thread::id ([thread.thread.id]/2): operator<< and
// formatter<thread::id, charT>. The representation is the decimal value of the PAL thread handle
// (0 for an id that represents no thread). Needs only the core of <format>.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/hosted/thread.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// The decimal digits of id's representation, written backwards ending at end.
inline char* __thread_id_chars(char* end, std::thread::id id) noexcept {
  return ::__ycxx::__detail::__charconv_write_unsigned(end, static_cast<unsigned long long>(__thread_access::__handle_of(id)), 10);
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [thread.thread.id]/9: "Inserts the text representation for charT of id", the one formatter uses,
// as a character sequence: the stream's basefield, showpos... and its locale's numpunct do not
// change it; width, fill and adjustfield pad it as any string ([ostream.formatted.reqmts]/3).
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& out, thread::id id) {
  char __buf[24];
  char* const end = __buf + sizeof(__buf);
  const char* const first = __ycxx::__detail::__thread_id_chars(end, id);
  __charT __text[sizeof(__buf) + 1];
  const size_t n = static_cast<size_t>(end - first);
  for (size_t i = 0; i != n; ++i)
    __text[i] = static_cast<__charT>(first[i]);
  __text[n] = __charT();
  return out << static_cast<const __charT*>(__text);
}

// thread-id-format-spec: fill-and-align(opt) width(opt); the default alignment is right.
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<thread::id, __charT> {
private:
  __ycxx::__detail::__fmt_spec<__charT> __spec_;

public:
  constexpr typename basic_format_parse_context<__charT>::iterator parse(basic_format_parse_context<__charT>& __ctx) {
    auto p = __ycxx::__detail::__fmt_parse_fill_align(__ctx.begin(), __ctx.end(), __spec_);
    p = __ycxx::__detail::__fmt_parse_width(__ctx, p, __ctx.end(), __spec_);
    if (p != __ctx.end() && *p != __charT('}'))
      __ycxx::__detail::__throw_format_error("std::formatter<std::thread::id>: invalid thread-id-format-spec");
    return p;
  }

  template <class _FormatContext>
  typename _FormatContext::iterator format(thread::id id, _FormatContext& __ctx) const {
    char __buf[24];
    char* const end = __buf + sizeof(__buf);
    const char* const first = __ycxx::__detail::__thread_id_chars(end, id);
    __charT __text[24];
    const size_t n = static_cast<size_t>(end - first);
    for (size_t i = 0; i != n; ++i)
      __text[i] = static_cast<__charT>(first[i]);
    return __ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), __spec_, __ycxx::__detail::__fmt_align::right,
                                                 __ycxx::__detail::__fmt_width(__spec_, __ctx), n, __text, n);
  }
};

template <>
inline constexpr bool enable_nonlocking_formatter_optimization<thread::id> = true;

}} // namespace std
