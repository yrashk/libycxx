// libycxx core: formatter<error_code> ([syserr.fmt]).
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/core/system_error.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// ec.message() in charT's encoding: UTF-8 with each maximal ill-formed subsequence replaced by
// U+FFFD for char ([syserr.fmt]/5.1), and UTF-32 (or UTF-16), decoded the same way, for wchar_t.
template <class __charT>
std::basic_string<__charT> __fmt_error_message(const std::string& m) {
  std::basic_string<__charT> out;
  out.reserve(m.size());
  const char* p = m.data();
  const char* const e = p + m.size();
  while (p != e) {
    const __uni::__decoded d = ::__ycxx::__detail::__uni::__decode(p, e);
    char32_t c = d.ok ? d.__cp : U'�';
    if constexpr (__uni::encoding<char> == 0) {
      c = static_cast<unsigned char>(*p); // no transcoding outside UTF-8
    }
    if constexpr (__is_same(__charT, char)) {
      if (d.ok || __uni::encoding<char> == 0)
        out.append(p, d.__len);
      else
        out.append("\xef\xbf\xbd", 3);
    } else if constexpr (__uni::encoding<wchar_t> == 16) {
      if (c >= 0x10000) {
        out.push_back(static_cast<wchar_t>(0xd800 + ((c - 0x10000) >> 10)));
        out.push_back(static_cast<wchar_t>(0xdc00 + ((c - 0x10000) & 0x3ff)));
      } else {
        out.push_back(static_cast<wchar_t>(c));
      }
    } else {
      out.push_back(static_cast<wchar_t>(c));
    }
    p += d.__len;
  }
  return out;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <__ycxx::__detail::__fmt_char __charT>
struct formatter<error_code, __charT> {
private:
  __ycxx::__detail::__fmt_spec<__charT> __spec_;
  bool __debug_ = false;
  bool __message_ = false;

public:
  constexpr void set_debug_format() { __debug_ = true; }

  constexpr typename basic_format_parse_context<__charT>::iterator parse(basic_format_parse_context<__charT>& __ctx) {
    auto p = __ctx.begin();
    const auto e = __ctx.end();
    p = __ycxx::__detail::__fmt_parse_fill_align(p, e, __spec_);
    p = __ycxx::__detail::__fmt_parse_width(__ctx, p, e, __spec_);
    if (p != e && *p == __charT('?'))
      __debug_ = true, ++p;
    if (p != e && *p == __charT('s'))
      __message_ = true, ++p;
    if (p != e && *p != __charT('}'))
      __ycxx::__detail::__throw_format_error("std::formatter<std::error_code>: invalid error-code-format-spec");
    return p;
  }

  template <class _Out>
  typename basic_format_context<_Out, __charT>::iterator format(const error_code& ec,
                                                             basic_format_context<_Out, __charT>& __ctx) const {
    basic_string<__charT> __msg;
    if (__message_) {
      __msg = __ycxx::__detail::__fmt_error_message<__charT>(ec.message());
    } else {
      for (const char* n = ec.category().name(); *n != 0; ++n)
        __msg.push_back(static_cast<__charT>(static_cast<unsigned char>(*n)));
      __msg.push_back(__charT(':'));
      char __buf[16];
      const to_chars_result r = __ycxx::__detail::__to_chars_integer(__buf, __buf + sizeof(__buf), ec.value(), 10);
      for (const char* __q = __buf; __q != r.ptr; ++__q)
        __msg.push_back(static_cast<__charT>(*__q));
    }
    __ycxx::__detail::__fmt_spec<__charT> s = __spec_;
    s.type = __debug_ ? '?' : 0;
    return __ycxx::__detail::__fmt_write_string(__ctx, __msg.data(), __msg.size(), s);
  }
};

template <>
inline constexpr bool enable_nonlocking_formatter_optimization<error_code> = true;

}} // namespace std
