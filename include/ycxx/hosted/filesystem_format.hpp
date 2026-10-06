// libycxx hosted: formatter<filesystem::path, charT> ([fs.path.fmtr]). Needs only the core of
// <format>. path::value_type is char (UTF-8); for wchar_t the path is transcoded as wstring()
// does (UTF-8 to UTF-32, the implementation-defined transcoding of [fs.path.fmtr.funcs]/5).
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/hosted/filesystem.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <__ycxx::__detail::__fmt_char __charT>
struct formatter<filesystem::path, __charT> {
private:
  __ycxx::__detail::__fmt_spec<__charT> __spec_; // fill, align, width; type '?' for the debug format
  bool __generic_ = false;

public:
  constexpr void set_debug_format() { __spec_.type = '?'; }

  // path-format-spec: fill-and-align(opt) width(opt) ?(opt) g(opt)
  constexpr typename basic_format_parse_context<__charT>::iterator parse(basic_format_parse_context<__charT>& __ctx) {
    auto p = __ycxx::__detail::__fmt_parse_fill_align(__ctx.begin(), __ctx.end(), __spec_);
    p = __ycxx::__detail::__fmt_parse_width(__ctx, p, __ctx.end(), __spec_);
    if (p != __ctx.end() && *p == __charT('?'))
      __spec_.type = '?', ++p;
    if (p != __ctx.end() && *p == __charT('g'))
      __generic_ = true, ++p;
    if (p != __ctx.end() && *p != __charT('}'))
      __ycxx::__detail::__throw_format_error("std::formatter<std::filesystem::path>: invalid path-format-spec");
    return p;
  }

  template <class _FormatContext>
  typename _FormatContext::iterator format(const filesystem::path& p, _FormatContext& __ctx) const {
    if constexpr (is_same_v<__charT, filesystem::path::value_type>) {
      if (!__generic_)
        return __ycxx::__detail::__fmt_write_string(__ctx, p.native().data(), p.native().size(), __spec_);
    }
    const basic_string<__charT> s = __generic_ ? p.template generic_string<__charT>() : p.template string<__charT>();
    return __ycxx::__detail::__fmt_write_string(__ctx, s.data(), s.size(), __spec_);
  }
};

template <>
inline constexpr bool enable_nonlocking_formatter_optimization<filesystem::path> = true;

} // namespace std
