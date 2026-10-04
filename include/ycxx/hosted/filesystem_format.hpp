// libycxx hosted: formatter<filesystem::path, charT> ([fs.path.fmtr]). Needs only the core of
// <format>. path::value_type is char (UTF-8); for wchar_t the path is transcoded as wstring()
// does (UTF-8 to UTF-32, the implementation-defined transcoding of [fs.path.fmtr.funcs]/5).
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/hosted/filesystem.hpp>

namespace std {

template <ycxx::detail::fmt_char charT>
struct formatter<filesystem::path, charT> {
private:
  ycxx::detail::fmt_spec<charT> spec_; // fill, align, width; type '?' for the debug format
  bool generic_ = false;

public:
  constexpr void set_debug_format() { spec_.type = '?'; }

  // path-format-spec: fill-and-align(opt) width(opt) ?(opt) g(opt)
  constexpr typename basic_format_parse_context<charT>::iterator parse(basic_format_parse_context<charT>& ctx) {
    auto p = ycxx::detail::fmt_parse_fill_align(ctx.begin(), ctx.end(), spec_);
    p = ycxx::detail::fmt_parse_width(ctx, p, ctx.end(), spec_);
    if (p != ctx.end() && *p == charT('?'))
      spec_.type = '?', ++p;
    if (p != ctx.end() && *p == charT('g'))
      generic_ = true, ++p;
    if (p != ctx.end() && *p != charT('}'))
      ycxx::detail::throw_format_error("std::formatter<std::filesystem::path>: invalid path-format-spec");
    return p;
  }

  template <class FormatContext>
  typename FormatContext::iterator format(const filesystem::path& p, FormatContext& ctx) const {
    if constexpr (is_same_v<charT, filesystem::path::value_type>) {
      if (!generic_)
        return ycxx::detail::fmt_write_string(ctx, p.native().data(), p.native().size(), spec_);
    }
    const basic_string<charT> s = generic_ ? p.template generic_string<charT>() : p.template string<charT>();
    return ycxx::detail::fmt_write_string(ctx, s.data(), s.size(), spec_);
  }
};

template <>
inline constexpr bool enable_nonlocking_formatter_optimization<filesystem::path> = true;

} // namespace std
