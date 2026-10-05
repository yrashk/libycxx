// libycxx hosted: the text representation of thread::id ([thread.thread.id]/2): operator<< and
// formatter<thread::id, charT>. The representation is the decimal value of the PAL thread handle
// (0 for an id that represents no thread). Needs only the core of <format>.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/hosted/thread.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// The decimal digits of id's representation, written backwards ending at end.
inline char* thread_id_chars(char* end, std::thread::id id) noexcept {
  return ::ycxx::detail::charconv_write_unsigned(end, static_cast<unsigned long long>(thread_access::handle_of(id)), 10);
}
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& out, thread::id id) {
  return out << static_cast<unsigned long long>(ycxx::detail::thread_access::handle_of(id));
}

// thread-id-format-spec: fill-and-align(opt) width(opt); the default alignment is right.
template <ycxx::detail::fmt_char charT>
struct formatter<thread::id, charT> {
private:
  ycxx::detail::fmt_spec<charT> spec_;

public:
  constexpr typename basic_format_parse_context<charT>::iterator parse(basic_format_parse_context<charT>& ctx) {
    auto p = ycxx::detail::fmt_parse_fill_align(ctx.begin(), ctx.end(), spec_);
    p = ycxx::detail::fmt_parse_width(ctx, p, ctx.end(), spec_);
    if (p != ctx.end() && *p != charT('}'))
      ycxx::detail::throw_format_error("std::formatter<std::thread::id>: invalid thread-id-format-spec");
    return p;
  }

  template <class FormatContext>
  typename FormatContext::iterator format(thread::id id, FormatContext& ctx) const {
    char buf[24];
    char* const end = buf + sizeof(buf);
    const char* const first = ycxx::detail::thread_id_chars(end, id);
    charT text[24];
    const size_t n = static_cast<size_t>(end - first);
    for (size_t i = 0; i != n; ++i)
      text[i] = static_cast<charT>(first[i]);
    return ycxx::detail::fmt_write_padded<charT>(ctx.out(), spec_, ycxx::detail::fmt_align::right,
                                                 ycxx::detail::fmt_width(spec_, ctx), n, text, n);
  }
};

template <>
inline constexpr bool enable_nonlocking_formatter_optimization<thread::id> = true;

} // namespace std
