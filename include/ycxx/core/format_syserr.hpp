// libycxx core: formatter<error_code> ([syserr.fmt]).
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/core/system_error.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// ec.message() in charT's encoding: UTF-8 with each maximal ill-formed subsequence replaced by
// U+FFFD for char ([syserr.fmt]/5.1), and UTF-32 (or UTF-16), decoded the same way, for wchar_t.
template <class charT>
std::basic_string<charT> fmt_error_message(const std::string& m) {
  std::basic_string<charT> out;
  out.reserve(m.size());
  const char* p = m.data();
  const char* const e = p + m.size();
  while (p != e) {
    const uni::decoded d = ::ycxx::detail::uni::decode(p, e);
    char32_t c = d.ok ? d.cp : U'�';
    if constexpr (uni::encoding<char> == 0) {
      c = static_cast<unsigned char>(*p); // no transcoding outside UTF-8
    }
    if constexpr (__is_same(charT, char)) {
      if (d.ok || uni::encoding<char> == 0)
        out.append(p, d.len);
      else
        out.append("\xef\xbf\xbd", 3);
    } else if constexpr (uni::encoding<wchar_t> == 16) {
      if (c >= 0x10000) {
        out.push_back(static_cast<wchar_t>(0xd800 + ((c - 0x10000) >> 10)));
        out.push_back(static_cast<wchar_t>(0xdc00 + ((c - 0x10000) & 0x3ff)));
      } else {
        out.push_back(static_cast<wchar_t>(c));
      }
    } else {
      out.push_back(static_cast<wchar_t>(c));
    }
    p += d.len;
  }
  return out;
}
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <ycxx::detail::fmt_char charT>
struct formatter<error_code, charT> {
private:
  ycxx::detail::fmt_spec<charT> spec_;
  bool debug_ = false;
  bool message_ = false;

public:
  constexpr void set_debug_format() { debug_ = true; }

  constexpr typename basic_format_parse_context<charT>::iterator parse(basic_format_parse_context<charT>& ctx) {
    auto p = ctx.begin();
    const auto e = ctx.end();
    p = ycxx::detail::fmt_parse_fill_align(p, e, spec_);
    p = ycxx::detail::fmt_parse_width(ctx, p, e, spec_);
    if (p != e && *p == charT('?'))
      debug_ = true, ++p;
    if (p != e && *p == charT('s'))
      message_ = true, ++p;
    if (p != e && *p != charT('}'))
      ycxx::detail::throw_format_error("std::formatter<std::error_code>: invalid error-code-format-spec");
    return p;
  }

  template <class Out>
  typename basic_format_context<Out, charT>::iterator format(const error_code& ec,
                                                             basic_format_context<Out, charT>& ctx) const {
    basic_string<charT> msg;
    if (message_) {
      msg = ycxx::detail::fmt_error_message<charT>(ec.message());
    } else {
      for (const char* n = ec.category().name(); *n != 0; ++n)
        msg.push_back(static_cast<charT>(static_cast<unsigned char>(*n)));
      msg.push_back(charT(':'));
      char buf[16];
      const to_chars_result r = ycxx::detail::to_chars_integer(buf, buf + sizeof(buf), ec.value(), 10);
      for (const char* q = buf; q != r.ptr; ++q)
        msg.push_back(static_cast<charT>(*q));
    }
    ycxx::detail::fmt_spec<charT> s = spec_;
    s.type = debug_ ? '?' : 0;
    return ycxx::detail::fmt_write_string(ctx, msg.data(), msg.size(), s);
  }
};

template <>
inline constexpr bool enable_nonlocking_formatter_optimization<error_code> = true;

} // namespace std
